# SNode.C source conventions

The paths below identify source examples in a SNode.C checkout.

1. Configure named application instances before initialization. The HTTP example in `src/apps/mcp/main.cpp` creates `express::legacy::in::WebApp`, attaches configuration and HTTP limits, calls `WebApp::init`, mounts routes, calls `listen`, then enters `WebApp::start`. Follow its listener-failure `core::SNodeC::stop` and initialization-failure `core::SNodeC::free` paths.

2. Keep handler work bounded between event-loop iterations. Follow `src/apps/mcp/Tools.cpp`: schedule the next slice with `core::timer::Timer::singleshotTimer`, check the shared reply's deadline and active state, and retain descriptor ownership until completion or cancellation.

3. Follow the MQTT module layout under `src/iot/mqtt` and the MCP example under `src/mcp/server`. Keep registration/dispatch in `mcp::server::Server` and framing/lifetime in its stdio or HTTP transport. Define `snodec::` target aliases, install headers and exports, and add components to the central inventory in `src/CMakeLists.txt`.

4. Extend `utils::SubCommand` for application options, as `mcp::server::Config` does. Attach it with `newSubCommand` before initialization; use `addOption` with CLI11 validators. The development application's HTTP options are under the named `mcp-http` instance and its `mcp` section; stdio uses a root `mcp` section.

5. Use `snode::log::application("component.name").info()` or `.error()` for application diagnostics. For stdio, follow `src/apps/mcp/main.cpp` and `src/mcp/server/stdio/Transport.cpp`: retain the protocol descriptor and redirect ordinary stdout before `core::SNodeC::init`, then default console logging to stderr.

6. Route HTTP through `express::Router`. Follow `src/mcp/server/http/Router.cpp` for explicit Content-Type and JSON-attribute checks around `JsonMiddleware`, and for an authorization callback before dispatch. Stream SSE through `Response::pipe()` and `core::pipe::Source`, send records ending in a blank line, and complete through Source EOF. Keep `json()`/`send()` on the non-streaming path. The router demonstrates queue bounds, backpressure, disconnect cancellation, and shutdown ordering; `src/web/http/server/SocketContext.cpp` derives SSE mode from the response Content-Type.

7. Define descriptor receiver destructors out of line, following `src/core/pipe/PipeSource.h`/`.cpp` and the private MCP receivers in `src/mcp/server/stdio/Transport.cpp`.

8. Use the existing CTest mechanism. Follow `tests/unit/mcp` for dispatch/schema tests, `tests/component/mcp` for public transport/application behavior, and `tests/StagedInstalledConsumerTest.cmake` for installed-package checks.
