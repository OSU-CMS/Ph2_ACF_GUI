/*!

        \file                   SEHTester.h
        \brief                  Class for 2S-SEH hybrids test using a testcard
        \author                 Alexander Pauls
        \version                1.0
        \date                   11/01/2020
        Support :               mail to : alexander.pauls@rwth-aachen.de

 */
#ifndef SEHTester_h__
#define SEHTester_h__
#if defined(__TCUSB__) && defined(__USE_ROOT__)

#include "OTHybridTester.h"
//
#include "USB_a.h"
#include "USB_libusb.h"

#include "TAxis.h"
#include "TF1.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TH2.h"
#include "THStack.h"
#include "TLegend.h"
#include "TMultiGraph.h"
#include "TObject.h"
#include "TRandom3.h"
#include "TString.h"
#include "TStyle.h"
#include "TTree.h"

#include <fstream>
#include <iostream>
#include <map>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/time.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

class SEHTester : public OTHybridTester
{
  public:
    SEHTester();
    ~SEHTester();

    void Initialise();
    void Start(const StartInfo& theStartInfo);
    void Stop();
    void Pause();
    void Resume();
    void RunHybridETest();
    void SEHInputsDebug();
    void TurnOn(uint32_t pRightLoadValue = 0, uint32_t pLeftLoadValue = 0, bool setLoad = false, bool measureTemperature = false);
    bool CheckShort(std::string powerSupplyId, std::string channelId);

    void TurnOff();
    void SetLoad(uint32_t pRightLoadValue = 0, uint32_t pLeftLoadValue = 0);
    void RampPowerSupply(std::string powerSupplyId, std::string channelId, const std::vector<float>& cVoltages);
    void CheckFastCommands(const std::string& sFastCommandPattern, const std::string& userFilename);
    void CheckHybridInputs(std::vector<std::string> pInputs, std::vector<uint32_t>& pCounters);
    void CheckHybridOutputs(std::vector<std::string> pOutputs, std::vector<uint32_t>& pCounters);
    void ClearBRAM(const std::string& sBRAMToReset = "ref");
    void ReadCheckAddrBRAM(int iCheckBRAMAddr = 0);
    void ReadRefAddrBRAM(int iRefBRAMAddr = 0);

    void UserFCMDTranslate(const std::string&);
    void CheckFastCommandsBRAM(const std::string& sFastCommandLine);
    void WritePatternToBRAM(const std::string& sFileName);
    void FastCommandScope();
    bool FastCommandChecker(uint8_t pPattern);
    void TestCardVoltages();
    void TestEfficiency(uint32_t pMinLoadValue, uint32_t pMaxLoadValue, uint32_t pStep);
    void TestLeakageCurrent(uint32_t pHvDacValue, double measurementTime);
    void TestBiasVoltage();
    void ExternalTestLeakageCurrent(uint16_t pHvSet, double measurementTime, std::string powerSupplyId, std::string channelId);
    void ExternalTestBiasVoltage(std::string powerSupplyId, std::string channelId);
    int  exampleFit();
    void readTestParameters(std::string file);
    void DCDCOutputEvaluation();
    void SetupExternalTestLeakageCurrent(uint16_t pHvSet, std::string powerSupplyId, std::string channelId);
    void EndExternalTestLeakageCurrent(std::string powerSupplyId, std::string channelId);

    // bool TestFixedADCs();
    // bool ToyTestFixedADCs();

  private:
    void        FastCommandScope(Ph2_HwDescription::BeBoard* pBoard);
    bool        FastCommandChecker(Ph2_HwDescription::BeBoard* pBoard, uint8_t pPattern);
    void        CheckFastCommands(Ph2_HwDescription::BeBoard* pBoard, const std::string& sFastCommandPattern, const std::string& userFilename);
    void        CheckFastCommandsBRAM(Ph2_HwDescription::BeBoard* pBoard, const std::string& sFastCommandLine);
    void        WritePatternToBRAM(Ph2_HwDescription::BeBoard* pBoard, const std::string&);
    void        ClearRefBRAM(Ph2_HwDescription::BeBoard* pBoard);
    void        ClearBRAM(Ph2_HwDescription::BeBoard* pBoard, const std::string& sBRAMToReset = "ref");
    void        ReadCheckAddrBRAM(Ph2_HwDescription::BeBoard* pBoard, int iCheckBRAMAddr = 0);
    void        ReadRefAddrBRAM(Ph2_HwDescription::BeBoard* pBoard, int iRefBRAMAddr = 0);
    float       PowerSupplyGetMeasurement(std::string name);
    float       getMeasurement(std::string name);
    std::string getVariableValue(std::string variable, std::string buffer);
    void        CheckHybridInputs(Ph2_HwDescription::BeBoard* pBoard, std::vector<std::string> pInputs, std::vector<uint32_t>& pCounters);
    void        CheckHybridOutputs(Ph2_HwDescription::BeBoard* pBoard, std::vector<std::string> pOutputs, std::vector<uint32_t>& pCounters);
    // void CheckFastCommands(Ph2_HwDescription::BeBoard* pBoard, const std::string & pFastCommand ,  uint8_t pDuartion=1);

