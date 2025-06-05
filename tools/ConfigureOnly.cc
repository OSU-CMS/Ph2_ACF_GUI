#include "tools/ConfigureOnly.h"
#include "System/RegisterHelper.h"

std::string ConfigureOnly::fCalibrationDescription = "Run only configuration step";

ConfigureOnly::ConfigureOnly() : Tool() {}

ConfigureOnly::~ConfigureOnly() {}

void ConfigureOnly::Running()
{
    int secondToSleep = 0;
    for(int second = 0; second < secondToSleep; ++second)
    {
        std::cout << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] sleeping for other " << secondToSleep - second << " seconds" << std::endl;
        usleep(1000000);
    }
}

void ConfigureOnly::Stop() {}

void ConfigureOnly::Pause() {}

void ConfigureOnly::Resume() {}
