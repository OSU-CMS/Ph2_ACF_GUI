#ifndef __CONTAINER_SERIALIZATION__
#define __CONTAINER_SERIALIZATION__

#include "HWDescription/Definition.h"
#include "HWDescription/RD53A.h"
#include "HWDescription/RD53B.h"
#include "NetworkUtils/TCPPublishServer.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/DataContainer.h"
#include "Utils/PacketHeader.h"
#include "Utils/RD53Shared.h"
#include "Utils/serialize_tuple.h"
#include <boost/serialization/export.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>
#include <boost/utility/identity_type.hpp>
#include <map>

class Occupancy;
class OccupancyAndPh;
class ThresholdAndNoise;
class EmptyContainer;
template <typename T, size_t N, size_t... S>
class GenericDataArray;
class GainFit;
class GenericDataVector;
template <typename T>
class ValueAndTime;
class LpGBTalignmentResult;
class ADCSlope;

BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((ChannelDataContainer<uint8_t>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((ChannelDataContainer<uint32_t>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((ChannelDataContainer<float>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((ChannelDataContainer<Occupancy>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((ChannelDataContainer<OccupancyAndPh>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((ChannelDataContainer<ThresholdAndNoise>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((ChannelDataContainer<EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((ChannelDataContainer<GainFit>)))

BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<Occupancy, Occupancy>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<OccupancyAndPh, OccupancyAndPh>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataVector, OccupancyAndPh>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, Occupancy>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GainFit, GainFit>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, float>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<float, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, ValueAndTime<float>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<ValueAndTime<float>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, std::pair<uint16_t, uint16_t>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, double>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<ThresholdAndNoise, ThresholdAndNoise>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<uint16_t, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, uint8_t>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, uint16_t>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, bool>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, std::string>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<std::string, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<std::string, std::string>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, std::pair<uint8_t, float>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<std::pair<uint8_t, float>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, ADCSlope>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<ADCSlope, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<uint32_t, uint32_t>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint16_t, VECSIZE>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint16_t, TDCBINS>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<GenericDataArray<uint16_t, VECSIZE>, VECSIZE>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, NCHANNELS + 1>, EmptyContainer>)))               // 1 CBC (+1)
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, NCHANNELS + 1>>)))               // 1 CBC (+1)
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, NSSACHANNELS + 1>, EmptyContainer>)))            // 1 SSA (+1)
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, NSSACHANNELS + 1>>)))            // 1 SSA (+1)
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, NSSACHANNELS * NMPAROWS + 1>, EmptyContainer>))) // 1 MPA (+1) - this is also the size of 1 module for PS SSAs
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, NSSACHANNELS * NMPAROWS + 1>>))) // 1 MPA (+1) - this is also the size of 1 module for PS SSAs
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT + 1>, GenericDataArray<uint32_t, NCHANNELS + 1>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2 + 1>, GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT + 1>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2, NCHANNELS * NCHIPS_OT * 2>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, NCHANNELS, NCHANNELS>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<std::vector<double>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<std::vector<float>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<std::vector<uint16_t>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<std::vector<bool>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (NCHANNELS / 2 + 1), (NCHANNELS / 2 + 1)>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (NSSACHANNELS + 1), (NSSACHANNELS * NMPAROWS + 1)>, EmptyContainer>))) // SSA-MPA correlation
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, (NSSACHANNELS + 1), (NSSACHANNELS * NMPAROWS + 1)>>))) // SSA-MPA correlation
BOOST_CLASS_EXPORT_KEY(
    BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (NSSACHANNELS * NCHIPS_OT + 1), (NSSACHANNELS * NMPAROWS * NCHIPS_OT + 1)>, EmptyContainer>))) // Strip-Pixel hybrid correlation
BOOST_CLASS_EXPORT_KEY(
    BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, (NSSACHANNELS * NCHIPS_OT + 1), (NSSACHANNELS * NMPAROWS * NCHIPS_OT + 1)>>))) // Strip-Pixel hybrid correlation
BOOST_CLASS_EXPORT_KEY(
    BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (NSSACHANNELS * NCHIPS_OT * 2 + 1), (NSSACHANNELS * NMPAROWS * NCHIPS_OT * 2 + 1)>, EmptyContainer>))) // Strip-Pixel module correlation
