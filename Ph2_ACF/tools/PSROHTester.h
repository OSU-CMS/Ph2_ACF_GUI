/*!

        \file                   PSROHTester.h
        \brief                  Class for PS-ROH hybrids test using a testcard
        \author                 Younes OTARID
        \version                1.0
        \date                   17/12/2020
        Support :               mail to : younes.otarid@cern.ch

 */
#ifndef PSROHTester_h__
#define PSROHTester_h__

#if defined(__TCUSB__) && defined(__USE_ROOT__)
#include "OTHybridTester.h"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "boost/format.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

class PSROHTester : public OTHybridTester
{
  public:
    PSROHTester();
    ~PSROHTester();

    void Initialise();
    void Start(const StartInfo& theStartInfo);
    void Stop();
    void Pause();
    void Resume();

    void PSROHInputsDebug();

    void CheckFastCommands(const std::string& sFastCommandPattern, const std::string& userFilename);
    void CheckHybridInputs(std::vector<std::string> pInputs, std::vector<uint32_t>& pCounters);
    void CheckHybridOutputs(std::vector<std::string> pOutputs, std::vector<uint32_t>& pCounters);
    void ClearBRAM(const std::string& sBRAMToReset = "ref");
    void ReadCheckAddrBRAM(int iCheckBRAMAddr = 0);
    void ReadRefAddrBRAM(int iRefBRAMAddr = 0);

    void UserFCMDTranslate(const std::string&);
    void CheckFastCommandsBRAM(const std::string& sFastCommandLine);
    void WritePatternToBRAM(const std::string& sFileName);
    bool FastCommandScope();
    bool TestResetLines(uint8_t pLevel);

    void MeasureInputIV(const std::string& cTestStep);

  private:
    bool FastCommandScope(Ph2_HwDescription::BeBoard* pBoard);
    void CheckFastCommands(Ph2_HwDescription::BeBoard* pBoard, const std::string& sFastCommandPattern, const std::string& userFilename);
    void CheckFastCommandsBRAM(Ph2_HwDescription::BeBoard* pBoard, const std::string& sFastCommandLine);
    void WritePatternToBRAM(Ph2_HwDescription::BeBoard* pBoard, const std::string&);
    void ClearRefBRAM(Ph2_HwDescription::BeBoard* pBoard);
    void ClearBRAM(Ph2_HwDescription::BeBoard* pBoard, const std::string& sBRAMToReset = "ref");
    void ReadCheckAddrBRAM(Ph2_HwDescription::BeBoard* pBoard, int iCheckBRAMAddr = 0);
    void ReadRefAddrBRAM(Ph2_HwDescription::BeBoard* pBoard, int iRefBRAMAddr = 0);

    void CheckHybridInputs(Ph2_HwDescription::BeBoard* pBoard, std::vector<std::string> pInputs, std::vector<uint32_t>& pCounters);
    void CheckHybridOutputs(Ph2_HwDescription::BeBoard* pBoard, std::vector<std::string> pOutputs, std::vector<uint32_t>& pCounters);
    // void CheckFastCommands(Ph2_HwDescription::BeBoard* pBoard, const std::string & pFastCommand ,  uint8_t pDuartion=1);

    std::map<std::string, uint8_t> fInputDebugMap = {{"l_fcmd_cic", 0},
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

    std::map<std::string, uint8_t> fOutputDebugMap =
        {{"cic_in_6", 0}, {"cic_in_5", 1}, {"cic_in_4", 2}, {"cic_in_3", 3}, {"cic_in_2", 4}, {"cic_in_1", 5}, {"cic_in_0", 6}, {"r_i2c_sda_i", 7}, {"l_i2c_sda_i", 8}, {"na", 9}};

    std::map<std::string, TC_PSROH::measurement> fInputIVMap = {{"_3V3", TC_PSROH::measurement::_3V3},
                                                                {"_2V55", TC_PSROH::measurement::_2V55},
                                                                {"_1V25", TC_PSROH::measurement::_1V25},
                                                                {"_1V25_REF", TC_PSROH::measurement::_1V25_REF},
                                                                {"_625mV_REF", TC_PSROH::measurement::_625mV_REF},
                                                                {"ISEN_2V55", TC_PSROH::measurement::ISEN_2V55},
                                                                {"ISEN_3V3", TC_PSROH::measurement::ISEN_3V3},
                                                                {"ISEN_1V25", TC_PSROH::measurement::ISEN_1V25}};

    static const int NBRAMADDR = 1024;
};

#endif
#endif
