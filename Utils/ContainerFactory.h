/*

        \file                          ContainerFactory.h
        \brief                         Container factory for DAQ
        \author                        Fabio Ravera, Lorenzo Uplegger
        \version                       1.0
        \date                          08/04/19
        Support :                      mail to : fabio.ravera@cern.ch

 */

#ifndef __CONTAINERFACTORY_H__
#define __CONTAINERFACTORY_H__

#include "HWDescription/BeBoard.h"
#include "HWDescription/Hybrid.h"
#include "HWDescription/OpticalGroup.h"
#include "HWDescription/ReadoutChip.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"
#include "Utils/EmptyContainer.h"
#include <iostream>
#include <map>
#include <vector>

class ChannelGroupBase;

namespace ContainerFactory
{

// Copy structure

// Copy structure -- Chip

inline void copyStructure(const ChipContainer& original, ChipDataContainer& copy) { copy.setNumberOfChannels(original.getNumberOfRows(), original.getNumberOfCols()); }

// Copy structure -- Hybrid

inline void copyStructure(const HybridContainer& original, HybridDataContainer& copy)
{
    copy.setId(original.getId());
    for(const auto chip: original)
    {
        copy.addChipDataContainer(chip->getId(), chip->getNumberOfRows(), chip->getNumberOfCols());
        copyStructure(*chip, *copy.back());
    }
}

// Copy structure -- Optical Group

inline void copyStructure(const OpticalGroupContainer& original, OpticalGroupDataContainer& copy)
{
    copy.setId(original.getId());
    for(const auto hybrid: original)
    {
        HybridDataContainer* copyHybrid = copy.addHybridDataContainer(hybrid->getId());
        copyStructure(*hybrid, *copyHybrid);
    }
}

// Copy structure -- Board

inline void copyStructure(const BoardContainer& original, BoardDataContainer& copy)
{
    copy.setId(original.getId());
    for(const auto opticalGroup: original)
    {
        OpticalGroupDataContainer* copyOpticalGroup = copy.addOpticalGroupDataContainer(opticalGroup->getId());
        copyStructure(*opticalGroup, *copyOpticalGroup);
    }
}

// Copy structure -- Detector

inline void copyStructure(const DetectorContainer& original, DetectorDataContainer& copy)
{
    for(const auto board: original)
    {
        BoardDataContainer* copyBoard = copy.addBoardDataContainer(board->getId());
        copyStructure(*board, *copyBoard);
    }
}

// Copy and init structure no initializer

// Copy and init structure no initializer -- Chip

template <typename T, typename SC>
void copyAndInitStructure(const ChipContainer& original, ChipDataContainer& copy)
{
    copy.setId(original.getId());
    copy.setNumberOfChannels(original.getNumberOfRows(), original.getNumberOfCols());
    copy.initialize<SC, T>();
}

template <typename T>
void copyAndInitStructure(const ChipContainer& original, ChipDataContainer& copy)
{
    copyAndInitStructure<T, T>(original, copy);
}

template <typename T>
void copyAndInitChannel(const ChipContainer& original, ChipDataContainer& copy)
{
    copyAndInitStructure<T, EmptyContainer>(original, copy);
}

template <typename T>
void copyAndInitChip(const ChipContainer& original, ChipDataContainer& copy)
{
    copyAndInitStructure<EmptyContainer, T>(original, copy);
}

// Copy and init structure no initializer -- Hybrid

template <typename T, typename SC, typename SH>
void copyAndInitStructure(const HybridContainer& original, HybridDataContainer& copy)
{
    copy.setId(original.getId());
    copy.initialize<SH, SC>();
    for(const auto chip: original)
    {
        copy.addChipDataContainer(chip->getId(), chip->getNumberOfRows(), chip->getNumberOfCols());
        copyAndInitStructure<T, SC>(*chip, *copy.back());
    }
}

template <typename T>
void copyAndInitStructure(const HybridContainer& original, HybridDataContainer& copy)
{
    copyAndInitStructure<T, T, T>(original, copy);
}

template <typename T, typename SC>
void copyAndInitStructure(const HybridContainer& original, HybridDataContainer& copy)
{
    copyAndInitStructure<T, SC, SC>(original, copy);
}

template <typename T>
void copyAndInitChannel(const HybridContainer& original, HybridDataContainer& copy)
{
    copyAndInitStructure<T, EmptyContainer, EmptyContainer>(original, copy);
}

template <typename T>
void copyAndInitChip(const HybridContainer& original, HybridDataContainer& copy)
{
    copyAndInitStructure<EmptyContainer, T, EmptyContainer>(original, copy);
}

template <typename T>
void copyAndInitHybrid(const HybridContainer& original, HybridDataContainer& copy)
{
    copyAndInitStructure<EmptyContainer, EmptyContainer, T>(original, copy);
}

// Copy and init structure no initializer -- Optical group

template <typename T, typename SC, typename SH, typename SO>
void copyAndInitStructure(const OpticalGroupContainer& original, OpticalGroupDataContainer& copy)
{
    copy.setId(original.getId());
    copy.initialize<SO, SH>();
    for(const auto hybrid: original)
    {
        HybridDataContainer* copyHybrid = copy.addHybridDataContainer(hybrid->getId());
        copyAndInitStructure<T, SC, SH>(*hybrid, *copyHybrid);
    }
}

template <typename T>
void copyAndInitStructure(const OpticalGroupContainer& original, OpticalGroupDataContainer& copy)
{
    copyAndInitStructure<T, T, T, T>(original, copy);
}

template <typename T, typename SC>
void copyAndInitStructure(const OpticalGroupContainer& original, OpticalGroupDataContainer& copy)
{
    copyAndInitStructure<T, SC, SC, SC>(original, copy);
}

template <typename T>
void copyAndInitChannel(const OpticalGroupContainer& original, OpticalGroupDataContainer& copy)
{
    copyAndInitStructure<T, EmptyContainer, EmptyContainer, EmptyContainer>(original, copy);
}

template <typename T>
void copyAndInitChip(const OpticalGroupContainer& original, OpticalGroupDataContainer& copy)
{
    copyAndInitStructure<EmptyContainer, T, EmptyContainer, EmptyContainer>(original, copy);
}

template <typename T>
void copyAndInitHybrid(const OpticalGroupContainer& original, OpticalGroupDataContainer& copy)
{
    copyAndInitStructure<EmptyContainer, EmptyContainer, T, EmptyContainer>(original, copy);
}

template <typename T>
void copyAndInitOpticalGroup(const OpticalGroupContainer& original, OpticalGroupDataContainer& copy)
{
    copyAndInitStructure<EmptyContainer, EmptyContainer, EmptyContainer, T>(original, copy);
}

// Copy and init structure no initializer -- Board

template <typename T, typename SC, typename SH, typename SO, typename SB>
void copyAndInitStructure(const BoardContainer& original, BoardDataContainer& copy)
{
    copy.setId(original.getId());
    copy.initialize<SB, SO>();
    for(const auto opticalGroup: original)
    {
        OpticalGroupDataContainer* copyOpticalGroup = copy.addOpticalGroupDataContainer(opticalGroup->getId());
        copyAndInitStructure<T, SC, SH, SO>(*opticalGroup, *copyOpticalGroup);
    }
}

template <typename T>
void copyAndInitStructure(const BoardContainer& original, BoardDataContainer& copy)
{
    copyAndInitStructure<T, T, T, T, T>(original, copy);
}

template <typename T, typename SC>
void copyAndInitStructure(const BoardContainer& original, BoardDataContainer& copy)
{
    copyAndInitStructure<T, SC, SC, SC, SC>(original, copy);
}

template <typename T>
void copyAndInitChannel(const BoardContainer& original, BoardDataContainer& copy)
{
    copyAndInitStructure<T, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer>(original, copy);
}

template <typename T>
void copyAndInitChip(const BoardContainer& original, BoardDataContainer& copy)
{
    copyAndInitStructure<EmptyContainer, T, EmptyContainer, EmptyContainer, EmptyContainer>(original, copy);
}

template <typename T>
void copyAndInitHybrid(const BoardContainer& original, BoardDataContainer& copy)
{
    copyAndInitStructure<EmptyContainer, EmptyContainer, T, EmptyContainer, EmptyContainer>(original, copy);
}

template <typename T>
void copyAndInitOpticalGroup(const BoardContainer& original, BoardDataContainer& copy)
{
    copyAndInitStructure<EmptyContainer, EmptyContainer, EmptyContainer, T, EmptyContainer>(original, copy);
}

template <typename T>
void copyAndInitBoard(const BoardContainer& original, BoardDataContainer& copy)
{
    copyAndInitStructure<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, T>(original, copy);
}

// Copy and init structure no initializer -- Detector

template <typename T, typename SC, typename SH, typename SO, typename SB, typename SD>
void copyAndInitStructure(const DetectorContainer& original, DetectorDataContainer& copy)
{
    copy.cleanDataStored();
    copy.reset();
    copy.initialize<SD, SB>();
    for(const auto board: original)
    {
        BoardDataContainer* copyBoard = copy.addBoardDataContainer(board->getId());
        copyAndInitStructure<T, SC, SH, SO, SB>(*board, *copyBoard);
    }
}

template <typename T>
void copyAndInitStructure(const DetectorContainer& original, DetectorDataContainer& copy)
{
    copyAndInitStructure<T, T, T, T, T, T>(original, copy);
}

template <typename T, typename SC>
void copyAndInitStructure(const DetectorContainer& original, DetectorDataContainer& copy)
{
    copyAndInitStructure<T, SC, SC, SC, SC, SC>(original, copy);
}

template <typename T>
void copyAndInitChannel(const DetectorContainer& original, DetectorDataContainer& copy)
{
    copyAndInitStructure<T, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer>(original, copy);
}

template <typename T>
void copyAndInitChip(const DetectorContainer& original, DetectorDataContainer& copy)
{
    copyAndInitStructure<EmptyContainer, T, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer>(original, copy);
}

template <typename T>
void copyAndInitHybrid(const DetectorContainer& original, DetectorDataContainer& copy)
{
    copyAndInitStructure<EmptyContainer, EmptyContainer, T, EmptyContainer, EmptyContainer, EmptyContainer>(original, copy);
}

template <typename T>
void copyAndInitOpticalGroup(const DetectorContainer& original, DetectorDataContainer& copy)
{
    copyAndInitStructure<EmptyContainer, EmptyContainer, EmptyContainer, T, EmptyContainer, EmptyContainer>(original, copy);
}

template <typename T>
void copyAndInitBoard(const DetectorContainer& original, DetectorDataContainer& copy)
{
    copyAndInitStructure<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, T, EmptyContainer>(original, copy);
}

template <typename T>
void copyAndInitDetector(const DetectorContainer& original, DetectorDataContainer& copy)
{
    copyAndInitStructure<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, T>(original, copy);
}

// Copy and init structure with initializer

// Copy and init structure with initializer -- Chip

template <typename T, typename SC>
void copyAndInitStructure(const ChipContainer& original, ChipDataContainer& copy, T& channel, SC& chipSummay)
{
    copy.setId(original.getId());
    copy.setNumberOfChannels(original.getNumberOfRows(), original.getNumberOfCols());
    static_cast<ChipDataContainer&>(copy).initialize<SC, T>(chipSummay, channel);
}

template <typename T>
void copyAndInitStructure(const ChipContainer& original, ChipDataContainer& copy, T& channel)
{
    copyAndInitStructure<T, T>(original, copy, channel, channel);
}

template <typename T>
void copyAndInitChannel(const ChipContainer& original, ChipDataContainer& copy, T& channel)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<T, EmptyContainer>(original, copy, channel, theEmpty);
}

