#include "tools/OTSSAtoMPAecv.h"
#include "HWDescription/ReadoutChip.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/MPA2Interface.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"
#include "Utils/PatternMatcher.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTSSAtoMPAecv::fCalibrationDescription = "Scan phase and strenght on the SSA to MPA lines";

OTSSAtoMPAecv::OTSSAtoMPAecv() : OTverifyMPASSAdataWord() {}

OTSSAtoMPAecv::~OTSSAtoMPAecv() {}

void OTSSAtoMPAecv::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    // free the registers in case any

    fFirstStrip = 6;
    fStripGap   = 6;
    // free the registers in case any
    fNumberOfStubBits      = findValueInSettings<double>("OTSSAtoMPAecv_NumberOfTestedStubBits", 1e8);
    fNumberOfL1Bits        = findValueInSettings<double>("OTSSAtoMPAecv_NumberOfTestedL1Bits", 1e6);
    fListOfSSAslvsCurrents = convertStringToFloatList(findValueInSettings<std::string>("OTSSAtoMPAecv_ListOfSSAslvsCurrents", "1, 4, 7"));

    ContainerFactory::copyAndInitHybrid<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 9, 2>>(*fDetectorContainer, fPatternMatchingEfficiencyContainer);

    setUpPatternMatching();
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTSSAtoMPAecv.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTSSAtoMPAecv::ConfigureCalibration() {}

void OTSSAtoMPAecv::Running()
{
    LOG(INFO) << "Starting OTSSAtoMPAecv measurement.";
    Initialise();
    runSSAtoMPAecvScan();
    LOG(INFO) << "Done with OTSSAtoMPAecv.";
    Reset();
}

void OTSSAtoMPAecv::Stop(void)
{
    LOG(INFO) << "Stopping OTSSAtoMPAecv measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTSSAtoMPAecv.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTSSAtoMPAecv stopped.";
}

void OTSSAtoMPAecv::Pause() {}

void OTSSAtoMPAecv::Resume() {}

void OTSSAtoMPAecv::Reset() { fRegisterHelper->restoreSnapshot(); }

void OTSSAtoMPAecv::runSSAtoMPAecvScan()
{
    auto        selectMPAfunction     = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::MPA2); };
    std::string selectMPAfunctionName = "SelectMPAfunction";
    fDetectorContainer->addReadoutChipQueryFunction(selectMPAfunction, selectMPAfunctionName);
    ContainerFactory::copyAndInitChip<std::pair<uint8_t, uint8_t>>(*fDetectorContainer, fOriginalL1PhaseContainer);
    ContainerFactory::copyAndInitChip<uint8_t>(*fDetectorContainer, fOriginalStubPhaseContainer);

    // Reading original phases
    for(auto theBoard: *fDetectorContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                for(auto theChip: *theHybrid)
                {
                    auto& theOriginalL1PhasePair =
                        fOriginalL1PhaseContainer.getChip(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId(), theChip->getId())->getSummary<std::pair<uint8_t, uint8_t>>();
                    auto&   theOriginalStubPhasePair = fOriginalStubPhaseContainer.getChip(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId(), theChip->getId())->getSummary<uint8_t>();
                    uint8_t phase320registerValue    = fReadoutChipInterface->ReadChipReg(theChip, "LatencyRx320");
                    uint8_t phase40registerValue     = fReadoutChipInterface->ReadChipReg(theChip, "LatencyRx40");
                    theOriginalL1PhasePair.first     = (phase320registerValue & 0x7) | (phase40registerValue & 0x3) << 3; // Start phase
                    theOriginalL1PhasePair.second    = (phase320registerValue & 0x7) | (phase40registerValue & 0xC) << 1; // Restart phase
                    theOriginalStubPhasePair         = (phase320registerValue >> 3) & 0x7;                                // Stub phase
                }
            }
        }
    }

    fDetectorContainer->removeReadoutChipQueryFunction(selectMPAfunctionName);
    for(uint8_t slvsCurrent: fListOfSSAslvsCurrents)
    {
        LOG(INFO) << BOLDGREEN << "SSA SLVS current = " << +slvsCurrent << RESET;
        auto        SSAqueryFunction          = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::SSA2); };
        std::string theSSAqueryFunctionString = "SSAqueryFunction";
        // settings for SSAs
        fDetectorContainer->addReadoutChipQueryFunction(SSAqueryFunction, theSSAqueryFunctionString);
        setSameDac("SLVS_pad_current_L1", slvsCurrent);
        setSameDac("SLVS_pad_current_Stub_0_1", slvsCurrent | (slvsCurrent << 3));
        setSameDac("SLVS_pad_current_Stub_2_3", slvsCurrent | (slvsCurrent << 3));
        setSameDac("SLVS_pad_current_Stub_4_5", slvsCurrent | (slvsCurrent << 3));
        setSameDac("SLVS_pad_current_Stub_6_7", slvsCurrent | (slvsCurrent << 3));
        fDetectorContainer->removeReadoutChipQueryFunction(theSSAqueryFunctionString);
        runSSAtoMPAecvScan(slvsCurrent);
        // runSSAtoMPAecvScanForL1(slvsCurrent);
    }
}

