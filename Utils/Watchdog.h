/*

    \file                          Watchdog.h
    \brief                         Simple watchdog timer to recover from stuck code
    \author                        Georg Auzinger
    \version                       1.0
    \date                          8/02/17
    Support :                      mail to : georg.auzinger@SPAMNOTcern.ch

 */

#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

class Watchdog
{
  private:
    std::function<void()>                 fCallback;
    std::mutex                            fMutex;
    std::condition_variable               fStopCondition;
    std::chrono::system_clock::time_point fLastResetTime;
    std::chrono::seconds                  fTimeout;
    std::thread                           fThread;
    std::atomic<bool>                     fRunning;

  public:
    Watchdog() : fTimeout(std::chrono::seconds(0)), fRunning(false) {}

    ~Watchdog() {}

    void Start(uint32_t pSeconds, std::function<void()> pCallback);
    void Stop();
    void Reset(uint32_t pSeconds);

  private:
    void Workloop();
};

#endif
