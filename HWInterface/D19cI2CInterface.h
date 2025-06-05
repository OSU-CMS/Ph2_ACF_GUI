#ifndef _D19cI2CInterface_H__
#define __D19cI2CInterface_H__

#include "HWInterface/FEConfigurationInterface.h"
#include <map>
#include <vector>

namespace Ph2_HwDescription
{
class BeBoard;
}

namespace Ph2_HwInterface
{
class RegManager;

class D19cI2CInterface : public FEConfigurationInterface
{
  public:
    D19cI2CInterface(RegManager* theRegManager);
    ~D19cI2CInterface();

  public:
    bool SingleWriteRead(Ph2_HwDescription::Chip* pChip, Ph2_HwDescription::ChipRegItem& pItem) override;
    bool MultiWriteRead(Ph2_HwDescription::Chip* pChip, std::vector<Ph2_HwDescription::ChipRegItem>& pItem) override;

    bool MultiWrite(Ph2_HwDescription::Chip* pChip, std::vector<Ph2_HwDescription::ChipRegItem>& pRegisterItems) override;
    bool MultiRead(Ph2_HwDescription::Chip* pChip, std::vector<Ph2_HwDescription::ChipRegItem>& pRegisterItems) override;

    bool SingleWrite(Ph2_HwDescription::Chip* pChip, Ph2_HwDescription::ChipRegItem& pItem) override;
    bool SingleRead(Ph2_HwDescription::Chip* pChip, Ph2_HwDescription::ChipRegItem& pItem) override;

    void PrintStatus() override;

    void ChipI2CRefresh();
    void ConfigureI2CMap(const Ph2_HwDescription::BeBoard* pBoard);

  private:
    // encode and decode information from D19c
    void EncodeReg(const Ph2_HwDescription::ChipRegItem& pRegItem, Ph2_HwDescription::Chip* pChip, std::vector<uint32_t>& pVecReq, bool pReadBack, bool pWrite);
    void DecodeReg(Ph2_HwDescription::ChipRegItem& pRegItem, uint8_t& pCbcId, uint32_t pWord, bool& pRead, bool& pFailed);
    // write block reg
    bool WriteChipBlockReg(std::vector<uint32_t>& pVecReg, uint8_t& pWriteAttempts, bool pReadback);
    void ReadChipBlockReg(std::vector<uint32_t>& pVecReg);

    // blcok read for CBCs
    void BCEncodeReg(const Ph2_HwDescription::ChipRegItem& pRegItem, uint8_t pNCbc, std::vector<uint32_t>& pVecReq, bool pReadBack, bool pWrite);
    bool BCWriteChipBlockReg(std::vector<uint32_t>& pVecReg, bool pReadback);

    // write and read I2C
    bool        WriteI2C(std::vector<uint32_t>& pVecSend, std::vector<uint32_t>& pReplies, bool pReadback, bool pBroadcast);
    bool        ReadI2C(uint32_t pNReplies, std::vector<uint32_t>& pReplies);
    void        ReadErrors();
    static bool cmd_reply_comp(const uint32_t& cWord1, const uint32_t& cWord2) { return true; }
    static bool cmd_reply_ack(const uint32_t& cWord1, const uint32_t& cWord2)
    {
        // if it was a write transaction (>>17 == 0) and
        // the CBC id matches it is false
        if(((cWord2 >> 16) & 0x1) == 0 && (cWord1 & 0x00F00000) == (cWord2 & 0x00F00000))
            return true;
        else
            return false;
    }

    std::map<uint8_t, std::vector<uint32_t>> fI2CSlaveMap;
    // i2c version of master
    uint32_t       fI2CVersion;
    const uint32_t SINGLE_I2C_WAIT = 200; // used for 1MHz I2C
    // #FIXME putting these here temporarily ...
    bool fRetry = false;
};
} // namespace Ph2_HwInterface
#endif