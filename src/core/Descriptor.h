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

#ifndef CORE_DESCRIPTOR_H
#define CORE_DESCRIPTOR_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class Descriptor {
    public:
        Descriptor() = default;

        Descriptor(const Descriptor& d) = delete;
        Descriptor& operator=(const Descriptor& descriptor) = delete;

        ~Descriptor();

        virtual int open(int fd);
        int getFd() const;

        void dontClose(bool dontClose);
        bool dontClose() const;

        void close();

    protected:
        enum struct FLAGS : unsigned short {
            none = 0,
            dontClose = 0x01 << 0 // do not close sys-descriptor in case of desctruction
        } flags{FLAGS::none};

    private:
        int fd = -1;

        friend enum FLAGS operator|(const enum FLAGS& f1, const enum FLAGS& f2);
        friend enum FLAGS operator&(const enum FLAGS& f1, const enum FLAGS& f2);
    };

} // namespace core

#endif // CORE_DESCRIPTOR_H
