#ifndef __MIDDLEWARE_STATE_MACHINE__
#define __MIDDLEWARE_STATE_MACHINE__

#include "miniDAQ/CombinedCalibrationFactory.h"
#include <map>
#include <string>
#include <vector>

namespace Ph2_HwInterface
{
class FC7FpgaConfig;

}
class Tool;

class MiddlewareStateMachine
{
  public:
    MiddlewareStateMachine();
    ~MiddlewareStateMachine();

    enum Status
    {
        RUNNING,
        DONE
    };
    // State machine commands
    void   initialize();
    void   configure(const ConfigureInfo theConfigureInfo);
    void   start(const StartInfo theStartInfo);
    void   stop();
    void   halt();
    void   pause();
    void   resume();
    void   abort();
    Status status();

    // FPGAconfig commands
    std::vector<std::string> getFirmwareList(const std::string& configurationFile, uint16_t boardId = 0);
    void                     deleteFirmwareFromSDcard(const std::string& configurationFile, const std::string& firmwareName, uint16_t boardId = 0);
    void                     loadFirmwareInFPGA(const std::string& configurationFile, const std::string& firmwareName, uint16_t boardId = 0);
    void                     uploadFirmwareOnSDcard(const std::string& configurationFile, const std::string& firmwareName, const std::string& firmwareFile, uint16_t boardId = 0);
    void                     downloadFirmwareFromSDcard(const std::string& configurationFile, const std::string& firmwareName, const std::string& firmwareFile, uint16_t boardId = 0);

    Tool* fTheTool{nullptr};

    const CombinedCalibrationFactory& getCombinedCalibrationFactory() const { return fCombinedCalibrationFactory; }

  private:
    Ph2_HwInterface::FC7FpgaConfig        getFpgaConfig(const std::string& configurationFile, uint16_t boardId);
    int                                   currentRun_;
    std::map<std::string, std::type_info> fClassesInfo;
    CombinedCalibrationFactory            fCombinedCalibrationFactory;
};

#endif