/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

@ if '%{Cpp:PragmaOnce}'
#pragma once
    @ else
#ifndef % {GUARD }
#define % {GUARD }
    @endif

    @ if '%{Base}'
#include "%{Base}.h"

    @endif
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

    @ if '%{NameSp}' namespace %
    {NameSp} {
    @endif

        @ if '%{Base}' class
        % {CN} : public % {
        Base
    }
    @ else class % {
        CN
    }
    @endif {
    public:
        % {CN}();
        % {CN}(const % {CN}&) = default;

        % {CN}& operator=(const % {CN}&) = default;

        ~ % {CN}();
    };
    % {
    JS:
        Cpp.closeNamespaces('%{Class}')
    }
    @ if '%{NameSp}'
} // %{NameSp}
@endif

    @ if !'%{Cpp:PragmaOnce}'
#endif // %{GUARD}
    @endif
