#!/usr/bin/env python3

"""Offline reproducibility and exact-diagnostic guards for the A1.2 audit."""

from __future__ import annotations

import argparse
import copy
import importlib.util
import json
import sys
import tempfile
import unittest
from collections import Counter
from pathlib import Path
from types import ModuleType, SimpleNamespace
from typing import Callable

sys.dont_write_bytecode = True


START_BASE_SHA = "9f7b2955d017cab189fb7b7a80211d0c2788f819"

ACCOUNT_REQUESTS = {
    "account/login/cancel",
    "account/login/start",
    "account/logout",
    "account/rateLimitResetCredit/consume",
    "account/rateLimits/read",
    "account/read",
    "account/sendAddCreditsNudgeEmail",
    "account/usage/read",
    "account/workspaceMessages/read",
}
MODEL_REQUESTS = {
    "model/list",
    "modelProvider/capabilities/read",
}
CONFIG_READ_REQUESTS = {
    "config/read",
    "configRequirements/read",
}
CONFIG_MUTATION_REQUESTS = {
    "config/batchWrite",
    "config/mcpServer/reload",
    "config/value/write",
    "experimentalFeature/enablement/set",
    "experimentalFeature/list",
}
CLIENT_REQUESTS = (
    ACCOUNT_REQUESTS
    | MODEL_REQUESTS
    | CONFIG_READ_REQUESTS
    | CONFIG_MUTATION_REQUESTS
)

ACCOUNT_NOTIFICATIONS = {
    "account/login/completed",
    "account/rateLimits/updated",
    "account/updated",
}
MODEL_NOTIFICATIONS = {
    "model/rerouted",
    "model/safetyBuffering/updated",
    "model/verification",
}
CONFIG_NOTIFICATIONS = {"configWarning"}
SERVER_NOTIFICATIONS = (
    ACCOUNT_NOTIFICATIONS | MODEL_NOTIFICATIONS | CONFIG_NOTIFICATIONS
)

ACCOUNT_UNIONS = {
    ("Account", "amazonBedrock"),
    ("Account", "apiKey"),
    ("Account", "chatgpt"),
    ("LoginAccountParams", "apiKey"),
    ("LoginAccountParams", "chatgpt"),
    ("LoginAccountParams", "chatgptAuthTokens"),
    ("LoginAccountParams", "chatgptDeviceCode"),
    ("LoginAccountResponse", "apiKey"),
    ("LoginAccountResponse", "chatgpt"),
    ("LoginAccountResponse", "chatgptAuthTokens"),
    ("LoginAccountResponse", "chatgptDeviceCode"),
}
CONFIG_LAYER_UNIONS = {
    ("ConfigLayerSource", "enterpriseManaged"),
    ("ConfigLayerSource", "legacyManagedConfigTomlFromFile"),
    ("ConfigLayerSource", "legacyManagedConfigTomlFromMdm"),
    ("ConfigLayerSource", "mdm"),
    ("ConfigLayerSource", "project"),
    ("ConfigLayerSource", "sessionFlags"),
    ("ConfigLayerSource", "system"),
    ("ConfigLayerSource", "user"),
}

OPERATION_CONTRACTS = {
    "account/login/cancel": (
        "CancelLoginAccountParams",
        "CancelLoginAccountResponse",
        "Concrete",
        "typed::Accounts::cancelLogin",
        False,
    ),
    "account/login/start": (
        "LoginAccountParams",
        "LoginAccountResponse",
        "Concrete",
        "typed::Accounts::startLogin",
        False,
    ),
    "account/logout": (
        "Unit",
        "Unit",
        "Unit",
        "typed::Accounts::logout",
        False,
    ),
    "account/rateLimitResetCredit/consume": (
        "ConsumeAccountRateLimitResetCreditParams",
        "ConsumeAccountRateLimitResetCreditResponse",
        "Concrete",
        "typed::Accounts::consumeRateLimitResetCredit",
        False,
    ),
    "account/rateLimits/read": (
        "Unit",
        "GetAccountRateLimitsResponse",
        "Concrete",
        "typed::Accounts::readRateLimits",
        True,
    ),
    "account/read": (
        "GetAccountParams",
        "GetAccountResponse",
        "Concrete",
        "typed::Accounts::read",
        True,
    ),
    "account/sendAddCreditsNudgeEmail": (
        "SendAddCreditsNudgeEmailParams",
        "SendAddCreditsNudgeEmailResponse",
        "Concrete",
        "typed::Accounts::sendAddCreditsNudgeEmail",
        False,
    ),
    "account/usage/read": (
        "Unit",
        "GetAccountTokenUsageResponse",
        "Concrete",
        "typed::Accounts::readUsage",
        True,
    ),
    "account/workspaceMessages/read": (
        "Unit",
        "GetWorkspaceMessagesResponse",
        "Concrete",
        "typed::Accounts::readWorkspaceMessages",
        True,
    ),
    "config/batchWrite": (
        "ConfigBatchWriteParams",
        "ConfigWriteResponse",
        "Concrete",
        "typed::Configuration::batchWrite",
        False,
    ),
    "config/mcpServer/reload": (
        "Unit",
        "Unit",
        "Unit",
        "typed::Configuration::reloadMcpServers",
        False,
    ),
    "config/read": (
        "ConfigReadParams",
        "ConfigReadResponse",
        "Concrete",
        "typed::Configuration::read",
        True,
    ),
    "config/value/write": (
        "ConfigValueWriteParams",
        "ConfigWriteResponse",
        "Concrete",
        "typed::Configuration::writeValue",
        False,
    ),
    "configRequirements/read": (
        "Unit",
        "ConfigRequirementsReadResponse",
        "Concrete",
        "typed::Configuration::readRequirements",
        True,
    ),
    "experimentalFeature/enablement/set": (
        "ExperimentalFeatureEnablementSetParams",
        "ExperimentalFeatureEnablementSetResponse",
        "Concrete",
        "typed::Configuration::setExperimentalFeatureEnablement",
        False,
    ),
    "experimentalFeature/list": (
        "ExperimentalFeatureListParams",
        "ExperimentalFeatureListResponse",
        "Concrete",
        "typed::Configuration::listExperimentalFeatures",
        True,
    ),
    "model/list": (
        "ModelListParams",
        "ModelListResponse",
        "Concrete",
        "typed::Models::list",
        True,
    ),
    "modelProvider/capabilities/read": (
        "ModelProviderCapabilitiesReadParams",
        "ModelProviderCapabilitiesReadResponse",
        "Concrete",
        "typed::Models::readProviderCapabilities",
        True,
    ),
}

EXPECTED_PARTIAL_KEYS = {
    (
        "server_notification",
        "ServerNotification",
        "method",
        "model/rerouted",
    ),
    (
        "server_request",
        "ServerRequest",
        "method",
        "account/chatgptAuthTokens/refresh",
    ),
}

EXPECTED_STAGED_GLOBAL_STATUS = {
    "B2": {
        "Complete": 191,
        "NotApplicable": 48,
        "NotImplemented": 141,
        "Partial": 7,
    },
    "B3": {
        "Complete": 196,
        "NotApplicable": 48,
        "NotImplemented": 137,
        "Partial": 6,
    },
    "B4": {
        "Complete": 207,
        "NotApplicable": 48,
        "NotImplemented": 126,
        "Partial": 6,
    },
    "B5": {
        "Complete": 212,
        "NotApplicable": 48,
        "NotImplemented": 121,
        "Partial": 6,
    },
}


