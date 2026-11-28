[Smoke test](#the-four-call-smoke-test) · [Feature notes](#what-each-feature-did) · [JSON exchanges](#json-exchange-archive) · [C++ classes](#c-class-cards)

LIVE CLIENT REPORT

# SNode.C MCP in Codex

Follow four local connections from the first greeting to form input, structured errors, resource updates and dynamic registration. Open any exchange to see the JSON in both directions and the C++ class behind it.

28 November 2026

- `snodec-dev-stdio` · stdio
- `snodec-dev-http` · HTTP
- `showcase-stdio` · stdio
- `showcase-http` · HTTP

- **4** local connections
- **82** exposed tools exercised
- **284** recorded exchanges
- **37** embedded C++ classes

## The four-call smoke test

Codex confirmed each native tool namespace before calling it. All four smoke calls passed.

| Server | Transport | Operation | Observed result | Status |
|---|---|---|---|---|
| `snodec-dev-stdio` | stdio | [`api.find`](#exchange-001) | SemanticLog.h, line 366, byte 18873 | PASS |
| `snodec-dev-http` | HTTP | [`api.find`](#exchange-002) | SemanticLog.h, line 366, byte 18873 | PASS |
| `showcase-stdio` | stdio | [`text`](#exchange-003) | Hello from the MCP showcase. | PASS |
| `showcase-http` | HTTP | [`text`](#exchange-004) | Hello from the MCP showcase. | PASS |

1. **Native Codex tool** · Names the configured connection and carries the test arguments.
2. **SNode.C registry** · Resolves the registered handler and validates applicable schemas.
3. **Class handler** · Reads, responds, requests input or returns an error.

## What each feature did

The example links open the recorded calls. The class links jump to the C++ listing farther down.

### A starting point that agrees on both transports

The first four smoke calls used separate namespaces. Both development calls located SemanticLog.h at line 366 and byte offset 18873. Both showcases returned “Hello from the MCP showcase.” The same greeting came back after the state tests.

[text](#exchange-003)

[`TextTool`](#texttool) · [`FindTool`](#findtool)

### From header search to exact bytes

The class query returned one bounded match on all four connections. A 48-byte read from that offset began with class SysErrorStream. The unique nonmatching query returned matches: [] and truncated: false. An empty query and an offset above 67108864 produced the registered input-validation errors.

[api.find](#exchange-001) · [api.read](#exchange-006)

[`FindTool`](#findtool) · [`ReadTool`](#readtool)

### Application and module previews

The application variant returned an example main.cpp and CMake target. The module variant returned router sources and their CMake files. Each preview arrived as a JSON files map. The expanded capture includes the complete maps for all four connections.

[skeleton.preview](#exchange-223)

[`SkeletonPreviewTool`](#skeletonpreviewtool)

### When an error is the expected result

The schema accepted Grüße with the alpha choice and é with integer choice 1. The crab emoji triggered the letters-only rule. The valid structured example contained answer: 42. The invalid one returned “Tool output failed its registered schema.” The deliberate tool failure used isError. The deliberate protocol failure was surfaced with JSON-RPC code -32602.

[schema](#exchange-038) · [structured.valid](#exchange-044) · [structured.invalid](#exchange-043) · [tool.error](#exchange-048) · [protocol.error](#exchange-031)

[`SchemaTool`](#schematool) · [`StructuredTool`](#structuredtool) · [`ToolErrorTool`](#toolerrortool) · [`ProtocolErrorTool`](#protocolerrortool)

### Beyond text

Codex received text, an SVG image, a resource link, embedded text and embedded binary content. The audio position contained Codex’s explicit unsupported-audio message. The captured result includes the media blocks that reached the client.

[content](#exchange-225)

[`ContentTool`](#contenttool)

### Request context and legacy ping

The region argument round-tripped as eu over stdio and HTTP. This demonstrates the tool result, with header transport outside the capture. Both principals were anonymous. The logging probe reported requestLogAdmitted true for stdio and false for HTTP. Client ping returned {} on both. The roots call reported an unsupported client capability.

[headers](#exchange-020) · [principal](#exchange-030) · [logging](#exchange-029) · [client.ping](#exchange-016) · [roots](#exchange-033)

[`HeadersTool`](#headerstool) · [`PrincipalTool`](#principaltool) · [`LoggingTool`](#loggingtool) · [`PingTool`](#pingtool) · [`RootsTool`](#rootstool)

### The form round trip

The first concurrent pass had several deadlines. Sequential retries completed review.label on all four connections. Both showcases accepted a single form, reached state second in the two-round form, and returned two independent accept responses in the parallel form. The optional URL acknowledgement returned completionNotified: true. The required URL case surfaced -32042. Task-originated input was rejected at task augmentation.

[review.label](#exchange-081) · [input.form](#exchange-068) · [input.rounds](#exchange-025) · [input.parallel](#exchange-023) · [input.url](#exchange-076) · [input.task](#exchange-026)

[`ReviewLabelTool`](#reviewlabeltool) · [`ReviewLabelResource`](#reviewlabelresource) · [`InputTool`](#inputtool)

### Reading resources, including URI templates

Discovery returned 2 resources and 1 template per development endpoint, and 15 resources and 12 templates per showcase endpoint. Reads returned the house style, selected labels, SNode.C text and AAEC binary bytes. The overview, greeting, reserved path and all nine catalogue template variants resolved. A resource-originated input request timed out on both showcases, including after a sequential retry.

[Resource list](#exchange-123) · [Template list](#exchange-124) · [Resource read](#exchange-233)

[`HouseStyleResource`](#housestyleresource) · [`ReviewLabelResource`](#reviewlabelresource) · [`StyleTemplate`](#styletemplate) · [`TextResourceItem`](#textresourceitem) · [`BlobResourceItem`](#blobresourceitem) · [`GreetingTemplate`](#greetingtemplate) · [`PathTemplate`](#pathtemplate) · [`CatalogueResource`](#catalogueresource) · [`CatalogueTemplate`](#cataloguetemplate) · [`InputResource`](#inputresource)

### A registration you can watch appear and disappear

Each catalogue tool from 0 through 8 returned its own name on both transports. Dynamic add exposed showcase://dynamic and showcase://dynamic/{value} in native discovery. The resource read returned dynamic. A callable dynamic.tool did not appear in the Codex tool inventory. Dynamic remove cleared both entries, and later reads returned -32002.

[catalogue.0](#exchange-007) · [catalogue.8](#exchange-015) · [dynamic](#exchange-195)

[`DynamicTool`](#dynamictool) · [`CatalogueTool`](#cataloguetool) · [`CatalogueResource`](#catalogueresource) · [`CatalogueTemplate`](#cataloguetemplate) · [`CataloguePrompt`](#catalogueprompt)

### One mutation with a visible result

On each showcase connection, the first resource read returned 0. update returned revision 1, and the next read returned 1. These three calls verify the mutation and the visible resource value. Delivery to a subscriber was outside the native operations exposed here.

[update](#exchange-203)

[`UpdateTool`](#updatetool) · [`CounterResource`](#counterresource)

### Work, deadlines and task negotiation

The short work call completed two steps. The long case returned “Request deadline exceeded.” An ordinary call to task.optional completed one step. task.required and input.task returned -32601 with “Tool task augmentation is forbidden or required.” The reverse client-task calls reported an unsupported input capability. Task lifecycle and progress events were unavailable at this boundary.

[work](#exchange-049) · [task.optional](#exchange-045) · [task.required](#exchange-046) · [client.task](#exchange-017)

[`WorkTool`](#worktool) · [`ClientTaskTool`](#clienttasktool) · [`InputTool`](#inputtool)

### Sampling requests reached the capability boundary

Text, image, audio and the tool-loop sampling variants all returned “Client does not support the requested input capability” on both showcases. The request shapes and identical responses appear in the archive.

[sampling](#exchange-034)

[`SamplingTool`](#samplingtool)

> Native prompt retrieval, completion operations, subscriptions, progress events and task lifecycle controls were unavailable in this run. The native tool results did not reveal the server versions or negotiated MCP protocol.

## JSON exchange archive

Requests and displayed responses are rendered with nlohmann::ordered_json::dump(4). JSON carried in a text field is expanded as a JSON value in the readable view. The downloadable archive preserves the original text and field types. Clipped JSON fragments link to the available complete capture.

> This is the Codex client boundary. Raw JSON-RPC frames, transport headers and reverse elicitation messages were outside the capture. The form results record the outcomes of the interactive tests.

[Download the JSON archive](codex-mcp-exchanges.json)

- [Initial smoke calls](#initial-smoke-calls)
- [Header search and source reads](#header-search-and-source-reads)
- [Skeleton previews](#skeleton-previews)
- [Content, schemas and errors](#content-schemas-and-errors)
- [Request context and client callbacks](#request-context-and-client-callbacks)
- [Interactive input and review labels](#interactive-input-and-review-labels)
- [Work and task support](#work-and-task-support)
- [Catalogue, dynamic registration and counter](#catalogue-dynamic-registration-and-counter)
- [Resource and template discovery and reads](#resource-and-template-discovery-and-reads)

### Initial smoke calls

#### Exchange 001

api.find · snodec-dev-stdio · Four-call smoke test · PASS

Four-call smoke test · Capture: native tool result

**Server classes** [`FindTool`](#findtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_stdio__api_find",
    "arguments": {
        "query": "class",
        "limit": 1
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "bytes": 20179,
                "entries": 4,
                "files": 1,
                "matches": [
                    {
                        "line": 366,
                        "offset": 18873,
                        "path": "SemanticLog.h"
                    }
                ],
                "truncated": true
            }
        }
    ],
    "structuredContent": {
        "bytes": 20179,
        "entries": 4,
        "files": 1,
        "matches": [
            {
                "line": 366,
                "offset": 18873,
                "path": "SemanticLog.h"
            }
        ],
        "truncated": true
    }
}
```

#### Exchange 002

api.find · snodec-dev-http · Four-call smoke test · PASS

Four-call smoke test · Capture: native tool result

**Server classes** [`FindTool`](#findtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_http__api_find",
    "arguments": {
        "query": "class",
        "limit": 1
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "bytes": 20179,
                "entries": 4,
                "files": 1,
                "matches": [
                    {
                        "line": 366,
                        "offset": 18873,
                        "path": "SemanticLog.h"
                    }
                ],
                "truncated": true
            }
        }
    ],
    "structuredContent": {
        "bytes": 20179,
        "entries": 4,
        "files": 1,
        "matches": [
            {
                "line": 366,
                "offset": 18873,
                "path": "SemanticLog.h"
            }
        ],
        "truncated": true
    }
}
```

#### Exchange 003

text · showcase-stdio · Four-call smoke test · PASS

Four-call smoke test · Capture: native tool result

**Server classes** [`TextTool`](#texttool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__text",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Hello from the MCP showcase."
        }
    ]
}
```

#### Exchange 004

text · showcase-http · Four-call smoke test · PASS

Four-call smoke test · Capture: native tool result

**Server classes** [`TextTool`](#texttool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__text",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Hello from the MCP showcase."
        }
    ]
}
```

### Header search and source reads

#### Exchange 005

api.find · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`FindTool`](#findtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__api_find",
    "arguments": {
        "query": "class",
        "limit": 1
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "bytes": 20179,
        "entries": 4,
        "files": 1,
        "matches": [
            {
                "line": 366,
                "offset": 18873,
                "path": "SemanticLog.h"
            }
        ],
        "truncated": true
    },
    "content": [
        {
            "type": "text",
            "text": {
                "bytes": 20179,
                "entries": 4,
                "files": 1,
                "matches": [
                    {
                        "line": 366,
                        "offset": 18873,
                        "path": "SemanticLog.h"
                    }
                ],
                "truncated": true
            }
        }
    ]
}
```

#### Exchange 006

api.read · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`ReadTool`](#readtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__api_read",
    "arguments": {
        "path": "SemanticLog.h",
        "offset": 18873,
        "bytes": 48
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "bytes": 48,
        "offset": 18873,
        "path": "SemanticLog.h",
        "text": "class SysErrorStream {\n    public:\n        SysEr",
        "truncated": true
    },
    "content": [
        {
            "type": "text",
            "text": {
                "bytes": 48,
                "offset": 18873,
                "path": "SemanticLog.h",
                "text": "class SysErrorStream {\n    public:\n        SysEr",
                "truncated": true
            }
        }
    ]
}
```

#### Exchange 051

api.find · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`FindTool`](#findtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__api_find",
    "arguments": {
        "query": "class",
        "limit": 1
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "bytes": 20179,
        "entries": 4,
        "files": 1,
        "matches": [
            {
                "line": 366,
                "offset": 18873,
                "path": "SemanticLog.h"
            }
        ],
        "truncated": true
    },
    "content": [
        {
            "type": "text",
            "text": {
                "bytes": 20179,
                "entries": 4,
                "files": 1,
                "matches": [
                    {
                        "line": 366,
                        "offset": 18873,
                        "path": "SemanticLog.h"
                    }
                ],
                "truncated": true
            }
        }
    ]
}
```

#### Exchange 052

api.read · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`ReadTool`](#readtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__api_read",
    "arguments": {
        "path": "SemanticLog.h",
        "offset": 18873,
        "bytes": 48
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "bytes": 48,
        "offset": 18873,
        "path": "SemanticLog.h",
        "text": "class SysErrorStream {\n    public:\n        SysEr",
        "truncated": true
    },
    "content": [
        {
            "type": "text",
            "text": {
                "bytes": 48,
                "offset": 18873,
                "path": "SemanticLog.h",
                "text": "class SysErrorStream {\n    public:\n        SysEr",
                "truncated": true
            }
        }
    ]
}
```

#### Exchange 100

api.find · snodec-dev-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`FindTool`](#findtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_http__api_find",
    "arguments": {
        "query": "class",
        "limit": 1
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "bytes": 20179,
        "entries": 4,
        "files": 1,
        "matches": [
            {
                "line": 366,
                "offset": 18873,
                "path": "SemanticLog.h"
            }
        ],
        "truncated": true
    },
    "content": [
        {
            "type": "text",
            "text": {
                "bytes": 20179,
                "entries": 4,
                "files": 1,
                "matches": [
                    {
                        "line": 366,
                        "offset": 18873,
                        "path": "SemanticLog.h"
                    }
                ],
                "truncated": true
            }
        }
    ]
}
```

#### Exchange 101

api.read · snodec-dev-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`ReadTool`](#readtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_http__api_read",
    "arguments": {
        "path": "SemanticLog.h",
        "offset": 18873,
        "bytes": 48
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "bytes": 48,
        "offset": 18873,
        "path": "SemanticLog.h",
        "text": "class SysErrorStream {\n    public:\n        SysEr",
        "truncated": true
    },
    "content": [
        {
            "type": "text",
            "text": {
                "bytes": 48,
                "offset": 18873,
                "path": "SemanticLog.h",
                "text": "class SysErrorStream {\n    public:\n        SysEr",
                "truncated": true
            }
        }
    ]
}
```

#### Exchange 105

api.find · snodec-dev-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`FindTool`](#findtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_stdio__api_find",
    "arguments": {
        "query": "class",
        "limit": 1
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "bytes": 20179,
        "entries": 4,
        "files": 1,
        "matches": [
            {
                "line": 366,
                "offset": 18873,
                "path": "SemanticLog.h"
            }
        ],
        "truncated": true
    },
    "content": [
        {
            "type": "text",
            "text": {
                "bytes": 20179,
                "entries": 4,
                "files": 1,
                "matches": [
                    {
                        "line": 366,
                        "offset": 18873,
                        "path": "SemanticLog.h"
                    }
                ],
                "truncated": true
            }
        }
    ]
}
```

#### Exchange 106

api.read · snodec-dev-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`ReadTool`](#readtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_stdio__api_read",
    "arguments": {
        "path": "SemanticLog.h",
        "offset": 18873,
        "bytes": 48
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "bytes": 48,
        "offset": 18873,
        "path": "SemanticLog.h",
        "text": "class SysErrorStream {\n    public:\n        SysEr",
        "truncated": true
    },
    "content": [
        {
            "type": "text",
            "text": {
                "bytes": 48,
                "offset": 18873,
                "path": "SemanticLog.h",
                "text": "class SysErrorStream {\n    public:\n        SysEr",
                "truncated": true
            }
        }
    ]
}
```

#### Exchange 111

api.find · snodec-dev-stdio · Search and offset boundaries · PASS

Search and offset boundaries · Capture: native tool result

**Server classes** [`FindTool`](#findtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_stdio__api_find",
    "arguments": {
        "query": "SNodeC_MCP_NATIVE_SCAN_NO_MATCH_4fc39f6c",
        "limit": 1
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "bytes": 2558330,
                "entries": 947,
                "files": 513,
                "matches": [],
                "truncated": false
            }
        }
    ],
    "structuredContent": {
        "bytes": 2558330,
        "entries": 947,
        "files": 513,
        "matches": [],
        "truncated": false
    }
}
```

#### Exchange 112

api.find · snodec-dev-stdio · Search and offset boundaries · EXPECTED ERROR

Search and offset boundaries · Capture: native tool result

**Server classes** [`FindTool`](#findtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_stdio__api_find",
    "arguments": {
        "query": "",
        "limit": 1
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "/query: instance is too short as per minLength:1"
        }
    ],
    "isError": true
}
```

#### Exchange 113

api.read · snodec-dev-stdio · Search and offset boundaries · EXPECTED ERROR

Search and offset boundaries · Capture: native tool result

**Server classes** [`ReadTool`](#readtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_stdio__api_read",
    "arguments": {
        "path": "SemanticLog.h",
        "offset": 99999999,
        "bytes": 32
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "/offset: instance exceeds maximum of 67108864"
        }
    ],
    "isError": true
}
```

#### Exchange 114

api.find · snodec-dev-http · Search and offset boundaries · PASS

Search and offset boundaries · Capture: native tool result

**Server classes** [`FindTool`](#findtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_http__api_find",
    "arguments": {
        "query": "SNodeC_MCP_NATIVE_SCAN_NO_MATCH_4fc39f6c",
        "limit": 1
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "bytes": 2558330,
                "entries": 947,
                "files": 513,
                "matches": [],
                "truncated": false
            }
        }
    ],
    "structuredContent": {
        "bytes": 2558330,
        "entries": 947,
        "files": 513,
        "matches": [],
        "truncated": false
    }
}
```

#### Exchange 115

api.find · snodec-dev-http · Search and offset boundaries · EXPECTED ERROR

Search and offset boundaries · Capture: native tool result

**Server classes** [`FindTool`](#findtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_http__api_find",
    "arguments": {
        "query": "",
        "limit": 1
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "/query: instance is too short as per minLength:1"
        }
    ],
    "isError": true
}
```

#### Exchange 116

api.read · snodec-dev-http · Search and offset boundaries · EXPECTED ERROR

Search and offset boundaries · Capture: native tool result

**Server classes** [`ReadTool`](#readtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_http__api_read",
    "arguments": {
        "path": "SemanticLog.h",
        "offset": 99999999,
        "bytes": 32
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "/offset: instance exceeds maximum of 67108864"
        }
    ],
    "isError": true
}
```

#### Exchange 117

api.find · showcase-stdio · Search and offset boundaries · PASS

Search and offset boundaries · Capture: native tool result

**Server classes** [`FindTool`](#findtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__api_find",
    "arguments": {
        "query": "SNodeC_MCP_NATIVE_SCAN_NO_MATCH_4fc39f6c",
        "limit": 1
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "bytes": 2558330,
                "entries": 947,
                "files": 513,
                "matches": [],
                "truncated": false
            }
        }
    ],
    "structuredContent": {
        "bytes": 2558330,
        "entries": 947,
        "files": 513,
        "matches": [],
        "truncated": false
    }
}
```

#### Exchange 118

api.find · showcase-stdio · Search and offset boundaries · EXPECTED ERROR

Search and offset boundaries · Capture: native tool result

**Server classes** [`FindTool`](#findtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__api_find",
    "arguments": {
        "query": "",
        "limit": 1
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "/query: instance is too short as per minLength:1"
        }
    ],
    "isError": true
}
```

#### Exchange 119

api.read · showcase-stdio · Search and offset boundaries · EXPECTED ERROR

Search and offset boundaries · Capture: native tool result

**Server classes** [`ReadTool`](#readtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__api_read",
    "arguments": {
        "path": "SemanticLog.h",
        "offset": 99999999,
        "bytes": 32
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "/offset: instance exceeds maximum of 67108864"
        }
    ],
    "isError": true
}
```

#### Exchange 120

api.find · showcase-http · Search and offset boundaries · PASS

Search and offset boundaries · Capture: native tool result

**Server classes** [`FindTool`](#findtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__api_find",
    "arguments": {
        "query": "SNodeC_MCP_NATIVE_SCAN_NO_MATCH_4fc39f6c",
        "limit": 1
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "bytes": 2558330,
                "entries": 947,
                "files": 513,
                "matches": [],
                "truncated": false
            }
        }
    ],
    "structuredContent": {
        "bytes": 2558330,
        "entries": 947,
        "files": 513,
        "matches": [],
        "truncated": false
    }
}
```

#### Exchange 121

api.find · showcase-http · Search and offset boundaries · EXPECTED ERROR

Search and offset boundaries · Capture: native tool result

**Server classes** [`FindTool`](#findtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__api_find",
    "arguments": {
        "query": "",
        "limit": 1
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "/query: instance is too short as per minLength:1"
        }
    ],
    "isError": true
}
```

#### Exchange 122

api.read · showcase-http · Search and offset boundaries · EXPECTED ERROR

Search and offset boundaries · Capture: native tool result

**Server classes** [`ReadTool`](#readtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__api_read",
    "arguments": {
        "path": "SemanticLog.h",
        "offset": 99999999,
        "bytes": 32
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "/offset: instance exceeds maximum of 67108864"
        }
    ],
    "isError": true
}
```

### Skeleton previews

#### Exchange 041

skeleton.preview · showcase-http · Expanded baseline calls · application · PASS

Expanded baseline calls · application · Capture: recorded tool projection

**Server classes** [`SkeletonPreviewTool`](#skeletonpreviewtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__skeleton_preview",
    "arguments": {
        "kind": "application"
    }
}
```

**MCP → Codex response · clipped preview**

```json
{
    "metadataKeys": [],
    "structured": {
        "captureNote": "The baseline capture clipped this JSON text. The archive retains the fragment.",
        "completeExchange": "exchange-223"
    },
    "content": [
        {
            "type": "text",
            "text": {
                "captureNote": "The baseline capture clipped this JSON text. The archive retains the fragment.",
                "completeExchange": "exchange-223"
            }
        }
    ]
}
```

See the [complete preview exchange](#exchange-223).

#### Exchange 042

skeleton.preview · showcase-http · Expanded baseline calls · module · PASS

Expanded baseline calls · module · Capture: recorded tool projection

**Server classes** [`SkeletonPreviewTool`](#skeletonpreviewtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__skeleton_preview",
    "arguments": {
        "kind": "module"
    }
}
```

**MCP → Codex response · clipped preview**

```json
{
    "metadataKeys": [],
    "structured": {
        "captureNote": "The baseline capture clipped this JSON text. The archive retains the fragment.",
        "completeExchange": "exchange-224"
    },
    "content": [
        {
            "type": "text",
            "text": {
                "captureNote": "The baseline capture clipped this JSON text. The archive retains the fragment.",
                "completeExchange": "exchange-224"
            }
        }
    ]
}
```

See the [complete preview exchange](#exchange-224).

#### Exchange 090

skeleton.preview · showcase-stdio · Expanded baseline calls · application · PASS

Expanded baseline calls · application · Capture: recorded tool projection

**Server classes** [`SkeletonPreviewTool`](#skeletonpreviewtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__skeleton_preview",
    "arguments": {
        "kind": "application"
    }
}
```

**MCP → Codex response · clipped preview**

```json
{
    "metadataKeys": [],
    "structured": {
        "captureNote": "The baseline capture clipped this JSON text. The archive retains the fragment.",
        "completeExchange": "exchange-226"
    },
    "content": [
        {
            "type": "text",
            "text": {
                "captureNote": "The baseline capture clipped this JSON text. The archive retains the fragment.",
                "completeExchange": "exchange-226"
            }
        }
    ]
}
```

See the [complete preview exchange](#exchange-226).

#### Exchange 091

skeleton.preview · showcase-stdio · Expanded baseline calls · module · PASS

Expanded baseline calls · module · Capture: recorded tool projection

**Server classes** [`SkeletonPreviewTool`](#skeletonpreviewtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__skeleton_preview",
    "arguments": {
        "kind": "module"
    }
}
```

**MCP → Codex response · clipped preview**

```json
{
    "metadataKeys": [],
    "structured": {
        "captureNote": "The baseline capture clipped this JSON text. The archive retains the fragment.",
        "completeExchange": "exchange-227"
    },
    "content": [
        {
            "type": "text",
            "text": {
                "captureNote": "The baseline capture clipped this JSON text. The archive retains the fragment.",
                "completeExchange": "exchange-227"
            }
        }
    ]
}
```

See the [complete preview exchange](#exchange-227).

#### Exchange 103

skeleton.preview · snodec-dev-http · Expanded baseline calls · application · PASS

Expanded baseline calls · application · Capture: recorded tool projection

**Server classes** [`SkeletonPreviewTool`](#skeletonpreviewtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_http__skeleton_preview",
    "arguments": {
        "kind": "application"
    }
}
```

**MCP → Codex response · clipped preview**

```json
{
    "metadataKeys": [],
    "structured": {
        "captureNote": "The baseline capture clipped this JSON text. The archive retains the fragment.",
        "completeExchange": "exchange-229"
    },
    "content": [
        {
            "type": "text",
            "text": {
                "captureNote": "The baseline capture clipped this JSON text. The archive retains the fragment.",
                "completeExchange": "exchange-229"
            }
        }
    ]
}
```

See the [complete preview exchange](#exchange-229).

#### Exchange 104

skeleton.preview · snodec-dev-http · Expanded baseline calls · module · PASS

Expanded baseline calls · module · Capture: recorded tool projection

**Server classes** [`SkeletonPreviewTool`](#skeletonpreviewtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_http__skeleton_preview",
    "arguments": {
        "kind": "module"
    }
}
```

**MCP → Codex response · clipped preview**

```json
{
    "metadataKeys": [],
    "structured": {
        "captureNote": "The baseline capture clipped this JSON text. The archive retains the fragment.",
        "completeExchange": "exchange-230"
    },
    "content": [
        {
            "type": "text",
            "text": {
                "captureNote": "The baseline capture clipped this JSON text. The archive retains the fragment.",
                "completeExchange": "exchange-230"
            }
        }
    ]
}
```

See the [complete preview exchange](#exchange-230).

#### Exchange 109

skeleton.preview · snodec-dev-stdio · Expanded baseline calls · application · PASS

Expanded baseline calls · application · Capture: recorded tool projection

**Server classes** [`SkeletonPreviewTool`](#skeletonpreviewtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_stdio__skeleton_preview",
    "arguments": {
        "kind": "application"
    }
}
```

**MCP → Codex response · clipped preview**

```json
{
    "metadataKeys": [],
    "structured": {
        "captureNote": "The baseline capture clipped this JSON text. The archive retains the fragment.",
        "completeExchange": "exchange-231"
    },
    "content": [
        {
            "type": "text",
            "text": {
                "captureNote": "The baseline capture clipped this JSON text. The archive retains the fragment.",
                "completeExchange": "exchange-231"
            }
        }
    ]
}
```

See the [complete preview exchange](#exchange-231).

#### Exchange 110

skeleton.preview · snodec-dev-stdio · Expanded baseline calls · module · PASS

Expanded baseline calls · module · Capture: recorded tool projection

**Server classes** [`SkeletonPreviewTool`](#skeletonpreviewtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_stdio__skeleton_preview",
    "arguments": {
        "kind": "module"
    }
}
```

**MCP → Codex response · clipped preview**

```json
{
    "metadataKeys": [],
    "structured": {
        "captureNote": "The baseline capture clipped this JSON text. The archive retains the fragment.",
        "completeExchange": "exchange-232"
    },
    "content": [
        {
            "type": "text",
            "text": {
                "captureNote": "The baseline capture clipped this JSON text. The archive retains the fragment.",
                "completeExchange": "exchange-232"
            }
        }
    ]
}
```

See the [complete preview exchange](#exchange-232).

#### Exchange 223

skeleton.preview · showcase-http · Read-only evidence capture · application · PASS

Read-only evidence capture · application · Capture: native tool result

**Server classes** [`SkeletonPreviewTool`](#skeletonpreviewtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__skeleton_preview",
    "arguments": {
        "kind": "application"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "files": {
                    "src/apps/example/CMakeLists.txt": "add_executable(example main.cpp)\ntarget_compile_features(example PRIVATE cxx_std_20)\ntarget_link_libraries(example PRIVATE http-server-express net-in-stream-legacy)\nset_target_properties(example PROPERTIES\n    INSTALL_RPATH \"${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}\")\ninstall(TARGETS example RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}\n    COMPONENT apps)\n",
                    "src/apps/example/main.cpp": "#include <express/legacy/in/WebApp.h>\n#include <express/Response.h>\n\nint main(int argc, char* argv[]) {\n    using WebApp = express::legacy::in::WebApp;\n    const WebApp app(\"example\");\n    WebApp::init(argc, argv);\n    app.get(\"/\", [](const auto&, const auto& response) {\n        response->send(\"SNode.C\");\n    });\n    app.listen([](const auto&, const auto&) {});\n    return WebApp::start();\n}\n"
                },
                "notes": "The apps directory discovers example/CMakeLists.txt; review before writing."
            }
        }
    ],
    "structuredContent": {
        "files": {
            "src/apps/example/CMakeLists.txt": "add_executable(example main.cpp)\ntarget_compile_features(example PRIVATE cxx_std_20)\ntarget_link_libraries(example PRIVATE http-server-express net-in-stream-legacy)\nset_target_properties(example PROPERTIES\n    INSTALL_RPATH \"${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}\")\ninstall(TARGETS example RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}\n    COMPONENT apps)\n",
            "src/apps/example/main.cpp": "#include <express/legacy/in/WebApp.h>\n#include <express/Response.h>\n\nint main(int argc, char* argv[]) {\n    using WebApp = express::legacy::in::WebApp;\n    const WebApp app(\"example\");\n    WebApp::init(argc, argv);\n    app.get(\"/\", [](const auto&, const auto& response) {\n        response->send(\"SNode.C\");\n    });\n    app.listen([](const auto&, const auto&) {});\n    return WebApp::start();\n}\n"
        },
        "notes": "The apps directory discovers example/CMakeLists.txt; review before writing."
    }
}
```

#### Exchange 224

skeleton.preview · showcase-http · Read-only evidence capture · module · PASS

Read-only evidence capture · module · Capture: native tool result

**Server classes** [`SkeletonPreviewTool`](#skeletonpreviewtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__skeleton_preview",
    "arguments": {
        "kind": "module"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "files": {
                    "src/example/CMakeLists.txt": "add_subdirectory(server)\n",
                    "src/example/server/CMakeLists.txt": "add_library(example-server SHARED Router.cpp Router.h)\nadd_library(snodec::example-server ALIAS example-server)\ntarget_compile_features(example-server PUBLIC cxx_std_20)\ntarget_link_libraries(example-server PUBLIC http-server-express)\ntarget_include_directories(example-server\n    PUBLIC \"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>\"\n           \"$<INSTALL_INTERFACE:include/snode.c>\")\nset_target_properties(example-server PROPERTIES\n    VERSION ${SNode.C_VERSION} SOVERSION ${SNODEC_SOVERSION})\ninstall(TARGETS example-server EXPORT snodec_example-server_Targets\n    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/example\n    COMPONENT example-server)\ninstall(FILES Router.h\n    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/example/server\n    COMPONENT example-server)\ninstall(EXPORT snodec_example-server_Targets\n    FILE snodec_example-server_Targets.cmake NAMESPACE snodec::\n    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec COMPONENT example-server)\n",
                    "src/example/server/Router.cpp": "#include \"example/server/Router.h\"\n#include <express/Response.h>\n\nnamespace example::server {\n    Router::Router() {\n        get(\"/\", [](const auto&, const auto& response) {\n            response->send(\"SNode.C\");\n        });\n    }\n}\n",
                    "src/example/server/Router.h": "#pragma once\n#include <express/Router.h>\n\nnamespace example::server {\n    class Router : public express::Router {\n    public:\n        Router();\n    };\n}\n"
                },
                "notes": "Add example to src/CMakeLists.txt and the central component/export inventory; review before writing."
            }
        }
    ],
    "structuredContent": {
        "files": {
            "src/example/CMakeLists.txt": "add_subdirectory(server)\n",
            "src/example/server/CMakeLists.txt": "add_library(example-server SHARED Router.cpp Router.h)\nadd_library(snodec::example-server ALIAS example-server)\ntarget_compile_features(example-server PUBLIC cxx_std_20)\ntarget_link_libraries(example-server PUBLIC http-server-express)\ntarget_include_directories(example-server\n    PUBLIC \"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>\"\n           \"$<INSTALL_INTERFACE:include/snode.c>\")\nset_target_properties(example-server PROPERTIES\n    VERSION ${SNode.C_VERSION} SOVERSION ${SNODEC_SOVERSION})\ninstall(TARGETS example-server EXPORT snodec_example-server_Targets\n    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/example\n    COMPONENT example-server)\ninstall(FILES Router.h\n    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/example/server\n    COMPONENT example-server)\ninstall(EXPORT snodec_example-server_Targets\n    FILE snodec_example-server_Targets.cmake NAMESPACE snodec::\n    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec COMPONENT example-server)\n",
            "src/example/server/Router.cpp": "#include \"example/server/Router.h\"\n#include <express/Response.h>\n\nnamespace example::server {\n    Router::Router() {\n        get(\"/\", [](const auto&, const auto& response) {\n            response->send(\"SNode.C\");\n        });\n    }\n}\n",
            "src/example/server/Router.h": "#pragma once\n#include <express/Router.h>\n\nnamespace example::server {\n    class Router : public express::Router {\n    public:\n        Router();\n    };\n}\n"
        },
        "notes": "Add example to src/CMakeLists.txt and the central component/export inventory; review before writing."
    }
}
```

#### Exchange 226

skeleton.preview · showcase-stdio · Read-only evidence capture · application · PASS

Read-only evidence capture · application · Capture: native tool result

**Server classes** [`SkeletonPreviewTool`](#skeletonpreviewtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__skeleton_preview",
    "arguments": {
        "kind": "application"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "files": {
                    "src/apps/example/CMakeLists.txt": "add_executable(example main.cpp)\ntarget_compile_features(example PRIVATE cxx_std_20)\ntarget_link_libraries(example PRIVATE http-server-express net-in-stream-legacy)\nset_target_properties(example PROPERTIES\n    INSTALL_RPATH \"${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}\")\ninstall(TARGETS example RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}\n    COMPONENT apps)\n",
                    "src/apps/example/main.cpp": "#include <express/legacy/in/WebApp.h>\n#include <express/Response.h>\n\nint main(int argc, char* argv[]) {\n    using WebApp = express::legacy::in::WebApp;\n    const WebApp app(\"example\");\n    WebApp::init(argc, argv);\n    app.get(\"/\", [](const auto&, const auto& response) {\n        response->send(\"SNode.C\");\n    });\n    app.listen([](const auto&, const auto&) {});\n    return WebApp::start();\n}\n"
                },
                "notes": "The apps directory discovers example/CMakeLists.txt; review before writing."
            }
        }
    ],
    "structuredContent": {
        "files": {
            "src/apps/example/CMakeLists.txt": "add_executable(example main.cpp)\ntarget_compile_features(example PRIVATE cxx_std_20)\ntarget_link_libraries(example PRIVATE http-server-express net-in-stream-legacy)\nset_target_properties(example PROPERTIES\n    INSTALL_RPATH \"${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}\")\ninstall(TARGETS example RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}\n    COMPONENT apps)\n",
            "src/apps/example/main.cpp": "#include <express/legacy/in/WebApp.h>\n#include <express/Response.h>\n\nint main(int argc, char* argv[]) {\n    using WebApp = express::legacy::in::WebApp;\n    const WebApp app(\"example\");\n    WebApp::init(argc, argv);\n    app.get(\"/\", [](const auto&, const auto& response) {\n        response->send(\"SNode.C\");\n    });\n    app.listen([](const auto&, const auto&) {});\n    return WebApp::start();\n}\n"
        },
        "notes": "The apps directory discovers example/CMakeLists.txt; review before writing."
    }
}
```

#### Exchange 227

skeleton.preview · showcase-stdio · Read-only evidence capture · module · PASS

Read-only evidence capture · module · Capture: native tool result

**Server classes** [`SkeletonPreviewTool`](#skeletonpreviewtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__skeleton_preview",
    "arguments": {
        "kind": "module"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "files": {
                    "src/example/CMakeLists.txt": "add_subdirectory(server)\n",
                    "src/example/server/CMakeLists.txt": "add_library(example-server SHARED Router.cpp Router.h)\nadd_library(snodec::example-server ALIAS example-server)\ntarget_compile_features(example-server PUBLIC cxx_std_20)\ntarget_link_libraries(example-server PUBLIC http-server-express)\ntarget_include_directories(example-server\n    PUBLIC \"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>\"\n           \"$<INSTALL_INTERFACE:include/snode.c>\")\nset_target_properties(example-server PROPERTIES\n    VERSION ${SNode.C_VERSION} SOVERSION ${SNODEC_SOVERSION})\ninstall(TARGETS example-server EXPORT snodec_example-server_Targets\n    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/example\n    COMPONENT example-server)\ninstall(FILES Router.h\n    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/example/server\n    COMPONENT example-server)\ninstall(EXPORT snodec_example-server_Targets\n    FILE snodec_example-server_Targets.cmake NAMESPACE snodec::\n    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec COMPONENT example-server)\n",
                    "src/example/server/Router.cpp": "#include \"example/server/Router.h\"\n#include <express/Response.h>\n\nnamespace example::server {\n    Router::Router() {\n        get(\"/\", [](const auto&, const auto& response) {\n            response->send(\"SNode.C\");\n        });\n    }\n}\n",
                    "src/example/server/Router.h": "#pragma once\n#include <express/Router.h>\n\nnamespace example::server {\n    class Router : public express::Router {\n    public:\n        Router();\n    };\n}\n"
                },
                "notes": "Add example to src/CMakeLists.txt and the central component/export inventory; review before writing."
            }
        }
    ],
    "structuredContent": {
        "files": {
            "src/example/CMakeLists.txt": "add_subdirectory(server)\n",
            "src/example/server/CMakeLists.txt": "add_library(example-server SHARED Router.cpp Router.h)\nadd_library(snodec::example-server ALIAS example-server)\ntarget_compile_features(example-server PUBLIC cxx_std_20)\ntarget_link_libraries(example-server PUBLIC http-server-express)\ntarget_include_directories(example-server\n    PUBLIC \"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>\"\n           \"$<INSTALL_INTERFACE:include/snode.c>\")\nset_target_properties(example-server PROPERTIES\n    VERSION ${SNode.C_VERSION} SOVERSION ${SNODEC_SOVERSION})\ninstall(TARGETS example-server EXPORT snodec_example-server_Targets\n    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/example\n    COMPONENT example-server)\ninstall(FILES Router.h\n    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/example/server\n    COMPONENT example-server)\ninstall(EXPORT snodec_example-server_Targets\n    FILE snodec_example-server_Targets.cmake NAMESPACE snodec::\n    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec COMPONENT example-server)\n",
            "src/example/server/Router.cpp": "#include \"example/server/Router.h\"\n#include <express/Response.h>\n\nnamespace example::server {\n    Router::Router() {\n        get(\"/\", [](const auto&, const auto& response) {\n            response->send(\"SNode.C\");\n        });\n    }\n}\n",
            "src/example/server/Router.h": "#pragma once\n#include <express/Router.h>\n\nnamespace example::server {\n    class Router : public express::Router {\n    public:\n        Router();\n    };\n}\n"
        },
        "notes": "Add example to src/CMakeLists.txt and the central component/export inventory; review before writing."
    }
}
```

#### Exchange 229

skeleton.preview · snodec-dev-http · Read-only evidence capture · application · PASS

Read-only evidence capture · application · Capture: native tool result

**Server classes** [`SkeletonPreviewTool`](#skeletonpreviewtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_http__skeleton_preview",
    "arguments": {
        "kind": "application"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "files": {
                    "src/apps/example/CMakeLists.txt": "add_executable(example main.cpp)\ntarget_compile_features(example PRIVATE cxx_std_20)\ntarget_link_libraries(example PRIVATE http-server-express net-in-stream-legacy)\nset_target_properties(example PROPERTIES\n    INSTALL_RPATH \"${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}\")\ninstall(TARGETS example RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}\n    COMPONENT apps)\n",
                    "src/apps/example/main.cpp": "#include <express/legacy/in/WebApp.h>\n#include <express/Response.h>\n\nint main(int argc, char* argv[]) {\n    using WebApp = express::legacy::in::WebApp;\n    const WebApp app(\"example\");\n    WebApp::init(argc, argv);\n    app.get(\"/\", [](const auto&, const auto& response) {\n        response->send(\"SNode.C\");\n    });\n    app.listen([](const auto&, const auto&) {});\n    return WebApp::start();\n}\n"
                },
                "notes": "The apps directory discovers example/CMakeLists.txt; review before writing."
            }
        }
    ],
    "structuredContent": {
        "files": {
            "src/apps/example/CMakeLists.txt": "add_executable(example main.cpp)\ntarget_compile_features(example PRIVATE cxx_std_20)\ntarget_link_libraries(example PRIVATE http-server-express net-in-stream-legacy)\nset_target_properties(example PROPERTIES\n    INSTALL_RPATH \"${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}\")\ninstall(TARGETS example RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}\n    COMPONENT apps)\n",
            "src/apps/example/main.cpp": "#include <express/legacy/in/WebApp.h>\n#include <express/Response.h>\n\nint main(int argc, char* argv[]) {\n    using WebApp = express::legacy::in::WebApp;\n    const WebApp app(\"example\");\n    WebApp::init(argc, argv);\n    app.get(\"/\", [](const auto&, const auto& response) {\n        response->send(\"SNode.C\");\n    });\n    app.listen([](const auto&, const auto&) {});\n    return WebApp::start();\n}\n"
        },
        "notes": "The apps directory discovers example/CMakeLists.txt; review before writing."
    }
}
```

#### Exchange 230

skeleton.preview · snodec-dev-http · Read-only evidence capture · module · PASS

Read-only evidence capture · module · Capture: native tool result

**Server classes** [`SkeletonPreviewTool`](#skeletonpreviewtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_http__skeleton_preview",
    "arguments": {
        "kind": "module"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "files": {
                    "src/example/CMakeLists.txt": "add_subdirectory(server)\n",
                    "src/example/server/CMakeLists.txt": "add_library(example-server SHARED Router.cpp Router.h)\nadd_library(snodec::example-server ALIAS example-server)\ntarget_compile_features(example-server PUBLIC cxx_std_20)\ntarget_link_libraries(example-server PUBLIC http-server-express)\ntarget_include_directories(example-server\n    PUBLIC \"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>\"\n           \"$<INSTALL_INTERFACE:include/snode.c>\")\nset_target_properties(example-server PROPERTIES\n    VERSION ${SNode.C_VERSION} SOVERSION ${SNODEC_SOVERSION})\ninstall(TARGETS example-server EXPORT snodec_example-server_Targets\n    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/example\n    COMPONENT example-server)\ninstall(FILES Router.h\n    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/example/server\n    COMPONENT example-server)\ninstall(EXPORT snodec_example-server_Targets\n    FILE snodec_example-server_Targets.cmake NAMESPACE snodec::\n    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec COMPONENT example-server)\n",
                    "src/example/server/Router.cpp": "#include \"example/server/Router.h\"\n#include <express/Response.h>\n\nnamespace example::server {\n    Router::Router() {\n        get(\"/\", [](const auto&, const auto& response) {\n            response->send(\"SNode.C\");\n        });\n    }\n}\n",
                    "src/example/server/Router.h": "#pragma once\n#include <express/Router.h>\n\nnamespace example::server {\n    class Router : public express::Router {\n    public:\n        Router();\n    };\n}\n"
                },
                "notes": "Add example to src/CMakeLists.txt and the central component/export inventory; review before writing."
            }
        }
    ],
    "structuredContent": {
        "files": {
            "src/example/CMakeLists.txt": "add_subdirectory(server)\n",
            "src/example/server/CMakeLists.txt": "add_library(example-server SHARED Router.cpp Router.h)\nadd_library(snodec::example-server ALIAS example-server)\ntarget_compile_features(example-server PUBLIC cxx_std_20)\ntarget_link_libraries(example-server PUBLIC http-server-express)\ntarget_include_directories(example-server\n    PUBLIC \"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>\"\n           \"$<INSTALL_INTERFACE:include/snode.c>\")\nset_target_properties(example-server PROPERTIES\n    VERSION ${SNode.C_VERSION} SOVERSION ${SNODEC_SOVERSION})\ninstall(TARGETS example-server EXPORT snodec_example-server_Targets\n    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/example\n    COMPONENT example-server)\ninstall(FILES Router.h\n    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/example/server\n    COMPONENT example-server)\ninstall(EXPORT snodec_example-server_Targets\n    FILE snodec_example-server_Targets.cmake NAMESPACE snodec::\n    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec COMPONENT example-server)\n",
            "src/example/server/Router.cpp": "#include \"example/server/Router.h\"\n#include <express/Response.h>\n\nnamespace example::server {\n    Router::Router() {\n        get(\"/\", [](const auto&, const auto& response) {\n            response->send(\"SNode.C\");\n        });\n    }\n}\n",
            "src/example/server/Router.h": "#pragma once\n#include <express/Router.h>\n\nnamespace example::server {\n    class Router : public express::Router {\n    public:\n        Router();\n    };\n}\n"
        },
        "notes": "Add example to src/CMakeLists.txt and the central component/export inventory; review before writing."
    }
}
```

#### Exchange 231

skeleton.preview · snodec-dev-stdio · Read-only evidence capture · application · PASS

Read-only evidence capture · application · Capture: native tool result

**Server classes** [`SkeletonPreviewTool`](#skeletonpreviewtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_stdio__skeleton_preview",
    "arguments": {
        "kind": "application"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "files": {
                    "src/apps/example/CMakeLists.txt": "add_executable(example main.cpp)\ntarget_compile_features(example PRIVATE cxx_std_20)\ntarget_link_libraries(example PRIVATE http-server-express net-in-stream-legacy)\nset_target_properties(example PROPERTIES\n    INSTALL_RPATH \"${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}\")\ninstall(TARGETS example RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}\n    COMPONENT apps)\n",
                    "src/apps/example/main.cpp": "#include <express/legacy/in/WebApp.h>\n#include <express/Response.h>\n\nint main(int argc, char* argv[]) {\n    using WebApp = express::legacy::in::WebApp;\n    const WebApp app(\"example\");\n    WebApp::init(argc, argv);\n    app.get(\"/\", [](const auto&, const auto& response) {\n        response->send(\"SNode.C\");\n    });\n    app.listen([](const auto&, const auto&) {});\n    return WebApp::start();\n}\n"
                },
                "notes": "The apps directory discovers example/CMakeLists.txt; review before writing."
            }
        }
    ],
    "structuredContent": {
        "files": {
            "src/apps/example/CMakeLists.txt": "add_executable(example main.cpp)\ntarget_compile_features(example PRIVATE cxx_std_20)\ntarget_link_libraries(example PRIVATE http-server-express net-in-stream-legacy)\nset_target_properties(example PROPERTIES\n    INSTALL_RPATH \"${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}\")\ninstall(TARGETS example RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}\n    COMPONENT apps)\n",
            "src/apps/example/main.cpp": "#include <express/legacy/in/WebApp.h>\n#include <express/Response.h>\n\nint main(int argc, char* argv[]) {\n    using WebApp = express::legacy::in::WebApp;\n    const WebApp app(\"example\");\n    WebApp::init(argc, argv);\n    app.get(\"/\", [](const auto&, const auto& response) {\n        response->send(\"SNode.C\");\n    });\n    app.listen([](const auto&, const auto&) {});\n    return WebApp::start();\n}\n"
        },
        "notes": "The apps directory discovers example/CMakeLists.txt; review before writing."
    }
}
```

#### Exchange 232

skeleton.preview · snodec-dev-stdio · Read-only evidence capture · module · PASS

Read-only evidence capture · module · Capture: native tool result

**Server classes** [`SkeletonPreviewTool`](#skeletonpreviewtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_stdio__skeleton_preview",
    "arguments": {
        "kind": "module"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "files": {
                    "src/example/CMakeLists.txt": "add_subdirectory(server)\n",
                    "src/example/server/CMakeLists.txt": "add_library(example-server SHARED Router.cpp Router.h)\nadd_library(snodec::example-server ALIAS example-server)\ntarget_compile_features(example-server PUBLIC cxx_std_20)\ntarget_link_libraries(example-server PUBLIC http-server-express)\ntarget_include_directories(example-server\n    PUBLIC \"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>\"\n           \"$<INSTALL_INTERFACE:include/snode.c>\")\nset_target_properties(example-server PROPERTIES\n    VERSION ${SNode.C_VERSION} SOVERSION ${SNODEC_SOVERSION})\ninstall(TARGETS example-server EXPORT snodec_example-server_Targets\n    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/example\n    COMPONENT example-server)\ninstall(FILES Router.h\n    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/example/server\n    COMPONENT example-server)\ninstall(EXPORT snodec_example-server_Targets\n    FILE snodec_example-server_Targets.cmake NAMESPACE snodec::\n    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec COMPONENT example-server)\n",
                    "src/example/server/Router.cpp": "#include \"example/server/Router.h\"\n#include <express/Response.h>\n\nnamespace example::server {\n    Router::Router() {\n        get(\"/\", [](const auto&, const auto& response) {\n            response->send(\"SNode.C\");\n        });\n    }\n}\n",
                    "src/example/server/Router.h": "#pragma once\n#include <express/Router.h>\n\nnamespace example::server {\n    class Router : public express::Router {\n    public:\n        Router();\n    };\n}\n"
                },
                "notes": "Add example to src/CMakeLists.txt and the central component/export inventory; review before writing."
            }
        }
    ],
    "structuredContent": {
        "files": {
            "src/example/CMakeLists.txt": "add_subdirectory(server)\n",
            "src/example/server/CMakeLists.txt": "add_library(example-server SHARED Router.cpp Router.h)\nadd_library(snodec::example-server ALIAS example-server)\ntarget_compile_features(example-server PUBLIC cxx_std_20)\ntarget_link_libraries(example-server PUBLIC http-server-express)\ntarget_include_directories(example-server\n    PUBLIC \"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>\"\n           \"$<INSTALL_INTERFACE:include/snode.c>\")\nset_target_properties(example-server PROPERTIES\n    VERSION ${SNode.C_VERSION} SOVERSION ${SNODEC_SOVERSION})\ninstall(TARGETS example-server EXPORT snodec_example-server_Targets\n    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/example\n    COMPONENT example-server)\ninstall(FILES Router.h\n    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/example/server\n    COMPONENT example-server)\ninstall(EXPORT snodec_example-server_Targets\n    FILE snodec_example-server_Targets.cmake NAMESPACE snodec::\n    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec COMPONENT example-server)\n",
            "src/example/server/Router.cpp": "#include \"example/server/Router.h\"\n#include <express/Response.h>\n\nnamespace example::server {\n    Router::Router() {\n        get(\"/\", [](const auto&, const auto& response) {\n            response->send(\"SNode.C\");\n        });\n    }\n}\n",
            "src/example/server/Router.h": "#pragma once\n#include <express/Router.h>\n\nnamespace example::server {\n    class Router : public express::Router {\n    public:\n        Router();\n    };\n}\n"
        },
        "notes": "Add example to src/CMakeLists.txt and the central component/export inventory; review before writing."
    }
}
```

### Content, schemas and errors

#### Exchange 019

content · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`ContentTool`](#contenttool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__content",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "Original generated media; one silent audio sample."
        },
        {
            "type": "image",
            "mimeType": "image/svg+xml",
            "dataLength": 128
        },
        {
            "type": "text",
            "text": "<audio content omitted because you do not support audio input>"
        },
        {
            "type": "resource_link",
            "mimeType": "text/plain",
            "uri": "showcase://text"
        },
        {
            "type": "resource",
            "resource": {
                "uri": "showcase://embedded/text",
                "mimeType": "text/plain"
            }
        },
        {
            "type": "resource",
            "resource": {
                "uri": "showcase://embedded/blob",
                "mimeType": "application/octet-stream"
            }
        }
    ]
}
```

#### Exchange 031

protocol.error · showcase-http · Expanded baseline calls · EXPECTED ERROR

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`ProtocolErrorTool`](#protocolerrortool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__protocol_error",
    "arguments": {}
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "isError": true,
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": {
                "message": "tool call error: tool call failed for `showcase-http/protocol.error`\n\nCaused by:\n    Mcp error: -32602: Deliberate protocol error.",
                "data": {
                    "feature": "errors"
                }
            }
        }
    ]
}
```

#### Exchange 038

schema · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`SchemaTool`](#schematool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__schema",
    "arguments": {
        "choice": "alpha",
        "word": "Grüße"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "choice": "alpha",
        "word": "Grüße"
    },
    "content": [
        {
            "type": "text",
            "text": {
                "choice": "alpha",
                "word": "Grüße"
            }
        }
    ]
}
```

#### Exchange 039

schema · showcase-http · Capability and schema variants · PASS

Capability and schema variants · Capture: native tool result

**Server classes** [`SchemaTool`](#schematool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__schema",
    "arguments": {
        "choice": 1,
        "word": "é"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "choice": 1,
                "word": "é"
            }
        }
    ],
    "structuredContent": {
        "choice": 1,
        "word": "é"
    }
}
```

#### Exchange 040

schema · showcase-http · Capability and schema variants · EXPECTED ERROR

Capability and schema variants · Capture: native tool result

**Server classes** [`SchemaTool`](#schematool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__schema",
    "arguments": {
        "choice": "alpha",
        "word": "Grüße 🦀"
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "/word: instance does not match regex pattern: ^\\p{L}+$"
        }
    ],
    "isError": true
}
```

#### Exchange 043

structured.invalid · showcase-http · Expanded baseline calls · EXPECTED ERROR

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`StructuredTool`](#structuredtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation) [Tool output validation](#tool-output-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__structured_invalid",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "isError": true,
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "Tool output failed its registered schema"
        }
    ]
}
```

#### Exchange 044

structured.valid · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`StructuredTool`](#structuredtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__structured_valid",
    "arguments": {}
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "answer": 42
    },
    "content": [
        {
            "type": "text",
            "text": {
                "answer": 42
            }
        }
    ]
}
```

#### Exchange 047

text · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`TextTool`](#texttool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__text",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "Hello from the MCP showcase."
        }
    ]
}
```

#### Exchange 048

tool.error · showcase-http · Expanded baseline calls · EXPECTED ERROR

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`ToolErrorTool`](#toolerrortool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__tool_error",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "isError": true,
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "Deliberate tool failure."
        }
    ]
}
```

#### Exchange 065

content · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`ContentTool`](#contenttool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__content",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "Original generated media; one silent audio sample."
        },
        {
            "type": "image",
            "mimeType": "image/svg+xml",
            "dataLength": 128
        },
        {
            "type": "text",
            "text": "<audio content omitted because you do not support audio input>"
        },
        {
            "type": "resource_link",
            "mimeType": "text/plain",
            "uri": "showcase://text"
        },
        {
            "type": "resource",
            "resource": {
                "uri": "showcase://embedded/text",
                "mimeType": "text/plain"
            }
        },
        {
            "type": "resource",
            "resource": {
                "uri": "showcase://embedded/blob",
                "mimeType": "application/octet-stream"
            }
        }
    ]
}
```

#### Exchange 079

protocol.error · showcase-stdio · Expanded baseline calls · EXPECTED ERROR

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`ProtocolErrorTool`](#protocolerrortool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__protocol_error",
    "arguments": {}
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "isError": true,
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": {
                "message": "tool call error: tool call failed for `showcase-stdio/protocol.error`\n\nCaused by:\n    Mcp error: -32602: Deliberate protocol error.",
                "data": {
                    "feature": "errors"
                }
            }
        }
    ]
}
```

#### Exchange 087

schema · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`SchemaTool`](#schematool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__schema",
    "arguments": {
        "choice": "alpha",
        "word": "Grüße"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "choice": "alpha",
        "word": "Grüße"
    },
    "content": [
        {
            "type": "text",
            "text": {
                "choice": "alpha",
                "word": "Grüße"
            }
        }
    ]
}
```

#### Exchange 088

schema · showcase-stdio · Capability and schema variants · PASS

Capability and schema variants · Capture: native tool result

**Server classes** [`SchemaTool`](#schematool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__schema",
    "arguments": {
        "choice": 1,
        "word": "é"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "choice": 1,
                "word": "é"
            }
        }
    ],
    "structuredContent": {
        "choice": 1,
        "word": "é"
    }
}
```

#### Exchange 089

schema · showcase-stdio · Capability and schema variants · EXPECTED ERROR

Capability and schema variants · Capture: native tool result

**Server classes** [`SchemaTool`](#schematool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__schema",
    "arguments": {
        "choice": "alpha",
        "word": "Grüße 🦀"
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "/word: instance does not match regex pattern: ^\\p{L}+$"
        }
    ],
    "isError": true
}
```

#### Exchange 092

structured.invalid · showcase-stdio · Expanded baseline calls · EXPECTED ERROR

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`StructuredTool`](#structuredtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation) [Tool output validation](#tool-output-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__structured_invalid",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "isError": true,
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "Tool output failed its registered schema"
        }
    ]
}
```

#### Exchange 093

structured.valid · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`StructuredTool`](#structuredtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__structured_valid",
    "arguments": {}
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "answer": 42
    },
    "content": [
        {
            "type": "text",
            "text": {
                "answer": 42
            }
        }
    ]
}
```

