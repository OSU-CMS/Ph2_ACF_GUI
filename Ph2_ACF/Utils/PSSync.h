/*

        \file                          Occupancy.h
        \brief                         Generic Occupancy for DAQ
        \author                        Fabio Ravera, Lorenzo Uplegger
        \version                       1.0
        \date                          08/04/19
        Support :                      mail to : fabio.ravera@cern.ch

 */

#ifndef __PSSYNC_H__
#define __PSSYNC_H__

#include "Utils/Container.h"
#include "Utils/D19cCic2Event.h"
#include "Utils/GenericDataArray.h"
#include <iostream>
#include <math.h>

template <size_t StripSize, size_t PixelSize, size_t StubSize>
class PSSync //: public streammable
{
  public:
    GenericDataArray<Ph2_HwInterface::StripClusterPS, StripSize> fSClusters;
    GenericDataArray<Ph2_HwInterface::PixelClusterPS, PixelSize> fPClusters;
    GenericDataArray<Ph2_HwInterface::EventStub, StubSize>       fStubs;
};

#endif
