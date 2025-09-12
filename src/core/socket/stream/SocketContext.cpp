/*
 * SNode.C - A Slim Toolkit for Network Communication
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

#include "core/socket/stream/SocketContext.h"

#include "core/socket/stream/SocketConnection.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cerrno>
#include <ctime>
#include <iomanip>
#include <sstream>
struct tm;

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    SocketContext::SocketContext(SocketConnection* socketConnection)
        : socketConnection(socketConnection)
        , onlineSinceTimePoint(std::chrono::system_clock::now()) {
    }

    void SocketContext::switchSocketContext(SocketContext* newSocketContext) {
        LOG(DEBUG) << socketConnection->getConnectionName() << " SocketContext: switch";
        socketConnection->switchSocketContext(newSocketContext);
    }

    SocketConnection* SocketContext::getSocketConnection() const {
        return socketConnection;
    }

    void SocketContext::sendToPeer(const char* chunk, std::size_t chunkLen) const {
        socketConnection->sendToPeer(chunk, chunkLen);
    }

    bool SocketContext::streamToPeer(pipe::Source* source) const {
        return socketConnection->streamToPeer(source);
    }

    void SocketContext::streamEof() {
        socketConnection->streamEof();
    }

    std::size_t SocketContext::readFromPeer(char* chunk, std::size_t chunklen) const {
        return socketConnection->readFromPeer(chunk, chunklen);
    }

    void SocketContext::setTimeout(const utils::Timeval& timeout) {
        socketConnection->setTimeout(timeout);
    }

    void SocketContext::close() {
        socketConnection->close();
    }

    std::size_t SocketContext::getTotalSent() const {
        return socketConnection->getTotalSent();
    }

    std::size_t SocketContext::getTotalQueued() const {
        return socketConnection->getTotalQueued() - alreadyTotalQueued;
    }

    std::size_t SocketContext::getTotalRead() const {
        return socketConnection->getTotalRead();
    }

    std::size_t SocketContext::getTotalProcessed() const {
        return socketConnection->getTotalProcessed() - alreadyTotalProcessed;
    }

    std::string SocketContext::getOnlineSince() const {
        return timePointToString(onlineSinceTimePoint);
    }
    std::string SocketContext::getOnlineDuration() const {
        return durationToString(onlineSinceTimePoint);
    }

    void SocketContext::shutdownRead() {
        socketConnection->shutdownRead();
    }

    void SocketContext::shutdownWrite(bool forceClose) {
        socketConnection->shutdownWrite(forceClose);
    }

    void SocketContext::onWriteError(int errnum) {
        errno = errnum;

        PLOG(DEBUG) << socketConnection->getConnectionName() << " SocketContext: onWriteError";
        shutdownRead();
    }

    void SocketContext::onReadError(int errnum) {
        errno = errnum;

        if (errno == 0) {
            LOG(DEBUG) << socketConnection->getConnectionName() << " SocketContext: EOF received";
        } else {
            PLOG(DEBUG) << socketConnection->getConnectionName() << " SocketContext: onReadError";
        }
        shutdownWrite();
    }

    void SocketContext::attach() {
        alreadyTotalQueued = socketConnection->getTotalQueued();
        alreadyTotalProcessed = socketConnection->getTotalProcessed();

        onConnected();
    }

    void SocketContext::detach() {
        onDisconnected();

        LOG(DEBUG) << socketConnection->getConnectionName() << " SocketContext: detach";
        LOG(DEBUG) << "     Online Since: " << getOnlineSince();
        LOG(DEBUG) << "  Online Duration: " << getOnlineDuration();
        LOG(DEBUG) << "       Total Sent: " << getTotalQueued();
        LOG(DEBUG) << "  Total Processed: " << getTotalProcessed();
    }

    std::string SocketContext::timePointToString(const std::chrono::time_point<std::chrono::system_clock>& timePoint) {
        const std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
        std::tm* tm_ptr = std::gmtime(&time);

        char buffer[100];
        std::string onlineSince = "Formatting error";

        // Format: "2025-02-02 14:30:00"
        if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_ptr) > 0) {
            onlineSince = std::string(buffer) + " UTC";
        }

        return onlineSince;
    }

    std::string SocketContext::durationToString(const std::chrono::time_point<std::chrono::system_clock>& bevore,
                                                const std::chrono::time_point<std::chrono::system_clock>& later) {
        using seconds_duration_type = std::chrono::duration<std::chrono::seconds::rep>::rep;

        const seconds_duration_type totalSeconds = std::chrono::duration_cast<std::chrono::seconds>(later - bevore).count();

        // Compute days, hours, minutes, and seconds
        const seconds_duration_type days = totalSeconds / 86400; // 86400 seconds in a day
        seconds_duration_type remainder = totalSeconds % 86400;
        const seconds_duration_type hours = remainder / 3600;
        remainder = remainder % 3600;
        const seconds_duration_type minutes = remainder / 60;
        const seconds_duration_type seconds = remainder % 60;

        // Format the components into a string using stringstream
        std::ostringstream oss;
        if (days > 0) {
            oss << days << " day" << (days == 1 ? "" : "s") << ", ";
        }
        oss << std::setw(2) << std::setfill('0') << hours << ":" << std::setw(2) << std::setfill('0') << minutes << ":" << std::setw(2)
            << std::setfill('0') << seconds;

        return oss.str();
    }

} // namespace core::socket::stream
