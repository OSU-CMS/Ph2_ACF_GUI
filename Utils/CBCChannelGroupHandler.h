#include "HWDescription/Definition.h"
#include "Utils/ChannelGroupHandler.h"

#ifndef CBCChannelGroupHandler_h
#define CBCChannelGroupHandler_h
class CBCChannelGroupHandler : public ChannelGroupHandler
{
  public:
    CBCChannelGroupHandler();
    CBCChannelGroupHandler(std::bitset<NCHANNELS>&& inputChannelsBitset);
    ~CBCChannelGroupHandler();
};

#endif