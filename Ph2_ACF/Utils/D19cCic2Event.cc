/*

        FileName :                     Event.cc
        Content :                      Event handling from DAQ
        Programmer :                   Nicolas PIERRE
        Version :                      1.0
        Date of creation :             10/07/14
        Support :                      mail to : nicolas.pierre@icloud.com

 */

#include "Utils/D19cCic2Event.h"
#include "HWDescription/BeBoard.h"
#include "HWDescription/Definition.h"
#include "HWDescription/OuterTrackerHybrid.h"
#include "Utils/ChannelGroupHandler.h"
#include "Utils/ContainerFactory.h"
#include "Utils/DataContainer.h"
#include "Utils/EmptyContainer.h"
#include "Utils/Occupancy.h"
#include "Utils/Utilities.h"
#include <cstdint>

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{

bool Cluster2S::parseData(uint32_t data)
{
    fFirstStrip   = ((data >> 3) & 0xFF) - 1;
    fClusterWidth = (data & 0x7) + 1;
    return fFirstStrip < NCHANNELS;
}

bool Cluster2S::isChannelHit(uint8_t channel) const { return channel >= fFirstStrip && channel < (fFirstStrip + fClusterWidth); }

void Cluster2S::print() const { std::cout << "First strip = " << +fFirstStrip << " cluster width = " << +fClusterWidth << std::endl; }

bool PixelClusterPS::parseData(uint32_t data)
{
    fAddress = ((data >> 7) & 0x7F) - 1;
    fWidth   = ((data >> 4) & 0x7) + 1;
    fZpos    = data & 0xF;
    return fAddress < NSSACHANNELS;
}

bool PixelClusterPS::isChannelHit(uint8_t row, uint8_t col) const { return col >= fAddress && col < (fAddress + fWidth) && row == fZpos; }

void PixelClusterPS::print() const { std::cout << "First pixel row = " << +fZpos << " col = " << +fAddress << " cluster width = " << +fWidth << std::endl; }

bool StripClusterPS::parseData(uint32_t data)
{
    fAddress = ((data >> 4) & 0x7F) - 1;
    fWidth   = ((data >> 1) & 0x7) + 1;
    fMip     = data & 0x1;
    return fAddress < NSSACHANNELS;
}

bool StripClusterPS::isChannelHit(uint8_t col) const { return col >= fAddress && col < (fAddress + fWidth); }

void StripClusterPS::print() const { std::cout << "First strip = " << +fAddress << " cluster width = " << +fWidth << std::endl; }

void HybridL1EventInfo::parseData(std::vector<uint32_t>::const_iterator dataStart)
{
    fErrorCode             = (*(dataStart) >> 24) & 0xF;
    fHybridId              = (*(dataStart) >> 16) & 0xFF;
    fChipId                = (*(dataStart) >> 12) & 0xF;
    fChipType              = (*(dataStart + 1) >> 12) & 0xF;
    fFrameDelay            = *(dataStart + 1) & 0xFFF;
    fStatusBits            = *(dataStart + 2) >> 23;
    fL1counter             = (*(dataStart + 2) >> 14) & 0x1FF;
    fNumberOfStripClusters = (*(dataStart + 2) >> 7) & 0x7F;
    fNumberOfPixelClusters = *(dataStart + 2) & 0x7F;
}

void HybridL1EventInfo::print() const
{
    std::cout << "ErrorCode             = " << +fErrorCode << std::endl;
    std::cout << "HybridId              = " << +fHybridId << std::endl;
    std::cout << "ChipId                = " << +fChipId << std::endl;
    std::cout << "ChipType              = " << +fChipType << std::endl;
    std::cout << "FrameDelay            = " << +fFrameDelay << std::endl;
    std::cout << "StatusBits            = " << +fStatusBits << std::endl;
    std::cout << "L1counter             = " << +fL1counter << std::endl;
    std::cout << "NumberOfStripClusters = " << +fNumberOfStripClusters << std::endl;
    std::cout << "NumberOfPixelClusters = " << +fNumberOfPixelClusters << std::endl;
}

void ChipL1EventInfo::parseData(std::array<uint32_t, NUMBER_OF_CIC_PORTS * 9>::const_iterator dataStart, size_t bitStart)
{
    fErrorCode       = getWord<2, 0x3>(dataStart, bitStart);
    fPipelineAddress = getWord<9, 0x1FF>(dataStart, bitStart + 2);
    fL1id            = getWord<9, 0x1FF>(dataStart, bitStart + 11);
    for(size_t wordIndex = 0; wordIndex < 7; ++wordIndex) { fRawData[wordIndex] = getWord<32, 0xFFFFFFFF>(dataStart, bitStart + 20 + (32 * wordIndex)); }
    fRawData[7] = getWord<30, 0x3FFFFFFF>(dataStart, bitStart + 20 + 32 * 7) << 2;
}

bool ChipL1EventInfo::isChannelHit(uint8_t channel) const { return (fRawData.at(channel / 32) >> (31 - channel % 32)) & 0x1; }

void ChipL1EventInfo::print() const
{
    std::cout << "ErrorCode             = " << +fErrorCode << std::endl;
    std::cout << "PipelineAddress       = " << +fPipelineAddress << std::endl;
    std::cout << "L1id                  = " << +fL1id << std::endl;
    std::vector<uint32_t> rawDataVector(fRawData.begin(), fRawData.end());
    std::cout << "RawData               = " << getPatternPrintout(rawDataVector, 1) << std::endl;
}

std::vector<uint8_t> ChipL1EventInfo::getChannelHitList() const
{
    std::vector<uint8_t> theHitList;
    theHitList.reserve(NCHANNELS);

    for(size_t channel = 0; channel < NCHANNELS; ++channel)
    {
        if(isChannelHit(channel)) theHitList.emplace_back(channel);
    }
    return theHitList;
}

uint8_t ChipL1EventInfo::countNumberOfHits() const
{
    uint8_t numberOfHits = 0;
    for(auto hitWord: fRawData) numberOfHits += __builtin_popcount(hitWord);
    return numberOfHits;
}

void HybridStubEventInfo::parseData(std::vector<uint32_t>::const_iterator dataStart)
{
    fStubDataDelay   = (*(dataStart) >> 12) & 0xFFF;
    fStatusBits      = (*(dataStart + 1) >> 22) & 0x1FF;
    fNumberOfStubs   = (*(dataStart + 1) >> 16) & 0x3F;
    fBunchCrossingId = *(dataStart + 1) & 0xFFF;
}

void HybridStubEventInfo::print() const
{
    std::cout << "StubDataDelay   = " << +fStubDataDelay << std::endl;
    std::cout << "StatusBits      = " << +fStatusBits << std::endl;
    std::cout << "NumberOfStubs   = " << +fNumberOfStubs << std::endl;
    std::cout << "BunchCrossingId = " << +fBunchCrossingId << std::endl;
}

bool EventStub::parseData(uint32_t data, bool is2S)
{
    if(is2S)
    {
        fPosition = ((data >> 4) & 0xFF);
        fBend     = (data & 0xF);
    }
    else
    {
        fPosition = ((data >> 7) & 0xFF);
        fBend     = ((data >> 4) & 0x7);
        fRow      = (data & 0xF);
    }
    return true;
}

void EventStub::print() const
{
    std::cout << "Seed = " << +fPosition;
    if(fRow != 0xFF) std::cout << " Z = " << +fRow;
    std::cout << " Bend = " << +fBend << std::endl;
}

bool      D19cCic2Event::ifAreDecodedEventContainersReady = false;
bool      D19cCic2Event::fIs2S                            = true;
bool      D19cCic2Event::fIsSparsified                    = true;
uintptr_t D19cCic2Event::fLastEventDecodedPointer         = reinterpret_cast<uintptr_t>(nullptr);

BoardDataContainer                            D19cCic2Event::fDecodedL1Event    = BoardDataContainer();
BoardDataContainer                            D19cCic2Event::fDecodedStubEvent  = BoardDataContainer();
std::array<uint32_t, NUMBER_OF_CIC_PORTS * 9> D19cCic2Event::fTheChipDataVector = std::array<uint32_t, NUMBER_OF_CIC_PORTS * 9>();

// Event implementation
D19cCic2Event::D19cCic2Event(const BeBoard* pBoard, std::vector<uint32_t>& list, bool pWithTLU) : fTLUenabled(pWithTLU), fBoard(pBoard)
{
    bool localIs2S         = pBoard->getFirstObject()->getFrontEndType() == FrontEndType::OuterTracker2S;
    bool localIsSparsified = pBoard->getSparsification();
    if(fIs2S != localIs2S || fIsSparsified != localIsSparsified)
    {
        fIsSparsified                    = localIsSparsified;
        fIs2S                            = localIs2S;
        ifAreDecodedEventContainersReady = false;
        fDecodedL1Event.reset();
        fDecodedStubEvent.reset();
    }

    if(!ifAreDecodedEventContainersReady)
    {
        if(fIs2S)
        {
            if(fIsSparsified) { ContainerFactory::copyAndInitStructure<EmptyContainer, ClusterCollection<Cluster2S, 31>, HybridL1EventInfo, EmptyContainer, EmptyContainer>(*pBoard, fDecodedL1Event); }
            else { ContainerFactory::copyAndInitStructure<EmptyContainer, ChipL1EventInfo, HybridL1EventInfo, EmptyContainer, EmptyContainer>(*pBoard, fDecodedL1Event); }
        }
        else
        {
            fDecodedL1Event.setId(pBoard->getId());
            fDecodedL1Event.initialize<EmptyContainer, EmptyContainer>();

            for(auto theOpticalGroup: *pBoard)
            {
                auto decodedEventOpticalGroup = fDecodedL1Event.addOpticalGroupDataContainer(theOpticalGroup->getId());
                decodedEventOpticalGroup->initialize<EmptyContainer, HybridL1EventInfo>();

                for(auto theHybrid: *theOpticalGroup)
                {
                    auto decodedEventHybrid = decodedEventOpticalGroup->addHybridDataContainer(theHybrid->getId());
                    decodedEventHybrid->initialize<HybridL1EventInfo, ClusterCollection<StripClusterPS, 32>>();

                    for(auto theChip: *theHybrid)
                    {
                        auto decodedEventChip = decodedEventHybrid->addChipDataContainer(theChip->getId(), theChip->getNumberOfRows(), theChip->getNumberOfCols());

                        if(theChip->getId() < 8) { ContainerFactory::copyAndInitChip<ClusterCollection<StripClusterPS, 32>>(*static_cast<ChipContainer*>(theChip), *decodedEventChip); }
                        else { ContainerFactory::copyAndInitChip<ClusterCollection<PixelClusterPS, 32>>(*static_cast<ChipContainer*>(theChip), *decodedEventChip); }
                    }
                }
            }
        }

        ContainerFactory::copyAndInitStructure<EmptyContainer, ClusterCollection<EventStub, 5>, HybridStubEventInfo, EmptyContainer, EmptyContainer>(*pBoard, fDecodedStubEvent);

        ifAreDecodedEventContainersReady = true;
    }

    fTLUenabled = (uint8_t)pWithTLU;

    fLocalData = std::move(list);
}

void D19cCic2Event::decodeEvent()
{
    if(fLastEventDecodedPointer == reinterpret_cast<uintptr_t>(this)) return;

    // decode Header
    if(fLocalData.size() < 4)
    {
        LOG(ERROR) << ERROR_FORMAT << "No event header received" << RESET;
        return;
    }

    if(fLocalData[0] >> 16 != 0xFFFF)
    {
        LOG(ERROR) << ERROR_FORMAT << "Invalid Header D19cCic2Event" << RESET;
        return;
    }

    size_t totalEventSize = fLocalData[0] & 0xFFFF;

    if(fLocalData.size() < totalEventSize)
    {
        LOG(ERROR) << ERROR_FORMAT << "Event incomplete" << RESET;
        return;
    }

    fExternalTriggerID = fLocalData[1] >> 16;
    fTDC               = ((fLocalData[2] >> 24) - 1) & 0x7;
    fEventCount        = fLocalData[2] & 0xFFFFFF;
    fBunch             = fLocalData[3];

    std::vector<uint32_t>::const_iterator theCurrentHybridDataPointer = fLocalData.begin() + 4;
    for(auto theOpticalGroup: *fBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            auto& theHybridL1EventContainer = fDecodedL1Event.getHybrid(theOpticalGroup->getId(), theHybrid->getId());
            if(fIsSparsified)
            {
                for(auto theChip: *theHybridL1EventContainer)
                {
                    if(fIs2S)
                        theChip->getSummary<ClusterCollection<Cluster2S, 31>>().fNumberOfClusters = 0;
                    else
                    {
                        if(theChip->getId() < 8)
                            theChip->getSummary<ClusterCollection<StripClusterPS, 32>>().fNumberOfClusters = 0;
                        else
                            theChip->getSummary<ClusterCollection<PixelClusterPS, 32>>().fNumberOfClusters = 0;
                    }
                }
            }
            uint16_t theL1EventDataSize = decodeHybridL1Event(theHybridL1EventContainer, theCurrentHybridDataPointer);

            theCurrentHybridDataPointer += theL1EventDataSize;

            auto& theHybridStubEventContainer = fDecodedStubEvent.getHybrid(theOpticalGroup->getId(), theHybrid->getId());
            for(auto theChip: *theHybridStubEventContainer) { theChip->getSummary<ClusterCollection<EventStub, 5>>().fNumberOfClusters = 0; }
            uint16_t theStubEventDataSize = decodeHybridStubEvent(theHybridStubEventContainer, theCurrentHybridDataPointer);

            theCurrentHybridDataPointer += theStubEventDataSize;
        }
    }

    fLastEventDecodedPointer = reinterpret_cast<uintptr_t>(this);
}

