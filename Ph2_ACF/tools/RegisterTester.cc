#include "RegisterTester.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

RegisterTester::RegisterTester() : Tool() { fNBadRegisters = 0; }

// D'tor
RegisterTester::~RegisterTester() {}

void RegisterTester::Initialise()
{
    // this is needed if you're going to use groups anywhere
    initializeRecycleBin();

    // read back original masks
    const auto cTimeStart = std::chrono::system_clock::now();
    fStartTime            = std::chrono::duration_cast<std::chrono::seconds>(cTimeStart.time_since_epoch()).count();
    LOG(INFO) << BOLDMAGENTA << "RegisterTester::Initialise at " << fStartTime << " s from epoch." << RESET;

    ContainerFactory::copyAndInitChip<Registers>(*fDetectorContainer, fRegList);
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    Registers& cRegList = fRegList.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<Registers>();
                    const ChipRegMap& cOriginalMap = cChip->getRegMap();
                    for(auto cMapItem: cOriginalMap) { cRegList.push_back(std::make_pair(cMapItem.first, cMapItem.second)); }
                    // registers sorted by ... all on page 0 then all on page 1
                    std::sort(cRegList.begin(), cRegList.end(), customLessThanPage); // all to page0 then page 1
                    // sort by address
                    if(fSortOrder == 1) std::sort(cRegList.begin(), cRegList.end(), customLessThanAddress);
                    if(fSortOrder == 2) std::sort(cRegList.begin(), cRegList.end(), customGreaterThanAddress);
                }
            }
        }
    } // board loop to save record of registers

#ifdef __USE_ROOT__
    fDQMHistogrammer.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void RegisterTester::SendHardReset(const OpticalGroup* pOpticalGroup, const Chip* pFrontEndChip)
{
    auto  cBoardId   = pOpticalGroup->getBeBoardId();
    auto  cBoardIter = std::find_if(fDetectorContainer->begin(), fDetectorContainer->end(), [&cBoardId](Ph2_HwDescription::BeBoard* x) { return x->getId() == cBoardId; });
    auto& clpGBT     = pOpticalGroup->flpGBT;
    bool  cWithLpGBT = (clpGBT != nullptr);
    if(!cWithLpGBT) { fBeBoardInterface->ChipReset((*cBoardIter)); }
    else
    {
        if(pFrontEndChip->getFrontEndType() == FrontEndType::CBC3)
            static_cast<D19clpGBTInterface*>(flpGBTInterface)->resetCBC(clpGBT, pFrontEndChip->getHybridId() % 2);
        else if(pFrontEndChip->getFrontEndType() == FrontEndType::SSA2)
            static_cast<D19clpGBTInterface*>(flpGBTInterface)->resetSSA(clpGBT, pFrontEndChip->getHybridId() % 2);
        else
            static_cast<D19clpGBTInterface*>(flpGBTInterface)->resetMPA(clpGBT, pFrontEndChip->getHybridId() % 2);
    }
}
void RegisterTester::CheckReadRegisters(uint8_t pPageToSelect, uint8_t pNRegisters)
{
    // first just test page toggle
    LOG(INFO) << BOLDMAGENTA << "Test of I2C registers in CBCs on page 0.... READ test" << RESET;

    // Container to hold mismatches per chip
    DetectorDataContainer cMismatches, cPageToggles, cMismatchValues;
    ContainerFactory::copyAndInitChip<std::map<uint32_t, uint32_t>>(*fDetectorContainer, cMismatches);
    ContainerFactory::copyAndInitChip<std::map<uint32_t, uint32_t>>(*fDetectorContainer, cPageToggles);
    ContainerFactory::copyAndInitChip<std::map<uint8_t, uint8_t>>(*fDetectorContainer, cMismatchValues);

    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            // here .. I need to send a hard reset to the Chips
            SendHardReset(cOpticalGroup, cOpticalGroup->getFirstObject()->getFirstObject());
        }
    } // board loop to save record of registers

    // now .. try and change page after config and look for mis-matches
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    if(cChip->getFrontEndType() != FrontEndType::CBC3) continue;

                    Registers& cRegList = fRegList.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<Registers>();
                    uint8_t    cDefPageRegValue = 0;
                    for(auto cItem: cRegList)
                    {
                        if(cItem.first == "FeCtrl&TrgLat2") cDefPageRegValue = cItem.second.fDefValue;
                    }
                    // reset page map control
                    static_cast<CbcInterface*>(fReadoutChipInterface)->resetPageMap();
                    uint8_t cDefPage = cDefPageRegValue >> 7;
                    LOG(INFO) << BOLDBLUE << " After a hard reset default page " << +cDefPage << ".. running READ test on Chip#" << +cChip->getId() << " on hybrid#" << +cHybrid->getId() << RESET;
                    // set page that you want to check
                    static_cast<CbcInterface*>(fReadoutChipInterface)->ConfigurePage(cChip, pPageToSelect, false);
                    uint8_t cCurrentPage  = cChip->getReg("FeCtrl&TrgLat2") >> 7;
                    uint8_t cNPageToggles = 0;
                    if(cCurrentPage != cDefPage)
                    {
                        LOG(INFO) << BOLDYELLOW << "Toggle page from " << +cDefPage << " to " << +cCurrentPage << RESET;
                        cNPageToggles++;
                    }

                    size_t cNReads = 0;
                    size_t cIndex  = 0;
                    // read from register on a given page
                    auto& cComparisons =
                        cMismatches.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<std::map<uint32_t, uint32_t>>();
                    auto& cToggles =
                        cPageToggles.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<std::map<uint32_t, uint32_t>>();
                    auto& cMismatches =
                        cMismatchValues.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<std::map<uint8_t, uint8_t>>();
                    for(auto cItem: cRegList)
                    {
                        if(cItem.second.fPage == pPageToSelect)
                        {
                            if(cIndex < pNRegisters)
                            {
                                uint8_t cCurrentPage = cChip->getReg("FeCtrl&TrgLat2") >> 7;
                                uint8_t cValue       = fReadoutChipInterface->ReadChipReg(cChip, cItem.first) & 0xFF;
                                uint8_t cNewPage     = cChip->getReg("FeCtrl&TrgLat2") >> 7;
                                if(cNewPage != cCurrentPage)
                                {
                                    LOG(DEBUG) << BOLDYELLOW << "Toggle page from " << +cCurrentPage << " to " << +cNewPage << RESET;
                                    cNPageToggles++;
                                }
                                LOG(INFO) << BOLDBLUE << "After reading 0x" << std::hex << +cValue << std::dec << " from register " << cItem.first << " ... have toggled page " << +cNPageToggles
                                          << " time(s) and read from " << +cNReads << " registers so far.... " << RESET;
                                // check if any other registers on this page have values that have changed
                                for(auto cItemToCheck: cRegList)
                                {
                                    if(cItemToCheck.second.fPage == cNewPage)
                                    {
                                        uint32_t cAddress = cItemToCheck.second.fPage << 8 | cItemToCheck.second.fAddress;
                                        uint8_t  cValue   = fReadoutChipInterface->ReadChipReg(cChip, cItemToCheck.first) & 0xFF;
                                        if(cItemToCheck.second.fDefValue != cValue)
                                        {
                                            auto cIter = cComparisons.find(cAddress);
                                            if(cIter == cComparisons.end())
                                                LOG(INFO) << BOLDRED << "\t...Read-back value from register " << cItemToCheck.first << " after " << +cNPageToggles << " page toggles... "
                                                          << " and " << +cNReads << " register reads - read back 0x" << std::hex << +cValue << std::dec << " expected 0x" << std::hex
                                                          << +cItemToCheck.second.fDefValue << std::dec << RESET;
                                            if(cIter == cComparisons.end())
                                            {
                                                cComparisons[cAddress]                     = cNReads;
                                                cToggles[cAddress]                         = cNPageToggles;
                                                cMismatches[cItemToCheck.second.fDefValue] = cValue;
                                            }
                                        }
                                        else
                                        {
                                            LOG(DEBUG) << BOLDGREEN << "\t...Read-back value from register " << cItemToCheck.first << " after " << cNPageToggles << " page toggles... "
                                                       << " read back 0x" << std::hex << +cValue << std::dec << " expected 0x" << std::hex << +cItemToCheck.second.fDefValue << std::dec << RESET;
                                        }
                                        cNReads++;
                                    }
                                } // registers to check
                            }
                            cIndex++;
                        }
                    } // loop over registers
                } // chips
            } // hyrbids
        } // optical group
    } // boards
