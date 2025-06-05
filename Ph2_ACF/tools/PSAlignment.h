/*!
 *
 * \file PSAlignment.h
 * \brief CIC FE alignment class, automated alignment procedure for CICs
 * connected to FEs
 * \author Sarah SEIF EL NASR-STOREY
 * \date 28 / 06 / 19
 *
 * \Support : sarah.storey@cern.ch
 *
 */

#ifndef PSAlignment_h__
#define PSAlignment_h__

#include "tools/OTTool.h"

#include <map>

struct MPAInputAlignment
{
    uint8_t  fL1InputPhase;
    uint8_t  fLatencyRx40;
    uint8_t  fStubInputPhase;
    uint8_t  fRetimePix;
    uint8_t  fEdgeSelT1;
    uint8_t  fEdgeSelL1;
    uint8_t  fEdgeSelStubs;
    uint16_t fStubOffset;
};
class PSAlignment : public OTTool
{
  public:
    PSAlignment();
    ~PSAlignment();

    void                                     Initialise();
    bool                                     AlignL1Inputs(Ph2_HwDescription::BeBoard* pBoard);
    bool                                     AlignStubInputs(Ph2_HwDescription::BeBoard* pBoard);
    bool                                     AlignInputs(Ph2_HwDescription::BeBoard* pBoard, uint8_t pChipId = 0);
    void                                     MapMPAOutputs(std::string pSetupType = "PSModule");
    void                                     ConfigureDefaultAlignmentParameters(std::string pSetupType = "PSModule");
    bool                                     Align();
    std::vector<std::pair<uint8_t, uint8_t>> AlignL1(Ph2_HwDescription::ReadoutChip* pChip, std::vector<Ph2_HwInterface::Injection> pInjections, uint8_t pEdgeSelRaw = 1);
    std::vector<std::pair<uint8_t, uint8_t>> AlignStubs(Ph2_HwDescription::ReadoutChip* pChip, std::vector<Ph2_HwInterface::Injection> pInjections, uint16_t pLatency);
    std::vector<std::pair<uint8_t, uint8_t>> AlignChip(Ph2_HwDescription::ReadoutChip* pChip, std::vector<Ph2_HwInterface::Injection> pInjections, uint16_t pLatency);
    void                                     Running() override;
    void                                     Stop() override;
    void                                     Pause() override;
    void                                     Resume() override;
    void                                     writeObjects();
    // get alignment results
    bool                           getStatus() const { return fSuccess; }
    std::vector<MPAInputAlignment> getAlignmentParameters(Ph2_HwDescription::ReadoutChip* pChip)
    {
        auto cBoardId   = pChip->getBeBoardId();
        auto cBoardIter = std::find_if(fDetectorContainer->begin(), fDetectorContainer->end(), [&cBoardId](Ph2_HwDescription::BeBoard* x) { return x->getId() == cBoardId; });
        auto cBoard     = (*cBoardIter);
        //
        auto cOGId             = pChip->getOpticalGroupId();
        auto cOpticalGroupIter = std::find_if(fDetectorContainer->getObject(cBoard->getId())->begin(),
                                              fDetectorContainer->getObject(cBoard->getId())->end(),
                                              [&cOGId](Ph2_HwDescription::OpticalGroup* x) { return x->getId() == cOGId; });
        auto cOG               = (*cOpticalGroupIter);
        //
        auto cHybridId   = pChip->getHybridId();
        auto cHybridIter = std::find_if(fDetectorContainer->getObject(cBoard->getId())->getObject(cOG->getId())->begin(),
                                        fDetectorContainer->getObject(cBoard->getId())->getObject(cOG->getId())->end(),
                                        [&cHybridId](Ph2_HwDescription::Hybrid* x) { return x->getId() == cHybridId; });
        auto cHybrid     = (*cHybridIter);
        //
        auto& cAlParsThisBoard = fAlParsContainer.getObject(cBoard->getId());
        auto& cAlParsThisOG    = cAlParsThisBoard->getObject(cOG->getId());
        auto& cAlParsThisHybrd = cAlParsThisOG->getObject(cHybrid->getId());
        auto& cAlParsThisChip  = cAlParsThisHybrd->getObject(pChip->getId());
        return cAlParsThisChip->getSummary<std::vector<MPAInputAlignment>>();
    }

  protected:
  private:
    // optimal latency
    uint16_t fOptimalLatency = 0;
    // status
    bool fSuccess;
    bool PSv2 = false;
    // Containers
    DetectorDataContainer fRegMapContainer;
    DetectorDataContainer fBoardRegContainer;
    DetectorDataContainer fAlParsContainer;
    DetectorDataContainer fStubAlParsContainer;
    DetectorDataContainer fL1AlParsContainer;

