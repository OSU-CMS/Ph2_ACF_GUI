#include "Utils/ContainerSerialization.h"
#include "Utils/ADCSlope.h"
#include "Utils/EmptyContainer.h"
#include "Utils/GainFit.h"
#include "Utils/GenericDataArray.h"
#include "Utils/GenericDataVector.h"
#include "Utils/LpGBTalignmentResult.h"
#include "Utils/Occupancy.h"
#include "Utils/OccupancyAndPh.h"
#include "Utils/ThresholdAndNoise.h"
#include "Utils/ValueAndTime.h"

BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((ChannelDataContainer<uint8_t>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((ChannelDataContainer<uint32_t>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((ChannelDataContainer<float>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((ChannelDataContainer<Occupancy>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((ChannelDataContainer<OccupancyAndPh>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((ChannelDataContainer<ThresholdAndNoise>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((ChannelDataContainer<EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((ChannelDataContainer<GainFit>)))

BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<Occupancy, Occupancy>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<OccupancyAndPh, OccupancyAndPh>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataVector, OccupancyAndPh>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, Occupancy>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GainFit, GainFit>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, float>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<float, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, ValueAndTime<float>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<ValueAndTime<float>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, std::pair<uint16_t, uint16_t>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, double>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<ThresholdAndNoise, ThresholdAndNoise>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<uint16_t, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, uint8_t>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, uint16_t>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, bool>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, std::string>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<std::string, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<std::string, std::string>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, std::pair<uint8_t, float>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<std::pair<uint8_t, float>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, ADCSlope>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<ADCSlope, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<uint32_t, uint32_t>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint16_t, VECSIZE>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint16_t, TDCBINS>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<GenericDataArray<uint16_t, VECSIZE>, VECSIZE>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, NCHANNELS + 1>, EmptyContainer>)))               // 1 CBC (+1)
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, NCHANNELS + 1>>)))               // 1 CBC (+1)
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, NSSACHANNELS + 1>, EmptyContainer>)))            // 1 SSA (+1)
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, NSSACHANNELS + 1>>)))            // 1 SSA (+1)
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, NSSACHANNELS * NMPAROWS + 1>, EmptyContainer>))) // 1 MPA (+1) - this is also the size of 1 module for PS SSAs
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, NSSACHANNELS * NMPAROWS + 1>>))) // 1 MPA (+1) - this is also the size of 1 module for PS SSAs
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT + 1>, GenericDataArray<uint32_t, NCHANNELS + 1>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2 + 1>, GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT + 1>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2, NCHANNELS * NCHIPS_OT * 2>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, NCHANNELS, NCHANNELS>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<std::vector<double>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<std::vector<float>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<std::vector<uint16_t>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<std::vector<bool>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (NCHANNELS / 2 + 1), (NCHANNELS / 2 + 1)>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (NSSACHANNELS + 1), (NSSACHANNELS * NMPAROWS + 1)>, EmptyContainer>))) // SSA-MPA correlation
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, (NSSACHANNELS + 1), (NSSACHANNELS * NMPAROWS + 1)>>))) // SSA-MPA correlation
BOOST_CLASS_EXPORT_IMPLEMENT(
    BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (NSSACHANNELS * NCHIPS_OT + 1), (NSSACHANNELS * NMPAROWS * NCHIPS_OT + 1)>, EmptyContainer>))) // Strip-Pixel hybrid correlation
BOOST_CLASS_EXPORT_IMPLEMENT(
    BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, (NSSACHANNELS * NCHIPS_OT + 1), (NSSACHANNELS * NMPAROWS * NCHIPS_OT + 1)>>))) // Strip-Pixel hybrid correlation
BOOST_CLASS_EXPORT_IMPLEMENT(
    BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (NSSACHANNELS * NCHIPS_OT * 2 + 1), (NSSACHANNELS * NMPAROWS * NCHIPS_OT * 2 + 1)>, EmptyContainer>))) // Strip-Pixel module correlation
BOOST_CLASS_EXPORT_IMPLEMENT(
    BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, (NSSACHANNELS * NCHIPS_OT * 2 + 1), (NSSACHANNELS * NMPAROWS * NCHIPS_OT * 2 + 1)>>))) // Strip-Pixel module correlation
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, 3 * (NCHANNELS + 1)>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, 3 * (NCHANNELS * NCHIPS_OT + 1)>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, 3 * (NCHANNELS * NCHIPS_OT * 2 + 1)>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (NSSACHANNELS * NCHIPS_OT + 1)>, EmptyContainer>)))                // 1 hybrid for PS SSAs
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, (NSSACHANNELS * NCHIPS_OT + 1)>>)))                // 1 hybrid for PS SSAs
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (NSSACHANNELS * NMPAROWS * NCHIPS_OT + 1)>, EmptyContainer>)))     // 1 hybrid for PS MPAs
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, (NSSACHANNELS * NMPAROWS * NCHIPS_OT + 1)>>)))     // 1 hybrid for PS MPAs
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (NSSACHANNELS * NMPAROWS * NCHIPS_OT * 2 + 1)>, EmptyContainer>))) // 1 module for PS MPAs
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, (NSSACHANNELS * NMPAROWS * NCHIPS_OT * 2 + 1)>>))) // 1 module for PS MPAs
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, NCHANNELS + 1, NCHANNELS * NCHIPS_OT + 1>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT + 1, NCHANNELS * NCHIPS_OT + 1>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<std::vector<uint8_t>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<LpGBTalignmentResult, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS, 16>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS - 1>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 9>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<float, NUMBER_OF_CIC_PORTS, 9>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<std::pair<uint16_t, uint16_t>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<float, 6>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<float, 6>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<float, 4, 2>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<float, 4, 2>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint8_t, 4>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint8_t, 4>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint16_t, 9>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint16_t, 9>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint16_t, 4>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint16_t, 4>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<std::pair<std::string, std::string>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint16_t, NMPAROWS + 1>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint16_t, NMPAROWS + 1>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (MAXCICCHANNELS + 1)>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, (MAXCICCHANNELS + 1)>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (MAXCICCHANNELS * 2 + 1)>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, (MAXCICCHANNELS * 2 + 1)>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (MAXCICCHANNELS + 1), (MAXCICCHANNELS + 1)>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, (MAXCICCHANNELS + 1), (MAXCICCHANNELS + 1)>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint16_t, 64, 31>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint16_t, 64, 31>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<std::vector<GenericDataArray<uint64_t, 2>>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, std::vector<GenericDataArray<uint64_t, 2>>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<std::vector<GenericDataArray<float, 2>>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, std::vector<GenericDataArray<float, 2>>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<float, NUMBER_OF_CIC_PORTS, 2, 2>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 2, 2>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<float, NUMBER_OF_CIC_PORTS, 9, 2>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 9, 2>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<float, 6, 2>>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<float, 6, 2>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<uint32_t, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, uint32_t>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint64_t, 2>, EmptyContainer>)))
BOOST_CLASS_EXPORT_IMPLEMENT(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint64_t, 2>>)))

ContainerSerialization::ContainerSerialization(const std::string& calibrationName) : fCalibrationName(calibrationName) {}

ContainerSerialization::~ContainerSerialization() {}

bool ContainerSerialization::attachDeserializer(std::string& inputBuffer)
{
    std::istringstream            inputStream(inputBuffer);
    boost::archive::text_iarchive theArchive(inputStream);
    std::string                   inputCalibrationName;
    theArchive >> inputCalibrationName;
    if(inputCalibrationName != fCalibrationName) return false;
    fStream = std::move(inputBuffer);
    return true;
}
