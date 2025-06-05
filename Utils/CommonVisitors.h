#ifndef COMMONVISITORS_H__
#define COMMONVISITORS_H__

#include "HWDescription/Definition.h"
#include "HWInterface/BeBoardFWInterface.h"
#include "HWInterface/BeBoardInterface.h"
#include "HWInterface/ChipInterface.h"
#include "HWInterface/ReadoutChipInterface.h"
#include "Utils/ConsoleColor.h"
#include "Utils/Visitor.h"

#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

// wriite single reg

using namespace Ph2_HwInterface;
using namespace Ph2_HwDescription;

// INITIALIZE_EASYLOGGINGPP

struct CbcRegWriter : public HwDescriptionVisitor
{
    Ph2_HwInterface::ChipInterface* fInterface;
    std::string                     fRegName;
    uint16_t                        fRegValue;

    CbcRegWriter(Ph2_HwInterface::ChipInterface* pInterface, std::string pRegName, uint16_t pRegValue) : fInterface(pInterface), fRegName(pRegName), fRegValue(pRegValue) {}
    CbcRegWriter(const CbcRegWriter& writer) : fInterface(writer.fInterface), fRegName(writer.fRegName), fRegValue(writer.fRegValue) {}

    void setRegister(std::string pRegName, uint16_t pRegValue)
    {
        fRegName  = pRegName;
        fRegValue = pRegValue;
    }

    void visitChip(Ph2_HwDescription::Chip& pCbc) { fInterface->WriteChipReg(&pCbc, fRegName, fRegValue); }
};

struct BeBoardRegWriter : public HwDescriptionVisitor
{
    Ph2_HwInterface::BeBoardInterface* fInterface;
    std::string                        fRegName;
    uint32_t                           fRegValue;

    BeBoardRegWriter(Ph2_HwInterface::BeBoardInterface* pInterface, std::string pRegName, uint32_t pRegValue) : fInterface(pInterface), fRegName(pRegName), fRegValue(pRegValue) {}

    BeBoardRegWriter(const BeBoardRegWriter& writer) : fInterface(writer.fInterface), fRegName(writer.fRegName), fRegValue(writer.fRegValue) {}

    void setRegister(std::string pRegName, uint16_t pRegValue)
    {
        fRegName  = pRegName;
        fRegValue = pRegValue;
    }

    void visitBeBoard(Ph2_HwDescription::BeBoard& pBoard) { fInterface->WriteBoardReg(&pBoard, fRegName, fRegValue); }
};

struct ReadoutChipRegWriter : public HwDescriptionVisitor
{
    ReadoutChipInterface* fInterface;
    std::string           fRegisterName;
    uint16_t              fRegisterValue;
    bool                  fLocal;

    ReadoutChipRegWriter(ReadoutChipInterface* pInterface, std::string pRegisterName, uint16_t pRegisterValue, bool pLocal)
        : fInterface(pInterface), fRegisterName(pRegisterName), fRegisterValue(pRegisterValue), fLocal(pLocal)
    {
    }

    void visitReadoutChip(Ph2_HwDescription::ReadoutChip& pChip)
    {
        if(fLocal) {}
        else
            fInterface->WriteChipReg(&pChip, fRegisterName, fRegisterValue);
    }
};

struct ChipMultiRegWriter : public HwDescriptionVisitor
{
    ChipInterface*                                fInterface;
    std::vector<std::pair<std::string, uint16_t>> fRegVec;

    ChipMultiRegWriter(ChipInterface* pInterface, std::vector<std::pair<std::string, uint16_t>> pRegVec) : fInterface(pInterface), fRegVec(pRegVec) {}
    void visitChip(Ph2_HwDescription::Chip& pCbc) { fInterface->WriteChipMultReg(&pCbc, fRegVec); }
};

// write multi reg
struct CbcMultiRegWriter : public HwDescriptionVisitor
{
    Ph2_HwInterface::ChipInterface*               fInterface;
    std::vector<std::pair<std::string, uint16_t>> fRegVec;