template <typename T>
void copyAndInitChip(const ChipContainer& original, ChipDataContainer& copy, T& chipSummary)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<EmptyContainer, T>(original, copy, theEmpty, chipSummary);
}

// Copy and init structure with initializer -- Hybrid

template <typename T, typename SC, typename SH>
void copyAndInitStructure(const HybridContainer& original, HybridDataContainer& copy, T& channel, SC& chipSummay, SH& hybridSummary)
{
    copy.setId(original.getId());
    static_cast<HybridDataContainer&>(copy).initialize<SH, SC>(hybridSummary);
    for(const ChipContainer* chip: original)
    {
        copy.addChipDataContainer(chip->getId(), chip->getNumberOfRows(), chip->getNumberOfCols());
        copyAndInitStructure<T, SC>(*chip, *copy.back(), channel, chipSummay);
    }
}

template <typename T>
void copyAndInitStructure(const HybridContainer& original, HybridDataContainer& copy, T& channel)
{
    copyAndInitStructure<T, T, T>(original, copy, channel, channel, channel);
}

template <typename T, typename S>
void copyAndInitStructure(const HybridContainer& original, HybridDataContainer& copy, T& channel, S& summay)
{
    copyAndInitStructure<T, S, S>(original, copy, channel, summay, summay);
}

