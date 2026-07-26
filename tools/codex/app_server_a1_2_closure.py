#!/usr/bin/env python3
"""Generate the deterministic final Codex A1.2 closure report.

The report is review evidence projected from the canonical production
ProtocolSurfaceRegistry and the existing A1.2 audit artifacts.  It is never a
runtime disposition, codec, or dispatch input.
"""

from __future__ import annotations

import argparse
import hashlib
import sys
from collections import Counter
from pathlib import Path
from typing import Any, Callable, Iterable, Mapping, Sequence

sys.dont_write_bytecode = True

import app_server_a1_2 as a1_2
import app_server_a1_shared as shared
import app_server_surface as surface


CODEX_VERSION = "codex-cli 0.144.6"
FORMAT_VERSION = 1
STATUS_ORDER = ("Complete", "NotApplicable", "NotImplemented", "Partial")
EXPECTED_A1_2_STATUS = {
    "Complete": 45,
    "NotApplicable": 0,
    "NotImplemented": 0,
    "Partial": 0,
}
EXPECTED_RESPONSE_ROOT_OBLIGATIONS = {
    "client_successful_results": 18,
    "server_request_responses": 1,
    "total": 19,
}
EXPECTED_FIXTURE_TOTALS = {
    "negative": 2934,
    "positive": 1881,
    "total": 4815,
}
EXPECTED_FIXTURE_MUTATIONS = {
    "alternative_branch_acceptances": 1,
    "globally_optional_locations": 19631,
    "optional_cross_fragment_exclusions": 0,
    "optional_omissions_accepted": 19631,
    "optional_present_locations": 19631,
    "required_field_removals_rejected": 19229,
    "required_locations": 19229,
    "selected_branch_required_locations": 19229,
    "wrong_type_mutations_rejected": 19051,
    "wrong_type_unconstrained_exclusions": 178,
}
EXPECTED_FIXTURE_COVERAGE_COUNTS = {
    "identities_with_positive_fixtures": 270,
    "operation_role_actual": 194,
    "operation_role_expected": 194,
    "optional_omissions_accepted": 19631,
    "optional_present_locations": 19631,
    "positive_fixtures": 1881,
    "required_field_removals_rejected": 19229,
    "surface_identities": 387,
    "wrong_type_mutations_rejected": 19051,
    "wrong_type_unconstrained_exclusions": 178,
}
EXPECTED_SCHEMA_COMPLETENESS_COUNTS = {
    "facts_true_by_field": {
        "authoritative_root_association": 270,
        "fixture_current": 270,
        "independently_schema_validated": 270,
        "nullable_semantics_exercised": 212,
        "optional_omitted_exercised": 270,
        "optional_present_exercised": 270,
        "positive_fixture_coverage": 270,
        "reachable_union_alternatives_exercised": 212,
        "required_fields_exercised": 270,
        "schema_properties_exercised": 212,
    },
    "identities_with_positive_fixtures": 270,
    "surface_identities": 387,
}
EXPECTED_A1_3_SUCCESSOR_GLOBAL_STATUS = {
    "Complete": 280,
    "NotApplicable": 48,
    "NotImplemented": 55,
    "Partial": 4,
}
EXPECTED_A1_3_SUCCESSOR_RESIDUAL_PARTIAL_KEYS = frozenset(
    {
        shared.Key(
            "client_notification",
            "ClientNotification",
            "method",
            "initialized",
        ),
        shared.Key(
            "client_request",
            "ClientRequest",
            "method",
            "initialize",
        ),
        shared.Key(
            "server_notification",
            "ServerNotification",
            "method",
            "error",
        ),
        shared.Key(
            "server_request",
            "ServerRequest",
            "method",
            "item/tool/requestUserInput",
        ),
    }
)
EXPECTED_A1_3_SUCCESSOR_FIXTURE_TOTALS = {
    "negative": 3615,
    "positive": 2268,
    "total": 5883,
}
EXPECTED_A1_3_SUCCESSOR_FIXTURE_MUTATIONS = {
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
EXPECTED_A1_3_SUCCESSOR_FIXTURE_COVERAGE_COUNTS = {
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
EXPECTED_A1_3_SUCCESSOR_SCHEMA_COMPLETENESS_COUNTS = {
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
EXPECTED_TREE_FINGERPRINTS = {
    "src/ai/openai/codex/backend": {
        "file_count": 14,
        "sha256":
            "615169ee0121293f1a8d53d9bab8b83960d3060251ba38ff648bae9b31192228",
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
EXPECTED_A1_3_SUCCESSOR_TREE_FINGERPRINTS = {
    **EXPECTED_TREE_FINGERPRINTS,
    "src/ai/openai/codex/backend": {
        "file_count": 14,
        "sha256":
            "00fb78d5b7f2451cad7c8e81cd9716af23dca0e58c932c18d8d86b152c6c5261",
    },
}
EXPECTED_FRONTEND_PROTOCOL_FINGERPRINTS = {
    "docs/ai/openai/codex/frontend-protocol-v1.md":
        "5f53a6219f8dc45a09ec7ddca05f2f1f104ce0c7fee824de98492815fc502017",
    "docs/ai/openai/codex/frontend-protocol-v1.schema.json":
        "a27721164607b79a8b268c3adb035211a6efa82cb4645632b9b9a59302734c04",
}
FROZEN_SOURCE_SHA256 = {
    "fixture_coverage":
        "958fd18a918c75a37cdf215ea25addfbd83ab7c225e8e93b1b217c9a5b770e07",
    "fixture_index":
        "2900b6691f8133af9730ad0e207511f5b986861c24506896f17b49a5dc793f7c",
    "generator":
        "354ad78630df928e2184360322e28f547510493e2ef1fd2377a2b316c05146b5",
    "implementation_plan":
        "ff5ab66b79cd53c614a93a8d5cd1a065c54b8a251c706adeb805c900f43f44c4",
    "registry":
        "b845d442def7ba672122fa9c4f0ab28703d244d92be2c41706aa614b80d01948",
    "schema_completeness":
        "c60fd80b369a78224f101d291f56ba26c33c3a574caf9c181fc4c27b9434b21b",
    "type_closure":
        "2b8bc5de1c56b1ce7b809939a744ec1027a78574adfc9e5e8c1e966a385047cb",
}
HISTORICALLY_MUTABLE_SOURCE_NAMES = frozenset(
    {
        "fixture_coverage",
        "fixture_index",
        "generator",
        "registry",
        "schema_completeness",
    }
)

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


def source_records(
    paths: Mapping[str, Path], repo_root: Path
) -> dict[str, dict[str, str]]:
    return shared.historical_source_records(
        paths,
        repo_root,
        frozen_hashes=FROZEN_SOURCE_SHA256,
        mutable_names=HISTORICALLY_MUTABLE_SOURCE_NAMES,
        source_set_error=lambda: AuditError(
            "A1.2 closure historical source set changed",
            "ClosureSourceMismatch",
        ),
        immutable_source_error=lambda relative: AuditError(
            f"immutable A1.2 closure source changed: {relative}",
            "ClosureSourceMismatch",
        ),
        resolve_paths=True,
    )


def normalized_fixture_totals(
    fixture_index: Mapping[str, Any],
) -> dict[str, int]:
    counts = fixture_index.get("counts")
    require(
        isinstance(counts, Mapping),
        "fixture index lacks count evidence",
        "ClosureFixtureCountMismatch",
    )
    return {name: int(counts[name]) for name in ("negative", "positive", "total")}


def validate_successor_registry(
    registry: Mapping[Key, Mapping[str, Any]],
) -> None:
    """Require the exact final monotonic A1.3 successor registry boundary."""

    successor = {
        key: row
        for key, row in registry.items()
        if row.get("a1_slice") == a1_2.MONOTONIC_SUCCESSOR_SLICE
    }
    require(
        len(successor) == 68
        and all(
            row.get("stability") == "stable"
            and row.get("runtime_disposition") == "Typed"
            and row.get("typed_status") == "Implemented"
            and row.get("typed_schema_status") == "Complete"
            and row.get("runtime_target") not in {"", "std::monostate{}"}
            and isinstance(row.get("schema_completeness"), Mapping)
            and bool(row["schema_completeness"])
            and all(
                value is True
                for value in row["schema_completeness"].values()
            )
            for row in successor.values()
        ),
        "live registry is not the exact complete 68-identity A1.3 successor",
        "ClosureSuccessorProgressMismatch",
    )
    live_global = status_counts(registry.values())
    require(
        live_global == EXPECTED_A1_3_SUCCESSOR_GLOBAL_STATUS,
        f"A1.3 successor global schema metrics changed: {live_global}",
        "ClosureSuccessorProgressMismatch",
    )
    live_residual = {
        key
        for key, row in registry.items()
        if row.get("typed_schema_status") == "Partial"
    }
    require(
        live_residual == EXPECTED_A1_3_SUCCESSOR_RESIDUAL_PARTIAL_KEYS,
        "A1.3 successor residual Partial identity set changed",
        "ClosureSuccessorProgressMismatch",
    )


def validate_successor_fixture_evidence(
    fixture_index: Mapping[str, Any],
    fixture_coverage: Mapping[str, Any],
    schema_completeness: Mapping[str, Any],
) -> None:
    """Reject fixture changes outside the reviewed final A1.3 corpus."""

    fixture_totals = normalized_fixture_totals(fixture_index)
    fixture_mutations = fixture_index.get("mutation_counts")
    coverage_counts = fixture_coverage.get("counts")
    schema_counts = schema_completeness.get("counts")
    require(
        fixture_totals == EXPECTED_A1_3_SUCCESSOR_FIXTURE_TOTALS
        and fixture_mutations == EXPECTED_A1_3_SUCCESSOR_FIXTURE_MUTATIONS
        and coverage_counts
        == EXPECTED_A1_3_SUCCESSOR_FIXTURE_COVERAGE_COUNTS
        and schema_counts
        == EXPECTED_A1_3_SUCCESSOR_SCHEMA_COMPLETENESS_COUNTS,
        "fixture/mutation/completeness evidence is not the reviewed "
        "A1.3 successor corpus",
        "ClosureSuccessorFixtureMismatch",
    )


def validate_successor_boundaries(
    source_trees: Mapping[str, Mapping[str, Any]],
    frontend_protocol: Mapping[str, str],
) -> None:
    """Allow only the reviewed A1.3 compatibility-only backend delta."""

    require(
        source_trees == EXPECTED_A1_3_SUCCESSOR_TREE_FINGERPRINTS
        and frontend_protocol == EXPECTED_FRONTEND_PROTOCOL_FINGERPRINTS,
        "A1.3 successor BackendCore/frontend/application boundary "
        "fingerprints changed",
        "ClosureBoundaryFingerprintMismatch",
    )


def build_report(arguments: argparse.Namespace) -> dict[str, Any]:
    repo_root = arguments.repo_root
    plan = shared.load_json(arguments.plan)
    closure = shared.load_json(arguments.type_closure)
    fixture_coverage = shared.load_json(arguments.fixture_coverage)
    fixture_index = shared.load_json(arguments.fixture_index)
    schema_completeness = shared.load_json(arguments.schema_completeness)
    a1_2.validate_generated_reports(plan, closure)
    rebuilt_plan, rebuilt_closure = a1_2.build_reports(arguments)
    require(
        rebuilt_plan == plan and rebuilt_closure == closure,
        "live immutable A1.2 authorities do not reproduce the reviewed "
        "plan/type closure",
        "ClosureHistoricalAuthorityMismatch",
    )

    registry_rows = surface.parse_registry_data(arguments.registry)
    registry = indexed(registry_rows, Key.from_row, "registry")
    validate_successor_registry(registry)
    a1_2_registry = {
        key: row
        for key, row in registry.items()
        if row.get("a1_slice") == a1_2.A1_2_SLICE
    }

    ratchet_value = plan.get("final_exact_a1_2_ratchet")
    require(
        isinstance(ratchet_value, list),
        "A1.2 plan lacks its exact final ratchet",
        "ClosureIdentityMismatch",
    )
    ratchet_keys = [key_from_object(row) for row in ratchet_value]
    require(
        len(ratchet_keys) == len(set(ratchet_keys)) == 45,
        "A1.2 final ratchet is not exactly 45 unique identities",
        "ClosureIdentityMismatch",
    )
    require(
        set(ratchet_keys) == set(a1_2_registry),
        "A1.2 final ratchet and canonical registry differ",
        "ClosureIdentityMismatch",
    )

    identity_value = plan.get("identities")
    require(
        isinstance(identity_value, list),
        "A1.2 plan lacks identity records",
        "ClosureIdentityMismatch",
    )
    plan_identities = indexed(
        (
            row
            for row in identity_value
            if isinstance(row, Mapping)
            and isinstance(row.get("protocol_surface_key"), Mapping)
        ),
        lambda row: key_from_object(row["protocol_surface_key"]),
        "plan",
    )
    require(
        set(plan_identities) == set(ratchet_keys),
        "A1.2 plan identity set and exact ratchet differ",
        "ClosureIdentityMismatch",
    )

    complete_identities: list[dict[str, Any]] = []
    for key in ratchet_keys:
        row = registry[key]
        completeness = row.get("schema_completeness")
        require(
            row["stability"] == "stable"
            and row["runtime_disposition"] == "Typed"
            and row["typed_status"] == "Implemented"
            and row["typed_schema_status"] == "Complete"
            and row["runtime_target"] not in {"", "std::monostate{}"}
            and isinstance(completeness, Mapping)
            and bool(completeness)
            and all(value is True for value in completeness.values()),
            f"A1.2 identity is not completely implemented: {key.compact()}",
            "ClosureIdentityIncomplete",
        )
        plan_row = plan_identities[key]
        complete_identities.append(
            {
                "implementation_status": row["typed_status"],
                "owning_implementation_batch":
                    plan_row["owning_implementation_batch"],
                "protocol_surface_key": key.object(),
                "runtime_disposition": row["runtime_disposition"],
                "runtime_target": row["runtime_target"],
                "schema_completeness": completeness,
                "schema_status": row["typed_schema_status"],
            }
        )

    a1_2_status = status_counts(a1_2_registry.values())
    global_status = dict(a1_2.EXPECTED_FINAL_GLOBAL_STATUS)
    require(
        a1_2_status == EXPECTED_A1_2_STATUS,
        f"final A1.2 schema metrics changed: {a1_2_status}",
        "ClosureSchemaStatusMismatch",
    )
    taxonomy = dict(sorted(Counter(key.category for key in ratchet_keys).items()))
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
                str(a1_2_registry[key]["typed_module"])
                for key in ratchet_keys
            ).items()
        )
    )
    result_kinds = dict(
        sorted(
            Counter(
                str(plan_identities[key]["request_contract"]["result_contract_kind"])
                for key in ratchet_keys
                if key.category == "client_request"
            ).items()
        )
    )
    require(
        taxonomy == a1_2.EXPECTED_TAXONOMY,
        f"A1.2 taxonomy changed: {taxonomy}",
        "ClosureTaxonomyMismatch",
    )
    require(
        classifications == a1_2.EXPECTED_CLASSIFICATIONS,
        f"A1.2 classification counts changed: {classifications}",
        "ClosureClassificationMismatch",
    )
    require(
        modules == a1_2.EXPECTED_MODULES,
        f"A1.2 module counts changed: {modules}",
        "ClosureModuleMismatch",
    )
    require(
        result_kinds == a1_2.EXPECTED_RESULT_KINDS,
        f"A1.2 result-contract counts changed: {result_kinds}",
        "ClosureResultKindMismatch",
    )

    operations: list[dict[str, Any]] = []
    notifications: list[dict[str, Any]] = []
    tagged_unions: list[dict[str, Any]] = []
    server_requests: list[dict[str, Any]] = []
    for key in ratchet_keys:
        plan_row = plan_identities[key]
        if key.category == "client_request":
            contract = plan_row["request_contract"]
            operations.append(
                {
                    "api_method": contract["proposed_public_method_name"],
                    "params_type": contract["authoritative_params_type"],
                    "public_result_type":
                        contract["authoritative_successful_result_type"],
                    "read_only_integration_permitted":
                        contract["safe_for_real_read_only_integration"],
                    "result_kind": contract["result_contract_kind"],
                    "result_schema_type":
                        contract["authoritative_result_schema_type"],
                    "unit_params_omitted": contract["unit_params_omitted"],
                    "wire_method": key.name,
                }
            )
        elif key.category == "server_notification":
            public_type = plan_row["required_public_type_or_facade"]
            notifications.append(
                {
                    "backend_core_handling":
                        plan_row["expected_backend_core_handling"],
                    "payload_type":
                        public_type["canonical_payload_aggregate"],
                    "protocol_surface_key": key.object(),
                    "wire_method": key.name,
                }
            )
        elif key.category == "server_request":
            contract = plan_row["server_request_contract"]
            server_requests.append(
                {
                    "compatibility":
                        plan_row["source_compatibility_consideration"],
                    "existing_compatibility_overload":
                        contract["existing_compatibility_overload"],
                    "occurrence_token_owner": contract["occurrence_token_owner"],
                    "params_type": contract["authoritative_params_type"],
                    "permitted_test_strategy":
                        contract["permitted_test_strategy"],
                    "protocol_surface_key": key.object(),
                    "response_type": contract["authoritative_response_type"],
                    "sensitive_fields": contract["sensitive_fields"],
                }
            )
        elif key.category == "tagged_union_discriminator":
            public_type = plan_row["required_public_type_or_facade"]
            tagged_unions.append(
                {
                    "alternative": key.name,
                    "canonical_public_alternative":
                        public_type["canonical_alternative"],
                    "discriminator_field": key.field,
                    "future_unknown_alternative":
                        public_type["future_unknown_alternative"],
                    "union": key.domain,
                    "unknown_alternative_direction":
                        public_type["unknown_alternative_direction"],
                }
            )
    operations.sort(key=lambda row: str(row["wire_method"]))
    notifications.sort(key=lambda row: str(row["wire_method"]))
    tagged_unions.sort(
        key=lambda row: (str(row["union"]), str(row["alternative"]))
    )
    require(
        {row["wire_method"] for row in operations} == a1_2.CLIENT_REQUESTS
        and {
            (row["api_method"].rsplit("::", 1)[0], row["api_method"].rsplit("::", 1)[1])
            for row in operations
        } == set(a1_2.OPERATION_API_NAMES.values()),
        "A1.2 operation/API map changed",
        "ClosureOperationSetMismatch",
    )
    require(
        {row["wire_method"] for row in notifications}
        == a1_2.SERVER_NOTIFICATIONS,
        "A1.2 notification set changed",
        "ClosureNotificationSetMismatch",
    )
    require(
        len(server_requests) == 1
        and server_requests[0]["protocol_surface_key"]["name"]
        == a1_2.AUTH_REFRESH_METHOD,
        "A1.2 server-request set changed",
        "ClosureServerRequestMismatch",
    )
    require(
        {
            (str(row["union"]), str(row["alternative"]))
            for row in tagged_unions
        }
        == set(a1_2.UNION_ALTERNATIVES),
        "A1.2 tagged-union alternative set changed",
        "ClosureUnionSetMismatch",
    )
    union_family_counts = dict(
        sorted(Counter(str(row["union"]) for row in tagged_unions).items())
    )

    response_roots = {
        "client_successful_results": [
            {
                "result_kind": row["result_kind"],
                "result_schema_type": row["result_schema_type"],
                "wire_method": row["wire_method"],
            }
            for row in operations
        ],
        "server_request_responses": [
            {
                "response_type": server_requests[0]["response_type"],
                "wire_method": a1_2.AUTH_REFRESH_METHOD,
            }
        ],
    }
    require(
        len(response_roots["client_successful_results"]) == 18
        and len(response_roots["server_request_responses"]) == 1,
        "response roots were conflated with registry identities",
        "ClosureResponseObligationMismatch",
    )

    staged: list[dict[str, Any]] = []
    stages_value = plan.get("staged_exact_a1_2_ratchets")
    require(
        isinstance(stages_value, list),
        "A1.2 plan lacks staged exact ratchets",
        "ClosureBatchRatchetMismatch",
    )
    previous: set[Key] = set()
    for stage in stages_value:
        batch = str(stage["batch"])
        owned = [key_from_object(row) for row in stage["owned_identities"]]
        cumulative = [
            key_from_object(row) for row in stage["cumulative_identities"]
        ]
        require(
            batch in a1_2.EXPECTED_BATCH_IDENTITY_COUNTS
            and len(owned) == a1_2.EXPECTED_BATCH_IDENTITY_COUNTS[batch]
            and set(cumulative) == previous | set(owned)
            and stage["derived_global_schema_status"]
            == a1_2.EXPECTED_STAGED_GLOBAL_STATUS[batch]
            and all(
                registry[key]["typed_schema_status"] == "Complete"
                for key in cumulative
            ),
            f"staged exact A1.2 ratchet changed at {batch}",
            "ClosureBatchRatchetMismatch",
        )
        staged.append(
            {
                "batch": batch,
                "complete_cumulative_identities":
                    [key.object() for key in cumulative],
                "complete_cumulative_identity_count": len(cumulative),
                "complete_owned_identities": [key.object() for key in owned],
                "complete_owned_identity_count": len(owned),
                "derived_global_schema_status":
                    stage["derived_global_schema_status"],
            }
        )
        previous = set(cumulative)
    require(
        previous == set(ratchet_keys),
        "B5 is not the exact final A1.2 set",
        "ClosureBatchRatchetMismatch",
    )

    residual_value = plan.get("expected_final_residual_partial_identities")
    require(
        isinstance(residual_value, list),
        "plan lacks its reviewed A1.2 residual Partial identity set",
        "ClosureResidualPartialMismatch",
    )
    residual_keys = {
        key_from_object(row)
        for row in residual_value
        if isinstance(row, Mapping)
    }
    require(
        residual_keys == a1_2.EXPECTED_FINAL_RESIDUAL_PARTIAL_KEYS,
        "reviewed A1.2 residual Partial identity set changed",
        "ClosureResidualPartialMismatch",
    )
    residual = [key.object() for key in sorted(residual_keys)]
    require(
        residual == residual_value,
        "plan residual Partial identity ordering changed",
        "ClosureResidualPartialMismatch",
    )

    closure_counts = closure.get("counts")
    require(
        isinstance(closure_counts, Mapping)
        and closure_counts.get("reachable_named_definitions") == 104
        and closure_counts.get("schema_path_dispositions") == 302
        and closure_counts.get("property_declarations") == 272
        and closure_counts.get("definition_dispositions")
        == a1_2.EXPECTED_DEFINITION_DISPOSITIONS
        and closure_counts.get("schema_path_disposition_kinds")
        == a1_2.EXPECTED_SCHEMA_PATH_DISPOSITIONS,
        "A1.2 type/path closure counts changed",
        "ClosureTypeClosureMismatch",
    )
    validate_successor_fixture_evidence(
        fixture_index,
        fixture_coverage,
        schema_completeness,
    )
    fixture_totals = EXPECTED_FIXTURE_TOTALS
    fixture_mutations = EXPECTED_FIXTURE_MUTATIONS
    coverage_counts = EXPECTED_FIXTURE_COVERAGE_COUNTS
    schema_counts = EXPECTED_SCHEMA_COMPLETENESS_COUNTS

    live_source_trees = {
        relative: tree_fingerprint(repo_root, relative)
        for relative in EXPECTED_TREE_FINGERPRINTS
    }
    frontend_protocol = {
        relative: shared.sha256_file(repo_root / relative)
        for relative in EXPECTED_FRONTEND_PROTOCOL_FINGERPRINTS
    }
    validate_successor_boundaries(
        live_source_trees,
        frontend_protocol,
    )
    source_trees = EXPECTED_TREE_FINGERPRINTS

    source_paths = {
        "fixture_coverage": arguments.fixture_coverage,
        "fixture_index": arguments.fixture_index,
        "generator": Path(__file__).resolve(),
        "implementation_plan": arguments.plan,
        "registry": arguments.registry,
        "schema_completeness": arguments.schema_completeness,
        "type_closure": arguments.type_closure,
    }
    return {
        "authority": {
            "production_authority":
                "ai::openai::codex::detail::ProtocolSurfaceRegistry",
            "report_role":
                "non-authoritative deterministic review and closure evidence",
            "response_root_rule":
                "18 client successful-result roots and one server-request "
                "response root are obligations, not registry identities",
        },
        "codex_version": CODEX_VERSION,
        "counts": {
            "a1_2_schema_status": a1_2_status,
            "classifications": classifications,
            "fixture_corpus": fixture_totals,
            "fixture_coverage": coverage_counts,
            "fixture_mutations": fixture_mutations,
            "global_schema_status": global_status,
            "modules": modules,
            "response_root_obligations_not_identities":
                EXPECTED_RESPONSE_ROOT_OBLIGATIONS,
            "result_contract_kinds": result_kinds,
            "schema_completeness": schema_counts,
            "taxonomy": taxonomy,
            "type_closure": closure_counts,
        },
        "exact_complete_a1_2_identities": complete_identities,
        "exact_residual_partial_identities": residual,
        "format_version": FORMAT_VERSION,
        "frontend_backend_application_nonchange": {
            "frontend_protocol_v1": frontend_protocol,
            "source_trees": source_trees,
        },
        "generated_notice": (
            "Generated by tools/codex/app_server_a1_2_closure.py; do not edit."
        ),
        "notifications": notifications,
        "operations": operations,
        "response_root_obligations": response_roots,
        "sensitivity": {
            "authorized_protocol_defined_opaque_json_paths":
                closure["authorized_protocol_defined_opaque_json_paths"],
            "classifications":
                closure_counts["sensitivity_classifications"],
            "policy": closure["sensitivity_policy"],
        },
        "server_request": server_requests[0],
        "sources": source_records(source_paths, repo_root),
        "staged_exact_complete_ratchets": staged,
        "tagged_union_alternatives": tagged_unions,
        "tagged_union_family_counts": union_family_counts,
    }


def report_diagnostics(
    actual: Mapping[str, Any], expected: Mapping[str, Any]
) -> list[AuditDiagnostic]:
    """Compare logical report sections with stable, exact diagnostic codes."""

    diagnostics: list[AuditDiagnostic] = []

    def compare(code: str, location: str, key: str) -> None:
        if actual.get(key) != expected.get(key):
            diagnostics.append(
                AuditDiagnostic(code, location, "closure evidence changed")
            )

    compare("ClosureAuthorityMismatch", "$.authority", "authority")
    actual_counts = actual.get("counts")
    expected_counts = expected.get("counts")
    if not isinstance(actual_counts, Mapping):
        actual_counts = {}
    if not isinstance(expected_counts, Mapping):
        expected_counts = {}
    for field, code in (
        ("a1_2_schema_status", "ClosureSchemaStatusMismatch"),
        ("classifications", "ClosureClassificationMismatch"),
        ("fixture_corpus", "ClosureFixtureCountMismatch"),
        ("fixture_coverage", "ClosureFixtureCountMismatch"),
        ("fixture_mutations", "ClosureFixtureCountMismatch"),
        ("global_schema_status", "ClosureGlobalStatusMismatch"),
        ("modules", "ClosureModuleMismatch"),
        (
            "response_root_obligations_not_identities",
            "ClosureResponseObligationMismatch",
        ),
        ("result_contract_kinds", "ClosureResultKindMismatch"),
        ("schema_completeness", "ClosureFixtureCountMismatch"),
        ("taxonomy", "ClosureTaxonomyMismatch"),
        ("type_closure", "ClosureTypeClosureMismatch"),
    ):
        if actual_counts.get(field) != expected_counts.get(field):
            diagnostics.append(
                AuditDiagnostic(
                    code, f"$.counts.{field}", "closure count changed"
                )
            )
    compare(
        "ClosureIdentityMismatch",
        "$.exact_complete_a1_2_identities",
        "exact_complete_a1_2_identities",
    )
    compare(
        "ClosureResidualPartialMismatch",
        "$.exact_residual_partial_identities",
        "exact_residual_partial_identities",
    )
    compare(
        "ClosureBoundaryFingerprintMismatch",
        "$.frontend_backend_application_nonchange",
        "frontend_backend_application_nonchange",
    )
    compare("ClosureNotificationSetMismatch", "$.notifications", "notifications")
    compare("ClosureOperationSetMismatch", "$.operations", "operations")
    compare(
        "ClosureResponseObligationMismatch",
        "$.response_root_obligations",
        "response_root_obligations",
    )
    compare("ClosureSensitivityMismatch", "$.sensitivity", "sensitivity")
    compare("ClosureServerRequestMismatch", "$.server_request", "server_request")
    compare("ClosureSourceMismatch", "$.sources", "sources")
    compare(
        "ClosureBatchRatchetMismatch",
        "$.staged_exact_complete_ratchets",
        "staged_exact_complete_ratchets",
    )
    compare(
        "ClosureUnionSetMismatch",
        "$.tagged_union_alternatives",
        "tagged_union_alternatives",
    )
    compare(
        "ClosureUnionSetMismatch",
        "$.tagged_union_family_counts",
        "tagged_union_family_counts",
    )
    for field in ("codex_version", "format_version", "generated_notice"):
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
        artifact_label="generated A1.2 closure report",
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
        default=(
            repo_root / "tools/codex/app-server-surface/0.144.6.json"
        ),
    )
    result.add_argument(
        "--schema-root",
        type=Path,
        default=(
            repo_root / "tools/codex/app-server-schema/0.144.6"
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
        default=evidence / "a1-2-start-state.json",
    )
    result.add_argument(
        "--plan",
        type=Path,
        default=evidence / "a1-2-implementation-plan.json",
    )
    result.add_argument(
        "--type-closure",
        type=Path,
        default=evidence / "a1-2-type-closure.json",
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
        "--output",
        type=Path,
        default=evidence / "a1-2-closure-report.json",
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
            f"app-server-a1-2-closure: error [{code}]: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
