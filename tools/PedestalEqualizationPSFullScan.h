/*!
 *
 * \file PedestalEqualizationPSFullScan.h
 * \brief PedestalEqualizationPSFullScan class
 * \author Fabio Ravera
 * \date 14/09/24
 *
 */

#ifndef PedestalEqualizationPSFullScan_h__
#define PedestalEqualizationPSFullScan_h__

#include "tools/PedestalEqualization.h"

class PedestalEqualizationPSFullScan : public PedestalEqualization
{
  public:
    void Initialise(bool pAllChan = false, bool pDisableStubLogic = true) override;
    void Reset() override;

    static std::string fCalibrationDescription;

  private:
    bool fOriginalIsFullScan;
};

#endif