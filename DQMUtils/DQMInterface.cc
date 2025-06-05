#include "DQMUtils/DQMInterface.h"
#include "DQMUtils/DQMCalibrationFactory.h"
#include "NetworkUtils/TCPSubscribeClient.h"
#include "Parser/FileParser.h"
#include "Utils/ConfigureInfo.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/StartInfo.h"

#include "TFile.h"

#include <iostream>
#include <string>

//========================================================================================================================
DQMInterface::DQMInterface() : fListener(nullptr), fRunning(false), fOutputFile(nullptr) {}

//========================================================================================================================
DQMInterface::~DQMInterface(void)
{
    LOG(INFO) << __PRETTY_FUNCTION__ << RESET;

    destroy();
}

//========================================================================================================================
void DQMInterface::destroy(void)
{
    LOG(INFO) << __PRETTY_FUNCTION__ << RESET;

    if(fListener != nullptr) delete fListener;
    destroyHistogram();
    fListener = nullptr;
    // for(auto dqmHistogrammer: fDQMHistogrammerVector) delete dqmHistogrammer;
    // fDQMHistogrammerVector.clear();
    delete fOutputFile;
    fOutputFile = nullptr;

    LOG(INFO) << __PRETTY_FUNCTION__ << " DONE" << RESET;
}

//========================================================================================================================
void DQMInterface::destroyHistogram(void)
{
    for(auto dqmHistogrammer: fDQMHistogrammerVector) delete dqmHistogrammer;
    fDQMHistogrammerVector.clear();

    LOG(INFO) << __PRETTY_FUNCTION__ << " DONE" << RESET;
}

//========================================================================================================================
void DQMInterface::configure(const ConfigureInfo& theConfigureInfo)
{
    std::string calibrationName       = theConfigureInfo.getCalibrationName();
    std::string configurationFilePath = theConfigureInfo.getConfigurationFile();
    LOG(INFO) << __PRETTY_FUNCTION__ << RESET;

    Ph2_Parser::FileParser theFileParser;
    std::stringstream      out;

    CommunicationSettingConfig theCommunicationSettingConfig;
    theFileParser.parseCommunicationSettings(configurationFilePath, theCommunicationSettingConfig, out);
    fListener = new TCPSubscribeClient(theCommunicationSettingConfig.fDQMCommunication.fIP, theCommunicationSettingConfig.fDQMCommunication.fPort);

    if(!fListener->connect())
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << " CAN'T CONNECT TO SERVER!" << RESET;
        abort();
    }
    LOG(INFO) << __PRETTY_FUNCTION__ << " DQM connected" << RESET;

    theFileParser.parseHW(configurationFilePath, &fDetectorStructure, out);
    theFileParser.parseSettings(configurationFilePath, fSettingsMap, out);

    theConfigureInfo.setEnabledObjects(&fDetectorStructure);

    DQMCalibrationFactory theDQMCalibrationFactory;
    fDQMHistogrammerVector = theDQMCalibrationFactory.createDQMHistogrammerVector(calibrationName);
}

//========================================================================================================================
void DQMInterface::startProcessingData(const StartInfo& theStartInfo)
{
    std::string resultDirectoryName = getResultDirectoryName(theStartInfo);
    std::string cCommand            = "mkdir -p " + resultDirectoryName;

    try
    {
        system(cCommand.c_str());
    }
    catch(std::exception& e)
    {
        LOG(ERROR) << BOLDRED << "Exceptin when trying to create Result Directory: " << e.what() << RESET;
    }
    std::string fileName = resultDirectoryName + "/Results.root";
    fOutputFile          = new TFile(fileName.c_str(), "RECREATE");
    for(auto dqmHistogrammer: fDQMHistogrammerVector) dqmHistogrammer->book(fOutputFile, fDetectorStructure, fSettingsMap);
    fRunning       = true;
    fRunningFuture = std::async(std::launch::async, &DQMInterface::running, this);
}

//========================================================================================================================
void DQMInterface::stopProcessingData(void)
{
    std::chrono::milliseconds span(1000);
    int                       timeout = 10; // in seconds

    fListener->disconnect();
    while(fRunningFuture.wait_for(span) == std::future_status::timeout && timeout > 0)
    {
        LOG(INFO) << __PRETTY_FUNCTION__ << " Process still running! Waiting " << timeout-- << " more seconds!" << RESET;
    }

    LOG(INFO) << __PRETTY_FUNCTION__ << " Thread done running" << RESET;

    if(fDataBuffer.size() > 0)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << " Buffer should be empty, some data were not read, Aborting" << RESET;
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << " Buffer size:" << fDataBuffer.size() << RESET;
        std::string inputStream(fDataBuffer.begin(), fDataBuffer.end());
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << " Buffer content:\n" << inputStream << RESET;
        abort();
    }

    for(auto dqmHistogrammer: fDQMHistogrammerVector) dqmHistogrammer->process();
    fOutputFile->Write();
}

//========================================================================================================================
bool DQMInterface::running()
{
    // CheckStream* theCurrentStream;
    // int               packetNumber = -1;
    std::vector<char> tmpDataBuffer;
    PacketHeader      thePacketHeader;
    uint8_t           packerHeaderSize = thePacketHeader.getPacketHeaderSize();

    while(fRunning)
    {
        // LOG(INFO) << __PRETTY_FUNCTION__ << " Running = " << fRunning << RESET;
        try
        {
            tmpDataBuffer = fListener->receive<std::vector<char>>();
        }
        catch(const std::exception& e)
        {
            LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "Error: " << e.what() << RESET;
            fRunning = false;
            break;
        }
        LOG(DEBUG) << "Got something" << RESET;
        LOG(DEBUG) << "Tmp buffer size: " << tmpDataBuffer.size() << RESET;
        fDataBuffer.insert(fDataBuffer.end(), tmpDataBuffer.begin(), tmpDataBuffer.end());
        LOG(DEBUG) << "Data buffer size: " << fDataBuffer.size() << RESET;
        while(fDataBuffer.size() > 0)
        {
            if(fDataBuffer.size() < packerHeaderSize)
            {
                LOG(WARNING) << BOLDBLUE << "Not enough bytes to retrieve data stream" << RESET;
                break; // Not enough bytes to retreive the packet size
            }
            uint32_t packetSize = thePacketHeader.getPacketSize(fDataBuffer);

            LOG(DEBUG) << "Vector size  = " << fDataBuffer.size() << "; expected = " << packetSize << RESET;

            if(fDataBuffer.size() < packetSize)
            {
                LOG(DEBUG) << "Packet not completed, waiting" << RESET;
                break;
            }

            std::string inputStream(fDataBuffer.begin() + packerHeaderSize, fDataBuffer.begin() + packetSize);
            fDataBuffer.erase(fDataBuffer.begin(), fDataBuffer.begin() + packetSize);

            if(inputStream == END_OF_TRANSMISSION_MESSAGE)
            {
                LOG(INFO) << BOLDBLUE << __PRETTY_FUNCTION__ << " End of transmission message received, stopping listening thread" << RESET;
                fRunning = false;
                break;
            }
            else
            {
                bool decodedByOneDQM = false;
                for(auto dqmHistogrammer: fDQMHistogrammerVector)
                {
                    if(dqmHistogrammer->fill(inputStream))
                    {
                        decodedByOneDQM = true;
                        break;
                    }
                }
                if(!decodedByOneDQM)
                {
                    LOG(WARNING) << BOLDRED << __PRETTY_FUNCTION__ << " No DQM histogrammer decoded message " << inputStream << ", aborting..." << RESET;
                    abort();
                }
            }
        }
    }

    return fRunning;
}
