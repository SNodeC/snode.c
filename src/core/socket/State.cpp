/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
        , errstr(errnum != 0 ? std::strerror(errnum) : "") {
    }

    State::State(const int& state, const std::string& file, const int& line, int errnum, const std::string& errstr)
        : state(state)
        , file(file)
        , line(line)
        , errnum(errnum)
        , errstr(errnum != 0 ? errstr : "") {
    }

    State::operator int() const {
        return state;
    }

    bool State::operator==(const int& state) const {
        return this->state == state;
    }

    State& State::operator=(int state) {
        this->state = state;

        return *this;
    }

    State& State::operator|=(int state) {
        this->state |= state;

        return *this;
    }

    State& State::operator&=(int state) {
        this->state &= state;

        return *this;
    }

    State& State::operator^=(int state) {
        this->state ^= state;

        return *this;
    }

    State State::operator|(int state) {
        return State(this->state | state, this->file, this->line, this->errnum, this->errstr);
    }

    State State::operator&(int state) {
        return State(this->state & state, this->file, this->line, this->errnum, this->errstr);
    }

    State State::operator^(int state) {
        return State(this->state ^ state, this->file, this->line, this->errnum, this->errstr);
    }

    std::string State::what() const {
        std::string stateString;

        switch (state & ~NO_RETRY) {
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

        return stateString.append(" [").append(std::to_string(errnum)).append("]");
    }

    std::string State::where() const {
        return file + "(" + std::to_string(line) + ")";
    }

} // namespace core::socket