    bool FindLatency(Ph2_HwDescription::BeBoard* pBoard, uint8_t pChipId, std::vector<Ph2_HwInterface::Injection> pInjections, uint8_t pEdgeSelT1);
    void PrintAlignmentParameters(MPAInputAlignment pPar)
    {
        LOG(INFO) << BOLDGREEN << "########################" << RESET;
        LOG(INFO) << BOLDGREEN << "Edge select T1 is " << +pPar.fEdgeSelT1 << " Edge select L1 data is " << +pPar.fEdgeSelL1 << " Edge select Stub data is " << +pPar.fEdgeSelStubs << RESET;
        LOG(INFO) << BOLDGREEN << "Alignment parameters for L1 hit data : [" << +pPar.fL1InputPhase << "," << +pPar.fLatencyRx40 << "]" << RESET;
        LOG(INFO) << BOLDGREEN << "Alignment parameters for Stub data : [" << +pPar.fStubInputPhase << "," << +pPar.fRetimePix << "]" << RESET;
        LOG(INFO) << BOLDGREEN << "Stub offset is set to " << pPar.fStubOffset << RESET;
        LOG(INFO) << BOLDGREEN << "###########################" << RESET;
    }
    void ConfigureRawInputs(Ph2_HwDescription::ReadoutChip* pChip, MPAInputAlignment pPar, uint8_t pPrint = 1)
    {
        if(pPrint)
        {
            LOG(INFO) << BOLDBLUE << "Edge select T1 is " << +pPar.fEdgeSelT1 << " Edge select L1 data is " << +pPar.fEdgeSelL1 << RESET;
            LOG(INFO) << BOLDBLUE << "Alignment parameters for L1 hit data : [" << +pPar.fL1InputPhase << "," << +pPar.fLatencyRx40 << "]" << RESET;
        }
        // L1
        fReadoutChipInterface->WriteChipReg(pChip, "SelectEdgeT1", pPar.fEdgeSelT1);
        fReadoutChipInterface->WriteChipReg(pChip, "SelectEdgeL8", pPar.fEdgeSelL1);
        fReadoutChipInterface->WriteChipReg(pChip, "L1InputPhase", pPar.fL1InputPhase);
        fReadoutChipInterface->WriteChipReg(pChip, "LatencyRx40", pPar.fLatencyRx40);
    }
    void ConfigureStubInputs(Ph2_HwDescription::ReadoutChip* pChip, MPAInputAlignment pPar, uint8_t pPrint = 1)
    {
        if(pPrint)
        {
            LOG(INFO) << BOLDYELLOW << " Edge select Stub data is " << +pPar.fEdgeSelStubs << RESET;
            LOG(INFO) << BOLDYELLOW << "Alignment parameters for Stub data : [" << +pPar.fStubInputPhase << "," << +pPar.fRetimePix << "]" << RESET;
            LOG(INFO) << BOLDYELLOW << "Stub offset is set to " << pPar.fStubOffset << RESET;
        }
        // Stubs
        for(uint8_t cLine = 0; cLine < 8; cLine++)
        {
            std::stringstream cRegName;
            cRegName << "SelectEdgeL" << +cLine;
            fReadoutChipInterface->WriteChipReg(pChip, cRegName.str(), pPar.fEdgeSelStubs);
        }
        fReadoutChipInterface->WriteChipReg(pChip, "StubInputPhase", pPar.fStubInputPhase);
        fReadoutChipInterface->WriteChipReg(pChip, "RetimePix", pPar.fRetimePix);
    }
    void ConfigureAllInputs(Ph2_HwDescription::ReadoutChip* pChip, MPAInputAlignment pPar, uint8_t pPrint = 0)
    {
        LOG(INFO) << BOLDGREEN << "MPA#" << +pChip->getId() << " - configuring alignment inputs [L1 + Stubs]" << RESET;
        PrintAlignmentParameters(pPar);
        ConfigureRawInputs(pChip, pPar, pPrint);
        ConfigureStubInputs(pChip, pPar, pPrint);
    }
    void Validate(Ph2_HwDescription::BeBoard* pBoard, std::vector<Ph2_HwInterface::Injection> pInjections, uint8_t pEdgeSelT1 = 0);
    bool CheckFullMatch(Ph2_HwDescription::ReadoutChip*             pChip,
                        const std::vector<Ph2_HwInterface::Event*>& pEvents,
                        std::vector<Ph2_HwInterface::Injection>     pInjections,
                        uint8_t                                     pTriggerId,
                        size_t                                      pTriggerMult);
    bool CheckL1Data(Ph2_HwInterface::ClusterCollection<Ph2_HwInterface::PixelClusterPS, 32> pPClusters,
                     Ph2_HwInterface::ClusterCollection<Ph2_HwInterface::StripClusterPS, 32> pSClusters,
                     const std::vector<Ph2_HwInterface::Injection>                           pInjections);
// booking histograms
#ifdef __USE_ROOT__
//  DQMHistogramCic fDQMHistogram;
#endif
};
#endif
