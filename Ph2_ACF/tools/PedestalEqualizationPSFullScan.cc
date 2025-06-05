#include "tools/PedestalEqualizationPSFullScan.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string PedestalEqualizationPSFullScan::fCalibrationDescription = "Equalize the pedestal/threshold for all channels with higher precision";

void PedestalEqualizationPSFullScan::Initialise(bool pAllChan, bool pDisableStubLogic)
{
    fOriginalIsFullScan = findValueInSettings<double>("FullScan", 0) > 0;
    setValueInSettings<double>("FullScan", 1);
    PedestalEqualization::Initialise(pAllChan, pDisableStubLogic);
}

void PedestalEqualizationPSFullScan::Reset()
{
    setValueInSettings<double>("FullScan", fOriginalIsFullScan ? 1 : 0); // restoring full scan original setting
    PedestalEqualization::Reset();
}
