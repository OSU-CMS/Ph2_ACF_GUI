/*!

\file                           D19cFWInterface.h
\brief                          D19cFWInterface init/config of the FC7 and its Chip's
\author                         G. Auzinger, K. Uchida, M. Haranko
        \version            1.0
        \date                           24.03.2017
        Support :                       mail to : georg.auzinger@SPAMNOT.cern.ch
                                                  mykyta.haranko@SPAMNOT.cern.ch
                                                  younes.otarid@SPAMNOT.cern.ch

*/

#ifndef _D19CFWINTERFACE_H__
#define _D19CFWINTERFACE_H__

#include "HWInterface/BeBoardFWInterface.h"
#include "Utils/DataContainer.h"
#include "Utils/Event.h"

#include <limits.h>
#include <map>
#include <mutex>
#include <numeric>
#include <stdint.h>
#include <string>
#include <vector>
// #include "Utils/OccupancyAndPh.h"
// #include "Utils/GenericDataVector.h"
#include <uhal/uhal.hpp>

namespace Ph2_HwDescription
{
class BeBoard;
}
/*!
 * \namespace Ph2_HwInterface
 * \brief Namespace regrouping all the interfaces to the hardware
 */
namespace Ph2_HwInterface
{
class L1ReadoutInterface;
class FEConfigurationInterface;
class TriggerInterface;
class FastCommandInterface;
class LinkInterface;
class D19cBackendAlignmentFWInterface;
class D19cDebugFWInterface;
class D19cOpticalInterface;
class D19clpGBTSlowControlWorkerInterface;
class D19cBERTinterface;

/*
 * \brief init/config of the Fc7 and its Chip's
 */
class D19cFWInterface : public BeBoardFWInterface
{
  private:
    FEConfigurationInterface*            fFEConfigurationInterface{nullptr};
    L1ReadoutInterface*                  fL1ReadoutInterface{nullptr};
    TriggerInterface*                    fTriggerInterface{nullptr};
    FastCommandInterface*                fFastCommandInterface{nullptr};
    LinkInterface*                       fLinkInterface{nullptr};
    D19cBackendAlignmentFWInterface*     fBackendAlignmentInterface{nullptr};
    D19cDebugFWInterface*                fDebugInterface{nullptr};
    D19clpGBTSlowControlWorkerInterface* flpGBTSlowControlWorkerInterface{nullptr};
    D19cBERTinterface*                   fBERTinterface{nullptr};

    FileHandler* fFileHandler;
    uint32_t     fBroadcastCbcId;
    uint32_t     fNCic;
    uint32_t     fFMCId;

    // number of chips and hybrids defined in firmware (compiled for)
    uint32_t     fFWNHybrids;
    uint32_t     fFWNChips;
    FrontEndType fFirmwareFrontEndType;
    bool         fCBC3Emulator;
    bool         fIsDDR3Readout;
    bool         fDDR3Calibrated;
    uint32_t     fDDR3Offset;
    // i2c version of master
    uint32_t fI2CVersion;
    // optical readout
    bool fOptical        = false;
    bool fUseOpticalLink = false;
    bool fConfigureCDCE  = false;

    // 2S or PS readout
    bool           fIs2S           = true;
    const uint32_t SINGLE_I2C_WAIT = 200; // used for 1MHz I2C
    // // I'm going to add a variable to hold the stub offset
    // event counter
    uint32_t fEventCounter = 0;

    // some useful stuff
    int fResetAttempts{0};

  public:
    /*!
     *
     * \brief Constructor of the Cbc3Fc7FWInterface class
     * \param puHalConfigFileName : path of the uHal Config File
     * \param pBoardId
     */
    D19cFWInterface(const std::string& puHalConfigFileName, uint32_t pBoardId, Ph2_HwDescription::BeBoard* theBoard);
    D19cFWInterface(const std::string& puHalConfigFileName, uint32_t pBoardId, FileHandler* pFileHandler, Ph2_HwDescription::BeBoard* theBoard);
    /*!
     *
     * \brief Constructor of the Cbc3Fc7FWInterface class
     * \param pId : ID string
     * \param pUri: URI string
     * \param pAddressTable: address tabel string
     */

    void createAuxiliaryInterfaces();

