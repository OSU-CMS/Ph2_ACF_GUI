/*!  *
 * \file Eudaq2Producer.cc
 * \brief Testbeam Producer for EUDAQ2
 * \author Younes OTARID
 * \date 13 / 09 / 21
 * \Support : younes.otarid@cern.ch
 *
 */

#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/L1ReadoutInterface.cc"
#include "Utils/CBCChannelGroupHandler.h"
#include "Utils/ContainerFactory.h"
#include "Utils/Occupancy.h"
#include "Utils/StartInfo.h"
#include "tools/Channel.h"
#include "tools/CicFEAlignment.h"
#include "tools/PSAlignment.h"
#include <boost/algorithm/string.hpp>

#include <fstream>
#include <iostream>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

#ifdef __EUDAQ__
#include "Eudaq2Producer.h"

Eudaq2Producer::Eudaq2Producer(const std::string& name, const std::string& runcontrol) : OTTool(), eudaq::Producer(name, runcontrol), fExitRun(false)
{
    fPh2FileHandler   = nullptr;
    fSLinkFileHandler = nullptr;
    fInitialised      = false;
}

Eudaq2Producer::~Eudaq2Producer()
{
    delete fPh2FileHandler;
    delete fSLinkFileHandler;
}

void Eudaq2Producer::DoInitialise()
{
    // Nothing particular to do here
    LOG(INFO) << BOLDBLUE << "[CMS-OT Producer] Initialising  ..." << RESET;
    auto cEudaqIni = GetInitConfiguration();

    bool cSkipAlignment = (cEudaqIni->Get("SkipAlignment", "true") == "true") ? true : false;
    LOG(INFO) << BOLDYELLOW << "Setting cSkipAlignment to : " << std::boolalpha << cSkipAlignment << RESET;

    // Get Outer Tracker configuration path
    fPathToHWFile = cEudaqIni->Get("HWFile", "./settings/DESY_FullModule.xml");
    LOG(INFO) << BOLDYELLOW << "Loading settings from file : " << fPathToHWFile << RESET;

    // auto        cRunNumber = GetRunNumber();
    // std::string cDirectory = Form("Results/EudaqProducer_Run%d", cRunNumber);

    std::stringstream outp;
    // Outer Tracker hardware configuration
    this->InitializeHw(fPathToHWFile);
    this->InitializeSettings(fPathToHWFile, outp);
    LOG(INFO) << outp.str();
    // this->CreateResultDirectory(cDirectory, false, false);
    // this->InitResultFile("Module");
    // this->AddMetadata();

    // check if PS module it is
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard) { fIsPS &= (cOpticalGroup->getFrontEndType() == FrontEndType::OuterTrackerPS); }
    }

    bool cReInitialize = true;
    bool cReconfigure  = (cEudaqIni->Get("Reconfigure", "false") == "true") ? true : false;
    if(cReconfigure)
    {
        this->ConfigureHw(cReInitialize);

        // map MPA outputs for PS module
        PSAlignment cPSAlignment;
        cPSAlignment.Inherit(this);
        cPSAlignment.Initialise();
        cPSAlignment.MapMPAOutputs();
        cPSAlignment.ConfigureDefaultAlignmentParameters();
        cPSAlignment.Reset();

        LinkAlignmentOT cLinkAlignment;
        cLinkAlignment.Inherit(this);
        try
        {
            StartInfo theStartInfo;
            theStartInfo.setRunNumber(0);
            cLinkAlignment.Start(theStartInfo);
        }
        catch(const std::exception& e)
        {
            LOG(INFO) << BOLDRED << "Could not align link in the BE... stopping here." << RESET;
            exit(0);
        }
        cLinkAlignment.waitForRunToBeCompleted();
        cLinkAlignment.dumpConfigFiles();
        if(!cLinkAlignment.getStatus())
        {
            LOG(INFO) << BOLDRED << "Could not align link in the BE... stopping here." << RESET;
            exit(0);
        }

        // align FEs - CIC
        CicFEAlignment cCicAligner;
        cCicAligner.Inherit(this);
        StartInfo theStartInfo;
        theStartInfo.setRunNumber(0);
        cCicAligner.Start(theStartInfo);
        cCicAligner.waitForRunToBeCompleted();
        cCicAligner.dumpConfigFiles();

        // now align data between SSA-MPA
        if(fIsPS && !cSkipAlignment)
        {
            cPSAlignment.dumpConfigFiles();
            cPSAlignment.Align();
        }

        // Update critical registers to correct value
        // #FIXME some registers like threshold might be overwritten later on (ie: in the DoConfigure function)
    }

    fInitialised = true;
    LOG(INFO) << BOLDGREEN << "[CMS-OT Producer] Initialised" << RESET;
    EUDAQ_INFO("[CMS-OT Producer] SUCESS : Initialised");
}

