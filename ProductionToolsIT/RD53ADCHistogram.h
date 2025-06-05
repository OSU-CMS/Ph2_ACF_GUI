/*!
  \file                  RD53ADCHistogram.h
  \brief                 Implementaion of ADCHistogram
  \author                Umberto MOLINATTI
  \version               1.0
  \date                  16/02/21
  Support:               email to umberto.molinatti@cern.ch
*/

#ifndef RD53ADCHistogram_H
#define RD53ADCHistogram_H

#include "tools/Tool.h"

#ifdef __USE_ROOT__
#include "DQMUtils/ProductionITRD53A/RD53ADCScanHistograms.h"
#endif

// #########################
// # ADCHistogram test suite #
// #########################
class ADCHistogram : public Tool
{
  public:
    void run();
    void draw();

#ifdef __USE_ROOT__
    ADCScanHistograms* histos;
#endif

  private:
    std::vector<double> ADCcode;

    int max_counts = 10000000;
};

#endif
