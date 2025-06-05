#if defined(__TCUSB__) && defined(__USE_ROOT__)

#include "OTHybridTester.h"
#include "HWInterface/D19cBackendAlignmentFWInterface.h"
#include "MonitorUtils/SEHMonitor.h"

OTHybridTester::OTHybridTester() : Tool()
{
    // I think that this is where the TC interface should be initialized
    // and where the lpGBT interface should be linked if needed
    // not in system controller
    // as this is very specific to each hybrid testing tool
}

OTHybridTester::~OTHybridTester() {}

void OTHybridTester::CheckConfiguredHw()
{
    bool cSucess = false;
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            std::ignore = cOpticalGroup;
            cSucess     = true;
        }
    }
    if(!cSucess) { throw std::runtime_error("Error: No optical hardware configured!"); }
}

void OTHybridTester::ReadChipIds()
{
    D19clpGBTInterface* clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            if(static_cast<lpGBT*>(cOpticalGroup->flpGBT)->getVersion() == 1)
            {
                uint32_t cChipID = clpGBTInterface->ReadChipFuseID(cOpticalGroup->flpGBT);

                LOG(INFO) << BOLDYELLOW << "lpgbt version 1" << RESET;
                LOG(INFO) << BOLDYELLOW << "lpgbt ID: 0x" << std::hex << +cChipID << std::dec << RESET;
                fillSummaryTree("lpgbt_id", cChipID);
                fillSummaryTree("lpgbt_version", 1);
            }
            else
            {
                LOG(INFO) << BOLDYELLOW << "lpgbt version 0" << RESET;
                LOG(INFO) << BOLDYELLOW << "no lpgbt ID" << RESET;

                fillSummaryTree("lpgbt_id", 0);
                fillSummaryTree("lpgbt_version", 0);
            }
        }
    }
}

void OTHybridTester::InitialiseTestCard(bool cIsSEH)
{
    if(cIsSEH)
    {
        LOG(INFO) << BOLDYELLOW << "Initializing controller (via usb) for 2S-SEH test system..." << RESET;
        fTC_2SSEH = new TC_2SSEH();
        fIsSEH    = true;
        if(fDetectorMonitor != nullptr) { fDetectorMonitor->setTestCardPointer(fTC_2SSEH); }
    }
    else
    {
        LOG(INFO) << BOLDYELLOW << "Initializing controller (via usb) for PS-ROH test system..." << RESET;
        fTC_PSROH = new TC_PSROH();
        fIsSEH    = false;
    }
}

void OTHybridTester::LpGBTInjectULInternalPattern(uint32_t pPattern)
{
    D19clpGBTInterface* clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            LOG(INFO) << BOLDGREEN << "Internal LpGBT pattern generation" << RESET;
            for(const auto& RxProperty: static_cast<lpGBT*>(cOpticalGroup->flpGBT)->getRxProperties())
            {
                clpGBTInterface->ConfigureRxPRBS(cOpticalGroup->flpGBT, RxProperty.Group, RxProperty.Channel, false);
                clpGBTInterface->ConfigureRxSource(cOpticalGroup->flpGBT, RxProperty.Group, 4);
            }
            clpGBTInterface->ConfigureDPPattern(cOpticalGroup->flpGBT, pPattern);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}

void OTHybridTester::LpGBTInjectULExternalPattern(bool pStart, uint8_t pPattern)
{
    DPInterface cDPInterfacer;
    for(auto cBoard: *fDetectorContainer)
    {
        bool isElectricalFc7 = true;
        for(auto cOpticalGroup: *cBoard)
        {
            if(cOpticalGroup->flpGBT != nullptr)
            {
                isElectricalFc7 = false;
                break;
            }
        }
        if(isElectricalFc7)
        {
            BeBoardFWInterface* pInterface = dynamic_cast<BeBoardFWInterface*>(fBeBoardFWMap.find(cBoard->getId())->second);
            if(pStart)
            {
                LOG(INFO) << BOLDGREEN << "Electrical FC7 pattern generation" << RESET;
                // Check if Emulator is running
                for(int i = 0; i < 5; i++)
                {
                    if(cDPInterfacer.IsRunning(pInterface, 1))
                    {
                        LOG(INFO) << BOLDYELLOW << " STATUS : Data Player is running and will be stopped " << RESET;
                        cDPInterfacer.Stop(pInterface);
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    // Configure and Start DataPlayer
                    cDPInterfacer.Configure(pInterface, pPattern);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    cDPInterfacer.Start(pInterface, 1);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    if(cDPInterfacer.IsRunning(pInterface, 1))
                    {
                        LOG(INFO) << BOLDBLUE << "FE data player " << BOLDGREEN << " running correctly!" << RESET;
                        break;
                    }
                    else
                        LOG(INFO) << BOLDRED << "Could not start FE data player" << RESET;
                }
            }
            else
            {
                LOG(INFO) << BOLDYELLOW << " Data Player will be stopped " << RESET;
                cDPInterfacer.Stop(pInterface);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            return;
        }
    }
}

bool OTHybridTester::LpGBTCheckULPattern(bool pIsExternal, uint8_t pPattern)
{
    bool     res = false;
    uint8_t  cMatch;
    uint8_t  cShift;
    uint8_t  cWrappedByte;
    uint32_t cWrappedData;

    for(auto cBoard: *fDetectorContainer)
    {
        for(auto __attribute__((unused)) cOpticalGroup: *cBoard) { res = true; }
    }
    if(!res)
    {
        LOG(INFO) << BOLDYELLOW << "OTHybridTester::LpGBTCheckULPattern Stopping test. No OpticalGroup enabled!" << RESET;
        return res;
    }

    D19clpGBTInterface* clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
    for(auto cBoard: *fDetectorContainer)
    {
        D19cFWInterface*      cFWInterface      = dynamic_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard));
        D19cTriggerInterface* cTriggerInterface = dynamic_cast<D19cTriggerInterface*>(cFWInterface->getTriggerInterface());

        for(auto cOpticalGroup: *cBoard)
        {
            const std::vector<uint8_t>& cPatternVec = {0x00, 0xff, 0xaa, 0xcc, 0xca};
            for(const auto cPattern: cPatternVec)
            {
                this->LpGBTInjectULExternalPattern(true, cPattern);
                LOG(INFO) << BOLDBLUE << "Checking against : " << std::bitset<8>(cPattern) << RESET;
                for(int hybridNumber = 0; hybridNumber < 2; hybridNumber++)
                {
                    auto cHybridId = 2 * cOpticalGroup->getId() + hybridNumber;
                    if(pIsExternal)
                    {
                        for(const auto& RxProperty: static_cast<lpGBT*>(cOpticalGroup->flpGBT)->getRxProperties())
                        {
                            clpGBTInterface->ConfigureRxPRBS(cOpticalGroup->flpGBT, RxProperty.Group, RxProperty.Channel, false);
                            clpGBTInterface->ConfigureRxSource(cOpticalGroup->flpGBT, RxProperty.Group, 0);
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    }

                    uint8_t cLine  = 0;
                    uint8_t nLines = 0;
                    if(fIsSEH) { nLines = 5; }
                    else { nLines = 6; }

                    do {
                        cFWInterface->selectLink(cOpticalGroup->getId());
                        cFWInterface->WriteReg("fc7_daq_cnfg.physical_interface_block.slvs_debug.hybrid_select", cHybridId);

                        LOG(INFO) << BOLDBLUE << "Stub lines " << RESET;
                        cFWInterface->WriteReg("fc7_daq_cnfg.ddr3_debug.stub_enable", 0x01);
                        cFWInterface->ChipTestPulse();
                        auto                     cWords = cFWInterface->ReadBlockReg("fc7_daq_stat.physical_interface_block.stub_debug", 80);
                        std::vector<std::string> cLines(0);

                        uint32_t cCicOutOutput = cWords[cLine * 10];
                        LOG(INFO) << BOLDBLUE << "Scoped output on Stub Line " << BOLDGREEN << +cLine << BOLDBLUE << ": " << std::bitset<32>(cCicOutOutput) << " for hybrid side " << +hybridNumber
                                  << RESET;

                        cMatch = 32;
                        cShift = 0;
                        for(uint8_t shift = 0; shift < 8; shift++)
                        {
                            cWrappedByte = (cPattern >> shift) | (cPattern << (8 - shift));
                            cWrappedData = (cWrappedByte << 24) | (cWrappedByte << 16) | (cWrappedByte << 8) | (cWrappedByte << 0);
                            LOG(DEBUG) << BOLDBLUE << std::bitset<8>(cWrappedByte) << RESET;
                            LOG(DEBUG) << BOLDBLUE << std::bitset<32>(cWrappedData) << RESET;
                            int popcount = __builtin_popcountll(cWrappedData ^ cCicOutOutput);
                            if(popcount < cMatch)
                            {
                                cMatch = popcount;
                                cShift = shift;
                            }
                            LOG(DEBUG) << BOLDBLUE << "Line " << +cLine << " Shift " << +shift << " Match " << +popcount << RESET;
                        }
                        LOG(INFO) << BOLDBLUE << "Found for stub line " << BOLDWHITE << +cLine << BOLDBLUE << " a minimal bit difference of " << BOLDWHITE << +cMatch << BOLDBLUE
                                  << " for a bit shift of " << BOLDWHITE << +cShift << RESET;
                        LOG(INFO) << Form("stub_%d_hybrid_%d_miss_match_0x%02X", int(cLine), hybridNumber, cPattern) << RESET;
                        fillSummaryTree(Form("stub_%d_hybrid_%d_miss_match_0x%02X", int(cLine), hybridNumber, cPattern), cMatch);
                        fillSummaryTree(Form("stub_%d_hybrid_%d_shift_0x%02X", int(cLine), hybridNumber, cPattern), cShift);

                        if((cMatch == 0)) { LOG(INFO) << BOLDGREEN << "CIC Out Test passed for stub line " << +cLine << " for hybrid side " << +hybridNumber << RESET; }
                        else
                        {
                            LOG(INFO) << BOLDRED << "CIC Out Test failed for stub line " << +cLine << " for hybrid side " << +hybridNumber << RESET;
                            res = false;
                        }
                        cLine++;
                    } while(cLine < nLines); // making sure missing stub line pair is skipped in 2S case

                    LOG(INFO) << BOLDBLUE << "L1 data " << RESET;
                    uint8_t maxAttempt = 3;
                    for(uint8_t attempt = 0; attempt < maxAttempt; attempt++)
                    {
                        cTriggerInterface->Start();
                        cTriggerInterface->WaitForNTriggers(10);
                        cTriggerInterface->Stop();
                        auto cWordsL1A = cFWInterface->ReadBlockReg("fc7_daq_stat.physical_interface_block.l1a_debug", 50);
                        for(auto cWord: cWordsL1A) LOG(DEBUG) << BOLDBLUE << "# " << std::bitset<32>(cWord) << RESET;
                        uint32_t cCicOutOutputL1A = cWordsL1A[0];
                        LOG(INFO) << BOLDBLUE << "Scoped output on L1A Line: " << std::bitset<32>(cCicOutOutputL1A) << " for hybrid side " << +hybridNumber << RESET;

                        cMatch = 32;
                        cShift = 0;
                        for(uint8_t shift = 0; shift < 8; shift++)
                        {
                            cWrappedByte = (cPattern >> shift) | (cPattern << (8 - shift));
                            cWrappedData = (cWrappedByte << 24) | (cWrappedByte << 16) | (cWrappedByte << 8) | (cWrappedByte << 0);
                            LOG(DEBUG) << BOLDBLUE << std::bitset<8>(cWrappedByte) << RESET;
                            LOG(DEBUG) << BOLDBLUE << std::bitset<32>(cWrappedData) << RESET;
                            int popcount = __builtin_popcountll(cWrappedData ^ cCicOutOutputL1A);
                            if(popcount < cMatch)
                            {
                                cMatch = popcount;
                                cShift = shift;
                            }
                            LOG(DEBUG) << BOLDBLUE << "Line L1A Shift " << +shift << " Match " << +popcount << RESET;
                        }
                        LOG(INFO) << BOLDBLUE << "Found for L1A a minimal bit difference of " << BOLDWHITE << +cMatch << BOLDBLUE << " for a bit shift of " << BOLDWHITE << +cShift << RESET;
                        cFWInterface->getL1ReadoutInterface()->ResetReadout();
                        if((cMatch == 0))
                        {
                            LOG(INFO) << BOLDGREEN << "CIC Out Test passed for L1A line"
                                      << " for hybrid side " << +hybridNumber << RESET;
                            break;
                        }
                        else
                        {
                            LOG(INFO) << BOLDBLUE << "Scoped output on L1A Line: " << std::bitset<32>(cCicOutOutputL1A) << " for hybrid side " << +hybridNumber << RESET;
                            LOG(INFO) << BOLDRED << "CIC Out Test failed for L1A line"
                                      << " for hybrid side " << +hybridNumber << RESET;
                            if(attempt == (maxAttempt - 1)) { res = false; }
                        }
                    }
                    fillSummaryTree(Form("L1A_hybrid_%d_miss_match_0x%02X", hybridNumber, cPattern), cMatch);
                    fillSummaryTree(Form("L1A_hybrid_%d_shift_0x%02X", hybridNumber, cPattern), cShift);
                    /* uint32_t cL1ATotalWrong = 0;
                    uint32_t cL1ATotal      = 0;
                    cWrappedByte            = (cPattern >> cShift) | (cPattern << (8 - cShift));
                    cWrappedData            = (cWrappedByte << 24) | (cWrappedByte << 16) | (cWrappedByte << 8) | (cWrappedByte << 0);
                    for(uint32_t cWord: cWordsL1A)
                    {
                        cL1ATotalWrong += __builtin_popcountll(cWrappedData ^ cWord);
                        cL1ATotal += 32;
                    }
                    LOG(DEBUG) << "L1A total wrong bits: " << BOLDBLUE << +cL1ATotalWrong << " in a total of: " << +cL1ATotal << RESET; */
                }
            }
        }
    }
    return res;
}

void OTHybridTester::LpGBTInjectDLInternalPattern(uint8_t pPattern)
{
    D19clpGBTInterface*      clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
    std::pair<bool, uint8_t> cReturn;
    bool                     cResult = true;
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            uint8_t cSource = 3;
            clpGBTInterface->ConfigureDPPattern(cOpticalGroup->flpGBT, pPattern << 24 | pPattern << 16 | pPattern << 8 | pPattern);
            for(const auto& TxProperty: static_cast<lpGBT*>(cOpticalGroup->flpGBT)->getTxProperties())
            {
                clpGBTInterface->ConfigureTxSource(cOpticalGroup->flpGBT, TxProperty.Group, cSource); // 0 --> link data, 3 --> constant pattern
            }
            // clpGBTInterface->ConfigureTxSource(cOpticalGroup->flpGBT, {,}, cSource); // 0 --> link data, 3 --> constant pattern
            if(!fIsSEH)
            {
                cReturn = PhaseTuneLineEleFC7(0, 0);
                cResult &= cReturn.first;
                cReturn = PhaseTuneLineEleFC7(0, 4);
                cResult &= cReturn.first;
            }

            cReturn = PhaseTuneLineEleFC7(0, 1); // SEH: FCMD_CIC_R
            cResult &= cReturn.first;
            cReturn = PhaseTuneLineEleFC7(0, 2); // SEH: FCMD_CIC_L
            cResult &= cReturn.first;
        }
    }
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(const auto& TxProperty: static_cast<lpGBT*>(cOpticalGroup->flpGBT)->getTxProperties())
            {
                clpGBTInterface->ConfigureTxSource(cOpticalGroup->flpGBT, TxProperty.Group, 0); // 0 --> link data, 3 --> constant pattern
            }
        }
    }
    // if(!cResult) { throw std::runtime_error("Failed to Phase Align "); }
}

