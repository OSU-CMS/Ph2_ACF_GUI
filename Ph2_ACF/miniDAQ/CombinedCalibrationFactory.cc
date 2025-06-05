#include "miniDAQ/CombinedCalibrationFactory.h"
#include "MiddlewareController.h"
#include "tools/BeamTestCheck.h"
#include "tools/CBCPulseShape.h"
#include "tools/CalibrationExample.h"
#include "tools/CombinedCalibration.h"
#include "tools/ConfigureOnly.h"
#include "tools/ECVLinkAlignmentOT.h"
#include "tools/ExtTriggerLatencyScan.h"
#include "tools/KIRA.h"
#include "tools/LatencyScan.h"
#include "tools/OTBitErrorRateTest.h"
#include "tools/OTCICBX0Alignment.h"
#include "tools/OTCICphaseAlignment.h"
#include "tools/OTCICtoLpGBTecv.h"
#include "tools/OTCICwordAlignment.h"
#include "tools/OTCMNoise.h"
#include "tools/OTChipToCICecv.h"
#include "tools/OTCicBypassTest.h"
#include "tools/OTLpGBTEyeOpeningTest.h"
#include "tools/OTMeasureOccupancy.h"
#include "tools/OTPSADCCalibration.h"
#include "tools/OTPScommonNoise.h"
#include "tools/OTPSringOscillatorTest.h"
#include "tools/OTPatternCheckerHelper.h"
#include "tools/OTRegisterTester.h"
#include "tools/OTSSAtoMPAecv.h"
#include "tools/OTSSAtoSSAecv.h"
#include "tools/OTTemperature.h"
#include "tools/OTVTRXLightOff.h"
#include "tools/OTVTRxLightYieldScan.h"
#include "tools/OTalignBoardDataWord.h"
#include "tools/OTalignLpGBTinputs.h"
#include "tools/OTalignLpGBTinputsForBypass.h"
#include "tools/OTalignStubPackage.h"
#include "tools/OTinjectionDelayOptimization.h"
#include "tools/OTinjectionOccupancyScan.h"
#include "tools/OTverifyBoardDataWord.h"
#include "tools/OTverifyCICdataWord.h"
#include "tools/OTverifyMPASSAdataWord.h"
#include "tools/PSCounterTest.h"
#include "tools/PSPhysics.h"
#include "tools/PedeNoise.h"
#include "tools/PedeNoisePSLowInjection.h"
#include "tools/PedestalEqualization.h"
#include "tools/PedestalEqualizationPSFullScan.h"
#include "tools/Physics2S.h"
#include "tools/RD53ClockDelay.h"
#include "tools/RD53Gain.h"
#include "tools/RD53GainOptimization.h"
#include "tools/RD53InjectionDelay.h"
#include "tools/RD53Latency.h"
#include "tools/RD53Physics.h"
#include "tools/RD53PixelAlive.h"
#include "tools/RD53SCurve.h"
#include "tools/RD53ThrAdjustment.h"
#include "tools/RD53ThrEqualization.h"
#include "tools/RD53ThrMinimization.h"
#include "tools/Tool.h"
#include "tools/TuneLpGBTVref.h"

using namespace MessageUtils;

