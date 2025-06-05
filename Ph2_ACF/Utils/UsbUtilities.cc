/*

\file                          Utilities.h
\brief                         Some objects that might come in handy when using Ph2_USBInstDriver
\author                        Sarah Seif El Nasr-Storey
\version                       1.0
\date                          18/01/16
Support :                      mail to : Sarah.Storey@cern.ch

*/

#include "Utils/UsbUtilities.h"

bool InitializeMonitoring(std::string pHostname, std::string pInstrumentName, PortsInfo& pPortsInfo, int pMonitorInterval, std::string pLogFile)
{
    InstrPorts cHttpInstrPorts = {{"HMP4040", 8080}, {"Ke2110", 8082}};
    InstrPorts cZmqInstrPorts  = {{"HMP4040", 8081}, {"Ke2110", 8083}};

    auto cHttpSearch = cHttpInstrPorts.find(pInstrumentName);
    bool cStatus     = false;

    if(cHttpSearch != cHttpInstrPorts.end())
    {
        int  cHttpPortNumber = cHttpSearch->second;
        auto cZmqSearch      = cZmqInstrPorts.find(pInstrumentName);

        if(cZmqSearch != cZmqInstrPorts.end())
        {
            int cZmqPortNumber = cZmqSearch->second;
            pPortsInfo         = std::pair<int, int>(cZmqPortNumber, cHttpPortNumber);
            pid_t cPid; // the child process that the execution will soon run inside of.

            cPid = fork();

            if(cPid < 0) // fork failed
                exit(1);
            else if(cPid == 0) // fork succeeded
            {
                // launch server
                if(pInstrumentName.compare("HMP4040") == 0)
                {
                    LOG(INFO) << BOLDBLUE << "Trying to launch server to monitor currents on the HMP4040" << RESET;

                    if(launch_HMPServer("HMP4040.xml", pHostname, pPortsInfo, pMonitorInterval, pLogFile) == 1) exit(0);
                }
                else if(pInstrumentName.compare("Ke2110") == 0)
                {
                    LOG(INFO) << BOLDBLUE << "Trying to launch server to monitor Temperature with the Ke2110" << RESET;

                    launch_DMMServer(pHostname, pPortsInfo, pMonitorInterval, pLogFile);

                    exit(0);
                }
            }
            else // Main (parent) process after fork succeeds
            {
                int returnStatus = -1;
                waitpid(cPid, &returnStatus, 0);

                if(returnStatus == 0)
                    cStatus = true; // child process terminated without error
                else if(returnStatus == 1)
                {
                    if(pInstrumentName.compare("HMP4040") == 0)
                        LOG(INFO) << "The PS child process terminated with an error!.";
                    else if(pInstrumentName.compare("Ke2110") == 0)
                        LOG(INFO) << "The DMM child process terminated with an error!.";

                    exit(1);
                }
            }
        }
    }

    return cStatus;
}
void HMP4040server_tmuxSession(std::string pInitScript, std::string pConfigFile, std::string pHostname, PortsInfo pPortsInfo, int pMeasureInterval_s, std::string pLogFile)
{
    char        buffer[512];
    std::string currentDir    = getcwd(buffer, sizeof(buffer));
    std::string baseDirectory = return_InstDriverHomeDirectory() + "/Ph2_USBInstDriver";

    // // create bash script to launch HMP4040 sessions
    sprintf(buffer, "%s/%s", baseDirectory.c_str(), pInitScript.c_str());
    std::cout << BOLDBLUE << "Creating launch script : " << buffer << " ." << RESET << std::endl;
    std::ofstream starterScript(buffer);
    starterScript << "#!/bin/bash" << std::endl;

    starterScript << "SESSION_NAME=HMP4040_Server" << std::endl << std::endl;
    starterScript << "tmux list-session 2>&1 | grep -q \"^$SESSION_NAME\" || tmux new-session -s $SESSION_NAME -d" << std::endl;
    // send chdir via tmux session
    sprintf(buffer, "tmux send-keys -t $SESSION_NAME \"cd %s\" Enter", baseDirectory.c_str());
    starterScript << buffer << std::endl << std::endl;
    // set-up environment for Ph2_USB_InstDriver
    starterScript << "tmux send-keys -t $SESSION_NAME \". ./setup.sh\" Enter" << std::endl;

    // launch HMP4040 server
    if(pLogFile.empty())
        sprintf(buffer, "tmux send-keys -t $SESSION_NAME \"lvSupervisor -f %s -r %d -p %d -i %d -S \" Enter", pConfigFile.c_str(), pPortsInfo.first, pPortsInfo.second, pMeasureInterval_s);
    else
        sprintf(buffer,
                "tmux send-keys -t $SESSION_NAME \"lvSupervisor -f %s -r %d -p %d -i %d -S -o %s\" Enter",
                pConfigFile.c_str(),
                pPortsInfo.first,
                pPortsInfo.second,
                pMeasureInterval_s,
                pLogFile.c_str());

    starterScript << buffer << std::endl;
    starterScript.close();
}
void Ke2110server_tmuxSession(std::string pInitScript, std::string pConfigFile, std::string pHostname, PortsInfo pPortsInfo, int pMeasureInterval_s, std::string pLogFile)
{
    char        buffer[512];
    std::string currentDir    = getcwd(buffer, sizeof(buffer));
    std::string baseDirectory = return_InstDriverHomeDirectory() + "/Ph2_USBInstDriver";

    // // create bash script to launch Ke2110 sessions
    sprintf(buffer, "%s/%s", baseDirectory.c_str(), pInitScript.c_str());
    std::cout << BOLDBLUE << "Creating launch script : " << buffer << " ." << RESET << std::endl;
    std::ofstream starterScript(buffer);
    starterScript << "#!/bin/bash" << std::endl;

    starterScript << "SESSION_NAME=Ke2110_Server" << std::endl << std::endl;
    starterScript << "tmux list-session 2>&1 | grep -q \"^$SESSION_NAME\" || tmux new-session -s $SESSION_NAME -d" << std::endl;
    // send chdir via tmux session
    sprintf(buffer, "tmux send-keys -t $SESSION_NAME \"cd %s\" Enter", baseDirectory.c_str());
    starterScript << buffer << std::endl << std::endl;
    // set-up environment for Ph2_USB_InstDriver
    starterScript << "tmux send-keys -t $SESSION_NAME \". ./setup.sh\" Enter" << std::endl;

    // TODO - change to configure DMM supervisor
    if(pLogFile.empty())

        sprintf(buffer, "tmux send-keys -t $SESSION_NAME \"dmmSupervisor -r %d -p %d\" Enter", pPortsInfo.first, pPortsInfo.second);
    else
        sprintf(buffer, "tmux send-keys -t $SESSION_NAME \"dmmSupervisor -r %d -p %d -o %s\" Enter", pPortsInfo.first, pPortsInfo.second, pLogFile.c_str());

    starterScript << buffer << std::endl;
    starterScript.close();
}

