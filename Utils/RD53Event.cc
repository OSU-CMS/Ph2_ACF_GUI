/*!
  \file                  RD53Event.cc
  \brief                 RD53Event class implementation
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53Event.h"
#include "HWDescription/RD53A.h"
#include "HWDescription/RD53B.h"

#ifdef __USE_ROOT__
#include "TFile.h"
#include "TTree.h"
#endif

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
void RD53Event::addBoardInfo2Events(const BeBoard* pBoard, std::vector<RD53Event>& decodedEvents)
{
    for(auto& evt: decodedEvents)
        for(auto& chip_event: evt.chip_events)
        {
            int chip_id = RD53Event::lane2chipId(pBoard, chip_event.hybrid_id, chip_event.chip_lane);
            if(chip_id != -1) chip_event.chip_id = chip_id;
        }
}

void RD53Event::fillDataContainer(BoardDataContainer* boardContainer, const std::shared_ptr<ChannelGroupBase> testChannelGroup)
{
    for(const auto& cOpticalGroup: *boardContainer)
        for(const auto& cHybrid: *cOpticalGroup)
            for(const auto& cChip: *cHybrid) RD53Event::fillChipDataContainer(cChip, testChannelGroup, cHybrid->getId());
}

void RD53Event::fillChipDataContainer(ChipDataContainer* chipContainer, const std::shared_ptr<ChannelGroupBase> testChannelGroup, uint8_t hybridId)
{
    bool   vectorRequired = chipContainer->isSummaryContainerType<Summary<GenericDataVector, OccupancyAndPh>>();
    size_t chipIndx;

    if(((RD53Event::weakCheckDataStatus == false) && (eventStatus == RD53FWEvtEncoder::GOOD) && (RD53Event::isHittedChip(hybridId, chipContainer->getId(), chipIndx) == true)) ||
       ((RD53Event::weakCheckDataStatus == true) && (RD53Event::isHittedChip(hybridId, chipContainer->getId(), chipIndx) == true) && (chip_events[chipIndx].eventStatus == RD53FWEvtEncoder::GOOD)))
    {
        if(vectorRequired == true)
        {
            chipContainer->getSummary<GenericDataVector, OccupancyAndPh>().data1.push_back(chip_events[chipIndx].bc_id);
            chipContainer->getSummary<GenericDataVector, OccupancyAndPh>().data2.push_back(chip_events[chipIndx].trigger_id);
        }

        for(const auto& hit: chip_events[chipIndx].hit_data)
        {
            chipContainer->getChannel<OccupancyAndPh>(hit.row, hit.col).fOccupancy++;
            chipContainer->getChannel<OccupancyAndPh>(hit.row, hit.col).fPh += static_cast<float>(hit.tot);
            chipContainer->getChannel<OccupancyAndPh>(hit.row, hit.col).fPhError += static_cast<float>(hit.tot * hit.tot);
            if(testChannelGroup->isChannelEnabled(hit.row, hit.col) == false) chipContainer->getChannel<OccupancyAndPh>(hit.row, hit.col).readoutError = true;
        }
    }
}

bool RD53Event::isHittedChip(uint8_t hybrid_id, uint8_t chip_id, size_t& chipIndx) const
{
    auto it = std::find_if(
        chip_events.begin(), chip_events.end(), [&](const RD53ChipEvent& event) { return ((event.hybrid_id == hybrid_id) && (event.chip_id == chip_id) && (event.hit_data.size() != 0)); });

    if(it == chip_events.end()) return false;
    chipIndx = it - chip_events.begin();

    return true;
}

int RD53Event::lane2chipId(const BeBoard* pBoard, uint8_t hybrid_id, uint8_t chip_lane)
{
    // #######################################################
    // # Translate lane to chip ID                           #
    // # Based on the assumption that the hybridId is unique #
    // #######################################################
    if(pBoard != nullptr)
    {
        for(const auto cOpticalGroup: *pBoard)
        {
            auto hybrid = std::find_if(cOpticalGroup->begin(), cOpticalGroup->end(), [&](HybridContainer* cHybrid) { return cHybrid->getId() == hybrid_id; });
            if(hybrid != cOpticalGroup->end())
            {
                auto it = std::find_if((*hybrid)->begin(), (*hybrid)->end(), [&](ChipContainer* pChip) { return static_cast<RD53*>(pChip)->getChipLane() == chip_lane; });

                if(it != (*hybrid)->end()) return (*it)->getId();
            }
        }
    }

    return -1; // Chip not found
}

void RD53Event::clearEventContainer(BeBoard& theBoard, DetectorDataContainer& theContainer)
{
    for(const auto cOpticalGroup: *theContainer.getObject(theBoard.getId()))
        for(const auto cHybrid: *cOpticalGroup)
            for(const auto cChip: *cHybrid)
            {
                for(auto row = 0u; row < RD53Shared::firstChip->getNRows(); row++)
                    for(auto col = 0u; col < RD53Shared::firstChip->getNCols(); col++)
                    {
                        cChip->getChannel<OccupancyAndPh>(row, col).fOccupancy   = 0;
                        cChip->getChannel<OccupancyAndPh>(row, col).fPh          = 0;
                        cChip->getChannel<OccupancyAndPh>(row, col).fPhError     = 0;
                        cChip->getChannel<OccupancyAndPh>(row, col).readoutError = false;
                    }

                cChip->getSummary<GenericDataVector, OccupancyAndPh>().data1.clear();
                cChip->getSummary<GenericDataVector, OccupancyAndPh>().data2.clear();
            }
}

// ##########################################
// # Event static data member instantiation #
// ##########################################
std::vector<RD53Event> RD53Event::decodedEvents;
bool                   RD53Event::weakCheckDataStatus(false);

std::vector<std::thread>            RD53Event::decodingThreads;
std::vector<std::vector<RD53Event>> RD53Event::vecEvents(RD53Shared::NTHREADS);
std::vector<std::vector<size_t>>    RD53Event::vecEventStart(RD53Shared::NTHREADS);
std::vector<uint32_t>               RD53Event::vecEventStatus(RD53Shared::NTHREADS);
std::vector<std::atomic<bool>>      RD53Event::vecWorkDone(RD53Shared::NTHREADS);
std::vector<uint32_t>*              RD53Event::theData;

std::condition_variable RD53Event::thereIsWork2Do;
std::atomic<bool>       RD53Event::keepDecodersRunning(false);
std::atomic<bool>       RD53Event::silentRunning(false);
std::mutex              RD53Event::theMtx;

void RD53Event::PrintEvents(const std::vector<RD53Event>& events, const std::vector<uint32_t>& pData)
{
    // ##################
    // # Print raw data #
    // ##################
    if(pData.size() != 0)
        for(auto j = 0u; j < pData.size(); j++)
        {
            if(j % NWORDS_DDR3 == 0) std::cout << std::dec << j << ":\t";
            std::cout << std::hex << std::setfill('0') << std::setw(8) << pData[j] << "\t";
            if(j % NWORDS_DDR3 == NWORDS_DDR3 - 1) std::cout << std::endl;
        }

    // ############################
    // # Print decoded event data #
    // ############################
    for(auto i = 0u; i < events.size(); i++)
    {
        auto& evt = events[i];
        LOG(INFO) << BOLDGREEN << "===========================" << RESET;
        LOG(INFO) << BOLDGREEN << "EVENT           = " << i << RESET;
        LOG(INFO) << BOLDGREEN << "block_size      = " << evt.block_size << RESET;
        LOG(INFO) << BOLDGREEN << "tlu_trigger_id  = " << evt.tlu_trigger_id << RESET;
        LOG(INFO) << BOLDGREEN << "trigger_tag     = " << evt.trigger_tag << RESET;
        LOG(INFO) << BOLDGREEN << "tdc             = " << evt.tdc << RESET;
        LOG(INFO) << BOLDGREEN << "l1a_counter     = " << evt.l1a_counter << RESET;
        LOG(INFO) << BOLDGREEN << "bx_counter      = " << evt.bx_counter << RESET;

        // ###########################
        // # Print decoded chip data #
        // ###########################
        for(auto& event: evt.chip_events)
        {
            LOG(INFO) << CYAN << "------- Chip Header -------" << RESET;
            LOG(INFO) << CYAN << "error_code      = " << event.error_code << RESET;
            LOG(INFO) << CYAN << "hybrid_id       = " << event.hybrid_id << RESET;
            LOG(INFO) << CYAN << "chip_lane       = " << event.chip_lane << RESET;
            LOG(INFO) << CYAN << "l1a_size        = " << event.l1a_size << RESET;
            LOG(INFO) << CYAN << "chip_type       = " << event.chip_type << RESET;
            LOG(INFO) << CYAN << "frame_delay     = " << event.frame_delay << RESET;

            LOG(INFO) << CYAN << "chip_id_mod4    = " << event.chip_id_mod4 << RESET;
            LOG(INFO) << CYAN << "trigger_id      = " << event.trigger_id << RESET;
            LOG(INFO) << CYAN << "trigger_tag     = " << event.trigger_tag << RESET;
            LOG(INFO) << CYAN << "bc_id           = " << event.bc_id << RESET;

            LOG(INFO) << BOLDYELLOW << "--- Hit Data (" << event.hit_data.size() << " hits) ---" << RESET;

            for(const auto& hit: event.hit_data)
                LOG(INFO) << BOLDYELLOW << "Column: " << std::setw(3) << hit.col << std::setw(-1) << ", Row: " << std::setw(3) << hit.row << std::setw(-1) << ", ToT: " << std::setw(3) << +hit.tot
                          << std::setw(-1) << RESET;
        }

        LOG(INFO) << BOLDGREEN << "===========================" << RESET;
        LOG(INFO) << BOLDGREEN << "EVENT STATUS^   = " << evt.eventStatus << RESET;
        RD53Event::EvtErrorHandler(evt.eventStatus);
    }
}

bool RD53Event::EvtErrorHandler(uint32_t status)
{
    bool isGood = true;

    if(status & RD53FWEvtEncoder::EVSIZE)
    {
        LOG(ERROR) << BOLDRED << "Invalid event size " << BOLDYELLOW << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53FWEvtEncoder::EMPTY)
    {
        LOG(ERROR) << BOLDRED << "No data collected " << BOLDYELLOW << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53FWEvtEncoder::NOEVHEADER)
    {
        LOG(ERROR) << BOLDRED << "No event headear found in data " << BOLDYELLOW << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53FWEvtEncoder::INCOMPLETE)
    {
        LOG(ERROR) << BOLDRED << "Incomplete event header " << BOLDYELLOW << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53FWEvtEncoder::L1A)
    {
        LOG(ERROR) << BOLDRED << "L1A counter mismatch " << BOLDYELLOW << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53FWEvtEncoder::TRGTAG_ER1)
    {
        LOG(ERROR) << BOLDRED << "Trigger tag counter mismatch " << BOLDYELLOW << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53FWEvtEncoder::TRGTAG_ER2)
    {
        LOG(ERROR) << BOLDRED << "Trigger tag single bit-flip detected in tag symbol of a trigger command " << BOLDYELLOW << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53FWEvtEncoder::TRGTAG_ER3)
    {
        LOG(ERROR) << BOLDRED << "Trigger tag unrecognized tag symbol " << BOLDYELLOW << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53FWEvtEncoder::NOFRHEADER)
    {
        LOG(ERROR) << BOLDRED << "No frame header found in data " << BOLDYELLOW << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53FWEvtEncoder::MISSCHIP)
    {
        LOG(ERROR) << BOLDRED << "Chip data are missing " << BOLDYELLOW << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53FWEvtEncoder::CORRUPTED)
    {
        LOG(ERROR) << BOLDRED << "Corrupted event " << BOLDYELLOW << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53EvtEncoder::CHIPHEAD)
    {
        LOG(ERROR) << BOLDRED << "Invalid chip header " << BOLDYELLOW << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53EvtEncoder::CHIPID)
    {
        LOG(ERROR) << BOLDRED << "Found conflicting chip ID " << BOLDYELLOW << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53EvtEncoder::CHIPPIX)
    {
        LOG(ERROR) << BOLDRED << "Invalid pixel row or column " << BOLDYELLOW << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53EvtEncoder::CHIPTOT)
    {
        LOG(ERROR) << BOLDRED << "Invalid TOT value " << BOLDYELLOW << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53EvtEncoder::CHIPNOHIT)
    {
        LOG(ERROR) << BOLDRED << "Hit data are missing " << BOLDYELLOW << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53EvtEncoder::CHIPFWERR)
    {
        LOG(ERROR) << BOLDRED << "Firmware error " << BOLDYELLOW << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53EvtEncoder::CHIPNS_WAS0)
    {
        LOG(ERROR) << BOLDRED << "New-stream bit was 0 in the first word of the event stream (it can happen when the FW timeout fires before the chip has sent the full event) " << BOLDYELLOW
                   << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53EvtEncoder::CHIPNS_WAS1)
    {
        LOG(ERROR) << BOLDRED << "New-stream bit was 1 before the last word of the event stream (it can indicate a bug in the FW) " << BOLDYELLOW << "--> retry" << std::setfill(' ') << std::setw(8)
                   << "" << RESET;
        isGood = false;
    }

    if(status & RD53EvtEncoder::CHIPQROW)
    {
        LOG(ERROR) << BOLDRED << "Neighbor bit set for the first qrow " << BOLDYELLOW << "--> retry" << RESET;
        isGood = false;
    }

    if(status & RD53EvtEncoder::CHIPTRUNC_MAXHITS) LOG(ERROR) << BOLDRED << "Truncation occurred due to max number of hits reached per core " << BOLDYELLOW << "--> no retry" << RESET;

    if(status & RD53EvtEncoder::CHIPTRUNC_TIMEOUT) LOG(ERROR) << BOLDRED << "Truncation occurred due to readout timeout" << BOLDYELLOW << "--> no retry" << RESET;

    return isGood;
}

bool RD53Event::findEventStarts(const std::vector<uint32_t>& data, std::vector<size_t>& eventStarts, uint32_t& eventStatus)
{
    size_t i               = 0u;
    bool   firstEventFound = false;

    while(i < data.size())
        if(data[i] >> RD53FWEvtEncoder::NBIT_BLOCKSIZE == RD53FWEvtEncoder::EVT_HEADER)
        {
            eventStarts.push_back(i);
            size_t block_size;
            std::tie(block_size) = bits::unpack<RD53FWEvtEncoder::NBIT_BLOCKSIZE>(data[i]);
            i += NWORDS_DDR3 * block_size;
            firstEventFound = true;
        }
        else if(firstEventFound == false)
            i++;
        else
        {
            eventStatus |= RD53FWEvtEncoder::EVSIZE;
            return false;
        }

    if(eventStarts.size() == 0)
    {
        eventStatus |= RD53FWEvtEncoder::NOEVHEADER;
        return false;
    }

    eventStarts.push_back(data.size());

    return true;
}

void RD53Event::DecodeEvents(const std::vector<uint32_t>& data, std::vector<RD53Event>& events, const std::vector<size_t>& eventStartExt, uint32_t& eventStatus, bool doSilentRunning)
{
    std::vector<size_t> eventStartLocal;
    eventStatus = RD53FWEvtEncoder::GOOD;

    // #####################
    // # Consistency check #
    // #####################
    if(data.size() == 0)
    {
        eventStatus |= RD53FWEvtEncoder::EMPTY;
        return;
    }

    // #####################
    // # Find events start #
    // #####################
    if((eventStartExt.size() == 0) && (RD53Event::findEventStarts(data, eventStartLocal, eventStatus) == false)) return;
    const std::vector<size_t>& refEventStart = (eventStartExt.size() == 0 ? const_cast<const std::vector<size_t>&>(eventStartLocal) : eventStartExt);

    // #################################
    // # Branching for RD53A and RD53B #
    // #################################
    events.reserve(events.size() + refEventStart.size() - 1);
    if(RD53Shared::firstChip->getFrontEndType() == FrontEndType::RD53A)
    {
        try
        {
            RD53Event::DecodeRD53AEvents(data, events, refEventStart, eventStatus);
        }
        catch(std::runtime_error& e)
        {
            if(doSilentRunning == false) LOG(ERROR) << BOLDRED << "Error while decoding this datastream: " << BOLDYELLOW << e.what() << RESET;
            events.clear();
            eventStatus = RD53FWEvtEncoder::CORRUPTED;
        }
    }
    else
    {
        try
        {
            RD53Event::DecodeRD53BEvents(data, events, refEventStart, eventStatus, RD53Shared::firstChip->getDataFormatOptions());
        }
        catch(std::runtime_error& e)
        {
            if(doSilentRunning == false) LOG(ERROR) << BOLDRED << "Error while decoding this datastream: " << BOLDYELLOW << e.what() << RESET;
            events.clear();
            eventStatus = RD53FWEvtEncoder::CORRUPTED;
        }
    }
}

void RD53Event::ForkDecodingThreads()
{
    if(RD53Event::decodingThreads.size() != 0) RD53Event::JoinDecodingThreads();

    RD53Event::keepDecodersRunning = true;

    RD53Event::decodingThreads.clear();
    RD53Event::decodingThreads.reserve(RD53Shared::NTHREADS);

    for(auto& events: RD53Event::vecEvents) events.clear();
    for(auto& eventStart: RD53Event::vecEventStart) eventStart.clear();
    for(auto& eventStatus: RD53Event::vecEventStatus) eventStatus = 0;
    for(auto& workDone: RD53Event::vecWorkDone) workDone = true;

    for(auto i = 0u; i < RD53Shared::NTHREADS; i++)
        RD53Event::decodingThreads.emplace_back(std::thread(&RD53Event::decoderThread,
                                                            std::ref(RD53Event::theData),
                                                            std::ref(RD53Event::vecEvents[i]),
                                                            std::ref(RD53Event::vecEventStart[i]),
                                                            std::ref(RD53Event::vecEventStatus[i]),
                                                            std::ref(RD53Event::vecWorkDone[i])));
}

void RD53Event::JoinDecodingThreads()
{
    RD53Event::keepDecodersRunning = false;
    for(auto& workDone: RD53Event::vecWorkDone) workDone = false;
    RD53Event::thereIsWork2Do.notify_all();

    for(auto& thr: RD53Event::decodingThreads)
        if(thr.joinable() == true) thr.join();
}

void RD53Event::decoderThread(std::vector<uint32_t>*& data, std::vector<RD53Event>& events, const std::vector<size_t>& eventStart, uint32_t& eventStatus, std::atomic<bool>& workDone)
{
    while(RD53Event::keepDecodersRunning == true)
    {
        std::unique_lock<std::mutex> theGuard(RD53Event::theMtx);
        RD53Event::thereIsWork2Do.wait(theGuard, [&workDone]() { return !workDone; });
        theGuard.unlock();
        if(eventStart.size() != 0) RD53Event::DecodeEvents(*data, events, eventStart, eventStatus, silentRunning);
        workDone = true;
    }
}

void RD53Event::DecodeEventsMultiThreads(const std::vector<uint32_t>& data, std::vector<RD53Event>& events, uint32_t& eventStatus, bool doSilentRunning)
{
    std::vector<size_t> eventStart;
    silentRunning = doSilentRunning;

    // #####################
    // # Consistency check #
    // #####################
    if((RD53Event::decodingThreads.size() == 0) || (data.size() == 0))
    {
        if(RD53Event::decodingThreads.size() == 0)
            LOG(ERROR) << BOLDRED << "Threads for data decoding haven't been forked: use " << BOLDYELLOW << "RD53Event::ForkDecodingThreads()" << BOLDRED << " first" << RESET;
        eventStatus |= RD53FWEvtEncoder::EMPTY;
        return;
    }

    // #####################
    // # Find events start #
    // #####################
    if(RD53Event::findEventStarts(data, eventStart, eventStatus) == false) return;
    const auto nEvents = ceil(static_cast<double>(eventStart.size() - 1) / RD53Shared::NTHREADS);

    // ######################
    // # Unpack data vector #
    // ######################
    size_t i = 0u;
    for(i = 0u; RD53Shared::NTHREADS > 1 && i < RD53Shared::NTHREADS - 1; i++)
    {
        auto firstEvent = eventStart.begin() + nEvents * i;
        if(firstEvent + nEvents + 1 > eventStart.end() - 1) break;
        auto lastEvent = firstEvent + nEvents + 1;
        std::move(firstEvent, lastEvent, std::back_inserter(RD53Event::vecEventStart[i]));
    }
    auto firstEvent = eventStart.begin() + nEvents * i;
    auto lastEvent  = eventStart.end();
    std::move(firstEvent, lastEvent, std::back_inserter(RD53Event::vecEventStart[i]));

    // #######################
    // # Start data decoding #
    // #######################
    RD53Event::theData = const_cast<std::vector<uint32_t>*>(&data);
    for(auto& workDone: RD53Event::vecWorkDone) workDone = false;
    RD53Event::thereIsWork2Do.notify_all();
    while(true)
    {
        bool allWorkDone = true;
        for(auto& workDone: RD53Event::vecWorkDone) allWorkDone &= workDone;
        if(allWorkDone == true) break;
    }

    // #####################
    // # Pack event vector #
    // #####################
    eventStatus = 0;
    for(auto i = 0u; i < RD53Shared::NTHREADS; i++)
    {
        RD53Shared::myMove(std::move(RD53Event::vecEvents[i]), events);
        eventStatus |= RD53Event::vecEventStatus[i];
        RD53Event::vecEventStatus[i] = 0;
        RD53Event::vecEventStart[i].clear();
    }
}

// ##########################################
// # Use of OpenMP (compiler flag -fopenmp) #
// ##########################################
/*
void RD53Event::DecodeEventsMultiThreads(const std::vector<uint32_t>& data, std::vector<RD53Event>& events, uint32_t& eventStatus, bool doSilentRunning)
{
    std::vector<size_t> eventStart;
    silentRunning = doSilentRunning;
    eventStatus |= RD53FWEvtEncoder::GOOD;

    // #####################
    // # Consistency check #
    // #####################
    if(data.size() == 0)
    {
        eventStatus |= RD53FWEvtEncoder::EMPTY;
        return;
    }

    // #####################
    // # Find events start #
    // #####################
    if(RD53Event::findEventStarts(data, eventStart, eventStatus) == false) return;
    const auto nEvents = ceil(static_cast<double>(eventStart.size() - 1) / omp_get_max_threads());

    // ######################
    // # Unpack data vector #
    // ######################
#pragma omp parallel
    {
        std::vector<RD53Event> vecEvents;
        std::vector<size_t>    vecEventStart;

        if(eventStart.begin() + nEvents * omp_get_thread_num() < eventStart.end())
        {
            auto status     = eventStatus;
            auto firstEvent = eventStart.begin() + nEvents * omp_get_thread_num();
            auto lastEvent  = firstEvent + nEvents + 1 < eventStart.end() ? firstEvent + nEvents + 1 : eventStart.end();
            std::move(firstEvent, lastEvent, std::back_inserter(vecEventStart));

            // #################################
            // # Branching for RD53A and RD53B #
            // #################################
            if(RD53Shared::firstChip->getFrontEndType() == FrontEndType::RD53A)
                RD53Event::DecodeRD53AEvents(data, vecEvents, vecEventStart, status);
            else
                RD53Event::DecodeRD53BEvents(data, vecEvents, vecEventStart, status, RD53Shared::firstChip->getDataFormatOptions());

            // #####################
            // # Pack event vector #
            // #####################
#pragma omp critical
            RD53Shared::myMove(std::move(vecEvents), events);
            eventStatus |= status;
        }
    }
}
*/
// ######################
// # Specific for RD53A #
// ######################

