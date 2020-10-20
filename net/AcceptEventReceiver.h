/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef ACCEPTEVENTRECEIVER_H
#define ACCEPTEVENTRECEIVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "EventReceiver.h"

namespace net {

    class AcceptEventReceiver : public EventReceiver {
    protected:
        AcceptEventReceiver();

        virtual void acceptEvent() = 0;

        void dispatchEvent() override {
            acceptEvent();
        }

    public:
        void setTimeout(long timeout = TIMEOUT::DEFAULT);

        void enable(long timeout = TIMEOUT::IGNORE) override;
        void disable() override;

        void suspend() override;
        void resume() override;

        template <typename AcceptEventReceiver>
        friend class EventDispatcher;
    };

} // namespace net

#endif // ACCEPTEVENTRECEIVER_H
