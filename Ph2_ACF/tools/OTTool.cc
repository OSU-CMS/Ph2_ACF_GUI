#include "OTTool.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/FEConfigurationInterface.h"
#include "HWInterface/L1ReadoutInterface.h"
#include "HWInterface/TriggerInterface.h"
#include "Utils/ContainerFactory.h"

OTTool::OTTool() : Tool()
{
    fBoardRegContainer.reset();
    fBrdRegsToPerserve.clear();
    fChipRegsToPerserve.reset();
    fSuccess = false;
    fMyName  = "OTTool";

#ifdef __USE_ROOT__
    LOG(INFO) << BOLDYELLOW << "Creating TTree to hold event data in ROOT file.." << RESET;
    fTree = new TTree();
    fTree->SetName("EventData");
    fTree->Branch("BeBoard_Id", &fBoardData.boardId);
    fTree->Branch("OpticalGroup_Id", &fBoardData.opticalGroupId);
    fTree->Branch("Event_Id", &fBoardData.cEventId);
    fTree->Branch("SensorId", &fBoardData.cSensorId);
    fTree->Branch("localX", &fBoardData.localX);
    fTree->Branch("localY", &fBoardData.localY);
    fTree->Branch("hit", &fBoardData.hit);
#endif
}

OTTool::~OTTool() {}
// Reset register on BeBoard + Chips
void OTTool::Reset()
{
    if(fReadoutMode == 1) return;

    LOG(INFO) << BOLDGREEN << "Resetting registers touched  by " << fMyName << RESET;
    // set everything back to original values .. like I wasn't here
    for(auto cBoard: *fDetectorContainer)
    {
        BeBoard* theBoard = static_cast<BeBoard*>(cBoard);
        LOG(INFO) << BOLDBLUE << fMyName << ":Resetting all registers on back-end board " << +cBoard->getId() << RESET;
        auto&                                         cBeRegMap = fBoardRegContainer.getObject(cBoard->getId())->getSummary<BeBoardRegMap>();
        std::vector<std::pair<std::string, uint32_t>> cVecBeBoardRegs;
        cVecBeBoardRegs.clear();
        for(auto cReg: cBeRegMap)
        {
            // skip registers that I should perserve for this board
            if(std::find(fBrdRegsToPerserve.begin(), fBrdRegsToPerserve.end(), cReg.first) != fBrdRegsToPerserve.end())
            {
                LOG(INFO) << BOLDBLUE << "Will not reconfigure " << cReg.first << RESET;
                continue;
            }
            cVecBeBoardRegs.push_back(make_pair(cReg.first, cReg.second.fValue));
        }
        fBeBoardInterface->WriteBoardMultReg(theBoard, cVecBeBoardRegs);
    } // for the board - reset registers

    resetPointers();
}

// Initialization function
void OTTool::Prepare()
{
    // retreive number of events from settings file
    fNevents = findValueInSettings<double>("Nevents", 10);

    if(fReadoutMode == 1) return;
    // retreive original settings for all chips and all back-end boards
    fBoardRegContainer.reset();
    ContainerFactory::copyAndInitBoard<BeBoardRegMap>(*fDetectorContainer, fBoardRegContainer);
    for(auto cBoard: *fDetectorContainer)
    {
        auto&                cBoardRegMap = fBoardRegContainer.getObject(cBoard->getId())->getSummary<BeBoardRegMap>();
        const BeBoardRegMap& cOrigRegMap  = static_cast<const BeBoard*>(cBoard)->getBeBoardRegMap();
        cBoardRegMap.insert(cOrigRegMap.begin(), cOrigRegMap.end());
    }

    // check sparsification
    for(auto cBoard: *fDetectorContainer)
    {
#ifdef __TCUSB__
        if(cBoard->getFirstObject()->flpGBT == nullptr) continue;
#endif
        uint32_t cSparsified = cBoard->getSparsification(); // this is set in the file parser .. so check using that
        LOG(DEBUG) << BOLDYELLOW << " Sparsification: " << +cSparsified << RESET;
        fBeBoardInterface->WriteBoardReg(cBoard, "fc7_daq_cnfg.physical_interface_block.cic.2s_sparsified_enable", cSparsified);
        // make sure I am in un-sparsified mode
        LOG(INFO) << BOLDGREEN << "Setting sparsification on BeBoard#" << +cBoard->getId() << ((cSparsified == 1) ? " ON" : " OFF") << RESET;
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
                fCicInterface->SetSparsification(cCic, cSparsified);
            }
        }
    }
    // for(auto cBoard: *fDetectorContainer)
    // {
    //     bool cSparsified = (fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_cnfg.physical_interface_block.cic.2s_sparsified_enable") == 1);
    //     cBoard->setSparsification(cSparsified);
    // }

    // clear map of modified registers
    // probably this should be a container per board
    // since we should be able to mix different types of boards
    // but need to decide if this is per board/OG/hybrid
    // TO-DO
    fWithCIC   = 0;
    fWithLpGBT = 0;
    fWithSSA   = 0;
    fWithMPA   = 0;
    fWithCBC   = 0;

    // figure out what type of FEs are connected
    for(auto cBoard: *fDetectorContainer)
    {
        auto cConnectedFEs = cBoard->connectedFrontEndTypes();
        fWithSSA           = (std::find_if(cConnectedFEs.begin(), cConnectedFEs.end(), [](FrontEndType x) { return (x == FrontEndType::SSA2); }) != cConnectedFEs.end()) ? 1 : 0;
        fWithMPA           = (std::find_if(cConnectedFEs.begin(), cConnectedFEs.end(), [](FrontEndType x) { return (x == FrontEndType::MPA2); }) != cConnectedFEs.end()) ? 1 : 0;
        fWithCBC           = (std::find_if(cConnectedFEs.begin(), cConnectedFEs.end(), [](FrontEndType x) { return x == FrontEndType::CBC3; }) != cConnectedFEs.end()) ? 1 : 0;
        for(auto cOpticalGroup: *cBoard)
        {
            fWithLpGBT = (cOpticalGroup->flpGBT != nullptr) ? 1 : 0;
            fWithCIC   = fWithLpGBT;
            for(auto cHybrid: *cOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
                fWithCIC   = fWithCIC || (cCic != nullptr);
            } // hybrid
        } // optical group
    } // board

    // prepare list of Chip registers to perserve
    fDetectorDataContainer = &fChipRegsToPerserve;
    ContainerFactory::copyAndInitChip<std::vector<std::string>>(*fDetectorContainer, *fDetectorDataContainer);

    fSuccess = false;
}

// configure print-out options
void OTTool::ConfigurePrintout(PrintConfig pCnfg)
{
    fPrintConfig.fVerbose    = pCnfg.fVerbose;
    fPrintConfig.fPrintEvery = pCnfg.fPrintEvery;
}
// set list of board registers to perserve
void OTTool::SetBrdRegstoPerserve(std::vector<std::string> pListOfRegs)
{
    // fBrdRegsToPerserve.clear();
    fBrdRegsToPerserve = pListOfRegs;
    // for(const auto& cRegName: pListOfRegs)
    // {
    //     LOG(DEBUG) << BOLDBLUE << "Adding " << cRegName << " to list of Brd Regs to perserve..." << RESET;
    //     fBrdRegsToPerserve.push_back(cRegName);
    // }
}
// set list of Chip registers to perserve
void OTTool::SetChipRegstoPerserve(FrontEndType pType, std::vector<std::string> pListOfRegs)
{
    LOG(INFO) << BOLDBLUE << fMyName << " setting registers to store on Chips." << RESET;
    for(auto cBoard: *fDetectorContainer)
    {
        auto& cChipRegsToPreserveThisBrd = fChipRegsToPerserve.getObject(cBoard->getId());
        for(auto cOpticalGroup: *cBoard)
        {
            auto& cChipRegsToPreserveThisOG = cChipRegsToPreserveThisBrd->getObject(cOpticalGroup->getId());
            for(auto cHybrid: *cOpticalGroup)
            {
                auto& cChipRegsToPreserveThisHybrd = cChipRegsToPreserveThisOG->getObject(cHybrid->getId());
                for(auto cChip: *cHybrid)
                {
                    if(cChip->getFrontEndType() != pType) continue;

                    auto& cChipRegsToPreserveThisChip = cChipRegsToPreserveThisHybrd->getObject(cChip->getId());
                    auto& cRegsToPerserve             = cChipRegsToPreserveThisChip->getSummary<std::vector<std::string>>();
                    cRegsToPerserve.clear();
                    for(const auto& cRegName: pListOfRegs)
                    {
                        LOG(DEBUG) << BOLDBLUE << "Adding " << cRegName << " to list of Chip Regs to perserve...Chip#" << +cChip->getId() << RESET;
                        cRegsToPerserve.push_back(cRegName);
                    }
                } // Chips
            } // Hybrds
        } // OGs
    } // brd
}