#ifdef __USE_ROOT__
    fDQMHistogrammer.fillRegisterReadMismatches(cMismatches, cPageToggles, cMismatchValues);
#endif
}
void RegisterTester::CheckWriteRegisters(uint8_t pPageToSelect, uint8_t pNRegisters)
{
    std::vector<std::string> cRegsToSkip{"BandgapFuse", "ChipIDFuse1", "ChipIDFuse2", "ChipIDFuse3"};
    // first just test page toggle
    LOG(INFO) << BOLDMAGENTA << "Test of I2C registers in CBCs .... going to toggle the page with a write command..." << RESET;

    // Container to hold mismatches per chip
    DetectorDataContainer cMismatches, cPageToggles, cMismatchValues;
    ContainerFactory::copyAndInitChip<std::map<uint32_t, uint32_t>>(*fDetectorContainer, cMismatches);
    ContainerFactory::copyAndInitChip<std::map<uint32_t, uint32_t>>(*fDetectorContainer, cPageToggles);
    ContainerFactory::copyAndInitChip<std::map<uint8_t, uint8_t>>(*fDetectorContainer, cMismatchValues);

    // first I want to record the register map for this map
    // retreive original settings for all chips and all back-end boards
    DetectorDataContainer cOriginalList;
    ContainerFactory::copyAndInitChip<Registers>(*fDetectorContainer, cOriginalList);
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            // here .. I need to send a hard reset to the Chips
            SendHardReset(cOpticalGroup, cOpticalGroup->getFirstObject()->getFirstObject());
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    Registers& cRegList = fRegList.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<Registers>();
                    Registers& cOrigRegList =
                        cOriginalList.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<Registers>();
                    for(auto cMapItem: cRegList) { cOrigRegList.push_back(std::make_pair(cMapItem.first, cMapItem.second)); }
                }
            }
        }
    } // board loop to save record of registers

    // now .. try and change page after config and look for mis-matches
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    if(cChip->getFrontEndType() != FrontEndType::CBC3) continue;

                    Registers& cRegList = fRegList.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<Registers>();
                    uint8_t    cDefPageRegValue = 0;
                    for(auto cItem: cRegList)
                    {
                        if(cItem.first == "FeCtrl&TrgLat2") cDefPageRegValue = cItem.second.fDefValue;
                    }
                    // reset page map control
                    static_cast<CbcInterface*>(fReadoutChipInterface)->resetPageMap();
                    uint8_t cDefPage = cDefPageRegValue >> 7;
                    LOG(INFO) << BOLDBLUE << " After a hard reset default page " << +cDefPage << ".. running WRITE test on Chip#" << +cChip->getId() << " on hybrid#" << +cHybrid->getId() << RESET;
                    // set page that you want to check
                    static_cast<CbcInterface*>(fReadoutChipInterface)->ConfigurePage(cChip, pPageToSelect, false);
                    uint8_t cCurrentPage  = cChip->getReg("FeCtrl&TrgLat2") >> 7;
                    uint8_t cNPageToggles = 0;
                    if(cCurrentPage != cDefPage)
                    {
                        LOG(INFO) << BOLDYELLOW << "Toggle page from " << +cDefPage << " to " << +cCurrentPage << RESET;
                        cNPageToggles++;
                    }

                    // write something to page 0
                    uint32_t cNWrites = 0;
                    // uint8_t cCurrentValue  = cChip->getReg("MaskChannel-008-to-001");
                    // uint8_t cThisMask  =  0x1;
                    // cCurrentValue ^= cThisMask;
                    // fReadoutChipInterface->WriteChipReg(cChip, "MaskChannel-008-to-001", cCurrentValue, false);
                    // cNWrites++;

                    // read from register on a given page
                    auto& cComparisons =
                        cMismatches.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<std::map<uint32_t, uint32_t>>();
                    auto& cToggles =
                        cPageToggles.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<std::map<uint32_t, uint32_t>>();
                    auto& cMismatches =
                        cMismatchValues.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<std::map<uint8_t, uint8_t>>();
                    // std::map<uint32_t,uint8_t> cComparisons;
                    uint32_t cNReads = 0;
                    for(auto& cItem: cRegList)
                    {
                        if(std::find(cRegsToSkip.begin(), cRegsToSkip.end(), cItem.first) != cRegsToSkip.end()) continue;

                        if(cItem.second.fPage == pPageToSelect)
                        {
                            if(cNWrites < pNRegisters)
                            {
                                uint8_t cCurrentPage = cChip->getReg("FeCtrl&TrgLat2") >> 7;
                                // flip the LSB
                                uint8_t cWriteVal = cChip->getReg(cItem.first);
                                uint8_t cOrigVal  = cWriteVal;
                                uint8_t cMask     = (fBitToFlip > 0) ? (1 << (fBitToFlip - 1)) : 0;
                                cWriteVal ^= cMask;
                                // write the new value
                                fReadoutChipInterface->WriteChipReg(cChip, cItem.first, cWriteVal, false);
                                // update default value
                                cItem.second.fDefValue = cWriteVal;
                                uint8_t cNewPage       = cChip->getReg("FeCtrl&TrgLat2") >> 7;
                                if(cNewPage != cCurrentPage)
                                {
                                    LOG(DEBUG) << BOLDYELLOW << "Toggle page from " << +cCurrentPage << " to " << +cNewPage << RESET;
                                    cNPageToggles++;
                                }
                                LOG(INFO) << BOLDBLUE << "After writing 0x" << std::hex << +cWriteVal << std::dec << " to register " << cItem.first << " [ change from " << std::hex << +cOrigVal
                                          << std::dec << " ] ... have toggled page " << +cNPageToggles << " time(s) and written to " << +cNWrites << " registers so far.... "
                                          << " and read from " << +cNReads << RESET;
                                // check if any other registers on this page have values that have changed
                                for(auto cItemToCheck: cRegList)
                                {
                                    if(cItemToCheck.second.fPage == cNewPage)
                                    {
                                        uint32_t cAddress = cItemToCheck.second.fPage << 8 | cItemToCheck.second.fAddress;
                                        uint8_t  cValue   = fReadoutChipInterface->ReadChipReg(cChip, cItemToCheck.first) & 0xFF;
                                        cNReads++;
                                        if(cItemToCheck.second.fDefValue != cValue)
                                        {
                                            auto cIter = cComparisons.find(cAddress);
                                            if(cIter == cComparisons.end())
                                                LOG(INFO) << BOLDRED << "\t...Read-back value from register " << cItemToCheck.first << " after " << +cNPageToggles << " page toggles... "
                                                          << " read back 0x" << std::hex << +cValue << std::dec << " expected 0x" << std::hex << +cItemToCheck.second.fDefValue << std::dec << RESET;
                                            if(cIter == cComparisons.end())
                                            {
                                                cComparisons[cAddress]                     = cNWrites;
                                                cToggles[cAddress]                         = cNPageToggles;
                                                cMismatches[cItemToCheck.second.fDefValue] = cValue;
                                            }
                                        }
                                        else
                                        {
                                            LOG(DEBUG) << BOLDGREEN << "\t...Read-back value from register " << cItemToCheck.first << " after " << cNPageToggles << " page toggles... "
                                                       << " read back 0x" << std::hex << +cValue << std::dec << " expected 0x" << std::hex << +cItemToCheck.second.fDefValue << std::dec << RESET;
                                        }
                                    }
                                }
                            }
                            cNWrites++;
                        }
                    } // loop over registers
                } // chips
            } // hyrbids
        } // optical group
    } // boards

    // make sure that original default values are put back
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    Registers& cOrigRegList =
                        cOriginalList.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<Registers>();
                    Registers& cRegList = fRegList.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<Registers>();
                    for(auto& cMapItem: cRegList)
                    {
                        for(auto cItem: cOrigRegList)
                        {
                            if(cItem.first == cMapItem.first && cMapItem.second.fDefValue != cItem.second.fDefValue)
                            {
                                LOG(DEBUG) << BOLDYELLOW << "Setting " << cItem.first << " from 0x" << std::hex << +cMapItem.second.fDefValue << std::dec << " to 0x" << std::hex
                                           << +cItem.second.fDefValue << std::dec << RESET;
                                cMapItem.second.fDefValue = cItem.second.fDefValue;
                                cChip->setReg(cItem.first, cMapItem.second.fDefValue);
                            }
                        }
                    }
                }
            }
        }
    } // board loop to save record of registers
