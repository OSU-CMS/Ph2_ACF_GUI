#include "Multimeter.h"

/*!
************************************************
* Multimeter constructor.
************************************************
*/
Multimeter::Multimeter(std::string model, const pugi::xml_node configuration) : fModel(model), fConfiguration(fConfigurationDoc.append_copy(configuration)) { ; }

/*!
************************************************
* Multimeter distructor.
************************************************
*/
Multimeter::~Multimeter(void) { ; }

/*!
************************************************
* \brief XML string conversion.
*  The xml string is converted to the string
*  needed.
* \param strXML String to be converted.
* \return String properly converted.
************************************************
*/

std::string Multimeter::convertToLFCR(std::string strXML)
{
    if(!strXML.compare("CR") || !strXML.compare("\\r"))
        return "\r";
    else if(!strXML.compare("LF") || !strXML.compare("\\n"))
        return "\n";
    else if(!strXML.compare("CRLF") || !strXML.compare("\\r\\n"))
        return "\r\n";
    else if(!strXML.compare("LFCR") || !strXML.compare("\\n\\r"))
        return "\n\r";
    else
    {
        std::stringstream error;
        error << "Multimeter configuration: string " << strXML
              << " is not understood, please check keeping in mind that the only available options are: \"\\n\" \"\\r\" \"\\n\\r\" \"\\r\\n\" \"LF\" \"CR\" \"CRLF\" \"LFCR\", aborting...";
        throw std::runtime_error(error.str());
    }
}

/*!
************************************************
************************************************
*/
std::string Multimeter::getModel() const { return fModel; }
