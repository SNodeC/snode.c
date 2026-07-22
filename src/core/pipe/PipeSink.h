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

#ifndef CORE_PIPE_PIPESINK_H
#define CORE_PIPE_PIPESINK_H

#include "core/eventreceiver/ReadEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <cstddef>
#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::pipe {

    /**
     * A self-owned asynchronous pipe reader.
     *
     * Instances are created by Pipe::releaseReadAsSink(). close() initiates
     * asynchronous EventLoop removal and destruction. Callbacks must never
     * delete the receiver and must not use its pointer after onClosed.
     */
    class PipeSink final : public core::eventreceiver::ReadEventReceiver {
    public:
        static constexpr std::size_t DEFAULT_MAX_BYTES_PER_EVENT = 256 * 1024;

        PipeSink(const PipeSink&) = delete;

        PipeSink& operator=(const PipeSink&) = delete;

        void close();

        void setOnData(const std::function<void(const char*, std::size_t)>& onData);
        void setOnEof(const std::function<void()>& onEof);
        void setOnError(const std::function<void(int)>& onError);
        void setOnClosed(const std::function<void()>& onClosed);
        void setOnShutdown(const std::function<void(const core::ShutdownContext&)>& onShutdown);

    private:
        explicit PipeSink(int fd,
                          std::size_t maxBytesPerEvent = DEFAULT_MAX_BYTES_PER_EVENT,
                          const utils::Timeval& timeout = utils::Timeval({60, 0}));
        ~PipeSink() override;

        void destruct() final;
        void readEvent() override;
        void unobservedEvent() override;
        void shutdownEvent(const core::ShutdownContext& context) override;
        void closeDescriptor() noexcept;

        std::function<void(const char*, std::size_t)> onData;
        std::function<void()> onEof;
        std::function<void(int errnum)> onError;
        std::function<void()> onClosed;
        std::function<void(const core::ShutdownContext&)> shutdownCallback;

        std::size_t maxBytesPerEvent;
        bool descriptorClosed = false;

        friend class Pipe;
    };

} // namespace core::pipe

#endif // CORE_PIPE_PIPESINK_H