template <typename T>
void copyAndInitChannel(const HybridContainer& original, HybridDataContainer& copy, T& channel)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<T, EmptyContainer, EmptyContainer>(original, copy, channel, theEmpty, theEmpty);
}

template <typename T>
void copyAndInitChip(const HybridContainer& original, HybridDataContainer& copy, T& chipSummary)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<EmptyContainer, T, EmptyContainer>(original, copy, theEmpty, chipSummary, theEmpty);
}

template <typename T>
void copyAndInitHybrid(const HybridContainer& original, HybridDataContainer& copy, T& hybridSummary)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<EmptyContainer, EmptyContainer, T>(original, copy, theEmpty, theEmpty, hybridSummary);
}

// Copy and init structure with initializer -- Optical group

template <typename T, typename SC, typename SH, typename SO>
void copyAndInitStructure(const OpticalGroupContainer& original, OpticalGroupDataContainer& copy, T& channel, SC& chipSummay, SH& hybridSummary, SO& opticalGroupSummary)
{
    copy.setId(original.getId());
    static_cast<OpticalGroupDataContainer&>(copy).initialize<SO, SH>(opticalGroupSummary);
    for(const HybridContainer* hybrid: original)
    {
        HybridDataContainer* copyHybrid = copy.addHybridDataContainer(hybrid->getId());
        copyAndInitStructure<T, SC, SH>(*hybrid, *copyHybrid, channel, chipSummay, hybridSummary);
    }
}

