/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EVENTRECEIVER_H
#define EVENTRECEIVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <climits>
#include <ctime>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    class ObservationCounter {
    public:
        void destructIfUnobserved() {
            if (observationCounter == 0) {
                unobserved();
            }
        }

        bool isObserved() {
            return observationCounter > 0;
        }

    protected:
        virtual void unobserved() = 0;

        int observationCounter = 0;
    };

    class EventReceiver : virtual public ObservationCounter {
    protected:
        class TIMEOUT {
        public:
            static const long IGNORE = -2;
            static const long DEFAULT = -1;
            static const long DISABLE = LONG_MAX;
        };

        EventReceiver(long timeout)
            : maxInactivity(timeout) {
        }

        EventReceiver(const EventReceiver&) = delete;
        EventReceiver& operator=(const EventReceiver&) = delete;

        virtual ~EventReceiver() = default;

    public:
        void enabled() {
            ObservationCounter::observationCounter++;
            _enabled = true;
            lastTriggered = time(nullptr);
        }

        void disabled() {
            ObservationCounter::observationCounter--;
            _enabled = false;
        }

        bool isEnabled() const {
            return _enabled;
        }

        long getTimeout() const {
            return maxInactivity;
        }

    protected:
        void setTimeout(long timeout, long defaultTimeout) { // -3: do not change, -2: set default, -1 disable, ...
            if (timeout != TIMEOUT::IGNORE) {
                if (timeout != TIMEOUT::DEFAULT) {
                    this->maxInactivity = timeout;
                } else {
                    this->maxInactivity = defaultTimeout;
                }
            }
        }

        virtual void enable(long timeout) = 0;
        virtual void disable() = 0;

        long maxInactivity = LONG_MAX;

    private:
        bool _enabled = false;

    public:
        time_t lastTriggered = 0;
    };

} // namespace net

#endif // EVENTRECEIVER_H
