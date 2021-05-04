#include "timer.h"
#include <chrono>

const double MILLISECOND = 1000;

Timer::Timer() { m_startTime = std::chrono::high_resolution_clock::now(); }
std::string Timer::getDuration()
{
    auto now = std::chrono::high_resolution_clock::now();
    auto startInstant
        = std::chrono::time_point_cast<std::chrono::microseconds>(m_startTime)
              .time_since_epoch()
              .count();
    auto stopInstant
        = std::chrono::time_point_cast<std::chrono::microseconds>(now)
              .time_since_epoch()
              .count();

    auto duration = stopInstant - startInstant;
    double ms = duration / MILLISECOND;
    return std::to_string(ms);
}
