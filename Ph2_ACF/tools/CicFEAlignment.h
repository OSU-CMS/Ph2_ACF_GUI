/*!
 *
 * \file CicFEAlignment.h
 * \brief CIC FE alignment class, automated alignment procedure for CICs
 * connected to FEs
 * \author Sarah SEIF EL NASR-STOREY
 * \date 28 / 06 / 19
 *
 * \Support : sarah.storey@cern.ch
 *
 */

#ifndef CicFEAlignment_h__
#define CicFEAlignment_h__

#include "OTTool.h"
#include <map>
// #ifdef __USE_ROOT__

#ifndef AlignmentValues
typedef std::vector<uint8_t> AlignmentValues;
#endif

#ifndef StubLineData
typedef std::vector<std::string> StubLineData;
#endif

#ifndef PhyPortCnfg
typedef std::pair<uint8_t, uint8_t> PhyPortCnfg;
#endif

#ifndef SlvsLineStatus
typedef std::pair<uint32_t, std::string> SlvsLineStatus;
#endif

#ifdef __USE_ROOT__
#include "DQMUtils/DQMHistogramCicFEAlignment.h"
#include "TH1.h"
#endif

// add break codes here
const uint8_t FAILED_PHASE_ALIGNMENT = 1;
const uint8_t FAILED_WORD_ALIGNMENT  = 2;
const uint8_t FAILED_BX_ALIGNMENT    = 3;

class CicFEAlignment : public OTTool
{
    using RegisterVector      = std::vector<std::pair<std::string, uint8_t>>;
    using TestGroupChannelMap = std::map<int, std::vector<uint8_t>>;

  public:
    CicFEAlignment();
    ~CicFEAlignment();

    void Initialise();
    bool CicLpGbtAlignment();
    bool CicLpGbtAlignment(const Ph2_HwDescription::OpticalGroup* pOpticalGroup);
    bool AlignInputs();
    void SetStaticPhaseAlignment();
    // manual scan of CIC input phases
    void                  InputLineScan();
    uint8_t               GenManPatternOutLine(uint8_t pLine);
    void                  ScanInputPhase(uint8_t pOutLine, uint8_t pPattern, uint8_t pStartScan, uint8_t pEndScan);
    DetectorDataContainer CheckCicInput(uint8_t pOutLine, uint8_t pPattern, uint8_t pPhase);
    void                  CheckOutLine(uint8_t pOutLine, uint8_t pPattern, uint8_t pPhase, DetectorDataContainer& pLineData, DetectorDataContainer& pErrorCounter);
    SlvsLineStatus        CheckPhyPort(const Ph2_HwDescription::Hybrid* pHybrid, PhyPortCnfg pPhyPortCnfg, uint8_t pPhase, uint8_t pPattern);
    // automated configuration of CIC input phase and delay
    bool PhaseAlignment(uint16_t pWait_us = 10, uint32_t pNTriggers = 500);
    bool WordAlignment(uint32_t pWait_us = 10);
    bool Bx0Alignment(uint8_t pFe = 0, uint8_t pLine = 4, uint16_t pDelay = 1, uint16_t pWait_ms = 100, int cNrials = 3);
    bool SetBx0Delay(uint8_t pDelay = 8, uint8_t pStubPackageDelay = 3);
    bool BackEndAlignment();
    void Running() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;
    void writeObjects();

    // get alignment results
    uint8_t getPhaseAlignmentValue(Ph2_HwDescription::BeBoard* pBoard, Ph2_HwDescription::OpticalGroup* pGroup, Ph2_HwDescription::Hybrid* pFe, Ph2_HwDescription::ReadoutChip* pChip, uint8_t pLine)
    {
        return fPhaseAlignmentValues.getObject(pBoard->getId())->getObject(pGroup->getId())->getObject(pFe->getId())->getObject(pChip->getId())->getSummary<AlignmentValues>()[pLine];
    }
    uint8_t getWordAlignmentValue(Ph2_HwDescription::BeBoard* pBoard, Ph2_HwDescription::OpticalGroup* pGroup, Ph2_HwDescription::Hybrid* pFe, Ph2_HwDescription::ReadoutChip* pChip, uint8_t pLine)
    {
        return fWordAlignmentValues.getObject(pBoard->getId())->getObject(pGroup->getId())->getObject(pFe->getId())->getObject(pChip->getId())->getSummary<AlignmentValues>()[pLine];
    }
    bool getStatus() const { return fSuccess; }

  protected:
  private:
    // status
    bool fSuccess;
    // Containers
    DetectorDataContainer fPhaseAlignmentValues;
    DetectorDataContainer fWordAlignmentValues;
    DetectorDataContainer fRegMapContainer;
    DetectorDataContainer fBoardRegContainer;

    // mapping of FEs for CIC
    std::vector<uint8_t> fFEMapping{3, 2, 1, 0, 4, 5, 6, 7}; // FE --> FE CIC [2S]
    void                 SetStubWindowOffsets(uint8_t pBendCode, int pBend);

    // expected number of bx first stub
    // appears after resync
    // different for CBC and MPA
    uint8_t fStubBxDelay2S = 8;
    uint8_t fStubBxDelayPS = 22;

#ifdef __USE_ROOT__
    DQMHistogramCicFEAlignment fDQMHistogrammer;
#endif
};

#endif
// #endif
