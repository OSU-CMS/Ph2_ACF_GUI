/*!OA
  \file                  RD53eudaqProducer.h
  \brief                 Implementaion of EUDAQ producer
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53eudaqProducer.h"

using namespace Ph2_HwInterface;

void RD53eudaqProducer::DoReset()
{
    RD53sysCntrPhys.Stop();
    RD53eudaqProducer::DoTerminate();
}

void RD53eudaqProducer::DoInitialise() {}

void RD53eudaqProducer::DoConfigure() { RD53sysCntrPhys.localConfigure("", -1); }

void RD53eudaqProducer::DoStartRun()
{
    // #################################
    // # Reconfigure all readout chips #
    // #################################
    LOG(INFO) << GREEN << "[RD53eudaqProducer::OnStartRun] Reconfiguring all readout chips" << RESET;
    for(const auto cBoard: *RD53sysCntrPhys.fDetectorContainer)
        for(auto cOpticalGroup: *cBoard)
            for(auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid) static_cast<Ph2_HwInterface::RD53Interface*>(RD53sysCntrPhys.fReadoutChipInterface)->ConfigureChip(cChip);

    RD53sysCntrPhys.theCurrentRun = GetRunNumber();
    swTrigCnt                     = 0;
    previousTLUTrigId             = 0;

    // #############################
    // # Add extra event if needed #
    // #############################
    // ev = eudaq::Event::MakeUnique(EUDAQ::EVENT);
    // ev->SetTriggerN(swTrigCnt++);
    // RD53eudaqProducer::MySendEvent(std::move(ev));

    // ###################################################
    // # Get configuration directly from EUDAQ framework #
    // ###################################################
    std::string fileName("Run" + RD53Shared::fromInt2Str(RD53sysCntrPhys.theCurrentRun) + "_Physics");
    RD53sysCntrPhys.initializeFiles(fileName, "Physics", RD53sysCntrPhys.histos, RD53sysCntrPhys.theCurrentRun);
    StartInfo theStartInfo;
    theStartInfo.setRunNumber(RD53sysCntrPhys.theCurrentRun);
    RD53sysCntrPhys.Start(theStartInfo);

    doExit = false;
}

void RD53eudaqProducer::DoStopRun()
{
    RD53sysCntrPhys.Stop();

    // #####################
    // # Send a EORE event #
    // #####################
    auto ev = eudaq::Event::MakeUnique(EUDAQ::EVENT);
    ev->SetEORE();
    RD53eudaqProducer::MySendEvent(std::move(ev));

    // ###########################
    // # Copy configuration file #
    // ###########################
    const auto configFileBasename = configFile.substr(configFile.find_last_of("/\\") + 1);
    const auto outputConfigFile   = std::string(RD53Shared::RESULTDIR) + "/Run" + RD53Shared::fromInt2Str(RD53sysCntrPhys.theCurrentRun) + "_" + configFileBasename;
    system(("cp " + configFile + " " + outputConfigFile).c_str());

    // #####################
    // # Update run number #
    // #####################
    std::ofstream fileRunNumberOut;
    RD53sysCntrPhys.theCurrentRun++;
    fileRunNumberOut.open(EUDAQ::FILERUNNUMBER, std::ios::out);
    if(fileRunNumberOut.is_open() == true) fileRunNumberOut << RD53Shared::fromInt2Str(RD53sysCntrPhys.theCurrentRun) << std::endl;
    fileRunNumberOut.close();

    RD53eudaqProducer::DoTerminate();
}

void RD53eudaqProducer::DoTerminate()
{
    std::unique_lock<std::recursive_mutex> theGuard(theMtx);
    doExit = true;
    theGuard.unlock();
    wakeUp.notify_one();
}

void RD53eudaqProducer::RunLoop()
{
    std::unique_lock<std::recursive_mutex> theGuard(theMtx);
    wakeUp.wait(theGuard, [this]() { return doExit; });
}

void RD53eudaqProducer::Creator(Ph2_System::SystemController& RD53SysCntr, const std::string& fileName)
{
    configFile = fileName;
    doExit     = false;
    RD53sysCntrPhys.Inherit(&RD53SysCntr);
    RD53sysCntrPhys.setGenericEvtConverter(RD53eudaqProducer::RD53eudaqEvtConverter(this));
}

void RD53eudaqProducer::MainLoop() const
{
    while(this->IsConnected() == true) std::this_thread::sleep_for(std::chrono::milliseconds(EUDAQ::WAIT));
}

void RD53eudaqProducer::MySendEvent(eudaq::EventSP theEvent)
{
    const auto MAXATTEMPTS = 2;
    for(auto i = 0; i < MAXATTEMPTS; i++)
    {
        try
        {
            this->SendEvent(theEvent);
            break;
        }
        catch(...)
        {
            std::cout << "[RD53eudaqProducer::MySendEvent] Resource unavailable, attempt " << i + 1 << "/" << MAXATTEMPTS << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(EUDAQ::WAIT));
    }
}

void RD53eudaqProducer::AddBoreInfoToEvent(eudaq::Event& ev) const
{
    // ################################################################
    // # Add Ph2-ACF configuration and extra information to the event #
    // ################################################################

    ev.SetBORE();

    ev.SetTag("Dataformat version", CMSITEventData::DataFormatVersion);
    ev.SetTag("Configuration file", "\n" + RD53sysCntrPhys.fParsedFile.str());

    for(const auto cBoard: *(RD53sysCntrPhys.fDetectorContainer))
    {
        std::stringstream header;
        header << "Firmware version: B" << cBoard->getId();
        ev.SetTag(header.str().c_str(), static_cast<RD53FWInterface*>(RD53sysCntrPhys.fBeBoardFWMap.at(cBoard->getId()))->getBoardInfo());

        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    std::stringstream header;
                    std::stringstream chipData = cChip->getRegMapStream();
                    header << "Register map and mask: B" << cBoard->getId() << "_O" << cOpticalGroup->getId() << "_H" << cHybrid->getId() << "_C" << +cChip->getId();
                    ev.SetTag(header.str().c_str(), "\n" + chipData.str());
                }
    }
}

void RD53eudaqProducer::RD53eudaqEvtConverter::operator()(const std::vector<Ph2_HwInterface::RD53Event>& RD53EvtList) const
{
    // #######################################################################################################################
    // # EUDAQ event parameters                                                                                              #
    // #######################################################################################################################
    // # - EventN:   is computed in the software                                                                             #
    // # - TriggerN: are typically assigned by the TLU to the event data, and TriggerN is typically used to build events     #
    // # Running ./euCliCollector                                                                                            #
    // # If we build events online with EventIDSyncDataCollector data collector then the building is done with EventN        #
    // # If we build events online with DirectSaveDataCOllector data collector then the building is done with TriggerN       #
    // # If we analyze with Corryvreckan, we unpack all events and subevents and look at either their timestamps or TriggerN #
    // #######################################################################################################################

    if(RD53EvtList.size() != 0)
    {
        size_t it = 0;
        while(it < RD53EvtList.size())
        {
            auto                      ev         = eudaq::Event::MakeUnique(EUDAQ::EVENT);
            auto                      eudaqEvent = static_cast<eudaq::RawEvent*>(ev.get());
            auto                      tluTrigId  = RD53EvtList[it].tlu_trigger_id;
            CMSITEventData::EventData theEvent{
                std::time(nullptr), static_cast<uint32_t>(eudaqProducer->RD53sysCntrPhys.nTRIGxEvent), RD53EvtList[it].l1a_counter, RD53EvtList[it].tdc, RD53EvtList[it].bx_counter, tluTrigId, {}};

            // #########################
            // # Add BORE if 1st event #
            // #########################
            if(eudaqProducer->GetEventN() == 0) eudaqProducer->AddBoreInfoToEvent(*ev);

            // ########################################################
            // # @TMP@ : choose between internal vs TLU event counter #
            // ########################################################
            // ev->SetTriggerN(eudaqProducer->swTrigCnt++); // Use internal counter
            // Use TLU counter
            if(tluTrigId < eudaqProducer->previousTLUTrigId)
            {
                eudaqProducer->swTrigCnt += 1 << EUDAQ::NBIT_TLU;
                std::cout << "[RD53eudaqProducer::RD53eudaqEvtConverter] Detected TLU trigger ID wrap around" << std::endl;
            }
            ev->SetTriggerN(eudaqProducer->swTrigCnt + tluTrigId);

            // ##################################################
            // # Collect all hits that have same TLU trigger ID #
            // ##################################################
            do {
                for(const auto& event: RD53EvtList[it].chip_events)
                {
                    std::string chipType = "unknown";
                    for(const auto& cHybrid: *(eudaqProducer->RD53sysCntrPhys.fDetectorContainer->getFirstObject()->getFirstObject()))
                        for(const auto& cChip: *cHybrid)
                            if((cHybrid->getId() == event.hybrid_id) && (cChip->getId() == event.chip_id)) chipType = static_cast<Ph2_HwDescription::RD53*>(cChip)->getComment();

                    theEvent.chipData.push_back({chipType, event.chip_id, event.chip_id_mod4, event.chip_lane, event.hybrid_id, event.trigger_id, event.trigger_tag, event.bc_id, {}});
                    for(const auto& hit: event.hit_data) theEvent.chipData.back().hits.push_back({hit.row, hit.col, hit.tot});
                }

                it++;
            } while((it < RD53EvtList.size()) && (RD53EvtList[it].tlu_trigger_id == tluTrigId));

            // #################
            // # Serialization #
            // #################
            std::ostringstream              theSerialized;
            boost::archive::binary_oarchive theArchive(theSerialized);
            theArchive << theEvent;
            const std::string& theStream = theSerialized.str();

            eudaqEvent->AddBlock(0, theStream.c_str(), theStream.size());
            eudaqProducer->MySendEvent(std::move(ev));

            eudaqProducer->previousTLUTrigId = tluTrigId;
        }
    }
}

namespace
{
auto dummy0 = eudaq::Factory<eudaq::Producer>::Register<RD53eudaqProducer, const std::string&, const std::string&>(RD53eudaqProducer::m_id_factory);
}
