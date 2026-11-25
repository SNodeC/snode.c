# JSON-RPC 2.0

`jsonrpc::JsonRpc` is a symmetric peer: every connection can issue and answer requests. It owns whole-message parsing, validation, incoming reply obligations, outgoing correlation and timers, and batch aggregation. The structure follows the MQTT module: nonvirtual intrinsic handlers call virtual application hooks, an owning context attaches itself to its engine, stream contexts delegate lifecycle events, and WebSocket templates have separately exported role instantiations. There are no JSON-RPC client/server engines.

The [JSON-RPC 2.0 specification](https://www.jsonrpc.org/specification) defines the envelope and batch behavior. Message structs in `jsonrpc/messages` are plain data with `toJson()` and `serialize()`. Optional params/data distinguish omission from an explicit null; params, when present, must be an object or array. Ids support strings, integers and null. Applications should use non-null ids.

## Build a protocol

```cpp
#include <jsonrpc/JsonRpc.h>
#include <jsonrpc/messages/Request.h>

class Calculator : public jsonrpc::JsonRpc {
    using Super = jsonrpc::JsonRpc;
public:
    Calculator() : Super("calculator") {}
private:
    void onRequest(const jsonrpc::messages::Request& request) final {
        if (request.method == "add" && request.params && request.params->is_array()) {
            sendResult(request.id, (*request.params)[0].get<double>() + (*request.params)[1].get<double>());
        } else {
            Super::onRequest(request);
        }
    }
};
```

`onRequest`, `onNotification`, `onResult`, `onError` and `onRequestTimeout` have safe defaults. The default request hook answers Method not found. A request hook can retain the id and call `sendResult` or `sendError` later on the event-loop thread; duplicate or unknown answers are dropped. Incoming ids and outgoing ids are independent namespaces. Outgoing responses are removed from the outstanding table before application hooks run. `sendRequest(method, params, timeout)` allocates an integer id; zero timeout disables the timer. Disconnect/destruction cancels timers and discards connection state. Overrides of lifecycle hooks call the base implementation.

Derived protocols configure `setBatchesEnabled`, `setMaximumMessageSize`, and `setMaximumNestingDepth`. Defaults are batches enabled, 1 MiB messages and 128 nested containers. Batch replies are emitted once all requests have answers, including deferred answers; notifications produce no replies. Limits are checked before dispatch. Invalid framing closes the connection; oversized whole messages and excessive nesting produce Invalid Request.

## Bind transports

A `core::socket::stream::SocketContextFactory` returns `new jsonrpc::SocketContext(connection, new Calculator)`. The context owns the calculator. Use that same factory with `net::in`, `net::in6`, `net::un`, Bluetooth stream transports, TLS variants, or `net::stdio::stream::SocketEndpoint`. No transport-specific protocol implementation is needed.

The default `SocketContext::Framing::Newline` terminates each compact JSON message with a newline, rejects raw embedded newlines on output, and bounds incomplete input by the engine message limit. `Framing::ContentLength` uses `Content-Length: <byte count>\r\n\r\n` headers, with an 8192-byte default header bound; duplicate, missing, malformed and oversized lengths close the connection. Both handle fragmented and coalesced input. Truncated input at EOF receives a parse error before write shutdown. EOF and shutdown are forwarded as `onSignal(0)` before draining output.

For WebSocket, a server factory creates `jsonrpc::server::SubProtocol(context, "jsonrpc", new Calculator)`; a client uses `jsonrpc::client::SubProtocol`. Text messages carry one complete JSON-RPC message; fragmentation is accumulated with a size bound. Binary messages close with code 1003. Oversized messages close with code 1009. There is no MQTT-style byte replay.

HTTP is not a context adapter. Protocol-specific HTTP bindings own their exchanges/sessions and drive a `JsonRpcContext`, whose `getSocketConnection()` may return null.

```cmake
find_package(snodec CONFIG REQUIRED COMPONENTS jsonrpc jsonrpc-server-websocket jsonrpc-client-websocket)
target_link_libraries(my_protocol PRIVATE snodec::jsonrpc)
```

The core component exports `snodec::jsonrpc` (`snodec-jsonrpc`) and depends on nlohmann_json, core timers and core-socket-stream. It has no web dependency. WebSocket components export `snodec::jsonrpc-server-websocket` and `snodec::jsonrpc-client-websocket`, each linking its corresponding WebSocket role. See `src/apps/jsonrpc` for a calculator served by one protocol class over TCP, Unix sockets, stdio and express WebSocket upgrade, with an outgoing peer example.
