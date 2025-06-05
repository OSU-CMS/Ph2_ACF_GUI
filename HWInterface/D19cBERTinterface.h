#ifndef __D19C_BERT_INTERFACE_H__
#define __D19C_BERT_INTERFACE_H__

#include <cstdint>
#include <map>
#include <vector>

class BoardContainer;
class OpticalGroupContainer;
class BoardDataContainer;
class OpticalGroupDataContainer;

namespace Ph2_HwInterface
{

class RegManager;

class BitErrorTestControl
{
    friend class BitErrorTestReply;

  public:
    BitErrorTestControl() {}

    enum class Command
    {
        ReturnConfig        = 0,
        ReturnFirstPattern  = 1,
        Configure           = 2,
        SetFirstPattern     = 3,
        ReadCounterData     = 4,
        ErrorInject         = 5,
        ReadBERTfirstData   = 6,
        ReadBERTsampledData = 7
    };

    enum class CounterSelect
    {
        FrameCounterLSB   = 0,
        FrameCounterMSB   = 1,
        FrameErrorCounter = 2,
        BitErrorCounter   = 3
    };

    enum class Mode
    {
        None0   = 0,
        PRBS    = 1,
        LSFR    = 2,
        Pattern = 3
    };

    void     resetCommandBits();
    uint32_t encodeCommand() const;
    void     getLine(const BitErrorTestControl& theBitErrorTestReply);

    void setHybridId(uint8_t theHybridId) { fHybridId = theHybridId; }
    void setChipId(uint8_t theChipId) { fChipId = theChipId; }
    void setLineId(uint8_t theLineId) { fLineId = theLineId; }
    void setLineSelect(uint8_t theLineSelect) { fLineSelect = theLineSelect; }
    void setCommand(Command theCommand) { fCommand = theCommand; }
    void setDebugMode(bool theDebugMode) { fDebugMode = theDebugMode; }
    void setCheckMode(bool theCheckMode) { fCheckMode = theCheckMode; }
    void setCounterSelect(CounterSelect theCounterSelect) { fCounterSelect = theCounterSelect; }
    void setMode(Mode theMode) { fMode = theMode; }
    void setCheckEnable(bool theCheckEnable) { fCheckEnable = theCheckEnable; }
    void setReceiveEnable(bool theReceiveEnable) { fReceiveEnable = theReceiveEnable; }
    void setFirstPattern(uint16_t theFirstPattern) { fFirstPattern = theFirstPattern; }
    void setErrorInjection(bool theErrorInjection) { fErrorInjection = theErrorInjection; }
    void setDataLoad(bool theDataLoad) { fDataLoad = theDataLoad; }
    void setIsMask(bool isMask) { fIsMask = isMask; }
    void setPatternWordIndex(uint8_t thePatternWordIndex) { fPatternWordIndex = thePatternWordIndex; }
    void setPattern(uint16_t thePattern) { fPattern = thePattern; }

  private:
    uint8_t       fHybridId{0};
    uint8_t       fChipId{0};
    uint8_t       fLineId{0};
    uint8_t       fLineSelect{0};
    Command       fCommand{Command::ReturnConfig};
    bool          fDebugMode{false};
    bool          fCheckMode{false};
    CounterSelect fCounterSelect{CounterSelect::FrameCounterLSB};
    Mode          fMode{Mode::None0};
    bool          fCheckEnable{false};
    bool          fReceiveEnable{false};
    uint16_t      fFirstPattern{0};
    bool          fErrorInjection{false};
    bool          fDataLoad{false};
    bool          fIsMask{false};
    uint8_t       fPatternWordIndex{0};
    uint16_t      fPattern{0};

    static bool          fIsDebugModeActivated;
    static bool          fCurrentCheckMode;
    static Mode          fCurrentMode;
    static CounterSelect fCurrentCounterSelect;
};

class BitErrorTestReply
{
  public:
    BitErrorTestReply() {};

    void decodeReply(uint32_t reply, const BitErrorTestControl& theBitErrorTestControl);

    uint8_t                   getHybridId() { return fHybridId; }
    uint8_t                   getChipId() { return fChipId; }
    uint8_t                   getLineId() { return fLineId; }
    bool                      getPRBScounterOverflow() { return fPRBScounterOverflow; }
    bool                      getLFSRcounterOverflow() { return fLFSRcounterOverflow; }
    uint8_t                   getPRBScheckStateMachineStatus() { return fPRBScheckStateMachineStatus; }
    bool                      getCheckMode() { return fCheckMode; }
    bool                      getCounterReset() { return fCounterReset; }
    uint8_t                   getCounterSelect() { return fCounterSelect; }
    BitErrorTestControl::Mode getMode() { return fMode; }
    bool                      getCheckEnable() { return fCheckEnable; }
    bool                      getReceiveEnable() { return fReceiveEnable; }
    uint16_t                  getFirstPattern() { return fFirstPattern; }
    uint32_t                  getPRBSframeCounterValueEmulator() { return fPRBSframeCounterValueEmulator; }
    uint32_t                  getPRBSbitCounterValueEmulator() { return fPRBSbitCounterValueEmulator; }
    uint32_t                  getPRBSframeCounterValuePredictNext() { return fPRBSframeCounterValuePredictNext; }
    uint32_t                  getPRBSbitCounterValuePredictNext() { return fPRBSbitCounterValuePredictNext; }

