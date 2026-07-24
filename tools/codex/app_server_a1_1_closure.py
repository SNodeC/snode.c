#!/usr/bin/env python3

"""Generate the final, offline Codex A1.1 closure report.

This report is review evidence.  It projects the final state of the canonical
production registry and the already-guarded A1.1 audit/fixture artifacts; it is
not a runtime disposition or dispatch authority.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence

sys.dont_write_bytecode = True

import app_server_surface as surface


CODEX_VERSION = "0.144.6"
FORMAT_VERSION = 1
EXPECTED_A1_STATUS = {
    "Complete": 151,
    "NotApplicable": 0,
    "NotImplemented": 0,
    "Partial": 0,
}
EXPECTED_GLOBAL_STATUS = {
    "Complete": 167,
    "NotApplicable": 48,
    "NotImplemented": 164,
    "Partial": 8,
}
EXPECTED_TAXONOMY = {
    "client_request": 22,
    "response_item": 16,
    "server_notification": 37,
    "tagged_union_discriminator": 58,
    "thread_item": 18,
}
EXPECTED_CLASSIFICATIONS = {
    "RootOwnedNestedUnion": 16,
    "SharedCommon": 26,
    "SharedWithinSlice": 16,
    "StableItemRoot": 34,
    "StablePublicRoot": 59,
}
EXPECTED_MODULES = {"Common": 26, "ThreadsTurnsSessions": 125}
EXPECTED_RESULT_KINDS = {"Concrete": 15, "Unit": 7}
EXPECTED_BATCH_COUNTS = {"B2": 26, "B3": 50, "B4": 38, "B5": 37}
EXPECTED_CUMULATIVE_BATCH_COUNTS = {"B2": 26, "B3": 76, "B4": 114, "B5": 151}
EXPECTED_TREE_FINGERPRINTS = {
    "src/ai/openai/codex/frontend": {
        "file_count": 14,
        "sha256": "2f87dfea3a01a2e74b8b9637b156f8535351c880b6c2d92d0723765e484583ff",
    },
    "src/apps/codex-backend": {
        "file_count": 10,
        "sha256": "2e3bebfbd4a3060eb7b39a6dfa2f5590175a4bc7874594f77de6f31a983737b9",
    },
    "src/apps/codex-backend-client": {
        "file_count": 21,
        "sha256": "c96bb51b727fd4f6a8a31034f906476e1e10aa1357ae889b9a664f11c09ea337",
    },
}
EXPECTED_FRONTEND_PROTOCOL_FINGERPRINTS = {
    "docs/ai/openai/codex/frontend-protocol-v1.md":
        "5f53a6219f8dc45a09ec7ddca05f2f1f104ce0c7fee824de98492815fc502017",
    "docs/ai/openai/codex/frontend-protocol-v1.schema.json":
        "a27721164607b79a8b268c3adb035211a6efa82cb4645632b9b9a59302734c04",
}


class ClosureError(RuntimeError):
    """A final A1.1 closure invariant was violated."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ClosureError(message)


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise ClosureError(f"unable to load {path}: {error}") from error
    require(isinstance(value, dict), f"expected an object-valued document: {path}")
    return value


def canonical_json(value: Any) -> str:
    return json.dumps(value, indent=2, sort_keys=True, ensure_ascii=False) + "\n"


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def surface_key(row: Mapping[str, Any]) -> tuple[str, str, str, str]:
    return (
        str(row["category"]),
        str(row["domain"]),
        str(row["discriminator_field"]),
        str(row["name"]),
    )


def key_object(key: tuple[str, str, str, str]) -> dict[str, str]:
    return {
        "category": key[0],
        "domain": key[1],
        "discriminator_field": key[2],
        "name": key[3],
    }


def indexed(
    rows: Iterable[Mapping[str, Any]], description: str
) -> dict[tuple[str, str, str, str], dict[str, Any]]:
    result: dict[tuple[str, str, str, str], dict[str, Any]] = {}
    for raw in rows:
        row = dict(raw)
        key = surface_key(row)
        require(key not in result, f"duplicate {description} identity: {key}")
        result[key] = row
    return result


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


