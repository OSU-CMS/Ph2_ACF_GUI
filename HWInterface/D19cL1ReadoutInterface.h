#ifndef _D19cL1ReadoutInterface_H__
#define __D19cL1ReadoutInterface_H__

#include "HWInterface/L1ReadoutInterface.h"
#include <cstdint>

namespace D19cL1EvntEncoder
{
// ################
// # Event header #
// ################
const uint16_t EVT_HEADER = 0xFFFF;

const uint16_t IWORD_L1_HEADER = 4;
const uint16_t SBIT_L1_HEADER  = 28;
const uint16_t SBIT_L1_STATUS  = 24;
const uint16_t SBIT_HYBRID_ID  = 16;
const uint16_t SBIT_CHIP_ID    = 12;

// ################
// # Event status #
// ################
const uint16_t GOOD           = 0x0000; // Event status Good
const uint16_t EMPTY          = 0x0002; // Event status Empty event
const uint16_t BADHEADER      = 0x0004; // Bad header
const uint8_t  GOODL1HEADER   = 0x0A;
const uint8_t  GOODStubHEADER = 0x05;
const uint16_t BADL1HEADER    = 0x0006; // Bad L1 header
const uint16_t BADSTUBHEADER  = 0x0008; // Bad Stub header
/*const uint16_t INCOMPLETE = 0x0004; // Event status Incomplete event header
const uint16_t L1A        = 0x0008; // Event status L1A counter mismatch
const uint16_t FWERR      = 0x0010; // Event status Firmware error
const uint16_t FRSIZE     = 0x0020; // Event status Invalid frame size
const uint16_t MISSCHIP   = 0x0040; // Event status Chip data are missing*/
const uint16_t NODECODER = 0xFFFF; // Event decoding not implemented

const uint16_t CLUSTER_2S   = 14;
const uint16_t SCLUSTER_PS  = 14;
const uint16_t PCLUSTER_PS  = 17;
const uint16_t SCLUSTER_MPA = 0;
const uint16_t PCLUSTER_MPA = 0;
const uint16_t HITS_2S      = 274;
const uint16_t HITS_SSA     = 120;
const uint16_t HITS_CBC     = 254;
} // namespace D19cL1EvntEncoder

namespace Ph2_HwInterface
{
class RegManager;
class D19cL1ReadoutInterface : public L1ReadoutInterface
{
  public:
    D19cL1ReadoutInterface(RegManager* theRegManager);
    ~D19cL1ReadoutInterface();

  public:
    void FillData() override;
    bool WaitForReadout() override;
    bool WaitForNTriggers() override;
    bool ReadEvents(const Ph2_HwDescription::BeBoard* pBoard) override;
    bool PollReadoutData(const Ph2_HwDescription::BeBoard* pBoard, bool pWait = false) override;
    bool ResetReadout() override;
    bool CheckBuffers() override;

    void SetWait(uint32_t pWait_us) { fWait_us = pWait_us; }

  private:
    bool WaitForData();
    bool CheckReadoutReq();
    bool CheckForWordsInReadout();
    void CountFwEvents();

    uint32_t fWait_us{100};
    uint32_t fReadoutAttempts{0};
    uint32_t fDDR3Offset{0};
    uint8_t  fWaitForReadoutReq{0};
};
} // namespace Ph2_HwInterface
#endif
