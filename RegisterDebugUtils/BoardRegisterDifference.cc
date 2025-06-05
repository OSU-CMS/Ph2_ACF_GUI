#include "../Parser/ParserDefinitions.h"
#include "pugixml.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <string>

void parseRegister(pugi::xml_node pRegisterNode, std::string& pAttributeString, double& pValue, std::map<std::string, uint32_t>& theRegisterMap)
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
                parseRegister(cNode, cAttributeString, pValue, theRegisterMap);
            }
        }
        else
        {
            if(!pAttributeString.empty()) pAttributeString += ".";

            pAttributeString += pRegisterNode.attribute(COMMON_NAME_ATTRIBUTE_NAME).value();
            pValue                           = std::stol(pRegisterNode.first_child().value());
            theRegisterMap[pAttributeString] = pValue;
        }
    }
}

std::map<std::string, uint32_t> loadConfigFile(const std::string& filename)
{
    std::map<std::string, uint32_t> theRegisterMap;
    pugi::xml_document              registerPugiDocument;
    pugi::xml_parse_result          result = registerPugiDocument.load_file(filename.c_str());
    if(!result) // Try if it is not a file, but a string containing the full xml
        result = registerPugiDocument.load_string(filename.c_str());
    if(!result) { std::cerr << "Error: unable to open board file " << filename << std::endl; }

    pugi::xml_node cBeBoardConfigurationNode = registerPugiDocument.child("BeBoardRegister");

    for(pugi::xml_node cBeBoardRegNode = cBeBoardConfigurationNode.child(BEBOARD_REGISTER_NODE_NAME); cBeBoardRegNode; cBeBoardRegNode = cBeBoardRegNode.next_sibling())
    {
        if(std::string(cBeBoardRegNode.name()) == BEBOARD_REGISTER_NODE_NAME)
        {
            std::string cNameString;
            double      cValue;
            parseRegister(cBeBoardRegNode, cNameString, cValue, theRegisterMap);
        }
    }
    return theRegisterMap;
}

// Function to compare two maps and print differences
void compareAndPrintDifferences(const std::map<std::string, uint32_t>& originalRegisterMap, const std::map<std::string, uint32_t>& finalRegisterMap)
{
    for(auto originalRegister = originalRegisterMap.begin(); originalRegister != originalRegisterMap.end(); ++originalRegister)
    {
        auto finalRegister = finalRegisterMap.find(originalRegister->first);
        if(finalRegister == finalRegisterMap.end()) { std::cout << std::hex << "Register " << originalRegister->first << " not found in the final configuration" << std::dec << std::endl; }
        else if(originalRegister->second != finalRegister->second)
        {
            std::cout << std::hex << "Modified register: " << originalRegister->first << " 0x" << originalRegister->second << " -> 0x" << finalRegister->second << std::dec << std::endl;
        }
    }

    for(auto finalRegister = finalRegisterMap.begin(); finalRegister != finalRegisterMap.end(); ++finalRegister)
    {
        if(originalRegisterMap.find(finalRegister->first) == originalRegisterMap.end())
        {
            std::cout << std::hex << "Register " << finalRegister->first << " not found in the original configuration" << std::dec << std::endl;
        }
    }
}

int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <original_config_path> <final_config_path>" << std::endl;
        return 1;
    }

    std::cout << "Comparing file " << argv[1] << " with file " << argv[2] << std::endl;

    std::string file1(argv[1]);
    std::string file2(argv[2]);

    auto originalRegisterMap = loadConfigFile(argv[1]);
    auto finalRegisterMap    = loadConfigFile(argv[2]);

    compareAndPrintDifferences(originalRegisterMap, finalRegisterMap);

    return 0;
}
