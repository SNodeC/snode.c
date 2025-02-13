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

#ifndef CORE_SNODEC_H
#define CORE_SNODEC_H

#include "core/State.h" // IWYU pragma: export
#include "core/TickStatus.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <climits>
#include <cstddef>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class SNodeC {
    public:
        SNodeC() = delete;
        ~SNodeC() = delete;

        void* operator new(std::size_t count) = delete;

        static void init(int argc, char* argv[]);
        static int start(const utils::Timeval& timeOut = {LONG_MAX, 0});
        static void stop();
        static TickStatus tick(const utils::Timeval& timeOut = 0);
        static void free();

        static State state();
    };

} // namespace core

#endif // CORE_SNODEC_H
