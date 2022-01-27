#include "gui_logger.h"

// Protocol:
//
// opcode:data\n
//
// opcode       data format
// =========================
// stat         text
// prog         number in [0, 1]
// msg          text
// warn         text
// err          text
// data         what>data

std::ofstream gui::pipe;

void gui::status(const char* status) { pipe << "stat:" << status << std::endl; }

void gui::progress(float progress) { pipe << "prog:" << progress << std::endl; }

void gui::message(const char* msg) { pipe << "msg:" << msg << std::endl; }

void gui::data(const char* what, const char* data) { pipe << "data:" << what << ">" << data << std::endl; }

void gui::LogDispatcher::handle(const el::LogDispatchData* data)
{
    if(data->logMessage()->level() == el::Level::Warning)
    {
        pipe << "warn:" << data->logMessage()->message() << std::endl;
        exit(0);
    }
    else if(data->logMessage()->level() == el::Level::Error)
    {
        pipe << "err:" << data->logMessage()->message() << std::endl;
        exit(0);
    }
    // switch(data->logMessage()->level())
    // {
    //     case el::Level::Warning:
    //         pipe << "warn:" << data->logMessage()->message() << std::endl;
    //         break;
    //     case el::Level::Error:
    //         pipe << "err:" << data->logMessage()->message() << std::endl;
    //         break;
    // }
}

void gui::init(const char* pipe_name)
{
    pipe.open(pipe_name);
    // el::Helpers::installLogDispatchCallback<LogDispatcher>("GuiLogDispatcher");
}