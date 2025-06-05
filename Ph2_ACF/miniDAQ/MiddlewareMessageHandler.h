#ifndef __MIDDLEWARE_MESSAGE_HANDLER__
#define __MIDDLEWARE_MESSAGE_HANDLER__

#include "MessageUtils/cpp/QueryMessage.pb.h"
#include "MessageUtils/cpp/ReplyMessage.pb.h"
#include "MiddlewareStateMachine.h"
#include <fstream>
#include <iostream>
class MiddlewareMessageHandler
{
  public:
    MiddlewareMessageHandler();
    virtual ~MiddlewareMessageHandler();

    // State machine commands
    std::string initialize(const std::string& message);
    std::string configure(const std::string& message);
    std::string start(const std::string& message);
    std::string stop(const std::string& message);
    std::string halt(const std::string& message);
    std::string pause(const std::string& message);
    std::string resume(const std::string& message);
    std::string abort(const std::string& message);
    std::string status(const std::string& message);

    // FPGAconfig commands
    std::string firmwareAction(const std::string& message);

    // Calibration list command
    std::string calibrationList(const std::string& message);

    template <typename T>
    static std::string serializeMessage(const T& theMessage);

  private:
    MiddlewareStateMachine fMiddlewareStateMachine;

    template <typename... Args>
    MessageUtils::ReplyMessage tryCatchWrapper(std::string currentStep, void (MiddlewareStateMachine::*F)(Args...), typename std::remove_reference<Args>::type&... arguments)
    {
        MessageUtils::ReplyMessage theReplyMessage;
        theReplyMessage.mutable_reply_type()->set_type(MessageUtils::ReplyType::SUCCESS);
        try
        {
            (fMiddlewareStateMachine.*F)(arguments...);
        }
        catch(const std::exception& theException)
        {
            catchFunction(theReplyMessage, theException, currentStep);
        }
        return theReplyMessage;
    }

    void catchFunction(MessageUtils::ReplyMessage& inputReplayMessage, const std::exception& theException, const std::string& currentFunction);
};

template <typename T>
std::string MiddlewareMessageHandler::serializeMessage(const T& theMessage)
{
    std::string stringMessage;
    theMessage.SerializeToString(&stringMessage);
    return stringMessage;
}

#endif