def load_tool(path: Path) -> ModuleType:
    tool_directory = str(path.resolve().parent)
    if tool_directory not in sys.path:
        sys.path.insert(0, tool_directory)
    specification = importlib.util.spec_from_file_location(
        "snodec_codex_a1_2_audit", path
    )
    if specification is None or specification.loader is None:
        raise RuntimeError(f"unable to load A1.2 audit tool: {path}")
    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


def load_json(path: Path) -> dict[str, object]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise AssertionError(f"expected an object-valued JSON document: {path}")
    return value


def surface_key(value: object) -> tuple[str, str, str, str]:
    if not isinstance(value, dict):
        raise AssertionError("ProtocolSurfaceKey must be an object")
    key = value.get("protocol_surface_key", value)
    if not isinstance(key, dict):
        raise AssertionError("ProtocolSurfaceKey projection must be an object")
    return (
        str(key.get("category")),
        str(key.get("domain")),
        str(key.get("discriminator_field", key.get("field"))),
        str(key.get("name")),
    )


def expected_batch_keys() -> dict[str, set[tuple[str, str, str, str]]]:
    return {
        "B2": {
            *{
                ("client_request", "ClientRequest", "method", name)
                for name in ACCOUNT_REQUESTS
            },
            *{
                (
                    "server_notification",
                    "ServerNotification",
                    "method",
                    name,
                )
                for name in ACCOUNT_NOTIFICATIONS
            },
            (
                "server_request",
                "ServerRequest",
                "method",
                "account/chatgptAuthTokens/refresh",
            ),
            *{
                ("tagged_union_discriminator", domain, "type", name)
                for domain, name in ACCOUNT_UNIONS
            },
        },
        "B3": {
            *{
                ("client_request", "ClientRequest", "method", name)
                for name in MODEL_REQUESTS
            },
            *{
                (
                    "server_notification",
                    "ServerNotification",
                    "method",
                    name,
                )
                for name in MODEL_NOTIFICATIONS
            },
        },
        "B4": {
            *{
                ("client_request", "ClientRequest", "method", name)
                for name in CONFIG_READ_REQUESTS
            },
            (
                "server_notification",
                "ServerNotification",
                "method",
                "configWarning",
            ),
            *{
                ("tagged_union_discriminator", domain, "type", name)
                for domain, name in CONFIG_LAYER_UNIONS
            },
        },
        "B5": {
            (
                "client_request",
                "ClientRequest",
                "method",
                name,
            )
            for name in CONFIG_MUTATION_REQUESTS
        },
    }


