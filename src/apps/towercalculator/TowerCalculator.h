/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TOWERCALCULATOR_H
#define TOWERCALCULATOR_H

#include "core/EventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <cstddef>
#include <deque>

#endif // DOXYGEN_SHOULD_SKIP_THIS

class TowerCalculator : public core::EventReceiver {
public:
    void* operator new(std::size_t size) = delete;

    TowerCalculator();

    void calculate(long startValue);

protected:
    enum class State { WAITING, MULTIPLY, DIVIDE };

    void calculate();

    void onEvent(const utils::Timeval& currentTime) override;

    long currentValue;

    std::deque<long> values;

    int multiplicator = 1;
    int divisor = 2;

    State state;
};

#endif // TOWERCALCULATOR_H
