#include <vector>

#include "HWInterface/BeBoardInterface.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/ExceptionHandler.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/DataContainer.h"

using namespace Ph2_HwInterface;

// Initialize static member variable
ExceptionHandler* ExceptionHandler::fInstance = nullptr;

ExceptionHandler::~ExceptionHandler()
{
    delete fDetectorContainer;
    fDetectorContainer = nullptr;

    delete fQueryFunctionNames;
    fQueryFunctionNames = nullptr;
}

void ExceptionHandler::initializeQueryFunctionNameContainer()
{
    delete fQueryFunctionNames;
    fQueryFunctionNames = new DetectorDataContainer();
    ContainerFactory::copyAndInitStructure<EmptyContainer, EmptyContainer, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>>(
        *fDetectorContainer, *fQueryFunctionNames);
}

void ExceptionHandler::setDetectorContainer(DetectorContainer* theDetectorContainer)
{
    fDetectorContainer = theDetectorContainer;
    initializeQueryFunctionNameContainer();
}

void ExceptionHandler::updateFWInformation(uint16_t boardId)
{
    if(fDetectorContainer->getObject(boardId)->getBoardType() == BoardType::D19C)
    {
        static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(fDetectorContainer->getObject(boardId)))->EnableFrontEnds(fDetectorContainer->getObject(boardId));
    }
}

void ExceptionHandler::disableChip(uint16_t boardId, uint16_t opticalGroupId, uint16_t hybridId, uint16_t chipId)
{
    std::string functionName = "ExceptionHandler_disableChip_B" + std::to_string(boardId) + "_O" + std::to_string(opticalGroupId) + "_H" + std::to_string(hybridId) + "_C" + std::to_string(chipId);
    auto        disableChip  = [chipId](const ChipContainer* theContainer)
    {
        if(theContainer->getId() == chipId) return false;
        return true;
    };
    fDetectorContainer->getObject(boardId)->getObject(opticalGroupId)->getObject(hybridId)->addQueryFunction(disableChip, functionName);
    fQueryFunctionNames->getObject(boardId)->getObject(opticalGroupId)->getObject(hybridId)->getSummary<std::vector<std::string>>().push_back(functionName);
    updateFWInformation(boardId);
    if(fDetectorContainer->getObject(boardId)->getObject(opticalGroupId)->getObject(hybridId)->size() == 0)
    {
        LOG(INFO) << BOLDRED << "No chip enabled on Board id " << boardId << " OpticalGroup id " << opticalGroupId << " Hybrid id " << hybridId << " --- Full hybrid will be disabled" << RESET;
        disableHybrid(boardId, opticalGroupId, hybridId);
    }
}

void ExceptionHandler::disableHybrid(uint16_t boardId, uint16_t opticalGroupId, uint16_t hybridId)
{
    std::string functionName  = "ExceptionHandler_disableChip_B" + std::to_string(boardId) + "_O" + std::to_string(opticalGroupId) + "_H" + std::to_string(hybridId);
    auto        disableHybrid = [hybridId](const HybridContainer* theContainer)
    {
        if(theContainer->getId() == hybridId) return false;
        return true;
    };
    fDetectorContainer->getObject(boardId)->getObject(opticalGroupId)->addQueryFunction(disableHybrid, functionName);
    fQueryFunctionNames->getObject(boardId)->getObject(opticalGroupId)->getSummary<std::vector<std::string>, std::vector<std::string>>().push_back(functionName);
    updateFWInformation(boardId);
    if(fDetectorContainer->getObject(boardId)->getObject(opticalGroupId)->size() == 0)
    {
        LOG(INFO) << BOLDRED << "No hybrid enabled on Board id " << boardId << " OpticalGroup id " << opticalGroupId << " --- Full optical group will be disabled" << RESET;
        disableOpticalGroup(boardId, opticalGroupId);
    }
}

void ExceptionHandler::disableOpticalGroup(uint16_t boardId, uint16_t opticalGroupId)
{
    std::string functionName        = "ExceptionHandler_disableChip_B" + std::to_string(boardId) + "_O" + std::to_string(opticalGroupId);
    auto        disableOpticalGroup = [opticalGroupId](const OpticalGroupContainer* theContainer)
    {
        if(theContainer->getId() == opticalGroupId) return false;
        return true;
    };
    fDetectorContainer->getObject(boardId)->addQueryFunction(disableOpticalGroup, functionName);
    fQueryFunctionNames->getObject(boardId)->getSummary<std::vector<std::string>, std::vector<std::string>>().push_back(functionName);
    updateFWInformation(boardId);
    if(fDetectorContainer->getObject(boardId)->size() == 0)
    {
        LOG(WARNING) << BOLDRED << "No optical group enabled on Board ID " << BOLDYELLOW << boardId << RESET;
        LOG(WARNING) << BOLDBLUE << "\t--> Full board will be disabled" << RESET;
        disableBoard(boardId);
    }
}

void ExceptionHandler::disableBoard(uint16_t boardId)
{
    std::string functionName = "ExceptionHandler_disableChip_B" + std::to_string(boardId);
    auto        disableBoard = [boardId](const BoardContainer* theContainer)
    {
        if(theContainer->getId() == boardId) return false;
        return true;
    };
    fDetectorContainer->addQueryFunction(disableBoard, functionName);
    fQueryFunctionNames->getSummary<std::vector<std::string>, std::vector<std::string>>().push_back(functionName);
    if(fDetectorContainer->size() == 0)
    {
        LOG(ERROR) << BOLDRED << "ExceptionHandler Error: No object enabled in fDetectorContainer, throwing exception" << RESET;
        throw std::runtime_error("ExceptionHandler Error: No object enabled in fDetectorContainer");
    }
}

void ExceptionHandler::resetExceptionQueries()
{
    for(const auto& functionName: fQueryFunctionNames->getSummary<std::vector<std::string>, std::vector<std::string>>()) { fDetectorContainer->removeQueryFunction(functionName); }
    for(auto board: *fDetectorContainer)
    {
        auto boardQueryFunctionNames = fQueryFunctionNames->getObject(board->getId());
        for(const auto& functionName: boardQueryFunctionNames->getSummary<std::vector<std::string>, std::vector<std::string>>()) { board->removeQueryFunction(functionName); }

        for(auto opticalGroup: *board)
        {
            auto opticalGroupQueryFunctionNames = boardQueryFunctionNames->getObject(opticalGroup->getId());
            for(const auto& functionName: opticalGroupQueryFunctionNames->getSummary<std::vector<std::string>, std::vector<std::string>>()) { opticalGroup->removeQueryFunction(functionName); }

            for(auto hybrid: *opticalGroup)
            {
                auto hybridQueryFunctionNames = opticalGroupQueryFunctionNames->getObject(hybrid->getId());
                for(const auto& functionName: hybridQueryFunctionNames->getSummary<std::vector<std::string>>()) { hybrid->removeQueryFunction(functionName); }
            }
        }
        updateFWInformation(board->getId());
    }

    initializeQueryFunctionNameContainer();
}
