/*!
  \file                  RD53FWInterface.h
  \bri7ef                 RD53FWInterface to initialize and configure the FW
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53FWInterface_H
#define RD53FWInterface_H

#include "HWDescription/RD53.h"
#include "HWDescription/RD53ACommands.h"
#include "HWInterface/BeBoardFWInterface.h"
#include "HWInterface/RD53lpGBTInterface.h"
#include "Utils/RD53Event.h"
#include "Utils/RD53RunProgress.h"
#include "Utils/RD53Shared.h"

#include <uhal/uhal.hpp>

namespace Ph2_HwDescription
{
class BeBoard;
}

// #######################
// # FW useful constants #
// #######################
namespace RD53FWconstants
{
const uint8_t  NMAXCHIP_HYBRID      = 4;      // Maximum number of chips in a hybrid
const uint8_t  NLANE_DUALHYBRID     = 2;      // Number of lanes per dual hybrid
const uint8_t  NLANE_QUADHYBRID     = 4;      // Number of lanes per quad hybrid
const uint8_t  HEADEAR_WRTCMD       = 0xFF;   // Header of chip write command sequence
const uint8_t  NBIT_FWVER           = 16;     // Number of bits for the firmware version
const uint16_t NBIT_SLOWCMD_FIFO    = 16;     // Slow command FIFO depth 65.536, i.e. 16 bits (in terms of 32-bit words)
const uint16_t NBIT_DATA_FIFO       = 27;     // Data FIFO depth 134.217.728, i.e. 27 bits (in terms of 32-bit words)
const uint8_t  AURORA_SPEED         = 0;      // 0 = 1.28 Gbps, 1 = 640 Mbps
const uint8_t  IPBUS_FASTDURATION   = 1;      // Duration of a fast command in terms of 40 MHz clk cycles
const uint16_t EVENT_STREAM_TIMEOUT = 0xFFFF; // Event stream timeout

enum ReadoutSpeed : uint8_t
{
    x1280,
    x640,
    x320
};

} // namespace RD53FWconstants

namespace Ph2_HwInterface
{
class RD53FWInterface : public BeBoardFWInterface
{
  public:
    RD53FWInterface(const std::string& pId, const std::string& pUri, const std::string& pAddressTable, Ph2_HwDescription::BeBoard* theBoard);
    ~RD53FWInterface() { delete fFileHandler; }

    // #############################
    // # Override member functions #
    // #############################
    void      setFileHandler(FileHandler* pHandler) override;
    uint32_t  getBoardInfo() override { return FWinfo; }
    uint32_t  getBoardFirmwareVersion() override { return FWinfo; }
    BoardType getBoardType() const override { return BoardType::RD53; }

    void ResetSequence(const std::string& refClockRate);
    void ConfigureBoard(const Ph2_HwDescription::BeBoard* pBoard) override;
    void PrintFWstatus() override;

    void Start(const Ph2_HwDescription::BeBoard* pBoard) override;
    void Stop() override;
    void Pause() override;
    void Resume() override;

    std::vector<double> RunBERtest(bool given_time, double frames_or_time, const std::map<uint32_t, std::vector<uint8_t>>& hybrid_id_chip_id_chip_lanes, uint8_t frontendSpeed) override;
    void                ReadNEvents(Ph2_HwDescription::BeBoard* pBoard, uint32_t pNEvents, std::vector<uint32_t>& pData, bool pWait = true) override;
    uint32_t            ReadData(Ph2_HwDescription::BeBoard* pBoard, bool pBreakTrigger, std::vector<uint32_t>& pData, bool pWait = true) override;
    void                ChipReset() override;
    void                ChipReSync() override;

    void selectLink(const uint8_t pLinkId, uint32_t pWait_ms = 100) override;
    void SetOptoLinkVersion(uint8_t version) override;
    // #############################
    float GetSFPParameter(std::string parameter, int channel);

    void ConfigurePCTestAdapter(const std::string& config);
    void SelectBERcheckBitORFrame(const uint8_t bitORframe);
    void WriteArbitraryRegister(const std::string&                regName,
                                const uint32_t                    value,
                                const Ph2_HwDescription::BeBoard* pBoard                = nullptr,
                                ReadoutChipInterface*             pReadoutChipInterface = nullptr,
                                const bool                        doReset               = false);
    void ResetBoard();
    void ResetFastCmdBlk();
    void ResetSlowCmdFIFO();
    void ResetReadBkFIFO();
    void ResetReadoutBlk();
    bool silentRunning{false};
    bool showRunProgress{false};

    // ####################################
    // # Check AURORA lock on data stream #
    // ####################################
    bool                          CheckChipCommunication(const Ph2_HwDescription::BeBoard* pBoard);
    RD53FWconstants::ReadoutSpeed ReadoutSpeed();
    bool                          getChipCommunicationStatus() { return isChipCommunicationOK; }
    size_t                        getNcorruptedNevents() { return NcorruptedNevents; }
    void                          resetNcorruptedNevents() { NcorruptedNevents = 0; }
    size_t                        getNtrialsNevents() { return NtrialsNevents; }
    void                          resetNtrialsNevents() { NtrialsNevents = 0; }

    // #############################################
    // # hybridId < 0 --> broadcast to all hybrids #
    // #############################################
    bool                                       WriteChipCommands(const std::vector<uint16_t>& data, int hybridId);
    void                                       ComposeAndPackChipCommands(const std::vector<uint16_t>& data, int hybridId, std::vector<uint32_t>& commandList);
    bool                                       SendChipCommands(const std::vector<uint32_t>& commandList);
    std::vector<std::pair<uint16_t, uint16_t>> ReadChipRegisters(Ph2_HwDescription::ReadoutChip* pChip);

    enum class TriggerSource : uint32_t
    {
        IPBus = 1,
        FastCMDFSM,
        TTC,
        TLU,
        External,
        HitOr,
        UserDefined,
        Undefined = 0
    };

    enum class AutozeroSource : uint32_t // @TMP@
    {
        Software = 1,
        FastCMDFSM,
        UserDefined, // --> Related to IPbus register "autozero_freq"
        Disabled = 0
    };

    struct FastCmdFSMConfig
    {
        bool ecr_en        = false;
        bool first_cal_en  = false;
        bool second_cal_en = false;
        bool trigger_en    = false;

        uint32_t first_cal_data  = 0;
        uint32_t second_cal_data = 0;

        uint32_t delay_after_first_prime = 0;
        uint32_t delay_after_ecr         = 0;
        uint32_t delay_after_autozero    = 0; // @TMP@
        uint32_t delay_after_inject      = 0;
        uint32_t delay_after_trigger     = 0;
        uint32_t delay_after_prime       = 0;
    };

    struct FastCommandsConfig
    {
        TriggerSource  trigger_source  = TriggerSource::FastCMDFSM;
        AutozeroSource autozero_source = AutozeroSource::Disabled; // @TMP@

        bool initial_ecr_en  = false;
        bool backpressure_en = false;
        bool veto_en         = false;

        uint32_t n_triggers        = 0;
        uint32_t ext_trigger_delay = 0; // Used when trigger_source == TriggerSource::External
        uint32_t trigger_duration  = 0; // Number of triggers on top of the L1A (maximum value is 31)
        uint32_t enable_hitor      = 0; // Enable HitOr signals

        FastCmdFSMConfig fast_cmd_fsm;

        static const std::array<std::string, 8> fastCmdWhiteList;
    };

    void ConfigureFromXML(const Ph2_HwDescription::BeBoard* pBoard);
    void ConfigureFastCommands(const Ph2_HwDescription::BeBoard* pBoard,
                               const uint32_t                    nTRIGxEvent,
                               const RD53Shared::INJtype         injType,
                               const uint32_t                    injLatency     = 0,
                               const uint32_t                    nClkDelays     = 0,
                               const bool                        enableAutozero = false);
    void SendFastCommands(const FastCommandsConfig* config = nullptr);

    struct DIO5Config
    {
        bool     enable             = false;
        bool     ext_clk_en         = false;
        uint32_t ch_out_en          = 0; // chn-1 = clk. to TLU, chn-2 = ext. trigger, chn-3 = busy to TLU, chn-4 = HitOr out, chn-5 = ext. clk.
        uint32_t fiftyohm_en        = 0x12;
        uint32_t ch1_thr            = 0x80; // [(thr/256*(5-1)V + 1V) * 3.3V/5V]
        uint32_t ch2_thr            = 0x80;
        uint32_t ch3_thr            = 0x80;
        uint32_t ch4_thr            = 0x80;
        uint32_t ch5_thr            = 0x80;
        bool     tlu_en             = false;
        uint32_t tlu_handshake_mode = 0; // 0 = simple handshake, 2 = data handshake
    };

    FastCommandsConfig* getLocalCfgFastCmd() { return &localCfgFastCmd; }

    // ###################################
    // # Read/Write Status Optical Group #
    // ###################################
    void     ResetOptoLink() override;
    void     StatusOptoLink(uint32_t& txStatus, uint32_t& rxStatus, uint32_t& mgtStatus) override;
    bool     WriteOptoLinkRegister(const Ph2_HwDescription::Chip* pChip, const uint32_t pAddress, const uint32_t pData, const bool pVerify = true) override;
    uint32_t ReadOptoLinkRegister(const Ph2_HwDescription::Chip* pChip, const uint32_t pAddress) override;
    void     ResetOptoLinkSlowControl();
    void     SetDownLinkMapping(uint8_t TxLink, uint8_t TxGroup, uint8_t TxModuleId);
    void     SetUpLinkMapping(uint8_t RxLink, uint8_t ModuleId, uint8_t ChipId, const std::vector<std::pair<uint8_t, uint8_t>>& RxGroupsChipLanes);

    // ####################################################
    // # Hybrid ADC measurements: temperature and voltage #
    // ####################################################
    float ReadHybridTemperature(int hybridId, bool silentRunning = false);
    float ReadHybridVoltage(int hybridId, bool silentRunning = false);
    float calcTemperature(uint32_t sensor1, uint32_t sensor2, int beta = 3435);
    float calcVoltage(uint32_t senseVDD, uint32_t senseGND);

  private:
    void     TurnOffFMC();
    void     TurnOnFMC();
    void     ConfigureDIO5(const Ph2_HwDescription::BeBoard* pBoard, DIO5Config* config);
    void     SendDIO5Cfg(const DIO5Config* config);
    void     SendBoardCommandWithStrobe(const std::string& cmdReg);
    void     ToggleRegister(const std::string& cmdReg, bool do1and0 = true);
    uint32_t GetBoardEnabledChips(const Ph2_HwDescription::BeBoard* pBoard, bool primariesOnly = false);
    uint32_t GetBoardEnabledHybrids(const Ph2_HwDescription::BeBoard* pBoard);

    // ###################
    // # Clock generator #
    // ###################
    void InitializeClockGenerator(const std::string& refClockRate = "160", bool doStoreInEEPROM = false);
    void ReadClockGenerator();

    FastCommandsConfig localCfgFastCmd;
    size_t             ddr3Offset;
    uint8_t            hybridType;
    uint32_t           FWinfo;
    uint32_t           enabledHybrids;
    bool               isChipCommunicationOK{false};
    size_t             NcorruptedNevents{0};
    size_t             NtrialsNevents{0};
};

} // namespace Ph2_HwInterface

#endif
