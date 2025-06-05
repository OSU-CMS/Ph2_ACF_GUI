/*!
  \file                  RD53InjectionDelay.h
  \brief                 Header of Injection Delay scan
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53InjectionDelay_H
#define RD53InjectionDelay_H

#include "RD53Latency.h"

#ifdef __USE_ROOT__
#include "DQMUtils/RD53InjectionDelayHistograms.h"
#else
typedef bool InjectionDelayHistograms;
#endif

// ##############################
// # Injection delay test suite #
// ##############################
class InjectionDelay : public PixelAlive
{
  public:
    ~InjectionDelay()
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
        const uint16_t nIterations =
            (stopValue - startValue + 1 <= RD53Shared::setBits(RD53Shared::MAXBITCHIPREG) + 1 ? stopValue - startValue + 1 : RD53Shared::setBits(RD53Shared::MAXBITCHIPREG) + 1);
        return PixelAlive::getNumberIterations() * nIterations;
    }

    void analyze();

    InjectionDelayHistograms* histos;

  private:
    void fillHisto() override;

    void scanDac(const std::string& regName, const std::vector<uint16_t>& dacList, DetectorDataContainer* theContainer);

    Latency               la;
    std::vector<uint16_t> dacList;
    DetectorDataContainer theOccContainer;
    DetectorDataContainer theInjectionDelayContainer;

  protected:
    // ######################################
    // # Parameters from configuration file #
    // ######################################
    size_t startValue;
    size_t stopValue;
};

#endif
