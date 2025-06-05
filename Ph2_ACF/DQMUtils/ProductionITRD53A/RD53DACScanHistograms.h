/*!
  \file                  RD53DACScanHistograms.h
  \brief                 Header file of DACScan histograms
  \author                Umberto MOLINATTI
  \version               1.0
  \date                  09/04/21
  Support:               email to umberto.molinatti@cern.ch
*/

#ifndef RD53DACScanHistograms_H
#define RD53DACScanHistograms_H

#include "../DQMHistogramBase.h"
#include "Utils/ContainerFactory.h"

#include "Utils/GenericDataArray.h"

#include "TApplication.h"
#include "TGraph.h"
#include <TStyle.h>

class DACScanHistograms : public DQMHistogramBase
{
  public:
    void fillDAC(DetectorContainer& DataContainer, double* fitStart, double* fitEnd, double** VMUXvolt, double** DACcode, double** DNLcode, double** INLcode, std::string* writeVar);

  private:
    DetectorDataContainer DetectorData;
};

#endif
