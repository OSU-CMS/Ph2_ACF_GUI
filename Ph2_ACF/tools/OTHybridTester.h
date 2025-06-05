/*!

        \file                   OTHybridTester.h
        \brief                  Base class for all hybrids test using a testcard
        \author                 Younes OTARID
        \version                1.0
        \date                   17/12/2020
        Support :               mail to : younes.otarid@cern.ch

 */

#ifndef OTHybridTester_h__
#define OTHybridTester_h__

#if defined(__TCUSB__) && defined(__USE_ROOT__)

#include "HWInterface/D19cDebugFWInterface.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/D19cLinkInterface.h"
#include "HWInterface/D19cOpticalInterface.h"
#include "HWInterface/D19cTriggerInterface.h"
#include "HWInterface/DPInterface.h"
#include "HWInterface/L1ReadoutInterface.h"
#include "Utils/linearFitter.h"
#include "tools/LinkAlignmentOT.h"
#include "tools/Tool.h"

#include "TAxis.h"
#include "TF1.h"
#include "TGraph.h"
#include "TH2.h"
#include "TH2I.h"
#include "TLegend.h"
#include "TMultiGraph.h"
#include "TObject.h"
#include "TRandom3.h"
#include "TString.h"
#include "TStyle.h"
#include "TTree.h"

#include <cstring>
#include <map>
#include <string>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

class OTHybridTester : public Tool
{
  public:
    OTHybridTester();
    ~OTHybridTester();

    // ###################################
    // # LpGBT related functions #
    // ###################################
    // Test lpGBT Up Link with internal pattern
    void LpGBTInjectULInternalPattern(uint32_t pPattern);
    // Test lpGBT Up Link with external pattern
    void LpGBTInjectULExternalPattern(bool pStart, uint8_t pPattern);
    // Check Up Link data in backend fc7
    bool LpGBTCheckULPattern(bool pIsExternal = false, uint8_t pPattern = 202);
    // Test lpGBT Down Link with internal pattern (Hybrid Fast Command)
    void LpGBTInjectDLInternalPattern(uint8_t pPattern);
    // Test lpGBT I2C Masters
    bool LpGBTTestI2CMaster(const std::vector<uint8_t>& pMasters, int pNTries = 1000);
    // Test lpGBT ADC
    void LpGBTTestADC(const std::vector<std::string>& pADCs, uint32_t pMinDAC, uint32_t pMaxDAC, uint32_t pStep, bool pCalibrate = false);
    // Set GPIO level
    void     LpGBTSetGPIOLevel(const std::vector<uint8_t>& pGPIOs, uint8_t Level);
    bool     LpGBTTestResetLines();
    bool     LpGBTTestFixedADCs();
    bool     LpGBTTestGPILines();
    bool     LpGBTTestVTRx();
    bool     LpGBTGetLinkLock();
    bool     LpGBTCheckClocks();
    bool     LpGBTFastCommandChecker(uint8_t pPattern);
    uint16_t calibrateADC();
    void     calibrateCurrentDAC();
    // Run Eye Openin Monitor
    void LpGBTRunEyeOpeningMonitor(uint8_t pEndOfCountSelect, uint8_t pEQAttenuation = 3);
    // Run Bit Error Rate Test
    void LpGBTRunBitErrorRateTest(uint8_t pCoarseSource, uint8_t pFineSource, uint8_t pMeasTime, uint32_t pPattern = 0x00000000);

    // Phase Alignment
    void                     BackEndAlignment(std::vector<std::string> pLines);
    std::pair<bool, uint8_t> PhaseTuneLineEleFC7(uint8_t pHybrid, uint8_t pLineId);
    void                     InitialiseTestCard(bool cIsSEH);
    void                     ReadChipIds();
    void                     CheckConfiguredHw();

  private:
    float       getMeasurement(std::string name);
    std::string getVariableValue(std::string variable, std::string buffer);
    bool        fIsSEH                = false;
    float       fNominalOutputbpol2v5 = 1.200; // [V] the GPIO high voltage depends on the bpol2v5 voltage
    float       fGradingThreshold     = 0.1;   // [V]
  protected:
    TC_2SSEH* fTC_2SSEH = nullptr;
    TC_PSROH* fTC_PSROH = nullptr;

    std::map<std::string, uint8_t> f2SSEHGPILines = {
        {"PG2V5", 13},
        {"PG1V25", 14},
    };
    std::map<std::string, uint8_t> fPSROHGPILines = {
        {"PWRGOOD", 13},
        {"VTRx.RSTN", 11},
    };

