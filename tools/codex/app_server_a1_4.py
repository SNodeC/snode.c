#!/usr/bin/env python3
"""Audit and freeze the Codex App Server Phase A1.4 residue.

The generated JSON documents are deterministic review evidence.  They guard
the total protocol partition, the native A1.4 start state, and the transitive
stable schema closure.  ``ProtocolSurfaceRegistryData.inc`` remains the sole
local production authority.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence

sys.dont_write_bytecode = True

import app_server_a1_shared as shared
import app_server_fixtures as fixtures
import app_server_schema_paths as schema_paths
import app_server_surface as surface


FORMAT_VERSION = 1
CODEX_VERSION = "codex-cli 0.144.6"
UPSTREAM_TAG = "rust-v0.144.6"
UPSTREAM_SOURCE_COMMIT = "5d1fbf26c43abc65a203928b2e31561cb039e06d"
A1_4_SLICE = "A1.4"
A1_4_MODULE = "IntegrationsAndLongTail"
EXPECTED_BASE_SHA = "bed5ee05184739d54cddc6518bd43b264e2b496d"
EXPECTED_BASE_TREE = "8bf44f0eed6bd8c9d5309176db07a7d6a6240ef3"
EXPECTED_PRODUCTION_TREE = "a1833fa778e97d29b9294627842ff9bd04f22379"
EXPECTED_FIXTURE_TREE = "be1eb65746c93a22a516af9bc1d1916ee8f2aa67"
EXPECTED_FRONTEND_PROTOCOL_BLOB = (
    "bb5abb687323c0a7e5ecc51bd9d5d58d0108a4da"
)
EXPECTED_SCHEMA_AGGREGATES = {
    "stable": "cee1ac3bcaf95e5fcdcf07499c7e6b00fc423b90c670ea3380f1799434b72add",
    "experimental": "4a0ef96787255364d99b15fe40fcfd6227901978d0cddc8b20340bfef98a0d1b",
}
EXPECTED_TYPESCRIPT_AGGREGATES = {
    "stable": "73c95b6a19d5559939519a771e0f7285cc60bbfaa1aac8bd9c52ba308c6e6811",
    "experimental": "d561ef0b4ef8a921fff50de4e3c662a0fdff643e82868f2b05a3e71f912aec8c",
}

EXPECTED_PARTITION = {
    "A1.0": 19,
    "A1.1": 151,
    "A1.2": 45,
    "A1.3": 68,
    "A1.4": 56,
    "InventoryOnly": 48,
}
EXPECTED_STABILITY_ACCOUNTING = {
    "experimental_only_inventory": 36,
    "stable_a1": 339,
    "stable_inventory": 351,
    "stable_unreachable_inventory": 12,
    "total_inventory": 387,
}
EXPECTED_INVENTORY_CLASSIFICATIONS = {
    "ExperimentalInventoryOnly": 36,
    "StableUnreachableInventory": 12,
}
EXPECTED_TAXONOMY = {
    "client_request": 29,
    "server_notification": 16,
    "server_request": 4,
    "tagged_union_discriminator": 7,
}
EXPECTED_RESULT_KINDS = {"Concrete": 26, "Unit": 3}
EXPECTED_NATIVE_START_STATUS = {
    "Complete": 0,
    "NotImplemented": 55,
    "Partial": 1,
}
EXPECTED_GLOBAL_START_STATUS = {
    "Complete": 280,
    "NotApplicable": 48,
    "NotImplemented": 55,
    "Partial": 4,
}
EXPECTED_NATIVE_COMPLETION_STATUS = {
    "Complete": 336,
    "NotApplicable": 48,
    "NotImplemented": 0,
    "Partial": 3,
}
EXPECTED_FINAL_A1_STATUS = {
    "Complete": 339,
    "NotApplicable": 48,
    "NotImplemented": 0,
    "Partial": 0,
}
EXPECTED_CLOSURE_COUNTS = {
    "default_bearing_paths": 22,
    "definition_namespaces": {"legacy": 34, "v2": 155},
    "integer_formats": {
        "double": 3,
        "int32": 1,
        "int64": 8,
        "uint": 1,
        "uint32": 4,
        "uint64": 6,
    },
    "maximum_bearing_paths": 0,
    "minimum_bearing_paths": 11,
    "nullable_paths": 243,
    "object_nodes": 162,
    "opaque_json_paths": 24,
    "optional_paths": 260,
    "reachable_named_definitions": 189,
    "required_paths": 288,
    "schema_path_kinds": {
        "array_element": 91,
        "map_value": 7,
        "property": 548,
    },
    "schema_paths": 646,
    "seed_definitions": 81,
    "sensitive_paths": 127,
}
EXPECTED_OBJECT_POLICIES = {
    "False": 12,
    "allowed_by_default": 143,
    "schema": 7,
}
FOUNDATION_CHANGED_PATHS = {
    "tests/CMakeLists.txt",
    "tests/component/codex/CodexA14AuditToolTest.py",
    (
        "tools/codex/app-server-evidence/0.144.6/"
        "a1-4-start-state.json"
    ),
    (
        "tools/codex/app-server-evidence/0.144.6/"
        "a1-4-total-partition.json"
    ),
    (
        "tools/codex/app-server-evidence/0.144.6/"
        "a1-4-type-closure.json"
    ),
    "tools/codex/app_server_a1_4.py",
}

AuditError = shared.AuditError
AuditDiagnostic = shared.AuditDiagnostic
Key = shared.Key


CLIENT_REQUEST_CONTRACTS = {
    "app/list": ("AppsListParams", "AppsListResponse", "Concrete"),
    "externalAgentConfig/detect": (
        "ExternalAgentConfigDetectParams",
        "ExternalAgentConfigDetectResponse",
        "Concrete",
    ),
    "externalAgentConfig/import": (
        "ExternalAgentConfigImportParams",
        "ExternalAgentConfigImportResponse",
        "Concrete",
    ),
    "externalAgentConfig/import/readHistories": (
        "Unit",
        "ExternalAgentConfigImportHistoriesReadResponse",
        "Concrete",
    ),
    "feedback/upload": (
        "FeedbackUploadParams",
        "FeedbackUploadResponse",
        "Concrete",
    ),
    "hooks/list": ("HooksListParams", "HooksListResponse", "Concrete"),
    "marketplace/add": (
        "MarketplaceAddParams",
        "MarketplaceAddResponse",
        "Concrete",
    ),
    "marketplace/remove": (
        "MarketplaceRemoveParams",
        "MarketplaceRemoveResponse",
        "Concrete",
    ),
    "marketplace/upgrade": (
        "MarketplaceUpgradeParams",
        "MarketplaceUpgradeResponse",
        "Concrete",
    ),
    "mcpServer/oauth/login": (
        "McpServerOauthLoginParams",
        "McpServerOauthLoginResponse",
        "Concrete",
    ),
    "mcpServer/resource/read": (
        "McpResourceReadParams",
        "McpResourceReadResponse",
        "Concrete",
    ),
    "mcpServer/tool/call": (
        "McpServerToolCallParams",
        "McpServerToolCallResponse",
        "Concrete",
    ),
    "mcpServerStatus/list": (
        "ListMcpServerStatusParams",
        "ListMcpServerStatusResponse",
        "Concrete",
    ),
    "plugin/install": (
        "PluginInstallParams",
        "PluginInstallResponse",
        "Concrete",
    ),
    "plugin/installed": (
        "PluginInstalledParams",
        "PluginInstalledResponse",
        "Concrete",
    ),
    "plugin/list": ("PluginListParams", "PluginListResponse", "Concrete"),
    "plugin/read": ("PluginReadParams", "PluginReadResponse", "Concrete"),
    "plugin/share/checkout": (
        "PluginShareCheckoutParams",
        "PluginShareCheckoutResponse",
        "Concrete",
    ),
    "plugin/share/delete": ("PluginShareDeleteParams", "Unit", "Unit"),
    "plugin/share/list": (
        "PluginShareListParams",
        "PluginShareListResponse",
        "Concrete",
    ),
    "plugin/share/save": (
        "PluginShareSaveParams",
        "PluginShareSaveResponse",
        "Concrete",
    ),
    "plugin/share/updateTargets": (
        "PluginShareUpdateTargetsParams",
        "PluginShareUpdateTargetsResponse",
        "Concrete",
    ),
    "plugin/skill/read": (
        "PluginSkillReadParams",
        "PluginSkillReadResponse",
        "Concrete",
    ),
    "plugin/uninstall": ("PluginUninstallParams", "Unit", "Unit"),
    "skills/config/write": (
        "SkillsConfigWriteParams",
        "SkillsConfigWriteResponse",
        "Concrete",
    ),
    "skills/extraRoots/set": (
        "SkillsExtraRootsSetParams",
        "Unit",
        "Unit",
    ),
    "skills/list": ("SkillsListParams", "SkillsListResponse", "Concrete"),
    "windowsSandbox/readiness": (
        "Unit",
        "WindowsSandboxReadinessResponse",
        "Concrete",
    ),
    "windowsSandbox/setupStart": (
        "WindowsSandboxSetupStartParams",
        "WindowsSandboxSetupStartResponse",
        "Concrete",
    ),
}

SERVER_NOTIFICATIONS = {
    "app/list/updated",
    "deprecationNotice",
    "externalAgentConfig/import/completed",
    "externalAgentConfig/import/progress",
    "hook/completed",
    "hook/started",
    "mcpServer/oauthLogin/completed",
    "mcpServer/startupStatus/updated",
    "process/exited",
    "process/outputDelta",
    "remoteControl/status/changed",
    "serverRequest/resolved",
    "skills/changed",
    "warning",
    "windows/worldWritableWarning",
    "windowsSandbox/setupCompleted",
}

SERVER_REQUEST_CONTRACTS = {
    "attestation/generate": (
        "AttestationGenerateParams",
        "AttestationGenerateResponse",
    ),
    "item/tool/call": (
        "DynamicToolCallParams",
        "DynamicToolCallResponse",
    ),
    "item/tool/requestUserInput": (
        "ToolRequestUserInputParams",
        "ToolRequestUserInputResponse",
    ),
    "mcpServer/elicitation/request": (
        "McpServerElicitationRequestParams",
        "McpServerElicitationRequestResponse",
    ),
}

UNION_FAMILIES = {
    ("McpServerElicitationRequestParams", "mode"): {
        "form",
        "openai/form",
        "url",
    },
    ("PluginSource", "type"): {"git", "local", "npm", "remote"},
}

EXPERIMENTAL_PROCESS_CONTROLS = {
    "process/kill",
    "process/resizePty",
    "process/spawn",
    "process/writeStdin",
}
EXPERIMENTAL_REMOTE_CONTROLS = {
    "remoteControl/client/list",
    "remoteControl/client/revoke",
    "remoteControl/disable",
    "remoteControl/enable",
    "remoteControl/pairing/start",
    "remoteControl/pairing/status",
    "remoteControl/status/read",
}

NATIVE_PARTIAL = Key(
    "server_request",
    "ServerRequest",
    "method",
    "item/tool/requestUserInput",
)
INHERITED_PARTIALS = {
    Key("client_request", "ClientRequest", "method", "initialize"),
    Key(
        "client_notification",
        "ClientNotification",
        "method",
        "initialized",
    ),
    Key("server_notification", "ServerNotification", "method", "error"),
}

IMPLEMENTATION_BATCHES = {
    "A14-UserIntegrations": {
        "branch": "codex/a1-4-user-integrations",
        "title": "Type the Codex A1.4 user-facing integrations",
        "subject": "Complete Codex user-facing integrations",
        "client_requests": {
            "app/list",
            "externalAgentConfig/detect",
            "externalAgentConfig/import",
            "externalAgentConfig/import/readHistories",
            "feedback/upload",
            "hooks/list",
            "marketplace/add",
            "marketplace/remove",
            "marketplace/upgrade",
            "plugin/install",
            "plugin/installed",
            "plugin/list",
            "plugin/read",
            "plugin/share/checkout",
            "plugin/share/delete",
            "plugin/share/list",
            "plugin/share/save",
            "plugin/share/updateTargets",
            "plugin/skill/read",
            "plugin/uninstall",
            "skills/config/write",
            "skills/extraRoots/set",
            "skills/list",
        },
        "server_notifications": {
            "app/list/updated",
            "externalAgentConfig/import/completed",
            "externalAgentConfig/import/progress",
            "hook/completed",
            "hook/started",
            "skills/changed",
        },
        "server_requests": set(),
        "union_families": {"PluginSource"},
        "dependencies": [],
        "identity_count": 33,
        "taxonomy": {
            "client_request": 23,
            "server_notification": 6,
            "server_request": 0,
            "tagged_union_discriminator": 4,
        },
        "client_result_kinds": {"Concrete": 20, "Unit": 3},
        "native_start": {
            "Complete": 0,
            "Partial": 1,
            "NotImplemented": 55,
        },
        "native_end": {
            "Complete": 33,
            "Partial": 1,
            "NotImplemented": 22,
        },
        "global_start": EXPECTED_GLOBAL_START_STATUS,
        "global_end": {
            "Complete": 313,
            "NotApplicable": 48,
            "NotImplemented": 22,
            "Partial": 4,
        },
        "headers": [
            "ai/openai/codex/typed/Apps.h",
            "ai/openai/codex/typed/ExternalAgents.h",
            "ai/openai/codex/typed/Feedback.h",
            "ai/openai/codex/typed/Hooks.h",
            "ai/openai/codex/typed/Marketplace.h",
            "ai/openai/codex/typed/Plugins.h",
            "ai/openai/codex/typed/Skills.h",
        ],
        "facades": [
            "Apps",
            "ExternalAgents",
            "Feedback",
            "Hooks",
            "Marketplace",
            "Plugins",
            "Skills",
        ],
        "codec_units": [
            "detail/AppCodec.{h,cpp}",
            "detail/ExternalAgentCodec.{h,cpp}",
            "detail/FeedbackCodec.{h,cpp}",
            "detail/HookCodec.{h,cpp}",
            "detail/MarketplaceCodec.{h,cpp}",
            "detail/PluginCodec.{h,cpp}",
            "detail/SkillCodec.{h,cpp}",
        ],
        "descriptor_changes": [
            "ClientOperationCodecDescriptors.inc",
            "ServerNotificationCodecDescriptors.inc",
            "IntegrationsAndLongTailUnionCodecDescriptors.inc",
        ],
        "primary_tests": [
            "CodexA14UserIntegrationsCodecTest",
            "CodexA14UserIntegrationsWireTest",
            "CodexA14UserIntegrationsFacadeTest",
            "CodexA14PluginSourceUnionTest",
        ],
        "logical_commits": [
            "Add Codex A1.4 integration public types",
            "Implement Codex A1.4 integration codecs and facades",
            "Close Codex A1.4 integration registry evidence",
        ],
    },
    "A14-McpReverse": {
        "branch": "codex/a1-4-mcp-reverse-requests",
        "title": "Type the Codex A1.4 MCP and reverse requests",
        "subject": "Complete Codex MCP and reverse requests",
        "client_requests": {
            "mcpServer/oauth/login",
            "mcpServer/resource/read",
            "mcpServer/tool/call",
            "mcpServerStatus/list",
        },
        "server_notifications": {
            "mcpServer/oauthLogin/completed",
            "mcpServer/startupStatus/updated",
        },
        "server_requests": set(SERVER_REQUEST_CONTRACTS),
        "union_families": {"McpServerElicitationRequestParams"},
        "dependencies": ["A14-UserIntegrations"],
        "identity_count": 13,
        "taxonomy": {
            "client_request": 4,
            "server_notification": 2,
            "server_request": 4,
            "tagged_union_discriminator": 3,
        },
        "client_result_kinds": {"Concrete": 4, "Unit": 0},
        "native_start": {
            "Complete": 33,
            "Partial": 1,
            "NotImplemented": 22,
        },
        "native_end": {
            "Complete": 46,
            "Partial": 0,
            "NotImplemented": 10,
        },
        "global_start": {
            "Complete": 313,
            "NotApplicable": 48,
            "NotImplemented": 22,
            "Partial": 4,
        },
        "global_end": {
            "Complete": 326,
            "NotApplicable": 48,
            "NotImplemented": 10,
            "Partial": 3,
        },
        "headers": [
            "ai/openai/codex/typed/Mcp.h",
            "ai/openai/codex/typed/ServerRequests.h",
        ],
        "facades": ["Mcp", "Requests", "Events"],
        "codec_units": [
            "detail/McpCodec.{h,cpp}",
            "detail/AttestationCodec.{h,cpp}",
            "detail/DynamicToolCodec.{h,cpp}",
            "detail/UserInputRequestCodec.{h,cpp}",
            "detail/McpElicitationCodec.{h,cpp}",
            "detail/ServerRequestDecoder.cpp",
        ],
        "descriptor_changes": [
            "ClientOperationCodecDescriptors.inc",
            "ServerNotificationCodecDescriptors.inc",
            "ServerRequestCodecDescriptors.inc",
            "IntegrationsAndLongTailUnionCodecDescriptors.inc",
        ],
        "primary_tests": [
            "CodexA14McpCodecTest",
            "CodexA14McpWireTest",
            "CodexA14ReverseRequestCodecTest",
            "CodexA14ReverseRequestWireTest",
            "CodexA14UserInputCompatibilityTest",
            "CodexA14McpElicitationUnionTest",
        ],
        "logical_commits": [
            "Add Codex A1.4 MCP and reverse-request public types",
            "Implement Codex A1.4 MCP and reverse-request codecs",
            "Close Codex A1.4 MCP and reverse-request registry evidence",
        ],
    },
    "A14-RuntimePlatform": {
        "branch": "codex/a1-4-runtime-platform",
        "title": "Type the Codex A1.4 runtime and platform long tail",
        "subject": "Complete Codex runtime and platform long tail",
        "client_requests": {
            "windowsSandbox/readiness",
            "windowsSandbox/setupStart",
        },
        "server_notifications": {
            "deprecationNotice",
            "process/exited",
            "process/outputDelta",
            "remoteControl/status/changed",
            "serverRequest/resolved",
            "warning",
            "windows/worldWritableWarning",
            "windowsSandbox/setupCompleted",
        },
        "server_requests": set(),
        "union_families": set(),
        "dependencies": ["A14-McpReverse"],
        "identity_count": 10,
        "taxonomy": {
            "client_request": 2,
            "server_notification": 8,
            "server_request": 0,
            "tagged_union_discriminator": 0,
        },
        "client_result_kinds": {"Concrete": 2, "Unit": 0},
        "native_start": {
            "Complete": 46,
            "Partial": 0,
            "NotImplemented": 10,
        },
        "native_end": {
            "Complete": 56,
            "Partial": 0,
            "NotImplemented": 0,
        },
        "global_start": {
            "Complete": 326,
            "NotApplicable": 48,
            "NotImplemented": 10,
            "Partial": 3,
        },
        "global_end": EXPECTED_NATIVE_COMPLETION_STATUS,
        "headers": [
            "ai/openai/codex/typed/WindowsSandbox.h",
            "ai/openai/codex/typed/Events.h",
        ],
        "facades": ["WindowsSandbox", "Events"],
        "codec_units": [
            "detail/RuntimePlatformCodec.{h,cpp}",
            "detail/WindowsSandboxCodec.{h,cpp}",
            "detail/EventDecoder.cpp",
        ],
        "descriptor_changes": [
            "ClientOperationCodecDescriptors.inc",
            "ServerNotificationCodecDescriptors.inc",
        ],
        "primary_tests": [
            "CodexA14RuntimePlatformCodecTest",
            "CodexA14RuntimePlatformWireTest",
            "CodexA14ServerRequestResolvedLifecycleTest",
            "CodexA14WindowsSandboxFacadeTest",
        ],
        "logical_commits": [
            "Add Codex A1.4 runtime and platform public types",
            "Implement Codex A1.4 runtime and platform codecs",
            "Close the native Codex A1.4 registry slice",
        ],
    },
}

FACADE_METHODS = {
    "Apps": {"list": "app/list"},
    "ExternalAgents": {
        "detect": "externalAgentConfig/detect",
        "importConfig": "externalAgentConfig/import",
        "readImportHistories": (
            "externalAgentConfig/import/readHistories"
        ),
    },
    "Feedback": {"upload": "feedback/upload"},
    "Hooks": {"list": "hooks/list"},
    "Marketplace": {
        "add": "marketplace/add",
        "remove": "marketplace/remove",
        "upgrade": "marketplace/upgrade",
    },
    "Mcp": {
        "oauthLogin": "mcpServer/oauth/login",
        "readResource": "mcpServer/resource/read",
        "callTool": "mcpServer/tool/call",
        "listServerStatuses": "mcpServerStatus/list",
    },
    "Plugins": {
        "install": "plugin/install",
        "installed": "plugin/installed",
        "list": "plugin/list",
        "read": "plugin/read",
        "checkoutShare": "plugin/share/checkout",
        "deleteShare": "plugin/share/delete",
        "listShares": "plugin/share/list",
        "saveShare": "plugin/share/save",
        "updateShareTargets": "plugin/share/updateTargets",
        "readSkill": "plugin/skill/read",
        "uninstall": "plugin/uninstall",
    },
    "Skills": {
        "writeConfig": "skills/config/write",
        "setExtraRoots": "skills/extraRoots/set",
        "list": "skills/list",
    },
    "WindowsSandbox": {
        "readiness": "windowsSandbox/readiness",
        "setupStart": "windowsSandbox/setupStart",
    },
}

EVENT_ADDITIONS = [
    ("app/list/updated", 51, 53),
    ("externalAgentConfig/import/completed", 52, 54),
    ("externalAgentConfig/import/progress", 53, 55),
    ("hook/completed", 54, 56),
    ("hook/started", 55, 57),
    ("skills/changed", 56, 58),
    ("mcpServer/oauthLogin/completed", 57, 59),
    ("mcpServer/startupStatus/updated", 58, 60),
    ("deprecationNotice", 59, 61),
    ("process/exited", 60, 62),
    ("process/outputDelta", 61, 63),
    ("remoteControl/status/changed", 62, 64),
    ("serverRequest/resolved", 63, 65),
    ("warning", 64, 66),
    ("windows/worldWritableWarning", 65, 67),
    ("windowsSandbox/setupCompleted", 66, 68),
]

SERVER_REQUEST_ADDITIONS = {
    "attestation/generate": {
        "public_type": "AttestationGenerateRequest",
        "variant_index": 8,
    },
    "item/tool/call": {
        "public_type": "DynamicToolCallRequest",
        "variant_index": 9,
    },
    "item/tool/requestUserInput": {
        "public_type": "UserInputRequest",
        "variant_index": 2,
        "completion_in_place": True,
    },
    "mcpServer/elicitation/request": {
        "public_type": "McpServerElicitationRequest",
        "variant_index": 10,
    },
}

REUSABLE_PUBLIC_TYPES = {
    "AbsolutePathBuf": "ai/openai/codex/typed/Types.h",
    "DynamicToolCallOutputContentItem": (
        "ai/openai/codex/typed/Conversation.h"
    ),
    "WindowsSandboxSetupMode": (
        "ai/openai/codex/typed/Configuration.h"
    ),
}

NON_REUSABLE_SIMILAR_TYPES = {
    "CommandExecProcessId/CommandExecOutputStream": (
        "A1.4 process handles and stream vocabulary are distinct"
    ),
    "ConfigWarningNotification/GuardianWarning": (
        "generic warning and deprecation payloads have distinct schemas"
    ),
    "conversation UserInput": (
        "ToolRequestUserInput is a reverse-request contract"
    ),
    "DynamicToolCallThreadItem": (
        "dynamic server call params/results are distinct; only the exact "
        "output-content union is reusable"
    ),
    "McpToolCallThreadItem": (
        "MCP server operation request/response schemas are distinct"
    ),
    "Accounts authentication types": (
        "MCP OAuth lifecycle is not account login lifecycle"
    ),
    "ConfigLayerSource/HookSource": (
        "PluginSource has its own tagged union and ownership"
    ),
    "SandboxMode/SandboxPolicy": (
        "Windows setup/readiness is a distinct platform contract"
    ),
}

PARTIAL_OBLIGATIONS = {
    "initialize": {
        "public_type_obligations": [
            "preserve automatic initialization and constructor signatures",
            "model ClientInfo.title as omitted/null/value",
            "append optional-null InitializeCapabilities",
            "model experimentalApi and requestAttestation defaults",
            "model optional mcpServerOpenaiFormElicitation",
            "model optional-null optOutNotificationMethods",
        ],
        "codec_obligations": [
            "encode exact InitializeParams without dropping known fields",
            "retain exact InitializeResponse decoding",
            "do not add a generic initialize facade",
        ],
        "test_obligations": [
            "omitted/null/value ClientInfo title and capabilities",
            "capability defaults and opt-out notification list",
            "request/result direction and registry target assertions",
            "automatic-handshake compatibility",
        ],
        "deferred_reason": (
            "cross-cutting automatic lifecycle compatibility belongs at "
            "the deliberate final A1 rebuild boundary"
        ),
    },
    "initialized": {
        "public_type_obligations": [
            "no new public payload type; the notification has no params",
        ],
        "codec_obligations": [
            "exact internal method-only notification encoder",
            "descriptor and registry direction assertion",
        ],
        "test_obligations": [
            "method-only positive schema case",
            "no-payload/no-drop direction evidence",
        ],
        "deferred_reason": (
            "the method-only notification completes the automatic "
            "initialization lifecycle with initialize"
        ),
    },
    "error": {
        "public_type_obligations": [
            "preserve Event alternative TurnErrorEvent at index 44",
            "append schema-complete ErrorNotification canonical view",
            "retain raw payload and diagnostics",
            "add optional canonical view to TurnErrorEvent",
        ],
        "codec_obligations": [
            "decode every CodexErrorInfo known/future/malformed union case",
            "preserve omitted/null/value fields and additional details",
            "retain current BackendCore/frontend projection",
        ],
        "test_obligations": [
            "all error properties and nullable states",
            "malformed-known and future-unknown error info",
            "Event index and backend compatibility",
        ],
        "deferred_reason": (
            "error is cross-cutting Common lifecycle state and its layout "
            "change belongs at final A1 closure"
        ),
    },
    "item/tool/requestUserInput": {
        "public_type_obligations": [
            "preserve UserInputRequest and TypedServerRequest index 2",
            "preserve UserInputQuestion/UserInputOption/UserInputAnswer",
            "append canonical ToolRequestUserInputParams and diagnostics",
            "model autoResolutionMs and options as omitted/null/value",
            "add canonical response map without removing legacy overload",
        ],
        "codec_obligations": [
            "exact params decoder with default and uint64 semantics",
            "exact ToolRequestUserInputResponse encoder",
            "post-encode schema validation and direct respondOwned path",
        ],
        "test_obligations": [
            "all params/response fields and nullable states",
            "known/duplicate question validation compatibility",
            "one-shot/retry/original-ID/generation lifecycle",
        ],
        "deferred_reason": (
            "native A1.4 reverse-request work owns this existing "
            "compatibility projection"
        ),
    },
}

EXPECTED_BATCH_CLOSURES = {
    "A14-UserIntegrations": {
        "seed_definitions": 52,
        "reachable_named_definitions": 118,
        "definition_namespaces": {"v2": 118},
        "schema_paths": 411,
        "schema_path_kinds": {
            "array_element": 68,
            "map_value": 4,
            "property": 339,
        },
        "object_nodes": 103,
        "required_paths": 176,
        "optional_paths": 163,
        "nullable_paths": 141,
        "default_bearing_paths": 19,
        "opaque_json_paths": 0,
        "sensitive_paths": 66,
    },
    "A14-McpReverse": {
        "seed_definitions": 18,
        "reachable_named_definitions": 55,
        "definition_namespaces": {"legacy": 34, "v2": 21},
        "schema_paths": 204,
        "schema_path_kinds": {
            "array_element": 22,
            "map_value": 3,
            "property": 179,
        },
        "object_nodes": 48,
        "required_paths": 87,
        "optional_paths": 92,
        "nullable_paths": 97,
        "default_bearing_paths": 3,
        "opaque_json_paths": 24,
        "sensitive_paths": 53,
    },
    "A14-RuntimePlatform": {
        "seed_definitions": 11,
        "reachable_named_definitions": 17,
        "definition_namespaces": {"v2": 17},
        "schema_paths": 31,
        "schema_path_kinds": {
            "array_element": 1,
            "map_value": 0,
            "property": 30,
        },
        "object_nodes": 11,
        "required_paths": 25,
        "optional_paths": 5,
        "nullable_paths": 5,
        "default_bearing_paths": 0,
        "opaque_json_paths": 0,
        "sensitive_paths": 8,
    },
}

SENSITIVE_FIELD_NAMES = {
    "_meta",
    "answers",
    "app",
    "apps",
    "arguments",
    "attachments",
    "authUrl",
    "checkout",
    "config",
    "content",
    "cwd",
    "data",
    "delta",
    "diagnostic",
    "environment",
    "error",
    "feedback",
    "history",
    "hook",
    "id",
    "input",
    "itemId",
    "message",
    "name",
    "output",
    "path",
    "processId",
    "prompt",
    "question",
    "questions",
    "reason",
    "repository",
    "requestedSchema",
    "resource",
    "result",
    "serverName",
    "source",
    "state",
    "stderr",
    "stdout",
    "target",
    "threadId",
    "token",
    "tool",
    "turnId",
    "url",
    "value",
    "values",
    "warning",
}


@dataclass
class Inputs:
    manifest: dict[str, Any]
    assignments_document: dict[str, Any]
    reachability_document: dict[str, Any]
    contracts_document: dict[str, Any]
    completeness_document: dict[str, Any]
    fixture_document: dict[str, Any]
    registry_rows: list[dict[str, Any]]
    manifest_rows: dict[Key, dict[str, Any]]
    assignments: dict[Key, dict[str, Any]]
    reachability: dict[Key, dict[str, Any]]
    contracts: dict[Key, dict[str, Any]]
    completeness: dict[Key, dict[str, Any]]
    fixture_coverage: dict[Key, dict[str, Any]]
    registry: dict[Key, dict[str, Any]]


def require(condition: bool, message: str, code: str) -> None:
    shared.require(condition, message, code)


def _counter(values: Iterable[str]) -> dict[str, int]:
    return dict(sorted(Counter(values).items()))


def _git(repo_root: Path, *arguments: str) -> str:
    completed = subprocess.run(
        ["git", *arguments],
        cwd=repo_root,
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode:
        raise AuditError(
            f"git {' '.join(arguments)} failed while freezing A1.4",
            "BaseCommitMismatch",
        )
    return completed.stdout.strip()


def _try_git(repo_root: Path, *arguments: str) -> str | None:
    """Return git output, or None when shallow history cannot satisfy a query."""

    completed = subprocess.run(
        ["git", *arguments],
        cwd=repo_root,
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode:
        return None
    return completed.stdout.strip()


def _source(path: Path, repo_root: Path) -> dict[str, str]:
    return {
        "path": path.resolve().relative_to(repo_root).as_posix(),
        "sha256": shared.sha256_file(path),
    }


def _keys_from_rows(
    rows: Sequence[Mapping[str, Any]],
    description: str,
) -> list[Key]:
    keys: list[Key] = []
    seen: dict[Key, Mapping[str, Any]] = {}
    for row in rows:
        key = Key.from_row(row)
        if key in seen:
            previous_slice = seen[key].get("slice")
            current_slice = row.get("slice")
            code = (
                "CrossSliceOverlap"
                if previous_slice != current_slice
                else "DuplicateAssignment"
            )
            raise AuditError(
                f"duplicate {description} identity: {key.compact()}",
                code,
            )
        seen[key] = row
        keys.append(key)
    return keys


def expected_a1_4_keys() -> set[Key]:
    result = {
        *(
            Key("client_request", "ClientRequest", "method", name)
            for name in CLIENT_REQUEST_CONTRACTS
        ),
        *(
            Key(
                "server_notification",
                "ServerNotification",
                "method",
                name,
            )
            for name in SERVER_NOTIFICATIONS
        ),
        *(
            Key("server_request", "ServerRequest", "method", name)
            for name in SERVER_REQUEST_CONTRACTS
        ),
    }
    for (domain, field), alternatives in UNION_FAMILIES.items():
        result.update(
            Key("tagged_union_discriminator", domain, field, name)
            for name in alternatives
        )
    return result


def load_inputs(arguments: argparse.Namespace) -> Inputs:
    manifest_document = shared.load_json(arguments.manifest)
    assignment_document = shared.load_json(arguments.assignments)
    registry_rows = surface.parse_registry_data(arguments.registry)
    manifest_rows = shared.records(
        manifest_document, ("entries",), "surface manifest"
    )
    assignment_rows = shared.records(
        assignment_document,
        ("assignments",),
        "module/slice assignment",
    )
    manifest_keys = _keys_from_rows(manifest_rows, "inventory")
    assignment_keys = _keys_from_rows(assignment_rows, "assignment")
    registry_keys = _keys_from_rows(registry_rows, "registry")
    require(
        len(manifest_keys) == 387,
        f"total inventory denominator changed: {len(manifest_keys)}",
        "TotalDenominatorMismatch",
    )
    manifest_set = set(manifest_keys)
    assignment_set = set(assignment_keys)
    require(
        assignment_set == manifest_set,
        "inventory/assignment exact-key mismatch",
        (
            "UnassignedIdentity"
            if manifest_set - assignment_set
            else "ExtraAssignment"
        ),
    )
    require(
        set(registry_keys) == manifest_set,
        "inventory/registry exact-key mismatch",
        "RegistryIdentitySetMismatch",
    )
    require(
        all(
            row.get("slice") in EXPECTED_PARTITION
            for row in assignment_rows
        ),
        "an inventory identity has no reviewed slice",
        "UnassignedIdentity",
    )
    values = shared.load_surface_evidence_inputs(
        manifest_path=arguments.manifest,
        assignments_path=arguments.assignments,
        reachability_path=arguments.reachability,
        contracts_path=arguments.contracts,
        completeness_path=arguments.schema_completeness,
        fixture_coverage_path=arguments.fixture_coverage,
        registry_path=arguments.registry,
        allowed_versions=frozenset({"0.144.6", CODEX_VERSION}),
    )
    return Inputs(**vars(values))


def _manifest_stability(row: Mapping[str, Any]) -> str:
    stability = str(row.get("stability"))
    require(
        stability in {"stable", "experimental_only"},
        f"unknown manifest stability: {stability}",
        "StabilityMismatch",
    )
    return stability


def validate_partition_inputs(inputs: Inputs) -> list[Key]:
    manifest_rows = shared.records(
        inputs.manifest, ("entries",), "surface manifest"
    )
    assignment_rows = shared.records(
        inputs.assignments_document,
        ("assignments",),
        "module/slice assignment",
    )
    registry_rows = inputs.registry_rows
    manifest_keys = _keys_from_rows(manifest_rows, "inventory")
    assignment_keys = _keys_from_rows(assignment_rows, "assignment")
    registry_keys = _keys_from_rows(registry_rows, "registry")

    require(
        len(manifest_keys) == EXPECTED_STABILITY_ACCOUNTING["total_inventory"],
        f"total inventory denominator changed: {len(manifest_keys)}",
        "TotalDenominatorMismatch",
    )
    manifest_set = set(manifest_keys)
    assignment_set = set(assignment_keys)
    registry_set = set(registry_keys)
    require(
        assignment_set == manifest_set,
        "inventory/assignment exact-key mismatch",
        (
            "UnassignedIdentity"
            if manifest_set - assignment_set
            else "ExtraAssignment"
        ),
    )
    require(
        registry_set == manifest_set,
        "inventory/registry exact-key mismatch",
        "RegistryIdentitySetMismatch",
    )

    allowed_slices = set(EXPECTED_PARTITION)
    for key in sorted(manifest_set):
        manifest = inputs.manifest_rows[key]
        assignment = inputs.assignments[key]
        registry = inputs.registry[key]
        assigned_slice = assignment.get("slice")
        require(
            assigned_slice in allowed_slices,
            f"identity is unassigned: {key.compact()}",
            "UnassignedIdentity",
        )
        stability = _manifest_stability(manifest)
        require(
            assignment.get("stability") == stability
            and registry.get("stability") == stability,
            f"stability mismatch: {key.compact()}",
            "StabilityMismatch",
        )
        require(
            registry.get("typed_module") == assignment.get("module")
            and registry.get("a1_slice") == assigned_slice,
            f"module/slice mismatch: {key.compact()}",
            "ModuleMismatch",
        )
        require(
            Key.from_row(manifest) == Key.from_row(assignment)
            == Key.from_row(registry),
            f"surface category mismatch: {key.compact()}",
            "CategoryMismatch",
        )

    for key in sorted(manifest_set):
        assignment = inputs.assignments[key]
        if assignment["slice"] == A1_4_SLICE:
            require(
                assignment["stability"] != "experimental_only"
                and assignment["classification"]
                != "ExperimentalInventoryOnly",
                f"experimental identity assigned to A1.4: {key.compact()}",
                "ExperimentalLeakage",
            )
            require(
                assignment["classification"]
                != "StableUnreachableInventory",
                (
                    "stable-unreachable identity assigned to A1.4: "
                    f"{key.compact()}"
                ),
                "StableUnreachableBoundaryMismatch",
            )
    for key in INHERITED_PARTIALS:
        assignment = inputs.assignments[key]
        require(
            assignment["slice"] == "A1.0"
            and assignment["module"] == "Common",
            f"inherited A1.0 owner changed: {key.compact()}",
            "CrossSliceOwnershipMismatch",
        )

    partition = _counter(
        str(inputs.assignments[key]["slice"]) for key in manifest_set
    )
    require(
        partition == EXPECTED_PARTITION,
        f"six-bucket partition changed: {partition}",
        "PartitionCountMismatch",
    )
    stability = _counter(
        _manifest_stability(inputs.manifest_rows[key])
        for key in manifest_set
    )
    require(
        stability == {"experimental_only": 36, "stable": 351},
        f"stable/experimental inventory changed: {stability}",
        "HiddenStableIdentity",
    )

    inventory_keys = {
        key
        for key in manifest_set
        if inputs.assignments[key]["slice"] == "InventoryOnly"
    }
    provisional_a1_4 = {
        key
        for key in manifest_set
        if inputs.assignments[key]["slice"] == A1_4_SLICE
    }
    for key in sorted(provisional_a1_4):
        assignment = inputs.assignments[key]
        require(
            assignment["stability"] != "experimental_only"
            and assignment["classification"]
            != "ExperimentalInventoryOnly",
            f"experimental identity assigned to A1.4: {key.compact()}",
            "ExperimentalLeakage",
        )
        require(
            assignment["classification"]
            != "StableUnreachableInventory",
            f"stable-unreachable identity assigned to A1.4: {key.compact()}",
            "StableUnreachableBoundaryMismatch",
        )
    for key in INHERITED_PARTIALS:
        assignment = inputs.assignments[key]
        require(
            assignment["slice"] == "A1.0"
            and assignment["module"] == "Common",
            f"inherited A1.0 owner changed: {key.compact()}",
            "CrossSliceOwnershipMismatch",
        )
    inventory_classifications = _counter(
        str(inputs.assignments[key]["classification"])
        for key in inventory_keys
    )
    require(
        inventory_classifications == EXPECTED_INVENTORY_CLASSIFICATIONS,
        (
            "InventoryOnly must remain 36 experimental-only plus "
            "12 stable-unreachable identities"
        ),
        "InventoryClassificationMismatch",
    )
    for key in inventory_keys:
        assignment = inputs.assignments[key]
        registry = inputs.registry[key]
        classification = assignment["classification"]
        if classification == "ExperimentalInventoryOnly":
            require(
                assignment["stability"] == "experimental_only",
                f"experimental inventory stability changed: {key.compact()}",
                "ExperimentalBoundaryMismatch",
            )
        else:
            require(
                classification == "StableUnreachableInventory"
                and assignment["stability"] == "stable",
                f"stable-unreachable inventory changed: {key.compact()}",
                "StableUnreachableBoundaryMismatch",
            )
        require(
            registry["typed_schema_status"] == "NotApplicable"
            and registry["typed_status"] == "NotImplemented",
            f"InventoryOnly status changed: {key.compact()}",
            "InventoryStatusMismatch",
        )

    a1_4 = {
        key
        for key in manifest_set
        if inputs.assignments[key]["slice"] == A1_4_SLICE
    }
    require(
        a1_4 == expected_a1_4_keys(),
        "native A1.4 identity set changed",
        "A14IdentitySetMismatch",
    )
    require(
        len(a1_4) == 56,
        f"native A1.4 denominator changed: {len(a1_4)}",
        "A14DenominatorMismatch",
    )
    for key in sorted(a1_4):
        assignment = inputs.assignments[key]
        require(
            assignment["stability"] == "stable",
            f"experimental identity assigned to A1.4: {key.compact()}",
            "ExperimentalLeakage",
        )
        require(
            assignment["module"] == A1_4_MODULE,
            f"wrong A1.4 module: {key.compact()}",
            "A14ModuleMismatch",
        )

    process_inventory = {
        key.name
        for key in inventory_keys
        if key.name in EXPERIMENTAL_PROCESS_CONTROLS
    }
    require(
        process_inventory == EXPERIMENTAL_PROCESS_CONTROLS,
        "experimental process controls left InventoryOnly",
        "ProcessBoundaryMismatch",
    )
    require(
        {
            "process/exited",
            "process/outputDelta",
        }.issubset({key.name for key in a1_4}),
        "stable process notifications are missing from A1.4",
        "ProcessBoundaryMismatch",
    )
    require(
        "remoteControl/status/changed" in {key.name for key in a1_4},
        "stable remote-control status notification is missing",
        "RemoteControlBoundaryMismatch",
    )
    remote_inventory = {
        key.name
        for key in inventory_keys
        if key.name in EXPERIMENTAL_REMOTE_CONTROLS
    }
    require(
        remote_inventory == EXPERIMENTAL_REMOTE_CONTROLS,
        "experimental remote-control methods left InventoryOnly",
        "RemoteControlBoundaryMismatch",
    )
    require(
        not (
            EXPERIMENTAL_REMOTE_CONTROLS
            & {
                key.name
                for key in a1_4
                if key.category == "client_request"
            }
        ),
        "experimental remote-control client method leaked into A1.4",
        "RemoteControlBoundaryMismatch",
    )
    return sorted(a1_4)


def _status_counter(
    rows: Iterable[Mapping[str, Any]],
) -> dict[str, int]:
    values = Counter(str(row["typed_schema_status"]) for row in rows)
    for name in (
        "Complete",
        "Partial",
        "NotImplemented",
        "NotApplicable",
    ):
        values.setdefault(name, 0)
    return dict(sorted(values.items()))


def validate_start_arithmetic(inputs: Inputs, keys: Sequence[Key]) -> None:
    native = _status_counter(inputs.registry[key] for key in keys)
    global_status = _status_counter(inputs.registry.values())
    require(
        native
        == {
            **EXPECTED_NATIVE_START_STATUS,
            "NotApplicable": 0,
        },
        f"native A1.4 starting status changed: {native}",
        "NativeStartStatusMismatch",
    )
    require(
        global_status == EXPECTED_GLOBAL_START_STATUS,
        f"global starting status changed: {global_status}",
        "GlobalStartStatusMismatch",
    )
    native_partial = {
        key
        for key in keys
        if inputs.registry[key]["typed_schema_status"] == "Partial"
    }
    require(
        native_partial == {NATIVE_PARTIAL},
        "item/tool/requestUserInput is not the sole native A1.4 Partial",
        "NativePartialMismatch",
    )
    global_partial = {
        key
        for key, row in inputs.registry.items()
        if row["typed_schema_status"] == "Partial"
    }
    require(
        global_partial == INHERITED_PARTIALS | {NATIVE_PARTIAL},
        "the four frozen global Partial identities changed",
        "GlobalPartialMismatch",
    )


def validate_provenance(arguments: argparse.Namespace) -> dict[str, Any]:
    schema_provenance = shared.load_json(arguments.schema_provenance)
    protocol_provenance = shared.load_json(arguments.protocol_provenance)
    release = schema_provenance.get("upstream", {}).get("release", {})
    require(
        release.get("tag") == UPSTREAM_TAG
        and release.get("source_commit_sha") == UPSTREAM_SOURCE_COMMIT,
        "schema provenance release changed",
        "ProvenanceMismatch",
    )
    protocol_release = protocol_provenance.get("release", {})
    require(
        protocol_release.get("tag") == UPSTREAM_TAG
        and protocol_release.get("source_commit_sha")
        == UPSTREAM_SOURCE_COMMIT,
        "protocol-source provenance release changed",
        "ProvenanceMismatch",
    )

    schema_tree_summary: dict[str, Any] = {}
    for stability, expected_aggregate in EXPECTED_SCHEMA_AGGREGATES.items():
        tree = schema_provenance.get("schema_trees", {}).get(stability, {})
        files = tree.get("files")
        require(
            isinstance(files, list),
            f"{stability} schema provenance lacks files",
            "ProvenanceMismatch",
        )
        records: list[str] = []
        for row in files:
            relative = str(row["path"])
            path = arguments.schema_root / stability / relative
            require(
                path.is_file()
                and path.stat().st_size == row["bytes"]
                and shared.sha256_file(path) == row["sha256"],
                f"schema provenance mismatch: {stability}/{relative}",
                "ProvenanceHashMismatch",
            )
            records.append(f"{row['sha256']}  {relative}\n")
        aggregate = hashlib.sha256(
            "".join(records).encode("utf-8")
        ).hexdigest()
        require(
            aggregate == expected_aggregate
            and tree.get("aggregate_sha256") == expected_aggregate,
            f"{stability} schema aggregate changed: {aggregate}",
            "ProvenanceHashMismatch",
        )
        schema_tree_summary[stability] = {
            "aggregate_sha256": aggregate,
            "file_count": len(files),
        }

    for row in protocol_provenance.get("files", []):
        path = arguments.protocol_source_root / str(row["path"])
        require(
            path.is_file()
            and path.stat().st_size == row["bytes"]
            and shared.sha256_file(path) == row["sha256"],
            f"protocol-source provenance mismatch: {row['path']}",
            "ProvenanceHashMismatch",
        )

    typescript = schema_provenance.get("typescript_cross_check", {})
    for stability, expected in EXPECTED_TYPESCRIPT_AGGREGATES.items():
        require(
            typescript.get(stability, {}).get("aggregate_sha256")
            == expected,
            f"{stability} TypeScript aggregate changed",
            "ProvenanceMismatch",
        )
    return {
        "codex_version": CODEX_VERSION,
        "upstream_tag": UPSTREAM_TAG,
        "source_commit_sha": UPSTREAM_SOURCE_COMMIT,
        "schema_trees": schema_tree_summary,
        "typescript_aggregate_sha256": EXPECTED_TYPESCRIPT_AGGREGATES,
        "schema_provenance_sha256": shared.sha256_file(
            arguments.schema_provenance
        ),
        "protocol_source_provenance_sha256": shared.sha256_file(
            arguments.protocol_provenance
        ),
        "protocol_source_files": [
            {
                "path": row["path"],
                "sha256": row["sha256"],
                "bytes": row["bytes"],
            }
            for row in protocol_provenance.get("files", [])
        ],
    }


def start_state_document(
    arguments: argparse.Namespace, base_sha: str
) -> dict[str, Any]:
    tree = _git(arguments.repo_root, "rev-parse", f"{base_sha}^{{tree}}")
    require(
        base_sha == EXPECTED_BASE_SHA and tree == EXPECTED_BASE_TREE,
        f"unexpected A1.4 base {base_sha} tree {tree}",
        "BaseCommitMismatch",
    )
    inputs = load_inputs(arguments)
    keys = validate_partition_inputs(inputs)
    validate_start_arithmetic(inputs, keys)
    provenance = validate_provenance(arguments)
    native_status = _status_counter(inputs.registry[key] for key in keys)
    protected_evidence = {
        path.resolve().relative_to(arguments.repo_root).as_posix():
        shared.sha256_file(path)
        for pattern in (
            "a1-1-*.json",
            "a1-2-*.json",
            "a1-3-*.json",
        )
        for path in sorted(arguments.evidence_root.glob(pattern))
    }
    return {
        "format_version": FORMAT_VERSION,
        "generated_notice": (
            "Frozen A1.4 audit start state; review evidence only, "
            "never a runtime registry."
        ),
        "codex_version": CODEX_VERSION,
        "upstream_tag": UPSTREAM_TAG,
        "base_sha": base_sha,
        "base_tree": tree,
        "provenance": provenance,
        "counts": {
            "identity_count": len(keys),
            "taxonomy": _counter(key.category for key in keys),
            "native_schema_status": native_status,
            "global_schema_status": _status_counter(
                inputs.registry.values()
            ),
            "runtime_dispositions": _counter(
                str(inputs.registry[key]["runtime_disposition"])
                for key in keys
            ),
        },
        "native_partial_identity": NATIVE_PARTIAL.object(),
        "inherited_a1_0_partial_identities": [
            key.object() for key in sorted(INHERITED_PARTIALS)
        ],
        "identities": [
            {
                "protocol_surface_key": key.object(),
                "assignment": inputs.assignments[key],
                "registry": inputs.registry[key],
            }
            for key in keys
        ],
        "capture_sources": {
            "assignment": _source(
                arguments.assignments, arguments.repo_root
            ),
            "contracts": _source(arguments.contracts, arguments.repo_root),
            "fixture_coverage": _source(
                arguments.fixture_coverage, arguments.repo_root
            ),
            "manifest": _source(arguments.manifest, arguments.repo_root),
            "nested_reachability": _source(
                arguments.reachability, arguments.repo_root
            ),
            "protocol_provenance": _source(
                arguments.protocol_provenance, arguments.repo_root
            ),
            "registry": _source(arguments.registry, arguments.repo_root),
            "schema_completeness": _source(
                arguments.schema_completeness, arguments.repo_root
            ),
            "schema_provenance": _source(
                arguments.schema_provenance, arguments.repo_root
            ),
            "soversion_authority": _source(
                arguments.repo_root / "CMakeLists.txt",
                arguments.repo_root,
            ),
        },
        "protected_predecessor_evidence": protected_evidence,
    }


def validate_start_state(
    document: Mapping[str, Any], arguments: argparse.Namespace
) -> None:
    require(
        document.get("format_version") == FORMAT_VERSION
        and document.get("codex_version") == CODEX_VERSION
        and document.get("upstream_tag") == UPSTREAM_TAG
        and document.get("base_sha") == EXPECTED_BASE_SHA
        and document.get("base_tree") == EXPECTED_BASE_TREE,
        "frozen A1.4 start-state metadata changed",
        "StartStateMetadataMismatch",
    )
    counts = document.get("counts")
    require(
        isinstance(counts, Mapping)
        and counts.get("identity_count") == 56
        and counts.get("taxonomy") == EXPECTED_TAXONOMY
        and counts.get("native_schema_status")
        == {
            **EXPECTED_NATIVE_START_STATUS,
            "NotApplicable": 0,
        }
        and counts.get("global_schema_status")
        == EXPECTED_GLOBAL_START_STATUS,
        "frozen A1.4 start-state counts changed",
        "StartStateCountMismatch",
    )
    sources = document.get("capture_sources")
    require(
        isinstance(sources, Mapping),
        "frozen A1.4 sources are absent",
        "StartStateSourceMismatch",
    )
    source_paths = {
        "assignment": arguments.assignments,
        "contracts": arguments.contracts,
        "fixture_coverage": arguments.fixture_coverage,
        "manifest": arguments.manifest,
        "nested_reachability": arguments.reachability,
        "protocol_provenance": arguments.protocol_provenance,
        "registry": arguments.registry,
        "schema_completeness": arguments.schema_completeness,
        "schema_provenance": arguments.schema_provenance,
        "soversion_authority": arguments.repo_root / "CMakeLists.txt",
    }
    for name, path in source_paths.items():
        row = sources.get(name)
        require(
            isinstance(row, Mapping)
            and row.get("sha256") == shared.sha256_file(path),
            f"frozen A1.4 input changed: {name}",
            "StartStateSourceMismatch",
        )
    protected = document.get("protected_predecessor_evidence")
    require(
        isinstance(protected, Mapping) and bool(protected),
        "predecessor A1 evidence guard is absent",
        "PredecessorEvidenceMismatch",
    )
    for relative, digest in protected.items():
        path = arguments.repo_root / str(relative)
        require(
            path.is_file() and shared.sha256_file(path) == digest,
            f"predecessor A1 evidence changed: {relative}",
            "PredecessorEvidenceMismatch",
        )


def _operation_rows(
    inputs: Inputs, keys: Sequence[Key]
) -> list[dict[str, Any]]:
    operations: list[dict[str, Any]] = []
    client_kinds: Counter[str] = Counter()
    for key in keys:
        if key.category not in {"client_request", "server_request"}:
            continue
        contract = inputs.contracts.get(key)
        require(
            contract is not None,
            f"missing operation contract: {key.compact()}",
            "ContractMismatch",
        )
        registry = inputs.registry[key]
        require(
            registry["parameter_type_identity"]
            == contract["parameter_type_identity"]
            and registry["result_type_identity"]
            == contract["result_type_identity"]
            and registry["result_contract_kind"]
            == contract["result_contract_kind"],
            f"registry/contract association mismatch: {key.compact()}",
            "ContractMismatch",
        )
        if key.category == "client_request":
            expected = CLIENT_REQUEST_CONTRACTS[key.name]
            actual = (
                contract["parameter_type_identity"],
                contract["result_type_identity"],
                contract["result_contract_kind"],
            )
            require(
                actual == expected,
                f"wrong client request/result association: {key.name}",
                "ClientResultAssociationMismatch",
            )
            client_kinds[str(contract["result_contract_kind"])] += 1
        else:
            expected_server = SERVER_REQUEST_CONTRACTS[key.name]
            actual_server = (
                contract["parameter_type_identity"],
                contract["result_type_identity"],
            )
            require(
                actual_server == expected_server
                and contract["result_contract_kind"] == "Concrete",
                f"wrong server request/response association: {key.name}",
                "ServerResponseContractMismatch",
            )
        operations.append(
            {
                "protocol_surface_key": key.object(),
                "parameter_type": contract["parameter_type_identity"],
                "result_type": contract["result_type_identity"],
                "result_schema_type": contract.get(
                    "result_schema_type_identity"
                ),
                "result_kind": contract["result_contract_kind"],
                "association_evidence_kind": contract[
                    "association_evidence_kind"
                ],
                "association_evidence_key": contract[
                    "association_evidence_key"
                ],
                "cross_check_evidence": contract.get(
                    "cross_check_evidence", []
                ),
                "direct_raw_protocol_response": (
                    key.category == "server_request"
                ),
                "depends_on_server_request_resolved": False,
            }
        )
    require(
        dict(sorted(client_kinds.items())) == EXPECTED_RESULT_KINDS,
        f"client result-kind split changed: {dict(client_kinds)}",
        "ResultKindMismatch",
    )
    return operations


def _nullable(schema: Any) -> bool:
    return shared.allows_null(schema)


def _value_kind(schema: Any) -> str:
    if schema is True or schema == {}:
        return "opaque_json"
    if schema is False:
        return "never"
    if not isinstance(schema, Mapping):
        return "unconstrained"
    if "$ref" in schema:
        return "reference"
    if "oneOf" in schema or "anyOf" in schema:
        return "union"
    if "allOf" in schema:
        branches = schema.get("allOf")
        if (
            isinstance(branches, list)
            and len(branches) == 1
            and isinstance(branches[0], Mapping)
            and "$ref" in branches[0]
        ):
            return "reference"
        return "intersection"
    schema_type = schema.get("type")
    if isinstance(schema_type, list):
        values = [str(value) for value in schema_type if value != "null"]
        return "|".join(values) or "null"
    return str(schema_type or "opaque_json")


def collect_schema_paths(
    nodes: Mapping[fixtures.DefinitionId, Any],
    closure: set[fixtures.DefinitionId],
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    paths: list[dict[str, Any]] = []
    objects: list[dict[str, Any]] = []

    def transition(
        _parent: str | None,
        kind: str,
        token: str | int | None,
        _path: str,
        _schema: Any,
        _required: bool | None,
    ) -> str | None:
        return (
            str(token)
            if kind == schema_paths.PROPERTY
            and isinstance(token, str)
            else None
        )

    for definition in sorted(closure):
        root = nodes[definition]
        root_path = (
            f"#/definitions/"
            f"{'v2/' if definition.namespace == 'v2' else ''}"
            f"{definition.name}"
        )
        visits = list(
            schema_paths.walk_schema_paths(
                root,
                path=root_path,
                state=None,
                transition=transition,
                skip_references=True,
            )
        )
        object_candidates = [(root_path, root), *(
            (visit.path, visit.schema) for visit in visits
        )]
        for object_path, schema in object_candidates:
            if not isinstance(schema, Mapping):
                continue
            properties = schema.get("properties")
            if (
                schema.get("type") != "object"
                and not isinstance(properties, Mapping)
                and "additionalProperties" not in schema
            ):
                continue
            additional = schema.get(
                "additionalProperties", "allowed_by_default"
            )
            objects.append(
                {
                    "definition": definition.to_json(),
                    "schema_path": object_path,
                    "additional_properties": (
                        "schema"
                        if isinstance(additional, Mapping)
                        else additional
                    ),
                    "property_count": (
                        len(properties)
                        if isinstance(properties, Mapping)
                        else 0
                    ),
                }
            )
        for visit in visits:
            if visit.kind not in schema_paths.VALUE_PATH_KINDS:
                continue
            child = visit.schema
            child_mapping = (
                child if isinstance(child, Mapping) else {}
            )
            is_property = visit.kind == schema_paths.PROPERTY
            is_array = visit.kind == schema_paths.ARRAY_ELEMENT
            is_map = visit.kind == schema_paths.MAP_VALUE
            paths.append(
                {
                    "definition": definition.to_json(),
                    "schema_path": visit.path,
                    "schema_node_kind": visit.kind,
                    "field": visit.state if is_property else None,
                    "required": (
                        bool(visit.required)
                        if is_property
                        else None
                    ),
                    "optional": (
                        not bool(visit.required)
                        if is_property
                        else None
                    ),
                    "nullable": _nullable(child),
                    "default_present": "default" in child_mapping,
                    "default": child_mapping.get("default"),
                    "value_kind": _value_kind(child),
                    "integer_format": child_mapping.get("format"),
                    "minimum": child_mapping.get("minimum"),
                    "maximum": child_mapping.get("maximum"),
                    "array_element_type": (
                        _value_kind(child_mapping["items"])
                        if "items" in child_mapping
                        else (
                            _value_kind(child)
                            if is_array
                            else None
                        )
                    ),
                    "map_value_type": (
                        _value_kind(
                            child_mapping["additionalProperties"]
                        )
                        if "additionalProperties" in child_mapping
                        else (_value_kind(child) if is_map else None)
                    ),
                    "additional_properties": (
                        "schema"
                        if isinstance(
                            child_mapping.get("additionalProperties"),
                            Mapping,
                        )
                        else child_mapping.get(
                            "additionalProperties",
                            (
                                "allowed_by_default"
                                if (
                                    child_mapping.get("type") == "object"
                                    or "properties" in child_mapping
                                )
                                else "not_applicable"
                            ),
                        )
                    ),
                    "intentionally_opaque_json": (
                        _value_kind(child) == "opaque_json"
                    ),
                    "sensitive": (
                        is_property
                        and visit.state in SENSITIVE_FIELD_NAMES
                    ),
                }
            )
    return paths, objects


def _definition_for_type(
    catalog: fixtures.SchemaCatalog,
    nodes: Mapping[fixtures.DefinitionId, Any],
    type_identity: str,
    key: Key,
    role: str,
) -> fixtures.DefinitionId:
    definition = fixtures.locate_definition_for_type(
        catalog, nodes, type_identity
    )
    require(
        definition is not None,
        f"schema root is absent for {key.compact()} {role}",
        "MissingSchemaRoot",
    )
    return definition


def schema_closure(
    arguments: argparse.Namespace,
    inputs: Inputs,
    keys: Sequence[Key],
) -> dict[str, Any]:
    draft07 = fixtures.load_draft07(arguments.draft07_validator)
    catalog = fixtures.SchemaCatalog(arguments.schema_root, draft07)
    aggregate = catalog.load(
        arguments.schema_root
        / "stable/codex_app_server_protocol.schemas.json"
    )
    nodes, edges = fixtures.definition_graph(aggregate)
    seeds: dict[fixtures.DefinitionId, list[dict[str, Any]]] = defaultdict(
        list
    )
    identity_definitions: dict[Key, set[fixtures.DefinitionId]] = (
        defaultdict(set)
    )

    def add(
        definition: fixtures.DefinitionId, role: str, key: Key
    ) -> None:
        association = {"role": role, "surface_key": key.object()}
        if association not in seeds[definition]:
            seeds[definition].append(association)
        identity_definitions[key].update(
            fixtures.transitive_definitions((definition,), edges)
        )

    for key in keys:
        if key.category in {"client_request", "server_request"}:
            contract = inputs.contracts[key]
            parameter_type = str(contract["parameter_type_identity"])
            if parameter_type != "Unit":
                add(
                    _definition_for_type(
                        catalog,
                        nodes,
                        parameter_type,
                        key,
                        "request_params",
                    ),
                    "request_params",
                    key,
                )
            result_schema_type = str(
                contract.get("result_schema_type_identity")
                or contract["result_type_identity"]
            )
            if result_schema_type != "Unit":
                add(
                    _definition_for_type(
                        catalog,
                        nodes,
                        result_schema_type,
                        key,
                        "successful_response",
                    ),
                    "successful_response",
                    key,
                )
        elif key.category == "server_notification":
            params = inputs.manifest_rows[key].get("params")
            type_identity = (
                params.get("type")
                if isinstance(params, Mapping)
                else None
            )
            require(
                isinstance(type_identity, str) and bool(type_identity),
                f"notification has no stable params root: {key.compact()}",
                "ContractMismatch",
            )
            add(
                _definition_for_type(
                    catalog,
                    nodes,
                    type_identity,
                    key,
                    "notification_params",
                ),
                "notification_params",
                key,
            )
        else:
            candidates = [
                definition
                for definition in (
                    fixtures.DefinitionId("legacy", key.domain),
                    fixtures.DefinitionId("v2", key.domain),
                )
                if definition in nodes
            ]
            require(
                len(candidates) == 1,
                f"union schema namespace is ambiguous: {key.domain}",
                "UnionSchemaMismatch",
            )
            add(candidates[0], "registered_union_family", key)

    closure = set(fixtures.transitive_definitions(seeds.keys(), edges))
    paths, objects = collect_schema_paths(nodes, closure)
    union_rows: list[dict[str, Any]] = []
    for (domain, field), expected_alternatives in sorted(
        UNION_FAMILIES.items()
    ):
        alternatives = [
            key
            for key in keys
            if key.category == "tagged_union_discriminator"
            and key.domain == domain
        ]
        definition = next(
            value
            for value in (
                fixtures.DefinitionId("legacy", domain),
                fixtures.DefinitionId("v2", domain),
            )
            if value in nodes
        )
        schema = nodes[definition]
        branches = schema.get("oneOf")
        require(
            isinstance(branches, list),
            f"union lacks oneOf representation: {domain}",
            "UnionSchemaMismatch",
        )
        branch_rows: list[dict[str, Any]] = []
        observed: set[str] = set()
        for index, branch in enumerate(branches):
            properties = branch.get("properties", {})
            discriminator = properties.get(field, {})
            values = discriminator.get("enum", [])
            require(
                isinstance(values, list)
                and len(values) == 1
                and isinstance(values[0], str),
                f"union discriminator is malformed: {domain}/{index}",
                "UnionSchemaMismatch",
            )
            name = str(values[0])
            observed.add(name)
            required = set(branch.get("required", []))
            branch_rows.append(
                {
                    "alternative": name,
                    "branch_index": index,
                    "title": branch.get("title"),
                    "required_fields": sorted(required),
                    "optional_fields": sorted(
                        set(properties) - required
                    ),
                    "nullable_fields": sorted(
                        name
                        for name, child in properties.items()
                        if _nullable(child)
                    ),
                    "intentionally_opaque_fields": sorted(
                        name
                        for name, child in properties.items()
                        if _value_kind(child) == "opaque_json"
                    ),
                    "additional_properties": branch.get(
                        "additionalProperties", "allowed_by_default"
                    ),
                    "property_schemas": [
                        {
                            "field": name,
                            "required": name in required,
                            "nullable": _nullable(child),
                            "value_kind": _value_kind(child),
                            "default_present": (
                                isinstance(child, Mapping)
                                and "default" in child
                            ),
                        }
                        for name, child in sorted(properties.items())
                    ],
                }
            )
        require(
            observed == expected_alternatives,
            f"union alternatives changed: {domain} {sorted(observed)}",
            "UnionSchemaMismatch",
        )
        roots = {
            Key.from_row(root["surface_key"])
            for key in alternatives
            for root in inputs.reachability[key]["reaching_roots"]
        }
        outer_properties = schema.get("properties", {})
        outer_required = set(schema.get("required", []))
        union_rows.append(
            {
                "domain": domain,
                "definition": definition.to_json(),
                "representation": "Draft-07 object plus oneOf branches",
                "discriminator": field,
                "alternatives": branch_rows,
                "outer_required_fields": sorted(outer_required),
                "outer_optional_fields": sorted(
                    set(outer_properties) - outer_required
                ),
                "outer_nullable_fields": sorted(
                    name
                    for name, child in outer_properties.items()
                    if _nullable(child)
                ),
                "outer_additional_properties": schema.get(
                    "additionalProperties", "allowed_by_default"
                ),
                "reaching_roots": [
                    root.object() for root in sorted(roots)
                ],
                "future_unknown_handling": (
                    "preserve raw unknown alternative and diagnostic; "
                    "never coerce it to a known alternative"
                ),
                "malformed_known_handling": (
                    "reject a known discriminator with missing or "
                    "wrong-typed required fields"
                ),
            }
        )

    path_kinds = _counter(str(row["schema_node_kind"]) for row in paths)
    return {
        "format_version": FORMAT_VERSION,
        "generated_notice": (
            "Generated A1.4 transitive stable-schema review evidence."
        ),
        "codex_version": CODEX_VERSION,
        "counts": {
            "seed_definitions": len(seeds),
            "reachable_named_definitions": len(closure),
            "definition_namespaces": _counter(
                definition.namespace for definition in closure
            ),
            "schema_paths": len(paths),
            "schema_path_kinds": path_kinds,
            "object_nodes": len(objects),
            "required_paths": sum(
                row["schema_node_kind"] == "property"
                and bool(row["required"])
                for row in paths
            ),
            "optional_paths": sum(
                row["schema_node_kind"] == "property"
                and bool(row["optional"])
                for row in paths
            ),
            "nullable_paths": sum(bool(row["nullable"]) for row in paths),
            "default_bearing_paths": sum(
                bool(row["default_present"]) for row in paths
            ),
            "integer_formats": _counter(
                str(row["integer_format"])
                for row in paths
                if row["integer_format"] is not None
            ),
            "minimum_bearing_paths": sum(
                row["minimum"] is not None for row in paths
            ),
            "maximum_bearing_paths": sum(
                row["maximum"] is not None for row in paths
            ),
            "opaque_json_paths": sum(
                bool(row["intentionally_opaque_json"]) for row in paths
            ),
            "sensitive_paths": sum(
                bool(row["sensitive"]) for row in paths
            ),
        },
        "seed_definitions": [
            {
                "definition": definition.to_json(),
                "associations": sorted(
                    associations,
                    key=lambda row: (
                        row["role"],
                        Key.from_row(row["surface_key"]),
                    ),
                ),
            }
            for definition, associations in sorted(seeds.items())
        ],
        "definitions": [
            {
                "definition": definition.to_json(),
                "direct_dependencies": [
                    dependency.to_json()
                    for dependency in sorted(edges[definition] & closure)
                ],
                "schema_sha256": shared.sha256_json(nodes[definition]),
            }
            for definition in sorted(closure)
        ],
        "schema_paths": paths,
        "object_policies": objects,
        "union_families": union_rows,
        "identity_reachable_definition_counts": [
            {
                "protocol_surface_key": key.object(),
                "reachable_definition_count": len(
                    identity_definitions[key]
                ),
            }
            for key in keys
        ],
    }


def partition_document(
    arguments: argparse.Namespace,
    inputs: Inputs,
    keys: Sequence[Key],
    operations: Sequence[Mapping[str, Any]],
) -> dict[str, Any]:
    all_keys = sorted(inputs.manifest_rows)
    buckets: dict[str, list[dict[str, Any]]] = {}
    for slice_name in EXPECTED_PARTITION:
        slice_keys = [
            key
            for key in all_keys
            if inputs.assignments[key]["slice"] == slice_name
        ]
        buckets[slice_name] = [
            {
                "protocol_surface_key": key.object(),
                "stability": inputs.assignments[key]["stability"],
                "classification": inputs.assignments[key][
                    "classification"
                ],
                "module": inputs.assignments[key]["module"],
                "a1_slice": inputs.assignments[key]["slice"],
                "runtime_disposition": inputs.registry[key][
                    "runtime_disposition"
                ],
                "typed_status": inputs.registry[key]["typed_status"],
                "typed_schema_status": inputs.registry[key][
                    "typed_schema_status"
                ],
            }
            for key in slice_keys
        ]
    inventory = buckets["InventoryOnly"]
    native_status = _status_counter(inputs.registry[key] for key in keys)
    native_completion = dict(EXPECTED_GLOBAL_START_STATUS)
    native_completion["Complete"] += 56
    native_completion["NotImplemented"] -= 55
    native_completion["Partial"] -= 1
    final_a1 = dict(native_completion)
    final_a1["Complete"] += 3
    final_a1["Partial"] -= 3
    require(
        native_completion == EXPECTED_NATIVE_COMPLETION_STATUS,
        f"native completion arithmetic changed: {native_completion}",
        "NativeCompletionArithmeticMismatch",
    )
    require(
        final_a1 == EXPECTED_FINAL_A1_STATUS,
        f"final A1 arithmetic changed: {final_a1}",
        "FinalA1ArithmeticMismatch",
    )
    changed_paths: set[str] = set()
    if (arguments.repo_root / ".git").exists():
        base_diff = _try_git(
            arguments.repo_root,
            "diff",
            "--name-only",
            EXPECTED_BASE_SHA,
        )
        if base_diff is None:
            # Pull-request CI checks out a depth-one synthetic merge commit.
            # Its tree is complete even though the frozen base commit is not,
            # so compare the protected tree/blob objects directly.
            production_tree = _git(
                arguments.repo_root,
                "rev-parse",
                "HEAD:src",
            )
            require(
                production_tree == EXPECTED_PRODUCTION_TREE,
                (
                    "production tree changed in shallow checkout: "
                    f"{production_tree}"
                ),
                "ProductionScopeViolation",
            )
            fixture_tree = _git(
                arguments.repo_root,
                "rev-parse",
                "HEAD:tools/codex/app-server-fixtures/0.144.6",
            )
            frontend_protocol_blob = _git(
                arguments.repo_root,
                "rev-parse",
                (
                    "HEAD:docs/ai/openai/codex/"
                    "frontend-protocol-v1.schema.json"
                ),
            )
            require(
                fixture_tree == EXPECTED_FIXTURE_TREE
                and frontend_protocol_blob
                == EXPECTED_FRONTEND_PROTOCOL_BLOB,
                (
                    "protected fixture/frontend objects changed in "
                    "shallow checkout"
                ),
                "ProtectedInputScopeViolation",
            )
        else:
            changed_paths.update(
                line for line in base_diff.splitlines() if line
            )
        changed_paths.update(
            line
            for line in _git(
                arguments.repo_root,
                "ls-files",
                "--others",
                "--exclude-standard",
            ).splitlines()
            if line
        )
    production_paths = sorted(
        path for path in changed_paths if path.startswith("src/")
    )
    protected_nonproduction_paths = sorted(
        path
        for path in changed_paths
        if path.startswith(
            "tools/codex/app-server-fixtures/0.144.6/"
        )
        or path
        == "docs/ai/openai/codex/frontend-protocol-v1.schema.json"
    )
    require(
        not production_paths,
        f"audit branch changes production paths: {production_paths}",
        "ProductionScopeViolation",
    )
    require(
        not protected_nonproduction_paths,
        (
            "audit branch changes protected fixture/frontend inputs: "
            f"{protected_nonproduction_paths}"
        ),
        "ProtectedInputScopeViolation",
    )
    return {
        "format_version": FORMAT_VERSION,
        "generated_notice": (
            "Generated total partition guard; "
            "ProtocolSurfaceRegistryData.inc remains authoritative."
        ),
        "codex_version": CODEX_VERSION,
        "counts": {
            "total_inventory": len(all_keys),
            "partition": {
                name: len(rows) for name, rows in buckets.items()
            },
            "stability_accounting": EXPECTED_STABILITY_ACCOUNTING,
            "inventory_only_classifications": _counter(
                str(row["classification"]) for row in inventory
            ),
            "native_a1_4_taxonomy": _counter(
                key.category for key in keys
            ),
            "native_a1_4_result_kinds": _counter(
                str(row["result_kind"])
                for row in operations
                if row["protocol_surface_key"]["category"]
                == "client_request"
            ),
            "native_a1_4_start_status": native_status,
            "global_start_status": _status_counter(
                inputs.registry.values()
            ),
            "native_completion_projection": native_completion,
            "final_a1_projection": final_a1,
        },
        "buckets": buckets,
        "inventory_only": {
            classification: [
                row
                for row in inventory
                if row["classification"] == classification
            ]
            for classification in EXPECTED_INVENTORY_CLASSIFICATIONS
        },
        "native_a1_4_operations": list(operations),
        "native_partial_identity": NATIVE_PARTIAL.object(),
        "inherited_a1_0_partial_identities": [
            key.object() for key in sorted(INHERITED_PARTIALS)
        ],
        "stable_experimental_asymmetries": {
            "stable_process_notifications": [
                "process/exited",
                "process/outputDelta",
            ],
            "experimental_process_controls": sorted(
                EXPERIMENTAL_PROCESS_CONTROLS
            ),
            "stable_remote_control_notification": (
                "remoteControl/status/changed"
            ),
            "experimental_remote_control_controls": sorted(
                key.name
                for key in inputs.assignments
                if key.category == "client_request"
                and key.name.startswith("remoteControl/")
                and inputs.assignments[key]["classification"]
                == "ExperimentalInventoryOnly"
            ),
            "rule": (
                "A stable status/output notification never promotes a "
                "similarly named experimental outgoing control."
            ),
        },
        "bijection": {
            "inventory_keys": len(inputs.manifest_rows),
            "assignment_keys": len(inputs.assignments),
            "registry_keys": len(inputs.registry),
            "missing_assignments": [],
            "extra_assignments": [],
            "missing_registry_rows": [],
            "extra_registry_rows": [],
            "duplicate_assignments": [],
            "cross_slice_overlaps": [],
        },
        "audit_boundaries": {
            "changed_paths": sorted(FOUNDATION_CHANGED_PATHS),
            "production_paths_changed": production_paths,
            "protected_nonproduction_paths_changed": (
                protected_nonproduction_paths
            ),
            "runtime_implementation_added": False,
            "registry_status_promoted": False,
            "module_assignment_changed": False,
            "soversion_changed": False,
            "backend_state_changed": False,
            "backend_command_changed": False,
            "frontend_protocol_changed": False,
        },
    }


def build_foundation(
    arguments: argparse.Namespace,
) -> tuple[dict[str, Any], dict[str, Any]]:
    start_state = shared.load_json(arguments.start_state)
    validate_start_state(start_state, arguments)
    inputs = load_inputs(arguments)
    keys = validate_partition_inputs(inputs)
    validate_start_arithmetic(inputs, keys)
    provenance = validate_provenance(arguments)
    require(
        provenance == start_state.get("provenance"),
        "live provenance differs from frozen start state",
        "ProvenanceMismatch",
    )
    taxonomy = _counter(key.category for key in keys)
    require(
        taxonomy == EXPECTED_TAXONOMY,
        f"native A1.4 taxonomy changed: {taxonomy}",
        "TaxonomyMismatch",
    )
    operations = _operation_rows(inputs, keys)
    partition = partition_document(arguments, inputs, keys, operations)
    closure = schema_closure(arguments, inputs, keys)
    validate_foundation_reports(partition, closure)
    return partition, closure


def _batch_keys(batch_name: str) -> set[Key]:
    batch = IMPLEMENTATION_BATCHES[batch_name]
    keys = {
        *(
            Key("client_request", "ClientRequest", "method", name)
            for name in batch["client_requests"]
        ),
        *(
            Key(
                "server_notification",
                "ServerNotification",
                "method",
                name,
            )
            for name in batch["server_notifications"]
        ),
        *(
            Key("server_request", "ServerRequest", "method", name)
            for name in batch["server_requests"]
        ),
    }
    for family in batch["union_families"]:
        matches = [
            (domain, field, alternatives)
            for (domain, field), alternatives in UNION_FAMILIES.items()
            if domain == family
        ]
        require(
            len(matches) == 1,
            f"implementation batch names unknown union: {family}",
            "ImplementationPlanIdentityMismatch",
        )
        domain, field, alternatives = matches[0]
        keys.update(
            Key(
                "tagged_union_discriminator",
                domain,
                field,
                alternative,
            )
            for alternative in alternatives
        )
    return keys


def _definition_key(row: Mapping[str, Any]) -> tuple[str, str]:
    return (str(row["namespace"]), str(row["name"]))


def _definition_object(
    definition: tuple[str, str],
) -> dict[str, str]:
    return {
        "namespace": definition[0],
        "name": definition[1],
    }


def _batch_closure(
    batch_name: str,
    closure: Mapping[str, Any],
) -> dict[str, Any]:
    keys = _batch_keys(batch_name)
    seed_rows = closure["seed_definitions"]
    seeds = {
        _definition_key(row["definition"])
        for row in seed_rows
        if any(
            Key.from_row(association["surface_key"]) in keys
            for association in row["associations"]
        )
    }
    edges = {
        _definition_key(row["definition"]): {
            _definition_key(dependency)
            for dependency in row["direct_dependencies"]
        }
        for row in closure["definitions"]
    }
    reachable = set(seeds)
    pending = list(seeds)
    while pending:
        definition = pending.pop()
        for dependency in edges[definition]:
            if dependency not in reachable:
                reachable.add(dependency)
                pending.append(dependency)
    paths = [
        row
        for row in closure["schema_paths"]
        if _definition_key(row["definition"]) in reachable
    ]
    objects = [
        row
        for row in closure["object_policies"]
        if _definition_key(row["definition"]) in reachable
    ]
    path_kinds = _counter(
        str(row["schema_node_kind"]) for row in paths
    )
    for kind in ("array_element", "map_value", "property"):
        path_kinds.setdefault(kind, 0)
    counts = {
        "seed_definitions": len(seeds),
        "reachable_named_definitions": len(reachable),
        "definition_namespaces": _counter(
            definition[0] for definition in reachable
        ),
        "schema_paths": len(paths),
        "schema_path_kinds": dict(sorted(path_kinds.items())),
        "object_nodes": len(objects),
        "required_paths": sum(
            row["schema_node_kind"] == "property"
            and bool(row["required"])
            for row in paths
        ),
        "optional_paths": sum(
            row["schema_node_kind"] == "property"
            and bool(row["optional"])
            for row in paths
        ),
        "nullable_paths": sum(bool(row["nullable"]) for row in paths),
        "default_bearing_paths": sum(
            bool(row["default_present"]) for row in paths
        ),
        "opaque_json_paths": sum(
            bool(row["intentionally_opaque_json"]) for row in paths
        ),
        "sensitive_paths": sum(
            bool(row["sensitive"]) for row in paths
        ),
    }
    require(
        counts == EXPECTED_BATCH_CLOSURES[batch_name],
        f"{batch_name} transitive schema closure changed: {counts}",
        "ImplementationDependencyMismatch",
    )
    return {
        "counts": counts,
        "seed_definitions": [
            _definition_object(definition)
            for definition in sorted(seeds)
        ],
        "reachable_definitions": [
            _definition_object(definition)
            for definition in sorted(reachable)
        ],
        "sensitive_schema_paths": [
            {
                "definition": row["definition"],
                "schema_path": row["schema_path"],
                "field": row["field"],
                "value_kind": row["value_kind"],
            }
            for row in paths
            if row["sensitive"]
        ],
    }


def _parse_variant(path: Path, alias: str) -> list[str]:
    source = path.read_text(encoding="utf-8")
    match = re.search(
        rf"using\s+{re.escape(alias)}\s*=\s*"
        r"std::variant<(?P<body>.*?)>;",
        source,
        flags=re.DOTALL,
    )
    require(
        match is not None,
        f"unable to locate public variant {alias} in {path}",
        "PublicVariantMismatch",
    )
    alternatives = [
        alternative.strip()
        for alternative in match.group("body").split(",")
    ]
    require(
        all(
            alternative
            and "<" not in alternative
            and ">" not in alternative
            for alternative in alternatives
        ),
        f"{alias} is no longer a flat named-alternative variant",
        "PublicVariantMismatch",
    )
    return alternatives


def _indexed(values: Sequence[str]) -> list[dict[str, Any]]:
    return [
        {"index": index, "type": value}
        for index, value in enumerate(values)
    ]


def public_variant_inventory(
    arguments: argparse.Namespace,
) -> dict[str, Any]:
    events_path = (
        arguments.repo_root / "src/ai/openai/codex/typed/Events.h"
    )
    requests_path = (
        arguments.repo_root
        / "src/ai/openai/codex/typed/ServerRequests.h"
    )
    canonical = _parse_variant(
        events_path, "CanonicalServerNotification"
    )
    events = _parse_variant(events_path, "Event")
    requests = _parse_variant(
        requests_path, "TypedServerRequest"
    )
    require(
        len(canonical) == 51
        and len(events) == 53
        and len(requests) == 8,
        "current public variant denominators changed",
        "PublicVariantMismatch",
    )
    require(
        events[44] == "TurnErrorEvent"
        and events[45] == "UnknownEvent"
        and requests[2] == "UserInputRequest"
        and requests[4] == "UnknownServerRequest",
        "ABI-sensitive public variant indices changed",
        "PublicVariantMismatch",
    )
    return {
        "current": {
            "CanonicalServerNotification": _indexed(canonical),
            "Event": _indexed(events),
            "TypedServerRequest": _indexed(requests),
        },
        "frozen_indices": {
            "TurnErrorEvent": 44,
            "UnknownEvent": 45,
            "UserInputRequest": 2,
            "UnknownServerRequest": 4,
        },
        "planned_notification_appends": [
            {
                "method": method,
                "canonical_index": canonical_index,
                "event_index": event_index,
            }
            for method, canonical_index, event_index in EVENT_ADDITIONS
        ],
        "planned_server_request_changes": [
            {"method": method, **details}
            for method, details in sorted(
                SERVER_REQUEST_ADDITIONS.items()
            )
        ],
        "final_a1_error_completion": {
            "canonical_type": "ErrorNotification",
            "canonical_index": 67,
            "event_change": (
                "TurnErrorEvent remains index 44 and gains an optional "
                "canonical view; no Event alternative is inserted"
            ),
        },
        "sources": [
            _source(events_path, arguments.repo_root),
            _source(requests_path, arguments.repo_root),
        ],
    }


def cross_slice_ledger_document(
    arguments: argparse.Namespace,
    inputs: Inputs,
) -> dict[str, Any]:
    assignment_source = _source(
        arguments.assignments, arguments.repo_root
    )

    def entry(
        key: Key,
        completion_stage: str,
        expected_batch: str,
    ) -> dict[str, Any]:
        assignment = inputs.assignments[key]
        registry = inputs.registry[key]
        false_facts = sorted(
            name
            for name, value in registry[
                "schema_completeness"
            ].items()
            if value is False
        )
        obligations = PARTIAL_OBLIGATIONS[key.name]
        return {
            "protocol_surface_key": key.object(),
            "category": key.category,
            "current_module": assignment["module"],
            "current_slice": assignment["slice"],
            "current_status": registry["typed_schema_status"],
            "current_runtime_disposition": registry[
                "runtime_disposition"
            ],
            "current_runtime_target": registry["runtime_target"],
            "missing_schema_completeness_facts": false_facts,
            "missing_public_type_obligations": obligations[
                "public_type_obligations"
            ],
            "missing_codec_obligations": obligations[
                "codec_obligations"
            ],
            "missing_test_obligations": obligations[
                "test_obligations"
            ],
            "intended_completion_batch": expected_batch,
            "completion_stage": completion_stage,
            "expected_status_after_completion": "Complete",
            "deferred_reason": obligations["deferred_reason"],
            "assignment_unchanged_proof": {
                "authority": assignment_source,
                "assignment": {
                    "stability": assignment["stability"],
                    "classification": assignment["classification"],
                    "module": assignment["module"],
                    "slice": assignment["slice"],
                },
                "registry_matches_assignment": (
                    registry["typed_module"] == assignment["module"]
                    and registry["a1_slice"] == assignment["slice"]
                ),
            },
        }

    inherited = [
        entry(
            key,
            "final A1 closure in A1.4",
            "A1-FinalClosure",
        )
        for key in sorted(INHERITED_PARTIALS)
    ]
    native = entry(
        NATIVE_PARTIAL,
        "native A1.4 implementation",
        "A14-McpReverse",
    )
    require(
        {
            row["protocol_surface_key"]["name"] for row in inherited
        }
        == {"initialize", "initialized", "error"}
        and all(
            row["current_module"] == "Common"
            and row["current_slice"] == "A1.0"
            and row["current_status"] == "Partial"
            for row in inherited
        ),
        "inherited A1.0 completion ownership changed",
        "CrossSliceOwnershipMismatch",
    )
    require(
        native["current_module"] == A1_4_MODULE
        and native["current_slice"] == A1_4_SLICE
        and native["current_status"] == "Partial",
        "native A1.4 partial ownership changed",
        "NativePartialMismatch",
    )
    return {
        "format_version": FORMAT_VERSION,
        "generated_notice": (
            "Generated cross-slice completion ledger; ownership remains "
            "defined by module-slice-assignment.json."
        ),
        "codex_version": CODEX_VERSION,
        "counts": {
            "inherited_a1_0_partial": len(inherited),
            "native_a1_4_partial": 1,
            "all_current_partial": len(inherited) + 1,
        },
        "ownership_rule": (
            "A1.4 is the scheduling milestone for final Common closure, "
            "not the ownership slice of initialize, initialized, or error."
        ),
        "inherited_final_a1_obligations": inherited,
        "native_a1_4_obligation": native,
        "native_denominator_rule": (
            "The three inherited rows are excluded from the native "
            "56-identity A1.4 denominator."
        ),
        "assignment_file_unchanged": True,
        "assignment_authority": assignment_source,
    }


def server_request_resolved_document(
    arguments: argparse.Namespace,
) -> dict[str, Any]:
    schema_path = (
        arguments.schema_root
        / "stable/v2/ServerRequestResolvedNotification.json"
    )
    schema = shared.load_json(schema_path)
    properties = schema.get("properties")
    require(
        schema.get("title") == "ServerRequestResolvedNotification"
        and schema.get("type") == "object"
        and set(schema.get("required", []))
        == {"requestId", "threadId"}
        and isinstance(properties, Mapping)
        and set(properties) == {"requestId", "threadId"}
        and "additionalProperties" not in schema,
        "serverRequest/resolved payload schema changed",
        "ServerRequestResolvedSemanticsMismatch",
    )
    request_id = schema.get("definitions", {}).get("RequestId", {})
    request_id_types = {
        (
            str(branch.get("type")),
            branch.get("format"),
        )
        for branch in request_id.get("anyOf", [])
        if isinstance(branch, Mapping)
    }
    require(
        request_id_types
        == {("string", None), ("integer", "int64")}
        and properties["threadId"] == {"type": "string"}
        and properties["requestId"]
        == {"$ref": "#/definitions/RequestId"},
        "serverRequest/resolved field types changed",
        "ServerRequestResolvedSemanticsMismatch",
    )
    commit_url = (
        "https://github.com/openai/codex/blob/"
        f"{UPSTREAM_SOURCE_COMMIT}/"
    )
    method_matrix = [
        {
            "slice": "A1.3",
            "method": "item/commandExecution/requestApproval",
            "emits_notification": True,
        },
        {
            "slice": "A1.3",
            "method": "item/fileChange/requestApproval",
            "emits_notification": True,
        },
        {
            "slice": "A1.3",
            "method": "item/permissions/requestApproval",
            "emits_notification": True,
        },
        {
            "slice": "A1.3",
            "method": "applyPatchApproval",
            "emits_notification": False,
        },
        {
            "slice": "A1.3",
            "method": "execCommandApproval",
            "emits_notification": False,
        },
        {
            "slice": "A1.4",
            "method": "item/tool/requestUserInput",
            "emits_notification": True,
        },
        {
            "slice": "A1.4",
            "method": "mcpServer/elicitation/request",
            "emits_notification": True,
        },
        {
            "slice": "A1.4",
            "method": "item/tool/call",
            "emits_notification": False,
        },
        {
            "slice": "A1.4",
            "method": "attestation/generate",
            "emits_notification": False,
        },
    ]
    return {
        "method": "serverRequest/resolved",
        "payload": {
            "type": "ServerRequestResolvedNotification",
            "required_fields": {
                "threadId": "string",
                "requestId": "string | int64",
            },
            "optional_fields": [],
            "nullable_fields": [],
            "additional_properties": "allowed_by_default",
            "absent_semantics": [
                "method",
                "item ID",
                "result",
                "error",
                "outcome",
                "generation",
                "completion status",
            ],
            "schema_authority": _source(
                schema_path, arguments.repo_root
            ),
        },
        "identifier_semantics": {
            "kind": "original server-generated JSON-RPC request ID",
            "not": [
                "item ID",
                "call ID",
                "SNode.C occurrence token",
            ],
            "preserve_string_ids": True,
            "upstream_current_generation": (
                "process-wide monotonically allocated integer"
            ),
        },
        "generation_semantics": {
            "payload_generation_scoped": False,
            "recipient_rule": (
                "scope the notification to the SNode.C transport "
                "generation that delivered it"
            ),
            "restart_risk": (
                "the upstream process counter can restart; an old "
                "generation must never consume a new occurrence"
            ),
        },
        "emission_method_matrix": method_matrix,
        "terminal_path_matrix": [
            {
                "path": "successful JSON-RPC result",
                "emitted_for_positive_methods": True,
            },
            {
                "path": "domain decline/cancel encoded as valid result",
                "emitted_for_positive_methods": True,
            },
            {
                "path": "client JSON-RPC rejection/error",
                "emitted_for_positive_methods": True,
            },
            {
                "path": "malformed successful result",
                "emitted_for_positive_methods": True,
                "note": "emission precedes fallback result decoding",
            },
            {
                "path": "turn start/complete/interrupt cleanup",
                "emitted_for_positive_methods": True,
            },
            {
                "path": "generic timeout",
                "emitted_for_positive_methods": False,
                "note": (
                    "the five positive handlers have no generic timer; "
                    "autoResolutionMs is only forwarded"
                ),
            },
            {
                "path": "disconnect alone",
                "emitted_for_positive_methods": False,
            },
            {
                "path": "server-side auto-resolution",
                "emitted_for_positive_methods": False,
                "note": "no such App Server timer exists at this pin",
            },
        ],
        "ordering": {
            "causal_sequence": [
                "client enqueues direct JSON-RPC result or error",
                "App Server removes the matching callback",
                "the per-method waiter wakes",
                "the handler queues ResolveServerRequest",
                "the listener emits serverRequest/resolved",
                "the handler resumes core processing",
            ],
            "direct_response_is_earlier": True,
            "notification_is_opposite_direction": True,
            "notification_precedes_downstream_completion": (
                "ordinary positive-handler paths"
            ),
        },
        "meaning": {
            "classification": "informational and lifecycle-significant",
            "communicates": (
                "a shared pending request was resolved or cleared"
            ),
            "does_not_communicate": (
                "success, decline, cancellation, rejection, or outcome"
            ),
            "outcome_authority": (
                "downstream item/completed or turn/completed"
            ),
        },
        "recipient_behavior": {
            "already_consumed_possible": True,
            "multi_subscriber_case": (
                "the responder usually removed its occurrence while "
                "another subscriber may still have the shared ID pending"
            ),
            "current_pending_match": (
                "retire idempotently as externally resolved only when "
                "ID, thread, and transport generation all match"
            ),
            "unknown_or_stale": (
                "deliver the typed event, perform no registry mutation, "
                "and keep the connection healthy"
            ),
            "duplicate": "nonfatal idempotent no-op",
            "thread_mismatch": "nonfatal no-op",
            "later_answer_after_external_resolution": (
                "fail locally without wire output"
            ),
        },
        "direct_response_non_regression": {
            "frozen_path": [
                "incoming server request",
                "existing occurrence token and generation",
                "typed callback",
                "typed response validation",
                (
                    "direct JSON-RPC response using the original "
                    "request ID"
                ),
            ],
            "is_response_transport": False,
            "is_response_prerequisite": False,
            "is_acknowledgement_prerequisite": False,
            "is_second_terminal_completion": False,
            "can_reopen_completed_occurrence": False,
            "changes_a1_3_response_contract": False,
        },
        "future_tests": [
            "integer and string IDs; required/null/wrong-type fields",
            "future additional properties are accepted and preserved",
            "positive five-method and negative four-method matrices",
            "success, decline/cancel, rejection, malformed-result paths",
            "turn-transition cleanup behavior",
            "response bytes and local consumption precede reentrant event",
            "typed responses remain valid if the event never arrives",
            "already-consumed event causes no second terminal action",
            "external resolution retires a current matching occurrence",
            "unknown, duplicate, thread-mismatched IDs are nonfatal",
            "old-generation event cannot consume a restarted occurrence",
            "multi-subscriber responder/observer behavior",
            "resolved precedes downstream completion where promised",
            "reuse A1.3 original-ID, one-shot, and generation tests",
        ],
        "pinned_upstream_evidence": [
            {
                "finding": "payload struct",
                "url": (
                    commit_url
                    + "codex-rs/app-server-protocol/src/protocol/"
                    "v2/notification.rs#L50-L56"
                ),
            },
            {
                "finding": "RequestId representation",
                "url": (
                    commit_url
                    + "codex-rs/app-server-protocol/src/rpc.rs#L13-L29"
                ),
            },
            {
                "finding": "ID generation and callback insertion",
                "url": (
                    commit_url
                    + "codex-rs/app-server/src/"
                    "outgoing_message.rs#L282-L350"
                ),
            },
            {
                "finding": "callback removal before waiter wake-up",
                "url": (
                    commit_url
                    + "codex-rs/app-server/src/"
                    "outgoing_message.rs#L373-L410"
                ),
            },
            {
                "finding": "single notification emission site",
                "url": (
                    commit_url
                    + "codex-rs/app-server/src/request_processors/"
                    "thread_lifecycle.rs#L748-L771"
                ),
            },
            {
                "finding": "five positive response handlers",
                "url": (
                    commit_url
                    + "codex-rs/app-server/src/"
                    "bespoke_event_handling.rs#L1569-L1923"
                ),
            },
            {
                "finding": "dynamic tool negative path",
                "url": (
                    commit_url
                    + "codex-rs/app-server/src/"
                    "dynamic_tools.rs#L16-L55"
                ),
            },
            {
                "finding": "attestation negative/timeout path",
                "url": (
                    commit_url
                    + "codex-rs/app-server/src/"
                    "attestation.rs#L63-L127"
                ),
            },
            {
                "finding": "turn-transition callback cancellation",
                "url": (
                    commit_url
                    + "codex-rs/app-server/src/"
                    "outgoing_message.rs#L176-L190"
                ),
            },
            {
                "finding": "disconnect behavior",
                "url": (
                    commit_url
                    + "codex-rs/app-server/src/"
                    "message_processor.rs#L695-L721"
                ),
            },
            {
                "finding": "listener serialization",
                "url": (
                    commit_url
                    + "codex-rs/app-server/src/"
                    "thread_state.rs#L164-L193"
                ),
            },
            {
                "finding": "documented lifecycle meaning",
                "url": (
                    commit_url
                    + "codex-rs/app-server/README.md#L1444-L1497"
                ),
            },
        ],
        "local_non_regression_evidence": [
            {
                "path": (
                    "src/ai/openai/codex/AppServerClient.cpp"
                ),
                "finding": (
                    "answerServerRequest enqueues the original ID and "
                    "consumes the occurrence before reentrant flush"
                ),
            },
            {
                "path": (
                    "tools/codex/app-server-evidence/0.144.6/"
                    "a1-3-implementation-plan.json"
                ),
                "finding": (
                    "all A1.3 response operations explicitly exclude "
                    "serverRequest/resolved as a transport dependency"
                ),
            },
        ],
    }


def _soversion_inventory(
    arguments: argparse.Namespace,
) -> dict[str, Any]:
    root_cmake = arguments.repo_root / "CMakeLists.txt"
    root_source = root_cmake.read_text(encoding="utf-8")
    require(
        re.search(
            r"project\(\s*SNode\.C\b.*?\bVERSION\s+1\.0\.1\s*\)",
            root_source,
            flags=re.DOTALL,
        )
        is not None
        and "set(SNODEC_SOVERSION ${SNode.C_VERSION_MAJOR})"
        in root_source,
        "root SOVERSION authority changed",
        "SOVERSIONDecisionMissing",
    )
    declaration_count = 0
    declaration_paths: list[str] = []
    for path in arguments.repo_root.rglob("CMakeLists.txt"):
        relative = path.relative_to(arguments.repo_root)
        if any(
            part == ".git"
            or part == "_deps"
            or part.startswith("build")
            for part in relative.parts
        ):
            continue
        count = path.read_text(encoding="utf-8").count(
            "SOVERSION ${SNODEC_SOVERSION}"
        )
        if count:
            declaration_count += count
            declaration_paths.extend([relative.as_posix()] * count)
    require(
        declaration_count == 68,
        (
            "global SNODEC_SOVERSION declaration count changed: "
            f"{declaration_count}"
        ),
        "SOVERSIONDecisionMissing",
    )
    codex_paths = {
        "src/ai/openai/codex/CMakeLists.txt",
        "src/ai/openai/codex/backend/CMakeLists.txt",
        "src/ai/openai/codex/frontend/CMakeLists.txt",
    }
    require(
        {
            path for path in declaration_paths if path in codex_paths
        }
        == codex_paths
        and sum(path in codex_paths for path in declaration_paths) == 3,
        "the three Codex shared-library SOVERSION consumers changed",
        "SOVERSIONDecisionMissing",
    )
    a1_3_abi_path = (
        arguments.evidence_root / "a1-3-api-abi-evidence.json"
    )
    a1_3_abi = shared.load_json(a1_3_abi_path)
    require(
        a1_3_abi.get("conclusion", {}).get("binary_compatible")
        is False
        and a1_3_abi.get("conclusion", {}).get("soversion") == 1
        and "TypedServerRequest=312"
        in a1_3_abi["layout_probe"]["base"]["stdout_lines"]
        and "TypedServerRequest=960"
        in a1_3_abi["layout_probe"]["final"]["stdout_lines"],
        "A1.3 ABI evidence no longer proves the A1 rebuild boundary",
        "SOVERSIONDecisionMissing",
    )
    return {
        "current_authority": {
            "project_version": "1.0.1",
            "SNODEC_SOVERSION": 1,
            "definition": (
                "SNODEC_SOVERSION = SNode.C_VERSION_MAJOR"
            ),
            "root_source": _source(
                root_cmake, arguments.repo_root
            ),
            "declarations_using_global_authority": declaration_count,
            "codex_declarations": sorted(codex_paths),
            "unrelated_declarations": declaration_count - 3,
        },
        "existing_a1_policy": {
            "source": (
                "docs/ai/openai/codex/a1-typed-foundation.md"
                "#final-closure-policy"
            ),
            "decision": (
                "remain at 1 through native A1.4 implementation, then "
                "bump once at final A1 closure"
            ),
        },
        "actual_abi_evidence": {
            "source": _source(
                a1_3_abi_path, arguments.repo_root
            ),
            "binary_compatible": False,
            "installed_consumers_must_rebuild": True,
            "base_typed_server_request_size": 312,
            "current_typed_server_request_size": 960,
            "current_app_server_client_size": 16,
            "current_typed_client_size": 8,
            "removed_symbols_at_a1_3_boundary": 6286,
            "added_symbols_at_a1_3_boundary": 15733,
        },
        "final_action": {
            "decision": "bump Codex SOVERSION 1 -> 2",
            "scope": sorted(
                [
                    "ai-openai-codex",
                    "ai-openai-codex-backend",
                    "ai-openai-codex-frontend",
                ]
            ),
            "mechanism": (
                "introduce a Codex-specific SOVERSION authority at "
                "final closure; do not bump 65 unrelated targets"
            ),
            "sequence_point": (
                "after the 339 Complete / 0 Partial / 0 "
                "NotImplemented / 48 NotApplicable proof, before final "
                "ABI and package capture"
            ),
            "binary_package_change": ".so.1 -> .so.2",
            "installed_consumer_action": (
                "rebuild and relink every installed consumer"
            ),
            "earlier_pr_rule": (
                "an unchanged SONAME 1 is not a binary-compatibility "
                "claim during native A1.4 implementation"
            ),
        },
        "required_layout_probes": [
            "AppServerClient",
            "typed::Client",
            "Event",
            "CanonicalServerNotification",
            "TypedServerRequest",
            "UserInputRequest",
            "TurnErrorEvent",
            "ClientInfo",
            "InitializeResult",
            "every introduced public aggregate and variant",
            "every new typed facade object",
        ],
        "expected_api_abi_changes": {
            "variants_gain_alternatives": [
                "CanonicalServerNotification",
                "Event",
                "TypedServerRequest",
                "PluginSource",
                "McpServerElicitationRequestParams",
            ],
            "aggregates_may_grow": [
                "ClientInfo",
                "TurnErrorEvent",
                "UserInputRequest",
                "notification and operation result aggregates",
            ],
            "symbols_added": [
                "typed::Client facade accessors",
                "typed operation methods",
                "typed reverse-request response overloads",
                "new aggregate and variant support symbols",
            ],
            "intentional_symbols_removed": [],
        },
    }


def implementation_plan_document(
    arguments: argparse.Namespace,
    inputs: Inputs,
    partition: Mapping[str, Any],
    closure: Mapping[str, Any],
    ledger: Mapping[str, Any],
) -> dict[str, Any]:
    batch_names = list(IMPLEMENTATION_BATCHES)
    key_batches: dict[Key, list[str]] = defaultdict(list)
    batch_closures: dict[str, dict[str, Any]] = {}
    definition_batches: dict[tuple[str, str], list[str]] = defaultdict(
        list
    )
    for batch_name in batch_names:
        for key in _batch_keys(batch_name):
            key_batches[key].append(batch_name)
        batch_closures[batch_name] = _batch_closure(
            batch_name, closure
        )
        for row in batch_closures[batch_name][
            "reachable_definitions"
        ]:
            definition_batches[_definition_key(row)].append(batch_name)
    require(
        set(key_batches) == expected_a1_4_keys(),
        "implementation batches do not cover the native A1.4 slice",
        "ImplementationPlanIdentityMissing",
    )
    require(
        all(len(owners) == 1 for owners in key_batches.values()),
        "an A1.4 identity is owned by multiple implementation batches",
        "ImplementationPlanIdentityOverlap",
    )
    facade_accessors = {
        "Apps": "apps",
        "ExternalAgents": "externalAgents",
        "Feedback": "feedback",
        "Hooks": "hooks",
        "Marketplace": "marketplace",
        "Mcp": "mcp",
        "Plugins": "plugins",
        "Skills": "skills",
        "WindowsSandbox": "windowsSandbox",
    }
    client_api_rows = [
        {
            "method": method,
            "facade": facade,
            "client_accessor": (
                f"typed::Client::{facade_accessors[facade]}"
            ),
            "operation": operation,
            "implementation_batch": key_batches[
                Key(
                    "client_request",
                    "ClientRequest",
                    "method",
                    method,
                )
            ][0],
        }
        for facade, operations in FACADE_METHODS.items()
        for operation, method in operations.items()
    ]
    require(
        {row["method"] for row in client_api_rows}
        == set(CLIENT_REQUEST_CONTRACTS)
        and len(client_api_rows) == 29,
        "public facade plan does not own all 29 client requests once",
        "PublicApiOwnershipMismatch",
    )
    notification_rows = [
        {
            "method": method,
            "owner": "typed::Events",
            "canonical_variant_index": canonical_index,
            "event_variant_index": event_index,
            "implementation_batch": key_batches[
                Key(
                    "server_notification",
                    "ServerNotification",
                    "method",
                    method,
                )
            ][0],
        }
        for method, canonical_index, event_index in EVENT_ADDITIONS
    ]
    request_rows = [
        {
            "method": method,
            "owner": "typed::Requests",
            "occurrence_machinery": "existing occurrence token/generation",
            "response_transport": (
                "existing direct RawProtocol JSON-RPC response"
            ),
            "implementation_batch": key_batches[
                Key(
                    "server_request",
                    "ServerRequest",
                    "method",
                    method,
                )
            ][0],
            **details,
        }
        for method, details in sorted(
            SERVER_REQUEST_ADDITIONS.items()
        )
    ]
    operation_rows = {
        Key.from_row(row["protocol_surface_key"]): row
        for row in partition["native_a1_4_operations"]
    }
    partition_rows = {
        Key.from_row(row["protocol_surface_key"]): row
        for row in partition["buckets"]["A1.4"]
    }
    primary_definition_owner = {
        definition: owners[0]
        for definition, owners in definition_batches.items()
    }
    batch_documents: list[dict[str, Any]] = []
    for batch_name in batch_names:
        batch = IMPLEMENTATION_BATCHES[batch_name]
        keys = sorted(_batch_keys(batch_name))
        taxonomy = _counter(key.category for key in keys)
        for category in EXPECTED_TAXONOMY:
            taxonomy.setdefault(category, 0)
        require(
            len(keys) == batch["identity_count"]
            and dict(sorted(taxonomy.items())) == batch["taxonomy"],
            f"{batch_name} identity denominator/taxonomy changed",
            "ImplementationPlanIdentityMismatch",
        )
        result_kinds = _counter(
            CLIENT_REQUEST_CONTRACTS[key.name][2]
            for key in keys
            if key.category == "client_request"
        )
        if not result_kinds:
            result_kinds = {}
        require(
            {
                kind: result_kinds.get(kind, 0)
                for kind in ("Concrete", "Unit")
            }
            == batch["client_result_kinds"],
            f"{batch_name} client result split changed",
            "ResultKindMismatch",
        )
        closure_document = batch_closures[batch_name]
        reachable = {
            _definition_key(row)
            for row in closure_document["reachable_definitions"]
        }
        owned = sorted(
            definition
            for definition in reachable
            if primary_definition_owner[definition] == batch_name
        )
        reused = sorted(
            definition
            for definition in reachable
            if primary_definition_owner[definition] != batch_name
        )
        identity_rows = [
            {
                "protocol_surface_key": key.object(),
                "current_runtime_disposition": partition_rows[key][
                    "runtime_disposition"
                ],
                "current_status": partition_rows[key][
                    "typed_schema_status"
                ],
                "operation_contract": (
                    operation_rows[key]
                    if key in operation_rows
                    else None
                ),
                "registry_promotion_at_honest_closure": (
                    "Complete"
                ),
            }
            for key in keys
        ]
        batch_documents.append(
            {
                "batch": batch_name,
                "branch": batch["branch"],
                "draft_pr_title": batch["title"],
                "identity_count": len(keys),
                "taxonomy": batch["taxonomy"],
                "client_result_kinds": batch[
                    "client_result_kinds"
                ],
                "owned_start_status": _counter(
                    partition_rows[key]["typed_schema_status"]
                    for key in keys
                ),
                "native_metrics": {
                    "start": batch["native_start"],
                    "end": batch["native_end"],
                },
                "global_metrics": {
                    "start": batch["global_start"],
                    "end": batch["global_end"],
                },
                "identities": identity_rows,
                "schema_closure": closure_document,
                "primary_owned_schema_definitions": [
                    _definition_object(definition)
                    for definition in owned
                ],
                "reused_schema_dependencies": [
                    {
                        "definition": _definition_object(definition),
                        "primary_owner": primary_definition_owner[
                            definition
                        ],
                    }
                    for definition in reused
                ],
                "owned_public_headers": batch["headers"],
                "proposed_facades_and_channels": batch["facades"],
                "codec_units": batch["codec_units"],
                "descriptor_changes": batch[
                    "descriptor_changes"
                ],
                "registry_promotions": [
                    key.object() for key in keys
                ],
                "primary_tests": batch["primary_tests"],
                "lifecycle_tests": (
                    [
                        (
                            "all nine A1.3/A1.4 typed requests "
                            "concurrent and out of order"
                        ),
                        (
                            "original request ID, one-shot token, "
                            "generation and reconnect isolation"
                        ),
                    ]
                    if batch_name == "A14-McpReverse"
                    else (
                        [
                            (
                                "direct response versus "
                                "serverRequest/resolved ordering"
                            ),
                            (
                                "unknown/stale/duplicate/external "
                                "resolution handling"
                            ),
                        ]
                        if batch_name == "A14-RuntimePlatform"
                        else []
                    )
                ),
                "package_tests": [
                    "public headers are self-contained",
                    "installed consumer compiles and links",
                    "source archive retains implementation evidence",
                    "binary archive excludes private evidence",
                ],
                "security_obligations": [
                    (
                        "redact every listed sensitive schema path "
                        "and every raw/opaque JSON value"
                    ),
                    (
                        "diagnostics may contain only method, schema "
                        "path, expected type, and safe counts"
                    ),
                    (
                        "fixtures use synthetic IDs, /synthetic paths, "
                        "example.invalid URLs, and fake tokens"
                    ),
                ],
                "api_abi_impact": (
                    "public headers, aggregates, facade symbols, and "
                    "variants may grow; unchanged SONAME 1 is not a "
                    "binary-compatibility claim"
                ),
                "logical_commit_subjects": batch[
                    "logical_commits"
                ],
                "dependencies": batch["dependencies"],
                "honest_closure_rule": (
                    "promote an identity only after its complete "
                    "transitive types, codecs, descriptors, API, and "
                    "tests are present in this PR"
                ),
            }
        )
    overlaps = [
        {
            "definition": _definition_object(definition),
            "batches": owners,
            "primary_owner": primary_definition_owner[definition],
        }
        for definition, owners in sorted(definition_batches.items())
        if len(owners) > 1
    ]
    require(
        overlaps
        == [
            {
                "definition": {
                    "namespace": "v2",
                    "name": "AbsolutePathBuf",
                },
                "batches": [
                    "A14-UserIntegrations",
                    "A14-RuntimePlatform",
                ],
                "primary_owner": "A14-UserIntegrations",
            }
        ],
        f"cross-batch schema overlap changed: {overlaps}",
        "ImplementationDependencyMismatch",
    )
    resolved = server_request_resolved_document(arguments)
    variants = public_variant_inventory(arguments)
    soversion = _soversion_inventory(arguments)
    runtime_matrix_pairs = (
        ("Deferred", "NotImplemented"),
        ("RawPreserved", "NotImplemented"),
        ("Typed", "Partial"),
        ("OpaquePreserved", "NotImplemented"),
        ("Deferred", "NotApplicable"),
        ("RawPreserved", "NotApplicable"),
        ("OpaquePreserved", "NotApplicable"),
    )
    global_runtime_matrix = [
        {
            "runtime_disposition": disposition,
            "typed_schema_status": status,
            "count": sum(
                row["runtime_disposition"] == disposition
                and row["typed_schema_status"] == status
                for rows in partition["buckets"].values()
                for row in rows
            ),
        }
        for disposition, status in runtime_matrix_pairs
    ]
    native_runtime_matrix = [
        {
            "runtime_disposition": disposition,
            "typed_schema_status": status,
            "count": sum(
                row["runtime_disposition"] == disposition
                and row["typed_schema_status"] == status
                for row in partition["buckets"]["A1.4"]
            ),
        }
        for disposition, status in runtime_matrix_pairs
        if status != "NotApplicable"
    ]
    return {
        "format_version": FORMAT_VERSION,
        "generated_notice": (
            "Generated implementation and final-A1 closure plan. "
            "Nothing in this report promotes production status."
        ),
        "authority": {
            "base_sha": EXPECTED_BASE_SHA,
            "base_tree": EXPECTED_BASE_TREE,
            "codex_version": CODEX_VERSION,
            "upstream_tag": UPSTREAM_TAG,
            "upstream_source_commit": UPSTREAM_SOURCE_COMMIT,
            "partition_report": _source(
                arguments.partition_output, arguments.repo_root
            ),
            "type_closure_report": _source(
                arguments.closure_output, arguments.repo_root
            ),
            "cross_slice_ledger": (
                "tools/codex/app-server-evidence/0.144.6/"
                "a1-final-cross-slice-ledger.json"
            ),
        },
        "decision": {
            "selected_native_implementation_pr_count": 3,
            "selected_batches": batch_names,
            "candidate_evaluation": [
                {
                    "candidate": "one native implementation PR",
                    "decision": "rejected",
                    "reason": (
                        "mixes nine facade domains, reverse-request "
                        "lifecycle, high-volume runtime events, platform "
                        "behavior, and two union families"
                    ),
                },
                {
                    "candidate": "two native implementation PRs",
                    "decision": "rejected",
                    "reason": (
                        "still forces either reverse-request lifecycle "
                        "or runtime/platform behavior into the large "
                        "user-facing integration review"
                    ),
                },
                {
                    "candidate": "three native implementation PRs",
                    "decision": "selected",
                    "reason": (
                        "creates coherent schema/API/lifecycle review "
                        "boundaries and every PR ends in an honest "
                        "registry state"
                    ),
                },
                {
                    "candidate": "more than three native PRs",
                    "decision": "rejected",
                    "reason": (
                        "fragments cohesive app/plugin/hook/skill or MCP "
                        "schema families without another independent "
                        "closure boundary"
                    ),
                },
            ],
            "selection_factors": [
                "domain cohesion",
                "transitive schema and codec sharing",
                "public facade ownership",
                "server-request and notification lifecycle risk",
                "API/ABI impact",
                "test-corpus reviewability",
                "honest independent registry closure",
            ],
        },
        "native_start": {
            "identity_count": 56,
            "taxonomy": EXPECTED_TAXONOMY,
            "status": EXPECTED_NATIVE_START_STATUS,
            "global_status": EXPECTED_GLOBAL_START_STATUS,
            "client_result_kinds": EXPECTED_RESULT_KINDS,
            "native_runtime_status_matrix": native_runtime_matrix,
            "global_runtime_status_matrix": global_runtime_matrix,
        },
        "implementation_batches": batch_documents,
        "identity_bijection": [
            {
                "protocol_surface_key": key.object(),
                "implementation_batch": owners[0],
            }
            for key, owners in sorted(key_batches.items())
        ],
        "schema_dependency_graph": {
            "edges": [
                {
                    "from": "A14-UserIntegrations",
                    "to": "A14-McpReverse",
                    "reason": (
                        "review sequence and stable public facade "
                        "foundation; schema closures are disjoint"
                    ),
                },
                {
                    "from": "A14-McpReverse",
                    "to": "A14-RuntimePlatform",
                    "reason": (
                        "serverRequest/resolved lifecycle tests require "
                        "all four A1.4 reverse-request types"
                    ),
                },
                {
                    "from": "A14-RuntimePlatform",
                    "to": "A1-FinalClosure",
                    "reason": (
                        "native 56 must be complete before inherited "
                        "Common closure and SONAME action"
                    ),
                },
            ],
            "definition_overlap": overlaps,
            "overlap_rule": (
                "v2::AbsolutePathBuf is an existing reusable public "
                "type, implemented once and referenced by the platform "
                "batch; no union family is implemented twice"
            ),
        },
        "public_api_plan": {
            "facade_operations": sorted(
                client_api_rows,
                key=lambda row: row["method"],
            ),
            "notification_ownership": notification_rows,
            "server_request_ownership": request_rows,
            "rules": [
                "retain typed::Client's PIMPL",
                "add no members to AppServerClient",
                "add no public virtual interface",
                "add no generic invoke-by-method API",
                "do not move A1.1-A1.3 operations",
                "notifications remain on Events",
                "incoming requests remain on Requests occurrences",
                (
                    "stable process and remote status notifications do "
                    "not create outgoing control facades"
                ),
            ],
            "reusable_exact_public_types": REUSABLE_PUBLIC_TYPES,
            "reusable_shared_vocabulary": [
                "Unit and OperationResult<T>",
                "ThreadId, TurnId, and ItemId",
                "Json, OptionalNullable, and DecodeDiagnostic",
                "ServerRequestId and ServerRequestToken",
            ],
            "similar_types_that_must_not_be_reused": (
                NON_REUSABLE_SIMILAR_TYPES
            ),
            "request_user_input_compatibility": (
                ledger["native_a1_4_obligation"]
            ),
            "variant_and_layout_plan": variants,
        },
        "server_request_resolved_semantics": resolved,
        "final_a1_closure": {
            "branch": "codex/a1-final-closure",
            "draft_pr_title": (
                "Close the Codex A1 typed surface and bump its SOVERSION"
            ),
            "identity_count": 3,
            "identities": [
                row["protocol_surface_key"]
                for row in ledger[
                    "inherited_final_a1_obligations"
                ]
            ],
            "owner": "A1.0 / Common, unchanged",
            "completion_stage": "final A1 closure in A1.4",
            "global_start": EXPECTED_NATIVE_COMPLETION_STATUS,
            "global_end": EXPECTED_FINAL_A1_STATUS,
            "native_a1_4_status": {
                "Complete": 56,
                "Partial": 0,
                "NotImplemented": 0,
            },
            "ordered_sequence": [
                "implement initialize schema-complete lifecycle in place",
                "implement initialized exact direction evidence in place",
                "implement error canonical completeness in place",
                "prove all 339 stable A1 identities Complete",
                "capture final public API/ABI layout and symbol evidence",
                "perform the scoped Codex SOVERSION 1 -> 2 action",
                "rebuild installed consumers and source/binary packages",
                "run the final A1 closure checker",
            ],
            "commit_rule": (
                "the pure closure commit follows the three production "
                "completion commits and contains no deferred runtime "
                "implementation"
            ),
        },
        "api_abi_and_soversion": soversion,
        "security_and_sensitive_data": {
            "default_policy": (
                "treat every raw envelope, opaque JSON field, and "
                "listed sensitive path as sensitive; redact values"
            ),
            "heuristic_schema_paths": {
                "count": closure["counts"]["sensitive_paths"],
                "coverage_note": (
                    "the 127 mechanically flagged paths are a minimum; "
                    "family/root policy remains default-deny"
                ),
            },
            "family_inventory": {
                "apps": [
                    "metadata",
                    "configuration",
                    "branding",
                    "screenshots",
                    "reviews",
                ],
                "external_agents": [
                    "configuration",
                    "cwd/home detection",
                    "import histories",
                    "progress",
                    "failures",
                ],
                "feedback": [
                    "content",
                    "reason",
                    "tags",
                    "attachments/logs",
                    "thread IDs",
                ],
                "hooks": [
                    "definitions",
                    "handlers",
                    "sources and paths",
                    "input/output",
                    "failures",
                ],
                "marketplace_plugins_skills": [
                    "source URLs, paths, refs, SHAs, packages",
                    "checkout/share principals and targets",
                    "installation roots and skill roots",
                    "configuration and errors",
                ],
                "mcp_and_reverse_requests": [
                    "server IDs and names",
                    "OAuth scopes, state, and URLs",
                    "resource URIs and contents",
                    "tool names, schemas, arguments, and results",
                    "elicitation messages, values, schemas, and URLs",
                    "all user questions, options, and answers",
                    "entire attestation input and output",
                ],
                "runtime_and_platform": [
                    "process handles and output",
                    "remote identities and status",
                    "warning/deprecation text",
                    "request/thread IDs",
                    "filesystem paths",
                    "Windows sandbox diagnostics",
                ],
            },
            "forbidden_real_fixture_data": [
                "credentials and OAuth tokens",
                "environment variables",
                "plugin repositories",
                "private filesystem paths",
                "MCP server configuration",
                "feedback content",
                "process output",
                "user answers",
                "attestation material",
            ],
            "safe_fixture_vocabulary": [
                "synthetic IDs",
                "/synthetic/... paths",
                "example.invalid URLs",
                "fake tokens and values",
            ],
            "diagnostic_rule": (
                "emit method, structural path, expected type, and safe "
                "counts only; never payload values"
            ),
        },
        "source_and_binary_package_plan": {
            "source_counts": {
                "evidence_files": {"before": 22, "after": 27},
                "codex_docs": {"before": 18, "after": 19},
                "top_level_codex_tools": {
                    "before": 13,
                    "after": 14,
                },
            },
            "required_source_entries": [
                "tools/codex/app_server_a1_4.py",
                "tests/component/codex/CodexA14AuditToolTest.py",
                (
                    "tools/codex/app-server-evidence/0.144.6/"
                    "a1-4-start-state.json"
                ),
                (
                    "tools/codex/app-server-evidence/0.144.6/"
                    "a1-4-total-partition.json"
                ),
                (
                    "tools/codex/app-server-evidence/0.144.6/"
                    "a1-4-type-closure.json"
                ),
                (
                    "tools/codex/app-server-evidence/0.144.6/"
                    "a1-4-implementation-plan.json"
                ),
                (
                    "tools/codex/app-server-evidence/0.144.6/"
                    "a1-final-cross-slice-ledger.json"
                ),
                (
                    "docs/ai/openai/codex/"
                    "a1-4-integrations-and-long-tail-plan.md"
                ),
            ],
            "extracted_check": (
                "python3 -B tools/codex/app_server_a1_4.py check "
                "--repo-root <extracted-root>"
            ),
            "predecessor_closure_compatibility": (
                "A1.3 retains its historical generator/package source "
                "records while live successor-aware tokens require all "
                "A1.4 package entries and the extracted A1.4 check"
            ),
            "binary_and_installed_policy": (
                "tools, tests, docs, evidence, JSON, and Python remain "
                "excluded; this audit adds no installed public header"
            ),
            "audit_public_header_count": {
                "before": 34,
                "after": 34,
            },
            "future_public_header_counts": [
                34,
                41,
                42,
                43,
            ],
        },
        "no_expansion_boundaries": [
            "no production implementation in this audit",
            "no ProtocolSurfaceRegistryData.inc status promotion",
            "no module-slice-assignment rewrite",
            "no generated operation/notification/request descriptor change",
            "no fixture or fixture-index change",
            "no BackendState or BackendCommand expansion",
            "no frontend protocol or application behavior change",
            "no outgoing process or remote-control stable API",
            "no SOVERSION change in this audit",
        ],
        "long_run_test_policy": {
            "skipped": [
                "unfiltered full-repository CTest",
                "stress tests",
                "soak tests",
                "fuzzing campaigns",
                "benchmarks",
                "sanitizer matrices",
                "credential-bearing live App Server integration",
            ],
            "reason": (
                "audit-only changes need deterministic Python, focused "
                "CTest, configure/registration, and package evidence; "
                "these long targets add no A1.4-specific signal"
            ),
            "residual_risk": (
                "runtime integration risk remains intentionally deferred "
                "to the three production implementation PRs"
            ),
        },
        "a2_non_goals": [
            "all 36 experimental-only InventoryOnly identities",
            "all 12 stable-unreachable InventoryOnly identities",
            "process control requests",
            "remote-control outgoing requests",
            "fuzzy-search session controls",
            "collaboration, environment, and memory operations",
            "background-terminal, realtime, search, and settings controls",
            "frontend/backend/application expansion outside typed A1",
        ],
    }


def planning_diagnostics(
    plan: Mapping[str, Any],
    ledger: Mapping[str, Any],
) -> list[AuditDiagnostic]:
    diagnostics: list[AuditDiagnostic] = []

    def add(code: str, location: str, message: str) -> None:
        diagnostics.append(AuditDiagnostic(code, location, message))

    native_start = plan.get("native_start")
    if (
        not isinstance(native_start, Mapping)
        or native_start.get("identity_count") != 56
        or native_start.get("taxonomy") != EXPECTED_TAXONOMY
        or native_start.get("status") != EXPECTED_NATIVE_START_STATUS
        or native_start.get("global_status")
        != EXPECTED_GLOBAL_START_STATUS
    ):
        add(
            "NativeDenominatorMismatch",
            "$.native_start",
            (
                "native A1.4 must remain exactly 56 identities with "
                "one native Partial"
            ),
        )

    batches = plan.get("implementation_batches")
    planned_keys: list[Key] = []
    if not isinstance(batches, list) or len(batches) != 3:
        add(
            "ImplementationPlanIdentityMismatch",
            "$.implementation_batches",
            "exactly three native implementation batches are required",
        )
    else:
        by_name = {
            str(row.get("batch")): row
            for row in batches
            if isinstance(row, Mapping)
        }
        if set(by_name) != set(IMPLEMENTATION_BATCHES):
            add(
                "ImplementationPlanIdentityMismatch",
                "$.implementation_batches",
                "implementation batch names changed",
            )
        for name, expected in IMPLEMENTATION_BATCHES.items():
            row = by_name.get(name)
            if not isinstance(row, Mapping):
                continue
            identities = row.get("identities")
            keys = (
                [
                    Key.from_row(item["protocol_surface_key"])
                    for item in identities
                    if isinstance(item, Mapping)
                    and isinstance(
                        item.get("protocol_surface_key"), Mapping
                    )
                ]
                if isinstance(identities, list)
                else []
            )
            planned_keys.extend(keys)
            if set(keys) != _batch_keys(name):
                add(
                    "ImplementationPlanIdentityMissing",
                    f"$.implementation_batches.{name}.identities",
                    f"{name} identity ownership changed",
                )
            if (
                row.get("identity_count")
                != expected["identity_count"]
                or row.get("taxonomy") != expected["taxonomy"]
            ):
                add(
                    "ImplementationPlanIdentityMismatch",
                    f"$.implementation_batches.{name}",
                    f"{name} denominator or taxonomy changed",
                )
            if (
                row.get("native_metrics")
                != {
                    "start": expected["native_start"],
                    "end": expected["native_end"],
                }
                or row.get("global_metrics")
                != {
                    "start": expected["global_start"],
                    "end": expected["global_end"],
                }
            ):
                add(
                    "NativeCompletionArithmeticMismatch",
                    f"$.implementation_batches.{name}",
                    f"{name} start/end arithmetic changed",
                )
            schema = row.get("schema_closure")
            if (
                not isinstance(schema, Mapping)
                or schema.get("counts")
                != EXPECTED_BATCH_CLOSURES[name]
            ):
                add(
                    "ImplementationDependencyMismatch",
                    (
                        "$.implementation_batches."
                        f"{name}.schema_closure"
                    ),
                    f"{name} schema dependency closure changed",
                )
        if len(planned_keys) != len(set(planned_keys)):
            add(
                "ImplementationPlanIdentityOverlap",
                "$.implementation_batches",
                "a native identity is owned by multiple batches",
            )
        if set(planned_keys) != expected_a1_4_keys():
            add(
                "ImplementationPlanIdentityMissing",
                "$.implementation_batches",
                "the 56-identity native slice is not covered exactly once",
            )

    identity_bijection = plan.get("identity_bijection")
    bijection_keys = (
        [
            Key.from_row(row["protocol_surface_key"])
            for row in identity_bijection
            if isinstance(row, Mapping)
            and isinstance(row.get("protocol_surface_key"), Mapping)
        ]
        if isinstance(identity_bijection, list)
        else []
    )
    if (
        len(bijection_keys) != 56
        or len(set(bijection_keys)) != 56
        or set(bijection_keys) != expected_a1_4_keys()
    ):
        add(
            "ImplementationPlanIdentityMismatch",
            "$.identity_bijection",
            "implementation ownership bijection changed",
        )

    api = plan.get("public_api_plan")
    if not isinstance(api, Mapping):
        add(
            "PublicApiOwnershipMismatch",
            "$.public_api_plan",
            "public API ownership plan is absent",
        )
    else:
        facade_rows = api.get("facade_operations")
        facade_methods = (
            [
                str(row.get("method"))
                for row in facade_rows
                if isinstance(row, Mapping)
            ]
            if isinstance(facade_rows, list)
            else []
        )
        if (
            len(facade_methods) != 29
            or set(facade_methods) != set(CLIENT_REQUEST_CONTRACTS)
        ):
            add(
                "PublicApiOwnershipMismatch",
                "$.public_api_plan.facade_operations",
                "all 29 client requests need one facade owner",
            )
        notification_rows = api.get("notification_ownership")
        notification_methods = (
            [
                str(row.get("method"))
                for row in notification_rows
                if isinstance(row, Mapping)
            ]
            if isinstance(notification_rows, list)
            else []
        )
        if (
            len(notification_methods) != 16
            or set(notification_methods) != SERVER_NOTIFICATIONS
        ):
            add(
                "PublicApiOwnershipMismatch",
                "$.public_api_plan.notification_ownership",
                "all 16 notifications need Events ownership",
            )
        request_rows = api.get("server_request_ownership")
        request_methods = (
            [
                str(row.get("method"))
                for row in request_rows
                if isinstance(row, Mapping)
            ]
            if isinstance(request_rows, list)
            else []
        )
        if (
            len(request_methods) != 4
            or set(request_methods) != set(SERVER_REQUEST_CONTRACTS)
        ):
            add(
                "PublicApiOwnershipMismatch",
                "$.public_api_plan.server_request_ownership",
                "all four incoming requests need Requests ownership",
            )

    resolved = plan.get("server_request_resolved_semantics")
    direct = (
        resolved.get("direct_response_non_regression")
        if isinstance(resolved, Mapping)
        else None
    )
    if (
        not isinstance(direct, Mapping)
        or direct.get("is_response_transport") is not False
        or direct.get("is_response_prerequisite") is not False
        or direct.get("is_acknowledgement_prerequisite") is not False
        or direct.get("is_second_terminal_completion") is not False
        or direct.get("can_reopen_completed_occurrence") is not False
        or direct.get("changes_a1_3_response_contract") is not False
    ):
        add(
            "ResponsePathMismatch",
            "$.server_request_resolved_semantics",
            (
                "serverRequest/resolved must not replace, gate, repeat, "
                "or retroactively change direct responses"
            ),
        )
    method_matrix = (
        resolved.get("emission_method_matrix")
        if isinstance(resolved, Mapping)
        else None
    )
    expected_emitting = {
        "item/commandExecution/requestApproval",
        "item/fileChange/requestApproval",
        "item/permissions/requestApproval",
        "item/tool/requestUserInput",
        "mcpServer/elicitation/request",
    }
    observed_emitting = (
        {
            str(row.get("method"))
            for row in method_matrix
            if isinstance(row, Mapping)
            and row.get("emits_notification") is True
        }
        if isinstance(method_matrix, list)
        else set()
    )
    if observed_emitting != expected_emitting:
        add(
            "ServerRequestResolvedSemanticsMismatch",
            (
                "$.server_request_resolved_semantics."
                "emission_method_matrix"
            ),
            "serverRequest/resolved production method matrix changed",
        )

    inherited = ledger.get("inherited_final_a1_obligations")
    inherited_rows = (
        [row for row in inherited if isinstance(row, Mapping)]
        if isinstance(inherited, list)
        else []
    )
    inherited_names = {
        str(row.get("protocol_surface_key", {}).get("name"))
        for row in inherited_rows
    }
    if inherited_names != {"initialize", "initialized", "error"}:
        add(
            "CrossSliceLedgerMismatch",
            "$.inherited_final_a1_obligations",
            "the three inherited A1.0 ledger entries are required",
        )
    elif any(
        row.get("current_slice") != "A1.0"
        or row.get("current_module") != "Common"
        or row.get("current_status") != "Partial"
        or row.get("expected_status_after_completion") != "Complete"
        for row in inherited_rows
    ):
        add(
            "CrossSliceOwnershipMismatch",
            "$.inherited_final_a1_obligations",
            "inherited entries must remain Common/A1.0-owned",
        )
    native = ledger.get("native_a1_4_obligation")
    if (
        not isinstance(native, Mapping)
        or native.get("protocol_surface_key", {}).get("name")
        != "item/tool/requestUserInput"
        or native.get("current_slice") != "A1.4"
        or native.get("current_module") != A1_4_MODULE
    ):
        add(
            "NativePartialMismatch",
            "$.native_a1_4_obligation",
            "native partial ledger entry changed",
        )

    closure_stage = plan.get("final_a1_closure")
    if (
        not isinstance(closure_stage, Mapping)
        or closure_stage.get("identity_count") != 3
        or closure_stage.get("global_start")
        != EXPECTED_NATIVE_COMPLETION_STATUS
        or closure_stage.get("global_end") != EXPECTED_FINAL_A1_STATUS
    ):
        add(
            "FinalA1ArithmeticMismatch",
            "$.final_a1_closure",
            "final inherited completion arithmetic changed",
        )

    soversion = plan.get("api_abi_and_soversion")
    action = (
        soversion.get("final_action")
        if isinstance(soversion, Mapping)
        else None
    )
    if (
        not isinstance(action, Mapping)
        or action.get("decision") != "bump Codex SOVERSION 1 -> 2"
        or action.get("binary_package_change") != ".so.1 -> .so.2"
        or action.get("earlier_pr_rule") is None
    ):
        add(
            "SOVERSIONDecisionMissing",
            "$.api_abi_and_soversion",
            "the final scoped SOVERSION 1 -> 2 decision is required",
        )

    boundaries = plan.get("no_expansion_boundaries")
    if (
        not isinstance(boundaries, list)
        or "no production implementation in this audit"
        not in boundaries
        or "no SOVERSION change in this audit" not in boundaries
    ):
        add(
            "BoundaryMismatch",
            "$.no_expansion_boundaries",
            "audit-only no-expansion boundaries changed",
        )
    return sorted(set(diagnostics))


def validate_planning_reports(
    plan: Mapping[str, Any],
    ledger: Mapping[str, Any],
) -> None:
    shared.validate_diagnostics(planning_diagnostics(plan, ledger))


def build_reports(
    arguments: argparse.Namespace,
) -> tuple[
    dict[str, Any],
    dict[str, Any],
    dict[str, Any],
    dict[str, Any],
]:
    partition, closure = build_foundation(arguments)
    inputs = load_inputs(arguments)
    ledger = cross_slice_ledger_document(arguments, inputs)
    plan = implementation_plan_document(
        arguments, inputs, partition, closure, ledger
    )
    validate_planning_reports(plan, ledger)
    return partition, closure, plan, ledger


def foundation_diagnostics(
    partition: Mapping[str, Any],
    closure: Mapping[str, Any],
) -> list[AuditDiagnostic]:
    diagnostics: list[AuditDiagnostic] = []

    def add(code: str, location: str, message: str) -> None:
        diagnostics.append(AuditDiagnostic(code, location, message))

    counts = partition.get("counts")
    if not isinstance(counts, Mapping):
        add("PartitionCountMismatch", "$.counts", "counts must be an object")
    else:
        if counts.get("total_inventory") != 387:
            add(
                "TotalDenominatorMismatch",
                "$.counts.total_inventory",
                "total inventory must remain 387",
            )
        if counts.get("partition") != EXPECTED_PARTITION:
            add(
                "PartitionCountMismatch",
                "$.counts.partition",
                "six-bucket partition changed",
            )
        if (
            counts.get("stability_accounting")
            != EXPECTED_STABILITY_ACCOUNTING
        ):
            add(
                "StabilityAccountingMismatch",
                "$.counts.stability_accounting",
                "stable/experimental accounting changed",
            )
        if (
            counts.get("inventory_only_classifications")
            != EXPECTED_INVENTORY_CLASSIFICATIONS
        ):
            add(
                "InventoryClassificationMismatch",
                "$.counts.inventory_only_classifications",
                "InventoryOnly is not 36 experimental plus 12 stable",
            )
        if counts.get("native_a1_4_taxonomy") != EXPECTED_TAXONOMY:
            add(
                "TaxonomyMismatch",
                "$.counts.native_a1_4_taxonomy",
                "native A1.4 taxonomy changed",
            )
        if (
            counts.get("native_a1_4_result_kinds")
            != EXPECTED_RESULT_KINDS
        ):
            add(
                "ResultKindMismatch",
                "$.counts.native_a1_4_result_kinds",
                "A1.4 Concrete/Unit result split changed",
            )
        if (
            counts.get("native_completion_projection")
            != EXPECTED_NATIVE_COMPLETION_STATUS
        ):
            add(
                "NativeCompletionArithmeticMismatch",
                "$.counts.native_completion_projection",
                "native completion arithmetic changed",
            )
        if (
            counts.get("final_a1_projection")
            != EXPECTED_FINAL_A1_STATUS
        ):
            add(
                "FinalA1ArithmeticMismatch",
                "$.counts.final_a1_projection",
                "final A1 arithmetic changed",
            )

    buckets = partition.get("buckets")
    if not isinstance(buckets, Mapping):
        add("PartitionSetMismatch", "$.buckets", "buckets must be an object")
    else:
        seen: list[Key] = []
        for name, expected_count in EXPECTED_PARTITION.items():
            rows = buckets.get(name)
            if not isinstance(rows, list) or len(rows) != expected_count:
                add(
                    "PartitionSetMismatch",
                    f"$.buckets.{name}",
                    f"{name} identity set/count changed",
                )
                continue
            for row in rows:
                if isinstance(row, Mapping) and isinstance(
                    row.get("protocol_surface_key"), Mapping
                ):
                    seen.append(
                        Key.from_row(row["protocol_surface_key"])
                    )
        if len(seen) != len(set(seen)):
            add(
                "CrossSliceOverlap",
                "$.buckets",
                "an identity appears in more than one slice",
            )
        if len(set(seen)) != 387:
            add(
                "PartitionSetMismatch",
                "$.buckets",
                "partition does not cover 387 unique identities",
            )
        a1_4_rows = buckets.get("A1.4")
        a1_4_keys = (
            {
                Key.from_row(row["protocol_surface_key"])
                for row in a1_4_rows
                if isinstance(row, Mapping)
                and isinstance(row.get("protocol_surface_key"), Mapping)
            }
            if isinstance(a1_4_rows, list)
            else set()
        )
        if a1_4_keys != expected_a1_4_keys():
            add(
                "A14IdentitySetMismatch",
                "$.buckets.A1.4",
                "native A1.4 identity set changed",
            )
        elif isinstance(a1_4_rows, list):
            if any(
                row.get("stability") != "stable"
                for row in a1_4_rows
                if isinstance(row, Mapping)
            ):
                add(
                    "StabilityMismatch",
                    "$.buckets.A1.4",
                    "every native A1.4 identity must remain stable",
                )
            if any(
                row.get("module") != A1_4_MODULE
                or row.get("a1_slice") != A1_4_SLICE
                for row in a1_4_rows
                if isinstance(row, Mapping)
            ):
                add(
                    "A14ModuleMismatch",
                    "$.buckets.A1.4",
                    "A1.4 module/slice projection changed",
                )

    operations = partition.get("native_a1_4_operations")
    if not isinstance(operations, list) or len(operations) != 33:
        add(
            "ContractMismatch",
            "$.native_a1_4_operations",
            "29 client and four server request contracts are required",
        )
    else:
        operation_keys = {
            Key.from_row(row["protocol_surface_key"]): row
            for row in operations
            if isinstance(row, Mapping)
            and isinstance(row.get("protocol_surface_key"), Mapping)
        }
        for name, expected in CLIENT_REQUEST_CONTRACTS.items():
            key = Key(
                "client_request", "ClientRequest", "method", name
            )
            row = operation_keys.get(key)
            if (
                row is None
                or (
                    row.get("parameter_type"),
                    row.get("result_type"),
                    row.get("result_kind"),
                )
                != expected
            ):
                add(
                    "ClientResultAssociationMismatch",
                    f"$.native_a1_4_operations.{name}",
                    "client request/result association changed",
                )
        for name, expected in SERVER_REQUEST_CONTRACTS.items():
            key = Key(
                "server_request", "ServerRequest", "method", name
            )
            row = operation_keys.get(key)
            if (
                row is None
                or (
                    row.get("parameter_type"),
                    row.get("result_type"),
                )
                != expected
                or row.get("result_kind") != "Concrete"
            ):
                add(
                    "ServerResponseContractMismatch",
                    f"$.native_a1_4_operations.{name}",
                    "server request/response association changed",
                )
            if row is not None and (
                row.get("direct_raw_protocol_response") is not True
                or row.get("depends_on_server_request_resolved")
                is not False
            ):
                add(
                    "ResponsePathMismatch",
                    f"$.native_a1_4_operations.{name}",
                    "direct response path changed",
                )

    inventory = partition.get("inventory_only")
    if not isinstance(inventory, Mapping):
        add(
            "InventoryClassificationMismatch",
            "$.inventory_only",
            "InventoryOnly groups are absent",
        )
    else:
        for classification, expected_count in (
            EXPECTED_INVENTORY_CLASSIFICATIONS.items()
        ):
            rows = inventory.get(classification)
            if not isinstance(rows, list) or len(rows) != expected_count:
                add(
                    "InventoryClassificationMismatch",
                    f"$.inventory_only.{classification}",
                    f"{classification} count changed",
                )

    native_partial = partition.get("native_partial_identity")
    if (
        not isinstance(native_partial, Mapping)
        or Key.from_row(native_partial) != NATIVE_PARTIAL
    ):
        add(
            "NativePartialMismatch",
            "$.native_partial_identity",
            "native A1.4 Partial identity changed",
        )
    inherited = partition.get("inherited_a1_0_partial_identities")
    inherited_keys = (
        {
            Key.from_row(row)
            for row in inherited
            if isinstance(row, Mapping)
        }
        if isinstance(inherited, list)
        else set()
    )
    if inherited_keys != INHERITED_PARTIALS:
        add(
            "CrossSliceLedgerMismatch",
            "$.inherited_a1_0_partial_identities",
            "all three inherited A1.0 partials must be explicit",
        )

    boundaries = partition.get("audit_boundaries")
    if (
        not isinstance(boundaries, Mapping)
        or boundaries.get("production_paths_changed") != []
        or boundaries.get("protected_nonproduction_paths_changed") != []
        or boundaries.get("runtime_implementation_added") is not False
    ):
        add(
            "ProductionScopeViolation",
            "$.audit_boundaries",
            "the audit must not change production paths",
        )
    if (
        isinstance(boundaries, Mapping)
        and boundaries.get("registry_status_promoted") is not False
    ):
        add(
            "RegistryPromotionViolation",
            "$.audit_boundaries.registry_status_promoted",
            "the audit must not promote registry status",
        )
    if (
        isinstance(boundaries, Mapping)
        and (
            boundaries.get("module_assignment_changed") is not False
            or boundaries.get("soversion_changed") is not False
            or boundaries.get("backend_state_changed") is not False
            or boundaries.get("backend_command_changed") is not False
            or boundaries.get("frontend_protocol_changed") is not False
        )
    ):
        add(
            "BoundaryMismatch",
            "$.audit_boundaries",
            "a frozen no-expansion boundary changed",
        )

    union_rows = closure.get("union_families")
    actual_unions = (
        {
            (
                str(row.get("domain")),
                str(row.get("discriminator")),
                tuple(
                    sorted(
                        str(branch.get("alternative"))
                        for branch in row.get("alternatives", [])
                        if isinstance(branch, Mapping)
                    )
                ),
            )
            for row in union_rows
            if isinstance(row, Mapping)
        }
        if isinstance(union_rows, list)
        else set()
    )
    expected_unions = {
        (domain, field, tuple(sorted(alternatives)))
        for (domain, field), alternatives in UNION_FAMILIES.items()
    }
    if actual_unions != expected_unions:
        add(
            "UnionSchemaMismatch",
            "$.union_families",
            "registered union alternatives changed",
        )
    closure_counts = closure.get("counts")
    if not isinstance(closure_counts, Mapping):
        add(
            "SchemaClosureMismatch",
            "$.counts",
            "closure counts must be an object",
        )
    elif dict(closure_counts) != EXPECTED_CLOSURE_COUNTS:
        add(
            "SchemaClosureMismatch",
            "$.counts",
            "the exact transitive A1.4 schema closure changed",
        )
    paths = closure.get("schema_paths")
    if (
        not isinstance(paths, list)
        or not paths
        or len(paths) != EXPECTED_CLOSURE_COUNTS["schema_paths"]
        or any(
            not isinstance(row, Mapping)
            or "required" not in row
            or "nullable" not in row
            or "value_kind" not in row
            or "additional_properties" not in row
            for row in paths
        )
    ):
        add(
            "SchemaPathMismatch",
            "$.schema_paths",
            "complete reachable field inventory is absent",
        )
    elif len(
        {
            (
                str(row["definition"].get("namespace")),
                str(row["definition"].get("name")),
                str(row["schema_path"]),
                str(row["schema_node_kind"]),
            )
            for row in paths
            if isinstance(row.get("definition"), Mapping)
        }
    ) != EXPECTED_CLOSURE_COUNTS["schema_paths"]:
        add(
            "SchemaPathMismatch",
            "$.schema_paths",
            "schema value paths are duplicated or malformed",
        )
    objects = closure.get("object_policies")
    object_counts = (
        _counter(
            str(row.get("additional_properties"))
            for row in objects
            if isinstance(row, Mapping)
        )
        if isinstance(objects, list)
        else {}
    )
    if (
        not isinstance(objects, list)
        or len(objects) != EXPECTED_CLOSURE_COUNTS["object_nodes"]
        or object_counts != EXPECTED_OBJECT_POLICIES
    ):
        add(
            "AdditionalPropertiesMismatch",
            "$.object_policies",
            "reachable object additionalProperties policies changed",
        )
    return sorted(set(diagnostics))


def validate_foundation_reports(
    partition: Mapping[str, Any],
    closure: Mapping[str, Any],
) -> None:
    shared.validate_diagnostics(
        foundation_diagnostics(partition, closure)
    )


def write_or_check(
    path: Path, document: Mapping[str, Any], check: bool
) -> None:
    shared.write_or_check(
        path,
        document,
        check,
        artifact_label="generated A1.4 audit",
    )


def parser() -> argparse.ArgumentParser:
    repo_root = Path(__file__).resolve().parents[2]
    evidence = repo_root / "tools/codex/app-server-evidence/0.144.6"
    schema_root = repo_root / "tools/codex/app-server-schema/0.144.6"
    protocol_source_root = (
        repo_root / "tools/codex/app-server-protocol-source/0.144.6"
    )
    result = argparse.ArgumentParser(description=__doc__)
    result.add_argument(
        "mode", choices=("freeze-start-state", "generate", "check")
    )
    result.add_argument("--repo-root", type=Path, default=repo_root)
    result.add_argument(
        "--manifest",
        type=Path,
        default=repo_root
        / "tools/codex/app-server-surface/0.144.6.json",
    )
    result.add_argument(
        "--registry",
        type=Path,
        default=repo_root
        / "src/ai/openai/codex/detail/ProtocolSurfaceRegistryData.inc",
    )
    result.add_argument(
        "--schema-root", type=Path, default=schema_root
    )
    result.add_argument(
        "--schema-provenance",
        type=Path,
        default=schema_root / "PROVENANCE.json",
    )
    result.add_argument(
        "--protocol-source-root",
        type=Path,
        default=protocol_source_root,
    )
    result.add_argument(
        "--protocol-provenance",
        type=Path,
        default=protocol_source_root / "PROVENANCE.json",
    )
    result.add_argument(
        "--assignments",
        type=Path,
        default=evidence / "module-slice-assignment.json",
    )
    result.add_argument(
        "--evidence-root", type=Path, default=evidence
    )
    result.add_argument(
        "--reachability",
        type=Path,
        default=evidence / "nested-reachability.json",
    )
    result.add_argument(
        "--contracts",
        type=Path,
        default=evidence / "operation-contracts.json",
    )
    result.add_argument(
        "--schema-completeness",
        type=Path,
        default=evidence / "schema-completeness-evidence.json",
    )
    result.add_argument(
        "--fixture-coverage",
        type=Path,
        default=evidence / "fixture-coverage.json",
    )
    result.add_argument(
        "--draft07-validator",
        type=Path,
        default=repo_root / "tools/codex/draft07.py",
    )
    result.add_argument(
        "--start-state",
        type=Path,
        default=evidence / "a1-4-start-state.json",
    )
    result.add_argument(
        "--partition-output",
        type=Path,
        default=evidence / "a1-4-total-partition.json",
    )
    result.add_argument(
        "--closure-output",
        type=Path,
        default=evidence / "a1-4-type-closure.json",
    )
    result.add_argument(
        "--plan-output",
        type=Path,
        default=evidence / "a1-4-implementation-plan.json",
    )
    result.add_argument(
        "--ledger-output",
        type=Path,
        default=evidence / "a1-final-cross-slice-ledger.json",
    )
    result.add_argument("--base-sha")
    return result


def main(argv: Sequence[str] | None = None) -> int:
    arguments = parser().parse_args(argv)
    for name, value in vars(arguments).items():
        if isinstance(value, Path):
            setattr(arguments, name, value.resolve())
    if arguments.mode == "freeze-start-state":
        base_sha = arguments.base_sha or _git(
            arguments.repo_root, "rev-parse", "HEAD"
        )
        document = start_state_document(arguments, base_sha)
        write_or_check(arguments.start_state, document, False)
        return 0
    partition, closure, plan, ledger = build_reports(arguments)
    check = arguments.mode == "check"
    write_or_check(arguments.partition_output, partition, check)
    write_or_check(arguments.closure_output, closure, check)
    write_or_check(arguments.plan_output, plan, check)
    write_or_check(arguments.ledger_output, ledger, check)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AuditError as error:
        print(
            f"app-server-a1-4: error: {error.code}: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
