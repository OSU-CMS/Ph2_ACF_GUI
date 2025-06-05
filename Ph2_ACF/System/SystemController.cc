/*!
  \file                  SystemController.cc
  \brief                 Controller of the System, overall wrapper of the framework
  \author                Mauro DINARDO
  \version               2.0
  \date                  01/01/20
  Support:               email to mauro.dinardo@cern.ch
*/

#include "System/SystemController.h"
#include "HWDescription/VTRx.h"
#include "HWInterface/BeBoardFWInterface.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/ExceptionHandler.h"
#include "HWInterface/LinkInterface.h"
#include "HWInterface/RD53AInterface.h"
#include "HWInterface/RD53BInterface.h"
#include "HWInterface/RD53FWInterface.h"
#include "HWInterface/VTRxInterface.h"
#include "MonitorUtils/DetectorMonitor.h"
#include "MonitorUtils/Monitor2S.h"
#include "MonitorUtils/PSMonitor.h"
#include "MonitorUtils/RD53Monitor.h"
#include "MonitorUtils/SEHMonitor.h"
#include "Parser/CommunicationSettingConfig.h"
#include "Parser/DetectorMonitorConfig.h"
#include "Parser/ParserDefinitions.h"
#include "System/RegisterHelper.h"
#include "Utils/ConfigureInfo.h"
#include "Utils/StartInfo.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_Parser;

