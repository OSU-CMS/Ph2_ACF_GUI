#include <cstdlib>
#include <inttypes.h>
#include <string>
#include <vector>

#include "Utils/ConsoleColor.h"
#include "Utils/Utilities.h"
#include "Utils/argvparser.h"
#include "Utils/easylogging++.h"
#include "miniDAQ/MiddlewareStateMachine.h"

using namespace CommandLineProcessing;

INITIALIZE_EASYLOGGINGPP

int main(int argc, char* argv[])
{
    auto* baseDirChar_p = std::getenv("PH2ACF_BASE_DIR");
    if(baseDirChar_p == nullptr)
    {
        LOG(ERROR) << "Error, the environment variable PH2ACF_BASE_DIR is not initialized (hint: source setup.sh)";
        exit(EXIT_FAILURE);
    }

    el::Configurations conf(std::string(baseDirChar_p) + "/settings/logger.conf");
    el::Loggers::reconfigureAllLoggers(conf);

    ArgvParser cmd;

    cmd.setIntroductoryDescription("CMS Ph2_ACF  Data acquisition test and Data dump");

    cmd.addErrorCode(0, "Success");
    cmd.addErrorCode(1, "Error");

    cmd.setHelpOption("h", "help", "Print this help page");

    cmd.defineOption("list", "Print the list of available firmware images on SD card (works only with CTA boards)");
    cmd.defineOptionAlternative("list", "l");

    cmd.defineOption("delete", "Delete a firmware image on SD card (works only with CTA boards)", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("delete", "d");

    cmd.defineOption("file", "Local FPGA Bitstream file (*.mcs format for GLIB or *.bit/*.bin format for CTA boards)", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOptionAlternative("file", "f");

    cmd.defineOption("download", "Download an FPGA configuration from SD card to file (only for CTA boards)", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOptionAlternative("download", "o");

    cmd.defineOption("config", "Hw Description File . Default value: settings/HWDescription_2CBC.xml", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOptionAlternative("config", "c");

    cmd.defineOption("image", "Without -f: load image from SD card to FPGA\nWith    -f: name of image written to SD card\n-f specifies the source filename", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("image", "i");

    cmd.defineOption("board", "In case of multiple boards in the same file, specify board Id", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("board", "b");

    int result = cmd.parse(argc, argv);

    if(result != ArgvParser::NoParserError)
    {
        LOG(INFO) << cmd.parseErrorDescription(result);
        exit(EXIT_FAILURE);
    }

    std::string configurationFile = (cmd.foundOption("config")) ? cmd.optionValue("config") : "settings/HWDescription_2CBC.xml";
    uint16_t    boardId           = (cmd.foundOption("board")) ? convertAnyInt(cmd.optionValue("board").c_str()) : 0;

    MiddlewareStateMachine theMiddlewareStateMachine;

    std::string cFWFile;
    if(cmd.foundOption("file")) cFWFile = cmd.optionValue("file");
    // std::string              strImage("1");

    if(cmd.foundOption("list"))
    {
        std::vector<std::string> firmwareList = theMiddlewareStateMachine.getFirmwareList(configurationFile, boardId);
        LOG(INFO) << firmwareList.size() << " firmware images on SD card:";
        for(auto& name: firmwareList) LOG(INFO) << " - " << name;
        exit(EXIT_SUCCESS);
    }

    if(cmd.foundOption("delete"))
    {
        std::string firmwareName = cmd.optionValue("delete");
        LOG(INFO) << BOLDBLUE << "Deleting " << firmwareName << " from SD card..." << RESET;
        theMiddlewareStateMachine.deleteFirmwareFromSDcard(configurationFile, firmwareName, boardId);
        LOG(INFO) << BOLDBLUE << ">>> Done <<<" << RESET;
        exit(EXIT_SUCCESS);
    }

    if(cmd.foundOption("image"))
    {
        std::string firmwareName = cmd.optionValue("image");
        if(cmd.foundOption("file") && !cmd.foundOption("download"))
        {
            std::string inputFileName = cmd.optionValue("file");
            LOG(INFO) << BOLDBLUE << "Uploading " << inputFileName << " on SD card with name " << firmwareName << "..." << RESET;
            theMiddlewareStateMachine.uploadFirmwareOnSDcard(configurationFile, firmwareName, inputFileName, boardId);
            LOG(INFO) << BOLDBLUE << ">>> Done <<<" << RESET;
            exit(EXIT_SUCCESS);
        }
        else if(cmd.foundOption("download") && !cmd.foundOption("file"))
        {
            std::string outputFileName = cmd.optionValue("download");
            LOG(INFO) << BOLDBLUE << "Downloading " << firmwareName << " on SD card into " << outputFileName << "..." << RESET;
            theMiddlewareStateMachine.downloadFirmwareFromSDcard(configurationFile, firmwareName, outputFileName, boardId);
            LOG(INFO) << BOLDBLUE << ">>> Done <<<" << RESET;
            exit(EXIT_SUCCESS);
        }
        else if(!cmd.foundOption("file") && !cmd.foundOption("download"))
        {
            LOG(INFO) << BOLDBLUE << "Loading " << firmwareName << " into the FPGA..." << RESET;
            theMiddlewareStateMachine.loadFirmwareInFPGA(configurationFile, firmwareName, boardId);
            LOG(INFO) << BOLDBLUE << ">>> Done <<<" << RESET;
        }
        else
        {
            LOG(ERROR) << BOLDRED << "Error: -f and -o options cannot be specified at the same time" << RESET;
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        LOG(ERROR) << "Error, no FW image specified";
        exit(EXIT_FAILURE);
    }
}
