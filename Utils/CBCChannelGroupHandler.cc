#include "Utils/CBCChannelGroupHandler.h"
#include <memory>

CBCChannelGroupHandler::CBCChannelGroupHandler()
{
    allChannelGroup_     = std::make_shared<ChannelGroup<1, NCHANNELS>>();
    currentChannelGroup_ = std::make_shared<ChannelGroup<1, NCHANNELS>>();
}

CBCChannelGroupHandler::CBCChannelGroupHandler(std::bitset<NCHANNELS>&& inputChannelsBitset)
{
    allChannelGroup_     = std::make_shared<ChannelGroup<1, NCHANNELS>>(std::move(inputChannelsBitset));
    currentChannelGroup_ = std::make_shared<ChannelGroup<1, NCHANNELS>>(std::move(inputChannelsBitset));
}
CBCChannelGroupHandler::~CBCChannelGroupHandler() {}