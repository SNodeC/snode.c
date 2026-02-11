# WebSocket Module (`src/web/websocket/*`)

## Module role

This module implements HTTP upgrade-based WebSocket communication with a subprotocol-centric API for both client and server.

## Architectural pieces

- `SocketContextUpgrade` (generic + role-specific): upgrade transition and lifecycle bridge from HTTP context.
- `SubProtocol`: base contract for app protocols over WS frames.
- `SubProtocolFactory` + selector: load/choose concrete protocol implementations.
- `Receiver` / `Transmitter`: frame parsing and frame emission mechanics.
- server `GroupsManager`: group membership and broadcast coordination.

## Subprotocol model

SNode.C makes subprotocols first-class:

- each connection binds to a concrete subprotocol object,
- callbacks map to lifecycle and framing events,
- subprotocol can send text/binary frames and control frames via facade methods.

Important callbacks include:

- `onConnected`, `onDisconnected`, `onSignal`
- `onMessageStart`, `onMessageData`, `onMessageEnd`, `onMessageError`
- ping/pong handling via built-in keepalive behavior

## Reliability and keepalive

`SubProtocol` integrates timer-based ping logic:

- configurable ping interval,
- tracking of flying pings,
- drop/close behavior if pong progression fails.

This is particularly relevant for long-lived IoT/browser sessions over unstable networks.

## Server extras

`server/GroupsManager` provides a practical primitive for chat rooms, topic fanout, and channel-like architectures without forcing external state for simple use cases.

## Functionality summary

- end-to-end WS upgrade/runtime support
- client and server APIs with shared semantics
- robust frame receive/transmit split
- subprotocol plugin architecture
- ping/pong keepalive and group broadcast support

## Pros

1. **Subprotocol-first architecture** maps naturally to real-time app design.
2. **Strong integration with HTTP upgrade layer** keeps protocol transitions clean.
3. **Built-in keepalive mechanics** improve production resilience.
4. **Group management support** accelerates common realtime patterns.

## Cons / tradeoffs

1. **Requires understanding of multiple layers** (HTTP + WS + subprotocol factories).
2. **Custom subprotocol development** still needs careful frame-state handling discipline.
3. **Dynamic loading/factory selection** can complicate deployment diagnostics if misconfigured.
