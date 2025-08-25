#pragma once
#include <chrono>

class ChronoTimer {
public:
    bool TimeOver() const {
        double time = Elapsed();
        return timeLimit < time;
    }

    void SetTimer(double timeLimit) {
        this->timeLimit = timeLimit;
    }

    void Start() {
        startT = std::chrono::system_clock::now();
    }

    double Elapsed() const {
        auto now = std::chrono::system_clock::now();
        return static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(now - startT).count());
    }

    double TimeLimit() const {
        return timeLimit;
    }

private:
    std::chrono::system_clock::time_point startT;
    double timeLimit;
};
