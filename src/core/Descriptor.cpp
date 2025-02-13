/*
 * SNode.C - a slim toolkit for network communication
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

#include "core/Descriptor.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/unistd.h"

#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    Descriptor::Descriptor(int fd)
        : fd(fd) {
    }

    Descriptor::Descriptor(Descriptor&& descriptor) noexcept {
        fd = std::exchange(descriptor.fd, -1);
    }

    Descriptor& Descriptor::operator=(Descriptor&& descriptor) noexcept {
        fd = std::exchange(descriptor.fd, -1);

        return *this;
    }

    Descriptor& Descriptor::operator=(int fd) {
        this->fd = fd;

        return *this;
    }

    Descriptor::~Descriptor() {
        if (fd >= 0) {
            core::system::close(fd);
            fd = -1;
        }
    }

    int Descriptor::getFd() const {
        return fd;
    }

} // namespace core
