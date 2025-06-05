#ifndef SEH_MONITOR_H
#define SEH_MONITOR_H
#include "MonitorUtils/DetectorMonitor.h"
#include "NetworkUtils/TCPClient.h"
#ifdef __USE_ROOT__
#include "MonitorDQM/MonitorDQMPlot2S.h"
#include "MonitorDQM/MonitorDQMPlotSEH.h"
#endif
class SEHMonitor : public DetectorMonitor
{
  public:
    SEHMonitor(const Ph2_System::SystemController* theSystemController, const DetectorMonitorConfig& theDetectorMonitorConfig);
    virtual ~SEHMonitor();
    TCPClient* fPowerSupplyClient{nullptr};

  protected:
    void runMonitor() override;

  private:
    void runInputCurrentMonitor(const std::string& registerName);
    void runLpGBTRegisterMonitor(const std::string& registerName);
    void runPowerSupplyMonitor(const std::string& registerName);
    void runTestCardMonitor(const std::string& registerName);
// bool doMonitorInputCurrent{false};
#ifdef __USE_ROOT__
    MonitorDQMPlotSEH* fMonitorPlotDQMSEH;
    MonitorDQMPlot2S*  fMonitorDQMPlot2S;
#endif

    std::string getVariableValue(const std::string& variable, const std::string& buffer);
};

#endif