    std::map<std::string, uint8_t>     fInputDebugMap = {{"l_fcmd_cic", 0},
                                                         {"r_fcmd_cic", 1},
                                                         {"l_fcmd_ssa", 2},
                                                         {"r_fcmd_ssa", 3},
                                                         {"l_clk_320", 4},
                                                         {"r_clk_320", 5},
                                                         {"l_clk_640", 6},
                                                         {"r_clk_640", 7},
                                                         {"l_i2c_scl", 8},
                                                         {"r_i2c_scl", 9},
                                                         {"l_i2c_sda_o", 10},
                                                         {"r_i2c_sda_o", 11},
                                                         {"cpg", 12},
                                                         {"bpg", 13},
                                                         {"na", 14}};
    std::map<std::string, std::string> fADCInputMap =
        {{"AMUX_L", "ADC0"}, {"VMON_P1V25_L", "ADC1"}, {"VMIN", "ADC2"}, {"AMUX_R", "ADC3"}, {"TEMPP", "ADC4"}, {"VTRX+_RSSI_ADC", "ADC5"}, {"PTAT_BPOL2V5", "ADC6"}, {"PTAT_BPOL12V", "ADC7"}};

    std::map<std::string, uint8_t> fOutputDebugMap =
        {{"cic_in_6", 0}, {"cic_in_5", 1}, {"cic_in_4", 2}, {"cic_in_3", 3}, {"cic_in_2", 4}, {"cic_in_1", 5}, {"cic_in_0", 6}, {"r_i2c_sda_i", 7}, {"l_i2c_sda_i", 8}, {"na", 9}};

    static const int NBRAMADDR = 1024;

    std::map<std::string, TC_2SSEH::supplyMeasurement> f2SSEHSupplyMeasurements = {{"U_P5V", TC_2SSEH::supplyMeasurement::U_P5V},
                                                                                   {"I_P5V", TC_2SSEH::supplyMeasurement::I_P5V},
                                                                                   {"U_P3V3", TC_2SSEH::supplyMeasurement::U_P3V3},
                                                                                   {"I_P3V3", TC_2SSEH::supplyMeasurement::I_P3V3},
                                                                                   {"U_P2V5", TC_2SSEH::supplyMeasurement::U_P2V5},
                                                                                   {"I_P2V5", TC_2SSEH::supplyMeasurement::I_P2V5},
                                                                                   {"U_P1V25", TC_2SSEH::supplyMeasurement::U_P1V25},
                                                                                   {"I_P1V25", TC_2SSEH::supplyMeasurement::I_P1V25},
                                                                                   {"U_SEH", TC_2SSEH::supplyMeasurement::U_SEH},
                                                                                   {"I_SEH", TC_2SSEH::supplyMeasurement::I_SEH}};
    std::map<std::string, double> fHybridNominalValues = {{"U_P5V", 5.}, {"I_P5V", 0.045}, {"U_P3V3", 3.29}, {"I_P3V3", 1.27}, {"U_P2V5", 2.5}, {"I_P2V5", 0.0}, {"U_P1V25", 1.24}, {"I_P1V25", 0.000}};

    std::map<std::string, TC_2SSEH::loadMeasurement> f2SSEHLoadMeasurements = {{"U_P1V2_R", TC_2SSEH::loadMeasurement::U_P1V2_R},
                                                                               {"I_P1V2_R", TC_2SSEH::loadMeasurement::I_P1V2_R},
                                                                               {"U_P1V2_L", TC_2SSEH::loadMeasurement::U_P1V2_L},
                                                                               {"I_P1V2_L", TC_2SSEH::loadMeasurement::I_P1V2_L},
                                                                               {"P2V5_VTRx_MON", TC_2SSEH::loadMeasurement::P2V5_VTRx_MON}};

    std::map<std::string, TC_2SSEH::temperatureMeasurement> f2SSEHTemperatureMeasurements = {{"Temp1", TC_2SSEH::temperatureMeasurement::Temp1},
                                                                                             {"Temp2", TC_2SSEH::temperatureMeasurement::Temp2},
                                                                                             {"Temp3", TC_2SSEH::temperatureMeasurement::Temp3},
                                                                                             {"Temp_SEH", TC_2SSEH::temperatureMeasurement::Temp_SEH}};

    std::map<std::string, TC_2SSEH::resetMeasurement> f2SSEHResetLines   = {{"RST_CBC_R", TC_2SSEH::resetMeasurement::RST_CBC_R},
                                                                            {"RST_CIC_R", TC_2SSEH::resetMeasurement::RST_CIC_R},
                                                                            {"RST_CBC_L", TC_2SSEH::resetMeasurement::RST_CBC_L},
                                                                            {"RST_CIC_L", TC_2SSEH::resetMeasurement::RST_CIC_L}};
    std::map<std::string, float>                      fDefaultParameters = {{"Spannung", 2},
                                                                            {"Strom", 0.5},
                                                                            {"HV", 1},
                                                                            {"VMON_P1V25_L_Nominal", 0.806},
                                                                            {"VMIN_Nominal", 0.49},
                                                                            {"TEMPP_Nominal", 0.6},
                                                                            {"VTRX+_RSSI_ADC_Nominal", 0.6},
                                                                            {"PTAT_BPOL2V5_Nominal", 0.6},
                                                                            {"PTAT_BPOL12V_Nominal", 0.6}};
};
#endif
#endif
