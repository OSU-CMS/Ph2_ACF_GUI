/*!
  Filename :                              OpticalGroup.cc
  Content :                               OpticalGroup Description class
  Programmer :                    Lorenzo BIDEGAIN
  Version :               1.0
  Date of Creation :              25/06/14
  Support :                               mail to : lorenzo.bidegain@gmail.com
*/

#include "HWDescription/OpticalGroup.h"
#include "HWDescription/VTRx.h"
#include "lpGBT.h"

namespace Ph2_HwDescription
{
// Default C'tor
OpticalGroup::OpticalGroup() : FrontEndDescription(), OpticalGroupContainer(0) {}

OpticalGroup::OpticalGroup(const FrontEndDescription& pFeDesc, uint8_t pOpticalGroupId) : FrontEndDescription(pFeDesc), OpticalGroupContainer(pOpticalGroupId) {}

OpticalGroup::OpticalGroup(uint8_t pBeBoardId, uint8_t pFMCId, uint8_t pOpticalGroupId) : FrontEndDescription(pBeBoardId, pFMCId, pOpticalGroupId, 0), OpticalGroupContainer(pOpticalGroupId) {}

OpticalGroup::~OpticalGroup()
{
    delete flpGBT;
    flpGBT = nullptr;
    delete fVTRx;
    fVTRx = nullptr;
}

void OpticalGroup::addlpGBT(lpGBT* plpGBT) { flpGBT = plpGBT; }

void OpticalGroup::addVTRx(VTRx* pVTRx) { fVTRx = pVTRx; }

std::map<uint8_t, std::vector<uint8_t>> OpticalGroup::getLpGBTrxGroupsAndChannels() const
{
    std::map<uint8_t, std::vector<uint8_t>> groupsAndChannels;
    groupsAndChannels[0] = {0, 2};
    groupsAndChannels[1] = {0, 2};
    groupsAndChannels[2] = {0, 2};
    groupsAndChannels[4] = {0, 2};
    groupsAndChannels[5] = {0, 2};

    if(getFrontEndType() == FrontEndType::OuterTracker2S)
    {
        groupsAndChannels[3] = {2};
        groupsAndChannels[6] = {0};
    }
    else
    {
        groupsAndChannels[3] = {0, 2};
        groupsAndChannels[6] = {0, 2};
    }
    return groupsAndChannels;
}

std::map<std::pair<uint8_t, uint8_t>, std::pair<uint8_t, uint8_t>> OpticalGroup::getLpGBTrxGroupsAndChannelsPerHybrid() const
{
    if(getFrontEndType() == FrontEndType::OuterTracker2S)
        return f2SgroupsAndChannelToCIClineMap;
    else
        return fPSgroupsAndChannelToCIClineMap;
}

std::pair<uint8_t, uint8_t> OpticalGroup::getGroupAndChannel(uint8_t hybridId, uint8_t line) const
{
    const auto& groupsAndChannelToCIClineMap = getLpGBTrxGroupsAndChannelsPerHybrid();
    auto        hybridIdAndLine              = std::make_pair(uint8_t(hybridId % 2), line);
    for(auto groupsAndChannelToCICline: groupsAndChannelToCIClineMap)
    {
        if(groupsAndChannelToCICline.second == hybridIdAndLine) return groupsAndChannelToCICline.first;
    }
    std::cerr << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] ERROR: No group and channel found" << std::endl;
    abort();
}

std::map<std::pair<uint8_t, uint8_t>, std::pair<uint8_t, uint8_t>> OpticalGroup::f2SgroupsAndChannelToCIClineMap{{{0, 0}, {0, 0}},
                                                                                                                 {{4, 0}, {0, 1}},
                                                                                                                 {{4, 2}, {0, 2}},
                                                                                                                 {{5, 0}, {0, 3}},
                                                                                                                 {{5, 2}, {0, 4}},
                                                                                                                 {{6, 0}, {0, 5}},
                                                                                                                 {{0, 2}, {1, 1}},
                                                                                                                 {{1, 0}, {1, 2}},
                                                                                                                 {{1, 2}, {1, 3}},
                                                                                                                 {{2, 0}, {1, 4}},
                                                                                                                 {{2, 2}, {1, 5}},
                                                                                                                 {{3, 2}, {1, 0}}};
std::map<std::pair<uint8_t, uint8_t>, std::pair<uint8_t, uint8_t>> OpticalGroup::fPSgroupsAndChannelToCIClineMap{{{0, 0}, {0, 1}},
                                                                                                                 {{4, 0}, {0, 6}},
                                                                                                                 {{4, 2}, {0, 0}},
                                                                                                                 {{5, 0}, {0, 5}},
                                                                                                                 {{5, 2}, {0, 4}},
                                                                                                                 {{6, 0}, {0, 3}},
                                                                                                                 {{6, 2}, {0, 2}},
                                                                                                                 {{0, 2}, {1, 6}},
                                                                                                                 {{1, 0}, {1, 0}},
                                                                                                                 {{1, 2}, {1, 5}},
                                                                                                                 {{2, 0}, {1, 4}},
                                                                                                                 {{2, 2}, {1, 3}},
                                                                                                                 {{3, 0}, {1, 2}},
                                                                                                                 {{3, 2}, {1, 1}}};

} // namespace Ph2_HwDescription
