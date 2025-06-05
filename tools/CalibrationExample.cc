#include "tools/CalibrationExample.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"

#include <boost/any.hpp>
#include <math.h>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

std::string CalibrationExample::fCalibrationDescription = "Run a simple occupancy measurement";

CalibrationExample::CalibrationExample() : Tool(), fEventsPerPoint(0) {}

CalibrationExample::~CalibrationExample() {}

void CalibrationExample::Initialise(void)
{
    auto cSetting   = fSettingsMap.find("Nevents");
    fEventsPerPoint = (cSetting != std::end(fSettingsMap)) ? boost::any_cast<double>(cSetting->second) : 10;

    LOG(INFO) << "Parsed settings:";
    LOG(INFO) << " Nevents = " << fEventsPerPoint;

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramCalibrationExample.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void CalibrationExample::runCalibrationExample(void)
{
    LOG(INFO) << "Taking Data with " << fEventsPerPoint << " triggers!";

    DetectorDataContainer theHitContainer;
    ContainerFactory::copyAndInitChannel<uint32_t>(*fDetectorContainer, theHitContainer);

    // getting n events and filling the container:
    for(auto board: theHitContainer) // for on boards - begin
    {
        BeBoard* theBeBoard = static_cast<BeBoard*>(fDetectorContainer->getObject(board->getId()));
        // Send N triggers (as it was in the past)
        ReadNEvents(theBeBoard, fEventsPerPoint);
        // Get the event vector (as it was in the past)

        const std::vector<Event*>& eventVector = GetEvents();

        for(auto& event: eventVector) // for on events - begin
        {
            for(auto opticalGroup: *board)
            {
                for(auto hybrid: *opticalGroup) // for on hybrid - begin
                {
                    for(auto chip: *hybrid) // for on chip - begin
                    {
                        unsigned int channelNumber = 0;
                        for(auto& channel: *chip->getChannelContainer<uint32_t>()) // for on channel - begin
                        {
                            // retreive data in the old way and add to the current number of hits of the corresponding
                            // channel
                            channel += event->DataBit(hybrid->getId(), chip->getId(), 0, channelNumber++);
                        } // for on channel - end
                    } // for on chip - end
                } // for on hybrid - end
            } // for on opticalGroup - end
        } // for on events - end
    } // for on board - end

#ifdef __USE_ROOT__
      // Calibration is not running on the SoC: plotting directly the data, no shipping is done
    fDQMHistogramCalibrationExample.fillCalibrationExamplePlots(theHitContainer);
#else
    // Calibration is running on the SoC: shipping the data!!!
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theContainerSerialization("CalibrationExampleHits");
        theContainerSerialization.streamByHybridContainer(fDQMStreamer, theHitContainer);
    }
#endif
}

void CalibrationExample::writeObjects()
{
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramCalibrationExample.process();
#endif
}

// For system on chip compatibility
void CalibrationExample::Running()
{
    LOG(INFO) << "Starting calibration example measurement.";
    Initialise();
    setSameDac("Threshold", 593);
    runCalibrationExample();
    LOG(INFO) << "Done with calibration example.";
}

// For system on chip compatibility
void CalibrationExample::Stop(void)
{
    LOG(INFO) << "Stopping calibration example measurement.";
    writeObjects();
    dumpConfigFiles();
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "Calibration example stopped.";
}
