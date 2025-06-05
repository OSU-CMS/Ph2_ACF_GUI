#include "DQMUtils/DQMHistogramOTCicBypassTest.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"

#include "TFile.h"

//========================================================================================================================
DQMHistogramOTCicBypassTest::DQMHistogramOTCicBypassTest() {}

//========================================================================================================================
DQMHistogramOTCicBypassTest::~DQMHistogramOTCicBypassTest() {}

//========================================================================================================================
void DQMHistogramOTCicBypassTest::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END
}

//========================================================================================================================
void DQMHistogramOTCicBypassTest::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTCicBypassTest::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool DQMHistogramOTCicBypassTest::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES

    // As example, I'm expecting to receive a data stream from an uint32_t contained from calibration "OTCicBypassTest"
    // ContainerSerialization myStreamer("OTCicBypassTest");

    // if(myStreamer.attachDeserializer(inputStream))
    // {
    //     // It matched! Decoding data
    //     std::cout << "Matched OTCicBypassTest!!!!!\n";
    //     // Need to tell to the streamer what data are contained (in this case in every channel there is an object of type MyType)
    //     DetectorDataContainer theDetectorData = myStreamer.deserializeChannelContainer<MyType>(fDetectorContainer);
    //     // Filling the histograms
    //     myFillplotFunction(theDetectorData);
    //     return true;
    // }
    // the stream does not match, the expected (DQM interface will try to check if other DQM istogrammers are looking
    // for this stream)
    return false;
    // SoC utilities only - END
}
