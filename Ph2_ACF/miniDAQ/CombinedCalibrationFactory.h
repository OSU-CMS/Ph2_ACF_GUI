#ifndef __COMBINED_CALIBRATION_FACTORY__
#define __COMBINED_CALIBRATION_FACTORY__

#include "tools/CombinedCalibration.h"
#include "tools/RD53Physics.h"
#include "tools/Tool.h"
#include <iostream>
#include <map>
#include <string>

class BaseCreator
{
  public:
    BaseCreator() {}
    virtual ~BaseCreator() {}
    virtual Tool*                                    Create() const = 0;
    std::vector<std::pair<std::string, std::string>> fSubCalibrationAndDescriptionList{};
};

template <typename T>
std::string getClassName()
{
    int32_t     status;
    std::string className = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
    return className;
}

namespace CombinedCalibrationFactory_detail
{
template <typename>
struct sfinae_true : std::true_type
{
};

template <typename T>
static auto test_calibrationDescription(int) -> sfinae_true<decltype(T::fCalibrationDescription)>;
template <typename>
static auto test_calibrationDescription(long) -> std::false_type;
} // namespace CombinedCalibrationFactory_detail

template <typename T>
struct has_calibrationDescription : decltype(CombinedCalibrationFactory_detail::test_calibrationDescription<T>(0))
{
};

template <typename T, bool hasCalibrationDescription>
struct GetCalibrationDescription
{
    std::pair<std::string, std::string> operator()() { return std::make_pair(getClassName<T>(), ""); }
};

template <typename T>
struct GetCalibrationDescription<T, true>
{
    std::pair<std::string, std::string> operator()() { return std::make_pair(getClassName<T>(), T::fCalibrationDescription); }
};

template <typename T>
void getFullDescription(std::vector<std::pair<std::string, std::string>>& theFullCalibrationDescription)
{
    GetCalibrationDescription<T, has_calibrationDescription<T>::value> theGetCalibrationDescription;
    std::pair<std::string, std::string>                                theCalibrationDescription = theGetCalibrationDescription();
    theFullCalibrationDescription.push_back(theCalibrationDescription);
}

template <typename T, typename U, typename... Args>
void getFullDescription(std::vector<std::pair<std::string, std::string>>& theFullCalibrationDescription)
{
    GetCalibrationDescription<T, has_calibrationDescription<T>::value> theGetCalibrationDescription;
    std::pair<std::string, std::string>                                theCalibrationDescription = theGetCalibrationDescription();
    theFullCalibrationDescription.push_back(theCalibrationDescription);
    getFullDescription<U, Args...>(theFullCalibrationDescription);
}

template <typename... Args>
class Creator : public BaseCreator
{
  public:
    Creator() { getFullDescription<Args...>(fSubCalibrationAndDescriptionList); }
    virtual ~Creator() {}
    Tool* Create() const override { return new CombinedCalibration<Args...>(); };
    // ##################################################
    // # @TMP@ : when running IT-DAQ uncomment thisone  #
    // # for better compatibility with running sequence #
    // ##################################################
    /* Physics* Create() const override { return new Physics(); }; */
};

class CombinedCalibrationFactory
{
  public:
    CombinedCalibrationFactory();
    ~CombinedCalibrationFactory();

    template <typename... Args>
    void Register(const std::string& hardwareType, const std::string& calibrationTag)
    {
        if(fCalibrationMap.count(calibrationTag))
        {
            std::cerr << "calibrationTag " << calibrationTag << " already exists, aborting..." << std::endl;
            abort();
        }
        fCalibrationMap[calibrationTag] = std::make_pair(hardwareType, new Creator<Args...>);
    }

    Tool* createCombinedCalibration(const std::string& calibrationName) const;

    std::map<std::string, std::map<std::string, std::vector<std::pair<std::string, std::string>>>> getAvailableCalibrations() const;

  private:
    std::map<std::string, std::pair<std::string, BaseCreator*>> fCalibrationMap;
};

#endif
