#!/usr/bin/env python3
"""Generate the frozen Phase A1.1 implementation and type-closure audits.

The generated reports are review evidence.  They deliberately copy current
runtime dispositions and targets from ProtocolSurfaceRegistryData.inc and
must never be used as a production dispatch or disposition registry.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence
from urllib.parse import quote

import app_server_fixtures as fixture_tool
import app_server_surface as surface


FORMAT_VERSION = 1
CODEX_VERSION = "codex-cli 0.144.6"
A1_1_SLICE = "A1.1"
EXPECTED_START_BASE_SHA = "a226e3df5efd55b5dbef12cb1760674388909d7c"

EXPECTED_CATEGORY_COUNTS = {
    "client_request": 22,
    "server_notification": 37,
    "thread_item": 18,
    "response_item": 16,
    "tagged_union_discriminator": 58,
}
EXPECTED_CLASSIFICATION_COUNTS = {
    "RootOwnedNestedUnion": 16,
    "SharedCommon": 26,
    "SharedWithinSlice": 16,
    "StableItemRoot": 34,
    "StablePublicRoot": 59,
}
EXPECTED_MODULE_COUNTS = {"Common": 26, "ThreadsTurnsSessions": 125}
EXPECTED_SCHEMA_STATUS_COUNTS = {"NotImplemented": 125, "Partial": 26}
EXPECTED_RESULT_KIND_COUNTS = {"Concrete": 15, "Unit": 7}
EXPECTED_UNIT_METHODS = {
    "thread/archive",
    "thread/compact/start",
    "thread/delete",
    "thread/inject_items",
    "thread/name/set",
    "thread/shellCommand",
    "turn/interrupt",
}
EXPECTED_GLOBAL_SCHEMA_STATUS = {
    "Complete": 16,
    "NotApplicable": 48,
    "NotImplemented": 289,
    "Partial": 34,
}
EXPECTED_FINAL_GLOBAL_SCHEMA_STATUS = {
    "Complete": 167,
    "NotApplicable": 48,
    "NotImplemented": 164,
    "Partial": 8,
}
EXPECTED_BATCH_IDENTITY_COUNTS = {"B2": 26, "B3": 50, "B4": 38, "B5": 37}
EXPECTED_BATCH_DEFINITION_COUNTS = {"B2": 12, "B3": 32, "B4": 72, "B5": 48}
EXPECTED_DEFINITION_SHAPES = {
    "object": 106,
    "open_string_enum": 27,
    "scalar_alias": 6,
    "tagged_or_untagged_union": 25,
}
EXPECTED_DEFINITION_DIRECTIONS = {
    "Both": 14,
    "DecodeOnly": 121,
    "EncodeOnly": 29,
}
EXPECTED_SCHEMA_PATH_DIRECTIONS = {
    "Both": 37,
    "DecodeOnly": 554,
    "EncodeOnly": 107,
}
EXPECTED_SCHEMA_PATH_KIND_DIRECTIONS = {
    "array_element": {"Both": 2, "DecodeOnly": 30, "EncodeOnly": 6},
    "map_value": {"Both": 0, "DecodeOnly": 2, "EncodeOnly": 3},
    "property": {"Both": 35, "DecodeOnly": 522, "EncodeOnly": 98},
}
EXPECTED_PROPERTY_PRESENCE_MODELS = {
    "DefaultedValue": 4,
    "OptionalNullable": 218,
    "OptionalValue": 15,
    "RequiredNullable": 5,
    "RequiredValue": 413,
}
REVIEWED_DEFAULTED_PROPERTY_VALUES = {
    "#/definitions/v2/ThreadForkResponse/properties/instructionSources": [],
    "#/definitions/v2/ThreadResumeResponse/properties/instructionSources": [],
    "#/definitions/v2/ThreadStartResponse/properties/instructionSources": [],
    "#/definitions/v2/Turn/properties/itemsView": "full",
}
REVIEWED_NULLABLE_DEFAULT_NULL_PATHS = {
    "#/definitions/v2/ActivePermissionProfile/properties/extends",
    "#/definitions/v2/ResponseItem/oneOf/2/properties/content",
    (
        "#/definitions/v2/SubAgentSource/oneOf/1/properties/"
        "thread_spawn/properties/agent_nickname"
    ),
    (
        "#/definitions/v2/SubAgentSource/oneOf/1/properties/"
        "thread_spawn/properties/agent_path"
    ),
    (
        "#/definitions/v2/SubAgentSource/oneOf/1/properties/"
        "thread_spawn/properties/agent_role"
    ),
    "#/definitions/v2/ThreadItem/oneOf/2/properties/memoryCitation",
    "#/definitions/v2/ThreadItem/oneOf/2/properties/phase",
    "#/definitions/v2/TurnError/properties/additionalDetails",
    "#/definitions/v2/UserInput/oneOf/1/properties/detail",
    "#/definitions/v2/UserInput/oneOf/2/properties/detail",
}
EXPECTED_CROSS_SLICE_OWNERSHIP = {
    "A11ForwardShared": 39,
    "A11Local": 122,
    "PriorSliceReuse": 3,
}
PRIOR_SLICE_REUSED_DEFINITIONS = {
    "CodexErrorInfo",
    "NonSteerableTurnKind",
    "TurnError",
}
FORWARD_SLICE_NAMES = {"A1.2", "A1.3", "A1.4"}
EXPECTED_STRONG_IDENTIFIER_COUNTS = {
    "ClientUserMessageId": {
        "scalar_property_paths": 3,
        "vector_property_paths": 0,
        "vector_element_paths": 0,
    },
    "ItemId": {
        "scalar_property_paths": 29,
        "vector_property_paths": 0,
        "vector_element_paths": 0,
    },
    "ModelId": {
        "scalar_property_paths": 10,
        "vector_property_paths": 0,
        "vector_element_paths": 0,
    },
    "ResponseCallId": {
        "scalar_property_paths": 7,
        "vector_property_paths": 0,
        "vector_element_paths": 0,
    },
    "ResponseItemId": {
        "scalar_property_paths": 14,
        "vector_property_paths": 0,
        "vector_element_paths": 0,
    },
    "SessionId": {
        "scalar_property_paths": 1,
        "vector_property_paths": 0,
        "vector_element_paths": 0,
    },
    "ThreadId": {
        "scalar_property_paths": 62,
        "vector_property_paths": 3,
        "vector_element_paths": 3,
    },
    "TurnId": {
        "scalar_property_paths": 24,
        "vector_property_paths": 0,
        "vector_element_paths": 0,
    },
}
REVIEWED_EXACT_STRONG_IDENTIFIER_PATHS = {
    "#/definitions/v2/Thread/properties/sessionId": "SessionId",
    (
        "#/definitions/v2/ThreadItem/oneOf/0/properties/clientId"
    ): "ClientUserMessageId",
    (
        "#/definitions/v2/TurnStartParams/properties/"
        "clientUserMessageId"
    ): "ClientUserMessageId",
    (
        "#/definitions/v2/TurnSteerParams/properties/"
        "clientUserMessageId"
    ): "ClientUserMessageId",
}
EXPECTED_IDENTITY_DIRECTIONS = {
    "Both": 13,
    "DecodeOnly": 116,
    "EncodeOnly": 22,
}
EXPECTED_RESIDUAL_PARTIAL_KEYS = {
    ("client_notification", "ClientNotification", "method", "initialized"),
    ("client_request", "ClientRequest", "method", "initialize"),
    ("server_notification", "ServerNotification", "method", "error"),
    ("server_notification", "ServerNotification", "method", "model/rerouted"),
    (
        "server_request",
        "ServerRequest",
        "method",
        "account/chatgptAuthTokens/refresh",
    ),
    (
        "server_request",
        "ServerRequest",
        "method",
        "item/commandExecution/requestApproval",
    ),
    (
        "server_request",
        "ServerRequest",
        "method",
        "item/fileChange/requestApproval",
    ),
    (
        "server_request",
        "ServerRequest",
        "method",
        "item/tool/requestUserInput",
    ),
}
REQUIRED_PLAN_IDENTITY_FIELDS = {
    "category",
    "classification",
    "current_fixture_ids",
    "current_implementation_status",
    "current_runtime_disposition",
    "current_runtime_target",
    "current_schema_completeness",
    "current_schema_status",
    "deprecated",
    "discriminator_field",
    "directionality",
    "domain",
    "expected_backend_core_handling",
    "expected_frontend_behavior",
    "module",
    "owning_implementation_batch",
    "protocol_surface_key",
    "reaching_stable_roots",
    "required_negative_fixtures",
    "required_positive_fixtures",
    "required_private_codec_or_descriptor",
    "required_public_type_or_facade",
    "source_compatibility_consideration",
    "stability",
}
REQUIRED_REQUEST_CONTRACT_FIELDS = {
    "authoritative_params_type",
    "authoritative_result_schema_type",
    "authoritative_successful_result_type",
    "current_typed_method",
    "deprecated_typed_api",
    "old_overload_or_alias_must_remain",
    "proposed_public_method_name",
    "result_contract_kind",
}
REQUIRED_POSITIVE_FIXTURE_FIELDS = {
    "base_cases",
    "existing_fixture_ids",
    "planned_fixture_ids",
    "required_fixture_ids",
}
REQUIRED_NEGATIVE_FIXTURE_FIELDS = {
    "runtime_case_ids",
    "schema_mutations",
}
REQUIRED_SCHEMA_MUTATION_GROUPS = {
    "conflicting_discriminator",
    "future_discriminator",
    "future_open_enum",
    "malformed_known",
    "missing_discriminator",
    "nullable_null",
    "nullable_omitted",
    "nullable_value",
    "optional_field_omission",
    "required_field_removal",
    "wrong_type",
}
REQUIRED_PUBLIC_MAPPING_FIELDS = {
    "client_request": {
        "facade",
        "method",
        "params_type",
        "successful_result_type",
    },
    "server_notification": {
        "canonical_payload_aggregate",
        "existing_semantic_adapter",
        "payload_direction",
    },
    "item_discriminator": {
        "canonical_alternative",
        "compatibility_aliases",
        "future_unknown_alternative",
        "schema_branch_title",
        "variant",
        "variant_compatibility_aliases",
    },
    "tagged_union_discriminator": {
        "canonical_alternative",
        "compatibility_aliases",
        "future_unknown_alternative",
        "schema_branch_title",
        "union",
    },
}

OPERATION_API_NAMES = {
    "thread/archive": ("typed::Threads", "archive"),
    "thread/compact/start": ("typed::Threads", "startCompaction"),
    "thread/delete": ("typed::Threads", "remove"),
    "thread/fork": ("typed::Threads", "fork"),
    "thread/goal/clear": ("typed::Threads", "clearGoal"),
    "thread/goal/get": ("typed::Threads", "getGoal"),
    "thread/goal/set": ("typed::Threads", "setGoal"),
    "thread/inject_items": ("typed::Threads", "injectItems"),
    "thread/list": ("typed::Threads", "list"),
    "thread/loaded/list": ("typed::Threads", "listLoaded"),
    "thread/metadata/update": ("typed::Threads", "updateMetadata"),
    "thread/name/set": ("typed::Threads", "setName"),
    "thread/read": ("typed::Threads", "read"),
    "thread/resume": ("typed::Threads", "resume"),
    "thread/rollback": ("typed::Threads", "rollback"),
    "thread/shellCommand": ("typed::Threads", "shellCommand"),
    "thread/start": ("typed::Threads", "start"),
    "thread/unarchive": ("typed::Threads", "unarchive"),
    "thread/unsubscribe": ("typed::Threads", "unsubscribe"),
    "turn/interrupt": ("typed::Turns", "interrupt"),
    "turn/start": ("typed::Turns", "start"),
    "turn/steer": ("typed::Turns", "steer"),
}
CURRENT_TYPED_METHODS = {
    "thread/list": "typed::Threads::list",
    "thread/read": "typed::Threads::read",
    "thread/resume": "typed::Threads::resume",
    "thread/start": "typed::Threads::start",
    "turn/interrupt": "typed::Turns::interrupt",
    "turn/start": "typed::Turns::start",
}

# Reviewed public mappings.  These are deliberately explicit: schema titles,
# compatibility aliases, and direction-specific unknown alternatives are API
# design, not a casing transform or generated production dispatch authority.
SERVER_NOTIFICATION_PAYLOAD_TYPES = {
    "item/agentMessage/delta": "AgentMessageDeltaNotification",
    "item/commandExecution/outputDelta": "CommandExecutionOutputDeltaNotification",
    "item/commandExecution/terminalInteraction": "TerminalInteractionNotification",
    "item/completed": "ItemCompletedNotification",
    "item/fileChange/outputDelta": "FileChangeOutputDeltaNotification",
    "item/fileChange/patchUpdated": "FileChangePatchUpdatedNotification",
    "item/mcpToolCall/progress": "McpToolCallProgressNotification",
    "item/plan/delta": "PlanDeltaNotification",
    "item/reasoning/summaryPartAdded": "ReasoningSummaryPartAddedNotification",
    "item/reasoning/summaryTextDelta": "ReasoningSummaryTextDeltaNotification",
    "item/reasoning/textDelta": "ReasoningTextDeltaNotification",
    "item/started": "ItemStartedNotification",
    "thread/archived": "ThreadArchivedNotification",
    "thread/closed": "ThreadClosedNotification",
    "thread/compacted": "ContextCompactedNotification",
    "thread/deleted": "ThreadDeletedNotification",
    "thread/goal/cleared": "ThreadGoalClearedNotification",
    "thread/goal/updated": "ThreadGoalUpdatedNotification",
    "thread/name/updated": "ThreadNameUpdatedNotification",
    "thread/realtime/closed": "ThreadRealtimeClosedNotification",
    "thread/realtime/error": "ThreadRealtimeErrorNotification",
    "thread/realtime/itemAdded": "ThreadRealtimeItemAddedNotification",
    "thread/realtime/outputAudio/delta": "ThreadRealtimeOutputAudioDeltaNotification",
    "thread/realtime/sdp": "ThreadRealtimeSdpNotification",
    "thread/realtime/started": "ThreadRealtimeStartedNotification",
    "thread/realtime/transcript/delta": "ThreadRealtimeTranscriptDeltaNotification",
    "thread/realtime/transcript/done": "ThreadRealtimeTranscriptDoneNotification",
    "thread/settings/updated": "ThreadSettingsUpdatedNotification",
    "thread/started": "ThreadStartedNotification",
    "thread/status/changed": "ThreadStatusChangedNotification",
    "thread/tokenUsage/updated": "ThreadTokenUsageUpdatedNotification",
    "thread/unarchived": "ThreadUnarchivedNotification",
    "turn/completed": "TurnCompletedNotification",
    "turn/diff/updated": "TurnDiffUpdatedNotification",
    "turn/moderationMetadata": "TurnModerationMetadataNotification",
    "turn/plan/updated": "TurnPlanUpdatedNotification",
    "turn/started": "TurnStartedNotification",
}
SERVER_NOTIFICATION_SEMANTIC_ADAPTERS = {
    "item/agentMessage/delta": {
        "type": "typed::AgentMessageDelta",
        "rule": "preserve the existing semantic event",
    },
    "item/commandExecution/outputDelta": {
        "type": "typed::CommandOutputDelta",
        "rule": "preserve the existing semantic event",
    },
    "item/completed": {
        "type": "typed::ItemCompleted",
        "rule": "preserve the existing semantic event",
    },
    "item/fileChange/patchUpdated": {
        "type": "typed::FileChangeUpdated",
        "rule": "preserve the existing semantic event",
    },
    "item/reasoning/summaryTextDelta": {
        "type": "typed::ReasoningDelta",
        "rule": "preserve ReasoningDelta::Kind::Summary",
    },
    "item/reasoning/textDelta": {
        "type": "typed::ReasoningDelta",
        "rule": "preserve ReasoningDelta::Kind::Text",
    },
    "item/started": {
        "type": "typed::ItemStarted",
        "rule": "preserve the existing semantic event",
    },
    "thread/started": {
        "type": "typed::ThreadStarted",
        "rule": "preserve the existing semantic event",
    },
    "thread/status/changed": {
        "type": "typed::ThreadStatusChanged",
        "rule": "preserve the existing semantic event",
    },
    "thread/tokenUsage/updated": {
        "type": "typed::TokenUsageUpdated",
        "rule": "preserve the existing semantic event",
    },
    "turn/completed": {
        "type": "typed::TurnCompleted | typed::TurnFailed",
        "rule": "preserve the existing status-based semantic split",
    },
    "turn/started": {
        "type": "typed::TurnStarted",
        "rule": "preserve the existing semantic event",
    },
}

THREAD_ITEM_ALTERNATIVES = {
    "agentMessage": "AgentMessageThreadItem",
    "collabAgentToolCall": "CollabAgentToolCallThreadItem",
    "commandExecution": "CommandExecutionThreadItem",
    "contextCompaction": "ContextCompactionThreadItem",
    "dynamicToolCall": "DynamicToolCallThreadItem",
    "enteredReviewMode": "EnteredReviewModeThreadItem",
    "exitedReviewMode": "ExitedReviewModeThreadItem",
    "fileChange": "FileChangeThreadItem",
    "hookPrompt": "HookPromptThreadItem",
    "imageGeneration": "ImageGenerationThreadItem",
    "imageView": "ImageViewThreadItem",
    "mcpToolCall": "McpToolCallThreadItem",
    "plan": "PlanThreadItem",
    "reasoning": "ReasoningThreadItem",
    "sleep": "SleepThreadItem",
    "subAgentActivity": "SubAgentActivityThreadItem",
    "userMessage": "UserMessageThreadItem",
    "webSearch": "WebSearchThreadItem",
}
THREAD_ITEM_COMPATIBILITY_ALIASES = {
    "agentMessage": ["typed::AgentMessageItem"],
    "commandExecution": ["typed::CommandExecutionItem"],
    "fileChange": ["typed::FileChangeItem"],
    "mcpToolCall": ["typed::ToolCallItem"],
    "reasoning": ["typed::ReasoningItem"],
    "userMessage": ["typed::UserMessageItem"],
    "webSearch": ["typed::WebSearchItem"],
}
RESPONSE_ITEM_ALTERNATIVES = {
    "agent_message": "AgentMessageResponseItem",
    "compaction": "CompactionResponseItem",
    "compaction_trigger": "CompactionTriggerResponseItem",
    "context_compaction": "ContextCompactionResponseItem",
    "custom_tool_call": "CustomToolCallResponseItem",
    "custom_tool_call_output": "CustomToolCallOutputResponseItem",
    "function_call": "FunctionCallResponseItem",
    "function_call_output": "FunctionCallOutputResponseItem",
    "image_generation_call": "ImageGenerationCallResponseItem",
    "local_shell_call": "LocalShellCallResponseItem",
    "message": "MessageResponseItem",
    "other": "OtherResponseItem",
    "reasoning": "ReasoningResponseItem",
    "tool_search_call": "ToolSearchCallResponseItem",
    "tool_search_output": "ToolSearchOutputResponseItem",
    "web_search_call": "WebSearchCallResponseItem",
}

NESTED_UNION_ALTERNATIVES = {
    ("AgentMessageInputContent", "encrypted_content"): "EncryptedContentAgentMessageInputContent",
    ("AgentMessageInputContent", "input_text"): "InputTextAgentMessageInputContent",
    ("AskForApproval", "granular"): "GranularAskForApproval",
    ("AskForApproval", "never"): "ApprovalPolicy",
    ("AskForApproval", "on-request"): "ApprovalPolicy",
    ("AskForApproval", "untrusted"): "ApprovalPolicy",
    ("CommandAction", "listFiles"): "ListFilesCommandAction",
    ("CommandAction", "read"): "ReadCommandAction",
    ("CommandAction", "search"): "SearchCommandAction",
    ("CommandAction", "unknown"): "UnknownCommandAction",
    ("ContentItem", "input_image"): "InputImageContentItem",
    ("ContentItem", "input_text"): "InputTextContentItem",
    ("ContentItem", "output_text"): "OutputTextContentItem",
    ("DynamicToolCallOutputContentItem", "inputImage"): "InputImageDynamicToolCallOutputContentItem",
    ("DynamicToolCallOutputContentItem", "inputText"): "InputTextDynamicToolCallOutputContentItem",
    ("FunctionCallOutputContentItem", "encrypted_content"): "EncryptedContentFunctionCallOutputContentItem",
    ("FunctionCallOutputContentItem", "input_image"): "InputImageFunctionCallOutputContentItem",
    ("FunctionCallOutputContentItem", "input_text"): "InputTextFunctionCallOutputContentItem",
    ("LocalShellAction", "exec"): "ExecLocalShellAction",
    ("PatchChangeKind", "add"): "AddPatchChangeKind",
    ("PatchChangeKind", "delete"): "DeletePatchChangeKind",
    ("PatchChangeKind", "update"): "UpdatePatchChangeKind",
    ("ReasoningItemContent", "reasoning_text"): "ReasoningTextReasoningItemContent",
    ("ReasoningItemContent", "text"): "TextReasoningItemContent",
    ("ReasoningItemReasoningSummary", "summary_text"): "SummaryTextReasoningItemReasoningSummary",
    ("ResponsesApiWebSearchAction", "find_in_page"): "FindInPageResponsesApiWebSearchAction",
    ("ResponsesApiWebSearchAction", "open_page"): "OpenPageResponsesApiWebSearchAction",
    ("ResponsesApiWebSearchAction", "other"): "OtherResponsesApiWebSearchAction",
    ("ResponsesApiWebSearchAction", "search"): "SearchResponsesApiWebSearchAction",
    ("SandboxPolicy", "dangerFullAccess"): "DangerFullAccessSandboxPolicy",
    ("SandboxPolicy", "externalSandbox"): "ExternalSandboxSandboxPolicy",
    ("SandboxPolicy", "readOnly"): "ReadOnlySandboxPolicy",
    ("SandboxPolicy", "workspaceWrite"): "WorkspaceWriteSandboxPolicy",
    ("SessionSource", "appServer"): "SessionSourceKind",
    ("SessionSource", "cli"): "SessionSourceKind",
    ("SessionSource", "custom"): "CustomSessionSource",
    ("SessionSource", "exec"): "SessionSourceKind",
    ("SessionSource", "subAgent"): "SubAgentSessionSource",
    ("SessionSource", "unknown"): "SessionSourceKind",
    ("SessionSource", "vscode"): "SessionSourceKind",
    ("SubAgentSource", "compact"): "SubAgentSourceKind",
    ("SubAgentSource", "memory_consolidation"): "SubAgentSourceKind",
    ("SubAgentSource", "other"): "OtherSubAgentSource",
    ("SubAgentSource", "review"): "SubAgentSourceKind",
    ("SubAgentSource", "thread_spawn"): "ThreadSpawnSubAgentSource",
    ("ThreadStatus", "active"): "ActiveThreadStatus",
    ("ThreadStatus", "idle"): "IdleThreadStatus",
    ("ThreadStatus", "notLoaded"): "NotLoadedThreadStatus",
    ("ThreadStatus", "systemError"): "SystemErrorThreadStatus",
    ("UserInput", "image"): "ImageUserInput",
    ("UserInput", "localImage"): "LocalImageUserInput",
    ("UserInput", "mention"): "MentionUserInput",
    ("UserInput", "skill"): "SkillUserInput",
    ("UserInput", "text"): "TextUserInput",
    ("WebSearchAction", "findInPage"): "FindInPageWebSearchAction",
    ("WebSearchAction", "openPage"): "OpenPageWebSearchAction",
    ("WebSearchAction", "other"): "OtherWebSearchAction",
    ("WebSearchAction", "search"): "SearchWebSearchAction",
}

# Inline schema objects need a reviewed public owner name when the generic
# owner/member spelling does not match the handwritten C++ aggregate.
EXPLICIT_SCHEMA_CPP_MAPPINGS = {
    "#/definitions/v2/AskForApproval/oneOf/1/properties/granular": (
        "typed::GranularAskForApproval",
        "granular",
        "typed::GranularApprovalOptions",
    ),
    "#/definitions/v2/SubAgentSource/oneOf/1/properties/thread_spawn": (
        "typed::ThreadSpawnSubAgentSource",
        "threadSpawn",
        "typed::ThreadSpawnSubAgentDetails",
    ),
}
NESTED_UNION_FUTURE_UNKNOWN = {
    "AgentMessageInputContent": "UnknownAgentMessageInputContent",
    "AskForApproval": "UnknownAskForApproval",
    "CommandAction": "UnrecognizedCommandAction",
    "ContentItem": "UnknownContentItem",
    "DynamicToolCallOutputContentItem": "UnknownDynamicToolCallOutputContentItem",
    "FunctionCallOutputContentItem": "UnknownFunctionCallOutputContentItem",
    "LocalShellAction": "UnknownLocalShellAction",
    "PatchChangeKind": "UnknownPatchChangeKind",
    "ReasoningItemContent": "UnknownReasoningItemContent",
    "ReasoningItemReasoningSummary": "UnknownReasoningItemReasoningSummary",
    "ResponsesApiWebSearchAction": "UnknownResponsesApiWebSearchAction",
    "SandboxPolicy": "UnknownSandboxPolicy",
    "SessionSource": "UnknownSessionSourceAlternative",
    "SubAgentSource": "UnknownSubAgentSource",
    "ThreadStatus": "UnknownThreadStatus",
    "UserInput": "UnknownUserInput",
    "WebSearchAction": "UnknownWebSearchAction",
}
NESTED_UNION_COMPATIBILITY_ALIASES = {
    ("SandboxPolicy", "externalSandbox"): ["typed::ExternalSandboxPolicy"],
    ("UserInput", "text"): ["typed::TextInput"],
    ("UserInput", "image"): ["typed::ImageUrlInput"],
    ("UserInput", "localImage"): ["typed::LocalImageInput"],
    ("UserInput", "skill"): ["typed::SkillInput"],
    ("UserInput", "mention"): ["typed::MentionInput"],
}
SCALAR_VARIANT_IDENTITIES = {
    ("AskForApproval", "never"),
    ("AskForApproval", "on-request"),
    ("AskForApproval", "untrusted"),
    ("SessionSource", "appServer"),
    ("SessionSource", "cli"),
    ("SessionSource", "exec"),
    ("SessionSource", "unknown"),
    ("SessionSource", "vscode"),
    ("SubAgentSource", "compact"),
    ("SubAgentSource", "memory_consolidation"),
    ("SubAgentSource", "review"),
}

# JSON Schema defaults additionalProperties to true for every object.  Only
# explicit unconstrained schemas and explicitly arbitrary containers are
# opaque here.  Known tagged-union alternatives such as ResponseItem "other"
# remain typed alternatives even when their complete raw object is retained.
EXPECTED_PROTOCOL_OPAQUE_PATHS = {
    "#/definitions/v2/McpToolCallResult/properties/_meta":
        "the protocol declares the MCP metadata value as unconstrained JSON",
    "#/definitions/v2/McpToolCallResult/properties/content/items":
        "the protocol declares each MCP content element as unconstrained JSON",
    "#/definitions/v2/McpToolCallResult/properties/structuredContent":
        "the protocol declares structured MCP content as unconstrained JSON",
    "#/definitions/v2/ResponseItem/oneOf/5/properties/arguments":
        "the protocol declares tool-search arguments as unconstrained JSON",
    "#/definitions/v2/ResponseItem/oneOf/9/properties/tools/items":
        "the protocol declares tool-search output elements as unconstrained JSON",
    "#/definitions/v2/ThreadForkParams/properties/config/additionalProperties":
        "the typed configuration map has protocol-defined unconstrained JSON values",
    "#/definitions/v2/ThreadInjectItemsParams/properties/items/items":
        "the protocol declares injected Responses API items as unconstrained JSON",
    "#/definitions/v2/ThreadItem/oneOf/7/properties/arguments":
        "the protocol declares MCP tool arguments as unconstrained JSON",
    "#/definitions/v2/ThreadItem/oneOf/8/properties/arguments":
        "the protocol declares dynamic-tool arguments as unconstrained JSON",
    "#/definitions/v2/ThreadRealtimeItemAddedNotification/properties/item":
        "the protocol declares the realtime item as unconstrained JSON",
    "#/definitions/v2/ThreadResumeParams/properties/config/additionalProperties":
        "the typed configuration map has protocol-defined unconstrained JSON values",
    "#/definitions/v2/ThreadStartParams/properties/config/additionalProperties":
        "the typed configuration map has protocol-defined unconstrained JSON values",
    "#/definitions/v2/TurnModerationMetadataNotification/properties/metadata":
        "the protocol declares moderation metadata as unconstrained JSON",
    "#/definitions/v2/TurnStartParams/properties/outputSchema":
        "the protocol declares the optional output JSON Schema annotation as unconstrained JSON",
}

UNIT_RESULT_SCHEMA_NAMES: set[str] = set()
VALID_TYPE_DISPOSITIONS = {
    "OpenStringEnum",
    "PrivateCodecHelper",
    "ProtocolDefinedOpaqueJson",
    "PublicHandwrittenType",
    "ReusedExistingType",
    "StrongIdentifier",
}
VALID_DIRECTIONALITIES = {"Both", "DecodeOnly", "EncodeOnly"}
VALID_CROSS_SLICE_OWNERSHIP = {
    "A11ForwardShared",
    "A11Local",
    "PriorSliceReuse",
}


class AuditError(RuntimeError):
    """A frozen A1.1 invariant was violated."""

    def __init__(
        self,
        message: str,
        code: str = "AuditInvariant",
        codes: Sequence[str] | None = None,
    ) -> None:
        super().__init__(message)
        self.code = code
        self.codes = tuple(codes) if codes is not None else (code,)


@dataclass(frozen=True, order=True)
class AuditDiagnostic:
    """Stable intrinsic diagnostic emitted for a mutated generated report."""

    code: str
    location: str
    message: str


@dataclass(frozen=True, order=True)
class Key:
    category: str
    domain: str
    field: str
    name: str

    @classmethod
    def from_row(cls, row: Mapping[str, Any]) -> "Key":
        category, domain, field, name = surface.surface_key(dict(row))
        return cls(category, domain, field, name)

    def object(self) -> dict[str, str]:
        return {
            "category": self.category,
            "domain": self.domain,
            "discriminator_field": self.field,
            "name": self.name,
        }

    def compact(self) -> str:
        return f"{self.category}:{self.domain}:{self.field}:{self.name}"


@dataclass
class LiveInputs:
    manifest: dict[str, Any]
    assignments_document: dict[str, Any]
    reachability_document: dict[str, Any]
    contracts_document: dict[str, Any]
    completeness_document: dict[str, Any]
    fixture_document: dict[str, Any]
    registry_rows: list[dict[str, Any]]
    registry: dict[Key, dict[str, Any]]
    manifest_rows: dict[Key, dict[str, Any]]
    assignments: dict[Key, dict[str, Any]]
    reachability: dict[Key, dict[str, Any]]
    contracts: dict[Key, dict[str, Any]]
    completeness: dict[Key, dict[str, Any]]
    fixture_coverage: dict[Key, dict[str, Any]]


def fail(message: str, code: str = "AuditInvariant") -> None:
    raise AuditError(message, code)


def require(
    condition: bool,
    message: str,
    code: str = "AuditInvariant",
) -> None:
    if not condition:
        fail(message, code)


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise AuditError(f"unable to load {path}: {error}") from error
    if not isinstance(value, dict):
        fail(f"expected an object-valued JSON document: {path}")
    return value


def canonical_json(value: Any) -> str:
    return json.dumps(value, indent=2, sort_keys=True, ensure_ascii=False) + "\n"


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def sha256_json(value: Any) -> str:
    return hashlib.sha256(
        json.dumps(
            value, sort_keys=True, separators=(",", ":"), ensure_ascii=False
        ).encode("utf-8")
    ).hexdigest()


def indexed(
    records: Iterable[Mapping[str, Any]], description: str
) -> dict[Key, dict[str, Any]]:
    result: dict[Key, dict[str, Any]] = {}
    for raw in records:
        row = dict(raw)
        key = Key.from_row(row)
        if key in result:
            fail(f"duplicate {description} identity: {key.compact()}")
        result[key] = row
    return result


def records(document: Mapping[str, Any], names: Sequence[str], description: str) -> list[dict[str, Any]]:
    for name in names:
        value = document.get(name)
        if isinstance(value, list) and all(isinstance(row, dict) for row in value):
            return [dict(row) for row in value]
    fail(f"{description} lacks one of the expected arrays: {', '.join(names)}")


def pascal(value: str) -> str:
    words = re.split(r"[^A-Za-z0-9]+", value)
    return "".join(word[:1].upper() + word[1:] for word in words if word)


def schema_references(value: Any) -> set[str]:
    result: set[str] = set()

    def walk(node: Any) -> None:
        if isinstance(node, dict):
            reference = node.get("$ref")
            if isinstance(reference, str):
                match = re.fullmatch(r"#/definitions/v2/([^/]+)", reference)
                if match:
                    result.add(match.group(1).replace("~1", "/").replace("~0", "~"))
            for child in node.values():
                walk(child)
        elif isinstance(node, list):
            for child in node:
                walk(child)

    walk(value)
    return result


def schema_reference_records(value: Any) -> list[tuple[str, str]]:
    result: list[tuple[str, str]] = []

    def walk(node: Any, path: str) -> None:
        if isinstance(node, dict):
            reference = node.get("$ref")
            if isinstance(reference, str):
                result.append((path + "/$ref", reference))
            for key, child in sorted(node.items()):
                if key != "$ref":
                    walk(child, path + "/" + str(key))
        elif isinstance(node, list):
            for index, child in enumerate(node):
                walk(child, path + f"/{index}")

    walk(value, "$")
    return result


def registry_projection(row: Mapping[str, Any]) -> dict[str, Any]:
    identity_fields = {"category", "discriminator_field", "domain", "name"}
    return {
        field: value
        for field, value in sorted(row.items())
        if field not in identity_fields
    }


def tuple_key_object(key: tuple[str, str, str, str]) -> dict[str, str]:
    return {
        "category": key[0],
        "domain": key[1],
        "discriminator_field": key[2],
        "name": key[3],
    }


def schema_path_mapping_projection(row: Mapping[str, Any]) -> dict[str, Any]:
    fields = (
        "schema_path",
        "schema_node_kind",
        "directionality",
        "cpp_owner",
        "cpp_member",
        "cpp_base_type",
        "cpp_type",
        "required",
        "nullable",
        "presence_model",
        "schema_default",
        "default_behavior",
        "semantic_identifier",
        "semantic_identifier_role",
    )
    return {field: row[field] for field in fields if field in row}


def definition_direction_ownership_projection(
    row: Mapping[str, Any]
) -> dict[str, Any]:
    fields = (
        "definition_key",
        "directionality",
        "cross_slice_ownership",
        "reused_by_later_slices",
    )
    return {field: row[field] for field in fields if field in row}


def definition_cpp_mapping_projection(
    row: Mapping[str, Any]
) -> dict[str, Any]:
    fields = (
        "definition_key",
        "disposition",
        "cpp_owner",
    )
    return {field: row[field] for field in fields if field in row}


def discriminator_mapping_projection(row: Mapping[str, Any]) -> dict[str, Any]:
    private = row.get("required_private_codec_or_descriptor")
    behavior = (
        private.get("discriminator_behavior")
        if isinstance(private, Mapping)
        else None
    )
    return {
        "protocol_surface_key": row.get("protocol_surface_key"),
        "directionality": row.get("directionality"),
        "descriptor_key": (
            private.get("descriptor_key")
            if isinstance(private, Mapping)
            else None
        ),
        "discriminator_behavior": behavior,
    }


def validate_stable_method_unions(
    all_definitions: Mapping[str, Any],
    manifest_rows: Mapping[Key, Mapping[str, Any]],
    contracts: Mapping[Key, Mapping[str, Any]],
) -> None:
    """Independently join aggregate method branches to manifest/contracts."""

    diagnostics: list[AuditDiagnostic] = []
    for category, domain in (
        ("client_request", "ClientRequest"),
        ("server_notification", "ServerNotification"),
    ):
        union = all_definitions.get(domain)
        branches = union.get("oneOf") if isinstance(union, dict) else None
        if not isinstance(branches, list):
            diagnostics.append(
                AuditDiagnostic(
                    "AggregateMethodBranchMismatch",
                    f"#/definitions/{domain}",
                    "stable aggregate method union lacks oneOf branches",
                )
            )
            continue
        actual: dict[str, tuple[int, str | None]] = {}
        malformed = False
        for index, branch in enumerate(branches):
            properties = branch.get("properties") if isinstance(branch, dict) else None
            method_schema = properties.get("method") if isinstance(properties, dict) else None
            params_schema = properties.get("params") if isinstance(properties, dict) else None
            method_values = method_schema.get("enum") if isinstance(method_schema, dict) else None
            params_ref = params_schema.get("$ref") if isinstance(params_schema, dict) else None
            null_params = (
                isinstance(params_schema, dict)
                and params_schema.get("type") == "null"
                and set(params_schema) == {"type"}
            )
            if (
                not isinstance(method_values, list)
                or len(method_values) != 1
                or not isinstance(method_values[0], str)
                or (not isinstance(params_ref, str) and not null_params)
            ):
                malformed = True
                continue
            method = method_values[0]
            if method in actual:
                malformed = True
                continue
            actual[method] = (
                index,
                params_ref if isinstance(params_ref, str) else None,
            )
        if malformed:
            diagnostics.append(
                AuditDiagnostic(
                    "AggregateMethodBranchMismatch",
                    f"#/definitions/{domain}/oneOf",
                    "method branch lacks a unique singleton method and params $ref",
                )
            )

        expected = {
            key.name: (key, row)
            for key, row in manifest_rows.items()
            if key.category == category and row.get("stability") == "stable"
        }
        if set(actual) != set(expected):
            diagnostics.append(
                AuditDiagnostic(
                    "AggregateMethodBranchMismatch",
                    f"#/definitions/{domain}/oneOf",
                    "stable aggregate methods disagree with the stable manifest",
                )
            )
        for method in sorted(set(actual) & set(expected)):
            index, params_ref = actual[method]
            key, manifest_row = expected[method]
            match = (
                re.fullmatch(r"#/definitions/(?:v2/)?([^/]+)", params_ref)
                if params_ref is not None
                else None
            )
            manifest_params = manifest_row.get("params")
            manifest_type = (
                manifest_params.get("type")
                if isinstance(manifest_params, dict)
                else None
            )
            referenced_type = match.group(1) if match is not None else None
            if (
                (params_ref is not None and match is None)
                or referenced_type != manifest_type
            ):
                diagnostics.append(
                    AuditDiagnostic(
                        "AggregateParamsReferenceMismatch",
                        f"#/definitions/{domain}/oneOf/{index}/properties/params/$ref",
                        f"{method} params reference disagrees with the manifest",
                    )
                )
                continue
            if category == "client_request":
                contract = contracts.get(key)
                expected_contract_type = (
                    referenced_type if referenced_type is not None else "Unit"
                )
                if (
                    contract is None
                    or contract.get("parameter_type_identity")
                    != expected_contract_type
                ):
                    diagnostics.append(
                        AuditDiagnostic(
                            "AggregateParamsReferenceMismatch",
                            f"#/definitions/{domain}/oneOf/{index}/properties/params/$ref",
                            f"{method} params reference disagrees with its operation contract",
                        )
                    )
    if diagnostics:
        diagnostics.sort()
        codes = tuple(diagnostic.code for diagnostic in diagnostics)
        message = "; ".join(
            f"{diagnostic.code} at {diagnostic.location}: {diagnostic.message}"
            for diagnostic in diagnostics
        )
        raise AuditError(message, diagnostics[0].code, codes)


def closure_reference_diagnostics(
    definitions: Mapping[str, Mapping[str, Any]],
    closure: Iterable[str],
) -> list[AuditDiagnostic]:
    return [
        AuditDiagnostic(
            "ExternalClosureReference",
            f"#/definitions/v2/{definition}{path[1:]}",
            f"non-v2 or external closure reference: {reference}",
        )
        for definition in sorted(closure)
        for path, reference in schema_reference_records(definitions[definition])
        if re.fullmatch(r"#/definitions/v2/[^/]+", reference) is None
    ]


def validate_closure_references(
    definitions: Mapping[str, Mapping[str, Any]],
    closure: Iterable[str],
) -> None:
    diagnostics = closure_reference_diagnostics(definitions, closure)
    if diagnostics:
        codes = tuple(diagnostic.code for diagnostic in diagnostics)
        message = "; ".join(
            f"{diagnostic.code} at {diagnostic.location}: {diagnostic.message}"
            for diagnostic in diagnostics
        )
        raise AuditError(message, diagnostics[0].code, codes)


def transitive_closure(starts: Iterable[str], edges: Mapping[str, set[str]]) -> set[str]:
    reached: set[str] = set()
    pending = list(starts)
    while pending:
        current = pending.pop()
        if current in reached:
            continue
        if current not in edges:
            fail(f"schema closure references an unknown v2 definition: {current}")
        reached.add(current)
        pending.extend(sorted(edges[current] - reached))
    return reached


def find_cycle(nodes: Iterable[str], edges: Mapping[str, set[str]]) -> list[str] | None:
    allowed = set(nodes)
    visited: set[str] = set()
    active: set[str] = set()
    stack: list[str] = []

    def visit(node: str) -> list[str] | None:
        visited.add(node)
        active.add(node)
        stack.append(node)
        for dependency in sorted(edges[node] & allowed):
            if dependency not in visited:
                cycle = visit(dependency)
                if cycle:
                    return cycle
            elif dependency in active:
                return stack[stack.index(dependency):] + [dependency]
        stack.pop()
        active.remove(node)
        return None

    for node in sorted(allowed):
        if node not in visited:
            cycle = visit(node)
            if cycle:
                return cycle
    return None


def schema_shape(schema: Mapping[str, Any]) -> str:
    if "enum" in schema:
        require(schema.get("type") == "string", "A1.1 enum is not string-backed")
        return "open_string_enum"
    if "oneOf" in schema or "anyOf" in schema:
        return "tagged_or_untagged_union"
    if schema.get("type") == "object" or "properties" in schema:
        return "object"
    if schema.get("type") == "array" or "items" in schema:
        return "array"
    if "$ref" in schema or "allOf" in schema:
        return "composition"
    return "scalar_alias"


def allows_null(schema: Any) -> bool:
    if not isinstance(schema, dict):
        return schema is True
    # Draft-07's empty schema accepts every JSON instance, including null.
    # Annotation-only schemas have the same validation semantics.
    annotation_keywords = {
        "$comment",
        "$id",
        "$schema",
        "default",
        "description",
        "examples",
        "readOnly",
        "title",
        "writeOnly",
    }
    if set(schema).issubset(annotation_keywords):
        return True
    schema_type = schema.get("type")
    if schema_type == "null":
        return True
    if isinstance(schema_type, list) and "null" in schema_type:
        return True
    return any(
        allows_null(branch)
        for keyword in ("oneOf", "anyOf")
        for branch in schema.get(keyword, [])
    )


def definition_disposition(name: str, schema: Mapping[str, Any], shape: str) -> tuple[str, str]:
    if name in UNIT_RESULT_SCHEMA_NAMES:
        return "ReusedExistingType", "typed::Unit"
    if name in PRIOR_SLICE_REUSED_DEFINITIONS:
        return "ReusedExistingType", f"typed::{name}"
    if shape == "open_string_enum":
        return "OpenStringEnum", f"typed::{name}"
    if name == "ThreadId":
        return "StrongIdentifier", "typed::ThreadId"
    return "PublicHandwrittenType", f"typed::{name}"


def cpp_member_name(name: str) -> str:
    """Return the reviewed lower-camel C++ spelling for one JSON member."""

    if name == "namespace":
        return "nameSpace"
    words = [word for word in re.split(r"[^A-Za-z0-9]+", name) if word]
    if not words:
        return "value"
    first = words[0]
    return first[:1].lower() + first[1:] + "".join(
        word[:1].upper() + word[1:] for word in words[1:]
    )


def schema_path_definition(path: str) -> str:
    match = re.match(r"^#/definitions/v2/([^/]+)(?:/|$)", path)
    require(match is not None, f"schema path is outside the v2 closure: {path}")
    return match.group(1)


def property_name_from_path(path: str) -> str | None:
    marker = "/properties/"
    if marker not in path:
        return None
    name = path.rsplit(marker, 1)[-1]
    return name if "/" not in name else None


def semantic_strong_identifier(
    path: str, kind: str
) -> tuple[str, str] | None:
    """Map semantically typed string fields that upstream leaves as strings.

    The rules intentionally encode domain meaning, rather than applying an
    unsafe generic `*Id` suffix transform.  The exact resulting path sets are
    ratcheted below.
    """

    property_path = path[:-len("/items")] if kind == "array_element" and path.endswith("/items") else path
    name = property_name_from_path(property_path)
    thread_loaded_data_path = (
        "#/definitions/v2/ThreadLoadedListResponse/properties/data"
    )

    if kind == "array_element":
        if (
            name in {"receiverThreadIds", "threadIds"}
            or property_path == thread_loaded_data_path
        ):
            return "ThreadId", "vector_element"
        return None

    if kind != "property" or name is None:
        return None

    reviewed_identifier = REVIEWED_EXACT_STRONG_IDENTIFIER_PATHS.get(path)
    if reviewed_identifier is not None:
        return reviewed_identifier, "scalar_property"

    if (
        name in {"receiverThreadIds", "threadIds"}
        or path == thread_loaded_data_path
    ):
        return "ThreadId", "vector_property"
    if (
        name
        in {
            "agentThreadId",
            "forkedFromId",
            "parentThreadId",
            "parent_thread_id",
            "senderThreadId",
            "threadId",
        }
        or path == "#/definitions/v2/Thread/properties/id"
    ):
        return "ThreadId", "scalar_property"
    if (
        name in {"expectedTurnId", "lastTurnId", "turnId", "turn_id"}
        or path == "#/definitions/v2/Turn/properties/id"
    ):
        return "TurnId", "scalar_property"
    if name == "itemId" or (
        path.startswith("#/definitions/v2/ThreadItem/")
        and name == "id"
    ):
        return "ItemId", "scalar_property"
    if (
        path.startswith("#/definitions/v2/ResponseItem/")
        and name == "id"
    ):
        return "ResponseItemId", "scalar_property"
    if name == "model":
        return "ModelId", "scalar_property"
    if name == "call_id":
        return "ResponseCallId", "scalar_property"
    return None


def non_null_schema(schema: Any) -> Any:
    """Strip only a schema-level null alternative, retaining every wrapper."""

    if not isinstance(schema, dict):
        return schema
    schema_type = schema.get("type")
    if isinstance(schema_type, list) and "null" in schema_type:
        non_null_types = [value for value in schema_type if value != "null"]
        result = dict(schema)
        result["type"] = (
            non_null_types[0] if len(non_null_types) == 1 else non_null_types
        )
        return result
    for keyword in ("oneOf", "anyOf"):
        branches = schema.get(keyword)
        if not isinstance(branches, list):
            continue
        non_null = [
            branch
            for branch in branches
            if not (
                isinstance(branch, dict)
                and branch.get("type") == "null"
                and set(branch) == {"type"}
            )
        ]
        if len(non_null) != len(branches):
            if len(non_null) == 1:
                return non_null[0]
            result = dict(schema)
            result[keyword] = non_null
            return result
    return schema


def inline_cpp_type_name(owner: str, member: str, suffix: str = "") -> str:
    owner_name = owner.removeprefix("typed::")
    member_words = [
        word
        for word in re.split(r"[^A-Za-z0-9]+", member.replace("[]", "Item"))
        if word
    ]
    stem = "".join(word[:1].upper() + word[1:] for word in member_words)
    return f"typed::{owner_name}{stem or 'Value'}{suffix}"


def schema_cpp_base_type(
    schema: Any,
    path: str,
    owner: str,
    member: str,
    semantic_identifier: tuple[str, str] | None,
) -> str:
    explicit_mapping = EXPLICIT_SCHEMA_CPP_MAPPINGS.get(path)
    if explicit_mapping is not None:
        require(
            semantic_identifier is None,
            f"explicit C++ type conflicts with a strong identifier mapping: {path}",
            "ConflictingClosureMapping",
        )
        return explicit_mapping[2]
    if semantic_identifier is not None:
        identifier, role = semantic_identifier
        if role == "vector_property":
            return f"std::vector<typed::{identifier}>"
        return f"typed::{identifier}"
    if path in EXPECTED_PROTOCOL_OPAQUE_PATHS:
        return "Json"
    if schema is True:
        return "Json"
    require(isinstance(schema, dict), f"schema path has a non-object schema: {path}")

    schema = non_null_schema(schema)
    require(isinstance(schema, dict), f"schema path has a malformed nullable schema: {path}")
    reference = schema.get("$ref")
    if isinstance(reference, str):
        match = re.fullmatch(r"#/definitions/v2/([^/]+)", reference)
        require(match is not None, f"schema path has a non-v2 reference: {path}")
        referenced_name = match.group(1)
        return (
            "typed::Unit"
            if referenced_name in UNIT_RESULT_SCHEMA_NAMES
            else f"typed::{referenced_name}"
        )

    all_of = schema.get("allOf")
    if isinstance(all_of, list):
        require(len(all_of) == 1, f"schema composition needs an explicit C++ mapping: {path}")
        return schema_cpp_base_type(
            all_of[0], path + "/allOf/0", owner, member, semantic_identifier
        )

    for keyword in ("oneOf", "anyOf"):
        branches = schema.get(keyword)
        if isinstance(branches, list):
            branch_types: list[str] = []
            for index, branch in enumerate(branches):
                branch_type = schema_cpp_base_type(
                    branch,
                    f"{path}/{keyword}/{index}",
                    owner,
                    member,
                    semantic_identifier,
                )
                if branch_type not in branch_types:
                    branch_types.append(branch_type)
            require(branch_types, f"schema union has no C++ alternatives: {path}")
            return (
                branch_types[0]
                if len(branch_types) == 1
                else "std::variant<" + ", ".join(branch_types) + ">"
            )

    enum_values = schema.get("enum")
    if isinstance(enum_values, list):
        require(
            enum_values and all(isinstance(value, str) for value in enum_values),
            f"schema enum has malformed values: {path}",
        )
        if len(enum_values) == 1:
            return f'detail::ExactDiscriminatorLiteral<"{enum_values[0]}">'
        return inline_cpp_type_name(owner, member, "Value")

    schema_type = schema.get("type")
    if schema_type == "array":
        require("items" in schema, f"array schema has no item mapping: {path}")
        item_type = schema_cpp_base_type(
            schema["items"], path + "/items", owner, member + "[]", None
        )
        if allows_null(schema["items"]) and item_type != "Json":
            item_type = f"std::optional<{item_type}>"
        return f"std::vector<{item_type}>"
    if schema_type == "object" or "properties" in schema:
        additional = schema.get("additionalProperties")
        if additional is True and not schema.get("properties"):
            return "std::map<std::string, Json>"
        if isinstance(additional, dict) and not schema.get("properties"):
            value_type = schema_cpp_base_type(
                additional,
                path + "/additionalProperties",
                owner,
                member + "[*]",
                None,
            )
            return f"std::map<std::string, {value_type}>"
        return inline_cpp_type_name(owner, member)
    if schema_type == "string":
        return "std::string"
    if schema_type == "boolean":
        return "bool"
    if schema_type == "integer":
        integer_format = schema.get("format")
        if integer_format == "int32":
            return "std::int32_t"
        if integer_format == "int64":
            return "std::int64_t"
        if integer_format == "uint16":
            return "std::uint16_t"
        if integer_format == "uint32":
            return "std::uint32_t"
        if integer_format in {"uint", "uint64"}:
            return "std::uint64_t"
        return (
            "std::uint64_t"
            if (
                isinstance(schema.get("minimum"), (int, float))
                and schema["minimum"] >= 0
            )
            else "std::int64_t"
        )
    if schema_type == "number":
        return "double"
    if schema_type == "null":
        return "std::monostate"
    if isinstance(schema_type, list):
        mapped = [
            schema_cpp_base_type(
                {"type": value}, path, owner, member, semantic_identifier
            )
            for value in schema_type
        ]
        return "std::variant<" + ", ".join(mapped) + ">"
    if not schema:
        return "Json"
    fail(f"schema path has no exact C++ type mapping: {path}", "MissingClosureMapping")


def property_presence_model(
    required: bool, nullable: bool, defaulted: bool = False
) -> str:
    if required:
        return "RequiredNullable" if nullable else "RequiredValue"
    if nullable:
        return "OptionalNullable"
    return "DefaultedValue" if defaulted else "OptionalValue"


def wrap_property_cpp_type(base_type: str, presence_model: str) -> str:
    if presence_model == "OptionalNullable":
        return f"typed::OptionalNullable<{base_type}>"
    if presence_model in {"OptionalValue", "RequiredNullable"}:
        return f"std::optional<{base_type}>"
    require(
        presence_model in {"DefaultedValue", "RequiredValue"},
        f"unknown property presence model: {presence_model}",
    )
    return base_type


def path_disposition(
    path: str,
    schema: Any,
    semantic_identifier: tuple[str, str] | None,
) -> str:
    if path in EXPECTED_PROTOCOL_OPAQUE_PATHS:
        return "ProtocolDefinedOpaqueJson"
    if semantic_identifier is not None:
        return "StrongIdentifier"
    if isinstance(schema, dict):
        reference = schema.get("$ref")
        if isinstance(reference, str):
            return "ReusedExistingType"
        if "enum" in schema:
            enum_values = schema.get("enum")
            if isinstance(enum_values, list) and len(enum_values) == 1:
                return "PrivateCodecHelper"
            return "OpenStringEnum"
        if (
            schema.get("type") == "object"
            or "properties" in schema
            or "oneOf" in schema
            or "anyOf" in schema
            or "allOf" in schema
        ):
            return "PublicHandwrittenType"
    return "ReusedExistingType"


def collect_schema_paths(
    definitions: Mapping[str, Mapping[str, Any]],
    closure: set[str],
    definition_directions: Mapping[str, str],
) -> tuple[
    list[dict[str, Any]],
    dict[str, int],
    set[str],
    dict[str, int],
    dict[str, dict[str, list[str]]],
]:
    result: list[dict[str, Any]] = []
    discovered_opaque: set[str] = set()
    field_counts: Counter[str] = Counter()
    presence_counts: Counter[str] = Counter()
    strong_identifier_paths: dict[str, dict[str, list[str]]] = {
        identifier: {
            "scalar_property_paths": [],
            "vector_property_paths": [],
            "vector_element_paths": [],
        }
        for identifier in EXPECTED_STRONG_IDENTIFIER_COUNTS
    }

    def add(
        path: str,
        kind: str,
        schema: Any,
        owner: str,
        member: str,
        directionality: str,
        required: bool | None = None,
    ) -> None:
        semantic_identifier = semantic_strong_identifier(path, kind)
        disposition = path_disposition(path, schema, semantic_identifier)
        if disposition == "ProtocolDefinedOpaqueJson":
            discovered_opaque.add(path)
        base_type = schema_cpp_base_type(
            schema, path, owner, member, semantic_identifier
        )
        cpp_type = base_type
        row: dict[str, Any] = {
            "schema_path": path,
            "schema_node_kind": kind,
            "directionality": directionality,
            "disposition": disposition,
            "cpp_owner": owner,
            "cpp_member": member,
            "cpp_base_type": base_type,
        }
        if required is not None:
            nullable = allows_null(schema)
            reviewed_defaulted = (
                path in REVIEWED_DEFAULTED_PROPERTY_VALUES
            )
            reviewed_nullable_default = (
                path in REVIEWED_NULLABLE_DEFAULT_NULL_PATHS
            )
            if reviewed_defaulted:
                require(
                    isinstance(schema, Mapping)
                    and schema.get("default")
                    == REVIEWED_DEFAULTED_PROPERTY_VALUES[path]
                    and not required
                    and not nullable,
                    "reviewed canonical non-null default changed: "
                    f"{path}",
                    "OptionalNullableMappingMismatch",
                )
            if reviewed_nullable_default:
                require(
                    isinstance(schema, Mapping)
                    and "default" in schema
                    and schema.get("default") is None
                    and not required
                    and nullable,
                    "reviewed nullable default:null mapping changed: "
                    f"{path}",
                    "OptionalNullableMappingMismatch",
                )
            presence_model = property_presence_model(
                required, nullable, reviewed_defaulted
            )
            cpp_type = wrap_property_cpp_type(base_type, presence_model)
            row.update(
                {
                    "required": required,
                    "nullable": nullable,
                    "presence_model": presence_model,
                }
            )
            if reviewed_defaulted:
                row["schema_default"] = (
                    REVIEWED_DEFAULTED_PROPERTY_VALUES[path]
                )
                row["default_behavior"] = "CanonicalValueOnOmission"
            elif reviewed_nullable_default:
                row["schema_default"] = None
                row["default_behavior"] = "PreserveOmittedNullValue"
            presence_counts[presence_model] += 1
            field_counts[
                ("required" if required else "optional")
                + ("_nullable" if nullable else "_nonnullable")
            ] += 1
        elif allows_null(schema) and base_type != "Json":
            cpp_type = f"std::optional<{base_type}>"
        row["cpp_type"] = cpp_type
        row["cpp_mapping"] = f"{owner}::{member} -> {cpp_type}"
        if semantic_identifier is not None:
            identifier, role = semantic_identifier
            role_key = {
                "scalar_property": "scalar_property_paths",
                "vector_property": "vector_property_paths",
                "vector_element": "vector_element_paths",
            }[role]
            strong_identifier_paths[identifier][role_key].append(path)
            row["semantic_identifier"] = f"typed::{identifier}"
            row["semantic_identifier_role"] = role
        if disposition == "ProtocolDefinedOpaqueJson":
            row["opaque_reason"] = EXPECTED_PROTOCOL_OPAQUE_PATHS[path]
        result.append(row)

    def walk(
        schema: Any,
        path: str,
        owner: str,
        member_prefix: str,
        directionality: str,
    ) -> None:
        if not isinstance(schema, dict):
            return
        required = set(schema.get("required", []))
        properties = schema.get("properties", {})
        if isinstance(properties, dict):
            for name, child in sorted(properties.items()):
                child_path = f"{path}/properties/{name}"
                member_name = cpp_member_name(name)
                if (
                    member_name == "id"
                    and owner.startswith("typed::")
                    and owner.endswith("ThreadItem")
                    and owner != "typed::ThreadItem"
                ):
                    # Incoming ThreadItem alternatives intentionally retain
                    # the established envelope/raw provenance aggregate.
                    member_name = "metadata.id"
                child_member = (
                    f"{member_prefix}.{member_name}"
                    if member_prefix
                    else member_name
                )
                add(
                    child_path,
                    "property",
                    child,
                    owner,
                    child_member,
                    directionality,
                    name in required,
                )
                walk(
                    child,
                    child_path,
                    owner,
                    child_member,
                    directionality,
                )
        if "items" in schema:
            child = schema["items"]
            child_path = f"{path}/items"
            child_member = f"{member_prefix}[]" if member_prefix else "value[]"
            add(
                child_path,
                "array_element",
                child,
                owner,
                child_member,
                directionality,
            )
            walk(child, child_path, owner, child_member, directionality)
        if "additionalProperties" in schema:
            child = schema["additionalProperties"]
            # `additionalProperties: false` closes an object; it does not
            # declare a map-value schema.  True and object-valued forms do.
            if child is not False:
                child_path = f"{path}/additionalProperties"
                child_member = (
                    f"{member_prefix}[*]" if member_prefix else "value[*]"
                )
                add(
                    child_path,
                    "map_value",
                    child,
                    owner,
                    child_member,
                    directionality,
                )
                walk(child, child_path, owner, child_member, directionality)
        for keyword in ("oneOf", "anyOf", "allOf"):
            branches = schema.get(keyword, [])
            if isinstance(branches, list):
                for index, branch in enumerate(branches):
                    branch_owner = owner
                    branch_member_prefix = member_prefix
                    if isinstance(branch, dict):
                        title = branch.get("title")
                        if isinstance(title, str) and title:
                            branch_owner = f"typed::{title}"
                            branch_member_prefix = ""
                    walk(
                        branch,
                        f"{path}/{keyword}/{index}",
                        branch_owner,
                        branch_member_prefix,
                        directionality,
                    )

    for name in sorted(closure):
        directionality = definition_directions[name]
        require(
            directionality in VALID_DIRECTIONALITIES,
            f"definition has invalid directionality: {name}",
        )
        walk(
            definitions[name],
            f"#/definitions/v2/{name}",
            f"typed::{name}",
            "",
            directionality,
        )

    paths = [row["schema_path"] for row in result]
    require(len(paths) == len(set(paths)), "duplicate type-closure schema path disposition")
    require(
        set(EXPLICIT_SCHEMA_CPP_MAPPINGS) <= set(paths),
        "reviewed explicit C++ type mapping no longer reaches the stable closure: "
        f"{sorted(set(EXPLICIT_SCHEMA_CPP_MAPPINGS) - set(paths))}",
        "StaleExplicitClosureMapping",
    )
    rows_by_path = {
        str(row["schema_path"]): row
        for row in result
    }
    for reviewed_path, (
        expected_owner,
        expected_member,
        expected_base_type,
    ) in sorted(EXPLICIT_SCHEMA_CPP_MAPPINGS.items()):
        reviewed_row = rows_by_path[reviewed_path]
        require(
            reviewed_row["cpp_owner"] == expected_owner
            and reviewed_row["cpp_member"] == expected_member
            and reviewed_row["cpp_base_type"] == expected_base_type
            and reviewed_row["cpp_type"] == expected_base_type
            and reviewed_row["cpp_mapping"]
            == (
                f"{expected_owner}::{expected_member} -> "
                f"{expected_base_type}"
            ),
            "reviewed explicit C++ owner/member/type mapping changed: "
            f"{reviewed_path}",
            "ConflictingClosureMapping",
        )
    reviewed_default_paths = (
        set(REVIEWED_DEFAULTED_PROPERTY_VALUES)
        | REVIEWED_NULLABLE_DEFAULT_NULL_PATHS
    )
    require(
        reviewed_default_paths <= set(paths),
        "reviewed schema-default mapping no longer reaches the stable "
        "closure: "
        f"{sorted(reviewed_default_paths - set(paths))}",
        "StaleExplicitClosureMapping",
    )
    for reviewed_path, expected_default in sorted(
        REVIEWED_DEFAULTED_PROPERTY_VALUES.items()
    ):
        reviewed_row = rows_by_path[reviewed_path]
        require(
            reviewed_row["presence_model"] == "DefaultedValue"
            and reviewed_row["cpp_type"]
            == reviewed_row["cpp_base_type"]
            and reviewed_row.get("schema_default") == expected_default
            and reviewed_row.get("default_behavior")
            == "CanonicalValueOnOmission",
            "reviewed canonical default C++ mapping changed: "
            f"{reviewed_path}",
            "OptionalNullableMappingMismatch",
        )
    for reviewed_path in sorted(
        REVIEWED_NULLABLE_DEFAULT_NULL_PATHS
    ):
        reviewed_row = rows_by_path[reviewed_path]
        require(
            reviewed_row["presence_model"] == "OptionalNullable"
            and reviewed_row.get("schema_default") is None
            and reviewed_row.get("default_behavior")
            == "PreserveOmittedNullValue",
            "reviewed nullable default:null C++ mapping changed: "
            f"{reviewed_path}",
            "OptionalNullableMappingMismatch",
        )
    require(
        discovered_opaque == set(EXPECTED_PROTOCOL_OPAQUE_PATHS),
        "protocol-defined opaque JSON paths changed: "
        f"expected={sorted(EXPECTED_PROTOCOL_OPAQUE_PATHS)}, actual={sorted(discovered_opaque)}",
    )
    json_mapped_paths = {
        row["schema_path"]
        for row in result
        if "Json" in row["cpp_base_type"]
    }
    unjustified_json_paths = {
        path
        for path in json_mapped_paths
        if not any(
            opaque_path == path
            or opaque_path.startswith(path + "/")
            for opaque_path in EXPECTED_PROTOCOL_OPAQUE_PATHS
        )
    }
    require(
        not unjustified_json_paths,
        "known schema value was mapped wholesale to Json without protocol "
        f"evidence: {sorted(unjustified_json_paths)}",
        "OpaqueJsonMisuse",
    )
    require(
        dict(sorted(presence_counts.items()))
        == EXPECTED_PROPERTY_PRESENCE_MODELS,
        "property OptionalNullable/accounting matrix changed: "
        f"{dict(sorted(presence_counts.items()))}",
        "OptionalNullableMappingMismatch",
    )
    for identifier in sorted(strong_identifier_paths):
        for role in strong_identifier_paths[identifier]:
            strong_identifier_paths[identifier][role].sort()
    actual_identifier_counts = {
        identifier: {
            role: len(paths)
            for role, paths in sorted(roles.items())
        }
        for identifier, roles in sorted(strong_identifier_paths.items())
    }
    require(
        actual_identifier_counts == EXPECTED_STRONG_IDENTIFIER_COUNTS,
        "semantic strong-identifier mapping changed: "
        f"{actual_identifier_counts}",
        "StrongIdentifierMappingMismatch",
    )
    result.sort(key=lambda row: (row["schema_path"], row["schema_node_kind"]))
    return (
        result,
        dict(sorted(field_counts.items())),
        discovered_opaque,
        dict(sorted(presence_counts.items())),
        strong_identifier_paths,
    )


def identity_batch(assignment: Mapping[str, Any], key: Key) -> str:
    classification = assignment["classification"]
    if classification == "SharedCommon":
        return "B2"
    if key.category == "item_discriminator" or classification == "RootOwnedNestedUnion":
        return "B3"
    if key.category == "client_request" or classification == "SharedWithinSlice":
        return "B4"
    if key.category == "server_notification":
        return "B5"
    fail(f"no frozen implementation batch for {key.compact()} ({classification})")


def stable_roots_for(
    key: Key,
    reachability: Mapping[Key, Mapping[str, Any]],
) -> list[dict[str, Any]]:
    if key.category == "tagged_union_discriminator":
        record = reachability.get(key)
        require(record is not None, f"nested identity lacks reachability: {key.compact()}")
        roots = record.get("reaching_roots")
        require(isinstance(roots, list), f"nested reachability roots malformed: {key.compact()}")
        return sorted(
            [dict(root) for root in roots],
            key=lambda root: (str(root.get("root_id")), str(root.get("role"))),
        )
    if key.category == "item_discriminator":
        return [{"role": "item_union", "root_id": f"item_union:{key.domain}", "slice": A1_1_SLICE}]
    return [
        {
            "role": "method",
            "root_id": key.compact(),
            "slice": A1_1_SLICE,
            "surface_key": key.object(),
        }
    ]


def mutation_case_id(kind: str, base_fixture_id: str, instance_path: str) -> str:
    return (
        f"mutation:{kind}:{base_fixture_id}:"
        f"{quote(instance_path, safe='')}"
    )


def _schema_reference_names(schema: Any) -> set[str]:
    names: set[str] = set()

    def walk(node: Any) -> None:
        if isinstance(node, dict):
            reference = node.get("$ref")
            if isinstance(reference, str):
                match = re.fullmatch(
                    r"#/definitions/(?:(?:v2)/)?([^/]+)", reference
                )
                if match:
                    names.add(
                        match.group(1)
                        .replace("~1", "/")
                        .replace("~0", "~")
                    )
            for child in node.values():
                walk(child)
        elif isinstance(node, list):
            for child in node:
                walk(child)

    walk(schema)
    return names


@dataclass
class PlannedFixtureBase:
    fixture_id: str
    status: str
    role: str
    file: str
    target: fixture_tool.SchemaTarget
    value: Any


class FixturePlanner:
    """Schema-only fixture and mutation planner for the frozen A1.1 audit."""

    def __init__(self, arguments: argparse.Namespace) -> None:
        draft07 = fixture_tool.load_draft07(arguments.draft07_validator)
        self.catalog = fixture_tool.SchemaCatalog(arguments.schema_root, draft07)
        self.synthesizer = fixture_tool.Synthesizer(self.catalog)
        self.fixture_root = arguments.fixture_index.parent
        fixture_index = load_json(arguments.fixture_index)
        validate_input_versions((fixture_index,))
        fixture_rows = records(
            fixture_index, ("fixtures",), "fixture index"
        )
        self.records: dict[str, dict[str, Any]] = {}
        for row in fixture_rows:
            fixture_id = row.get("id")
            require(
                isinstance(fixture_id, str) and fixture_id,
                "fixture index contains a malformed fixture ID",
            )
            require(
                fixture_id not in self.records,
                f"fixture index contains duplicate ID: {fixture_id}",
            )
            self.records[fixture_id] = row

    def _existing(self, fixture_id: str) -> PlannedFixtureBase:
        record = self.records[fixture_id]
        relative = record.get("file")
        require(
            isinstance(relative, str) and relative,
            f"fixture {fixture_id} lacks its file",
        )
        target = fixture_tool.schema_target_from_record(
            self.catalog, record["schema"]
        )
        value = json.loads(
            (self.fixture_root / relative).read_text(encoding="utf-8")
        )
        diagnostics = self.catalog.target_validator(
            target
        ).validate_subschema(value, target.schema, target.schema_path)
        require(
            not diagnostics,
            f"existing positive fixture is no longer schema-valid: {fixture_id}",
        )
        return PlannedFixtureBase(
            fixture_id,
            "existing",
            str(record["role"]),
            relative,
            target,
            value,
        )

    def bases(self, key: Key) -> list[PlannedFixtureBase]:
        if key.category == "client_request":
            return [
                self._existing(
                    f"operation:client_request:{key.name}:{role}"
                )
                for role in ("params", "result")
            ]

        if key.category == "server_notification":
            fixture_id = (
                "baseline:server_notification:ServerNotification:"
                f"method:{key.name}"
            )
            if fixture_id in self.records:
                return [self._existing(fixture_id)]
            target, index, branch = self.catalog.method_target(
                key.category, key.name
            )
            branch_path = fixture_tool.pointer_child(
                fixture_tool.pointer_child("#", "oneOf"), index
            )
            value = self.synthesizer.sample(target, branch, branch_path)
            return [
                PlannedFixtureBase(
                    fixture_id,
                    "planned",
                    "server_notification_identity",
                    (
                        "cases/baseline/server_notification/"
                        f"{fixture_tool.slug(key.name)}.json"
                    ),
                    target,
                    value,
                )
            ]

        fixture_id = f"union:{key.domain}:{key.name}"
        if fixture_id in self.records:
            return [self._existing(fixture_id)]
        target = self.catalog.union_target(key.domain)
        index, branch, forced_scalar, forced_property = (
            fixture_tool.branch_for_union_identity(
                self.catalog,
                target,
                key.field,
                key.name,
            )
        )
        keyword = "oneOf" if "oneOf" in target.schema else "anyOf"
        branch_path = fixture_tool.pointer_child(
            fixture_tool.pointer_child(target.schema_path, keyword), index
        )
        value = self.synthesizer.sample(
            target,
            branch,
            branch_path,
            forced_scalar,
            forced_property,
        )
        return [
            PlannedFixtureBase(
                fixture_id,
                "planned",
                "union_branch",
                (
                    f"cases/unions/{fixture_tool.slug(key.domain)}/"
                    f"{fixture_tool.slug(key.name)}.json"
                ),
                target,
                value,
            )
        ]

    def _diagnostic_codes(
        self, target: fixture_tool.SchemaTarget, value: Any
    ) -> list[str]:
        diagnostics = self.catalog.target_validator(
            target
        ).validate_subschema(value, target.schema, target.schema_path)
        return sorted({item.code for item in diagnostics})

    def _wrong_type_case(
        self,
        base: PlannedFixtureBase,
        location: fixture_tool.RequiredLocation,
        requiredness: str,
    ) -> dict[str, Any]:
        original = fixture_tool.get_instance_path(
            base.value, location.instance_path
        )
        validator = self.catalog.target_validator(base.target)
        selected: Any = fixture_tool.NO_WRONG_TYPE_MUTATION
        codes: list[str] = []
        validation_scope = "root_schema"
        for candidate in fixture_tool.WRONG_TYPE_CANDIDATES:
            if fixture_tool.canonical_json(candidate) == fixture_tool.canonical_json(
                original
            ):
                continue
            mutated = copy.deepcopy(base.value)
            parent, field = fixture_tool.get_parent_path(
                mutated, location.instance_path
            )
            parent[field] = copy.deepcopy(candidate)
            root_diagnostics = validator.validate_subschema(
                mutated, base.target.schema, base.target.schema_path
            )
            owner = fixture_tool.get_instance_path(
                mutated, location.required_owner_instance_path
            )
            owner_diagnostics = validator.validate_subschema(
                owner,
                location.required_owner_schema,
                location.required_owner_schema_path,
            )
            diagnostics = root_diagnostics or owner_diagnostics
            if diagnostics:
                selected = candidate
                codes = sorted({item.code for item in diagnostics})
                validation_scope = (
                    "root_schema"
                    if root_diagnostics
                    else "selected_schema_fragment"
                )
                break
        instance_path = fixture_tool.json_path(location.instance_path)
        result: dict[str, Any] = {
            "id": mutation_case_id(
                "wrong-type", base.fixture_id, instance_path
            ),
            "base_fixture_id": base.fixture_id,
            "kind": "wrong_type",
            "instance_path": instance_path,
            "property_schema_path": location.schema_path,
            "selected_owner_schema_path": (
                location.required_owner_schema_path
            ),
            "requiredness": requiredness,
        }
        if selected is fixture_tool.NO_WRONG_TYPE_MUTATION:
            result.update(
                {
                    "applicability": "not_applicable",
                    "not_applicable_reason": (
                        "schema accepts every deterministic JSON-type candidate; "
                        "a matching ProtocolDefinedOpaqueJson path is required"
                    ),
                }
            )
        else:
            result.update(
                {
                    "applicability": "required",
                    "expected_valid": False,
                    "expected_diagnostic_codes": codes,
                    "validation_scope": validation_scope,
                    "replacement_json_type": fixture_tool.instance_type(
                        selected
                    ),
                }
            )
        return result

    def _discriminator_cases(
        self, key: Key, base: PlannedFixtureBase
    ) -> dict[str, list[dict[str, Any]]]:
        groups: dict[str, list[dict[str, Any]]] = {
            "future_discriminator": [],
            "missing_discriminator": [],
            "conflicting_discriminator": [],
        }
        if key.category == "client_request":
            groups["future_discriminator"].append(
                {
                    "id": "method:client_request:future-unknown",
                    "kind": "future_discriminator",
                    "base_fixture_id": None,
                    "instance_path": "$/method",
                    "applicability": "shared_category_fixture",
                    "expected_valid": False,
                    "expected_diagnostic_codes": ["one_of_zero"],
                }
            )
            for group in (
                "missing_discriminator",
                "conflicting_discriminator",
            ):
                groups[group].append(
                    {
                        "id": (
                            f"mutation:{group}:"
                            f"client_request:{key.name}:%24%2Fmethod"
                        ),
                        "kind": group,
                        "base_fixture_id": None,
                        "instance_path": "$/method",
                        "applicability": "not_applicable",
                        "not_applicable_reason": (
                            "request params/result fixture is not a JSON-RPC "
                            "method envelope; exact method bytes are covered "
                            "by the required runtime transcript"
                        ),
                    }
                )
            return groups

        value = copy.deepcopy(base.value)
        if key.category == "server_notification":
            discriminator_path = "$/method"
            value["method"] = f"future/{fixture_tool.slug(key.name)}"
            missing = copy.deepcopy(base.value)
            missing.pop("method")
        elif key.field == "type":
            discriminator_path = "$/type"
            value["type"] = f"future{pascal(key.domain)}Alternative"
            missing = copy.deepcopy(base.value)
            missing.pop("type")
        elif isinstance(value, str):
            discriminator_path = "$"
            value = f"future{pascal(key.domain)}Alternative"
            missing = None
        else:
            require(
                isinstance(value, dict) and key.name in value,
                f"externally tagged fixture lacks {key.name}: {key.compact()}",
            )
            discriminator_path = (
                "$/" + key.name.replace("~", "~0").replace("/", "~1")
            )
            payload = value.pop(key.name)
            value[f"future{pascal(key.domain)}Alternative"] = payload
            missing = {}

        future_codes = self._diagnostic_codes(base.target, value)
        require(
            future_codes,
            f"future discriminator unexpectedly validates: {key.compact()}",
        )
        groups["future_discriminator"].append(
            {
                "id": mutation_case_id(
                    "future-discriminator",
                    base.fixture_id,
                    discriminator_path,
                ),
                "kind": "future_discriminator",
                "base_fixture_id": base.fixture_id,
                "instance_path": discriminator_path,
                "applicability": "required",
                "expected_valid": False,
                "expected_diagnostic_codes": future_codes,
                "expected_typed_diagnostic": {
                    "kind": "UnknownDiscriminator",
                    "severity": "ForwardCompatibility",
                },
            }
        )
        if missing is None:
            groups["missing_discriminator"].append(
                {
                    "id": mutation_case_id(
                        "missing-discriminator",
                        base.fixture_id,
                        discriminator_path,
                    ),
                    "kind": "missing_discriminator",
                    "base_fixture_id": base.fixture_id,
                    "instance_path": discriminator_path,
                    "applicability": "not_applicable",
                    "not_applicable_reason": (
                        "scalar externally selected alternative has no "
                        "discriminator property to remove"
                    ),
                }
            )
        else:
            missing_codes = self._diagnostic_codes(base.target, missing)
            require(
                missing_codes,
                f"missing discriminator unexpectedly validates: {key.compact()}",
            )
            groups["missing_discriminator"].append(
                {
                    "id": mutation_case_id(
                        "missing-discriminator",
                        base.fixture_id,
                        discriminator_path,
                    ),
                    "kind": "missing_discriminator",
                    "base_fixture_id": base.fixture_id,
                    "instance_path": discriminator_path,
                    "applicability": "required",
                    "expected_valid": False,
                    "expected_diagnostic_codes": missing_codes,
                    "expected_typed_diagnostic": {
                        "kind": "MalformedKnownPayload",
                        "severity": "ProtocolWarning",
                    },
                }
            )

        conflict_pairs = {
            "SessionSource": {"custom": "subAgent", "subAgent": "custom"},
            "SubAgentSource": {
                "thread_spawn": "other",
                "other": "thread_spawn",
            },
        }
        other_name = conflict_pairs.get(key.domain, {}).get(key.name)
        if other_name is None:
            groups["conflicting_discriminator"].append(
                {
                    "id": mutation_case_id(
                        "conflicting-discriminator",
                        base.fixture_id,
                        discriminator_path,
                    ),
                    "kind": "conflicting_discriminator",
                    "base_fixture_id": base.fixture_id,
                    "instance_path": discriminator_path,
                    "applicability": "not_applicable",
                    "not_applicable_reason": (
                        "the selected JSON representation cannot contain two "
                        "distinct known discriminators"
                    ),
                }
            )
        else:
            target = self.catalog.union_target(key.domain)
            _, branch, forced_scalar, forced_property = (
                fixture_tool.branch_for_union_identity(
                    self.catalog,
                    target,
                    "$variant",
                    other_name,
                )
            )
            other_value = self.synthesizer.sample(
                target,
                branch,
                target.schema_path,
                forced_scalar,
                forced_property,
            )
            require(
                isinstance(base.value, dict)
                and isinstance(other_value, dict)
                and other_name in other_value,
                f"conflict fixture shape changed for {key.compact()}",
            )
            conflict = copy.deepcopy(base.value)
            conflict[other_name] = other_value[other_name]
            conflict_codes = self._diagnostic_codes(base.target, conflict)
            require(
                conflict_codes,
                f"conflicting discriminator unexpectedly validates: {key.compact()}",
            )
            groups["conflicting_discriminator"].append(
                {
                    "id": mutation_case_id(
                        "conflicting-discriminator",
                        base.fixture_id,
                        discriminator_path,
                    ),
                    "kind": "conflicting_discriminator",
                    "base_fixture_id": base.fixture_id,
                    "instance_path": discriminator_path,
                    "conflicting_known_identity": other_name,
                    "applicability": "required",
                    "expected_valid": False,
                    "expected_diagnostic_codes": conflict_codes,
                    "expected_typed_diagnostic": {
                        "kind": "MalformedKnownPayload",
                        "severity": "ProtocolWarning",
                    },
                }
            )
        return groups

    def plan(
        self,
        key: Key,
        current_fixture_ids: Sequence[str],
        open_enum_names: set[str],
    ) -> tuple[dict[str, Any], dict[str, Any]]:
        bases = self.bases(key)
        positive_ids = [base.fixture_id for base in bases]
        positive = {
            "existing_fixture_ids": sorted(
                fixture_id
                for fixture_id in current_fixture_ids
                if fixture_id in self.records
            ),
            "required_fixture_ids": sorted(positive_ids),
            "planned_fixture_ids": sorted(
                base.fixture_id
                for base in bases
                if base.status == "planned"
            ),
            "base_cases": [
                {
                    "id": base.fixture_id,
                    "status": base.status,
                    "role": base.role,
                    "file": base.file,
                    "schema": base.target.to_json(),
                }
                for base in bases
            ],
        }
        mutation_groups: dict[str, list[dict[str, Any]]] = {
            "required_field_removal": [],
            "optional_field_omission": [],
            "nullable_null": [],
            "nullable_value": [],
            "nullable_omitted": [],
            "wrong_type": [],
            "future_discriminator": [],
            "missing_discriminator": [],
            "conflicting_discriminator": [],
            "future_open_enum": [],
            "malformed_known": [],
        }
        for base in bases:
            evidence = fixture_tool.mutation_evidence(
                self.catalog, base.target, base.value
            )
            removals = {
                record["instance_path"]: record
                for record in evidence["removals"]
            }
            omissions = {
                record["instance_path"]: record
                for record in evidence["optional_omissions"]
            }
            required_locations = fixture_tool.collect_required_locations(
                self.catalog, base.target, base.value
            )
            optional_locations = (
                fixture_tool.collect_optional_present_locations(
                    self.catalog, base.target, base.value
                )
            )
            for requiredness, locations in (
                ("required", required_locations),
                ("optional", optional_locations),
            ):
                for location in locations:
                    instance_path = fixture_tool.json_path(
                        location.instance_path
                    )
                    if requiredness == "required":
                        removal = removals[instance_path]
                        mutation_groups["required_field_removal"].append(
                            {
                                "id": mutation_case_id(
                                    "required-remove",
                                    base.fixture_id,
                                    instance_path,
                                ),
                                "kind": "required_field_removal",
                                "base_fixture_id": base.fixture_id,
                                "instance_path": instance_path,
                                "property_schema_path": location.schema_path,
                                "selected_owner_schema_path": (
                                    location.required_owner_schema_path
                                ),
                                "applicability": "required",
                                "expected_valid": False,
                                "expected_diagnostic_codes": removal[
                                    "diagnostic_codes"
                                ],
                                "validation_scope": removal[
                                    "validation_scope"
                                ],
                            }
                        )
                    elif instance_path in omissions:
                        mutation_groups["optional_field_omission"].append(
                            {
                                "id": mutation_case_id(
                                    "optional-omit",
                                    base.fixture_id,
                                    instance_path,
                                ),
                                "kind": "optional_field_omission",
                                "base_fixture_id": base.fixture_id,
                                "instance_path": instance_path,
                                "property_schema_path": location.schema_path,
                                "applicability": "required",
                                "expected_valid": True,
                                "expected_diagnostic_codes": [],
                            }
                        )

                    mutation_groups["wrong_type"].append(
                        self._wrong_type_case(
                            base, location, requiredness
                        )
                    )

                    mutated_null = copy.deepcopy(base.value)
                    parent, field = fixture_tool.get_parent_path(
                        mutated_null, location.instance_path
                    )
                    parent[field] = None
                    null_codes = self._diagnostic_codes(
                        base.target, mutated_null
                    )
                    if not null_codes:
                        value_id = mutation_case_id(
                            "nullable-value",
                            base.fixture_id,
                            instance_path,
                        )
                        null_id = mutation_case_id(
                            "nullable-null",
                            base.fixture_id,
                            instance_path,
                        )
                        mutation_groups["nullable_value"].append(
                            {
                                "id": value_id,
                                "kind": "nullable_value",
                                "base_fixture_id": base.fixture_id,
                                "instance_path": instance_path,
                                "property_schema_path": location.schema_path,
                                "applicability": "required",
                                "expected_valid": True,
                                "expected_diagnostic_codes": [],
                            }
                        )
                        mutation_groups["nullable_null"].append(
                            {
                                "id": null_id,
                                "kind": "nullable_null",
                                "base_fixture_id": base.fixture_id,
                                "instance_path": instance_path,
                                "property_schema_path": location.schema_path,
                                "applicability": "required",
                                "expected_valid": True,
                                "expected_diagnostic_codes": [],
                            }
                        )
                        omission_kind = (
                            "optional-omit"
                            if requiredness == "optional"
                            else "required-remove"
                        )
                        mutation_groups["nullable_omitted"].append(
                            {
                                "id": mutation_case_id(
                                    "nullable-omitted",
                                    base.fixture_id,
                                    instance_path,
                                ),
                                "kind": "nullable_omitted",
                                "base_fixture_id": base.fixture_id,
                                "instance_path": instance_path,
                                "property_schema_path": location.schema_path,
                                "applicability": (
                                    "required"
                                    if requiredness == "optional"
                                    else "negative_requiredness_control"
                                ),
                                "expected_valid": (
                                    requiredness == "optional"
                                ),
                                "reuses_case_id": mutation_case_id(
                                    omission_kind,
                                    base.fixture_id,
                                    instance_path,
                                ),
                            }
                        )

                    enum_names = sorted(
                        _schema_reference_names(location.schema)
                        & open_enum_names
                    )
                    for enum_name in enum_names:
                        mutated_enum = copy.deepcopy(base.value)
                        enum_parent, enum_field = (
                            fixture_tool.get_parent_path(
                                mutated_enum, location.instance_path
                            )
                        )
                        enum_parent[enum_field] = (
                            f"future{enum_name}Value"
                        )
                        mutation_groups["future_open_enum"].append(
                            {
                                "id": mutation_case_id(
                                    f"future-open-enum-{enum_name}",
                                    base.fixture_id,
                                    instance_path,
                                ),
                                "kind": "future_open_enum",
                                "base_fixture_id": base.fixture_id,
                                "instance_path": instance_path,
                                "property_schema_path": location.schema_path,
                                "open_enum_definition": enum_name,
                                "applicability": "required",
                                "expected_valid": not bool(
                                    self._diagnostic_codes(
                                        base.target, mutated_enum
                                    )
                                ),
                                "expected_diagnostic_codes": (
                                    self._diagnostic_codes(
                                        base.target, mutated_enum
                                    )
                                ),
                                "expected_typed_diagnostic": {
                                    "kind": "UnknownEnumValue",
                                    "severity": "ForwardCompatibility",
                                },
                            }
                        )

            if key.category != "client_request" or base is bases[0]:
                discriminator_groups = self._discriminator_cases(key, base)
                for group, cases in discriminator_groups.items():
                    mutation_groups[group].extend(cases)

            discriminator_path = (
                "$/method"
                if key.category
                in {"client_request", "server_notification"}
                else "$/type"
                if key.field == "type"
                else "$"
            )
            payload_wrong = next(
                (
                    case
                    for case in mutation_groups["wrong_type"]
                    if case["base_fixture_id"] == base.fixture_id
                    and case["applicability"] == "required"
                    and (
                        key.field == "$variant"
                        or case["instance_path"] != discriminator_path
                    )
                ),
                None,
            )
            if key.category in {
                "item_discriminator",
                "tagged_union_discriminator",
            }:
                if payload_wrong is None:
                    mutation_groups["malformed_known"].append(
                        {
                            "id": mutation_case_id(
                                "malformed-known",
                                base.fixture_id,
                                discriminator_path,
                            ),
                            "kind": "malformed_known",
                            "base_fixture_id": base.fixture_id,
                            "instance_path": discriminator_path,
                            "applicability": "not_applicable",
                            "not_applicable_reason": (
                                "known alternative has no payload field that "
                                "can be malformed while retaining its discriminator"
                            ),
                        }
                    )
                else:
                    mutation_groups["malformed_known"].append(
                        {
                            "id": mutation_case_id(
                                "malformed-known",
                                base.fixture_id,
                                payload_wrong["instance_path"],
                            ),
                            "kind": "malformed_known",
                            "base_fixture_id": base.fixture_id,
                            "instance_path": payload_wrong[
                                "instance_path"
                            ],
                            "applicability": "required",
                            "reuses_case_id": payload_wrong["id"],
                            "expected_typed_diagnostic": {
                                "kind": "MalformedKnownPayload",
                                "severity": "ProtocolWarning",
                            },
                        }
                    )

        if key.category == "client_request":
            result_base = next(
                base
                for base in bases
                if base.fixture_id.endswith(":result")
            )
            result_wrong = next(
                (
                    case
                    for case in mutation_groups["wrong_type"]
                    if case["base_fixture_id"] == result_base.fixture_id
                    and case["applicability"] == "required"
                ),
                None,
            )
            if result_wrong is not None:
                mutation_groups["malformed_known"].append(
                    {
                        "id": mutation_case_id(
                            "malformed-known-result",
                            result_base.fixture_id,
                            result_wrong["instance_path"],
                        ),
                        "kind": "malformed_known",
                        "base_fixture_id": result_base.fixture_id,
                        "instance_path": result_wrong["instance_path"],
                        "applicability": "required",
                        "reuses_case_id": result_wrong["id"],
                        "expected_typed_diagnostic": {
                            "kind": "MalformedKnownPayload",
                            "severity": "ProtocolWarning",
                        },
                    }
                )
            else:
                result_codes = self._diagnostic_codes(
                    result_base.target, None
                )
                require(
                    result_codes,
                    f"non-object Unit/result mutation unexpectedly validates: "
                    f"{key.compact()}",
                )
                mutation_groups["malformed_known"].append(
                    {
                        "id": mutation_case_id(
                            "malformed-known-result",
                            result_base.fixture_id,
                            "$",
                        ),
                        "kind": "malformed_known",
                        "base_fixture_id": result_base.fixture_id,
                        "instance_path": "$",
                        "applicability": "required",
                        "expected_valid": False,
                        "expected_diagnostic_codes": result_codes,
                        "replacement_json_type": "null",
                        "expected_typed_diagnostic": {
                            "kind": "MalformedKnownPayload",
                            "severity": "ProtocolWarning",
                        },
                    }
                )
        elif key.category == "server_notification":
            envelope = bases[0]
            notification_wrong = next(
                (
                    case
                    for case in mutation_groups["wrong_type"]
                    if case["base_fixture_id"] == envelope.fixture_id
                    and case["applicability"] == "required"
                    and case["instance_path"] != "$/method"
                ),
                None,
            )
            require(
                notification_wrong is not None,
                f"notification lacks a malformed-known payload mutation: "
                f"{key.compact()}",
                "MalformedFixturePlan",
            )
            mutation_groups["malformed_known"].append(
                {
                    "id": mutation_case_id(
                        "malformed-known",
                        envelope.fixture_id,
                        notification_wrong["instance_path"],
                    ),
                    "kind": "malformed_known",
                    "base_fixture_id": envelope.fixture_id,
                    "instance_path": notification_wrong["instance_path"],
                    "applicability": "required",
                    "reuses_case_id": notification_wrong["id"],
                    "expected_typed_diagnostic": {
                        "kind": "MalformedKnownPayload",
                        "severity": "ProtocolWarning",
                    },
                }
            )

        runtime_case_ids: list[str] = []
        if key.category == "client_request":
            runtime_case_ids = [
                f"wire:{key.name}:{case}"
                for case in (
                    "accepted",
                    "success",
                    "remote-error",
                    "synchronous-rejection",
                    "cancel",
                    "stale-generation",
                    "callback-order",
                    "callback-exception",
                    "raw-observer-coexistence",
                )
            ]
        elif key.category == "server_notification":
            runtime_case_ids = [
                f"wire:{key.name}:{case}"
                for case in (
                    "valid",
                    "malformed-known",
                    "raw-observer-coexistence",
                    "bounded-preservation",
                )
            ]

        for cases in mutation_groups.values():
            cases.sort(key=lambda record: str(record["id"]))
        return positive, {
            "schema_mutations": mutation_groups,
            "runtime_case_ids": runtime_case_ids,
        }


def compatibility_note(key: Key, registry: Mapping[str, Any]) -> str:
    if key.category == "client_request":
        if key.name in CURRENT_TYPED_METHODS:
            extra = "retain the existing overload as a forwarding compatibility API"
            if key.name == "turn/interrupt":
                extra += " and retain TurnInterruptResult through a typed::Unit alias/wrapper"
            return extra
        if key.name == "thread/rollback":
            return "new grouped method is explicitly deprecated because the stable wire method is deprecated"
        return "additive grouped-facade method; no direct AppServerClient method or sessions facade"
    if key.category == "item_discriminator":
        if key.domain == "ThreadItem":
            if registry["typed_schema_status"] == "Partial":
                return "complete the existing alternative without changing modeled reducer semantics; retain Item = ThreadItem"
            return "new known alternative must preserve the observability previously provided by UnknownItem"
        return "ResponseItem remains directionally distinct and gains its own UnknownResponseItem"
    if key.category == "server_notification":
        return (
            "retain existing reducer semantics"
            if registry["typed_schema_status"] == "Partial"
            else "new typed event remains observable through bounded preservation without new canonical semantics"
        )
    special = {
        "AskForApproval": "complete the canonical type and provide deliberate ApprovalPolicy compatibility/conversion",
        "SandboxPolicy": "complete the canonical tagged union without preserving the partial sandbox model",
        "ThreadStatus": "replace the partial wrapper with the complete tagged union and document the source change",
        "UserInput": "introduce canonical UserInput and retain deliberate TurnInput compatibility/conversion",
        "SessionSource": "public protocol type only; do not add a sessions facade",
    }
    return special.get(key.domain, "additive handwritten alternative; preserve open-enum and raw-retention behavior")


def backend_behavior(key: Key, registry: Mapping[str, Any]) -> str:
    if key.category == "client_request":
        return "NotApplicable: typed RawProtocol operation only; no BackendCommand"
    if key.category == "server_notification":
        if registry["typed_schema_status"] == "Partial":
            return "retain the existing modeled reducer behavior byte/semantically unchanged"
        return "typed event -> preserveUnmodeledTypedEvent -> CodexExtensionReceived -> bounded recent-extension state"
    if key.category == "item_discriminator":
        if key.domain == "ThreadItem" and registry["typed_schema_status"] == "Partial":
            return "retain existing item reducer semantics"
        if key.domain == "ThreadItem":
            return "preserve common metadata/raw payload through the existing generic item/extension contract"
        return "NotApplicable unless reached by an existing event; no new canonical semantics"
    return "NotApplicable directly; verified through every reaching stable root"


def frontend_behavior(key: Key, registry: Mapping[str, Any]) -> str:
    if key.category == "client_request":
        return "no Frontend Protocol operation, command, or security decision"
    if key.category == "server_notification" and registry["typed_schema_status"] == "Partial":
        return "existing Frontend Protocol v1 event/snapshot behavior remains byte/semantically unchanged"
    if key.category in {"server_notification", "item_discriminator"}:
        return "existing bounded/redacted generic extension or item metadata contract only; no v1 schema field"
    return "no direct frontend exposure; owning-root behavior remains unchanged"


def public_requirement(
    key: Key,
    contract: Mapping[str, Any] | None,
    schema_branch_title: str | None = None,
) -> dict[str, Any]:
    if key.category == "client_request":
        assert contract is not None
        facade, method = OPERATION_API_NAMES[key.name]
        result_type = (
            "typed::Unit"
            if contract["result_contract_kind"] == "Unit"
            else f"typed::{contract['result_type_identity']}"
        )
        return {
            "facade": facade,
            "method": method,
            "params_type": f"typed::{contract['parameter_type_identity']}",
            "successful_result_type": result_type,
        }
    if key.category == "server_notification":
        payload_type = SERVER_NOTIFICATION_PAYLOAD_TYPES[key.name]
        adapter = SERVER_NOTIFICATION_SEMANTIC_ADAPTERS.get(key.name)
        return {
            "canonical_payload_aggregate": f"typed::{payload_type}",
            "existing_semantic_adapter": adapter,
            "payload_direction": "incoming",
        }
    if key.category == "item_discriminator":
        if key.domain == "ThreadItem":
            alternative = THREAD_ITEM_ALTERNATIVES[key.name]
            unknown = "UnknownItem"
            aliases = THREAD_ITEM_COMPATIBILITY_ALIASES.get(key.name, [])
        else:
            alternative = RESPONSE_ITEM_ALTERNATIVES[key.name]
            unknown = "UnknownResponseItem"
            aliases = []
        return {
            "variant": f"typed::{key.domain}",
            "canonical_alternative": f"typed::{alternative}",
            "schema_branch_title": schema_branch_title,
            "future_unknown_alternative": f"typed::{unknown}",
            "compatibility_aliases": aliases,
            "variant_compatibility_aliases": (
                ["typed::Item = typed::ThreadItem"]
                if key.domain == "ThreadItem"
                else []
            ),
        }
    alternative = NESTED_UNION_ALTERNATIVES[(key.domain, key.name)]
    return {
        "union": f"typed::{key.domain}",
        "canonical_alternative": f"typed::{alternative}",
        "schema_branch_title": schema_branch_title,
        "future_unknown_alternative": (
            f"typed::{NESTED_UNION_FUTURE_UNKNOWN[key.domain]}"
        ),
        "compatibility_aliases": NESTED_UNION_COMPATIBILITY_ALIASES.get(
            (key.domain, key.name), []
        ),
    }


def identity_directionality(
    key: Key, definition_directions: Mapping[str, str]
) -> str:
    if key.category == "client_request":
        return "EncodeOnly"
    if key.category in {"server_notification", "item_discriminator"}:
        return "DecodeOnly"
    require(
        key.category == "tagged_union_discriminator"
        and key.domain in definition_directions,
        f"identity has no schema direction: {key.compact()}",
        "DirectionalityMismatch",
    )
    return definition_directions[key.domain]


def reviewed_identity_directionality(key: Key) -> str:
    if key.category == "client_request":
        return "EncodeOnly"
    if key.category in {"server_notification", "item_discriminator"}:
        return "DecodeOnly"
    require(
        key.category == "tagged_union_discriminator",
        f"reviewed identity has an unexpected category: {key.compact()}",
        "DirectionalityMismatch",
    )
    return (
        "Both"
        if key.domain in {"AskForApproval", "SandboxPolicy", "UserInput"}
        else "DecodeOnly"
    )


def discriminator_behavior(key: Key, directionality: str) -> dict[str, str]:
    require(
        directionality in VALID_DIRECTIONALITIES,
        f"identity has invalid directionality: {key.compact()}",
        "DirectionalityMismatch",
    )
    if key.field == "method":
        representation = "json_rpc_method"
        field_path = "$.method"
    elif key.field == "type":
        representation = "object_discriminator_property"
        field_path = "$.type"
    elif (key.domain, key.name) in SCALAR_VARIANT_IDENTITIES:
        representation = "scalar_union_literal"
        field_path = "$"
    else:
        require(
            key.field == "$variant",
            f"unknown discriminator representation: {key.compact()}",
            "DiscriminatorLiteralMismatch",
        )
        representation = "externally_tagged_object_key"
        field_path = "$." + key.name
    return {
        "directionality": directionality,
        "representation": representation,
        "field_path": field_path,
        "exact_literal": key.name,
        "encode_behavior": (
            "emit_exact_literal"
            if directionality in {"Both", "EncodeOnly"}
            else "not_applicable_decode_only"
        ),
        "decode_behavior": (
            "registry_select_exact_literal_then_decode"
            if directionality in {"Both", "DecodeOnly"}
            else "not_applicable_encode_only"
        ),
        "future_literal_behavior": (
            "explicit_unknown_alternative"
            if directionality in {"Both", "DecodeOnly"}
            else "outgoing_unknown_alternative_rejected"
        ),
        "malformed_known_behavior": (
            "MalformedKnownPayload/ProtocolWarning"
            if directionality in {"Both", "DecodeOnly"}
            else "synchronous_local_rejection"
        ),
    }


def private_requirement(
    key: Key, directionality: str
) -> dict[str, Any]:
    family = {
        "client_request": "ClientRequest",
        "server_notification": "ServerNotification",
        "item_discriminator": key.domain,
        "tagged_union_discriminator": key.domain,
    }[key.category]
    return {
        "descriptor_family": family,
        "descriptor_key": key.compact(),
        "dispatch_rule": "lookup begins with the exact canonical ProtocolSurfaceRegistry row",
        "discriminator_behavior": discriminator_behavior(
            key, directionality
        ),
    }


def validate_reviewed_public_mappings(
    a1_keys: Sequence[Key],
    manifest_rows: Mapping[Key, Mapping[str, Any]],
    catalog: fixture_tool.SchemaCatalog,
) -> dict[Key, str | None]:
    notification_keys = {
        key.name for key in a1_keys if key.category == "server_notification"
    }
    thread_item_keys = {
        key.name
        for key in a1_keys
        if key.category == "item_discriminator"
        and key.domain == "ThreadItem"
    }
    response_item_keys = {
        key.name
        for key in a1_keys
        if key.category == "item_discriminator"
        and key.domain == "ResponseItem"
    }
    nested_keys = {
        (key.domain, key.name)
        for key in a1_keys
        if key.category == "tagged_union_discriminator"
    }
    nested_domains = {domain for domain, _ in nested_keys}

    require(
        len(SERVER_NOTIFICATION_PAYLOAD_TYPES) == 37
        and set(SERVER_NOTIFICATION_PAYLOAD_TYPES) == notification_keys,
        "reviewed server-notification public mapping must contain exactly "
        "the 37 frozen identities",
        "ReviewedPublicMappingMismatch",
    )
    require(
        len(THREAD_ITEM_ALTERNATIVES) == 18
        and set(THREAD_ITEM_ALTERNATIVES) == thread_item_keys,
        "reviewed ThreadItem public mapping must contain exactly 18 alternatives",
        "ReviewedPublicMappingMismatch",
    )
    require(
        len(RESPONSE_ITEM_ALTERNATIVES) == 16
        and set(RESPONSE_ITEM_ALTERNATIVES) == response_item_keys,
        "reviewed ResponseItem public mapping must contain exactly 16 alternatives",
        "ReviewedPublicMappingMismatch",
    )
    require(
        len(NESTED_UNION_ALTERNATIVES) == 58
        and set(NESTED_UNION_ALTERNATIVES) == nested_keys,
        "reviewed nested-union public mapping must contain exactly 58 alternatives",
        "ReviewedPublicMappingMismatch",
    )
    require(
        len(NESTED_UNION_FUTURE_UNKNOWN) == 17
        and set(NESTED_UNION_FUTURE_UNKNOWN) == nested_domains,
        "reviewed nested-union future-unknown mapping must contain exactly "
        "17 distinct families",
        "ReviewedPublicMappingMismatch",
    )
    require(
        len(SERVER_NOTIFICATION_SEMANTIC_ADAPTERS) == 12
        and set(SERVER_NOTIFICATION_SEMANTIC_ADAPTERS).issubset(
            notification_keys
        ),
        "reviewed existing notification semantic adapters changed",
        "ReviewedPublicMappingMismatch",
    )

    for key in a1_keys:
        if key.category != "server_notification":
            continue
        params = manifest_rows[key].get("params")
        params_type = params.get("type") if isinstance(params, Mapping) else None
        require(
            params_type == SERVER_NOTIFICATION_PAYLOAD_TYPES[key.name],
            f"notification payload mapping/schema params title mismatch: "
            f"{key.compact()}: {params_type!r}",
            "ReviewedPublicMappingMismatch",
        )

    branch_titles: dict[Key, str | None] = {}
    no_title_mappings = {
        ("AskForApproval", "never"),
        ("AskForApproval", "on-request"),
        ("AskForApproval", "untrusted"),
        ("SessionSource", "appServer"),
        ("SessionSource", "cli"),
        ("SessionSource", "exec"),
        ("SessionSource", "unknown"),
        ("SessionSource", "vscode"),
        ("SubAgentSource", "compact"),
        ("SubAgentSource", "memory_consolidation"),
        ("SubAgentSource", "review"),
    }
    for key in a1_keys:
        if key.category not in {
            "item_discriminator",
            "tagged_union_discriminator",
        }:
            continue
        target = catalog.union_target(key.domain)
        _, branch, _, _ = fixture_tool.branch_for_union_identity(
            catalog, target, key.field, key.name
        )
        title = branch.get("title") if isinstance(branch, Mapping) else None
        require(
            title is None or isinstance(title, str),
            f"schema branch title is malformed: {key.compact()}",
            "ReviewedPublicMappingMismatch",
        )
        branch_titles[key] = title
        mapped = (
            THREAD_ITEM_ALTERNATIVES[key.name]
            if key.domain == "ThreadItem"
            else RESPONSE_ITEM_ALTERNATIVES[key.name]
            if key.domain == "ResponseItem"
            else NESTED_UNION_ALTERNATIVES[(key.domain, key.name)]
        )
        if (key.domain, key.name) in no_title_mappings:
            require(
                title is None,
                f"reviewed open-string branch unexpectedly gained a schema title: "
                f"{key.compact()}",
                "ReviewedPublicMappingMismatch",
            )
        else:
            require(
                title == mapped,
                f"reviewed alternative/schema branch title mismatch: "
                f"{key.compact()}: schema={title!r}, mapped={mapped!r}",
                "ReviewedPublicMappingMismatch",
            )

    # The known wire alternative "unknown" and the future-unknown public
    # alternative must be independently named.
    require(
        NESTED_UNION_ALTERNATIVES[("SessionSource", "unknown")]
        != NESTED_UNION_FUTURE_UNKNOWN["SessionSource"],
        "known SessionSource 'unknown' conflates with its future alternative",
        "ReviewedPublicMappingMismatch",
    )
    require(
        NESTED_UNION_ALTERNATIVES[("CommandAction", "unknown")]
        != NESTED_UNION_FUTURE_UNKNOWN["CommandAction"],
        "known CommandAction 'unknown' conflates with its future alternative",
        "ReviewedPublicMappingMismatch",
    )
    return branch_titles


def validate_input_versions(documents: Iterable[Mapping[str, Any]]) -> None:
    for document in documents:
        version = str(document.get("codex_version", ""))
        require(version in {"0.144.6", CODEX_VERSION}, f"unexpected Codex evidence version: {version!r}")


def load_live_inputs(arguments: argparse.Namespace) -> LiveInputs:
    manifest = load_json(arguments.manifest)
    assignments_document = load_json(arguments.assignments)
    reachability_document = load_json(arguments.reachability)
    contracts_document = load_json(arguments.contracts)
    completeness_document = load_json(arguments.schema_completeness)
    fixture_document = load_json(arguments.fixture_coverage)
    validate_input_versions(
        (
            manifest,
            assignments_document,
            reachability_document,
            contracts_document,
            completeness_document,
            fixture_document,
        )
    )

    registry_rows = surface.parse_registry_data(arguments.registry)
    registry = indexed(registry_rows, "registry")
    manifest_rows = indexed(records(manifest, ("entries",), "surface manifest"), "manifest")
    assignments = indexed(
        records(assignments_document, ("assignments",), "module/slice assignment"),
        "assignment",
    )
    reachability = indexed(
        records(reachability_document, ("records",), "nested reachability"),
        "reachability",
    )
    contracts = indexed(
        records(contracts_document, ("contracts",), "operation contracts"),
        "operation contract",
    )
    completeness = indexed(
        records(completeness_document, ("records",), "schema completeness"),
        "schema completeness",
    )
    fixture_coverage = indexed(
        records(fixture_document, ("identity_coverage",), "fixture coverage"),
        "fixture coverage",
    )

    if set(registry) != set(manifest_rows):
        fail("registry/manifest exact-key mismatch", "LiveIdentityMismatch")
    if set(assignments) != set(manifest_rows):
        fail("assignment/manifest exact-key mismatch", "AssignmentMismatch")
    if set(completeness) != set(manifest_rows):
        fail("schema-completeness/manifest exact-key mismatch", "LiveIdentityMismatch")
    if set(fixture_coverage) != set(manifest_rows):
        fail("fixture-coverage/manifest exact-key mismatch", "LiveIdentityMismatch")

    registry_evidence = {
        "operation_contracts": contracts_document,
        "assignments": assignments_document,
        "reachability": reachability_document,
        "fixture_coverage": fixture_document,
    }
    expected_registry_data = surface.generate_registry_data(manifest, registry_evidence)
    if arguments.registry.read_text(encoding="utf-8") != expected_registry_data:
        fail(
            "canonical registry is stale relative to the guarded evidence",
            "StaleCanonicalRegistry",
        )
    for key in sorted(manifest_rows):
        if (
            completeness[key].get("schema_fixture_facts")
            != fixture_coverage[key].get("completeness_evidence")
        ):
            fail(
                f"schema-completeness/fixture evidence mismatch: {key.compact()}",
                "CompletenessEvidenceMismatch",
            )

    association_errors = surface.association_diagnostics(manifest, contracts_document, registry_rows)
    if association_errors:
        codes = tuple(sorted(str(error["code"]) for error in association_errors))
        raise AuditError(
            f"operation-contract mismatch: {association_errors}",
            codes[0],
            codes,
        )
    assignment_errors = surface.assignment_reachability_diagnostics(
        manifest, assignments_document, reachability_document
    )
    if assignment_errors:
        codes = tuple(sorted(str(error["code"]) for error in assignment_errors))
        raise AuditError(
            f"assignment/reachability mismatch: {assignment_errors}",
            codes[0],
            codes,
        )

    return LiveInputs(
        manifest=manifest,
        assignments_document=assignments_document,
        reachability_document=reachability_document,
        contracts_document=contracts_document,
        completeness_document=completeness_document,
        fixture_document=fixture_document,
        registry_rows=registry_rows,
        registry=registry,
        manifest_rows=manifest_rows,
        assignments=assignments,
        reachability=reachability,
        contracts=contracts,
        completeness=completeness,
        fixture_coverage=fixture_coverage,
    )


def frozen_a1_keys(assignments: Mapping[Key, Mapping[str, Any]]) -> list[Key]:
    keys = sorted(key for key, row in assignments.items() if row.get("slice") == A1_1_SLICE)
    require(len(keys) == 151, f"A1.1 identity count changed: {len(keys)}")
    require(len(set(keys)) == 151, "A1.1 contains a duplicate identity")
    require(all(assignments[key]["stability"] == "stable" for key in keys), "A1.1 contains an unstable identity")
    return keys


def start_state_document(arguments: argparse.Namespace, base_sha: str) -> dict[str, Any]:
    inputs = load_live_inputs(arguments)
    a1_keys = frozen_a1_keys(inputs.assignments)

    schema_status = dict(
        sorted(Counter(inputs.registry[key]["typed_schema_status"] for key in a1_keys).items())
    )
    implementation_status = dict(
        sorted(Counter(inputs.registry[key]["typed_status"] for key in a1_keys).items())
    )
    global_status = dict(
        sorted(Counter(row["typed_schema_status"] for row in inputs.registry.values()).items())
    )
    require(
        schema_status == EXPECTED_SCHEMA_STATUS_COUNTS,
        f"start-state A1.1 schema status changed: {schema_status}",
    )
    require(
        implementation_status == {"Implemented": 26, "NotImplemented": 125},
        f"start-state A1.1 implementation status changed: {implementation_status}",
    )
    require(
        global_status == EXPECTED_GLOBAL_SCHEMA_STATUS,
        f"start-state global schema status changed: {global_status}",
    )

    capture_paths = {
        "assignments": arguments.assignments,
        "app_server_surface_generator": Path(surface.__file__).resolve(),
        "draft07_validator": arguments.draft07_validator,
        "fixture_coverage": arguments.fixture_coverage,
        "fixture_index": arguments.fixture_index,
        "manifest": arguments.manifest,
        "operation_contracts": arguments.contracts,
        "reachability": arguments.reachability,
        "registry": arguments.registry,
        "schema_completeness": arguments.schema_completeness,
        "stable_aggregate": arguments.schema_root / "stable/codex_app_server_protocol.schemas.json",
    }
    capture_sources = {
        name: {
            "path": path.resolve().relative_to(arguments.repo_root).as_posix(),
            "sha256": sha256_file(path),
        }
        for name, path in sorted(capture_paths.items())
    }
    identities = [
        {
            "protocol_surface_key": key.object(),
            "assignment": inputs.assignments[key],
            "registry": inputs.registry[key],
            "schema_completeness": inputs.completeness[key],
            "fixture_coverage": inputs.fixture_coverage[key],
        }
        for key in a1_keys
    ]
    a1_key_set = set(a1_keys)
    unrelated_registry = [
        {
            "protocol_surface_key": key.object(),
            "registry_projection": registry_projection(inputs.registry[key]),
        }
        for key in sorted(set(inputs.registry) - a1_key_set)
    ]
    require(
        len(unrelated_registry) == 236,
        f"unrelated start-state registry identity count changed: {len(unrelated_registry)}",
    )
    return {
        "generated_notice": (
            "Frozen by tools/codex/app_server_a1_1.py freeze-start-state; "
            "do not edit or regenerate after Commit 1."
        ),
        "format_version": FORMAT_VERSION,
        "codex_version": CODEX_VERSION,
        "authority_note": (
            "Immutable review evidence captured from the verified A1.0 base. "
            "This snapshot reproduces the A1.1 starting audit and is not a "
            "production disposition or dispatch authority."
        ),
        "base_sha": base_sha,
        "capture_sources": capture_sources,
        "counts": {
            "identity_count": len(a1_keys),
            "unrelated_registry_identity_count": len(unrelated_registry),
            "a1_1_schema_status": schema_status,
            "a1_1_implementation_status": implementation_status,
            "global_schema_status": global_status,
        },
        "identities": identities,
        "unrelated_registry_identities": unrelated_registry,
    }


def freeze_start_state(arguments: argparse.Namespace) -> None:
    if arguments.base_sha is None:
        fail(
            "freeze-start-state requires --base-sha",
            "MissingBaseSha",
        )
    if arguments.base_sha != EXPECTED_START_BASE_SHA:
        fail(
            "A1.1 start-state base SHA must be "
            f"{EXPECTED_START_BASE_SHA}, got {arguments.base_sha}",
            "BaseShaMismatch",
        )
    if arguments.start_state.exists() and not arguments.force_start_state:
        fail(
            f"refusing to overwrite frozen A1.1 start state: {arguments.start_state}; "
            "pass --force-start-state only during the reviewed Commit 1 freeze",
            "StartStateOverwriteRefused",
        )
    document = start_state_document(arguments, arguments.base_sha)
    arguments.start_state.parent.mkdir(parents=True, exist_ok=True)
    arguments.start_state.write_text(canonical_json(document), encoding="utf-8")


def start_state_by_key(document: Mapping[str, Any]) -> dict[Key, dict[str, Any]]:
    require(
        document.get("base_sha") == EXPECTED_START_BASE_SHA,
        "frozen A1.1 start-state base SHA changed",
    )
    identities = records(document, ("identities",), "A1.1 start state")
    result = indexed(identities, "A1.1 start-state")
    require(len(result) == 151, f"A1.1 start-state identity count changed: {len(result)}")
    counts = document.get("counts")
    require(isinstance(counts, dict), "A1.1 start state lacks counts")
    require(counts.get("identity_count") == 151, "A1.1 start-state denominator changed")
    require(
        counts.get("a1_1_schema_status") == EXPECTED_SCHEMA_STATUS_COUNTS,
        "A1.1 start-state schema-status split changed",
    )
    require(
        counts.get("a1_1_implementation_status")
        == {"Implemented": 26, "NotImplemented": 125},
        "A1.1 start-state implementation-status split changed",
    )
    require(
        counts.get("global_schema_status") == EXPECTED_GLOBAL_SCHEMA_STATUS,
        "A1.1 start-state global schema-status split changed",
    )
    for key, row in result.items():
        for field in (
            "assignment",
            "registry",
            "schema_completeness",
            "fixture_coverage",
        ):
            projection = row.get(field)
            require(
                isinstance(projection, dict)
                and Key.from_row(projection) == key,
                f"A1.1 start-state {field} identity drift: {key.compact()}",
            )
    derived_schema_status = dict(
        sorted(
            Counter(
                row["registry"]["typed_schema_status"] for row in result.values()
            ).items()
        )
    )
    derived_implementation_status = dict(
        sorted(
            Counter(row["registry"]["typed_status"] for row in result.values()).items()
        )
    )
    require(
        derived_schema_status == EXPECTED_SCHEMA_STATUS_COUNTS,
        f"A1.1 start-state identity schema statuses changed: {derived_schema_status}",
    )
    require(
        derived_implementation_status == {"Implemented": 26, "NotImplemented": 125},
        "A1.1 start-state identity implementation statuses changed: "
        f"{derived_implementation_status}",
    )
    return result


def unrelated_start_state_by_key(
    document: Mapping[str, Any],
) -> dict[Key, dict[str, Any]]:
    rows = records(
        document,
        ("unrelated_registry_identities",),
        "A1.1 unrelated registry start state",
    )
    result = indexed(rows, "A1.1 unrelated start-state registry")
    require(
        len(result) == 236,
        f"unrelated start-state registry identity count changed: {len(result)}",
    )
    counts = document.get("counts")
    require(
        isinstance(counts, dict)
        and counts.get("unrelated_registry_identity_count") == 236,
        "unrelated start-state registry denominator changed",
    )
    for key, row in result.items():
        projection = row.get("registry_projection")
        require(
            isinstance(projection, dict) and projection,
            f"unrelated start-state registry projection is malformed: {key.compact()}",
        )
    return result


def unrelated_live_diagnostics(
    a1_keys: Sequence[Key],
    baseline: Mapping[Key, Mapping[str, Any]],
    inputs: LiveInputs,
) -> list[AuditDiagnostic]:
    diagnostics: list[AuditDiagnostic] = []
    live_keys = set(inputs.registry) - set(a1_keys)
    if live_keys != set(baseline):
        diagnostics.append(
            AuditDiagnostic(
                "UnrelatedSliceIdentityDrift",
                "registry",
                "non-A1.1 registry exact-key set changed",
            )
        )
    for key in sorted(live_keys & set(baseline)):
        frozen_projection = baseline[key].get("registry_projection")
        if frozen_projection != registry_projection(inputs.registry[key]):
            diagnostics.append(
                AuditDiagnostic(
                    "UnrelatedSliceDrift",
                    key.compact(),
                    "non-A1.1 runtime, status, layer, completeness, or static field changed",
                )
            )
    return sorted(diagnostics)


def identity_has_advanced(
    key: Key,
    frozen: Mapping[str, Any],
    inputs: LiveInputs,
) -> bool:
    frozen_registry = frozen["registry"]
    live_registry = inputs.registry[key]
    schema_order = {"NotImplemented": 0, "Partial": 1, "Complete": 2}
    implementation_order = {"NotImplemented": 0, "Implemented": 1}
    layer_order = {"NotImplemented": 0, "Implemented": 1}
    if (
        implementation_order.get(live_registry.get("typed_status"), -1)
        > implementation_order.get(frozen_registry.get("typed_status"), -1)
        or schema_order.get(live_registry.get("typed_schema_status"), -1)
        > schema_order.get(frozen_registry.get("typed_schema_status"), -1)
    ):
        return True
    for field in ("backend_status", "canonical_state_status"):
        if (
            layer_order.get(live_registry.get(field), -1)
            > layer_order.get(frozen_registry.get(field), -1)
        ):
            return True
    frozen_facts = frozen_registry.get("schema_completeness", {})
    live_facts = live_registry.get("schema_completeness", {})
    if isinstance(frozen_facts, dict) and isinstance(live_facts, dict):
        if any(
            frozen_value is False and live_facts.get(field) is True
            for field, frozen_value in frozen_facts.items()
        ):
            return True
    frozen_fixture = frozen["fixture_coverage"]
    live_fixture = inputs.fixture_coverage[key]
    if set(live_fixture.get("fixture_ids", [])) > set(
        frozen_fixture.get("fixture_ids", [])
    ):
        return True
    return any(
        isinstance(live_fixture.get(field), int)
        and isinstance(frozen_fixture.get(field), int)
        and live_fixture[field] > frozen_fixture[field]
        for field in (
            "optional_omission_mutations",
            "positive_fixture_count",
            "required_field_mutations",
            "wrong_type_mutations",
        )
    )


def staged_progress_diagnostics(
    keys: Sequence[Key],
    baseline: Mapping[Key, Mapping[str, Any]],
    inputs: LiveInputs,
) -> list[AuditDiagnostic]:
    batch_order = ("B2", "B3", "B4", "B5")
    batch_keys: dict[str, list[Key]] = defaultdict(list)
    for key in keys:
        batch_keys[identity_batch(inputs.assignments[key], key)].append(key)
    diagnostics: list[AuditDiagnostic] = []
    for index, batch in enumerate(batch_order):
        if not any(identity_has_advanced(key, baseline[key], inputs) for key in batch_keys[batch]):
            continue
        incomplete_earlier = [
            key
            for earlier in batch_order[:index]
            for key in batch_keys[earlier]
            if inputs.registry[key].get("typed_schema_status") != "Complete"
        ]
        if incomplete_earlier:
            diagnostics.append(
                AuditDiagnostic(
                    "NonContiguousBatchProgress",
                    batch,
                    f"{batch} advanced before all earlier batch identities were Complete",
                )
            )
    return diagnostics


def live_progress_diagnostics(
    keys: Sequence[Key],
    baseline: Mapping[Key, Mapping[str, Any]],
    inputs: LiveInputs,
) -> list[AuditDiagnostic]:
    """Validate live Commit 2-6 progress without changing the frozen audit."""

    diagnostics: list[AuditDiagnostic] = []

    def add(code: str, key: Key, message: str) -> None:
        diagnostics.append(AuditDiagnostic(code, key.compact(), message))

    schema_order = {"NotImplemented": 0, "Partial": 1, "Complete": 2}
    implementation_order = {"NotImplemented": 0, "Implemented": 1}
    layer_order = {"NotImplemented": 0, "Implemented": 1}
    progress_fields = {
        "backend_status",
        "canonical_state_status",
        "runtime_disposition",
        "runtime_target",
        "schema_completeness",
        "typed_schema_status",
        "typed_status",
    }

    for key in keys:
        frozen = baseline[key]
        frozen_assignment = frozen.get("assignment")
        frozen_registry = frozen.get("registry")
        frozen_completeness = frozen.get("schema_completeness")
        frozen_fixture = frozen.get("fixture_coverage")
        if not all(
            isinstance(value, dict)
            for value in (
                frozen_assignment,
                frozen_registry,
                frozen_completeness,
                frozen_fixture,
            )
        ):
            add("StartStateIdentityDrift", key, "frozen identity projection is malformed")
            continue

        if frozen_assignment != inputs.assignments[key]:
            add("StaticIdentityDrift", key, "module/slice assignment changed from the frozen base")

        live_registry = inputs.registry[key]
        frozen_static = {
            field: value
            for field, value in frozen_registry.items()
            if field not in progress_fields
        }
        live_static = {
            field: value
            for field, value in live_registry.items()
            if field not in progress_fields
        }
        if frozen_static != live_static:
            add("StaticIdentityDrift", key, "static canonical-registry fields changed")

        frozen_implementation = frozen_registry.get("typed_status")
        live_implementation = live_registry.get("typed_status")
        if (
            frozen_implementation not in implementation_order
            or live_implementation not in implementation_order
            or implementation_order[live_implementation]
            < implementation_order[frozen_implementation]
        ):
            add("ImplementationDemotion", key, "typed implementation status regressed")

        frozen_schema = frozen_registry.get("typed_schema_status")
        live_schema = live_registry.get("typed_schema_status")
        if (
            frozen_schema not in schema_order
            or live_schema not in schema_order
            or schema_order[live_schema] < schema_order[frozen_schema]
        ):
            add("SchemaStatusDemotion", key, "typed schema status regressed")

        for field in ("backend_status", "canonical_state_status"):
            frozen_layer = frozen_registry.get(field)
            live_layer = live_registry.get(field)
            if frozen_layer == "NotApplicable":
                regressed = live_layer != "NotApplicable"
            else:
                regressed = (
                    frozen_layer not in layer_order
                    or live_layer not in layer_order
                    or layer_order[live_layer] < layer_order[frozen_layer]
                )
            if regressed:
                add("LayerDispositionDemotion", key, f"{field} regressed")

        if live_implementation == frozen_implementation:
            if (
                live_registry.get("runtime_disposition")
                != frozen_registry.get("runtime_disposition")
                or live_registry.get("runtime_target")
                != frozen_registry.get("runtime_target")
            ):
                add(
                    "StaticIdentityDrift",
                    key,
                    "runtime target/disposition changed without implementation advancement",
                )
        elif live_implementation == "Implemented":
            if (
                live_registry.get("runtime_disposition") != "Typed"
                or live_registry.get("runtime_target") in {"", "std::monostate{}"}
            ):
                add(
                    "InvalidRuntimePromotion",
                    key,
                    "implemented identity lacks a nonempty Typed runtime target",
                )

        frozen_schema_facts = frozen_registry.get("schema_completeness")
        live_schema_facts = live_registry.get("schema_completeness")
        if not isinstance(frozen_schema_facts, dict) or not isinstance(live_schema_facts, dict):
            add("CompletenessDemotion", key, "schema-completeness projection is malformed")
        else:
            for field, frozen_value in frozen_schema_facts.items():
                live_value = live_schema_facts.get(field)
                if frozen_value is True and live_value is not True:
                    add("CompletenessDemotion", key, f"schema-completeness fact {field} regressed")
                    break
                if not isinstance(live_value, bool):
                    add("CompletenessDemotion", key, f"schema-completeness fact {field} is not boolean")
                    break

        live_completeness = inputs.completeness[key]
        frozen_fixture_ids = set(frozen_completeness.get("fixture_ids", []))
        live_fixture_ids = set(live_completeness.get("fixture_ids", []))
        if not frozen_fixture_ids.issubset(live_fixture_ids):
            add("FixtureCoverageDemotion", key, "schema-completeness fixture IDs regressed")

        live_fixture = inputs.fixture_coverage[key]
        frozen_coverage_ids = set(frozen_fixture.get("fixture_ids", []))
        live_coverage_ids = set(live_fixture.get("fixture_ids", []))
        if not frozen_coverage_ids.issubset(live_coverage_ids):
            add("FixtureCoverageDemotion", key, "fixture-coverage IDs regressed")
        for field in (
            "positive_fixture_count",
            "required_field_mutations",
            "wrong_type_mutations",
            "optional_omission_mutations",
        ):
            frozen_count = frozen_fixture.get(field, 0)
            live_count = live_fixture.get(field, 0)
            if (
                not isinstance(frozen_count, int)
                or not isinstance(live_count, int)
                or live_count < frozen_count
            ):
                add("FixtureCoverageDemotion", key, f"fixture metric {field} regressed")
                break

    diagnostics.extend(staged_progress_diagnostics(keys, baseline, inputs))
    return sorted(set(diagnostics))


def validate_live_progress(
    keys: Sequence[Key],
    baseline: Mapping[Key, Mapping[str, Any]],
    inputs: LiveInputs,
    unrelated_baseline: Mapping[Key, Mapping[str, Any]] | None = None,
) -> None:
    diagnostics = live_progress_diagnostics(keys, baseline, inputs)
    if unrelated_baseline is not None:
        diagnostics.extend(
            unrelated_live_diagnostics(keys, unrelated_baseline, inputs)
        )
        diagnostics.sort()
    if diagnostics:
        codes = tuple(diagnostic.code for diagnostic in diagnostics)
        message = "; ".join(
            f"{diagnostic.code} at {diagnostic.location}: {diagnostic.message}"
            for diagnostic in diagnostics
        )
        raise AuditError(message, diagnostics[0].code, codes)


def definition_directionalities(
    closure: set[str],
    seed_closures: Mapping[str, set[str]],
    seeds: Mapping[str, Mapping[str, Any]],
) -> dict[str, str]:
    encode_roots = {
        name
        for name, seed in seeds.items()
        if seed.get("role") == "client_request_params"
    }
    decode_roots = {
        name
        for name, seed in seeds.items()
        if seed.get("role")
        in {
            "client_request_result",
            "item_union",
            "server_notification_params",
        }
    }
    encode_closure = set().union(
        *(seed_closures[name] for name in sorted(encode_roots))
    )
    decode_closure = set().union(
        *(seed_closures[name] for name in sorted(decode_roots))
    )
    require(
        encode_closure | decode_closure == closure,
        "registered-union seed introduced a definition with no real "
        "encode/decode root",
        "DirectionalityMismatch",
    )
    result = {
        name: (
            "Both"
            if name in encode_closure and name in decode_closure
            else "EncodeOnly"
            if name in encode_closure
            else "DecodeOnly"
        )
        for name in sorted(closure)
    }
    counts = dict(sorted(Counter(result.values()).items()))
    require(
        counts == EXPECTED_DEFINITION_DIRECTIONS,
        f"definition directionality changed: {counts}",
        "DirectionalityMismatch",
    )
    return result


def stable_slice_definition_roots(
    slice_name: str,
    typed_definitions: Mapping[str, Mapping[str, Any]],
    assignments: Mapping[Key, Mapping[str, Any]],
    manifest_rows: Mapping[Key, Mapping[str, Any]],
    contracts: Mapping[Key, Mapping[str, Any]],
) -> set[str]:
    roots: set[str] = set()
    for key, assignment in assignments.items():
        if (
            assignment.get("slice") != slice_name
            or assignment.get("stability") != "stable"
        ):
            continue
        candidates: list[Any] = []
        if key.category in {"client_request", "server_request"}:
            contract = contracts.get(key)
            if contract is not None:
                candidates.extend(
                    (
                        contract.get("parameter_type_identity"),
                        contract.get("result_schema_type_identity"),
                    )
                )
        elif key.category in {"client_notification", "server_notification"}:
            params = manifest_rows[key].get("params")
            if isinstance(params, Mapping):
                candidates.append(params.get("type"))
        elif key.category in {
            "delta_progress_discriminator",
            "item_discriminator",
            "tagged_union_discriminator",
        }:
            candidates.append(key.domain)
        roots.update(
            candidate
            for candidate in candidates
            if isinstance(candidate, str) and candidate in typed_definitions
        )
    return roots


def cross_slice_definition_ownership(
    closure: set[str],
    edges: Mapping[str, set[str]],
    typed_definitions: Mapping[str, Mapping[str, Any]],
    assignments: Mapping[Key, Mapping[str, Any]],
    manifest_rows: Mapping[Key, Mapping[str, Any]],
    contracts: Mapping[Key, Mapping[str, Any]],
) -> tuple[dict[str, str], dict[str, list[str]]]:
    later_reachability: dict[str, set[str]] = {}
    for slice_name in sorted(FORWARD_SLICE_NAMES):
        roots = stable_slice_definition_roots(
            slice_name,
            typed_definitions,
            assignments,
            manifest_rows,
            contracts,
        )
        require(roots, f"{slice_name} has no stable schema roots")
        later_reachability[slice_name] = set().union(
            *(transitive_closure((root,), edges) for root in sorted(roots))
        )

    ownership: dict[str, str] = {}
    reused_by: dict[str, list[str]] = {}
    for name in sorted(closure):
        later_slices = sorted(
            slice_name
            for slice_name, reached in later_reachability.items()
            if name in reached
        )
        reused_by[name] = later_slices
        if name in PRIOR_SLICE_REUSED_DEFINITIONS:
            ownership[name] = "PriorSliceReuse"
        elif later_slices:
            ownership[name] = "A11ForwardShared"
        else:
            ownership[name] = "A11Local"
    counts = dict(sorted(Counter(ownership.values()).items()))
    require(
        counts == EXPECTED_CROSS_SLICE_OWNERSHIP,
        f"cross-slice definition ownership changed: {counts}",
        "CrossSliceOwnershipMismatch",
    )
    actual_prior = {
        name for name, value in ownership.items() if value == "PriorSliceReuse"
    }
    require(
        actual_prior == PRIOR_SLICE_REUSED_DEFINITIONS,
        f"prior-slice reuse set changed: {sorted(actual_prior)}",
        "CrossSliceOwnershipMismatch",
    )
    return ownership, reused_by


def build_reports(arguments: argparse.Namespace) -> tuple[dict[str, Any], dict[str, Any]]:
    inputs = load_live_inputs(arguments)
    manifest = inputs.manifest
    assignments_document = inputs.assignments_document
    reachability_document = inputs.reachability_document
    contracts_document = inputs.contracts_document
    registry = inputs.registry
    manifest_rows = inputs.manifest_rows
    assignments = inputs.assignments
    reachability = inputs.reachability
    contracts = inputs.contracts
    completeness = inputs.completeness
    fixture_coverage = inputs.fixture_coverage

    start_state = load_json(arguments.start_state)
    validate_input_versions((start_state,))
    baseline = start_state_by_key(start_state)
    unrelated_baseline = unrelated_start_state_by_key(start_state)
    a1_keys = frozen_a1_keys(assignments)
    fixture_planner = FixturePlanner(arguments)
    reviewed_branch_titles = validate_reviewed_public_mappings(
        a1_keys, manifest_rows, fixture_planner.catalog
    )
    require(
        set(baseline) == set(a1_keys),
        "frozen A1.1 start-state/live assignment exact-key mismatch",
    )
    require(
        not (set(baseline) & set(unrelated_baseline))
        and set(baseline) | set(unrelated_baseline) == set(registry),
        "frozen start-state global registry partition mismatch",
    )
    validate_live_progress(a1_keys, baseline, inputs, unrelated_baseline)
    partial_notification_names = {
        key.name
        for key in a1_keys
        if key.category == "server_notification"
        and baseline[key]["registry"]["typed_schema_status"] == "Partial"
    }
    require(
        set(SERVER_NOTIFICATION_SEMANTIC_ADAPTERS)
        == partial_notification_names,
        "reviewed existing notification semantic-adapter set changed: "
        f"{sorted(partial_notification_names)}",
        "ReviewedPublicMappingMismatch",
    )
    residual_partial_keys = {
        (key.category, key.domain, key.field, key.name)
        for key, row in unrelated_baseline.items()
        if row["registry_projection"].get("typed_schema_status") == "Partial"
    }
    require(
        residual_partial_keys == EXPECTED_RESIDUAL_PARTIAL_KEYS,
        f"expected residual partial identities changed: {sorted(residual_partial_keys)}",
    )

    category_counts = Counter(key.category for key in a1_keys)
    actual_categories = {
        "client_request": category_counts["client_request"],
        "server_notification": category_counts["server_notification"],
        "thread_item": sum(key.category == "item_discriminator" and key.domain == "ThreadItem" for key in a1_keys),
        "response_item": sum(key.category == "item_discriminator" and key.domain == "ResponseItem" for key in a1_keys),
        "tagged_union_discriminator": category_counts["tagged_union_discriminator"],
    }
    require(actual_categories == EXPECTED_CATEGORY_COUNTS, f"A1.1 taxonomy changed: {actual_categories}")
    require(category_counts["client_notification"] == 0, "A1.1 unexpectedly contains a client notification")
    require(category_counts["server_request"] == 0, "A1.1 unexpectedly contains a server request")

    classifications = Counter(assignments[key]["classification"] for key in a1_keys)
    modules = Counter(assignments[key]["module"] for key in a1_keys)
    require(dict(sorted(classifications.items())) == EXPECTED_CLASSIFICATION_COUNTS, f"classification counts changed: {classifications}")
    require(dict(sorted(modules.items())) == EXPECTED_MODULE_COUNTS, f"module counts changed: {modules}")

    schema_status = Counter(
        baseline[key]["registry"]["typed_schema_status"] for key in a1_keys
    )
    require(dict(sorted(schema_status.items())) == EXPECTED_SCHEMA_STATUS_COUNTS, f"current A1.1 status changed: {schema_status}")
    require(
        Counter(baseline[key]["registry"]["typed_status"] for key in a1_keys)
        == Counter({"Implemented": 26, "NotImplemented": 125}),
        "current A1.1 implementation-status split changed",
    )
    global_status = dict(start_state["counts"]["global_schema_status"])
    require(global_status == EXPECTED_GLOBAL_SCHEMA_STATUS, f"global schema status changed: {global_status}")

    request_keys = [key for key in a1_keys if key.category == "client_request"]
    require(len(request_keys) == 22, "A1.1 successful-result obligation count is not 22")
    require(set(OPERATION_API_NAMES) == {key.name for key in request_keys}, "reviewed A1.1 operation API mapping is stale")
    request_contracts: dict[Key, dict[str, Any]] = {}
    for key in request_keys:
        contract = contracts.get(key)
        require(contract is not None, f"A1.1 request lacks an authoritative contract: {key.compact()}")
        request_contracts[key] = contract
        row = registry[key]
        require(row["parameter_type_identity"] == contract["parameter_type_identity"], f"request parameter mismatch: {key.compact()}")
        require(row["result_type_identity"] == contract["result_type_identity"], f"request result mismatch: {key.compact()}")
        require(row["result_contract_kind"] == contract["result_contract_kind"], f"request result-kind mismatch: {key.compact()}")
    result_kinds = Counter(contract["result_contract_kind"] for contract in request_contracts.values())
    require(dict(sorted(result_kinds.items())) == EXPECTED_RESULT_KIND_COUNTS, f"A1.1 result kinds changed: {result_kinds}")
    unit_methods = {key.name for key, contract in request_contracts.items() if contract["result_contract_kind"] == "Unit"}
    require(unit_methods == EXPECTED_UNIT_METHODS, f"A1.1 Unit method set changed: {sorted(unit_methods)}")

    global UNIT_RESULT_SCHEMA_NAMES
    UNIT_RESULT_SCHEMA_NAMES = {
        contract["result_schema_type_identity"]
        for contract in request_contracts.values()
        if contract["result_contract_kind"] == "Unit"
    }

    identity_batches = {key: identity_batch(assignments[key], key) for key in a1_keys}
    identity_batch_counts = dict(sorted(Counter(identity_batches.values()).items()))
    require(identity_batch_counts == EXPECTED_BATCH_IDENTITY_COUNTS, f"identity batch counts changed: {identity_batch_counts}")

    aggregate_path = arguments.schema_root / "stable/codex_app_server_protocol.schemas.json"
    aggregate = load_json(aggregate_path)
    all_definitions = aggregate.get("definitions")
    require(isinstance(all_definitions, dict), "stable aggregate lacks definitions")
    validate_stable_method_unions(all_definitions, manifest_rows, contracts)
    v2_definitions = all_definitions.get("v2")
    require(isinstance(v2_definitions, dict), "stable aggregate lacks definitions.v2")
    require(all(isinstance(schema, dict) for schema in v2_definitions.values()), "v2 definition is not an object")
    typed_definitions: dict[str, dict[str, Any]] = {
        str(name): dict(schema) for name, schema in v2_definitions.items()
    }
    edges = {name: schema_references(schema) for name, schema in typed_definitions.items()}
    unknown_edges = {
        name: sorted(dependencies - set(typed_definitions))
        for name, dependencies in edges.items()
        if dependencies - set(typed_definitions)
    }
    require(not unknown_edges, f"v2 schema graph contains unresolved references: {unknown_edges}")

    seeds: dict[str, dict[str, Any]] = {}

    def add_seed(name: str, role: str, batch: str, owners: list[dict[str, Any]]) -> None:
        require(name in typed_definitions, f"schema seed has no v2 definition: {name}")
        require(name not in seeds, f"duplicate schema-closure seed: {name}")
        seeds[name] = {"definition": name, "role": role, "batch": batch, "owning_surfaces": owners}

    for key in request_keys:
        contract = request_contracts[key]
        add_seed(contract["parameter_type_identity"], "client_request_params", "B4", [key.object()])
        add_seed(contract["result_schema_type_identity"], "client_request_result", "B4", [key.object()])
    for key in (key for key in a1_keys if key.category == "server_notification"):
        entry = manifest_rows[key]
        type_identity = entry.get("params", {}).get("type")
        require(isinstance(type_identity, str) and type_identity, f"notification lacks params type: {key.compact()}")
        add_seed(type_identity, "server_notification_params", "B5", [key.object()])
    for domain in ("ResponseItem", "ThreadItem"):
        owners = [key.object() for key in a1_keys if key.category == "item_discriminator" and key.domain == domain]
        add_seed(domain, "item_union", "B3", owners)
    nested_domains = sorted({key.domain for key in a1_keys if key.category == "tagged_union_discriminator"})
    require(len(nested_domains) == 17, f"nested-union family count changed: {len(nested_domains)}")
    for domain in nested_domains:
        member_keys = [key for key in a1_keys if key.category == "tagged_union_discriminator" and key.domain == domain]
        batches = {identity_batches[key] for key in member_keys}
        require(len(batches) == 1, f"nested-union family is split across batches: {domain}")
        add_seed(domain, "registered_tagged_union_family", next(iter(batches)), [key.object() for key in member_keys])

    require(len(seeds) == 100, f"A1.1 schema seed count changed: {len(seeds)}")
    seed_role_counts = dict(sorted(Counter(seed["role"] for seed in seeds.values()).items()))
    require(
        seed_role_counts
        == {
            "client_request_params": 22,
            "client_request_result": 22,
            "item_union": 2,
            "registered_tagged_union_family": 17,
            "server_notification_params": 37,
        },
        f"A1.1 schema seed taxonomy changed: {seed_role_counts}",
    )

    seed_closures = {name: transitive_closure((name,), edges) for name in seeds}
    closure = set().union(*seed_closures.values())
    require(len(closure) == 164, f"A1.1 named-definition closure changed: {len(closure)}")
    validate_closure_references(typed_definitions, closure)
    cycle = find_cycle(closure, edges)
    require(cycle is None, f"A1.1 schema dependency cycle: {' -> '.join(cycle or [])}")
    definition_directions = definition_directionalities(
        closure, seed_closures, seeds
    )
    cross_slice_ownership, reused_by_later_slices = (
        cross_slice_definition_ownership(
            closure,
            edges,
            typed_definitions,
            assignments,
            manifest_rows,
            contracts,
        )
    )

    batch_order = {"B2": 2, "B3": 3, "B4": 4, "B5": 5}
    definition_batches: dict[str, str] = {}
    reaching_seeds: dict[str, list[str]] = {}
    for definition in sorted(closure):
        reached_by = sorted(seed for seed, reached in seed_closures.items() if definition in reached)
        require(reached_by, f"closure definition has no seed: {definition}")
        reaching_seeds[definition] = reached_by
        definition_batches[definition] = min(
            (seeds[seed]["batch"] for seed in reached_by),
            key=batch_order.__getitem__,
        )
    definition_batch_counts = dict(sorted(Counter(definition_batches.values()).items()))
    require(
        definition_batch_counts == EXPECTED_BATCH_DEFINITION_COUNTS,
        f"definition batch counts changed: {definition_batch_counts}",
    )

    batch_dependencies: dict[str, set[str]] = {batch: set() for batch in batch_order}
    for definition in closure:
        owner_batch = definition_batches[definition]
        for dependency in edges[definition] & closure:
            dependency_batch = definition_batches[dependency]
            require(
                batch_order[dependency_batch] <= batch_order[owner_batch],
                f"batch dependency points forward: {definition} ({owner_batch}) -> {dependency} ({dependency_batch})",
            )
            if dependency_batch != owner_batch:
                batch_dependencies[owner_batch].add(dependency_batch)
    for batch, dependencies in batch_dependencies.items():
        require(batch not in dependencies, f"batch dependency self-cycle: {batch}")
        require(
            all(batch_order[dependency] < batch_order[batch] for dependency in dependencies),
            f"batch dependency cycle/order violation: {batch} -> {sorted(dependencies)}",
        )

    definition_records: list[dict[str, Any]] = []
    shape_counts: Counter[str] = Counter()
    disposition_counts: Counter[str] = Counter()
    for name in sorted(closure):
        schema = typed_definitions[name]
        shape = schema_shape(schema)
        shape_counts[shape] += 1
        disposition, cpp_owner = definition_disposition(name, schema, shape)
        disposition_counts[disposition] += 1
        definition_record: dict[str, Any] = {
            "definition_key": {"namespace": "v2", "name": name},
            "schema_path": f"#/definitions/v2/{name}",
            "schema_shape": shape,
            "disposition": disposition,
            "cpp_owner": cpp_owner,
            "directionality": definition_directions[name],
            "cross_slice_ownership": cross_slice_ownership[name],
            "reused_by_later_slices": reused_by_later_slices[name],
            "owning_implementation_batch": definition_batches[name],
            "direct_dependencies": sorted(edges[name] & closure),
            "reaching_seeds": reaching_seeds[name],
        }
        if shape == "open_string_enum":
            values = schema.get("enum")
            require(
                isinstance(values, list)
                and values
                and all(isinstance(value, str) for value in values),
                f"open-string enum values are malformed: {name}",
                "MalformedFixturePlan",
            )
            definition_record["fixture_plan"] = {
                "known_value_case_ids": [
                    f"enum:{name}:{value}" for value in values
                ],
                "future_value_case_id": f"enum:{name}:future-unknown",
                "future_value_typed_diagnostic": {
                    "kind": "UnknownEnumValue",
                    "severity": "ForwardCompatibility",
                },
            }
        definition_records.append(definition_record)
    require(dict(sorted(shape_counts.items())) == EXPECTED_DEFINITION_SHAPES, f"definition shapes changed: {shape_counts}")
    open_enum_known_value_cases = sum(
        len(record["fixture_plan"]["known_value_case_ids"])
        for record in definition_records
        if "fixture_plan" in record
    )
    require(
        open_enum_known_value_cases == 95,
        f"reviewed open-enum known-value fixture count changed: "
        f"{open_enum_known_value_cases}",
        "MalformedFixturePlan",
    )

    (
        path_records,
        property_counts,
        opaque_paths,
        presence_counts,
        strong_identifier_paths,
    ) = collect_schema_paths(
        typed_definitions, closure, definition_directions
    )
    require(
        property_counts
        == {
            "optional_nonnullable": 19,
            "optional_nullable": 218,
            "required_nonnullable": 413,
            "required_nullable": 5,
        },
        f"A1.1 property-shape counts changed: {property_counts}",
    )
    path_direction_counts = dict(
        sorted(Counter(row["directionality"] for row in path_records).items())
    )
    require(
        path_direction_counts == EXPECTED_SCHEMA_PATH_DIRECTIONS,
        f"schema-path directionality changed: {path_direction_counts}",
        "DirectionalityMismatch",
    )
    path_kind_direction_counts = {
        kind: {
            direction: sum(
                row["schema_node_kind"] == kind
                and row["directionality"] == direction
                for row in path_records
            )
            for direction in sorted(VALID_DIRECTIONALITIES)
        }
        for kind in ("array_element", "map_value", "property")
    }
    require(
        path_kind_direction_counts == EXPECTED_SCHEMA_PATH_KIND_DIRECTIONS,
        "schema-path kind/direction accounting changed: "
        f"{path_kind_direction_counts}",
        "DirectionalityMismatch",
    )

    input_paths = {
        "app_server_surface_generator": Path(surface.__file__).resolve(),
        "draft07_validator": arguments.draft07_validator,
        "fixture_generator": Path(fixture_tool.__file__).resolve(),
        "fixture_index": arguments.fixture_index,
        "manifest": arguments.manifest,
        "assignments": arguments.assignments,
        "reachability": arguments.reachability,
        "operation_contracts": arguments.contracts,
        "stable_aggregate": aggregate_path,
        "start_state": arguments.start_state,
        "generator": Path(__file__).resolve(),
    }
    sources = {
        name: {"path": path.resolve().relative_to(arguments.repo_root).as_posix(), "sha256": sha256_file(path)}
        for name, path in sorted(input_paths.items())
    }
    live_progress_inputs = {
        name: {
            "path": path.resolve().relative_to(arguments.repo_root).as_posix(),
            "hash_policy": (
                "intentionally unhashed mutable progress input; validated against "
                "the immutable A1.1 start state"
            ),
        }
        for name, path in sorted(
            {
                "fixture_coverage": arguments.fixture_coverage,
                "registry": arguments.registry,
                "schema_completeness": arguments.schema_completeness,
            }.items()
        )
    }

    open_enum_names = {
        record["definition_key"]["name"]
        for record in definition_records
        if record["schema_shape"] == "open_string_enum"
    }
    require(
        len(open_enum_names) == 27,
        f"reviewed open-string enum definition count changed: "
        f"{len(open_enum_names)}",
        "MalformedFixturePlan",
    )

    plan_identities: list[dict[str, Any]] = []
    for key in a1_keys:
        registry_row = baseline[key]["registry"]
        assignment = assignments[key]
        contract = request_contracts.get(key)
        directionality = identity_directionality(
            key, definition_directions
        )
        require(
            directionality == reviewed_identity_directionality(key),
            f"derived/reviewed identity direction differs: {key.compact()}",
            "DirectionalityMismatch",
        )
        # Fixture requirements are a live monotonic implementation input:
        # Commit 1's frozen row remains the no-regression floor, while newly
        # generated indexed fixtures replace planned obligations in later
        # staged batches.
        fixture_row = inputs.fixture_coverage[key]
        positive, negative = fixture_planner.plan(
            key,
            list(fixture_row.get("fixture_ids", [])),
            open_enum_names,
        )
        row: dict[str, Any] = {
            "protocol_surface_key": key.object(),
            "category": key.category,
            "domain": key.domain,
            "discriminator_field": key.field,
            "directionality": directionality,
            "stability": assignment["stability"],
            "deprecated": bool(registry_row["deprecated"]),
            "classification": assignment["classification"],
            "module": assignment["module"],
            "reaching_stable_roots": stable_roots_for(key, reachability),
            "current_runtime_disposition": registry_row["runtime_disposition"],
            "current_runtime_target": registry_row["runtime_target"],
            "current_implementation_status": registry_row["typed_status"],
            "current_schema_status": registry_row["typed_schema_status"],
            "current_schema_completeness": registry_row["schema_completeness"],
            "current_fixture_ids": list(fixture_row.get("fixture_ids", [])),
            "owning_implementation_batch": identity_batches[key],
            "required_public_type_or_facade": public_requirement(
                key, contract, reviewed_branch_titles.get(key)
            ),
            "required_private_codec_or_descriptor": private_requirement(
                key, directionality
            ),
            "required_positive_fixtures": positive,
            "required_negative_fixtures": negative,
            "expected_backend_core_handling": backend_behavior(key, registry_row),
            "expected_frontend_behavior": frontend_behavior(key, registry_row),
            "source_compatibility_consideration": compatibility_note(key, registry_row),
        }
        if contract is not None:
            facade, method = OPERATION_API_NAMES[key.name]
            row["request_contract"] = {
                "authoritative_params_type": contract["parameter_type_identity"],
                "authoritative_successful_result_type": contract["result_type_identity"],
                "authoritative_result_schema_type": contract["result_schema_type_identity"],
                "result_contract_kind": contract["result_contract_kind"],
                "current_typed_method": CURRENT_TYPED_METHODS.get(key.name),
                "proposed_public_method_name": f"{facade}::{method}",
                "old_overload_or_alias_must_remain": key.name in CURRENT_TYPED_METHODS,
                "deprecated_typed_api": key.name == "thread/rollback",
            }
        plan_identities.append(row)

    identity_direction_counts = dict(
        sorted(
            Counter(
                row["directionality"] for row in plan_identities
            ).items()
        )
    )
    require(
        identity_direction_counts == EXPECTED_IDENTITY_DIRECTIONS,
        f"identity directionality changed: {identity_direction_counts}",
        "DirectionalityMismatch",
    )

    fixture_plan_counts: Counter[str] = Counter()
    fixture_plan_counts["required_positive_fixture_roles"] = sum(
        len(row["required_positive_fixtures"]["required_fixture_ids"])
        for row in plan_identities
    )
    fixture_plan_counts["planned_positive_fixtures"] = sum(
        len(row["required_positive_fixtures"]["planned_fixture_ids"])
        for row in plan_identities
    )
    fixture_plan_counts["current_a1_1_positive_records"] = sum(
        len(row["required_positive_fixtures"]["existing_fixture_ids"])
        for row in plan_identities
    )
    for row in plan_identities:
        mutation_groups = row["required_negative_fixtures"][
            "schema_mutations"
        ]
        for group, cases in mutation_groups.items():
            fixture_plan_counts[f"{group}_cases"] += len(cases)
            fixture_plan_counts[f"{group}_applicable"] += sum(
                case.get("applicability") != "not_applicable"
                for case in cases
            )
            fixture_plan_counts[f"{group}_not_applicable"] += sum(
                case.get("applicability") == "not_applicable"
                for case in cases
            )
    require(
        fixture_plan_counts["required_positive_fixture_roles"] == 173,
        "A1.1 canonical positive fixture-role count changed",
        "MalformedFixturePlan",
    )
    # Commit 1 froze 83 planned plus 104 current positive associations. As
    # implementation batches land, a planned association becomes current; the
    # total obligation set must remain exact while progress is monotonic.
    require(
        fixture_plan_counts["planned_positive_fixtures"] <= 83,
        "A1.1 planned positive fixture count regressed above the frozen base",
        "MalformedFixturePlan",
    )
    require(
        fixture_plan_counts["current_a1_1_positive_records"] >= 104,
        "A1.1 current positive fixture association count regressed below the frozen base",
        "MalformedFixturePlan",
    )
    for row in plan_identities:
        if (
            row["owning_implementation_batch"] == "B2"
            and row["current_schema_status"] == "Complete"
        ):
            require(
                not row["required_positive_fixtures"]["planned_fixture_ids"],
                "a Complete B2 identity still depends on a planned positive fixture",
                "MalformedFixturePlan",
            )
    discriminator_representation_counts = dict(
        sorted(
            Counter(
                row["required_private_codec_or_descriptor"][
                    "discriminator_behavior"
                ]["representation"]
                for row in plan_identities
            ).items()
        )
    )
    require(
        discriminator_representation_counts
        == {
            "externally_tagged_object_key": 5,
            "json_rpc_method": 59,
            "object_discriminator_property": 76,
            "scalar_union_literal": 11,
        },
        "discriminator representation accounting changed: "
        f"{discriminator_representation_counts}",
        "DiscriminatorLiteralMismatch",
    )
    discriminator_mapping_sha256 = sha256_json(
        [
            discriminator_mapping_projection(row)
            for row in plan_identities
        ]
    )

    batch_subjects = {
        "B2": "Add Codex conversation union and value foundations",
        "B3": "Complete Codex ThreadItem and ResponseItem models",
        "B4": "Complete Codex thread and turn operations",
        "B5": "Complete Codex A1.1 notifications and preservation",
        "B6": "Close and verify the Codex A1.1 slice",
    }
    batches = [
        {
            "batch": batch,
            "commit_subject": batch_subjects[batch],
            "identity_count": EXPECTED_BATCH_IDENTITY_COUNTS.get(batch, 0),
            "definition_count": EXPECTED_BATCH_DEFINITION_COUNTS.get(batch, 0),
            "depends_on": sorted(batch_dependencies.get(batch, set()))
            if batch != "B6"
            else ["B2", "B3", "B4", "B5"],
            "runtime_implementation_allowed": batch != "B6",
        }
        for batch in ("B2", "B3", "B4", "B5", "B6")
    ]
    staged_ratchets: list[dict[str, Any]] = []
    cumulative_keys: list[Key] = []
    for batch in ("B2", "B3", "B4", "B5"):
        owned_keys = sorted(
            key for key in a1_keys if identity_batches[key] == batch
        )
        cumulative_keys = sorted(cumulative_keys + owned_keys)
        staged_ratchets.append(
            {
                "batch": batch,
                "owned_identity_count": len(owned_keys),
                "cumulative_identity_count": len(cumulative_keys),
                "owned_identities": [key.object() for key in owned_keys],
                "cumulative_identities": [key.object() for key in cumulative_keys],
            }
        )

    final_status = dict(global_status)
    final_status["Complete"] += len(a1_keys)
    final_status["Partial"] -= EXPECTED_SCHEMA_STATUS_COUNTS["Partial"]
    final_status["NotImplemented"] -= EXPECTED_SCHEMA_STATUS_COUNTS["NotImplemented"]
    require(final_status == EXPECTED_FINAL_GLOBAL_SCHEMA_STATUS, f"derived final global metrics changed: {final_status}")

    plan = {
        "generated_notice": "Generated by tools/codex/app_server_a1_1.py; do not edit.",
        "format_version": FORMAT_VERSION,
        "codex_version": CODEX_VERSION,
        "authority_note": (
            "Review and batching evidence only. Starting runtime disposition, runtime target, implementation "
            "status, schema status, and completeness are copied from the canonical production registry into "
            "the immutable verified-base snapshot; neither this report nor that snapshot is a production input."
        ),
        "sources": sources,
        "live_progress_inputs": live_progress_inputs,
        "counts": {
            "identity_denominator": 151,
            "successful_result_root_obligations_not_identities": 22,
            "taxonomy": EXPECTED_CATEGORY_COUNTS,
            "classifications": EXPECTED_CLASSIFICATION_COUNTS,
            "modules": EXPECTED_MODULE_COUNTS,
            "current_a1_1_schema_status": EXPECTED_SCHEMA_STATUS_COUNTS,
            "current_global_schema_status": EXPECTED_GLOBAL_SCHEMA_STATUS,
            "derived_final_global_schema_status": final_status,
            "result_contract_kinds": EXPECTED_RESULT_KIND_COUNTS,
            "batch_identities": EXPECTED_BATCH_IDENTITY_COUNTS,
            "batch_definitions": EXPECTED_BATCH_DEFINITION_COUNTS,
            "identity_directions": identity_direction_counts,
            "discriminator_representations": (
                discriminator_representation_counts
            ),
            "discriminator_mapping_sha256": (
                discriminator_mapping_sha256
            ),
            "fixture_plan": dict(sorted(fixture_plan_counts.items())),
            "open_enum_known_value_cases": open_enum_known_value_cases,
            "open_enum_future_value_cases": len(open_enum_names),
        },
        "taxonomy_rule": (
            "The 22 paired successful result roots are implementation obligations for the 22 client-request "
            "identities; they are not added to the 151 ProtocolSurfaceKey identity denominator."
        ),
        "unit_result_methods": sorted(EXPECTED_UNIT_METHODS),
        "batches": batches,
        "staged_exact_a1_1_ratchets": staged_ratchets,
        "expected_residual_partial_identities": [
            tuple_key_object(key) for key in sorted(EXPECTED_RESIDUAL_PARTIAL_KEYS)
        ],
        "final_exact_a1_1_ratchet": [key.object() for key in a1_keys],
        "identities": plan_identities,
    }

    definition_direction_counts = dict(
        sorted(Counter(definition_directions.values()).items())
    )
    cross_slice_ownership_counts = dict(
        sorted(Counter(cross_slice_ownership.values()).items())
    )
    schema_path_mapping_sha256 = sha256_json(
        [
            schema_path_mapping_projection(row)
            for row in path_records
        ]
    )
    definition_direction_ownership_sha256 = sha256_json(
        [
            definition_direction_ownership_projection(row)
            for row in definition_records
        ]
    )
    definition_cpp_mapping_sha256 = sha256_json(
        [
            definition_cpp_mapping_projection(row)
            for row in definition_records
        ]
    )
    closure_report = {
        "generated_notice": "Generated by tools/codex/app_server_a1_1.py; do not edit.",
        "format_version": FORMAT_VERSION,
        "codex_version": CODEX_VERSION,
        "authority_note": (
            "Transitive stable v2 schema-definition closure and reviewed C++ ownership dispositions. "
            "This report is not a runtime registry or dispatch table."
        ),
        "sources": sources,
        "live_progress_inputs": live_progress_inputs,
        "counts": {
            "seed_definitions": len(seeds),
            "reachable_named_definitions": len(closure),
            "definition_shapes": dict(sorted(shape_counts.items())),
            "definition_dispositions": dict(sorted(disposition_counts.items())),
            "definition_batches": definition_batch_counts,
            "definition_directions": definition_direction_counts,
            "cross_slice_ownership": cross_slice_ownership_counts,
            "definition_direction_ownership_sha256": (
                definition_direction_ownership_sha256
            ),
            "definition_cpp_mapping_sha256": (
                definition_cpp_mapping_sha256
            ),
            "schema_path_dispositions": len(path_records),
            "schema_path_disposition_kinds": dict(
                sorted(Counter(row["disposition"] for row in path_records).items())
            ),
            "schema_path_directions": path_direction_counts,
            "schema_path_kind_directions": path_kind_direction_counts,
            "schema_path_mapping_sha256": schema_path_mapping_sha256,
            "property_declarations": sum(property_counts.values()),
            "property_shapes": property_counts,
            "property_presence_models": presence_counts,
            "array_element_paths": sum(row["schema_node_kind"] == "array_element" for row in path_records),
            "map_value_paths": sum(row["schema_node_kind"] == "map_value" for row in path_records),
            "protocol_defined_opaque_json_paths": len(opaque_paths),
            "open_enum_known_value_fixture_cases": (
                open_enum_known_value_cases
            ),
            "open_enum_future_value_fixture_cases": len(open_enum_names),
        },
        "batch_dependency_graph": {
            batch: sorted(dependencies) for batch, dependencies in sorted(batch_dependencies.items())
        },
        "seeds": [seeds[name] for name in sorted(seeds)],
        "definitions": definition_records,
        "schema_paths": path_records,
        "semantic_strong_identifiers": strong_identifier_paths,
        "cross_slice_definition_ownership": {
            "prior_slice_reuse": sorted(
                name
                for name, ownership in cross_slice_ownership.items()
                if ownership == "PriorSliceReuse"
            ),
            "forward_shared": sorted(
                name
                for name, ownership in cross_slice_ownership.items()
                if ownership == "A11ForwardShared"
            ),
            "a1_1_local": sorted(
                name
                for name, ownership in cross_slice_ownership.items()
                if ownership == "A11Local"
            ),
        },
        "optional_nullable_matrix": {
            "DefaultedValue": {
                "schema_condition": (
                    "optional, non-nullable, and reviewed non-null default"
                ),
                "cpp_wrapper": "T with schema default applied on omission",
                "count": presence_counts["DefaultedValue"],
            },
            "RequiredValue": {
                "schema_condition": "required and non-nullable",
                "cpp_wrapper": "T",
                "count": presence_counts["RequiredValue"],
            },
            "RequiredNullable": {
                "schema_condition": "required and nullable",
                "cpp_wrapper": "std::optional<T>",
                "count": presence_counts["RequiredNullable"],
            },
            "OptionalValue": {
                "schema_condition": "optional and non-nullable",
                "cpp_wrapper": "std::optional<T>",
                "count": presence_counts["OptionalValue"],
            },
            "OptionalNullable": {
                "schema_condition": "optional and nullable",
                "cpp_wrapper": "typed::OptionalNullable<T>",
                "count": presence_counts["OptionalNullable"],
            },
        },
        "protocol_defined_opaque_json": [
            {"schema_path": path, "reason": EXPECTED_PROTOCOL_OPAQUE_PATHS[path]}
            for path in sorted(EXPECTED_PROTOCOL_OPAQUE_PATHS)
        ],
    }
    validate_generated_reports(plan, closure_report)
    return plan, closure_report


def _report_surface_key(value: Any) -> tuple[str, str, str, str] | None:
    if not isinstance(value, Mapping):
        return None
    fields = (
        value.get("category"),
        value.get("domain"),
        value.get("discriminator_field"),
        value.get("name"),
    )
    if not all(isinstance(field, str) and field for field in fields):
        return None
    return fields  # type: ignore[return-value]


def _dependency_cycle(graph: Mapping[str, set[str]]) -> bool:
    active: set[str] = set()
    visited: set[str] = set()

    def visit(node: str) -> bool:
        if node in active:
            return True
        if node in visited:
            return False
        active.add(node)
        for dependency in sorted(graph.get(node, set())):
            if visit(dependency):
                return True
        active.remove(node)
        visited.add(node)
        return False

    return any(visit(node) for node in sorted(graph))


def report_diagnostics(
    plan: Mapping[str, Any], closure: Mapping[str, Any]
) -> list[AuditDiagnostic]:
    """Return stable intrinsic diagnostics for generated-report mutations.

    This deliberately validates the generated review artifacts rather than
    acting as a runtime registry.  Tests mutate one report invariant at a time
    and assert the exact returned diagnostic-code multiset.
    """

    diagnostics: list[AuditDiagnostic] = []

    def add(code: str, location: str, message: str) -> None:
        diagnostics.append(AuditDiagnostic(code, location, message))

    identities_value = plan.get("identities")
    identities = identities_value if isinstance(identities_value, list) else []
    for index, row in enumerate(identities):
        if not isinstance(row, Mapping):
            add(
                "MissingPlanIdentityField",
                f"$.identities[{index}]",
                "identity record is not an object",
            )
            continue
        missing_fields = sorted(REQUIRED_PLAN_IDENTITY_FIELDS - set(row))
        if missing_fields:
            add(
                "MissingPlanIdentityField",
                f"$.identities[{index}]",
                f"missing required Section 5.1 fields: {missing_fields}",
            )
        public_mapping = row.get("required_public_type_or_facade")
        category = row.get("category")
        required_mapping_fields = REQUIRED_PUBLIC_MAPPING_FIELDS.get(
            str(category), set()
        )
        if not isinstance(public_mapping, Mapping):
            add(
                "MissingReviewedPublicMapping",
                f"$.identities[{index}].required_public_type_or_facade",
                "reviewed public mapping is not an object",
            )
        elif required_mapping_fields - set(public_mapping):
            add(
                "MissingReviewedPublicMapping",
                f"$.identities[{index}].required_public_type_or_facade",
                "reviewed public mapping fields are missing: "
                f"{sorted(required_mapping_fields - set(public_mapping))}",
            )

        positive = row.get("required_positive_fixtures")
        if not isinstance(positive, Mapping):
            add(
                "MissingFixturePlanField",
                f"$.identities[{index}].required_positive_fixtures",
                "positive fixture plan is not an object",
            )
        elif REQUIRED_POSITIVE_FIXTURE_FIELDS - set(positive):
            add(
                "MissingFixturePlanField",
                f"$.identities[{index}].required_positive_fixtures",
                "positive fixture fields are missing: "
                f"{sorted(REQUIRED_POSITIVE_FIXTURE_FIELDS - set(positive))}",
            )
        else:
            required_ids = positive.get("required_fixture_ids")
            planned_ids = positive.get("planned_fixture_ids")
            existing_ids = positive.get("existing_fixture_ids")
            base_cases = positive.get("base_cases")
            if not (
                isinstance(required_ids, list)
                and required_ids
                and all(isinstance(value, str) for value in required_ids)
                and isinstance(planned_ids, list)
                and all(isinstance(value, str) for value in planned_ids)
                and isinstance(existing_ids, list)
                and all(isinstance(value, str) for value in existing_ids)
                and isinstance(base_cases, list)
                and len(base_cases) == len(required_ids)
                and all(isinstance(value, Mapping) for value in base_cases)
            ):
                add(
                    "MalformedFixturePlan",
                    f"$.identities[{index}].required_positive_fixtures",
                    "positive fixture IDs/base cases are malformed",
                )

        negative = row.get("required_negative_fixtures")
        if not isinstance(negative, Mapping):
            add(
                "MissingFixturePlanField",
                f"$.identities[{index}].required_negative_fixtures",
                "negative fixture plan is not an object",
            )
        elif REQUIRED_NEGATIVE_FIXTURE_FIELDS - set(negative):
            add(
                "MissingFixturePlanField",
                f"$.identities[{index}].required_negative_fixtures",
                "negative fixture fields are missing: "
                f"{sorted(REQUIRED_NEGATIVE_FIXTURE_FIELDS - set(negative))}",
            )
        else:
            mutation_groups = negative.get("schema_mutations")
            if (
                not isinstance(mutation_groups, Mapping)
                or set(mutation_groups) != REQUIRED_SCHEMA_MUTATION_GROUPS
                or not all(
                    isinstance(cases, list)
                    and all(isinstance(case, Mapping) for case in cases)
                    for cases in mutation_groups.values()
                )
            ):
                add(
                    "MalformedFixturePlan",
                    f"$.identities[{index}].required_negative_fixtures.schema_mutations",
                    "schema mutation groups are missing or malformed",
                )
    mutation_records_by_id: dict[str, str] = {}
    conflicting_mutation_ids: set[str] = set()
    for index, row in enumerate(identities):
        if not isinstance(row, Mapping):
            continue
        negative = row.get("required_negative_fixtures")
        groups = (
            negative.get("schema_mutations")
            if isinstance(negative, Mapping)
            else None
        )
        if not isinstance(groups, Mapping):
            continue
        for group, cases in groups.items():
            if not isinstance(cases, list):
                continue
            for case_index, case in enumerate(cases):
                location = (
                    f"$.identities[{index}].required_negative_fixtures."
                    f"schema_mutations.{group}[{case_index}]"
                )
                if not isinstance(case, Mapping):
                    continue
                required_case_fields = {
                    "applicability",
                    "base_fixture_id",
                    "id",
                    "instance_path",
                    "kind",
                }
                if required_case_fields - set(case):
                    add(
                        "MissingFixturePlanField",
                        location,
                        "mutation case fields are missing: "
                        f"{sorted(required_case_fields - set(case))}",
                    )
                    continue
                case_id = case.get("id")
                if not isinstance(case_id, str) or not case_id:
                    add(
                        "MalformedFixturePlan",
                        location,
                        "mutation case ID is empty or non-string",
                    )
                    continue
                applicability = case.get("applicability")
                if applicability == "not_applicable":
                    if not isinstance(
                        case.get("not_applicable_reason"), str
                    ):
                        add(
                            "MalformedFixturePlan",
                            location,
                            "not-applicable mutation lacks its exact reason",
                        )
                elif applicability not in {
                    "required",
                    "shared_category_fixture",
                    "negative_requiredness_control",
                }:
                    add(
                        "MalformedFixturePlan",
                        location,
                        f"unknown mutation applicability {applicability!r}",
                    )
                rendered = json.dumps(
                    dict(case), sort_keys=True, separators=(",", ":")
                )
                previous = mutation_records_by_id.setdefault(
                    case_id, rendered
                )
                if previous != rendered:
                    conflicting_mutation_ids.add(case_id)
    if conflicting_mutation_ids:
        add(
            "DuplicateFixtureMutationId",
            "$.identities[*].required_negative_fixtures.schema_mutations",
            "one mutation ID denotes conflicting case records: "
            f"{sorted(conflicting_mutation_ids)}",
        )

    identity_keys = [
        _report_surface_key(row.get("protocol_surface_key"))
        if isinstance(row, Mapping)
        else None
        for row in identities
    ]
    valid_identity_keys = [key for key in identity_keys if key is not None]
    duplicate_identity_keys = sorted(
        key for key, count in Counter(valid_identity_keys).items() if count > 1
    )
    if duplicate_identity_keys:
        add(
            "DuplicateIdentity",
            "$.identities",
            f"duplicate exact ProtocolSurfaceKey entries: {duplicate_identity_keys}",
        )
        duplicate_batches = False
        by_key: dict[tuple[str, str, str, str], set[str]] = defaultdict(set)
        for row, key in zip(identities, identity_keys):
            if key is not None and isinstance(row, Mapping):
                batch = row.get("owning_implementation_batch")
                if isinstance(batch, str):
                    by_key[key].add(batch)
        duplicate_batches = any(len(by_key[key]) > 1 for key in duplicate_identity_keys)
        if duplicate_batches:
            add(
                "DuplicateBatchAssignment",
                "$.identities[*].owning_implementation_batch",
                "an exact ProtocolSurfaceKey is assigned to more than one implementation batch",
            )
    if len(identities) != 151 or len(valid_identity_keys) != 151:
        add(
            "IdentityCountMismatch",
            "$.identities",
            f"expected 151 identity records, found {len(identities)}",
        )

    ratchet_value = plan.get("final_exact_a1_1_ratchet")
    ratchet = ratchet_value if isinstance(ratchet_value, list) else []
    ratchet_keys = [_report_surface_key(row) for row in ratchet]
    valid_ratchet_keys = [key for key in ratchet_keys if key is not None]
    if len(valid_ratchet_keys) != 151 or len(set(valid_ratchet_keys)) != 151:
        add(
            "IdentityCountMismatch",
            "$.final_exact_a1_1_ratchet",
            "the exact A1.1 ratchet must contain 151 unique identities",
        )
    expected_key_set = set(valid_ratchet_keys)
    actual_key_set = set(valid_identity_keys)
    missing = sorted(expected_key_set - actual_key_set)
    extra = sorted(actual_key_set - expected_key_set)
    if missing:
        add("MissingIdentity", "$.identities", f"missing ratcheted identities: {missing}")
    if extra:
        add("CrossSliceIdentity", "$.identities", f"unexpected identities: {extra}")

    category_counts: Counter[str] = Counter()
    classifications: Counter[str] = Counter()
    modules: Counter[str] = Counter()
    schema_statuses: Counter[str] = Counter()
    implementation_statuses: Counter[str] = Counter()
    identity_batches: Counter[str] = Counter()
    identity_directions: Counter[str] = Counter()
    discriminator_representations: Counter[str] = Counter()
    request_result_kinds: Counter[str] = Counter()
    unit_methods: set[str] = set()
    request_methods: set[str] = set()
    unstable = False
    discriminator_mapping_mismatch = False
    identity_direction_mismatch = False
    for row, key in zip(identities, identity_keys):
        if key is None or not isinstance(row, Mapping):
            continue
        category, domain, _, method = key
        if category == "item_discriminator" and domain == "ThreadItem":
            category_counts["thread_item"] += 1
        elif category == "item_discriminator" and domain == "ResponseItem":
            category_counts["response_item"] += 1
        else:
            category_counts[category] += 1
        classifications[str(row.get("classification"))] += 1
        modules[str(row.get("module"))] += 1
        schema_statuses[str(row.get("current_schema_status"))] += 1
        implementation_statuses[str(row.get("current_implementation_status"))] += 1
        identity_batches[str(row.get("owning_implementation_batch"))] += 1
        directionality = row.get("directionality")
        identity_directions[str(directionality)] += 1
        unstable = unstable or row.get("stability") != "stable"
        identity_key = Key(category, domain, key[2], method)
        try:
            expected_directionality = reviewed_identity_directionality(
                identity_key
            )
        except AuditError:
            expected_directionality = ""
        if (
            directionality not in VALID_DIRECTIONALITIES
            or directionality != expected_directionality
        ):
            identity_direction_mismatch = True
        private_mapping = row.get(
            "required_private_codec_or_descriptor"
        )
        behavior = (
            private_mapping.get("discriminator_behavior")
            if isinstance(private_mapping, Mapping)
            else None
        )
        if isinstance(behavior, Mapping):
            discriminator_representations[
                str(behavior.get("representation"))
            ] += 1
        try:
            expected_behavior = discriminator_behavior(
                identity_key, expected_directionality
            )
        except AuditError:
            expected_behavior = {}
        if (
            not isinstance(private_mapping, Mapping)
            or private_mapping.get("descriptor_key")
            != identity_key.compact()
            or behavior != expected_behavior
        ):
            discriminator_mapping_mismatch = True
        public_mapping = row.get("required_public_type_or_facade")
        if isinstance(public_mapping, Mapping):
            if category == "server_notification":
                expected_payload = (
                    "typed::"
                    + SERVER_NOTIFICATION_PAYLOAD_TYPES.get(method, "")
                )
                if (
                    public_mapping.get("canonical_payload_aggregate")
                    != expected_payload
                    or public_mapping.get("existing_semantic_adapter")
                    != SERVER_NOTIFICATION_SEMANTIC_ADAPTERS.get(method)
                ):
                    add(
                        "MalformedReviewedPublicMapping",
                        f"$.identities[{method!r}].required_public_type_or_facade",
                        "notification payload/semantic-adapter mapping changed",
                    )
            elif category == "item_discriminator":
                alternatives = (
                    THREAD_ITEM_ALTERNATIVES
                    if domain == "ThreadItem"
                    else RESPONSE_ITEM_ALTERNATIVES
                )
                expected_unknown = (
                    "typed::UnknownItem"
                    if domain == "ThreadItem"
                    else "typed::UnknownResponseItem"
                )
                if (
                    public_mapping.get("canonical_alternative")
                    != "typed::" + alternatives.get(method, "")
                    or public_mapping.get("future_unknown_alternative")
                    != expected_unknown
                ):
                    add(
                        "MalformedReviewedPublicMapping",
                        f"$.identities[{domain!r}:{method!r}].required_public_type_or_facade",
                        "item alternative/future-unknown mapping changed",
                    )
            elif category == "tagged_union_discriminator":
                expected_alternative = NESTED_UNION_ALTERNATIVES.get(
                    (domain, method)
                )
                expected_unknown = NESTED_UNION_FUTURE_UNKNOWN.get(domain)
                if (
                    public_mapping.get("canonical_alternative")
                    != f"typed::{expected_alternative}"
                    or public_mapping.get("future_unknown_alternative")
                    != f"typed::{expected_unknown}"
                ):
                    add(
                        "MalformedReviewedPublicMapping",
                        f"$.identities[{domain!r}:{method!r}].required_public_type_or_facade",
                        "nested alternative/future-unknown mapping changed",
                    )
        if category == "client_request":
            request_methods.add(method)
            contract = row.get("request_contract")
            if not isinstance(contract, Mapping):
                add(
                    "ContractMismatch",
                    f"$.identities[{method!r}].request_contract",
                    "client request lacks its successful-result contract",
                )
                continue
            missing_contract_fields = sorted(
                REQUIRED_REQUEST_CONTRACT_FIELDS - set(contract)
            )
            if missing_contract_fields:
                add(
                    "MissingRequestContractField",
                    f"$.identities[{method!r}].request_contract",
                    f"missing required request fields: {missing_contract_fields}",
                )
            kind = contract.get("result_contract_kind")
            request_result_kinds[str(kind)] += 1
            if kind == "Unit":
                unit_methods.add(method)
            facade, api_method = OPERATION_API_NAMES.get(method, ("", ""))
            if contract.get("proposed_public_method_name") != f"{facade}::{api_method}":
                add(
                    "ContractMismatch",
                    f"$.identities[{method!r}].request_contract.proposed_public_method_name",
                    "reviewed grouped-facade mapping changed",
                )

    normalized_taxonomy = {
        name: category_counts[name] for name in EXPECTED_CATEGORY_COUNTS
    }
    if normalized_taxonomy != EXPECTED_CATEGORY_COUNTS:
        add("TaxonomyMismatch", "$.identities", f"taxonomy changed: {normalized_taxonomy}")
    if category_counts["client_notification"] or category_counts["server_request"]:
        add(
            "CrossSliceIdentity",
            "$.identities",
            "A1.1 must contain no client notification or server request",
        )
    if unstable:
        add("UnstableIdentity", "$.identities[*].stability", "A1.1 contains an unstable identity")
    if dict(sorted(classifications.items())) != EXPECTED_CLASSIFICATION_COUNTS:
        add("AssignmentMismatch", "$.identities[*].classification", "classification counts changed")
    if dict(sorted(modules.items())) != EXPECTED_MODULE_COUNTS:
        add("AssignmentMismatch", "$.identities[*].module", "module ownership counts changed")
    if dict(sorted(schema_statuses.items())) != EXPECTED_SCHEMA_STATUS_COUNTS:
        add("StatusSplitMismatch", "$.identities[*].current_schema_status", "schema-status split changed")
    if implementation_statuses != Counter({"Implemented": 26, "NotImplemented": 125}):
        add(
            "StatusSplitMismatch",
            "$.identities[*].current_implementation_status",
            "implementation-status split changed",
        )
    if dict(sorted(identity_batches.items())) != EXPECTED_BATCH_IDENTITY_COUNTS:
        add("AssignmentMismatch", "$.identities[*].owning_implementation_batch", "identity batch counts changed")
    identity_direction_mismatch = (
        identity_direction_mismatch
        or dict(sorted(identity_directions.items()))
        != EXPECTED_IDENTITY_DIRECTIONS
    )
    if request_methods != set(OPERATION_API_NAMES):
        add("ContractMismatch", "$.identities[*].request_contract", "request method contract set changed")
    if dict(sorted(request_result_kinds.items())) != EXPECTED_RESULT_KIND_COUNTS:
        add("ResultKindMismatch", "$.identities[*].request_contract", "result-kind counts changed")
    if unit_methods != EXPECTED_UNIT_METHODS:
        add("ResultKindMismatch", "$.unit_result_methods", "reviewed Unit method set changed")
    if plan.get("unit_result_methods") != sorted(EXPECTED_UNIT_METHODS):
        add("ResultKindMismatch", "$.unit_result_methods", "stored Unit method ratchet changed")

    plan_counts_value = plan.get("counts")
    plan_counts = plan_counts_value if isinstance(plan_counts_value, Mapping) else {}
    if (
        plan_counts.get("identity_denominator") != 151
        or plan_counts.get("successful_result_root_obligations_not_identities") != 22
    ):
        add(
            "TaxonomyArithmeticMismatch",
            "$.counts",
            "A1.1 has exactly 151 identities; its 22 successful-result roots are obligations, not extra identities",
        )
    if plan_counts.get("taxonomy") != normalized_taxonomy:
        add("TaxonomyMismatch", "$.counts.taxonomy", "taxonomy summary disagrees with identity records")
    if plan_counts.get("classifications") != dict(sorted(classifications.items())):
        add(
            "AssignmentMismatch",
            "$.counts.classifications",
            "classification summary disagrees with identity records",
        )
    if plan_counts.get("modules") != dict(sorted(modules.items())):
        add("AssignmentMismatch", "$.counts.modules", "module summary disagrees with identity records")
    if plan_counts.get("current_a1_1_schema_status") != dict(sorted(schema_statuses.items())):
        add(
            "StatusSplitMismatch",
            "$.counts.current_a1_1_schema_status",
            "schema-status summary disagrees with identity records",
        )
    if plan_counts.get("current_global_schema_status") != EXPECTED_GLOBAL_SCHEMA_STATUS:
        add(
            "StatusSplitMismatch",
            "$.counts.current_global_schema_status",
            "current global schema-status ratchet changed",
        )
    if plan_counts.get("derived_final_global_schema_status") != EXPECTED_FINAL_GLOBAL_SCHEMA_STATUS:
        add(
            "StatusSplitMismatch",
            "$.counts.derived_final_global_schema_status",
            "derived final global schema-status ratchet changed",
        )
    if plan_counts.get("result_contract_kinds") != dict(sorted(request_result_kinds.items())):
        add(
            "ResultKindMismatch",
            "$.counts.result_contract_kinds",
            "result-kind summary disagrees with request records",
        )
    if plan_counts.get("batch_identities") != dict(sorted(identity_batches.items())):
        add(
            "AssignmentMismatch",
            "$.counts.batch_identities",
            "identity batch summary disagrees with identity records",
        )
    if plan_counts.get("batch_definitions") != EXPECTED_BATCH_DEFINITION_COUNTS:
        add(
            "AssignmentMismatch",
            "$.counts.batch_definitions",
            "definition batch summary changed",
        )
    if (
        plan_counts.get("identity_directions")
        != dict(sorted(identity_directions.items()))
        or identity_direction_mismatch
    ):
        add(
            "DirectionalityMismatch",
            "$.counts.identity_directions",
            "identity direction summary disagrees with exact identity records",
        )
    exact_identity_set = (
        len(valid_identity_keys) == 151
        and len(set(valid_identity_keys)) == 151
        and set(valid_identity_keys) == set(valid_ratchet_keys)
    )
    expected_discriminator_representations = {
        "externally_tagged_object_key": 5,
        "json_rpc_method": 59,
        "object_discriminator_property": 76,
        "scalar_union_literal": 11,
    }
    actual_discriminator_sha256 = sha256_json(
        [
            discriminator_mapping_projection(row)
            for row in identities
            if isinstance(row, Mapping)
        ]
    )
    if exact_identity_set and not identity_direction_mismatch and (
        discriminator_mapping_mismatch
        or dict(sorted(discriminator_representations.items()))
        != expected_discriminator_representations
        or plan_counts.get("discriminator_representations")
        != expected_discriminator_representations
        or plan_counts.get("discriminator_mapping_sha256")
        != actual_discriminator_sha256
    ):
        add(
            "DiscriminatorLiteralMismatch",
            "$.identities[*].required_private_codec_or_descriptor.discriminator_behavior",
            "exact discriminator literal/representation/codec behavior changed",
        )

    batches_value = plan.get("batches")
    batches = batches_value if isinstance(batches_value, list) else []
    batch_graph: dict[str, set[str]] = {}
    batch_names: list[str] = []
    for row in batches:
        if not isinstance(row, Mapping) or not isinstance(row.get("batch"), str):
            continue
        batch = row["batch"]
        batch_names.append(batch)
        dependencies = row.get("depends_on")
        batch_graph[batch] = {
            dependency for dependency in dependencies or [] if isinstance(dependency, str)
        } if isinstance(dependencies, list) else set()
        expected_identity_count = EXPECTED_BATCH_IDENTITY_COUNTS.get(batch, 0)
        expected_definition_count = EXPECTED_BATCH_DEFINITION_COUNTS.get(batch, 0)
        if (
            row.get("identity_count") != expected_identity_count
            or row.get("definition_count") != expected_definition_count
        ):
            add(
                "AssignmentMismatch",
                f"$.batches[{batch!r}]",
                "batch count summary disagrees with the frozen dependency plan",
            )
    if len(batch_names) != len(set(batch_names)):
        add("DuplicateBatchAssignment", "$.batches", "duplicate implementation batch record")
    if set(batch_names) != {"B2", "B3", "B4", "B5", "B6"} or _dependency_cycle(batch_graph):
        add("BatchDependencyCycle", "$.batches[*].depends_on", "batch dependency graph is cyclic or incomplete")

    expected_owned: dict[str, list[tuple[str, str, str, str]]] = {
        batch: sorted(
            key
            for row, key in zip(identities, identity_keys)
            if key is not None
            and isinstance(row, Mapping)
            and row.get("owning_implementation_batch") == batch
        )
        for batch in ("B2", "B3", "B4", "B5")
    }
    expected_stages: list[dict[str, Any]] = []
    cumulative: list[tuple[str, str, str, str]] = []
    for batch in ("B2", "B3", "B4", "B5"):
        cumulative = sorted(cumulative + expected_owned[batch])
        expected_stages.append(
            {
                "batch": batch,
                "owned_identity_count": len(expected_owned[batch]),
                "cumulative_identity_count": len(cumulative),
                "owned_identities": [
                    tuple_key_object(key) for key in expected_owned[batch]
                ],
                "cumulative_identities": [
                    tuple_key_object(key) for key in cumulative
                ],
            }
        )
    if plan.get("staged_exact_a1_1_ratchets") != expected_stages:
        add(
            "StagedRatchetMismatch",
            "$.staged_exact_a1_1_ratchets",
            "owned/cumulative B2-B5 ratchet schema or exact identities changed",
        )
    expected_residual = [
        tuple_key_object(key) for key in sorted(EXPECTED_RESIDUAL_PARTIAL_KEYS)
    ]
    if plan.get("expected_residual_partial_identities") != expected_residual:
        add(
            "ResidualPartialMismatch",
            "$.expected_residual_partial_identities",
            "exact eight residual partial ProtocolSurfaceKeys changed",
        )

    seeds_value = closure.get("seeds")
    seeds = seeds_value if isinstance(seeds_value, list) else []
    seed_names = [
        row.get("definition") for row in seeds
        if isinstance(row, Mapping) and isinstance(row.get("definition"), str)
    ]
    if len(seeds) != 100 or len(seed_names) != 100 or len(set(seed_names)) != 100:
        add("MissingClosureDisposition", "$.seeds", "closure must contain 100 unique seed definitions")

    definitions_value = closure.get("definitions")
    definitions = definitions_value if isinstance(definitions_value, list) else []
    definition_names: list[str] = []
    definition_batches: Counter[str] = Counter()
    definition_shapes: Counter[str] = Counter()
    definition_dispositions: Counter[str] = Counter()
    definition_directions: Counter[str] = Counter()
    cross_slice_ownership_counts: Counter[str] = Counter()
    owner_records: dict[str, list[Mapping[str, Any]]] = defaultdict(list)
    missing_disposition = False
    missing_cpp_owner = False
    opaque_definition = False
    definition_direction_mismatch = False
    cross_slice_ownership_mismatch = False
    for row in definitions:
        if not isinstance(row, Mapping):
            missing_disposition = True
            continue
        key = row.get("definition_key")
        name = key.get("name") if isinstance(key, Mapping) else None
        if isinstance(name, str):
            definition_names.append(name)
        disposition = row.get("disposition")
        if disposition not in VALID_TYPE_DISPOSITIONS:
            missing_disposition = True
        elif isinstance(disposition, str):
            definition_dispositions[disposition] += 1
        if disposition == "ProtocolDefinedOpaqueJson":
            opaque_definition = True
        shape = row.get("schema_shape")
        if isinstance(shape, str):
            definition_shapes[shape] += 1
        directionality = row.get("directionality")
        if directionality not in VALID_DIRECTIONALITIES:
            definition_direction_mismatch = True
        else:
            definition_directions[str(directionality)] += 1
        ownership = row.get("cross_slice_ownership")
        if ownership not in VALID_CROSS_SLICE_OWNERSHIP:
            cross_slice_ownership_mismatch = True
        else:
            cross_slice_ownership_counts[str(ownership)] += 1
        reused_by = row.get("reused_by_later_slices")
        if not (
            isinstance(reused_by, list)
            and all(
                isinstance(slice_name, str)
                and slice_name in FORWARD_SLICE_NAMES
                for slice_name in reused_by
            )
            and reused_by == sorted(set(reused_by))
        ):
            cross_slice_ownership_mismatch = True
        elif ownership == "A11Local" and reused_by:
            cross_slice_ownership_mismatch = True
        elif ownership == "A11ForwardShared" and not reused_by:
            cross_slice_ownership_mismatch = True
        if isinstance(name, str):
            expected_prior = name in PRIOR_SLICE_REUSED_DEFINITIONS
            if (
                (ownership == "PriorSliceReuse") != expected_prior
                or (
                    expected_prior
                    and disposition != "ReusedExistingType"
                )
            ):
                cross_slice_ownership_mismatch = True
        if shape == "open_string_enum":
            fixture_plan = row.get("fixture_plan")
            if not (
                isinstance(fixture_plan, Mapping)
                and isinstance(
                    fixture_plan.get("known_value_case_ids"), list
                )
                and fixture_plan.get("known_value_case_ids")
                and all(
                    isinstance(value, str)
                    for value in fixture_plan["known_value_case_ids"]
                )
                and isinstance(
                    fixture_plan.get("future_value_case_id"), str
                )
                and isinstance(
                    fixture_plan.get("future_value_typed_diagnostic"),
                    Mapping,
                )
            ):
                add(
                    "MalformedFixturePlan",
                    "$.definitions[*].fixture_plan",
                    "open-string enum lacks exact known/future fixture IDs",
                )
        definition_batches[str(row.get("owning_implementation_batch"))] += 1
        owner = row.get("cpp_owner")
        if isinstance(owner, str) and owner:
            owner_records[owner].append(row)
        else:
            missing_cpp_owner = True
    if len(definitions) != 164 or len(definition_names) != 164 or len(set(definition_names)) != 164:
        add("MissingClosureDisposition", "$.definitions", "closure must contain 164 uniquely owned definitions")
    if missing_disposition:
        add("MissingClosureDisposition", "$.definitions[*].disposition", "definition disposition is missing or invalid")
    if missing_cpp_owner:
        add(
            "MissingClosureMapping",
            "$.definitions[*].cpp_owner",
            "every reachable definition requires a nonempty C++ owner mapping",
        )
    if opaque_definition:
        add(
            "OpaqueJsonMisuse",
            "$.definitions[*].disposition",
            "a known named definition is represented wholesale as opaque JSON",
        )
    if dict(sorted(definition_batches.items())) != EXPECTED_BATCH_DEFINITION_COUNTS:
        add("AssignmentMismatch", "$.definitions[*].owning_implementation_batch", "definition batch counts changed")
    if (
        definition_direction_mismatch
        or dict(sorted(definition_directions.items()))
        != EXPECTED_DEFINITION_DIRECTIONS
    ):
        add(
            "DirectionalityMismatch",
            "$.definitions[*].directionality",
            "definition directionality changed",
        )
        definition_direction_mismatch = True
    if (
        cross_slice_ownership_mismatch
        or dict(sorted(cross_slice_ownership_counts.items()))
        != EXPECTED_CROSS_SLICE_OWNERSHIP
    ):
        add(
            "CrossSliceOwnershipMismatch",
            "$.definitions[*].cross_slice_ownership",
            "A1.1 local/forward-shared/prior-reuse ownership changed",
        )
        cross_slice_ownership_mismatch = True
    conflicting_owners = sorted(
        owner
        for owner, rows in owner_records.items()
        if len(rows) > 1
        and not (
            owner == "typed::Unit"
            and all(row.get("disposition") == "ReusedExistingType" for row in rows)
        )
    )
    if conflicting_owners:
        add(
            "ConflictingCppOwnership",
            "$.definitions[*].cpp_owner",
            f"duplicate C++ ownership mappings: {conflicting_owners}",
        )

    definition_rows_by_name = {
        row["definition_key"]["name"]: row
        for row in definitions
        if isinstance(row, Mapping)
        and isinstance(row.get("definition_key"), Mapping)
        and isinstance(row["definition_key"].get("name"), str)
    }
    batch_order = {"B2": 2, "B3": 3, "B4": 4, "B5": 5}
    cross_slice_dependency = False
    forward_dependency = False
    derived_closure_graph: dict[str, set[str]] = {
        batch: set() for batch in batch_order
    }
    for name, row in definition_rows_by_name.items():
        owner_batch = row.get("owning_implementation_batch")
        dependencies = row.get("direct_dependencies")
        if not isinstance(dependencies, list):
            cross_slice_dependency = True
            continue
        for dependency in dependencies:
            dependency_row = definition_rows_by_name.get(dependency)
            if dependency_row is None:
                cross_slice_dependency = True
                continue
            dependency_batch = dependency_row.get("owning_implementation_batch")
            if owner_batch not in batch_order or dependency_batch not in batch_order:
                cross_slice_dependency = True
                continue
            if batch_order[dependency_batch] > batch_order[owner_batch]:
                forward_dependency = True
            elif dependency_batch != owner_batch:
                derived_closure_graph[owner_batch].add(dependency_batch)
    if cross_slice_dependency:
        add(
            "CrossSliceDependency",
            "$.definitions[*].direct_dependencies",
            "a closure definition references a definition outside the guarded A1.1 closure",
        )
    if forward_dependency:
        add(
            "BatchDependencyCycle",
            "$.definitions[*].direct_dependencies",
            "a definition depends on a definition assigned to a later implementation batch",
        )

    schema_paths_value = closure.get("schema_paths")
    schema_paths = schema_paths_value if isinstance(schema_paths_value, list) else []
    path_names: list[str] = []
    path_missing_disposition = False
    path_missing_mapping = False
    actual_opaque_paths: set[str] = set()
    schema_path_kinds: Counter[str] = Counter()
    schema_path_dispositions: Counter[str] = Counter()
    schema_path_directions: Counter[str] = Counter()
    schema_path_kind_directions: dict[str, Counter[str]] = {
        "array_element": Counter(),
        "map_value": Counter(),
        "property": Counter(),
    }
    property_shapes: Counter[str] = Counter()
    property_presence_models: Counter[str] = Counter()
    path_direction_mismatch = False
    cpp_path_mapping_mismatch = False
    optional_nullable_mapping_mismatch = False
    strong_identifier_mapping_mismatch = False
    observed_strong_identifiers: dict[str, dict[str, list[str]]] = {
        identifier: {
            "scalar_property_paths": [],
            "vector_property_paths": [],
            "vector_element_paths": [],
        }
        for identifier in EXPECTED_STRONG_IDENTIFIER_COUNTS
    }
    for row in schema_paths:
        if not isinstance(row, Mapping):
            path_missing_disposition = True
            continue
        path = row.get("schema_path")
        if isinstance(path, str):
            path_names.append(path)
        kind = row.get("schema_node_kind")
        if isinstance(kind, str):
            schema_path_kinds[kind] += 1
        directionality = row.get("directionality")
        if directionality not in VALID_DIRECTIONALITIES:
            path_direction_mismatch = True
        else:
            schema_path_directions[str(directionality)] += 1
            if kind in schema_path_kind_directions:
                schema_path_kind_directions[str(kind)][
                    str(directionality)
                ] += 1
        if isinstance(path, str):
            match = re.match(
                r"^#/definitions/v2/([^/]+)(?:/|$)", path
            )
            definition_row = (
                definition_rows_by_name.get(match.group(1))
                if match is not None
                else None
            )
            if definition_row is None or (
                definition_row.get("directionality") != directionality
            ):
                path_direction_mismatch = True
        cpp_owner = row.get("cpp_owner")
        cpp_member = row.get("cpp_member")
        cpp_base_type = row.get("cpp_base_type")
        cpp_type = row.get("cpp_type")
        if not all(
            isinstance(value, str) and value
            for value in (
                cpp_owner,
                cpp_member,
                cpp_base_type,
                cpp_type,
            )
        ):
            cpp_path_mapping_mismatch = True
        elif row.get("cpp_mapping") != (
            f"{cpp_owner}::{cpp_member} -> {cpp_type}"
        ):
            cpp_path_mapping_mismatch = True
        if kind == "property":
            required = row.get("required")
            nullable = row.get("nullable")
            if isinstance(required, bool) and isinstance(nullable, bool):
                property_shapes[
                    ("required" if required else "optional")
                    + ("_nullable" if nullable else "_nonnullable")
                ] += 1
                reviewed_defaulted = (
                    isinstance(path, str)
                    and path in REVIEWED_DEFAULTED_PROPERTY_VALUES
                )
                reviewed_nullable_default = (
                    isinstance(path, str)
                    and path in REVIEWED_NULLABLE_DEFAULT_NULL_PATHS
                )
                expected_presence = property_presence_model(
                    required, nullable, reviewed_defaulted
                )
                property_presence_models[expected_presence] += 1
                expected_cpp_type = (
                    wrap_property_cpp_type(
                        str(cpp_base_type), expected_presence
                    )
                    if isinstance(cpp_base_type, str)
                    else None
                )
                if (
                    row.get("presence_model") != expected_presence
                    or cpp_type != expected_cpp_type
                ):
                    optional_nullable_mapping_mismatch = True
                if reviewed_defaulted and (
                    row.get("schema_default")
                    != REVIEWED_DEFAULTED_PROPERTY_VALUES[path]
                    or row.get("default_behavior")
                    != "CanonicalValueOnOmission"
                ):
                    optional_nullable_mapping_mismatch = True
                if reviewed_nullable_default and (
                    row.get("schema_default") is not None
                    or row.get("default_behavior")
                    != "PreserveOmittedNullValue"
                    or expected_presence != "OptionalNullable"
                ):
                    optional_nullable_mapping_mismatch = True
            else:
                optional_nullable_mapping_mismatch = True
        elif any(
            field in row
            for field in (
                "required",
                "nullable",
                "presence_model",
            )
        ):
            optional_nullable_mapping_mismatch = True
        disposition = row.get("disposition")
        if disposition not in VALID_TYPE_DISPOSITIONS:
            path_missing_disposition = True
        elif isinstance(disposition, str):
            schema_path_dispositions[disposition] += 1
        if disposition == "ProtocolDefinedOpaqueJson" and isinstance(path, str):
            actual_opaque_paths.add(path)
        if not isinstance(row.get("cpp_mapping"), str) or not row.get("cpp_mapping"):
            path_missing_mapping = True
        if isinstance(path, str) and isinstance(kind, str):
            semantic = semantic_strong_identifier(path, kind)
            if semantic is None:
                if (
                    "semantic_identifier" in row
                    or "semantic_identifier_role" in row
                    or disposition == "StrongIdentifier"
                ):
                    strong_identifier_mapping_mismatch = True
            else:
                identifier, role = semantic
                role_key = {
                    "scalar_property": "scalar_property_paths",
                    "vector_property": "vector_property_paths",
                    "vector_element": "vector_element_paths",
                }[role]
                observed_strong_identifiers[identifier][role_key].append(
                    path
                )
                expected_base_type = (
                    f"std::vector<typed::{identifier}>"
                    if role == "vector_property"
                    else f"typed::{identifier}"
                )
                if (
                    row.get("semantic_identifier")
                    != f"typed::{identifier}"
                    or row.get("semantic_identifier_role") != role
                    or cpp_base_type != expected_base_type
                    or disposition != "StrongIdentifier"
                ):
                    strong_identifier_mapping_mismatch = True
    for reviewed_path, (
        expected_owner,
        expected_member,
        expected_base_type,
    ) in sorted(EXPLICIT_SCHEMA_CPP_MAPPINGS.items()):
        reviewed_rows = [
            row
            for row in schema_paths
            if isinstance(row, Mapping)
            and row.get("schema_path") == reviewed_path
        ]
        if (
            len(reviewed_rows) != 1
            or reviewed_rows[0].get("cpp_owner") != expected_owner
            or reviewed_rows[0].get("cpp_member") != expected_member
            or reviewed_rows[0].get("cpp_base_type")
            != expected_base_type
            or reviewed_rows[0].get("cpp_type") != expected_base_type
            or reviewed_rows[0].get("cpp_mapping")
            != (
                f"{expected_owner}::{expected_member} -> "
                f"{expected_base_type}"
            )
        ):
            cpp_path_mapping_mismatch = True
    if len(path_names) != len(set(path_names)):
        add("ConflictingCppOwnership", "$.schema_paths", "duplicate schema-path ownership disposition")
    if path_missing_disposition:
        add("MissingClosureDisposition", "$.schema_paths[*].disposition", "schema path disposition is missing or invalid")
    if path_missing_mapping:
        add(
            "MissingClosureMapping",
            "$.schema_paths[*].cpp_mapping",
            "every reachable schema path requires a nonempty C++ mapping",
        )
    normalized_kind_directions = {
        kind: {
            direction: counts[direction]
            for direction in sorted(VALID_DIRECTIONALITIES)
        }
        for kind, counts in sorted(
            schema_path_kind_directions.items()
        )
    }
    if (
        path_direction_mismatch
        or dict(sorted(schema_path_directions.items()))
        != EXPECTED_SCHEMA_PATH_DIRECTIONS
        or normalized_kind_directions
        != EXPECTED_SCHEMA_PATH_KIND_DIRECTIONS
    ):
        add(
            "DirectionalityMismatch",
            "$.schema_paths[*].directionality",
            "schema-path direction/kind accounting changed",
        )
        path_direction_mismatch = True
    optional_nullable_mapping_mismatch = (
        optional_nullable_mapping_mismatch
        or dict(sorted(property_presence_models.items()))
        != EXPECTED_PROPERTY_PRESENCE_MODELS
        or not (
            set(REVIEWED_DEFAULTED_PROPERTY_VALUES)
            | REVIEWED_NULLABLE_DEFAULT_NULL_PATHS
        ).issubset(path_names)
    )
    if (
        optional_nullable_mapping_mismatch
        and not strong_identifier_mapping_mismatch
    ):
        add(
            "OptionalNullableMappingMismatch",
            "$.schema_paths[*].presence_model",
            "required/optional/nullable C++ wrapper matrix changed",
        )
    for identifier in observed_strong_identifiers:
        for role in observed_strong_identifiers[identifier]:
            observed_strong_identifiers[identifier][role].sort()
    observed_identifier_counts = {
        identifier: {
            role: len(paths)
            for role, paths in sorted(roles.items())
        }
        for identifier, roles in sorted(
            observed_strong_identifiers.items()
        )
    }
    strong_listing = closure.get("semantic_strong_identifiers")
    if (
        strong_identifier_mapping_mismatch
        or observed_identifier_counts
        != EXPECTED_STRONG_IDENTIFIER_COUNTS
        or strong_listing != observed_strong_identifiers
    ):
        add(
            "StrongIdentifierMappingMismatch",
            "$.semantic_strong_identifiers",
            "semantic Thread/Turn/Item/Response/Model/Call/Session/"
            "ClientUserMessage ID mapping changed",
        )
        strong_identifier_mapping_mismatch = True
    if actual_opaque_paths != set(EXPECTED_PROTOCOL_OPAQUE_PATHS):
        add(
            "OpaqueJsonMisuse",
            "$.schema_paths[*].disposition",
            "protocol-defined opaque JSON paths differ from the reviewed exact set",
        )
    opaque_listing_value = closure.get("protocol_defined_opaque_json")
    opaque_listing = opaque_listing_value if isinstance(opaque_listing_value, list) else []
    listed_opaque_paths = {
        row.get("schema_path")
        for row in opaque_listing
        if isinstance(row, Mapping) and isinstance(row.get("schema_path"), str)
    }
    if listed_opaque_paths != set(EXPECTED_PROTOCOL_OPAQUE_PATHS):
        add(
            "OpaqueJsonMisuse",
            "$.protocol_defined_opaque_json",
            "reviewed opaque JSON evidence listing changed",
        )

    cross_listing_value = closure.get(
        "cross_slice_definition_ownership"
    )
    expected_cross_listing = {
        "prior_slice_reuse": sorted(
            name
            for name, row in definition_rows_by_name.items()
            if row.get("cross_slice_ownership") == "PriorSliceReuse"
        ),
        "forward_shared": sorted(
            name
            for name, row in definition_rows_by_name.items()
            if row.get("cross_slice_ownership") == "A11ForwardShared"
        ),
        "a1_1_local": sorted(
            name
            for name, row in definition_rows_by_name.items()
            if row.get("cross_slice_ownership") == "A11Local"
        ),
    }
    definition_direction_ownership_sha256 = sha256_json(
        [
            definition_direction_ownership_projection(row)
            for row in definitions
            if isinstance(row, Mapping)
        ]
    )
    definition_cpp_mapping_sha256 = sha256_json(
        [
            definition_cpp_mapping_projection(row)
            for row in definitions
            if isinstance(row, Mapping)
        ]
    )
    schema_path_mapping_sha256 = sha256_json(
        [
            schema_path_mapping_projection(row)
            for row in schema_paths
            if isinstance(row, Mapping)
        ]
    )
    expected_optional_nullable_matrix = {
        "DefaultedValue": {
            "schema_condition": (
                "optional, non-nullable, and reviewed non-null default"
            ),
            "cpp_wrapper": "T with schema default applied on omission",
            "count": property_presence_models["DefaultedValue"],
        },
        "RequiredValue": {
            "schema_condition": "required and non-nullable",
            "cpp_wrapper": "T",
            "count": property_presence_models["RequiredValue"],
        },
        "RequiredNullable": {
            "schema_condition": "required and nullable",
            "cpp_wrapper": "std::optional<T>",
            "count": property_presence_models["RequiredNullable"],
        },
        "OptionalValue": {
            "schema_condition": "optional and non-nullable",
            "cpp_wrapper": "std::optional<T>",
            "count": property_presence_models["OptionalValue"],
        },
        "OptionalNullable": {
            "schema_condition": "optional and nullable",
            "cpp_wrapper": "typed::OptionalNullable<T>",
            "count": property_presence_models["OptionalNullable"],
        },
    }
    if (
        cross_listing_value != expected_cross_listing
        and not cross_slice_ownership_mismatch
    ):
        add(
            "CrossSliceOwnershipMismatch",
            "$.cross_slice_definition_ownership",
            "exact local/forward-shared/prior-reuse definition lists changed",
        )
        cross_slice_ownership_mismatch = True
    if (
        closure.get("optional_nullable_matrix")
        != expected_optional_nullable_matrix
        and not optional_nullable_mapping_mismatch
    ):
        add(
            "OptionalNullableMappingMismatch",
            "$.optional_nullable_matrix",
            "stored OptionalNullable matrix disagrees with property records",
        )
        optional_nullable_mapping_mismatch = True

    closure_counts_value = closure.get("counts")
    closure_counts = closure_counts_value if isinstance(closure_counts_value, Mapping) else {}
    expected_closure_summary = {
        "seed_definitions": len(seeds),
        "reachable_named_definitions": len(definitions),
        "definition_shapes": dict(sorted(definition_shapes.items())),
        "definition_dispositions": dict(sorted(definition_dispositions.items())),
        "definition_batches": dict(sorted(definition_batches.items())),
        "schema_path_dispositions": len(schema_paths),
        "schema_path_disposition_kinds": dict(
            sorted(schema_path_dispositions.items())
        ),
        "property_declarations": schema_path_kinds["property"],
        "property_shapes": dict(sorted(property_shapes.items())),
        "array_element_paths": schema_path_kinds["array_element"],
        "map_value_paths": schema_path_kinds["map_value"],
        "protocol_defined_opaque_json_paths": len(actual_opaque_paths),
    }
    if any(
        closure_counts.get(field) != value
        for field, value in expected_closure_summary.items()
    ):
        add(
            "MissingClosureDisposition",
            "$.counts",
            "closure count summary disagrees with seed, definition, or schema-path records",
        )
    if (
        closure_counts.get("definition_directions")
        != dict(sorted(definition_directions.items()))
        and not definition_direction_mismatch
    ):
        add(
            "DirectionalityMismatch",
            "$.counts.definition_directions",
            "definition direction summary disagrees with definition records",
        )
        definition_direction_mismatch = True
    if (
        closure_counts.get("schema_path_directions")
        != dict(sorted(schema_path_directions.items()))
        or closure_counts.get("schema_path_kind_directions")
        != normalized_kind_directions
    ) and not path_direction_mismatch:
        add(
            "DirectionalityMismatch",
            "$.counts.schema_path_directions",
            "schema-path direction summary disagrees with path records",
        )
        path_direction_mismatch = True
    if (
        closure_counts.get("cross_slice_ownership")
        != dict(sorted(cross_slice_ownership_counts.items()))
        and not cross_slice_ownership_mismatch
    ):
        add(
            "CrossSliceOwnershipMismatch",
            "$.counts.cross_slice_ownership",
            "cross-slice ownership summary disagrees with definition records",
        )
        cross_slice_ownership_mismatch = True
    if (
        closure_counts.get("definition_direction_ownership_sha256")
        != definition_direction_ownership_sha256
        and not (
            cross_slice_ownership_mismatch
            or definition_direction_mismatch
            or missing_disposition
        )
    ):
        add(
            "CrossSliceOwnershipMismatch",
            "$.counts.definition_direction_ownership_sha256",
            "exact definition direction/cross-slice ownership ratchet changed",
        )
        cross_slice_ownership_mismatch = True
    if (
        closure_counts.get("definition_cpp_mapping_sha256")
        != definition_cpp_mapping_sha256
        and not (
            missing_disposition
            or missing_cpp_owner
            or bool(conflicting_owners)
        )
    ):
        add(
            "CppDefinitionMappingMismatch",
            "$.counts.definition_cpp_mapping_sha256",
            "exact definition C++ ownership ratchet changed",
        )
    if (
        closure_counts.get("property_presence_models")
        != dict(sorted(property_presence_models.items()))
        and not optional_nullable_mapping_mismatch
    ):
        add(
            "OptionalNullableMappingMismatch",
            "$.counts.property_presence_models",
            "property wrapper summary disagrees with path records",
        )
        optional_nullable_mapping_mismatch = True
    if (
        closure_counts.get("schema_path_mapping_sha256")
        != schema_path_mapping_sha256
        or cpp_path_mapping_mismatch
    ) and not (
        path_missing_mapping
        or path_direction_mismatch
        or optional_nullable_mapping_mismatch
        or strong_identifier_mapping_mismatch
    ):
        add(
            "CppPathMappingMismatch",
            "$.counts.schema_path_mapping_sha256",
            "exact schema-path C++ owner/member/type ratchet changed",
        )

    closure_graph_value = closure.get("batch_dependency_graph")
    closure_graph: dict[str, set[str]] = {}
    if isinstance(closure_graph_value, Mapping):
        for batch, dependencies in closure_graph_value.items():
            if isinstance(batch, str) and isinstance(dependencies, list):
                closure_graph[batch] = {
                    dependency for dependency in dependencies if isinstance(dependency, str)
                }
    if set(closure_graph) != {"B2", "B3", "B4", "B5"} or _dependency_cycle(closure_graph):
        add(
            "BatchDependencyCycle",
            "$.batch_dependency_graph",
            "closure batch dependency graph is cyclic or incomplete",
        )
    elif closure_graph != derived_closure_graph:
        add(
            "AssignmentMismatch",
            "$.batch_dependency_graph",
            "closure batch dependency graph disagrees with definition dependencies",
        )

    return sorted(diagnostics)


def validate_generated_reports(
    plan: Mapping[str, Any], closure: Mapping[str, Any]
) -> None:
    diagnostics = report_diagnostics(plan, closure)
    if diagnostics:
        codes = tuple(diagnostic.code for diagnostic in diagnostics)
        message = "; ".join(
            f"{diagnostic.code} at {diagnostic.location}: {diagnostic.message}"
            for diagnostic in diagnostics
        )
        raise AuditError(message, diagnostics[0].code, codes)


def write_or_check(path: Path, document: Mapping[str, Any], check: bool) -> None:
    generated = canonical_json(document)
    if check:
        try:
            committed = path.read_text(encoding="utf-8")
        except OSError as error:
            raise AuditError(
                f"missing generated A1.1 audit {path}: {error}",
                "StaleGeneratedAudit",
            ) from error
        if committed != generated:
            fail(f"generated A1.1 audit is stale: {path}", "StaleGeneratedAudit")
    else:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(generated, encoding="utf-8")


def parser() -> argparse.ArgumentParser:
    repo_root = Path(__file__).resolve().parents[2]
    evidence_root = repo_root / "tools/codex/app-server-evidence/0.144.6"
    result = argparse.ArgumentParser(description=__doc__)
    result.add_argument("mode", choices=("freeze-start-state", "generate", "check"))
    result.add_argument("--repo-root", type=Path, default=repo_root)
    result.add_argument(
        "--manifest",
        type=Path,
        default=repo_root / "tools/codex/app-server-surface/0.144.6.json",
    )
    result.add_argument(
        "--registry",
        type=Path,
        default=repo_root / "src/ai/openai/codex/detail/ProtocolSurfaceRegistryData.inc",
    )
    result.add_argument(
        "--schema-root",
        type=Path,
        default=repo_root / "tools/codex/app-server-schema/0.144.6",
    )
    result.add_argument("--assignments", type=Path, default=evidence_root / "module-slice-assignment.json")
    result.add_argument("--reachability", type=Path, default=evidence_root / "nested-reachability.json")
    result.add_argument("--contracts", type=Path, default=evidence_root / "operation-contracts.json")
    result.add_argument(
        "--schema-completeness",
        type=Path,
        default=evidence_root / "schema-completeness-evidence.json",
    )
    result.add_argument("--fixture-coverage", type=Path, default=evidence_root / "fixture-coverage.json")
    result.add_argument(
        "--fixture-index",
        type=Path,
        default=repo_root / "tools/codex/app-server-fixtures/0.144.6/index.json",
    )
    result.add_argument(
        "--draft07-validator",
        type=Path,
        default=repo_root / "tools/codex/draft07.py",
    )
    result.add_argument(
        "--start-state",
        type=Path,
        default=evidence_root / "a1-1-start-state.json",
    )
    result.add_argument(
        "--base-sha",
        help="verified master SHA; required only by freeze-start-state",
    )
    result.add_argument(
        "--force-start-state",
        action="store_true",
        help="explicitly replace an existing frozen start state",
    )
    result.add_argument(
        "--plan-output",
        type=Path,
        default=evidence_root / "a1-1-implementation-plan.json",
    )
    result.add_argument(
        "--closure-output",
        type=Path,
        default=evidence_root / "a1-1-type-closure.json",
    )
    return result


def main(argv: Sequence[str] | None = None) -> int:
    arguments = parser().parse_args(argv)
    for attribute in (
        "repo_root",
        "manifest",
        "registry",
        "schema_root",
        "assignments",
        "reachability",
        "contracts",
        "schema_completeness",
        "fixture_coverage",
        "fixture_index",
        "draft07_validator",
        "start_state",
        "plan_output",
        "closure_output",
    ):
        setattr(arguments, attribute, getattr(arguments, attribute).resolve())
    if arguments.mode == "freeze-start-state":
        freeze_start_state(arguments)
        return 0
    plan, closure = build_reports(arguments)
    check = arguments.mode == "check"
    write_or_check(arguments.plan_output, plan, check)
    write_or_check(arguments.closure_output, closure, check)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AuditError as error:
        print(f"app-server-a1-1: error: {error.code}: {error}", file=sys.stderr)
        raise SystemExit(1)
