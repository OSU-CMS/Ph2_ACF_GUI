/*!
  \file                DQMEvent.cc
  \brief               Reader for SLink Event
  \author              Suchandra Dutta and Subir Sarkar
  \version             0.8
  \date                05/11/17
  Support              mail to: Suchandra.Dutta@cern.ch, Subir.Sarkar@cern.ch
*/

#include "DQMEvent.h"

#include "Utils/SLinkEvent.h"
#include <boost/dynamic_bitset.hpp>

DQMEvent::DQMEvent(SLinkEvent* rptr)
{
    setEvent(rptr);
    parseEvent();
}

DQMEvent::DQMEvent(std::vector<uint64_t>& dVec)
{
    setEvent(dVec);
    parseEvent();
}
void DQMEvent::setEvent(SLinkEvent* rptr) { ptr_ = std::unique_ptr<SLinkEvent>(rptr); }
void DQMEvent::setEvent(std::vector<uint64_t>& dVec) { ptr_ = std::unique_ptr<SLinkEvent>(new SLinkEvent(dVec)); }
void DQMEvent::parseEvent(bool printData)
{
    std::vector<uint64_t> list = ptr_->getData<uint64_t>();
    // std::cout << "nwords: " << list.size() << std::endl;

    // DAQHeader
    daqHeader().set(list.front());

    // Tracker Header
    size_t nwords = trkHeader().set(list);
    // std::cout << "trkHeader nwords: " << nwords << std::endl;

    // Tracker payload
    nwords += trkPayload().set(list, 3 + nwords, trkHeader().FEStatus(), trkHeader().readoutStatusList());
    // std::cout << "+trkPayload nwords: " << nwords << std::endl;

    // Stub Data
    nwords += stubData().set(list, 3 + nwords, trkHeader().FEStatus());
    // std::cout << "+stubData nwords: " << nwords << std::endl;

    // Condition Data
    size_t iword     = 3 + nwords;
    size_t nCondData = list.at(iword & 0xFFFF);
    // std::cout << "nCondData: " << nCondData << std::endl;
    for(size_t i = 0; i < nCondData; ++i)
    {
        uint64_t w = list.at(iword + i + 1);
        condData().set(w);
    }
    // std::cout << "+conditionData nwords: " << nwords+nCondData+1 << std::endl;

    // DAQ trailer
    daqTrailer().set(list.back());

    if(printData) print();
}
void DQMEvent::printRaw(std::ostream& os) const { ptr_->print(); }
