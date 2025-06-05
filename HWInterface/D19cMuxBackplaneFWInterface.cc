#include "HWInterface/D19cMuxBackplaneFWInterface.h"
#include "HWDescription/BeBoard.h"

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
D19cMuxBackplaneFWInterface::D19cMuxBackplaneFWInterface(const char* puHalConfigFileName, uint32_t pBoardId, FileHandler* pFileHandler, BeBoard* theBoard)
    : BeBoardFWInterface(puHalConfigFileName, pBoardId, theBoard)
{
}
D19cMuxBackplaneFWInterface::D19cMuxBackplaneFWInterface(const char* pId, const char* pUri, const char* pAddressTable, FileHandler* pFileHandler, BeBoard* theBoard)
    : BeBoardFWInterface(pId, pUri, pAddressTable, theBoard)
{
}
D19cMuxBackplaneFWInterface::~D19cMuxBackplaneFWInterface() {}

// disconnect setup with multiplexing backplane
void D19cMuxBackplaneFWInterface::DisconnectMultiplexingSetup(uint8_t pWait_ms)
{
    LOG(INFO) << BOLDBLUE << "Disconnect multiplexing set-up" << RESET;

    bool L12Power = (ReadReg("sysreg.fmc_pwr.l12_pwr_en") == 1);
    bool L8Power  = (ReadReg("sysreg.fmc_pwr.l8_pwr_en") == 1);
    bool PGC2M    = (ReadReg("sysreg.fmc_pwr.pg_c2m") == 1);
    if(!L12Power)
    {
        LOG(ERROR) << RED << "Power on L12 is not enabled" << RESET;
        throw std::runtime_error("FC7 power is not enabled!");
    }
    if(!L8Power)
    {
        LOG(ERROR) << RED << "Power on L8 is not enabled" << RESET;
        throw std::runtime_error("FC7 power is not enabled!");
    }
    if(!PGC2M)
    {
        LOG(ERROR) << RED << "PG C2M is not enabled" << RESET;
        throw std::runtime_error("FC7 power is not enabled!");
    }

    bool BackplanePG = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.backplane_powergood") == 1);
    bool CardPG      = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.card_powergood") == 1);
    LOG(INFO) << BOLDBLUE << "Back-plane power good " << BackplanePG << " and card power good " << CardPG << " ." << RESET;
    bool SystemPowered = false;
    if(BackplanePG && CardPG)
    {
        LOG(INFO) << BOLDBLUE << "Back-plane power good and card power good." << RESET;
        WriteReg("fc7_daq_ctrl.physical_interface_block.multiplexing_bp.setup_disconnect", 0x1);
        SystemPowered = true;
    }
    else
    {
        LOG(INFO) << GREEN << "============================" << RESET;
        LOG(INFO) << BOLDGREEN << "Setup is disconnected" << RESET;
    }
    if(SystemPowered)
    {
        int  cWait                  = 0;
        bool CardsDisconnected      = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.cards_disconnected") == 1);
        bool c                      = false;
        bool BackplanesDisconnected = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.backplanes_disconnected") == 1);
        bool b                      = false;
        LOG(INFO) << GREEN << "============================" << RESET;
        LOG(INFO) << BOLDGREEN << "Disconnecting setup" << RESET;

        while(!CardsDisconnected)
        {
            if(c == false) LOG(INFO) << "Disconnecting cards";
            c = true;
            std::this_thread::sleep_for(std::chrono::microseconds(pWait_ms * 1000));
            CardsDisconnected = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.cards_disconnected") == 1);
            LOG(DEBUG) << BOLDBLUE << "Set-up scanned : " << +ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.setup_scanned") << RESET;
            cWait += pWait_ms;
        }

        LOG(INFO) << "Card deselected after " << +cWait << " ms" << RESET;
        cWait = 0;
        while(!BackplanesDisconnected)
        {
            if(b == false) LOG(INFO) << "Disconnecting backplanes";
            b = true;
            std::this_thread::sleep_for(std::chrono::microseconds(pWait_ms * 1000));
            BackplanesDisconnected = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.backplanes_disconnected") == 1);
            LOG(DEBUG) << BOLDBLUE << "Set-up scanned : " << +ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.setup_scanned") << RESET;
            cWait += pWait_ms;
        }

        LOG(INFO) << "Backplane deselected after " << +cWait << " ms" << RESET;

        if(CardsDisconnected && BackplanesDisconnected)
        {
            LOG(INFO) << GREEN << "============================" << RESET;
            LOG(INFO) << BOLDGREEN << "Setup is disconnected" << RESET;
        }
    }
}

// scan setup with multiplexing backplane
uint32_t D19cMuxBackplaneFWInterface::ScanMultiplexingSetup(uint8_t pWait_ms)
{
    int AvailableBackplanesCards = 0;
    this->DisconnectMultiplexingSetup(pWait_ms);

    LOG(INFO) << BOLDBLUE << "Sending a global reset to the FC7 ..... " << RESET;
    this->WriteReg("fc7_daq_ctrl.command_processor_block.global.reset", 0x1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    bool ConfigurationRequired = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.configuration_required") == 1);
    bool SystemNotConfigured   = false;
    if(ConfigurationRequired)
    {
        SystemNotConfigured = true;
        WriteReg("fc7_daq_ctrl.physical_interface_block.multiplexing_bp.setup_scan", 0x1);
    }

    if(SystemNotConfigured == true)
    {
        bool SetupScanned = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.setup_scanned") == 1);
        bool s            = false;
        LOG(INFO) << GREEN << "============================" << RESET;
        LOG(INFO) << BOLDGREEN << "Scan setup" << RESET;
        while(!SetupScanned)
        {
            if(s == false) LOG(INFO) << "Scanning setup";
            s = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(pWait_ms));
            SetupScanned = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.setup_scanned") == 1);
        }

        if(SetupScanned)
        {
            LOG(INFO) << GREEN << "============================" << RESET;
            LOG(INFO) << BOLDGREEN << "Setup is scanned" << RESET;
            AvailableBackplanesCards = ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.available_backplanes_cards");
        }
    }
    return AvailableBackplanesCards;
}

