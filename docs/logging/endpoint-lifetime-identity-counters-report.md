# Endpoint lifetime identity and counters report

This update separates stream connection display names from semantic connection
identity. Shared SocketServer and SocketClient contexts allocate monotonically
increasing per-endpoint connection IDs, pass them through acceptor/connector and
legacy/TLS construction paths, and use them as the semantic `conn` value while
leaving `getConnectionName()` unchanged for display compatibility. Unnamed endpoints deliberately retain the framework `ConfigInstance` behavior and emit `inst=<anonymous> conn=<n>`.

FlowController now records dispatched retry attempts, and ClientFlowController
records dispatched reconnect attempts. The counters intentionally live in the
flow controllers and do not introduce logging dependencies there.

Shared endpoint contexts own the instance-level semantic summary scope and emit
one Info lifetime summary exactly once when either SocketServer/SocketClient
policy rejects a normal runtime retry/reconnect continuation, or the EventLoop
performs graceful shutdown. Graceful shutdown includes SIGINT, SIGTERM, SIGHUP,
and `SNodeC::stop()`. The same Context-owned exactly-once guard covers both
paths, so a policy-triggered summary is not repeated during `EventLoop::free()`.
SIGKILL, crashes, aborts, and immediate process termination cannot guarantee a
final endpoint summary.

Constructor compatibility note: `SocketConnection`, `SocketConnectionT`, `SocketAcceptor`, `SocketConnector`, all legacy acceptor/connector/connection variants, and all TLS acceptor/connector/connection variants now carry the explicit connection-ID allocator or value. These installed public headers changed source and ABI compatibility, so dependent applications and libraries must be rebuilt.
