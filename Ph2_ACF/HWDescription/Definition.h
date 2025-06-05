/*

    \file                          Definition.h
    \brief                         Definition File, listing the registers
    \author                        Nicolas PIERRE
    \version                       1.0
    \date                          07/06/14
    Support :                      mail to : nico.pierre@icloud.com

 */
#ifndef _DEFINITION_H__
#define _DEFINITION_H__

#include <string>

//-----------------------------------------------------------------------------
// Glib Config Files

// Time out for stack writing
// #define TIME_OUT         5

//------------------------------------------------------------------------------
#define NCHANNELS 254
#define NSSACHANNELS 120
#define NMPAROWS 16
#define NCHIPS_OT 8
#define MAXCICCLUSTERS 127
#define MAXCICCHANNELS 127 * 8 // cluster width is 3 bits so it can be maximum 7

// Fix issue if HOST_NAME_MAX is not declared
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 64
#endif

// Cbc Event
#define OFFSET_CBCSTUBDATA 264 + 23 // LAST BIT
#define WIDTH_CBCSTUBDATA 12

// D19C event header size (CBC)
#define D19C_EVENT_HEADER1_SIZE_32_CBC3 4
#define D19C_EVENT_SIZE_32_CBC3 16

#define NUMBER_OF_ELECTRON_PER_UM 75 // Number of electron deposited by a MIP in silicon per um

// CBC
#define CBC_VCTH_ELECTRON_UNIT 156              // conversion factor from 1 VcTh DAC into electron
#define CBC_ELECTRON_TO_CALDAC_INTERCEPT 253.96 // Electron to CalDac calibration curve - intercept
#define CBC_ELECTRON_TO_CALDAC_SLOPE -0.001648  // Electron to CalDac calibration curve - slope
#define TWOS_SENSOR_THICHNESS 290               // 2S sensor active thickness

// SSA
#define SSA_THDAC_ELECTRON_UNIT 250  // conversion factor from 1 ThDac into electron
#define SSA_CALDAC_ELECTRON_UNIT 243 // conversion factor from 1 CalDac into electron
#define PSS_SENSOR_THICHNESS 290     // PSS sensor active thickness

// MPA
#define MPA_THDAC_ELECTRON_UNIT 94   // conversion factor from 1 ThDac into electron
#define MPA_CALDAC_ELECTRON_UNIT 220 // conversion factor from 1 CalDac into electron
#define PSP_SENSOR_THICHNESS 290     // PSP sensor active thickness

// SSA2
// in float
#define SSA2_VBG_EXPECTED 0.275   // FIXME [V] this will need to be taken from database
#define SSA2_VREF_EXPECTED 0.850  // [V] this is true if ADC_VREF is tuned
#define SSA2_VREF_MIN 0.750       // FIXME [V] should be verified once we have numbers/manual is updated
#define SSA2_VREF_MAX 1.0         // FIXME [V] should be verified once we have numbers/manual is updated
#define SSA2_ADC_PRECISION 0.010  // [V] from skeleton testing: changing by 1 bit ADC_VREF, VREF measured on skeleton changes by 7-8 mV. Here we are rounding up the precision.
#define SSA2_ELECTRON_CALDAC 243. // 1 CalDAC = 0.039 fC = 243 electrons - confirmed by Davide
#define SSA2_ELECTRON_THDAC 250.  // 1 ThDAC  = 0.040 fC = 250 electrons - confirmed by Davide

// MPA2
// in float
#define MPA2_VBG_EXPECTED 0.280   // FIXME [V] this will need to be taken from database
#define MPA2_VREF_EXPECTED 0.850  // [V] this is true if ADC_VREF is tuned
#define MPA2_VREF_MIN 0.750       // FIXME [V] should be verified once we have numbers/manual is updated
#define MPA2_VREF_MAX 1.0         // FIXME [V] should be verified once we have numbers/manual is updated
#define MPA2_ADC_PRECISION 0.010  // [V] from skeleton testing: changing by 1 bit ADC_VREF, VREF measured on skeleton changes by 7-8 mV. Here we are rounding up the precision.
#define MPA2_ELECTRON_CALDAC 220. // 1 CalDAC = 0.035 fC = 220 electrons - confirmed by Davide
#define MPA2_ELECTRON_THDAC 94.   // 1 ThDAC  = 0.015 fC =  94 electrons - confirmed by Davide

// points to bufferoverlow
#define D19C_OFFSET_ERROR_CBC3 2 * 32 + 0

#define CBC_CHANNEL_GROUP_BITSET                                                                                                                                                                       \
    std::string("0000000000001100000000000000110000000000000011000000000000001100000000000000110000000000000011000000"                                                                                 \
                "0000000011000000000000001100000000000000110000000000000011000000000000001100000000000000110000000000"                                                                                 \
                "000011000000000000001100000000000000110000000000000011")

#define D19C_SCluster_SIZE_32_MPA 11

// Cbc Event
#define WIDTH_CBCSTUBDATA 12

// number of bend codes
#define BENDBINS 30
// Latency Scan
#define TDCBINS 8
#define VECSIZE 1000
//------------------------------------------------------------------------------

// OT Physics parameters
#define MAX_NUMBER_OF_STRIP_CLUSTERS 5
#define MAX_NUMBER_OF_PIXEL_CLUSTERS 5
#define MAX_NUMBER_OF_STUB_CLUSTERS_PS 5
#define MAX_NUMBER_OF_STUB_CLUSTERS_2S 3

#define BERT_ALIGNMENT_PATTERN 0x020C

// LpGBT conversion factors
#define VREF_LPGBT 1.0
#define CONVERSION_FACTOR (VREF_LPGBT / 1024.)

enum class BoardType
{
    UNDEFINED,
    D19C,
    RD53
};
enum class FrontEndType
{
    UNDEFINED = 0,
    HYBRID,
    CBC3,
    MPA2,
    SSA2,
    RD53A,
    RD53Bv1,
    RD53Bv2,
    CIC2,
    OuterTracker2S,
    OuterTrackerPS,
    InnerTrackerDouble,
    InnerTrackerQuad,
    HYBRID2S,
    HYBRIDPS,
    LpGBT,
    VTRx
};
enum class SLinkDebugMode
{
    SUMMARY = 0,
    FULL    = 1,
    ERROR   = 2
};
enum class EventType
{
    ZS              = 1, // ZeroSuppression
    VR              = 2, // VirginRaw
    PSAS            = 3,
    VR2S            = 4,
    VRPCTestAdapter = 5 // VirginRaw with IT PortCard test adapter
};

#define NUMBER_OF_CIC_PORTS 8
#define NUMBER_OF_LINES_PER_CIC_PORTS 6
#define NUMBER_OF_LINES_PER_CIC_PHY_PORTS 4

#define TIME_FORMAT "%Y-%m-%d %H:%M:%S"

#define CLUSTER_2S_DATA_SIZE 14 // bits
#define CLUSTER_2S_DATA_MASK 0x3FFF
#define STRIP_CLUSTER_PS_DATA_SIZE 14 // bits
#define STRIP_CLUSTER_PS_DATA_MASK 0x3FFF
#define PIXEL_CLUSTER_PS_DATA_SIZE 17 // bits
#define PIXEL_CLUSTER_PS_DATA_MASK 0x1FFFF

#define STUB_2S_DATA_SIZE 15 // bits
#define STUB_2S_DATA_MASK 0x7FFF
#define STUB_PS_DATA_SIZE 18 // bits
#define STUB_PS_DATA_MASK 0x3FFFF

#define L1_UNSPARSFIED_BLOCK_SIZE_2S 11

#endif
