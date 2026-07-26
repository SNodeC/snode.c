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
        changed_paths.update(
            line
            for line in _git(
                arguments.repo_root,
                "diff",
                "--name-only",
                EXPECTED_BASE_SHA,
            ).splitlines()
            if line
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
            "changed_paths": sorted(changed_paths),
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
    partition, closure = build_foundation(arguments)
    check = arguments.mode == "check"
    write_or_check(arguments.partition_output, partition, check)
    write_or_check(arguments.closure_output, closure, check)
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
