
#ifndef __ROHInterface_H__
#define __ROHInterface_H__

#include "HWInterface/BeBoardFWInterface.h"
#include <boost/tokenizer.hpp>
#include <random>

namespace Ph2_HwInterface
{
// Class to interface with ROH hybrid on TestCard
class DPInterface
{
  public:
    DPInterface();
    ~DPInterface();
    void Configure(Ph2_HwInterface::BeBoardFWInterface* pInterface, uint32_t pPattern, uint16_t pFrequency = 320);
    // default config is for ps-feh data player
    void Start(Ph2_HwInterface::BeBoardFWInterface* pInterface, uint8_t pType = 0);
    void Stop(Ph2_HwInterface::BeBoardFWInterface* pInterface);
    //
    void StartSyncPlaying(Ph2_HwInterface::BeBoardFWInterface* pInterface);
    void StopSyncPlaying(Ph2_HwInterface::BeBoardFWInterface* pInterface);
    //
    void ResetFCMDBram(Ph2_HwInterface::BeBoardFWInterface* pInterface);

    // default config is for ps-feh data player
    bool     IsRunning(Ph2_HwInterface::BeBoardFWInterface* pInterface, uint8_t pType = 0);
    void     CheckNPatterns(Ph2_HwInterface::BeBoardFWInterface* pInterface);
    void     ConfigureLine(Ph2_HwInterface::BeBoardFWInterface* pInterface, uint16_t pPattern, uint8_t pLine);
    void     ConfigureLineBRAM(Ph2_HwInterface::BeBoardFWInterface* pInterface, uint8_t pLine, uint8_t pPattern, uint16_t pNumberOfClks);
    void     ConstPatternBRAMs(Ph2_HwInterface::BeBoardFWInterface* pInterface, std::vector<uint8_t> pPatterns, uint16_t pNumberOfClks);
    bool     ReadL1Data(Ph2_HwInterface::BeBoardFWInterface* pInterface);
    uint16_t LoadL1Data(Ph2_HwInterface::BeBoardFWInterface* pInterface); // returns number of L1 triggers sent
    uint16_t SendTriggers(Ph2_HwInterface::BeBoardFWInterface* pInterface);
    void     SetMaxNBx(uint16_t pNumberOfClks) { fMaxBx = pNumberOfClks; }
    void     CheckFcmdBRAM(Ph2_HwInterface::BeBoardFWInterface* pInterface);

    void GenerateDummyData();

  protected:
  private:
    bool     fEmulatorRunning;
    bool     fEmulatorConfigured;
    uint32_t fWait_us = 10;

    std::vector<uint8_t>              fFastCommands;
    std::vector<std::vector<uint8_t>> fInputCICL1Data;
    uint16_t                          fNTriggers;
    uint16_t                          fMaxBx = 1000;
};

} // namespace Ph2_HwInterface
#endif