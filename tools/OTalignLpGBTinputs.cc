#include "tools/OTalignLpGBTinputs.h"
#include "HWInterface/ExceptionHandler.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/LpGBTalignmentResult.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTalignLpGBTinputs::fCalibrationDescription = "Optimize LpGBT Rx phases to properly decode the inputs from the CICs";

OTalignLpGBTinputs::OTalignLpGBTinputs() : Tool() {}

OTalignLpGBTinputs::~OTalignLpGBTinputs() {}

void OTalignLpGBTinputs::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    fRegisterHelper->freeFrontEndRegister(FrontEndType::LpGBT, "^EPRX\\d{2}ChnCntr$");

    fNumberOfAlignmentIterations = findValueInSettings<double>("OTalignLpGBTinputs_NumberOfAlignmentIterations", 100);
    fMinAlignmentSuccessRate     = findValueInSettings<double>("OTalignLpGBTinputs_MinAlignmentSuccessRate", 0.99);

    for(const auto cBoard: *fDetectorContainer)
    {
        // force trigger source to be internal triggers
        LOG(INFO) << BOLDYELLOW << "Forcing trigger source to internal triggers" << RESET;
        fBeBoardInterface->WriteBoardReg(cBoard, "fc7_daq_cnfg.fast_command_block.trigger_source", 3);
    }

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTalignLpGBTinputs.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTalignLpGBTinputs::ConfigureCalibration() {}

void OTalignLpGBTinputs::AlignLpGBTInputs()
{
    DetectorDataContainer theAlignmentResultContainer;
    ContainerFactory::copyAndInitOpticalGroup<LpGBTalignmentResult>(*fDetectorContainer, theAlignmentResultContainer);
    for(auto theBoard: *fDetectorContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            auto& theOpticalGroupAlignmentResult = theAlignmentResultContainer.getObject(theBoard->getId())->getObject(theOpticalGroup->getId())->getSummary<LpGBTalignmentResult>();
            LOG(INFO) << BOLDYELLOW << "OTalignLpGBTinputs::AlignLpGBTInputs ..." << RESET;
            auto cBoardId   = theOpticalGroup->getBeBoardId();
            auto cBoardIter = std::find_if(fDetectorContainer->begin(), fDetectorContainer->end(), [&cBoardId](Ph2_HwDescription::BeBoard* x) { return x->getId() == cBoardId; });
            // stop triggers to make sure that there are no L1 packets from the CIC
            fBeBoardInterface->Stop((*cBoardIter));

            LOG(INFO) << BOLDMAGENTA << "Aligning CIC-lpGBT data on OpticalGroup#" << +theOpticalGroup->getId() << RESET;
            auto& clpGBT = theOpticalGroup->flpGBT;
            if(clpGBT == nullptr) continue;

            // configure CICs to output alignment pattern on stub lines
            std::vector<uint8_t> cFeEnableRegs(0);
            for(auto cHybrid: *theOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
                // enable alignment output
                fCicInterface->SelectOutput(cCic, true);
                cFeEnableRegs.push_back(fCicInterface->ReadChipReg(cCic, "FE_ENABLE"));
                fCicInterface->EnableFEs(cCic, {0, 1, 2, 3, 4, 5, 6, 7}, false);
            }
            std::map<uint8_t, std::vector<uint8_t>> groupsAndChannels = theOpticalGroup->getLpGBTrxGroupsAndChannels();
            theOpticalGroupAlignmentResult                            = static_cast<D19clpGBTInterface*>(flpGBTInterface)->PhaseAlignRx(clpGBT, groupsAndChannels, fNumberOfAlignmentIterations);
            bool isAligned = static_cast<D19clpGBTInterface*>(flpGBTInterface)->didAlignmentSucceded(theOpticalGroupAlignmentResult, fMinAlignmentSuccessRate, theOpticalGroup);

            if(!isAligned)
            {
                LOG(INFO) << BOLDRED << "FAILED to align LpGBT inputs on Board id " << +theBoard->getId() << " OpticalGroup id" << +theOpticalGroup->getId() << " --- OpticalGroup will be disabled"
                          << RESET;
                ExceptionHandler::getInstance()->disableOpticalGroup(theBoard->getId(), theOpticalGroup->getId());
                continue;
            }
        }
    }

#ifdef __USE_ROOT__
    fDQMHistogramOTalignLpGBTinputs.fillPhaseAlignmentResults(theAlignmentResultContainer);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theAlignmentResultsContainerSerialization("OTalignLpGBTinputsAlignmentResults");
        theAlignmentResultsContainerSerialization.streamByOpticalGroupContainer(fDQMStreamer, theAlignmentResultContainer);
    }
#endif
}

void OTalignLpGBTinputs::Running()
{
    LOG(INFO) << "Starting OTalignLpGBTinputs measurement.";
    Initialise();
    AlignLpGBTInputs();
    LOG(INFO) << "Done with OTalignLpGBTinputs.";
    Reset();
}

void OTalignLpGBTinputs::Stop(void)
{
    LOG(INFO) << "Stopping OTalignLpGBTinputs measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTalignLpGBTinputs.process();
#endif
    LOG(INFO) << "OTalignLpGBTinputs stopped.";
}

void OTalignLpGBTinputs::Pause() {}

void OTalignLpGBTinputs::Resume() {}

void OTalignLpGBTinputs::Reset() { fRegisterHelper->restoreSnapshot(); }
