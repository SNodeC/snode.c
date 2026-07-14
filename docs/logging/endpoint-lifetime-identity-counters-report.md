# Endpoint lifetime identity and counters report

This update separates stream connection display names from semantic connection
identity. Shared SocketServer and SocketClient contexts allocate monotonically
increasing per-endpoint connection IDs, pass them through acceptor/connector and
legacy/TLS construction paths, and use them as the semantic `conn` value while
leaving `getConnectionName()` unchanged for display compatibility.

FlowController now records dispatched retry attempts, and ClientFlowController
records dispatched reconnect attempts. The counters intentionally live in the
flow controllers and do not introduce logging dependencies there.

Shared endpoint contexts own the instance-level semantic summary scope and emit
one Info lifetime summary only when a normal runtime retry/reconnect
continuation is rejected by SocketServer/SocketClient policy.