#### Exchange 096

text · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`TextTool`](#texttool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__text",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "Hello from the MCP showcase."
        }
    ]
}
```

#### Exchange 097

tool.error · showcase-stdio · Expanded baseline calls · EXPECTED ERROR

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`ToolErrorTool`](#toolerrortool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__tool_error",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "isError": true,
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "Deliberate tool failure."
        }
    ]
}
```

#### Exchange 217

text · showcase-stdio · Final native health check · PASS

Final native health check · Capture: native tool result

**Server classes** [`TextTool`](#texttool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__text",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Hello from the MCP showcase."
        }
    ]
}
```

#### Exchange 220

text · showcase-http · Final native health check · PASS

Final native health check · Capture: native tool result

**Server classes** [`TextTool`](#texttool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__text",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Hello from the MCP showcase."
        }
    ]
}
```

#### Exchange 225

content · showcase-http · Read-only evidence capture · PASS

Read-only evidence capture · Capture: native tool result

**Server classes** [`ContentTool`](#contenttool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__content",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Original generated media; one silent audio sample.",
            "annotations": {
                "audience": [
                    "user",
                    "assistant"
                ],
                "priority": 0.5,
                "lastModified": "2026-01-01T00:00:00Z"
            }
        },
        {
            "type": "image",
            "data": "PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIxIiBoZWlnaHQ9IjEiPjxyZWN0IHdpZHRoPSIxIiBoZWlnaHQ9IjEiLz48L3N2Zz4=",
            "mimeType": "image/svg+xml",
            "annotations": {
                "audience": [
                    "user",
                    "assistant"
                ],
                "priority": 0.5,
                "lastModified": "2026-01-01T00:00:00Z"
            }
        },
        {
            "type": "text",
            "text": "<audio content omitted because you do not support audio input>"
        },
        {
            "type": "resource_link",
            "uri": "showcase://text",
            "name": "Showcase text",
            "title": "Text",
            "description": "A readable resource",
            "mimeType": "text/plain",
            "size": 8,
            "icons": [
                {
                    "src": "data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIxNiIgaGVpZ2h0PSIxNiI+PHJlY3Qgd2lkdGg9IjE2IiBoZWlnaHQ9IjE2IiBmaWxsPSIjMjQ3Ii8+PGNpcmNsZSBjeD0iOCIgY3k9IjgiIHI9IjUiIGZpbGw9IiNmZmYiLz48L3N2Zz4=",
                    "mimeType": "image/svg+xml",
                    "sizes": [
                        "16x16"
                    ],
                    "theme": "light"
                }
            ],
            "annotations": {
                "audience": [
                    "user",
                    "assistant"
                ],
                "priority": 0.5,
                "lastModified": "2026-01-01T00:00:00Z"
            }
        },
        {
            "type": "resource",
            "resource": {
                "uri": "showcase://embedded/text",
                "mimeType": "text/plain",
                "text": "embedded"
            },
            "annotations": {
                "audience": [
                    "user",
                    "assistant"
                ],
                "priority": 0.5,
                "lastModified": "2026-01-01T00:00:00Z"
            }
        },
        {
            "type": "resource",
            "resource": {
                "uri": "showcase://embedded/blob",
                "mimeType": "application/octet-stream",
                "blob": "U05vZGUuQw=="
            },
            "annotations": {
                "audience": [
                    "user",
                    "assistant"
                ],
                "priority": 0.5,
                "lastModified": "2026-01-01T00:00:00Z"
            }
        }
    ]
}
```

#### Exchange 228

content · showcase-stdio · Read-only evidence capture · PASS

Read-only evidence capture · Capture: native tool result

**Server classes** [`ContentTool`](#contenttool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__content",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Original generated media; one silent audio sample.",
            "annotations": {
                "audience": [
                    "user",
                    "assistant"
                ],
                "priority": 0.5,
                "lastModified": "2026-01-01T00:00:00Z"
            }
        },
        {
            "type": "image",
            "data": "PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIxIiBoZWlnaHQ9IjEiPjxyZWN0IHdpZHRoPSIxIiBoZWlnaHQ9IjEiLz48L3N2Zz4=",
            "mimeType": "image/svg+xml",
            "annotations": {
                "audience": [
                    "user",
                    "assistant"
                ],
                "priority": 0.5,
                "lastModified": "2026-01-01T00:00:00Z"
            }
        },
        {
            "type": "text",
            "text": "<audio content omitted because you do not support audio input>"
        },
        {
            "type": "resource_link",
            "uri": "showcase://text",
            "name": "Showcase text",
            "title": "Text",
            "description": "A readable resource",
            "mimeType": "text/plain",
            "size": 8,
            "icons": [
                {
                    "src": "data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIxNiIgaGVpZ2h0PSIxNiI+PHJlY3Qgd2lkdGg9IjE2IiBoZWlnaHQ9IjE2IiBmaWxsPSIjMjQ3Ii8+PGNpcmNsZSBjeD0iOCIgY3k9IjgiIHI9IjUiIGZpbGw9IiNmZmYiLz48L3N2Zz4=",
                    "mimeType": "image/svg+xml",
                    "sizes": [
                        "16x16"
                    ],
                    "theme": "light"
                }
            ],
            "annotations": {
                "audience": [
                    "user",
                    "assistant"
                ],
                "priority": 0.5,
                "lastModified": "2026-01-01T00:00:00Z"
            }
        },
        {
            "type": "resource",
            "resource": {
                "uri": "showcase://embedded/text",
                "mimeType": "text/plain",
                "text": "embedded"
            },
            "annotations": {
                "audience": [
                    "user",
                    "assistant"
                ],
                "priority": 0.5,
                "lastModified": "2026-01-01T00:00:00Z"
            }
        },
        {
            "type": "resource",
            "resource": {
                "uri": "showcase://embedded/blob",
                "mimeType": "application/octet-stream",
                "blob": "U05vZGUuQw=="
            },
            "annotations": {
                "audience": [
                    "user",
                    "assistant"
                ],
                "priority": 0.5,
                "lastModified": "2026-01-01T00:00:00Z"
            }
        }
    ]
}
```

