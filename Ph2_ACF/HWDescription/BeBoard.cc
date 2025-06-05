/*!

        Filename :                              BeBoard.cc
        Content :                               BeBoard Description class, configs of the BeBoard
        Programmer :                    Lorenzo BIDEGAIN
        Version :               1.0
        Date of Creation :              14/07/14
        Support :                               mail to : lorenzo.bidegain@gmail.com

 */

#include "HWDescription/BeBoard.h"
#include "HWDescription/BeBoardRegItem.h"
#include "Parser/ParserDefinitions.h"
#include "Utils/Utilities.h"
#include "pugixml.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>

namespace Ph2_HwDescription
{
// Constructors

BeBoard::BeBoard() : BoardContainer(0), fEventType(EventType::VR), fCondDataSet(nullptr) {}

BeBoard::BeBoard(uint8_t pBeId) : BoardContainer(pBeId), fEventType(EventType::VR), fCondDataSet(nullptr) {}

BeBoard::BeBoard(uint8_t pBeId, const std::string& filename) : BoardContainer(pBeId), fEventType(EventType::VR), fCondDataSet(nullptr) { loadConfigFile(filename); }

// Public Members:

uint32_t BeBoard::getReg(const std::string& pReg) const
{
    BeBoardRegMap::const_iterator i = fRegMap.find(pReg);

    if(i == fRegMap.end())
    {
        LOG(INFO) << "The Board object: " << +getId() << " doesn't have " << pReg;
        return 0;
    }
    else
        return i->second.fValue;
}

void BeBoard::setReg(const std::string& pReg, uint32_t psetValue)
{
    auto oldRegister     = fRegMap[pReg].fValue;
    fRegMap[pReg].fValue = psetValue;
    if(fTrackModifiedRegistersEnabled)
    {
        if(fModifiedRegisters.find(pReg) == fModifiedRegisters.end()) // check if it already tracked
        {
            bool isFreeRegister = false;
            for(const auto& freeRegister: fListOfFreeRegisters)
            {
                isFreeRegister = std::regex_match(pReg, freeRegister.first);
                if(isFreeRegister) break;
            }
            if(!isFreeRegister && oldRegister != psetValue) { fModifiedRegisters[pReg] = oldRegister; }
        }
    }
}

void BeBoard::updateCondData(uint32_t& pTDCVal)
{
    if(fCondDataSet == nullptr)
        return;
    else if(fCondDataSet->fCondDataVector.size() == 0)
        return;
    else if(!fCondDataSet->testEffort())
        return;
    else
    {
        for(auto& cCondItem: this->fCondDataSet->fCondDataVector)
        {
            // if it is the TDC item, save it in fValue
            if(cCondItem.fUID == 3)
                cCondItem.fValue = pTDCVal;
            else if(cCondItem.fUID == 1)
            {
                for(auto cOpticalGroup: *this)
                {
                    for(auto cHybrid: *cOpticalGroup)
                    {
                        if(cCondItem.fHybridId != cHybrid->getId()) continue;

                        for(auto cCbc: *cHybrid)
                        {
                            if(cCondItem.fCbcId != cCbc->getId())
                                continue;
                            else if(cHybrid->getId() == cCondItem.fHybridId && cCbc->getId() == cCondItem.fCbcId)
                            {
                                ChipRegItem cRegItem = static_cast<ReadoutChip*>(cCbc)->getRegItem(cCondItem.fRegName);
                                cCondItem.fValue     = cRegItem.fValue;
                            }
                        }
                    }
                }
            }
        }
    }
}

void BeBoard::parseRegister(pugi::xml_node pRegisterNode, std::string& pAttributeString, double& pValue)
{
    if(std::string(pRegisterNode.name()) == BEBOARD_REGISTER_NODE_NAME)
    {
        if(std::string(pRegisterNode.first_child().value()).empty())
        {
            if(!pAttributeString.empty()) pAttributeString += ".";

            pAttributeString += pRegisterNode.attribute(COMMON_NAME_ATTRIBUTE_NAME).value();

            for(pugi::xml_node cNode = pRegisterNode.child(BEBOARD_REGISTER_NODE_NAME); cNode; cNode = cNode.next_sibling())
            {
                std::string cAttributeString = pAttributeString;
                parseRegister(cNode, cAttributeString, pValue);
            }
        }
        else
        {
            if(!pAttributeString.empty()) pAttributeString += ".";

            pAttributeString += pRegisterNode.attribute(COMMON_NAME_ATTRIBUTE_NAME).value();
            pValue = convertAnyDouble(pRegisterNode.first_child().value());
            BeBoardRegItem theRegister(pValue);
            theRegister.fPrmptCfg     = true;
            fRegMap[pAttributeString] = theRegister;
        }
    }
}

// Private Members:

void BeBoard::loadConfigFile(const std::string& filename)
{
    initializeFreeRegisters();
    pugi::xml_document     registerPugiDocument;
    pugi::xml_parse_result result = registerPugiDocument.load_file(filename.c_str());
    if(!result) // Try if it is not a file, but a string containing the full xml
        result = registerPugiDocument.load_string(filename.c_str());
    if(!result)
    {
        LOG(ERROR) << BOLDRED << "Error: Unable to open the file " << BOLDYELLOW << filename << RESET;
        LOG(ERROR) << BOLDRED << "Error description: " << BOLDYELLOW << result.description() << RESET;
        throw Exception("Unable to parse BeBoard XML source!");
    }

    pugi::xml_node cBeBoardConfigurationNode = registerPugiDocument.child("BeBoardRegister");

    for(pugi::xml_node cBeBoardRegNode = cBeBoardConfigurationNode.child(BEBOARD_REGISTER_NODE_NAME); cBeBoardRegNode; cBeBoardRegNode = cBeBoardRegNode.next_sibling())
    {
        if(std::string(cBeBoardRegNode.name()) == BEBOARD_REGISTER_NODE_NAME)
        {
            std::string cNameString;
            double      cValue;
            parseRegister(cBeBoardRegNode, cNameString, cValue);
        }
    }
}

std::vector<FrontEndType> BeBoard::connectedFrontEndTypes() const
{
    std::vector<FrontEndType> cFrontEndTypes;
    for(auto cOpticalGroup: *this)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            for(auto cChip: *cHybrid)
            {
                auto cIter = std::find(cFrontEndTypes.begin(), cFrontEndTypes.end(), cChip->getFrontEndType());
                if(cIter == cFrontEndTypes.end()) cFrontEndTypes.push_back(cChip->getFrontEndType());
            } // chips
        } // hybrids
    } // opticalGroup
    return cFrontEndTypes;
}

