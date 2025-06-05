#include <iostream>

#include "System/SystemController.h"

INITIALIZE_EASYLOGGINGPP

int main()
{
    Ph2_System::SystemController sysCon;
    std::cout << "I've instantiated a Ph2_System::SystemController" << '\n';

    Ph2_HwInterface::RD53Event::ForkDecodingThreads();
    std::cout << "I've used RD53Event::ForkDecodingThreads()" << '\n';

    Ph2_HwInterface::RD53Event::JoinDecodingThreads();
    std::cout << "I've used RD53Event::JoinDecodingThreads()" << '\n';

    return 0;
}