template <typename T>
void copyAndInitStructure(const OpticalGroupContainer& original, OpticalGroupDataContainer& copy, T& channel)
{
    copyAndInitStructure<T, T, T, T>(original, copy, channel, channel, channel, channel);
}

template <typename T, typename S>
void copyAndInitStructure(const OpticalGroupContainer& original, OpticalGroupDataContainer& copy, T& channel, S& summay)
{
    copyAndInitStructure<T, S, S, S>(original, copy, channel, summay, summay, summay);
}

template <typename T>
void copyAndInitChannel(const OpticalGroupContainer& original, OpticalGroupDataContainer& copy, T& channel)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<T, EmptyContainer, EmptyContainer, EmptyContainer>(original, copy, channel, theEmpty, theEmpty, theEmpty);
}

template <typename T>
void copyAndInitChip(const OpticalGroupContainer& original, OpticalGroupDataContainer& copy, T& chipSummary)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<EmptyContainer, T, EmptyContainer, EmptyContainer>(original, copy, theEmpty, chipSummary, theEmpty, theEmpty);
}

template <typename T>
void copyAndInitHybrid(const OpticalGroupContainer& original, OpticalGroupDataContainer& copy, T& hybridSummary)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<EmptyContainer, EmptyContainer, T, EmptyContainer>(original, copy, theEmpty, theEmpty, hybridSummary, theEmpty);
}

template <typename T>
void copyAndInitOpticalGroup(const OpticalGroupContainer& original, OpticalGroupDataContainer& copy, T& opticalGroupSummary)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<EmptyContainer, EmptyContainer, EmptyContainer, T>(original, copy, theEmpty, theEmpty, theEmpty, opticalGroupSummary);
}

// Copy and init structure with initializer -- Board