#ifdef __USE_ROOT__
    fDQMHistogrammer.fillRegisterWriteMismatches(cMismatches, cPageToggles, cMismatchValues);
#endif
}
void RegisterTester::CheckPageSwitchRead(uint8_t pPageToSelect, uint8_t pNRegisters)
{
    // first just test page toggle
    LOG(INFO) << BOLDMAGENTA << "Test of I2C registers in CBCs .... going to toggle the page with a read command..." << RESET;

    // Container to hold mismatches per chip
    DetectorDataContainer cMismatches, cPageToggles, cMismatchValues, cTotalReadAttempts, cTotalPageToggles;
    ContainerFactory::copyAndInitChip<std::map<uint32_t, uint32_t>>(*fDetectorContainer, cMismatches);
    ContainerFactory::copyAndInitChip<std::map<uint32_t, uint32_t>>(*fDetectorContainer, cPageToggles);
    ContainerFactory::copyAndInitChip<size_t>(*fDetectorContainer, cTotalReadAttempts);
    ContainerFactory::copyAndInitChip<size_t>(*fDetectorContainer, cTotalPageToggles);
    ContainerFactory::copyAndInitChip<std::map<uint8_t, uint8_t>>(*fDetectorContainer, cMismatchValues);

    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            // here .. I need to send a hard reset to the Chips
            SendHardReset(cOpticalGroup, cOpticalGroup->getFirstObject()->getFirstObject());
        }
    } // board loop to save record of registers

    // now .. try and change page after config and look for mis-matches
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    if(cChip->getFrontEndType() != FrontEndType::CBC3) continue;

                    Registers& cRegList = fRegList.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<Registers>();
                    uint8_t    cDefPageRegValue = 0;
                    for(auto cItem: cRegList)
                    {
                        if(cItem.first == "FeCtrl&TrgLat2") cDefPageRegValue = cItem.second.fDefValue;
                    }
                    // reset page map control
                    static_cast<CbcInterface*>(fReadoutChipInterface)->resetPageMap();
                    uint8_t cDefPage = cDefPageRegValue >> 7;
                    LOG(INFO) << BOLDYELLOW << " After a hard reset default page " << +cDefPage << ".. running READ test on Chip#" << +cChip->getId() << " on hybrid#" << +cHybrid->getId() << RESET;

                    // set page that you want to check
                    // size_t cNPageToggles=0;
                    auto& cNPageToggles = cTotalPageToggles.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<size_t>();
                    cNPageToggles       = 0;
                    uint8_t cCurrentPage = cChip->getReg("FeCtrl&TrgLat2") >> 7;
                    static_cast<CbcInterface*>(fReadoutChipInterface)->ConfigurePage(cChip, pPageToSelect, false);
                    uint8_t cNewPage = cChip->getReg("FeCtrl&TrgLat2") >> 7;
                    if(cNewPage != cCurrentPage)
                    {
                        LOG(DEBUG) << BOLDYELLOW << "Toggle page from " << +cCurrentPage << " to " << +cNewPage << RESET;
                        cNPageToggles++;
                    }

                    auto& cNReads = cTotalReadAttempts.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<size_t>();
                    cNReads       = 0;
                    size_t cIndex = 0;
                    // read from register on a given page
                    auto& cComparisons =
                        cMismatches.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<std::map<uint32_t, uint32_t>>();
                    auto& cToggles =
                        cPageToggles.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<std::map<uint32_t, uint32_t>>();
                    auto& cMismatches =
                        cMismatchValues.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<std::map<uint8_t, uint8_t>>();
                    for(auto cItem: cRegList)
                    {
                        if(cItem.second.fPage == pPageToSelect)
                        {
                            if(cIndex < pNRegisters)
                            {
                                uint8_t cCurrentPage = cChip->getReg("FeCtrl&TrgLat2") >> 7;
                                uint8_t cValue       = fReadoutChipInterface->ReadChipReg(cChip, cItem.first) & 0xFF;
                                cNewPage             = cChip->getReg("FeCtrl&TrgLat2") >> 7;
                                if(cNewPage != cCurrentPage)
                                {
                                    LOG(DEBUG) << BOLDYELLOW << "Toggle page from " << +cCurrentPage << " to " << +cNewPage << RESET;
                                    cNPageToggles++;
                                }
                                cNReads++;
                                LOG(DEBUG) << BOLDBLUE << "After reading 0x" << std::hex << +cValue << std::dec << " from register " << cItem.first << " ... have toggled page " << cNPageToggles
                                           << " time(s) and read from " << +cNReads << " registers so far.... " << RESET;
                                // check if any other registers on this page have values that have changed
                                for(auto cItemToCheck: cRegList)
                                {
                                    if(cItemToCheck.second.fPage == cNewPage)
                                    {
                                        uint32_t cAddress = cItemToCheck.second.fPage << 8 | cItemToCheck.second.fAddress;
                                        uint8_t  cValue   = fReadoutChipInterface->ReadChipReg(cChip, cItemToCheck.first) & 0xFF;
                                        if(cItemToCheck.second.fDefValue != cValue)
                                        {
                                            auto cIter = cComparisons.find(cAddress);
                                            if(cIter == cComparisons.end())
                                                LOG(INFO) << BOLDRED << "\t...Read-back value from register " << cItemToCheck.first << " after " << cNPageToggles << " page toggles... "
                                                          << " read back 0x" << std::hex << +cValue << std::dec << " expected 0x" << std::hex << +cItemToCheck.second.fDefValue << std::dec << RESET;
                                            if(cIter == cComparisons.end())
                                            {
                                                cComparisons[cAddress]                     = cNReads;
                                                cToggles[cAddress]                         = cNPageToggles;
                                                cMismatches[cItemToCheck.second.fDefValue] = cValue;
                                            }
                                        }
                                        else
                                        {
                                            LOG(DEBUG) << BOLDGREEN << "\t...Read-back value from register " << cItemToCheck.first << " after " << cNPageToggles << " page toggles... "
                                                       << " read back 0x" << std::hex << +cValue << std::dec << " expected 0x" << std::hex << +cItemToCheck.second.fDefValue << std::dec << RESET;
                                        }
                                        cNReads++;
                                    }
                                }
                                // cToggles[ cItem.second.fPage << 8 | cItem.second.fAddress ] = cNPageToggles;
                                // set back default page
                                if(fReturnToDefPage)
                                {
                                    static_cast<CbcInterface*>(fReadoutChipInterface)->ConfigurePage(cChip, cDefPage, false);
                                    cCurrentPage = cChip->getReg("FeCtrl&TrgLat2") >> 7;
                                    if(cNewPage != cCurrentPage)
                                    {
                                        LOG(DEBUG) << BOLDYELLOW << "Toggle page from " << +cNewPage << " to " << +cCurrentPage << RESET;
                                        cNPageToggles++;
                                    }
                                }
                            }
                            cIndex++;
                        }
                    } // loop over registers
                    LOG(INFO) << BOLDBLUE << "Hybrid#" << +cHybrid->getId() << " Chip#" << +cChip->getId() << " found " << +cMismatches.size() << " mismatches in " << +cNReads << " reads and "
                              << +cNPageToggles << " page toggles." << RESET;
                } // chips
            } // hyrbids
        } // optical group
    } // boards
