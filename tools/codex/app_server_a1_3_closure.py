#!/usr/bin/env python3
"""Generate the deterministic final Codex A1.3 closure report.

The report is review evidence projected from the canonical production
ProtocolSurfaceRegistry and the frozen A1.3 audit artifacts. It is never a
runtime disposition, codec, dispatch, policy, or transport input.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import re
import sys
from collections import Counter
from pathlib import Path
from typing import Any, Callable, Iterable, Mapping, Sequence

sys.dont_write_bytecode = True

import app_server_a1_3 as a1_3
import app_server_a1_shared as shared
import app_server_surface as surface


FORMAT_VERSION = 1
CODEX_VERSION = "codex-cli 0.144.6"
UPSTREAM_TAG = "rust-v0.144.6"
STATUS_ORDER = ("Complete", "NotApplicable", "NotImplemented", "Partial")
EXPECTED_A1_3_STATUS = {
    "Complete": 68,
    "NotApplicable": 0,
    "NotImplemented": 0,
    "Partial": 0,
}
EXPECTED_GLOBAL_STATUS = {
    "Complete": 280,
    "NotApplicable": 48,
    "NotImplemented": 55,
    "Partial": 4,
}
# These two records describe the historical A1.3 closure boundary. Successor
# audit phases may extend the closure checker and source-package guard without
# rewriting the already captured A1.3 report; their live semantics are checked
# by successor-aware tokens below.
FROZEN_SUCCESSOR_MUTABLE_SOURCES = {
    "closure_generator": {
        "path": "tools/codex/app_server_a1_3_closure.py",
        "sha256":
            "678e8b97267919d61ec99a0a9fa3b259d3d3c1f20eca7cb91fc068cb182f87fe",
    },
    "source_package_guard": {
        "path": "tests/CodexSourcePackageTest.cmake",
        "sha256":
            "7a95b8cdd07dcc9702bb21bd249da09de1789efed44149f1d53401a919f08cce",
    },
}
EXPECTED_CLASSIFICATIONS = {
    "RootOwnedNestedUnion": 17,
    "SharedWithinSlice": 22,
    "StablePublicRoot": 29,
}
EXPECTED_MODULES = {"CommandsFilesystemReviewsApprovals": 68}
EXPECTED_RESPONSE_ROOTS = {
    "client_successful_results": 17,
    "concrete_client_results": 8,
    "server_request_responses": 5,
    "total": 22,
    "unit_client_results": 9,
}
EXPECTED_FIXTURE_TOTALS = {
    "negative": 3615,
    "positive": 2268,
    "total": 5883,
}
EXPECTED_FIXTURE_MUTATIONS = {
    "alternative_branch_acceptances": 1,
    "globally_optional_locations": 21164,
    "optional_cross_fragment_exclusions": 0,
    "optional_omissions_accepted": 21164,
    "optional_present_locations": 21164,
    "required_field_removals_rejected": 21267,
    "required_locations": 21267,
    "selected_branch_required_locations": 21267,
    "wrong_type_mutations_rejected": 21083,
    "wrong_type_unconstrained_exclusions": 184,
}
EXPECTED_FIXTURE_COVERAGE_COUNTS = {
    "identities_with_positive_fixtures": 316,
    "operation_role_actual": 194,
    "operation_role_expected": 194,
    "optional_omissions_accepted": 21164,
    "optional_present_locations": 21164,
    "positive_fixtures": 2268,
    "required_field_removals_rejected": 21267,
    "surface_identities": 387,
    "wrong_type_mutations_rejected": 21083,
    "wrong_type_unconstrained_exclusions": 184,
}
EXPECTED_SCHEMA_COMPLETENESS_COUNTS = {
    "facts_true_by_field": {
        "authoritative_root_association": 316,
        "fixture_current": 316,
        "independently_schema_validated": 316,
        "nullable_semantics_exercised": 280,
        "optional_omitted_exercised": 316,
        "optional_present_exercised": 316,
        "positive_fixture_coverage": 316,
        "reachable_union_alternatives_exercised": 280,
        "required_fields_exercised": 316,
        "schema_properties_exercised": 280,
    },
    "identities_with_positive_fixtures": 316,
    "surface_identities": 387,
}
EXPECTED_A1_3_FIXTURE_COUNTS = {
    "negative": 647,
    "positive": 396,
    "total": 1043,
}
EXPECTED_TREE_FINGERPRINTS = {
    "src/ai/openai/codex/backend": {
        "file_count": 14,
        "sha256":
            "00fb78d5b7f2451cad7c8e81cd9716af23dca0e58c932c18d8d86b152c6c5261",
    },
    "src/ai/openai/codex/frontend": {
        "file_count": 14,
        "sha256":
            "2f87dfea3a01a2e74b8b9637b156f8535351c880b6c2d92d0723765e484583ff",
    },
    "src/apps/codex-backend": {
        "file_count": 10,
        "sha256":
            "2e3bebfbd4a3060eb7b39a6dfa2f5590175a4bc7874594f77de6f31a983737b9",
    },
    "src/apps/codex-backend-client": {
        "file_count": 21,
        "sha256":
            "c96bb51b727fd4f6a8a31034f906476e1e10aa1357ae889b9a664f11c09ea337",
    },
}
EXPECTED_IMMUTABLE_BACKEND_HEADERS = {
    "src/ai/openai/codex/backend/BackendCommand.h":
        "702360f5f53e4c1959f6dbb81dec33f4a3d382bf9c2e72595e88af8a331b78f1",
    "src/ai/openai/codex/backend/BackendState.h":
        "044f48c22ffff76d91518f932e5c96a5883e613f00dfb2deac69006e0c0e8b1b",
}
EXPECTED_FRONTEND_PROTOCOL = {
    "docs/ai/openai/codex/frontend-protocol-v1.md":
        "5f53a6219f8dc45a09ec7ddca05f2f1f104ce0c7fee824de98492815fc502017",
    "docs/ai/openai/codex/frontend-protocol-v1.schema.json":
        "a27721164607b79a8b268c3adb035211a6efa82cb4645632b9b9a59302734c04",
}
EXPECTED_STABLE_SCHEMA_AGGREGATE = (
    "cee1ac3bcaf95e5fcdcf07499c7e6b00fc423b90c670ea3380f1799434b72add"
)
EXPECTED_EXPERIMENTAL_SCHEMA_AGGREGATE = (
    "4a0ef96787255364d99b15fe40fcfd6227901978d0cddc8b20340bfef98a0d1b"
)
EXPECTED_PROTOCOL_SOURCE_COMMIT = (
    "5d1fbf26c43abc65a203928b2e31561cb039e06d"
)
EXPECTED_DESCRIPTOR_ROWS = {
    "client_operations": 57,
    "server_notifications": 52,
    "server_requests": 6,
    "a1_3_unions": 39,
}
EXPECTED_A1_3_DESCRIPTOR_ROWS = {
    "client_operations": 17,
    "server_notifications": 7,
    "server_requests": 5,
    "a1_3_unions": 39,
}
EXPECTED_WIRE_TESTS = (
    "CodexA13CommandWireTest",
    "CodexA13FilesystemWireTest",
    "CodexA13ApprovalWireTest",
    "CodexA13ReviewWireTest",
)
EXPECTED_CODEC_TESTS = (
    "CodexA13CommandCodecTest",
    "CodexA13FilesystemCodecTest",
    "CodexA13ApprovalCodecTest",
    "CodexA13ReviewCodecTest",
)
EXPECTED_BOUNDARY_TESTS = (
    "CodexA13CommandBackendCompatibilityTest",
    "CodexA13FilesystemBackendCompatibilityTest",
    "CodexA13ApprovalBackendCompatibilityTest",
    "CodexA13ReviewBackendCompatibilityTest",
)
EXPECTED_INSTALLED_CONSUMERS = (
    "CodexCommandsHeaderConsumer.cpp",
    "CodexFilesystemHeaderConsumer.cpp",
    "CodexPermissionProfilesHeaderConsumer.cpp",
    "CodexReviewsHeaderConsumer.cpp",
    "CodexTypedConsumer.cpp",
)
EXPECTED_ACCEPTANCE_TEST_SOURCES = tuple(
    f"CodexA13{domain}{kind}Test.cpp"
    for domain in ("Command", "Filesystem", "Approval", "Review")
    for kind in ("Codec", "Wire", "BackendCompatibility")
)
EXPECTED_ABI_LAYOUTS = {
    "base": (
        "AppServerClient=16",
        "Client=8",
        "Event=2776",
        "CanonicalServerNotification=1400",
        "TypedServerRequest=312",
        "CommandApprovalRequest=304",
        "FileChangeApprovalRequest=248",
    ),
    "final": (
        "AppServerClient=16",
        "Client=8",
        "Event=2776",
        "CanonicalServerNotification=1400",
        "TypedServerRequest=960",
        "CommandApprovalRequest=952",
        "FileChangeApprovalRequest=512",
    ),
}
EXPECTED_ABI_BASE_HEADERS = {
    "src/ai/openai/codex/AppServerClient.h":
        "077e737996013f1445c7d8a1bb4d001b0a3b6693dc821e60da30133cf3b67269",
    "src/ai/openai/codex/typed/Client.h":
        "64367604ec5c21df4b6828f7b0aa1b4e1c6ae98bc56334996756b8d2e0afba53",
    "src/ai/openai/codex/typed/Events.h":
        "125fe356f43868573f1005e411c9b35ee39269f4b729dbe5d0a54736c91f17ca",
    "src/ai/openai/codex/typed/ServerRequests.h":
        "f34b297531f8140d0042a726080d0aa6f8342f9d2bf26bf489b51769a15c8b41",
}
EXPECTED_ABI_SYMBOLS = {
    "base_count": 47416,
    "base_sha256":
        "0bd89fe1c936ec777f86050d807c6695b1d810c85d8e7af1b3fae47c72eb68a9",
    "final_count": 56863,
    "final_sha256":
        "6b06827f8804c9a8a080a9d2cf30e4549a0e2517ceeb2f34ad87be8897edd0cb",
    "removed": 6286,
    "added": 15733,
}

AuditDiagnostic = shared.AuditDiagnostic
AuditError = shared.AuditError
Key = shared.Key


def require(condition: bool, message: str, code: str) -> None:
    shared.require(condition, message, code)


def key_from_object(value: Mapping[str, Any]) -> Key:
    return Key(
        str(value["category"]),
        str(value["domain"]),
        str(value["discriminator_field"]),
        str(value["name"]),
    )


def indexed(
    rows: Iterable[Mapping[str, Any]],
    key_of: Callable[[Mapping[str, Any]], Key],
    description: str,
) -> dict[Key, dict[str, Any]]:
    result: dict[Key, dict[str, Any]] = {}
    for raw in rows:
        row = dict(raw)
        key = key_of(row)
        require(
            key not in result,
            f"duplicate {description} identity: {key.compact()}",
            "ClosureDuplicateIdentity",
        )
        result[key] = row
    return result


def status_counts(rows: Iterable[Mapping[str, Any]]) -> dict[str, int]:
    counts = Counter(str(row["typed_schema_status"]) for row in rows)
    return {name: counts[name] for name in STATUS_ORDER}


def tree_fingerprint(repo_root: Path, relative: str) -> dict[str, Any]:
    root = repo_root / relative
    files = sorted(path for path in root.rglob("*") if path.is_file())
    digest = hashlib.sha256()
    for path in files:
        digest.update(path.relative_to(root).as_posix().encode("utf-8"))
        digest.update(b"\0")
        digest.update(hashlib.sha256(path.read_bytes()).digest())
        digest.update(b"\0")
    return {"file_count": len(files), "sha256": digest.hexdigest()}


def source_record(path: Path, repo_root: Path) -> dict[str, str]:
    return {
        "path": path.resolve().relative_to(repo_root).as_posix(),
        "sha256": shared.sha256_file(path),
    }


def logical_source_record(path: Path, logical_path: str) -> dict[str, str]:
    return {
        "path": logical_path,
        "sha256": shared.sha256_file(path),
    }


def require_tokens(
    path: Path,
    required: Iterable[str],
    code: str,
    description: str,
) -> str:
    try:
        text = path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        raise AuditError(
            f"unable to read {description} {path}: {error}", code
        ) from error
    missing = [token for token in required if token not in text]
    require(
        not missing,
        f"{description} lacks required evidence tokens: {missing}",
        code,
    )
    return text


def validate_provenance(
    arguments: argparse.Namespace,
    start_state: Mapping[str, Any],
) -> dict[str, Any]:
    provenance = shared.load_json(arguments.schema_root / "PROVENANCE.json")
    protocol_provenance = shared.load_json(arguments.protocol_provenance)
    capture_sources = start_state.get("capture_sources")
    require(
        isinstance(capture_sources, Mapping),
        "A1.3 start state lacks capture-source hashes",
        "ClosureProvenanceMismatch",
    )
    immutable_paths = {
        "assignment": arguments.assignments,
        "contracts": arguments.contracts,
        "manifest": arguments.manifest,
        "provenance": arguments.schema_root / "PROVENANCE.json",
    }
    for name, path in immutable_paths.items():
        record = capture_sources.get(name)
        require(
            isinstance(record, Mapping)
            and record.get("sha256") == shared.sha256_file(path),
            f"immutable A1.3 {name} authority changed",
            "ClosureProvenanceMismatch",
        )
    schema_trees = provenance.get("schema_trees")
    release = protocol_provenance.get("release")
    require(
        provenance.get("codex_version") == CODEX_VERSION
        and isinstance(schema_trees, Mapping)
        and schema_trees.get("stable", {}).get("aggregate_sha256")
        == EXPECTED_STABLE_SCHEMA_AGGREGATE
        and schema_trees.get("experimental", {}).get("aggregate_sha256")
        == EXPECTED_EXPERIMENTAL_SCHEMA_AGGREGATE
        and isinstance(release, Mapping)
        and release.get("tag") == UPSTREAM_TAG
        and release.get("source_commit_sha")
        == EXPECTED_PROTOCOL_SOURCE_COMMIT,
        "Codex pin, schema provenance, or protocol-source provenance changed",
        "ClosureProvenanceMismatch",
    )
    return {
        "codex_version": CODEX_VERSION,
        "experimental_schema_aggregate_sha256":
            EXPECTED_EXPERIMENTAL_SCHEMA_AGGREGATE,
        "protocol_source_commit": EXPECTED_PROTOCOL_SOURCE_COMMIT,
        "stable_schema_aggregate_sha256":
            EXPECTED_STABLE_SCHEMA_AGGREGATE,
        "upstream_tag": UPSTREAM_TAG,
    }


def validate_registry(
    rows: Sequence[Mapping[str, Any]],
    start_state: Mapping[str, Any],
) -> tuple[dict[Key, dict[str, Any]], dict[str, Any]]:
    registry = indexed(rows, Key.from_row, "registry")
    expected = a1_3.expected_keys()
    slice_rows = {
        key: row
        for key, row in registry.items()
        if row.get("a1_slice") == a1_3.A1_3_SLICE
    }
    require(
        set(slice_rows) == expected and len(slice_rows) == 68,
        "canonical registry is not the exact frozen 68-identity A1.3 set",
        "ClosureIdentityMismatch",
    )
    for key, row in slice_rows.items():
        completeness = row.get("schema_completeness")
        require(
            row.get("stability") == "stable"
            and row.get("typed_module") == a1_3.MODULE
            and row.get("runtime_disposition") == "Typed"
            and row.get("typed_status") == "Implemented"
            and row.get("typed_schema_status") == "Complete"
            and row.get("runtime_target") not in {"", "std::monostate{}"}
            and isinstance(completeness, Mapping)
            and bool(completeness)
            and all(value is True for value in completeness.values()),
            f"A1.3 identity is not schema-complete: {key.compact()}",
            "ClosureIdentityIncomplete",
        )
    targets = [str(row["runtime_target"]) for row in slice_rows.values()]
    require(
        len(targets) == len(set(targets)) == 68,
        "A1.3 registry targets are missing or duplicated",
        "ClosureRegistryTargetMismatch",
    )
    slice_status = status_counts(slice_rows.values())
    global_status = status_counts(registry.values())
    residual = {
        key
        for key, row in registry.items()
        if row.get("typed_schema_status") == "Partial"
    }
    expected_residual = {
        key
        for key in registry
        if key.name in a1_3.EXPECTED_RESIDUAL_PARTIAL_NAMES
    }

    unrelated_value = start_state.get("unrelated_registry_identities")
    require(
        isinstance(unrelated_value, list),
        "A1.3 start state lacks unrelated registry rows",
        "ClosureUnrelatedPromotion",
    )
    frozen_unrelated = indexed(
        (
            row["registry"]
            for row in unrelated_value
            if isinstance(row, Mapping)
            and isinstance(row.get("registry"), Mapping)
        ),
        Key.from_row,
        "frozen unrelated registry",
    )
    current_unrelated = {
        key: row for key, row in registry.items() if key not in expected
    }
    require(
        current_unrelated == frozen_unrelated,
        "a non-A1.3 registry identity changed or was promoted",
        "ClosureUnrelatedPromotion",
    )
    require(
        slice_status == EXPECTED_A1_3_STATUS
        and global_status == EXPECTED_GLOBAL_STATUS
        and residual == expected_residual
        and {key.name for key in residual}
        == a1_3.EXPECTED_RESIDUAL_PARTIAL_NAMES,
        "final A1.3/global status or residual Partial set changed",
        "ClosureStatusMismatch",
    )
    require(
        not ({key.name for key in slice_rows} & a1_3.EXCLUDED_METHODS),
        "an experimental or A1.4 operation leaked into A1.3",
        "ClosureExperimentalLeakage",
    )
    return registry, {
        "a1_3_schema_status": slice_status,
        "global_schema_status": global_status,
        "residual_partial": [
            key.object() for key in sorted(residual)
        ],
    }


def _positive_fixture(record: Mapping[str, Any]) -> bool:
    return (
        record.get("expected_valid") is not False
        and "negative_case" not in record
    )


def validate_fixture_evidence(
    fixture_index: Mapping[str, Any],
    fixture_coverage: Mapping[str, Any],
    schema_completeness: Mapping[str, Any],
    plan_identities: Mapping[Key, Mapping[str, Any]],
) -> dict[str, Any]:
    counts = fixture_index.get("counts")
    fixture_totals = (
        {
            name: int(counts[name])
            for name in ("negative", "positive", "total")
        }
        if isinstance(counts, Mapping)
        else {}
    )
    require(
        fixture_totals == EXPECTED_FIXTURE_TOTALS
        and fixture_index.get("mutation_counts")
        == EXPECTED_FIXTURE_MUTATIONS
        and fixture_coverage.get("counts")
        == EXPECTED_FIXTURE_COVERAGE_COUNTS
        and schema_completeness.get("counts")
        == EXPECTED_SCHEMA_COMPLETENESS_COUNTS,
        "final fixture, mutation, or schema-completeness totals changed",
        "ClosureFixtureCountMismatch",
    )
    fixture_value = fixture_index.get("fixtures")
    completeness_value = schema_completeness.get("records")
    require(
        isinstance(fixture_value, list)
        and isinstance(completeness_value, list),
        "fixture or completeness evidence lacks identity records",
        "ClosureFixtureCoverageMismatch",
    )
    a1_fixtures = [
        record
        for record in fixture_value
        if isinstance(record, Mapping)
        and record.get("a1_slice") == a1_3.A1_3_SLICE
    ]
    positive = [record for record in a1_fixtures if _positive_fixture(record)]
    negative = [record for record in a1_fixtures if not _positive_fixture(record)]
    require(
        {
            "negative": len(negative),
            "positive": len(positive),
            "total": len(a1_fixtures),
        }
        == EXPECTED_A1_3_FIXTURE_COUNTS,
        "A1.3 fixture corpus count changed",
        "ClosureFixtureCountMismatch",
    )
    positive_by_key: dict[Key, list[Mapping[str, Any]]] = {
        key: [] for key in plan_identities
    }
    for record in positive:
        key_value = record.get("protocol_surface_key")
        require(
            isinstance(key_value, Mapping),
            "positive A1.3 fixture lacks a protocol key",
            "ClosureFixtureCoverageMismatch",
        )
        key = key_from_object(key_value)
        require(
            key in positive_by_key,
            f"positive fixture is outside A1.3: {key.compact()}",
            "ClosureFixtureCoverageMismatch",
        )
        positive_by_key[key].append(record)
    completeness = indexed(
        (
            row
            for row in completeness_value
            if isinstance(row, Mapping)
            and isinstance(row.get("protocol_surface_key"), Mapping)
        ),
        lambda row: key_from_object(row["protocol_surface_key"]),
        "schema completeness",
    )
    required_roles = {
        "client_request": {"client_request_params", "client_request_result"},
        "server_notification": {"server_notification_identity"},
        "server_request": {"server_request_params", "server_request_response"},
        "tagged_union_discriminator": {"union_branch"},
    }
    for key, plan_row in plan_identities.items():
        records = positive_by_key[key]
        ids = sorted(str(record["id"]) for record in records)
        roles = {str(record["role"]) for record in records}
        evidence = completeness.get(key)
        require(
            bool(records)
            and required_roles[key.category].issubset(roles)
            and sorted(plan_row.get("fixture_ids", [])) == ids
            and isinstance(evidence, Mapping)
            and sorted(evidence.get("fixture_ids", [])) == ids
            and evidence.get("positive_fixture_count") == len(ids)
            and all(
                value is True
                for value in evidence.get(
                    "schema_fixture_facts", {}
                ).values()
            ),
            f"incomplete root/response/union fixture coverage: {key.compact()}",
            "ClosureFixtureCoverageMismatch",
        )
    role_counts = dict(
        sorted(Counter(str(record["role"]) for record in a1_fixtures).items())
    )
    require(
        role_counts.get("client_request_params") == 17
        and role_counts.get("client_request_result") == 17
        and role_counts.get("server_notification_identity") == 7
        and role_counts.get("server_request_params") == 5
        and role_counts.get("server_request_response") == 5
        and role_counts.get("union_branch") == 39,
        "A1.3 root/response/union fixture role counts changed",
        "ClosureFixtureCoverageMismatch",
    )
    return {
        "a1_3": EXPECTED_A1_3_FIXTURE_COUNTS,
        "global": EXPECTED_FIXTURE_TOTALS,
        "required_root_roles": {
            name: role_counts[name]
            for name in (
                "client_request_params",
                "client_request_result",
                "server_notification_identity",
                "server_request_params",
                "server_request_response",
                "union_branch",
            )
        },
    }


def validate_descriptors(
    arguments: argparse.Namespace,
    manifest: Mapping[str, Any],
) -> dict[str, Any]:
    evidence = surface.load_a1_registry_evidence(arguments.evidence_root)
    specifications = {
        "client_operations": (
            arguments.operation_descriptors,
            surface.generate_client_operation_descriptor_data(
                dict(manifest), evidence
            ),
            "CODEX_CLIENT_OPERATION_CODEC_DESCRIPTOR(",
        ),
        "server_notifications": (
            arguments.notification_descriptors,
            surface.generate_server_notification_descriptor_data(
                dict(manifest), evidence
            ),
            "CODEX_SERVER_NOTIFICATION_CODEC_DESCRIPTOR(",
        ),
        "server_requests": (
            arguments.server_request_descriptors,
            surface.generate_server_request_descriptor_data(
                dict(manifest), evidence
            ),
            "CODEX_SERVER_REQUEST_CODEC_DESCRIPTOR(",
        ),
        "a1_3_unions": (
            arguments.union_descriptors,
            surface.generate_commands_filesystem_reviews_approvals_union_descriptor_data(
                dict(manifest), arguments.schema_root, evidence
            ),
            (
                "CODEX_COMMANDS_FILESYSTEM_REVIEWS_APPROVALS_"
                "UNION_CODEC_DESCRIPTOR("
            ),
        ),
    }
    result: dict[str, Any] = {}
    for name, (path, generated, marker) in specifications.items():
        try:
            committed = path.read_text(encoding="utf-8")
        except (OSError, UnicodeError) as error:
            raise AuditError(
                f"unable to read generated descriptor {path}: {error}",
                "ClosureDescriptorMismatch",
            ) from error
        require(
            committed == generated
            and committed.count(marker) == EXPECTED_DESCRIPTOR_ROWS[name],
            f"generated {name} descriptor is stale or incomplete",
            "ClosureDescriptorMismatch",
        )
        result[name] = {
            "a1_3_rows": EXPECTED_A1_3_DESCRIPTOR_ROWS[name],
            "generated_rows": EXPECTED_DESCRIPTOR_ROWS[name],
            **source_record(path, arguments.repo_root),
        }
    return result


def validate_api_abi(arguments: argparse.Namespace) -> dict[str, Any]:
    evidence = shared.load_json(arguments.abi_evidence)
    authority = evidence.get("authority")
    probe = evidence.get("layout_probe")
    symbols = evidence.get("shared_library_symbols")
    conclusion = evidence.get("conclusion")
    require(
        evidence.get("format_version") == 1
        and isinstance(authority, Mapping)
        and authority.get("base_sha") == a1_3.EXPECTED_BASE_SHA
        and authority.get("base_tree") == a1_3.EXPECTED_BASE_TREE
        and authority.get("codex_version") == CODEX_VERSION
        and authority.get("upstream_tag") == UPSTREAM_TAG
        and authority.get("final_boundary")
        == "Complete Codex reviews and guardian protocol"
        and isinstance(probe, Mapping)
        and isinstance(symbols, Mapping)
        and conclusion
        == {
            "binary_compatible": False,
            "installed_consumers_must_rebuild": True,
            "reason": (
                "Public aggregate and std::variant layouts changed; an "
                "unchanged SOVERSION or symbol list is not "
                "binary-compatibility proof."
            ),
            "soversion": 1,
        },
        "A1.3 API/ABI capture authority changed",
        "ClosureAbiEvidenceMismatch",
    )
    probe_source = arguments.abi_probe
    require(
        probe.get("source")
        == "tests/installed/codex/CodexA13AbiLayoutProbe.cpp"
        and probe.get("source_sha256") == shared.sha256_file(probe_source)
        and probe.get("compile_command")
        == (
            "g++ -std=c++20 -I{source}/src "
            "-I{build}/_deps/json-src/single_include "
            "{repo}/tests/installed/codex/CodexA13AbiLayoutProbe.cpp "
            "-o {probe_binary}"
        )
        and probe.get("run_command") == "{probe_binary}",
        "A1.3 ABI layout probe or reproduction command changed",
        "ClosureAbiEvidenceMismatch",
    )
    for boundary, expected_lines in EXPECTED_ABI_LAYOUTS.items():
        record = probe.get(boundary)
        require(
            isinstance(record, Mapping)
            and record.get("stdout_lines") == list(expected_lines)
            and record.get("stdout_sha256")
            == hashlib.sha256(
                ("\n".join(expected_lines) + "\n").encode("utf-8")
            ).hexdigest(),
            f"A1.3 {boundary} ABI layout capture changed",
            "ClosureAbiEvidenceMismatch",
        )
    require(
        probe["base"].get("header_sha256") == EXPECTED_ABI_BASE_HEADERS,
        "frozen-base public header hashes changed",
        "ClosureAbiEvidenceMismatch",
    )
    final_headers = {
        relative: shared.sha256_file(arguments.repo_root / relative)
        for relative in EXPECTED_ABI_BASE_HEADERS
    }
    require(
        probe["final"].get("header_sha256") == final_headers,
        "final public headers do not match the ABI capture",
        "ClosureAbiEvidenceMismatch",
    )
    base_symbols = symbols.get("base")
    final_symbols = symbols.get("final")
    require(
        isinstance(base_symbols, Mapping)
        and isinstance(final_symbols, Mapping)
        and base_symbols.get("symbol_count")
        == EXPECTED_ABI_SYMBOLS["base_count"]
        and base_symbols.get("symbol_list_sha256")
        == EXPECTED_ABI_SYMBOLS["base_sha256"]
        and final_symbols.get("symbol_count")
        == EXPECTED_ABI_SYMBOLS["final_count"]
        and final_symbols.get("symbol_list_sha256")
        == EXPECTED_ABI_SYMBOLS["final_sha256"]
        and symbols.get("removed_from_base")
        == EXPECTED_ABI_SYMBOLS["removed"]
        and symbols.get("added_in_final")
        == EXPECTED_ABI_SYMBOLS["added"]
        and symbols.get("extract_command")
        == (
            "nm -D --defined-only --format=posix {library} | "
            "cut -d ' ' -f1 | LC_ALL=C sort -u > {symbols}"
        )
        and symbols.get("comparison_commands")
        == [
            "comm -23 {base_symbols} {final_symbols} | wc -l",
            "comm -13 {base_symbols} {final_symbols} | wc -l",
        ]
        and symbols.get("tool_availability")
        == {"abidiff": False, "nm": True},
        "A1.3 shared-library symbol capture changed",
        "ClosureAbiEvidenceMismatch",
    )
    return {
        "capture": evidence,
        "capture_source": source_record(
            arguments.abi_evidence, arguments.repo_root
        ),
        "live_final_header_sha256": final_headers,
        "live_probe_source": source_record(
            probe_source, arguments.repo_root
        ),
    }


def validate_public_api(
    arguments: argparse.Namespace,
    plan: Mapping[str, Any],
) -> dict[str, Any]:
    require(
        plan.get("public_api") == a1_3.PUBLIC_API
        and plan.get("installed_public_headers")
        == a1_3.EXPECTED_INSTALLED_HEADERS,
        "A1.3 public API plan changed",
        "ClosurePublicApiMismatch",
    )
    cmake = require_tokens(
        arguments.codex_cmake,
        ("DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/ai/openai/codex/typed",),
        "ClosureInstalledHeaderMismatch",
        "Codex install CMake",
    )
    match = re.search(
        r"install\(\s*FILES (?P<body>typed/Accounts\.h.*?)"
        r"DESTINATION \$\{CMAKE_INSTALL_INCLUDEDIR\}/snode\.c/"
        r"ai/openai/codex/typed",
        cmake,
        flags=re.DOTALL,
    )
    installed = (
        sorted(set(re.findall(r"typed/[A-Za-z0-9]+\.h", match.group("body"))))
        if match
        else []
    )
    require(
        installed == a1_3.EXPECTED_INSTALLED_HEADERS
        and all(
            (arguments.repo_root / "src/ai/openai/codex" / header).is_file()
            for header in installed
        ),
        "installed typed header inventory changed",
        "ClosureInstalledHeaderMismatch",
    )
    token_map = {
        "typed/Client.h": (
            "Commands& commands() noexcept;",
            "Filesystem& filesystem() noexcept;",
            "PermissionProfiles& permissionProfiles() noexcept;",
            "Reviews& reviews() noexcept;",
        ),
        "typed/Commands.h": (
            "Submission exec(CommandExecParams params",
            "Submission resize(CommandExecResizeParams params",
            "Submission terminate(CommandExecTerminateParams params",
            "Submission write(CommandExecWriteParams params",
        ),
        "typed/Filesystem.h": (
            "Submission copy(FsCopyParams params",
            "Submission createDirectory(FsCreateDirectoryParams params",
            "Submission getMetadata(FsGetMetadataParams params",
            "Submission readDirectory(FsReadDirectoryParams params",
            "Submission readFile(FsReadFileParams params",
            "Submission remove(FsRemoveParams params",
            "Submission watch(FsWatchParams params",
            "Submission unwatch(FsUnwatchParams params",
            "Submission writeFile(FsWriteFileParams params",
            "Submission fuzzyFileSearch(FuzzyFileSearchParams params",
        ),
        "typed/PermissionProfiles.h": (
            "Submission list(PermissionProfileListParams params",
        ),
        "typed/Reviews.h": (
            "Submission start(ReviewStartParams params",
            "Turn turn;",
        ),
        "typed/Threads.h": (
            "Submission approveGuardianDeniedAction(",
        ),
    }
    for relative, tokens in token_map.items():
        text = require_tokens(
            arguments.repo_root / "src/ai/openai/codex" / relative,
            tokens,
            "ClosurePublicApiMismatch",
            relative,
        )
        require(
            "client.request(" not in text
            and " raw(" not in text,
            f"generic raw API leaked into {relative}",
            "ClosurePublicApiMismatch",
        )
    installed_cmake = require_tokens(
        arguments.installed_cmake,
        (
            "Commands Filesystem",
            "PermissionProfiles Reviews",
            "Codex${domain}HeaderConsumer.cpp",
            "CodexTypedConsumer.cpp",
        ),
        "ClosureInstalledConsumerMismatch",
        "installed-consumer CMake",
    )
    del installed_cmake
    for name in EXPECTED_INSTALLED_CONSUMERS:
        require(
            (arguments.installed_cmake.parent / name).is_file(),
            f"missing installed consumer {name}",
            "ClosureInstalledConsumerMismatch",
        )
    return {
        "facades": plan["public_api"],
        "installed_consumers": list(EXPECTED_INSTALLED_CONSUMERS),
        "installed_typed_headers": installed,
    }


def validate_transport_and_wire(
    arguments: argparse.Namespace,
) -> dict[str, Any]:
    test_root = arguments.component_test_root
    cmake = require_tokens(
        arguments.component_cmake,
        (
            *EXPECTED_WIRE_TESTS,
            *EXPECTED_CODEC_TESTS,
            *EXPECTED_BOUNDARY_TESTS,
            "CodexA13AuditToolTest",
            "CodexA13ClosureEvidenceTest",
        ),
        "ClosureTestIntegrityMismatch",
        "Codex component-test registration",
    )
    require(
        not any(
            re.search(
                rf"{re.escape(name)}.*?DISABLED\s+TRUE",
                cmake,
                flags=re.DOTALL,
            )
            for name in (
                *EXPECTED_WIRE_TESTS,
                *EXPECTED_CODEC_TESTS,
                *EXPECTED_BOUNDARY_TESTS,
            )
        ),
        "an A1.3 acceptance test was disabled",
        "ClosureTestIntegrityMismatch",
    )
    client = require_tokens(
        arguments.repo_root / "src/ai/openai/codex/typed/Client.cpp",
        (
            "AppServerClient::RawProtocol& protocol",
            "protocol->respondOwned(",
            "ApplyPatchApprovalRequest",
            "ExecCommandApprovalRequest",
            "CommandApprovalRequest",
            "FileChangeApprovalRequest",
            "PermissionsApprovalRequest",
        ),
        "ClosureResponsePathMismatch",
        "typed client response path",
    )
    raw_protocol = require_tokens(
        arguments.repo_root / "src/ai/openai/codex/AppServerClient.cpp",
        (
            "pendingServerRequests",
            "connectionGeneration",
            "respondOwned(",
            "pendingServerRequests.erase(id)",
        ),
        "ClosureResponsePathMismatch",
        "RawProtocol response path",
    )
    require(
        "serverRequest/resolved" not in client
        and "serverRequest/resolved" not in raw_protocol,
        "A1.3 response delivery depends on serverRequest/resolved",
        "ClosureResponsePathMismatch",
    )
    approval_wire = require_tokens(
        test_root / "CodexA13ApprovalWireTest.cpp",
        (
            "InitialRequestCount = 5",
            "socketpair(AF_UNIX",
            '"applyPatchApproval"',
            '"execCommandApproval"',
            '"item/commandExecution/requestApproval"',
            '"item/fileChange/requestApproval"',
            '"item/permissions/requestApproval"',
            "out-of-order responses retain exact IDs, schemas, decisions, and JSONL bytes",
            "duplicate responses for all five request types are rejected",
            "reconnect creates new occurrence tokens for all five reused request IDs",
            "stale transport callbacks cannot resurrect a server request",
        ),
        "ClosureServerRequestLifecycleMismatch",
        "five-request AF_UNIX acceptance test",
    )
    del approval_wire
    command_wire = require_tokens(
        test_root / "CodexA13CommandWireTest.cpp",
        (
            "socketpair(AF_UNIX",
            "chunks precede command/exec completion",
            "reentrant",
            "cancel",
            "command restart installs a distinct transport generation",
        ),
        "ClosureWireEvidenceMismatch",
        "command AF_UNIX acceptance test",
    )
    del command_wire
    for test in ("Filesystem", "Review"):
        require_tokens(
            test_root / f"CodexA13{test}WireTest.cpp",
            ("socketpair(AF_UNIX", "reentrant", "callbackGenerations"),
            "ClosureWireEvidenceMismatch",
            f"{test} AF_UNIX acceptance test",
        )
    acceptance_sources = {
        name: logical_source_record(
            test_root / name,
            f"tests/component/codex/{name}",
        )
        for name in EXPECTED_ACCEPTANCE_TEST_SOURCES
    }
    return {
        "acceptance_test_sources": acceptance_sources,
        "direct_raw_protocol_response": True,
        "server_request_resolved_dependency": False,
        "server_request_types_concurrently_pending": 5,
        "out_of_order_response_correlation": True,
        "cross_delivery_prevented": True,
        "wire_tests": list(EXPECTED_WIRE_TESTS),
    }


def validate_boundaries(arguments: argparse.Namespace) -> dict[str, Any]:
    trees = {
        relative: tree_fingerprint(arguments.repo_root, relative)
        for relative in EXPECTED_TREE_FINGERPRINTS
    }
    immutable_headers = {
        relative: shared.sha256_file(arguments.repo_root / relative)
        for relative in EXPECTED_IMMUTABLE_BACKEND_HEADERS
    }
    frontend_protocol = {
        relative: shared.sha256_file(arguments.repo_root / relative)
        for relative in EXPECTED_FRONTEND_PROTOCOL
    }
    require(
        trees == EXPECTED_TREE_FINGERPRINTS
        and immutable_headers == EXPECTED_IMMUTABLE_BACKEND_HEADERS
        and frontend_protocol == EXPECTED_FRONTEND_PROTOCOL,
        "A1.3 BackendCore/frontend/application boundary changed",
        "ClosureBoundaryFingerprintMismatch",
    )
    forbidden = re.compile(
        r"#include\s*<filesystem>|std::filesystem::"
        r"|(?<![A-Za-z0-9_])::fork\s*\("
        r"|(?<![A-Za-z0-9_])::exec(?:l|v|ve|vp)?\s*\("
        r"|(?<![A-Za-z0-9_])::posix_spawn"
        r"|::system\s*\(|::popen\s*\(|\binotify_|\bkqueue\s*\("
        r"|ReadDirectoryChangesW|#include\s*<thread>"
    )
    violations: list[str] = []
    production_root = arguments.repo_root / "src/ai/openai/codex"
    for path in sorted(production_root.rglob("*")):
        if path.suffix not in {".cpp", ".h"}:
            continue
        text = path.read_text(encoding="utf-8")
        if forbidden.search(text):
            violations.append(path.relative_to(arguments.repo_root).as_posix())
    require(
        not violations,
        f"local command/filesystem/thread implementation leaked into A1.3: {violations}",
        "ClosureLocalImplementationLeakage",
    )
    return {
        "backend_command_and_state_unchanged": immutable_headers,
        "frontend_protocol_v1": frontend_protocol,
        "source_trees": trees,
        "forbidden_local_implementation_matches": [],
    }


def validate_package_and_integrity(
    arguments: argparse.Namespace,
) -> dict[str, Any]:
    integrity = require_tokens(
        arguments.test_integrity,
        (
            "# Codex A1.3 test-integrity accounting",
            "No pre-existing test was deleted",
            "Long-running targets intentionally skipped",
            "AddressSanitizer",
            "LeakSanitizer",
            "UndefinedBehaviorSanitizer",
            "optional live App Server smoke",
            "CodexSyntheticSecretLeakGuardTest",
        ),
        "ClosureTestIntegrityMismatch",
        "A1.3 test-integrity report",
    )
    del integrity
    require_tokens(
        arguments.repo_root / "tests/policy/CMakeLists.txt",
        ("CodexSyntheticSecretLeakGuardTest",),
        "ClosureSecurityGuardMismatch",
        "security-test registration",
    )
    secret_guard_path = (
        arguments.repo_root
        / "tests/policy/security/CodexSyntheticSecretLeakGuardTest.py"
    )
    specification = importlib.util.spec_from_file_location(
        "snodec_a1_3_secret_guard", secret_guard_path
    )
    require(
        specification is not None and specification.loader is not None,
        "unable to load the synthetic-secret guard",
        "ClosureSecurityGuardMismatch",
    )
    assert specification is not None and specification.loader is not None
    secret_guard_module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = secret_guard_module
    try:
        specification.loader.exec_module(secret_guard_module)
        secret_guard_module.run_negative_self_test()
        secret_guard = secret_guard_module.SecretLeakGuard(
            arguments.repo_root, ()
        )
        secret_guard.validate_source_allowlist()
        source_scopes = ("src", "tests", "tools", "docs")
        for relative in source_scopes:
            secret_guard.scan_tree(
                arguments.repo_root / relative,
                f"source/{relative}",
                build_root=None,
            )
        secret_findings = tuple(
            sorted(set(secret_guard.findings))
        )
    except Exception as error:
        raise AuditError(
            "synthetic-secret guard could not complete its source-tree "
            "self-check",
            "ClosureSecurityGuardMismatch",
        ) from error
    require(
        not secret_findings,
        "synthetic-secret source-tree guard reported a finding",
        "ClosureSecurityGuardMismatch",
    )
    successor_compatibility = require_tokens(
        Path(__file__).resolve(),
        (
            "FROZEN_SUCCESSOR_MUTABLE_SOURCES",
            '"A1.4 partition and implementation-plan audit"',
        ),
        "ClosurePackageMismatch",
        "successor-aware A1.3 closure checker",
    )
    del successor_compatibility
    source_package = require_tokens(
        arguments.source_package_test,
        (
            'assert_retained_prefix("tools/codex/app-server-evidence/0.144.6" 27)',
            'assert_retained_prefix("docs/ai/openai/codex" 19)',
            "top_level_codex_tool_count EQUAL 14",
            '"tools/codex/app_server_a1_3.py"',
            '"tools/codex/app_server_a1_3_closure.py"',
            '"tools/codex/app-server-evidence/0.144.6/a1-3-closure-report.json"',
            '"tools/codex/app-server-evidence/0.144.6/a1-3-api-abi-evidence.json"',
            '"tests/component/codex/CodexA13ClosureEvidenceTest.py"',
            '"tests/installed/codex/CodexA13AbiLayoutProbe.cpp"',
            '"docs/ai/openai/codex/a1-3-test-integrity.md"',
            '"A1.3 implementation-plan/type-closure audit"',
            '"A1.3 final closure report"',
            '"tools/codex/app_server_a1_4.py"',
            '"tools/codex/app-server-evidence/0.144.6/a1-4-start-state.json"',
            '"tools/codex/app-server-evidence/0.144.6/a1-4-total-partition.json"',
            '"tools/codex/app-server-evidence/0.144.6/a1-4-type-closure.json"',
            '"tools/codex/app-server-evidence/0.144.6/a1-4-implementation-plan.json"',
            '"tools/codex/app-server-evidence/0.144.6/a1-final-cross-slice-ledger.json"',
            '"tests/component/codex/CodexA14AuditToolTest.py"',
            '"A1.4 partition and implementation-plan audit"',
        ),
        "ClosurePackageMismatch",
        "source-package closure guard",
    )
    del source_package
    binary_package = require_tokens(
        arguments.binary_package_test,
        (
            '"typed/Commands.h"',
            '"typed/Filesystem.h"',
            '"typed/PermissionProfiles.h"',
            '"typed/Reviews.h"',
            "forbidden_binary_patterns",
        ),
        "ClosurePackageMismatch",
        "binary-package API guard",
    )
    del binary_package
    typed_consumer = require_tokens(
        arguments.repo_root
        / "tests/installed/codex/CodexTypedConsumer.cpp",
        (
            "std::variant_size_v<typed::CanonicalServerNotification> == 51",
            "std::variant_size_v<typed::Event> == 53",
            "std::variant_size_v<typed::TypedServerRequest> == 8",
            "sizeof(ai::openai::codex::AppServerClient) == 2 * sizeof(void*)",
            "sizeof(typed::Client) == sizeof(void*)",
        ),
        "ClosureAbiEvidenceMismatch",
        "installed API/ABI consumer",
    )
    del typed_consumer
    return {
        "binary_package_private_evidence_excluded": True,
        "secret_guard": {
            "closure_source_scope_scan": {
                "finding_count": 0,
                "scopes": list(source_scopes),
                "status": "passed",
            },
            "finding_count": 0,
            "negative_self_test": "passed",
            "package_build_tree_scan": (
                "registered final validation; not claimed by this "
                "build-independent closure check"
            ),
            "registered_test": "CodexSyntheticSecretLeakGuardTest",
        },
        "source_package_offline_checks": [
            "A1.3 implementation-plan/type-closure audit",
            "A1.3 final closure report",
        ],
        "test_integrity_report":
            source_record(arguments.test_integrity, arguments.repo_root),
    }


def build_report(arguments: argparse.Namespace) -> dict[str, Any]:
    plan = shared.load_json(arguments.plan)
    closure = shared.load_json(arguments.type_closure)
    start_state = shared.load_json(arguments.start_state)
    fixture_index = shared.load_json(arguments.fixture_index)
    fixture_coverage = shared.load_json(arguments.fixture_coverage)
    schema_completeness = shared.load_json(arguments.schema_completeness)
    manifest = shared.load_json(arguments.manifest)
    a1_3.validate_start_state(start_state)
    a1_3.validate_generated_reports(plan, closure)

    audit_arguments = argparse.Namespace(**vars(arguments))
    audit_arguments.plan_output = arguments.plan
    audit_arguments.closure_output = arguments.type_closure
    rebuilt_plan, rebuilt_closure = a1_3.build_reports(audit_arguments)
    require(
        rebuilt_plan == plan and rebuilt_closure == closure,
        "live A1.3 authorities do not reproduce the checked-in audit",
        "ClosureAuditAuthorityMismatch",
    )
    provenance = validate_provenance(arguments, start_state)
    registry_rows = surface.parse_registry_data(arguments.registry)
    registry, registry_evidence = validate_registry(
        registry_rows, start_state
    )
    plan_identity_value = plan.get("identities")
    require(
        isinstance(plan_identity_value, list),
        "A1.3 plan lacks identity records",
        "ClosureIdentityMismatch",
    )
    plan_identities = indexed(
        (
            row
            for row in plan_identity_value
            if isinstance(row, Mapping)
            and isinstance(row.get("protocol_surface_key"), Mapping)
        ),
        lambda row: key_from_object(row["protocol_surface_key"]),
        "A1.3 plan",
    )
    require(
        set(plan_identities) == a1_3.expected_keys(),
        "A1.3 plan and frozen identity set differ",
        "ClosureIdentityMismatch",
    )
    fixture_evidence = validate_fixture_evidence(
        fixture_index,
        fixture_coverage,
        schema_completeness,
        plan_identities,
    )
    descriptors = validate_descriptors(arguments, manifest)
    api_abi = validate_api_abi(arguments)
    public_api = validate_public_api(arguments, plan)
    transport = validate_transport_and_wire(arguments)
    boundaries = validate_boundaries(arguments)
    package_integrity = validate_package_and_integrity(arguments)

    taxonomy = dict(
        sorted(Counter(key.category for key in plan_identities).items())
    )
    classifications = dict(
        sorted(
            Counter(
                str(row["classification"])
                for row in plan_identities.values()
            ).items()
        )
    )
    modules = dict(
        sorted(
            Counter(
                str(row["module"]) for row in plan_identities.values()
            ).items()
        )
    )
    require(
        taxonomy == a1_3.EXPECTED_TAXONOMY
        and classifications == EXPECTED_CLASSIFICATIONS
        and modules == EXPECTED_MODULES,
        "A1.3 taxonomy, classification, or module split changed",
        "ClosureTaxonomyMismatch",
    )

    client_operations = sorted(
        (
            operation
            for operation in plan["operations"]
            if operation["protocol_surface_key"]["category"]
            == "client_request"
        ),
        key=lambda row: row["protocol_surface_key"]["name"],
    )
    server_requests = sorted(
        (
            operation
            for operation in plan["operations"]
            if operation["protocol_surface_key"]["category"]
            == "server_request"
        ),
        key=lambda row: row["protocol_surface_key"]["name"],
    )
    notifications = sorted(
        plan["notifications"],
        key=lambda row: row["protocol_surface_key"]["name"],
    )
    result_kinds = dict(
        sorted(
            Counter(str(row["result_kind"]) for row in client_operations).items()
        )
    )
    require(
        len(client_operations) == 17
        and result_kinds == a1_3.EXPECTED_RESULT_KINDS
        and len(server_requests) == 5
        and all(row["result_kind"] == "Concrete" for row in server_requests)
        and len(notifications) == 7,
        "A1.3 operation/result/notification contract counts changed",
        "ClosureOperationContractMismatch",
    )
    response_roots = {
        "client": [
            {
                "method": row["protocol_surface_key"]["name"],
                "result_kind": row["result_kind"],
                "result_schema_type": row["result_schema_type"],
                "result_type": row["result_type"],
            }
            for row in client_operations
        ],
        "server_request": [
            {
                "method": row["protocol_surface_key"]["name"],
                "response_type": row["result_type"],
            }
            for row in server_requests
        ],
    }

    union_families = plan["union_families"]
    union_alternatives = [
        {
            "alternative": alternative,
            "discriminator_notation": family["discriminator_notation"],
            "domain": family["domain"],
        }
        for family in union_families
        for alternative in family["alternatives"]
    ]
    require(
        len(union_alternatives) == 39,
        "A1.3 union alternative denominator changed",
        "ClosureUnionMismatch",
    )

    complete_identities = []
    for key in sorted(plan_identities):
        row = registry[key]
        plan_row = plan_identities[key]
        complete_identities.append(
            {
                "batch": plan_row["batch"],
                "fixture_count": len(plan_row["fixture_ids"]),
                "fixture_ids_sha256":
                    shared.sha256_json(sorted(plan_row["fixture_ids"])),
                "protocol_surface_key": key.object(),
                "runtime_target": row["runtime_target"],
                "schema_completeness": row["schema_completeness"],
            }
        )

    staged = []
    cumulative: set[Key] = set()
    for batch in ("B2", "B3", "B4", "B5"):
        owned = {
            key
            for key, row in plan_identities.items()
            if row["batch"] == batch
        }
        cumulative |= owned
        require(
            len(owned) == a1_3.BATCH_COUNTS[batch]
            and all(
                registry[key]["typed_schema_status"] == "Complete"
                for key in cumulative
            ),
            f"A1.3 staged closure changed at {batch}",
            "ClosureBatchRatchetMismatch",
        )
        staged.append(
            {
                "batch": batch,
                "complete_cumulative_identities":
                    [key.object() for key in sorted(cumulative)],
                "complete_cumulative_identity_count": len(cumulative),
                "complete_owned_identities":
                    [key.object() for key in sorted(owned)],
                "complete_owned_identity_count": len(owned),
                "derived_global_schema_status":
                    a1_3.STAGE_GLOBAL_STATUS[batch],
            }
        )
    require(
        len(cumulative) == 68,
        "A1.3 B5 cumulative ratchet is not exact",
        "ClosureBatchRatchetMismatch",
    )

    documentation = require_tokens(
        arguments.documentation,
        (
            "68 A1.3 Complete",
            "280 Complete",
            "`serverRequest/resolved` belongs to A1.4",
            "The five reverse requests",
            "ABI changes",
            "SOVERSION remains 1",
        ),
        "ClosureDocumentationMismatch",
        "A1.3 status document",
    )
    del documentation
    source_paths = {
        "api_abi_evidence": arguments.abi_evidence,
        "api_abi_probe": arguments.abi_probe,
        "assignment": arguments.assignments,
        "audit_generator": Path(a1_3.__file__).resolve(),
        "closure_generator": Path(__file__).resolve(),
        "closure_test": arguments.closure_test,
        "component_test_registration": arguments.component_cmake,
        "contracts": arguments.contracts,
        "documentation": arguments.documentation,
        "fixture_coverage": arguments.fixture_coverage,
        "fixture_index": arguments.fixture_index,
        "implementation_plan": arguments.plan,
        "manifest": arguments.manifest,
        "protocol_provenance": arguments.protocol_provenance,
        "registry": arguments.registry,
        "schema_completeness": arguments.schema_completeness,
        "schema_provenance": arguments.schema_root / "PROVENANCE.json",
        "source_package_guard": arguments.source_package_test,
        "start_state": arguments.start_state,
        "test_integrity": arguments.test_integrity,
        "type_closure": arguments.type_closure,
    }
    source_records = {
        name: source_record(path, arguments.repo_root)
        for name, path in sorted(source_paths.items())
    }
    source_records.update(FROZEN_SUCCESSOR_MUTABLE_SOURCES)
    return {
        "api_abi": api_abi,
        "authority": {
            "base_sha": a1_3.EXPECTED_BASE_SHA,
            "base_tree": a1_3.EXPECTED_BASE_TREE,
            "production_authority":
                "ai::openai::codex::detail::ProtocolSurfaceRegistry",
            "provenance": provenance,
            "report_role":
                "non-authoritative deterministic review and closure evidence",
        },
        "boundaries": boundaries,
        "codex_version": CODEX_VERSION,
        "counts": {
            "a1_3_schema_status":
                registry_evidence["a1_3_schema_status"],
            "classifications": classifications,
            "fixture_corpus": fixture_evidence,
            "global_schema_status":
                registry_evidence["global_schema_status"],
            "modules": modules,
            "response_root_obligations_not_identities":
                EXPECTED_RESPONSE_ROOTS,
            "result_contract_kinds": result_kinds,
            "schema_completeness":
                EXPECTED_SCHEMA_COMPLETENESS_COUNTS,
            "taxonomy": taxonomy,
            "type_closure": closure["counts"],
        },
        "descriptors": descriptors,
        "exact_complete_a1_3_identities": complete_identities,
        "exact_residual_partial_identities":
            registry_evidence["residual_partial"],
        "format_version": FORMAT_VERSION,
        "generated_notice": (
            "Generated by tools/codex/app_server_a1_3_closure.py; do not edit."
        ),
        "notifications": notifications,
        "operations": client_operations,
        "package_and_test_integrity": package_integrity,
        "public_api": public_api,
        "response_root_obligations": response_roots,
        "server_requests": server_requests,
        "sources": source_records,
        "staged_exact_complete_ratchets": staged,
        "transport_and_lifecycle": transport,
        "union_alternatives": union_alternatives,
        "union_families": union_families,
        "upstream_tag": UPSTREAM_TAG,
    }


def report_diagnostics(
    actual: Mapping[str, Any], expected: Mapping[str, Any]
) -> list[AuditDiagnostic]:
    diagnostics: list[AuditDiagnostic] = []

    def compare(code: str, location: str, key: str) -> None:
        if actual.get(key) != expected.get(key):
            diagnostics.append(
                AuditDiagnostic(code, location, "closure evidence changed")
            )

    compare("ClosureAbiEvidenceMismatch", "$.api_abi", "api_abi")
    compare("ClosureAuthorityMismatch", "$.authority", "authority")
    compare("ClosureBoundaryFingerprintMismatch", "$.boundaries", "boundaries")
    actual_counts = actual.get("counts")
    expected_counts = expected.get("counts")
    if not isinstance(actual_counts, Mapping):
        actual_counts = {}
    if not isinstance(expected_counts, Mapping):
        expected_counts = {}
    for field, code in (
        ("a1_3_schema_status", "ClosureStatusMismatch"),
        ("classifications", "ClosureTaxonomyMismatch"),
        ("fixture_corpus", "ClosureFixtureCountMismatch"),
        ("global_schema_status", "ClosureStatusMismatch"),
        ("modules", "ClosureTaxonomyMismatch"),
        (
            "response_root_obligations_not_identities",
            "ClosureResponseObligationMismatch",
        ),
        ("result_contract_kinds", "ClosureOperationContractMismatch"),
        ("schema_completeness", "ClosureFixtureCoverageMismatch"),
        ("taxonomy", "ClosureTaxonomyMismatch"),
        ("type_closure", "ClosureTypeClosureMismatch"),
    ):
        if actual_counts.get(field) != expected_counts.get(field):
            diagnostics.append(
                AuditDiagnostic(
                    code, f"$.counts.{field}", "closure count changed"
                )
            )
    compare("ClosureDescriptorMismatch", "$.descriptors", "descriptors")
    compare(
        "ClosureIdentityMismatch",
        "$.exact_complete_a1_3_identities",
        "exact_complete_a1_3_identities",
    )
    compare(
        "ClosureStatusMismatch",
        "$.exact_residual_partial_identities",
        "exact_residual_partial_identities",
    )
    compare("ClosureNotificationMismatch", "$.notifications", "notifications")
    compare("ClosureOperationContractMismatch", "$.operations", "operations")
    compare(
        "ClosurePackageMismatch",
        "$.package_and_test_integrity",
        "package_and_test_integrity",
    )
    compare("ClosurePublicApiMismatch", "$.public_api", "public_api")
    compare(
        "ClosureResponseObligationMismatch",
        "$.response_root_obligations",
        "response_root_obligations",
    )
    compare(
        "ClosureServerRequestMismatch",
        "$.server_requests",
        "server_requests",
    )
    compare("ClosureSourceMismatch", "$.sources", "sources")
    compare(
        "ClosureBatchRatchetMismatch",
        "$.staged_exact_complete_ratchets",
        "staged_exact_complete_ratchets",
    )
    compare(
        "ClosureResponsePathMismatch",
        "$.transport_and_lifecycle",
        "transport_and_lifecycle",
    )
    compare("ClosureUnionMismatch", "$.union_alternatives", "union_alternatives")
    compare("ClosureUnionMismatch", "$.union_families", "union_families")
    for field in (
        "codex_version",
        "format_version",
        "generated_notice",
        "upstream_tag",
    ):
        if actual.get(field) != expected.get(field):
            diagnostics.append(
                AuditDiagnostic(
                    "ClosureMetadataMismatch",
                    f"$.{field}",
                    "closure metadata changed",
                )
            )
    return sorted(set(diagnostics))


def validate_report(
    actual: Mapping[str, Any], expected: Mapping[str, Any]
) -> None:
    shared.validate_diagnostics(report_diagnostics(actual, expected))


def write_or_check(
    path: Path, report: Mapping[str, Any], check: bool
) -> None:
    shared.write_or_check(
        path,
        report,
        check,
        artifact_label="generated A1.3 closure report",
    )


def parser() -> argparse.ArgumentParser:
    repo_root = Path(__file__).resolve().parents[2]
    evidence = repo_root / "tools/codex/app-server-evidence/0.144.6"
    result = argparse.ArgumentParser(description=__doc__)
    result.add_argument("mode", choices=("generate", "check"))
    result.add_argument("--repo-root", type=Path, default=repo_root)
    result.add_argument(
        "--manifest",
        type=Path,
        default=repo_root / "tools/codex/app-server-surface/0.144.6.json",
    )
    result.add_argument(
        "--schema-root",
        type=Path,
        default=repo_root / "tools/codex/app-server-schema/0.144.6",
    )
    result.add_argument(
        "--protocol-provenance",
        type=Path,
        default=(
            repo_root
            / "tools/codex/app-server-protocol-source/0.144.6/PROVENANCE.json"
        ),
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
        "--draft07-validator",
        type=Path,
        default=repo_root / "tools/codex/draft07.py",
    )
    result.add_argument(
        "--start-state",
        type=Path,
        default=evidence / "a1-3-start-state.json",
    )
    result.add_argument(
        "--plan",
        type=Path,
        default=evidence / "a1-3-implementation-plan.json",
    )
    result.add_argument(
        "--type-closure",
        type=Path,
        default=evidence / "a1-3-type-closure.json",
    )
    result.add_argument(
        "--fixture-coverage",
        type=Path,
        default=evidence / "fixture-coverage.json",
    )
    result.add_argument(
        "--fixture-index",
        type=Path,
        default=(
            repo_root / "tools/codex/app-server-fixtures/0.144.6/index.json"
        ),
    )
    result.add_argument(
        "--schema-completeness",
        type=Path,
        default=evidence / "schema-completeness-evidence.json",
    )
    result.add_argument(
        "--registry",
        type=Path,
        default=(
            repo_root
            / "src/ai/openai/codex/detail/ProtocolSurfaceRegistryData.inc"
        ),
    )
    result.add_argument(
        "--evidence-root",
        type=Path,
        default=evidence,
    )
    detail = repo_root / "src/ai/openai/codex/detail"
    result.add_argument(
        "--operation-descriptors",
        type=Path,
        default=detail / "ClientOperationCodecDescriptors.inc",
    )
    result.add_argument(
        "--notification-descriptors",
        type=Path,
        default=detail / "ServerNotificationCodecDescriptors.inc",
    )
    result.add_argument(
        "--server-request-descriptors",
        type=Path,
        default=detail / "ServerRequestCodecDescriptors.inc",
    )
    result.add_argument(
        "--union-descriptors",
        type=Path,
        default=(
            detail
            / "CommandsFilesystemReviewsApprovalsUnionCodecDescriptors.inc"
        ),
    )
    result.add_argument(
        "--codex-cmake",
        type=Path,
        default=repo_root / "src/ai/openai/codex/CMakeLists.txt",
    )
    result.add_argument(
        "--component-cmake",
        type=Path,
        default=repo_root / "tests/component/codex/CMakeLists.txt",
    )
    result.add_argument(
        "--component-test-root",
        type=Path,
        default=repo_root / "tests/component/codex",
    )
    result.add_argument(
        "--closure-test",
        type=Path,
        default=(
            repo_root
            / "tests/component/codex/CodexA13ClosureEvidenceTest.py"
        ),
    )
    result.add_argument(
        "--installed-cmake",
        type=Path,
        default=repo_root / "tests/installed/codex/CMakeLists.txt",
    )
    result.add_argument(
        "--source-package-test",
        type=Path,
        default=repo_root / "tests/CodexSourcePackageTest.cmake",
    )
    result.add_argument(
        "--binary-package-test",
        type=Path,
        default=repo_root / "tests/CodexBinaryPackageTest.cmake",
    )
    result.add_argument(
        "--documentation",
        type=Path,
        default=(
            repo_root
            / "docs/ai/openai/codex/"
            "a1-3-commands-filesystem-reviews-approvals.md"
        ),
    )
    result.add_argument(
        "--test-integrity",
        type=Path,
        default=(
            repo_root
            / "docs/ai/openai/codex/a1-3-test-integrity.md"
        ),
    )
    result.add_argument(
        "--abi-evidence",
        type=Path,
        default=evidence / "a1-3-api-abi-evidence.json",
    )
    result.add_argument(
        "--abi-probe",
        type=Path,
        default=(
            repo_root / "tests/installed/codex/CodexA13AbiLayoutProbe.cpp"
        ),
    )
    result.add_argument(
        "--output",
        type=Path,
        default=evidence / "a1-3-closure-report.json",
    )
    return result


def main(argv: Sequence[str] | None = None) -> int:
    arguments = parser().parse_args(argv)
    for name, value in vars(arguments).items():
        if isinstance(value, Path):
            setattr(arguments, name, value.resolve())
    report = build_report(arguments)
    write_or_check(arguments.output, report, arguments.mode == "check")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AuditError, surface.SurfaceError) as error:
        code = getattr(error, "code", "ClosureInputError")
        print(
            f"app-server-a1-3-closure: error [{code}]: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
