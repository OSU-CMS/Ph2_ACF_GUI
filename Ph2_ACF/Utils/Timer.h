#ifndef __HWInterface_Timer_h
#define __HWInterface_Timer_h

#include <chrono>
#include <ctime>
#include <iostream>

class Timer
{
  public:
    Timer() : start_(), end_() {}

    virtual ~Timer() {}

    void start() { start_ = std::chrono::system_clock::now(); }
    void stop() { end_ = std::chrono::system_clock::now(); }

    void show(const std::string& label)
    {
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end_ - start_);
        std::time_t                   end_time  = std::chrono::system_clock::to_time_t(end_);

        const std::string& tnow = std::ctime(&end_time);
        std::cout << label << " finished at: " << tnow << "\telapsed time: " << time_span.count() << " seconds" << std::endl;
    }

    double getElapsedTime()
    {
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end_ - start_);
        return time_span.count();
    }

    double getCurrentTime()
    {
        std::chrono::system_clock::time_point now_      = std::chrono::system_clock::now();
        std::chrono::duration<double>         time_span = std::chrono::duration_cast<std::chrono::duration<double>>(now_ - start_);
        return time_span.count();
    }

    void reset() { start_ = end_; }

  private:
    std::chrono::system_clock::time_point start_, end_;
};

#endif
