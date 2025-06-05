#ifndef DETECTOR_MONITOR_H
#define DETECTOR_MONITOR_H

#include "Parser/DetectorMonitorConfig.h"
#include "System/SystemController.h"

#include "chrono"
#include "thread"

#ifdef __USE_ROOT__
class TFile;
#include "MonitorDQM/MonitorDQMPlotBase.h"
#endif

class DetectorMonitor
{
  public:
    DetectorMonitor(const Ph2_System::SystemController* theSystemController, const DetectorMonitorConfig& theDetectorMonitorConfig);
    virtual ~DetectorMonitor();
    void        forkMonitor();
    void        operator()();
    void        startMonitoring() { fEnableMonitor = true; }
    void        stopMonitoring() { fEnableMonitor = false; }
    void        resumeMonitoring() { startMonitoring(); }
    void        pauseMonitoring();
    void        stopRunning() { fKeepRunning = false; }
    void        waitForMonitorToStop();
    std::string getMonitorFileName();
#ifdef __USE_ROOT__
    TFile* getMonitorFile() { return fOutputFile; }
#endif
#if defined(__TCUSB__)
    void      setTestCardPointer(TC_2SSEH* cTC_2SSEH) { pTC_2SSEH = cTC_2SSEH; };
    TC_2SSEH* pTC_2SSEH{nullptr};
#endif

  protected:
    virtual void                        runMonitor() = 0;
    const Ph2_System::SystemController* fTheSystemController{nullptr};
    DetectorMonitorConfig               fDetectorMonitorConfig;
    std::string                         getMonitorName();
#ifdef __USE_ROOT__
    TFile*              fOutputFile{nullptr};
    MonitorDQMPlotBase* fMonitorPlotDQM{nullptr};
    std::string         fMonitorFileName = "";
#endif

  private:
    std::atomic<bool> fKeepRunning;
    std::atomic<bool> fEnableMonitor;
    std::atomic<bool> fIsMonitorRunning;
    std::future<void> fMonitorFuture;

    int fMaximumStopTentatives = 100;
};

#endif
