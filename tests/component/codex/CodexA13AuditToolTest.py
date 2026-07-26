#!/usr/bin/env python3

"""Determinism and mutation guards for the frozen A1.3 audit."""

from __future__ import annotations

import argparse
import copy
import importlib.util
import json
import sys
import unittest
from pathlib import Path
from types import ModuleType
from typing import Callable

sys.dont_write_bytecode = True


def load_tool(path: Path) -> ModuleType:
    sys.path.insert(0, str(path.resolve().parent))
    specification = importlib.util.spec_from_file_location(
        "snodec_codex_a1_3_audit", path
    )
    if specification is None or specification.loader is None:
        raise AssertionError(f"unable to import A1.3 audit tool: {path}")
    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


class CodexA13AuditToolTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.tool = load_tool(OPTIONS.tool)
        cls.arguments = cls.tool.parser().parse_args(
            [
                "check",
                "--repo-root",
                str(OPTIONS.repo_root),
                "--start-state",
                str(OPTIONS.start_state),
                "--plan-output",
                str(OPTIONS.plan),
                "--closure-output",
                str(OPTIONS.closure),
            ]
        )
        for name, value in vars(cls.arguments).items():
            if isinstance(value, Path):
                setattr(cls.arguments, name, value.resolve())
        cls.plan, cls.closure = cls.tool.build_reports(cls.arguments)

    def assert_mutation(
        self,
        mutate: Callable[[dict[str, object], dict[str, object]], None],
        expected_code: str,
    ) -> None:
        plan = copy.deepcopy(self.plan)
        closure = copy.deepcopy(self.closure)
        mutate(plan, closure)
        diagnostics = self.tool.report_diagnostics(plan, closure)
        self.assertIn(expected_code, {row.code for row in diagnostics})
        with self.assertRaises(self.tool.AuditError):
            self.tool.validate_generated_reports(plan, closure)

    def test_checked_reports_are_current_and_deterministic(self) -> None:
        second_plan, second_closure = self.tool.build_reports(self.arguments)
        self.assertEqual(self.plan, second_plan)
        self.assertEqual(self.closure, second_closure)
        self.assertEqual(
            self.plan,
            json.loads(OPTIONS.plan.read_text(encoding="utf-8")),
        )
        self.assertEqual(
            self.closure,
            json.loads(OPTIONS.closure.read_text(encoding="utf-8")),
        )
        self.tool.validate_generated_reports(self.plan, self.closure)

    def test_exact_start_inventory_and_contract_arithmetic(self) -> None:
        counts = self.plan["counts"]
        self.assertEqual(68, counts["identity_count"])
        self.assertEqual(
            {
                "client_request": 17,
                "server_notification": 7,
                "server_request": 5,
                "tagged_union_discriminator": 39,
            },
            counts["taxonomy"],
        )
        self.assertEqual(
            {"Concrete": 8, "Unit": 9},
            counts["result_contract_kinds"],
        )
        self.assertEqual(5, counts["server_request_response_contracts"])
        self.assertEqual(
            {"NotImplemented": 66, "Partial": 2},
            counts["initial_a1_3_schema_status"],
        )
        self.assertEqual("B4", counts["current_progress_stage"])
        self.assertEqual(
            {"Complete": 53, "NotImplemented": 15},
            counts["current_a1_3_schema_status"],
        )
        self.assertEqual(
            {
                "Complete": 265,
                "NotApplicable": 48,
                "NotImplemented": 70,
                "Partial": 4,
            },
            counts["current_global_schema_status"],
        )

    def test_full_transitive_schema_closure_is_frozen(self) -> None:
        counts = self.closure["counts"]
        self.assertEqual(59, counts["seed_definitions"])
        self.assertEqual(123, counts["reachable_named_definitions"])
        self.assertEqual({"legacy": 26, "v2": 97}, counts["definition_namespaces"])
        self.assertEqual(480, counts["schema_paths"])
        self.assertEqual(
            {
                "array_element": 34,
                "map_value": 3,
                "property": 443,
            },
            counts["schema_path_kinds"],
        )
        self.assertEqual(346, counts["required_paths"])
        self.assertEqual(134, counts["optional_paths"])
        self.assertEqual(114, counts["nullable_paths"])
        self.assertEqual(20, counts["default_bearing_paths"])

    def test_b4_approval_permission_checkpoint_is_exact(self) -> None:
        b4 = [
            row for row in self.plan["identities"] if row["batch"] == "B4"
        ]
        self.assertEqual(35, len(b4))
        self.assertTrue(
            all(
                row["implementation_status"] == "Implemented"
                and row["schema_status"] == "Complete"
                and row["fixture_ids"]
                for row in b4
            )
        )
        server_requests = [
            row
            for row in self.plan["operations"]
            if row["protocol_surface_key"]["category"] == "server_request"
        ]
        self.assertEqual(5, len(server_requests))
        self.assertTrue(
            all(
                row["result_kind"] == "Concrete"
                and row["direct_raw_protocol_response"]
                and not row["depends_on_server_request_resolved"]
                for row in server_requests
            )
        )
        union_counts = {
            row["domain"]: len(row["alternatives"])
            for row in self.plan["union_families"]
            if row["domain"]
            in {
                "CommandExecutionApprovalDecision",
                "FileChange",
                "FileSystemPath",
                "FileSystemSpecialPath",
                "ParsedCommand",
                "ReviewDecision",
            }
        }
        self.assertEqual(
            {
                "CommandExecutionApprovalDecision": 6,
                "FileChange": 3,
                "FileSystemPath": 3,
                "FileSystemSpecialPath": 6,
                "ParsedCommand": 4,
                "ReviewDecision": 7,
            },
            union_counts,
        )

    def test_server_request_resolved_is_excluded_from_transport(self) -> None:
        self.assertFalse(
            self.plan["response_path"]["server_request_resolved_in_slice"]
        )
        self.assertFalse(
            self.plan["response_path"][
                "server_request_resolved_transport_dependency"
            ]
        )
        self.assertEqual(
            5, self.plan["response_path"]["concurrent_server_request_types"]
        )
        self.assertTrue(
            all(
                not row["depends_on_server_request_resolved"]
                for row in self.plan["operations"]
            )
        )

    def test_logical_mutation_guards(self) -> None:
        cases = (
            (
                "missing identity",
                lambda plan, closure: plan["identities"].pop(),
                "IdentitySetMismatch",
            ),
            (
                "extra identity",
                lambda plan, closure: plan["identities"].append(
                    {
                        "protocol_surface_key": {
                            "category": "server_request",
                            "domain": "ServerRequest",
                            "discriminator_field": "method",
                            "name": "item/tool/requestUserInput",
                        }
                    }
                ),
                "IdentitySetMismatch",
            ),
            (
                "denominator",
                lambda plan, closure: plan["counts"].__setitem__(
                    "identity_count", 67
                ),
                "IdentityCountMismatch",
            ),
            (
                "taxonomy",
                lambda plan, closure: plan["counts"].__setitem__(
                    "taxonomy", {}
                ),
                "TaxonomyMismatch",
            ),
            (
                "result kind",
                lambda plan, closure: plan["counts"].__setitem__(
                    "result_contract_kinds", {"Concrete": 17}
                ),
                "ResultKindMismatch",
            ),
            (
                "missing response",
                lambda plan, closure: plan["operations"].pop(),
                "ContractMismatch",
            ),
            (
                "batch map",
                lambda plan, closure: plan["counts"].__setitem__(
                    "batch_identity_counts", {"B2": 68}
                ),
                "BatchAssignmentMismatch",
            ),
            (
                "response dependency",
                lambda plan, closure: plan["response_path"].__setitem__(
                    "server_request_resolved_transport_dependency", True
                ),
                "ResponsePathMismatch",
            ),
            (
                "command lifecycle merge",
                lambda plan, closure: plan["notifications"][0].__setitem__(
                    "protocol_surface_key",
                    {
                        "category": "server_notification",
                        "domain": "ServerNotification",
                        "discriminator_field": "method",
                        "name": "item/commandExecution/outputDelta",
                    },
                ),
                "ContractMismatch",
            ),
            (
                "union alternative",
                lambda plan, closure: plan["union_families"][0][
                    "alternatives"
                ].pop(),
                "UnionSchemaMismatch",
            ),
            (
                "schema definition",
                lambda plan, closure: closure["counts"].__setitem__(
                    "reachable_named_definitions", 122
                ),
                "SchemaClosureMismatch",
            ),
            (
                "required schema path",
                lambda plan, closure: closure["schema_paths"].pop(),
                "SchemaPathMismatch",
            ),
            (
                "backend expansion",
                lambda plan, closure: plan["boundaries"].__setitem__(
                    "backend_state_expansion", True
                ),
                "BoundaryMismatch",
            ),
        )
        for name, mutate, code in cases:
            with self.subTest(case=name):
                self.assert_mutation(mutate, code)


def parse_options() -> argparse.Namespace:
    root = Path(__file__).resolve().parents[3]
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--tool",
        type=Path,
        default=root / "tools/codex/app_server_a1_3.py",
    )
    parser.add_argument("--repo-root", type=Path, default=root)
    evidence = root / "tools/codex/app-server-evidence/0.144.6"
    parser.add_argument(
        "--start-state",
        type=Path,
        default=evidence / "a1-3-start-state.json",
    )
    parser.add_argument(
        "--plan",
        type=Path,
        default=evidence / "a1-3-implementation-plan.json",
    )
    parser.add_argument(
        "--closure",
        type=Path,
        default=evidence / "a1-3-type-closure.json",
    )
    return parser.parse_args()


OPTIONS = parse_options()

if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
