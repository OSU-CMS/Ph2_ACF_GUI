/*!
  \file                  RD53Latency.h
  \brief                 Header of Latency scan
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53Latency_H
#define RD53Latency_H

#include "RD53PixelAlive.h"

#ifdef __USE_ROOT__
#include "DQMUtils/RD53LatencyHistograms.h"
#else
typedef bool LatencyHistograms;
#endif

// ######################
// # Latency test suite #
// ######################
class Latency : public PixelAlive
{
  public:
    ~Latency()
    {
        this->WriteRootFile();
        delete histos;
    }

    void   Running() override;
    void   Stop() override;
    void   ConfigureCalibration() override;
    void   sendData() override;
    void   localConfigure(const std::string& histoFileName, int currentRun) override;
    void   run() override;
    void   draw(bool saveData = true) override;
    size_t getNumberIterations() override
    {
        const uint16_t nIterations = ((stopValue - startValue) / nTRIGxEvent + 1 <= RD53Shared::setBits(RD53Shared::MAXBITCHIPREG) + 1 ? (stopValue - startValue) / nTRIGxEvent + 1
                                                                                                                                       : RD53Shared::setBits(RD53Shared::MAXBITCHIPREG) + 1);
        return PixelAlive::getNumberIterations() * nIterations;
    }

    void analyze();

    LatencyHistograms* histos;

  private:
    void fillHisto() override;

    void scanDac(const std::string& regName, const std::vector<uint16_t>& dacList, DetectorDataContainer* theContainer);

    std::vector<uint16_t> dacList;
    DetectorDataContainer theOccContainer;
    DetectorDataContainer theLatencyContainer;

  protected:
    // ######################################
    // # Parameters from configuration file #
    // ######################################
    size_t startValue;
    size_t stopValue;
};

#endif
