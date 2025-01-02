/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef KEYBOARDREADER_H
#define KEYBOARDREADER_H

#include "core/eventreceiver/ReadEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif // DOXYGEN_SHOULD_SKIP_THIS

class KeyboardReader : core::eventreceiver::ReadEventReceiver {
public:
    explicit KeyboardReader(const std::function<void(long)>& cb);

protected:
    void readEvent() override;
    void unobservedEvent() override;

    std::function<void(long)> callBack;

    // void foo(long value);
};

#endif // KEYBOARDREADER_H
