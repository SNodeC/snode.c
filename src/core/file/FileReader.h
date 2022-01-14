/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#ifndef CORE_FILE_FILEREADER_H
#define CORE_FILE_FILEREADER_H

#include "core/ReadEventReceiver.h"
#include "core/file/File.h" // IWYU pragma: export
#include "core/pipe/Source.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::pipe {
    class Sink;
}

namespace core::file {

    class FileReader
        : public core::ReadEventReceiver
        , public core::pipe::Source
        , virtual public File {
    protected:
        FileReader(int fd, core::pipe::Sink& writeStream);

    public:
        static FileReader* connect(const std::string& path, core::pipe::Sink& writeStream, const std::function<void(int err)>& onError);

        void readEvent() override;

    private:
        void terminate() override;

        void sinkDisconnected() override;

        void unobservedEvent() override;

        bool continueReadImmediately() const override;
    };

} // namespace core::file

#endif // CORE_FILE_FILEREADER_H
