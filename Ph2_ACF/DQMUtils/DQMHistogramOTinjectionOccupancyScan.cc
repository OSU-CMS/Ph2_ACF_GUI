#include "DQMUtils/DQMHistogramOTinjectionOccupancyScan.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"

#include "TFile.h"

//========================================================================================================================
DQMHistogramOTinjectionOccupancyScan::DQMHistogramOTinjectionOccupancyScan() {}

//========================================================================================================================
DQMHistogramOTinjectionOccupancyScan::~DQMHistogramOTinjectionOccupancyScan() {}

//========================================================================================================================
void DQMHistogramOTinjectionOccupancyScan::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    std::vector<float> theListOfPulseValues = convertStringToFloatList(findValueInSettings<std::string>(pSettingsMap, "OTinjectionOccupancyScan_ListOfInjectedPulses", "0, 0.25, 0.5, 1., 2."));
    uint32_t           theNumberOfEventsWithoutInjection = findValueInSettings<double>(pSettingsMap, "OTinjectionOccupancyScan_NumberOfEventsWithoutInjection", 1000000);
    uint32_t           theNumberOfEventsWithInjection    = findValueInSettings<double>(pSettingsMap, "OTinjectionOccupancyScan_NumberOfEventsWithInjection", 1000);

    for(size_t iteration = 0; iteration < theListOfPulseValues.size(); ++iteration)
    {
        float    pulseValue     = theListOfPulseValues.at(iteration);
        uint32_t numberOfEvents = (pulseValue == 0.) ? theNumberOfEventsWithoutInjection : theNumberOfEventsWithInjection;
        bookPlotsForInjection(theOutputFile, numberOfEvents, pulseValue, pulseValue, pulseValue, iteration);
    }
}