template <typename T, typename SC, typename SH, typename SO, typename SB>
void copyAndInitStructure(const BoardContainer& original, BoardDataContainer& copy, T& channel, SC& chipSummay, SH& hybridSummary, SO& opticalGroupSummary, SB& boardSummary)
{
    copy.setId(original.getId());
    static_cast<BoardDataContainer&>(copy).initialize<SB, SO>(boardSummary);
    for(const OpticalGroupContainer* opticalGroup: original)
    {
        OpticalGroupDataContainer* copyOpticalGroup = copy.addOpticalGroupDataContainer(opticalGroup->getId());
        copyAndInitStructure<T, SC, SH, SO>(*opticalGroup, *copyOpticalGroup, channel, chipSummay, hybridSummary, opticalGroupSummary);
    }
}

template <typename T>
void copyAndInitStructure(const BoardContainer& original, BoardDataContainer& copy, T& channel)
{
    copyAndInitStructure<T, T, T, T, T>(original, copy, channel, channel, channel, channel, channel);
}

template <typename T, typename S>
void copyAndInitStructure(const BoardContainer& original, BoardDataContainer& copy, T& channel, S& summay)
{
    copyAndInitStructure<T, S, S, S, S>(original, copy, channel, summay, summay, summay, summay);
}

template <typename T>
void copyAndInitChannel(const BoardContainer& original, BoardDataContainer& copy, T& channel)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<T, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer>(original, copy, channel, theEmpty, theEmpty, theEmpty, theEmpty);
}

template <typename T>
void copyAndInitChip(const BoardContainer& original, BoardDataContainer& copy, T& chipSummary)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<EmptyContainer, T, EmptyContainer, EmptyContainer, EmptyContainer>(original, copy, theEmpty, chipSummary, theEmpty, theEmpty, theEmpty);
}

template <typename T>
void copyAndInitHybrid(const BoardContainer& original, BoardDataContainer& copy, T& hybridSummary)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<EmptyContainer, EmptyContainer, T, EmptyContainer, EmptyContainer>(original, copy, theEmpty, theEmpty, hybridSummary, theEmpty, theEmpty);
}

template <typename T>
void copyAndInitOpticalGroup(const BoardContainer& original, BoardDataContainer& copy, T& opticalGroupSummary)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<EmptyContainer, EmptyContainer, EmptyContainer, T, EmptyContainer>(original, copy, theEmpty, theEmpty, theEmpty, opticalGroupSummary, theEmpty);
}

template <typename T>
void copyAndInitBoard(const BoardContainer& original, BoardDataContainer& copy, T& boardSummary)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, T>(original, copy, theEmpty, theEmpty, theEmpty, theEmpty, boardSummary);
}

// Copy and init structure with initializer -- Detector

template <typename T, typename SC, typename SH, typename SO, typename SB, typename SD>
void copyAndInitStructure(const DetectorContainer& original, DetectorDataContainer& copy, T& channel, SC& chipSummay, SH& hybridSummary, SO& opticalGroupSummary, SB& boardSummary, SD& detectorSummary)
{
    static_cast<DetectorDataContainer&>(copy).cleanDataStored();
    static_cast<DetectorDataContainer&>(copy).reset();
    static_cast<DetectorDataContainer&>(copy).initialize<SD, SB>(detectorSummary);
    for(const BoardContainer* board: original)
    {
        BoardDataContainer* copyBoard = copy.addBoardDataContainer(board->getId());
        copyAndInitStructure<T, SC, SH, SO, SB>(*board, *copyBoard, channel, chipSummay, hybridSummary, opticalGroupSummary, boardSummary);
    }
}

template <typename T>
void copyAndInitStructure(const DetectorContainer& original, DetectorDataContainer& copy, T& channel)
{
    copyAndInitStructure<T, T, T, T, T, T>(original, copy, channel, channel, channel, channel, channel, channel);
}

template <typename T, typename S>
void copyAndInitStructure(const DetectorContainer& original, DetectorDataContainer& copy, T& channel, S& summay)
{
    copyAndInitStructure<T, S, S, S, S, S>(original, copy, channel, summay, summay, summay, summay, summay);
}

template <typename T>
void copyAndInitChannel(const DetectorContainer& original, DetectorDataContainer& copy, T& channel)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<T, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer>(original, copy, channel, theEmpty, theEmpty, theEmpty, theEmpty, theEmpty);
}

