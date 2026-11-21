// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "SkeletonPreviewTool.h"

#include "Tools.h"
#include "mcp/server/Request.h"
#include "mcp/server/Results.h"
#include "mcp/server/Types.h"

#include <map>
#include <memory>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <string>
namespace apps::mcp {
    SkeletonPreviewTool::SkeletonPreviewTool()
        : Tool({"skeleton.preview",
                "Preview an express application or router module. Does not write files.",
                parseSchema("skeleton.preview", R"json(
{
    "additionalProperties": false,
    "properties": {
        "kind": {
            "default": "application",
            "enum": [
                "application",
                "module"
            ]
        }
    },
    "type": "object"
}
)json")}) {
    }
    void SkeletonPreviewTool::call(const ::mcp::server::ToolCall& request, const std::shared_ptr<::mcp::server::ToolReply>& reply) {
        const auto& args = request.arguments;
        const bool module = args.value("kind", std::string("application")) == "module";
        ::mcp::server::Json files;
        if (module) {
            files = {{"src/example/CMakeLists.txt", "add_subdirectory(server)\n"},
                     {"src/example/server/Router.h", R"preview(#pragma once
#include <express/Router.h>
namespace example::server {
    class Router : public express::Router {
    public:
        Router();
    };
}
)preview"},
                     {"src/example/server/Router.cpp", R"preview(#include "example/server/Router.h"
#include <express/Response.h>
namespace example::server {
    Router::Router() {
        get("/", [](const auto&, const auto& response) {
            response->send("SNode.C");
        });
    }
}
)preview"},
                     {"src/example/server/CMakeLists.txt", R"preview(add_library(example-server SHARED Router.cpp Router.h)
add_library(snodec::example-server ALIAS example-server)
target_compile_features(example-server PUBLIC cxx_std_20)
target_link_libraries(example-server PUBLIC http-server-express)
target_include_directories(example-server
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>")
set_target_properties(example-server PROPERTIES
    VERSION ${SNode.C_VERSION} SOVERSION ${SNODEC_SOVERSION})
install(TARGETS example-server EXPORT snodec_example-server_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/example
    COMPONENT example-server)
install(FILES Router.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/example/server
    COMPONENT example-server)
install(EXPORT snodec_example-server_Targets
    FILE snodec_example-server_Targets.cmake NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec COMPONENT example-server)
)preview"}};
        } else {
            files = {{"src/apps/example/main.cpp", R"preview(#include <express/legacy/in/WebApp.h>
#include <express/Response.h>
int main(int argc, char* argv[]) {
    using WebApp = express::legacy::in::WebApp;
    const WebApp app("example");
    WebApp::init(argc, argv);
    app.get("/", [](const auto&, const auto& response) {
        response->send("SNode.C");
    });
    app.listen([](const auto&, const auto&) {});
    return WebApp::start();
}
)preview"},
                     {"src/apps/example/CMakeLists.txt", R"preview(add_executable(example main.cpp)
target_compile_features(example PRIVATE cxx_std_20)
target_link_libraries(example PRIVATE http-server-express net-in-stream-legacy)
set_target_properties(example PROPERTIES
    INSTALL_RPATH "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}")
install(TARGETS example RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT apps)
)preview"}};
        }
        const ::mcp::server::Json value{
            {"files", files},
            {"notes",
             module ? "Add example to src/CMakeLists.txt and the central component/export inventory; review before writing."
                    : "The apps directory discovers example/CMakeLists.txt; review before writing."}};
        reply->result(::mcp::server::ToolResult::json(value));
    }
} // namespace apps::mcp
