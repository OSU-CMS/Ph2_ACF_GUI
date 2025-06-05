/*!
  \file                  SystemController.h
  \brief                 Controller of the System, overall wrapper of the framework
  \author                Mauro DINARDO
  \version               2.0
  \date                  01/01/20
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef SYSTEMCONTROLLER_H
#define SYSTEMCONTROLLER_H

#include "HWDescription/Definition.h"
#include "HWDescription/OuterTrackerHybrid.h"
#include "HWDescription/RD53A.h"
#include "HWInterface/BeBoardFWInterface.h"
#include "HWInterface/BeBoardInterface.h"
#include "HWInterface/CbcInterface.h"
#include "HWInterface/ChipInterface.h"
#include "HWInterface/CicInterface.h"
#include "HWInterface/D19clpGBTInterface.h"
#include "HWInterface/MPA2Interface.h"
#include "HWInterface/PSInterface.h"
#include "HWInterface/RD53lpGBTInterface.h"
#include "HWInterface/ReadoutChipInterface.h"
#include "HWInterface/SSA2Interface.h"
#include "HWInterface/lpGBTInterface.h"
#include "NetworkUtils/TCPClient.h"
#include "NetworkUtils/TCPPublishServer.h"
#include "Parser/FileParser.h"
#include "Utils/ChannelGroupHandler.h"
#include "Utils/ConsoleColor.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/D19cCic2Event.h"
#include "Utils/D19cPSEventAS.h"
#include "Utils/Event.h"
#include "Utils/FileHandler.h"
#include "Utils/Utilities.h"
#include "Utils/easylogging++.h"

#include <boost/any.hpp>
#include <future>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <utility>
#include <vector>

// ########################################################
// # Librariries for communicating with Hybrid Test Cards #
// ########################################################
#ifdef __TCUSB__
#include "HWInterface/TCInterface.h"
#endif

class DetectorMonitor;
class ChannelGroupHandler;
class ConfigureInfo;
class StartInfo;
class CommunicationSettingConfig;

namespace Ph2_HwInterface
{
class VTRxInterface;
}

/*!
 * \namespace Ph2_System
 * \brief Namespace regrouping the framework wrapper
 */
namespace Ph2_System
{
class RegisterHelper;

using BeBoardFWMap = std::map<uint16_t, Ph2_HwInterface::BeBoardFWInterface*>; /*!< Map of Board connected */

/*!
 * \class SystemController
 * \brief Create, initialise, configure a predefined HW structure
 */
class SystemController
{
  public:
    Ph2_HwInterface::BeBoardInterface*     fBeBoardInterface;     //!< Interface to the BeBoard
    Ph2_HwInterface::ReadoutChipInterface* fReadoutChipInterface; //!< Interface to the readout chip
    Ph2_HwInterface::lpGBTInterface*       flpGBTInterface;       //!< Interface to the LpGBT
    Ph2_HwInterface::VTRxInterface*        fVTRxInterface;        //!< Interface to the VTRx
    Ph2_HwInterface::CicInterface*         fCicInterface;         //!< Interface to a CIC [only valid for OT]

    DetectorContainer*      fDetectorContainer;
    BeBoardFWMap            fBeBoardFWMap;
    Ph2_Parser::SettingsMap fSettingsMap;
    FileHandler*            fFileHandler;
    std::string             fRawFileName;
    std::stringstream       fParsedFile;
    bool                    fWriteHandlerEnabled;
    bool                    fDQMStreamerEnabled;
    bool                    fMonitorDQMStreamerEnabled;
    TCPPublishServer*       fDQMStreamer;
    TCPPublishServer*       fMonitorDQMStreamer;
    DetectorMonitor*        fDetectorMonitor;
    TCPClient*              fPowerSupplyClient{nullptr};
    RegisterHelper*         fRegisterHelper{nullptr};
    /*!
     * \brief Constructor of the SystemController class
     */
    SystemController();

    /*!
     * \brief Destructor of the SystemController class
     */
    virtual ~SystemController();

    /*!
     * \brief Method to construct a system controller object from another one while re-using the same members
     */
    void Inherit(const SystemController* pController);

    /*!
     * \brief Destroy the SystemController object: clear the HWDescription Objects, FWInterface etc.
     */
    void Destroy();

    /*!
     * \brief Allow tool to act on monitoring dqm
     */
    void        StopMonitoring();
    std::string GetMonitorFileName();

    /*!
     * \brief Create a FileHandler object with
     * \param pFilename : the filename of the binary file
     */
    void         addFileHandler(const std::string& pFilename, char pOption);
    void         closeFileHandler();
    FileHandler* getFileHandler() { return fFileHandler; }

    /*!
     * \brief Issues a FileHandler for writing files to every BeBoardFWInterface if addFileHandler was called
     */
    void     initializeWriteFileHandler();
    uint32_t computeEventSize32(const Ph2_HwDescription::BeBoard* pBoard);

    /*!
     * \brief Read file in the a FileHandler object
     * \param pVec : the data vector
     */
    void readFile(std::vector<uint32_t>& pVec, uint32_t pNWords32 = 0);

