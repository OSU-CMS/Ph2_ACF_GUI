#include "miniDAQ/MiddlewareStateMachine.h"
#include "HWInterface/FC7FpgaConfig.h"
#include "Parser/FileParser.h"
#include "Utils/ConfigureInfo.h"
#include "Utils/StartInfo.h"
#include "tools/Tool.h"

using namespace Ph2_Parser;
using namespace Ph2_HwInterface;

MiddlewareStateMachine::MiddlewareStateMachine() {}

MiddlewareStateMachine::~MiddlewareStateMachine()
{
    if(fTheTool != nullptr)
    {
        delete fTheTool;
        fTheTool = nullptr;
    }
}

void MiddlewareStateMachine::initialize()
{
    LOG(INFO) << "Initialized" << RESET;
    return;
}

void MiddlewareStateMachine::configure(const ConfigureInfo theConfigureInfo)
{
    fTheTool = fCombinedCalibrationFactory.createCombinedCalibration(theConfigureInfo.getCalibrationName());

    LOG(INFO) << BOLDBLUE << "Tool created" << RESET;

    fTheTool->Configure(theConfigureInfo);

    LOG(INFO) << "Configured" << RESET;

    return;
}

void MiddlewareStateMachine::start(const StartInfo theStartInfo)
{
    currentRun_ = theStartInfo.getRunNumber();
    fTheTool->resetOutputDirectoryName();
    fTheTool->Start(theStartInfo);
    LOG(INFO) << "Run " << currentRun_ << " started" << RESET;
    return;
}

void MiddlewareStateMachine::stop()
{
    fTheTool->Stop();
    LOG(INFO) << "Run " << currentRun_ << " stopped" << RESET;
    return;
}

void MiddlewareStateMachine::halt()
{
    fTheTool->Destroy();
    LOG(INFO) << "Halted" << RESET;
}

void MiddlewareStateMachine::pause()
{
    fTheTool->Pause();
    LOG(INFO) << "Paused" << RESET;
}

void MiddlewareStateMachine::resume()
{
    fTheTool->Resume();
    LOG(INFO) << "Resumed" << RESET;
}

void MiddlewareStateMachine::abort()
{
    fTheTool->dumpConfigFiles(false);
    fTheTool->WriteRootFile();
    fTheTool->Destroy();
    LOG(INFO) << "Aborted" << RESET;
}

MiddlewareStateMachine::Status MiddlewareStateMachine::status()
{
    try
    {
        return fTheTool->GetRunningStatus() ? Status::DONE : Status::RUNNING;
    }
    catch(const std::exception& e)
    {
#ifdef __USE_ROOT__
        LOG(ERROR) << ERROR_FORMAT << "Caught exception " << e.what() << ". Trying to save result directory before crashing" << RESET;
        this->abort();
#endif
        throw e;
    }
}

FC7FpgaConfig MiddlewareStateMachine::getFpgaConfig(const std::string& configurationFile, uint16_t boardId)
{
    FileParser                                                                  theFileParser;
    const std::map<uint16_t, std::tuple<std::string, std::string, std::string>> theRegManagerInfoList = theFileParser.getRegManagerInfoList(configurationFile);

    try
    {
        const auto& theRegManagerInfo = theRegManagerInfoList.at(boardId);
        return FC7FpgaConfig(RegManager(std::get<0>(theRegManagerInfo), std::get<1>(theRegManagerInfo), std::get<2>(theRegManagerInfo), nullptr));
    }
    catch(const std::exception& e)
    {
        std::cerr << __PRETTY_FUNCTION__ << " caught exception " << e.what() << std::endl;
        std::string errorMessage = "Impossible to create FC7FpgaConfig fpr Board with id " + std::to_string(boardId) + " from file " + configurationFile;
        throw std::runtime_error(errorMessage);
    }
}

std::vector<std::string> MiddlewareStateMachine::getFirmwareList(const std::string& configurationFile, uint16_t boardId)
{
    FC7FpgaConfig theFC7FpgaConfig = getFpgaConfig(configurationFile, boardId);
    return theFC7FpgaConfig.getFpgaConfigList();
}

void MiddlewareStateMachine::deleteFirmwareFromSDcard(const std::string& configurationFile, const std::string& firmwareName, uint16_t boardId)
{
    FC7FpgaConfig theFC7FpgaConfig = getFpgaConfig(configurationFile, boardId);
    theFC7FpgaConfig.deleteFpgaConfig(firmwareName);
    LOG(INFO) << "Firmware image: " << firmwareName << " deleted from SD card";
}

void MiddlewareStateMachine::loadFirmwareInFPGA(const std::string& configurationFile, const std::string& firmwareName, uint16_t boardId)
{
    FC7FpgaConfig theFC7FpgaConfig = getFpgaConfig(configurationFile, boardId);
    theFC7FpgaConfig.jumpToFpgaConfig(firmwareName);
    LOG(INFO) << "Firmware image: " << firmwareName << " loaded on FPGA";
}

void MiddlewareStateMachine::uploadFirmwareOnSDcard(const std::string& configurationFile, const std::string& firmwareName, const std::string& firmwareFile, uint16_t boardId)
{
    FC7FpgaConfig theFC7FpgaConfig = getFpgaConfig(configurationFile, boardId);
    theFC7FpgaConfig.flashProm(firmwareName, firmwareFile);
}

void MiddlewareStateMachine::downloadFirmwareFromSDcard(const std::string& configurationFile, const std::string& firmwareName, const std::string& firmwareFile, uint16_t boardId)
{
    FC7FpgaConfig theFC7FpgaConfig = getFpgaConfig(configurationFile, boardId);
    theFC7FpgaConfig.downloadFpgaConfig(firmwareName, firmwareFile);
}
