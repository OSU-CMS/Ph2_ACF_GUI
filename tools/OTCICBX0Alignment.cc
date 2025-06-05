#include "tools/OTCICBX0Alignment.h"
#include "HWDescription/BeBoard.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/ExceptionHandler.h"
#include "HWInterface/FastCommandInterface.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTCICBX0Alignment::fCalibrationDescription = "Perform the BX0 alignment between FE chips and CIC. Should be done after the word alignment.";

OTCICBX0Alignment::OTCICBX0Alignment() : Tool() {}

OTCICBX0Alignment::~OTCICBX0Alignment() {}

void OTCICBX0Alignment::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    fRegisterHelper->freeFrontEndRegister(FrontEndType::CIC2, "^BX0_DELAY$");
    fRegisterHelper->freeFrontEndRegister(FrontEndType::CIC2, "^EXT_BX0_DELAY$");
    fRegisterHelper->freeFrontEndRegister(FrontEndType::CIC2, "^BX0_ALIGN_CONFIG$");

#ifdef __USE_ROOT__ // to disable and anable ROOT by command
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTCICBX0Alignment.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTCICBX0Alignment::ConfigureCalibration() {}

void OTCICBX0Alignment::Running()
{
    LOG(INFO) << BOLDMAGENTA << "Starting OTCICBX0Alignment measurement." << RESET;
    Initialise();
    // FIXME the retime pix scan is temporary, only to verify uniformity across modules
    if(fDetectorContainer->getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS) ScanRetimePixAndBX0Alignment();
    BX0Alignment();
    LOG(INFO) << BOLDGREEN << "Done with OTCICBX0Alignment." << RESET;
    Reset();
}

void OTCICBX0Alignment::Stop(void)
{
    LOG(INFO) << "Stopping OTCICBX0Alignment measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTCICBX0Alignment.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTCICBX0Alignment stopped.";
}

void OTCICBX0Alignment::Pause() {}

void OTCICBX0Alignment::Resume() {}

void OTCICBX0Alignment::Reset() { fRegisterHelper->restoreSnapshot(); }

