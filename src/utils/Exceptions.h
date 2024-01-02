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

#ifndef CLI_EXCEPTIONS_H
#define CLI_EXCEPTIONS_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
#ifdef __has_warning
#if __has_warning("-Wweak-vtables")
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#endif
#include "utils/CLI11.hpp" // IWYU pragma: export
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS
#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {

    class CallForCommandline : public CLI::Success {
    public:
        enum class Mode { REQUIRED, NONEDEFAULT, FULL, DEFAULT };

        CallForCommandline(CLI::App* app, const std::string& description, Mode mode);
        ~CallForCommandline() override;

        CLI::App* getApp() const;
        Mode getMode() const;

    private:
        CLI::App* app;
        Mode mode;
    };

    class CallForShowConfig : public CLI::Success {
    public:
        CallForShowConfig(CLI::App* app);
        ~CallForShowConfig() override;

        CLI::App* getApp() const;

    private:
        CLI::App* app;
    };

    class CallForWriteConfig : public CLI::Success {
    public:
        explicit CallForWriteConfig(const std::string& configFile);
        ~CallForWriteConfig() override;

        std::string getConfigFile() const;

    private:
        std::string configFile;
    };

} // namespace CLI

#endif // CLI_EXCEPTIONS_H
