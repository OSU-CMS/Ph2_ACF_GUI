#include "tools/PedeNoisePSLowInjection.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string PedeNoisePSLowInjection::fCalibrationDescription = "Measure noise and pulse peak with low injection";

void PedeNoisePSLowInjection::Initialise(bool pAllChan, bool pDisableStubLogic)
{
    fOriginalPulseAmplitude    = findValueInSettings<double>("PedeNoise_PulseAmplitude", 0);
    fOriginalPulseAmplitudePix = findValueInSettings<double>("PedeNoise_PulseAmplitudePix", fPulseAmplitude);
    setValueInSettings<double>("PedeNoise_PulseAmplitude", findValueInSettings<double>("PedeNoisePSLowInjection_PulseAmplitude", 30));
    setValueInSettings<double>("PedeNoise_PulseAmplitudePix", findValueInSettings<double>("PedeNoisePSLowInjection_PulseAmplitudePix", 30));
    PedeNoise::Initialise(pAllChan, pDisableStubLogic);
}

void PedeNoisePSLowInjection::Reset()
{
    // restoring original setting
    setValueInSettings<double>("PedeNoise_PulseAmplitude", fOriginalPulseAmplitude);
    setValueInSettings<double>("PedeNoise_PulseAmplitudePix", fOriginalPulseAmplitudePix);
    PedeNoise::Reset();
}