uint16_t D19cCic2Event::decodeHybridL1Event(HybridDataContainer* theHybridEventContainer, std::vector<uint32_t>::const_iterator dataStartIterator)
{
    auto theCicToChipMapping = getCicToChipMapping(theHybridEventContainer->getId());

    HybridL1EventInfo* theHybridL1EventInfoPointer;

    if(fIs2S)
    {
        if(fIsSparsified) { theHybridL1EventInfoPointer = &theHybridEventContainer->getSummary<HybridL1EventInfo, ClusterCollection<Cluster2S, 31>>(); }
        else { theHybridL1EventInfoPointer = &theHybridEventContainer->getSummary<HybridL1EventInfo, ChipL1EventInfo>(); }
    }
    else { theHybridL1EventInfoPointer = &theHybridEventContainer->getSummary<HybridL1EventInfo, ClusterCollection<StripClusterPS, 32>>(); }

    theHybridL1EventInfoPointer->parseData(dataStartIterator);

    size_t currentBitCount = 32 * 3;
    if(fIs2S)
    {
        if(fIsSparsified)
        {
            for(size_t clusterNumber = 0; clusterNumber < theHybridL1EventInfoPointer->fNumberOfStripClusters; ++clusterNumber)
            {
                uint32_t  theClusterWord = getWord<CLUSTER_2S_DATA_SIZE, CLUSTER_2S_DATA_MASK>(dataStartIterator + currentBitCount / 32, currentBitCount % 32);
                Cluster2S theCluster2S;
                if(theCluster2S.parseData(theClusterWord))
                {
                    theHybridEventContainer->getObject((*theCicToChipMapping)[theClusterWord >> (CLUSTER_2S_DATA_SIZE - 3)])->getSummary<ClusterCollection<Cluster2S, 31>>().addCluster(theCluster2S);
                }
                currentBitCount += CLUSTER_2S_DATA_SIZE;
            }
        }
        else
        {
            fTheChipDataVector.fill(0);

            uint32_t currentCBCwordOffset = 0;

            for(size_t numberOfCBCblock = 0; numberOfCBCblock < 25; ++numberOfCBCblock)
            {
                for(int chipIdForCIC = NUMBER_OF_CIC_PORTS - 1; chipIdForCIC >= 0; --chipIdForCIC)
                {
                    uint32_t currentWordOffsetQuotient  = currentCBCwordOffset / 32;
                    uint32_t currentWordOffsetRemainder = currentCBCwordOffset % 32;
                    uint32_t theWord                    = getWord<L1_UNSPARSFIED_BLOCK_SIZE_2S, 0x7FF>(dataStartIterator, currentBitCount);

                    currentBitCount += L1_UNSPARSFIED_BLOCK_SIZE_2S;

                    if(32 - L1_UNSPARSFIED_BLOCK_SIZE_2S >= currentWordOffsetRemainder)
                    {
                        fTheChipDataVector[chipIdForCIC * 9 + currentWordOffsetQuotient] |= (theWord << (32 - L1_UNSPARSFIED_BLOCK_SIZE_2S - currentWordOffsetRemainder));
                    }
                    else
                    {
                        fTheChipDataVector[chipIdForCIC * 9 + currentWordOffsetQuotient] |= (theWord >> (L1_UNSPARSFIED_BLOCK_SIZE_2S - 32 + currentWordOffsetRemainder));
                        fTheChipDataVector[chipIdForCIC * 9 + currentWordOffsetQuotient + 1] |= (theWord << (32 - (L1_UNSPARSFIED_BLOCK_SIZE_2S - 32 + currentWordOffsetRemainder)));
                    }
                }
                currentCBCwordOffset += L1_UNSPARSFIED_BLOCK_SIZE_2S;
            }

            for(auto* theChipEventContainer: *theHybridEventContainer)
            {
                theChipEventContainer->getSummary<ChipL1EventInfo>().parseData(fTheChipDataVector.begin() + 9 * getIdForCic(theHybridEventContainer->getId(), theChipEventContainer->getId()));
            }
        }
    }
    else
    {
        for(size_t stripClusterNumber = 0; stripClusterNumber < theHybridL1EventInfoPointer->fNumberOfStripClusters; ++stripClusterNumber)
        {
            uint32_t       theClusterWord = getWord<STRIP_CLUSTER_PS_DATA_SIZE, STRIP_CLUSTER_PS_DATA_MASK>(dataStartIterator + currentBitCount / 32, currentBitCount % 32);
            StripClusterPS theStripClusterPS;
            if(theStripClusterPS.parseData(theClusterWord))
            {
                theHybridEventContainer->getObject((*theCicToChipMapping)[theClusterWord >> (STRIP_CLUSTER_PS_DATA_SIZE - 3)])
                    ->getSummary<ClusterCollection<StripClusterPS, 32>>()
                    .addCluster(theStripClusterPS);
            }
            currentBitCount += STRIP_CLUSTER_PS_DATA_SIZE;
        }
        for(size_t pixelClusterNumber = 0; pixelClusterNumber < theHybridL1EventInfoPointer->fNumberOfPixelClusters; ++pixelClusterNumber)
        {
            uint32_t       theClusterWord = getWord<PIXEL_CLUSTER_PS_DATA_SIZE, PIXEL_CLUSTER_PS_DATA_MASK>(dataStartIterator + currentBitCount / 32, currentBitCount % 32);
            PixelClusterPS thePixelClusterPS;
            if(thePixelClusterPS.parseData(theClusterWord))
            {
                theHybridEventContainer->getObject((*theCicToChipMapping)[theClusterWord >> (PIXEL_CLUSTER_PS_DATA_SIZE - 3)] + 8)
                    ->getSummary<ClusterCollection<PixelClusterPS, 32>>()
                    .addCluster(thePixelClusterPS);
            }
            currentBitCount += PIXEL_CLUSTER_PS_DATA_SIZE;
        }
    }

    uint16_t hybridL1DataSize = ((*dataStartIterator) & 0xFFF) << 2; // << 2 is equal to * 4
    return hybridL1DataSize;
}