    D19cFWInterface(const std::string& pId, const std::string& pUri, const std::string& pAddressTable, Ph2_HwDescription::BeBoard* theBoard);
    D19cFWInterface(const std::string& pId, const std::string& pUri, const std::string& pAddressTable, FileHandler* pFileHandler, Ph2_HwDescription::BeBoard* theBoard);
    void setFileHandler(FileHandler* pHandler);

    void                                 printReadoutInterface() { LOG(INFO) << BOLDYELLOW << "D19cFWInterface::ReadNEvent L1ReadoutInterface " << fL1ReadoutInterface << RESET; }
    D19cBackendAlignmentFWInterface*     getBackendAlignmentInterface() { return fBackendAlignmentInterface; }
    D19cDebugFWInterface*                getDebugInterface() { return fDebugInterface; }
    TriggerInterface*                    getTriggerInterface() { return fTriggerInterface; }
    L1ReadoutInterface*                  getL1ReadoutInterface() { return fL1ReadoutInterface; }
    FastCommandInterface*                getFastCommandInterface() { return fFastCommandInterface; }
    FEConfigurationInterface*            getFEConfigurationInterface() { return fFEConfigurationInterface; }
    LinkInterface*                       getLinkInterface() { return fLinkInterface; }
    D19clpGBTSlowControlWorkerInterface* getlpGBTSlowControlInterface() { return flpGBTSlowControlWorkerInterface; }
    D19cBERTinterface*                   getBERTinterface() { return fBERTinterface; }
    //
    void ConfigureInterfaces(const Ph2_HwDescription::BeBoard* pBoard);

    /*!
     *
     * \brief Destructor of the Cbc3Fc7FWInterface class
     */

    ~D19cFWInterface();

    ///////////////////////////////////////////////////////
    //      d19c Methods                                //
    /////////////////////////////////////////////////////

    // initialize interfaces to handle communication with certain blocks
    void InitializePSCounterFWInterface(const Ph2_HwDescription::BeBoard* pBoard);
    void InitalizeL1ReadoutInterface(const Ph2_HwDescription::BeBoard* pBoard);

    // uint16_t ParseEvents(const std::vector<uint32_t>& pData) override;
    /*! \brief Read a block of a given size
     * \param pRegNode Param Node name
     * \param pBlocksize Number of 32-bit words to read
     * \return Vector of validated 32-bit values
     */
    std::vector<uint32_t> ReadBlockRegValue(const std::string& pRegNode, const uint32_t& pBlocksize) override;

    /*! \brief Read a block of a given size
     * \param pRegNode Param Node name
     * \param pBlocksize Number of 32-bit words to read
     * \param pBlockOffset Offset of the block
     * \return Vector of validated 32-bit values
     */
    std::vector<uint32_t> ReadBlockRegOffsetValue(const std::string& pRegNode, const uint32_t& pBlocksize, const uint32_t& pBlockOffset);

    bool WriteBlockReg(const std::string& pRegNode, const std::vector<uint32_t>& pValues) override;
    /*!
     * \brief Get the FW info
     */
    uint32_t getBoardInfo();

    uint32_t getBoardFirmwareVersion() override { return 999; }

    BoardType getBoardType() const { return BoardType::D19C; }
    /*!
     * \brief Configure the board with its Config File
     * \param pBoard
     */
    void ConfigureBoard(const Ph2_HwDescription::BeBoard* pBoard) override;
    /*!
     * \brief Detect the right Hybrid Id to write the right registers (not working with the latest Firmware)
     */

    void     ResetEventCounter() { fEventCounter = 0; }
    uint32_t GetEventCounter() { return fEventCounter; }
    /*!
     * \brief Status of triggers
     */
    /*!
     * \brief Start a DAQ
     */
    void Start(const Ph2_HwDescription::BeBoard* pBoard) override;
    /*!
     * \brief Stop a DAQ
     */
    void Stop() override;
    /*!
     * \brief Pause a DAQ
     */
    void Pause() override;
    /*!
     * \brief Unpause a DAQ
     */
    void Resume() override;

    /*!
     * \brief DDR3 Self-test
     */
    void DDR3SelfTest();

    /*!
     * \brief Read data from DAQ
     * \param pBreakTrigger : if true, enable the break trigger
     * \return fNpackets: the number of packets read
     */
    uint32_t ReadData(Ph2_HwDescription::BeBoard* pBoard, bool pBreakTrigger, std::vector<uint32_t>& pData, bool pWait = true) override;

