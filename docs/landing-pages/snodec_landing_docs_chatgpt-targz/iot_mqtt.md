# MQTT module (`src/iot/mqtt/*`) — client, server/broker, packets, sessions

This document covers `src/iot/mqtt/*` **only**.

> Important: this documentation intentionally **does not cover** `src/iot/mqtt-fast/*`.
> The focus here is the regular `src/iot/mqtt` implementation.

SNode.C’s MQTT module implements MQTT protocol handling on top of SNode.C’s event-driven socket layer:

- event-driven reading/writing using `core/socket/stream::SocketContext`
- explicit packet parsing via a `ControlPacketDeserializer` state machine
- keepalive handling via `core::timer::Timer`
- server-side broker features: subscriptions, retained messages, sessions

---

## 1. Public API entry points

### Shared MQTT core
- `#include "iot/mqtt/Mqtt.h"` — base protocol engine (send publish/acks, keepalive)
- `#include "iot/mqtt/SocketContext.h"` — socket integration (MQTT context as socket context)
- `#include "iot/mqtt/MqttContext.h"` — abstract transport hooks for `Mqtt`
- `#include "iot/mqtt/FixedHeader.h"` — MQTT fixed header parsing/serialization
- `#include "iot/mqtt/ControlPacket.h"` — base control packet
- `#include "iot/mqtt/ControlPacketDeserializer.h"` — incremental packet deserialization
- `#include "iot/mqtt/Topic.h"` / `Session.h` — topic/session helpers

### Packets
- `src/iot/mqtt/packets/*` — CONNECT/CONNACK/PUBLISH/… (protocol packets)

### Client
- `#include "iot/mqtt/client/Mqtt.h"`
- `#include "iot/mqtt/client/SubProtocol.h"`

### Server / broker
- `#include "iot/mqtt/server/Mqtt.h"`
- `#include "iot/mqtt/server/SocketContextFactory.h"`
- Broker:
  - `#include "iot/mqtt/server/broker/Broker.h"`
  - `#include "iot/mqtt/server/broker/SubscriptionTree.h"`
  - `#include "iot/mqtt/server/broker/RetainTree.h"`
  - `#include "iot/mqtt/server/broker/Session.h"`

---

## 2. Architecture overview

At a high level, MQTT handling is split into:

1. **Socket integration** (`iot::mqtt::SocketContext`):
   - adapts a `core::socket::stream::SocketConnection` to MQTT’s “recv/send” model.
2. **Protocol engine** (`iot::mqtt::Mqtt`):
   - owns keepalive timer, packet identifier, session pointer
   - drives the `ControlPacketDeserializer` and dispatches parsed packets
3. **Role-specific subclasses**:
   - `iot::mqtt::client::Mqtt` / `SubProtocol`
   - `iot::mqtt::server::Mqtt` / `SubProtocol` + broker integration
4. **Broker state** (server-side):
   - subscription tree
   - retained message tree
   - session store (active + retained sessions)

The main benefit of this split is testability and composability:
the protocol engine can be reasoned about separately from the socket layer.

---

## 3. Socket integration: `iot::mqtt::SocketContext`

`iot::mqtt::SocketContext` (`src/iot/mqtt/SocketContext.h/.cpp`) is a bridge class:

- It **inherits** `core::socket::stream::SocketContext` so the event loop can drive it.
- It **implements** (privately) `iot::mqtt::MqttContext`, providing:
  - `recv(...)` / `send(...)`
  - `end()` / `close()`
  - socket access (`getSocketConnection()`)

Its overrides map socket events to protocol engine calls:

- `onConnected()` → `MqttContext::onConnected()` → `Mqtt::onConnected()`
- `onReceivedFromPeer()` → `MqttContext::onReceivedFromPeer()` → parse loop
- `onDisconnected()` → `MqttContext::onDisconnected()`
- `onSignal(sig)` → `MqttContext::onSignal(sig)` → `Mqtt::onSignal(sig)`