void BeBoard::takeSnapshot()
{
    clearSnapshot();
    fTrackModifiedRegistersEnabled = true;
}

void BeBoard::clearSnapshot()
{
    fTrackModifiedRegistersEnabled = false;
    fModifiedRegisters.clear();
}

std::vector<std::pair<std::string, uint32_t>> BeBoard::getSnapshot() const
{
    std::vector<std::pair<std::string, uint32_t>> theModifiedRegisterVector(fModifiedRegisters.begin(), fModifiedRegisters.end());
    theModifiedRegisterVector.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1}); // needed to make sure that it trigger config updates it it properly loaded
    return theModifiedRegisterVector;
}

void BeBoard::reinitializeFreeRegisters()
{
    std::remove_if(fListOfFreeRegisters.begin(), fListOfFreeRegisters.end(), [](std::pair<std::regex, RegisterType> theRegister) { return (theRegister.second == RegisterType::User); });
}

void BeBoard::initializeFreeRegisters()
{
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("sysreg\\.buf_test\\..*"), RegisterType::Utility));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("buf_cta\\..*"), RegisterType::Utility));
}

void BeBoard::addFreeRegister(const std::regex& theRegisterName) { fListOfFreeRegisters.push_back(std::make_pair(theRegisterName, RegisterType::User)); }

