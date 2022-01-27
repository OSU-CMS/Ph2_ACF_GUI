#include "pugixml.hpp"
#include <cstdlib>
#include <iostream>

int mainReadXML(int argc, char* argv[])
{
    pugi::xml_document     doc;
    pugi::xml_parse_result result = doc.load_file("config.xml");
    std::cout << "Load result: " << result.description() << "\n";
    std::cout << doc.child("connection").attribute("serial").value() << "\n";
    return 0;
}
