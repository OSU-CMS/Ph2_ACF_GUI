/* Copyright 2014 Institut Pluridisciplinaire Hubert Curien
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   FileName :       FC7FpgaConfig.cc
   Content :        FPGA configuration
   Programmer :     Christian Bonnin
   Version :
   Date of creation : 2014-07-10
   Support :        mail to : christian.bonnin@iphc.cnrs.fr
*/

#include "HWInterface/FC7FpgaConfig.h"
#include "HWInterface/Firmware.h"
#include "HWInterface/MmcPipeInterface.h"
#include "HWInterface/RegManager.h"
#include "Utils/easylogging++.h"
#include <fstream>

using namespace std;
using namespace Ph2_HwInterface;

#define SECURE_MODE_PASSWORD "RuleBritannia"

namespace Ph2_HwInterface
{
FC7FpgaConfig::FC7FpgaConfig(RegManager&& pbbi) : FpgaConfig(std::move(pbbi)), lNode(nullptr)
{
    // Quick fix for IT - OT incompatibilities
    try
    {
        lNode = new fc7::MmcPipeInterface(dynamic_cast<const fc7::MmcPipeInterface&>(fwManager.getUhalNode("system.buf_cta")));
    }
    catch(const std::exception& lExc)
    {
        lNode = new fc7::MmcPipeInterface(dynamic_cast<const fc7::MmcPipeInterface&>(fwManager.getUhalNode("buf_cta")));
    }
}

FC7FpgaConfig::FC7FpgaConfig(FC7FpgaConfig&& theFC7FpgaConfig) : FpgaConfig(std::move(theFC7FpgaConfig))
{
    lNode                  = theFC7FpgaConfig.lNode;
    theFC7FpgaConfig.lNode = nullptr;
}

FC7FpgaConfig::~FC7FpgaConfig() { delete lNode; }

void FC7FpgaConfig::flashProm(const std::string& strImage, const std::string& szFile)
{
    if(szFile.compare(szFile.length() - 4, 4, ".bit") && szFile.compare(szFile.length() - 4, 4, ".bin"))
    {
        std::string errorMessage = "Error, the specified file is neither a .bit nor a .bin file";
        throw std::runtime_error(errorMessage);
    }
    checkIfUploading();
    numUploadingFpga = 1;
    progressValue    = 0;
    progressString   = "Starting upload";
    dumpFromFileIntoSD(strImage, szFile);
}

void FC7FpgaConfig::dumpFromFileIntoSD(const std::string& strImage, const std::string& pstrFile)
{
    if(string(pstrFile).compare(string(pstrFile).length() - 4, 4, ".bit") == 0)
    {
        fc7::XilinxBitFile bitFile(pstrFile);
        lNode->FileToSD(strImage, bitFile, &progressValue, &progressString);
    }
    else
    {
        fc7::XilinxBinFile binFile(pstrFile);
        lNode->FileToSD(strImage, binFile, &progressValue, &progressString);
    }

    progressValue = 100;
    lNode->RebootFPGA(strImage, SECURE_MODE_PASSWORD);
}

void FC7FpgaConfig::jumpToFpgaConfig(const std::string& strImage)
{
    checkIfUploading();
    verifyImageName(strImage);
    lNode->RebootFPGA(strImage, SECURE_MODE_PASSWORD);
}

void FC7FpgaConfig::downloadFpgaConfig(const std::string& strImage, const std::string& szFile)
{
    checkIfUploading();
    verifyImageName(strImage);
    vector<string> lstNames = lNode->ListFilesOnSD();

    for(size_t iName = 0; iName < lstNames.size(); iName++)
    {
        if(!strImage.compare(lstNames.at(iName))) numUploadingFpga = iName + 1;

        break;
    }

    progressValue  = 0;
    progressString = "Downloading configuration";
    downloadImage(strImage, szFile);
}

void FC7FpgaConfig::downloadImage(const std::string& strImage, const std::string& strDestFile)
{
    fc7::Firmware bitStream1 = lNode->FileFromSD(strImage, &progressValue, 0);
    progressString           = "Checking download";
    fc7::Firmware bitStream2 = lNode->FileFromSD(strImage, &progressValue, 33);
    fc7::Firmware bitStream3("empty");
    ofstream      oFile;
    oFile.open(strDestFile, ios::out | ios::binary);

    // for (const auto& uVal:bitStream.Bitstream())
    for(size_t idx = 0; idx < bitStream1.Bitstream().size(); idx++)
    {
        if(bitStream1.Bitstream().at(idx) != bitStream2.Bitstream().at(idx))
        {
            if(bitStream3.Bitstream().empty())
            {
                progressString = "Errors found, checking again";
                bitStream3     = lNode->FileFromSD(strImage, &progressValue, 66);
            }

            if(bitStream1.Bitstream().at(idx) == bitStream3.Bitstream().at(idx))
                oFile << (char)bitStream1.Bitstream().at(idx);
            else if(bitStream2.Bitstream().at(idx) == bitStream3.Bitstream().at(idx))
                oFile << (char)bitStream2.Bitstream().at(idx);
            else
                throw fc7::CorruptedFile();
        }
        else
            oFile << (char)bitStream1.Bitstream().at(idx);
    }

    oFile.close();
    progressValue = 100;
}

std::vector<std::string> FC7FpgaConfig::getFpgaConfigList()
{
    checkIfUploading();
    return lNode->ListFilesOnSD();
}

void FC7FpgaConfig::deleteFpgaConfig(const std::string& strId)
{
    checkIfUploading();
    verifyImageName(strId);
    lNode->DeleteFromSD(strId, SECURE_MODE_PASSWORD);
}

void FC7FpgaConfig::rebootBoard() { lNode->BoardHardReset(SECURE_MODE_PASSWORD); }

void FC7FpgaConfig::checkIfUploading()
{
    if(getUploadingFpga() > 0) throw std::runtime_error("This board is uploading an FPGA configuration");
}

void FC7FpgaConfig::verifyImageName(const std::string& firmwareName)
{
    std::vector<std::string> firmwareList = getFpgaConfigList();
    if(firmwareList.empty())
    {
        if(firmwareName.compare("1") != 0 && firmwareName.compare("2") != 0)
        {
            std::string errorMessage = "Error, invalid image name, should be 1 (golden) or 2 (user)";
            LOG(ERROR) << errorMessage;
            throw std::runtime_error(errorMessage);
        }
    }
    else
    {
        bool bFound = false;

        for(size_t iName = 0; iName < firmwareList.size(); iName++)
        {
            if(!firmwareName.compare(firmwareList.at(iName)))
            {
                bFound = true;
                break;
            }
        }

        if(!bFound)
        {
            std::string errorMessage = "Error, this image name: " + firmwareName + " is not available on SD card";
            LOG(ERROR) << errorMessage;
            throw std::runtime_error(errorMessage);
        }
    }
}

} // namespace Ph2_HwInterface
