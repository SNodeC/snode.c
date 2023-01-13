#include "TowerCalculator.h"

#include <iostream>
#include <string>

TowerCalculator::TowerCalculator()
    : core::EventReceiver("TowerCalculator")
    , currentValue(0)
    , state(State::WAITING) {
}

void TowerCalculator::calculate(long startValue) {
    values.push_back(startValue);

    calculate();
}

void TowerCalculator::calculate() {
    if (!values.empty() && state == State::WAITING) {
        currentValue = values.front();
        values.pop_front();

        state = State::MULTIPLY;
        multiplicator = 1;
        divisor = 2;

        std::cout << std::endl << "Starting calculation with value = " << currentValue << std::endl;

        span();
    }
}

void TowerCalculator::onEvent([[maybe_unused]] const utils::Timeval& currentTime) {
    switch (state) {
        case State::MULTIPLY:
            if (multiplicator < 10) {
                std::cout << currentValue << " * " << multiplicator << " = ";
                currentValue = currentValue * multiplicator;
                std::cout << currentValue << std::endl;
                multiplicator++;
            } else {
                state = State::DIVIDE;
            }
            span();
            break;
        case State::DIVIDE:
            if (divisor < 10) {
                std::cout << currentValue << " / " << divisor << " = ";
                currentValue = currentValue / divisor;
                std::cout << currentValue << std::endl;
                divisor++;
            } else {
                state = State::WAITING;
            }
            span();
            break;
        case State::WAITING:
            calculate();
            break;
    }
}
