/*!
  \file                  RD53.h
  \brief                 RD53 description class, config of the RD53
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53_H
#define RD53_H

#include "BeBoard.h"
#include "ReadoutChip.h"
#include "Utils/BitMaster/bit_packing.h"
#include "Utils/ConsoleColor.h"
#include "Utils/Container.h"
#include "Utils/RD53Event.h"
#include "Utils/RD53Shared.h"

// ##################
// # Default values #
// ##################
#define NCHIPLANES 4

// #########################
// # Chip useful constants #
// #########################
namespace RD53Constants
{
const uint8_t ACCELERATOR_CLK = 40;   // Accelerator clock frequency [MHz]
const uint8_t NBIT_MAXREG     = 16;   // Maximum number of bits for a chip register
const uint8_t NPIX_REGION     = 4;    // Number of pixels in a region (1x4)
const uint8_t NROW_CORE       = 8;    // Number of rows in a core
const uint8_t NBIT_ADDR       = 9;    // Number of address bits
const uint8_t NBIT_TOT        = 4;    // Number of ToT bits
const uint8_t NSYNC_WORDS_S   = 2;    // Number of Sync words for synchronization (S = small)
const uint8_t NSYNC_WORDS_L   = 64;   // Number of Sync words for synchronization (L = large)
const uint8_t NWORDS_TO_SYNC  = 30;   // Number of words beforse send a Sync
const uint8_t PATTERN_PRBS    = 0xAA; // Start PRBS pattern
const uint8_t PATTERN_AURORA  = 0x55; // Start AURORA pattern
const uint8_t PATTERN_CLOCK   = 0x00; // Start clock pattern
} // namespace RD53Constants

// #####################
// # Chip event status #
// #####################
namespace RD53EvtEncoder
{
const uint32_t CHIPGOOD          = 0x00000000; // Chip event status Good
const uint32_t CHIPHEAD          = 0x00010000; // Chip event status Bad chip header
const uint32_t CHIPID            = 0x00020000; // Chip event status Found conflicting chip ID
const uint32_t CHIPPIX           = 0x00040000; // Chip event status Bad pixel row or column
const uint32_t CHIPTOT           = 0x00080000; // Chip event status Invalid TOT value
const uint32_t CHIPNOHIT         = 0x00100000; // Chip event status Hit data are missing
const uint32_t CHIPFWERR         = 0x00200000; // Chip event status Firmware error
const uint32_t CHIPNS_WAS0       = 0x00400000; // Chip event status new-stream bit was 0 in the first word of the event stream
const uint32_t CHIPNS_WAS1       = 0x00800000; // Chip event status new-stream bit was 1 before the last word of the event stream
const uint32_t CHIPQROW          = 0x01000000; // Chip event status neighbor bit set for the first qrow
const uint32_t CHIPTRUNC_MAXHITS = 0x02000000; // Chip event status truncation occurred due to max number of hits reached per core
const uint32_t CHIPTRUNC_TIMEOUT = 0x04000000; // Chip event status truncation occurred due to readout timeout
} // namespace RD53EvtEncoder

namespace Ph2_HwDescription
{
// ############################################
// # Data structure describing the chip masks #
// ############################################
struct pixelMask
{
    pixelMask() = default;
    pixelMask(size_t size, bool en, bool hb, bool ie, uint8_t tdac) : Enable(size, en), HitBus(size, hb), InjEn(size, ie), TDAC(size, tdac) {}

    std::vector<bool>    Enable;
    std::vector<bool>    HitBus;
    std::vector<bool>    InjEn;
    std::vector<uint8_t> TDAC;
};

// ###################################################
// # Data structure describing the chip lane mapping #
// ###################################################
struct LaneConfig
{
    LaneConfig() : outputLaneMapping({0, 1, 2, 3}), inputLaneMapping({0, 1, 2, 3}), internalLanesEnabled({0, 0, 0, 0, 0}), nOutputLanes(1), masterLane(0), isPrimary(true) {}
    LaneConfig(bool                                   isPrimary,
               uint8_t                                masterLane,
               const std::array<uint8_t, NCHIPLANES>& outputLanes,
               const std::array<bool, NCHIPLANES>&    signleChannelInputLanes,
               const std::array<bool, NCHIPLANES>&    dualChannelInputLanes);

    // ####################################
    // # Serialize an array of N elements #
    // # allowing S-bits for each element #
    // # into a TT-type variable          #
    // ####################################
    template <typename T, size_t N, size_t S, typename TT>
    TT serializeArray(const std::array<T, N>& arr)
    {
        return processUnfoldedArray<T, N, S, TT>(arr, std::make_index_sequence<N>{});
    }

    uint8_t packOutputLanes()
    {
        std::array<bool, NCHIPLANES> laneEnable = {false};
        std::transform(outputLaneMapping.begin(), outputLaneMapping.end(), laneEnable.begin(), [&](const auto& x) { return x < nOutputLanes; });
        return serializeArray<bool, NCHIPLANES, 1, uint16_t>(laneEnable);
    }

    std::array<uint8_t, NCHIPLANES>  outputLaneMapping;
    std::array<uint8_t, NCHIPLANES>  inputLaneMapping;
    std::array<bool, NCHIPLANES + 1> internalLanesEnabled;
    uint8_t                          nOutputLanes;
    uint8_t                          masterLane;
    bool                             isPrimary;

  private:
    template <typename T, size_t N, size_t S, typename TT, size_t... Is>
    TT processUnfoldedArray(const std::array<T, N>& arr, std::index_sequence<Is...>)
    {
        return serializeElements<S, TT, Is...>(arr[Is]...);
    }

    template <size_t S, typename TT, size_t... Is, typename... Args>
    TT serializeElements(Args... args)
    {
        TT                           result = 0;
        __attribute__((unused)) auto unused = {result |= args << (S * Is)...};
        return result;
    }
};

// ####################################
// # Optional fields in chip protocol #
// ####################################
struct DataFormatOptions
{
    bool enableChipId;
    bool enableToT;
    bool enableBCID;
    bool enableTriggerId;
    bool enableEOSmarker;
    bool enableRawMap;
    bool enableCRC;
};

class RD53 : public ReadoutChip
{
  public:
    // ########################################
    // # Support for different FrontEnd types #
    // ########################################
    struct FrontEnd
    {
        const char*                     name;
        const std::vector<const char*>  thresholdRegs;
        const char*                     gainReg;
        const char*                     latencyReg;
        const char*                     TDACGainReg;
        const char*                     VDDDreadReg;
        const char*                     VDDAreadReg;
        size_t                          nLatencyBins2Span;
        size_t                          nTDACvalues;
        size_t                          maxToTvalue;
        size_t                          splitToTvalue;
        size_t                          nBitTrimDig;
        size_t                          nBitTrimAna;
        size_t                          colStart;
        size_t                          colStop;
        size_t                          VCalSleepTime; // [microseconds]
        size_t                          AutoIncrementMask;
        size_t                          broadcastChipId;
        std::map<std::string, uint16_t> GlobalPulseConfMap;
        std::vector<std::string>        CoreColRegs;
    };

    struct SpecialRegInfo
    {
        std::string regName;
        uint8_t     start; // Bit index at which the special register, i.e. field, starts
    };

    // ####################################
    // # Input&Output lanes configuration #
    // ####################################
    LaneConfig laneConfig;

    // ############################
    // # Virtual member functions #
    // ############################
    virtual size_t                   getNRows() const                                                                                                     = 0;
    virtual size_t                   getNCols() const                                                                                                     = 0;
    virtual size_t                   getMaxBCIDvalue() const                                                                                              = 0;
    virtual size_t                   getMaxTRIGIDvalue() const                                                                                            = 0;
    virtual std::vector<uint16_t>    getLaneUpInitSequence() const                                                                                        = 0;
    virtual const DataFormatOptions& getDataFormatOptions()                                                                                               = 0;
    virtual const FrontEnd*          getFEtype(const size_t colStart = 0, const size_t colStop = 0)                                                       = 0;
    virtual uint32_t                 getCalCmd(bool cal_edge_mode, size_t cal_edge_delay, size_t cal_edge_width, bool cal_aux_mode, size_t cal_aux_delay) = 0;
    virtual float                    VCal2Charge(float VCal, bool isNoise = false) const                                                                  = 0;
    virtual float                    Charge2VCal(float Charge) const                                                                                      = 0;
    virtual bool                     getUseGainDualSlope() const                                                                                          = 0;

    // ################
    // # Constructors #
    // ################
    RD53() : ReadoutChip(0, 0, 0, 0, 0) {}
    RD53(uint8_t            pBeId,
         uint8_t            pFMCId,
         uint8_t            pOpticalGroupId,
         uint8_t            pHybridId,
         uint8_t            pRD53Id,
         uint8_t            pRD53Lane,
         int64_t            pRD53eFuseCode,
         const std::string& fileName,
         const std::string& cfgComment);
    RD53(const RD53&) = delete;

    // #############################
    // # Override member functions #
    // #############################
    void              loadfRegMap(const std::string& fileName) override;
    std::stringstream getRegMapStream() override;
    uint32_t          getNumberOfChannels() const override;
    bool              isDACLocal(const std::string& regName) override;
    uint8_t           getNumberOfBits(const std::string& regName) override;
    // #############################

    pixelMask&  getPixelsMask() { return fPixelsMask; }
    pixelMask&  getPixelsMaskDefault() { return fPixelsMaskDefault; }
    void        copyMaskFromDefault(const std::string& which = "all");
    void        copyMaskToDefault(const std::string& which = "all");
    void        resetMask();
    void        enableAllPixels();
    void        disableAllPixels();
    size_t      getNbMaskedPixels();
    void        enablePixel(unsigned int row, unsigned int col, bool enable);
    void        enableDefaultPixel(unsigned int row, unsigned int col, bool enable);
    void        maskCoreDefault(unsigned int row, unsigned int col);
    void        injectPixel(unsigned int row, unsigned int col, bool inject);
    void        setTDAC(unsigned int row, unsigned int col, uint8_t TDAC);
    void        resetTDAC(uint8_t TDAC);
    uint8_t     getTDAC(unsigned int row, unsigned int col);
    uint8_t     getChipLane() const { return myChipLane; }
    int64_t     geteFuseCode() const { return myeFuseCode; }
    std::string getComment() const { return myComment; }

    // #################
    // # LpGBT mapping #
    // #################
    void addRxGroup(uint8_t pRxGroup, uint8_t chipLane) { fLpGBTmap.RxGroupsChipLanes.push_back(std::pair(pRxGroup, chipLane)); }
    void setRxChannel(uint8_t pRxChannel) { fLpGBTmap.RxChannel = pRxChannel; }
    void setRxPolarity(uint8_t pRxPolarity) { fLpGBTmap.RxPolarity = pRxPolarity; }

    void setTxGroup(uint8_t pTxGroup) { fLpGBTmap.TxGroup = pTxGroup; }
    void setTxChannel(uint8_t pTxChannel) { fLpGBTmap.TxChannel = pTxChannel; }
    void setTxPolarity(uint8_t pTxPolarity) { fLpGBTmap.TxPolarity = pTxPolarity; }

    std::vector<std::pair<uint8_t, uint8_t>> getRxGroupsChipLanes() { return fLpGBTmap.RxGroupsChipLanes; }
    std::vector<uint8_t>                     getRxGroups()
    {
        std::vector<uint8_t> RxGroups;
        for(auto RxGroupChipLane: fLpGBTmap.RxGroupsChipLanes) RxGroups.push_back(RxGroupChipLane.first);
        return RxGroups;
    }
    uint8_t getRxChannel() { return fLpGBTmap.RxChannel; }
    uint8_t getRxPolarity() { return fLpGBTmap.RxPolarity; }

    uint8_t getTxGroup() { return fLpGBTmap.TxGroup; }
    uint8_t getTxChannel() { return fLpGBTmap.TxChannel; }
    uint8_t getTxPolarity() { return fLpGBTmap.TxPolarity; }

  protected:
    DataFormatOptions dataFormatOptions;

  private:
    struct LpGBTmap
    {
        std::vector<std::pair<uint8_t, uint8_t>> RxGroupsChipLanes;
        uint8_t                                  RxChannel;
        uint8_t                                  RxPolarity;
        uint8_t                                  TxGroup;
        uint8_t                                  TxChannel;
        uint8_t                                  TxPolarity;
    } fLpGBTmap;
    pixelMask   fPixelsMask;
    pixelMask   fPixelsMaskDefault;
    std::string myComment;
    uint8_t     myChipLane;
    int64_t     myeFuseCode;
};

} // namespace Ph2_HwDescription

#endif
