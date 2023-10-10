/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef CORE_SOCKET_STATE_H
#define CORE_SOCKET_STATE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <cstring>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket {

    //    enum class State { OK, DISABLED, ERROR, FATAL };

    class State {
    public:
        constexpr static int OK = 0;
        constexpr static int DISABLED = 1;
        constexpr static int ERROR = 2;
        constexpr static int FATAL = 3;

#define Ok State(core::socket::State::OK, __FILE__, __LINE__)
#define Disabled State(core::socket::State::DISABLED, __FILE__, __LINE__)
#define Error State(core::socket::State::ERROR, __FILE__, __LINE__)
#define Fatal State(core::socket::State::FATAL, __FILE__, __LINE__)

        State(const int& state)
            : state(state)
            , errnum(errno) {
        }

        State(const int& state, const std::string& file, const int& line)
            : state(state)
            , file(file)
            , line(line)
            , errnum(errno) {
        }

        operator int() {
            return state;
        }

        bool operator==(const int& state) {
            return this->state == state;
        }

        std::string what() {
            std::string stateString;

            switch (state) {
                case OK:
                    stateString = "OK: ";
                    break;
                case DISABLED:
                    stateString = "DISABLED: ";
                    break;
                case ERROR:
                    stateString = "ERROR in " + file + " at line " + std::to_string(line) + ": " + std::strerror(errnum) + " [" +
                                  std::to_string(errnum) + "]";
                    break;
                case FATAL:
                    stateString = "FATAL in " + file + " at line " + std::to_string(line) + ": " + std::strerror(errnum) + " [" +
                                  std::to_string(errnum) + "]";
                    break;
            }

            return stateString;
        }

    private:
        int state;
        std::string file;
        int line = 0;

        int errnum;
    };

} // namespace core::socket

#endif // CORE_SOCKET_STATE_H
