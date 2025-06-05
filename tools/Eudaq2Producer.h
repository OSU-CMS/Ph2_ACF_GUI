/*!
 *
 * \file Eudaq2Producer.h
 * \brief Testbeam Producer for EUDAQ2
 * \author Younes OTARID
 * \date 13 / 09 / 21
 *
 * \Support : younes.otarid@cern.ch
 *
 */
#ifndef Eudaq2Producer_h__
#define Eudaq2Producer_h__

#include "Utils/CommonVisitors.h"
#include "Utils/Visitor.h"
#include "tools/LinkAlignmentOT.h"
#include "tools/OTTool.h"

#include <cmath>
#include <map>
#include <memory>
#include <stdlib.h>

#ifdef __USE_ROOT__
#include "TCanvas.h"
#include "TGraphErrors.h"
#include "TH2.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include "TString.h"
#include "TText.h"
#endif

// eudaq stuff
#ifdef __EUDAQ__
#include "eudaq/Configuration.hh"
#include "eudaq/Event.hh"
#include "eudaq/Factory.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Producer.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/Time.hh"
#include "eudaq/Utils.hh"
#endif

#ifdef __EUDAQ__
class Eudaq2Producer
    : public OTTool
    , public eudaq::Producer
{
  public:
    Eudaq2Producer(const std::string& name, const std::string& runcontrol);
    ~Eudaq2Producer();

    // ph2 acf tool init
    void Initialise();
    void writeObjects();

    // to offload overriden methods a bit
    void ReadoutLoop();
    void ConvertToSubEvent(const Ph2_HwDescription::BeBoard*, const Ph2_HwInterface::Event*, eudaq::EventSP);
    bool EventsPending();
    void EnableDigitalInjection(uint8_t pPulseAmplitude, uint8_t pThresholdMPA, uint8_t pThresholdSSA);

    // override initialization from euDAQ
    void DoConfigure() override;
    void DoInitialise() override;
    void DoStartRun() override;
    void DoStopRun() override;
    void DoTerminate() override;
    void DoReset() override;

    // register producer in eudaq2
    static const uint32_t m_id_factory = eudaq::cstr2hash("CMSPhase2Producer");

  protected:
  private:
    // Some HW settings
    bool     fHandshakeEnabled;
    uint32_t fTriggerMultiplicity;
    uint32_t fHitsCounter;
    bool     fIsPS            = true;
    bool     fEnableInjection = false;
    // std::vector<int> fThresholdList;

    uint8_t               fThresholdMPA;
    uint8_t               fThresholdSSA;
    uint16_t              fThresholdCBC;
    int                   fRelativeThreshold;
    DetectorDataContainer fChipThreshContainer;

    // Run status variables
    bool        fExitRun, fConfigured, fInitialised;
    std::thread fThreadRun;
    bool        fSkipFirstEvent = true;

    // Handlers gor Ph2ACF Raw data and SLink data
    std::string  fPathToHWFile;
    std::string  fPathToRawPh2ACF;
    FileHandler* fPh2FileHandler;
    FileHandler* fSLinkFileHandler;

    // Temporary
    uint16_t fOriginalTriggerSrc;
    uint8_t  fOriginalTLUConfig;
};

// Register Producer in EUDAQ Factory
namespace
{
auto dummy0 = eudaq::Factory<eudaq::Producer>::Register<Eudaq2Producer, const std::string&, const std::string&>(Eudaq2Producer::m_id_factory);
}

#endif
#endif
