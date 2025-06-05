#include "Utils/StartInfo.h"
#include "MessageUtils/cpp/QueryMessage.pb.h"

void StartInfo::parseProtobufMessage(const std::string& theConfigureInfoString)
{
    MessageUtils::StartMessage theStartMessage;
    theStartMessage.ParseFromString(theConfigureInfoString);

    fRunNumber         = theStartMessage.data().run_number();
    fAppendInformation = theStartMessage.data().append();
}

std::string StartInfo::createProtobufMessage() const
{
    MessageUtils::StartMessage theStartMessage;
    theStartMessage.mutable_query_type()->set_type(MessageUtils::QueryType::START);
    theStartMessage.mutable_data()->set_run_number(fRunNumber);
    theStartMessage.mutable_data()->set_append(fAppendInformation);

    std::string theStartMessageString;
    theStartMessage.SerializeToString(&theStartMessageString);
    return theStartMessageString;
}