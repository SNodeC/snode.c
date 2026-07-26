#!/usr/bin/env python3

"""Determinism and mutation guards for the frozen A1.4 audit."""

from __future__ import annotations

import argparse
import copy
import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path
from types import ModuleType
from typing import Callable

sys.dont_write_bytecode = True


def load_tool(path: Path) -> ModuleType:
    sys.path.insert(0, str(path.resolve().parent))
    specification = importlib.util.spec_from_file_location(
        "snodec_codex_a1_4_audit", path
    )
    if specification is None or specification.loader is None:
        raise AssertionError(f"unable to import A1.4 audit tool: {path}")
    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


class CodexA14AuditToolTest(unittest.TestCase):
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
                "--partition-output",
                str(OPTIONS.partition),
                "--closure-output",
                str(OPTIONS.closure),
            ]
        )
        for name, value in vars(cls.arguments).items():
            if isinstance(value, Path):
                setattr(cls.arguments, name, value.resolve())
        cls.inputs = cls.tool.load_inputs(cls.arguments)
        cls.partition, cls.closure = cls.tool.build_foundation(
            cls.arguments
        )

    def assert_report_mutation(
        self,
        mutate: Callable[[dict[str, object], dict[str, object]], None],
        expected_code: str,
    ) -> None:
        partition = copy.deepcopy(self.partition)
        closure = copy.deepcopy(self.closure)
        mutate(partition, closure)
        diagnostics = self.tool.foundation_diagnostics(
            partition, closure
        )
        self.assertIn(
            expected_code,
            {row.code for row in diagnostics},
        )
        with self.assertRaises(self.tool.AuditError):
            self.tool.validate_foundation_reports(partition, closure)

    def assert_input_mutation(
        self,
        mutate: Callable[[object], None],
        expected_code: str,
        *,
        arithmetic: bool = False,
        contracts: bool = False,
    ) -> None:
        inputs = copy.deepcopy(self.inputs)
        mutate(inputs)
        with self.assertRaises(self.tool.AuditError) as raised:
            keys = self.tool.validate_partition_inputs(inputs)
            if arithmetic:
                self.tool.validate_start_arithmetic(inputs, keys)
            if contracts:
                self.tool._operation_rows(inputs, keys)
        self.assertIn(expected_code, raised.exception.codes)

    def test_checked_reports_are_current_and_deterministic(self) -> None:
        second_partition, second_closure = (
            self.tool.build_foundation(self.arguments)
        )
        self.assertEqual(self.partition, second_partition)
        self.assertEqual(self.closure, second_closure)
        self.assertEqual(
            self.partition,
            json.loads(OPTIONS.partition.read_text(encoding="utf-8")),
        )
        self.assertEqual(
            self.closure,
            json.loads(OPTIONS.closure.read_text(encoding="utf-8")),
        )
        self.tool.validate_foundation_reports(
            self.partition, self.closure
        )

    def test_total_partition_and_stability_are_exact(self) -> None:
        counts = self.partition["counts"]
        self.assertEqual(387, counts["total_inventory"])
        self.assertEqual(
            {
                "A1.0": 19,
                "A1.1": 151,
                "A1.2": 45,
                "A1.3": 68,
                "A1.4": 56,
                "InventoryOnly": 48,
            },
            counts["partition"],
        )
        self.assertEqual(
            {
                "experimental_only_inventory": 36,
                "stable_a1": 339,
                "stable_inventory": 351,
                "stable_unreachable_inventory": 12,
                "total_inventory": 387,
            },
            counts["stability_accounting"],
        )
        self.assertEqual(
            {
                "ExperimentalInventoryOnly": 36,
                "StableUnreachableInventory": 12,
            },
            counts["inventory_only_classifications"],
        )
        all_rows = [
            row
            for rows in self.partition["buckets"].values()
            for row in rows
        ]
        all_keys = [
            self.tool.Key.from_row(row["protocol_surface_key"])
            for row in all_rows
        ]
        self.assertEqual(387, len(all_keys))
        self.assertEqual(387, len(set(all_keys)))
        self.assertTrue(
            all(
                row["typed_schema_status"] == "NotApplicable"
                for row in self.partition["buckets"][
                    "InventoryOnly"
                ]
            )
        )

    def test_native_inventory_contracts_and_status_are_exact(self) -> None:
        counts = self.partition["counts"]
        self.assertEqual(
            {
                "client_request": 29,
                "server_notification": 16,
                "server_request": 4,
                "tagged_union_discriminator": 7,
            },
            counts["native_a1_4_taxonomy"],
        )
        self.assertEqual(
            {"Concrete": 26, "Unit": 3},
            counts["native_a1_4_result_kinds"],
        )
        self.assertEqual(
            {
                "Complete": 0,
                "NotApplicable": 0,
                "NotImplemented": 55,
                "Partial": 1,
            },
            counts["native_a1_4_start_status"],
        )
        rows = self.partition["buckets"]["A1.4"]
        self.assertEqual(56, len(rows))
        self.assertTrue(
            all(
                row["stability"] == "stable"
                and row["module"] == "IntegrationsAndLongTail"
                and row["a1_slice"] == "A1.4"
                for row in rows
            )
        )
        operations = self.partition["native_a1_4_operations"]
        self.assertEqual(33, len(operations))
        units = {
            row["protocol_surface_key"]["name"]
            for row in operations
            if row["protocol_surface_key"]["category"]
            == "client_request"
            and row["result_kind"] == "Unit"
        }
        self.assertEqual(
            {
                "plugin/share/delete",
                "plugin/uninstall",
                "skills/extraRoots/set",
            },
            units,
        )
        server = [
            row
            for row in operations
            if row["protocol_surface_key"]["category"]
            == "server_request"
        ]
        self.assertEqual(4, len(server))
        self.assertTrue(
            all(
                row["result_kind"] == "Concrete"
                and row["direct_raw_protocol_response"]
                and not row["depends_on_server_request_resolved"]
                for row in server
            )
        )

    def test_current_and_projected_arithmetic_is_exact(self) -> None:
        counts = self.partition["counts"]
        self.assertEqual(
            {
                "Complete": 280,
                "NotApplicable": 48,
                "NotImplemented": 55,
                "Partial": 4,
            },
            counts["global_start_status"],
        )
        self.assertEqual(
            {
                "Complete": 336,
                "NotApplicable": 48,
                "NotImplemented": 0,
                "Partial": 3,
            },
            counts["native_completion_projection"],
        )
        self.assertEqual(
            {
                "Complete": 339,
                "NotApplicable": 48,
                "NotImplemented": 0,
                "Partial": 0,
            },
            counts["final_a1_projection"],
        )
        self.assertEqual(
            "item/tool/requestUserInput",
            self.partition["native_partial_identity"]["name"],
        )
        self.assertEqual(
            {"error", "initialize", "initialized"},
            {
                row["name"]
                for row in self.partition[
                    "inherited_a1_0_partial_identities"
                ]
            },
        )

    def test_inventory_only_and_asymmetries_are_hard_boundaries(
        self,
    ) -> None:
        inventory = self.partition["inventory_only"]
        self.assertEqual(
            36, len(inventory["ExperimentalInventoryOnly"])
        )
        self.assertEqual(
            12, len(inventory["StableUnreachableInventory"])
        )
        self.assertTrue(
            all(
                row["stability"] == "experimental_only"
                for row in inventory["ExperimentalInventoryOnly"]
            )
        )
        self.assertTrue(
            all(
                row["stability"] == "stable"
                for row in inventory["StableUnreachableInventory"]
            )
        )
        asymmetry = self.partition[
            "stable_experimental_asymmetries"
        ]
        self.assertEqual(
            ["process/exited", "process/outputDelta"],
            asymmetry["stable_process_notifications"],
        )
        self.assertEqual(
            [
                "process/kill",
                "process/resizePty",
                "process/spawn",
                "process/writeStdin",
            ],
            asymmetry["experimental_process_controls"],
        )
        self.assertEqual(
            "remoteControl/status/changed",
            asymmetry["stable_remote_control_notification"],
        )
        self.assertEqual(
            sorted(self.tool.EXPERIMENTAL_REMOTE_CONTROLS),
            asymmetry["experimental_remote_control_controls"],
        )

    def test_full_transitive_schema_closure_is_frozen(self) -> None:
        self.assertEqual(
            {
                "default_bearing_paths": 22,
                "definition_namespaces": {
                    "legacy": 34,
                    "v2": 155,
                },
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
            },
            self.closure["counts"],
        )
        paths = self.closure["schema_paths"]
        self.assertEqual(646, len(paths))
        self.assertTrue(
            all(
                {
                    "required",
                    "optional",
                    "nullable",
                    "default_present",
                    "value_kind",
                    "integer_format",
                    "minimum",
                    "maximum",
                    "array_element_type",
                    "map_value_type",
                    "additional_properties",
                    "intentionally_opaque_json",
                }.issubset(row)
                for row in paths
            )
        )

    def test_union_representation_and_reachability_are_exact(self) -> None:
        unions = {
            row["domain"]: row
            for row in self.closure["union_families"]
        }
        self.assertEqual(
            {"McpServerElicitationRequestParams", "PluginSource"},
            set(unions),
        )
        self.assertEqual(
            {"form", "openai/form", "url"},
            {
                row["alternative"]
                for row in unions[
                    "McpServerElicitationRequestParams"
                ]["alternatives"]
            },
        )
        self.assertEqual(
            {"git", "local", "npm", "remote"},
            {
                row["alternative"]
                for row in unions["PluginSource"]["alternatives"]
            },
        )
        self.assertEqual(
            {"mcpServer/elicitation/request"},
            {
                row["name"]
                for row in unions[
                    "McpServerElicitationRequestParams"
                ]["reaching_roots"]
            },
        )
        self.assertEqual(
            {
                "plugin/installed",
                "plugin/list",
                "plugin/read",
                "plugin/share/list",
            },
            {
                row["name"]
                for row in unions["PluginSource"]["reaching_roots"]
            },
        )
        self.assertTrue(
            all(
                row["future_unknown_handling"]
                and row["malformed_known_handling"]
                for row in unions.values()
            )
        )

    def test_provenance_and_audit_scope_are_frozen(self) -> None:
        start = json.loads(
            OPTIONS.start_state.read_text(encoding="utf-8")
        )
        provenance = start["provenance"]
        self.assertEqual(
            "5d1fbf26c43abc65a203928b2e31561cb039e06d",
            provenance["source_commit_sha"],
        )
        self.assertEqual(
            "cee1ac3bcaf95e5fcdcf07499c7e6b00fc423b90c670ea3380f1799434b72add",
            provenance["schema_trees"]["stable"][
                "aggregate_sha256"
            ],
        )
        boundaries = self.partition["audit_boundaries"]
        self.assertEqual([], boundaries["production_paths_changed"])
        self.assertFalse(boundaries["registry_status_promoted"])
        self.assertFalse(boundaries["soversion_changed"])
        self.assertTrue(
            all(
                not path.startswith("src/")
                for path in boundaries["changed_paths"]
            )
        )

    def test_duplicate_unassigned_and_overlap_input_guards(self) -> None:
        rows = copy.deepcopy(
            self.inputs.assignments_document["assignments"]
        )
        duplicate = copy.deepcopy(rows[0])
        with self.assertRaises(self.tool.AuditError) as raised:
            self.tool._keys_from_rows(
                [*rows, duplicate], "assignment"
            )
        self.assertEqual("DuplicateAssignment", raised.exception.code)

        overlap = copy.deepcopy(rows[0])
        overlap["slice"] = "A1.4"
        with self.assertRaises(self.tool.AuditError) as raised:
            self.tool._keys_from_rows(
                [*rows, overlap], "assignment"
            )
        self.assertEqual("CrossSliceOverlap", raised.exception.code)

        def unassign(inputs: object) -> None:
            inputs.assignments_document["assignments"][0][
                "slice"
            ] = "Unassigned"
            key = self.tool.Key.from_row(
                inputs.assignments_document["assignments"][0]
            )
            inputs.assignments[key]["slice"] = "Unassigned"

        self.assert_input_mutation(
            unassign, "UnassignedIdentity"
        )

    def test_missing_and_extra_inventory_authority_are_rejected(
        self,
    ) -> None:
        manifest = json.loads(
            self.arguments.manifest.read_text(encoding="utf-8")
        )
        mutations = (
            ("missing", lambda rows: rows.pop()),
            (
                "extra",
                lambda rows: rows.append(
                    {
                        **copy.deepcopy(rows[-1]),
                        "id": "client_request:synthetic/extra",
                        "name": "synthetic/extra",
                        "category": "client_request",
                        "domain": "ClientRequest",
                        "discriminator_field": "method",
                    }
                ),
            ),
        )
        for name, mutate in mutations:
            with self.subTest(case=name), tempfile.TemporaryDirectory(
                prefix=f"codex-a1-4-{name}-"
            ) as directory:
                changed = copy.deepcopy(manifest)
                mutate(changed["entries"])
                path = Path(directory) / "manifest.json"
                path.write_text(
                    json.dumps(changed, indent=2) + "\n",
                    encoding="utf-8",
                )
                arguments = copy.copy(self.arguments)
                arguments.manifest = path
                with self.assertRaises(
                    self.tool.AuditError
                ) as raised:
                    self.tool.load_inputs(arguments)
                self.assertEqual(
                    "TotalDenominatorMismatch",
                    raised.exception.code,
                )

    def test_assignment_and_boundary_input_guards(self) -> None:
        a1_4_key = next(
            key
            for key in self.inputs.assignments
            if key.name == "app/list"
        )
        experimental_key = next(
            key
            for key, row in self.inputs.assignments.items()
            if row["classification"]
            == "ExperimentalInventoryOnly"
        )
        unreachable_key = next(
            key
            for key, row in self.inputs.assignments.items()
            if row["classification"]
            == "StableUnreachableInventory"
        )

        def wrong_module(inputs: object) -> None:
            inputs.assignments[a1_4_key]["module"] = "Common"
            inputs.registry[a1_4_key]["typed_module"] = "Common"

        self.assert_input_mutation(
            wrong_module, "A14ModuleMismatch"
        )

        def wrong_stability(inputs: object) -> None:
            inputs.assignments[a1_4_key][
                "stability"
            ] = "experimental_only"

        self.assert_input_mutation(
            wrong_stability, "StabilityMismatch"
        )

        def promote_experimental(inputs: object) -> None:
            inputs.assignments[experimental_key]["slice"] = "A1.4"
            inputs.assignments[experimental_key][
                "module"
            ] = "IntegrationsAndLongTail"
            inputs.registry[experimental_key]["a1_slice"] = "A1.4"
            inputs.registry[experimental_key][
                "typed_module"
            ] = "IntegrationsAndLongTail"

        self.assert_input_mutation(
            promote_experimental, "ExperimentalLeakage"
        )

        def promote_unreachable(inputs: object) -> None:
            inputs.assignments[unreachable_key]["slice"] = "A1.4"
            inputs.assignments[unreachable_key][
                "module"
            ] = "IntegrationsAndLongTail"
            inputs.registry[unreachable_key]["a1_slice"] = "A1.4"
            inputs.registry[unreachable_key][
                "typed_module"
            ] = "IntegrationsAndLongTail"

        self.assert_input_mutation(
            promote_unreachable,
            "StableUnreachableBoundaryMismatch",
        )

        def move_native_to_inventory(inputs: object) -> None:
            inputs.assignments[a1_4_key]["slice"] = "InventoryOnly"
            inputs.registry[a1_4_key][
                "a1_slice"
            ] = "InventoryOnly"

        self.assert_input_mutation(
            move_native_to_inventory, "PartitionCountMismatch"
        )

        for inherited_name in ("initialize", "initialized", "error"):
            inherited_key = next(
                key
                for key in self.inputs.assignments
                if key.name == inherited_name
            )

            def reassign(inputs: object, key=inherited_key) -> None:
                inputs.assignments[key]["slice"] = "A1.4"
                inputs.assignments[key][
                    "module"
                ] = "IntegrationsAndLongTail"
                inputs.registry[key]["a1_slice"] = "A1.4"
                inputs.registry[key][
                    "typed_module"
                ] = "IntegrationsAndLongTail"

            with self.subTest(inherited=inherited_name):
                self.assert_input_mutation(
                    reassign, "CrossSliceOwnershipMismatch"
                )

    def test_contract_and_status_input_guards(self) -> None:
        client_key = next(
            key
            for key in self.inputs.contracts
            if key.name == "app/list"
        )
        server_key = next(
            key
            for key in self.inputs.contracts
            if key.name == "attestation/generate"
        )

        def wrong_client_result(inputs: object) -> None:
            inputs.contracts[client_key][
                "result_type_identity"
            ] = "Unit"
            inputs.registry[client_key][
                "result_type_identity"
            ] = "Unit"

        self.assert_input_mutation(
            wrong_client_result,
            "ClientResultAssociationMismatch",
            contracts=True,
        )

        def wrong_server_response(inputs: object) -> None:
            inputs.contracts[server_key][
                "result_type_identity"
            ] = "Unit"
            inputs.registry[server_key][
                "result_type_identity"
            ] = "Unit"

        self.assert_input_mutation(
            wrong_server_response,
            "ServerResponseContractMismatch",
            contracts=True,
        )

        def promote_registry(inputs: object) -> None:
            key = next(
                value
                for value in inputs.registry
                if value.name == "app/list"
            )
            inputs.registry[key]["typed_schema_status"] = "Complete"

        self.assert_input_mutation(
            promote_registry,
            "NativeStartStatusMismatch",
            arithmetic=True,
        )

    def test_logical_report_mutation_guards(self) -> None:
        cases = (
            (
                "total denominator",
                lambda partition, closure: partition["counts"].__setitem__(
                    "total_inventory", 386
                ),
                "TotalDenominatorMismatch",
            ),
            (
                "partition denominator",
                lambda partition, closure: partition["counts"][
                    "partition"
                ].__setitem__("A1.4", 55),
                "PartitionCountMismatch",
            ),
            (
                "missing identity",
                lambda partition, closure: partition["buckets"][
                    "A1.4"
                ].pop(),
                "PartitionSetMismatch",
            ),
            (
                "cross-slice overlap",
                lambda partition, closure: partition["buckets"][
                    "A1.4"
                ].append(
                    copy.deepcopy(partition["buckets"]["A1.0"][0])
                ),
                "PartitionSetMismatch",
            ),
            (
                "taxonomy",
                lambda partition, closure: partition["counts"].__setitem__(
                    "native_a1_4_taxonomy", {}
                ),
                "TaxonomyMismatch",
            ),
            (
                "module",
                lambda partition, closure: partition["buckets"]["A1.4"][
                    0
                ].__setitem__("module", "Common"),
                "A14ModuleMismatch",
            ),
            (
                "stability",
                lambda partition, closure: partition["buckets"]["A1.4"][
                    0
                ].__setitem__("stability", "experimental_only"),
                "StabilityMismatch",
            ),
            (
                "result kind",
                lambda partition, closure: partition["counts"].__setitem__(
                    "native_a1_4_result_kinds",
                    {"Concrete": 29},
                ),
                "ResultKindMismatch",
            ),
            (
                "client result association",
                lambda partition, closure: next(
                    row
                    for row in partition[
                        "native_a1_4_operations"
                    ]
                    if row["protocol_surface_key"]["name"]
                    == "app/list"
                ).__setitem__("result_type", "Unit"),
                "ClientResultAssociationMismatch",
            ),
            (
                "server response association",
                lambda partition, closure: next(
                    row
                    for row in partition[
                        "native_a1_4_operations"
                    ]
                    if row["protocol_surface_key"]["name"]
                    == "attestation/generate"
                ).__setitem__("result_type", "Unit"),
                "ServerResponseContractMismatch",
            ),
            (
                "resolved response transport",
                lambda partition, closure: next(
                    row
                    for row in partition[
                        "native_a1_4_operations"
                    ]
                    if row["protocol_surface_key"]["name"]
                    == "item/tool/requestUserInput"
                ).__setitem__(
                    "depends_on_server_request_resolved", True
                ),
                "ResponsePathMismatch",
            ),
            (
                "native partial",
                lambda partition, closure: partition[
                    "native_partial_identity"
                ].__setitem__("name", "initialize"),
                "NativePartialMismatch",
            ),
            (
                "missing inherited partial",
                lambda partition, closure: partition[
                    "inherited_a1_0_partial_identities"
                ].pop(),
                "CrossSliceLedgerMismatch",
            ),
            (
                "all InventoryOnly experimental",
                lambda partition, closure: partition["counts"].__setitem__(
                    "inventory_only_classifications",
                    {"ExperimentalInventoryOnly": 48},
                ),
                "InventoryClassificationMismatch",
            ),
            (
                "missing stable unreachable",
                lambda partition, closure: partition["inventory_only"][
                    "StableUnreachableInventory"
                ].pop(),
                "InventoryClassificationMismatch",
            ),
            (
                "native arithmetic",
                lambda partition, closure: partition["counts"].__setitem__(
                    "native_completion_projection", {}
                ),
                "NativeCompletionArithmeticMismatch",
            ),
            (
                "final arithmetic",
                lambda partition, closure: partition["counts"].__setitem__(
                    "final_a1_projection", {}
                ),
                "FinalA1ArithmeticMismatch",
            ),
            (
                "union alternative",
                lambda partition, closure: closure["union_families"][0][
                    "alternatives"
                ].pop(),
                "UnionSchemaMismatch",
            ),
            (
                "schema closure",
                lambda partition, closure: closure["counts"].__setitem__(
                    "schema_paths", 645
                ),
                "SchemaClosureMismatch",
            ),
            (
                "schema path",
                lambda partition, closure: closure[
                    "schema_paths"
                ].pop(),
                "SchemaPathMismatch",
            ),
            (
                "production scope",
                lambda partition, closure: partition[
                    "audit_boundaries"
                ].__setitem__(
                    "production_paths_changed",
                    ["src/ai/openai/codex/AppServerClient.cpp"],
                ),
                "ProductionScopeViolation",
            ),
            (
                "registry promotion",
                lambda partition, closure: partition[
                    "audit_boundaries"
                ].__setitem__("registry_status_promoted", True),
                "RegistryPromotionViolation",
            ),
            (
                "SOVERSION decision",
                lambda partition, closure: partition[
                    "audit_boundaries"
                ].__setitem__("soversion_changed", True),
                "BoundaryMismatch",
            ),
        )
        for name, mutate, code in cases:
            with self.subTest(case=name):
                self.assert_report_mutation(mutate, code)

    def test_named_stable_experimental_mutation_guards(self) -> None:
        names_and_codes = (
            ("process/exited", "A14IdentitySetMismatch"),
            ("process/outputDelta", "A14IdentitySetMismatch"),
            (
                "remoteControl/status/changed",
                "A14IdentitySetMismatch",
            ),
            ("serverRequest/resolved", "A14IdentitySetMismatch"),
            (
                "item/tool/requestUserInput",
                "A14IdentitySetMismatch",
            ),
        )
        for name, code in names_and_codes:
            def remove(
                partition: dict[str, object],
                _closure: dict[str, object],
                identity=name,
            ) -> None:
                rows = partition["buckets"]["A1.4"]
                rows[:] = [
                    row
                    for row in rows
                    if row["protocol_surface_key"]["name"] != identity
                ]

            with self.subTest(identity=name):
                self.assert_report_mutation(remove, code)

        inventory_by_name = {
            row["protocol_surface_key"]["name"]: row
            for row in self.partition["buckets"]["InventoryOnly"]
        }
        for name in (
            *sorted(self.tool.EXPERIMENTAL_PROCESS_CONTROLS),
            *sorted(self.tool.EXPERIMENTAL_REMOTE_CONTROLS),
        ):
            self.assertIn(name, inventory_by_name)

            def include(
                partition: dict[str, object],
                _closure: dict[str, object],
                identity=name,
            ) -> None:
                partition["buckets"]["A1.4"][0] = copy.deepcopy(
                    inventory_by_name[identity]
                )

            with self.subTest(experimental_identity=name):
                self.assert_report_mutation(
                    include, "A14IdentitySetMismatch"
                )

    def test_stale_generated_report_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="codex-a1-4-stale-"
        ) as directory:
            path = Path(directory) / "partition.json"
            stale = copy.deepcopy(self.partition)
            stale["counts"]["total_inventory"] = 386
            path.write_text(
                json.dumps(stale, indent=2, sort_keys=True) + "\n",
                encoding="utf-8",
            )
            with self.assertRaises(self.tool.AuditError):
                self.tool.write_or_check(
                    path, self.partition, True
                )


def parse_options() -> argparse.Namespace:
    root = Path(__file__).resolve().parents[3]
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--tool",
        type=Path,
        default=root / "tools/codex/app_server_a1_4.py",
    )
    parser.add_argument("--repo-root", type=Path, default=root)
    evidence = root / "tools/codex/app-server-evidence/0.144.6"
    parser.add_argument(
        "--start-state",
        type=Path,
        default=evidence / "a1-4-start-state.json",
    )
    parser.add_argument(
        "--partition",
        type=Path,
        default=evidence / "a1-4-total-partition.json",
    )
    parser.add_argument(
        "--closure",
        type=Path,
        default=evidence / "a1-4-type-closure.json",
    )
    return parser.parse_args()


OPTIONS = parse_options()

if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
