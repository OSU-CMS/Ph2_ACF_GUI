#ifndef __START_INFO__
#define __START_INFO__

#include <string>

class StartInfo
{
  public:
    StartInfo() {};
    ~StartInfo() {};

    void setRunNumber(int theRunNumber) { fRunNumber = theRunNumber; }
    int  getRunNumber() const { return fRunNumber; }

    void        setAppendInformation(std::string theAppendInformation) { fAppendInformation = theAppendInformation; }
    std::string getAppendInformation() const { return fAppendInformation; }

    void        parseProtobufMessage(const std::string& theConfigureInfoString);
    std::string createProtobufMessage() const;

  private:
    int         fRunNumber{-1};
    std::string fAppendInformation{""};
};

#endif