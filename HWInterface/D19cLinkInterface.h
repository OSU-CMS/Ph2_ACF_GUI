#ifndef _D19cLinkInterface_H__
#define _D19cLinkInterface_H__

#include "HWInterface/LinkInterface.h"
#include <cstdint>

namespace Ph2_HwInterface
{
class RegManager;

class D19cLinkInterface : public LinkInterface
{
  public:
    D19cLinkInterface(RegManager* theRegManager);
    ~D19cLinkInterface();

  public:
    uint32_t fWait_ms = 100;
    void     ResetLinks() override;
    bool     GetLinkStatus(uint8_t pLinkId = 0) override;
    void     GeneralLinkReset(const Ph2_HwDescription::BeBoard* pBoard) override;

  private:
};
} // namespace Ph2_HwInterface
#endif