namespace Ph2_System
{
SystemController::SystemController()
    : fBeBoardInterface(nullptr)
    , fReadoutChipInterface(nullptr)
    , flpGBTInterface(nullptr)
    , fVTRxInterface(nullptr)
    , fCicInterface(nullptr)
    , fDetectorContainer(nullptr)
    , fSettingsMap()
    , fFileHandler(nullptr)
    , fRawFileName("")
    , fWriteHandlerEnabled(false)
    , fDQMStreamerEnabled(false)
    , fMonitorDQMStreamerEnabled(false)
    , fDQMStreamer(nullptr)
    , fMonitorDQMStreamer(nullptr)
    , fDetectorMonitor(nullptr)
    , fChannelGroupHandlerContainer(nullptr)
    , fNameContainer(nullptr)
{
}

SystemController::~SystemController() {}

void SystemController::Inherit(const SystemController* pController)
{
    fBeBoardInterface     = pController->fBeBoardInterface;
    fReadoutChipInterface = pController->fReadoutChipInterface;
    flpGBTInterface       = pController->flpGBTInterface;
    fVTRxInterface        = pController->fVTRxInterface;
    fBeBoardFWMap         = pController->fBeBoardFWMap;
    fSettingsMap          = pController->fSettingsMap;
    fFileHandler          = pController->fFileHandler;
    fRawFileName          = pController->fRawFileName;
    fParsedFile.clear();
    fParsedFile.str("");
    fParsedFile << pController->fParsedFile.rdbuf();
    fWriteHandlerEnabled          = pController->fWriteHandlerEnabled;
    fDetectorMonitor              = pController->fDetectorMonitor;
    fDQMStreamerEnabled           = pController->fDQMStreamerEnabled;
    fMonitorDQMStreamerEnabled    = pController->fMonitorDQMStreamerEnabled;
    fDQMStreamer                  = pController->fDQMStreamer;
    fMonitorDQMStreamer           = pController->fMonitorDQMStreamer;
    fDetectorContainer            = pController->fDetectorContainer;
    fCicInterface                 = pController->fCicInterface;
    fPowerSupplyClient            = pController->fPowerSupplyClient;
    fChannelGroupHandlerContainer = pController->fChannelGroupHandlerContainer;
    fEventList                    = pController->fEventList;
    // fFuture                       = pController->fFuture;
    fEventSize                       = pController->fEventSize;
    fNCbc                            = pController->fNCbc;
    fParser                          = pController->fParser;
    fSameChannelGroupForAllChannels  = pController->fSameChannelGroupForAllChannels;
    fInitializeInterfaces            = pController->fInitializeInterfaces;
    fNameContainer                   = pController->fNameContainer;
    fBoardType                       = pController->fBoardType;
    fConfigurationFileName           = pController->fConfigurationFileName;
    fSettingsFileName                = pController->fSettingsFileName;
    fCalibrationName                 = pController->fCalibrationName;
    fInitialConfigurationFileContent = pController->fInitialConfigurationFileContent;
    fRegisterHelper                  = pController->fRegisterHelper;
    fCommunicationSettingConfig      = pController->fCommunicationSettingConfig;
    fDetectorMonitorConfig           = pController->fDetectorMonitorConfig;
}

void SystemController::StopMonitoring()
{
    if(fDetectorMonitor != nullptr) { fDetectorMonitor->stopMonitoring(); }
}

std::string SystemController::GetMonitorFileName()
{
    if(fDetectorMonitor != nullptr) { return fDetectorMonitor->getMonitorFileName(); }
    else { return ""; }
}

void SystemController::Destroy()
{
    this->closeFileHandler();

    LOG(INFO) << BOLDRED << ">>> Destroying interfaces <<<" << RESET;

    RD53Event::JoinDecodingThreads();

    if(fDetectorMonitor != nullptr)
    {
        fDetectorMonitor->stopRunning();
        fDetectorMonitor->waitForMonitorToStop();
    }
    delete fDetectorMonitor;
    fDetectorMonitor = nullptr;

    delete fBeBoardInterface;
    fBeBoardInterface = nullptr;

    delete fReadoutChipInterface;
    fReadoutChipInterface = nullptr;

    delete flpGBTInterface;
    flpGBTInterface = nullptr;

    delete fVTRxInterface;
    fVTRxInterface = nullptr;

    delete fDetectorContainer;
    fDetectorContainer = nullptr;

    delete fCicInterface;
    fCicInterface = nullptr;

    fBeBoardFWMap.clear();
    fSettingsMap.clear();

    LOG(INFO) << GREEN << "Trying to shutdown Calibration DQM Server..." << RESET;
    delete fDQMStreamer;
    fDQMStreamer = nullptr;
    LOG(INFO) << BOLDBLUE << "\t--> Operation completed" << RESET;

    LOG(INFO) << GREEN << "Trying to shutdown Monitor DQM Server..." << RESET;
    delete fMonitorDQMStreamer;
    fMonitorDQMStreamer = nullptr;
    LOG(INFO) << BOLDBLUE << "\t--> Operation completed" << RESET;

    delete fPowerSupplyClient;
    fPowerSupplyClient = nullptr;

    delete fChannelGroupHandlerContainer;
    fChannelGroupHandlerContainer = nullptr;

    delete fNameContainer;
    fNameContainer = nullptr;

    delete fRegisterHelper;
    fRegisterHelper = nullptr;

    delete fCommunicationSettingConfig;
    fCommunicationSettingConfig = nullptr;

    delete fDetectorMonitorConfig;
    fDetectorMonitorConfig = nullptr;

    LOG(INFO) << BOLDRED << ">>> Interfaces  destroyed <<<" << RESET;
}

void SystemController::addFileHandler(const std::string& pFilename, char pOption)
{
    if(pOption == 'r')
        fFileHandler = new FileHandler(pFilename, pOption);
    else if(pOption == 'w')
    {
        fRawFileName         = pFilename;
        fWriteHandlerEnabled = true;
    }
}

void SystemController::closeFileHandler()
{
    if(fFileHandler != nullptr)
    {
        if(fFileHandler->isFileOpen() == true) fFileHandler->closeFile();
        delete fFileHandler;
        fFileHandler = nullptr;
    }
}

void SystemController::readFile(std::vector<uint32_t>& pVec, uint32_t pNWords32)
{
    if(pNWords32 == 0)
        pVec = fFileHandler->readFile();
    else
        pVec = fFileHandler->readFileChunks(pNWords32);
}

void SystemController::InitializeHw(const std::string& pFilename, std::ostream& os)
{
    if(fCommunicationSettingConfig != nullptr) throw std::runtime_error("Error: SystemController::InitializeHw was already called once, this should never happen");
    fCommunicationSettingConfig = new CommunicationSettingConfig();
    this->fParser.parseCommunicationSettings(pFilename, *fCommunicationSettingConfig, os);

    fDQMStreamerEnabled = fCommunicationSettingConfig->fDQMCommunication.fEnable;
    if(fDQMStreamerEnabled && (fDQMStreamer == nullptr))
    {
        fDQMStreamer = new TCPPublishServer(fCommunicationSettingConfig->fDQMCommunication.fPort, 1);
        fDQMStreamer->startAccept();
    }

    LOG(INFO) << GREEN << "Bootstrapping TCP Server..." << RESET;

    fMonitorDQMStreamerEnabled = fCommunicationSettingConfig->fMonitorDQMCommunication.fEnable;
    if(fMonitorDQMStreamerEnabled && (fMonitorDQMStreamer == nullptr))
    {
        fMonitorDQMStreamer = new TCPPublishServer(fCommunicationSettingConfig->fMonitorDQMCommunication.fPort, 1);
        fMonitorDQMStreamer->startAccept();
    }

    fDetectorContainer = new DetectorContainer;
    this->fParser.parseHW(pFilename, fDetectorContainer, os);

    for(const auto theBoard: *fDetectorContainer)
    {
        std::string cId           = theBoard->getConnectionId();
        std::string cUri          = theBoard->getConnectionUri();
        std::string cAddressTable = theBoard->getAddressTable();
        if(theBoard->getBoardType() == BoardType::D19C)
            fBeBoardFWMap[theBoard->getId()] = new D19cFWInterface(cId, cUri, cAddressTable, theBoard);
        else if(theBoard->getBoardType() == BoardType::RD53)
            fBeBoardFWMap[theBoard->getId()] = new RD53FWInterface(cId, cUri, cAddressTable, theBoard);
    }

    fBeBoardInterface = new BeBoardInterface(fBeBoardFWMap);

    LOG(INFO) << GREEN << "Trying to connect to the Power Supply Server..." << RESET;

    if(fCommunicationSettingConfig->fPowerSupplyDQMCommunication.fEnable == true)
    {
        fPowerSupplyClient = new TCPClient(fCommunicationSettingConfig->fPowerSupplyDQMCommunication.fIP, fCommunicationSettingConfig->fPowerSupplyDQMCommunication.fPort);
        if(!fPowerSupplyClient->connect(1))
        {
            LOG(INFO) << GREEN << "Cannot connect to the Power Supply Server, power supplies will need to be controlled manually" << RESET;
            delete fPowerSupplyClient;
            fPowerSupplyClient = nullptr;
        }
        else { LOG(INFO) << GREEN << "Connected to the Power Supply Server!" << RESET; }
    }

    LOG(INFO) << BOLDBLUE << "\t--> Operation completed" << RESET;

    if((fDetectorContainer->size() > 0) && (fInitializeInterfaces == 1))
    {
        const BeBoard* cFirstBoard = fDetectorContainer->getFirstObject();
        fBoardType                 = cFirstBoard->getBoardType();
        if(fBoardType == BoardType::D19C)
        {
            LOG(INFO) << BOLDBLUE << "Initializing HwInterfaces for OT BeBoards.." << RESET;
            if(cFirstBoard->size() > 0) // # Of optical groups connected to Board0
            {
                auto cFirstOpticalGroup = cFirstBoard->getFirstObject();
                LOG(INFO) << BOLDBLUE << "\t...Initializing HwInterfaces for OpticalGroups.." << +cFirstBoard->size() << " optical group(s) found ..." << RESET;
                bool cWithLpGBT = (cFirstOpticalGroup->flpGBT != nullptr);
                if(cWithLpGBT)
                {
                    LOG(INFO) << BOLDBLUE << "\t\t\t.. Initializing HwInterface for lpGBT" << RESET;
                    flpGBTInterface = new D19clpGBTInterface(fBeBoardFWMap, cFirstOpticalGroup->flpGBT->isOptical());
                }
                bool cWithVTRx = (cFirstOpticalGroup->fVTRx != nullptr);
                if(cWithVTRx)
                {
                    LOG(INFO) << BOLDBLUE << "\t\t\t.. Initializing HwInterface for VTRx" << RESET;
                    fVTRxInterface = new VTRxInterface(fBeBoardFWMap, flpGBTInterface);
                }

                LOG(INFO) << BOLDBLUE << "Found " << +cFirstOpticalGroup->size() << " hybrids in this group..." << RESET;
                if(cFirstOpticalGroup->size() > 0) // # Of hybrids connected to OpticalGroup0
                {
                    LOG(INFO) << BOLDBLUE << "\t\t...Initializing HwInterfaces for FrontEnd Hybrids.." << +cFirstOpticalGroup->size() << " hybrid(s) found ..." << RESET;
                    auto cFirstHybrid = cFirstOpticalGroup->getFirstObject();
                    auto cType        = FrontEndType::CBC3;
                    bool cWithCBC  = (std::find_if(cFirstHybrid->begin(), cFirstHybrid->end(), [&cType](Ph2_HwDescription::Chip* x) { return x->getFrontEndType() == cType; }) != cFirstHybrid->end());
                    cType          = FrontEndType::SSA2;
                    bool cWithSSA2 = (std::find_if(cFirstHybrid->begin(), cFirstHybrid->end(), [&cType](Ph2_HwDescription::Chip* x) { return x->getFrontEndType() == cType; }) != cFirstHybrid->end());
                    cType          = FrontEndType::MPA2;
                    bool cWithMPA2 = (std::find_if(cFirstHybrid->begin(), cFirstHybrid->end(), [&cType](Ph2_HwDescription::Chip* x) { return x->getFrontEndType() == cType; }) != cFirstHybrid->end());
                    if(cWithCBC)
                    {
                        LOG(INFO) << BOLDBLUE << "\t\t\t\t.. Initializing HwInterface(s) for CBC(s)" << RESET;
                        fReadoutChipInterface = new CbcInterface(fBeBoardFWMap);
                    }
                    if((cWithMPA2 || cWithSSA2) && cWithLpGBT)
                    {
                        LOG(INFO) << BOLDBLUE << "\t\t\t\t.. Initializing HwInterface(s) for PS module(s)" << RESET;
                        fReadoutChipInterface = new PSInterface(fBeBoardFWMap);
                    }
                    if(fReadoutChipInterface != nullptr)
                    {
                        bool cFoundLpgbt = fReadoutChipInterface->lpGBTCheck(cFirstBoard);
                        if(cFoundLpgbt) LOG(INFO) << BOLDGREEN << "\t\t\t\t\t.. Readout chip interface aware of the lpGBT connected to this board ... " << RESET;
                    }
                } // create Chip interfaces

                LOG(INFO) << BOLDBLUE << "\t\t\t.. Initializing HwInterface for CIC" << RESET;
                fCicInterface = new CicInterface(fBeBoardFWMap);
                if(cFirstOpticalGroup->flpGBT != nullptr)
                {
                    bool cFoundLpgbt = fCicInterface->lpGBTCheck(cFirstBoard);
                    if(cFoundLpgbt) LOG(INFO) << BOLDGREEN << "\t\t\t\t\t.. CIC interface aware of the lpGBT connected to this board ... " << RESET;
                }
                else
                    fCicInterface->setWithLpGBT(false);
            }
        }
        else if(fBoardType == BoardType::RD53)
        {
            // ################################
            // # Instantiate proper inerfaces #
            // ################################
            flpGBTInterface = new RD53lpGBTInterface(fBeBoardFWMap);
            if(cFirstBoard->getFrontEndType() == FrontEndType::RD53A)
                fReadoutChipInterface = new RD53AInterface(fBeBoardFWMap);
            else
                fReadoutChipInterface = new RD53BInterface(fBeBoardFWMap);
            RD53Shared::setFirstChip(*fDetectorContainer);
            RD53Shared::setChipInterface(*fReadoutChipInterface);
        }
        else
            throw Exception("[SystemController::InitializeHw] Error, board type not recognized");
    }

    if(fWriteHandlerEnabled == true) this->initializeWriteFileHandler();

    // ###################
    // # Set module type #
    // ###################
    fDetectorMonitorConfig = new DetectorMonitorConfig();
    fParser.parseMonitor(pFilename, *fDetectorMonitorConfig, os);

    if(fDetectorMonitorConfig->fEnable == true)
    {
        if(fDetectorMonitorConfig->fMonitoringType == MONITORING_NODE_TYPE_ATTRIBUTE_2S_VALUE)
            fDetectorMonitor = new Monitor2S(this, *fDetectorMonitorConfig);
        else if((fDetectorMonitorConfig->fMonitoringType == MONITORING_NODE_TYPE_ATTRIBUTE_RD53A_VALUE) || (fDetectorMonitorConfig->fMonitoringType == MONITORING_NODE_TYPE_ATTRIBUTE_RD53B_VALUE))
            fDetectorMonitor = new RD53Monitor(this, *fDetectorMonitorConfig);
        else if(fDetectorMonitorConfig->fMonitoringType == MONITORING_NODE_TYPE_ATTRIBUTE_2SSEH_VALUE)
            fDetectorMonitor = new SEHMonitor(this, *fDetectorMonitorConfig);
        else if(fDetectorMonitorConfig->fMonitoringType == MONITORING_NODE_TYPE_ATTRIBUTE_PS_VALUE)
            fDetectorMonitor = new PSMonitor(this, *fDetectorMonitorConfig);
        else
        {
            LOG(ERROR) << BOLDRED << "Unrecognized monitor type. Aborting" << RESET;
            exit(EXIT_FAILURE);
        }

        fDetectorMonitor->forkMonitor();
    }

    for(const auto cBoard: *fDetectorContainer)
    {
        if(cBoard->getBoardType() == BoardType::RD53) continue;

        // ###########################################
        // # Make sure all interfaces are configured #
        // ###########################################
        static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard))->ConfigureInterfaces(cBoard);

