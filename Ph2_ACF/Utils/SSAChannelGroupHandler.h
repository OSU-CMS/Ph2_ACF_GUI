#include "HWDescription/Definition.h"
#include "Utils/ChannelGroupHandler.h"

#ifndef SSAChannelGroupHandler_h
#define SSAChannelGroupHandler_h
class SSAChannelGroupHandler : public ChannelGroupHandler
{
  public:
    SSAChannelGroupHandler();
    SSAChannelGroupHandler(std::bitset<NSSACHANNELS>&& inputChannelsBitset);
    ~SSAChannelGroupHandler();
};

#endif