void RD53ChipEvent::decodeChipFrame(const uint32_t data0, const uint32_t data1, RD53ChipEvent& event)
{
    std::tie(event.error_code, event.hybrid_id, event.chip_lane, event.l1a_size) =
        bits::unpack<RD53FWEvtEncoder::NBIT_ERR, RD53FWEvtEncoder::NBIT_HYBRID, RD53FWEvtEncoder::NBIT_CHIPID, RD53FWEvtEncoder::NBIT_L1ASIZE>(data0);
    std::tie(event.chip_type, event.frame_delay) = bits::unpack<RD53FWEvtEncoder::NBIT_CHIPTYPE, RD53FWEvtEncoder::NBIT_DELAY>(data1);
}

RD53Event RD53Event::DecodeRD53AEvent(const uint32_t* data, size_t n32bitsWords)
{
    RD53Event evt;

    // ######################
    // # Consistency checks #
    // ######################
    if(n32bitsWords < RD53FWEvtEncoder::EVT_HEADER_SIZE)
    {
        evt.eventStatus |= RD53FWEvtEncoder::INCOMPLETE;
        return evt;
    }

    // #########################
    // # Decode event preamble #
    // #########################
    bool dummy_size;
    std::tie(evt.block_size)                                  = bits::unpack<RD53FWEvtEncoder::NBIT_BLOCKSIZE>(data[0]);
    std::tie(evt.tlu_trigger_id, evt.trigger_tag, dummy_size) = bits::unpack<RD53FWEvtEncoder::NBIT_TRIGID, RD53FWEvtEncoder::NBIT_TRGTAG, RD53FWEvtEncoder::NBIT_DUMMY>(data[1]);
    std::tie(evt.tdc, evt.l1a_counter)                        = bits::unpack<RD53FWEvtEncoder::NBIT_TDC, RD53FWEvtEncoder::NBIT_L1ACNT>(data[2]);
    evt.bx_counter                                            = data[3];

    // ############################
    // # Search for frame lengths #
    // ############################
    std::vector<size_t> event_sizes;
    size_t              index = RD53FWEvtEncoder::EVT_HEADER_SIZE;
    while(index < n32bitsWords - dummy_size * NWORDS_DDR3)
    {
        if(data[index] >> (RD53FWEvtEncoder::NBIT_ERR + RD53FWEvtEncoder::NBIT_HYBRID + RD53FWEvtEncoder::NBIT_CHIPID + RD53FWEvtEncoder::NBIT_L1ASIZE) != RD53FWEvtEncoder::FRAME_HEADER)
        {
            evt.eventStatus |= RD53FWEvtEncoder::NOFRHEADER;
            return evt;
        }

        size_t size = (data[index] & ((1 << RD53FWEvtEncoder::NBIT_L1ASIZE) - 1)) * NWORDS_DDR3;
        event_sizes.push_back(size);
        index += size;
    }

    if(index != n32bitsWords - dummy_size * NWORDS_DDR3)
    {
        evt.eventStatus |= RD53FWEvtEncoder::MISSCHIP;
        return evt;
    }

    // ##############################
    // # Decode frame and chip data #
    // ##############################
    evt.chip_events.reserve(event_sizes.size());
    index = RD53FWEvtEncoder::EVT_HEADER_SIZE;
    for(auto size: event_sizes)
    {
        RD53ChipEvent chipEvt;

        // #########################
        // # Decode frame preamble #
        // #########################
        RD53ChipEvent::decodeChipFrame(data[index], data[index + 1], chipEvt);

        // ####################
        // # Decode chip data #
        // ####################
        RD53A::decodeChipData(&data[index + 2], size - 2, chipEvt);

        if(chipEvt.error_code != 0)
            chipEvt.eventStatus |= RD53EvtEncoder::CHIPFWERR;
        else
            evt.chip_events.push_back(std::move(chipEvt));

        evt.eventStatus |= chipEvt.eventStatus;
        index += size;
    }

    return evt;
}

