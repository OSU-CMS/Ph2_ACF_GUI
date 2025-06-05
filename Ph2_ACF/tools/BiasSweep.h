/*!

        \file                   BiasSweep.h
        \brief                  Class to sweep one of the CBC3 biases and perform an analog measurement via the
   AnalogMux and Ke2110 DMM \author                 Georg AUZINGER \version                1.0 \date 31/10/16 Support :
   mail to : georg.auzinger@SPAMNOT.cern.ch

 */

#ifndef __BIASSWEEP_H__
#define __BIASSWEEP_H__

#ifdef __USE_ROOT__

#include "HWDescription/ReadoutChip.h"
#include "TAxis.h"
#include "TGraph.h"
#include "TObject.h"
#include "TString.h"
#include "TTree.h"
#include "Utils/CommonVisitors.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"
#include "tools/Tool.h"
#include <atomic>
#include <map>
#include <mutex>
#include <vector>

using namespace Ph2_System;

#ifdef __USBINST__
#include "ArdNanoController.h"
#include "HMP4040Client.h"
#include "Ke2110Controller.h"
using namespace Ph2_UsbInst;
#endif

#ifdef __MAKECINT__
#pragma link C++ class vector < float> + ;
#pragma link C++ class vector < uint16_t> + ;
#endif

class AmuxSetting
{
  public:
    std::string fRegName;
    uint8_t     fAmuxCode;
    uint8_t     fBitMask;
    uint8_t     fBitShift;

    AmuxSetting(std::string pRegName, uint8_t pAmuxCode, uint8_t pBitMask, uint8_t pBitShift) : fRegName(pRegName), fAmuxCode(pAmuxCode), fBitMask(pBitMask), fBitShift(pBitShift) {}
};

class BiasSweepData : public TObject
{
  public:
    uint16_t              fHybridId;
    uint16_t              fCbcId;
    std::string           fBias;
    long int              fTimestamp;
    char                  fUnit[2];
    uint16_t              fInitialXValue;
    float                 fInitialYValue;
    std::vector<uint16_t> fXValues;
    std::vector<float>    fYValues;

    BiasSweepData() : fHybridId(0), fCbcId(0), fBias(""), fTimestamp(0) {}
    ~BiasSweepData() {}

    // ClassDef (BiasSweepData, 1); //The class title
};

class BiasSweep : public Tool
{
  public:
#ifdef __USBINST__
    BiasSweep(HMP4040Client* pClient = nullptr, Ke2110Controller* pController = nullptr);
#endif
    BiasSweep();
    ~BiasSweep();
    void Initialize();
    // *******
    void SweepBias(std::string pBias, Ph2_HwDescription::ReadoutChip* pCbc);
    void MeasureMinPower(Ph2_HwDescription::BeBoard* pBoard, Ph2_HwDescription::ReadoutChip* pCbc);

  private:
    // std::thread fThread;
    // std::mutex fHWMutex;
    std::atomic<bool> fDAQrunning;
    void              InitializeAmuxMap();
    uint8_t           configureAmux(std::map<std::string, AmuxSetting>::iterator pAmuxValue, Ph2_HwDescription::Chip* pCbc, double pSettlingTime_s = 0);
    void              resetAmux(uint8_t pAmuxValue, Ph2_HwDescription::Chip* pCbc, double pSettlingTime_s = 0);
    void              sweep8Bit(std::map<std::string, AmuxSetting>::iterator pAmuxValue, TGraph* pGraph, Ph2_HwDescription::Chip* pCbc, bool pCurrent);
    void              measureSingle(std::map<std::string, AmuxSetting>::iterator pAmuxMap, Ph2_HwDescription::Chip* pCbc);
    void              sweepVCth(TGraph* pGraph, Ph2_HwDescription::Chip* pCbc);
    void              cleanup();
    void              DAQloop();
    void              StartDAQ();
    void              StopDAQ();

    TCanvas* fSweepCanvas;

    std::map<std::string, AmuxSetting> fAmuxSettings;
    // for the TTree
    BiasSweepData* fData;

    // settings
    int fSweepTimeout, fKePort, fHMPPort, fStepSize;

#ifdef __USBINST__
    Ke2110Controller*  fKeController;
    HMP4040Client*     fHMPClient;
    ArdNanoController* fArdNanoController;
#endif

    void writeObjects();
};

#endif
#endif