void Eudaq2Producer::DoConfigure()
{
    LOG(INFO) << "[CMS-OT Producer] Configuring  ..." << RESET;
    // Getting Run Number and EUDAQ configuration file object
    // auto cRunNumber = GetRunNumber();
    auto cEudaqConf = GetConfiguration();

    // get MPA and SSA thresholds
    fThresholdMPA = std::stoi(cEudaqConf->Get("ThresholdMPA", "75"));
    fThresholdSSA = std::stoi(cEudaqConf->Get("ThresholdSSA", "35"));
    // get CBC threshold
    fThresholdCBC      = std::stoi(cEudaqConf->Get("ThresholdCBC", "550"));
    fRelativeThreshold = std::stoi(cEudaqConf->Get("RelativeThreshold", "0")); // 0 will correspond to a threshold at the pedestal

    fHandshakeEnabled          = (cEudaqConf->Get("DataHandshakeEnable", "false") == "true") ? true : false;
    uint8_t cTLUTriggerIdDelay = std::stoi(cEudaqConf->Get("TLUTriggerIdDelay", "2"));

    // Check if Handshake mode is enabled and get trigger multiplicity value
    this->fBeBoardInterface->WriteBoardReg(fDetectorContainer->getFirstObject(), "fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity", std::stoi(cEudaqConf->Get("TriggerMultiplicity", "0")));
    fTriggerMultiplicity = this->fBeBoardInterface->ReadBoardReg(fDetectorContainer->getFirstObject(), "fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity");
    LOG(INFO) << "Trigger Multiplicity : " << +fTriggerMultiplicity << RESET;

    // check if first event needs to be skipped (was necessary at several beam tests at DESY to get correlations)
    fSkipFirstEvent = (cEudaqConf->Get("SkipFirstEvent", "true") == "true") ? true : false;

    fEnableInjection = (cEudaqConf->Get("EnableInjection", "false") == "true") ? true : false;
    if(fIsPS && fEnableInjection)
    {
        uint8_t cPulseAmplitude = std::stoi(cEudaqConf->Get("PulseAmplitude", "120"));
        EnableDigitalInjection(cPulseAmplitude, fThresholdMPA, fThresholdSSA);
    }
    for(auto cBoard: *fDetectorContainer)
    {
        UpdateFromRegMap(cBoard);
        this->fBeBoardInterface->WriteBoardReg(cBoard, "fc7_daq_cnfg.readout_block.packet_nbr", 999);
        this->fBeBoardInterface->WriteBoardReg(cBoard, "fc7_daq_cnfg.readout_block.global.data_handshake_enable", fHandshakeEnabled);
        this->fBeBoardInterface->WriteBoardReg(cBoard, "fc7_daq_cnfg.tlu_block.trigger_id_delay", cTLUTriggerIdDelay);
        LOG(INFO) << "Board : " << +cBoard->getId() << " -- Data Handshake : " << +fHandshakeEnabled << RESET;
        LOG(INFO) << "Board : " << +cBoard->getId() << " -- TLU Trigger Id Delay : " << +cTLUTriggerIdDelay << RESET;

        // send a Resync to this board
        this->fBeBoardInterface->ChipReSync(cBoard);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    fConfigured = true;
    LOG(INFO) << BOLDGREEN << "[CMS-OT Producer] Configured" << RESET;
    EUDAQ_INFO("[CMS-OT Producer] SUCESS : Configured");
}

void Eudaq2Producer::DoStartRun()
{
    LOG(INFO) << "[CMS-OT Producer] Starting Run..." << RESET;

    auto cEudaqConf = GetConfiguration();
    // Save current threshold values per chip to restore them in the stop state
    // Save individual chip thresholds in a data container
    ContainerFactory::copyAndInitChip<uint16_t>(*this->fDetectorContainer, fChipThreshContainer);
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    auto& cRegister =
                        fChipThreshContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>();
                    // Fill chip threshold with current value
                    if(cChip->getFrontEndType() == FrontEndType::CBC3)
                        cRegister = cChip->getReg("VCth2") << 8 | cChip->getReg("VCth1");
                    else if(cChip->getFrontEndType() == FrontEndType::MPA2)
                        cRegister = cChip->getReg("ThDAC0");
                    else if(cChip->getFrontEndType() == FrontEndType::SSA2)
                        cRegister = cChip->getReg("Bias_THDAC");
                }
            }
        }
    }

    // Set thresholds -- two different possibilities implemented
    // Possibility 1: Global threshold value read from config file for all chips
    //                Needs to have either ThresholdMPA or ThresholdCBC to be defined in EUDAQ config file
    if(!cEudaqConf->Get("ThresholdMPA", "").empty() || !cEudaqConf->Get("ThresholdCBC", "").empty())
    {
        LOG(INFO) << BOLDYELLOW << "Using global threshold setting to set common threshold for all chips!" << RESET;
        for(auto cBoard: *fDetectorContainer)
        {
            for(auto cOpticalGroup: *cBoard)
            {
                for(auto cHybrid: *cOpticalGroup)
                {
                    for(auto cChip: *cHybrid)
                    {
                        if(cChip->getFrontEndType() == FrontEndType::CBC3)
                            this->fReadoutChipInterface->WriteChipReg(cChip, "Threshold", fThresholdCBC);
                        else if(cChip->getFrontEndType() == FrontEndType::MPA2)
                            this->fReadoutChipInterface->WriteChipReg(cChip, "Threshold", fThresholdMPA);
                        else if(cChip->getFrontEndType() == FrontEndType::SSA2)
                            this->fReadoutChipInterface->WriteChipReg(cChip, "Threshold", fThresholdSSA);
                    }
                }
            }
        }
    }
    // Possibility 2: Set threshold per chip relative to the measured pedestal
    //                Threshold set per chip is 'chip_threshold = pedestal + relative_threshold' (negative values possible!)
    //                IMPORTANT: please note
    //                  * the register 'threshold' register has to be removed from the Ph2_ACF xml config file for all hybrids
    //                  * the individual chip thresholds need to be configured at their pedestal (e.g. by the PedeNoise class ran previously)
    else if(!cEudaqConf->Get("RelativeThreshold", "").empty())
    {
        LOG(INFO) << BOLDYELLOW << "Using relativ threshold method to set threshold per chip!" << RESET;
        for(auto cBoard: *fDetectorContainer)
        {
            for(auto cOpticalGroup: *cBoard)
            {
                for(auto cHybrid: *cOpticalGroup)
                {
                    for(auto cChip: *cHybrid)
                    {
                        auto& cRegister =
                            fChipThreshContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>();
                        LOG(INFO) << "Set Threshold on FE" << cHybrid->getId() << " Chip" << cChip->getId() << " to " << int(cRegister) + fRelativeThreshold << RESET;
                        this->fReadoutChipInterface->WriteChipReg(cChip, "Threshold", int(cRegister) + fRelativeThreshold);
                    }
                }
            }
        }
    }

    // Send BORE event at beginning of run with important register information
    eudaq::EventSP cEudaqEvent = eudaq::Event::MakeShared("CMSPhase2RawEvent");
    std::time_t    cTimestamp  = std::time(nullptr);
    cEudaqEvent->SetTimestamp(cTimestamp, cTimestamp);
    cEudaqEvent->SetBORE();
    // Readout the CBCs register data and store them as Tags in a BORE event
    char name[150];
    char name2[150];
    if(!fIsPS)
    {
        LOG(INFO) << "Downloading the register configuration of the CBCs" << RESET;
        for(auto cBoard: *fDetectorContainer)
        {
            for(auto cOpticalGroup: *cBoard)
            {
                for(auto cHybrid: *cOpticalGroup)
                {
                    for(auto cChip: *cHybrid)
                    {
                        int              cCbcId    = int(cChip->getId());
                        uint32_t         cHybridId = cHybrid->getId();
                        auto             cRegMap   = cChip->getRegMap();
                        ThresholdVisitor cThresholdVisitor(fReadoutChipInterface);
                        static_cast<ReadoutChip*>(cChip)->accept(cThresholdVisitor);
                        uint16_t cOriginalThreshold = cThresholdVisitor.getThreshold();
                        std::sprintf(name2, "Threshold_%02d_%02d", int(cHybridId), int(cCbcId));
                        cEudaqEvent->SetTag(name2, (uint32_t)cOriginalThreshold);
                        LOG(INFO) << "Threshold FE" << int(cHybridId) << "CBC" << int(cCbcId) << ": " << cOriginalThreshold << RESET;
                        for(auto& ireg: cRegMap)
                        {
                            std::sprintf(name, "%s_%02d_%02d", ireg.first.c_str(), int(cHybridId), int(cCbcId));
                            cEudaqEvent->SetTag(name, (uint32_t)ireg.second.fValue);
                            LOG(DEBUG) << "Register " << ireg.first.c_str() << "\t" << ireg.second.fValue << RESET;
                        } // end of ireg loop
                    }
                }
            }
        }
    }
    SendEvent(cEudaqEvent);

    // Getting Run Number and EUDAQ configuration file object
    auto cRunNumber = GetRunNumber();

    // Get path to Ph2ACF Raw data and SLink data
    fPathToRawPh2ACF = cEudaqConf->Get("RawDataDirectory", "/tmp/") + "Run_" + std::to_string(cRunNumber) + ".raw";
    LOG(INFO) << BOLDBLUE << "[CMS-OT Producer] Writing Raw Ph2_ACF data to " << fPathToRawPh2ACF << RESET;
    // Prepare Ph2_ACF Raw data and SLink data file header information
    BeBoard* cBoard = static_cast<BeBoard*>(fDetectorContainer->getFirstObject());
    uint32_t cBeId  = cBoard->getId();
    uint32_t cNChip = 0;
    // FIXME this is hard coded now .. should figure out how to calculate this
    uint32_t    cNEventSize32 = 80; // this->computeEventSize32 (cBoard);
    std::string cBoardTypeString;
    BoardType   cBoardType = cBoard->getBoardType();
    // count chips
    for(auto cOpticalGroup: *cBoard)
        for(auto cHybrid: *cOpticalGroup) cNChip += cHybrid->getNChip();
    // check board type
    if(cBoardType == BoardType::D19C)
        cBoardTypeString = "D19C";
    else if(cBoardType != BoardType::RD53)
        cBoardTypeString = "FC7";
    uint32_t   cFWWord  = fBeBoardInterface->getBoardInfo(cBoard);
    uint32_t   cFWMajor = (cFWWord & 0xFFFF0000) >> 16;
    uint32_t   cFWMinor = (cFWWord & 0x0000FFFF);
    FileHeader cHeader(cBoardTypeString, cFWMajor, cFWMinor, cBeId, cNChip, cNEventSize32, cBoard->getEventType());

    // Create Ph2_ACF Raw data and SLink data files handlers
    fPh2FileHandler          = new FileHandler(fPathToRawPh2ACF, 'w', cHeader);
    std::string cSlinkPh2ACF = cEudaqConf->Get("RawDataDirectory", "/tmp/") + "Run_" + std::to_string(cRunNumber) + ".daq";
    LOG(INFO) << BOLDBLUE << "[CMS-OT Producer] Writing s-link data to " << cSlinkPh2ACF << RESET;
    fSLinkFileHandler = new FileHandler(cSlinkPh2ACF, 'w', cHeader);

    // Only keep shutter open if Async Couters are not used,
    // Otherwise look at ReadoutLoop where the shutter is open and closed sequentially to record counters
    // Now start data acquisition by opening shutter
    LOG(INFO) << BOLDBLUE << "[CMS-OT Producer] Opening shutter ..." << RESET;
    for(auto cBoard: *fDetectorContainer)
    {
        auto cInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard));
        cInterface->ResetEventCounter();

        auto cReadoutInterface = cInterface->getL1ReadoutInterface();
        cReadoutInterface->ResetReadout();

        this->fBeBoardInterface->Start(static_cast<BeBoard*>(cBoard));
        LOG(INFO) << BOLDBLUE << "[CMS-OT Producer] Shutter opened on board " << +cBoard->getId() << RESET;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        LOG(INFO) << "[CMS-OT Producer] Run Started, number of triggers received so far: " << +this->fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_stat.fast_command_block.trigger_in_counter");
    }

    // starting readout loop in thread
    fExitRun   = false;
    fThreadRun = std::thread(&Eudaq2Producer::ReadoutLoop, this);
    LOG(INFO) << "[CMS-OT Producer] Started Run" << RESET;
    EUDAQ_INFO("[CMS-OT Producer] SUCESS : Started Run");
}