BOOST_CLASS_EXPORT_KEY(
    BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, (NSSACHANNELS * NCHIPS_OT * 2 + 1), (NSSACHANNELS * NMPAROWS * NCHIPS_OT * 2 + 1)>>))) // Strip-Pixel module correlation
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, 3 * (NCHANNELS + 1)>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, 3 * (NCHANNELS * NCHIPS_OT + 1)>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, 3 * (NCHANNELS * NCHIPS_OT * 2 + 1)>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (NSSACHANNELS * NCHIPS_OT + 1)>, EmptyContainer>)))                // 1 hybrid for PS SSAs
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, (NSSACHANNELS * NCHIPS_OT + 1)>>)))                // 1 hybrid for PS SSAs
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (NSSACHANNELS * NMPAROWS * NCHIPS_OT + 1)>, EmptyContainer>)))     // 1 hybrid for PS MPAs
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, (NSSACHANNELS * NMPAROWS * NCHIPS_OT + 1)>>)))     // 1 hybrid for PS MPAs
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (NSSACHANNELS * NMPAROWS * NCHIPS_OT * 2 + 1)>, EmptyContainer>))) // 1 module for PS MPAs
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, (NSSACHANNELS * NMPAROWS * NCHIPS_OT * 2 + 1)>>))) // 1 module for PS MPAs
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, NCHANNELS + 1, NCHANNELS * NCHIPS_OT + 1>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT + 1, NCHANNELS * NCHIPS_OT + 1>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<std::vector<uint8_t>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<LpGBTalignmentResult, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS, 16>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS - 1>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 9>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<float, NUMBER_OF_CIC_PORTS, 9>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<std::pair<uint16_t, uint16_t>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<float, 6>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<float, 6>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<float, 4, 2>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<float, 4, 2>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint8_t, 4>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint8_t, 4>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint16_t, 9>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint16_t, 9>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint16_t, 4>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint16_t, 4>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<std::pair<std::string, std::string>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint16_t, NMPAROWS + 1>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint16_t, NMPAROWS + 1>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (MAXCICCHANNELS + 1)>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, (MAXCICCHANNELS + 1)>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (MAXCICCHANNELS * 2 + 1)>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, (MAXCICCHANNELS * 2 + 1)>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint32_t, (MAXCICCHANNELS + 1), (MAXCICCHANNELS + 1)>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint32_t, (MAXCICCHANNELS + 1), (MAXCICCHANNELS + 1)>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint16_t, 64, 31>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint16_t, 64, 31>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<std::vector<GenericDataArray<uint64_t, 2>>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, std::vector<GenericDataArray<uint64_t, 2>>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<std::vector<GenericDataArray<float, 2>>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, std::vector<GenericDataArray<float, 2>>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<float, NUMBER_OF_CIC_PORTS, 2, 2>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 2, 2>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<float, NUMBER_OF_CIC_PORTS, 9, 2>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 9, 2>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<float, 6, 2>>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<float, 6, 2>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<uint32_t, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, uint32_t>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<GenericDataArray<uint64_t, 2>, EmptyContainer>)))
BOOST_CLASS_EXPORT_KEY(BOOST_IDENTITY_TYPE((Summary<EmptyContainer, GenericDataArray<uint64_t, 2>>)))

#include "Utils/Occupancy.h"
#include <arpa/inet.h>
#include <iostream>

// #define END_OF_TRANSMISSION_MESSAGE "DoneWithRun"

template <uint N>
struct Serialize
{
    template <class Archive, typename... Args>
    static void serialize(Archive& theArchive, std::tuple<Args...>& theTuple)
    {
        theArchive& std::get<N - 1>(theTuple);
        Serialize<N - 1>::serialize(theArchive, theTuple);
    }
};

template <>
struct Serialize<0>
{
    template <class Archive, typename... Args>
    static void serialize(Archive& theArchive, std::tuple<Args...>& theTuple)
    {
        (void)theArchive;
        (void)theTuple;
    }
};

template <class Archive, typename... Args>
void serialize(Archive& theArchive, std::tuple<Args...>& theTuple)
{
    Serialize<sizeof...(Args)>::serialize(theArchive, theTuple);
}

class ContainerSerialization
{
  public:
    ContainerSerialization(const std::string& calibrationName);
    ~ContainerSerialization();

    bool attachDeserializer(std::string& inputBuffer);

