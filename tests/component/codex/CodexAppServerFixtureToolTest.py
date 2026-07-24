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


class AppServerFixtureToolTest(unittest.TestCase):
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

    def test_committed_corpus_is_deterministic_current_and_valid(self) -> None:
        configured = arguments()
        first, first_index = tool.generated_outputs(configured)
        second, second_index = tool.generated_outputs(configured)
        self.assertEqual(first, second)
        self.assertEqual(first_index, second_index)
        tool.check_outputs(first, (configured.fixture_root,))
        tool.validate_committed(configured)

        self.assertEqual(
            first_index["counts"],
            {
                "total": 2915,
                "positive": 1259,
                "negative": 1656,
                "by_role": {
                    "client_request_params": 87,
                    "client_request_result": 87,
                    "existing_typed_identity": 34,
                    "malformed_known": 1,
                    "malformed_known_conflicting_discriminators": 2,
                    "malformed_known_empty_string": 1,
                    "malformed_known_missing_discriminator": 81,
                    "malformed_known_missing_required": 151,
                    "malformed_known_wrong_discriminator_type": 81,
                    "malformed_known_wrong_outer_shape": 3,
                    "malformed_known_wrong_type": 304,
                    "nested_union_failure": 4,
                    "open_enum_known_value": 86,
                    "operation_helper_union_branch": 10,
                    "operation_missing_required": 299,
                    "operation_nullable_null": 250,
                    "operation_numeric_boundary": 2,
                    "operation_numeric_boundary_invalid": 2,
                    "operation_opaque_value": 3,
                    "operation_optional_omitted": 276,
                    "operation_pinned_format_unrepresentable": 1,
                    "operation_wrong_type": 621,
                    "server_request_params": 10,
                    "server_request_response": 10,
                    "union_branch": 108,
                    "union_branch_supplement": 33,
                    "union_nullable_null": 126,
                    "union_optional_omitted": 136,
                    "unknown_discriminator": 23,
                    "unknown_enum_value": 79,
                    "unknown_method": 4,
                },
            },
        )
        self.assertEqual(
            first_index["mutation_counts"],
            {
                "selected_branch_required_locations": 13359,
                "required_locations": 13359,
                "required_field_removals_rejected": 13359,
                "wrong_type_mutations_rejected": 13298,
                "wrong_type_unconstrained_exclusions": 61,
                "alternative_branch_acceptances": 1,
                "optional_present_locations": 12058,
                "globally_optional_locations": 12058,
                "optional_omissions_accepted": 12058,
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
            completeness["counts"]["identities_with_positive_fixtures"], 220
        )
        self.assertEqual(
            completeness["counts"]["facts_true_by_field"],
            {
                "authoritative_root_association": 220,
                "fixture_current": 220,
                "independently_schema_validated": 220,
                "nullable_semantics_exercised": 130,
                "optional_omitted_exercised": 220,
                "optional_present_exercised": 220,
                "positive_fixture_coverage": 220,
                "reachable_union_alternatives_exercised": 130,
                "required_fields_exercised": 220,
                "schema_properties_exercised": 130,
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
        self.assertEqual(len(exclusions), 61)
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
                generate("seed-1", "1"),
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
