#ifndef _L1ReadoutInterface_H__
#define _L1ReadoutInterface_H__

#include <cstdint>
#include <string>
#include <vector>

namespace Ph2_HwDescription
{
class BeBoard;
}
namespace Ph2_HwInterface
{
class RegManager;
class FastCommandInterface;
class TriggerInterface;
class L1ReadoutInterface
{
  public: // constructors
    L1ReadoutInterface(RegManager* theRegManager);
    virtual ~L1ReadoutInterface();

  protected:
    FastCommandInterface* fFastCommandInterface{nullptr};
    TriggerInterface*     fTriggerInterface{nullptr};
    uint8_t               fHandshake;
    std::vector<uint32_t> fData;
    uint32_t              fNEvents{100};
    uint32_t              fNReadoutEvents;
    uint32_t              fMaxAttempts{5};
    uint32_t              fReadoutAttempt{0};
    uint32_t              fTimeout_us{5000000}; // time-out after 5s
    RegManager*           fTheRegManager{nullptr};

  public: // virtual functions
    virtual void FillData();
    virtual bool WaitForReadout();
    virtual bool WaitForNTriggers();
    virtual bool ReadEvents(const Ph2_HwDescription::BeBoard* pBoard);
    virtual bool PollReadoutData(const Ph2_HwDescription::BeBoard* pBoard, bool pWait = false);
    virtual bool ResetReadout();
    virtual bool CheckBuffers();

    std::vector<uint32_t> getData() { return fData; }
    void                  clearData() { fData.clear(); }
    // function to link FEConfigurationInterface
    void LinkFastCommandInterface(FastCommandInterface* pInterface) { fFastCommandInterface = pInterface; }
    void LinkTriggerInterface(TriggerInterface* pInterface) { fTriggerInterface = pInterface; }

    void     setNEvents(uint32_t pNEvents) { fNEvents = pNEvents; }
    uint32_t getNEvents() { return fNEvents; }
    uint32_t getNReadoutEvents() { return fNReadoutEvents; }
    void     setHandshake(uint8_t pHandshake) { fHandshake = pHandshake; }
};
} // namespace Ph2_HwInterface
#endif