void Eudaq2Producer::DoStopRun()
{
    // Finalize data acquisition
    LOG(INFO) << "[CMS-OT Producer] Stopping Run..." << RESET;

    // Set exit run flag and join running data thread
    // No need to reset fConfigured flag. We need to allow for a restart of the run without the full configuration sequence
    fExitRun = true;

    // Close shutter
    LOG(INFO) << BOLDBLUE << "[CMS-OT Producer] Closing shutter..." << RESET;
    for(auto cBoard: *fDetectorContainer)
    {
        auto cInterface        = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard));
        auto cReadoutInterface = cInterface->getL1ReadoutInterface();
        this->fBeBoardInterface->Stop(static_cast<BeBoard*>(cBoard));
        LOG(INFO) << BOLDBLUE << "[CMS-OT Producer] Shutter closed on board " << +cBoard->getId() << RESET;
        cReadoutInterface->ResetReadout();
        LOG(INFO) << BOLDBLUE << "Reset readout on D19cFWInterface" << RESET;
        // Show number of triggers received so far
        LOG(INFO) << "[CMS-OT Producer] Run Stopped, number of triggers received so far on board : " << +cBoard->getId() << " = "
                  << +this->fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_stat.fast_command_block.trigger_in_counter");
    }

    // Close raw data and Slink data files
    if(fThreadRun.joinable()) fThreadRun.join();
    // Check if Ph2ACF Raw data file handler is open
    if(fPh2FileHandler->isFileOpen())
    {
        LOG(INFO) << BOLDBLUE << "[CMS-OT Producer] Closing file handler for .raw " << RESET;
        fPh2FileHandler->closeFile();
    }
    // Check if SLink data file handler is open
    if(fSLinkFileHandler->isFileOpen())
    {
        LOG(INFO) << BOLDBLUE << "[CMS-OT Producer] Closing file handler for .daq " << RESET;
        fSLinkFileHandler->closeFile();
    }

    // Reset chip thresholds to original values
    LOG(INFO) << BOLDYELLOW << "Resetting chip thresholds!" << RESET;
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    auto& cRegister =
                        fChipThreshContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>();
                    LOG(INFO) << "Reset Threshold on FE" << cHybrid->getId() << " Chip" << cChip->getId() << " to " << int(cRegister) << RESET;
                    this->fReadoutChipInterface->WriteChipReg(cChip, "Threshold", int(cRegister));
                }
            }
        }
    }
    LOG(INFO) << "[CMS-OT Producer] Stopped Run" << RESET;
    EUDAQ_INFO("[CMS-OT Producer] SUCESS : Stopped Run");
}