// read data from file
void OTTool::ReadDataFromFile(std::string pRawFileName)
{
    this->addFileHandler(pRawFileName, 'r');
    std::vector<uint32_t> cData;
    this->readFile(cData);
    LOG(INFO) << BOLDBLUE << "BeamTestCheck::ReadDataFromFile Read back " << +cData.size() << " 32-bit words from the .raw file : " << pRawFileName << RESET;
    for(auto cBoard: *fDetectorContainer)
    {
        size_t cNevents = fNevents;
        DecodeData(cBoard, cData, cNevents, fBeBoardInterface->getBoardType(cBoard));
        // const std::vector<Event*>& cEvents = GetEvents ();
        LOG(INFO) << BOLDBLUE << "BeamTestCheck::ReadDataFromFile decoded back " << +cNevents << " events from the .raw file [BeBoard#" << +cBoard->getId() << "]" << RESET;
        if(fPrintConfig.fVerbose) PrintData(cBoard);
    }
#ifdef __USE_ROOT__
    if(fSaveTree) fTree->Write();
#endif
}

// print data
void OTTool::PrintData(BeBoard* pBoard)
{
    const std::vector<Event*>& cEvents = GetEvents();
    LOG(INFO) << BOLDRED << "Printing events from FC7.. collected : " << +cEvents.size() << " events." << RESET;
    // fEventCountInt = 0;
    // for(auto& cEvent: cEvents)
    // {
    //     //EventPrintout(pBoard, cEvent);
    //     fEventCountInt++;
    // }
}

// wait for triggers
void OTTool::WaitForTriggers(BeBoard* pBoard)
{
    // get D19cFW Interface
    auto cInterface        = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(pBoard));
    auto cTriggerInterface = cInterface->getTriggerInterface();

    LOG(INFO) << BOLDBLUE << fMyName << "::WaitForTriggers with ReadData.. will wait to readout until I've seen " << fNevents << " triggers " << RESET;
    std::vector<uint32_t> cCompleteData(0);

    // stop triggers
    fBeBoardInterface->Stop(pBoard);
    size_t              cCounter = 0;
    uint32_t            cNevents = 0;
    bool                cBreak   = false;
    bool                cWait    = false;
    std::vector<size_t> cTriggerCounters(0);
    do {
        // check state of triggers FSM
        if(cTriggerInterface->GetTriggerState() != 1)
        {
            cTriggerInterface->Stop();
            cTriggerInterface->Start();
        }

        std::this_thread::sleep_for(std::chrono::microseconds(fReadoutPause));
        auto cTriggerCounter = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.fast_command_block.trigger_in_counter");
        cTriggerCounters.push_back(cTriggerCounter);
        if(cCounter % 200 == 0 && cCounter > 0)
        {
            LOG(INFO) << BOLDMAGENTA << "BeamTestCheck continuousReadout loop ... " << +cTriggerCounters[cTriggerCounters.size() - 1] << " triggers received" << RESET;
        }
        cCounter++;
        cBreak = (cTriggerCounter >= fNevents);
    } while(!cBreak);
    // stop triggers
    cTriggerInterface->Stop();
    // wait for 100 ms after stopping triggers just in case
    // we are still reading out a very large event
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::vector<uint32_t> cData(0);
    cNevents += ReadData(pBoard, cData, cWait);
    if(cData.size() != 0) std::move(cData.begin(), cData.end(), std::back_inserter(cCompleteData));
    DecodeData(pBoard, cCompleteData, cNevents, fBeBoardInterface->getBoardType(pBoard));
    LOG(INFO) << BOLDYELLOW << fMyName << " : Mean trigger rate is " << cTriggerCounters[cTriggerCounters.size() - 1] / (cCounter * fReadoutPause * 1e-6) << " Hz"
              << " .... readout " << cNevents << " when " << fNevents << " were requested." << RESET;
}