### Request context and client callbacks

#### Exchange 016

client.ping · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`PingTool`](#pingtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__client_ping",
    "arguments": {}
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {},
    "content": [
        {
            "type": "text",
            "text": {}
        }
    ]
}
```

#### Exchange 020

headers · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`HeadersTool`](#headerstool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__headers",
    "arguments": {
        "region": "eu"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "region": "eu"
    },
    "content": [
        {
            "type": "text",
            "text": {
                "region": "eu"
            }
        }
    ]
}
```

#### Exchange 029

logging · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`LoggingTool`](#loggingtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__logging",
    "arguments": {}
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "requestLogAdmitted": false
    },
    "content": [
        {
            "type": "text",
            "text": {
                "requestLogAdmitted": false
            }
        }
    ]
}
```

#### Exchange 030

principal · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`PrincipalTool`](#principaltool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__principal",
    "arguments": {}
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "subject": "anonymous"
    },
    "content": [
        {
            "type": "text",
            "text": {
                "subject": "anonymous"
            }
        }
    ]
}
```

#### Exchange 033

roots · showcase-http · Expanded baseline calls · CLIENT BLOCKED

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`RootsTool`](#rootstool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__roots",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "isError": true,
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "Client does not support the requested input capability"
        }
    ]
}
```

#### Exchange 034

sampling · showcase-http · Capability and schema variants · CLIENT BLOCKED

Capability and schema variants · Capture: native tool result

**Server classes** [`SamplingTool`](#samplingtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__sampling",
    "arguments": {
        "context": "none",
        "media": "text",
        "toolChoice": "none"
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Client does not support the requested input capability"
        }
    ],
    "isError": true
}
```

#### Exchange 035

sampling · showcase-http · Capability and schema variants · CLIENT BLOCKED

Capability and schema variants · Capture: native tool result

**Server classes** [`SamplingTool`](#samplingtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__sampling",
    "arguments": {
        "context": "none",
        "media": "image",
        "toolChoice": "none"
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Client does not support the requested input capability"
        }
    ],
    "isError": true
}
```

#### Exchange 036

sampling · showcase-http · Capability and schema variants · CLIENT BLOCKED

Capability and schema variants · Capture: native tool result

**Server classes** [`SamplingTool`](#samplingtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__sampling",
    "arguments": {
        "context": "none",
        "media": "audio",
        "toolChoice": "none"
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Client does not support the requested input capability"
        }
    ],
    "isError": true
}
```

#### Exchange 037

sampling · showcase-http · Capability and schema variants · CLIENT BLOCKED

Capability and schema variants · Capture: native tool result

**Server classes** [`SamplingTool`](#samplingtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__sampling",
    "arguments": {
        "context": "none",
        "media": "text",
        "toolChoice": "required",
        "toolUse": true,
        "toolLoop": true
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Client does not support the requested input capability"
        }
    ],
    "isError": true
}
```

#### Exchange 062

client.ping · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`PingTool`](#pingtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__client_ping",
    "arguments": {}
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {},
    "content": [
        {
            "type": "text",
            "text": {}
        }
    ]
}
```

#### Exchange 066

headers · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`HeadersTool`](#headerstool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__headers",
    "arguments": {
        "region": "eu"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "region": "eu"
    },
    "content": [
        {
            "type": "text",
            "text": {
                "region": "eu"
            }
        }
    ]
}
```

#### Exchange 077

logging · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`LoggingTool`](#loggingtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__logging",
    "arguments": {}
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "requestLogAdmitted": true
    },
    "content": [
        {
            "type": "text",
            "text": {
                "requestLogAdmitted": true
            }
        }
    ]
}
```

#### Exchange 078

principal · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`PrincipalTool`](#principaltool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__principal",
    "arguments": {}
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "subject": "anonymous"
    },
    "content": [
        {
            "type": "text",
            "text": {
                "subject": "anonymous"
            }
        }
    ]
}
```

#### Exchange 082

roots · showcase-stdio · Expanded baseline calls · CLIENT BLOCKED

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`RootsTool`](#rootstool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__roots",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "isError": true,
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "Client does not support the requested input capability"
        }
    ]
}
```

#### Exchange 083

sampling · showcase-stdio · Capability and schema variants · CLIENT BLOCKED

Capability and schema variants · Capture: native tool result

**Server classes** [`SamplingTool`](#samplingtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__sampling",
    "arguments": {
        "context": "none",
        "media": "text",
        "toolChoice": "none"
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Client does not support the requested input capability"
        }
    ],
    "isError": true
}
```

#### Exchange 084

sampling · showcase-stdio · Capability and schema variants · CLIENT BLOCKED

Capability and schema variants · Capture: native tool result

**Server classes** [`SamplingTool`](#samplingtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__sampling",
    "arguments": {
        "context": "none",
        "media": "image",
        "toolChoice": "none"
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Client does not support the requested input capability"
        }
    ],
    "isError": true
}
```

#### Exchange 085

sampling · showcase-stdio · Capability and schema variants · CLIENT BLOCKED

Capability and schema variants · Capture: native tool result

**Server classes** [`SamplingTool`](#samplingtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__sampling",
    "arguments": {
        "context": "none",
        "media": "audio",
        "toolChoice": "none"
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Client does not support the requested input capability"
        }
    ],
    "isError": true
}
```

#### Exchange 086

sampling · showcase-stdio · Capability and schema variants · CLIENT BLOCKED

Capability and schema variants · Capture: native tool result

**Server classes** [`SamplingTool`](#samplingtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__sampling",
    "arguments": {
        "context": "none",
        "media": "text",
        "toolChoice": "required",
        "toolUse": true,
        "toolLoop": true
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Client does not support the requested input capability"
        }
    ],
    "isError": true
}
```

### Interactive input and review labels

#### Exchange 021

input.form · showcase-http · First interactive pass · PASS

First interactive pass · Capture: native tool result

**Server classes** [`InputTool`](#inputtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__input_form",
    "arguments": {}
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "action": "accept",
                "label": "4",
                "state": "first"
            }
        }
    ],
    "structuredContent": {
        "action": "accept",
        "label": "4",
        "state": "first"
    }
}
```

#### Exchange 022

input.parallel · showcase-http · First interactive pass · TIMEOUT

First interactive pass · Capture: native tool result

**Server classes** [`InputTool`](#inputtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__input_parallel",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Request deadline exceeded"
        }
    ],
    "isError": true
}
```

#### Exchange 023

input.parallel · showcase-http · Sequential interactive retry · PASS

Sequential interactive retry · Capture: native tool result

**Server classes** [`InputTool`](#inputtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__input_parallel",
    "arguments": {}
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "first": {
                    "action": "accept",
                    "label": "4"
                },
                "second": {
                    "action": "accept",
                    "label": "4"
                }
            }
        }
    ],
    "structuredContent": {
        "first": {
            "action": "accept",
            "label": "4"
        },
        "second": {
            "action": "accept",
            "label": "4"
        }
    }
}
```

#### Exchange 024

input.rounds · showcase-http · First interactive pass · TIMEOUT

First interactive pass · Capture: native tool result

**Server classes** [`InputTool`](#inputtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__input_rounds",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Request deadline exceeded"
        }
    ],
    "isError": true
}
```

#### Exchange 025

input.rounds · showcase-http · Sequential interactive retry · PASS

Sequential interactive retry · Capture: native tool result

**Server classes** [`InputTool`](#inputtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__input_rounds",
    "arguments": {}
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "action": "accept",
                "label": "3",
                "state": "second"
            }
        }
    ],
    "structuredContent": {
        "action": "accept",
        "label": "3",
        "state": "second"
    }
}
```

#### Exchange 026

input.task · showcase-http · Capability and schema variants · CLIENT BLOCKED

Capability and schema variants · Capture: native tool result

**Server classes** [`InputTool`](#inputtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation) [Task augmentation validation](#task-augmentation-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__input_task",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "tool call error: tool call failed for `showcase-http/input.task`\n\nCaused by:\n    Mcp error: -32601: Tool task augmentation is forbidden or required"
        }
    ],
    "isError": true
}
```

#### Exchange 027

input.url · showcase-http · First interactive pass · EXPECTED ERROR

First interactive pass · Capture: native tool result

**Server classes** [`InputTool`](#inputtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__input_url",
    "arguments": {
        "required": true
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "message": "tool call error: tool call failed for `showcase-http/input.url`\n\nCaused by:\n    Mcp error: -32042: Complete the demonstration URL before retrying.",
                "data": {
                    "elicitations": [
                        {
                            "elicitationId": "96c5c374-a02f-d512-e6c7-4768d342460a",
                            "message": "Demonstration acknowledgement only.",
                            "mode": "url",
                            "url": "https://example.com/"
                        }
                    ]
                }
            }
        }
    ],
    "isError": true
}
```

#### Exchange 028

input.url · showcase-http · First interactive pass · PASS

First interactive pass · Capture: native tool result

**Server classes** [`InputTool`](#inputtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__input_url",
    "arguments": {
        "required": false
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "action": "accept",
                "completionNotified": true,
                "state": "first"
            }
        }
    ],
    "structuredContent": {
        "action": "accept",
        "completionNotified": true,
        "state": "first"
    }
}
```

#### Exchange 032

review.label · showcase-http · First interactive pass · PASS

First interactive pass · Capture: native tool result

**Server classes** [`ReviewLabelTool`](#reviewlabeltool) [`ReviewLabelResource`](#reviewlabelresource)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__review_label",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Review label selected."
        }
    ]
}
```

#### Exchange 067

input.form · showcase-stdio · First interactive pass · TIMEOUT

First interactive pass · Capture: native tool result

**Server classes** [`InputTool`](#inputtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__input_form",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Request deadline exceeded"
        }
    ],
    "isError": true
}
```

#### Exchange 068

input.form · showcase-stdio · Sequential interactive retry · PASS

Sequential interactive retry · Capture: native tool result

**Server classes** [`InputTool`](#inputtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__input_form",
    "arguments": {}
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "action": "accept",
                "label": "4",
                "state": "first"
            }
        }
    ],
    "structuredContent": {
        "action": "accept",
        "label": "4",
        "state": "first"
    }
}
```

#### Exchange 069

input.parallel · showcase-stdio · First interactive pass · TIMEOUT

First interactive pass · Capture: native tool result

**Server classes** [`InputTool`](#inputtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__input_parallel",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Request deadline exceeded"
        }
    ],
    "isError": true
}
```

#### Exchange 070

input.parallel · showcase-stdio · Sequential interactive retry · PASS

Sequential interactive retry · Capture: native tool result

**Server classes** [`InputTool`](#inputtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__input_parallel",
    "arguments": {}
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "first": {
                    "action": "accept",
                    "label": "4"
                },
                "second": {
                    "action": "accept",
                    "label": "5"
                }
            }
        }
    ],
    "structuredContent": {
        "first": {
            "action": "accept",
            "label": "4"
        },
        "second": {
            "action": "accept",
            "label": "5"
        }
    }
}
```

#### Exchange 071

input.rounds · showcase-stdio · First interactive pass · TIMEOUT

First interactive pass · Capture: native tool result

**Server classes** [`InputTool`](#inputtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__input_rounds",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Request deadline exceeded"
        }
    ],
    "isError": true
}
```

#### Exchange 072

input.rounds · showcase-stdio · Sequential interactive retry · PASS

Sequential interactive retry · Capture: native tool result

**Server classes** [`InputTool`](#inputtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__input_rounds",
    "arguments": {}
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "action": "accept",
                "label": "6",
                "state": "second"
            }
        }
    ],
    "structuredContent": {
        "action": "accept",
        "label": "6",
        "state": "second"
    }
}
```

#### Exchange 073

input.task · showcase-stdio · Capability and schema variants · CLIENT BLOCKED

Capability and schema variants · Capture: native tool result

**Server classes** [`InputTool`](#inputtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation) [Task augmentation validation](#task-augmentation-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__input_task",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "tool call error: tool call failed for `showcase-stdio/input.task`\n\nCaused by:\n    Mcp error: -32601: Tool task augmentation is forbidden or required"
        }
    ],
    "isError": true
}
```

#### Exchange 074

input.url · showcase-stdio · First interactive pass · EXPECTED ERROR

First interactive pass · Capture: native tool result

**Server classes** [`InputTool`](#inputtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__input_url",
    "arguments": {
        "required": true
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "message": "tool call error: tool call failed for `showcase-stdio/input.url`\n\nCaused by:\n    Mcp error: -32042: Complete the demonstration URL before retrying.",
                "data": {
                    "elicitations": [
                        {
                            "elicitationId": "027f5b9a-ad51-5959-45a7-c5a4b6497aed",
                            "message": "Demonstration acknowledgement only.",
                            "mode": "url",
                            "url": "https://example.com/"
                        }
                    ]
                }
            }
        }
    ],
    "isError": true
}
```

#### Exchange 075

input.url · showcase-stdio · First interactive pass · TIMEOUT

First interactive pass · Capture: native tool result

**Server classes** [`InputTool`](#inputtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__input_url",
    "arguments": {
        "required": false
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Request deadline exceeded"
        }
    ],
    "isError": true
}
```

#### Exchange 076

input.url · showcase-stdio · Sequential interactive retry · PASS

Sequential interactive retry · Capture: native tool result

**Server classes** [`InputTool`](#inputtool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__input_url",
    "arguments": {
        "required": false
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "action": "accept",
                "completionNotified": true,
                "state": "first"
            }
        }
    ],
    "structuredContent": {
        "action": "accept",
        "completionNotified": true,
        "state": "first"
    }
}
```

#### Exchange 080

review.label · showcase-stdio · First interactive pass · TIMEOUT

First interactive pass · Capture: native tool result

**Server classes** [`ReviewLabelTool`](#reviewlabeltool) [`ReviewLabelResource`](#reviewlabelresource)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__review_label",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Request deadline exceeded"
        }
    ],
    "isError": true
}
```

#### Exchange 081

review.label · showcase-stdio · Sequential interactive retry · PASS

Sequential interactive retry · Capture: native tool result

**Server classes** [`ReviewLabelTool`](#reviewlabeltool) [`ReviewLabelResource`](#reviewlabelresource)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__review_label",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Review label selected."
        }
    ]
}
```

#### Exchange 102

review.label · snodec-dev-http · First interactive pass · PASS

First interactive pass · Capture: native tool result

**Server classes** [`ReviewLabelTool`](#reviewlabeltool) [`ReviewLabelResource`](#reviewlabelresource)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_http__review_label",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Review label selected."
        }
    ]
}
```

#### Exchange 107

review.label · snodec-dev-stdio · First interactive pass · TIMEOUT

First interactive pass · Capture: native tool result

**Server classes** [`ReviewLabelTool`](#reviewlabeltool) [`ReviewLabelResource`](#reviewlabelresource)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_stdio__review_label",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Request deadline exceeded"
        }
    ],
    "isError": true
}
```

#### Exchange 108

review.label · snodec-dev-stdio · Sequential interactive retry · PASS

Sequential interactive retry · Capture: native tool result

**Server classes** [`ReviewLabelTool`](#reviewlabeltool) [`ReviewLabelResource`](#reviewlabelresource)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__snodec_dev_stdio__review_label",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Review label selected."
        }
    ]
}
```

#### Exchange 135

Resource read · showcase-http · Resource and template reads · showcase://input · TIMEOUT

Resource and template reads · showcase://input · Capture: native exception

**Server classes** [`InputResource`](#inputresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://input"
    }
}
```

**MCP → Codex response**

```json
{
    "error": "resources/read failed: resources/read failed for `showcase-http` (showcase://input): Mcp error: -32603: Request deadline exceeded"
}
```

#### Exchange 136

Resource read · showcase-http · Sequential resource elicitation retry · showcase://input · TIMEOUT

Sequential resource elicitation retry · showcase://input · Capture: native exception

**Server classes** [`InputResource`](#inputresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://input"
    }
}
```

**MCP → Codex response**

```json
{
    "error": "resources/read failed: resources/read failed for `showcase-http` (showcase://input): Mcp error: -32603: Request deadline exceeded"
}
```

#### Exchange 139

Resource read · showcase-http · Resource and template reads · snodec://development/review-label · PASS

Resource and template reads · snodec://development/review-label · Capture: recorded resource projection

**Server classes** [`ReviewLabelResource`](#reviewlabelresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "snodec://development/review-label"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "snodec://development/review-label",
            "mimeType": "text/plain",
            "text": ""
        }
    ]
}
```

#### Exchange 140

Resource read · showcase-http · Style and selected-label verification · snodec://development/review-label · PASS

Style and selected-label verification · snodec://development/review-label · Capture: recorded resource projection

**Server classes** [`ReviewLabelResource`](#reviewlabelresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "snodec://development/review-label"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "snodec://development/review-label",
            "text": "34"
        }
    ]
}
```

#### Exchange 164

Resource read · showcase-stdio · Resource and template reads · showcase://input · TIMEOUT

Resource and template reads · showcase://input · Capture: native exception

**Server classes** [`InputResource`](#inputresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://input"
    }
}
```

**MCP → Codex response**

```json
{
    "error": "resources/read failed: resources/read failed for `showcase-stdio` (showcase://input): Mcp error: -32603: Request deadline exceeded"
}
```

#### Exchange 165

Resource read · showcase-stdio · Sequential resource elicitation retry · showcase://input · TIMEOUT

Sequential resource elicitation retry · showcase://input · Capture: native exception

**Server classes** [`InputResource`](#inputresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://input"
    }
}
```

**MCP → Codex response**

```json
{
    "error": "resources/read failed: resources/read failed for `showcase-stdio` (showcase://input): Mcp error: -32603: Request deadline exceeded"
}
```

#### Exchange 168

Resource read · showcase-stdio · Resource and template reads · snodec://development/review-label · PASS

Resource and template reads · snodec://development/review-label · Capture: recorded resource projection

**Server classes** [`ReviewLabelResource`](#reviewlabelresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "snodec://development/review-label"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "snodec://development/review-label",
            "mimeType": "text/plain",
            "text": ""
        }
    ]
}
```

#### Exchange 169

Resource read · showcase-stdio · Style and selected-label verification · snodec://development/review-label · PASS

Style and selected-label verification · snodec://development/review-label · Capture: recorded resource projection

**Server classes** [`ReviewLabelResource`](#reviewlabelresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "snodec://development/review-label"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "snodec://development/review-label",
            "text": ""
        }
    ]
}
```

#### Exchange 184

Resource read · snodec-dev-http · Resource and template reads · snodec://development/review-label · PASS

Resource and template reads · snodec://development/review-label · Capture: recorded resource projection

**Server classes** [`ReviewLabelResource`](#reviewlabelresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "snodec-dev-http",
        "uri": "snodec://development/review-label"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "snodec://development/review-label",
            "mimeType": "text/plain",
            "text": ""
        }
    ]
}
```

#### Exchange 185

Resource read · snodec-dev-http · Style and selected-label verification · snodec://development/review-label · PASS

Style and selected-label verification · snodec://development/review-label · Capture: recorded resource projection

**Server classes** [`ReviewLabelResource`](#reviewlabelresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "snodec-dev-http",
        "uri": "snodec://development/review-label"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "snodec://development/review-label",
            "text": "4"
        }
    ]
}
```

#### Exchange 189

Resource read · snodec-dev-stdio · Resource and template reads · snodec://development/review-label · PASS

Resource and template reads · snodec://development/review-label · Capture: recorded resource projection

**Server classes** [`ReviewLabelResource`](#reviewlabelresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "snodec-dev-stdio",
        "uri": "snodec://development/review-label"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "snodec://development/review-label",
            "mimeType": "text/plain",
            "text": ""
        }
    ]
}
```

#### Exchange 190

Resource read · snodec-dev-stdio · Style and selected-label verification · snodec://development/review-label · PASS

Style and selected-label verification · snodec://development/review-label · Capture: recorded resource projection

**Server classes** [`ReviewLabelResource`](#reviewlabelresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "snodec-dev-stdio",
        "uri": "snodec://development/review-label"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "snodec://development/review-label",
            "text": ""
        }
    ]
}
```

### Work and task support

#### Exchange 017

client.task · showcase-http · Capability and schema variants · sampling · CLIENT BLOCKED

Capability and schema variants · sampling · Capture: native tool result

**Server classes** [`ClientTaskTool`](#clienttasktool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__client_task",
    "arguments": {
        "kind": "sampling",
        "cancel": true
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Client does not support the requested input capability"
        }
    ],
    "isError": true
}
```

#### Exchange 018

client.task · showcase-http · Capability and schema variants · elicitation · CLIENT BLOCKED

Capability and schema variants · elicitation · Capture: native tool result

**Server classes** [`ClientTaskTool`](#clienttasktool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__client_task",
    "arguments": {
        "kind": "elicitation",
        "cancel": true
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Client does not support the requested input capability"
        }
    ],
    "isError": true
}
```

#### Exchange 045

task.optional · showcase-http · Capability and schema variants · PASS

Capability and schema variants · Capture: native tool result

**Server classes** [`WorkTool`](#worktool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation) [Task augmentation validation](#task-augmentation-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__task_optional",
    "arguments": {
        "steps": 1,
        "delayMs": 10
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "completed": 1
            }
        }
    ],
    "structuredContent": {
        "completed": 1
    }
}
```

#### Exchange 046

task.required · showcase-http · Capability and schema variants · CLIENT BLOCKED

Capability and schema variants · Capture: native tool result

**Server classes** [`WorkTool`](#worktool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation) [Task augmentation validation](#task-augmentation-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__task_required",
    "arguments": {
        "steps": 1,
        "delayMs": 10
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "tool call error: tool call failed for `showcase-http/task.required`\n\nCaused by:\n    Mcp error: -32601: Tool task augmentation is forbidden or required"
        }
    ],
    "isError": true
}
```

#### Exchange 049

work · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`WorkTool`](#worktool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__work",
    "arguments": {
        "delayMs": 20,
        "steps": 2
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "completed": 2
    },
    "content": [
        {
            "type": "text",
            "text": {
                "completed": 2
            }
        }
    ]
}
```

#### Exchange 050

work · showcase-http · First interactive pass · EXPECTED ERROR

First interactive pass · Capture: native tool result

**Server classes** [`WorkTool`](#worktool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__work",
    "arguments": {
        "steps": 20,
        "delayMs": 1000
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Request deadline exceeded"
        }
    ],
    "isError": true
}
```

#### Exchange 063

client.task · showcase-stdio · Capability and schema variants · sampling · CLIENT BLOCKED

Capability and schema variants · sampling · Capture: native tool result

**Server classes** [`ClientTaskTool`](#clienttasktool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__client_task",
    "arguments": {
        "kind": "sampling",
        "cancel": true
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Client does not support the requested input capability"
        }
    ],
    "isError": true
}
```

#### Exchange 064

client.task · showcase-stdio · Capability and schema variants · elicitation · CLIENT BLOCKED

Capability and schema variants · elicitation · Capture: native tool result

**Server classes** [`ClientTaskTool`](#clienttasktool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__client_task",
    "arguments": {
        "kind": "elicitation",
        "cancel": true
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Client does not support the requested input capability"
        }
    ],
    "isError": true
}
```

#### Exchange 094

task.optional · showcase-stdio · Capability and schema variants · PASS

Capability and schema variants · Capture: native tool result

**Server classes** [`WorkTool`](#worktool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation) [Task augmentation validation](#task-augmentation-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__task_optional",
    "arguments": {
        "steps": 1,
        "delayMs": 10
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "completed": 1
            }
        }
    ],
    "structuredContent": {
        "completed": 1
    }
}
```

#### Exchange 095

task.required · showcase-stdio · Capability and schema variants · CLIENT BLOCKED

Capability and schema variants · Capture: native tool result

**Server classes** [`WorkTool`](#worktool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation) [Task augmentation validation](#task-augmentation-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__task_required",
    "arguments": {
        "steps": 1,
        "delayMs": 10
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "tool call error: tool call failed for `showcase-stdio/task.required`\n\nCaused by:\n    Mcp error: -32601: Tool task augmentation is forbidden or required"
        }
    ],
    "isError": true
}
```

#### Exchange 098

work · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`WorkTool`](#worktool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__work",
    "arguments": {
        "delayMs": 20,
        "steps": 2
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "metadataKeys": [],
    "structured": {
        "completed": 2
    },
    "content": [
        {
            "type": "text",
            "text": {
                "completed": 2
            }
        }
    ]
}
```

#### Exchange 099

work · showcase-stdio · First interactive pass · EXPECTED ERROR

First interactive pass · Capture: native tool result

**Server classes** [`WorkTool`](#worktool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__work",
    "arguments": {
        "steps": 20,
        "delayMs": 1000
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "Request deadline exceeded"
        }
    ],
    "isError": true
}
```

### Catalogue, dynamic registration and counter

#### Exchange 007

catalogue.0 · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`CatalogueTool`](#cataloguetool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__catalogue_0",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "catalogue.0"
        }
    ]
}
```

#### Exchange 008

catalogue.1 · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`CatalogueTool`](#cataloguetool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__catalogue_1",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "catalogue.1"
        }
    ]
}
```

#### Exchange 009

catalogue.2 · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`CatalogueTool`](#cataloguetool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__catalogue_2",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "catalogue.2"
        }
    ]
}
```

#### Exchange 010

catalogue.3 · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`CatalogueTool`](#cataloguetool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__catalogue_3",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "catalogue.3"
        }
    ]
}
```

#### Exchange 011

catalogue.4 · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`CatalogueTool`](#cataloguetool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__catalogue_4",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "catalogue.4"
        }
    ]
}
```

#### Exchange 012

catalogue.5 · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`CatalogueTool`](#cataloguetool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__catalogue_5",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "catalogue.5"
        }
    ]
}
```

#### Exchange 013

catalogue.6 · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`CatalogueTool`](#cataloguetool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__catalogue_6",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "catalogue.6"
        }
    ]
}
```

#### Exchange 014

catalogue.7 · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`CatalogueTool`](#cataloguetool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__catalogue_7",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "catalogue.7"
        }
    ]
}
```

#### Exchange 015

catalogue.8 · showcase-http · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`CatalogueTool`](#cataloguetool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__catalogue_8",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "catalogue.8"
        }
    ]
}
```

#### Exchange 053

catalogue.0 · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`CatalogueTool`](#cataloguetool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__catalogue_0",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "catalogue.0"
        }
    ]
}
```

#### Exchange 054

catalogue.1 · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`CatalogueTool`](#cataloguetool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__catalogue_1",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "catalogue.1"
        }
    ]
}
```

#### Exchange 055

catalogue.2 · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`CatalogueTool`](#cataloguetool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__catalogue_2",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "catalogue.2"
        }
    ]
}
```

#### Exchange 056

catalogue.3 · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`CatalogueTool`](#cataloguetool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__catalogue_3",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "catalogue.3"
        }
    ]
}
```

#### Exchange 057

catalogue.4 · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`CatalogueTool`](#cataloguetool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__catalogue_4",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "catalogue.4"
        }
    ]
}
```

#### Exchange 058