#ifdef __USE_ROOT__
    fDQMHistogrammer.fillRegisterReadMismatches(cMismatches, cPageToggles, cMismatchValues);
    fDQMHistogrammer.fillRegisterReadCounts(cTotalReadAttempts, cTotalPageToggles);
#endif
}
void RegisterTester::CheckPageSwitchWrite(uint8_t pPageToSelect, uint8_t pNRegisters)
{
    std::vector<std::string> cRegsToSkip{"BandgapFuse", "ChipIDFuse1", "ChipIDFuse2", "ChipIDFuse3"};
    // first just test page toggle
    LOG(INFO) << BOLDMAGENTA << "Test of I2C registers in CBCs .... going to toggle the page with a write command..." << RESET;

    // Container to hold mismatches per chip
    DetectorDataContainer cMismatches, cPageToggles, cMismatchValues, cTotalWriteAttempts, cTotalPageToggles, cTotalReadAttempts;
    ContainerFactory::copyAndInitChip<std::map<uint32_t, uint32_t>>(*fDetectorContainer, cMismatches);
    ContainerFactory::copyAndInitChip<std::map<uint32_t, uint32_t>>(*fDetectorContainer, cPageToggles);
    ContainerFactory::copyAndInitChip<std::map<uint8_t, uint8_t>>(*fDetectorContainer, cMismatchValues);
    ContainerFactory::copyAndInitChip<size_t>(*fDetectorContainer, cTotalWriteAttempts);
    ContainerFactory::copyAndInitChip<size_t>(*fDetectorContainer, cTotalPageToggles);
    ContainerFactory::copyAndInitChip<size_t>(*fDetectorContainer, cTotalReadAttempts);

    // first I want to record the register map for this map
    // retreive original settings for all chips and all back-end boards
    DetectorDataContainer cOriginalList;
    ContainerFactory::copyAndInitChip<Registers>(*fDetectorContainer, cOriginalList);
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            // here .. I need to send a hard reset to the Chips
            SendHardReset(cOpticalGroup, cOpticalGroup->getFirstObject()->getFirstObject());
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    Registers& cRegList = fRegList.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<Registers>();
                    Registers& cOrigRegList =
                        cOriginalList.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<Registers>();
                    for(auto cMapItem: cRegList) { cOrigRegList.push_back(std::make_pair(cMapItem.first, cMapItem.second)); }
                }
            }
        }
    } // board loop to save record of registers

    // now .. try and change page after config and look for mis-matches
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    if(cChip->getFrontEndType() != FrontEndType::CBC3) continue;

                    Registers& cRegList = fRegList.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<Registers>();
                    uint8_t    cDefPageRegValue = 0;
                    for(auto cItem: cRegList)
                    {
                        if(cItem.first == "FeCtrl&TrgLat2") cDefPageRegValue = cItem.second.fDefValue;
                    }
                    // reset page map control
                    static_cast<CbcInterface*>(fReadoutChipInterface)->resetPageMap();
                    uint8_t cDefPage = cDefPageRegValue >> 7;
                    LOG(INFO) << BOLDYELLOW << " After a hard reset default page " << +cDefPage << ".. running WRITE test on Chip#" << +cChip->getId() << " on hybrid#" << +cHybrid->getId() << RESET;
                    // read from register on a given page
                    auto& cComparisons =
                        cMismatches.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<std::map<uint32_t, uint32_t>>();
                    auto& cToggles =
                        cPageToggles.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<std::map<uint32_t, uint32_t>>();
                    auto& cMismatches =
                        cMismatchValues.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<std::map<uint8_t, uint8_t>>();
                    // std::map<uint32_t,uint8_t> cComparisons;
                    auto& cNPageToggles = cTotalPageToggles.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<size_t>();
                    cNPageToggles       = 0;
                    auto& cNWrites = cTotalWriteAttempts.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<size_t>();
                    cNWrites       = 0;
                    size_t cWriteCount = 0;
                    auto&  cNReads     = cTotalReadAttempts.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<size_t>();
                    cNReads            = 0;
                    uint8_t cCurrentPage = cChip->getReg("FeCtrl&TrgLat2") >> 7;

                    // set page that you want to check
                    static_cast<CbcInterface*>(fReadoutChipInterface)->ConfigurePage(cChip, pPageToSelect, false);
                    cCurrentPage = cChip->getReg("FeCtrl&TrgLat2") >> 7;
                    if(cCurrentPage != cDefPage)
                    {
                        LOG(INFO) << BOLDYELLOW << "Toggle page from " << +cDefPage << " to " << +cCurrentPage << RESET;
                        cNPageToggles++;
                    }

                    for(auto& cItem: cRegList)
                    {
                        if(std::find(cRegsToSkip.begin(), cRegsToSkip.end(), cItem.first) != cRegsToSkip.end()) continue;

                        if(cItem.second.fPage == pPageToSelect)
                        {
                            if(cWriteCount < pNRegisters)
                            {
                                cCurrentPage = cChip->getReg("FeCtrl&TrgLat2") >> 7;
                                // flip the LSB
                                uint8_t cWriteVal = cChip->getReg(cItem.first);
                                uint8_t cOrigVal  = cWriteVal;
                                uint8_t cMask     = (fBitToFlip > 0) ? (1 << (fBitToFlip - 1)) : 0;
                                cWriteVal ^= cMask;
                                // write the new value
                                fReadoutChipInterface->WriteChipReg(cChip, cItem.first, cWriteVal, false);
                                cNWrites++;
                                // update default value
                                cItem.second.fDefValue = cWriteVal;
                                uint8_t cNewPage       = cChip->getReg("FeCtrl&TrgLat2") >> 7;
                                if(cNewPage != cCurrentPage)
                                {
                                    LOG(DEBUG) << BOLDYELLOW << "Toggle page from " << +cCurrentPage << " to " << +cNewPage << RESET;
                                    cNPageToggles++;
                                }
                                LOG(DEBUG) << BOLDBLUE << "After writing 0x" << std::hex << +cWriteVal << std::dec << " to register " << cItem.first << " [ change from " << std::hex << +cOrigVal
                                           << std::dec << " ] ... have toggled page " << +cNPageToggles << " time(s) and written to " << +cNWrites << " registers so far.... "
                                           << " and read from " << +cNReads << RESET;
                                // check if any other registers on this page have values that have changed
                                for(auto cItemToCheck: cRegList)
                                {
                                    if(cItemToCheck.second.fPage == cNewPage)
                                    {
                                        uint32_t cAddress = cItemToCheck.second.fPage << 8 | cItemToCheck.second.fAddress;
                                        uint8_t  cValue   = fReadoutChipInterface->ReadChipReg(cChip, cItemToCheck.first) & 0xFF;
                                        cNReads++;
                                        if(cItemToCheck.second.fDefValue != cValue)
                                        {
                                            auto cIter = cComparisons.find(cAddress);
                                            if(cIter == cComparisons.end())
                                                LOG(INFO) << BOLDRED << "\t...Read-back value from register " << cItemToCheck.first << " after " << cNPageToggles << " page toggles... "
                                                          << " read back 0x" << std::hex << +cValue << std::dec << " expected 0x" << std::hex << +cItemToCheck.second.fDefValue << std::dec << RESET;
                                            if(cIter == cComparisons.end())
                                            {
                                                cComparisons[cAddress]                     = cNWrites;
                                                cToggles[cAddress]                         = cNPageToggles;
                                                cMismatches[cItemToCheck.second.fDefValue] = cValue;
                                            }
                                        }
                                        else
                                        {
                                            LOG(DEBUG) << BOLDGREEN << "\t...Read-back value from register " << cItemToCheck.first << " after " << cNPageToggles << " page toggles... "
                                                       << " read back 0x" << std::hex << +cValue << std::dec << " expected 0x" << std::hex << +cItemToCheck.second.fDefValue << std::dec << RESET;
                                        }
                                    }
                                }
                                // set back default page
                                if(fReturnToDefPage)
                                {
                                    static_cast<CbcInterface*>(fReadoutChipInterface)->ConfigurePage(cChip, cDefPage, false);
                                    cCurrentPage = cChip->getReg("FeCtrl&TrgLat2") >> 7;
                                    if(cNewPage != cCurrentPage)
                                    {
                                        LOG(DEBUG) << BOLDYELLOW << "Toggle page from " << +cNewPage << " to " << +cCurrentPage << RESET;
                                        cNPageToggles++;
                                    }
                                }
                            }
                            cWriteCount++;
                        }
                    } // loop over registers
                } // chips
            } // hyrbids
        } // optical group
    } // boards

    // make sure that original default values are put back
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    Registers& cOrigRegList =
                        cOriginalList.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<Registers>();
                    Registers& cRegList = fRegList.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<Registers>();
                    for(auto& cMapItem: cRegList)
                    {
                        for(auto cItem: cOrigRegList)
                        {
                            if(cItem.first == cMapItem.first)
                            {
                                cMapItem.second.fDefValue = cItem.second.fDefValue;
                                cChip->setReg(cItem.first, cMapItem.second.fDefValue);
                            }
                        }
                    }
                }
            }
        }
    } // board loop to save record of registers
