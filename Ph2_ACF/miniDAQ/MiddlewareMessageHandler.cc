#include "miniDAQ/MiddlewareMessageHandler.h"
#include "Utils/ConfigureInfo.h"
#include "Utils/StartInfo.h"
#include "miniDAQ/CombinedCalibrationFactory.h"

MiddlewareMessageHandler::MiddlewareMessageHandler() {}

MiddlewareMessageHandler::~MiddlewareMessageHandler() {}

std::string MiddlewareMessageHandler::initialize(const std::string& message)
{
    MessageUtils::ReplyMessage theReplyMessage = tryCatchWrapper(__PRETTY_FUNCTION__, &MiddlewareStateMachine::initialize);
    return serializeMessage(theReplyMessage);
}

std::string MiddlewareMessageHandler::configure(const std::string& message)
{
    ConfigureInfo theConfigurationInfo;
    theConfigurationInfo.parseProtobufMessage(message);

    MessageUtils::ReplyMessage theReplyMessage = tryCatchWrapper(__PRETTY_FUNCTION__, &MiddlewareStateMachine::configure, theConfigurationInfo);
    return serializeMessage(theReplyMessage);
}

std::string MiddlewareMessageHandler::start(const std::string& message)
{
    StartInfo theStartInfo;
    theStartInfo.parseProtobufMessage(message);

    MessageUtils::ReplyMessage theReplyMessage = tryCatchWrapper(__PRETTY_FUNCTION__, &MiddlewareStateMachine::start, theStartInfo);
    return serializeMessage(theReplyMessage);
}

std::string MiddlewareMessageHandler::stop(const std::string& message)
{
    MessageUtils::ReplyMessage theReplyMessage = tryCatchWrapper(__PRETTY_FUNCTION__, &MiddlewareStateMachine::stop);
    return serializeMessage(theReplyMessage);
}

std::string MiddlewareMessageHandler::halt(const std::string& message)
{
    MessageUtils::ReplyMessage theReplyMessage = tryCatchWrapper(__PRETTY_FUNCTION__, &MiddlewareStateMachine::halt);
    return serializeMessage(theReplyMessage);
}

std::string MiddlewareMessageHandler::pause(const std::string& message)
{
    MessageUtils::ReplyMessage theReplyMessage = tryCatchWrapper(__PRETTY_FUNCTION__, &MiddlewareStateMachine::pause);
    return serializeMessage(theReplyMessage);
}

std::string MiddlewareMessageHandler::resume(const std::string& message)
{
    MessageUtils::ReplyMessage theReplyMessage = tryCatchWrapper(__PRETTY_FUNCTION__, &MiddlewareStateMachine::resume);
    return serializeMessage(theReplyMessage);
}

std::string MiddlewareMessageHandler::abort(const std::string& message)
{
    MessageUtils::ReplyMessage theReplyMessage = tryCatchWrapper(__PRETTY_FUNCTION__, &MiddlewareStateMachine::abort);
    return serializeMessage(theReplyMessage);
}

std::string MiddlewareMessageHandler::status(const std::string& message)
{
    MessageUtils::ReplyMessage theReplyMessage;

    try
    {
        MiddlewareStateMachine::Status theStatus = fMiddlewareStateMachine.status();
        if(theStatus == MiddlewareStateMachine::Status::DONE)
            theReplyMessage.mutable_reply_type()->set_type(MessageUtils::ReplyType::SUCCESS);
        else if(theStatus == MiddlewareStateMachine::Status::RUNNING)
            theReplyMessage.mutable_reply_type()->set_type(MessageUtils::ReplyType::RUNNING);
    }
    catch(const std::exception& theException)
    {
        catchFunction(theReplyMessage, theException, __PRETTY_FUNCTION__);
    }
    return serializeMessage(theReplyMessage);
}

void MiddlewareMessageHandler::catchFunction(MessageUtils::ReplyMessage& inputReplayMessage, const std::exception& theException, const std::string& currentFunction)
{
    std::string theExceptionMessage = theException.what();
    std::string outputMessage       = "Exception thrown during SM step " + currentFunction + " - catched exception message: " + theExceptionMessage;
    inputReplayMessage.mutable_reply_type()->set_type(MessageUtils::ReplyType::ERROR);
    inputReplayMessage.set_message(outputMessage.c_str());
}