bool OTHybridTester::LpGBTTestI2CMaster(const std::vector<uint8_t>& pMasters, int pNTries)
{
    bool cTestSuccess = false;
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto __attribute__((unused)) cOpticalGroup: *cBoard) { cTestSuccess = true; }
    }
    if(!cTestSuccess)
    {
        LOG(INFO) << BOLDYELLOW << "OTHybridTester::LpGBTTestI2CMaster Stopping test. No OpticalGroup enabled!" << RESET;
        return cTestSuccess;
    }
    D19clpGBTInterface* clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
    for(auto cBoard: *fDetectorContainer)
    {
        bool isElectricalFc7 = true;
        for(auto cOpticalGroup: *cBoard)
        {
            if(cOpticalGroup->flpGBT != nullptr)
            {
                isElectricalFc7 = false;
                break;
            }
        }
        if(isElectricalFc7)
        {
            fBeBoardInterface->WriteBoardReg(cBoard, "fc7_daq_ctrl.physical_interface_block.data_player.i2c_slave_reset", 0x01);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            LOG(DEBUG) << BOLDBLUE << "Reset I2C slave in electrical FC7" << RESET;
        }
    }

    // Create variables for TTree branches
    std::vector<std::vector<uint8_t>> cI2CStatusVectVect;
    for(auto cBoard: *fDetectorContainer)
    {
        D19cFWInterface*      pInterface        = static_cast<D19cFWInterface*>(fBeBoardFWMap.find(cBoard->getId())->second);
        D19cOpticalInterface* cOpticalInterface = static_cast<D19cOpticalInterface*>(pInterface->getFEConfigurationInterface());

        for(auto cOpticalGroup: *cBoard)
        {
            auto clpGBT = cOpticalGroup->flpGBT;
            clpGBTInterface->ResetI2C(clpGBT, {0, 1, 2});
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            for(const auto cMasterId: pMasters)
            {
                bool                 cMasterSuccess = true;
                std::vector<uint8_t> cI2CStatusVect;
                struct timeval       stop, start;
                gettimeofday(&start, NULL);
                // do stuff
                uint8_t cFailCntr     = 0;
                uint8_t cFrequency    = (cMasterId == 1) ? 2 : 3;
                uint8_t cSlaveAddress = 0x60;
                for(int cTryCntr = 0; cTryCntr < pNTries; cTryCntr++)
                {
                    uint8_t cI2CStatus = 4; // clpGBTInterface->GetI2CStatus(cOpticalGroup->flpGBT, cMaster);
                    uint8_t cNbyte = 1, cSlaveData = 0x9;
                    uint8_t cMasterConfig = (cNbyte << 2) | (cFrequency << 0);
                    bool    cSuccess      = cOpticalInterface->SingleMultiByteWriteI2C(clpGBT, cMasterId, cMasterConfig, cSlaveAddress, cSlaveData);
                    if(cSuccess)
                    {
                        LOG(DEBUG) << BOLDGREEN << "I2C Master " << +cMasterId << " PASSED" << RESET;
                        cFailCntr = 0;
                    }
                    else
                    {
                        cI2CStatus = clpGBTInterface->GetI2CStatus(clpGBT, cMasterId);
                        LOG(INFO) << GREEN << "I2C Master " << +cMasterId << " -- Status : " << fI2CStatusMap[cI2CStatus] << RESET;
                        LOG(INFO) << BOLDRED << "I2C Master " << +cMasterId << " FAILED" << RESET;
                        LOG(INFO) << BOLDBLUE << "I2C test number " << BOLDRED << +cTryCntr << " failed" << RESET;
                        cFailCntr++;
                        if(cFailCntr >= 5)
                        {
                            cI2CStatusVect.push_back(cI2CStatus);
                            cMasterSuccess &= cSuccess;
                            break;
                        }
                    }
                    cI2CStatusVect.push_back(cI2CStatus);
                    cMasterSuccess &= cSuccess;
                }
                if(cMasterSuccess) { LOG(INFO) << BOLDGREEN << "I2C Master " << +cMasterId << " PASSED the Test Card Test" << RESET; }
                else { LOG(INFO) << BOLDRED << "I2C Master " << +cMasterId << " FAILED the Test Card Test" << RESET; }
                fillSummaryTree(Form("i2cmaster%i", cMasterId), cMasterSuccess);
                gettimeofday(&stop, NULL);
                LOG(INFO) << BOLDBLUE << "Duration " << std::to_string((stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec) << RESET;
                cTestSuccess &= cMasterSuccess;
                cI2CStatusVectVect.push_back(cI2CStatusVect);
            }
        }
    }

    int index = 0;
    for(const auto cMasterId: pMasters)
    {
        auto cI2CTree = new TTree(Form("tI2CMaster%i", cMasterId), Form("I2C Master %i Test Tree", cMasterId));
        cI2CTree->Branch("I2C_Master_status", &cI2CStatusVectVect[index]);
        cI2CTree->Fill();
        fResultFile->cd();
        // cI2CTree->Write();
        index += 1;
    }
    return cTestSuccess;
}

