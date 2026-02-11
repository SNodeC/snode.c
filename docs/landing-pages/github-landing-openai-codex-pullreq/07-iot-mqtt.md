# MQTT Module (`src/iot/mqtt/*`)

> Scope excludes `src/iot/mqtt-fast` by request.

## Module role

This module provides MQTT protocol support as a native SNode.C subsystem with both direct MQTT handling and MQTT-over-WebSocket integration.

## Major components

- `MqttContext`: transport abstraction contract for send/recv/end/close and connection callbacks.
- `ControlPacket` + packet classes (`Connect`, `Connack`, `Publish`, `Subscribe`, QoS control packets, etc.).
- `ControlPacketDeserializer`: incremental packet decoding logic.
- `types/*`: MQTT primitive encodings (`UIntV`, strings, binary data, etc.).
- `Session`: QoS state persistence for in-flight publish/pubrel/pubcomp tracking.
- `client/*` and `server/*`: role-specific adapters.
- `SubProtocol` bridge tying MQTT onto WebSocket subprotocol infrastructure.

## Protocol handling model

The module follows MQTT packet grammar closely:

- fixed header + variable-length encodings,
- typed packet classes for each control packet,
- serializer/deserializer pathways,
- session-aware QoS flow management.

This explicit packet-per-class design improves readability and protocol-level traceability.

## MQTT over WebSocket

A notable strength is direct composition with `web/websocket`:

- MQTT subprotocol classes derive through templated WS subprotocol roles,
- shared callback model (`onConnected`, `onReceivedFromPeer`, `onDisconnected`),
- reusable transport-agnostic MQTT core logic.

This supports browser-connected MQTT clients and mixed deployment topologies.

## Session semantics

`Session` tracks sender/receiver-side packet identifiers and retained in-flight packets relevant to QoS workflows, enabling recovery and protocol-correct behavior across message acknowledgment stages.

## Functionality summary

- MQTT packet/type implementation
- role-specific client/server adapters
- QoS/session state tracking
- native MQTT-over-WebSocket integration path

## Pros

1. **Protocol-explicit design** with clear class boundaries.
2. **Strong interoperability story** via WebSocket subprotocol bridge.
3. **State tracking abstractions** for QoS correctness.
4. **Good fit for IoT gateway use cases** where transport variants coexist.

## Cons / tradeoffs

1. **Many packet/type classes** increase code volume and learning curve.
2. **QoS/session correctness is inherently complex** and requires careful testing.
3. **Dual mode (native + WS) means broader surface for integration bugs if not validated with matrix tests.**

## Suggested landing-page message

Position SNode.C MQTT as **“an embeddable protocol stack with first-class WebSocket bridging for edge/cloud interoperability.”**