void RD53Event::DecodeRD53AEvents(const std::vector<uint32_t>& data, std::vector<RD53Event>& events, const std::vector<size_t>& refEventStart, uint32_t& eventStatus)
{
    const size_t maxL1Counter = RD53Shared::setBits(RD53AEvtEncoder::NBIT_TRIGID) + 1;

    for(auto i = 0u; i < refEventStart.size() - 1; i++)
    {
        const auto start = refEventStart[i];
        const auto end   = refEventStart[i + 1];

        // #######################
        // # Decode single event #
        // #######################
        auto evt = RD53Event::DecodeRD53AEvent(&data[start], end - start);

        for(auto j = 0u; j < evt.chip_events.size(); j++)
            if(evt.l1a_counter % maxL1Counter != evt.chip_events[j].trigger_id) evt.eventStatus |= RD53FWEvtEncoder::L1A;

        events.push_back(std::move(evt));
        eventStatus |= evt.eventStatus;
    }
}

// ######################
// # Specific for RD53B #
// ######################

size_t RD53Event::DecodeRD53BEvents(const std::vector<uint32_t>& data, std::vector<RD53Event>& events, uint32_t& eventStatus, const DataFormatOptions& options)
{
    return RD53Event::DecodeRD53BEvents(&data[0], events, RD53FWEvtEncoder::NBIT_EVT_WORD * data.size(), eventStatus, options);
}

