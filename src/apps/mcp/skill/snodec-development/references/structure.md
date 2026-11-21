# Established SNode.C examples

Paths refer to a SNode.C source checkout.

1. For an HTTP application, follow `src/apps/http/httpserver.cpp`: create a named WebApp, attach its configuration subcommand, call `WebApp::init`, add middleware, listen, then return `WebApp::start`. The raw echo example in `src/apps/echo/echoserver.cpp` uses `core::SNodeC::init`, a server listen callback, and `core::SNodeC::start`.

2. Handle the four listen states shown in both examples: `core::socket::State::OK`, `DISABLED`, `ERROR`, and `FATAL`. They log the instance name; OK adds the address, while ERROR/FATAL add error information.

3. Choose the legacy or TLS WebApp variant using the compile-time aliases in `src/apps/http/model/servers.h`. Both `getWebApp` factories construct the selected WebApp with an instance name.

4. Follow `src/apps/http/ConfigWWW.h` and `.cpp` for application configuration: derive from `utils::SubCommand`, add an option with a CLI11 validator, and expose a typed getter. Options may be marked required: `ConfigWWW.cpp:54` does this for `--html-root`, while `setHtmlRoot()` supplies a default and lifts the requirement (`ConfigWWW.cpp:58,60`). The HTTP application attaches this subcommand with `newSubCommand` before initialization.

5. Follow `src/apps/main.cpp` for express routing: `use` attaches middleware and nested `express::Router` objects; `get` registers handlers that reply with `send` or `json`. Its `/sse` example sets SSE headers, calls `sendHeader`, sends `sendFragment` fragments from an interval timer, and stops that timer when `isConnected()` is false. For open-ended browser streams use that timer + `sendFragment` pattern; for finite streamed responses use `Response::pipe()` with a `core::pipe::Source` and complete through Source EOF (see “Embedding the MCP module” in `mcp-usage.md`).

6. Use the semantic logger shown by the HTTP and echo callbacks: `snode::log::application().info()`, `.error()`, and `.critical()`. `src/Log.h` declares the application logger factory and level-specific streaming methods.

7. Follow `src/iot/mqtt/CMakeLists.txt` and `src/iot/mqtt/server/CMakeLists.txt` for module layout: client/server subdirectories, shared-library targets, `snodec::` aliases, explicit dependencies, installed public headers, and component-scoped target exports.

8. Use the existing CTest helper in `tests/cmake/AddSNodeCTest.cmake`: `snodec_add_test` creates an executable, registers it, and sets a timeout. `tests/support/TestResult.h` provides `tests::support::TestResult` with `expectTrue`, `expectEqual`, and `processResult`.
