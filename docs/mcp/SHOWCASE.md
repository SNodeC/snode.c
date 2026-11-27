# MCP feature tour

The separate `snodec-mcp-showcase-stdio` and `snodec-mcp-showcase-http` applications demonstrate every feature implemented by SNode.C's shared MCP values, server, HTTP server, client and HTTP client components. The development assistant's source-inspection tools, previews, resources and review prompt remain available. Additional examples operate on generated or process-local data. They do not execute commands or write source/configuration files. All registrations use concrete `Tool`, `Resource`, `ResourceTemplate` or `Prompt` subclasses. There is one registry/dispatcher and one client engine, shared across transports.

The [public API inventory](SHOWCASE-API.md) maps the installed entry points to the examples and qualification. The `sampling` and `roots` tools demonstrate [client-controlled sampling and root disclosure](CLIENT-INPUTS.md), including sampling tool-use results and an observable `listChanges` count for legacy roots notifications. A fresh showcase has 35 tools, 15 resources, 12 prompts and 12 resource templates. The tour uses a deterministic test model and an informational demonstration root; it does not contact an LLM provider or inspect that directory. Both features are deprecated but retained by the 2026-07-28 specification. Logging is legacy-only. Native harness support is separate from server/library support; a harness need not expose every protocol operation to an LLM.

## Build and run

Initialize submodules, install PCRE2, then use an external build directory as described in the [MCP README](README.md). These dedicated showcase executables leave the development assistant unchanged. The development assistant retains its original four tools, two resources, one prompt and one resource template on a single page. Set `BUILD_DIR` and `HEADER_ROOT` for your environment. `HEADER_ROOT` is the directory containing installed `core/`, `express/`, etc.

```sh
cmake -S . -B "$BUILD_DIR" -DSNODEC_BUILD_APPS=ON -DSNODEC_BUILD_TESTS=ON \
  -DSNODEC_CONTROL_BUILD_TESTS=ON
cmake --build "$BUILD_DIR"
```

The C++ and raw-wire qualification needs no SDK dependency. Optional SDK test setup is described in the [component test guide](../../tests/component/mcp/README.md); only its two stdio showcase cases are disabled by owner decision.

Run the client tour over stdio. The demonstration deadline is shortened to two seconds so that deadline enforcement is observable without a long wait. The application defaults to page size three so all four catalogues span several pages; `mcp --page-size` overrides it. The library default remains 128. The client owns process spawning and shutdown; no shell transport is involved.

```sh
snodec-mcp-client --config-file /dev/null --quiet client --command tour \
  --server snodec-mcp-showcase-stdio \
  --server-arguments '["--config-file","/dev/null","--quiet","mcp","--page-size","3","--deadline-ms","2000"]'
```

Add `client --legacy true` to select 2025-11-25. A tour without that option selects 2026-07-28; ordinary client commands still default to automatic discovery/fallback. Supply `--answer non-sensitive-label` and `--answer-action accept|decline|cancel` in the `client` section to select canned input. Responses are asynchronous, and independent simultaneous input requests retain independent responders. Outside the tour, elicitation is declined by default.

For HTTP, start the showcase server and run the client in a second terminal. Transport address options belong to the existing named instance's `remote` configuration, not application/root options.

```sh
snodec-mcp-showcase-http --config-file /dev/null \
  mcp-http local --host 127.0.0.1 --port 3337 \
  mcp --header-root "$HEADER_ROOT" --page-size 3 --deadline-ms 2000
snodec-mcp-client --config-file /dev/null --quiet \
  client --command tour \
  mcp-client-http remote --host 127.0.0.1 --port 3337
snodec-mcp-client --config-file /dev/null --quiet \
  client --command tour --legacy true \
  mcp-client-http remote --host 127.0.0.1 --port 3337
```

The same HTTP executable owns an independent `mcp-https` instance. Each named HTTP/TLS instance has its own registry, header root, limits, authorization and process-local resource/task state. It stays disabled until certificates are configured. Use your test certificate/key and trust its CA explicitly; do not disable verification. No key or certificate is shipped with the demonstration.

