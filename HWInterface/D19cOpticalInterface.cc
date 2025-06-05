#include "HWInterface/D19cOpticalInterface.h"
#include "HWDescription/Chip.h"
#include "HWInterface/D19clpGBTSlowControlWorkerInterface.h"
#include "HWInterface/RegManager.h"

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
// ##########################################
// # Constructors #
// #########################################

D19cOpticalInterface::D19cOpticalInterface(RegManager* theRegMaster) : FEConfigurationInterface(theRegMaster) { fType = ConfigurationType::IC; }

D19cOpticalInterface::~D19cOpticalInterface() {}
// ##########################################
// # Chip Register read/write #
// #########################################
//
bool D19cOpticalInterface::Read(Chip* pChip, std::vector<ChipRegItem>& pRegisterItems)
{
    std::lock_guard<std::recursive_mutex> theGuard(fTheRegManager->fMutex);
    flpGBTSlowControlWorkerInterface->SelectLink(pChip->getOpticalGroupId());
    uint8_t                  cFunctionId = (pChip->getFrontEndType() == FrontEndType::LpGBT) ? LpGBTSlowControlWorker::READ_IC : LpGBTSlowControlWorker::READ_FE;
    std::vector<ChipRegItem> cRegisterBlock;
    bool                     cSuccess = true;
    int                      cBlockId = 0;
    for(std::vector<ChipRegItem>::iterator cRegItemIter = pRegisterItems.begin(); cRegItemIter < pRegisterItems.end(); cRegItemIter++)
    {
        cRegisterBlock.push_back(*cRegItemIter);
        if(cRegisterBlock.size() == LpGBTSlowControlWorker::BLOCK_SIZE || cRegItemIter == pRegisterItems.end() - 1)
        {
            // Encode command frame
            auto cCommand = flpGBTSlowControlWorkerInterface->EncodeCommand(cFunctionId, pChip, cRegisterBlock);
            // Send commands to worker
            flpGBTSlowControlWorkerInterface->WriteCommand(cCommand);
            // Wait for worker to be done
            if(!flpGBTSlowControlWorkerInterface->WaitDone(cFunctionId))
            {
                LOG(ERROR) << BOLDRED << "D19cOpticalInterface::Read : Tool stuck ... Sending soft reset" << RESET;
                return false;
            }
            // Check if multilple tries happened
            uint8_t cTryCntr = flpGBTSlowControlWorkerInterface->GetTryCounter(cFunctionId);
            if(cTryCntr > 0)
            {
                uint8_t cMaxRetry = (pChip->getFrontEndType() == FrontEndType::LpGBT) ? fConfiguration.fMaxRetryIC : fConfiguration.fMaxRetryFE;
                LOG(ERROR) << BOLDRED << "D19cOpticalInterface::Read : Tried " << +cTryCntr << "/" << +cMaxRetry << " before success" << RESET;
            }
            // Get replies from worker
            auto cReplies = flpGBTSlowControlWorkerInterface->ReadReply(cRegisterBlock.size() + 1); // N words + 1 header
            // Verifiy reply frame integrity - only n_words in the header
            size_t cNWords = (cReplies.at(0) & (0xFFFF << 0)) >> 0;
            if(cNWords != cRegisterBlock.size())
            {
                LOG(ERROR) << BOLDRED << "D19cOpticalInterface::Read -- Corrupted CPB reply: cNWords = " << cNWords << " - cDataBlock.size() = " << cRegisterBlock.size() << RESET;
                throw std::runtime_error("D19cOpticalInterface::Read -- Corrupted CPB reply");
            }
            // Decode reply frame and extract data
            for(size_t cReplyIdx = 0; cReplyIdx < cReplies.size(); cReplyIdx++)
            {
                if(cReplyIdx == 0) continue; // skip header
                uint8_t cErrorCode = (cReplies.at(cReplyIdx) & (0xFF << 8)) >> 8;
                uint8_t cReadBack  = (cReplies.at(cReplyIdx) & (0xFF << 0)) >> 0;
                if(cErrorCode != 0)
                {
                    if(pChip->getFrontEndType() == FrontEndType::LpGBT)
                        LOG(ERROR) << BOLDRED << "D19cOpticalInterface::Read -- Error Code : " << +cErrorCode << RESET;
                    else
                        LOG(ERROR) << BOLDRED << "D19cOpticalInterface::Read -- Error Code : " << +cErrorCode << " -- I2C Status : " << LpGBTSlowControlWorker::I2C_STATUS_MAP.at(cReadBack) << RESET;
                    LOG(ERROR) << BOLDRED << "Chip code : " << +pChip->getChipCode() << " -- Chip Id : " << +pChip->getId() << " -- Register address 0x" << std::hex
                               << +pRegisterItems.at(cBlockId * LpGBTSlowControlWorker::BLOCK_SIZE + cReplyIdx - 1).fAddress << std::dec << RESET;
                    cSuccess &= false;
                }
                pRegisterItems.at(cBlockId * LpGBTSlowControlWorker::BLOCK_SIZE + cReplyIdx - 1).fValue = cReadBack;
            }
            cRegisterBlock.clear();
            cBlockId++;
        }
    }
    return cSuccess;
}

