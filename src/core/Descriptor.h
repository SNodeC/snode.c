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

#ifndef CORE_DESCRIPTOR_H
#define CORE_DESCRIPTOR_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class Descriptor {
    public:
        Descriptor() = delete;
        Descriptor(const Descriptor& d) = delete;

    protected:
        explicit Descriptor(int fd);
        Descriptor(Descriptor&& descriptor) noexcept;

        virtual ~Descriptor();

    public:
        Descriptor& operator=(int fd);
        Descriptor& operator=(const Descriptor& descriptor) = delete;
        Descriptor& operator=(Descriptor&& descriptor) noexcept;

        int getFd() const;

    private:
        int fd = -1;
    };

} // namespace core

#endif // CORE_DESCRIPTOR_H
