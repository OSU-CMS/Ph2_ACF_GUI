#ifndef _FastCommandInterface_H__
#define _FastCommandInterface_H__

#include <cstdint>
#include <string>
#include <vector>

namespace Ph2_HwInterface
{
struct FastCommand
{
    uint32_t resync_en{0};
    uint32_t l1a_en{0};
    uint32_t cal_pulse_en{0};
    uint32_t bc0_en{0};
    uint32_t duration{0};
};

class RegManager;

class FastCommandInterface
{
  public: // constructors
    FastCommandInterface(RegManager* theRegManager);
    virtual ~FastCommandInterface();

  protected:
    FastCommand fFastCmd;
    RegManager* fTheRegManager{nullptr};

  public: // virtual functions
    void setDuration(uint32_t pDuration) { fFastCmd.duration = pDuration; }

    virtual void SendGlobalReSync(uint8_t pDuration = 0);               // 1 clk cycle
    virtual void SendGlobalCalPulse(uint8_t pDuration = 0);             // 1 clk cycle
    virtual void SendGlobalL1A(uint8_t pDuration = 0);                  // 1 clk cycle
    virtual void SendGlobalCounterReset(uint8_t pDuration = 0);         // 1 clk cycle
    virtual void SendGlobalCounterResetResync(uint8_t pDuration = 0);   // 1 clk cycle  // BC0 + ReScync
    virtual void SendGlobalCounterResetL1A(uint8_t pDuration = 0);      // 1 clk cycle  // BC0 + ReScync
    virtual void SendGlobalCounterResetCalPulse(uint8_t pDuration = 0); // 1 clk cycle  // BC0 + ReScync
    virtual void SendGlobalCustomFastCommands(std::vector<FastCommand>& pFastCmd);
    virtual void ComposeFastCommand(const FastCommand& pFastCommand);

  private:
};
} // namespace Ph2_HwInterface
#endif
