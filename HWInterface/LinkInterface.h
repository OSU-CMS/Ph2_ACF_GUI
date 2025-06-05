#ifndef _LinkInterface_H__
#define _LinkInterface_H__

#include <cstdint>
#include <string>

namespace Ph2_HwDescription
{
class BeBoard;
}
namespace Ph2_HwInterface
{
struct LinkInterfaceConfiguration
{
    uint32_t fResetWait_ms = 200;
    uint8_t  fReTry        = 0;
    size_t   fMaxAttempts  = 10;
};

class RegManager;

class LinkInterface
{
  public:
    LinkInterface(RegManager* theRegManager);
    virtual ~LinkInterface();

  public:
    virtual void ResetLinks();
    virtual void ResetLink(uint8_t pLinkId = 0);
    virtual bool GetLinkStatus(uint8_t pLinkId = 0);
    virtual void GeneralLinkReset(const Ph2_HwDescription::BeBoard* pBoard);

  protected:
    RegManager*                fTheRegManager{nullptr};
    LinkInterfaceConfiguration fConfiguration;
    uint32_t                   getReset() { return fConfiguration.fResetWait_ms; }
    void                       setReset(uint32_t pReset_ms) { fConfiguration.fResetWait_ms = pReset_ms; }
};
} // namespace Ph2_HwInterface
#endif