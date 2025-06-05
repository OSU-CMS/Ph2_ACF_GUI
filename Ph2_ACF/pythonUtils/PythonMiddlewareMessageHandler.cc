
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include "pybind11/pybind11.h"
#pragma GCC diagnostic pop
#include "Utils/easylogging++.h"
#include "iostream"
#include "miniDAQ/MiddlewareMessageHandler.cc"
#include "string"

INITIALIZE_EASYLOGGINGPP

void configureLogger(std::string loggerConfigFile)
{
    el::Configurations conf(loggerConfigFile);
    el::Loggers::reconfigureAllLoggers(conf);
}

auto stringConverter(std::string (MiddlewareMessageHandler::*function)(const std::string&))
{
    return [function](MiddlewareMessageHandler& theMiddlewareMessageHandler, const std::string& theMessage) -> pybind11::bytes { return (theMiddlewareMessageHandler.*function)(theMessage); };
}

PYBIND11_MODULE(Ph2_ACF_PythonInterface, handle)
{
    handle.doc() = "Handle for MiddlewareMessageHandler";

    pybind11::class_<MiddlewareMessageHandler>(handle, "MiddlewareMessageHandler")
        .def(pybind11::init<>())
        .def("initialize", stringConverter(&MiddlewareMessageHandler::initialize))
        .def("configure", stringConverter(&MiddlewareMessageHandler::configure))
        .def("start", stringConverter(&MiddlewareMessageHandler::start))
        .def("stop", stringConverter(&MiddlewareMessageHandler::stop))
        .def("halt", stringConverter(&MiddlewareMessageHandler::halt))
        .def("pause", stringConverter(&MiddlewareMessageHandler::pause))
        .def("resume", stringConverter(&MiddlewareMessageHandler::resume))
        .def("abort", stringConverter(&MiddlewareMessageHandler::abort))
        .def("status", stringConverter(&MiddlewareMessageHandler::status))
        .def("calibrationList", stringConverter(&MiddlewareMessageHandler::calibrationList))
        .def("firmwareAction", stringConverter(&MiddlewareMessageHandler::firmwareAction));

    handle.def("configureLogger", &configureLogger);
}
