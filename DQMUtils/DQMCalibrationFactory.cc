#include "DQMUtils/DQMCalibrationFactory.h"
#include "DQMUtils/CBCHistogramPulseShape.h"
#include "DQMUtils/DQMHistogramBeamTestCheck.h"
#include "DQMUtils/DQMHistogramCalibrationExample.h"
#include "DQMUtils/DQMHistogramKira.h"
#include "DQMUtils/DQMHistogramLatencyScan.h"
#include "DQMUtils/DQMHistogramOTBitErrorRateTest.h"
#include "DQMUtils/DQMHistogramOTCICBX0Alignment.h"
#include "DQMUtils/DQMHistogramOTCICphaseAlignment.h"
#include "DQMUtils/DQMHistogramOTCICtoLpGBTecv.h"
#include "DQMUtils/DQMHistogramOTCICwordAlignment.h"
#include "DQMUtils/DQMHistogramOTCMNoise.h"
#include "DQMUtils/DQMHistogramOTChipToCICecv.h"
#include "DQMUtils/DQMHistogramOTCicBypassTest.h"
#include "DQMUtils/DQMHistogramOTLpGBTEyeOpeningTest.h"
#include "DQMUtils/DQMHistogramOTMeasureOccupancy.h"
#include "DQMUtils/DQMHistogramOTPSADCCalibration.h"
#include "DQMUtils/DQMHistogramOTPScommonNoise.h"
#include "DQMUtils/DQMHistogramOTPSringOscillatorTest.h"
#include "DQMUtils/DQMHistogramOTRegisterTester.h"
#include "DQMUtils/DQMHistogramOTSSAtoMPAecv.h"
#include "DQMUtils/DQMHistogramOTSSAtoSSAecv.h"
#include "DQMUtils/DQMHistogramOTVTRxLightYieldScan.h"
#include "DQMUtils/DQMHistogramOTalignBoardDataWord.h"
#include "DQMUtils/DQMHistogramOTalignLpGBTinputs.h"
#include "DQMUtils/DQMHistogramOTalignLpGBTinputsForBypass.h"
#include "DQMUtils/DQMHistogramOTalignStubPackage.h"
#include "DQMUtils/DQMHistogramOTinjectionDelayOptimization.h"
#include "DQMUtils/DQMHistogramOTinjectionOccupancyScan.h"
#include "DQMUtils/DQMHistogramOTverifyBoardDataWord.h"
#include "DQMUtils/DQMHistogramOTverifyCICdataWord.h"
#include "DQMUtils/DQMHistogramOTverifyMPASSAdataWord.h"
#include "DQMUtils/DQMHistogramPedeNoise.h"
#include "DQMUtils/DQMHistogramPedestalEqualization.h"
#include "DQMUtils/DQMMetadataIT.h"
#include "DQMUtils/DQMMetadataOT.h"
#include "DQMUtils/PSPhysicsHistograms.h"
#include "DQMUtils/Physics2SHistograms.h"
#include "DQMUtils/RD53ClockDelayHistograms.h"
#include "DQMUtils/RD53DataReadbackOptimizationHistograms.h"
#include "DQMUtils/RD53GainHistograms.h"
#include "DQMUtils/RD53GainOptimizationHistograms.h"
#include "DQMUtils/RD53GenericDacDacScanHistograms.h"
#include "DQMUtils/RD53InjectionDelayHistograms.h"
#include "DQMUtils/RD53LatencyHistograms.h"
#include "DQMUtils/RD53PhysicsHistograms.h"
#include "DQMUtils/RD53PixelAliveHistograms.h"
#include "DQMUtils/RD53SCurveHistograms.h"
#include "DQMUtils/RD53ThrEqualizationHistograms.h"
#include "DQMUtils/RD53ThresholdHistograms.h"
#include "DQMUtils/RD53VoltageTuningHistograms.h"

using namespace MessageUtils;

