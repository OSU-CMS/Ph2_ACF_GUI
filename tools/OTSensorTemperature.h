/*!
 *
 * \file LinkAlignment.h
 * \brief Link alignment class, automated alignment procedure for CIC-lpGBT-BE
 * connected to FEs
 * \author Sarah SEIF EL NASR-STOREY
 * \date 28 / 06 / 19
 *
 * \Support : sarah.storey@cern.ch
 *
 */

#ifndef OTSensorTemperature_h__
#define OTSensorTemperature_h__

#include "OTTool.h"

#ifdef __USE_ROOT__
#endif

using namespace Ph2_HwDescription;

class OTSensorTemperature : public OTTool
{
  public:
    OTSensorTemperature();
    ~OTSensorTemperature();

    void Initialise();
    void Running() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;
    void writeObjects();

  protected:
  private:
    void                 ReadThermistors(const OpticalGroup* pOpticalGroup);
    float                ReadThermistor(const OpticalGroup* pOpticalGroup, std::string pADC, std::string theNTCtype);
    void                 ReadModuleTemperatures();
    float                fVref{1};        // reference voltage for lpgBT
    uint8_t              fGain{0};        // gain
    float                fVinput2S{10.5}; // input voltage to 2S-SEH
    std::vector<uint8_t> fCurrentDACs{0x01, 0x02, 0x03, 0x04, 0x05, 0x07, 0x10, 0x12, 0x15};
};
#endif
