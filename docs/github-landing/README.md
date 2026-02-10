# SNode.C GitHub Landing Documentation

This folder contains a modular deep-dive into SNode.C, organized for a GitHub landing page experience.

## Reading order

1. [`01-core-runtime.md`](./01-core-runtime.md) – runtime, event loop, timers, low-level utilities, and logging (**excluding `src/core/socket`**)  
2. [`02-core-socket.md`](./02-core-socket.md) – socket abstraction, stream stack, legacy vs TLS, and TLS handshake/shutdown internals  
3. [`03-net-and-config.md`](./03-net-and-config.md) – network specializations and the config model (CLI + config file + in-code API)  
4. [`04-web-http.md`](./04-web-http.md) – HTTP client/server architecture, upgrade hooks, parser/utilities, and EventSource (SSE)  
5. [`05-express.md`](./05-express.md) – high-level Express-like routing/middleware layer  
6. [`06-websocket.md`](./06-websocket.md) – WebSocket upgrade/runtime, subprotocol model, groups, framing, and keepalive behavior  
7. [`07-iot-mqtt.md`](./07-iot-mqtt.md) – MQTT packet/type/session stack and MQTT-over-WebSocket integration (**excluding `mqtt-fast`**)  
8. [`08-database.md`](./08-database.md) – MariaDB integration with async/sync command execution
9. [`09-extended-review-snodec-and-mqttsuite.md`](./09-extended-review-snodec-and-mqttsuite.md) – extended architectural review of SNode.C and the MQTTSuite ecosystem

## Scope notes

- This documentation is source-driven and follows module boundaries under `src/`.
- As requested, `src/core/socket` is documented separately from the rest of core.
- As requested, `src/iot/mqtt-fast` is intentionally not covered.

## Intended audience

- Developers evaluating SNode.C for embedded, IoT, edge, and backend service workloads.
- Teams planning to build custom protocol stacks on top of a single-threaded, event-driven C++ runtime.
- Contributors who want a practical map of subsystem responsibilities before diving into API docs.