bool D19cOpticalInterface::Write(Chip* pChip, std::vector<ChipRegItem>& pRegisterItems, bool pVerify)
{
    std::lock_guard<std::recursive_mutex> theGuard(fTheRegManager->fMutex);
    flpGBTSlowControlWorkerInterface->SelectLink(pChip->getOpticalGroupId());
    uint8_t                  cFunctionId = (pChip->getFrontEndType() == FrontEndType::LpGBT) ? LpGBTSlowControlWorker::WRITE_IC : LpGBTSlowControlWorker::WRITE_FE;
    std::vector<ChipRegItem> cRegisterBlock;
    bool                     cSuccess = true;
    int                      cBlockId = 0;
    for(std::vector<ChipRegItem>::iterator cRegItemIter = pRegisterItems.begin(); cRegItemIter < pRegisterItems.end(); cRegItemIter++)
    {
        cRegisterBlock.push_back(*cRegItemIter);
        if(cRegisterBlock.size() == LpGBTSlowControlWorker::BLOCK_SIZE || cRegItemIter == pRegisterItems.end() - 1)
        {
            // Encode command frame
            auto cCommand = flpGBTSlowControlWorkerInterface->EncodeCommand(cFunctionId, pChip, cRegisterBlock, pVerify);
            // Send commands to worker
            flpGBTSlowControlWorkerInterface->WriteCommand(cCommand);
            // Wait for worker to be done
            if(!flpGBTSlowControlWorkerInterface->WaitDone(cFunctionId))
            {
                LOG(ERROR) << BOLDRED << "D19cOpticalInterface::Write : Tool stuck ... Sending soft reset" << RESET;
                return false;
            }
            // Check if multilple tries happened
            uint8_t cTryCntr = flpGBTSlowControlWorkerInterface->GetTryCounter(cFunctionId);
            if(cTryCntr > 0)
            {
                uint8_t cMaxRetry = (pChip->getFrontEndType() == FrontEndType::LpGBT) ? fConfiguration.fMaxRetryIC : fConfiguration.fMaxRetryFE;
                LOG(ERROR) << BOLDRED << "D19cOpticalInterface::Write : Tried " << +cTryCntr << "/" << +cMaxRetry << " before success" << RESET;
            }
            // Get replies from worker
            auto cReplies = flpGBTSlowControlWorkerInterface->ReadReply(cRegisterBlock.size() + 1); // N words + 1 header
            // Verifiy reply frame integrity - only n_words in the header
            size_t cNWords = (cReplies.at(0) & (0xFFFF << 0)) >> 0;
            if(cNWords != cRegisterBlock.size())
            {
                LOG(ERROR) << BOLDRED << "D19cOpticalInterface::Write -- Corrupted CPB reply: cNWords = " << cNWords << " - cDataBlock.size() = " << cRegisterBlock.size() << RESET;
                throw std::runtime_error("D19cOpticalInterface::Write -- Corrupted CPB reply");
            }
            // Decode reply frame and extract data
            for(size_t cReplyIdx = 0; cReplyIdx < cReplies.size(); cReplyIdx++)
            {
                if(cReplyIdx == 0) continue; // skip header
                uint8_t cErrorCode = (cReplies.at(cReplyIdx) & (0xFF << 8)) >> 8;
                uint8_t cReadBack  = (cReplies.at(cReplyIdx) & (0xFF << 0)) >> 0;
                if(cErrorCode != 0)
                {
                    if(pChip->getFrontEndType() == FrontEndType::LpGBT)
                        LOG(ERROR) << BOLDRED << "D19cOpticalInterface::Write -- Error Code : " << +cErrorCode << " for LpGBT on Board id " << +pChip->getBeBoardId() << " OpticalGroup id "
                                   << +pChip->getOpticalGroupId() << RESET;
                    else
                        LOG(ERROR) << BOLDRED << "D19cOpticalInterface::Write -- Error Code : " << +cErrorCode << " -- I2C Status : " << LpGBTSlowControlWorker::I2C_STATUS_MAP.at(cReadBack) << RESET;
                    LOG(ERROR) << BOLDRED << "Chip code : " << +pChip->getChipCode() << " on Board id " << +pChip->getBeBoardId() << " OpticalGroup id " << +pChip->getOpticalGroupId() << " Hybrid id "
                               << +pChip->getHybridId() << " Chip Id : " << +pChip->getId() << " -- Register address 0x" << std::hex
                               << +pRegisterItems.at(cBlockId * LpGBTSlowControlWorker::BLOCK_SIZE + cReplyIdx - 1).fAddress << std::dec << RESET;
                    cSuccess &= false;
                }
                if(pVerify)
                {
                    auto& writtenValue = pRegisterItems.at(cBlockId * LpGBTSlowControlWorker::BLOCK_SIZE + cReplyIdx - 1);
                    if(cReadBack != writtenValue.fValue)
                    {
                        LOG(ERROR) << BOLDRED << "D19cOpticalInterface::Write : Wrong value read back, written 0x" << std::hex << writtenValue.fValue << " but read back 0x" << +cReadBack << std::dec
                                   << RESET;
                        cSuccess &= false;
                    }
                    writtenValue.fValue = cReadBack;
                }
            }
            cRegisterBlock.clear();
            cBlockId++;
        }
    }
    return cSuccess;
}

