# MCP application and protocol regressions

`McpReadTest` checks exact source-header byte ranges over stdio and HTTP, including the complete structured result and its serialized text mirror. `McpReadSdkTest` repeats the declaration read through the official TypeScript SDK on both transports.

To enable the SDK test, install `@modelcontextprotocol/client@2.4.0` in an external test dependency directory and configure `SNODEC_MCP_SDK_ROOT` with the package directory containing `package.json` and `dist/`. Node.js must be available. The server and normal build do not depend on this test package.

`McpSourceSearch_stdio` and `McpSourceSearch_http` search the source header tree with explicit eight-second budgets per search. Both exercise an open subscription and an independent read answered within 750 ms while the search remains active. `TimerEqualDeadlineTest` controls the existing clock boundary in its test executable to give distinct timers identical deadlines, then checks restart and cancellation without changing the production clock.

`McpShowcaseTour_*` drives SNode.C's public C++ client against the separate snodec-mcp-showcase executables in both protocol eras over stdio, HTTP and HTTPS; `McpShowcaseWire_*` independently checks raw envelopes. Only the two SDK stdio showcase tests are disabled by owner decision because SDK 2.4.0 drops the final progress callback when the result arrives in the same batch. Their exact assertions remain unchanged; SDK HTTP, read and client tests remain enabled when configured.

`McpHttpReplayTest` and `McpTlsReplayTest` disconnect a legacy GET SSE channel, delete its session, wait through the router cleanup interval, reject reuse of the deleted session, verify a fresh request and require clean server shutdown. They also retain the existing replay, eviction, expiry, isolation and no-duplicate-execution checks.

`McpDevelopmentInventory_*` guards both development-assistant transports and protocol eras against the exact `69b16419a` inventories: four tools, two resources, one prompt and one resource template, each on one page. It also rejects showcase registrations, synthetic HTTP routes and synthetic CLI policy options. Showcase tours, raw-wire tests, SDK tests and independent HTTP/HTTPS policy checks target `snodec-mcp-showcase-stdio` and `snodec-mcp-showcase-http`. The reusable router fixture regressions remain independent library controls.
