#!/usr/bin/env python3
"""Check the reviewed A1.2 Markdown against generated audit evidence."""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import defaultdict
from pathlib import Path
from typing import Any


def load_json(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise AssertionError(f"{path}: expected a JSON object")
    return value


def normalized(value: str) -> str:
    return re.sub(r"\s+", " ", value).strip()


def require_contains(document: str, expected: str) -> None:
    if normalized(expected) not in normalized(document):
        raise AssertionError(
            "documentation lacks generated evidence text: "
            + normalized(expected)
        )


def surface_key(row: dict[str, Any]) -> tuple[str, str, str, str]:
    key = row["protocol_surface_key"]
    return (
        str(key["category"]),
        str(key["domain"]),
        str(key["discriminator_field"]),
        str(key["name"]),
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--documentation", required=True, type=Path)
    parser.add_argument("--start-state", required=True, type=Path)
    parser.add_argument("--plan", required=True, type=Path)
    parser.add_argument("--closure", required=True, type=Path)
    options = parser.parse_args()

    document = options.documentation.read_text(encoding="utf-8")
    start = load_json(options.start_state)
    plan = load_json(options.plan)
    closure = load_json(options.closure)
    rows = [
        row
        for row in plan["identities"]
        if isinstance(row, dict)
    ]
    counts = plan["counts"]
    closure_counts = closure["counts"]

    require_contains(
        document,
        f"The audited starting `origin/master` is "
        f"`{start['base_sha']}`.",
    )
    require_contains(
        document,
        f"The two commits have the same tree "
        f"`{start['prerequisite']['base_tree_sha']}`",
    )

    taxonomy = counts["taxonomy"]
    for label, category in (
        ("Client-request methods", "client_request"),
        ("Server-notification methods", "server_notification"),
        ("Server-request methods", "server_request"),
        ("Tagged-union alternatives", "tagged_union_discriminator"),
    ):
        require_contains(
            document, f"| {label} | {taxonomy[category]} |"
        )
    require_contains(
        document,
        f"| **Total** | **{counts['identity_denominator']}** |",
    )
    for classification, count in sorted(
        counts["classifications"].items()
    ):
        require_contains(
            document, f"| `{classification}` | {count} |"
        )
    for module, count in sorted(counts["modules"].items()):
        require_contains(document, f"| `{module}` | {count} |")

    for boundary, metrics in (
        ("Merged A1.1 base", counts["initial_global_schema_status"]),
        *[
            (
                f"After {batch}",
                counts["staged_global_schema_status"][batch],
            )
            for batch in ("B2", "B3", "B4", "B5")
        ],
    ):
        require_contains(
            document,
            f"| {boundary} | {metrics['Complete']} | "
            f"{metrics['Partial']} | {metrics['NotImplemented']} | "
            f"{metrics['NotApplicable']} |",
        )

    request_rows = sorted(
        (
            row
            for row in rows
            if surface_key(row)[0] == "client_request"
        ),
        key=lambda row: surface_key(row)[3],
    )
    if len(request_rows) != 18:
        raise AssertionError("documentation input is not 18 requests")
    for row in request_rows:
        method = surface_key(row)[3]
        contract = row["request_contract"]
        api = str(contract["proposed_public_method_name"]).removeprefix(
            "typed::"
        )
        params = str(contract["authoritative_params_type"])
        result = str(contract["authoritative_successful_result_type"])
        params_cell = "Unit" if params == "Unit" else f"`{params}`"
        result_cell = "Unit" if result == "Unit" else f"`{result}`"
        require_contains(
            document,
            f"| `{method}` | `{api}` | "
            f"{params_cell} | {result_cell} | "
            f"{contract['result_contract_kind']} |",
        )

    notification_names = sorted(
        surface_key(row)[3]
        for row in rows
        if surface_key(row)[0] == "server_notification"
    )
    if len(notification_names) != 7:
        raise AssertionError("documentation input is not seven notifications")
    for name in notification_names:
        require_contains(document, f"`{name}`")

    server_requests = [
        row
        for row in rows
        if surface_key(row)[0] == "server_request"
    ]
    if len(server_requests) != 1:
        raise AssertionError("documentation input is not one server request")
    require_contains(
        document, f"`{surface_key(server_requests[0])[3]}`"
    )

    union_alternatives: dict[str, list[str]] = defaultdict(list)
    for row in rows:
        category, domain, field, name = surface_key(row)
        if category == "tagged_union_discriminator":
            if field != "type":
                raise AssertionError(
                    f"{domain}/{name}: unexpected discriminator {field}"
                )
            union_alternatives[domain].append(name)
    if sum(map(len, union_alternatives.values())) != 19:
        raise AssertionError("documentation input is not 19 alternatives")
    for domain, alternatives in sorted(union_alternatives.items()):
        require_contains(
            document,
            f"| `{domain}` | `type` | "
            + ", ".join(f"`{name}`" for name in sorted(alternatives))
            + " |",
        )

    require_contains(
        document,
        "Namespace-aware traversal currently derives "
        f"{closure_counts['seed_definitions']} unique "
        f"definition seeds, "
        f"{closure_counts['reachable_named_definitions']} reachable "
        f"named definitions, and "
        f"{closure_counts['schema_path_dispositions']} schema paths.",
    )
    for disposition, count in sorted(
        closure_counts["definition_dispositions"].items()
    ):
        require_contains(
            document, f"| `{disposition}` | {count} |"
        )
    for disposition, count in sorted(
        closure_counts["schema_path_disposition_kinds"].items()
    ):
        require_contains(
            document, f"| `{disposition}` | {count} |"
        )

    sensitivity = closure_counts["sensitivity_classifications"]
    require_contains(
        document,
        "The closure records "
        + ", ".join(
            f"{count} {name}"
            for name, count in (
                ("NonSensitiveProtocolData", sensitivity[
                    "NonSensitiveProtocolData"
                ]),
                ("AuthenticationFlowData", sensitivity[
                    "AuthenticationFlowData"
                ]),
                ("AccountUsageData", sensitivity["AccountUsageData"]),
                (
                    "PersonallyIdentifyingAccountData",
                    sensitivity["PersonallyIdentifyingAccountData"],
                ),
                (
                    "PotentialCredentialBearingConfiguration",
                    sensitivity[
                        "PotentialCredentialBearingConfiguration"
                    ],
                ),
                ("WorkspaceContent", sensitivity["WorkspaceContent"]),
                (
                    "AccountWorkspaceIdentity",
                    sensitivity["AccountWorkspaceIdentity"],
                ),
                ("CredentialSecret", sensitivity["CredentialSecret"]),
            )
        )
        + f", and {sensitivity['EphemeralAuthenticationCode']} "
        "EphemeralAuthenticationCode paths.",
    )

    fixture = counts["fixture_plan"]
    require_contains(
        document,
        f"The Commit 1 fixture plan contains "
        f"{fixture['required_positive_fixture_roles']} required positive "
        "roles:",
    )
    require_contains(
        document,
        f"{fixture['required_field_mutation_paths']} required-field, "
        f"{fixture['wrong_type_mutation_paths']} wrong-type, "
        f"{fixture['optional_omission_paths']} optional-omission, and "
        f"{fixture['nullable_tri_state_paths']} nullable tri-state path "
        "obligations",
    )
    require_contains(
        document,
        "Each identity additionally records "
        f"{fixture['required_property_present_positive_paths']} "
        "required-property-present, "
        f"{fixture['optional_property_present_positive_paths']} "
        "optional-present, "
        f"{fixture['optional_property_omitted_positive_paths']} "
        "optional-omitted, "
        f"{fixture['nullable_positive_state_cases']} schema-valid "
        "nullable-state, "
        f"{fixture['defaulted_positive_paths']} defaulted-path, "
        f"{fixture['integer_boundary_positive_paths']} integer-boundary, "
        f"{fixture['opaque_json_positive_paths']} authorized opaque-JSON, "
        f"{fixture['collection_empty_nonempty_positive_paths']} "
        "empty/non-empty collection, "
        f"{fixture['known_open_enum_positive_values']} known "
        "open-enum-value, and "
        f"{fixture['registered_union_positive_alternatives']} registered "
        "union-alternative obligations.",
    )

    forbidden = (
        "test-api-key-not-real",
        "test-access-token-not-real",
    )
    for sentinel in forbidden:
        if sentinel in document:
            raise AssertionError(
                "documentation contains a forbidden synthetic sentinel"
            )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, KeyError, TypeError, ValueError) as error:
        print(f"Codex A1.2 documentation error: {error}", file=sys.stderr)
        raise SystemExit(1)
