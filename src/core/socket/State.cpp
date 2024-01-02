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

#include "core/socket/State.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <cstring>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket {

    State::State(const int& state, const std::string& file, const int& line)
        : state(state)
        , file(file)
        , line(line)
        , errnum(errno)
        , errstr(errnum != 0 ? std::string(std::strerror(errnum)).append(" [").append(std::to_string(errnum)).append("]") : "") {
    }

    State::State(const int& state, const std::string& file, const int& line, int errnum, const std::string& errstr)
        : state(state)
        , file(file)
        , line(line)
        , errnum(errnum)
        , errstr(errnum != 0 ? std::string(errstr).append(" [").append(std::to_string(errnum)).append("]") : "") {
    }

    State::operator int() const {
        return state;
    }

    bool State::operator==(const int& state) const {
        return this->state == state;
    }

    std::string State::what() const {
        std::string stateString;

        switch (state) {
            case OK:
                stateString = "OK";
                break;
            case DISABLED:
                stateString = "DISABLED";
                break;
            case ERROR:
                [[fallthrough]];
            case FATAL:
                stateString = errstr;
                break;
        }

        return stateString;
    }

    std::string State::where() const {
        return file + "(" + std::to_string(line) + ")";
    }

} // namespace core::socket