std::string MiddlewareMessageHandler::firmwareAction(const std::string& message)
{
    MessageUtils::ReplyMessage         theReplyMessage;
    MessageUtils::FirmwareQueryMessage theFirmwareMessage;
    theFirmwareMessage.ParseFromString(message);
    const std::string& configurationFile = theFirmwareMessage.configuration_file();
    uint32_t           boardIdRaw        = theFirmwareMessage.board_id();
    if(boardIdRaw > 0xFFFF)
    {
        std::runtime_error theError("Board Id has to be contained in 16 bits");
        catchFunction(theReplyMessage, theError, __PRETTY_FUNCTION__);
        return serializeMessage(theReplyMessage);
    }
    uint16_t boardId = boardIdRaw;

    switch(theFirmwareMessage.action())
    {
    case MessageUtils::FirmwareQueryMessage::LIST:
    {
        try
        {
            MessageUtils::FirmwareReplyMessage theFirmwareListReply;
            std::vector<std::string>           firmwareList = fMiddlewareStateMachine.getFirmwareList(configurationFile, boardId);
            theFirmwareListReply.mutable_reply_type()->set_type(MessageUtils::ReplyType::SUCCESS);

            for(const auto& firmware: firmwareList) theFirmwareListReply.add_firmware_name(firmware);

            return serializeMessage(theFirmwareListReply);
        }
        catch(const std::exception& theException)
        {
            catchFunction(theReplyMessage, theException, __PRETTY_FUNCTION__);
        }
        break;
    }

    case MessageUtils::FirmwareQueryMessage::LOAD:
    {
        const std::string& firmwareName = theFirmwareMessage.firmware_name();
        theReplyMessage                 = tryCatchWrapper(__PRETTY_FUNCTION__, &MiddlewareStateMachine::loadFirmwareInFPGA, configurationFile, firmwareName, boardId);
        break;
    }

    case MessageUtils::FirmwareQueryMessage::UPLOAD:
    {
        const std::string& firmwareName = theFirmwareMessage.firmware_name();
        const std::string& fileName     = theFirmwareMessage.file_name();
        theReplyMessage                 = tryCatchWrapper(__PRETTY_FUNCTION__, &MiddlewareStateMachine::uploadFirmwareOnSDcard, configurationFile, firmwareName, fileName, boardId);
        break;
    }

    case MessageUtils::FirmwareQueryMessage::DOWNLOAD:
    {
        const std::string& firmwareName = theFirmwareMessage.firmware_name();
        const std::string& fileName     = theFirmwareMessage.file_name();
        theReplyMessage                 = tryCatchWrapper(__PRETTY_FUNCTION__, &MiddlewareStateMachine::downloadFirmwareFromSDcard, configurationFile, firmwareName, fileName, boardId);
        break;
    }

    case MessageUtils::FirmwareQueryMessage::DELETE:
    {
        const std::string& firmwareName = theFirmwareMessage.firmware_name();
        theReplyMessage                 = tryCatchWrapper(__PRETTY_FUNCTION__, &MiddlewareStateMachine::deleteFirmwareFromSDcard, configurationFile, firmwareName, boardId);
        break;
    }

    default:
    {
        std::runtime_error theError("MiddlewareMessageHandler::firmwareAction not able to identity action");
        catchFunction(theReplyMessage, theError, __PRETTY_FUNCTION__);
        break;
    }
    }

    return serializeMessage(theReplyMessage);
}

std::string MiddlewareMessageHandler::calibrationList(const std::string& message)
{
    MessageUtils::CalibrationListReplyMessage theCalibrationList;
    theCalibrationList.mutable_reply_type()->set_type(MessageUtils::ReplyType::SUCCESS);

    for(const auto& calibrationHardware: fMiddlewareStateMachine.getCombinedCalibrationFactory().getAvailableCalibrations())
    {
        auto theCalibrationPerHardwareTypeListMessage = theCalibrationList.add_calibrationperhardwaretype();
        theCalibrationPerHardwareTypeListMessage->set_hardwaretype(calibrationHardware.first);

        for(const auto& theCalibrationList: calibrationHardware.second)
        {
            auto theCalibrationListMessage = theCalibrationPerHardwareTypeListMessage->add_calibrationinfo();
            theCalibrationListMessage->set_calibrationname(theCalibrationList.first);
            for(const auto& theSubCalibrationList: theCalibrationList.second)
            {
                auto theSubCalibrationListMessage = theCalibrationListMessage->add_subcalibrationinfo();
                theSubCalibrationListMessage->set_subcalibrationname(theSubCalibrationList.first);
                theSubCalibrationListMessage->set_subcalibrationdescription(theSubCalibrationList.second);
            }
        }
    }

    return serializeMessage(theCalibrationList);
}