void OTSSAtoMPAecv::runSSAtoMPAecvScan(uint8_t slvsCurrent)
{
    LOG(INFO) << BOLDGREEN << "Scanning SSA to MPA stub phases" << RESET;
    for(uint8_t clockEdge = 0; clockEdge < 2; ++clockEdge)
    {
        for(int samplingPhaseOffset = fMinimum320PhaseShift; samplingPhaseOffset <= fMaximum320PhaseShift; ++samplingPhaseOffset)
        {
            LOG(INFO) << BOLDGREEN << "MPA sampling clockEdge = " << +clockEdge << " and sampling offset = " << samplingPhaseOffset << RESET;

            resetPatternMatchingEfficiencyContainer();

            for(auto theBoard: *fDetectorContainer)
            {
                setSampleClockEdgeAndPhase(theBoard, clockEdge, samplingPhaseOffset);
                auto theFWInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard));
                runStubIntegrityTest(theBoard, theFWInterface);
                runL1IntegrityTest(theBoard, theFWInterface);
            }

#ifdef __USE_ROOT__
            fDQMHistogramOTSSAtoMPAecv.fillPatternEfficiencyScan(fPatternMatchingEfficiencyContainer, clockEdge, slvsCurrent, samplingPhaseOffset);
#else
            if(fDQMStreamerEnabled)
            {
                ContainerSerialization thePatternMatchingEfficiencyContainerSerialization("OTSSAtoMPAecvPatternMatchingEfficiency");
                thePatternMatchingEfficiencyContainerSerialization.streamByHybridContainer(fDQMStreamer, fPatternMatchingEfficiencyContainer, clockEdge, slvsCurrent, samplingPhaseOffset);
            }
#endif
        }
    }
}

void OTSSAtoMPAecv::setSampleClockEdgeAndPhase(Ph2_HwDescription::BeBoard* theBoard, uint8_t clockEdge, int samplingPhaseOffset)
{
    for(auto theOpticalGroup: *theBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            for(auto theChip: *theHybrid)
            {
                if(theChip->getFrontEndType() == FrontEndType::MPA2)
                {
                    auto    theMPAInterface = static_cast<PSInterface*>(fReadoutChipInterface)->fTheMPA2Interface;
                    uint8_t stubRegisterValue;
                    if(clockEdge == 0)
                        stubRegisterValue = 0x00;
                    else
                        stubRegisterValue = 0xFF;
                    theMPAInterface->WriteChipReg(theChip, "EdgeSelTrig", stubRegisterValue);

                    auto&   theOriginalStubPhasePair = fOriginalStubPhaseContainer.getChip(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId(), theChip->getId())->getSummary<uint8_t>();
                    uint8_t theNewStubPhase          = theOriginalStubPhasePair + samplingPhaseOffset;
                    if(theNewStubPhase == 0xff) theNewStubPhase = 0x7;
                    if(theNewStubPhase == 0x8) theNewStubPhase = 0x0;
                    theMPAInterface->WriteChipRegBits(theChip, "LatencyRx320", theNewStubPhase << 3, "Mask", 0x38);

                    uint8_t l1RegisterValue;
                    if(clockEdge == 0)
                        l1RegisterValue = 0x0;
                    else
                        l1RegisterValue = 0x1;
                    theMPAInterface->WriteChipRegBits(theChip, "EdgeSelT1Raw", l1RegisterValue, "Mask", 0x01);
                    auto theOriginalPhasePair =
                        fOriginalL1PhaseContainer.getChip(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId(), theChip->getId())->getSummary<std::pair<uint8_t, uint8_t>>();
                    uint8_t newOffsetStart   = theOriginalPhasePair.first + samplingPhaseOffset;
                    uint8_t newOffsetRestart = theOriginalPhasePair.second + samplingPhaseOffset;
                    if(newOffsetStart > 0x1F || newOffsetStart < (-samplingPhaseOffset) || newOffsetRestart > 0x1F || newOffsetRestart < (-samplingPhaseOffset))
                    {
                        LOG(ERROR) << BOLDYELLOW << "ERROR: impossible to apply samplingPhaseOffset = " << samplingPhaseOffset << " - going out of range" << RESET;
                        continue;
                    }
                    theMPAInterface->WriteChipRegBits(theChip, "LatencyRx320", newOffsetStart & 0x7, "Mask", 0x07);
                    uint8_t phase40LatencyRegister = ((newOffsetStart & 0x18) >> 3) | ((newOffsetRestart & 0x18) >> 1);
                    theMPAInterface->WriteChipRegBits(theChip, "LatencyRx40", phase40LatencyRegister, "Mask", 0x0F);
                }
            }
        }
    }
}

void OTSSAtoMPAecv::resetPatternMatchingEfficiencyContainer()
{
    for(auto theBoard: fPatternMatchingEfficiencyContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup) { theHybrid->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 9, 2>>() = GenericDataArray<float, NUMBER_OF_CIC_PORTS, 9, 2>(); }
        }
    }
}