    CbcMultiRegWriter(Ph2_HwInterface::ChipInterface* pInterface, std::vector<std::pair<std::string, uint16_t>> pRegVec) : fInterface(pInterface), fRegVec(pRegVec) {}

    void visitChip(Ph2_HwDescription::Chip& pCbc) { fInterface->WriteChipMultReg(&pCbc, fRegVec); }
};

// HwDescription Objects Counter
class Counter : public HwDescriptionVisitor
{
  private:
    uint32_t fNCbc;
    uint32_t fNHybrid;
    uint32_t fNBe;
    uint32_t fCbcMask;

  public:
    Counter() : fNCbc(0), fNHybrid(0), fNBe(0), fCbcMask(0) {}
    void visitChip(Ph2_HwDescription::Chip& pCbc)
    {
        fNCbc++;
        fCbcMask |= (1 << pCbc.getId());
    }
    void     visitHybrid(Ph2_HwDescription::Hybrid& pHybrid) { fNHybrid++; }
    void     visitBeboard(Ph2_HwDescription::BeBoard& pBoard) { fNBe++; }
    uint32_t getNChip() const { return fNCbc; }
    uint32_t getNHybrid() const { return fNHybrid; }
    uint32_t getNBe() const { return fNBe; }
    uint32_t getChipMask() const { return fCbcMask; }
};

// Configurator
class Configurator : public HwDescriptionVisitor
{
  private:
    Ph2_HwInterface::BeBoardInterface* fBeBoardInterface;
    Ph2_HwInterface::ChipInterface*    fCbcInterface;

  public:
    Configurator(Ph2_HwInterface::BeBoardInterface* pBeBoardInterface, Ph2_HwInterface::ChipInterface* pCbcInterface) : fBeBoardInterface(pBeBoardInterface), fCbcInterface(pCbcInterface) {}
    void visitBeBoard(Ph2_HwDescription::BeBoard& pBoard)
    {
        fBeBoardInterface->ConfigureBoard(&pBoard);
        LOG(INFO) << "Successfully configured Board " << +pBoard.getId();
    }
    void visitChip(Ph2_HwDescription::Chip& pCbc)
    {
        fCbcInterface->ConfigureChip(&pCbc);
        LOG(INFO) << "Successfully configured Chip " << +pCbc.getId();
    }
};

// read a single CBC register from fRegMap, from the physical CBC
struct CbcRegReader : public HwDescriptionVisitor
{
    std::string                     fRegName;
    uint16_t                        fRegValue;
    uint16_t                        fReadRegValue;
    Ph2_HwInterface::ChipInterface* fInterface;
    bool                            fOutput;

    CbcRegReader(Ph2_HwInterface::ChipInterface* pInterface, std::string pRegName) : fRegName(pRegName), fInterface(pInterface), fOutput(true) {}
    CbcRegReader(const CbcRegReader& reader) : fRegName(reader.fRegName), fInterface(reader.fInterface), fOutput(reader.fOutput) {}

    void setRegister(std::string pRegName) { fRegName = pRegName; }
    void visitChip(Ph2_HwDescription::Chip& pCbc)
    {
        fRegValue = pCbc.getReg(fRegName);
        fInterface->ReadChipReg(&pCbc, fRegName);
        fReadRegValue = pCbc.getReg(fRegName);

        if(fOutput)
            LOG(INFO) << "Reading Reg " << RED << fRegName << RESET << " on CBC " << +pCbc.getId() << " memory value: " << std::hex << +fRegValue << " read value: " << +fReadRegValue << std::dec;
    }
    uint16_t getMemoryValue() { return fRegValue; }
    uint16_t getHWValue() { return fReadRegValue; }
    void     setOutput(bool pOutput) { fOutput = pOutput; }
};

struct CbcRegIncrementer : public HwDescriptionVisitor
{
    Ph2_HwInterface::ChipInterface* fInterface;
    std::string                     fRegName;
    int                             fRegIncrement;