    // !!! ---------------------------------------------------------------------------- !!! //
    // Stream
    // !!! ---------------------------------------------------------------------------- !!! //
    template <typename... Args>
    void streamByDetectorContainer(TCPPublishServer* networkStreamer, DetectorDataContainer& theInputContainer, Args&... extraArguments) const
    {
        std::string  myStream = serializeDetectorContainer(theInputContainer, extraArguments...);
        PacketHeader thePacketHeader;
        thePacketHeader.addPacketHeader(myStream);
        networkStreamer->broadcast(myStream);
    }

    template <typename... Args>
    void streamByBoardContainer(TCPPublishServer* networkStreamer, DetectorDataContainer& theInputContainer, Args&... extraArguments) const
    {
        for(auto board: theInputContainer)
        {
            std::string  myStream = serializeBoardContainer(board, extraArguments...);
            PacketHeader thePacketHeader;
            thePacketHeader.addPacketHeader(myStream);
            networkStreamer->broadcast(myStream);
        }
    }

    template <typename... Args>
    void streamByOpticalGroupContainer(TCPPublishServer* networkStreamer, DetectorDataContainer& theInputContainer, Args&... extraArguments) const
    {
        for(auto board: theInputContainer)
        {
            for(auto opticalGroup: *board)
            {
                std::string  myStream = serializeOpticalGroupContainer(opticalGroup, board->getId(), extraArguments...);
                PacketHeader thePacketHeader;
                thePacketHeader.addPacketHeader(myStream);
                networkStreamer->broadcast(myStream);
            }
        }
    }

    template <typename... Args>
    void streamByHybridContainer(TCPPublishServer* networkStreamer, DetectorDataContainer& theInputContainer, Args&... extraArguments) const
    {
        for(auto board: theInputContainer)
        {
            for(auto opticalGroup: *board)
            {
                for(auto hybrid: *opticalGroup)
                {
                    std::string  myStream = serializeHybridContainer(hybrid, board->getId(), opticalGroup->getId(), extraArguments...);
                    PacketHeader thePacketHeader;
                    thePacketHeader.addPacketHeader(myStream);
                    networkStreamer->broadcast(myStream);
                }
            }
        }
    }

    template <typename... Args>
    void streamByChipContainer(TCPPublishServer* networkStreamer, DetectorDataContainer& theInputContainer, Args&... extraArguments) const
    {
        for(auto board: theInputContainer)
        {
            for(auto opticalGroup: *board)
            {
                for(auto hybrid: *opticalGroup)
                {
                    for(auto chip: *hybrid)
                    {
                        std::string  myStream = serializeChipContainer(chip, board->getId(), opticalGroup->getId(), hybrid->getId(), extraArguments...);
                        PacketHeader thePacketHeader;
                        thePacketHeader.addPacketHeader(myStream);
                        networkStreamer->broadcast(myStream);
                    }
                }
            }
        }
    }

    // !!! ---------------------------------------------------------------------------- !!! //
    // Serialize
    // !!! ---------------------------------------------------------------------------- !!! //

    template <typename... Args>
    std::string serializeDetectorContainer(DetectorDataContainer& theInputContainer, Args&... extraArguments) const
    {
        std::ostringstream            ouputStream;
        boost::archive::text_oarchive theArchive(ouputStream);

        theArchive << fCalibrationName;
        theArchive << theInputContainer;

        serializeExtraArguments(theArchive, extraArguments...);
        return ouputStream.str();
    }

    template <typename... Args>
    std::string serializeBoardContainer(BoardDataContainer* theInputContainer, Args&... extraArguments) const
    {
        std::ostringstream            ouputStream;
        boost::archive::text_oarchive theArchive(ouputStream);
        uint16_t                      id = theInputContainer->getId();

        theArchive << fCalibrationName;
        theArchive << id;
        theArchive << *theInputContainer;

        serializeExtraArguments(theArchive, extraArguments...);
        return ouputStream.str();
    }

    template <typename... Args>
    std::string serializeOpticalGroupContainer(OpticalGroupDataContainer* theInputContainer, uint16_t boardId, Args&... extraArguments) const
    {
        std::ostringstream            ouputStream;
        boost::archive::text_oarchive theArchive(ouputStream);
        uint16_t                      id = theInputContainer->getId();

        theArchive << fCalibrationName;
        theArchive << boardId;
        theArchive << id;
        theArchive << *theInputContainer;

        serializeExtraArguments(theArchive, extraArguments...);
        return ouputStream.str();
    }

