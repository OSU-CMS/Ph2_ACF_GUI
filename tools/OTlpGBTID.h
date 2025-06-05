#ifndef OTlpGBTID_h__
#define OTlpGBTID_h__

#include "OTTool.h"

#ifdef __USE_ROOT__
#endif

using namespace Ph2_HwDescription;

class OTlpGBTID : public OTTool
{
  public:
    OTlpGBTID();
    ~OTlpGBTID();

    void Initialise();
    void Running() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;

    void ReadlpGBTIDs();
};
#endif