    CbcRegIncrementer(Ph2_HwInterface::ChipInterface* pInterface, std::string pRegName, int pRegIncrement) : fInterface(pInterface), fRegName(pRegName), fRegIncrement(pRegIncrement) {}
    CbcRegIncrementer(const CbcRegIncrementer& incrementer) : fInterface(incrementer.fInterface), fRegName(incrementer.fRegName), fRegIncrement(incrementer.fRegIncrement) {}

    void setRegister(std::string pRegName, int pRegIncrement)
    {
        fRegName      = pRegName;
        fRegIncrement = pRegIncrement;
    }

    void visitChip(Ph2_HwDescription::Chip& pCbc)
    {
        uint16_t currentValue = pCbc.getReg(fRegName);
        int      targetValue  = int(currentValue) + fRegIncrement;

        if(targetValue > 255)
            LOG(ERROR) << "Error: cannot increment register above 255" << std::endl, targetValue = 255;
        else if(targetValue < 0)
            LOG(ERROR) << "Error: cannot increment register below 0 " << std::endl, targetValue = 0;

        fInterface->WriteChipReg(&pCbc, fRegName, uint16_t(targetValue));
    }
};

struct ThresholdVisitor : public HwDescriptionVisitor
{
    uint16_t                        fThreshold;
    Ph2_HwInterface::ChipInterface* fInterface;
    char                            fOption;

    // Write constructor
    ThresholdVisitor(Ph2_HwInterface::ChipInterface* pInterface, uint16_t pThreshold) : fThreshold(pThreshold), fInterface(pInterface), fOption('w')
    {
        if(fThreshold > 1023)
        {
            LOG(ERROR) << "Error, Threshold value can be 10 bit max (1023)! - quitting";
            std::runtime_error(std::string("CommonVisitor::ThresholdVisitor : Error, Threshold value can be 10 bit max (1023)! - quitting"));
        }
    }
    // Read constructor
    ThresholdVisitor(Ph2_HwInterface::ChipInterface* pInterface) : fInterface(pInterface), fOption('r') {}
    // Copy constructor
    ThresholdVisitor(const ThresholdVisitor& pSetter) : fThreshold(pSetter.fThreshold), fInterface(pSetter.fInterface), fOption(pSetter.fOption) {}

    void setOption(char pOption)
    {
        if(pOption == 'w' || pOption == 'r')
            fOption = pOption;
        else
            LOG(ERROR) << "Error, not a valid option!";
    }
    uint16_t getThreshold() { return fThreshold; }
    void     setThreshold(uint16_t pThreshold) { fThreshold = pThreshold; }

    void visitChip(Ph2_HwDescription::Chip& pCbc)
    {
        if(pCbc.getFrontEndType() == FrontEndType::CBC3)
        {
            if(fOption == 'w')
            {
                if(fThreshold > 1023)
                    LOG(ERROR) << "Error, Threshold for CBC3 can only be 10 bit max (1023)!"; // h
                else
                {
                    std::vector<std::pair<std::string, uint16_t>> cRegVec;
                    // VCth1 holds bits 0-7 and VCth2 holds 8-9
                    uint16_t cVCth1 = fThreshold & 0x00FF;
                    uint16_t cVCth2 = (fThreshold & 0x0300) >> 8;
                    cRegVec.emplace_back("VCth1", cVCth1);
                    cRegVec.emplace_back("VCth2", cVCth2);
                    fInterface->WriteChipMultReg(&pCbc, cRegVec);
                }
            }
            else if(fOption == 'r')
            {
                fInterface->ReadChipReg(&pCbc, "VCth1");
                fInterface->ReadChipReg(&pCbc, "VCth2");
                uint16_t cVCth2 = pCbc.getReg("VCth2");
                uint16_t cVCth1 = pCbc.getReg("VCth1");
                fThreshold      = (((cVCth2 & 0x03) << 8) | (cVCth1 & 0xFF));
            }
            else
                LOG(ERROR) << "Unknown option " << fOption;
        }
        else if(pCbc.getFrontEndType() == FrontEndType::SSA2)
        {
            if(fOption == 'w')
            {
                if(fThreshold > 255)
                    LOG(ERROR) << "Error, Threshold for SSA can only be 10 bit max (255)!"; // h
                else
                {
                    std::vector<std::pair<std::string, uint16_t>> cRegVec;
                    uint16_t                                      cVCth1 = fThreshold & 0x00FF;
                    cRegVec.emplace_back("Bias_THDAC", cVCth1);
                    fInterface->WriteChipMultReg(&pCbc, cRegVec);
                }
            }
            else if(fOption == 'r')
            {
                fInterface->ReadChipReg(&pCbc, "Bias_THDAC");
                uint16_t cVCth1 = pCbc.getReg("Bias_THDAC");
                fThreshold      = cVCth1 & 0xFF;
            }
            else
                LOG(ERROR) << "Unknown option " << fOption;
        }
        else
            LOG(ERROR) << __PRETTY_FUNCTION__ << " Not a valid chip type!";
    }
};

