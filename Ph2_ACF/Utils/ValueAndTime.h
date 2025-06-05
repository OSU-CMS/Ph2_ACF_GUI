#ifndef ValueAndTime_H
#define ValueAndTime_H

#include "Utils/ContainerSerialization.h"

template <typename T>
class ValueAndTime
{
  public:
    ValueAndTime(T theValue, std::string theTime) : fValue(theValue), fTime(theTime) {}
    ValueAndTime() = default;

    T           fValue;
    std::string fTime;

  private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
        theArchive & fValue;
        theArchive & fTime;
    }
};

#endif
