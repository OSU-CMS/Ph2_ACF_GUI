/*!
 *
 * \file PedeNoisePSLowInjection.h
 * \brief PedeNoisePSLowInjection class
 * \author Fabio Ravera
 * \date 14/09/24
 *
 */

#ifndef PedeNoisePSLowInjection_h__
#define PedeNoisePSLowInjection_h__

#include "tools/PedeNoise.h"

class PedeNoisePSLowInjection : public PedeNoise
{
  public:
    void Initialise(bool pAllChan = false, bool pDisableStubLogic = true) override;
    void Reset() override;

    static std::string fCalibrationDescription;

  private:
    uint8_t fOriginalPulseAmplitude;
    uint8_t fOriginalPulseAmplitudePix;
};

#endif