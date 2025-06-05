#include "tools/OTCICwordAlignment.h"
#include "HWInterface/ExceptionHandler.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTCICwordAlignment::fCalibrationDescription = "Run the CIC automatic word alignment procedure to properly decode CBC/MPA stub lines";

OTCICwordAlignment::OTCICwordAlignment() : Tool() {}

OTCICwordAlignment::~OTCICwordAlignment() {}

void OTCICwordAlignment::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    fRegisterHelper->freeFrontEndRegister(FrontEndType::CIC2, "EXT_WA_DELAY[0-1][0-9]");

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTCICwordAlignment.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTCICwordAlignment::ConfigureCalibration() {}

void OTCICwordAlignment::Running()
{
    LOG(INFO) << "Starting OTCICwordAlignment measurement.";
    Initialise();
    WordAlignment();
    LOG(INFO) << "Done with OTCICwordAlignment.";
    Reset();
}

void OTCICwordAlignment::Stop(void)
{
    LOG(INFO) << "Stopping OTCICwordAlignment measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTCICwordAlignment.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTCICwordAlignment stopped.";
}

void OTCICwordAlignment::Pause() {}

void OTCICwordAlignment::Resume() {}

void OTCICwordAlignment::Reset()
{
    fRegisterHelper->restoreSnapshot();

    // TODO: this is a temporary test to read events at end of the alignment procedure.
    fRegisterHelper->takeSnapshot();
    LOG(INFO) << BOLDMAGENTA << " Trying to read events at the end of the alignment procedure" << RESET;
    for(auto theBoard: *fDetectorContainer)
    {
        fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.global.hybrid_enable", fEnableMask);
        std::vector<std::pair<std::string, uint32_t>> cRegVec;
        cRegVec.clear();
        cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", 3});
        cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
        cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity", 0});
        cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse", 300});
        cRegVec.push_back({"fc7_daq_cnfg.readout_block.global.common_stubdata_delay", 217});
        cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.test_pulse.en_fast_reset", 1});
        cRegVec.push_back({"fc7_daq_cnfg.tlu_block.tlu_enabled", 0});
        fBeBoardInterface->WriteBoardMultReg(theBoard, cRegVec);
        // first lets figure out how many hybrids are enabled
        auto cEnableMask = fBeBoardInterface->ReadBoardReg(theBoard, "fc7_daq_cnfg.global.hybrid_enable");
        fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.global.hybrid_enable", cEnableMask);
        // reconfigure sparsification + FEs enabled in this CIC
        bool cSparsified = theBoard->getSparsification();
        fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.physical_interface_block.cic.2s_sparsified_enable", (int)cSparsified);

        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                fCicInterface->WriteChipReg(cCic, "FE_ENABLE", 0);
                std::vector<uint8_t> listOfEnabledChips;
                for(auto theChip: *theHybrid) { listOfEnabledChips.push_back(theChip->getId() % 8); }
                fCicInterface->EnableFEs(cCic, listOfEnabledChips, true);
            }
        }

        // and check
        ReadNEvents(theBoard, 10);
        const std::vector<Event*>& cEvents = this->GetEvents();
        for(auto& cEvent: cEvents)
        {
            for(auto cOpticalGroup: *theBoard)
            {
                for(auto cHybrid: *cOpticalGroup)
                {
                    auto cBx = (int)cEvent->BxId(cHybrid->getId());
                    LOG(DEBUG) << BOLDGREEN << "Link#" << +cOpticalGroup->getId() << " Hybrid#" << +cHybrid->getId() << " BxId " << cBx << RESET;
                }
            }
        }
        LOG(INFO) << BOLDMAGENTA << "Done reading events" << RESET;
    }
    fRegisterHelper->restoreSnapshot();
}

void OTCICwordAlignment::WordAlignment(uint32_t pWait_us)
{
    LOG(INFO) << BOLDBLUE << "Starting CIC automated word alignment procedure .... " << RESET;
    std::string theQueryFunction = "skipSSAQuery";
    auto        theSkipSSAquery  = [](const ChipContainer* theReadoutChip)
    {
        if(static_cast<const ReadoutChip*>(theReadoutChip)->getFrontEndType() == FrontEndType::SSA2) return false;
        return true;
    };
    fDetectorContainer->addReadoutChipQueryFunction(theSkipSSAquery, theQueryFunction);

    DetectorDataContainer theWordAlignmentDelayContainer;
    ContainerFactory::copyAndInitHybrid<GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS - 1>>(*fDetectorContainer, theWordAlignmentDelayContainer);

    for(auto theBoard: *fDetectorContainer)
    {
        fEnableMask = fBeBoardInterface->ReadBoardReg(theBoard, "fc7_daq_cnfg.global.hybrid_enable");
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                fFeEnableRegs.push_back(fCicInterface->ReadChipReg(cCic, "FE_ENABLE"));
                // configure word alignment pattern on FEs
                std::vector<uint8_t> cAlignmentPatterns = fReadoutChipInterface->getWordAlignmentPatterns();
                for(auto cChip: *theHybrid) { fReadoutChipInterface->produceWordAlignmentPattern(cChip); }
                bool  cSuccessAlign          = fCicInterface->AutomatedWordAlignment(cCic, cAlignmentPatterns);
                auto& theWordAlignmentValues = theWordAlignmentDelayContainer.getObject(theBoard->getId())
                                                   ->getObject(theOpticalGroup->getId())
                                                   ->getObject(theHybrid->getId())
                                                   ->getSummary<GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS - 1>>();
                theWordAlignmentValues = fCicInterface->retrieveExternalWordAlignmentValues(cCic);
                cSuccessAlign          = cSuccessAlign && fCicInterface->ConfigureExternalWordAlignment(cCic, theWordAlignmentValues);
                if(cSuccessAlign) { LOG(INFO) << BOLDBLUE << "Automated word alignment procedure " << BOLDGREEN << " SUCCEEDED!" << RESET; }
                else
                {
                    LOG(INFO) << BOLDRED << "Automated word alignment procedure " << BOLDRED << " FAILED!" << RESET;
                    LOG(INFO) << BOLDRED << "FAILED CIC word alignment word on Board id " << +theBoard->getId() << " OpticalGroup id" << +theOpticalGroup->getId() << " Hybrid id"
                              << +theHybrid->getId() << " --- Hybrid will be disabled" << RESET;
                    ExceptionHandler::getInstance()->disableHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId());
                    continue;
                }
                fCicInterface->SetStaticWordAlignment(cCic);
            } // hybrid - configure word alignment patterns
        }
    }

#ifdef __USE_ROOT__
    fDQMHistogramOTCICwordAlignment.fillWordAlignmentDelay(theWordAlignmentDelayContainer);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theWordAlignmentDelayContainerSerialization("OTCICwordAlignmentWordAlignmentDelay");
        theWordAlignmentDelayContainerSerialization.streamByOpticalGroupContainer(fDQMStreamer, theWordAlignmentDelayContainer);
    }
#endif

    fDetectorContainer->removeReadoutChipQueryFunction(theQueryFunction);
}
