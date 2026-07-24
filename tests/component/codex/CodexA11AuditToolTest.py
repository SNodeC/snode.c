#!/usr/bin/env python3

"""Offline reproducibility and exact-diagnostic guards for the A1.1 audit."""

from __future__ import annotations

import argparse
import copy
import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path
from types import ModuleType, SimpleNamespace
from typing import Callable

sys.dont_write_bytecode = True


def load_tool(path: Path) -> ModuleType:
    tool_directory = str(path.resolve().parent)
    if tool_directory not in sys.path:
        sys.path.insert(0, tool_directory)
    specification = importlib.util.spec_from_file_location(
        "snodec_codex_a1_1_audit", path
    )
    if specification is None or specification.loader is None:
        raise RuntimeError(f"unable to load A1.1 audit tool: {path}")
    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


def load_json(path: Path) -> dict[str, object]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise AssertionError(f"expected an object-valued JSON document: {path}")
    return value


class CodexA11AuditToolTest(unittest.TestCase):
    tool: ModuleType
    arguments: SimpleNamespace
    plan: dict[str, object]
    closure: dict[str, object]

    @classmethod
    def setUpClass(cls) -> None:
        for name, value in vars(OPTIONS).items():
            if isinstance(value, Path):
                setattr(OPTIONS, name, value.resolve())
        cls.tool = load_tool(OPTIONS.tool)
        cls.arguments = SimpleNamespace(
            repo_root=OPTIONS.repo_root,
            manifest=OPTIONS.manifest,
            registry=OPTIONS.registry,
            schema_root=OPTIONS.schema_root,
            assignments=OPTIONS.assignments,
            reachability=OPTIONS.reachability,
            contracts=OPTIONS.contracts,
            schema_completeness=OPTIONS.schema_completeness,
            fixture_coverage=OPTIONS.fixture_coverage,
            production_coverage=OPTIONS.production_coverage,
            notification_production_coverage=
                OPTIONS.notification_production_coverage,
            fixture_index=OPTIONS.fixture_index,
            draft07_validator=OPTIONS.draft07_validator,
            start_state=OPTIONS.start_state,
            plan_output=OPTIONS.plan_output,
            closure_output=OPTIONS.closure_output,
        )
        cls.plan, cls.closure = cls.tool.build_reports(cls.arguments)

    def assert_report_mutation(
        self,
        mutate: Callable[[dict[str, object], dict[str, object]], None],
        expected_codes: tuple[str, ...],
    ) -> None:
        plan = copy.deepcopy(self.plan)
        closure = copy.deepcopy(self.closure)
        mutate(plan, closure)
        actual = tuple(
            diagnostic.code
            for diagnostic in self.tool.report_diagnostics(plan, closure)
        )
        self.assertEqual(expected_codes, actual)
        with self.assertRaises(self.tool.AuditError) as caught:
            self.tool.validate_generated_reports(plan, closure)
        self.assertEqual(expected_codes, caught.exception.codes)
        self.assertEqual(expected_codes[0], caught.exception.code)

    def test_canonical_reports_are_deterministic_current_and_valid(self) -> None:
        second_plan, second_closure = self.tool.build_reports(self.arguments)
        self.assertEqual(self.plan, second_plan)
        self.assertEqual(self.closure, second_closure)
        self.assertEqual(load_json(OPTIONS.plan_output), self.plan)
        self.assertEqual(load_json(OPTIONS.closure_output), self.closure)
        self.assertEqual([], self.tool.report_diagnostics(self.plan, self.closure))
        self.tool.validate_generated_reports(self.plan, self.closure)
        self.tool.write_or_check(OPTIONS.plan_output, self.plan, True)
        self.tool.write_or_check(OPTIONS.closure_output, self.closure, True)

        counts = self.plan["counts"]
        assert isinstance(counts, dict)
        self.assertEqual(151, counts["identity_denominator"])
        self.assertEqual(
            22, counts["successful_result_root_obligations_not_identities"]
        )
        self.assertEqual(
            {
                "client_request": 22,
                "response_item": 16,
                "server_notification": 37,
                "tagged_union_discriminator": 58,
                "thread_item": 18,
            },
            counts["taxonomy"],
        )
        closure_counts = self.closure["counts"]
        assert isinstance(closure_counts, dict)
        self.assertEqual(164, closure_counts["reachable_named_definitions"])
        self.assertEqual(698, closure_counts["schema_path_dispositions"])
        self.assertEqual(
            {
                "PrivateCodecHelper": 76,
                "ProtocolDefinedOpaqueJson": 14,
                "PublicHandwrittenType": 92,
                "ReusedExistingType": 360,
                "StrongIdentifier": 156,
            },
            closure_counts["schema_path_disposition_kinds"],
        )
        self.assertEqual(
            {
                "optional_nonnullable": 19,
                "optional_nullable": 218,
                "required_nonnullable": 413,
                "required_nullable": 5,
            },
            closure_counts["property_shapes"],
        )

        reviewed_path = (
            "#/definitions/v2/SubAgentSource/oneOf/1/"
            "properties/thread_spawn"
        )
        schema_paths = self.closure["schema_paths"]
        assert isinstance(schema_paths, list)
        reviewed_mapping = next(
            row
            for row in schema_paths
            if isinstance(row, dict)
            and row.get("schema_path") == reviewed_path
        )
        self.assertEqual(
            {
                "cpp_base_type": "typed::ThreadSpawnSubAgentDetails",
                "cpp_mapping": (
                    "typed::ThreadSpawnSubAgentSource::threadSpawn -> "
                    "typed::ThreadSpawnSubAgentDetails"
                ),
                "cpp_member": "threadSpawn",
                "cpp_owner": "typed::ThreadSpawnSubAgentSource",
                "cpp_type": "typed::ThreadSpawnSubAgentDetails",
                "directionality": "DecodeOnly",
                "disposition": "PublicHandwrittenType",
                "nullable": False,
                "presence_model": "RequiredValue",
                "required": True,
                "schema_node_kind": "property",
                "schema_path": reviewed_path,
            },
            reviewed_mapping,
        )
        loaded_data_path = (
            "#/definitions/v2/ThreadLoadedListResponse/properties/data"
        )
        loaded_data_mapping = next(
            row
            for row in schema_paths
            if isinstance(row, dict)
            and row.get("schema_path") == loaded_data_path
        )
        self.assertEqual(
            {
                "cpp_base_type": "std::vector<typed::ThreadId>",
                "cpp_mapping": (
                    "typed::ThreadLoadedListResponse::data -> "
                    "std::vector<typed::ThreadId>"
                ),
                "cpp_member": "data",
                "cpp_owner": "typed::ThreadLoadedListResponse",
                "cpp_type": "std::vector<typed::ThreadId>",
                "directionality": "DecodeOnly",
                "disposition": "StrongIdentifier",
                "nullable": False,
                "presence_model": "RequiredValue",
                "required": True,
                "schema_node_kind": "property",
                "schema_path": loaded_data_path,
                "semantic_identifier": "typed::ThreadId",
                "semantic_identifier_role": "vector_property",
            },
            loaded_data_mapping,
        )
        loaded_item_mapping = next(
            row
            for row in schema_paths
            if isinstance(row, dict)
            and row.get("schema_path") == loaded_data_path + "/items"
        )
        self.assertEqual(
            {
                "cpp_base_type": "typed::ThreadId",
                "cpp_mapping": (
                    "typed::ThreadLoadedListResponse::data[] -> "
                    "typed::ThreadId"
                ),
                "cpp_member": "data[]",
                "cpp_owner": "typed::ThreadLoadedListResponse",
                "cpp_type": "typed::ThreadId",
                "directionality": "DecodeOnly",
                "disposition": "StrongIdentifier",
                "schema_node_kind": "array_element",
                "schema_path": loaded_data_path + "/items",
                "semantic_identifier": "typed::ThreadId",
                "semantic_identifier_role": "vector_element",
            },
            loaded_item_mapping,
        )
        session_id_path = (
            "#/definitions/v2/Thread/properties/sessionId"
        )
        session_id_mapping = next(
            row
            for row in schema_paths
            if isinstance(row, dict)
            and row.get("schema_path") == session_id_path
        )
        self.assertEqual(
            {
                "cpp_base_type": "typed::SessionId",
                "cpp_mapping": (
                    "typed::Thread::sessionId -> typed::SessionId"
                ),
                "cpp_member": "sessionId",
                "cpp_owner": "typed::Thread",
                "cpp_type": "typed::SessionId",
                "directionality": "DecodeOnly",
                "disposition": "StrongIdentifier",
                "nullable": False,
                "presence_model": "RequiredValue",
                "required": True,
                "schema_node_kind": "property",
                "schema_path": session_id_path,
                "semantic_identifier": "typed::SessionId",
                "semantic_identifier_role": "scalar_property",
            },
            session_id_mapping,
        )
        client_user_message_paths = {
            (
                "#/definitions/v2/ThreadItem/oneOf/0/properties/"
                "clientId"
            ): ("typed::UserMessageThreadItem", "clientId", "DecodeOnly"),
            (
                "#/definitions/v2/TurnStartParams/properties/"
                "clientUserMessageId"
            ): (
                "typed::TurnStartParams",
                "clientUserMessageId",
                "EncodeOnly",
            ),
            (
                "#/definitions/v2/TurnSteerParams/properties/"
                "clientUserMessageId"
            ): (
                "typed::TurnSteerParams",
                "clientUserMessageId",
                "EncodeOnly",
            ),
        }
        for path, (
            owner,
            member,
            directionality,
        ) in client_user_message_paths.items():
            with self.subTest(client_user_message_id_path=path):
                row = next(
                    value
                    for value in schema_paths
                    if isinstance(value, dict)
                    and value.get("schema_path") == path
                )
                self.assertEqual(
                    {
                        "cpp_base_type": "typed::ClientUserMessageId",
                        "cpp_mapping": (
                            f"{owner}::{member} -> "
                            "typed::OptionalNullable<"
                            "typed::ClientUserMessageId>"
                        ),
                        "cpp_member": member,
                        "cpp_owner": owner,
                        "cpp_type": (
                            "typed::OptionalNullable<"
                            "typed::ClientUserMessageId>"
                        ),
                        "directionality": directionality,
                        "disposition": "StrongIdentifier",
                        "nullable": True,
                        "presence_model": "OptionalNullable",
                        "required": False,
                        "schema_node_kind": "property",
                        "schema_path": path,
                        "semantic_identifier": (
                            "typed::ClientUserMessageId"
                        ),
                        "semantic_identifier_role": "scalar_property",
                    },
                    row,
                )
        semantic_identifiers = self.closure[
            "semantic_strong_identifiers"
        ]
        assert isinstance(semantic_identifiers, dict)
        self.assertEqual(
            [session_id_path],
            semantic_identifiers["SessionId"]["scalar_property_paths"],
        )
        self.assertEqual(
            sorted(client_user_message_paths),
            semantic_identifiers["ClientUserMessageId"][
                "scalar_property_paths"
            ],
        )
        reviewed_defaults = {
            (
                "#/definitions/v2/ThreadForkResponse/properties/"
                "instructionSources"
            ): [],
            (
                "#/definitions/v2/ThreadResumeResponse/properties/"
                "instructionSources"
            ): [],
            (
                "#/definitions/v2/ThreadStartResponse/properties/"
                "instructionSources"
            ): [],
            "#/definitions/v2/Turn/properties/itemsView": "full",
        }
        defaulted_rows = {
            row["schema_path"]: row
            for row in schema_paths
            if isinstance(row, dict)
            and row.get("schema_path") in reviewed_defaults
        }
        self.assertEqual(set(reviewed_defaults), set(defaulted_rows))
        for path, default in reviewed_defaults.items():
            with self.subTest(defaulted_path=path):
                row = defaulted_rows[path]
                self.assertEqual("DefaultedValue", row["presence_model"])
                self.assertEqual(row["cpp_base_type"], row["cpp_type"])
                self.assertEqual(default, row["schema_default"])
                self.assertEqual(
                    "CanonicalValueOnOmission",
                    row["default_behavior"],
                )

        nullable_default_paths = {
            "#/definitions/v2/ActivePermissionProfile/properties/extends",
            "#/definitions/v2/ResponseItem/oneOf/2/properties/content",
            (
                "#/definitions/v2/SubAgentSource/oneOf/1/properties/"
                "thread_spawn/properties/agent_nickname"
            ),
            (
                "#/definitions/v2/SubAgentSource/oneOf/1/properties/"
                "thread_spawn/properties/agent_path"
            ),
            (
                "#/definitions/v2/SubAgentSource/oneOf/1/properties/"
                "thread_spawn/properties/agent_role"
            ),
            (
                "#/definitions/v2/ThreadItem/oneOf/2/properties/"
                "memoryCitation"
            ),
            "#/definitions/v2/ThreadItem/oneOf/2/properties/phase",
            "#/definitions/v2/TurnError/properties/additionalDetails",
            "#/definitions/v2/UserInput/oneOf/1/properties/detail",
            "#/definitions/v2/UserInput/oneOf/2/properties/detail",
        }
        nullable_default_rows = {
            row["schema_path"]: row
            for row in schema_paths
            if isinstance(row, dict)
            and row.get("schema_path") in nullable_default_paths
        }
        self.assertEqual(
            nullable_default_paths, set(nullable_default_rows)
        )
        for path in sorted(nullable_default_paths):
            with self.subTest(nullable_default_path=path):
                row = nullable_default_rows[path]
                self.assertEqual("OptionalNullable", row["presence_model"])
                self.assertIsNone(row["schema_default"])
                self.assertEqual(
                    "PreserveOmittedNullValue",
                    row["default_behavior"],
                )
    def test_exact_report_mutation_diagnostics(self) -> None:
        def identity(
            plan: dict[str, object],
            predicate: Callable[[dict[str, object]], bool],
        ) -> dict[str, object]:
            rows = plan["identities"]
            assert isinstance(rows, list)
            return next(row for row in rows if isinstance(row, dict) and predicate(row))

        def taxonomy_arithmetic(plan: dict[str, object], _: dict[str, object]) -> None:
            counts = plan["counts"]
            assert isinstance(counts, dict)
            counts["identity_denominator"] = 173

        def identity_count(plan: dict[str, object], _: dict[str, object]) -> None:
            ratchet = plan["final_exact_a1_1_ratchet"]
            assert isinstance(ratchet, list)
            ratchet.append(copy.deepcopy(ratchet[0]))

        def result_kind(plan: dict[str, object], _: dict[str, object]) -> None:
            row = identity(
                plan,
                lambda value: isinstance(value.get("request_contract"), dict)
                and value["request_contract"].get("result_contract_kind") == "Unit",
            )
            contract = row["request_contract"]
            assert isinstance(contract, dict)
            contract["result_contract_kind"] = "Concrete"

        def status_split(plan: dict[str, object], _: dict[str, object]) -> None:
            identity(
                plan, lambda value: value.get("current_schema_status") == "Partial"
            )["current_schema_status"] = "NotImplemented"

        def duplicate_identity(plan: dict[str, object], _: dict[str, object]) -> None:
            rows = plan["identities"]
            assert isinstance(rows, list)
            indexes = [
                index
                for index, row in enumerate(rows)
                if isinstance(row, dict)
                and row.get("domain") == "ResponseItem"
                and row.get("category") == "item_discriminator"
            ]
            rows[indexes[1]] = copy.deepcopy(rows[indexes[0]])

        def cross_slice_identity(plan: dict[str, object], _: dict[str, object]) -> None:
            row = identity(
                plan,
                lambda value: value.get("category") == "item_discriminator",
            )
            key = row["protocol_surface_key"]
            assert isinstance(key, dict)
            key["name"] = "futureItem"

        def unstable_identity(plan: dict[str, object], _: dict[str, object]) -> None:
            rows = plan["identities"]
            assert isinstance(rows, list) and isinstance(rows[0], dict)
            rows[0]["stability"] = "experimental_only"

        def assignment_mismatch(plan: dict[str, object], _: dict[str, object]) -> None:
            rows = plan["identities"]
            assert isinstance(rows, list) and isinstance(rows[0], dict)
            rows[0]["module"] = "WrongModule"

        def contract_mismatch(plan: dict[str, object], _: dict[str, object]) -> None:
            row = identity(
                plan, lambda value: value.get("category") == "client_request"
            )
            contract = row["request_contract"]
            assert isinstance(contract, dict)
            contract["proposed_public_method_name"] = "typed::Threads::wrong"

        def batch_cycle(plan: dict[str, object], _: dict[str, object]) -> None:
            batches = plan["batches"]
            assert isinstance(batches, list)
            batch = next(
                row
                for row in batches
                if isinstance(row, dict) and row.get("batch") == "B2"
            )
            batch["depends_on"] = ["B5"]

        def duplicate_batch_assignment(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            rows = plan["identities"]
            assert isinstance(rows, list)
            indexes = [
                index
                for index, row in enumerate(rows)
                if isinstance(row, dict)
                and row.get("domain") == "ResponseItem"
                and row.get("category") == "item_discriminator"
            ]
            replacement = copy.deepcopy(rows[indexes[0]])
            assert isinstance(replacement, dict)
            replacement["owning_implementation_batch"] = "B4"
            rows[indexes[1]] = replacement

        def missing_closure_disposition(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            definitions = closure["definitions"]
            assert isinstance(definitions, list) and isinstance(definitions[0], dict)
            definitions[0].pop("disposition")

        def opaque_json_misuse(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            paths = closure["schema_paths"]
            assert isinstance(paths, list)
            row = next(
                value
                for value in paths
                if isinstance(value, dict)
                and value.get("disposition") == "ProtocolDefinedOpaqueJson"
            )
            row["disposition"] = "ReusedExistingType"

        def conflicting_ownership(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            definitions = closure["definitions"]
            assert isinstance(definitions, list)
            rows = [
                row
                for row in definitions
                if isinstance(row, dict)
                and row.get("disposition") == "PublicHandwrittenType"
            ]
            rows[1]["cpp_owner"] = rows[0]["cpp_owner"]

        def cross_slice_dependency(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            definitions = closure["definitions"]
            assert isinstance(definitions, list) and isinstance(definitions[0], dict)
            dependencies = definitions[0]["direct_dependencies"]
            assert isinstance(dependencies, list)
            dependencies.append("OutsideA11")

        def directionality_mismatch(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            counts = closure["counts"]
            assert isinstance(counts, dict)
            counts["definition_directions"] = {"Both": 999}

        def discriminator_literal_mismatch(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            counts = plan["counts"]
            assert isinstance(counts, dict)
            counts["discriminator_mapping_sha256"] = "0" * 64

        def cross_slice_ownership_mismatch(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            closure["cross_slice_definition_ownership"] = {}

        def optional_nullable_mapping_mismatch(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            closure["optional_nullable_matrix"] = {}

        def strong_identifier_mapping_mismatch(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            closure["semantic_strong_identifiers"] = {}

        def reassign_strong_identifier(
            closure: dict[str, object],
            path: str,
            old_identifier: str,
            new_identifier: str,
        ) -> None:
            paths = closure["schema_paths"]
            assert isinstance(paths, list)
            row = next(
                value
                for value in paths
                if isinstance(value, dict)
                and value.get("schema_path") == path
            )
            new_base_type = f"typed::{new_identifier}"
            row["cpp_base_type"] = new_base_type
            if row.get("presence_model") == "OptionalNullable":
                row["cpp_type"] = (
                    f"typed::OptionalNullable<{new_base_type}>"
                )
            else:
                row["cpp_type"] = new_base_type
            row["cpp_mapping"] = (
                f"{row['cpp_owner']}::{row['cpp_member']} -> "
                f"{row['cpp_type']}"
            )
            row["semantic_identifier"] = new_base_type

            semantic_identifiers = closure[
                "semantic_strong_identifiers"
            ]
            assert isinstance(semantic_identifiers, dict)
            old_paths = semantic_identifiers[old_identifier][
                "scalar_property_paths"
            ]
            new_paths = semantic_identifiers[new_identifier][
                "scalar_property_paths"
            ]
            assert isinstance(old_paths, list)
            assert isinstance(new_paths, list)
            old_paths.remove(path)
            new_paths.append(path)
            new_paths.sort()

            counts = closure["counts"]
            assert isinstance(counts, dict)
            counts["schema_path_mapping_sha256"] = self.tool.sha256_json(
                [
                    self.tool.schema_path_mapping_projection(value)
                    for value in paths
                    if isinstance(value, dict)
                ]
            )

        def session_id_semantic_mismatch(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            reassign_strong_identifier(
                closure,
                "#/definitions/v2/Thread/properties/sessionId",
                "SessionId",
                "ThreadId",
            )

        def client_user_message_id_semantic_mismatch(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            reassign_strong_identifier(
                closure,
                (
                    "#/definitions/v2/TurnStartParams/properties/"
                    "clientUserMessageId"
                ),
                "ClientUserMessageId",
                "ItemId",
            )

        def cpp_path_mapping_mismatch(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            counts = closure["counts"]
            assert isinstance(counts, dict)
            counts["schema_path_mapping_sha256"] = "0" * 64

        def reviewed_inline_cpp_mapping_mismatch(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            paths = closure["schema_paths"]
            assert isinstance(paths, list)
            row = next(
                value
                for value in paths
                if isinstance(value, dict)
                and value.get("schema_path")
                == (
                    "#/definitions/v2/SubAgentSource/oneOf/1/"
                    "properties/thread_spawn"
                )
            )
            row["cpp_base_type"] = "typed::NonexistentThreadSpawnDetails"
            row["cpp_type"] = "typed::NonexistentThreadSpawnDetails"
            row["cpp_mapping"] = (
                "typed::ThreadSpawnSubAgentSource::threadSpawn -> "
                "typed::NonexistentThreadSpawnDetails"
            )
            counts = closure["counts"]
            assert isinstance(counts, dict)
            counts["schema_path_mapping_sha256"] = self.tool.sha256_json(
                [
                    self.tool.schema_path_mapping_projection(value)
                    for value in paths
                    if isinstance(value, dict)
                ]
            )

        def reviewed_default_mapping_mismatch(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            paths = closure["schema_paths"]
            assert isinstance(paths, list)
            row = next(
                value
                for value in paths
                if isinstance(value, dict)
                and value.get("schema_path")
                == "#/definitions/v2/Turn/properties/itemsView"
            )
            row["schema_default"] = "summary"
            counts = closure["counts"]
            assert isinstance(counts, dict)
            counts["schema_path_mapping_sha256"] = self.tool.sha256_json(
                [
                    self.tool.schema_path_mapping_projection(value)
                    for value in paths
                    if isinstance(value, dict)
                ]
            )

        def nullable_default_mapping_mismatch(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            paths = closure["schema_paths"]
            assert isinstance(paths, list)
            row = next(
                value
                for value in paths
                if isinstance(value, dict)
                and value.get("schema_path")
                == (
                    "#/definitions/v2/SubAgentSource/oneOf/1/"
                    "properties/thread_spawn/properties/agent_path"
                )
            )
            row["default_behavior"] = "CanonicalValueOnOmission"
            counts = closure["counts"]
            assert isinstance(counts, dict)
            counts["schema_path_mapping_sha256"] = self.tool.sha256_json(
                [
                    self.tool.schema_path_mapping_projection(value)
                    for value in paths
                    if isinstance(value, dict)
                ]
            )

        def cpp_definition_mapping_mismatch(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            counts = closure["counts"]
            assert isinstance(counts, dict)
            counts["definition_cpp_mapping_sha256"] = "0" * 64

        cases = (
            ("taxonomy arithmetic", taxonomy_arithmetic, ("TaxonomyArithmeticMismatch",)),
            ("identity count", identity_count, ("IdentityCountMismatch",)),
            (
                "result kind",
                result_kind,
                ("ResultKindMismatch",) * 3,
            ),
            (
                "status split",
                status_split,
                ("StatusSplitMismatch",) * 2,
            ),
            (
                "duplicate and missing identity",
                duplicate_identity,
                (
                    "DuplicateIdentity",
                    "MissingIdentity",
                    "StagedRatchetMismatch",
                ),
            ),
            (
                "cross-slice and missing identity",
                cross_slice_identity,
                (
                    "CrossSliceIdentity",
                    "MalformedReviewedPublicMapping",
                    "MissingIdentity",
                    "StagedRatchetMismatch",
                ),
            ),
            ("unstable identity", unstable_identity, ("UnstableIdentity",)),
            (
                "assignment mismatch",
                assignment_mismatch,
                ("AssignmentMismatch",) * 2,
            ),
            ("operation contract", contract_mismatch, ("ContractMismatch",)),
            ("batch dependency cycle", batch_cycle, ("BatchDependencyCycle",)),
            (
                "duplicate batch assignment",
                duplicate_batch_assignment,
                (
                    "AssignmentMismatch",
                    "AssignmentMismatch",
                    "DuplicateBatchAssignment",
                    "DuplicateIdentity",
                    "MissingIdentity",
                    "StagedRatchetMismatch",
                ),
            ),
            (
                "missing closure disposition",
                missing_closure_disposition,
                ("MissingClosureDisposition",) * 2,
            ),
            (
                "opaque JSON misuse",
                opaque_json_misuse,
                ("MissingClosureDisposition", "OpaqueJsonMisuse"),
            ),
            (
                "conflicting C++ ownership",
                conflicting_ownership,
                ("ConflictingCppOwnership",),
            ),
            (
                "cross-slice closure dependency",
                cross_slice_dependency,
                ("CrossSliceDependency",),
            ),
            (
                "definition directionality summary",
                directionality_mismatch,
                ("DirectionalityMismatch",),
            ),
            (
                "discriminator literal mapping",
                discriminator_literal_mismatch,
                ("DiscriminatorLiteralMismatch",),
            ),
            (
                "cross-slice definition ownership",
                cross_slice_ownership_mismatch,
                ("CrossSliceOwnershipMismatch",),
            ),
            (
                "optional-nullable mapping matrix",
                optional_nullable_mapping_mismatch,
                ("OptionalNullableMappingMismatch",),
            ),
            (
                "semantic strong-identifier mapping",
                strong_identifier_mapping_mismatch,
                ("StrongIdentifierMappingMismatch",),
            ),
            (
                "App-Server session strong-identifier mapping",
                session_id_semantic_mismatch,
                ("StrongIdentifierMappingMismatch",),
            ),
            (
                "client user-message strong-identifier mapping",
                client_user_message_id_semantic_mismatch,
                ("StrongIdentifierMappingMismatch",),
            ),
            (
                "schema-path C++ mapping",
                cpp_path_mapping_mismatch,
                ("CppPathMappingMismatch",),
            ),
            (
                "reviewed inline C++ mapping",
                reviewed_inline_cpp_mapping_mismatch,
                ("CppPathMappingMismatch",),
            ),
            (
                "reviewed canonical default mapping",
                reviewed_default_mapping_mismatch,
                ("OptionalNullableMappingMismatch",),
            ),
            (
                "nullable default:null mapping",
                nullable_default_mapping_mismatch,
                ("OptionalNullableMappingMismatch",),
            ),
            (
                "definition C++ mapping",
                cpp_definition_mapping_mismatch,
                ("CppDefinitionMappingMismatch",),
            ),
        )
        for description, mutate, expected in cases:
            with self.subTest(description):
                self.assert_report_mutation(mutate, expected)

    def test_intrinsic_evidence_diagnostic_codes(self) -> None:
        manifest = load_json(OPTIONS.manifest)
        assignments = load_json(OPTIONS.assignments)
        reachability = load_json(OPTIONS.reachability)
        contracts = load_json(OPTIONS.contracts)
        registry = self.tool.surface.parse_registry_data(OPTIONS.registry)

        duplicate_assignments = copy.deepcopy(assignments)
        rows = duplicate_assignments["assignments"]
        assert isinstance(rows, list)
        rows.append(copy.deepcopy(rows[0]))
        diagnostics = self.tool.surface.assignment_reachability_diagnostics(
            manifest, duplicate_assignments, reachability
        )
        self.assertEqual(
            ["DuplicateModuleSliceAssignment"],
            [diagnostic["code"] for diagnostic in diagnostics],
        )

        mutated_reachability = copy.deepcopy(reachability)
        records = mutated_reachability["records"]
        assert isinstance(records, list) and isinstance(records[0], dict)
        records[0]["reaching_root_count"] += 1
        diagnostics = self.tool.surface.assignment_reachability_diagnostics(
            manifest, assignments, mutated_reachability
        )
        self.assertEqual(
            ["ReachabilityRootSetMismatch"],
            [diagnostic["code"] for diagnostic in diagnostics],
        )

        mutated_contracts = copy.deepcopy(contracts)
        contract_rows = mutated_contracts["contracts"]
        assert isinstance(contract_rows, list)
        contract = next(
            row
            for row in contract_rows
            if isinstance(row, dict)
            and row.get("surface_key", {}).get("name") == "thread/archive"
        )
        contract["parameter_type_identity"] = "WrongThreadArchiveParams"
        diagnostics = self.tool.surface.association_diagnostics(
            manifest, mutated_contracts, registry
        )
        self.assertEqual(
            ["WrongParameterType"],
            [diagnostic["code"] for diagnostic in diagnostics],
        )

    def test_stale_generated_audit_has_exact_code(self) -> None:
        with tempfile.TemporaryDirectory(prefix="snodec-codex-a1-1-audit-") as raw:
            path = Path(raw) / "plan.json"
            self.tool.write_or_check(path, self.plan, False)
            self.tool.write_or_check(path, self.plan, True)
            path.write_text(
                path.read_text(encoding="utf-8") + " ",
                encoding="utf-8",
            )
            with self.assertRaises(self.tool.AuditError) as caught:
                self.tool.write_or_check(path, self.plan, True)
            self.assertEqual("StaleGeneratedAudit", caught.exception.code)
            self.assertEqual(("StaleGeneratedAudit",), caught.exception.codes)


def parse_options() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tool", required=True, type=Path)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--registry", required=True, type=Path)
    parser.add_argument("--schema-root", required=True, type=Path)
    parser.add_argument("--assignments", required=True, type=Path)
    parser.add_argument("--reachability", required=True, type=Path)
    parser.add_argument("--contracts", required=True, type=Path)
    parser.add_argument("--schema-completeness", required=True, type=Path)
    parser.add_argument("--fixture-coverage", required=True, type=Path)
    parser.add_argument("--production-coverage", required=True, type=Path)
    parser.add_argument(
        "--notification-production-coverage", required=True, type=Path
    )
    parser.add_argument("--fixture-index", required=True, type=Path)
    parser.add_argument("--draft07-validator", required=True, type=Path)
    parser.add_argument("--start-state", required=True, type=Path)
    parser.add_argument("--plan-output", required=True, type=Path)
    parser.add_argument("--closure-output", required=True, type=Path)
    return parser.parse_args()


if __name__ == "__main__":
    OPTIONS = parse_options()
    unittest.main(argv=[sys.argv[0]])
