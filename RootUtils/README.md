## Code description 
* `DQMEvent`: a reader/parser of the SLink64 Event format. The class has two contructors, as follows
  * DQMEvent(SLinkEvent* ptr)
  * DQMEvent(std::vector<uint64_t>& dVec): an SLinkEvent object is created from the data vector and the smart pointer stored.
     This constructor allows one to create a DQMEvent from any source (socket etc.).

  DQMEvent contains as many inner classes as there are sections in the .daq file
  * DAQHeader      (accessed as dqmEv->daqHeader())
  * TrackerHeader  (accessed as dqmEv->trkHeader())
  * TrackerPayload (accessed as dqmEv->trkPayload())
  * StubData       (accessed as dqmEv->stubData())
  * ConditionData  (accessed as dqmEv->condData())
  * DAQTrailer     (accessed as dqmEv->daqTrailer())

  DAQEvent.h contains two more basic stuctures
  * ReadoutStatus
  * StubInfo

  DQMEvent offers a print() method which prints an event in a human readable format

* `SLinkDQMHistogrammer`: All the histogramming is done here. Histograms are stored in a hierarchical structure
* `miniSLinkDQM.cc`: is the main application where a .daq binary file is read and converted into vector<DQMEvent*> followed by booking, filling, saving  and publishing histograms on the web

* Please refer to the presentation: https://indico.cern.ch/event/680223/contributions/2790717/attachments/1559529/2455326/DQMUpdate_16Nov2017.pdf
