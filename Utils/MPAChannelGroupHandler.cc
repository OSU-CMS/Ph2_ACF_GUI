#include "Utils/MPAChannelGroupHandler.h"
#include "HWDescription/Definition.h"

MPAChannelGroupHandler::MPAChannelGroupHandler()
{
    allChannelGroup_     = std::make_shared<ChannelGroup<NMPAROWS, NSSACHANNELS>>();
    currentChannelGroup_ = std::make_shared<ChannelGroup<NMPAROWS, NSSACHANNELS>>();
}

MPAChannelGroupHandler::MPAChannelGroupHandler(std::bitset<NSSACHANNELS * NMPAROWS>&& inputChannelsBitset)
{
    allChannelGroup_     = std::make_shared<ChannelGroup<NMPAROWS, NSSACHANNELS>>(std::move(inputChannelsBitset));
    currentChannelGroup_ = std::make_shared<ChannelGroup<NMPAROWS, NSSACHANNELS>>(std::move(inputChannelsBitset));
}

MPAChannelGroupHandler::~MPAChannelGroupHandler() {}