// poll boards for number of triggers
void OTTool::TriggerMonitor(uint32_t pDelta_s)
{
    // launch thread to catch ctrl+c from command line
    std::thread cCatchStopTh;

    // make sure that triggers have been started on all board
    for(auto cBoard: *fDetectorContainer)
    {
        auto cInterface        = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard));
        auto cTriggerInterface = cInterface->getTriggerInterface();
        if(cTriggerInterface->GetTriggerState() == 0) cTriggerInterface->Start();
    }

    // get start time for monitoring
    auto startTimeUTC_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    // create text file to store data
    // file name :
    std::stringstream cFileName;
    cFileName << fDirectoryName << "TriggerMonitor_" << startTimeUTC_us << ".dat";
    // file format :
    std::ofstream cLogFile;
    cLogFile.open(cFileName.str(), std::ios::out | std::ios::app);
    cLogFile.close();
    LOG(INFO) << fMyName << ":Starting trigger monitor ... Results will be saved to " << cFileName.str() << RESET;
    cCatchStopTh = std::thread(&OTTool::CatchStop, this);
    try
    {
        size_t cLoopCounter = 0;
        // initialize container to hold trigger counters
        DetectorDataContainer cTrigCounters;
        ContainerFactory::copyAndInitBoard<std::vector<uint32_t>>(*fDetectorContainer, cTrigCounters);
        for(auto cBoard: *fDetectorContainer)
        {
            auto& cCounterThisBrd = cTrigCounters.getObject(cBoard->getId())->getSummary<std::vector<uint32_t>>();
            cCounterThisBrd.clear();
        }
        auto cTime0 = startTimeUTC_us;
        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(pDelta_s * 1000));
            auto currentTimeUTC_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            auto cDeltaTime_us     = currentTimeUTC_us - cTime0;
            cLogFile.open(cFileName.str(), std::ios::out | std::ios::app);
            for(auto cBoard: *fDetectorContainer)
            {
                auto  cInterface        = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard));
                auto  cTriggerInterface = cInterface->getTriggerInterface();
                auto  cTriggerState     = cTriggerInterface->GetTriggerState();
                auto& cCounterThisBrd   = cTrigCounters.getObject(cBoard->getId())->getSummary<std::vector<uint32_t>>();
                cCounterThisBrd.push_back(fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_stat.fast_command_block.trigger_in_counter"));
                auto cDeltaTriggers   = (cCounterThisBrd.size() == 1) ? cCounterThisBrd[0] : cCounterThisBrd[cCounterThisBrd.size() - 1] - cCounterThisBrd[cCounterThisBrd.size() - 2];
                auto cTriggerSource   = fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_cnfg.fast_command_block.trigger_source");
                auto cTriggerRateInst = cDeltaTriggers / (cDeltaTime_us * 1e-6); // Hz
                // output to terminal
                LOG(INFO) << BOLDBLUE << "Monitoring triggers...BeBoard#" << +cBoard->getId() << " --- received " << cCounterThisBrd[cCounterThisBrd.size() - 1] << " triggers so far "
                          << " --- " << cDeltaTriggers << " triggers since the last check "
                          << " Inst. Trigger rate " << std::scientific << std::setprecision(3) << cTriggerRateInst << std::dec << " Hz "
                          << " [ trigger source is " << +cTriggerSource << " ]"
                          << " [ trigger state is " << +cTriggerState << " ]" << RESET;
                // send to file once you've got more than one point
                if(cCounterThisBrd.size() > 1) cLogFile << +cBoard->getId() << "\t" << currentTimeUTC_us << "\t" << cDeltaTriggers << "\n";
            }
            cLogFile.close();
            cTime0 = currentTimeUTC_us;
            cLoopCounter++;
        } while(fStopTriggerMonitor == 0);
    }
    catch(const std::exception& e)
    {
        LOG(INFO) << BOLDBLUE << "CatchStop caught ctrl+c ... will now exit main monitoring thread" << RESET;
        // wait for all threads to finish
        cCatchStopTh.join();
        cLogFile.close();
    }
}
// thread to monitor exit signal from main program
void OTTool::CatchStop()
{
    try
    {
        signal(SIGINT, StopTriggerMonitor);
    }
    catch(const std::exception& e)
    {
        LOG(INFO) << BOLDBLUE << "Caught stop signal from terminal..." << RESET;
        fStopTriggerMonitor = 1;
    }
}
// poll board from data
// one thread per BeBoard connected to this computer
void OTTool::ContinuousReadout()
{
    // std::vector<uint32_t> cBrdEvntCntrs(fDetectorContainer->size(), 0);
    // reset all event counters
    for(auto cBoard: *fDetectorContainer)
    {
        auto cInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard));
        cInterface->ResetEventCounter();
        // cBrdEvntCntrs[cBoard->getId()] = cInterface->GetEventCounter();
    }

    // temporary thread object representing a new thread
    // there will be one check thread per board
    std::thread* cStartThreads = new std::thread[fDetectorContainer->size()];
    for(auto cBoard: *fDetectorContainer)
    {
        // launch threads for continuous readout
        cStartThreads[cBoard->getId()] = std::thread(&OTTool::StartReadoutTh, this, cBoard->getId());
    }
    for(auto cBoard: *fDetectorContainer)
    {
        // launch threads for continuous readout
        cStartThreads[cBoard->getId()].join(); // pauses until first finishes
    }

    // temporary thread object representing a new thread
    // there will be one check thread per board
    std::thread* cCheckDoneThreads = new std::thread[fDetectorContainer->size()];
    for(auto cBoard: *fDetectorContainer)
    {
        // launch threads for continuous readout
        cCheckDoneThreads[cBoard->getId()] = std::thread(&OTTool::CheckFinishedTh, this, cBoard->getId());
    }

    // temporary thread object representing a new thread
    // there will be one readout thread per board
    std::thread* cReadoutThreads = new std::thread[fDetectorContainer->size()];
    for(auto cBoard: *fDetectorContainer)
    {
        // launch threads for continuous readout
        cReadoutThreads[cBoard->getId()] = std::thread(&OTTool::ContinuousReadoutTh, this, cBoard->getId());
    }

    // start triggers on all boards
    // for(auto cBoard: *fDetectorContainer) { fBeBoardInterface->Start(cBoard); }

    // wait until you have all events from all boards
    // exit condition for this run
    // bool   cAllFinished = false;
    // size_t cWaitCounter = 0;
    // LOG(DEBUG) << BOLDBLUE << fMyName << ":Main thread - starting to wait for events to be readout.." << RESET;
    // do
    // {
    //     size_t cNFinished = 0;
    //     for(auto cBoard: *fDetectorContainer)
    //     {
    //         auto cInterface                   = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard));
    //         cBrdEvntCntrs[cBoard->getId()] = cInterface->GetEventCounter();
    //         if(cWaitCounter % 1000 == 0)
    //             LOG(DEBUG) << BOLDBLUE << fMyName << ":Main thread ... read-back " << cBrdEvntCntrs[cBoard->getId()] << " events from BeBoard#" << +cBoard->getId() << RESET;
    //         // finished if I've received all events or if someone else has stopped triggers for me
    //         if(cBrdEvntCntrs[cBoard->getId()] >= fNevents || cInterface->GetTriggerState() == 0)
    //         {
    //             LOG(INFO) << BOLDBLUE << fMyName << ":Main thread ... finished collecting all requested events from BeBoard" << +cBoard->getId() << RESET;
    //             cNFinished++;
    //         }
    //     }
    //     cWaitCounter++;
    //     cAllFinished = (cNFinished == fDetectorContainer->size());
    // } while(!cAllFinished);

    // make double sure that all triggers have
    // been stopped here
    // stop triggers on all boards
    // for(auto cBoard: *fDetectorContainer) { fBeBoardInterface->Stop(cBoard); }

    // wait for all threads to finish
    // this will just do in sequence for all boards
    for(auto cBoard: *fDetectorContainer)
    {
        // launch threads for continuous readout
        cReadoutThreads[cBoard->getId()].join();   // pauses until first finishes
        cCheckDoneThreads[cBoard->getId()].join(); // pauses until first finishes
    }

    // now decode data sequentially
    // was having trouble doing this in the separate thread
    // for(auto cBoard : *fDetectorContainer)
    // {
    //     DecodeData(cBoard, fReadoutData[cBoard->getId()], fNevents, fBeBoardInterface->getBoardType(cBoard));
    //     LOG(INFO) << BOLDYELLOW << fMyName <<  " .... decoded " << fNevents << " events from the FC7" << RESET;
    // }
}
// poll board to check if readout has completed
void OTTool::StartReadoutTh(uint8_t cBrdId)
{
    // get D19cFW Interface
    auto cBoardIter        = std::find_if(fDetectorContainer->begin(), fDetectorContainer->end(), [&cBrdId](Ph2_HwDescription::BeBoard* x) { return x->getId() == cBrdId; });
    auto cInterface        = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(fDetectorContainer->getObject((*cBoardIter)->getId())));
    auto cTriggerInterface = cInterface->getTriggerInterface();
    auto cReadoutInterface = cInterface->getL1ReadoutInterface();
    cReadoutInterface->ResetReadout();
    cTriggerInterface->Start();
    LOG(INFO) << BOLDMAGENTA << "Started triggers on BeBoard#" << +cBrdId << RESET;
}
// continuous readout
// this will continue to read data until the stop triggers command
// has been reached
void OTTool::ContinuousReadout(BeBoard* pBoard)
{
    // get D19cFW Interface
    auto cInterface        = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(pBoard));
    auto cTriggerInterface = cInterface->getTriggerInterface();

    LOG(INFO) << BOLDBLUE << fMyName << "::ContinuousReadout ... until I've received " << fNevents << " events in the readout" << RESET;
    std::vector<uint32_t> cCompleteData(0);
    // stop triggers
    cTriggerInterface->Start();
    fEventCounter                = 0;
    size_t              cCounter = 0;
    bool                cBreak   = false;
    bool                cWait    = false;
    std::vector<size_t> cTriggerCounters(0);
    do {
        // check state of triggers FSM
        if(cTriggerInterface->GetTriggerState() != 1)
        {
            cTriggerInterface->Stop();
            cTriggerInterface->Start();
        }
        std::this_thread::sleep_for(std::chrono::microseconds(fReadoutPause));
        auto                  cTriggerCounter = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_stat.fast_command_block.trigger_in_counter");
        std::vector<uint32_t> cData(0);
        fEventCounter += ReadData(pBoard, cData, cWait);
        if(cData.size() != 0) std::move(cData.begin(), cData.end(), std::back_inserter(cCompleteData));
        cTriggerCounters.push_back(cTriggerCounter);
        if(cCounter % 200 == 0 && cCounter > 0)
        {
            LOG(INFO) << BOLDMAGENTA << "BeamTestCheck continuousReadout loop ... " << +cTriggerCounters[cTriggerCounters.size() - 1] << " triggers received" << RESET;
        }
        cCounter++;
        cBreak = (fEventCounter >= fNevents);
    } while(!cBreak);
    cTriggerInterface->Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::vector<uint32_t> cData(0);
    fEventCounter += ReadData(pBoard, cData, cWait);
    if(cData.size() != 0) std::move(cData.begin(), cData.end(), std::back_inserter(cCompleteData));
    DecodeData(pBoard, cCompleteData, fEventCounter, fBeBoardInterface->getBoardType(pBoard));
    LOG(INFO) << BOLDYELLOW << fMyName << " : Mean trigger rate is " << cTriggerCounters[cTriggerCounters.size() - 1] / (cCounter * fReadoutPause * 1e-6) << " Hz"
              << " .... readout " << fEventCounter << RESET;
}
// poll board to check if readout has completed
void OTTool::CheckFinishedTh(uint8_t cBrdId)
{
    // get D19cFW Interface
    auto cBoardIter        = std::find_if(fDetectorContainer->begin(), fDetectorContainer->end(), [&cBrdId](Ph2_HwDescription::BeBoard* x) { return x->getId() == cBrdId; });
    auto cInterface        = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(fDetectorContainer->getObject((*cBoardIter)->getId())));
    auto cTriggerInterface = cInterface->getTriggerInterface();

    // wait until triggers have started
    size_t cWaitCounter = 0;
    size_t cMaxWait     = 10000;
    do {
        std::this_thread::sleep_for(std::chrono::microseconds(fThreadWait));
        if(cWaitCounter % 10 == 0)
        {
            LOG(DEBUG) << BOLDBLUE << "\t\t" << fMyName << " " << __PRETTY_FUNCTION__ << " cTriggerInterface->GetTriggerState() " << cTriggerInterface->GetTriggerState() << RESET;
            LOG(DEBUG) << BOLDBLUE << "\t\t" << fMyName << " " << __LINE__ << ": Waiting for triggers to start on BeBoard#" << +cBrdId << RESET;
        }
        cWaitCounter++;
        LOG(INFO) << BOLDBLUE << __PRETTY_FUNCTION__ << " cTriggerInterface->GetTriggerState() " << cTriggerInterface->GetTriggerState() << " cWaitCounter " << cWaitCounter << " cMaxWait " << cMaxWait
                  << RESET;
    } while(cTriggerInterface->GetTriggerState() != 1 && cWaitCounter < cMaxWait);

    auto cCounter = cInterface->GetEventCounter();
    do {
        auto nTrigger = cInterface->ReadReg("fc7_daq_stat.fast_command_block.trigger_in_counter");
        LOG(DEBUG) << BOLDGREEN << __PRETTY_FUNCTION__ << " nTriggers " << nTrigger << RESET;
        LOG(DEBUG) << BOLDGREEN << __PRETTY_FUNCTION__ << " Delta " << nTrigger - cCounter << RESET;

        if(cCounter >= fNevents || cTriggerInterface->GetTriggerState() == 0)
        {
            LOG(INFO) << BOLDBLUE << fMyName << ":Main thread ... finished collecting all requested events from BeBoard" << +cBrdId << RESET;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(fReadoutPause));
        cCounter = cInterface->GetEventCounter();
    } while(cCounter < fNevents);
    LOG(INFO) << BOLDBLUE << fMyName << ": check finished thread ... finished collecting all requested events from BeBoard" << +cBrdId << " .. will stop triggers.. " << RESET;
    cInterface->Stop();
}
// poll board from data
// one thread per BeBoard connected to this computer
void OTTool::ContinuousReadoutTh(uint8_t cBrdId)
{
    fReadoutData[cBrdId].clear();
    LOG(DEBUG) << BOLDBLUE << fMyName << ":Starting continuous readout thread for BeBoard#" << +cBrdId << RESET;
    // get D19cFW Interface
    auto cBoardIter        = std::find_if(fDetectorContainer->begin(), fDetectorContainer->end(), [&cBrdId](Ph2_HwDescription::BeBoard* x) { return x->getId() == cBrdId; });
    auto cInterface        = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(fDetectorContainer->getObject((*cBoardIter)->getId())));
    auto cTriggerInterface = cInterface->getTriggerInterface();
    // wait until triggers have started
    size_t cWaitCounter = 0;
    size_t cMaxWait     = 10000;
    do {
        std::this_thread::sleep_for(std::chrono::microseconds(fThreadWait));
        if(cWaitCounter % 100 == 0) LOG(DEBUG) << BOLDBLUE << "\t\t" << fMyName << ":Waiting for triggers to start on BeBoard#" << +cBrdId << RESET;
        cWaitCounter++;
    } while(cTriggerInterface->GetTriggerState() != 1 && cWaitCounter < cMaxWait);

    if(cWaitCounter == cMaxWait)
    {
        LOG(INFO) << BOLDRED << "Triggers not started on this board.. start them myself!" << RESET;
        cTriggerInterface->Start();
        do {
            std::this_thread::sleep_for(std::chrono::microseconds(fThreadWait));
            LOG(INFO) << BOLDRED << " ... waiting  for triggers to start... " << RESET;
        } while(cTriggerInterface->GetTriggerState() == 0);
    }

    LOG(DEBUG) << BOLDBLUE << fMyName << ":Triggers started .. now polling readout.." << RESET;
    bool                cWait = false;
    std::vector<size_t> cTriggerCounters(0);
    // repeat until triggers have stopped
    cWaitCounter        = 0;
    size_t cLclEvntCntr = 0;
    auto   cStartTime = std::chrono::high_resolution_clock::now(), cEndTime = cStartTime;
    size_t cAccumulatedWaits = 0;

    auto cTriggerState = cTriggerInterface->GetTriggerState();
    do {
        cTriggerState = cTriggerInterface->GetTriggerState();
        cAccumulatedWaits += fReadoutPause;
        std::this_thread::sleep_for(std::chrono::microseconds(fReadoutPause));
        auto                  cTriggerSource  = fBeBoardInterface->ReadBoardReg((*cBoardIter), "fc7_daq_cnfg.fast_command_block.trigger_source");
        auto                  cTriggerCounter = fBeBoardInterface->ReadBoardReg((*cBoardIter), "fc7_daq_stat.fast_command_block.trigger_in_counter");
        std::vector<uint32_t> cData(0);
        // cLclEvntCntr += ReadData(*cBoardIter, cData, cWait);
        cLclEvntCntr += fBeBoardInterface->ReadData((*cBoardIter), false, cData, cWait);
        if(cData.size() != 0) std::move(cData.begin(), cData.end(), std::back_inserter(fReadoutData[cBrdId]));

        cTriggerCounters.push_back(cTriggerCounter);
        if(cWaitCounter % 100 == 0 && cWaitCounter > 0)
        {
            LOG(DEBUG) << BOLDBLUE << "\t\t" << fMyName << ":Waiting for triggers to be stopped on BeBoard#" << +cBrdId << " continuousReadout loop ... "
                       << +cTriggerCounters[cTriggerCounters.size() - 1] << " triggers received"
                       << " trigger source is " << +cTriggerSource << " trigger state is " << +cTriggerState << " and " << cLclEvntCntr << " events in the readout so far ... " << RESET;
        }
        cWaitCounter++;
    } while(cTriggerState == 1);
    // now decode data
    cAccumulatedWaits += 100 * 1e3;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::vector<uint32_t> cData(0);
    // cLclEvntCntr += ReadData(*cBoardIter, cData, cWait);
    cLclEvntCntr += fBeBoardInterface->ReadData((*cBoardIter), false, cData, cWait);
    cEndTime       = std::chrono::high_resolution_clock::now();
    auto cDuration = std::chrono::duration_cast<std::chrono::milliseconds>(cEndTime - cStartTime).count();
    if(cData.size() != 0) std::move(cData.begin(), cData.end(), std::back_inserter(fReadoutData[cBrdId]));
    LOG(INFO) << BOLDYELLOW << "BeBoard#" << +cBrdId << " readData found " << cLclEvntCntr << " events...\tAccumulated " << cAccumulatedWaits * 1e-3 << " ms of waits.. read-out "
              << fReadoutData[cBrdId].size() << " 32-bit words in " << cDuration << " ms." << RESET;

    DecodeData(*cBoardIter, fReadoutData[cBrdId], fNevents, fBeBoardInterface->getBoardType(*cBoardIter));
    LOG(INFO) << BOLDYELLOW << fMyName << "  Th for BeBoard#" << +cBrdId << " .... finished decoding " << fNevents << " events from the FC7" << RESET;
}
// event print-out
void OTTool::EventPrintout(BeBoard* pBoard, Event* pEvent)
{
    auto cSparsified = pBoard->getSparsification();
    if(cSparsified)
        LOG(DEBUG) << BOLDBLUE << "Checking with internal - sparisified data" << RESET;
    else
        LOG(DEBUG) << BOLDBLUE << "Checking with internal - un-sparisified data" << RESET;

    if(pEvent->GetEventCount() % fPrintConfig.fPrintEvery != 0) return;
    std::stringstream cEvntHeader;
    cEvntHeader << "Event#" << +pEvent->GetEventCount() << " -- " << +fEventCountInt << " in readout..." << RESET;
    std::stringstream cHeader;

    std::ofstream cOutFile_LT, cOutFile_RT, cOutFile_LB, cOutFile_RB;
    cOutFile_LT.open("OTTool_LT.dat", std::ios_base::app);
    cOutFile_RT.open("OTTool_RT.dat", std::ios_base::app);
    cOutFile_LB.open("OTTool_LB.dat", std::ios_base::app);
    cOutFile_RB.open("OTTool_RB.dat", std::ios_base::app);
    fBoardData.cEventId = pEvent->GetEventCount();
    fBoardData.boardId  = pBoard->getId();
    for(auto cOpticalGroup: *pBoard)
    {
        std::vector<uint16_t> cBxIds(0);
        std::vector<uint16_t> cL1Ids(0);
        std::stringstream     cOutOfSync;
        fBoardData.opticalGroupId = cOpticalGroup->getId();
        for(auto cHybrid: *cOpticalGroup)
        {
            auto cL1IdCIC  = static_cast<D19cCic2Event*>(pEvent)->L1Id(cHybrid->getId(), 0);
            auto cL1Status = static_cast<D19cCic2Event*>(pEvent)->L1Status(cHybrid->getId());
            auto cBxId     = (pEvent)->BxId(cHybrid->getId());
            auto cStubStat = static_cast<D19cCic2Event*>(pEvent)->StubStatus(cHybrid->getId());

            if(pEvent->GetEventCount() < 10000)
            {
                std::vector<uint32_t> cHits_TopSensor(cHybrid->size() * 127, 0);
                std::vector<uint32_t> cHits_BottomSensor(cHybrid->size() * 127, 0);
                uint32_t              cTotalNHits = 0;
                for(auto cChip: *cHybrid)
                {
                    uint16_t cOffset = cChip->getId() * cChip->size() / 2.;
                    auto     cHits   = pEvent->GetHits(cHybrid->getId(), cChip->getId());
                    for(auto cChnl = 0; cChnl < (int)cChip->size(); cChnl++)
                    {
                        uint16_t cStripOffset = cOffset; //(cChnlIndx % 2 == 0) ? cOffset : (cNchannels*8) / 2 + cOffset;
                        uint16_t cStripId     = cStripOffset + cChnl / 2;
                        if(cHybrid->getId() % 2 == 0)
                        {
                            cStripOffset = (cChip->size() * 8) / 2 - 1; //(cChnlIndx % 2 == 0) ? (cNchannels*8) / 2 : (cNchannels*8);
                            cStripId     = cStripOffset - (cChip->getId() * cChip->size() / 2 + cChnl / 2);
                        }
                        fBoardData.localX                       = cHybrid->getId() % 2;
                        fBoardData.localY                       = cStripId;
                        fBoardData.cSensorId                    = (cChnl % 2 == 0) ? 0 : 1;
                        std::pair<uint16_t, uint16_t> theHit    = {0, cChnl};
                        auto                          cHitFound = std::find(cHits.begin(), cHits.end(), theHit) != cHits.end();
                        fBoardData.hit                          = (cHitFound) ? 1 : 0;
#ifdef __USE_ROOT__
                        if(fSaveTree) fTree->Fill();
#endif
                        cTotalNHits += (cHitFound) ? 1 : 0;
                        if(cChnl % 2 == 0)
                            cHits_BottomSensor[cStripId] = cHitFound ? 1 : 0;
                        else
                            cHits_TopSensor[cStripId] = cHitFound ? 1 : 0;
                    }
                }
                // if(cTotalNHits >= 0 )
                //{
                std::stringstream cEventPrintout_TopSensor;
                std::stringstream cEventPrintout_BottomSensor;
                // cEventPrintout_TopSensor << cBxId << "\t";
                // cEventPrintout_BottomSensor << cBxId << "\t";
                for(auto cStripId = 0; cStripId < cHybrid->size() * 127; cStripId++)
                {
                    if(cStripId < cHybrid->size() * 127 - 1)
                    {
                        cEventPrintout_TopSensor << cHits_TopSensor[cStripId] << "\t";
                        cEventPrintout_BottomSensor << cHits_BottomSensor[cStripId] << "\t";
                    }
                    else
                    {
                        cEventPrintout_TopSensor << cHits_TopSensor[cStripId];
                        cEventPrintout_BottomSensor << cHits_BottomSensor[cStripId];
                    }
                }

                if(cHybrid->getId() % 2 == 0)
                {
                    cOutFile_RT << cEventPrintout_TopSensor.str() << "\n";
                    cOutFile_RB << cEventPrintout_BottomSensor.str() << "\n";
                }
                else
                {
                    cOutFile_LT << cEventPrintout_TopSensor.str() << "\n";
                    cOutFile_LB << cEventPrintout_BottomSensor.str() << "\n";
                }
                //}
            } // checking

            std::stringstream cOutStubs;
            std::stringstream cOutL1;
            std::stringstream cOutAna;
            std::stringstream cOutEvntHeader;
            if(cStubStat != 0x00 || cL1Status != 0x00)
            {
                cOutEvntHeader << cEvntHeader.str() << BOLDRED << " Hybrid#" << +cHybrid->getId() << " on Link#" << +cOpticalGroup->getId() << " L1Id " << +cL1IdCIC << " Stub status is "
                               << std::bitset<8>(cStubStat) << " L1 status [FEs] is " << std::bitset<8>(cL1Status) << " L1 status [CIC] is " << std::bitset<1>(cL1Status & 0x1) << " BxId is "
                               << +cBxId;
                cHeader << BOLDRED << "\t\t.. "
                        << " Hybrid#" << +cHybrid->getId() << " on Link#" << +cOpticalGroup->getId() << " L1Id " << +cL1IdCIC << " Stub status is " << std::bitset<8>(cStubStat)
                        << " L1 status [FEs] is " << std::bitset<8>(cL1Status) << " L1 status [CIC] is " << std::bitset<1>(cL1Status & 0x1) << " BxId is " << +cBxId << "\n"
                        << RESET;
            }
            else
            {
                cOutEvntHeader << cEvntHeader.str() << BOLDGREEN << " Hybrid#" << +cHybrid->getId() << " on Link#" << +cOpticalGroup->getId() << " L1Id " << +cL1IdCIC << " Stub status is "
                               << std::bitset<8>(cStubStat) << " L1 status [FEs] is " << std::bitset<8>(cL1Status) << " L1 status [CIC] is " << std::bitset<1>(cL1Status & 0x1) << " BxId is "
                               << +cBxId;
                cHeader << BOLDGREEN << "\t\t.. "
                        << " Hybrid#" << +cHybrid->getId() << " on Link#" << +cOpticalGroup->getId() << " L1Id " << +cL1IdCIC << " Stub status is " << std::bitset<8>(cStubStat)
                        << " L1 status [FEs] is " << std::bitset<8>(cL1Status) << " L1 status [CIC] is " << std::bitset<1>(cL1Status & 0x1) << " BxId is " << +cBxId << "\n"
                        << RESET;
            }

            if(cBxIds.size() == 0)
            {
                cBxIds.push_back(cBxId);
                cL1Ids.push_back(cL1IdCIC);
            }
            else
            {
                if(std::find(cBxIds.begin(), cBxIds.end(), cBxId) == cBxIds.end()) cBxIds.push_back(cBxId);
                if(std::find(cL1Ids.begin(), cL1Ids.end(), cL1IdCIC) == cL1Ids.end()) cL1Ids.push_back(cL1IdCIC);
            }
        }
    }

    cOutFile_LT.close();
    cOutFile_RT.close();
    cOutFile_LB.close();
    cOutFile_RB.close();
}

