# Framework resource policies and descriptor streaming

SNode.C keeps transport mechanics and protocol resource limits in the framework. Applications remain responsible for protocol semantics and application policy, such as HTTP methods, authorization, `Host`, `Origin`, cookies, and Unix-peer trust decisions.

## Configuration hierarchy

The policies use the existing `utils::SubCommand`/CLI11 configuration tree. Named server and client instances therefore expose the values through the C++ configuration object, their instance subcommands, and the normal configuration-file machinery. Command-line values take precedence over configuration-file values, which take precedence over C++ defaults.

Every stream connection instance has the `connection` options below. HTTP server and client instances additionally register `http.parser` and `websocket`; only HTTP server instances have the server-policy options directly under `http`.

```text
named instance
├── connection
│   ├── maximum-write-queue-bytes
│   ├── write-queue-high-watermark
│   └── write-queue-low-watermark
├── http
│   ├── server-only policy (server instances)
│   └── parser
│       ├── maximum-start-line-bytes
│       ├── maximum-header-line-bytes
│       ├── maximum-header-bytes
│       ├── maximum-header-fields
│       └── maximum-body-bytes
└── websocket
    ├── maximum-frame-bytes
    ├── maximum-message-bytes
    └── maximum-fragments
```

For a named instance `api`, representative command lines are:

```shell
application api connection \
  --maximum-write-queue-bytes 1048576 \
  --write-queue-high-watermark 786432 \
  --write-queue-low-watermark 262144

application api http parser \
  --maximum-start-line-bytes 8192 \
  --maximum-header-line-bytes 8192 \
  --maximum-header-bytes 65536 \
  --maximum-header-fields 100 \
  --maximum-body-bytes 8388608

application api websocket \
  --maximum-frame-bytes 1048576 \
  --maximum-message-bytes 8388608 \
  --maximum-fragments 128
```

An HTTP server additionally accepts these options in its `http` section:

```shell
application api http \
  --maximum-pending-requests 16 \
  --allow-chunked-transfer=false \
  --allow-pipelining=false
```

All of these options are persistent. The equivalent configuration-file entries are:

```ini
api.connection.maximum-write-queue-bytes=1048576
api.connection.write-queue-high-watermark=786432
api.connection.write-queue-low-watermark=262144

api.http.maximum-pending-requests=16
api.http.allow-chunked-transfer=false
api.http.allow-pipelining=false
api.http.parser.maximum-start-line-bytes=8192
api.http.parser.maximum-header-line-bytes=8192
api.http.parser.maximum-header-bytes=65536
api.http.parser.maximum-header-fields=100
api.http.parser.maximum-body-bytes=8388608

api.websocket.maximum-frame-bytes=1048576
api.websocket.maximum-message-bytes=8388608
api.websocket.maximum-fragments=128
```

As with other SNode.C options, `--show-config` displays the effective tree and `--write-config` writes persistent options.

### Defaults

| Section | Option | Default | Meaning |
| --- | --- | ---: | --- |
| `http.parser` | `maximum-start-line-bytes` | `0` | Unlimited, preserving the previous behavior |
| `http.parser` | `maximum-header-line-bytes` | `8192` | Preserves the previous parser limit |
| `http.parser` | `maximum-header-bytes` | `0` | Unlimited |
| `http.parser` | `maximum-header-fields` | `0` | Unlimited |
| `http.parser` | `maximum-body-bytes` | `0` | Unlimited decoded body bytes |
| server `http` | `maximum-pending-requests` | `0` | Unlimited per connection |
| server `http` | `allow-chunked-transfer` | `true` | Accept chunked request bodies |
| server `http` | `allow-pipelining` | `true` | Accept pipelined requests |
| `connection` | `maximum-write-queue-bytes` | `0` | Unlimited |
| `connection` | `write-queue-high-watermark` | `0` | Use the legacy threshold: suspend above five write blocks, capped by a finite maximum queue size |
| `connection` | `write-queue-low-watermark` | `0` | Resume when the queue is empty |
| `websocket` | `maximum-frame-bytes` | `0` | Unlimited |
| `websocket` | `maximum-message-bytes` | `0` | Unlimited across all data fragments |
| `websocket` | `maximum-fragments` | `0` | Unlimited data frames per message |

