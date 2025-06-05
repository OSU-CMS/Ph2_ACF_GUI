
#if defined(__TCUSB__) && defined(__USE_ROOT__)
#include "PSROHTester.h"

// initialize the static member
PSROHTester::PSROHTester() : OTHybridTester() {}

PSROHTester::~PSROHTester() {}

void PSROHTester::Initialise()
{
    for(auto cBoard: *fDetectorContainer)
    {
        D19clpGBTInterface* clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
        for(auto cOpticalGroup: *cBoard)
        {
            clpGBTInterface->AddPSROHeLinkProperties(cOpticalGroup->flpGBT);

            clpGBTInterface->ConfigurePSROH(cOpticalGroup->flpGBT);
            //
            uint8_t          cChipRate = clpGBTInterface->GetChipRate(cOpticalGroup->flpGBT);
            lpGBTClockConfig cClkCnfg;
            cClkCnfg.fClkFreq         = (cChipRate == 5) ? 4 : 5;
            cClkCnfg.fClkDriveStr     = 7;
            cClkCnfg.fClkPreEmphWidth = 0;
            cClkCnfg.fClkPreEmphMode  = 0; // 3;
            cClkCnfg.fClkPreEmphStr   = 0; // 7;

            cClkCnfg.fClkInvert = 1;
            LOG(INFO) << BOLDBLUE << "Enabling SSA clocks" << RESET;
            clpGBTInterface->hybridClock(cOpticalGroup->flpGBT, cClkCnfg, 0);
            clpGBTInterface->hybridClock(cOpticalGroup->flpGBT, cClkCnfg, 1);

            // enable clock to CIC
            cClkCnfg.fClkInvert = 0;
            LOG(INFO) << BOLDBLUE << "Enabling CIC clocks" << RESET;
            clpGBTInterface->cicClock(cOpticalGroup->flpGBT, cClkCnfg, 0);
            clpGBTInterface->cicClock(cOpticalGroup->flpGBT, cClkCnfg, 1);
        }
    }
}

void PSROHTester::MeasureInputIV(const std::string& cTestStep)
{
    for(auto& cMeas: fInputIVMap)
    {
        float cVal;
        fTC_PSROH->adc_get(cMeas.second, cVal);
        LOG(INFO) << BOLDYELLOW << "Measuring " << cMeas.first << " to be at " << +(cVal / 1000) << " [SI] during phase " << cTestStep << RESET;
    }
}

void PSROHTester::UserFCMDTranslate(const std::string& userFilename = "fcmd_file.txt")
{
    const std::string cUserFilenameFull    = "fcmd_files/user_files/" + userFilename;
    const std::string cRefFCMDFilenameFull = "fcmd_files/" + userFilename;

    std::ifstream            cFCMDUserFileHandle(cUserFilenameFull);
    std::vector<std::string> cUserRequests;
    std::string              cLine;
    while(std::getline(cFCMDUserFileHandle, cLine))
    {
        boost::trim_right(cLine);
        cUserRequests.push_back(cLine);
    }

    std::map<int, std::string> cFCMDvsBX;
    std::vector<std::string>   tokens;
    for(auto cUserRequest: cUserRequests)
    {
        boost::split(tokens, cUserRequest, boost::is_any_of(" "));
        int        cBX   = std::atoi(tokens[0].c_str());
        const auto cFCMD = std::string(tokens[1]);
        cFCMDvsBX[cBX]   = std::string("101") + cFCMD + std::string("0");
        // cFCMDvsBX[cBX] = std::string("110")+cFCMD+std::string("1");
    }

    int cMaxNumBXs = -1;
    for(auto cItem: cFCMDvsBX) cMaxNumBXs = (cItem.first > cMaxNumBXs) ? cItem.first : cMaxNumBXs;

    std::ofstream cRefFCMDHandle(cRefFCMDFilenameFull);
    for(int cBXNum = 1; cBXNum < cMaxNumBXs + 1; ++cBXNum)
    {
        auto cIt   = cFCMDvsBX.find(cBXNum);
        auto cFCMD = (cIt == cFCMDvsBX.end()) ? "11000001" : cIt->second;
        cRefFCMDHandle << cFCMD << std::endl;
    }
    cRefFCMDHandle.close();
}

