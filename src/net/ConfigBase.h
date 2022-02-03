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

#ifndef NET_CONFIGBASE_H
#define NET_CONFIGBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class App;
} // namespace CLI

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    class ConfigBase {
    public:
        explicit ConfigBase(const std::string& name);

        const std::string& getName() const;

        virtual int parse() const;

    protected:
        CLI::App* baseSc = nullptr;

    private:
        std::string name;
    };

} // namespace net

#endif // NET_CONFIGBASE_H