    /*!
     * \brief Read data for pNEvents
     * \param pBoard : the pointer to the BeBoard
     * \param pNEvents :  the 1 indexed number of Events to read - this will set the packet size to this value -1
     */

    void ReadNEvents(Ph2_HwDescription::BeBoard* pBoard, uint32_t pNEvents, std::vector<uint32_t>& pData, bool pWait = true);
    // FMCs
    void InitFMCPower();

  private:
    uint32_t fReadoutAttempts = 0;
    uint16_t fWait_us         = 10000; // 10 ms

    // get data from FC7
    // split data per hybrid/chip for a given board
    uint32_t computeEventSize(Ph2_HwDescription::BeBoard* pBoard);

    // binary predicate for comparing sent I2C commands with replies using std::mismatch
    static bool cmd_reply_comp(const uint32_t& cWord1, const uint32_t& cWord2);
    static bool cmd_reply_ack(const uint32_t& cWord1, const uint32_t& cWord2);

    // ########################################
    // # FMC powering/control/configuration  #
    // ########################################
    void powerAllFMCs(bool pEnable = false);
    // dedicated method to power on dio5
    void PowerOnDIO5(uint8_t pFMCId);
    // get fmc card name
    std::string getFMCCardName(uint32_t id);
    // convert code of the chip from firmware
    std::string  getChipName(uint32_t pChipCode);
    FrontEndType getFrontEndType(uint32_t pChipCode);

    // FMC Maps
    std::map<uint32_t, std::string> fFMCMap = {{0, "NONE"},
                                               {1, "DIO5"},
                                               {2, "2CBC2"},
                                               {3, "8CBC2"},
                                               {4, "2CBC3"},
                                               {5, "8CBC3_1"},
                                               {6, "8CBC3_2"},
                                               {7, "1CBC3"},
                                               {8, "MPA_SSA"},
                                               {9, "FERMI_TRIGGER"},
                                               {10, "CIC1_FMC1"},
                                               {11, "CIC1_FMC2"},
                                               {12, "PS_FMC1"},
                                               {13, "PS_FMC2"},
                                               {14, "2S_FMC1"},
                                               {15, "2S_FMC2"},
                                               {16, "2S"},
                                               {17, "OPTO_QUAD"},
                                               {18, "OPTO_OCTA"},
                                               {19, "FMC_FE_FOR_PS_ROH_FMC1"},
                                               {20, "FMC_FE_FOR_PS_ROH_FMC2"}};

    std::map<uint32_t, std::string> fChipNamesMap = {{1, "CBC3"}, {2, "MPA2"}, {3, "SSA2"}, {5, "CIC2"}};

    std::map<uint32_t, FrontEndType> fFETypesMap = {{1, FrontEndType::CBC3}, {2, FrontEndType::MPA2}, {3, FrontEndType::SSA2}, {5, FrontEndType::CIC2}};

    // template to copy every nth element out of a vector to another vector
    template <class in_it, class out_it>
    out_it copy_every_n(in_it b, in_it e, out_it r, size_t n)
    {
        for(size_t i = std::distance(b, e) / n; i--; std::advance(b, n)) *r++ = *b;

        return r;
    }

  public:
    void EnableFrontEnds(const Ph2_HwDescription::BeBoard* pBoard);
    void ChipReSync() override;

    void ChipReset() override;

    void ChipTrigger() override;

    void ChipTestPulse() override;

    void ReadoutChipReset();
    // CIC BE stuff
    bool Bx0Alignment();
    // TP FSM
    void ConfigureTestPulseFSM(uint16_t pDelayAfterFastReset = 1,
                               uint16_t pDelayAfterTP        = 200,
                               uint16_t pDelayBeforeNextTP   = 400,
                               uint8_t  pEnableFastReset     = 1,
                               uint8_t  pEnableTP            = 1,
                               uint8_t  pEnableL1A           = 1);
    // trigger FSM
    void ConfigureTriggerFSM(uint16_t pNtriggers = 100, uint16_t pTriggerRate = 100, uint8_t pSource = 3, uint8_t pStubsMask = 0, uint8_t pStubLatency = 50);
    // consecutive triggers FSM
    void ConfigureConsecutiveTriggerFSM(uint16_t pNtriggers = 32, uint16_t pDelayBetweenTriggers = 1, uint16_t pDelayToNext = 1);
    // back-end tuning for CIC data
    void ConfigureFastCommandBlock(const Ph2_HwDescription::BeBoard* pBoard);
    // consecutive triggers FSM
    void ConfigureAntennaFSM(uint16_t pNtriggers = 1, uint16_t pTriggerRate = 1, uint16_t pL1Delay = 100);

