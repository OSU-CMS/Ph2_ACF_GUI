#include "HWInterface/FEConfigurationInterface.h"
#include "Utils/ConsoleColor.h"
#include "Utils/easylogging++.h"

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
FEConfigurationInterface::FEConfigurationInterface(RegManager* theRegManager) : fTheRegManager(theRegManager) {}

FEConfigurationInterface::~FEConfigurationInterface() {}

bool FEConfigurationInterface::MultiWrite(Chip* pChip, std::vector<ChipRegItem>& pRegisterItems)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function FEConfiguration::MultiWrite is absent" << RESET;
    return false;
}

bool FEConfigurationInterface::MultiRead(Chip* pChip, std::vector<ChipRegItem>& pRegisterItems)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function FEConfiguration::MultiRead is absent" << RESET;
    return 0;
}

bool FEConfigurationInterface::SingleWriteRead(Chip* pChip, ChipRegItem& pItem)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function FEConfiguration::SingleWrite is absent" << RESET;
    return false;
}

bool FEConfigurationInterface::MultiWriteRead(Chip* pChip, std::vector<ChipRegItem>& pItem)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function FEConfiguration::MultiWriteRead is absent" << RESET;
    return false;
}

bool FEConfigurationInterface::SingleWrite(Chip* pChip, ChipRegItem& pItem)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function FEConfiguration::SingleWrite is absent" << RESET;
    return false;
}

bool FEConfigurationInterface::SingleRead(Chip* pChip, ChipRegItem& pItem)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function FEConfiguration::SingleRead is absent" << RESET;
    return 0;
}

void FEConfigurationInterface::PrintStatus()
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function TriggerInterface::PrintStatus is absent" << RESET;
    return;
}

void FEConfigurationInterface::Configure(Configuration pConfiguration)
{
    fConfiguration.fRetryIC  = pConfiguration.fRetryIC;
    fConfiguration.fRetryI2C = pConfiguration.fRetryI2C;
    fConfiguration.fRetryFE  = pConfiguration.fRetryFE;

    fConfiguration.fMaxRetryIC  = pConfiguration.fMaxRetryIC;
    fConfiguration.fMaxRetryI2C = pConfiguration.fMaxRetryI2C;
    fConfiguration.fMaxRetryFE  = pConfiguration.fMaxRetryFE;

    fConfiguration.fRetry       = pConfiguration.fRetry;
    fConfiguration.fVerify      = pConfiguration.fVerify;
    fConfiguration.fMaxAttempts = pConfiguration.fMaxAttempts;
}

} // namespace Ph2_HwInterface
