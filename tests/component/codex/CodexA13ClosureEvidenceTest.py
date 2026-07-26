#!/usr/bin/env python3
"""Verify the deterministic final A1.3 closure evidence and its guards."""

from __future__ import annotations

import argparse
import copy
import hashlib
import importlib.util
import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from types import ModuleType
from typing import Any, Callable
from unittest import mock

sys.dont_write_bytecode = True


def load_tool(path: Path) -> ModuleType:
    sys.path.insert(0, str(path.parent))
    specification = importlib.util.spec_from_file_location(
        "app_server_a1_3_closure_under_test", path
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


class CodexA13ClosureEvidenceTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.tool = load_tool(OPTIONS.tool)
        cls.arguments = tool_arguments(cls.tool)
        cls.expected = cls.tool.build_report(cls.arguments)

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
            "closure check failed by code/path only: "
            f"stderr-lines={len(completed.stderr.splitlines())}",
        )
        committed = json.loads(OPTIONS.report.read_text(encoding="utf-8"))
        self.tool.validate_report(committed, self.expected)

    def test_exact_final_closure_metrics(self) -> None:
        counts = self.expected["counts"]
        self.assertEqual(
            {
                "Complete": 68,
                "NotApplicable": 0,
                "NotImplemented": 0,
                "Partial": 0,
            },
            counts["a1_3_schema_status"],
        )
        self.assertEqual(
            {
                "Complete": 280,
                "NotApplicable": 48,
                "NotImplemented": 55,
                "Partial": 4,
            },
            counts["global_schema_status"],
        )
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
            {
                "client_successful_results": 17,
                "concrete_client_results": 8,
                "server_request_responses": 5,
                "total": 22,
                "unit_client_results": 9,
            },
            counts["response_root_obligations_not_identities"],
        )
        self.assertEqual(
            {"Concrete": 8, "Unit": 9},
            counts["result_contract_kinds"],
        )
        self.assertEqual(
            {
                "negative": 647,
                "positive": 396,
                "total": 1043,
            },
            counts["fixture_corpus"]["a1_3"],
        )
        self.assertEqual(68, len(self.expected["exact_complete_a1_3_identities"]))
        self.assertEqual(4, len(self.expected["exact_residual_partial_identities"]))
        self.assertEqual(17, len(self.expected["operations"]))
        self.assertEqual(7, len(self.expected["notifications"]))
        self.assertEqual(5, len(self.expected["server_requests"]))
        self.assertEqual(39, len(self.expected["union_alternatives"]))
        self.assertEqual(123, counts["type_closure"]["reachable_named_definitions"])
        self.assertEqual(480, counts["type_closure"]["schema_paths"])

    def test_exact_remaining_partial_and_batch_ratchets(self) -> None:
        names = {
            row["name"]
            for row in self.expected["exact_residual_partial_identities"]
        }
        self.assertEqual(
            {
                "error",
                "initialize",
                "initialized",
                "item/tool/requestUserInput",
            },
            names,
        )
        ratchets = self.expected["staged_exact_complete_ratchets"]
        self.assertEqual(["B2", "B3", "B4", "B5"], [row["batch"] for row in ratchets])
        self.assertEqual(
            [5, 18, 53, 68],
            [row["complete_cumulative_identity_count"] for row in ratchets],
        )

    def test_direct_response_path_and_concurrent_lifecycle(self) -> None:
        transport = self.expected["transport_and_lifecycle"]
        self.assertTrue(transport["direct_raw_protocol_response"])
        self.assertFalse(transport["server_request_resolved_dependency"])
        self.assertEqual(5, transport["server_request_types_concurrently_pending"])
        self.assertTrue(transport["out_of_order_response_correlation"])
        self.assertTrue(transport["cross_delivery_prevented"])

    def test_public_api_package_security_and_abi_evidence(self) -> None:
        api = self.expected["public_api"]
        self.assertEqual(
            self.tool.a1_3.EXPECTED_INSTALLED_HEADERS,
            api["installed_typed_headers"],
        )
        integrity = self.expected["package_and_test_integrity"]
        self.assertEqual(
            {
                "closure_source_scope_scan": {
                    "finding_count": 0,
                    "scopes": ["src", "tests", "tools", "docs"],
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
            integrity["secret_guard"],
        )
        abi = self.expected["api_abi"]
        capture = abi["capture"]
        self.assertFalse(capture["conclusion"]["binary_compatible"])
        self.assertTrue(
            capture["conclusion"]["installed_consumers_must_rebuild"]
        )
        self.assertEqual(1, capture["conclusion"]["soversion"])
        self.assertEqual(
            list(self.tool.EXPECTED_ABI_LAYOUTS["base"]),
            capture["layout_probe"]["base"]["stdout_lines"],
        )
        self.assertEqual(
            list(self.tool.EXPECTED_ABI_LAYOUTS["final"]),
            capture["layout_probe"]["final"]["stdout_lines"],
        )

    def test_registry_identity_status_target_and_unrelated_guards(self) -> None:
        rows = self.tool.surface.parse_registry_data(self.arguments.registry)
        start_state = self.tool.shared.load_json(self.arguments.start_state)
        a1_3_rows = [
            row
            for row in rows
            if row["a1_slice"] == self.tool.a1_3.A1_3_SLICE
        ]
        victim = a1_3_rows[0]

        missing = [
            row
            for row in rows
            if self.tool.Key.from_row(row) != self.tool.Key.from_row(victim)
        ]
        with self.assertRaises(self.tool.AuditError) as missing_error:
            self.tool.validate_registry(missing, start_state)
        self.assertEqual(("ClosureIdentityMismatch",), missing_error.exception.codes)

        extra = copy.deepcopy(rows)
        unrelated = next(
            row for row in extra if row["a1_slice"] != self.tool.a1_3.A1_3_SLICE
        )
        unrelated["a1_slice"] = self.tool.a1_3.A1_3_SLICE
        unrelated["typed_module"] = self.tool.a1_3.MODULE
        with self.assertRaises(self.tool.AuditError) as extra_error:
            self.tool.validate_registry(extra, start_state)
        self.assertEqual(("ClosureIdentityMismatch",), extra_error.exception.codes)

        incomplete = copy.deepcopy(rows)
        incomplete_row = next(
            row
            for row in incomplete
            if self.tool.Key.from_row(row) == self.tool.Key.from_row(victim)
        )
        incomplete_row["typed_schema_status"] = "Partial"
        with self.assertRaises(self.tool.AuditError) as incomplete_error:
            self.tool.validate_registry(incomplete, start_state)
        self.assertEqual(
            ("ClosureIdentityIncomplete",),
            incomplete_error.exception.codes,
        )

        duplicate_target = copy.deepcopy(rows)
        targets = [
            row
            for row in duplicate_target
            if row["a1_slice"] == self.tool.a1_3.A1_3_SLICE
        ]
        targets[0]["runtime_target"] = targets[1]["runtime_target"]
        with self.assertRaises(self.tool.AuditError) as target_error:
            self.tool.validate_registry(duplicate_target, start_state)
        self.assertEqual(
            ("ClosureRegistryTargetMismatch",),
            target_error.exception.codes,
        )

        unrelated_promotion = copy.deepcopy(rows)
        promoted = next(
            row
            for row in unrelated_promotion
            if row["a1_slice"] != self.tool.a1_3.A1_3_SLICE
            and row["typed_schema_status"] == "NotImplemented"
        )
        promoted["typed_schema_status"] = "Complete"
        with self.assertRaises(self.tool.AuditError) as promotion_error:
            self.tool.validate_registry(unrelated_promotion, start_state)
        self.assertEqual(
            ("ClosureUnrelatedPromotion",),
            promotion_error.exception.codes,
        )

    def test_fixture_response_and_union_mutations_are_rejected(self) -> None:
        fixture_index = self.tool.shared.load_json(self.arguments.fixture_index)
        fixture_coverage = self.tool.shared.load_json(
            self.arguments.fixture_coverage
        )
        schema_completeness = self.tool.shared.load_json(
            self.arguments.schema_completeness
        )
        plan = self.tool.shared.load_json(self.arguments.plan)
        plan_identities = self.tool.indexed(
            plan["identities"],
            lambda row: self.tool.key_from_object(row["protocol_surface_key"]),
            "A1.3 plan",
        )
        for role in ("server_request_response", "union_branch"):
            with self.subTest(role=role):
                changed = copy.deepcopy(fixture_index)
                record = next(
                    row
                    for row in changed["fixtures"]
                    if row.get("a1_slice") == self.tool.a1_3.A1_3_SLICE
                    and row.get("role") == role
                    and self.tool._positive_fixture(row)
                )
                record["role"] = "mutated_role"
                with self.assertRaises(self.tool.AuditError) as caught:
                    self.tool.validate_fixture_evidence(
                        changed,
                        fixture_coverage,
                        schema_completeness,
                        plan_identities,
                    )
                self.assertEqual(
                    ("ClosureFixtureCoverageMismatch",),
                    caught.exception.codes,
                )

    def test_live_provenance_descriptor_public_and_boundary_guards(self) -> None:
        start_state = self.tool.shared.load_json(self.arguments.start_state)
        with tempfile.TemporaryDirectory() as temporary:
            temporary_root = Path(temporary)
            changed_assignment = temporary_root / "assignment.json"
            changed_assignment.write_bytes(
                self.arguments.assignments.read_bytes() + b"\n"
            )
            changed_arguments = copy.copy(self.arguments)
            changed_arguments.assignments = changed_assignment
            with self.assertRaises(self.tool.AuditError) as provenance_error:
                self.tool.validate_provenance(changed_arguments, start_state)
            self.assertEqual(
                ("ClosureProvenanceMismatch",),
                provenance_error.exception.codes,
            )

            changed_descriptor = temporary_root / "client-descriptors.inc"
            changed_descriptor.write_bytes(
                self.arguments.operation_descriptors.read_bytes() + b"\n"
            )
            changed_arguments = copy.copy(self.arguments)
            changed_arguments.operation_descriptors = changed_descriptor
            manifest = self.tool.shared.load_json(self.arguments.manifest)
            with self.assertRaises(self.tool.AuditError) as descriptor_error:
                self.tool.validate_descriptors(changed_arguments, manifest)
            self.assertEqual(
                ("ClosureDescriptorMismatch",),
                descriptor_error.exception.codes,
            )

        changed_plan = self.tool.shared.load_json(self.arguments.plan)
        changed_plan["public_api"] = {"mutated": True}
        with self.assertRaises(self.tool.AuditError) as public_error:
            self.tool.validate_public_api(self.arguments, changed_plan)
        self.assertEqual(
            ("ClosurePublicApiMismatch",),
            public_error.exception.codes,
        )

        with mock.patch.object(
            self.tool,
            "tree_fingerprint",
            return_value={"file_count": 0, "sha256": "0" * 64},
        ):
            with self.assertRaises(self.tool.AuditError) as boundary_error:
                self.tool.validate_boundaries(self.arguments)
        self.assertEqual(
            ("ClosureBoundaryFingerprintMismatch",),
            boundary_error.exception.codes,
        )

    def test_marker_preserving_acceptance_source_mutation_stales_report(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            temporary_root = Path(temporary)
            for name in self.tool.EXPECTED_ACCEPTANCE_TEST_SOURCES:
                shutil.copy2(
                    self.arguments.component_test_root / name,
                    temporary_root / name,
                )
            changed_source = temporary_root / "CodexA13CommandCodecTest.cpp"
            text = changed_source.read_text(encoding="utf-8")
            self.assertIn('"exitCode"', text)
            changed_source.write_text(
                text.replace('"exitCode"', '"exitCodf"', 1),
                encoding="utf-8",
            )
            changed_arguments = copy.copy(self.arguments)
            changed_arguments.component_test_root = temporary_root
            changed_transport = self.tool.validate_transport_and_wire(
                changed_arguments
            )

        changed = copy.deepcopy(self.expected)
        changed["transport_and_lifecycle"] = changed_transport
        with self.assertRaises(self.tool.AuditError) as caught:
            self.tool.validate_report(changed, self.expected)
        self.assertEqual(
            ("ClosureResponsePathMismatch",),
            caught.exception.codes,
        )

    def test_predecessor_closure_evidence_is_byte_stable(self) -> None:
        evidence = OPTIONS.repo_root / "tools/codex/app-server-evidence/0.144.6"
        expected = {
            "a1-1-closure-report.json":
                "be9e5f028de164b57519a956a0b2615b15faa238d889b7f1391b07c3b473fb41",
            "a1-1-implementation-plan.json":
                "59fbbd1e6c1ffcd5e0064e7ff7e2a6a68673ececac5ccb19d3d46cc57eda7661",
            "a1-1-notification-production-coverage.json":
                "191c9e9152fe6c3a2fd052f542911c510e6643e78bfaf2b293729f9bfe7e62b0",
            "a1-1-operation-production-coverage.json":
                "9e9cc9a58651d39f2e2b5e9ee9cdb712a6338660029a77814988a7e904ef156e",
            "a1-1-start-state.json":
                "1614414ea6320f8ac5eb47ef928c4bc8de587c9f429f86d7f94ffeb437b6c705",
            "a1-1-type-closure.json":
                "187db47c37d094b5f22c27cb0a6c1fc656cd0b3e34c63faf9b81ededce2f8bfc",
            "a1-2-closure-report.json":
                "6324a85d998fdbf6c3601f7e19b90c61cea472fde63b49c676511ce1654494cc",
            "a1-2-implementation-plan.json":
                "ff5ab66b79cd53c614a93a8d5cd1a065c54b8a251c706adeb805c900f43f44c4",
            "a1-2-start-state.json":
                "86dc9319ab212c3bdea0d483a6759dfde7d7eba7d5ab92a440a206d1097ab7b9",
            "a1-2-type-closure.json":
                "2b8bc5de1c56b1ce7b809939a744ec1027a78574adfc9e5e8c1e966a385047cb",
        }
        actual = {
            name: hashlib.sha256((evidence / name).read_bytes()).hexdigest()
            for name in expected
        }
        self.assertEqual(expected, actual)

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
            report["exact_complete_a1_3_identities"].pop()

        cases: tuple[tuple[str, Mutation, tuple[str, ...]], ...] = (
            ("ABI", set_path("api_abi"), ("ClosureAbiEvidenceMismatch",)),
            ("authority", set_path("authority"), ("ClosureAuthorityMismatch",)),
            (
                "boundary",
                set_path("boundaries"),
                ("ClosureBoundaryFingerprintMismatch",),
            ),
            (
                "slice status",
                set_path("counts", "a1_3_schema_status"),
                ("ClosureStatusMismatch",),
            ),
            (
                "taxonomy",
                set_path("counts", "taxonomy"),
                ("ClosureTaxonomyMismatch",),
            ),
            (
                "fixture",
                set_path("counts", "fixture_corpus"),
                ("ClosureFixtureCountMismatch",),
            ),
            (
                "response roots",
                set_path("counts", "response_root_obligations_not_identities"),
                ("ClosureResponseObligationMismatch",),
            ),
            (
                "result kind",
                set_path("counts", "result_contract_kinds"),
                ("ClosureOperationContractMismatch",),
            ),
            (
                "type closure",
                set_path("counts", "type_closure"),
                ("ClosureTypeClosureMismatch",),
            ),
            (
                "descriptor",
                set_path("descriptors"),
                ("ClosureDescriptorMismatch",),
            ),
            ("identity", remove_complete_identity, ("ClosureIdentityMismatch",)),
            (
                "residual",
                set_path("exact_residual_partial_identities"),
                ("ClosureStatusMismatch",),
            ),
            (
                "notification",
                set_path("notifications"),
                ("ClosureNotificationMismatch",),
            ),
            (
                "operation",
                set_path("operations"),
                ("ClosureOperationContractMismatch",),
            ),
            (
                "package",
                set_path("package_and_test_integrity"),
                ("ClosurePackageMismatch",),
            ),
            (
                "public API",
                set_path("public_api"),
                ("ClosurePublicApiMismatch",),
            ),
            (
                "response obligations",
                set_path("response_root_obligations"),
                ("ClosureResponseObligationMismatch",),
            ),
            (
                "server request",
                set_path("server_requests"),
                ("ClosureServerRequestMismatch",),
            ),
            ("source", set_path("sources"), ("ClosureSourceMismatch",)),
            (
                "batch ratchet",
                set_path("staged_exact_complete_ratchets"),
                ("ClosureBatchRatchetMismatch",),
            ),
            (
                "transport",
                set_path("transport_and_lifecycle"),
                ("ClosureResponsePathMismatch",),
            ),
            (
                "union",
                set_path("union_alternatives"),
                ("ClosureUnionMismatch",),
            ),
            (
                "metadata",
                set_path("format_version"),
                ("ClosureMetadataMismatch",),
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

    def test_stale_report_and_payload_leak_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "a1-3-closure-report.json"
            output.write_text("{}\n", encoding="utf-8")
            with self.assertRaises(self.tool.AuditError) as caught:
                self.tool.write_or_check(output, self.expected, True)
            self.assertEqual(("StaleGeneratedAudit",), caught.exception.codes)

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
