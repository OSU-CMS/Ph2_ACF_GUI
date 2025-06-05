/*!
  \file                  RD53eudaqProducer.h
  \brief                 Header of EUDAQ producer
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53eudaqProducer_H
#define RD53eudaqProducer_H

#include "../user/CMSIT/module/include/CMSITEventData.hh"
#include "RD53Physics.h"

#include "eudaq/Producer.hh"
#include "eudaq/RawEvent.hh"

#include "boost/archive/binary_oarchive.hpp"
#include "boost/serialization/vector.hpp"

namespace EUDAQ
{
const std::string EVENT    = "CMSIT";
const int         WAIT     = 1000; // [ms]
const int         NBIT_TLU = 15;
const std::string FILERUNNUMBER("./RunNumber.txt");
constexpr char    EUDAQproducerNAME[] = "RD53eudaqProducer";
} // namespace EUDAQ

class RD53eudaqProducer : public eudaq::Producer
{
    class RD53eudaqEvtConverter
    {
      public:
        RD53eudaqEvtConverter(RD53eudaqProducer* eudaqProducer) : eudaqProducer(eudaqProducer) {}
        void operator()(const std::vector<Ph2_HwInterface::RD53Event>& RD53EvtList) const;

      private:
        RD53eudaqProducer* eudaqProducer;
    };

  public:
    RD53eudaqProducer(const std::string& appName, const std::string& address) : Producer(appName, address) {}

    void DoReset() override;
    void DoInitialise() override;
    void DoConfigure() override;
    void DoStartRun() override;
    void DoStopRun() override;
    void DoTerminate() override;
    void RunLoop() override;

    void     Creator(Ph2_System::SystemController& RD53SysCntr, const std::string& fileName);
    void     MainLoop() const;
    void     MySendEvent(eudaq::EventSP theEvent);
    void     AddBoreInfoToEvent(eudaq::Event& ev) const;
    uint32_t GetEventN() const { return m_evt_c; }

    uint32_t previousTLUTrigId;
    size_t   swTrigCnt;

    Physics RD53sysCntrPhys;

    static const uint32_t m_id_factory = eudaq::cstr2hash(EUDAQ::EUDAQproducerNAME);

  private:
    std::condition_variable_any wakeUp;
    std::recursive_mutex        theMtx;
    bool                        doExit;
    std::string                 configFile;
};

#endif