std::string return_InstDriverHomeDirectory()
{
    char                     buffer[256];
    std::string              currentDir = getcwd(buffer, sizeof(buffer));
    std::vector<std::string> directories;
    tokenize(currentDir, directories, "/");

    std::string homeDir = "";

    for(unsigned int i = 0; i < directories.size() - 1; i++) homeDir += "/" + directories[i];

    return homeDir;
}

PortsInfo parse_ServerInfo(std::string pInfo)
{
    int cZmqPortNumber, cHttpPortNumber;

    // LOG (INFO) << "Retreived the following parameters from the info file: " << pInfo;
    // tokenize the cInfo string to recover the port numbers so the client can be smart enough to connect to the correct
    // port!
    std::vector<std::string> cTokens;
    cTokens.clear();
    tokenize(pInfo, cTokens, " ");
    std::vector<int> cPorts;
    cPorts.clear();

    for(auto token: cTokens)
    {
        // if ( std::stoi(token) )  cPorts.push_back ( boost::lexical_cast<int> (token) );
        // this instead of lexical_cast from boost
        try
        {
            cPorts.push_back(std::stoi(token));
        }
        catch(const std::invalid_argument& ia)
        {
        }
    }

    // THttp port comes first, then ZMQ
    cZmqPortNumber  = cPorts[0];
    cHttpPortNumber = cPorts[1];
    return std::pair<int, int>(cZmqPortNumber, cHttpPortNumber);
}

