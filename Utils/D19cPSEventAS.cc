#include "Utils/D19cPSEventAS.h"
#include "HWDescription/Definition.h"
#include "Utils/ChannelGroupHandler.h"
#include "Utils/ContainerFactory.h"
#include "Utils/DataContainer.h"
#include "Utils/EmptyContainer.h"
#include "Utils/Occupancy.h"
#include <algorithm>
#include <numeric>

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{

bool D19cPSEventAS::fEnableFastReadout = true;

void D19cPSEventAS::configureFastReadout(bool enableFastReadout) { fEnableFastReadout = enableFastReadout; }

D19cPSEventAS::D19cPSEventAS(const BeBoard* pBoard, const std::vector<uint32_t>& list) { this->Set(pBoard, list); }

void D19cPSEventAS::Set(const BeBoard* pBoard, const std::vector<uint32_t>& pData)
{
    ContainerFactory::copyAndInitChannel<uint16_t>(*pBoard, fTheOccupancyContainer);
    if(fEnableFastReadout)
    {
        size_t numberOfParsedOpticalGroups = 0;
        size_t numberOfChannels            = NSSACHANNELS * (NMPAROWS + 1);
        size_t hybridDataSize              = 8;

        for(auto theOpticalGroup: *pBoard)
        {
            for(size_t dataCounterPacketNumber = 0; dataCounterPacketNumber < numberOfChannels; ++dataCounterPacketNumber)
            {
                uint8_t pixelCol = dataCounterPacketNumber % NSSACHANNELS;
                uint8_t pixelRow = dataCounterPacketNumber / NSSACHANNELS;

                for(auto theHybrid: *theOpticalGroup)
                {
                    size_t hybridDataStart = numberOfChannels * hybridDataSize * numberOfParsedOpticalGroups * 2 + (dataCounterPacketNumber * 2 + (theHybrid->getId() % 2)) * hybridDataSize;

                    for(uint8_t counterNumber = 0; counterNumber < 8; ++counterNumber)
                    {
                        uint32_t counterStart = 21 * counterNumber;
                        uint32_t counterEnd   = 21 * (counterNumber + 1) - 1;

                        uint32_t firstWord  = counterStart / 32;
                        uint32_t secondWord = counterEnd / 32;

                        uint32_t counterPacket;
                        if(firstWord == secondWord) { counterPacket = (pData.at(hybridDataStart + firstWord) >> (counterStart % 32)) & 0x1FFFFF; }
                        else { counterPacket = (pData.at(hybridDataStart + firstWord) >> (counterStart % 32) | (pData.at(hybridDataStart + secondWord) << (32 - counterStart % 32))) & 0x1FFFFF; }

                        uint16_t rawCounterData = (((counterPacket >> 8) & 0x7F) | ((counterPacket & 0x7F) << 7));
                        if(rawCounterData == 0) continue;
                        uint8_t chipId = getChipIdMapped(theHybrid->getId(), (counterPacket >> 15) & 0x7) + (pixelRow < 16 ? 8 : 0);
                        try
                        {
                            fTheOccupancyContainer.getChip(theOpticalGroup->getId(), theHybrid->getId(), chipId)->getChannel<uint16_t>(pixelRow % 16, pixelCol) = rawCounterData - 1;
                        }
                        catch(const std::exception& e)
                        {
                            // do nothing, the chip is disabled
                        }
                    }
                }
            }
            ++numberOfParsedOpticalGroups;
        }

        // Last and first pixel not present in the fast readout, but by construction must have the same entries of the neighbour
        for(auto theOpticalGroup: fTheOccupancyContainer)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                for(auto theChip: *theHybrid)
                {
                    if(theChip->getNumberOfRows() == NMPAROWS)
                    {
                        theChip->getChannel<uint16_t>(0, 0)    = theChip->getChannel<uint16_t>(0, 1);
                        theChip->getChannel<uint16_t>(15, 119) = theChip->getChannel<uint16_t>(15, 118);
                    }
                }
            }
        }
    }
    else
    {
        size_t dataIndex = 0;
        for(auto theOpticalGroup: fTheOccupancyContainer)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                for(auto theChip: *theHybrid)
                {
                    uint8_t numberOfCols = theChip->getNumberOfCols();
                    for(uint8_t row = 0; row < theChip->getNumberOfRows(); ++row)
                    {
                        for(uint8_t col = 0; col < numberOfCols; col += 2)
                        {
                            uint32_t rawCounter = pData.at(dataIndex++);
                            // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] rawCounter = " << std::hex << rawCounter << std::dec << std::endl;

                            theChip->getChannel<uint16_t>(row, col) = rawCounter & 0x7FFF;
                            // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] theChip->getChannel<uint16_t>(row, col) = " << std::hex << theChip->getChannel<uint16_t>(row, col) << std::dec
                            // << std::endl;
                            theChip->getChannel<uint16_t>(row, col + 1) = (rawCounter >> 16) & 0x7FFF;
                            // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] theChip->getChannel<uint16_t>(row, col + 1) = " << std::hex << theChip->getChannel<uint16_t>(row, col + 1) <<
                            // std::dec << std::endl;
                        }
                    }
                }
            }
        }
    }
}

void D19cPSEventAS::fillChipDataContainer(ChipDataContainer* chipContainer, const std::shared_ptr<ChannelGroupBase> testChannelGroup, uint8_t hybridId)
{
    if(testChannelGroup == nullptr) return;

    ChipDataContainer* theChipEventContainer;
    try
    {
        theChipEventContainer = fTheOccupancyContainer.getChip(hybridId / 2, hybridId, chipContainer->getId());
    }
    catch(const std::exception& e)
    {
        // handled disabled chip
        return;
    }

    for(uint8_t row = 0; row < theChipEventContainer->getNumberOfRows(); ++row)
    {
        for(uint8_t col = 0; col < theChipEventContainer->getNumberOfCols(); ++col)
        {
            if(testChannelGroup->isChannelEnabled(row, col)) { chipContainer->getChannel<Occupancy>(row, col).fOccupancy += theChipEventContainer->getChannel<uint16_t>(row, col); }
        }
    }
}

uint32_t D19cPSEventAS::GetNHits(uint8_t pHybridId, uint8_t pChipId)
{
    const auto& theChipEventChannelVector = fTheOccupancyContainer.getChip(pHybridId / 2, pHybridId % 2, pChipId)->getChannelContainer<uint16_t>();
    return std::accumulate(theChipEventChannelVector->begin(), theChipEventChannelVector->end(), 0);
}
} // namespace Ph2_HwInterface