def report_source(path: Path, repo_root: Path) -> dict[str, str]:
    return {
        "path": path.relative_to(repo_root).as_posix(),
        "sha256": sha256_file(path),
    }


def taxonomy(keys: Iterable[tuple[str, str, str, str]]) -> dict[str, int]:
    counts: Counter[str] = Counter()
    for category, domain, _, _ in keys:
        if category == "item_discriminator" and domain == "ThreadItem":
            counts["thread_item"] += 1
        elif category == "item_discriminator" and domain == "ResponseItem":
            counts["response_item"] += 1
        else:
            counts[category] += 1
    return {name: counts[name] for name in EXPECTED_TAXONOMY}


def markdown_section(document: str, start: str, end: str) -> str:
    start_index = document.find(start)
    end_index = document.find(end, start_index + len(start))
    require(start_index >= 0 and end_index > start_index, f"missing Markdown section {start!r}")
    return document[start_index:end_index]


def backtick_bullets(document: str) -> list[str]:
    return re.findall(r"^- `([^`]+)`", document, flags=re.MULTILINE)


def validate_documentation(
    path: Path,
    operations: Sequence[Mapping[str, Any]],
    notifications: Sequence[Mapping[str, str]],
    thread_items: Sequence[str],
    response_items: Sequence[str],
    nested_families: Mapping[str, int],
) -> None:
    document = path.read_text(encoding="utf-8")

    operation_section = markdown_section(
        document, "## Client operations and grouped API", "## Stable server notifications"
    )
    operation_rows: list[tuple[str, str, str, str, str, str]] = []
    for line in operation_section.splitlines():
        if not line.startswith("| `"):
            continue
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) != 6:
            continue
        values = [re.findall(r"`([^`]+)`", cell) for cell in cells[:5]]
        if not all(values):
            continue
        operation_rows.append(
            (
                values[0][0],
                values[1][0],
                values[2][0],
                values[3][0],
                values[4][0],
                cells[5],
            )
        )
    expected_operation_rows = [
        (
            str(row["wire_method"]),
            str(row["api_method"]).removeprefix("typed::"),
            str(row["params_type"]),
            str(row["public_result_type"]),
            str(row["result_schema_type"]),
            str(row["result_kind"]),
        )
        for row in operations
    ]
    require(
        operation_rows == expected_operation_rows,
        "A1.1 documentation operation table is not the exact ordered 22-row API map",
    )
    require(
        len({row[0] for row in operation_rows}) == 22,
        "A1.1 documentation operation table contains a duplicate wire method",
    )

    notification_section = markdown_section(
        document, "## Stable server notifications", "## Item roots"
    )
    documented_notifications = backtick_bullets(notification_section)
    expected_notifications = [row["name"] for row in notifications]
    require(
        documented_notifications == expected_notifications,
        "A1.1 documentation notification list is not the exact ordered 37-method set",
    )
    require(
        len(set(documented_notifications)) == 37,
        "A1.1 documentation notification list contains a duplicate method",
    )

    item_section = markdown_section(document, "## Item roots", "## Nested tagged unions")
    response_marker = "`ResponseItem` is a distinct public variant"
    response_index = item_section.find(response_marker)
    require(response_index >= 0, "A1.1 documentation lacks the ResponseItem boundary")
    require(
        backtick_bullets(item_section[:response_index]) == list(thread_items),
        "A1.1 documentation ThreadItem list is not the exact ordered 18-alternative set",
    )
    require(
        backtick_bullets(item_section[response_index:]) == list(response_items),
        "A1.1 documentation ResponseItem list is not the exact ordered 16-alternative set",
    )

    nested_section = markdown_section(
        document, "## Nested tagged unions", "## Full stable schema-definition closure"
    )
    documented_families: dict[str, int] = {}
    for line in nested_section.splitlines():
        match = re.match(r"^\| `([^`]+)` \| `[^`]+` \| .* \| (\d+) \| B[2345] \|$", line)
        if match:
            documented_families[match.group(1)] = int(match.group(2))
    require(
        documented_families == dict(nested_families),
        "A1.1 documentation nested-union family table is not the exact 58-alternative set",
    )

    required_closure_statements = (
        "A1.1 Complete:          151",
        "A1.1 Partial:             0",
        "A1.1 NotImplemented:      0",
        "Complete | 167 |",
        "Partial | 8 |",
        "Not implemented | 164 |",
        "Not applicable | 48 |",
        "22 result roots remain implementation obligations, not registry identities",
        "No A1.1 runtime implementation is deferred to Commit 6",
    )
    normalized_document = " ".join(document.split())
    for statement in required_closure_statements:
        normalized_statement = " ".join(statement.split())
        require(
            normalized_statement in normalized_document,
            f"A1.1 closure documentation lacks {statement!r}",
        )


