# HTTP Module (`src/web/http/*`) and EventSource

## Module role

The HTTP module sits above `net` and delivers reusable client/server HTTP building blocks, including request/response abstractions, parser utilities, status handling, mime support, upgrades, and SSE client support.

## Structure overview

- `web/http/server/*`: server request parsing, response composition, context factories
- `web/http/client/*`: client request API, response parser, command pipeline
- `web/http/*` common: status codes, MIME map, cookie options, HTTP helpers, case-insensitive header map
- `legacy/*` + `tls/*`: transport bindings for family/protocol combinations

## Server side

Key components:

- `RequestParser`: incremental parse of request line, headers, and body handling semantics.
- `Request`: normalized access to method/path/query/headers/cookies/body.
- `Response`: status/header/body construction and connection-state decisions.
- `SocketContextFactory` and upgrade selectors: plugin point for protocol upgrades (e.g., WebSocket).

## Client side

Key components:

- `Client`, `Request`, `Response`, and parser classes.
- Command objects (`SendHeaderCommand`, `SendFileCommand`, `SendFragmentCommand`, `UpgradeCommand`, `SseCommand`, `EndCommand`) provide composable, async-style request emission.
- Upgrade factory selector mirrors server strategy for negotiated protocol transitions.

## Shared HTTP utilities

- `http_utils`: URL encode/decode, trim/split helpers, HTTP date conversion, textual dumps.
- `StatusCodes`: canonical status maps.
- `MimeTypes`: extension/content-type resolution.
- `CookieOptions`: declarative cookie attributes.

## EventSource (SSE) in detail

EventSource support is implemented in `web/http/client/tools/EventSource*` and intentionally mirrors JavaScript EventSource semantics:

- ready states: `CONNECTING`, `OPEN`, `CLOSED`
- event model: `onOpen`, `onError`, `onMessage`, and typed listeners via `addEventListener`
- parser behavior for SSE fields (`data`, `event`, `id`, `retry`)
- `lastEventId` retention and reconnect retry handling
- origin/path tracking in shared state

### Why this matters

SSE support is unusually strong for a C++ framework and enables low-overhead server push patterns where full-duplex WebSocket is unnecessary.

## Functionality summary

- production-oriented HTTP server/client abstractions
- transport-independent HTTP APIs (legacy/tls and multiple network families)
- protocol upgrade extension points
- built-in SSE client EventSource implementation

## Pros

1. **Complete vertical slice** from parser to response writer.
2. **Pluggable upgrades** align naturally with WebSocket and custom protocols.
3. **Command-based client API** enables clean request composition.
4. **Solid SSE support** with JS-like developer ergonomics.

## Cons / tradeoffs

1. **Extensive class graph** can be overwhelming at first contact.
2. **Manual pipeline composition** may be verbose versus highly opinionated HTTP frameworks.
3. **Need clear docs/examples** for upgrade factory customization to unlock full value.
