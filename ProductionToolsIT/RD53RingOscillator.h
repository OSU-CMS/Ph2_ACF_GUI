/*!
  \file                  RD53RingOscillator.h
  \brief                 Implementaion of RingOscillator
  \author                Umberto MOLINATTI
  \version               1.0
  \date                  16/02/21
  Support:               email to umberto.molinatti@cern.ch
*/

#ifndef RD53RingOscillator_H
#define RD53RingOscillator_H

#include "tools/Tool.h"

#include <chrono>
#include <cmath>

#include "HWInterface/BeBoardFWInterface.h"
#include "HWInterface/RD53FWInterface.h"
#include "HWInterface/ReadoutChipInterface.h"

#ifdef __USE_ROOT__
#include "DQMUtils/ProductionITRD53A/RD53RingOscillatorHistograms.h"
#endif

// #########################
// # RingOscillator test suite #
// #########################
namespace Ph2_HwInterface
{
class RD53InterfaceRing : public ReadoutChipInterface
{
  public:
    template <typename T>
    void sendCommand(Ph2_HwDescription::ReadoutChip* pChip, const T& cmd)
    {
        static_cast<RD53FWInterface*>(fBoardFW)->WriteChipCommands(serialize(cmd), pChip->getHybridId());
    }
};
} // namespace Ph2_HwInterface
class RingOscillator : public Tool
{
  public:
    void run();
    void draw();

#ifdef __USE_ROOT__
    RingOscillatorHistograms* histos;
#endif

  private:
    double gloPulse[11];
    double oscCounts[8][11];
    double oscFrequency[8][11];
    double trimOscCounts[8][16];
    double trimOscFrequency[8][16];
    double trimVoltage[16];
};

#endif
