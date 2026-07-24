#!/usr/bin/env python3
"""Generate the frozen Phase A1.2 account/model/configuration audits.

The reports generated here are review and closure evidence.  Runtime
disposition and dispatch remain exclusively owned by
``ProtocolSurfaceRegistryData.inc``.

This module deliberately reuses the schema catalog, namespace-aware
definition graph, Draft-07 validator, fixture synthesis primitives, registry
parser, and association validators already used by the A1.0/A1.1 tooling.  It
contains only the reviewed A1.2 slice facts and the small amount of
slice-specific report assembly needed for the frozen 45-identity audit.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence

sys.dont_write_bytecode = True

import app_server_a1_1 as a11
import app_server_a1_shared as shared
import app_server_fixtures as fixture_tool
import app_server_schema_paths as schema_walk
import app_server_surface as surface


FORMAT_VERSION = 1
CODEX_VERSION = "codex-cli 0.144.6"
A1_2_SLICE = "A1.2"
EXPECTED_START_BASE_SHA = "9f7b2955d017cab189fb7b7a80211d0c2788f819"
PREREQUISITE_TIP_SHA = "d32d1cb03aea86b9dcd8deb8078f02a5cee7e5ad"
START_TREE_SHA = "adb042243c7d0942fc5a702fd56638f34dce931f"
START_STATE_GENERATED_NOTICE = (
    "Frozen by tools/codex/app_server_a1_2.py freeze-start-state; "
    "do not edit or regenerate after Commit 1."
)
START_STATE_AUTHORITY_NOTE = (
    "Immutable review evidence captured from the verified merged A1.1 "
    "master. Runtime/status values are snapshots of the sole canonical "
    "ProtocolSurfaceRegistry and are never runtime inputs."
)
EXPECTED_START_CAPTURE_SOURCES_SHA256 = (
    "75cb6b6996e46dca5fbf5c9f865156062d77607e52dda41b9387e276f6bb2cb9"
)
EXPECTED_START_STATE_DOCUMENT_SHA256 = (
    "fe8cb2ec653e4f87636f358b9eef6dd0504b846e052202795840b41cddc5ceab"
)

AuditError = shared.AuditError
AuditDiagnostic = shared.AuditDiagnostic
Key = shared.Key

EXPECTED_TAXONOMY = {
    "client_request": 18,
    "server_notification": 7,
    "server_request": 1,
    "tagged_union_discriminator": 19,
}
EXPECTED_CLASSIFICATIONS = {
    "RootOwnedNestedUnion": 11,
    "SharedWithinSlice": 8,
    "StablePublicRoot": 26,
}
EXPECTED_MODULES = {"AccountsModelsConfiguration": 45}
EXPECTED_START_SCHEMA_STATUS = {"NotImplemented": 43, "Partial": 2}
EXPECTED_START_IMPLEMENTATION_STATUS = {
    "Implemented": 2,
    "NotImplemented": 43,
}
EXPECTED_PROGRESS_SCHEMA_STATUS = {
    "Start": EXPECTED_START_SCHEMA_STATUS,
    "B2": {"Complete": 24, "NotImplemented": 20, "Partial": 1},
    "B3": {"Complete": 29, "NotImplemented": 16},
    "B4": {"Complete": 40, "NotImplemented": 5},
    "B5": {"Complete": 45},
}
EXPECTED_PROGRESS_IMPLEMENTATION_STATUS = {
    "Start": EXPECTED_START_IMPLEMENTATION_STATUS,
    "B2": {"Implemented": 25, "NotImplemented": 20},
    "B3": {"Implemented": 29, "NotImplemented": 16},
    "B4": {"Implemented": 40, "NotImplemented": 5},
    "B5": {"Implemented": 45},
}
EXPECTED_START_PARTIAL_IDENTITIES = frozenset(
    {
        Key(
            "server_notification",
            "ServerNotification",
            "method",
            "model/rerouted",
        ),
        Key(
            "server_request",
            "ServerRequest",
            "method",
            "account/chatgptAuthTokens/refresh",
        ),
    }
)
EXPECTED_START_GLOBAL_STATUS = {
    "Complete": 167,
    "NotApplicable": 48,
    "NotImplemented": 164,
    "Partial": 8,
}
EXPECTED_FINAL_GLOBAL_STATUS = {
    "Complete": 212,
    "NotApplicable": 48,
    "NotImplemented": 121,
    "Partial": 6,
}
EXPECTED_RESULT_KINDS = {"Concrete": 16, "Unit": 2}
EXPECTED_UNIT_METHODS = {
    "account/logout",
    "config/mcpServer/reload",
}
EXPECTED_BATCH_IDENTITY_COUNTS = {"B2": 24, "B3": 5, "B4": 11, "B5": 5}
EXPECTED_BATCH_DEFINITION_COUNTS = {"B2": 42, "B3": 16, "B4": 32, "B5": 14}
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
    "B5": EXPECTED_FINAL_GLOBAL_STATUS,
}
EXPECTED_IDENTITY_DIRECTIONS = {
    "Both": 1,
    "DecodeOnly": 22,
    "EncodeOnly": 22,
}
EXPECTED_DEFINITION_DIRECTIONS = {"DecodeOnly": 87, "EncodeOnly": 17}
EXPECTED_DEFINITION_SHAPES = {
    "object": 67,
    "open_string_enum": 29,
    "scalar_alias": 2,
    "tagged_or_untagged_union": 6,
}
EXPECTED_DEFINITION_DISPOSITIONS = {
    "OpenStringEnum": 26,
    "PublicHandwrittenType": 70,
    "ReusedExistingType": 8,
}
EXPECTED_CROSS_SLICE_OWNERSHIP = {
    "A12ForwardShared": 1,
    "A12Local": 97,
    "PriorSliceReuse": 6,
}
EXPECTED_SCHEMA_PATH_COUNT = 302
EXPECTED_SCHEMA_PATH_KINDS = {
    "array_element": 21,
    "map_value": 9,
    "property": 272,
}
EXPECTED_SCHEMA_PATH_DIRECTIONS = {"DecodeOnly": 260, "EncodeOnly": 42}
EXPECTED_SCHEMA_PATH_DISPOSITIONS = {
    "OpenStringEnum": 2,
    "PrivateCodecHelper": 19,
    "ProtocolDefinedOpaqueJson": 7,
    "PublicHandwrittenType": 47,
    "ReusedExistingType": 185,
    "StrongIdentifier": 42,
}
EXPECTED_PROPERTY_PRESENCE_MODELS = {
    "DefaultedValue": 11,
    "OptionalNullable": 117,
    "OptionalValue": 5,
    "RequiredNullable": 5,
    "RequiredValue": 134,
}
EXPECTED_SEMANTIC_MAP_KEY_TYPES = {
    "typed::ConfigKeyPath": 2,
    "typed::ExperimentalFeatureId": 6,
    "typed::PermissionProfileName": 2,
    "typed::RateLimitId": 2,
}
EXPECTED_DEFINITION_MAPPING_SHA256 = (
    "7941a6b74955affd9f7cc9bd8284b6e699c1f491b0ef24f8e6e0f924e13e073e"
)
EXPECTED_IDENTITY_ASSIGNMENT_MAPPING_SHA256 = (
    "2a0963ab932ff4a01455977e22ec65a9338c3f69e7f70e72a6c4212de4713358"
)
EXPECTED_IDENTITY_START_MAPPING_SHA256 = (
    "a5f14f451e72d39650567ded59f6fa5766a3f4eeece9aa51ae8e623daa374d75"
)
EXPECTED_IDENTITY_CONTRACT_MAPPING_SHA256 = (
    "069a165250d9dbe4b2d496cf69ec4bb21c48f95ce5779f969b81c90e69086bb9"
)
EXPECTED_IDENTITY_FIXTURE_MAPPING_SHA256 = (
    "e5fc8a66b996b86af96f51367d8ffbc1b910a4f29e7b654bc0a191ab898dacb2"
)
EXPECTED_SCHEMA_PATH_MAPPING_SHA256 = (
    "62a17f36b0595b8634484f7136450b8a8a4780427a34affb899ac1db7d4ce383"
)

EXPECTED_PARTIAL_KEYS = {
    Key("server_notification", "ServerNotification", "method", "model/rerouted"),
    Key(
        "server_request",
        "ServerRequest",
        "method",
        "account/chatgptAuthTokens/refresh",
    ),
}
EXPECTED_FINAL_RESIDUAL_PARTIAL_KEYS = {
    Key("client_notification", "ClientNotification", "method", "initialized"),
    Key("client_request", "ClientRequest", "method", "initialize"),
    Key("server_notification", "ServerNotification", "method", "error"),
    Key(
        "server_request",
        "ServerRequest",
        "method",
        "item/commandExecution/requestApproval",
    ),
    Key(
        "server_request",
        "ServerRequest",
        "method",
        "item/fileChange/requestApproval",
    ),
    Key(
        "server_request",
        "ServerRequest",
        "method",
        "item/tool/requestUserInput",
    ),
}

ACCOUNTS_REQUESTS = {
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
MODELS_REQUESTS = {
    "model/list",
    "modelProvider/capabilities/read",
}
CONFIG_READ_REQUESTS = {
    "config/read",
    "configRequirements/read",
}
CONFIG_MUTATION_FEATURE_REQUESTS = {
    "config/batchWrite",
    "config/mcpServer/reload",
    "config/value/write",
    "experimentalFeature/enablement/set",
    "experimentalFeature/list",
}
CLIENT_REQUESTS = (
    ACCOUNTS_REQUESTS
    | MODELS_REQUESTS
    | CONFIG_READ_REQUESTS
    | CONFIG_MUTATION_FEATURE_REQUESTS
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
AUTH_REFRESH_METHOD = "account/chatgptAuthTokens/refresh"

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

OPERATION_API_NAMES = {
    "account/login/cancel": ("typed::Accounts", "cancelLogin"),
    "account/login/start": ("typed::Accounts", "startLogin"),
    "account/logout": ("typed::Accounts", "logout"),
    "account/rateLimitResetCredit/consume": (
        "typed::Accounts",
        "consumeRateLimitResetCredit",
    ),
    "account/rateLimits/read": ("typed::Accounts", "readRateLimits"),
    "account/read": ("typed::Accounts", "read"),
    "account/sendAddCreditsNudgeEmail": (
        "typed::Accounts",
        "sendAddCreditsNudgeEmail",
    ),
    "account/usage/read": ("typed::Accounts", "readUsage"),
    "account/workspaceMessages/read": (
        "typed::Accounts",
        "readWorkspaceMessages",
    ),
    "config/batchWrite": ("typed::Configuration", "batchWrite"),
    "config/mcpServer/reload": (
        "typed::Configuration",
        "reloadMcpServers",
    ),
    "config/read": ("typed::Configuration", "read"),
    "config/value/write": ("typed::Configuration", "writeValue"),
    "configRequirements/read": (
        "typed::Configuration",
        "readRequirements",
    ),
    "experimentalFeature/enablement/set": (
        "typed::Configuration",
        "setExperimentalFeatureEnablement",
    ),
    "experimentalFeature/list": (
        "typed::Configuration",
        "listExperimentalFeatures",
    ),
    "model/list": ("typed::Models", "list"),
    "modelProvider/capabilities/read": (
        "typed::Models",
        "readProviderCapabilities",
    ),
}
READ_ONLY_INTEGRATION_METHODS = {
    "account/rateLimits/read",
    "account/read",
    "account/usage/read",
    "account/workspaceMessages/read",
    "config/read",
    "configRequirements/read",
    "experimentalFeature/list",
    "model/list",
    "modelProvider/capabilities/read",
}

NOTIFICATION_PAYLOAD_TYPES = {
    "account/login/completed": "AccountLoginCompletedNotification",
    "account/rateLimits/updated": "AccountRateLimitsUpdatedNotification",
    "account/updated": "AccountUpdatedNotification",
    "configWarning": "ConfigWarningNotification",
    "model/rerouted": "ModelReroutedNotification",
    "model/safetyBuffering/updated": (
        "ModelSafetyBufferingUpdatedNotification"
    ),
    "model/verification": "ModelVerificationNotification",
}

UNION_ALTERNATIVES = {
    ("Account", "amazonBedrock"): "AmazonBedrockAccount",
    ("Account", "apiKey"): "ApiKeyAccount",
    ("Account", "chatgpt"): "ChatgptAccount",
    (
        "ConfigLayerSource",
        "enterpriseManaged",
    ): "EnterpriseManagedConfigLayerSource",
    (
        "ConfigLayerSource",
        "legacyManagedConfigTomlFromFile",
    ): "LegacyManagedConfigTomlFromFileConfigLayerSource",
    (
        "ConfigLayerSource",
        "legacyManagedConfigTomlFromMdm",
    ): "LegacyManagedConfigTomlFromMdmConfigLayerSource",
    ("ConfigLayerSource", "mdm"): "MdmConfigLayerSource",
    ("ConfigLayerSource", "project"): "ProjectConfigLayerSource",
    ("ConfigLayerSource", "sessionFlags"): "SessionFlagsConfigLayerSource",
    ("ConfigLayerSource", "system"): "SystemConfigLayerSource",
    ("ConfigLayerSource", "user"): "UserConfigLayerSource",
    ("LoginAccountParams", "apiKey"): "ApiKeyLoginAccountParams",
    ("LoginAccountParams", "chatgpt"): "ChatgptLoginAccountParams",
    (
        "LoginAccountParams",
        "chatgptAuthTokens",
    ): "ChatgptAuthTokensLoginAccountParams",
    (
        "LoginAccountParams",
        "chatgptDeviceCode",
    ): "ChatgptDeviceCodeLoginAccountParams",
    ("LoginAccountResponse", "apiKey"): "ApiKeyLoginAccountResponse",
    ("LoginAccountResponse", "chatgpt"): "ChatgptLoginAccountResponse",
    (
        "LoginAccountResponse",
        "chatgptAuthTokens",
    ): "ChatgptAuthTokensLoginAccountResponse",
    (
        "LoginAccountResponse",
        "chatgptDeviceCode",
    ): "ChatgptDeviceCodeLoginAccountResponse",
}
UNION_UNKNOWN_ALTERNATIVES = {
    "Account": "UnknownAccount",
    "ConfigLayerSource": "UnknownConfigLayerSource",
    "LoginAccountParams": "UnknownLoginAccountParams",
    "LoginAccountResponse": "UnknownLoginAccountResponse",
}
UNION_BRANCH_ORDER = {
    "Account": ("apiKey", "chatgpt", "amazonBedrock"),
    "ConfigLayerSource": (
        "mdm",
        "system",
        "enterpriseManaged",
        "user",
        "project",
        "sessionFlags",
        "legacyManagedConfigTomlFromFile",
        "legacyManagedConfigTomlFromMdm",
    ),
    "LoginAccountParams": (
        "apiKey",
        "chatgpt",
        "chatgptDeviceCode",
        "chatgptAuthTokens",
    ),
    "LoginAccountResponse": (
        "apiKey",
        "chatgpt",
        "chatgptDeviceCode",
        "chatgptAuthTokens",
    ),
}

PRIOR_SLICE_REUSED_DEFINITIONS = {
    fixture_tool.DefinitionId("v2", "AbsolutePathBuf"),
    fixture_tool.DefinitionId("v2", "ApprovalsReviewer"),
    fixture_tool.DefinitionId("v2", "AskForApproval"),
    fixture_tool.DefinitionId("v2", "ReasoningEffort"),
    fixture_tool.DefinitionId("v2", "ReasoningSummary"),
    fixture_tool.DefinitionId("v2", "SandboxMode"),
}
UNIT_RESULT_DEFINITIONS = {
    fixture_tool.DefinitionId("v2", "LogoutAccountResponse"),
    fixture_tool.DefinitionId("v2", "McpServerRefreshResponse"),
}
FORWARD_SHARED_DEFINITIONS = {
    fixture_tool.DefinitionId("v2", "WindowsSandboxSetupMode"),
}

REVIEWED_NON_NULL_DEFAULTS = {
    (
        "#/definitions/v2/Account/oneOf/2/properties/"
        "credentialSource"
    ): "awsManaged",
    "#/definitions/v2/Model/properties/additionalSpeedTiers": [],
    "#/definitions/v2/Model/properties/inputModalities": ["text", "image"],
    "#/definitions/v2/Model/properties/serviceTiers": [],
    "#/definitions/v2/Model/properties/supportsPersonality": False,
    (
        "#/definitions/v2/SandboxWorkspaceWrite/properties/"
        "exclude_slash_tmp"
    ): False,
    (
        "#/definitions/v2/SandboxWorkspaceWrite/properties/"
        "exclude_tmpdir_env_var"
    ): False,
    (
        "#/definitions/v2/SandboxWorkspaceWrite/properties/network_access"
    ): False,
    (
        "#/definitions/v2/SandboxWorkspaceWrite/properties/writable_roots"
    ): [],
    (
        "#/definitions/v2/AskForApproval/oneOf/1/properties/granular/"
        "properties/request_permissions"
    ): False,
    (
        "#/definitions/v2/AskForApproval/oneOf/1/properties/granular/"
        "properties/skill_approval"
    ): False,
}
REVIEWED_NULL_DEFAULTS = {
    "#/definitions/v2/LoginAccountParams/oneOf/1/properties/appBrand",
    "#/definitions/v2/Model/properties/defaultServiceTier",
}

AUTHORIZED_OPAQUE_PATHS = {
    "#/definitions/v2/AnalyticsConfig/additionalProperties": (
        "the pinned schema explicitly permits arbitrary analytics keys and "
        "values with additionalProperties: true"
    ),
    "#/definitions/v2/Config/additionalProperties": (
        "the pinned schema explicitly permits unknown configuration keys with "
        "additionalProperties: true while known fields remain typed"
    ),
    (
        "#/definitions/v2/Config/properties/desktop/"
        "additionalProperties"
    ): (
        "the pinned desktop configuration map explicitly permits arbitrary "
        "JSON values"
    ),
    "#/definitions/v2/ConfigEdit/properties/value": (
        "ConfigBatchWriteParams.ConfigEdit.value is the protocol-defined "
        "arbitrary JSON write value"
    ),
    "#/definitions/v2/ConfigLayer/properties/config": (
        "the pinned schema defines a complete layer config value as arbitrary "
        "JSON while layer metadata remains typed"
    ),
    "#/definitions/v2/ConfigValueWriteParams/properties/value": (
        "config/value/write.value is the protocol-defined arbitrary JSON "
        "write value"
    ),
    "#/definitions/v2/OverriddenMetadata/properties/effectiveValue": (
        "the pinned schema defines effectiveValue without constraints"
    ),
}

EXPECTED_MIXED_OBJECT_UNKNOWN_PROPERTY_CONTAINERS = {
    "#/definitions/v2/AnalyticsConfig/additionalProperties": (
        "typed::AnalyticsConfig::unknownProperties -> "
        "std::map<std::string, Json>"
    ),
    "#/definitions/v2/Config/additionalProperties": (
        "typed::Config::unknownProperties -> "
        "std::map<std::string, Json>"
    ),
}

SEMANTIC_IDENTIFIERS = {
    "#/definitions/ChatgptAuthTokensRefreshParams/properties/previousAccountId": (
        "AccountId"
    ),
    "#/definitions/ChatgptAuthTokensRefreshResponse/properties/chatgptAccountId": (
        "AccountId"
    ),
    "#/definitions/v2/AccountLoginCompletedNotification/properties/loginId": (
        "LoginId"
    ),
    "#/definitions/v2/CancelLoginAccountParams/properties/loginId": "LoginId",
    (
        "#/definitions/v2/ConfigBatchWriteParams/properties/filePath"
    ): "AbsolutePathBuf",
    (
        "#/definitions/v2/ConfigValueWriteParams/properties/filePath"
    ): "AbsolutePathBuf",
    "#/definitions/v2/ConfigWarningNotification/properties/path": (
        "AbsolutePathBuf"
    ),
    (
        "#/definitions/v2/ConsumeAccountRateLimitResetCreditParams/"
        "properties/creditId"
    ): "RateLimitResetCreditId",
    (
        "#/definitions/v2/ConsumeAccountRateLimitResetCreditParams/"
        "properties/idempotencyKey"
    ): "IdempotencyKey",
    (
        "#/definitions/v2/ExperimentalFeatureListParams/properties/threadId"
    ): "ThreadId",
    "#/definitions/v2/Config/properties/model": "ModelId",
    "#/definitions/v2/Config/properties/model_provider": "ModelProviderId",
    "#/definitions/v2/Config/properties/review_model": "ModelId",
    "#/definitions/v2/Config/properties/service_tier": "ModelServiceTierId",
    "#/definitions/v2/ConfigEdit/properties/keyPath": "ConfigKeyPath",
    (
        "#/definitions/v2/ConfigLayerSource/oneOf/2/properties/id"
    ): "ConfigLayerId",
    (
        "#/definitions/v2/ConfigValueWriteParams/properties/keyPath"
    ): "ConfigKeyPath",
    "#/definitions/v2/ExperimentalFeature/properties/name": (
        "ExperimentalFeatureId"
    ),
    (
        "#/definitions/v2/LoginAccountParams/oneOf/3/properties/"
        "chatgptAccountId"
    ): "AccountId",
    (
        "#/definitions/v2/LoginAccountResponse/oneOf/1/properties/loginId"
    ): "LoginId",
    (
        "#/definitions/v2/LoginAccountResponse/oneOf/2/properties/loginId"
    ): "LoginId",
    "#/definitions/v2/Model/properties/id": "ModelId",
    (
        "#/definitions/v2/Model/properties/additionalSpeedTiers/items"
    ): "ModelServiceTierId",
    "#/definitions/v2/Model/properties/defaultServiceTier": (
        "ModelServiceTierId"
    ),
    "#/definitions/v2/Model/properties/model": "ModelId",
    (
        "#/definitions/v2/ModelReroutedNotification/properties/fromModel"
    ): "ModelId",
    (
        "#/definitions/v2/ModelReroutedNotification/properties/threadId"
    ): "ThreadId",
    (
        "#/definitions/v2/ModelReroutedNotification/properties/toModel"
    ): "ModelId",
    (
        "#/definitions/v2/ModelReroutedNotification/properties/turnId"
    ): "TurnId",
    (
        "#/definitions/v2/ModelSafetyBufferingUpdatedNotification/"
        "properties/fasterModel"
    ): "ModelId",
    (
        "#/definitions/v2/ModelSafetyBufferingUpdatedNotification/"
        "properties/model"
    ): "ModelId",
    (
        "#/definitions/v2/ModelSafetyBufferingUpdatedNotification/"
        "properties/threadId"
    ): "ThreadId",
    (
        "#/definitions/v2/ModelSafetyBufferingUpdatedNotification/"
        "properties/turnId"
    ): "TurnId",
    "#/definitions/v2/ModelServiceTier/properties/id": "ModelServiceTierId",
    "#/definitions/v2/ModelUpgradeInfo/properties/model": "ModelId",
    "#/definitions/v2/NewThreadModelDefaults/properties/model": "ModelId",
    "#/definitions/v2/NewThreadModelDefaults/properties/serviceTier": (
        "ModelServiceTierId"
    ),
    (
        "#/definitions/v2/ModelVerificationNotification/properties/threadId"
    ): "ThreadId",
    (
        "#/definitions/v2/ModelVerificationNotification/properties/turnId"
    ): "TurnId",
    "#/definitions/v2/RateLimitResetCredit/properties/id": (
        "RateLimitResetCreditId"
    ),
    "#/definitions/v2/RateLimitSnapshot/properties/limitId": "RateLimitId",
    "#/definitions/v2/WorkspaceMessage/properties/messageId": (
        "WorkspaceMessageId"
    ),
}
SEMANTIC_OPEN_STRING_TYPES = {
    (
        "#/definitions/ChatgptAuthTokensRefreshResponse/properties/"
        "chatgptPlanType"
    ): "PlanType",
    (
        "#/definitions/v2/LoginAccountParams/oneOf/3/properties/"
        "chatgptPlanType"
    ): "PlanType",
}
SEMANTIC_MAP_KEY_TYPES = {
    "#/definitions/v2/ConfigReadResponse/properties/origins": (
        "ConfigKeyPath"
    ),
    (
        "#/definitions/v2/ConfigRequirements/properties/"
        "allowedPermissionProfiles"
    ): "PermissionProfileName",
    (
        "#/definitions/v2/ConfigRequirements/properties/"
        "featureRequirements"
    ): "ExperimentalFeatureId",
    (
        "#/definitions/v2/ExperimentalFeatureEnablementSetParams/"
        "properties/enablement"
    ): "ExperimentalFeatureId",
    (
        "#/definitions/v2/ExperimentalFeatureEnablementSetResponse/"
        "properties/enablement"
    ): "ExperimentalFeatureId",
    (
        "#/definitions/v2/GetAccountRateLimitsResponse/properties/"
        "rateLimitsByLimitId"
    ): "RateLimitId",
}

FROZEN_A1_1_ARTIFACTS = {
    "a1_1_start_state": (
        "tools/codex/app-server-evidence/0.144.6/a1-1-start-state.json",
        "1614414ea6320f8ac5eb47ef928c4bc8de587c9f429f86d7f94ffeb437b6c705",
    ),
    "a1_1_implementation_plan": (
        "tools/codex/app-server-evidence/0.144.6/a1-1-implementation-plan.json",
        "59fbbd1e6c1ffcd5e0064e7ff7e2a6a68673ececac5ccb19d3d46cc57eda7661",
    ),
    "a1_1_type_closure": (
        "tools/codex/app-server-evidence/0.144.6/a1-1-type-closure.json",
        "187db47c37d094b5f22c27cb0a6c1fc656cd0b3e34c63faf9b81ededce2f8bfc",
    ),
    "a1_1_closure_report": (
        "tools/codex/app-server-evidence/0.144.6/a1-1-closure-report.json",
        "be9e5f028de164b57519a956a0b2615b15faa238d889b7f1391b07c3b473fb41",
    ),
    "a1_1_operation_production_coverage": (
        "tools/codex/app-server-evidence/0.144.6/"
        "a1-1-operation-production-coverage.json",
        "9e9cc9a58651d39f2e2b5e9ee9cdb712a6338660029a77814988a7e904ef156e",
    ),
    "a1_1_notification_production_coverage": (
        "tools/codex/app-server-evidence/0.144.6/"
        "a1-1-notification-production-coverage.json",
        "191c9e9152fe6c3a2fd052f542911c510e6643e78bfaf2b293729f9bfe7e62b0",
    ),
}

BATCH_SUBJECTS = {
    "B2": "Complete Codex account and authentication surface",
    "B3": "Complete Codex model and provider surface",
    "B4": "Complete Codex configuration read surface",
    "B5": "Complete Codex configuration mutation and feature surface",
}

VALID_TYPE_DISPOSITIONS = {
    "OpenStringEnum",
    "PrivateCodecHelper",
    "ProtocolDefinedOpaqueJson",
    "PublicHandwrittenType",
    "ReusedExistingType",
    "StrongIdentifier",
}
VALID_DIRECTIONS = {"Both", "DecodeOnly", "EncodeOnly"}


@dataclass
class LiveInputs:
    manifest: dict[str, Any]
    assignments_document: dict[str, Any]
    reachability_document: dict[str, Any]
    contracts_document: dict[str, Any]
    completeness_document: dict[str, Any]
    fixture_document: dict[str, Any]
    registry_rows: list[dict[str, Any]]
    manifest_rows: dict[Key, dict[str, Any]]
    assignments: dict[Key, dict[str, Any]]
    reachability: dict[Key, dict[str, Any]]
    contracts: dict[Key, dict[str, Any]]
    completeness: dict[Key, dict[str, Any]]
    fixture_coverage: dict[Key, dict[str, Any]]
    registry: dict[Key, dict[str, Any]]


fail = shared.fail
require = shared.require
load_json = shared.load_json
canonical_json = shared.canonical_json
sha256_file = shared.sha256_file
sha256_json = shared.sha256_json


def definition_mapping_projection(
    rows: Sequence[Mapping[str, Any]],
) -> list[dict[str, Any]]:
    return [
        {
            "definition_key": row["definition_key"],
            "disposition": row["disposition"],
            "cpp_owner": row["cpp_owner"],
            "directionality": row["directionality"],
            "batch": row["owning_implementation_batch"],
        }
        for row in rows
    ]


def identity_assignment_projection(
    rows: Sequence[Mapping[str, Any]],
) -> list[dict[str, Any]]:
    fields = (
        "protocol_surface_key",
        "category",
        "domain",
        "discriminator_field",
        "stability",
        "deprecated",
        "classification",
        "module",
        "directionality",
        "owning_implementation_batch",
    )
    return [{name: row[name] for name in fields} for row in rows]


def identity_start_projection(
    rows: Sequence[Mapping[str, Any]],
) -> list[dict[str, Any]]:
    fields = (
        "protocol_surface_key",
        "current_runtime_disposition",
        "current_runtime_target",
        "current_implementation_status",
        "current_schema_status",
        "current_schema_completeness",
        "current_fixture_ids",
    )
    return [{name: row[name] for name in fields} for row in rows]


def live_progress_stage(
    schema_status: Mapping[str, int],
    implementation_status: Mapping[str, int],
) -> str | None:
    normalized_schema = dict(sorted(schema_status.items()))
    normalized_implementation = dict(
        sorted(implementation_status.items())
    )
    matches = [
        stage
        for stage in ("Start", "B2", "B3", "B4", "B5")
        if normalized_schema
        == dict(sorted(EXPECTED_PROGRESS_SCHEMA_STATUS[stage].items()))
        and normalized_implementation
        == dict(
            sorted(
                EXPECTED_PROGRESS_IMPLEMENTATION_STATUS[stage].items()
            )
        )
    ]
    return matches[0] if len(matches) == 1 else None


def identity_contract_projection(
    rows: Sequence[Mapping[str, Any]],
) -> list[dict[str, Any]]:
    return [
        {
            "protocol_surface_key": row["protocol_surface_key"],
            "request_contract": row.get("request_contract"),
            "server_request_contract": row.get(
                "server_request_contract"
            ),
        }
        for row in rows
    ]


def identity_fixture_projection(
    rows: Sequence[Mapping[str, Any]],
) -> list[dict[str, Any]]:
    return [
        {
            "protocol_surface_key": row["protocol_surface_key"],
            "required_positive_fixtures": row[
                "required_positive_fixtures"
            ],
            "required_negative_fixtures": row[
                "required_negative_fixtures"
            ],
        }
        for row in rows
    ]


def schema_path_mapping_projection(
    rows: Sequence[Mapping[str, Any]],
) -> list[dict[str, Any]]:
    fields = (
        "schema_path",
        "schema_node_kind",
        "directionality",
        "disposition",
        "cpp_owner",
        "cpp_member",
        "cpp_base_type",
        "cpp_type",
        "schema_value_kind",
        "semantic_map_key_type",
        "required",
        "nullable",
        "defaulted",
        "presence_model",
        "owning_implementation_batch",
        "sensitivity",
        "raw_retention_permitted",
        "reaching_roots",
    )
    return [
        {
            **{name: row[name] for name in fields},
            "map_container": row.get("map_container"),
        }
        for row in rows
    ]


def records(
    document: Mapping[str, Any],
    names: Sequence[str],
    description: str,
) -> list[dict[str, Any]]:
    return shared.records(document, names, description)


def indexed(
    values: Iterable[Mapping[str, Any]], description: str
) -> dict[Key, dict[str, Any]]:
    return shared.indexed(values, description)


def load_live_inputs(arguments: argparse.Namespace) -> LiveInputs:
    common = shared.load_surface_evidence_inputs(
        manifest_path=arguments.manifest,
        assignments_path=arguments.assignments,
        reachability_path=arguments.reachability,
        contracts_path=arguments.contracts,
        completeness_path=arguments.schema_completeness,
        fixture_coverage_path=arguments.fixture_coverage,
        registry_path=arguments.registry,
        allowed_versions=frozenset({"0.144.6", CODEX_VERSION}),
    )
    return LiveInputs(**vars(common))


def frozen_a1_2_keys(
    assignments: Mapping[Key, Mapping[str, Any]],
) -> list[Key]:
    keys = sorted(
        key
        for key, row in assignments.items()
        if row.get("slice") == A1_2_SLICE
    )
    require(
        len(keys) == len(set(keys)) == 45,
        f"A1.2 identity count changed: {len(keys)}",
        "IdentityCountMismatch",
    )
    require(
        all(assignments[key].get("stability") == "stable" for key in keys),
        "A1.2 contains an unstable/experimental-only identity",
        "UnstableIdentity",
    )
    return keys


def identity_batch(key: Key) -> str:
    if (
        key.name in ACCOUNTS_REQUESTS
        or key.name in ACCOUNT_NOTIFICATIONS
        or key.name == AUTH_REFRESH_METHOD
        or (key.domain, key.name) in ACCOUNT_UNIONS
    ):
        return "B2"
    if key.name in MODELS_REQUESTS or key.name in MODEL_NOTIFICATIONS:
        return "B3"
    if (
        key.name in CONFIG_READ_REQUESTS
        or key.name in CONFIG_NOTIFICATIONS
        or (key.domain, key.name) in CONFIG_LAYER_UNIONS
    ):
        return "B4"
    if key.name in CONFIG_MUTATION_FEATURE_REQUESTS:
        return "B5"
    fail(
        f"no reviewed A1.2 implementation batch: {key.compact()}",
        "BatchAssignmentMismatch",
    )


def expected_identity_progress(
    stage: str, key: Key
) -> tuple[str, str]:
    """Return the exact schema/implementation status for one stage row."""

    batch_order = {"B2": 1, "B3": 2, "B4": 3, "B5": 4}
    completed_order = {
        "Start": 0,
        "B2": 1,
        "B3": 2,
        "B4": 3,
        "B5": 4,
    }
    require(
        stage in completed_order,
        f"unknown A1.2 progress stage: {stage}",
        "ProgressStageMismatch",
    )
    if batch_order[identity_batch(key)] <= completed_order[stage]:
        return ("Complete", "Implemented")
    if key in EXPECTED_START_PARTIAL_IDENTITIES:
        return ("Partial", "Implemented")
    return ("NotImplemented", "NotImplemented")


def _source_record(path: Path, repo_root: Path) -> dict[str, str]:
    return {
        "path": path.resolve().relative_to(repo_root).as_posix(),
        "sha256": sha256_file(path),
    }


def start_state_document(
    arguments: argparse.Namespace, base_sha: str
) -> dict[str, Any]:
    inputs = load_live_inputs(arguments)
    keys = frozen_a1_2_keys(inputs.assignments)
    schema_status = dict(
        sorted(
            Counter(
                inputs.registry[key]["typed_schema_status"] for key in keys
            ).items()
        )
    )
    implementation_status = dict(
        sorted(
            Counter(
                inputs.registry[key]["typed_status"] for key in keys
            ).items()
        )
    )
    global_status = dict(
        sorted(
            Counter(
                row["typed_schema_status"] for row in inputs.registry_rows
            ).items()
        )
    )
    partial_keys = {
        key
        for key in keys
        if inputs.registry[key]["typed_schema_status"] == "Partial"
    }
    require(
        schema_status == EXPECTED_START_SCHEMA_STATUS,
        f"A1.2 start schema-status split changed: {schema_status}",
        "StartStateMismatch",
    )
    require(
        implementation_status == EXPECTED_START_IMPLEMENTATION_STATUS,
        "A1.2 start implementation-status split changed: "
        f"{implementation_status}",
        "StartStateMismatch",
    )
    require(
        global_status == EXPECTED_START_GLOBAL_STATUS,
        f"global A1.1 baseline status changed: {global_status}",
        "StartStateMismatch",
    )
    require(
        partial_keys == EXPECTED_PARTIAL_KEYS,
        f"A1.2 partial identity set changed: "
        f"{[key.compact() for key in sorted(partial_keys)]}",
        "StartStateMismatch",
    )

    capture_paths = {
        "assignments": arguments.assignments,
        "fixture_coverage": arguments.fixture_coverage,
        "fixture_index": arguments.fixture_index,
        "manifest": arguments.manifest,
        "operation_contracts": arguments.contracts,
        "reachability": arguments.reachability,
        "registry": arguments.registry,
        "schema_completeness": arguments.schema_completeness,
        "stable_aggregate": (
            arguments.schema_root
            / "stable/codex_app_server_protocol.schemas.json"
        ),
    }
    capture_sources = {
        name: _source_record(path, arguments.repo_root)
        for name, path in sorted(capture_paths.items())
    }
    frozen_a1_1: dict[str, dict[str, str]] = {}
    for name, (relative, expected_hash) in sorted(
        FROZEN_A1_1_ARTIFACTS.items()
    ):
        path = arguments.repo_root / relative
        actual = sha256_file(path)
        require(
            actual == expected_hash,
            f"merged A1.1 artifact changed before A1.2 freeze: {relative}",
            "A11ArtifactDrift",
        )
        frozen_a1_1[name] = {"path": relative, "sha256": actual}

    key_set = set(keys)
    identities = [
        {
            "protocol_surface_key": key.object(),
            "assignment": inputs.assignments[key],
            "registry": inputs.registry[key],
            "schema_completeness": inputs.completeness[key],
            "fixture_coverage": inputs.fixture_coverage[key],
        }
        for key in keys
    ]
    unrelated = [
        {
            "protocol_surface_key": key.object(),
            "registry_projection": a11.registry_projection(
                inputs.registry[key]
            ),
        }
        for key in sorted(set(inputs.registry) - key_set)
    ]
    require(
        len(unrelated) == 342,
        f"unrelated registry denominator changed: {len(unrelated)}",
        "StartStateMismatch",
    )
    return {
        "generated_notice": START_STATE_GENERATED_NOTICE,
        "format_version": FORMAT_VERSION,
        "codex_version": CODEX_VERSION,
        "authority_note": START_STATE_AUTHORITY_NOTE,
        "base_sha": base_sha,
        "prerequisite": {
            "a1_1_pr_tip_sha": PREREQUISITE_TIP_SHA,
            "ancestor_of_base": True,
            "base_tree_sha": START_TREE_SHA,
            "pr_tip_tree_sha": START_TREE_SHA,
            "tree_equivalent": True,
        },
        "capture_sources": capture_sources,
        "frozen_a1_1_artifacts": frozen_a1_1,
        "counts": {
            "identity_count": 45,
            "unrelated_registry_identity_count": 342,
            "a1_2_schema_status": schema_status,
            "a1_2_implementation_status": implementation_status,
            "global_schema_status": global_status,
        },
        "initial_partial_identities": [
            key.object() for key in sorted(partial_keys)
        ],
        "identities": identities,
        "unrelated_registry_identities": unrelated,
    }


def freeze_start_state(arguments: argparse.Namespace) -> None:
    require(
        arguments.base_sha is not None,
        "freeze-start-state requires --base-sha",
        "MissingBaseSha",
    )
    require(
        arguments.base_sha == EXPECTED_START_BASE_SHA,
        "A1.2 start-state base SHA must be "
        f"{EXPECTED_START_BASE_SHA}, got {arguments.base_sha}",
        "BaseShaMismatch",
    )
    if arguments.start_state.exists() and not arguments.force_start_state:
        fail(
            f"refusing to overwrite frozen A1.2 start state: "
            f"{arguments.start_state}",
            "StartStateOverwriteRefused",
        )
    document = start_state_document(arguments, arguments.base_sha)
    arguments.start_state.parent.mkdir(parents=True, exist_ok=True)
    arguments.start_state.write_text(
        canonical_json(document), encoding="utf-8"
    )


def validate_start_state_metadata(document: Mapping[str, Any]) -> None:
    require(
        document.get("generated_notice") == START_STATE_GENERATED_NOTICE
        and document.get("format_version") == FORMAT_VERSION
        and document.get("codex_version") == CODEX_VERSION
        and document.get("authority_note") == START_STATE_AUTHORITY_NOTE
        and document.get("base_sha") == EXPECTED_START_BASE_SHA,
        "frozen A1.2 start-state metadata changed",
        "StartStateMetadataMismatch",
    )
    require(
        document.get("prerequisite")
        == {
            "a1_1_pr_tip_sha": PREREQUISITE_TIP_SHA,
            "ancestor_of_base": True,
            "base_tree_sha": START_TREE_SHA,
            "pr_tip_tree_sha": START_TREE_SHA,
            "tree_equivalent": True,
        },
        "frozen A1.2 prerequisite proof changed",
        "StartStatePrerequisiteMismatch",
    )

    capture_sources = document.get("capture_sources")
    require(
        isinstance(capture_sources, Mapping)
        and sha256_json(capture_sources)
        == EXPECTED_START_CAPTURE_SOURCES_SHA256,
        "frozen A1.2 capture-source path/hash set changed",
        "StartStateCaptureSourceMismatch",
    )
    expected_a1_1 = {
        name: {"path": path, "sha256": digest}
        for name, (path, digest) in sorted(
            FROZEN_A1_1_ARTIFACTS.items()
        )
    }
    require(
        document.get("frozen_a1_1_artifacts") == expected_a1_1,
        "frozen A1.1 artifact path/hash set changed in A1.2 start state",
        "StartStateFrozenA11ArtifactMismatch",
    )
    require(
        document.get("initial_partial_identities")
        == [key.object() for key in sorted(EXPECTED_PARTIAL_KEYS)],
        "frozen A1.2 initial Partial identity set changed",
        "StartStatePartialSetMismatch",
    )
    require(
        sha256_json(document) == EXPECTED_START_STATE_DOCUMENT_SHA256,
        "frozen A1.2 start-state document changed",
        "StartStateDocumentMismatch",
    )


def start_state_by_key(
    document: Mapping[str, Any],
) -> dict[Key, dict[str, Any]]:
    validate_start_state_metadata(document)
    rows = records(document, ("identities",), "A1.2 start state")
    result = indexed(rows, "A1.2 start state")
    require(
        len(result) == 45,
        f"A1.2 start-state identity count changed: {len(result)}",
        "StartStateMismatch",
    )
    counts = document.get("counts")
    require(
        isinstance(counts, Mapping)
        and counts.get("identity_count") == 45
        and counts.get("unrelated_registry_identity_count") == 342
        and counts.get("a1_2_schema_status")
        == EXPECTED_START_SCHEMA_STATUS
        and counts.get("a1_2_implementation_status")
        == EXPECTED_START_IMPLEMENTATION_STATUS
        and counts.get("global_schema_status")
        == EXPECTED_START_GLOBAL_STATUS,
        "A1.2 start-state count/status ratchet changed",
        "StartStateMismatch",
    )
    for key, row in result.items():
        for field in (
            "assignment",
            "registry",
            "schema_completeness",
            "fixture_coverage",
        ):
            value = row.get(field)
            require(
                isinstance(value, Mapping)
                and Key.from_row(value) == key,
                f"A1.2 start-state {field} key drift: {key.compact()}",
                "StartStateMismatch",
            )
    return result


def unrelated_start_state_by_key(
    document: Mapping[str, Any],
) -> dict[Key, dict[str, Any]]:
    rows = records(
        document,
        ("unrelated_registry_identities",),
        "unrelated A1.2 start state",
    )
    result = indexed(rows, "unrelated A1.2 start state")
    require(
        len(result) == 342,
        f"unrelated start-state identity count changed: {len(result)}",
        "StartStateMismatch",
    )
    return result


def validate_live_progress(
    keys: Sequence[Key],
    baseline: Mapping[Key, Mapping[str, Any]],
    unrelated_baseline: Mapping[Key, Mapping[str, Any]],
    inputs: LiveInputs,
) -> None:
    diagnostics: list[AuditDiagnostic] = []

    def add(code: str, key: Key, message: str) -> None:
        diagnostics.append(
            AuditDiagnostic(code, key.compact(), message)
        )

    for key in keys:
        frozen = baseline[key]
        frozen_assignment = frozen["assignment"]
        frozen_registry = frozen["registry"]
        frozen_completeness = frozen["schema_completeness"]
        frozen_fixture = frozen["fixture_coverage"]
        if frozen_assignment != inputs.assignments[key]:
            add(
                "StaticIdentityDrift",
                key,
                "module/slice assignment changed from the frozen base",
            )
        diagnostics.extend(
            shared.registry_transition_diagnostics(
                key,
                frozen_registry,
                inputs.registry[key],
                shared.RegistryTransitionPolicy(
                    scope="A1.2",
                    mutable_layer_fields=(),
                    require_exact_completeness_shape=False,
                    enforce_false_completeness=False,
                    invalid_runtime_targets=frozenset(
                        (None, "", "std::monostate{}")
                    ),
                ),
            )
        )
        diagnostics.extend(
            shared.fixture_transition_diagnostics(
                key,
                frozen_completeness,
                inputs.completeness[key],
                frozen_fixture,
                inputs.fixture_coverage[key],
            )
        )
    live_unrelated = set(inputs.registry) - set(keys)
    if live_unrelated != set(unrelated_baseline):
        diagnostics.append(
            AuditDiagnostic(
                "UnrelatedSliceIdentityDrift",
                "registry",
                "non-A1.2 registry exact-key set changed",
            )
        )
    for key in sorted(live_unrelated & set(unrelated_baseline)):
        frozen_projection = unrelated_baseline[key].get(
            "registry_projection"
        )
        if frozen_projection != a11.registry_projection(
            inputs.registry[key]
        ):
            add(
                "UnrelatedSliceDrift",
                key,
                "non-A1.2 registry row changed",
            )

    diagnostics.extend(
        shared.staged_progress_diagnostics(
            keys,
            inputs.registry,
            batch_for_key=identity_batch,
            identity_has_advanced=lambda key: (
                inputs.registry[key]["typed_schema_status"]
                != baseline[key]["registry"]["typed_schema_status"]
                or inputs.registry[key]["typed_status"]
                != baseline[key]["registry"]["typed_status"]
            ),
            incomplete_message=(
                "{batch} advanced before all earlier batches completed"
            ),
        )
    )
    shared.validate_diagnostics(diagnostics)


def definition_path(identity: fixture_tool.DefinitionId) -> str:
    return shared.definition_path(identity.namespace, identity.name)


def _definition_from_reference(reference: str) -> fixture_tool.DefinitionId:
    parts = shared.definition_reference_parts(reference)
    require(
        parts is not None,
        f"unsupported schema reference in A1.2 closure: {reference}",
        "UnresolvedSchemaReference",
    )
    assert parts is not None
    return fixture_tool.DefinitionId(*parts)


def _direct_string_enum_values(
    schema: Mapping[str, Any],
) -> list[str] | None:
    values = schema.get("enum")
    if (
        isinstance(values, list)
        and values
        and all(isinstance(value, str) for value in values)
    ):
        return list(values)
    return None


def _string_enum_union_values(
    schema: Mapping[str, Any],
) -> list[str] | None:
    branches = schema.get("oneOf")
    if not isinstance(branches, list) or not branches:
        return None
    values: list[str] = []
    for branch in branches:
        if not isinstance(branch, Mapping) or branch.get("type") != "string":
            return None
        branch_values = _direct_string_enum_values(branch)
        if branch_values is None:
            return None
        for value in branch_values:
            if value not in values:
                values.append(value)
    return values


def open_string_known_values(
    schema: Mapping[str, Any],
) -> list[str] | None:
    return _direct_string_enum_values(
        schema
    ) or _string_enum_union_values(schema)


def schema_shape(schema: Mapping[str, Any]) -> str:
    if open_string_known_values(schema) is not None:
        return "open_string_enum"
    return a11.schema_shape(schema)


def allows_null(schema: Any) -> bool:
    return shared.allows_null(schema)


def non_null_schema(schema: Any) -> Any:
    return shared.non_null_schema(schema)


def cpp_member_name(name: str) -> str:
    return a11.cpp_member_name(name)


def _pascal(value: str) -> str:
    return a11.pascal(value)


def _cpp_inline_type(owner: str, member: str, suffix: str = "") -> str:
    owner_name = owner.removeprefix("typed::")
    return f"typed::{owner_name}{_pascal(member.replace('[]', ' item'))}{suffix}"


def _reference_cpp_type(identity: fixture_tool.DefinitionId) -> str:
    if identity in UNIT_RESULT_DEFINITIONS:
        return "typed::Unit"
    return f"typed::{identity.name}"


def schema_cpp_base_type(
    schema: Any,
    path: str,
    owner: str,
    member: str,
) -> str:
    def semantic_type(
        _schema: Any,
        candidate_path: str,
        _owner: str,
        _member: str,
    ) -> str | None:
        semantic_open_string = SEMANTIC_OPEN_STRING_TYPES.get(
            candidate_path
        )
        if semantic_open_string is not None:
            return f"typed::{semantic_open_string}"
        semantic = SEMANTIC_IDENTIFIERS.get(candidate_path)
        return f"typed::{semantic}" if semantic is not None else None

    def reference_type(reference: str, _path: str) -> str:
        return _reference_cpp_type(
            _definition_from_reference(reference)
        )

    def special_string_union_type(
        candidate: Mapping[str, Any],
        candidate_owner: str,
        candidate_member: str,
    ) -> str | None:
        if _string_enum_union_values(candidate) is None:
            return None
        return _cpp_inline_type(
            candidate_owner, candidate_member, "Value"
        )

    def map_key_type(candidate_path: str) -> str:
        semantic_key = SEMANTIC_MAP_KEY_TYPES.get(candidate_path)
        return (
            f"typed::{semantic_key}"
            if semantic_key is not None
            else "std::string"
        )

    policy = shared.CppTypePolicy(
        semantic_type=semantic_type,
        reference_type=reference_type,
        inline_type=_cpp_inline_type,
        special_string_union_type=special_string_union_type,
        map_key_type=map_key_type,
        opaque_paths=frozenset(AUTHORIZED_OPAQUE_PATHS),
        allow_unreviewed_true=False,
        allow_empty_schema_json=False,
        allow_annotation_only_json=False,
        nullable_array_elements=False,
        missing_mapping_code="MissingClosureDisposition",
        opaque_misuse_code="OpaqueJsonMisuse",
    )
    return shared.schema_cpp_base_type(
        schema, path, owner, member, policy
    )


def sensitivity_classification(path: str) -> str:
    credential_secret_paths = {
        (
            "#/definitions/ChatgptAuthTokensRefreshResponse/properties/"
            "accessToken"
        ),
        (
            "#/definitions/v2/LoginAccountParams/oneOf/0/properties/"
            "apiKey"
        ),
        (
            "#/definitions/v2/LoginAccountParams/oneOf/3/properties/"
            "accessToken"
        ),
        (
            "#/definitions/v2/LoginAccountResponse/oneOf/3/properties/"
            "accessToken"
        ),
    }
    account_identity_paths = {
        (
            "#/definitions/ChatgptAuthTokensRefreshParams/properties/"
            "previousAccountId"
        ),
        (
            "#/definitions/ChatgptAuthTokensRefreshResponse/properties/"
            "chatgptAccountId"
        ),
        (
            "#/definitions/v2/LoginAccountParams/oneOf/3/properties/"
            "chatgptAccountId"
        ),
        (
            "#/definitions/v2/LoginAccountResponse/oneOf/3/properties/"
            "chatgptAccountId"
        ),
        (
            "#/definitions/v2/Config/properties/"
            "forced_chatgpt_workspace_id"
        ),
        (
            "#/definitions/v2/ConfigRequirements/properties/"
            "allowedChatgptWorkspaceIds"
        ),
    }
    if path in credential_secret_paths:
        return "CredentialSecret"
    if path in account_identity_paths:
        return "AccountWorkspaceIdentity"
    if path == (
        "#/definitions/ChatgptAuthTokensRefreshResponse/properties/"
        "chatgptPlanType"
    ) or path == (
        "#/definitions/v2/LoginAccountParams/oneOf/3/properties/"
        "chatgptPlanType"
    ):
        return "AuthenticationFlowData"
    if path == (
        "#/definitions/v2/LoginAccountResponse/oneOf/2/properties/"
        "userCode"
    ):
        return "EphemeralAuthenticationCode"
    if path.startswith("#/definitions/v2/LoginAccountResponse/"):
        return "AuthenticationFlowData"
    if path.startswith(
        "#/definitions/v2/AccountLoginCompletedNotification/"
    ):
        return "AuthenticationFlowData"
    if path.startswith("#/definitions/v2/Account/"):
        return "PersonallyIdentifyingAccountData"
    if path.startswith("#/definitions/v2/WorkspaceMessage/"):
        return "WorkspaceContent"
    if path.startswith(
        "#/definitions/v2/AccountTokenUsage"
    ) or path.startswith("#/definitions/v2/GetAccountTokenUsageResponse/"):
        return "AccountUsageData"
    if any(
        path.startswith(f"#/definitions/v2/{name}/")
        for name in (
            "AccountRateLimitsUpdatedNotification",
            "ConsumeAccountRateLimitResetCreditResponse",
            "CreditsSnapshot",
            "GetAccountRateLimitsResponse",
            "RateLimitResetCredit",
            "RateLimitResetCreditsSummary",
            "RateLimitSnapshot",
            "RateLimitWindow",
            "SendAddCreditsNudgeEmailResponse",
        )
    ):
        return "AccountUsageData"
    if path in {
        "#/definitions/v2/Config/properties/compact_prompt",
        "#/definitions/v2/Config/properties/developer_instructions",
        "#/definitions/v2/Config/properties/instructions",
        (
            "#/definitions/v2/ConfigWarningNotification/properties/"
            "details"
        ),
    }:
        return "PotentialCredentialBearingConfiguration"
    if path in AUTHORIZED_OPAQUE_PATHS and (
        "/Config" in path
        or "/AnalyticsConfig/" in path
        or "/OverriddenMetadata/" in path
    ):
        return "PotentialCredentialBearingConfiguration"
    if path.startswith("#/definitions/v2/Config/") and (
        path.endswith("/additionalProperties")
        or "/desktop/" in path
    ):
        return "PotentialCredentialBearingConfiguration"
    return "NonSensitiveProtocolData"


SENSITIVITY_PRIORITY = {
    "NonSensitiveProtocolData": 0,
    "AccountUsageData": 10,
    "PersonallyIdentifyingAccountData": 20,
    "WorkspaceContent": 30,
    "AuthenticationFlowData": 40,
    "EphemeralAuthenticationCode": 50,
    "AccountWorkspaceIdentity": 60,
    "PotentialCredentialBearingConfiguration": 70,
    "CredentialSecret": 80,
}


def sensitivity_policy(
    path: str, classification: str | None = None
) -> dict[str, Any]:
    classification = (
        sensitivity_classification(path)
        if classification is None
        else classification
    )
    require(
        classification in SENSITIVITY_PRIORITY,
        f"unreviewed sensitivity classification at {path}: "
        f"{classification}",
        "SensitivePathPolicyMissing",
    )
    sensitive = classification != "NonSensitiveProtocolData"
    return {
        "classification": classification,
        "credential_or_account_value": sensitive,
        "log_value_permitted": not sensitive,
        "diagnostic_value_permitted": False,
        "backend_canonical_state_permitted": False,
        "frontend_protocol_exposure_permitted": False,
        "fixture_value_policy": (
            "deterministic-low-risk-synthetic"
            if sensitive
            else "deterministic-schema-synthetic"
        ),
        "failure_output_policy": "field-path-and-stable-code-only",
    }


def path_disposition(path: str, schema: Any) -> str:
    if path in AUTHORIZED_OPAQUE_PATHS:
        return "ProtocolDefinedOpaqueJson"
    if path in SEMANTIC_OPEN_STRING_TYPES:
        return "OpenStringEnum"
    if path in SEMANTIC_IDENTIFIERS:
        return "StrongIdentifier"
    if isinstance(schema, Mapping):
        if isinstance(schema.get("$ref"), str):
            return "ReusedExistingType"
        if _string_enum_union_values(schema) is not None:
            return "OpenStringEnum"
        enum_values = schema.get("enum")
        if isinstance(enum_values, list):
            return (
                "PrivateCodecHelper"
                if len(enum_values) == 1
                else "OpenStringEnum"
            )
        if any(
            keyword in schema
            for keyword in (
                "properties",
                "oneOf",
                "anyOf",
                "allOf",
            )
        ) or schema.get("type") == "object":
            return "PublicHandwrittenType"
    return "ReusedExistingType"


def schema_value_kind(schema: Any) -> str:
    if schema is True:
        return "arbitrary_json"
    if not isinstance(schema, Mapping):
        return "malformed"
    if isinstance(schema.get("$ref"), str):
        return "reference"
    if "oneOf" in schema or "anyOf" in schema:
        return "union"
    if "allOf" in schema:
        return "composition"
    if "enum" in schema:
        return "string_enum"
    schema_type = schema.get("type")
    if isinstance(schema_type, list):
        non_null = [value for value in schema_type if value != "null"]
        return (
            f"nullable_{non_null[0]}"
            if len(non_null) == 1
            else "type_union"
        )
    if isinstance(schema_type, str):
        return schema_type
    if "properties" in schema:
        return "object"
    return "arbitrary_json"


def _presence_model(
    required: bool, nullable: bool, defaulted: bool
) -> str:
    return shared.property_presence_model(
        required, nullable, defaulted
    )


def _wrap_cpp_type(base: str, presence: str) -> str:
    return shared.wrap_property_cpp_type(base, presence)


def semantic_map_key_type_for_path(path: str) -> str | None:
    owner_path = (
        path.removesuffix("/additionalProperties")
        if path.endswith("/additionalProperties")
        else path
    )
    value = SEMANTIC_MAP_KEY_TYPES.get(owner_path)
    return f"typed::{value}" if value is not None else None


def _registered_union_branch_cpp_owner(
    owner_definition: fixture_tool.DefinitionId,
    branch: Any,
    index: int,
) -> str | None:
    order = UNION_BRANCH_ORDER.get(owner_definition.name)
    if order is None:
        return None
    require(
        owner_definition.namespace == "v2"
        and index < len(order)
        and isinstance(branch, Mapping),
        f"registered union branch shape changed: {owner_definition} "
        f"oneOf/{index}",
        "ReviewedPublicMappingMismatch",
    )
    properties = branch.get("properties")
    discriminator = (
        properties.get("type")
        if isinstance(properties, Mapping)
        else None
    )
    values = (
        discriminator.get("enum")
        if isinstance(discriminator, Mapping)
        else None
    )
    expected_value = order[index]
    require(
        values == [expected_value],
        f"registered union branch order/discriminator changed: "
        f"{owner_definition} oneOf/{index}",
        "ReviewedPublicMappingMismatch",
    )
    alternative = UNION_ALTERNATIVES.get(
        (owner_definition.name, expected_value)
    )
    require(
        alternative is not None,
        f"registered union branch lacks a reviewed public owner: "
        f"{owner_definition} {expected_value}",
        "ReviewedPublicMappingMismatch",
    )
    return f"typed::{alternative}"


def collect_schema_paths(
    nodes: Mapping[fixture_tool.DefinitionId, Any],
    closure: set[fixture_tool.DefinitionId],
    directions: Mapping[fixture_tool.DefinitionId, str],
    batches: Mapping[fixture_tool.DefinitionId, str],
    reaching_roots: Mapping[
        fixture_tool.DefinitionId, Sequence[Mapping[str, Any]]
    ],
) -> list[dict[str, Any]]:
    result: list[dict[str, Any]] = []
    discovered_opaque: set[str] = set()
    discovered_non_null_defaults: dict[str, Any] = {}
    discovered_null_defaults: set[str] = set()

    def subtree_sensitivity(
        schema: Any,
        path: str,
        seen_definitions: frozenset[fixture_tool.DefinitionId],
    ) -> str:
        classifications = [sensitivity_classification(path)]

        def append_reference(
            candidate: Any,
            candidate_path: str,
        ) -> None:
            if not isinstance(candidate, Mapping):
                return
            reference = candidate.get("$ref")
            if not isinstance(reference, str):
                return
            definition = _definition_from_reference(reference)
            if definition not in seen_definitions:
                require(
                    definition in nodes,
                    f"unresolved schema reference while classifying "
                    f"sensitivity: {reference}",
                    "UnresolvedSchemaReference",
                )
                classifications.append(
                    subtree_sensitivity(
                        nodes[definition],
                        definition_path(definition),
                        seen_definitions | {definition},
                    )
                )

        append_reference(schema, path)
        for visit in schema_walk.walk_schema_paths(
            schema,
            path=path,
            state=None,
            skip_references=True,
        ):
            classifications.append(
                sensitivity_classification(visit.path)
            )
            append_reference(visit.schema, visit.path)
        return max(
            classifications,
            key=lambda value: SENSITIVITY_PRIORITY[value],
        )

    def path_reaching_roots(
        path: str,
        owner_definition: fixture_tool.DefinitionId,
    ) -> list[Mapping[str, Any]]:
        roots = list(reaching_roots[owner_definition])
        order = UNION_BRANCH_ORDER.get(owner_definition.name)
        prefix = f"{definition_path(owner_definition)}/oneOf/"
        if order is None or not path.startswith(prefix):
            return roots
        suffix = path.removeprefix(prefix)
        index_text = suffix.split("/", 1)[0]
        require(
            index_text.isdigit() and int(index_text) < len(order),
            f"registered union schema path has an unknown branch: {path}",
            "ReviewedPublicMappingMismatch",
        )
        expected_alternative = order[int(index_text)]
        return [
            root
            for root in roots
            if root.get("role") != "registered_tagged_union_family"
            or (
                isinstance(root.get("surface_key"), Mapping)
                and root["surface_key"].get("domain")
                == owner_definition.name
                and root["surface_key"].get("name")
                == expected_alternative
            )
        ]

    def add(
        path: str,
        kind: str,
        schema: Any,
        owner_definition: fixture_tool.DefinitionId,
        owner: str,
        member: str,
        required: bool | None,
        map_container_member: str | None = None,
        map_container_separates_unknown_fields: bool = False,
    ) -> None:
        disposition = path_disposition(path, schema)
        require(
            disposition in VALID_TYPE_DISPOSITIONS,
            f"invalid path disposition at {path}: {disposition}",
            "MissingClosureDisposition",
        )
        if disposition == "ProtocolDefinedOpaqueJson":
            discovered_opaque.add(path)
        base_type = schema_cpp_base_type(schema, path, owner, member)
        cpp_type = base_type
        nullable = allows_null(schema)
        has_schema_default = (
            isinstance(schema, Mapping) and "default" in schema
        )
        classification = subtree_sensitivity(
            schema, path, frozenset({owner_definition})
        )
        semantic_map_key = semantic_map_key_type_for_path(path)
        row: dict[str, Any] = {
            "schema_path": path,
            "schema_node_kind": kind,
            "directionality": directions[owner_definition],
            "owning_implementation_batch": batches[owner_definition],
            "owning_definition": owner_definition.to_json(),
            "cpp_owner": owner,
            "cpp_member": member,
            "cpp_base_type": base_type,
            "disposition": disposition,
            "schema_value_kind": schema_value_kind(schema),
            "semantic_map_key_type": semantic_map_key,
            "required": required,
            "nullable": nullable,
            "defaulted": has_schema_default,
            "presence_model": (
                "ArrayElementValue"
                if kind == "array_element"
                else "MapValue"
                if kind == "map_value"
                else None
            ),
            "sensitivity": sensitivity_policy(path, classification),
            "raw_retention_permitted": (
                disposition == "ProtocolDefinedOpaqueJson"
            ),
            "reaching_roots": path_reaching_roots(
                path, owner_definition
            ),
        }
        if kind == "map_value":
            require(
                map_container_member is not None,
                f"map value lacks a reviewed container member: {path}",
                "MissingClosureDisposition",
            )
            map_key_type = semantic_map_key or "std::string"
            container_type = (
                f"std::map<{map_key_type}, {base_type}>"
            )
            row["map_container"] = {
                "cpp_owner": owner,
                "cpp_member": map_container_member,
                "cpp_type": container_type,
                "cpp_mapping": (
                    f"{owner}::{map_container_member} -> "
                    f"{container_type}"
                ),
                "separates_unknown_fields_from_known_fields": (
                    map_container_separates_unknown_fields
                ),
            }
        if disposition == "ProtocolDefinedOpaqueJson":
            row["opaque_reason"] = AUTHORIZED_OPAQUE_PATHS[path]
        if path in SEMANTIC_IDENTIFIERS:
            row["semantic_identifier"] = (
                f"typed::{SEMANTIC_IDENTIFIERS[path]}"
            )
        if isinstance(schema, Mapping):
            constraints = {
                name: schema[name]
                for name in (
                    "format",
                    "minimum",
                    "maximum",
                    "minItems",
                    "maxItems",
                    "minLength",
                    "maxLength",
                )
                if name in schema
            }
            if constraints:
                row["schema_constraints"] = constraints
        if required is not None:
            canonical_default = False
            if isinstance(schema, Mapping) and "default" in schema:
                value = schema["default"]
                if value is None:
                    discovered_null_defaults.add(path)
                else:
                    discovered_non_null_defaults[path] = value
                    canonical_default = True
            presence = _presence_model(
                required, nullable, canonical_default
            )
            cpp_type = _wrap_cpp_type(base_type, presence)
            row["presence_model"] = presence
            if isinstance(schema, Mapping) and "default" in schema:
                row["schema_default"] = schema["default"]
                row["default_behavior"] = (
                    "PreserveOmittedNullValue"
                    if schema["default"] is None
                    else "CanonicalValueOnOmission"
                )
        elif allows_null(schema) and base_type != "Json":
            cpp_type = f"std::optional<{base_type}>"
        row["cpp_type"] = cpp_type
        row["cpp_mapping"] = f"{owner}::{member} -> {cpp_type}"
        result.append(row)

    @dataclass(frozen=True)
    class PathState:
        owner_definition: fixture_tool.DefinitionId
        cpp_owner: str
        cpp_member: str
        node_has_known_properties: bool
        map_container_member: str | None = None
        map_separates_unknown_fields: bool = False

    def path_state_transition(
        state: PathState,
        kind: str,
        token: str | int | None,
        path: str,
        child: Any,
        _required: bool | None,
    ) -> PathState:
        child_properties = (
            child.get("properties")
            if isinstance(child, Mapping)
            else None
        )
        child_has_known_properties = (
            isinstance(child_properties, Mapping)
            and bool(child_properties)
        )
        if kind == schema_walk.PROPERTY:
            require(
                isinstance(token, str),
                "shared schema walker returned a non-string property token",
                "MissingClosureDisposition",
            )
            member = cpp_member_name(token)
            qualified_member = (
                f"{state.cpp_member}.{member}"
                if state.cpp_member
                else member
            )
            return PathState(
                state.owner_definition,
                state.cpp_owner,
                qualified_member,
                child_has_known_properties,
            )
        if kind == schema_walk.MAP_VALUE:
            mixed_known_object = state.node_has_known_properties
            container_member = (
                (
                    f"{state.cpp_member}.unknownProperties"
                    if state.cpp_member
                    else "unknownProperties"
                )
                if mixed_known_object
                else (state.cpp_member or "values")
            )
            return PathState(
                state.owner_definition,
                state.cpp_owner,
                f"{container_member}[*]",
                child_has_known_properties,
                container_member,
                mixed_known_object,
            )
        if kind == schema_walk.ARRAY_ELEMENT:
            member = (
                f"{state.cpp_member}[{token}]"
                if isinstance(token, int) and state.cpp_member
                else f"items[{token}]"
                if isinstance(token, int)
                else f"{state.cpp_member}[]"
                if state.cpp_member
                else "items[]"
            )
            return PathState(
                state.owner_definition,
                state.cpp_owner,
                member,
                child_has_known_properties,
            )
        if kind in schema_walk.COMBINATOR_BRANCH_KINDS:
            owner = state.cpp_owner
            member = state.cpp_member
            if (
                kind == schema_walk.ONEOF_BRANCH
                and isinstance(token, int)
                and path.rsplit("/oneOf/", 1)[0]
                == definition_path(state.owner_definition)
            ):
                reviewed_owner = _registered_union_branch_cpp_owner(
                    state.owner_definition, child, token
                )
                if reviewed_owner is not None:
                    owner = reviewed_owner
                    member = ""
            return PathState(
                state.owner_definition,
                owner,
                member,
                child_has_known_properties,
            )
        fail(
            f"shared schema walker returned an unknown edge kind: {kind}",
            "MissingClosureDisposition",
        )

    for identity in sorted(closure):
        owner = _reference_cpp_type(identity)
        schema = nodes[identity]
        properties = (
            schema.get("properties")
            if isinstance(schema, Mapping)
            else None
        )
        initial_state = PathState(
            identity,
            owner,
            "",
            isinstance(properties, Mapping) and bool(properties),
        )
        for visit in schema_walk.walk_schema_paths(
            schema,
            path=definition_path(identity),
            state=initial_state,
            transition=path_state_transition,
            skip_references=True,
        ):
            if visit.kind not in schema_walk.VALUE_PATH_KINDS:
                continue
            state = visit.state
            add(
                visit.path,
                visit.kind,
                visit.schema,
                state.owner_definition,
                state.cpp_owner,
                state.cpp_member,
                visit.required,
                state.map_container_member,
                state.map_separates_unknown_fields,
            )

    require(
        discovered_opaque == set(AUTHORIZED_OPAQUE_PATHS),
        "authorized/discovered protocol opaque paths differ: "
        f"missing={sorted(set(AUTHORIZED_OPAQUE_PATHS)-discovered_opaque)}, "
        f"extra={sorted(discovered_opaque-set(AUTHORIZED_OPAQUE_PATHS))}",
        "OpaqueJsonMisuse",
    )
    require(
        discovered_non_null_defaults == REVIEWED_NON_NULL_DEFAULTS,
        "reviewed non-null defaults changed: "
        f"{discovered_non_null_defaults}",
        "OptionalNullableMappingMismatch",
    )
    require(
        discovered_null_defaults == REVIEWED_NULL_DEFAULTS,
        f"reviewed nullable default:null paths changed: "
        f"{sorted(discovered_null_defaults)}",
        "OptionalNullableMappingMismatch",
    )
    mixed_unknown_containers = {
        str(row["schema_path"]): str(
            row["map_container"]["cpp_mapping"]
        )
        for row in result
        if isinstance(row.get("map_container"), Mapping)
        and row["map_container"].get(
            "separates_unknown_fields_from_known_fields"
        )
        is True
    }
    require(
        mixed_unknown_containers
        == EXPECTED_MIXED_OBJECT_UNKNOWN_PROPERTY_CONTAINERS,
        "mixed known-object additionalProperties containers changed: "
        f"{mixed_unknown_containers}",
        "AdditionalPropertiesRetentionMismatch",
    )
    result.sort(
        key=lambda row: (
            str(row["schema_path"]),
            str(row["schema_node_kind"]),
        )
    )
    return result


def _unit_result_schema_is_empty(
    schema: Mapping[str, Any]
) -> bool:
    structural = {
        key: value
        for key, value in schema.items()
        if key
        not in {
            "$comment",
            "$id",
            "$schema",
            "description",
            "examples",
            "title",
        }
    }
    return structural == {"type": "object"}


def _identity_direction(key: Key) -> str:
    if key.category == "client_request":
        return "EncodeOnly"
    if key.category == "server_notification":
        return "DecodeOnly"
    if key.category == "server_request":
        return "Both"
    require(
        key.category == "tagged_union_discriminator",
        f"unexpected A1.2 identity category: {key.compact()}",
        "DirectionalityMismatch",
    )
    if key.domain == "LoginAccountParams":
        return "EncodeOnly"
    return "DecodeOnly"


def _stable_roots_for_identity(
    key: Key,
    reachability: Mapping[Key, Mapping[str, Any]],
) -> list[dict[str, Any]]:
    if key.category == "tagged_union_discriminator":
        row = reachability.get(key)
        require(
            row is not None,
            f"tagged union lacks reachability: {key.compact()}",
            "ReachabilityMismatch",
        )
        roots = row.get("reaching_roots")
        require(
            isinstance(roots, list),
            f"tagged-union roots malformed: {key.compact()}",
            "ReachabilityMismatch",
        )
        return sorted(
            (dict(root) for root in roots),
            key=lambda root: (
                str(root.get("root_id")),
                str(root.get("role")),
            ),
        )
    return [
        {
            "role": "method",
            "root_id": key.compact(),
            "slice": A1_2_SLICE,
            "surface_key": key.object(),
        }
    ]


def _fixture_plan(
    key: Key,
    frozen_fixture_ids: Sequence[str],
    closure_obligations: Mapping[str, Any],
) -> dict[str, Any]:
    if key.category == "client_request":
        cases = [
            (
                f"operation:client_request:{key.name}:params",
                "client_request_params",
            ),
            (
                f"operation:client_request:{key.name}:result",
                "client_request_result",
            ),
        ]
    elif key.category == "server_notification":
        cases = [
            (
                "baseline:server_notification:ServerNotification:"
                f"method:{key.name}",
                "server_notification_envelope_and_params",
            )
        ]
    elif key.category == "server_request":
        cases = [
            (
                "baseline:server_request:ServerRequest:"
                f"method:{key.name}",
                "server_request_envelope",
            ),
            (
                f"operation:server_request:{key.name}:params",
                "server_request_params",
            ),
            (
                f"operation:server_request:{key.name}:result",
                "server_request_response",
            ),
        ]
    else:
        cases = [
            (
                f"union:{key.domain}:{key.name}",
                "tagged_union_alternative",
            )
        ]
    frozen = set(frozen_fixture_ids)
    required_ids = [fixture_id for fixture_id, _ in cases]
    expansion = closure_obligations.get("positive")
    require(
        isinstance(expansion, Mapping),
        f"shared fixture closure lacks positive obligations: "
        f"{key.compact()}",
        "FixturePlanMismatch",
    )
    return {
        "required_fixture_ids": required_ids,
        "existing_fixture_ids": [
            fixture_id for fixture_id in required_ids if fixture_id in frozen
        ],
        "planned_fixture_ids": [
            fixture_id
            for fixture_id in required_ids
            if fixture_id not in frozen
        ],
        "base_cases": [
            {
                "id": fixture_id,
                "role": role,
                "commit_1_status": (
                    "existing" if fixture_id in frozen else "planned"
                ),
                "independent_draft07_validation_required": True,
                "may_contain_only_synthetic_sensitive_values": True,
            }
            for fixture_id, role in cases
        ],
        "base_cases_are_seeds_not_complete_coverage": True,
        "expansion_obligations": dict(expansion),
        "coverage_completion_rule": (
            "Every base role and every expansion obligation must have an "
            "independently Draft-07-valid fixture assertion before this "
            "identity can become Complete."
        ),
    }


def _negative_fixture_plan(
    key: Key,
    closure_obligations: Mapping[str, Any],
    open_enum_names: Sequence[str],
) -> dict[str, Any]:
    shared_negative = closure_obligations.get("negative")
    require(
        isinstance(shared_negative, Mapping),
        f"shared fixture closure lacks negative obligations: "
        f"{key.compact()}",
        "FixturePlanMismatch",
    )
    has_object_discriminator = (
        key.category == "tagged_union_discriminator"
        and key.field == "type"
    )
    has_incoming_discriminator = _identity_direction(key) in {
        "Both",
        "DecodeOnly",
    }
    return {
        "required_field_removal_schema_paths": list(
            shared_negative["required_field_removal_schema_paths"]
        ),
        "wrong_type_schema_paths": list(
            shared_negative["wrong_type_schema_paths"]
        ),
        "optional_omission_schema_paths": list(
            shared_negative["optional_omission_schema_paths"]
        ),
        "nullable_omitted_null_value_schema_paths": list(
            shared_negative[
                "nullable_omitted_null_value_schema_paths"
            ]
        ),
        "reachable_open_enum_definitions": sorted(open_enum_names),
        "discriminator_cases": {
            "unknown_discriminator": has_incoming_discriminator,
            "wrong_discriminator_type": has_incoming_discriminator,
            "missing_discriminator": has_incoming_discriminator,
            "conflicting_discriminator": (
                has_incoming_discriminator and has_object_discriminator
            ),
            "malformed_known_branch": has_incoming_discriminator,
            "future_nested_enum_retains_known_outer_branch": (
                has_incoming_discriminator and bool(open_enum_names)
            ),
        },
        "numeric_mutations": dict(
            shared_negative["numeric_mutations"]
        ),
        "stable_diagnostic_policy": {
            "unknown": "ForwardCompatibility",
            "malformed_known": (
                "MalformedKnownPayload/ProtocolWarning"
            ),
            "secret_values_in_diagnostics": False,
        },
        "unit_schema_property_mutation_required": (
            key.category == "client_request"
            and key.name in EXPECTED_UNIT_METHODS
        ),
        "runtime_case_ids": [
            f"runtime:{key.compact()}:accepted",
            f"runtime:{key.compact()}:malformed-or-local-rejection",
            f"runtime:{key.compact()}:forward-compatibility",
        ],
    }


def _public_requirement(
    key: Key,
    contract: Mapping[str, Any] | None,
    schema_branch_title: str | None,
) -> dict[str, Any]:
    if key.category == "client_request":
        assert contract is not None
        facade, method = OPERATION_API_NAMES[key.name]
        return {
            "facade": facade,
            "method": method,
            "params_type": (
                "typed::Unit"
                if contract["parameter_type_identity"] == "Unit"
                else f"typed::{contract['parameter_type_identity']}"
            ),
            "successful_result_type": (
                "typed::Unit"
                if contract["result_contract_kind"] == "Unit"
                else f"typed::{contract['result_type_identity']}"
            ),
        }
    if key.category == "server_notification":
        return {
            "canonical_payload_aggregate": (
                f"typed::{NOTIFICATION_PAYLOAD_TYPES[key.name]}"
            ),
            "payload_direction": "incoming",
            "existing_semantic_adapter": (
                "typed::ModelRerouted -> unchanged reducer"
                if key.name == "model/rerouted"
                else None
            ),
            "raw_member_scope": (
                "typed event retains complete envelope internally; "
                "BackendCore preservation receives params only"
            ),
        }
    if key.category == "server_request":
        return {
            "canonical_request_type": (
                "typed::ChatgptAuthTokensRefreshRequest"
            ),
            "canonical_response_type": (
                "typed::ChatgptAuthTokensRefreshResponse"
            ),
            "compatibility_request_type": "typed::AuthenticationRequest",
            "compatibility_response_type": "typed::AuthenticationResponse",
            "response_facade": "typed::Requests",
            "response_method": "respond",
        }
    alternative = UNION_ALTERNATIVES[(key.domain, key.name)]
    return {
        "union": f"typed::{key.domain}",
        "canonical_alternative": f"typed::{alternative}",
        "schema_branch_title": schema_branch_title,
        "future_unknown_alternative": (
            f"typed::{UNION_UNKNOWN_ALTERNATIVES[key.domain]}"
        ),
        "unknown_alternative_direction": (
            "outgoing-explicit-and-rejected-before-enqueue"
            if key.domain == "LoginAccountParams"
            else "incoming-with-raw-json-retention"
        ),
        "compatibility_aliases": [],
    }


def _private_requirement(
    key: Key, direction: str
) -> dict[str, Any]:
    if key.category == "client_request":
        family = "ClientOperationDescriptor"
    elif key.category == "server_notification":
        family = "ServerNotificationDescriptor"
    elif key.category == "server_request":
        family = "ServerRequestDescriptor"
    else:
        family = "AccountsModelsConfigurationUnionDescriptor"
    return {
        "descriptor_family": family,
        "descriptor_key": key.compact(),
        "directionality": direction,
        "registry_lookup_rule": (
            "lookup begins with the exact canonical "
            "ProtocolSurfaceRegistry row"
        ),
        "runtime_state_owner": "AppServerClient::RawProtocol",
        "independent_pending_state_permitted": False,
    }


def _backend_behavior(key: Key) -> str:
    if key.category == "client_request":
        return "NotApplicable: local typed RawProtocol API; no BackendCommand"
    if key.category == "server_request":
        return (
            "retain existing pending authentication request/response path; "
            "no new canonical state"
        )
    if key.category == "server_notification":
        if key.name == "model/rerouted":
            return (
                "retain exact existing model-rerouted reducer and bounded "
                "history semantics"
            )
        return (
            "typed event -> preserveUnmodeledTypedEvent -> "
            "CodexExtensionReceived(params only) -> bounded extension state"
        )
    return "NotApplicable directly; verified through reaching stable roots"


def _frontend_behavior(key: Key) -> str:
    if key.category == "server_request":
        return (
            "existing authentication Frontend Protocol fields only; "
            "no new operation or response shape"
        )
    if key.category == "server_notification":
        if key.name == "model/rerouted":
            return "existing model-rerouted behavior remains unchanged"
        return (
            "existing bounded/redacted codex.extension.params only; "
            "never the complete envelope"
        )
    return (
        "no Frontend Protocol operation, state, command, or security "
        "decision"
    )


def _compatibility_note(key: Key) -> str:
    if key.category == "server_request":
        return (
            "preserve AuthenticationRequest/AuthenticationResponse and "
            "Requests::respond compatibility with explicit tri-state "
            "canonical adapters"
        )
    if key.name == "model/rerouted":
        return (
            "complete payload while preserving thread/turn/model/reason and "
            "bounded history semantics"
        )
    if key.category == "client_request":
        return (
            "additive grouped-facade API; no direct AppServerClient "
            "forwarder"
        )
    return (
        "additive typed payload/alternative; no new BackendCore or "
        "Frontend Protocol semantics"
    )


def _identity_security(
    path_rows: Sequence[Mapping[str, Any]]
) -> dict[str, Any]:
    classifications = sorted(
        {
            str(row["sensitivity"]["classification"])
            for row in path_rows
            if isinstance(row.get("sensitivity"), Mapping)
        }
    )
    sensitive = [
        name for name in classifications if name != "NonSensitiveProtocolData"
    ]
    return {
        "classifications": classifications,
        "contains_sensitive_or_potentially_sensitive_fields": bool(
            sensitive
        ),
        "sensitive_classifications": sensitive,
        "diagnostics_may_contain_values": False,
        "logs_may_contain_values": False if sensitive else True,
        "backend_canonical_state": "forbidden",
        "frontend_protocol_exposure": "forbidden",
        "test_failure_output": "fixture-identity-path-and-code-only",
    }


def _definition_disposition(
    identity: fixture_tool.DefinitionId,
    schema: Mapping[str, Any],
    shape: str,
) -> tuple[str, str]:
    if identity in PRIOR_SLICE_REUSED_DEFINITIONS:
        return "ReusedExistingType", f"typed::{identity.name}"
    if identity in UNIT_RESULT_DEFINITIONS:
        return "ReusedExistingType", "typed::Unit"
    if shape == "open_string_enum":
        return "OpenStringEnum", f"typed::{identity.name}"
    return "PublicHandwrittenType", f"typed::{identity.name}"


def build_reports(
    arguments: argparse.Namespace,
) -> tuple[dict[str, Any], dict[str, Any]]:
    inputs = load_live_inputs(arguments)
    start_state = load_json(arguments.start_state)
    baseline = start_state_by_key(start_state)
    unrelated_baseline = unrelated_start_state_by_key(start_state)
    keys = frozen_a1_2_keys(inputs.assignments)
    require(
        set(keys) == set(baseline),
        "A1.2 assignment/start-state exact-key mismatch",
        "StartStateMismatch",
    )
    validate_live_progress(
        keys, baseline, unrelated_baseline, inputs
    )

    taxonomy = dict(
        sorted(Counter(key.category for key in keys).items())
    )
    require(
        taxonomy == EXPECTED_TAXONOMY,
        f"A1.2 taxonomy changed: {taxonomy}",
        "TaxonomyMismatch",
    )
    require(
        all(
            key.category
            not in {
                "client_notification",
                "delta_progress_discriminator",
                "item_discriminator",
            }
            for key in keys
        ),
        "A1.2 contains a prohibited client notification/item identity",
        "TaxonomyMismatch",
    )
    classifications = dict(
        sorted(
            Counter(
                inputs.assignments[key]["classification"] for key in keys
            ).items()
        )
    )
    modules = dict(
        sorted(
            Counter(
                inputs.assignments[key]["module"] for key in keys
            ).items()
        )
    )
    require(
        classifications == EXPECTED_CLASSIFICATIONS,
        f"A1.2 classifications changed: {classifications}",
        "AssignmentMismatch",
    )
    require(
        modules == EXPECTED_MODULES,
        f"A1.2 modules changed: {modules}",
        "AssignmentMismatch",
    )
    require(
        {key.name for key in keys if key.category == "client_request"}
        == CLIENT_REQUESTS,
        "A1.2 exact client-operation set changed",
        "MissingStableFeatureOperation",
    )
    require(
        {
            "experimentalFeature/enablement/set",
            "experimentalFeature/list",
        }.issubset(CLIENT_REQUESTS),
        "stable experimentalFeature operations are missing",
        "MissingStableFeatureOperation",
    )

    request_keys = [
        key for key in keys if key.category == "client_request"
    ]
    server_request_keys = [
        key for key in keys if key.category == "server_request"
    ]
    require(
        len(request_keys) == 18 and len(server_request_keys) == 1,
        "operation response-root obligation count changed",
        "ResponseRootArithmeticMismatch",
    )
    request_contracts: dict[Key, dict[str, Any]] = {}
    for key in request_keys + server_request_keys:
        contract = inputs.contracts.get(key)
        require(
            contract is not None,
            f"A1.2 method lacks an authoritative contract: {key.compact()}",
            "ContractMismatch",
        )
        request_contracts[key] = contract
        registry_row = inputs.registry[key]
        require(
            registry_row.get("parameter_type_identity")
            == contract.get("parameter_type_identity")
            and registry_row.get("result_type_identity")
            == contract.get("result_type_identity")
            and registry_row.get("result_contract_kind")
            == contract.get("result_contract_kind"),
            f"registry/contract mismatch: {key.compact()}",
            "ContractMismatch",
        )
    result_kinds = dict(
        sorted(
            Counter(
                request_contracts[key]["result_contract_kind"]
                for key in request_keys
            ).items()
        )
    )
    require(
        result_kinds == EXPECTED_RESULT_KINDS,
        f"A1.2 result kinds changed: {result_kinds}",
        "ResultKindMismatch",
    )
    unit_methods = {
        key.name
        for key in request_keys
        if request_contracts[key]["result_contract_kind"] == "Unit"
    }
    require(
        unit_methods == EXPECTED_UNIT_METHODS,
        f"A1.2 Unit methods changed: {sorted(unit_methods)}",
        "ResultKindMismatch",
    )

    draft07 = fixture_tool.load_draft07(arguments.draft07_validator)
    catalog = fixture_tool.SchemaCatalog(arguments.schema_root, draft07)
    aggregate_path = (
        arguments.schema_root
        / "stable/codex_app_server_protocol.schemas.json"
    )
    aggregate = load_json(aggregate_path)
    nodes, edges = fixture_tool.definition_graph(aggregate)

    seed_associations: dict[
        fixture_tool.DefinitionId, list[dict[str, Any]]
    ] = defaultdict(list)
    association_closures: list[
        tuple[dict[str, Any], set[fixture_tool.DefinitionId]]
    ] = []

    def add_seed(
        definition: fixture_tool.DefinitionId,
        role: str,
        key: Key,
        reached_definitions: set[fixture_tool.DefinitionId] | None = None,
    ) -> None:
        require(
            definition in nodes,
            f"schema seed is absent: {definition}",
            "UnresolvedSchemaReference",
        )
        record = {
            "role": role,
            "surface_key": key.object(),
            "batch": identity_batch(key),
        }
        if record not in seed_associations[definition]:
            seed_associations[definition].append(record)
            reached = (
                set(
                    fixture_tool.transitive_definitions(
                        (definition,), edges
                    )
                )
                if reached_definitions is None
                else set(reached_definitions)
            )
            require(
                definition in reached,
                f"schema association closure lost its seed: "
                f"{definition} {key.compact()}",
                "MissingClosureDisposition",
            )
            association_closures.append((record, reached))

    for key in request_keys + server_request_keys:
        contract = request_contracts[key]
        for role, type_identity in (
            (
                "client_request_params"
                if key.category == "client_request"
                else "server_request_params",
                contract["parameter_type_identity"],
            ),
            (
                "client_request_result"
                if key.category == "client_request"
                else "server_request_response",
                contract["result_schema_type_identity"],
            ),
        ):
            if type_identity == "Unit":
                continue
            definition = fixture_tool.locate_definition_for_type(
                catalog, nodes, type_identity
            )
            require(
                definition is not None,
                f"A1.2 schema root unexpectedly resolves to Unit: "
                f"{key.compact()} {role}",
                "UnresolvedSchemaReference",
            )
            add_seed(definition, role, key)
    for key in (
        key for key in keys if key.category == "server_notification"
    ):
        params = inputs.manifest_rows[key].get("params")
        type_identity = (
            params.get("type") if isinstance(params, Mapping) else None
        )
        require(
            isinstance(type_identity, str) and type_identity,
            f"notification lacks params type: {key.compact()}",
            "ContractMismatch",
        )
        definition = fixture_tool.locate_definition_for_type(
            catalog, nodes, type_identity
        )
        require(
            definition is not None,
            f"notification params definition absent: {key.compact()}",
            "UnresolvedSchemaReference",
        )
        add_seed(definition, "server_notification_params", key)
    for key in (
        key
        for key in keys
        if key.category == "tagged_union_discriminator"
    ):
        definition = fixture_tool.DefinitionId("v2", key.domain)
        target = catalog.union_target(key.domain)
        branch_index, _, _, _ = fixture_tool.branch_for_union_identity(
            catalog, target, key.field, key.name
        )
        union_schema = nodes[definition]
        union_branches = (
            union_schema.get("oneOf")
            if isinstance(union_schema, Mapping)
            else None
        )
        require(
            isinstance(union_branches, list)
            and branch_index < len(union_branches),
            f"registered union aggregate branch is absent: "
            f"{key.compact()}",
            "ReviewedPublicMappingMismatch",
        )
        branch = union_branches[branch_index]
        branch_references = fixture_tool.schema_references(branch)
        branch_closure = {definition}
        if branch_references:
            branch_closure.update(
                fixture_tool.transitive_definitions(
                    branch_references, edges
                )
            )
        add_seed(
            definition,
            "registered_tagged_union_family",
            key,
            branch_closure,
        )

    require(
        len(seed_associations) == 40,
        f"A1.2 unique schema seed count changed: "
        f"{len(seed_associations)}",
        "ClosureCountMismatch",
    )
    seed_closures = {
        seed: set(fixture_tool.transitive_definitions((seed,), edges))
        for seed in seed_associations
    }
    closure = set().union(*seed_closures.values())
    require(
        len(closure) == 104,
        f"A1.2 named-definition closure changed: {len(closure)}",
        "ClosureCountMismatch",
    )
    namespace_counts = dict(
        sorted(Counter(item.namespace for item in closure).items())
    )
    require(
        namespace_counts == {"legacy": 3, "v2": 101},
        f"A1.2 closure namespaces changed: {namespace_counts}",
        "ClosureCountMismatch",
    )
    cycle = shared.find_dependency_cycle(closure, edges)
    require(
        cycle is None,
        f"A1.2 schema dependency cycle: {cycle}",
        "BatchDependencyCycle",
    )

    encode_roles = {
        "client_request_params",
        "server_request_response",
    }
    decode_roles = {
        "client_request_result",
        "server_notification_params",
        "server_request_params",
    }
    encode = set().union(
        *(
            seed_closures[seed]
            for seed, associations in seed_associations.items()
            if any(
                association["role"] in encode_roles
                for association in associations
            )
        )
    )
    decode = set().union(
        *(
            seed_closures[seed]
            for seed, associations in seed_associations.items()
            if any(
                association["role"] in decode_roles
                for association in associations
            )
        )
    )
    require(
        encode | decode == closure,
        "registered union introduced a definition with no real direction",
        "DirectionalityMismatch",
    )
    definition_directions = {
        definition: (
            "Both"
            if definition in encode and definition in decode
            else "EncodeOnly"
            if definition in encode
            else "DecodeOnly"
        )
        for definition in closure
    }
    direction_counts = dict(
        sorted(Counter(definition_directions.values()).items())
    )
    require(
        direction_counts == EXPECTED_DEFINITION_DIRECTIONS,
        f"A1.2 definition directions changed: {direction_counts}",
        "DirectionalityMismatch",
    )

    batch_order = {"B2": 2, "B3": 3, "B4": 4, "B5": 5}
    definition_batches: dict[fixture_tool.DefinitionId, str] = {}
    definition_reaching_roots: dict[
        fixture_tool.DefinitionId, list[dict[str, Any]]
    ] = {}
    for definition in sorted(closure):
        associations = [
            association
            for association, reached in association_closures
            if definition in reached
        ]
        require(
            bool(associations),
            f"closure definition has no reaching seed: {definition}",
            "MissingClosureDisposition",
        )
        definition_batches[definition] = min(
            (str(value["batch"]) for value in associations),
            key=batch_order.__getitem__,
        )
        roots_by_key = {
            Key.from_row(value["surface_key"]): value
            for value in associations
        }
        definition_reaching_roots[definition] = [
            {
                "role": roots_by_key[key]["role"],
                "surface_key": key.object(),
            }
            for key in sorted(roots_by_key)
        ]
    identity_definition_closures: dict[
        Key, set[fixture_tool.DefinitionId]
    ] = defaultdict(set)
    for association, reached in association_closures:
        identity_definition_closures[
            Key.from_row(association["surface_key"])
        ].update(reached)
    definition_batch_counts = dict(
        sorted(Counter(definition_batches.values()).items())
    )
    require(
        definition_batch_counts == EXPECTED_BATCH_DEFINITION_COUNTS,
        f"A1.2 definition batches changed: {definition_batch_counts}",
        "BatchAssignmentMismatch",
    )
    batch_dependencies: dict[str, set[str]] = {
        batch: set() for batch in batch_order
    }
    for definition in closure:
        owner_batch = definition_batches[definition]
        for dependency in edges[definition] & closure:
            dependency_batch = definition_batches[dependency]
            require(
                batch_order[dependency_batch]
                <= batch_order[owner_batch],
                f"forward batch dependency: {definition} -> {dependency}",
                "BatchDependencyCycle",
            )
            if dependency_batch != owner_batch:
                batch_dependencies[owner_batch].add(dependency_batch)

    definition_records: list[dict[str, Any]] = []
    for definition in sorted(closure):
        schema = nodes[definition]
        require(
            isinstance(schema, Mapping),
            f"definition schema is malformed: {definition}",
            "MissingClosureDisposition",
        )
        shape = schema_shape(schema)
        disposition, cpp_owner = _definition_disposition(
            definition, schema, shape
        )
        if definition in PRIOR_SLICE_REUSED_DEFINITIONS:
            ownership = "PriorSliceReuse"
        elif definition in FORWARD_SHARED_DEFINITIONS:
            ownership = "A12ForwardShared"
        else:
            ownership = "A12Local"
        raw_retention = (
            definition_directions[definition] == "DecodeOnly"
            and disposition
            in {"PublicHandwrittenType", "ReusedExistingType"}
            and definition not in UNIT_RESULT_DEFINITIONS
        )
        record: dict[str, Any] = {
            "definition_key": definition.to_json(),
            "schema_path": definition_path(definition),
            "schema_shape": shape,
            "disposition": disposition,
            "cpp_owner": cpp_owner,
            "directionality": definition_directions[definition],
            "cross_slice_ownership": ownership,
            "reused_by_later_slices": (
                ["A1.4"]
                if definition
                == fixture_tool.DefinitionId(
                    "v2", "WindowsSandboxSetupMode"
                )
                else []
            ),
            "owning_implementation_batch": (
                definition_batches[definition]
            ),
            "direct_dependencies": [
                item.to_json()
                for item in sorted(edges[definition] & closure)
            ],
            "reaching_roots": definition_reaching_roots[definition],
            "raw_retention": {
                "permitted": raw_retention,
                "scope": (
                    "incoming aggregate exact schema value only; known "
                    "properties remain typed"
                    if raw_retention
                    else "not permitted"
                ),
            },
        }
        if shape == "open_string_enum":
            values = open_string_known_values(schema)
            require(
                isinstance(values, list)
                and values
                and all(isinstance(value, str) for value in values),
                f"open enum values malformed: {definition}",
                "MissingClosureDisposition",
            )
            record["known_values"] = values
            record["future_value_behavior"] = {
                "representation": "open-string-backed",
                "diagnostic_kind": "UnknownEnumValue",
                "diagnostic_severity": "ForwardCompatibility",
                "retains_outer_known_branch": True,
            }
        definition_records.append(record)
    shape_counts = dict(
        sorted(
            Counter(
                row["schema_shape"] for row in definition_records
            ).items()
        )
    )
    disposition_counts = dict(
        sorted(
            Counter(
                row["disposition"] for row in definition_records
            ).items()
        )
    )
    ownership_counts = dict(
        sorted(
            Counter(
                row["cross_slice_ownership"]
                for row in definition_records
            ).items()
        )
    )
    require(
        shape_counts == EXPECTED_DEFINITION_SHAPES,
        f"A1.2 definition shapes changed: {shape_counts}",
        "ClosureCountMismatch",
    )
    require(
        disposition_counts == EXPECTED_DEFINITION_DISPOSITIONS,
        f"A1.2 definition dispositions changed: {disposition_counts}",
        "MissingClosureDisposition",
    )
    require(
        ownership_counts == EXPECTED_CROSS_SLICE_OWNERSHIP,
        f"A1.2 cross-slice ownership changed: {ownership_counts}",
        "CrossSliceOwnershipMismatch",
    )

    path_records = collect_schema_paths(
        nodes,
        closure,
        definition_directions,
        definition_batches,
        definition_reaching_roots,
    )
    path_kind_counts = dict(
        sorted(
            Counter(
                str(row["schema_node_kind"]) for row in path_records
            ).items()
        )
    )
    path_direction_counts = dict(
        sorted(
            Counter(
                str(row["directionality"]) for row in path_records
            ).items()
        )
    )
    require(
        len(path_records) == EXPECTED_SCHEMA_PATH_COUNT,
        f"A1.2 schema-path closure changed: {len(path_records)}",
        "ClosureCountMismatch",
    )
    require(
        path_kind_counts == EXPECTED_SCHEMA_PATH_KINDS,
        f"A1.2 schema-path kinds changed: {path_kind_counts}",
        "ClosureCountMismatch",
    )
    require(
        path_direction_counts == EXPECTED_SCHEMA_PATH_DIRECTIONS,
        f"A1.2 schema-path directions changed: "
        f"{path_direction_counts}",
        "DirectionalityMismatch",
    )

    for definition in UNIT_RESULT_DEFINITIONS:
        require(
            definition in closure
            and isinstance(nodes[definition], Mapping)
            and _unit_result_schema_is_empty(nodes[definition]),
            f"Unit result schema is no longer the reviewed empty object: "
            f"{definition}",
            "UnitSchemaMismatch",
        )

    branch_titles: dict[Key, str | None] = {}
    for key in (
        key
        for key in keys
        if key.category == "tagged_union_discriminator"
    ):
        target = catalog.union_target(key.domain)
        _, branch, _, _ = fixture_tool.branch_for_union_identity(
            catalog, target, key.field, key.name
        )
        title = branch.get("title") if isinstance(branch, Mapping) else None
        require(
            title is None or isinstance(title, str),
            f"union schema branch title malformed: {key.compact()}",
            "ReviewedPublicMappingMismatch",
        )
        branch_titles[key] = title

    definition_by_id = {
        fixture_tool.DefinitionId(
            str(row["definition_key"]["namespace"]),
            str(row["definition_key"]["name"]),
        ): row
        for row in definition_records
    }
    plan_identities: list[dict[str, Any]] = []
    for key in keys:
        baseline_row = baseline[key]
        registry_row = inputs.registry[key]
        assignment = baseline_row["assignment"]
        contract = request_contracts.get(key)
        reached_definitions = identity_definition_closures[key]
        identity_path_rows = [
            row for row in path_records
            if any(
                Key.from_row(root["surface_key"]) == key
                for root in row["reaching_roots"]
            )
        ]
        open_enum_records = [
            {
                "definition_key": definition.to_json(),
                "known_values": list(
                    definition_by_id[definition]["known_values"]
                ),
            }
            for definition in sorted(reached_definitions)
            if definition_by_id[definition]["schema_shape"]
            == "open_string_enum"
        ]
        open_enum_names = [
            str(row["definition_key"]["name"])
            for row in open_enum_records
        ]
        registered_union_records = [
            {
                "definition_key": definition.to_json(),
                "alternatives": list(
                    UNION_BRANCH_ORDER[definition.name]
                ),
            }
            for definition in sorted(reached_definitions)
            if definition.name in UNION_BRANCH_ORDER
        ]
        closure_fixture_obligations = (
            shared.closure_fixture_obligations(
                identity_path_rows,
                open_enum_records,
                registered_union_records,
            )
        )
        direction = _identity_direction(key)
        row: dict[str, Any] = {
            "protocol_surface_key": key.object(),
            "category": key.category,
            "domain": key.domain,
            "discriminator_field": key.field,
            "directionality": direction,
            "stability": assignment["stability"],
            "deprecated": bool(registry_row["deprecated"]),
            "classification": assignment["classification"],
            "module": assignment["module"],
            "reaching_stable_roots": _stable_roots_for_identity(
                key, inputs.reachability
            ),
            "current_runtime_disposition": registry_row[
                "runtime_disposition"
            ],
            "current_runtime_target": registry_row["runtime_target"],
            "current_implementation_status": registry_row[
                "typed_status"
            ],
            "current_schema_status": registry_row[
                "typed_schema_status"
            ],
            "current_schema_completeness": registry_row[
                "schema_completeness"
            ],
            "current_fixture_ids": list(
                inputs.fixture_coverage[key].get(
                    "fixture_ids", []
                )
            ),
            "owning_implementation_batch": identity_batch(key),
            "required_public_type_or_facade": _public_requirement(
                key, contract, branch_titles.get(key)
            ),
            "required_private_codec_or_descriptor": (
                _private_requirement(key, direction)
            ),
            "required_positive_fixtures": _fixture_plan(
                key,
                baseline_row["fixture_coverage"].get(
                    "fixture_ids", []
                ),
                closure_fixture_obligations,
            ),
            "required_negative_fixtures": _negative_fixture_plan(
                key, closure_fixture_obligations, open_enum_names
            ),
            "expected_backend_core_handling": _backend_behavior(key),
            "expected_frontend_behavior": _frontend_behavior(key),
            "security_and_sensitivity": _identity_security(
                identity_path_rows
            ),
            "source_compatibility_consideration": (
                _compatibility_note(key)
            ),
        }
        if key.category == "client_request":
            assert contract is not None
            facade, method = OPERATION_API_NAMES[key.name]
            unit_params = contract["parameter_type_identity"] == "Unit"
            row["request_contract"] = {
                "authoritative_params_type": contract[
                    "parameter_type_identity"
                ],
                "authoritative_successful_result_type": contract[
                    "result_type_identity"
                ],
                "authoritative_result_schema_type": contract[
                    "result_schema_type_identity"
                ],
                "result_contract_kind": contract[
                    "result_contract_kind"
                ],
                "proposed_public_method_name": (
                    f"{facade}::{method}"
                ),
                "params_wire_encoding": (
                    "explicit-json-null-per-existing-RawProtocol-contract"
                    if unit_params
                    else "exact-schema-object"
                ),
                "unit_params_omitted": False if unit_params else None,
                "safe_for_real_read_only_integration": (
                    key.name in READ_ONLY_INTEGRATION_METHODS
                ),
            }
        elif key.category == "server_request":
            assert contract is not None
            row["server_request_contract"] = {
                "authoritative_params_type": contract[
                    "parameter_type_identity"
                ],
                "authoritative_response_type": contract[
                    "result_schema_type_identity"
                ],
                "result_contract_kind": contract[
                    "result_contract_kind"
                ],
                "occurrence_token_owner": (
                    "AppServerClient::RawProtocol pending server-request "
                    "registry; typed::ServerRequestToken"
                ),
                "sensitive_fields": [
                    "accessToken",
                    "chatgptAccountId",
                    "chatgptPlanType",
                    "previousAccountId",
                ],
                "existing_compatibility_types": [
                    "typed::AuthenticationRequest",
                    "typed::AuthenticationResponse",
                ],
                "existing_compatibility_overload": (
                    "typed::Requests::respond(AuthenticationRequest, "
                    "AuthenticationResponse)"
                ),
                "permitted_test_strategy": (
                    "offline fixture, fake transport, and local socket bytes; "
                    "never real token refresh"
                ),
            }
        plan_identities.append(row)

    identity_direction_counts = dict(
        sorted(
            Counter(
                row["directionality"] for row in plan_identities
            ).items()
        )
    )
    require(
        identity_direction_counts == EXPECTED_IDENTITY_DIRECTIONS,
        f"A1.2 identity directions changed: "
        f"{identity_direction_counts}",
        "DirectionalityMismatch",
    )
    identity_batch_counts = dict(
        sorted(
            Counter(
                row["owning_implementation_batch"]
                for row in plan_identities
            ).items()
        )
    )
    require(
        identity_batch_counts == EXPECTED_BATCH_IDENTITY_COUNTS,
        f"A1.2 identity batches changed: {identity_batch_counts}",
        "BatchAssignmentMismatch",
    )

    immutable_paths = {
        "assignments": arguments.assignments,
        "fixture_generator": Path(fixture_tool.__file__).resolve(),
        "generator": Path(__file__).resolve(),
        "manifest": arguments.manifest,
        "operation_contracts": arguments.contracts,
        "reachability": arguments.reachability,
        "shared_a1_validation": Path(shared.__file__).resolve(),
        "shared_schema_path_walker": Path(
            schema_walk.__file__
        ).resolve(),
        "stable_aggregate": aggregate_path,
        "start_state": arguments.start_state,
    }
    sources = {
        name: _source_record(path, arguments.repo_root)
        for name, path in sorted(immutable_paths.items())
    }
    live_progress_inputs = {
        name: {
            "path": path.resolve()
            .relative_to(arguments.repo_root)
            .as_posix(),
            "hash_policy": (
                "intentionally unhashed mutable A1.2 progress input; "
                "validated monotonically against the immutable start state"
            ),
        }
        for name, path in sorted(
            {
                "fixture_coverage": arguments.fixture_coverage,
                "fixture_index": arguments.fixture_index,
                "registry": arguments.registry,
                "schema_completeness": arguments.schema_completeness,
            }.items()
        )
    }

    frozen_global_status = Counter(
        str(row["registry"]["typed_schema_status"])
        for row in baseline.values()
    )
    frozen_global_status.update(
        str(row["registry_projection"]["typed_schema_status"])
        for row in unrelated_baseline.values()
    )
    derived_start_global_status = dict(sorted(frozen_global_status.items()))
    require(
        derived_start_global_status == EXPECTED_START_GLOBAL_STATUS,
        "frozen identity rows do not reproduce the reviewed global start "
        f"metrics: {derived_start_global_status}",
        "StartStateMismatch",
    )

    staged_ratchets: list[dict[str, Any]] = []
    derived_staged_global_status: dict[str, dict[str, int]] = {}
    cumulative: list[Key] = []
    for batch in ("B2", "B3", "B4", "B5"):
        owned = sorted(
            key for key in keys if identity_batch(key) == batch
        )
        for key in owned:
            initial_status = str(
                baseline[key]["registry"]["typed_schema_status"]
            )
            frozen_global_status[initial_status] -= 1
            frozen_global_status["Complete"] += 1
        derived_status = {
            name: frozen_global_status[name]
            for name in (
                "Complete",
                "NotApplicable",
                "NotImplemented",
                "Partial",
            )
        }
        require(
            derived_status == EXPECTED_STAGED_GLOBAL_STATUS[batch],
            f"{batch} mechanically derived global metrics changed: "
            f"{derived_status}",
            "StartStateMismatch",
        )
        derived_staged_global_status[batch] = derived_status
        cumulative = sorted(cumulative + owned)
        staged_ratchets.append(
            {
                "batch": batch,
                "owned_identity_count": len(owned),
                "cumulative_identity_count": len(cumulative),
                "owned_identities": [key.object() for key in owned],
                "cumulative_identities": [
                    key.object() for key in cumulative
                ],
                "derived_global_schema_status": derived_status,
            }
        )
    derived_residual_partial_keys = sorted(
        key
        for key, row in unrelated_baseline.items()
        if row["registry_projection"].get("typed_schema_status")
        == "Partial"
    )
    require(
        set(derived_residual_partial_keys)
        == EXPECTED_FINAL_RESIDUAL_PARTIAL_KEYS,
        "mechanically derived residual Partial identity set changed",
        "StartStateMismatch",
    )
    batches = [
        {
            "batch": batch,
            "commit_subject": BATCH_SUBJECTS[batch],
            "identity_count": EXPECTED_BATCH_IDENTITY_COUNTS[batch],
            "definition_count": EXPECTED_BATCH_DEFINITION_COUNTS[batch],
            "depends_on": sorted(batch_dependencies[batch]),
            "runtime_implementation_allowed": True,
        }
        for batch in ("B2", "B3", "B4", "B5")
    ] + [
        {
            "batch": "B6",
            "commit_subject": "Close and verify the Codex A1.2 slice",
            "identity_count": 0,
            "definition_count": 0,
            "depends_on": ["B2", "B3", "B4", "B5"],
            "runtime_implementation_allowed": False,
        }
    ]

    fixture_plan_counts = {
        "required_positive_fixture_roles": sum(
            len(
                row["required_positive_fixtures"][
                    "required_fixture_ids"
                ]
            )
            for row in plan_identities
        ),
        "existing_positive_fixture_roles_at_start": sum(
            len(
                row["required_positive_fixtures"][
                    "existing_fixture_ids"
                ]
            )
            for row in plan_identities
        ),
        "planned_positive_fixture_roles_at_start": sum(
            len(
                row["required_positive_fixtures"][
                    "planned_fixture_ids"
                ]
            )
            for row in plan_identities
        ),
        "required_property_present_positive_paths": sum(
            len(
                row["required_positive_fixtures"][
                    "expansion_obligations"
                ]["required_properties_present_schema_paths"]
            )
            for row in plan_identities
        ),
        "optional_property_present_positive_paths": sum(
            len(
                row["required_positive_fixtures"][
                    "expansion_obligations"
                ]["optional_properties_present_schema_paths"]
            )
            for row in plan_identities
        ),
        "optional_property_omitted_positive_paths": sum(
            len(
                row["required_positive_fixtures"][
                    "expansion_obligations"
                ]["optional_properties_omitted_schema_paths"]
            )
            for row in plan_identities
        ),
        "nullable_positive_state_cases": sum(
            len(item["schema_valid_states"])
            for row in plan_identities
            for item in row["required_positive_fixtures"][
                "expansion_obligations"
            ]["nullable_schema_valid_states"]
        ),
        "defaulted_positive_paths": sum(
            len(
                row["required_positive_fixtures"][
                    "expansion_obligations"
                ]["defaulted_schema_paths"]
            )
            for row in plan_identities
        ),
        "integer_boundary_positive_paths": sum(
            len(
                row["required_positive_fixtures"][
                    "expansion_obligations"
                ]["integer_boundary_schema_paths"]
            )
            for row in plan_identities
        ),
        "opaque_json_positive_paths": sum(
            len(
                row["required_positive_fixtures"][
                    "expansion_obligations"
                ][
                    "protocol_defined_opaque_json_schema_paths"
                ]
            )
            for row in plan_identities
        ),
        "collection_empty_nonempty_positive_paths": sum(
            len(
                row["required_positive_fixtures"][
                    "expansion_obligations"
                ]["collection_empty_nonempty_schema_paths"]
            )
            for row in plan_identities
        ),
        "known_open_enum_positive_values": sum(
            len(enum["known_values"])
            for row in plan_identities
            for enum in row["required_positive_fixtures"][
                "expansion_obligations"
            ]["open_string_enum_known_values"]
        ),
        "registered_union_positive_alternatives": sum(
            len(union["alternatives"])
            for row in plan_identities
            for union in row["required_positive_fixtures"][
                "expansion_obligations"
            ]["registered_tagged_union_alternatives"]
        ),
        "required_field_mutation_paths": sum(
            len(
                row["required_negative_fixtures"][
                    "required_field_removal_schema_paths"
                ]
            )
            for row in plan_identities
        ),
        "wrong_type_mutation_paths": sum(
            len(
                row["required_negative_fixtures"][
                    "wrong_type_schema_paths"
                ]
            )
            for row in plan_identities
        ),
        "optional_omission_paths": sum(
            len(
                row["required_negative_fixtures"][
                    "optional_omission_schema_paths"
                ]
            )
            for row in plan_identities
        ),
        "nullable_tri_state_paths": sum(
            len(
                row["required_negative_fixtures"][
                    "nullable_omitted_null_value_schema_paths"
                ]
            )
            for row in plan_identities
        ),
    }

    current_schema_status = dict(
        sorted(
            Counter(
                str(row["current_schema_status"])
                for row in plan_identities
            ).items()
        )
    )
    current_implementation_status = dict(
        sorted(
            Counter(
                str(row["current_implementation_status"])
                for row in plan_identities
            ).items()
        )
    )
    current_progress_stage = live_progress_stage(
        current_schema_status, current_implementation_status
    )
    require(
        current_progress_stage is not None,
        "A1.2 live status counts do not match a complete dependency "
        f"boundary: schema={current_schema_status} "
        f"implementation={current_implementation_status}",
        "ProgressStageMismatch",
    )

    plan = {
        "generated_notice": (
            "Generated by tools/codex/app_server_a1_2.py; do not edit."
        ),
        "format_version": FORMAT_VERSION,
        "codex_version": CODEX_VERSION,
        "authority_note": (
            "Review/batching evidence only. Frozen start facts remain in "
            "a1-2-start-state.json; current progress facts are copied from "
            "the sole canonical production registry. This report is not a "
            "runtime registry or dispatch input."
        ),
        "sources": sources,
        "live_progress_inputs": live_progress_inputs,
        "counts": {
            "identity_denominator": len(keys),
            "client_successful_result_root_obligations_not_identities": (
                len(request_keys)
            ),
            "server_request_response_root_obligations_not_identities": (
                len(server_request_keys)
            ),
            "response_root_obligations_not_identities": (
                len(request_keys) + len(server_request_keys)
            ),
            "total_response_root_obligations_not_identities": (
                len(request_keys) + len(server_request_keys)
            ),
            "taxonomy": taxonomy,
            "classifications": classifications,
            "modules": modules,
            "initial_a1_2_schema_status": dict(
                sorted(
                    Counter(
                        str(
                            baseline[key]["registry"][
                                "typed_schema_status"
                            ]
                        )
                        for key in keys
                    ).items()
                )
            ),
            "initial_global_schema_status": derived_start_global_status,
            "current_progress_stage": current_progress_stage,
            "current_a1_2_schema_status": current_schema_status,
            "current_a1_2_implementation_status": (
                current_implementation_status
            ),
            "derived_final_global_schema_status": (
                derived_staged_global_status["B5"]
            ),
            "result_contract_kinds": result_kinds,
            "batch_identities": identity_batch_counts,
            "batch_definitions": definition_batch_counts,
            "staged_global_schema_status": derived_staged_global_status,
            "identity_directions": identity_direction_counts,
            "fixture_plan": fixture_plan_counts,
            "identity_assignment_mapping_sha256": sha256_json(
                identity_assignment_projection(plan_identities)
            ),
            "identity_start_mapping_sha256": (
                EXPECTED_IDENTITY_START_MAPPING_SHA256
            ),
            "identity_live_progress_mapping_sha256": sha256_json(
                identity_start_projection(plan_identities)
            ),
            "identity_contract_mapping_sha256": sha256_json(
                identity_contract_projection(plan_identities)
            ),
            "identity_fixture_mapping_sha256": sha256_json(
                identity_fixture_projection(plan_identities)
            ),
        },
        "taxonomy_rule": (
            "The 18 client successful-result roots and one server-request "
            "response root are 19 implementation obligations, not additional "
            "ProtocolSurfaceKey identities; the denominator is exactly 45."
        ),
        "unit_result_methods": sorted(EXPECTED_UNIT_METHODS),
        "batches": batches,
        "staged_exact_a1_2_ratchets": staged_ratchets,
        "expected_final_residual_partial_identities": [
            key.object()
            for key in derived_residual_partial_keys
        ],
        "final_exact_a1_2_ratchet": [
            key.object() for key in keys
        ],
        "identities": plan_identities,
    }

    property_rows = [
        row
        for row in path_records
        if row["schema_node_kind"] == "property"
    ]
    path_disposition_counts = dict(
        sorted(
            Counter(
                str(row["disposition"]) for row in path_records
            ).items()
        )
    )
    presence_counts = dict(
        sorted(
            Counter(
                str(row["presence_model"]) for row in property_rows
            ).items()
        )
    )
    sensitivity_counts = dict(
        sorted(
            Counter(
                str(row["sensitivity"]["classification"])
                for row in path_records
            ).items()
        )
    )
    semantic_map_key_counts = dict(
        sorted(
            Counter(
                str(row["semantic_map_key_type"])
                for row in path_records
                if row["semantic_map_key_type"] is not None
            ).items()
        )
    )
    require(
        semantic_map_key_counts == EXPECTED_SEMANTIC_MAP_KEY_TYPES,
        "semantic map-key assignments changed: "
        f"{semantic_map_key_counts}",
        "StrongIdentifierMappingMismatch",
    )
    closure_report = {
        "generated_notice": (
            "Generated by tools/codex/app_server_a1_2.py; do not edit."
        ),
        "format_version": FORMAT_VERSION,
        "codex_version": CODEX_VERSION,
        "authority_note": (
            "Full transitive stable schema-definition/path closure and "
            "reviewed C++ ownership evidence only; not a production registry "
            "or dispatch table."
        ),
        "sources": sources,
        "live_progress_inputs": live_progress_inputs,
        "counts": {
            "seed_definitions": len(seed_associations),
            "seed_role_associations": dict(
                sorted(
                    Counter(
                        association["role"]
                        for values in seed_associations.values()
                        for association in values
                    ).items()
                )
            ),
            "reachable_named_definitions": len(closure),
            "definition_namespaces": namespace_counts,
            "definition_shapes": shape_counts,
            "definition_dispositions": disposition_counts,
            "definition_batches": definition_batch_counts,
            "definition_directions": direction_counts,
            "cross_slice_ownership": ownership_counts,
            "schema_path_dispositions": len(path_records),
            "schema_path_disposition_kinds": (
                path_disposition_counts
            ),
            "schema_path_node_kinds": path_kind_counts,
            "schema_path_directions": path_direction_counts,
            "property_declarations": len(property_rows),
            "property_presence_models": presence_counts,
            "protocol_defined_opaque_json_paths": len(
                AUTHORIZED_OPAQUE_PATHS
            ),
            "sensitivity_classifications": sensitivity_counts,
            "semantic_map_key_types": semantic_map_key_counts,
            "definition_mapping_sha256": sha256_json(
                definition_mapping_projection(definition_records)
            ),
            "schema_path_mapping_sha256": sha256_json(
                schema_path_mapping_projection(path_records)
            ),
        },
        "batch_dependency_graph": {
            batch: sorted(dependencies)
            for batch, dependencies in sorted(
                batch_dependencies.items()
            )
        },
        "seeds": [
            {
                "definition_key": definition.to_json(),
                "associations": sorted(
                    seed_associations[definition],
                    key=lambda row: (
                        str(row["role"]),
                        json.dumps(
                            row["surface_key"], sort_keys=True
                        ),
                    ),
                ),
            }
            for definition in sorted(seed_associations)
        ],
        "definitions": definition_records,
        "schema_paths": path_records,
        "authorized_protocol_defined_opaque_json_paths": [
            {"schema_path": path, "reason": reason}
            for path, reason in sorted(
                AUTHORIZED_OPAQUE_PATHS.items()
            )
        ],
        "semantic_strong_identifiers": [
            {
                "schema_path": path,
                "cpp_type": f"typed::{identifier}",
            }
            for path, identifier in sorted(
                SEMANTIC_IDENTIFIERS.items()
            )
        ],
        "sensitivity_policy": {
            "diagnostic_values_permitted": False,
            "generated_evidence_values_permitted": False,
            "frontend_exposure_from_typing_permitted": False,
            "backend_canonical_state_from_typing_permitted": False,
            "configuration_opaque_values_are_potentially_credential_bearing": (
                True
            ),
        },
    }
    validate_generated_reports(plan, closure_report)
    return plan, closure_report


def _report_key(value: Any) -> tuple[str, str, str, str] | None:
    if not isinstance(value, Mapping):
        return None
    required = {
        "category",
        "domain",
        "discriminator_field",
        "name",
    }
    if not required.issubset(value):
        return None
    return (
        str(value["category"]),
        str(value["domain"]),
        str(value["discriminator_field"]),
        str(value["name"]),
    )


def _has_dependency_cycle(
    graph: Mapping[str, set[str]]
) -> bool:
    return shared.has_dependency_cycle(graph)


def _reported_presence_model(row: Mapping[str, Any]) -> str | None:
    required = row.get("required")
    nullable = row.get("nullable")
    if not isinstance(required, bool) or not isinstance(nullable, bool):
        return None
    if "schema_default" in row and row["schema_default"] is not None:
        return "DefaultedValue"
    if required:
        return "RequiredNullable" if nullable else "RequiredValue"
    return "OptionalNullable" if nullable else "OptionalValue"


def report_diagnostics(
    plan: Mapping[str, Any], closure: Mapping[str, Any]
) -> list[AuditDiagnostic]:
    diagnostics: list[AuditDiagnostic] = []

    def add(code: str, location: str, message: str) -> None:
        diagnostics.append(AuditDiagnostic(code, location, message))

    identities_value = plan.get("identities")
    identities = (
        identities_value if isinstance(identities_value, list) else []
    )
    identity_keys = [
        _report_key(
            row.get("protocol_surface_key")
            if isinstance(row, Mapping)
            else None
        )
        for row in identities
    ]
    valid_keys = [key for key in identity_keys if key is not None]
    duplicates = sorted(
        key
        for key, count in Counter(valid_keys).items()
        if count > 1
    )
    if duplicates:
        add(
            "DuplicateIdentity",
            "$.identities",
            f"duplicate ProtocolSurfaceKey rows: {duplicates}",
        )
    if len(identities) != 45 or len(valid_keys) != 45:
        add(
            "IdentityCountMismatch",
            "$.identities",
            f"expected 45 identity rows, found {len(identities)}",
        )
    ratchet_value = plan.get("final_exact_a1_2_ratchet")
    ratchet = (
        ratchet_value if isinstance(ratchet_value, list) else []
    )
    ratchet_keys = [_report_key(row) for row in ratchet]
    valid_ratchet = [key for key in ratchet_keys if key is not None]
    if len(valid_ratchet) != 45 or len(set(valid_ratchet)) != 45:
        add(
            "IdentityCountMismatch",
            "$.final_exact_a1_2_ratchet",
            "exact A1.2 ratchet is not 45 unique identities",
        )
    missing = sorted(set(valid_ratchet) - set(valid_keys))
    extra = sorted(set(valid_keys) - set(valid_ratchet))
    if missing:
        add(
            "MissingIdentity",
            "$.identities",
            f"missing exact identities: {missing}",
        )
    if extra:
        add(
            "CrossSliceIdentity",
            "$.identities",
            f"unexpected identities: {extra}",
        )

    taxonomy: Counter[str] = Counter()
    classifications: Counter[str] = Counter()
    modules: Counter[str] = Counter()
    statuses: Counter[str] = Counter()
    implementation_statuses: Counter[str] = Counter()
    batches: Counter[str] = Counter()
    directions: Counter[str] = Counter()
    result_kinds: Counter[str] = Counter()
    unit_methods: set[str] = set()
    request_methods: set[str] = set()
    server_request_methods: set[str] = set()
    for index, (row, key_tuple) in enumerate(
        zip(identities, identity_keys)
    ):
        location = f"$.identities[{index}]"
        if not isinstance(row, Mapping):
            add(
                "MissingPlanIdentityField",
                location,
                "identity row is not an object",
            )
            continue
        required_fields = {
            "category",
            "classification",
            "current_fixture_ids",
            "current_implementation_status",
            "current_runtime_disposition",
            "current_runtime_target",
            "current_schema_completeness",
            "current_schema_status",
            "deprecated",
            "discriminator_field",
            "directionality",
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
        missing_fields = sorted(required_fields - set(row))
        if missing_fields:
            add(
                "MissingPlanIdentityField",
                location,
                f"missing fields: {missing_fields}",
            )
        positive = row.get("required_positive_fixtures")
        required_expansion_fields = {
            "collection_empty_nonempty_schema_paths",
            "defaulted_schema_paths",
            "integer_boundary_schema_paths",
            "nullable_schema_valid_states",
            "open_string_enum_known_values",
            "optional_properties_omitted_schema_paths",
            "optional_properties_present_schema_paths",
            "protocol_defined_opaque_json_schema_paths",
            "registered_tagged_union_alternatives",
            "required_properties_present_schema_paths",
        }
        expansion = (
            positive.get("expansion_obligations")
            if isinstance(positive, Mapping)
            else None
        )
        if (
            not isinstance(positive, Mapping)
            or positive.get(
                "base_cases_are_seeds_not_complete_coverage"
            )
            is not True
            or not isinstance(expansion, Mapping)
            or set(expansion) != required_expansion_fields
            or not isinstance(
                positive.get("coverage_completion_rule"), str
            )
        ):
            add(
                "FixturePlanMismatch",
                location + ".required_positive_fixtures",
                "positive fixture plan does not explicitly expand base "
                "seeds across every reviewed coverage class",
            )
        if key_tuple is None:
            continue
        category, domain, field, name = key_tuple
        if (
            row.get("category") != category
            or row.get("domain") != domain
            or row.get("discriminator_field") != field
        ):
            add(
                "PlanIdentityKeyMismatch",
                location,
                "duplicated category/domain/discriminator fields differ "
                "from ProtocolSurfaceKey",
            )
        taxonomy[category] += 1
        classifications[str(row.get("classification"))] += 1
        modules[str(row.get("module"))] += 1
        statuses[str(row.get("current_schema_status"))] += 1
        implementation_statuses[
            str(row.get("current_implementation_status"))
        ] += 1
        batches[str(row.get("owning_implementation_batch"))] += 1
        directions[str(row.get("directionality"))] += 1
        if row.get("stability") != "stable":
            add(
                "UnstableIdentity",
                location + ".stability",
                "A1.2 identity is not stable",
            )
        try:
            expected_batch = identity_batch(
                Key(category, domain, field, name)
            )
        except AuditError:
            expected_batch = ""
        if row.get("owning_implementation_batch") != expected_batch:
            add(
                "BatchAssignmentMismatch",
                location + ".owning_implementation_batch",
                "identity is assigned to the wrong batch",
            )
        expected_direction = _identity_direction(
            Key(category, domain, field, name)
        )
        if row.get("directionality") != expected_direction:
            add(
                "DirectionalityMismatch",
                location + ".directionality",
                "identity direction differs from reviewed direction",
            )
        private = row.get(
            "required_private_codec_or_descriptor"
        )
        if (
            not isinstance(private, Mapping)
            or private.get("descriptor_key")
            != Key(category, domain, field, name).compact()
        ):
            add(
                "DescriptorKeyMismatch",
                location
                + ".required_private_codec_or_descriptor",
                "descriptor key differs from ProtocolSurfaceKey",
            )
        sensitivity = row.get("security_and_sensitivity")
        if (
            not isinstance(sensitivity, Mapping)
            or sensitivity.get("diagnostics_may_contain_values")
            is not False
        ):
            add(
                "SensitivePathPolicyMissing",
                location + ".security_and_sensitivity",
                "identity lacks a no-value diagnostic policy",
            )
        if category == "client_request":
            request_methods.add(name)
            contract = row.get("request_contract")
            if not isinstance(contract, Mapping):
                add(
                    "ContractMismatch",
                    location + ".request_contract",
                    "client request lacks contract evidence",
                )
            else:
                kind = str(contract.get("result_contract_kind"))
                result_kinds[kind] += 1
                if kind == "Unit":
                    unit_methods.add(name)
                expected_api = OPERATION_API_NAMES.get(name)
                if expected_api is None or contract.get(
                    "proposed_public_method_name"
                ) != f"{expected_api[0]}::{expected_api[1]}":
                    add(
                        "ContractMismatch",
                        location + ".request_contract",
                        "grouped facade mapping changed",
                    )
        elif category == "server_request":
            server_request_methods.add(name)
            contract = row.get("server_request_contract")
            if not isinstance(contract, Mapping):
                add(
                    "ContractMismatch",
                    location + ".server_request_contract",
                    "server request lacks response contract",
                )

    normalized_taxonomy = {
        name: taxonomy[name] for name in EXPECTED_TAXONOMY
    }
    if normalized_taxonomy != EXPECTED_TAXONOMY or any(
        taxonomy[name]
        for name in (
            "client_notification",
            "item_discriminator",
            "response_item",
        )
    ):
        add(
            "TaxonomyMismatch",
            "$.identities",
            f"A1.2 taxonomy changed: {normalized_taxonomy}",
        )
    if dict(sorted(classifications.items())) != EXPECTED_CLASSIFICATIONS:
        add(
            "AssignmentMismatch",
            "$.identities[*].classification",
            "classification counts changed",
        )
    if dict(sorted(modules.items())) != EXPECTED_MODULES:
        add(
            "AssignmentMismatch",
            "$.identities[*].module",
            "module counts changed",
        )
    observed_schema_status = dict(sorted(statuses.items()))
    observed_implementation_status = dict(
        sorted(implementation_statuses.items())
    )
    observed_progress_stage = live_progress_stage(
        observed_schema_status, observed_implementation_status
    )
    if observed_progress_stage is None:
        add(
            "ProgressStageMismatch",
            "$.identities[*].current_schema_status",
            "live schema/implementation split does not match an exact "
            "dependency-closed A1.2 boundary",
        )
    else:
        identity_progress_mismatch = False
        for row in identities:
            if not isinstance(row, Mapping):
                continue
            key_row = row.get("protocol_surface_key")
            if not isinstance(key_row, Mapping):
                continue
            try:
                key = Key.from_row(key_row)
                expected_schema, expected_implementation = (
                    expected_identity_progress(
                        observed_progress_stage, key
                    )
                )
            except (AuditError, KeyError, TypeError, ValueError):
                continue
            if (
                row.get("current_schema_status") != expected_schema
                or row.get("current_implementation_status")
                != expected_implementation
            ):
                identity_progress_mismatch = True
                break
        if identity_progress_mismatch:
            add(
                "ProgressIdentityMismatch",
                "$.identities",
                "live status counts match a dependency boundary but "
                "the completed/partial identity mapping does not",
            )
    if dict(sorted(batches.items())) != EXPECTED_BATCH_IDENTITY_COUNTS:
        add(
            "BatchAssignmentMismatch",
            "$.identities[*].owning_implementation_batch",
            "identity batch counts changed",
        )
    if dict(sorted(directions.items())) != EXPECTED_IDENTITY_DIRECTIONS:
        add(
            "DirectionalityMismatch",
            "$.identities[*].directionality",
            "identity direction counts changed",
        )
    if request_methods != CLIENT_REQUESTS:
        add(
            "ContractMismatch",
            "$.identities[*].request_contract",
            "exact client request set changed",
        )
    if not {
        "experimentalFeature/enablement/set",
        "experimentalFeature/list",
    }.issubset(request_methods):
        add(
            "MissingStableFeatureOperation",
            "$.identities",
            "stable experimentalFeature methods are missing",
        )
    if server_request_methods != {AUTH_REFRESH_METHOD}:
        add(
            "ContractMismatch",
            "$.identities[*].server_request_contract",
            "auth-refresh server request is missing",
        )
    if dict(sorted(result_kinds.items())) != EXPECTED_RESULT_KINDS:
        add(
            "ResultKindMismatch",
            "$.identities[*].request_contract",
            "client result-kind counts changed",
        )
    if unit_methods != EXPECTED_UNIT_METHODS:
        add(
            "ResultKindMismatch",
            "$.unit_result_methods",
            "Unit operation set changed",
        )

    counts = plan.get("counts")
    counts = counts if isinstance(counts, Mapping) else {}
    try:
        assignment_mapping_sha256 = sha256_json(
            identity_assignment_projection(
                [
                    row
                    for row in identities
                    if isinstance(row, Mapping)
                ]
            )
        )
    except (KeyError, TypeError):
        assignment_mapping_sha256 = ""
    if (
        assignment_mapping_sha256
        != EXPECTED_IDENTITY_ASSIGNMENT_MAPPING_SHA256
        or counts.get("identity_assignment_mapping_sha256")
        != EXPECTED_IDENTITY_ASSIGNMENT_MAPPING_SHA256
    ):
        add(
            "PlanAssignmentMappingMismatch",
            "$.identities",
            "per-identity assignment/module/batch/direction mapping "
            "differs from the exact reviewed plan",
        )
    try:
        start_mapping_sha256 = sha256_json(
            identity_start_projection(
                [
                    row
                    for row in identities
                    if isinstance(row, Mapping)
                ]
            )
        )
    except (KeyError, TypeError):
        start_mapping_sha256 = ""
    if counts.get(
        "identity_start_mapping_sha256"
    ) != EXPECTED_IDENTITY_START_MAPPING_SHA256:
        add(
            "PlanStartStateMappingMismatch",
            "$.counts.identity_start_mapping_sha256",
            "frozen start-state projection hash changed",
        )
    if counts.get(
        "identity_live_progress_mapping_sha256"
    ) != start_mapping_sha256:
        add(
            "PlanProgressMappingMismatch",
            "$.identities",
            "live per-identity runtime/status/completeness projection "
            "does not match its generated hash",
        )
    try:
        contract_mapping_sha256 = sha256_json(
            identity_contract_projection(
                [
                    row
                    for row in identities
                    if isinstance(row, Mapping)
                ]
            )
        )
    except (KeyError, TypeError):
        contract_mapping_sha256 = ""
    if (
        contract_mapping_sha256
        != EXPECTED_IDENTITY_CONTRACT_MAPPING_SHA256
        or counts.get("identity_contract_mapping_sha256")
        != EXPECTED_IDENTITY_CONTRACT_MAPPING_SHA256
    ):
        add(
            "PlanContractMappingMismatch",
            "$.identities",
            "per-operation params/result/server-response contract "
            "mapping differs from the exact reviewed plan",
        )
    try:
        fixture_mapping_sha256 = sha256_json(
            identity_fixture_projection(
                [
                    row
                    for row in identities
                    if isinstance(row, Mapping)
                ]
            )
        )
    except (KeyError, TypeError):
        fixture_mapping_sha256 = ""
    if (
        fixture_mapping_sha256
        != EXPECTED_IDENTITY_FIXTURE_MAPPING_SHA256
        or counts.get("identity_fixture_mapping_sha256")
        != EXPECTED_IDENTITY_FIXTURE_MAPPING_SHA256
    ):
        add(
            "FixturePlanMappingMismatch",
            "$.identities",
            "per-identity positive/negative fixture obligations differ "
            "from the exact reviewed plan",
        )
    if counts.get("identity_denominator") != 45:
        add(
            "IdentityCountMismatch",
            "$.counts.identity_denominator",
            "identity denominator is not 45",
        )
    if (
        counts.get(
            "client_successful_result_root_obligations_not_identities"
        )
        != 18
        or counts.get(
            "server_request_response_root_obligations_not_identities"
        )
        != 1
        or counts.get(
            "response_root_obligations_not_identities"
        )
        != 19
    ):
        add(
            "ResponseRootArithmeticMismatch",
            "$.counts",
            "19 response roots were counted incorrectly",
        )
    if counts.get("taxonomy") != EXPECTED_TAXONOMY:
        add(
            "TaxonomyMismatch",
            "$.counts.taxonomy",
            "taxonomy count summary changed",
        )
    if counts.get("classifications") != EXPECTED_CLASSIFICATIONS:
        add(
            "AssignmentMismatch",
            "$.counts.classifications",
            "classification summary changed",
        )
    if counts.get("modules") != EXPECTED_MODULES:
        add(
            "AssignmentMismatch",
            "$.counts.modules",
            "module summary changed",
        )
    if (
        counts.get("initial_a1_2_schema_status")
        != EXPECTED_START_SCHEMA_STATUS
    ):
        add(
            "StartStateMismatch",
            "$.counts.initial_a1_2_schema_status",
            "initial A1.2 status summary changed",
        )
    if (
        counts.get("current_progress_stage")
        != observed_progress_stage
        or counts.get("current_a1_2_schema_status")
        != observed_schema_status
        or counts.get("current_a1_2_implementation_status")
        != observed_implementation_status
    ):
        add(
            "ProgressStageMismatch",
            "$.counts",
            "current progress summaries differ from identity rows",
        )
    if (
        counts.get("initial_global_schema_status")
        != EXPECTED_START_GLOBAL_STATUS
        or counts.get("derived_final_global_schema_status")
        != EXPECTED_FINAL_GLOBAL_STATUS
    ):
        add(
            "StartStateMismatch",
            "$.counts",
            "global start/final metrics changed",
        )
    if (
        counts.get("result_contract_kinds")
        != EXPECTED_RESULT_KINDS
    ):
        add(
            "ResultKindMismatch",
            "$.counts.result_contract_kinds",
            "result-kind summary changed",
        )
    if (
        counts.get("batch_identities")
        != EXPECTED_BATCH_IDENTITY_COUNTS
        or counts.get("batch_definitions")
        != EXPECTED_BATCH_DEFINITION_COUNTS
    ):
        add(
            "BatchAssignmentMismatch",
            "$.counts",
            "batch count summary changed",
        )

    batch_records = plan.get("batches")
    batch_records = (
        batch_records if isinstance(batch_records, list) else []
    )
    graph: dict[str, set[str]] = {}
    names: list[str] = []
    for record in batch_records:
        if not isinstance(record, Mapping):
            continue
        batch = record.get("batch")
        if not isinstance(batch, str):
            continue
        names.append(batch)
        dependencies = record.get("depends_on")
        graph[batch] = (
            {
                str(value)
                for value in dependencies
                if isinstance(value, str)
            }
            if isinstance(dependencies, list)
            else set()
        )
        if batch == "B6":
            if (
                record.get("identity_count") != 0
                or record.get("definition_count") != 0
                or record.get("runtime_implementation_allowed")
                is not False
            ):
                add(
                    "BatchAssignmentMismatch",
                    "$.batches[B6]",
                    "closure-only batch gained runtime work",
                )
        elif (
            record.get("identity_count")
            != EXPECTED_BATCH_IDENTITY_COUNTS.get(batch)
            or record.get("definition_count")
            != EXPECTED_BATCH_DEFINITION_COUNTS.get(batch)
        ):
            add(
                "BatchAssignmentMismatch",
                f"$.batches[{batch}]",
                "batch counts changed",
            )
    if (
        set(names) != {"B2", "B3", "B4", "B5", "B6"}
        or len(names) != len(set(names))
        or _has_dependency_cycle(graph)
    ):
        add(
            "BatchDependencyCycle",
            "$.batches[*].depends_on",
            "batch graph is incomplete, duplicated, or cyclic",
        )

    definitions_value = closure.get("definitions")
    definitions = (
        definitions_value
        if isinstance(definitions_value, list)
        else []
    )
    definition_keys: list[tuple[str, str] | None] = []
    for index, row in enumerate(definitions):
        key = row.get("definition_key") if isinstance(row, Mapping) else None
        if isinstance(key, Mapping) and {
            "namespace",
            "name",
        }.issubset(key):
            definition_keys.append(
                (str(key["namespace"]), str(key["name"]))
            )
        else:
            definition_keys.append(None)
        if not isinstance(row, Mapping) or row.get(
            "disposition"
        ) not in VALID_TYPE_DISPOSITIONS:
            add(
                "MissingClosureDisposition",
                f"$.definitions[{index}]",
                "definition lacks an exact disposition",
            )
        elif row.get("schema_shape") == "open_string_enum":
            known_values = row.get("known_values")
            if (
                not isinstance(known_values, list)
                or not known_values
                or not all(
                    isinstance(value, str) and value
                    for value in known_values
                )
                or row.get("future_value_behavior")
                != {
                    "representation": "open-string-backed",
                    "diagnostic_kind": "UnknownEnumValue",
                    "diagnostic_severity": "ForwardCompatibility",
                    "retains_outer_known_branch": True,
                }
            ):
                add(
                    "OpenStringEnumPolicyMismatch",
                    f"$.definitions[{index}]",
                    "open string enum lacks exact known/future-value "
                    "behavior",
                )
        elif row.get("disposition") == "OpenStringEnum":
            add(
                "OpenStringEnumPolicyMismatch",
                f"$.definitions[{index}]",
                "OpenStringEnum disposition is not backed by a reviewed "
                "string-enum schema",
            )
        if isinstance(row, Mapping):
            definition_key = row.get("definition_key")
            definition_name = (
                str(definition_key.get("name"))
                if isinstance(definition_key, Mapping)
                else ""
            )
            expected_raw_retention = (
                row.get("directionality") == "DecodeOnly"
                and row.get("disposition")
                in {"PublicHandwrittenType", "ReusedExistingType"}
                and definition_name
                not in {
                    definition.name
                    for definition in UNIT_RESULT_DEFINITIONS
                }
            )
            raw_retention = row.get("raw_retention")
            if (
                not isinstance(raw_retention, Mapping)
                or raw_retention.get("permitted")
                is not expected_raw_retention
            ):
                add(
                    "RawRetentionPolicyMismatch",
                    f"$.definitions[{index}].raw_retention",
                    "definition raw retention differs from the reviewed "
                    "directional policy",
                )
    valid_definition_keys = [
        key for key in definition_keys if key is not None
    ]
    if (
        len(definitions) != 104
        or len(valid_definition_keys) != 104
        or len(set(valid_definition_keys)) != 104
    ):
        add(
            "ClosureCountMismatch",
            "$.definitions",
            "definition closure is not 104 unique records",
        )

    paths_value = closure.get("schema_paths")
    paths = paths_value if isinstance(paths_value, list) else []
    path_keys: list[tuple[str, str]] = []
    path_dispositions: Counter[str] = Counter()
    path_node_kinds: Counter[str] = Counter()
    path_directions: Counter[str] = Counter()
    property_presence_models: Counter[str] = Counter()
    cpp_ownership_keys: list[tuple[str, str]] = []
    path_rows_by_surface_key: dict[
        tuple[str, str, str, str], list[Mapping[str, Any]]
    ] = defaultdict(list)
    for index, row in enumerate(paths):
        if not isinstance(row, Mapping):
            add(
                "MissingClosureDisposition",
                f"$.schema_paths[{index}]",
                "schema path row is not an object",
            )
            continue
        path = str(row.get("schema_path"))
        kind = str(row.get("schema_node_kind"))
        path_keys.append((path, kind))
        path_dispositions[str(row.get("disposition"))] += 1
        path_node_kinds[kind] += 1
        path_directions[str(row.get("directionality"))] += 1
        cpp_owner = row.get("cpp_owner")
        cpp_member = row.get("cpp_member")
        cpp_type = row.get("cpp_type")
        if not all(
            isinstance(value, str) and value
            for value in (cpp_owner, cpp_member, cpp_type)
        ):
            add(
                "CppOwnershipMismatch",
                f"$.schema_paths[{index}]",
                "schema path lacks an exact C++ owner/member/type",
            )
        else:
            cpp_ownership_keys.append((cpp_owner, cpp_member))
            if row.get("cpp_mapping") != (
                f"{cpp_owner}::{cpp_member} -> {cpp_type}"
            ):
                add(
                    "CppOwnershipMismatch",
                    f"$.schema_paths[{index}].cpp_mapping",
                    "C++ mapping disagrees with owner/member/type",
                )
        expected_map_key_type = semantic_map_key_type_for_path(path)
        if row.get("semantic_map_key_type") != expected_map_key_type:
            add(
                "StrongIdentifierMappingMismatch",
                f"$.schema_paths[{index}].semantic_map_key_type",
                "semantic JSON-object key type differs from the exact "
                "reviewed map-key association",
            )
        branch_match = re.match(
            r"^#/definitions/v2/"
            r"(Account|ConfigLayerSource|LoginAccountParams|"
            r"LoginAccountResponse)/oneOf/(\d+)(?:/|$)",
            path,
        )
        if branch_match is not None:
            union_name = branch_match.group(1)
            branch_index = int(branch_match.group(2))
            order = UNION_BRANCH_ORDER[union_name]
            valid_branch_index = branch_index < len(order)
            expected_owner = (
                f"typed::{UNION_ALTERNATIVES[(union_name, order[branch_index])]}"
                if valid_branch_index
                else ""
            )
            if cpp_owner != expected_owner:
                add(
                    "CppOwnershipMismatch",
                    f"$.schema_paths[{index}].cpp_owner",
                    "registered tagged-union field is not owned by its "
                    "distinct reviewed alternative",
                )
            tagged_roots = [
                root
                for root in row.get("reaching_roots", [])
                if isinstance(root, Mapping)
                and root.get("role")
                == "registered_tagged_union_family"
            ]
            if (
                not valid_branch_index
                or
                len(tagged_roots) != 1
                or not isinstance(
                    tagged_roots[0].get("surface_key"), Mapping
                )
                or tagged_roots[0]["surface_key"].get("domain")
                != union_name
                or tagged_roots[0]["surface_key"].get("name")
                != (
                    order[branch_index]
                    if valid_branch_index
                    else None
                )
            ):
                add(
                    "ReachabilityRootMismatch",
                    f"$.schema_paths[{index}].reaching_roots",
                    "registered tagged-union branch path does not reach "
                    "exactly its own alternative identity",
                )
        reaching_roots = row.get("reaching_roots")
        if isinstance(reaching_roots, list):
            for root in reaching_roots:
                root_key = _report_key(
                    root.get("surface_key")
                    if isinstance(root, Mapping)
                    else None
                )
                if root_key is not None:
                    path_rows_by_surface_key[root_key].append(row)
        if (
            "required" not in row
            or not isinstance(row.get("nullable"), bool)
            or not isinstance(row.get("defaulted"), bool)
            or not isinstance(row.get("presence_model"), str)
        ):
            add(
                "OptionalNullableMappingMismatch",
                f"$.schema_paths[{index}]",
                "schema path lacks explicit required/default/nullable "
                "evidence",
            )
        if row.get("disposition") not in VALID_TYPE_DISPOSITIONS:
            add(
                "MissingClosureDisposition",
                f"$.schema_paths[{index}].disposition",
                "schema path lacks exact disposition",
            )
        if row.get("raw_retention_permitted") is not (
            row.get("disposition") == "ProtocolDefinedOpaqueJson"
        ):
            add(
                "RawRetentionPolicyMismatch",
                f"$.schema_paths[{index}].raw_retention_permitted",
                "schema path authorizes raw retention outside the exact "
                "protocol-defined opaque JSON set",
            )
        sensitivity = row.get("sensitivity")
        sensitivity_class = (
            sensitivity.get("classification")
            if isinstance(sensitivity, Mapping)
            else None
        )
        direct_sensitivity = sensitivity_classification(path)
        expected_reported_policy = (
            sensitivity_policy(path, str(sensitivity_class))
            if sensitivity_class in SENSITIVITY_PRIORITY
            else None
        )
        if (
            sensitivity != expected_reported_policy
            or sensitivity_class not in SENSITIVITY_PRIORITY
            or SENSITIVITY_PRIORITY[str(sensitivity_class)]
            < SENSITIVITY_PRIORITY[direct_sensitivity]
        ):
            add(
                "SensitivePathPolicyMissing",
                f"$.schema_paths[{index}].sensitivity",
                "schema path direct/transitive sensitivity or "
                "log/backend/frontend policy differs from the reviewed "
                "policy",
            )
        map_container = row.get("map_container")
        if kind == "map_value":
            expected_key_type = expected_map_key_type or "std::string"
            expected_container_type = (
                f"std::map<{expected_key_type}, "
                f"{row.get('cpp_base_type')}>"
            )
            if (
                not isinstance(map_container, Mapping)
                or map_container.get("cpp_owner") != cpp_owner
                or map_container.get("cpp_type")
                != expected_container_type
                or map_container.get("cpp_mapping")
                != (
                    f"{cpp_owner}::{map_container.get('cpp_member')} -> "
                    f"{expected_container_type}"
                )
                or not isinstance(
                    map_container.get("cpp_member"), str
                )
            ):
                add(
                    "AdditionalPropertiesRetentionMismatch",
                    f"$.schema_paths[{index}].map_container",
                    "additionalProperties value lacks its exact typed "
                    "container ownership",
                )
        elif map_container is not None:
            add(
                "AdditionalPropertiesRetentionMismatch",
                f"$.schema_paths[{index}].map_container",
                "non-map schema path gained a map container",
            )
        if (
            row.get("disposition")
            == "ProtocolDefinedOpaqueJson"
            and path not in AUTHORIZED_OPAQUE_PATHS
        ):
            add(
                "OpaqueJsonMisuse",
                f"$.schema_paths[{index}]",
                "unauthorized opaque JSON path",
            )
        if kind == "property":
            actual_presence = row.get("presence_model")
            expected_presence = _reported_presence_model(row)
            property_presence_models[str(actual_presence)] += 1
            expected_default_behavior = (
                "PreserveOmittedNullValue"
                if row.get("schema_default") is None
                else "CanonicalValueOnOmission"
            )
            default_behavior_mismatch = (
                "schema_default" in row
                and row.get("default_behavior")
                != expected_default_behavior
            )
            if (
                expected_presence is None
                or actual_presence != expected_presence
                or default_behavior_mismatch
            ):
                add(
                    "OptionalNullableMappingMismatch",
                    f"$.schema_paths[{index}].presence_model",
                    "property omitted/null/value mapping differs from "
                    "required, nullable, and default evidence",
                )
    duplicate_cpp_owners = sorted(
        key
        for key, count in Counter(cpp_ownership_keys).items()
        if count > 1
    )
    if (
        len(cpp_ownership_keys) != len(paths)
        or duplicate_cpp_owners
    ):
        add(
            "CppOwnershipConflict",
            "$.schema_paths[*]",
            "schema paths have missing or duplicate C++ owner/member "
            f"assignments: {duplicate_cpp_owners}",
        )
    for index, (row, key_tuple) in enumerate(
        zip(identities, identity_keys)
    ):
        if not isinstance(row, Mapping) or key_tuple is None:
            continue
        expected_security = _identity_security(
            path_rows_by_surface_key.get(key_tuple, [])
        )
        if row.get("security_and_sensitivity") != expected_security:
            add(
                "SensitiveIdentityPolicyMismatch",
                f"$.identities[{index}].security_and_sensitivity",
                "identity sensitivity policy differs from its exact "
                "transitive schema-path policy",
            )
    if (
        len(paths) != EXPECTED_SCHEMA_PATH_COUNT
        or len(path_keys) != EXPECTED_SCHEMA_PATH_COUNT
        or len(set(path_keys)) != EXPECTED_SCHEMA_PATH_COUNT
    ):
        add(
            "ClosureCountMismatch",
            "$.schema_paths",
            "schema-path closure is not 302 unique records",
        )
    actual_opaque = {
        str(row.get("schema_path"))
        for row in paths
        if isinstance(row, Mapping)
        and row.get("disposition")
        == "ProtocolDefinedOpaqueJson"
    }
    if actual_opaque != set(AUTHORIZED_OPAQUE_PATHS):
        add(
            "OpaqueJsonMisuse",
            "$.schema_paths[*].disposition",
            "opaque path set differs from exact reviewed set",
        )
    actual_mixed_unknown_containers = {
        str(row.get("schema_path")): str(
            row["map_container"].get("cpp_mapping")
        )
        for row in paths
        if isinstance(row, Mapping)
        and isinstance(row.get("map_container"), Mapping)
        and row["map_container"].get(
            "separates_unknown_fields_from_known_fields"
        )
        is True
    }
    if (
        actual_mixed_unknown_containers
        != EXPECTED_MIXED_OBJECT_UNKNOWN_PROPERTY_CONTAINERS
    ):
        add(
            "AdditionalPropertiesRetentionMismatch",
            "$.schema_paths[*].map_container",
            "mixed known objects do not retain unknown properties in "
            "their exact separate containers",
        )
    closure_counts = closure.get("counts")
    closure_counts = (
        closure_counts
        if isinstance(closure_counts, Mapping)
        else {}
    )
    try:
        definition_mapping_sha256 = sha256_json(
            definition_mapping_projection(
                [
                    row
                    for row in definitions
                    if isinstance(row, Mapping)
                ]
            )
        )
    except (KeyError, TypeError):
        definition_mapping_sha256 = ""
    if (
        definition_mapping_sha256
        != EXPECTED_DEFINITION_MAPPING_SHA256
        or closure_counts.get("definition_mapping_sha256")
        != EXPECTED_DEFINITION_MAPPING_SHA256
    ):
        add(
            "DefinitionMappingMismatch",
            "$.definitions",
            "definition C++ ownership/disposition mapping differs from "
            "the exact reviewed closure",
        )
    try:
        schema_path_mapping_sha256 = sha256_json(
            schema_path_mapping_projection(
                [
                    row
                    for row in paths
                    if isinstance(row, Mapping)
                ]
            )
        )
    except (KeyError, TypeError):
        schema_path_mapping_sha256 = ""
    if (
        schema_path_mapping_sha256
        != EXPECTED_SCHEMA_PATH_MAPPING_SHA256
        or closure_counts.get("schema_path_mapping_sha256")
        != EXPECTED_SCHEMA_PATH_MAPPING_SHA256
    ):
        add(
            "SchemaPathMappingMismatch",
            "$.schema_paths",
            "schema-path primitive/container/wrapper/owner/disposition "
            "mapping differs from the exact reviewed closure",
        )
    normalized_path_dispositions = dict(
        sorted(path_dispositions.items())
    )
    normalized_path_node_kinds = dict(sorted(path_node_kinds.items()))
    normalized_path_directions = dict(sorted(path_directions.items()))
    normalized_presence_models = dict(
        sorted(property_presence_models.items())
    )
    normalized_semantic_map_key_types = dict(
        sorted(
            Counter(
                str(row.get("semantic_map_key_type"))
                for row in paths
                if isinstance(row, Mapping)
                and row.get("semantic_map_key_type") is not None
            ).items()
        )
    )
    if (
        normalized_path_dispositions
        != EXPECTED_SCHEMA_PATH_DISPOSITIONS
        or closure_counts.get("schema_path_disposition_kinds")
        != normalized_path_dispositions
    ):
        add(
            "MissingClosureDisposition",
            "$.counts.schema_path_disposition_kinds",
            "schema-path disposition rows or summary changed",
        )
    if (
        normalized_path_node_kinds != EXPECTED_SCHEMA_PATH_KINDS
        or closure_counts.get("schema_path_node_kinds")
        != normalized_path_node_kinds
    ):
        add(
            "ClosureCountMismatch",
            "$.counts.schema_path_node_kinds",
            "schema-path node-kind rows or summary changed",
        )
    if (
        normalized_path_directions != EXPECTED_SCHEMA_PATH_DIRECTIONS
        or closure_counts.get("schema_path_directions")
        != normalized_path_directions
    ):
        add(
            "DirectionalityMismatch",
            "$.counts.schema_path_directions",
            "schema-path direction rows or summary changed",
        )
    if (
        normalized_presence_models
        != EXPECTED_PROPERTY_PRESENCE_MODELS
        or closure_counts.get("property_presence_models")
        != normalized_presence_models
    ):
        add(
            "OptionalNullableMappingMismatch",
            "$.counts.property_presence_models",
            "property presence-model rows or summary changed",
        )
    if (
        normalized_semantic_map_key_types
        != EXPECTED_SEMANTIC_MAP_KEY_TYPES
        or closure_counts.get("semantic_map_key_types")
        != normalized_semantic_map_key_types
    ):
        add(
            "StrongIdentifierMappingMismatch",
            "$.counts.semantic_map_key_types",
            "semantic map-key rows or summary changed",
        )
    if (
        closure_counts.get("seed_definitions") != 40
        or closure_counts.get("reachable_named_definitions") != 104
        or closure_counts.get("schema_path_dispositions") != 302
    ):
        add(
            "ClosureCountMismatch",
            "$.counts",
            "closure count summary changed",
        )
    if (
        closure_counts.get("definition_dispositions")
        != EXPECTED_DEFINITION_DISPOSITIONS
    ):
        add(
            "MissingClosureDisposition",
            "$.counts.definition_dispositions",
            "definition disposition summary changed",
        )
    if (
        closure_counts.get("protocol_defined_opaque_json_paths")
        != len(AUTHORIZED_OPAQUE_PATHS)
    ):
        add(
            "OpaqueJsonMisuse",
            "$.counts.protocol_defined_opaque_json_paths",
            "opaque-path count changed",
        )
    return sorted(set(diagnostics))


def validate_generated_reports(
    plan: Mapping[str, Any], closure: Mapping[str, Any]
) -> None:
    shared.validate_diagnostics(report_diagnostics(plan, closure))


def write_or_check(
    path: Path, document: Mapping[str, Any], check: bool
) -> None:
    shared.write_or_check(
        path,
        document,
        check,
        artifact_label="generated A1.2 audit",
    )


def parser() -> argparse.ArgumentParser:
    repo_root = Path(__file__).resolve().parents[2]
    evidence = (
        repo_root / "tools/codex/app-server-evidence/0.144.6"
    )
    result = argparse.ArgumentParser(description=__doc__)
    result.add_argument(
        "mode", choices=("freeze-start-state", "generate", "check")
    )
    result.add_argument("--repo-root", type=Path, default=repo_root)
    result.add_argument(
        "--manifest",
        type=Path,
        default=(
            repo_root / "tools/codex/app-server-surface/0.144.6.json"
        ),
    )
    result.add_argument(
        "--registry",
        type=Path,
        default=(
            repo_root
            / "src/ai/openai/codex/detail/"
            "ProtocolSurfaceRegistryData.inc"
        ),
    )
    result.add_argument(
        "--schema-root",
        type=Path,
        default=(
            repo_root / "tools/codex/app-server-schema/0.144.6"
        ),
    )
    result.add_argument(
        "--assignments",
        type=Path,
        default=evidence / "module-slice-assignment.json",
    )
    result.add_argument(
        "--reachability",
        type=Path,
        default=evidence / "nested-reachability.json",
    )
    result.add_argument(
        "--contracts",
        type=Path,
        default=evidence / "operation-contracts.json",
    )
    result.add_argument(
        "--schema-completeness",
        type=Path,
        default=evidence / "schema-completeness-evidence.json",
    )
    result.add_argument(
        "--fixture-coverage",
        type=Path,
        default=evidence / "fixture-coverage.json",
    )
    result.add_argument(
        "--fixture-index",
        type=Path,
        default=(
            repo_root
            / "tools/codex/app-server-fixtures/0.144.6/index.json"
        ),
    )
    result.add_argument(
        "--draft07-validator",
        type=Path,
        default=repo_root / "tools/codex/draft07.py",
    )
    result.add_argument(
        "--start-state",
        type=Path,
        default=evidence / "a1-2-start-state.json",
    )
    result.add_argument("--base-sha")
    result.add_argument("--force-start-state", action="store_true")
    result.add_argument(
        "--plan-output",
        type=Path,
        default=evidence / "a1-2-implementation-plan.json",
    )
    result.add_argument(
        "--closure-output",
        type=Path,
        default=evidence / "a1-2-type-closure.json",
    )
    return result


def main(argv: Sequence[str] | None = None) -> int:
    arguments = parser().parse_args(argv)
    for name, value in vars(arguments).items():
        if isinstance(value, Path):
            setattr(arguments, name, value.resolve())
    if arguments.mode == "freeze-start-state":
        freeze_start_state(arguments)
        return 0
    plan, closure = build_reports(arguments)
    check = arguments.mode == "check"
    write_or_check(arguments.plan_output, plan, check)
    write_or_check(arguments.closure_output, closure, check)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AuditError as error:
        print(
            f"app-server-a1-2: error: {error.code}: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