void query_Server(std::string pConfigFile, std::string pInstrumentName, std::string pHostname, PortsInfo& pPortsInfo, int pMeasureInterval_s, std::string pLogFile)
{
#if __ZMQ__
    std::string baseDirectory = return_InstDriverHomeDirectory() + "/Ph2_USBInstDriver";
    // LOG (DEBUG) << "Querying server - " << baseDirectory;
    InstrMaps cServerList        = {{"HMP4040", "lvSupervisor"}, {"Ke2110", "DMMSupervisor"}};
    InstrMaps cLaunchScriptsList = {{"HMP4040", "start_HMP4040.sh"}, {"Ke2110", "start_Ke2110.sh"}};

    int  cZmqPortNumber, cHttpPortNumber;
    auto search = cServerList.find(pInstrumentName);

    if(search != cServerList.end())
    {
        std::string cLockFileName = "/tmp/" + search->second + ".lock";
        AppLock*    cLock         = new AppLock(cLockFileName);

        // server already running - so lock file exists
        if(cLock->getLockDescriptor() < 0)
        {
            // retreive the info before quitting!
            std::string cInfo = cLock->get_info();
            std::cout << BOLDBLUE << "Port information from the " << cLockFileName << " lock file:\n\t" << cInfo << RESET << std::endl;

            pPortsInfo = parse_ServerInfo(cInfo);

            if(cLock) delete cLock;
        }
        else
        {
            auto cLaunchScript = cLaunchScriptsList.find(pInstrumentName);

            if(cLaunchScript != cLaunchScriptsList.end())
            {
                std::cout << BOLDBLUE << pInstrumentName << " server not running .... nothing to query." << RESET << std::endl;

                if(cLaunchScript->first == "HMP4040")
                    HMP4040server_tmuxSession(cLaunchScript->second, pConfigFile, pHostname, pPortsInfo, pMeasureInterval_s, pLogFile);
                else if(cLaunchScript->first == "Ke2110")
                    Ke2110server_tmuxSession(cLaunchScript->second, pConfigFile, pHostname, pPortsInfo, pMeasureInterval_s, pLogFile);

                char cmd[120];
                sprintf(cmd, ". %s/%s", baseDirectory.c_str(), (cLaunchScript->second).c_str());
                LOG(INFO) << BOLDBLUE << "Launching server using set-up script : " << cLaunchScript->second << " ." << RESET;

                if(cLock) delete cLock;

                system(cmd);
            }
        }
    }

#endif
}

int launch_HMPServer(std::string pConfigFile, std::string pHostname, PortsInfo& pPortsInfo, int pMeasureInterval_s, std::string pLogFile)
{
#if __ZMQ__
    char        buffer[256];
    std::string currentDir    = getcwd(buffer, sizeof(buffer));
    std::string baseDirectory = return_InstDriverHomeDirectory() + "/Ph2_USBInstDriver";

    std::string cHWfile = baseDirectory + "/settings/" + pConfigFile;

    // get device name from configuration file
    if(cHWfile.find(".xml") != std::string::npos)
    {
        // Get instrument name from XML file
        // std::cout << "Reading " << cHWfile << std::endl;
        pugi::xml_document     cDoc;
        pugi::xml_parse_result cResult     = cDoc.load_file(cHWfile.c_str());
        std::string            cInstrtName = std::string(cDoc.child("InstrumentDescription").first_child().name());

        // Now server stuff  - is it open? on which ports?
        query_Server(cHWfile, cInstrtName, "localhost", pPortsInfo, pMeasureInterval_s, pLogFile);
        return 1;
    }
    else
        std::cout << BOLDRED << "Configuration file not found." << RESET << std::endl;

#endif
    return 0;
}

int launch_DMMServer(std::string pHostname, PortsInfo& pPortsInfo, int pMeasureInterval_s, std::string pLogFile)
{
#if __ZMQ__
    char        buffer[256];
    std::string currentDir    = getcwd(buffer, sizeof(buffer));
    std::string baseDirectory = return_InstDriverHomeDirectory() + "/Ph2_USBInstDriver";

    std::string cHWfile     = "";
    std::string cInstrtName = "Ke2110";

    query_Server(cHWfile, cInstrtName, "localhost", pPortsInfo, pMeasureInterval_s, pLogFile);
#endif
    return 0;
}