        for(auto cOpticalGroup: *cBoard)
        {
            bool cWithLpGBT = (cOpticalGroup->flpGBT != nullptr);

            if(cOpticalGroup->getFrontEndType() == FrontEndType::OuterTrackerPS)
                static_cast<D19clpGBTInterface*>(flpGBTInterface)->AddPSROHeLinkProperties(cOpticalGroup->flpGBT);
            else if(cOpticalGroup->getFrontEndType() == FrontEndType::OuterTracker2S)
                static_cast<D19clpGBTInterface*>(flpGBTInterface)->Add2SSEHeLinkProperties(cOpticalGroup->flpGBT);
            else if(cWithLpGBT && flpGBTInterface != nullptr)
                static_cast<D19clpGBTInterface*>(flpGBTInterface)->setFrontEndType(cOpticalGroup->getFrontEndType());
            else
                LOG(INFO) << BOLDMAGENTA << "UN-KNOWN MODULE TYPE" << RESET;
        }
    }

    fRegisterHelper = new RegisterHelper(fDetectorContainer, fBeBoardInterface, fReadoutChipInterface, fVTRxInterface, flpGBTInterface, fCicInterface, &fBeBoardFWMap);
}

void SystemController::InitializeSettings(const std::string& pFilename, std::ostream& os) { this->fParser.parseSettings(pFilename, fSettingsMap, os); }

void SystemController::ReadSystemMonitor(BeBoard* pBoard, const std::vector<std::string>& args, bool silentRunning) const
{
    if(args.size() != 0)
        for(const auto cOpticalGroup: *pBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(silentRunning == false)
                        LOG(INFO) << GREEN << "Monitor data for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << pBoard->getId() << "/" << cOpticalGroup->getId() << "/" << cHybrid->getId() << "/"
                                  << +cChip->getId() << RESET << GREEN << "]" << RESET;
                    fBeBoardInterface->ReadHybridVoltageMonitor(fReadoutChipInterface, cChip, silentRunning);
                    fBeBoardInterface->ReadHybridTemperatureMonitor(fReadoutChipInterface, cChip, silentRunning);
                }
}

// ######################################
// # Configuring Inner Tracker hardware #
// ######################################
void SystemController::ConfigureIT(BeBoard* pBoard)
{
    auto& theBeBoardFW = this->fBeBoardFWMap[pBoard->getId()];

    // #################
    // # Configure FSM #
    // #################
    const size_t nTRIGxEvent = SystemController::findValueInSettings<double>("nTRIGxEvent", 1);
    const auto   injType     = static_cast<RD53Shared::INJtype>(SystemController::findValueInSettings<double>("INJtype", 1));
    const size_t injLatency  = SystemController::findValueInSettings<double>("InjLatency", 32);
    const size_t nClkDelays  = SystemController::findValueInSettings<double>("nClkDelays", 1000);
    const size_t colStart    = SystemController::findValueInSettings<double>("COLstart", 0);
    static_cast<RD53FWInterface*>(theBeBoardFW)->ConfigureFastCommands(pBoard, nTRIGxEvent, injType, injLatency, nClkDelays, RD53Shared::firstChip->getFEtype(colStart, colStart) == &RD53A::SYNC);

    // ###############
    // # Program FSM #
    // ###############
    LOG(INFO) << CYAN << "=== Configuring FSM fast command block ===" << RESET;
    static_cast<RD53FWInterface*>(theBeBoardFW)->SendFastCommands();
    static_cast<RD53FWInterface*>(theBeBoardFW)->PrintFWstatus();
    LOG(INFO) << CYAN << "================== Done ==================" << RESET;

    // ######################
    // # Configure from XML #
    // ######################
    static_cast<RD53FWInterface*>(theBeBoardFW)->ConfigureFromXML(pBoard);

    // ########################
    // # Configure LpGBT chip #
    // ########################
    for(auto cOpticalGroup: *pBoard)
    {
        if(cOpticalGroup->flpGBT != nullptr)
        {
            LOG(INFO) << CYAN << "=== Initializing communication to Low-power Gigabit Transceiver (LpGBT): " << BOLDYELLOW << +cOpticalGroup->getId() << RESET << CYAN << " ===" << RESET;
            static_cast<RD53lpGBTInterface*>(flpGBTInterface)->SetDownLinkMapping(cOpticalGroup);
            static_cast<RD53lpGBTInterface*>(flpGBTInterface)->SetUpLinkMapping(cOpticalGroup);
            LOG(INFO) << BOLDBLUE << "\t--> Configured up and down link mapping in firmware" << RESET;

            if(flpGBTInterface->ConfigureChip(cOpticalGroup->flpGBT) == true)
            {
                static_cast<RD53lpGBTInterface*>(flpGBTInterface)->PhaseAlignRx(cOpticalGroup->flpGBT, pBoard, cOpticalGroup, fReadoutChipInterface);
                LOG(INFO) << CYAN << "=== LpGBT chip " << BOLDYELLOW << +cOpticalGroup->getId() << RESET << CYAN << " configured ===" << RESET;
            }
            else
                LOG(ERROR) << BOLDRED << "=== LpGBT chip " << BOLDYELLOW << +cOpticalGroup->getId() << BOLDRED << " not configured, reached maximum number of attempts (" << BOLDYELLOW
                           << +RD53Shared::MAXATTEMPTS << BOLDRED << ") ===" << RESET;
        }
    }

    // #######################
    // # Status optical link #
    // #######################
    uint32_t txStatus, rxStatus, mgtStatus;
    LOG(INFO) << GREEN << "Checking status of the optical links:" << RESET;
    static_cast<RD53FWInterface*>(theBeBoardFW)->StatusOptoLink(txStatus, rxStatus, mgtStatus);

    // ##########################################
    // # Configure down-links to frontend chips #
    // ##########################################
    LOG(INFO) << CYAN << "=== Configuring frontend chip communication ===" << RESET;
    LOG(INFO) << GREEN << "Down-link phase initialization..." << RESET;
    static_cast<RD53Interface*>(fReadoutChipInterface)->InitRD53Downlink(pBoard);
    LOG(INFO) << BOLDBLUE << "\t--> Done" << RESET;

    // ##########################################
    // # Configure up-links from frontend chips #
    // ##########################################
    for(auto cOpticalGroup: *pBoard)
        for(auto cHybrid: *cOpticalGroup)
        {
            LOG(INFO) << GREEN << "Initializing chip communication of hybrid: " << BOLDYELLOW << +cHybrid->getId() << RESET;
            for(const auto cChip: *cHybrid)
            {
                LOG(INFO) << GREEN << "Initializing communication to/from RD53: " << BOLDYELLOW << +cChip->getId() << RESET;
                LOG(INFO) << GREEN << "Configuring up-link lanes and monitoring..." << RESET;
                static_cast<RD53Interface*>(fReadoutChipInterface)->InitRD53Uplinks(cChip);
                LOG(INFO) << BOLDBLUE << "\t--> Done" << RESET;
            }
        }

    LOG(INFO) << CYAN << "==================== Done =====================" << RESET;

    // ####################################
    // # Check AURORA lock on data stream #
    // ####################################
    const bool stopIfCommFails = SystemController::findValueInSettings<double>("StopIfCommFails", 1);
    if((static_cast<RD53FWInterface*>(theBeBoardFW)->CheckChipCommunication(pBoard) == false) && (stopIfCommFails == true))
    {
        LOG(ERROR) << BOLDRED << "===== Aborting =====" << RESET;
        exit(EXIT_FAILURE);
    }
}