bool D19cOpticalInterface::SingleWrite(Chip* pChip, ChipRegItem& pRegisterItem)
{
    std::vector<ChipRegItem> pRegisterItemTemp = {pRegisterItem};
    bool                     cSuccess          = Write(pChip, pRegisterItemTemp, false);
    pRegisterItem.fValue                       = pRegisterItemTemp.at(0).fValue;
    return cSuccess;
}

bool D19cOpticalInterface::SingleRead(Chip* pChip, ChipRegItem& pRegisterItem)
{
    std::vector<ChipRegItem> pRegisterItemTemp = {pRegisterItem};
    bool                     cSuccess          = Read(pChip, pRegisterItemTemp);
    pRegisterItem.fValue                       = pRegisterItemTemp.at(0).fValue;
    return cSuccess;
}

bool D19cOpticalInterface::MultiRead(Chip* pChip, std::vector<ChipRegItem>& pRegisterItems) { return Read(pChip, pRegisterItems); }

bool D19cOpticalInterface::MultiWrite(Chip* pChip, std::vector<ChipRegItem>& pRegisterItems) { return Write(pChip, pRegisterItems, false); }

bool D19cOpticalInterface::SingleWriteRead(Chip* pChip, ChipRegItem& pRegisterItem)
{
    std::vector<ChipRegItem> pRegisterItemTemp = {pRegisterItem};
    bool                     cSuccess          = Write(pChip, pRegisterItemTemp, true);
    pRegisterItem.fValue                       = pRegisterItemTemp.at(0).fValue;
    return cSuccess;
}

bool D19cOpticalInterface::MultiWriteRead(Chip* pChip, std::vector<ChipRegItem>& pRegisterItems) { return Write(pChip, pRegisterItems, true); }

