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

#include "net/config/ConfigCluster.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#include <cstddef>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    ConfigCluster::ConfigCluster() {
        if (!getName().empty()) {
            clusterSc = add_subcommand("cluster", "Options for clustering");
            clusterSc->group("Option groups");
            clusterSc->preparse_callback([this]([[maybe_unused]] std::size_t num) -> void {
                mode = MODE::PRIMARY;
            });
            clusterSc->configurable(false);

            modeOpt = clusterSc->add_option("--mode", mode, "Clustering mode");
            modeOpt->type_name("[" + std::to_string(MODE::STANDALONE) + " = STANDALONE, " + std::to_string(MODE::PRIMARY) +
                               " = PRIMARY, " + std::to_string(MODE::SECONDARY) + " = SECONDARY, " + std::to_string(MODE::PROXY) +
                               " = PROXY]");
            modeOpt->default_val(MODE::STANDALONE);
            modeOpt->configurable(false);
        }
    }

    int ConfigCluster::getClusterMode() const {
        return mode;
    }

} // namespace net::config
