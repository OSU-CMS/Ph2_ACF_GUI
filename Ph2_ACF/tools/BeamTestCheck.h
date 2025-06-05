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

#ifndef BeamTestCheck_h__
#define BeamTestCheck_h__

#include "OTTool.h"
#include "Utils/ContainerRecycleBin.h"

#ifdef __USE_ROOT__
#include "DQMUtils/DQMHistogramBeamTestCheck.h"
#include "TH1.h"
#endif

/*!
 * \class LatencyScan
 * \brief Class to perform latency and threshold scans
 */
class Occupancy;

using namespace Ph2_HwDescription;

class BeamTestCheck : public OTTool
{
  public:
    BeamTestCheck();
    ~BeamTestCheck();

    void CheckWithTP(uint8_t pContinuousReadout = 1);
    void ValidateTP();
    void CheckWithInternal(uint8_t pContinuousReadout = 1);
    void CheckWithExternal(uint8_t pContinuousReadout = 1);
    void CheckWithTLU(uint8_t pContinuousReadout = 1);
    void ValidateRaw();
    void ValidateExternal();
    void ValidateTLU();
    void Initialise();
    void Running() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;
    // void Reset();
    void writeObjects();
    // void ReadDataFromFile(std::string pRawFileName);
    // void SetReadoutPause(uint32_t pReadoutPause) { fReadoutPause = pReadoutPause; }
    void DisableAllFEs();
    void ScanStubLatency(uint8_t pContinuousReadout);
    void ScanL1Latency(uint8_t pContinuousReadout);

    void ConfigureScans(uint8_t pStatusL1, uint8_t pStatusStubs)
    {
        fScanL1Latency   = pStatusL1;
        fScanStubLatency = pStatusStubs;
    }

  protected:
    void initializeRecycleBin() { fRecycleBin.setDetectorContainer(fDetectorContainer); }
    void cleanContainerMap()
    {
        for(auto container: fSCurveOccupancyMap) fRecycleBin.free(container.second);
        fSCurveOccupancyMap.clear();
    }

    // Method to create injection pixels/strips
    void defInjection()
    {
        Ph2_HwInterface::Injection cInjection;
        // inject one cluster into each MPA-SSA pair
        fInjections.clear();
        cInjection.fRow    = 12;
        cInjection.fColumn = 98;
        fInjections.push_back(cInjection);
    }

  private:
    // Containers
    // TDC
    DetectorDataContainer fTDCContainer;
    // Latency
    DetectorDataContainer fLatencyContainer, fStubLatencyContainer;
    DetectorDataContainer fLatencyContainerS0, fLatencyContainerS1;
    DetectorDataContainer fLatencyContainerCoincidence;
    // Cluster size
    DetectorDataContainer fClusterOccupancy;
    DetectorDataContainer fClusterOccupancyS0, fClusterOccupancyS1;
    // Pedestals
    DetectorDataContainer fPedestalContainer;
    DetectorDataContainer fSignalContainer;
    // Optimal Latencies
    DetectorDataContainer fOptimalL1Latency, fOptimalStubLatency;
    // Hit Containers
    DetectorDataContainer fHitOccupancyS0, fHitOccupancyS1;
    DetectorDataContainer fHitOccupancyCoinc;
    DetectorDataContainer fHitContainerTDC;
    DetectorDataContainer fStubOccupancy;
    // Hit Maps
    DetectorDataContainer fHitMap, fStubMap;
    // Bend Maps
    DetectorDataContainer fBendMap;
    // Event count
    DetectorDataContainer fEventsWithSingleClusters, fEventsWithStubs;
    // Stub + Hit count per
    DetectorDataContainer fEventSubSet, fStubSubSet;

    std::map<uint16_t, DetectorDataContainer*> fSCurveOccupancyMap;
    ContainerRecycleBin<Occupancy>             fRecycleBin;

    void        PrepareForInternal(Ph2_HwDescription::BeBoard* pBoard, uint8_t pLimitTriggers = 0);
    void        PrepareForExternalTP(Ph2_HwDescription::BeBoard* pBoard);
    void        PrepareForTP(Ph2_HwDescription::BeBoard* pBoard);
    void        PrepareForExternal(Ph2_HwDescription::BeBoard* pBoard);
    void        PrepareForTLU(Ph2_HwDescription::BeBoard* pBoard);
    void        ScanLatency(Ph2_HwDescription::BeBoard* pBoard, uint8_t pContinuousReadout);
    void        ScanThreshold(Ph2_HwDescription::BeBoard* pBoard);
    void        UpdateClusterContainers(Ph2_HwDescription::BeBoard* pBoard, const std::vector<Ph2_HwInterface::Event*> pEvents, size_t pIndx);
    void        ProcessEvents(Ph2_HwDescription::BeBoard* pBoard);
    void        Count(const std::vector<Ph2_HwInterface::Event*> pEvents, size_t pTriggerId, uint8_t pFillCorrelations = 0, uint8_t pPrintOut = 0);
    void        Validate();
    std::string GetStubLatencyRegName(int pOGId);
    void        SetStubLatencyOG(BeBoard* pBoard, int pOGId, int pStubLatency);
    unsigned    GetBitsFromStubLatency(uint32_t pStubLatency, uint8_t pOGId);

    std::vector<Ph2_HwInterface::Injection> fInjections;

    // calibration parameters
    uint8_t  fTPamplitude{255};
    uint8_t  fTPdelay{0};
    uint16_t fThreshold{0};
    uint16_t fStartLatency{0};
    uint16_t fLatencyRange{0};
    uint16_t fOptimalLatency;
    size_t   fThStep{0};

    // configure checks
    uint8_t fScanL1Latency{0};
    uint8_t fScanStubLatency{0};
#ifdef __USE_ROOT__
    DQMHistogramBeamTestCheck fDQMHistogrammer;
#endif
};
#endif