uint16_t D19cCic2Event::decodeHybridStubEvent(HybridDataContainer* theHybridStubEventContainer, std::vector<uint32_t>::const_iterator dataStartIterator)
{
    HybridStubEventInfo& theHybridStubEventInfo = theHybridStubEventContainer->getSummary<HybridStubEventInfo, ClusterCollection<EventStub, 5>>();
    theHybridStubEventInfo.parseData(dataStartIterator);

    auto theCicToChipMapping = getCicToChipMapping(theHybridStubEventContainer->getId());

    size_t currentBitCount = 32 * 2;
    for(size_t stubNumber = 0; stubNumber < theHybridStubEventInfo.fNumberOfStubs; ++stubNumber)
    {
        if(fIs2S)
        {
            uint32_t  theClusterWord = getWord<STUB_2S_DATA_SIZE, STUB_2S_DATA_MASK>(dataStartIterator + currentBitCount / 32, currentBitCount % 32);
            EventStub theEventStub;
            if(theEventStub.parseData(theClusterWord, true))
            {
                theHybridStubEventContainer->getObject((*theCicToChipMapping)[theClusterWord >> (STUB_2S_DATA_SIZE - 3)])->getSummary<ClusterCollection<EventStub, 5>>().addCluster(theEventStub);
            }
            currentBitCount += STUB_2S_DATA_SIZE;
        }
        else
        {
            uint32_t  theClusterWord = getWord<STUB_PS_DATA_SIZE, STUB_PS_DATA_MASK>(dataStartIterator + currentBitCount / 32, currentBitCount % 32);
            EventStub theEventStub;
            if(theEventStub.parseData(theClusterWord, false))
            {
                theHybridStubEventContainer->getObject((*theCicToChipMapping)[theClusterWord >> (STUB_PS_DATA_SIZE - 3)] + 8)->getSummary<ClusterCollection<EventStub, 5>>().addCluster(theEventStub);
            }
            currentBitCount += STUB_PS_DATA_SIZE;
        }
    }

    uint16_t hybridStubDataSize = ((*dataStartIterator) & 0xFFF) << 2; // << 2 is equal to * 4
    return hybridStubDataSize;
}

