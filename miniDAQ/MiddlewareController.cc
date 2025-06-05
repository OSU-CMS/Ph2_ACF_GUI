#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "MiddlewareController.h"
#include "tools/CBCPulseShape.h"
#include "tools/CalibrationExample.h"
#include "tools/CombinedCalibration.h"
#include "tools/LatencyScan.h"
#include "tools/PedeNoise.h"
#include "tools/PedestalEqualization.h"
#include "tools/RD53ClockDelay.h"
#include "tools/RD53Gain.h"
#include "tools/RD53GainOptimization.h"
#include "tools/RD53InjectionDelay.h"
#include "tools/RD53Latency.h"
#include "tools/RD53Physics.h"
#include "tools/RD53PixelAlive.h"
#include "tools/RD53SCurve.h"
#include "tools/RD53ThrAdjustment.h"
#include "tools/RD53ThrEqualization.h"
#include "tools/RD53ThrMinimization.h"
#include "tools/Tool.h"
// #include "tools/SSAPhysics.h"
#include "tools/CicFEAlignment.h"
#include "tools/LinkAlignmentOT.h"
#include "tools/PSPhysics.h"
#include "tools/Physics2S.h"

#include "MessageUtils/cpp/QueryMessage.pb.h"
#include "MessageUtils/cpp/ReplyMessage.pb.h"

using namespace MessageUtils;

//========================================================================================================================
MiddlewareController::MiddlewareController(uint16_t portShift) : TCPServer(PORT_BASE + portShift, 1)
{
    // std::cout << "[" << __LINE__ << "]" << __PRETTY_FUNCTION__ << "come on!" << std::endl;
}

//========================================================================================================================
MiddlewareController::~MiddlewareController(void) { LOG(INFO) << __PRETTY_FUNCTION__ << " DESTRUCTOR" << RESET; }

//========================================================================================================================
std::string MiddlewareController::interpretMessage(const std::string& buffer)
{
    LOG(INFO) << __PRETTY_FUNCTION__ << " Message received from OTSDAQ: " << buffer << RESET;

    QueryMessage theInputQuery;
    theInputQuery.ParseFromString(buffer);
    theInputQuery.PrintDebugString();

    std::string replyString;

    switch(theInputQuery.query_type().type())
    {
    case QueryType::INITIALIZE:
    {
        replyString = fMiddlewareMessageHandler.initialize(buffer);
        break;
    }
    case QueryType::CONFIGURE:
    {
        ConfigurationMessage theConfigurationMessage;
        theConfigurationMessage.ParseFromString(buffer);
        replyString = fMiddlewareMessageHandler.configure(buffer);
        break;
    }
    case QueryType::START:
    {
        StartMessage theStartQuery;
        theStartQuery.ParseFromString(buffer);
        replyString = fMiddlewareMessageHandler.start(buffer);
        break;
    }
    case QueryType::STOP:
    {
        replyString = fMiddlewareMessageHandler.stop(buffer);
        break;
    }
    case QueryType::HALT:
    {
        replyString = fMiddlewareMessageHandler.halt(buffer);
        break;
    }
    case QueryType::PAUSE:
    {
        replyString = fMiddlewareMessageHandler.pause(buffer);
        break;
    }
    case QueryType::RESUME:
    {
        replyString = fMiddlewareMessageHandler.resume(buffer);
        break;
    }
    case QueryType::ABORT:
    {
        replyString = fMiddlewareMessageHandler.abort(buffer);
        break;
    }
    case QueryType::ERROR:
    {
        ReplyMessage theReplyMessage;
        theReplyMessage.mutable_reply_type()->set_type(ReplyType::ERROR);
        theReplyMessage.set_message("Received an Error message from the client");
        theReplyMessage.SerializeToString(&replyString);
        break;
    }
    case QueryType::STATUS:
    {
        replyString = fMiddlewareMessageHandler.status(buffer);
        break;
    }
    default:
    {
        ReplyMessage theReplyMessage;
        theReplyMessage.mutable_reply_type()->set_type(ReplyType::ERROR);
        theReplyMessage.set_message("Can't recognize message");
        theReplyMessage.SerializeToString(&replyString);
        break;
    }
    }

    return replyString;
}
