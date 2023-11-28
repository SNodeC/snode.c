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

#include "utils/Exceptions.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {

    CallForCommandline::CallForCommandline(CLI::App* app, Mode mode)
        : CLI::Success("CallForCommandline", "A template command line is shown below:\n", CLI::ExitCodes::Success)
        , app(app)
        , mode(mode) {
    }

    CallForCommandline::~CallForCommandline() {
    }

    CLI::App* CallForCommandline::getApp() const {
        return app;
    }

    CallForCommandline::Mode CallForCommandline::getMode() const {
        return mode;
    }

    CallForShowConfig::CallForShowConfig(CLI::App* app)
        : CLI::Success("CallForPrintConfig", "Show current configuration", CLI::ExitCodes::Success)
        , app(app) {
    }

    CallForShowConfig::~CallForShowConfig() {
    }

    CLI::App* CallForShowConfig::getApp() const {
        return app;
    }

    CallForWriteConfig::CallForWriteConfig(const std::string& configFile)
        : CLI::Success("CallForWriteConfig", "Writing config file: " + configFile, CLI::ExitCodes::Success)
        , configFile(configFile) {
    }

    CallForWriteConfig::~CallForWriteConfig() {
    }

    std::string CallForWriteConfig::getConfigFile() const {
        return configFile;
    }

} // namespace CLI
