#ifndef TOWERCALCULATOR_H
#define TOWERCALCULATOR_H

#include "core/EventReceiver.h"
#include "utils/Timeval.h"

#include <deque>

class TowerCalculator : public core::EventReceiver {
public:
    TowerCalculator();

private:
    void* operator new(std::size_t size) = delete;

public:
    void calculate(long startValue);

protected:
    enum class State { WAITING, MULTIPLY, DIVIDE };

    void calculate();

    void event(const utils::Timeval& currentTime) override;

    long currentValue;

    std::deque<long> values;

    int multiplicator = 1;
    int divisor = 2;

    State state;
};

#endif // TOWERCALCULATOR_H
