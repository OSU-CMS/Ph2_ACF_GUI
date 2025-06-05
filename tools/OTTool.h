/*!
 *
 * \file LinkAlignment.h
 * \brief Link alignment class, automated alignment procedure for CIC-lpGBT-BE
 * connected to FEs
 * \author Sarah SEIF EL NASR-STOREY
 * \date 28 / 06 / 19
 *
 * \Support : sarah.storey@cern.ch
 *
 */

#ifndef OTTool_h__
#define OTTool_h__

#include "tools/Tool.h"
#include <signal.h>
using namespace Ph2_HwDescription;

struct PrintConfig
{
    uint8_t  fVerbose    = 0;
    uint32_t fPrintEvery = 1;
};

//***************************************************
struct boardData
{
    uint8_t  hit;
    uint8_t  localX; // 0 , Bottom - 1 , Tp[ ]
    uint16_t localY; // 0 - closest to SEH
    uint32_t cEventId;
    uint32_t L1Id;
    uint8_t  cSensorId;
    uint8_t  opticalGroupId;
    uint8_t  boardId;
};

class OTTool : public Tool
{
  public:
    OTTool();
    ~OTTool();

    // expect thise to be the same for
    void        Prepare();
    void        Reset();
    void        ConfigurePrintout(PrintConfig pCnfg);
    void        SaveHitDataTree() { fSaveTree = true; }
    void        SetChipRegstoPerserve(FrontEndType pType, std::vector<std::string> pListOfRegs);
    void        SetBrdRegstoPerserve(std::vector<std::string> pListOfRegs);
    void        SetReadoutPause(uint32_t pReadoutPause) { fReadoutPause = pReadoutPause; }
    void        ReadDataFromFile(std::string pRawFileName);
    void        ContinuousReadout();
    void        SetName(std::string pName) { fMyName = pName; };
    void        TriggerMonitor(uint32_t pDelta_s = 1);
    static void StopTriggerMonitor(int signum)
    {
        LOG(INFO) << BOLDRED << "Caught Ctrl+C from command line [signum == " << signum << RESET;
        throw Exception("Ctrl+C caught from command line..");
    };
    void InjectPattern(Ph2_HwDescription::BeBoard* pBoard, std::vector<Ph2_HwInterface::Injection> pInjections, int pChipId = -1);
    void SetInjectionType(uint8_t pInjType) { fInjectionType = pInjType; }; // 0-digital, 1 - analogue
    void SetReadoutMode(uint8_t pType) { fReadoutMode = pType; };

  protected:
    // success or fail
    bool fSuccess{false};
    // chips found on the detector
    uint8_t fWithCIC{0};
    uint8_t fWithLpGBT{0};
    uint8_t fWithMPA{0};
    uint8_t fWithSSA{0};
    uint8_t fWithCBC{0};
    // readout related items
    uint32_t fNevents{100};
    uint32_t fEventCountInt{0};
    uint32_t fReadoutPause{1};
    uint32_t fEventCounter{0};
    // waits
    uint32_t fThreadWait{1}; // in us
    // stop trigger monitor
    uint8_t fStopTriggerMonitor{0};
    // type of injection
    uint8_t fInjectionType{0}; // 0-digital,1-analogue
    // type of readout
    uint8_t fReadoutMode{0}; // 0  - from board; 1 -- from .raw

    void UpdateFromRegMap(Ph2_HwDescription::BeBoard* pBoard);
    void PrintData(Ph2_HwDescription::BeBoard* pBoard);
    void EventPrintout(Ph2_HwDescription::BeBoard* pBoard, Ph2_HwInterface::Event* pEvent);
    void ContinuousReadout(Ph2_HwDescription::BeBoard* pBoard);
    void WaitForTriggers(Ph2_HwDescription::BeBoard* pBoard);
    void ContinuousReadoutTh(uint8_t cBrdId);
    void CheckFinishedTh(uint8_t cBrdId);
    void StartReadoutTh(uint8_t cBrdId);
    void CatchStop();

  private:
    // Containers
    DetectorDataContainer fBoardRegContainer;
    // configuration of print-out
    PrintConfig fPrintConfig;
    // name of the tool
    std::string fMyName;
    // Data container
    std::map<uint8_t, std::vector<uint32_t>> fReadoutData;

    // list of registers to perserve
    std::vector<std::string> fBrdRegsToPerserve;
    DetectorDataContainer    fChipRegsToPerserve;

    //
    boardData fBoardData;
    bool      fSaveTree{0};
#ifdef __USE_ROOT__
    TTree* fTree;
#endif
};
#endif
