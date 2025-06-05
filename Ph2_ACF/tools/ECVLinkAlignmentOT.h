/*!
 *
 * \file ECVLinkAlignment.h
 * \brief Link alignment class, automated alignment procedure for CIC-lpGBT-BE
 * connected to FEs
 * \author Stefan Maier based on Sarah SEIF EL NASR-STOREY work in LinkAlignment
 * \date 21 / 11 / 23
 *
 * \Support : s.maier@kit.edu
 *
 */

#ifndef ECVLinkAlignmentOT_h__
#define ECVLinkAlignmentOT_h__

#include "tools/LinkAlignmentOT.h"

#ifdef __USE_ROOT__
#include "DQMUtils/DQMHistogramECV.h"
#endif

using namespace Ph2_HwDescription;

class ECVLinkAlignmentOT : public LinkAlignmentOT
{
  public:
    ECVLinkAlignmentOT();
    ~ECVLinkAlignmentOT();

    void Initialise();
    void Running() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;

    void writeObjects();
    bool Scan();
    bool isL1HeaderFound(const std::vector<uint32_t>& theWordVector, uint8_t numberOfBytesInSinglePacket);

  protected:
  private:
#ifdef __USE_ROOT__
    DQMHistogramECV fDQMHistogrammer;
#endif
    void                 SetCICClockPolarityAndStrength(const Ph2_HwDescription::OpticalGroup* pOpticalGroup, bool pInverted, uint8_t pStrength);
    std::vector<uint8_t> getGroupsAndChannels(const Ph2_HwDescription::OpticalGroup* pOpticalGroup, bool pGroups);
    void                 ECV(const Ph2_HwDescription::OpticalGroup* pOpticalGroup);

    std::vector<std::pair<uint8_t, std::pair<uint8_t, float>>> StubBitErrorTest(const Ph2_HwDescription::OpticalGroup* pOpticalGroup);
    void                                                       InitL1BitErrorTest(const OpticalGroup* pOpticalGroup);
    std::vector<std::pair<uint8_t, std::pair<uint8_t, float>>> L1BitErrorTest(const Ph2_HwDescription::OpticalGroup* pOpticalGroup);
    void                                                       SetlpGBTRxPhase(const Ph2_HwDescription::OpticalGroup* pOpticalGroup, uint8_t pPhase);
    void                                                       InitWordAlignStubs(const Ph2_HwDescription::OpticalGroup* pOpticalGroup);
    std::vector<std::pair<uint8_t, std::pair<uint8_t, bool>>>  CheckWordAlignBEdataStubs(const Ph2_HwDescription::OpticalGroup* pOpticalGroup);
    void                                                       StopWordAlignStubs(const Ph2_HwDescription::OpticalGroup* pOpticalGroup);
    std::vector<std::pair<uint8_t, std::pair<uint8_t, bool>>>  CheckWordAlignBEdataL1(const Ph2_HwDescription::OpticalGroup* pOpticalGroup);
    void    StoreBERInHistogram(uint8_t pClockPolarity, uint8_t pClockStrength, uint8_t pCicStrength, uint8_t pPhase, uint8_t pHybridId, uint8_t pLine, float pBer);
    void    StoreWordAlignInHistogram(uint8_t pClockPolarity, uint8_t pClockStrength, uint8_t pCicStrength, uint8_t pPhase, uint8_t pHybridId, uint8_t pLine, bool pAligned);
    void    StoreTrainedPhases(uint8_t pClockPolarity, uint8_t pClockStrength, uint8_t pCicStrength, uint8_t pHybridId, uint8_t pLine, uint8_t pPhase);
    void    StoreChosenPhase(uint8_t pClockPolarity, uint8_t pClockStrength, uint8_t pCicStrength, uint8_t pPhase);
    uint8_t getNumberOfBytesInSinglePacket(const Ph2_HwDescription::OpticalGroup* pOpticalGroup);
};
#endif
