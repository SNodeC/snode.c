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

#ifndef MANAGEDDESCRIPTOR_H
#define MANAGEDDESCRIPTOR_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

class ObservationCounter {
protected:
    int observationCounter = 0;
};

class EventReceiver : virtual public ObservationCounter {
public:
    EventReceiver() = default;

    EventReceiver(const EventReceiver&) = delete;

    EventReceiver& operator=(const EventReceiver&) = delete;

    virtual ~EventReceiver() = default;

    void observe() {
        observationCounter++;
        enabled = true;
    }

    void unobserve() {
        observationCounter--;
        enabled = false;
    }

    void checkObserved() {
        if (observationCounter == 0) {
            unobserved();
        }
    }

    bool isEnabled() const {
        return enabled;
    }

protected:
    virtual void enable() = 0;
    virtual void disable() = 0;

    virtual void unobserved() = 0;

private:
    bool enabled = false;
};

#endif // MANAGEDDESCRIPTOR_H