const std::vector<uint8_t>* D19cCic2Event::getChipToCicMapping(uint16_t theHybridId) const
{
    const std::vector<uint8_t>* theChipToCicMapping;
    if(fIs2S)
        theChipToCicMapping = &fChipToCicMapping2S;
    else
        theChipToCicMapping = theHybridId % 2 == 0 ? &fChipToCicMappingPSR : &fChipToCicMappingPSL;
    return theChipToCicMapping;
}

const std::vector<uint8_t>* D19cCic2Event::getCicToChipMapping(uint16_t theHybridId) const
{
    const std::vector<uint8_t>* theCicToChipMapping;
    if(fIs2S)
        theCicToChipMapping = &fCicToChipMapping2S;
    else
        theCicToChipMapping = theHybridId % 2 == 0 ? &fCicToChipMappingPSR : &fCicToChipMappingPSL;
    return theCicToChipMapping;
}

void D19cCic2Event::fillChipDataContainer(ChipDataContainer* chipContainer, const std::shared_ptr<ChannelGroupBase> testChannelGroup, uint8_t hybridId)
{
    decodeEvent();
    auto               readoutChipId = chipContainer->getId();
    ChipDataContainer* theChipL1Container;
    try
    {
        theChipL1Container = fDecodedL1Event.getChip(hybridId / 2, hybridId, readoutChipId);
    }
    catch(const std::exception& e)
    {
        // hybrid was disabled
        return;
    }
    if(theChipL1Container == nullptr) return;

    auto updateIfUnmasked = [chipContainer, &testChannelGroup](uint16_t row, uint16_t col)
    {
        if(testChannelGroup->isChannelEnabled(row, col)) { chipContainer->getChannel<Occupancy>(row, col).fOccupancy += 1.; }
    };

    if(fIsSparsified)
    {
        if(fIs2S)
        {
            for(auto theCluster: theChipL1Container->getSummary<ClusterCollection<Cluster2S, 31>>())
            {
                for(size_t channel = theCluster.fFirstStrip; channel < theCluster.fFirstStrip + theCluster.fClusterWidth; ++channel) { updateIfUnmasked(0, channel); }
            }
        }
        else
        {
            if(readoutChipId < 8)
            {
                for(auto theCluster: theChipL1Container->getSummary<ClusterCollection<StripClusterPS, 32>>())
                {
                    for(size_t channel = theCluster.fAddress; channel < theCluster.fAddress + theCluster.fWidth; ++channel) { updateIfUnmasked(0, channel); }
                }
            }
            else
            {
                for(auto theCluster: theChipL1Container->getSummary<ClusterCollection<PixelClusterPS, 32>>())
                {
                    for(size_t channel = theCluster.fAddress; channel < theCluster.fAddress + theCluster.fWidth; ++channel) { updateIfUnmasked(theCluster.fZpos, channel); }
                }
            }
        }
    }
    else
    {
        for(auto theHitChannel: theChipL1Container->getSummary<ChipL1EventInfo>().getChannelHitList()) { updateIfUnmasked(0, theHitChannel); }
    }
}

