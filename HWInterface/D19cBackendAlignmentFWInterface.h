#ifndef __D19cBackendAlignmentFWInterface_H__
#define __D19cBackendAlignmentFWInterface_H__

#include "HWDescription/Definition.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

class BoardContainer;
class BoardDataContainer;
namespace Ph2_HwDescription
{
class Chip;
}
namespace Ph2_HwInterface
{

class PhaseTuningControl
{
    friend class PhaseTuningReply;

  public:
    PhaseTuningControl(bool isOptical) : fIsOptical(isOptical) {};
    enum class Command
    {
        ReturnConfig       = 0,
        ReturnResult       = 1,
        Configure          = 2,
        SetSyncPatternMask = 3,
        SetSyncPattern     = 4,
        Align              = 5,
        ReturnFSMstatus    = 6
    };
    enum class Mode
    {
        Auto   = 0,
        Slave  = 1,
        Manual = 2
    };
    // setter
    void setBitSlip(uint8_t theBitSlip) { fBitSlip = theBitSlip; }
    void setDelay(uint8_t theDelay) { fDelay = theDelay; }
    void setSyncPatternMask(uint16_t theSyncPatternMask) { fSyncPatternMask = theSyncPatternMask; }
    void setSyncPattern(uint16_t theSyncPattern) { fSyncPattern = theSyncPattern; }
    void setEnableSync(bool enableSync) { fEnableSync = enableSync; }
    void setDoWordAlignment(bool doWordAlignment) { fDoWordAlignment = doWordAlignment; }
    void setDoPhaseAlignment(bool doPhaseAlignment) { fDoPhaseAlignment = doPhaseAlignment; }
    void setDoReset(bool doReset) { fDoReset = doReset; }
    void setApplyManual(bool applyManual) { fApplyManual = applyManual; }
    void setEnableCustomPattern(bool enablePRBS) { fEnablePRBS = enablePRBS; }
    void setMasterLineId(uint8_t theMasterLineId) { fMasterLineId = theMasterLineId; }
    void setEnableLFSR(bool enableLFSR) { fEnableLFSR = enableLFSR; }
    void setEnableL1A(bool enableL1A) { fEnableL1A = enableL1A; }
    void setEnableLCC(bool enableLCC) { fEnableLCC = enableLCC; }
    void setMode(Mode theMode) { fMode = theMode; }
    void setCommand(Command theCommand) { fCommand = theCommand; }
    void setLineId(uint8_t theLineId) { fLineId = theLineId; }
    void setChipId(uint8_t theChipId) { fChipId = theChipId; }
    void setHybridId(uint8_t theHybridId) { fHybridId = theHybridId; }

    void     resetCommandBits();
    uint32_t encodeCommand() const;

  private:
    bool     fIsOptical{true};
    uint8_t  fBitSlip{0};
    uint8_t  fDelay{0};
    uint16_t fSyncPatternMask{0};
    uint16_t fSyncPattern{0};
    bool     fDoWordAlignment{false};
    bool     fDoPhaseAlignment{false};
    bool     fDoReset{false};
    bool     fApplyManual{false};
    uint8_t  fMasterLineId{0};
    bool     fEnableSync{false};
    bool     fEnablePRBS{false};
    bool     fEnableLFSR{false};
    bool     fEnableL1A{false};
    bool     fEnableLCC{false};
    Mode     fMode{Mode::Auto};
    Command  fCommand{Command::ReturnConfig};
    uint8_t  fLineId{0};
    uint8_t  fChipId{0};
    uint8_t  fHybridId{0};

    uint32_t fWait_us{100};
};

class PhaseTuningReply
{
  public:
    enum class WordFSMstate
    {
        IdleWordOrWaitIserdese = 0,
        WaitFrame              = 1,
        ApplyBitslip           = 2,
        WaitBitslip            = 3,
        PatternVerification    = 4,
        NotDefined1            = 5,
        NotDefined2            = 6,
        NotDefined3            = 7,
        NotDefined4            = 8,
        NotDefined5            = 9,
        NotDefined6            = 10,
        NotDefined7            = 11,
        FailedFrame            = 12,
        FailedVerification     = 13,
        TunedWORD              = 14,
        Unknown                = 15
    };
    enum class PhaseFSMstate
    {
        IdlePHASE              = 0,
        ResetIDELAYE           = 1,
        WaitResetIDELAYE       = 2,
        ApplyInitialDelay      = 3,
        CheckInitialDelay      = 4,
        InitialSampling        = 5,
        ProcessInitialSampling = 6,
        ApplyDelay             = 7,
        CheckDelay             = 8,
        Sampling               = 9,
        ProcessSampling        = 10,
        WaitGoodDelay          = 11,
        FailedInitial          = 12,
        FailedToApplyDelay     = 13,
        TunedPHASE             = 14,
        Unknown                = 15
    };
    // getter
    uint8_t       getBitSlip() const { return fBitSlip; }
    uint8_t       getDelay() const { return fDelay; }
    bool          getIsDone() const { return fIsDone; }
    WordFSMstate  getWordFSMstate() const { return fWordFSMstate; }
    PhaseFSMstate getPhaseFSMstate() const { return fPhaseFSMstate; }
    uint8_t       getMasterLineId() const { return fMasterLineId; }