def build_report(arguments: argparse.Namespace) -> dict[str, Any]:
    repo_root = arguments.repo_root
    plan = load_json(arguments.plan)
    closure = load_json(arguments.type_closure)
    fixture_coverage = load_json(arguments.fixture_coverage)
    fixture_index = load_json(arguments.fixture_index)
    schema_completeness = load_json(arguments.schema_completeness)
    operation_coverage = load_json(arguments.operation_coverage)
    notification_coverage = load_json(arguments.notification_coverage)
    registry_rows = surface.parse_registry_data(arguments.registry)
    registry = indexed(registry_rows, "registry")

    ratchet_rows = plan.get("final_exact_a1_1_ratchet")
    require(isinstance(ratchet_rows, list), "A1.1 plan lacks its exact final ratchet")
    ratchet_keys = [surface_key(row) for row in ratchet_rows]
    require(
        len(ratchet_keys) == len(set(ratchet_keys)) == 151,
        "A1.1 final ratchet is not 151 unique identities",
    )
    require(
        set(ratchet_keys)
        == {key for key, row in registry.items() if row.get("a1_slice") == "A1.1"},
        "A1.1 final ratchet and canonical registry key set differ",
    )

    plan_identities_value = plan.get("identities")
    require(isinstance(plan_identities_value, list), "A1.1 plan lacks identity records")
    plan_identities = indexed(
        (
            {
                **row["protocol_surface_key"],
                "_record": row,
            }
            for row in plan_identities_value
            if isinstance(row, dict) and isinstance(row.get("protocol_surface_key"), dict)
        ),
        "plan",
    )
    require(set(plan_identities) == set(ratchet_keys), "A1.1 plan identity set changed")

    complete_identities: list[dict[str, Any]] = []
    for key in ratchet_keys:
        row = registry[key]
        require(row["stability"] == "stable", f"A1.1 identity became unstable: {key}")
        require(row["runtime_disposition"] == "Typed", f"A1.1 identity is not Typed: {key}")
        require(row["typed_status"] == "Implemented", f"A1.1 identity is not implemented: {key}")
        require(row["typed_schema_status"] == "Complete", f"A1.1 identity is not Complete: {key}")
        require(
            row["runtime_target"] not in {"", "std::monostate{}"},
            f"A1.1 identity lacks a production runtime target: {key}",
        )
        completeness = row.get("schema_completeness")
        require(
            isinstance(completeness, dict)
            and completeness
            and all(value is True for value in completeness.values()),
            f"A1.1 identity does not satisfy every completeness predicate: {key}",
        )
        plan_record = plan_identities[key]["_record"]
        complete_identities.append(
            {
                "protocol_surface_key": key_object(key),
                "implementation_status": row["typed_status"],
                "runtime_disposition": row["runtime_disposition"],
                "runtime_target": row["runtime_target"],
                "schema_completeness": completeness,
                "schema_status": row["typed_schema_status"],
                "owning_implementation_batch":
                    plan_record["owning_implementation_batch"],
            }
        )

    a1_status_counter = Counter(registry[key]["typed_schema_status"] for key in ratchet_keys)
    a1_status = {
        name: a1_status_counter[name]
        for name in ("Complete", "NotApplicable", "NotImplemented", "Partial")
    }
    global_counter = Counter(row["typed_schema_status"] for row in registry_rows)
    global_status = {
        name: global_counter[name]
        for name in ("Complete", "NotApplicable", "NotImplemented", "Partial")
    }
    require(a1_status == EXPECTED_A1_STATUS, f"final A1.1 metrics changed: {a1_status}")
    require(global_status == EXPECTED_GLOBAL_STATUS, f"final global metrics changed: {global_status}")

    counts = plan["counts"]
    require(taxonomy(ratchet_keys) == EXPECTED_TAXONOMY, "final A1.1 taxonomy changed")
    require(counts["classifications"] == EXPECTED_CLASSIFICATIONS, "classification counts changed")
    require(counts["modules"] == EXPECTED_MODULES, "module counts changed")
    require(counts["result_contract_kinds"] == EXPECTED_RESULT_KINDS, "result-kind counts changed")
    require(
        counts["successful_result_root_obligations_not_identities"] == 22,
        "successful result roots were incorrectly added to the identity denominator",
    )

    operations: list[dict[str, Any]] = []
    notifications: list[dict[str, str]] = []
    thread_items: list[str] = []
    response_items: list[str] = []
    nested_families: dict[str, int] = dict(
        sorted(
            Counter(
                key[1]
                for key in ratchet_keys
                if key[0] == "tagged_union_discriminator"
            ).items()
        )
    )
    for key in ratchet_keys:
        plan_record = plan_identities[key]["_record"]
        if key[0] == "client_request":
            contract = plan_record["request_contract"]
            operations.append(
                {
                    "api_method": contract["proposed_public_method_name"],
                    "params_type": contract["authoritative_params_type"],
                    "public_result_type":
                        contract["authoritative_successful_result_type"],
                    "result_kind": contract["result_contract_kind"],
                    "result_schema_type":
                        contract["authoritative_result_schema_type"],
                    "wire_method": key[3],
                }
            )
        elif key[0] == "server_notification":
            notifications.append({"name": key[3], "protocol_surface_key": key_object(key)})
        elif key[:2] == ("item_discriminator", "ThreadItem"):
            thread_items.append(key[3])
        elif key[:2] == ("item_discriminator", "ResponseItem"):
            response_items.append(key[3])
    operations.sort(key=lambda row: str(row["wire_method"]))
    notifications.sort(key=lambda row: row["name"])
    thread_items.sort()
    response_items.sort()
    require(len(operations) == 22, "final operation map is not exactly 22 rows")
    require(len(notifications) == 37, "final notification set is not exactly 37 rows")
    require(len(thread_items) == 18, "final ThreadItem set is not exactly 18 alternatives")
    require(len(response_items) == 16, "final ResponseItem set is not exactly 16 alternatives")
    require(sum(nested_families.values()) == 58, "final nested-union set is not exactly 58 alternatives")

    staged: list[dict[str, Any]] = []
    plan_stages = plan.get("staged_exact_a1_1_ratchets")
    require(isinstance(plan_stages, list), "A1.1 plan lacks staged exact ratchets")
    previous: set[tuple[str, str, str, str]] = set()
    for stage in plan_stages:
        require(isinstance(stage, dict), "malformed staged A1.1 ratchet")
        batch = str(stage["batch"])
        owned = [surface_key(row) for row in stage["owned_identities"]]
        cumulative = [surface_key(row) for row in stage["cumulative_identities"]]
        require(len(owned) == EXPECTED_BATCH_COUNTS[batch], f"{batch} owned count changed")
        require(
            len(cumulative) == EXPECTED_CUMULATIVE_BATCH_COUNTS[batch],
            f"{batch} cumulative count changed",
        )
        require(set(cumulative) == previous | set(owned), f"{batch} is not cumulative")
        require(
            all(registry[key]["typed_schema_status"] == "Complete" for key in cumulative),
            f"{batch} cumulative ratchet regressed",
        )
        staged.append(
            {
                "batch": batch,
                "complete_owned_identities": [key_object(key) for key in owned],
                "complete_owned_identity_count": len(owned),
                "complete_cumulative_identities": [key_object(key) for key in cumulative],
                "complete_cumulative_identity_count": len(cumulative),
            }
        )
        previous = set(cumulative)
    require(previous == set(ratchet_keys), "B5 cumulative ratchet is not the exact A1.1 set")

    residual = [
        key_object(key)
        for key, row in sorted(registry.items())
        if row["typed_schema_status"] == "Partial"
    ]
    require(
        residual == plan["expected_residual_partial_identities"],
        "the exact eight residual partial identities changed",
    )

    closure_counts = closure.get("counts")
    require(isinstance(closure_counts, dict), "type closure lacks count evidence")
    require(closure_counts["reachable_named_definitions"] == 164, "type closure changed")
    require(closure_counts["schema_path_dispositions"] == 698, "schema-path closure changed")
    require(
        sum(closure_counts["definition_dispositions"].values()) == 164,
        "definition dispositions no longer close exactly",
    )
    require(
        sum(closure_counts["schema_path_disposition_kinds"].values()) == 698,
        "schema-path dispositions no longer close exactly",
    )

    fixture_counts = fixture_index.get("counts")
    coverage_counts = fixture_coverage.get("counts")
    schema_counts = schema_completeness.get("counts")
    operation_counts = operation_coverage.get("counts")
    notification_counts = notification_coverage.get("counts")
    for value, name in (
        (fixture_counts, "fixture index"),
        (coverage_counts, "fixture coverage"),
        (schema_counts, "schema completeness"),
        (operation_counts, "operation production coverage"),
        (notification_counts, "notification production coverage"),
    ):
        require(isinstance(value, dict), f"{name} lacks count evidence")
    require(fixture_counts["total"] == 3714, "fixture corpus total changed")
    require(
        fixture_counts["positive"] == coverage_counts["positive_fixtures"] == 1415,
        "positive fixture totals disagree",
    )
    require(fixture_counts["negative"] == 2299, "negative fixture total changed")
    require(operation_counts["operations"] == 22, "operation coverage count changed")
    require(
        operation_counts["unit_result_roots"] == 7
        and operation_counts["concrete_result_roots"] == 15,
        "operation result-kind coverage changed",
    )
    require(
        notification_counts["a1_1_server_notifications"] == 37,
        "notification production coverage changed",
    )

    fingerprints = {
        relative: tree_fingerprint(repo_root, relative)
        for relative in EXPECTED_TREE_FINGERPRINTS
    }
    require(
        fingerprints == EXPECTED_TREE_FINGERPRINTS,
        f"frontend/application source boundary changed: {fingerprints}",
    )
    frontend_protocol_fingerprints = {
        relative: sha256_file(repo_root / relative)
        for relative in EXPECTED_FRONTEND_PROTOCOL_FINGERPRINTS
    }
    require(
        frontend_protocol_fingerprints == EXPECTED_FRONTEND_PROTOCOL_FINGERPRINTS,
        "Frontend Protocol v1 documentation/schema byte identity changed",
    )

    validate_documentation(
        arguments.documentation,
        operations,
        notifications,
        thread_items,
        response_items,
        nested_families,
    )

    source_paths = {
        "documentation": arguments.documentation,
        "fixture_coverage": arguments.fixture_coverage,
        "fixture_index": arguments.fixture_index,
        "generator": Path(__file__).resolve(),
        "notification_production_coverage": arguments.notification_coverage,
        "operation_production_coverage": arguments.operation_coverage,
        "registry": arguments.registry,
        "schema_completeness": arguments.schema_completeness,
        "type_closure": arguments.type_closure,
        "implementation_plan": arguments.plan,
    }
    return {
        "authority_note": (
            "Final review evidence only. Every runtime/status value is projected "
            "from the sole canonical ProtocolSurfaceRegistry; this report is not "
            "a production registry or dispatch input."
        ),
        "codex_version": CODEX_VERSION,
        "counts": {
            "a1_1_schema_status": a1_status,
            "classifications": EXPECTED_CLASSIFICATIONS,
            "fixture_corpus": fixture_counts,
            "fixture_coverage": coverage_counts,
            "global_schema_status": global_status,
            "modules": EXPECTED_MODULES,
            "result_contract_kinds": EXPECTED_RESULT_KINDS,
            "schema_completeness": schema_counts,
            "successful_result_root_obligations_not_identities": 22,
            "taxonomy": EXPECTED_TAXONOMY,
            "type_closure": closure_counts,
        },
        "exact_complete_a1_1_identities": complete_identities,
        "exact_residual_partial_identities": residual,
        "format_version": FORMAT_VERSION,
        "frontend_application_nonchange": {
            "frontend_protocol_v1": frontend_protocol_fingerprints,
            "source_trees": fingerprints,
        },
        "generated_notice": (
            "Generated by tools/codex/app_server_a1_1_closure.py; do not edit."
        ),
        "nested_union_family_counts": nested_families,
        "notifications": notifications,
        "operations": operations,
        "response_item_alternatives": response_items,
        "sources": {
            name: report_source(path, repo_root)
            for name, path in sorted(source_paths.items())
        },
        "staged_exact_complete_ratchets": staged,
        "thread_item_alternatives": thread_items,
    }