ClusterCollection<PixelClusterPS, 32> D19cCic2Event::GetPixelClusters(uint8_t pHybridId, uint8_t pReadoutChipId)
{
    decodeEvent();
    if(fIs2S || pReadoutChipId < 8)
    {
        std::cerr << "D19cCic2Event::GetPixelClusters can be called only for MPA, aborting" << std::endl;
        abort();
    }
    return fDecodedL1Event.getChip(pHybridId / 2, pHybridId, pReadoutChipId)->getSummary<ClusterCollection<PixelClusterPS, 32>>();
}

ClusterCollection<StripClusterPS, 32> D19cCic2Event::GetStripClusters(uint8_t pHybridId, uint8_t pReadoutChipId)
{
    decodeEvent();

    if(fIs2S || pReadoutChipId >= 8)
    {
        std::cerr << "D19cCic2Event::getClusters can be called only for SSA, aborting" << std::endl;
        abort();
    }
    return fDecodedL1Event.getChip(pHybridId / 2, pHybridId, pReadoutChipId)->getSummary<ClusterCollection<StripClusterPS, 32>>();
}

uint16_t D19cCic2Event::L1Status(uint8_t pHybridId)
{
    decodeEvent();

    auto                     theHybridL1Event = fDecodedL1Event.getHybrid(pHybridId / 2, pHybridId);
    const HybridL1EventInfo* theHybridL1EventInfoPointer;
    if(fIsSparsified)
    {
        if(fIs2S)
            theHybridL1EventInfoPointer = &theHybridL1Event->getSummary<HybridL1EventInfo, ClusterCollection<Cluster2S, 31>>();
        else
            theHybridL1EventInfoPointer = &theHybridL1Event->getSummary<HybridL1EventInfo, ClusterCollection<StripClusterPS, 32>>();
    }
    else { theHybridL1EventInfoPointer = &theHybridL1Event->getSummary<HybridL1EventInfo, ChipL1EventInfo>(); }
    return theHybridL1EventInfoPointer->fStatusBits;
}

