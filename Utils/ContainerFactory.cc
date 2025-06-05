/*

        \file                          ContainerFactory.cc
        \brief                         Container factory for DAQ
        \author                        Fabio Ravera, Lorenzo Uplegger
        \version                       1.0
        \date                          08/04/19
        Support :                      mail to : fabio.ravera@cern.ch

 */

#include "Utils/ContainerFactory.h"
#include "HWDescription/BeBoard.h"
#include "HWDescription/Hybrid.h"
#include "HWDescription/OpticalGroup.h"
#include "HWDescription/ReadoutChip.h"
#include "Utils/ChannelGroupHandler.h"
#include "Utils/Container.h"

// void ContainerFactory::copyStructure(const DetectorContainer& original, DetectorDataContainer& copy)
// {
//     for(const auto board: original)
//     {
//         BoardDataContainer* copyBoard = copy.addBoardDataContainer(board->getId());
//         for(const auto opticalGroup: *board)
//         {
//             OpticalGroupDataContainer* copyOpticalGroup = copyBoard->addOpticalGroupDataContainer(opticalGroup->getId());

//             for(const auto hybrid: *opticalGroup)
//             {
//                 HybridDataContainer* copyHybrid = copyOpticalGroup->addHybridDataContainer(hybrid->getId());
//                 for(const auto chip: *hybrid) { copyHybrid->addChipDataContainer(chip->getId(), chip->getNumberOfRows(), chip->getNumberOfCols()); }
//             }
//         }
//     }
// }