#ifdef __USE_ROOT__
    fDQMHistogrammer.fillRegisterWriteMismatches(cMismatches, cPageToggles, cMismatchValues);
    fDQMHistogrammer.fillRegisterWriteCounts(cTotalWriteAttempts, cTotalReadAttempts, cTotalPageToggles);
#endif
}
void RegisterTester::RegisterTest()
{
    std::vector<std::string> cRegsToSkip{"Bandgap", "ChipIDFuse", "FeCtrl&TrgLat2"};
    size_t                   cAttempts   = 1;
    uint8_t                  cTestFlavor = 0;
    // first just test page toggle
    LOG(INFO) << BOLDMAGENTA << "Test" << +cTestFlavor << " of I2C registers in CBCs .... just going to toggle the page without writing..." << RESET;
    uint8_t cSortOrder = 0;
    // uint8_t cFirstPage=(cSortOrder==0)? 0 : 1;
    // std::vector<uint8_t> cPages{cFirstPage,static_cast<uint8_t>(~cFirstPage&0x01)};//static_cast<uint8_t>(~cFirstPage&0x01),cFirstPage,static_cast<uint8_t>(~cFirstPage&0x01)};

    // first I want to record the register map for this map
    // retreive original settings for all chips and all back-end boards
    DetectorDataContainer cRegListContainer;
    ContainerFactory::copyAndInitChip<Registers>(*fDetectorContainer, cRegListContainer);
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    Registers& cRegList =
                        cRegListContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<Registers>();
                    const ChipRegMap& cOriginalMap = cChip->getRegMap();
                    for(auto cMapItem: cOriginalMap)
                    {
                        if(std::find(cRegsToSkip.begin(), cRegsToSkip.end(), cMapItem.first) != cRegsToSkip.end()) continue;
                        LOG(DEBUG) << BOLDMAGENTA << "Will configure register " << cMapItem.first << " on page " << +cMapItem.second.fPage << " with address " << +cMapItem.second.fAddress
                                   << " with value 0x" << std::hex << +cMapItem.second.fValue << std::dec << RESET;
                        cRegList.push_back(std::make_pair(cMapItem.first, cMapItem.second));
                    }
                    // registers sorted by ... all on page 0 then all on page 1
                    if(cSortOrder == 0) std::sort(cRegList.begin(), cRegList.end(), customLessThanPage);    // all to page0 then page 1
                    if(cSortOrder == 1) std::sort(cRegList.begin(), cRegList.end(), customGreaterThanPage); // all to page1 then page 0
                }
            }
        }
    } // board loop to save record of registers

    // container to store default register values after a hard reset
    LOG(INFO) << BOLDMAGENTA << "Reading back default register values after a hard reset ... using values stored in register map " << RESET;
    DetectorDataContainer cDefRegListContainer;
    ContainerFactory::copyAndInitChip<Registers>(*fDetectorContainer, cDefRegListContainer);
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    Registers& cOriginaList =
                        cRegListContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<Registers>();
                    Registers& cList =
                        cDefRegListContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<Registers>();
                    // set page to that of the first register in the map
                    // if( cChip->getFrontEndType() == FrontEndType::CBC3 ) static_cast<CbcInterface*>(fReadoutChipInterface)->ConfigurePage( cChip, 1);//(*cOriginalMap.begin()).second.fPage );
                    for(auto cListItem: cOriginaList)
                    {
                        if(std::find(cRegsToSkip.begin(), cRegsToSkip.end(), cListItem.first) != cRegsToSkip.end()) continue;

                        auto cRegItem   = cListItem.second;
                        cRegItem.fValue = (uint8_t)(cChip->getRegItem(cListItem.first).fDefValue); // fReadoutChipInterface->ReadChipReg(cChip, cListItem.first);
                        cList.push_back(std::make_pair(cListItem.first, cRegItem));
                        LOG(DEBUG) << BOLDMAGENTA << "Default value after a hard reset of register " << cListItem.first << " is 0x" << std::hex << +cRegItem.fValue << std::dec
                                   << " value after configuration should be 0x" << std::hex << cListItem.second.fValue << std::dec << RESET;
                    } // map
                } // chip
            } // hybrid
        } // OG
    } // board loop to save record of registers

    for(size_t cAttempt = 0; cAttempt < cAttempts; cAttempt++)
    {
        // now .. try and change page after config and look for mis-matches
        for(auto cBoard: *fDetectorContainer)
        {
            for(auto cOpticalGroup: *cBoard)
            {
                for(auto cHybrid: *cOpticalGroup)
                {
                    for(auto cChip: *cHybrid)
                    {
                        if(cChip->getFrontEndType() != FrontEndType::CBC3) continue;

                        LOG(INFO) << BOLDMAGENTA << "Chip#" << +cChip->getId() << " on hybrid" << +cHybrid->getId() << RESET;
                        Registers& cExpectedLst =
                            cDefRegListContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<Registers>();
                        Registers cSensitiveRegisters;
                        cSensitiveRegisters.clear();
                        std::vector<int> cPageToggles(0);
                        size_t           cMatches = 0;
                        size_t           cNRegs   = 0;
                        for(auto& cItem: cExpectedLst) // loop over what I think the current values are
                        {
                            // here .. I need to send a hard reset to the Chips
                            SendHardReset(cOpticalGroup, cChip);

                            // reset page map control
                            static_cast<CbcInterface*>(fReadoutChipInterface)->resetPageMap();

                            // read previous page
                            uint8_t cPreviousPage = static_cast<CbcInterface*>(fReadoutChipInterface)->GetLastPage(cChip);
                            LOG(DEBUG) << BOLDMAGENTA << "Page after a hard reset is " << +cPreviousPage << RESET;
                            // if this page is the same .. ignore this register for now
                            if(cItem.second.fPage == cPreviousPage) continue;

                            // so now I expect the value to be the default
                            // compare the value read back from the chip against what is expected
                            // if I'm reading from a different page - this means I'm switching pages
                            LOG(DEBUG) << BOLDMAGENTA << "\t... Reading back value from register " << cItem.first << " on page " << +cItem.second.fPage << " with expected value 0x" << std::hex
                                       << +cItem.second.fValue << std::dec << " when previous page is " << +cPreviousPage << RESET;
                            auto cReadBack = fReadoutChipInterface->ReadChipReg(cChip, cItem.first);
                            auto cValue    = cItem.second.fValue;

                            bool cCorrupted = cReadBack != cValue;
                            if(cCorrupted)
                            {
                                std::string cRegName = cItem.first;
                                if(std::find_if(cSensitiveRegisters.begin(), cSensitiveRegisters.end(), [&cRegName](Register x) { return x.first == cRegName; }) == cSensitiveRegisters.end())
                                {
                                    cSensitiveRegisters.push_back(cItem);
                                    cPageToggles.push_back(cPreviousPage - cItem.second.fPage);
                                }
                                LOG(INFO) << BOLDRED << "\t\t\t\t..When switching from page " << +cPreviousPage << " to page " << +cItem.second.fPage << " register# " << +cNRegs
                                          << "\t\t...Mismatch in I2C register " << cItem.first << " value stored in map is 0x" << std::hex << +cItem.second.fValue << std::dec
                                          << " value read-back from chip is 0x" << std::hex << +cReadBack << std::dec << RESET;
                                // if register value does not match
                                // then update value in memory
                                cItem.second.fValue = cReadBack;
                            }
                            else
                            {
                                LOG(DEBUG) << BOLDGREEN << "\t\t\t\t..When switching from page " << +cPreviousPage << " to page " << +cItem.second.fPage << " register# " << +cNRegs
                                           << "\t\t...Match in I2C register " << cItem.first << " value stored in map is 0x" << std::hex << +cItem.second.fValue << std::dec
                                           << " value read-back from chip is 0x" << std::hex << +cReadBack << std::dec << RESET;
                                cMatches++;
                            }
                            cPreviousPage = cItem.second.fPage;
                            cNRegs++;

                        } // loop over current values and compare what I read back against what I have stored in memory
                        float cMatchedPerc = cMatches / (float)(cNRegs);
                        if(cMatches == cNRegs)
                            LOG(INFO) << BOLDGREEN << "\t\t.. Found read-back matched fraction from other registers to be " << 100 * cMatchedPerc << " percent. Found " << +cSensitiveRegisters.size()
                                      << " sensitive registers." << RESET;
                        else
                            LOG(INFO) << BOLDRED << "\t\t.. Found read-back matched fraction from other registers to be " << 100 * cMatchedPerc << " percent. Found " << +cSensitiveRegisters.size()
                                      << " sensitive registers." << RESET;
                        std::sort(cSensitiveRegisters.begin(), cSensitiveRegisters.end(), customGreaterThanAddress);
                        for(auto cSensitiveRegister: cSensitiveRegisters)
                        {
                            LOG(INFO) << BOLDRED << "Sensitive register " << cSensitiveRegister.first << " on page " << +cSensitiveRegister.second.fPage << RESET;
                        }
                    } // chip
                } // hybrid
            } // OG
        } // board loop to run test
    }
    cTestFlavor++;
}
void RegisterTester::TestRegisters()
{
    // two bit patterns to rest registers with
    uint8_t cFirstBitPattern  = 0xAA;
    uint8_t cSecondBitPattern = 0x55;

    std::ofstream report;
    report.open(fDirectoryName + "/TestReport.txt", std::ofstream::out | std::ofstream::app);
    char line[240];

    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    auto cMap = cChip->getRegMap();

                    for(const auto& cReg: cMap)
                    {
                        if(!fReadoutChipInterface->WriteChipReg(cChip, cReg.first, cFirstBitPattern, true))
                        {
                            sprintf(line, "# Writing 0x%.2x to CBC Register %s FAILED.\n", cFirstBitPattern, (cReg.first).c_str());
                            LOG(INFO) << BOLDRED << line << RESET;
                            report << line;
                            fBadRegisters[cChip->getId()].insert(cReg.first);
                            fNBadRegisters++;
                        }

                        // sleep for 100 ns between register writes
                        std::this_thread::sleep_for(std::chrono::nanoseconds(100));

                        if(!fReadoutChipInterface->WriteChipReg(cChip, cReg.first, cSecondBitPattern, true))
                        {
                            sprintf(line, "# Writing 0x%.2x to CBC Register %s FAILED.\n", cSecondBitPattern, (cReg.first).c_str());
                            LOG(INFO) << BOLDRED << line << RESET;
                            report << line;
                            fBadRegisters[cChip->getId()].insert(cReg.first);
                            fNBadRegisters++;
                        }

                        // sleep for 100 ns between register writes
                        std::this_thread::sleep_for(std::chrono::nanoseconds(100));
                    }

                    fBeBoardInterface->ChipReSync(cBoard);
                }
            }
        }
    }

    report.close();
}