class CodexA12AuditToolTest(unittest.TestCase):
    tool: ModuleType
    arguments: SimpleNamespace
    start_state: dict[str, object]
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
            fixture_index=OPTIONS.fixture_index,
            draft07_validator=OPTIONS.draft07_validator,
            start_state=OPTIONS.start_state,
            plan_output=OPTIONS.plan_output,
            closure_output=OPTIONS.closure_output,
            base_sha=None,
            force_start_state=False,
        )
        cls.start_state = load_json(OPTIONS.start_state)
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

    def assert_start_state_mutation(
        self,
        mutate: Callable[[dict[str, object]], None],
        expected_code: str,
    ) -> None:
        start_state = copy.deepcopy(self.start_state)
        mutate(start_state)
        with tempfile.TemporaryDirectory(
            prefix="snodec-codex-a1-2-start-state-"
        ) as directory:
            path = Path(directory) / "a1-2-start-state.json"
            path.write_text(
                json.dumps(
                    start_state,
                    indent=2,
                    sort_keys=True,
                    ensure_ascii=False,
                )
                + "\n",
                encoding="utf-8",
            )
            arguments = SimpleNamespace(**vars(self.arguments))
            arguments.start_state = path
            with self.assertRaises(self.tool.AuditError) as caught:
                self.tool.build_reports(arguments)
        self.assertEqual((expected_code,), caught.exception.codes)
        self.assertEqual(expected_code, caught.exception.code)

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

    def test_exact_start_state_and_identity_partition(self) -> None:
        self.assertEqual(START_BASE_SHA, self.start_state["base_sha"])
        counts = self.start_state["counts"]
        assert isinstance(counts, dict)
        self.assertEqual(45, counts["identity_count"])
        self.assertEqual(342, counts["unrelated_registry_identity_count"])
        self.assertEqual(
            {"NotImplemented": 43, "Partial": 2},
            counts["a1_2_schema_status"],
        )
        self.assertEqual(
            {"Implemented": 2, "NotImplemented": 43},
            counts["a1_2_implementation_status"],
        )
        self.assertEqual(
            {
                "Complete": 167,
                "NotApplicable": 48,
                "NotImplemented": 164,
                "Partial": 8,
            },
            counts["global_schema_status"],
        )

        identities = self.start_state["identities"]
        unrelated = self.start_state["unrelated_registry_identities"]
        assert isinstance(identities, list)
        assert isinstance(unrelated, list)
        keys = [surface_key(row) for row in identities]
        unrelated_keys = [surface_key(row) for row in unrelated]
        self.assertEqual(45, len(keys))
        self.assertEqual(45, len(set(keys)))
        self.assertEqual(342, len(unrelated_keys))
        self.assertEqual(342, len(set(unrelated_keys)))
        self.assertTrue(set(keys).isdisjoint(unrelated_keys))
        self.assertEqual(
            EXPECTED_PARTIAL_KEYS,
            {
                surface_key(row)
                for row in identities
                if isinstance(row, dict)
                and isinstance(row.get("registry"), dict)
                and row["registry"].get("typed_schema_status") == "Partial"
            },
        )

    def test_start_state_provenance_and_prerequisite_are_immutable(self) -> None:
        self.assert_start_state_mutation(
            lambda document: document["prerequisite"].__setitem__(
                "a1_1_pr_tip_sha", "0" * 40
            ),
            "StartStatePrerequisiteMismatch",
        )
        self.assert_start_state_mutation(
            lambda document: document["capture_sources"][
                "registry"
            ].__setitem__("sha256", "0" * 64),
            "StartStateCaptureSourceMismatch",
        )
        self.assert_start_state_mutation(
            lambda document: document["frozen_a1_1_artifacts"][
                "a1_1_type_closure"
            ].__setitem__("sha256", "0" * 64),
            "StartStateFrozenA11ArtifactMismatch",
        )

    def test_start_state_metadata_and_partial_set_are_immutable(self) -> None:
        self.assert_start_state_mutation(
            lambda document: document.__setitem__(
                "codex_version", "codex-cli future"
            ),
            "StartStateMetadataMismatch",
        )
        self.assert_start_state_mutation(
            lambda document: document.__setitem__(
                "initial_partial_identities",
                document["initial_partial_identities"][:-1],
            ),
            "StartStatePartialSetMismatch",
        )
        self.assert_start_state_mutation(
            lambda document: document.__setitem__(
                "unreviewed_extra_field", True
            ),
            "StartStateDocumentMismatch",
        )

    def test_live_plan_rows_are_the_exact_final_b5_progress_boundary(
        self,
    ) -> None:
        identities = self.plan["identities"]
        counts = self.plan["counts"]
        assert isinstance(identities, list)
        assert isinstance(counts, dict)
        rows = {
            surface_key(row): row
            for row in identities
            if isinstance(row, dict)
        }
        complete_keys = (
            expected_batch_keys()["B2"]
            | expected_batch_keys()["B3"]
            | expected_batch_keys()["B4"]
            | expected_batch_keys()["B5"]
        )
        self.assertEqual("B5", counts["current_progress_stage"])
        self.assertEqual(
            {
                "Complete": 45,
                "Partial": 0,
                "NotImplemented": 0,
            },
            {
                status: counts["current_a1_2_schema_status"].get(
                    status, 0
                )
                for status in (
                    "Complete",
                    "Partial",
                    "NotImplemented",
                )
            },
        )
        self.assertEqual(
            {"Implemented": 45, "NotImplemented": 0},
            {
                status: counts[
                    "current_a1_2_implementation_status"
                ].get(status, 0)
                for status in ("Implemented", "NotImplemented")
            },
        )
        self.assertEqual(
            {"NotImplemented": 43, "Partial": 2},
            counts["initial_a1_2_schema_status"],
        )
        self.assertEqual(
            {
                "Complete": 212,
                "Partial": 6,
                "NotImplemented": 121,
                "NotApplicable": 48,
            },
            counts["derived_final_global_schema_status"],
        )
        self.assertEqual(
            complete_keys,
            {
                key
                for key, row in rows.items()
                if row["current_schema_status"] == "Complete"
            },
        )
        self.assertEqual(
            set(),
            {
                key
                for key, row in rows.items()
                if row["current_schema_status"] == "Partial"
            },
        )
        self.assertEqual(
            set(),
            {
                key
                for key, row in rows.items()
                if row["current_schema_status"] == "NotImplemented"
            },
        )
        self.assertEqual(
            complete_keys,
            {
                key
                for key, row in rows.items()
                if row["current_implementation_status"]
                == "Implemented"
            },
        )
        self.assertTrue(
            all(
                rows[key]["current_runtime_target"]
                not in (None, "", "std::monostate{}")
                and rows[key]["current_fixture_ids"]
                for key in complete_keys
            )
        )

    def test_exact_taxonomy_assignments_batches_and_contracts(self) -> None:
        identities = self.plan["identities"]
        assert isinstance(identities, list)
        rows = {
            surface_key(row): row
            for row in identities
            if isinstance(row, dict)
        }
        self.assertEqual(45, len(identities))
        self.assertEqual(45, len(rows))
        required_identity_fields = {
            "category",
            "classification",
            "current_fixture_ids",
            "current_implementation_status",
            "current_runtime_disposition",
            "current_runtime_target",
            "current_schema_completeness",
            "current_schema_status",
            "deprecated",
            "directionality",
            "discriminator_field",
            "domain",
            "expected_backend_core_handling",
            "expected_frontend_behavior",
            "module",
            "owning_implementation_batch",
            "protocol_surface_key",
            "reaching_stable_roots",
            "required_negative_fixtures",
            "required_positive_fixtures",
            "required_private_codec_or_descriptor",
            "required_public_type_or_facade",
            "security_and_sensitivity",
            "source_compatibility_consideration",
            "stability",
        }
        for key, row in rows.items():
            with self.subTest(required_identity_fields=key):
                self.assertEqual(
                    set(),
                    required_identity_fields - set(row),
                )

        taxonomy = Counter(key[0] for key in rows)
        self.assertEqual(
            {
                "client_request": 18,
                "server_notification": 7,
                "server_request": 1,
                "tagged_union_discriminator": 19,
            },
            dict(sorted(taxonomy.items())),
        )
        self.assertFalse(
            {
                "client_notification",
                "delta_progress_discriminator",
                "item_discriminator",
            }
            & set(taxonomy)
        )
        self.assertEqual(
            {
                "RootOwnedNestedUnion": 11,
                "SharedWithinSlice": 8,
                "StablePublicRoot": 26,
            },
            dict(
                sorted(
                    Counter(
                        str(row.get("classification"))
                        for row in identities
                        if isinstance(row, dict)
                    ).items()
                )
            ),
        )
        self.assertEqual(
            {"AccountsModelsConfiguration": 45},
            dict(
                Counter(
                    str(row.get("module"))
                    for row in identities
                    if isinstance(row, dict)
                )
            ),
        )
        self.assertTrue(
            all(
                isinstance(row, dict)
                and row.get("stability") == "stable"
                and row.get("deprecated") is False
                for row in identities
            )
        )

        actual_batches: dict[str, set[tuple[str, str, str, str]]] = {}
        for batch in ("B2", "B3", "B4", "B5"):
            actual_batches[batch] = {
                key
                for key, row in rows.items()
                if row.get("owning_implementation_batch") == batch
            }
        self.assertEqual(expected_batch_keys(), actual_batches)
        self.assertEqual(
            {"B2": 24, "B3": 5, "B4": 11, "B5": 5},
            {batch: len(keys) for batch, keys in actual_batches.items()},
        )

        client_rows = {
            key[3]: row
            for key, row in rows.items()
            if key[0] == "client_request"
        }
        self.assertEqual(CLIENT_REQUESTS, set(client_rows))
        self.assertIn("experimentalFeature/enablement/set", client_rows)
        self.assertIn("experimentalFeature/list", client_rows)
        result_kinds: Counter[str] = Counter()
        for method, expected in sorted(OPERATION_CONTRACTS.items()):
            with self.subTest(client_request=method):
                contract = client_rows[method].get("request_contract")
                assert isinstance(contract, dict)
                params, result_type, kind, api_method, safe = expected
                self.assertEqual(
                    params, contract["authoritative_params_type"]
                )
                self.assertEqual(
                    result_type,
                    contract["authoritative_successful_result_type"],
                )
                self.assertEqual(kind, contract["result_contract_kind"])
                self.assertEqual(
                    api_method, contract["proposed_public_method_name"]
                )
                self.assertEqual(
                    safe, contract["safe_for_real_read_only_integration"]
                )
                self.assertIn("params_wire_encoding", contract)
                result_kinds[kind] += 1
        self.assertEqual({"Concrete": 16, "Unit": 2}, dict(result_kinds))
        self.assertEqual(
            {"account/logout", "config/mcpServer/reload"},
            {
                method
                for method, row in client_rows.items()
                if isinstance(row.get("request_contract"), dict)
                and row["request_contract"].get("result_contract_kind")
                == "Unit"
            },
        )

        auth_key = (
            "server_request",
            "ServerRequest",
            "method",
            "account/chatgptAuthTokens/refresh",
        )
        auth_contract = rows[auth_key].get("server_request_contract")
        assert isinstance(auth_contract, dict)
        self.assertEqual(
            "ChatgptAuthTokensRefreshParams",
            auth_contract["authoritative_params_type"],
        )
        self.assertEqual(
            "ChatgptAuthTokensRefreshResponse",
            auth_contract["authoritative_response_type"],
        )
        for required_field in (
            "occurrence_token_owner",
            "sensitive_fields",
            "existing_compatibility_types",
            "existing_compatibility_overload",
            "permitted_test_strategy",
        ):
            self.assertIn(required_field, auth_contract)
        self.assertEqual(
            [
                "accessToken",
                "chatgptAccountId",
                "chatgptPlanType",
                "previousAccountId",
            ],
            auth_contract["sensitive_fields"],
        )

        plan_counts = self.plan["counts"]
        assert isinstance(plan_counts, dict)
        self.assertEqual(45, plan_counts["identity_denominator"])
        self.assertEqual(
            18,
            plan_counts[
                "client_successful_result_root_obligations_not_identities"
            ],
        )
        self.assertEqual(
            1,
            plan_counts[
                "server_request_response_root_obligations_not_identities"
            ],
        )
        self.assertEqual(
            19,
            plan_counts[
                "total_response_root_obligations_not_identities"
            ],
        )
        staged = self.plan["staged_exact_a1_2_ratchets"]
        assert isinstance(staged, list)
        self.assertEqual(
            EXPECTED_STAGED_GLOBAL_STATUS,
            {
                str(row["batch"]): row["derived_global_schema_status"]
                for row in staged
                if isinstance(row, dict)
            },
        )

    def test_closure_counts_are_mechanically_derived(self) -> None:
        definitions = self.closure["definitions"]
        paths = self.closure["schema_paths"]
        counts = self.closure["counts"]
        assert isinstance(definitions, list)
        assert isinstance(paths, list)
        assert isinstance(counts, dict)
        self.assertEqual(
            len(definitions), counts["reachable_named_definitions"]
        )
        self.assertEqual(len(paths), counts["schema_path_dispositions"])
        self.assertEqual(
            dict(
                sorted(
                    Counter(
                        str(row.get("disposition"))
                        for row in definitions
                        if isinstance(row, dict)
                    ).items()
                )
            ),
            counts["definition_dispositions"],
        )
        self.assertEqual(
            dict(
                sorted(
                    Counter(
                        str(row.get("disposition"))
                        for row in paths
                        if isinstance(row, dict)
                    ).items()
                )
            ),
            counts["schema_path_disposition_kinds"],
        )
        valid_dispositions = {
            "OpenStringEnum",
            "PrivateCodecHelper",
            "ProtocolDefinedOpaqueJson",
            "PublicHandwrittenType",
            "ReusedExistingType",
            "StrongIdentifier",
        }
        self.assertTrue(
            all(
                isinstance(row, dict)
                and row.get("disposition") in valid_dispositions
                for row in definitions
            )
        )
        self.assertTrue(
            all(
                isinstance(row, dict)
                and row.get("disposition") in valid_dispositions
                for row in paths
            )
        )
        self.assertTrue(
            all(
                isinstance(row, dict)
                and row.get("raw_retention_permitted")
                is (
                    row.get("disposition")
                    == "ProtocolDefinedOpaqueJson"
                )
                for row in paths
            )
        )
        definitions_by_name = {
            str(row["definition_key"]["name"]): row
            for row in definitions
            if isinstance(row, dict)
            and isinstance(row.get("definition_key"), dict)
        }
        refresh_reason = definitions_by_name[
            "ChatgptAuthTokensRefreshReason"
        ]
        self.assertEqual(
            "OpenStringEnum", refresh_reason["disposition"]
        )
        self.assertEqual(
            ["unauthorized"], refresh_reason["known_values"]
        )
        self.assertEqual(
            "ForwardCompatibility",
            refresh_reason["future_value_behavior"][
                "diagnostic_severity"
            ],
        )
        for name in (
            "AuthMode",
            "AutoCompactTokenLimitScope",
            "ConsumeAccountRateLimitResetCreditOutcome",
            "ExperimentalFeatureStage",
            "InputModality",
        ):
            with self.subTest(open_string_enum=name):
                self.assertEqual(
                    "OpenStringEnum",
                    definitions_by_name[name]["disposition"],
                )

        paths_by_name = {
            str(row["schema_path"]): row
            for row in paths
            if isinstance(row, dict)
        }
        self.assertEqual(
            "typed::ApiKeyLoginAccountParams",
            paths_by_name[
                "#/definitions/v2/LoginAccountParams/oneOf/0/"
                "properties/apiKey"
            ]["cpp_owner"],
        )
        self.assertEqual(
            "typed::OptionalNullable<typed::PlanType>",
            paths_by_name[
                "#/definitions/ChatgptAuthTokensRefreshResponse/"
                "properties/chatgptPlanType"
            ]["cpp_type"],
        )
        self.assertEqual(
            "typed::OptionalNullable<typed::ModelServiceTierId>",
            paths_by_name[
                "#/definitions/v2/Model/properties/defaultServiceTier"
            ]["cpp_type"],
        )
        self.assertEqual(
            "std::map<typed::ExperimentalFeatureId, bool>",
            paths_by_name[
                "#/definitions/v2/"
                "ExperimentalFeatureEnablementSetParams/properties/"
                "enablement"
            ]["cpp_type"],
        )
        self.assertEqual(
            (
                "typed::OptionalNullable<std::map<typed::RateLimitId, "
                "typed::RateLimitSnapshot>>"
            ),
            paths_by_name[
                "#/definitions/v2/GetAccountRateLimitsResponse/"
                "properties/rateLimitsByLimitId"
            ]["cpp_type"],
        )
        self.assertEqual(
            {
                "typed::ConfigKeyPath": 2,
                "typed::ExperimentalFeatureId": 6,
                "typed::PermissionProfileName": 2,
                "typed::RateLimitId": 2,
            },
            counts["semantic_map_key_types"],
        )
        self.assertEqual(
            {
                "cpp_mapping": (
                    "typed::Config::unknownProperties -> "
                    "std::map<std::string, Json>"
                ),
                "cpp_member": "unknownProperties",
                "cpp_owner": "typed::Config",
                "cpp_type": "std::map<std::string, Json>",
                "separates_unknown_fields_from_known_fields": True,
            },
            paths_by_name[
                "#/definitions/v2/Config/additionalProperties"
            ]["map_container"],
        )
        self.assertEqual(
            "PotentialCredentialBearingConfiguration",
            paths_by_name[
                "#/definitions/v2/ConfigReadResponse/properties/config"
            ]["sensitivity"]["classification"],
        )
        self.assertFalse(
            paths_by_name[
                "#/definitions/v2/ConfigReadResponse/properties/config"
            ]["sensitivity"]["log_value_permitted"]
        )
        workspace_id_items = paths_by_name[
            "#/definitions/v2/ForcedChatgptWorkspaceIds/anyOf/1/items"
        ]["sensitivity"]
        self.assertEqual(
            "AccountWorkspaceIdentity",
            workspace_id_items["classification"],
        )
        self.assertTrue(workspace_id_items["credential_or_account_value"])
        self.assertFalse(workspace_id_items["log_value_permitted"])
        self.assertEqual(
            "PersonallyIdentifyingAccountData",
            paths_by_name[
                "#/definitions/v2/GetAccountResponse/properties/account"
            ]["sensitivity"]["classification"],
        )
        self.assertFalse(
            paths_by_name[
                "#/definitions/v2/GetAccountResponse/properties/account"
            ]["sensitivity"]["log_value_permitted"]
        )

        plan_rows = {
            surface_key(row): row
            for row in self.plan["identities"]
            if isinstance(row, dict)
        }
        api_key_params = plan_rows[
            (
                "tagged_union_discriminator",
                "LoginAccountParams",
                "type",
                "apiKey",
            )
        ]
        self.assertEqual(
            ["CredentialSecret"],
            api_key_params["security_and_sensitivity"][
                "sensitive_classifications"
            ],
        )
        self.assertTrue(
            all(
                "/oneOf/0/" in path
                for path in api_key_params[
                    "required_negative_fixtures"
                ]["required_field_removal_schema_paths"]
            )
        )
        device_code_params = plan_rows[
            (
                "tagged_union_discriminator",
                "LoginAccountParams",
                "type",
                "chatgptDeviceCode",
            )
        ]
        self.assertFalse(
            device_code_params["security_and_sensitivity"][
                "contains_sensitive_or_potentially_sensitive_fields"
            ]
        )
        rate_limits = plan_rows[
            (
                "client_request",
                "ClientRequest",
                "method",
                "account/rateLimits/read",
            )
        ]
        self.assertIn(
            "AccountUsageData",
            rate_limits["security_and_sensitivity"][
                "sensitive_classifications"
            ],
        )
        self.assertFalse(
            rate_limits["security_and_sensitivity"][
                "logs_may_contain_values"
            ]
        )
        login_start = plan_rows[
            (
                "client_request",
                "ClientRequest",
                "method",
                "account/login/start",
            )
        ]
        positive = login_start["required_positive_fixtures"]
        self.assertTrue(
            positive["base_cases_are_seeds_not_complete_coverage"]
        )
        self.assertCountEqual(
            ["apiKey", "chatgpt", "chatgptAuthTokens", "chatgptDeviceCode"],
            next(
                row["alternatives"]
                for row in positive["expansion_obligations"][
                    "registered_tagged_union_alternatives"
                ]
                if row["definition_key"]["name"]
                == "LoginAccountParams"
            ),
        )
        self.assertTrue(
            positive["expansion_obligations"][
                "nullable_schema_valid_states"
            ]
        )
        api_key_path = paths_by_name[
            "#/definitions/v2/LoginAccountParams/oneOf/0/"
            "properties/apiKey"
        ]
        self.assertEqual(
            {"apiKey"},
            {
                str(root["surface_key"]["name"])
                for root in api_key_path["reaching_roots"]
                if root["role"] == "registered_tagged_union_family"
            },
        )
        opaque_paths = {
            str(row.get("schema_path"))
            for row in paths
            if isinstance(row, dict)
            and row.get("disposition") == "ProtocolDefinedOpaqueJson"
        }
        evidence = self.closure[
            "authorized_protocol_defined_opaque_json_paths"
        ]
        assert isinstance(evidence, list)
        self.assertEqual(
            opaque_paths,
            {
                str(row.get("schema_path"))
                for row in evidence
                if isinstance(row, dict)
            },
        )
        self.assertTrue(
            all(
                isinstance(row, dict)
                and isinstance(row.get("sensitivity"), dict)
                and isinstance(
                    row["sensitivity"].get("classification"), str
                )
                and bool(row["sensitivity"].get("classification"))
                and row["sensitivity"].get(
                    "diagnostic_value_permitted"
                )
                is False
                for row in paths
            )
        )
        self.assertTrue(
            all(
                isinstance(row, dict)
                and "required" in row
                and isinstance(row.get("nullable"), bool)
                and isinstance(row.get("defaulted"), bool)
                and isinstance(row.get("presence_model"), str)
                for row in paths
            )
        )

    def test_exact_report_mutation_diagnostics(self) -> None:
        def identity_denominator(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            counts = plan["counts"]
            assert isinstance(counts, dict)
            counts["identity_denominator"] = 64

        def taxonomy_summary(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            counts = plan["counts"]
            assert isinstance(counts, dict)
            taxonomy = counts["taxonomy"]
            assert isinstance(taxonomy, dict)
            taxonomy["client_request"] = 19

        def response_arithmetic(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            counts = plan["counts"]
            assert isinstance(counts, dict)
            counts[
                "response_root_obligations_not_identities"
            ] = 64

        def wrong_classification(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            counts = plan["counts"]
            assert isinstance(counts, dict)
            classifications = counts["classifications"]
            assert isinstance(classifications, dict)
            classifications["StablePublicRoot"] = 25

        def wrong_module(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            identities = plan["identities"]
            assert isinstance(identities, list)
            row = identities[0]
            assert isinstance(row, dict)
            row["module"] = "WrongModule"

        def wrong_category_field(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            identities = plan["identities"]
            assert isinstance(identities, list)
            row = identities[0]
            assert isinstance(row, dict)
            row["category"] = "server_request"

        def wrong_domain_field(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            identities = plan["identities"]
            assert isinstance(identities, list)
            row = identities[0]
            assert isinstance(row, dict)
            row["domain"] = "WrongDomain"

        def wrong_discriminator_field(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            identities = plan["identities"]
            assert isinstance(identities, list)
            row = identities[0]
            assert isinstance(row, dict)
            row["discriminator_field"] = "type"

        def coherent_classification_swap(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            identities = plan["identities"]
            counts = plan["counts"]
            assert isinstance(identities, list)
            assert isinstance(counts, dict)
            first = next(
                row
                for row in identities
                if isinstance(row, dict)
                and row.get("classification") == "StablePublicRoot"
            )
            second = next(
                row
                for row in identities
                if isinstance(row, dict)
                and row.get("classification")
                == "RootOwnedNestedUnion"
            )
            first["classification"], second["classification"] = (
                second["classification"],
                first["classification"],
            )
            counts["identity_assignment_mapping_sha256"] = (
                self.tool.sha256_json(
                    self.tool.identity_assignment_projection(identities)
                )
            )

        def wrong_live_progress_status(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            identities = plan["identities"]
            assert isinstance(identities, list)
            row = next(
                value
                for value in identities
                if isinstance(value, dict)
                and value.get("current_schema_status") == "Complete"
            )
            row["current_schema_status"] = "Partial"

        def coherent_live_progress_wrong_identity_boundary(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            identities = plan["identities"]
            counts = plan["counts"]
            assert isinstance(identities, list)
            assert isinstance(counts, dict)
            wrong_boundary_rows = [
                row
                for row in identities
                if isinstance(row, dict)
                and row.get("current_schema_status") == "Complete"
                and row.get("owning_implementation_batch") == "B2"
            ][:5]
            self.assertEqual(5, len(wrong_boundary_rows))
            for row in wrong_boundary_rows:
                row["current_schema_status"] = "NotImplemented"
                row["current_implementation_status"] = "NotImplemented"
            counts["current_progress_stage"] = "B4"
            counts["current_a1_2_schema_status"] = {
                "Complete": 40,
                "NotImplemented": 5,
            }
            counts["current_a1_2_implementation_status"] = {
                "Implemented": 40,
                "NotImplemented": 5,
            }
            counts["identity_live_progress_mapping_sha256"] = (
                self.tool.sha256_json(
                    self.tool.identity_start_projection(identities)
                )
            )

        def unstable_identity(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            identities = plan["identities"]
            assert isinstance(identities, list)
            row = identities[0]
            assert isinstance(row, dict)
            row["stability"] = "experimental_only"

        def batch_self_cycle(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            batches = plan["batches"]
            assert isinstance(batches, list)
            row = next(
                value
                for value in batches
                if isinstance(value, dict)
                and value.get("batch") == "B2"
            )
            row["depends_on"] = ["B2"]

        def missing_definition_disposition(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            definitions = closure["definitions"]
            assert isinstance(definitions, list)
            row = definitions[0]
            assert isinstance(row, dict)
            row.pop("disposition")

        def missing_path_disposition(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            paths = closure["schema_paths"]
            assert isinstance(paths, list)
            row = next(
                value
                for value in paths
                if isinstance(value, dict)
                and value.get("disposition")
                != "ProtocolDefinedOpaqueJson"
            )
            row.pop("disposition")

        def missing_sensitivity_policy(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            paths = closure["schema_paths"]
            assert isinstance(paths, list)
            row = paths[0]
            assert isinstance(row, dict)
            row.pop("sensitivity")

        def permit_sensitive_path_logging(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            paths = closure["schema_paths"]
            assert isinstance(paths, list)
            row = next(
                value
                for value in paths
                if isinstance(value, dict)
                and isinstance(value.get("sensitivity"), dict)
                and value["sensitivity"].get("classification")
                == "CredentialSecret"
            )
            row["sensitivity"]["log_value_permitted"] = True

        def permit_sensitive_path_frontend(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            paths = closure["schema_paths"]
            assert isinstance(paths, list)
            row = next(
                value
                for value in paths
                if isinstance(value, dict)
                and isinstance(value.get("sensitivity"), dict)
                and value["sensitivity"].get("classification")
                == "CredentialSecret"
            )
            row["sensitivity"]["frontend_protocol_exposure_permitted"] = (
                True
            )

        def permit_sensitive_identity_logging(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            identities = plan["identities"]
            assert isinstance(identities, list)
            row = next(
                value
                for value in identities
                if isinstance(value, dict)
                and isinstance(
                    value.get("security_and_sensitivity"), dict
                )
                and value["security_and_sensitivity"].get(
                    "contains_sensitive_or_potentially_sensitive_fields"
                )
                is True
            )
            row["security_and_sensitivity"][
                "logs_may_contain_values"
            ] = True

        def coherent_union_owner_drift(
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
                    "#/definitions/v2/LoginAccountParams/oneOf/0/"
                    "properties/apiKey"
                )
            )
            row["cpp_owner"] = "typed::LoginAccountParams"
            row["cpp_mapping"] = (
                "typed::LoginAccountParams::apiKey -> std::string"
            )

        def duplicate_cpp_ownership(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            paths = closure["schema_paths"]
            assert isinstance(paths, list)
            first = paths[0]
            second = paths[1]
            assert isinstance(first, dict)
            assert isinstance(second, dict)
            second["cpp_owner"] = first["cpp_owner"]
            second["cpp_member"] = first["cpp_member"]
            second["cpp_type"] = first["cpp_type"]
            second["cpp_mapping"] = first["cpp_mapping"]

        def collapse_optional_nullable(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            paths = closure["schema_paths"]
            assert isinstance(paths, list)
            row = next(
                value
                for value in paths
                if isinstance(value, dict)
                and value.get("presence_model")
                == "OptionalNullable"
            )
            row["presence_model"] = "OptionalValue"

        def descriptor_key_drift(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            identities = plan["identities"]
            assert isinstance(identities, list)
            row = identities[0]
            assert isinstance(row, dict)
            descriptor = row["required_private_codec_or_descriptor"]
            assert isinstance(descriptor, dict)
            descriptor["descriptor_key"] = "wrong"

        def incomplete_positive_fixture_plan(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            identities = plan["identities"]
            assert isinstance(identities, list)
            row = identities[0]
            assert isinstance(row, dict)
            positive = row["required_positive_fixtures"]
            assert isinstance(positive, dict)
            expansion = positive["expansion_obligations"]
            assert isinstance(expansion, dict)
            expansion.pop("nullable_schema_valid_states")

        def wrong_result_kind(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            identities = plan["identities"]
            assert isinstance(identities, list)
            row = next(
                value
                for value in identities
                if isinstance(value, dict)
                and isinstance(value.get("request_contract"), dict)
                and value["request_contract"].get(
                    "result_contract_kind"
                )
                == "Unit"
            )
            contract = row["request_contract"]
            assert isinstance(contract, dict)
            contract["result_contract_kind"] = "Concrete"

        def coherent_result_decoder_drift(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            identities = plan["identities"]
            counts = plan["counts"]
            assert isinstance(identities, list)
            assert isinstance(counts, dict)
            row = next(
                value
                for value in identities
                if isinstance(value, dict)
                and isinstance(value.get("request_contract"), dict)
                and value["protocol_surface_key"].get("name")
                == "account/login/cancel"
            )
            row["request_contract"][
                "authoritative_successful_result_type"
            ] = "WrongAccountResult"
            counts["identity_contract_mapping_sha256"] = (
                self.tool.sha256_json(
                    self.tool.identity_contract_projection(identities)
                )
            )

        def unauthorized_opaque_json(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            paths = closure["schema_paths"]
            assert isinstance(paths, list)
            row = next(
                value
                for value in paths
                if isinstance(value, dict)
                and value.get("disposition")
                != "ProtocolDefinedOpaqueJson"
            )
            row["disposition"] = "ProtocolDefinedOpaqueJson"

        def unauthorized_raw_retention(
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
                    "#/definitions/ChatgptAuthTokensRefreshResponse/"
                    "properties/accessToken"
                )
            )
            row["raw_retention_permitted"] = True

        def coherent_cpp_wrapper_drift(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            paths = closure["schema_paths"]
            counts = closure["counts"]
            assert isinstance(paths, list)
            assert isinstance(counts, dict)
            row = next(
                value
                for value in paths
                if isinstance(value, dict)
                and value.get("schema_path")
                == (
                    "#/definitions/ChatgptAuthTokensRefreshParams/"
                    "properties/previousAccountId"
                )
            )
            row["cpp_base_type"] = "Json"
            row["cpp_type"] = "std::optional<Json>"
            row["cpp_mapping"] = (
                f"{row['cpp_owner']}::{row['cpp_member']} -> "
                "std::optional<Json>"
            )
            counts["schema_path_mapping_sha256"] = (
                self.tool.sha256_json(
                    self.tool.schema_path_mapping_projection(paths)
                )
            )

        def coherent_path_disposition_swap(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            paths = closure["schema_paths"]
            counts = closure["counts"]
            assert isinstance(paths, list)
            assert isinstance(counts, dict)
            open_row = next(
                value
                for value in paths
                if isinstance(value, dict)
                and value.get("schema_path")
                == (
                    "#/definitions/ChatgptAuthTokensRefreshResponse/"
                    "properties/chatgptPlanType"
                )
            )
            reused_row = next(
                value
                for value in paths
                if isinstance(value, dict)
                and value.get("disposition") == "ReusedExistingType"
                and value.get("raw_retention_permitted") is False
            )
            open_row["disposition"], reused_row["disposition"] = (
                reused_row["disposition"],
                open_row["disposition"],
            )
            counts["schema_path_mapping_sha256"] = (
                self.tool.sha256_json(
                    self.tool.schema_path_mapping_projection(paths)
                )
            )

        def coherent_semantic_map_key_drift(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            paths = closure["schema_paths"]
            counts = closure["counts"]
            assert isinstance(paths, list)
            assert isinstance(counts, dict)
            property_path = (
                "#/definitions/v2/"
                "ExperimentalFeatureEnablementSetParams/properties/"
                "enablement"
            )
            for row in paths:
                if not isinstance(row, dict):
                    continue
                if row.get("schema_path") == property_path:
                    row["semantic_map_key_type"] = None
                    row["cpp_base_type"] = "std::map<std::string, bool>"
                    row["cpp_type"] = "std::map<std::string, bool>"
                    row["cpp_mapping"] = (
                        f"{row['cpp_owner']}::{row['cpp_member']} -> "
                        "std::map<std::string, bool>"
                    )
                elif row.get("schema_path") == (
                    property_path + "/additionalProperties"
                ):
                    row["semantic_map_key_type"] = None
            counts["semantic_map_key_types"][
                "typed::ExperimentalFeatureId"
            ] = 4
            counts["schema_path_mapping_sha256"] = (
                self.tool.sha256_json(
                    self.tool.schema_path_mapping_projection(paths)
                )
            )

        def coherent_branch_reachability_drift(
            _: dict[str, object], closure: dict[str, object]
        ) -> None:
            paths = closure["schema_paths"]
            counts = closure["counts"]
            assert isinstance(paths, list)
            assert isinstance(counts, dict)
            mdm = next(
                value
                for value in paths
                if isinstance(value, dict)
                and value.get("schema_path")
                == (
                    "#/definitions/v2/ConfigLayerSource/oneOf/0/"
                    "properties/type"
                )
            )
            enterprise = next(
                value
                for value in paths
                if isinstance(value, dict)
                and value.get("schema_path")
                == (
                    "#/definitions/v2/ConfigLayerSource/oneOf/2/"
                    "properties/type"
                )
            )
            extra_root = next(
                copy.deepcopy(root)
                for root in enterprise["reaching_roots"]
                if root["role"] == "registered_tagged_union_family"
            )
            mdm["reaching_roots"].append(extra_root)
            counts["schema_path_mapping_sha256"] = (
                self.tool.sha256_json(
                    self.tool.schema_path_mapping_projection(paths)
                )
            )

        def wrong_identity_batch(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            identities = plan["identities"]
            assert isinstance(identities, list)
            row = identities[0]
            assert isinstance(row, dict)
            row["owning_implementation_batch"] = "B3"

        def duplicate_replaces_identity(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            identities = plan["identities"]
            assert isinstance(identities, list)
            identities[1] = copy.deepcopy(identities[0])

        def remove_identity(
            plan: dict[str, object], _: dict[str, object]
        ) -> None:
            identities = plan["identities"]
            assert isinstance(identities, list)
            identities.pop(0)

        cases = (
            (
                "identity denominator",
                identity_denominator,
                ("IdentityCountMismatch",),
            ),
            (
                "taxonomy summary",
                taxonomy_summary,
                ("TaxonomyMismatch",),
            ),
            (
                "response-root arithmetic",
                response_arithmetic,
                ("ResponseRootArithmeticMismatch",),
            ),
            (
                "classification count",
                wrong_classification,
                ("AssignmentMismatch",),
            ),
            (
                "module ownership",
                wrong_module,
                (
                    "AssignmentMismatch",
                    "PlanAssignmentMappingMismatch",
                ),
            ),
            (
                "duplicated category field",
                wrong_category_field,
                (
                    "PlanAssignmentMappingMismatch",
                    "PlanIdentityKeyMismatch",
                ),
            ),
            (
                "duplicated domain field",
                wrong_domain_field,
                (
                    "PlanAssignmentMappingMismatch",
                    "PlanIdentityKeyMismatch",
                ),
            ),
            (
                "duplicated discriminator field",
                wrong_discriminator_field,
                (
                    "PlanAssignmentMappingMismatch",
                    "PlanIdentityKeyMismatch",
                ),
            ),
            (
                "coherent classification swap",
                coherent_classification_swap,
                ("PlanAssignmentMappingMismatch",),
            ),
            (
                "live progress status split",
                wrong_live_progress_status,
                (
                    "PlanProgressMappingMismatch",
                    "ProgressStageMismatch",
                    "ProgressStageMismatch",
                ),
            ),
            (
                "coherent wrong-identity B4 progress boundary",
                coherent_live_progress_wrong_identity_boundary,
                ("ProgressIdentityMismatch",),
            ),
            (
                "unstable identity",
                unstable_identity,
                (
                    "PlanAssignmentMappingMismatch",
                    "UnstableIdentity",
                ),
            ),
            (
                "batch dependency cycle",
                batch_self_cycle,
                ("BatchDependencyCycle",),
            ),
            (
                "missing definition disposition",
                missing_definition_disposition,
                (
                    "DefinitionMappingMismatch",
                    "MissingClosureDisposition",
                    "RawRetentionPolicyMismatch",
                ),
            ),
            (
                "missing schema-path disposition",
                missing_path_disposition,
                (
                    "MissingClosureDisposition",
                    "MissingClosureDisposition",
                    "SchemaPathMappingMismatch",
                ),
            ),
            (
                "missing sensitivity policy",
                missing_sensitivity_policy,
                (
                    "SchemaPathMappingMismatch",
                    "SensitivePathPolicyMissing",
                ),
            ),
            (
                "sensitive path logging",
                permit_sensitive_path_logging,
                (
                    "SchemaPathMappingMismatch",
                    "SensitivePathPolicyMissing",
                ),
            ),
            (
                "sensitive path frontend exposure",
                permit_sensitive_path_frontend,
                (
                    "SchemaPathMappingMismatch",
                    "SensitivePathPolicyMissing",
                ),
            ),
            (
                "sensitive identity logging",
                permit_sensitive_identity_logging,
                ("SensitiveIdentityPolicyMismatch",),
            ),
            (
                "coherent registered-union owner drift",
                coherent_union_owner_drift,
                (
                    "CppOwnershipMismatch",
                    "SchemaPathMappingMismatch",
                ),
            ),
            (
                "duplicate C++ ownership",
                duplicate_cpp_ownership,
                (
                    "CppOwnershipConflict",
                    "SchemaPathMappingMismatch",
                ),
            ),
            (
                "optional-nullable tri-state collapse",
                collapse_optional_nullable,
                (
                    "OptionalNullableMappingMismatch",
                    "OptionalNullableMappingMismatch",
                    "SchemaPathMappingMismatch",
                ),
            ),
            (
                "descriptor key drift",
                descriptor_key_drift,
                ("DescriptorKeyMismatch",),
            ),
            (
                "incomplete positive fixture expansion",
                incomplete_positive_fixture_plan,
                (
                    "FixturePlanMappingMismatch",
                    "FixturePlanMismatch",
                ),
            ),
            (
                "wrong result contract kind",
                wrong_result_kind,
                (
                    "PlanContractMappingMismatch",
                    "ResultKindMismatch",
                    "ResultKindMismatch",
                ),
            ),
            (
                "coherent result decoder drift",
                coherent_result_decoder_drift,
                ("PlanContractMappingMismatch",),
            ),
            (
                "unauthorized opaque JSON",
                unauthorized_opaque_json,
                (
                    "MissingClosureDisposition",
                    "OpaqueJsonMisuse",
                    "OpaqueJsonMisuse",
                    "RawRetentionPolicyMismatch",
                    "SchemaPathMappingMismatch",
                ),
            ),
            (
                "unauthorized raw retention",
                unauthorized_raw_retention,
                (
                    "RawRetentionPolicyMismatch",
                    "SchemaPathMappingMismatch",
                ),
            ),
            (
                "coherent C++ wrapper drift",
                coherent_cpp_wrapper_drift,
                ("SchemaPathMappingMismatch",),
            ),
            (
                "coherent path disposition swap",
                coherent_path_disposition_swap,
                ("SchemaPathMappingMismatch",),
            ),
            (
                "coherent semantic map-key drift",
                coherent_semantic_map_key_drift,
                (
                    "SchemaPathMappingMismatch",
                    "StrongIdentifierMappingMismatch",
                    "StrongIdentifierMappingMismatch",
                    "StrongIdentifierMappingMismatch",
                ),
            ),
            (
                "coherent union-branch reachability drift",
                coherent_branch_reachability_drift,
                (
                    "ReachabilityRootMismatch",
                    "SchemaPathMappingMismatch",
                ),
            ),
            (
                "wrong identity batch",
                wrong_identity_batch,
                (
                    "BatchAssignmentMismatch",
                    "BatchAssignmentMismatch",
                    "PlanAssignmentMappingMismatch",
                ),
            ),
            (
                "duplicate replaces exact identity",
                duplicate_replaces_identity,
                (
                    "ContractMismatch",
                    "DuplicateIdentity",
                    "FixturePlanMappingMismatch",
                    "MissingIdentity",
                    "PlanAssignmentMappingMismatch",
                    "PlanContractMappingMismatch",
                    "PlanProgressMappingMismatch",
                ),
            ),
            (
                "remove exact identity",
                remove_identity,
                (
                    "AssignmentMismatch",
                    "AssignmentMismatch",
                    "BatchAssignmentMismatch",
                    "ContractMismatch",
                    "DirectionalityMismatch",
                    "FixturePlanMappingMismatch",
                    "IdentityCountMismatch",
                    "MissingIdentity",
                    "PlanAssignmentMappingMismatch",
                    "PlanContractMappingMismatch",
                    "PlanProgressMappingMismatch",
                    "ProgressStageMismatch",
                    "ProgressStageMismatch",
                    "ResultKindMismatch",
                    "TaxonomyMismatch",
                ),
            ),
        )
        for description, mutate, expected in cases:
            with self.subTest(description=description):
                self.assert_report_mutation(mutate, expected)

    def test_intrinsic_evidence_diagnostic_codes(self) -> None:
        manifest = load_json(OPTIONS.manifest)
        assignments = load_json(OPTIONS.assignments)
        reachability = load_json(OPTIONS.reachability)
        contracts = load_json(OPTIONS.contracts)
        registry = self.tool.surface.parse_registry_data(
            OPTIONS.registry
        )

        duplicate_assignments = copy.deepcopy(assignments)
        assignment_rows = duplicate_assignments["assignments"]
        assert isinstance(assignment_rows, list)
        assignment_rows.append(copy.deepcopy(assignment_rows[0]))
        diagnostics = (
            self.tool.surface.assignment_reachability_diagnostics(
                manifest, duplicate_assignments, reachability
            )
        )
        self.assertEqual(
            ["DuplicateModuleSliceAssignment"],
            [diagnostic["code"] for diagnostic in diagnostics],
        )

        mutated_reachability = copy.deepcopy(reachability)
        reachability_rows = mutated_reachability["records"]
        assert isinstance(reachability_rows, list)
        reachability_row = reachability_rows[0]
        assert isinstance(reachability_row, dict)
        reachability_row["reaching_root_count"] += 1
        diagnostics = (
            self.tool.surface.assignment_reachability_diagnostics(
                manifest, assignments, mutated_reachability
            )
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
            and isinstance(row.get("surface_key"), dict)
            and row["surface_key"].get("name")
            == "account/login/cancel"
        )
        contract["parameter_type_identity"] = "WrongAccountParams"
        diagnostics = self.tool.surface.association_diagnostics(
            manifest, mutated_contracts, registry
        )
        self.assertEqual(
            ["WrongParameterType"],
            [diagnostic["code"] for diagnostic in diagnostics],
        )

    def test_start_state_refuses_wrong_base_and_overwrite(self) -> None:
        arguments = copy.copy(self.arguments)
        arguments.base_sha = "0" * 40
        with self.assertRaises(self.tool.AuditError) as wrong_base:
            self.tool.freeze_start_state(arguments)
        self.assertEqual("BaseShaMismatch", wrong_base.exception.code)
        self.assertEqual(
            ("BaseShaMismatch",), wrong_base.exception.codes
        )

        arguments.base_sha = START_BASE_SHA
        with self.assertRaises(self.tool.AuditError) as overwrite:
            self.tool.freeze_start_state(arguments)
        self.assertEqual(
            "StartStateOverwriteRefused", overwrite.exception.code
        )
        self.assertEqual(
            ("StartStateOverwriteRefused",),
            overwrite.exception.codes,
        )

    def test_stale_generated_audit_has_exact_code(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="snodec-codex-a1-2-audit-"
        ) as raw:
            for name, document in (
                ("plan.json", self.plan),
                ("closure.json", self.closure),
            ):
                with self.subTest(artifact=name):
                    path = Path(raw) / name
                    self.tool.write_or_check(path, document, False)
                    self.tool.write_or_check(path, document, True)
                    path.write_text(
                        path.read_text(encoding="utf-8") + " ",
                        encoding="utf-8",
                    )
                    with self.assertRaises(self.tool.AuditError) as caught:
                        self.tool.write_or_check(path, document, True)
                    self.assertEqual(
                        "StaleGeneratedAudit", caught.exception.code
                    )
                    self.assertEqual(
                        ("StaleGeneratedAudit",), caught.exception.codes
                    )


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
    parser.add_argument("--fixture-index", required=True, type=Path)
    parser.add_argument("--draft07-validator", required=True, type=Path)
    parser.add_argument("--start-state", required=True, type=Path)
    parser.add_argument("--plan-output", required=True, type=Path)
    parser.add_argument("--closure-output", required=True, type=Path)
    return parser.parse_args()


if __name__ == "__main__":
    OPTIONS = parse_options()
    unittest.main(argv=[sys.argv[0]])