    /*!
     * \brief Acceptor method for HwDescriptionVisitor
     * \param pVisitor
     */
    void accept(HwDescriptionVisitor& pVisitor)
    {
        pVisitor.visitSystemController(*this);

        for(auto* cBoard: *fDetectorContainer) static_cast<Ph2_HwDescription::BeBoard*>(cBoard)->accept(pVisitor);
    }

    /*!
     * \brief Initialize the Hardware via a config file
     * \param pFilename : HW Description file
     * \param os        : ostream to dump output
     */
    void InitializeHw(const std::string& pFilename, std::ostream& os = std::cout);

    /*!
     * \brief Initialize the settings
     * \param pFilename : settings file
     *\param os         : ostream to dump output
     */
    void InitializeSettings(const std::string& pFilename, std::ostream& os = std::cout);

    /*!
     * \brief Configure the Hardware with XML file indicated values
     */
    void ConfigureHw(bool pReInitialize = true);

    // IT + OT specific configurations
    /*!
     * \brief Configure the Hardware with XML file indicated values
     */
    void ConfigureIT(Ph2_HwDescription::BeBoard* pBoard);
    void ConfigureFrontendIT(Ph2_HwDescription::BeBoard* pBoard);
    void InitializeOT(Ph2_HwDescription::BeBoard* pBoard);
    void ConfigureOT(Ph2_HwDescription::BeBoard* pBoard);

    // OT specific configurations for 2S + PS modules
    /*!
     * \brief Configure the Hardware with XML file indicated values
     */
    void ModuleStartUpPS(const Ph2_HwDescription::OpticalGroup* pOpticalGroup);
    void ModuleStartUp2S(const Ph2_HwDescription::OpticalGroup* pOpticalGroup);
    bool CicStartUp(const Ph2_HwDescription::OpticalGroup* pOpticalGroup, bool cStartUpSequence);

    void initializeExceptionHandler();

    /*!
     * \brief Read Monitor Data from pBoard
     * \param pBeBoard
     * \param args
     * \return: none
     */
    void ReadSystemMonitor(Ph2_HwDescription::BeBoard* pBoard, const std::vector<std::string>& args, bool silentRunning = false) const;

    /*!
     * \brief Read Data from pBoard
     * \param pBeBoard
     * \return: number of packets
     */
    uint32_t ReadData(Ph2_HwDescription::BeBoard* pBoard, bool pWait = true);

    /*!
     * \brief Read Data from pBoard for use with OTSDAQ
     * \param pBeBoard
     * \param pData: data vector reference
     * \param pWait: wait  until sufficient data is there, default true
     * \return: number of packets
     */
    uint32_t ReadData(Ph2_HwDescription::BeBoard* pBoard, std::vector<uint32_t>& pData, bool pWait = true);

    /*!
     * \brief Read Data from all boards
     */
    void ReadData(bool pWait = true);

    virtual void Start(const StartInfo& theStartInfo);
    virtual void Stop();
    virtual void Pause();
    virtual void Resume();
    virtual void Configure(const ConfigureInfo& theConfigureInfo, bool pReInitialize = true);

    void StartBoard(Ph2_HwDescription::BeBoard* pBoard);
    void StopBoard(Ph2_HwDescription::BeBoard* pBoard);
    void PauseBoard(Ph2_HwDescription::BeBoard* pBoard);
    void ResumeBoard(Ph2_HwDescription::BeBoard* pBoard);

    void Abort();

    /*!
     * \brief Read N Events from pBoard
     * \param pBeBoard
     * \param pNEvents
     */
    void ReadNEvents(Ph2_HwDescription::BeBoard* pBoard, uint32_t pNEvents);

    /*!
     * \brief Read N Events from pBoard
     * \param pBeBoard
     * \param pNEvents
     * \param pData: data vector
     * \param pWait: contunue polling until enough data is present
     */
    void ReadNEvents(Ph2_HwDescription::BeBoard* pBoard, uint32_t pNEvents, std::vector<uint32_t>& pData, bool pWait = true);

    /*!
     * \brief Read N Events from all boards
     * \param pNEvents
     */
    void ReadNEvents(uint32_t pNEvents);

    const Ph2_HwDescription::BeBoard* getBoard(int boardId) const { return fDetectorContainer->getObject(boardId); }

    const std::vector<Ph2_HwInterface::Event*>& GetEvents()
    {
        if(fFuture.valid() == true) fFuture.get();
        return fEventList;
    }

    void DecodeData(const Ph2_HwDescription::BeBoard* pBoard, const std::vector<uint32_t>& pData, uint32_t pNevents, BoardType pType);

    template <typename T>
    T findValueInSettings(const std::string name, T defaultValue = T()) const
    {
        auto setting = fSettingsMap.find(name);
        return (setting != std::end(fSettingsMap) ? boost::any_cast<T>(setting->second) : defaultValue);
    }

    template <typename T>
    bool setValueInSettings(const std::string name, T val)
    {
        auto setting = fSettingsMap.find(name);
        if(setting != std::end(fSettingsMap))
        {
            fSettingsMap[name] = val;
            return true;
        }
        return false;
    }