    template <typename... Args>
    std::string serializeHybridContainer(HybridDataContainer* theInputContainer, uint16_t boardId, uint16_t opticalGroupId, Args&... extraArguments) const
    {
        std::ostringstream            ouputStream;
        boost::archive::text_oarchive theArchive(ouputStream);
        uint16_t                      id = theInputContainer->getId();

        theArchive << fCalibrationName;
        theArchive << boardId;
        theArchive << opticalGroupId;
        theArchive << id;
        theArchive << *theInputContainer;

        serializeExtraArguments(theArchive, extraArguments...);
        return ouputStream.str();
    }

    template <typename... Args>
    std::string serializeChipContainer(ChipDataContainer* theInputContainer, uint16_t boardId, uint16_t opticalGroupId, uint16_t hybridId, Args&... extraArguments) const
    {
        std::ostringstream            ouputStream;
        boost::archive::text_oarchive theArchive(ouputStream);
        uint16_t                      id = theInputContainer->getId();

        theArchive << fCalibrationName;
        theArchive << boardId;
        theArchive << opticalGroupId;
        theArchive << hybridId;
        theArchive << id;
        theArchive << *theInputContainer;

        serializeExtraArguments(theArchive, extraArguments...);
        return ouputStream.str();
    }

    // !!! ---------------------------------------------------------------------------- !!! //
    // Deserialize
    // !!! ---------------------------------------------------------------------------- !!! //

    template <typename T, typename SC, typename SH, typename SO, typename SB, typename SD, typename... Args>
    DetectorDataContainer deserializeDetectorContainer(const DetectorContainer* theDetectorContainer, Args&... extraArguments)
    {
        DetectorDataContainer theOutputContainer;
        ContainerFactory::copyAndInitStructure<T, SC, SH, SO, SB, SD>(*theDetectorContainer, theOutputContainer);
        std::istringstream            inputStream(fStream);
        boost::archive::text_iarchive theArchive(inputStream);
        std::string                   inputCalibrationName;
        theArchive >> inputCalibrationName;
        theArchive >> theOutputContainer;
        serializeExtraArguments(theArchive, extraArguments...);

        theOutputContainer.remapIdtoPointer();
        return theOutputContainer;
    }

    template <typename T, typename SC, typename SH, typename SO, typename SB, typename... Args>
    DetectorDataContainer deserializeBoardContainer(const DetectorContainer* theDetectorContainer, Args&... extraArguments)
    {
        DetectorDataContainer theOutputContainer;
        ContainerFactory::copyStructure(*theDetectorContainer, theOutputContainer);
        std::istringstream            inputStream(fStream);
        boost::archive::text_iarchive theArchive(inputStream);
        std::string                   inputCalibrationName;
        theArchive >> inputCalibrationName;
        uint16_t boardId = 65535;
        theArchive >> boardId;

        BoardDataContainer& board = *theOutputContainer.getObject(boardId);
        board.initialize<SB, SO>();
        for(const auto opticalGroup: board)
        {
            opticalGroup->initialize<SO, SH>();
            for(const auto hybrid: *opticalGroup)
            {
                hybrid->initialize<SH, SC>();
                for(const auto chip: *hybrid) { chip->initialize<SC, T>(); }
            }
        }
        theArchive >> board;
        serializeExtraArguments(theArchive, extraArguments...);

        theOutputContainer.remapIdtoPointer();
        return theOutputContainer;
    }

    template <typename T, typename SC, typename SH, typename SO, typename... Args>
    DetectorDataContainer deserializeOpticalGroupContainer(const DetectorContainer* theDetectorContainer, Args&... extraArguments)
    {
        DetectorDataContainer theOutputContainer;
        ContainerFactory::copyStructure(*theDetectorContainer, theOutputContainer);
        std::istringstream            inputStream(fStream);
        boost::archive::text_iarchive theArchive(inputStream);
        std::string                   inputCalibrationName;
        theArchive >> inputCalibrationName;
        uint16_t boardId        = 65535;
        uint16_t opticalGroupId = 65535;
        theArchive >> boardId;
        theArchive >> opticalGroupId;

        OpticalGroupDataContainer& opticalGroup = *theOutputContainer.getObject(boardId)->getObject(opticalGroupId);
        opticalGroup.initialize<SO, SH>();
        for(const auto hybrid: opticalGroup)
        {
            hybrid->initialize<SH, SC>();
            for(const auto chip: *hybrid) { chip->initialize<SC, T>(); }
        }

        theArchive >> opticalGroup;
        serializeExtraArguments(theArchive, extraArguments...);

        theOutputContainer.remapIdtoPointer();
        return theOutputContainer;
    }

