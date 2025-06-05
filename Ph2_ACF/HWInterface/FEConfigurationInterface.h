#ifndef _FEConfigurationInterface_H__
#define _FEConfigurationInterface_H__

#include <cstdint>
#include <string>
#include <vector>

namespace Ph2_HwDescription
{
class ChipRegItem;
class Chip;
} // namespace Ph2_HwDescription
namespace Ph2_HwInterface
{
struct Configuration
{
    // only used in D19cOpticalInterface
    bool    fRetryIC     = true;
    uint8_t fMaxRetryIC  = 100;
    bool    fRetryI2C    = true;
    uint8_t fMaxRetryI2C = 100;
    bool    fRetryFE     = true;
    uint8_t fMaxRetryFE  = 100;

    // only used in D19cI2CInterface
    bool     fVerify      = false;
    bool     fRetry       = false;
    uint16_t fMaxAttempts = 500;
};

enum class ConfigurationType
{
    I2C   = 1, // electrical I2C via FW
    IC    = 2, // IC on optical link
    EC    = 3, // EC on optical link
    SLAVE = 4  // to an I2C slave on the optical link
};

class RegManager;

class FEConfigurationInterface
{
  public:
    FEConfigurationInterface(RegManager* theRegManager);
    virtual ~FEConfigurationInterface();

  public:
    virtual bool MultiWrite(Ph2_HwDescription::Chip* pChip, std::vector<Ph2_HwDescription::ChipRegItem>& pRegisterItems);
    virtual bool MultiRead(Ph2_HwDescription::Chip* pChip, std::vector<Ph2_HwDescription::ChipRegItem>& pRegisterItems);
    virtual bool SingleWriteRead(Ph2_HwDescription::Chip* pChip, Ph2_HwDescription::ChipRegItem& pItem);
    virtual bool MultiWriteRead(Ph2_HwDescription::Chip* pChip, std::vector<Ph2_HwDescription::ChipRegItem>& pItem);
    virtual bool SingleWrite(Ph2_HwDescription::Chip* pChip, Ph2_HwDescription::ChipRegItem& pItem);
    virtual bool SingleRead(Ph2_HwDescription::Chip* pChip, Ph2_HwDescription::ChipRegItem& pItem);
    virtual void PrintStatus();

    void Configure(Configuration pConfiguration);

    void              setConfigurationType(ConfigurationType pType) { fType = pType; }
    ConfigurationType getConfigurationType() { return fType; }

  protected:
    RegManager*       fTheRegManager{nullptr};
    Configuration     fConfiguration;
    uint8_t           fNReadoutChip{0};
    ConfigurationType fType;
};
} // namespace Ph2_HwInterface
#endif