//
void OTTool::InjectPattern(BeBoard* pBoard, std::vector<Injection> pInjections, int pChipId)
{
    bool cInjectAll = false;
    LOG(DEBUG) << BOLDMAGENTA << "Injecting " << +pInjections.size() << " in PS module.." << RESET;
    // inject pixel clusters
    bool cLastFive = true;
    for(auto cOpticalReadout: *pBoard)
    {
        for(auto cHybrid: *cOpticalReadout)
        {
            for(auto cChip: *cHybrid)
            {
                // fReadoutChipInterface->WriteChipReg(cChip, "ReadoutMode",0x0);
                if(cChip->getId() % 8 != pChipId && pChipId > 0) continue;
                LOG(DEBUG) << BOLDMAGENTA << "Injecting patterns in Chip#" << +cChip->getId() << RESET;
                // make sure L1 latency is configured
                if(cChip->getFrontEndType() == FrontEndType::MPA2)
                {
                    LOG(DEBUG) << BOLDMAGENTA << __PRETTY_FUNCTION__ << " fInjectionType " << +fInjectionType << RESET;
                    if(fInjectionType == 0)
                    {
                        // for digi injection .. explicity disable all other pixels
                        fReadoutChipInterface->WriteChipReg(cChip, "ENFLAGS_ALL", 0x0);
                        (static_cast<PSInterface*>(fReadoutChipInterface))->digiInjection(cChip, pInjections, 0x01);
                    }
                    else // analog injection
                    {
                        int inj = 0;
                        fReadoutChipInterface->WriteChipReg(cChip, "ENFLAGS_ALL", 0x0); // disable all pixels :)
                        for(auto cInjection: pInjections)
                        {
                            if(pInjections.size() - inj > 5 and cLastFive) continue;
                            if(cInjectAll) continue;
                            auto cPxl = cInjection.fColumn * NSSACHANNELS + (uint32_t)cInjection.fRow;
                            LOG(INFO) << BOLDBLUE << " Injecting in pixel " << +cPxl << " column " << +cInjection.fColumn << " row " << +cInjection.fRow << RESET;
                            std::stringstream cRegName;
                            cRegName << "ENFLAGS_C" << +cInjection.fColumn << "_R" << +cInjection.fRow;
                            fReadoutChipInterface->WriteChipReg(cChip, cRegName.str(), 0x4F, false);

                            /* Multi strips injection*/
                            /*
                            LOG(INFO) << BOLDBLUE << " Injecting in pixel " << +(cPxl+1) << " column " << +cInjection.fColumn << " row " << +cInjection.fRow << RESET;
                            // Inject in a second pixel
                            std::stringstream cRegName2;
                            cRegName << "ENFLAGS_C" << (cInjection.fColumn + 1) << "_R" << +cInjection.fRow;
                            fReadoutChipInterface->WriteChipReg(cChip, cRegName2.str(), 0x4F, false);


                            LOG(INFO) << BOLDBLUE << " Injecting in pixel " << +(cPxl+2) << " column " << +cInjection.fColumn << " row " << +cInjection.fRow << RESET;
                            // Inject in a third pixel
                            std::stringstream cRegName3;
                           cRegName << "ENFLAGS_C" << (cInjection.fColumn + 2) << "_R" << +cInjection.fRow;
                            fReadoutChipInterface->WriteChipReg(cChip, cRegName3.str(), 0x4F, false);
                            */

                            inj += 1;
                        }
                        if(cInjectAll) fReadoutChipInterface->WriteChipReg(cChip, "ENFLAGS_ALL", 0x5F);
                    }
                }
                if(cChip->getFrontEndType() == FrontEndType::SSA2)
                {
                    // for digi injection .. explicity disable all other strips
                    uint16_t enflags = cChip->getReg("ENFLAGS");
                    if(fInjectionType == 0) { enflags = (enflags & 0xE7) + 0x8; }
                    else if(fInjectionType == 1)
                    {
                        enflags = (enflags & 0xE7) + 0x10;
                        LOG(DEBUG) << BOLDRED << __LINE__ << "] ANALOG ENFLAGS Memory=0x " << std::hex << enflags << std::dec << RESET;
                    }
                    else
                    {
                        LOG(ERROR) << BOLDRED << "Unknown charge injection type " << fInjectionType << RESET;
                        throw std::runtime_error(std::string("Unknown charge injection type "));
                    }
                    if(pInjections.size() > 0 && fInjectionType == 0)
                    {
                        fReadoutChipInterface->WriteChipReg(cChip, "ENFLAGS_ALL", 0x0);            // Disable all strip (StripControl1 in SSA2 manual)
                        fReadoutChipInterface->WriteChipReg(cChip, "DigCalibPattern_L_ALL", 0x00); // Set injection pattern
                        fReadoutChipInterface->WriteChipReg(cChip, "DigCalibPattern_H_ALL", 0x00); // Set injection pattern
                        fReadoutChipInterface->WriteChipReg(cChip, "CalPulse_duration", 0x01);     // CalPulse_duration or CalPulse_lenght is Duration of the Calibration pulse distributed to the Strip
                                                                                                   // channels, in multiples of 25ns. Corresponds to bits 7:4 of control_2.
                    }
                    LOG(DEBUG) << BOLDRED << __LINE__ << "] Setting CalPulse_duration HARDCODED TO 0x8" << RESET;
                    fReadoutChipInterface->WriteChipReg(cChip, "CalPulse_duration", 0x08);
                    if(cInjectAll)
                    {
                        LOG(DEBUG) << BOLDRED << __LINE__ << "] INJECTING ALL STRIPS, ENFLAGS Memory ALL=0x " << std::hex << enflags << std::dec << RESET;
                        fReadoutChipInterface->WriteChipReg(cChip, "ENFLAGS_ALL", enflags);
                    }
                    else if(pInjections.size() > 0)
                    {
                        for(auto cInjection: pInjections)
                        {
                            uint8_t stripInj = cInjection.fColumn + 1;
                            LOG(DEBUG) << BOLDRED << __LINE__ << "] INJECTING STRIP: " << +stripInj << " with ENFLAGS Memory ALL=0x " << std::hex << enflags << std::dec << RESET;

                            // Inject in one strip
                            std::stringstream cRegNameEn;

                            cRegNameEn << "ENFLAGS_S" << +stripInj;
                            enflags          = (enflags & 0xFE) + 0x01; // Making sure to enable the strip
                            uint16_t readReg = fReadoutChipInterface->ReadChipReg(cChip, cRegNameEn.str());
                            LOG(DEBUG) << BOLDRED << __LINE__ << "] is STRIP disabled? Checking number " << +stripInj << " 0x" << std::hex << readReg << std::dec << RESET;

                            fReadoutChipInterface->WriteChipReg(cChip, cRegNameEn.str(), enflags);
                            readReg = fReadoutChipInterface->ReadChipReg(cChip, cRegNameEn.str());
                            LOG(DEBUG) << BOLDRED << __LINE__ << "] !!! Enable STRIP, Checking number " << +stripInj << " 0x" << std::hex << readReg << std::dec << RESET;

                            if(fInjectionType == 0)
                            {
                                std::stringstream cRegNamePattern;
                                cRegNamePattern << "DigCalibPattern_L_S" << +stripInj;
                                fReadoutChipInterface->WriteChipReg(cChip, cRegNamePattern.str(), 0x01);
                            }
                        }
                    }
                    else
                    {
                        LOG(ERROR) << BOLDRED << "No strip has been selected for injection!" << RESET;
                        throw std::runtime_error(std::string("No strip has been selected for injection!"));
                    }
                }
            } // chip
        } // hybrid
    } // optica]l group
}
void OTTool::UpdateFromRegMap(BeBoard* pBoard)
{
    // important registers for beBoard
    std::vector<std::string> cBoardRegs{
        "fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity", // 0
        "fc7_daq_cnfg.readout_block.global.common_stubdata_delay",
        "fc7_daq_cnfg.fast_command_block.trigger_source", // 6

        "fc7_daq_cnfg.tlu_block.trigger_id_delay", // 1
        "fc7_daq_cnfg.tlu_block.tlu_enabled",      // 0
        "fc7_daq_cnfg.tlu_block.handshake_mode"    // 2
    };

    BeBoardRegMap cRegMap = pBoard->getBeBoardRegMap();
    for(auto cReg: cBoardRegs)
    {
        LOG(INFO) << BOLDBLUE << "Setting " << cReg << " to " << +cRegMap[cReg].fValue << RESET;
        fBeBoardInterface->WriteBoardReg(pBoard, cReg, cRegMap[cReg].fValue);
    }

    // set MPA Sync with SSA
    LOG(INFO) << BOLDRED << "Setting MPA Sync with SSA" << RESET;
    if(true)
    {
        for(auto cOpticalGroup: *pBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;
                    uint16_t cValueInMemory;
                    uint16_t cValueInChip;
                    cValueInMemory = cChip->getReg("EdgeSelTrig");
                    fReadoutChipInterface->WriteChipReg(cChip, "EdgeSelTrig", cValueInMemory);
                    cValueInChip = fReadoutChipInterface->ReadChipReg(cChip, "EdgeSelTrig");
                    LOG(DEBUG) << BOLDYELLOW << "Chip # " << +cChip->getId() << " Set EdgeSelTrig register to 0x" << std::hex << cValueInMemory << "=0x" << cValueInChip << std::dec << RESET;

                    cValueInMemory = cChip->getReg("EdgeSelT1Raw");
                    fReadoutChipInterface->WriteChipReg(cChip, "EdgeSelT1Raw", cValueInMemory);
                    cValueInChip = fReadoutChipInterface->ReadChipReg(cChip, "EdgeSelT1Raw");
                    LOG(DEBUG) << BOLDYELLOW << "Chip # " << +cChip->getId() << " Set EdgeSelT1Raw register to 0x" << std::hex << cValueInMemory << "=0x" << cValueInChip << std::dec << RESET;

                    cValueInMemory = cChip->getReg("LatencyRx320");
                    fReadoutChipInterface->WriteChipReg(cChip, "LatencyRx320", cValueInMemory);
                    cValueInChip = fReadoutChipInterface->ReadChipReg(cChip, "LatencyRx320");
                    LOG(DEBUG) << BOLDYELLOW << "Chip # " << +cChip->getId() << " Set LatencyRx320 register to 0x" << std::hex << cValueInMemory << "=0x" << cValueInChip << std::dec << RESET;

                    cValueInMemory = cChip->getReg("LatencyRx40");
                    fReadoutChipInterface->WriteChipReg(cChip, "LatencyRx40", cValueInMemory);
                    cValueInChip = fReadoutChipInterface->ReadChipReg(cChip, "LatencyRx40");
                    LOG(DEBUG) << BOLDYELLOW << "Chip # " << +cChip->getId() << " Set LatencyRx40 register to 0x" << std::hex << cValueInMemory << "=0x" << cValueInChip << std::dec << RESET;
                }
            }
        }
        LOG(INFO) << BOLDRED << "Done Setting MPA Sync with SSA" << RESET;
    }

    // set thresholds
    LOG(INFO) << BOLDRED << "Setting Thresholds" << RESET;
    for(auto cOpticalGroup: *pBoard)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            for(auto cChip: *cHybrid)
            {
                uint16_t cValueInMemory;
                uint16_t cValueInChip;
                if(cChip->getFrontEndType() == FrontEndType::CBC3)
                {
                    cValueInMemory = (cChip->getReg("VCth1") + (cChip->getReg("VCth2") << 8));
                    fReadoutChipInterface->WriteChipReg(cChip, "Threshold", cValueInMemory);
                }
                else if(cChip->getFrontEndType() == FrontEndType::SSA2)
                {
                    cValueInMemory = cChip->getReg("Bias_THDAC");
                    fReadoutChipInterface->WriteChipReg(cChip, "Threshold", cValueInMemory);
                    cValueInChip = fReadoutChipInterface->ReadChipReg(cChip, "Threshold");
                    LOG(INFO) << BOLDYELLOW << "Chip # " << +cChip->getId() << " Set Threshold register to " << cValueInMemory << "=" << cValueInChip << RESET;

                    cValueInMemory = cChip->getReg("Bias_THDACHIGH");
                    fReadoutChipInterface->WriteChipReg(cChip, "ThresholdHigh", cValueInMemory);
                    cValueInChip = fReadoutChipInterface->ReadChipReg(cChip, "ThresholdHigh");
                    LOG(INFO) << BOLDYELLOW << "Chip # " << +cChip->getId() << " Set ThresholdHigh register to " << cValueInMemory << "=" << cValueInChip << RESET;
                }
                else if(cChip->getFrontEndType() == FrontEndType::MPA2)
                {
                    cValueInMemory = cChip->getReg("ThDAC0");
                    fReadoutChipInterface->WriteChipReg(cChip, "Threshold", cValueInMemory);
                    cValueInChip = fReadoutChipInterface->ReadChipReg(cChip, "Threshold");
                    LOG(INFO) << BOLDYELLOW << "Chip # " << +cChip->getId() << " Set Threshold register to " << cValueInMemory << "=" << cValueInChip << RESET;
                }
            }
        }
    }
    LOG(INFO) << BOLDRED << "Done Setting Thresholds" << RESET;

    LOG(INFO) << BOLDRED << "Setting Stub Mode and Window" << RESET;
    // set stub mode from xml
    for(auto cOpticalGroup: *pBoard)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            for(auto cChip: *cHybrid)
            {
                uint16_t cValueInChip;
                if(cChip->getFrontEndType() == FrontEndType::MPA2)
                {
                    uint16_t cValueInMemory = cChip->getReg("ECM");
                    uint16_t cModeMemory    = (cValueInMemory & 0xC0) >> 6;
                    uint16_t cWindowMemory  = cValueInMemory & 0x3F;
                    fReadoutChipInterface->WriteChipReg(cChip, "StubMode", cModeMemory);
                    cValueInChip = fReadoutChipInterface->ReadChipReg(cChip, "StubMode");
                    LOG(INFO) << BOLDYELLOW << "Chip # " << +cChip->getId() << " Set StubMode register to " << cModeMemory << "=" << cValueInChip << RESET;

                    fReadoutChipInterface->WriteChipReg(cChip, "StubWindow", cWindowMemory);
                    cValueInChip = fReadoutChipInterface->ReadChipReg(cChip, "StubWindow");
                    LOG(INFO) << BOLDYELLOW << "Chip # " << +cChip->getId() << " Set StubWindow register to " << cWindowMemory << "=" << cValueInChip << RESET;
                }
            }
        }
    }
    LOG(INFO) << BOLDRED << "Done Setting Stub Mode and Window" << RESET;

    // set hit sampling mode from xml
    LOG(INFO) << BOLDRED << "Setting Sampling Mode" << RESET;
    for(auto cOpticalGroup: *pBoard)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            for(auto cChip: *cHybrid)
            {
                if(cChip->getFrontEndType() == FrontEndType::MPA2)
                {
                    auto cValueInMemory = cChip->getReg("PixelControl_ALL");
                    LOG(INFO) << BOLDRED << "Chip # " << +cChip->getId() << " Set ModeSel, ClusterCut and HipCut register to 0x" << std::hex << cValueInMemory << std::dec << RESET;
                    auto cModeInMemory       = cValueInMemory & 0x03;
                    auto cClusterCutInMemory = (cValueInMemory & 0x1C) >> 2;
                    auto cHipCutInMemory     = (cValueInMemory & 0xE0) >> 5;
                    fReadoutChipInterface->WriteChipReg(cChip, "ModeSel_ALL", cModeInMemory);
                    fReadoutChipInterface->WriteChipReg(cChip, "ClusterCut_ALL", cClusterCutInMemory);
                    fReadoutChipInterface->WriteChipReg(cChip, "HipCut_ALL", cHipCutInMemory);
                    uint16_t cValueInChip = fReadoutChipInterface->ReadChipReg(cChip, "PixelControl_ALL");
                    LOG(INFO) << BOLDYELLOW << "Chip # " << +cChip->getId() << " Set ModeSel, ClusterCut and HipCut register to 0x" << std::hex << cValueInMemory << "=0x" << cValueInChip << std::dec
                              << RESET;
                }
                else if(cChip->getFrontEndType() == FrontEndType::SSA2)
                {
                    auto cValueInMemory = cChip->getReg("ENFLAGS");
                    LOG(INFO) << BOLDYELLOW << "Chip # " << +cChip->getId() << " Setting SamplingMode_ALL register to 0x" << std::hex << cValueInMemory << std::dec << RESET;
                    auto cMode = (cValueInMemory & 0x60) >> 5;
                    fReadoutChipInterface->WriteChipReg(cChip, "SamplingMode_ALL", cMode);
                    uint16_t cValueInChip = fReadoutChipInterface->ReadChipReg(cChip, "ENFLAGS");
                    LOG(INFO) << BOLDYELLOW << "Chip # " << +cChip->getId() << " Set ENFLAGS register to 0x" << std::hex << cValueInMemory << "=" << cValueInChip << std::dec << RESET;
                }
            }
        }
    }
    LOG(INFO) << BOLDRED << "Done Setting Sampling Mode" << RESET;

    // charge injection
    LOG(INFO) << BOLDRED << "Setting Charge Injection" << RESET;
    for(auto cOpticalGroup: *pBoard)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            for(auto cChip: *cHybrid)
            {
                std::string cRegName = "";
                if(cChip->getFrontEndType() == FrontEndType::MPA2) { cRegName = "CalDAC0"; }
                else if(cChip->getFrontEndType() == FrontEndType::SSA2) { cRegName = "Bias_CALDAC"; }
                else if(cChip->getFrontEndType() == FrontEndType::CBC3) { cRegName = "MiscTestPulseCtrl&AnalogMux"; }
                uint16_t cValueInMemory = cChip->getReg(cRegName);
                if(cChip->getFrontEndType() == FrontEndType::CBC3) { cValueInMemory = (cValueInMemory >> 6) & 0x3F; }

                if(cChip->getFrontEndType() == FrontEndType::SSA2 || cChip->getFrontEndType() == FrontEndType::MPA2 || cChip->getFrontEndType() == FrontEndType::CBC3)
                {
                    fReadoutChipInterface->WriteChipReg(cChip, "InjectedCharge", cValueInMemory);
                    uint16_t cValueInChip = fReadoutChipInterface->ReadChipReg(cChip, "InjectedCharge");
                    LOG(INFO) << BOLDYELLOW << "Chip # " << +cChip->getId() << " Set InjectedCharge register to " << cValueInMemory << "=" << cValueInChip << RESET;
                }
            }
        }
    }
    LOG(INFO) << BOLDRED << "Done Setting Charge Injection" << RESET;

    // latency
    LOG(INFO) << BOLDRED << "Setting Latency" << RESET;
    for(auto cOpticalGroup: *pBoard)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            for(auto cChip: *cHybrid)
            {
                uint16_t cValueInMemory = 0;
                if(cChip->getFrontEndType() == FrontEndType::MPA2) { cValueInMemory = (cChip->getReg("MemoryControl_2_ALL") & 0x1) << 8 | cChip->getReg("MemoryControl_1_ALL"); }
                else if(cChip->getFrontEndType() == FrontEndType::SSA2) { cValueInMemory = ((cChip->getReg("control_1") & (1 << 4)) >> 4) << 8 | cChip->getReg("control_3"); }
                else if(cChip->getFrontEndType() == FrontEndType::CBC3)
                {
                    auto cRegValueFirst  = cChip->getReg("FeCtrl&TrgLat2");
                    auto cRegValueSecond = cChip->getReg("TriggerLatency1");
                    cValueInMemory       = ((cRegValueFirst & 0x1) << 8) | cRegValueSecond;
                }
                if(cChip->getFrontEndType() == FrontEndType::SSA2 || cChip->getFrontEndType() == FrontEndType::MPA2 || cChip->getFrontEndType() == FrontEndType::CBC3)
                {
                    fReadoutChipInterface->WriteChipReg(cChip, "TriggerLatency", cValueInMemory);
                    uint16_t cValueInChip = fReadoutChipInterface->ReadChipReg(cChip, "TriggerLatency");
                    LOG(INFO) << BOLDYELLOW << "Chip # " << +cChip->getId() << " Set TriggerLatency register to " << cValueInMemory << "=" << cValueInChip << RESET;
                }
                if(cChip->getFrontEndType() == FrontEndType::MPA2)
                {
                    cValueInMemory = cChip->getReg("Control_1");
                    fReadoutChipInterface->WriteChipReg(cChip, "Control_1", cValueInMemory);
                    uint16_t cValueInChip = fReadoutChipInterface->ReadChipReg(cChip, "Control_1");
                    LOG(INFO) << BOLDYELLOW << "Chip # " << +cChip->getId() << " Set Control_1 register for retimePix to 0x" << std::hex << cValueInMemory << "=0x" << cValueInChip << std::dec
                              << RESET;
                }
            }
        }
    }
    LOG(INFO) << BOLDRED << "Setting Latency" << RESET;

    // configure HIPs
    LOG(INFO) << BOLDRED << "Setting HIP for all chips except MPA2 that are set above" << RESET;
    for(auto cOpticalGroup: *pBoard)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            for(auto cChip: *cHybrid)
            {
                std::string cRegName;
                if(cChip->getFrontEndType() == FrontEndType::SSA2) { cRegName = "StripControl2"; }
                else if(cChip->getFrontEndType() == FrontEndType::MPA2)
                {
                    continue; // it is done above!
                }
                else if(cChip->getFrontEndType() == FrontEndType::CBC3) { cRegName = "HIP&TestMode"; }
                if(cChip->getFrontEndType() == FrontEndType::SSA2 || cChip->getFrontEndType() == FrontEndType::MPA2)
                {
                    uint16_t cValueInMemory = cChip->getReg(cRegName);
                    fReadoutChipInterface->WriteChipReg(cChip, cRegName, cValueInMemory);
                    uint16_t cValueInChip = fReadoutChipInterface->ReadChipReg(cChip, cRegName);
                    LOG(INFO) << BOLDYELLOW << "Chip # " << +cChip->getId() << " Set HIP register " << cRegName << " to 0x" << std::hex << cValueInMemory << "=0x" << cValueInChip << std::dec << RESET;
                }
            }
        }
    }
    LOG(INFO) << BOLDRED << "Done Setting HIP" << RESET;

    // configure sampling delay
    LOG(INFO) << BOLDRED << "Setting Sampling Delay" << RESET;
    for(auto cOpticalGroup: *pBoard)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            for(auto cChip: *cHybrid)
            {
                std::vector<std::string> cRegNames;
                if(cChip->getFrontEndType() == FrontEndType::SSA2) { cRegNames = {"ClockDeskewing_coarse", "ClockDeskewing_fine"}; }
                else if(cChip->getFrontEndType() == FrontEndType::MPA2) { cRegNames = {"Control_1", "ConfDLL"}; }
                if(cChip->getFrontEndType() == FrontEndType::SSA2 || cChip->getFrontEndType() == FrontEndType::MPA2)
                {
                    for(auto cRegName: cRegNames)
                    {
                        uint16_t cValueInMemory = cChip->getReg(cRegName);
                        fReadoutChipInterface->WriteChipReg(cChip, cRegName, cValueInMemory);
                        uint16_t cValueInChip = fReadoutChipInterface->ReadChipReg(cChip, cRegName);
                        LOG(INFO) << BOLDYELLOW << "Chip # " << +cChip->getId() << " Set sampling delay register " << cRegName << " to 0x" << std::hex << cValueInMemory << "=0x" << cValueInChip
                                  << std::dec << RESET;
                    }
                }
            }
        }
    }
    LOG(INFO) << BOLDRED << "Done Setting Sampling Delay" << RESET;

    // make sure MPAs have both modes enabled
    // and that the disabled strips in the SSAs are really disabled
    LOG(INFO) << BOLDRED << "Enabling pixel strips" << RESET;
    for(auto cOpticalGroup: *pBoard)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            for(auto cChip: *cHybrid)
            {
                std::string cRegName = "";
                if(cChip->getFrontEndType() == FrontEndType::MPA2)
                {
                    std::string cRegName       = "ENFLAGS_ALL";
                    uint16_t    cValueInMemory = cChip->getReg(cRegName);
                    fReadoutChipInterface->WriteChipReg(cChip, cRegName, cValueInMemory, false); // Pixel Enable register - table 8 MPA2 manual
                    uint16_t cValueInChip = fReadoutChipInterface->ReadChipReg(cChip, cRegName);
                    LOG(INFO) << BOLDYELLOW << "Chip # " << +cChip->getId() << " Set enable pixels register " << cRegName << " to 0x" << std::hex << cValueInMemory << "=0x" << cValueInChip << std::dec
                              << RESET;
                }
                if(cChip->getFrontEndType() == FrontEndType::SSA2)
                {
                    for(size_t cIndx = 0; cIndx < NSSACHANNELS; cIndx++)
                    {
                        std::stringstream cRegName;
                        cRegName << "ENFLAGS_S" << +(cIndx + 1);                                // StripControl1 in SSA2 manual
                        fReadoutChipInterface->WriteChipReg(cChip, cRegName.str(), 0x1, false); // THIS ENABLES!
                    }
                }
            }
        }
    }
    LOG(INFO) << BOLDRED << "Done Enabling pixel strips" << RESET;

    // make sure all CBC logic registers are re-configured
    for(auto cOpticalGroup: *pBoard)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            for(auto cChip: *cHybrid)
            {
                if(cChip->getFrontEndType() != FrontEndType::CBC3) continue;

                std::vector<std::string> cRegNames{"TestPulsePotNodeSel",
                                                   "MiscTestPulseCtrl&AnalogMux",
                                                   "HIP&TestMode",
                                                   "Pipe&StubInpSel&Ptwidth",
                                                   "CoincWind&Offset34",
                                                   "CoincWind&Offset12",
                                                   "LayerSwap&CluWidth",
                                                   "40MhzClk&Or254"};
                for(auto cRegName: cRegNames)
                {
                    auto cValueInMemory = cChip->getReg(cRegName);
                    fReadoutChipInterface->WriteChipReg(cChip, cRegName, cValueInMemory);
                    LOG(DEBUG) << BOLDMAGENTA << "Configuring CBC#" << +cChip->getId() << " register " << cRegName << " to 0x" << std::hex << +cValueInMemory << std::dec << RESET;
                }
                // masks
                for(auto cMapItem: cChip->getRegMap())
                {
                    if(cMapItem.first.find("MaskChannel") != std::string::npos)
                    {
                        auto cValueInMemory = cChip->getReg(cMapItem.first);
                        fReadoutChipInterface->WriteChipReg(cChip, cMapItem.first, cValueInMemory);
                        LOG(DEBUG) << BOLDMAGENTA << "Configuring CBC#" << +cChip->getId() << " register " << cMapItem.first << " to 0x" << std::hex << +cValueInMemory << std::dec << RESET;
                    }
                }
            }
        }
    }
}