uint32_t D19cCic2Event::Error(uint8_t pHybridId, uint8_t pReadoutChipId)
{
    decodeEvent();

    if(fIsSparsified)
    {
        auto                     theHybridL1Event = fDecodedL1Event.getHybrid(pHybridId / 2, pHybridId);
        const HybridL1EventInfo* theHybridL1EventInfoPointer;
        if(fIs2S)
            theHybridL1EventInfoPointer = &theHybridL1Event->getSummary<HybridL1EventInfo, ClusterCollection<Cluster2S, 31>>();
        else
            theHybridL1EventInfoPointer = &theHybridL1Event->getSummary<HybridL1EventInfo, ClusterCollection<StripClusterPS, 32>>();
        auto theCicStatusBits = theHybridL1EventInfoPointer->fStatusBits;
        return theCicStatusBits >> (getIdForCic(pHybridId, pReadoutChipId) + 1);
    }
    else { return fDecodedL1Event.getChip(pHybridId / 2, pHybridId, pReadoutChipId)->getSummary<ChipL1EventInfo>().fErrorCode; }
}

uint32_t D19cCic2Event::BxId(uint8_t pHybridId)
{
    decodeEvent();
    return fDecodedStubEvent.getHybrid(pHybridId / 2, pHybridId)->getSummary<HybridStubEventInfo, ClusterCollection<EventStub, 5>>().fBunchCrossingId;
}

uint16_t D19cCic2Event::StubStatus(uint8_t pHybridId)
{
    decodeEvent();
    return fDecodedStubEvent.getHybrid(pHybridId / 2, pHybridId)->getSummary<HybridStubEventInfo, ClusterCollection<EventStub, 5>>().fStatusBits;
}

uint32_t D19cCic2Event::L1Id(uint8_t pHybridId, uint8_t pReadoutChipId)
{
    decodeEvent();
    if(fIsSparsified)
    {
        auto                     theHybridL1Event = fDecodedL1Event.getHybrid(pHybridId / 2, pHybridId);
        const HybridL1EventInfo* theHybridL1EventInfoPointer;
        if(fIs2S)
            theHybridL1EventInfoPointer = &theHybridL1Event->getSummary<HybridL1EventInfo, ClusterCollection<Cluster2S, 31>>();
        else
            theHybridL1EventInfoPointer = &theHybridL1Event->getSummary<HybridL1EventInfo, ClusterCollection<StripClusterPS, 32>>();
        return theHybridL1EventInfoPointer->fL1counter;
    }
    else { return fDecodedL1Event.getChip(pHybridId / 2, pHybridId, pReadoutChipId)->getSummary<ChipL1EventInfo>().fL1id; }
}

// does not apply for sparsified event
uint32_t D19cCic2Event::PipelineAddress(uint8_t pHybridId, uint8_t pReadoutChipId)
{
    decodeEvent();
    if(fIsSparsified)
    {
        std::cerr << "D19cCic2Event::PipelineAddress can be called only for 2S read in unsparsified mode, aborting" << std::endl;
        abort();
    }
    else
        return fDecodedL1Event.getChip(pHybridId / 2, pHybridId, pReadoutChipId)->getSummary<ChipL1EventInfo>().fPipelineAddress;
}