struct LatencyVisitor : public HwDescriptionVisitor
{
    Ph2_HwInterface::ChipInterface* fInterface;
    uint16_t                        fLatency;
    char                            fOption;

    // write constructor
    LatencyVisitor(Ph2_HwInterface::ChipInterface* pInterface, uint16_t pLatency) : fInterface(pInterface), fLatency(pLatency), fOption('w') {}
    // read constructor
    LatencyVisitor(Ph2_HwInterface::ChipInterface* pInterface) : fInterface(pInterface), fOption('r') {}
    // copy constructor
    LatencyVisitor(const LatencyVisitor& pVisitor) : fInterface(pVisitor.fInterface), fLatency(pVisitor.fLatency), fOption(pVisitor.fOption) {}

    void setOption(char pOption)
    {
        if(pOption == 'w' || pOption == 'r')
            fOption = pOption;
        else
            LOG(ERROR) << "Error, not a valid option!";
    }
    uint16_t getLatency() { return fLatency; }
    void     setLatency(uint16_t pLatency) { fLatency = pLatency; }
    void     visitChip(Ph2_HwDescription::Chip& pCbc)
    {
        if(pCbc.getFrontEndType() == FrontEndType::CBC3)
        {
            if(fOption == 'w')
            {
                std::vector<std::pair<std::string, uint16_t>> cRegVec;
                // TriggerLatency1 holds bits 0-7 and FeCtrl&TrgLate2 holds 8
                uint16_t cLat1 = fLatency & 0x00FF;
                // in order to not mess with the other settings in FrontEndControl&TriggerLatency2, I have to read the
                // reg
                uint16_t presentValue = pCbc.getReg("FeCtrl&TrgLat2") & 0xFE;
                uint16_t cLat2        = presentValue | ((fLatency & 0x0100) >> 8);
                cRegVec.emplace_back("TriggerLatency1", cLat1);
                cRegVec.emplace_back("FeCtrl&TrgLat2", cLat2);
                fInterface->WriteChipMultReg(&pCbc, cRegVec);
            }
            else
            {
                fInterface->ReadChipReg(&pCbc, "TriggerLatency1");
                fInterface->ReadChipReg(&pCbc, "FeCtrl&TrgLat2");
                fLatency = ((pCbc.getReg("FeCtrl&TrgLat2") & 0x01) << 8) | (pCbc.getReg("TriggerLatency1") & 0xFF);
            }
        }

        else if(pCbc.getFrontEndType() == FrontEndType::SSA2)
        {
            if(fOption == 'w')
            {
                // std::cout << "MPAL" << std::endl;
                std::vector<std::pair<std::string, uint16_t>> cRegVec;
                cRegVec.emplace_back("L1-Latency_LSB", (0x00FF & fLatency) >> 0);
                cRegVec.emplace_back("L1-Latency_MSB", (0x0100 & fLatency) >> 8);
                fInterface->WriteChipMultReg(&pCbc, cRegVec);
            }
            else { fLatency = fInterface->ReadChipReg(&pCbc, "L1-Latency_LSB") | (fInterface->ReadChipReg(&pCbc, "L1-Latency_MSB") << 8); }
        }
        else
            LOG(ERROR) << __PRETTY_FUNCTION__ << " Not a valid chip type!";
    }
};

#endif