void SystemController::ConfigureFrontendIT(BeBoard* pBoard)
{
    // ############################
    // # Configuration parameters #
    // ############################
    bool resetMask = SystemController::findValueInSettings<double>("ResetMask");
    int  resetTDAC = SystemController::findValueInSettings<double>("ResetTDAC");

    // ############################
    // # Configure frontend chips #
    // ############################
    LOG(INFO) << CYAN << "===== Configuring frontend chip registers =====" << RESET;
    for(auto cOpticalGroup: *pBoard)
        for(auto cHybrid: *cOpticalGroup)
        {
            LOG(INFO) << GREEN << "Configuring chips of hybrid: " << BOLDYELLOW << +cHybrid->getId() << RESET;
            bool   eFuseCodeCheck = true;
            double eFuseCode;

            for(const auto cChip: *cHybrid)
            {
                LOG(INFO) << GREEN << "Configuring RD53: " << BOLDYELLOW << +cChip->getId() << RESET;

                if(resetMask == true) static_cast<RD53*>(cChip)->enableAllPixels();
                if(resetTDAC >= 0) static_cast<RD53*>(cChip)->resetTDAC(resetTDAC);
                static_cast<RD53*>(cChip)->copyMaskToDefault();
                static_cast<RD53Interface*>(fReadoutChipInterface)->ConfigureChip(cChip);

                try
                {
                    eFuseCode = fReadoutChipInterface->ReadChipFuseID(cChip);
                }
                catch(const std::system_error& err)
                {
                    LOG(WARNING) << RED << err.what() << RESET;
                    eFuseCode      = -1;
                    eFuseCodeCheck = false;
                }
                catch(const std::runtime_error& err)
                {
                    LOG(WARNING) << RED << err.what() << RESET;
                    eFuseCode      = -1;
                    eFuseCodeCheck = false;
                }
                catch(const std::out_of_range& err)
                {
                    LOG(DEBUG) << GREEN << "Chip e-fuse code: " << BOLDYELLOW << err.what() << RESET;
                    eFuseCode = atoi(err.what());
                }

                LOG(INFO) << BOLDBLUE << "\t--> Done" << RESET;
                if(eFuseCode >= 0) LOG(INFO) << GREEN << "e-fuse code: " << BOLDYELLOW << static_cast<uint32_t>(eFuseCode) << RESET;
                LOG(INFO) << GREEN << "Number of masked pixels: " << BOLDYELLOW << static_cast<RD53*>(cChip)->getNbMaskedPixels() << RESET;
            }

            if(eFuseCodeCheck == false) throw std::runtime_error("Please set the proper e-fuse code(s) in the xml file");

            LOG(INFO) << GREEN << "Optimizing up-link slave-chip phases (if any and if needed) for hybrid: " << BOLDYELLOW << +cHybrid->getId() << RESET;
            static_cast<RD53Interface*>(fReadoutChipInterface)->TAP0slaveOptimization(pBoard, cHybrid);
            LOG(INFO) << BOLDBLUE << "\t--> Done" << RESET;
        }

    LOG(INFO) << CYAN << "==================== Done =====================" << RESET;
}