size_t
RD53Event::DecodeRD53BEvents(const std::vector<uint32_t>& data, std::vector<RD53Event>& events, const std::vector<size_t>& refEventStart, uint32_t& eventStatus, const DataFormatOptions& options)
{
    return RD53Event::DecodeRD53BEvents(&data[refEventStart[0]], events, RD53FWEvtEncoder::NBIT_EVT_WORD * (refEventStart.back() - refEventStart[0]), eventStatus, options);
}

size_t RD53Event::DecodeRD53BEvents(const uint32_t* data, std::vector<RD53Event>& events, const size_t howMany, uint32_t& eventStatus, const DataFormatOptions& options)
{
    auto         bits         = bit_view(data, 0, howMany);
    const size_t n32bitsWords = bits.size() / RD53FWEvtEncoder::NBIT_EVT_WORD;
    const size_t maxL1Counter = RD53Shared::setBits(RD53BEvtEncoder::NBIT_TRIGID * (options.enableBCID == true ? 1 : 2)) + 1;

    if(howMany == 0) eventStatus |= RD53FWEvtEncoder::EMPTY;

    while(bits.size() != 0)
    {
        RD53Event evt;

        // ######################
        // # Consistency checks #
        // ######################
        if(n32bitsWords < RD53FWEvtEncoder::EVT_HEADER_SIZE)
        {
            eventStatus |= RD53FWEvtEncoder::INCOMPLETE;
            break;
        }

        // #########################
        // # Decode event preamble #
        // #########################
        bits.skip(RD53FWEvtEncoder::NBIT_EVTHEAD);
        evt.block_size     = bits.pop(RD53FWEvtEncoder::NBIT_BLOCKSIZE);
        evt.tlu_trigger_id = bits.pop(RD53FWEvtEncoder::NBIT_TRIGID);
        evt.trigger_tag    = bits.pop(RD53FWEvtEncoder::NBIT_TRGTAG);
        size_t dummy_size  = bits.pop(RD53FWEvtEncoder::NBIT_DUMMY);
        evt.tdc            = bits.pop(RD53FWEvtEncoder::NBIT_TDC);
        evt.l1a_counter    = bits.pop(RD53FWEvtEncoder::NBIT_L1ACNT);
        evt.bx_counter     = bits.pop(RD53FWEvtEncoder::NBIT_BXCNT);

        // ##############################
        // # Decode frame and chip data #
        // ##############################
        auto event_bits = bits.pop_slice(NWORDS_DDR3 * RD53FWEvtEncoder::NBIT_EVT_WORD * (evt.block_size - 1 - dummy_size));
        while(event_bits.size() != 0)
        {
            if(event_bits.pop(RD53FWEvtEncoder::NBIT_FRAMEHEAD) != RD53FWEvtEncoder::FRAME_HEADER)
            {
                evt.eventStatus |= RD53FWEvtEncoder::NOFRHEADER;
                evt.chip_events.clear();
                break;
            }

            RD53ChipEvent chipEvt;

            // #########################
            // # Decode frame preamble #
            // #########################
            chipEvt.error_code = event_bits.pop(RD53FWEvtEncoder::NBIT_ERR);
            chipEvt.hybrid_id  = event_bits.pop(RD53FWEvtEncoder::NBIT_HYBRID);
            chipEvt.chip_lane  = event_bits.pop(RD53FWEvtEncoder::NBIT_CHIPID);
            chipEvt.l1a_size   = event_bits.pop(RD53FWEvtEncoder::NBIT_L1ASIZE);
            event_bits.skip(RD53FWEvtEncoder::NBIT_PADDING);
            event_bits.skip(RD53FWEvtEncoder::NBIT_CHIPTYPE);
            event_bits.skip(RD53FWEvtEncoder::NBIT_DELAY);

            if(chipEvt.error_code != 0)
            {
                chipEvt.eventStatus |= RD53EvtEncoder::CHIPFWERR;
                event_bits.skip(chipEvt.l1a_size * NWORDS_DDR3 * RD53FWEvtEncoder::NBIT_EVT_WORD - 64);
                continue;
            }

            // ####################
            // # Decode chip data #
            // ####################
            RD53B::decodeChipData(event_bits.pop_slice(chipEvt.l1a_size * NWORDS_DDR3 * RD53FWEvtEncoder::NBIT_EVT_WORD - 64), chipEvt, options);
            evt.eventStatus |= chipEvt.eventStatus;
            if((chipEvt.eventStatus & (RD53FWEvtEncoder::MISSCHIP | RD53EvtEncoder::CHIPNS_WAS0 | RD53EvtEncoder::CHIPNS_WAS1)) != 0) break;
            evt.chip_events.push_back(std::move(chipEvt));
        }

        bits.skip(NWORDS_DDR3 * RD53FWEvtEncoder::NBIT_EVT_WORD * dummy_size);

        if(options.enableTriggerId == true)
            for(auto j = 0u; j < evt.chip_events.size(); j++)
                if(evt.l1a_counter % maxL1Counter != evt.chip_events[j].trigger_id) evt.eventStatus |= RD53FWEvtEncoder::L1A;

        for(auto j = 0u; j < evt.chip_events.size(); j++)
        {
            if(evt.chip_events[j].trigger_tag <= RD53BEvtEncoder::MAX_TRGTAG)
            {
                if(((evt.trigger_tag + 1) % 32) != (evt.chip_events[j].trigger_tag >> 2)) evt.eventStatus |= RD53FWEvtEncoder::TRGTAG_ER1;
            }
            else if(evt.chip_events[j].trigger_tag <= RD53BEvtEncoder::MAX_TRGTAG_ERR1)
                evt.eventStatus |= RD53FWEvtEncoder::TRGTAG_ER2;
            else if(evt.chip_events[j].trigger_tag <= RD53BEvtEncoder::MAX_TRGTAG_ERR2)
                evt.eventStatus |= RD53FWEvtEncoder::TRGTAG_ER3;
        }

        events.push_back(std::move(evt));
        eventStatus |= evt.eventStatus;
    }

    return events.size();
}

