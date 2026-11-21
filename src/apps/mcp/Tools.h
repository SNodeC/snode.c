// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#pragma once
#include "mcp/server/Config.h"
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"

#include <cstddef>
#include <string>
namespace CLI {
    class Option;
}
namespace utils {
    class SubCommand;
}
namespace apps::mcp {
    class Headers;
    enum class LookupMode { Find, Read };
    std::shared_ptr<Headers> openHeaders(const std::string& root);
    void lookup(const std::shared_ptr<Headers>& headers,
                const ::mcp::server::ToolCall& call,
                const std::shared_ptr<::mcp::server::ToolReply>& reply,
                LookupMode mode,
                std::size_t files,
                std::size_t bytes);
    ::mcp::server::Json parseSchema(const std::string& name, const char* text, const char* field = "inputSchema");
    class DevelopmentConfig : public ::mcp::server::Config {
    public:
        explicit DevelopmentConfig(utils::SubCommand* parent);
        std::string getHeaderRoot() const;
        std::size_t getSearchFiles() const;
        std::size_t getSearchBytes() const;

    private:
        CLI::Option *headerRoot, *searchFiles, *searchBytes;
    };
    std::string instructions();
} // namespace apps::mcp