// ######################################
// # Configuring Outer Tracker hardware #
// ######################################
void SystemController::InitializeOT(BeBoard* pBoard)
{
    LOG(INFO) << BOLDMAGENTA << "Initializing OT hardware.." << RESET;
    LOG(INFO) << BOLDBLUE << "Now going to configuring lpGBTs#" << RESET;

    for(auto cOpticalGroup: *pBoard)
    {
        LOG(INFO) << BOLDBLUE << "Now going to configuring lpGBTs#" << +cOpticalGroup->getId() << " on Board " << int(pBoard->getId()) << RESET;
        D19clpGBTInterface* clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
        if(cOpticalGroup->getReset() == 0)
        {
            LOG(INFO) << BOLDYELLOW << "Will not re-configure lpGBT on Link#" << +cOpticalGroup->getId() << RESET;
            continue;
        }

        if(!clpGBTInterface->ConfigureChip(cOpticalGroup->flpGBT))
        {
            LOG(INFO) << BOLDRED << "SOMETHING FUNNY" << RESET;
            continue;
        }
        if(cOpticalGroup->fVTRx != nullptr) fVTRxInterface->ConfigureChip(cOpticalGroup->fVTRx);
    }

    // module start-up
    // depends on module type
    for(auto cOpticalGroup: *pBoard)
    {
        if(cOpticalGroup->getReset() == 0)
        {
            LOG(INFO) << BOLDYELLOW << "Will not re-configure lpGBT for specific module type.." << RESET;
            continue;
        }

        if(cOpticalGroup->getFrontEndType() == FrontEndType::OuterTracker2S)
        {
            LOG(INFO) << BOLDMAGENTA << "Configuring an OuterTracker2S module " << RESET;
            ModuleStartUp2S(cOpticalGroup);
        }
        if(cOpticalGroup->getFrontEndType() == FrontEndType::OuterTrackerPS)
        {
            LOG(INFO) << BOLDMAGENTA << "Configuring an OuterTrackerPS module " << RESET;
            ModuleStartUpPS(cOpticalGroup);
        }
    }

    // CIC reset
    for(auto cOpticalGroup: *pBoard)
    {
        auto& clpGBT = cOpticalGroup->flpGBT;
        if(clpGBT == nullptr) continue;

        for(auto cHybrid: *cOpticalGroup)
        {
            uint8_t cSide = cHybrid->getId() % 2;
            static_cast<D19clpGBTInterface*>(flpGBTInterface)->resetCIC(clpGBT, cSide);
        }
    }

    // CIC start-up
    for(auto cOpticalGroup: *pBoard)
    {
        // CIC configuration part .. first configure
        for(auto cHybrid: *cOpticalGroup)
        {
            auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
            if(cCic == NULL) continue;

            LOG(INFO) << BOLDBLUE << "Configuring CIC" << +(cHybrid->getId() % 2) << " on link " << +cHybrid->getOpticalGroupId() << " on hybrid " << +cHybrid->getId() << RESET;
            if(!fCicInterface->ConfigureChip(cCic))
            {
                LOG(INFO) << BOLDRED << "FAILED to configure CIC on Board Id " << +pBoard->getId() << " OpticalGroup id" << +cOpticalGroup->getId() << " Hybrid id " << +cHybrid->getId()
                          << " --- Hybrid will be disabled" << RESET;
                ExceptionHandler::getInstance()->disableHybrid(pBoard->getId(), cOpticalGroup->getId(), cHybrid->getId());
                continue;
            }
        }

        if(!CicStartUp(cOpticalGroup, true))
        {
            LOG(INFO) << BOLDRED << "Failed CIC start-up sequence on Board Id " << +pBoard->getId() << " OpticalGroup id" << +cOpticalGroup->getId()
                      << " for all its hybrids --- OpticalGroup will be disabled" << RESET;
            ExceptionHandler::getInstance()->disableOpticalGroup(pBoard->getId(), cOpticalGroup->getId());
            continue;
        }
    }

    // check if a resync is needed
    LOG(INFO) << BOLDBLUE << "Checking if a ReSync is needed for Board" << +pBoard->getId() << RESET;
    bool cReSyncNeeded = false;
    for(auto cOpticalGroup: *pBoard)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
            if(cCic == NULL) continue;
            cReSyncNeeded = fCicInterface->GetResyncRequest(cCic);
            if(cReSyncNeeded)
            {
                LOG(INFO) << BOLDBLUE << "\t... CIC" << +cHybrid->getId() << " requires a ReSync" << RESET;
                break;
            }
        }
        if(cReSyncNeeded) break;
    }

    if(cReSyncNeeded)
    {
        LOG(INFO) << BOLDMAGENTA << "Sending a ReSync at the end of the OT-module configuration step" << RESET;
        // send a ReSync to all chips before starting
        fBeBoardInterface->ChipReSync(pBoard);
        // check resync request has been cleared
        for(auto cOpticalGroup: *pBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
                if(cCic == NULL) continue;

                if(fCicInterface->GetResyncRequest(cCic))
                {
                    LOG(INFO) << BOLDYELLOW << "FAILED to clear CIC ReSync request on Board Id " << +pBoard->getId() << " OpticalGroup id" << +cOpticalGroup->getId() << " Hybrid id "
                              << +cHybrid->getId() << " --- trying to change fast command sampling edge" << RESET;

                    // Change the sampling edge of the fast command and then resync again
                    uint8_t cCicEdge = fCicInterface->ReadFCMDEdge(cCic);
                    cCicEdge         = (cCicEdge == 0) ? 1 : 0;
                    fCicInterface->ConfigureFCMDEdge(cCic, cCicEdge);
                    fBeBoardInterface->ChipReSync(pBoard);

                    if(fCicInterface->GetResyncRequest(cCic))
                    {
                        LOG(INFO) << BOLDRED << "FAILED to clear CIC ReSync request on Board Id " << +pBoard->getId() << " OpticalGroup id" << +cOpticalGroup->getId() << " Hybrid id "
                                  << +cHybrid->getId() << " --- Hybrid will be disabled" << RESET;
                        ExceptionHandler::getInstance()->disableHybrid(pBoard->getId(), cOpticalGroup->getId(), cHybrid->getId());
                        continue;
                    }
                }
            }
        }
    }
    else
        LOG(INFO) << BOLDMAGENTA << "No ReSync needed after OT-module configuration step" << RESET;
}

