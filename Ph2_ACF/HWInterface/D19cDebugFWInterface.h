#ifndef _D19cDebugFWInterface_H__
#define __D19cDebugFWInterface_H__

#include <cstdint>
#include <string>
#include <vector>

namespace Ph2_HwInterface
{
class RegManager;
class D19cDebugFWInterface
{
  public:
    D19cDebugFWInterface(RegManager* theRegManager);

    ~D19cDebugFWInterface();

    std::vector<std::vector<uint32_t>> StubDebug(bool pWithTestPulse = true, uint8_t pNlines = 6, bool pPrint = true);
    std::vector<uint32_t>              L1ADebug(uint8_t pWait_ms = 1, bool pPrint = true);
    std::vector<std::string>           ScopeStubLines(bool pWithTestPulse = true);
    RegManager*                        fTheRegManager{nullptr};
    uint32_t                           fTotalNumberOfTriggers{0};
};
} // namespace Ph2_HwInterface
#endif