catalogue.5 · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`CatalogueTool`](#cataloguetool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__catalogue_5",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "catalogue.5"
        }
    ]
}
```

#### Exchange 059

catalogue.6 · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`CatalogueTool`](#cataloguetool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__catalogue_6",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "catalogue.6"
        }
    ]
}
```

#### Exchange 060

catalogue.7 · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`CatalogueTool`](#cataloguetool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__catalogue_7",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "catalogue.7"
        }
    ]
}
```

#### Exchange 061

catalogue.8 · showcase-stdio · Expanded baseline calls · PASS

Expanded baseline calls · Capture: recorded tool projection

**Server classes** [`CatalogueTool`](#cataloguetool)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__catalogue_8",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "metadataKeys": [],
    "content": [
        {
            "type": "text",
            "text": "catalogue.8"
        }
    ]
}
```

#### Exchange 193

Resource list · showcase-stdio · Dynamic registration baseline · PASS

Dynamic registration baseline · Capture: dynamic inventory projection

**Server classes** [`DynamicTool`](#dynamictool) [`CatalogueResource`](#catalogueresource)

**Framework** [Resource inventory](#resource-inventory)

**Codex → MCP request**

```json
{
    "tool": "list_mcp_resources",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "observedServer": "showcase-stdio",
    "matchingResourceUris": [],
    "callableDynamicTools": []
}
```

#### Exchange 194

Template list · showcase-stdio · Dynamic registration baseline · PASS

Dynamic registration baseline · Capture: dynamic inventory projection

**Server classes** [`DynamicTool`](#dynamictool) [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource template inventory](#resource-template-inventory)

**Codex → MCP request**

```json
{
    "tool": "list_mcp_resource_templates",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "observedServer": "showcase-stdio",
    "matchingTemplateUris": []
}
```

#### Exchange 195

dynamic · showcase-stdio · Dynamic registration and cleanup · add · PASS

Dynamic registration and cleanup · add · Capture: native tool result

**Server classes** [`DynamicTool`](#dynamictool) [`CatalogueTool`](#cataloguetool) [`CatalogueResource`](#catalogueresource) [`CatalogueTemplate`](#cataloguetemplate) [`CataloguePrompt`](#catalogueprompt)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__dynamic",
    "arguments": {
        "action": "add"
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "add"
        }
    ]
}
```

#### Exchange 196

Resource list · showcase-stdio · Dynamic registration added · PASS

Dynamic registration added · Capture: dynamic inventory projection

**Server classes** [`DynamicTool`](#dynamictool) [`CatalogueResource`](#catalogueresource)

**Framework** [Resource inventory](#resource-inventory)

**Codex → MCP request**

```json
{
    "tool": "list_mcp_resources",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "observedServer": "showcase-stdio",
    "matchingResourceUris": [
        "showcase://dynamic"
    ],
    "callableDynamicTools": []
}
```

#### Exchange 197

Template list · showcase-stdio · Dynamic registration added · PASS

Dynamic registration added · Capture: dynamic inventory projection

**Server classes** [`DynamicTool`](#dynamictool) [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource template inventory](#resource-template-inventory)

**Codex → MCP request**

```json
{
    "tool": "list_mcp_resource_templates",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "observedServer": "showcase-stdio",
    "matchingTemplateUris": [
        "showcase://dynamic/{value}"
    ]
}
```

#### Exchange 198

Resource read · showcase-stdio · Read while dynamically registered · showcase://dynamic · PASS

Read while dynamically registered · showcase://dynamic · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource) [`DynamicTool`](#dynamictool)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://dynamic"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://dynamic",
    "contents": [
        {
            "uri": "showcase://dynamic",
            "mimeType": "text/plain",
            "text": "dynamic"
        }
    ]
}
```

#### Exchange 199

dynamic · showcase-stdio · Dynamic registration and cleanup · remove · PASS

Dynamic registration and cleanup · remove · Capture: native tool result

**Server classes** [`DynamicTool`](#dynamictool) [`CatalogueTool`](#cataloguetool) [`CatalogueResource`](#catalogueresource) [`CatalogueTemplate`](#cataloguetemplate) [`CataloguePrompt`](#catalogueprompt)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__dynamic",
    "arguments": {
        "action": "remove"
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "remove"
        }
    ]
}
```

#### Exchange 200

Resource list · showcase-stdio · Dynamic registration removed · PASS

Dynamic registration removed · Capture: dynamic inventory projection

**Server classes** [`DynamicTool`](#dynamictool) [`CatalogueResource`](#catalogueresource)

**Framework** [Resource inventory](#resource-inventory)

**Codex → MCP request**

```json
{
    "tool": "list_mcp_resources",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "observedServer": "showcase-stdio",
    "matchingResourceUris": [],
    "callableDynamicTools": []
}
```

#### Exchange 201

Template list · showcase-stdio · Dynamic registration removed · PASS

Dynamic registration removed · Capture: dynamic inventory projection

**Server classes** [`DynamicTool`](#dynamictool) [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource template inventory](#resource-template-inventory)

**Codex → MCP request**

```json
{
    "tool": "list_mcp_resource_templates",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "observedServer": "showcase-stdio",
    "matchingTemplateUris": []
}
```

#### Exchange 202

Resource read · showcase-stdio · Counter before update · showcase://counter · PASS

Counter before update · showcase://counter · Capture: native json text

**Server classes** [`CounterResource`](#counterresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://counter"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://counter",
    "contents": [
        {
            "uri": "showcase://counter",
            "mimeType": "text/plain",
            "text": "0"
        }
    ]
}
```

#### Exchange 203

update · showcase-stdio · Counter update · PASS

Counter update · Capture: native tool result

**Server classes** [`UpdateTool`](#updatetool) [`CounterResource`](#counterresource)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_stdio__update",
    "arguments": {}
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "revision": 1
            }
        }
    ],
    "structuredContent": {
        "revision": 1
    }
}
```

#### Exchange 204

Resource read · showcase-stdio · Counter after update · showcase://counter · PASS

Counter after update · showcase://counter · Capture: native json text

**Server classes** [`CounterResource`](#counterresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://counter"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://counter",
    "contents": [
        {
            "uri": "showcase://counter",
            "mimeType": "text/plain",
            "text": "1"
        }
    ]
}
```

#### Exchange 205

Resource list · showcase-http · Dynamic registration baseline · PASS

Dynamic registration baseline · Capture: dynamic inventory projection

**Server classes** [`DynamicTool`](#dynamictool) [`CatalogueResource`](#catalogueresource)

**Framework** [Resource inventory](#resource-inventory)

**Codex → MCP request**

```json
{
    "tool": "list_mcp_resources",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "observedServer": "showcase-http",
    "matchingResourceUris": [],
    "callableDynamicTools": []
}
```

#### Exchange 206

Template list · showcase-http · Dynamic registration baseline · PASS

Dynamic registration baseline · Capture: dynamic inventory projection

**Server classes** [`DynamicTool`](#dynamictool) [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource template inventory](#resource-template-inventory)

**Codex → MCP request**

```json
{
    "tool": "list_mcp_resource_templates",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "observedServer": "showcase-http",
    "matchingTemplateUris": []
}
```

#### Exchange 207

dynamic · showcase-http · Dynamic registration and cleanup · add · PASS

Dynamic registration and cleanup · add · Capture: native tool result

**Server classes** [`DynamicTool`](#dynamictool) [`CatalogueTool`](#cataloguetool) [`CatalogueResource`](#catalogueresource) [`CatalogueTemplate`](#cataloguetemplate) [`CataloguePrompt`](#catalogueprompt)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__dynamic",
    "arguments": {
        "action": "add"
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "add"
        }
    ]
}
```

#### Exchange 208

Resource list · showcase-http · Dynamic registration added · PASS

Dynamic registration added · Capture: dynamic inventory projection

**Server classes** [`DynamicTool`](#dynamictool) [`CatalogueResource`](#catalogueresource)

**Framework** [Resource inventory](#resource-inventory)

**Codex → MCP request**

```json
{
    "tool": "list_mcp_resources",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "observedServer": "showcase-http",
    "matchingResourceUris": [
        "showcase://dynamic"
    ],
    "callableDynamicTools": []
}
```

#### Exchange 209

Template list · showcase-http · Dynamic registration added · PASS

Dynamic registration added · Capture: dynamic inventory projection

**Server classes** [`DynamicTool`](#dynamictool) [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource template inventory](#resource-template-inventory)

**Codex → MCP request**

```json
{
    "tool": "list_mcp_resource_templates",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "observedServer": "showcase-http",
    "matchingTemplateUris": [
        "showcase://dynamic/{value}"
    ]
}
```

#### Exchange 210

Resource read · showcase-http · Read while dynamically registered · showcase://dynamic · PASS

Read while dynamically registered · showcase://dynamic · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource) [`DynamicTool`](#dynamictool)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://dynamic"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://dynamic",
    "contents": [
        {
            "uri": "showcase://dynamic",
            "mimeType": "text/plain",
            "text": "dynamic"
        }
    ]
}
```

#### Exchange 211

dynamic · showcase-http · Dynamic registration and cleanup · remove · PASS

Dynamic registration and cleanup · remove · Capture: native tool result

**Server classes** [`DynamicTool`](#dynamictool) [`CatalogueTool`](#cataloguetool) [`CatalogueResource`](#catalogueresource) [`CatalogueTemplate`](#cataloguetemplate) [`CataloguePrompt`](#catalogueprompt)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__dynamic",
    "arguments": {
        "action": "remove"
    }
}
```

**MCP → Codex response**

```json
{
    "content": [
        {
            "type": "text",
            "text": "remove"
        }
    ]
}
```

#### Exchange 212

Resource list · showcase-http · Dynamic registration removed · PASS

Dynamic registration removed · Capture: dynamic inventory projection

**Server classes** [`DynamicTool`](#dynamictool) [`CatalogueResource`](#catalogueresource)

**Framework** [Resource inventory](#resource-inventory)

**Codex → MCP request**

```json
{
    "tool": "list_mcp_resources",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "observedServer": "showcase-http",
    "matchingResourceUris": [],
    "callableDynamicTools": []
}
```

#### Exchange 213

Template list · showcase-http · Dynamic registration removed · PASS

Dynamic registration removed · Capture: dynamic inventory projection

**Server classes** [`DynamicTool`](#dynamictool) [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource template inventory](#resource-template-inventory)

**Codex → MCP request**

```json
{
    "tool": "list_mcp_resource_templates",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "observedServer": "showcase-http",
    "matchingTemplateUris": []
}
```

#### Exchange 214

Resource read · showcase-http · Counter before update · showcase://counter · PASS

Counter before update · showcase://counter · Capture: native json text

**Server classes** [`CounterResource`](#counterresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://counter"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://counter",
    "contents": [
        {
            "uri": "showcase://counter",
            "mimeType": "text/plain",
            "text": "0"
        }
    ]
}
```

#### Exchange 215

update · showcase-http · Counter update · PASS

Counter update · Capture: native tool result

**Server classes** [`UpdateTool`](#updatetool) [`CounterResource`](#counterresource)

**Framework** [Tool dispatch and input validation](#tool-dispatch-and-input-validation)

**Codex → MCP request**

```json
{
    "tool": "mcp__showcase_http__update",
    "arguments": {}
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "content": [
        {
            "type": "text",
            "text": {
                "revision": 1
            }
        }
    ],
    "structuredContent": {
        "revision": 1
    }
}
```

#### Exchange 216

Resource read · showcase-http · Counter after update · showcase://counter · PASS

Counter after update · showcase://counter · Capture: native json text

**Server classes** [`CounterResource`](#counterresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://counter"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://counter",
    "contents": [
        {
            "uri": "showcase://counter",
            "mimeType": "text/plain",
            "text": "1"
        }
    ]
}
```

#### Exchange 218

Resource read · showcase-stdio · Final counter observation · showcase://counter · PASS

Final counter observation · showcase://counter · Capture: recorded resource projection

**Server classes** [`CounterResource`](#counterresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://counter"
    }
}
```

**MCP → Codex response**

```json
{
    "counter": "1"
}
```

#### Exchange 219

Resource read · showcase-stdio · Verify dynamic cleanup · showcase://dynamic · EXPECTED ERROR

Verify dynamic cleanup · showcase://dynamic · Capture: native exception

**Server classes** [`CatalogueResource`](#catalogueresource) [`DynamicTool`](#dynamictool)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://dynamic"
    }
}
```

**MCP → Codex response**

```json
{
    "error": "resources/read failed: resources/read failed for `showcase-stdio` (showcase://dynamic): Mcp error: -32002: Unknown registration or malformed arguments"
}
```

#### Exchange 221

Resource read · showcase-http · Final counter observation · showcase://counter · PASS

Final counter observation · showcase://counter · Capture: recorded resource projection

**Server classes** [`CounterResource`](#counterresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://counter"
    }
}
```

**MCP → Codex response**

```json
{
    "counter": "2"
}
```

#### Exchange 222

Resource read · showcase-http · Verify dynamic cleanup · showcase://dynamic · EXPECTED ERROR

Verify dynamic cleanup · showcase://dynamic · Capture: native exception

**Server classes** [`CatalogueResource`](#catalogueresource) [`DynamicTool`](#dynamictool)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://dynamic"
    }
}
```

**MCP → Codex response**

```json
{
    "error": "resources/read failed: resources/read failed for `showcase-http` (showcase://dynamic): Mcp error: -32002: Unknown registration or malformed arguments"
}
```

### Resource and template discovery and reads

#### Exchange 123

Resource list · all four · Initial resource discovery · PASS

Initial resource discovery · Capture: local server list projection

**Server classes** [`HouseStyleResource`](#housestyleresource) [`ReviewLabelResource`](#reviewlabelresource) [`TextResourceItem`](#textresourceitem) [`BlobResourceItem`](#blobresourceitem) [`CounterResource`](#counterresource) [`InputResource`](#inputresource) [`CatalogueResource`](#catalogueresource)

**Framework** [Resource inventory](#resource-inventory)

**Codex → MCP request**

```json
{
    "tool": "list_mcp_resources",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "resources": [
        {
            "server": "showcase-http",
            "uri": "showcase://blob",
            "name": "blob",
            "description": "Generated binary resource with private cache hints.",
            "mimeType": "application/octet-stream"
        },
        {
            "server": "showcase-http",
            "uri": "showcase://catalogue/0",
            "name": "catalogue.0",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uri": "showcase://catalogue/1",
            "name": "catalogue.1",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uri": "showcase://catalogue/2",
            "name": "catalogue.2",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uri": "showcase://catalogue/3",
            "name": "catalogue.3",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uri": "showcase://catalogue/4",
            "name": "catalogue.4",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uri": "showcase://catalogue/5",
            "name": "catalogue.5",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uri": "showcase://catalogue/6",
            "name": "catalogue.6",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uri": "showcase://catalogue/7",
            "name": "catalogue.7",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uri": "showcase://catalogue/8",
            "name": "catalogue.8",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uri": "showcase://counter",
            "name": "counter",
            "description": "Changing resource; subscribe/listen before calling update.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uri": "showcase://input",
            "name": "input",
            "description": "Resource-originated elicitation and input_required.",
            "mimeType": "application/json"
        },
        {
            "server": "showcase-http",
            "uri": "showcase://text",
            "name": "text",
            "title": "Showcase text",
            "description": "Text resource, metadata and explicit public caching.",
            "mimeType": "text/plain",
            "icons": [
                {
                    "src": "data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIxNiIgaGVpZ2h0PSIxNiI+PHJlY3Qgd2lkdGg9IjE2IiBoZWlnaHQ9IjE2IiBmaWxsPSIjMjQ3Ii8+PGNpcmNsZSBjeD0iOCIgY3k9IjgiIHI9IjUiIGZpbGw9IiNmZmYiLz48L3N2Zz4=",
                    "mimeType": "image/svg+xml",
                    "sizes": [
                        "16x16"
                    ],
                    "theme": "light"
                }
            ],
            "_meta": {
                "showcase/resource": "text"
            },
            "annotations": {
                "audience": [
                    "user"
                ],
                "priority": 0.7,
                "lastModified": "2026-01-01T00:00:00Z"
            }
        },
        {
            "server": "showcase-http",
            "uri": "snodec://development/house-style",
            "name": "SNode.C house style",
            "description": "",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uri": "snodec://development/review-label",
            "name": "Current review label",
            "description": "The non-sensitive label selected with review.label.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uri": "showcase://blob",
            "name": "blob",
            "description": "Generated binary resource with private cache hints.",
            "mimeType": "application/octet-stream"
        },
        {
            "server": "showcase-stdio",
            "uri": "showcase://catalogue/0",
            "name": "catalogue.0",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uri": "showcase://catalogue/1",
            "name": "catalogue.1",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uri": "showcase://catalogue/2",
            "name": "catalogue.2",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uri": "showcase://catalogue/3",
            "name": "catalogue.3",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uri": "showcase://catalogue/4",
            "name": "catalogue.4",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uri": "showcase://catalogue/5",
            "name": "catalogue.5",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uri": "showcase://catalogue/6",
            "name": "catalogue.6",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uri": "showcase://catalogue/7",
            "name": "catalogue.7",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uri": "showcase://catalogue/8",
            "name": "catalogue.8",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uri": "showcase://counter",
            "name": "counter",
            "description": "Changing resource; subscribe/listen before calling update.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uri": "showcase://input",
            "name": "input",
            "description": "Resource-originated elicitation and input_required.",
            "mimeType": "application/json"
        },
        {
            "server": "showcase-stdio",
            "uri": "showcase://text",
            "name": "text",
            "title": "Showcase text",
            "description": "Text resource, metadata and explicit public caching.",
            "mimeType": "text/plain",
            "icons": [
                {
                    "src": "data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIxNiIgaGVpZ2h0PSIxNiI+PHJlY3Qgd2lkdGg9IjE2IiBoZWlnaHQ9IjE2IiBmaWxsPSIjMjQ3Ii8+PGNpcmNsZSBjeD0iOCIgY3k9IjgiIHI9IjUiIGZpbGw9IiNmZmYiLz48L3N2Zz4=",
                    "mimeType": "image/svg+xml",
                    "sizes": [
                        "16x16"
                    ],
                    "theme": "light"
                }
            ],
            "_meta": {
                "showcase/resource": "text"
            },
            "annotations": {
                "audience": [
                    "user"
                ],
                "priority": 0.7,
                "lastModified": "2026-01-01T00:00:00Z"
            }
        },
        {
            "server": "showcase-stdio",
            "uri": "snodec://development/house-style",
            "name": "SNode.C house style",
            "description": "",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uri": "snodec://development/review-label",
            "name": "Current review label",
            "description": "The non-sensitive label selected with review.label.",
            "mimeType": "text/plain"
        },
        {
            "server": "snodec-dev-http",
            "uri": "snodec://development/house-style",
            "name": "SNode.C house style",
            "description": "",
            "mimeType": "text/plain"
        },
        {
            "server": "snodec-dev-http",
            "uri": "snodec://development/review-label",
            "name": "Current review label",
            "description": "The non-sensitive label selected with review.label.",
            "mimeType": "text/plain"
        },
        {
            "server": "snodec-dev-stdio",
            "uri": "snodec://development/house-style",
            "name": "SNode.C house style",
            "description": "",
            "mimeType": "text/plain"
        },
        {
            "server": "snodec-dev-stdio",
            "uri": "snodec://development/review-label",
            "name": "Current review label",
            "description": "The non-sensitive label selected with review.label.",
            "mimeType": "text/plain"
        }
    ]
}
```

#### Exchange 124

Template list · all four · Initial resource discovery · PASS

Initial resource discovery · Capture: local server list projection

**Server classes** [`StyleTemplate`](#styletemplate) [`GreetingTemplate`](#greetingtemplate) [`PathTemplate`](#pathtemplate) [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource template inventory](#resource-template-inventory)

**Codex → MCP request**

```json
{
    "tool": "list_mcp_resource_templates",
    "arguments": {}
}
```

**MCP → Codex response**

```json
{
    "resourceTemplates": [
        {
            "server": "showcase-http",
            "uriTemplate": "showcase://catalogue/0/{value}",
            "name": "catalogue.0",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uriTemplate": "showcase://catalogue/1/{value}",
            "name": "catalogue.1",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uriTemplate": "showcase://catalogue/2/{value}",
            "name": "catalogue.2",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uriTemplate": "showcase://catalogue/3/{value}",
            "name": "catalogue.3",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uriTemplate": "showcase://catalogue/4/{value}",
            "name": "catalogue.4",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uriTemplate": "showcase://catalogue/5/{value}",
            "name": "catalogue.5",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uriTemplate": "showcase://catalogue/6/{value}",
            "name": "catalogue.6",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uriTemplate": "showcase://catalogue/7/{value}",
            "name": "catalogue.7",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uriTemplate": "showcase://catalogue/8/{value}",
            "name": "catalogue.8",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uriTemplate": "showcase://greeting/{name}",
            "name": "greeting",
            "description": "Template variables and contextual completion.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uriTemplate": "showcase://path/{+path}",
            "name": "path",
            "description": "Reserved URI-template expansion.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-http",
            "uriTemplate": "snodec://development/style/{section}",
            "name": "SNode.C style sections",
            "description": "Read the existing house-style overview.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uriTemplate": "showcase://catalogue/0/{value}",
            "name": "catalogue.0",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uriTemplate": "showcase://catalogue/1/{value}",
            "name": "catalogue.1",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uriTemplate": "showcase://catalogue/2/{value}",
            "name": "catalogue.2",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uriTemplate": "showcase://catalogue/3/{value}",
            "name": "catalogue.3",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uriTemplate": "showcase://catalogue/4/{value}",
            "name": "catalogue.4",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uriTemplate": "showcase://catalogue/5/{value}",
            "name": "catalogue.5",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uriTemplate": "showcase://catalogue/6/{value}",
            "name": "catalogue.6",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uriTemplate": "showcase://catalogue/7/{value}",
            "name": "catalogue.7",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uriTemplate": "showcase://catalogue/8/{value}",
            "name": "catalogue.8",
            "description": "Catalogue entry demonstrating pagination and dynamic registration.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uriTemplate": "showcase://greeting/{name}",
            "name": "greeting",
            "description": "Template variables and contextual completion.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uriTemplate": "showcase://path/{+path}",
            "name": "path",
            "description": "Reserved URI-template expansion.",
            "mimeType": "text/plain"
        },
        {
            "server": "showcase-stdio",
            "uriTemplate": "snodec://development/style/{section}",
            "name": "SNode.C style sections",
            "description": "Read the existing house-style overview.",
            "mimeType": "text/plain"
        },
        {
            "server": "snodec-dev-http",
            "uriTemplate": "snodec://development/style/{section}",
            "name": "SNode.C style sections",
            "description": "Read the existing house-style overview.",
            "mimeType": "text/plain"
        },
        {
            "server": "snodec-dev-stdio",
            "uriTemplate": "snodec://development/style/{section}",
            "name": "SNode.C style sections",
            "description": "Read the existing house-style overview.",
            "mimeType": "text/plain"
        }
    ]
}
```

#### Exchange 125

Resource read · showcase-http · Resource and template reads · showcase://blob · PASS

Resource and template reads · showcase://blob · Capture: recorded resource projection

**Server classes** [`BlobResourceItem`](#blobresourceitem)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://blob"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://blob",
            "mimeType": "application/octet-stream",
            "blob": "AAEC"
        }
    ]
}
```

#### Exchange 126

Resource read · showcase-http · Resource and template reads · showcase://catalogue/0 · PASS

Resource and template reads · showcase://catalogue/0 · Capture: recorded resource projection

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/0"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/0",
            "mimeType": "text/plain",
            "text": "catalogue.0"
        }
    ]
}
```

#### Exchange 127

Resource read · showcase-http · Resource and template reads · showcase://catalogue/1 · PASS

Resource and template reads · showcase://catalogue/1 · Capture: recorded resource projection

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/1"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/1",
            "mimeType": "text/plain",
            "text": "catalogue.1"
        }
    ]
}
```

#### Exchange 128

Resource read · showcase-http · Resource and template reads · showcase://catalogue/2 · PASS

Resource and template reads · showcase://catalogue/2 · Capture: recorded resource projection

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/2"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/2",
            "mimeType": "text/plain",
            "text": "catalogue.2"
        }
    ]
}
```

#### Exchange 129

Resource read · showcase-http · Resource and template reads · showcase://catalogue/3 · PASS

Resource and template reads · showcase://catalogue/3 · Capture: recorded resource projection

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/3"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/3",
            "mimeType": "text/plain",
            "text": "catalogue.3"
        }
    ]
}
```

#### Exchange 130

Resource read · showcase-http · Resource and template reads · showcase://catalogue/4 · PASS

Resource and template reads · showcase://catalogue/4 · Capture: recorded resource projection

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/4"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/4",
            "mimeType": "text/plain",
            "text": "catalogue.4"
        }
    ]
}
```

#### Exchange 131

Resource read · showcase-http · Resource and template reads · showcase://catalogue/5 · PASS

Resource and template reads · showcase://catalogue/5 · Capture: recorded resource projection

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/5"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/5",
            "mimeType": "text/plain",
            "text": "catalogue.5"
        }
    ]
}
```

#### Exchange 132

Resource read · showcase-http · Resource and template reads · showcase://catalogue/6 · PASS

Resource and template reads · showcase://catalogue/6 · Capture: recorded resource projection

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/6"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/6",
            "mimeType": "text/plain",
            "text": "catalogue.6"
        }
    ]
}
```

#### Exchange 133

Resource read · showcase-http · Resource and template reads · showcase://catalogue/7 · PASS

Resource and template reads · showcase://catalogue/7 · Capture: recorded resource projection

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/7"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/7",
            "mimeType": "text/plain",
            "text": "catalogue.7"
        }
    ]
}
```

#### Exchange 134

Resource read · showcase-http · Resource and template reads · showcase://catalogue/8 · PASS

Resource and template reads · showcase://catalogue/8 · Capture: recorded resource projection

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/8"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/8",
            "mimeType": "text/plain",
            "text": "catalogue.8"
        }
    ]
}
```

#### Exchange 137

Resource read · showcase-http · Resource and template reads · showcase://text · PASS

Resource and template reads · showcase://text · Capture: recorded resource projection

**Server classes** [`TextResourceItem`](#textresourceitem)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://text"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://text",
            "mimeType": "text/plain",
            "text": "SNode.C\n"
        }
    ]
}
```

#### Exchange 138

Resource read · showcase-http · Resource and template reads · snodec://development/house-style · PASS

Resource and template reads · snodec://development/house-style · Capture: recorded resource projection

**Server classes** [`HouseStyleResource`](#housestyleresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "snodec://development/house-style"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "snodec://development/house-style",
            "mimeType": "text/plain",
            "text": "SNode.C: use one event loop; configure named instances before init; listen, then start. Use semantic logging, express routing and "
        }
    ]
}
```

#### Exchange 141

Resource read · showcase-http · Resource and template reads · showcase://catalogue/0/test · PASS

Resource and template reads · showcase://catalogue/0/test · Capture: recorded resource projection

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/0/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/0/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 142

Resource read · showcase-http · Resource and template reads · showcase://catalogue/1/test · PASS

Resource and template reads · showcase://catalogue/1/test · Capture: recorded resource projection

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/1/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/1/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 143

Resource read · showcase-http · Resource and template reads · showcase://catalogue/2/test · PASS

Resource and template reads · showcase://catalogue/2/test · Capture: recorded resource projection

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/2/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/2/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 144

Resource read · showcase-http · Resource and template reads · showcase://catalogue/3/test · PASS

Resource and template reads · showcase://catalogue/3/test · Capture: recorded resource projection

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/3/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/3/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 145

Resource read · showcase-http · Resource and template reads · showcase://catalogue/4/test · PASS

Resource and template reads · showcase://catalogue/4/test · Capture: recorded resource projection

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/4/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/4/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 146

Resource read · showcase-http · Resource and template reads · showcase://catalogue/5/test · PASS

Resource and template reads · showcase://catalogue/5/test · Capture: recorded resource projection

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/5/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/5/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 147

Resource read · showcase-http · Resource and template reads · showcase://catalogue/6/test · PASS

Resource and template reads · showcase://catalogue/6/test · Capture: recorded resource projection

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/6/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/6/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 148

Resource read · showcase-http · Resource and template reads · showcase://catalogue/7/test · PASS

Resource and template reads · showcase://catalogue/7/test · Capture: recorded resource projection

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/7/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/7/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 149

Resource read · showcase-http · Resource and template reads · showcase://catalogue/8/test · PASS

Resource and template reads · showcase://catalogue/8/test · Capture: recorded resource projection

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/8/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/8/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 150

Resource read · showcase-http · Resource and template reads · showcase://greeting/test · PASS

Resource and template reads · showcase://greeting/test · Capture: recorded resource projection

**Server classes** [`GreetingTemplate`](#greetingtemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://greeting/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://greeting/test",
            "mimeType": "text/plain",
            "text": "Hello test"
        }
    ]
}
```

#### Exchange 151

Resource read · showcase-http · Resource and template reads · showcase://path/one/two · PASS

Resource and template reads · showcase://path/one/two · Capture: recorded resource projection

**Server classes** [`PathTemplate`](#pathtemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://path/one/two"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://path/one/two",
            "mimeType": "text/plain",
            "text": "one/two"
        }
    ]
}
```

#### Exchange 152

Resource read · showcase-http · Resource and template reads · snodec://development/style/test · EXPECTED ERROR

Resource and template reads · snodec://development/style/test · Capture: native exception

**Server classes** [`StyleTemplate`](#styletemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "snodec://development/style/test"
    }
}
```

**MCP → Codex response**

```json
{
    "error": "resources/read failed: resources/read failed for `showcase-http` (snodec://development/style/test): Mcp error: -32602: Unknown style section"
}
```

#### Exchange 153

Resource read · showcase-http · Style and selected-label verification · snodec://development/style/overview · PASS

Style and selected-label verification · snodec://development/style/overview · Capture: recorded resource projection

**Server classes** [`StyleTemplate`](#styletemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "snodec://development/style/overview"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "snodec://development/style/overview",
            "text": "SNode.C: use one event loop; configure named instances before init; listen, then start. Use semantic logging, express routing and existing CLI11 instance options. Ke"
        }
    ]
}
```

#### Exchange 154

Resource read · showcase-stdio · Resource and template reads · showcase://blob · PASS

Resource and template reads · showcase://blob · Capture: recorded resource projection

**Server classes** [`BlobResourceItem`](#blobresourceitem)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://blob"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://blob",
            "mimeType": "application/octet-stream",
            "blob": "AAEC"
        }
    ]
}
```