```sh
snodec-mcp-showcase-http --config-file /dev/null \
  mcp-http local --host 127.0.0.1 --port 3337 \
  mcp --header-root "$HEADER_ROOT" --page-size 3 --deadline-ms 2000 \
  mcp-https local --host 127.0.0.1 --port 3338 \
  tls --cert "$TEST_CERT" --cert-key "$TEST_KEY" \
  mcp --header-root "$HEADER_ROOT" --page-size 3 --deadline-ms 2000
snodec-mcp-client --config-file /dev/null --quiet \
  client --command tour --transport https \
  mcp-client-https remote --host 127.0.0.1 --port 3338 \
  tls --ca-cert "$TEST_CA"
```

Repeat with `client --legacy true` for legacy HTTPS. The component tests generate a fresh certificate and key at test time, use ephemeral ports and keep logs separate per test.

## Features and outcomes

Each client step prints its feature name, request summary and final result or expected error as JSON. A successful tour ends with `{"tour":"PASS",...}`. These examples deliberately include errors; an expected rejection is a passing tour step. Requests, notifications and exact raw envelopes are captured independently by the wire/SDK qualification.

| Feature | Registration / operation | Expected demonstration |
|---|---|---|
| Identity and negotiation | `server/discover` / `initialize` | Name, version, title, description, website, generated icon, instructions and era-specific capabilities |
| Four paginated inventories | Nine `catalogue.N` tools/resources/templates/prompts | Client follows opaque pages and returns full inventories; default library page size remains configurable |
| Text / errors | `text`, `tool.error`, `protocol.error` | Text result; `isError`; explicit JSON-RPC `-32602`, respectively |
| Output validation | `structured.valid`, `structured.invalid` | Valid structured answer 42; invalid output is rejected by the module |
| JSON Schema 2020-12 | `schema` | Local `$defs/$ref`, `oneOf`, enum/const, Unicode `\p{L}`, `unevaluatedProperties`; invalid inputs do not reach the handler |
| Header mirroring | `headers(region)` | Modern HTTP mirrors `x-mcp-header` and rejects mismatches; stdio/legacy use the same arguments |
| All content variants | `content` | Text, generated SVG image, generated silent WAV audio, resource link, embedded text and blob; icons/annotations/metadata |
| Principal | `principal` | Application-authorized subject for HTTP; anonymous without authorization |
| Progress and cancellation | `work(steps, delayMs)` | Three ordered progress messages; cancel and continue using the same connection |
| Deadlines | `work` with longer duration | Server deadline returns tool error; client timeout invokes its typed timeout hook |
| Tasks | `task.optional`, `task.required` | Legacy augmentation, get/result/list/cancel/status; modern extension handles, polling/get/update and task-id subscriptions |
| Input while a task runs | `input.task` | Required task enters input-required, accepts asynchronous input, then completes |
| Form / URL input | `input.form`, `input.url` | Legacy `elicitation/create` or modern `input_required`; accept, decline and cancel preserved |
| Continuations | `input.rounds`, `input.parallel` | Two rounds with opaque `requestState`; two independent inputs collected before continuation |
| Resource/prompt input | `showcase://input`, prompt `input` | The same typed input API resumes resource and prompt handlers |
| Text / blob resources | `showcase://text`, `showcase://blob` | Owned content and explicit modern public/private cache hints; legacy omits those hints |
| Updates | `showcase://counter`, `update` | Legacy subscribe/unsubscribe or modern acknowledged filtered listen with subscription metadata |
| Templates | `showcase://greeting/{name}`, `showcase://path/{+path}` | Scalar and reserved RFC 6570 expansion; completion with `context.arguments` |
| Prompts | `conversation(name)` | Completed argument, multiple messages and embedded resource |
| Dynamic registry | `dynamic(action=add|remove)` | Add/remove one item of all four kinds; coalesced list-changed notifications; subsequent inventories reflect the change |
| Logging | `logging` | Legacy `logging/setLevel`, request-context and server logging; modern logging is absent/rejected |
| HTTP policy | `mcp-http mcp` / `mcp-https mcp` | Optional authorizer, protected-resource metadata, 401/403 challenges, exact Origin policy |
| Legacy HTTP lifetime | Session ID, GET, DELETE, `Last-Event-ID` | Bounded replay for interrupted POST SSE and GET notification streams; modern never uses replay |
| Transport shutdown | EOF, disconnect, client stop | Shared stream framing/context and transport ownership |

The URL-input example is an acknowledgement of the public `https://example.com/` page, not a real authentication or consent workflow. Its state never grants access. The label examples validate accepted content against a server-owned schema and treat retries/state as untrusted input.

