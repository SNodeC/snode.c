#!/usr/bin/env python3
"""Focused offline guards for generated Codex schema fixtures."""

from __future__ import annotations

import copy
import importlib.util
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from types import ModuleType, SimpleNamespace

sys.dont_write_bytecode = True


REPOSITORY_ROOT = Path(__file__).resolve().parents[3]


def load_tool() -> ModuleType:
    path = REPOSITORY_ROOT / "tools/codex/app_server_fixtures.py"
    specification = importlib.util.spec_from_file_location(
        "snodec_codex_app_server_fixtures_test", path
    )
    if specification is None or specification.loader is None:
        raise RuntimeError(f"unable to load fixture tool: {path}")
    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


tool = load_tool()


def load_shared_tool() -> ModuleType:
    path = REPOSITORY_ROOT / "tools/codex/app_server_a1_shared.py"
    tool_directory = str(path.parent)
    if tool_directory not in sys.path:
        sys.path.insert(0, tool_directory)
    specification = importlib.util.spec_from_file_location(
        "snodec_codex_app_server_a1_shared_test", path
    )
    if specification is None or specification.loader is None:
        raise RuntimeError(f"unable to load shared A1 tool: {path}")
    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


shared_tool = load_shared_tool()


def load_schema_path_tool() -> ModuleType:
    path = REPOSITORY_ROOT / "tools/codex/app_server_schema_paths.py"
    specification = importlib.util.spec_from_file_location(
        "snodec_codex_app_server_schema_paths_test", path
    )
    if specification is None or specification.loader is None:
        raise RuntimeError(f"unable to load schema-path tool: {path}")
    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


schema_path_tool = load_schema_path_tool()


def arguments(fixture_root: Path | None = None) -> SimpleNamespace:
    version = "0.144.6"
    return SimpleNamespace(
        repo_root=REPOSITORY_ROOT,
        schema_root=(
            REPOSITORY_ROOT / f"tools/codex/app-server-schema/{version}"
        ),
        manifest=(
            REPOSITORY_ROOT / f"tools/codex/app-server-surface/{version}.json"
        ),
        contracts=(
            REPOSITORY_ROOT
            / f"tools/codex/app-server-evidence/{version}/operation-contracts.json"
        ),
        validator=REPOSITORY_ROOT / "tools/codex/draft07.py",
        fixture_root=(
            fixture_root
            if fixture_root is not None
            else REPOSITORY_ROOT
            / f"tools/codex/app-server-fixtures/{version}"
        ),
        evidence_root=(
            REPOSITORY_ROOT / f"tools/codex/app-server-evidence/{version}"
        ),
    )


_GENERATED_OUTPUTS_SNAPSHOT: (
    tuple[tuple[tuple[Path, bytes], ...], bytes] | None
) = None


def generated_outputs_snapshot() -> tuple[dict[Path, bytes], dict[str, object]]:
    """Return fresh containers backed by one immutable exhaustive generation."""

    global _GENERATED_OUTPUTS_SNAPSHOT
    if _GENERATED_OUTPUTS_SNAPSHOT is None:
        outputs, index = tool.generated_outputs(arguments())
        _GENERATED_OUTPUTS_SNAPSHOT = (
            tuple(sorted(outputs.items(), key=lambda item: str(item[0]))),
            tool.encoded_json(index),
        )
    encoded_outputs, encoded_index = _GENERATED_OUTPUTS_SNAPSHOT
    return dict(encoded_outputs), json.loads(encoded_index)


def normalized_generated_outputs(
    outputs: dict[Path, bytes], configured: SimpleNamespace
) -> dict[str, bytes]:
    normalized: dict[str, bytes] = {}
    for root_name, root in (
        ("fixtures", configured.fixture_root),
        ("evidence", configured.evidence_root),
    ):
        resolved_root = root.resolve()
        for path, payload in outputs.items():
            try:
                relative = path.resolve().relative_to(resolved_root)
            except ValueError:
                continue
            normalized[f"{root_name}/{relative.as_posix()}"] = payload
    if len(normalized) != len(outputs):
        raise AssertionError("generated output escaped its managed roots")
    return normalized


