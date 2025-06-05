#include "NetworkUtils/TCPSubscribeClient.h"
#include "Parser/FileParser.h"
#include "Parser/ParserDefinitions.h"
#include "Utils/Container.h"

#include "MonitorDQM/MonitorDQMInterface.h"
#include "MonitorDQM/MonitorDQMPlot2S.h"
#include "MonitorDQM/MonitorDQMPlotPS.h"
#include "Parser/DetectorMonitorConfig.h"
#include "Utils/ConfigureInfo.h"
#include "Utils/ContainerSerialization.h"

#include "TFile.h"

#include <ctime>
#include <iostream>
#include <string>

//========================================================================================================================
MonitorDQMInterface::MonitorDQMInterface() : fListener(nullptr), fRunning(false), fOutputFile(nullptr) {}

//========================================================================================================================
MonitorDQMInterface::~MonitorDQMInterface(void)
{
    LOG(INFO) << __PRETTY_FUNCTION__ << RESET;

    destroy();
}

//========================================================================================================================
void MonitorDQMInterface::destroy(void)
{
    LOG(INFO) << __PRETTY_FUNCTION__ << RESET;

    if(fListener != nullptr) delete fListener;
    destroyDQMs();
    fListener = nullptr;
    for(auto monitorDQM: fMonitorDQMVector) delete monitorDQM;
    fMonitorDQMVector.clear();
    delete fOutputFile;
    fOutputFile = nullptr;

    LOG(INFO) << __PRETTY_FUNCTION__ << " DONE" << RESET;
}

//========================================================================================================================
void MonitorDQMInterface::destroyDQMs(void)
{
    for(auto monitorDQM: fMonitorDQMVector) delete monitorDQM;
    fMonitorDQMVector.clear();

    LOG(INFO) << __PRETTY_FUNCTION__ << " DONE" << RESET;
}

//========================================================================================================================
void MonitorDQMInterface::configure(const ConfigureInfo& theConfigureInfo)
{
    Ph2_Parser::FileParser theFileParser;
    std::stringstream      out;
    std::string            configurationFilePath = theConfigureInfo.getConfigurationFile();

    CommunicationSettingConfig theCommunicationSettingConfig;
    theFileParser.parseCommunicationSettings(configurationFilePath, theCommunicationSettingConfig, out);
    fListener = new TCPSubscribeClient(theCommunicationSettingConfig.fMonitorDQMCommunication.fIP, theCommunicationSettingConfig.fMonitorDQMCommunication.fPort);

    if(!fListener->connect())
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << " CAN'T CONNECT TO SERVER!" << RESET;
        abort();
    }
    LOG(INFO) << __PRETTY_FUNCTION__ << " DQM connected" << RESET;

    theFileParser.parseHW(configurationFilePath, &fDetectorStructure, out);

    theConfigureInfo.setEnabledObjects(&fDetectorStructure);

    DetectorMonitorConfig theDetectorMonitorConfig;
    theFileParser.parseMonitor(configurationFilePath, theDetectorMonitorConfig, out);

    if(theDetectorMonitorConfig.fMonitoringType == MONITORING_NODE_TYPE_ATTRIBUTE_2S_VALUE) fMonitorDQMVector.push_back(new MonitorDQMPlot2S());
    if(theDetectorMonitorConfig.fMonitoringType == MONITORING_NODE_TYPE_ATTRIBUTE_PS_VALUE) fMonitorDQMVector.push_back(new MonitorDQMPlotPS());

    fOutputFile = new TFile("Monitor_tmp.root", "RECREATE");
    for(auto monitorDQM: fMonitorDQMVector) monitorDQM->book(fOutputFile, fDetectorStructure, theDetectorMonitorConfig);
}

//========================================================================================================================
void MonitorDQMInterface::startProcessingData()
{
    fRunning       = true;
    fRunningFuture = std::async(std::launch::async, &MonitorDQMInterface::running, this);
}

//========================================================================================================================
void MonitorDQMInterface::stopProcessingData(void)
{
    std::chrono::milliseconds span(1000);
    int                       timeout = 10; // in seconds

    fListener->disconnect();
    while(fRunningFuture.wait_for(span) == std::future_status::timeout && timeout >= 0)
    {
        LOG(INFO) << __PRETTY_FUNCTION__ << " Process still running! Waiting " << timeout-- << " more seconds!" << RESET;
    }

    LOG(INFO) << __PRETTY_FUNCTION__ << " Thread done running" << RESET;

    if(fDataBuffer.size() > 0)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << " Buffer should be empty, some data were not read, Aborting" << RESET;
        abort();
    }

    for(auto monitorDQM: fMonitorDQMVector) monitorDQM->process();
    fOutputFile->Write();
}

//========================================================================================================================
bool MonitorDQMInterface::running()
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
                for(auto monitorDQM: fMonitorDQMVector)
                {
                    if(monitorDQM->fill(inputStream))
                    {
                        decodedByOneDQM = true;
                        break;
                    }
                }
                if(!decodedByOneDQM)
                {
                    LOG(WARNING) << BOLDRED << __PRETTY_FUNCTION__ << "None decoded message " << inputStream << ", aborting..." << RESET;
                    abort();
                }
            }
        }
    }

    return fRunning;
}
