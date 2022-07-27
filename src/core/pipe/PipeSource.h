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

#ifndef STREAM_PIPESOURCE_H
#define STREAM_PIPESOURCE_H

#include "core/eventreceiver/WriteEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::pipe {

    class PipeSource : public core::eventreceiver::WriteEventReceiver {
        PipeSource(const PipeSource&) = delete;
        PipeSource& operator=(const PipeSource&) = delete;

    public:
        explicit PipeSource(int fd);
        ~PipeSource() override;

        void send(const char* junk, std::size_t junkLen);
        void send(const std::string& data);
        void eof();

        void setOnError(const std::function<void(int errnum)>& onError);

    protected:
        void writeEvent() override;
        void unobservedEvent() override;

    private:
        void terminate() override;

    protected:
        std::function<void(int errnum)> onError;

        std::vector<char> writeBuffer;
    };

} // namespace core::pipe

#endif // STREAM_PIPESOURCE_H
