#ifndef __NTC_HANDLER__
#define __NTC_HANDLER__

#include <map>
#include <string>

// singleton

class NTChandler
{
    typedef std::map<float, float> NTCtable;

  public:
    static NTChandler& getInstance()
    {
        static NTChandler theNTChandler;
        return theNTChandler;
    }

    void addNTCtable(const std::string& theNTCtype, const std::string& theNTCtableFileName);

    float getTemperature(const std::string& theNTCtype, float resistance);

    std::string getNTCfile(const std::string& theNTCtype);

  private:
    NTChandler() {};
    NTChandler(const NTChandler&)            = delete;
    NTChandler& operator=(const NTChandler&) = delete;

    std::map<std::string, NTCtable>    fNTCtableMap;
    std::map<std::string, std::string> fNTCtypeAndFilename;
};

#endif