void OTHybridTester::LpGBTTestADC(const std::vector<std::string>& pADCs, uint32_t pMinDACValue, uint32_t pMaxDACValue, uint32_t pStep, bool pCalibrate)
{
    D19clpGBTInterface* clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
    uint8_t             cTrim           = 0;
    uint8_t             cVersion        = 0;
    std::string         registerStr     = "VREFTUNE";
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            // Create TTree for DAC to ADC conversion in lpGBT
            auto cDACtoADCTree = new TTree("tDACtoADC", "DAC to ADC conversion in lpGBT");
            // Create variables for TTree branches
            int              cADCId = -1;
            std::vector<int> cDACValVect;
            std::vector<int> cADCValVect;
            // Create TTree Branches
            cDACtoADCTree->Branch("Id", &cADCId);
            cDACtoADCTree->Branch("DAC", &cDACValVect);
            cDACtoADCTree->Branch("ADC", &cADCValVect);

            if(fIsSEH & pCalibrate)
            {
                cTrim = calibrateADC();
                calibrateCurrentDAC();
            }
            cVersion = static_cast<lpGBT*>(cOpticalGroup->flpGBT)->getVersion();
            if(cVersion == 0) { registerStr = "VREFCNTR"; }

            LOG(INFO) << BOLDBLUE << "VREFTune value " << +cTrim << RESET;
            clpGBTInterface->WriteChipReg(cOpticalGroup->flpGBT, registerStr, cTrim);

            // Create TCanvas & TMultiGraph
            auto cDACtoADCCanvas = new TCanvas("cDACtoADC", "DAC to ADC conversion", 500, 500);
            auto cObj            = gROOT->FindObject("mgDACtoADC");
            if(cObj) delete cObj;
            auto cDACtoADCMultiGraph = new TMultiGraph();
            cDACtoADCMultiGraph->SetName("mgDACtoADC");
            cDACtoADCMultiGraph->SetTitle("lpGBT - DAC to ADC conversion");
            auto dieLegende = new TLegend(0.1, 0.7, 0.48, 0.9);

            LOG(INFO) << BOLDMAGENTA << "Testing ADC channels" << RESET;

            fitter::Linear_Regression<int> cReg_Class;
            std::vector<std::vector<int>>  cfitDataVect(2);
            for(const auto& cADC: pADCs)
            {
                cDACValVect.clear(), cADCValVect.clear();
                cfitDataVect.clear();
                // uint32_t cNValues = (cMaxDAC-cMinDAC)/cStep;
                cADCId = cADC[3] - '0';
                for(int cDACValue = pMinDACValue; cDACValue <= (int)pMaxDACValue; cDACValue += pStep)
                {
                    // Need to confirm conversion factor for 2S-SEH
                    // fTC_2SSEH->set_AMUX(cDACValue, cDACValue);
                    // example to program current Dac for temperature sensor clpGBTInterface->ConfigureCurrentDAC(cOpticalGroup->flpGBT, pADCs,0);
                    if(fIsSEH) { fTC_2SSEH->set_AMUX(cDACValue, cDACValue); }
                    else { fTC_PSROH->dac_output(cDACValue); }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
                    int cADCValue = clpGBTInterface->ReadADC(cOpticalGroup->flpGBT, cADC);
                    LOG(INFO) << BOLDBLUE << "DAC value = 0x" << std::hex << +cDACValue << std::dec << " --- ADC value = 0x" << std::hex << +cADCValue << std::dec << RESET;
                    cDACValVect.push_back(cDACValue);
                    cADCValVect.push_back(cADCValue);
                }
                cDACtoADCTree->Fill();
                auto cDACtoADCGraph = new TGraph(cDACValVect.size(), cDACValVect.data(), cADCValVect.data());
                cDACtoADCGraph->SetName(Form("gADC%i", cADCId));
                cDACtoADCGraph->SetTitle(Form("ADC%i", cADCId));
                cDACtoADCGraph->SetLineColor(cADCId + 1);
                cDACtoADCGraph->SetFillColor(0);
                cDACtoADCGraph->SetLineWidth(3);
                cDACtoADCMultiGraph->Add(cDACtoADCGraph);
                cfitDataVect[0] = cDACValVect;
                cfitDataVect[1] = cADCValVect;
                cReg_Class.fit(cDACValVect, cADCValVect);
                cDACtoADCGraph->Fit("pol1");
                cDACtoADCGraph->GetFunction("pol1")->SetLineColor(cADCId + 2);

                // TF1* cFit = (TF1*)cDACtoADCGraph->GetListOfFunctions()->FindObject("pol1");
                TF1* cFit = cDACtoADCGraph->GetFunction("pol1");
                dieLegende->AddEntry(cDACtoADCGraph);
                dieLegende->AddEntry(cFit, Form("Fit ADC%i", cADCId), "lpf");
                // LOG(INFO) << BOLDBLUE << "Using ROOT for ADC " << cADCId << ": Parameter 1  " << cFit->GetParameter(0) << "  Parameter 2   " << cFit->GetParameter(1) << RESET;
                // LOG(INFO) << BOLDBLUE << "Using custom class for ADC " << cADCId << ": Parameter 1  " << cReg_Class.b_0 << "  Parameter 2   " << cReg_Class.b_1 << RESET;
                LOG(INFO) << BOLDBLUE << "Using custom class for ADC " << cADCId << ": Parameter 1  " << cReg_Class.b_0 << " +/- " << cReg_Class.b_0_error << "  Parameter 2   " << cReg_Class.b_1
                          << " +/- " << cReg_Class.b_1_error << RESET;
                LOG(INFO) << BOLDBLUE << "Using ROOT for ADC " << cADCId << ": Parameter 1  " << cFit->GetParameter(0) << " +/- " << cFit->GetParError(0) << "  Parameter 2   " << cFit->GetParameter(1)
                          << " +/- " << cFit->GetParError(1) << " Chi^2 " << cFit->GetChisquare() << " NDF " << cFit->GetNDF() << RESET;
                // ---Information also included in ROOT file of the fit
                fillSummaryTree(Form("ADC%i_p0", cADCId), cReg_Class.b_0);
                fillSummaryTree(Form("ADC%i_p1", cADCId), cReg_Class.b_1);
                fillSummaryTree(Form("ADC%i_p0_sigma", cADCId), cReg_Class.b_0_error);
                fillSummaryTree(Form("ADC%i_p1_sigma", cADCId), cReg_Class.b_1_error);
                fillSummaryTree(Form("ADC%i_chisquare", cADCId), cFit->GetChisquare());
                fillSummaryTree(Form("ADC%i_ndf", cADCId), cFit->GetNDF());
            }
            fillSummaryTree("VREFCNTR", cTrim);
            fResultFile->cd();
            // cDACtoADCTree->Write();
            cDACtoADCMultiGraph->Draw("AL*");
            cDACtoADCMultiGraph->GetXaxis()->SetTitle("DAC");
            cDACtoADCMultiGraph->GetYaxis()->SetTitle("ADC");
            dieLegende->Draw();
            // TLegend* dieLegende= cDACtoADCCanvas->BuildLegend();
            // dieLegende->AddEntry("pol1","Fit x","lpf");
            cDACtoADCCanvas->Write();
            // cDACtoADCMultiGraph->Write();
        }
    }
}

