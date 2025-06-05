/*!
 *
 * \file OTVTRXLightOff.h
 * \brief Turn off VTR light to measure Module IV curve
 * connected to FEs
 * \author Stefan Maier
 * \date 02 / 06 / 23
 *
 * \Support : sarah.storey@cern.ch
 *
 */

#ifndef OTVTRXLightOff_h__
#define OTVTRXLightOff_h__

#include "tools/Tool.h"

#ifdef __USE_ROOT__
#endif

using namespace Ph2_HwDescription;

class OTVTRXLightOff : public Tool
{
  public:
    OTVTRXLightOff();
    ~OTVTRXLightOff();

    void Initialise();
    void Running() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;
    // void Reset();
    void writeObjects();

    void TurnOffLight();

  protected:
  private:
};
#endif