void SystemController::ConfigureOT(BeBoard* pBoard)
{
    // Set board sparisification
    // based on what is configured in the fw register
    // read CIC sparsification setting from fW register
    // make sure board is also set to the same thing
    pBoard->setSparsification(fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.physical_interface_block.cic.2s_sparsified_enable") == 1);
    InitializeOT(pBoard);

    // Hard reset Chips on hybrid if lpGBT is there; if no lpGBT this
    // is already taken care of by ConfigureBoard
    for(auto cOpticalGroup: *pBoard)
    {
        auto& clpGBT = cOpticalGroup->flpGBT;
        for(auto cHybrid: *cOpticalGroup)
        {
            if(clpGBT != nullptr)
            {
                uint8_t cSide = cHybrid->getId() % 2;
                LOG(INFO) << BOLDBLUE << "Configuring ReadoutOutChips on Hybrid" << +cHybrid->getId() << RESET;

                if(cHybrid->getReset() == 0) { LOG(INFO) << BOLDYELLOW << "Will not send a hard-reset to Chips on Hybrid#" << +cHybrid->getId() << RESET; }
                else
                {
                    // no SSA because I don't want to reset it here. . already done earlier
                    if(cOpticalGroup->getFrontEndType() == FrontEndType::OuterTrackerPS)
                    {
                        LOG(DEBUG) << BOLDBLUE << "\t... Applying hard reset to MPAs" << RESET;
                        static_cast<D19clpGBTInterface*>(flpGBTInterface)->resetMPA(clpGBT, cSide);
                    }
                    if(cOpticalGroup->getFrontEndType() == FrontEndType::OuterTracker2S)
                    {
                        LOG(DEBUG) << BOLDBLUE << "\t... Applying hard reset to CBCs" << RESET;
                        static_cast<D19clpGBTInterface*>(flpGBTInterface)->resetCBC(clpGBT, cSide);
                    }
                }
            }

            for(auto cChip: *cHybrid) { fReadoutChipInterface->ConfigureChip(cChip); } // Chip config
        } // hybrid
    } // OG
    LOG(INFO) << BOLDMAGENTA << "Configured OT module" << RESET;
}

void SystemController::ModuleStartUpPS(const OpticalGroup* pOpticalGroup)
{
    auto cBoardId = pOpticalGroup->getBeBoardId();
    LOG(INFO) << BOLDBLUE << "SystemController::ModuleStartUpPS for BeBoard#" << +cBoardId << " OpticalGroup#" << +pOpticalGroup->getId() << RESET;

    auto& clpGBT = pOpticalGroup->flpGBT;
    // configure PS ROHs
    if(clpGBT != nullptr)
    {
        auto theD19clpGBTInterface = static_cast<D19clpGBTInterface*>(flpGBTInterface);
        theD19clpGBTInterface->holdPSModuleResets(clpGBT);
        theD19clpGBTInterface->updateCICinputClockToMatchPSrate(clpGBT);

        for(auto cHybrid: *pOpticalGroup)
        {
            uint8_t cSide = cHybrid->getId() % 2;
            LOG(INFO) << BOLDBLUE << "Resetting SSA" << RESET;
            theD19clpGBTInterface->resetSSA(clpGBT, cSide);
        } // hybrid
    }
}

void SystemController::ModuleStartUp2S(const OpticalGroup* pOpticalGroup)
{
    auto cBoardId = pOpticalGroup->getBeBoardId();
    LOG(INFO) << BOLDBLUE << "SystemController::ModuleStartUp2S for BeBoard#" << +cBoardId << " OpticalGroup#" << +pOpticalGroup->getId() << RESET;

    // reset I2C and old chip resets
    auto& clpGBT = pOpticalGroup->flpGBT;
    if(clpGBT != nullptr) { static_cast<D19clpGBTInterface*>(flpGBTInterface)->hold2SModuleResets(clpGBT); } // lpGBT part ... resets + clocks
}

bool SystemController::CicStartUp(const OpticalGroup* pOpticalGroup, bool cStartUpSequence)
{
    auto cBoardId        = pOpticalGroup->getBeBoardId();
    auto cOpticalGroupId = pOpticalGroup->getId();
    bool cIs2S           = (pOpticalGroup->getFrontEndType() == FrontEndType::OuterTracker2S);

    auto exceptionHandleFunction = [cBoardId, cOpticalGroupId, this](uint16_t hybridId, const std::string&& failMode)
    {
        LOG(INFO) << BOLDRED << "FAILED to " << failMode << " for Board Id " << +cBoardId << " OpticalGroup id " << +cOpticalGroupId << " Hybrid id " << +hybridId << " --- Disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(cBoardId, cOpticalGroupId, hybridId);
    };

    auto& clpGBT   = pOpticalGroup->flpGBT;
    bool  cSuccess = false;
    LOG(INFO) << BOLDGREEN << "####################################################################################" << RESET;
    for(auto cHybrid: *pOpticalGroup)
    {
        auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
        if(cCic == NULL) continue;

        LOG(INFO) << BOLDMAGENTA << "SystemController::CicStartUp for OpticalGroup#" << +pOpticalGroup->getId() << " CIC#" << +cCic->getHybridId() << RESET;

        std::stringstream enabledFEsPrintout;
        enabledFEsPrintout << "Overriding FE_ENABLE for CIC on Hybrid " << +cHybrid->getId() << " OpticalGroup " << +cOpticalGroupId << " BeBoard " << +cBoardId << " to enable ";
        if(cIs2S)
            enabledFEsPrintout << " CBCs ";
        else
            enabledFEsPrintout << " MPAs ";
        std::vector<uint8_t> cChipIds(0);
        for(auto cReadoutChip: *cHybrid)
        {
            // Only consider MPAs and CBCs
            if(cReadoutChip->getFrontEndType() == FrontEndType::SSA2) continue;
            cChipIds.push_back(cReadoutChip->getId() % 8);
            enabledFEsPrintout << +cReadoutChip->getId() << " ";
        }
        if(!fCicInterface->configureEnabledFEs(cCic, cChipIds))
        {
            exceptionHandleFunction(cHybrid->getId(), "configure CIC Termination");
            continue;
        }
        LOG(INFO) << BOLDMAGENTA << enabledFEsPrintout.str() << RESET;

        // Make sure the CIC data rate matches the LpGBT one
        if(clpGBT != nullptr && cCic->getFrontEndType() == FrontEndType::CIC2)
        {
            auto     cChipRate                    = static_cast<D19clpGBTInterface*>(flpGBTInterface)->GetChipRate(clpGBT);
            uint16_t cClkFrequency                = (cChipRate == 5) ? 320 : 640;
            uint8_t  cFeConfigReg                 = fCicInterface->ReadChipReg(cCic, "FE_CONFIG");
            bool     is640clockBitEnabled         = ((cFeConfigReg >> 1) & 0x1) == 1;
            bool     is640clockBitNeedToBeEnabled = (cClkFrequency == 640);
            if(is640clockBitEnabled != is640clockBitNeedToBeEnabled)
            {
                uint8_t cNewValue = (cFeConfigReg & 0xFD) | ((is640clockBitNeedToBeEnabled ? 1 : 0) << 1);
                fCicInterface->WriteChipReg(cCic, "FE_CONFIG", cNewValue);
                LOG(INFO) << BOLDMAGENTA << "Overriding FE_CONFIG to 0x" << std::hex << +cNewValue << std::dec << " for CIC on Hybrid " << +cHybrid->getId() << " OpticalGroup " << +cOpticalGroupId
                          << " BeBoard " << +cBoardId << " to run with " << cClkFrequency << " MHz clock to match LpGBT configuration" << RESET;
            }
        }

        if(cStartUpSequence)
        {
            LOG(INFO) << BOLDYELLOW << "Launching CIC start-up sequence.." << RESET;
            fCicInterface->StartUp(cCic);
        }
        else
            LOG(INFO) << BOLDYELLOW << "Not launching CIC start-up sequence..." << RESET;

        cSuccess = true; // At least one hybrid is working fine
    } // All hybrids connected to this OG
#ifdef __TCUSB__
    cSuccess = true; // No hybrids in the SEH/ROH test system
#endif
    LOG(INFO) << BOLDGREEN << "####################################################################################" << RESET;
    return cSuccess;
}

void SystemController::ConfigureHw(bool pReInitialize)
{
    if(fDetectorContainer == nullptr)
    {
        LOG(ERROR) << BOLDRED << "Hardware not initialized: run SystemController::InitializeHw first" << RESET;
        exit(EXIT_FAILURE);
    }

    LOG(INFO) << BOLDMAGENTA << "@@@ Configuring HW parsed from XML file @@@" << RESET;
    for(const auto cBoard: *fDetectorContainer)
    {
        std::cout << std::endl;

        cBoard->printBoardType();
        if(cBoard->getToConfigure()) fBeBoardInterface->ConfigureBoard(cBoard);
    }

    for(const auto cBoard: *fDetectorContainer)
    {
        std::cout << std::endl;

        LOG(INFO) << BOLDMAGENTA << "@@@ Configuring frontend for Board Id: " << BOLDYELLOW << cBoard->getId() << BOLDMAGENTA << " @@@" << RESET;

        // #################
        // # Outer Tracker #
        // #################
        if(cBoard->getBoardType() == BoardType::D19C) ConfigureOT(cBoard);

        // #################
        // # Inner Tracker #
        // #################
        else if(cBoard->getBoardType() == BoardType::RD53)
        {
            if(pReInitialize == true)
            {
                // #################################
                // # Initialize board and frontend #
                // #################################
                ConfigureIT(cBoard);
                ConfigureFrontendIT(cBoard);
            }
            else
            {
                // ###################
                // # Configuring FSM #
                // ###################
                const size_t nTRIGxEvent = SystemController::findValueInSettings<double>("nTRIGxEvent", 1);
                const auto   injType     = static_cast<RD53Shared::INJtype>(SystemController::findValueInSettings<double>("INJtype", 1));
                const size_t injLatency  = SystemController::findValueInSettings<double>("InjLatency", 32);
                const size_t nClkDelays  = SystemController::findValueInSettings<double>("nClkDelays", 1000);
                const size_t colStart    = SystemController::findValueInSettings<double>("COLstart", 0);
                static_cast<RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])
                    ->ConfigureFastCommands(cBoard, nTRIGxEvent, injType, injLatency, nClkDelays, RD53Shared::firstChip->getFEtype(colStart, colStart) == &RD53A::SYNC);
            }

            // ######################################
            // # Dispatch threads for data decoding #
            // ######################################
            LOG(INFO) << GREEN << "Using " << BOLDYELLOW << RD53Shared::NTHREADS << RESET << GREEN << " threads for data decoding at runtime" << RESET;
            RD53Event::ForkDecodingThreads();
        }
    }

    // ####################
    // # Start monitoring #
    // ####################
    if(fDetectorMonitor != nullptr)
    {
        LOG(INFO) << GREEN << "Starting " << BOLDYELLOW << "monitoring" << RESET << GREEN << " thread" << RESET;
        fDetectorMonitor->startMonitoring();
    }
}

void SystemController::initializeWriteFileHandler()
{
    for(const auto cBoard: *fDetectorContainer)
    {
        uint32_t cNChip        = 0;
        uint32_t cBeId         = cBoard->getId();
        uint32_t cNEventSize32 = this->computeEventSize32(cBoard);

        std::string cBoardTypeString;
        BoardType   cBoardType = cBoard->getBoardType();

        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup) cNChip += cHybrid->size();

        if(cBoardType == BoardType::D19C)
            cBoardTypeString = "D19C";
        else if(cBoardType == BoardType::RD53)
            cBoardTypeString = "RD53";

        uint32_t cFWWord  = fBeBoardInterface->getBoardInfo(cBoard);
        uint32_t cFWMajor = (cFWWord & 0xFFFF0000) >> 16;
        uint32_t cFWMinor = (cFWWord & 0x0000FFFF);

        FileHeader cHeader(cBoardTypeString, cFWMajor, cFWMinor, cBeId, cNChip, cNEventSize32, cBoard->getEventType());

        std::stringstream cBeBoardString;
        cBeBoardString << "_Board" << std::setw(3) << std::setfill('0') << cBeId;
        std::string cFilename = fRawFileName;
        if(fRawFileName.find(".raw") != std::string::npos) cFilename.insert(fRawFileName.find(".raw"), cBeBoardString.str());

        fFileHandler = new FileHandler(cFilename, 'w', cHeader);

        fBeBoardInterface->SetFileHandler(cBoard, fFileHandler);
        LOG(INFO) << GREEN << "Saving binary data into: " << BOLDYELLOW << cFilename << RESET;
    }
}

