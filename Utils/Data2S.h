/*

        \file                          Occupancy.h
        \brief                         Generic Occupancy for DAQ
        \author                        Fabio Ravera, Lorenzo Uplegger
        \version                       1.0
        \date                          08/04/19
        Support :                      mail to : fabio.ravera@cern.ch

 */

#ifndef __DATA2S_H__
#define __DATA2S_H__

#include "Utils/Container.h"
#include "Utils/D19cCic2Event.h"
#include "Utils/GenericDataArray.h"
#include <iostream>
#include <math.h>

template <size_t ClusterSize, size_t StubSize>
class Data2S //: public streammable
{
  public:
    GenericDataArray<Ph2_HwInterface::Cluster2S, ClusterSize> fClusters;
    GenericDataArray<Ph2_HwInterface::EventStub, StubSize>    fStubs;
};

#endif
