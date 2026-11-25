# Calculator peer

Build `jsonrpc-calculator`. It uses the same `Calculator` engine over every binding. Run with `--config-file=/dev/null --transport=stdio` (default), `--transport=tcp --port=7777`, `--transport=unix --path=calculator.sock`, or `--transport=websocket --port=7777` (upgrade `/ws`, subprotocol `jsonrpc`). Diagnostics go to stderr; stdio protocol messages go to stdout.

Send `{"jsonrpc":"2.0","id":1,"method":"add","params":[20,22]}` followed by a newline; the response has `result:42.0`. Stream transports also accept `--framing=content-length`, with `Content-Length: <byte count>\r\n\r\n` preceding each message. Against the TCP server, `--transport=client --port=7777` demonstrates a result, a Method not found error, and a timeout for the intentionally unanswered `wait` method. Overrides call the base lifecycle hook, and use inherited sending and correlation.
