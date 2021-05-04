#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <string>

class Timer {
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;

public:
    Timer();
    ~Timer() = default;
    std::string getDuration();
};

#endif /* TIMER_H */
