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

#ifndef CORE_FILE_FILEREADER_H
#define CORE_FILE_FILEREADER_H

#include "core/EventReceiver.h"
#include "core/file/File.h"
#include "core/pipe/Source.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <fcntl.h>
#include <functional>
#include <string>
#include <string_view>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::file {

    class FileReader
        : private core::EventReceiver
        , public core::pipe::Source
        , virtual public File {
    private:
        FileReader(int fd, const std::string& name, std::size_t pufferSize, int openErrno);

    public:
        /**
         * Opens path relative to the current working directory. Flags which
         * require an additional mode argument are rejected with EINVAL.
         */
        static FileReader* open(std::string_view path, int flags = O_RDONLY);

        /**
         * Compatibility overload which reports the open result before the
         * returned reader assumes ownership. It returns nullptr on failure.
         */
        static FileReader* open(const std::string& path, const std::function<void(int)>& callback);

        /**
         * Opens path with POSIX openat semantics: relative paths use
         * directoryFd, absolute paths ignore it, and AT_FDCWD uses the current
         * working directory. This API does not provide root confinement.
         * Flags which require an additional mode argument are rejected with
         * EINVAL. Returns nullptr on failure and leaves the error in errno.
         */
        static FileReader* open(int directoryFd, std::string_view path, int flags = O_RDONLY);

        /**
         * Transfers ownership of fd to a self-managed FileReader. Once called
         * with a non-negative descriptor, the descriptor is closed exactly
         * once even if reader construction throws.
         */
        static FileReader* adopt(int fd);

        bool isOpen() override;

        void start() final;
        void suspend() final;
        void resume() final;
        void stop() final;

    private:
        static FileReader* create(int fd, const std::string& name);

        void onEvent(const utils::Timeval& currentTime) override;

        std::size_t pufferSize = 0;

        bool suspended = false;
        bool stopping = false;

    protected:
        int openErrno = 0;
        bool running = false;
    };

} // namespace core::file

#endif // CORE_FILE_FILEREADER_H
