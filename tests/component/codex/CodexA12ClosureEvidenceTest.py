#!/usr/bin/env python3
"""Verify the deterministic final A1.2 closure evidence and its guards."""

from __future__ import annotations

import argparse
import copy
import importlib.util
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from types import ModuleType
from typing import Any, Callable

sys.dont_write_bytecode = True


def load_tool(path: Path) -> ModuleType:
    sys.path.insert(0, str(path.parent))
    specification = importlib.util.spec_from_file_location(
        "app_server_a1_2_closure_under_test", path
    )
    if specification is None or specification.loader is None:
        raise AssertionError(f"unable to import closure tool: {path}")
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


def tool_arguments(tool: ModuleType) -> argparse.Namespace:
    arguments = tool.parser().parse_args(
        [
            "check",
            "--repo-root",
            str(OPTIONS.repo_root),
            "--output",
            str(OPTIONS.report),
        ]
    )
    for name, value in vars(arguments).items():
        if isinstance(value, Path):
            setattr(arguments, name, value.resolve())
    return arguments


class CodexA12ClosureEvidenceTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.tool = load_tool(OPTIONS.tool)
        cls.expected = cls.tool.build_report(tool_arguments(cls.tool))

    def test_checked_in_report_is_current(self) -> None:
        completed = subprocess.run(
            [
                sys.executable,
                str(OPTIONS.tool),
                "check",
                "--repo-root",
                str(OPTIONS.repo_root),
                "--output",
                str(OPTIONS.report),
            ],
            check=False,
            capture_output=True,
            text=True,
        )
        self.assertEqual(
            0,
            completed.returncode,
            f"closure check failed by code/path only: "
            f"stderr-lines={len(completed.stderr.splitlines())}",
        )
        committed = json.loads(OPTIONS.report.read_text(encoding="utf-8"))
        self.tool.validate_report(committed, self.expected)

    def test_exact_final_closure_metrics(self) -> None:
        counts = self.expected["counts"]
        self.assertEqual(
            {
                "Complete": 45,
                "NotApplicable": 0,
                "NotImplemented": 0,
                "Partial": 0,
            },
            counts["a1_2_schema_status"],
        )
        self.assertEqual(
            {
                "Complete": 212,
                "NotApplicable": 48,
                "NotImplemented": 121,
                "Partial": 6,
            },
            counts["global_schema_status"],
        )
        self.assertEqual(
            {
                "client_request": 18,
                "server_notification": 7,
                "server_request": 1,
                "tagged_union_discriminator": 19,
            },
            counts["taxonomy"],
        )
        self.assertEqual(
            {
                "client_successful_results": 18,
                "server_request_responses": 1,
                "total": 19,
            },
            counts["response_root_obligations_not_identities"],
        )
        self.assertEqual(
            45, len(self.expected["exact_complete_a1_2_identities"])
        )
        self.assertEqual(
            6, len(self.expected["exact_residual_partial_identities"])
        )
        self.assertEqual(18, len(self.expected["operations"]))
        self.assertEqual(7, len(self.expected["notifications"]))
        self.assertEqual(19, len(self.expected["tagged_union_alternatives"]))
        self.assertEqual(
            104, counts["type_closure"]["reachable_named_definitions"]
        )
        self.assertEqual(
            302, counts["type_closure"]["schema_path_dispositions"]
        )
        self.assertEqual(
            272, counts["type_closure"]["property_declarations"]
        )

    def test_logical_mutations_have_exact_diagnostic_codes(self) -> None:
        Mutation = Callable[[dict[str, Any]], None]

        def set_path(*path: str) -> Mutation:
            def mutate(report: dict[str, Any]) -> None:
                current: dict[str, Any] = report
                for component in path[:-1]:
                    current = current[component]
                current[path[-1]] = {"mutated": True}

            return mutate

        def remove_complete_identity(report: dict[str, Any]) -> None:
            report["exact_complete_a1_2_identities"].pop()

        cases: tuple[tuple[str, Mutation, tuple[str, ...]], ...] = (
            (
                "authority",
                set_path("authority"),
                ("ClosureAuthorityMismatch",),
            ),
            (
                "taxonomy",
                set_path("counts", "taxonomy"),
                ("ClosureTaxonomyMismatch",),
            ),
            (
                "response roots",
                set_path(
                    "counts", "response_root_obligations_not_identities"
                ),
                ("ClosureResponseObligationMismatch",),
            ),
            (
                "classification",
                set_path("counts", "classifications"),
                ("ClosureClassificationMismatch",),
            ),
            (
                "module",
                set_path("counts", "modules"),
                ("ClosureModuleMismatch",),
            ),
            (
                "result kind",
                set_path("counts", "result_contract_kinds"),
                ("ClosureResultKindMismatch",),
            ),
            (
                "slice status",
                set_path("counts", "a1_2_schema_status"),
                ("ClosureSchemaStatusMismatch",),
            ),
            (
                "global status",
                set_path("counts", "global_schema_status"),
                ("ClosureGlobalStatusMismatch",),
            ),
            (
                "type closure",
                set_path("counts", "type_closure"),
                ("ClosureTypeClosureMismatch",),
            ),
            (
                "fixture totals",
                set_path("counts", "fixture_corpus"),
                ("ClosureFixtureCountMismatch",),
            ),
            (
                "identity",
                remove_complete_identity,
                ("ClosureIdentityMismatch",),
            ),
            (
                "residual partial",
                set_path("exact_residual_partial_identities"),
                ("ClosureResidualPartialMismatch",),
            ),
            (
                "operation",
                set_path("operations"),
                ("ClosureOperationSetMismatch",),
            ),
            (
                "notification",
                set_path("notifications"),
                ("ClosureNotificationSetMismatch",),
            ),
            (
                "server request",
                set_path("server_request"),
                ("ClosureServerRequestMismatch",),
            ),
            (
                "tagged union",
                set_path("tagged_union_alternatives"),
                ("ClosureUnionSetMismatch",),
            ),
            (
                "batch ratchet",
                set_path("staged_exact_complete_ratchets"),
                ("ClosureBatchRatchetMismatch",),
            ),
            (
                "boundary",
                set_path("frontend_backend_application_nonchange"),
                ("ClosureBoundaryFingerprintMismatch",),
            ),
            (
                "sensitivity",
                set_path("sensitivity"),
                ("ClosureSensitivityMismatch",),
            ),
            (
                "source",
                set_path("sources"),
                ("ClosureSourceMismatch",),
            ),
        )
        for name, mutate, expected_codes in cases:
            with self.subTest(case=name):
                changed = copy.deepcopy(self.expected)
                mutate(changed)
                diagnostics = self.tool.report_diagnostics(
                    changed, self.expected
                )
                self.assertEqual(
                    expected_codes,
                    tuple(diagnostic.code for diagnostic in diagnostics),
                )
                with self.assertRaises(self.tool.AuditError) as caught:
                    self.tool.validate_report(changed, self.expected)
                self.assertEqual(expected_codes, caught.exception.codes)

    def test_stale_generated_report_has_exact_diagnostic_code(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "a1-2-closure-report.json"
            output.write_text("{}\n", encoding="utf-8")
            with self.assertRaises(self.tool.AuditError) as caught:
                self.tool.write_or_check(output, self.expected, True)
            self.assertEqual(("StaleGeneratedAudit",), caught.exception.codes)

    def test_report_contains_no_fixture_payloads(self) -> None:
        rendered = self.tool.shared.canonical_json(self.expected)
        self.assertNotIn('"fixtures"', rendered)
        self.assertNotIn('"current_fixture_ids"', rendered)
        self.assertNotIn('"payload"', rendered)


def parse_options() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tool", required=True, type=Path)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--report", required=True, type=Path)
    return parser.parse_args()


if __name__ == "__main__":
    OPTIONS = parse_options()
    OPTIONS.tool = OPTIONS.tool.resolve()
    OPTIONS.repo_root = OPTIONS.repo_root.resolve()
    OPTIONS.report = OPTIONS.report.resolve()
    unittest.main(argv=[sys.argv[0]])