// configure setup with multiplexing backplane
void D19cMuxBackplaneFWInterface::ConfigureMultiplexingSetup(int BackplaneNum, int CardNum, bool InterlockEnabled, uint8_t pWait_ms)
{
    bool InitialStatusPowerGood = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.flag_power_good_dropped") == 1);
    LOG(INFO) << BOLDCYAN << "Power good monitoring flag : " << InitialStatusPowerGood << RESET;

    // Disconnect any selected cards
    this->DisconnectMultiplexingSetup();

    // Clear power good monitoring flag
    LOG(INFO) << "Resetting " << BOLDBLUE << "powergood_dropped" << RESET << " flag" << RESET;
    WriteReg("fc7_daq_ctrl.physical_interface_block.multiplexing_bp.reset_powergood_flag", 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    WriteReg("fc7_daq_ctrl.physical_interface_block.multiplexing_bp.reset_powergood_flag", 0);

    WriteReg("fc7_daq_cnfg.physical_interface_block.multiplexing_bp.backplane_num", 0xF & ~(1 << (3 - BackplaneNum)));
    WriteReg("fc7_daq_cnfg.physical_interface_block.multiplexing_bp.card_num", 0xF & ~(1 << (3 - CardNum)));
    std::this_thread::sleep_for(std::chrono::milliseconds(pWait_ms));
    bool BackplaneValid = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.backplane_valid") == 1);
    bool CardValid      = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.card_valid") == 1);
    LOG(INFO) << BOLDMAGENTA << "backplane_valid ->" << BackplaneValid << RESET;
    LOG(INFO) << BOLDMAGENTA << "card_valid ->" << CardValid << RESET;
    bool ConfigurationRequired = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.configuration_required") == 1);
    bool SystemNotConfigured   = false;
    if(ConfigurationRequired)
    {
        SystemNotConfigured = true;
        WriteReg("fc7_daq_ctrl.physical_interface_block.multiplexing_bp.setup_configure", 0x1);
        std::this_thread::sleep_for(std::chrono::microseconds(pWait_ms * 1000));
    }

    if(SystemNotConfigured == true)
    {
        //************************************** SCAN IS NOT MANDATORY ANYMORE **********************************************
        // bool SetupScanned   = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.setup_scanned") == 1);
        // bool BackplaneValid = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.backplane_valid") == 1);
        // bool CardValid      = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.card_valid") == 1);
        /*if(SetupScanned)
        {
            if(BackplaneValid) { LOG(INFO) << BLUE << "Backplane configuration VALID" << RESET; }
            else
            {
                LOG(ERROR) << RED << "Backplane configuration is NOT VALID" << RESET;
                throw std::runtime_error(std::string("Backplane configuration is NOT VALID"));
            }
            if(CardValid) { LOG(INFO) << BLUE << "Card configuration VALID" << RESET; }
            else
            {
                LOG(ERROR) << RED << "Card configuration is NOT VALID" << RESET;
                throw std::runtime_error(std::string("Card configuration is NOT VALID"));
            }
        }
        else
            LOG(ERROR) << RED << "First you must scan the setup! Map of present backplanes and cards is not available!" << RESET;*/

        bool SetupConfigured = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.setup_configured") == 1);
        bool c               = false;
        LOG(INFO) << GREEN << "============================" << RESET;
        LOG(INFO) << BOLDGREEN << "Configure setup" << RESET;
        const auto MAXNRETRY = 100;
        auto       NTrials   = 0;
        while(!SetupConfigured && NTrials < MAXNRETRY)
        {
            if(c == false) LOG(INFO) << "Configuring setup";
            c = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(pWait_ms));
            SetupConfigured = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.setup_configured") == 1);
            NTrials++;
        }

        bool PowerGoodFlagTripped     = (ReadReg("fc7_daq_stat.physical_interface_block.multiplexing_bp.flag_power_good_dropped") == 1);
        auto PowerGoodGlitchesCounter = ReadReg("fc7_daq_stat.physical_interface_block.counter_powergood_glitches");
        LOG(INFO) << BOLDCYAN << "Powergood dropped flag: " << PowerGoodFlagTripped << RESET;
        LOG(INFO) << BOLDCYAN << "Powergood dropped counter: " << PowerGoodGlitchesCounter << RESET;

        if(SetupConfigured)
        {
            LOG(INFO) << GREEN << "============================" << RESET;
            LOG(INFO) << BOLDGREEN << "Setup with backplane " << BackplaneNum << " and card " << CardNum << " is configured" << RESET;
        }
        else
        {                            // The card was not configured. Check all possibles issues
            if(PowerGoodFlagTripped) // Did the power good flag drop?
            {
                LOG(INFO) << GREEN << "============================" << RESET;
                LOG(INFO) << BOLDRED << "Setup is not configured. Problems with card power good signal! Check the HW!" << RESET;

                if(InterlockEnabled) // Is the interlock feature enabled?
                {
                    LOG(INFO) << BOLDMAGENTA << " !! The interlock feature is ENABLED -> the interlock switch might be open !!" << RESET;
                }
            }
            else
            {
                // Otherwise, the card is not present in the system
                LOG(INFO) << GREEN << "============================" << RESET;
                LOG(INFO) << BOLDRED << "Setup is not configured." << BOLDMAGENTA << " The card is not present in the system" << RESET;
            }
        }
    }
}
} // namespace Ph2_HwInterface