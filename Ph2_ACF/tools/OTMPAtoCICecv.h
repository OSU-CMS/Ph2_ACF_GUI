/*!
 *
 * \file OTMPAtoCICecv.h
 * \brief OTMPAtoCICecv class
 * \author Fabio Ravera
 * \date 28/05/24
 *
 */

#ifndef OTMPAtoCICecv_h__
#define OTMPAtoCICecv_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTMPAtoCICecv.h"
#endif

namespace Ph2_HwDescription
{
class Hybrid;
}

namespace Ph2_HwInterface
{
class D19cFWInterface;
}

class OTMPAtoCICecv : public Tool
{
  public:
    OTMPAtoCICecv();
    ~OTMPAtoCICecv();

    void Initialise(void);

    // State machine
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void Pause() override;
    void Resume() override;
    void Reset();

    static std::string fCalibrationDescription;

  private:
    void                               setMPAshiftRegister();
    void                               runElectricChainValidation();
    std::vector<std::vector<uint32_t>> readCICbypassOutput(Ph2_HwDescription::Hybrid* theHybrid, Ph2_HwInterface::D19cFWInterface* theFWinterface, uint8_t phyPort);

    uint8_t            fShiftRegisterPattern{0xAA};
    uint32_t           fNumberOfIterations{1000};
    std::vector<float> fListOfMPAslvsCurrents{1, 4, 7};

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTMPAtoCICecv fDQMHistogramOTMPAtoCICecv;
#endif
};

#endif
