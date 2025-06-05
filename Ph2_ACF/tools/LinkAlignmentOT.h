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

#ifndef LinkAlignmentOT_h__
#define LinkAlignmentOT_h__

#include "tools/OTTool.h"

using namespace Ph2_HwDescription;

class LinkAlignmentOT : public OTTool
{
  public:
    LinkAlignmentOT();
    ~LinkAlignmentOT();

    void Initialise();
    void Running() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;

    // get alignment results
    bool getStatus() const { return fSuccess; }

    bool    PhaseAlignBEdata(const Ph2_HwDescription::BeBoard* pBoard);
    uint8_t getBeSamplingDelay(uint8_t pBoardIndx, uint8_t pOGIndx, uint8_t pHybridIndx, uint8_t pLineIndx)
    {
        auto cBeSamplingDelay      = fBeSamplingDelay.getObject(pBoardIndx);
        auto cBeSamplingDelayOG    = cBeSamplingDelay->getObject(pOGIndx);
        auto cBeSamplingDelayHybrd = cBeSamplingDelayOG->getObject(pHybridIndx);
        return cBeSamplingDelayHybrd->getSummary<std::vector<uint8_t>>()[pLineIndx];
    }

    void    AlignStubPackage();
    uint8_t GetLpGBTDelay(const OpticalGroup* pOpticalGroup) const
    {
        auto cBoardId   = pOpticalGroup->getBeBoardId();
        auto cBoardIter = std::find_if(fDetectorContainer->begin(), fDetectorContainer->end(), [&cBoardId](Ph2_HwDescription::BeBoard* x) { return x->getId() == cBoardId; });
        return fLpGBTSamplingDelay.getObject((*cBoardIter)->getId())->getObject(pOpticalGroup->getId())->getFirstObject()->getSummary<uint8_t>();
    }
    bool L1WordAlignment(const Ph2_HwDescription::OpticalGroup* pOpticalGroup, bool pScope, uint8_t pSkipLine = 0);

  protected:
    // Alignment parameters
    DetectorDataContainer fLpGBTSamplingDelay;
    DetectorDataContainer fBeSamplingDelay; // one per line per data line from hybrid
    DetectorDataContainer fBeBitSlip;       // one per line per data line from hybrid
    bool                  fStubDebug{false};
    bool                  fL1Debug{false};
    bool                  fAllowZeroBitslip{true};

    bool AlignLpGBTInputs(const Ph2_HwDescription::OpticalGroup* pOpticalGroup);
    bool PhaseAlignBEdata(const Ph2_HwDescription::OpticalGroup* pOpticalGroup);
    bool WordAlignBEdata(const Ph2_HwDescription::OpticalGroup* pOpticalGroup, bool pDisableUnsresponsiveHybrids = true);
    bool AlignStubPackage(const Ph2_HwDescription::OpticalGroup* pOpticalGroup);
    void AlignStubPackage(Ph2_HwDescription::BeBoard* pBoard);
    bool Align();

  private:
};
#endif
