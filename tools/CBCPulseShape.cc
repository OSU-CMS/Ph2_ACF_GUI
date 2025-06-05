#include "tools/CBCPulseShape.h"
#include "Utils/CBCChannelGroupHandler.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/Exception.h"
#include "Utils/Occupancy.h"
#include "Utils/ThresholdAndNoise.h"
#include "Utils/Utilities.h"

#include <math.h>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

std::string CBCPulseShape::fCalibrationDescription = "Run multiple SCurve with injection changing sampling point to reconstruct pulse shape";

CBCPulseShape::CBCPulseShape() : PedeNoise() {}

CBCPulseShape::~CBCPulseShape() {}

void CBCPulseShape::Initialise(bool pAllChan, bool pDisableStubLogic)
{
    fEventsPerPoint        = findValueInSettings<double>("PulseShape_Nevents", 10);
    fInitialLatency        = findValueInSettings<double>("PulseShapeInitialLatency", 200);
    fInitialDelay          = findValueInSettings<double>("PulseShape_InitialDelay", 0);
    fFinalDelay            = findValueInSettings<double>("PulseShape_FinalDelay", 25);
    fDelayStep             = findValueInSettings<double>("PulseShape_DelayStep", 1);
    fPulseAmplitude        = findValueInSettings<double>("PulseShape_PulseAmplitude", 150);
    fChannelGroup          = findValueInSettings<double>("PulseShape_ChannelGroup", -1);
    fPlotPulseShapeSCurves = findValueInSettings<double>("PlotPulseShapeSCurves", 0);

    fLimit = 0.01; // larger tollerance for SCurve limits

    LOG(INFO) << "Parsed settings:";
    LOG(INFO) << " Nevents = " << fEventsPerPoint;

    if(fChannelGroup >= 8) throw Exception(std::string(__PRETTY_FUNCTION__) + " fChannelGroup cannot be grater than 7");
    CBCChannelGroupHandler theChannelGroupHandler;
    if(fChannelGroup > 0) CBCChannelGroupHandler theChannelGroupHandler(std::bitset<NCHANNELS>(CBC_CHANNEL_GROUP_BITSET) << (fChannelGroup * 2));

    theChannelGroupHandler.setChannelGroupParameters(16, 1, 2);
    setChannelGroupHandler(theChannelGroupHandler);

    initializeRecycleBin();

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fCBCHistogramPulseShape.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void CBCPulseShape::runCBCPulseShape(void)
{
    LOG(INFO) << "Taking Data with " << fEventsPerPoint << " triggers!";

    this->enableTestPulse(true);
    disableStubLogic();

    setSameDac("TestPulsePotNodeSel", fPulseAmplitude);
    setSameDac("TriggerLatency", fInitialLatency);

    // setSameGlobalDac("TestPulsePotNodeSel",  pTPAmplitude);
    LOG(INFO) << BLUE << "Enabled test pulse. " << RESET;

    for(uint16_t delay = fInitialDelay; delay <= fFinalDelay; delay += fDelayStep)
    {
        uint16_t delayDAC   = 25 - (delay % 25);
        uint16_t latencyDAC = fInitialLatency - (delay / 25);
        if(delayDAC == 25)
        {
            delayDAC   = 0;
            latencyDAC = latencyDAC + 1;
        }
        LOG(INFO) << BOLDBLUE << "Scanning VcThr for delay = " << +delayDAC << " and latency = " << +latencyDAC << RESET;

        setSameDac("TestPulseDelay", delayDAC);
        setSameDac("TriggerLatency", latencyDAC);

        findPedestal();
        measureSCurves(fMeanStrips, fMeanPixels);
        extractPedeNoise();

#ifdef __USE_ROOT__
        LOG(INFO) << BOLDGREEN << "Plotting delay for " << +delay << RESET;
        fCBCHistogramPulseShape.fillCBCPulseShapePlots(delay, *fThresholdAndNoiseContainer);
        if(fPlotPulseShapeSCurves)
            for(auto& scurveOccupancy: fSCurveStripOccupancyMap) { fCBCHistogramPulseShape.fillSCurvePlots(scurveOccupancy.first, latencyDAC, delayDAC, *scurveOccupancy.second); }
#else

        if(fDQMStreamerEnabled)
        {
            ContainerSerialization theThresholdAndNoiseSerialization("CBCPulseShapeThresholdAndNoise");
            theThresholdAndNoiseSerialization.streamByHybridContainer(fDQMStreamer, *fThresholdAndNoiseContainer, delay);

            for(auto& scurveOccupancy: fSCurveStripOccupancyMap)
            {
                ContainerSerialization theSCurveSerialization("CBCPulseShapeSCurve");
                theSCurveSerialization.streamByHybridContainer(fDQMStreamer, *scurveOccupancy.second, scurveOccupancy.first, latencyDAC, delayDAC);
            }
        }
#endif
        fThresholdAndNoiseContainer->reset();
        cleanContainerVector();
    }

    this->enableTestPulse(false);
    setSameGlobalDac("TestPulsePotNodeSel", 0);
    LOG(INFO) << BLUE << "Disabled test pulse. " << RESET;
}

void CBCPulseShape::writeObjects()
{
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fCBCHistogramPulseShape.process();
#endif
}

// For system on chip compatibility
void CBCPulseShape::Running()
{
    LOG(INFO) << "Starting calibration example measurement.";
    Initialise();
    runCBCPulseShape();
    LOG(INFO) << "Done with calibration example.";
}

// For system on chip compatibility
void CBCPulseShape::Stop(void)
{
    LOG(INFO) << "Stopping calibration example measurement.";
    writeObjects();
    dumpConfigFiles();
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "Calibration example stopped.";
}