// Fixed in this context means: The ADC pin is not an AMUX pin
// Need statistics on spread of RSSI and temperature sensors
bool OTHybridTester::LpGBTTestFixedADCs()
{
    bool                                cReturn = true;
    std::map<std::string, std::string>  cADCsMap;
    std::map<std::string, float>*       cDefaultParameters = nullptr;
    std::map<std::string, std::string>* cADCNametoPinMapping;
    std::string                         cADCNameString;
    std::vector<int>                    cADCValueVect;

    auto cFixedADCsTree = new TTree("tFixedADCs", "lpGBT ADCs not tied to AMUX");
    cFixedADCsTree->Branch("Id", &cADCNameString);
    cFixedADCsTree->Branch("AdcValue", &cADCValueVect);
    gStyle->SetOptStat(0);

    if(fIsSEH)
    {
        cADCsMap             = {{"VMON_P1V25_L", "VMON_P1V25_L_Nominal"},
                                {"VMIN", "VMIN_Nominal"},
                                {"TEMPP", "TEMPP_Nominal"},
                                {"VTRX+_RSSI_ADC", "VTRX+_RSSI_ADC_Nominal"},
                                {"PTAT_BPOL2V5", "PTAT_BPOL2V5_Nominal"},
                                {"PTAT_BPOL12V", "PTAT_BPOL12V_Nominal"}};
        cDefaultParameters   = &f2SSEHDefaultParameters;
        cADCNametoPinMapping = &f2SSEHADCInputMap;

        fTC_2SSEH->set_P1V25_L_Sense(TC_2SSEH::P1V25SenseState::P1V25SenseState_On);
    }
    else
    {
        cADCsMap             = {{"12V_MONITOR_VD", "12V_MONITOR_VD_Nominal"},
                                {"TEMP", "TEMP_Nominal"},
                                {"VTRX+.RSSI_ADC", "VTRX+.RSSI_ADC_Nominal"},

                                {"1V25_MONITOR", "1V25_MONITOR_Nominal"},
                                {"2V55_MONITOR", "2V55_MONITOR_Nominal"}};
        cDefaultParameters   = &fPSROHDefaultParameters;
        cADCNametoPinMapping = &fPSROHADCInputMap;
    }

    auto cADCHistogram = new TH2I("hADCHistogram", "Fixed ADC Histogram", cADCsMap.size(), 0, cADCsMap.size(), 1024, 0, 1024);
    cADCHistogram->GetZaxis()->SetTitle("Number of entries");

    auto cADCsMapIterator = cADCsMap.begin();
    int  cADCValue;
    int  cBinCount = 1;
    fillSummaryTree("ADC conversion factor", CONVERSION_FACTOR);
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            D19clpGBTInterface* clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
            float               cGain           = clpGBTInterface->GetADCGain(cOpticalGroup->flpGBT, false);
            uint16_t            cOffset         = clpGBTInterface->GetADCOffset(cOpticalGroup->flpGBT, false);
            fillSummaryTree("ADC offset", cOffset);
            fillSummaryTree("ADC gain", cGain);

            if(fIsSEH) { fTC_2SSEH->set_AMUX(3500, 3500); }
            // else{
            // FIXME why is this here and what is cDACValue
            // flpGBTInterface->GetExternalController()->getInterface().dac_output(cDACValue);    }

            do {
                cADCValueVect.clear();
                cADCNameString = cADCsMapIterator->first;
                cADCHistogram->GetXaxis()->SetBinLabel(cBinCount, cADCsMapIterator->first.c_str());

                for(int cIteration = 0; cIteration < 10; ++cIteration)
                {
                    cADCValue = clpGBTInterface->ReadADC(cOpticalGroup->flpGBT, (*cADCNametoPinMapping)[cADCsMapIterator->first]);
                    // cADCValue-=34;
                    cADCValueVect.push_back(cADCValue);
                    cADCHistogram->Fill(cADCsMapIterator->first.c_str(), cADCValue, 1);
                    LOG(INFO) << BOLDBLUE << "Read " << cADCsMapIterator->first << " ADC Value 0x" << std::hex << +cADCValue << std::dec << RESET;
                }

                // float cTestValue = clpGBTInterface->GetADCVoltage(cOpticalGroup->flpGBT, "ADC5", true);
                // cTestValue       = clpGBTInterface->GetRssiPower(cOpticalGroup->flpGBT, "ADC5", 0.49, true);
                // LOG(INFO) << BOLDBLUE << "Read " << cTestValue << " ADC Value " << RESET;

                float sum           = std::accumulate(cADCValueVect.begin(), cADCValueVect.end(), 0.0);
                float mean          = sum / cADCValueVect.size();
                float result        = (mean - cOffset * (1 - cGain / 2.)) / cGain / 512.; // ADC master formula
                float cDifference_V = std::fabs((*cDefaultParameters)[cADCsMapIterator->second] - result);
                fillSummaryTree(cADCsMapIterator->first.c_str(), result);
                // Still hard coded threshold for imidiate boolean result, actual values are stored
                if(cDifference_V > fGradingThreshold)
                {
                    LOG(INFO) << BOLDRED << "Mismatch in fixed ADC channel " << cADCsMapIterator->first << " measured value is " << result << " V, nominal value is "
                              << (*cDefaultParameters)[cADCsMapIterator->second] << " V" << RESET;
                    cReturn = false;
                }
                else
                {
                    LOG(INFO) << BOLDGREEN << "Match in fixed ADC channel " << cADCsMapIterator->first << " measured value is " << result << " V, nominal value is "
                              << (*cDefaultParameters)[cADCsMapIterator->second] << " V" << RESET;
                }
                cFixedADCsTree->Fill();
                cADCsMapIterator++;
                cBinCount++;

            } while(cADCsMapIterator != cADCsMap.end());
        }
    }
    auto cADCCanvas = new TCanvas("cFixedADCs", "lpGBT ADCs not tied to AMUX", 1600, 900);
    cADCCanvas->SetRightMargin(0.2);
    cADCHistogram->GetXaxis()->SetTitle("ADC channel");
    cADCHistogram->GetYaxis()->SetTitle("ADC count");

    cADCHistogram->Draw("colz");
    cADCCanvas->Write();
    // cFixedADCsTree->Write();

    // flpGBTInterface->GetExternalController()->getInterface().set_P1V25_L_Sense(TC_2SSEH::P1V25SenseState::P1V25SenseState_Off);

    return cReturn;
}

void OTHybridTester::LpGBTSetGPIOLevel(const std::vector<uint8_t>& pGPIOs, uint8_t pLevel)
{
    D19clpGBTInterface* clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            // LOG(INFO) << BOLDBLUE << "Set levels to " << +pLevel << RESET;
            clpGBTInterface->ConfigureGPIODirection(cOpticalGroup->flpGBT, pGPIOs, 1);
            clpGBTInterface->ConfigureGPIOLevel(cOpticalGroup->flpGBT, pGPIOs, pLevel);
        }
    }
}

bool OTHybridTester::LpGBTTestResetLines()
{
    bool                                         cValid  = false;
    std::vector<std::pair<std::string, uint8_t>> cLevels = {{"High", 1}, {"Low", 0}};
    std::vector<uint8_t>                         cGPIOs;
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto __attribute__((unused)) cOpticalGroup: *cBoard) { cValid = true; }
    }
    if(!cValid)
    {
        LOG(INFO) << BOLDYELLOW << "OTHybridTester::LpGBTTestResetLines Stopping test. No OpticalGroup enabled!" << RESET;
        return cValid;
    }
    // lpGBTinterface now knows this .. so don't need the if statements

    if(fIsSEH) { cGPIOs = static_cast<D19clpGBTInterface*>(flpGBTInterface)->get2SResetGPIOs(); }
    else { cGPIOs = static_cast<D19clpGBTInterface*>(flpGBTInterface)->getPSResetGPIOs(); }

    LpGBTSetGPIOLevel(cGPIOs, 1);

    float cMeasurement;
    for(auto cLevel: cLevels)
    {
        LpGBTSetGPIOLevel(cGPIOs, cLevel.second);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if(fIsSEH)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            // mu-controller is too slow

            auto cMapIterator = f2SSEHResetLines.begin();

            bool cStatus = true;
            do {
                float cDifference_mV = 0;
                fTC_2SSEH->read_reset(cMapIterator->second, cMeasurement);
                cDifference_mV = std::fabs((cLevel.second * fNominalOutputbpol2v5) - cMeasurement) * 1000.; // 1300
                fillSummaryTree(cMapIterator->first.c_str() + cLevel.first + "_value", cMeasurement);
                cStatus = cStatus && (cDifference_mV <= fGradingThreshold * 1000);

                cValid = cValid && cStatus;
                // cLineNames.push_back(cMapIterator->first.c_str() + cLevel.first);
                // cValues.push_back(cMeasurement);
                if(cDifference_mV > fGradingThreshold * 1000)
                {
                    LOG(INFO) << BOLDRED << "Mismatch in GPIO connected to " << cMapIterator->first << RESET;
                    fillSummaryTree(cMapIterator->first.c_str() + cLevel.first, 0);
                }
                else
                {
                    LOG(INFO) << BOLDGREEN << "Match in GPIO connected to " << cMapIterator->first << RESET;
                    fillSummaryTree(cMapIterator->first.c_str() + cLevel.first, 1);
                }
                cMapIterator++;
            } while(cMapIterator != f2SSEHResetLines.end());
            if(cStatus)
                LOG(INFO) << BOLDBLUE << "Set levels to " << cLevel.first << " : test " << BOLDGREEN << " passed." << RESET;
            else
                LOG(INFO) << BOLDRED << "Set levels to " << cLevel.first << " : test " << BOLDRED << " failed." << RESET;
        }
        else
        {
            auto cMapIterator = fResetLines.begin();

            bool cStatus = true;
            do {
                float cDifference_mV = 0;
                fTC_PSROH->adc_get(cMapIterator->second, cMeasurement);
                cDifference_mV = std::fabs((cLevel.second * fNominalOutputbpol2v5) - cMeasurement / 1000.) * 1000.; // 1300
                fillSummaryTree(cMapIterator->first.c_str() + cLevel.first + "_value", cMeasurement);
                cStatus = cStatus && (cDifference_mV <= fGradingThreshold * 1000);

                cValid = cValid && cStatus;
                // cLineNames.push_back(cMapIterator->first.c_str() + cLevel.first);
                // cValues.push_back(cMeasurement);
                if(cDifference_mV > fGradingThreshold * 1000)
                {
                    LOG(INFO) << BOLDRED << "Mismatch in GPIO connected to " << cMapIterator->first << RESET;
                    fillSummaryTree(cMapIterator->first.c_str() + cLevel.first, 0);
                }
                else
                {
                    LOG(INFO) << BOLDGREEN << "Match in GPIO connected to " << cMapIterator->first << RESET;
                    fillSummaryTree(cMapIterator->first.c_str() + cLevel.first, 1);
                }
                cMapIterator++;
            } while(cMapIterator != fResetLines.end());
            if(cStatus)
                LOG(INFO) << BOLDBLUE << "Set levels to " << cLevel.first << " : test " << BOLDGREEN << " passed." << RESET;
            else
                LOG(INFO) << BOLDRED << "Set levels to " << cLevel.first << " : test " << BOLDRED << " failed." << RESET;
        }
    }
    return cValid;
}

bool OTHybridTester::LpGBTTestGPILines()
{
    std::map<std::string, uint8_t> fGPILines;

    if(fIsSEH) { fGPILines = f2SSEHGPILines; }
    else { fGPILines = fPSROHGPILines; }
    // On the TC the PWRGOOD is connected to a switch!
    bool                cValid = true;
    bool                cReadGPI;
    auto                cMapIterator    = fGPILines.begin();
    D19clpGBTInterface* clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            while(cMapIterator != fGPILines.end())
            {
                cReadGPI = clpGBTInterface->ReadGPIO(cOpticalGroup->flpGBT, cMapIterator->second);
                cValid   = cValid && cReadGPI;
                if(!cReadGPI) { LOG(INFO) << BOLDRED << "GPIO connected to " << cMapIterator->first << " is low!" << RESET; }
                else { LOG(INFO) << BOLDGREEN << "GPIO connected to " << cMapIterator->first << " is high!" << RESET; }
                fillSummaryTree(cMapIterator->first.c_str(), cReadGPI);
                cMapIterator++;
            }
        }
    }
    return cValid;
}

