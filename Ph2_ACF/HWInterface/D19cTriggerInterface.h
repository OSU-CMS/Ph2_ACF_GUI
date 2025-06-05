#ifndef _D19cTriggerInterface_H__
#define _D19cTriggerInterface_H__

#include "HWInterface/TriggerInterface.h"
#include <cstdint>
#include <string>
#include <vector>

namespace Ph2_HwInterface
{
class RegManager;
class D19cTriggerInterface : public TriggerInterface
{
  public: // constructors
    D19cTriggerInterface(RegManager* theRegManager);
    ~D19cTriggerInterface();

  public: // virtual functions
    bool     SetNTriggersToAccept(uint32_t pNTriggersToAccept) override;
    void     ResetTriggerFSM() override;
    void     ReconfigureTriggerFSM(std::vector<std::pair<std::string, uint32_t>> pTriggerConfig) override;
    bool     Start(bool checkIfStarted = false) override;
    void     Pause() override;
    void     Resume() override;
    bool     Stop() override;
    bool     RunTriggerFSM() override;
    uint32_t GetTriggerState() override;
    bool     WaitForNTriggers(uint32_t pNTriggers) override;
    bool     SendNTriggers(uint32_t pNTriggers) override;
    void     PrintStatus() override;

  private:
    void TriggerConfiguration();
};
} // namespace Ph2_HwInterface
#endif