void Eudaq2Producer::DoReset()
{
    LOG(INFO) << "[CMS-OT Producer] Reseting Run..." << RESET;
    if(fInitialised) // Avoid seg fault if Tool object is not yet initialised
    {
        // Just in case close the shutter
        for(auto cBoard: *fDetectorContainer)
        {
            auto cInterface        = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard));
            auto cReadoutInterface = cInterface->getL1ReadoutInterface();

            fBeBoardInterface->Stop(static_cast<BeBoard*>(cBoard));
            cReadoutInterface->ResetReadout();
            LOG(INFO) << BOLDBLUE << "Reset readout on D19cFWInterface" << RESET;
            // Show number of triggers received so far
            LOG(INFO) << "[CMS-OT Producer] Run Stopped, number of triggers received so far on board : " << +cBoard->getId() << " = "
                      << +this->fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_stat.fast_command_block.trigger_in_counter");
        }
    }

    fExitRun = true, fConfigured = false;
    // Stop the running data thread
    if(fThreadRun.joinable()) fThreadRun.join();
    // fThreadRun = std::thread();
    LOG(INFO) << "[CMS-OT Producer] Reset Run..." << RESET;
    EUDAQ_INFO("[CMS-OT Producer] SUCESS : Reset Run");
}

void Eudaq2Producer::DoTerminate()
{
    LOG(INFO) << "[CMS-OT Producer] Terminating Run..." << RESET;
    fExitRun = true, fConfigured = false;
    LOG(INFO) << "[CMS-OT Producer] Terminated Run" << RESET;
    EUDAQ_INFO("[CMS-OT Producer] SUCESS : Terminated Run");
    if(fThreadRun.joinable()) { fThreadRun.join(); }
}

// ReadoutLoop has been modified in order to allow for the acquisition of multiple events by a single trigger signal.
// This way, time walk performance can be evaluated in the analysis
// Multiple Ph2ACF Events are read, converted and stored as EUDAQ SubEvents within in one EUDAQ Event
void Eudaq2Producer::ReadoutLoop()
{
    std::vector<Event*> cPh2Events; // Ph2ACF Event vector to store newly read data and previously remaining one
    while(!fExitRun)
    {
        if(fIsPS && fEnableInjection)
        {
            for(auto cBoard: *fDetectorContainer)
            {
                BeBoard*              cTheBoard = static_cast<BeBoard*>(cBoard);
                std::vector<uint32_t> cRawData(0);
                // Read N Events
                LOG(INFO) << MAGENTA << "Running on injection" << RESET;
                uint32_t cNEvents = 1000;
                this->ReadNEvents(cTheBoard, cNEvents);
                std::this_thread::sleep_for(std::chrono::microseconds(1000));

                std::vector<Event*> cPh2NewEvents = this->GetEvents();
                if(cPh2NewEvents.size() == 0)
                {
                    LOG(INFO) << BOLDRED << "Decoded 0 valid events.. not going to send anything ... " << RESET;
                    continue;
                }
                // Update new data container
                LOG(INFO) << BOLDBLUE << +cPh2NewEvents.size() << " events read back from FC7 with ReadData" << RESET;
                std::move(cPh2NewEvents.begin(), cPh2NewEvents.end(), std::back_inserter(cPh2Events)); // insert new data
                std::time_t cTimestamp = std::time(nullptr);
                while(cPh2Events.size() > fTriggerMultiplicity)
                {
                    auto cEudaqEvent = eudaq::Event::MakeUnique("CMSPhase2RawEvent");
                    cEudaqEvent->SetTimestamp(cTimestamp, cTimestamp);
                    // Add multiple Ph2ACF Events as EUDAQ SubEvents to a EUDAQ Event
                    for(auto cPh2Event = cPh2Events.begin(); cPh2Event < cPh2Events.begin() + fTriggerMultiplicity + 1; cPh2Event++)
                    {
                        // un-comment this to test s-link event writing
                        // SLinkEvent cSLinkEvent = (*cPh2Event)->GetSLinkEvent(cBoard);
                        // std::vector<uint32_t> tmp = cSLinkEvent.getData<uint32_t>();
                        // fSLinkFileHandler->setData(tmp);

                        auto cEudaqSubEvent = eudaq::Event::MakeShared("CMSPhase2RawSubEvent");
                        this->ConvertToSubEvent(cBoard, *cPh2Event, cEudaqSubEvent);
                        cEudaqSubEvent->SetTimestamp(cTimestamp, cTimestamp);
                        cEudaqEvent->AddSubEvent(cEudaqSubEvent);
                    }
                    cPh2Events.erase(cPh2Events.begin(), cPh2Events.begin() + fTriggerMultiplicity + 1);
                    // #FIXME check if you want to keep the lines bellow
                    //  skip first event
                    if(!fSkipFirstEvent) SendEvent(std::move(cEudaqEvent));
                    fSkipFirstEvent = false;
                }
            } // end of cBoard loop
        } // end of if fEnableInjection
        else
        {
            // Check if any data is pending
            LOG(INFO) << MAGENTA << "Running on normal mode" << RESET;
            for(auto cBoard: *fDetectorContainer)
            {
                BeBoard*              cTheBoard = static_cast<BeBoard*>(cBoard);
                std::vector<uint32_t> cRawData(0);
                // Get data
                this->ReadData(cTheBoard, cRawData);
                // If no data, wait and pass
                if(cRawData.size() == 0)
                {
                    LOG(INFO) << BOLDRED << "Read-back 0 words from the DD3 memory using ReadData.. waiting 100 us " << RESET;
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    continue;
                }

                // Check and fill phase 2 raw data
                fPh2FileHandler->setData(cRawData);

                std::vector<Event*> cPh2NewEvents = this->GetEvents();
                if(cPh2NewEvents.size() == 0)
                {
                    LOG(INFO) << BOLDRED << "Decoded 0 valid events.. not going to send anything ... " << RESET;
                    continue;
                }
                // Update new data container
                LOG(INFO) << BOLDBLUE << +cPh2NewEvents.size() << " events read back from FC7 with ReadData" << RESET;
                std::move(cPh2NewEvents.begin(), cPh2NewEvents.end(), std::back_inserter(cPh2Events)); // insert new data
                std::time_t cTimestamp = std::time(nullptr);
                while(cPh2Events.size() > fTriggerMultiplicity)
                {
                    auto cEudaqEvent = eudaq::Event::MakeUnique("CMSPhase2RawEvent");
                    cEudaqEvent->SetTimestamp(cTimestamp, cTimestamp);
                    // Add multiple Ph2ACF Events as EUDAQ SubEvents to a EUDAQ Event
                    for(auto cPh2Event = cPh2Events.begin(); cPh2Event < cPh2Events.begin() + fTriggerMultiplicity + 1; cPh2Event++)
                    {
                        // un-comment this to test s-link event writing
                        // SLinkEvent cSLinkEvent = (*cPh2Event)->GetSLinkEvent(cBoard);
                        // std::vector<uint32_t> tmp = cSLinkEvent.getData<uint32_t>();
                        // fSLinkFileHandler->setData(tmp);

                        auto cEudaqSubEvent = eudaq::Event::MakeShared("CMSPhase2RawSubEvent");
                        this->ConvertToSubEvent(cBoard, *cPh2Event, cEudaqSubEvent);
                        cEudaqSubEvent->SetTimestamp(cTimestamp, cTimestamp);
                        cEudaqEvent->AddSubEvent(cEudaqSubEvent);
                    }
                    cPh2Events.erase(cPh2Events.begin(), cPh2Events.begin() + fTriggerMultiplicity + 1);
                    // #FIXME check if you want to keep the lines bellow
                    //  skip first event
                    if(!fSkipFirstEvent) SendEvent(std::move(cEudaqEvent));
                    fSkipFirstEvent = false;
                }
            } // end of cBoard loop
        } // end of if enable injection
    } // end of !fExitRun loop
    fSkipFirstEvent = true; // Reset skipping of first event
}