class AppServerFixtureToolTest(unittest.TestCase):
    def test_shared_graph_helpers_preserve_fixture_contracts(self) -> None:
        first = tool.DefinitionId("v2", "First")
        second = tool.DefinitionId("v2", "Second")
        edges = {first: {second}, second: set()}

        self.assertEqual(
            frozenset({first, second}),
            shared_tool.transitive_closure(
                (first,),
                edges,
                unknown_node_error=lambda current: tool.FixtureError(
                    f"unknown definition graph root {current}"
                ),
            ),
        )
        with self.assertRaisesRegex(
            tool.FixtureError,
            r"^unknown definition graph root DefinitionId"
        ):
            shared_tool.transitive_closure(
                (tool.DefinitionId("v2", "Missing"),),
                edges,
                unknown_node_error=lambda current: tool.FixtureError(
                    f"unknown definition graph root {current}"
                ),
            )
        self.assertEqual(
            {
                tool.DefinitionId("legacy", "Legacy/Name"),
                tool.DefinitionId("v2", "Future~Name"),
            },
            tool.schema_references(
                {
                    "allOf": [
                        {"$ref": "#/definitions/Legacy~1Name"},
                        {"$ref": "#/definitions/v2/Future~0Name"},
                        {"$ref": "external.json#/definitions/Ignored"},
                    ],
                }
            ),
        )
        self.assertEqual(
            ("v2", "Future~Name"),
            shared_tool.definition_reference_parts(
                "#/definitions/v2/Future~0Name"
            ),
        )
        self.assertEqual(
            "#/definitions/v2/Future~0Name",
            shared_tool.definition_path("v2", "Future~Name"),
        )
        self.assertIsNone(
            shared_tool.definition_reference_parts(
                "external.json#/definitions/Ignored"
            )
        )
        self.assertFalse(shared_tool.has_dependency_cycle(edges))
        self.assertTrue(
            shared_tool.has_dependency_cycle(
                {first: {second}, second: {first}}
            )
        )
        self.assertEqual(
            [first, second, first],
            shared_tool.find_dependency_cycle(
                {first, second},
                {first: {second}, second: {first}},
            ),
        )

    def test_shared_historical_source_projection_preserves_policy(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "source.txt"
            source.write_text("reviewed\n", encoding="utf-8")
            frozen_hash = tool.sha256_file(source)

            records = shared_tool.historical_source_records(
                {"source": source},
                root,
                frozen_hashes={"source": frozen_hash},
                mutable_names=(),
                source_set_error=lambda: RuntimeError("source set"),
                immutable_source_error=lambda relative: RuntimeError(
                    f"immutable source: {relative}"
                ),
                resolve_paths=False,
            )
            self.assertEqual(
                {
                    "source": {
                        "path": "source.txt",
                        "sha256": frozen_hash,
                    }
                },
                records,
            )

            source.write_text("drifted\n", encoding="utf-8")
            with self.assertRaisesRegex(
                RuntimeError,
                r"^immutable source: source\.txt$",
            ):
                shared_tool.historical_source_records(
                    {"source": source},
                    root,
                    frozen_hashes={"source": frozen_hash},
                    mutable_names=(),
                    source_set_error=lambda: RuntimeError("source set"),
                    immutable_source_error=lambda relative: RuntimeError(
                        f"immutable source: {relative}"
                    ),
                    resolve_paths=False,
                )
            with self.assertRaisesRegex(RuntimeError, r"^source set$"):
                shared_tool.historical_source_records(
                    {},
                    root,
                    frozen_hashes={"source": frozen_hash},
                    mutable_names=(),
                    source_set_error=lambda: RuntimeError("source set"),
                    immutable_source_error=lambda relative: RuntimeError(
                        f"immutable source: {relative}"
                    ),
                    resolve_paths=False,
                )

    def test_schema_path_walker_is_deterministic_and_state_carrying(
        self,
    ) -> None:
        schema = {
            "required": ["required/name"],
            "properties": {
                "z": {"type": "string"},
                "required/name": {
                    "required": ["nested"],
                    "properties": {
                        "nested": {"type": "integer"},
                    },
                },
            },
            "additionalProperties": {"type": "number"},
            "items": [
                {"type": "boolean"},
                {"type": "null"},
            ],
            "allOf": [
                {"properties": {"all": {"type": "string"}}},
            ],
            "anyOf": [
                {"properties": {"any": {"type": "string"}}},
            ],
            "oneOf": [
                {"properties": {"one": {"type": "string"}}},
            ],
        }

        def transition(
            state: tuple[str, ...],
            kind: str,
            token: str | int | None,
            path: str,
            child_schema: object,
            required: bool | None,
        ) -> tuple[str, ...]:
            self.assertTrue(path.startswith("#/"))
            self.assertIsNotNone(child_schema)
            return (*state, f"{kind}:{token}:{required}")

        visits = list(
            schema_path_tool.walk_schema_paths(
                schema,
                path="#",
                state=(),
                transition=transition,
            )
        )
        self.assertEqual(
            [
                "#/properties/required~1name",
                "#/properties/required~1name/properties/nested",
                "#/properties/z",
                "#/additionalProperties",
                "#/items/0",
                "#/items/1",
                "#/allOf/0",
                "#/allOf/0/properties/all",
                "#/anyOf/0",
                "#/anyOf/0/properties/any",
                "#/oneOf/0",
                "#/oneOf/0/properties/one",
            ],
            [visit.path for visit in visits],
        )
        self.assertEqual(
            [
                schema_path_tool.PROPERTY,
                schema_path_tool.PROPERTY,
                schema_path_tool.PROPERTY,
                schema_path_tool.MAP_VALUE,
                schema_path_tool.ARRAY_ELEMENT,
                schema_path_tool.ARRAY_ELEMENT,
                schema_path_tool.ALLOF_BRANCH,
                schema_path_tool.PROPERTY,
                schema_path_tool.ANYOF_BRANCH,
                schema_path_tool.PROPERTY,
                schema_path_tool.ONEOF_BRANCH,
                schema_path_tool.PROPERTY,
            ],
            [visit.kind for visit in visits],
        )
        self.assertEqual(
            [
                True,
                True,
                False,
                None,
                None,
                None,
                None,
                False,
                None,
                False,
                None,
                False,
            ],
            [visit.required for visit in visits],
        )
        self.assertEqual(
            (
                "property:required/name:True",
                "property:nested:True",
            ),
            visits[1].state,
        )
        self.assertEqual(
            (
                "oneOf_branch:0:None",
                "property:one:False",
            ),
            visits[-1].state,
        )

        legacy_order = list(
            schema_path_tool.walk_schema_paths(
                schema,
                path="#",
                state=None,
                value_keyword_order=("items", "additionalProperties"),
                combinator_keyword_order=("oneOf", "anyOf", "allOf"),
            )
        )
        legacy_paths = [visit.path for visit in legacy_order]
        self.assertLess(
            legacy_paths.index("#/items/0"),
            legacy_paths.index("#/additionalProperties"),
        )
        self.assertLess(
            legacy_paths.index("#/oneOf/0"),
            legacy_paths.index("#/allOf/0"),
        )

    def test_schema_path_walker_handles_single_items_closed_maps_and_refs(
        self,
    ) -> None:
        schema = {
            "$ref": "#/definitions/base",
            "properties": {"sibling": {"type": "string"}},
            "additionalProperties": False,
            "items": True,
        }
        unskipped = list(
            schema_path_tool.walk_schema_paths(
                schema,
                path="#",
                state="unchanged",
            )
        )
        self.assertEqual(
            [
                (
                    "#/properties/sibling",
                    schema_path_tool.PROPERTY,
                    False,
                    "unchanged",
                ),
                (
                    "#/items",
                    schema_path_tool.ARRAY_ELEMENT,
                    None,
                    "unchanged",
                ),
            ],
            [
                (visit.path, visit.kind, visit.required, visit.state)
                for visit in unskipped
            ],
        )
        self.assertEqual(
            [],
            list(
                schema_path_tool.walk_schema_paths(
                    schema,
                    path="#",
                    state=None,
                    skip_references=True,
                )
            ),
        )

        with self.assertRaisesRegex(
            schema_path_tool.SchemaPathWalkError,
            "required must be an array of strings at #",
        ):
            list(
                schema_path_tool.walk_schema_paths(
                    {"required": "not-an-array"},
                    path="#",
                    state=None,
                )
            )

    def test_reviewed_existing_typed_scope_matches_cpp_ratchet(self) -> None:
        header = (
            REPOSITORY_ROOT
            / "tests/component/codex/CodexTypedSurfaceBaseline.h"
        ).read_text(encoding="utf-8")
        category_names = {
            "ClientNotification": tool.CLIENT_NOTIFICATION,
            "ClientRequest": tool.CLIENT_REQUEST,
            "ItemDiscriminator": tool.ITEM_DISCRIMINATOR,
            "ServerNotification": tool.SERVER_NOTIFICATION,
            "ServerRequest": tool.SERVER_REQUEST,
        }
        pattern = re.compile(
            r"SurfaceCategory::(?P<category>[A-Za-z]+)\s*,\s*"
            r'"(?P<domain>[^"]+)"\s*,\s*'
            r'"(?P<field>[^"]+)"\s*,\s*'
            r'"(?P<name>[^"]+)"',
            re.MULTILINE,
        )
        header_identities = {
            (
                category_names[match.group("category")],
                match.group("domain"),
                match.group("field"),
                match.group("name"),
            )
            for match in pattern.finditer(header)
        }
        reviewed = set(tool.EXISTING_TYPED_FIXTURE_IDENTITIES)
        self.assertEqual(len(reviewed), 34)
        self.assertEqual(header_identities, reviewed)

    def test_unrecognized_stable_root_requires_explicit_slice_review(self) -> None:
        with self.assertRaises(tool.FixtureError) as caught:
            tool.root_slice(
                tool.CLIENT_REQUEST,
                "plugin/futureStableOperation",
            )
        self.assertEqual(
            str(caught.exception),
            "A1_SLICE_UNRECOGNIZED_STABLE_ROOT: "
            "category='client_request' "
            "name='plugin/futureStableOperation'",
        )

    def test_codex_error_rules_are_exactly_bidirectional_with_pin(self) -> None:
        configured = arguments()
        draft07 = tool.load_draft07(configured.validator)
        catalog = tool.SchemaCatalog(configured.schema_root, draft07)
        manifest = tool.load_json(configured.manifest)
        schema = catalog.union_target("CodexErrorInfo").schema
        tool.validate_codex_error_rule_sets(manifest, schema)

        extra_identity = copy.deepcopy(manifest)
        template = next(
            entry
            for entry in extra_identity["entries"]
            if entry["domain"] == "CodexErrorInfo"
        )
        added = copy.deepcopy(template)
        added["id"] = "tagged_union_discriminator:CodexErrorInfo:$variant:futurePinnedError"
        added["name"] = "futurePinnedError"
        extra_identity["entries"].append(added)
        with self.assertRaisesRegex(
            tool.FixtureError,
            "CODEX_ERROR_INFO_MANIFEST_RULE_MISMATCH",
        ):
            tool.validate_codex_error_rule_sets(extra_identity, schema)

        wrong_http_shape = copy.deepcopy(schema)
        http_branch = next(
            branch
            for branch in wrong_http_shape["oneOf"]
            if "httpConnectionFailed" in branch.get("properties", {})
        )
        http_branch["properties"]["httpConnectionFailed"]["properties"][
            "httpStatusCode"
        ]["type"] = ["string", "null"]
        with self.assertRaisesRegex(
            tool.FixtureError,
            "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH",
        ):
            tool.validate_codex_error_rule_sets(manifest, wrong_http_shape)

    def test_b2_shared_common_assignment_key_set_is_exact(self) -> None:
        assignment_document = json.loads(
            (
                arguments().evidence_root
                / "module-slice-assignment.json"
            ).read_text(encoding="utf-8")
        )
        assignments = {
            tool.SurfaceKey.from_contract(record["surface_key"]): record
            for record in assignment_document["assignments"]
        }
        keys = tool.derive_b2_shared_common_keys(assignments)
        actual_by_family: dict[str, set[str]] = {}
        for key in keys:
            actual_by_family.setdefault(key.domain, set()).add(key.name)
        self.assertEqual(
            actual_by_family,
            {
                "AskForApproval": {
                    "granular",
                    "never",
                    "on-request",
                    "untrusted",
                },
                "CommandAction": {
                    "listFiles",
                    "read",
                    "search",
                    "unknown",
                },
                "DynamicToolCallOutputContentItem": {
                    "inputImage",
                    "inputText",
                },
                "PatchChangeKind": {"add", "delete", "update"},
                "SandboxPolicy": {
                    "dangerFullAccess",
                    "externalSandbox",
                    "readOnly",
                    "workspaceWrite",
                },
                "UserInput": {
                    "image",
                    "localImage",
                    "mention",
                    "skill",
                    "text",
                },
                "WebSearchAction": {
                    "findInPage",
                    "openPage",
                    "other",
                    "search",
                },
            },
        )
        self.assertEqual(len(keys), 26)

        mutated = copy.deepcopy(assignments)
        removed = next(
            key
            for key in keys
            if key.category == tool.TAGGED_UNION_DISCRIMINATOR
        )
        mutated[removed] = {
            **mutated[removed],
            "classification": "SharedWithinSlice",
        }
        with self.assertRaisesRegex(
            tool.FixtureError,
            "A1.1 SharedCommon assignment mismatch: count=25",
        ):
            tool.derive_b2_shared_common_keys(mutated)

    def test_b3_item_assignment_key_set_is_exact(self) -> None:
        assignment_document = json.loads(
            (
                arguments().evidence_root
                / "module-slice-assignment.json"
            ).read_text(encoding="utf-8")
        )
        assignments = {
            tool.SurfaceKey.from_contract(record["surface_key"]): record
            for record in assignment_document["assignments"]
        }
        keys = tool.derive_b3_item_keys(assignments)
        actual_by_family: dict[str, set[str]] = {}
        for key in keys:
            actual_by_family.setdefault(key.domain, set()).add(key.name)
        self.assertEqual(
            actual_by_family,
            {
                domain: set(identities)
                for domain, identities in (
                    tool.B3_ITEM_FAMILY_IDENTITIES.items()
                )
            },
        )
        self.assertEqual(len(keys), 50)
        self.assertEqual(
            sum(
                key.category == tool.ITEM_DISCRIMINATOR
                for key in keys
            ),
            34,
        )
        self.assertEqual(
            sum(
                key.category == tool.TAGGED_UNION_DISCRIMINATOR
                for key in keys
            ),
            16,
        )

        mutated = copy.deepcopy(assignments)
        removed = next(
            key
            for key in keys
            if key.category == tool.TAGGED_UNION_DISCRIMINATOR
        )
        mutated[removed] = {
            **mutated[removed],
            "classification": "SharedWithinSlice",
        }
        with self.assertRaisesRegex(
            tool.FixtureError,
            "A1.1 item assignment mismatch: count=49",
        ):
            tool.derive_b3_item_keys(mutated)

    def test_b2_shared_common_indexed_schema_closure_is_exact(self) -> None:
        configured = arguments()
        index = json.loads(
            (configured.fixture_root / "index.json").read_text(
                encoding="utf-8"
            )
        )
        coverage = json.loads(
            (
                configured.evidence_root / "fixture-coverage.json"
            ).read_text(encoding="utf-8")
        )
        b2 = index["a1_1_shared_common"]
        assignment_keys = {
            tool.SurfaceKey.from_contract(value)
            for value in b2["assignment_derived_keys"]
        }
        self.assertEqual(len(assignment_keys), 26)
        self.assertEqual(
            b2["negative_coverage"]["family_counts"],
            tool.B2_SHARED_COMMON_FAMILY_COUNTS,
        )
        self.assertEqual(
            len(b2["indexed_schema_coverage"]), 26
        )

        records_by_id = {
            record["id"]: record for record in index["fixtures"]
        }

        base_ids = {
            f"union:{key.domain}:{key.name}" for key in assignment_keys
        }
        self.assertTrue(base_ids <= set(records_by_id))
        self.assertTrue(
            all(
                records_by_id[fixture_id]["role"] == "union_branch"
                for fixture_id in base_ids
            )
        )
        self.assertEqual(
            sum(
                record["role"] == "union_optional_omitted"
                and tool.SurfaceKey.from_contract(
                    record["protocol_surface_key"]
                )
                in assignment_keys
                for record in index["fixtures"]
            ),
            21,
        )
        self.assertEqual(
            sum(
                record["role"] == "union_nullable_null"
                and tool.SurfaceKey.from_contract(
                    record["protocol_surface_key"]
                )
                in assignment_keys
                for record in index["fixtures"]
            ),
            12,
        )

        coverage_by_key = {
            tool.SurfaceKey.from_contract(
                record["protocol_surface_key"]
            ): record
            for record in coverage["identity_coverage"]
        }
        for key in assignment_keys:
            record = coverage_by_key[key]
            self.assertTrue(record["schema_direction_coverage"])
            self.assertEqual(
                record["schema_directions_exercised"],
                list(tool.B2_SHARED_COMMON_DIRECTIONS[key.domain]),
            )
            self.assertTrue(
                all(record["completeness_evidence"].values()),
                key.compact(),
            )

        negative = b2["negative_coverage"]
        self.assertEqual(
            sum(
                len(record["missing_required_fixture_ids"])
                for record in negative["alternatives"].values()
            ),
            21,
        )
        self.assertEqual(
            sum(
                len(record["wrong_nested_type_fixture_ids"])
                for record in negative["alternatives"].values()
            ),
            42,
        )
        self.assertEqual(
            sum(
                len(record["missing_discriminator_fixture_ids"])
                for record in negative["alternatives"].values()
            ),
            23,
        )
        self.assertEqual(
            sum(
                len(
                    record[
                        "wrong_discriminator_type_fixture_ids"
                    ]
                )
                for record in negative["alternatives"].values()
            ),
            23,
        )
        malformed_ids = {
            fixture_id
            for record in negative["alternatives"].values()
            for field in (
                "missing_required_fixture_ids",
                "missing_discriminator_fixture_ids",
                "wrong_nested_type_fixture_ids",
                "wrong_discriminator_type_fixture_ids",
            )
            for fixture_id in record[field]
        }
        self.assertEqual(len(malformed_ids), 109)
        self.assertTrue(
            all(
                records_by_id[fixture_id][
                    "expected_diagnostic_codes"
                ]
                == ["one_of_zero"]
                for fixture_id in malformed_ids
            )
        )

        families = negative["families"]
        self.assertEqual(
            set(families), set(tool.B2_SHARED_COMMON_FAMILY_COUNTS)
        )
        future_ids = {
            record["future_discriminator_fixture_id"]
            for record in families.values()
        }
        self.assertEqual(len(future_ids), 7)
        self.assertTrue(
            all(
                records_by_id[fixture_id][
                    "expected_diagnostic_codes"
                ]
                == ["one_of_zero"]
                for fixture_id in future_ids
            )
        )
        ask_future = records_by_id[
            "union:AskForApproval:future-unknown"
        ]
        self.assertEqual(
            json.loads(
                (
                    configured.fixture_root / ask_future["file"]
                ).read_text(encoding="utf-8")
            ),
            "futureAskForApproval",
        )

        enum_coverage = negative["open_string_enums"]
        self.assertEqual(
            set(enum_coverage), {"ImageDetail", "NetworkAccess"}
        )
        expected_enum_ids = {
            f"enum:{domain}:{value}"
            for domain, values in tool.B2_OPEN_STRING_ENUMS.items()
            for value in values
        }
        self.assertEqual(
            {
                fixture_id
                for value in enum_coverage.values()
                for fixture_id in value["known_value_fixture_ids"]
            },
            expected_enum_ids,
        )
        for value in enum_coverage.values():
            future = records_by_id[value["future_value_fixture_id"]]
            self.assertEqual(
                future["expected_diagnostic_codes"],
                ["enum_mismatch"],
            )

    def test_b3_item_indexed_schema_closure_is_exact(self) -> None:
        configured = arguments()
        index = json.loads(
            (configured.fixture_root / "index.json").read_text(
                encoding="utf-8"
            )
        )
        coverage = json.loads(
            (
                configured.evidence_root / "fixture-coverage.json"
            ).read_text(encoding="utf-8")
        )
        b3 = index["a1_1_item_models"]
        assignment_keys = {
            tool.SurfaceKey.from_contract(value)
            for value in b3["assignment_derived_keys"]
        }
        self.assertEqual(len(assignment_keys), 50)
        self.assertEqual(
            b3["negative_coverage"]["family_counts"],
            tool.B3_ITEM_FAMILY_COUNTS,
        )
        self.assertEqual(len(b3["indexed_schema_coverage"]), 50)

        records_by_id = {
            record["id"]: record for record in index["fixtures"]
        }

        def fixture_value(fixture_id: str) -> object:
            record = records_by_id[fixture_id]
            return json.loads(
                (
                    configured.fixture_root / record["file"]
                ).read_text(encoding="utf-8")
            )

        def value_at_path(value: object, path: tuple[object, ...]) -> object:
            current = value
            for component in path:
                if isinstance(component, int):
                    self.assertIsInstance(current, list)
                    current = current[component]
                else:
                    self.assertIsInstance(current, dict)
                    current = current[component]
            return current

        base_ids = {
            f"union:{key.domain}:{key.name}" for key in assignment_keys
        }
        self.assertTrue(base_ids <= set(records_by_id))
        self.assertTrue(
            all(
                records_by_id[fixture_id]["role"] == "union_branch"
                for fixture_id in base_ids
            )
        )
        alternatives = b3["negative_coverage"]["alternatives"]
        expected_reachable_fixture_ids = {
            "item_discriminator:ResponseItem:type:agent_message": {
                "union:ResponseItem:agent_message",
                (
                    "union:ResponseItem:agent_message:nested:"
                    "AgentMessageInputContent:encrypted_content:content-0"
                ),
            },
            (
                "item_discriminator:ResponseItem:"
                "type:custom_tool_call_output"
            ): {
                (
                    "union:ResponseItem:custom_tool_call_output:"
                    "body-content-array"
                ),
            },
            "item_discriminator:ResponseItem:type:function_call_output": {
                (
                    "union:ResponseItem:function_call_output:"
                    "body-content-array"
                ),
            },
            "item_discriminator:ResponseItem:type:local_shell_call": {
                "union:ResponseItem:local_shell_call",
            },
            "item_discriminator:ResponseItem:type:message": {
                "union:ResponseItem:message",
                (
                    "union:ResponseItem:message:nested:"
                    "ContentItem:input_image:content-0"
                ),
                (
                    "union:ResponseItem:message:nested:"
                    "ContentItem:output_text:content-0"
                ),
            },
            "item_discriminator:ResponseItem:type:reasoning": {
                "union:ResponseItem:reasoning",
                (
                    "union:ResponseItem:reasoning:nested:"
                    "ReasoningItemContent:text:content-0"
                ),
            },
            "item_discriminator:ResponseItem:type:web_search_call": {
                "union:ResponseItem:web_search_call",
                (
                    "union:ResponseItem:web_search_call:nested:"
                    "ResponsesApiWebSearchAction:find_in_page:action"
                ),
                (
                    "union:ResponseItem:web_search_call:nested:"
                    "ResponsesApiWebSearchAction:open_page:action"
                ),
                (
                    "union:ResponseItem:web_search_call:nested:"
                    "ResponsesApiWebSearchAction:other:action"
                ),
            },
            "item_discriminator:ThreadItem:type:commandExecution": {
                "union:ThreadItem:commandExecution",
                (
                    "union:ThreadItem:commandExecution:nested:"
                    "CommandAction:listFiles:commandactions-0"
                ),
                (
                    "union:ThreadItem:commandExecution:nested:"
                    "CommandAction:search:commandactions-0"
                ),
                (
                    "union:ThreadItem:commandExecution:nested:"
                    "CommandAction:unknown:commandactions-0"
                ),
            },
            "item_discriminator:ThreadItem:type:dynamicToolCall": {
                "union:ThreadItem:dynamicToolCall",
                (
                    "union:ThreadItem:dynamicToolCall:nested:"
                    "DynamicToolCallOutputContentItem:"
                    "inputImage:contentitems-0"
                ),
            },
            "item_discriminator:ThreadItem:type:fileChange": {
                "union:ThreadItem:fileChange",
                (
                    "union:ThreadItem:fileChange:nested:"
                    "PatchChangeKind:delete:changes-0-kind"
                ),
                (
                    "union:ThreadItem:fileChange:nested:"
                    "PatchChangeKind:update:changes-0-kind"
                ),
            },
            "item_discriminator:ThreadItem:type:userMessage": {
                "union:ThreadItem:userMessage",
                (
                    "union:ThreadItem:userMessage:nested:"
                    "UserInput:image:content-0"
                ),
                (
                    "union:ThreadItem:userMessage:nested:"
                    "UserInput:localImage:content-0"
                ),
                (
                    "union:ThreadItem:userMessage:nested:"
                    "UserInput:mention:content-0"
                ),
                (
                    "union:ThreadItem:userMessage:nested:"
                    "UserInput:skill:content-0"
                ),
            },
            "item_discriminator:ThreadItem:type:webSearch": {
                "union:ThreadItem:webSearch",
                (
                    "union:ThreadItem:webSearch:nested:"
                    "WebSearchAction:findInPage:action"
                ),
                (
                    "union:ThreadItem:webSearch:nested:"
                    "WebSearchAction:openPage:action"
                ),
                (
                    "union:ThreadItem:webSearch:nested:"
                    "WebSearchAction:other:action"
                ),
            },
        }
        actual_reachable_fixture_ids = {
            compact: set(record["reachable_union_fixture_ids"])
            for compact, record in alternatives.items()
            if record["reachable_union_fixture_ids"]
        }
        self.assertEqual(
            actual_reachable_fixture_ids,
            expected_reachable_fixture_ids,
        )

        nested_identity_expectations = {
            "item_discriminator:ResponseItem:type:agent_message": (
                (("content", 0), {"encrypted_content", "input_text"}),
            ),
            "item_discriminator:ResponseItem:type:local_shell_call": (
                (("action",), {"exec"}),
            ),
            "item_discriminator:ResponseItem:type:message": (
                (
                    ("content", 0),
                    {"input_image", "input_text", "output_text"},
                ),
            ),
            "item_discriminator:ResponseItem:type:reasoning": (
                (("content", 0), {"reasoning_text", "text"}),
                (("summary", 0), {"summary_text"}),
            ),
            "item_discriminator:ResponseItem:type:web_search_call": (
                (
                    ("action",),
                    {"find_in_page", "open_page", "other", "search"},
                ),
            ),
            "item_discriminator:ThreadItem:type:commandExecution": (
                (
                    ("commandActions", 0),
                    {"listFiles", "read", "search", "unknown"},
                ),
            ),
            "item_discriminator:ThreadItem:type:dynamicToolCall": (
                (("contentItems", 0), {"inputImage", "inputText"}),
            ),
            "item_discriminator:ThreadItem:type:fileChange": (
                (("changes", 0, "kind"), {"add", "delete", "update"}),
            ),
            "item_discriminator:ThreadItem:type:userMessage": (
                (
                    ("content", 0),
                    {"image", "localImage", "mention", "skill", "text"},
                ),
            ),
            "item_discriminator:ThreadItem:type:webSearch": (
                (
                    ("action",),
                    {"findInPage", "openPage", "other", "search"},
                ),
            ),
        }
        for compact, expectations in nested_identity_expectations.items():
            fixture_ids = actual_reachable_fixture_ids[compact]
            payloads = [
                fixture_value(fixture_id) for fixture_id in fixture_ids
            ]
            for path, expected_identities in expectations:
                self.assertEqual(
                    {
                        value_at_path(payload, path)["type"]
                        for payload in payloads
                    },
                    expected_identities,
                    f"{compact}:{path}",
                )

        for identity in (
            "custom_tool_call_output",
            "function_call_output",
        ):
            compact = (
                "item_discriminator:ResponseItem:type:" f"{identity}"
            )
            record = alternatives[compact]
            base = fixture_value(record["base_fixture_id"])
            self.assertIsInstance(base["output"], str)
            array_id = (
                f"union:ResponseItem:{identity}:body-content-array"
            )
            self.assertEqual(record["helper_union_fixture_ids"], [array_id])
            self.assertEqual(
                record["reachable_union_fixture_ids"], [array_id]
            )
            array_value = fixture_value(array_id)
            self.assertEqual(
                [item["type"] for item in array_value["output"]],
                ["encrypted_content", "input_image", "input_text"],
            )

        local_shell_container_mutations = {
            (
                "union:ResponseItem:local_shell_call:"
                "wrong-nested-type:action-command-0"
            ): ("action", "command", 0),
            (
                "union:ResponseItem:local_shell_call:"
                "wrong-nested-type:action-env-synthetickey"
            ): ("action", "env", "syntheticKey"),
            (
                "union:LocalShellAction:exec:"
                "wrong-nested-type:command-0"
            ): ("command", 0),
            (
                "union:LocalShellAction:exec:"
                "wrong-nested-type:env-synthetickey"
            ): ("env", "syntheticKey"),
        }
        self.assertTrue(
            set(local_shell_container_mutations)
            <= set(
                alternatives[
                    "item_discriminator:ResponseItem:"
                    "type:local_shell_call"
                ]["wrong_nested_type_fixture_ids"]
            )
            | set(
                alternatives[
                    "tagged_union_discriminator:"
                    "LocalShellAction:type:exec"
                ]["wrong_nested_type_fixture_ids"]
            )
        )
        for fixture_id, path in local_shell_container_mutations.items():
            record = records_by_id[fixture_id]
            self.assertEqual(record["role"], "malformed_known_wrong_type")
            self.assertEqual(record["negative_case"], "wrong_nested_type")
            self.assertEqual(
                record["expected_diagnostic_codes"], ["one_of_zero"]
            )
            self.assertIsNone(value_at_path(fixture_value(fixture_id), path))

        coverage_by_key = {
            tool.SurfaceKey.from_contract(
                record["protocol_surface_key"]
            ): record
            for record in coverage["identity_coverage"]
        }
        for key in assignment_keys:
            record = coverage_by_key[key]
            self.assertTrue(record["schema_direction_coverage"])
            self.assertEqual(
                record["schema_directions_exercised"],
                ["Decode"],
            )
            self.assertTrue(
                all(record["completeness_evidence"].values()),
                key.compact(),
            )

        negative = b3["negative_coverage"]
        self.assertEqual(
            negative["counts"],
            {
                "empty_constrained_string": 1,
                "future_discriminator": 9,
                "future_discriminator_generated": 7,
                "future_discriminator_reused": 2,
                "future_open_enum": 24,
                "helper_union_future": 4,
                "helper_union_positive": 24,
                "missing_discriminator": 50,
                "missing_required": 127,
                "nullable_null": 111,
                "optional_omitted": 112,
                "required_nullable": 3,
                "wrong_discriminator_type": 50,
                "wrong_nested_type": 257,
                "wrong_nested_type_generated": 255,
                "wrong_nested_type_reused": 2,
                "wrong_type_opaque_exclusions": 7,
            },
        )
        alternatives = negative["alternatives"]
        self.assertEqual(len(alternatives), 50)
        self.assertEqual(
            sum(
                len(record["optional_omitted_fixture_ids"])
                for record in alternatives.values()
            ),
            112,
        )
        self.assertEqual(
            sum(
                len(record["nullable_null_fixture_ids"])
                for record in alternatives.values()
            ),
            111,
        )
        self.assertEqual(
            sum(
                len(record["required_nullable_fixture_ids"])
                for record in alternatives.values()
            ),
            3,
        )
        self.assertEqual(
            sum(
                len(record["missing_required_fixture_ids"])
                for record in alternatives.values()
            ),
            127,
        )
        self.assertEqual(
            sum(
                len(record["wrong_nested_type_fixture_ids"])
                for record in alternatives.values()
            ),
            257,
        )
        self.assertEqual(
            sum(
                len(record["reused_wrong_nested_type_fixture_ids"])
                for record in alternatives.values()
            ),
            2,
        )
        self.assertEqual(
            sum(
                len(record["wrong_type_opaque_exclusions"])
                for record in alternatives.values()
            ),
            7,
        )
        self.assertEqual(
            {
                (
                    compact,
                    exclusion["instance_path"],
                    exclusion["schema_path"],
                    exclusion["reason"],
                )
                for compact, record in alternatives.items()
                for exclusion in record[
                    "wrong_type_opaque_exclusions"
                ]
            },
            {
                (
                    "item_discriminator:ResponseItem:"
                    "type:tool_search_call",
                    "$/arguments",
                    (
                        "#/definitions/ResponseItem/oneOf/5/"
                        "properties/arguments"
                    ),
                    (
                        "schema accepts every deterministic JSON-type "
                        "candidate; the protocol defines this field as "
                        "semantically arbitrary JSON"
                    ),
                ),
                (
                    "item_discriminator:ResponseItem:"
                    "type:tool_search_output",
                    "$/tools/0",
                    (
                        "#/definitions/ResponseItem/oneOf/9/"
                        "properties/tools/items"
                    ),
                    (
                        "schema accepts every deterministic JSON-type "
                        "candidate; the protocol defines this field as "
                        "semantically arbitrary JSON"
                    ),
                ),
                (
                    "item_discriminator:ThreadItem:"
                    "type:dynamicToolCall",
                    "$/arguments",
                    (
                        "#/definitions/ThreadItem/oneOf/8/"
                        "properties/arguments"
                    ),
                    (
                        "schema accepts every deterministic JSON-type "
                        "candidate; the protocol defines this field as "
                        "semantically arbitrary JSON"
                    ),
                ),
                (
                    "item_discriminator:ThreadItem:type:mcpToolCall",
                    "$/arguments",
                    (
                        "#/definitions/ThreadItem/oneOf/7/"
                        "properties/arguments"
                    ),
                    (
                        "schema accepts every deterministic JSON-type "
                        "candidate; the protocol defines this field as "
                        "semantically arbitrary JSON"
                    ),
                ),
                (
                    "item_discriminator:ThreadItem:type:mcpToolCall",
                    "$/result/_meta",
                    (
                        "#/definitions/McpToolCallResult/"
                        "properties/_meta"
                    ),
                    (
                        "schema accepts every deterministic JSON-type "
                        "candidate; the protocol defines this field as "
                        "semantically arbitrary JSON"
                    ),
                ),
                (
                    "item_discriminator:ThreadItem:type:mcpToolCall",
                    "$/result/content/0",
                    (
                        "#/definitions/McpToolCallResult/"
                        "properties/content/items"
                    ),
                    (
                        "schema accepts every deterministic JSON-type "
                        "candidate; the protocol defines this field as "
                        "semantically arbitrary JSON"
                    ),
                ),
                (
                    "item_discriminator:ThreadItem:type:mcpToolCall",
                    "$/result/structuredContent",
                    (
                        "#/definitions/McpToolCallResult/"
                        "properties/structuredContent"
                    ),
                    (
                        "schema accepts every deterministic JSON-type "
                        "candidate; the protocol defines this field as "
                        "semantically arbitrary JSON"
                    ),
                ),
            },
        )

        malformed_ids = {
            fixture_id
            for record in alternatives.values()
            for field in (
                "missing_required_fixture_ids",
                "missing_discriminator_fixture_ids",
                "wrong_nested_type_fixture_ids",
                "wrong_discriminator_type_fixture_ids",
            )
            for fixture_id in record[field]
        }
        self.assertEqual(len(malformed_ids), 484)
        self.assertTrue(
            all(
                records_by_id[fixture_id][
                    "expected_diagnostic_codes"
                ]
                == ["one_of_zero"]
                for fixture_id in malformed_ids
            )
        )

        families = negative["families"]
        self.assertEqual(
            set(families), set(tool.B3_ITEM_FAMILY_IDENTITIES)
        )
        future_ids = {
            record["future_discriminator_fixture_id"]
            for record in families.values()
        }
        self.assertEqual(len(future_ids), 9)
        self.assertTrue(
            all(
                records_by_id[fixture_id][
                    "expected_diagnostic_codes"
                ]
                == ["one_of_zero"]
                for fixture_id in future_ids
            )
        )
        self.assertTrue(
            all(
                not record["conflicting_discriminator_fixture_ids"]
                and record["conflicting_discriminator_exclusion"]
                for record in families.values()
            )
        )

        future_open_enum_ids = {
            fixture_id
            for record in alternatives.values()
            for fixture_id in record["future_open_enum_fixture_ids"]
        }
        self.assertEqual(len(future_open_enum_ids), 24)
        self.assertEqual(
            sum(":empty-open-enum:" in fixture_id for fixture_id in future_open_enum_ids),
            12,
        )
        self.assertEqual(
            sum(":future-open-enum:" in fixture_id for fixture_id in future_open_enum_ids),
            12,
        )
        self.assertTrue(
            all(
                records_by_id[fixture_id][
                    "expected_diagnostic_codes"
                ]
                == ["one_of_zero"]
                for fixture_id in future_open_enum_ids
            )
        )
        helper_ids = {
            fixture_id
            for record in alternatives.values()
            for fixture_id in record["helper_union_fixture_ids"]
        }
        helper_future_ids = {
            fixture_id
            for record in alternatives.values()
            for fixture_id in record[
                "helper_union_future_fixture_ids"
            ]
        }
        self.assertEqual(
            helper_ids,
            {
                (
                    "union:ResponseItem:custom_tool_call_output:"
                    "body-content-array"
                ),
                (
                    "union:ResponseItem:function_call_output:"
                    "body-content-array"
                ),
                "union:ResponseItem:message:phase-final_answer",
                "union:ThreadItem:agentMessage:phase-final_answer",
            },
        )
        self.assertEqual(len(helper_future_ids), 4)
        self.assertEqual(
            sum(":phase-empty-unknown" in fixture_id for fixture_id in helper_future_ids),
            2,
        )
        self.assertEqual(
            sum(":phase-future-unknown" in fixture_id for fixture_id in helper_future_ids),
            2,
        )
        self.assertTrue(
            all(
                records_by_id[fixture_id][
                    "expected_diagnostic_codes"
                ]
                == ["one_of_zero"]
                for fixture_id in helper_future_ids
            )
        )
        constrained_string_ids = {
            fixture_id
            for record in alternatives.values()
            for fixture_id in record["constrained_string_fixture_ids"]
        }
        self.assertEqual(
            constrained_string_ids,
            {
                (
                    "union:ThreadItem:collabAgentToolCall:"
                    "empty-constrained-string:reasoningeffort"
                )
            },
        )
        constrained = records_by_id[next(iter(constrained_string_ids))]
        self.assertEqual(
            constrained["role"], "malformed_known_empty_string"
        )
        self.assertEqual(
            constrained["expected_diagnostic_codes"], ["one_of_zero"]
        )

        enum_coverage = negative["open_string_enums"]
        self.assertEqual(set(enum_coverage), set(tool.B3_OPEN_STRING_ENUMS))
        self.assertEqual(
            sum(
                len(record["known_value_fixture_ids"])
                for record in enum_coverage.values()
            ),
            39,
        )
        for record in enum_coverage.values():
            future = records_by_id[record["future_value_fixture_id"]]
            self.assertEqual(
                future["expected_diagnostic_codes"],
                ["enum_mismatch"],
            )

    def test_b4_operation_fixture_plan_is_exact_and_complete(self) -> None:
        index = json.loads(
            (arguments().fixture_root / "index.json").read_text(
                encoding="utf-8"
            )
        )
        records_by_id = {
            record["id"]: record for record in index["fixtures"]
        }
        b4 = index["a1_1_operations"]
        self.assertEqual(
            {
                record["name"]
                for record in b4["assignment_derived_request_keys"]
            },
            set(tool.B4_CLIENT_REQUEST_METHODS),
        )
        self.assertEqual(
            len(b4["assignment_derived_request_keys"]), 22
        )
        self.assertEqual(
            len(b4["assignment_derived_union_keys"]), 16
        )
        self.assertEqual(len(b4["indexed_schema_coverage"]), 38)
        self.assertTrue(
            all(
                all(record["schema_fixture_facts"].values())
                and record["schema_direction_coverage"]
                for record in b4["indexed_schema_coverage"].values()
            )
        )

        union_coverage = b4["negative_coverage"][
            "operation_unions"
        ]
        self.assertEqual(
            union_coverage["family_counts"],
            {
                domain: len(names)
                for domain, names in (
                    tool.B4_OPERATION_UNION_FAMILY_IDENTITIES.items()
                )
            },
        )
        self.assertEqual(
            union_coverage["assignment_derived_key_count"], 16
        )
        families = union_coverage["families"]
        self.assertEqual(
            families["SessionSource"][
                "future_external_discriminator_fixture_ids"
            ],
            ["union:SessionSource:future-object-unknown"],
        )
        self.assertEqual(
            families["SubAgentSource"][
                "future_external_discriminator_fixture_ids"
            ],
            ["union:SubAgentSource:future-object-unknown"],
        )
        for domain in ("SessionSource", "SubAgentSource"):
            with self.subTest(future_union_family=domain):
                self.assertEqual(
                    families[domain][
                        "future_discriminator_fixture_id"
                    ],
                    f"union:{domain}:future-unknown",
                )
                self.assertEqual(
                    records_by_id[
                        f"union:{domain}:future-unknown"
                    ]["role"],
                    "unknown_discriminator",
                )
                self.assertEqual(
                    records_by_id[
                        f"union:{domain}:future-object-unknown"
                    ]["role"],
                    "unknown_discriminator",
                )
        self.assertEqual(
            families["ThreadStatus"][
                "future_external_discriminator_fixture_ids"
            ],
            [],
        )
        self.assertEqual(
            {
                domain: record["wrong_outer_shape_fixture_ids"]
                for domain, record in families.items()
            },
            {
                "SessionSource": [
                    "union:SessionSource:wrong-outer-shape"
                ],
                "SubAgentSource": [
                    "union:SubAgentSource:wrong-outer-shape"
                ],
                "ThreadStatus": [
                    "union:ThreadStatus:wrong-outer-shape"
                ],
            },
        )
        self.assertEqual(
            families["SessionSource"][
                "conflicting_discriminator_fixture_ids"
            ],
            ["union:SessionSource:conflicting-discriminators"],
        )
        self.assertEqual(
            families["SubAgentSource"][
                "conflicting_discriminator_fixture_ids"
            ],
            ["union:SubAgentSource:conflicting-discriminators"],
        )
        self.assertIsNone(
            families["SessionSource"][
                "conflicting_discriminator_exclusion"
            ]
        )
        self.assertIsNone(
            families["SubAgentSource"][
                "conflicting_discriminator_exclusion"
            ]
        )
        alternatives = union_coverage["alternatives"]
        for compact, conflict_id in (
            (
                "tagged_union_discriminator:SessionSource:"
                "$variant:custom",
                "union:SessionSource:conflicting-discriminators",
            ),
            (
                "tagged_union_discriminator:SessionSource:"
                "$variant:subAgent",
                "union:SessionSource:conflicting-discriminators",
            ),
            (
                "tagged_union_discriminator:SubAgentSource:"
                "$variant:other",
                "union:SubAgentSource:conflicting-discriminators",
            ),
            (
                "tagged_union_discriminator:SubAgentSource:"
                "$variant:thread_spawn",
                "union:SubAgentSource:conflicting-discriminators",
            ),
        ):
            with self.subTest(conflicting_alternative=compact):
                self.assertEqual(
                    alternatives[compact][
                        "conflicting_discriminator_fixture_ids"
                    ],
                    [conflict_id],
                )
                self.assertIsNone(
                    alternatives[compact][
                        "conflicting_discriminator_exclusion"
                    ]
                )
        for domain, names in (
            (
                "SessionSource",
                ("appServer", "cli", "exec", "unknown", "vscode"),
            ),
            (
                "SubAgentSource",
                ("compact", "memory_consolidation", "review"),
            ),
        ):
            for name in names:
                compact = (
                    "tagged_union_discriminator:"
                    f"{domain}:$variant:{name}"
                )
                with self.subTest(scalar_alternative=compact):
                    self.assertNotIn(
                        "conflicting_discriminator_fixture_ids",
                        alternatives[compact],
                    )
                    self.assertEqual(
                        alternatives[compact][
                            "conflicting_discriminator_exclusion"
                        ],
                        (
                            "JSON value shape provides exactly one scalar "
                            "or object discriminator slot"
                        ),
                    )
        self.assertEqual(
            alternatives[
                "tagged_union_discriminator:SessionSource:"
                "$variant:subAgent"
            ]["nested_malformed_fixture_ids"],
            [
                "union:SessionSource:subAgent:"
                "nested-malformed:thread_spawn-depth"
            ],
        )
        nested_future_id = (
            "union:SessionSource:subAgent:nested-future-unknown"
        )
        self.assertEqual(
            alternatives[
                "tagged_union_discriminator:SessionSource:"
                "$variant:subAgent"
            ]["nested_future_discriminator_fixture_ids"],
            [nested_future_id],
        )
        nested_future = records_by_id[nested_future_id]
        self.assertEqual(nested_future["role"], "unknown_discriminator")
        self.assertEqual(
            nested_future["expected_diagnostic_codes"],
            ["one_of_zero"],
        )
        self.assertEqual(
            json.loads(
                (
                    arguments().fixture_root / nested_future["file"]
                ).read_text(encoding="utf-8")
            ),
            {"subAgent": "futureSubAgentSource"},
        )
        self.assertIn(
            (
                "union:ThreadStatus:active:"
                "wrong-nested-type:activeflags-0"
            ),
            alternatives[
                "tagged_union_discriminator:"
                "ThreadStatus:type:active"
            ]["wrong_nested_type_fixture_ids"],
        )
        thread_status_wrong_outer = records_by_id[
            "union:ThreadStatus:wrong-outer-shape"
        ]
        self.assertEqual(
            json.loads(
                (
                    arguments().fixture_root
                    / thread_status_wrong_outer["file"]
                ).read_text(encoding="utf-8")
            ),
            "idle",
        )
        self.assertEqual(
            set(
                b4["negative_coverage"]["open_string_enums"]
            ),
            set(tool.B4_OPEN_STRING_ENUMS),
        )

        opaque = b4["negative_coverage"][
            "operation_opaque_exclusions"
        ]
        self.assertEqual(len(opaque), 5)
        self.assertEqual(
            {
                (
                    record["operation"],
                    record["instance_path"],
                )
                for record in opaque
            },
            {
                ("thread/fork", "$/config/syntheticKey"),
                ("thread/inject_items", "$/items/0"),
                ("thread/resume", "$/config/syntheticKey"),
                ("thread/start", "$/config/syntheticKey"),
                ("turn/start", "$/outputSchema"),
            },
        )

        start = b4["indexed_schema_coverage"][
            "client_request:ClientRequest:method:thread/start"
        ]
        fork = b4["indexed_schema_coverage"][
            "client_request:ClientRequest:method:thread/fork"
        ]
        self.assertIn(
            "#/properties/ephemeral",
            start["roots"]["params"][
                "optional_property_schema_paths"
            ],
        )
        self.assertIn(
            "#/properties/ephemeral",
            start["roots"]["params"][
                "nullable_property_schema_paths"
            ],
        )
        self.assertIn(
            "#/properties/ephemeral",
            fork["roots"]["params"][
                "optional_property_schema_paths"
            ],
        )
        self.assertNotIn(
            "#/properties/ephemeral",
            fork["roots"]["params"][
                "nullable_property_schema_paths"
            ],
        )
        self.assertIn(
            "#/definitions/Turn/properties/itemsView",
            start["roots"]["result"][
                "optional_property_schema_paths"
            ],
        )
        self.assertIn(
            "#/definitions/Turn/properties/error",
            start["roots"]["result"][
                "nullable_property_schema_paths"
            ],
        )

        records = {
            record["id"]: record for record in index["fixtures"]
        }
        overflow = records[
            "operation:client_request:thread/rollback:params:"
            "uint32-overflow"
        ]
        self.assertNotIn("expected_valid", overflow)
        self.assertEqual(
            overflow["typed_state_boundary"],
            {
                "representable": False,
                "production_diagnostic_expected": False,
                "schema_path": "#/properties/numTurns",
                "format": "uint32",
                "maximum_representable": 4_294_967_295,
                "production_evidence": [
                    "ThreadRollbackParams::numTurns is std::uint32_t",
                    "CodexA11OperationWireTest encodes uint32 maximum exactly",
                    "compile-time numeric_limits<uint32_t> ratchet",
                ],
                "reason": (
                    "Draft-07 format is annotative; the pinned format:uint32 "
                    "value is outside the public typed state and therefore "
                    "cannot produce a runtime codec diagnostic"
                ),
            },
        )
        self.assertEqual(
            records[
                "operation:client_request:thread/rollback:params:"
                "uint32-negative"
            ]["expected_diagnostic_codes"],
            ["minimum"],
        )
        self.assertEqual(
            records[
                "operation:client_request:thread/rollback:params:"
                "uint32-fractional"
            ]["expected_diagnostic_codes"],
            ["type_mismatch"],
        )

    def test_b5_notification_fixture_plan_is_exact_and_complete(self) -> None:
        configured = arguments()
        index = json.loads(
            (configured.fixture_root / "index.json").read_text(
                encoding="utf-8"
            )
        )
        records_by_id = {
            record["id"]: record for record in index["fixtures"]
        }
        b5 = index["a1_1_notifications"]
        assignment_keys = b5["assignment_derived_keys"]
        self.assertEqual(len(assignment_keys), 37)
        self.assertEqual(
            {record["name"] for record in assignment_keys},
            set(tool.B5_SERVER_NOTIFICATION_METHODS),
        )
        self.assertTrue(
            all(
                record == {
                    "category": tool.SERVER_NOTIFICATION,
                    "domain": "ServerNotification",
                    "discriminator_field": "method",
                    "name": record["name"],
                }
                for record in assignment_keys
            )
        )

        root_plan = b5["root_fixture_plan"]
        indexed = b5["indexed_schema_coverage"]
        self.assertEqual(len(root_plan), 37)
        self.assertEqual(len(indexed), 37)
        self.assertEqual(
            sum(record["base_fixture_reused"] for record in root_plan.values()),
            12,
        )
        self.assertEqual(
            sum(
                not record["base_fixture_reused"]
                for record in root_plan.values()
            ),
            25,
        )
        self.assertTrue(
            all(
                record["schema_direction_coverage"]
                and record["directions_exercised"] == ["Decode"]
                and all(record["schema_fixture_facts"].values())
                for record in indexed.values()
            )
        )
        self.assertTrue(
            all(
                set(record["required_reachable_fixture_ids"])
                <= set(records_by_id)
                for record in indexed.values()
            )
        )

        mutation = b5["negative_coverage"]["payload_mutations"]
        self.assertEqual(
            mutation["counts"],
            {
                "base_generated": 25,
                "missing_required": 279,
                "nullable_null": 57,
                "optional_omitted": 65,
                "required_nullable_null": 2,
                "wrong_type": 358,
                "wrong_type_opaque_exclusions": 2,
            },
        )
        self.assertEqual(
            {
                (
                    record["notification"],
                    record["instance_path"],
                    record["schema_path"],
                )
                for record in mutation["opaque_exclusions"]
            },
            {
                (
                    "thread/realtime/itemAdded",
                    "$/params/item",
                    (
                        "#/definitions/"
                        "ThreadRealtimeItemAddedNotification/properties/item"
                    ),
                ),
                (
                    "turn/moderationMetadata",
                    "$/params/metadata",
                    (
                        "#/definitions/"
                        "TurnModerationMetadataNotification/"
                        "properties/metadata"
                    ),
                ),
            },
        )

        existing_bases = {
            tool.SurfaceKey(category, domain, field, name).compact()
            for category, domain, field, name
            in tool.EXISTING_TYPED_FIXTURE_IDENTITIES
            if category == tool.SERVER_NOTIFICATION
            and name in tool.B5_SERVER_NOTIFICATION_METHODS
        }
        self.assertEqual(len(existing_bases), 12)
        for compact_key, plan in root_plan.items():
            with self.subTest(notification=compact_key):
                base = records_by_id[plan["base_fixture_id"]]
                expected_existing = compact_key in existing_bases
                self.assertEqual(
                    base["role"],
                    (
                        "existing_typed_identity"
                        if expected_existing
                        else "server_notification_identity"
                    ),
                )
                self.assertEqual(
                    plan["base_fixture_reused"], expected_existing
                )
                self.assertEqual(
                    plan["future_method_fixture_id"],
                    "method:server_notification:future-unknown",
                )
                self.assertEqual(
                    plan["conflicting_discriminator_fixture_ids"], []
                )
                self.assertTrue(
                    plan["conflicting_discriminator_exclusion"]
                )
                for fixture_id in (
                    plan["optional_omitted_fixture_ids"]
                    + plan["nullable_null_fixture_ids"]
                    + plan["required_nullable_null_fixture_ids"]
                ):
                    record = records_by_id[fixture_id]
                    self.assertNotIn("expected_valid", record)
                    self.assertEqual(
                        record["schema_fixture_coverage"][
                            "directions_exercised"
                        ],
                        ["Decode"],
                    )
                for fixture_id in (
                    plan["missing_required_fixture_ids"]
                    + plan["wrong_type_fixture_ids"]
                ):
                    record = records_by_id[fixture_id]
                    self.assertFalse(record["expected_valid"])
                    self.assertEqual(
                        record["expected_diagnostic_codes"],
                        sorted(
                            set(record["expected_diagnostic_codes"])
                        ),
                    )
                    self.assertTrue(record["expected_diagnostic_codes"])

        settings_key = (
            "server_notification:ServerNotification:"
            "method:thread/settings/updated"
        )
        settings_positive_records = [
            record
            for record in index["fixtures"]
            if record.get("expected_valid", True)
            and record.get("protocol_surface_key", {}).get("name")
            == "thread/settings/updated"
        ]
        self.assertTrue(settings_positive_records)
        for record in settings_positive_records:
            with self.subTest(reasoning_effort_fixture=record["id"]):
                payload = json.loads(
                    (
                        configured.fixture_root / record["file"]
                    ).read_text(encoding="utf-8")
                )
                thread_settings = payload["params"]["threadSettings"]
                effort = thread_settings.get("effort")
                if effort is not None:
                    self.assertEqual(
                        effort,
                        tool.B5_REASONING_EFFORT_FIXTURE_VALUE,
                    )
                collaboration_mode = thread_settings.get(
                    "collaborationMode"
                )
                if isinstance(collaboration_mode, dict):
                    collaboration_settings = collaboration_mode.get(
                        "settings"
                    )
                    if isinstance(collaboration_settings, dict):
                        reasoning_effort = collaboration_settings.get(
                            "reasoning_effort"
                        )
                        if reasoning_effort is not None:
                            self.assertEqual(
                                reasoning_effort,
                                tool.B5_REASONING_EFFORT_FIXTURE_VALUE,
                            )
        self.assertEqual(
            root_plan[settings_key]["base_fixture_id"],
            (
                "baseline:server_notification:ServerNotification:"
                "method:thread/settings/updated"
            ),
        )

        enum_coverage = b5["negative_coverage"]["open_string_enums"]
        self.assertEqual(set(enum_coverage), set(tool.B5_OPEN_STRING_ENUMS))
        self.assertEqual(
            sum(
                len(record["known_value_fixture_ids"])
                for record in enum_coverage.values()
            ),
            7,
        )
        for domain, record in enum_coverage.items():
            with self.subTest(open_enum=domain):
                self.assertEqual(
                    record["directions_exercised"], ["Decode"]
                )
                for fixture_id in record["known_value_fixture_ids"]:
                    self.assertEqual(
                        records_by_id[fixture_id][
                            "schema_fixture_coverage"
                        ]["directions_exercised"],
                        ["Decode"],
                    )
                for fixture_id in (
                    record["future_value_fixture_id"],
                    record["empty_value_fixture_id"],
                ):
                    self.assertEqual(
                        records_by_id[fixture_id][
                            "expected_diagnostic_codes"
                        ],
                        ["enum_mismatch"],
                    )

    def test_a1_2_b2_account_fixture_plan_is_exact_and_complete(
        self,
    ) -> None:
        configured = arguments()
        index = json.loads(
            (configured.fixture_root / "index.json").read_text(
                encoding="utf-8"
            )
        )
        records_by_id = {
            record["id"]: record for record in index["fixtures"]
        }
        b2 = index["a1_2_accounts_authentication"]

        operation_keys = b2["assignment_derived_operation_keys"]
        self.assertEqual(len(operation_keys), 10)
        self.assertEqual(
            {
                record["name"]
                for record in operation_keys
                if record["category"] == tool.CLIENT_REQUEST
            },
            set(tool.A12_B2_ACCOUNT_CLIENT_REQUEST_METHODS),
        )
        self.assertEqual(
            {
                record["name"]
                for record in operation_keys
                if record["category"] == tool.SERVER_REQUEST
            },
            {tool.A12_B2_AUTH_REFRESH_METHOD},
        )
        notification_keys = b2["assignment_derived_notification_keys"]
        self.assertEqual(len(notification_keys), 3)
        self.assertEqual(
            {record["name"] for record in notification_keys},
            set(tool.A12_B2_ACCOUNT_NOTIFICATION_METHODS),
        )
        union_keys = b2["assignment_derived_union_keys"]
        self.assertEqual(len(union_keys), 11)
        self.assertEqual(
            {
                (record["domain"], record["name"])
                for record in union_keys
            },
            {
                (domain, name)
                for domain, names in (
                    tool.A12_B2_ACCOUNT_UNION_IDENTITIES.items()
                )
                for name in names
            },
        )

        indexed = b2["indexed_schema_coverage"]
        self.assertEqual(len(indexed), 24)
        self.assertTrue(
            all(
                record["schema_direction_coverage"]
                and all(record["schema_fixture_facts"].values())
                and set(record.get("required_reachable_fixture_ids", []))
                <= set(records_by_id)
                for record in indexed.values()
            )
        )

        operation_plan = b2["operation_root_fixture_plan"]
        self.assertEqual(len(operation_plan), 10)
        self.assertEqual(
            sum(len(record["roots"]) for record in operation_plan.values()),
            20,
        )
        operation_mutation_counts = {
            field: sum(
                len(root[field])
                for record in operation_plan.values()
                for root in record["roots"].values()
            )
            for field in (
                "missing_required_fixture_ids",
                "nullable_null_fixture_ids",
                "required_nullable_null_fixture_ids",
                "optional_omitted_fixture_ids",
                "wrong_type_fixture_ids",
            )
        }
        self.assertEqual(
            operation_mutation_counts,
            {
                "missing_required_fixture_ids": 44,
                "nullable_null_fixture_ids": 44,
                "required_nullable_null_fixture_ids": 0,
                "optional_omitted_fixture_ids": 45,
                "wrong_type_fixture_ids": 93,
            },
        )
        auth_key = (
            "server_request:ServerRequest:method:"
            "account/chatgptAuthTokens/refresh"
        )
        self.assertEqual(
            {
                root: record["direction"]
                for root, record in operation_plan[auth_key][
                    "roots"
                ].items()
            },
            {"params": "Decode", "result": "Encode"},
        )

        notification_plan = b2["notification_root_fixture_plan"]
        self.assertEqual(len(notification_plan), 3)
        notification_mutations = b2["negative_coverage"][
            "notification_payload_mutations"
        ]
        self.assertEqual(
            notification_mutations,
            {
                "counts": {
                    "base_generated": 3,
                    "missing_required": 16,
                    "nullable_null": 17,
                    "optional_omitted": 17,
                    "required_nullable_null": 0,
                    "wrong_type": 33,
                    "wrong_type_opaque_exclusions": 0,
                },
                "opaque_exclusions": [],
            },
        )

        union_coverage = b2["negative_coverage"][
            "account_login_unions"
        ]
        self.assertEqual(
            union_coverage["family_counts"],
            {
                domain: len(names)
                for domain, names in (
                    tool.A12_B2_ACCOUNT_UNION_IDENTITIES.items()
                )
            },
        )
        self.assertEqual(
            union_coverage["assignment_derived_key_count"], 11
        )
        self.assertEqual(
            {
                domain: record["future_discriminator_fixture_id"]
                for domain, record in union_coverage["families"].items()
            },
            {
                domain: f"union:{domain}:future-unknown"
                for domain in tool.A12_B2_ACCOUNT_UNION_IDENTITIES
            },
        )
        for alternative in union_coverage["alternatives"].values():
            self.assertTrue(
                alternative["missing_discriminator_fixture_ids"]
            )
            self.assertTrue(
                alternative["wrong_discriminator_type_fixture_ids"]
            )
            for fixture_id in (
                alternative["missing_discriminator_fixture_ids"]
                + alternative["missing_required_fixture_ids"]
                + alternative[
                    "wrong_discriminator_type_fixture_ids"
                ]
                + alternative["wrong_nested_type_fixture_ids"]
            ):
                fixture = records_by_id[fixture_id]
                self.assertFalse(fixture["expected_valid"])
                self.assertEqual(
                    fixture["expected_diagnostic_codes"],
                    ["one_of_zero"],
                )

        enum_coverage = b2["negative_coverage"]["open_string_enums"]
        self.assertEqual(
            set(enum_coverage), set(tool.A12_B2_OPEN_STRING_ENUMS)
        )
        self.assertEqual(
            sum(
                len(record["known_value_fixture_ids"])
                for record in enum_coverage.values()
            ),
            48,
        )
        for domain, record in enum_coverage.items():
            self.assertEqual(
                len(record["known_value_fixture_ids"]),
                len(tool.A12_B2_OPEN_STRING_ENUMS[domain]),
            )
            for fixture_id in (
                record["future_value_fixture_id"],
                record["empty_value_fixture_id"],
            ):
                self.assertEqual(
                    records_by_id[fixture_id][
                        "expected_diagnostic_codes"
                    ],
                    record["future_value_schema_diagnostic_codes"],
                )
        self.assertEqual(
            b2["negative_coverage"]["operation_opaque_exclusions"],
            [],
        )

    def test_a1_2_b3_model_fixture_plan_is_exact_and_complete(
        self,
    ) -> None:
        configured = arguments()
        index = json.loads(
            (configured.fixture_root / "index.json").read_text(
                encoding="utf-8"
            )
        )
        records_by_id = {
            record["id"]: record for record in index["fixtures"]
        }
        b3 = index["a1_2_models_providers"]

        operation_keys = b3["assignment_derived_operation_keys"]
        notification_keys = b3["assignment_derived_notification_keys"]
        self.assertEqual(len(operation_keys), 2)
        self.assertEqual(
            {record["name"] for record in operation_keys},
            set(tool.A12_B3_MODEL_CLIENT_REQUEST_METHODS),
        )
        self.assertEqual(len(notification_keys), 3)
        self.assertEqual(
            {record["name"] for record in notification_keys},
            set(tool.A12_B3_MODEL_NOTIFICATION_METHODS),
        )

        indexed = b3["indexed_schema_coverage"]
        self.assertEqual(len(indexed), 5)
        self.assertTrue(
            all(
                record["schema_direction_coverage"]
                and all(record["schema_fixture_facts"].values())
                and set(record.get("required_reachable_fixture_ids", []))
                <= set(records_by_id)
                for record in indexed.values()
            )
        )

        operation_plan = b3["operation_root_fixture_plan"]
        self.assertEqual(len(operation_plan), 2)
        self.assertEqual(
            sum(len(record["roots"]) for record in operation_plan.values()),
            4,
        )
        mutation_counts = {
            field: sum(
                len(root[field])
                for record in operation_plan.values()
                for root in record["roots"].values()
            )
            for field in (
                "missing_required_fixture_ids",
                "nullable_null_fixture_ids",
                "required_nullable_null_fixture_ids",
                "optional_omitted_fixture_ids",
                "wrong_type_fixture_ids",
            )
        }
        self.assertEqual(
            mutation_counts,
            {
                "missing_required_fixture_ids": 19,
                "nullable_null_fixture_ids": 11,
                "required_nullable_null_fixture_ids": 0,
                "optional_omitted_fixture_ids": 15,
                "wrong_type_fixture_ids": 39,
            },
        )
        self.assertTrue(
            all(
                {
                    root: record["direction"]
                    for root, record in operation["roots"].items()
                }
                == {"params": "Encode", "result": "Decode"}
                for operation in operation_plan.values()
            )
        )
        notification_mutations = b3["negative_coverage"][
            "notification_payload_mutations"
        ]
        self.assertEqual(
            notification_mutations,
            {
                "counts": {
                    "base_generated": 2,
                    "missing_required": 20,
                    "nullable_null": 1,
                    "optional_omitted": 1,
                    "required_nullable_null": 0,
                    "wrong_type": 24,
                    "wrong_type_opaque_exclusions": 0,
                },
                "opaque_exclusions": [],
            },
        )

        enum_coverage = b3["negative_coverage"]["open_string_enums"]
        self.assertEqual(
            set(enum_coverage), set(tool.A12_B3_OPEN_STRING_ENUMS)
        )
        self.assertEqual(
            sum(
                len(record["known_value_fixture_ids"])
                for record in enum_coverage.values()
            ),
            4,
        )
        for domain, record in enum_coverage.items():
            self.assertEqual(
                len(record["known_value_fixture_ids"]),
                len(tool.A12_B3_OPEN_STRING_ENUMS[domain]),
            )
            for fixture_id in (
                record["future_value_fixture_id"],
                record["empty_value_fixture_id"],
            ):
                self.assertFalse(records_by_id[fixture_id]["expected_valid"])
                self.assertEqual(
                    records_by_id[fixture_id][
                        "expected_diagnostic_codes"
                    ],
                    record["future_value_schema_diagnostic_codes"],
                )

        boundaries = b3["negative_coverage"]["uint32_boundaries"]
        self.assertEqual(len(boundaries["fixture_ids"]), 5)
        self.assertEqual(
            boundaries["schema_valid_codec_valid"],
            boundaries["fixture_ids"][:2],
        )
        overflow = records_by_id[
            boundaries["schema_valid_typed_unrepresentable"][0]
        ]
        self.assertNotIn("expected_valid", overflow)
        self.assertEqual(
            {
                "format": "uint32",
                "maximum_representable": 4_294_967_295,
                "production_diagnostic_expected": False,
                "representable": False,
                "schema_path": "#/properties/limit",
            },
            {
                key: value
                for key, value in overflow["typed_state_boundary"].items()
                if key != "production_evidence" and key != "reason"
            },
        )
        for fixture_id in boundaries["schema_invalid"]:
            record = records_by_id[fixture_id]
            self.assertFalse(record["expected_valid"])
            self.assertTrue(record["expected_diagnostic_codes"])
        self.assertEqual(
            b3["negative_coverage"]["operation_opaque_exclusions"],
            [],
        )

        empty_arrays = b3["explicit_present_empty_arrays"]
        self.assertEqual(
            empty_arrays["counts"],
            {
                "fixtures": 4,
                "schema_paths": 8,
            },
        )
        empty_fixture_ids = set(empty_arrays["fixture_ids"])
        self.assertEqual(len(empty_fixture_ids), 4)
        self.assertEqual(
            {
                (
                    record["surface_key"]["name"],
                    record["instance_path"],
                    record["schema_path"],
                    record["value_state"],
                )
                for record in empty_arrays["path_evidence"]
            },
            {
                (
                    "model/list",
                    "$/data",
                    "#/properties/data",
                    "present_empty_array",
                ),
                (
                    "model/list",
                    "$/data/0/additionalSpeedTiers",
                    (
                        "#/definitions/Model/properties/"
                        "additionalSpeedTiers"
                    ),
                    "present_empty_array",
                ),
                (
                    "model/list",
                    "$/data/0/inputModalities",
                    "#/definitions/Model/properties/inputModalities",
                    "present_empty_array",
                ),
                (
                    "model/list",
                    "$/data/0/serviceTiers",
                    "#/definitions/Model/properties/serviceTiers",
                    "present_empty_array",
                ),
                (
                    "model/list",
                    "$/data/0/supportedReasoningEfforts",
                    (
                        "#/definitions/Model/properties/"
                        "supportedReasoningEfforts"
                    ),
                    "present_empty_array",
                ),
                (
                    "model/safetyBuffering/updated",
                    "$/params/reasons",
                    (
                        "#/definitions/"
                        "ModelSafetyBufferingUpdatedNotification/"
                        "properties/reasons"
                    ),
                    "present_empty_array",
                ),
                (
                    "model/safetyBuffering/updated",
                    "$/params/useCases",
                    (
                        "#/definitions/"
                        "ModelSafetyBufferingUpdatedNotification/"
                        "properties/useCases"
                    ),
                    "present_empty_array",
                ),
                (
                    "model/verification",
                    "$/params/verifications",
                    (
                        "#/definitions/ModelVerificationNotification/"
                        "properties/verifications"
                    ),
                    "present_empty_array",
                ),
            },
        )
        self.assertTrue(
            all(
                record["fixture_id"] in empty_fixture_ids
                and record["direction"] == "Decode"
                for record in empty_arrays["path_evidence"]
            )
        )
        for fixture_id in empty_fixture_ids:
            fixture = records_by_id[fixture_id]
            self.assertNotIn("expected_valid", fixture)
            self.assertTrue(fixture["validation"]["independent"])
            self.assertEqual(
                fixture["schema_fixture_coverage"][
                    "directions_exercised"
                ],
                ["Decode"],
            )

        model_data = json.loads(
            (
                configured.fixture_root
                / records_by_id[
                    (
                        "operation:client_request:model/list:result:"
                        "explicit-empty-array:data"
                    )
                ]["file"]
            ).read_text(encoding="utf-8")
        )
        self.assertEqual(model_data["data"], [])
        model_collections = json.loads(
            (
                configured.fixture_root
                / records_by_id[
                    (
                        "operation:client_request:model/list:result:"
                        "explicit-empty-array:model-collections"
                    )
                ]["file"]
            ).read_text(encoding="utf-8")
        )["data"][0]
        for field in (
            "additionalSpeedTiers",
            "inputModalities",
            "serviceTiers",
            "supportedReasoningEfforts",
        ):
            self.assertEqual(model_collections[field], [])

        safety = json.loads(
            (
                configured.fixture_root
                / records_by_id[
                    (
                        "baseline:server_notification:"
                        "model/safetyBuffering/updated:"
                        "explicit-empty-array:reasons-use-cases"
                    )
                ]["file"]
            ).read_text(encoding="utf-8")
        )["params"]
        self.assertEqual(safety["reasons"], [])
        self.assertEqual(safety["useCases"], [])
        verification = json.loads(
            (
                configured.fixture_root
                / records_by_id[
                    (
                        "baseline:server_notification:model/verification:"
                        "explicit-empty-array:verifications"
                    )
                ]["file"]
            ).read_text(encoding="utf-8")
        )
        self.assertEqual(verification["params"]["verifications"], [])

    def test_a1_2_b4_configuration_read_fixture_plan_is_exact_and_complete(
        self,
    ) -> None:
        configured = arguments()
        index = json.loads(
            (configured.fixture_root / "index.json").read_text(
                encoding="utf-8"
            )
        )
        records_by_id = {
            record["id"]: record for record in index["fixtures"]
        }
        b4 = index["a1_2_configuration_read"]

        operation_keys = b4["assignment_derived_operation_keys"]
        notification_keys = b4["assignment_derived_notification_keys"]
        union_keys = b4["assignment_derived_union_keys"]
        self.assertEqual(len(operation_keys), 2)
        self.assertEqual(
            {record["name"] for record in operation_keys},
            set(tool.A12_B4_CONFIG_READ_CLIENT_REQUEST_METHODS),
        )
        self.assertEqual(len(notification_keys), 1)
        self.assertEqual(
            {record["name"] for record in notification_keys},
            set(tool.A12_B4_CONFIG_READ_NOTIFICATION_METHODS),
        )
        self.assertEqual(len(union_keys), 8)
        self.assertEqual(
            {record["domain"] for record in union_keys},
            {"ConfigLayerSource"},
        )
        self.assertEqual(
            {record["name"] for record in union_keys},
            set(
                tool.A12_B4_CONFIG_LAYER_SOURCE_IDENTITIES[
                    "ConfigLayerSource"
                ]
            ),
        )

        indexed = b4["indexed_schema_coverage"]
        self.assertEqual(len(indexed), 11)
        self.assertTrue(
            all(
                record["schema_direction_coverage"]
                and all(record["schema_fixture_facts"].values())
                and set(record.get("required_reachable_fixture_ids", []))
                <= set(records_by_id)
                for record in indexed.values()
            )
        )
        operation_plan = b4["operation_root_fixture_plan"]
        self.assertEqual(len(operation_plan), 2)
        self.assertTrue(
            all(
                {
                    root: record["direction"]
                    for root, record in operation["roots"].items()
                }
                == {"params": "Encode", "result": "Decode"}
                for operation in operation_plan.values()
            )
        )
        mutation_counts = {
            field: sum(
                len(root[field])
                for operation in operation_plan.values()
                for root in operation["roots"].values()
            )
            for field in (
                "missing_required_fixture_ids",
                "nullable_null_fixture_ids",
                "required_nullable_null_fixture_ids",
                "optional_omitted_fixture_ids",
                "wrong_type_fixture_ids",
            )
        }
        self.assertEqual(
            mutation_counts,
            {
                "missing_required_fixture_ids": 13,
                "nullable_null_fixture_ids": 54,
                "required_nullable_null_fixture_ids": 1,
                "optional_omitted_fixture_ids": 59,
                "wrong_type_fixture_ids": 81,
            },
        )
        self.assertEqual(
            b4["negative_coverage"]["operation_opaque_exclusions"],
            [
                {
                    "instance_path": "$/layers/0/config",
                    "operation": "config/read",
                    "reason": (
                        "the pinned protocol schema accepts semantically "
                        "arbitrary JSON at this path"
                    ),
                    "root": "result",
                    "schema_path": (
                        "#/definitions/ConfigLayer/properties/config"
                    ),
                }
            ],
        )
        self.assertEqual(
            b4["negative_coverage"]["notification_payload_mutations"],
            {
                "counts": {
                    "base_generated": 1,
                    "missing_required": 9,
                    "nullable_null": 3,
                    "optional_omitted": 3,
                    "required_nullable_null": 0,
                    "wrong_type": 12,
                    "wrong_type_opaque_exclusions": 0,
                },
                "opaque_exclusions": [],
            },
        )

        union_coverage = b4["negative_coverage"][
            "config_layer_source_unions"
        ]
        self.assertEqual(union_coverage["assignment_derived_key_count"], 8)
        self.assertEqual(
            union_coverage["family_counts"],
            {"ConfigLayerSource": 8},
        )
        self.assertEqual(
            set(union_coverage["alternatives"]),
            {
                (
                    "tagged_union_discriminator:ConfigLayerSource:"
                    f"type:{name}"
                )
                for name in tool.A12_B4_CONFIG_LAYER_SOURCE_IDENTITIES[
                    "ConfigLayerSource"
                ]
            },
        )
        family = union_coverage["families"]["ConfigLayerSource"]
        self.assertEqual(family["known_alternative_count"], 8)
        self.assertEqual(
            records_by_id[family["future_discriminator_fixture_id"]][
                "expected_diagnostic_codes"
            ],
            ["one_of_zero"],
        )
        self.assertEqual(
            records_by_id[family["wrong_outer_shape_fixture_id"]][
                "expected_diagnostic_codes"
            ],
            ["one_of_zero"],
        )
        for alternative in union_coverage["alternatives"].values():
            for fixture_id in (
                alternative["missing_discriminator_fixture_ids"]
                + alternative["missing_required_fixture_ids"]
                + alternative[
                    "wrong_discriminator_type_fixture_ids"
                ]
                + alternative["wrong_nested_type_fixture_ids"]
            ):
                fixture = records_by_id[fixture_id]
                self.assertFalse(fixture["expected_valid"])
                self.assertTrue(
                    fixture["expected_diagnostic_codes"]
                )

        helper = b4["negative_coverage"]["helper_unions"][
            "ForcedChatgptWorkspaceIds"
        ]
        self.assertEqual(
            set(helper["known_branch_fixture_ids"]),
            set(tool.A12_B4_FORCED_WORKSPACE_HELPER_FIXTURE_IDS),
        )
        self.assertEqual(
            records_by_id[helper["wrong_array_item_fixture_id"]][
                "expected_diagnostic_codes"
            ],
            ["any_of_zero"],
        )

        enum_coverage = b4["negative_coverage"]["open_string_enums"]
        self.assertEqual(
            set(enum_coverage), set(tool.A12_B4_OPEN_STRING_ENUMS)
        )
        self.assertEqual(
            sum(
                len(record["known_value_fixture_ids"])
                for record in enum_coverage.values()
            ),
            17,
        )
        for domain, record in enum_coverage.items():
            self.assertEqual(
                len(record["known_value_fixture_ids"]),
                len(tool.A12_B4_OPEN_STRING_ENUMS[domain]),
            )
            for fixture_id in (
                record["future_value_fixture_id"],
                record["empty_value_fixture_id"],
            ):
                self.assertFalse(records_by_id[fixture_id]["expected_valid"])
                self.assertEqual(
                    records_by_id[fixture_id][
                        "expected_diagnostic_codes"
                    ],
                    record["future_value_schema_diagnostic_codes"],
                )

        positive = b4["positive_coverage"]
        empty_arrays = positive["explicit_present_empty_arrays"]
        self.assertEqual(
            empty_arrays["counts"],
            {"fixtures": 3, "schema_paths": 8},
        )
        self.assertTrue(
            all(
                record["fixture_id"] in records_by_id
                and record["direction"] == "Decode"
                and record["value_state"] == "present_empty_array"
                for record in empty_arrays["path_evidence"]
            )
        )
        self.assertEqual(
            {
                record["instance_path"]
                for record in empty_arrays["path_evidence"]
            },
            {
                "$",
                "$/config/sandbox_workspace_write/writable_roots",
                "$/config/tools/web_search/allowed_domains",
                "$/layers",
                "$/requirements/allowedApprovalPolicies",
                "$/requirements/allowedSandboxModes",
                "$/requirements/allowedWebSearchModes",
                (
                    "$/requirements/"
                    "allowedWindowsSandboxImplementations"
                ),
            },
        )
        opaque = positive["authorized_opaque_json"]
        self.assertEqual(opaque["path_count"], 4)
        self.assertEqual(
            {
                record["closure_schema_path"]
                for record in opaque["path_evidence"]
            },
            {
                (
                    "#/definitions/v2/AnalyticsConfig/"
                    "additionalProperties"
                ),
                "#/definitions/v2/Config/additionalProperties",
                (
                    "#/definitions/v2/Config/properties/desktop/"
                    "additionalProperties"
                ),
                "#/definitions/v2/ConfigLayer/properties/config",
            },
        )
        opaque_fixture = json.loads(
            (
                configured.fixture_root
                / records_by_id[opaque["fixture_id"]]["file"]
            ).read_text(encoding="utf-8")
        )
        self.assertIn(
            "syntheticConfigExtension", opaque_fixture["config"]
        )
        self.assertIn(
            "syntheticDesktopExtension",
            opaque_fixture["config"]["desktop"],
        )
        self.assertIn(
            "syntheticAnalyticsExtension",
            opaque_fixture["config"]["analytics"],
        )
        self.assertIsInstance(
            opaque_fixture["layers"][0]["config"], dict
        )

        defaults = positive["defaulted_fields"]
        self.assertEqual(defaults["count"], 6)
        for record in defaults["path_evidence"]:
            self.assertIn(record["present_fixture_id"], records_by_id)
            self.assertIn(record["omitted_fixture_id"], records_by_id)

        boundaries = positive["numeric_boundaries"]
        self.assertEqual(
            len(boundaries["int64"]["fixture_ids"]), 4
        )
        self.assertEqual(
            len(
                boundaries["int64"][
                    "schema_valid_typed_unrepresentable"
                ]
            ),
            2,
        )
        self.assertEqual(
            len(boundaries["uint"]["fixture_ids"]), 3
        )
        self.assertEqual(
            len(
                boundaries["uint"][
                    "schema_valid_typed_unrepresentable"
                ]
            ),
            1,
        )
        for fixture_id in (
            boundaries["int64"]["schema_invalid"]
        ):
            fixture = records_by_id[fixture_id]
            self.assertFalse(fixture["expected_valid"])
            self.assertEqual(
                fixture["expected_diagnostic_codes"],
                ["type_mismatch"],
            )
        for fixture_id in boundaries["uint"]["schema_invalid"]:
            fixture = records_by_id[fixture_id]
            self.assertFalse(fixture["expected_valid"])
            self.assertEqual(
                fixture["expected_diagnostic_codes"],
                ["one_of_zero"],
            )
        for fixture_id in (
            boundaries["int64"][
                "schema_valid_typed_unrepresentable"
            ]
            + boundaries["uint"][
                "schema_valid_typed_unrepresentable"
            ]
        ):
            fixture = records_by_id[fixture_id]
            self.assertNotIn("expected_valid", fixture)
            self.assertFalse(
                fixture["typed_state_boundary"]["representable"]
            )

        def diagnostic_histogram(
            selected: list[dict[str, object]],
        ) -> dict[tuple[str, ...], int]:
            counts: dict[tuple[str, ...], int] = {}
            for fixture in selected:
                codes = tuple(fixture["expected_diagnostic_codes"])
                counts[codes] = counts.get(codes, 0) + 1
            return counts

        operation_names = set(
            tool.A12_B4_CONFIG_READ_CLIENT_REQUEST_METHODS
        )
        self.assertEqual(
            diagnostic_histogram(
                [
                    fixture
                    for fixture in index["fixtures"]
                    if not fixture.get("expected_valid", True)
                    and fixture.get("protocol_surface_key", {}).get(
                        "name"
                    )
                    in operation_names
                ]
            ),
            {
                ("any_of_zero",): 53,
                ("one_of_zero",): 14,
                ("required_missing",): 7,
                ("type_mismatch",): 22,
            },
        )
        self.assertEqual(
            diagnostic_histogram(
                [
                    fixture
                    for fixture in index["fixtures"]
                    if not fixture.get("expected_valid", True)
                    and fixture.get("protocol_surface_key", {}).get(
                        "name"
                    )
                    == "configWarning"
                ]
            ),
            {("one_of_zero",): 23},
        )
        self.assertEqual(
            diagnostic_histogram(
                [
                    fixture
                    for fixture in index["fixtures"]
                    if not fixture.get("expected_valid", True)
                    and fixture.get("protocol_surface_key", {}).get(
                        "domain"
                    )
                    == "ConfigLayerSource"
                ]
            ),
            {("one_of_zero",): 33},
        )

    def test_a1_2_b5_configuration_mutation_fixture_plan_is_exact(
        self,
    ) -> None:
        configured = arguments()
        index = json.loads(
            (configured.fixture_root / "index.json").read_text(
                encoding="utf-8"
            )
        )
        records_by_id = {
            record["id"]: record for record in index["fixtures"]
        }
        b5 = index["a1_2_configuration_mutation_features"]
        operation_keys = b5["assignment_derived_operation_keys"]
        self.assertEqual(len(operation_keys), 5)
        self.assertEqual(
            {record["name"] for record in operation_keys},
            set(
                tool.A12_B5_CONFIGURATION_MUTATION_CLIENT_REQUEST_METHODS
            ),
        )
        self.assertTrue(
            all(
                record["category"] == "client_request"
                and record["domain"] == "ClientRequest"
                and record["discriminator_field"] == "method"
                for record in operation_keys
            )
        )

        indexed = b5["indexed_schema_coverage"]
        self.assertEqual(len(indexed), 5)
        self.assertTrue(
            all(
                record["schema_direction_coverage"]
                and set(record["directions_exercised"])
                == {"Decode", "Encode"}
                and all(record["schema_fixture_facts"].values())
                and set(record["required_reachable_fixture_ids"])
                <= set(records_by_id)
                for record in indexed.values()
            )
        )
        operation_plan = b5["operation_root_fixture_plan"]
        self.assertEqual(len(operation_plan), 5)
        self.assertEqual(
            sum(
                len(operation["roots"])
                for operation in operation_plan.values()
            ),
            10,
        )
        self.assertTrue(
            all(
                {
                    name: root["direction"]
                    for name, root in operation["roots"].items()
                }
                == {"params": "Encode", "result": "Decode"}
                for operation in operation_plan.values()
            )
        )
        mutation_counts = {
            field: sum(
                len(root[field])
                for operation in operation_plan.values()
                for root in operation["roots"].values()
            )
            for field in (
                "missing_required_fixture_ids",
                "nullable_null_fixture_ids",
                "required_nullable_null_fixture_ids",
                "optional_omitted_fixture_ids",
                "wrong_type_fixture_ids",
            )
        }
        self.assertEqual(
            mutation_counts,
            {
                "missing_required_fixture_ids": 36,
                "nullable_null_fixture_ids": 13,
                "required_nullable_null_fixture_ids": 4,
                "optional_omitted_fixture_ids": 14,
                "wrong_type_fixture_ids": 50,
            },
        )

        opaque_exclusions = b5["negative_coverage"][
            "operation_opaque_exclusions"
        ]
        self.assertEqual(len(opaque_exclusions), 4)
        self.assertEqual(
            {
                (
                    record["operation"],
                    record["root"],
                    record["instance_path"],
                    record["schema_path"],
                )
                for record in opaque_exclusions
            },
            {
                (
                    "config/batchWrite",
                    "params",
                    "$/edits/0/value",
                    "#/definitions/ConfigEdit/properties/value",
                ),
                (
                    "config/batchWrite",
                    "result",
                    "$/overriddenMetadata/effectiveValue",
                    (
                        "#/definitions/OverriddenMetadata/properties/"
                        "effectiveValue"
                    ),
                ),
                (
                    "config/value/write",
                    "params",
                    "$/value",
                    "#/properties/value",
                ),
                (
                    "config/value/write",
                    "result",
                    "$/overriddenMetadata/effectiveValue",
                    (
                        "#/definitions/OverriddenMetadata/properties/"
                        "effectiveValue"
                    ),
                ),
            },
        )

        contracts = json.loads(
            configured.contracts.read_text(encoding="utf-8")
        )["contracts"]
        selected_contracts = [
            record
            for record in contracts
            if record["surface_key"]["name"]
            in tool.A12_B5_CONFIGURATION_MUTATION_CLIENT_REQUEST_METHODS
        ]
        self.assertEqual(len(selected_contracts), 5)
        self.assertEqual(
            {
                kind: sum(
                    record["result_contract_kind"] == kind
                    for record in selected_contracts
                )
                for kind in ("Concrete", "Unit")
            },
            {"Concrete": 4, "Unit": 1},
        )

        unit = b5["negative_coverage"]["unit_result_invariant"]
        self.assertEqual(
            {
                "method": "config/mcpServer/reload",
                "parameter_type_identity": "Unit",
                "result_type_identity": "Unit",
                "result_schema_type_identity": (
                    "McpServerRefreshResponse"
                ),
                "result_contract_kind": "Unit",
                "property_count": 0,
                "future_property_addition_fails_generation": True,
            },
            {
                key: unit[key]
                for key in (
                    "method",
                    "parameter_type_identity",
                    "result_type_identity",
                    "result_schema_type_identity",
                    "result_contract_kind",
                    "property_count",
                    "future_property_addition_fails_generation",
                )
            },
        )
        unit_params = json.loads(
            (
                configured.fixture_root
                / records_by_id[unit["encoded_params_fixture_id"]]["file"]
            ).read_text(encoding="utf-8")
        )
        unit_result = json.loads(
            (
                configured.fixture_root
                / records_by_id[unit["empty_result_fixture_id"]]["file"]
            ).read_text(encoding="utf-8")
        )
        self.assertIsNone(unit_params)
        self.assertEqual(unit_result, {})
        mutated_unit_schema = copy.deepcopy(
            unit["reviewed_result_schema"]
        )
        mutated_unit_schema["properties"] = {
            "futureProperty": {"type": "boolean"}
        }
        with self.assertRaisesRegex(
            tool.FixtureError,
            "Unit result no longer resolves",
        ):
            tool.require_reviewed_unit_result_schema(
                method=unit["method"],
                actual=mutated_unit_schema,
                expected=unit["reviewed_result_schema"],
            )

        positive = b5["positive_coverage"]
        opaque = positive["authorized_opaque_json"]
        expected_json_kinds = [
            "null",
            "boolean",
            "integer",
            "number",
            "string",
            "array",
            "object",
        ]
        self.assertEqual(opaque["path_count"], 3)
        self.assertEqual(
            opaque["json_value_kinds"], expected_json_kinds
        )
        self.assertEqual(
            {
                record["closure_schema_path"]
                for record in opaque["path_evidence"]
            },
            {
                "#/definitions/v2/ConfigEdit/properties/value",
                (
                    "#/definitions/v2/ConfigValueWriteParams/"
                    "properties/value"
                ),
                (
                    "#/definitions/v2/OverriddenMetadata/properties/"
                    "effectiveValue"
                ),
            },
        )
        self.assertTrue(
            all(
                record["sensitivity"]
                == "PotentialCredentialBearingConfiguration"
                and record["json_value_kinds"] == expected_json_kinds
                for record in opaque["path_evidence"]
            )
        )
        serialized_b5_evidence = json.dumps(b5, sort_keys=True)
        for prohibited_value in (
            "synthetic-config-value",
            "synthetic.config.key",
            "synthetic override message",
        ):
            self.assertNotIn(prohibited_value, serialized_b5_evidence)

        def json_kind(value: object) -> str:
            if value is None:
                return "null"
            if isinstance(value, bool):
                return "boolean"
            if isinstance(value, int):
                return "integer"
            if isinstance(value, float):
                return "number"
            if isinstance(value, str):
                return "string"
            if isinstance(value, list):
                return "array"
            if isinstance(value, dict):
                return "object"
            self.fail(f"unexpected fixture JSON kind: {type(value)}")

        batch_opaque = next(
            record
            for record in opaque["path_evidence"]
            if record["closure_schema_path"].endswith(
                "ConfigEdit/properties/value"
            )
        )
        batch_payload = json.loads(
            (
                configured.fixture_root
                / records_by_id[batch_opaque["fixture_ids"][0]]["file"]
            ).read_text(encoding="utf-8")
        )
        self.assertEqual(
            [json_kind(edit["value"]) for edit in batch_payload["edits"]],
            expected_json_kinds,
        )
        self.assertEqual(
            [edit["keyPath"] for edit in batch_payload["edits"]],
            [
                f"synthetic.order.{index}"
                for index in range(len(expected_json_kinds))
            ],
        )

        value_opaque = next(
            record
            for record in opaque["path_evidence"]
            if "ConfigValueWriteParams" in record["closure_schema_path"]
        )
        self.assertEqual(
            [
                json_kind(
                    json.loads(
                        (
                            configured.fixture_root
                            / records_by_id[fixture_id]["file"]
                        ).read_text(encoding="utf-8")
                    )["value"]
                )
                for fixture_id in value_opaque["fixture_ids"]
            ],
            expected_json_kinds,
        )
        effective_opaque = next(
            record
            for record in opaque["path_evidence"]
            if "OverriddenMetadata" in record["closure_schema_path"]
        )
        for fixture_ids in effective_opaque[
            "fixture_ids_by_operation"
        ].values():
            self.assertEqual(
                [
                    json_kind(
                        json.loads(
                            (
                                configured.fixture_root
                                / records_by_id[fixture_id]["file"]
                            ).read_text(encoding="utf-8")
                        )["overriddenMetadata"]["effectiveValue"]
                    )
                    for fixture_id in fixture_ids
                ],
                expected_json_kinds,
            )

        empty_containers = positive["explicit_empty_containers"]
        self.assertEqual(
            empty_containers["counts"],
            {"fixtures": 4, "schema_paths": 4},
        )
        self.assertTrue(
            all(
                record["fixture_id"] in records_by_id
                and record["value_state"]
                in {"present_empty_array", "present_empty_map"}
                for record in empty_containers["path_evidence"]
            )
        )
        self.assertEqual(
            positive["ordered_config_edits"],
            {
                "fixture_id": batch_opaque["fixture_ids"][0],
                "edit_count": len(expected_json_kinds),
                "order_preserved": True,
            },
        )

        stages = positive["all_known_feature_stages"]
        self.assertEqual(
            stages["known_values"],
            list(
                tool.A12_B5_OPEN_STRING_ENUMS[
                    "ExperimentalFeatureStage"
                ]
            ),
        )
        stage_payload = json.loads(
            (
                configured.fixture_root
                / records_by_id[stages["fixture_id"]]["file"]
            ).read_text(encoding="utf-8")
        )
        self.assertEqual(
            [record["stage"] for record in stage_payload["data"]],
            stages["known_values"],
        )

        defaults = positive["default_semantics"]
        self.assertEqual(
            defaults["json_schema_default_keyword_count"], 0
        )
        self.assertEqual(
            len(defaults["described_server_default_paths"]), 3
        )
        for record in defaults["described_server_default_paths"]:
            self.assertFalse(record["schema_default_keyword_present"])
            self.assertFalse(record["client_default_invented"])
            self.assertIn(record["omitted_fixture_id"], records_by_id)

        enum_coverage = b5["negative_coverage"]["open_string_enums"]
        self.assertEqual(
            set(enum_coverage), set(tool.A12_B5_OPEN_STRING_ENUMS)
        )
        self.assertEqual(
            sum(
                len(record["known_value_fixture_ids"])
                for record in enum_coverage.values()
            ),
            9,
        )
        for domain, record in enum_coverage.items():
            self.assertEqual(
                len(record["known_value_fixture_ids"]),
                len(tool.A12_B5_OPEN_STRING_ENUMS[domain]),
            )
            for fixture_id in (
                record["future_value_fixture_id"],
                record["empty_value_fixture_id"],
            ):
                self.assertEqual(
                    records_by_id[fixture_id][
                        "expected_diagnostic_codes"
                    ],
                    record["future_value_schema_diagnostic_codes"],
                )

        boundaries = b5["negative_coverage"]["uint32_boundaries"]
        self.assertEqual(len(boundaries["fixture_ids"]), 5)
        self.assertEqual(
            boundaries["schema_valid_codec_valid"],
            boundaries["fixture_ids"][:2],
        )
        overflow = records_by_id[
            boundaries["schema_valid_typed_unrepresentable"][0]
        ]
        self.assertFalse(
            overflow["typed_state_boundary"]["representable"]
        )
        self.assertEqual(
            records_by_id[boundaries["schema_invalid"][0]][
                "expected_diagnostic_codes"
            ],
            ["minimum"],
        )
        self.assertEqual(
            records_by_id[boundaries["schema_invalid"][1]][
                "expected_diagnostic_codes"
            ],
            ["type_mismatch"],
        )
        diagnostic_counts: dict[tuple[str, ...], int] = {}
        operation_names = set(
            tool.A12_B5_CONFIGURATION_MUTATION_CLIENT_REQUEST_METHODS
        )
        for fixture in index["fixtures"]:
            if (
                fixture.get("expected_valid", True)
                or fixture.get("protocol_surface_key", {}).get("name")
                not in operation_names
            ):
                continue
            codes = tuple(fixture["expected_diagnostic_codes"])
            diagnostic_counts[codes] = (
                diagnostic_counts.get(codes, 0) + 1
            )
        self.assertEqual(
            diagnostic_counts,
            {
                ("any_of_zero",): 32,
                ("minimum",): 1,
                ("one_of_zero",): 1,
                ("required_missing",): 20,
                ("type_mismatch",): 34,
            },
        )

    def test_fixture_generator_has_no_production_decoder_inputs(self) -> None:
        source = (
            REPOSITORY_ROOT / "tools/codex/app_server_fixtures.py"
        ).read_text(encoding="utf-8")
        for prohibited in (
            "ProtocolSurfaceRegistryData.inc",
            "EventDecoder.cpp",
            "typed/Events.h",
            "BackendCore.cpp",
        ):
            self.assertNotIn(prohibited, source)

    def test_committed_corpus_is_deterministic_current_and_valid(self) -> None:
        configured = arguments()
        first, first_index = generated_outputs_snapshot()
        tool.check_outputs(first, (configured.fixture_root,))
        tool.validate_committed(configured)

        self.assertEqual(
            first_index["counts"],
            {
                "total": 4815,
                "positive": 1881,
                "negative": 2934,
                "by_role": {
                    "client_request_params": 87,
                    "client_request_result": 87,
                    "existing_typed_identity": 34,
                    "malformed_known": 1,
                    "malformed_known_conflicting_discriminators": 2,
                    "malformed_known_empty_string": 1,
                    "malformed_known_missing_discriminator": 100,
                    "malformed_known_missing_required": 169,
                    "malformed_known_wrong_discriminator_type": 100,
                    "malformed_known_wrong_outer_shape": 4,
                    "malformed_known_wrong_type": 328,
                    "nested_union_failure": 4,
                    "notification_explicit_empty_array": 2,
                    "notification_missing_required": 324,
                    "notification_nullable_null": 80,
                    "notification_numeric_boundary": 1,
                    "notification_numeric_boundary_invalid": 2,
                    "notification_optional_omitted": 86,
                    "notification_pinned_format_unrepresentable": 1,
                    "notification_wrong_type": 427,
                    "open_enum_known_value": 171,
                    "operation_empty_aggregate": 2,
                    "operation_explicit_empty_array": 6,
                    "operation_explicit_empty_map": 2,
                    "operation_helper_union_branch": 13,
                    "operation_known_enum_values": 1,
                    "operation_missing_required": 411,
                    "operation_nullable_null": 377,
                    "operation_numeric_boundary": 8,
                    "operation_numeric_boundary_invalid": 8,
                    "operation_opaque_value": 26,
                    "operation_optional_omitted": 409,
                    "operation_pinned_format_unrepresentable": 5,
                    "operation_wrong_type": 885,
                    "server_notification_identity": 31,
                    "server_request_params": 10,
                    "server_request_response": 10,
                    "union_branch": 127,
                    "union_branch_supplement": 33,
                    "union_nullable_null": 130,
                    "union_optional_omitted": 142,
                    "unknown_discriminator": 27,
                    "unknown_enum_value": 137,
                    "unknown_method": 4,
                },
            },
        )
        self.assertEqual(
            first_index["mutation_counts"],
            {
                "selected_branch_required_locations": 19229,
                "required_locations": 19229,
                "required_field_removals_rejected": 19229,
                "wrong_type_mutations_rejected": 19051,
                "wrong_type_unconstrained_exclusions": 178,
                "alternative_branch_acceptances": 1,
                "optional_present_locations": 19631,
                "globally_optional_locations": 19631,
                "optional_omissions_accepted": 19631,
                "optional_cross_fragment_exclusions": 0,
            },
        )
        elicitation = next(
            fixture
            for fixture in first_index["fixtures"]
            if fixture["id"]
            == "operation:server_request:mcpServer/elicitation/request:params"
        )
        self.assertEqual(
            elicitation["validation"]["alternative_branch_removals"],
            [
                {
                    "instance_path": (
                        "$/requestedSchema/properties/syntheticKey/enum"
                    ),
                    "reason": (
                        "root schema accepts an alternative branch; the "
                        "selected fixture branch rejects the removal"
                    ),
                }
            ],
        )
        enum_removal = next(
            removal
            for removal in elicitation["validation"]["removals"]
            if removal["instance_path"]
            == "$/requestedSchema/properties/syntheticKey/enum"
        )
        self.assertEqual(
            enum_removal,
            {
                "instance_path": (
                    "$/requestedSchema/properties/syntheticKey/enum"
                ),
                "diagnostic_codes": ["required_missing"],
                "validation_scope": "selected_schema_fragment",
            },
        )
        completeness_path = (
            configured.evidence_root / "schema-completeness-evidence.json"
        )
        completeness = json.loads(first[completeness_path].decode("utf-8"))
        self.assertEqual(completeness["counts"]["surface_identities"], 387)
        self.assertEqual(
            completeness["counts"]["identities_with_positive_fixtures"], 270
        )
        self.assertEqual(
            completeness["counts"]["facts_true_by_field"],
            {
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
        )
        self.assertEqual(len(completeness["records"]), 387)
        serialized_completeness = json.dumps(completeness, sort_keys=True)
        for prohibited in (
            "direction_assertions_exercised",
            "runtime_decoder_matches_registry",
            "opaque_fields_declared",
            "no_known_schema_fields_dropped",
        ):
            self.assertNotIn(prohibited, serialized_completeness)
        self.assertNotIn("typed_schema_status", serialized_completeness)

        exclusions = [
            mutation
            for record in first_index["fixtures"]
            if record.get("expected_valid", True)
            and "wrong_types" in record["validation"]
            for mutation in record["validation"]["wrong_types"]
            if mutation["exclusion"] is not None
        ]
        # The two new B5 protocol-opaque base paths are retained in the
        # detailed index. Supplemental notification records use the compact
        # mutation-evidence form and remain covered by the global count of
        # 178 after the A1.2 B5 configuration-mutation closure.
        self.assertEqual(len(exclusions), 63)
        self.assertTrue(
            all(
                mutation == {
                    "instance_path": mutation["instance_path"],
                    "mutation_found": False,
                    "replacement_type": None,
                    "diagnostic_codes": [],
                    "exclusion": "schema accepts every JSON type candidate",
                }
                for mutation in exclusions
            )
        )

    def test_generation_is_byte_identical_across_process_hash_seeds(self) -> None:
        self.assertEqual(os.environ.get("PYTHONHASHSEED"), "1")
        configured = arguments()
        first, _ = generated_outputs_snapshot()
        with tempfile.TemporaryDirectory(
            prefix="snodec-codex-fixture-process-determinism-"
        ) as raw:
            temporary = Path(raw)

            def generate(label: str, hash_seed: str) -> dict[str, bytes]:
                fixture_root = temporary / label / "fixtures"
                evidence_root = temporary / label / "evidence"
                environment = os.environ.copy()
                environment["PYTHONHASHSEED"] = hash_seed
                subprocess.run(
                    [
                        sys.executable,
                        str(
                            REPOSITORY_ROOT
                            / "tools/codex/app_server_fixtures.py"
                        ),
                        "generate",
                        "--fixture-root",
                        str(fixture_root),
                        "--evidence-root",
                        str(evidence_root),
                    ],
                    cwd=REPOSITORY_ROOT,
                    env=environment,
                    check=True,
                    capture_output=True,
                    text=True,
                )
                return {
                    f"{root_name}/{path.relative_to(root).as_posix()}": path.read_bytes()
                    for root_name, root in (
                        ("fixtures", fixture_root),
                        ("evidence", evidence_root),
                    )
                    for path in sorted(root.rglob("*"))
                    if path.is_file()
                }

            self.assertEqual(
                normalized_generated_outputs(first, configured),
                generate("seed-2", "8675309"),
            )

    def test_output_guard_rejects_missing_extra_and_edited_files(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="snodec-codex-fixture-output-guard-"
        ) as raw:
            root = Path(raw)
            expected_path = root / "managed/fixture.json"
            expected = {expected_path: b'{"valid":true}\n'}

            with self.assertRaisesRegex(tool.FixtureError, "missing="):
                tool.check_outputs(expected, (root / "managed",))

            expected_path.parent.mkdir(parents=True)
            expected_path.write_bytes(expected[expected_path])
            extra_path = root / "managed/extra.json"
            extra_path.write_text("{}\n", encoding="utf-8")
            with self.assertRaisesRegex(tool.FixtureError, "extra="):
                tool.check_outputs(expected, (root / "managed",))

            extra_path.unlink()
            expected_path.write_text('{"valid":false}\n', encoding="utf-8")
            with self.assertRaisesRegex(tool.FixtureError, "changed="):
                tool.check_outputs(expected, (root / "managed",))

    def test_validation_rejects_wrong_exact_surface_identity(self) -> None:
        committed = json.loads(
            (arguments().fixture_root / "index.json").read_text(
                encoding="utf-8"
            )
        )
        mutated = copy.deepcopy(committed)
        record = next(
            fixture
            for fixture in mutated["fixtures"]
            if "protocol_surface_key" in fixture
        )
        record["protocol_surface_key"]["name"] = "future/not-a-real-method"

        with tempfile.TemporaryDirectory(
            prefix="snodec-codex-fixture-wrong-identity-"
        ) as raw:
            temporary_root = Path(raw)
            temporary_root.mkdir(exist_ok=True)
            (temporary_root / "index.json").write_bytes(
                tool.encoded_json(mutated)
            )
            with self.assertRaisesRegex(
                tool.FixtureError,
                "fixture references unknown protocol surface identity",
            ):
                tool.validate_committed(arguments(temporary_root))

    def test_validation_rejects_source_provenance_mismatch(self) -> None:
        committed = json.loads(
            (arguments().fixture_root / "index.json").read_text(
                encoding="utf-8"
            )
        )
        committed["sources"]["surface_manifest_sha256"] = "0" * 64

        with tempfile.TemporaryDirectory(
            prefix="snodec-codex-fixture-stale-source-"
        ) as raw:
            temporary_root = Path(raw)
            (temporary_root / "index.json").write_bytes(
                tool.encoded_json(committed)
            )
            with self.assertRaisesRegex(
                tool.FixtureError,
                "source/provenance hashes are stale",
            ):
                tool.validate_committed(arguments(temporary_root))

    def test_validation_rejects_stale_indexed_schema_coverage(self) -> None:
        configured = arguments()
        with tempfile.TemporaryDirectory(
            prefix="snodec-codex-fixture-stale-schema-coverage-"
        ) as raw:
            temporary_root = Path(raw) / "fixtures"
            shutil.copytree(configured.fixture_root, temporary_root)
            index_path = temporary_root / "index.json"
            index = json.loads(index_path.read_text(encoding="utf-8"))
            record = next(
                fixture
                for fixture in index["fixtures"]
                if fixture["id"]
                == (
                    "union:UserInput:image:"
                    "nullable-null:detail"
                )
            )
            # Validation is intentionally order-independent. Put the planted
            # record first so this negative guard tests the same diagnostic
            # without revalidating the unchanged corpus before reaching it.
            index["fixtures"].remove(record)
            index["fixtures"].insert(0, record)
            self.assertTrue(
                record["schema_fixture_coverage"][
                    "nullable_null_schema_paths"
                ]
            )
            record["schema_fixture_coverage"][
                "nullable_null_schema_paths"
            ] = []
            index_path.write_bytes(tool.encoded_json(index))

            with self.assertRaisesRegex(
                tool.FixtureError,
                "committed schema fixture coverage is stale",
            ):
                tool.validate_committed(arguments(temporary_root))


if __name__ == "__main__":
    unittest.main()