    void PrintRegCount()
    {
        // Print number of I2C transactions
        for(auto cBoard: *fDetectorContainer)
        {
            for(auto cOpticalGroup: *cBoard)
            {
                auto& clpGBT = cOpticalGroup->flpGBT;
                for(auto cHybrid: *cOpticalGroup)
                {
                    LOG(INFO) << BOLDBLUE << "Hybrid#" << +cHybrid->getId() << RESET;
                    uint8_t cMasterId = (cHybrid->getId() % 2 == 0) ? 2 : 0;
                    for(auto cChip: *cHybrid)
                    {
                        LOG(INFO) << BOLDMAGENTA << "\t\t..Chip#" << +cChip->getId() << " performed " << cChip->getWriteCount() << " CPB I2C writes and " << cChip->getReadCount() << " CPB I2C reads; "
                                  << " wrote " << cChip->getRegWriteCount() << " registers and read " << cChip->getRegReadCount() << " registers." << RESET;
                        if(clpGBT != nullptr)
                        {
                            clpGBT->updateWriteCount(cMasterId, cChip->getWriteCount());
                            clpGBT->updateReadCount(cMasterId, cChip->getReadCount());
                        }
                    }
                    if(clpGBT != nullptr)
                    {
                        LOG(INFO) << BOLDBLUE << "\tCPB I2C writes on Master" << +cMasterId << " : " << clpGBT->getWriteCount(cMasterId) << RESET;
                        LOG(INFO) << BOLDBLUE << "\tCPB I2C reads on Master" << +cMasterId << " : " << clpGBT->getReadCount(cMasterId) << RESET;
                    }
                }
            }
        }
    }

    void setChannelGroupHandler(ChannelGroupHandler& theChannelGroupHandler, std::function<bool(const ChipContainer*)> theQueryFunction = [](const ChipContainer*) { return true; });
    void
    setChannelGroupHandler(std::shared_ptr<ChannelGroupHandler> theChannelGroupHandlerPointer, std::function<bool(const ChipContainer*)> theQueryFunction = [](const ChipContainer*) { return true; });
    void setChannelGroupHandler(ChannelGroupHandler& theChannelGroupHandler, FrontEndType theFrontEndType);
    void setChannelGroupHandler(ChannelGroupHandler& theChannelGroupHandler, std::vector<FrontEndType> theFrontEndType);

    void setChannelGroupHandler(std::shared_ptr<ChannelGroupHandler> theChannelGroupHandlerPointer, uint16_t boardId, uint16_t opticalGroupId, uint16_t hybridId, uint16_t chipId);
    const DetectorDataContainer* getChannelGroupHandlerContainer() const { return fChannelGroupHandlerContainer; }

    inline const std::shared_ptr<ChannelGroupBase> getChannelGroup(int groupNumber, uint16_t boardId, uint16_t opticalGroupId, uint16_t hybridId, uint16_t chipId)
    {
        auto theChannelGroup = fChannelGroupHandlerContainer->getObject(boardId)->getObject(opticalGroupId)->getObject(hybridId)->getObject(chipId)->getSummary<std::shared_ptr<ChannelGroupHandler>>();
        if(groupNumber < theChannelGroup->getNumberOfGroups())
            return theChannelGroup->getTestGroup(groupNumber);
        else
            return std::shared_ptr<ChannelGroupBase>();
    }

    inline const std::shared_ptr<ChannelGroupBase> getChannelGroup(int groupNumber)
    {
        auto theChannelGroup = fChannelGroupHandlerContainer->getFirstObject()->getFirstObject()->getFirstObject()->getFirstObject()->getSummary<std::shared_ptr<ChannelGroupHandler>>();
        if(groupNumber < theChannelGroup->getNumberOfGroups())
            return theChannelGroup->getTestGroup(groupNumber);
        else
            return std::shared_ptr<ChannelGroupBase>();
    }

    void setInterfaceInitialization(uint8_t pCnfg) { fInitializeInterfaces = pCnfg; }
    void disableAllChannels();
    void DumpRegisters();

  private:
    void SetFuture(const Ph2_HwDescription::BeBoard* pBoard, const std::vector<uint32_t>& pData, uint32_t pNevents, BoardType pType);

    std::vector<Ph2_HwInterface::Event*> fEventList;
    std::future<void>                    fFuture;
    uint32_t                             fEventSize;
    uint32_t                             fNCbc;
    Ph2_Parser::FileParser               fParser;

    DetectorDataContainer* fChannelGroupHandlerContainer;

  protected:
    DetectorDataContainer*      fNameContainer;
    bool                        fSameChannelGroupForAllChannels{true};
    uint8_t                     fInitializeInterfaces{1};
    std::string                 fConfigurationFileName{""};
    std::string                 fSettingsFileName{""};
    std::string                 fCalibrationName{""};
    std::string                 fInitialConfigurationFileContent{""};
    BoardType                   fBoardType{BoardType::UNDEFINED};
    CommunicationSettingConfig* fCommunicationSettingConfig{nullptr};
    DetectorMonitorConfig*      fDetectorMonitorConfig{nullptr};
};

} // namespace Ph2_System

#endif
