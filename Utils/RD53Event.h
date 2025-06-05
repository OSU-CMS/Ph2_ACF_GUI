/*!
  \file                  RD53Event.h
  \brief                 RD53Event description class
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53Event_H
#define RD53Event_H

#include "BitMaster/bit_packing.h"
#include "DataContainer.h"
#include "Event.h"
#include "GenericDataVector.h"

#include <atomic>
#include <condition_variable>
#include <omp.h>

namespace Ph2_HwDescription
{
struct DataFormatOptions;
} // namespace Ph2_HwDescription

// #############
// # CONSTANTS #
// #############
#define NWORDS_DDR3 4 // Number of IPbus words in a DDR3 word

// ##############################
// # RD53Event useful constants #
// ##############################
namespace RD53FWEvtEncoder
{
// ################
// # Event header #
// ################
const uint16_t EVT_HEADER      = 0xFFFF;
const uint8_t  NBIT_EVT_WORD   = 32; // Number of bits for event word
const uint8_t  EVT_HEADER_SIZE = 4;  // Number of words in event header
const uint8_t  NBIT_EVTHEAD    = 16; // Number of bits for the Error Code
const uint8_t  NBIT_BLOCKSIZE  = 16; // Number of bits for the Block Size
const uint8_t  NBIT_TRIGID     = 16; // Number of bits for the TLU Trigger ID
const uint8_t  NBIT_TRGTAG     = 8;  // Number of bits for the trigger tag
const uint8_t  NBIT_DUMMY      = 8;  // Number of bits for the Dummy Size
const uint8_t  NBIT_TDC        = 8;  // Number of bits for the TDC
const uint8_t  NBIT_L1ACNT     = 24; // Number of bits for the L1A Counter (Event number)
const uint8_t  NBIT_BXCNT      = 32; // Number of bits for the BX Counter

// ###############
// # Chip header #
// ###############
const uint8_t FRAME_HEADER   = 0xA;
const uint8_t NBIT_FRAMEHEAD = 4;  // Number of bits for the Frame Header
const uint8_t NBIT_ERR       = 4;  // Number of bits for the Error Code
const uint8_t NBIT_HYBRID    = 8;  // Number of bits for the Hybrid ID
const uint8_t NBIT_CHIPID    = 4;  // Number of bits for the Chip ID
const uint8_t NBIT_L1ASIZE   = 12; // Number of bits for the L1A Data Size
const uint8_t NBIT_PADDING   = 16; // Number of bits for the Padding
const uint8_t NBIT_CHIPTYPE  = 4;  // Number of bits for the Chip Type
const uint8_t NBIT_DELAY     = 12; // Number of bits for the Frame Delay

// ################
// # Event status #
// ################
const uint32_t GOOD       = 0x00000000; // Event status Good
const uint32_t EVSIZE     = 0x00000001; // Event status Invalid event size
const uint32_t EMPTY      = 0x00000002; // Event status Empty event
const uint32_t NOEVHEADER = 0x00000004; // Event status No event headear found in data
const uint32_t INCOMPLETE = 0x00000008; // Event status Incomplete event header
const uint32_t L1A        = 0x00000010; // Event status L1A counter mismatch
const uint32_t TRGTAG_ER1 = 0x00000020; // Event status Trigger tag mismatch
const uint32_t TRGTAG_ER2 = 0x00000040; // Event status Trigger tag single bit-flip
const uint32_t TRGTAG_ER3 = 0x00000080; // Event status Trigger tag Unrecognized tag symbol
const uint32_t NOFRHEADER = 0x00000100; // Event status No frame header found in data
const uint32_t MISSCHIP   = 0x00000200; // Event status Chip data are missing
const uint32_t CORRUPTED  = 0x00000400; // Event status Corrupted event
} // namespace RD53FWEvtEncoder

namespace Ph2_HwInterface
{
struct HitData
{
    HitData(uint16_t row, uint16_t col, uint8_t tot) : row(row), col(col), tot(tot) {}

    uint16_t row;
    uint16_t col;
    uint8_t  tot;
};

struct RD53ChipEvent
{
    static void decodeChipFrame(const uint32_t data0, const uint32_t data1, RD53ChipEvent& event);

    // #######################
    // # Firmware event data #
    // #######################
    uint16_t error_code;
    uint16_t hybrid_id;
    uint16_t chip_id;
    uint16_t chip_lane;
    uint16_t l1a_size;
    uint16_t chip_type;
    uint16_t frame_delay;

    // ###################
    // # Chip event data #
    // ###################
    uint16_t             chip_id_mod4;
    uint16_t             trigger_id;
    uint16_t             trigger_tag;
    uint16_t             bc_id;
    std::vector<HitData> hit_data;

    uint32_t eventStatus = RD53FWEvtEncoder::GOOD;
};

class RD53Event : public Ph2_HwInterface::Event
{
  public:
    // ######################
    // # Specific for RD53A #
    // ######################
    static RD53Event DecodeRD53AEvent(const uint32_t* data, size_t n32bitsWords);
    static void      DecodeRD53AEvents(const std::vector<uint32_t>& data, std::vector<RD53Event>& events, const std::vector<size_t>& refEventStart, uint32_t& eventStatus);

    // ######################
    // # Specific for RD53B #
    // ######################
    static size_t DecodeRD53BEvents(const std::vector<uint32_t>& data, std::vector<RD53Event>& events, uint32_t& eventStatus, const Ph2_HwDescription::DataFormatOptions& options);
    static size_t DecodeRD53BEvents(const std::vector<uint32_t>&                data,
                                    std::vector<RD53Event>&                     events,
                                    const std::vector<size_t>&                  refEventStart,
                                    uint32_t&                                   eventStatus,
                                    const Ph2_HwDescription::DataFormatOptions& options);
    static size_t DecodeRD53BEvents(const uint32_t* data, std::vector<RD53Event>& events, const size_t howMany, uint32_t& eventStatus, const Ph2_HwDescription::DataFormatOptions& options);

    // ############################
    // # Generic member functions #
    // ############################
    void fillDataContainer(BoardDataContainer* boardContainer, const std::shared_ptr<ChannelGroupBase> testChannelGroup) override;
    void fillChipDataContainer(ChipDataContainer* chipContainer, const std::shared_ptr<ChannelGroupBase> testChannelGroup, uint8_t hybridId) override;

    static void clearEventContainer(Ph2_HwDescription::BeBoard& theBoard, DetectorDataContainer& theContainer);
    static void addBoardInfo2Events(const Ph2_HwDescription::BeBoard* pBoard, std::vector<RD53Event>& decodedEvents);
    static bool findEventStarts(const std::vector<uint32_t>& data, std::vector<size_t>& eventStarts, uint32_t& eventStatus);
    static void ForkDecodingThreads();
    static void JoinDecodingThreads();
    static void DecodeEventsMultiThreads(const std::vector<uint32_t>& data, std::vector<RD53Event>& events, uint32_t& eventStatus, bool silentRunning = false);
    static void DecodeEvents(const std::vector<uint32_t>& data, std::vector<RD53Event>& events, const std::vector<size_t>& eventStart, uint32_t& eventStatus, bool silentRunning = false);
    static bool EvtErrorHandler(uint32_t status);
    static void PrintEvents(const std::vector<RD53Event>& events, const std::vector<uint32_t>& pData = {});
    static bool MakeNtuple(const std::string& fileName, const std::vector<RD53Event>& events);

    // ################
    // # Event format #
    // ################
    uint16_t                   block_size;
    uint16_t                   tlu_trigger_id;
    uint16_t                   trigger_tag;
    uint16_t                   tdc;
    uint32_t                   l1a_counter;
    uint32_t                   bx_counter;
    uint32_t                   eventStatus = RD53FWEvtEncoder::GOOD;
    std::vector<RD53ChipEvent> chip_events;

    // ########################################
    // # Vector containing the decoded events #
    // ########################################
    static std::vector<RD53Event> decodedEvents;
    static bool                   weakCheckDataStatus;

  private:
    bool        isHittedChip(uint8_t hybrid_id, uint8_t chip_id, size_t& chipIndx) const;
    static int  lane2chipId(const Ph2_HwDescription::BeBoard* pBoard, uint8_t hybrid_id, uint8_t chip_lane);
    static void decoderThread(std::vector<uint32_t>*& data, std::vector<RD53Event>& events, const std::vector<size_t>& eventStart, uint32_t& eventStatus, std::atomic<bool>& workDone);

    static std::vector<std::thread>            decodingThreads;
    static std::vector<std::vector<RD53Event>> vecEvents;
    static std::vector<std::vector<size_t>>    vecEventStart;
    static std::vector<uint32_t>               vecEventStatus;
    static std::vector<std::atomic<bool>>      vecWorkDone;
    static std::vector<uint32_t>*              theData;

    static std::condition_variable thereIsWork2Do;
    static std::atomic<bool>       keepDecodersRunning;
    static std::atomic<bool>       silentRunning;
    static std::mutex              theMtx;
};

} // namespace Ph2_HwInterface

#endif
