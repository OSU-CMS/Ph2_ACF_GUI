#ifndef DETECTOR_MONITOR_CONFIG_H
#define DETECTOR_MONITOR_CONFIG_H

#include "Parser/ParserDefinitions.h"
#include <algorithm>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

struct DetectorMonitorConfig
{
    DetectorMonitorConfig()
    {
        fMonitorElementList["Board"]       = {};
        fMonitorElementList["LpGBT"]       = {};
        fMonitorElementList["CIC"]         = {};
        fMonitorElementList["MPA2"]        = {};
        fMonitorElementList["SSA2"]        = {};
        fMonitorElementList["CBC"]         = {};
        fMonitorElementList["RD53"]        = {};
        fMonitorElementList["PowerSupply"] = {};
        fMonitorElementList["TestCard"]    = {};
    }
    int         fSleepTimeMs{1000};
    std::string fMonitoringType{MONITORING_NODE_TYPE_ATTRIBUTE_NONE_VALUE};
    bool        fEnable{false};
    bool        fSilentRunning{false};

    void addElementToMonitor(const std::string& chipName, const std::string& registerName, const bool enable)
    {
        if(fMonitorElementList.find(chipName) == fMonitorElementList.end())
        {
            std::string exceptionMessage = "Error: cannot monitor chip type " + chipName + ". Chip type allowed: ";
            for(const auto& monitor: fMonitorElementList) exceptionMessage += (monitor.first + " ");
            exceptionMessage += "\n";
            throw std::runtime_error(exceptionMessage);
        }
        fMonitorElementList.at(chipName).emplace_back(std::make_pair(registerName, enable));
        ++fNumberOfMonitoredRegister;
    }

    uint16_t getNumberOfMonitoredRegisters() const { return fNumberOfMonitoredRegister; }

    std::map<std::string, std::vector<std::pair<std::string, bool>>> fMonitorElementList;
    uint16_t                                                         fNumberOfMonitoredRegister{0};
};

#endif