bool OTHybridTester::LpGBTTestVTRx()
{
    bool cSuccess = false;
    bool cRecent;
    bool cReset = true;
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto __attribute__((unused)) cOpticalGroup: *cBoard) { cSuccess = true; }
    }
    if(!cSuccess)
    {
        LOG(INFO) << BOLDYELLOW << "OTHybridTester::LpGBTTestVTRx Stopping test. No OpticalGroup enabled!" << RESET;
        return cSuccess;
    }
    D19clpGBTInterface* clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
    for(auto cBoard: *fDetectorContainer)
    {
        D19cFWInterface*      pInterface        = static_cast<D19cFWInterface*>(fBeBoardFWMap.find(cBoard->getId())->second);
        D19cOpticalInterface* cOpticalInterface = static_cast<D19cOpticalInterface*>(pInterface->getFEConfigurationInterface());
        for(auto cOpticalGroup: *cBoard)
        {
            auto clpGBT = cOpticalGroup->flpGBT;
            clpGBTInterface->ResetI2C(clpGBT, {0, 1, 2});
            std::this_thread::sleep_for(std::chrono::milliseconds(30));

            // Configuring I2C Master pull-ups
            clpGBTInterface->WriteChipReg(clpGBT, "I2CM1Config", 1 << 3 | 1 << 4 | 1 << 5 | 1 << 6);

            LpGBTSetGPIOLevel({static_cast<D19clpGBTInterface*>(flpGBTInterface)->getVtrxResetGPIO()}, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            LpGBTSetGPIOLevel({static_cast<D19clpGBTInterface*>(flpGBTInterface)->getVtrxResetGPIO()}, 1);

            uint8_t cMasterId = 1, cSlaveAddress = 0x50, cSlaveData = 0x15, cNbyte = 1, cFrequency = 2;
            uint8_t cMasterConfig = (cNbyte << 2) | (cFrequency << 0);
            cRecent               = cOpticalInterface->SingleMultiByteWriteI2C(clpGBT, cMasterId, cMasterConfig, cSlaveAddress, cSlaveData);
            for(int i = 0; i < 5 && !(cRecent); i++) { cRecent = cOpticalInterface->SingleMultiByteWriteI2C(clpGBT, cMasterId, cMasterConfig, cSlaveAddress, cSlaveData); }
            auto                       cReadBackValue            = cOpticalInterface->SingleSingleByteReadI2C(clpGBT, cMasterId, cMasterConfig, cSlaveAddress);
            std::map<uint8_t, uint8_t> cVTRxplusDefaultRegisters = fVTRxplusDefaultRegisters;
            if(cReadBackValue == 0x15)
            {
                cVTRxplusDefaultRegisters = fVTRxplusDefaultRegistersV13;
                LOG(INFO) << BOLDGREEN << "VTRx+ register map for version 1.3 is used!" << RESET;
                fillSummaryTree("vtrxplusversion", 1.3);
                uint32_t cChipId = 0;
                for(int i = 0; i < 4; i++)
                {
                    cRecent        = cOpticalInterface->SingleMultiByteWriteI2C(clpGBT, cMasterId, cMasterConfig, cSlaveAddress, i + 0x16);
                    cReadBackValue = cOpticalInterface->SingleSingleByteReadI2C(clpGBT, cMasterId, cMasterConfig, cSlaveAddress);
                    cChipId        = cChipId | cReadBackValue << (i * 8);
                    LOG(INFO) << BOLDGREEN << "VTRx+ register 0x" << std::hex << +i + 0x16 << std::dec << " contains the value 0x" << std::hex << +cReadBackValue << std::dec << " ." << RESET;
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                LOG(INFO) << BOLDYELLOW << "vtrx+ ID: 0x" << std::hex << +cChipId << std::dec << RESET;
                fillSummaryTree("vtrxplus_id", cChipId);
            }
            else
            {
                LOG(INFO) << BOLDGREEN << "VTRx+ register map for version 1.2 is used!" << RESET;
                fillSummaryTree("vtrxplusversion", 1.2);
                fillSummaryTree("vtrxplus_id", 0);
                LOG(INFO) << BOLDYELLOW << "no VTRx+ ID" << RESET;
            }

            auto cMapIterator = cVTRxplusDefaultRegisters.begin();
            do {
                cRecent        = cOpticalInterface->SingleMultiByteWriteI2C(clpGBT, cMasterId, cMasterConfig, cSlaveAddress, cMapIterator->first);
                cReadBackValue = cOpticalInterface->SingleSingleByteReadI2C(clpGBT, cMasterId, cMasterConfig, cSlaveAddress);
                cSuccess       = cSuccess && cRecent && (cReadBackValue == cMapIterator->second);
                if(cRecent && (cReadBackValue == cMapIterator->second))
                {
                    LOG(INFO) << BOLDGREEN << "VTRx+ register " << +(cMapIterator->first) << " contains the default value " << +cReadBackValue << " ." << RESET;
                }
                else
                {
                    LOG(INFO) << BOLDRED << "Error in VTRx+ register " << +(cMapIterator->first) << " ." << RESET;
                    LOG(INFO) << BOLDRED << "value " << +(cReadBackValue) << " ." << RESET;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                cMapIterator++;
            } while(cMapIterator != cVTRxplusDefaultRegisters.end());
            cRecent        = cOpticalInterface->SingleMultiByteWriteI2C(clpGBT, cMasterId, cMasterConfig, cSlaveAddress, 0x0d);
            cReadBackValue = cOpticalInterface->SingleSingleByteReadI2C(clpGBT, cMasterId, cMasterConfig, cSlaveAddress);
            if(cReadBackValue == 0xA0)
            {
                cNbyte         = 2;
                cMasterConfig  = (cNbyte << 2) | (cFrequency << 0);
                cRecent        = cOpticalInterface->SingleMultiByteWriteI2C(clpGBT, cMasterId, cMasterConfig, cSlaveAddress, 0xac0d);
                cNbyte         = 1;
                cMasterConfig  = (cNbyte << 2) | (cFrequency << 0);
                cRecent        = cOpticalInterface->SingleMultiByteWriteI2C(clpGBT, cMasterId, cMasterConfig, cSlaveAddress, 0x0d);
                cReadBackValue = cOpticalInterface->SingleSingleByteReadI2C(clpGBT, cMasterId, cMasterConfig, cSlaveAddress);
                if(cReadBackValue == 0xAC)
                {
                    LpGBTSetGPIOLevel({static_cast<D19clpGBTInterface*>(flpGBTInterface)->getVtrxResetGPIO()}, 0);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    LpGBTSetGPIOLevel({static_cast<D19clpGBTInterface*>(flpGBTInterface)->getVtrxResetGPIO()}, 1);
                    cRecent        = cOpticalInterface->SingleMultiByteWriteI2C(clpGBT, cMasterId, cMasterConfig, cSlaveAddress, 0x0d);
                    cReadBackValue = cOpticalInterface->SingleSingleByteReadI2C(clpGBT, cMasterId, cMasterConfig, cSlaveAddress);
                    if(cReadBackValue == 0xA0)
                    {
                        LOG(INFO) << BOLDGREEN << "Passed VTRx+ reset, read back 0xA0" << RESET;
                        cReset = true;
                    }
                    else
                    {
                        LOG(INFO) << BOLDRED << "Failed to reset VTRx+, read back 0x" << std::hex << +cReadBackValue << std::dec << RESET;
                        cReset = false;
                    }
                }
                else
                {
                    LOG(INFO) << BOLDRED << "Failed to write 0xAC to VTRx+ register 0x0D for reset test, read back 0x" << std::hex << +cReadBackValue << std::dec << RESET;
                    cReset = false;
                }
            }
            else
            {
                LOG(INFO) << BOLDRED << "Wrong starting value in register 0x0D for VTRx+ reset test (0x" << std::hex << +cReadBackValue << std::dec << ")" << RESET;
                cReset = false;
            }
        }
    }
    fillSummaryTree("vtrxplus_register", cSuccess);
    fillSummaryTree("vtrxplus_reset", cReset);
    fillSummaryTree("vtrxplusslowcontrol", cSuccess & cReset);
    return cSuccess & cReset;
}

bool OTHybridTester::LpGBTGetLinkLock()
{
    bool cStatus = false;
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            D19cFWInterface*   cFWInterface   = dynamic_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard));
            D19cLinkInterface* cLinkInterface = static_cast<D19cLinkInterface*>(cFWInterface->getLinkInterface());
            cStatus                           = cLinkInterface->GetLinkStatus(cOpticalGroup->getId());
        }
    }
    return cStatus;
}

bool OTHybridTester::LpGBTFastCommandChecker(uint8_t pPattern)
{
    uint8_t  cMatch;
    uint8_t  cShift;
    uint8_t  cWrappedByte;
    uint32_t cWrappedData;
    bool     res = false;
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto __attribute__((unused)) cOpticalGroup: *cBoard) { res = true; }
    }
    if(!res)
    {
        LOG(INFO) << BOLDYELLOW << "OTHybridTester::LpGBTFastCommandChecker Stopping test. No OpticalGroup enabled!" << RESET;
        return res;
    }
    D19clpGBTInterface*         clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
    const std::vector<uint8_t>& cPatternVec     = {0x07, 0x00, 0xff, 0xaa, 0xcc, 0xca};
    for(const auto cPattern: cPatternVec)
    {
        for(auto cBoard: *fDetectorContainer)
        {
            for(auto cOpticalGroup: *cBoard)
            {
                if(cPattern != 0x07)
                {
                    clpGBTInterface->ConfigureDPPattern(cOpticalGroup->flpGBT, cPattern << 24 | cPattern << 16 | cPattern << 8 | cPattern);
                    for(const auto& TxProperty: static_cast<lpGBT*>(cOpticalGroup->flpGBT)->getTxProperties())
                    {
                        clpGBTInterface->ConfigureTxSource(cOpticalGroup->flpGBT, TxProperty.Group, 3); // 0 --> link data, 3 --> constant pattern   }
                    }
                }
            }
        }

        for(auto cBoard: *fDetectorContainer)
        {
            bool isElectricalFc7 = true;
            for(auto cOpticalGroup: *cBoard)
            {
                std::ignore     = cOpticalGroup;
                isElectricalFc7 = false;
                break;
            }
            if(isElectricalFc7)
            {
                std::map<std::string, std::string> fFCMDLines;

                if(fIsSEH) { fFCMDLines = f2SSEHFCMDLines; }
                else { fFCMDLines = fPSROHFCMDLines; }

                auto cMapIterator = fFCMDLines.begin();
                LOG(INFO) << BOLDBLUE << "Checking against : " << std::bitset<8>(cPattern) << RESET;
                do {
                    uint32_t cFCMDOutput = fBeBoardInterface->ReadBoardReg(cBoard, cMapIterator->second);
                    LOG(INFO) << BOLDBLUE << "Scoped output on " << cMapIterator->first << ": " << std::bitset<32>(cFCMDOutput) << RESET;

                    cMatch = 32;
                    cShift = 0;
                    for(uint8_t shift = 0; shift < 8; shift++)
                    {
                        cWrappedByte = (cPattern >> shift) | (cPattern << (8 - shift));
                        cWrappedData = (cWrappedByte << 24) | (cWrappedByte << 16) | (cWrappedByte << 8) | (cWrappedByte << 0);
                        LOG(DEBUG) << BOLDBLUE << std::bitset<8>(cWrappedByte) << RESET;
                        LOG(DEBUG) << BOLDBLUE << std::bitset<32>(cWrappedData) << RESET;
                        int popcount = __builtin_popcountll(cWrappedData ^ cFCMDOutput);
                        if(popcount < cMatch)
                        {
                            cMatch = popcount;
                            cShift = shift;
                        }
                        LOG(DEBUG) << BOLDBLUE << "Line " << cMapIterator->first << " Shift " << +shift << " Match " << +popcount << RESET;
                    }
                    LOG(INFO) << BOLDBLUE << "Found for " << cMapIterator->first << " a minimal bit difference of " << +cMatch << " for a bit shift of " << +cShift << RESET;
                    LOG(INFO) << Form("_miss_match_0x%02X", cPattern) << RESET;

                    fillSummaryTree(cMapIterator->first + Form("_miss_match_0x%02X", cPattern), cMatch);
                    fillSummaryTree(cMapIterator->first + Form("_miss_shift_0x%02X", cPattern), cShift);

                    if((cMatch == 0)) { LOG(INFO) << BOLDGREEN << "FCMD Test passed for " << cMapIterator->first << RESET; }
                    else
                    {
                        LOG(INFO) << BOLDRED << "FCMD Test failed for " << cMapIterator->first << RESET;
                        res &= false;
                    }
                    cMapIterator++;
                } while(cMapIterator != fFCMDLines.end());
            }
        }
    }
    return res;
}