template <typename T>
void copyAndInitChip(const DetectorContainer& original, DetectorDataContainer& copy, T& chipSummary)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<EmptyContainer, T, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer>(original, copy, theEmpty, chipSummary, theEmpty, theEmpty, theEmpty, theEmpty);
}

template <typename T>
void copyAndInitHybrid(const DetectorContainer& original, DetectorDataContainer& copy, T& hybridSummary)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<EmptyContainer, EmptyContainer, T, EmptyContainer, EmptyContainer, EmptyContainer>(original, copy, theEmpty, theEmpty, hybridSummary, theEmpty, theEmpty, theEmpty);
}

template <typename T>
void copyAndInitOpticalGroup(const DetectorContainer& original, DetectorDataContainer& copy, T& opticalGroupSummary)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<EmptyContainer, EmptyContainer, EmptyContainer, T, EmptyContainer, EmptyContainer>(original, copy, theEmpty, theEmpty, theEmpty, opticalGroupSummary, theEmpty, theEmpty);
}

template <typename T>
void copyAndInitBoard(const DetectorContainer& original, DetectorDataContainer& copy, T& boardSummary)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, T, EmptyContainer>(original, copy, theEmpty, theEmpty, theEmpty, theEmpty, boardSummary, theEmpty);
}

template <typename T>
void copyAndInitDetector(const DetectorContainer& original, DetectorDataContainer& copy, T& detectorSummary)
{
    EmptyContainer theEmpty;
    copyAndInitStructure<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, T>(original, copy, theEmpty, theEmpty, theEmpty, theEmpty, theEmpty, detectorSummary);
}

// Reinitialize structure

// Reinitialize structure -- Chip

template <typename T, typename SC>
void reinitializeContainer(ChipDataContainer& theDataContainer, T& channel, SC& chipSummary)
{
    theDataContainer.resetSummary<SC, T>(chipSummary);
    theDataContainer.resetChannels<T>(channel);
}

template <typename T>
void reinitializeContainer(ChipDataContainer& theDataContainer, T& channel)
{
    reinitializeContainer<T, T>(theDataContainer, channel, channel);
}

template <typename T>
void reinitializeChannel(ChipDataContainer& theDataContainer, T& channel)
{
    EmptyContainer theEmpty;
    reinitializeContainer<T, EmptyContainer>(theDataContainer, channel, theEmpty);
}

template <typename T>
void reinitializeChip(ChipDataContainer& theDataContainer, T& chipSummary)
{
    EmptyContainer theEmpty;
    reinitializeContainer<EmptyContainer, T>(theDataContainer, theEmpty, chipSummary);
}

// Reinitialize structure -- Hybrid

template <typename T, typename SC, typename SH>
void reinitializeContainer(HybridDataContainer& theDataContainer, T& channel, SC& chipSummary, SH& hybridSummary)
{
    theDataContainer.resetSummary<SH, SC>(hybridSummary);
    for(auto chip: theDataContainer) { reinitializeContainer<T, SC>(*chip, channel, chipSummary); }
}

template <typename T, typename S>
void reinitializeContainer(HybridDataContainer& theDataContainer, T& channel, S& summay)
{
    reinitializeContainer<T, S, S>(theDataContainer, channel, summay, summay);
}

template <typename T>
void reinitializeContainer(HybridDataContainer& theDataContainer, T& channel)
{
    reinitializeContainer<T, T, T>(theDataContainer, channel, channel, channel);
}

template <typename T>
void reinitializeChannel(HybridDataContainer& theDataContainer, T& channel)
{
    EmptyContainer theEmpty;
    reinitializeContainer<T, EmptyContainer, EmptyContainer>(theDataContainer, channel, theEmpty, theEmpty);
}

template <typename T>
void reinitializeChip(HybridDataContainer& theDataContainer, T& chipSummary)
{
    EmptyContainer theEmpty;
    reinitializeContainer<EmptyContainer, T, EmptyContainer>(theDataContainer, theEmpty, chipSummary, theEmpty);
}

template <typename T>
void reinitializeHybrid(HybridDataContainer& theDataContainer, T& hybridSummary)
{
    EmptyContainer theEmpty;
    reinitializeContainer<EmptyContainer, EmptyContainer, T>(theDataContainer, theEmpty, theEmpty, hybridSummary);
}

