#ifndef OTLightTransmission_h__
#define OTLightTransmission_h__

#include "OTTool.h"

#ifdef __USE_ROOT__
#endif

using namespace Ph2_HwDescription;

class OTLightTransmission : public OTTool
{
  public:
    OTLightTransmission(int);
    ~OTLightTransmission();

    void Initialise();
    void Running() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;

    void ReadRegisters();

  private:
    int cChannel;
};
#endif
