/*

        \file                          Occupancy.h
        \brief                         Generic Occupancy for DAQ
        \author                        Fabio Ravera, Lorenzo Uplegger
        \version                       1.0
        \date                          08/04/19
        Support :                      mail to : fabio.ravera@cern.ch

 */

#ifndef __ADCSLOPE_H__
#define __ADCSLOPE_H__

#include "Utils/Container.h"
#include <iostream>
#include <math.h>

class ADCSlope //: public streammable
{
  public:
    ADCSlope() : fSlope(0), fOffset(0), fADC_GND(0), fADC_VBG(0), fMeasured_VBG(0) { ; }

    void print(void) { std::cout << fSlope << std::endl; }

    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
        theArchive & fSlope;
        theArchive & fOffset;
        theArchive & fADC_GND;
        theArchive & fADC_VBG;
        theArchive & fMeasured_VBG;
    }

    float fSlope;
    float fOffset;
    float fADC_GND;
    float fADC_VBG;
    float fMeasured_VBG;
};

#endif
