#!/usr/bin/env python3
"""Generate and guard offline Codex App Server schema evidence fixtures.

Inputs are limited to the vendored schema tree, the pinned A0 surface manifest,
the reviewed operation-contract evidence, and deterministic rules in this
file, including the exact existing-34 fixture scope and frozen slice routing.
Production C++ decoders and test ratchet headers are deliberately not
inspected.

Generated files are review evidence, not implementation or completeness
claims.  Use ``generate`` to refresh them, ``check`` to compare regeneration
byte-for-byte, and ``validate`` to revalidate the committed corpus and all
required-field/wrong-type mutations.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import importlib.util
import json
import re
import shutil
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from types import ModuleType
from typing import Any, Iterable, Iterator, Mapping, MutableMapping, Sequence

sys.dont_write_bytecode = True


FORMAT_VERSION = 1
CODEX_VERSION = "codex-cli 0.144.6"
RULES_VERSION = 2

CLIENT_REQUEST = "client_request"
CLIENT_NOTIFICATION = "client_notification"
SERVER_NOTIFICATION = "server_notification"
SERVER_REQUEST = "server_request"
ITEM_DISCRIMINATOR = "item_discriminator"
TAGGED_UNION_DISCRIMINATOR = "tagged_union_discriminator"

METHOD_SCHEMA_FILES = {
    CLIENT_REQUEST: "ClientRequest.json",
    CLIENT_NOTIFICATION: "ClientNotification.json",
    SERVER_NOTIFICATION: "ServerNotification.json",
    SERVER_REQUEST: "ServerRequest.json",
}

# Reviewed deterministic fixture scope for the A0 runtime floor. This is input
# to fixture synthesis, not an implementation claim. The focused tool test
# independently compares it with the non-generated C++ ratchet so neither can
# drift silently.
EXISTING_TYPED_FIXTURE_IDENTITIES = (
    (CLIENT_NOTIFICATION, "ClientNotification", "method", "initialized"),
    (CLIENT_REQUEST, "ClientRequest", "method", "initialize"),
    (CLIENT_REQUEST, "ClientRequest", "method", "thread/list"),
    (CLIENT_REQUEST, "ClientRequest", "method", "thread/read"),
    (CLIENT_REQUEST, "ClientRequest", "method", "thread/resume"),
    (CLIENT_REQUEST, "ClientRequest", "method", "thread/start"),
    (CLIENT_REQUEST, "ClientRequest", "method", "turn/interrupt"),
    (CLIENT_REQUEST, "ClientRequest", "method", "turn/start"),
    (SERVER_NOTIFICATION, "ServerNotification", "method", "error"),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "item/agentMessage/delta",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "item/commandExecution/outputDelta",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "item/completed",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "item/fileChange/patchUpdated",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "item/reasoning/summaryTextDelta",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "item/reasoning/textDelta",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "item/started",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "model/rerouted",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "thread/started",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "thread/status/changed",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "thread/tokenUsage/updated",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "turn/completed",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "turn/started",
    ),
    (
        SERVER_REQUEST,
        "ServerRequest",
        "method",
        "account/chatgptAuthTokens/refresh",
    ),
    (
        SERVER_REQUEST,
        "ServerRequest",
        "method",
        "item/commandExecution/requestApproval",
    ),
    (
        SERVER_REQUEST,
        "ServerRequest",
        "method",
        "item/fileChange/requestApproval",
    ),
    (
        SERVER_REQUEST,
        "ServerRequest",
        "method",
        "item/tool/requestUserInput",
    ),
    (ITEM_DISCRIMINATOR, "ThreadItem", "type", "agentMessage"),
    (ITEM_DISCRIMINATOR, "ThreadItem", "type", "commandExecution"),
    (ITEM_DISCRIMINATOR, "ThreadItem", "type", "dynamicToolCall"),
    (ITEM_DISCRIMINATOR, "ThreadItem", "type", "fileChange"),
    (ITEM_DISCRIMINATOR, "ThreadItem", "type", "mcpToolCall"),
    (ITEM_DISCRIMINATOR, "ThreadItem", "type", "reasoning"),
    (ITEM_DISCRIMINATOR, "ThreadItem", "type", "userMessage"),
    (ITEM_DISCRIMINATOR, "ThreadItem", "type", "webSearch"),
)

CODEX_ERROR_INFO_IDENTITIES = (
    "activeTurnNotSteerable",
    "badRequest",
    "contextWindowExceeded",
    "cyberPolicy",
    "httpConnectionFailed",
    "internalServerError",
    "other",
    "responseStreamConnectionFailed",
    "responseStreamDisconnected",
    "responseTooManyFailedAttempts",
    "sandboxError",
    "serverOverloaded",
    "sessionBudgetExceeded",
    "threadRollbackFailed",
    "unauthorized",
    "usageLimitExceeded",
)

CODEX_ERROR_INFO_HTTP_IDENTITIES = (
    "httpConnectionFailed",
    "responseStreamConnectionFailed",
    "responseStreamDisconnected",
    "responseTooManyFailedAttempts",
)

# Commit B2 owns the exact A1.1 SharedCommon tagged-union batch.  The keys
# themselves are derived from module-slice-assignment evidence below; these
# reviewed counts and directions are only bidirectional audit ratchets for the
# fixture plan, not a runtime disposition or dispatch registry.
B2_SHARED_COMMON_FAMILY_COUNTS = {
    "AskForApproval": 4,
    "CommandAction": 4,
    "DynamicToolCallOutputContentItem": 2,
    "PatchChangeKind": 3,
    "SandboxPolicy": 4,
    "UserInput": 5,
    "WebSearchAction": 4,
}
B2_SHARED_COMMON_DIRECTIONS = {
    "AskForApproval": ("Decode", "Encode"),
    "CommandAction": ("Decode",),
    "DynamicToolCallOutputContentItem": ("Decode",),
    "PatchChangeKind": ("Decode",),
    "SandboxPolicy": ("Decode", "Encode"),
    "UserInput": ("Decode", "Encode"),
    "WebSearchAction": ("Decode",),
}
B2_OPEN_STRING_ENUMS = {
    "ImageDetail": ("auto", "low", "high", "original"),
    "NetworkAccess": ("restricted", "enabled"),
}

# Commit B3 owns both directionally distinct item families and the nested
# tagged unions that are reachable only through those item roots.  Assignment
# evidence chooses the exact keys; these family/value maps are bidirectional
# ratchets against the vendored schema pin.
B3_ITEM_FAMILY_IDENTITIES = {
    "AgentMessageInputContent": (
        "encrypted_content",
        "input_text",
    ),
    "ContentItem": (
        "input_image",
        "input_text",
        "output_text",
    ),
    "FunctionCallOutputContentItem": (
        "encrypted_content",
        "input_image",
        "input_text",
    ),
    "LocalShellAction": ("exec",),
    "ReasoningItemContent": (
        "reasoning_text",
        "text",
    ),
    "ReasoningItemReasoningSummary": ("summary_text",),
    "ResponseItem": (
        "agent_message",
        "compaction",
        "compaction_trigger",
        "context_compaction",
        "custom_tool_call",
        "custom_tool_call_output",
        "function_call",
        "function_call_output",
        "image_generation_call",
        "local_shell_call",
        "message",
        "other",
        "reasoning",
        "tool_search_call",
        "tool_search_output",
        "web_search_call",
    ),
    "ResponsesApiWebSearchAction": (
        "find_in_page",
        "open_page",
        "other",
        "search",
    ),
    "ThreadItem": (
        "agentMessage",
        "collabAgentToolCall",
        "commandExecution",
        "contextCompaction",
        "dynamicToolCall",
        "enteredReviewMode",
        "exitedReviewMode",
        "fileChange",
        "hookPrompt",
        "imageGeneration",
        "imageView",
        "mcpToolCall",
        "plan",
        "reasoning",
        "sleep",
        "subAgentActivity",
        "userMessage",
        "webSearch",
    ),
}
B3_ITEM_FAMILY_COUNTS = {
    domain: len(identities)
    for domain, identities in B3_ITEM_FAMILY_IDENTITIES.items()
}
B3_OPEN_STRING_ENUMS = {
    "CollabAgentStatus": (
        "pendingInit",
        "running",
        "interrupted",
        "completed",
        "errored",
        "shutdown",
        "notFound",
    ),
    "CollabAgentTool": (
        "spawnAgent",
        "sendInput",
        "resumeAgent",
        "wait",
        "closeAgent",
    ),
    "CollabAgentToolCallStatus": (
        "inProgress",
        "completed",
        "failed",
    ),
    "CommandExecutionSource": (
        "agent",
        "userShell",
        "unifiedExecStartup",
        "unifiedExecInteraction",
    ),
    "CommandExecutionStatus": (
        "inProgress",
        "completed",
        "failed",
        "declined",
    ),
    "DynamicToolCallStatus": (
        "inProgress",
        "completed",
        "failed",
    ),
    "LocalShellStatus": (
        "completed",
        "in_progress",
        "incomplete",
    ),
    "McpToolCallStatus": (
        "inProgress",
        "completed",
        "failed",
    ),
    "PatchApplyStatus": (
        "inProgress",
        "completed",
        "failed",
        "declined",
    ),
    "SubAgentActivityKind": (
        "started",
        "interacted",
        "interrupted",
    ),
}

# Commit B4 owns the exact stable operation roots and the three union families
# first completed by their aggregate codecs.  Assignment evidence selects the
# exact keys; the reviewed values below ratchet the pinned discriminator and
# open-string sets without becoming a runtime dispatch table.
B4_CLIENT_REQUEST_METHODS = frozenset(
    {
        "thread/archive",
        "thread/compact/start",
        "thread/delete",
        "thread/fork",
        "thread/goal/clear",
        "thread/goal/get",
        "thread/goal/set",
        "thread/inject_items",
        "thread/list",
        "thread/loaded/list",
        "thread/metadata/update",
        "thread/name/set",
        "thread/read",
        "thread/resume",
        "thread/rollback",
        "thread/shellCommand",
        "thread/start",
        "thread/unarchive",
        "thread/unsubscribe",
        "turn/interrupt",
        "turn/start",
        "turn/steer",
    }
)
B4_OPERATION_UNION_FAMILY_IDENTITIES = {
    "SessionSource": (
        "appServer",
        "cli",
        "custom",
        "exec",
        "subAgent",
        "unknown",
        "vscode",
    ),
    "SubAgentSource": (
        "compact",
        "memory_consolidation",
        "other",
        "review",
        "thread_spawn",
    ),
    "ThreadStatus": (
        "active",
        "idle",
        "notLoaded",
        "systemError",
    ),
}
B4_OPERATION_UNION_DIRECTIONS = {
    domain: ("Decode",)
    for domain in B4_OPERATION_UNION_FAMILY_IDENTITIES
}
B4_OPEN_STRING_ENUMS = {
    "ApprovalsReviewer": ("user", "auto_review", "guardian_subagent"),
    "Personality": ("none", "friendly", "pragmatic"),
    "SandboxMode": (
        "read-only",
        "workspace-write",
        "danger-full-access",
    ),
    "SortDirection": ("asc", "desc"),
    "ThreadActiveFlag": ("waitingOnApproval", "waitingOnUserInput"),
    "ThreadGoalStatus": (
        "active",
        "paused",
        "blocked",
        "usageLimited",
        "budgetLimited",
        "complete",
    ),
    "ThreadSortKey": ("created_at", "updated_at", "recency_at"),
    "ThreadSourceKind": (
        "cli",
        "vscode",
        "exec",
        "appServer",
        "subAgent",
        "subAgentReview",
        "subAgentCompact",
        "subAgentThreadSpawn",
        "subAgentOther",
        "unknown",
    ),
    "ThreadStartSource": ("startup", "clear"),
    "ThreadUnsubscribeStatus": ("notLoaded", "notSubscribed", "unsubscribed"),
    "TurnStatus": ("completed", "interrupted", "failed", "inProgress"),
}
B4_HELPER_STRING_UNIONS = {
    "ReasoningSummary": (
        ("auto", "concise", "detailed", "none"),
        ("Decode", "Encode"),
    ),
    "TurnItemsView": (
        ("notLoaded", "summary", "full"),
        ("Decode",),
    ),
}

# Commit B5 owns the exact stable inbound notification family. Assignment
# evidence remains authoritative; this reviewed method set and the three
# open-string value sets are bidirectional pin ratchets for independent
# fixture generation only.
B5_SERVER_NOTIFICATION_METHODS = frozenset(
    {
        "item/agentMessage/delta",
        "item/commandExecution/outputDelta",
        "item/commandExecution/terminalInteraction",
        "item/completed",
        "item/fileChange/outputDelta",
        "item/fileChange/patchUpdated",
        "item/mcpToolCall/progress",
        "item/plan/delta",
        "item/reasoning/summaryPartAdded",
        "item/reasoning/summaryTextDelta",
        "item/reasoning/textDelta",
        "item/started",
        "thread/archived",
        "thread/closed",
        "thread/compacted",
        "thread/deleted",
        "thread/goal/cleared",
        "thread/goal/updated",
        "thread/name/updated",
        "thread/realtime/closed",
        "thread/realtime/error",
        "thread/realtime/itemAdded",
        "thread/realtime/outputAudio/delta",
        "thread/realtime/sdp",
        "thread/realtime/started",
        "thread/realtime/transcript/delta",
        "thread/realtime/transcript/done",
        "thread/settings/updated",
        "thread/started",
        "thread/status/changed",
        "thread/tokenUsage/updated",
        "thread/unarchived",
        "turn/completed",
        "turn/diff/updated",
        "turn/moderationMetadata",
        "turn/plan/updated",
        "turn/started",
    }
)
B5_OPEN_STRING_ENUMS = {
    "ModeKind": ("plan", "default"),
    "RealtimeConversationVersion": ("v1", "v2"),
    "TurnPlanStepStatus": ("pending", "inProgress", "completed"),
}
B5_REASONING_EFFORT_FIXTURE_VALUE = "medium"
SLICE_ORDER = {"A1.0": 0, "A1.1": 1, "A1.2": 2, "A1.3": 3, "A1.4": 4}
SLICE_MODULES = {
    "A1.0": "Common",
    "A1.1": "ThreadsTurnsSessions",
    "A1.2": "AccountsModelsConfiguration",
    "A1.3": "CommandsFilesystemReviewsApprovals",
    "A1.4": "IntegrationsAndLongTail",
}

# These are the reviewed stable public roots assigned to the A1.4 long-tail
# slice. Keep the sets category-specific and exact: adding a new stable public
# root to the pinned manifest must fail until its slice ownership is reviewed.
A1_4_PUBLIC_ROOTS = {
    CLIENT_REQUEST: frozenset(
        {
            "app/list",
            "externalAgentConfig/detect",
            "externalAgentConfig/import",
            "externalAgentConfig/import/readHistories",
            "feedback/upload",
            "hooks/list",
            "marketplace/add",
            "marketplace/remove",
            "marketplace/upgrade",
            "mcpServer/oauth/login",
            "mcpServer/resource/read",
            "mcpServer/tool/call",
            "mcpServerStatus/list",
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
            "windowsSandbox/readiness",
            "windowsSandbox/setupStart",
        }
    ),
    SERVER_NOTIFICATION: frozenset(
        {
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
    ),
    SERVER_REQUEST: frozenset(
        {
            "attestation/generate",
            "item/tool/call",
            "item/tool/requestUserInput",
            "mcpServer/elicitation/request",
        }
    ),
}


class FixtureError(RuntimeError):
    pass


def load_draft07(path: Path) -> ModuleType:
    specification = importlib.util.spec_from_file_location(
        "snodec_codex_draft07_fixture_tool", path
    )
    if specification is None or specification.loader is None:
        raise FixtureError(f"unable to load Draft-07 validator: {path}")
    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


def load_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise FixtureError(f"unable to load JSON artifact {path}: {error}") from error


def encoded_json(value: Any) -> bytes:
    return (
        json.dumps(
            value,
            ensure_ascii=False,
            allow_nan=False,
            indent=2,
            sort_keys=True,
        )
        + "\n"
    ).encode("utf-8")


def canonical_json(value: Any) -> str:
    return json.dumps(
        value,
        ensure_ascii=False,
        allow_nan=False,
        separators=(",", ":"),
        sort_keys=True,
    )


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: Path) -> str:
    try:
        return sha256_bytes(path.read_bytes())
    except OSError as error:
        raise FixtureError(f"unable to hash {path}: {error}") from error


def compact_generated_record(record: Mapping[str, Any]) -> dict[str, Any]:
    """Compact repetitive supplemental evidence without losing guards.

    The full mutation and schema-path structures remain available while the
    corpus is built and are summarized in the indexed per-root accounting.
    Checked-in supplemental records retain deterministic hashes and scalar
    counts so validation can recompute the complete structures offline.
    """

    result = copy.deepcopy(dict(record))
    role = str(result.get("role", ""))
    if not (
        role.startswith("operation_")
        or role.startswith("notification_")
    ):
        return result
    validation = result.get("validation")
    if isinstance(validation, dict):
        mutation = {
            key: value
            for key, value in validation.items()
            if key not in {"independent", "one_of_branch_indices"}
        }
        result["validation"] = {
            "independent": True,
            "one_of_branch_indices": validation.get(
                "one_of_branch_indices", []
            ),
            "compact_mutation_evidence": True,
            "mutation_evidence_sha256": sha256_bytes(
                encoded_json(mutation)
            ),
            "mutation_counts": {
                key: value
                for key, value in mutation.items()
                if isinstance(value, int) and not isinstance(value, bool)
            },
        }
    schema_coverage = result.get("schema_fixture_coverage")
    if isinstance(schema_coverage, dict):
        result["schema_fixture_coverage"] = {
            "compact_schema_fixture_coverage": True,
            "schema_fixture_coverage_sha256": sha256_bytes(
                encoded_json(schema_coverage)
            ),
            "optional_property_schema_paths_omitted": (
                schema_coverage.get(
                    "optional_property_schema_paths_omitted", []
                )
            ),
            "directions_exercised": schema_coverage.get(
                "directions_exercised", []
            ),
        }
    return result


def slug(value: str) -> str:
    rendered = re.sub(r"[^A-Za-z0-9._-]+", "-", value).strip("-").lower()
    return rendered or "root"


def pointer_token(value: str) -> str:
    return value.replace("~", "~0").replace("/", "~1")


def pointer_child(path: str, token: str | int) -> str:
    return f"{path}/{pointer_token(str(token))}"


def instance_child(path: tuple[str | int, ...], token: str | int) -> tuple[str | int, ...]:
    return (*path, token)


def json_path(path: Sequence[str | int]) -> str:
    result = "$"
    for token in path:
        result += f"/{pointer_token(str(token))}"
    return result


def get_instance_path(instance: Any, path: Sequence[str | int]) -> Any:
    node = instance
    for token in path:
        if isinstance(token, int):
            node = node[token]
        else:
            node = node[token]
    return node


def get_parent_path(instance: Any, path: Sequence[str | int]) -> tuple[Any, str | int]:
    if not path:
        raise FixtureError("root instance has no parent")
    return get_instance_path(instance, path[:-1]), path[-1]


@dataclass(frozen=True, order=True)
class SurfaceKey:
    category: str
    domain: str
    discriminator_field: str
    name: str

    @classmethod
    def from_manifest(cls, entry: Mapping[str, Any]) -> "SurfaceKey":
        return cls(
            str(entry["category"]),
            str(entry["domain"]),
            str(entry["discriminator_field"]),
            str(entry["name"]),
        )

    @classmethod
    def from_contract(cls, value: Mapping[str, Any]) -> "SurfaceKey":
        field = value.get("discriminator_field", value.get("field"))
        if not isinstance(field, str):
            raise FixtureError("operation contract surface_key lacks field")
        return cls(
            str(value["category"]),
            str(value["domain"]),
            field,
            str(value["name"]),
        )

    def to_json(self) -> dict[str, str]:
        return {
            "category": self.category,
            "domain": self.domain,
            "discriminator_field": self.discriminator_field,
            "name": self.name,
        }

    def compact(self) -> str:
        return (
            f"{self.category}:{self.domain}:"
            f"{self.discriminator_field}:{self.name}"
        )


def derive_b2_shared_common_keys(
    assignments: Mapping[SurfaceKey, Mapping[str, Any]],
) -> tuple[SurfaceKey, ...]:
    keys = tuple(
        sorted(
            key
            for key, assignment in assignments.items()
            if assignment["a1_slice"] == "A1.1"
            and assignment["classification"] == "SharedCommon"
        )
    )
    family_counts: dict[str, int] = {}
    for key in keys:
        if key.category != TAGGED_UNION_DISCRIMINATOR:
            raise FixtureError(
                "A1.1 SharedCommon batch contains a non-union identity: "
                f"{key.compact()}"
            )
        family_counts[key.domain] = family_counts.get(key.domain, 0) + 1
    if (
        len(keys) != 26
        or family_counts != B2_SHARED_COMMON_FAMILY_COUNTS
        or set(family_counts) != set(B2_SHARED_COMMON_DIRECTIONS)
    ):
        raise FixtureError(
            "A1.1 SharedCommon assignment mismatch: "
            f"count={len(keys)} "
            f"families={dict(sorted(family_counts.items()))}"
        )
    return keys


def derive_b3_item_keys(
    assignments: Mapping[SurfaceKey, Mapping[str, Any]],
) -> tuple[SurfaceKey, ...]:
    keys = tuple(
        sorted(
            key
            for key, assignment in assignments.items()
            if assignment["a1_slice"] == "A1.1"
            and (
                key.category == ITEM_DISCRIMINATOR
                or assignment["classification"] == "RootOwnedNestedUnion"
            )
        )
    )
    family_counts: dict[str, int] = {}
    family_identities: dict[str, set[str]] = {}
    for key in keys:
        if key.category not in {
            ITEM_DISCRIMINATOR,
            TAGGED_UNION_DISCRIMINATOR,
        }:
            raise FixtureError(
                "A1.1 item batch contains a non-union identity: "
                f"{key.compact()}"
            )
        if key.discriminator_field != "type":
            raise FixtureError(
                "A1.1 item batch contains a non-type discriminator: "
                f"{key.compact()}"
            )
        family_counts[key.domain] = family_counts.get(key.domain, 0) + 1
        family_identities.setdefault(key.domain, set()).add(key.name)
    expected_identities = {
        domain: set(identities)
        for domain, identities in B3_ITEM_FAMILY_IDENTITIES.items()
    }
    if (
        len(keys) != 50
        or family_counts != B3_ITEM_FAMILY_COUNTS
        or family_identities != expected_identities
    ):
        raise FixtureError(
            "A1.1 item assignment mismatch: "
            f"count={len(keys)} "
            f"families={dict(sorted(family_identities.items()))}"
        )
    return keys


def derive_b4_operation_keys(
    assignments: Mapping[SurfaceKey, Mapping[str, Any]],
) -> tuple[SurfaceKey, ...]:
    keys = tuple(
        sorted(
            key
            for key, assignment in assignments.items()
            if assignment["a1_slice"] == "A1.1"
            and key.category == CLIENT_REQUEST
        )
    )
    if (
        len(keys) != 22
        or {key.name for key in keys} != B4_CLIENT_REQUEST_METHODS
        or any(
            key.domain != "ClientRequest"
            or key.discriminator_field != "method"
            for key in keys
        )
    ):
        raise FixtureError(
            "A1.1 operation assignment mismatch: "
            f"count={len(keys)} methods={sorted(key.name for key in keys)}"
        )
    return keys


def derive_b4_operation_union_keys(
    assignments: Mapping[SurfaceKey, Mapping[str, Any]],
) -> tuple[SurfaceKey, ...]:
    expected = {
        (domain, name)
        for domain, names in B4_OPERATION_UNION_FAMILY_IDENTITIES.items()
        for name in names
    }
    keys = tuple(
        sorted(
            key
            for key, assignment in assignments.items()
            if assignment["a1_slice"] == "A1.1"
            and assignment["classification"] == "SharedWithinSlice"
        )
    )
    if (
        len(keys) != 16
        or {(key.domain, key.name) for key in keys} != expected
        or any(key.category != TAGGED_UNION_DISCRIMINATOR for key in keys)
    ):
        raise FixtureError(
            "A1.1 operation-union assignment mismatch: "
            f"count={len(keys)} identities="
            f"{sorted((key.domain, key.name) for key in keys)}"
        )
    return keys


def derive_b5_notification_keys(
    assignments: Mapping[SurfaceKey, Mapping[str, Any]],
) -> tuple[SurfaceKey, ...]:
    keys = tuple(
        sorted(
            key
            for key, assignment in assignments.items()
            if assignment["a1_slice"] == "A1.1"
            and key.category == SERVER_NOTIFICATION
        )
    )
    if (
        len(keys) != 37
        or {key.name for key in keys} != B5_SERVER_NOTIFICATION_METHODS
        or any(
            key.domain != "ServerNotification"
            or key.discriminator_field != "method"
            for key in keys
        )
    ):
        raise FixtureError(
            "A1.1 notification assignment mismatch: "
            f"count={len(keys)} methods={sorted(key.name for key in keys)}"
        )
    return keys


def normalize_b5_notification_sample(
    key: SurfaceKey, value: Any
) -> None:
    """Select reviewed ordinary values for unconstrained protocol strings.

    ReasoningEffort is intentionally open in the pinned schema, so generic
    synthesis produces a deterministic future-looking string. The explicit
    forward-compatibility regressions live elsewhere in the corpus; ordinary
    thread-settings notification fixtures use one known upstream value. This
    rule depends only on the reviewed protocol field paths and never inspects
    a production decoder.
    """

    if key.name != "thread/settings/updated":
        return
    if not isinstance(value, dict):
        raise FixtureError(
            "B5 thread/settings/updated sample is not an object"
        )
    params = value.get("params")
    if not isinstance(params, dict):
        raise FixtureError(
            "B5 thread/settings/updated sample lacks params"
        )
    thread_settings = params.get("threadSettings")
    if not isinstance(thread_settings, dict):
        raise FixtureError(
            "B5 thread/settings/updated sample lacks threadSettings"
        )
    collaboration_mode = thread_settings.get("collaborationMode")
    if not isinstance(collaboration_mode, dict):
        raise FixtureError(
            "B5 thread/settings/updated sample lacks collaborationMode"
        )
    collaboration_settings = collaboration_mode.get("settings")
    if not isinstance(collaboration_settings, dict):
        raise FixtureError(
            "B5 thread/settings/updated sample lacks collaboration settings"
        )
    if not isinstance(thread_settings.get("effort"), str) or not isinstance(
        collaboration_settings.get("reasoning_effort"), str
    ):
        raise FixtureError(
            "B5 thread/settings/updated ReasoningEffort paths changed"
        )
    thread_settings["effort"] = B5_REASONING_EFFORT_FIXTURE_VALUE
    collaboration_settings[
        "reasoning_effort"
    ] = B5_REASONING_EFFORT_FIXTURE_VALUE


@dataclass(frozen=True)
class SchemaTarget:
    document: Any
    schema: Any
    schema_path: str
    relative_file: str | None
    file_sha256: str | None
    inline_schema: Any | None = None

    def to_json(self) -> dict[str, Any]:
        if self.inline_schema is not None:
            return {
                "inline": self.inline_schema,
                "pointer": self.schema_path,
            }
        assert self.relative_file is not None and self.file_sha256 is not None
        return {
            "file": self.relative_file,
            "pointer": self.schema_path,
            "sha256": self.file_sha256,
        }


class SchemaCatalog:
    def __init__(self, schema_root: Path, draft07: ModuleType):
        self.root = schema_root.resolve()
        self.stable_root = (self.root / "stable").resolve()
        self.draft07 = draft07
        if not self.stable_root.is_dir():
            raise FixtureError(f"stable schema tree is missing: {self.stable_root}")
        self._documents: dict[Path, Any] = {}
        self._validators: dict[Path, Any] = {}
        self._type_paths: dict[str, Path] = {}
        for path in sorted(self.stable_root.rglob("*.json")):
            previous = self._type_paths.setdefault(path.stem, path)
            if previous != path:
                raise FixtureError(
                    f"duplicate stable schema basename {path.stem}: "
                    f"{previous} and {path}"
                )

    def load(self, path: Path) -> Any:
        path = path.resolve()
        try:
            path.relative_to(self.root)
        except ValueError as error:
            raise FixtureError(f"schema path escapes vendored root: {path}") from error
        if path not in self._documents:
            self._documents[path] = load_json(path)
        return self._documents[path]

    def validator(self, path: Path) -> Any:
        path = path.resolve()
        if path not in self._validators:
            self._validators[path] = self.draft07.Validator(self.load(path))
        return self._validators[path]

    def relative(self, path: Path) -> str:
        return path.resolve().relative_to(self.root).as_posix()

    def standalone(self, type_identity: str) -> SchemaTarget:
        if type_identity == "Unit":
            schema = {"type": "null"}
            return SchemaTarget(schema, schema, "#", None, None, schema)
        path = self._type_paths.get(type_identity)
        if path is None:
            raise FixtureError(
                f"no unique stable standalone schema root for {type_identity!r}"
            )
        document = self.load(path)
        title = document.get("title")
        if title != type_identity:
            raise FixtureError(
                f"standalone schema title mismatch for {type_identity}: "
                f"{self.relative(path)} declares {title!r}"
            )
        self.validator(path)
        return SchemaTarget(
            document,
            document,
            "#",
            self.relative(path),
            sha256_file(path),
        )

    def method_target(
        self, category: str, method: str
    ) -> tuple[SchemaTarget, int, Any]:
        relative = METHOD_SCHEMA_FILES.get(category)
        if relative is None:
            raise FixtureError(f"{category} is not a method category")
        path = self.stable_root / relative
        document = self.load(path)
        validator = self.validator(path)
        matches: list[tuple[int, Any]] = []
        for index, branch in enumerate(document.get("oneOf", [])):
            values = (
                branch.get("properties", {})
                .get("method", {})
                .get("enum", [])
            )
            if values == [method]:
                matches.append((index, branch))
        if len(matches) != 1:
            raise FixtureError(
                f"{category}:{method} selects {len(matches)} branches in {relative}"
            )
        index, branch = matches[0]
        target = SchemaTarget(
            document,
            document,
            "#",
            self.relative(path),
            sha256_file(path),
        )
        return target, index, branch

    def union_target(self, domain: str) -> SchemaTarget:
        path = self.stable_root / "codex_app_server_protocol.v2.schemas.json"
        document = self.load(path)
        definitions = document.get("definitions", {})
        if domain not in definitions:
            raise FixtureError(
                f"stable standalone v2 aggregate lacks union {domain}"
            )
        self.validator(path)
        return SchemaTarget(
            document,
            definitions[domain],
            f"#/definitions/{pointer_token(domain)}",
            self.relative(path),
            sha256_file(path),
        )

    def target_validator(self, target: SchemaTarget) -> Any:
        if target.relative_file is None:
            return self.draft07.Validator(target.document)
        return self.validator(self.root / target.relative_file)

    def resolve(
        self, target: SchemaTarget, schema: Any, schema_path: str
    ) -> tuple[Any, str]:
        validator = self.target_validator(target)
        seen: set[str] = set()
        node = schema
        path = schema_path
        while isinstance(node, dict) and "$ref" in node:
            reference = node["$ref"]
            if reference in seen:
                raise FixtureError(
                    f"direct schema reference cycle while synthesizing {path}"
                )
            seen.add(reference)
            node, path = validator.resolve_reference(
                reference, pointer_child(path, "$ref")
            )
        return node, path


class Synthesizer:
    def __init__(self, catalog: SchemaCatalog):
        self.catalog = catalog

    def sample(
        self,
        target: SchemaTarget,
        schema: Any | None = None,
        schema_path: str | None = None,
        forced_scalar: Any | None = None,
        forced_property: tuple[str, Any] | None = None,
    ) -> Any:
        schema = target.schema if schema is None else schema
        schema_path = target.schema_path if schema_path is None else schema_path
        value = self._synthesize(
            target,
            schema,
            schema_path,
            full=True,
            active=frozenset(),
            depth=0,
        )
        if forced_scalar is not None:
            value = forced_scalar
        if forced_property is not None:
            if not isinstance(value, dict):
                raise FixtureError(
                    f"cannot force property on non-object fixture at {schema_path}"
                )
            value[forced_property[0]] = forced_property[1]
        diagnostics = self.catalog.target_validator(target).validate_subschema(
            value, schema, schema_path
        )
        if diagnostics:
            rendered = [item.to_json() for item in diagnostics[:5]]
            raise FixtureError(
                f"unable to synthesize valid fixture for {schema_path}: {rendered}"
            )
        return value

    def _synthesize(
        self,
        target: SchemaTarget,
        schema: Any,
        schema_path: str,
        full: bool,
        active: frozenset[tuple[str, str]],
        depth: int,
    ) -> Any:
        if depth > 80:
            raise FixtureError(f"schema synthesis depth exceeded at {schema_path}")
        if schema is True:
            return {}
        if schema is False:
            raise FixtureError(f"cannot synthesize false schema at {schema_path}")
        if not isinstance(schema, dict):
            raise FixtureError(f"non-schema during synthesis at {schema_path}")

        if "$ref" in schema:
            validator = self.catalog.target_validator(target)
            referenced, referenced_path = validator.resolve_reference(
                schema["$ref"], pointer_child(schema_path, "$ref")
            )
            marker = (schema_path, referenced_path)
            if marker in active or (referenced_path, referenced_path) in active:
                return self._recursive_base(
                    target,
                    referenced,
                    referenced_path,
                    active,
                    depth + 1,
                )
            return self._synthesize(
                target,
                referenced,
                referenced_path,
                full,
                active | {marker, (referenced_path, referenced_path)},
                depth + 1,
            )

        if "const" in schema:
            return copy.deepcopy(schema["const"])
        if "enum" in schema:
            return copy.deepcopy(schema["enum"][0])

        if "oneOf" in schema:
            validator = self.catalog.target_validator(target)
            outer = self._outer_combinator_value(
                target,
                schema,
                schema_path,
                "oneOf",
                full,
                active,
                depth,
            )
            for index, branch in enumerate(schema["oneOf"]):
                try:
                    candidate = self._synthesize(
                        target,
                        branch,
                        pointer_child(pointer_child(schema_path, "oneOf"), index),
                        full,
                        active,
                        depth + 1,
                    )
                    candidate = combine_values(
                        outer, candidate, schema_path
                    )
                except FixtureError:
                    continue
                diagnostics = validator.validate_subschema(
                    candidate, schema, schema_path
                )
                if not diagnostics:
                    return candidate
            raise FixtureError(
                f"no deterministic exclusive oneOf sample at {schema_path}"
            )

        if "anyOf" in schema:
            validator = self.catalog.target_validator(target)
            outer = self._outer_combinator_value(
                target,
                schema,
                schema_path,
                "anyOf",
                full,
                active,
                depth,
            )
            indexed = list(enumerate(schema["anyOf"]))
            indexed.sort(
                key=lambda item: (
                    self._schema_is_null(target, item[1], schema_path),
                    item[0],
                )
            )
            for index, branch in indexed:
                try:
                    candidate = self._synthesize(
                        target,
                        branch,
                        pointer_child(pointer_child(schema_path, "anyOf"), index),
                        full,
                        active,
                        depth + 1,
                    )
                    candidate = combine_values(
                        outer, candidate, schema_path
                    )
                except FixtureError:
                    continue
                if not validator.validate_subschema(candidate, schema, schema_path):
                    return candidate
            raise FixtureError(f"no deterministic anyOf sample at {schema_path}")

        all_of_values: list[Any] = []
        for index, branch in enumerate(schema.get("allOf", [])):
            all_of_values.append(
                self._synthesize(
                    target,
                    branch,
                    pointer_child(pointer_child(schema_path, "allOf"), index),
                    full,
                    active,
                    depth + 1,
                )
            )

        expected = schema.get("type")
        expected_types = [expected] if isinstance(expected, str) else expected
        if expected_types is None:
            if "properties" in schema or "required" in schema:
                expected_types = ["object"]
            elif "items" in schema:
                expected_types = ["array"]
            elif "minimum" in schema or "maximum" in schema:
                expected_types = ["number"]
            elif (
                "minLength" in schema
                or "maxLength" in schema
                or "pattern" in schema
            ):
                expected_types = ["string"]
            elif all_of_values:
                return self._merge_all_of(all_of_values, schema_path)
            else:
                return {}
        assert isinstance(expected_types, list)
        selected_type = next(
            (value for value in expected_types if value != "null"),
            expected_types[0],
        )

        if selected_type == "null":
            return None
        if selected_type == "boolean":
            return False
        if selected_type in {"integer", "number"}:
            minimum = schema.get("minimum", 0)
            maximum = schema.get("maximum")
            value: int | float = max(minimum, 0)
            if maximum is not None and value > maximum:
                value = maximum
            if selected_type == "integer":
                value = int(math_ceiling(value))
                if maximum is not None and value > maximum:
                    value = int(maximum)
            return value
        if selected_type == "string":
            return self._string_value(schema, schema_path)
        if selected_type == "array":
            items = schema.get("items", True)
            minimum_count = schema.get("minItems", 0)
            desired_count = max(1 if full else 0, minimum_count)
            maximum_count = schema.get("maxItems")
            if maximum_count is not None:
                desired_count = min(desired_count, maximum_count)
            if isinstance(items, list):
                return [
                    self._synthesize(
                        target,
                        child,
                        pointer_child(pointer_child(schema_path, "items"), index),
                        full,
                        active,
                        depth + 1,
                    )
                    for index, child in enumerate(items[:desired_count])
                ]
            return [
                self._synthesize(
                    target,
                    items,
                    pointer_child(schema_path, "items"),
                    full,
                    active,
                    depth + 1,
                )
                for _ in range(desired_count)
            ]
        if selected_type == "object":
            result: dict[str, Any] = {}
            for value in all_of_values:
                if not isinstance(value, dict):
                    raise FixtureError(
                        f"non-object allOf branch in object schema {schema_path}"
                    )
                merge_object(result, value, schema_path)
            properties = schema.get("properties", {})
            required = set(schema.get("required", []))
            for name, child in sorted(properties.items()):
                if full or name in required:
                    result[name] = self._synthesize(
                        target,
                        child,
                        pointer_child(
                            pointer_child(schema_path, "properties"), name
                        ),
                        full,
                        active,
                        depth + 1,
                    )
            additional = schema.get("additionalProperties", True)
            if (
                full
                and not properties
                and isinstance(additional, dict)
            ):
                result["syntheticKey"] = self._synthesize(
                    target,
                    additional,
                    pointer_child(schema_path, "additionalProperties"),
                    full,
                    active,
                    depth + 1,
                )
            return result
        raise FixtureError(
            f"unsupported synthesis type {selected_type!r} at {schema_path}"
        )

    def _outer_combinator_value(
        self,
        target: SchemaTarget,
        schema: Mapping[str, Any],
        schema_path: str,
        keyword: str,
        full: bool,
        active: frozenset[tuple[str, str]],
        depth: int,
    ) -> Any | None:
        constraint_keys = {
            "allOf",
            "const",
            "enum",
            "items",
            "maximum",
            "maxItems",
            "maxLength",
            "minimum",
            "minItems",
            "minLength",
            "pattern",
            "properties",
            "required",
            "type",
        }
        outer_schema = {
            key: value for key, value in schema.items() if key != keyword
        }
        if not (set(outer_schema) & constraint_keys):
            return None
        return self._synthesize(
            target,
            outer_schema,
            schema_path,
            full,
            active,
            depth + 1,
        )

    def _recursive_base(
        self,
        target: SchemaTarget,
        schema: Any,
        schema_path: str,
        active: frozenset[tuple[str, str]],
        depth: int,
    ) -> Any:
        if self._schema_allows_null(target, schema, schema_path):
            return None
        resolved, resolved_path = self.catalog.resolve(target, schema, schema_path)
        if isinstance(resolved, dict):
            expected = resolved.get("type")
            expected_types = [expected] if isinstance(expected, str) else expected
            if expected_types and "object" in expected_types:
                result: dict[str, Any] = {}
                properties = resolved.get("properties", {})
                for name in resolved.get("required", []):
                    child = properties.get(name, True)
                    result[name] = self._synthesize(
                        target,
                        child,
                        pointer_child(
                            pointer_child(resolved_path, "properties"), name
                        ),
                        False,
                        active,
                        depth + 1,
                    )
                return result
            if expected_types and "array" in expected_types:
                return []
        return self._synthesize(
            target,
            resolved,
            resolved_path,
            False,
            frozenset(),
            depth + 1,
        )

    def _schema_is_null(
        self, target: SchemaTarget, schema: Any, schema_path: str
    ) -> bool:
        resolved, _ = self.catalog.resolve(target, schema, schema_path)
        return isinstance(resolved, dict) and resolved.get("type") == "null"

    def _schema_allows_null(
        self, target: SchemaTarget, schema: Any, schema_path: str
    ) -> bool:
        validator = self.catalog.target_validator(target)
        return not validator.validate_subschema(None, schema, schema_path)

    def _string_value(self, schema: Mapping[str, Any], schema_path: str) -> str:
        minimum = int(schema.get("minLength", 0))
        maximum = schema.get("maxLength")
        digest = hashlib.sha256(schema_path.encode("utf-8")).hexdigest()[:10]
        candidates = [
            f"synthetic-{digest}",
            "item-42",
            "synthetic",
            "a",
            "0",
            "",
        ]
        pattern = schema.get("pattern")
        for candidate in candidates:
            if len(candidate) < minimum:
                candidate += "x" * (minimum - len(candidate))
            if maximum is not None:
                candidate = candidate[: int(maximum)]
            if len(candidate) < minimum:
                continue
            if pattern is None or re.search(pattern, candidate):
                return candidate
        raise FixtureError(
            f"unable to synthesize safe string matching pattern at {schema_path}"
        )

    def _merge_all_of(self, values: Sequence[Any], schema_path: str) -> Any:
        if all(isinstance(value, dict) for value in values):
            result: dict[str, Any] = {}
            for value in values:
                merge_object(result, value, schema_path)
            return result
        first = values[0]
        if all(canonical_json(value) == canonical_json(first) for value in values):
            return first
        raise FixtureError(f"conflicting allOf samples at {schema_path}")


def math_ceiling(value: int | float) -> int:
    integer = int(value)
    return integer if integer >= value else integer + 1


def merge_object(target: MutableMapping[str, Any], source: Mapping[str, Any], path: str) -> None:
    for key, value in source.items():
        if key in target and canonical_json(target[key]) != canonical_json(value):
            raise FixtureError(f"conflicting generated object field {key!r} at {path}")
        target[key] = copy.deepcopy(value)


def combine_values(outer: Any | None, branch: Any, path: str) -> Any:
    if outer is None:
        return branch
    if isinstance(outer, dict) and isinstance(branch, dict):
        result = copy.deepcopy(outer)
        merge_object(result, branch, path)
        return result
    if canonical_json(outer) == canonical_json(branch):
        return branch
    raise FixtureError(f"combinator branch conflicts with outer schema at {path}")


def matching_one_of_branches(
    catalog: SchemaCatalog, target: SchemaTarget, instance: Any, schema: Any, schema_path: str
) -> tuple[int, ...]:
    resolved, resolved_path = catalog.resolve(target, schema, schema_path)
    if not isinstance(resolved, dict) or "oneOf" not in resolved:
        return ()
    validator = catalog.target_validator(target)
    return tuple(
        index
        for index, branch in enumerate(resolved["oneOf"])
        if not validator.validate_subschema(
            instance,
            branch,
            pointer_child(pointer_child(resolved_path, "oneOf"), index),
        )
    )


@dataclass(frozen=True)
class RequiredLocation:
    instance_path: tuple[str | int, ...]
    schema: Any
    schema_path: str
    required_owner_instance_path: tuple[str | int, ...]
    required_owner_schema: Any
    required_owner_schema_path: str


@dataclass(frozen=True)
class ContainerValueLocation:
    instance_path: tuple[str | int, ...]
    schema: Any
    schema_path: str


@dataclass(frozen=True)
class ReferencedDefinitionLocation:
    instance_path: tuple[str | int, ...]
    definition_name: str
    schema_path: str


def collect_referenced_definition_locations(
    catalog: SchemaCatalog,
    target: SchemaTarget,
    instance: Any,
    definition_names: frozenset[str],
) -> list[ReferencedDefinitionLocation]:
    """Locate selected, exercised references by instance path.

    This is deliberately instance-sensitive: for a tagged union it follows
    only the branch selected by the root fixture.  The resulting paths let the
    generator prove nested-union reachability through each owning root instead
    of crediting an unrelated standalone fixture elsewhere in the corpus.
    """

    validator = catalog.target_validator(target)
    found: dict[
        tuple[tuple[str | int, ...], str], ReferencedDefinitionLocation
    ] = {}
    active: set[tuple[tuple[str | int, ...], str]] = set()

    def walk(
        value: Any,
        schema: Any,
        schema_path: str,
        instance_path: tuple[str | int, ...],
    ) -> None:
        if isinstance(schema, bool) or not isinstance(schema, dict):
            return
        reference = schema.get("$ref")
        if isinstance(reference, str):
            match = re.fullmatch(
                r"#/definitions/(?:(?:v2)/)?([^/]+)", reference
            )
            if match:
                name = match.group(1).replace("~1", "/").replace("~0", "~")
                if name in definition_names:
                    key = (instance_path, name)
                    found.setdefault(
                        key,
                        ReferencedDefinitionLocation(
                            instance_path,
                            name,
                            pointer_child(schema_path, "$ref"),
                        ),
                    )
            resolved, resolved_path = validator.resolve_reference(
                reference, pointer_child(schema_path, "$ref")
            )
            marker = (instance_path, resolved_path)
            if marker in active:
                return
            active.add(marker)
            walk(value, resolved, resolved_path, instance_path)
            active.remove(marker)
            return

        for index, child in enumerate(schema.get("allOf", [])):
            walk(
                value,
                child,
                pointer_child(pointer_child(schema_path, "allOf"), index),
                instance_path,
            )
        for keyword in ("oneOf", "anyOf"):
            branches = schema.get(keyword, [])
            matches = [
                (index, child)
                for index, child in enumerate(branches)
                if not validator.validate_subschema(
                    value,
                    child,
                    pointer_child(pointer_child(schema_path, keyword), index),
                )
            ]
            if keyword == "anyOf":
                matches = matches[:1]
            for index, child in matches:
                walk(
                    value,
                    child,
                    pointer_child(pointer_child(schema_path, keyword), index),
                    instance_path,
                )

        if isinstance(value, dict):
            properties = schema.get("properties", {})
            for name, child in properties.items():
                if name in value:
                    walk(
                        value[name],
                        child,
                        pointer_child(
                            pointer_child(schema_path, "properties"), name
                        ),
                        instance_child(instance_path, name),
                    )
            additional = schema.get("additionalProperties", True)
            if isinstance(additional, dict):
                for name in sorted(set(value) - set(properties)):
                    walk(
                        value[name],
                        additional,
                        pointer_child(schema_path, "additionalProperties"),
                        instance_child(instance_path, name),
                    )
        elif isinstance(value, list):
            items = schema.get("items")
            if isinstance(items, list):
                for index, (child_value, child) in enumerate(zip(value, items)):
                    walk(
                        child_value,
                        child,
                        pointer_child(pointer_child(schema_path, "items"), index),
                        instance_child(instance_path, index),
                    )
            elif isinstance(items, (bool, dict)):
                for index, child_value in enumerate(value):
                    walk(
                        child_value,
                        items,
                        pointer_child(schema_path, "items"),
                        instance_child(instance_path, index),
                    )

    walk(instance, target.schema, target.schema_path, ())
    return [
        found[key]
        for key in sorted(
            found,
            key=lambda value: (
                tuple(map(str, value[0])),
                value[1],
            ),
        )
    ]


def collect_container_value_locations(
    catalog: SchemaCatalog,
    target: SchemaTarget,
    instance: Any,
) -> list[ContainerValueLocation]:
    """Collect exercised array elements and typed map values.

    Object-property mutations alone do not exercise element/value codecs.  In
    particular, replacing an entire ``array<string>`` with a scalar never
    reaches the decoder branch that validates ``array[0]``.  Keep these
    locations separate from required/optional object properties so removal
    semantics remain tied only to actual properties.
    """

    validator = catalog.target_validator(target)
    found: dict[tuple[str | int, ...], ContainerValueLocation] = {}
    active: set[tuple[tuple[str | int, ...], str]] = set()

    def walk(
        value: Any,
        schema: Any,
        schema_path: str,
        instance_path: tuple[str | int, ...],
    ) -> None:
        if isinstance(schema, bool) or not isinstance(schema, dict):
            return
        if "$ref" in schema:
            resolved, resolved_path = validator.resolve_reference(
                schema["$ref"], pointer_child(schema_path, "$ref")
            )
            marker = (instance_path, resolved_path)
            if marker in active:
                return
            active.add(marker)
            walk(value, resolved, resolved_path, instance_path)
            active.remove(marker)
            return

        for index, child in enumerate(schema.get("allOf", [])):
            walk(
                value,
                child,
                pointer_child(pointer_child(schema_path, "allOf"), index),
                instance_path,
            )
        for keyword in ("oneOf", "anyOf"):
            branches = schema.get(keyword, [])
            matches = [
                (index, child)
                for index, child in enumerate(branches)
                if not validator.validate_subschema(
                    value,
                    child,
                    pointer_child(pointer_child(schema_path, keyword), index),
                )
            ]
            if keyword == "anyOf":
                matches = matches[:1]
            for index, child in matches:
                walk(
                    value,
                    child,
                    pointer_child(pointer_child(schema_path, keyword), index),
                    instance_path,
                )

        if isinstance(value, dict):
            properties = schema.get("properties", {})
            for name, child in properties.items():
                if name in value:
                    walk(
                        value[name],
                        child,
                        pointer_child(
                            pointer_child(schema_path, "properties"), name
                        ),
                        instance_child(instance_path, name),
                    )
            additional = schema.get("additionalProperties", True)
            if isinstance(additional, dict):
                for name in sorted(set(value) - set(properties)):
                    path = instance_child(instance_path, name)
                    location = ContainerValueLocation(
                        path,
                        additional,
                        pointer_child(schema_path, "additionalProperties"),
                    )
                    found.setdefault(path, location)
                    walk(
                        value[name],
                        additional,
                        location.schema_path,
                        path,
                    )
        elif isinstance(value, list):
            items = schema.get("items")
            if isinstance(items, list):
                for index, (child_value, child) in enumerate(zip(value, items)):
                    path = instance_child(instance_path, index)
                    location = ContainerValueLocation(
                        path,
                        child,
                        pointer_child(pointer_child(schema_path, "items"), index),
                    )
                    found.setdefault(path, location)
                    walk(child_value, child, location.schema_path, path)
            elif isinstance(items, (bool, dict)):
                for index, child_value in enumerate(value):
                    path = instance_child(instance_path, index)
                    location = ContainerValueLocation(
                        path,
                        items,
                        pointer_child(schema_path, "items"),
                    )
                    found.setdefault(path, location)
                    walk(child_value, items, location.schema_path, path)

    walk(instance, target.schema, target.schema_path, ())
    return [found[path] for path in sorted(found, key=lambda item: tuple(map(str, item)))]


def _collect_property_locations(
    catalog: SchemaCatalog,
    target: SchemaTarget,
    instance: Any,
    *,
    required_properties: bool,
) -> list[RequiredLocation]:
    validator = catalog.target_validator(target)
    found: dict[tuple[str | int, ...], RequiredLocation] = {}
    active: set[tuple[tuple[str | int, ...], str]] = set()

    def walk(
        value: Any,
        schema: Any,
        schema_path: str,
        instance_path: tuple[str | int, ...],
    ) -> None:
        if isinstance(schema, bool):
            return
        if not isinstance(schema, dict):
            return
        if "$ref" in schema:
            resolved, resolved_path = validator.resolve_reference(
                schema["$ref"], pointer_child(schema_path, "$ref")
            )
            marker = (instance_path, resolved_path)
            if marker in active:
                return
            active.add(marker)
            walk(value, resolved, resolved_path, instance_path)
            active.remove(marker)
            return

        for index, child in enumerate(schema.get("allOf", [])):
            walk(
                value,
                child,
                pointer_child(pointer_child(schema_path, "allOf"), index),
                instance_path,
            )
        for keyword in ("oneOf", "anyOf"):
            branches = schema.get(keyword, [])
            matches = [
                (index, child)
                for index, child in enumerate(branches)
                if not validator.validate_subschema(
                    value,
                    child,
                    pointer_child(pointer_child(schema_path, keyword), index),
                )
            ]
            # oneOf has exactly one; for anyOf the first deterministic matching
            # branch supplies the fixture's exercised constraints.
            if keyword == "anyOf":
                matches = matches[:1]
            for index, child in matches:
                walk(
                    value,
                    child,
                    pointer_child(pointer_child(schema_path, keyword), index),
                    instance_path,
                )

        if isinstance(value, dict):
            properties = schema.get("properties", {})
            required_names = set(schema.get("required", []))
            selected_names = (
                required_names
                if required_properties
                else set(properties) - required_names
            )
            for name in sorted(selected_names):
                if name in value:
                    property_schema = properties.get(name, True)
                    location = RequiredLocation(
                        instance_child(instance_path, name),
                        property_schema,
                        pointer_child(
                            pointer_child(schema_path, "properties"), name
                        ),
                        instance_path,
                        schema,
                        schema_path,
                    )
                    found.setdefault(location.instance_path, location)
            for name, child in properties.items():
                if name in value:
                    walk(
                        value[name],
                        child,
                        pointer_child(
                            pointer_child(schema_path, "properties"), name
                        ),
                        instance_child(instance_path, name),
                    )
            additional = schema.get("additionalProperties", True)
            if isinstance(additional, dict):
                for name in sorted(set(value) - set(properties)):
                    walk(
                        value[name],
                        additional,
                        pointer_child(schema_path, "additionalProperties"),
                        instance_child(instance_path, name),
                    )
        elif isinstance(value, list):
            items = schema.get("items")
            if isinstance(items, list):
                for index, (child_value, child) in enumerate(zip(value, items)):
                    walk(
                        child_value,
                        child,
                        pointer_child(pointer_child(schema_path, "items"), index),
                        instance_child(instance_path, index),
                    )
            elif isinstance(items, dict):
                for index, child_value in enumerate(value):
                    walk(
                        child_value,
                        items,
                        pointer_child(schema_path, "items"),
                        instance_child(instance_path, index),
                    )

    walk(instance, target.schema, target.schema_path, ())
    return [found[path] for path in sorted(found, key=lambda item: tuple(map(str, item)))]


def collect_required_locations(
    catalog: SchemaCatalog,
    target: SchemaTarget,
    instance: Any,
) -> list[RequiredLocation]:
    return _collect_property_locations(
        catalog,
        target,
        instance,
        required_properties=True,
    )


def collect_optional_present_locations(
    catalog: SchemaCatalog,
    target: SchemaTarget,
    instance: Any,
) -> list[RequiredLocation]:
    return _collect_property_locations(
        catalog,
        target,
        instance,
        required_properties=False,
    )


def schema_fixture_coverage(
    catalog: SchemaCatalog,
    target: SchemaTarget,
    instance: Any,
    *,
    omitted_schema_paths: Sequence[str] = (),
    directions_exercised: Sequence[str] = (),
) -> dict[str, Any]:
    """Describe schema facts proved by one independently validated fixture.

    Property paths are schema paths rather than C++ field names.  That keeps
    the evidence tied to the vendored pin and lets completeness be recomputed
    solely from the indexed corpus.  Omitted paths are explicit because an
    absent property cannot be discovered by walking an instance.
    """

    required = collect_required_locations(catalog, target, instance)
    optional = collect_optional_present_locations(catalog, target, instance)
    locations = {
        location.schema_path: location
        for location in (*required, *optional)
    }
    validator = catalog.target_validator(target)
    nullable = {
        schema_path
        for schema_path, location in locations.items()
        if not validator.validate_subschema(
            None, location.schema, location.schema_path
        )
    }
    nullable_null = {
        location.schema_path
        for location in (*required, *optional)
        if location.schema_path in nullable
        and get_instance_path(instance, location.instance_path) is None
    }
    nullable_value = nullable - nullable_null
    return {
        "property_schema_paths_present": sorted(locations),
        "optional_property_schema_paths_present": sorted(
            location.schema_path for location in optional
        ),
        "optional_property_schema_paths_omitted": sorted(
            set(omitted_schema_paths)
        ),
        "nullable_property_schema_paths": sorted(nullable),
        "nullable_value_schema_paths": sorted(nullable_value),
        "nullable_null_schema_paths": sorted(nullable_null),
        "directions_exercised": sorted(set(directions_exercised)),
    }


WRONG_TYPE_CANDIDATES = (None, False, 0, "wrong-type", [], {})
NO_WRONG_TYPE_MUTATION = object()


def mutation_evidence(
    catalog: SchemaCatalog, target: SchemaTarget, instance: Any
) -> dict[str, Any]:
    validator = catalog.target_validator(target)
    required = collect_required_locations(catalog, target, instance)
    optional_present = collect_optional_present_locations(
        catalog, target, instance
    )
    removal_records: list[dict[str, Any]] = []
    alternative_branch_records: list[dict[str, Any]] = []
    optional_omission_records: list[dict[str, Any]] = []
    optional_not_global_records: list[dict[str, Any]] = []
    wrong_type_records: list[dict[str, Any]] = []
    for location in required:
        mutated = copy.deepcopy(instance)
        parent, field = get_parent_path(mutated, location.instance_path)
        if not isinstance(parent, dict) or not isinstance(field, str):
            raise FixtureError(
                f"required location is not an object property: {json_path(location.instance_path)}"
            )
        parent.pop(field)
        root_diagnostics = validator.validate_subschema(
            mutated, target.schema, target.schema_path
        )
        required_owner = get_instance_path(
            mutated, location.required_owner_instance_path
        )
        intended_branch_diagnostics = validator.validate_subschema(
            required_owner,
            location.required_owner_schema,
            location.required_owner_schema_path,
        )
        if not intended_branch_diagnostics:
            raise FixtureError(
                "selected schema fragment accepted removal of its required "
                f"property at {json_path(location.instance_path)}"
            )
        if not root_diagnostics:
            # Draft-07 anyOf legitimately permits the mutated instance to
            # select another branch. Keep that full-root result, but prove
            # independently that the exact schema fragment selected during
            # fixture generation rejects removal of its required property.
            alternative_branch_records.append(
                {
                    "instance_path": json_path(location.instance_path),
                    "reason": (
                        "root schema accepts an alternative branch; the "
                        "selected fixture branch rejects the removal"
                    ),
                }
            )
        diagnostics = root_diagnostics or intended_branch_diagnostics
        removal_records.append(
            {
                "instance_path": json_path(location.instance_path),
                "diagnostic_codes": sorted({item.code for item in diagnostics}),
                "validation_scope": (
                    "root_schema"
                    if root_diagnostics
                    else "selected_schema_fragment"
                ),
            }
        )

        original = get_instance_path(instance, location.instance_path)
        selected: Any = NO_WRONG_TYPE_MUTATION
        selected_codes: list[str] = []
        for candidate in WRONG_TYPE_CANDIDATES:
            if canonical_json(candidate) == canonical_json(original):
                continue
            mutated = copy.deepcopy(instance)
            parent, field = get_parent_path(mutated, location.instance_path)
            parent[field] = copy.deepcopy(candidate)
            root_diagnostics = validator.validate_subschema(
                mutated, target.schema, target.schema_path
            )
            required_owner = get_instance_path(
                mutated, location.required_owner_instance_path
            )
            intended_branch_diagnostics = validator.validate_subschema(
                required_owner,
                location.required_owner_schema,
                location.required_owner_schema_path,
            )
            diagnostics = root_diagnostics or intended_branch_diagnostics
            if diagnostics:
                selected = candidate
                selected_codes = sorted({item.code for item in diagnostics})
                break
        wrong_type_records.append(
            {
                "instance_path": json_path(location.instance_path),
                "mutation_found": selected is not NO_WRONG_TYPE_MUTATION,
                "replacement_type": (
                    instance_type(selected)
                    if selected is not NO_WRONG_TYPE_MUTATION
                    else None
                ),
                "diagnostic_codes": selected_codes,
                "exclusion": (
                    None
                    if selected is not NO_WRONG_TYPE_MUTATION
                    else "schema accepts every JSON type candidate"
                ),
            }
        )
    for location in optional_present:
        mutated = copy.deepcopy(instance)
        parent, field = get_parent_path(mutated, location.instance_path)
        if not isinstance(parent, dict) or not isinstance(field, str):
            raise FixtureError(
                f"optional location is not an object property: {json_path(location.instance_path)}"
            )
        parent.pop(field)
        diagnostics = validator.validate_subschema(
            mutated, target.schema, target.schema_path
        )
        if diagnostics:
            optional_not_global_records.append(
                {
                    "instance_path": json_path(location.instance_path),
                    "diagnostic_codes": sorted(
                        {item.code for item in diagnostics}
                    ),
                    "reason": (
                        "property is optional in one selected schema fragment "
                        "but constrained by another active fragment"
                    ),
                }
            )
            continue
        optional_omission_records.append(
            {
                "instance_path": json_path(location.instance_path),
                "accepted": True,
            }
        )
    return {
        "selected_branch_required_locations": len(required),
        "required_locations": len(removal_records),
        "required_field_mutations": len(removal_records),
        "all_required_field_mutations_rejected": (
            len(removal_records) == len(required)
        ),
        "alternative_branch_acceptances": len(alternative_branch_records),
        "selected_branch_optional_present_locations": len(optional_present),
        "globally_optional_present_locations": len(
            optional_omission_records
        ),
        "optional_omission_mutations": len(optional_omission_records),
        "optional_not_globally_optional_locations": len(
            optional_not_global_records
        ),
        "all_globally_optional_omissions_accepted": True,
        "wrong_type_mutations": sum(
            record["mutation_found"] for record in wrong_type_records
        ),
        "wrong_type_unconstrained_exclusions": sum(
            record["exclusion"] is not None for record in wrong_type_records
        ),
        "removals": removal_records,
        "alternative_branch_removals": alternative_branch_records,
        "optional_omissions": optional_omission_records,
        "optional_not_globally_optional": optional_not_global_records,
        "wrong_types": wrong_type_records,
    }


def instance_type(value: Any) -> str:
    if value is None:
        return "null"
    if isinstance(value, bool):
        return "boolean"
    if isinstance(value, int):
        return "integer"
    if isinstance(value, float):
        return "number"
    if isinstance(value, str):
        return "string"
    if isinstance(value, list):
        return "array"
    if isinstance(value, dict):
        return "object"
    return type(value).__name__


def contract_map(
    manifest: Mapping[str, Any], contracts: Mapping[str, Any]
) -> dict[SurfaceKey, Mapping[str, Any]]:
    rows = contracts.get("contracts")
    if not isinstance(rows, list):
        raise FixtureError("operation-contracts.json lacks contracts array")
    result: dict[SurfaceKey, Mapping[str, Any]] = {}
    for row in rows:
        key = SurfaceKey.from_contract(row["surface_key"])
        if key in result:
            raise FixtureError(f"duplicate operation contract {key.compact()}")
        result[key] = row
    stable_requests = {
        SurfaceKey.from_manifest(entry): entry
        for entry in manifest["entries"]
        if entry["stability"] == "stable"
        and entry["category"] in {CLIENT_REQUEST, SERVER_REQUEST}
    }
    if set(result) != set(stable_requests):
        missing = sorted(key.compact() for key in set(stable_requests) - set(result))
        stale = sorted(key.compact() for key in set(result) - set(stable_requests))
        raise FixtureError(
            f"operation contracts mismatch stable request roots; "
            f"missing={missing}, stale={stale}"
        )
    for key, row in result.items():
        manifest_type = stable_requests[key]["params"]["type"] or "Unit"
        parameter_type = row.get("parameter_type_identity")
        if parameter_type != manifest_type:
            raise FixtureError(
                f"contract parameter type mismatch for {key.compact()}: "
                f"{parameter_type!r} != {manifest_type!r}"
            )
        result_type = row.get("result_type_identity")
        kind = row.get("result_contract_kind")
        if kind == "Unresolved" or result_type in {None, "", "Unresolved"}:
            raise FixtureError(
                f"unresolved operation result contract for {key.compact()}"
            )
        if kind == "Unit" and result_type != "Unit":
            raise FixtureError(
                f"Unit result contract has non-Unit type for {key.compact()}"
            )
    if len(result) != 97:
        raise FixtureError(f"expected 97 stable operation contracts, got {len(result)}")
    return result


def branch_for_union_identity(
    catalog: SchemaCatalog,
    target: SchemaTarget,
    field: str,
    name: str,
) -> tuple[int, Any, Any | None, tuple[str, Any] | None]:
    union, union_path = catalog.resolve(
        target, target.schema, target.schema_path
    )
    if not isinstance(union, dict):
        raise FixtureError(f"union target is not an object: {target.schema_path}")
    branches = union.get("oneOf", union.get("anyOf"))
    keyword = "oneOf" if "oneOf" in union else "anyOf"
    if not isinstance(branches, list):
        raise FixtureError(f"{target.schema_path} is not a union")
    matches: list[tuple[int, Any, Any | None, tuple[str, Any] | None]] = []
    validator = catalog.target_validator(target)
    for index, branch in enumerate(branches):
        resolved, branch_path = validator.resolve_reference(
            branch["$ref"],
            pointer_child(
                pointer_child(pointer_child(union_path, keyword), index), "$ref"
            ),
        ) if isinstance(branch, dict) and "$ref" in branch else (
            branch,
            pointer_child(pointer_child(union_path, keyword), index),
        )
        if not isinstance(resolved, dict):
            continue
        if field == "$variant":
            enum_values = resolved.get("enum")
            if isinstance(enum_values, list) and name in enum_values:
                matches.append((index, branch, name, None))
                continue
            required = resolved.get("required", [])
            properties = resolved.get("properties", {})
            if (
                isinstance(required, list)
                and name in required
                and name in properties
            ):
                matches.append((index, branch, None, None))
            continue
        properties = resolved.get("properties", {})
        property_schema = properties.get(field)
        if not isinstance(property_schema, dict):
            continue
        property_resolved, _ = catalog.resolve(
            target,
            property_schema,
            pointer_child(pointer_child(branch_path, "properties"), field),
        )
        if not isinstance(property_resolved, dict):
            continue
        if property_resolved.get("const") == name or property_resolved.get(
            "enum"
        ) == [name]:
            matches.append((index, branch, None, (field, name)))
    if len(matches) != 1:
        raise FixtureError(
            f"{target.schema_path} {field}={name!r} selects "
            f"{len(matches)} union branches"
        )
    return matches[0]


def validate_codex_error_rule_sets(
    manifest: Mapping[str, Any], schema: Mapping[str, Any]
) -> None:
    manifest_names = {
        str(entry["name"])
        for entry in manifest["entries"]
        if entry["stability"] == "stable"
        and entry["category"] == TAGGED_UNION_DISCRIMINATOR
        and entry["domain"] == "CodexErrorInfo"
        and entry["discriminator_field"] == "$variant"
    }
    reviewed_names = set(CODEX_ERROR_INFO_IDENTITIES)
    if (
        len(CODEX_ERROR_INFO_IDENTITIES) != len(reviewed_names)
        or manifest_names != reviewed_names
    ):
        raise FixtureError(
            "CODEX_ERROR_INFO_MANIFEST_RULE_MISMATCH: "
            f"missing={sorted(manifest_names - reviewed_names)!r} "
            f"stale={sorted(reviewed_names - manifest_names)!r}"
        )

    reviewed_http_names = set(CODEX_ERROR_INFO_HTTP_IDENTITIES)
    if (
        len(CODEX_ERROR_INFO_HTTP_IDENTITIES) != len(reviewed_http_names)
        or not reviewed_http_names < reviewed_names
        or "activeTurnNotSteerable" in reviewed_http_names
    ):
        raise FixtureError(
            "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: invalid reviewed HTTP "
            "identity subset"
        )

    branches = schema.get("oneOf")
    if not isinstance(branches, list):
        raise FixtureError(
            "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: CodexErrorInfo is not "
            "an exact oneOf union"
        )

    scalar_names: set[str] = set()
    object_schemas: dict[str, Mapping[str, Any]] = {}
    for branch in branches:
        if not isinstance(branch, dict):
            raise FixtureError(
                "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: non-object branch"
            )
        if branch.get("type") == "string":
            values = branch.get("enum")
            if not isinstance(values, list) or not all(
                isinstance(value, str) for value in values
            ):
                raise FixtureError(
                    "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: scalar branch "
                    "lacks a string enum"
                )
            scalar_names.update(values)
            continue
        if branch.get("type") != "object":
            raise FixtureError(
                "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: unsupported branch "
                "shape"
            )
        required = branch.get("required")
        properties = branch.get("properties")
        if (
            not isinstance(required, list)
            or len(required) != 1
            or not isinstance(required[0], str)
            or not isinstance(properties, dict)
            or set(properties) != {required[0]}
            or branch.get("additionalProperties") is not False
            or not isinstance(properties[required[0]], dict)
        ):
            raise FixtureError(
                "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: object branch does "
                "not have one exact discriminator property"
            )
        name = required[0]
        if name in object_schemas:
            raise FixtureError(
                "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: duplicate object "
                f"branch {name!r}"
            )
        object_schemas[name] = properties[name]

    expected_scalar_names = (
        reviewed_names - reviewed_http_names - {"activeTurnNotSteerable"}
    )
    if (
        scalar_names != expected_scalar_names
        or set(object_schemas)
        != reviewed_http_names | {"activeTurnNotSteerable"}
        or scalar_names | set(object_schemas) != manifest_names
    ):
        raise FixtureError(
            "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: reviewed scalar/object "
            "partition differs from the pinned schema"
        )

    for name in reviewed_http_names:
        nested = object_schemas[name]
        properties = nested.get("properties")
        status = (
            properties.get("httpStatusCode")
            if isinstance(properties, dict)
            else None
        )
        if (
            nested.get("type") != "object"
            or nested.get("required", []) != []
            or not isinstance(properties, dict)
            or set(properties) != {"httpStatusCode"}
            or not isinstance(status, dict)
            or set(status.get("type", [])) != {"integer", "null"}
            or status.get("format") != "uint16"
            or status.get("minimum") != 0.0
        ):
            raise FixtureError(
                "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: reviewed HTTP "
                f"shape differs for {name!r}"
            )

    active = object_schemas["activeTurnNotSteerable"]
    active_properties = active.get("properties")
    turn_kind = (
        active_properties.get("turnKind")
        if isinstance(active_properties, dict)
        else None
    )
    if (
        active.get("type") != "object"
        or active.get("required") != ["turnKind"]
        or not isinstance(active_properties, dict)
        or set(active_properties) != {"turnKind"}
        or not isinstance(turn_kind, dict)
        or turn_kind.get("$ref") != "#/definitions/NonSteerableTurnKind"
    ):
        raise FixtureError(
            "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: reviewed "
            "activeTurnNotSteerable shape differs"
        )


def schema_target_from_record(
    catalog: SchemaCatalog, schema_record: Mapping[str, Any]
) -> SchemaTarget:
    if "inline" in schema_record:
        document = schema_record["inline"]
        return SchemaTarget(
            document,
            document,
            str(schema_record.get("pointer", "#")),
            None,
            None,
            document,
        )
    relative = str(schema_record["file"])
    path = catalog.root / relative
    if sha256_file(path) != schema_record["sha256"]:
        raise FixtureError(f"fixture schema hash is stale: {relative}")
    document = catalog.load(path)
    validator = catalog.validator(path)
    pointer = str(schema_record["pointer"])
    if pointer == "#":
        schema = document
    else:
        schema, resolved_path = validator.resolve_reference(
            pointer, "#/fixturePointer"
        )
        if resolved_path != pointer:
            raise FixtureError(
                f"fixture schema pointer normalized unexpectedly: {pointer}"
            )
    return SchemaTarget(
        document,
        schema,
        pointer,
        relative,
        schema_record["sha256"],
    )


def root_slice(category: str, name: str) -> tuple[str, str]:
    if category == CLIENT_NOTIFICATION and name == "initialized":
        return "A1.0", "Common"
    if category == SERVER_NOTIFICATION and name == "error":
        return "A1.0", "Common"
    if name == "configWarning":
        return "A1.2", SLICE_MODULES["A1.2"]
    if name in {
        "guardianWarning",
        "permissionProfile/list",
        "thread/approveGuardianDeniedAction",
    }:
        return "A1.3", SLICE_MODULES["A1.3"]

    prefix = name.split("/", 1)[0]
    if prefix in {"thread", "turn"}:
        return "A1.1", SLICE_MODULES["A1.1"]
    if prefix == "item" and category == SERVER_NOTIFICATION:
        if any(
            marker in name
            for marker in (
                "autoApprovalReview",
                "requestApproval",
                "guardian",
                "permissions",
            )
        ):
            return "A1.3", SLICE_MODULES["A1.3"]
        return "A1.1", SLICE_MODULES["A1.1"]
    if prefix in {
        "account",
        "model",
        "modelProvider",
        "config",
        "configRequirements",
        "experimentalFeature",
    }:
        return "A1.2", SLICE_MODULES["A1.2"]
    if prefix in {
        "command",
        "fs",
        "review",
        "fuzzyFileSearch",
        "execCommandApproval",
        "applyPatchApproval",
    }:
        return "A1.3", SLICE_MODULES["A1.3"]
    if prefix == "item" and category == SERVER_REQUEST:
        if any(
            marker in name
            for marker in (
                "commandExecution",
                "fileChange",
                "permissions",
            )
        ):
            return "A1.3", SLICE_MODULES["A1.3"]
    if name == "initialize":
        return "A1.0", "Common"
    if name in A1_4_PUBLIC_ROOTS.get(category, frozenset()):
        return "A1.4", SLICE_MODULES["A1.4"]
    raise FixtureError(
        "A1_SLICE_UNRECOGNIZED_STABLE_ROOT: "
        f"category={category!r} name={name!r}"
    )


@dataclass(frozen=True, order=True)
class DefinitionId:
    namespace: str
    name: str

    def to_json(self) -> dict[str, str]:
        return {"namespace": self.namespace, "name": self.name}


@dataclass(frozen=True)
class RootReachability:
    root_id: str
    surface_key: SurfaceKey | None
    role: str
    slice: str
    definitions: frozenset[DefinitionId]

    def to_json(self) -> dict[str, Any]:
        result: dict[str, Any] = {
            "root_id": self.root_id,
            "role": self.role,
            "slice": self.slice,
        }
        if self.surface_key is not None:
            result["surface_key"] = self.surface_key.to_json()
        return result


def definition_graph(
    aggregate: Mapping[str, Any],
) -> tuple[dict[DefinitionId, Any], dict[DefinitionId, set[DefinitionId]]]:
    raw_definitions = aggregate.get("definitions")
    if not isinstance(raw_definitions, dict):
        raise FixtureError("combined aggregate lacks definitions")
    v2 = raw_definitions.get("v2")
    if not isinstance(v2, dict):
        raise FixtureError("combined aggregate lacks definitions.v2 namespace")
    nodes: dict[DefinitionId, Any] = {}
    for name, schema in raw_definitions.items():
        if name != "v2":
            nodes[DefinitionId("legacy", name)] = schema
    for name, schema in v2.items():
        nodes[DefinitionId("v2", name)] = schema

    def references(value: Any) -> Iterator[DefinitionId]:
        if isinstance(value, dict):
            reference = value.get("$ref")
            if isinstance(reference, str):
                match = re.fullmatch(
                    r"#/definitions/(?:(v2)/)?([^/]+)", reference
                )
                if match:
                    yield DefinitionId(
                        "v2" if match.group(1) else "legacy",
                        match.group(2).replace("~1", "/").replace("~0", "~"),
                    )
            for child in value.values():
                yield from references(child)
        elif isinstance(value, list):
            for child in value:
                yield from references(child)

    edges: dict[DefinitionId, set[DefinitionId]] = {}
    for identity, schema in nodes.items():
        outgoing = set(references(schema))
        unknown = outgoing - set(nodes)
        if unknown:
            raise FixtureError(
                f"definition graph has unresolved refs from {identity}: {sorted(unknown)}"
            )
        edges[identity] = outgoing
    return nodes, edges


def transitive_definitions(
    starts: Iterable[DefinitionId],
    edges: Mapping[DefinitionId, set[DefinitionId]],
) -> frozenset[DefinitionId]:
    reached: set[DefinitionId] = set()
    pending = list(starts)
    while pending:
        current = pending.pop()
        if current in reached:
            continue
        if current not in edges:
            raise FixtureError(f"unknown definition graph root {current}")
        reached.add(current)
        pending.extend(sorted(edges[current] - reached))
    return frozenset(reached)


def schema_references(value: Any) -> set[DefinitionId]:
    result: set[DefinitionId] = set()

    def walk(node: Any) -> None:
        if isinstance(node, dict):
            reference = node.get("$ref")
            if isinstance(reference, str):
                match = re.fullmatch(
                    r"#/definitions/(?:(v2)/)?([^/]+)", reference
                )
                if match:
                    result.add(
                        DefinitionId(
                            "v2" if match.group(1) else "legacy",
                            match.group(2).replace("~1", "/").replace("~0", "~"),
                        )
                    )
            for child in node.values():
                walk(child)
        elif isinstance(node, list):
            for child in node:
                walk(child)

    walk(value)
    return result


def locate_definition_for_type(
    catalog: SchemaCatalog,
    nodes: Mapping[DefinitionId, Any],
    type_identity: str,
) -> DefinitionId | None:
    if type_identity == "Unit":
        return None
    path = catalog._type_paths.get(type_identity)
    if path is None:
        raise FixtureError(f"no standalone root for graph type {type_identity}")
    relative = path.relative_to(catalog.stable_root)
    namespace = "v2" if relative.parts[0] == "v2" else "legacy"
    identity = DefinitionId(namespace, type_identity)
    if identity not in nodes:
        raise FixtureError(
            f"aggregate graph lacks {namespace} definition {type_identity}"
        )
    return identity


def build_reachability_and_assignments(
    catalog: SchemaCatalog,
    manifest: Mapping[str, Any],
    contracts: Mapping[SurfaceKey, Mapping[str, Any]],
) -> tuple[dict[str, Any], dict[str, Any]]:
    aggregate_path = (
        catalog.stable_root / "codex_app_server_protocol.schemas.json"
    )
    aggregate = catalog.load(aggregate_path)
    nodes, edges = definition_graph(aggregate)
    aggregate_definitions = aggregate["definitions"]

    roots: list[RootReachability] = []
    stable_entries = [
        entry
        for entry in manifest["entries"]
        if entry["stability"] == "stable"
    ]
    stable_by_key = {
        SurfaceKey.from_manifest(entry): entry for entry in stable_entries
    }

    method_unions = {
        category: aggregate_definitions[
            {
                CLIENT_REQUEST: "ClientRequest",
                CLIENT_NOTIFICATION: "ClientNotification",
                SERVER_NOTIFICATION: "ServerNotification",
                SERVER_REQUEST: "ServerRequest",
            }[category]
        ]
        for category in METHOD_SCHEMA_FILES
    }

    for key, entry in sorted(stable_by_key.items()):
        if key.category not in METHOD_SCHEMA_FILES:
            continue
        union = method_unions[key.category]
        matching = [
            branch
            for branch in union["oneOf"]
            if (
                branch.get("properties", {})
                .get("method", {})
                .get("enum")
                == [key.name]
            )
        ]
        if len(matching) != 1:
            raise FixtureError(
                f"aggregate method root mismatch for {key.compact()}"
            )
        branch = matching[0]
        starts = schema_references(branch.get("properties", {}).get("params", {}))
        if key.category in {CLIENT_REQUEST, SERVER_REQUEST}:
            result_type = str(
                contracts[key].get(
                    "result_schema_type_identity",
                    contracts[key]["result_type_identity"],
                )
            )
            result_definition = locate_definition_for_type(
                catalog, nodes, result_type
            )
            if result_definition is not None:
                starts.add(result_definition)
        reached = transitive_definitions(starts, edges) if starts else frozenset()
        slice_name, _ = root_slice(key.category, key.name)
        roots.append(
            RootReachability(
                key.compact(),
                key,
                "method",
                slice_name,
                reached,
            )
        )

    for domain in ("ThreadItem", "ResponseItem"):
        identity = DefinitionId("v2", domain)
        reached = transitive_definitions([identity], edges)
        roots.append(
            RootReachability(
                f"item_union:{domain}",
                None,
                "item_union",
                "A1.1",
                reached,
            )
        )

    reaching_by_domain: dict[str, list[RootReachability]] = {}
    for root in roots:
        for definition in root.definitions:
            reaching_by_domain.setdefault(definition.name, []).append(root)

    reachability_records: list[dict[str, Any]] = []
    assignments: list[dict[str, Any]] = []
    for entry in sorted(
        manifest["entries"],
        key=lambda value: SurfaceKey.from_manifest(value),
    ):
        key = SurfaceKey.from_manifest(entry)
        stability = str(entry["stability"])
        if stability == "experimental_only":
            assignments.append(
                {
                    "surface_key": key.to_json(),
                    "stability": stability,
                    "module": "ExperimentalInventory",
                    "a1_slice": "InventoryOnly",
                    "classification": "ExperimentalInventoryOnly",
                    "reason": "experimental-only roots remain registered inventory and are not typed during A1",
                }
            )
            continue

        if key.category in METHOD_SCHEMA_FILES:
            slice_name, module = root_slice(key.category, key.name)
            assignments.append(
                {
                    "surface_key": key.to_json(),
                    "stability": stability,
                    "module": module,
                    "a1_slice": slice_name,
                    "classification": "StablePublicRoot",
                    "reason": "fixed A1 domain-root assignment",
                }
            )
            continue
        if key.category == ITEM_DISCRIMINATOR:
            assignments.append(
                {
                    "surface_key": key.to_json(),
                    "stability": stability,
                    "module": SLICE_MODULES["A1.1"],
                    "a1_slice": "A1.1",
                    "classification": "StableItemRoot",
                    "reason": f"{key.domain} is owned by thread/turn/session flows",
                }
            )
            continue
        if key.category != TAGGED_UNION_DISCRIMINATOR:
            raise FixtureError(
                f"unhandled stable assignment category {key.category}"
            )

        reaching = sorted(
            {
                root.root_id: root
                for root in reaching_by_domain.get(key.domain, [])
            }.values(),
            key=lambda root: root.root_id,
        )
        if key.domain == "CodexErrorInfo":
            slice_name = "A1.0"
            module = "Common"
            classification = "CrossCuttingA1_0Exception"
            reason = "CodexErrorInfo is the fixed cross-cutting A1.0 exception"
        elif not reaching:
            slice_name = "InventoryOnly"
            module = "StableUnreachableInventory"
            classification = "StableUnreachableInventory"
            reason = "no stable public request/result/notification/item root reaches this union"
        else:
            slices = sorted(
                {root.slice for root in reaching},
                key=lambda value: SLICE_ORDER[value],
            )
            slice_name = slices[0]
            if len(slices) > 1:
                module = "Common"
                classification = "SharedCommon"
                reason = (
                    "assigned to earliest fixed slice among multiple reaching domains"
                )
            elif len(reaching) > 1:
                module = SLICE_MODULES[slice_name]
                classification = "SharedWithinSlice"
                reason = "shared by multiple stable roots in one fixed slice"
            else:
                module = SLICE_MODULES[slice_name]
                classification = "RootOwnedNestedUnion"
                reason = "assigned to its sole reaching stable root slice"
        assignments.append(
            {
                "surface_key": key.to_json(),
                "stability": stability,
                "module": module,
                "a1_slice": slice_name,
                "classification": classification,
                "reason": reason,
            }
        )
        reachability_records.append(
            {
                "surface_key": key.to_json(),
                "definition_domain": key.domain,
                "reaching_roots": [root.to_json() for root in reaching],
                "reaching_root_count": len(reaching),
                "reaching_slices": sorted(
                    {root.slice for root in reaching},
                    key=lambda value: SLICE_ORDER[value],
                ),
                "assigned_slice": slice_name,
                "classification": classification,
            }
        )

    if len(assignments) != 387:
        raise FixtureError(
            f"slice assignment must contain all 387 A0 identities, got {len(assignments)}"
        )
    assignment_keys = [
        SurfaceKey.from_contract(record["surface_key"]) for record in assignments
    ]
    if len(set(assignment_keys)) != 387:
        raise FixtureError("slice assignment contains duplicate exact identities")
    for record in assignments:
        # `slice` is the canonical consumer field. `a1_slice` is retained as
        # a descriptive compatibility alias for reports and fixture records.
        record["slice"] = record["a1_slice"]

    assignment_counts: dict[str, int] = {}
    classification_counts: dict[str, int] = {}
    for record in assignments:
        assignment_counts[record["a1_slice"]] = (
            assignment_counts.get(record["a1_slice"], 0) + 1
        )
        classification_counts[record["classification"]] = (
            classification_counts.get(record["classification"], 0) + 1
        )
    reachability = {
        "generated_notice": "Generated by tools/codex/app_server_fixtures.py; do not edit.",
        "format_version": FORMAT_VERSION,
        "codex_version": CODEX_VERSION,
        "root_count": len(roots),
        "roots": [root.to_json() for root in sorted(roots, key=lambda item: item.root_id)],
        "records": reachability_records,
        "counts": {
            "nested_union_identities": len(reachability_records),
            "stable_unreachable": sum(
                record["classification"] == "StableUnreachableInventory"
                for record in reachability_records
            ),
            "shared_common": sum(
                record["classification"] == "SharedCommon"
                for record in reachability_records
            ),
        },
    }
    assignment = {
        "generated_notice": "Generated by tools/codex/app_server_fixtures.py; do not edit.",
        "format_version": FORMAT_VERSION,
        "codex_version": CODEX_VERSION,
        "counts": {
            "total": len(assignments),
            "by_slice": dict(sorted(assignment_counts.items())),
            "by_classification": dict(sorted(classification_counts.items())),
        },
        "assignments": assignments,
    }
    return reachability, assignment


def schema_position_nodes(
    schema: Any,
    path: str = "#",
    combined_v2_namespace: bool = False,
) -> Iterator[tuple[str, Mapping[str, Any]]]:
    if isinstance(schema, bool):
        return
    if not isinstance(schema, dict):
        raise FixtureError(f"non-schema at {path}")
    yield path, schema
    for keyword in ("definitions", "properties"):
        children = schema.get(keyword, {})
        if not isinstance(children, dict):
            continue
        for name, child in sorted(children.items()):
            child_schema_path = pointer_child(pointer_child(path, keyword), name)
            if (
                combined_v2_namespace
                and keyword == "definitions"
                and name == "v2"
                and isinstance(child, dict)
            ):
                for nested_name, nested_schema in sorted(child.items()):
                    yield from schema_position_nodes(
                        nested_schema,
                        pointer_child(child_schema_path, nested_name),
                        False,
                    )
            else:
                yield from schema_position_nodes(
                    child, child_schema_path, False
                )
    additional = schema.get("additionalProperties")
    if isinstance(additional, (dict, bool)):
        yield from schema_position_nodes(
            additional, pointer_child(path, "additionalProperties"), False
        )
    items = schema.get("items")
    if isinstance(items, list):
        for index, child in enumerate(items):
            yield from schema_position_nodes(
                child, pointer_child(pointer_child(path, "items"), index), False
            )
    elif isinstance(items, (dict, bool)):
        yield from schema_position_nodes(
            items, pointer_child(path, "items"), False
        )
    for keyword in ("allOf", "anyOf", "oneOf"):
        children = schema.get(keyword, [])
        if isinstance(children, list):
            for index, child in enumerate(children):
                yield from schema_position_nodes(
                    child,
                    pointer_child(pointer_child(path, keyword), index),
                    False,
                )
    if isinstance(schema.get("not"), (dict, bool)):
        yield from schema_position_nodes(
            schema["not"], pointer_child(path, "not"), False
        )


def keyword_report(
    schema_root: Path, draft07: ModuleType
) -> dict[str, Any]:
    tree_reports: dict[str, Any] = {}
    for stability in ("stable", "experimental"):
        root = schema_root / stability
        counts: dict[str, int] = {}
        files = sorted(root.rglob("*.json"))
        unsupported_occurrences: list[dict[str, str]] = []
        for path in files:
            document = load_json(path)
            combined = path.name == "codex_app_server_protocol.schemas.json"
            for schema_path, node in schema_position_nodes(
                document, "#", combined
            ):
                for keyword in node:
                    counts[keyword] = counts.get(keyword, 0) + 1
                    if keyword not in draft07.SUPPORTED_SCHEMA_KEYWORDS:
                        unsupported_occurrences.append(
                            {
                                "file": path.relative_to(schema_root).as_posix(),
                                "schema_path": schema_path,
                                "keyword": keyword,
                            }
                        )
        if unsupported_occurrences:
            raise FixtureError(
                f"{stability} schemas use unsupported schema-position keywords: "
                f"{unsupported_occurrences[:10]}"
            )
        tree_reports[stability] = {
            "file_count": len(files),
            "actual_keyword_counts": dict(sorted(counts.items())),
            "unsupported_occurrences": [],
        }
    return {
        "generated_notice": "Generated by tools/codex/app_server_fixtures.py; do not edit.",
        "format_version": FORMAT_VERSION,
        "codex_version": CODEX_VERSION,
        "supported": {
            "validating": sorted(draft07.VALIDATING_KEYWORDS),
            "structural": sorted(draft07.STRUCTURAL_KEYWORDS),
            "annotations": sorted(draft07.ANNOTATION_KEYWORDS),
        },
        "trees": tree_reports,
        "notes": [
            "Counts are schema-position occurrences and include definitions duplicated by the upstream generator.",
            "Protocol properties named const, pattern, maximum, maxItems, maxLength, or dependencies are not counted as schema keywords.",
            "format values in the pin are schemars numeric metadata and are retained as annotations, not asserted.",
        ],
    }


class CorpusBuilder:
    def __init__(
        self,
        root: Path,
        schema_root: Path,
        manifest_path: Path,
        contracts_path: Path,
        draft07_path: Path,
    ):
        self.root = root
        self.schema_root = schema_root
        self.manifest_path = manifest_path
        self.contracts_path = contracts_path
        self.draft07_path = draft07_path
        self.draft07 = load_draft07(draft07_path)
        self.catalog = SchemaCatalog(schema_root, self.draft07)
        self.synthesizer = Synthesizer(self.catalog)
        self.manifest = load_json(manifest_path)
        self.contract_document = load_json(contracts_path)
        self.contracts = contract_map(self.manifest, self.contract_document)
        self.manifest_by_key = {
            SurfaceKey.from_manifest(entry): entry
            for entry in self.manifest["entries"]
        }
        self.assignments: dict[SurfaceKey, Mapping[str, Any]] = {}
        self.b2_shared_common_keys: tuple[SurfaceKey, ...] = ()
        self.b2_negative_coverage: dict[str, Any] = {}
        self.b2_indexed_coverage: dict[str, Any] = {}
        self.b3_item_keys: tuple[SurfaceKey, ...] = ()
        self.b3_negative_coverage: dict[str, Any] = {}
        self.b3_indexed_coverage: dict[str, Any] = {}
        self.b4_operation_keys: tuple[SurfaceKey, ...] = ()
        self.b4_operation_union_keys: tuple[SurfaceKey, ...] = ()
        self.b4_negative_coverage: dict[str, Any] = {}
        self.b4_indexed_coverage: dict[str, Any] = {}
        self.b4_operation_root_coverage: dict[str, Any] = {}
        self.b5_notification_keys: tuple[SurfaceKey, ...] = ()
        self.b5_negative_coverage: dict[str, Any] = {}
        self.b5_indexed_coverage: dict[str, Any] = {}
        self.b5_notification_root_coverage: dict[str, Any] = {}
        self.reachability: dict[str, Any] = {}
        self.files: dict[str, bytes] = {}
        self.records: list[dict[str, Any]] = []

    def add_positive(
        self,
        fixture_id: str,
        relative_file: str,
        role: str,
        target: SchemaTarget,
        value: Any,
        surface_key: SurfaceKey | None,
        intended_branch_indices: tuple[int, ...] = (),
        omitted_schema_paths: Sequence[str] = (),
        directions_exercised: Sequence[str] = (),
    ) -> None:
        validator = self.catalog.target_validator(target)
        diagnostics = validator.validate_subschema(
            value, target.schema, target.schema_path
        )
        if diagnostics:
            raise FixtureError(
                f"generated positive fixture {fixture_id} is invalid: "
                f"{[item.to_json() for item in diagnostics[:5]]}"
            )
        if intended_branch_indices:
            matches = matching_one_of_branches(
                self.catalog,
                target,
                value,
                target.schema,
                target.schema_path,
            )
            if matches != intended_branch_indices:
                raise FixtureError(
                    f"fixture {fixture_id} matches branches {matches}, "
                    f"expected {intended_branch_indices}"
                )
        mutation = mutation_evidence(self.catalog, target, value)
        payload = encoded_json(value)
        self.files[relative_file] = payload
        assignment = (
            self.assignments.get(surface_key) if surface_key is not None else None
        )
        completeness_evidence = {
            "authoritative_root_association": True,
            "positive_fixture_coverage": True,
            "required_fields_exercised": True,
            "schema_properties_exercised": False,
            "optional_present_exercised": (
                mutation["selected_branch_optional_present_locations"]
                == mutation["globally_optional_present_locations"]
                + mutation["optional_not_globally_optional_locations"]
            ),
            "optional_omitted_exercised": (
                mutation["globally_optional_present_locations"]
                == mutation["optional_omission_mutations"]
            ),
            "nullable_semantics_exercised": False,
            "reachable_union_alternatives_exercised": role
            in {"union_branch"},
            "fixture_current": True,
            "independently_schema_validated": True,
        }
        record: dict[str, Any] = {
            "id": fixture_id,
            "file": relative_file,
            "file_sha256": sha256_bytes(payload),
            "fixture_sha256": sha256_bytes(payload),
            "role": role,
            "schema": target.to_json(),
            "validation": {
                "independent": True,
                "one_of_branch_indices": list(intended_branch_indices),
                **mutation,
            },
            "completeness_evidence": completeness_evidence,
        }
        if omitted_schema_paths or directions_exercised:
            record["schema_fixture_coverage"] = schema_fixture_coverage(
                self.catalog,
                target,
                value,
                omitted_schema_paths=omitted_schema_paths,
                directions_exercised=directions_exercised,
            )
        if surface_key is not None:
            record["protocol_surface_key"] = surface_key.to_json()
        if assignment is not None:
            record["a1_slice"] = assignment["a1_slice"]
        self.records.append(record)

    def add_negative(
        self,
        fixture_id: str,
        relative_file: str,
        role: str,
        target: SchemaTarget,
        value: Any,
        expected_codes: Sequence[str],
        surface_key: SurfaceKey | None = None,
        negative_case: str | None = None,
    ) -> None:
        diagnostics = self.catalog.target_validator(target).validate_subschema(
            value, target.schema, target.schema_path
        )
        actual_codes = sorted({item.code for item in diagnostics})
        if actual_codes != sorted(expected_codes):
            raise FixtureError(
                f"negative fixture {fixture_id} diagnostics {actual_codes}, "
                f"expected {sorted(expected_codes)}"
            )
        payload = encoded_json(value)
        self.files[relative_file] = payload
        record: dict[str, Any] = {
            "id": fixture_id,
            "file": relative_file,
            "file_sha256": sha256_bytes(payload),
            "role": role,
            "schema": target.to_json(),
            "expected_valid": False,
            "expected_diagnostic_codes": actual_codes,
        }
        if surface_key is not None:
            record["protocol_surface_key"] = surface_key.to_json()
            assignment = self.assignments.get(surface_key)
            if assignment is not None:
                record["a1_slice"] = assignment["a1_slice"]
        if negative_case is not None:
            record["negative_case"] = negative_case
        self.records.append(record)

    def build(
        self,
    ) -> tuple[
        dict[str, bytes],
        dict[str, Any],
        dict[str, Any],
        dict[str, Any],
        dict[str, Any],
        dict[str, Any],
    ]:
        reachability, assignment_document = build_reachability_and_assignments(
            self.catalog, self.manifest, self.contracts
        )
        self.reachability = reachability
        self.assignments = {
            SurfaceKey.from_contract(record["surface_key"]): record
            for record in assignment_document["assignments"]
        }
        self.b2_shared_common_keys = derive_b2_shared_common_keys(
            self.assignments
        )
        self.b3_item_keys = derive_b3_item_keys(self.assignments)
        self.b4_operation_keys = derive_b4_operation_keys(self.assignments)
        self.b4_operation_union_keys = derive_b4_operation_union_keys(
            self.assignments
        )
        self.b5_notification_keys = derive_b5_notification_keys(
            self.assignments
        )

        self._build_operation_fixtures()
        self._build_b4_operation_supplements()
        self._build_b4_helper_union_fixtures()
        self._build_baseline_fixtures()
        self._build_b5_notification_fixtures()
        self._build_union_fixtures()
        self._build_b2_open_enum_fixtures()
        self._build_b3_open_enum_fixtures()
        self._build_b4_open_enum_fixtures()
        self._build_b5_open_enum_fixtures()

        self.records.sort(key=lambda record: record["id"])
        fixture_counts: dict[str, int] = {}
        for record in self.records:
            fixture_counts[record["role"]] = (
                fixture_counts.get(record["role"], 0) + 1
            )
        positive_records = [
            record
            for record in self.records
            if record.get("expected_valid", True)
        ]
        positive_fixture_ids = {
            str(record["id"]) for record in positive_records
        }
        all_codex_error_alternatives_exercised = all(
            f"union:CodexErrorInfo:{name}" in positive_fixture_ids
            for name in CODEX_ERROR_INFO_IDENTITIES
        )
        for record in positive_records:
            key_record = record.get("protocol_surface_key")
            if not isinstance(key_record, dict):
                continue
            key = SurfaceKey.from_contract(key_record)
            if (
                key.category != TAGGED_UNION_DISCRIMINATOR
                or key.domain != "CodexErrorInfo"
                or key.name not in CODEX_ERROR_INFO_IDENTITIES
            ):
                continue
            if key.name in CODEX_ERROR_INFO_HTTP_IDENTITIES:
                required_fixtures = {
                    f"union:CodexErrorInfo:{key.name}",
                    f"union:CodexErrorInfo:{key.name}:http-status-null",
                    f"union:CodexErrorInfo:{key.name}:http-status-omitted",
                }
            elif key.name == "activeTurnNotSteerable":
                required_fixtures = {
                    "union:CodexErrorInfo:activeTurnNotSteerable",
                    "union:CodexErrorInfo:activeTurnNotSteerable:turn-kind-compact",
                }
            else:
                required_fixtures = {f"union:CodexErrorInfo:{key.name}"}
            properties_exercised = required_fixtures <= positive_fixture_ids
            facts = record["completeness_evidence"]
            facts["schema_properties_exercised"] = properties_exercised
            facts["nullable_semantics_exercised"] = (
                properties_exercised
                if key.name in CODEX_ERROR_INFO_HTTP_IDENTITIES
                else True
            )
            facts["reachable_union_alternatives_exercised"] = (
                all_codex_error_alternatives_exercised
            )
        self._apply_b2_indexed_completeness(
            positive_records, positive_fixture_ids
        )
        self._apply_b3_indexed_completeness(positive_records)
        self._apply_b2_indexed_completeness(
            positive_records,
            positive_fixture_ids,
            keys=self.b4_operation_union_keys,
            directions_by_domain=B4_OPERATION_UNION_DIRECTIONS,
            batch="B4",
        )
        self._apply_b4_operation_indexed_completeness(
            positive_records, positive_fixture_ids
        )
        self._apply_b5_notification_indexed_completeness(
            positive_records, positive_fixture_ids
        )
        mutation_counts = {
            "selected_branch_required_locations": sum(
                record["validation"]["selected_branch_required_locations"]
                for record in positive_records
            ),
            "required_locations": sum(
                record["validation"]["required_locations"]
                for record in positive_records
            ),
            "required_field_removals_rejected": sum(
                record["validation"]["required_field_mutations"]
                for record in positive_records
            ),
            "wrong_type_mutations_rejected": sum(
                record["validation"]["wrong_type_mutations"]
                for record in positive_records
            ),
            "wrong_type_unconstrained_exclusions": sum(
                record["validation"][
                    "wrong_type_unconstrained_exclusions"
                ]
                for record in positive_records
            ),
            "alternative_branch_acceptances": sum(
                record["validation"]["alternative_branch_acceptances"]
                for record in positive_records
            ),
            "optional_present_locations": sum(
                record["validation"][
                    "selected_branch_optional_present_locations"
                ]
                for record in positive_records
            ),
            "globally_optional_locations": sum(
                record["validation"]["globally_optional_present_locations"]
                for record in positive_records
            ),
            "optional_omissions_accepted": sum(
                record["validation"]["optional_omission_mutations"]
                for record in positive_records
            ),
            "optional_cross_fragment_exclusions": sum(
                record["validation"][
                    "optional_not_globally_optional_locations"
                ]
                for record in positive_records
            ),
        }
        if (
            mutation_counts["selected_branch_required_locations"]
            != mutation_counts["required_locations"]
            or mutation_counts["required_locations"]
            != mutation_counts["required_field_removals_rejected"]
        ):
            raise FixtureError(
                "not every selected-branch required field removal was rejected"
            )
        if (
            mutation_counts["required_locations"]
            != mutation_counts["wrong_type_mutations_rejected"]
            + mutation_counts["wrong_type_unconstrained_exclusions"]
        ):
            raise FixtureError(
                "wrong-type mutations and explicit unconstrained exclusions "
                "do not cover every globally required value"
            )
        if (
            mutation_counts["globally_optional_locations"]
            != mutation_counts["optional_omissions_accepted"]
            or mutation_counts["optional_present_locations"]
            != mutation_counts["globally_optional_locations"]
            + mutation_counts["optional_cross_fragment_exclusions"]
        ):
            raise FixtureError(
                "optional-present and optional-omitted mutation evidence is incomplete"
            )
        serialized_records = [
            compact_generated_record(record) for record in self.records
        ]
        index = {
            "generated_notice": "Generated by tools/codex/app_server_fixtures.py; do not edit.",
            "format_version": FORMAT_VERSION,
            "codex_version": CODEX_VERSION,
            "rules_version": RULES_VERSION,
            "sources": self._source_hashes(),
            "counts": {
                "total": len(self.records),
                "positive": len(positive_records),
                "negative": len(self.records) - len(positive_records),
                "by_role": dict(sorted(fixture_counts.items())),
            },
            "mutation_counts": mutation_counts,
            "a1_1_shared_common": {
                "assignment_derived_keys": [
                    key.to_json() for key in self.b2_shared_common_keys
                ],
                "indexed_schema_coverage": self.b2_indexed_coverage,
                "negative_coverage": self.b2_negative_coverage,
            },
            "a1_1_item_models": {
                "assignment_derived_keys": [
                    key.to_json() for key in self.b3_item_keys
                ],
                "indexed_schema_coverage": self.b3_indexed_coverage,
                "negative_coverage": self.b3_negative_coverage,
            },
            "a1_1_operations": {
                "assignment_derived_request_keys": [
                    key.to_json() for key in self.b4_operation_keys
                ],
                "assignment_derived_union_keys": [
                    key.to_json()
                    for key in self.b4_operation_union_keys
                ],
                "indexed_schema_coverage": self.b4_indexed_coverage,
                "root_fixture_plan": self.b4_operation_root_coverage,
                "negative_coverage": self.b4_negative_coverage,
            },
            "a1_1_notifications": {
                "assignment_derived_keys": [
                    key.to_json() for key in self.b5_notification_keys
                ],
                "indexed_schema_coverage": self.b5_indexed_coverage,
                "root_fixture_plan": self.b5_notification_root_coverage,
                "negative_coverage": self.b5_negative_coverage,
            },
            "fixtures": serialized_records,
        }
        self.files["index.json"] = encoded_json(index)

        coverage_by_key: dict[SurfaceKey, list[Mapping[str, Any]]] = {}
        for record in positive_records:
            key_record = record.get("protocol_surface_key")
            if key_record is not None:
                coverage_by_key.setdefault(
                    SurfaceKey.from_contract(key_record), []
                ).append(record)
        coverage_records: list[dict[str, Any]] = []
        for key in sorted(self.manifest_by_key):
            records = coverage_by_key.get(key, [])
            record_ids = {str(record["id"]) for record in records}
            is_codex_error = (
                key.category == TAGGED_UNION_DISCRIMINATOR
                and key.domain == "CodexErrorInfo"
                and key.name in CODEX_ERROR_INFO_IDENTITIES
            )
            if key.name in CODEX_ERROR_INFO_HTTP_IDENTITIES and is_codex_error:
                expected_property_fixtures = {
                    f"union:CodexErrorInfo:{key.name}",
                    f"union:CodexErrorInfo:{key.name}:http-status-null",
                    f"union:CodexErrorInfo:{key.name}:http-status-omitted",
                }
            elif key.name == "activeTurnNotSteerable" and is_codex_error:
                expected_property_fixtures = {
                    "union:CodexErrorInfo:activeTurnNotSteerable",
                    "union:CodexErrorInfo:activeTurnNotSteerable:turn-kind-compact",
                }
            elif is_codex_error:
                expected_property_fixtures = {
                    f"union:CodexErrorInfo:{key.name}"
                }
            else:
                expected_property_fixtures = set()
            codex_properties_exercised = (
                is_codex_error
                and expected_property_fixtures <= record_ids
            )
            codex_nullable_exercised = (
                codex_properties_exercised
                if key.name in CODEX_ERROR_INFO_HTTP_IDENTITIES
                else is_codex_error
            )
            is_b2_shared_common = key in self.b2_shared_common_keys
            b2_schema_facts = (
                self.b2_indexed_coverage.get(key.compact(), {}).get(
                    "schema_fixture_facts", {}
                )
                if is_b2_shared_common
                else {}
            )
            is_b3_item = key in self.b3_item_keys
            b3_schema_facts = (
                self.b3_indexed_coverage.get(key.compact(), {}).get(
                    "schema_fixture_facts", {}
                )
                if is_b3_item
                else {}
            )
            is_b4_operation = (
                key in self.b4_operation_keys
                or key in self.b4_operation_union_keys
            )
            b4_schema_facts = (
                self.b4_indexed_coverage.get(key.compact(), {}).get(
                    "schema_fixture_facts", {}
                )
                if is_b4_operation
                else {}
            )
            is_b5_notification = key in self.b5_notification_keys
            b5_schema_facts = (
                self.b5_indexed_coverage.get(key.compact(), {}).get(
                    "schema_fixture_facts", {}
                )
                if is_b5_notification
                else {}
            )
            indexed_coverage = (
                self.b2_indexed_coverage.get(key.compact(), {})
                if is_b2_shared_common
                else self.b3_indexed_coverage.get(key.compact(), {})
                if is_b3_item
                else self.b4_indexed_coverage.get(key.compact(), {})
                if is_b4_operation
                else self.b5_indexed_coverage.get(key.compact(), {})
                if is_b5_notification
                else {}
            )
            coverage_records.append(
                {
                    "protocol_surface_key": key.to_json(),
                    "fixture_ids": sorted(record["id"] for record in records),
                    "positive_fixture_count": len(records),
                    "fixture_current": bool(records),
                    "independently_schema_validated": bool(records),
                    "required_field_mutations": sum(
                        record["validation"]["required_field_mutations"]
                        for record in records
                    ),
                    "optional_omission_mutations": sum(
                        record["validation"]["optional_omission_mutations"]
                        for record in records
                    ),
                    "wrong_type_mutations": sum(
                        record["validation"]["wrong_type_mutations"]
                        for record in records
                    ),
                    "schema_directions_exercised": (
                        indexed_coverage.get("directions_exercised", [])
                    ),
                    "schema_direction_coverage": bool(
                        indexed_coverage.get(
                            "schema_direction_coverage", False
                        )
                    ),
                    "completeness_evidence": {
                        "authoritative_root_association": bool(records),
                        "positive_fixture_coverage": bool(records),
                        "required_fields_exercised": bool(records),
                        "schema_properties_exercised": (
                            codex_properties_exercised
                            or bool(
                                b2_schema_facts.get(
                                    "schema_properties_exercised", False
                                )
                            )
                            or bool(
                                b3_schema_facts.get(
                                    "schema_properties_exercised", False
                                )
                            )
                            or bool(
                                b4_schema_facts.get(
                                    "schema_properties_exercised", False
                                )
                            )
                            or bool(
                                b5_schema_facts.get(
                                    "schema_properties_exercised", False
                                )
                            )
                        ),
                        "optional_present_exercised": bool(records)
                        and all(
                            record["completeness_evidence"][
                                "optional_present_exercised"
                            ]
                            for record in records
                        ),
                        "optional_omitted_exercised": bool(records)
                        and all(
                            record["completeness_evidence"][
                                "optional_omitted_exercised"
                            ]
                            for record in records
                        ),
                        "nullable_semantics_exercised": (
                            codex_nullable_exercised
                            or bool(
                                b2_schema_facts.get(
                                    "nullable_semantics_exercised", False
                                )
                            )
                            or bool(
                                b3_schema_facts.get(
                                    "nullable_semantics_exercised", False
                                )
                            )
                            or bool(
                                b4_schema_facts.get(
                                    "nullable_semantics_exercised", False
                                )
                            )
                            or bool(
                                b5_schema_facts.get(
                                    "nullable_semantics_exercised", False
                                )
                            )
                        ),
                        "reachable_union_alternatives_exercised": (
                            (
                                is_codex_error
                                and all_codex_error_alternatives_exercised
                            )
                            or bool(
                                b2_schema_facts.get(
                                    "reachable_union_alternatives_exercised",
                                    False,
                                )
                            )
                            or bool(
                                b3_schema_facts.get(
                                    "reachable_union_alternatives_exercised",
                                    False,
                                )
                            )
                            or bool(
                                b4_schema_facts.get(
                                    "reachable_union_alternatives_exercised",
                                    False,
                                )
                            )
                            or bool(
                                b5_schema_facts.get(
                                    "reachable_union_alternatives_exercised",
                                    False,
                                )
                            )
                        ),
                        "fixture_current": bool(records),
                        "independently_schema_validated": bool(records),
                    },
                }
            )
        fixture_coverage = {
            "generated_notice": "Generated by tools/codex/app_server_fixtures.py; do not edit.",
            "format_version": FORMAT_VERSION,
            "codex_version": CODEX_VERSION,
            "sources": index["sources"],
            "counts": {
                "surface_identities": len(coverage_records),
                "identities_with_positive_fixtures": sum(
                    bool(record["positive_fixture_count"])
                    for record in coverage_records
                ),
                "positive_fixtures": len(positive_records),
                "operation_role_expected": 194,
                "operation_role_actual": sum(
                    record["role"]
                    in {
                        "client_request_params",
                        "client_request_result",
                        "server_request_params",
                        "server_request_response",
                    }
                    for record in positive_records
                ),
                "required_field_removals_rejected": mutation_counts[
                    "required_field_removals_rejected"
                ],
                "optional_present_locations": mutation_counts[
                    "optional_present_locations"
                ],
                "optional_omissions_accepted": mutation_counts[
                    "optional_omissions_accepted"
                ],
                "wrong_type_mutations_rejected": mutation_counts[
                    "wrong_type_mutations_rejected"
                ],
                "wrong_type_unconstrained_exclusions": mutation_counts[
                    "wrong_type_unconstrained_exclusions"
                ],
            },
            "a1_1_shared_common": {
                "assignment_derived_keys": [
                    key.to_json() for key in self.b2_shared_common_keys
                ],
                "indexed_schema_coverage": self.b2_indexed_coverage,
                "negative_coverage": self.b2_negative_coverage,
            },
            "a1_1_item_models": {
                "assignment_derived_keys": [
                    key.to_json() for key in self.b3_item_keys
                ],
                "indexed_schema_coverage": self.b3_indexed_coverage,
                "negative_coverage": self.b3_negative_coverage,
            },
            "a1_1_operations": {
                "assignment_derived_request_keys": [
                    key.to_json() for key in self.b4_operation_keys
                ],
                "assignment_derived_union_keys": [
                    key.to_json()
                    for key in self.b4_operation_union_keys
                ],
                "indexed_schema_coverage": self.b4_indexed_coverage,
                "root_fixture_plan": self.b4_operation_root_coverage,
                "negative_coverage": self.b4_negative_coverage,
            },
            "a1_1_notifications": {
                "assignment_derived_keys": [
                    key.to_json() for key in self.b5_notification_keys
                ],
                "indexed_schema_coverage": self.b5_indexed_coverage,
                "root_fixture_plan": self.b5_notification_root_coverage,
                "negative_coverage": self.b5_negative_coverage,
            },
            "fixtures": [
                compact_generated_record(record)
                for record in positive_records
                if "protocol_surface_key" in record
            ],
            "identity_coverage": coverage_records,
        }
        if fixture_coverage["counts"]["operation_role_actual"] != 194:
            raise FixtureError(
                "operation fixture corpus must contain 194 request/result roles"
            )
        keywords = keyword_report(self.schema_root, self.draft07)
        return self.files, index, fixture_coverage, keywords, reachability, assignment_document

    def _source_hashes(self) -> dict[str, Any]:
        provenance = load_json(self.schema_root / "PROVENANCE.json")
        return {
            "stable_schema_aggregate_sha256": provenance["schema_trees"]["stable"][
                "aggregate_sha256"
            ],
            "experimental_schema_aggregate_sha256": provenance["schema_trees"][
                "experimental"
            ]["aggregate_sha256"],
            "surface_manifest_sha256": sha256_file(self.manifest_path),
            "operation_contracts_sha256": sha256_file(self.contracts_path),
            "validator_sha256": sha256_file(self.draft07_path),
            "generator_sha256": sha256_file(Path(__file__).resolve()),
        }

    def _apply_b2_indexed_completeness(
        self,
        positive_records: Sequence[MutableMapping[str, Any]],
        positive_fixture_ids: set[str],
        *,
        keys: Sequence[SurfaceKey] | None = None,
        directions_by_domain: Mapping[str, tuple[str, ...]] | None = None,
        batch: str = "B2",
    ) -> None:
        selected_keys = (
            tuple(keys)
            if keys is not None
            else self.b2_shared_common_keys
        )
        selected_directions = (
            directions_by_domain
            if directions_by_domain is not None
            else B2_SHARED_COMMON_DIRECTIONS
        )
        records_by_key: dict[
            SurfaceKey, list[MutableMapping[str, Any]]
        ] = {}
        for record in positive_records:
            key_record = record.get("protocol_surface_key")
            if isinstance(key_record, dict):
                records_by_key.setdefault(
                    SurfaceKey.from_contract(key_record), []
                ).append(record)

        indexed_coverage: dict[str, Any] = {}
        for key in selected_keys:
            records = records_by_key.get(key, [])
            base_id = f"union:{key.domain}:{key.name}"
            base_records = [
                record for record in records if record["id"] == base_id
            ]
            if len(base_records) != 1:
                raise FixtureError(
                    f"{batch} identity lacks exactly one base fixture: {key.compact()}"
                )
            base_coverage = base_records[0]["schema_fixture_coverage"]
            expected_properties = set(
                base_coverage["property_schema_paths_present"]
            )
            expected_optional = set(
                base_coverage[
                    "optional_property_schema_paths_present"
                ]
            )
            expected_nullable = set(
                base_coverage["nullable_property_schema_paths"]
            )
            present = {
                path
                for record in records
                for path in record["schema_fixture_coverage"][
                    "property_schema_paths_present"
                ]
            }
            optional_present = {
                path
                for record in records
                for path in record["schema_fixture_coverage"][
                    "optional_property_schema_paths_present"
                ]
            }
            optional_omitted = {
                path
                for record in records
                for path in record["schema_fixture_coverage"][
                    "optional_property_schema_paths_omitted"
                ]
            }
            nullable_value = {
                path
                for record in records
                for path in record["schema_fixture_coverage"][
                    "nullable_value_schema_paths"
                ]
            }
            nullable_null = {
                path
                for record in records
                for path in record["schema_fixture_coverage"][
                    "nullable_null_schema_paths"
                ]
            }
            directions = {
                direction
                for record in records
                for direction in record["schema_fixture_coverage"][
                    "directions_exercised"
                ]
            }
            family_base_ids = {
                f"union:{candidate.domain}:{candidate.name}"
                for candidate in selected_keys
                if candidate.domain == key.domain
            }
            facts = {
                "schema_properties_exercised": (
                    expected_properties <= present
                ),
                "optional_present_exercised": (
                    expected_optional <= optional_present
                ),
                "optional_omitted_exercised": (
                    expected_optional <= optional_omitted
                ),
                "nullable_semantics_exercised": (
                    expected_nullable <= nullable_value
                    and expected_nullable <= nullable_null
                    and expected_nullable <= optional_omitted
                ),
                "reachable_union_alternatives_exercised": (
                    family_base_ids <= positive_fixture_ids
                ),
            }
            required_directions = set(
                selected_directions[key.domain]
            )
            direction_coverage = required_directions <= directions
            if not all(facts.values()) or not direction_coverage:
                raise FixtureError(
                    f"{batch} indexed fixture completeness is incomplete for "
                    f"{key.compact()}: facts={facts} "
                    f"directions={sorted(directions)}"
                )
            for record in records:
                record_facts = record["completeness_evidence"]
                record_facts.update(facts)
            indexed_coverage[key.compact()] = {
                "base_fixture_id": base_id,
                "property_schema_paths": sorted(expected_properties),
                "optional_property_schema_paths": sorted(
                    expected_optional
                ),
                "nullable_property_schema_paths": sorted(
                    expected_nullable
                ),
                "required_directions": sorted(required_directions),
                "directions_exercised": sorted(directions),
                "schema_direction_coverage": direction_coverage,
                "schema_fixture_facts": facts,
            }
        if batch == "B2":
            self.b2_indexed_coverage = dict(
                sorted(indexed_coverage.items())
            )
        elif batch == "B4":
            self.b4_indexed_coverage.update(indexed_coverage)
            self.b4_indexed_coverage = dict(
                sorted(self.b4_indexed_coverage.items())
            )
        else:
            raise FixtureError(f"unsupported indexed union batch {batch}")

    def _apply_b3_indexed_completeness(
        self,
        positive_records: Sequence[MutableMapping[str, Any]],
    ) -> None:
        records_by_key: dict[
            SurfaceKey, list[MutableMapping[str, Any]]
        ] = {}
        for record in positive_records:
            key_record = record.get("protocol_surface_key")
            if isinstance(key_record, dict):
                records_by_key.setdefault(
                    SurfaceKey.from_contract(key_record), []
                ).append(record)

        alternative_evidence = self.b3_negative_coverage.get("alternatives")
        if not isinstance(alternative_evidence, dict):
            raise FixtureError(
                "B3 root-specific reachability evidence is unavailable"
            )
        indexed_coverage: dict[str, Any] = {}
        property_count = 0
        optional_count = 0
        nullable_count = 0

        for key in self.b3_item_keys:
            records = records_by_key.get(key, [])
            base_id = f"union:{key.domain}:{key.name}"
            base_records = [
                record for record in records if record["id"] == base_id
            ]
            if len(base_records) != 1:
                raise FixtureError(
                    f"B3 identity lacks exactly one base fixture: {key.compact()}"
                )
            base_coverage = base_records[0].get("schema_fixture_coverage")
            if not isinstance(base_coverage, dict):
                raise FixtureError(
                    f"B3 base fixture lacks indexed schema coverage: {key.compact()}"
                )
            expected_properties = set(
                base_coverage["property_schema_paths_present"]
            )
            expected_optional = set(
                base_coverage[
                    "optional_property_schema_paths_present"
                ]
            )
            expected_nullable = set(
                base_coverage["nullable_property_schema_paths"]
            )
            property_count += len(expected_properties)
            optional_count += len(expected_optional)
            nullable_count += len(expected_nullable)

            schema_records = [
                record
                for record in records
                if isinstance(record.get("schema_fixture_coverage"), dict)
            ]
            present = {
                path
                for record in schema_records
                for path in record["schema_fixture_coverage"][
                    "property_schema_paths_present"
                ]
            }
            optional_present = {
                path
                for record in schema_records
                for path in record["schema_fixture_coverage"][
                    "optional_property_schema_paths_present"
                ]
            }
            optional_omitted = {
                path
                for record in schema_records
                for path in record["schema_fixture_coverage"][
                    "optional_property_schema_paths_omitted"
                ]
            }
            nullable_value = {
                path
                for record in schema_records
                for path in record["schema_fixture_coverage"][
                    "nullable_value_schema_paths"
                ]
            }
            nullable_null = {
                path
                for record in schema_records
                for path in record["schema_fixture_coverage"][
                    "nullable_null_schema_paths"
                ]
            }
            directions = {
                direction
                for record in schema_records
                for direction in record["schema_fixture_coverage"][
                    "directions_exercised"
                ]
            }
            optional_nullable = expected_nullable & expected_optional
            per_root_evidence = alternative_evidence.get(key.compact())
            if not isinstance(per_root_evidence, dict):
                raise FixtureError(
                    "B3 root lacks exact nested reachability evidence: "
                    f"{key.compact()}"
                )
            required_reachable_ids = set(
                per_root_evidence.get(
                    "reachable_union_fixture_ids", ()
                )
            )
            required_helper_ids = set(
                per_root_evidence.get("helper_union_fixture_ids", ())
            )
            root_positive_fixture_ids = {
                str(record["id"]) for record in records
            }
            facts = {
                "schema_properties_exercised": (
                    expected_properties <= present
                    and required_helper_ids <= root_positive_fixture_ids
                ),
                "optional_present_exercised": (
                    expected_optional <= optional_present
                ),
                "optional_omitted_exercised": (
                    expected_optional <= optional_omitted
                ),
                "nullable_semantics_exercised": (
                    expected_nullable <= nullable_value
                    and expected_nullable <= nullable_null
                    and optional_nullable <= optional_omitted
                ),
                "reachable_union_alternatives_exercised": (
                    required_reachable_ids <= root_positive_fixture_ids
                ),
            }
            required_directions = {"Decode"}
            direction_coverage = required_directions <= directions
            if not all(facts.values()) or not direction_coverage:
                raise FixtureError(
                    "B3 indexed fixture completeness is incomplete for "
                    f"{key.compact()}: facts={facts} "
                    f"directions={sorted(directions)}"
                )
            for record in records:
                record["completeness_evidence"].update(facts)
            indexed_coverage[key.compact()] = {
                "base_fixture_id": base_id,
                "property_schema_paths": sorted(expected_properties),
                "optional_property_schema_paths": sorted(
                    expected_optional
                ),
                "nullable_property_schema_paths": sorted(
                    expected_nullable
                ),
                "required_nullable_property_schema_paths": sorted(
                    expected_nullable - expected_optional
                ),
                "optional_nullable_property_schema_paths": sorted(
                    optional_nullable
                ),
                "required_directions": sorted(required_directions),
                "reachable_union_fixture_ids": sorted(
                    required_reachable_ids
                ),
                "helper_union_fixture_ids": sorted(required_helper_ids),
                "directions_exercised": sorted(directions),
                "schema_direction_coverage": direction_coverage,
                "schema_fixture_facts": facts,
            }

        if (
            len(indexed_coverage) != 50
            or property_count != 289
            or optional_count != 112
            or nullable_count != 111
        ):
            raise FixtureError(
                "B3 indexed schema-path accounting changed: "
                f"identities={len(indexed_coverage)} "
                f"properties={property_count} "
                f"optional={optional_count} "
                f"nullable={nullable_count}"
            )
        self.b3_indexed_coverage = dict(sorted(indexed_coverage.items()))

    def _apply_b4_operation_indexed_completeness(
        self,
        positive_records: Sequence[MutableMapping[str, Any]],
        positive_fixture_ids: set[str],
    ) -> None:
        records_by_key: dict[
            SurfaceKey, list[MutableMapping[str, Any]]
        ] = {}
        for record in positive_records:
            key_record = record.get("protocol_surface_key")
            if isinstance(key_record, dict):
                records_by_key.setdefault(
                    SurfaceKey.from_contract(key_record), []
                ).append(record)

        reachability_by_root: dict[SurfaceKey, set[str]] = {}
        for row in self.reachability.get("records", []):
            if not isinstance(row, dict):
                continue
            surface = row.get("surface_key")
            if not isinstance(surface, dict):
                continue
            nested_key = SurfaceKey.from_contract(surface)
            nested_id = f"union:{nested_key.domain}:{nested_key.name}"
            for root in row.get("reaching_roots", []):
                root_key_record = (
                    root.get("surface_key")
                    if isinstance(root, dict)
                    else None
                )
                if not isinstance(root_key_record, dict):
                    continue
                root_key = SurfaceKey.from_contract(root_key_record)
                if root_key in self.b4_operation_keys:
                    reachability_by_root.setdefault(root_key, set()).add(
                        nested_id
                    )

        item_fixture_ids = {
            f"union:ThreadItem:{name}"
            for name in B3_ITEM_FAMILY_IDENTITIES["ThreadItem"]
        }
        helper_fixture_ids = {
            domain: {
                f"helper-union:{domain}:{name}"
                for name in names
            }
            for domain, (names, _) in B4_HELPER_STRING_UNIONS.items()
        }
        helper_fixture_ids["ThreadListCwdFilter"] = {
            "helper-union:ThreadListCwdFilter:scalar",
            "helper-union:ThreadListCwdFilter:array",
            "helper-union:ThreadListCwdFilter:empty-array",
        }
        known_enum_fixture_ids = {
            domain: {
                f"enum:{domain}:{name}"
                for name in names
            }
            for domain, names in B4_OPEN_STRING_ENUMS.items()
        }
        reference_names = frozenset(
            {
                "ThreadItem",
                *helper_fixture_ids,
                *known_enum_fixture_ids,
            }
        )

        indexed: dict[str, Any] = {}
        for key in self.b4_operation_keys:
            records = [
                record
                for record in records_by_key.get(key, [])
                if isinstance(record.get("schema_fixture_coverage"), dict)
            ]
            root_evidence: dict[str, Any] = {}
            required_external_fixture_ids = set(
                reachability_by_root.get(key, ())
            )

            contract = self.contracts[key]
            root_targets = {
                "params": self.catalog.standalone(
                    str(contract["parameter_type_identity"])
                ),
                "result": self.catalog.standalone(
                    str(
                        contract.get(
                            "result_schema_type_identity",
                            contract["result_type_identity"],
                        )
                    )
                ),
            }
            for root_name, target in root_targets.items():
                base_id = (
                    f"operation:{key.category}:{key.name}:{root_name}"
                )
                base_records = [
                    record for record in records if record["id"] == base_id
                ]
                if len(base_records) != 1:
                    raise FixtureError(
                        "B4 operation lacks exactly one indexed base root: "
                        f"{key.compact()}:{root_name}"
                    )
                root_records = [
                    record
                    for record in records
                    if str(record["id"]).startswith(base_id)
                ]
                base = base_records[0]
                base_coverage = base["schema_fixture_coverage"]
                expected_properties = set(
                    base_coverage["property_schema_paths_present"]
                )
                expected_optional = set(
                    base_coverage[
                        "optional_property_schema_paths_present"
                    ]
                )
                expected_nullable = set(
                    base_coverage["nullable_property_schema_paths"]
                )
                present = {
                    path
                    for record in root_records
                    for path in record["schema_fixture_coverage"][
                        "property_schema_paths_present"
                    ]
                }
                optional_present = {
                    path
                    for record in root_records
                    for path in record["schema_fixture_coverage"][
                        "optional_property_schema_paths_present"
                    ]
                }
                optional_omitted = {
                    path
                    for record in root_records
                    for path in record["schema_fixture_coverage"][
                        "optional_property_schema_paths_omitted"
                    ]
                }
                nullable_value = {
                    path
                    for record in root_records
                    for path in record["schema_fixture_coverage"][
                        "nullable_value_schema_paths"
                    ]
                }
                nullable_null = {
                    path
                    for record in root_records
                    for path in record["schema_fixture_coverage"][
                        "nullable_null_schema_paths"
                    ]
                }
                directions = {
                    direction
                    for record in root_records
                    for direction in record["schema_fixture_coverage"][
                        "directions_exercised"
                    ]
                }
                expected_direction = (
                    "Encode" if root_name == "params" else "Decode"
                )

                base_value = json.loads(
                    self.files[str(base["file"])].decode("utf-8")
                )
                references = collect_referenced_definition_locations(
                    self.catalog,
                    target,
                    base_value,
                    reference_names,
                )
                referenced_names = {
                    location.definition_name for location in references
                }
                if "ThreadItem" in referenced_names:
                    required_external_fixture_ids.update(item_fixture_ids)
                for domain in referenced_names & set(helper_fixture_ids):
                    required_external_fixture_ids.update(
                        helper_fixture_ids[domain]
                    )
                for domain in referenced_names & set(
                    known_enum_fixture_ids
                ):
                    required_external_fixture_ids.update(
                        known_enum_fixture_ids[domain]
                    )

                optional_nullable = (
                    expected_nullable & expected_optional
                )
                facts = {
                    "schema_properties_exercised": (
                        expected_properties <= present
                    ),
                    "optional_present_exercised": (
                        expected_optional <= optional_present
                    ),
                    "optional_omitted_exercised": (
                        expected_optional <= optional_omitted
                    ),
                    "nullable_semantics_exercised": (
                        expected_nullable <= nullable_value
                        and expected_nullable <= nullable_null
                        and optional_nullable <= optional_omitted
                    ),
                    "direction_assertion_exercised": (
                        expected_direction in directions
                    ),
                }
                if not all(facts.values()):
                    raise FixtureError(
                        "B4 operation indexed root coverage is incomplete: "
                        f"{key.compact()}:{root_name}: facts={facts}"
                    )
                root_evidence[root_name] = {
                    "base_fixture_id": base_id,
                    "property_schema_paths": sorted(
                        expected_properties
                    ),
                    "optional_property_schema_paths": sorted(
                        expected_optional
                    ),
                    "nullable_property_schema_paths": sorted(
                        expected_nullable
                    ),
                    "directions_exercised": sorted(directions),
                    "schema_fixture_facts": facts,
                }

            external_complete = (
                required_external_fixture_ids <= positive_fixture_ids
            )
            if not external_complete:
                raise FixtureError(
                    "B4 operation reachable union/helper fixture coverage "
                    f"is incomplete for {key.compact()}: missing="
                    f"{sorted(required_external_fixture_ids - positive_fixture_ids)}"
                )
            facts = {
                "schema_properties_exercised": all(
                    root["schema_fixture_facts"][
                        "schema_properties_exercised"
                    ]
                    for root in root_evidence.values()
                ),
                "optional_present_exercised": all(
                    root["schema_fixture_facts"][
                        "optional_present_exercised"
                    ]
                    for root in root_evidence.values()
                ),
                "optional_omitted_exercised": all(
                    root["schema_fixture_facts"][
                        "optional_omitted_exercised"
                    ]
                    for root in root_evidence.values()
                ),
                "nullable_semantics_exercised": all(
                    root["schema_fixture_facts"][
                        "nullable_semantics_exercised"
                    ]
                    for root in root_evidence.values()
                ),
                "reachable_union_alternatives_exercised": (
                    external_complete
                ),
            }
            for record in records_by_key.get(key, []):
                record["completeness_evidence"].update(facts)
            indexed[key.compact()] = {
                "roots": root_evidence,
                "required_reachable_fixture_ids": sorted(
                    required_external_fixture_ids
                ),
                "schema_fixture_facts": facts,
                "directions_exercised": ["Decode", "Encode"],
                "schema_direction_coverage": True,
            }

        if len(indexed) != 22:
            raise FixtureError(
                "B4 operation indexed coverage must contain exactly 22 roots"
            )
        self.b4_indexed_coverage.update(indexed)
        self.b4_indexed_coverage = dict(
            sorted(self.b4_indexed_coverage.items())
        )

    def _apply_b5_notification_indexed_completeness(
        self,
        positive_records: Sequence[MutableMapping[str, Any]],
        positive_fixture_ids: set[str],
    ) -> None:
        records_by_key: dict[
            SurfaceKey, list[MutableMapping[str, Any]]
        ] = {}
        for record in positive_records:
            key_record = record.get("protocol_surface_key")
            if isinstance(key_record, dict):
                records_by_key.setdefault(
                    SurfaceKey.from_contract(key_record), []
                ).append(record)

        reachability_by_root: dict[SurfaceKey, set[str]] = {}
        for row in self.reachability.get("records", []):
            if not isinstance(row, dict):
                continue
            surface = row.get("surface_key")
            if not isinstance(surface, dict):
                continue
            nested_key = SurfaceKey.from_contract(surface)
            nested_id = f"union:{nested_key.domain}:{nested_key.name}"
            for root in row.get("reaching_roots", []):
                root_key_record = (
                    root.get("surface_key")
                    if isinstance(root, dict)
                    else None
                )
                if not isinstance(root_key_record, dict):
                    continue
                root_key = SurfaceKey.from_contract(root_key_record)
                if root_key in self.b5_notification_keys:
                    reachability_by_root.setdefault(root_key, set()).add(
                        nested_id
                    )

        aggregate = self.catalog.load(
            self.catalog.stable_root
            / "codex_app_server_protocol.schemas.json"
        )
        definitions, edges = definition_graph(aggregate)
        item_fixture_ids = {
            "ThreadItem": {
                f"union:ThreadItem:{name}"
                for name in B3_ITEM_FAMILY_IDENTITIES["ThreadItem"]
            },
            "ResponseItem": {
                f"union:ResponseItem:{name}"
                for name in B3_ITEM_FAMILY_IDENTITIES["ResponseItem"]
            },
        }
        helper_fixture_ids = {
            domain: {
                f"helper-union:{domain}:{name}"
                for name in names
            }
            for domain, (names, _) in B4_HELPER_STRING_UNIONS.items()
        }
        helper_fixture_ids["ThreadListCwdFilter"] = {
            "helper-union:ThreadListCwdFilter:scalar",
            "helper-union:ThreadListCwdFilter:array",
            "helper-union:ThreadListCwdFilter:empty-array",
        }
        all_open_string_enums = {
            **B2_OPEN_STRING_ENUMS,
            **B3_OPEN_STRING_ENUMS,
            **B4_OPEN_STRING_ENUMS,
            **B5_OPEN_STRING_ENUMS,
        }
        known_enum_fixture_ids = {
            domain: {
                f"enum:{domain}:{name}" for name in names
            }
            for domain, names in all_open_string_enums.items()
        }

        indexed: dict[str, Any] = {}
        for key in self.b5_notification_keys:
            records = [
                record
                for record in records_by_key.get(key, [])
                if isinstance(record.get("schema_fixture_coverage"), dict)
            ]
            base_id = f"baseline:{key.compact()}"
            base_records = [
                record for record in records if record["id"] == base_id
            ]
            if len(base_records) != 1:
                raise FixtureError(
                    "B5 notification lacks exactly one indexed base root: "
                    f"{key.compact()}"
                )
            base = base_records[0]
            base_coverage = base["schema_fixture_coverage"]
            expected_properties = set(
                base_coverage["property_schema_paths_present"]
            )
            expected_optional = set(
                base_coverage["optional_property_schema_paths_present"]
            )
            expected_nullable = set(
                base_coverage["nullable_property_schema_paths"]
            )
            present = {
                path
                for record in records
                for path in record["schema_fixture_coverage"][
                    "property_schema_paths_present"
                ]
            }
            optional_present = {
                path
                for record in records
                for path in record["schema_fixture_coverage"][
                    "optional_property_schema_paths_present"
                ]
            }
            optional_omitted = {
                path
                for record in records
                for path in record["schema_fixture_coverage"][
                    "optional_property_schema_paths_omitted"
                ]
            }
            nullable_value = {
                path
                for record in records
                for path in record["schema_fixture_coverage"][
                    "nullable_value_schema_paths"
                ]
            }
            nullable_null = {
                path
                for record in records
                for path in record["schema_fixture_coverage"][
                    "nullable_null_schema_paths"
                ]
            }
            directions = {
                direction
                for record in records
                for direction in record["schema_fixture_coverage"][
                    "directions_exercised"
                ]
            }

            manifest_entry = self.manifest_by_key[key]
            parameter_type = manifest_entry.get("params", {}).get("type")
            if not isinstance(parameter_type, str):
                raise FixtureError(
                    "B5 notification lacks an authoritative params type: "
                    f"{key.compact()}"
                )
            definition = locate_definition_for_type(
                self.catalog, definitions, parameter_type
            )
            if definition is None:
                raise FixtureError(
                    "B5 notification unexpectedly has a Unit params type: "
                    f"{key.compact()}"
                )
            reachable_definitions = transitive_definitions(
                (definition,), edges
            )
            reachable_names = {
                item.name for item in reachable_definitions
            }
            required_external_fixture_ids = set(
                reachability_by_root.get(key, ())
            )
            for domain in reachable_names & set(item_fixture_ids):
                required_external_fixture_ids.update(
                    item_fixture_ids[domain]
                )
            for domain in reachable_names & set(helper_fixture_ids):
                required_external_fixture_ids.update(
                    helper_fixture_ids[domain]
                )
            for domain in reachable_names & set(known_enum_fixture_ids):
                required_external_fixture_ids.update(
                    known_enum_fixture_ids[domain]
                )

            missing_external = (
                required_external_fixture_ids - positive_fixture_ids
            )
            if missing_external:
                raise FixtureError(
                    "B5 notification reachable union/helper fixture "
                    f"coverage is incomplete for {key.compact()}: "
                    f"missing={sorted(missing_external)}"
                )

            optional_nullable = expected_nullable & expected_optional
            facts = {
                "schema_properties_exercised": (
                    expected_properties <= present
                ),
                "optional_present_exercised": (
                    expected_optional <= optional_present
                ),
                "optional_omitted_exercised": (
                    expected_optional <= optional_omitted
                ),
                "nullable_semantics_exercised": (
                    expected_nullable <= nullable_value
                    and expected_nullable <= nullable_null
                    and optional_nullable <= optional_omitted
                ),
                "reachable_union_alternatives_exercised": True,
            }
            direction_assertion_exercised = "Decode" in directions
            if not all(facts.values()) or not direction_assertion_exercised:
                raise FixtureError(
                    "B5 notification indexed root coverage is incomplete: "
                    f"{key.compact()}: facts={facts} "
                    f"direction={direction_assertion_exercised}"
                )
            for record in records_by_key.get(key, []):
                record["completeness_evidence"].update(facts)
            indexed[key.compact()] = {
                "base_fixture_id": base_id,
                "property_schema_paths": sorted(expected_properties),
                "optional_property_schema_paths": sorted(
                    expected_optional
                ),
                "nullable_property_schema_paths": sorted(
                    expected_nullable
                ),
                "reachable_definition_names": sorted(reachable_names),
                "required_reachable_fixture_ids": sorted(
                    required_external_fixture_ids
                ),
                "schema_fixture_facts": facts,
                "directions_exercised": sorted(directions),
                "schema_direction_coverage": True,
            }

        if len(indexed) != 37:
            raise FixtureError(
                "B5 notification indexed coverage must contain exactly 37 "
                "roots"
            )
        self.b5_indexed_coverage = dict(sorted(indexed.items()))

    def _build_operation_fixtures(self) -> None:
        for key, contract in sorted(self.contracts.items()):
            family = "client" if key.category == CLIENT_REQUEST else "server"
            operation_slug = slug(key.name)
            parameter_target = self.catalog.standalone(
                str(contract["parameter_type_identity"])
            )
            parameter_value = self.synthesizer.sample(parameter_target)
            parameter_role = (
                "client_request_params"
                if key.category == CLIENT_REQUEST
                else "server_request_params"
            )
            self.add_positive(
                f"operation:{key.category}:{key.name}:params",
                f"cases/operations/{family}/{operation_slug}/params.json",
                parameter_role,
                parameter_target,
                parameter_value,
                key,
                directions_exercised=(
                    ("Encode",)
                    if key in self.b4_operation_keys
                    else ()
                ),
            )

            # Unit is the canonical operation result contract, while the
            # generator still emits a named empty-object response schema.
            # Validate the authoritative schema root and retain Unit only as
            # the public semantic identity.
            result_schema_type = contract.get(
                "result_schema_type_identity",
                contract["result_type_identity"],
            )
            result_target = self.catalog.standalone(
                str(result_schema_type)
            )
            result_value = self.synthesizer.sample(result_target)
            result_role = (
                "client_request_result"
                if key.category == CLIENT_REQUEST
                else "server_request_response"
            )
            self.add_positive(
                f"operation:{key.category}:{key.name}:result",
                f"cases/operations/{family}/{operation_slug}/result.json",
                result_role,
                result_target,
                result_value,
                key,
                directions_exercised=(
                    ("Decode",)
                    if key in self.b4_operation_keys
                    else ()
                ),
            )

    def _build_b4_operation_supplements(self) -> None:
        coverage: dict[str, Any] = {}
        opaque_exclusions: list[dict[str, str]] = []

        for key in self.b4_operation_keys:
            contract = self.contracts[key]
            roots = (
                (
                    "params",
                    self.catalog.standalone(
                        str(contract["parameter_type_identity"])
                    ),
                    "Encode",
                ),
                (
                    "result",
                    self.catalog.standalone(
                        str(
                            contract.get(
                                "result_schema_type_identity",
                                contract["result_type_identity"],
                            )
                        )
                    ),
                    "Decode",
                ),
            )
            root_records: dict[str, Any] = {}
            for root_name, target, direction in roots:
                base_value = self.synthesizer.sample(target)
                validator = self.catalog.target_validator(target)
                optional_locations = collect_optional_present_locations(
                    self.catalog, target, base_value
                )
                required_locations = collect_required_locations(
                    self.catalog, target, base_value
                )
                container_locations = collect_container_value_locations(
                    self.catalog, target, base_value
                )
                optional_ids: list[str] = []
                nullable_null_ids: list[str] = []
                required_nullable_ids: list[str] = []
                missing_required_ids: list[str] = []
                wrong_type_ids: list[str] = []

                for location in optional_locations:
                    path_name = slug(json_path(location.instance_path))
                    omitted = copy.deepcopy(base_value)
                    parent, field = get_parent_path(
                        omitted, location.instance_path
                    )
                    if (
                        not isinstance(parent, dict)
                        or not isinstance(field, str)
                    ):
                        raise FixtureError(
                            "B4 optional operation field is not an object "
                            f"property: {key.compact()}:{root_name}:"
                            f"{json_path(location.instance_path)}"
                        )
                    parent.pop(field)
                    omitted_id = (
                        f"operation:{key.category}:{key.name}:{root_name}:"
                        f"optional-omitted:{path_name}"
                    )
                    self.add_positive(
                        omitted_id,
                        (
                            "cases/operations/client/"
                            f"{slug(key.name)}/supplements/{root_name}-"
                            f"optional-omitted-{path_name}.json"
                        ),
                        "operation_optional_omitted",
                        target,
                        omitted,
                        key,
                        omitted_schema_paths=(location.schema_path,),
                        directions_exercised=(direction,),
                    )
                    optional_ids.append(omitted_id)

                    if validator.validate_subschema(
                        None, location.schema, location.schema_path
                    ):
                        continue
                    if (
                        get_instance_path(
                            base_value, location.instance_path
                        )
                        is None
                    ):
                        raise FixtureError(
                            "B4 operation base fixture selected null instead "
                            "of the required nullable-value case: "
                            f"{key.compact()}:{root_name}:"
                            f"{json_path(location.instance_path)}"
                        )
                    null_value = copy.deepcopy(base_value)
                    parent, field = get_parent_path(
                        null_value, location.instance_path
                    )
                    parent[field] = None
                    null_id = (
                        f"operation:{key.category}:{key.name}:{root_name}:"
                        f"nullable-null:{path_name}"
                    )
                    self.add_positive(
                        null_id,
                        (
                            "cases/operations/client/"
                            f"{slug(key.name)}/supplements/{root_name}-"
                            f"nullable-null-{path_name}.json"
                        ),
                        "operation_nullable_null",
                        target,
                        null_value,
                        key,
                        directions_exercised=(direction,),
                    )
                    nullable_null_ids.append(null_id)

                for location in required_locations:
                    path_name = slug(json_path(location.instance_path))
                    missing = copy.deepcopy(base_value)
                    parent, field = get_parent_path(
                        missing, location.instance_path
                    )
                    if (
                        not isinstance(parent, dict)
                        or not isinstance(field, str)
                    ):
                        raise FixtureError(
                            "B4 required operation field is not an object "
                            f"property: {key.compact()}:{root_name}:"
                            f"{json_path(location.instance_path)}"
                        )
                    parent.pop(field)
                    diagnostics = validator.validate_subschema(
                        missing, target.schema, target.schema_path
                    )
                    codes = sorted({item.code for item in diagnostics})
                    if not codes:
                        raise FixtureError(
                            "B4 required operation removal was accepted: "
                            f"{key.compact()}:{root_name}:"
                            f"{json_path(location.instance_path)}"
                        )
                    missing_id = (
                        f"operation:{key.category}:{key.name}:{root_name}:"
                        f"missing-required:{path_name}"
                    )
                    self.add_negative(
                        missing_id,
                        (
                            "cases/operations/client/"
                            f"{slug(key.name)}/mutations/{root_name}-"
                            f"missing-required-{path_name}.json"
                        ),
                        "operation_missing_required",
                        target,
                        missing,
                        codes,
                        key,
                        "missing_required",
                    )
                    missing_required_ids.append(missing_id)

                    if validator.validate_subschema(
                        None, location.schema, location.schema_path
                    ):
                        continue
                    if (
                        get_instance_path(
                            base_value, location.instance_path
                        )
                        is None
                    ):
                        raise FixtureError(
                            "B4 required nullable operation field selected "
                            "null in its value fixture: "
                            f"{key.compact()}:{root_name}:"
                            f"{json_path(location.instance_path)}"
                        )
                    null_value = copy.deepcopy(base_value)
                    parent, field = get_parent_path(
                        null_value, location.instance_path
                    )
                    parent[field] = None
                    null_id = (
                        f"operation:{key.category}:{key.name}:{root_name}:"
                        f"required-nullable-null:{path_name}"
                    )
                    self.add_positive(
                        null_id,
                        (
                            "cases/operations/client/"
                            f"{slug(key.name)}/supplements/{root_name}-"
                            f"required-nullable-null-{path_name}.json"
                        ),
                        "operation_nullable_null",
                        target,
                        null_value,
                        key,
                        directions_exercised=(direction,),
                    )
                    required_nullable_ids.append(null_id)

                wrong_locations: dict[
                    tuple[str | int, ...], tuple[Any, str]
                ] = {}
                for location in (*required_locations, *optional_locations):
                    wrong_locations.setdefault(
                        location.instance_path,
                        (location.schema, location.schema_path),
                    )
                for location in container_locations:
                    wrong_locations.setdefault(
                        location.instance_path,
                        (location.schema, location.schema_path),
                    )

                for instance_path in sorted(
                    wrong_locations,
                    key=lambda value: tuple(map(str, value)),
                ):
                    schema, schema_path = wrong_locations[instance_path]
                    original = get_instance_path(base_value, instance_path)
                    selected: Any = NO_WRONG_TYPE_MUTATION
                    selected_codes: list[str] = []
                    for candidate in WRONG_TYPE_CANDIDATES:
                        if canonical_json(candidate) == canonical_json(
                            original
                        ):
                            continue
                        mutated = copy.deepcopy(base_value)
                        parent, field = get_parent_path(
                            mutated, instance_path
                        )
                        parent[field] = copy.deepcopy(candidate)
                        diagnostics = validator.validate_subschema(
                            mutated, target.schema, target.schema_path
                        )
                        codes = sorted(
                            {item.code for item in diagnostics}
                        )
                        if codes:
                            selected = mutated
                            selected_codes = codes
                            break
                    if selected is NO_WRONG_TYPE_MUTATION:
                        opaque_exclusions.append(
                            {
                                "operation": key.name,
                                "root": root_name,
                                "instance_path": json_path(instance_path),
                                "schema_path": schema_path,
                                "reason": (
                                    "the pinned protocol schema accepts "
                                    "semantically arbitrary JSON at this path"
                                ),
                            }
                        )
                        continue
                    path_name = slug(json_path(instance_path))
                    wrong_id = (
                        f"operation:{key.category}:{key.name}:{root_name}:"
                        f"wrong-type:{path_name}"
                    )
                    self.add_negative(
                        wrong_id,
                        (
                            "cases/operations/client/"
                            f"{slug(key.name)}/mutations/{root_name}-"
                            f"wrong-type-{path_name}.json"
                        ),
                        "operation_wrong_type",
                        target,
                        selected,
                        selected_codes,
                        key,
                        (
                            "wrong_container_value_type"
                            if any(
                                location.instance_path == instance_path
                                for location in container_locations
                            )
                            else "wrong_property_type"
                        ),
                    )
                    wrong_type_ids.append(wrong_id)

                root_records[root_name] = {
                    "base_fixture_id": (
                        f"operation:{key.category}:{key.name}:{root_name}"
                    ),
                    "direction": direction,
                    "optional_omitted_fixture_ids": sorted(optional_ids),
                    "nullable_null_fixture_ids": sorted(nullable_null_ids),
                    "required_nullable_null_fixture_ids": sorted(
                        required_nullable_ids
                    ),
                    "missing_required_fixture_ids": sorted(
                        missing_required_ids
                    ),
                    "wrong_type_fixture_ids": sorted(wrong_type_ids),
                }

            coverage[key.compact()] = {
                "roots": root_records,
            }

        for method in (
            "thread/fork",
            "thread/resume",
            "thread/start",
        ):
            key = next(
                candidate
                for candidate in self.b4_operation_keys
                if candidate.name == method
            )
            contract = self.contracts[key]
            target = self.catalog.standalone(
                str(contract["parameter_type_identity"])
            )
            value = self.synthesizer.sample(target)
            if not isinstance(value, dict) or "config" not in value:
                raise FixtureError(
                    f"{method} base params no longer exercise config"
                )
            value["config"] = {
                "syntheticKey": {
                    "protocolOpaque": True,
                    "nested": [1, "two", None],
                }
            }
            fixture_id = (
                f"operation:client_request:{method}:params:"
                "opaque-config-map-value"
            )
            self.add_positive(
                fixture_id,
                (
                    f"cases/operations/client/{slug(method)}/supplements/"
                    "params-opaque-config-map-value.json"
                ),
                "operation_opaque_value",
                target,
                value,
                key,
                directions_exercised=("Encode",),
            )
            opaque_exclusions.append(
                {
                    "operation": method,
                    "root": "params",
                    "instance_path": "$/config/syntheticKey",
                    "schema_path": (
                        "#/properties/config/additionalProperties"
                    ),
                    "reason": (
                        "the pinned typed configuration map declares "
                        "semantically arbitrary JSON values"
                    ),
                }
            )

        # The deprecated rollback operation retains the exact upstream uint32
        # contract: zero is valid, and the upper bound is inclusive.
        rollback_key = next(
            key
            for key in self.b4_operation_keys
            if key.name == "thread/rollback"
        )
        rollback_target = self.catalog.standalone("ThreadRollbackParams")
        rollback_base = self.synthesizer.sample(rollback_target)
        rollback_validator = self.catalog.target_validator(rollback_target)
        for case, value, schema_valid, codec_valid in (
            ("zero", 0, True, True),
            ("maximum", 4_294_967_295, True, True),
            # JSON Schema Draft-07 treats ``format`` as an annotation.  The
            # pinned format:uint32 still binds the handwritten typed codec,
            # so overflow is an intentionally schema-valid semantic-negative.
            ("overflow", 4_294_967_296, True, False),
            ("negative", -1, False, False),
            ("fractional", 0.5, False, False),
        ):
            sample = copy.deepcopy(rollback_base)
            sample["numTurns"] = value
            fixture_id = (
                "operation:client_request:thread/rollback:params:"
                f"uint32-{case}"
            )
            relative = (
                "cases/operations/client/thread-rollback/"
                f"{'supplements' if schema_valid else 'mutations'}/"
                f"params-uint32-{case}.json"
            )
            if schema_valid:
                self.add_positive(
                    fixture_id,
                    relative,
                    (
                        "operation_numeric_boundary"
                        if codec_valid
                        else "operation_pinned_format_unrepresentable"
                    ),
                    rollback_target,
                    sample,
                    rollback_key,
                    directions_exercised=(
                        ("Encode",) if codec_valid else ()
                    ),
                )
                if not codec_valid:
                    self.records[-1]["typed_state_boundary"] = {
                        "representable": False,
                        "production_diagnostic_expected": False,
                        "schema_path": "#/properties/numTurns",
                        "format": "uint32",
                        "maximum_representable": 4_294_967_295,
                        "production_evidence": [
                            "ThreadRollbackParams::numTurns is std::uint32_t",
                            "CodexA11OperationWireTest encodes uint32 maximum exactly",
                            "compile-time numeric_limits<uint32_t> ratchet",
                        ],
                        "reason": (
                            "Draft-07 format is annotative; the pinned "
                            "format:uint32 value is outside the public typed "
                            "state and therefore cannot produce a runtime "
                            "codec diagnostic"
                        ),
                    }
            else:
                diagnostics = rollback_validator.validate_subschema(
                    sample,
                    rollback_target.schema,
                    rollback_target.schema_path,
                )
                codes = sorted({item.code for item in diagnostics})
                if not codes:
                    raise FixtureError(
                        "ThreadRollbackParams uint32 boundary mutation was "
                        f"accepted: {case}"
                    )
                self.add_negative(
                    fixture_id,
                    relative,
                    "operation_numeric_boundary_invalid",
                    rollback_target,
                    sample,
                    codes,
                    rollback_key,
                    f"uint32_{case}",
                )

        self.b4_operation_root_coverage = dict(sorted(coverage.items()))
        self.b4_negative_coverage["operation_opaque_exclusions"] = sorted(
            opaque_exclusions,
            key=lambda record: (
                record["operation"],
                record["root"],
                record["instance_path"],
            ),
        )
        if len(opaque_exclusions) != 5:
            raise FixtureError(
                "B4 operation opaque-path accounting must remain exactly "
                "three config map values, injected items, and outputSchema"
            )

    def _build_b4_helper_union_fixtures(self) -> None:
        coverage: dict[str, Any] = {}
        for domain, (known_values, directions) in sorted(
            B4_HELPER_STRING_UNIONS.items()
        ):
            target = self.catalog.union_target(domain)
            known_ids: list[str] = []
            for value in known_values:
                fixture_id = f"helper-union:{domain}:{value}"
                self.add_positive(
                    fixture_id,
                    (
                        f"cases/unions/{slug(domain)}/"
                        f"{slug(value)}.json"
                    ),
                    "operation_helper_union_branch",
                    target,
                    value,
                    None,
                    directions_exercised=directions,
                )
                known_ids.append(fixture_id)
            future_ids: list[str] = []
            for case, value in (
                ("future-unknown", f"future{domain}"),
                ("empty-unknown", ""),
            ):
                fixture_id = f"helper-union:{domain}:{case}"
                self.add_negative(
                    fixture_id,
                    (
                        f"cases/unions/{slug(domain)}/"
                        f"{case}.json"
                    ),
                    "unknown_enum_value",
                    target,
                    value,
                    ["one_of_zero"],
                    negative_case=case.replace("-", "_"),
                )
                future_ids.append(fixture_id)
            coverage[domain] = {
                "known_value_fixture_ids": known_ids,
                "future_value_fixture_ids": future_ids,
                "directions_exercised": list(directions),
            }

        cwd_target = self.catalog.union_target("ThreadListCwdFilter")
        cwd_cases = {
            "scalar": "/synthetic/workspace",
            "array": [
                "/synthetic/workspace-a",
                "/synthetic/workspace-b",
            ],
            "empty-array": [],
        }
        cwd_ids: list[str] = []
        for case, value in cwd_cases.items():
            fixture_id = f"helper-union:ThreadListCwdFilter:{case}"
            self.add_positive(
                fixture_id,
                (
                    "cases/unions/threadlistcwdfilter/"
                    f"{case}.json"
                ),
                "operation_helper_union_branch",
                cwd_target,
                value,
                None,
                directions_exercised=("Encode",),
            )
            cwd_ids.append(fixture_id)
        wrong_cwd = [0]
        cwd_diagnostics = self.catalog.target_validator(
            cwd_target
        ).validate_subschema(
            wrong_cwd, cwd_target.schema, cwd_target.schema_path
        )
        cwd_codes = sorted({item.code for item in cwd_diagnostics})
        self.add_negative(
            "helper-union:ThreadListCwdFilter:wrong-array-item",
            (
                "cases/unions/threadlistcwdfilter/"
                "wrong-array-item.json"
            ),
            "operation_wrong_type",
            cwd_target,
            wrong_cwd,
            cwd_codes,
            negative_case="wrong_array_element_type",
        )
        coverage["ThreadListCwdFilter"] = {
            "known_branch_fixture_ids": cwd_ids,
            "wrong_array_item_fixture_id": (
                "helper-union:ThreadListCwdFilter:wrong-array-item"
            ),
            "directions_exercised": ["Encode"],
        }
        self.b4_negative_coverage["helper_unions"] = dict(
            sorted(coverage.items())
        )

    def _build_baseline_fixtures(self) -> None:
        keys = sorted(
            SurfaceKey(category, domain, field, name)
            for category, domain, field, name in EXISTING_TYPED_FIXTURE_IDENTITIES
        )
        if len(keys) != 34 or len(set(keys)) != 34:
            raise FixtureError(
                "EXISTING_TYPED_FIXTURE_IDENTITIES must contain exactly "
                "34 unique reviewed identities"
            )
        for key in keys:
            if key.category == ITEM_DISCRIMINATOR:
                target = self.catalog.union_target(key.domain)
                index, branch, forced_scalar, forced_property = (
                    branch_for_union_identity(
                        self.catalog,
                        target,
                        key.discriminator_field,
                        key.name,
                    )
                )
                value = self.synthesizer.sample(
                    target,
                    branch,
                    pointer_child(
                        pointer_child(target.schema_path, "oneOf"), index
                    ),
                    forced_scalar,
                    forced_property,
                )
                self.add_positive(
                    f"baseline:{key.compact()}",
                    (
                        f"cases/baseline/{key.category}/"
                        f"{slug(key.domain)}-{slug(key.name)}.json"
                    ),
                    "existing_typed_identity",
                    target,
                    value,
                    key,
                    (index,),
                )
                continue

            target, index, branch = self.catalog.method_target(
                key.category, key.name
            )
            value = self.synthesizer.sample(
                target,
                branch,
                pointer_child(pointer_child("#", "oneOf"), index),
            )
            self.add_positive(
                f"baseline:{key.compact()}",
                (
                    f"cases/baseline/{key.category}/"
                    f"{slug(key.name)}.json"
                ),
                "existing_typed_identity",
                target,
                value,
                key,
                (index,),
                directions_exercised=(
                    ("Decode",)
                    if key in self.b5_notification_keys
                    else ()
                ),
            )

    def _build_b5_notification_fixtures(self) -> None:
        existing_keys = {
            SurfaceKey(category, domain, field, name)
            for category, domain, field, name
            in EXISTING_TYPED_FIXTURE_IDENTITIES
            if category == SERVER_NOTIFICATION
        }
        existing_b5 = existing_keys & set(self.b5_notification_keys)
        if len(existing_b5) != 12:
            raise FixtureError(
                "B5 must reuse exactly 12 existing notification fixtures"
            )

        generated_bases = 0
        coverage: dict[str, Any] = {}
        opaque_exclusions: list[dict[str, str]] = []
        counts = {
            "base_generated": 0,
            "optional_omitted": 0,
            "nullable_null": 0,
            "required_nullable_null": 0,
            "missing_required": 0,
            "wrong_type": 0,
            "wrong_type_opaque_exclusions": 0,
        }

        for key in self.b5_notification_keys:
            target, index, branch = self.catalog.method_target(
                key.category, key.name
            )
            branch_path = pointer_child(pointer_child("#", "oneOf"), index)
            base_value = self.synthesizer.sample(
                target, branch, branch_path
            )
            normalize_b5_notification_sample(key, base_value)
            base_id = f"baseline:{key.compact()}"
            base_file = (
                f"cases/baseline/{key.category}/{slug(key.name)}.json"
            )
            if key not in existing_b5:
                self.add_positive(
                    base_id,
                    base_file,
                    "server_notification_identity",
                    target,
                    base_value,
                    key,
                    (index,),
                    directions_exercised=("Decode",),
                )
                generated_bases += 1
                counts["base_generated"] += 1

            validator = self.catalog.target_validator(target)
            optional_locations = collect_optional_present_locations(
                self.catalog, target, base_value
            )
            required_locations = collect_required_locations(
                self.catalog, target, base_value
            )
            container_locations = collect_container_value_locations(
                self.catalog, target, base_value
            )
            optional_ids: list[str] = []
            nullable_null_ids: list[str] = []
            required_nullable_ids: list[str] = []
            missing_required_ids: list[str] = []
            wrong_type_ids: list[str] = []

            for location in optional_locations:
                path_name = slug(json_path(location.instance_path))
                omitted = copy.deepcopy(base_value)
                parent, field = get_parent_path(
                    omitted, location.instance_path
                )
                if not isinstance(parent, dict) or not isinstance(field, str):
                    raise FixtureError(
                        "B5 optional notification field is not an object "
                        f"property: {key.compact()}:"
                        f"{json_path(location.instance_path)}"
                    )
                parent.pop(field)
                fixture_id = f"{base_id}:optional-omitted:{path_name}"
                self.add_positive(
                    fixture_id,
                    (
                        "cases/notifications/server/"
                        f"{slug(key.name)}/supplements/"
                        f"optional-omitted-{path_name}.json"
                    ),
                    "notification_optional_omitted",
                    target,
                    omitted,
                    key,
                    (index,),
                    omitted_schema_paths=(location.schema_path,),
                    directions_exercised=("Decode",),
                )
                optional_ids.append(fixture_id)
                counts["optional_omitted"] += 1

                if validator.validate_subschema(
                    None, location.schema, location.schema_path
                ):
                    continue
                if (
                    get_instance_path(base_value, location.instance_path)
                    is None
                ):
                    raise FixtureError(
                        "B5 notification base fixture selected null instead "
                        "of the required nullable-value case: "
                        f"{key.compact()}:"
                        f"{json_path(location.instance_path)}"
                    )
                null_value = copy.deepcopy(base_value)
                parent, field = get_parent_path(
                    null_value, location.instance_path
                )
                parent[field] = None
                null_id = f"{base_id}:nullable-null:{path_name}"
                self.add_positive(
                    null_id,
                    (
                        "cases/notifications/server/"
                        f"{slug(key.name)}/supplements/"
                        f"nullable-null-{path_name}.json"
                    ),
                    "notification_nullable_null",
                    target,
                    null_value,
                    key,
                    (index,),
                    directions_exercised=("Decode",),
                )
                nullable_null_ids.append(null_id)
                counts["nullable_null"] += 1

            for location in required_locations:
                path_name = slug(json_path(location.instance_path))
                missing = copy.deepcopy(base_value)
                parent, field = get_parent_path(
                    missing, location.instance_path
                )
                if not isinstance(parent, dict) or not isinstance(field, str):
                    raise FixtureError(
                        "B5 required notification field is not an object "
                        f"property: {key.compact()}:"
                        f"{json_path(location.instance_path)}"
                    )
                parent.pop(field)
                diagnostics = validator.validate_subschema(
                    missing, target.schema, target.schema_path
                )
                codes = sorted({item.code for item in diagnostics})
                if not codes:
                    raise FixtureError(
                        "B5 required notification removal was accepted: "
                        f"{key.compact()}:"
                        f"{json_path(location.instance_path)}"
                    )
                missing_id = f"{base_id}:missing-required:{path_name}"
                self.add_negative(
                    missing_id,
                    (
                        "cases/notifications/server/"
                        f"{slug(key.name)}/mutations/"
                        f"missing-required-{path_name}.json"
                    ),
                    "notification_missing_required",
                    target,
                    missing,
                    codes,
                    key,
                    "missing_required",
                )
                missing_required_ids.append(missing_id)
                counts["missing_required"] += 1

                if validator.validate_subschema(
                    None, location.schema, location.schema_path
                ):
                    continue
                if (
                    get_instance_path(base_value, location.instance_path)
                    is None
                ):
                    raise FixtureError(
                        "B5 required nullable notification field selected "
                        "null in its value fixture: "
                        f"{key.compact()}:"
                        f"{json_path(location.instance_path)}"
                    )
                null_value = copy.deepcopy(base_value)
                parent, field = get_parent_path(
                    null_value, location.instance_path
                )
                parent[field] = None
                null_id = (
                    f"{base_id}:required-nullable-null:{path_name}"
                )
                self.add_positive(
                    null_id,
                    (
                        "cases/notifications/server/"
                        f"{slug(key.name)}/supplements/"
                        f"required-nullable-null-{path_name}.json"
                    ),
                    "notification_nullable_null",
                    target,
                    null_value,
                    key,
                    (index,),
                    directions_exercised=("Decode",),
                )
                required_nullable_ids.append(null_id)
                counts["required_nullable_null"] += 1

            wrong_locations: dict[
                tuple[str | int, ...], tuple[Any, str]
            ] = {}
            for location in (*required_locations, *optional_locations):
                wrong_locations.setdefault(
                    location.instance_path,
                    (location.schema, location.schema_path),
                )
            for location in container_locations:
                wrong_locations.setdefault(
                    location.instance_path,
                    (location.schema, location.schema_path),
                )

            for instance_path in sorted(
                wrong_locations,
                key=lambda value: tuple(map(str, value)),
            ):
                schema, schema_path = wrong_locations[instance_path]
                original = get_instance_path(base_value, instance_path)
                selected: Any = NO_WRONG_TYPE_MUTATION
                selected_codes: list[str] = []
                for candidate in WRONG_TYPE_CANDIDATES:
                    if canonical_json(candidate) == canonical_json(original):
                        continue
                    mutated = copy.deepcopy(base_value)
                    parent, field = get_parent_path(
                        mutated, instance_path
                    )
                    parent[field] = copy.deepcopy(candidate)
                    diagnostics = validator.validate_subschema(
                        mutated, target.schema, target.schema_path
                    )
                    codes = sorted({item.code for item in diagnostics})
                    if codes:
                        selected = mutated
                        selected_codes = codes
                        break
                if selected is NO_WRONG_TYPE_MUTATION:
                    opaque_exclusions.append(
                        {
                            "notification": key.name,
                            "instance_path": json_path(instance_path),
                            "schema_path": schema_path,
                            "reason": (
                                "the pinned protocol schema accepts "
                                "semantically arbitrary JSON at this path"
                            ),
                        }
                    )
                    counts["wrong_type_opaque_exclusions"] += 1
                    continue
                path_name = slug(json_path(instance_path))
                wrong_id = f"{base_id}:wrong-type:{path_name}"
                self.add_negative(
                    wrong_id,
                    (
                        "cases/notifications/server/"
                        f"{slug(key.name)}/mutations/"
                        f"wrong-type-{path_name}.json"
                    ),
                    "notification_wrong_type",
                    target,
                    selected,
                    selected_codes,
                    key,
                    (
                        "wrong_container_value_type"
                        if any(
                            location.instance_path == instance_path
                            for location in container_locations
                        )
                        else "wrong_property_type"
                    ),
                )
                wrong_type_ids.append(wrong_id)
                counts["wrong_type"] += 1

            coverage[key.compact()] = {
                "base_fixture_id": base_id,
                "base_fixture_reused": key in existing_b5,
                "optional_omitted_fixture_ids": sorted(optional_ids),
                "nullable_null_fixture_ids": sorted(nullable_null_ids),
                "required_nullable_null_fixture_ids": sorted(
                    required_nullable_ids
                ),
                "missing_required_fixture_ids": sorted(
                    missing_required_ids
                ),
                "wrong_type_fixture_ids": sorted(wrong_type_ids),
                "future_method_fixture_id": (
                    "method:server_notification:future-unknown"
                ),
                "conflicting_discriminator_fixture_ids": [],
                "conflicting_discriminator_exclusion": (
                    "the pinned JSON representation has exactly one method "
                    "property, so multiple discriminator values are not "
                    "structurally representable"
                ),
            }

        if generated_bases != 25:
            raise FixtureError(
                "B5 must generate exactly 25 new notification base fixtures"
            )
        expected_counts = {
            "base_generated": 25,
            "missing_required": 279,
            "nullable_null": 57,
            "optional_omitted": 65,
            "required_nullable_null": 2,
            "wrong_type": 358,
            "wrong_type_opaque_exclusions": 2,
        }
        if counts != expected_counts:
            raise FixtureError(
                "B5 notification fixture/mutation accounting changed: "
                f"{dict(sorted(counts.items()))}"
            )
        expected_opaque_paths = {
            (
                "thread/realtime/itemAdded",
                "$/params/item",
                (
                    "#/definitions/ThreadRealtimeItemAddedNotification/"
                    "properties/item"
                ),
            ),
            (
                "turn/moderationMetadata",
                "$/params/metadata",
                (
                    "#/definitions/TurnModerationMetadataNotification/"
                    "properties/metadata"
                ),
            ),
        }
        actual_opaque_paths = {
            (
                record["notification"],
                record["instance_path"],
                record["schema_path"],
            )
            for record in opaque_exclusions
        }
        if actual_opaque_paths != expected_opaque_paths:
            raise FixtureError(
                "B5 protocol-defined opaque notification paths changed: "
                f"{sorted(actual_opaque_paths)}"
            )
        self.b5_notification_root_coverage = dict(
            sorted(coverage.items())
        )
        self.b5_negative_coverage["payload_mutations"] = {
            "counts": dict(sorted(counts.items())),
            "opaque_exclusions": sorted(
                opaque_exclusions,
                key=lambda record: (
                    record["notification"],
                    record["instance_path"],
                ),
            ),
        }

    def _build_union_fixtures(self) -> None:
        codex_error_target = self.catalog.union_target("CodexErrorInfo")
        validate_codex_error_rule_sets(
            self.manifest, codex_error_target.schema
        )
        known_union_values: dict[tuple[str, str], tuple[SchemaTarget, Any]] = {}
        b2_domains = tuple(
            sorted({key.domain for key in self.b2_shared_common_keys})
        )
        b3_domains = tuple(sorted({key.domain for key in self.b3_item_keys}))
        b4_domains = tuple(
            sorted({key.domain for key in self.b4_operation_union_keys})
        )
        for domain in (
            "CodexErrorInfo",
            *b2_domains,
            *(
                candidate
                for candidate in b3_domains
                if candidate not in b2_domains
            ),
            *(
                candidate
                for candidate in b4_domains
                if candidate not in b2_domains
                and candidate not in b3_domains
            ),
        ):
            target = self.catalog.union_target(domain)
            if domain in b2_domains:
                identities = sorted(
                    (
                        key
                        for key in self.b2_shared_common_keys
                        if key.domain == domain
                    ),
                    key=lambda key: key.name,
                )
            elif domain in b3_domains:
                identities = sorted(
                    (
                        key
                        for key in self.b3_item_keys
                        if key.domain == domain
                    ),
                    key=lambda key: key.name,
                )
            elif domain in b4_domains:
                identities = sorted(
                    (
                        key
                        for key in self.b4_operation_union_keys
                        if key.domain == domain
                    ),
                    key=lambda key: key.name,
                )
            else:
                identities = sorted(
                    (
                        SurfaceKey.from_manifest(entry)
                        for entry in self.manifest["entries"]
                        if entry["stability"] == "stable"
                        and entry["domain"] == domain
                        and entry["category"]
                        in {
                            ITEM_DISCRIMINATOR,
                            TAGGED_UNION_DISCRIMINATOR,
                        }
                    ),
                    key=lambda key: key.name,
                )
            for key in identities:
                index, branch, forced_scalar, forced_property = (
                    branch_for_union_identity(
                        self.catalog,
                        target,
                        key.discriminator_field,
                        key.name,
                    )
                )
                keyword = (
                    "oneOf"
                    if isinstance(target.schema, dict)
                    and "oneOf" in target.schema
                    else "anyOf"
                )
                branch_path = pointer_child(
                    pointer_child(target.schema_path, keyword), index
                )
                value = self.synthesizer.sample(
                    target,
                    branch,
                    branch_path,
                    forced_scalar,
                    forced_property,
                )
                known_union_values[(domain, key.name)] = (
                    target,
                    copy.deepcopy(value),
                )
                intended = (index,) if keyword == "oneOf" else ()
                self.add_positive(
                    f"union:{domain}:{key.name}",
                    (
                        f"cases/unions/{slug(domain)}/"
                        f"{slug(key.name)}.json"
                    ),
                    "union_branch",
                    target,
                    value,
                    key,
                    intended,
                    directions_exercised=(
                        B2_SHARED_COMMON_DIRECTIONS[domain]
                        if domain in B2_SHARED_COMMON_DIRECTIONS
                        else ("Decode",)
                        if domain in b3_domains or domain in b4_domains
                        else ()
                    ),
                )

        self._build_b2_union_supplements(known_union_values)
        self._build_b3_union_supplements(known_union_values)
        self._build_b2_union_supplements(
            known_union_values,
            keys=self.b4_operation_union_keys,
            directions_by_domain=B4_OPERATION_UNION_DIRECTIONS,
            batch="B4",
        )
        self._build_b4_conversation_negative_supplements(
            known_union_values
        )

        target = codex_error_target
        for name in CODEX_ERROR_INFO_HTTP_IDENTITIES:
            key = SurfaceKey(
                TAGGED_UNION_DISCRIMINATOR,
                "CodexErrorInfo",
                "$variant",
                name,
            )
            index, _, _, _ = branch_for_union_identity(
                self.catalog, target, "$variant", name
            )
            for case, nested in (
                ("http-status-null", {"httpStatusCode": None}),
                ("http-status-omitted", {}),
            ):
                self.add_positive(
                    f"union:CodexErrorInfo:{name}:{case}",
                    (
                        "cases/unions/codexerrorinfo/"
                        f"{slug(name)}-{case}.json"
                    ),
                    "union_branch_supplement",
                    target,
                    {name: nested},
                    key,
                    (index,),
                )

        active_key = SurfaceKey(
            TAGGED_UNION_DISCRIMINATOR,
            "CodexErrorInfo",
            "$variant",
            "activeTurnNotSteerable",
        )
        active_index, _, _, _ = branch_for_union_identity(
            self.catalog, target, "$variant", "activeTurnNotSteerable"
        )
        self.add_positive(
            "union:CodexErrorInfo:activeTurnNotSteerable:turn-kind-compact",
            "cases/unions/codexerrorinfo/activeturnnotsteerable-turn-kind-compact.json",
            "union_branch_supplement",
            target,
            {"activeTurnNotSteerable": {"turnKind": "compact"}},
            active_key,
            (active_index,),
        )

        self.add_negative(
            "union:CodexErrorInfo:future-unknown",
            "cases/unions/codexerrorinfo/future-unknown.json",
            "unknown_discriminator",
            target,
            "futureCodexErrorInfo",
            ["one_of_zero"],
        )
        self.add_negative(
            "union:CodexErrorInfo:activeTurnNotSteerable:future-turn-kind",
            "cases/unions/codexerrorinfo/activeturnnotsteerable-future-turn-kind.json",
            "unknown_enum_value",
            target,
            {"activeTurnNotSteerable": {"turnKind": "futureTurnKind"}},
            ["one_of_zero"],
        )

        for domain, known_name, future_name in (
            ("ThreadItem", "agentMessage", "futureThreadItem"),
            ("ResponseItem", "agent_message", "future_response_item"),
        ):
            union_target, known_value = known_union_values[(domain, known_name)]
            if not isinstance(known_value, dict) or "type" not in known_value:
                raise FixtureError(
                    f"{domain}:{known_name} lacks the reviewed type discriminator"
                )
            known_value["type"] = future_name
            self.add_negative(
                f"union:{domain}:future-unknown",
                (
                    f"cases/unions/{slug(domain)}/"
                    "future-unknown.json"
                ),
                "unknown_discriminator",
                union_target,
                known_value,
                ["one_of_zero"],
            )

        for category, type_identity, future_method in (
            (CLIENT_REQUEST, "ClientRequest", "future/clientRequest"),
            (
                CLIENT_NOTIFICATION,
                "ClientNotification",
                "future/clientNotification",
            ),
            (SERVER_REQUEST, "ServerRequest", "future/serverRequest"),
            (
                SERVER_NOTIFICATION,
                "ServerNotification",
                "future/serverNotification",
            ),
        ):
            method_target = self.catalog.standalone(type_identity)
            known_value = self.synthesizer.sample(method_target)
            if not isinstance(known_value, dict) or "method" not in known_value:
                raise FixtureError(
                    f"{type_identity} sample lacks the reviewed method discriminator"
                )
            known_value["method"] = future_method
            self.add_negative(
                f"method:{category}:future-unknown",
                f"cases/mutations/{slug(type_identity)}/future-method.json",
                "unknown_method",
                method_target,
                known_value,
                ["one_of_zero"],
            )

        thread_target, malformed_thread = known_union_values[
            ("ThreadItem", "agentMessage")
        ]
        if not isinstance(malformed_thread, dict) or "id" not in malformed_thread:
            raise FixtureError("ThreadItem:agentMessage sample lacks required id")
        malformed_thread["id"] = 0
        self.add_negative(
            "union:ThreadItem:nested-wrong-type",
            "cases/unions/threaditem/nested-wrong-type.json",
            "nested_union_failure",
            thread_target,
            malformed_thread,
            ["one_of_zero"],
        )

        response_target, malformed_response = known_union_values[
            ("ResponseItem", "agent_message")
        ]
        if (
            not isinstance(malformed_response, dict)
            or "content" not in malformed_response
        ):
            raise FixtureError(
                "ResponseItem:agent_message sample lacks required content"
            )
        malformed_response["content"] = 0
        self.add_negative(
            "union:ResponseItem:nested-wrong-type",
            "cases/unions/responseitem/nested-wrong-type.json",
            "nested_union_failure",
            response_target,
            malformed_response,
            ["one_of_zero"],
        )
        self.add_negative(
            "union:CodexErrorInfo:malformed-known",
            "cases/unions/codexerrorinfo/malformed-known.json",
            "malformed_known",
            target,
            {"activeTurnNotSteerable": {}},
            ["one_of_zero"],
        )
        self.add_negative(
            "union:CodexErrorInfo:nested-wrong-type",
            "cases/unions/codexerrorinfo/nested-wrong-type.json",
            "nested_union_failure",
            target,
            {"activeTurnNotSteerable": {"turnKind": 0}},
            ["one_of_zero"],
        )

    def _build_b2_union_supplements(
        self,
        known_union_values: Mapping[
            tuple[str, str], tuple[SchemaTarget, Any]
        ],
        *,
        keys: Sequence[SurfaceKey] | None = None,
        directions_by_domain: Mapping[str, tuple[str, ...]] | None = None,
        batch: str = "B2",
    ) -> None:
        selected_keys = (
            tuple(keys)
            if keys is not None
            else self.b2_shared_common_keys
        )
        selected_directions = (
            directions_by_domain
            if directions_by_domain is not None
            else B2_SHARED_COMMON_DIRECTIONS
        )
        alternative_coverage: dict[str, Any] = {}
        family_coverage: dict[str, Any] = {}

        for key in selected_keys:
            target, base_value = known_union_values[(key.domain, key.name)]
            index, _, _, _ = branch_for_union_identity(
                self.catalog,
                target,
                key.discriminator_field,
                key.name,
            )
            intended = (index,) if "oneOf" in target.schema else ()
            directions = selected_directions[key.domain]
            optional_locations = collect_optional_present_locations(
                self.catalog, target, base_value
            )
            validator = self.catalog.target_validator(target)
            optional_fixture_ids: list[str] = []
            nullable_null_fixture_ids: list[str] = []
            nullable_paths: list[str] = []

            for location in optional_locations:
                path_slug = slug(json_path(location.instance_path))
                omitted = copy.deepcopy(base_value)
                parent, field = get_parent_path(
                    omitted, location.instance_path
                )
                if not isinstance(parent, dict) or not isinstance(field, str):
                    raise FixtureError(
                        f"{batch} optional property is not an object field: "
                        f"{key.compact()}:{json_path(location.instance_path)}"
                    )
                parent.pop(field)
                if validator.validate_subschema(
                    omitted, target.schema, target.schema_path
                ):
                    raise FixtureError(
                        f"{batch} optional omission was rejected by the full schema: "
                        f"{key.compact()}:{json_path(location.instance_path)}"
                    )
                omitted_id = (
                    f"union:{key.domain}:{key.name}:"
                    f"optional-omitted:{path_slug}"
                )
                self.add_positive(
                    omitted_id,
                    (
                        f"cases/unions/{slug(key.domain)}/supplements/"
                        f"{slug(key.name)}-optional-omitted-{path_slug}.json"
                    ),
                    "union_optional_omitted",
                    target,
                    omitted,
                    key,
                    intended,
                    omitted_schema_paths=(location.schema_path,),
                    directions_exercised=directions,
                )
                optional_fixture_ids.append(omitted_id)

                nullable = not validator.validate_subschema(
                    None, location.schema, location.schema_path
                )
                if not nullable:
                    continue
                if (
                    get_instance_path(base_value, location.instance_path)
                    is None
                ):
                    raise FixtureError(
                        f"{batch} base fixture must exercise a non-null nullable "
                        f"value: {key.compact()}:{json_path(location.instance_path)}"
                    )
                nullable_paths.append(location.schema_path)
                null_value = copy.deepcopy(base_value)
                parent, field = get_parent_path(
                    null_value, location.instance_path
                )
                parent[field] = None
                null_id = (
                    f"union:{key.domain}:{key.name}:"
                    f"nullable-null:{path_slug}"
                )
                self.add_positive(
                    null_id,
                    (
                        f"cases/unions/{slug(key.domain)}/supplements/"
                        f"{slug(key.name)}-nullable-null-{path_slug}.json"
                    ),
                    "union_nullable_null",
                    target,
                    null_value,
                    key,
                    intended,
                    directions_exercised=directions,
                )
                nullable_null_fixture_ids.append(null_id)

            required_locations = collect_required_locations(
                self.catalog, target, base_value
            )
            discriminator_locations = [
                location
                for location in required_locations
                if (
                    (
                        key.discriminator_field != "$variant"
                        and location.instance_path
                        == (key.discriminator_field,)
                    )
                    or (
                        key.discriminator_field == "$variant"
                        and location.instance_path == (key.name,)
                    )
                )
            ]
            if len(discriminator_locations) > 1:
                raise FixtureError(
                    f"{batch} alternative has multiple discriminator locations: "
                    f"{key.compact()}"
                )
            missing_required_locations = [
                location
                for location in required_locations
                if not (
                    (
                        key.discriminator_field != "$variant"
                        and location.instance_path
                        == (key.discriminator_field,)
                    )
                    or (
                        key.discriminator_field == "$variant"
                        and location.instance_path == (key.name,)
                    )
                )
            ]
            wrong_type_locations = {
                location.schema_path: location
                for location in (
                    *missing_required_locations,
                    *optional_locations,
                )
            }
            missing_fixture_ids: list[str] = []
            wrong_type_fixture_ids: list[str] = []
            missing_discriminator_fixture_ids: list[str] = []
            wrong_discriminator_fixture_ids: list[str] = []

            for location in discriminator_locations:
                missing_discriminator = copy.deepcopy(base_value)
                parent, field = get_parent_path(
                    missing_discriminator, location.instance_path
                )
                if not isinstance(parent, dict) or not isinstance(field, str):
                    raise FixtureError(
                        f"{batch} discriminator is not an object field: "
                        f"{key.compact()}"
                    )
                parent.pop(field)
                missing_discriminator_id = (
                    f"union:{key.domain}:{key.name}:"
                    "missing-discriminator"
                )
                self.add_negative(
                    missing_discriminator_id,
                    (
                        f"cases/unions/{slug(key.domain)}/mutations/"
                        f"{slug(key.name)}-missing-discriminator.json"
                    ),
                    "malformed_known_missing_discriminator",
                    target,
                    missing_discriminator,
                    ["one_of_zero"],
                    key,
                    "missing_discriminator",
                )
                missing_discriminator_fixture_ids.append(
                    missing_discriminator_id
                )

                wrong_discriminator = copy.deepcopy(base_value)
                parent, field = get_parent_path(
                    wrong_discriminator, location.instance_path
                )
                parent[field] = None
                wrong_discriminator_id = (
                    f"union:{key.domain}:{key.name}:"
                    "wrong-discriminator-type"
                )
                self.add_negative(
                    wrong_discriminator_id,
                    (
                        f"cases/unions/{slug(key.domain)}/mutations/"
                        f"{slug(key.name)}-wrong-discriminator-type.json"
                    ),
                    "malformed_known_wrong_discriminator_type",
                    target,
                    wrong_discriminator,
                    ["one_of_zero"],
                    key,
                    "wrong_discriminator_type",
                )
                wrong_discriminator_fixture_ids.append(
                    wrong_discriminator_id
                )

            for location in missing_required_locations:
                path_slug = slug(json_path(location.instance_path))
                missing = copy.deepcopy(base_value)
                parent, field = get_parent_path(
                    missing, location.instance_path
                )
                if not isinstance(parent, dict) or not isinstance(field, str):
                    raise FixtureError(
                        f"{batch} required property is not an object field: "
                        f"{key.compact()}:{json_path(location.instance_path)}"
                    )
                parent.pop(field)
                missing_id = (
                    f"union:{key.domain}:{key.name}:"
                    f"missing-required:{path_slug}"
                )
                self.add_negative(
                    missing_id,
                    (
                        f"cases/unions/{slug(key.domain)}/mutations/"
                        f"{slug(key.name)}-missing-required-{path_slug}.json"
                    ),
                    "malformed_known_missing_required",
                    target,
                    missing,
                    ["one_of_zero"],
                    key,
                    "missing_required",
                )
                missing_fixture_ids.append(missing_id)

            for location in (
                wrong_type_locations[path]
                for path in sorted(wrong_type_locations)
            ):
                path_slug = slug(json_path(location.instance_path))
                original = get_instance_path(
                    base_value, location.instance_path
                )
                wrong_value: Any = NO_WRONG_TYPE_MUTATION
                for candidate in WRONG_TYPE_CANDIDATES:
                    if canonical_json(candidate) == canonical_json(original):
                        continue
                    mutated = copy.deepcopy(base_value)
                    parent, field = get_parent_path(
                        mutated, location.instance_path
                    )
                    parent[field] = copy.deepcopy(candidate)
                    if validator.validate_subschema(
                        mutated, target.schema, target.schema_path
                    ):
                        wrong_value = mutated
                        break
                if wrong_value is NO_WRONG_TYPE_MUTATION:
                    raise FixtureError(
                        f"{batch} property has no rejecting wrong-type "
                        f"mutation: {key.compact()}:{json_path(location.instance_path)}"
                    )
                wrong_id = (
                    f"union:{key.domain}:{key.name}:"
                    f"wrong-nested-type:{path_slug}"
                )
                self.add_negative(
                    wrong_id,
                    (
                        f"cases/unions/{slug(key.domain)}/mutations/"
                        f"{slug(key.name)}-wrong-nested-type-{path_slug}.json"
                    ),
                    "malformed_known_wrong_type",
                    target,
                    wrong_value,
                    ["one_of_zero"],
                    key,
                    "wrong_nested_type",
                )
                wrong_type_fixture_ids.append(wrong_id)

            alternative_coverage[key.compact()] = {
                "base_fixture_id": f"union:{key.domain}:{key.name}",
                "optional_omitted_fixture_ids": sorted(
                    optional_fixture_ids
                ),
                "nullable_null_fixture_ids": sorted(
                    nullable_null_fixture_ids
                ),
                "nullable_schema_paths": sorted(nullable_paths),
                "missing_required_fixture_ids": sorted(
                    missing_fixture_ids
                ),
                "missing_discriminator_fixture_ids": sorted(
                    missing_discriminator_fixture_ids
                ),
                "wrong_nested_type_fixture_ids": sorted(
                    wrong_type_fixture_ids
                ),
                "wrong_discriminator_type_fixture_ids": sorted(
                    wrong_discriminator_fixture_ids
                ),
                "missing_discriminator_exclusion": (
                    None
                    if discriminator_locations
                    else (
                        "scalar externally selected alternative has no "
                        "discriminator property"
                    )
                ),
                "missing_required_exclusion": (
                    None
                    if missing_required_locations
                    else (
                        "alternative has no non-discriminator required "
                        "payload field"
                    )
                ),
                "malformed_known_exclusion": (
                    None
                    if wrong_type_locations
                    else (
                        "alternative has no non-discriminator payload field"
                    )
                ),
                "conflicting_discriminator_exclusion": (
                    "JSON value shape provides exactly one scalar or object "
                    "discriminator slot"
                ),
            }

        family_counts = {
            domain: sum(key.domain == domain for key in selected_keys)
            for domain in sorted({key.domain for key in selected_keys})
        }
        for domain in sorted(family_counts):
            keys = [
                key
                for key in selected_keys
                if key.domain == domain
            ]
            representative = keys[0]
            target, future_value = known_union_values[
                (domain, representative.name)
            ]
            future_name = f"future{domain}"
            if representative.discriminator_field == "$variant":
                future_value = future_name
            else:
                future_value = copy.deepcopy(future_value)
                if not isinstance(future_value, dict):
                    raise FixtureError(
                        f"{domain} sample lacks its object discriminator"
                    )
                future_value[representative.discriminator_field] = future_name
            future_id = f"union:{domain}:future-unknown"
            self.add_negative(
                future_id,
                f"cases/unions/{slug(domain)}/future-unknown.json",
                "unknown_discriminator",
                target,
                future_value,
                ["one_of_zero"],
                negative_case="future_discriminator",
            )
            family_coverage[domain] = {
                "future_discriminator_fixture_id": future_id,
                "known_alternative_count": len(keys),
                "conflicting_discriminator_fixture_ids": [],
                "conflicting_discriminator_exclusion": (
                    "the pinned JSON representation has exactly one "
                    "discriminator slot, so multiple discriminator values "
                    "are not structurally representable"
                ),
            }

        coverage = {
            "assignment_derived_key_count": len(selected_keys),
            "family_counts": dict(sorted(family_counts.items())),
            "alternatives": dict(sorted(alternative_coverage.items())),
            "families": dict(sorted(family_coverage.items())),
        }
        if batch == "B2":
            self.b2_negative_coverage = coverage
        elif batch == "B4":
            self.b4_negative_coverage["operation_unions"] = coverage
        else:
            raise FixtureError(f"unsupported generic union batch {batch}")

    def _build_b4_conversation_negative_supplements(
        self,
        known_union_values: Mapping[
            tuple[str, str], tuple[SchemaTarget, Any]
        ],
    ) -> None:
        """Add family-shape negatives that do not belong to one alternative.

        The generic per-alternative mutations above cannot synthesize a
        future externally tagged object, two simultaneous external tags, or
        a wrong outer union shape.  These cases are independently validated
        against the same pinned schemas and indexed alongside the ordinary
        B4 operation-union fixtures.
        """

        coverage = self.b4_negative_coverage.get("operation_unions")
        if not isinstance(coverage, dict):
            raise FixtureError(
                "B4 operation-union coverage must exist before family "
                "negative supplements"
            )
        families = coverage.get("families")
        alternatives = coverage.get("alternatives")
        if not isinstance(families, dict) or not isinstance(
            alternatives, dict
        ):
            raise FixtureError(
                "B4 operation-union coverage lacks family/alternative maps"
            )

        keys = {
            (key.domain, key.name): key
            for key in self.b4_operation_union_keys
        }
        targets = {
            domain: known_union_values[(domain, names[0])][0]
            for domain, names in B4_OPERATION_UNION_FAMILY_IDENTITIES.items()
        }

        for domain in sorted(B4_OPERATION_UNION_FAMILY_IDENTITIES):
            wrong_outer: Any = (
                "idle" if domain == "ThreadStatus" else []
            )
            fixture_id = f"union:{domain}:wrong-outer-shape"
            self.add_negative(
                fixture_id,
                (
                    f"cases/unions/{slug(domain)}/mutations/"
                    "wrong-outer-shape.json"
                ),
                "malformed_known_wrong_outer_shape",
                targets[domain],
                wrong_outer,
                ["one_of_zero"],
                negative_case="wrong_outer_shape",
            )
            families[domain]["wrong_outer_shape_fixture_ids"] = [
                fixture_id
            ]

        external_families = {
            "SessionSource": {
                "future_tag": "futureSessionSourceObject",
                "future_value": {"synthetic": True},
                "conflict_names": ("custom", "subAgent"),
            },
            "SubAgentSource": {
                "future_tag": "future_sub_agent_object",
                "future_value": {"synthetic": True},
                "conflict_names": ("other", "thread_spawn"),
            },
        }
        for domain, specification in sorted(external_families.items()):
            future_id = f"union:{domain}:future-object-unknown"
            self.add_negative(
                future_id,
                (
                    f"cases/unions/{slug(domain)}/mutations/"
                    "future-object-unknown.json"
                ),
                "unknown_discriminator",
                targets[domain],
                {
                    specification["future_tag"]: copy.deepcopy(
                        specification["future_value"]
                    )
                },
                ["one_of_zero"],
                negative_case="future_object_discriminator",
            )

            first_name, second_name = specification["conflict_names"]
            first = copy.deepcopy(
                known_union_values[(domain, first_name)][1]
            )
            second = copy.deepcopy(
                known_union_values[(domain, second_name)][1]
            )
            if (
                not isinstance(first, dict)
                or len(first) != 1
                or not isinstance(second, dict)
                or len(second) != 1
                or set(first) & set(second)
            ):
                raise FixtureError(
                    f"B4 {domain} external conflict bases changed shape"
                )
            conflict_id = f"union:{domain}:conflicting-discriminators"
            self.add_negative(
                conflict_id,
                (
                    f"cases/unions/{slug(domain)}/mutations/"
                    "conflicting-discriminators.json"
                ),
                "malformed_known_conflicting_discriminators",
                targets[domain],
                {**first, **second},
                ["one_of_zero"],
                negative_case="conflicting_discriminators",
            )
            families[domain][
                "future_external_discriminator_fixture_ids"
            ] = [future_id]
            families[domain][
                "conflicting_discriminator_fixture_ids"
            ] = [conflict_id]
            families[domain]["conflicting_discriminator_exclusion"] = None
            for name in (first_name, second_name):
                alternative = alternatives[
                    keys[(domain, name)].compact()
                ]
                alternative[
                    "conflicting_discriminator_fixture_ids"
                ] = [conflict_id]
                alternative[
                    "conflicting_discriminator_exclusion"
                ] = None

        families["ThreadStatus"][
            "future_external_discriminator_fixture_ids"
        ] = []

        session_subagent_key = keys[("SessionSource", "subAgent")]
        malformed_spawn = copy.deepcopy(
            known_union_values[("SubAgentSource", "thread_spawn")][1]
        )
        if (
            not isinstance(malformed_spawn, dict)
            or not isinstance(
                malformed_spawn.get("thread_spawn"), dict
            )
            or "depth" not in malformed_spawn["thread_spawn"]
        ):
            raise FixtureError(
                "B4 SessionSource nested-malformation base changed shape"
            )
        malformed_spawn["thread_spawn"]["depth"] = None
        nested_id = (
            "union:SessionSource:subAgent:"
            "nested-malformed:thread_spawn-depth"
        )
        self.add_negative(
            nested_id,
            (
                "cases/unions/sessionsource/mutations/"
                "subagent-nested-malformed-thread_spawn-depth.json"
            ),
            "nested_union_failure",
            targets["SessionSource"],
            {"subAgent": malformed_spawn},
            ["one_of_zero"],
            session_subagent_key,
            "nested_union_failure",
        )
        alternatives[session_subagent_key.compact()][
            "nested_malformed_fixture_ids"
        ] = [nested_id]

        nested_future_id = (
            "union:SessionSource:subAgent:nested-future-unknown"
        )
        self.add_negative(
            nested_future_id,
            (
                "cases/unions/sessionsource/mutations/"
                "subagent-nested-future-unknown.json"
            ),
            "unknown_discriminator",
            targets["SessionSource"],
            {"subAgent": "futureSubAgentSource"},
            ["one_of_zero"],
            session_subagent_key,
            "nested_future_discriminator",
        )
        alternatives[session_subagent_key.compact()][
            "nested_future_discriminator_fixture_ids"
        ] = [nested_future_id]

        active_key = keys[("ThreadStatus", "active")]
        active_element = copy.deepcopy(
            known_union_values[("ThreadStatus", "active")][1]
        )
        if (
            not isinstance(active_element, dict)
            or not isinstance(active_element.get("activeFlags"), list)
            or not active_element["activeFlags"]
        ):
            raise FixtureError(
                "B4 ThreadStatus active base no longer exercises an "
                "activeFlags element"
            )
        active_element["activeFlags"][0] = None
        active_element_id = (
            "union:ThreadStatus:active:"
            "wrong-nested-type:activeflags-0"
        )
        self.add_negative(
            active_element_id,
            (
                "cases/unions/threadstatus/mutations/"
                "active-wrong-nested-type-activeflags-0.json"
            ),
            "malformed_known_wrong_type",
            targets["ThreadStatus"],
            active_element,
            ["one_of_zero"],
            active_key,
            "wrong_nested_type",
        )
        alternatives[active_key.compact()][
            "wrong_nested_type_fixture_ids"
        ].append(active_element_id)
        alternatives[active_key.compact()][
            "wrong_nested_type_fixture_ids"
        ].sort()

    def _build_b3_union_supplements(
        self,
        known_union_values: Mapping[
            tuple[str, str], tuple[SchemaTarget, Any]
        ],
    ) -> None:
        alternative_coverage: dict[str, Any] = {}
        family_coverage: dict[str, Any] = {}
        open_enum_names = set(B2_OPEN_STRING_ENUMS) | set(
            B3_OPEN_STRING_ENUMS
        )
        reused_wrong_type_fixtures = {
            ("ThreadItem", "agentMessage", "$/id"): (
                "union:ThreadItem:nested-wrong-type"
            ),
            ("ResponseItem", "agent_message", "$/content"): (
                "union:ResponseItem:nested-wrong-type"
            ),
        }
        reused_future_fixtures = {
            "ThreadItem": "union:ThreadItem:future-unknown",
            "ResponseItem": "union:ResponseItem:future-unknown",
        }
        counts: dict[str, int] = {
            "optional_omitted": 0,
            "nullable_null": 0,
            "required_nullable": 0,
            "missing_required": 0,
            "missing_discriminator": 0,
            "wrong_discriminator_type": 0,
            "wrong_nested_type": 0,
            "wrong_nested_type_generated": 0,
            "wrong_nested_type_reused": 0,
            "wrong_type_opaque_exclusions": 0,
            "future_discriminator": 0,
            "future_discriminator_generated": 0,
            "future_discriminator_reused": 0,
            "future_open_enum": 0,
            "empty_constrained_string": 0,
            "helper_union_positive": 0,
            "helper_union_future": 0,
        }
        registered_nested_families = frozenset(
            (
                set(B2_SHARED_COMMON_FAMILY_COUNTS)
                | set(B3_ITEM_FAMILY_COUNTS)
            )
            - {"ThreadItem", "ResponseItem"}
        )
        referenced_definition_names = (
            registered_nested_families | {"FunctionCallOutputBody"}
        )
        b2_family_identities: dict[str, tuple[str, ...]] = {
            domain: tuple(
                sorted(
                    key.name
                    for key in self.b2_shared_common_keys
                    if key.domain == domain
                )
            )
            for domain in B2_SHARED_COMMON_FAMILY_COUNTS
        }
        if {
            domain: len(names)
            for domain, names in b2_family_identities.items()
        } != B2_SHARED_COMMON_FAMILY_COUNTS:
            raise FixtureError(
                "B3 nested reachability cannot derive the exact B2 families"
            )

        for key in self.b3_item_keys:
            target, base_value = known_union_values[(key.domain, key.name)]
            index, _, _, _ = branch_for_union_identity(
                self.catalog,
                target,
                key.discriminator_field,
                key.name,
            )
            intended = (index,) if "oneOf" in target.schema else ()
            directions = ("Decode",)
            validator = self.catalog.target_validator(target)
            required_locations = collect_required_locations(
                self.catalog, target, base_value
            )
            optional_locations = collect_optional_present_locations(
                self.catalog, target, base_value
            )
            discriminator_locations = [
                location
                for location in required_locations
                if location.instance_path == ("type",)
            ]
            if len(discriminator_locations) != 1:
                raise FixtureError(
                    "B3 alternative lacks exactly one type discriminator: "
                    f"{key.compact()}"
                )
            payload_required_locations = [
                location
                for location in required_locations
                if location.instance_path != ("type",)
            ]

            optional_fixture_ids: list[str] = []
            nullable_null_fixture_ids: list[str] = []
            required_nullable_fixture_ids: list[str] = []
            missing_required_fixture_ids: list[str] = []
            wrong_nested_type_fixture_ids: list[str] = []
            wrong_nested_type_reused_fixture_ids: list[str] = []
            wrong_type_opaque_exclusions: list[dict[str, str]] = []
            future_open_enum_fixture_ids: list[str] = []
            helper_union_fixture_ids: list[str] = []
            helper_union_future_fixture_ids: list[str] = []
            reachable_union_fixture_ids: list[str] = []
            constrained_string_fixture_ids: list[str] = []

            for location in optional_locations:
                path_name = json_path(location.instance_path)
                path_slug = slug(path_name)
                omitted = copy.deepcopy(base_value)
                parent, field = get_parent_path(
                    omitted, location.instance_path
                )
                if not isinstance(parent, dict) or not isinstance(field, str):
                    raise FixtureError(
                        "B3 optional property is not an object field: "
                        f"{key.compact()}:{path_name}"
                    )
                parent.pop(field)
                if validator.validate_subschema(
                    omitted, target.schema, target.schema_path
                ):
                    raise FixtureError(
                        "B3 optional omission was rejected by the full schema: "
                        f"{key.compact()}:{path_name}"
                    )
                fixture_id = (
                    f"union:{key.domain}:{key.name}:"
                    f"optional-omitted:{path_slug}"
                )
                self.add_positive(
                    fixture_id,
                    (
                        f"cases/unions/{slug(key.domain)}/supplements/"
                        f"{slug(key.name)}-optional-omitted-{path_slug}.json"
                    ),
                    "union_optional_omitted",
                    target,
                    omitted,
                    key,
                    intended,
                    omitted_schema_paths=(location.schema_path,),
                    directions_exercised=directions,
                )
                optional_fixture_ids.append(fixture_id)
                counts["optional_omitted"] += 1

            for requiredness, locations in (
                ("required", required_locations),
                ("optional", optional_locations),
            ):
                for location in locations:
                    if validator.validate_subschema(
                        None, location.schema, location.schema_path
                    ):
                        continue
                    path_name = json_path(location.instance_path)
                    path_slug = slug(path_name)
                    if (
                        get_instance_path(
                            base_value, location.instance_path
                        )
                        is None
                    ):
                        raise FixtureError(
                            "B3 base fixture must exercise a non-null "
                            f"nullable value: {key.compact()}:{path_name}"
                        )
                    null_value = copy.deepcopy(base_value)
                    parent, field = get_parent_path(
                        null_value, location.instance_path
                    )
                    parent[field] = None
                    if validator.validate_subschema(
                        null_value, target.schema, target.schema_path
                    ):
                        raise FixtureError(
                            "B3 nullable-null mutation was rejected: "
                            f"{key.compact()}:{path_name}"
                        )
                    fixture_id = (
                        f"union:{key.domain}:{key.name}:"
                        f"nullable-null:{path_slug}"
                    )
                    self.add_positive(
                        fixture_id,
                        (
                            f"cases/unions/{slug(key.domain)}/supplements/"
                            f"{slug(key.name)}-nullable-null-{path_slug}.json"
                        ),
                        "union_nullable_null",
                        target,
                        null_value,
                        key,
                        intended,
                        directions_exercised=directions,
                    )
                    nullable_null_fixture_ids.append(fixture_id)
                    counts["nullable_null"] += 1
                    if requiredness == "required":
                        required_nullable_fixture_ids.append(fixture_id)
                        counts["required_nullable"] += 1

            discriminator = discriminator_locations[0]
            missing_discriminator = copy.deepcopy(base_value)
            parent, field = get_parent_path(
                missing_discriminator, discriminator.instance_path
            )
            parent.pop(field)
            missing_discriminator_id = (
                f"union:{key.domain}:{key.name}:missing-discriminator"
            )
            self.add_negative(
                missing_discriminator_id,
                (
                    f"cases/unions/{slug(key.domain)}/mutations/"
                    f"{slug(key.name)}-missing-discriminator.json"
                ),
                "malformed_known_missing_discriminator",
                target,
                missing_discriminator,
                ["one_of_zero"],
                key,
                "missing_discriminator",
            )
            counts["missing_discriminator"] += 1

            wrong_discriminator = copy.deepcopy(base_value)
            parent, field = get_parent_path(
                wrong_discriminator, discriminator.instance_path
            )
            parent[field] = None
            wrong_discriminator_id = (
                f"union:{key.domain}:{key.name}:"
                "wrong-discriminator-type"
            )
            self.add_negative(
                wrong_discriminator_id,
                (
                    f"cases/unions/{slug(key.domain)}/mutations/"
                    f"{slug(key.name)}-wrong-discriminator-type.json"
                ),
                "malformed_known_wrong_discriminator_type",
                target,
                wrong_discriminator,
                ["one_of_zero"],
                key,
                "wrong_discriminator_type",
            )
            counts["wrong_discriminator_type"] += 1

            for location in payload_required_locations:
                path_name = json_path(location.instance_path)
                path_slug = slug(path_name)
                missing = copy.deepcopy(base_value)
                parent, field = get_parent_path(
                    missing, location.instance_path
                )
                if not isinstance(parent, dict) or not isinstance(field, str):
                    raise FixtureError(
                        "B3 required property is not an object field: "
                        f"{key.compact()}:{path_name}"
                    )
                parent.pop(field)
                fixture_id = (
                    f"union:{key.domain}:{key.name}:"
                    f"missing-required:{path_slug}"
                )
                self.add_negative(
                    fixture_id,
                    (
                        f"cases/unions/{slug(key.domain)}/mutations/"
                        f"{slug(key.name)}-missing-required-{path_slug}.json"
                    ),
                    "malformed_known_missing_required",
                    target,
                    missing,
                    ["one_of_zero"],
                    key,
                    "missing_required",
                )
                missing_required_fixture_ids.append(fixture_id)
                counts["missing_required"] += 1

            payload_locations = {
                location.instance_path: location
                for location in (
                    *payload_required_locations,
                    *optional_locations,
                    *collect_container_value_locations(
                        self.catalog, target, base_value
                    ),
                )
            }
            for location in (
                payload_locations[path]
                for path in sorted(
                    payload_locations,
                    key=lambda value: tuple(map(str, value)),
                )
            ):
                path_name = json_path(location.instance_path)
                path_slug = slug(path_name)
                original = get_instance_path(
                    base_value, location.instance_path
                )
                wrong_value: Any = NO_WRONG_TYPE_MUTATION
                for candidate in WRONG_TYPE_CANDIDATES:
                    if canonical_json(candidate) == canonical_json(original):
                        continue
                    mutated = copy.deepcopy(base_value)
                    parent, field = get_parent_path(
                        mutated, location.instance_path
                    )
                    parent[field] = copy.deepcopy(candidate)
                    if validator.validate_subschema(
                        mutated, target.schema, target.schema_path
                    ):
                        wrong_value = mutated
                        break
                if wrong_value is NO_WRONG_TYPE_MUTATION:
                    wrong_type_opaque_exclusions.append(
                        {
                            "instance_path": path_name,
                            "schema_path": location.schema_path,
                            "reason": (
                                "schema accepts every deterministic JSON-type "
                                "candidate; the protocol defines this field "
                                "as semantically arbitrary JSON"
                            ),
                        }
                    )
                    counts["wrong_type_opaque_exclusions"] += 1
                    continue

                reused_id = reused_wrong_type_fixtures.get(
                    (key.domain, key.name, path_name)
                )
                if reused_id is not None:
                    wrong_nested_type_fixture_ids.append(reused_id)
                    wrong_nested_type_reused_fixture_ids.append(reused_id)
                    counts["wrong_nested_type"] += 1
                    counts["wrong_nested_type_reused"] += 1
                    continue

                fixture_id = (
                    f"union:{key.domain}:{key.name}:"
                    f"wrong-nested-type:{path_slug}"
                )
                self.add_negative(
                    fixture_id,
                    (
                        f"cases/unions/{slug(key.domain)}/mutations/"
                        f"{slug(key.name)}-wrong-nested-type-{path_slug}.json"
                    ),
                    "malformed_known_wrong_type",
                    target,
                    wrong_value,
                    ["one_of_zero"],
                    key,
                    "wrong_nested_type",
                )
                wrong_nested_type_fixture_ids.append(fixture_id)
                counts["wrong_nested_type"] += 1
                counts["wrong_nested_type_generated"] += 1

            for location in (*required_locations, *optional_locations):
                referenced_enums = sorted(
                    {
                        identity.name
                        for identity in schema_references(location.schema)
                    }
                    & open_enum_names
                )
                for enum_name in referenced_enums:
                    path_name = json_path(location.instance_path)
                    path_slug = slug(path_name)
                    future_value = copy.deepcopy(base_value)
                    parent, field = get_parent_path(
                        future_value, location.instance_path
                    )
                    parent[field] = f"future{enum_name}Value"
                    diagnostics = validator.validate_subschema(
                        future_value, target.schema, target.schema_path
                    )
                    codes = sorted({item.code for item in diagnostics})
                    if codes != ["one_of_zero"]:
                        raise FixtureError(
                            "B3 future open-enum mutation diagnostic changed: "
                            f"{key.compact()}:{path_name}:{codes}"
                        )
                    fixture_id = (
                        f"union:{key.domain}:{key.name}:"
                        f"future-open-enum:{slug(enum_name)}:{path_slug}"
                    )
                    self.add_negative(
                        fixture_id,
                        (
                            f"cases/unions/{slug(key.domain)}/mutations/"
                            f"{slug(key.name)}-future-open-enum-"
                            f"{slug(enum_name)}-{path_slug}.json"
                        ),
                        "unknown_enum_value",
                        target,
                        future_value,
                        ["one_of_zero"],
                        key,
                        "future_open_enum_value",
                    )
                    future_open_enum_fixture_ids.append(fixture_id)
                    counts["future_open_enum"] += 1

                    empty_value = copy.deepcopy(base_value)
                    parent, field = get_parent_path(
                        empty_value, location.instance_path
                    )
                    parent[field] = ""
                    empty_codes = sorted(
                        {
                            item.code
                            for item in validator.validate_subschema(
                                empty_value,
                                target.schema,
                                target.schema_path,
                            )
                        }
                    )
                    if empty_codes != ["one_of_zero"]:
                        raise FixtureError(
                            "B3 empty open-enum mutation diagnostic changed: "
                            f"{key.compact()}:{path_name}:{empty_codes}"
                        )
                    empty_id = (
                        f"union:{key.domain}:{key.name}:"
                        f"empty-open-enum:{slug(enum_name)}:{path_slug}"
                    )
                    self.add_negative(
                        empty_id,
                        (
                            f"cases/unions/{slug(key.domain)}/mutations/"
                            f"{slug(key.name)}-empty-open-enum-"
                            f"{slug(enum_name)}-{path_slug}.json"
                        ),
                        "unknown_enum_value",
                        target,
                        empty_value,
                        ["one_of_zero"],
                        key,
                        "empty_open_enum_value",
                    )
                    future_open_enum_fixture_ids.append(empty_id)
                    counts["future_open_enum"] += 1

            referenced_locations = collect_referenced_definition_locations(
                self.catalog,
                target,
                base_value,
                frozenset(referenced_definition_names),
            )
            for location in referenced_locations:
                if location.definition_name == "FunctionCallOutputBody":
                    continue
                identities = (
                    b2_family_identities.get(
                        location.definition_name
                    )
                    or B3_ITEM_FAMILY_IDENTITIES.get(
                        location.definition_name
                    )
                )
                if identities is None:
                    raise FixtureError(
                        "B3 root reaches an unreviewed registered union family: "
                        f"{key.compact()}:{location.definition_name}"
                    )
                for identity_name in identities:
                    nested_value = copy.deepcopy(
                        known_union_values[
                            (location.definition_name, identity_name)
                        ][1]
                    )
                    if canonical_json(
                        get_instance_path(
                            base_value, location.instance_path
                        )
                    ) == canonical_json(nested_value):
                        reachable_union_fixture_ids.append(
                            f"union:{key.domain}:{key.name}"
                        )
                        continue
                    nested_root = copy.deepcopy(base_value)
                    parent, field = get_parent_path(
                        nested_root, location.instance_path
                    )
                    parent[field] = nested_value
                    fixture_id = (
                        f"union:{key.domain}:{key.name}:nested:"
                        f"{location.definition_name}:{identity_name}:"
                        f"{slug(json_path(location.instance_path))}"
                    )
                    self.add_positive(
                        fixture_id,
                        (
                            f"cases/unions/{slug(key.domain)}/supplements/"
                            f"{slug(key.name)}-nested-"
                            f"{slug(location.definition_name)}-"
                            f"{slug(identity_name)}-"
                            f"{slug(json_path(location.instance_path))}.json"
                        ),
                        "union_branch_supplement",
                        target,
                        nested_root,
                        key,
                        intended,
                        directions_exercised=directions,
                    )
                    reachable_union_fixture_ids.append(fixture_id)
                    counts["helper_union_positive"] += 1

            if (
                key.domain,
                key.name,
            ) in {
                ("ThreadItem", "agentMessage"),
                ("ResponseItem", "message"),
            }:
                final_phase = copy.deepcopy(base_value)
                if (
                    not isinstance(final_phase, dict)
                    or final_phase.get("phase") != "commentary"
                ):
                    raise FixtureError(
                        "B3 MessagePhase base fixture no longer selects "
                        f"commentary: {key.compact()}"
                    )
                final_phase["phase"] = "final_answer"
                final_id = (
                    f"union:{key.domain}:{key.name}:"
                    "phase-final_answer"
                )
                self.add_positive(
                    final_id,
                    (
                        f"cases/unions/{slug(key.domain)}/supplements/"
                        f"{slug(key.name)}-phase-final_answer.json"
                    ),
                    "union_branch_supplement",
                    target,
                    final_phase,
                    key,
                    intended,
                    directions_exercised=directions,
                )
                helper_union_fixture_ids.append(final_id)
                counts["helper_union_positive"] += 1

                future_phase = copy.deepcopy(base_value)
                future_phase["phase"] = "futureMessagePhase"
                future_id = (
                    f"union:{key.domain}:{key.name}:"
                    "phase-future-unknown"
                )
                self.add_negative(
                    future_id,
                    (
                        f"cases/unions/{slug(key.domain)}/mutations/"
                        f"{slug(key.name)}-phase-future-unknown.json"
                    ),
                    "unknown_enum_value",
                    target,
                    future_phase,
                    ["one_of_zero"],
                    key,
                    "future_unregistered_string_union_value",
                )
                helper_union_future_fixture_ids.append(future_id)
                counts["helper_union_future"] += 1

                empty_phase = copy.deepcopy(base_value)
                empty_phase["phase"] = ""
                empty_phase_id = (
                    f"union:{key.domain}:{key.name}:phase-empty-unknown"
                )
                self.add_negative(
                    empty_phase_id,
                    (
                        f"cases/unions/{slug(key.domain)}/mutations/"
                        f"{slug(key.name)}-phase-empty-unknown.json"
                    ),
                    "unknown_enum_value",
                    target,
                    empty_phase,
                    ["one_of_zero"],
                    key,
                    "empty_unregistered_string_union_value",
                )
                helper_union_future_fixture_ids.append(empty_phase_id)
                counts["helper_union_future"] += 1

            if (
                key.domain == "ThreadItem"
                and key.name == "collabAgentToolCall"
            ):
                empty_effort = copy.deepcopy(base_value)
                if not isinstance(
                    empty_effort.get("reasoningEffort"), str
                ):
                    raise FixtureError(
                        "B3 collabAgentToolCall base no longer exercises "
                        "ReasoningEffort"
                    )
                empty_effort["reasoningEffort"] = ""
                empty_effort_id = (
                    "union:ThreadItem:collabAgentToolCall:"
                    "empty-constrained-string:reasoningeffort"
                )
                self.add_negative(
                    empty_effort_id,
                    (
                        "cases/unions/threaditem/mutations/"
                        "collabagenttoolcall-empty-constrained-string-"
                        "reasoningeffort.json"
                    ),
                    "malformed_known_empty_string",
                    target,
                    empty_effort,
                    ["one_of_zero"],
                    key,
                    "empty_constrained_string",
                )
                constrained_string_fixture_ids.append(empty_effort_id)
                counts["empty_constrained_string"] += 1

            if key.domain == "ResponseItem" and key.name in {
                "custom_tool_call_output",
                "function_call_output",
            }:
                content_values = [
                    copy.deepcopy(
                        known_union_values[
                            ("FunctionCallOutputContentItem", identity_name)
                        ][1]
                    )
                    for identity_name in B3_ITEM_FAMILY_IDENTITIES[
                        "FunctionCallOutputContentItem"
                    ]
                ]
                array_body = copy.deepcopy(base_value)
                if (
                    not isinstance(array_body, dict)
                    or not isinstance(array_body.get("output"), str)
                ):
                    raise FixtureError(
                        "B3 FunctionCallOutputBody base fixture no longer "
                        "selects its string branch"
                    )
                array_body["output"] = content_values
                array_id = (
                    f"union:ResponseItem:{key.name}:"
                    "body-content-array"
                )
                self.add_positive(
                    array_id,
                    (
                        "cases/unions/responseitem/supplements/"
                        f"{key.name}-body-content-array.json"
                    ),
                    "union_branch_supplement",
                    target,
                    array_body,
                    key,
                    intended,
                    directions_exercised=directions,
                )
                helper_union_fixture_ids.append(array_id)
                reachable_union_fixture_ids.append(array_id)
                counts["helper_union_positive"] += 1

            alternative_coverage[key.compact()] = {
                "base_fixture_id": f"union:{key.domain}:{key.name}",
                "optional_omitted_fixture_ids": sorted(
                    optional_fixture_ids
                ),
                "nullable_null_fixture_ids": sorted(
                    nullable_null_fixture_ids
                ),
                "required_nullable_fixture_ids": sorted(
                    required_nullable_fixture_ids
                ),
                "missing_required_fixture_ids": sorted(
                    missing_required_fixture_ids
                ),
                "missing_discriminator_fixture_ids": [
                    missing_discriminator_id
                ],
                "wrong_nested_type_fixture_ids": sorted(
                    wrong_nested_type_fixture_ids
                ),
                "reused_wrong_nested_type_fixture_ids": sorted(
                    wrong_nested_type_reused_fixture_ids
                ),
                "wrong_type_opaque_exclusions": sorted(
                    wrong_type_opaque_exclusions,
                    key=lambda record: (
                        record["schema_path"],
                        record["instance_path"],
                    ),
                ),
                "wrong_discriminator_type_fixture_ids": [
                    wrong_discriminator_id
                ],
                "future_open_enum_fixture_ids": sorted(
                    future_open_enum_fixture_ids
                ),
                "helper_union_fixture_ids": sorted(
                    helper_union_fixture_ids
                ),
                "reachable_union_fixture_ids": sorted(
                    set(reachable_union_fixture_ids)
                ),
                "helper_union_future_fixture_ids": sorted(
                    helper_union_future_fixture_ids
                ),
                "constrained_string_fixture_ids": sorted(
                    constrained_string_fixture_ids
                ),
                "conflicting_discriminator_fixture_ids": [],
                "conflicting_discriminator_exclusion": (
                    "the pinned JSON representation has exactly one type "
                    "property, so multiple discriminator values are not "
                    "structurally representable"
                ),
                "malformed_known_exclusion": (
                    None
                    if wrong_nested_type_fixture_ids
                    else (
                        "alternative has no non-discriminator payload field "
                        "with a schema-rejected JSON type"
                    )
                ),
            }

        for domain in sorted(B3_ITEM_FAMILY_COUNTS):
            keys = [
                key for key in self.b3_item_keys if key.domain == domain
            ]
            representative = keys[0]
            target, future_value = known_union_values[
                (domain, representative.name)
            ]
            future_value = copy.deepcopy(future_value)
            if not isinstance(future_value, dict):
                raise FixtureError(
                    f"B3 {domain} sample lacks its type discriminator"
                )
            future_value["type"] = f"future{domain}Alternative"
            fixture_id = reused_future_fixtures.get(domain)
            if fixture_id is None:
                fixture_id = f"union:{domain}:future-unknown"
                self.add_negative(
                    fixture_id,
                    f"cases/unions/{slug(domain)}/future-unknown.json",
                    "unknown_discriminator",
                    target,
                    future_value,
                    ["one_of_zero"],
                    negative_case="future_discriminator",
                )
                counts["future_discriminator_generated"] += 1
            else:
                counts["future_discriminator_reused"] += 1
            counts["future_discriminator"] += 1
            family_coverage[domain] = {
                "future_discriminator_fixture_id": fixture_id,
                "known_alternative_count": len(keys),
                "conflicting_discriminator_fixture_ids": [],
                "conflicting_discriminator_exclusion": (
                    "the pinned JSON representation has exactly one type "
                    "property, so multiple discriminator values are not "
                    "structurally representable"
                ),
            }

        expected_counts = {
            "optional_omitted": 112,
            "nullable_null": 111,
            "required_nullable": 3,
            "missing_required": 127,
            "missing_discriminator": 50,
            "wrong_discriminator_type": 50,
            "wrong_nested_type": 257,
            "wrong_nested_type_generated": 255,
            "wrong_nested_type_reused": 2,
            "wrong_type_opaque_exclusions": 7,
            "future_discriminator": 9,
            "future_discriminator_generated": 7,
            "future_discriminator_reused": 2,
            "future_open_enum": 24,
            "empty_constrained_string": 1,
            "helper_union_positive": 24,
            "helper_union_future": 4,
        }
        if counts != expected_counts:
            raise FixtureError(
                "B3 fixture/mutation accounting changed: "
                f"{dict(sorted(counts.items()))}"
            )
        self.b3_negative_coverage = {
            "assignment_derived_key_count": len(self.b3_item_keys),
            "family_counts": dict(sorted(B3_ITEM_FAMILY_COUNTS.items())),
            "counts": dict(sorted(counts.items())),
            "alternatives": dict(sorted(alternative_coverage.items())),
            "families": dict(sorted(family_coverage.items())),
        }

    def _build_b2_open_enum_fixtures(self) -> None:
        enum_coverage: dict[str, Any] = {}
        for domain, expected_values in sorted(B2_OPEN_STRING_ENUMS.items()):
            target = self.catalog.union_target(domain)
            resolved, _ = self.catalog.resolve(
                target, target.schema, target.schema_path
            )
            actual_values = (
                tuple(resolved.get("enum", ()))
                if isinstance(resolved, dict)
                else ()
            )
            if actual_values != expected_values:
                raise FixtureError(
                    f"B2 open-enum pin mismatch for {domain}: "
                    f"{actual_values!r}"
                )
            known_ids: list[str] = []
            for value in expected_values:
                fixture_id = f"enum:{domain}:{value}"
                self.add_positive(
                    fixture_id,
                    f"cases/enums/{slug(domain)}/{slug(value)}.json",
                    "open_enum_known_value",
                    target,
                    value,
                    None,
                    directions_exercised=("Decode", "Encode"),
                )
                known_ids.append(fixture_id)
            future_id = f"enum:{domain}:future-unknown"
            self.add_negative(
                future_id,
                f"cases/enums/{slug(domain)}/future-unknown.json",
                "unknown_enum_value",
                target,
                f"future{domain}",
                ["enum_mismatch"],
                negative_case="future_open_enum_value",
            )
            empty_id = f"enum:{domain}:empty-unknown"
            self.add_negative(
                empty_id,
                f"cases/enums/{slug(domain)}/empty-unknown.json",
                "unknown_enum_value",
                target,
                "",
                ["enum_mismatch"],
                negative_case="empty_open_enum_value",
            )
            enum_coverage[domain] = {
                "known_value_fixture_ids": known_ids,
                "future_value_fixture_id": future_id,
                "empty_value_fixture_id": empty_id,
                "future_value_schema_diagnostic_codes": [
                    "enum_mismatch"
                ],
            }
        self.b2_negative_coverage["open_string_enums"] = dict(
            sorted(enum_coverage.items())
        )

    def _build_b3_open_enum_fixtures(self) -> None:
        enum_coverage: dict[str, Any] = {}
        for domain, expected_values in sorted(B3_OPEN_STRING_ENUMS.items()):
            target = self.catalog.union_target(domain)
            resolved, _ = self.catalog.resolve(
                target, target.schema, target.schema_path
            )
            actual_values = (
                tuple(resolved.get("enum", ()))
                if isinstance(resolved, dict)
                else ()
            )
            if actual_values != expected_values:
                raise FixtureError(
                    f"B3 open-enum pin mismatch for {domain}: "
                    f"{actual_values!r}"
                )
            known_ids: list[str] = []
            for value in expected_values:
                fixture_id = f"enum:{domain}:{value}"
                self.add_positive(
                    fixture_id,
                    f"cases/enums/{slug(domain)}/{slug(value)}.json",
                    "open_enum_known_value",
                    target,
                    value,
                    None,
                    directions_exercised=("Decode",),
                )
                known_ids.append(fixture_id)
            future_id = f"enum:{domain}:future-unknown"
            self.add_negative(
                future_id,
                f"cases/enums/{slug(domain)}/future-unknown.json",
                "unknown_enum_value",
                target,
                f"future{domain}",
                ["enum_mismatch"],
                negative_case="future_open_enum_value",
            )
            empty_id = f"enum:{domain}:empty-unknown"
            self.add_negative(
                empty_id,
                f"cases/enums/{slug(domain)}/empty-unknown.json",
                "unknown_enum_value",
                target,
                "",
                ["enum_mismatch"],
                negative_case="empty_open_enum_value",
            )
            enum_coverage[domain] = {
                "known_value_fixture_ids": known_ids,
                "future_value_fixture_id": future_id,
                "empty_value_fixture_id": empty_id,
                "future_value_schema_diagnostic_codes": [
                    "enum_mismatch"
                ],
            }
        if (
            len(enum_coverage) != 10
            or sum(
                len(record["known_value_fixture_ids"])
                for record in enum_coverage.values()
            )
            != 39
        ):
            raise FixtureError(
                "B3 open-enum fixture accounting changed"
            )
        self.b3_negative_coverage["open_string_enums"] = dict(
            sorted(enum_coverage.items())
        )

    def _build_b4_open_enum_fixtures(self) -> None:
        enum_coverage: dict[str, Any] = {}
        for domain, expected_values in sorted(B4_OPEN_STRING_ENUMS.items()):
            target = self.catalog.union_target(domain)
            resolved, _ = self.catalog.resolve(
                target, target.schema, target.schema_path
            )
            actual_values = (
                tuple(resolved.get("enum", ()))
                if isinstance(resolved, dict)
                else ()
            )
            if actual_values != expected_values:
                raise FixtureError(
                    f"B4 open-enum pin mismatch for {domain}: "
                    f"{actual_values!r}"
                )
            known_ids: list[str] = []
            for value in expected_values:
                fixture_id = f"enum:{domain}:{value}"
                self.add_positive(
                    fixture_id,
                    f"cases/enums/{slug(domain)}/{slug(value)}.json",
                    "open_enum_known_value",
                    target,
                    value,
                    None,
                    directions_exercised=(
                        ("Decode", "Encode")
                        if domain
                        in {
                            "ApprovalsReviewer",
                            "Personality",
                            "ThreadGoalStatus",
                        }
                        else ("Encode",)
                        if domain
                        in {
                            "SandboxMode",
                            "SortDirection",
                            "ThreadSortKey",
                            "ThreadSourceKind",
                            "ThreadStartSource",
                        }
                        else ("Decode",)
                    ),
                )
                known_ids.append(fixture_id)
            future_id = f"enum:{domain}:future-unknown"
            self.add_negative(
                future_id,
                f"cases/enums/{slug(domain)}/future-unknown.json",
                "unknown_enum_value",
                target,
                f"future{domain}",
                ["enum_mismatch"],
                negative_case="future_open_enum_value",
            )
            empty_id = f"enum:{domain}:empty-unknown"
            self.add_negative(
                empty_id,
                f"cases/enums/{slug(domain)}/empty-unknown.json",
                "unknown_enum_value",
                target,
                "",
                ["enum_mismatch"],
                negative_case="empty_open_enum_value",
            )
            enum_coverage[domain] = {
                "known_value_fixture_ids": known_ids,
                "future_value_fixture_id": future_id,
                "empty_value_fixture_id": empty_id,
                "future_value_schema_diagnostic_codes": [
                    "enum_mismatch"
                ],
            }
        if (
            len(enum_coverage) != 11
            or sum(
                len(record["known_value_fixture_ids"])
                for record in enum_coverage.values()
            )
            != 41
        ):
            raise FixtureError("B4 open-enum fixture accounting changed")
        self.b4_negative_coverage["open_string_enums"] = dict(
            sorted(enum_coverage.items())
        )

    def _build_b5_open_enum_fixtures(self) -> None:
        enum_coverage: dict[str, Any] = {}
        for domain, expected_values in sorted(B5_OPEN_STRING_ENUMS.items()):
            target = self.catalog.union_target(domain)
            resolved, _ = self.catalog.resolve(
                target, target.schema, target.schema_path
            )
            actual_values = (
                tuple(resolved.get("enum", ()))
                if isinstance(resolved, dict)
                else ()
            )
            if actual_values != expected_values:
                raise FixtureError(
                    f"B5 open-enum pin mismatch for {domain}: "
                    f"{actual_values!r}"
                )
            known_ids: list[str] = []
            for value in expected_values:
                fixture_id = f"enum:{domain}:{value}"
                self.add_positive(
                    fixture_id,
                    f"cases/enums/{slug(domain)}/{slug(value)}.json",
                    "open_enum_known_value",
                    target,
                    value,
                    None,
                    directions_exercised=("Decode",),
                )
                known_ids.append(fixture_id)
            future_id = f"enum:{domain}:future-unknown"
            self.add_negative(
                future_id,
                f"cases/enums/{slug(domain)}/future-unknown.json",
                "unknown_enum_value",
                target,
                f"future{domain}",
                ["enum_mismatch"],
                negative_case="future_open_enum_value",
            )
            empty_id = f"enum:{domain}:empty-unknown"
            self.add_negative(
                empty_id,
                f"cases/enums/{slug(domain)}/empty-unknown.json",
                "unknown_enum_value",
                target,
                "",
                ["enum_mismatch"],
                negative_case="empty_open_enum_value",
            )
            enum_coverage[domain] = {
                "known_value_fixture_ids": known_ids,
                "future_value_fixture_id": future_id,
                "empty_value_fixture_id": empty_id,
                "future_value_schema_diagnostic_codes": [
                    "enum_mismatch"
                ],
                "directions_exercised": ["Decode"],
            }
        if (
            len(enum_coverage) != 3
            or sum(
                len(record["known_value_fixture_ids"])
                for record in enum_coverage.values()
            )
            != 7
        ):
            raise FixtureError("B5 open-enum fixture accounting changed")
        self.b5_negative_coverage["open_string_enums"] = dict(
            sorted(enum_coverage.items())
        )


def generated_outputs(
    arguments: argparse.Namespace,
) -> tuple[dict[Path, bytes], dict[str, Any]]:
    builder = CorpusBuilder(
        arguments.repo_root,
        arguments.schema_root,
        arguments.manifest,
        arguments.contracts,
        arguments.validator,
    )
    (
        fixture_files,
        index,
        fixture_coverage,
        keywords,
        reachability,
        assignments,
    ) = builder.build()
    completeness_records = [
        {
            "protocol_surface_key": record["protocol_surface_key"],
            "fixture_ids": record["fixture_ids"],
            "positive_fixture_count": record["positive_fixture_count"],
            "schema_fixture_facts": record["completeness_evidence"],
        }
        for record in fixture_coverage["identity_coverage"]
    ]
    completeness_facts = (
        completeness_records[0]["schema_fixture_facts"]
        if completeness_records
        else {}
    )
    completeness_report = {
        "generated_notice": "Generated by tools/codex/app_server_fixtures.py; do not edit.",
        "format_version": FORMAT_VERSION,
        "codex_version": CODEX_VERSION,
        "authority_note": (
            "This report contains schema- and fixture-derived facts only. "
            "The production registry combines them with implementation-owned "
            "evidence and mechanically derives TypedSchemaStatus; this report "
            "does not assert or improve a status."
        ),
        "counts": {
            "surface_identities": len(completeness_records),
            "identities_with_positive_fixtures": sum(
                record["positive_fixture_count"] > 0
                for record in completeness_records
            ),
            "facts_true_by_field": {
                field: sum(
                    record["schema_fixture_facts"][field]
                    for record in completeness_records
                )
                for field in sorted(completeness_facts)
            },
        },
        "records": completeness_records,
    }
    outputs: dict[Path, bytes] = {
        arguments.fixture_root / relative: payload
        for relative, payload in fixture_files.items()
    }
    outputs.update(
        {
            arguments.evidence_root / "fixture-coverage.json": encoded_json(
                fixture_coverage
            ),
            arguments.evidence_root / "schema-keywords.json": encoded_json(
                keywords
            ),
            arguments.evidence_root / "nested-reachability.json": encoded_json(
                reachability
            ),
            arguments.evidence_root
            / "module-slice-assignment.json": encoded_json(assignments),
            arguments.evidence_root
            / "schema-completeness-evidence.json": encoded_json(
                completeness_report
            ),
        }
    )
    return outputs, index


def write_outputs(outputs: Mapping[Path, bytes], managed_roots: Sequence[Path]) -> None:
    expected = {path.resolve() for path in outputs}
    for root in managed_roots:
        if root.exists():
            for path in sorted(root.rglob("*.json")):
                if path.resolve() not in expected:
                    path.unlink()
    for path, payload in sorted(outputs.items(), key=lambda item: str(item[0])):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(payload)


def check_outputs(outputs: Mapping[Path, bytes], managed_roots: Sequence[Path]) -> None:
    expected = {path.resolve() for path in outputs}
    managed_actual: set[Path] = set()
    for root in managed_roots:
        if root.exists():
            managed_actual.update(
                path.resolve() for path in root.rglob("*.json")
            )
    missing = sorted(
        str(path) for path in expected if not path.is_file()
    )
    extra = sorted(str(path) for path in managed_actual - expected)
    changed = sorted(
        str(path)
        for path, payload in outputs.items()
        if path.is_file() and path.read_bytes() != payload
    )
    if missing or extra or changed:
        raise FixtureError(
            f"generated fixture evidence is stale; "
            f"missing={missing}, extra={extra}, changed={changed}"
        )


def validate_committed(arguments: argparse.Namespace) -> None:
    index_path = arguments.fixture_root / "index.json"
    index = load_json(index_path)
    if index.get("format_version") != FORMAT_VERSION:
        raise FixtureError("unsupported committed fixture index format")
    expected_sources = CorpusBuilder(
        arguments.repo_root,
        arguments.schema_root,
        arguments.manifest,
        arguments.contracts,
        arguments.validator,
    )._source_hashes()
    if index.get("sources") != expected_sources:
        raise FixtureError("fixture index source/provenance hashes are stale")
    draft07 = load_draft07(arguments.validator)
    catalog = SchemaCatalog(arguments.schema_root, draft07)
    manifest = load_json(arguments.manifest)
    manifest_by_key = {
        SurfaceKey.from_manifest(entry): entry
        for entry in manifest["entries"]
    }
    seen_files: set[str] = set()
    seen_ids: set[str] = set()
    for record in index["fixtures"]:
        fixture_id = str(record["id"])
        if fixture_id in seen_ids:
            raise FixtureError(f"duplicate fixture identity in index: {fixture_id}")
        seen_ids.add(fixture_id)
        key_record = record.get("protocol_surface_key")
        if key_record is not None:
            surface_key = SurfaceKey.from_contract(key_record)
            entry = manifest_by_key.get(surface_key)
            if entry is None:
                raise FixtureError(
                    "fixture references unknown protocol surface identity: "
                    f"{surface_key.compact()}"
                )
            if entry["stability"] != "stable":
                raise FixtureError(
                    "fixture incorrectly counts an experimental-only identity: "
                    f"{surface_key.compact()}"
                )
        relative = str(record["file"])
        if relative in seen_files:
            raise FixtureError(f"duplicate fixture file in index: {relative}")
        seen_files.add(relative)
        path = (arguments.fixture_root / relative).resolve()
        try:
            path.relative_to(arguments.fixture_root.resolve())
        except ValueError as error:
            raise FixtureError(
                f"fixture path escapes the corpus root: {relative}"
            ) from error
        payload = path.read_bytes()
        if sha256_bytes(payload) != record["file_sha256"]:
            raise FixtureError(f"fixture hash mismatch: {relative}")
        value = json.loads(payload.decode("utf-8"))
        target = schema_target_from_record(catalog, record["schema"])
        diagnostics = catalog.target_validator(target).validate_subschema(
            value, target.schema, target.schema_path
        )
        expected_valid = record.get("expected_valid", True)
        if expected_valid and diagnostics:
            raise FixtureError(
                f"committed positive fixture is invalid: {relative}: "
                f"{[item.to_json() for item in diagnostics[:5]]}"
            )
        if not expected_valid:
            actual_codes = sorted({item.code for item in diagnostics})
            if actual_codes != sorted(record["expected_diagnostic_codes"]):
                raise FixtureError(
                    f"committed negative fixture diagnostics changed: {relative}"
                )
        else:
            if record.get("fixture_sha256") != record["file_sha256"]:
                raise FixtureError(
                    f"positive fixture evidence hash mismatch: {relative}"
                )
            validation = record.get("validation")
            if not isinstance(validation, dict) or validation.get(
                "independent"
            ) is not True:
                raise FixtureError(
                    f"positive fixture lacks independent validation: {relative}"
                )
            intended_branches = tuple(
                validation.get("one_of_branch_indices", [])
            )
            if intended_branches:
                actual_branches = matching_one_of_branches(
                    catalog,
                    target,
                    value,
                    target.schema,
                    target.schema_path,
                )
                if actual_branches != intended_branches:
                    raise FixtureError(
                        f"fixture {relative} matches oneOf branches "
                        f"{actual_branches}, expected {intended_branches}"
                    )
            recomputed = mutation_evidence(catalog, target, value)
            if validation.get("compact_mutation_evidence") is True:
                expected_hash = sha256_bytes(encoded_json(recomputed))
                expected_counts = {
                    key: value
                    for key, value in recomputed.items()
                    if isinstance(value, int)
                    and not isinstance(value, bool)
                }
                if (
                    validation.get("mutation_evidence_sha256")
                    != expected_hash
                    or validation.get("mutation_counts")
                    != expected_counts
                ):
                    raise FixtureError(
                        "committed compact mutation evidence is stale: "
                        f"{relative}"
                    )
            else:
                committed_mutation = {
                    key: value
                    for key, value in validation.items()
                    if key
                    not in {"independent", "one_of_branch_indices"}
                }
                if recomputed != committed_mutation:
                    raise FixtureError(
                        f"committed mutation evidence is stale: {relative}"
                    )
            committed_schema_coverage = record.get(
                "schema_fixture_coverage"
            )
            if committed_schema_coverage is not None:
                if not isinstance(committed_schema_coverage, dict):
                    raise FixtureError(
                        f"positive fixture has invalid schema coverage: {relative}"
                    )
                recomputed_schema_coverage = schema_fixture_coverage(
                    catalog,
                    target,
                    value,
                    omitted_schema_paths=committed_schema_coverage.get(
                        "optional_property_schema_paths_omitted", []
                    ),
                    directions_exercised=committed_schema_coverage.get(
                        "directions_exercised", []
                    ),
                )
                if committed_schema_coverage.get(
                    "compact_schema_fixture_coverage"
                ) is True:
                    if committed_schema_coverage.get(
                        "schema_fixture_coverage_sha256"
                    ) != sha256_bytes(
                        encoded_json(recomputed_schema_coverage)
                    ):
                        raise FixtureError(
                            "committed compact schema fixture coverage is "
                            f"stale: {relative}"
                        )
                elif (
                    recomputed_schema_coverage
                    != committed_schema_coverage
                ):
                    raise FixtureError(
                        "committed schema fixture coverage is stale: "
                        f"{relative}"
                    )
    actual_files = {
        path.relative_to(arguments.fixture_root).as_posix()
        for path in arguments.fixture_root.rglob("*.json")
    }
    expected_files = seen_files | {"index.json"}
    if actual_files != expected_files:
        raise FixtureError(
            f"fixture corpus file set mismatch; "
            f"missing={sorted(expected_files-actual_files)}, "
            f"extra={sorted(actual_files-expected_files)}"
        )


def _parser() -> argparse.ArgumentParser:
    repo_root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "mode", choices=("generate", "check", "validate")
    )
    parser.add_argument("--repo-root", type=Path, default=repo_root)
    parser.add_argument(
        "--schema-root",
        type=Path,
        default=repo_root / "tools/codex/app-server-schema/0.144.6",
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        default=repo_root / "tools/codex/app-server-surface/0.144.6.json",
    )
    parser.add_argument(
        "--contracts",
        type=Path,
        default=repo_root
        / "tools/codex/app-server-evidence/0.144.6/operation-contracts.json",
    )
    parser.add_argument(
        "--validator",
        type=Path,
        default=repo_root / "tools/codex/draft07.py",
    )
    parser.add_argument(
        "--fixture-root",
        type=Path,
        default=repo_root / "tools/codex/app-server-fixtures/0.144.6",
    )
    parser.add_argument(
        "--evidence-root",
        type=Path,
        default=repo_root / "tools/codex/app-server-evidence/0.144.6",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    arguments = _parser().parse_args(argv)
    arguments.repo_root = arguments.repo_root.resolve()
    arguments.schema_root = arguments.schema_root.resolve()
    arguments.manifest = arguments.manifest.resolve()
    arguments.contracts = arguments.contracts.resolve()
    arguments.validator = arguments.validator.resolve()
    arguments.fixture_root = arguments.fixture_root.resolve()
    arguments.evidence_root = arguments.evidence_root.resolve()
    try:
        if arguments.mode == "validate":
            validate_committed(arguments)
            return 0
        outputs, _ = generated_outputs(arguments)
        managed_roots = (arguments.fixture_root,)
        if arguments.mode == "generate":
            write_outputs(outputs, managed_roots)
        else:
            check_outputs(outputs, managed_roots)
        return 0
    except (
        FixtureError,
        OSError,
        UnicodeError,
        json.JSONDecodeError,
        KeyError,
        TypeError,
        ValueError,
    ) as error:
        print(f"app-server-fixtures: error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