bool D19cCic2Event::DataBit(uint8_t pHybridId, uint8_t pReadoutChipId, uint8_t row, uint8_t col)
{
    decodeEvent();
    auto theChipL1Container = fDecodedL1Event.getChip(pHybridId / 2, pHybridId, pReadoutChipId);
    if(fIsSparsified)
    {
        if(fIs2S)
        {
            for(auto theCluster: theChipL1Container->getSummary<ClusterCollection<Cluster2S, 31>>())
            {
                if(theCluster.isChannelHit(col)) return true;
            }
        }
        else
        {
            if(pReadoutChipId < 8)
            {
                for(auto theCluster: theChipL1Container->getSummary<ClusterCollection<StripClusterPS, 32>>())
                {
                    if(theCluster.isChannelHit(col)) return true;
                }
            }
            else
            {
                for(auto theCluster: theChipL1Container->getSummary<ClusterCollection<PixelClusterPS, 32>>())
                {
                    if(theCluster.isChannelHit(row, col)) return true;
                }
            }
        }
        return false;
    }
    else { return theChipL1Container->getSummary<ChipL1EventInfo>().isChannelHit(col); }
}

std::vector<bool> D19cCic2Event::DataBitVector(uint8_t pHybridId, uint8_t pReadoutChipId)
{
    decodeEvent();
    auto theChipL1Container = fDecodedL1Event.getChip(pHybridId / 2, pHybridId, pReadoutChipId);
    if(fIsSparsified)
    {
        if(fIs2S)
        {
            std::vector<bool> bitList(NCHANNELS, false);
            for(auto theCluster: theChipL1Container->getSummary<ClusterCollection<Cluster2S, 31>>())
            {
                for(size_t channel = theCluster.fFirstStrip; channel < theCluster.fFirstStrip + theCluster.fClusterWidth; ++channel) { bitList[channel] = true; }
            }
            return bitList;
        }
        else
        {
            if(pReadoutChipId < 8)
            {
                std::vector<bool> bitList(NSSACHANNELS, false);
                for(auto theCluster: theChipL1Container->getSummary<ClusterCollection<StripClusterPS, 32>>())
                {
                    for(size_t channel = theCluster.fAddress; channel < theCluster.fAddress + theCluster.fWidth; ++channel) { bitList[channel] = true; }
                }
                return bitList;
            }
            else
            {
                std::vector<bool> bitList(NSSACHANNELS * NMPAROWS, false);
                for(auto theCluster: theChipL1Container->getSummary<ClusterCollection<PixelClusterPS, 32>>())
                {
                    for(size_t channel = theCluster.fAddress; channel < theCluster.fAddress + theCluster.fWidth; ++channel) { bitList[channel + NSSACHANNELS * theCluster.fZpos] = true; }
                }
                return bitList;
            }
        }
    }
    else
    {
        std::vector<bool> bitList(NCHANNELS, false);
        for(auto theHitChannel: theChipL1Container->getSummary<ChipL1EventInfo>().getChannelHitList()) { bitList[theHitChannel] = true; }
        return bitList;
    }
}

ClusterCollection<EventStub, 5> D19cCic2Event::StubVector(uint8_t pHybridId, uint8_t pReadoutChipId)
{
    decodeEvent();
    return fDecodedStubEvent.getChip(pHybridId / 2, pHybridId, pReadoutChipId)->getSummary<ClusterCollection<EventStub, 5>>();
}

bool D19cCic2Event::StubBit(uint8_t pHybridId, uint8_t pCbcId)
{
    decodeEvent();
    return (fDecodedStubEvent.getChip(pHybridId / 2, pHybridId, pCbcId)->getSummary<ClusterCollection<EventStub, 5>>().size() > 0);
}

uint32_t D19cCic2Event::GetNHits(uint8_t pHybridId, uint8_t pReadoutChipId)
{
    decodeEvent();
    auto     theChipL1Container = fDecodedL1Event.getChip(pHybridId / 2, pHybridId, pReadoutChipId);
    uint32_t numberOfHits       = 0;
    if(fIsSparsified)
    {
        if(fIs2S)
        {
            for(auto theCluster: theChipL1Container->getSummary<ClusterCollection<Cluster2S, 31>>()) { numberOfHits += theCluster.fClusterWidth; }
        }
        else
        {
            if(pReadoutChipId < 8)
            {
                for(auto theCluster: theChipL1Container->getSummary<ClusterCollection<StripClusterPS, 32>>()) { numberOfHits += theCluster.fWidth; }
            }
            else
            {
                for(auto theCluster: theChipL1Container->getSummary<ClusterCollection<PixelClusterPS, 32>>()) { numberOfHits += theCluster.fWidth; }
            }
        }
    }
    else { numberOfHits = theChipL1Container->getSummary<ChipL1EventInfo>().countNumberOfHits(); }

    return numberOfHits;
}

