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

#ifndef CORE_PIPE_SINK_H
#define CORE_PIPE_SINK_H

namespace core::pipe {
    class Source;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::pipe {

    class Sink {
    public:
        Sink(Sink&) = delete;
        Sink& operator=(Sink&) = delete;

    protected:
        Sink() = default;

        Sink(Sink&&) noexcept = default;

        Sink& operator=(Sink&&) noexcept = default;

        virtual ~Sink();

    private:
        void pipe(Source* source);

    protected:
        bool isStreaming();
        void stop();

    private:
        void streamData(const char* chunk, std::size_t chunkLen);
        void streamEof();
        void streamError(int errnum);

        void disconnect(const Source* source);

        virtual void onSourceConnect(Source* source) = 0;
        virtual void onSourceData(const char* chunk, std::size_t chunkLen) = 0;
        virtual void onSourceEof() = 0;
        virtual void onSourceError(int errnum) = 0;

        Source* source = nullptr;

        friend class Source;
    };

} // namespace core::pipe

#endif // CORE_PIPE_SINK_H