    void decodeReply(uint32_t reply, const PhaseTuningControl& thePhaseTuningControl);

  private:
    uint8_t                  fBitSlip{0};
    uint8_t                  fDelay{0};
    bool                     fIsDone{false};
    WordFSMstate             fWordFSMstate{WordFSMstate::Unknown};
    PhaseFSMstate            fPhaseFSMstate{PhaseFSMstate::Unknown};
    PhaseTuningControl::Mode fMode{PhaseTuningControl::Mode::Auto};
    uint8_t                  fMasterLineId{0};
};

struct AlignmentResult
{
    AlignmentResult() {}
    AlignmentResult(const PhaseTuningReply& thePhaseTuningReply);
    bool        fDone{false};
    bool        fWordAlignmentSuccess{false};
    bool        fPhaseAlignmentSuccess{false};
    std::string fWordAlignmentFSMstate  = {"NotDefined"};
    std::string fPhaseAlignmentFSMstate = {"NotDefined"};
    uint8_t     fDelay                  = 0;
    uint8_t     fBitslip                = 0;
};

class RegManager;
class D19cBackendAlignmentFWInterface
{
  public:
    D19cBackendAlignmentFWInterface(RegManager* theRegManager);
    ~D19cBackendAlignmentFWInterface();

    void enableAlignmentOnPRBS(uint8_t hybridId);
    void disableAlignmentOnPRBS(uint8_t hybridId);
    void enableAlignmentOnCustomPattern(uint8_t hybridId, uint16_t thePattern, uint16_t thePatternMask);
    void disableAlignmentOnCustomPattern(uint8_t hybridId);

    AlignmentResult              alignWord(uint8_t hybridId, uint8_t lineId);
    std::vector<AlignmentResult> alignWordAllLines(uint8_t hybridId, uint8_t numberOfLines);
    BoardDataContainer           alignWordAllHybrids(BoardContainer* theBoardContainer, uint8_t numberOfLines);
    AlignmentResult              tunePhase(uint8_t hybridId, uint8_t lineId);
    void                         setManualBitSlip(uint8_t hybridId, uint8_t lineId, uint8_t bitSlip);

    void setIsOptical(bool isOptical) { fIsOptical = isOptical; }
    void setSuppressPrintout(bool suppressPrintout) { fSuppressPrintout = suppressPrintout; }
    void setSuppressErrorPrintout(bool suppressErrorPrintout) { fSuppressErrorPrintout = suppressErrorPrintout; }

  private:
    RegManager*                 fTheRegManager{nullptr};
    bool                        fIsOptical{true};
    std::string                 fPhaseTuningControlRegisterName = "fc7_daq_ctrl.physical_interface_block.phase_tuning_ctrl";
    std::string                 fPhaseTuningResultRegisterName  = "fc7_daq_stat.physical_interface_block.phase_tuning_reply";
    std::map<uint8_t, bool>     fAlignOnCustomPattern;
    std::map<uint8_t, uint16_t> fCustomAlignmentPattern;
    std::map<uint8_t, uint16_t> fCustomAlignmentPatternMask;
    bool                        fSuppressPrintout{false};

    AlignmentResult              retrieveAlignmentResult(uint8_t hybridId, uint8_t lineId);
    std::vector<AlignmentResult> retrieveAllLineAlignmentResult(uint8_t hybridId, uint8_t numberOfLines);
    void                         writeCommand(const PhaseTuningControl& thePhaseTunerControl);
    void                         runWordAlignment(uint8_t hybridId, uint8_t lineId);
    bool                         fSuppressErrorPrintout{false};
};
} // namespace Ph2_HwInterface
#endif