void OTHybridTester::LpGBTRunEyeOpeningMonitor(uint8_t pEndOfCountSelect, uint8_t pEQAttenuation)
{
    D19clpGBTInterface* clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            LOG(INFO) << MAGENTA << "VDDRX read value = " << +clpGBTInterface->ReadADC(cOpticalGroup->flpGBT, "VDDRX") << RESET;
            // uint8_t cEQConfig = (clpGBTInterface->ReadChipReg(cOpticalGroup->flpGBT, "EQConfig") & ~(0x2 << 3)) | (pEQAttenuation << 3);
            // FIXME for now I am forcing to 0x00 EQCap bits of the register
            clpGBTInterface->WriteChipReg(cOpticalGroup->flpGBT, "EQConfig", pEQAttenuation << 3);
            LOG(INFO) << MAGENTA << "EQConfig set to : 0x" << std::hex << +clpGBTInterface->ReadChipReg(cOpticalGroup->flpGBT, "EQConfig") << std::dec << RESET;
            // ROOT Tree for Eye Diagram from lpGBT Eye Opening Monitor
            auto cEyeDiagramTree = new TTree(Form("tEyeDiagram%i", cOpticalGroup->getOpticalGroupId()), "Eye Diagram form lpGBT Eye Opening Monitor");
            // vectors for Tree
            std::vector<int> cVoltageVector;
            std::vector<int> cTimeVector;
            std::vector<int> cCounterVector;
            // TBranches
            cEyeDiagramTree->Branch("VoltageStep", &cVoltageVector);
            cEyeDiagramTree->Branch("TimeStep", &cTimeVector);
            cEyeDiagramTree->Branch("Counter", &cCounterVector);
            // Create TCanvas & TH2I
            auto cEyeDiagramCanvas = new TCanvas(Form("cEyeDiagram%i", cOpticalGroup->getOpticalGroupId()), "Eye Opening Image", 500, 500);
            auto cObj              = gROOT->FindObject(Form("hEyeDiagram%i", cOpticalGroup->getOpticalGroupId()));
            if(cObj) delete cObj;
            auto cEyeDiagramHist = new TH2I(Form("hEyeDiagram%i", cOpticalGroup->getOpticalGroupId()), "Eye Opening Image", 64, 0, 63, 32, 0, 31);
            clpGBTInterface->ConfigureEOM(cOpticalGroup->flpGBT, pEndOfCountSelect, false, true);
            for(uint8_t cVoltageStep = 0; cVoltageStep < 31; cVoltageStep++)
            {
                LOG(INFO) << YELLOW << "voltage step " << +cVoltageStep << RESET;
                clpGBTInterface->SelectEOMVof(cOpticalGroup->flpGBT, cVoltageStep);
                for(uint8_t cTimeStep = 0; cTimeStep < 64; cTimeStep++)
                {
                    clpGBTInterface->SelectEOMPhase(cOpticalGroup->flpGBT, cTimeStep);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    clpGBTInterface->StartEOM(cOpticalGroup->flpGBT, true);
                    uint8_t cEOMStatus = clpGBTInterface->GetEOMStatus(cOpticalGroup->flpGBT);
                    while((cEOMStatus & (0x1 << 1) >> 1) && !(cEOMStatus & (0x1 << 0))) { cEOMStatus = clpGBTInterface->GetEOMStatus(cOpticalGroup->flpGBT); }
                    uint16_t cCounterValue    = clpGBTInterface->GetEOMCounter(cOpticalGroup->flpGBT);
                    uint16_t c40MCounterValue = clpGBTInterface->ReadChipReg(cOpticalGroup->flpGBT, "EOMCounter40MH") << 8 | clpGBTInterface->ReadChipReg(cOpticalGroup->flpGBT, "EOMCounter40ML");
                    LOG(DEBUG) << YELLOW << "\t time step " << +cTimeStep << ", counter value " << +cCounterValue << ", 40M counter " << +c40MCounterValue << RESET;
                    clpGBTInterface->StartEOM(cOpticalGroup->flpGBT, false);
                    cVoltageVector.push_back(cVoltageStep * 40); // 40 mV step
                    cTimeVector.push_back(cTimeStep * 6.1);      // 6.1 ps step
                    cCounterVector.push_back(cCounterValue);
                    // ROOT related filling
                    cEyeDiagramHist->Fill(cTimeStep, cVoltageStep, cCounterValue);
                    cEyeDiagramTree->Fill();
                }
            }
            cEyeDiagramHist->SetTitle(Form("Eye Opening Diagram - EQAttenuation = %i", pEQAttenuation));
            cEyeDiagramHist->GetXaxis()->SetTitle("Time [ps]");
            cEyeDiagramHist->GetYaxis()->SetTitle("Vof [mV]");
            fResultFile->cd();
            // cEyeDiagramTree->Write();
            cEyeDiagramHist->Write();
            cEyeDiagramCanvas->cd();
            cEyeDiagramHist->Draw("COLZ");
        }
    }
}

void OTHybridTester::LpGBTRunBitErrorRateTest(uint8_t pCoarseSource, uint8_t pFineSource, uint8_t pMeasTime, uint32_t pPattern)
{
    D19clpGBTInterface* clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
    if(pPattern != 0x00000000)
    {
        LOG(INFO) << BOLDMAGENTA << "Performing BER Test with constant pattern 0x" << std::hex << +pPattern << std::dec << RESET;
        LpGBTInjectULExternalPattern(true, pPattern & 0xFF);
    }
    // Run Bit Error Rate Test
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            // Configure BERT Pattern for comparision
            if(pPattern != 0x00000000) { clpGBTInterface->ConfigureBERTPattern(cOpticalGroup->flpGBT, pPattern); }
            else
            {
                LOG(INFO) << BOLDMAGENTA << "Performing BER Test with PRBS7" << RESET;
                for(const auto& RxProperty: static_cast<lpGBT*>(cOpticalGroup->flpGBT)->getRxProperties())
                {
                    clpGBTInterface->ConfigureRxPRBS(cOpticalGroup->flpGBT, RxProperty.Group, RxProperty.Channel, true);
                }
            }
            // Configure BERT block
            clpGBTInterface->ConfigureBERT(cOpticalGroup->flpGBT, pCoarseSource, pFineSource, pMeasTime);
            uint8_t cRxTerm = 1, cRxAcBias = 0, cRxInvert = 1;
            for(uint8_t cRxEqual = 0; cRxEqual < 4; cRxEqual++)
            {
                for(uint16_t cRxPhase = 0; cRxPhase < 16; cRxPhase++)
                {
                    clpGBTInterface->ConfigureRxChannel(cOpticalGroup->flpGBT, 0, 0, cRxEqual, cRxTerm, cRxAcBias, cRxInvert, cRxPhase);
                    // Run BERT and get result (fraction of errors)
                    float cBERTResult = 100 * clpGBTInterface->GetBERTResult(cOpticalGroup->flpGBT);
                    LOG(INFO) << BOLDWHITE << "\tBit Error Rate [RxEqual=" << +cRxEqual << ":RxPhase=" << +cRxPhase << "] = " << +cBERTResult << "%" << RESET;
                }
            }
            if(pPattern == 0x00000000)
            {
                for(const auto& RxProperty: static_cast<lpGBT*>(cOpticalGroup->flpGBT)->getRxProperties())
                {
                    clpGBTInterface->ConfigureRxPRBS(cOpticalGroup->flpGBT, RxProperty.Group, RxProperty.Channel, false);
                }
            }
        }
    }
}

