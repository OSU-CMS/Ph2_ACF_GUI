/*!

        \file                                            CICInterface.h
        \brief                                           User Interface to the Cics
        \version                                         1.0

 */

#ifdef __TCUSB__

#ifndef __TCINTERFACE_H__
#define __TCINTERFACE_H__

#pragma once
#include "USB_a.h"
#include "USB_libusb.h"
/*!
 * \namespace Ph2_HwInterface
 * \brief Namespace regrouping all the interfaces to the hardware
 */
namespace Ph2_HwInterface
{
template <class T>
class TCInterface
{
  public:
    TCInterface() { fPtr = new T(); }
    TCInterface(const std::string pName)
    {
        fPtr  = new T();
        fName = pName;
    }
    ~TCInterface()
    {
        if(fPtr != nullptr)
        {
            delete fPtr;
            fPtr = nullptr;
        }
    }
    // T* getInterface() const { return fPtr; }
    T getInterface() const { return *fPtr; }
    // user-defined copy assignment (copy-and-swap idiom)
    // T& operator=(const T original) { *fPtr = *original.fPtr; return *this; }
    // user defined assignment operator
    T& operator=(const T& rhs)
    {
        // shallow copy of ptr object
        fPtr  = rhs.fPtr;
        fName = rhs.fName;
        return *this;
    }
    std::string getName() { return fName; }
    void        setName(const std::string pName) { fName = pName; }

  private:
    T*          fPtr;
    std::string fName;
};

} // namespace Ph2_HwInterface
#endif // TCINTERFACE_H
#endif