std::unique_ptr<pugi::xml_document> BeBoard::createRegisterPugiDocument() const
{
    std::function<void(std::string, std::vector<std::string>&)> splitRegister;
    splitRegister = [&splitRegister](std::string theFullRegister, std::vector<std::string>& theSplittedRegister)
    {
        auto nextSplitPoint = theFullRegister.find(".");
        if(nextSplitPoint == std::string::npos)
        {
            theSplittedRegister.push_back(theFullRegister);
            return;
        }
        else
        {
            theSplittedRegister.push_back(theFullRegister.substr(0, nextSplitPoint));
            splitRegister(theFullRegister.erase(0, nextSplitPoint + 1), theSplittedRegister);
        }
    };

    std::vector<std::pair<std::vector<std::string>, uint32_t>> theRegisterListSplitted;

    for(const auto& theRegisterNameAndValue: fRegMap)
    {
        std::vector<std::string> theSplittedRegisterName;
        splitRegister(theRegisterNameAndValue.first, theSplittedRegisterName);
        // for(const auto& reg : theSplittedRegisterName) std::cout<<reg<< " | ";
        // std::cout<<std::endl;
        theRegisterListSplitted.push_back(std::make_pair(theSplittedRegisterName, theRegisterNameAndValue.second.fValue));
    }

    std::function<void(pugi::xml_node&, const std::vector<std::pair<std::vector<std::string>, uint32_t>>&)> groupByRegisterAndDumpIntoFile;
    groupByRegisterAndDumpIntoFile = [&groupByRegisterAndDumpIntoFile](pugi::xml_node& theMotherNode, const std::vector<std::pair<std::vector<std::string>, uint32_t>>& theRegisterListSplitted)
    {
        std::map<std::string, std::vector<std::pair<std::vector<std::string>, uint32_t>>> theMapOfTheRegisterListSplitted;
        for(const auto& theRegisterSplittedAndValue: theRegisterListSplitted)
        {
            const std::vector<std::string>& theRegisterSplitted = theRegisterSplittedAndValue.first;
            const uint32_t&                 theRegisterValue    = theRegisterSplittedAndValue.second;
            if(theRegisterSplitted.size() == 1)
            {
                auto theRegisterNode                                         = theMotherNode.append_child(BEBOARD_REGISTER_NODE_NAME);
                theRegisterNode.append_attribute(COMMON_NAME_ATTRIBUTE_NAME) = theRegisterSplitted.at(0).c_str();
                theRegisterNode.append_child(pugi::node_pcdata).set_value(std::to_string(theRegisterValue).c_str());
            }
            else
            {
                std::vector<std::string> theSubVector(theRegisterSplitted.begin() + 1, theRegisterSplitted.end());
                theMapOfTheRegisterListSplitted[theRegisterSplitted.at(0)].push_back(std::make_pair(theSubVector, theRegisterValue));
            }
        }

        for(const auto& theMapElement: theMapOfTheRegisterListSplitted)
        {
            auto theRegisterNode                                         = theMotherNode.append_child(BEBOARD_REGISTER_NODE_NAME);
            theRegisterNode.append_attribute(COMMON_NAME_ATTRIBUTE_NAME) = theMapElement.first.c_str();
            groupByRegisterAndDumpIntoFile(theRegisterNode, theMapElement.second);
        }
    };

    std::unique_ptr<pugi::xml_document> registerPugiDocument = std::make_unique<pugi::xml_document>();

    // Add a declaration node
    pugi::xml_node declarationNode               = registerPugiDocument->prepend_child(pugi::node_declaration);
    declarationNode.append_attribute("version")  = "1.0";
    declarationNode.append_attribute("encoding") = "utf-8";

    pugi::xml_node boardRegisterNode = registerPugiDocument->append_child(BEBOARDREGISTER_NODE_NAME);

    groupByRegisterAndDumpIntoFile(boardRegisterNode, theRegisterListSplitted);

    return registerPugiDocument;
}

void BeBoard::saveRegMap(const std::string& fileName)
{
    auto registerPugiDocument = createRegisterPugiDocument();
    if(registerPugiDocument->save_file(fileName.c_str())) { LOG(INFO) << BOLDGREEN << "XML file " << fileName << " created successfully." << RESET; }
    else { LOG(ERROR) << BOLDRED << "Error opening file " << BOLDYELLOW << fileName << RESET; }
}

std::stringstream BeBoard::getRegMapStream() const
{
    std::stringstream theStream;
    auto              registerPugiDocument = createRegisterPugiDocument();
    registerPugiDocument->save(theStream);
    return theStream;
}

void BeBoard::dumpRegisters()
{
    for(const auto& reg: fRegMap) std::cout << reg.first << " " << reg.second.fValue << std::endl;
}

} // namespace Ph2_HwDescription