bool OTHybridTester::LpGBTCheckClocks()
{
    bool                     cStatus         = false;
    uint8_t                  cChipRate       = 0;
    std::vector<std::string> cClockTestTypes = {"_default", "_short", "_open"};

    for(auto cBoard: *fDetectorContainer)
    {
        for(auto __attribute__((unused)) cOpticalGroup: *cBoard) { cStatus = true; }
    }
    if(!cStatus)
    {
        LOG(INFO) << BOLDYELLOW << "OTHybridTester::LpGBTCheckClocks Stopping test. No OpticalGroup enabled!" << RESET;
        return cStatus;
    }

    for(const auto& cClockTestType: cClockTestTypes)
    {
        for(auto cBoard: *fDetectorContainer)
        {
            bool isElectricalFc7 = true;

            D19clpGBTInterface* clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
            for(auto cOpticalGroup: *cBoard)
            {
                cChipRate = clpGBTInterface->GetChipRate(cOpticalGroup->flpGBT);
                if(cClockTestType == "_short" || cClockTestType == "_open")
                {
                    lpGBTClockConfig cClkCnfg;
                    cClkCnfg.fClkFreq         = 1;
                    cClkCnfg.fClkDriveStr     = 1;
                    cClkCnfg.fClkPreEmphWidth = (cClockTestType == "_short") ? 0 : 7; // 7
                    cClkCnfg.fClkPreEmphMode  = (cClockTestType == "_short") ? 0 : 2; // 2;
                    cClkCnfg.fClkPreEmphStr   = (cClockTestType == "_short") ? 0 : 7; // 7;

                    cClkCnfg.fClkInvert = 1;
                    LOG(INFO) << BOLDBLUE << "Enabling clock with fClkFreq " << +cClkCnfg.fClkFreq << " fClkDriveStr " << +cClkCnfg.fClkDriveStr << " fClkPreEmphStr " << +cClkCnfg.fClkPreEmphStr
                              << " fClkPreEmphWidth " << +cClkCnfg.fClkPreEmphWidth << " fClkPreEmphMode " << +cClkCnfg.fClkPreEmphMode << RESET;
                    clpGBTInterface->hybridClock(cOpticalGroup->flpGBT, cClkCnfg, 0);
                    clpGBTInterface->hybridClock(cOpticalGroup->flpGBT, cClkCnfg, 1);
                    cClkCnfg.fClkInvert = 0;
                    LOG(INFO) << BOLDBLUE << "Enabling CIC clocks" << RESET;
                    clpGBTInterface->cicClock(cOpticalGroup->flpGBT, cClkCnfg, 0);
                    clpGBTInterface->cicClock(cOpticalGroup->flpGBT, cClkCnfg, 1);
                }
                if(cOpticalGroup->flpGBT != nullptr)
                {
                    isElectricalFc7 = false;
                    break;
                }
            }
            if(isElectricalFc7)
            {
                // clk test
                fBeBoardInterface->WriteBoardReg(cBoard, "fc7_daq_ctrl.physical_interface_block.multiplexing_bp.check_return_clock", 0x1);
                fBeBoardInterface->WriteBoardReg(cBoard, "fc7_daq_ctrl.physical_interface_block.multiplexing_bp.check_return_clock", 0x0);
                std::map<std::string, std::string> cClockMap;

                if(fIsSEH) { cClockMap = f2SSEHClockMap; }
                else { cClockMap = fPSROHClockMap; }

                auto cMapIterator = cClockMap.begin();
                bool cClkTestDone = false;
                bool cClkStat     = false;

                LOG(INFO) << GREEN << "============================" << RESET;
                LOG(INFO) << BOLDGREEN << "Clock test" << RESET;

                do {
                    cClkTestDone = (fBeBoardInterface->ReadBoardReg(cBoard, cMapIterator->second + "_test_done") == 1);
                    while(!cClkTestDone)
                    {
                        LOG(INFO) << "Waiting for clock test" << RESET;
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        cClkTestDone = (fBeBoardInterface->ReadBoardReg(cBoard, cMapIterator->second + "_test_done") == 1);
                    }
                    if(cClkTestDone)
                    {
                        std::string cRegName        = "";
                        uint16_t    cClkTestCounter = 0, cClkRefCounter = 0;
                        if(cMapIterator->first == "320_l_Clk_Test")
                        {
                            cClkTestCounter = fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_stat.physical_interface_block.clk_test_debug_1.fe_for_ps_roh_clk_320_l_test_counter");
                            cClkRefCounter  = fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_stat.physical_interface_block.clk_test_debug_1.fe_for_ps_roh_clk_320_l_ref_counter");
                        }
                        else if(cMapIterator->first == "320_r_Clk_Test")
                        {
                            cClkTestCounter = fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_stat.physical_interface_block.clk_test_debug_2.fe_for_ps_roh_clk_320_r_test_counter");
                            cClkRefCounter  = fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_stat.physical_interface_block.clk_test_debug_2.fe_for_ps_roh_clk_320_r_ref_counter");
                        }
                        else if(cMapIterator->first == "640_l_Clk_Test")
                        {
                            cClkTestCounter = fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_stat.physical_interface_block.clk_test_debug_3.fe_for_ps_roh_clk_640_l_test_counter");
                            cClkRefCounter  = fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_stat.physical_interface_block.clk_test_debug_3.fe_for_ps_roh_clk_640_l_ref_counter");
                        }
                        else if(cMapIterator->first == "640_r_Clk_Test")
                        {
                            cClkTestCounter = fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_stat.physical_interface_block.clk_test_debug_4.fe_for_ps_roh_clk_640_r_test_counter");
                            cClkRefCounter  = fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_stat.physical_interface_block.clk_test_debug_4.fe_for_ps_roh_clk_640_r_ref_counter");
                        }

                        LOG(INFO) << "\t Test Counter = " << +cClkTestCounter << " --- Ref Counter = " << +cClkRefCounter << RESET;
                        // Ignored until firmware is fixed
                        cClkStat = fBeBoardInterface->ReadBoardReg(cBoard, cMapIterator->second + "_stat");
                        if(cClkStat)
                            LOG(INFO) << cMapIterator->first << " test ->" << BOLDGREEN << " PASSED (firmware)" << RESET;
                        else
                        {
                            LOG(INFO) << cMapIterator->first << " test ->" << BOLDRED << " FAILED (firmware)" << RESET;
                            // cStatus &= false;
                        }
                        // HOTFIX
                        if(cClockTestType == "_short" || cClockTestType == "_open")
                        {
                            float cDivider = (cChipRate == 5) ? 8 : 16;
                            cClkStat       = (abs(cClkTestCounter - cClkRefCounter / cDivider) < 5);
                        }
                        else { cClkStat = (abs(cClkTestCounter - cClkRefCounter) < 5); }
                        if(cClkStat)
                            LOG(INFO) << cMapIterator->first << " test ->" << BOLDGREEN << " PASSED (counter)" << RESET;
                        else
                        {
                            LOG(INFO) << cMapIterator->first << " test ->" << BOLDRED << " FAILED (counter)" << RESET;
                            cStatus &= false;
                        }
                        fillSummaryTree(cMapIterator->first + cClockTestType, cClkStat);
                    }
                    cMapIterator++;
                } while(cMapIterator != cClockMap.end());

                LOG(INFO) << GREEN << "============================" << RESET;
            }
        }
    }
    return cStatus;
}

std::pair<bool, uint8_t> OTHybridTester::PhaseTuneLineEleFC7(uint8_t pHybrid, uint8_t pLineId)
{
    std::pair<bool, uint8_t> cLineStatus;
    cLineStatus.first  = false;
    cLineStatus.second = 0;
    uint8_t pChip      = 0;
    auto    cBoardId   = 1;
    auto    cBoardIter = std::find_if(fDetectorContainer->begin(), fDetectorContainer->end(), [&cBoardId](Ph2_HwDescription::BeBoard* x) { return x->getId() == cBoardId; });
    LOG(DEBUG) << BOLDYELLOW << "OTHybridTester::PhaseTuneLineEleFC7#" << +pLineId << " for a Chip#" << +pChip << RESET;
    auto cInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(fDetectorContainer->getObject(cBoardId)));

    D19cBackendAlignmentFWInterface* cAlignerInterface   = cInterface->getBackendAlignmentInterface();
    auto                             theAlignmentResults = cAlignerInterface->tunePhase(pHybrid, pLineId);
    cLineStatus.first                                    = theAlignmentResults.fPhaseAlignmentSuccess;
    if(!cLineStatus.first)
    {
        LOG(INFO) << BOLDRED << "Could not phase align-BE data for BeBoard#" << +cBoardId << " Hybrid#" << +pHybrid << " Chip#" << +pChip << " line# " << +pLineId << RESET;
        // throw std::runtime_error(std::string("Could not phase align-BE data in LinkAlignmentOT..."));
    }
    else { LOG(INFO) << BOLDBLUE << "Could phase align-BE data for BeBoard#" << +cBoardId << " Hybrid#" << +pHybrid << " Chip#" << +pChip << " line# " << +pLineId << RESET; }

    cLineStatus.second = theAlignmentResults.fDelay;
    return cLineStatus;
}

