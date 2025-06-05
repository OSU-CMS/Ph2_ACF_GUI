/*!
 *
 * \file PSPixelAlive.h
 * \brief Calibration class, calibration of the hardware
 * \author Adam Kobert
 * \date 2 / 27 / 23
 *
 *
 *
 */

#ifndef PSPixelAlive_h__
#define PSPixelAlive_h__

#include "Utils/CommonVisitors.h"
#include "Utils/ContainerRecycleBin.h"
#include "Utils/Visitor.h"
#include "tools/Tool.h"

#ifdef __USE_ROOT__
#include "DQMUtils/DQMHistogramPSPixelAlive.h"
#endif

#include <map>

using namespace Ph2_System;

class DetectorContainer;
class Occupancy;

class PSPixelAlive : public Tool
{
  public:
    PSPixelAlive();
    ~PSPixelAlive();
    void clearDataMembers();

    void Initialise();
    void measurePixels(); // Calls Occupancy Measurements and Plotting Functions
                          //    void Validate();
    void writeObjects();

    //    void Run() override;
    //    void Stoper() override;
    void Run();
    void Stoper();
    //    void ConfigureCalibration() override;
    void Pause() override;
    void Resume() override;
    void Reset();

  protected:
    void measureOccupancy();
    void cleanContainerVector();
    void initializeRecycleBin() { fRecycleBin.setDetectorContainer(fDetectorContainer); }

    uint8_t fPulseAmplitude_MPA{0};
    uint8_t fPulseAmplitude_SSA{0};
    uint8_t fPulseThreshold_MPA{0};
    uint8_t fPulseThreshold_SSA{0};

    uint32_t fPSPixelAliveMask{0};
    uint32_t fEventsPerPoint{0};
    uint32_t fMaxNevents{65535};
    int      fNEventsPerBurst{-1};
    bool     fUseFixRange{false};
    uint16_t fMinThreshold{0};
    uint16_t fMaxThreshold{1023};
    uint32_t fNeventsForValidation{0};
    float    fMaskingThreshold{0};
    bool     fMaskNoisyChannels{0};

    //    DetectorDataContainer*                     fThresholdAndNoiseContainer;
    //    std::map<uint16_t, DetectorDataContainer*> fSCurveStripOccupancyMap, fSCurvePixelOccupancyMap;

  private:
    // to hold the original register values
    std::vector<EventType> fEventTypes;
    DetectorDataContainer* fStubLogicValue;
    DetectorDataContainer* fHIPCountValue;
    DetectorDataContainer  fBoardRegContainer;

    bool fWithCBC = true;
    bool fWithSSA = false;
    bool fWithMPA = false;

    // Settings
    bool fDisableStubLogic{true};

    void producePSPixelAlivePlots();

    // for validation
    void maskNoisyChannels(BoardDataContainer* board);

    ContainerRecycleBin<Occupancy> fRecycleBin;

#ifdef __USE_ROOT__
    DQMHistogramPSPixelAlive fDQMHistogramPSPixelAlive;
#endif
};

#endif
