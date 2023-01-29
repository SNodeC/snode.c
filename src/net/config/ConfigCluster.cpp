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

#include "net/config/ConfigCluster.h"

#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    ConfigCluster::ConfigCluster(ConfigInstance* instance)
        : net::config::ConfigSection(instance, "cluster", "Options for clustering") {
        add_option(modeOpt,
                   "--mode",
                   "Clustering mode",
                   std::to_string(MODE::STANDALONE) + " = STANDALONE, " + std::to_string(MODE::PRIMARY) + " = PRIMARY, " +
                       std::to_string(MODE::SECONDARY) + " = SECONDARY, " + std::to_string(MODE::PROXY) + " = PROXY",
                   MODE::STANDALONE)
            ->configurable(false)
            ->type_name("mode")
            ->check(CLI::Range(STANDALONE, PROXY));
    }

    ConfigCluster::MODE ConfigCluster::getClusterMode() const {
        return modeOpt->as<MODE>();
    }

    void ConfigCluster::setClusterMode(ConfigCluster::MODE newMode) {
        modeOpt->default_val(newMode)->clear();
    }

} // namespace net::config
