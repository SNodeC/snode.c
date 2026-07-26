#!/usr/bin/env python3
"""Freeze and verify the Codex App Server Phase A1.3 implementation plan.

The generated documents are deterministic review evidence.  Runtime
disposition, typed targets, implementation status, and schema completeness
remain owned solely by ``ProtocolSurfaceRegistryData.inc``.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence

sys.dont_write_bytecode = True

import app_server_a1_shared as shared
import app_server_fixtures as fixtures


FORMAT_VERSION = 1
CODEX_VERSION = "codex-cli 0.144.6"
UPSTREAM_TAG = "rust-v0.144.6"
A1_3_SLICE = "A1.3"
MODULE = "CommandsFilesystemReviewsApprovals"
EXPECTED_BASE_SHA = "304a817d371597fc764c3404c36cc880bf65536d"
EXPECTED_BASE_TREE = "d0c8f21a415139655d4e9d92bb6efd0892b6bf4c"

AuditError = shared.AuditError
AuditDiagnostic = shared.AuditDiagnostic
Key = shared.Key

CLIENT_REQUESTS = {
    "command/exec",
    "command/exec/resize",
    "command/exec/terminate",
    "command/exec/write",
    "fs/copy",
    "fs/createDirectory",
    "fs/getMetadata",
    "fs/readDirectory",
    "fs/readFile",
    "fs/remove",
    "fs/unwatch",
    "fs/watch",
    "fs/writeFile",
    "fuzzyFileSearch",
    "permissionProfile/list",
    "review/start",
    "thread/approveGuardianDeniedAction",
}
SERVER_NOTIFICATIONS = {
    "command/exec/outputDelta",
    "fs/changed",
    "fuzzyFileSearch/sessionCompleted",
    "fuzzyFileSearch/sessionUpdated",
    "guardianWarning",
    "item/autoApprovalReview/completed",
    "item/autoApprovalReview/started",
}
SERVER_REQUESTS = {
    "applyPatchApproval",
    "execCommandApproval",
    "item/commandExecution/requestApproval",
    "item/fileChange/requestApproval",
    "item/permissions/requestApproval",
}
UNION_FAMILIES = {
    ("CommandExecutionApprovalDecision", "$variant"): {
        "accept",
        "acceptForSession",
        "acceptWithExecpolicyAmendment",
        "applyNetworkPolicyAmendment",
        "cancel",
        "decline",
    },
    ("FileChange", "type"): {"add", "delete", "update"},
    ("FileSystemPath", "type"): {"glob_pattern", "path", "special"},
    ("FileSystemSpecialPath", "kind"): {
        "minimal",
        "project_roots",
        "root",
        "slash_tmp",
        "tmpdir",
        "unknown",
    },
    ("GuardianApprovalReviewAction", "type"): {
        "applyPatch",
        "command",
        "execve",
        "mcpToolCall",
        "networkAccess",
        "requestPermissions",
    },
    ("ParsedCommand", "type"): {
        "list_files",
        "read",
        "search",
        "unknown",
    },
    ("ReviewDecision", "$variant"): {
        "abort",
        "approved",
        "approved_execpolicy_amendment",
        "approved_for_session",
        "denied",
        "network_policy_amendment",
        "timed_out",
    },
    ("ReviewTarget", "type"): {
        "baseBranch",
        "commit",
        "custom",
        "uncommittedChanges",
    },
}

EXPECTED_TAXONOMY = {
    "client_request": 17,
    "server_notification": 7,
    "server_request": 5,
    "tagged_union_discriminator": 39,
}
EXPECTED_START_SLICE_STATUS = {
    "NotImplemented": 66,
    "Partial": 2,
}
EXPECTED_START_GLOBAL_STATUS = {
    "Complete": 212,
    "NotApplicable": 48,
    "NotImplemented": 121,
    "Partial": 6,
}
EXPECTED_FINAL_GLOBAL_STATUS = {
    "Complete": 280,
    "NotApplicable": 48,
    "NotImplemented": 55,
    "Partial": 4,
}
EXPECTED_RESULT_KINDS = {"Concrete": 8, "Unit": 9}
EXPECTED_PARTIAL = {
    Key(
        "server_request",
        "ServerRequest",
        "method",
        "item/commandExecution/requestApproval",
    ),
    Key(
        "server_request",
        "ServerRequest",
        "method",
        "item/fileChange/requestApproval",
    ),
}
EXPECTED_RESIDUAL_PARTIAL_NAMES = {
    "error",
    "initialize",
    "initialized",
    "item/tool/requestUserInput",
}

EXCLUDED_METHODS = {
    "item/tool/requestUserInput",
    "serverRequest/resolved",
    "fuzzyFileSearch/sessionStart",
    "fuzzyFileSearch/sessionUpdate",
    "fuzzyFileSearch/sessionStop",
    "process/spawn",
    "process/kill",
    "process/writeStdin",
    "process/resizePty",
    "process/outputDelta",
    "process/exited",
}

BATCHES = {
    "B2": {
        "subject": "Complete Codex one-off command execution",
        "requests": {
            "command/exec",
            "command/exec/resize",
            "command/exec/terminate",
            "command/exec/write",
        },
        "notifications": {"command/exec/outputDelta"},
        "server_requests": set(),
        "unions": set(),
    },
    "B3": {
        "subject": "Complete Codex filesystem and fuzzy search",
        "requests": {
            "fs/copy",
            "fs/createDirectory",
            "fs/getMetadata",
            "fs/readDirectory",
            "fs/readFile",
            "fs/remove",
            "fs/unwatch",
            "fs/watch",
            "fs/writeFile",
            "fuzzyFileSearch",
        },
        "notifications": {
            "fs/changed",
            "fuzzyFileSearch/sessionCompleted",
            "fuzzyFileSearch/sessionUpdated",
        },
        "server_requests": set(),
        "unions": set(),
    },
    "B4": {
        "subject": "Complete Codex approvals, permissions, and file changes",
        "requests": {"permissionProfile/list"},
        "notifications": set(),
        "server_requests": SERVER_REQUESTS,
        "unions": {
            "CommandExecutionApprovalDecision",
            "FileChange",
            "FileSystemPath",
            "FileSystemSpecialPath",
            "ParsedCommand",
            "ReviewDecision",
        },
    },
    "B5": {
        "subject": "Complete Codex reviews and guardian protocol",
        "requests": {
            "review/start",
            "thread/approveGuardianDeniedAction",
        },
        "notifications": {
            "guardianWarning",
            "item/autoApprovalReview/completed",
            "item/autoApprovalReview/started",
        },
        "server_requests": set(),
        "unions": {"GuardianApprovalReviewAction", "ReviewTarget"},
    },
}
BATCH_COUNTS = {"B2": 5, "B3": 13, "B4": 35, "B5": 15}
STAGE_SLICE_STATUS = {
    "Start": {"Complete": 0, "NotImplemented": 66, "Partial": 2},
    "B2": {"Complete": 5, "NotImplemented": 61, "Partial": 2},
    "B3": {"Complete": 18, "NotImplemented": 48, "Partial": 2},
    "B4": {"Complete": 53, "NotImplemented": 15, "Partial": 0},
    "B5": {"Complete": 68, "NotImplemented": 0, "Partial": 0},
}
STAGE_GLOBAL_STATUS = {
    "Start": EXPECTED_START_GLOBAL_STATUS,
    "B2": {
        "Complete": 217,
        "NotApplicable": 48,
        "NotImplemented": 116,
        "Partial": 6,
    },
    "B3": {
        "Complete": 230,
        "NotApplicable": 48,
        "NotImplemented": 103,
        "Partial": 6,
    },
    "B4": {
        "Complete": 265,
        "NotApplicable": 48,
        "NotImplemented": 70,
        "Partial": 4,
    },
    "B5": EXPECTED_FINAL_GLOBAL_STATUS,
}

PUBLIC_API = {
    "Commands": {
        "accessor": "typed::Client::commands",
        "header": "ai/openai/codex/typed/Commands.h",
        "methods": ["exec", "resize", "terminate", "write"],
    },
    "Filesystem": {
        "accessor": "typed::Client::filesystem",
        "header": "ai/openai/codex/typed/Filesystem.h",
        "methods": [
            "copy",
            "createDirectory",
            "getMetadata",
            "readDirectory",
            "readFile",
            "remove",
            "watch",
            "unwatch",
            "writeFile",
            "fuzzyFileSearch",
        ],
    },
    "PermissionProfiles": {
        "accessor": "typed::Client::permissionProfiles",
        "header": "ai/openai/codex/typed/PermissionProfiles.h",
        "methods": ["list"],
    },
    "Reviews": {
        "accessor": "typed::Client::reviews",
        "header": "ai/openai/codex/typed/Reviews.h",
        "methods": ["start"],
    },
    "Threads": {
        "accessor": "typed::Client::threads",
        "header": "ai/openai/codex/typed/Threads.h",
        "methods": ["approveGuardianDeniedAction"],
    },
    "Events": {
        "accessor": "typed::Client::events",
        "header": "ai/openai/codex/typed/Events.h",
        "methods": ["setOnEvent"],
    },
    "ServerRequests": {
        "accessor": "typed::Client::requests",
        "header": "ai/openai/codex/typed/ServerRequests.h",
        "methods": ["setOnRequest", "respond"],
    },
}
EXPECTED_INSTALLED_HEADERS = sorted(
    {
        "typed/Accounts.h",
        "typed/Client.h",
        "typed/CodexErrorInfo.h",
        "typed/Commands.h",
        "typed/Configuration.h",
        "typed/Conversation.h",
        "typed/Events.h",
        "typed/Filesystem.h",
        "typed/Items.h",
        "typed/Models.h",
        "typed/PermissionProfiles.h",
        "typed/Results.h",
        "typed/Reviews.h",
        "typed/ServerRequests.h",
        "typed/Threads.h",
        "typed/Turns.h",
        "typed/Types.h",
    }
)

SENSITIVE_FIELDS = sorted(
    {
        "approvalId",
        "callId",
        "command",
        "commandActions",
        "content",
        "cwd",
        "dataBase64",
        "deltaBase64",
        "description",
        "env",
        "event",
        "fileChanges",
        "grantRoot",
        "message",
        "path",
        "reason",
        "stderr",
        "stdin",
        "stdout",
        "unified_diff",
    }
)


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


def load_inputs(arguments: argparse.Namespace) -> Inputs:
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


def expected_keys() -> set[Key]:
    result = {
        *(Key("client_request", "ClientRequest", "method", name)
          for name in CLIENT_REQUESTS),
        *(Key("server_notification", "ServerNotification", "method", name)
          for name in SERVER_NOTIFICATIONS),
        *(Key("server_request", "ServerRequest", "method", name)
          for name in SERVER_REQUESTS),
    }
    for (domain, field), alternatives in UNION_FAMILIES.items():
        result.update(
            Key("tagged_union_discriminator", domain, field, name)
            for name in alternatives
        )
    return result


def slice_keys(inputs: Inputs) -> list[Key]:
    assigned = {
        key
        for key, row in inputs.assignments.items()
        if row.get("slice") == A1_3_SLICE
    }
    registered = {
        key
        for key, row in inputs.registry.items()
        if row.get("a1_slice") == A1_3_SLICE
    }
    expected = expected_keys()
    require(
        assigned == expected,
        "frozen assignment no longer contains the exact 68 A1.3 identities",
        "IdentitySetMismatch",
    )
    require(
        registered == expected,
        "production registry no longer contains the exact assigned A1.3 set",
        "RegistryIdentitySetMismatch",
    )
    require(
        len(expected) == 68,
        f"reviewed A1.3 denominator changed: {len(expected)}",
        "IdentityCountMismatch",
    )
    require(
        all(
            inputs.assignments[key].get("module") == MODULE
            and inputs.assignments[key].get("stability") == "stable"
            for key in expected
        ),
        "A1.3 module, slice, or stability assignment changed",
        "AssignmentMismatch",
    )
    return sorted(expected)


def batch_for(key: Key) -> str:
    for batch, details in BATCHES.items():
        if (
            key.category == "client_request"
            and key.name in details["requests"]
        ) or (
            key.category == "server_notification"
            and key.name in details["notifications"]
        ) or (
            key.category == "server_request"
            and key.name in details["server_requests"]
        ) or (
            key.category == "tagged_union_discriminator"
            and key.domain in details["unions"]
        ):
            return batch
    raise AuditError(
        f"A1.3 identity has no reviewed implementation batch: {key.compact()}",
        "BatchAssignmentMismatch",
    )


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
            f"git {' '.join(arguments)} failed while freezing the A1.3 base",
            "BaseCommitMismatch",
        )
    return completed.stdout.strip()


def _source(path: Path, repo_root: Path) -> dict[str, str]:
    return {
        "path": path.resolve().relative_to(repo_root).as_posix(),
        "sha256": shared.sha256_file(path),
    }


def start_state_document(
    arguments: argparse.Namespace, base_sha: str
) -> dict[str, Any]:
    inputs = load_inputs(arguments)
    keys = slice_keys(inputs)
    repo_root = arguments.repo_root
    tree = _git(repo_root, "rev-parse", f"{base_sha}^{{tree}}")
    require(
        base_sha == EXPECTED_BASE_SHA and tree == EXPECTED_BASE_TREE,
        f"unexpected A1.3 base {base_sha} tree {tree}",
        "BaseCommitMismatch",
    )
    slice_status = _counter(
        inputs.registry[key]["typed_schema_status"] for key in keys
    )
    global_status = _counter(
        row["typed_schema_status"] for row in inputs.registry.values()
    )
    require(
        slice_status == EXPECTED_START_SLICE_STATUS,
        f"A1.3 starting schema status changed: {slice_status}",
        "StartStatusMismatch",
    )
    require(
        global_status == EXPECTED_START_GLOBAL_STATUS,
        f"global A1.3 starting status changed: {global_status}",
        "GlobalStartStatusMismatch",
    )
    partial = {
        key
        for key in keys
        if inputs.registry[key]["typed_schema_status"] == "Partial"
    }
    require(
        partial == EXPECTED_PARTIAL,
        "the two frozen A1.3 Partial identities changed",
        "PartialSetMismatch",
    )
    return {
        "format_version": FORMAT_VERSION,
        "generated_notice": (
            "Frozen by tools/codex/app_server_a1_3.py freeze-start-state; "
            "review evidence only, never a runtime registry."
        ),
        "codex_version": CODEX_VERSION,
        "upstream_tag": UPSTREAM_TAG,
        "base_sha": base_sha,
        "base_tree": tree,
        "counts": {
            "identity_count": len(keys),
            "taxonomy": _counter(key.category for key in keys),
            "a1_3_schema_status": slice_status,
            "global_schema_status": global_status,
        },
        "initial_partial_identities": [
            key.object() for key in sorted(partial)
        ],
        "identities": [
            {
                "protocol_surface_key": key.object(),
                "assignment": inputs.assignments[key],
                "registry": inputs.registry[key],
            }
            for key in keys
        ],
        "unrelated_registry_identities": [
            {
                "protocol_surface_key": key.object(),
                "registry": row,
            }
            for key, row in sorted(inputs.registry.items())
            if key not in set(keys)
        ],
        "capture_sources": {
            "assignment": _source(arguments.assignments, repo_root),
            "contracts": _source(arguments.contracts, repo_root),
            "manifest": _source(arguments.manifest, repo_root),
            "provenance": _source(
                arguments.schema_root / "PROVENANCE.json", repo_root
            ),
            "registry": _source(arguments.registry, repo_root),
            "schema_completeness": _source(
                arguments.schema_completeness, repo_root
            ),
        },
    }


def validate_start_state(document: Mapping[str, Any]) -> None:
    require(
        document.get("format_version") == FORMAT_VERSION
        and document.get("codex_version") == CODEX_VERSION
        and document.get("upstream_tag") == UPSTREAM_TAG
        and document.get("base_sha") == EXPECTED_BASE_SHA
        and document.get("base_tree") == EXPECTED_BASE_TREE,
        "frozen A1.3 start-state metadata changed",
        "StartStateMetadataMismatch",
    )
    counts = document.get("counts")
    require(
        isinstance(counts, Mapping)
        and counts.get("identity_count") == 68
        and counts.get("taxonomy") == EXPECTED_TAXONOMY
        and counts.get("a1_3_schema_status") == EXPECTED_START_SLICE_STATUS
        and counts.get("global_schema_status")
        == EXPECTED_START_GLOBAL_STATUS,
        "frozen A1.3 start-state counts changed",
        "StartStateCountMismatch",
    )
    partial = document.get("initial_partial_identities")
    require(
        isinstance(partial, list)
        and {Key.from_row(row) for row in partial} == EXPECTED_PARTIAL,
        "frozen A1.3 initial Partial set changed",
        "StartStatePartialMismatch",
    )


def live_stage(keys: Sequence[Key], inputs: Inputs) -> str:
    slice_status = _counter(
        inputs.registry[key]["typed_schema_status"] for key in keys
    )
    global_status = _counter(
        row["typed_schema_status"] for row in inputs.registry.values()
    )
    matches = [
        stage
        for stage in STAGE_SLICE_STATUS
        if slice_status == {
            key: value
            for key, value in STAGE_SLICE_STATUS[stage].items()
            if value
        }
        and global_status == STAGE_GLOBAL_STATUS[stage]
    ]
    require(
        len(matches) == 1,
        (
            "registry progress is not an exact reviewed A1.3 stage: "
            f"slice={slice_status} global={global_status}"
        ),
        "ProgressStageMismatch",
    )
    stage = matches[0]
    order = {"Start": 0, "B2": 1, "B3": 2, "B4": 3, "B5": 4}
    for key in keys:
        completed = order[batch_for(key)] <= order[stage]
        expected_schema = (
            "Complete"
            if completed
            else "Partial"
            if key in EXPECTED_PARTIAL
            else "NotImplemented"
        )
        expected_implementation = (
            "Implemented"
            if completed or key in EXPECTED_PARTIAL
            else "NotImplemented"
        )
        row = inputs.registry[key]
        require(
            row["typed_schema_status"] == expected_schema
            and row["typed_status"] == expected_implementation,
            f"identity is outside its owning batch: {key.compact()}",
            "BatchProgressMismatch",
        )
    return stage


def _nullable(schema: Any) -> bool:
    if not isinstance(schema, Mapping):
        return False
    schema_type = schema.get("type")
    if schema_type == "null":
        return True
    if isinstance(schema_type, list) and "null" in schema_type:
        return True
    return any(
        isinstance(branch, Mapping) and branch.get("type") == "null"
        for keyword in ("anyOf", "oneOf")
        for branch in (
            schema.get(keyword, [])
            if isinstance(schema.get(keyword), list)
            else []
        )
    )


def _schema_value_kind(schema: Any) -> str:
    if not isinstance(schema, Mapping):
        return "unconstrained"
    if "$ref" in schema:
        return "reference"
    if "oneOf" in schema or "anyOf" in schema:
        return "union"
    schema_type = schema.get("type")
    if isinstance(schema_type, list):
        values = [value for value in schema_type if value != "null"]
        return "|".join(str(value) for value in values) or "null"
    return str(schema_type or "opaque")


def collect_schema_paths(
    nodes: Mapping[fixtures.DefinitionId, Any],
    closure: set[fixtures.DefinitionId],
) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []

    def walk(
        definition: fixtures.DefinitionId, schema: Any, path: str
    ) -> None:
        if isinstance(schema, list):
            for index, child in enumerate(schema):
                walk(definition, child, f"{path}[{index}]")
            return
        if not isinstance(schema, Mapping) or "$ref" in schema:
            return
        required_names = (
            set(schema.get("required", []))
            if isinstance(schema.get("required"), list)
            else set()
        )
        properties = schema.get("properties")
        if isinstance(properties, Mapping):
            for name, child in sorted(properties.items()):
                child_path = f"{path}.{name}"
                child_mapping = (
                    child if isinstance(child, Mapping) else {}
                )
                rows.append(
                    {
                        "definition": definition.to_json(),
                        "schema_path": child_path,
                        "node_kind": "property",
                        "field": name,
                        "required": name in required_names,
                        "nullable": _nullable(child),
                        "default_present": "default" in child_mapping,
                        "default": child_mapping.get("default"),
                        "value_kind": _schema_value_kind(child),
                        "integer_format": child_mapping.get("format"),
                        "minimum": child_mapping.get("minimum"),
                        "maximum": child_mapping.get("maximum"),
                        "additional_properties": child_mapping.get(
                            "additionalProperties", "unspecified"
                        ),
                        "sensitive": name in SENSITIVE_FIELDS,
                    }
                )
                walk(definition, child, child_path)
        items = schema.get("items")
        if isinstance(items, (Mapping, list)):
            rows.append(
                {
                    "definition": definition.to_json(),
                    "schema_path": f"{path}[]",
                    "node_kind": "array_element",
                    "field": None,
                    "required": True,
                    "nullable": _nullable(items),
                    "default_present": False,
                    "default": None,
                    "value_kind": _schema_value_kind(items),
                    "integer_format": (
                        items.get("format")
                        if isinstance(items, Mapping)
                        else None
                    ),
                    "minimum": None,
                    "maximum": None,
                    "additional_properties": "not_applicable",
                    "sensitive": False,
                }
            )
            walk(definition, items, f"{path}[]")
        additional = schema.get("additionalProperties")
        if isinstance(additional, Mapping):
            rows.append(
                {
                    "definition": definition.to_json(),
                    "schema_path": f"{path}{{}}",
                    "node_kind": "map_value",
                    "field": None,
                    "required": True,
                    "nullable": _nullable(additional),
                    "default_present": False,
                    "default": None,
                    "value_kind": _schema_value_kind(additional),
                    "integer_format": additional.get("format"),
                    "minimum": additional.get("minimum"),
                    "maximum": additional.get("maximum"),
                    "additional_properties": "schema",
                    "sensitive": False,
                }
            )
            walk(definition, additional, f"{path}{{}}")
        for keyword in ("allOf", "anyOf", "oneOf"):
            branches = schema.get(keyword)
            if isinstance(branches, list):
                for index, branch in enumerate(branches):
                    walk(
                        definition,
                        branch,
                        f"{path}<{keyword}:{index}>",
                    )

    for definition in sorted(closure):
        walk(
            definition,
            nodes[definition],
            f"{definition.namespace}:{definition.name}",
        )
    return rows


def schema_closure(
    arguments: argparse.Namespace,
    inputs: Inputs,
    keys: Sequence[Key],
) -> tuple[dict[str, Any], dict[Key, set[fixtures.DefinitionId]]]:
    draft07 = fixtures.load_draft07(arguments.draft07_validator)
    catalog = fixtures.SchemaCatalog(arguments.schema_root, draft07)
    aggregate = catalog.load(
        arguments.schema_root
        / "stable/codex_app_server_protocol.schemas.json"
    )
    nodes, edges = fixtures.definition_graph(aggregate)
    seeds: dict[fixtures.DefinitionId, list[dict[str, Any]]] = defaultdict(list)
    identity_definitions: dict[Key, set[fixtures.DefinitionId]] = defaultdict(
        set
    )

    def add(
        definition: fixtures.DefinitionId, role: str, key: Key
    ) -> None:
        record = {"role": role, "surface_key": key.object()}
        if record not in seeds[definition]:
            seeds[definition].append(record)
        reached = set(fixtures.transitive_definitions((definition,), edges))
        identity_definitions[key].update(reached)

    for key in keys:
        if key.category in {"client_request", "server_request"}:
            contract = inputs.contracts.get(key)
            require(
                contract is not None,
                f"A1.3 operation lacks a guarded contract: {key.compact()}",
                "ContractMismatch",
            )
            for role, type_identity in (
                ("request_params", contract["parameter_type_identity"]),
                (
                    "successful_response",
                    contract.get(
                        "result_schema_type_identity",
                        contract["result_type_identity"],
                    ),
                ),
            ):
                if type_identity == "Unit":
                    continue
                definition = fixtures.locate_definition_for_type(
                    catalog, nodes, type_identity
                )
                require(
                    definition is not None,
                    f"schema root is absent for {key.compact()} {role}",
                    "MissingResponseSchema",
                )
                add(definition, role, key)
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
            definition = fixtures.locate_definition_for_type(
                catalog, nodes, type_identity
            )
            require(
                definition is not None,
                f"notification schema root is absent: {key.compact()}",
                "MissingResponseSchema",
            )
            add(definition, "notification_params", key)
        elif key.category == "tagged_union_discriminator":
            legacy = fixtures.DefinitionId("legacy", key.domain)
            v2 = fixtures.DefinitionId("v2", key.domain)
            candidates = [value for value in (legacy, v2) if value in nodes]
            require(
                len(candidates) == 1,
                f"union family has ambiguous schema namespace: {key.domain}",
                "UnionSchemaMismatch",
            )
            add(candidates[0], "registered_union_family", key)

    closure = set(
        fixtures.transitive_definitions(seeds.keys(), edges)
    )
    paths = collect_schema_paths(nodes, closure)
    require(
        len(seeds) == 59
        and len(closure) == 123
        and _counter(value.namespace for value in closure)
        == {"legacy": 26, "v2": 97},
        "A1.3 transitive definition closure changed",
        "SchemaClosureMismatch",
    )
    require(
        len(paths) == 480
        and _counter(str(row["node_kind"]) for row in paths)
        == {
            "array_element": 34,
            "map_value": 3,
            "property": 443,
        },
        "A1.3 transitive schema-path closure changed",
        "SchemaPathMismatch",
    )
    return (
        {
            "counts": {
                "seed_definitions": len(seeds),
                "reachable_named_definitions": len(closure),
                "definition_namespaces": _counter(
                    value.namespace for value in closure
                ),
                "schema_paths": len(paths),
                "schema_path_kinds": _counter(
                    str(row["node_kind"]) for row in paths
                ),
                "required_paths": sum(
                    bool(row["required"]) for row in paths
                ),
                "optional_paths": sum(
                    not bool(row["required"]) for row in paths
                ),
                "nullable_paths": sum(
                    bool(row["nullable"]) for row in paths
                ),
                "default_bearing_paths": sum(
                    bool(row["default_present"]) for row in paths
                ),
                "integer_formats": _counter(
                    str(row["integer_format"])
                    for row in paths
                    if row["integer_format"] is not None
                ),
            },
            "seed_definitions": [
                {
                    "definition": definition.to_json(),
                    "associations": sorted(
                        records,
                        key=lambda row: (
                            row["role"],
                            Key.from_row(row["surface_key"]),
                        ),
                    ),
                }
                for definition, records in sorted(seeds.items())
            ],
            "definitions": [
                {
                    "definition": definition.to_json(),
                    "direct_dependencies": [
                        value.to_json()
                        for value in sorted(edges[definition] & closure)
                    ],
                    "schema_sha256": shared.sha256_json(nodes[definition]),
                }
                for definition in sorted(closure)
            ],
            "schema_paths": paths,
        },
        identity_definitions,
    )


def build_reports(
    arguments: argparse.Namespace,
) -> tuple[dict[str, Any], dict[str, Any]]:
    start_state = shared.load_json(arguments.start_state)
    validate_start_state(start_state)
    inputs = load_inputs(arguments)
    keys = slice_keys(inputs)
    stage = live_stage(keys, inputs)
    taxonomy = _counter(key.category for key in keys)
    require(
        taxonomy == EXPECTED_TAXONOMY,
        f"A1.3 taxonomy changed: {taxonomy}",
        "TaxonomyMismatch",
    )
    require(
        not ({key.name for key in keys} & EXCLUDED_METHODS),
        "an excluded A1.4 or experimental method leaked into A1.3",
        "ExperimentalLeakage",
    )

    request_keys = [
        key for key in keys if key.category == "client_request"
    ]
    server_request_keys = [
        key for key in keys if key.category == "server_request"
    ]
    contracts: dict[Key, Mapping[str, Any]] = {}
    for key in request_keys + server_request_keys:
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
            f"wrong request/result association: {key.compact()}",
            "ContractMismatch",
        )
        contracts[key] = contract
    result_kinds = _counter(
        str(contracts[key]["result_contract_kind"])
        for key in request_keys
    )
    require(
        result_kinds == EXPECTED_RESULT_KINDS,
        f"A1.3 client result-kind split changed: {result_kinds}",
        "ResultKindMismatch",
    )
    require(
        all(
            contracts[key]["result_contract_kind"] == "Concrete"
            for key in server_request_keys
        ),
        "all five A1.3 server requests require concrete responses",
        "ServerResponseContractMismatch",
    )

    closure, identity_definitions = schema_closure(
        arguments, inputs, keys
    )
    batch_counts = _counter(batch_for(key) for key in keys)
    require(
        batch_counts == BATCH_COUNTS,
        f"A1.3 implementation-batch map changed: {batch_counts}",
        "BatchAssignmentMismatch",
    )
    operations = []
    for key in request_keys + server_request_keys:
        contract = contracts[key]
        operations.append(
            {
                "protocol_surface_key": key.object(),
                "batch": batch_for(key),
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
                "direct_raw_protocol_response": (
                    key.category == "server_request"
                ),
                "depends_on_server_request_resolved": False,
            }
        )

    plan = {
        "format_version": FORMAT_VERSION,
        "generated_notice": (
            "Generated A1.3 review evidence; "
            "ProtocolSurfaceRegistryData.inc remains authoritative."
        ),
        "codex_version": CODEX_VERSION,
        "upstream_tag": UPSTREAM_TAG,
        "base_sha": start_state["base_sha"],
        "base_tree": start_state["base_tree"],
        "counts": {
            "identity_count": len(keys),
            "taxonomy": taxonomy,
            "result_contract_kinds": result_kinds,
            "server_request_response_contracts": len(
                server_request_keys
            ),
            "initial_a1_3_schema_status": EXPECTED_START_SLICE_STATUS,
            "current_a1_3_schema_status": _counter(
                inputs.registry[key]["typed_schema_status"]
                for key in keys
            ),
            "current_global_schema_status": _counter(
                row["typed_schema_status"]
                for row in inputs.registry.values()
            ),
            "expected_final_global_schema_status": (
                EXPECTED_FINAL_GLOBAL_STATUS
            ),
            "current_progress_stage": stage,
            "batch_identity_counts": batch_counts,
        },
        "identities": [
            {
                "protocol_surface_key": key.object(),
                "classification": inputs.assignments[key]["classification"],
                "module": inputs.assignments[key]["module"],
                "batch": batch_for(key),
                "runtime_disposition": inputs.registry[key][
                    "runtime_disposition"
                ],
                "runtime_target": inputs.registry[key]["runtime_target"],
                "implementation_status": inputs.registry[key][
                    "typed_status"
                ],
                "schema_status": inputs.registry[key][
                    "typed_schema_status"
                ],
                "schema_completeness": inputs.registry[key][
                    "schema_completeness"
                ],
                "fixture_ids": inputs.fixture_coverage[key].get(
                    "fixture_ids", []
                ),
                "reachable_definition_count": len(
                    identity_definitions[key]
                ),
            }
            for key in keys
        ],
        "operations": operations,
        "notifications": [
            {
                "protocol_surface_key": key.object(),
                "batch": batch_for(key),
                "params_type": inputs.manifest_rows[key]["params"]["type"],
            }
            for key in keys
            if key.category == "server_notification"
        ],
        "union_families": [
            {
                "domain": domain,
                "discriminator_notation": field,
                "alternatives": sorted(alternatives),
                "wire_note": (
                    "$variant is inventory notation and is never emitted "
                    "unless the concrete schema defines a member"
                    if field == "$variant"
                    else f"wire discriminator member is {field}"
                ),
            }
            for (domain, field), alternatives in sorted(
                UNION_FAMILIES.items()
            )
        ],
        "batches": [
            {
                "batch": batch,
                "subject": details["subject"],
                "identity_count": BATCH_COUNTS[batch],
                "dependencies": (
                    []
                    if batch == "B2"
                    else [f"B{int(batch[1]) - 1}"]
                ),
            }
            for batch, details in BATCHES.items()
        ],
        "public_api": PUBLIC_API,
        "installed_public_headers": EXPECTED_INSTALLED_HEADERS,
        "response_path": {
            "mechanism": (
                "incoming request -> pending occurrence/token/generation -> "
                "typed callback -> validated response -> respondOwned with "
                "the original JSON-RPC id"
            ),
            "server_request_resolved_in_slice": False,
            "server_request_resolved_transport_dependency": False,
            "concurrent_server_request_types": 5,
        },
        "boundaries": {
            "one_raw_protocol": True,
            "command_runner_added": False,
            "filesystem_implementation_added": False,
            "backend_command_expansion": False,
            "backend_state_expansion": False,
            "frontend_protocol_expansion": False,
            "experimental_methods": sorted(EXCLUDED_METHODS),
        },
        "sensitivity": {
            "fields": SENSITIVE_FIELDS,
            "production_payload_logging_permitted": False,
            "evidence_uses_synthetic_values_only": True,
        },
    }
    closure_report = {
        "format_version": FORMAT_VERSION,
        "codex_version": CODEX_VERSION,
        "base_sha": start_state["base_sha"],
        "base_tree": start_state["base_tree"],
        "counts": closure["counts"],
        "seed_definitions": closure["seed_definitions"],
        "definitions": closure["definitions"],
        "schema_paths": closure["schema_paths"],
        "reuse": {
            "SandboxPolicy": "typed::SandboxPolicy",
            "Turn": "typed::Turn",
            "thread_item_protocol": (
                "A1.1 remains distinct from one-off command/exec"
            ),
        },
    }
    validate_generated_reports(plan, closure_report)
    return plan, closure_report


def report_diagnostics(
    plan: Mapping[str, Any], closure: Mapping[str, Any]
) -> list[AuditDiagnostic]:
    diagnostics: list[AuditDiagnostic] = []

    def add(code: str, location: str, message: str) -> None:
        diagnostics.append(AuditDiagnostic(code, location, message))

    counts = plan.get("counts")
    if not isinstance(counts, Mapping):
        add("PlanCountMismatch", "$.counts", "counts must be an object")
    else:
        if counts.get("identity_count") != 68:
            add(
                "IdentityCountMismatch",
                "$.counts.identity_count",
                "A1.3 denominator must remain 68",
            )
        if counts.get("taxonomy") != EXPECTED_TAXONOMY:
            add(
                "TaxonomyMismatch",
                "$.counts.taxonomy",
                "A1.3 root/union taxonomy changed",
            )
        if counts.get("result_contract_kinds") != EXPECTED_RESULT_KINDS:
            add(
                "ResultKindMismatch",
                "$.counts.result_contract_kinds",
                "A1.3 Concrete/Unit result split changed",
            )
        if counts.get("server_request_response_contracts") != 5:
            add(
                "ServerResponseContractMismatch",
                "$.counts.server_request_response_contracts",
                "all five server requests need concrete responses",
            )
        if counts.get("batch_identity_counts") != BATCH_COUNTS:
            add(
                "BatchAssignmentMismatch",
                "$.counts.batch_identity_counts",
                "frozen implementation batches changed",
            )
    identity_rows = plan.get("identities")
    actual_keys = (
        {
            Key.from_row(row["protocol_surface_key"])
            for row in identity_rows
            if isinstance(row, Mapping)
            and isinstance(row.get("protocol_surface_key"), Mapping)
        }
        if isinstance(identity_rows, list)
        else set()
    )
    if actual_keys != expected_keys():
        add(
            "IdentitySetMismatch",
            "$.identities",
            "plan identity set is not the frozen A1.3 assignment",
        )
    operations = plan.get("operations")
    if not isinstance(operations, list) or len(operations) != 22:
        add(
            "ContractMismatch",
            "$.operations",
            "17 client and five server request contracts are required",
        )
    elif any(
        row.get("depends_on_server_request_resolved") is not False
        for row in operations
        if isinstance(row, Mapping)
    ):
        add(
            "ResponsePathMismatch",
            "$.operations",
            "serverRequest/resolved is not an A1.3 response dependency",
        )
    notifications = plan.get("notifications")
    actual_notification_keys = (
        {
            Key.from_row(row["protocol_surface_key"])
            for row in notifications
            if isinstance(row, Mapping)
            and isinstance(row.get("protocol_surface_key"), Mapping)
        }
        if isinstance(notifications, list)
        else set()
    )
    expected_notification_keys = {
        Key(
            "server_notification",
            "ServerNotification",
            "method",
            name,
        )
        for name in SERVER_NOTIFICATIONS
    }
    if actual_notification_keys != expected_notification_keys:
        add(
            "ContractMismatch",
            "$.notifications",
            "A1.3 notification roots changed or command lifecycles merged",
        )
    response_path = plan.get("response_path")
    if (
        not isinstance(response_path, Mapping)
        or response_path.get("server_request_resolved_in_slice") is not False
        or response_path.get(
            "server_request_resolved_transport_dependency"
        )
        is not False
        or response_path.get("concurrent_server_request_types") != 5
    ):
        add(
            "ResponsePathMismatch",
            "$.response_path",
            "direct reverse-request response path changed",
        )
    unions = plan.get("union_families")
    expected_unions = {
        (domain, field, tuple(sorted(alternatives)))
        for (domain, field), alternatives in UNION_FAMILIES.items()
    }
    actual_unions = (
        {
            (
                str(row.get("domain")),
                str(row.get("discriminator_notation")),
                tuple(row.get("alternatives", [])),
            )
            for row in unions
            if isinstance(row, Mapping)
        }
        if isinstance(unions, list)
        else set()
    )
    if actual_unions != expected_unions:
        add(
            "UnionSchemaMismatch",
            "$.union_families",
            "registered union alternatives or notation changed",
        )
    boundaries = plan.get("boundaries")
    if (
        not isinstance(boundaries, Mapping)
        or boundaries.get("one_raw_protocol") is not True
        or boundaries.get("backend_command_expansion") is not False
        or boundaries.get("backend_state_expansion") is not False
        or boundaries.get("frontend_protocol_expansion") is not False
    ):
        add(
            "BoundaryMismatch",
            "$.boundaries",
            "A1.3 protocol/application boundary changed",
        )
    closure_counts = closure.get("counts")
    if (
        not isinstance(closure_counts, Mapping)
        or closure_counts.get("seed_definitions") != 59
        or closure_counts.get("reachable_named_definitions") != 123
        or closure_counts.get("schema_paths") != 480
    ):
        add(
            "SchemaClosureMismatch",
            "$.counts",
            "transitive stable schema closure changed",
        )
    paths = closure.get("schema_paths")
    if not isinstance(paths, list) or len(paths) != 480:
        add(
            "SchemaPathMismatch",
            "$.schema_paths",
            "required/optional/nullable field inventory changed",
        )
    return sorted(set(diagnostics))


def validate_generated_reports(
    plan: Mapping[str, Any], closure: Mapping[str, Any]
) -> None:
    shared.validate_diagnostics(report_diagnostics(plan, closure))


def write_or_check(
    path: Path, document: Mapping[str, Any], check: bool
) -> None:
    shared.write_or_check(
        path,
        document,
        check,
        artifact_label="generated A1.3 audit",
    )


def parser() -> argparse.ArgumentParser:
    repo_root = Path(__file__).resolve().parents[2]
    evidence = repo_root / "tools/codex/app-server-evidence/0.144.6"
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
        "--schema-root",
        type=Path,
        default=repo_root / "tools/codex/app-server-schema/0.144.6",
    )
    result.add_argument(
        "--assignments",
        type=Path,
        default=evidence / "module-slice-assignment.json",
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
        "--fixture-index",
        type=Path,
        default=repo_root
        / "tools/codex/app-server-fixtures/0.144.6/index.json",
    )
    result.add_argument(
        "--draft07-validator",
        type=Path,
        default=repo_root / "tools/codex/draft07.py",
    )
    result.add_argument(
        "--start-state",
        type=Path,
        default=evidence / "a1-3-start-state.json",
    )
    result.add_argument("--base-sha")
    result.add_argument(
        "--plan-output",
        type=Path,
        default=evidence / "a1-3-implementation-plan.json",
    )
    result.add_argument(
        "--closure-output",
        type=Path,
        default=evidence / "a1-3-type-closure.json",
    )
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
    plan, closure = build_reports(arguments)
    check = arguments.mode == "check"
    write_or_check(arguments.plan_output, plan, check)
    write_or_check(arguments.closure_output, closure, check)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AuditError as error:
        print(
            f"app-server-a1-3: error: {error.code}: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