void PSROHTester::ClearBRAM(BeBoard* pBoard, const std::string& sBRAMToReset)
{
    std::string cRegNameData;
    std::string cRegNameAddr;
    std::string cRegNameWrite;
    if(sBRAMToReset == std::string("ref"))
    {
        cRegNameData  = "fc7_daq_cnfg.physical_interface_block.fe_for_ps_roh_fcmd_test.ref_data_to_bram";
        cRegNameAddr  = "fc7_daq_cnfg.physical_interface_block.fe_for_ps_roh_fcmd_test.ref_data_bram_addr";
        cRegNameWrite = "fc7_daq_ctrl.physical_interface_block.fe_for_ps_roh.write_ref_fcmd_to_bram";
    }
    else if(sBRAMToReset == std::string("test"))
    {
        cRegNameData  = "fc7_daq_cnfg.physical_interface_block.fe_for_ps_roh_fcmd_check.test_data_to_bram";
        cRegNameAddr  = "fc7_daq_cnfg.physical_interface_block.fe_for_ps_roh_fcmd_check.test_data_bram_addr";
        cRegNameWrite = "fc7_daq_ctrl.physical_interface_block.fe_for_ps_roh.write_test_fcmd_to_bram";
    }
    for(unsigned int cBRAMAddress = 0; cBRAMAddress < NBRAMADDR; ++cBRAMAddress)
    {
        fBeBoardInterface->WriteBoardReg(pBoard, cRegNameData.c_str(), 0x00);
        fBeBoardInterface->WriteBoardReg(pBoard, cRegNameAddr, cBRAMAddress);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        fBeBoardInterface->WriteBoardReg(pBoard, cRegNameWrite, 0x01);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void PSROHTester::ClearBRAM(const std::string& sBramToReset)
{
    for(auto cBoard: *fDetectorContainer)
    {
        if(cBoard->isOptical()) continue;
        this->ClearBRAM(cBoard, sBramToReset);
    }
}

void PSROHTester::WritePatternToBRAM(BeBoard* pBoard, const std::string& filename = "fcmd_file.txt")
{
    //        this -> UserFCMDTranslate(filename);
    this->ClearBRAM("ref");
    bool             cIsSSAlFCMDBRAMGood = true;
    bool             cIsSSArFCMDBRAMGood = true;
    bool             cIsCIClFCMDBRAMGood = true;
    bool             cIsCICrFCMDBRAMGood = true;
    std::vector<int> cFailedAddrSSAl;
    std::vector<int> cFailedAddrSSAr;
    std::vector<int> cFailedAddrCICl;
    std::vector<int> cFailedAddrCICr;

    const std::string cRefFCMDFilenameFull = "fcmd_files/" + filename;
    std::ifstream     cUserHandle(cRefFCMDFilenameFull);
    std::string       cLine;
    int               cBRAMAddress = 0;
    while(std::getline(cUserHandle, cLine))
    {
        // std::cout << cBRAMAddress << std::endl;
        // std::cout << std::atoi(cLine.c_str()) << " " << std::stoi(cLine.c_str(),nullptr,2) << std::endl;
        fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.physical_interface_block.fe_for_ps_roh_fcmd_test.ref_data_to_bram", std::stoi(cLine.c_str(), nullptr, 2));
        fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.physical_interface_block.fe_for_ps_roh_fcmd_test.ref_data_bram_addr", cBRAMAddress);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_ctrl.physical_interface_block.fe_for_ps_roh.write_ref_fcmd_to_bram", 0x01);

        // Verify write operation is correct
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        int cRefSSAlFCMDBRAMData = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fe_for_ps_roh_ssa_fcmd_test.fe_for_ps_roh_fcmd_SSA_l_ref");
        // std::cout << "SSAl:" << cRefSSAlFCMDBRAMData << " <-> " << cLine << std::endl;
        if(cRefSSAlFCMDBRAMData != std::stoi(cLine.c_str(), nullptr, 2))
        {
            cFailedAddrSSAl.push_back(cBRAMAddress);
            cIsSSAlFCMDBRAMGood = false;
        }

        int cRefSSArFCMDBRAMData = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fe_for_ps_roh_ssa_fcmd_test.fe_for_ps_roh_fcmd_SSA_r_ref");
        // std::cout << "SSAr:" << cRefSSArFCMDBRAMData << " <-> " << cLine << std::endl;
        if(cRefSSArFCMDBRAMData != std::stoi(cLine.c_str(), nullptr, 2))
        {
            cFailedAddrSSAr.push_back(cBRAMAddress);
            cIsSSArFCMDBRAMGood = false;
        }

        int cRefCIClFCMDBRAMData = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fe_for_ps_roh_cic_fcmd_test.fe_for_ps_roh_fcmd_CIC_l_ref");
        // std::cout << "CICl:" << cRefCIClFCMDBRAMData << " <-> " << cLine << std::endl;
        if(cRefCIClFCMDBRAMData != std::stoi(cLine.c_str(), nullptr, 2))
        {
            cFailedAddrCICl.push_back(cBRAMAddress);
            cIsCIClFCMDBRAMGood = false;
        }

        int cRefCICrFCMDBRAMData = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fe_for_ps_roh_cic_fcmd_test.fe_for_ps_roh_fcmd_CIC_r_ref");
        // std::cout << "CICr:" << cRefCICrFCMDBRAMData << " <-> " << cLine << std::endl;
        if(cRefCICrFCMDBRAMData != std::stoi(cLine.c_str(), nullptr, 2))
        {
            cFailedAddrCICr.push_back(cBRAMAddress);
            cIsCICrFCMDBRAMGood = false;
        }

        cBRAMAddress++;
    }
    if(cIsSSAlFCMDBRAMGood)
        LOG(INFO) << "SSA l reference FCMD writing to BRAM ->" << BOLDGREEN << " Successful" << RESET;
    else
    {
        LOG(ERROR) << "SSA l reference FCMD writing to BRAM ->" << BOLDRED << " Failed" << RESET;
        LOG(INFO) << "Failed address ";
        std::stringstream cssFailedAddrList;
        for(auto el: cFailedAddrSSAl) cssFailedAddrList << el << " ";
        LOG(INFO) << BOLDBLUE << cssFailedAddrList.str() << RESET;
    }

    if(cIsSSArFCMDBRAMGood)
        LOG(INFO) << "SSA r reference FCMD writing to BRAM ->" << BOLDGREEN << " Successful" << RESET;
    else
    {
        LOG(ERROR) << "SSA r reference FCMD writing to BRAM ->" << BOLDRED << " Failed" << RESET;
        LOG(INFO) << "Failed address ";
        std::stringstream cssFailedAddrList;
        for(auto el: cFailedAddrSSAr) cssFailedAddrList << el << " ";
        LOG(INFO) << BOLDBLUE << cssFailedAddrList.str() << RESET;
    }

    if(cIsCIClFCMDBRAMGood)
        LOG(INFO) << "CIC l reference FCMD writing to BRAM ->" << BOLDGREEN << " Successful" << RESET;
    else
    {
        LOG(ERROR) << "CIC l reference FCMD writing to BRAM ->" << BOLDRED << " Failed" << RESET;
        LOG(INFO) << "Failed address ";
        std::stringstream cssFailedAddrList;
        for(auto el: cFailedAddrCICl) cssFailedAddrList << el << " ";
        LOG(INFO) << BOLDBLUE << cssFailedAddrList.str() << RESET;
    }

    if(cIsCICrFCMDBRAMGood)
        LOG(INFO) << "CIC r reference FCMD writing to BRAM ->" << BOLDGREEN << " Successful" << RESET;
    else
    {
        LOG(ERROR) << "CIC r reference FCMD writing to BRAM ->" << BOLDRED << " Failed" << RESET;
        LOG(INFO) << "Failed address ";
        std::stringstream cssFailedAddrList;
        for(auto el: cFailedAddrCICr) cssFailedAddrList << el << " ";
        LOG(INFO) << BOLDBLUE << cssFailedAddrList.str() << RESET;
    }
}

void PSROHTester::WritePatternToBRAM(const std::string& sFileName = "fcmd_file.txt")
{
    for(auto cBoard: *fDetectorContainer)
    {
        if(cBoard->isOptical()) continue;
        this->WritePatternToBRAM(cBoard, sFileName);
    }
}

void PSROHTester::CheckFastCommandsBRAM(BeBoard* pBoard, const std::string& sFCMDLine)
{
    std::string                     cOutputErrorsFileName = sFCMDLine;
    std::ofstream                   cBRAMErrorsFileHandle(cOutputErrorsFileName);
    std::map<int, std::vector<int>> cPatterns;
    for(int cBRAMAddress = 0; cBRAMAddress < NBRAMADDR; ++cBRAMAddress)
    {
        fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.physical_interface_block.fe_for_ps_roh_fcmd_check.test_data_bram_addr", cBRAMAddress);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::string cRegName("fc7_daq_stat.physical_interface_block.");
        cRegName += sFCMDLine;
        int              cCheckFCMDBRAMData = fBeBoardInterface->ReadBoardReg(pBoard, cRegName.c_str());
        std::vector<int> temp;
        auto             cIt = cPatterns.find(cCheckFCMDBRAMData);
        if(cIt != cPatterns.end()) temp = cIt->second;
        temp.push_back(cBRAMAddress);
        cPatterns[cCheckFCMDBRAMData] = temp;
        cBRAMErrorsFileHandle << std::setw(10) << cBRAMAddress << std::setw(10) << std::bitset<8>(cCheckFCMDBRAMData) << std::endl;
    }
    cBRAMErrorsFileHandle.close();

    LOG(INFO) << BOLDBLUE << "Patterns: " << RESET;
    for(auto cIt: cPatterns)
    {
        LOG(INFO) << BOLDBLUE << std::bitset<8>(cIt.first) << " appears " << cIt.second.size() << " times " << RESET;
        std::stringstream csCorruptedAddrList;
        for(auto el: cIt.second) csCorruptedAddrList << el << " ";
        LOG(INFO) << BOLDBLUE << "Addresses list: " << csCorruptedAddrList.str() << RESET;
    }
}

void PSROHTester::CheckFastCommandsBRAM(const std::string& sFCMDLine)
{
    for(auto cBoard: *fDetectorContainer)
    {
        if(cBoard->isOptical()) continue;
        this->CheckFastCommandsBRAM(cBoard, sFCMDLine);
    }
}

void PSROHTester::CheckFastCommands(BeBoard* pBoard, const std::string& sFastCommand, const std::string& filename = "fcmd_file.txt")
{
    this->ClearBRAM("test");
    this->WritePatternToBRAM(pBoard, filename);
    // fcmd test
    fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.physical_interface_block.fe_for_ps_roh_fcmd_test.start_pattern", std::stoi(sFastCommand.c_str(), nullptr, 2));
    fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_ctrl.physical_interface_block.fe_for_ps_roh.start_fe_for_ps_roh_fcmd_test", 0x01);

    bool cSSAlFCMDCheckDone = (fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fcmd_test.fe_for_ps_roh_fcmd_SSA_l_test_done") == 1);
    bool cSSArFCMDCheckDone = (fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fcmd_test.fe_for_ps_roh_fcmd_SSA_r_test_done") == 1);
    bool cCIClFCMDCheckDone = (fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fcmd_test.fe_for_ps_roh_fcmd_CIC_l_test_done") == 1);
    bool cCICrFCMDCheckDone = (fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fcmd_test.fe_for_ps_roh_fcmd_CIC_r_test_done") == 1);

    LOG(INFO) << GREEN << "============================" << RESET;
    LOG(INFO) << BOLDGREEN << "Fast commands test" << RESET;

    LOG(INFO) << "Waiting for FCMD test";
    const auto MAXNRETRY = 100;
    auto       NTrials   = 0;
    while(!cSSAlFCMDCheckDone && NTrials < MAXNRETRY)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cSSAlFCMDCheckDone = (fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fcmd_test.fe_for_ps_roh_fcmd_SSA_l_test_done") == 1);
        NTrials++;
    }
    if(cSSAlFCMDCheckDone)
    {
        bool SSAlFCMDStat = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fcmd_test.fe_for_ps_roh_fcmd_SSA_l_stat");
        if(SSAlFCMDStat) { LOG(INFO) << "SSA l FCMD test ->" << BOLDGREEN << " PASSED" << RESET; }
        else
        {
            LOG(ERROR) << "SSA l FCMD test ->" << BOLDRED << " FAILED" << RESET;
            this->CheckFastCommandsBRAM(pBoard, std::string("fe_for_ps_roh_ssa_fcmd_test.fe_for_ps_roh_fcmd_SSA_l_check"));
        }
    }
    else
        LOG(INFO) << "SSA l FCMD test ->" << BOLDGREEN << " time out" << RESET;

    NTrials = 0;
    while(!cSSArFCMDCheckDone && NTrials < MAXNRETRY)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cSSArFCMDCheckDone = (fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fcmd_test.fe_for_ps_roh_fcmd_SSA_r_test_done") == 1);
        NTrials++;
    }
    if(cSSArFCMDCheckDone)
    {
        bool SSArFCMDStat = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fcmd_test.fe_for_ps_roh_fcmd_SSA_r_stat");
        if(SSArFCMDStat) { LOG(INFO) << "SSA r FCMD test ->" << BOLDGREEN << " PASSED" << RESET; }
        else
        {
            LOG(ERROR) << "SSA r FCMD test ->" << BOLDRED << " FAILED" << RESET;
            this->CheckFastCommandsBRAM(pBoard, std::string("fe_for_ps_roh_ssa_fcmd_test.fe_for_ps_roh_fcmd_SSA_r_check"));
        }
    }
    else
        LOG(INFO) << "SSA r FCMD test ->" << BOLDGREEN << " time out" << RESET;

    NTrials = 0;
    while(!cCIClFCMDCheckDone && NTrials < MAXNRETRY)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cCIClFCMDCheckDone = (fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fcmd_test.fe_for_ps_roh_fcmd_CIC_l_test_done") == 1);
        NTrials++;
    }
    if(cCIClFCMDCheckDone)
    {
        bool CIClFCMDStat = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fcmd_test.fe_for_ps_roh_fcmd_CIC_l_stat");
        if(CIClFCMDStat) { LOG(INFO) << "CIC l FCMD test ->" << BOLDGREEN << " PASSED" << RESET; }
        else
        {
            LOG(ERROR) << "CIC l FCMD test ->" << BOLDRED << " FAILED" << RESET;
            this->CheckFastCommandsBRAM(pBoard, std::string("fe_for_ps_roh_cic_fcmd_test.fe_for_ps_roh_fcmd_CIC_l_check"));
        }
    }
    else
        LOG(INFO) << "CIC l FCMD test ->" << BOLDGREEN << " time out" << RESET;

    NTrials = 0;
    while(!cCICrFCMDCheckDone && NTrials < MAXNRETRY)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cCICrFCMDCheckDone = (fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fcmd_test.fe_for_ps_roh_fcmd_CIC_r_test_done") == 1);
        NTrials++;
    }
    if(cCICrFCMDCheckDone)
    {
        bool CICrFCMDStat = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fcmd_test.fe_for_ps_roh_fcmd_CIC_r_stat");
        if(CICrFCMDStat) { LOG(INFO) << "CIC r FCMD test ->" << BOLDGREEN << " PASSED" << RESET; }
        else
        {
            LOG(ERROR) << "CIC r FCMD test ->" << BOLDRED << " FAILED" << RESET;
            this->CheckFastCommandsBRAM(pBoard, std::string("fe_for_ps_roh_cic_fcmd_test.fe_for_ps_roh_fcmd_CIC_r_check"));
        }
    }
    else
        LOG(INFO) << "CIC r FCMD test ->" << BOLDGREEN << " time out" << RESET;

    LOG(INFO) << GREEN << "============================" << RESET;
}

void PSROHTester::CheckFastCommands(const std::string& sFastCommand, const std::string& filename = "fcmd_file.txt")
{
    for(auto cBoard: *fDetectorContainer)
    {
        if(cBoard->isOptical()) continue;
        this->CheckFastCommands(cBoard, sFastCommand, filename);
    }
}

void PSROHTester::ReadRefAddrBRAM(BeBoard* pBoard, int iRefBRAMAddr)
{
    fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.physical_interface_block.fe_for_ps_roh_fcmd_test.ref_data_bram_addr", iRefBRAMAddr);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    int cRefSSAlFCMDBRAMData = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fe_for_ps_roh_ssa_fcmd_test.fe_for_ps_roh_fcmd_SSA_l_ref");
    LOG(INFO) << BOLDGREEN << "SSA l " << cRefSSAlFCMDBRAMData << RESET;

    int cRefSSArFCMDBRAMData = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fe_for_ps_roh_ssa_fcmd_test.fe_for_ps_roh_fcmd_SSA_r_ref");
    LOG(INFO) << BOLDGREEN << "SSA r " << cRefSSArFCMDBRAMData << RESET;

    int cRefCIClFCMDBRAMData = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fe_for_ps_roh_cic_fcmd_test.fe_for_ps_roh_fcmd_CIC_l_ref");
    LOG(INFO) << BOLDGREEN << "CIC l " << cRefCIClFCMDBRAMData << RESET;

    int cRefCICrFCMDBRAMData = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fe_for_ps_roh_cic_fcmd_test.fe_for_ps_roh_fcmd_CIC_r_ref");
    LOG(INFO) << BOLDGREEN << "CIC r " << cRefCICrFCMDBRAMData << RESET;
}
void PSROHTester::ReadRefAddrBRAM(int iRefBRAMAddr)
{
    for(auto cBoard: *fDetectorContainer)
    {
        if(cBoard->isOptical()) continue;
        this->ReadRefAddrBRAM(cBoard, iRefBRAMAddr);
    }
}

void PSROHTester::ReadCheckAddrBRAM(BeBoard* pBoard, int iCheckBRAMAddr)
{
    fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.physical_interface_block.fe_for_ps_roh_fcmd_check.test_data_bram_addr", iCheckBRAMAddr);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    int cCheckSSAlFCMDBRAMData = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fe_for_ps_roh_ssa_fcmd_test.fe_for_ps_roh_fcmd_SSA_l_check");
    LOG(INFO) << BOLDGREEN << "SSA l " << cCheckSSAlFCMDBRAMData << RESET;

    int cCheckSSArFCMDBRAMData = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fe_for_ps_roh_ssa_fcmd_test.fe_for_ps_roh_fcmd_SSA_r_check");
    LOG(INFO) << BOLDGREEN << "SSA r " << cCheckSSArFCMDBRAMData << RESET;

    int cCheckCIClFCMDBRAMData = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fe_for_ps_roh_cic_fcmd_test.fe_for_ps_roh_fcmd_CIC_l_check");
    LOG(INFO) << BOLDGREEN << "CIC l " << cCheckCIClFCMDBRAMData << RESET;

    int cCheckCICrFCMDBRAMData = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fe_for_ps_roh_cic_fcmd_test.fe_for_ps_roh_fcmd_CIC_r_check");
    LOG(INFO) << BOLDGREEN << "CIC r " << cCheckCICrFCMDBRAMData << RESET;
}

void PSROHTester::ReadCheckAddrBRAM(int iCheckBRAMAddr)
{
    for(auto cBoard: *fDetectorContainer)
    {
        if(cBoard->isOptical()) continue;
        this->ReadCheckAddrBRAM(cBoard, iCheckBRAMAddr);
    }
}

bool PSROHTester::FastCommandScope(BeBoard* pBoard)
{
    uint32_t cSSA_L = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fcmd_debug_ssa_l");
    uint32_t cSSA_R = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fcmd_debug_ssa_r");
    uint32_t cCIC_L = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fcmd_debug_cic_l");
    uint32_t cCIC_R = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.physical_interface_block.fcmd_debug_cic_r");

    LOG(INFO) << BOLDBLUE << "Scoped output on SSA_L : " << std::bitset<32>(cSSA_L) << RESET;
    LOG(INFO) << BOLDBLUE << "Scoped output on SSA_R : " << std::bitset<32>(cSSA_R) << RESET;
    LOG(INFO) << BOLDBLUE << "Scoped output on CIC_L : " << std::bitset<32>(cCIC_L) << RESET;
    LOG(INFO) << BOLDBLUE << "Scoped output on CIC_R : " << std::bitset<32>(cCIC_R) << RESET;

    /******************************/
    /* Processing of scoped lines */
    /******************************/
    bool        cSuccess      = true;
    std::string cPattern      = "11000001";
    std::string cSSA_L_scoped = std::bitset<32>(cSSA_L).to_string();
    std::string cSSA_R_scoped = std::bitset<32>(cSSA_R).to_string();
    std::string cCIC_L_scoped = std::bitset<32>(cCIC_L).to_string();
    std::string cCIC_R_scoped = std::bitset<32>(cCIC_R).to_string();

    std::vector<std::string> cScoped_Lines = {cSSA_L_scoped, cSSA_R_scoped, cCIC_L_scoped, cCIC_R_scoped};
    std::vector<bool>        cStatus_Lines;
    uint8_t                  cNFailedLines = 0;

    for(auto& cLine: cScoped_Lines)
    {
        for(int i = 0; (i + cPattern.length()) < cLine.length(); i += cPattern.length())
        {
            std::string cSubLine      = cLine.substr(i, cPattern.length());
            bool        cPatternFound = false;
            for(int j = 0; j < (int)cPattern.length(); j++) cPatternFound |= ((cPattern.substr(j, cPattern.length() - j) + cPattern.substr(0, j)) == cSubLine);

            cSuccess &= cPatternFound;
            cStatus_Lines.push_back(cPatternFound);
            if(!cPatternFound) cNFailedLines++;
        }
    }
#ifdef __USE_ROOT__
    fillSummaryTree("fcmd_failures", cNFailedLines);

    if(cNFailedLines > 0)
    {
        auto    cFCMD_Tree = new TTree("tFCMD", "FCMD Lines Test Tree");
        TString cLine_name("");
        TString cScoped_line("");
        cFCMD_Tree->Branch("Line", &cLine_name);
        cFCMD_Tree->Branch("Scoped data", &cScoped_line);

        if(!cStatus_Lines[0])
        {
            cLine_name   = "SSA_L";
            cScoped_line = cScoped_Lines[0];
            cFCMD_Tree->Fill();
        }

        if(!cStatus_Lines[1])
        {
            cLine_name   = "SSA_R";
            cScoped_line = cScoped_Lines[1];
            cFCMD_Tree->Fill();
        }

        if(!cStatus_Lines[2])
        {
            cLine_name   = "CIC_L";
            cScoped_line = cScoped_Lines[2];
            cFCMD_Tree->Fill();
        }

        if(!cStatus_Lines[3])
        {
            cLine_name   = "CIC_R";
            cScoped_line = cScoped_Lines[3];
            cFCMD_Tree->Fill();
        }

        fResultFile->cd();
        cFCMD_Tree->Write();
    }
#endif
    // TODO create summary tree with the details (failing line and actual scoped value)
    return cSuccess;
}

bool PSROHTester::FastCommandScope()
{
    bool cSuccess = true;
    for(auto cBoard: *fDetectorContainer)
    {
        if(cBoard->isOptical()) continue;
        cSuccess &= this->FastCommandScope(cBoard);
    }
    return cSuccess;
}
void PSROHTester::CheckHybridInputs(BeBoard* pBoard, std::vector<std::string> pInputs, std::vector<uint32_t>& pCounters)
{
    uint32_t             cRegisterValue = 0;
    std::vector<uint8_t> cIndices(0);
    for(auto cInput: pInputs)
    {
        auto cMapIterator = fInputDebugMap.find(cInput);
        if(cMapIterator != fInputDebugMap.end())
        {
            auto& cIndex   = cMapIterator->second;
            cRegisterValue = cRegisterValue | (1 << cIndex);
            cIndices.push_back(cIndex);
        }
    }
    // select input lines
    LOG(INFO) << BOLDBLUE << "Configuring debug register : " << std::bitset<32>(cRegisterValue) << RESET;
    fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.physical_interface_block.debug_blk_input", cRegisterValue);
    // start
    fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_ctrl.physical_interface_block.debug_blk.start_input", 1);
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    // stop
    fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_ctrl.physical_interface_block.debug_blk.stop_input", 1);
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    // check counters
    pCounters.clear();
    pCounters.resize(cIndices.size());
    for(auto cIndex: cIndices)
    {
        // char cBuffer[19];
        // sprintf(cBuffer, "debug_blk_counter%02d", cIndex);
        // std::string cRegName = cBuffer;
        std::string cRegName = "debug_blk_counter" + (boost::format("%|02|") % cIndex).str();
        uint32_t    cCounter = fBeBoardInterface->ReadBoardReg(pBoard, cRegName);
        pCounters.push_back(cCounter);
    }
}
void PSROHTester::CheckHybridInputs(std::vector<std::string> pInputs, std::vector<uint32_t>& pCounters)
{
    for(auto cBoard: *fDetectorContainer)
    {
        if(cBoard->isOptical()) continue;
        this->CheckHybridInputs(cBoard, pInputs, pCounters);
    }
}

void PSROHTester::CheckHybridOutputs(BeBoard* pBoard, std::vector<std::string> pOutputs, std::vector<uint32_t>& pCounters)
{
    uint32_t             cRegisterValue = 0;
    std::vector<uint8_t> cIndices(0);
    for(auto cInput: pOutputs)
    {
        auto cMapIterator = fOutputDebugMap.find(cInput);
        if(cMapIterator != fOutputDebugMap.end())
        {
            auto& cIndex   = cMapIterator->second;
            cRegisterValue = cRegisterValue | (1 << cIndex);
            cIndices.push_back(cIndex);
        }
    }
    // select input lines
    LOG(INFO) << BOLDBLUE << "Configuring debug register : " << std::bitset<32>(cRegisterValue) << RESET;
    fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.physical_interface_block.debug_blk_output", cRegisterValue);
    // start
    fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_ctrl.physical_interface_block.debug_blk.start_output", 1);
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    // stop
    fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_ctrl.physical_interface_block.debug_blk.stop_output", 1);
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    // check counters
    pCounters.clear();
    pCounters.resize(cIndices.size());
    for(auto cIndex: cIndices)
    {
        // char cBuffer[19];
        // sprintf(cBuffer, "debug_blk_counter%02d", cIndex);
        // std::string cRegName = cBuffer;
        std::string cRegName = "debug_blk_counter" + (boost::format("%|02|") % cIndex).str();
        uint32_t    cCounter = fBeBoardInterface->ReadBoardReg(pBoard, cRegName);
        pCounters.push_back(cCounter);
    }
}

void PSROHTester::PSROHInputsDebug()
{
    for(auto cBoard: *fDetectorContainer)
    {
        fBeBoardInterface->WriteBoardReg(cBoard, "fc7_daq_cnfg.physical_interface_block.debug_blk_input", 0x00003FFF);
        // start
        LOG(INFO) << BOLDBLUE << "Do you want to start test? [y/n]" << RESET;
        char Answer;
        std::cin >> Answer;
        if(Answer == 'y') { fBeBoardInterface->WriteBoardReg(cBoard, "fc7_daq_ctrl.physical_interface_block.debug_blk.start_input", 1); }
        else if(Answer == 'n') { exit(1); }
        else
        {
            LOG(ERROR) << "Wrong option!" << std::endl;
            exit(1);
        }
        // stop
        LOG(INFO) << BOLDBLUE << "Do you want to stop test? [y/n]" << RESET;
        std::cin >> Answer;
        while(Answer != 'y') { LOG(INFO) << BOLDBLUE << "Do you want to stop test? " << RESET; }
        fBeBoardInterface->WriteBoardReg(cBoard, "fc7_daq_ctrl.physical_interface_block.debug_blk.stop_input", 1);
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        // results
        LOG(INFO) << BOLDBLUE << "Input lines debug done:" << fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_stat.physical_interface_block.input_lines_debug_done");
        LOG(INFO) << BOLDBLUE << "Results for line:" << RESET;
        std::vector<std::string> RegisterTable = {{"fc7_daq_stat.physical_interface_block.debug_blk_counter00"},
                                                  {"fc7_daq_stat.physical_interface_block.debug_blk_counter01"},
                                                  {"fc7_daq_stat.physical_interface_block.debug_blk_counter02"},
                                                  {"fc7_daq_stat.physical_interface_block.debug_blk_counter03"},
                                                  {"fc7_daq_stat.physical_interface_block.debug_blk_counter04"},
                                                  {"fc7_daq_stat.physical_interface_block.debug_blk_counter05"},
                                                  {"fc7_daq_stat.physical_interface_block.debug_blk_counter06"},
                                                  {"fc7_daq_stat.physical_interface_block.debug_blk_counter07"},
                                                  {"fc7_daq_stat.physical_interface_block.debug_blk_counter08"},
                                                  {"fc7_daq_stat.physical_interface_block.debug_blk_counter09"},
                                                  {"fc7_daq_stat.physical_interface_block.debug_blk_counter10"},
                                                  {"fc7_daq_stat.physical_interface_block.debug_blk_counter11"},
                                                  {"fc7_daq_stat.physical_interface_block.debug_blk_counter12"},
                                                  {"fc7_daq_stat.physical_interface_block.debug_blk_counter13"}};

        std::map<std::string, std::string> RegisterAlias = {{"fc7_daq_stat.physical_interface_block.debug_blk_counter00", "l_fcmd_cic"},
                                                            {"fc7_daq_stat.physical_interface_block.debug_blk_counter01", "r_fcmd_cic"},
                                                            {"fc7_daq_stat.physical_interface_block.debug_blk_counter02", "l_fcmd_ssa"},
                                                            {"fc7_daq_stat.physical_interface_block.debug_blk_counter03", "r_fcmd_ssa"},
                                                            {"fc7_daq_stat.physical_interface_block.debug_blk_counter04", "l_clk_320"},
                                                            {"fc7_daq_stat.physical_interface_block.debug_blk_counter05", "r_clk_640"},
                                                            {"fc7_daq_stat.physical_interface_block.debug_blk_counter06", "l_clk_320"},
                                                            {"fc7_daq_stat.physical_interface_block.debug_blk_counter07", "r_clk_640"},
                                                            {"fc7_daq_stat.physical_interface_block.debug_blk_counter08", "l_i2c_scl"},
                                                            {"fc7_daq_stat.physical_interface_block.debug_blk_counter09", "r_i2c_scl"},
                                                            {"fc7_daq_stat.physical_interface_block.debug_blk_counter10", "l_i2c_sda_o"},
                                                            {"fc7_daq_stat.physical_interface_block.debug_blk_counter11", "r_i2c_sda_o"},
                                                            {"fc7_daq_stat.physical_interface_block.debug_blk_counter12", "cpg"},
                                                            {"fc7_daq_stat.physical_interface_block.debug_blk_counter13", "bpg"}};

        for(const auto& RegName: RegisterTable)
        {
            auto result = fBeBoardInterface->ReadBoardReg(cBoard, RegName.c_str());
            LOG(INFO) << BOLDBLUE << std::setw(20) << RegisterAlias[RegName] << std::setw(10) << result << RESET;
        }
    }
}

void PSROHTester::CheckHybridOutputs(std::vector<std::string> pInputs, std::vector<uint32_t>& pCounters)
{
    for(auto cBoard: *fDetectorContainer)
    {
        if(cBoard->isOptical()) continue;
        this->CheckHybridOutputs(cBoard, pInputs, pCounters);
    }
}

void PSROHTester::Start(const StartInfo& theStartInfo)
{
    LOG(INFO) << BOLDBLUE << "Starting PS ROH Tester" << RESET;
    Initialise();
}

void PSROHTester::Stop()
{
    LOG(INFO) << BOLDBLUE << "Stopping PS ROH Tester" << RESET;
    // writeObjects();
    dumpConfigFiles();
    Destroy();
}

void PSROHTester::Pause() {}

void PSROHTester::Resume() {}
#endif
