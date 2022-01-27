#include "easylogging++.h"
#include <fstream>
#include <unistd.h>

#ifndef gui_logger
#define gui_logger
namespace gui
{
extern std::ofstream pipe;

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

void status(const char*);

void progress(float);

void message(const char*);

void data(const char*, const char*);

class LogDispatcher : public el::LogDispatchCallback // el::base::DefaultLogDispatchCallback
{
  protected:
    void handle(const el::LogDispatchData*);
};

void init(const char*);
} // namespace gui

#endif