bool D19cOpticalInterface::WriteI2C(Ph2_HwDescription::Chip* pChip, uint8_t pMasterId, uint8_t pMasterConfig, std::vector<uint32_t>& pSlaveData, bool pWaitToBeDone = true)
{
    std::lock_guard<std::recursive_mutex> theGuard(fTheRegManager->fMutex);
    flpGBTSlowControlWorkerInterface->SelectLink(pChip->getOpticalGroupId());
    uint8_t               cFunctionId = LpGBTSlowControlWorker::MULTI_BYTE_WRITE_I2C;
    std::vector<uint32_t> cDataBlock;
    bool                  cSuccess = true;
    int                   cBlockId = 0;
    for(std::vector<uint32_t>::iterator cDataIter = pSlaveData.begin(); cDataIter < pSlaveData.end(); cDataIter++)
    {
        cDataBlock.push_back(*cDataIter);
        if(cDataBlock.size() == LpGBTSlowControlWorker::BLOCK_SIZE || cDataIter == pSlaveData.end() - 1)
        {
            // Encode command frame
            auto cCommand = flpGBTSlowControlWorkerInterface->EncodeCommandI2C(cFunctionId, pChip, pMasterId, pMasterConfig, cDataBlock);
            // Send command to worker
            flpGBTSlowControlWorkerInterface->WriteCommand(cCommand);
            // Wait for worker to be done
            if(pWaitToBeDone) // Skip this block if no response is expected due to a switched off light
            {
                if(!flpGBTSlowControlWorkerInterface->WaitDone(cFunctionId))
                {
                    LOG(ERROR) << BOLDRED << "D19cOpticalInterface::WriteI2C : Tool stuck ... Sending soft reset" << RESET;
                    return false;
                }
                // Get replies from worker
                auto cReplies = flpGBTSlowControlWorkerInterface->ReadReply(cDataBlock.size() + 1); // N words + 1 header
                // Verifiy reply frame integrity - only n_words in the header
                size_t cNWords = (cReplies.at(0) & (0xFFFF << 0)) >> 0;
                if(cNWords != cDataBlock.size())
                {
                    LOG(ERROR) << BOLDRED << "D19cOpticalInterface::WriteI2C -- Corrupted CPB reply: cNWords = " << cNWords << " - cDataBlock.size() = " << cDataBlock.size() << RESET;
                    throw std::runtime_error("D19cOpticalInterface::WriteI2C -- Corrupted CPB reply");
                }
                // Decode reply frame and extract data
                for(size_t cReplyIdx = 0; cReplyIdx < cReplies.size(); cReplyIdx++)
                {
                    if(cReplyIdx == 0) continue; // skip header
                    uint8_t cErrorCode = (cReplies.at(cReplyIdx) & (0xFF << 8)) >> 8;
                    uint8_t cReadBack  = (cReplies.at(cReplyIdx) & (0xFF << 0)) >> 0;
                    if(cErrorCode != 0)
                    {
                        LOG(ERROR) << BOLDRED << "D19cOpticalInterface::WriteI2C -- Error Code : " << +cErrorCode << " -- I2C Status : " << LpGBTSlowControlWorker::I2C_STATUS_MAP.at(cReadBack)
                                   << RESET;
                        cSuccess &= false;
                    }
                }
                cDataBlock.clear();
                cBlockId++;
            }
        }
    }
    return cSuccess;
}

std::vector<uint16_t> D19cOpticalInterface::ReadI2C(Ph2_HwDescription::Chip* pChip, uint8_t pMasterId, uint8_t pMasterConfig, std::vector<uint32_t>& pSlaveData)
{
    std::lock_guard<std::recursive_mutex> theGuard(fTheRegManager->fMutex);
    flpGBTSlowControlWorkerInterface->SelectLink(pChip->getOpticalGroupId());
    uint8_t               cFunctionId = LpGBTSlowControlWorker::SINGLE_BYTE_READ_I2C;
    std::vector<uint32_t> cDataBlock;
    std::vector<uint16_t> cReadBackData;
    bool                  cSuccess = true;
    int                   cBlockId = 0;
    for(std::vector<uint32_t>::iterator cDataIter = pSlaveData.begin(); cDataIter < pSlaveData.end(); cDataIter++)
    {
        cDataBlock.push_back(*cDataIter);
        if(cDataBlock.size() == LpGBTSlowControlWorker::BLOCK_SIZE || cDataIter == pSlaveData.end() - 1)
        {
            // Encode command frame
            auto cCommand = flpGBTSlowControlWorkerInterface->EncodeCommandI2C(cFunctionId, pChip, pMasterId, pMasterConfig, cDataBlock);
            // Send command to worker
            flpGBTSlowControlWorkerInterface->WriteCommand(cCommand);
            // Wait for worker to be done
            if(!flpGBTSlowControlWorkerInterface->WaitDone(cFunctionId))
            {
                LOG(ERROR) << BOLDRED << "D19cOpticalInterface::ReadI2C : Tool stuck ... Sending soft reset" << RESET;
                return {};
            }
            // Get replies from worker
            auto cReplies = flpGBTSlowControlWorkerInterface->ReadReply(cDataBlock.size() + 1); // N words + 1 header
            // Verifiy reply frame integrity - only n_words in the header
            size_t cNWords = (cReplies.at(0) & (0xFFFF << 0)) >> 0;
            if(cNWords != cDataBlock.size())
            {
                LOG(ERROR) << BOLDRED << "D19cOpticalInterface::ReadI2C -- Corrupted CPB reply: cNWords = " << cNWords << " - cDataBlock.size() = " << cDataBlock.size() << RESET;
                throw std::runtime_error("D19cOpticalInterface::ReadI2C -- Corrupted CPB reply");
            }
            // Decode reply frame and extract data
            for(size_t cReplyIdx = 0; cReplyIdx < cReplies.size(); cReplyIdx++)
            {
                if(cReplyIdx == 0) continue; // skip header
                uint8_t cErrorCode = (cReplies.at(cReplyIdx) & (0xFF << 8)) >> 8;
                uint8_t cReadBack  = (cReplies.at(cReplyIdx) & (0xFF << 0)) >> 0;
                if(cErrorCode != 0)
                {
                    LOG(ERROR) << BOLDRED << "D19cOpticalInterface::ReadI2C -- Error Code : " << +cErrorCode << " -- I2C Status : " << LpGBTSlowControlWorker::I2C_STATUS_MAP.at(cReadBack) << RESET;
                    cSuccess &= false;
                }
                cReadBackData.push_back(cErrorCode | cReadBack << 0);
            }
            cDataBlock.clear();
            cBlockId++;
        }
    }
    return cReadBackData;
}

