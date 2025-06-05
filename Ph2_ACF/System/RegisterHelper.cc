#include "System/RegisterHelper.h"
#include "HWDescription/Definition.h"
#include "HWDescription/OuterTrackerHybrid.h"
#include "HWDescription/VTRx.h"
#include "HWInterface/BeBoardFWInterface.h"
#include "HWInterface/BeBoardInterface.h"
#include "HWInterface/CicInterface.h"
#include "HWInterface/VTRxInterface.h"
#include "HWInterface/lpGBTInterface.h"
#include "Parser/ParserDefinitions.h"
#include "Utils/Container.h"
#include "pugixml.hpp"

#include "iostream"

using namespace Ph2_System;
using namespace Ph2_HwInterface;
using namespace Ph2_HwDescription;

RegisterHelper::RegisterHelper(DetectorContainer*                                        theDetectorContainer,
                               BeBoardInterface*                                         theBeBoardInterface,
                               ReadoutChipInterface*                                     theReadoutChipInterface,
                               VTRxInterface*                                            theVTRxInterface,
                               lpGBTInterface*                                           thelpGBTInterface,
                               CicInterface*                                             theCicInterface,
                               std::map<uint16_t, Ph2_HwInterface::BeBoardFWInterface*>* theBeBoardFWMap)
    : fDetectorContainer(theDetectorContainer)
    , fBeBoardInterface(theBeBoardInterface)
    , fReadoutChipInterface(theReadoutChipInterface)
    , flpGBTInterface(thelpGBTInterface)
    , fVTRxInterface(theVTRxInterface)
    , fCicInterface(theCicInterface)
    , fBeBoardFWMap(theBeBoardFWMap)
{
}

void RegisterHelper::takeSnapshot()
{
    LOG(INFO) << BOLDYELLOW << __PRETTY_FUNCTION__ << " taking snapshot of the current HW configuration" << RESET;
    for(auto theBoard: *fDetectorContainer)
    {
        theBoard->takeSnapshot();
        for(auto theOpticalGroup: *theBoard)
        {
            auto theLpGBT = theOpticalGroup->flpGBT;
            if(theLpGBT != nullptr) { theLpGBT->takeSnapshot(); }

            auto theVTRx = theOpticalGroup->fVTRx;
            if(theVTRx != nullptr) { theVTRx->takeSnapshot(); }

            for(auto theHybrid: *theOpticalGroup)
            {
                if(fCicInterface != nullptr) // easy check if it is IT or OT
                {
                    auto theCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                    if(theCic != nullptr) { theCic->takeSnapshot(); }
                }
                for(auto theChip: *theHybrid) { theChip->takeSnapshot(); }
            }
        }
    }
}

void RegisterHelper::clearSnapshot()
{
    for(auto theBoard: *fDetectorContainer)
    {
        theBoard->clearSnapshot();
        for(auto theOpticalGroup: *theBoard)
        {
            auto theLpGBT = theOpticalGroup->flpGBT;
            if(theLpGBT != nullptr) { theLpGBT->clearSnapshot(); }

            auto theVTRx = theOpticalGroup->fVTRx;
            if(theVTRx != nullptr) { theVTRx->clearSnapshot(); }

            for(auto theHybrid: *theOpticalGroup)
            {
                if(fCicInterface != nullptr) // easy check if it is IT or OT
                {
                    auto theCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                    if(theCic != nullptr) { theCic->clearSnapshot(); }
                }
                for(auto theChip: *theHybrid) { theChip->clearSnapshot(); }
            }
        }
    }
}

void RegisterHelper::restoreSnapshot()
{
    LOG(INFO) << BOLDYELLOW << __PRETTY_FUNCTION__ << " restoring snapshot of the HW configuration" << RESET;

    for(auto theBoard: *fDetectorContainer)
    {
        const auto modifiedBoardRegisters = theBoard->getSnapshot();
        fBeBoardInterface->WriteBoardMultReg(theBoard, modifiedBoardRegisters);
        for(auto theOpticalGroup: *theBoard)
        {
            auto theLpGBT = theOpticalGroup->flpGBT;
            if(theLpGBT != nullptr)
            {
                const auto modifiedLpGBTRegisters = theLpGBT->getSnapshot();
                flpGBTInterface->WriteChipMultReg(theLpGBT, modifiedLpGBTRegisters);
            }

            auto theVTRx = theOpticalGroup->fVTRx;
            if(theVTRx != nullptr)
            {
                const auto modifiedVTRxRegisters = theVTRx->getSnapshot();
                fVTRxInterface->WriteChipMultReg(theVTRx, modifiedVTRxRegisters);
            }

            for(auto theHybrid: *theOpticalGroup)
            {
                if(fCicInterface != nullptr) // easy check if it is IT or OT
                {
                    auto theCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                    if(theCic != nullptr)
                    {
                        const auto modifiedCicRegisters = theCic->getSnapshot();
                        fCicInterface->WriteChipMultReg(theCic, modifiedCicRegisters);
                    }
                }
                for(auto theChip: *theHybrid)
                {
                    const auto modifiedChipRegisters = theChip->getSnapshot();
                    fReadoutChipInterface->WriteChipMultReg(theChip, modifiedChipRegisters);
                }
            }
        }
    }

    clearSnapshot();
    resetFreeRegisters();
}

