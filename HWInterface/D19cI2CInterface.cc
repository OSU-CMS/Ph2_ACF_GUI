#include "D19cI2CInterface.h"

using namespace Ph2_HwDescription;

#include "HWDescription/BeBoard.h"
#include "HWDescription/OpticalGroup.h"
#include "HWDescription/OuterTrackerHybrid.h"
#include "HWInterface/RegManager.h"
#include "Utils/Utilities.h"
#include <thread>
namespace Ph2_HwInterface
{
D19cI2CInterface::D19cI2CInterface(RegManager* theRegManager) : FEConfigurationInterface(theRegManager)
{
    fI2CVersion = fTheRegManager->ReadReg("fc7_daq_stat.command_processor_block.i2c.master_version");
    fI2CSlaveMap.clear();
    fType = ConfigurationType::I2C;
}

D19cI2CInterface::~D19cI2CInterface() {}

void D19cI2CInterface::PrintStatus()
{
    // temporary used for board status printing
    LOG(INFO) << YELLOW << "============================" << RESET;
    LOG(INFO) << BOLDBLUE << "Current Status" << RESET;

    ReadErrors();

    int i2c_replies_empty = fTheRegManager->ReadReg("fc7_daq_stat.command_processor_block.i2c.reply_fifo.empty");
    if(i2c_replies_empty == 0)
        LOG(INFO) << "I2C Replies Available: " << BOLDGREEN << "Yes" << RESET;
    else
        LOG(INFO) << "I2C Replies Available: " << BOLDGREEN << "No" << RESET;

    LOG(INFO) << YELLOW << "============================" << RESET;
}
void D19cI2CInterface::ConfigureI2CMap(const BeBoard* pBoard)
{
    if(fI2CVersion >= 1)
    {
        LOG(INFO) << BOLDBLUE << "Configuring I2C Map for I2C version" << +fI2CVersion << " of uDTC FW" << RESET;
        // assuming only one type of CIC per board ...
        bool cWithCBC3 = false;
        for(auto cModule: *pBoard)
        {
            if(cWithCBC3) break;
            for(auto cHybrid: *cModule)
            {
                if(cWithCBC3) break;
                for(auto cChip: *cHybrid)
                {
                    if(cWithCBC3) break;
                    cWithCBC3 = (cChip->getFrontEndType() == FrontEndType::CBC3);
                }
            }
        }

        for(auto cModule: *pBoard)
        {
            // default I2C map is for 8CBC3
            for(auto cHybrid: *cModule)
            {
                auto    cOuterTrackerHybrid = static_cast<OuterTrackerHybrid*>(cHybrid);
                auto&   cCic                = cOuterTrackerHybrid->fCic;
                uint8_t cNBytes;
                if(cCic != NULL)
                {
                    for(auto cChip: *cHybrid)
                    {
                        cNBytes            = (cChip->getFrontEndType() == FrontEndType::SSA2 || cChip->getFrontEndType() == FrontEndType::MPA2) ? 2 : 1;
                        uint8_t cLastValue = 1;
                        if(fI2CSlaveMap.find(cChip->getId()) == fI2CSlaveMap.end())
                        {
                            std::vector<uint32_t> cOldI2CSlaveDescription = {cChip->getChipAddress(), cNBytes, 1, 1, 1, cLastValue, (uint32_t)(cChip->getId() % 8)};
                            std::vector<uint32_t> cI2CSlaveDescription    = {cChip->getChipAddress(), cNBytes, 1, 1, 1, cLastValue};

                            LOG(INFO) << BOLDBLUE << "Adding chip with address 0x" << std::hex << +cChip->getChipAddress() << std::dec << " to I2C slave map.." << RESET;
                            fI2CSlaveMap[cChip->getId()] = cI2CSlaveDescription;
                        }
                    } // chips
                    // cBaseAddress                                  = 0x60;
                    cNBytes                                       = 2;
                    std::vector<uint32_t> cOldI2CSlaveDescription = {cCic->getChipAddress(), cNBytes, 1, 1, 1, 1, cCic->getId()};
                    std::vector<uint32_t> cI2CSlaveDescription    = {cCic->getChipAddress(), cNBytes, 1, 1, 1, 1};
                    fI2CSlaveMap[cCic->getId()]                   = cI2CSlaveDescription;
                    LOG(INFO) << BOLDBLUE << "Adding chip with address 0x" << std::hex << +cCic->getChipAddress() << std::dec << " to I2C slave map.." << RESET;
                }
                else
                {
                    for(auto cChip: *cHybrid)
                    {
                        cNBytes            = (cChip->getFrontEndType() == FrontEndType::SSA2 || cChip->getFrontEndType() == FrontEndType::MPA2) ? 2 : 1;
                        uint8_t cLastValue = 1;
                        LOG(INFO) << BOLDBLUE << "Adding slave with I2C address 0x" << std::hex << +cChip->getChipAddress() << std::dec << RESET;

                        std::vector<uint32_t> cOldI2CSlaveDescription = {cChip->getChipAddress(), cNBytes, 1, 1, 1, cLastValue, (uint32_t)(cChip->getId() % 8)};
                        std::vector<uint32_t> cI2CSlaveDescription    = {cChip->getChipAddress(), cNBytes, 1, 1, 1, cLastValue};
                        fI2CSlaveMap[cChip->getId()]                  = cI2CSlaveDescription;
                    } // chips
                }
            } // hybrids
        } // modules
        // and then loop over map and write
        for(auto cIterator = fI2CSlaveMap.begin(); cIterator != fI2CSlaveMap.end(); cIterator++)
        {
            // auto cChipId = cIterator->first;
            auto cDescription = cIterator->second;
            // setting the params
            uint32_t shifted_i2c_address             = cDescription.at(0) << 25;
            uint32_t shifted_register_address_nbytes = cDescription.at(1) << 10;
            uint32_t shifted_data_wr_nbytes          = cDescription.at(2) << 5;
            uint32_t shifted_data_rd_nbytes          = cDescription.at(3) << 0;
            uint32_t shifted_stop_for_rd_en          = cDescription.at(4) << 24;
            uint32_t shifted_nack_en                 = cDescription.at(5) << 23;

            // writing the item to the firmware
            if(!cWithCBC3)
            {
                uint32_t    final_item = shifted_i2c_address + shifted_register_address_nbytes + shifted_data_wr_nbytes + shifted_data_rd_nbytes + shifted_stop_for_rd_en + shifted_nack_en;
                std::string curreg     = "fc7_daq_cnfg.command_processor_block.i2c_address_table.slave_" + std::to_string(std::distance(fI2CSlaveMap.begin(), cIterator)) + "_config";
                LOG(INFO) << BOLDMAGENTA << "Writing " << std::bitset<32>(final_item) << " to register " << curreg << RESET;
                fTheRegManager->WriteReg(curreg, final_item);
            }
        }
    }
}
void D19cI2CInterface::EncodeReg(const ChipRegItem& pRegItem, Chip* pChip, std::vector<uint32_t>& pVecReq, bool pReadBack, bool pWrite)
{
    uint8_t pCbcId       = pChip->getId();
    uint8_t pHybridId    = pChip->getHybridId();
    auto    cMapIterator = fI2CSlaveMap.find(pCbcId);
    bool    cFound       = (cMapIterator != fI2CSlaveMap.end());
    if(cFound)
    {
        // remember .. encoded command the chip id is .. the index and not the id!!
        uint8_t pIndex = std::distance(fI2CSlaveMap.begin(), cMapIterator);
        // LOG(INFO) << BOLDGREEN << "Encoding register from chip " << +pCbcId << " which is index " << +pIndex << " in I2C map " << RESET;
        // use fBroadcastCBCId for broadcast commands
        bool pUseMask = false;
        if(fI2CVersion >= 1)
        {
            // new command consists of one word if its read command, and of two words if its write. first word is always
            // the same
            pVecReq.push_back((0 << 28) | (0 << 27) | (pHybridId << 23) | (pIndex << 18) | (pReadBack << 17) | ((!pWrite) << 16) | (pRegItem.fPage << 8) | (pRegItem.fAddress << 0));
            // only for write commands
            if(pWrite) pVecReq.push_back((0 << 28) | (pWrite << 27) | (pRegItem.fValue << 0));
        }
        else
        {
            pVecReq.push_back((0 << 28) | (pHybridId << 24) | (pCbcId << 20) | (pReadBack << 19) | (pUseMask << 18) | ((pRegItem.fPage) << 17) | ((!pWrite) << 16) | (pRegItem.fAddress << 8) |
                              pRegItem.fValue);
        }
    }
    else { LOG(INFO) << BOLDRED << "Could not find address in I2C map.. " << RESET; }
}
void D19cI2CInterface::DecodeReg(ChipRegItem& pRegItem, uint8_t& pCbcId, uint32_t pWord, bool& pRead, bool& pFailed)
{
    if(fI2CVersion >= 1)
    {
        // pHybridId    =  ( ( pWord & 0x07800000 ) >> 27) ;
        pCbcId            = ((pWord & 0x007c0000) >> 22);
        pFailed           = 0;
        pRegItem.fPage    = pRegItem.fPage;
        pRead             = true;
        pRegItem.fAddress = (pRegItem.fAddress <= 0xFF) ? (pWord & 0x0000FF00) >> 8 : pRegItem.fAddress; // check in the FW what it does with 16 bit addresses here
        pRegItem.fValue   = (pWord & 0x000000FF);
        // LOG (INFO) << BOLDYELLOW << "Reg 0x" << std::hex << +pRegItem.fAddress  << " set to 0x" << +pRegItem.fValue << std::dec << RESET;
    }
    else
    {
        // pHybridId    =  ( ( pWord & 0x00f00000 ) >> 24) ;
        pCbcId            = ((pWord & 0x00f00000) >> 20);
        pFailed           = 0;
        pRegItem.fPage    = ((pWord & 0x00020000) >> 17);
        pRead             = (pWord & 0x00010000) >> 16;
        pRegItem.fAddress = (pWord & 0x0000FF00) >> 8;
        pRegItem.fValue   = (pWord & 0x000000FF);
    }
}

// Write + Read functions
bool D19cI2CInterface::SingleWriteRead(Chip* pChip, ChipRegItem& pItem)
{
    std::vector<ChipRegItem> cItems;
    cItems.push_back(pItem);
    return MultiWriteRead(pChip, cItems);
}
// for now this is just a write to all followed by a read from all
bool D19cI2CInterface::MultiWriteRead(Chip* pChip, std::vector<ChipRegItem>& pWriteRegs)
{
    size_t cAttempts = 0;

    // prepare vector to hold read-back values
    std::vector<ChipRegItem> cReadbackRegs;
    for(auto cItem: pWriteRegs) { cReadbackRegs.push_back(cItem); }

    // perform write + check read-back
    // until it works or you've tried
    // too many times
    bool cSuccess = false;
    do {
        if(MultiWrite(pChip, pWriteRegs))
        {
            std::this_thread::sleep_for(std::chrono::microseconds(100000)); // need this pause for SSA I2C to work .. why?
            if(MultiRead(pChip, cReadbackRegs))
            {
                // check read against write
                for(auto cReadBackReg: cReadbackRegs)
                {
                    auto cIterator =
                        find_if(pWriteRegs.begin(), pWriteRegs.end(), [&cReadBackReg](const ChipRegItem& obj) { return obj.fAddress == cReadBackReg.fAddress && obj.fPage == cReadBackReg.fPage; });
                    if(cIterator != pWriteRegs.end())
                    {
                        if(cReadBackReg.fValue != cIterator->fValue)
                            LOG(INFO) << BOLDRED << "D19cI2CInterface::MultiWriteRead"
                                      << " mismatch in readback register " << std::hex << +cIterator->fAddress << " NO MATCH!"
                                      << " expected " << cIterator->fValue << " read back " << cReadBackReg.fValue << std::dec << RESET;
                        // else
                        //     LOG(DEBUG) << BOLDGREEN << "D19cI2CInterface::MultiWriteRead"
                        //                << " match in readback register " << std::hex << +cIterator->fAddress << " MATCH!" << std::dec << RESET;

                        cSuccess = (cReadBackReg.fValue == cIterator->fValue);
                    }
                }
            }
        }
    } while(cAttempts < fConfiguration.fMaxAttempts && !cSuccess && fConfiguration.fRetry);
    return cSuccess;
}

bool D19cI2CInterface::MultiWrite(Chip* pChip, std::vector<ChipRegItem>& pRegisterItems)
{
    if(pRegisterItems.size() == 0) return true;
    // Deal with the ChipRegItems and encode them
    std::vector<uint32_t> cVec;
    for(const auto& cItem: pRegisterItems)
    {
        // update list of modified registers
        EncodeReg(cItem, pChip, cVec, fConfiguration.fVerify, true);
    }

    uint8_t cWriteAttempts = 0;
    // if the transaction is successfull, update the HWDescription object
    return WriteChipBlockReg(cVec, cWriteAttempts, fConfiguration.fVerify);
}
bool D19cI2CInterface::SingleWrite(Chip* pChip, Ph2_HwDescription::ChipRegItem& pItem)
{
    std::vector<ChipRegItem> cItems;
    cItems.push_back(pItem);
    return MultiWrite(pChip, cItems);
}
bool D19cI2CInterface::MultiRead(Chip* pChip, std::vector<ChipRegItem>& pRegisterItems)
{
    std::vector<uint32_t> cVecReq;
    // make sure you don't read ctrl registers
    for(auto cRegItem: pRegisterItems)
    {
        if(cRegItem.fControlReg == 0x1)
        {
            LOG(DEBUG) << BOLDRED << "Control register MultiRead" << std::hex << +cRegItem.fAddress << std::dec << RESET;
            continue;
        }
        else
            LOG(DEBUG) << BOLDGREEN << std::hex << +cRegItem.fAddress << std::dec << "\t" << +cRegItem.fControlReg << RESET;
        EncodeReg(cRegItem, pChip, cVecReq, true, false);
    }

    bool cSucess = true;
    if(cVecReq.size() > 0)
    {
        ReadChipBlockReg(cVecReq);
        size_t cIndx = 0;
        for(auto& cRegItem: pRegisterItems)
        {
            uint8_t cChipId;
            bool    cRead   = true;
            bool    cFailed = false;
            DecodeReg(cRegItem, cChipId, cVecReq.at(cIndx), cRead, cFailed);
            LOG(DEBUG) << BOLDYELLOW << "D19cI2CInterface::MultiRead Reg#" << +cIndx << " at 0x" << std::hex << +cRegItem.fAddress << " set to 0x" << +cRegItem.fValue << " page " << +cRegItem.fPage
                       << std::dec << RESET;
            cSucess = cSucess && !cFailed;
            cIndx++;
        }
    }
    return cSucess;
}
bool D19cI2CInterface::SingleRead(Chip* pChip, ChipRegItem& pRegisterItem)
{
    std::vector<ChipRegItem> cItems;
    cItems.push_back(pRegisterItem);
    bool cSuccess = MultiRead(pChip, cItems);
    pRegisterItem = cItems.at(0);
    return cSuccess;
}

// D19c I2C write and read
bool D19cI2CInterface::WriteI2C(std::vector<uint32_t>& pVecSend, std::vector<uint32_t>& pReplies, bool pReadback, bool pBroadcast)
{
    bool cFailed(false);
    // reset the I2C controller
    fTheRegManager->WriteReg("fc7_daq_ctrl.command_processor_block.i2c.control.reset_fifos", 0x1);
    // usleep (10);
    try
    {
        fTheRegManager->WriteBlockReg("fc7_daq_ctrl.command_processor_block.i2c.command_fifo", pVecSend);
    }
    catch(Exception& except)
    {
        throw except;
    }

    uint32_t cNReplies = 0;
    for(auto word: pVecSend)
    {
        // if read or readback for write == 1, then count
        if(fI2CVersion >= 1)
        {
            // uint32_t cWord = (pLinkId << 29) | (0 << 28) | (0 << 27) | (pHybridId << 23) | (pCbcId << 18) | (pReadBack << 17) | ((!pWrite) << 16) | (pRegItem.fPage << 8) | (pRegItem.fAddress << 0);
            if((((word & 0x08000000) >> 27) == 0) && ((((word & 0x00010000) >> 16) == 1) or (((word & 0x00020000) >> 17) == 1)))
            {
                if(pBroadcast)
                    cNReplies += fNReadoutChip;
                else
                    cNReplies += 1;
            }
        }
        else
        {
            if((((word & 0x00010000) >> 16) == 1) or (((word & 0x00080000) >> 19) == 1))
            {
                if(pBroadcast)
                    cNReplies += fNReadoutChip;
                else
                    cNReplies += 1;
            }
        }
    }
    // std::this_thread::sleep_for (std::chrono::microseconds (fWait_us) );
    // usleep (20);
    cFailed = ReadI2C(cNReplies, pReplies);

    return cFailed;
}
bool D19cI2CInterface::ReadI2C(uint32_t pNReplies, std::vector<uint32_t>& pReplies)
{
    bool cFailed(false);

    uint32_t single_WaitingTime = SINGLE_I2C_WAIT * pNReplies;
    uint32_t max_Attempts       = 100;
    uint32_t counter_Attempts   = 0;

    // read the number of received replies from ndata and use this number to compare with the number of expected replies
    // and to read this number 32-bit words from the reply FIFO
    uint32_t cNReplies = 0;
    while(cNReplies != pNReplies)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(single_WaitingTime));
        cNReplies = fTheRegManager->ReadReg("fc7_daq_stat.command_processor_block.i2c.nreplies");