void Eudaq2Producer::ConvertToSubEvent(const BeBoard* pBoard, const Event* pPh2Event, eudaq::EventSP pEudaqSubEvent)
{
    pEudaqSubEvent->SetTag("L1_COUNTER_BOARD", pPh2Event->GetEventCount());
    pEudaqSubEvent->SetTag("TDC", pPh2Event->GetTDC());
    pEudaqSubEvent->SetTag("BX_COUNTER", pPh2Event->GetBunch());
    pEudaqSubEvent->SetTriggerN(pPh2Event->GetExternalTriggerId());
    pEudaqSubEvent->SetTag("TLU_TRIGGER_ID", pPh2Event->GetExternalTriggerId());

    if(fIsPS)
    {
        // module map dimenstions
        uint8_t  cMaxNChip      = 8;
        uint8_t  cMaxNHybrid    = 2;
        uint16_t cNPixelColumns = (NMPAROWS * NSSACHANNELS / 16) * cMaxNChip;
        uint16_t cNPixelRows    = NMPAROWS * cMaxNHybrid;
        uint16_t cNStripColumns = NSSACHANNELS * cMaxNChip;
        uint16_t cNStripRows    = cMaxNHybrid;
        // Loop over optical groups
        for(auto cOpticalGroup: *pBoard)
        {
            uint8_t cOpticalGroupId = cOpticalGroup->getId();
            // Assign an Id to each sensor
            uint8_t cPixelSensorId = 2 * cOpticalGroupId;
            uint8_t cStripSensorId = 2 * cOpticalGroupId + 1;
            // Data containers
            uint16_t             cNPixelHit = 0, cNStripHit = 0;
            uint16_t             cPixelDataOffset = 0, cStripDataOffset = 0;
            std::vector<uint8_t> cPixelData, cStripData;
            std::vector<uint8_t> cPixelDataFinal(6), cStripDataFinal(6);
            // Loop over hybrids
            for(auto cHybrid: *cOpticalGroup)
            {
                uint8_t cHybridId = cHybrid->getId();
                // Loop over chips and get clusters
                for(auto cChip: *cHybrid)
                {
                    uint8_t cChipId = cChip->getId();
                    // skip if not MPA. MPA holds cluster information for both pixel and strip
                    if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;
                    // Get pixel clusters
                    auto cPClusters = static_cast<const D19cCic2Event*>(pPh2Event)->GetPixelClusters(cHybridId, cChipId);
                    // Extract pixel hit information
                    // #FIXME not using GetHits for a more readable code
                    for(auto cCluster: cPClusters)
                    {
                        for(uint16_t cHitId = 0; cHitId < cCluster.fWidth; cHitId++)
                        {
                            // extent pixel data container by 6 elements
                            cPixelData.resize(cPixelDataOffset + 6);
                            // LOG(INFO) << BOLDRED << "Pixel Data size : " << +cPixelData.size() << RESET;
                            // tranform pixel hit Address (row) according to hybrid to build pixel map
                            uint16_t cHitPosition = cCluster.fAddress + cHitId + (NMPAROWS * NSSACHANNELS / 16) * (cChipId % 8);
                            uint16_t cHitAddress  = (cHybridId % 2 == 0) ? (960 - cHitPosition) : (cHitPosition - 1);
                            // push pixel hit address (column) in 16bits word
                            cPixelData[cPixelDataOffset + 0] = (cHitAddress >> 0) & 0xFF;
                            cPixelData[cPixelDataOffset + 1] = (cHitAddress >> 8) & 0xFF;
                            // transform pixel hit Zpos (row) according to hybrid to build pixel map
                            uint16_t cHitZpos = (cHybridId % 2 != 0) ? (31 - cCluster.fZpos) : (cCluster.fZpos);
                            // push pixel hit Zpos in 16bits word
                            cPixelData[cPixelDataOffset + 2] = (cHitZpos >> 0) & 0xFF;
                            cPixelData[cPixelDataOffset + 3] = (cHitZpos >> 8) & 0xFF;
                            // push hit ToT (binary readout so always equals to 1)
                            cPixelData[cPixelDataOffset + 4] = 1;
                            cPixelData[cPixelDataOffset + 5] = 0;
                            // shift offset by 6 elements
                            cPixelDataOffset += 6;
                            cNPixelHit++;
                            /*
                                          LOG(INFO) << "Hybrid Id              : " << +cHybridId << RESET;
                                          LOG(INFO) << "Chip Id                : " << +(cChipId%8) << RESET;
                                      LOG(INFO) << "Pixel Initial position : " << +cCluster.fAddress << RESET;
                                      LOG(INFO) << "Pixel Final position   : " << +cHitAddress << RESET;
                                          LOG(INFO) << "Pixel Initial Zpos     : " << +cCluster.fZpos << RESET;
                                          LOG(INFO) << "Pixel Final Zpos       : " << +cHitZpos << RESET;
                                          LOG(INFO) << BOLDYELLOW << "  ----- " << RESET;
                            */
                        } // enf of hit loop
                    } // end of PixelClusterPS loop

                    // Get strip clusters
                    auto cSClusters = static_cast<const D19cCic2Event*>(pPh2Event)->GetStripClusters(cHybridId, cChipId);
                    // Extract strip hit information
                    for(auto cCluster: cSClusters)
                    {
                        for(uint16_t cHitId = 0; cHitId < cCluster.fWidth; cHitId++)
                        {
                            // extent strip data container by 6 elements
                            cStripData.resize(cStripDataOffset + 6);
                            // transform strip cluster Address (column) according to hybrid to build strip map
                            uint16_t cHitPosition = cCluster.fAddress + cHitId + NSSACHANNELS * (cChipId % 8);
                            uint16_t cHitAddress  = (cHybridId % 2 == 0) ? (960 - cHitPosition) : (cHitPosition - 1);
                            // push strip cluster Address in 16bits word
                            cStripData[cStripDataOffset + 0] = (cHitAddress >> 0) & 0xFF;
                            cStripData[cStripDataOffset + 1] = (cHitAddress >> 8) & 0xFF;
                            // transform strip cluster Zpos (row) according to hybrid to build pixel map
                            uint16_t cHitZpos = (cHybridId % 2 != 0) ? 1 : 0;
                            // push strip cluster Zpos in 16bits word
                            cStripData[cStripDataOffset + 2] = (cHitZpos >> 0) & 0xFF;
                            cStripData[cStripDataOffset + 3] = (cHitZpos >> 8) & 0xFF;
                            // push hit ToT (binary readout so always equals to 1)
                            cStripData[cStripDataOffset + 4] = 1;
                            cStripData[cStripDataOffset + 5] = 0;
                            // shift offset by 6 elements
                            cStripDataOffset += 6;
                            cNStripHit++;
                            /*
                                                        LOG(INFO) << "Hybrid Id              : " << +cHybridId << RESET;
                                                        LOG(INFO) << "Chip Id                : " << +(cChipId % 8) << RESET;
                                                        LOG(INFO) << "Pixel Initial position : " << +cCluster.fAddress << RESET;
                                                        LOG(INFO) << "Pixel Final position   : " << +cHitAddress << RESET;
                                                        LOG(INFO) << "Pixel Final Zpos       : " << +cHitZpos << RESET;
                                                        LOG(INFO) << BOLDYELLOW << "  ----- " << RESET;
                            */

                        } // end of hit loop
                    } // end of StripClusterPS loop
                } // end of chip loop
            } // end of hybrid loop
            // Fill final pixel data container
            // push number of pixel rows in 16bits word
            cPixelDataFinal[0] = (cNPixelColumns >> 0) & 0xFF;
            cPixelDataFinal[1] = (cNPixelColumns >> 8) & 0xFF;
            // push number of pixel columns in 16bits word
            cPixelDataFinal[2] = (cNPixelRows >> 0) & 0xFF;
            cPixelDataFinal[3] = (cNPixelRows >> 8) & 0xFF;
            // push number of pixel hits in 16bits word
            cPixelDataFinal[4] = (cNPixelHit >> 0) & 0xFF;
            cPixelDataFinal[5] = (cNPixelHit >> 8) & 0xFF;
            /*
                  LOG(INFO) << "Data size : " << BOLDRED << +cPixelData.size() << RESET;
                  LOG(INFO) << "Data Final size : " << BOLDRED << +cPixelDataFinal.size() << RESET;
                  //push pixel hit data
                  LOG(INFO) << BOLDBLUE << +((cPixelData[1] << 8) | (cPixelData[0] << 0)) << "\n" << RESET;
            */

            cPixelDataFinal.insert(cPixelDataFinal.end(), cPixelData.begin(), cPixelData.end());
            // add EUDAQ sub-event
            pEudaqSubEvent->AddBlock(cPixelSensorId, cPixelDataFinal);
            //
            // Fill final strip data container
            // push number of strip columns in 16bits word
            cStripDataFinal[0] = (cNStripColumns >> 0) & 0xFF;
            cStripDataFinal[1] = (cNStripColumns >> 8) & 0xFF;
            // push number of strip rows in 16bits word
            cStripDataFinal[2] = (cNStripRows >> 0) & 0xFF;
            cStripDataFinal[3] = (cNStripRows >> 8) & 0xFF;
            // push number of pixel hits in 16bits word
            cStripDataFinal[4] = (cNStripHit >> 0) & 0xFF;
            cStripDataFinal[5] = (cNStripHit >> 8) & 0xFF;
            // push strip hit data
            cStripDataFinal.insert(cStripDataFinal.end(), cStripData.begin(), cStripData.end());
            // add EUDAQ sub-event
            pEudaqSubEvent->AddBlock(cStripSensorId, cStripDataFinal);
        } // end of optical group loop
    }
    else
    {
        uint32_t cMaxNChip = 8;
        uint32_t cNColumns = (NCHANNELS / 2) * cMaxNChip;
        // Extract hit information
        for(auto cOpticalGroup: *pBoard)
        {
            uint8_t              cOpticalGroupId = cOpticalGroup->getId();
            uint8_t              cTopSensorId    = 2 * cOpticalGroupId;
            uint8_t              cBottomSensorId = 2 * cOpticalGroupId + 1;
            std::vector<uint8_t> cTopData, cBottomData;
            uint16_t             cTopDataOffset = 0, cBottomDataOffset = 0;
            std::vector<uint8_t> cTopDataFinal(6), cBottomDataFinal(6);
            for(auto cHybrid: *cOpticalGroup)
            {
                uint32_t cHybridId = cHybrid->getId();
                for(auto cChip: *cHybrid)
                {
                    uint32_t cChipId = cChip->getId();
                    // FIXME Adding this check here [sarah]
                    // std::string cCheck = pPh2Event->DataBitString( cCbc->getHybridId() , cCbc->getChipId() );
                    // if( cCheck.empty() )
                    //	continue;

                    // Get chip hits : 254 bit vector
                    const auto cHits = pPh2Event->GetHits(cHybridId, cChipId);
                    fHitsCounter += pPh2Event->GetNHits(cHybridId, cChipId);
                    // EUDAQ reauires blocks of uint8_t vectors
                    // Hit row position, collumn, and ToT will be stored over 2 uint8_t each in the Top/BottomData
                    // Thus an offset of 6 between each hit
                    for(auto cHit: cHits)
                    {
                        if(cHit.second % 2 == 1) // Top sensor : odd channels
                        {
                            // extent strip data container by 6 elements
                            cTopData.resize(cTopDataOffset + 6);
                            // transform hit position (row) according to hybrid to build strip map
                            uint32_t cHitPosition = (cChipId * NCHANNELS / 2) + (cHit.second - 1) / 2;
                            if(cHybridId % 2 == 0) cHitPosition = 1015 - cHitPosition;
                            // push hit columns in 16bits word
                            cTopData[cTopDataOffset + 0] = (cHitPosition >> 0) & 0xFF;
                            cTopData[cTopDataOffset + 1] = (cHitPosition >> 8) & 0xFF;
                            // push hit rows in 16bits word
                            cTopData[cTopDataOffset + 2] = ((1 - (cHybridId % 2)) >> 0) & 0xFF;
                            cTopData[cTopDataOffset + 3] = ((1 - (cHybridId % 2)) >> 8) & 0xFF;
                            // push hit ToT in 16bits word
                            cTopData[cTopDataOffset + 4] = 1;
                            cTopData[cTopDataOffset + 5] = 0;
                            // shift offset by 6 elements
                            cTopDataOffset += 6;
                        }
                        else // Bottom sensor : even channels
                        {
                            // extent strip data container by 6 elements
                            cBottomData.resize(cBottomDataOffset + 6);
                            // transform hit position (row) according to hybrid to build strip map
                            uint32_t cHitPosition = (cChipId * NCHANNELS / 2) + cHit.second / 2;
                            if(cHybridId % 2 == 0) cHitPosition = 1015 - cHitPosition;
                            // push hit row in 16bits word
                            cBottomData[cBottomDataOffset + 0] = (cHitPosition >> 0) & 0xFF; // First 8bits of row position
                            cBottomData[cBottomDataOffset + 1] = (cHitPosition >> 8) & 0xFF; // Second 8bits of row position
                            // push hit columns in 16bits word
                            cBottomData[cBottomDataOffset + 2] = ((1 - (cHybridId % 2)) >> 0) & 0xFF; // First 8bits of collumn position
                            cBottomData[cBottomDataOffset + 3] = ((1 - (cHybridId % 2)) >> 8) & 0xFF; // Second 8bits of collumn position
                            // push hit ToT in 16bits word
                            cBottomData[cBottomDataOffset + 4] = 1; // First 8bits of ToT. Always equals to 1 (binary readout)
                            cBottomData[cBottomDataOffset + 5] = 0; // Second 8bits of ToT
                            // shift offset by 6 elements
                            cBottomDataOffset += 6; // Offset in Data vector
                        }
                    } // end of hit loop
                } // end of chip loop
            } // end of hybrid loop
            // Fill final Top data container
            cTopDataFinal[0]   = (cNColumns >> 0) & 0xFF; // First 8bits of cNColumns
            cTopDataFinal[1]   = (cNColumns >> 8) & 0xFF; // Second 8bits of cNColumns
            cTopDataFinal[2]   = 2;                       // First 8bits of cNRows. Always 2 (two half sensors)
            cTopDataFinal[3]   = 0;                       // Second 8bits of cNRows
            uint32_t cTopNHits = (cTopData.size()) / 6;   // Divide by 6 as each hit information is stored over 6 elements
            cTopDataFinal[4]   = (cTopNHits >> 0) & 0xFF; // First 8bits of cTopNHits
            cTopDataFinal[5]   = (cTopNHits >> 8) & 0xFF; // Second 8bits of cTopNHits
            // Now append hit data data
            cTopDataFinal.insert(cTopDataFinal.end(), cTopData.begin(), cTopData.end());
            // Add data as EUDAQ sub-event block
            pEudaqSubEvent->AddBlock(cTopSensorId, cTopDataFinal);
            //
            // Fill final Bottom data container
            cBottomDataFinal[0]   = (cNColumns >> 0) & 0xFF;    // First 8bits of cNColumns
            cBottomDataFinal[1]   = (cNColumns >> 8) & 0xFF;    // Second 8bits of cNColums
            cBottomDataFinal[2]   = 2;                          // First 8bits of cNRows. Always 2 (two half sensors)
            cBottomDataFinal[3]   = 0;                          // Second 8bits of cNRows
            uint32_t cBottomNHits = (cBottomData.size()) / 6;   // Divide by 6 as each hit information is stored over 6 elements
            cBottomDataFinal[4]   = (cBottomNHits >> 0) & 0xFF; // First 8bits of cBottomNHits
            cBottomDataFinal[5]   = (cBottomNHits >> 8) & 0xFF; // Second 8bits of cBottomNHits
            // Now append hit data data
            cBottomDataFinal.insert(cBottomDataFinal.end(), cBottomData.begin(), cBottomData.end());
            // Add data as EUDAQ sub-event block
            pEudaqSubEvent->AddBlock(cBottomSensorId, cBottomDataFinal);
        } // end of optical group loop
    }

    // Extract Stubs and common information and store them in dedicated tags
    // Loop over optical groups
    for(auto cOpticalGroup: *pBoard)
    {
        // Loop over hybrids
        for(auto cHybrid: *cOpticalGroup)
        {
            uint8_t cHybridId = cHybrid->getId();
            // Extract Bx Id
            char cTagName[100];
            auto cBxId = static_cast<const D19cCic2Event*>(pPh2Event)->BxId(cHybridId);
            std::sprintf(cTagName, "bx_ID_%02d", cHybridId);
            pEudaqSubEvent->SetTag(cTagName, (uint32_t)cBxId);
            // Extract Status
            auto cStatusBit = static_cast<const D19cCic2Event*>(pPh2Event)->Status(cHybridId);
            std::sprintf(cTagName, "status_%02d", cHybridId);
            pEudaqSubEvent->SetTag(cTagName, (uint32_t)cStatusBit);
            // Loop over chips
            for(auto cChip: *cHybrid)
            {
                if(cChip->getFrontEndType() == FrontEndType::SSA2) continue;
                uint32_t cChipId = cChip->getId();
                // Extract pipeline address
                char cTagName[100];
                std::sprintf(cTagName, "pipeline_address_%02d_%02d", cHybridId, cChipId);
                pEudaqSubEvent->SetTag(cTagName, (uint32_t)pPh2Event->PipelineAddress(cHybridId, cChipId));
                // Extract L1 packet error address
                std::sprintf(cTagName, "error_%02d_%02d", cHybridId, cChipId);
                pEudaqSubEvent->SetTag(cTagName, (uint32_t)pPh2Event->Error(cHybridId, cChipId));
                // Extract Stubs
                uint32_t cStubId = 0;
                if(static_cast<D19cCic2Event*>(pPh2Event)->StubVector(cHybridId, cChipId).size() > 0)
                {
                    LOG(INFO) << BOLDMAGENTA << "\tFound  " << +static_cast<D19cCic2Event*>(pPh2Event)->StubVector(cHybridId, cChipId).size() << " stubs in Hybrid " << +cHybridId << ", Chip "
                              << +cChipId << RESET;
                }
                for(auto cStub: static_cast<D19cCic2Event*>(pPh2Event)->StubVector(cHybridId, cChipId))
                {
                    // LOG(INFO) << BLUE << "\t\tPosition " << +cStub.getPosition() << " , Row " << +cStub.getRow() << ", Bend " << +cStub.getBend() << RESET;
                    std::sprintf(cTagName, "stub_pos_%02d_%02d_%02d", cHybridId, cChipId, cStubId);
                    pEudaqSubEvent->SetTag(cTagName, (uint32_t)cStub.getPosition());
                    std::sprintf(cTagName, "stub_bend_%02d_%02d_%02d", cHybridId, cChipId, cStubId);
                    pEudaqSubEvent->SetTag(cTagName, (uint32_t)cStub.getBend());
                    std::sprintf(cTagName, "stub_row_%02d_%02d_%02d", cHybridId, cChipId, cStubId);
                    pEudaqSubEvent->SetTag(cTagName, (uint32_t)cStub.getRow());
                    std::sprintf(cTagName, "stub_center_%02d_%02d_%02d", cHybridId, cChipId, cStubId);
                    pEudaqSubEvent->SetTag(cTagName, (uint32_t)cStub.getCenter());
                    cStubId++;
                } // end of stub loop
            } // end of chip loop
        } // end of hybrid loop
    } // end of optical group loop
}