void RegisterHelper::freeFrontEndRegister(const FrontEndType theFrontEndType, std::string registerName)
{
    LOG(INFO) << BOLDYELLOW << __PRETTY_FUNCTION__ << " Freeing registers matching pattern " << registerName << " for frontend type " << FrontEndDescription::getFrontEndName(theFrontEndType) << RESET;

    std::regex registerPattern(registerName);
    for(auto theBoard: *fDetectorContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            if(theOpticalGroup->flpGBT != nullptr)
            {
                if(theOpticalGroup->flpGBT->getFrontEndType() == theFrontEndType) { theOpticalGroup->flpGBT->addFreeRegister(registerPattern); }
            }

            if(theOpticalGroup->fVTRx != nullptr)
            {
                if(theOpticalGroup->fVTRx->getFrontEndType() == theFrontEndType) { theOpticalGroup->fVTRx->addFreeRegister(registerPattern); }
            }

            for(auto theHybrid: *theOpticalGroup)
            {
                if(fCicInterface != nullptr) // easy check if it is IT or OT
                {
                    auto theCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                    if(theCic != nullptr)
                    {
                        if(theCic->getFrontEndType() == theFrontEndType) { theCic->addFreeRegister(registerPattern); }
                    }
                }
                for(auto theChip: *theHybrid)
                {
                    if(theChip->getFrontEndType() == theFrontEndType) { theChip->addFreeRegister(registerPattern); }
                }
            }
        }
    }
}

void RegisterHelper::freeBoardRegister(std::string registerName)
{
    std::regex registerPattern(registerName);
    for(auto theBoard: *fDetectorContainer) { theBoard->addFreeRegister(registerPattern); }
}

void RegisterHelper::resetFreeRegisters()
{
    for(auto theBoard: *fDetectorContainer)
    {
        theBoard->reinitializeFreeRegisters();
        for(auto theOpticalGroup: *theBoard)
        {
            auto theLpGBT = theOpticalGroup->flpGBT;
            if(theLpGBT != nullptr) { theLpGBT->reinitializeFreeRegisters(); }

            auto theVTRx = theOpticalGroup->fVTRx;
            if(theVTRx != nullptr) { theVTRx->reinitializeFreeRegisters(); }

            for(auto theHybrid: *theOpticalGroup)
            {
                if(fCicInterface != nullptr) // easy check if it is IT or OT
                {
                    auto theCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                    if(theCic != nullptr) { theCic->reinitializeFreeRegisters(); }
                }
                for(auto theChip: *theHybrid) { theChip->reinitializeFreeRegisters(); }
            }
        }
    }
}

void RegisterHelper::dumpBeBoardRegisterIntoXml(std::string outputFileName)
{
    std::function<void(pugi::xml_node&, BeBoard*, BeBoardInterface*, const std::string&, const uhal::HwInterface*, const std::string&)> recursiveListNodes;
    recursiveListNodes = [&recursiveListNodes](pugi::xml_node&          theMotherNode,
                                               BeBoard*                 cBoard,
                                               BeBoardInterface*        theBeBoardInterface,
                                               const std::string&       theNodeName,
                                               const uhal::HwInterface* hwInterface,
                                               const std::string&       motherNodeName)
    {
        std::string fullNodeName;
        if(motherNodeName != "")
            fullNodeName = motherNodeName + "." + theNodeName;
        else
            fullNodeName = theNodeName;
        const auto& node = hwInterface->getNode(fullNodeName);
        if(node.getPermission() != uhal::defs::READWRITE) return;
        // if(!((int)node.getPermission() & true)) return;

        pugi::xml_node theChildNode                               = theMotherNode.append_child(BEBOARD_REGISTER_NODE_NAME);
        theChildNode.append_attribute(COMMON_NAME_ATTRIBUTE_NAME) = theNodeName.c_str();

        if((++node.begin() == node.end()))
        {
            auto theRegisterValue = theBeBoardInterface->ReadBoardReg(cBoard, fullNodeName, false);
            theChildNode.append_child(pugi::node_pcdata).set_value(std::to_string(theRegisterValue).c_str());
        }
        const auto&                    theSubnodeList     = hwInterface->getNode(fullNodeName);
        const std::vector<std::string> theSubnodeNameList = theSubnodeList.getNodes();
        if(theSubnodeNameList.size() == 0) return;
        for(const auto& theSubNodeName: theSubnodeNameList)
        {
            if(theSubNodeName.find(".") != std::string::npos) continue;
            recursiveListNodes(theChildNode, cBoard, theBeBoardInterface, theSubNodeName, hwInterface, fullNodeName);
        }
    };

    // #################################################
    // # Dump firmware register content for all boards #
    // #################################################
    for(const auto cBoard: *fDetectorContainer)
    {
        pugi::xml_document doc;

        // Add a declaration node
        pugi::xml_node declarationNode               = doc.prepend_child(pugi::node_declaration);
        declarationNode.append_attribute("version")  = "1.0";
        declarationNode.append_attribute("encoding") = "utf-8";

        pugi::xml_node boardRegisterNode = doc.append_child(BEBOARDREGISTER_NODE_NAME);
        LOG(INFO) << GREEN << "Firmware register content for [board = " << BOLDYELLOW << cBoard->getId() << GREEN << "]" << RESET;

        const auto theBeBoardFW = this->fBeBoardFWMap->at(cBoard->getId());
        const auto hwInterface  = theBeBoardFW->getHardwareInterface();

        for(const auto& theNodeName: hwInterface->getNodes())
        {
            if(theNodeName.find(".") != std::string::npos) continue;
            recursiveListNodes(boardRegisterNode, cBoard, fBeBoardInterface, theNodeName, hwInterface, "");
        }

        std::string fileNameAppend = "_BeBoard_" + std::to_string(cBoard->getId()) + ".xml";
        std::size_t found          = outputFileName.rfind(".");
        if(found != std::string::npos) outputFileName.erase(outputFileName.begin() + found, outputFileName.end());
        outputFileName.append(fileNameAppend);

        if(doc.save_file(outputFileName.c_str())) { std::cout << "XML file " << outputFileName << " created successfully." << std::endl; }
        else { std::cerr << "Error saving XML file." << std::endl; }
    }
}
