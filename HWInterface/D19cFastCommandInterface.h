#ifndef _D19cFastCommandInterface_H__
#define _D19cFastCommandInterface_H__

#include "HWInterface/FastCommandInterface.h"
#include <cstdint>

namespace Ph2_HwInterface
{
class RegManager;
class D19cFastCommandInterface : public FastCommandInterface
{
  public: // constructors
    D19cFastCommandInterface(RegManager* theRegManager);
    ~D19cFastCommandInterface();

  public:                                                                // functions
    void SendGlobalReSync(uint8_t pDuration = 0) override;               // 1 clk cycle  ;
    void SendGlobalCalPulse(uint8_t pDuration = 0) override;             // 1 clk cycle  ;
    void SendGlobalL1A(uint8_t pDuration = 0) override;                  // 1 clk cycle  ;
    void SendGlobalCounterReset(uint8_t pDuration = 0) override;         // 1 clk cycle  ;
    void SendGlobalCounterResetResync(uint8_t pDuration = 0) override;   // 1 clk cycle  ;
    void SendGlobalCounterResetL1A(uint8_t pDuration = 0) override;      // 1 clk cycle  ;
    void SendGlobalCounterResetCalPulse(uint8_t pDuration = 0) override; // 1 clk cycle  ;
    void SendGlobalCustomFastCommands(std::vector<FastCommand>& pFastCmd) override;
    void ComposeFastCommand(const FastCommand& pFastCommand) override;

  private:
    uint32_t fFastCommand{0};
};
} // namespace Ph2_HwInterface
#endif