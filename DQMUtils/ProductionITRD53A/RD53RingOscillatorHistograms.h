/*!
  \file                  RD53RingOscillatorHistograms.h
  \brief                 Header file of RingOscillator histograms
  \author                Umberto MOLINATTI
  \version               1.0
  \date                  19/04/21
  Support:               email to umberto.molinatti@cern.ch
*/

#ifndef RD53RingOscillatorHistograms_H
#define RD53RingOscillatorHistograms_H

#include "../DQMHistogramBase.h"
#include "Utils/ContainerFactory.h"

#include "Utils/GenericDataArray.h"

#include "TFitResult.h"
#include "TGraph.h"
#include <TStyle.h>
#include <fstream>

#include "TGraph.h"
#include "TH1.h"
#include "TMultiGraph.h"

class RingOscillatorHistograms : public DQMHistogramBase
{
  public:
    void fillRO(double gloPulse[11], double oscCounts[8][11], double oscFrequency[8][11], double trimOscCounts[8][16], double trimOscFrequency[8][16], double trimVoltage[16]);

  private:
    DetectorDataContainer DetectorData;
    const char*           oscNames[8] = {"CKND0", "CKND4", "INV0", "INV4", "NAND0", "NAND4", "NOR0", "NOR4"};
};

#endif