CombinedCalibrationFactory::CombinedCalibrationFactory()
{
    // Common calibrations
    Register<TuneLpGBTVref>("Common", "tunelpgbtvref");

    Register<TuneLpGBTVref, ConfigureOnly>("Common", "configureonly");

    // OT calibrations

    Register<PedeNoise>("Outer Tracker", "noiseOT");

    Register<OTVTRXLightOff>("Outer Tracker", "vtrxoff");

    Register<OTVTRxLightYieldScan>("Outer Tracker", "vtrxLightYield");

    Register<OTalignLpGBTinputs, OTCICtoLpGBTecv>("Outer Tracker", "OTCICtoLpGBTecv");

    Register<OTRegisterTester>("Outer Tracker", "OTRegisterTester");

    Register<OTalignLpGBTinputs>("Outer Tracker", "OTalignLpGBTinputs");

    Register<OTalignBoardDataWord>("Outer Tracker", "OTalignBoardDataWord");

    Register<OTalignLpGBTinputs, OTalignBoardDataWord, OTverifyBoardDataWord>("Outer Tracker", "OTverifyBoardDataWord");

    Register<OTalignBoardDataWord, OTalignStubPackage>("Outer Tracker", "OTalignStubPackage");

    Register<TuneLpGBTVref,
             OTalignLpGBTinputs,
             OTalignBoardDataWord,
             OTverifyBoardDataWord,
             OTCICphaseAlignment,
             OTCICwordAlignment,
             OTCICBX0Alignment,
             OTalignStubPackage,
             OTverifyCICdataWord,
             OTverifyMPASSAdataWord>("Outer Tracker", "alignment");

    Register<OTalignLpGBTinputs, OTPatternCheckerHelper>("Outer Tracker", "patternChecker");

    Register<OTCICphaseAlignment, OTalignLpGBTinputsForBypass, OTCicBypassTest>("Outer Tracker", "testCICbypass");

    Register<OTalignBoardDataWord, OTinjectionDelayOptimization>("Outer Tracker", "injectionDelayOptimization");

    Register<OTalignBoardDataWord, OTMeasureOccupancy>("Outer Tracker", "measureOccupancy");

    Register<OTalignBoardDataWord, OTinjectionOccupancyScan>("Outer Tracker", "injectionOccupancyScan");

    Register<TuneLpGBTVref,
             OTalignLpGBTinputs,
             OTalignBoardDataWord,
             OTverifyBoardDataWord,
             OTCICphaseAlignment,
             OTCICwordAlignment,
             OTCICBX0Alignment,
             OTalignStubPackage,
             OTverifyCICdataWord,
             OTverifyMPASSAdataWord,
             PedestalEqualization>("Outer Tracker", "calibration");

    Register<OTalignLpGBTinputs,
             OTalignBoardDataWord,
             OTverifyBoardDataWord,
             OTCICphaseAlignment,
             OTCICwordAlignment,
             OTCICBX0Alignment,
             OTalignStubPackage,
             OTverifyCICdataWord,
             OTverifyMPASSAdataWord,
             PedestalEqualization,
             KIRA>("Outer Tracker", "calibrationandkira");

    Register<OTalignLpGBTinputs,
             OTalignBoardDataWord,
             OTverifyBoardDataWord,
             OTCICphaseAlignment,
             OTCICwordAlignment,
             OTverifyCICdataWord,
             OTverifyMPASSAdataWord,
             PedestalEqualization,
             PedeNoise,
             KIRA>("Outer Tracker", "calibrationandpedenoiseandkira"); // will be used in future version of GIPHT

    Register<OTalignLpGBTinputs, OTalignBoardDataWord, OTCICphaseAlignment, OTCICwordAlignment, OTCICBX0Alignment, OTalignStubPackage, PedeNoise>("Outer Tracker", "pedenoise");

    Register<OTalignLpGBTinputs, OTalignBoardDataWord, OTCICphaseAlignment, OTCICwordAlignment, OTCICBX0Alignment, OTalignStubPackage, PedestalEqualization, PedeNoise>("Outer Tracker",
                                                                                                                                                                        "calibrationandpedenoise");

    Register<TuneLpGBTVref,
             OTPSADCCalibration,
             OTalignLpGBTinputs,
             OTalignBoardDataWord,
             OTverifyBoardDataWord,
             OTCICphaseAlignment,
             OTCICwordAlignment,
             OTCICBX0Alignment,
             OTalignStubPackage,
             OTverifyCICdataWord,
             OTverifyMPASSAdataWord,
             PedestalEqualization,
             PedeNoise>("Outer Tracker", "adccalibrationandpedenoise");

    Register<OTalignLpGBTinputs,
             OTalignBoardDataWord,
             OTverifyBoardDataWord,
             OTCICphaseAlignment,
             OTCICwordAlignment,
             OTCICBX0Alignment,
             OTalignStubPackage,
             OTverifyCICdataWord,
             OTverifyMPASSAdataWord,
             CalibrationExample>("Outer Tracker", "calibrationexample");
    Register<OTalignLpGBTinputs, OTalignBoardDataWord, OTCICphaseAlignment, OTCICwordAlignment, OTCICBX0Alignment, OTalignStubPackage, PSCounterTest>("Outer Tracker", "pscountertest");
    Register<OTalignLpGBTinputs,
             OTalignBoardDataWord,
             OTverifyBoardDataWord,
             OTCICphaseAlignment,
             OTCICwordAlignment,
             OTCICBX0Alignment,
             OTalignStubPackage,
             OTverifyCICdataWord,
             OTverifyMPASSAdataWord,
             LatencyScan>("Outer Tracker", "otlatency");

    Register<OTalignLpGBTinputs,
             OTalignBoardDataWord,
             OTverifyBoardDataWord,
             OTCICphaseAlignment,
             OTCICwordAlignment,
             OTCICBX0Alignment,
             OTalignStubPackage,
             OTverifyCICdataWord,
             OTverifyMPASSAdataWord,
             ExtTriggerLatencyScan>("Outer Tracker", "exttriggerotlatency");

    Register<OTCICphaseAlignment, OTalignLpGBTinputsForBypass>("Outer Tracker", "alignLpGBTinputsForBypass");

    Register<OTBitErrorRateTest>("Outer Tracker", "bert");

    Register<OTLpGBTEyeOpeningTest>("Outer Tracker", "eyeOpening");

    Register<OTCICphaseAlignment, OTalignLpGBTinputsForBypass, OTChipToCICecv>("2S Module", "ChipToCICecv");

    // 2S specific calibrations

    Register<OTalignBoardDataWord, OTCMNoise>("2S Module", "commonNoise2S");

    Register<OTalignLpGBTinputs,
             OTalignBoardDataWord,
             OTverifyBoardDataWord,
             OTCICphaseAlignment,
             OTCICwordAlignment,
             OTCICBX0Alignment,
             OTalignStubPackage,
             OTverifyCICdataWord,
             OTverifyMPASSAdataWord,
             CBCPulseShape>("2S Module", "cbcpulseshape");

    Register<TuneLpGBTVref,
             OTVTRxLightYieldScan,
             OTLpGBTEyeOpeningTest,
             OTalignLpGBTinputs,
             OTalignBoardDataWord,
             OTverifyBoardDataWord,
             OTCICphaseAlignment,
             OTCICwordAlignment,
             OTCICBX0Alignment,
             OTalignStubPackage,
             OTverifyCICdataWord,
             PedestalEqualization,
             PedeNoise>("2S Module", "2SquickTest");

    Register<TuneLpGBTVref,
             OTVTRxLightYieldScan,
             OTLpGBTEyeOpeningTest,
             OTalignLpGBTinputs,
             OTalignBoardDataWord,
             OTverifyBoardDataWord,
             OTCICphaseAlignment,
             OTCICwordAlignment,
             OTCICBX0Alignment,
             OTalignStubPackage,
             OTverifyCICdataWord,
             PedestalEqualization,
             PedeNoise,
             OTinjectionDelayOptimization,
             OTinjectionOccupancyScan,
             OTCMNoise,
             OTCICtoLpGBTecv,
             OTalignLpGBTinputsForBypass,
             OTChipToCICecv,
             OTBitErrorRateTest,
             OTRegisterTester>("2S Module", "2SfullTest");

    Register<Physics2S>("2S Module", "physics2s");

    Register<OTalignLpGBTinputs, OTalignBoardDataWord, OTCICphaseAlignment, OTCICwordAlignment, OTCICtoLpGBTecv, OTalignLpGBTinputsForBypass, OTChipToCICecv>("2S Module", "2Secv");

    // PS specific calibrations

    Register<TuneLpGBTVref,
             OTVTRxLightYieldScan,
             OTLpGBTEyeOpeningTest,
             OTalignLpGBTinputs,
             OTalignBoardDataWord,
             OTverifyBoardDataWord,
             OTalignStubPackage,
             OTPSringOscillatorTest,
             PedestalEqualization,
             PedeNoise>("PS Module", "PSskeletonTest");

    Register<TuneLpGBTVref,
             OTVTRxLightYieldScan,
             OTLpGBTEyeOpeningTest,
             OTalignLpGBTinputs,
             OTalignBoardDataWord,
             OTverifyBoardDataWord,
             OTCICphaseAlignment,
             OTCICwordAlignment,
             OTCICBX0Alignment,
             OTalignStubPackage,
             OTverifyCICdataWord,
             OTverifyMPASSAdataWord,
             OTPSringOscillatorTest,
             PedestalEqualization,
             PedeNoise>("PS Module", "PSquickTest");

    Register<TuneLpGBTVref,
             OTPSADCCalibration,
             OTVTRxLightYieldScan,
             OTLpGBTEyeOpeningTest,
             OTalignLpGBTinputs,
             OTalignBoardDataWord,
             OTverifyBoardDataWord,
             OTCICphaseAlignment,
             OTCICwordAlignment,
             OTCICBX0Alignment,
             OTalignStubPackage,
             OTverifyCICdataWord,
             OTverifyMPASSAdataWord,
             OTPSringOscillatorTest,
             PedestalEqualizationPSFullScan,
             PedeNoisePSLowInjection,
             OTinjectionDelayOptimization,
             OTinjectionOccupancyScan,
             OTPScommonNoise,
             OTSSAtoMPAecv,
             OTSSAtoSSAecv,
             OTCICtoLpGBTecv,
             OTalignLpGBTinputsForBypass,
             OTChipToCICecv,
             OTBitErrorRateTest,
             OTRegisterTester>("PS Module", "PSfullTest");

    Register<TuneLpGBTVref,
             OTPSADCCalibration,
             OTVTRxLightYieldScan,
             OTLpGBTEyeOpeningTest,
             OTalignLpGBTinputs,
             OTalignBoardDataWord,
             OTverifyBoardDataWord,
             OTCICphaseAlignment,
             OTCICwordAlignment,
             OTCICBX0Alignment,
             OTalignStubPackage,
             OTverifyCICdataWord,
             OTverifyMPASSAdataWord,
             OTPSringOscillatorTest,
             PedestalEqualizationPSFullScan,
             PedeNoisePSLowInjection>("PS Module", "PSfullTestPart1");

    Register<OTalignBoardDataWord,
             OTinjectionDelayOptimization,
             OTinjectionOccupancyScan,
             OTPScommonNoise,
             OTSSAtoMPAecv,
             OTSSAtoSSAecv,
             OTCICtoLpGBTecv,
             OTalignLpGBTinputsForBypass,
             OTChipToCICecv,
             OTBitErrorRateTest,
             OTRegisterTester>("PS Module", "PSfullTestPart2");

    Register<PSPhysics>("PS Module", "psphysics");

    Register<OTalignBoardDataWord, OTPSADCCalibration>("PS Module", "ADCBiasCalibration");

    Register<OTalignLpGBTinputs, OTalignBoardDataWord, OTCICphaseAlignment, OTCICwordAlignment, OTSSAtoMPAecv>("PS Module", "SSAtoMPAecv");

    Register<OTalignLpGBTinputs, OTalignBoardDataWord, OTCICphaseAlignment, OTCICwordAlignment, OTSSAtoSSAecv>("PS Module", "SSAtoSSAecv");

    Register<OTalignLpGBTinputs, OTalignBoardDataWord, OTCICphaseAlignment, OTCICwordAlignment, OTSSAtoMPAecv, OTSSAtoSSAecv, OTCICtoLpGBTecv, OTalignLpGBTinputsForBypass, OTChipToCICecv>("PS Module",
                                                                                                                                                                                            "PSecv");

    Register<OTPSringOscillatorTest>("PS Module", "ringOscillatorTest");

    Register<OTalignBoardDataWord, OTPScommonNoise>("PS Module", "commonNoisePS");

    // IT calibrations
    Register<PixelAlive>("Inner Tracker", "pixelalive");
    Register<PixelAlive>("Inner Tracker", "noise");
    Register<SCurve>("Inner Tracker", "scurve");
    Register<Gain>("Inner Tracker", "gain");
    Register<GainOptimization>("Inner Tracker", "gainopt");
    Register<ThrEqualization>("Inner Tracker", "threqu");
    Register<ThrMinimization>("Inner Tracker", "thrmin");
    Register<ThrAdjustment>("Inner Tracker", "thradj");
    Register<Latency>("Inner Tracker", "latency");
    Register<InjectionDelay>("Inner Tracker", "injdelay");
    Register<ClockDelay>("Inner Tracker", "clockdelay");
    Register<Physics>("Inner Tracker", "physics");
}

CombinedCalibrationFactory::~CombinedCalibrationFactory()
{
    for(auto& calibrationListPerHardware: fCalibrationMap)
    {
        delete calibrationListPerHardware.second.second;
        calibrationListPerHardware.second.second = nullptr;
    }
    fCalibrationMap.clear();
}

Tool* CombinedCalibrationFactory::createCombinedCalibration(const std::string& calibrationName) const
{
    try
    {
        return fCalibrationMap.at(calibrationName).second->Create();
    }
    catch(const std::exception& theException)
    {
        std::string errorMessage = "Error: calibration tag " + calibrationName + " does not exist";
        throw std::runtime_error(errorMessage);
    }

    return nullptr;
}

std::map<std::string, std::map<std::string, std::vector<std::pair<std::string, std::string>>>> CombinedCalibrationFactory::getAvailableCalibrations() const
{
    std::map<std::string, std::map<std::string, std::vector<std::pair<std::string, std::string>>>> listOfCalibrations;

    for(const auto& element: fCalibrationMap) { listOfCalibrations[element.second.first][element.first] = element.second.second->fSubCalibrationAndDescriptionList; }
    return listOfCalibrations;
}