        if(counter_Attempts > max_Attempts)
        {
            LOG(INFO) << "Error: Read " << cNReplies << " I2C replies whereas " << pNReplies << " are expected!";
            ReadErrors();
            cFailed = true;
            break;
        }
        counter_Attempts++;
    }

    try
    {
        pReplies = fTheRegManager->ReadBlockReg("fc7_daq_ctrl.command_processor_block.i2c.reply_fifo", cNReplies);
    }
    catch(Exception& except)
    {
        throw except;
    }

    // reset the i2c controller here?
    return cFailed;
}
void D19cI2CInterface::ReadErrors()
{
    int error_counter = fTheRegManager->ReadReg("fc7_daq_stat.general.global_error.counter");

    if(error_counter == 0)
        LOG(INFO) << "No Errors detected";
    else
    {
        std::vector<uint32_t> pErrors = fTheRegManager->ReadBlockReg("fc7_daq_stat.general.global_error.full_error", error_counter);

        for(auto& cError: pErrors)
        {
            int error_block_id = (cError & 0x0000000f);
            int error_code     = ((cError & 0x00000ff0) >> 4);
            LOG(ERROR) << "Block: " << BOLDRED << error_block_id << RESET << ", Code: " << BOLDRED << error_code << RESET;
        }
    }
}
// Write block reg
bool D19cI2CInterface::WriteChipBlockReg(std::vector<uint32_t>& pVecReg, uint8_t& pWriteAttempts, bool pReadback)
{
    uint8_t cMaxWriteAttempts = 5;
    // the actual write & readback command is in the vector
    std::vector<uint32_t> cReplies;
    bool                  cSuccess = !WriteI2C(pVecReg, cReplies, pReadback, false);

    // here make a distinction: if pReadback is true, compare only the read replies using the binary predicate
    // else, just check that info is 0 and thus the CBC acqnowledged the command if the writeread is 0
    std::vector<uint32_t> cWriteAgain;

    if(pReadback)
    {
        // now use the Template from BeBoardFWInterface to return a vector with all written words that have been read
        // back incorrectly
        cWriteAgain = get_mismatches(pVecReg.begin(), pVecReg.end(), cReplies.begin(), cmd_reply_comp);

        // now clear the initial cmd Vec and set the read-back
        pVecReg.clear();
        pVecReg = cReplies;
    }
    else
    {
        // since I do not read back, I can safely just check that the info bit of the reply is 0 and that it was an
        // actual write reply then i put the replies in pVecReg so I can decode later in CBCInterface cWriteAgain =
        // get_mismatches (pVecReg.begin(), pVecReg.end(), cReplies.begin(), D19cFWInterface::cmd_reply_ack);
        pVecReg.clear();
        pVecReg = cReplies;
    }

    // now check the size of the WriteAgain vector and assert Success or not
    // also check that the number of write attempts does not exceed cMaxWriteAttempts
    if(cWriteAgain.empty())
        cSuccess = true;
    else
    {
        cSuccess = false;

        // if the number of errors is greater than 100, give up
        if(cWriteAgain.size() < 100 && pWriteAttempts < cMaxWriteAttempts)
        {
            if(pReadback)
                LOG(INFO) << BOLDRED << "(WRITE#" << std::to_string(pWriteAttempts) << ") There were " << cWriteAgain.size() << " Readback Errors -trying again!" << RESET;
            else
                LOG(INFO) << BOLDRED << "(WRITE#" << std::to_string(pWriteAttempts) << ") There were " << cWriteAgain.size() << " CBC CMD acknowledge bits missing -trying again!" << RESET;

            pWriteAttempts++;
            this->WriteChipBlockReg(cWriteAgain, pWriteAttempts, true);
        }
        else if(pWriteAttempts >= cMaxWriteAttempts)
        {
            cSuccess       = false;
            pWriteAttempts = 0;
        }
        else
            throw Exception("Too many CBC readback errors - no functional I2C communication. Check the Setup");
    }

    return cSuccess;
}