    uint32_t getPRBSfirstData() { return fPRBSfirstData; }
    uint32_t getLFSRfirstData() { return fLFSRfirstData; }
    uint32_t getPRBSdata() { return fPRBSdata; }
    uint32_t getLFSRdata() { return fLFSRdata; }
    uint32_t getFrameCounterLSB() { return fFrameCounterLSB; }
    uint32_t getFrameCounterMSB() { return fFrameCounterMSB; }

  private:
    uint8_t                   fHybridId{99};
    uint8_t                   fChipId{99};
    uint8_t                   fLineId{99};
    bool                      fPRBScounterOverflow{false};
    bool                      fLFSRcounterOverflow{false};
    uint8_t                   fPRBScheckStateMachineStatus{99};
    bool                      fCheckMode{false};
    bool                      fCounterReset{false};
    uint8_t                   fCounterSelect{99};
    BitErrorTestControl::Mode fMode{BitErrorTestControl::Mode::None0};
    bool                      fCheckEnable{false};
    bool                      fReceiveEnable{false};
    uint16_t                  fFirstPattern{0x999};
    uint32_t                  fPRBSframeCounterValueEmulator{0x999};
    uint32_t                  fPRBSbitCounterValueEmulator{0x999};
    uint32_t                  fPRBSframeCounterValuePredictNext{0x999};
    uint32_t                  fPRBSbitCounterValuePredictNext{0x999};
    uint32_t                  fPRBSfirstData{0x999};
    uint32_t                  fLFSRfirstData{0x999};
    uint32_t                  fPRBSdata{0x999};
    uint32_t                  fLFSRdata{0x999};
    uint32_t                  fFrameCounterLSB;
    uint32_t                  fFrameCounterMSB;
};

class D19cBERTinterface
{
  public:
    D19cBERTinterface(RegManager* theRegManager);
    ~D19cBERTinterface();

    void startPatternSyncronization(uint8_t hybridId, uint8_t lineId);
    void startBitErrorRateTest(uint8_t hybridId, uint8_t lineId);
    void stopBitErrorRateTest(uint8_t hybridId, uint8_t lineId);
    void haltBitErrorRateTest(uint8_t hybridId, uint8_t lineId);

    BoardDataContainer runBERTonSingleLine(BoardContainer* theBoardContainer, uint8_t lineNumber, bool is10Gmodule, float numberOfMatchedBits);

    void setCheckedPattern(uint8_t hybridId, const std::vector<uint32_t>& theCheckedPattern);
    void setCheckedPatternMask(uint8_t hybridId, const std::vector<uint32_t>& theCheckedPatternMask);
    void setUsePRBS(bool usePRBS) { fUsePRBS = usePRBS; }
    void setSuppressErrorPrintout(bool suppressErrorPrintout) { fSuppressErrorPrintout = suppressErrorPrintout; }

  private:
    RegManager*                                      fTheRegManager{nullptr};
    uint32_t                                         getBitErrorCounters(uint8_t hybridId, uint8_t lineId);
    uint64_t                                         getFrameCounters(uint8_t hybridId, uint8_t lineId, bool isMSB);
    uint32_t                                         getFirstData(uint8_t hybridId, uint8_t lineId);
    void                                             injectError(uint8_t hybridId, uint8_t lineId);
    void                                             selectFrameCounters(bool isMSB);
    void                                             waitForNeededBits(bool is10Gmodule, float numberOfMatchedBits);
    std::tuple<bool, bool, std::map<uint16_t, bool>> isStartPatternFound(BoardContainer* theBoardContainer, uint8_t lineNumber);
    bool     retrieveBitTestedCounterLine(BoardDataContainer* theBoardContainer, uint8_t lineNumber, bool is10Gmodule, float numberOfMatchedBits, std::map<uint16_t, bool> startPatternFoundHybridMap);
    void     retrieveErrorCounter(OpticalGroupDataContainer* theOpticalGroupContainer, uint8_t numberOfLines);
    void     retrieveErrorCounterLine(BoardDataContainer* theBoardContainer, uint8_t lineNumber);
    uint64_t readNumberOfTestedBit(uint16_t hybridId, uint8_t lineId, bool is10Gmodule);
    void     loadSampleData(uint16_t hybridId, uint8_t lineId);
    void     readSampleData(uint16_t hybridId, uint8_t lineId);
    bool     isStateMachineStarted(BoardContainer* theBoardContainer, uint8_t lineNumber);
    uint8_t  getCheckerFSMstatus(uint8_t hybridId, uint8_t lineId);
    void     loadCheckedPattern(uint8_t hybridId, uint8_t lineId);

    void              writeCommand(BitErrorTestControl theBitErrorTestControl);
    BitErrorTestReply readReplay(const BitErrorTestControl& theBitErrorTestControl);
    void              loadAllCheckedPatternsInBoard(BoardContainer* theBoardContainer);

    void loadAlignmentPattern(uint16_t hybridId, uint8_t lineId);

    uint8_t fLineSelect;

    std::map<uint8_t, std::vector<uint32_t>> fCheckedPatternMap;
    std::map<uint8_t, std::vector<uint32_t>> fCheckedPatternMaskMap;
    std::map<uint8_t, float>                 fNumberOfCheckedBitsMap;
    bool                                     fUsePRBS = true;
    bool                                     fSuppressErrorPrintout{false};
};

} // namespace Ph2_HwInterface

#endif