def write_or_check(path: Path, report: Mapping[str, Any], check: bool) -> None:
    rendered = canonical_json(report)
    if check:
        try:
            committed = path.read_text(encoding="utf-8")
        except OSError as error:
            raise ClosureError(f"missing final A1.1 closure report {path}: {error}") from error
        require(committed == rendered, f"final A1.1 closure report is stale: {path}")
    else:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(rendered, encoding="utf-8")


def parser() -> argparse.ArgumentParser:
    repo_root = Path(__file__).resolve().parents[2]
    evidence = repo_root / "tools/codex/app-server-evidence/0.144.6"
    result = argparse.ArgumentParser(description=__doc__)
    result.add_argument("mode", choices=("generate", "check"))
    result.add_argument("--repo-root", type=Path, default=repo_root)
    result.add_argument(
        "--plan", type=Path, default=evidence / "a1-1-implementation-plan.json"
    )
    result.add_argument(
        "--type-closure", type=Path, default=evidence / "a1-1-type-closure.json"
    )
    result.add_argument(
        "--fixture-coverage", type=Path, default=evidence / "fixture-coverage.json"
    )
    result.add_argument(
        "--fixture-index",
        type=Path,
        default=repo_root / "tools/codex/app-server-fixtures/0.144.6/index.json",
    )
    result.add_argument(
        "--schema-completeness",
        type=Path,
        default=evidence / "schema-completeness-evidence.json",
    )
    result.add_argument(
        "--operation-coverage",
        type=Path,
        default=evidence / "a1-1-operation-production-coverage.json",
    )
    result.add_argument(
        "--notification-coverage",
        type=Path,
        default=evidence / "a1-1-notification-production-coverage.json",
    )
    result.add_argument(
        "--registry",
        type=Path,
        default=repo_root / "src/ai/openai/codex/detail/ProtocolSurfaceRegistryData.inc",
    )
    result.add_argument(
        "--documentation",
        type=Path,
        default=repo_root / "docs/ai/openai/codex/a1-1-conversation-domain.md",
    )
    result.add_argument(
        "--output", type=Path, default=evidence / "a1-1-closure-report.json"
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
    except ClosureError as error:
        print(f"app-server-a1-1-closure: error: {error}", file=sys.stderr)
        raise SystemExit(1)
