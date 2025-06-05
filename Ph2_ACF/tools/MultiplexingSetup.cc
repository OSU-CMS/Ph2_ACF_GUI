#include "MultiplexingSetup.h"
#include "HWInterface/D19cMuxBackplaneFWInterface.h"
using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

// initialize the static member

MultiplexingSetup::MultiplexingSetup() : Tool()
{
    fAvailableCards = 0;
    fAvailable.clear();
}

MultiplexingSetup::~MultiplexingSetup() {}

void MultiplexingSetup::Initialise()
{
    // If I do this here.. DLL does not lock
    for(auto cBoard: *fDetectorContainer)
    {
        if(cBoard->isOptical()) continue;

        auto cBeBoard      = static_cast<BeBoard*>(cBoard);
        bool cSetupScanned = (fBeBoardInterface->ReadBoardReg(cBeBoard, "fc7_daq_stat.physical_interface_block.multiplexing_bp.setup_scanned") == 1);
        // if its not been scanned.. then send a reset
        if(cSetupScanned) { LOG(INFO) << BOLDBLUE << "Set-up has already been scanned..." << RESET; }
        else
        {
            LOG(INFO) << BOLDBLUE << "Set-up has not been scanned..." << RESET;
            LOG(INFO) << BOLDBLUE << "To send a global reset to the FC7 use mux_scan... " << RESET;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        // Interlock switch feature control.
        // The Interlock feature is set when the tool is initialized
        fInterlockEnabled = (this->findValueInSettings<double>("InterlockEnabled") == (double)1.0);
        if(fInterlockEnabled)
        {
            LOG(INFO) << "Enabling the interlock feature. Now controlled by the interlock switch" << RESET;
            fBeBoardInterface->WriteBoardReg(cBeBoard, "fc7_daq_cnfg.physical_interface_block.multiplexing_bp.interlock_switch_feature", 0x1);
        }
        else
        {
            LOG(INFO) << "Disabling the interlock feature. Now controlled by the Backend" << RESET;
            fBeBoardInterface->WriteBoardReg(cBeBoard, "fc7_daq_cnfg.physical_interface_block.multiplexing_bp.interlock_switch_feature", 0x0);
            fBeBoardInterface->WriteBoardReg(cBeBoard, "fc7_daq_cnfg.physical_interface_block.multiplexing_bp.interlock_switch_output", 0x1);
        }
    }
}

// Scan multiplexing set-up
void MultiplexingSetup::Scan()
{
    for(auto cBoard: *fDetectorContainer)
    {
        if(cBoard->isOptical()) continue;
        uint16_t theBoardId = static_cast<BeBoard*>(cBoard)->getId();
        LOG(INFO) << BOLDBLUE << "Scanning all available backplanes and cards on BeBoard " << +theBoardId << RESET;
        D19cMuxBackplaneFWInterface* cInterface = static_cast<D19cMuxBackplaneFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard));
        fAvailableCards                         = cInterface->ScanMultiplexingSetup();
        parseAvailable(false);
        printAvailableCards();
    }
}

// Disconnect multiplexing set-up
void MultiplexingSetup::Disconnect()
{
    for(auto cBoard: *fDetectorContainer)
    {
        if(cBoard->isOptical()) continue;
        uint16_t theBoardId = static_cast<BeBoard*>(cBoard)->getId();
        LOG(INFO) << BOLDBLUE << "Disconnecting all backplanes and cards on BeBoard " << +theBoardId << RESET;
        fBeBoardInterface->getBoardInfo(static_cast<BeBoard*>(cBoard));
        D19cMuxBackplaneFWInterface* cInterface = static_cast<D19cMuxBackplaneFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard));
        cInterface->DisconnectMultiplexingSetup();
    }
}
void MultiplexingSetup::ConfigureSingleCard(uint8_t pBackPlaneId, uint8_t pCardId)
{
    for(auto cBoard: *fDetectorContainer)
    {
        if(cBoard->isOptical()) continue;
        uint16_t theBoardId = static_cast<BeBoard*>(cBoard)->getId();
        LOG(INFO) << BOLDBLUE << "Configuring backplane " << +pBackPlaneId << " card " << +pCardId << " on BeBoard " << +theBoardId << RESET;
        D19cMuxBackplaneFWInterface* cInterface = static_cast<D19cMuxBackplaneFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard));
        cInterface->ConfigureMultiplexingSetup(pBackPlaneId, pCardId, fInterlockEnabled);
        parseAvailable();
        printAvailableCards();
    }
}
void MultiplexingSetup::ConfigureAll()
{
    for(auto cBoard: *fDetectorContainer)
    {
        if(cBoard->isOptical()) continue;
        uint16_t theBoardId = static_cast<BeBoard*>(cBoard)->getId();
        LOG(INFO) << BOLDBLUE << "Configuring all cards on BeBoard " << +theBoardId << RESET;
        D19cMuxBackplaneFWInterface* cInterface = static_cast<D19cMuxBackplaneFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard));
        fAvailableCards                         = cInterface->ScanMultiplexingSetup();
        parseAvailable(false);
        printAvailableCards();
        for(const auto& el: fAvailable)
        {
            int         cBackPlaneId = el.first;
            const auto& cCardIds     = el.second;
            for(auto cCardId: cCardIds) this->ConfigureSingleCard(cBackPlaneId, cCardId);
        }
    }
}
void MultiplexingSetup::Power(bool pEnable)
{
    for(auto cBoard: *fDetectorContainer)
    {
        if(cBoard->isOptical()) continue;
        uint16_t theBoardId = static_cast<BeBoard*>(cBoard)->getId();
        LOG(INFO) << BOLDBLUE << "Powering FMCs on " << +theBoardId << RESET;
        // static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard))->InitFMCPower();
    }
}

void MultiplexingSetup::printAvailableCards()
{
    for(auto const& itBPCard: fAvailable)
    {
        std::stringstream sstr;
        if(itBPCard.second.empty())
            sstr << "No cards";
        else
            for(auto const& itCard: itBPCard.second) sstr << itCard << " ";
        LOG(INFO) << BLUE << "Available cards for bp " << itBPCard.first << ":"
                  << "[ " << sstr.str() << "]" << RESET;
    }
}
std::map<int, std::vector<int>> MultiplexingSetup::getAvailableCards(bool filterBoardsWithoutCards)
{
    parseAvailable(filterBoardsWithoutCards);
    return fAvailable;
}

// State machine control functions
void MultiplexingSetup::Running()
{
    LOG(INFO) << "Starting Multiplexing set-up";
    Initialise();
}

void MultiplexingSetup::Stop()
{
    LOG(INFO) << "Stopping  Multiplexing set-up";
    // writeObjects();
    dumpConfigFiles();
    Destroy();
}

void MultiplexingSetup::Pause() {}

void MultiplexingSetup::Resume() {}