// Reinitialize structure -- Optical Group

template <typename T, typename SC, typename SH, typename SO>
void reinitializeContainer(OpticalGroupDataContainer& theDataContainer, T& channel, SC& chipSummary, SH& hybridSummary, SO& opticalGroupSummary)
{
    theDataContainer.resetSummary<SO, SH>(opticalGroupSummary);
    for(auto hybrid: theDataContainer) { reinitializeContainer<T, SC, SH>(*hybrid, channel, chipSummary, hybridSummary); }
}

template <typename T, typename S>
void reinitializeContainer(OpticalGroupDataContainer& theDataContainer, T& channel, S& summay)
{
    reinitializeContainer<T, S, S, S>(theDataContainer, channel, summay, summay, summay);
}

template <typename T>
void reinitializeContainer(OpticalGroupDataContainer& theDataContainer, T& channel)
{
    reinitializeContainer<T, T, T, T>(theDataContainer, channel, channel, channel, channel);
}

template <typename T>
void reinitializeChannel(OpticalGroupDataContainer& theDataContainer, T& channel)
{
    EmptyContainer theEmpty;
    reinitializeContainer<T, EmptyContainer, EmptyContainer, EmptyContainer>(theDataContainer, channel, theEmpty, theEmpty, theEmpty);
}

template <typename T>
void reinitializeChip(OpticalGroupDataContainer& theDataContainer, T& chipSummary)
{
    EmptyContainer theEmpty;
    reinitializeContainer<EmptyContainer, T, EmptyContainer, EmptyContainer>(theDataContainer, theEmpty, chipSummary, theEmpty, theEmpty);
}

template <typename T>
void reinitializeHybrid(OpticalGroupDataContainer& theDataContainer, T& hybridSummary)
{
    EmptyContainer theEmpty;
    reinitializeContainer<EmptyContainer, EmptyContainer, T, EmptyContainer>(theDataContainer, theEmpty, theEmpty, hybridSummary, theEmpty);
}

template <typename T>
void reinitializeOpticalGroup(OpticalGroupDataContainer& theDataContainer, T& opticalGroupSummary)
{
    EmptyContainer theEmpty;
    reinitializeContainer<EmptyContainer, EmptyContainer, EmptyContainer, T>(theDataContainer, theEmpty, theEmpty, theEmpty, opticalGroupSummary);
}

// Reinitialize structure -- Board

template <typename T, typename SC, typename SH, typename SO, typename SB>
void reinitializeContainer(BoardDataContainer& theDataContainer, T& channel, SC& chipSummary, SH& hybridSummary, SO& opticalGroupSummary, SB& boardSummary)
{
    theDataContainer.resetSummary<SB, SO>(boardSummary);
    for(auto opticalGroup: theDataContainer) { reinitializeContainer<T, SC, SH, SO>(*opticalGroup, channel, chipSummary, hybridSummary, opticalGroupSummary); }
}

template <typename T, typename S>
void reinitializeContainer(BoardDataContainer& theDataContainer, T& channel, S& summay)
{
    reinitializeContainer<T, S, S, S, S>(theDataContainer, channel, summay, summay, summay, summay);
}

template <typename T>
void reinitializeContainer(BoardDataContainer& theDataContainer, T& channel)
{
    reinitializeContainer<T, T, T, T, T>(theDataContainer, channel, channel, channel, channel, channel);
}

template <typename T>
void reinitializeChannel(BoardDataContainer& theDataContainer, T& channel)
{
    EmptyContainer theEmpty;
    reinitializeContainer<T, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer>(theDataContainer, channel, theEmpty, theEmpty, theEmpty, theEmpty);
}

template <typename T>
void reinitializeChip(BoardDataContainer& theDataContainer, T& chipSummary)
{
    EmptyContainer theEmpty;
    reinitializeContainer<EmptyContainer, T, EmptyContainer, EmptyContainer, EmptyContainer>(theDataContainer, theEmpty, chipSummary, theEmpty, theEmpty, theEmpty);
}

