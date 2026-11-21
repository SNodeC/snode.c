// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "mcp/server/Config.h"
#include <stdexcept>
#include <cmath>
namespace mcp::server {
    struct Config::Impl {
        CLI::Option *message, *output, *deadline, *origins;
        std::vector<std::string> allowedOrigins;
    };
    Config::Config(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Applications")
        , impl(std::make_unique<Impl>()) {
        impl->message =
            addOption("--maximum-message-bytes", "Maximum MCP message bytes", "bytes", std::size_t{1048576}, CLI::Range(1, 16777216));
        impl->output = addOption(
            "--maximum-output-bytes", "Maximum queued MCP output bytes", "bytes", std::size_t{1048576}, CLI::Range(1024, 16777216));
        const CLI::Range range(1.0, 300000.0);
        const CLI::Validator deadline(
            [range](std::string& value) -> std::string {
                const std::string error = range(value);
                return error.empty() && !std::isfinite(std::stod(value)) ? "Finite deadline required" : error;
            },
            range.get_description());
        impl->deadline = addOption("--deadline-ms", "Ordinary request deadline", "milliseconds", 30000.0, deadline);
        impl->origins =
            addOptionVariable(
                "--allowed-origin", impl->allowedOrigins, "Exact allowed browser Origin (repeatable)", "origin", CLI::Validator())
                ->expected(0, -1)
                ->force_callback();
        setDefaultValue(impl->origins, std::vector<std::string>{});
    }
    Config::~Config() = default;
    std::size_t Config::getMaximumMessageBytes() const {
        return impl->message->as<std::size_t>();
    }
    Config* Config::setMaximumMessageBytes(std::size_t bytes) {
        if (!setDefaultValue(impl->message, bytes)) {
            throw std::invalid_argument("Invalid maximum-message-bytes");
        }
        return this;
    }
    std::size_t Config::getMaximumOutputBytes() const {
        return impl->output->as<std::size_t>();
    }
    Config* Config::setMaximumOutputBytes(std::size_t bytes) {
        if (!setDefaultValue(impl->output, bytes)) {
            throw std::invalid_argument("Invalid maximum-output-bytes");
        }
        return this;
    }
    utils::Timeval Config::getDeadline() const {
        return utils::Timeval(impl->deadline->as<double>() / 1000.0);
    }
    Config* Config::setDeadline(const utils::Timeval& deadline) {
        if (!setDefaultValue(impl->deadline, deadline.getMsd())) {
            throw std::invalid_argument("Invalid deadline-ms");
        }
        return this;
    }
    const std::vector<std::string>& Config::getAllowedOrigins() const {
        return impl->allowedOrigins;
    }
    Config* Config::setAllowedOrigins(std::vector<std::string> origins) {
        setDefaultValue(impl->origins, origins);
        return this;
    }
} // namespace mcp::server
