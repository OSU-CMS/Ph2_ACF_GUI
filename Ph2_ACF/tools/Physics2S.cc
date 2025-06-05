/*!
  \file                  SSAPhysics.cc
  \brief                 Implementaion of Physics data taking
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "tools/Physics2S.h"
#include "HWInterface/D19cFWInterface.h"
#include "Utils/CBCChannelGroupHandler.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/Data2S.h"
#include "Utils/GenericDataArray.h"
#include "Utils/Occupancy.h"
#include "Utils/StartInfo.h"
#include "tools/CicFEAlignment.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

std::string Physics2S::fCalibrationDescription = "Take data";

void Physics2S::ConfigureCalibration()
{
    // #######################
    // # Retrieve parameters #
    // #######################
    saveRawData = this->findValueInSettings<double>("SaveRawData");
    doLocal     = false;

    // ###########################################
    // # Initialize directory and data container #
    // ###########################################

    ContainerFactory::copyAndInitChannel<Occupancy>(*fDetectorContainer, fOccupancyContainer);
    ContainerFactory::copyAndInitChannel<float>(*fDetectorContainer, fStubContainer);

    CBCChannelGroupHandler theChannelGroupHandler;
    theChannelGroupHandler.setChannelGroupParameters(16, 1, 2);
    setChannelGroupHandler(theChannelGroupHandler);

    CicFEAlignment cCicAligner;
    cCicAligner.Inherit(this);
    StartInfo theStartInfo;
    theStartInfo.setRunNumber(0);
    cCicAligner.Start(theStartInfo);
    cCicAligner.waitForRunToBeCompleted();
    cCicAligner.Reset();
    cCicAligner.dumpConfigFiles();
}

void Physics2S::Running()
{
    LOG(INFO) << GREEN << "[Physics2S::Start] Starting" << RESET;

    if(saveRawData == true)
    {
        char runString[7];
        sprintf(runString, "%06d", (fRunNumber & 0xF423F)); // max value can be 999999, to avoid GCC 8 warning
        this->addFileHandler(std::string(RESULTDIR) + "/run_" + runString + ".raw", 'w');
        this->initializeWriteFileHandler();
    }

    for(const auto cBoard: *fDetectorContainer) static_cast<D19cFWInterface*>(this->fBeBoardFWMap[static_cast<BeBoard*>(cBoard)->getId()])->ChipReSync();
    StartInfo theStartInfo;
    theStartInfo.setRunNumber(fRunNumber);
    SystemController::Start(theStartInfo);

    Physics2S::run();
}

void Physics2S::Stop()
{
    LOG(INFO) << GREEN << "[Physics2S::Stop] Stopping" << RESET;

    Tool::Stop();

    fTotalDataSize += getDataFromBoards();

    LOG(WARNING) << BOLDBLUE << "Number of collected events = " << fTotalDataSize << RESET;

    if(fTotalDataSize == 0) LOG(WARNING) << BOLDBLUE << "No data collected" << RESET;

    // ################
    // # Error report #
    // ################
    Physics2S::chipErrorReport();

    this->closeFileHandler();
}

void Physics2S::initialize(const std::string fileRes_, const std::string fileReg_)
{
    fileRes = fileRes_;
    fileReg = fileReg_;

    Physics2S::ConfigureCalibration();

#ifdef __USE_ROOT__
    myApp = nullptr;

    if(doDisplay == true) myApp = new TApplication("myApp", nullptr, nullptr);

    this->InitResultFile(fileRes);

    Physics2S::initHisto();
#endif

    doLocal = true;
}

unsigned int Physics2S::getDataFromBoards()
{
    unsigned int dataSize = 0;
    for(const auto cBoard: *fDetectorContainer)
    {
        dataSize += SystemController::ReadData(static_cast<BeBoard*>(cBoard), false);
        if(dataSize != 0)
        {
            const std::vector<Event*>& events = SystemController::GetEvents();
            // for(const auto& event : events)
            //     std::cout<<"L1 id = " << std::dec<<static_cast<D19cCic2Event*>(event)->L1Id(1,0)<<std::endl;
            fillDataContainer(cBoard, events);
        }
    }

    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theOccupancySerialization("Physics2SOccupancy");
        theOccupancySerialization.streamByHybridContainer(fDQMStreamer, fOccupancyContainer);

        ContainerSerialization theStubSerialization("Physics2SStub");
        theStubSerialization.streamByHybridContainer(fDQMStreamer, fStubContainer);
    }

    return dataSize;
}

void Physics2S::run()
{
    fTotalDataSize = 0;

    while(fKeepRunning)
    {
        fTotalDataSize += getDataFromBoards();

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}

void Physics2S::draw()
{
#ifdef __USE_ROOT__
    Physics2S::fillHisto();
    Physics2S::display();

    if(doDisplay == true) myApp->Run(true);
    this->WriteRootFile();
    this->CloseResultFile();
#endif
}

void Physics2S::initHisto()
{
#ifdef __USE_ROOT__
    histos.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void Physics2S::fillHisto()
{
#ifdef __USE_ROOT__
    histos.fillOccupancy(fOccupancyContainer);
    histos.fillStub(fStubContainer);
#endif
}

void Physics2S::display()
{
#ifdef __USE_ROOT__
    histos.process();
#endif
}

// void Physics2S::fillDataContainer(BoardContainer* const& cBoard)
// {

//     // ###################
//     // # Fill containers #
//     // ###################
//     const std::vector<Event*>& events = SystemController::GetEvents();
//     for(const auto& event: events)
// 	{
// 		for(const auto cOpticalGroup: *f2SDataContainer.getObject(cBoard->getId()))
// 		{
// 		    for(const auto cHybrid: *cOpticalGroup)
// 			{
// 		        for(const auto cChip: *cHybrid)
// 				{

//     				auto curchip = cBoard->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId());;
//                     if(curchip->getFrontEndType() != FrontEndType::MPA2) continue;

// 					auto data2S = cChip->getSummary<Data2S<NCHANNELS, MAX_NUMBER_OF_STUB_CLUSTERS_2S>>();
//                     data2S.fClusters    = fromVectorToGenericDataArray<Cluster2S, NCHANNELS>(static_cast<D19cCic2Event*>(event)->getClusters(cHybrid->getId(), cChip->getId()));
//                     data2S.fStubs       = fromVectorToGenericDataArray<EventStub, MAX_NUMBER_OF_STUB_CLUSTERS_2S>(static_cast<D19cCic2Event*>(event)->StubVector(cHybrid->getId(), cChip->getId()));
// 				}
// 			}
// 		}
// 	}
// }

void Physics2S::chipErrorReport() {}

void Physics2S::fillDataContainer(BoardContainer* cBoard, const std::vector<Event*> eventList)
{
    // std::cout<<__LINE__<<std::endl;
    clearContainers(cBoard);
    // std::cout<<__LINE__<<std::endl;

    // Assuming all chip will have all channels enabled:
    auto allChannelGroup = getChannelGroup(-1);

    for(auto event: eventList)
    {
        event->fillDataContainer(fOccupancyContainer.getObject(cBoard->getId()), allChannelGroup);
        // ###################
        // # Fill containers #
        // ###################
        for(const auto cOpticalGroup: *fStubContainer.getObject(cBoard->getId()))
        {
            for(const auto cHybrid: *cOpticalGroup)
            {
                for(const auto cChip: *cHybrid)
                {
                    auto stubList = static_cast<D19cCic2Event*>(event)->StubVector(cHybrid->getId(), cChip->getId());

                    for(auto& stub: stubList)
                    {
                        // std::cout<<__LINE__<<std::endl;
                        if(ceil(stub.getCenter()) != stub.getCenter())
                        {
                            // std::cout<<__LINE__<<std::endl;
                            if(size_t(ceil(stub.getCenter())) < 254u) cChip->getChannel<float>(0, size_t(ceil(stub.getCenter()))) += 0.5;
                            // std::cout<<__LINE__<<std::endl;
                            if(size_t(floor(stub.getCenter())) < 254u) cChip->getChannel<float>(0, size_t(floor(stub.getCenter()))) += 0.5;
                            // std::cout<<__LINE__<<std::endl;
                        }
                        else
                        {
                            // std::cout<<__LINE__<<std::endl;
                            if(stub.getCenter() < 254u) ++cChip->getChannel<float>(0, size_t(stub.getCenter()));
                            // std::cout<<__LINE__<<std::endl;
                        }
                        // std::cout<<__LINE__<<std::endl;
                    }

                    // std::cout<<__LINE__<<std::endl;
                }
            }
        }
    }
}

void Physics2S::clearContainers(BoardContainer* theBoard)
{
    // ####################
    // # Clear containers #
    // ####################
    for(const auto cOpticalGroup: *fOccupancyContainer.getObject(theBoard->getId()))
    {
        for(const auto cHybrid: *cOpticalGroup)
        {
            for(const auto cChip: *cHybrid)
            {
                for(auto& cChannel: *cChip->getChannelContainer<float>()) cChannel = 0.;
            }
        }
    }

    for(const auto cOpticalGroup: *fStubContainer.getObject(theBoard->getId()))
    {
        for(const auto cHybrid: *cOpticalGroup)
        {
            for(const auto cChip: *cHybrid)
            {
                for(auto& cChannel: *cChip->getChannelContainer<float>()) cChannel = 0.;
            }
        }
    }
}
