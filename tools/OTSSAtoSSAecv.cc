#include "tools/OTSSAtoSSAecv.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/SSA2Interface.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTSSAtoSSAecv::fCalibrationDescription = "Scan phase and strenght on the SSA to SSA lines";

OTSSAtoSSAecv::OTSSAtoSSAecv() : OTSSAtoMPAecv() {}

OTSSAtoSSAecv::~OTSSAtoSSAecv() {}

void OTSSAtoSSAecv::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    // free the registers in case any
    fNumberOfStubBits      = findValueInSettings<double>("OTSSAtoSSAecv_NumberOfTestedStubBits", 1e6);
    fListOfSSAslvsCurrents = convertStringToFloatList(findValueInSettings<std::string>("OTSSAtoSSAecv_ListOfSSAslvsCurrents", "1, 4, 7"));

    ContainerFactory::copyAndInitHybrid<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 2, 2>>(*fDetectorContainer, fPatternMatchingEfficiencyContainer);

    setUpPatternMatching();

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTSSAtoSSAecv.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTSSAtoSSAecv::ConfigureCalibration() {}

void OTSSAtoSSAecv::Running()
{
    LOG(INFO) << "Starting OTSSAtoSSAecv measurement.";
    Initialise();
    runSSAtoSSAecvScan();
    LOG(INFO) << "Done with OTSSAtoSSAecv.";
    Reset();
}

void OTSSAtoSSAecv::Stop(void)
{
    LOG(INFO) << "Stopping OTSSAtoSSAecv measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTSSAtoSSAecv.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTSSAtoSSAecv stopped.";
}

void OTSSAtoSSAecv::Pause() {}

void OTSSAtoSSAecv::Resume() {}

void OTSSAtoSSAecv::Reset() { fRegisterHelper->restoreSnapshot(); }

void OTSSAtoSSAecv::runSSAtoSSAecvScan()
{
    auto        selectSSAfunction     = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::SSA2); };
    std::string selectSSAfunctionName = "SelectSSAfunction";
    fDetectorContainer->addReadoutChipQueryFunction(selectSSAfunction, selectSSAfunctionName);
    ContainerFactory::copyAndInitChip<uint8_t>(*fDetectorContainer, fOriginalPhaseContainer);

    // Reading original phases
    for(auto theBoard: *fDetectorContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                for(auto theChip: *theHybrid)
                {
                    auto& theOriginalPhase = fOriginalPhaseContainer.getChip(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId(), theChip->getId())->getSummary<uint8_t>();
                    theOriginalPhase       = fReadoutChipInterface->ReadChipReg(theChip, "LateralRX_sampling");
                }
            }
        }
    }

    fDetectorContainer->removeReadoutChipQueryFunction(selectSSAfunctionName);

    for(uint8_t slvsCurrent: fListOfSSAslvsCurrents)
    {
        LOG(INFO) << BOLDGREEN << "Scanning SSA SLVS current = " << +slvsCurrent << RESET;
        for(auto theBoard: *fDetectorContainer)
        {
            for(auto theOpticalGroup: *theBoard)
            {
                for(auto theHybrid: *theOpticalGroup)
                {
                    for(auto theChip: *theHybrid)
                    {
                        if(theChip->getFrontEndType() == FrontEndType::SSA2)
                        {
                            static_cast<PSInterface*>(fReadoutChipInterface)
                                ->fTheSSA2Interface->WriteChipRegBits(theChip, "SLVS_pad_current_Lateral", slvsCurrent | (slvsCurrent << 3), "mask_peri_D", 0x3F);
                        }
                    }
                }
            }
        }

        for(uint8_t clockEdge = 0; clockEdge < 2; ++clockEdge)
        {
            for(int samplingPhaseOffset = fMinimum320PhaseShift; samplingPhaseOffset <= fMaximum320PhaseShift; ++samplingPhaseOffset)
            {
                LOG(INFO) << BOLDGREEN << "SSA sampling clockEdge = " << +clockEdge << " and sampling offset = " << samplingPhaseOffset << RESET;

                resetPatternMatchingEfficiencyContainer();

                for(auto theBoard: *fDetectorContainer)
                {
                    setSampleClockEdgeAndPhase(theBoard, clockEdge, samplingPhaseOffset);
                    auto theFWInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard));
                    runStubIntegrityTest(theBoard, theFWInterface);
                }

#ifdef __USE_ROOT__
                fDQMHistogramOTSSAtoSSAecv.fillStubPatternEfficiencyScan(fPatternMatchingEfficiencyContainer, clockEdge, slvsCurrent, samplingPhaseOffset);
#else
                if(fDQMStreamerEnabled)
                {
                    ContainerSerialization thePatternMatchingEfficiencyContainerSerialization("OTSSAtoSSAecvStubPatternMatchingEfficiency");
                    thePatternMatchingEfficiencyContainerSerialization.streamByHybridContainer(fDQMStreamer, fPatternMatchingEfficiencyContainer, clockEdge, slvsCurrent, samplingPhaseOffset);
                }
#endif
            }
        }
    }
}

