// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_CONFIG_H
#define MCP_SERVER_CONFIG_H
#include "utils/SubCommand.h"
#include "utils/Timeval.h"
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
namespace mcp::server {
    /// CLI11 configuration owner; create before init and retain through transport drain.
    class Config : public utils::SubCommand {
    public:
        static constexpr std::string_view NAME = "mcp";                                 ///< Stable CLI subcommand name.
        static constexpr std::string_view DESCRIPTION = "MCP limits and Origin policy"; ///< CLI help text.
        /// Borrow parent through destruction; configure on the event-loop thread.
        explicit Config(utils::SubCommand* parent);
        ~Config() override;                        ///< Destroy after all attached transports have drained.
        Config(const Config&) = delete;            ///< Stable CLI owner; copying is unsupported.
        Config& operator=(const Config&) = delete; ///< Stable CLI owner; assignment is unsupported.
        Config(Config&&) = delete;                 ///< CLI callbacks borrow this stable owner.
        Config& operator=(Config&&) = delete;      ///< CLI callbacks borrow this stable owner.
        /// Set the default before init; range 1..16777216; throws on invalid input; returns this.
        Config* setMaximumMessageBytes(std::size_t bytes);
        std::size_t getMaximumMessageBytes() const; ///< Read the effective message limit on the event loop.
        /// Set the default before init; range 1024..16777216; throws on invalid input; returns this.
        Config* setMaximumOutputBytes(std::size_t bytes);
        std::size_t getMaximumOutputBytes() const; ///< Read the effective output limit on the event loop.
        /// Set the default before init; range 1..300000 ms; throws on invalid input; returns this.
        Config* setDeadline(const utils::Timeval& deadline);
        utils::Timeval getDeadline() const; ///< Copy the effective timeout; new admissions capture it.
        /// Set owned default Origins before init; CLI values override defaults; returns this.
        Config* setAllowedOrigins(std::vector<std::string> origins);
        /// Borrow the authoritative list for this event-loop turn, never across reconfiguration.
        const std::vector<std::string>& getAllowedOrigins() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
    };
} // namespace mcp::server
#endif