    void configureLinks(const Ph2_HwDescription::BeBoard* pBoard);
    void configureTxRxPolarities(const Ph2_HwDescription::BeBoard* pBoard);
    void configureLpGbtVersions(const Ph2_HwDescription::BeBoard* pBoard);

    // CDCE
    void configureCDCE_old(uint16_t pClockRate = 120);
    void configureCDCE(uint16_t pClockRate = 120, std::pair<std::string, float> pCDCEselect = std::make_pair("sec", 40));
    void syncCDCE();
    void epromCDCE();

    void SetForceStart(bool bStart) override {}

    ///////////////////////////////////////////////////////
    //      Optical readout                                 //
    /////////////////////////////////////////////////////

    // ##############################
    // # Pseudo Random Bit Sequence #
    // ##############################
    std::vector<double> RunBERtest(bool given_time, double frames_or_time, const std::map<uint32_t, std::vector<uint8_t>>& hybrid_id_chip_id_chip_lanes, uint8_t frontendSpeed) override { return {}; };

    // ############################
    // # Read/Write Optical Group #
    // ############################
    // Functions for standard uDTC
    void     SetOptoLinkVersion(uint8_t version) override {};
    void     selectLink(const uint8_t pLinkId = 0, uint32_t cWait_ms = 100) override {};
    void     StatusOptoLink(uint32_t& txStatus, uint32_t& rxStatus, uint32_t& mgtStatus) override {}
    void     ResetOptoLink() override {};
    bool     WriteOptoLinkRegister(const Ph2_HwDescription::Chip* pChip, const uint32_t pAddress, const uint32_t pData, const bool pVerifLoop = false) override { return true; };
    uint32_t ReadOptoLinkRegister(const Ph2_HwDescription::Chip* pChip, const uint32_t pAddress) override { return 0; };

    // Generic FE configuration functions
    // single register functions
    // Register write
    bool SingleRegisterWrite(Ph2_HwDescription::Chip* pChip, Ph2_HwDescription::ChipRegItem& pItem, bool pVerify = true) override;
    bool MultiRegisterWrite(Ph2_HwDescription::Chip* pChip, std::vector<Ph2_HwDescription::ChipRegItem>& pItem, bool pVerify = true) override;
    // Register write + read-back
    bool SingleRegisterWriteRead(Ph2_HwDescription::Chip* pChip, Ph2_HwDescription::ChipRegItem& pItem) override;
    bool MultiRegisterWriteRead(Ph2_HwDescription::Chip* pChip, std::vector<Ph2_HwDescription::ChipRegItem>& pItem) override;
    // Register read
    uint8_t              SingleRegisterRead(Ph2_HwDescription::Chip* pChip, Ph2_HwDescription::ChipRegItem& pItem) override;
    std::vector<uint8_t> MultiRegisterRead(Ph2_HwDescription::Chip* pChip, std::vector<Ph2_HwDescription::ChipRegItem>& pItem) override;

    // fast command generic block
    void  ResetFCMDBram();
    void  ConfigureFCMDBram(std::vector<uint8_t> pFastCommands);
    float GetSFPParameter(std::string parameter, int channel, bool isL8);
    float GetSFPParameter_L8(std::string parameter, int channel) { return GetSFPParameter(parameter, channel, true); };
    float GetSFPParameter_L12(std::string parameter, int channel) { return GetSFPParameter(parameter, channel, false); };
    float GetSFPParameter(Ph2_HwDescription::OpticalGroup* theOpticalGroup, std::string parameter);

    void vtrxHardReset(Ph2_HwDescription::OpticalGroup* theOpticalGroup);

    std::vector<uint32_t>              L1ADebug(uint8_t pWait_ms, bool pPrint);
    std::vector<std::vector<uint32_t>> StubDebug(bool pWithTestPulse, uint8_t pNlines, bool pPrint);
};
} // namespace Ph2_HwInterface

#endif