The write-queue low watermark must not exceed the effective high watermark. An explicit high watermark must not exceed a finite maximum queue size. Configuration validation rejects inconsistent values.

Start-line and header-line byte limits include their wire line terminators. `maximum-header-bytes` counts the complete header section, including its terminating empty line; the same limit is applied separately to a chunked trailer section. `maximum-header-fields` likewise applies independently to header and trailer fields. `maximum-body-bytes` counts decoded entity bytes, not chunk framing.

`maximum-pending-requests` counts all parsed requests whose responses have not completed, including the request currently delivered to application middleware. With pipelining disabled, a second request is not delivered while the first is outstanding; already-buffered extra request bytes cause the connection to close after the current response.

## C++ configuration

The HTTP server and client each register an `http` subcommand. Both contain the same `ConfigHttpParser` type and pass an immutable `ParserLimits` snapshot into every connection parser. The server also snapshots `HttpServerPolicy`. WebSocket upgrades snapshot `ConfigWebSocket` receiver limits. For example:

```cpp
auto* config = server.getConfig();

config->setMaximumWriteQueueBytes(1024 * 1024)
    ->setWriteQueueHighWatermark(768 * 1024)
    ->setWriteQueueLowWatermark(256 * 1024);

auto* http = config->Instance::getSubCommand<web::http::server::ConfigHttpServer>();
http->setMaximumPendingRequests(16)->setAllowPipelining(false);
http->getParserConfig()->setMaximumHeaderBytes(64 * 1024)->setMaximumBodyBytes(8 * 1024 * 1024);

config->Instance::getSubCommand<web::http::ConfigWebSocket>()
    ->setMaximumFrameBytes(1024 * 1024)
    ->setMaximumMessageBytes(8 * 1024 * 1024)
    ->setMaximumFragments(128);

auto* clientHttp = client.getConfig()->Instance::getSubCommand<web::http::client::ConfigHTTP>();
clientHttp->getParserConfig()->setMaximumBodyBytes(8 * 1024 * 1024);
```

The configuration classes expose matching setters and getters for every option:

| Configuration class | Accessors |
| --- | --- |
| `net::config::ConfigConnection` | `set/getMaximumWriteQueueBytes`, `set/getWriteQueueHighWatermark`, `set/getWriteQueueLowWatermark` |
| `web::http::ConfigHttpParser` | `set/getMaximumStartLineBytes`, `set/getMaximumHeaderLineBytes`, `set/getMaximumHeaderBytes`, `set/getMaximumHeaderFields`, `set/getMaximumBodyBytes` |
| `web::http::server::ConfigHttpServer` | `set/getMaximumPendingRequests`, `set/getAllowChunkedTransfer`, `set/getAllowPipelining`, `getParserConfig`, `getParserLimits`, `getServerPolicy` |
| `web::http::client::ConfigHTTP` (`ConfigHttpClient`) | `getParserConfig`, `getParserLimits` |
| `web::http::ConfigWebSocket` | `set/getMaximumFrameBytes`, `set/getMaximumMessageBytes`, `set/getMaximumFragments` |

HTTP parser limits apply to both server requests and ordinary client responses. No new client-only policy was added: the client continues to use its existing `ConfigHTTP` section plus the shared nested parser section. Each connection receives value snapshots (`ParserLimits`, `HttpServerPolicy`, and WebSocket `Receiver::Limits`) rather than reading mutable configuration while processing traffic.

Valid bounded requests continue through the normal Express middleware chain; resource checks do not introduce a second admission callback. A WebSocket receiver limit violation uses close code `1009` (`Message Too Big`). The frame limit applies to data and control-frame payloads, while message and fragment limits apply to data messages. Sender framing remains unchanged because no separate deployment requirement currently justifies another tuning option.

