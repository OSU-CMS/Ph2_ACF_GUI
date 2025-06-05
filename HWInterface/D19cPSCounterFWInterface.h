#ifndef _D19cPSCounterFWInterface_H__
#define __D19cPSCounterFWInterface_H__

#include "HWInterface/L1ReadoutInterface.h"

namespace Ph2_HwDescription
{
class BeBoard;
struct ChipRegItem;
} // namespace Ph2_HwDescription
namespace Ph2_HwInterface
{

class RegManager;
class FEConfigurationInterface;
class FastCommandInterface;

class D19cPSCounterFWInterface : public L1ReadoutInterface
{
  public:
    D19cPSCounterFWInterface(RegManager* theRegManager);
    ~D19cPSCounterFWInterface();

  public:
    bool ReadEvents(const Ph2_HwDescription::BeBoard* theBoard) override;
    bool ReadEventsLocal(const Ph2_HwDescription::BeBoard* theBoard);
    // function to link FEConfigurationInterface
    void LinkFEConfigurationInterface(FEConfigurationInterface* pInterface) { fFEConfigurationInterface = pInterface; }
    void configureFastReadout(bool enableFastReadout);

  private:
    FEConfigurationInterface* fFEConfigurationInterface{nullptr};
    bool                      fPSCounterFast{true};
    // function read-back counters
    void                                                                      SlowRead(const Ph2_HwDescription::BeBoard* pBoard);
    bool                                                                      FastRead(const Ph2_HwDescription::BeBoard* pBoard);
    std::pair<Ph2_HwDescription::ChipRegItem, Ph2_HwDescription::ChipRegItem> getChannelCounterRegister(uint16_t row, uint16_t col, bool isMPA);
};
} // namespace Ph2_HwInterface
#endif
