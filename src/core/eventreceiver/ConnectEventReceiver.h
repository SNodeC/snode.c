/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef CORE_EVENTRECEIVER_CONNECTEVENTRECEIVER_H
#define CORE_EVENTRECEIVER_CONNECTEVENTRECEIVER_H

#include "core/DescriptorEventReceiver.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string> // for string

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MAX_CONNECT_INACTIVITY 10

namespace core::eventreceiver {

    class ConnectEventReceiver : public core::DescriptorEventReceiver {
    protected:
        ConnectEventReceiver(const std::string& name, const utils::Timeval& timeout = MAX_CONNECT_INACTIVITY);

    private:
        virtual void connectEvent() = 0;
        virtual void connectTimeout();

        void dispatchEvent() final;
        void timeoutEvent() final;
    };

    class InitConnectEventReceiver : public core::EventReceiver {
    protected:
        InitConnectEventReceiver(const std::string& name);

    private:
        void event(const utils::Timeval& currentTime) override;

        virtual void initConnectEvent() = 0;
    };

} // namespace core::eventreceiver

#endif // CORE_EVENTRECEIVER_CONNECTEVENTRECEIVER_H
