#include "Watchdog.h"

void Watchdog::Start(uint32_t pSeconds, std::function<void()> pCallback)
{
    fCallback      = pCallback;
    fLastResetTime = std::chrono::system_clock::now();
    fTimeout       = std::chrono::seconds(pSeconds);

    // if (std::atomic_comapre_exchange_strong (fRunning, false, true) )
    fRunning = true;
    fThread  = std::thread(&Watchdog::Workloop, this);
}

void Watchdog::Stop()
{
    if(fRunning.load())
    {
        std::unique_lock<std::mutex> cLock(fMutex);
        fRunning = false;
        fStopCondition.notify_all();
        cLock.unlock();

        if(fThread.joinable()) fThread.join();
    }
}

void Watchdog::Reset(uint32_t pSeconds)
{
    std::unique_lock<std::mutex> cLock(fMutex);
    fLastResetTime = std::chrono::system_clock::now();
    fTimeout       = std::chrono::seconds(pSeconds);
}

void Watchdog::Workloop()
{
    std::unique_lock<std::mutex> cLock(fMutex);

    while(fRunning.load() && (std::chrono::system_clock::now() - fLastResetTime < fTimeout)) // was stop called or Pet or Start just in time?
        // here the threads waits until:
        //  1. the condition_variable is notified in ::Stop()
        //  2. or the timeout expires
        //  3. or until spurious wakeup
        fStopCondition.wait_for(cLock, fTimeout);

    // cLock.unlock();

    if(fRunning.load())
    {
        fRunning = false;
        fCallback();
    }
}
