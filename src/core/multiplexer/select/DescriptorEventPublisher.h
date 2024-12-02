/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#ifndef CORE_SELECT_DESCRIPTOREVENTPUBLISHER_H
#define CORE_SELECT_DESCRIPTOREVENTPUBLISHER_H

#include "core/DescriptorEventPublisher.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/select.h"

#include <string> // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::multiplexer::select {

    class FdSet {
    public:
        FdSet();

        void set(int fd);
        void clr(int fd);
        int isSet(int fd) const;
        void zero();
        fd_set& get();

    private:
        fd_set registered{};
        fd_set active{};
    };

    class DescriptorEventPublisher : public core::DescriptorEventPublisher {
    public:
        DescriptorEventPublisher(const std::string& name, FdSet& fdSet);

    private:
        void muxAdd(core::DescriptorEventReceiver* eventReceiver) override;
        void muxDel(int fd) override;
        void muxOn(core::DescriptorEventReceiver* eventReceiver) override;
        void muxOff(core::DescriptorEventReceiver* eventReceiver) override;

        void spanActiveEvents() override;

        FdSet& fdSet;
    };

} // namespace core::multiplexer::select

#endif // CORE_SELECT_DESCRIPTOREVENTPUBLISHER_H