void OTCICBX0Alignment::BX0Alignment()
{
    LOG(INFO) << BOLDMAGENTA << "Starting CIC automated BX0 alignment procedure .... " << RESET;
    std::string theQueryFunction = "skipSSAQuery";
    auto        theSkipSSAquery  = [](const ChipContainer* theReadoutChip)
    {
        if(static_cast<const ReadoutChip*>(theReadoutChip)->getFrontEndType() == FrontEndType::SSA2) return false;
        return true;
    };
    fDetectorContainer->addReadoutChipQueryFunction(theSkipSSAquery, theQueryFunction);

    DetectorDataContainer theBX0AlignmentDelayContainer;
    ContainerFactory::copyAndInitHybrid<uint16_t>(*fDetectorContainer, theBX0AlignmentDelayContainer);

    for(auto theBoard: *fDetectorContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                // Making sure to use an enabled FE
                auto theFEtoUse = theHybrid->getFirstObject(); //  *(theHybrid->begin());
                if(theFEtoUse == nullptr)
                {
                    LOG(INFO) << BOLDRED << "No FE suitable for BX0 alignment enabled on Board id " << +cCic->getBeBoardId() << " OpticalGroup id" << +cCic->getOpticalGroupId() << " Hybrid id "
                              << +cCic->getHybridId() << " --- Hybrid will be disabled" << RESET;
                    ExceptionHandler::getInstance()->disableHybrid(cCic->getBeBoardId(), cCic->getOpticalGroupId(), cCic->getHybridId());
                    return;
                }
                // std::cout << " the FE to use is " << +theFEtoUse->getId() << std::endl;
                auto    theFECICmapping = cCic->getMapping();
                uint8_t theIndex        = (theFEtoUse->getFrontEndType() == FrontEndType::MPA2) ? theFEtoUse->getId() - 8 : theFEtoUse->getId();
                uint8_t theFEId         = theFECICmapping.at(theIndex);
                // configure word alignment pattern on FEs
                std::vector<uint8_t> cAlignmentPatterns = fReadoutChipInterface->getBX0AlignmentPatterns();
                uint8_t              pLine              = 0;
                if(theOpticalGroup->getFrontEndType() == FrontEndType::OuterTracker2S) pLine = 4;
                fCicInterface->PrepareForAutomatedBX0Alignment(cCic, cAlignmentPatterns, pLine, theFEId);
                for(uint8_t cIndex = 0; cIndex < (uint8_t)cAlignmentPatterns.size(); cIndex += 1)
                {
                    LOG(INFO) << BOLDBLUE << "Calibration pattern set on readout chip on stub line " << +cIndex << " set to " << std::bitset<8>(cAlignmentPatterns.at(cIndex)) << RESET;
                }

                // std::cout << " BXO alignment pattern for chip " << +theFEtoUse->getId() << std::endl;
                fReadoutChipInterface->produceBX0AlignmentPattern(theFEtoUse);
            } // hybrids
        } // optical group

        // Send Resync to all hybrids connected to one board at once
        LOG(INFO) << BOLDMAGENTA << " Sending Resync !" << RESET;
        auto    cInterface            = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard));
        auto    cFastCommandInterface = cInterface->getFastCommandInterface();
        uint8_t theNumberOfResyncs    = 5;
        cFastCommandInterface->SendGlobalCounterResetResync(theNumberOfResyncs);
        // fBeBoardInterface->ChipReSync(theBoard);
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto& cCic          = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                bool  cSuccessAlign = fCicInterface->CheckAutomatedBX0Alignment(cCic);

                auto& theBX0AlignmentValue = theBX0AlignmentDelayContainer.getObject(theBoard->getId())->getObject(theOpticalGroup->getId())->getObject(theHybrid->getId())->getSummary<uint16_t>();
                theBX0AlignmentValue       = fCicInterface->retrieveExternalBX0AlignmentValue(cCic);
                ++theBX0AlignmentValue; // increased by one bases on the experience with multuple PS, but root cause not understood
                cSuccessAlign = cSuccessAlign && fCicInterface->ConfigureExternalBX0Delay(cCic, theBX0AlignmentValue);
                if(cSuccessAlign) { LOG(INFO) << BOLDBLUE << "Automated BX0 alignment procedure " << BOLDGREEN << " SUCCEEDED!" << RESET; }
                else
                {
                    LOG(INFO) << BOLDRED << "Automated BX0 alignment procedure " << BOLDRED << " FAILED!" << RESET;
                    LOG(INFO) << BOLDRED << "FAILED CIC BX0 alignment word on Board id " << +theBoard->getId() << " OpticalGroup id" << +theOpticalGroup->getId() << " Hybrid id" << +theHybrid->getId()
                              << " --- Hybrid will be disabled" << RESET;
                    ExceptionHandler::getInstance()->disableHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId());
                    continue;
                }
            } // hybrids
        } // optical group
    }

#ifdef __USE_ROOT__
    fDQMHistogramOTCICBX0Alignment.fillBX0AlignmentDelay(theBX0AlignmentDelayContainer);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theBX0AlignmentDelayContainerSerialization("OTCICBX0AlignmentBX0AlignmentDelay");
        theBX0AlignmentDelayContainerSerialization.streamByOpticalGroupContainer(fDQMStreamer, theBX0AlignmentDelayContainer);
    }
#endif

    fDetectorContainer->removeReadoutChipQueryFunction(theQueryFunction);
}