    template <typename T, typename SC, typename SH, typename... Args>
    DetectorDataContainer deserializeHybridContainer(const DetectorContainer* theDetectorContainer, Args&... extraArguments)
    {
        DetectorDataContainer theOutputContainer;
        ContainerFactory::copyStructure(*theDetectorContainer, theOutputContainer);
        std::istringstream            inputStream(fStream);
        boost::archive::text_iarchive theArchive(inputStream);
        std::string                   inputCalibrationName;
        theArchive >> inputCalibrationName;
        uint16_t boardId        = 65535;
        uint16_t opticalGroupId = 65535;
        uint16_t hybridId       = 65535;
        theArchive >> boardId;
        theArchive >> opticalGroupId;
        theArchive >> hybridId;

        HybridDataContainer& hybrid = *theOutputContainer.getObject(boardId)->getObject(opticalGroupId)->getObject(hybridId);
        hybrid.initialize<SH, SC>();
        for(auto chip: hybrid) { chip->initialize<SC, T>(); }

        theArchive >> hybrid;
        serializeExtraArguments(theArchive, extraArguments...);

        theOutputContainer.remapIdtoPointer();
        return theOutputContainer;
    }

    template <typename T, typename SC, typename... Args>
    DetectorDataContainer deserializeChipContainer(const DetectorContainer* theDetectorContainer, Args&... extraArguments)
    {
        DetectorDataContainer theOutputContainer;
        ContainerFactory::copyStructure(*theDetectorContainer, theOutputContainer);
        std::istringstream            inputStream(fStream);
        boost::archive::text_iarchive theArchive(inputStream);
        std::string                   inputCalibrationName;
        theArchive >> inputCalibrationName;
        uint16_t boardId        = 65535;
        uint16_t opticalGroupId = 65535;
        uint16_t hybridId       = 65535;
        uint16_t chipId         = 65535;
        theArchive >> boardId;
        theArchive >> opticalGroupId;
        theArchive >> hybridId;
        theArchive >> chipId;

        ChipDataContainer& chip = *theOutputContainer.getObject(boardId)->getObject(opticalGroupId)->getObject(hybridId)->getObject(chipId);
        chip.initialize<SC, T>();

        theArchive >> chip;
        serializeExtraArguments(theArchive, extraArguments...);

        theOutputContainer.remapIdtoPointer();
        return theOutputContainer;
    }

    // template <typename T, typename... Args>
    // DetectorDataContainer deserializeChannelContainer(const DetectorContainer* theDetectorContainer, Args&... extraArguments)
    // {
    //     DetectorDataContainer theOutputContainer;
    //     ContainerFactory::copyStructure(*theDetectorContainer, theOutputContainer);
    //     std::istringstream inputStream(fStream);
    //     boost::archive::text_iarchive theArchive(inputStream);
    //     std::string inputCalibrationName;
    //     theArchive >> inputCalibrationName;
    //     uint16_t boardId = 65535;
    //     uint16_t opticalGroupId = 65535;
    //     uint16_t hybridId = 65535;
    //     uint16_t chipId = 65535;
    //     theArchive >> boardId;
    //     theArchive >> opticalGroupId;
    //     theArchive >> hybridId;
    //     theArchive >> chipId;

    //     ChannelDataContainer* channel = theOutputContainer.getObject(boardId)->getObject(opticalGroupId)->getObject(hybridId)->getObject(chipId);
    //     chip->initializeChannels<T>();

    //     theArchive >> channel;
    //     serializeExtraArguments(theArchive, extraArguments...);
    //     return theOutputContainer;
    // }

  private:
    template <typename T, typename... Args>
    void serializeExtraArguments(T& theArchive, Args&... extraArguments) const
    {
        auto extraArgumentTuple = std::tuple<Args&...>(extraArguments...);
        serialize(theArchive, extraArgumentTuple);
    }

    std::string fCalibrationName;
    std::string fStream;
};

#endif
