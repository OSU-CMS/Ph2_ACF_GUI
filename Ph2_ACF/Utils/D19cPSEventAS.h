
#ifndef __D19cPSEventAS_H__
#define __D19cPSEventAS_H__

#include "Utils/Event.h"

class BoardDataContainer;

namespace Ph2_HwInterface
{ // Begin namespace

using EventDataVector = std::vector<std::vector<uint32_t>>;
class D19cPSEventAS : public Event
{
  public:
    D19cPSEventAS(const Ph2_HwDescription::BeBoard* pBoard, const std::vector<uint32_t>& list);
    ~D19cPSEventAS() {}

    void        Set(const Ph2_HwDescription::BeBoard* pBoard, const std::vector<uint32_t>& list);
    uint32_t    GetNHits(uint8_t pHybridId, uint8_t pMPAId) override;
    void        fillChipDataContainer(ChipDataContainer* chipContainer, const std::shared_ptr<ChannelGroupBase> testChannelGroup, uint8_t hybridId) override;
    static void configureFastReadout(bool enableFastReadout);

  private:
    static bool fEnableFastReadout;

    std::vector<uint8_t> fChipToCicMappingPSR{5, 4, 3, 2, 6, 7, 0, 1}; //  Index CIC Fe Id , Value Hybrid Fe Id
    std::vector<uint8_t> fChipToCicMappingPSL{1, 0, 7, 6, 2, 3, 4, 5}; // Index Hybrid Fe Id , Value CIC Fe Id

    inline uint8_t getChipIdMapped(uint8_t pHybridId, uint8_t pReadoutChipId) const
    {
        pReadoutChipId = pReadoutChipId % 8;
        // assign front-end mapping
        std::vector<uint8_t> cFeMapping = (pHybridId % 2 == 0) ? fChipToCicMappingPSR : fChipToCicMappingPSL;
        return cFeMapping[pReadoutChipId];
    }

    BoardDataContainer fTheOccupancyContainer;
};

} // namespace Ph2_HwInterface
#endif
