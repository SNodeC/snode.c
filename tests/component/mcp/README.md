# MCP development regressions

`McpReadTest` checks exact source-header byte ranges over stdio and HTTP, including the complete structured result and its serialized text mirror. `McpReadSdkTest` repeats the declaration read through the official TypeScript SDK on both transports.

To enable the SDK test, install `@modelcontextprotocol/client@2.4.0` in an external test dependency directory and configure `SNODEC_MCP_SDK_ROOT` with the package directory containing `package.json` and `dist/`. Node.js must be available. The server and normal build do not depend on this test package.