Client `requestEventSource` responses are a deliberate streaming exception to `maximum-body-bytes`: after the HTTP response headers have been validated and the connection switches to the raw SSE event receiver, the HTTP parser no longer owns or accumulates that unbounded event stream. Header limits still apply. Applications that require an SSE byte or event budget must enforce it in the event receiver.

## Bounded socket writes and pipe backpressure

`core::socket::stream::SocketConnection::trySendToPeer(...)` and the corresponding `SocketContext` overloads return `QueueResult`:

- `Queued`: the complete input was appended;
- `WouldExceedLimit`: none of the input was appended;
- `Closed`: the writer is not available;
- `ShutdownInProgress`: write shutdown has begun.

The existing `sendToPeer(...)` API remains available. With a finite maximum configured, an overflow through that legacy void API is treated as a write error and fails the connection, preventing a live protocol stream with silently omitted bytes. Callers that need to recover or choose their own policy before connection failure should use `trySendToPeer(...)`.

Attached `core::pipe::Source` objects are suspended when the connection-local queue reaches the high watermark and resumed after it drains to the low watermark. A source is not resumed during write shutdown. Streamed HTTP headers, chunk framing, and payload fragments use atomic bounded admission, and a rejection terminates the stream and connection instead of emitting a partial fragment.

## Descriptor-based file streaming

`core::file::FileReader` can open a path, use POSIX `openat()` lookup, or adopt an already-authorized descriptor:

```cpp
// Choose one opening form.
core::file::FileReader* source = core::file::FileReader::open("assets/index.html");
// core::file::FileReader* source =
//     core::file::FileReader::open(directoryFd, "index.html", O_RDONLY | O_CLOEXEC);
// core::file::FileReader* source = core::file::FileReader::adopt(fd);

if (source != nullptr && !response->pipe(source)) {
    source->stop();
}
```

For `open(directoryFd, path, flags)`, relative paths are resolved relative to `directoryFd`, absolute paths have normal absolute-path behavior and ignore `directoryFd`, and `AT_FDCWD` selects the current working directory. This is exactly lookup delegation to `openat()`; it does **not** confine access beneath a directory or prevent symlink, `..`, mount, or rename races. Applications that require confinement must establish it separately and can pass the resulting descriptor to `adopt()`.

The overload has no mode argument, so flags that require one (`O_CREAT` and, where available, `O_TMPFILE`) fail with `EINVAL`. A successful `adopt(fd)` transfers ownership immediately: the caller must not close or reuse `fd`, and `FileReader` closes it exactly once. Failed opens return `nullptr` and preserve `errno`.

`web::http::server::Response::pipe(core::pipe::Source*)` and `express::Response::pipe(core::pipe::Source*)` connect a source to the existing `Source`/`Sink` lifecycle. Backpressure, EOF, source errors, queue admission failures, and disconnects use that lifecycle; applications do not need a second file-copy loop.

## Unix peer credentials

`net::un::peerCredentials(fd)` reports peer facts for a connected Unix-domain socket:

```cpp
const net::un::PeerCredentials peer = net::un::peerCredentials(fd);
if (peer.status == net::un::PeerCredentialsStatus::Success) {
    // peer.uid and peer.gid are verified transport facts.
    // peer.pid is present on Linux and absent on getpeereid platforms.
}
```

Linux uses `SO_PEERCRED`; supported BSD/macOS targets use `getpeereid()`. Other targets return `Unsupported`, and syscall failures return `Error` with the platform error number in `peer.error`. SNode.C does not turn these facts into authorization: same-user, trusted-user, or other acceptance decisions remain application policy.

## Migration and compatibility

No configuration change is needed to retain the previous runtime limits and pipe thresholds. Existing source APIs remain available, while applications can progressively adopt the result-returning queue API, descriptor opening, response piping, and credential query.

This release changes installed class layouts and the `SocketConnection` virtual interface to carry immutable policy snapshots and queue results. Rebuild applications and plugins against the updated SNode.C headers and libraries; binary objects built against the previous layouts are not ABI-compatible. The legacy callback-based `FileReader::open(path, callback)` now returns `nullptr` after a failed open, so callers should test the result before piping it.