    std::map<std::string, std::string> f2SSEHFCMDLines = {
        {"FCMD_CIC_R", "fc7_daq_stat.physical_interface_block.fcmd_debug_ssa_r"},
        {"FCMD_CIC_L", "fc7_daq_stat.physical_interface_block.fcmd_debug_cic_l"},
    };
    std::map<std::string, std::string> f2SSEHClockMap = {
        {"320_r_Clk_Test", "fc7_daq_stat.physical_interface_block.clk_test.fe_for_ps_roh_clk_320_r"},
        {"320_l_Clk_Test", "fc7_daq_stat.physical_interface_block.clk_test.fe_for_ps_roh_clk_320_l"},
    };
    std::map<std::string, std::string> fPSROHFCMDLines = {
        {"FCMD_CIC_R", "fc7_daq_stat.physical_interface_block.fcmd_debug_cic_r"},
        {"FCMD_CIC_L", "fc7_daq_stat.physical_interface_block.fcmd_debug_cic_l"},
        {"FCMD_SSA_R", "fc7_daq_stat.physical_interface_block.fcmd_debug_ssa_r"},
        {"FCMD_SSA_L", "fc7_daq_stat.physical_interface_block.fcmd_debug_ssa_l"},
    };
    std::map<std::string, std::string> fPSROHClockMap = {
        {"320_r_Clk_Test", "fc7_daq_stat.physical_interface_block.clk_test.fe_for_ps_roh_clk_320_r"},
        {"320_l_Clk_Test", "fc7_daq_stat.physical_interface_block.clk_test.fe_for_ps_roh_clk_320_l"},
        {"640_r_Clk_Test", "fc7_daq_stat.physical_interface_block.clk_test.fe_for_ps_roh_clk_640_r"},
        {"640_l_Clk_Test", "fc7_daq_stat.physical_interface_block.clk_test.fe_for_ps_roh_clk_640_l"},
    };
    std::map<std::string, TC_2SSEH::resetMeasurement> f2SSEHResetLines = {{"RST_CBC_R", TC_2SSEH::resetMeasurement::RST_CBC_R},
                                                                          {"RST_CIC_R", TC_2SSEH::resetMeasurement::RST_CIC_R},
                                                                          {"RST_CBC_L", TC_2SSEH::resetMeasurement::RST_CBC_L},
                                                                          {"RST_CIC_L", TC_2SSEH::resetMeasurement::RST_CIC_L}};

    std::map<std::string, TC_PSROH::measurement> fResetLines = {{"L_MPA", TC_PSROH::measurement::L_MPA_RST},
                                                                {"L_CIC", TC_PSROH::measurement::L_CIC_RST},
                                                                {"L_SSA", TC_PSROH::measurement::L_SSA_RST},
                                                                {"R_MPA", TC_PSROH::measurement::R_MPA_RST},
                                                                {"R_CIC", TC_PSROH::measurement::R_CIC_RST},
                                                                {"R_SSA", TC_PSROH::measurement::R_SSA_RST}};
    std::map<std::string, float>                 f2SSEHDefaultParameters =
        {{"VMON_P1V25_L_Nominal", 0.806}, {"VMIN_Nominal", 0.49}, {"TEMPP_Nominal", 0.6}, {"VTRX+_RSSI_ADC_Nominal", 0.6}, {"PTAT_BPOL2V5_Nominal", 0.6}, {"PTAT_BPOL12V_Nominal", 0.6}};
    std::map<std::string, std::string> f2SSEHADCInputMap =
        {{"AMUX_L", "ADC0"}, {"VMON_P1V25_L", "ADC1"}, {"VMIN", "ADC2"}, {"AMUX_R", "ADC3"}, {"TEMPP", "ADC4"}, {"VTRX+_RSSI_ADC", "ADC5"}, {"PTAT_BPOL2V5", "ADC6"}, {"PTAT_BPOL12V", "ADC7"}};
    std::map<std::string, std::string> fPSROHADCInputMap            = {{"L_AMUX_OUT", "ADC0"},
                                                                       {"1V_MONITOR", "ADC1"},
                                                                       {"12V_MONITOR_VD", "ADC2"},
                                                                       {"R_AMUX_OUT", "ADC3"},
                                                                       {"TEMP", "ADC4"},
                                                                       {"VTRX+.RSSI_ADC", "ADC5"},
                                                                       {"1V25_MONITOR", "ADC6"},
                                                                       {"2V55_MONITOR", "ADC7"}};
    std::map<std::string, float>       fPSROHDefaultParameters      = {{"12V_MONITOR_VD_Nominal", 0.21},
                                                                       {"TEMP_Nominal", 0.6},
                                                                       {"VTRX+.RSSI_ADC_Nominal", 0.6},
                                                                       {"1V25_MONITOR_Nominal", 0.806},
                                                                       {"2V55_MONITOR_Nominal", 0.808}};
    std::map<uint8_t, uint8_t>         fVTRxplusDefaultRegisters    = {{0x00, 0x0f}, {0x01, 0x01}, {0x04, 0x0f}, {0x05, 0x2f}, {0x06, 0x26}, {0x07, 0x00}};
    std::map<uint8_t, uint8_t>         fVTRxplusDefaultRegistersV13 = {{0x00, 0x01}, {0x02, 0x01}, {0x03, 0x30}, {0x04, 0xa0}, {0x05, 0x00}, {0x11, 0x07}};
    std::map<uint8_t, std::string>     fI2CStatusMap                = {{4, "TransactionSucess"}, {8, "SDAPulledLow"}, {32, "InvalidCommand"}, {64, "NotACK"}};

    std::map<std::string, uint8_t> fBackendAlignmentLineMap = {{"SSA_FCMD_L", 0}, {"SSA_FCMD_R", 1}, {"CIC_FCMD_L", 2}, {"CIC_FCMD_R", 3}, {"2S_R", 1}, {"2S_L", 2}};
};
#endif
#endif
