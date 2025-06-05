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

#ifndef StubQuickCheck_h__
#define StubQuickCheck_h__

#include "PedeNoise.h"

class StubQuickCheck : public PedeNoise
{
    using RegisterVector      = std::vector<std::pair<std::string, uint8_t>>;
    using TestGroupChannelMap = std::map<int, std::vector<uint8_t>>;

  public:
    StubQuickCheck();
    ~StubQuickCheck();

    void Initialise(bool pAllChan = false, bool pDisableStubLogic = true) override;
    void StubCheck(BeBoard* pBoard, std::vector<Event*> pEvents);
    void Running() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;
    void writeObjects();

  protected:
  private:
    // Containers
    DetectorDataContainer fNoiseContainer, fPedestalContainer, fOccupancyContainer, fShortsContainer;

// Canvases
// Counters

// Settings

// mapping of FEs for CIC

// booking histograms
#ifdef __USE_ROOT__
//  DQMHistogramCic fDQMHistogram;
#endif
};

#endif