uint16_t OTHybridTester::calibrateADC()
{
    uint8_t     cTrimOptimized   = 0;
    float       cTestCard1V25    = 0;
    float       cTestCardVDD2V55 = 0;
    float       cVref            = 0;
    float       cGain            = 0;
    uint16_t    cOffset          = 0;
    uint16_t    cADC1V25         = 0;
    uint8_t     cVersion         = 0;
    std::string registerStr      = "VREFTUNE";

    // Create TTree for DAC to ADC conversion in lpGBT
    auto cCalibrationTree  = new TTree("tADCCalibration", "Calibration of the ADC");
    auto cCalibrationGraph = new TGraph();
    // Create variables for TTree branches

    std::vector<uint16_t> cVREFTuneVect;
    std::vector<uint16_t> cADCValVect;
    std::vector<uint16_t> cOffsetVect;
    std::vector<float>    cGainVect;
    std::vector<float>    cVrefVect;
    // Create TTree Branches
    cCalibrationTree->Branch("VREFTune", &cVREFTuneVect);
    cCalibrationTree->Branch("ADCValue", &cADCValVect);
    cCalibrationTree->Branch("Offset", &cOffsetVect);
    cCalibrationTree->Branch("Gain", &cGainVect);
    cCalibrationTree->Branch("Vref", &cVrefVect);
    if(fIsSEH)
    {
        fTC_2SSEH->read_supply(TC_2SSEH::supplyMeasurement::U_P1V25, cTestCard1V25);
        fTC_2SSEH->set_P1V25_L_Sense(TC_2SSEH::P1V25SenseState::P1V25SenseState_On);
        fTC_2SSEH->set_AMUX(3303, 3303);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    else { fTC_PSROH->adc_get(TC_PSROH::measurement::_2V55, cTestCardVDD2V55); }
    D19clpGBTInterface* clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            cGain         = clpGBTInterface->GetADCGain(cOpticalGroup->flpGBT, true);
            cOffset       = clpGBTInterface->GetADCOffset(cOpticalGroup->flpGBT, true);
            cVersion      = static_cast<lpGBT*>(cOpticalGroup->flpGBT)->getVersion();
            uint8_t nBits = 8;
            uint8_t cTrim = 0;
            if(cVersion == 0)
            {
                registerStr = "VREFCNTR";
                nBits       = 6;
            }
            /* for(uint8_t cBit = 0; cBit < nBits; cBit += 1)
            {
                cTrim = (1 << (nBits - cBit - 1)) | cTrimOptimized;
                clpGBTInterface->WriteChipReg(cOpticalGroup->flpGBT, registerStr, cTrim);
                cADC1V25 = clpGBTInterface->ReadADC(cOpticalGroup->flpGBT, "ADC1", "VREF/2");
                cVref    = cTestCard1V25 * 200. / 310. * cGain * 512 / (cADC1V25 - cOffset * (1 - cGain / 2.));
                // std::cout << std::dec << trim << "," << cOffset << "," << cGain << "," << cADC1V25 << "," << cTestCard1V25 << "," << cVref << std::endl;
                LOG(INFO) << BOLDBLUE << "OTHybridTester::calibrateADC(): VREFTune: 0x" << std::hex << +cTrim << std::dec << " ADC value: 0x" << std::hex << +cADC1V25 << std::dec
                          << " calculated Vref: " << cVref << "V offset: 0x" << std::hex << +cOffset << std::dec << " gain: " << cGain << RESET;
                if(cVref < 1.0) { cTrimOptimized = (1 << (nBits - cBit - 1)) | cTrimOptimized; }
                cVREFTuneVect.push_back(cTrim);
                cADCValVect.push_back(cADC1V25);
                cOffsetVect.push_back(cOffset);
                cGainVect.push_back(cGain);
                cVrefVect.push_back(cVref);
            }
            for(uint16_t i = 0; i < cVREFTuneVect.size(); i++) { cCalibrationGraph->SetPoint(i, cVrefVect.at(i), cVREFTuneVect.at(i)); }
            LOG(INFO) << BOLDBLUE << "OTHybridTester::calibrateADC(): Best Vref: " << +cTrimOptimized << " rounded 0x" << std::hex << +cTrimOptimized << std::dec << RESET;
            // cGain   = clpGBTInterface->GetADCGain(cOpticalGroup->flpGBT, false);
            // cOffset = clpGBTInterface->GetADCOffset(cOpticalGroup->flpGBT, false);

            // flpGBTInterface->GetExternalController()->getInterface().set_AMUX(4095, 4095);
            cVREFTuneVect.clear();
            cADCValVect.clear();
            cOffsetVect.clear();
            cGainVect.clear();
            cVrefVect.clear();
            cCalibrationGraph->Clear(); */
            for(uint8_t cBit = 0; cBit < nBits; cBit += 1)
            {
                cTrim = (1 << (nBits - cBit - 1)) | cTrimOptimized;
                clpGBTInterface->WriteChipReg(cOpticalGroup->flpGBT, registerStr, cTrim);
                if(fIsSEH)
                {
                    cADC1V25 = clpGBTInterface->ReadADC(cOpticalGroup->flpGBT, "ADC0", "VREF/2");
                    cVref    = 3303. / 4096. * cGain * 512 / (cADC1V25 - cOffset * (1 - cGain / 2.));
                    // std::cout << std::dec << trim << "," << cOffset << "," << cGain << "," << cADC1V25 << "," << cTestCard1V25 << "," << cVref << std::endl;
                    LOG(INFO) << BOLDBLUE << "OTHybridTester::calibrateADC(): VREFTune: 0x" << std::hex << +cTrim << std::dec << " ADC value: 0x" << std::hex << +cADC1V25 << std::dec
                              << " calculated Vref: " << cVref << "V offset: 0x" << std::hex << +cOffset << std::dec << " gain: " << cGain << RESET;
                }
                else
                {
                    cADC1V25 = clpGBTInterface->ReadADC(cOpticalGroup->flpGBT, "ADC7", "VREF/2");
                    cVref    = cTestCardVDD2V55 * 51. / 161. * cGain * 512 / (cADC1V25 - cOffset * (1 - cGain / 2.));
                    // std::cout << std::dec << trim << "," << cOffset << "," << cGain << "," << cADC1V25 << "," << cTestCard1V25 << "," << cVref << std::endl;
                    LOG(INFO) << BOLDBLUE << "OTHybridTester::calibrateADC(): VREFTune: 0x" << std::hex << +cTrim << std::dec << " ADC value: 0x" << std::hex << +cADC1V25 << std::dec
                              << " calculated Vref: " << cVref << "V offset: 0x" << std::hex << +cOffset << std::dec << " gain: " << cGain << RESET;
                }
                if(cVref < 1.0) { cTrimOptimized = (1 << (nBits - cBit - 1)) | cTrimOptimized; }
                cVREFTuneVect.push_back(cTrim);
                cADCValVect.push_back(cADC1V25);
                cOffsetVect.push_back(cOffset);
                cGainVect.push_back(cGain);
                cVrefVect.push_back(cVref);
            }
            for(uint16_t i = 0; i < cVREFTuneVect.size(); i++) { cCalibrationGraph->SetPoint(i, cVrefVect.at(i), cVREFTuneVect.at(i)); }
            LOG(INFO) << BOLDBLUE << "OTHybridTester::calibrateADC(): Best Vref: " << +cTrimOptimized << " rounded 0x" << std::hex << +cTrimOptimized << std::dec << RESET;
        }
    }
    fResultFile->cd();
    cCalibrationTree->Fill();
    // cCalibrationTree->Write();
    fTC_2SSEH->set_P1V25_L_Sense(TC_2SSEH::P1V25SenseState::P1V25SenseState_Off);
    return cTrimOptimized;
}

void OTHybridTester::calibrateCurrentDAC()
{
    float                 cGain     = 0;
    uint16_t              cOffset   = 0;
    uint16_t              cADCTempp = 0;
    std::vector<uint16_t> cDACVect;
    std::vector<float>    cCurrentVect;
    std::vector<uint16_t> cADCVect;
    std::vector<uint16_t> cOffsetVect;
    std::vector<float>    cGainVect;
    auto                  cDACtoADCCanvas = new TCanvas("cCurrentDACtoADC", " Current DAC to ADC conversion", 500, 500);
    auto                  cObj            = gROOT->FindObject("mgCurrentDACtoADC");
    if(cObj) delete cObj;
    auto cDACtoADCMultiGraph = new TMultiGraph();
    cDACtoADCMultiGraph->SetName("mgCurrentDACtoADC");
    cDACtoADCMultiGraph->SetTitle("lpGBT - Current DAC to ADC conversion");
    auto cCalibrationTree = new TTree("tCurrentDACCalibration", "Calibration of the current DAC");

    cCalibrationTree->Branch("ADCValue", &cADCVect);
    cCalibrationTree->Branch("CurrentDACValue", &cDACVect);
    cCalibrationTree->Branch("Current", &cCurrentVect);
    cCalibrationTree->Branch("Offset", &cOffsetVect);
    cCalibrationTree->Branch("Gain", &cGainVect);

    D19clpGBTInterface* clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            // clpGBTInterface->WriteChipReg(cOpticalGroup->flpGBT, "VREFTUNE", 132); // optimal tune
            cGain   = clpGBTInterface->GetADCGain(cOpticalGroup->flpGBT, false);
            cOffset = clpGBTInterface->GetADCOffset(cOpticalGroup->flpGBT, false);
            for(int cDAC = 2; cDAC < 52; cDAC += 0x1)
            {
                clpGBTInterface->ConfigureCurrentDAC(cOpticalGroup->flpGBT, std::vector<std::string>{"ADC4"}, cDAC);
                cADCTempp           = clpGBTInterface->ReadADC(cOpticalGroup->flpGBT, "ADC4", "VREF/2", 0);
                float cTemppVoltage = 1. / cGain / 512. * (cADCTempp - cOffset * (1. - cGain / 2.));
                float cCurrent      = cTemppVoltage / 5100. * 1e6;
                LOG(INFO) << BOLDBLUE << "OTHybridTester::calibrateCurrenDAC(): Current DAC value: 0x" << std::hex << +cDAC << std::dec << " ADC value: 0x" << std::hex << +cADCTempp << std::dec
                          << " calculated voltage: " << cTemppVoltage << "V calculated current: " << cCurrent << "muA offset: 0x" << std::hex << +cOffset << std::dec << " gain: " << cGain << RESET;
                cCurrentVect.push_back(cCurrent);
                cDACVect.push_back(cDAC);
                cADCVect.push_back(cADCTempp);
                cOffsetVect.push_back(cOffset);
                cGainVect.push_back(cGain);
            }
        }
    }
    auto cCalibrationGraph = new TGraph();
    for(uint16_t i = 0; i < cCurrentVect.size(); i++) { cCalibrationGraph->SetPoint(i, cDACVect.at(i), cCurrentVect.at(i)); }
    cCalibrationGraph->Fit("pol1");
    TF1* cFit = cCalibrationGraph->GetFunction("pol1");
    fillSummaryTree("Current_DAC_p1", cFit->GetParameter(1));
    fResultFile->cd();
    cCalibrationTree->Fill();
    cCalibrationTree->Write();
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard) { clpGBTInterface->ConfigureCurrentDAC(cOpticalGroup->flpGBT, std::vector<std::string>{"ADC4"}, 0x1c); }
    }
    auto cDACtoADCGraph = new TGraph();
    for(uint16_t i = 0; i < cDACVect.size(); i++) { cDACtoADCGraph->SetPoint(i, cDACVect.at(i), cADCVect.at(i)); }
    cDACtoADCGraph->SetName("gCurrentDACtoADC");
    cDACtoADCGraph->SetTitle("CurrentDACtoADC");
    cDACtoADCGraph->SetLineColor(1);
    cDACtoADCGraph->SetFillColor(0);
    cDACtoADCGraph->SetLineWidth(3);
    cDACtoADCMultiGraph->Add(cDACtoADCGraph);
    cDACtoADCMultiGraph->Draw("AL*");
    cDACtoADCMultiGraph->GetXaxis()->SetTitle("Current DAC");
    cDACtoADCMultiGraph->GetYaxis()->SetTitle("ADC");
    cDACtoADCCanvas->Write();
    // cCalibrationTree->Write();
}

#endif