std::vector<std::pair<uint16_t, uint16_t>> D19cCic2Event::GetHits(uint8_t pHybridId, uint8_t pReadoutChipId)
{
    decodeEvent();
    std::vector<std::pair<uint16_t, uint16_t>> theHitList;
    auto                                       theChipL1Container = fDecodedL1Event.getChip(pHybridId / 2, pHybridId, pReadoutChipId);
    if(fIsSparsified)
    {
        theHitList.reserve(MAXCICCHANNELS);
        if(fIs2S)
        {
            for(auto theCluster: theChipL1Container->getSummary<ClusterCollection<Cluster2S, 31>>())
            {
                for(size_t channel = theCluster.fFirstStrip; channel < theCluster.fFirstStrip + theCluster.fClusterWidth; ++channel) { theHitList.emplace_back(0, channel); }
            }
        }
        else
        {
            if(pReadoutChipId < 8)
            {
                for(auto theCluster: theChipL1Container->getSummary<ClusterCollection<StripClusterPS, 32>>())
                {
                    for(size_t channel = theCluster.fAddress; channel < theCluster.fAddress + theCluster.fWidth; ++channel) { theHitList.emplace_back(0, channel); }
                }
            }
            else
            {
                for(auto theCluster: theChipL1Container->getSummary<ClusterCollection<PixelClusterPS, 32>>())
                {
                    for(size_t channel = theCluster.fAddress; channel < theCluster.fAddress + theCluster.fWidth; ++channel) { theHitList.emplace_back(theCluster.fZpos, channel); }
                }
            }
        }
    }
    else
    {
        theHitList.reserve(NCHANNELS);
        for(auto theHitChannel: theChipL1Container->getSummary<ChipL1EventInfo>().getChannelHitList()) { theHitList.emplace_back(0, theHitChannel); }
    }

    return theHitList;
}

ClusterCollection<Cluster2S, 31> D19cCic2Event::getClusters(uint8_t pHybridId, uint8_t pReadoutChipId)
{
    decodeEvent();
    if(!fIsSparsified || !fIs2S)
    {
        std::cerr << "D19cCic2Event::getClusters can be called only for 2S read in sparsified mode, aborting" << std::endl;
        abort();
    }
    return fDecodedL1Event.getChip(pHybridId / 2, pHybridId, pReadoutChipId)->getSummary<ClusterCollection<Cluster2S, 31>>();
}

void D19cCic2Event::print()
{
    decodeEvent();
    std::cout << "D19cCic2Event event printout" << std::endl;

    std::cout << "ExternalTriggerID = " << fExternalTriggerID << std::endl;
    std::cout << "TDC               = " << fTDC << std::endl;
    std::cout << "EventCount        = " << fEventCount << std::endl;
    std::cout << "BxId              = " << fBunch << std::endl;

    for(auto theOpticalGroup: fDecodedL1Event)
    {
        std::cout << "OpticalGroup id = " << theOpticalGroup->getId() << std::endl;
        for(auto theHybrid: *theOpticalGroup)
        {
            std::cout << "Hybrid id = " << theHybrid->getId() << std::endl;

            const HybridL1EventInfo* theHybridL1EventInfoPointer;
            if(fIsSparsified)
            {
                if(fIs2S)
                    theHybridL1EventInfoPointer = &theHybrid->getSummary<HybridL1EventInfo, ClusterCollection<Cluster2S, 31>>();
                else
                    theHybridL1EventInfoPointer = &theHybrid->getSummary<HybridL1EventInfo, ClusterCollection<StripClusterPS, 32>>();
            }
            else { theHybridL1EventInfoPointer = &theHybrid->getSummary<HybridL1EventInfo, ChipL1EventInfo>(); }
            std::cout << "L1 header" << std::endl;
            theHybridL1EventInfoPointer->print();
            for(auto theChip: *theHybrid)
            {
                std::cout << "Chip id = " << theChip->getId() << std::endl;
                if(fIsSparsified)
                {
                    if(fIs2S)
                    {
                        for(auto theCluster: theChip->getSummary<ClusterCollection<Cluster2S, 31>>()) { theCluster.print(); }
                    }
                    else
                    {
                        if(theChip->getId() < 8)
                        {
                            for(auto theCluster: theChip->getSummary<ClusterCollection<StripClusterPS, 32>>()) { theCluster.print(); }
                        }
                        else
                        {
                            for(auto theCluster: theChip->getSummary<ClusterCollection<PixelClusterPS, 32>>()) { theCluster.print(); }
                        }
                    }
                }
                else { theChip->getSummary<ChipL1EventInfo>().print(); }
            }

            auto theStubHybrid = fDecodedStubEvent.getHybrid(theOpticalGroup->getId(), theHybrid->getId());
            std::cout << "Stub header" << std::endl;
            theStubHybrid->getSummary<HybridStubEventInfo, ClusterCollection<EventStub, 5>>().print();
            for(auto theChip: *theStubHybrid)
            {
                std::cout << "Chip id = " << theChip->getId() << std::endl;
                for(auto theStub: theChip->getSummary<ClusterCollection<EventStub, 5>>()) { theStub.print(); }
            }
        }
    }
}

} // namespace Ph2_HwInterface