uint32_t SystemController::computeEventSize32(const BeBoard* pBoard)
{
    uint32_t cNEventSize32 = 0;
    uint32_t cNChip        = 0;

    for(const auto cOpticalGroup: *pBoard)
        for(const auto cHybrid: *cOpticalGroup) cNChip += cHybrid->size();

    if(pBoard->getBoardType() == BoardType::D19C) cNEventSize32 = D19C_EVENT_HEADER1_SIZE_32_CBC3 + cNChip * D19C_EVENT_SIZE_32_CBC3;

    return cNEventSize32;
}

void SystemController::Configure(const ConfigureInfo& theConfigureInfo, bool pReInitialize)
{
    fConfigurationFileName = theConfigureInfo.getConfigurationFile();
    fSettingsFileName      = theConfigureInfo.getSettingsFile();
    fCalibrationName       = theConfigureInfo.getCalibrationName();

    // #######################################
    // # Save raw configuration file content #
    // #######################################
    fInitialConfigurationFileContent = theConfigureInfo.getConfigFileStream(fConfigurationFileName);
    if(fConfigurationFileName != fSettingsFileName) fInitialConfigurationFileContent += theConfigureInfo.getConfigFileStream(fSettingsFileName);

    // ##################
    // # Initialization #
    // ##################
    InitializeHw(fConfigurationFileName, fParsedFile);
    InitializeSettings(fSettingsFileName, fParsedFile);
    theConfigureInfo.setEnabledObjects(fDetectorContainer);

    fNameContainer = new DetectorDataContainer();
    ContainerFactory::copyAndInitStructure<EmptyContainer, std::string, std::string, std::string, std::string, EmptyContainer>(*fDetectorContainer, *fNameContainer);
    theConfigureInfo.extractObjectNames(fNameContainer);

    initializeExceptionHandler();

    // ########################################################
    // # Formatted printout on screen of the xml file content #
    // ########################################################
    std::cout << fParsedFile.str() << std::endl;

    if(findValueInSettings<double>("SkipConfigureHW", 0) == 0) ConfigureHw(pReInitialize);
}

void SystemController::initializeExceptionHandler()
{
    ExceptionHandler::getInstance()->setDetectorContainer(fDetectorContainer);
    ExceptionHandler::getInstance()->setBeBoardInterface(fBeBoardInterface);
}

void SystemController::Start(const StartInfo& theStartInfo)
{
    for(auto cBoard: *fDetectorContainer) fBeBoardInterface->Start(cBoard);
}

void SystemController::Stop()
{
    for(auto cBoard: *fDetectorContainer) fBeBoardInterface->Stop(cBoard);
}

void SystemController::Pause()
{
    for(auto cBoard: *fDetectorContainer) fBeBoardInterface->Pause(cBoard);
}

void SystemController::Resume()
{
    for(auto cBoard: *fDetectorContainer) fBeBoardInterface->Resume(cBoard);
}

void SystemController::StartBoard(BeBoard* pBoard) { fBeBoardInterface->Start(pBoard); }
void SystemController::StopBoard(BeBoard* pBoard) { fBeBoardInterface->Stop(pBoard); }

void SystemController::Abort() { LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << " Abort not implemented" << RESET; }

uint32_t SystemController::ReadData(BeBoard* pBoard, bool pWait)
{
    std::vector<uint32_t> cData;
    return this->ReadData(pBoard, cData, pWait);
}

void SystemController::ReadData(bool pWait)
{
    for(auto cBoard: *fDetectorContainer) this->ReadData(cBoard, pWait);
}

uint32_t SystemController::ReadData(BeBoard* pBoard, std::vector<uint32_t>& pData, bool pWait)
{
    uint32_t cNPackets = fBeBoardInterface->ReadData(pBoard, false, pData, pWait);
    if(cNPackets == 0) return cNPackets;

    this->DecodeData(pBoard, pData, cNPackets, fBeBoardInterface->getBoardType(pBoard));
    return cNPackets;
}

void SystemController::ReadNEvents(BeBoard* pBoard, uint32_t pNEvents)
{
    std::vector<uint32_t> cData;
    this->ReadNEvents(pBoard, pNEvents, cData, true);
}

void SystemController::ReadNEvents(uint32_t pNEvents)
{
    for(auto cBoard: *fDetectorContainer) this->ReadNEvents(cBoard, pNEvents);
}

void SystemController::ReadNEvents(BeBoard* pBoard, uint32_t pNEvents, std::vector<uint32_t>& pData, bool pWait)
{
    fBeBoardInterface->ReadNEvents(pBoard, pNEvents, pData, pWait);
    if(fBeBoardInterface->getBoardType(pBoard) == BoardType::D19C) pNEvents = pNEvents * (fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity") + 1);
    this->DecodeData(pBoard, pData, pNEvents, fBeBoardInterface->getBoardType(pBoard));
}

// #################
// # Data decoding #
// #################

void SystemController::SetFuture(const BeBoard* pBoard, const std::vector<uint32_t>& pData, uint32_t pNevents, BoardType pType)
{
    if(pData.size() != 0) fFuture = std::async(&SystemController::DecodeData, this, pBoard, pData, pNevents, pType);
}

void SystemController::DecodeData(const BeBoard* pBoard, const std::vector<uint32_t>& pData, uint32_t pNevents, BoardType pType)
{
    // ####################
    // # Decoding IT data #
    // ####################
    if(pType == BoardType::RD53)
    {
        uint32_t status;
        fEventList.clear();
        if(RD53Event::decodedEvents.size() == 0) RD53Event::DecodeEventsMultiThreads(pData, RD53Event::decodedEvents, status);
        RD53Event::addBoardInfo2Events(pBoard, RD53Event::decodedEvents);
        for(auto& evt: RD53Event::decodedEvents) fEventList.push_back(&evt);
    }
    // ####################
    // # Decoding OT data #
    // ####################
    else if(pType == BoardType::D19C && pBoard->getEventType() != EventType::PSAS)
    {
        if(pData.size() == 0) { throw std::runtime_error("SystemController::DecodeData -> data vector is empty"); }
        bool cTLUconfig = 2;
        // bool cTLUconfig = (fBeBoardInterface->ReadBoardReg(fDetectorContainer->getObject(pBoard->getId()), "fc7_daq_cnfg.tlu_block.handshake_mode") == 2 &&
        //                    fBeBoardInterface->ReadBoardReg(fDetectorContainer->getObject(pBoard->getId()), "fc7_daq_cnfg.tlu_block.tlu_enabled") == 1);
        // for (auto L : pData) LOG(INFO) << BOLDBLUE << std::bitset<32>(L) << RESET;
        for(auto& pevt: fEventList) delete pevt;
        fEventList.clear();

        EventType fEventType = pBoard->getEventType();
        // uint32_t  cBlockSize = 0x0000FFFF & pData.at(0);
        // LOG(INFO) << BOLDBLUE << "Reading events from " << +fNHybrid << " FEs connected to uDTC...[ " << +cBlockSize * 4 << " 32 bit words to decode]" << RESET;
        fEventSize = static_cast<uint32_t>((pData.size()) / pNevents);
        // uint32_t nmpa = 0;
        uint32_t maxind = 0;
        for(auto opticalGroup: *pBoard)
            for(auto hybrid: *opticalGroup) maxind = std::max(maxind, uint32_t(hybrid->size()));

        if(fEventType != EventType::ZS)
        {
            // for( auto cWord : pData )
            //     LOG (INFO) << BOLDYELLOW << "SystemController \t..." << std::bitset<32>(cWord) << RESET;

            size_t cEventIndex    = 0;
            auto   cEventIterator = pData.begin();
            do {
                uint32_t cHeader = (0xFFFF0000 & (*cEventIterator)) >> 16;
                if(cHeader != 0xFFFF)
                {
                    int cPositionInData = (int)std::distance(pData.begin(), cEventIterator);
                    // first part of data header
                    if(fFileHandler != nullptr && cPositionInData > 12)
                        LOG(INFO) << BOLDRED << "SystemController::DecodeData Invalid header from the FW in position#" << cPositionInData << RESET;
                    else if(fFileHandler == nullptr)
                        LOG(INFO) << BOLDRED << "SystemController::DecodeData Invalid header from the FW in position#" << cPositionInData << RESET;
                    cEventIterator++;
                }
                else // valid event  // decode
                {
                    uint32_t cEventSize = (0x0000FFFF & (*cEventIterator)) * 4; // event size is given in 128 bit words
                    // LOG (INFO) << BOLDMAGENTA << "SystemController::DecodeData Decoding event made of up " << +cEventSize << " 32 bit words. " << RESET;
                    auto cEnd = ((cEventIterator + cEventSize) > pData.end()) ? pData.end() : (cEventIterator + cEventSize);
                    // retrieve chunck of data vector belonging to this event
                    if(cEnd - cEventIterator == cEventSize)
                    {
                        std::vector<uint32_t> cEvent(cEventIterator, cEnd);
                        if(pBoard->getFrontEndType() == FrontEndType::CIC2) { fEventList.push_back(new D19cCic2Event(pBoard, cEvent, cTLUconfig)); }
                        cEventIndex++;
                    }
                    cEventIterator += cEventSize;
                }
            } while(cEventIterator < pData.end());
        }
    }
    else if(pType == BoardType::D19C && pBoard->getEventType() == EventType::PSAS)
    {
        fEventList.clear();
        fEventList.push_back(new D19cPSEventAS(pBoard, pData));
    }
}

void SystemController::setChannelGroupHandler(ChannelGroupHandler& theChannelGroupHandler, std::vector<FrontEndType> cFrontEndTypes)
{
    auto selectChipFlavourFunction = [cFrontEndTypes](const ChipContainer* theChip)
    { return (std::find(cFrontEndTypes.begin(), cFrontEndTypes.end(), static_cast<const ReadoutChip*>(theChip)->getFrontEndType()) != cFrontEndTypes.end()); };
    setChannelGroupHandler(theChannelGroupHandler, selectChipFlavourFunction);
}

void SystemController::setChannelGroupHandler(ChannelGroupHandler& theChannelGroupHandler, FrontEndType theFrontEndType)
{
    auto selectChipFlavourFunction = [theFrontEndType](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == theFrontEndType); };
    setChannelGroupHandler(theChannelGroupHandler, selectChipFlavourFunction);
}