void OTSSAtoSSAecv::setSampleClockEdgeAndPhase(Ph2_HwDescription::BeBoard* theBoard, uint8_t clockEdge, int samplingPhaseOffset)
{
    auto calculateNewPhase = [clockEdge, samplingPhaseOffset](uint8_t originalClockEdgeAndPhase) -> uint8_t
    {
        uint8_t originalPhase = originalClockEdgeAndPhase & 0x7;
        uint8_t newPhase      = originalClockEdgeAndPhase + samplingPhaseOffset;
        if(originalPhase == 0 && samplingPhaseOffset < 0) newPhase = 0x8 - samplingPhaseOffset;
        if(originalPhase == 0x7 && samplingPhaseOffset > 0) newPhase = 0x0 + samplingPhaseOffset - 1;

        return ((clockEdge & 0x1) << 3) | (newPhase & 0x7);
    };

    for(auto theOpticalGroup: *theBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            for(auto theChip: *theHybrid)
            {
                if(theChip->getFrontEndType() == FrontEndType::SSA2)
                {
                    auto theOriginalPhase = fOriginalPhaseContainer.getChip(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId(), theChip->getId())->getSummary<uint8_t>();

                    auto theNewPhaseLeft  = calculateNewPhase(theOriginalPhase & 0xf);
                    auto theNewPhaseRight = calculateNewPhase((theOriginalPhase >> 4) & 0xf);

                    fReadoutChipInterface->WriteChipReg(theChip, "LateralRX_sampling", (theNewPhaseRight << 4) | theNewPhaseLeft);
                }
            }
        }
    }
}

std::vector<Cluster> OTSSAtoSSAecv::produceStripClusterList()
{
    std::vector<Cluster> listOfInjectedStrips;

    listOfInjectedStrips.push_back(Cluster(0, fStripClusterColLeftToRight, 1));
    listOfInjectedStrips.push_back(Cluster(0, fStripClusterColRightToLeft, 1));

    return listOfInjectedStrips;
}

std::vector<std::vector<Stub>> OTSSAtoSSAecv::createPSstubList()
{
    std::vector<std::vector<Stub>> theListOfStubInjections;

    std::vector<Stub> theSubListRightToLeft{Stub(fPixelClusterColRightToLeft * 2, fBendingToCode.at(0), 0xf)};
    theListOfStubInjections.push_back(theSubListRightToLeft);

    std::vector<Stub> theSubListLeftToRight{Stub(fPixelClusterColLeftToRight * 2, fBendingToCode.at(0), 0xf)};
    theListOfStubInjections.push_back(theSubListLeftToRight);

    return theListOfStubInjections;
}

void OTSSAtoSSAecv::setStripOffsetParameters(Ph2_HwDescription::ReadoutChip* theSSA)
{
    uint8_t stripOffsetLeftToRight = (NSSACHANNELS + fPixelClusterColLeftToRight - fStripClusterColLeftToRight) * 2;
    uint8_t stripOffsetRightToLeft = stripOffsetLeftToRight | 0x10;

    std::vector<std::pair<std::string, uint16_t>> stripOffsetRegisters{
        {"StripOffset_byte0", stripOffsetLeftToRight << 3}, {"StripOffset_byte1", 0}, {"StripOffset_byte2", 0}, {"StripOffset_byte3", stripOffsetRightToLeft << 2}};

    fReadoutChipInterface->WriteChipMultReg(theSSA, stripOffsetRegisters);
}

GenericDataArray<float, 2>& OTSSAtoSSAecv::getStorageForStubErrorRate(Hybrid* theHybrid, uint8_t chipId, uint8_t line, size_t stubPatternCounter)
{
    return fPatternMatchingEfficiencyContainer.getHybrid(theHybrid->getBeBoardId(), theHybrid->getOpticalGroupId(), theHybrid->getId())
        ->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 2, 2>>()
        .at(chipId)
        .at(stubPatternCounter);
}

void OTSSAtoSSAecv::resetPatternMatchingEfficiencyContainer()
{
    for(auto theBoard: fPatternMatchingEfficiencyContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup) { theHybrid->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 2, 2>>() = GenericDataArray<float, NUMBER_OF_CIC_PORTS, 2, 2>(); }
        }
    }
}