bool RD53Event::MakeNtuple(const std::string& fileName, const std::vector<RD53Event>& events)
{
#ifdef __USE_ROOT__
    TFile theFile(fileName.c_str(), "RECREATE");
    TTree theTree("theTree", "Ntuple with event data");

    uint16_t FW_block_size, FW_tlu_trigger_id, FW_trigger_tag, FW_tdc;
    uint32_t event, FW_l1a_counter, FW_bx_counter, FW_event_status, FW_nframes;

    theTree.Branch("event", &event, "event/i");
    theTree.Branch("FW_block_size", &FW_block_size, "FW_block_size/i");
    theTree.Branch("FW_tlu_trigger_id", &FW_tlu_trigger_id, "FW_tlu_trigger_id/i");
    theTree.Branch("FW_trigger_tag", &FW_trigger_tag, "FW_trigger_tag/i");
    theTree.Branch("FW_tdc", &FW_tdc, "FW_tdc/i");
    theTree.Branch("FW_l1a_counter", &FW_l1a_counter, "FW_l1a_counter/i");
    theTree.Branch("FW_bx_counter", &FW_bx_counter, "FW_bx_counter/i");
    theTree.Branch("FW_event_status", &FW_event_status, "FW_event_status/i");
    theTree.Branch("FW_nframes", &FW_nframes, "FW_nframes/i");

    std::vector<uint16_t> FW_frame_event_error_code;
    std::vector<uint16_t> FW_frame_event_hybrid_id;
    std::vector<uint16_t> FW_frame_event_chip_lane;
    std::vector<uint16_t> FW_frame_event_l1a_size;
    std::vector<uint16_t> FW_frame_event_chip_type;
    std::vector<uint16_t> FW_frame_event_frame_delay;

    std::vector<uint16_t> RD53_frame_event_chip_id_mod4;
    std::vector<uint16_t> RD53_frame_event_trigger_id;
    std::vector<uint16_t> RD53_frame_event_trigger_tag;
    std::vector<uint16_t> RD53_frame_event_bc_id;
    std::vector<uint32_t> RD53_frame_event_status;
    std::vector<uint32_t> RD53_frame_event_nhits;

    theTree.Branch("FW_frame_event_error_code", &FW_frame_event_error_code);
    theTree.Branch("FW_frame_event_hybrid_id", &FW_frame_event_hybrid_id);
    theTree.Branch("FW_frame_event_chip_lane", &FW_frame_event_chip_lane);
    theTree.Branch("FW_frame_event_l1a_size", &FW_frame_event_l1a_size);
    theTree.Branch("FW_frame_event_chip_type", &FW_frame_event_chip_type);
    theTree.Branch("FW_frame_event_frame_delay", &FW_frame_event_frame_delay);

    theTree.Branch("RD53_frame_event_chip_id_mod4", &RD53_frame_event_chip_id_mod4);
    theTree.Branch("RD53_frame_event_trigger_id", &RD53_frame_event_trigger_id);
    theTree.Branch("RD53_frame_event_trigger_tag", &RD53_frame_event_trigger_tag);
    theTree.Branch("RD53_frame_event_bc_id", &RD53_frame_event_bc_id);
    theTree.Branch("RD53_frame_event_status", &RD53_frame_event_status);
    theTree.Branch("RD53_frame_event_nhits", &RD53_frame_event_nhits);

    std::vector<uint16_t> RD53_hit_row;
    std::vector<uint16_t> RD53_hit_col;
    std::vector<uint8_t>  RD53_hit_tot;

    // ###################################################################
    // # Needed to split the hits per chip: jagged aray needs dictionary #
    // ###################################################################
    // std::vector<std::vector<uint16_t>> RD53_hit_row;
    // std::vector<std::vector<uint16_t>> RD53_hit_col;
    // std::vector<std::vector<uint8_t>>  RD53_hit_tot;

    theTree.Branch("RD53_hit_row", &RD53_hit_row);
    theTree.Branch("RD53_hit_col", &RD53_hit_col);
    theTree.Branch("RD53_hit_tot", &RD53_hit_tot);

    for(auto i = 0u; i < events.size(); i++)
    {
        auto& evt = events[i];

        event             = i;
        FW_block_size     = evt.block_size;
        FW_tlu_trigger_id = evt.tlu_trigger_id;
        FW_trigger_tag    = evt.trigger_tag;
        FW_tdc            = evt.tdc;
        FW_l1a_counter    = evt.l1a_counter;
        FW_bx_counter     = evt.bx_counter;
        FW_event_status   = evt.eventStatus;
        FW_nframes        = evt.chip_events.size();

        FW_frame_event_error_code.clear();
        FW_frame_event_hybrid_id.clear();
        FW_frame_event_chip_lane.clear();
        FW_frame_event_l1a_size.clear();
        FW_frame_event_chip_type.clear();
        FW_frame_event_frame_delay.clear();

        RD53_frame_event_chip_id_mod4.clear();
        RD53_frame_event_trigger_id.clear();
        RD53_frame_event_trigger_tag.clear();
        RD53_frame_event_bc_id.clear();
        RD53_frame_event_status.clear();
        RD53_frame_event_nhits.clear();

        RD53_hit_row.clear();
        RD53_hit_col.clear();
        RD53_hit_tot.clear();

        for(auto& event: evt.chip_events)
        {
            FW_frame_event_error_code.push_back(event.error_code);
            FW_frame_event_hybrid_id.push_back(event.hybrid_id);
            FW_frame_event_chip_lane.push_back(event.chip_lane);
            FW_frame_event_l1a_size.push_back(event.l1a_size);
            FW_frame_event_chip_type.push_back(event.chip_type);
            FW_frame_event_frame_delay.push_back(event.frame_delay);

            RD53_frame_event_chip_id_mod4.push_back(event.chip_id_mod4);
            RD53_frame_event_trigger_id.push_back(event.trigger_id);
            RD53_frame_event_trigger_tag.push_back(event.trigger_tag);
            RD53_frame_event_bc_id.push_back(event.bc_id);
            RD53_frame_event_status.push_back(event.eventStatus);
            RD53_frame_event_nhits.push_back(event.hit_data.size());

            // std::vector<uint16_t> hit_rows;
            // std::vector<uint16_t> hit_cols;
            // std::vector<uint8_t>  hit_tots;

            for(const auto& hit: event.hit_data)
            {
                RD53_hit_row.push_back(hit.row);
                RD53_hit_col.push_back(hit.col);
                RD53_hit_tot.push_back(hit.tot);
                // hit_rows.push_back(hit.row);
                // hit_cols.push_back(hit.col);
                // hit_tots.push_back(hit.tot);
            }

            // RD53_hit_row.push_back(std::move(hit_rows));
            // RD53_hit_col.push_back(std::move(hit_cols));
            // RD53_hit_tot.push_back(std::move(hit_tots));
        }

        theTree.Fill();
    }

    theTree.Write();
    theFile.Close();

    return true;
#else
    LOG(WARNING) << BOLDRED << ">>> The function to translate raw data into ROOT ntuple was not compiled <<<" << RESET;
    return false;
#endif
}

} // namespace Ph2_HwInterface
