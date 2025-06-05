/*!

        \file                   PhaseScan.h
        \brief                 class to do latency and threshold scans
        \author              Georg AUZINGER
        \version                1.0
        \date                   20/01/15
        Support :               mail to : georg.auzinger@cern.ch

 */

#ifndef PhaseScan_H__
#define PhaseScan_H__

#include "../Utils/CommonVisitors.h"
#include "../Utils/ContainerRecycleBin.h"
#include "../Utils/Visitor.h"
#include "tools/Tool.h"
#ifdef __USE_ROOT__
#include "../DQMUtils/DQMHistogramPhaseScan.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TGaxis.h"
#include "TH1F.h"
#include "TH2F.h"
#endif

using namespace Ph2_System;

/*!
 * \class PhaseScan
 * \brief Class to perform latency and threshold scans
 */
class Occupancy;

class PhaseScan : public Tool
{
  public:
    PhaseScan();
    ~PhaseScan();
    void Initialize();
    // this is used by commission, and supervisor
    void ScanPahse();
    void writeObjects();
    void ScanPhase();
    void Running() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;

  protected:
    void cleanContainerMap();
    void initializeRecycleBin() { fRecycleBin.setDetectorContainer(fDetectorContainer); }

  private:
    int  countStubs(Ph2_HwDescription::Hybrid* pFe, const Ph2_HwInterface::Event* pEvent, std::string pHistName, uint8_t pParameter);
    int  countHitsLat(BeBoard* pBoard, const std::vector<Event*> pEventVec, std::string pHistName, uint16_t pParameter, uint32_t pStartLatency);
    void parseSettings();

    //  Members
    uint32_t fNevents;
    // uint32_t fInitialThreshold;
    uint32_t fStartPhase;
    uint32_t fPhaseRange;
    uint32_t fPhaseStartLatency;
    uint32_t fPhaseLatencyRange;

    ContainerRecycleBin<Occupancy> fRecycleBin;

#ifdef __USE_ROOT__
    DQMHistogramPhaseScan fDQMHistogramPhaseScan;
#endif
};

#endif
