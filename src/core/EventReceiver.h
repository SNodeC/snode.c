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

#ifndef CORE_EVENTRECEIVER_H
#define CORE_EVENTRECEIVER_H

#include "core/Event.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

namespace utils {
    class Timeval;
} // namespace utils

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class EventReceiver {
    public:
        EventReceiver(const std::string& name);

        virtual void destruct();

    protected:
        virtual ~EventReceiver() = default;

    public:
        static void atNextTick(const std::function<void(void)>& callBack);

        void span();
        void relax();

        virtual void onEvent(const utils::Timeval& currentTime) = 0;

        const std::string& getName() const;

    private:
        Event event;
    };

} // namespace core

#endif // CORE_EVENTRECEIVER_H