## HTTP authorization and replay

Authentication is disabled unless the instance's `mcp --demo-token` option is supplied. There is no default secret. This is an intentionally synthetic authorizer, not an OAuth server. With a configured token, the exact value grants access, `TOKEN:read` demonstrates insufficient scope, `TOKEN:forbidden` demonstrates forbidden, and other/missing values demonstrate unauthorized. Never reuse a real access token for this demonstration. Real applications validate issuer, audience/resource, expiry and scopes in their own `Authorize` callback.

```sh
snodec-mcp-showcase-http --config-file /dev/null \
  mcp-http local --host 127.0.0.1 --port 3337 \
  mcp --header-root "$HEADER_ROOT" --demo-token "$DEMO_TOKEN" \
  --resource-uri http://127.0.0.1:3337/mcp \
  --authorization-server https://authorization.example.com \
  --replay-events 128 --allowed-origin http://localhost:3000
MCP_BEARER_TOKEN="$DEMO_TOKEN" snodec-mcp-client --config-file /dev/null --quiet \
  client --command tour --legacy true \
  mcp-client-http remote --host 127.0.0.1 --port 3337
```

Metadata is available at the RFC 9728 well-known root and resource-path location. Challenges include `resource_metadata`; insufficient scope includes the Bearer error and required scope. Origin checking is independent. Replay is bounded by the existing router configuration; the demo exposes its event bound. The legacy HTTP tour test cuts a real SSE connection and verifies that the normal client reconnects using GET plus `Last-Event-ID`, without resubmitting POST. Modern reconnect/replay is deliberately not invented.

## Native harness configuration

Replace the executable placeholder with the installed path. These are examples for manual configuration; qualification uses per-invocation configuration and does not change user profiles.

Claude Code, top-level `mcpServers` in its user JSON (or a project `.mcp.json`):

```json
{
    "mcpServers": {
        "snodec-mcp-showcase-stdio": {
            "type": "stdio",
            "command": "<installed-bindir>/snodec-mcp-showcase-stdio",
            "args": ["--config-file", "/dev/null"]
        },
        "snodec-mcp-showcase-http": {
            "type": "http",
            "url": "http://127.0.0.1:3337/mcp"
        }
    }
}
```

