/*!
  \file                  SSAPhysics.cc
  \brief                 Implementaion of Physics data taking
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "tools/SSAPhysics.h"

#include "HWInterface/D19cFWInterface.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/Occupancy.h"
#include "Utils/StartInfo.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void SSAPhysics::ConfigureCalibration()
{
    // #######################
    // # Retrieve parameters #
    // #######################
    saveRawData = this->findValueInSettings<double>("SaveRawData", true);
    doLocal     = false;

    // ###########################################
    // # Initialize directory and data container #
    // ###########################################
    ContainerFactory::copyAndInitStructure<Occupancy>(*fDetectorContainer, fOccContainer);

    SSAChannelGroupHandler theChannelGroupHandler;
    theChannelGroupHandler.setChannelGroupParameters(1, 1, NSSACHANNELS); // 16*2*8
    setChannelGroupHandler(theChannelGroupHandler);
}

void SSAPhysics::Running()
{
    LOG(INFO) << GREEN << "[SSAPhysics::Start] Starting" << RESET;

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

    SSAPhysics::run();
}

void SSAPhysics::Stop()
{
    LOG(INFO) << GREEN << "[SSAPhysics::Stop] Stopping" << RESET;

    Tool::Stop();

    // ################
    // # Error report #
    // ################
    SSAPhysics::chipErrorReport();

    this->closeFileHandler();
}

void SSAPhysics::initialize(const std::string fileRes_, const std::string fileReg_)
{
    fileRes = fileRes_;
    fileReg = fileReg_;

    SSAPhysics::ConfigureCalibration();

#ifdef __USE_ROOT__
    myApp = nullptr;

    if(doDisplay == true) myApp = new TApplication("myApp", nullptr, nullptr);

    this->InitResultFile(fileRes);

    SSAPhysics::initHisto();
#endif

    doLocal = true;
}

void SSAPhysics::run()
{
    unsigned int totalDataSize = 0;

    while(fKeepRunning)
    {
        for(const auto cBoard: *fDetectorContainer)
        {
            unsigned int dataSize = SystemController::ReadData(static_cast<BeBoard*>(cBoard), false);
            if(dataSize != 0) { SSAPhysics::fillDataContainer(cBoard); }
            totalDataSize += dataSize;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }

    LOG(WARNING) << BOLDBLUE << "Number of collected events = " << totalDataSize << RESET;

    if(totalDataSize == 0) LOG(WARNING) << BOLDBLUE << "No data collected" << RESET;

    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theContainerSerialization("SSAPhysicsOccupancy");
        theContainerSerialization.streamByHybridContainer(fDQMStreamer, fOccContainer);
    }
}

void SSAPhysics::draw()
{
#ifdef __USE_ROOT__
    SSAPhysics::fillHisto();
    SSAPhysics::display();

    if(doDisplay == true) myApp->Run(true);
    this->WriteRootFile();
    this->CloseResultFile();
#endif
}

void SSAPhysics::initHisto()
{
#ifdef __USE_ROOT__
    histos.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void SSAPhysics::fillHisto()
{
#ifdef __USE_ROOT__
    histos.fillOccupancy(fOccContainer);
#endif
}

void SSAPhysics::display()
{
#ifdef __USE_ROOT__
    histos.process();
#endif
}

void SSAPhysics::fillDataContainer(BoardContainer* const& cBoard)
{
    // ###################
    // # Clear container #
    // ###################
    for(const auto cOpticalGroup: *fOccContainer.getObject(cBoard->getId()))
        for(const auto cHybrid: *cOpticalGroup)
            for(const auto cChip: *cHybrid)
                for(auto& channel: *cChip->getChannelContainer<Occupancy>())
                {
                    channel.fOccupancy      = 0;
                    channel.fOccupancyError = 0;
                }

    // ###################
    // # Fill containers #
    // ###################
    const std::vector<Event*>& events = SystemController::GetEvents();
    // Assuming all chip will have all channels enabled:
    auto allChannelGroup = getChannelGroup(-1);

    for(const auto& event: events) { event->fillDataContainer(fOccContainer.getObject(cBoard->getId()), allChannelGroup); }
}

void SSAPhysics::chipErrorReport() {}