#### Exchange 155

Resource read · showcase-stdio · Resource and template reads · showcase://catalogue/0 · PASS

Resource and template reads · showcase://catalogue/0 · Capture: recorded resource projection

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/0"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/0",
            "mimeType": "text/plain",
            "text": "catalogue.0"
        }
    ]
}
```

#### Exchange 156

Resource read · showcase-stdio · Resource and template reads · showcase://catalogue/1 · PASS

Resource and template reads · showcase://catalogue/1 · Capture: recorded resource projection

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/1"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/1",
            "mimeType": "text/plain",
            "text": "catalogue.1"
        }
    ]
}
```

#### Exchange 157

Resource read · showcase-stdio · Resource and template reads · showcase://catalogue/2 · PASS

Resource and template reads · showcase://catalogue/2 · Capture: recorded resource projection

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/2"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/2",
            "mimeType": "text/plain",
            "text": "catalogue.2"
        }
    ]
}
```

#### Exchange 158

Resource read · showcase-stdio · Resource and template reads · showcase://catalogue/3 · PASS

Resource and template reads · showcase://catalogue/3 · Capture: recorded resource projection

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/3"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/3",
            "mimeType": "text/plain",
            "text": "catalogue.3"
        }
    ]
}
```

#### Exchange 159

Resource read · showcase-stdio · Resource and template reads · showcase://catalogue/4 · PASS

Resource and template reads · showcase://catalogue/4 · Capture: recorded resource projection

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/4"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/4",
            "mimeType": "text/plain",
            "text": "catalogue.4"
        }
    ]
}
```

#### Exchange 160

Resource read · showcase-stdio · Resource and template reads · showcase://catalogue/5 · PASS

Resource and template reads · showcase://catalogue/5 · Capture: recorded resource projection

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/5"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/5",
            "mimeType": "text/plain",
            "text": "catalogue.5"
        }
    ]
}
```

#### Exchange 161

Resource read · showcase-stdio · Resource and template reads · showcase://catalogue/6 · PASS

Resource and template reads · showcase://catalogue/6 · Capture: recorded resource projection

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/6"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/6",
            "mimeType": "text/plain",
            "text": "catalogue.6"
        }
    ]
}
```

#### Exchange 162

Resource read · showcase-stdio · Resource and template reads · showcase://catalogue/7 · PASS

Resource and template reads · showcase://catalogue/7 · Capture: recorded resource projection

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/7"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/7",
            "mimeType": "text/plain",
            "text": "catalogue.7"
        }
    ]
}
```

#### Exchange 163

Resource read · showcase-stdio · Resource and template reads · showcase://catalogue/8 · PASS

Resource and template reads · showcase://catalogue/8 · Capture: recorded resource projection

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/8"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/8",
            "mimeType": "text/plain",
            "text": "catalogue.8"
        }
    ]
}
```

#### Exchange 166

Resource read · showcase-stdio · Resource and template reads · showcase://text · PASS

Resource and template reads · showcase://text · Capture: recorded resource projection

**Server classes** [`TextResourceItem`](#textresourceitem)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://text"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://text",
            "mimeType": "text/plain",
            "text": "SNode.C\n"
        }
    ]
}
```

#### Exchange 167

Resource read · showcase-stdio · Resource and template reads · snodec://development/house-style · PASS

Resource and template reads · snodec://development/house-style · Capture: recorded resource projection

**Server classes** [`HouseStyleResource`](#housestyleresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "snodec://development/house-style"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "snodec://development/house-style",
            "mimeType": "text/plain",
            "text": "SNode.C: use one event loop; configure named instances before init; listen, then start. Use semantic logging, express routing and "
        }
    ]
}
```

#### Exchange 170

Resource read · showcase-stdio · Resource and template reads · showcase://catalogue/0/test · PASS

Resource and template reads · showcase://catalogue/0/test · Capture: recorded resource projection

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/0/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/0/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 171

Resource read · showcase-stdio · Resource and template reads · showcase://catalogue/1/test · PASS

Resource and template reads · showcase://catalogue/1/test · Capture: recorded resource projection

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/1/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/1/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 172

Resource read · showcase-stdio · Resource and template reads · showcase://catalogue/2/test · PASS

Resource and template reads · showcase://catalogue/2/test · Capture: recorded resource projection

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/2/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/2/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 173

Resource read · showcase-stdio · Resource and template reads · showcase://catalogue/3/test · PASS

Resource and template reads · showcase://catalogue/3/test · Capture: recorded resource projection

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/3/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/3/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 174

Resource read · showcase-stdio · Resource and template reads · showcase://catalogue/4/test · PASS

Resource and template reads · showcase://catalogue/4/test · Capture: recorded resource projection

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/4/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/4/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 175

Resource read · showcase-stdio · Resource and template reads · showcase://catalogue/5/test · PASS

Resource and template reads · showcase://catalogue/5/test · Capture: recorded resource projection

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/5/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/5/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 176

Resource read · showcase-stdio · Resource and template reads · showcase://catalogue/6/test · PASS

Resource and template reads · showcase://catalogue/6/test · Capture: recorded resource projection

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/6/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/6/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 177

Resource read · showcase-stdio · Resource and template reads · showcase://catalogue/7/test · PASS

Resource and template reads · showcase://catalogue/7/test · Capture: recorded resource projection

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/7/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/7/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 178

Resource read · showcase-stdio · Resource and template reads · showcase://catalogue/8/test · PASS

Resource and template reads · showcase://catalogue/8/test · Capture: recorded resource projection

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/8/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://catalogue/8/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 179

Resource read · showcase-stdio · Resource and template reads · showcase://greeting/test · PASS

Resource and template reads · showcase://greeting/test · Capture: recorded resource projection

**Server classes** [`GreetingTemplate`](#greetingtemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://greeting/test"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://greeting/test",
            "mimeType": "text/plain",
            "text": "Hello test"
        }
    ]
}
```

#### Exchange 180

Resource read · showcase-stdio · Resource and template reads · showcase://path/one/two · PASS

Resource and template reads · showcase://path/one/two · Capture: recorded resource projection

**Server classes** [`PathTemplate`](#pathtemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://path/one/two"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "showcase://path/one/two",
            "mimeType": "text/plain",
            "text": "one/two"
        }
    ]
}
```

#### Exchange 181

Resource read · showcase-stdio · Resource and template reads · snodec://development/style/test · EXPECTED ERROR

Resource and template reads · snodec://development/style/test · Capture: native exception

**Server classes** [`StyleTemplate`](#styletemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "snodec://development/style/test"
    }
}
```

**MCP → Codex response**

```json
{
    "error": "resources/read failed: resources/read failed for `showcase-stdio` (snodec://development/style/test): Mcp error: -32602: Unknown style section"
}
```

#### Exchange 182

Resource read · showcase-stdio · Style and selected-label verification · snodec://development/style/overview · PASS

Style and selected-label verification · snodec://development/style/overview · Capture: recorded resource projection

**Server classes** [`StyleTemplate`](#styletemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "snodec://development/style/overview"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "snodec://development/style/overview",
            "text": "SNode.C: use one event loop; configure named instances before init; listen, then start. Use semantic logging, express routing and existing CLI11 instance options. Ke"
        }
    ]
}
```

#### Exchange 183

Resource read · snodec-dev-http · Resource and template reads · snodec://development/house-style · PASS

Resource and template reads · snodec://development/house-style · Capture: recorded resource projection

**Server classes** [`HouseStyleResource`](#housestyleresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "snodec-dev-http",
        "uri": "snodec://development/house-style"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "snodec://development/house-style",
            "mimeType": "text/plain",
            "text": "SNode.C: use one event loop; configure named instances before init; listen, then start. Use semantic logging, express routing and "
        }
    ]
}
```

#### Exchange 186

Resource read · snodec-dev-http · Resource and template reads · snodec://development/style/test · EXPECTED ERROR

Resource and template reads · snodec://development/style/test · Capture: native exception

**Server classes** [`StyleTemplate`](#styletemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "snodec-dev-http",
        "uri": "snodec://development/style/test"
    }
}
```

**MCP → Codex response**

```json
{
    "error": "resources/read failed: resources/read failed for `snodec-dev-http` (snodec://development/style/test): Mcp error: -32602: Unknown style section"
}
```

#### Exchange 187

Resource read · snodec-dev-http · Style and selected-label verification · snodec://development/style/overview · PASS

Style and selected-label verification · snodec://development/style/overview · Capture: recorded resource projection

**Server classes** [`StyleTemplate`](#styletemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "snodec-dev-http",
        "uri": "snodec://development/style/overview"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "snodec://development/style/overview",
            "text": "SNode.C: use one event loop; configure named instances before init; listen, then start. Use semantic logging, express routing and existing CLI11 instance options. Ke"
        }
    ]
}
```

#### Exchange 188

Resource read · snodec-dev-stdio · Resource and template reads · snodec://development/house-style · PASS

Resource and template reads · snodec://development/house-style · Capture: recorded resource projection

**Server classes** [`HouseStyleResource`](#housestyleresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "snodec-dev-stdio",
        "uri": "snodec://development/house-style"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "snodec://development/house-style",
            "mimeType": "text/plain",
            "text": "SNode.C: use one event loop; configure named instances before init; listen, then start. Use semantic logging, express routing and "
        }
    ]
}
```

#### Exchange 191

Resource read · snodec-dev-stdio · Resource and template reads · snodec://development/style/test · EXPECTED ERROR

Resource and template reads · snodec://development/style/test · Capture: native exception

**Server classes** [`StyleTemplate`](#styletemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "snodec-dev-stdio",
        "uri": "snodec://development/style/test"
    }
}
```

**MCP → Codex response**

```json
{
    "error": "resources/read failed: resources/read failed for `snodec-dev-stdio` (snodec://development/style/test): Mcp error: -32602: Unknown style section"
}
```

#### Exchange 192

Resource read · snodec-dev-stdio · Style and selected-label verification · snodec://development/style/overview · PASS

Style and selected-label verification · snodec://development/style/overview · Capture: recorded resource projection

**Server classes** [`StyleTemplate`](#styletemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "snodec-dev-stdio",
        "uri": "snodec://development/style/overview"
    }
}
```

**MCP → Codex response**

```json
{
    "contents": [
        {
            "uri": "snodec://development/style/overview",
            "text": "SNode.C: use one event loop; configure named instances before init; listen, then start. Use semantic logging, express routing and existing CLI11 instance options. Ke"
        }
    ]
}
```

#### Exchange 233

Resource read · showcase-http · Read-only evidence capture · showcase://blob · PASS

Read-only evidence capture · showcase://blob · Capture: native json text

**Server classes** [`BlobResourceItem`](#blobresourceitem)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://blob"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://blob",
    "contents": [
        {
            "uri": "showcase://blob",
            "mimeType": "application/octet-stream",
            "blob": "AAEC"
        }
    ]
}
```

#### Exchange 234

Resource read · showcase-http · Read-only evidence capture · showcase://catalogue/0 · PASS

Read-only evidence capture · showcase://catalogue/0 · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/0"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://catalogue/0",
    "contents": [
        {
            "uri": "showcase://catalogue/0",
            "mimeType": "text/plain",
            "text": "catalogue.0"
        }
    ]
}
```

#### Exchange 235

Resource read · showcase-http · Read-only evidence capture · showcase://catalogue/1 · PASS

Read-only evidence capture · showcase://catalogue/1 · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/1"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://catalogue/1",
    "contents": [
        {
            "uri": "showcase://catalogue/1",
            "mimeType": "text/plain",
            "text": "catalogue.1"
        }
    ]
}
```

#### Exchange 236

Resource read · showcase-http · Read-only evidence capture · showcase://catalogue/2 · PASS

Read-only evidence capture · showcase://catalogue/2 · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/2"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://catalogue/2",
    "contents": [
        {
            "uri": "showcase://catalogue/2",
            "mimeType": "text/plain",
            "text": "catalogue.2"
        }
    ]
}
```

#### Exchange 237

Resource read · showcase-http · Read-only evidence capture · showcase://catalogue/3 · PASS

Read-only evidence capture · showcase://catalogue/3 · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/3"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://catalogue/3",
    "contents": [
        {
            "uri": "showcase://catalogue/3",
            "mimeType": "text/plain",
            "text": "catalogue.3"
        }
    ]
}
```

#### Exchange 238

Resource read · showcase-http · Read-only evidence capture · showcase://catalogue/4 · PASS

Read-only evidence capture · showcase://catalogue/4 · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/4"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://catalogue/4",
    "contents": [
        {
            "uri": "showcase://catalogue/4",
            "mimeType": "text/plain",
            "text": "catalogue.4"
        }
    ]
}
```

#### Exchange 239

Resource read · showcase-http · Read-only evidence capture · showcase://catalogue/5 · PASS

Read-only evidence capture · showcase://catalogue/5 · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/5"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://catalogue/5",
    "contents": [
        {
            "uri": "showcase://catalogue/5",
            "mimeType": "text/plain",
            "text": "catalogue.5"
        }
    ]
}
```

#### Exchange 240

Resource read · showcase-http · Read-only evidence capture · showcase://catalogue/6 · PASS

Read-only evidence capture · showcase://catalogue/6 · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/6"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://catalogue/6",
    "contents": [
        {
            "uri": "showcase://catalogue/6",
            "mimeType": "text/plain",
            "text": "catalogue.6"
        }
    ]
}
```

#### Exchange 241

Resource read · showcase-http · Read-only evidence capture · showcase://catalogue/7 · PASS

Read-only evidence capture · showcase://catalogue/7 · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/7"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://catalogue/7",
    "contents": [
        {
            "uri": "showcase://catalogue/7",
            "mimeType": "text/plain",
            "text": "catalogue.7"
        }
    ]
}
```

#### Exchange 242

Resource read · showcase-http · Read-only evidence capture · showcase://catalogue/8 · PASS

Read-only evidence capture · showcase://catalogue/8 · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/8"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://catalogue/8",
    "contents": [
        {
            "uri": "showcase://catalogue/8",
            "mimeType": "text/plain",
            "text": "catalogue.8"
        }
    ]
}
```

#### Exchange 243

Resource read · showcase-http · Read-only evidence capture · showcase://text · PASS

Read-only evidence capture · showcase://text · Capture: native json text

**Server classes** [`TextResourceItem`](#textresourceitem)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://text"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://text",
    "contents": [
        {
            "uri": "showcase://text",
            "mimeType": "text/plain",
            "text": "SNode.C\n",
            "_meta": {
                "showcase/content": true
            }
        }
    ],
    "_meta": {
        "showcase/result": true
    }
}
```

#### Exchange 244

Resource read · showcase-http · Read-only evidence capture · snodec://development/house-style · PASS

Read-only evidence capture · snodec://development/house-style · Capture: native json text

**Server classes** [`HouseStyleResource`](#housestyleresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "snodec://development/house-style"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "snodec://development/house-style",
    "contents": [
        {
            "uri": "snodec://development/house-style",
            "mimeType": "text/plain",
            "text": "SNode.C: use one event loop; configure named instances before init; listen, then start. Use semantic logging, express routing and existing CLI11 instance options. Keep protocol code independent of transports. Bound every handler's work. Stream with Response::pipe and finish through Source EOF. Never mix send/json with an open stream."
        }
    ]
}
```

#### Exchange 245

Resource read · showcase-stdio · Read-only evidence capture · showcase://blob · PASS

Read-only evidence capture · showcase://blob · Capture: native json text

**Server classes** [`BlobResourceItem`](#blobresourceitem)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://blob"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://blob",
    "contents": [
        {
            "uri": "showcase://blob",
            "mimeType": "application/octet-stream",
            "blob": "AAEC"
        }
    ]
}
```

#### Exchange 246

Resource read · showcase-stdio · Read-only evidence capture · showcase://catalogue/0 · PASS

Read-only evidence capture · showcase://catalogue/0 · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/0"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://catalogue/0",
    "contents": [
        {
            "uri": "showcase://catalogue/0",
            "mimeType": "text/plain",
            "text": "catalogue.0"
        }
    ]
}
```

#### Exchange 247

Resource read · showcase-stdio · Read-only evidence capture · showcase://catalogue/1 · PASS

Read-only evidence capture · showcase://catalogue/1 · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/1"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://catalogue/1",
    "contents": [
        {
            "uri": "showcase://catalogue/1",
            "mimeType": "text/plain",
            "text": "catalogue.1"
        }
    ]
}
```

#### Exchange 248

Resource read · showcase-stdio · Read-only evidence capture · showcase://catalogue/2 · PASS

Read-only evidence capture · showcase://catalogue/2 · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/2"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://catalogue/2",
    "contents": [
        {
            "uri": "showcase://catalogue/2",
            "mimeType": "text/plain",
            "text": "catalogue.2"
        }
    ]
}
```

#### Exchange 249

Resource read · showcase-stdio · Read-only evidence capture · showcase://catalogue/3 · PASS

Read-only evidence capture · showcase://catalogue/3 · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/3"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://catalogue/3",
    "contents": [
        {
            "uri": "showcase://catalogue/3",
            "mimeType": "text/plain",
            "text": "catalogue.3"
        }
    ]
}
```

#### Exchange 250

Resource read · showcase-stdio · Read-only evidence capture · showcase://catalogue/4 · PASS

Read-only evidence capture · showcase://catalogue/4 · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/4"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://catalogue/4",
    "contents": [
        {
            "uri": "showcase://catalogue/4",
            "mimeType": "text/plain",
            "text": "catalogue.4"
        }
    ]
}
```

#### Exchange 251

Resource read · showcase-stdio · Read-only evidence capture · showcase://catalogue/5 · PASS

Read-only evidence capture · showcase://catalogue/5 · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/5"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://catalogue/5",
    "contents": [
        {
            "uri": "showcase://catalogue/5",
            "mimeType": "text/plain",
            "text": "catalogue.5"
        }
    ]
}
```

#### Exchange 252

Resource read · showcase-stdio · Read-only evidence capture · showcase://catalogue/6 · PASS

Read-only evidence capture · showcase://catalogue/6 · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/6"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://catalogue/6",
    "contents": [
        {
            "uri": "showcase://catalogue/6",
            "mimeType": "text/plain",
            "text": "catalogue.6"
        }
    ]
}
```

#### Exchange 253

Resource read · showcase-stdio · Read-only evidence capture · showcase://catalogue/7 · PASS

Read-only evidence capture · showcase://catalogue/7 · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/7"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://catalogue/7",
    "contents": [
        {
            "uri": "showcase://catalogue/7",
            "mimeType": "text/plain",
            "text": "catalogue.7"
        }
    ]
}
```

#### Exchange 254

Resource read · showcase-stdio · Read-only evidence capture · showcase://catalogue/8 · PASS

Read-only evidence capture · showcase://catalogue/8 · Capture: native json text

**Server classes** [`CatalogueResource`](#catalogueresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/8"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://catalogue/8",
    "contents": [
        {
            "uri": "showcase://catalogue/8",
            "mimeType": "text/plain",
            "text": "catalogue.8"
        }
    ]
}
```

#### Exchange 255

Resource read · showcase-stdio · Read-only evidence capture · showcase://text · PASS

Read-only evidence capture · showcase://text · Capture: native json text

**Server classes** [`TextResourceItem`](#textresourceitem)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://text"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://text",
    "contents": [
        {
            "uri": "showcase://text",
            "mimeType": "text/plain",
            "text": "SNode.C\n",
            "_meta": {
                "showcase/content": true
            }
        }
    ],
    "_meta": {
        "showcase/result": true
    }
}
```

#### Exchange 256

Resource read · showcase-stdio · Read-only evidence capture · snodec://development/house-style · PASS

Read-only evidence capture · snodec://development/house-style · Capture: native json text

**Server classes** [`HouseStyleResource`](#housestyleresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "snodec://development/house-style"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "snodec://development/house-style",
    "contents": [
        {
            "uri": "snodec://development/house-style",
            "mimeType": "text/plain",
            "text": "SNode.C: use one event loop; configure named instances before init; listen, then start. Use semantic logging, express routing and existing CLI11 instance options. Keep protocol code independent of transports. Bound every handler's work. Stream with Response::pipe and finish through Source EOF. Never mix send/json with an open stream."
        }
    ]
}
```

#### Exchange 257

Resource read · snodec-dev-http · Read-only evidence capture · snodec://development/house-style · PASS

Read-only evidence capture · snodec://development/house-style · Capture: native json text

**Server classes** [`HouseStyleResource`](#housestyleresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "snodec-dev-http",
        "uri": "snodec://development/house-style"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "snodec-dev-http",
    "uri": "snodec://development/house-style",
    "contents": [
        {
            "uri": "snodec://development/house-style",
            "mimeType": "text/plain",
            "text": "SNode.C: use one event loop; configure named instances before init; listen, then start. Use semantic logging, express routing and existing CLI11 instance options. Keep protocol code independent of transports. Bound every handler's work. Stream with Response::pipe and finish through Source EOF. Never mix send/json with an open stream."
        }
    ]
}
```

#### Exchange 258

Resource read · snodec-dev-stdio · Read-only evidence capture · snodec://development/house-style · PASS

Read-only evidence capture · snodec://development/house-style · Capture: native json text

**Server classes** [`HouseStyleResource`](#housestyleresource)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "snodec-dev-stdio",
        "uri": "snodec://development/house-style"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "snodec-dev-stdio",
    "uri": "snodec://development/house-style",
    "contents": [
        {
            "uri": "snodec://development/house-style",
            "mimeType": "text/plain",
            "text": "SNode.C: use one event loop; configure named instances before init; listen, then start. Use semantic logging, express routing and existing CLI11 instance options. Keep protocol code independent of transports. Bound every handler's work. Stream with Response::pipe and finish through Source EOF. Never mix send/json with an open stream."
        }
    ]
}
```

#### Exchange 259

Resource read · showcase-http · Read-only evidence capture · showcase://catalogue/0/test · PASS

Read-only evidence capture · showcase://catalogue/0/test · Capture: native json text

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/0/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://catalogue/0/test",
    "contents": [
        {
            "uri": "showcase://catalogue/0/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 260

Resource read · showcase-http · Read-only evidence capture · showcase://catalogue/1/test · PASS

Read-only evidence capture · showcase://catalogue/1/test · Capture: native json text

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/1/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://catalogue/1/test",
    "contents": [
        {
            "uri": "showcase://catalogue/1/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 261

Resource read · showcase-http · Read-only evidence capture · showcase://catalogue/2/test · PASS

Read-only evidence capture · showcase://catalogue/2/test · Capture: native json text

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/2/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://catalogue/2/test",
    "contents": [
        {
            "uri": "showcase://catalogue/2/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 262

Resource read · showcase-http · Read-only evidence capture · showcase://catalogue/3/test · PASS

Read-only evidence capture · showcase://catalogue/3/test · Capture: native json text

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/3/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://catalogue/3/test",
    "contents": [
        {
            "uri": "showcase://catalogue/3/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 263

Resource read · showcase-http · Read-only evidence capture · showcase://catalogue/4/test · PASS

Read-only evidence capture · showcase://catalogue/4/test · Capture: native json text

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/4/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://catalogue/4/test",
    "contents": [
        {
            "uri": "showcase://catalogue/4/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 264

Resource read · showcase-http · Read-only evidence capture · showcase://catalogue/5/test · PASS

Read-only evidence capture · showcase://catalogue/5/test · Capture: native json text

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/5/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://catalogue/5/test",
    "contents": [
        {
            "uri": "showcase://catalogue/5/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 265

Resource read · showcase-http · Read-only evidence capture · showcase://catalogue/6/test · PASS

Read-only evidence capture · showcase://catalogue/6/test · Capture: native json text

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/6/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://catalogue/6/test",
    "contents": [
        {
            "uri": "showcase://catalogue/6/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 266

Resource read · showcase-http · Read-only evidence capture · showcase://catalogue/7/test · PASS

Read-only evidence capture · showcase://catalogue/7/test · Capture: native json text

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/7/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://catalogue/7/test",
    "contents": [
        {
            "uri": "showcase://catalogue/7/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 267

Resource read · showcase-http · Read-only evidence capture · showcase://catalogue/8/test · PASS

Read-only evidence capture · showcase://catalogue/8/test · Capture: native json text

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://catalogue/8/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://catalogue/8/test",
    "contents": [
        {
            "uri": "showcase://catalogue/8/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 268

Resource read · showcase-http · Read-only evidence capture · showcase://greeting/test · PASS

Read-only evidence capture · showcase://greeting/test · Capture: native json text

**Server classes** [`GreetingTemplate`](#greetingtemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://greeting/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://greeting/test",
    "contents": [
        {
            "uri": "showcase://greeting/test",
            "mimeType": "text/plain",
            "text": "Hello test"
        }
    ]
}
```

#### Exchange 269

Resource read · showcase-http · Read-only evidence capture · showcase://path/one/two · PASS

Read-only evidence capture · showcase://path/one/two · Capture: native json text

**Server classes** [`PathTemplate`](#pathtemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "showcase://path/one/two"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "showcase://path/one/two",
    "contents": [
        {
            "uri": "showcase://path/one/two",
            "mimeType": "text/plain",
            "text": "one/two"
        }
    ]
}
```

#### Exchange 270

Resource read · showcase-http · Read-only evidence capture · snodec://development/style/overview · PASS

Read-only evidence capture · snodec://development/style/overview · Capture: native json text

**Server classes** [`StyleTemplate`](#styletemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-http",
        "uri": "snodec://development/style/overview"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-http",
    "uri": "snodec://development/style/overview",
    "contents": [
        {
            "uri": "snodec://development/style/overview",
            "mimeType": "text/plain",
            "text": "SNode.C: use one event loop; configure named instances before init; listen, then start. Use semantic logging, express routing and existing CLI11 instance options. Keep protocol code independent of transports. Bound every handler's work. Stream with Response::pipe and finish through Source EOF. Never mix send/json with an open stream."
        }
    ]
}
```

#### Exchange 271

Resource read · showcase-stdio · Read-only evidence capture · showcase://catalogue/0/test · PASS

