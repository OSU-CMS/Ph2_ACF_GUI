#ifndef _DQMInterface_h_
#define _DQMInterface_h_

#include "Parser/FileParser.h"
#include "Utils/Container.h"
#include <future>
#include <vector>

class TCPSubscribeClient;
class DQMHistogramBase;
class TFile;
class ConfigureInfo;
class StartInfo;

class DQMInterface
{
  public:
    DQMInterface();
    ~DQMInterface(void);

    void configure(const ConfigureInfo& theConfigureInfo);
    void startProcessingData(const StartInfo& theStartInfo);
    void stopProcessingData(void);
    void pauseProcessingData(void) {}
    void resumeProcessingData(void) {}

    bool running(void);

  private:
    void                           destroy(void);
    void                           destroyHistogram(void);
    TCPSubscribeClient*            fListener;
    std::vector<DQMHistogramBase*> fDQMHistogrammerVector;
    std::vector<char>              fDataBuffer;
    bool                           fRunning;
    std::future<bool>              fRunningFuture;
    TFile*                         fOutputFile;
    DetectorContainer              fDetectorStructure;
    Ph2_Parser::SettingsMap        fSettingsMap;
};

#endif