// FIXME the retime pix scan is temporary, only to verify uniformity across modules
void OTCICBX0Alignment::ScanRetimePixAndBX0Alignment()
{
    LOG(INFO) << BOLDMAGENTA << "Starting CIC automated BX0 alignment procedure for different retime pix values .... " << RESET;
    std::string theQueryFunction = "skipSSAQuery";
    auto        theSkipSSAquery  = [](const ChipContainer* theReadoutChip)
    {
        if(static_cast<const ReadoutChip*>(theReadoutChip)->getFrontEndType() == FrontEndType::SSA2) return false;
        return true;
    };
    fDetectorContainer->addReadoutChipQueryFunction(theSkipSSAquery, theQueryFunction);

    DetectorDataContainer theBX0AlignmentDelayVsRetimePixContainer;
    size_t                theRetimePixValues = 8;
    std::vector<uint16_t> initialEmptyVector(theRetimePixValues, 0);
    ContainerFactory::copyAndInitHybrid<std::vector<uint16_t>>(*fDetectorContainer, theBX0AlignmentDelayVsRetimePixContainer, initialEmptyVector);
    // auto& theBX0AlignmentValue = theBX0AlignmentDelayVsRetimePixContainer.getObject(theBoard->getId())
    //                                                    ->getObject(theOpticalGroup->getId())
    //                                                    ->getObject(theHybrid->getId())
    //                                                    ->getSummary<std::vector<uint16_t>>();

    for(auto theBoard: *fDetectorContainer)
    {
        for(uint8_t theRetimePix = 0; theRetimePix < theRetimePixValues; theRetimePix++)
        {
            for(auto theOpticalGroup: *theBoard)
            {
                for(auto theHybrid: *theOpticalGroup)
                {
                    auto& cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                    // Making sure to use an enabled FE
                    auto theFEtoUse = theHybrid->getFirstObject(); //  *(theHybrid->begin());
                    if(theFEtoUse == nullptr)
                    {
                        LOG(INFO) << BOLDRED << "No FE suitable for BX0 alignment enabled on Board id " << +cCic->getBeBoardId() << " OpticalGroup id" << +cCic->getOpticalGroupId() << " Hybrid id "
                                  << +cCic->getHybridId() << " --- Hybrid will be disabled" << RESET;
                        ExceptionHandler::getInstance()->disableHybrid(cCic->getBeBoardId(), cCic->getOpticalGroupId(), cCic->getHybridId());
                        return;
                    }
                    // std::cout << " the FE to use is " << +theFEtoUse->getId() << std::endl;
                    auto    theFECICmapping = cCic->getMapping();
                    uint8_t theIndex        = (theFEtoUse->getFrontEndType() == FrontEndType::MPA2) ? theFEtoUse->getId() - 8 : theFEtoUse->getId();
                    uint8_t theFEId         = theFECICmapping.at(theIndex);
                    // configure word alignment pattern on FEs
                    std::vector<uint8_t> cAlignmentPatterns = fReadoutChipInterface->getBX0AlignmentPatterns();
                    uint8_t              pLine              = 0;
                    if(theOpticalGroup->getFrontEndType() == FrontEndType::OuterTracker2S) pLine = 4;
                    fCicInterface->PrepareForAutomatedBX0Alignment(cCic, cAlignmentPatterns, pLine, theFEId);
                    for(uint8_t cIndex = 0; cIndex < (uint8_t)cAlignmentPatterns.size(); cIndex += 1)
                    {
                        LOG(INFO) << BOLDBLUE << "Calibration pattern set on readout chip on stub line " << +cIndex << " set to " << std::bitset<8>(cAlignmentPatterns.at(cIndex)) << RESET;
                    }

                    LOG(INFO) << BOLDMAGENTA << " BXO alignment pattern for hybrid " << +theHybrid->getId() << " on chip " << +theFEtoUse->getId() << " with retime pix " << +theRetimePix << RESET;
                    fReadoutChipInterface->WriteChipReg(theFEtoUse, "RetimePix", theRetimePix);
                    // auto retimepix = fReadoutChipInterface->ReadChipReg(theFEtoUse,"RetimePix");
                    // std::cout << " wrote retime pix " << retimepix << std::endl;
                    fReadoutChipInterface->produceBX0AlignmentPattern(theFEtoUse);
                } // hybrids
            } // optical group

            // Send Resync to all hybrids connected to one board at once
            LOG(INFO) << BOLDMAGENTA << " Sending Resync !" << RESET;
            auto    cInterface            = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard));
            auto    cFastCommandInterface = cInterface->getFastCommandInterface();
            uint8_t theNumberOfResyncs    = 5;
            cFastCommandInterface->SendGlobalCounterResetResync(theNumberOfResyncs);
            // fBeBoardInterface->ChipReSync(theBoard);
            for(auto theOpticalGroup: *theBoard)
            {
                for(auto theHybrid: *theOpticalGroup)
                {
                    auto& cCic          = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                    bool  cSuccessAlign = fCicInterface->CheckAutomatedBX0Alignment(cCic);
                    auto& theBX0AlignmentValues =
                        theBX0AlignmentDelayVsRetimePixContainer.getObject(theBoard->getId())->getObject(theOpticalGroup->getId())->getObject(theHybrid->getId())->getSummary<std::vector<uint16_t>>();

                    // bool  cSuccessAlign          = fCicInterface->AutomatedBX0Alignment(cCic, cAlignmentPatterns);
                    theBX0AlignmentValues.at(theRetimePix) = fCicInterface->retrieveExternalBX0AlignmentValue(cCic);
                    // std::cout << " theBX0AlignmentValues.at(theRetimePix) " << theBX0AlignmentValues.at(theRetimePix) << std::endl;
                    cSuccessAlign = cSuccessAlign && fCicInterface->ConfigureExternalBX0Delay(cCic, theBX0AlignmentValues.at(theRetimePix));
                    if(cSuccessAlign) { LOG(INFO) << BOLDBLUE << "Automated BX0 alignment procedure " << BOLDGREEN << " SUCCEEDED!" << RESET; }
                    else
                    {
                        LOG(INFO) << BOLDRED << "Automated BX0 alignment procedure " << BOLDRED << " FAILED!" << RESET;
                        LOG(INFO) << BOLDRED << "FAILED CIC BX0 alignment word on Board id " << +theBoard->getId() << " OpticalGroup id" << +theOpticalGroup->getId() << " Hybrid id"
                                  << +theHybrid->getId() << " --- Hybrid will be disabled" << RESET;
                        ExceptionHandler::getInstance()->disableHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId());
                        continue;
                    }

                    // Set the again default RetimePix Value of 4
                    auto theFEtoUse = theHybrid->getFirstObject(); //  *(theHybrid->begin());
                    if(theFEtoUse == nullptr)
                    {
                        LOG(INFO) << BOLDRED << "No FE suitable for BX0 alignment enabled on Board id " << +cCic->getBeBoardId() << " OpticalGroup id" << +cCic->getOpticalGroupId() << " Hybrid id "
                                  << +cCic->getHybridId() << " --- Hybrid will be disabled" << RESET;
                        ExceptionHandler::getInstance()->disableHybrid(cCic->getBeBoardId(), cCic->getOpticalGroupId(), cCic->getHybridId());
                        return;
                    }
                    // std::cout << __LINE__ << " the FE to use is " << +theFEtoUse->getId() << std::endl;
                    fReadoutChipInterface->WriteChipReg(theFEtoUse, "RetimePix", 4);

                } // hybrids
            } // optical group
        }

#ifdef __USE_ROOT__
        fDQMHistogramOTCICBX0Alignment.fillBX0AlignmentDelayVsRetimePix(theBX0AlignmentDelayVsRetimePixContainer);
#else
        if(fDQMStreamerEnabled)
        {
            ContainerSerialization theBX0AlignmentDelayVsRetimePixContainerSerialization("OTCICBX0AlignmentBX0AlignmentDelayVsRetimePix");
            theBX0AlignmentDelayVsRetimePixContainerSerialization.streamByOpticalGroupContainer(fDQMStreamer, theBX0AlignmentDelayVsRetimePixContainer);
        }
#endif

        fDetectorContainer->removeReadoutChipQueryFunction(theQueryFunction);
    }
}