This is the standard SNode.C pattern:
**protocol logic lives in a module-specific context, but the loop drives it through core socket abstractions.**

---

## 4. The MQTT protocol engine: `iot::mqtt::Mqtt`

`iot::mqtt::Mqtt` (`src/iot/mqtt/Mqtt.h/.cpp`) is the core class that implements:

- fixed header parsing support (owns a `FixedHeader`)
- a pointer to the current `ControlPacketDeserializer`
- keepalive timer (`core::timer::Timer keepAliveTimer`)
- packet identifier generation
- sending helper functions for common control packets
- hooks for publish-related QoS flows

### 4.1 Packet deserialization and delivery

The engine defines two pure virtual hooks:

- `createControlPacketDeserializer(fixedHeader)`  
  Creates the correct deserializer for the incoming packet type.

- `deliverPacket(deserializer)`  
  Called once a full packet has been parsed, so role-specific code can act.

This allows:

- the base engine to own the read loop and parsing lifecycle,
- while client and server subclasses implement their specific packet semantics.

### 4.2 Publishing and QoS helpers

The base engine provides:

- `sendPublish(topic, message, qos, retain)`
- `sendPuback(packetId)`
- `sendPubrec(packetId)`
- `sendPubrel(packetId)`
- `sendPubcomp(packetId)`

and virtual hooks:

- `onPublish`, `onPuback`, `onPubrec`, `onPubrel`, `onPubcomp`

It also provides `_onPublish(...)` etc. which suggests a separation between:
- generic handling that updates internal state,
- and virtual hooks for user-defined behavior.

### 4.3 Keepalive handling

The engine has:

- `initSession(Session*, keepAlive)` which likely:
  - stores the keepalive value
  - arms the keepAliveTimer accordingly

Because SNode.C is event-driven, keepalive must not rely on blocking waits;
the timer integrates naturally with the core event loop.

---

## 5. Packet model (`FixedHeader`, `ControlPacket`, `packets/*`, `types/*`)

### 5.1 Fixed header

`FixedHeader` (`src/iot/mqtt/FixedHeader.h/.cpp`) represents MQTT’s fixed header:

- control packet type
- flags
- Remaining Length (variable-length integer)

This is the first stage of parsing for any incoming packet.

### 5.2 Control packet base + deserializer

- `ControlPacket` (`src/iot/mqtt/ControlPacket.h/.cpp`) provides a polymorphic base.
- `ControlPacketDeserializer` (`src/iot/mqtt/ControlPacketDeserializer.h/.cpp`) is the incremental parser.

Deserialization is important in an event-driven system:
packets may arrive fragmented across multiple reads, so a streaming parser is necessary.

### 5.3 Concrete packet types

Under `src/iot/mqtt/packets/*` you find classes for core MQTT packets:

- `Connect`, `Connack`
- `Publish`, and QoS companions: `Puback`, `Pubrec`, `Pubrel`, `Pubcomp`
- `Subscribe`, `Suback`
- `Unsubscribe`, `Unsuback`
- `Pingreq`, `Pingresp`
- `Disconnect`

The client/server subdirectories also contain specialized packet handling for those roles, reflecting differences in which packets are legal in which direction.

### 5.4 MQTT wire types

Under `src/iot/mqtt/types/*`, the module implements MQTT’s wire-level primitives:

- `UInt8`, `UInt16`, `UInt32`, `UIntV` (variable length)
- `String`, `StringRaw`
- `BinaryData`
- `StringPair`

These classes centralize the serialization/deserialization rules and reduce duplicate parsing code in packet classes.

---

## 6. Client implementation (`src/iot/mqtt/client/*`)

The client side provides:

- `iot::mqtt::client::Mqtt` — role-specific protocol engine
- `iot::mqtt::client::SubProtocol` — higher-level behavior and integration hooks
- `client/ControlPacketDeserializer` — a role-specific packet decoder

In SNode.C’s architecture, “subprotocol” often means:
a set of callbacks that define what to do with protocol events, similar to WebSocket subprotocols.

A typical client workflow:

1. Create a socket client (legacy or TLS, depending on transport binding in your app).
2. Install an MQTT socket context and a client `Mqtt` instance.
3. On connect:
   - send CONNECT
   - await CONNACK
4. Manage subscriptions/publishes
5. Keepalive timer triggers PINGREQ as needed
6. Handle disconnect/reconnect policy at the application layer

---

## 7. Server/broker implementation (`src/iot/mqtt/server/*`)

The server side includes:

- `iot::mqtt::server::Mqtt` — server role engine
- `server/SocketContextFactory` — creates MQTT contexts for incoming connections
- `server/SubProtocol` — hooks for server-side behavior
- `server/broker/*` — broker state and message distribution

### 7.1 Broker core: `iot::mqtt::server::broker::Broker`

The broker (`server/broker/Broker.h/.cpp`) manages:

- a `SubscriptionTree`
- a `RetainTree`
- a `sessionStore` mapping `clientId` → `Session`
- a `maxQoS` cap (ensuring publishes do not exceed broker policy)
- a `sessionStoreFileName` for persistence (session retention across restarts)

Key operations:

- `publish(originClientId, topic, message, qos, retain)`
- session lifecycle:
  - `newSession`, `renewSession`
  - `restartSession`, `retainSession`, `deleteSession`
  - `hasSession`, `hasActiveSession`, `hasRetainedSession`
- subscription management:
  - `subscribe(clientId, topic, qos)`
  - `unsubscribe(...)`
  - `getSubscriptions(clientId)`

### 7.2 SubscriptionTree: hierarchical topic matching

MQTT subscriptions include wildcards (`+`, `#`).
The `SubscriptionTree` provides efficient matching by storing subscriptions in a tree keyed by topic levels.

The broker can expose a structured view:

- `getSubscriptionTree()` returns a map suitable for introspection / UI display

This is useful for admin dashboards.

### 7.3 RetainTree: retained messages

Retained MQTT messages are stored per topic and delivered to new subscribers on subscribe.

The `RetainTree` manages:

- storing retained messages
- matching them against new subscriptions
- providing an inspection view via `getRetainTree()`

### 7.4 Sessions: active vs retained

Server-side `Session` objects (`server/broker/Session.*`) represent per-client state:

- active session: client currently connected
- retained session: client disconnected but state persisted (depending on clean session flag and broker policy)

Persistence is hinted by `sessionStoreFileName` and session lifecycle methods.

---

## 8. Example: conceptual broker usage

SNode.C’s MQTT broker is typically embedded in a server program:

- listen on an MQTT port (plain or TLS)
- accept connections and create MQTT socket contexts
- on packet delivery:
  - CONNECT → create/renew session
  - SUBSCRIBE → update subscription tree and send retained messages
  - PUBLISH → distribute to matching subscribers, store retain if needed

Because the broker runs in the event loop, it can handle many connections efficiently—provided handlers avoid blocking operations.

---

## 9. Pros / cons

### Pros
- Full event-driven design consistent with the rest of SNode.C.
- Clean separation between socket context and protocol engine.
- Broker features (subscriptions, retained messages, sessions) are built-in and introspectable.

### Cons / tradeoffs
- MQTT is stateful and has subtle QoS flows; thorough interoperability testing is important (especially QoS 1/2).
- Persistence semantics depend on session storage implementation; if you need “exactly-once” guarantees across crashes, you must verify the persistence model fits your requirements.

### Gotchas
- Keepalive must be armed correctly; a too-small keepalive will cause frequent pings and disconnects on poor links.
- Topic matching must respect MQTT wildcard rules; use the broker’s `SubscriptionTree` rather than ad-hoc matching.
- Don’t mix blocking storage I/O into the event loop when persisting sessions/messages; if session persistence writes synchronously, consider offloading or batching.

---

## 10. Next steps

- [`database.md`](database.md) — MariaDB integration for services that store MQTT data.
- [`core_socket.md`](core_socket.md) — deeper understanding of connection lifecycles and timeouts.
