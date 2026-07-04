# Legacy Stream Component Test Coverage

## Scope

This document covers the plain legacy stream component tests under `tests/component/net/`.

The scope is:

- `net::in::stream::legacy`
- `net::in6::stream::legacy`
- `net::un::stream::legacy`

These tests focus on raw stream server/client behavior and intentionally exclude HTTP, Express, WebSocket, SSE, MQTT, TLS, database integrations, and application-level tests.

## Coverage Matrix

| Behavior | IPv4 legacy stream | IPv6 legacy stream | Unix-domain legacy stream |
| --- | --- | --- | --- |
| Composition | `InetLegacyServerClientCompositionTest` | `Inet6LegacyServerClientCompositionTest` | `UnixLegacyServerClientCompositionTest` |
| One-shot payload exchange | `InetLegacyServerClientPayloadExchangeTest` | `Inet6LegacyServerClientPayloadExchangeTest` | `UnixLegacyServerClientPayloadExchangeTest` |
| Multi-message payload exchange | `InetLegacyServerClientMultiMessagePayloadExchangeTest` | `Inet6LegacyServerClientMultiMessagePayloadExchangeTest` | `UnixLegacyServerClientMultiMessagePayloadExchangeTest` |
| Large payload exchange | `InetLegacyServerClientLargePayloadExchangeTest` | `Inet6LegacyServerClientLargePayloadExchangeTest` | `UnixLegacyServerClientLargePayloadExchangeTest` |
| Framed payload reconstruction | `InetLegacyServerClientFramedPayloadExchangeTest` | `Inet6LegacyServerClientFramedPayloadExchangeTest` | `UnixLegacyServerClientFramedPayloadExchangeTest` |
| Connect failure | `InetLegacyClientConnectFailureTest` | `Inet6LegacyClientConnectFailureTest` | `UnixLegacyClientConnectFailureTest` |
| Client-controlled disconnect lifecycle | `InetLegacyServerClientDisconnectLifecycleTest` | `Inet6LegacyServerClientDisconnectLifecycleTest` | `UnixLegacyServerClientDisconnectLifecycleTest` |
| Server-controlled disconnect lifecycle | `InetLegacyServerControlledCloseLifecycleTest` | `Inet6LegacyServerControlledCloseLifecycleTest` | `UnixLegacyServerControlledCloseLifecycleTest` |
| Multiple-client composition | `InetLegacyServerMultipleClientsCompositionTest` | `Inet6LegacyServerMultipleClientsCompositionTest` | `UnixLegacyServerMultipleClientsCompositionTest` |
| Multiple-client payload exchange | `InetLegacyServerMultipleClientsPayloadExchangeTest` | `Inet6LegacyServerMultipleClientsPayloadExchangeTest` | `UnixLegacyServerMultipleClientsPayloadExchangeTest` |
| Multiple-client disconnect lifecycle | `InetLegacyServerMultipleClientsDisconnectLifecycleTest` | `Inet6LegacyServerMultipleClientsDisconnectLifecycleTest` | `UnixLegacyServerMultipleClientsDisconnectLifecycleTest` |

## Composition tests

Composition tests prove that each legacy server/client pair can be constructed, configured, started, connected, and shut down through the component-test harness.

## One-shot payload tests

One-shot payload tests prove that a single client payload can traverse the raw stream connection and be echoed or observed by the peer.

## Multi-message payload tests

Multi-message payload tests prove that repeated application writes over one connection are delivered and reconstructed without requiring a reconnect.

## Large payload tests

Large payload tests prove that payloads larger than a typical small receive buffer can be transferred and reconstructed completely.

## Framed payload tests

Stream sockets do not preserve application message boundaries. The framed payload tests verify test-local length-prefixed reconstruction from a byte stream. They do not assert receive-callback, packet, or write-call boundaries.

## Connect-failure tests

Connect-failure tests prove that clients report deterministic failure state when no compatible listening peer is available.

## Disconnect lifecycle tests

Disconnect lifecycle tests prove both client-controlled and server-controlled close paths, including the expected connected/disconnected state transitions and callback ordering visible to the component tests.

## Multiple-client tests

Multiple-client tests prove that one legacy stream server can accept more than one client, exchange payloads with each client, and handle independent client disconnect lifecycles.

## Design Rules for This Test Layer

- Keep this layer focused on plain legacy stream server/client behavior.
- Add tests as IPv4 / IPv6 / Unix-domain triplets when the behavior is transport-generic.
- Do not rely on TCP packet boundaries.
- Do not rely on one `sendToPeer()` call becoming one `onReceivedFromPeer()` callback.
- For stream-message behavior, test reconstruction semantics instead of callback boundaries.
- Keep protocol-layer tests separate from raw stream component tests.
- Do not mix TLS, HTTP, WebSocket, SSE, MQTT, DB, or application tests into this layer.
- Prefer deterministic behavior over timing-sensitive assertions.

## Deferred / Separate Test Tracks

- TLS stream component tests: cover encrypted stream setup, certificate handling, and TLS-specific connection behavior separately.
- HTTP parser / formatter tests: cover HTTP message parsing and formatting without mixing protocol assertions into raw stream tests.
- Express routing tests: cover routing, middleware, and application-facing request handling in the Express layer.
- WebSocket tests: cover upgrade handling, frames, close semantics, and WebSocket-specific message behavior.
- SSE tests: cover event-stream formatting, reconnect hints, and server-to-client event delivery.
- MQTT tests: cover MQTT packet semantics, session behavior, subscriptions, and broker/client interactions.
- queued-write / backpressure behavior: needs a clear observable contract before adding tests because it is more timing-sensitive than the current deterministic legacy stream tests.
- database / MariaDB tests: cover database connection, query, and callback behavior outside the raw stream layer.
- application-level smoke tests: cover integrated example or application behavior separately from component-level raw stream contracts.

## Maintenance Notes

- This document should be updated when new component-test families are added.
- New raw legacy stream tests should be checked against the matrix first to avoid duplicate coverage.
- Protocol-layer tests should get their own coverage documents when those tracks become systematic.
