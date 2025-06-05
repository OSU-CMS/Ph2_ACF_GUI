#include "Utils/SSAChannelGroupHandler.h"
#include "HWDescription/Definition.h"

SSAChannelGroupHandler::SSAChannelGroupHandler()
{
    allChannelGroup_     = std::make_shared<ChannelGroup<1, NSSACHANNELS>>();
    currentChannelGroup_ = std::make_shared<ChannelGroup<1, NSSACHANNELS>>();
}

SSAChannelGroupHandler::SSAChannelGroupHandler(std::bitset<NSSACHANNELS>&& inputChannelsBitset)
{
    allChannelGroup_     = std::make_shared<ChannelGroup<1, NSSACHANNELS>>(std::move(inputChannelsBitset));
    currentChannelGroup_ = std::make_shared<ChannelGroup<1, NSSACHANNELS>>(std::move(inputChannelsBitset));
}

SSAChannelGroupHandler::~SSAChannelGroupHandler() {}