bool D19cOpticalInterface::SingleMultiByteWriteI2C(Ph2_HwDescription::Chip* pChip, uint8_t pMasterId, uint8_t pMasterConfig, uint8_t pSlaveAddress, uint32_t pSlaveData, bool pWaitToBeDone)
{
    std::vector<uint32_t> cMasterData;
    cMasterData.push_back(pSlaveData << 8 | pSlaveAddress << 0);
    return WriteI2C(pChip, pMasterId, pMasterConfig, cMasterData, pWaitToBeDone);
}

bool D19cOpticalInterface::MultiMultiByteWriteI2C(Ph2_HwDescription::Chip* pChip, uint8_t pMasterId, uint8_t pMasterConfig, std::vector<uint32_t>& pSlaveData)
{
    return WriteI2C(pChip, pMasterId, pMasterConfig, pSlaveData);
}

uint8_t D19cOpticalInterface::SingleSingleByteReadI2C(Ph2_HwDescription::Chip* pChip, uint8_t pMasterId, uint8_t pMasterConfig, uint8_t pSlaveAddress)
{
    std::vector<uint32_t> cMasterData;
    cMasterData.push_back(pSlaveAddress << 0);
    auto cReadBackData = ReadI2C(pChip, pMasterId, pMasterConfig, cMasterData);
    if(cReadBackData.size() == 0)
    {
        LOG(INFO) << BOLDGREEN << "D19cOpticalInterface::SingleSingleByteReadI2C No data in read back, returning 0" << RESET;
        return 0;
    }
    auto cReadBackValue = (cReadBackData.at(0) & 0xFF);

    return cReadBackValue;
}

std::vector<uint16_t> D19cOpticalInterface::MultiSingleByteReadI2C(Ph2_HwDescription::Chip* pChip, uint8_t pMasterId, uint8_t pMasterConfig, std::vector<uint32_t>& pSlaveData)
{
    return ReadI2C(pChip, pMasterId, pMasterConfig, pSlaveData);
}

bool D19cOpticalInterface::testLinkStability(Ph2_HwDescription::Chip* pLpGBT)
{
    if(pLpGBT->getFrontEndType() != FrontEndType::LpGBT)
    {
        LOG(ERROR) << ERROR_FORMAT << "D19cOpticalInterface::testLinkStability can be used only for LpGBT, " << FrontEndDescription::getFrontEndName(pLpGBT->getFrontEndType())
                   << " provided instead, aborting" << RESET;
        abort();
    }
    std::lock_guard<std::recursive_mutex> theGuard(fTheRegManager->fMutex);
    flpGBTSlowControlWorkerInterface->SelectLink(pLpGBT->getOpticalGroupId());
    uint8_t     cFunctionId = LpGBTSlowControlWorker::READ_IC;
    ChipRegItem theChipId0register;
    theChipId0register.fAddress = 0x000;
    theChipId0register.fPage    = 0x000;
    std::vector<ChipRegItem> theRegisterList(LpGBTSlowControlWorker::BLOCK_SIZE, theChipId0register);

    auto cCommand = flpGBTSlowControlWorkerInterface->EncodeCommand(cFunctionId, pLpGBT, theRegisterList);
    flpGBTSlowControlWorkerInterface->WriteCommand(cCommand);
    // Wait for worker to be done
    if(!flpGBTSlowControlWorkerInterface->WaitDone(cFunctionId))
    {
        LOG(ERROR) << BOLDRED << "D19cOpticalInterface::Read : Tool stuck ... Sending soft reset" << RESET;
        return false;
    }

    uint8_t cTryCntr = flpGBTSlowControlWorkerInterface->GetTryCounter(cFunctionId);
    if(cTryCntr > 0)
    {
        return false;
        LOG(WARNING) << WARNING_FORMAT << "D19cOpticalInterface::testLinkStability : try counter  " << +cTryCntr << " >  0" << RESET;
    }
    return true;
}

} // namespace Ph2_HwInterface
