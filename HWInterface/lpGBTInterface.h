/*!
  \file                  lpGBTInterface.h
  \brief                 Interface to access and control the low-power Gigabit Transceiver chip
  \author                Younes Otarid
  \version               1.0
  \date                  03/03/20
  Support:               email to younes.otarid@cern.ch
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef lpGBTInterface_H
#define lpGBTInterface_H

#include "HWDescription/OpticalGroup.h"
#include "HWDescription/lpGBT.h"
#include "HWInterface/ChipInterface.h"
#include "HWInterface/ReadoutChipInterface.h"

#if defined(__TCUSB__)
#include "HWInterface/TCInterface.h"
#endif

// ##########################
// # LpGBT useful constants #
// ##########################
namespace lpGBTconstants
{
const uint8_t  PATTERN_PRBS      = 0x1;    // Start PRBS pattern
const uint8_t  PATTERN_NORMAL    = 0x0;    // Start normal-mode pattern
const uint8_t  PATTERN_CONST     = 0x4;    // Constant pattern set by DP pattern
const uint8_t  PATTERN_CONST_INV = 0x5;    // Inverted constant pattern
const uint8_t  RxPhaseTracking   = 2;      // Rx phase tracking mode [0 = no-tracking, 2 = automatic-tracking]
const uint32_t DEEPSLEEP         = 100000; // [microseconds]
const uint8_t  MAXATTEMPTS       = 40;     // Maximum number of attempts
const float    ACCELERATOR_CLK   = 40e6;   // Accelerator clock frequency [Hz]
} // namespace lpGBTconstants

namespace Ph2_HwDescription
{
class lpGBT;
}

namespace Ph2_HwInterface
{
struct lpGBTClockConfig
{
    uint8_t fClkFreq = 4, fClkDriveStr = 1, fClkInvert = 1, fI2CFreq = 2;
    uint8_t fClkPreEmphWidth = 0, fClkPreEmphMode = 0, fClkPreEmphStr = 0;
};

class lpGBTInterface : public ChipInterface
{
  public:
    lpGBTInterface(const BeBoardFWMap& pBoardMap) : ChipInterface(pBoardMap) {}
    virtual ~lpGBTInterface() {}

    void StartPRBSpattern(Ph2_HwDescription::Chip* pChip);
    void StopPRBSpattern(Ph2_HwDescription::Chip* pChip);

    // #####################################
    // # Phase alignment virtual functions #
    // #####################################
    virtual void
    PhaseAlignRx(Ph2_HwDescription::Chip* pChip, const Ph2_HwDescription::BeBoard* pBoard, const Ph2_HwDescription::OpticalGroup* pOpticalGroup, ReadoutChipInterface* pReadoutChipInterface) = 0;

    // ################################
    // # Chip configuration functions #
    // ################################
    bool     WriteChipReg(Ph2_HwDescription::Chip* pChip, const std::string& pDacName, uint16_t pDacValue, bool pVerify = true) override;
    int32_t  ReadChipReg(Ph2_HwDescription::Chip* pChip, const std::string& pRegNode) override;
    uint32_t ReadChipFuseID(Ph2_HwDescription::Chip* pChip, uint8_t version = 1) override;
    uint32_t ReadChipFusedBlock(Ph2_HwDescription::Chip* pChip, uint8_t cFuseH, uint8_t cFuseL);
    bool     WriteChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::pair<std::string, uint16_t>>& RegVec, bool pVerify = true) override;
    uint32_t ReadVTRxChipFuseID(Ph2_HwDescription::Chip* pChip);
    void     DumpChipRegisters(Ph2_HwDescription::Chip* pChip);

    // #######################################
    // # LpGBT block configuration functions #
    // #######################################
    void     SetPUSMDone(Ph2_HwDescription::Chip* pChip, bool pPllConfigDone, bool pDllConfigDone);
    void     ConfigureRxAlignmentMode(Ph2_HwDescription::Chip* pChip, const std::vector<uint8_t>& pGroups, uint8_t pTrackMode);
    uint16_t GetRxDataRate(Ph2_HwDescription::Chip* pChip, uint8_t pGroup);
    uint8_t  GetChipRate(Ph2_HwDescription::Chip* pChip);
    void     ConfigureRxGroup(Ph2_HwDescription::Chip* pChip, uint8_t pGroup, uint8_t pChannel, uint8_t pDataRate, uint8_t pTrackMode);
    void     ConfigureRxChannel(Ph2_HwDescription::Chip* pChip, uint8_t pGroup, uint8_t pChannel, uint8_t pEqual, uint8_t pTerm, uint8_t pAcBias, uint8_t pInvert, uint8_t pPhase);
    void     ConfigureTxGroup(Ph2_HwDescription::Chip* pChip, uint8_t pGroup, uint8_t pChannel, uint8_t pDataRate);
    void     ConfigureTxChannel(Ph2_HwDescription::Chip* pChip, uint8_t pGroup, uint8_t pChannel, uint8_t pDriveStr, uint8_t pPreEmphMode, uint8_t pPreEmphStr, uint8_t pPreEmphWidth, uint8_t pInvert);
    void     ConfigureClocks(Ph2_HwDescription::Chip*    pChip,
                             const std::vector<uint8_t>& pClock,
                             uint8_t                     pFreq,
                             uint8_t                     pDriveStr,
                             uint8_t                     pInvert,
                             uint8_t                     pPreEmphWidth,
                             uint8_t                     pPreEmphMode,
                             uint8_t                     pPreEmphStr);
    void     ConfigureHighSpeedPolarity(Ph2_HwDescription::Chip* pChip, uint8_t pOutPolarity, uint8_t pInPolarity);
    void     ConfigureDPPattern(Ph2_HwDescription::Chip* pChip, uint32_t pPattern);
    void     ConfigureRxPRBS(Ph2_HwDescription::Chip* pChip, uint8_t pGroup, uint8_t pChannel, bool pEnable);
    void     ConfigureRxSource(Ph2_HwDescription::Chip* pChip, uint8_t pGroup, uint8_t pSource);
    void     ConfigureTxSource(Ph2_HwDescription::Chip* pChip, uint8_t pGroup, uint8_t pSource);
    bool     ConfigureRxPhase(Ph2_HwDescription::Chip* pChip, uint8_t pGroup, uint8_t pChannel, uint8_t pPhase);
    void     ConfigureAllRxPhase(Ph2_HwDescription::Chip* pChip, uint8_t pPhase, std::map<uint8_t, std::vector<uint8_t>> theGroupsAndChannels);
    void     ConfigurePhShifter(Ph2_HwDescription::Chip* pChip, const std::vector<uint8_t>& pClocks, uint8_t pFreq, uint8_t pDriveStr, uint8_t pEnFTune, uint16_t pDelay);
    void     SetPhaseTap(Ph2_HwDescription::Chip* pChip, uint8_t pGroup, uint8_t pChannel, uint8_t pPhase);
    uint8_t  GetPhaseTap(Ph2_HwDescription::Chip* pChip, uint8_t pGroup, uint8_t pChannel); // To-Do: change to a map

    // ####################################
    // # LpGBT specific routine functions #
    // ####################################
    bool    ConfigureVref(Ph2_HwDescription::Chip* pChip, uint8_t pEnable, uint8_t pCorrection);
    bool    EnableInternalVref(Ph2_HwDescription::Chip* pChip, bool pEnable);
    bool    SetVrefTune(Ph2_HwDescription::Chip* pChip, uint8_t pVrefTune);
    uint8_t GetVrefTune(Ph2_HwDescription::Chip* pChip);
    float   GetVref(Ph2_HwDescription::Chip* pChip, const std::string& pADC, uint16_t pVinput);
    uint8_t TuneVref(Ph2_HwDescription::Chip* pChip);
    void    PhaseTrainRx(Ph2_HwDescription::Chip* pChip, const std::vector<uint8_t>& pGroups);
    void    ResetRxDll(Ph2_HwDescription::Chip* pChip, const std::vector<uint8_t>& pGroups);
    void    hardReset(Ph2_HwDescription::Chip* pChip);

    // ################################
    // # LpGBT block status functions #
    // ################################
    bool    IsPUSMDone(Ph2_HwDescription::Chip* pChip);
    uint8_t PrintChipMode(Ph2_HwDescription::Chip* pChip);
    uint8_t GetPUSMStatus(Ph2_HwDescription::Chip* pChip);
    uint8_t GetRxPhase(Ph2_HwDescription::Chip* pChip, uint8_t pGroup, uint8_t pChannel);
    bool    IsRxLocked(Ph2_HwDescription::Chip* pChip, uint8_t pGroup);
    uint8_t GetRxDllStatus(Ph2_HwDescription::Chip* pChip, uint8_t pGroup);
    uint8_t GetSFPchannel(const Ph2_HwDescription::OpticalGroup* pOpticalGroup);

    // ########################
    // # LpGBT GPIO functions #
    // ########################
    void ConfigureGPIODirection(Ph2_HwDescription::Chip* pChip, const std::vector<uint8_t>& pGPIOs, uint8_t pDir);
    void ConfigureGPIOLevel(Ph2_HwDescription::Chip* pChip, const std::vector<uint8_t>& pGPIOs, uint8_t pOut);
    void ConfigureGPIODriverStrength(Ph2_HwDescription::Chip* pChip, const std::vector<uint8_t>& pGPIOs, uint8_t pDriveStr);
    void ConfigureGPIOPull(Ph2_HwDescription::Chip* pChip, const std::vector<uint8_t>& pGPIOs, uint8_t pPullEn, uint8_t pPullUpDown);
    bool ReadGPIO(Ph2_HwDescription::Chip* pChip, uint8_t pGPIO);

    // ###########################
    // # LpGBT ADC-DAC functions #
    // ###########################
    void     ConfigureADC(Ph2_HwDescription::Chip* pChip, uint8_t pGainSelect, bool pADCEnable, bool pStartConversion);
    void     ConfigureCurrentDAC(Ph2_HwDescription::Chip* pChip, const std::vector<std::string>& pCurrentDACChannels, uint8_t pCurrentDACOutput);
    void     ConfigureInternalMonitoring(Ph2_HwDescription::Chip* pChip, uint8_t pEnable);
    float    GetInternalTemperature(Ph2_HwDescription::Chip* pChip);
    float    ReadResistance(Ph2_HwDescription::Chip* pChip, const std::string& pADC, const std::vector<uint8_t>& pCurrents, uint8_t pGain = 0);
    uint16_t GetADCOffset(Ph2_HwDescription::Chip* pChip, bool pVerbose = true);
    float    GetADCVoltage(Ph2_HwDescription::Chip* pChip, const std::string& pADCInputP, uint16_t cOffset, float cGain, bool pVerbose = true);
    float    GetADCVoltage(Ph2_HwDescription::Chip* pChip, const std::string& pADCInputP, bool pVerbose = true);
    float    GetRssiPower(Ph2_HwDescription::Chip* pChip, const std::string& pADCInputP, float cResponsivity, uint16_t cOffset, float cGain, bool pVerbose = true);
    float    GetRssiPower(Ph2_HwDescription::Chip* pChip, const std::string& pADCInputP, float cResponsivity, bool pVerbose = true);
    float    GetADCGain(Ph2_HwDescription::Chip* pChip, bool pVerbose = true);
    uint16_t ReadADC(Ph2_HwDescription::Chip* pChip, const std::string& pADCInputP, const std::string& pADCInputN = "VREF/2", uint8_t pGain = 0, bool silentRunning = false);
    bool     IsReadADCDone(Ph2_HwDescription::Chip* pChip);

    // #############################
    // # LpGBT Bit Error Rate test #
    // #############################
    void     ConfigureBERT(Ph2_HwDescription::Chip* pChip, uint8_t pCoarseSource, uint8_t pFineSource, uint8_t pMeasTime, bool pSkipDisable = false);
    void     StartBERT(Ph2_HwDescription::Chip* pChip, bool pStartBERT = true);
    void     ConfigureBERTPattern(Ph2_HwDescription::Chip* pChip, uint32_t pPattern);
    uint8_t  GetBERTStatus(Ph2_HwDescription::Chip* pChip);
    bool     IsBERTDone(Ph2_HwDescription::Chip* pChip);
    bool     IsBERTEmptyData(Ph2_HwDescription::Chip* pChip);
    uint64_t GetBERTErrors(Ph2_HwDescription::Chip* pChip);
    double   GetBERTResult(Ph2_HwDescription::Chip* pChip);
    double   RunBERtest(Ph2_HwDescription::Chip* pChip, const std::vector<uint8_t>& pGroup, uint8_t pChannel, bool given_time, double frames_or_time, uint8_t frontendSpeed);

    // ####################################
    // # LpGBT eye opening monitor tester #
    // ####################################
    void     ConfigureEOM(Ph2_HwDescription::Chip* pChip, uint8_t pEndOfCountSelect, bool pByPassPhaseInterpolator = false, bool pEnableEOM = true);
    void     StartEOM(Ph2_HwDescription::Chip* pChip, bool pStartEOM = true);
    void     SelectEOMPhase(Ph2_HwDescription::Chip* pChip, uint8_t pPhase);
    void     SelectEOMVof(Ph2_HwDescription::Chip* pChip, uint8_t pVof);
    uint8_t  GetEOMStatus(Ph2_HwDescription::Chip* pChip);
    uint16_t GetEOMCounter(Ph2_HwDescription::Chip* pChip);

    // ##############################################
    // # LpGBT I2C Masters functions (Slow Control) #
    // ##############################################
    void        ResetI2C(Ph2_HwDescription::Chip* pChip, const std::vector<uint8_t>& pMasters);
    void        ConfigureI2C(Ph2_HwDescription::Chip* pChip, uint8_t pMaster, uint8_t pFreq, uint8_t pNBytes, uint8_t pSCLDriveMode, bool verify = true);
    uint8_t     GetI2CConfiguration(Ph2_HwDescription::Chip* pChip, uint8_t pMaster);
    bool        WriteI2C(Ph2_HwDescription::Chip* pChip, uint8_t pMaster, uint8_t pSlaveAddress, uint32_t pData, uint8_t pNBytes, uint8_t pFreq = 3 /* 3   1 MHz */, bool verify = true);
    uint32_t    ReadI2C(Ph2_HwDescription::Chip* pChip, uint8_t pMaster, uint8_t pSlaveAddress, uint8_t pNBytes, uint8_t pFreq = 3 /* 3   1 MHz */);
    uint8_t     GetI2CStatus(Ph2_HwDescription::Chip* pChip, uint8_t pMaster);
    std::string GetI2CState(Ph2_HwDescription::Chip* pChip, uint8_t pStatus);
    bool        IsI2CSuccess(Ph2_HwDescription::Chip* pChip, uint8_t pMaster);

    // #############
    // # LpGBT map #
    // #############
    std::map<std::string, uint8_t> fADCInputMap = {{"ADC0", 0},
                                                   {"ADC1", 1},
                                                   {"ADC2", 2},
                                                   {"ADC3", 3},
                                                   {"ADC4", 4},
                                                   {"ADC5", 5},
                                                   {"ADC6", 6},
                                                   {"ADC7", 7},
                                                   {"EOM_DAC", 8},
                                                   {"VDDIO", 9},
                                                   {"VDDTX", 10},
                                                   {"VDDRX", 11},
                                                   {"VDD", 12},
                                                   {"VDDA", 13},
                                                   {"TEMP", 14},
                                                   {"VREF/2", 15}};
    std::map<uint8_t, std::string> fADCGainMap  = {{0, "X2"}, {1, "X8"}, {2, "X16"}, {3, "X32"}}; // WARNING: for X32 no calibration

    // # The calibration values in the dictionary below correspond to the average value
    // # of calibration coefficients (obtained for all chips) and will be used if
    // # no per-chip calibration data is loaded. In order to improve the quality
    // # of calibration the user is expected to call load_calibration_data method.

    void    LoadCalibrationData(Ph2_HwDescription::lpGBT* pChip, uint32_t pChipId, std::string pFileName = expandEnvironmentVariables("${PH2ACF_BASE_DIR}/settings/lpGBTFiles/lpgbt_calibration.csv"));
    float   EstimateTemperatureUncalibVref(Ph2_HwDescription::lpGBT* pChip, bool pResetTempSensor = true);
    void    TuneVrefControlLib(Ph2_HwDescription::lpGBT* pChip, bool pEnable = true);
    void    AutoTuneVref(Ph2_HwDescription::lpGBT* pChip, bool pResetTempSensor = true);
    float   AdcGetVin(Ph2_HwDescription::lpGBT* pChip, const std::string& pADCInputP, const std::string& pADCInputN, uint8_t pGain, uint8_t pSamples = 1);
    float   _CdacCodeToCurrent(Ph2_HwDescription::lpGBT* pChip, const std::string& pChannel, uint8_t pCode);
    float   _CdacCodeToRout(Ph2_HwDescription::lpGBT* pChip, const std::string& pChannel, uint8_t pCode);
    uint8_t _CdacGetOptimumCodeForCurrent(Ph2_HwDescription::lpGBT* pChip, const std::string& pChannel, float pCurrentA);
    void    CdacSetCurrent(Ph2_HwDescription::lpGBT* pChip, const std::string& pChannel, float pCurrentA);
    float   MeasureResistance(Ph2_HwDescription::lpGBT* pChip, const std::string& pChannel, bool pImprovePrecision = true);
    float   MeasureResistance(Ph2_HwDescription::lpGBT* pChip, const std::string& pChannel, float pExpectedROhm, bool pImprovePrecision = true);
    void    VdacSetVout(Ph2_HwDescription::lpGBT* pChip, float pVoltageV, bool pEnable = true);
    float   MeasureTemperature(Ph2_HwDescription::lpGBT* pChip, uint8_t pSamples = 1, bool pResetTempSensor = true);
    float   MeasurePowerSupplyVoltage(Ph2_HwDescription::lpGBT* pChip, const std::string& pPowerSupply, uint8_t pSamples = 1, bool pDisableMonitorAfterMeasurement = true);
    float   ReadChipMonitor(const Ph2_HwDescription::OpticalGroup* pOpticalGroup, const std::string& registerName, bool silentRunning = false);
    float   GetLastNTCResistance(Ph2_HwDescription::lpGBT* pChip, const std::string& theNTCtype);
    void    SetLastNTCResistance(Ph2_HwDescription::lpGBT* pChip, const std::string& theNTCtype, float res);

    uint8_t                        fChosenPhase;
    std::map<std::string, uint8_t> GetPhaseTapMap();

  protected:
    // ##############
    // # LpGBT maps #
    // ##############
    std::map<uint16_t, uint8_t> fClockFrequencyMap = {{0, 0}, {40, 1}, {80, 2}, {160, 3}, {320, 4}, {640, 5}, {1280, 7}};
    std::map<uint16_t, uint8_t> fTxDataRateMap     = {{0, 0}, {80, 1}, {160, 2}, {320, 3}};
    std::map<uint16_t, uint8_t> f5GRxDataRateMap   = {{0, 0}, {160, 1}, {320, 2}, {640, 3}};
    std::map<uint16_t, uint8_t> f10GRxDataRateMap  = {{0, 0}, {320, 1}, {640, 2}, {1280, 3}};

    std::map<uint8_t, uint8_t> fGroup2BERTsourceCourse      = {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}};
    std::map<uint8_t, uint8_t> fChannelSpeed2BERTsourceFine = {{0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}, {6, 5}, {8, 6}}; // channel + 4 * (2 - frontendSpeed)

    // Power Up State Machine maps for both LpGBT-v0 and LpGBT-v1
    // Map read as : map[lpgbt_version][state_id] = state_description
    std::map<uint8_t, std::map<uint8_t, std::string>> fPUSMStatusMap = {{0,
                                                                         {{0, "ARESET"},
                                                                          {1, "RESET"},
                                                                          {2, "WAIT_VDD_STABLE"},
                                                                          {3, "WAIT_VDD_HIGHER_THAN_0V90"},
                                                                          {4, "FUSE_SAMPLING"},
                                                                          {5, "UPDATE_FROM_FUSES"},
                                                                          {6, "WAIT_FOR_PLL_CONFIG"},
                                                                          {7, "WAIT_POWER_GOOD"},
                                                                          {8, "RESETOUT"},
                                                                          {9, "I2C_TRANS"},
                                                                          {10, "RESET_PLL"},
                                                                          {11, "WAIT_PLL_LOCK"},
                                                                          {12, "INIT_SCRAM"},
                                                                          {13, "PAUSE_FOR_DLL_CONFIG"},
                                                                          {14, "RESET_DLLS"},
                                                                          {15, "WAIT_DLL_LOCK"},
                                                                          {16, "RESET_LOGIC_USING_DLL"},
                                                                          {17, "WAIT_CHNS_LOCKED"},
                                                                          {18, "READY"}}},
                                                                        {1,
                                                                         {{0, "ARESET"},
                                                                          {1, "RESET"},
                                                                          {2, "WAIT_VDD_STABLE"},
                                                                          {3, "WAIT_VDD_HIGHER_THAN_0V90"},
                                                                          {4, "STATE_COPY_FUSES"},
                                                                          {5, "STATE_CALCULATE_CHECKSUM"},
                                                                          {6, "COPY_ROM"},
                                                                          {7, "PAUSE_FOR_PLL_CONFIG"},
                                                                          {8, "WAIT_POWER_GOOD"},
                                                                          {9, "RESET_PLL"},
                                                                          {10, "WAIT_PLL_LOCK"},
                                                                          {11, "INIT_SCRAM"},
                                                                          {12, "RESETOUT"},
                                                                          {13, "I2C_TRANS"},
                                                                          {14, "PAUSE_FOR_DLL_CONFIG"},
                                                                          {15, "RESET_DLLS"},
                                                                          {16, "WAIT_DLL_LOCK"},
                                                                          {17, "RESET_LOGIC_USING_DLL"},
                                                                          {18, "WAIT_CHNS_LOCKED"},
                                                                          {19, "READY"}}}};

    std::map<std::string, uint8_t> revertedPUSMStatusMap;
    std::map<uint8_t, size_t>      fBERTMeasTimeMap = {{0, 1UL << 5},
                                                       {1, 1UL << 7},
                                                       {2, 1UL << 9},
                                                       {3, 1UL << 11},
                                                       {4, 1UL << 13},
                                                       {5, 1UL << 15},
                                                       {6, 1UL << 17},
                                                       {7, 1UL << 19},
                                                       {8, 1UL << 21},
                                                       {9, 1UL << 23},
                                                       {10, 1UL << 25},
                                                       {11, 1UL << 27},
                                                       {12, 1UL << 29},
                                                       {13, 1UL << 31},
                                                       {14, 1UL << 33},
                                                       {15, 1UL << 35}};

    std::map<uint8_t, std::string> fEOMStatusMap = {{0, "smIdle"}, {1, "smResetCounters"}, {2, "smCount"}, {3, "smEndOfCount"}};

    std::map<uint8_t, std::string> fI2CStatusMap = {{4, "TransactionSucess"}, {8, "SDAPulledLow"}, {32, "InvalidCommand"}, {64, "NotACK"}};
    std::map<std::string, uint8_t> fPhaseTapMap  = {
        {"Group0Channel0", 0},
        {"Group0Channel2", 0},
        {"Group1Channel0", 0},
        {"Group1Channel2", 0},
        {"Group2Channel0", 0},
        {"Group2Channel2", 0},
        {"Group3Channel0", 0},
        {"Group3Channel2", 0},
        {"Group4Channel0", 0},
        {"Group4Channel2", 0},
        {"Group5Channel0", 0},
        {"Group5Channel2", 0},
        {"Group6Channel0", 0},
        {"Group6Channel2", 0},
    };
};

} // namespace Ph2_HwInterface

#endif
