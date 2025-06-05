/*!
  \file                  RD53.cc
  \brief                 RD53 implementation class, config of the RD53
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinard@cern.ch
*/

#include "RD53.h"

namespace Ph2_HwDescription
{
LaneConfig::LaneConfig(bool                                   isPrimary,
                       uint8_t                                masterLane,
                       const std::array<uint8_t, NCHIPLANES>& outputLanes,
                       const std::array<bool, NCHIPLANES>&    signleChannelInputLanes,
                       const std::array<bool, NCHIPLANES>&    dualChannelInputLanes)
    : outputLaneMapping({0, 1, 2, 3})
    , inputLaneMapping({0, 1, 2, 3})
    , internalLanesEnabled({0, 0, 0, 0, 0})
    , nOutputLanes(std::count_if(outputLanes.begin(), outputLanes.end(), [](auto x) { return x > 0; }))
    , masterLane(masterLane)
    , isPrimary(isPrimary)
{
    // #####################
    // # outputLaneMapping #
    // #####################
    for(auto i = 0u; i < NCHIPLANES; i++)
    {
        if(outputLanes[NCHIPLANES - 1 - i] > 0)
            outputLaneMapping[i] = outputLanes[NCHIPLANES - 1 - i] - 1;
        else
            outputLaneMapping[i] = nOutputLanes;
    }

    // ########################
    // # internalLanesEnabled #
    // ########################
    size_t nBondedChannels = std::count(dualChannelInputLanes.begin(), dualChannelInputLanes.end(), true);
    if(nBondedChannels > 0)
    {
        internalLanesEnabled[0] = true;
        auto it                 = std::find(dualChannelInputLanes.rbegin(), dualChannelInputLanes.rend(), true);
        inputLaneMapping[0]     = it - dualChannelInputLanes.rbegin();
        it                      = std::find(it + 1, dualChannelInputLanes.rend(), true);
        inputLaneMapping[1]     = it - dualChannelInputLanes.rbegin();
    }

    // #############################################
    // # inputLaneMapping and internalLanesEnabled #
    // #############################################
    size_t nSingleChannels = std::count(signleChannelInputLanes.begin(), signleChannelInputLanes.end(), true);
    if(nSingleChannels > 0)
    {
        size_t j = nBondedChannels + 1;
        for(auto i = 0u; i < NCHIPLANES; i++)
            if(signleChannelInputLanes[NCHIPLANES - 1 - i])
            {
                inputLaneMapping[j - 1] = i;
                internalLanesEnabled[j] = true;
                j++;
            }
    }
}

RD53::RD53(uint8_t            pBeId,
           uint8_t            pFMCId,
           uint8_t            pOpticalGroupId,
           uint8_t            pHybridId,
           uint8_t            pRD53Id,
           uint8_t            pRD53Lane,
           int64_t            pRD53eFuseCode,
           const std::string& fileName,
           const std::string& cfgComment)
    : ReadoutChip(pBeId, pFMCId, pOpticalGroupId, pHybridId, pRD53Id)
{
    fMaxRegValue    = RD53Shared::setBits(RD53Constants::NBIT_MAXREG);
    fConfigFileName = fileName;
    myComment       = cfgComment;
    myChipLane      = pRD53Lane;
    myeFuseCode     = pRD53eFuseCode;
}

void RD53::loadfRegMap(const std::string& fileName)
{
    std::stringstream myString;
    std::ifstream     file(fileName.c_str(), std::ios::in);
    pixelMask         thePixMask(this->getNRows() * this->getNCols(), false, false, false, 0);

    if(file.good() == true)
    {
        std::string  line, fName, fAddress_str, fDefValue_str, fValue_str, fBitSize_str;
        bool         foundPixelConfig = false;
        int          cLineCounter     = 0;
        unsigned int col              = 0;
        ChipRegItem  fRegItem;

        initializeFreeRegisters();
        while(getline(file, line))
        {
            if(line.find_first_not_of(" \t") == std::string::npos || line.at(0) == '#' || line.at(0) == '*' || line.empty())
                fCommentMap[cLineCounter] = line;
            else if((line.find("PIXELCONFIGURATION") != std::string::npos) || (foundPixelConfig == true))
            {
                foundPixelConfig = true;

                if(line.find("ENABLE") != std::string::npos)
                {
                    line.erase(line.find("ENABLE"), 6);
                    myString.str("");
                    myString.clear();
                    myString << line;
                    unsigned int row = 0;
                    std::string  readWord;

                    while(getline(myString, readWord, ','))
                    {
                        readWord.erase(std::remove_if(readWord.begin(), readWord.end(), isspace), readWord.end());
                        if(std::all_of(readWord.begin(), readWord.end(), isdigit))
                        {
                            thePixMask.Enable.at(row + this->getNRows() * col) = std::atoi(readWord.c_str());
                            if(thePixMask.Enable[row + this->getNRows() * col] == false)
                                fChipOriginalMask->disableChannel(row, col);
                            else
                                fChipOriginalMask->enableChannel(row, col);
                            row++;
                        }
                    }

                    if(row < this->getNRows())
                    {
                        myString.str("");
                        myString.clear();
                        myString << "[RD53::loadfRegMap] Error, problem reading RD53 config file: too few rows (" << row << ") for column size " << thePixMask.Enable.size();
                        throw Exception(myString.str().c_str());
                    }
                }
                else if(line.find("HITBUS") != std::string::npos)
                {
                    line.erase(line.find("HITBUS"), 6);
                    myString.str("");
                    myString.clear();
                    myString << line;
                    unsigned int row = 0;
                    std::string  readWord;

                    while(getline(myString, readWord, ','))
                    {
                        readWord.erase(std::remove_if(readWord.begin(), readWord.end(), isspace), readWord.end());
                        if(std::all_of(readWord.begin(), readWord.end(), isdigit))
                        {
                            thePixMask.HitBus.at(row + this->getNRows() * col) = std::atoi(readWord.c_str());
                            row++;
                        }
                    }

                    if(row < this->getNRows())
                    {
                        myString.str("");
                        myString.clear();
                        myString << "[RD53::loadfRegMap] Error, problem reading RD53 config file: too few rows (" << row << ") for column size " << thePixMask.HitBus.size();
                        throw Exception(myString.str().c_str());
                    }
                }
                else if(line.find("INJEN") != std::string::npos)
                {
                    line.erase(line.find("INJEN"), 5);
                    myString.str("");
                    myString.clear();
                    myString << line;
                    unsigned int row = 0;
                    std::string  readWord;

                    while(getline(myString, readWord, ','))
                    {
                        readWord.erase(std::remove_if(readWord.begin(), readWord.end(), isspace), readWord.end());
                        if(std::all_of(readWord.begin(), readWord.end(), isdigit))
                        {
                            thePixMask.InjEn.at(row + this->getNRows() * col) = std::atoi(readWord.c_str());
                            row++;
                        }
                    }

                    if(row < this->getNRows())
                    {
                        myString.str("");
                        myString.clear();
                        myString << "[RD53::loadfRegMap] Error, problem reading RD53 config file: too few rows (" << row << ") for column size " << thePixMask.InjEn.size();
                        throw Exception(myString.str().c_str());
                    }
                }
                else if(line.find("TDAC") != std::string::npos)
                {
                    line.erase(line.find("TDAC"), 4);
                    myString.str("");
                    myString.clear();
                    myString << line;
                    unsigned int row = 0;
                    std::string  readWord;

                    while(getline(myString, readWord, ','))
                    {
                        readWord.erase(std::remove_if(readWord.begin(), readWord.end(), isspace), readWord.end());
                        if(std::all_of(readWord.begin(), readWord.end(), isdigit))
                        {
                            thePixMask.TDAC.at(row + this->getNRows() * col) = std::atoi(readWord.c_str());
                            row++;
                        }
                    }

                    if(row < this->getNRows())
                    {
                        myString.str("");
                        myString.clear();
                        myString << "[RD53::loadfRegMap] Error, problem reading RD53 config file: too few rows (" << row << ") for column size " << thePixMask.TDAC.size();
                        throw Exception(myString.str().c_str());
                    }
                    col++;
                }
            }
            else
            {
                myString.str("");
                myString.clear();
                myString << line;
                myString >> fName >> fAddress_str >> fDefValue_str >> fValue_str >> fBitSize_str;

                fRegItem.fAddress = strtoul(fAddress_str.c_str(), 0, 16);

                int baseType;
                if(fDefValue_str.compare(0, 2, "0x") == 0)
                    baseType = 16;
                else if(fDefValue_str.compare(0, 2, "0d") == 0)
                    baseType = 10;
                else if(fDefValue_str.compare(0, 2, "0b") == 0)
                    baseType = 2;
                else
                {
                    LOG(ERROR) << BOLDRED << "Unknown base " << BOLDYELLOW << fDefValue_str << RESET;
                    throw Exception("[RD53::loadfRegMap] Error, unknown base");
                }
                fDefValue_str.erase(0, 2);
                fRegItem.fDefValue = strtoul(fDefValue_str.c_str(), 0, baseType);

                if(fValue_str.compare(0, 2, "0x") == 0)
                    baseType = 16;
                else if(fValue_str.compare(0, 2, "0d") == 0)
                    baseType = 10;
                else if(fValue_str.compare(0, 2, "0b") == 0)
                    baseType = 2;
                else
                {
                    LOG(ERROR) << BOLDRED << "Unknown base " << BOLDYELLOW << fValue_str << RESET;
                    throw Exception("[RD53::loadfRegMap] Error, unknown base");
                }

                fValue_str.erase(0, 2);
                fRegItem.fValue = strtoul(fValue_str.c_str(), 0, baseType);

                fRegItem.fPage    = 0;
                fRegItem.fBitSize = strtoul(fBitSize_str.c_str(), 0, 10);
                fRegMap[fName]    = fRegItem;
            }

            cLineCounter++;
        }

        fPixelsMask        = thePixMask;
        fPixelsMaskDefault = thePixMask;

        file.close();
    }
    else
        throw Exception("[RD53::loadfRegMap] The RD53 file settings does not exist");
}

std::stringstream RD53::getRegMapStream()
{
    const unsigned int Nspaces = 26; // @CONST@
    std::stringstream  theStream;

    std::set<ChipRegPair, RegItemComparer> fSetRegItem;
    for(const auto& it: fRegMap) fSetRegItem.insert({it.first, it.second});

    int cLineCounter = 0;
    for(const auto& reg: fSetRegItem)
    {
        while(fCommentMap.find(cLineCounter) != std::end(fCommentMap))
        {
            auto cComment = fCommentMap.find(cLineCounter);

            theStream << cComment->second << std::endl;
            cLineCounter++;
        }

        theStream << reg.first;
        for(auto j = 0u; j < Nspaces; j++) theStream << " ";
        theStream.seekp(-(reg.first.size() < Nspaces ? reg.first.size() : Nspaces - 2), std::ios_base::cur);
        theStream << "0x" << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << int(reg.second.fAddress);
        for(auto j = 0u; j < 10 - (reg.first.size() < Nspaces ? 0 : reg.first.size() - Nspaces + 2); j++) theStream << " ";
        theStream << "0x" << std::setfill('0') << std::setw(4) << std::hex << std::uppercase << int(reg.second.fValue); // Copy fValue in fDefValue
        for(auto j = 0u; j < 18; j++) theStream << " ";
        theStream << "0x" << std::setfill('0') << std::setw(4) << std::hex << std::uppercase << int(reg.second.fValue);
        for(auto j = 0u; j < 29; j++) theStream << " ";
        theStream << std::setfill('0') << std::setw(2) << std::dec << std::uppercase << int(reg.second.fBitSize) << std::endl;

        cLineCounter++;
    }

    theStream << std::dec << std::endl;
    theStream << "*-----------------------------------------------------------------------------------------------------"
                 "--"
              << std::endl;
    theStream << "PIXELCONFIGURATION" << std::endl;
    theStream << "*-----------------------------------------------------------------------------------------------------"
                 "--"
              << std::endl;
    for(auto col = 0u; col < this->getNCols(); col++)
    {
        theStream << "COL                  " << std::setfill('0') << std::setw(3) << col << std::endl;

        theStream << "ENABLE " << +fPixelsMask.Enable[0 + this->getNRows() * col];
        for(auto row = 1u; row < this->getNRows(); row++) theStream << "," << +fPixelsMask.Enable[row + this->getNRows() * col];
        theStream << std::endl;

        theStream << "HITBUS " << +fPixelsMask.HitBus[0 + this->getNRows() * col];
        for(auto row = 1u; row < this->getNRows(); row++) theStream << "," << +fPixelsMask.HitBus[row + this->getNRows() * col];
        theStream << std::endl;

        theStream << "INJEN  " << +fPixelsMask.InjEn[0 + this->getNRows() * col];
        for(auto row = 1u; row < this->getNRows(); row++) theStream << "," << +fPixelsMask.InjEn[row + this->getNRows() * col];
        theStream << std::endl;

        theStream << "TDAC   " << +fPixelsMask.TDAC[0 + this->getNRows() * col];
        for(auto row = 1u; row < this->getNRows(); row++) theStream << "," << +fPixelsMask.TDAC[row + this->getNRows() * col];
        theStream << std::endl;

        theStream << std::endl;
    }

    return theStream;
}

void RD53::copyMaskFromDefault(const std::string& which)
// #######################
// # which = all         #
// # which = en : Enable #
// # which = hb : HitBus #
// # which = in : InjEn  #
// # which = td : TDAC   #
// #######################
{
    if(which.find("all") != std::string::npos)
        fPixelsMask = fPixelsMaskDefault;
    else
    {
        if(which.find("en") != std::string::npos) fPixelsMask.Enable = fPixelsMaskDefault.Enable;
        if(which.find("hb") != std::string::npos) fPixelsMask.HitBus = fPixelsMaskDefault.HitBus;
        if(which.find("in") != std::string::npos) fPixelsMask.InjEn = fPixelsMaskDefault.InjEn;
        if(which.find("td") != std::string::npos) fPixelsMask.TDAC = fPixelsMaskDefault.TDAC;
    }
}

void RD53::copyMaskToDefault(const std::string& which)
// #######################
// # which = all         #
// # which = en : Enable #
// # which = hb : HitBus #
// # which = in : InjEn  #
// # which = td : TDAC   #
// #######################
{
    if(which.find("all") != std::string::npos)
        fPixelsMaskDefault = fPixelsMask;
    else
    {
        if(which.find("en") != std::string::npos) fPixelsMaskDefault.Enable = fPixelsMask.Enable;
        if(which.find("hb") != std::string::npos) fPixelsMaskDefault.HitBus = fPixelsMask.HitBus;
        if(which.find("in") != std::string::npos) fPixelsMaskDefault.InjEn = fPixelsMask.InjEn;
        if(which.find("td") != std::string::npos) fPixelsMaskDefault.TDAC = fPixelsMask.TDAC;
    }

    if((which.find("all") != std::string::npos) || (which.find("en") != std::string::npos))
        for(auto col = 0u; col < this->getNCols(); col++)
            for(auto row = 0u; row < this->getNRows(); row++)
            {
                if(fPixelsMaskDefault.Enable[row + this->getNRows() * col] == true)
                    fChipOriginalMask->enableChannel(row, col);
                else
                    fChipOriginalMask->disableChannel(row, col);
            }
}

void RD53::resetMask()
{
    std::fill(fPixelsMask.Enable.begin(), fPixelsMask.Enable.end(), false);
    std::fill(fPixelsMask.HitBus.begin(), fPixelsMask.HitBus.end(), false);
    std::fill(fPixelsMask.InjEn.begin(), fPixelsMask.InjEn.end(), false);
    std::fill(fPixelsMask.TDAC.begin(), fPixelsMask.TDAC.end(), this->getFEtype(this->getNCols() / 2, this->getNCols() / 2)->nTDACvalues / 2);
}

void RD53::enableAllPixels()
{
    std::fill(fPixelsMask.Enable.begin(), fPixelsMask.Enable.end(), true);
    std::fill(fPixelsMask.HitBus.begin(), fPixelsMask.HitBus.end(), true);
}

void RD53::disableAllPixels()
{
    std::fill(fPixelsMask.Enable.begin(), fPixelsMask.Enable.end(), false);
    std::fill(fPixelsMask.HitBus.begin(), fPixelsMask.HitBus.end(), false);
}

size_t RD53::getNbMaskedPixels() { return std::count(fPixelsMask.Enable.begin(), fPixelsMask.Enable.end(), false); }

void RD53::enablePixel(unsigned int row, unsigned int col, bool enable)
{
    fPixelsMask.Enable[row + this->getNRows() * col] = enable;
    fPixelsMask.HitBus[row + this->getNRows() * col] = enable;
}

void RD53::enableDefaultPixel(unsigned int row, unsigned int col, bool enable)
{
    fPixelsMaskDefault.Enable[row + this->getNRows() * col] = enable;
    fPixelsMaskDefault.HitBus[row + this->getNRows() * col] = enable;

    if(enable == true)
        fChipOriginalMask->enableChannel(row, col);
    else
        fChipOriginalMask->disableChannel(row, col);
}

void     RD53::injectPixel(unsigned int row, unsigned int col, bool inject) { fPixelsMask.InjEn[row + this->getNRows() * col] = inject; }
void     RD53::setTDAC(unsigned int row, unsigned int col, uint8_t TDAC) { fPixelsMask.TDAC[row + this->getNRows() * col] = TDAC; }
void     RD53::resetTDAC(uint8_t TDAC) { std::fill(fPixelsMask.TDAC.begin(), fPixelsMask.TDAC.end(), TDAC); }
uint8_t  RD53::getTDAC(unsigned int row, unsigned int col) { return fPixelsMask.TDAC[row + this->getNRows() * col]; }
uint32_t RD53::getNumberOfChannels() const { return this->getNRows() * this->getNCols(); }

void RD53::maskCoreDefault(unsigned int row, unsigned int col)
{
    const unsigned int rowStart = RD53Constants::NROW_CORE * (row / RD53Constants::NROW_CORE);
    const unsigned int rowStop  = rowStart + RD53Constants::NROW_CORE;
    const unsigned int colStart = RD53Constants::NROW_CORE * (col / RD53Constants::NROW_CORE);
    const unsigned int colStop  = colStart + RD53Constants::NROW_CORE;

    for(auto r = rowStart; r < rowStop; r++)
        for(auto c = colStart; c < colStop; c++) RD53::enableDefaultPixel(r, c, false);
}

bool RD53::isDACLocal(const std::string& regName)
{
    if(regName == "PIX_PORTAL") return true;
    return false;
}

uint8_t RD53::getNumberOfBits(const std::string& regName)
{
    auto it = fRegMap.find(regName);
    if(it == fRegMap.end()) return 0;
    return it->second.fBitSize;
}

} // namespace Ph2_HwDescription
