#include "tools/ExtTriggerLatencyScan.h"

std::string ExtTriggerLatencyScan::fCalibrationDescription = "Run latency scan with external triger";

ExtTriggerLatencyScan::ExtTriggerLatencyScan() : LatencyScan() {}
ExtTriggerLatencyScan::~ExtTriggerLatencyScan() {}

//////////////////////////////////////          PRIVATE METHODS             //////////////////////////////////////

void ExtTriggerLatencyScan::InitializeExternalTriggers()
{
    LOG(INFO) << "Prepare for external triggers" << RESET;
    this->enableTestPulse(false);
    for(auto cBoard: *fDetectorContainer)
    {
        uint8_t                                       cTriggerSource = 5;
        std::vector<std::string>                      cFcmdRegs{"trigger_source", "triggers_to_accept"};
        std::vector<uint32_t>                         cFcmdRegVals{cTriggerSource, 0}; // fNevents};
        std::vector<uint32_t>                         cFcmdRegOrigVals(cFcmdRegs.size(), 0);
        std::vector<std::pair<std::string, uint32_t>> cRegVec;
        cRegVec.clear();
        for(size_t cIndx = 0; cIndx < cFcmdRegs.size(); cIndx++)
        {
            std::string cRegName    = "fc7_daq_cnfg.fast_command_block." + cFcmdRegs[cIndx];
            cFcmdRegOrigVals[cIndx] = fBeBoardInterface->ReadBoardReg(cBoard, cRegName);
            cRegVec.push_back({cRegName, cFcmdRegVals[cIndx]});
        }
        cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity", 0});
        cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
        // enable DIO5
        LOG(INFO) << "\tTrigger source: 5" << RESET;
        LOG(INFO) << "\tEnable DIO5 Block" << RESET;
        LOG(INFO) << "\tDisable output on channel 2" << RESET;
        LOG(INFO) << "\tEnable termination on channel 2" << RESET;
        LOG(INFO) << "\tSet threshold on channel to 0" << RESET;

        cRegVec.push_back({"fc7_daq_cnfg.readout_block.global.data_handshake_enable", 0x0});
        cRegVec.push_back({"fc7_daq_cnfg.dio5_block.dio5_en", 0x1});
        cRegVec.push_back({"fc7_daq_cnfg.dio5_block.ch2.out_enable", 0});
        // cRegVec.push_back({"fc7_daq_cnfg.dio5_block.ch2.term_enable", 1});
        cRegVec.push_back({"fc7_daq_cnfg.dio5_block.ch2.threshold", 0});

        fBeBoardInterface->WriteBoardMultReg(cBoard, cRegVec);
        fBeBoardInterface->WriteBoardReg(cBoard, "fc7_daq_cnfg.tlu_block.tlu_enabled", 0);

        // stop triggers
        fBeBoardInterface->Stop(cBoard);
        // send a ReSync
        fBeBoardInterface->ChipReSync(cBoard);
    }
}

// State machine control functions

void ExtTriggerLatencyScan::ConfigureCalibration() {}

void ExtTriggerLatencyScan::Running()
{
    LOG(INFO) << "Starting Latency Scan using external triggers";
    Initialize();
    InitializeExternalTriggers();
    ScanLatency();
    LOG(INFO) << "Done with Latency Scan using external triggers";
}

void ExtTriggerLatencyScan::Stop()
{
    LOG(INFO) << "Stopping Latency Scan using external triggers.";
    writeObjects();
    dumpConfigFiles();
    closeFileHandler();
    LOG(INFO) << "Latency Scan stopped using external triggers.";
}

void ExtTriggerLatencyScan::Pause() {}

void ExtTriggerLatencyScan::Resume() {}