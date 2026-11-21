# SNode.C source structure

Paths below are relative to the repository root. Recheck line numbers against the checkout being used.

## Applications and lifecycle

`src/apps/mcp/main.cpp` is the complete development-server example. The HTTP variant creates a named `express::legacy::in::WebApp`, attaches configuration and finite HTTP defaults before `WebApp::init`, mounts an express router, calls `listen`, then `WebApp::start`. Its listen callback handles disabled/failed listeners. The stdio variant preserves the protocol descriptor before `core::SNodeC::init`, creates the transport, then enters `core::SNodeC::start`.

The event-loop entry points are declared in `src/core/SNodeC.h:66`: `init`, `start`, `stop`, and `free`. Copy the application's normal start/shutdown or initialization-failure cleanup path; do not create a second event loop. `core::timer::Timer::singleshotTimer` (`src/core/timer/Timer.h:73`) schedules the next bounded slice. The header search in `src/apps/mcp/Tools.cpp` demonstrates cancellation/deadline checks between slices and descriptor ownership without child processes or threads.

## Modules and configuration

Use `src/iot/mqtt` as the protocol-module template and `src/mcp/server/CMakeLists.txt` as the additive MCP example. Protocol dispatch belongs in `mcp::server::Server`; transports adapt framing and lifetime around the same registry. Targets `mcp-server`, `mcp-server-stdio`, and `mcp-server-http` have `snodec::` aliases and installed exports. Update the central inventory in `src/CMakeLists.txt` when adding another component; installing a library alone does not make it discoverable by consumers.

`mcp::server::Config` extends `utils::SubCommand`. Create it with `newSubCommand` before initialization, as shown in `src/apps/mcp/main.cpp`. Use `addOption` and the existing CLI11 validators instead of an independent argument parser. Named HTTP instance options live under `mcp-http`; its MCP limits live under `mcp-http mcp`. Stdio MCP options live under the root `mcp` section. Read the current executable's section `--help` for exact spellings and defaults.

Use `snode::log::application("component.name").info()` or `.error()` for application diagnostics, following `src/apps/mcp/main.cpp`. Stdio protocol output must remain on its retained descriptor: redirect ordinary stdout before initialization and default logging to stderr. CLI11 help and errors can otherwise corrupt the protocol.

## HTTP streaming and ownership

HTTP routing uses `express::Router`; MCP's router is `src/mcp/server/http/Router.cpp`. The HTTP middleware checks Content-Type and parser-attribute presence itself because `JsonMiddleware` intentionally ignores invalid JSON and media types. An authentication hook can inspect the express request before dispatch.

For ordinary responses use the express response API. For SSE use `Response::pipe()` with `core::pipe::Source`, send complete SSE records ending in a blank line, and finish through the source's protected `eof()`. Never mix `json()` or `send()` with an open stream. The MCP router's source implementation shows backpressure, queue bounds, disconnect cancellation, and shutdown ordering. SSE mode belongs to the actual response Content-Type, as enforced in `src/web/http/server/SocketContext.cpp:184`.

Private descriptor receiver classes need an out-of-line virtual definition under strict Clang warnings. Follow `src/core/pipe/PipeSource.h:99` and `src/core/pipe/PipeSource.cpp:75`; the MCP stdio receivers use out-of-line destructors in their implementation file.

## Verification

Use the existing CTest mechanism. `tests/unit/mcp/McpServerTest.cpp` exercises dispatch/schema behavior; `tests/component/mcp` exercises public stdio/HTTP boundaries and the development tools. `tests/component/http/InetHttpServerSseModeTest.cpp` verifies exact wire output, keep-alive reuse, and timeout restoration. Build the relevant targets, format only changed files with the committed configuration, and report actual commands and outcomes. For this repository's database fixtures run CTest with a short temporary directory outside the build tree.
