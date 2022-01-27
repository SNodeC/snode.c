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

#include "utils/Config.h"

#include "utils/CLI11.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <ostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace utils {

    Config::Config()
        : app(new CLI::App()) {
    }

    Config::~Config() {
        delete app;
    }

    Config& Config::instance() {
        static Config config;

        return config;
    }

    int Config::init(const std::string& name, int argc, char* argv[]) {
        this->name = name;
        this->argc = argc;
        this->argv = argv;

        app->description("Configuration file for application " + name);

        app->set_config("--config", name + ".conf", "Read an config file", false);

        CLI::Option* dumpConfigFlg = app->add_flag("-d,--dump-config", _dumpConfig, "Dump config file");
        dumpConfigFlg->configurable(false);

        parse(true);

        setRequired(_dumpConfig == false);

        return 0;
    }

    void Config::finish() {
        parse(true);

        if (_dumpConfig) {
            std::cout << "Dumping config file: " << name + ".conf" << std::endl;
            std::cout << app->config_to_str(true, true) << std::endl;

            std::ofstream confFile(std::string(name + ".conf"));
            confFile << app->config_to_str(true, true);
        }
    }

    CLI::App* Config::add_subcommand(const std::string& subcommand_name, const std::string& subcommand_description) {
        return app->add_subcommand(subcommand_name, subcommand_description);
    }

    CLI::App* Config::required(CLI::App* app, bool required) {
        return app->required(required & _required);
    }

    CLI::Option* Config::required(CLI::Option* option, bool required) {
        return option->required(required & _required);
    }

    void Config::setRequired(bool required) {
        _required = required;
    }

    int Config::parse(bool quiet) {
        try {
            app->parse(argc, argv);
        } catch (const CLI::ParseError& e) {
            if (!quiet) {
                //                return app->exit(0);
                exit(app->exit(e));
            }
        }

        return 0;
    }

} // namespace utils