Read-only evidence capture · showcase://catalogue/0/test · Capture: native json text

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/0/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://catalogue/0/test",
    "contents": [
        {
            "uri": "showcase://catalogue/0/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 272

Resource read · showcase-stdio · Read-only evidence capture · showcase://catalogue/1/test · PASS

Read-only evidence capture · showcase://catalogue/1/test · Capture: native json text

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/1/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://catalogue/1/test",
    "contents": [
        {
            "uri": "showcase://catalogue/1/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 273

Resource read · showcase-stdio · Read-only evidence capture · showcase://catalogue/2/test · PASS

Read-only evidence capture · showcase://catalogue/2/test · Capture: native json text

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/2/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://catalogue/2/test",
    "contents": [
        {
            "uri": "showcase://catalogue/2/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 274

Resource read · showcase-stdio · Read-only evidence capture · showcase://catalogue/3/test · PASS

Read-only evidence capture · showcase://catalogue/3/test · Capture: native json text

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/3/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://catalogue/3/test",
    "contents": [
        {
            "uri": "showcase://catalogue/3/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 275

Resource read · showcase-stdio · Read-only evidence capture · showcase://catalogue/4/test · PASS

Read-only evidence capture · showcase://catalogue/4/test · Capture: native json text

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/4/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://catalogue/4/test",
    "contents": [
        {
            "uri": "showcase://catalogue/4/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 276

Resource read · showcase-stdio · Read-only evidence capture · showcase://catalogue/5/test · PASS

Read-only evidence capture · showcase://catalogue/5/test · Capture: native json text

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/5/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://catalogue/5/test",
    "contents": [
        {
            "uri": "showcase://catalogue/5/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 277

Resource read · showcase-stdio · Read-only evidence capture · showcase://catalogue/6/test · PASS

Read-only evidence capture · showcase://catalogue/6/test · Capture: native json text

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/6/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://catalogue/6/test",
    "contents": [
        {
            "uri": "showcase://catalogue/6/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 278

Resource read · showcase-stdio · Read-only evidence capture · showcase://catalogue/7/test · PASS

Read-only evidence capture · showcase://catalogue/7/test · Capture: native json text

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/7/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://catalogue/7/test",
    "contents": [
        {
            "uri": "showcase://catalogue/7/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 279

Resource read · showcase-stdio · Read-only evidence capture · showcase://catalogue/8/test · PASS

Read-only evidence capture · showcase://catalogue/8/test · Capture: native json text

**Server classes** [`CatalogueTemplate`](#cataloguetemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://catalogue/8/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://catalogue/8/test",
    "contents": [
        {
            "uri": "showcase://catalogue/8/test",
            "mimeType": "text/plain",
            "text": "test"
        }
    ]
}
```

#### Exchange 280

Resource read · showcase-stdio · Read-only evidence capture · showcase://greeting/test · PASS

Read-only evidence capture · showcase://greeting/test · Capture: native json text

**Server classes** [`GreetingTemplate`](#greetingtemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://greeting/test"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://greeting/test",
    "contents": [
        {
            "uri": "showcase://greeting/test",
            "mimeType": "text/plain",
            "text": "Hello test"
        }
    ]
}
```

#### Exchange 281

Resource read · showcase-stdio · Read-only evidence capture · showcase://path/one/two · PASS

Read-only evidence capture · showcase://path/one/two · Capture: native json text

**Server classes** [`PathTemplate`](#pathtemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "showcase://path/one/two"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "showcase://path/one/two",
    "contents": [
        {
            "uri": "showcase://path/one/two",
            "mimeType": "text/plain",
            "text": "one/two"
        }
    ]
}
```

#### Exchange 282

Resource read · showcase-stdio · Read-only evidence capture · snodec://development/style/overview · PASS

Read-only evidence capture · snodec://development/style/overview · Capture: native json text

**Server classes** [`StyleTemplate`](#styletemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "showcase-stdio",
        "uri": "snodec://development/style/overview"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "showcase-stdio",
    "uri": "snodec://development/style/overview",
    "contents": [
        {
            "uri": "snodec://development/style/overview",
            "mimeType": "text/plain",
            "text": "SNode.C: use one event loop; configure named instances before init; listen, then start. Use semantic logging, express routing and existing CLI11 instance options. Keep protocol code independent of transports. Bound every handler's work. Stream with Response::pipe and finish through Source EOF. Never mix send/json with an open stream."
        }
    ]
}
```

#### Exchange 283

Resource read · snodec-dev-http · Read-only evidence capture · snodec://development/style/overview · PASS

Read-only evidence capture · snodec://development/style/overview · Capture: native json text

**Server classes** [`StyleTemplate`](#styletemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "snodec-dev-http",
        "uri": "snodec://development/style/overview"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "snodec-dev-http",
    "uri": "snodec://development/style/overview",
    "contents": [
        {
            "uri": "snodec://development/style/overview",
            "mimeType": "text/plain",
            "text": "SNode.C: use one event loop; configure named instances before init; listen, then start. Use semantic logging, express routing and existing CLI11 instance options. Keep protocol code independent of transports. Bound every handler's work. Stream with Response::pipe and finish through Source EOF. Never mix send/json with an open stream."
        }
    ]
}
```

#### Exchange 284

Resource read · snodec-dev-stdio · Read-only evidence capture · snodec://development/style/overview · PASS

Read-only evidence capture · snodec://development/style/overview · Capture: native json text

**Server classes** [`StyleTemplate`](#styletemplate)

**Framework** [Resource dispatch](#resource-dispatch)

**Codex → MCP request**

```json
{
    "tool": "read_mcp_resource",
    "arguments": {
        "server": "snodec-dev-stdio",
        "uri": "snodec://development/style/overview"
    }
}
```

**MCP → Codex response · JSON text decoded**

```json
{
    "server": "snodec-dev-stdio",
    "uri": "snodec://development/style/overview",
    "contents": [
        {
            "uri": "snodec://development/style/overview",
            "mimeType": "text/plain",
            "text": "SNode.C: use one event loop; configure named instances before init; listen, then start. Use semantic logging, express routing and existing CLI11 instance options. Keep protocol code independent of transports. Bound every handler's work. Stream with Response::pipe and finish through Source EOF. Never mix send/json with an open stream."
        }
    ]
}
```

All recorded attempts including retries and captures · 18 client blocked, 26 expected error, 228 pass, 12 timeout

## C++ class cards

Open a card to read one class, jump to its helpers, or download its standalone C++ example. Each download includes the application definitions used by that class, with the SNode.C and system headers it needs.

[Supporting code](#supporting-code) · [Source audit](#source-fragment-audit)

Source checkout revision [`aa3d904fba0573d6be110aa2f6c6d5c2ef320177`](https://github.com/SNodeC/snode.c/tree/aa3d904fba0573d6be110aa2f6c6d5c2ef320177)

### Shared development registrations

> #### FindTool
>
> Registers api.find and supplies its limits to the shared header lookup.
>
> [Download standalone C++](class-examples/FindTool.cpp) **Uses** [`LookupMode`](#lookupmode) · [`Headers`](#headers) · [`DevelopmentConfig`](#developmentconfig) · [`openHeaders`](#openheaders) · [`lookup`](#lookup-function) · [`parseSchema`](#parseschema)
>
> ```cpp
> class FindTool : public ::mcp::server::Tool {
> public:
>     explicit FindTool(const DevelopmentConfig& config)
>         : Tool({"api.find",
>                 "Literal search of confined installed headers; returns relative paths, lines and byte offsets.",
>                 parseSchema("api.find", R"json(
> {
>     "type": "object",
>     "properties": {
>         "query": {
>             "type": "string",
>             "maxLength": 128,
>             "minLength": 1
>         },
>         "limit": {
>             "type": "integer",
>             "default": 20,
>             "maximum": 100,
>             "minimum": 1
>         }
>     },
>     "required": [
>         "query"
>     ],
>     "additionalProperties": false
> }
> )json")})
>         , headers(openHeaders(config.getHeaderRoot()))
>         , files(config.getSearchFiles())
>         , bytes(config.getSearchBytes()) {
>     }
>
>     void call(const ::mcp::server::ToolCall& request, const std::shared_ptr<::mcp::server::ToolReply>& reply) override {
>         lookup(headers, request, reply, LookupMode::Find, files, bytes);
>     }
>
> private:
>     std::shared_ptr<Headers> headers;
>     std::size_t files;
>     std::size_t bytes;
> };
> ```
>
> [FindTool.h · 60–70](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/FindTool.h#L60-L70) · [FindTool.cpp · 55–87](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/FindTool.cpp#L55-L87) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.cpp#L424)

> #### ReadTool
>
> Registers api.read and delegates a bounded read to the shared header lookup.
>
> [Download standalone C++](class-examples/ReadTool.cpp) **Uses** [`LookupMode`](#lookupmode) · [`Headers`](#headers) · [`DevelopmentConfig`](#developmentconfig) · [`openHeaders`](#openheaders) · [`lookup`](#lookup-function) · [`parseSchema`](#parseschema)
>
> ```cpp
> class ReadTool : public ::mcp::server::Tool {
> public:
>     explicit ReadTool(const DevelopmentConfig& config)
>         : Tool({"api.read",
>                 "Read a bounded UTF-8 header range using a relative path and optional byte offset.",
>                 parseSchema("api.read", R"json(
> {
>     "type": "object",
>     "properties": {
>         "path": {
>             "type": "string",
>             "maxLength": 512,
>             "minLength": 1
>         },
>         "offset": {
>             "type": "integer",
>             "maximum": 67108864,
>             "minimum": 0
>         },
>         "bytes": {
>             "type": "integer",
>             "maximum": 65536,
>             "minimum": 1
>         }
>     },
>     "required": [
>         "path"
>     ],
>     "additionalProperties": false
> }
> )json")})
>         , headers(openHeaders(config.getHeaderRoot())) {
>     }
>
>     void call(const ::mcp::server::ToolCall& request, const std::shared_ptr<::mcp::server::ToolReply>& reply) override {
>         lookup(headers, request, reply, LookupMode::Read, 1, 65536);
>     }
>
> private:
>     std::shared_ptr<Headers> headers;
> };
> ```
>
> [ReadTool.h · 59–67](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/ReadTool.h#L59-L67) · [ReadTool.cpp · 55–89](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/ReadTool.cpp#L55-L89) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.cpp#L425)

> #### SkeletonPreviewTool
>
> Returns source and CMake strings for an Express application or router module.
>
> [Download standalone C++](class-examples/SkeletonPreviewTool.cpp) **Uses** [`parseSchema`](#parseschema)
>
> ```cpp
> class SkeletonPreviewTool : public ::mcp::server::Tool {
> public:
>     SkeletonPreviewTool()
>         : Tool({"skeleton.preview",
>                 "Preview an express application or router module. Does not write files.",
>                 parseSchema("skeleton.preview", R"json(
> {
>     "type": "object",
>     "properties": {
>         "kind": {
>             "default": "application",
>             "enum": [
>                 "application",
>                 "module"
>             ]
>         }
>     },
>     "additionalProperties": false
> }
> )json")}) {
>     }
>
>     void call(const ::mcp::server::ToolCall& request, const std::shared_ptr<::mcp::server::ToolReply>& reply) override {
>         const auto& args = request.arguments;
>         const bool module = args.value("kind", std::string("application")) == "module";
>         ::mcp::server::Json files;
>         if (module) {
>             files = {
>                 {"src/example/CMakeLists.txt", "add_subdirectory(server)\n"},
>                 {"src/example/server/Router.h", R"preview(#pragma once
> #include <express/Router.h>
>
> namespace example::server {
>     class Router : public express::Router {
>     public:
>         Router();
>     };
> }
> )preview"},
>                 {"src/example/server/Router.cpp", R"preview(#include "example/server/Router.h"
> #include <express/Response.h>
>
> namespace example::server {
>     Router::Router() {
>         get("/", [](const auto&, const auto& response) {
>             response->send("SNode.C");
>         });
>     }
> }
> )preview"},
>                 {"src/example/server/CMakeLists.txt", R"preview(add_library(example-server SHARED Router.cpp Router.h)
> add_library(snodec::example-server ALIAS example-server)
> target_compile_features(example-server PUBLIC cxx_std_20)
> target_link_libraries(example-server PUBLIC http-server-express)
> target_include_directories(example-server
>     PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
>            "$<INSTALL_INTERFACE:include/snode.c>")
> set_target_properties(example-server PROPERTIES
>     VERSION ${SNode.C_VERSION} SOVERSION ${SNODEC_SOVERSION})
> install(TARGETS example-server EXPORT snodec_example-server_Targets
>     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/example
>     COMPONENT example-server)
> install(FILES Router.h
>     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/example/server
>     COMPONENT example-server)
> install(EXPORT snodec_example-server_Targets
>     FILE snodec_example-server_Targets.cmake NAMESPACE snodec::
>     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec COMPONENT example-server)
> )preview"}
>             };
>         } else {
>             files = {
>                 {"src/apps/example/main.cpp", R"preview(#include <express/legacy/in/WebApp.h>
> #include <express/Response.h>
>
> int main(int argc, char* argv[]) {
>     using WebApp = express::legacy::in::WebApp;
>     const WebApp app("example");
>     WebApp::init(argc, argv);
>     app.get("/", [](const auto&, const auto& response) {
>         response->send("SNode.C");
>     });
>     app.listen([](const auto&, const auto&) {});
>     return WebApp::start();
> }
> )preview"},
>                 {"src/apps/example/CMakeLists.txt", R"preview(add_executable(example main.cpp)
> target_compile_features(example PRIVATE cxx_std_20)
> target_link_libraries(example PRIVATE http-server-express net-in-stream-legacy)
> set_target_properties(example PROPERTIES
>     INSTALL_RPATH "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}")
> install(TARGETS example RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
>     COMPONENT apps)
> )preview"}
>             };
>         }
>
>         const ::mcp::server::Json value{
>             {"files", files},
>             {"notes", module ? "Add example to src/CMakeLists.txt and the central component/export inventory; review before writing."
>                     : "The apps directory discovers example/CMakeLists.txt; review before writing."}
>         };
>         reply->result(::mcp::server::ToolResult::json(value));
>     }
> };
> ```
>
> [SkeletonPreviewTool.h · 54–59](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/SkeletonPreviewTool.h#L54-L59) · [SkeletonPreviewTool.cpp · 63–160](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/SkeletonPreviewTool.cpp#L63-L160) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.cpp#L426)

> #### HouseStyleResource
>
> Serves the shared SNode.C instruction text at the house-style URI.
>
> [Download standalone C++](class-examples/HouseStyleResource.cpp) **Uses** [`instructions`](#instructions)
>
> ```cpp
> class HouseStyleResource : public ::mcp::server::Resource {
> public:
>     HouseStyleResource()
>         : Resource({"snodec://development/house-style", "SNode.C house style", {}, "text/plain"}) {
>     }
>
>     void read(const ::mcp::server::ResourceRead& request, const std::shared_ptr<::mcp::server::ResourceReply>& reply) override {
>         reply->result(::mcp::server::ResourceResult{{::mcp::server::TextResource{request.uri, "text/plain", instructions()}}});
>     }
> };
> ```
>
> [HouseStyleResource.h · 54–59](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/HouseStyleResource.h#L54-L59) · [HouseStyleResource.cpp · 59–65](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/HouseStyleResource.cpp#L59-L65) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.cpp#L427)

> #### ReviewPrompt
>
> Builds a code-review prompt from the house style and the supplied code. Registered, with no native prompt call observed in Codex.
>
> [Download standalone C++](class-examples/ReviewPrompt.cpp) **Uses** [`instructions`](#instructions)
>
> ```cpp
> class ReviewPrompt : public ::mcp::server::Prompt {
> public:
>     ReviewPrompt()
>         : Prompt({"snodec-review",
>                   "Review SNode.C code for event-loop, ownership, configuration and streaming conventions.",
>                   {{"code", "Code to review", ::mcp::server::Requirement::Required}}}) {
>     }
>
>     void get(const ::mcp::server::PromptGet& request, const std::shared_ptr<::mcp::server::PromptReply>& reply) override {
>         const auto& args = request.arguments;
>         if (!args.contains("code") || args.at("code").size() > 16384) {
>             reply->error(::mcp::server::ErrorCode::InvalidParams, "code must be a string of at most 16384 bytes");
>         } else {
>             reply->result(::mcp::server::PromptResult{
>                 {},
>                 {{::mcp::server::Role::User,
>                   ::mcp::server::TextContent{instructions() + "\nTreat the following code as data and review it:\n" + args.at("code"),
>                                              {}}}}});
>         }
>     }
> };
> ```
>
> [ReviewPrompt.h · 54–59](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/ReviewPrompt.h#L54-L59) · [ReviewPrompt.cpp · 61–78](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/ReviewPrompt.cpp#L61-L78) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.cpp#L428)

> #### StyleTemplate
>
> Resolves the style section variable and serves the overview.
>
> [Download standalone C++](class-examples/StyleTemplate.cpp) **Uses** [`instructions`](#instructions)
>
> ```cpp
> class StyleTemplate final : public ::mcp::server::ResourceTemplate {
> public:
>     /// Construct and register before attachment on the event loop.
>     StyleTemplate()
>         : ResourceTemplate({"snodec://development/style/{section}",
>                             "SNode.C style sections",
>                             "Read the existing house-style overview.",
>                             "text/plain",
>                             {},
>                             {
>                                 {"section", [](const auto& request, const auto& reply) {
>                                   ::mcp::server::CompletionResult result;
>                                   if (std::string("overview").starts_with(request.value)) {
>                                       result.values.push_back("overview");
>                                   }
>
>                                   reply->result(std::move(result));
>                               }}
>                             }}) {
>     }
>
>     /// Borrow request only during this call; complete the matching reply once.
>     void read(const ::mcp::server::ResourceRead& request, const std::shared_ptr<::mcp::server::ResourceReply>& reply) override {
>         if (request.variables.at("section") == "overview") {
>             reply->result(::mcp::server::ResourceResult{{::mcp::server::TextResource{request.uri, "text/plain", instructions()}}});
>         } else {
>             reply->error(::mcp::server::ErrorCode::InvalidParams, "Unknown style section");
>         }
>     }
> };
> ```
>
> [StyleTemplate.h · 51–58](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/StyleTemplate.h#L51-L58) · [StyleTemplate.cpp · 63–85](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/StyleTemplate.cpp#L63-L85) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.cpp#L429)

> #### ReviewLabelResource
>
> Owns the selected label and updates the resource when the value changes.
>
> [Download standalone C++](class-examples/ReviewLabelResource.cpp)
>
> ```cpp
> class ReviewLabelResource final : public ::mcp::server::Resource {
> public:
>     /// Borrow Server through this object's lifetime; construct on the event loop before attachment.
>     explicit ReviewLabelResource(::mcp::server::Server& server)
>         : Resource({"snodec://development/review-label",
>                     "Current review label",
>                     "The non-sensitive label selected with review.label.",
>                     "text/plain"})
>         , server(server) {
>     }
>
>     /// Replace the label on the event loop and notify subscribers only when it changes.
>     void setLabel(std::string next) {
>         if (label != next) {
>             label = std::move(next);
>             server.resourceUpdated(getDefinition().uri);
>         }
>     }
>
>     /// Complete once with the current owned label.
>     void read(const ::mcp::server::ResourceRead& request, const std::shared_ptr<::mcp::server::ResourceReply>& reply) override {
>         reply->result(::mcp::server::ResourceResult{{::mcp::server::TextResource{request.uri, "text/plain", label}}});
>     }
>
> private:
>     ::mcp::server::Server& server;
>     std::string label;
> };
> ```
>
> [ReviewLabelResource.h · 61–74](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/ReviewLabelResource.h#L61-L74) · [ReviewLabelResource.cpp · 59–76](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/ReviewLabelResource.cpp#L59-L76) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.cpp#L430-L431)

> #### ReviewLabelTool
>
> Requests a non-sensitive label, then updates the paired resource after acceptance.
>
> [Download standalone C++](class-examples/ReviewLabelTool.cpp) **Uses** [`parseSchema`](#parseschema) · [`ReviewLabelResource`](#reviewlabelresource)
>
> ```cpp
> class ReviewLabelTool final : public ::mcp::server::Tool {
> public:
>     /// Retain the resource through all requests; construct on the event loop before attachment.
>     explicit ReviewLabelTool(std::shared_ptr<ReviewLabelResource> resource)
>         : Tool({"review.label",
>                 "Choose a non-sensitive review label through elicitation. Does not modify files.",
>                 parseSchema("review.label", R"json(
> {
>     "type": "object",
>     "additionalProperties": false
> }
> )json")})
>         , resource(std::move(resource))
>         , formSchema(parseSchema("review.label",
>                                  R"json(
> {
>     "type": "object",
>     "properties": {
>         "label": {
>             "type": "string",
>             "maxLength": 128
>         }
>     },
>     "required": [
>         "label"
>     ]
> }
> )json",
>                                  "requestedSchema")) {
>     }
>
>     /// Borrow request during this call; complete once or request additional form input.
>     void call(const ::mcp::server::ToolCall& request, const std::shared_ptr<::mcp::server::ToolReply>& reply) override {
>         const auto& inputs = request.context.getInputResponses();
>         const auto* input = request.context.getInputResponse("label", formSchema);
>         if (inputs.empty()) {
>             reply->inputRequired(::mcp::server::InputRequired{
>                 {
>                     {"label", ::mcp::server::FormInput{"Choose a non-sensitive review label.", formSchema}}
>                 }, {}});
>         } else if (input == nullptr || input->action != ::mcp::server::ElicitationAction::Accept) {
>             reply->result(::mcp::server::ToolResult::text("No label selected."));
>         } else {
>             const auto& content = input->content;
>             const auto value = content.is_object() ? content.value("label", ::mcp::server::Json()) : ::mcp::server::Json();
>             if (!value.is_string() || value.get_ref<const std::string&>().size() > 128) {
>                 reply->error(::mcp::server::ErrorCode::InvalidParams, "label must be a string of at most 128 bytes");
>             } else {
>                 resource->setLabel(value.get<std::string>());
>                 reply->result(::mcp::server::ToolResult::text("Review label selected."));
>             }
>         }
>     }
>
> private:
>     std::shared_ptr<ReviewLabelResource> resource;
>     ::mcp::server::Json formSchema;
> };
> ```
>
> [ReviewLabelTool.h · 64–75](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/ReviewLabelTool.h#L64-L75) · [ReviewLabelTool.cpp · 61–107](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/ReviewLabelTool.cpp#L61-L107) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.cpp#L432)

### Showcase content

> #### TextTool
>
> Returns the showcase greeting.
>
> [Download standalone C++](class-examples/TextTool.cpp) **Uses** [`icon`](#icon)
>
> ```cpp
> class TextTool final : public Tool {
> public:
>     TextTool()
>         : Tool({.name = "text",
>                 .description = "Plain text results and tool annotations.",
>                 .inputSchema = Json::parse(R"json(
> {
>     "type": "object",
>     "additionalProperties": false
> }
> )json"),
>                 .title = "text",
>                 .annotations =
>                     Json::parse(R"json(
> {
>     "readOnlyHint": true,
>     "destructiveHint": false,
>     "idempotentHint": true,
>     "openWorldHint": false
> }
> )json"),
>                 .icons = {icon()},
>                 .meta = Json::parse(R"json(
> {
>     "showcase/feature": "text"
> }
> )json")}) {
>     }
>
>     void call(const ToolCall&, const std::shared_ptr<ToolReply>& reply) override {
>         reply->result(ToolResult::text("Hello from the MCP showcase.").setMeta({
>             {"showcase/result", "text"}
>         }));
>     }
> };
> ```
>
> [FeatureContent.cpp · 113–122](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureContent.cpp#L113-L122) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureContent.cpp#L311)

> #### ToolErrorTool
>
> Returns a deliberate tool-level failure.
>
> [Download standalone C++](class-examples/ToolErrorTool.cpp) **Uses** [`icon`](#icon)
>
> ```cpp
> class ToolErrorTool final : public Tool {
> public:
>     ToolErrorTool()
>         : Tool({.name = "tool.error",
>                 .description = "A tool failure: isError, not a JSON-RPC envelope error.",
>                 .inputSchema = Json::parse(R"json(
> {
>     "type": "object",
>     "additionalProperties": false
> }
> )json"),
>                 .title = "tool.error",
>                 .annotations =
>                     Json::parse(R"json(
> {
>     "readOnlyHint": true,
>     "destructiveHint": false,
>     "idempotentHint": true,
>     "openWorldHint": false
> }
> )json"),
>                 .icons = {icon()},
>                 .meta = Json::parse(R"json(
> {
>     "showcase/feature": "tool.error"
> }
> )json")}) {
>     }
>
>     void call(const ToolCall&, const std::shared_ptr<ToolReply>& reply) override {
>         reply->result(ToolResult::toolError("Deliberate tool failure."));
>     }
> };
> ```
>
> [FeatureContent.cpp · 124–133](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureContent.cpp#L124-L133) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureContent.cpp#L312)

> #### ProtocolErrorTool
>
> Completes the call with a deliberate JSON-RPC InvalidParams error.
>
> [Download standalone C++](class-examples/ProtocolErrorTool.cpp) **Uses** [`icon`](#icon)
>
> ```cpp
> class ProtocolErrorTool final : public Tool {
> public:
>     ProtocolErrorTool()
>         : Tool({.name = "protocol.error",
>                 .description = "An explicit JSON-RPC InvalidParams error.",
>                 .inputSchema = Json::parse(R"json(
> {
>     "type": "object",
>     "additionalProperties": false
> }
> )json"),
>                 .title = "protocol.error",
>                 .annotations =
>                     Json::parse(R"json(
> {
>     "readOnlyHint": true,
>     "destructiveHint": false,
>     "idempotentHint": true,
>     "openWorldHint": false
> }
> )json"),
>                 .icons = {icon()},
>                 .meta = Json::parse(R"json(
> {
>     "showcase/feature": "protocol.error"
> }
> )json")}) {
>     }
>
>     void call(const ToolCall&, const std::shared_ptr<ToolReply>& reply) override {
>         reply->error(ErrorCode::InvalidParams, "Deliberate protocol error.", {
>             {"feature", "errors"}
>         });
>     }
> };
> ```
>
> [FeatureContent.cpp · 135–144](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureContent.cpp#L135-L144) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureContent.cpp#L313)

> #### StructuredTool
>
> Provides the valid and invalid variants of the structured-answer example.
>
> [Download standalone C++](class-examples/StructuredTool.cpp) **Uses** [`icon`](#icon)
>
> ```cpp
> class StructuredTool final : public Tool {
> public:
>     StructuredTool(bool valid)
>         : Tool({.name = valid ? "structured.valid" : "structured.invalid",
>                 .description = "Output-schema enforcement and structuredContent.",
>                 .inputSchema = Json::parse(R"json(
> {
>     "type": "object",
>     "additionalProperties": false
> }
> )json"),
>                 .outputSchema = Json::parse(R"json(
> {
>     "type": "object",
>     "properties": {
>         "answer": {
>             "type": "integer",
>             "const": 42
>         }
>     },
>     "required": [
>         "answer"
>     ],
>     "additionalProperties": false
> }
> )json"),
>                 .title = valid ? "structured.valid" : "structured.invalid",
>                 .annotations =
>                     Json::parse(R"json(
> {
>     "readOnlyHint": true,
>     "destructiveHint": false,
>     "idempotentHint": true,
>     "openWorldHint": false
> }
> )json"),
>                 .icons = {icon()},
>                 .meta = {
>                     {"showcase/feature", valid ? "structured.valid" : "structured.invalid"}
>                 }})
>         , valid(valid) {
>     }
>
>     void call(const ToolCall&, const std::shared_ptr<ToolReply>& reply) override {
>         reply->result(ToolResult::json({
>             {"answer", valid ? 42 : 0}
>         }));
>     }
>
> private:
>     bool valid;
> };
> ```
>
> [FeatureContent.cpp · 167–180](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureContent.cpp#L167-L180) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureContent.cpp#L314-L315)

> #### SchemaTool
>
> Defines the referenced input schema and echoes the accepted arguments.
>
> [Download standalone C++](class-examples/SchemaTool.cpp) **Uses** [`icon`](#icon)
>
> ```cpp
> class SchemaTool final : public Tool {
> public:
>     SchemaTool()
>         : Tool({.name = "schema",
>                 .description = "JSON Schema 2020-12: refs, oneOf, const/enum, Unicode and unevaluatedProperties.",
>                 .inputSchema = Json::parse(R"json(
> {
>     "$schema": "https://json-schema.org/draft/2020-12/schema",
>     "type": "object",
>     "$defs": {
>         "word": {
>             "type": "string",
>             "pattern": "^\\p{L}+$"
>         }
>     },
>     "properties": {
>         "word": {
>             "$ref": "#/$defs/word"
>         },
>         "choice": {
>             "oneOf": [
>                 {
>                     "const": "alpha"
>                 },
>                 {
>                     "type": "integer",
>                     "enum": [
>                         1,
>                         2
>                     ]
>                 }
>             ]
>         }
>     },
>     "required": [
>         "word",
>         "choice"
>     ],
>     "unevaluatedProperties": false
> }
> )json"),
>                 .title = "schema",
>                 .annotations =
>                     Json::parse(R"json(
> {
>     "readOnlyHint": true,
>     "destructiveHint": false,
>     "idempotentHint": true,
>     "openWorldHint": false
> }
> )json"),
>                 .icons = {icon()},
>                 .meta = Json::parse(R"json(
> {
>     "showcase/feature": "schema"
> }
> )json")}) {
>     }
>
>     void call(const ToolCall& call, const std::shared_ptr<ToolReply>& reply) override {
>         reply->result(ToolResult::json(call.arguments));
>     }
> };
> ```
>
> [FeatureContent.cpp · 182–227](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureContent.cpp#L182-L227) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureContent.cpp#L316)

> #### HeadersTool
>
> Declares the region argument and its HTTP header mirror.
>
> [Download standalone C++](class-examples/HeadersTool.cpp) **Uses** [`icon`](#icon)
>
> ```cpp
> class HeadersTool final : public Tool {
> public:
>     HeadersTool()
>         : Tool({.name = "headers",
>                 .description = "2026 HTTP x-mcp-header mirroring (ordinary argument in 2025 and stdio).",
>                 .inputSchema = Json::parse(R"json(
> {
>     "type": "object",
>     "properties": {
>         "region": {
>             "type": "string",
>             "x-mcp-header": "Region"
>         }
>     },
>     "required": [
>         "region"
>     ],
>     "additionalProperties": false
> }
> )json"),
>                 .title = "headers",
>                 .annotations =
>                     Json::parse(R"json(
> {
>     "readOnlyHint": true,
>     "destructiveHint": false,
>     "idempotentHint": true,
>     "openWorldHint": false
> }
> )json"),
>                 .icons = {icon()},
>                 .meta = Json::parse(R"json(
> {
>     "showcase/feature": "headers"
> }
> )json")}) {
>     }
>
>     void call(const ToolCall& call, const std::shared_ptr<ToolReply>& reply) override {
>         reply->result(ToolResult::json(call.arguments));
>     }
> };
> ```
>
> [FeatureContent.cpp · 229–252](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureContent.cpp#L229-L252) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureContent.cpp#L317)

> #### ContentTool
>
> Produces text, image, audio, a resource link and embedded text and binary content.
>
> [Download standalone C++](class-examples/ContentTool.cpp) **Uses** [`encoded`](#encoded) · [`icon`](#icon)
>
> ```cpp
> class ContentTool final : public Tool {
> public:
>     ContentTool()
>         : Tool({.name = "content",
>                 .description = "Every content variant: text, image, audio, resource link and embedded text/blob.",
>                 .inputSchema = Json::parse(R"json(
> {
>     "type": "object",
>     "additionalProperties": false
> }
> )json"),
>                 .title = "content",
>                 .annotations =
>                     Json::parse(R"json(
> {
>     "readOnlyHint": true,
>     "destructiveHint": false,
>     "idempotentHint": true,
>     "openWorldHint": false
> }
> )json"),
>                 .icons = {icon()},
>                 .meta = Json::parse(R"json(
> {
>     "showcase/feature": "content"
> }
> )json")}) {
>     }
>
>     void call(const ToolCall&, const std::shared_ptr<ToolReply>& reply) override {
>         const Annotations annotation{{Role::User, Role::Assistant}, 0.5, "2026-01-01T00:00:00Z"};
>         const std::string svg =
>             "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1\" height=\"1\"><rect width=\"1\" height=\"1\"/></svg>";
>         std::string wav = "RIFF";
>         const auto little = [&wav](std::uint32_t value, unsigned width) {
>             for (unsigned i = 0; i < width; ++i) {
>                 wav.push_back(static_cast<char>((value >> (8U * i)) & 255U));
>             }
>         };
>         little(38, 4);
>         wav += "WAVEfmt ";
>         little(16, 4);
>         little(1, 2);
>         little(1, 2);
>         little(8000, 4);
>         little(16000, 4);
>         little(2, 2);
>         little(16, 2);
>         wav += "data";
>         little(2, 4);
>         little(0, 2);
>         reply->result(ToolResult::content(
>             {TextContent{"Original generated media; one silent audio sample.", annotation},
>              ImageContent{encoded(svg), "image/svg+xml", annotation},
>              AudioContent{encoded(wav), "audio/wav", annotation},
>              ResourceLink{
>                  "showcase://text", "Showcase text", "A readable resource", "text/plain", 8, annotation, {}, "Text", {icon()}},
>              EmbeddedResource{TextResource{"showcase://embedded/text", "text/plain", "embedded"}, annotation},
>              EmbeddedResource{BlobResource{"showcase://embedded/blob", "application/octet-stream", encoded("SNode.C")},
>                               annotation}}));
>     }
> };
> ```
>
> [FeatureContent.cpp · 254–293](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureContent.cpp#L254-L293) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureContent.cpp#L318)

> #### PrincipalTool
>
> Reports the application-authorized identity obtained from the request context.
>
> [Download standalone C++](class-examples/PrincipalTool.cpp) **Uses** [`icon`](#icon)
>
> ```cpp
> class PrincipalTool final : public Tool {
> public:
>     PrincipalTool()
>         : Tool({.name = "principal",
>                 .description = "Application-authorized identity from RequestContext, never from arguments.",
>                 .inputSchema = Json::parse(R"json(
> {
>     "type": "object",
>     "additionalProperties": false
> }
> )json"),
>                 .title = "principal",
>                 .annotations =
>                     Json::parse(R"json(
> {
>     "readOnlyHint": true,
>     "destructiveHint": false,
>     "idempotentHint": true,
>     "openWorldHint": false
> }
> )json"),
>                 .icons = {icon()},
>                 .meta = Json::parse(R"json(
> {
>     "showcase/feature": "principal"
> }
> )json")}) {
>     }
>
>     void call(const ToolCall& call, const std::shared_ptr<ToolReply>& reply) override {
>         const auto& principal = call.context.getPrincipal();
>         reply->result(ToolResult::json({
>             {"subject", principal ? principal->subject : "anonymous"}
>         }));
>     }
> };
> ```
>
> [FeatureContent.cpp · 295–306](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureContent.cpp#L295-L306) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureContent.cpp#L319)

### Showcase interactions

> #### WorkTool
>
> Registers ordinary, optional-task and required-task work. It delegates execution to the work helper.
>
> [Download standalone C++](class-examples/WorkTool.cpp) **Uses** [`icon`](#icon) · [`Work`](#work)
>
> ```cpp
> class WorkTool final : public Tool {
> public:
>     WorkTool(const std::string& name, TaskSupport support)
>         : Tool({.name = name,
>                 .description = "Bounded asynchronous work: progress, cancellation, deadline and task lifecycle.",
>                 .inputSchema = Json::parse(R"json(
> {
>     "type": "object",
>     "properties": {
>         "steps": {
>             "type": "integer",
>             "minimum": 1,
>             "maximum": 20
>         },
>         "delayMs": {
>             "type": "integer",
>             "minimum": 10,
>             "maximum": 1000
>         }
>     },
>     "additionalProperties": false
> }
> )json"),
>                 .title = name,
>                 .annotations =
>                     Json::parse(R"json(
> {
>     "readOnlyHint": true,
>     "destructiveHint": false,
>     "idempotentHint": true,
>     "openWorldHint": false
> }
> )json"),
>                 .icons = {icon()},
>                 .meta = {
>                     {"showcase/feature", name}
>                 },
>                 .taskSupport = support}) {
>     }
>
>     void call(const ToolCall& call, const std::shared_ptr<ToolReply>& reply) override {
>         if (getDefinition().taskSupport != TaskSupport::Forbidden) {
>             reply->startTask(TaskOptions{5000, 20});
>         }
>         reply->taskStatus(::mcp::TaskState::Working, "Running bounded demonstration work");
>         const auto work = std::make_shared<Work>(reply, call.arguments.value("steps", 5U), call.arguments.value("delayMs", 50U));
>         work->start(call.context);
>     }
> };
> ```
>
> [FeatureInteractions.cpp · 143–157](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L143-L157) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L535-L537)

> #### InputTool
>
> Implements the five registered input modes through the same class.
>
> [Download standalone C++](class-examples/InputTool.cpp) **Uses** [`icon`](#icon) · [`labelSchema`](#labelschema) · [`actionName`](#actionname) · [`inputResult`](#inputresult)
>
> ```cpp
> class InputTool final : public Tool {
> public:
>     explicit InputTool(std::string mode)
>         : Tool({.name = "input." + mode,
>                 .description = "Elicitation: non-sensitive form, URL or two-round continuation.",
>                 .inputSchema = mode == "url" ? Json::parse(R"json(
> {
>     "type": "object",
>     "additionalProperties": false,
>     "properties": {
>         "required": {
>             "type": "boolean"
>         }
>     }
> }
> )json")
>                                              : Json::parse(R"json(
> {
>     "type": "object",
>     "additionalProperties": false
> }
> )json"),
>                 .title = "input." + mode,
>                 .annotations =
>                     Json::parse(R"json(
> {
>     "readOnlyHint": true,
>     "destructiveHint": false,
>     "idempotentHint": true,
>     "openWorldHint": false
> }
> )json"),
>                 .icons = {icon()},
>                 .meta = {
>                     {"showcase/feature", "input." + mode}
>                 },
>                 .taskSupport = mode == "task" ? TaskSupport::Required : TaskSupport::Forbidden})
>         , mode(std::move(mode))
>         , form(labelSchema()) {
>     }
>
>     void call(const ToolCall& call, const std::shared_ptr<ToolReply>& reply) override {
>         if (mode == "task" && !call.context.getTaskId()) {
>             reply->startTask(TaskOptions{5000, 20});
>         }
>         const auto& state = call.context.getRequestState();
>         const auto& inputs = call.context.getInputResponses();
>         if (mode == "url" && call.arguments.value("required", false) &&
>             call.context.getProtocolEra() == ::mcp::ProtocolEra::Initialization20251125) {
>             reply->error(ErrorCode::UrlElicitationRequired,
>                          "Complete the demonstration URL before retrying.",
>                          {
>                              {"elicitations", {{{"mode", "url"},
>                              {"message", "Demonstration acknowledgement only."},
>                              {"url", "https://example.com/"},
>                              {"elicitationId", utils::Uuid::getUuid()}}}}
>                          });
>         } else if (inputs.empty()) {
>             InputRequired input{{
>                 {"first", FormInput{"Enter a non-sensitive label.", form}}
>             }, "first"};
>             if (mode == "parallel") {
>                 input.requests["second"] = FormInput{"Enter a second independent label.", form};
>             }
>             if (mode == "url") {
>                 input.requests["first"] =
>                     UrlInput{"Acknowledge this demonstration URL; no login is performed.", "https://example.com/"};
>             }
>             reply->inputRequired(std::move(input));
>         } else if (mode == "rounds" && state == std::optional<std::string>{"first"}) {
>             const auto* first = call.context.getInputResponse("first", form);
>             if (first != nullptr && first->action == ElicitationAction::Accept) {
>                 reply->inputRequired(InputRequired{{
>                     {"second", FormInput{"Confirm with a second label.", form}}
>                 }, "second"});
>             } else {
>                 reply->result(ToolResult::json(inputResult(first)));
>             }
>         } else if (mode == "parallel") {
>             reply->result(ToolResult::json({
>                 {"first", inputResult(call.context.getInputResponse("first", form))},
>                 {"second", inputResult(call.context.getInputResponse("second", form))}
>             }));
>         } else {
>             const std::string key = mode == "rounds" ? "second" : "first";
>             const auto* response = mode == "url" ? &inputs.at(key) : call.context.getInputResponse(key, form);
>             Json value{
>                 {"action", actionName(response)},
>                 {"state", state.value_or("")}
>             };
>             if (mode == "url") {
>                 const auto id = call.context.getElicitationId(key);
>                 value["completionNotified"] =
>                     id && response->action == ElicitationAction::Accept && call.context.notifyElicitationComplete(*id);
>             }
>             if (mode != "url" && response != nullptr && response->action == ElicitationAction::Accept) {
>                 value["label"] = response->content.at("label");
>             }
>             reply->result(ToolResult::json(std::move(value)));
>         }
>     }
>
> private:
>     std::string mode;
>     Json form;
> };
> ```
>
> [FeatureInteractions.cpp · 206–268](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L206-L268) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L538-L540)

> #### InputResource
>
> Requests a form while reading showcase://input and returns the resulting action.
>
> [Download standalone C++](class-examples/InputResource.cpp) **Uses** [`labelSchema`](#labelschema) · [`inputResult`](#inputresult) · [`labelRequest`](#labelrequest)
>
> ```cpp
> class InputResource final : public Resource {
> public:
>     InputResource()
>         : Resource({"showcase://input", "input", "Resource-originated elicitation and input_required.", "application/json"})
>         , form(labelSchema()) {
>     }
>
>     void read(const ResourceRead& call, const std::shared_ptr<ResourceReply>& reply) override {
>         const auto* answer = call.context.getInputResponse("first", form);
>         if (answer == nullptr) {
>             reply->inputRequired(labelRequest("Choose a non-sensitive label for this resource read."));
>         } else {
>             reply->result(ResourceResult{{TextResource{call.uri, "application/json", inputResult(answer).dump(4)}}});
>         }
>     }
>
> private:
>     Json form;
> };
> ```
>
> [FeatureInteractions.cpp · 270–288](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L270-L288) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L541)

> #### InputPrompt
>
> Requests a form during prompt retrieval. Registered, with no native prompt call observed in Codex.
>
> [Download standalone C++](class-examples/InputPrompt.cpp) **Uses** [`labelSchema`](#labelschema) · [`inputResult`](#inputresult) · [`labelRequest`](#labelrequest)
>
> ```cpp
> class InputPrompt final : public Prompt {
> public:
>     InputPrompt()
>         : Prompt({"input", "Prompt-originated elicitation and input_required."})
>         , form(labelSchema()) {
>     }
>
>     void get(const PromptGet& call, const std::shared_ptr<PromptReply>& reply) override {
>         const auto* answer = call.context.getInputResponse("first", form);
>         if (answer == nullptr) {
>             reply->inputRequired(labelRequest("Choose a non-sensitive label for this prompt."));
>         } else {
>             reply->result(PromptResult{{}, {{Role::User, TextContent{inputResult(answer).dump(4), {}}}}});
>         }
>     }
>
> private:
>     Json form;
> };
> ```
>
> [FeatureInteractions.cpp · 290–308](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L290-L308) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L542)

> #### LoggingTool
>
> Reports whether logging was admitted for the request.
>
> [Download standalone C++](class-examples/LoggingTool.cpp) **Uses** [`icon`](#icon)
>
> ```cpp
> class LoggingTool final : public Tool {
> public:
>     explicit LoggingTool(Server& server)
>         : Tool({.name = "logging",
>                 .description = "Session logging in 2025 and opt-in request logging in 2026.",
>                 .inputSchema = Json::parse(R"json(
> {
>     "type": "object",
>     "additionalProperties": false
> }
> )json"),
>                 .title = "logging",
>                 .annotations =
>                     Json::parse(R"json(
> {
>     "readOnlyHint": true,
>     "destructiveHint": false,
>     "idempotentHint": true,
>     "openWorldHint": false
> }
> )json"),
>                 .icons = {icon()},
>                 .meta = Json::parse(R"json(
> {
>     "showcase/feature": "logging"
> }
> )json")})
>         , server(server) {
>     }
>
>     void call(const ToolCall& call, const std::shared_ptr<ToolReply>& reply) override {
>         const bool admitted = call.context.log(LogLevel::Info, {
>             {"source", "handler"}
>         }, "showcase");
>         server.log(LogLevel::Warning, {
>             {"source", "server"}
>         }, "showcase");
>         reply->result(ToolResult::json({
>             {"requestLogAdmitted", admitted}
>         }));
>     }
>
> private:
>     Server& server;
> };
> ```
>
> [FeatureInteractions.cpp · 310–325](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L310-L325) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L543)

> #### SamplingTool
>
> Requests client sampling with selected media, options and tool-loop behavior.
>
> [Download standalone C++](class-examples/SamplingTool.cpp) **Uses** [`icon`](#icon) · [`clientInputResult`](#clientinputresult)
>
> ```cpp
> class SamplingTool final : public Tool {
> public:
>     SamplingTool()
>         : Tool({.name = "sampling",
>                 .description = "Client-controlled sampling, media, context, model options and a complete tool loop.",
>                 .inputSchema = Json::parse(R"json(
> {
>     "type": "object",
>     "properties": {
>         "toolUse": {
>             "type": "boolean"
>         },
>         "toolLoop": {
>             "type": "boolean"
>         },
>         "toolChoice": {
>             "enum": [
>                 "auto",
>                 "none",
>                 "required"
>             ]
>         },
>         "context": {
>             "enum": [
>                 "none",
>                 "thisServer",
>                 "allServers"
>             ]
>         },
>         "media": {
>             "enum": [
>                 "text",
>                 "image",
>                 "audio"
>             ]
>         },
>         "options": {
>             "type": "boolean"
>         }
>     }
> }
> )json"),
>                 .title = "sampling",
>                 .annotations =
>                     Json::parse(R"json(
> {
>     "readOnlyHint": true,
>     "destructiveHint": false,
>     "idempotentHint": true,
>     "openWorldHint": false
> }
> )json"),
>                 .icons = {icon()},
>                 .meta = Json::parse(R"json(
> {
>     "showcase/feature": "sampling"
> }
> )json")}) {
>     }
>
>     void call(const ToolCall& request, const std::shared_ptr<ToolReply>& reply) final {
>         request.context.trackClientProgress("sample");
>         request.context.trackClientProgress("final");
>         Json content{
>             {"type", "text"},
>             {"text", "Return the demonstration answer 42."}
>         };
>         const std::string media = request.arguments.value("media", "text");
>         if (media != "text") {
>             content = {
>                 {"type", media},
>                 {"data", media == "image"
>                             ? "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+ip1sAAAAASUVORK5CYII="
>                             : "UklGRiYAAABXQVZFZm10IBAAAAABAAEARKwAAIhYAQACABAAZGF0YQIAAAAAAA=="},
>                 {"mimeType", media == "image" ? "image/png" : "audio/wav"}
>             };
>         }
>         Json params{
>             {"messages", {{{"role", "user"}, {"content", content}}}},
>             {"maxTokens", 64},
>             {"includeContext", request.arguments.value("context", "none")}
>         };
>         const bool loop = request.arguments.value("toolLoop", false);
>         if (request.arguments.value("toolUse", false) || loop || request.arguments.contains("toolChoice")) {
>             params["tools"] = {
>                 {
>                     {"name", "answer"},
>                     {"inputSchema", Json{{"type", "object"}, {"additionalProperties", false}}},
>                     {"outputSchema", {
>                         {"type", "object"},
>                         {"properties", {
>                             {"answer", {
>                                 {"type", "integer"}
>                             }}
>                         }},
>                         {"required", {"answer"}}
>                     }}
>                 }};
>             params["toolChoice"] = {
>                 {"mode", loop ? "required" : request.arguments.value("toolChoice", "required")}
>             };
>         }
>         if (request.arguments.value("options", false)) {
>             params["systemPrompt"] = "Demonstration only; do not access external data.";
>             params["temperature"] = 0.25;
>             params["stopSequences"] = {"END"};
>             params["modelPreferences"] = {
>                 {"hints", {{{"name", "demonstration"}}}},
>                 {"costPriority", 0.5},
>                 {"speedPriority", 0.5},
>                 {"intelligencePriority", 0.5}
>             };
>             params["metadata"] = {
>                 {"showcase", true}
>             };
>         }
>         const ::mcp::SamplingRequest sampling(params);
>         const auto& state = request.context.getRequestState();
>         const auto firstResult = loop && state && state->starts_with("{")
>                                      ? std::optional<::mcp::SamplingResult>(::mcp::SamplingResult(Json::parse(*state)))
>                                      : request.context.getSamplingResponse("sample", sampling);
>         if (loop && firstResult) {
>             const Json first = firstResult->toJson();
>             params["messages"].push_back({
>                 {"role", "assistant"},
>                 {"content", first.at("content")}
>             });
>             const Json blocks = first.at("content").is_array() ? first.at("content") : Json::array({first.at("content")});
>             Json results = Json::array();
>             for (const auto& block : blocks) {
>                 if (block.at("type") == "tool_use") {
>                     results.push_back({
>                         {"type", "tool_result"},
>                         {"toolUseId", block.at("id")},
>                         {"content", {{{"type", "text"}, {"text", "42"}}}},
>                         {"structuredContent", {
>                             {"answer", 42}
>                         }}
>                     });
>                 }
>             }
>             params["messages"].push_back({
>                 {"role", "user"},
>                 {"content", results}
>             });
>             params["toolChoice"] = {
>                 {"mode", "none"}
>             };
>             const ::mcp::SamplingRequest final(params);
>             if (const auto response = request.context.getSamplingResponse("final", final)) {
>                 reply->result(clientInputResult(request.context, "final", response->toJson()));
>             } else {
>                 reply->clientInputRequired({{
>                     {"final", final}
>                 }, first.dump(4)});
>             }
>         } else if (firstResult) {
>             reply->result(clientInputResult(request.context, "sample", firstResult->toJson()));
>         } else {
>             reply->clientInputRequired({{
>                 {"sample", sampling}
>             }, "sampling"});
>         }
>     }
> };
> ```
>
> [FeatureInteractions.cpp · 338–420](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L338-L420) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L531)

> #### ClientTaskTool
>
> Exercises client-owned task creation and lifecycle controls when supported.
>
> [Download standalone C++](class-examples/ClientTaskTool.cpp) **Uses** [`icon`](#icon) · [`labelSchema`](#labelschema) · [`clientInputResult`](#clientinputresult)
>
> ```cpp
> class ClientTaskTool final : public Tool {
> public:
>     ClientTaskTool()
>         : Tool({.name = "client.task",
>                 .description = "Legacy client-owned sampling or elicitation task and lifecycle controls.",
>                 .inputSchema =
>                     Json::parse(R"json(
> {
>     "type": "object",
>     "properties": {
>         "kind": {
>             "enum": [
>                 "sampling",
>                 "elicitation"
>             ]
>         },
>         "cancel": {
>             "type": "boolean"
>         }
>     }
> }
> )json"),
>                 .title = "client.task",
>                 .annotations =
>                     Json::parse(R"json(
> {
>     "readOnlyHint": true,
>     "destructiveHint": false,
>     "idempotentHint": true,
>     "openWorldHint": false
> }
> )json"),
>                 .icons = {icon()},
>                 .meta = Json::parse(R"json(
> {
>     "showcase/feature": "client.task"
> }
> )json")}) {
>     }
>
>     void call(const ToolCall& request, const std::shared_ptr<ToolReply>& reply) final {
>         request.context.trackClientProgress("created");
>         const bool sampling = request.arguments.value("kind", "sampling") == "sampling";
>         const auto& state = request.context.getRequestState();
>         if (request.context.getProtocolEra() != ::mcp::ProtocolEra::Initialization20251125) {
>             reply->result(ToolResult::toolError({TextContent{
>                 "Client-owned task augmentation belongs to 2025-11-25; use task input continuations in 2026-07-28.", {}}}));
>         } else if (!state) {
>             Json params =
>                 sampling
>                     ? Json::parse(R"json(
> {
>     "messages": [
>         {
>             "role": "user",
>             "content": {
>                 "type": "text",
>                 "text": "42"
>             }
>         }
>     ],
>     "maxTokens": 64
> }
> )json")
>                     : Json{
>                         {"mode", "form"},
>                         {"message", "Client task demonstration"},
>                         {"requestedSchema", labelSchema()}
>                     };
>             params["task"] = {
>                 {"ttl", 60000}
>             };
>             reply->clientRequestsRequired(
>                 {
>                     {"created", {
>                         {"method", sampling ? "sampling/createMessage" : "elicitation/create"},
>                         {"params", params}
>                     }}
>                 },
>                 "created");
>         } else if (*state == "created") {
>             const auto* ticket = request.context.getClientResponse("created");
>             if (ticket && ticket->contains("task")) {
>                 const std::string id = ticket->at("task").at("taskId");
>                 reply->clientRequestsRequired({
>                     {"status", {
>                         {"method", "tasks/get"},
>                         {"params", {
>                             {"taskId", id}
>                         }}
>                     }},
>                     {"inventory", {
>                         {"method", "tasks/list"},
>                         {"params", Json::object()}
>                     }}
>                 },
>                                               Json{
>                                                   {"phase", "inspect"},
>                                                   {"taskId", id}
>                                               }.dump(4));
>             } else {
>                 reply->error(ErrorCode::InvalidParams, "Client returned no task ticket");
>             }
>         } else {
>             Json data = Json::parse(*state);
>             const std::string phase = data.at("phase");
>             if (phase == "inspect") {
>                 const bool cancel = request.arguments.value("cancel", false);
>                 data["phase"] = cancel ? "cancelled" : "completed";
>                 reply->clientRequestsRequired(
>                     {
>                         {"payload", {
>                             {"method", cancel ? "tasks/cancel" : "tasks/result"},
>                             {"params", {
>                                 {"taskId", data.at("taskId")}
>                             }}
>                         }}
>                     },
>                     data.dump(4));
>             } else {
>                 const auto* payload = request.context.getClientResponse("payload");
>                 if (payload) {
>                     if (phase == "completed" && sampling) {
>                         static_cast<void>(::mcp::SamplingResult(*payload));
>                     } else if (phase == "completed") {
>                         static_cast<void>(request.context.getInputResponse("payload", labelSchema()));
>                     }
>                     reply->result(clientInputResult(request.context, "created", {
>                         {"status", phase},
>                         {"payload", *payload}
>                     }));
>                 } else {
>                     reply->error(ErrorCode::InvalidParams, "Missing client task result");
>                 }
>             }
>         }
>     }
> };
> ```
>
> [FeatureInteractions.cpp · 422–483](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L422-L483) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L532)

> #### PingTool
>
> Pings the initialized legacy client.
>
> [Download standalone C++](class-examples/PingTool.cpp) **Uses** [`icon`](#icon)
>
> ```cpp
> class PingTool final : public Tool {
> public:
>     PingTool()
>         : Tool({.name = "client.ping",
>                 .description = "Ping the initialized legacy client.",
>                 .inputSchema = Json::parse(R"json(
> {
>     "type": "object",
>     "additionalProperties": false
> }
> )json"),
>                 .title = "client.ping",
>                 .annotations =
>                     Json::parse(R"json(
> {
>     "readOnlyHint": true,
>     "destructiveHint": false,
>     "idempotentHint": true,
>     "openWorldHint": false
> }
> )json"),
>                 .icons = {icon()},
>                 .meta = Json::parse(R"json(
> {
>     "showcase/feature": "client.ping"
> }
> )json")}) {
>     }
>
>     void call(const ToolCall& request, const std::shared_ptr<ToolReply>& reply) final {
>         if (request.context.getProtocolEra() != ::mcp::ProtocolEra::Initialization20251125) {
>             reply->result(ToolResult::toolError({TextContent{"Ping belongs to the initialized 2025-11-25 session.", {}}}));
>         } else if (const auto* result = request.context.getClientResponse("ping")) {
>             reply->result(ToolResult::json(*result));
>         } else {
>             reply->clientRequestsRequired({
>                 {"ping", {
>                     {"method", "ping"},
>                     {"params", Json::object()}
>                 }}
>             });
>         }
>     }
> };
> ```
>
> [FeatureInteractions.cpp · 485–500](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L485-L500) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L533)

> #### RootsTool
>
> Requests the filesystem roots that the client chooses to disclose.
>
> [Download standalone C++](class-examples/RootsTool.cpp) **Uses** [`icon`](#icon) · [`clientInputResult`](#clientinputresult)
>
> ```cpp
> class RootsTool final : public Tool {
> public:
>     RootsTool()
>         : Tool({.name = "roots",
>                 .description = "Request client-disclosed informational filesystem roots.",
>                 .inputSchema = Json::parse(R"json(
> {
>     "type": "object",
>     "additionalProperties": false
> }
> )json"),
>                 .title = "roots",
>                 .annotations =
>                     Json::parse(R"json(
> {
>     "readOnlyHint": true,
>     "destructiveHint": false,
>     "idempotentHint": true,
>     "openWorldHint": false
> }
> )json"),
>                 .icons = {icon()},
>                 .meta = Json::parse(R"json(
> {
>     "showcase/feature": "roots"
> }
> )json")}) {
>     }
>
>     void call(const ToolCall& request, const std::shared_ptr<ToolReply>& reply) final {
>         request.context.trackClientProgress("roots");
>         const std::weak_ptr<unsigned> weak = changes;
>         request.context.onRootsChanged([weak] {
>             if (const auto count = weak.lock()) {
>                 ++*count;
>             }
>         });
>         if (const auto response = request.context.getRootsResponse("roots")) {
>             Json value = response->toJson();
>             value["listChanges"] = *changes;
>             reply->result(clientInputResult(request.context, "roots", std::move(value)));
>         } else {
>             reply->clientInputRequired({{
>                 {"roots", ::mcp::RootsRequest{}}
>             }, "roots"});
>         }
>     }
>
> private:
>     std::shared_ptr<unsigned> changes = std::make_shared<unsigned>(0);
> };
> ```
>
> [FeatureInteractions.cpp · 502–527](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L502-L527) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L534)

### Showcase resources and registration

> #### TextResourceItem
>
> Serves showcase://text as text with metadata and cache hints.
>
> [Download standalone C++](class-examples/TextResourceItem.cpp) **Uses** [`icon`](#icon)
>
> ```cpp
> class TextResourceItem final : public Resource {
> public:
>     TextResourceItem()
>         : Resource({.uri = "showcase://text",
>                     .name = "text",
>                     .description = "Text resource, metadata and explicit public caching.",
>                     .mimeType = "text/plain",
>                     .title = "Showcase text",
>                     .icons = {icon()},
>                     .meta = Json::parse(R"json(
> {
>     "showcase/resource": "text"
> }
> )json"),
>                     .annotations = {{Role::User}, 0.7, "2026-01-01T00:00:00Z"}}) {
>     }
>
>     void read(const ResourceRead& call, const std::shared_ptr<ResourceReply>& reply) override {
>         reply->result(ResourceResult{{TextResource{call.uri, "text/plain", "SNode.C\n", {
>             {"showcase/content", true}
>         }}},
>                                      {
>                                          {"showcase/result", true}
>                                      },
>                                      CacheHints{1000, CacheScope::Public}});
>     }
> };
> ```
>
> [FeatureResources.cpp · 82–93](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L82-L93) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L307)

> #### BlobResourceItem
>
> Serves showcase://blob as generated binary content.
>
> [Download standalone C++](class-examples/BlobResourceItem.cpp) **Uses** [`encoded`](#encoded)
>
> ```cpp
> class BlobResourceItem final : public Resource {
> public:
>     BlobResourceItem()
>         : Resource({"showcase://blob", "blob", "Generated binary resource with private cache hints.", "application/octet-stream"}) {
>     }
>
>     void read(const ResourceRead& call, const std::shared_ptr<ResourceReply>& reply) override {
>         reply->result(ResourceResult{{BlobResource{call.uri, "application/octet-stream", encoded(std::string("\0\1\2", 3))}},
>                                      {},
>                                      CacheHints{500, CacheScope::Private}});
>     }
> };
> ```
>
> [FeatureResources.cpp · 95–106](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L95-L106) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L308)

> #### CounterResource
>
> Owns the counter used by resource reads and the update demo.
>
> [Download standalone C++](class-examples/CounterResource.cpp)
>
> ```cpp
> class CounterResource final : public Resource {
> public:
>     explicit CounterResource(Server& server)
>         : Resource({"showcase://counter", "counter", "Changing resource; subscribe/listen before calling update.", "text/plain"})
>         , server(server) {
>     }
>
>     void read(const ResourceRead& call, const std::shared_ptr<ResourceReply>& reply) override {
>         reply->result(ResourceResult{{TextResource{call.uri, "text/plain", std::to_string(revision)}}});
>     }
>
>     std::uint64_t update() {
>         ++revision;
>         server.resourceUpdated(getDefinition().uri);
>         return revision;
>     }
>
> private:
>     Server& server;
>     std::uint64_t revision = 0;
> };
> ```
>
> [FeatureResources.cpp · 108–128](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L108-L128) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L309-L310)

> #### UpdateTool
>
> Increments the counter and returns its revision.
>
> [Download standalone C++](class-examples/UpdateTool.cpp) **Uses** [`icon`](#icon) · [`CounterResource`](#counterresource)
>
> ```cpp
> class UpdateTool final : public Tool {
> public:
>     explicit UpdateTool(std::shared_ptr<CounterResource> counter)
>         : Tool({.name = "update",
>                 .description = "Increment the resource and emit resourceUpdated to subscribed consumers.",
>                 .inputSchema = Json::parse(R"json(
> {
>     "type": "object",
>     "additionalProperties": false
> }
> )json"),
>                 .title = "update",
>                 .annotations =
>                     Json::parse(R"json(
> {
>     "readOnlyHint": false,
>     "destructiveHint": false,
>     "idempotentHint": false,
>     "openWorldHint": false
> }
> )json"),
>                 .icons = {icon()},
>                 .meta = Json::parse(R"json(
> {
>     "showcase/feature": "update"
> }
> )json")})
>         , counter(std::move(counter)) {
>     }
>
>     void call(const ToolCall&, const std::shared_ptr<ToolReply>& reply) override {
>         reply->result(ToolResult::json({
>             {"revision", counter->update()}
>         }));
>     }
>
> private:
>     std::shared_ptr<CounterResource> counter;
> };
> ```
>
> [FeatureResources.cpp · 137–150](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L137-L150) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L311)

> #### GreetingTemplate
>
> Expands a name into a greeting.
>
> [Download standalone C++](class-examples/GreetingTemplate.cpp) **Uses** [`completeName`](#completename)
>
> ```cpp
> class GreetingTemplate final : public ResourceTemplate {
> public:
>     GreetingTemplate()
>         : ResourceTemplate({"showcase://greeting/{name}",
>                             "greeting",
>                             "Template variables and contextual completion.",
>                             "text/plain",
>                             {},
>                             {
>                                 {"name", completeName}
>                             }}) {
>     }
>
>     void read(const ResourceRead& call, const std::shared_ptr<ResourceReply>& reply) override {
>         reply->result(ResourceResult{{TextResource{call.uri, "text/plain", "Hello " + call.variables.at("name")}}});
>     }
> };
> ```
>
> [FeatureResources.cpp · 165–179](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L165-L179) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L312)

> #### PathTemplate
>
> Reads a URI template with a reserved path expansion.
>
> [Download standalone C++](class-examples/PathTemplate.cpp)
>
> ```cpp
> class PathTemplate final : public ResourceTemplate {
> public:
>     PathTemplate()
>         : ResourceTemplate({"showcase://path/{+path}", "path", "Reserved URI-template expansion.", "text/plain"}) {
>     }
>
>     void read(const ResourceRead& call, const std::shared_ptr<ResourceReply>& reply) override {
>         reply->result(ResourceResult{{TextResource{call.uri, "text/plain", call.variables.at("path")}}});
>     }
> };
> ```
>
> [FeatureResources.cpp · 181–190](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L181-L190) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L313)

> #### ConversationPrompt
>
> Builds a two-message prompt that embeds showcase text. Registered, with no native prompt call observed in Codex.
>
> [Download standalone C++](class-examples/ConversationPrompt.cpp) **Uses** [`completeName`](#completename)
>
> ```cpp
> class ConversationPrompt final : public Prompt {
> public:
>     ConversationPrompt()
>         : Prompt({"conversation",
>                   "Prompt arguments, completion, roles, multiple messages and embedded resources.",
>                   {{"name", "Name to greet", Requirement::Required, completeName}}}) {
>     }
>
>     void get(const PromptGet& call, const std::shared_ptr<PromptReply>& reply) override {
>         reply->result(
>             PromptResult{"A generated multi-message conversation.",
>                          {{Role::User, TextContent{"Hello " + call.arguments.at("name"), {}}},
>                           {Role::Assistant, EmbeddedResource{TextResource{"showcase://text", "text/plain", "SNode.C"}, {}}}},
>                          {
>                              {"showcase/prompt", true}
>                          }});
>     }
> };
> ```
>
> [FeatureResources.cpp · 192–207](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L192-L207) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L314)

> #### CatalogueTool
>
> Returns the name for each catalogue tool, including the dynamically registered instance.
>
> [Download standalone C++](class-examples/CatalogueTool.cpp) **Uses** [`icon`](#icon)
>
> ```cpp
> class CatalogueTool final : public Tool {
> public:
>     explicit CatalogueTool(std::string name)
>         : Tool({.name = name,
>                 .description = "Catalogue entry demonstrating pagination and dynamic registration.",
>                 .inputSchema = Json::parse(R"json(
> {
>     "type": "object",
>     "additionalProperties": false
> }
> )json"),
>                 .title = name,
>                 .annotations =
>                     Json::parse(R"json(
> {
>     "readOnlyHint": true,
>     "destructiveHint": false,
>     "idempotentHint": true,
>     "openWorldHint": false
> }
> )json"),
>                 .icons = {icon()},
>                 .meta = {
>                     {"showcase/feature", name}
>                 }}) {
>     }
>
>     void call(const ToolCall&, const std::shared_ptr<ToolReply>& reply) override {
>         reply->result(ToolResult::text(getDefinition().name));
>     }
> };
> ```
>
> [FeatureResources.cpp · 209–218](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L209-L218) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L315-L317)

> #### CatalogueResource
>
> Serves the named fixed catalogue resources and the dynamic resource.
>
> [Download standalone C++](class-examples/CatalogueResource.cpp)
>
> ```cpp
> class CatalogueResource final : public Resource {
> public:
>     CatalogueResource(const std::string& uri, const std::string& name)
>         : Resource({uri, name, "Catalogue entry demonstrating pagination and dynamic registration.", "text/plain"}) {
>     }
>
>     void read(const ResourceRead& call, const std::shared_ptr<ResourceReply>& reply) override {
>         reply->result(ResourceResult{{TextResource{call.uri, "text/plain", getDefinition().name}}});
>     }
> };
> ```
>
> [FeatureResources.cpp · 220–229](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L220-L229) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L318)

> #### CataloguePrompt
>
> Supplies the catalogue and dynamic prompt entries. Dynamic registration was exercised, while prompt retrieval was unavailable.
>
> [Download standalone C++](class-examples/CataloguePrompt.cpp)
>
> ```cpp
> class CataloguePrompt final : public Prompt {
> public:
>     explicit CataloguePrompt(const std::string& name)
>         : Prompt({name, "Catalogue entry demonstrating pagination and dynamic registration."}) {
>     }
>
>     void get(const PromptGet&, const std::shared_ptr<PromptReply>& reply) override {
>         reply->result(PromptResult{{}, {{Role::User, TextContent{getDefinition().name, {}}}}});
>     }
> };
> ```
>
> [FeatureResources.cpp · 231–240](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L231-L240) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L319)

> #### CatalogueTemplate
>
> Returns the captured variable for the fixed catalogue and dynamic resource templates.
>
> [Download standalone C++](class-examples/CatalogueTemplate.cpp)
>
> ```cpp
> class CatalogueTemplate final : public ResourceTemplate {
> public:
>     CatalogueTemplate(const std::string& uri, const std::string& name)
>         : ResourceTemplate({uri, name, "Catalogue entry demonstrating pagination and dynamic registration.", "text/plain"}) {
>     }
>
>     void read(const ResourceRead& call, const std::shared_ptr<ResourceReply>& reply) override {
>         reply->result(ResourceResult{{TextResource{call.uri, "text/plain", call.variables.at("value")}}});
>     }
> };
> ```
>
> [FeatureResources.cpp · 242–251](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L242-L251) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L320-L321)

> #### DynamicTool
>
> Adds and removes four registrations together using the server registry.
>
> [Download standalone C++](class-examples/DynamicTool.cpp) **Uses** [`icon`](#icon) · [`CatalogueTool`](#cataloguetool) · [`CatalogueResource`](#catalogueresource) · [`CataloguePrompt`](#catalogueprompt) · [`CatalogueTemplate`](#cataloguetemplate)
>
> ```cpp
> class DynamicTool final : public Tool {
> public:
>     explicit DynamicTool(Server& server)
>         : Tool({.name = "dynamic",
>                 .description = "Add/remove all four registration kinds; coalesced list_changed notifications.",
>                 .inputSchema = Json::parse(R"json(
> {
>     "type": "object",
>     "properties": {
>         "action": {
>             "enum": [
>                 "add",
>                 "remove"
>             ]
>         }
>     },
>     "required": [
>         "action"
>     ],
>     "additionalProperties": false
> }
> )json"),
>                 .title = "dynamic",
>                 .annotations =
>                     Json::parse(R"json(
> {
>     "readOnlyHint": false,
>     "destructiveHint": true,
>     "idempotentHint": false,
>     "openWorldHint": false
> }
> )json"),
>                 .icons = {icon()},
>                 .meta = Json::parse(R"json(
> {
>     "showcase/feature": "dynamic"
> }
> )json")})
>         , server(server) {
>     }
>
>     void call(const ToolCall& call, const std::shared_ptr<ToolReply>& reply) override {
>         if (call.arguments.at("action") == "add") {
>             server.tool(std::make_shared<CatalogueTool>("dynamic.tool"));
>             server.resource(std::make_shared<CatalogueResource>("showcase://dynamic", "dynamic"));
>             server.resourceTemplate(std::make_shared<CatalogueTemplate>("showcase://dynamic/{value}", "dynamic"));
>             server.prompt(std::make_shared<CataloguePrompt>("dynamic"));
>         } else {
>             server.remove(RegistrationKind::Tool, "dynamic.tool");
>             server.remove(RegistrationKind::Resource, "showcase://dynamic");
>             server.remove(RegistrationKind::ResourceTemplate, "showcase://dynamic/{value}");
>             server.remove(RegistrationKind::Prompt, "dynamic");
>         }
>         reply->result(ToolResult::text(call.arguments.at("action")));
>     }
>
> private:
>     Server& server;
> };
> ```
>
> [FeatureResources.cpp · 278–302](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L278-L302) · [Registration](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L323)

### Supporting code

> #### LookupMode
>
> Selects header search or bounded read.
>
> ```cpp
> enum class LookupMode { Find, Read };
> ```
>
> [Tools.h · 73](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.h#L73)

> #### header
>
> Checks the installed header file extensions.
>
> ```cpp
> bool header(const std::string& path) {
>     return path.ends_with(".h") || path.ends_with(".hpp");
> }
> ```
>
> [Tools.cpp · 92–94](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.cpp#L92-L94)

> #### Headers
>
> Owns the root directory descriptor and opens confined paths.
>
> **Uses** [`header`](#header)
>
> ```cpp
> class Headers {
> public:
>     explicit Headers(const std::string& path)
>         : root(::open(path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW)) {
>         if (root < 0) {
>             throw std::runtime_error("Cannot open installed header root");
>         }
>     }
>
>     Headers(const Headers&) = delete;
>     Headers& operator=(const Headers&) = delete;
>
>     ~Headers() {
>         ::close(root);
>     }
>
>     int open(const std::string& path, bool directory) const {
>         std::vector<std::string> parts;
>         std::size_t position = 0;
>         bool valid = path.size() <= 512 && path.find('\0') == std::string::npos && (directory || header(path));
>         while (valid && position < path.size()) {
>             const auto slash = path.find('/', position);
>             const std::string part = path.substr(position, slash == std::string::npos ? slash : slash - position);
>             valid = !part.empty() && part != "." && part != ".." && parts.size() < 16;
>             parts.push_back(part);
>             position = slash == std::string::npos ? path.size() : slash + 1;
>         }
>
>         valid = valid && (path.empty() || path.back() != '/');
>         int fd = valid ? ::openat(root, ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW) : -1;
>         for (std::size_t i = 0; fd >= 0 && i < parts.size(); ++i) {
>             const bool parent = directory || i + 1 < parts.size();
>             const int next = ::openat(fd, parts[i].c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | (parent ? O_DIRECTORY : O_NONBLOCK));
>             ::close(fd);
>             fd = next;
>         }
>
>         struct stat info{};
>         if (fd >= 0 && !directory && (::fstat(fd, &info) != 0 || !S_ISREG(info.st_mode))) {
>             ::close(fd);
>             fd = -1;
>         }
>
>         return fd;
>     }
>
> private:
>     int root;
> };
> ```
>
> [Tools.cpp · 96–144](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.cpp#L96-L144)

> #### Lookup
>
> Schedules lookup work, owns file state and completes the reply.
>
> **Uses** [`LookupMode`](#lookupmode) · [`header`](#header) · [`Headers`](#headers)
>
> ```cpp
> class Lookup final : public std::enable_shared_from_this<Lookup> {
> public:
>     Lookup(std::shared_ptr<Headers> headers,
>            std::shared_ptr<ToolReply> reply,
>            Json args,
>            LookupMode mode,
>            std::size_t maximumFiles,
>            std::size_t maximumBytes)
>         : headers(std::move(headers))
>         , reply(std::move(reply))
>         , args(std::move(args))
>         , mode(mode)
>         , maximumFiles(maximumFiles)
>         , maximumBytes(mode == LookupMode::Read ? 65536 : maximumBytes) {
>         limit =
>             this->args.value(mode == LookupMode::Read ? "bytes" : "limit", std::size_t{mode == LookupMode::Read ? 16384U : 20U});
>         query = this->args.value("query", std::string());
>     }
>
>     ~Lookup() {
>         closeFile();
>         if (directory != nullptr) {
>             ::closedir(directory);
>         }
>     }
>
>     void start(const ::mcp::server::RequestContext& context) {
>         const std::weak_ptr<Lookup> weak = shared_from_this();
>         context.onCancel([weak](::mcp::server::CancelReason) {
>             if (const auto current = weak.lock()) {
>                 current->cancel();
>             }
>         });
>         schedule();
>     }
>
> private:
>     void schedule() {
>         timer = core::timer::Timer::singleshotTimer(
>             [self = shared_from_this()] {
>                 auto current = std::move(self->timer); // Detach this firing handle before scheduling its successor.
>                 self->tick();
>             },
>             utils::Timeval(0.001));
>     }
>
>     void cancel() {
>         done = true;
>         timer.cancel();
>         closeFile();
>         if (directory != nullptr) {
>             ::closedir(directory);
>             directory = nullptr;
>         }
>     }
>
>     core::timer::Timer timer;
>     std::shared_ptr<Headers> headers;
>     std::shared_ptr<ToolReply> reply;
>     Json args;
>     LookupMode mode;
>     std::size_t maximumFiles;
>     std::size_t maximumBytes;
>     std::size_t limit = 0;
>     std::size_t files = 0;
>     std::size_t bytes = 0;
>     std::size_t entries = 0;
>     std::size_t offset = 0;
>     std::size_t lines = 0;
>     std::string query;
>     std::string path;
>     std::string folder;
>     std::string tail;
>     std::string text;
>     std::string failure;
>     std::deque<std::string> directories{std::string()};
>     DIR* directory = nullptr;
>     int file = -1;
>     Json matches = Json::array();
>     bool done = false;
>     bool truncated = false;
>
>     void collect(const char* chunk, std::size_t length) {
>         if (!done) {
>             bytes += length;
>             if (mode == LookupMode::Read) {
>                 text.append(chunk, std::min(length, limit - text.size()));
>                 done = text.size() == limit;
>                 truncated = done;
>             } else {
>                 const std::string window = tail + std::string(chunk, length);
>                 const std::size_t baseLine = lines - static_cast<std::size_t>(std::count(tail.begin(), tail.end(), '\n')) + 1;
>                 std::size_t at = window.find(query);
>                 while (at != std::string::npos && matches.size() < limit) {
>                     matches.push_back({
>                         {"path", path},
>                         {"offset", offset - tail.size() + at},
>                         {"line", baseLine + static_cast<std::size_t>(std::count(
>                                                        window.begin(), window.begin() + static_cast<std::ptrdiff_t>(at), '\n'))}
>                     });
>                     at = window.find(query, at + 1);
>                 }
>
>                 lines += static_cast<std::size_t>(std::count(chunk, chunk + length, '\n'));
>                 offset += length;
>                 tail = window.substr(window.size() - std::min(window.size(), query.size() - 1));
>                 done = matches.size() == limit;
>                 truncated = done;
>             }
>         }
>     }
>
>     void closeFile() {
>         if (file >= 0) {
>             ::close(file);
>             file = -1;
>         }
>     }
>
>     void readChunk() {
>         std::array<char, 16384> chunk{};
>         const std::size_t remaining = mode == LookupMode::Read ? limit - text.size() : maximumBytes - bytes;
>         const ssize_t count = core::system::read(file, chunk.data(), std::min(chunk.size(), remaining));
>         if (count > 0) {
>             collect(chunk.data(), static_cast<std::size_t>(count));
>         } else if (count == 0) {
>             closeFile();
>             if (mode == LookupMode::Read) {
>                 done = true;
>                 truncated = false;
>             }
>         } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
>             failure = "Header read failed";
>             done = true;
>         }
>     }
>
>     void attach() {
>         int fd = headers->open(path, false);
>         if (fd >= 0 && mode == LookupMode::Read &&
>             ::lseek(fd, static_cast<off_t>(args.value("offset", std::size_t{0})), SEEK_SET) < 0) {
>             ::close(fd);
>             fd = -1;
>         }
>
>         if (fd >= 0) {
>             ++files;
>             tail.clear();
>             offset = 0;
>             lines = 0;
>             file = fd;
>         } else if (mode == LookupMode::Read) {
>             failure = "Header is unavailable or outside the confined tree";
>             done = true;
>         }
>     }
>
>     void traverse() {
>         if (directory == nullptr && !directories.empty()) {
>             folder = std::move(directories.front());
>             directories.pop_front();
>             const int fd = headers->open(folder, true);
>             if (fd >= 0) {
>                 directory = ::fdopendir(fd);
>                 if (directory == nullptr) {
>                     ::close(fd);
>                 }
>             }
>         }
>
>         for (std::size_t i = 0; i < 8 && directory != nullptr && file < 0 && !done; ++i) {
>             dirent* entry = ::readdir(directory);
>             if (entry == nullptr) {
>                 ::closedir(directory);
>                 directory = nullptr;
>             } else {
>                 ++entries;
>                 const std::string name(entry->d_name);
>                 const std::string candidate = folder.empty() ? name : folder + "/" + name;
>                 struct stat info{};
>                 if (name != "." && name != ".." && candidate.size() <= 512 &&
>                     ::fstatat(::dirfd(directory), name.c_str(), &info, AT_SYMLINK_NOFOLLOW) == 0) {
>                     static_cast<void>(Json(candidate).dump(4));
>                     if (S_ISDIR(info.st_mode) && directories.size() < 512) {
>                         directories.push_back(candidate);
>                     } else if (S_ISREG(info.st_mode) && header(candidate)) {
>                         path = candidate;
>                         attach();
>                     }
>                 }
>
>                 if (entries >= 8192 || directories.size() >= 512) {
>                     truncated = true;
>                     done = true;
>                 }
>             }
>         }
>
>         done = done || (directory == nullptr && directories.empty() && file < 0);
>     }
>
>     void finish() {
>         closeFile();
>         if (!failure.empty()) {
>             reply->result(ToolResult::toolError(failure));
>         } else if (mode == LookupMode::Read) {
>             const Json value{
>                 {"path", path},
>                 {"offset", args.value("offset", std::size_t{0})},
>                 {"bytes", text.size()},
>                 {"text", text},
>                 {"truncated", truncated}
>             };
>             reply->result(ToolResult::json(value));
>         } else {
>             const Json value{
>                 {"matches", matches},
>                 {"files", files},
>                 {"bytes", bytes},
>                 {"entries", entries},
>                 {"truncated", truncated}
>             };
>             reply->result(ToolResult::json(value));
>         }
>     }
>
>     void tick() {
>         try {
>             if (!done && (bytes >= maximumBytes || (files >= maximumFiles && file < 0)) && mode == LookupMode::Find) {
>                 truncated = true;
>                 done = true;
>             }
>
>             if (!done && file >= 0) {
>                 readChunk();
>             } else if (!done && mode == LookupMode::Read) {
>                 path = args["path"].get<std::string>();
>                 attach();
>             } else if (!done) {
>                 traverse();
>             }
>
>             if (done) {
>                 finish();
>             } else {
>                 schedule();
>             }
>         } catch (const std::exception&) {
>             failure = "Lookup failed (including invalid UTF-8 header data)";
>             finish();
>         }
>     }
> };
> ```
>
> [Tools.cpp · 148–394](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.cpp#L148-L394)

> #### style
>
> Stores the shared house-style text.
>
> ```cpp
> constexpr const char* style = "SNode.C: use one event loop; configure named instances before init; listen, then start. "
>                               "Use semantic logging, express routing and existing CLI11 instance options. "
>                               "Keep protocol code independent of transports. Bound every handler's work. "
>                               "Stream with Response::pipe and finish through Source EOF. Never mix send/json with an open stream.";
> ```
>
> [Tools.cpp · 396–399](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.cpp#L396-L399)

> #### DevelopmentConfig
>
> Declares the installed header root and search limits.
>
> ```cpp
> class DevelopmentConfig : public ::mcp::server::Config {
> public:
>     explicit DevelopmentConfig(utils::SubCommand* parent)
>         : ::mcp::server::Config(parent) {
>         headerRoot = addOption("--header-root", "Installed SNode.C header root", "directory", CLI::ExistingDirectory);
>         setDefaultValue(headerRoot, std::string(MCP_HEADER_ROOT));
>         searchFiles = addOption("--search-files", "Maximum headers per search", "count", std::size_t{4096}, CLI::Range(1, 8192));
>         searchBytes =
>             addOption("--search-bytes", "Maximum bytes read per search", "bytes", std::size_t{8388608}, CLI::Range(1, 67108864));
>     }
>
>     std::string getHeaderRoot() const {
>         return headerRoot->as<std::string>();
>     }
>
>     std::size_t getSearchFiles() const {
>         return searchFiles->as<std::size_t>();
>     }
>
>     std::size_t getSearchBytes() const {
>         return searchBytes->as<std::size_t>();
>     }
>
> private:
>     CLI::Option* headerRoot;
>     CLI::Option* searchFiles;
>     CLI::Option* searchBytes;
> };
> ```
>
> [Tools.h · 83–95](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.h#L83-L95) · [Tools.cpp · 403–421](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.cpp#L403-L421)

> #### instructions
>
> Returns the house-style text for registration and reads.
>
> **Uses** [`style`](#style)
>
> ```cpp
> std::string instructions() {
>     return style;
> }
> ```
>
> [Tools.cpp · 435–437](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.cpp#L435-L437)

> #### openHeaders
>
> Creates the confined header root.
>
> **Uses** [`Headers`](#headers)
>
> ```cpp
> std::shared_ptr<Headers> openHeaders(const std::string& root) {
>     return std::make_shared<Headers>(root);
> }
> ```
>
> [Tools.cpp · 439–441](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.cpp#L439-L441)

> #### lookup function
>
> Creates and starts the asynchronous lookup.
>
> **Uses** [`LookupMode`](#lookupmode) · [`Headers`](#headers) · [`Lookup`](#lookup)
>
> ```cpp
> void lookup(const std::shared_ptr<Headers>& headers,
>             const ::mcp::server::ToolCall& call,
>             const std::shared_ptr<::mcp::server::ToolReply>& reply,
>             LookupMode mode,
>             std::size_t files,
>             std::size_t bytes) {
>     std::make_shared<Lookup>(headers, reply, call.arguments, mode, files, bytes)->start(call.context);
> }
> ```
>
> [Tools.cpp · 443–450](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.cpp#L443-L450)

> #### parseSchema
>
> Parses inline schema JSON and attaches registration error context.
>
> ```cpp
> ::mcp::server::Json parseSchema(const std::string& name, const char* text, const char* field = "inputSchema") {
>     ::mcp::server::Json schema;
>     try {
>         schema = nlohmann::ordered_json::parse(text);
>     } catch (const ::mcp::server::Json::parse_error& error) {
>         throw ::mcp::server::RegistrationError(::mcp::server::RegistrationKind::Tool, name, field, error.what());
>     }
>
>     return schema;
> }
> ```
>
> [Tools.cpp · 452–461](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/Tools.cpp#L452-L461)

> #### encoded
>
> Encodes showcase media and resource bytes.
>
> ```cpp
> std::string encoded(const std::string& bytes) {
>     return base64::base64_encode(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
> }
> ```
>
> [FeatureContent.cpp · 70–72](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureContent.cpp#L70-L72)

> #### icon
>
> Builds the showcase’s inline SVG icon.
>
> **Uses** [`encoded`](#encoded)
>
> ```cpp
> Icon icon() {
>     const std::string svg =
>         R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16"><rect width="16" height="16" fill="#247"/><circle cx="8" cy="8" r="5" fill="#fff"/></svg>)svg";
>     return Icon{"data:image/svg+xml;base64," + encoded(svg), "image/svg+xml", {"16x16"}, IconTheme::Light};
> }
> ```
>
> [FeatureContent.cpp · 74–78](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureContent.cpp#L74-L78)

> #### Work
>
> Implements the timer, cancellation, progress and completion for bounded work.
>
> ```cpp
> class Work final : public std::enable_shared_from_this<Work> {
> public:
>     Work(std::shared_ptr<ToolReply> reply, unsigned steps, unsigned delay)
>         : reply(std::move(reply))
>         , steps(steps)
>         , delay(delay) {
>     }
>
>     void start(const RequestContext& context) {
>         const std::weak_ptr<Work> weak = shared_from_this();
>         context.onCancel([weak](CancelReason) {
>             if (const auto work = weak.lock()) {
>                 work->timer.cancel();
>             }
>         });
>         schedule();
>     }
>
> private:
>     void schedule() {
>         timer = core::timer::Timer::singleshotTimer(
>             [self = shared_from_this()] {
>                 auto firing = std::move(self->timer);
>                 ++self->completed;
>                 self->reply->progress(
>                     Progress{static_cast<double>(self->completed), static_cast<double>(self->steps), "bounded work"});
>                 if (self->completed == self->steps) {
>                     self->reply->result(ToolResult::json({
>                         {"completed", self->completed}
>                     }));
>                 } else {
>                     self->schedule();
>                 }
>             },
>             utils::Timeval(static_cast<double>(delay) / 1000.0));
>     }
>
>     std::shared_ptr<ToolReply> reply;
>     unsigned steps;
>     unsigned delay;
>     unsigned completed = 0;
>     core::timer::Timer timer;
> };
> ```
>
> [FeatureInteractions.cpp · 77–117](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L77-L117)

> #### labelSchema
>
> Defines the shared form’s label field.
>
> ```cpp
> Json labelSchema() {
>     return Json::parse(R"json(
> {
>     "type": "object",
>     "properties": {
>         "label": {
>             "type": "string",
>             "minLength": 1,
>             "maxLength": 64
>         }
>     },
>     "required": [
>         "label"
>     ]
> }
> )json");
> }
> ```
>
> [FeatureInteractions.cpp · 159–175](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L159-L175)

> #### actionName
>
> Maps the client’s action to its result string.
>
> ```cpp
> std::string actionName(const InputResponse* response) {
>     return response != nullptr && response->action == ElicitationAction::Accept   ? "accept"
>            : response != nullptr && response->action == ElicitationAction::Cancel ? "cancel"
>                                                                                   : "decline";
> }
> ```
>
> [FeatureInteractions.cpp · 177–181](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L177-L181)

> #### inputResult
>
> Packs an elicitation action and accepted label into JSON.
>
> **Uses** [`actionName`](#actionname)
>
> ```cpp
> Json inputResult(const InputResponse* response) {
>     Json value{
>         {"action", actionName(response)}
>     };
>     if (response != nullptr && response->action == ElicitationAction::Accept) {
>         value["label"] = response->content.at("label");
>     }
>     return value;
> }
> ```
>
> [FeatureInteractions.cpp · 183–189](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L183-L189)

> #### labelRequest
>
> Creates a label request for resource and prompt handlers.
>
> **Uses** [`labelSchema`](#labelschema)
>
> ```cpp
> InputRequired labelRequest(const std::string& message) {
>     return InputRequired{{
>         {"first", FormInput{message, labelSchema()}}
>     }, "label"};
> }
> ```
>
> [FeatureInteractions.cpp · 191–193](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L191-L193)

> #### clientInputResult
>
> Preserves returned data and optional tracked client progress.
>
> ```cpp
> ToolResult clientInputResult(const RequestContext& context, const std::string& key, Json value) {
>     auto result = ToolResult::json(std::move(value));
>     if (const auto progress = context.getClientProgress(key)) {
>         result.setMeta({
>             {"showcase/clientProgress", progress->value}
>         });
>     }
>     return result;
> }
> ```
>
> [FeatureInteractions.cpp · 330–336](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureInteractions.cpp#L330-L336)

> #### completeName
>
> Returns matching demonstration names for completion.
>
> ```cpp
> void completeName(const CompletionRequest& call, const std::shared_ptr<CompletionReply>& reply) {
>     CompletionResult result;
>     const std::string prefix = call.arguments.contains("prefix") ? call.arguments.at("prefix") : std::string{};
>     for (const auto& value : {"alpha", "beta", "gamma"}) {
>         if (std::string(value).starts_with(call.value)) {
>             result.values.push_back(prefix + value);
>         }
>     }
>     result.total = result.values.size();
>     result.meta = Json::parse(R"json(
> {
>     "showcase/completion": true
> }
> )json");
>     reply->result(std::move(result));
> }
> ```
>
> [FeatureResources.cpp · 152–163](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/apps/mcp/showcase/FeatureResources.cpp#L152-L163)

### Source fragment audit

All 37 standalone class examples passed independent C++20 syntax checks against the installed SNode.C headers. The earlier constructor comparison matched 34 showcase tool definitions and the text resource against their source definitions.

| Area | Finding | Fix |
|---|---|---|
| Translation-unit context | All 37 class fragments depended on surrounding namespace declarations and headers. The previous snippets omitted that context, including framework aliases used by the showcase. | Each standalone class example includes its headers and namespace. Examples that use DevelopmentConfig include the configured header-root default. |
| Header lookup and configuration | FindTool and ReadTool depended on DevelopmentConfig, openHeaders, lookup and LookupMode. The lookup path also required Headers, the header predicate and the asynchronous Lookup implementation. | The FindTool and ReadTool examples include configuration, confined open and lookup code, cancellation, cleanup and result handling. |
| Development schemas and house style | FindTool, ReadTool, SkeletonPreviewTool and ReviewLabelTool used parseSchema. HouseStyleResource, ReviewPrompt and StyleTemplate used instructions and its style text. | Cards link to the shared parser and instruction text, and the dependent examples include them. The registered tools provide their schema JSON in the constructor and keep registration errors contextual. |
| Review label state | ReviewLabelTool needed the complete ReviewLabelResource class to store the accepted label. The source fragments showed the two classes separately. | ReviewLabelTool links to ReviewLabelResource and includes it in its standalone example. |
| Showcase metadata and schemas | Showcase tools used the shared toolDefinition builder and its schema, emptySchema and icon helpers. StructuredTool also used structuredDefinition to attach the output schema. | Constructors now contain the definition fields and schema JSON. StructuredTool defines its input and output schemas together. The icon and encoding routines appear once for callers that use them. |
| Work, progress and task support | WorkTool called workDefinition and instantiated Work. Both were absent. The work helper owns the timer, counts steps, reports progress and responds to cancellation. | WorkTool declares the bounded input schema and task support inside Tool(...). The Work helper has its own card and is included in the WorkTool download. |
| Forms and client callbacks | InputTool needed inputDefinition, labelSchema, actionName and inputResult. InputResource and InputPrompt also needed labelRequest. SamplingTool, ClientTaskTool and RootsTool needed clientInputResult. ClientTaskTool also uses labelSchema. | The input tool declares its mode-specific schema and task support in its constructor. Shared runtime forms, response conversion and client progress handling are included once. |
| Media and resource metadata | ContentTool and BlobResourceItem used encoded. ContentTool and the text resource used icon. TextResourceItem delegated metadata and annotations to textDefinition. | The media helpers are included. TextResourceItem declares its definition, annotations and metadata directly in Resource(...). |
| Updates and dynamic registration | UpdateTool depended on updateDefinition and CounterResource. DynamicTool depended on dynamicDefinition and the four catalogue classes it registers. | Both tools define their schemas and annotations in their constructors. The counter and catalogue classes have individual cards and are included in the examples that use them. |
| Completion callbacks and simple resources | GreetingTemplate and ConversationPrompt referred to completeName. CounterResource, PathTemplate, CatalogueResource, CataloguePrompt and CatalogueTemplate used the framework definitions and dispatch context around their class bodies. | The callback has a linked card and is included with each class that uses it. Direct constructor definitions remain direct. |
| JSON formatting | The exchange panels, Markdown JSON blocks and archive used two-space indentation. | The readable exchanges and static C++ JSON literals use nlohmann’s four-space format. JSON text is expanded in the reports and retained verbatim in the archive. Run-time C++ JSON initializers are laid out by field and serialized with dump(4). |

### Shared framework dispatch

The registry handles lookup and validation around the registered classes.

#### Resource inventory

The server lists descriptions from registered entries. It does not call the individual resource read methods for discovery. [Framework source](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/mcp/server/Server.cpp#L988-L1007)

#### Resource template inventory

The server lists registered resource template descriptions. [Framework source](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/mcp/server/Server.cpp#L979-L987)

#### Resource dispatch

The server resolves the URI against a fixed registration or template and dispatches to the selected class. [Framework source](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/mcp/server/Server.cpp#L1008-L1065)

#### Tool dispatch and input validation

The server validates arguments against the registered schema before calling the tool class. [Framework source](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/mcp/server/Server.cpp#L1008-L1065)

#### Tool output validation

A tool response with a registered output schema is checked before it is returned to the client. [Framework source](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/mcp/server/Server.cpp#L174-L181)

#### Task augmentation validation

The server checks the requested task metadata against the tool’s task support and the protocol path. [Framework source](https://github.com/SNodeC/snode.c/blob/aa3d904fba0573d6be110aa2f6c6d5c2ef320177/src/mcp/server/Server.cpp#L1092-L1105)

---

Live Codex observations and the SNode.C source used to explain them.
