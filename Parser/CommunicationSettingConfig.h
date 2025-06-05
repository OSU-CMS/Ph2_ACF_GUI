#ifndef __COMMUNICATION_SETTING_CONFIG__
#define __COMMUNICATION_SETTING_CONFIG__

#include <string>

class CommunicationSettingConfig
{
  public:
    struct CommunicationSetting
    {
        CommunicationSetting() {};
        CommunicationSetting(std::string theIP, uint16_t thePort, bool theEnable) : fIP(theIP), fPort(thePort), fEnable(theEnable) {}
        std::string fIP{""};
        uint16_t    fPort{0};
        bool        fEnable{false};
    };

    CommunicationSettingConfig() {};
    ~CommunicationSettingConfig() {};

    CommunicationSetting fControllerCommunication;
    CommunicationSetting fDQMCommunication;
    CommunicationSetting fMonitorDQMCommunication;
    CommunicationSetting fPowerSupplyDQMCommunication;
};

#endif