// Reload CBC registers from file found in directory.
// If no directory is given use the default files for the different operational modes found in Ph2_ACF/settings
void RegisterTester::ReconfigureRegisters(std::string pDirectoryName)
{
    for(auto cBoard: *fDetectorContainer)
    {
        fBeBoardInterface->ChipReset(cBoard);

        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    std::string pRegFile;

                    if(pDirectoryName.empty())
                        pRegFile = "settings/CbcFiles/CBC3_default.txt";
                    else
                    {
                        char buffer[120];
                        sprintf(buffer, "%s/FE%dCBC%d.txt", pDirectoryName.c_str(), cHybrid->getId(), cChip->getId());
                        pRegFile = buffer;
                    }

                    cChip->loadfRegMap(pRegFile);
                    fReadoutChipInterface->ConfigureChip(cChip);
                    LOG(INFO) << GREEN << "\t\t Successfully (re)configured Chip" << int(cChip->getId()) << "'s regsiters from " << pRegFile << " ." << RESET;
                }
            }
        }

        fBeBoardInterface->ChipReSync(cBoard);
    }
}
void RegisterTester::PrintTestReport()
{
    std::ofstream report(fDirectoryName + "/registers_test.txt"); // Creates a file in the current directory
    PrintTestResults(report);
    report.close();
}
void RegisterTester::PrintTestResults(std::ostream& os)
{
    os << "Testing Chip Registers one-by-one with complimentary bit-patterns (0xAA, 0x55)" << std::endl;

    for(const auto& cCbc: fBadRegisters)
    {
        os << "Malfunctioning Registers on Chip " << cCbc.first << " : " << std::endl;

        for(const auto& cReg: cCbc.second) os << cReg << std::endl;
    }

    LOG(INFO) << BOLDBLUE << "Channels diagnosis report written to: " + fDirectoryName + "/registers_test.txt" << RESET;
}

bool RegisterTester::PassedTest()
{
    bool passFlag = ((int)(fNBadRegisters) == 0) ? true : false;
    return passFlag;
}

//
void RegisterTester::writeObjects()
{
#ifdef __USE_ROOT__
    this->SaveResults();
    fDQMHistogrammer.process();
#endif
}
// State machine control functions
void RegisterTester::Running() { Initialise(); }

void RegisterTester::Stop()
{
    this->SaveResults();
#ifdef __USE_ROOT__
    fResultFile->Flush();
#endif

    SaveResults();
#ifdef __USE_ROOT__
    CloseResultFile();
#endif
    Destroy();
}

void RegisterTester::Pause() {}

void RegisterTester::Resume() {}
