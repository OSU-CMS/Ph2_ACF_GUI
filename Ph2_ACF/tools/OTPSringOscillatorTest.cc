#include "tools/OTPSringOscillatorTest.h"
#include "HWDescription/Definition.h"
#include "HWDescription/MPA2.h"
#include "HWDescription/SSA2.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTPSringOscillatorTest::fCalibrationDescription = "Insert brief calibration description here";

OTPSringOscillatorTest::OTPSringOscillatorTest() : Tool() {}

OTPSringOscillatorTest::~OTPSringOscillatorTest() {}

void OTPSringOscillatorTest::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    // free the registers in case any
    uint32_t numberOfClockCycles = findValueInSettings<double>("OTPSringOscillatorTest_NumberOfClockCycles", 100);
    if(numberOfClockCycles > 127)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << " OTPSringOscillatorTest_NumberOfClockCycles cannot be greater than 127, throwing exception..." << std::endl;
        throw std::runtime_error("Setting OTPSringOscillatorTest_NumberOfClockCycles out of range");
    }
    fNumberOfClockCycles = numberOfClockCycles;

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTPSringOscillatorTest.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTPSringOscillatorTest::ConfigureCalibration() {}

void OTPSringOscillatorTest::Running()
{
    LOG(INFO) << "Starting OTPSringOscillatorTest measurement.";
    Initialise();
    runRingOscillatorTest();
    LOG(INFO) << "Done with OTPSringOscillatorTest.";
    Reset();
}

void OTPSringOscillatorTest::Stop(void)
{
    LOG(INFO) << "Stopping OTPSringOscillatorTest measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTPSringOscillatorTest.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTPSringOscillatorTest stopped.";
}

void OTPSringOscillatorTest::Pause() {}

void OTPSringOscillatorTest::Resume() {}

void OTPSringOscillatorTest::Reset() { fRegisterHelper->restoreSnapshot(); }

void OTPSringOscillatorTest::runRingOscillatorTest()
{
    runMPAringOscillatorTest();
    runSSAringOscillatorTest();
}

void OTPSringOscillatorTest::runMPAringOscillatorTest()
{
    auto        selectMPAfunction     = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::MPA2); };
    std::string selectMPAfunctionName = "SelectMPAfunction";
    fDetectorContainer->addReadoutChipQueryFunction(selectMPAfunction, selectMPAfunctionName);

    DetectorDataContainer theMPAInverterRingOscillatorContained;
    ContainerFactory::copyAndInitChip<GenericDataArray<uint16_t, NMPAROWS + 1>>(*fDetectorContainer, theMPAInverterRingOscillatorContained);

    DetectorDataContainer theMPADelayRingOscillatorContained;
    ContainerFactory::copyAndInitChip<GenericDataArray<uint16_t, NMPAROWS + 1>>(*fDetectorContainer, theMPADelayRingOscillatorContained);

    std::vector<std::string> listOfRingOscillatorControls{"RingOscillator", "RingOscillator_ALL"};

    for(auto registerName: listOfRingOscillatorControls)
    {
        setSameDac(registerName, 0x00);
        setSameDac(registerName, 0x1 << 7 | fNumberOfClockCycles);
    }

    usleep(100);

    std::vector<std::string> listOfRingOscillatorInverter{"RO_Inverter_LSB", "RO_Inverter_MSB"};
    for(int row = 0; row < NMPAROWS; ++row)
    {
        listOfRingOscillatorInverter.push_back(MPA2::getRowRegisterName("RO_Inverter_LSB", row));
        listOfRingOscillatorInverter.push_back(MPA2::getRowRegisterName("RO_Inverter_MSB", row));
    }

    std::vector<std::string> listOfRingOscillatorDelay{"RO_Delay_LSB", "RO_Delay_MSB"};
    for(int row = 0; row < NMPAROWS; ++row)
    {
        listOfRingOscillatorDelay.push_back(MPA2::getRowRegisterName("RO_Delay_LSB", row));
        listOfRingOscillatorDelay.push_back(MPA2::getRowRegisterName("RO_Delay_MSB", row));
    }

    auto getOscillatorCounts = [this](ReadoutChip* theChip, const std::vector<std::string>& theRegisterList, DetectorDataContainer& theOutputContainer)
    {
        auto registerValues = this->fReadoutChipInterface->ReadChipMultReg(theChip, theRegisterList);
        for(int registerIndex = 0; registerIndex < NMPAROWS + 1; ++registerIndex)
        {
            uint16_t totalCount = registerValues.at(registerIndex * 2).second | (registerValues.at(registerIndex * 2 + 1).second << 8);
            theOutputContainer.getChip(theChip->getBeBoardId(), theChip->getOpticalGroupId(), theChip->getHybridId(), theChip->getId())
                ->getSummary<GenericDataArray<uint16_t, NMPAROWS + 1>>()
                .at(registerIndex) = totalCount;
        }
    };

    for(auto board: *fDetectorContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    getOscillatorCounts(chip, listOfRingOscillatorInverter, theMPAInverterRingOscillatorContained);
                    getOscillatorCounts(chip, listOfRingOscillatorDelay, theMPADelayRingOscillatorContained);
                }
            }
        }
    }

