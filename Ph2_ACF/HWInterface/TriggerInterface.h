#ifndef _TriggerInterface_H__
#define _TriggerInterface_H__

#include <cstdint>
#include <string>
#include <vector>

namespace Ph2_HwInterface
{
struct TriggerConfiguration
{
    uint8_t  fTriggerSource;
    uint16_t fTriggerRate;
    uint32_t fNtriggersToAccept;
};

class RegManager;
class TriggerInterface
{
  public: // constructors
    TriggerInterface(RegManager* theRegManager);
    virtual ~TriggerInterface();

  public: // virtual functions
    virtual bool SetNTriggersToAccept(uint32_t pNTriggersToAccept);
    virtual void ResetTriggerFSM();
    virtual void ReconfigureTriggerFSM(std::vector<std::pair<std::string, uint32_t>> pTriggerConfig);

    virtual bool     Start(bool checkIfStarted = false);
    virtual bool     Stop();
    virtual void     Pause();
    virtual void     Resume();
    virtual uint32_t GetTriggerState();
    virtual bool     RunTriggerFSM();
    virtual bool     WaitForNTriggers(uint32_t pNTriggers);
    virtual bool     SendNTriggers(uint32_t pNTriggers);
    virtual void     PrintStatus();

    uint8_t     getTriggerSource() { return fTriggerConfiguration.fTriggerSource; }
    uint8_t     getTriggerRate() { return fTriggerConfiguration.fTriggerRate; }
    void        setTimeout(uint32_t pTimeout_us) { fTimeout_us = pTimeout_us; }
    uint32_t    getTimeout() { return fTimeout_us; }
    RegManager* fTheRegManager{nullptr};

  protected:
    uint32_t             fWait_us{10};
    TriggerConfiguration fTriggerConfiguration;
    uint32_t             fTimeout_us{60000000}; // time-out after 60s
};
} // namespace Ph2_HwInterface
#endif