void SystemController::setChannelGroupHandler(ChannelGroupHandler& theChannelGroupHandler, std::function<bool(const ChipContainer*)> theQueryFunction)
{
    auto theChannelGroupHandlerPointer = std::make_shared<ChannelGroupHandler>(std::move(theChannelGroupHandler));
    setChannelGroupHandler(theChannelGroupHandlerPointer, theQueryFunction);
}

void SystemController::setChannelGroupHandler(std::shared_ptr<ChannelGroupHandler> theChannelGroupHandlerPointer, std::function<bool(const ChipContainer*)> theQueryFunction)
{
    uint16_t totalNumberOfChips        = 0;
    uint16_t totalNumberOfQueriedChips = 0;
    for(const auto board: *fDetectorContainer)
    {
        for(const auto opticalGroup: *board)
        {
            for(const auto hybrid: *opticalGroup)
            {
                totalNumberOfChips += hybrid->size();
                for(const auto chip: *hybrid)
                {
                    if(theQueryFunction(chip)) totalNumberOfQueriedChips++;
                }
            }
        }
    }
    fSameChannelGroupForAllChannels = (totalNumberOfQueriedChips == totalNumberOfChips);

    if(fChannelGroupHandlerContainer == nullptr)
    {
        fChannelGroupHandlerContainer = new DetectorDataContainer();
        ContainerFactory::copyAndInitChip<std::shared_ptr<ChannelGroupHandler>>(*fDetectorContainer, *fChannelGroupHandlerContainer);
    }
    for(const auto board: *fDetectorContainer)
    {
        for(const auto opticalGroup: *board)
        {
            for(const auto hybrid: *opticalGroup)
            {
                for(const auto chip: *hybrid)
                {
                    if(!theQueryFunction(chip)) continue;

                    fChannelGroupHandlerContainer->getObject(board->getId())
                        ->getObject(opticalGroup->getId())
                        ->getObject(hybrid->getId())
                        ->getObject(chip->getId())
                        ->getSummary<std::shared_ptr<ChannelGroupHandler>>() = theChannelGroupHandlerPointer;
                }
            }
        }
    }
}

void SystemController::setChannelGroupHandler(std::shared_ptr<ChannelGroupHandler> theChannelGroupHandlerPointer, uint16_t boardId, uint16_t opticalGroupId, uint16_t hybridId, uint16_t chipId)
{
    fChannelGroupHandlerContainer->getObject(boardId)->getObject(opticalGroupId)->getObject(hybridId)->getObject(chipId)->getSummary<std::shared_ptr<ChannelGroupHandler>>() =
        theChannelGroupHandlerPointer;
}

void SystemController::disableAllChannels()
{
    // ###########################################
    // # Disable channels of the entire detector #
    // ###########################################
    if(SystemController::findValueInSettings<double>("DisableChannelsAtExit", false) == true)
        for(const auto cBoard: *fDetectorContainer)
            for(const auto cOpticalGroup: *cBoard)
                for(const auto cHybrid: *cOpticalGroup)
                    for(const auto cChip: *cHybrid) fReadoutChipInterface->MaskAllChannels(cChip, true);
}

void SystemController::DumpRegisters()
{
    // #################################################
    // # Dump firmware register content for all boards #
    // #################################################
    for(const auto cBoard: *fDetectorContainer)
    {
        LOG(INFO) << GREEN << "Firmware register content for [board = " << BOLDYELLOW << cBoard->getId() << GREEN << "]" << RESET;

        const auto theBeBoardFW = this->fBeBoardFWMap[cBoard->getId()];
        const auto hwInterface  = theBeBoardFW->getHardwareInterface();
        theBeBoardFW->PrintFWstatus();

        for(const auto& path: hwInterface->getNodes("user.+"))
        {
            const auto& node = hwInterface->getNode(path);

            if((node.getMode() == uhal::defs::BlockReadWriteMode::SINGLE) && ((int)node.getPermission() & true) && (++node.begin() == node.end()))
            {
                const auto value = fBeBoardInterface->ReadBoardReg(cBoard, path, false);
                std::cout << "\t--> Register " << std::left << std::setfill(' ') << std::setw(56) << path << " = " << std::setw(8) << std::dec << value << std::hex << "(0x" << value << ")"
                          << std::endl;
            }
        }
    }

    // ##################################################
    // # Dump frontend registers of the entire detector #
    // ##################################################
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    LOG(INFO) << GREEN << "Readout chip register content for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/"
                              << cHybrid->getId() << "/" << +cChip->getId() << RESET << GREEN << "]" << RESET;
                    fReadoutChipInterface->DumpChipRegisters(cChip);
                }

    // ######################################################
    // # Dump OpticalGroup registers of the entire detector #
    // ######################################################
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            if(cOpticalGroup->flpGBT != nullptr)
            {
                LOG(INFO) << GREEN << "OpticalGroup chip register content for [board/opticalGroup = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << RESET << GREEN << "]"
                          << RESET;
                flpGBTInterface->DumpChipRegisters(cOpticalGroup->flpGBT);
            }
}

} // namespace Ph2_System