template <typename T>
void reinitializeHybrid(BoardDataContainer& theDataContainer, T& hybridSummary)
{
    EmptyContainer theEmpty;
    reinitializeContainer<EmptyContainer, EmptyContainer, T, EmptyContainer, EmptyContainer>(theDataContainer, theEmpty, theEmpty, hybridSummary, theEmpty, theEmpty);
}

template <typename T>
void reinitializeOpticalGroup(BoardDataContainer& theDataContainer, T& opticalGroupSummary)
{
    EmptyContainer theEmpty;
    reinitializeContainer<EmptyContainer, EmptyContainer, EmptyContainer, T, EmptyContainer>(theDataContainer, theEmpty, theEmpty, theEmpty, opticalGroupSummary, theEmpty);
}

template <typename T>
void reinitializeBoard(BoardDataContainer& theDataContainer, T& boardSummary)
{
    EmptyContainer theEmpty;
    reinitializeContainer<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, T>(theDataContainer, theEmpty, theEmpty, theEmpty, theEmpty, boardSummary);
}

// Reinitialize structure -- Detector

template <typename T, typename SC, typename SH, typename SO, typename SB, typename SD>
void reinitializeContainer(DetectorDataContainer& theDataContainer, T& channel, SC& chipSummary, SH& hybridSummary, SO& opticalGroupSummary, SB& boardSummary, SD& detectorSummary)
{
    theDataContainer.resetSummary<SD, SB>(detectorSummary);
    for(auto board: theDataContainer) { reinitializeContainer<T, SC, SH, SO, SB>(*board, channel, chipSummary, hybridSummary, opticalGroupSummary, boardSummary); }
}

template <typename T, typename S>
void reinitializeContainer(DetectorDataContainer& theDataContainer, T& channel, S& summay)
{
    reinitializeContainer<T, S, S, S, S, S>(theDataContainer, channel, summay, summay, summay, summay, summay);
}

template <typename T>
void reinitializeContainer(DetectorDataContainer& theDataContainer, T& channel)
{
    reinitializeContainer<T, T, T, T, T, T>(theDataContainer, channel, channel, channel, channel, channel, channel);
}

template <typename T>
void reinitializeChannel(DetectorDataContainer& theDataContainer, T& channel)
{
    EmptyContainer theEmpty;
    reinitializeContainer<T, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer>(theDataContainer, channel, theEmpty, theEmpty, theEmpty, theEmpty, theEmpty);
}

template <typename T>
void reinitializeChip(DetectorDataContainer& theDataContainer, T& chipSummary)
{
    EmptyContainer theEmpty;
    reinitializeContainer<EmptyContainer, T, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer>(theDataContainer, theEmpty, chipSummary, theEmpty, theEmpty, theEmpty, theEmpty);
}

template <typename T>
void reinitializeHybrid(DetectorDataContainer& theDataContainer, T& hybridSummary)
{
    EmptyContainer theEmpty;
    reinitializeContainer<EmptyContainer, EmptyContainer, T, EmptyContainer, EmptyContainer, EmptyContainer>(theDataContainer, theEmpty, theEmpty, hybridSummary, theEmpty, theEmpty, theEmpty);
}

template <typename T>
void reinitializeOpticalGroup(DetectorDataContainer& theDataContainer, T& opticalGroupSummary)
{
    EmptyContainer theEmpty;
    reinitializeContainer<EmptyContainer, EmptyContainer, EmptyContainer, T, EmptyContainer, EmptyContainer>(theDataContainer, theEmpty, theEmpty, theEmpty, opticalGroupSummary, theEmpty, theEmpty);
}

template <typename T>
void reinitializeBoard(DetectorDataContainer& theDataContainer, T& boardSummary)
{
    EmptyContainer theEmpty;
    reinitializeContainer<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, T, EmptyContainer>(theDataContainer, theEmpty, theEmpty, theEmpty, theEmpty, boardSummary, theEmpty);
}

template <typename T>
void reinitializeDetector(DetectorDataContainer& theDataContainer, T& detectorSummary)
{
    EmptyContainer theEmpty;
    reinitializeContainer<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, T>(theDataContainer, theEmpty, theEmpty, theEmpty, theEmpty, theEmpty, detectorSummary);
}

} // namespace ContainerFactory

#endif