Codex, `config.toml` MCP entries ([official MCP commands](https://learn.chatgpt.com/docs/developer-commands#codex-mcp)):

```toml
[mcp_servers.snodec-mcp-showcase-stdio]
command = "<installed-bindir>/snodec-mcp-showcase-stdio"
args = ["--config-file", "/dev/null"]

[mcp_servers.snodec-mcp-showcase-http]
url = "http://127.0.0.1:3337/mcp"
```

Ask the harness to call `text`, `schema`, `content`, read `showcase://text`, obtain `conversation` and invoke `input.form` with a non-sensitive label. Only the client's negotiated capabilities determine which input mechanism is used. Task polling, protocol logging, completion, subscriptions and replay need a client exposing the corresponding API; the typed C++ tour exercises them regardless of harness UI availability.

## Recorded wire exchanges

[SHOWCASE-WIRE.json](SHOWCASE-WIRE.json) contains selected complete JSON-RPC envelopes captured from the real C++ client tour, formatted with four-space indentation. It covers negotiation, pagination, content, errors, schemas, headers, progress, cancellation, deadlines, input, resources, prompts, completion, dynamic changes, tasks, logging and subscriptions in both eras. Unrelated messages between streaming events are omitted; envelopes themselves are not edited. Task identifiers/timestamps are expired synthetic test data.

The following two server envelopes show the input mechanisms. The associated calls, user answers and continuation requests are in the capture. The legacy form is an independent server request; the modern form completes the original call with `input_required`. These follow the [2025 elicitation specification](https://modelcontextprotocol.io/specification/2025-11-25/client/elicitation) and [2026 multi-round-trip pattern](https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/mrtr).

2025-11-25:

```json
{
    "id": "snodec.elicitation.1",
    "jsonrpc": "2.0",
    "method": "elicitation/create",
    "params": {
        "message": "Enter a non-sensitive label.",
        "mode": "form",
        "requestedSchema": {
            "properties": {
                "label": {
                    "maxLength": 64,
                    "minLength": 1,
                    "type": "string"
                }
            },
            "required": [
                "label"
            ],
            "type": "object"
        }
    }
}
```

2026-07-28:

```json
{
    "id": 92,
    "jsonrpc": "2.0",
    "result": {
        "_meta": {
            "io.modelcontextprotocol/serverInfo": {
                "description": "An executable tour of implemented MCP server and client features.",
                "icons": [
                    {
                        "mimeType": "image/svg+xml",
                        "sizes": [
                            "16x16"
                        ],
                        "src": "data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIxNiIgaGVpZ2h0PSIxNiI+PHJlY3Qgd2lkdGg9IjE2IiBoZWlnaHQ9IjE2IiBmaWxsPSIjMjQ3Ii8+PGNpcmNsZSBjeD0iOCIgY3k9IjgiIHI9IjUiIGZpbGw9IiNmZmYiLz48L3N2Zz4=",
                        "theme": "light"
                    }
                ],
                "name": "snodec-mcp-showcase",
                "title": "SNode.C MCP feature showcase",
                "version": "3.0.0",
                "websiteUrl": "https://github.com/SNodeC/snode.c"
            }
        },
        "inputRequests": {
            "first": {
                "method": "elicitation/create",
                "params": {
                    "message": "Enter a non-sensitive label.",
                    "mode": "form",
                    "requestedSchema": {
                        "properties": {
                            "label": {
                                "maxLength": 64,
                                "minLength": 1,
                                "type": "string"
                            }
                        },
                        "required": [
                            "label"
                        ],
                        "type": "object"
                    }
                }
            }
        },
        "requestState": "first",
        "resultType": "input_required"
    }
}
```

The raw HTTP tests also captured these response observations. The resource URL and issuer are reserved demonstration values; no access token is included. The challenge fields follow [MCP authorization](https://modelcontextprotocol.io/specification/2026-07-28/basic/authorization).

```json
[
    {
        "authorization": {
            "httpStatus": 401,
            "wwwAuthenticate": "Bearer realm=\"showcase\", resource_metadata=\"https://resource.example/.well-known/oauth-protected-resource/mcp\", scope=\"showcase:read\""
        }
    },
    {
        "authorization": {
            "httpStatus": 403,
            "wwwAuthenticate": "Bearer error=\"insufficient_scope\", scope=\"showcase:write\", resource_metadata=\"https://resource.example/.well-known/oauth-protected-resource/mcp\""
        }
    },
    {
        "authorization": {
            "httpStatus": 403,
            "wwwAuthenticate": ""
        }
    }
]
```

## Qualification

The six `McpShowcaseTour_ERA_TRANSPORT` tests exercise the typed C++ client in both eras over stdio/HTTP/HTTPS. The six matching `McpShowcaseWire_*` tests verify envelopes, metadata, invalid values, authorization, Origin and replay. `McpShowcaseSdk_*` uses the official TypeScript SDK in both eras over stdio/HTTP. The staged installed consumer compiles the same example classes and client against installed exported components and runs both stdio tours. All commands run sequentially, with `TMPDIR=/tmp/snc` and Python `-B`.

The owner has disabled only `McpShowcaseSdk_2025-11-25_stdio` and `McpShowcaseSdk_2026-07-28_stdio`; their assertions remain intact. The other four SDK registrations remain enabled when the existing SDK test dependency is configured. SDK 2.4.0 currently loses the last progress callback when stdio delivers the final progress notification and response together. The wire observer receives `[1,2,3]` in order before the result; only two callbacks reach application code. The SDK queues notification handling in a promise but removes progress handlers synchronously on response. The exact three-callback assertion remains enabled. See the [SDK protocol implementation](https://github.com/modelcontextprotocol/typescript-sdk/blob/v2.4.0/packages/core-internal/src/shared/protocol.ts). HTTP does not reproduce this delivery-batching issue. The SDK's tasks extension 0.2.3 tests both eras. Its documented modern adapter uses host-owned raw dispatch with disjoint string request ids and the extension's own request framing; the SDK and extension sources are unmodified. These are client limitations, not permission to weaken server assertions.

The [qualification record](QUALIFICATION.md) reports the toolchain results, test inventory, replay retirement regression and native/Inspector observations. The C++ tour and raw-wire assertions remain enabled across both eras and all three transports. Interactive cancellation remains a manual owner check; automated C++ tests verify protocol cancellation. Native controls that were not exposed remain **UNVERIFIED**.
