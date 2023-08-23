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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdlib>
#include <iostream>
#include <mysql/mysql.h>

#endif // DOXYGEN_SHOULD_SKIP_THIS

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    MYSQL* mysql = nullptr;
    MYSQL* ret = nullptr;

    mysql = mysql_init(nullptr);
    std::cout << "mysql_options: " << mysql_options(mysql, MYSQL_OPT_NONBLOCK, 0) << std::endl;

    int status = mysql_real_connect_start(&ret, mysql, "localhost", "snodec", "pentium5", "snodec", 3306, "/run/mysqld/mysqld.sock", 0);

    while (status) {
        status = mysql_real_connect_cont(&ret, mysql, status);
    }

    if (ret == nullptr) {
        std::cout << "error mysql: " << mysql_error(mysql) << std::endl;
    } else {
        std::cout << "mysql_real_connect: SUCCESS" << std::endl;
    }

    return EXIT_SUCCESS;
}