#ifdef __USE_ROOT__
    fDQMHistogramOTPSringOscillatorTest.fillMPAringOscillatorInverter(theMPAInverterRingOscillatorContained);
    fDQMHistogramOTPSringOscillatorTest.fillMPAringOscillatorDelay(theMPADelayRingOscillatorContained);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theMPAringOscillatorInverterSerialization("OTPSringOscillatorTestMPAringOscillatorInverter");
        theMPAringOscillatorInverterSerialization.streamByHybridContainer(fDQMStreamer, theMPAInverterRingOscillatorContained);

        ContainerSerialization theMPAringOscillatorDelaySerialization("OTPSringOscillatorTestMPAringOscillatorDelay");
        theMPAringOscillatorDelaySerialization.streamByHybridContainer(fDQMStreamer, theMPADelayRingOscillatorContained);
    }
#endif

    fDetectorContainer->removeReadoutChipQueryFunction(selectMPAfunctionName);
}

void OTPSringOscillatorTest::runSSAringOscillatorTest()
{
    auto        selectSSAfunction     = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::SSA2); };
    std::string selectSSAfunctionName = "SelectSSAfunction";
    fDetectorContainer->addReadoutChipQueryFunction(selectSSAfunction, selectSSAfunctionName);

    DetectorDataContainer theSSAInverterRingOscillatorContained;
    ContainerFactory::copyAndInitChip<GenericDataArray<uint16_t, 4>>(*fDetectorContainer, theSSAInverterRingOscillatorContained);

    DetectorDataContainer theSSADelayRingOscillatorContained;
    ContainerFactory::copyAndInitChip<GenericDataArray<uint16_t, 4>>(*fDetectorContainer, theSSADelayRingOscillatorContained);

    setSameDac("Ring_oscillator_ctrl", 0x00);
    setSameDac("Ring_oscillator_ctrl", 0x1 << 7 | fNumberOfClockCycles);

    usleep(100);

    std::vector<std::string> listOfRingOscillatorInverter{"Ring_oscillator_out_locBL_T1_L",
                                                          "Ring_oscillator_out_locBL_T1_H",
                                                          "Ring_oscillator_out_locBC_T1_L",
                                                          "Ring_oscillator_out_locBC_T1_H",
                                                          "Ring_oscillator_out_locBR_T1_L",
                                                          "Ring_oscillator_out_locBR_T1_H",
                                                          "Ring_oscillator_out_locTR_T1_L",
                                                          "Ring_oscillator_out_locTR_T1_H"};
    std::vector<std::string> listOfRingOscillatorDelay{"Ring_oscillator_out_locBL_T2_L",
                                                       "Ring_oscillator_out_locBL_T2_H",
                                                       "Ring_oscillator_out_locBC_T2_L",
                                                       "Ring_oscillator_out_locBC_T2_H",
                                                       "Ring_oscillator_out_locBR_T2_L",
                                                       "Ring_oscillator_out_locBR_T2_H",
                                                       "Ring_oscillator_out_locTR_T2_L",
                                                       "Ring_oscillator_out_locTR_T2_H"};

    auto getOscillatorCounts = [this](ReadoutChip* theChip, const std::vector<std::string>& theRegisterList, DetectorDataContainer& theOutputContainer)
    {
        auto registerValues = this->fReadoutChipInterface->ReadChipMultReg(theChip, theRegisterList);
        for(int registerIndex = 0; registerIndex < 4; ++registerIndex)
        {
            uint16_t totalCount = registerValues.at(registerIndex * 2).second | (registerValues.at(registerIndex * 2 + 1).second << 8);
            theOutputContainer.getChip(theChip->getBeBoardId(), theChip->getOpticalGroupId(), theChip->getHybridId(), theChip->getId())->getSummary<GenericDataArray<uint16_t, 4>>().at(registerIndex) =
                totalCount;
        }
    };

    for(auto board: *fDetectorContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    getOscillatorCounts(chip, listOfRingOscillatorInverter, theSSAInverterRingOscillatorContained);
                    getOscillatorCounts(chip, listOfRingOscillatorDelay, theSSADelayRingOscillatorContained);
                }
            }
        }
    }

#ifdef __USE_ROOT__
    fDQMHistogramOTPSringOscillatorTest.fillSSAringOscillatorInverter(theSSAInverterRingOscillatorContained);
    fDQMHistogramOTPSringOscillatorTest.fillSSAringOscillatorDelay(theSSADelayRingOscillatorContained);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theSSAringOscillatorInverterSerialization("OTPSringOscillatorTestSSAringOscillatorInverter");
        theSSAringOscillatorInverterSerialization.streamByHybridContainer(fDQMStreamer, theSSAInverterRingOscillatorContained);

        ContainerSerialization theSSAringOscillatorDelaySerialization("OTPSringOscillatorTestSSAringOscillatorDelay");
        theSSAringOscillatorDelaySerialization.streamByHybridContainer(fDQMStreamer, theSSADelayRingOscillatorContained);
    }
#endif

    fDetectorContainer->removeReadoutChipQueryFunction(selectSSAfunctionName);
}
