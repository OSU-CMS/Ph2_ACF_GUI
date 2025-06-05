#ifndef __REGISTER_HELPER_H__
#define __REGISTER_HELPER_H__

#include "HWDescription/FrontEndDescription.h"
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace Ph2_HwInterface
{
class ReadoutChipInterface;
class BeBoardInterface;
class VTRxInterface;
class lpGBTInterface;
class CicInterface;
class BeBoardFWInterface;
} // namespace Ph2_HwInterface
class DetectorContainer;
enum class FrontEndType;

namespace Ph2_System
{
class RegisterHelper
{
  public:
    RegisterHelper(DetectorContainer*                                        theDetectorContainer,
                   Ph2_HwInterface::BeBoardInterface*                        theBeBoardInterface,
                   Ph2_HwInterface::ReadoutChipInterface*                    theReadoutChipInterface,
                   Ph2_HwInterface::VTRxInterface*                           theVTRxInterface,
                   Ph2_HwInterface::lpGBTInterface*                          thelpGBTInterface,
                   Ph2_HwInterface::CicInterface*                            theCicInterface,
                   std::map<uint16_t, Ph2_HwInterface::BeBoardFWInterface*>* theBeBoardFWMap);

    RegisterHelper(const RegisterHelper&) = delete;

    ~RegisterHelper() {};

    void takeSnapshot();
    void restoreSnapshot();
    void freeFrontEndRegister(const FrontEndType theFrontEndType, std::string registerName);
    void freeBoardRegister(std::string registerName);

    void dumpBeBoardRegisterIntoXml(std::string outputFileName);

  private:
    void clearSnapshot();
    void resetFreeRegisters();
    template <typename C, typename T>
    void printModifiedRegisters(const C* theContainer, const std::vector<std::pair<std::string, T>> registerMap)
    {
        std::cout << Ph2_HwDescription::FrontEndDescription::getFrontEndName(theContainer->getFrontEndType()) << " id " << +theContainer->getId() << std::endl;
        for(const auto& registerNameAndValue: registerMap)
        {
            std::cout << "Setting back " << registerNameAndValue.first << " to 0x" << std::hex << +registerNameAndValue.second << std::dec << std::endl;
        }
    }

    DetectorContainer*                                        fDetectorContainer{nullptr};
    Ph2_HwInterface::BeBoardInterface*                        fBeBoardInterface{nullptr};
    Ph2_HwInterface::ReadoutChipInterface*                    fReadoutChipInterface{nullptr};
    Ph2_HwInterface::lpGBTInterface*                          flpGBTInterface{nullptr};
    Ph2_HwInterface::VTRxInterface*                           fVTRxInterface{nullptr};
    Ph2_HwInterface::CicInterface*                            fCicInterface{nullptr}; // Interface to a CIC [only valid for OT]
    std::map<uint16_t, Ph2_HwInterface::BeBoardFWInterface*>* fBeBoardFWMap{nullptr};
};
} // namespace Ph2_System

#endif