DQMCalibrationFactory::DQMCalibrationFactory()
{
    // Common calibrations
    Register<DQMMetadataOT>("tunelpgbtvref");

    Register<DQMMetadataOT>("configureonly");

    // OT calibrations
    Register<DQMMetadataOT, DQMHistogramPedeNoise>("noiseOT");

    Register<DQMMetadataOT>("vtrxoff");

    Register<DQMMetadataOT, DQMHistogramOTVTRxLightYieldScan>("vtrxLightYield");

    Register<DQMMetadataOT, DQMHistogramOTalignLpGBTinputs, DQMHistogramOTalignBoardDataWord, DQMHistogramOTCICtoLpGBTecv>("OTCICtoLpGBTecv");

    Register<DQMMetadataOT, DQMHistogramOTRegisterTester>("OTRegisterTester");

    Register<DQMMetadataOT, DQMHistogramOTalignLpGBTinputs>("OTalignLpGBTinputs");

    Register<DQMMetadataOT, DQMHistogramOTalignBoardDataWord>("OTalignBoardDataWord");

    Register<DQMMetadataOT, DQMHistogramOTverifyBoardDataWord>("OTverifyBoardDataWord");

    Register<DQMMetadataOT, DQMHistogramOTalignStubPackage>("OTalignStubPackage");

    Register<DQMMetadataOT,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTverifyBoardDataWord,
             DQMHistogramOTCICphaseAlignment,
             DQMHistogramOTCICwordAlignment,
             DQMHistogramOTCICBX0Alignment,
             DQMHistogramOTalignStubPackage,
             DQMHistogramOTverifyCICdataWord,
             DQMHistogramOTverifyMPASSAdataWord>("alignment");

    Register<DQMMetadataOT, DQMHistogramOTCICphaseAlignment, DQMHistogramOTalignLpGBTinputsForBypass, DQMHistogramOTCicBypassTest>("testCICbypass");

    Register<DQMMetadataOT, DQMHistogramOTalignBoardDataWord, DQMHistogramOTinjectionDelayOptimization>("injectionDelayOptimization");

    Register<DQMMetadataOT, DQMHistogramOTalignBoardDataWord, DQMHistogramOTMeasureOccupancy>("measureOccupancy");

    Register<DQMMetadataOT, DQMHistogramOTalignBoardDataWord, DQMHistogramOTinjectionOccupancyScan>("injectionOccupancyScan");

    Register<DQMMetadataOT,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTverifyBoardDataWord,
             DQMHistogramOTCICphaseAlignment,
             DQMHistogramOTCICwordAlignment,
             DQMHistogramOTCICBX0Alignment,
             DQMHistogramOTalignStubPackage,
             DQMHistogramOTverifyCICdataWord,
             DQMHistogramOTverifyMPASSAdataWord,
             DQMHistogramPedestalEqualization>("calibration");

    Register<DQMMetadataOT,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTverifyBoardDataWord,
             DQMHistogramOTCICphaseAlignment,
             DQMHistogramOTCICwordAlignment,
             DQMHistogramOTCICBX0Alignment,
             DQMHistogramOTalignStubPackage,
             DQMHistogramOTverifyCICdataWord,
             DQMHistogramOTverifyMPASSAdataWord,
             DQMHistogramPedestalEqualization,
             DQMHistogramKira>("calibrationandkira");

    Register<DQMMetadataOT,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTverifyBoardDataWord,
             DQMHistogramOTCICphaseAlignment,
             DQMHistogramOTCICwordAlignment,
             DQMHistogramOTCICBX0Alignment,
             DQMHistogramOTalignStubPackage,
             DQMHistogramOTverifyCICdataWord,
             DQMHistogramOTverifyMPASSAdataWord,
             DQMHistogramPedestalEqualization,
             DQMHistogramPedeNoise,
             DQMHistogramKira>("calibrationandpedenoiseandkira"); // will be used in future version of GIPHT

    Register<DQMMetadataOT,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTverifyBoardDataWord,
             DQMHistogramOTCICphaseAlignment,
             DQMHistogramOTCICwordAlignment,
             DQMHistogramOTCICBX0Alignment,
             DQMHistogramOTalignStubPackage,
             DQMHistogramOTverifyCICdataWord,
             DQMHistogramOTverifyMPASSAdataWord,
             DQMHistogramPedeNoise>("pedenoise");

    Register<DQMMetadataOT,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTverifyBoardDataWord,
             DQMHistogramOTCICphaseAlignment,
             DQMHistogramOTCICwordAlignment,
             DQMHistogramOTCICBX0Alignment,
             DQMHistogramOTalignStubPackage,
             DQMHistogramOTverifyCICdataWord,
             DQMHistogramOTverifyMPASSAdataWord,
             DQMHistogramPedestalEqualization,
             DQMHistogramPedeNoise>("calibrationandpedenoise");

    Register<DQMMetadataOT,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTverifyBoardDataWord,
             DQMHistogramOTCICphaseAlignment,
             DQMHistogramOTCICwordAlignment,
             DQMHistogramOTCICBX0Alignment,
             DQMHistogramOTalignStubPackage,
             DQMHistogramOTverifyCICdataWord,
             DQMHistogramOTverifyMPASSAdataWord,
             DQMHistogramOTPSADCCalibration,
             DQMHistogramPedestalEqualization,
             DQMHistogramPedeNoise>("adccalibrationandpedenoise");

    Register<DQMMetadataOT,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTverifyBoardDataWord,
             DQMHistogramOTCICphaseAlignment,
             DQMHistogramOTCICwordAlignment,
             DQMHistogramOTCICBX0Alignment,
             DQMHistogramOTalignStubPackage,
             DQMHistogramOTverifyCICdataWord,
             DQMHistogramOTverifyMPASSAdataWord,
             DQMHistogramCalibrationExample>("calibrationexample");

    Register<DQMMetadataOT,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTverifyBoardDataWord,
             DQMHistogramOTCICphaseAlignment,
             DQMHistogramOTCICwordAlignment,
             DQMHistogramOTCICBX0Alignment,
             DQMHistogramOTalignStubPackage,
             DQMHistogramOTverifyCICdataWord,
             DQMHistogramOTverifyMPASSAdataWord,
             DQMHistogramLatencyScan>("otlatency");

    Register<DQMMetadataOT,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTverifyBoardDataWord,
             DQMHistogramOTCICphaseAlignment,
             DQMHistogramOTCICwordAlignment,
             DQMHistogramOTCICBX0Alignment,
             DQMHistogramOTalignStubPackage,
             DQMHistogramOTverifyCICdataWord,
             DQMHistogramOTverifyMPASSAdataWord>("exttriggerotlatency");

    Register<DQMMetadataOT, DQMHistogramOTalignBoardDataWord, DQMHistogramOTCMNoise>("commonNoise2S");

    Register<DQMMetadataOT,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTverifyBoardDataWord,
             DQMHistogramOTCICphaseAlignment,
             DQMHistogramOTCICwordAlignment,
             DQMHistogramOTCICBX0Alignment,
             DQMHistogramOTalignStubPackage,
             DQMHistogramOTverifyCICdataWord,
             DQMHistogramOTverifyMPASSAdataWord,
             CBCHistogramPulseShape>("cbcpulseshape");

    Register<DQMMetadataOT, DQMHistogramOTCICphaseAlignment, DQMHistogramOTalignLpGBTinputsForBypass>("alignLpGBTinputsForBypass");

    Register<DQMMetadataOT, DQMHistogramOTBitErrorRateTest>("bert");

    Register<DQMMetadataOT, DQMHistogramOTLpGBTEyeOpeningTest>("eyeOpening");

    Register<DQMMetadataOT, DQMHistogramOTCICphaseAlignment, DQMHistogramOTalignLpGBTinputsForBypass, DQMHistogramOTChipToCICecv>("ChipToCICecv");

    // 2S specific calibrations

    Register<DQMMetadataOT,
             DQMHistogramOTVTRxLightYieldScan,
             DQMHistogramOTLpGBTEyeOpeningTest,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTverifyBoardDataWord,
             DQMHistogramOTCICphaseAlignment,
             DQMHistogramOTCICwordAlignment,
             DQMHistogramOTCICBX0Alignment,
             DQMHistogramOTalignStubPackage,
             DQMHistogramOTverifyCICdataWord,
             DQMHistogramPedestalEqualization,
             DQMHistogramPedeNoise>("2SquickTest");

    Register<DQMMetadataOT,
             DQMHistogramOTVTRxLightYieldScan,
             DQMHistogramOTLpGBTEyeOpeningTest,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTverifyBoardDataWord,
             DQMHistogramOTCICphaseAlignment,
             DQMHistogramOTCICwordAlignment,
             DQMHistogramOTCICBX0Alignment,
             DQMHistogramOTalignStubPackage,
             DQMHistogramOTverifyCICdataWord,
             DQMHistogramPedestalEqualization,
             DQMHistogramPedeNoise,
             DQMHistogramOTinjectionDelayOptimization,
             DQMHistogramOTinjectionOccupancyScan,
             DQMHistogramOTCMNoise,
             DQMHistogramOTCICtoLpGBTecv,
             DQMHistogramOTalignLpGBTinputsForBypass,
             DQMHistogramOTChipToCICecv,
             DQMHistogramOTBitErrorRateTest,
             DQMHistogramOTRegisterTester>("2SfullTest");

    Register<DQMMetadataOT, Physics2SHistograms>("physics2s");

    Register<DQMMetadataOT,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTCICphaseAlignment,
             DQMHistogramOTCICwordAlignment,
             DQMHistogramOTCICtoLpGBTecv,
             DQMHistogramOTalignLpGBTinputsForBypass,
             DQMHistogramOTChipToCICecv>("2Secv");

    // PS specific calibrations

    Register<DQMMetadataOT,
             DQMHistogramOTVTRxLightYieldScan,
             DQMHistogramOTLpGBTEyeOpeningTest,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTverifyBoardDataWord,
             DQMHistogramOTalignStubPackage,
             DQMHistogramOTPSringOscillatorTest,
             DQMHistogramPedestalEqualization,
             DQMHistogramPedeNoise>("PSskeletonTest");

    Register<DQMMetadataOT,
             DQMHistogramOTVTRxLightYieldScan,
             DQMHistogramOTLpGBTEyeOpeningTest,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTverifyBoardDataWord,
             DQMHistogramOTCICphaseAlignment,
             DQMHistogramOTCICwordAlignment,
             DQMHistogramOTCICBX0Alignment,
             DQMHistogramOTalignStubPackage,
             DQMHistogramOTverifyCICdataWord,
             DQMHistogramOTverifyMPASSAdataWord,
             DQMHistogramOTPSringOscillatorTest,
             DQMHistogramPedestalEqualization,
             DQMHistogramPedeNoise>("PSquickTest");

    Register<DQMMetadataOT,
             DQMHistogramOTPSADCCalibration,
             DQMHistogramOTVTRxLightYieldScan,
             DQMHistogramOTLpGBTEyeOpeningTest,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTverifyBoardDataWord,
             DQMHistogramOTCICphaseAlignment,
             DQMHistogramOTCICwordAlignment,
             DQMHistogramOTCICBX0Alignment,
             DQMHistogramOTalignStubPackage,
             DQMHistogramOTverifyCICdataWord,
             DQMHistogramOTverifyMPASSAdataWord,
             DQMHistogramOTPSringOscillatorTest,
             DQMHistogramPedestalEqualization,
             DQMHistogramPedeNoise,
             DQMHistogramOTinjectionDelayOptimization,
             DQMHistogramOTinjectionOccupancyScan,
             DQMHistogramOTPScommonNoise,
             DQMHistogramOTSSAtoMPAecv,
             DQMHistogramOTSSAtoSSAecv,
             DQMHistogramOTCICtoLpGBTecv,
             DQMHistogramOTalignLpGBTinputsForBypass,
             DQMHistogramOTChipToCICecv,
             DQMHistogramOTBitErrorRateTest,
             DQMHistogramOTRegisterTester>("PSfullTest");

    Register<DQMMetadataOT,
             DQMHistogramOTPSADCCalibration,
             DQMHistogramOTVTRxLightYieldScan,
             DQMHistogramOTLpGBTEyeOpeningTest,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTverifyBoardDataWord,
             DQMHistogramOTCICphaseAlignment,
             DQMHistogramOTCICwordAlignment,
             DQMHistogramOTCICBX0Alignment,
             DQMHistogramOTalignStubPackage,
             DQMHistogramOTverifyCICdataWord,
             DQMHistogramOTverifyMPASSAdataWord,
             DQMHistogramOTPSringOscillatorTest,
             DQMHistogramPedestalEqualization,
             DQMHistogramPedeNoise>("PSfullTestPart1");

    Register<DQMMetadataOT,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTinjectionDelayOptimization,
             DQMHistogramOTinjectionOccupancyScan,
             DQMHistogramOTPScommonNoise,
             DQMHistogramOTSSAtoMPAecv,
             DQMHistogramOTSSAtoSSAecv,
             DQMHistogramOTCICtoLpGBTecv,
             DQMHistogramOTalignLpGBTinputsForBypass,
             DQMHistogramOTChipToCICecv,
             DQMHistogramOTBitErrorRateTest,
             DQMHistogramOTRegisterTester>("PSfullTestPart2");

    Register<DQMMetadataOT, PSPhysicsHistograms>("psphysics");

    Register<DQMMetadataOT, DQMHistogramOTalignBoardDataWord, DQMHistogramOTPSADCCalibration>("ADCBiasCalibration");

    Register<DQMMetadataOT, DQMHistogramOTalignLpGBTinputs, DQMHistogramOTalignBoardDataWord, DQMHistogramOTCICphaseAlignment, DQMHistogramOTCICwordAlignment, DQMHistogramOTSSAtoMPAecv>(
        "SSAtoMPAecv");

    Register<DQMMetadataOT, DQMHistogramOTalignLpGBTinputs, DQMHistogramOTalignBoardDataWord, DQMHistogramOTCICphaseAlignment, DQMHistogramOTCICwordAlignment, DQMHistogramOTSSAtoSSAecv>(
        "SSAtoSSAecv");

    Register<DQMMetadataOT,
             DQMHistogramOTalignLpGBTinputs,
             DQMHistogramOTalignBoardDataWord,
             DQMHistogramOTCICphaseAlignment,
             DQMHistogramOTCICwordAlignment,
             DQMHistogramOTSSAtoMPAecv,
             DQMHistogramOTSSAtoSSAecv,
             DQMHistogramOTCICtoLpGBTecv,
             DQMHistogramOTalignLpGBTinputsForBypass,
             DQMHistogramOTChipToCICecv>("PSecv");

    Register<DQMHistogramOTPSringOscillatorTest>("ringOscillatorTest");

    Register<DQMMetadataOT, DQMHistogramOTalignBoardDataWord, DQMHistogramOTPScommonNoise>("commonNoisePS");

    // ###################
    // # IT calibrations #
    // ###################
    Register<DQMMetadataIT, PixelAliveHistograms>("pixelalive");
    Register<DQMMetadataIT, PixelAliveHistograms>("noise");
    Register<DQMMetadataIT, SCurveHistograms>("scurve");
    Register<DQMMetadataIT, GainHistograms>("gain");
    Register<DQMMetadataIT, GainOptimizationHistograms>("gainopt");
    Register<DQMMetadataIT, ThrEqualizationHistograms>("threqu");
    Register<DQMMetadataIT, ThresholdHistograms>("thrmin");
    Register<DQMMetadataIT, ThresholdHistograms>("thradj");
    Register<DQMMetadataIT, LatencyHistograms>("latency");
    Register<DQMMetadataIT, InjectionDelayHistograms>("injdelay");
    Register<DQMMetadataIT, ClockDelayHistograms>("clockdelay");
    Register<DQMMetadataIT, DataReadbackOptimizationHistograms>("datarbopt");
    Register<DQMMetadataIT, GenericDacDacScanHistograms>("genericdacdac");
    Register<DQMMetadataIT, VoltageTuningHistograms>("voltagetuning");
    Register<DQMMetadataIT, PhysicsHistograms>("physics");
}

DQMCalibrationFactory::~DQMCalibrationFactory()
{
    for(auto& element: fDQMInterfaceMap)
    {
        delete element.second;
        element.second = nullptr;
    }
    fDQMInterfaceMap.clear();
}

std::vector<DQMHistogramBase*> DQMCalibrationFactory::createDQMHistogrammerVector(const std::string& calibrationTag) const
{
    try
    {
        return fDQMInterfaceMap.at(calibrationTag)->Create();
    }
    catch(const std::exception& theException)
    {
        std::string errorMessage = "Error: calibration tag " + calibrationTag + " does not exist";
        throw std::runtime_error(errorMessage);
    }
}

std::vector<std::string> DQMCalibrationFactory::getAvailableCalibrations() const
{
    std::vector<std::string> listOfCalibrations;

    for(const auto& element: fDQMInterfaceMap) { listOfCalibrations.emplace_back(element.first); }
    return listOfCalibrations;
}
