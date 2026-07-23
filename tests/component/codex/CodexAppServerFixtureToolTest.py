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
        removed = keys[0]
        mutated[removed] = {
            **mutated[removed],
            "classification": "SharedWithinSlice",
        }
        with self.assertRaisesRegex(
            tool.FixtureError,
            "A1.1 SharedCommon assignment mismatch: count=25",
        ):
            tool.derive_b2_shared_common_keys(mutated)

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
                for record in index["fixtures"]
            ),
            21,
        )
        self.assertEqual(
            sum(
                record["role"] == "union_nullable_null"
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
                "total": 482,
                "positive": 352,
                "negative": 130,
                "by_role": {
                    "client_request_params": 87,
                    "client_request_result": 87,
                    "existing_typed_identity": 34,
                    "malformed_known": 1,
                    "malformed_known_missing_discriminator": 23,
                    "malformed_known_missing_required": 21,
                    "malformed_known_wrong_discriminator_type": 23,
                    "malformed_known_wrong_type": 42,
                    "nested_union_failure": 3,
                    "open_enum_known_value": 6,
                    "server_request_params": 10,
                    "server_request_response": 10,
                    "union_branch": 76,
                    "union_branch_supplement": 9,
                    "union_nullable_null": 12,
                    "union_optional_omitted": 21,
                    "unknown_discriminator": 10,
                    "unknown_enum_value": 3,
                    "unknown_method": 4,
                },
            },
        )
        self.assertEqual(
            first_index["mutation_counts"],
            {
                "selected_branch_required_locations": 1373,
                "required_locations": 1373,
                "required_field_removals_rejected": 1373,
                "wrong_type_mutations_rejected": 1360,
                "wrong_type_unconstrained_exclusions": 13,
                "alternative_branch_acceptances": 1,
                "optional_present_locations": 1126,
                "globally_optional_locations": 1126,
                "optional_omissions_accepted": 1126,
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
            completeness["counts"]["identities_with_positive_fixtures"], 188
        )
        self.assertEqual(
            completeness["counts"]["facts_true_by_field"],
            {
                "authoritative_root_association": 188,
                "fixture_current": 188,
                "independently_schema_validated": 188,
                "nullable_semantics_exercised": 42,
                "optional_omitted_exercised": 188,
                "optional_present_exercised": 188,
                "positive_fixture_coverage": 188,
                "reachable_union_alternatives_exercised": 42,
                "required_fields_exercised": 188,
                "schema_properties_exercised": 42,
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
            for mutation in record["validation"]["wrong_types"]
            if mutation["exclusion"] is not None
        ]
        self.assertEqual(len(exclusions), 13)
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