void D19cI2CInterface::ReadChipBlockReg(std::vector<uint32_t>& pVecReg)
{
    std::vector<uint32_t> cReplies;
    // it sounds weird, but ReadI2C is called inside writeI2c, therefore here I have to write and disable the readback.
    // The actual read command is in the words of the vector, no broadcast, maybe I can get rid of it
    WriteI2C(pVecReg, cReplies, false, false);
    pVecReg.clear();
    pVecReg = cReplies;
}

void D19cI2CInterface::ChipI2CRefresh() { fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control.fast_i2c_refresh", 0x1); }

void D19cI2CInterface::BCEncodeReg(const ChipRegItem& pRegItem, uint8_t pNCbc, std::vector<uint32_t>& pVecReq, bool pReadBack, bool pWrite)
{
    // use fBroadcastCBCId for broadcast commands
    bool pUseMask = false;
    pVecReq.push_back((2 << 28) | (pReadBack << 19) | (pUseMask << 18) | ((pRegItem.fPage) << 17) | ((!pWrite) << 16) | (pRegItem.fAddress << 8) | pRegItem.fValue);
}

bool D19cI2CInterface::BCWriteChipBlockReg(std::vector<uint32_t>& pVecReg, bool pReadback)
{
    std::vector<uint32_t> cReplies;
    bool                  cSuccess = !WriteI2C(pVecReg, cReplies, false, true);

    // just as above, I can check the replies - there will be NCbc * pVecReg.size() write replies and also read replies
    // if I chose to enable readback this needs to be adapted
    if(pReadback)
    {
        // TODO: actually, i just need to check the read write and the info bit in each reply - if all info bits are 0,
        // this is as good as it gets, else collect the replies that faild for decoding - potentially no iterative
        // retrying
        // TODO: maybe I can do something with readback here - think about it
        for(auto& cWord: cReplies)
        {
            // it was a write transaction!
            if(((cWord >> 16) & 0x1) == 0)
            {
                // infor bit is 0 which means that the transaction was acknowledged by the CBC
                // if ( ( (cWord >> 20) & 0x1) == 0)
                cSuccess = true;
                // else cSuccess == false;
            }
            else
                cSuccess = false;

            // LOG(INFO) << std::bitset<32>(cWord) ;
        }

        // cWriteAgain = get_mismatches (pVecReg.begin(), pVecReg.end(), cReplies.begin(),
        // Cbc3Fc7FWInterface::cmd_reply_ack);
        pVecReg.clear();
        pVecReg = cReplies;
    }

    return cSuccess;
}

} // namespace Ph2_HwInterface