bool Eudaq2Producer::EventsPending()
{
    if(fConfigured)
    {
        for(auto cBoard: *fDetectorContainer)
        {
            BeBoard* theBoard = static_cast<BeBoard*>(cBoard);
            if(theBoard->getBoardType() == BoardType::D19C)
            {
                if(this->fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_stat.readout_block.general.readout_req") > 0) { return true; } // end of if ReadBoardReg
            } // end of if BoardType
        } // end of cBoard loop
    }
    return false;
}

void Eudaq2Producer::EnableDigitalInjection(uint8_t pPulseAmplitude, uint8_t pThresholdMPA, uint8_t pThresholdSSA)
{
    for(auto cBoard: *fDetectorContainer)
    {
        // Injection pixel and strip hits
        // Enabling testpulse
        // this->enableTestPulse(true);
        this->setFWTestPulse(true);
        setSameDacBeBoard(static_cast<BeBoard*>(cBoard), "InjectedCharge", pPulseAmplitude);

        // Arbitrary diagonal injection
        Injection              cInjection;
        std::vector<Injection> cInjections;
        cInjection.fRow    = 10;
        cInjection.fColumn = 2;
        cInjections.push_back(cInjection); // 0
        cInjection.fRow    = 20;
        cInjection.fColumn = 3;
        cInjections.push_back(cInjection); // 1
        cInjection.fRow    = 30;
        cInjection.fColumn = 4;
        cInjections.push_back(cInjection); // 2
        cInjection.fRow    = 40;
        cInjection.fColumn = 5;
        cInjections.push_back(cInjection); // 3
        cInjection.fRow    = 50;
        cInjection.fColumn = 6;
        cInjections.push_back(cInjection); // 4

        // Disabling TLU
        std::vector<std::pair<std::string, uint32_t>> cRegVec;
        LOG(INFO) << BOLDYELLOW << "Disabling TLU" << RESET;
        cRegVec.push_back({"fc7_daq_cnfg.tlu_block.tlu_enabled", 0x0});
        fBeBoardInterface->WriteBoardMultReg(cBoard, cRegVec);

        auto     cTriggerMult   = fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity");
        uint16_t cDelay         = fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse");
        int      cOptimalOffset = -1 + (cTriggerMult > 1);
        uint16_t cLatency       = cDelay + cOptimalOffset;
        LOG(DEBUG) << BOLDMAGENTA << "Expect correct latency to be " << +cLatency << RESET;

        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    if(cChip->getFrontEndType() == FrontEndType::MPA2)
                    {
                        fReadoutChipInterface->WriteChipReg(cChip, "ReadoutMode", 0x00);
                        // make sure L1 latency is configured
                        fReadoutChipInterface->WriteChipReg(cChip, "TriggerLatency", cLatency);
                        // set mapping of digital injection
                        (static_cast<PSInterface*>(fReadoutChipInterface))->digiInjection(cChip, cInjections, 0x01);
                    }

                    if(cChip->getFrontEndType() == FrontEndType::SSA2)
                    {
                        // make sure L1 latency is configured
                        fReadoutChipInterface->WriteChipReg(cChip, "TriggerLatency", cLatency - 1);
                        // Disable all SSA channels
                        fReadoutChipInterface->WriteChipReg(cChip, "ENFLAGS_ALL", 0x0);
                        // set calibration pulse duratrion to 25ns
                        fReadoutChipInterface->WriteChipReg(cChip, "CalPulse_duration", 0x01);
                        // set SSA injection pattern
                        fReadoutChipInterface->WriteChipReg(cChip, "DigCalibPattern_L_ALL", 0x01);
                        // set mappinfg of digital injection
                        for(auto cInjection: cInjections) { fReadoutChipInterface->WriteChipReg(cChip, "ENFLAGS_S" + std::to_string(cInjection.fRow), 0x9); }
                    }
                }
            }
        }
    }
}

#endif
