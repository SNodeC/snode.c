#!/usr/bin/env python3
"""Codex App Server schema provenance and protocol-surface tooling.

The generated JSON Schema artifacts are authoritative.  TypeScript artifacts
are accepted only as an independent census cross-check and are never merged
into the schema-derived surface.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import importlib.util
import json
import re
import sys
from pathlib import Path
from typing import Any, Iterator, Sequence


FORMAT_VERSION = 1
PROVENANCE_FORMAT_VERSION = 2
CODEX_VERSION = "codex-cli 0.144.6"
VENDORED_RUST_COMMON_PATH = (
    "codex-rs/app-server-protocol/src/protocol/common.rs"
)
STARTING_SNODEC_SHA = "138f5022c19b24847bee42a21242aaaf7dde5a04"
UPSTREAM_PROVENANCE = {
    "project": "OpenAI Codex",
    "repository": "https://github.com/openai/codex",
    "release": {
        "tag": "rust-v0.144.6",
        "url": "https://github.com/openai/codex/releases/tag/rust-v0.144.6",
        "published_at": "2026-07-18T13:51:52Z",
        "annotated_tag_object_sha": "1e66aaa95b5ab39d3ef3057cd50bdecd576a8356",
        "source_commit_sha": "5d1fbf26c43abc65a203928b2e31561cb039e06d",
        "tag_signature_status": "unsigned",
        "commit_signature_status": "unsigned",
    },
    "authority_binary": {
        "version_output": CODEX_VERSION,
        "target": "x86_64-unknown-linux-musl",
        "executable_bytes": 298516528,
        "executable_sha256": "a31ae9450a26216eb1e7c53102fd42123dd675974310b0e2ca3aa4cb622a2c15",
        "release_asset": "codex-x86_64-unknown-linux-musl.tar.gz",
        "release_asset_url": (
            "https://github.com/openai/codex/releases/download/rust-v0.144.6/"
            "codex-x86_64-unknown-linux-musl.tar.gz"
        ),
        "release_asset_bytes": 109369631,
        "release_asset_sha256": "6a9def51a0ad8cea6684d8eb3bf033c89f33e3bc5cfe492f1a1e0a718451a1c6",
        "source_revision_verification": (
            "The installed executable was byte-identical to the executable extracted from the official "
            "rust-v0.144.6 GitHub release asset; the annotated release tag resolves to source commit "
            "5d1fbf26c43abc65a203928b2e31561cb039e06d. The Git tag and commit are unsigned, "
            "so this record does not claim Git signature verification."
        ),
    },
    "license": {
        "spdx_identifier": "Apache-2.0",
        "copyright": "Copyright 2025 OpenAI",
        "license_file": {
            "path": "LICENSE.openai-codex",
            "source_url": "https://github.com/openai/codex/blob/rust-v0.144.6/LICENSE",
            "git_blob_sha": "4606e72e042564097e8780d66c1d4dcb611869bd",
            "bytes": 10926,
            "sha256": "d17f227e4df5da1600391338865ce0f3055211760a36688f816941d58232d8dc",
        },
        "notice_file": {
            "path": "NOTICE.openai-codex",
            "source_url": "https://github.com/openai/codex/blob/rust-v0.144.6/NOTICE",
            "git_blob_sha": "2805899d56d0332d175cfc613c67d45d6f006db7",
            "bytes": 242,
            "sha256": "9d71575ecfd9a843fc1677b0efb08053c6ba9fd686a0de1a6f5382fd3c220915",
        },
        "notice_attributions": [
            "OpenAI Codex; Copyright 2025 OpenAI",
            "Ratatui-derived code under the MIT license; Copyright (c) 2016-2022 Florian Dehau",
            "Ratatui-derived code under the MIT license; Copyright (c) 2023-2025 The Ratatui Developers",
        ],
        "redistribution": (
            "The exact upstream Apache-2.0 license and NOTICE accompany the generated schemas in source "
            "distributions. The vendored generated schema files themselves remain unmodified."
        ),
    },
}
REQUIRED_SCHEMA_FILES = (
    "ClientNotification.json",
    "ClientRequest.json",
    "ServerNotification.json",
    "ServerRequest.json",
    "codex_app_server_protocol.schemas.json",
    "codex_app_server_protocol.v2.schemas.json",
)
METHOD_FILES = {
    "client_request": "ClientRequest.json",
    "client_notification": "ClientNotification.json",
    "server_notification": "ServerNotification.json",
    "server_request": "ServerRequest.json",
}
EXPECTED_TITLES = {
    "client_request": "ClientRequest",
    "client_notification": "ClientNotification",
    "server_notification": "ServerNotification",
    "server_request": "ServerRequest",
}
EXPECTED_ENVELOPE_REQUIRED = {
    "client_request": frozenset({"id", "method"}),
    "client_notification": frozenset({"method"}),
    "server_notification": frozenset({"method", "params"}),
    "server_request": frozenset({"id", "method", "params"}),
}
EXPECTED_ENVELOPE_PROPERTIES = {
    "client_request": frozenset({"id", "method", "params"}),
    "client_notification": frozenset({"method"}),
    "server_notification": frozenset({"method", "params"}),
    "server_request": frozenset({"id", "method", "params"}),
}
TS_METHOD_FILES = {
    "client_request": "ClientRequest.ts",
    "client_notification": "ClientNotification.ts",
    "server_notification": "ServerNotification.ts",
    "server_request": "ServerRequest.ts",
}
METHOD_CATEGORIES = frozenset(METHOD_FILES)
CPP_CATEGORIES = {
    "client_notification": "SurfaceCategory::ClientNotification",
    "client_request": "SurfaceCategory::ClientRequest",
    "delta_progress_discriminator": "SurfaceCategory::DeltaProgressDiscriminator",
    "item_discriminator": "SurfaceCategory::ItemDiscriminator",
    "server_notification": "SurfaceCategory::ServerNotification",
    "server_request": "SurfaceCategory::ServerRequest",
    "tagged_union_discriminator": "SurfaceCategory::TaggedUnionDiscriminator",
}
CPP_RESULT_CONTRACT_KINDS = {
    "Concrete": "ResultContractKind::Concrete",
    "Unit": "ResultContractKind::Unit",
    "Nullable": "ResultContractKind::Nullable",
    "ProtocolSpecial": "ResultContractKind::ProtocolSpecial",
    "Unresolved": "ResultContractKind::Unresolved",
    "NotApplicable": "ResultContractKind::NotApplicable",
}
CPP_ASSOCIATION_EVIDENCE_KINDS = {
    "VendoredRust": "AssociationEvidenceKind::VendoredRust",
    "VendoredSchemaPair": "AssociationEvidenceKind::VendoredSchemaPair",
    "VendoredTypeScriptCrossCheck": (
        "AssociationEvidenceKind::VendoredTypeScriptCrossCheck"
    ),
    "NotApplicable": "AssociationEvidenceKind::NotApplicable",
}
CPP_A1_SLICES = {
    "A1.0": "A1Slice::A1_0",
    "A1.1": "A1Slice::A1_1",
    "A1.2": "A1Slice::A1_2",
    "A1.3": "A1Slice::A1_3",
    "A1.4": "A1Slice::A1_4",
    "InventoryOnly": "A1Slice::InventoryOnly",
}
A1_SLICE_ORDER = {
    "A1.0": 0,
    "A1.1": 1,
    "A1.2": 2,
    "A1.3": 3,
    "A1.4": 4,
}
SLICE_TYPED_MODULES = {
    "A1.0": "Common",
    "A1.1": "ThreadsTurnsSessions",
    "A1.2": "AccountsModelsConfiguration",
    "A1.3": "CommandsFilesystemReviewsApprovals",
    "A1.4": "IntegrationsAndLongTail",
}
NESTED_ASSIGNMENT_CLASSIFICATIONS = frozenset(
    {
        "CrossCuttingA1_0Exception",
        "RootOwnedNestedUnion",
        "SharedCommon",
        "SharedWithinSlice",
        "StableUnreachableInventory",
    }
)
# Reviewed stable public roots owned by the A1.4 long-tail slice. This exact
# category-specific list intentionally mirrors (and independently checks) the
# deterministic fixture generator's frozen routing decision.
A1_4_PUBLIC_ROOTS = {
    "client_request": frozenset(
        {
            "app/list",
            "externalAgentConfig/detect",
            "externalAgentConfig/import",
            "externalAgentConfig/import/readHistories",
            "feedback/upload",
            "hooks/list",
            "marketplace/add",
            "marketplace/remove",
            "marketplace/upgrade",
            "mcpServer/oauth/login",
            "mcpServer/resource/read",
            "mcpServer/tool/call",
            "mcpServerStatus/list",
            "plugin/install",
            "plugin/installed",
            "plugin/list",
            "plugin/read",
            "plugin/share/checkout",
            "plugin/share/delete",
            "plugin/share/list",
            "plugin/share/save",
            "plugin/share/updateTargets",
            "plugin/skill/read",
            "plugin/uninstall",
            "skills/config/write",
            "skills/extraRoots/set",
            "skills/list",
            "windowsSandbox/readiness",
            "windowsSandbox/setupStart",
        }
    ),
    "server_notification": frozenset(
        {
            "app/list/updated",
            "deprecationNotice",
            "externalAgentConfig/import/completed",
            "externalAgentConfig/import/progress",
            "hook/completed",
            "hook/started",
            "mcpServer/oauthLogin/completed",
            "mcpServer/startupStatus/updated",
            "process/exited",
            "process/outputDelta",
            "remoteControl/status/changed",
            "serverRequest/resolved",
            "skills/changed",
            "warning",
            "windows/worldWritableWarning",
            "windowsSandbox/setupCompleted",
        }
    ),
    "server_request": frozenset(
        {
            "attestation/generate",
            "item/tool/call",
            "item/tool/requestUserInput",
            "mcpServer/elicitation/request",
        }
    ),
}
# SHA-256 over the sorted stable tagged-union key -> reaching-root-id mapping,
# using _reachability_membership_sha256(). The deterministic schema generator
# independently regenerates the full report; this reviewed pin prevents a
# coordinated in-memory/file mutation from remaining internally self-consistent.
PINNED_REACHABILITY_MEMBERSHIP_SHA256 = (
    "c60c3f978ec377f1d0f083921557433f570e3d270136c0b3eaeb02d2ace121fa"
)
CPP_TYPED_SCHEMA_STATUSES = {
    "Complete": "TypedSchemaStatus::Complete",
    "Partial": "TypedSchemaStatus::Partial",
    "NotImplemented": "TypedSchemaStatus::NotImplemented",
    "NotApplicable": "TypedSchemaStatus::NotApplicable",
}
COMPLETENESS_EVIDENCE_FIELDS = (
    "authoritative_root_association",
    "positive_fixture_coverage",
    "required_fields_exercised",
    "schema_properties_exercised",
    "optional_present_exercised",
    "optional_omitted_exercised",
    "nullable_semantics_exercised",
    "reachable_union_alternatives_exercised",
    "direction_assertions_exercised",
    "fixture_current",
    "runtime_decoder_matches_registry",
    "opaque_fields_declared",
    "independently_schema_validated",
    "no_known_schema_fields_dropped",
)
SCHEMA_FIXTURE_COMPLETENESS_FIELDS = frozenset(
    {
        "authoritative_root_association",
        "positive_fixture_coverage",
        "required_fields_exercised",
        "schema_properties_exercised",
        "optional_present_exercised",
        "optional_omitted_exercised",
        "nullable_semantics_exercised",
        "reachable_union_alternatives_exercised",
        "fixture_current",
        "independently_schema_validated",
    }
)
LOCAL_DISPOSITION_FIELDS = frozenset(
    {
        "runtime_disposition",
        "typed_status",
        "typed_implementation",
        "backend_status",
        "backend_core",
        "canonical_state_status",
        "canonical_state",
        "frontend_exposure",
        "frontend_protocol",
        "frontend_security",
        "runtime_target",
        "typed_schema_status",
        "schema_status",
    }
)
ASSOCIATION_ERROR_CODES = frozenset(
    {
        "MissingAssociation",
        "DuplicateAssociation",
        "StaleAssociation",
        "WrongAssociationCategory",
        "WrongParameterType",
        "WrongResultType",
        "ConflictingAssociationEvidence",
        "UnitWithNonUnitResultType",
        "ConcreteWithoutResultType",
        "ContractOnNonRequest",
        "ExperimentalAssociationCountedAsStable",
    }
)
ASSIGNMENT_ERROR_CODES = frozenset(
    {
        "DuplicateModuleSliceAssignment",
        "MissingModuleSliceAssignment",
        "StaleModuleSliceAssignment",
        "AssignmentStabilityMismatch",
        "InvalidAssignmentClassification",
        "AssignmentSliceMismatch",
        "AssignmentDomainSliceMismatch",
        "AssignmentModuleMismatch",
        "DuplicateReachabilityRoot",
        "MissingReachabilityRoot",
        "StaleReachabilityRoot",
        "ReachabilityRootSetMismatch",
        "DuplicateReachabilityRecord",
        "MissingReachabilityRecord",
        "StaleReachabilityRecord",
        "ReachabilityAssignedSliceMismatch",
        "ReachabilityClassificationMismatch",
        "ReachabilityNotEarliestSlice",
        "FalseStableUnreachable",
    }
)
COMPLETENESS_ERROR_CODES = frozenset(
    {
        "RequiredFieldNotExercised",
        "SchemaPropertyNotExercised",
        "OptionalPresentCaseMissing",
        "OptionalOmittedCaseMissing",
        "NullableSemanticsMissing",
        "ReachableUnionAlternativeMissing",
        "DirectionAssertionMissing",
        "StaleFixture",
        "CompletenessRuntimeTargetMismatch",
        "UnrecordedOpaqueField",
        "ClaimedCompleteWithoutAuthoritativeAssociation",
        "ClaimedCompleteWithoutPositiveFixtureCoverage",
        "ClaimedCompleteWithoutIndependentValidation",
        "KnownSchemaFieldDropped",
        "TypedSchemaStatusMismatch",
    }
)
DEFAULT_A1_EVIDENCE_ROOT = (
    Path(__file__).resolve().parent / "app-server-evidence" / "0.144.6"
)
DEFAULT_PINNED_SCHEMA_ROOT = (
    Path(__file__).resolve().parent / "app-server-schema" / "0.144.6"
)
DEFAULT_PINNED_PROTOCOL_SOURCE_ROOT = (
    Path(__file__).resolve().parent
    / "app-server-protocol-source"
    / "0.144.6"
)
DEFAULT_REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_A1_FIXTURE_INDEX = (
    Path(__file__).resolve().parent
    / "app-server-fixtures"
    / "0.144.6"
    / "index.json"
)
OPERATION_PRODUCTION_COVERAGE_FILENAME = (
    "a1-1-operation-production-coverage.json"
)
NOTIFICATION_PRODUCTION_COVERAGE_FILENAME = (
    "a1-1-notification-production-coverage.json"
)
OPERATION_PRODUCTION_COVERAGE_SOURCES = (
    "tools/codex/app_server_surface.py",
    "src/ai/openai/codex/detail/ClientOperationCodecDescriptors.inc",
    "src/ai/openai/codex/detail/ClientOperationCodec.cpp",
    "src/ai/openai/codex/detail/ClientOperationCodec.h",
    "src/ai/openai/codex/detail/ConversationUnionCodecDescriptors.inc",
    "src/ai/openai/codex/detail/ProtocolSurfaceRegistry.cpp",
    "src/ai/openai/codex/detail/ConversationCodec.cpp",
    "src/ai/openai/codex/detail/ItemDecoder.cpp",
    "src/ai/openai/codex/detail/ThreadCodec.cpp",
    "src/ai/openai/codex/detail/ThreadCodec.h",
    "src/ai/openai/codex/detail/TurnCodec.cpp",
    "src/ai/openai/codex/detail/TurnCodec.h",
    "src/ai/openai/codex/typed/Client.cpp",
    "src/ai/openai/codex/typed/Conversation.h",
    "src/ai/openai/codex/typed/Items.h",
    "src/ai/openai/codex/typed/Threads.h",
    "src/ai/openai/codex/typed/Turns.h",
    "src/ai/openai/codex/typed/Types.h",
    "tests/component/codex/CMakeLists.txt",
    "tests/component/codex/CodexA11OperationResultCorpusTest.cpp",
    "tests/component/codex/CodexA11OperationAggregateValueCorpusTest.cpp",
    "tests/component/codex/CodexConversationB4NestedCodecTest.cpp",
    "tests/component/codex/CodexTypedThreadCodecTest.cpp",
    "tests/component/codex/CodexA11OperationWireTest.cpp",
    "tests/installed/codex/CodexTypedConsumer.cpp",
)
NOTIFICATION_PRODUCTION_COVERAGE_SOURCES = (
    "tools/codex/app_server_surface.py",
    "src/ai/openai/codex/detail/ServerNotificationCodecDescriptors.inc",
    "src/ai/openai/codex/detail/EventDecoder.cpp",
    "src/ai/openai/codex/detail/EventDecoder.h",
    "src/ai/openai/codex/detail/ProtocolSurfaceRegistry.cpp",
    "src/ai/openai/codex/detail/ProtocolSurfaceRegistry.h",
    "src/ai/openai/codex/typed/Events.h",
    "tests/component/codex/CodexA11NotificationCodecTest.cpp",
    "tests/component/codex/CMakeLists.txt",
)

# A1.1's checked production-coverage documents are frozen historical evidence.
# Their source inventories still describe the exact tool and CMake inputs used to
# close that slice.  Later A1 slices may extend those two files, so retain only
# those historical source-record values while continuing to hash every production
# implementation/test source live.  The current CMake registrations are validated
# independently below before either historical record is used.
_A1_1_FROZEN_EXTENSIBLE_SOURCE_RECORDS = {
    "tools/codex/app_server_surface.py": {
        "path": "tools/codex/app_server_surface.py",
        "bytes": 323551,
        "sha256": (
            "628254afe57d142e8c0862d0607180a169dadb93844a1ccabc35e61921408b23"
        ),
    },
    "tests/component/codex/CMakeLists.txt": {
        "path": "tests/component/codex/CMakeLists.txt",
        "bytes": 35990,
        "sha256": (
            "6f5a4fdff45cde4dc6bf7f940fb3da4904ce64964fcebbad1f357302843977e5"
        ),
    },
}


def _a1_1_production_coverage_source_record(
    repo_root: Path, relative: str, missing_code: str
) -> dict[str, Any]:
    path = repo_root / relative
    try:
        data = path.read_bytes()
    except OSError as error:
        raise SurfaceError(
            f"{missing_code}: unable to read {relative}: {error}"
        ) from error
    frozen = _A1_1_FROZEN_EXTENSIBLE_SOURCE_RECORDS.get(relative)
    if frozen is not None:
        return dict(frozen)
    return {
        "path": relative,
        "bytes": len(data),
        "sha256": sha256_bytes(data),
    }


def _validate_a1_1_registered_coverage_tests(
    repo_root: Path,
    registrations: tuple[tuple[str, str, bool], ...],
    coverage_definition: str,
    diagnostic_prefix: str,
) -> None:
    cmake_path = repo_root / "tests/component/codex/CMakeLists.txt"
    try:
        cmake = cmake_path.read_text(encoding="utf-8")
    except OSError as error:
        raise SurfaceError(
            f"{diagnostic_prefix}RegistrationMissing: unable to read "
            f"{cmake_path.relative_to(repo_root)}: {error}"
        ) from error

    for ctest_name, source, generic_typed_registration in registrations:
        source_path = repo_root / source
        if not source_path.is_file():
            raise SurfaceError(
                f"{diagnostic_prefix}RegistrationMissing: {ctest_name} "
                f"source {source} is absent"
            )
        source_name = source_path.name
        if generic_typed_registration:
            foreach_match = re.search(
                r"foreach\s*\(\s*typed_test\b(?P<body>.*?)\)",
                cmake,
                flags=re.DOTALL,
            )
            registered = (
                foreach_match is not None
                and ctest_name in foreach_match.group("body").split()
                and re.search(
                    r"snodec_add_test\s*\(\s*\$\{typed_test\}\s+"
                    r"\$\{typed_test\}\.cpp\s*\)",
                    cmake,
                )
                is not None
            )
        else:
            registered = (
                re.search(
                    rf"snodec_add_test\s*\(\s*{re.escape(ctest_name)}\s+"
                    rf"{re.escape(source_name)}\s*\)",
                    cmake,
                )
                is not None
            )
        if not registered:
            raise SurfaceError(
                f"{diagnostic_prefix}RegistrationMissing: {ctest_name} "
                f"is not registered with {source_name}"
            )

        compile_definition = re.search(
            rf"target_compile_definitions\s*\(\s*{re.escape(ctest_name)}\b"
            rf"(?P<body>.*?)\)",
            cmake,
            flags=re.DOTALL,
        )
        if (
            compile_definition is None
            or coverage_definition not in compile_definition.group("body")
        ):
            raise SurfaceError(
                f"{diagnostic_prefix}RegistrationMissing: {ctest_name} "
                f"does not consume {coverage_definition}"
            )


_VENDORED_RUST_ASSOCIATIONS: tuple[dict[str, Any], dict[str, Any]] | None = None


def vendored_rust_associations() -> tuple[dict[str, Any], dict[str, Any]]:
    """Reparse exact vendored Rust authority for association guard checks."""

    global _VENDORED_RUST_ASSOCIATIONS
    if _VENDORED_RUST_ASSOCIATIONS is not None:
        return _VENDORED_RUST_ASSOCIATIONS

    tool_path = Path(__file__).resolve().with_name("app_server_contracts.py")
    specification = importlib.util.spec_from_file_location(
        "snodec_codex_app_server_contracts_authority", tool_path
    )
    if specification is None or specification.loader is None:
        raise SurfaceError(
            f"unable to load vendored Rust association extractor: {tool_path}"
        )
    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    try:
        specification.loader.exec_module(module)
        module.source_provenance(DEFAULT_PINNED_PROTOCOL_SOURCE_ROOT)
        source = (
            DEFAULT_PINNED_PROTOCOL_SOURCE_ROOT / module.COMMON_PATH
        ).read_text(encoding="utf-8")
        clients = {
            association.method: association
            for association in module.parse_request_macro(
                source, "client_request_definitions"
            )
        }
        servers = {
            association.method: association
            for association in module.parse_request_macro(
                source, "server_request_definitions"
            )
        }
    except (OSError, UnicodeError, ValueError, RuntimeError) as error:
        raise SurfaceError(
            f"unable to rederive vendored Rust associations: {error}"
        ) from error
    _VENDORED_RUST_ASSOCIATIONS = (clients, servers)
    return _VENDORED_RUST_ASSOCIATIONS


def schema_result_contract_kind(document: dict[str, Any]) -> str:
    """Derive result semantics from the independently loaded schema root."""

    schema_type = document.get("type")
    if schema_type == "null" or (
        isinstance(schema_type, list) and "null" in schema_type
    ):
        return "Nullable"
    for keyword in ("oneOf", "anyOf"):
        branches = document.get(keyword)
        if isinstance(branches, list) and any(
            isinstance(branch, dict) and branch.get("type") == "null"
            for branch in branches
        ):
            return "Nullable"
    properties = document.get("properties")
    required = document.get("required")
    if (
        schema_type == "object"
        and (properties is None or properties == {})
        and (required is None or required == [])
        and not any(
            keyword in document
            for keyword in ("$ref", "allOf", "anyOf", "oneOf")
        )
    ):
        return "Unit"
    return "Concrete"


CONVERSATION_UNION_CODECS = {
    (
        "tagged_union_discriminator",
        "AgentMessageInputContent",
        "type",
        "encrypted_content",
    ): (
        "ConversationUnionTarget::AgentMessageInputContentEncryptedContent",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "AgentMessageInputContent",
        "type",
        "input_text",
    ): (
        "ConversationUnionTarget::AgentMessageInputContentInputText",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "AskForApproval",
        "$variant",
        "granular",
    ): (
        "ConversationUnionTarget::AskForApprovalGranular",
        "ConversationUnionCodecShape::ExternallyTaggedObject",
        "ConversationUnionCodecDirection::Bidirectional",
    ),
    (
        "tagged_union_discriminator",
        "AskForApproval",
        "$variant",
        "never",
    ): (
        "ConversationUnionTarget::AskForApprovalNever",
        "ConversationUnionCodecShape::ScalarString",
        "ConversationUnionCodecDirection::Bidirectional",
    ),
    (
        "tagged_union_discriminator",
        "AskForApproval",
        "$variant",
        "on-request",
    ): (
        "ConversationUnionTarget::AskForApprovalOnRequest",
        "ConversationUnionCodecShape::ScalarString",
        "ConversationUnionCodecDirection::Bidirectional",
    ),
    (
        "tagged_union_discriminator",
        "AskForApproval",
        "$variant",
        "untrusted",
    ): (
        "ConversationUnionTarget::AskForApprovalUntrusted",
        "ConversationUnionCodecShape::ScalarString",
        "ConversationUnionCodecDirection::Bidirectional",
    ),
    (
        "tagged_union_discriminator",
        "CommandAction",
        "type",
        "listFiles",
    ): (
        "ConversationUnionTarget::CommandActionListFiles",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "ContentItem",
        "type",
        "input_image",
    ): (
        "ConversationUnionTarget::ContentItemInputImage",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "ContentItem",
        "type",
        "input_text",
    ): (
        "ConversationUnionTarget::ContentItemInputText",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "ContentItem",
        "type",
        "output_text",
    ): (
        "ConversationUnionTarget::ContentItemOutputText",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "CommandAction",
        "type",
        "read",
    ): (
        "ConversationUnionTarget::CommandActionRead",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "CommandAction",
        "type",
        "search",
    ): (
        "ConversationUnionTarget::CommandActionSearch",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "CommandAction",
        "type",
        "unknown",
    ): (
        "ConversationUnionTarget::CommandActionUnknown",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "DynamicToolCallOutputContentItem",
        "type",
        "inputImage",
    ): (
        "ConversationUnionTarget::DynamicToolCallOutputContentItemInputImage",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "FunctionCallOutputContentItem",
        "type",
        "encrypted_content",
    ): (
        "ConversationUnionTarget::FunctionCallOutputContentItemEncryptedContent",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "FunctionCallOutputContentItem",
        "type",
        "input_image",
    ): (
        "ConversationUnionTarget::FunctionCallOutputContentItemInputImage",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "FunctionCallOutputContentItem",
        "type",
        "input_text",
    ): (
        "ConversationUnionTarget::FunctionCallOutputContentItemInputText",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "LocalShellAction",
        "type",
        "exec",
    ): (
        "ConversationUnionTarget::LocalShellActionExec",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "DynamicToolCallOutputContentItem",
        "type",
        "inputText",
    ): (
        "ConversationUnionTarget::DynamicToolCallOutputContentItemInputText",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "PatchChangeKind",
        "type",
        "add",
    ): (
        "ConversationUnionTarget::PatchChangeKindAdd",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "ReasoningItemContent",
        "type",
        "reasoning_text",
    ): (
        "ConversationUnionTarget::ReasoningItemContentReasoningText",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "ReasoningItemContent",
        "type",
        "text",
    ): (
        "ConversationUnionTarget::ReasoningItemContentText",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "ReasoningItemReasoningSummary",
        "type",
        "summary_text",
    ): (
        "ConversationUnionTarget::ReasoningItemReasoningSummarySummaryText",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "ResponsesApiWebSearchAction",
        "type",
        "find_in_page",
    ): (
        "ConversationUnionTarget::ResponsesApiWebSearchActionFindInPage",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "ResponsesApiWebSearchAction",
        "type",
        "open_page",
    ): (
        "ConversationUnionTarget::ResponsesApiWebSearchActionOpenPage",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "ResponsesApiWebSearchAction",
        "type",
        "other",
    ): (
        "ConversationUnionTarget::ResponsesApiWebSearchActionOther",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "ResponsesApiWebSearchAction",
        "type",
        "search",
    ): (
        "ConversationUnionTarget::ResponsesApiWebSearchActionSearch",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "PatchChangeKind",
        "type",
        "delete",
    ): (
        "ConversationUnionTarget::PatchChangeKindDelete",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "PatchChangeKind",
        "type",
        "update",
    ): (
        "ConversationUnionTarget::PatchChangeKindUpdate",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "SandboxPolicy",
        "type",
        "dangerFullAccess",
    ): (
        "ConversationUnionTarget::SandboxPolicyDangerFullAccess",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::Bidirectional",
    ),
    (
        "tagged_union_discriminator",
        "SandboxPolicy",
        "type",
        "externalSandbox",
    ): (
        "ConversationUnionTarget::SandboxPolicyExternalSandbox",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::Bidirectional",
    ),
    (
        "tagged_union_discriminator",
        "SandboxPolicy",
        "type",
        "readOnly",
    ): (
        "ConversationUnionTarget::SandboxPolicyReadOnly",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::Bidirectional",
    ),
    (
        "tagged_union_discriminator",
        "SandboxPolicy",
        "type",
        "workspaceWrite",
    ): (
        "ConversationUnionTarget::SandboxPolicyWorkspaceWrite",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::Bidirectional",
    ),
    (
        "tagged_union_discriminator",
        "UserInput",
        "type",
        "image",
    ): (
        "ConversationUnionTarget::UserInputImage",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::Bidirectional",
    ),
    (
        "tagged_union_discriminator",
        "UserInput",
        "type",
        "localImage",
    ): (
        "ConversationUnionTarget::UserInputLocalImage",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::Bidirectional",
    ),
    (
        "tagged_union_discriminator",
        "UserInput",
        "type",
        "mention",
    ): (
        "ConversationUnionTarget::UserInputMention",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::Bidirectional",
    ),
    (
        "tagged_union_discriminator",
        "UserInput",
        "type",
        "skill",
    ): (
        "ConversationUnionTarget::UserInputSkill",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::Bidirectional",
    ),
    (
        "tagged_union_discriminator",
        "UserInput",
        "type",
        "text",
    ): (
        "ConversationUnionTarget::UserInputText",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::Bidirectional",
    ),
    (
        "tagged_union_discriminator",
        "WebSearchAction",
        "type",
        "findInPage",
    ): (
        "ConversationUnionTarget::WebSearchActionFindInPage",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "WebSearchAction",
        "type",
        "openPage",
    ): (
        "ConversationUnionTarget::WebSearchActionOpenPage",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "WebSearchAction",
        "type",
        "other",
    ): (
        "ConversationUnionTarget::WebSearchActionOther",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "WebSearchAction",
        "type",
        "search",
    ): (
        "ConversationUnionTarget::WebSearchActionSearch",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
}

# Commit B4 adds the three operation-aggregate union families.  They reuse the
# same exact-key descriptor mechanism as B2/B3, but remain decode-only because
# the pinned operation closure reaches them only through successful results.
B4_CONVERSATION_UNION_CODECS = {
    (
        "tagged_union_discriminator",
        "SessionSource",
        "$variant",
        "appServer",
    ): (
        "ConversationUnionTarget::SessionSourceAppServer",
        "ConversationUnionCodecShape::ScalarString",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "SessionSource",
        "$variant",
        "cli",
    ): (
        "ConversationUnionTarget::SessionSourceCli",
        "ConversationUnionCodecShape::ScalarString",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "SessionSource",
        "$variant",
        "custom",
    ): (
        "ConversationUnionTarget::SessionSourceCustom",
        "ConversationUnionCodecShape::ExternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "SessionSource",
        "$variant",
        "exec",
    ): (
        "ConversationUnionTarget::SessionSourceExec",
        "ConversationUnionCodecShape::ScalarString",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "SessionSource",
        "$variant",
        "subAgent",
    ): (
        "ConversationUnionTarget::SessionSourceSubAgent",
        "ConversationUnionCodecShape::ExternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "SessionSource",
        "$variant",
        "unknown",
    ): (
        "ConversationUnionTarget::SessionSourceUnknown",
        "ConversationUnionCodecShape::ScalarString",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "SessionSource",
        "$variant",
        "vscode",
    ): (
        "ConversationUnionTarget::SessionSourceVscode",
        "ConversationUnionCodecShape::ScalarString",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "SubAgentSource",
        "$variant",
        "compact",
    ): (
        "ConversationUnionTarget::SubAgentSourceCompact",
        "ConversationUnionCodecShape::ScalarString",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "SubAgentSource",
        "$variant",
        "memory_consolidation",
    ): (
        "ConversationUnionTarget::SubAgentSourceMemoryConsolidation",
        "ConversationUnionCodecShape::ScalarString",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "SubAgentSource",
        "$variant",
        "other",
    ): (
        "ConversationUnionTarget::SubAgentSourceOther",
        "ConversationUnionCodecShape::ExternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "SubAgentSource",
        "$variant",
        "review",
    ): (
        "ConversationUnionTarget::SubAgentSourceReview",
        "ConversationUnionCodecShape::ScalarString",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "SubAgentSource",
        "$variant",
        "thread_spawn",
    ): (
        "ConversationUnionTarget::SubAgentSourceThreadSpawn",
        "ConversationUnionCodecShape::ExternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "ThreadStatus",
        "type",
        "active",
    ): (
        "ConversationUnionTarget::ThreadStatusActive",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "ThreadStatus",
        "type",
        "idle",
    ): (
        "ConversationUnionTarget::ThreadStatusIdle",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "ThreadStatus",
        "type",
        "notLoaded",
    ): (
        "ConversationUnionTarget::ThreadStatusNotLoaded",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
    (
        "tagged_union_discriminator",
        "ThreadStatus",
        "type",
        "systemError",
    ): (
        "ConversationUnionTarget::ThreadStatusSystemError",
        "ConversationUnionCodecShape::InternallyTaggedObject",
        "ConversationUnionCodecDirection::DecodeOnly",
    ),
}
if set(CONVERSATION_UNION_CODECS) & set(B4_CONVERSATION_UNION_CODECS):
    raise AssertionError("B4 conversation-union descriptor keys overlap B2/B3")
CONVERSATION_UNION_CODECS.update(B4_CONVERSATION_UNION_CODECS)

RUNTIME_TARGETS = {
    ("client_request", "ClientRequest", "method", "initialize"): "ClientRequestTarget::Initialize",
    ("client_request", "ClientRequest", "method", "thread/archive"): "ClientRequestTarget::ThreadArchive",
    (
        "client_request",
        "ClientRequest",
        "method",
        "thread/compact/start",
    ): "ClientRequestTarget::ThreadCompactStart",
    ("client_request", "ClientRequest", "method", "thread/delete"): "ClientRequestTarget::ThreadDelete",
    ("client_request", "ClientRequest", "method", "thread/fork"): "ClientRequestTarget::ThreadFork",
    (
        "client_request",
        "ClientRequest",
        "method",
        "thread/goal/clear",
    ): "ClientRequestTarget::ThreadGoalClear",
    (
        "client_request",
        "ClientRequest",
        "method",
        "thread/goal/get",
    ): "ClientRequestTarget::ThreadGoalGet",
    (
        "client_request",
        "ClientRequest",
        "method",
        "thread/goal/set",
    ): "ClientRequestTarget::ThreadGoalSet",
    (
        "client_request",
        "ClientRequest",
        "method",
        "thread/inject_items",
    ): "ClientRequestTarget::ThreadInjectItems",
    ("client_request", "ClientRequest", "method", "thread/start"): "ClientRequestTarget::ThreadStart",
    ("client_request", "ClientRequest", "method", "thread/resume"): "ClientRequestTarget::ThreadResume",
    ("client_request", "ClientRequest", "method", "thread/list"): "ClientRequestTarget::ThreadList",
    (
        "client_request",
        "ClientRequest",
        "method",
        "thread/loaded/list",
    ): "ClientRequestTarget::ThreadLoadedList",
    (
        "client_request",
        "ClientRequest",
        "method",
        "thread/metadata/update",
    ): "ClientRequestTarget::ThreadMetadataUpdate",
    (
        "client_request",
        "ClientRequest",
        "method",
        "thread/name/set",
    ): "ClientRequestTarget::ThreadSetName",
    ("client_request", "ClientRequest", "method", "thread/read"): "ClientRequestTarget::ThreadRead",
    (
        "client_request",
        "ClientRequest",
        "method",
        "thread/rollback",
    ): "ClientRequestTarget::ThreadRollback",
    (
        "client_request",
        "ClientRequest",
        "method",
        "thread/shellCommand",
    ): "ClientRequestTarget::ThreadShellCommand",
    (
        "client_request",
        "ClientRequest",
        "method",
        "thread/unarchive",
    ): "ClientRequestTarget::ThreadUnarchive",
    (
        "client_request",
        "ClientRequest",
        "method",
        "thread/unsubscribe",
    ): "ClientRequestTarget::ThreadUnsubscribe",
    ("client_request", "ClientRequest", "method", "turn/start"): "ClientRequestTarget::TurnStart",
    ("client_request", "ClientRequest", "method", "turn/interrupt"): "ClientRequestTarget::TurnInterrupt",
    ("client_request", "ClientRequest", "method", "turn/steer"): "ClientRequestTarget::TurnSteer",
    ("client_notification", "ClientNotification", "method", "initialized"): "ClientNotificationTarget::Initialized",
    ("server_notification", "ServerNotification", "method", "error"): "ServerNotificationTarget::Error",
    ("server_notification", "ServerNotification", "method", "thread/started"): "ServerNotificationTarget::ThreadStarted",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/status/changed",
    ): "ServerNotificationTarget::ThreadStatusChanged",
    ("server_notification", "ServerNotification", "method", "turn/started"): "ServerNotificationTarget::TurnStarted",
    ("server_notification", "ServerNotification", "method", "turn/completed"): "ServerNotificationTarget::TurnCompleted",
    ("server_notification", "ServerNotification", "method", "item/started"): "ServerNotificationTarget::ItemStarted",
    ("server_notification", "ServerNotification", "method", "item/completed"): "ServerNotificationTarget::ItemCompleted",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "item/agentMessage/delta",
    ): "ServerNotificationTarget::AgentMessageDelta",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "item/commandExecution/terminalInteraction",
    ): "ServerNotificationTarget::TerminalInteraction",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "item/reasoning/textDelta",
    ): "ServerNotificationTarget::ReasoningTextDelta",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "item/reasoning/summaryPartAdded",
    ): "ServerNotificationTarget::ReasoningSummaryPartAdded",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "item/reasoning/summaryTextDelta",
    ): "ServerNotificationTarget::ReasoningSummaryTextDelta",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "item/commandExecution/outputDelta",
    ): "ServerNotificationTarget::CommandExecutionOutputDelta",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "item/fileChange/outputDelta",
    ): "ServerNotificationTarget::FileChangeOutputDelta",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "item/fileChange/patchUpdated",
    ): "ServerNotificationTarget::FileChangePatchUpdated",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "item/mcpToolCall/progress",
    ): "ServerNotificationTarget::McpToolCallProgress",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "item/plan/delta",
    ): "ServerNotificationTarget::PlanDelta",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/archived",
    ): "ServerNotificationTarget::ThreadArchived",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/closed",
    ): "ServerNotificationTarget::ThreadClosed",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/compacted",
    ): "ServerNotificationTarget::ContextCompacted",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/deleted",
    ): "ServerNotificationTarget::ThreadDeleted",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/goal/cleared",
    ): "ServerNotificationTarget::ThreadGoalCleared",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/goal/updated",
    ): "ServerNotificationTarget::ThreadGoalUpdated",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/name/updated",
    ): "ServerNotificationTarget::ThreadNameUpdated",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/realtime/closed",
    ): "ServerNotificationTarget::ThreadRealtimeClosed",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/realtime/error",
    ): "ServerNotificationTarget::ThreadRealtimeError",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/realtime/itemAdded",
    ): "ServerNotificationTarget::ThreadRealtimeItemAdded",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/realtime/outputAudio/delta",
    ): "ServerNotificationTarget::ThreadRealtimeOutputAudioDelta",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/realtime/sdp",
    ): "ServerNotificationTarget::ThreadRealtimeSdp",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/realtime/started",
    ): "ServerNotificationTarget::ThreadRealtimeStarted",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/realtime/transcript/delta",
    ): "ServerNotificationTarget::ThreadRealtimeTranscriptDelta",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/realtime/transcript/done",
    ): "ServerNotificationTarget::ThreadRealtimeTranscriptDone",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/settings/updated",
    ): "ServerNotificationTarget::ThreadSettingsUpdated",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/tokenUsage/updated",
    ): "ServerNotificationTarget::ThreadTokenUsageUpdated",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "thread/unarchived",
    ): "ServerNotificationTarget::ThreadUnarchived",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "turn/diff/updated",
    ): "ServerNotificationTarget::TurnDiffUpdated",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "turn/moderationMetadata",
    ): "ServerNotificationTarget::TurnModerationMetadata",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "turn/plan/updated",
    ): "ServerNotificationTarget::TurnPlanUpdated",
    (
        "server_notification",
        "ServerNotification",
        "method",
        "model/rerouted",
    ): "ServerNotificationTarget::ModelRerouted",
    (
        "server_request",
        "ServerRequest",
        "method",
        "item/commandExecution/requestApproval",
    ): "ServerRequestTarget::CommandExecutionRequestApproval",
    (
        "server_request",
        "ServerRequest",
        "method",
        "item/fileChange/requestApproval",
    ): "ServerRequestTarget::FileChangeRequestApproval",
    (
        "server_request",
        "ServerRequest",
        "method",
        "item/tool/requestUserInput",
    ): "ServerRequestTarget::ToolRequestUserInput",
    (
        "server_request",
        "ServerRequest",
        "method",
        "account/chatgptAuthTokens/refresh",
    ): "ServerRequestTarget::ChatgptAuthTokensRefresh",
    ("item_discriminator", "ThreadItem", "type", "agentMessage"): "ItemDiscriminatorTarget::AgentMessage",
    (
        "item_discriminator",
        "ThreadItem",
        "type",
        "collabAgentToolCall",
    ): "ItemDiscriminatorTarget::CollabAgentToolCall",
    (
        "item_discriminator",
        "ThreadItem",
        "type",
        "commandExecution",
    ): "ItemDiscriminatorTarget::CommandExecution",
    (
        "item_discriminator",
        "ThreadItem",
        "type",
        "contextCompaction",
    ): "ItemDiscriminatorTarget::ContextCompaction",
    (
        "item_discriminator",
        "ThreadItem",
        "type",
        "dynamicToolCall",
    ): "ItemDiscriminatorTarget::DynamicToolCall",
    (
        "item_discriminator",
        "ThreadItem",
        "type",
        "enteredReviewMode",
    ): "ItemDiscriminatorTarget::EnteredReviewMode",
    (
        "item_discriminator",
        "ThreadItem",
        "type",
        "exitedReviewMode",
    ): "ItemDiscriminatorTarget::ExitedReviewMode",
    ("item_discriminator", "ThreadItem", "type", "fileChange"): "ItemDiscriminatorTarget::FileChange",
    ("item_discriminator", "ThreadItem", "type", "hookPrompt"): "ItemDiscriminatorTarget::HookPrompt",
    (
        "item_discriminator",
        "ThreadItem",
        "type",
        "imageGeneration",
    ): "ItemDiscriminatorTarget::ImageGeneration",
    ("item_discriminator", "ThreadItem", "type", "imageView"): "ItemDiscriminatorTarget::ImageView",
    ("item_discriminator", "ThreadItem", "type", "mcpToolCall"): "ItemDiscriminatorTarget::McpToolCall",
    ("item_discriminator", "ThreadItem", "type", "plan"): "ItemDiscriminatorTarget::Plan",
    ("item_discriminator", "ThreadItem", "type", "reasoning"): "ItemDiscriminatorTarget::Reasoning",
    ("item_discriminator", "ThreadItem", "type", "sleep"): "ItemDiscriminatorTarget::Sleep",
    (
        "item_discriminator",
        "ThreadItem",
        "type",
        "subAgentActivity",
    ): "ItemDiscriminatorTarget::SubAgentActivity",
    ("item_discriminator", "ThreadItem", "type", "userMessage"): "ItemDiscriminatorTarget::UserMessage",
    ("item_discriminator", "ThreadItem", "type", "webSearch"): "ItemDiscriminatorTarget::WebSearch",
    (
        "item_discriminator",
        "ResponseItem",
        "type",
        "agent_message",
    ): "ResponseItemTarget::AgentMessage",
    ("item_discriminator", "ResponseItem", "type", "compaction"): "ResponseItemTarget::Compaction",
    (
        "item_discriminator",
        "ResponseItem",
        "type",
        "compaction_trigger",
    ): "ResponseItemTarget::CompactionTrigger",
    (
        "item_discriminator",
        "ResponseItem",
        "type",
        "context_compaction",
    ): "ResponseItemTarget::ContextCompaction",
    (
        "item_discriminator",
        "ResponseItem",
        "type",
        "custom_tool_call",
    ): "ResponseItemTarget::CustomToolCall",
    (
        "item_discriminator",
        "ResponseItem",
        "type",
        "custom_tool_call_output",
    ): "ResponseItemTarget::CustomToolCallOutput",
    (
        "item_discriminator",
        "ResponseItem",
        "type",
        "function_call",
    ): "ResponseItemTarget::FunctionCall",
    (
        "item_discriminator",
        "ResponseItem",
        "type",
        "function_call_output",
    ): "ResponseItemTarget::FunctionCallOutput",
    (
        "item_discriminator",
        "ResponseItem",
        "type",
        "image_generation_call",
    ): "ResponseItemTarget::ImageGenerationCall",
    (
        "item_discriminator",
        "ResponseItem",
        "type",
        "local_shell_call",
    ): "ResponseItemTarget::LocalShellCall",
    ("item_discriminator", "ResponseItem", "type", "message"): "ResponseItemTarget::Message",
    ("item_discriminator", "ResponseItem", "type", "other"): "ResponseItemTarget::Other",
    ("item_discriminator", "ResponseItem", "type", "reasoning"): "ResponseItemTarget::Reasoning",
    (
        "item_discriminator",
        "ResponseItem",
        "type",
        "tool_search_call",
    ): "ResponseItemTarget::ToolSearchCall",
    (
        "item_discriminator",
        "ResponseItem",
        "type",
        "tool_search_output",
    ): "ResponseItemTarget::ToolSearchOutput",
    (
        "item_discriminator",
        "ResponseItem",
        "type",
        "web_search_call",
    ): "ResponseItemTarget::WebSearchCall",
    (
        "tagged_union_discriminator",
        "CodexErrorInfo",
        "$variant",
        "activeTurnNotSteerable",
    ): "CodexErrorInfoTarget::ActiveTurnNotSteerable",
    (
        "tagged_union_discriminator",
        "CodexErrorInfo",
        "$variant",
        "badRequest",
    ): "CodexErrorInfoTarget::BadRequest",
    (
        "tagged_union_discriminator",
        "CodexErrorInfo",
        "$variant",
        "contextWindowExceeded",
    ): "CodexErrorInfoTarget::ContextWindowExceeded",
    (
        "tagged_union_discriminator",
        "CodexErrorInfo",
        "$variant",
        "cyberPolicy",
    ): "CodexErrorInfoTarget::CyberPolicy",
    (
        "tagged_union_discriminator",
        "CodexErrorInfo",
        "$variant",
        "httpConnectionFailed",
    ): "CodexErrorInfoTarget::HttpConnectionFailed",
    (
        "tagged_union_discriminator",
        "CodexErrorInfo",
        "$variant",
        "internalServerError",
    ): "CodexErrorInfoTarget::InternalServerError",
    (
        "tagged_union_discriminator",
        "CodexErrorInfo",
        "$variant",
        "other",
    ): "CodexErrorInfoTarget::Other",
    (
        "tagged_union_discriminator",
        "CodexErrorInfo",
        "$variant",
        "responseStreamConnectionFailed",
    ): "CodexErrorInfoTarget::ResponseStreamConnectionFailed",
    (
        "tagged_union_discriminator",
        "CodexErrorInfo",
        "$variant",
        "responseStreamDisconnected",
    ): "CodexErrorInfoTarget::ResponseStreamDisconnected",
    (
        "tagged_union_discriminator",
        "CodexErrorInfo",
        "$variant",
        "responseTooManyFailedAttempts",
    ): "CodexErrorInfoTarget::ResponseTooManyFailedAttempts",
    (
        "tagged_union_discriminator",
        "CodexErrorInfo",
        "$variant",
        "sandboxError",
    ): "CodexErrorInfoTarget::SandboxError",
    (
        "tagged_union_discriminator",
        "CodexErrorInfo",
        "$variant",
        "serverOverloaded",
    ): "CodexErrorInfoTarget::ServerOverloaded",
    (
        "tagged_union_discriminator",
        "CodexErrorInfo",
        "$variant",
        "sessionBudgetExceeded",
    ): "CodexErrorInfoTarget::SessionBudgetExceeded",
    (
        "tagged_union_discriminator",
        "CodexErrorInfo",
        "$variant",
        "threadRollbackFailed",
    ): "CodexErrorInfoTarget::ThreadRollbackFailed",
    (
        "tagged_union_discriminator",
        "CodexErrorInfo",
        "$variant",
        "unauthorized",
    ): "CodexErrorInfoTarget::Unauthorized",
    (
        "tagged_union_discriminator",
        "CodexErrorInfo",
        "$variant",
        "usageLimitExceeded",
    ): "CodexErrorInfoTarget::UsageLimitExceeded",
}

if set(RUNTIME_TARGETS) & set(CONVERSATION_UNION_CODECS):
    raise AssertionError("conversation-union runtime targets duplicate an existing mapping")
RUNTIME_TARGETS.update(
    {
        key: descriptor[0]
        for key, descriptor in CONVERSATION_UNION_CODECS.items()
    }
)

SERVER_NOTIFICATION_PAYLOAD_TYPES_BY_METHOD = {
    "error": "typed::TurnErrorEvent",
    "item/agentMessage/delta": "typed::AgentMessageDeltaNotification",
    "item/commandExecution/outputDelta": "typed::CommandExecutionOutputDeltaNotification",
    "item/commandExecution/terminalInteraction": "typed::TerminalInteractionNotification",
    "item/completed": "typed::ItemCompletedNotification",
    "item/fileChange/outputDelta": "typed::FileChangeOutputDeltaNotification",
    "item/fileChange/patchUpdated": "typed::FileChangePatchUpdatedNotification",
    "item/mcpToolCall/progress": "typed::McpToolCallProgressNotification",
    "item/plan/delta": "typed::PlanDeltaNotification",
    "item/reasoning/summaryPartAdded": "typed::ReasoningSummaryPartAddedNotification",
    "item/reasoning/summaryTextDelta": "typed::ReasoningSummaryTextDeltaNotification",
    "item/reasoning/textDelta": "typed::ReasoningTextDeltaNotification",
    "item/started": "typed::ItemStartedNotification",
    "model/rerouted": "typed::ModelRerouted",
    "thread/archived": "typed::ThreadArchivedNotification",
    "thread/closed": "typed::ThreadClosedNotification",
    "thread/compacted": "typed::ContextCompactedNotification",
    "thread/deleted": "typed::ThreadDeletedNotification",
    "thread/goal/cleared": "typed::ThreadGoalClearedNotification",
    "thread/goal/updated": "typed::ThreadGoalUpdatedNotification",
    "thread/name/updated": "typed::ThreadNameUpdatedNotification",
    "thread/realtime/closed": "typed::ThreadRealtimeClosedNotification",
    "thread/realtime/error": "typed::ThreadRealtimeErrorNotification",
    "thread/realtime/itemAdded": "typed::ThreadRealtimeItemAddedNotification",
    "thread/realtime/outputAudio/delta": "typed::ThreadRealtimeOutputAudioDeltaNotification",
    "thread/realtime/sdp": "typed::ThreadRealtimeSdpNotification",
    "thread/realtime/started": "typed::ThreadRealtimeStartedNotification",
    "thread/realtime/transcript/delta": "typed::ThreadRealtimeTranscriptDeltaNotification",
    "thread/realtime/transcript/done": "typed::ThreadRealtimeTranscriptDoneNotification",
    "thread/settings/updated": "typed::ThreadSettingsUpdatedNotification",
    "thread/started": "typed::ThreadStartedNotification",
    "thread/status/changed": "typed::ThreadStatusChangedNotification",
    "thread/tokenUsage/updated": "typed::ThreadTokenUsageUpdatedNotification",
    "thread/unarchived": "typed::ThreadUnarchivedNotification",
    "turn/completed": "typed::TurnCompletedNotification",
    "turn/diff/updated": "typed::TurnDiffUpdatedNotification",
    "turn/moderationMetadata": "typed::TurnModerationMetadataNotification",
    "turn/plan/updated": "typed::TurnPlanUpdatedNotification",
    "turn/started": "typed::TurnStartedNotification",
}
SERVER_NOTIFICATION_EVENT_ALTERNATIVES_BY_METHOD = {
    method: payload_type.removeprefix("typed::")
    for method, payload_type in SERVER_NOTIFICATION_PAYLOAD_TYPES_BY_METHOD.items()
}
SERVER_NOTIFICATION_EVENT_ALTERNATIVES_BY_METHOD.update(
    {
        "item/agentMessage/delta": "AgentMessageDelta",
        "item/commandExecution/outputDelta": "CommandOutputDelta",
        "item/completed": "ItemCompleted",
        "item/fileChange/patchUpdated": "FileChangeUpdated",
        "item/reasoning/summaryTextDelta": "ReasoningDelta",
        "item/reasoning/textDelta": "ReasoningDelta",
        "item/started": "ItemStarted",
        "thread/started": "ThreadStarted",
        "thread/status/changed": "ThreadStatusChanged",
        "thread/tokenUsage/updated": "TokenUsageUpdated",
        "turn/completed": "TurnCompleted|TurnFailed",
        "turn/started": "TurnStarted",
    }
)
EXISTING_MODELED_SERVER_NOTIFICATION_METHODS = frozenset(
    {
        "error",
        "item/agentMessage/delta",
        "item/commandExecution/outputDelta",
        "item/completed",
        "item/fileChange/patchUpdated",
        "item/reasoning/summaryTextDelta",
        "item/reasoning/textDelta",
        "item/started",
        "model/rerouted",
        "thread/started",
        "thread/status/changed",
        "thread/tokenUsage/updated",
        "turn/completed",
        "turn/started",
    }
)

SERVER_NOTIFICATION_CODECS = {
    key: (target, SERVER_NOTIFICATION_PAYLOAD_TYPES_BY_METHOD[key[3]])
    for key, target in RUNTIME_TARGETS.items()
    if key[0] == "server_notification"
}
if (
    len(SERVER_NOTIFICATION_CODECS) != 39
    or set(SERVER_NOTIFICATION_PAYLOAD_TYPES_BY_METHOD)
    != {key[3] for key in SERVER_NOTIFICATION_CODECS}
):
    raise AssertionError(
        "server-notification payload mapping must cover every production target exactly"
    )

EXISTING_FRONTEND_OPERATION_DETAILS = {
    (
        "client_request",
        "thread/start",
    ): "v1 `thread.start`: cwd, model, modelProvider, approvalPolicy, sandboxMode, ephemeral",
    (
        "client_request",
        "thread/resume",
    ): "v1 `thread.resume`: threadId, cwd, model, modelProvider, approvalPolicy, sandboxMode",
    (
        "client_request",
        "thread/list",
    ): "v1 `thread.list`: cursor, limit, archived, searchTerm",
    (
        "client_request",
        "thread/read",
    ): "v1 `thread.read`: threadId, includeTurns",
    (
        "client_request",
        "turn/start",
    ): "v1 `turn.start`: threadId, input, cwd, model, reasoningEffort, approvalPolicy, sandboxPolicy",
    (
        "client_request",
        "turn/interrupt",
    ): "v1 `turn.interrupt`: threadId, turnId",
    (
        "server_request",
        "item/commandExecution/requestApproval",
    ): (
        "v1 `request.pending` command_approval summary: threadId, turnId, itemId, "
        "command, cwd, reason; response via `request.approval.respond` decision"
    ),
    (
        "server_request",
        "item/fileChange/requestApproval",
    ): (
        "v1 `request.pending` file_change_approval summary: threadId, turnId, itemId, "
        "reason, grantRoot; response via `request.approval.respond` decision"
    ),
    (
        "server_request",
        "item/tool/requestUserInput",
    ): (
        "v1 `request.pending` user_input summary: threadId, turnId, itemId, question "
        "metadata/options and autoResolutionMs; response via `request.userInput.respond`"
    ),
    (
        "server_request",
        "account/chatgptAuthTokens/refresh",
    ): (
        "v1 `request.pending` authentication summary: reason, previousAccountId; "
        "response via `request.authentication.respond` token/account fields"
    ),
}
EXISTING_FRONTEND_OPERATIONS = frozenset(EXISTING_FRONTEND_OPERATION_DETAILS)
BACKEND_IMPLEMENTED_IDENTITIES = frozenset(
    {
        ("client_request", "thread/start"),
        ("client_request", "thread/resume"),
        ("client_request", "thread/list"),
        ("client_request", "thread/read"),
        ("client_request", "turn/start"),
        ("client_request", "turn/interrupt"),
        ("server_notification", "error"),
        ("server_notification", "thread/started"),
        ("server_notification", "thread/status/changed"),
        ("server_notification", "turn/started"),
        ("server_notification", "turn/completed"),
        ("server_notification", "item/started"),
        ("server_notification", "item/completed"),
        ("server_notification", "item/agentMessage/delta"),
        ("server_notification", "item/reasoning/textDelta"),
        ("server_notification", "item/reasoning/summaryTextDelta"),
        ("server_notification", "item/commandExecution/outputDelta"),
        ("server_notification", "item/fileChange/patchUpdated"),
        ("server_notification", "thread/tokenUsage/updated"),
        ("server_notification", "model/rerouted"),
        ("server_request", "item/commandExecution/requestApproval"),
        ("server_request", "item/fileChange/requestApproval"),
        ("server_request", "item/tool/requestUserInput"),
        ("server_request", "account/chatgptAuthTokens/refresh"),
        ("item_discriminator", "agentMessage"),
        ("item_discriminator", "userMessage"),
        ("item_discriminator", "reasoning"),
        ("item_discriminator", "commandExecution"),
        ("item_discriminator", "fileChange"),
        ("item_discriminator", "mcpToolCall"),
        ("item_discriminator", "dynamicToolCall"),
        ("item_discriminator", "webSearch"),
    }
)
CANONICAL_STATE_IMPLEMENTED_IDENTITIES = frozenset(
    {
        ("client_request", "thread/start"),
        ("client_request", "thread/resume"),
        ("client_request", "thread/list"),
        ("client_request", "thread/read"),
        ("client_request", "turn/start"),
        ("client_request", "turn/interrupt"),
        ("server_notification", "error"),
        ("server_notification", "thread/started"),
        ("server_notification", "thread/status/changed"),
        ("server_notification", "turn/started"),
        ("server_notification", "turn/completed"),
        ("server_notification", "item/started"),
        ("server_notification", "item/completed"),
        ("server_notification", "item/agentMessage/delta"),
        ("server_notification", "item/reasoning/textDelta"),
        ("server_notification", "item/reasoning/summaryTextDelta"),
        ("server_notification", "item/commandExecution/outputDelta"),
        ("server_notification", "item/fileChange/patchUpdated"),
        ("server_notification", "thread/tokenUsage/updated"),
        ("server_notification", "model/rerouted"),
        ("server_request", "item/commandExecution/requestApproval"),
        ("server_request", "item/fileChange/requestApproval"),
        ("server_request", "item/tool/requestUserInput"),
        ("server_request", "account/chatgptAuthTokens/refresh"),
        ("item_discriminator", "agentMessage"),
        ("item_discriminator", "userMessage"),
        ("item_discriminator", "reasoning"),
        ("item_discriminator", "commandExecution"),
        ("item_discriminator", "fileChange"),
        ("item_discriminator", "mcpToolCall"),
        ("item_discriminator", "dynamicToolCall"),
        ("item_discriminator", "webSearch"),
    }
)


class SurfaceError(RuntimeError):
    """An authoritative artifact is missing, ambiguous, or unrecognized."""


def canonical_json(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    encoded = json.dumps(value, ensure_ascii=False, indent=2, sort_keys=False) + "\n"
    path.write_text(encoded, encoding="utf-8")


def cpp_string(value: str) -> str:
    return json.dumps(value, ensure_ascii=True)


def load_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise SurfaceError(f"unable to load JSON artifact {path}: {error}") from error


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def tree_records(root: Path) -> list[dict[str, Any]]:
    if not root.is_dir():
        raise SurfaceError(f"artifact tree is missing: {root}")
    records: list[dict[str, Any]] = []
    for path in sorted(candidate for candidate in root.rglob("*") if candidate.is_file()):
        relative = path.relative_to(root).as_posix()
        data = path.read_bytes()
        records.append({"path": relative, "bytes": len(data), "sha256": sha256_bytes(data)})
    if not records:
        raise SurfaceError(f"artifact tree contains no files: {root}")
    return records


def aggregate_hash(records: Sequence[dict[str, Any]]) -> str:
    payload = "".join(f"{record['sha256']}  {record['path']}\n" for record in records)
    return sha256_bytes(payload.encode("utf-8"))


def compare_json_trees(first: Path, second: Path) -> dict[str, Any]:
    first_records = tree_records(first)
    second_records = tree_records(second)
    first_by_path = {record["path"]: record for record in first_records}
    second_by_path = {record["path"]: record for record in second_records}
    missing_from_second = sorted(set(first_by_path) - set(second_by_path))
    missing_from_first = sorted(set(second_by_path) - set(first_by_path))
    differing = sorted(
        path
        for path in set(first_by_path) & set(second_by_path)
        if first_by_path[path]["sha256"] != second_by_path[path]["sha256"]
    )
    semantic_differences: list[str] = []
    ordering_only: list[str] = []
    object_member_order_differences: list[dict[str, Any]] = []
    for relative in differing:
        first_path = first / relative
        second_path = second / relative
        first_json = load_json(first_path) if first_path.suffix == ".json" else None
        second_json = load_json(second_path) if second_path.suffix == ".json" else None
        if first_path.suffix == ".json" and first_json == second_json:
            ordering_only.append(relative)
            object_member_order_differences.extend(
                {
                    "file": relative,
                    **difference,
                }
                for difference in json_object_order_differences(first_json, second_json)
            )
        else:
            semantic_differences.append(relative)
    return {
        "byte_identical": not missing_from_second and not missing_from_first and not differing,
        "missing_from_first": missing_from_first,
        "missing_from_second": missing_from_second,
        "ordering_only_differences": ordering_only,
        "object_member_order_differences": object_member_order_differences,
        "semantic_differences": semantic_differences,
    }


def json_object_order_differences(
    first: Any, second: Any, pointer: str = ""
) -> list[dict[str, Any]]:
    differences: list[dict[str, Any]] = []
    if isinstance(first, dict) and isinstance(second, dict):
        first_keys = list(first)
        second_keys = list(second)
        if first_keys != second_keys and set(first_keys) == set(second_keys):
            changed_indices = [
                index
                for index, (first_key, second_key) in enumerate(
                    zip(first_keys, second_keys, strict=True)
                )
                if first_key != second_key
            ]
            first_index = min(changed_indices)
            last_index = max(changed_indices)
            differences.append(
                {
                    "json_pointer": pointer or "/",
                    "first_changed_index": first_index,
                    "last_changed_index": last_index,
                    "first_order": first_keys[first_index : last_index + 1],
                    "second_order": second_keys[first_index : last_index + 1],
                }
            )
        for key in first:
            if key in second:
                encoded = key.replace("~", "~0").replace("/", "~1")
                differences.extend(
                    json_object_order_differences(
                        first[key], second[key], f"{pointer}/{encoded}"
                    )
                )
    elif isinstance(first, list) and isinstance(second, list):
        for index, (first_value, second_value) in enumerate(
            zip(first, second, strict=True)
        ):
            differences.extend(
                json_object_order_differences(
                    first_value, second_value, f"{pointer}/{index}"
                )
            )
    return differences


def json_pointer(document: Any, pointer: str, context: str) -> Any:
    if not pointer:
        return document
    if not pointer.startswith("/"):
        raise SurfaceError(f"invalid JSON pointer in {context}: #{pointer}")
    current = document
    for encoded in pointer[1:].split("/"):
        token = encoded.replace("~1", "/").replace("~0", "~")
        if isinstance(current, dict) and token in current:
            current = current[token]
        elif isinstance(current, list) and token.isdigit() and int(token) < len(current):
            current = current[int(token)]
        else:
            raise SurfaceError(f"unresolved JSON pointer in {context}: #{pointer}")
    return current


class SchemaRepository:
    def __init__(self, root: Path):
        self.root = root.resolve()
        self._documents: dict[Path, Any] = {}

    def load(self, path: Path) -> Any:
        absolute = path.resolve()
        try:
            absolute.relative_to(self.root)
        except ValueError as error:
            raise SurfaceError(f"schema reference escapes vendored tree: {path}") from error
        if absolute not in self._documents:
            self._documents[absolute] = load_json(absolute)
        return self._documents[absolute]

    def resolve(self, schema: Any, current_file: Path) -> tuple[Any, Path]:
        seen: set[tuple[Path, str]] = set()
        value = schema
        source = current_file.resolve()
        while isinstance(value, dict) and "$ref" in value:
            reference = value["$ref"]
            if not isinstance(reference, str):
                raise SurfaceError(f"non-string $ref in {source}")
            if "://" in reference:
                raise SurfaceError(f"network schema reference is not repository-local: {reference}")
            file_part, separator, pointer = reference.partition("#")
            target_file = (source.parent / file_part).resolve() if file_part else source
            marker = (target_file, pointer)
            if marker in seen:
                raise SurfaceError(f"cyclic direct $ref chain at {target_file}#{pointer}")
            seen.add(marker)
            document = self.load(target_file)
            value = json_pointer(document, pointer, f"{target_file}#{pointer}") if separator else document
            source = target_file
        return value, source

    def validate_all_references(self) -> int:
        count = 0
        for path in sorted(self.root.rglob("*.json")):
            document = self.load(path)
            for node in walk_json(document):
                if isinstance(node, dict) and "$ref" in node:
                    self.resolve(node, path)
                    count += 1
        return count


def walk_json(value: Any) -> Iterator[Any]:
    yield value
    if isinstance(value, dict):
        for child in value.values():
            yield from walk_json(child)
    elif isinstance(value, list):
        for child in value:
            yield from walk_json(child)


def schema_description(*schemas: Any) -> str:
    descriptions: list[str] = []
    for schema in schemas:
        if isinstance(schema, dict):
            description = schema.get("description")
            if isinstance(description, str) and description.strip():
                descriptions.append(" ".join(description.split()))
    return " ".join(dict.fromkeys(descriptions))


def deprecated_marker(*schemas: Any) -> bool:
    for schema in schemas:
        if not isinstance(schema, dict):
            continue
        if schema.get("deprecated") is True:
            return True
        description = schema.get("description")
        if isinstance(description, str) and re.search(r"\bdeprecated\b", description, re.IGNORECASE):
            return True
    return False


def schema_type_name(reference_or_schema: Any) -> str | None:
    if not isinstance(reference_or_schema, dict):
        return None
    reference = reference_or_schema.get("$ref")
    if isinstance(reference, str):
        fragment = reference.split("#", 1)[-1]
        return fragment.rsplit("/", 1)[-1].replace("~1", "/").replace("~0", "~")
    title = reference_or_schema.get("title")
    return title if isinstance(title, str) else None


def required_field_summary(schema: Any, repository: SchemaRepository, source: Path) -> list[str]:
    resolved, resolved_source = repository.resolve(schema, source)
    required: set[str] = set()
    if isinstance(resolved, dict):
        values = resolved.get("required")
        if isinstance(values, list) and all(isinstance(value, str) for value in values):
            required.update(values)
        all_of = resolved.get("allOf")
        if isinstance(all_of, list):
            for part in all_of:
                required.update(required_field_summary(part, repository, resolved_source))
    return sorted(required)


def property_path_summary(
    schema: Any,
    repository: SchemaRepository,
    source: Path,
    prefix: str = "",
    depth: int = 0,
    ancestors: frozenset[tuple[Path, str]] = frozenset(),
) -> set[str]:
    if depth >= 12:
        return {f"{prefix}.…" if prefix else "…"}
    resolved, resolved_source = repository.resolve(schema, source)
    if not isinstance(resolved, dict):
        return set()
    marker = (
        resolved_source,
        sha256_bytes(canonical_json(resolved).encode("utf-8")),
    )
    if marker in ancestors:
        return set()
    nested_ancestors = ancestors | {marker}
    paths: set[str] = set()
    if resolved.get("type") == "array" and resolved.get("items") is not None:
        paths.update(
            property_path_summary(
                resolved["items"],
                repository,
                resolved_source,
                f"{prefix}[]" if prefix else "[]",
                depth + 1,
                nested_ancestors,
            )
        )
    properties = resolved.get("properties")
    if isinstance(properties, dict):
        for field, child in properties.items():
            path = f"{prefix}.{field}" if prefix else field
            paths.add(path)
            child_resolved, child_source = repository.resolve(child, resolved_source)
            if isinstance(child_resolved, dict) and child_resolved.get("type") == "array":
                items = child_resolved.get("items")
                if items is not None:
                    paths.update(
                        property_path_summary(
                            items,
                            repository,
                            child_source,
                            f"{path}[]",
                            depth + 1,
                            nested_ancestors,
                        )
                    )
            else:
                paths.update(
                    property_path_summary(
                        child,
                        repository,
                        resolved_source,
                        path,
                        depth + 1,
                        nested_ancestors,
                    )
                )
    for keyword in ("allOf", "oneOf", "anyOf"):
        branches = resolved.get(keyword)
        if isinstance(branches, list):
            for branch in branches:
                paths.update(
                    property_path_summary(
                        branch,
                        repository,
                        resolved_source,
                        prefix,
                        depth + 1,
                        nested_ancestors,
                    )
                )
    return paths


def method_union_variants(
    document: Any, category: str, context: str
) -> list[tuple[dict[str, Any], str]]:
    if not isinstance(document, dict):
        raise SurfaceError(f"unrecognized {context} layout: object schema is required")
    variants = document.get("oneOf")
    if not isinstance(variants, list) or not variants:
        raise SurfaceError(f"unrecognized {context} layout: non-empty top-level oneOf is required")
    expected_required = EXPECTED_ENVELOPE_REQUIRED[category]
    expected_properties = EXPECTED_ENVELOPE_PROPERTIES[category]
    result: list[tuple[dict[str, Any], str]] = []
    seen: set[str] = set()
    for index, variant in enumerate(variants):
        if not isinstance(variant, dict) or variant.get("type") != "object":
            raise SurfaceError(f"{context} oneOf[{index}] is not an object schema")
        required = variant.get("required")
        if (
            not isinstance(required, list)
            or not all(isinstance(field, str) for field in required)
            or not expected_required.issubset(required)
        ):
            raise SurfaceError(
                f"{context} oneOf[{index}] does not require the expected envelope fields"
            )
        properties = variant.get("properties")
        if not isinstance(properties, dict) or not expected_properties.issubset(properties):
            raise SurfaceError(
                f"{context} oneOf[{index}] lacks the expected envelope properties"
            )
        method_schema = properties["method"]
        enum_values = method_schema.get("enum") if isinstance(method_schema, dict) else None
        if (
            not isinstance(method_schema, dict)
            or method_schema.get("type") != "string"
            or not isinstance(enum_values, list)
            or len(enum_values) != 1
            or not isinstance(enum_values[0], str)
        ):
            raise SurfaceError(
                f"{context} oneOf[{index}] has no singleton string method enum"
            )
        name = enum_values[0]
        if name in seen:
            raise SurfaceError(f"duplicate upstream method entry: {category}:{name}")
        seen.add(name)
        result.append((variant, name))
    return result


def method_entries(schema_root: Path, stability: str) -> list[dict[str, Any]]:
    repository = SchemaRepository(schema_root)
    aggregate_path = schema_root / "codex_app_server_protocol.schemas.json"
    aggregate = repository.load(aggregate_path)
    aggregate_definitions = aggregate.get("definitions")
    if aggregate.get("title") != "CodexAppServerProtocol" or not isinstance(aggregate_definitions, dict):
        raise SurfaceError("unrecognized codex_app_server_protocol.schemas.json layout")
    standalone_v2_path = schema_root / "codex_app_server_protocol.v2.schemas.json"
    standalone_v2 = repository.load(standalone_v2_path)
    standalone_v2_definitions = standalone_v2.get("definitions")
    if standalone_v2.get("title") != "CodexAppServerProtocolV2" or not isinstance(
        standalone_v2_definitions, dict
    ):
        raise SurfaceError("unrecognized codex_app_server_protocol.v2.schemas.json layout")
    entries: list[dict[str, Any]] = []
    for category, relative in METHOD_FILES.items():
        path = schema_root / relative
        document = repository.load(path)
        if document.get("title") != EXPECTED_TITLES[category]:
            raise SurfaceError(
                f"unrecognized {relative} layout: expected title {EXPECTED_TITLES[category]!r}, "
                f"got {document.get('title')!r}"
            )
        variants = method_union_variants(document, category, relative)
        dedicated_names = [name for _, name in variants]
        title = EXPECTED_TITLES[category]
        aggregate_schema = aggregate_definitions.get(title)
        if aggregate_schema is None:
            raise SurfaceError(f"aggregate schema lacks method union definition {title}")
        aggregate_names = [
            name
            for _, name in method_union_variants(
                aggregate_schema,
                category,
                f"{aggregate_path.name}#/definitions/{title}",
            )
        ]
        if dedicated_names != aggregate_names:
            raise SurfaceError(
                f"{relative} method order/surface differs from the combined aggregate definition"
            )
        standalone_schema = standalone_v2_definitions.get(title)
        if standalone_schema is not None:
            standalone_names = [
                name
                for _, name in method_union_variants(
                    standalone_schema,
                    category,
                    f"{standalone_v2_path.name}#/definitions/{title}",
                )
            ]
            if dedicated_names != standalone_names:
                raise SurfaceError(
                    f"{relative} method order/surface differs from the standalone v2 aggregate definition"
                )
        for index, (variant, name) in enumerate(variants):
            properties = variant.get("properties")
            assert isinstance(properties, dict)
            method_schema = properties["method"]
            params_schema = properties.get("params", {}) if isinstance(properties, dict) else {}
            resolved_params, _ = repository.resolve(params_schema, path)
            params_type = schema_type_name(params_schema) or schema_type_name(resolved_params)
            param_properties, _ = merged_properties(params_schema, repository, path)
            entry = {
                "id": f"{category}:{name}",
                "name": name,
                "category": category,
                "domain": EXPECTED_TITLES[category],
                "discriminator_field": "method",
                "stability": stability,
                "in_stable": stability == "stable",
                "in_experimental": True,
                "deprecated": deprecated_marker(variant, method_schema, resolved_params),
                "delta_or_progress": bool(re.search(r"delta|progress", name, re.IGNORECASE)),
                "description": schema_description(variant, resolved_params),
                "params": {
                    "type": params_type,
                    "required_fields": required_field_summary(params_schema, repository, path),
                    "fields": sorted(param_properties),
                    "property_paths": sorted(
                        property_path_summary(params_schema, repository, path)
                    ),
                },
                "result": {
                    "type": None,
                    "note": (
                        "The generated request union associates method with params only; it does not "
                        "authoritatively associate a result schema."
                    )
                    if category in {"client_request", "server_request"}
                    else "not_applicable",
                },
                "sources": [{"file": relative, "pointer": f"/oneOf/{index}"}],
            }
            entries.append(entry)
    return entries


def merged_properties(schema: Any, repository: SchemaRepository, source: Path) -> tuple[dict[str, Any], Path]:
    resolved, resolved_source = repository.resolve(schema, source)
    if not isinstance(resolved, dict):
        return {}, resolved_source
    properties: dict[str, Any] = {}
    direct = resolved.get("properties")
    if isinstance(direct, dict):
        properties.update(direct)
    all_of = resolved.get("allOf")
    if isinstance(all_of, list):
        for part in all_of:
            part_properties, _ = merged_properties(part, repository, resolved_source)
            for key, value in part_properties.items():
                if key in properties and canonical_json(properties[key]) != canonical_json(value):
                    raise SurfaceError(f"conflicting allOf property {key!r} in {resolved_source}")
                properties[key] = value
    return properties, resolved_source


def singleton_literal(schema: Any, repository: SchemaRepository, source: Path) -> str | None:
    resolved, _ = repository.resolve(schema, source)
    if not isinstance(resolved, dict):
        return None
    if isinstance(resolved.get("const"), str):
        return resolved["const"]
    enum_values = resolved.get("enum")
    if isinstance(enum_values, list) and len(enum_values) == 1 and isinstance(enum_values[0], str):
        return enum_values[0]
    return None


def discover_item_union_names(
    definition_sets: Sequence[tuple[str, dict[str, Any], Path]],
    repository: SchemaRepository,
) -> set[str]:
    names: set[str] = set()
    for _, definitions, source_path in definition_sets:
        for definition_name, definition in definitions.items():
            if not re.search(r"Item(?:Started|Completed)Notification$", definition_name):
                continue
            properties, source = merged_properties(definition, repository, source_path)
            item_schema = properties.get("item")
            if not isinstance(item_schema, dict):
                continue
            item_name = schema_type_name(item_schema)
            resolved, _ = repository.resolve(item_schema, source)
            if item_name and isinstance(resolved, dict) and (
                isinstance(resolved.get("oneOf"), list) or isinstance(resolved.get("anyOf"), list)
            ):
                names.add(item_name)
    if any("ThreadItem" in definitions for _, definitions, _ in definition_sets):
        names.add("ThreadItem")
    return names


def tagged_union_entries(schema_root: Path, stability: str) -> tuple[list[dict[str, Any]], list[dict[str, str]]]:
    bundle_path = schema_root / "codex_app_server_protocol.schemas.json"
    v2_bundle_path = schema_root / "codex_app_server_protocol.v2.schemas.json"
    repository = SchemaRepository(schema_root)
    bundle = repository.load(bundle_path)
    definitions = bundle.get("definitions")
    if bundle.get("title") != "CodexAppServerProtocol" or not isinstance(definitions, dict):
        raise SurfaceError("unrecognized codex_app_server_protocol.schemas.json layout")
    v2_definitions = definitions.get("v2")
    if not isinstance(v2_definitions, dict):
        raise SurfaceError("unrecognized aggregate layout: definitions.v2 map is required")
    v2_bundle = repository.load(v2_bundle_path)
    standalone_v2_definitions = v2_bundle.get("definitions")
    if v2_bundle.get("title") != "CodexAppServerProtocolV2" or not isinstance(standalone_v2_definitions, dict):
        raise SurfaceError("unrecognized codex_app_server_protocol.v2.schemas.json layout")
    definition_sets: list[tuple[str, dict[str, Any], Path]] = [
        (
            "/definitions",
            {name: definition for name, definition in definitions.items() if name != "v2"},
            bundle_path,
        ),
        ("/definitions/v2", v2_definitions, bundle_path),
        ("/definitions", standalone_v2_definitions, v2_bundle_path),
    ]
    item_union_names = discover_item_union_names(definition_sets, repository)
    entries: list[dict[str, Any]] = []
    untagged: list[dict[str, str]] = []
    seen: dict[tuple[str, str, str], dict[str, Any]] = {}
    seen_untagged: set[tuple[str, str, str, str]] = set()
    for pointer_prefix, namespace_definitions, source_path in definition_sets:
        for definition_name in sorted(namespace_definitions):
            if definition_name in EXPECTED_TITLES.values():
                continue
            definition = namespace_definitions[definition_name]
            resolved, resolved_source = repository.resolve(definition, source_path)
            if not isinstance(resolved, dict):
                continue
            union_keyword = None
            variants = None
            for candidate in ("oneOf", "anyOf"):
                minimum_variants = 1 if candidate == "oneOf" else 2
                if isinstance(resolved.get(candidate), list) and len(resolved[candidate]) >= minimum_variants:
                    union_keyword = candidate
                    variants = resolved[candidate]
                    break
            if variants is None or union_keyword is None:
                continue
            variant_literals: list[dict[str, str]] = []
            for variant in variants:
                properties, property_source = merged_properties(variant, repository, resolved_source)
                literals: dict[str, str] = {}
                for field, property_schema in properties.items():
                    value = singleton_literal(property_schema, repository, property_source)
                    if value is not None:
                        literals[field] = value
                variant_literals.append(literals)
            common_fields = set(variant_literals[0])
            for literals in variant_literals[1:]:
                common_fields.intersection_update(literals)
            valid_fields = [
                field
                for field in common_fields
                if len({literals[field] for literals in variant_literals}) == len(variant_literals)
            ]
            if valid_fields:
                priorities = {"type": 0, "kind": 1, "status": 2, "mode": 3, "action": 4}
                valid_fields.sort(key=lambda field: (priorities.get(field, 100), field))
                tag_field = valid_fields[0]
                tagged_variants = [
                    (index, literals[tag_field]) for index, literals in enumerate(variant_literals)
                ]
            else:
                tagged_variants: list[tuple[int, str]] = []
                has_scalar_variant = False
                has_object_variant = False
                scalar_only_values: list[str] = []
                for variant_index, variant in enumerate(variants):
                    resolved_variant, variant_source = repository.resolve(variant, resolved_source)
                    enum_values = (
                        resolved_variant.get("enum") if isinstance(resolved_variant, dict) else None
                    )
                    if (
                        isinstance(enum_values, list)
                        and enum_values
                        and all(isinstance(value, str) for value in enum_values)
                    ):
                        has_scalar_variant = True
                        scalar_only_values.extend(enum_values)
                        tagged_variants.extend((variant_index, value) for value in enum_values)
                        continue
                    properties, _ = merged_properties(resolved_variant, repository, variant_source)
                    required = resolved_variant.get("required") if isinstance(resolved_variant, dict) else None
                    required_names = (
                        required
                        if isinstance(required, list) and all(isinstance(value, str) for value in required)
                        else []
                    )
                    candidates = [name for name in required_names if name in properties]
                    if len(candidates) != 1:
                        tagged_variants = []
                        break
                    has_object_variant = True
                    tagged_variants.append((variant_index, candidates[0]))
                values = [value for _, value in tagged_variants]
                if (
                    not has_scalar_variant
                    or not has_object_variant
                    or not tagged_variants
                    or len(set(values)) != len(values)
                ):
                    reason = (
                        "scalar-only non-dispatch union"
                        if has_scalar_variant and not has_object_variant and tagged_variants
                        else "not a common-field or mixed externally tagged dispatch union"
                    )
                    untagged_identity = (source_path.name, definition_name, union_keyword, reason)
                    if untagged_identity not in seen_untagged:
                        seen_untagged.add(untagged_identity)
                        untagged.append(
                            {
                                "source": source_path.name,
                                "definition": definition_name,
                                "union_keyword": union_keyword,
                                "reason": reason,
                            }
                        )
                    continue
                tag_field = "$variant"
            values = [value for _, value in tagged_variants]
            category = "tagged_union_discriminator"
            if definition_name in item_union_names:
                category = "item_discriminator"
            elif re.search(r"delta|progress", definition_name + " " + " ".join(values), re.IGNORECASE):
                category = "delta_progress_discriminator"
            for index, value in tagged_variants:
                variant = variants[index]
                resolved_variant, _ = repository.resolve(variant, resolved_source)
                candidate_entry = {
                    "id": f"{category}:{definition_name}:{tag_field}:{value}",
                    "name": value,
                    "category": category,
                    "domain": definition_name,
                    "discriminator_field": tag_field,
                    "stability": stability,
                    "in_stable": stability == "stable",
                    "in_experimental": True,
                    "deprecated": deprecated_marker(definition, variant, resolved_variant),
                    "delta_or_progress": category == "delta_progress_discriminator"
                    or bool(re.search(r"delta|progress", definition_name + " " + value, re.IGNORECASE)),
                    "description": schema_description(definition, variant, resolved_variant),
                    "params": None,
                    "result": None,
                    "sources": [
                        {
                            "file": source_path.name,
                            "pointer": f"{pointer_prefix}/{definition_name}/{union_keyword}/{index}",
                        }
                    ],
                }
                identity = (definition_name, tag_field, value)
                previous = seen.get(identity)
                if previous is not None:
                    comparable_keys = (
                        "name",
                        "category",
                        "domain",
                        "discriminator_field",
                        "deprecated",
                        "delta_or_progress",
                    )
                    if any(previous[key] != candidate_entry[key] for key in comparable_keys):
                        raise SurfaceError(
                            f"conflicting tagged-union discriminator: {definition_name}.{tag_field}={value}"
                        )
                    previous["sources"].extend(candidate_entry["sources"])
                    continue
                seen[identity] = candidate_entry
                entries.append(candidate_entry)
    return entries, untagged


def merge_stability(
    stable_entries: Sequence[dict[str, Any]], experimental_entries: Sequence[dict[str, Any]]
) -> list[dict[str, Any]]:
    stable = {entry["id"]: copy.deepcopy(entry) for entry in stable_entries}
    experimental = {entry["id"]: copy.deepcopy(entry) for entry in experimental_entries}
    missing_from_experimental = sorted(set(stable) - set(experimental))
    if missing_from_experimental:
        raise SurfaceError(
            "stable surface entries disappeared from --experimental generation: "
            + ", ".join(missing_from_experimental)
        )
    merged: list[dict[str, Any]] = []
    for identity in sorted(set(stable) | set(experimental)):
        if identity in stable:
            entry = stable[identity]
            entry["stability"] = "stable"
            entry["in_stable"] = True
            entry["in_experimental"] = True
            if identity in experimental:
                stable_shape = {
                    key: value
                    for key, value in stable[identity].items()
                    if key
                    not in {
                        "description",
                        "sources",
                        "stability",
                        "in_stable",
                        "in_experimental",
                        "params",
                    }
                }
                experimental_shape = {
                    key: value
                    for key, value in experimental[identity].items()
                    if key
                    not in {
                        "description",
                        "sources",
                        "stability",
                        "in_stable",
                        "in_experimental",
                        "params",
                    }
                }
                if stable_shape != experimental_shape:
                    raise SurfaceError(f"stable entry changes shape under --experimental: {identity}")
                if stable[identity].get("params") != experimental[identity].get("params"):
                    entry["experimental_params"] = experimental[identity].get("params")
        else:
            entry = experimental[identity]
            entry["stability"] = "experimental_only"
            entry["in_stable"] = False
            entry["in_experimental"] = True
        merged.append(entry)
    detect_category_collisions(merged)
    return sorted(
        merged,
        key=lambda entry: (
            entry["category"],
            entry["domain"],
            entry["discriminator_field"],
            entry["name"],
        ),
    )


def detect_category_collisions(entries: Sequence[dict[str, Any]]) -> None:
    methods: dict[str, str] = {}
    identities: set[tuple[str, str, str, str]] = set()
    discriminator_categories: dict[tuple[str, str, str], str] = {}
    for entry in entries:
        identity = (
            entry["category"],
            entry["domain"],
            entry["discriminator_field"],
            entry["name"],
        )
        if identity in identities:
            raise SurfaceError(f"duplicate surface identity: {identity}")
        identities.add(identity)
        discriminator_identity = (
            entry["domain"],
            entry["discriminator_field"],
            entry["name"],
        )
        previous_category = discriminator_categories.get(discriminator_identity)
        if previous_category is not None and previous_category != entry["category"]:
            raise SurfaceError(
                "protocol discriminator category collision for "
                f"{discriminator_identity}: {previous_category} and {entry['category']}"
            )
        discriminator_categories[discriminator_identity] = entry["category"]
        if entry["category"] in METHOD_CATEGORIES:
            previous = methods.get(entry["name"])
            if previous is not None and previous != entry["category"]:
                raise SurfaceError(
                    f"method category collision for {entry['name']!r}: {previous} and {entry['category']}"
                )
            methods[entry["name"]] = entry["category"]


def split_typescript_union(expression: str) -> list[str]:
    parts: list[str] = []
    start = 0
    braces = brackets = parentheses = 0
    quote: str | None = None
    escaped = False
    for index, character in enumerate(expression):
        if quote is not None:
            if escaped:
                escaped = False
            elif character == "\\":
                escaped = True
            elif character == quote:
                quote = None
            continue
        if character in {'"', "'", "`"}:
            quote = character
        elif character == "{":
            braces += 1
        elif character == "}":
            braces -= 1
        elif character == "[":
            brackets += 1
        elif character == "]":
            brackets -= 1
        elif character == "(":
            parentheses += 1
        elif character == ")":
            parentheses -= 1
        elif character == "|" and not any((braces, brackets, parentheses)):
            parts.append(expression[start:index].strip())
            start = index + 1
    parts.append(expression[start:].strip())
    return [part for part in parts if part]


def typescript_type_expression(text: str) -> tuple[str, str] | None:
    match = re.search(r"\bexport\s+type\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*=", text)
    if not match:
        return None
    name = match.group(1)
    start = match.end()
    # ts-rs emits one exported type per file and terminates it with the final
    # semicolon. Comments can contain angle brackets and other punctuation, so
    # treating those as TypeScript nesting tokens is less reliable than this
    # generator invariant.
    end = text.rfind(";")
    if end >= start:
        return name, text[start:end].strip()
    raise SurfaceError(f"unterminated generated TypeScript export type {name}")


def outer_literal_properties(branch: str) -> dict[str, str]:
    literals: dict[str, str] = {}
    depth = 0
    quote: str | None = None
    escaped = False
    index = 0
    while index < len(branch):
        character = branch[index]
        if quote is not None:
            if escaped:
                escaped = False
            elif character == "\\":
                escaped = True
            elif character == quote:
                quote = None
            index += 1
            continue
        if character in {'"', "'", "`"}:
            quote = character
            index += 1
            continue
        if character == "{":
            depth += 1
            index += 1
            continue
        if character == "}":
            depth -= 1
            index += 1
            continue
        if depth == 1:
            match = re.match(
                r'\s*(?:"([^"]+)"|([A-Za-z_$][A-Za-z0-9_$]*))\s*:\s*"([^"]+)"',
                branch[index:],
            )
            if match:
                field = match.group(1) or match.group(2)
                literals[field] = match.group(3)
                index += match.end()
                continue
        index += 1
    return literals


def outer_property_names(branch: str) -> list[str]:
    names: list[str] = []
    depth = 0
    quote: str | None = None
    escaped = False
    index = 0
    while index < len(branch):
        character = branch[index]
        if quote is not None:
            if escaped:
                escaped = False
            elif character == "\\":
                escaped = True
            elif character == quote:
                quote = None
            index += 1
            continue
        if character in {'"', "'", "`"}:
            quote = character
            index += 1
            continue
        if character == "{":
            depth += 1
            index += 1
            continue
        if character == "}":
            depth -= 1
            index += 1
            continue
        if depth == 1:
            match = re.match(
                r'\s*(?:"([^"]+)"|([A-Za-z_$][A-Za-z0-9_$]*))\s*\??\s*:',
                branch[index:],
            )
            if match:
                names.append(match.group(1) or match.group(2))
                index += match.end()
                continue
        index += 1
    return names


def typescript_census(root: Path) -> dict[str, Any]:
    if not root.is_dir():
        raise SurfaceError(f"TypeScript cross-check tree is missing: {root}")
    methods: dict[str, list[str]] = {}
    for category, relative in TS_METHOD_FILES.items():
        path = root / relative
        text = path.read_text(encoding="utf-8")
        parsed = typescript_type_expression(text)
        if parsed is None or parsed[0] != EXPECTED_TITLES[category]:
            raise SurfaceError(f"unrecognized generated TypeScript method union: {path}")
        names = re.findall(r'"method"\s*:\s*"([^"]+)"', parsed[1])
        if len(names) != len(set(names)):
            raise SurfaceError(f"duplicate generated TypeScript method in {path}")
        methods[category] = sorted(names)
    discriminators: set[tuple[str, str, str, str]] = set()
    for path in sorted(root.rglob("*.ts")):
        parsed = typescript_type_expression(path.read_text(encoding="utf-8"))
        if parsed is None:
            continue
        owner, expression = parsed
        if owner in EXPECTED_TITLES.values():
            continue
        expression = re.sub(r"/\*.*?\*/", "", expression, flags=re.DOTALL)
        expression = re.sub(r"//[^\n]*", "", expression)
        repeated_literals: dict[str, set[str]] = {}
        for field, value in re.findall(
            r'"([^"]+)"\s*:\s*"([^"]+)"', expression
        ):
            repeated_literals.setdefault(field, set()).add(value)
        source = path.relative_to(root).as_posix()
        for field, values in repeated_literals.items():
            if len(values) >= 2:
                discriminators.update(
                    (source, owner, field, value) for value in values
                )
        branches = split_typescript_union(expression)
        literals = [outer_literal_properties(branch) for branch in branches]
        common = set(literals[0])
        for branch_literals in literals[1:]:
            common.intersection_update(branch_literals)
        for field in sorted(common):
            values = [branch_literals[field] for branch_literals in literals]
            if len(set(values)) == len(values):
                discriminators.update((source, owner, field, value) for value in values)
        if common:
            continue
        external_values: list[str] = []
        has_object_variant = False
        has_scalar_variant = False
        for branch in branches:
            stripped = branch.strip()
            literal_match = re.fullmatch(r'"([^"]+)"', stripped)
            if literal_match:
                has_scalar_variant = True
                external_values.append(literal_match.group(1))
                continue
            names = outer_property_names(branch)
            if len(names) != 1:
                external_values = []
                break
            has_object_variant = True
            external_values.append(names[0])
        if (
            has_object_variant
            and has_scalar_variant
            and len(branches) >= 2
            and len(external_values) == len(branches)
            and len(set(external_values)) == len(external_values)
        ):
            discriminators.update(
                (source, owner, "$variant", value) for value in external_values
            )
    return {
        "methods": methods,
        "discriminators": sorted(
            (
                {"domain": owner, "field": field, "name": name}
                | {"source": source}
                for source, owner, field, name in discriminators
            ),
            key=lambda entry: (
                entry["source"],
                entry["domain"],
                entry["field"],
                entry["name"],
            ),
        ),
    }


def cross_check_typescript(entries: Sequence[dict[str, Any]], ts_root: Path) -> dict[str, Any]:
    census = typescript_census(ts_root)
    discrepancies: dict[str, Any] = {"methods": {}, "discriminators": {}}
    for category in sorted(METHOD_CATEGORIES):
        schema_names = {entry["name"] for entry in entries if entry["category"] == category}
        ts_names = set(census["methods"][category])
        discrepancies["methods"][category] = {
            "schema_only": sorted(schema_names - ts_names),
            "typescript_only": sorted(ts_names - schema_names),
        }
    schema_groups: dict[tuple[str, str], set[str]] = {}
    for entry in entries:
        if entry["category"] not in METHOD_CATEGORIES:
            schema_groups.setdefault(
                (entry["domain"], entry["discriminator_field"]), set()
            ).add(entry["name"])
    ts_groups: dict[tuple[str, str, str], set[str]] = {}
    for entry in census["discriminators"]:
        ts_groups.setdefault(
            (entry["source"], entry["domain"], entry["field"]), set()
        ).add(entry["name"])

    matched_ts_groups: set[tuple[str, str, str]] = set()
    schema_only: list[dict[str, str]] = []
    aliases: list[dict[str, str]] = []
    for (domain, field), expected_values in sorted(schema_groups.items()):
        exact_candidates = [
            key
            for key, values in ts_groups.items()
            if key[1] == domain and key[2] == field and values == expected_values
        ]
        candidates = exact_candidates or [
            key
            for key, values in ts_groups.items()
            if key[2] == field and values == expected_values
        ]
        if len(candidates) != 1:
            schema_only.extend(
                {"domain": domain, "field": field, "name": value}
                for value in sorted(expected_values)
            )
            continue
        match = candidates[0]
        matched_ts_groups.add(match)
        if match[1] != domain:
            aliases.append(
                {
                    "schema_domain": domain,
                    "typescript_domain": match[1],
                    "typescript_source": match[0],
                    "reason": "unique discriminator field/value signature match",
                }
            )

    unmatched_ts_groups = [
        {
            "typescript_source": source,
            "typescript_domain": domain,
            "field": field,
            "values": sorted(values),
        }
        for (source, domain, field), values in sorted(ts_groups.items())
        if (source, domain, field) not in matched_ts_groups
    ]
    discrepancies["discriminators"] = {
        "schema_only": schema_only,
        "typescript_only": unmatched_ts_groups,
        "aliases": aliases,
    }
    discrepancies["explanations"] = [
        {
            "discrepancy": (
                "TypeScript-only getAuthStatus, getConversationSummary, and gitDiffToRemote client "
                "requests, plus rawResponseItem/completed server notification"
            ),
            "explanation": (
                "The generated TypeScript envelope unions contain these entries while the generated "
                "JSON Schema envelope unions omit them. JSON Schema remains authoritative for the "
                "vendored manifest; the entries are not merged into it."
            ),
        },
        {
            "discrepancy": "TypeScript-only root SessionSource tagged union",
            "explanation": (
                "This legacy root type is reachable from the TypeScript-only getConversationSummary "
                "surface. The distinct v2/SessionSource.ts union matches the JSON v2 SessionSource "
                "definition and is cross-checked separately."
            ),
        },
        {
            "discrepancy": "ResponsesApiWebSearchAction JSON/TypeScript type-name difference",
            "explanation": (
                "The JSON definition matches root WebSearchAction.ts by its unique type discriminator "
                "value signature. The distinct v2 WebSearchAction matches v2/WebSearchAction.ts."
            ),
        },
    ]
    return discrepancies


def surface_counts(entries: Sequence[dict[str, Any]]) -> dict[str, Any]:
    categories = sorted({entry["category"] for entry in entries})
    stable = {
        category: sum(1 for entry in entries if entry["stability"] == "stable" and entry["category"] == category)
        for category in categories
    }
    experimental_only = {
        category: sum(
            1
            for entry in entries
            if entry["stability"] == "experimental_only" and entry["category"] == category
        )
        for category in categories
    }
    return {
        "stable": {**stable, "total": sum(stable.values())},
        "experimental_only": {**experimental_only, "total": sum(experimental_only.values())},
        "experimental_inclusive": {
            **{
                category: stable[category] + experimental_only[category]
                for category in categories
            },
            "total": len(entries),
        },
    }


def extract_surface(schema_version_root: Path) -> dict[str, Any]:
    stable_root = schema_version_root / "stable"
    experimental_root = schema_version_root / "experimental"
    for root in (stable_root, experimental_root):
        missing = [relative for relative in REQUIRED_SCHEMA_FILES if not (root / relative).is_file()]
        if missing:
            raise SurfaceError(f"{root} lacks required generated files: {', '.join(missing)}")
    stable_repository = SchemaRepository(stable_root)
    experimental_repository = SchemaRepository(experimental_root)
    stable_ref_count = stable_repository.validate_all_references()
    experimental_ref_count = experimental_repository.validate_all_references()
    stable_methods = method_entries(stable_root, "stable")
    experimental_methods = method_entries(experimental_root, "experimental_only")
    stable_discriminators, stable_untagged = tagged_union_entries(stable_root, "stable")
    experimental_discriminators, experimental_untagged = tagged_union_entries(
        experimental_root, "experimental_only"
    )
    entries = merge_stability(
        [*stable_methods, *stable_discriminators],
        [*experimental_methods, *experimental_discriminators],
    )
    stable_records = tree_records(stable_root)
    experimental_records = tree_records(experimental_root)
    return {
        "format_version": FORMAT_VERSION,
        "codex_version": CODEX_VERSION,
        "starting_snodec_sha": STARTING_SNODEC_SHA,
        "schema_authority": "vendored generated JSON Schema",
        "schema_trees": {
            "stable_aggregate_sha256": aggregate_hash(stable_records),
            "experimental_aggregate_sha256": aggregate_hash(experimental_records),
        },
        "entries": entries,
        "counts": surface_counts(entries),
        "layout_evidence": {
            "stable_resolved_reference_count": stable_ref_count,
            "experimental_resolved_reference_count": experimental_ref_count,
            "stable_untagged_named_unions": stable_untagged,
            "experimental_untagged_named_unions": experimental_untagged,
        },
    }


def verify_provenance(schema_version_root: Path, provenance_path: Path) -> None:
    provenance = load_json(provenance_path)
    if provenance.get("format_version") != PROVENANCE_FORMAT_VERSION:
        raise SurfaceError("unsupported provenance format_version")
    if provenance.get("codex_version") != CODEX_VERSION:
        raise SurfaceError("provenance Codex version does not match the pinned tool")
    if provenance.get("upstream") != UPSTREAM_PROVENANCE:
        raise SurfaceError("provenance upstream attribution does not match the pinned tool")
    license_provenance = UPSTREAM_PROVENANCE["license"]
    for field in ("license_file", "notice_file"):
        expected_file = license_provenance[field]
        path = schema_version_root / expected_file["path"]
        try:
            data = path.read_bytes()
        except OSError as error:
            raise SurfaceError(f"unable to read required upstream attribution file {path}: {error}") from error
        if len(data) != expected_file["bytes"] or hashlib.sha256(data).hexdigest() != expected_file["sha256"]:
            raise SurfaceError(f"required upstream attribution file does not match provenance: {path}")
    for stability in ("stable", "experimental"):
        expected = provenance.get("schema_trees", {}).get(stability)
        if not isinstance(expected, dict):
            raise SurfaceError(f"provenance lacks {stability} schema tree")
        actual_records = tree_records(schema_version_root / stability)
        if actual_records != expected.get("files"):
            raise SurfaceError(f"{stability} vendored schema files or hashes do not match provenance")
        actual_aggregate = aggregate_hash(actual_records)
        if actual_aggregate != expected.get("aggregate_sha256"):
            raise SurfaceError(f"{stability} vendored schema aggregate hash mismatch")


def build_provenance(
    schema_version_root: Path,
    stable_first: Path,
    stable_second: Path,
    experimental_first: Path,
    experimental_second: Path,
    ts_stable: Path,
    ts_experimental: Path,
    stable_cross_check: dict[str, Any],
    experimental_cross_check: dict[str, Any],
) -> dict[str, Any]:
    stable_records = tree_records(schema_version_root / "stable")
    experimental_records = tree_records(schema_version_root / "experimental")
    ts_stable_records = tree_records(ts_stable)
    ts_experimental_records = tree_records(ts_experimental)
    return {
        "format_version": PROVENANCE_FORMAT_VERSION,
        "codex_version": CODEX_VERSION,
        "starting_snodec_sha": STARTING_SNODEC_SHA,
        "upstream": copy.deepcopy(UPSTREAM_PROVENANCE),
        "generation_commands": [
            'CODEX_BIN="${CODEX_BIN:-codex}"',
            '"$CODEX_BIN" --version',
            '"$CODEX_BIN" app-server generate-json-schema --out /tmp/snodec-codex-schema-stable-a',
            '"$CODEX_BIN" app-server generate-json-schema --out /tmp/snodec-codex-schema-stable-b',
            '"$CODEX_BIN" app-server generate-json-schema --out /tmp/snodec-codex-schema-experimental-a --experimental',
            '"$CODEX_BIN" app-server generate-json-schema --out /tmp/snodec-codex-schema-experimental-b --experimental',
            '"$CODEX_BIN" app-server generate-ts --out /tmp/snodec-codex-ts-stable',
            '"$CODEX_BIN" app-server generate-ts --out /tmp/snodec-codex-ts-experimental --experimental',
        ],
        "schema_generation_comparison": {
            "stable": compare_json_trees(stable_first, stable_second),
            "experimental": compare_json_trees(experimental_first, experimental_second),
            "explanation": (
                "The generator emits semantically identical JSON but nondeterministically orders a trailing "
                "subset of the top-level definitions object in codex_app_server_protocol.v2.schemas.json. "
                "One first-generation tree is vendored byte-for-byte; semantic content is not normalized."
            ),
        },
        "schema_trees": {
            "stable": {
                "aggregate_algorithm": "sha256 of sorted '<file-sha256>  <relative-path>\\n' records",
                "aggregate_sha256": aggregate_hash(stable_records),
                "files": stable_records,
            },
            "experimental": {
                "aggregate_algorithm": "sha256 of sorted '<file-sha256>  <relative-path>\\n' records",
                "aggregate_sha256": aggregate_hash(experimental_records),
                "files": experimental_records,
            },
        },
        "typescript_cross_check": {
            "stable": {
                "aggregate_sha256": aggregate_hash(ts_stable_records),
                "file_count": len(ts_stable_records),
                "discrepancies": stable_cross_check,
            },
            "experimental": {
                "aggregate_sha256": aggregate_hash(ts_experimental_records),
                "file_count": len(ts_experimental_records),
                "discrepancies": experimental_cross_check,
            },
            "authority_rule": (
                "TypeScript is an independent cross-check only. TypeScript-only entries are recorded as "
                "discrepancies and are not inserted into the JSON-schema-derived manifest."
            ),
        },
    }


def surface_key(entry: dict[str, Any]) -> tuple[str, str, str, str]:
    """Return the exact canonical ProtocolSurfaceKey for a manifest-like row."""

    key = entry.get("surface_key", entry.get("protocol_surface_key", entry.get("key", entry)))
    if not isinstance(key, dict):
        raise SurfaceError("protocol evidence row has no object-valued surface key")
    category = key.get("category")
    domain = key.get("domain")
    field = key.get("field", key.get("discriminator_field"))
    name = key.get("name")
    if not all(isinstance(value, str) and value for value in (category, domain, field, name)):
        raise SurfaceError("protocol evidence row has an invalid exact surface key")
    return category, domain, field, name


def evidence_records(
    document: dict[str, Any], names: Sequence[str], description: str
) -> list[dict[str, Any]]:
    reject_local_dispositions(document, description)
    for name in names:
        records = document.get(name)
        if isinstance(records, list):
            if not all(isinstance(record, dict) for record in records):
                raise SurfaceError(f"{description} contains a non-object record")
            return records
    raise SurfaceError(
        f"{description} has none of the required arrays: {', '.join(names)}"
    )


def reject_local_dispositions(record: Any, description: str) -> None:
    def walk(value: Any, path: str) -> None:
        if isinstance(value, dict):
            prohibited = set(value) & LOCAL_DISPOSITION_FIELDS
            if prohibited:
                raise SurfaceError(
                    f"{description} contains local disposition fields at "
                    f"{path}: {sorted(prohibited)}"
                )
            for key, child in value.items():
                walk(child, f"{path}/{key}")
        elif isinstance(value, list):
            for index, child in enumerate(value):
                walk(child, f"{path}/{index}")

    walk(record, "$")


def load_a1_registry_evidence(
    root: Path = DEFAULT_A1_EVIDENCE_ROOT,
    *,
    require_operation_production_coverage: bool = True,
    require_notification_production_coverage: bool = True,
) -> dict[str, Any]:
    filenames = {
        "operation_contracts": "operation-contracts.json",
        "assignments": "module-slice-assignment.json",
        "reachability": "nested-reachability.json",
        "fixture_coverage": "fixture-coverage.json",
        "type_closure": "a1-1-type-closure.json",
    }
    if require_operation_production_coverage:
        filenames["operation_production_coverage"] = (
            OPERATION_PRODUCTION_COVERAGE_FILENAME
        )
    if require_notification_production_coverage:
        filenames["notification_production_coverage"] = (
            NOTIFICATION_PRODUCTION_COVERAGE_FILENAME
        )
    result: dict[str, Any] = {}
    for key, filename in filenames.items():
        path = root / filename
        try:
            document = load_json(path)
        except OSError as error:
            raise SurfaceError(f"unable to read required A1 evidence {path}: {error}") from error
        if not isinstance(document, dict):
            raise SurfaceError(f"required A1 evidence is not a JSON object: {path}")
        result[key] = document
    return result


def _diagnostic(code: str, key: tuple[str, str, str, str], message: str) -> dict[str, Any]:
    if (
        code not in ASSOCIATION_ERROR_CODES
        and code not in ASSIGNMENT_ERROR_CODES
        and code not in COMPLETENESS_ERROR_CODES
    ):
        raise AssertionError(f"unregistered protocol evidence diagnostic code: {code}")
    return {
        "code": code,
        "surface_key": {
            "category": key[0],
            "domain": key[1],
            "field": key[2],
            "name": key[3],
        },
        "message": message,
    }


def association_diagnostics(
    manifest: dict[str, Any],
    contracts_document: dict[str, Any],
    registry_entries: Sequence[dict[str, Any]] | None = None,
) -> list[dict[str, Any]]:
    """Validate evidence and registry associations in both directions.

    The evidence contains no local dispositions. The canonical registry copies
    its operation fields from this evidence and remains the sole runtime/local
    disposition authority.
    """

    rust_client_authority, rust_server_authority = (
        vendored_rust_associations()
    )
    manifest_entries = {surface_key(entry): entry for entry in manifest["entries"]}
    expected = {
        key: entry
        for key, entry in manifest_entries.items()
        if entry["stability"] == "stable"
        and entry["category"] in {"client_request", "server_request"}
    }
    contracts = evidence_records(
        contracts_document, ("contracts",), "operation-contract evidence"
    )
    by_key: dict[tuple[str, str, str, str], dict[str, Any]] = {}
    duplicate_keys: set[tuple[str, str, str, str]] = set()
    category_replacements: set[tuple[str, str, str, str]] = set()
    diagnostics: list[dict[str, Any]] = []
    evidence_identity_owners: dict[
        tuple[str, str], tuple[str, str, str, str]
    ] = {}
    schema_pair_branch_owners: dict[
        tuple[str, int], tuple[str, str, str, str]
    ] = {}

    for contract in contracts:
        reject_local_dispositions(contract, "operation-contract evidence")
        key = surface_key(contract)
        if key in by_key:
            if key not in duplicate_keys:
                diagnostics.append(
                    _diagnostic(
                        "DuplicateAssociation",
                        key,
                        "operation evidence contains a duplicate exact ProtocolSurfaceKey",
                    )
                )
                duplicate_keys.add(key)
            continue

        manifest_entry = manifest_entries.get(key)
        if manifest_entry is None:
            same_identity = [
                candidate
                for candidate in expected
                if candidate[1:] == key[1:] and candidate[0] != key[0]
            ]
            if len(same_identity) == 1:
                category_replacements.add(same_identity[0])
                diagnostics.append(
                    _diagnostic(
                        "WrongAssociationCategory",
                        key,
                        "operation evidence uses the wrong category for an otherwise exact identity",
                    )
                )
            else:
                diagnostics.append(
                    _diagnostic(
                        "StaleAssociation",
                        key,
                        "operation evidence identity is absent from the pinned manifest",
                    )
                )
            continue

        if manifest_entry["category"] not in {"client_request", "server_request"}:
            diagnostics.append(
                _diagnostic(
                    "ContractOnNonRequest",
                    key,
                    "operation evidence is attached to a non-request surface",
                )
            )
            continue
        if manifest_entry["stability"] != "stable":
            diagnostics.append(
                _diagnostic(
                    "ExperimentalAssociationCountedAsStable",
                    key,
                    "experimental-only operation evidence cannot count toward stable coverage",
                )
            )
            continue
        by_key[key] = contract

    for key in sorted(expected):
        if key not in by_key and key not in category_replacements:
            diagnostics.append(
                _diagnostic(
                    "MissingAssociation",
                    key,
                    "stable request has no authoritative operation association",
                )
            )

    registry = registry_by_key(registry_entries) if registry_entries is not None else {}
    for key, contract in sorted(by_key.items()):
        manifest_entry = expected[key]
        evidence_key = contract.get("association_evidence_key")
        primary_identity_duplicate = False
        if not isinstance(evidence_key, str) or not evidence_key:
            diagnostics.append(
                _diagnostic(
                    "MissingAssociation",
                    key,
                    "authoritative operation association has no stable evidence key",
                )
            )
        else:
            primary_identity = (
                str(contract.get("association_evidence_kind")),
                evidence_key,
            )
            previous_owner = evidence_identity_owners.setdefault(
                primary_identity, key
            )
            if previous_owner != key:
                primary_identity_duplicate = True
                diagnostics.append(
                    _diagnostic(
                        "DuplicateAssociation",
                        key,
                        (
                            "authoritative evidence identity is already owned "
                            f"by {previous_owner}"
                        ),
                    )
                )
        parameter_type = contract.get("parameter_type_identity")
        expected_parameter = manifest_entry["params"]["type"] or "Unit"
        parameter_schema = contract.get("parameter_schema")
        parameter_schema_path = (
            Path(parameter_schema)
            if isinstance(parameter_schema, str) and parameter_schema
            else None
        )
        safe_parameter_schema_path = (
            parameter_schema_path is not None
            and not parameter_schema_path.is_absolute()
            and ".." not in parameter_schema_path.parts
            and bool(parameter_schema_path.parts)
            and parameter_schema_path.parts[0] == "stable"
            and parameter_schema_path.suffix == ".json"
        )
        parameter_schema_document: Any = None
        if safe_parameter_schema_path:
            try:
                parameter_schema_document = json.loads(
                    (
                        DEFAULT_PINNED_SCHEMA_ROOT / parameter_schema_path
                    ).read_text(encoding="utf-8")
                )
            except (
                OSError,
                UnicodeError,
                json.JSONDecodeError,
            ):
                parameter_schema_document = None
        parameter_schema_consistent = (
            parameter_schema is None
            if expected_parameter == "Unit"
            else (
                safe_parameter_schema_path
                and isinstance(parameter_schema_document, dict)
                and parameter_schema_document.get("title")
                == expected_parameter
                and parameter_schema_path.stem == expected_parameter
            )
        )
        registry_entry = registry.get(key)
        if registry_entries is not None and registry_entry is None:
            wrong_category = [
                candidate
                for candidate in registry
                if candidate[1:] == key[1:] and candidate[0] != key[0]
            ]
            diagnostics.append(
                _diagnostic(
                    (
                        "WrongAssociationCategory"
                        if len(wrong_category) == 1
                        else "StaleAssociation"
                    ),
                    key,
                    (
                        "canonical registry associates this exact identity with the wrong category"
                        if len(wrong_category) == 1
                        else "authoritative association does not resolve to one canonical registry row"
                    ),
                )
            )
            continue
        registry_parameter = (
            registry_entry.get("parameter_type_identity")
            if registry_entry is not None
            else parameter_type
        )
        parameter_identity_consistent = (
            parameter_type == expected_parameter
            and registry_parameter == parameter_type
            and parameter_schema_consistent
        )
        if not parameter_identity_consistent:
            diagnostics.append(
                _diagnostic(
                    "WrongParameterType",
                    key,
                    "operation parameter identity disagrees with schema or registry",
                )
            )

        result_type = contract.get("result_type_identity")
        named_result_type = contract.get("result_schema_type_identity")
        result_schema = contract.get("result_schema")
        result_kind = contract.get("result_contract_kind")
        local_result_consistent = True
        wrong_result_reported = False
        if result_kind == "Unit" and result_type not in {"Unit", "()"}:
            diagnostics.append(
                _diagnostic(
                    "UnitWithNonUnitResultType",
                    key,
                    "Unit result contract lacks an explicit unit result identity",
                )
            )
            local_result_consistent = False
        elif result_kind == "Concrete" and not result_type:
            diagnostics.append(
                _diagnostic(
                    "ConcreteWithoutResultType",
                    key,
                    "Concrete result contract lacks a result type identity",
                )
            )
            local_result_consistent = False
        elif result_kind not in {
            "Concrete",
            "Unit",
            "Nullable",
            "ProtocolSpecial",
        }:
            diagnostics.append(
                _diagnostic(
                    "MissingAssociation",
                    key,
                    "stable request has an unresolved or inapplicable result contract",
                )
            )
            local_result_consistent = False
        elif result_kind in {"Nullable", "ProtocolSpecial"} and not result_type:
            diagnostics.append(
                _diagnostic(
                    "WrongResultType",
                    key,
                    "result contract requires a non-empty result type identity",
                )
            )
            local_result_consistent = False

        result_schema_path = (
            Path(result_schema)
            if isinstance(result_schema, str) and result_schema
            else None
        )
        safe_result_schema_path = (
            result_schema_path is not None
            and not result_schema_path.is_absolute()
            and ".." not in result_schema_path.parts
            and bool(result_schema_path.parts)
            and result_schema_path.parts[0] == "stable"
            and result_schema_path.suffix == ".json"
        )
        result_schema_document: Any = None
        if safe_result_schema_path:
            try:
                result_schema_document = json.loads(
                    (
                        DEFAULT_PINNED_SCHEMA_ROOT / result_schema_path
                    ).read_text(encoding="utf-8")
                )
            except (
                OSError,
                UnicodeError,
                json.JSONDecodeError,
            ):
                result_schema_document = None
        result_schema_identity_consistent = (
            isinstance(named_result_type, str)
            and bool(named_result_type)
            and safe_result_schema_path
            and result_schema_path.stem == named_result_type
            and isinstance(result_schema_document, dict)
            and result_schema_document.get("title") == named_result_type
            and (
                result_kind == "Unit"
                or result_type == named_result_type
            )
        )
        if local_result_consistent and not result_schema_identity_consistent:
            diagnostics.append(
                _diagnostic(
                    "WrongResultType",
                    key,
                    (
                        "named result identity, schema root, and canonical "
                        "result contract disagree"
                    ),
                )
            )
            wrong_result_reported = True
        if (
            local_result_consistent
            and isinstance(result_schema_document, dict)
            and schema_result_contract_kind(result_schema_document)
            != result_kind
            and not wrong_result_reported
        ):
            diagnostics.append(
                _diagnostic(
                    "WrongResultType",
                    key,
                    (
                        "declared result contract kind disagrees with "
                        "independently loaded schema semantics"
                    ),
                )
            )
            wrong_result_reported = True

        if registry_entry is not None and local_result_consistent:
            if (
                registry_entry.get("result_type_identity") != result_type
                or registry_entry.get("result_contract_kind") != result_kind
            ) and not wrong_result_reported:
                diagnostics.append(
                    _diagnostic(
                        "WrongResultType",
                        key,
                        "operation result identity disagrees with canonical registry",
                    )
                )
                wrong_result_reported = True

        expected_evidence = (
            "VendoredRust" if key[0] == "client_request" else "VendoredSchemaPair"
        )
        evidence_kind = contract.get("association_evidence_kind")
        registry_evidence = (
            registry_entry.get("association_evidence_kind")
            if registry_entry is not None
            else evidence_kind
        )
        conflict = evidence_kind != expected_evidence or registry_evidence != evidence_kind
        if key[0] == "client_request":
            rust = contract.get("rust")
            authority = rust_client_authority.get(key[3])
            if authority is None or authority.experimental:
                conflict = True
            else:
                expected_rust_record = {
                    "variant": authority.variant,
                    "wire_method": authority.method,
                    "parameter_type": authority.parameter_type,
                    "result_type": authority.result_type,
                }
                expected_rust_key = (
                    f"{VENDORED_RUST_COMMON_PATH}"
                    f"#client_request_definitions!/{authority.variant}"
                )
                authoritative_parameter = (
                    "Unit"
                    if re.search(
                        r"\bOption\s*<\s*\(\s*\)\s*>",
                        authority.parameter_type,
                    )
                    else authority.parameter_type.rsplit("::", 1)[-1]
                )
                authoritative_result = authority.result_type.rsplit(
                    "::", 1
                )[-1]
                if (
                    rust != expected_rust_record
                    or (
                        evidence_key != expected_rust_key
                        and not primary_identity_duplicate
                    )
                    or (
                        parameter_identity_consistent
                        and authoritative_parameter != parameter_type
                    )
                    or authoritative_result != named_result_type
                ):
                    conflict = True
                expected_result_schema = (
                    "stable/"
                    + authority.result_type.replace("::", "/")
                    + ".json"
                )
                if (
                    result_schema != expected_result_schema
                    and not wrong_result_reported
                ):
                    diagnostics.append(
                        _diagnostic(
                            "WrongResultType",
                            key,
                            (
                                "result schema root disagrees with the "
                                "vendored Rust successful result type"
                            ),
                        )
                    )
                    wrong_result_reported = True
        else:
            schema_pair = contract.get("schema_pair")
            manifest_sources = manifest_entry.get("sources", [])
            server_union_sources = [
                source
                for source in manifest_sources
                if isinstance(source, dict)
                and source.get("file") == "ServerRequest.json"
                and isinstance(source.get("pointer"), str)
                and re.fullmatch(r"/oneOf/[0-9]+", source["pointer"])
            ]
            expected_branch = (
                int(server_union_sources[0]["pointer"].rsplit("/", 1)[-1])
                if len(server_union_sources) == 1
                else None
            )
            if not isinstance(schema_pair, dict):
                conflict = True
            else:
                request_union = schema_pair.get("request_union")
                branch = schema_pair.get("branch")
                parameter_schema = schema_pair.get("parameter_schema")
                response_schema = schema_pair.get("response_schema")
                valid_branch = isinstance(branch, int) and not isinstance(
                    branch, bool
                ) and branch >= 0
                expected_schema_key = (
                    f"{request_union}#/oneOf/{branch}+{response_schema}"
                    if isinstance(request_union, str)
                    and valid_branch
                    and isinstance(response_schema, str)
                    else None
                )
                if (
                    request_union != "stable/ServerRequest.json"
                    or not valid_branch
                    or branch != expected_branch
                    or parameter_schema != contract.get("parameter_schema")
                    or response_schema != result_schema
                    or (
                        evidence_key != expected_schema_key
                        and not primary_identity_duplicate
                    )
                ):
                    conflict = True
                if isinstance(request_union, str) and valid_branch:
                    branch_identity = (request_union, branch)
                    previous_owner = schema_pair_branch_owners.setdefault(
                        branch_identity, key
                    )
                    if previous_owner != key:
                        diagnostics.append(
                            _diagnostic(
                                "DuplicateAssociation",
                                key,
                                (
                                    "server-request schema branch is already "
                                    f"owned by {previous_owner}"
                                ),
                            )
                        )
        cross_checks = contract.get("cross_check_evidence", [])
        if not isinstance(cross_checks, list):
            raise SurfaceError("operation cross_check_evidence is not an array")
        seen_cross_checks: set[str] = set()
        for cross_check in cross_checks:
            if not isinstance(cross_check, dict):
                raise SurfaceError("operation cross-check evidence contains a non-object")
            kind = cross_check.get("kind")
            cross_key = cross_check.get("key")
            if (
                not isinstance(kind, str)
                or kind in seen_cross_checks
                or not isinstance(cross_key, str)
                or not cross_key
            ):
                conflict = True
            if isinstance(kind, str):
                seen_cross_checks.add(kind)
            if (
                isinstance(kind, str)
                and isinstance(cross_key, str)
                and cross_key
            ):
                cross_identity = (kind, cross_key)
                previous_owner = evidence_identity_owners.setdefault(
                    cross_identity, key
                )
                if previous_owner != key:
                    diagnostics.append(
                        _diagnostic(
                            "DuplicateAssociation",
                            key,
                            (
                                "cross-check evidence identity is already "
                                f"owned by {previous_owner}"
                            ),
                        )
                    )
            cross_parameter = cross_check.get("parameter_type_identity", parameter_type)
            cross_result = cross_check.get("result_type_identity", result_type)
            cross_named_result = cross_check.get(
                "result_schema_type_identity", named_result_type
            )
            cross_kind = cross_check.get("result_contract_kind", result_kind)
            if (
                cross_parameter != parameter_type
                or cross_result != result_type
                or cross_named_result != named_result_type
                or cross_kind != result_kind
                or cross_check.get("agrees", True) is not True
            ):
                conflict = True
            if kind == "VendoredRust" and key[0] == "server_request":
                authority = rust_server_authority.get(key[3])
                cross_duplicate = (
                    isinstance(cross_key, str)
                    and evidence_identity_owners.get((kind, cross_key)) != key
                )
                if authority is None or authority.experimental:
                    conflict = True
                else:
                    authoritative_parameter = (
                        "Unit"
                        if re.search(
                            r"\bOption\s*<\s*\(\s*\)\s*>",
                            authority.parameter_type,
                        )
                        else authority.parameter_type.rsplit("::", 1)[-1]
                    )
                    authoritative_result = authority.result_type.rsplit(
                        "::", 1
                    )[-1]
                    expected_cross_key = (
                        f"{VENDORED_RUST_COMMON_PATH}"
                        f"#server_request_definitions!/{authority.variant}"
                    )
                    expected_cross_check = {
                        "kind": "VendoredRust",
                        "key": cross_key,
                        "parameter_type_identity": authoritative_parameter,
                        "result_type_identity": (
                            "Unit"
                            if result_kind == "Unit"
                            else authoritative_result
                        ),
                        "result_schema_type_identity": authoritative_result,
                        "result_contract_kind": result_kind,
                        "agrees": True,
                    }
                    if (
                        cross_check != expected_cross_check
                        or (
                            cross_key != expected_cross_key
                            and not cross_duplicate
                        )
                    ):
                        conflict = True
        if conflict:
            diagnostics.append(
                _diagnostic(
                    "ConflictingAssociationEvidence",
                    key,
                    "authoritative and cross-check operation evidence conflict",
                )
            )

        if registry_entry is not None and (
            registry_entry.get("association_evidence_key")
            != evidence_key
        ):
            diagnostics.append(
                _diagnostic(
                    "ConflictingAssociationEvidence",
                    key,
                    "registry evidence key disagrees with authoritative association",
                )
            )

    return diagnostics


def _frozen_public_root_slice(
    category: str, name: str
) -> tuple[str, str] | None:
    if category == "client_notification" and name == "initialized":
        return "A1.0", "Common"
    if category == "server_notification" and name == "error":
        return "A1.0", "Common"
    if name == "configWarning":
        return "A1.2", SLICE_TYPED_MODULES["A1.2"]
    if name in {
        "guardianWarning",
        "permissionProfile/list",
        "thread/approveGuardianDeniedAction",
    }:
        return "A1.3", SLICE_TYPED_MODULES["A1.3"]

    prefix = name.split("/", 1)[0]
    if prefix in {"thread", "turn"}:
        return "A1.1", SLICE_TYPED_MODULES["A1.1"]
    if prefix == "item" and category == "server_notification":
        if any(
            marker in name
            for marker in (
                "autoApprovalReview",
                "requestApproval",
                "guardian",
                "permissions",
            )
        ):
            return "A1.3", SLICE_TYPED_MODULES["A1.3"]
        return "A1.1", SLICE_TYPED_MODULES["A1.1"]
    if prefix in {
        "account",
        "model",
        "modelProvider",
        "config",
        "configRequirements",
        "experimentalFeature",
    }:
        return "A1.2", SLICE_TYPED_MODULES["A1.2"]
    if prefix in {
        "command",
        "fs",
        "review",
        "fuzzyFileSearch",
        "execCommandApproval",
        "applyPatchApproval",
    }:
        return "A1.3", SLICE_TYPED_MODULES["A1.3"]
    if prefix == "item" and category == "server_request":
        if any(
            marker in name
            for marker in ("commandExecution", "fileChange", "permissions")
        ):
            return "A1.3", SLICE_TYPED_MODULES["A1.3"]
    if name == "initialize":
        return "A1.0", "Common"
    if name in A1_4_PUBLIC_ROOTS.get(category, frozenset()):
        return "A1.4", SLICE_TYPED_MODULES["A1.4"]
    return None


def _assignment_profile(
    entry: dict[str, Any],
    classification: object,
    slice_name: object,
) -> tuple[str | None, bool]:
    """Return the only module allowed by an assignment's semantic profile.

    The boolean indicates whether the category, stability, classification, and
    slice form one of the frozen A1 assignment profiles. Nested-union
    classifications are additionally proven from reachability below.
    """

    category = entry["category"]
    stability = entry["stability"]
    if stability == "experimental_only":
        valid = (
            classification == "ExperimentalInventoryOnly"
            and slice_name == "InventoryOnly"
        )
        return ("ExperimentalInventory" if valid else None), valid
    if category == "item_discriminator":
        valid = classification == "StableItemRoot" and slice_name == "A1.1"
        return ("ThreadsTurnsSessions" if valid else None), valid
    if category != "tagged_union_discriminator":
        expected = _frozen_public_root_slice(category, entry["name"])
        valid = (
            classification == "StablePublicRoot"
            and expected is not None
            and slice_name == expected[0]
        )
        return (expected[1] if valid else None), valid

    if classification not in NESTED_ASSIGNMENT_CLASSIFICATIONS:
        return None, False
    if classification == "CrossCuttingA1_0Exception":
        valid = entry["domain"] == "CodexErrorInfo" and slice_name == "A1.0"
        return ("Common" if valid else None), valid
    if classification == "StableUnreachableInventory":
        valid = slice_name == "InventoryOnly"
        return ("StableUnreachableInventory" if valid else None), valid
    if classification == "SharedCommon":
        valid = isinstance(slice_name, str) and slice_name in A1_SLICE_ORDER
        return ("Common" if valid else None), valid
    valid = (
        classification in {"RootOwnedNestedUnion", "SharedWithinSlice"}
        and isinstance(slice_name, str)
        and slice_name in SLICE_TYPED_MODULES
        and slice_name != "A1.0"
    )
    return (SLICE_TYPED_MODULES.get(slice_name) if valid else None), valid


def _synthetic_reachability_key(root_id: str) -> tuple[str, str, str, str]:
    return (
        "tagged_union_discriminator",
        "ReachabilityRoot",
        "root_id",
        root_id,
    )


def _canonical_surface_key_object(
    key: tuple[str, str, str, str],
) -> dict[str, str]:
    return {
        "category": key[0],
        "domain": key[1],
        "discriminator_field": key[2],
        "name": key[3],
    }


def _expected_reachability_roots(
    manifest_entries: dict[tuple[str, str, str, str], dict[str, Any]],
    assignments: dict[tuple[str, str, str, str], dict[str, Any]],
) -> dict[str, dict[str, Any]]:
    roots: dict[str, dict[str, Any]] = {}
    for key, entry in sorted(manifest_entries.items()):
        if (
            entry["stability"] != "stable"
            or entry["category"]
            not in {
                "client_notification",
                "client_request",
                "server_notification",
                "server_request",
            }
        ):
            continue
        root_id = ":".join(key)
        roots[root_id] = {
            "role": "method",
            "root_id": root_id,
            "slice": assignments[key]["slice"],
            "surface_key": _canonical_surface_key_object(key),
        }
    for item_domain in ("ResponseItem", "ThreadItem"):
        root_id = f"item_union:{item_domain}"
        roots[root_id] = {
            "role": "item_union",
            "root_id": root_id,
            "slice": "A1.1",
        }
    return roots


def _root_record_matches(
    actual: dict[str, Any], expected: dict[str, Any]
) -> bool:
    if (
        actual.get("role") != expected["role"]
        or actual.get("root_id") != expected["root_id"]
        or actual.get("slice") != expected["slice"]
    ):
        return False
    expected_key = expected.get("surface_key")
    actual_key = actual.get("surface_key")
    if expected_key is None:
        return actual_key is None
    if actual_key is None:
        return False
    try:
        return surface_key({"surface_key": actual_key}) == surface_key(
            {"surface_key": expected_key}
        )
    except SurfaceError:
        return False


def _reachability_membership_sha256(
    records: dict[tuple[str, str, str, str], dict[str, Any]],
) -> str:
    membership = [
        {
            "surface_key": _canonical_surface_key_object(key),
            "reaching_root_ids": sorted(
                root["root_id"] for root in record["reaching_roots"]
            ),
        }
        for key, record in sorted(records.items())
    ]
    return hashlib.sha256(canonical_json(membership).encode("utf-8")).hexdigest()


def assignment_reachability_diagnostics(
    manifest: dict[str, Any],
    assignments_document: dict[str, Any],
    reachability_document: dict[str, Any],
) -> list[dict[str, Any]]:
    """Validate frozen module/slice and nested reachability evidence.

    Assignment rows join bidirectionally to the exact pinned manifest.
    Every stable tagged-union row then joins to exactly one reachability row,
    whose root set is checked against the frozen root catalog. Diagnostics are
    deliberately code-stable so mutation tests can reject unrelated failures.
    """

    manifest_entries = {
        surface_key(entry): entry for entry in manifest["entries"]
    }
    assignment_records = evidence_records(
        assignments_document,
        ("assignments", "entries", "identities"),
        "module/slice assignment evidence",
    )
    assignments: dict[tuple[str, str, str, str], dict[str, Any]] = {}
    duplicate_assignments: set[tuple[str, str, str, str]] = set()
    diagnostics: list[dict[str, Any]] = []

    for record in assignment_records:
        reject_local_dispositions(record, "module/slice assignment evidence")
        key = surface_key(record)
        if key in assignments:
            if key not in duplicate_assignments:
                diagnostics.append(
                    _diagnostic(
                        "DuplicateModuleSliceAssignment",
                        key,
                        "module/slice evidence contains a duplicate exact ProtocolSurfaceKey",
                    )
                )
                duplicate_assignments.add(key)
            continue
        if key not in manifest_entries:
            diagnostics.append(
                _diagnostic(
                    "StaleModuleSliceAssignment",
                    key,
                    "module/slice evidence identity is absent from the pinned manifest",
                )
            )
            continue
        assignments[key] = record

    for key in sorted(set(manifest_entries) - set(assignments)):
        diagnostics.append(
            _diagnostic(
                "MissingModuleSliceAssignment",
                key,
                "pinned surface identity has no exact module/slice assignment",
            )
        )

    if diagnostics:
        return diagnostics

    for key, record in sorted(assignments.items()):
        entry = manifest_entries[key]
        if record.get("stability") != entry["stability"]:
            diagnostics.append(
                _diagnostic(
                    "AssignmentStabilityMismatch",
                    key,
                    "module/slice evidence stability disagrees with the pinned manifest",
                )
            )
            continue
        slice_name = record.get("slice")
        if (
            slice_name not in CPP_A1_SLICES
            or record.get("a1_slice", slice_name) != slice_name
        ):
            diagnostics.append(
                _diagnostic(
                    "AssignmentSliceMismatch",
                    key,
                    "module/slice evidence has an invalid or internally inconsistent A1 slice",
                )
            )
            continue
        if (
            entry["stability"] == "stable"
            and entry["category"] != "tagged_union_discriminator"
            and entry["category"] != "item_discriminator"
            and record.get("classification") == "StablePublicRoot"
        ):
            frozen_root = _frozen_public_root_slice(
                entry["category"], entry["name"]
            )
            if frozen_root is None or slice_name != frozen_root[0]:
                diagnostics.append(
                    _diagnostic(
                        "AssignmentDomainSliceMismatch",
                        key,
                        "stable public root disagrees with its reviewed fixed domain slice",
                    )
                )
                continue
        expected_module, profile_valid = _assignment_profile(
            entry, record.get("classification"), slice_name
        )
        if not profile_valid:
            diagnostics.append(
                _diagnostic(
                    "InvalidAssignmentClassification",
                    key,
                    "category, stability, classification, and A1 slice are inconsistent",
                )
            )
            continue
        if record.get("module") != expected_module:
            diagnostics.append(
                _diagnostic(
                    "AssignmentModuleMismatch",
                    key,
                    "typed module disagrees with the frozen classification/slice profile",
                )
            )

    # Assignment failures are returned before reachability is evaluated so one
    # mutated authority row cannot cause a cascade of unrelated join failures.
    if diagnostics:
        return diagnostics

    expected_roots = _expected_reachability_roots(manifest_entries, assignments)
    roots = evidence_records(
        reachability_document, ("roots",), "nested-reachability root evidence"
    )
    roots_by_id: dict[str, dict[str, Any]] = {}
    duplicate_roots: set[str] = set()
    for root in roots:
        reject_local_dispositions(root, "nested-reachability root evidence")
        root_id = root.get("root_id")
        if not isinstance(root_id, str) or not root_id:
            raise SurfaceError("nested-reachability root has no stable root_id")
        key = (
            surface_key(root)
            if root.get("surface_key") is not None
            else _synthetic_reachability_key(root_id)
        )
        if root_id in roots_by_id:
            if root_id not in duplicate_roots:
                diagnostics.append(
                    _diagnostic(
                        "DuplicateReachabilityRoot",
                        key,
                        "nested-reachability evidence contains a duplicate root_id",
                    )
                )
                duplicate_roots.add(root_id)
            continue
        if root_id not in expected_roots:
            diagnostics.append(
                _diagnostic(
                    "StaleReachabilityRoot",
                    key,
                    "nested-reachability root is not a stable method or fixed item root",
                )
            )
            continue
        roots_by_id[root_id] = root

    for root_id in sorted(set(expected_roots) - set(roots_by_id)):
        expected = expected_roots[root_id]
        key = (
            surface_key(expected)
            if expected.get("surface_key") is not None
            else _synthetic_reachability_key(root_id)
        )
        diagnostics.append(
            _diagnostic(
                "MissingReachabilityRoot",
                key,
                "stable method or fixed item root is absent from reachability evidence",
            )
        )
    for root_id, root in sorted(roots_by_id.items()):
        if not _root_record_matches(root, expected_roots[root_id]):
            expected = expected_roots[root_id]
            key = (
                surface_key(expected)
                if expected.get("surface_key") is not None
                else _synthetic_reachability_key(root_id)
            )
            diagnostics.append(
                _diagnostic(
                    "ReachabilityRootSetMismatch",
                    key,
                    "reachability root identity, role, or assigned slice is inconsistent",
                )
            )

    if diagnostics:
        return diagnostics

    reachability_records = evidence_records(
        reachability_document,
        (
            "reachability",
            "entries",
            "nested_identities",
            "nested_union_records",
            "records",
        ),
        "nested-reachability evidence",
    )
    expected_nested = {
        key
        for key, entry in manifest_entries.items()
        if entry["stability"] == "stable"
        and entry["category"] == "tagged_union_discriminator"
    }
    reachability_by_key: dict[
        tuple[str, str, str, str], dict[str, Any]
    ] = {}
    duplicate_reachability: set[tuple[str, str, str, str]] = set()
    for record in reachability_records:
        reject_local_dispositions(record, "nested-reachability evidence")
        key = surface_key(record)
        if key in reachability_by_key:
            if key not in duplicate_reachability:
                diagnostics.append(
                    _diagnostic(
                        "DuplicateReachabilityRecord",
                        key,
                        "nested-union identity has more than one reachability record",
                    )
                )
                duplicate_reachability.add(key)
            continue
        if key not in expected_nested:
            diagnostics.append(
                _diagnostic(
                    "StaleReachabilityRecord",
                    key,
                    "reachability record is not a stable tagged-union identity",
                )
            )
            continue
        reachability_by_key[key] = record

    for key in sorted(expected_nested - set(reachability_by_key)):
        diagnostics.append(
            _diagnostic(
                "MissingReachabilityRecord",
                key,
                "stable tagged-union assignment has no reachability record",
            )
        )

    if diagnostics:
        return diagnostics

    for key, record in sorted(reachability_by_key.items()):
        assignment = assignments[key]
        assigned_slice = record.get("assigned_slice")
        if assigned_slice != assignment["slice"]:
            diagnostics.append(
                _diagnostic(
                    "ReachabilityAssignedSliceMismatch",
                    key,
                    "reachability assigned slice disagrees with its exact assignment row",
                )
            )
            continue
        if record.get("classification") != assignment.get("classification"):
            diagnostics.append(
                _diagnostic(
                    "ReachabilityClassificationMismatch",
                    key,
                    "reachability classification disagrees with its exact assignment row",
                )
            )
            continue

        reaching_roots = record.get("reaching_roots")
        root_set_valid = isinstance(reaching_roots, list) and all(
            isinstance(root, dict) for root in reaching_roots
        )
        root_ids: list[str] = []
        if root_set_valid:
            root_ids = [root.get("root_id") for root in reaching_roots]
            root_set_valid = (
                all(isinstance(root_id, str) and root_id for root_id in root_ids)
                and len(set(root_ids)) == len(root_ids)
                and all(root_id in expected_roots for root_id in root_ids)
                and all(
                    _root_record_matches(root, expected_roots[root_id])
                    for root, root_id in zip(reaching_roots, root_ids)
                )
            )
        root_slices = sorted(
            {
                expected_roots[root_id]["slice"]
                for root_id in root_ids
                if root_id in expected_roots
            },
            key=lambda value: A1_SLICE_ORDER[value],
        )
        if (
            not root_set_valid
            or record.get("reaching_root_count") != len(root_ids)
            or record.get("reaching_slices") != root_slices
        ):
            diagnostics.append(
                _diagnostic(
                    "ReachabilityRootSetMismatch",
                    key,
                    "reaching roots, count, and slice set are not internally exact",
                )
            )
            continue

        classification = assignment["classification"]
        if classification == "StableUnreachableInventory" and root_ids:
            diagnostics.append(
                _diagnostic(
                    "FalseStableUnreachable",
                    key,
                    "identity classified as stable-unreachable has stable reaching roots",
                )
            )
            continue
        if not root_ids:
            expected_classification = "StableUnreachableInventory"
        elif manifest_entries[key]["domain"] == "CodexErrorInfo":
            expected_classification = "CrossCuttingA1_0Exception"
        elif len(root_ids) == 1:
            expected_classification = "RootOwnedNestedUnion"
        elif len(root_slices) == 1:
            expected_classification = "SharedWithinSlice"
        else:
            expected_classification = "SharedCommon"
        if classification != expected_classification:
            diagnostics.append(
                _diagnostic(
                    (
                        "FalseStableUnreachable"
                        if classification == "StableUnreachableInventory"
                        else "ReachabilityClassificationMismatch"
                    ),
                    key,
                    "classification is not mechanically implied by its reaching root set",
                )
            )
            continue

        if root_slices:
            earliest_slice = root_slices[0]
            if assigned_slice != earliest_slice:
                diagnostics.append(
                    _diagnostic(
                        "ReachabilityNotEarliestSlice",
                        key,
                        "nested identity is not assigned to its earliest stable reaching-root slice",
                    )
                )

    if (
        not diagnostics
        and _reachability_membership_sha256(reachability_by_key)
        != PINNED_REACHABILITY_MEMBERSHIP_SHA256
    ):
        diagnostics.append(
            _diagnostic(
                "ReachabilityRootSetMismatch",
                (
                    "tagged_union_discriminator",
                    "NestedReachability",
                    "root-set",
                    "pinned-membership",
                ),
                "schema-derived nested reachability membership differs from its reviewed pin",
            )
        )

    return diagnostics


def assignment_by_key(
    manifest: dict[str, Any], assignments_document: dict[str, Any]
) -> dict[tuple[str, str, str, str], dict[str, Any]]:
    records = evidence_records(
        assignments_document,
        ("assignments", "entries", "identities"),
        "module/slice assignment evidence",
    )
    result: dict[tuple[str, str, str, str], dict[str, Any]] = {}
    manifest_keys = {surface_key(entry) for entry in manifest["entries"]}
    for record in records:
        reject_local_dispositions(record, "module/slice assignment evidence")
        key = surface_key(record)
        if key in result:
            raise SurfaceError(f"duplicate module/slice assignment: {key}")
        if key not in manifest_keys:
            raise SurfaceError(f"stale module/slice assignment: {key}")
        module = record.get("module")
        slice_name = record.get("slice", record.get("a1_slice"))
        if not isinstance(module, str) or not module:
            raise SurfaceError(f"module/slice assignment has no typed module: {key}")
        if slice_name not in CPP_A1_SLICES:
            raise SurfaceError(f"module/slice assignment has an invalid A1 slice: {key}")
        manifest_entry = next(
            entry for entry in manifest["entries"] if surface_key(entry) == key
        )
        if (
            manifest_entry["stability"] == "experimental_only"
            and slice_name != "InventoryOnly"
        ):
            raise SurfaceError(
                f"experimental-only surface is not assigned to inventory-only: {key}"
            )
        normalized = dict(record)
        normalized["slice"] = slice_name
        result[key] = normalized
    missing = sorted(manifest_keys - result.keys())
    if missing:
        raise SurfaceError(f"module/slice assignment lacks exact surface key: {missing[0]}")
    return result


def fixture_evidence_by_key(
    fixture_document: dict[str, Any],
) -> dict[tuple[str, str, str, str], list[dict[str, Any]]]:
    fixtures = evidence_records(
        fixture_document, ("fixtures",), "fixture-coverage evidence"
    )
    result: dict[tuple[str, str, str, str], list[dict[str, Any]]] = {}
    for fixture in fixtures:
        result.setdefault(surface_key(fixture), []).append(fixture)
    return result


def completeness_evidence(
    entry: dict[str, Any],
    fixtures: Sequence[dict[str, Any]],
    assignment: dict[str, Any],
) -> dict[str, bool]:
    evidence = {field: False for field in COMPLETENESS_EVIDENCE_FIELDS}
    if "completeness_evidence" in assignment:
        raise SurfaceError(
            "module/slice assignment evidence cannot carry schema-completeness or local runtime dispositions"
        )

    if fixtures:
        for fixture in fixtures:
            reject_local_dispositions(fixture, "fixture-coverage evidence")
            fixture_evidence = fixture.get("completeness_evidence")
            if fixture_evidence is None:
                continue
            if not isinstance(fixture_evidence, dict):
                raise SurfaceError("fixture completeness_evidence is not an object")
            unknown = set(fixture_evidence) - SCHEMA_FIXTURE_COMPLETENESS_FIELDS
            if unknown:
                raise SurfaceError(
                    "fixture completeness evidence contains unsupported or local "
                    f"disposition fields: {sorted(unknown)}"
                )
            for field, value in fixture_evidence.items():
                if not isinstance(value, bool):
                    raise SurfaceError(
                        f"fixture completeness evidence {field} is not boolean"
                    )
                evidence[field] = evidence[field] or value

        evidence["positive_fixture_coverage"] = True
        evidence["fixture_current"] = all(
            isinstance(
                fixture.get("fixture_sha256", fixture.get("file_sha256")), str
            )
            and bool(fixture.get("fixture_sha256", fixture.get("file_sha256")))
            for fixture in fixtures
        )
        evidence["independently_schema_validated"] = all(
            fixture.get("validation", {}).get("independent") is True
            for fixture in fixtures
        )
        if entry["category"] in {"client_request", "server_request"}:
            evidence["authoritative_root_association"] = True
    return evidence


def derived_schema_status(
    typed_status: str, slice_name: str, evidence: dict[str, bool]
) -> str:
    if slice_name == "InventoryOnly" or typed_status == "NotApplicable":
        return "NotApplicable"
    if typed_status != "Implemented":
        return "NotImplemented"
    return "Complete" if all(evidence.values()) else "Partial"


def operation_contract_by_key(
    manifest: dict[str, Any], contracts_document: dict[str, Any]
) -> dict[tuple[str, str, str, str], dict[str, Any]]:
    diagnostics = association_diagnostics(manifest, contracts_document)
    if diagnostics:
        first = diagnostics[0]
        raise SurfaceError(
            f"{first['code']}: {first['message']} at {first['surface_key']}"
        )
    return {
        surface_key(contract): contract
        for contract in evidence_records(
            contracts_document, ("contracts",), "operation-contract evidence"
        )
    }


def registry_statuses(
    entry: dict[str, Any],
    contract: dict[str, Any] | None,
    assignment: dict[str, Any],
    fixtures: Sequence[dict[str, Any]],
    operation_production_coverage: dict[str, Any] | None = None,
    notification_production_coverage: (
        dict[tuple[str, str, str, str], dict[str, Any]] | None
    ) = None,
) -> tuple[str, ...]:
    identity = (
        entry["category"],
        entry["domain"],
        entry["discriminator_field"],
        entry["name"],
    )
    target = RUNTIME_TARGETS.get(identity)
    category = entry["category"]
    is_operation = category in {"client_request", "server_request"}
    is_existing_frontend = (category, entry["name"]) in EXISTING_FRONTEND_OPERATIONS
    backend_implemented = (
        category,
        entry["name"],
    ) in BACKEND_IMPLEMENTED_IDENTITIES and (
        category != "item_discriminator" or entry["domain"] == "ThreadItem"
    )
    state_implemented = (
        category,
        entry["name"],
    ) in CANONICAL_STATE_IMPLEMENTED_IDENTITIES and (
        category != "item_discriminator" or entry["domain"] == "ThreadItem"
    )

    if target is not None:
        runtime_disposition = "RuntimeDisposition::Typed"
        typed_status = "TypedImplementationStatus::Implemented"
    else:
        runtime_disposition = {
            "client_request": "RuntimeDisposition::Deferred",
            "client_notification": "RuntimeDisposition::RawPreserved",
            "server_notification": "RuntimeDisposition::RawPreserved",
            "server_request": "RuntimeDisposition::RawPreserved",
            "item_discriminator": "RuntimeDisposition::OpaquePreserved",
            "delta_progress_discriminator": "RuntimeDisposition::OpaquePreserved",
            "tagged_union_discriminator": "RuntimeDisposition::OpaquePreserved",
        }[category]
        typed_status = "TypedImplementationStatus::NotImplemented"
    layer_applicable = category in {
        "client_request",
        "server_notification",
        "server_request",
        "item_discriminator",
        "delta_progress_discriminator",
    }
    if identity in {
        ("client_request", "ClientRequest", "method", "initialize"),
        ("client_notification", "ClientNotification", "method", "initialized"),
    }:
        layer_applicable = False
    backend_status = (
        "LayerStatus::Implemented"
        if backend_implemented
        else (
            "LayerStatus::NotImplemented"
            if layer_applicable
            else "LayerStatus::NotApplicable"
        )
    )
    canonical_state_status = (
        "LayerStatus::Implemented"
        if state_implemented
        else (
            "LayerStatus::NotImplemented"
            if layer_applicable
            else "LayerStatus::NotApplicable"
        )
    )

    if is_existing_frontend:
        frontend_exposure = "FrontendExposure::ExistingOperationSubset"
        frontend_security = (
            "FrontendSecurityDecision::ExistingOperationSubsetExpansionUnresolved"
        )
    elif category == "server_request":
        frontend_exposure = "FrontendExposure::GenericUnknownRequest"
        frontend_security = (
            "FrontendSecurityDecision::ExistingGenericContractDedicatedUnresolved"
        )
    elif category == "server_notification":
        if entry["name"] in EXISTING_MODELED_SERVER_NOTIFICATION_METHODS:
            frontend_exposure = "FrontendExposure::ExistingEventSubset"
            frontend_security = (
                "FrontendSecurityDecision::ExistingEventSubsetContract"
            )
        else:
            frontend_exposure = "FrontendExposure::GenericExtension"
            frontend_security = (
                "FrontendSecurityDecision::ExistingRedactedExtensionContract"
            )
    elif category == "item_discriminator":
        if entry["domain"] == "ThreadItem" and backend_implemented:
            frontend_exposure = "FrontendExposure::ExistingEventSubset"
            frontend_security = (
                "FrontendSecurityDecision::ExistingEventSubsetContract"
            )
        elif entry["domain"] == "ThreadItem":
            frontend_exposure = "FrontendExposure::ExistingUnknownItemSubset"
            frontend_security = (
                "FrontendSecurityDecision::ExistingUnknownItemMetadataContract"
            )
        else:
            frontend_exposure = "FrontendExposure::NotExposed"
            frontend_security = "FrontendSecurityDecision::Unresolved"
    elif is_operation:
        frontend_exposure = "FrontendExposure::NotExposed"
        frontend_security = "FrontendSecurityDecision::Unresolved"
    else:
        frontend_exposure = "FrontendExposure::NotApplicable"
        frontend_security = "FrontendSecurityDecision::NotApplicable"

    # Initialization is internal lifecycle negotiation rather than an
    # application command or out-of-process frontend operation.
    if identity == ("client_request", "ClientRequest", "method", "initialize"):
        frontend_exposure = "FrontendExposure::NotApplicable"
        frontend_security = "FrontendSecurityDecision::NotApplicable"

    if contract is None:
        parameter_type_identity = ""
        result_type_identity = ""
        result_contract_kind = "NotApplicable"
        association_evidence_kind = "NotApplicable"
        association_evidence_key = ""
    else:
        parameter_type_identity = contract.get("parameter_type_identity")
        result_type_identity = contract.get("result_type_identity")
        result_contract_kind = contract.get("result_contract_kind")
        association_evidence_kind = contract.get("association_evidence_kind")
        association_evidence_key = contract.get("association_evidence_key")
        if not all(
            isinstance(value, str)
            for value in (
                parameter_type_identity,
                result_type_identity,
                result_contract_kind,
                association_evidence_kind,
                association_evidence_key,
            )
        ):
            raise SurfaceError(f"operation contract has a non-string field: {identity}")
    try:
        result_contract_cpp = CPP_RESULT_CONTRACT_KINDS[result_contract_kind]
        association_evidence_cpp = CPP_ASSOCIATION_EVIDENCE_KINDS[
            association_evidence_kind
        ]
    except KeyError as error:
        raise SurfaceError(f"operation contract has an unsupported enum: {identity}") from error

    module = assignment["module"]
    slice_name = assignment["slice"]
    evidence = completeness_evidence(entry, fixtures, assignment)
    if (
        identity[0] == "tagged_union_discriminator"
        and identity[1] == "CodexErrorInfo"
        and target is not None
    ):
        # A1.0's reviewed nested-union descriptor is the production dispatch
        # target. Focused codec tests exercise every generated positive fixture,
        # all diagnostic classes, raw retention, and every public mapped field.
        # CodexErrorInfo has no intentionally opaque schema field; full raw
        # retention is supplemental rather than a replacement for field mapping.
        evidence["direction_assertions_exercised"] = True
        evidence["runtime_decoder_matches_registry"] = True
        evidence["opaque_fields_declared"] = True
        evidence["no_known_schema_fields_dropped"] = True
    if (
        identity in CONVERSATION_UNION_CODECS
        and identity not in B4_CONVERSATION_UNION_CODECS
        and target is not None
    ):
        # The dependency-closed B2+B3 descriptor set is generated from the
        # same mapping that supplies the canonical registry targets. B4 is
        # excluded here and requires checked production-corpus evidence below.
        evidence["direction_assertions_exercised"] = True
        evidence["runtime_decoder_matches_registry"] = True
        evidence["opaque_fields_declared"] = True
        evidence["no_known_schema_fields_dropped"] = True
    if (
        identity[0] == "client_request"
        and assignment.get("slice") == "A1.1"
        and operation_production_coverage is not None
        and identity in operation_production_coverage.get("operations", {})
    ):
        # The checked operation table is consumed by the registered C++ corpus
        # test and is reproducibly bound to the exact fixture index, production
        # decoders, generated descriptors, grouped facade, and wire test.
        # Validation rejects stale hashes, missing/duplicate rows, wrong
        # targets, and false success/malformed-known outcome promotion before
        # these local completeness facts may advance.
        evidence["direction_assertions_exercised"] = True
        evidence["runtime_decoder_matches_registry"] = True
        evidence["opaque_fields_declared"] = True
        evidence["no_known_schema_fields_dropped"] = True
    if (
        identity in B4_CONVERSATION_UNION_CODECS
        and target is not None
        and operation_production_coverage is not None
        and identity
        in operation_production_coverage.get("b4_conversation_unions", {})
    ):
        # Operation-owned B4 unions advance only through the checked exact
        # fixture table consumed by the registered production C++ corpus.
        # Descriptor presence by itself is deliberately insufficient.
        evidence["direction_assertions_exercised"] = True
        evidence["runtime_decoder_matches_registry"] = True
        evidence["opaque_fields_declared"] = True
        evidence["no_known_schema_fields_dropped"] = True
    if (
        identity[0] == "server_notification"
        and assignment.get("slice") == "A1.1"
        and target is not None
        and notification_production_coverage is not None
        and identity in notification_production_coverage
    ):
        # Notification identities advance only through the checked exact
        # fixture/type/production association consumed by the registered
        # C++ corpus. Descriptor presence alone is deliberately insufficient.
        evidence["direction_assertions_exercised"] = True
        evidence["runtime_decoder_matches_registry"] = True
        evidence["opaque_fields_declared"] = True
        evidence["no_known_schema_fields_dropped"] = True
    if (
        identity[0] == "item_discriminator"
        and identity[1] in {"ThreadItem", "ResponseItem"}
        and target is not None
    ):
        # Commit 3's two generated item target families are exact-keyed
        # production descriptors. The schema-derived item corpus exercises
        # every known branch, represented property, presence state, open
        # value, and the seven protocol-defined opaque paths through the
        # production decoders.
        evidence["direction_assertions_exercised"] = True
        evidence["runtime_decoder_matches_registry"] = True
        evidence["opaque_fields_declared"] = True
        evidence["no_known_schema_fields_dropped"] = True
    schema_status = derived_schema_status(
        typed_status.removeprefix("TypedImplementationStatus::"),
        slice_name,
        evidence,
    )

    return (
        runtime_disposition,
        typed_status,
        backend_status,
        canonical_state_status,
        frontend_exposure,
        frontend_security,
        target or "std::monostate{}",
        cpp_string(parameter_type_identity),
        cpp_string(result_type_identity),
        result_contract_cpp,
        association_evidence_cpp,
        cpp_string(association_evidence_key),
        cpp_string(module),
        CPP_A1_SLICES[slice_name],
        CPP_TYPED_SCHEMA_STATUSES[schema_status],
        "schemaCompletenessEvidence("
        + ", ".join("true" if evidence[field] else "false" for field in COMPLETENESS_EVIDENCE_FIELDS)
        + ")",
    )


def generate_registry_data(
    manifest: dict[str, Any], evidence: dict[str, Any] | None = None
) -> str:
    entries = manifest.get("entries")
    if not isinstance(entries, list):
        raise SurfaceError("surface manifest has no entries array")
    evidence = evidence if evidence is not None else load_a1_registry_evidence()
    contracts = operation_contract_by_key(manifest, evidence["operation_contracts"])
    assignment_diagnostics = assignment_reachability_diagnostics(
        manifest, evidence["assignments"], evidence["reachability"]
    )
    if assignment_diagnostics:
        first = assignment_diagnostics[0]
        raise SurfaceError(
            f"{first['code']}: {first['message']} ({first['surface_key']})"
        )
    assignments = assignment_by_key(manifest, evidence["assignments"])
    fixtures = fixture_evidence_by_key(evidence["fixture_coverage"])
    operation_production_coverage = validate_operation_production_coverage(
        evidence.get("operation_production_coverage"),
        manifest,
        evidence,
    )
    notification_production_coverage = (
        validate_notification_production_coverage(
            evidence.get("notification_production_coverage"),
            manifest,
            evidence,
        )
    )

    lines = [
        "// Generated by tools/codex/app_server_surface.py registry.",
        "// Inventory fields come only from the vendored JSON Schema manifest.",
        "// Operation contracts and A1 assignments come only from guarded offline evidence.",
        "// Local disposition fields remain the sole production implementation authority.",
    ]
    for entry in entries:
        try:
            category = CPP_CATEGORIES[entry["category"]]
        except KeyError as error:
            raise SurfaceError(f"registry has unknown category: {entry.get('category')!r}") from error
        key = surface_key(entry)
        statuses = registry_statuses(
            entry,
            contracts.get(key),
            assignments[key],
            fixtures.get(key, ()),
            operation_production_coverage,
            notification_production_coverage,
        )
        arguments = (
            category,
            cpp_string(entry["domain"]),
            cpp_string(entry["discriminator_field"]),
            cpp_string(entry["name"]),
            "Stability::Stable"
            if entry["stability"] == "stable"
            else "Stability::ExperimentalOnly",
            "true" if entry["deprecated"] else "false",
            *statuses,
        )
        lines.append("CODEX_PROTOCOL_SURFACE_ENTRY(" + ", ".join(arguments) + ")")
    return "\n".join(lines) + "\n"


def _conversation_union_schema_branch(
    entry: dict[str, Any], schema_root: Path
) -> dict[str, Any]:
    sources = entry.get("sources")
    if not isinstance(sources, list):
        raise SurfaceError(
            "ConversationUnionDescriptorSchemaMismatch: "
            f"{surface_key(entry)} has no schema sources"
        )
    source = next(
        (
            candidate
            for candidate in sources
            if isinstance(candidate, dict)
            and candidate.get("file") == "codex_app_server_protocol.schemas.json"
            and isinstance(candidate.get("pointer"), str)
            and candidate["pointer"].startswith("/definitions/v2/")
        ),
        None,
    )
    if source is None:
        raise SurfaceError(
            "ConversationUnionDescriptorSchemaMismatch: "
            f"{surface_key(entry)} has no stable v2 aggregate source"
        )
    path = schema_root / "stable" / source["file"]
    document = load_json(path)
    branch = json_pointer(document, source["pointer"], str(path))
    if not isinstance(branch, dict):
        raise SurfaceError(
            "ConversationUnionDescriptorSchemaMismatch: "
            f"{surface_key(entry)} does not resolve to an object-valued schema branch"
        )
    return branch


def _validate_conversation_union_descriptor_shape(
    entry: dict[str, Any], branch: dict[str, Any], shape: str
) -> None:
    key = surface_key(entry)
    field = entry["discriminator_field"]
    name = entry["name"]
    if shape == "ConversationUnionCodecShape::ScalarString":
        valid = (
            field == "$variant"
            and branch.get("type") == "string"
            and isinstance(branch.get("enum"), list)
            and name in branch["enum"]
        )
    elif shape == "ConversationUnionCodecShape::ExternallyTaggedObject":
        properties = branch.get("properties")
        valid = (
            field == "$variant"
            and branch.get("type") == "object"
            and isinstance(properties, dict)
            and set(properties) == {name}
            and branch.get("required") == [name]
        )
    elif shape == "ConversationUnionCodecShape::InternallyTaggedObject":
        properties = branch.get("properties")
        discriminator = (
            properties.get(field) if isinstance(properties, dict) else None
        )
        valid = (
            field == "type"
            and branch.get("type") == "object"
            and isinstance(discriminator, dict)
            and discriminator.get("enum") == [name]
            and field in branch.get("required", [])
        )
    else:
        valid = False
    if not valid:
        raise SurfaceError(
            "ConversationUnionDescriptorSchemaMismatch: "
            f"{key} does not match reviewed codec shape {shape}"
        )


def generate_conversation_union_descriptor_data(
    manifest: dict[str, Any],
    schema_root: Path,
    evidence: dict[str, Any] | None = None,
) -> str:
    """Generate private codec metadata from one reviewed exact-key mapping."""

    evidence = evidence if evidence is not None else load_a1_registry_evidence()
    assignments = assignment_by_key(manifest, evidence["assignments"])
    expected_keys = {
        key
        for key, assignment in assignments.items()
        if assignment.get("slice") == "A1.1"
        and (
            (
                assignment.get("classification") == "SharedCommon"
                and assignment.get("module") == "Common"
            )
            or (
                assignment.get("classification") == "RootOwnedNestedUnion"
                and assignment.get("module") == "ThreadsTurnsSessions"
            )
            or (
                key[0] == "tagged_union_discriminator"
                and key[1] in {"SessionSource", "SubAgentSource", "ThreadStatus"}
                and assignment.get("classification")
                in {"SharedWithinSlice", "StablePublicRoot"}
                and assignment.get("module") == "ThreadsTurnsSessions"
            )
        )
        and assignment.get("stability") == "stable"
    }
    descriptor_keys = set(CONVERSATION_UNION_CODECS)
    if expected_keys != descriptor_keys or len(descriptor_keys) != 58:
        raise SurfaceError(
            "ConversationUnionDescriptorAssignmentMismatch: "
            "reviewed descriptor keys must equal the exact 26 stable B2 "
            "SharedCommon/Common, 16 B3 RootOwnedNestedUnion assignments, "
            "and 16 dependency-closed B4 operation union assignments"
        )

    manifest_entries = {
        surface_key(entry): entry for entry in manifest.get("entries", [])
    }
    targets = [metadata[0] for metadata in CONVERSATION_UNION_CODECS.values()]
    if len(set(targets)) != 58:
        raise SurfaceError(
            "DuplicateConversationUnionDescriptorTarget: "
            "each exact key must own one unique runtime target"
        )
    if (
        sum(
            metadata[2] == "ConversationUnionCodecDirection::Bidirectional"
            for metadata in CONVERSATION_UNION_CODECS.values()
        )
        != 13
        or sum(
            metadata[2] == "ConversationUnionCodecDirection::DecodeOnly"
            for metadata in CONVERSATION_UNION_CODECS.values()
        )
        != 45
    ):
        raise SurfaceError(
            "ConversationUnionDescriptorDirectionMismatch: "
            "reviewed B2+B3+B4 direction split must remain "
            "13 bidirectional/45 decode-only"
        )

    lines = [
        "// Generated by tools/codex/app_server_surface.py conversation-descriptors; do not edit.",
        "// Exact keys remain subordinate to ProtocolSurfaceRegistryData.inc.",
        "// Shape and direction are private codec metadata, not production dispositions.",
    ]
    for key in sorted(descriptor_keys):
        entry = manifest_entries.get(key)
        if (
            entry is None
            or entry.get("stability") != "stable"
            or entry.get("category") != "tagged_union_discriminator"
        ):
            raise SurfaceError(
                "ConversationUnionDescriptorAssignmentMismatch: "
                f"missing stable tagged-union manifest entry for {key}"
            )
        target, shape, direction = CONVERSATION_UNION_CODECS[key]
        branch = _conversation_union_schema_branch(entry, schema_root)
        _validate_conversation_union_descriptor_shape(entry, branch, shape)
        arguments = (
            CPP_CATEGORIES[key[0]],
            cpp_string(key[1]),
            cpp_string(key[2]),
            cpp_string(key[3]),
            target,
            shape,
            direction,
        )
        lines.append(
            "CODEX_CONVERSATION_UNION_CODEC_DESCRIPTOR("
            + ", ".join(arguments)
            + ")"
        )
    return "\n".join(lines) + "\n"


def generate_client_operation_descriptor_data(
    manifest: dict[str, Any],
    evidence: dict[str, Any] | None = None,
) -> str:
    """Generate exact private method/contract metadata for the A1.1 requests."""

    evidence = evidence if evidence is not None else load_a1_registry_evidence()
    assignments = assignment_by_key(manifest, evidence["assignments"])
    contracts = operation_contract_by_key(
        manifest, evidence["operation_contracts"]
    )
    expected_keys = {
        key
        for key, assignment in assignments.items()
        if key[0] == "client_request"
        and assignment.get("slice") == "A1.1"
        and assignment.get("classification") == "StablePublicRoot"
        and assignment.get("module") == "ThreadsTurnsSessions"
        and assignment.get("stability") == "stable"
    }
    targets = {
        key: target
        for key, target in RUNTIME_TARGETS.items()
        if key in expected_keys
    }
    if (
        len(expected_keys) != 22
        or set(targets) != expected_keys
        or len(set(targets.values())) != 22
        or any(
            not target.startswith("ClientRequestTarget::")
            for target in targets.values()
        )
    ):
        raise SurfaceError(
            "ClientOperationDescriptorAssignmentMismatch: "
            "the exact 22 stable A1.1 client requests must each own one "
            "unique ClientRequestTarget"
        )
    if set(contracts) & expected_keys != expected_keys:
        raise SurfaceError(
            "ClientOperationDescriptorContractMismatch: "
            "an A1.1 request lacks its authoritative operation contract"
        )

    unit_keys = {
        key
        for key in expected_keys
        if contracts[key]["result_contract_kind"] == "Unit"
    }
    expected_unit_methods = {
        "thread/archive",
        "thread/compact/start",
        "thread/delete",
        "thread/inject_items",
        "thread/name/set",
        "thread/shellCommand",
        "turn/interrupt",
    }
    if (
        {key[3] for key in unit_keys} != expected_unit_methods
        or len(expected_keys - unit_keys) != 15
        or any(
            contracts[key]["result_contract_kind"] != "Concrete"
            for key in expected_keys - unit_keys
        )
    ):
        raise SurfaceError(
            "ClientOperationDescriptorResultKindMismatch: "
            "A1.1 must remain exactly 7 Unit and 15 Concrete requests"
        )

    result_decoders = {
        "Unit",
        "ThreadForkResponse",
        "ThreadGoalClearResponse",
        "ThreadGoalGetResponse",
        "ThreadGoalSetResponse",
        "ThreadListResponse",
        "ThreadLoadedListResponse",
        "ThreadMetadataUpdateResponse",
        "ThreadReadResponse",
        "ThreadResumeResponse",
        "ThreadRollbackResponse",
        "ThreadStartResponse",
        "ThreadUnarchiveResponse",
        "ThreadUnsubscribeResponse",
        "TurnStartResponse",
        "TurnSteerResponse",
    }
    result_type_identities = {
        str(contracts[key]["result_type_identity"]) for key in expected_keys
    }
    if result_type_identities != result_decoders:
        raise SurfaceError(
            "ClientOperationDescriptorDecoderMismatch: "
            "the exact authoritative result identities must each have one "
            "reviewed production result decoder binding"
        )

    lines = [
        "// Generated by tools/codex/app_server_surface.py operation-descriptors; do not edit.",
        "// Exact method keys and contracts remain subordinate to ProtocolSurfaceRegistryData.inc.",
        "// Descriptor rows are private codec metadata, not a second disposition registry.",
    ]
    for key in sorted(expected_keys):
        contract = contracts[key]
        arguments = (
            CPP_CATEGORIES[key[0]],
            cpp_string(key[1]),
            cpp_string(key[2]),
            cpp_string(key[3]),
            targets[key],
            cpp_string(targets[key]),
            cpp_string(str(contract["parameter_type_identity"])),
            cpp_string(str(contract["result_type_identity"])),
            CPP_RESULT_CONTRACT_KINDS[
                str(contract["result_contract_kind"])
            ],
            "ClientOperationResultDecoder::"
            + str(contract["result_type_identity"]),
        )
        lines.append(
            "CODEX_CLIENT_OPERATION_CODEC_DESCRIPTOR("
            + ", ".join(arguments)
            + ")"
        )
    return "\n".join(lines) + "\n"


def generate_server_notification_descriptor_data(
    manifest: dict[str, Any],
    evidence: dict[str, Any] | None = None,
) -> str:
    """Generate exact private decode metadata for typed server notifications."""

    evidence = evidence if evidence is not None else load_a1_registry_evidence()
    assignments = assignment_by_key(manifest, evidence["assignments"])
    manifest_entries = {
        surface_key(entry): entry for entry in manifest.get("entries", [])
    }
    expected_keys = {
        key
        for key, target in RUNTIME_TARGETS.items()
        if key[0] == "server_notification"
        and target.startswith("ServerNotificationTarget::")
    }
    descriptor_keys = set(SERVER_NOTIFICATION_CODECS)
    if (
        len(expected_keys) != 39
        or descriptor_keys != expected_keys
        or len({metadata[0] for metadata in SERVER_NOTIFICATION_CODECS.values()})
        != 39
    ):
        raise SurfaceError(
            "ServerNotificationDescriptorAssignmentMismatch: "
            "every one of the 39 typed server-notification targets must own "
            "one exact generated descriptor"
        )

    a11_keys = {
        key
        for key in descriptor_keys
        if assignments[key].get("slice") == "A1.1"
    }
    residual_keys = descriptor_keys - a11_keys
    if (
        len(a11_keys) != 37
        or {key[3] for key in residual_keys} != {"error", "model/rerouted"}
    ):
        raise SurfaceError(
            "ServerNotificationDescriptorSliceMismatch: "
            "descriptors must distinguish the exact 37 A1.1 rows from "
            "residual partial error and model/rerouted rows"
        )

    lines = [
        "// Generated by tools/codex/app_server_surface.py notification-descriptors; do not edit.",
        "// Exact method keys remain subordinate to ProtocolSurfaceRegistryData.inc.",
        "// Payload identity and A1.1 membership are private decode metadata, not dispatch authority.",
    ]
    for key in sorted(descriptor_keys):
        entry = manifest_entries.get(key)
        if (
            entry is None
            or entry.get("stability") != "stable"
            or entry.get("category") != "server_notification"
            or entry.get("domain") != "ServerNotification"
            or entry.get("discriminator_field") != "method"
        ):
            raise SurfaceError(
                "ServerNotificationDescriptorAssignmentMismatch: "
                f"missing stable server-notification manifest entry for {key}"
            )
        target, payload_type = SERVER_NOTIFICATION_CODECS[key]
        arguments = (
            CPP_CATEGORIES[key[0]],
            cpp_string(key[1]),
            cpp_string(key[2]),
            cpp_string(key[3]),
            target,
            cpp_string(payload_type),
            "true" if key in a11_keys else "false",
        )
        lines.append(
            "CODEX_SERVER_NOTIFICATION_CODEC_DESCRIPTOR("
            + ", ".join(arguments)
            + ")"
        )
    return "\n".join(lines) + "\n"


def _operation_production_specs(
    manifest: dict[str, Any], evidence: dict[str, Any]
) -> dict[tuple[str, str, str, str], dict[str, str]]:
    assignments = assignment_by_key(manifest, evidence["assignments"])
    contracts = operation_contract_by_key(
        manifest, evidence["operation_contracts"]
    )
    expected_keys = {
        key
        for key, assignment in assignments.items()
        if key[0] == "client_request"
        and assignment.get("slice") == "A1.1"
        and assignment.get("classification") == "StablePublicRoot"
        and assignment.get("module") == "ThreadsTurnsSessions"
        and assignment.get("stability") == "stable"
    }
    if len(expected_keys) != 22:
        raise SurfaceError(
            "OperationProductionCoverageAssignmentMismatch: "
            "expected exactly 22 stable A1.1 client requests"
        )
    result: dict[tuple[str, str, str, str], dict[str, str]] = {}
    for key in sorted(expected_keys):
        target = RUNTIME_TARGETS.get(key)
        contract = contracts.get(key)
        if (
            not isinstance(target, str)
            or not target.startswith("ClientRequestTarget::")
            or contract is None
        ):
            raise SurfaceError(
                "OperationProductionCoverageTargetMismatch: "
                f"missing exact production target/contract for {key}"
            )
        result[key] = {
            "runtime_target": target,
            "parameter_type_identity": str(
                contract["parameter_type_identity"]
            ),
            "result_type_identity": str(contract["result_type_identity"]),
            "result_contract_kind": str(
                contract["result_contract_kind"]
            ),
        }
    kinds = [spec["result_contract_kind"] for spec in result.values()]
    if kinds.count("Unit") != 7 or kinds.count("Concrete") != 15:
        raise SurfaceError(
            "OperationProductionCoverageResultKindMismatch: "
            "expected exactly seven Unit and fifteen Concrete results"
        )
    return result


def _b4_conversation_production_specs(
    manifest: dict[str, Any], evidence: dict[str, Any]
) -> dict[tuple[str, str, str, str], dict[str, str]]:
    assignments = assignment_by_key(manifest, evidence["assignments"])
    expected_keys = {
        key
        for key, assignment in assignments.items()
        if key in B4_CONVERSATION_UNION_CODECS
        and assignment.get("slice") == "A1.1"
        and assignment.get("classification")
        in {"RootOwnedNestedUnion", "SharedWithinSlice"}
        and assignment.get("module") == "ThreadsTurnsSessions"
        and assignment.get("stability") == "stable"
    }
    if expected_keys != set(B4_CONVERSATION_UNION_CODECS) or len(expected_keys) != 16:
        raise SurfaceError(
            "ConversationProductionCoverageAssignmentMismatch: "
            "expected exactly the 16 dependency-closed B4 conversation-union identities"
        )
    result: dict[tuple[str, str, str, str], dict[str, str]] = {}
    for key in sorted(expected_keys):
        target, shape, direction = B4_CONVERSATION_UNION_CODECS[key]
        if (
            RUNTIME_TARGETS.get(key) != target
            or not target.startswith("ConversationUnionTarget::")
            or direction != "ConversationUnionCodecDirection::DecodeOnly"
        ):
            raise SurfaceError(
                "ConversationProductionCoverageTargetMismatch: "
                f"missing exact decode-only production target for {key}"
            )
        result[key] = {
            "runtime_target": target,
            "codec_shape": shape.removeprefix("ConversationUnionCodecShape::"),
            "codec_direction": direction.removeprefix(
                "ConversationUnionCodecDirection::"
            ),
        }
    return result


def generate_operation_production_coverage(
    manifest: dict[str, Any],
    evidence: dict[str, Any],
    fixture_index: dict[str, Any],
    repo_root: Path = DEFAULT_REPO_ROOT,
    fixture_index_path: Path = DEFAULT_A1_FIXTURE_INDEX,
) -> dict[str, Any]:
    """Generate the one checked operation-result corpus/production table."""

    specs = _operation_production_specs(manifest, evidence)
    conversation_specs = _b4_conversation_production_specs(manifest, evidence)
    allowed_roles = {
        "client_request_result": ("Decoded",),
        "operation_optional_omitted": ("Decoded",),
        "operation_nullable_null": ("Decoded",),
        "operation_missing_required": ("MalformedKnownPayload",),
        "operation_wrong_type": ("MalformedKnownPayload",),
    }
    indexed_records = fixture_index.get("fixtures")
    if not isinstance(indexed_records, list):
        raise SurfaceError(
            "OperationProductionCoverageFixtureIndexMismatch: "
            "fixture index lacks its records array"
        )

    records: list[dict[str, Any]] = []
    role_counts: dict[str, int] = {role: 0 for role in sorted(allowed_roles)}
    per_key: dict[
        tuple[str, str, str, str], list[dict[str, Any]]
    ] = {key: [] for key in specs}
    seen_ids: set[str] = set()
    for fixture in indexed_records:
        if not isinstance(fixture, dict):
            continue
        key_object = fixture.get("protocol_surface_key")
        if (
            fixture.get("a1_slice") != "A1.1"
            or not isinstance(key_object, dict)
            or key_object.get("category") != "client_request"
            or ":result" not in str(fixture.get("id", ""))
        ):
            continue
        key = surface_key(key_object)
        if key not in specs:
            raise SurfaceError(
                "OperationProductionCoverageFixtureIndexMismatch: "
                f"result fixture is outside the exact operation set: {key}"
            )
        role = fixture.get("role")
        if role not in allowed_roles:
            raise SurfaceError(
                "OperationProductionCoverageRoleMismatch: "
                f"unsupported result fixture role {role!r}"
            )
        fixture_id = fixture.get("id")
        file = fixture.get("file")
        file_sha256 = fixture.get("file_sha256")
        if (
            not isinstance(fixture_id, str)
            or not isinstance(file, str)
            or not isinstance(file_sha256, str)
            or not file_sha256
        ):
            raise SurfaceError(
                "OperationProductionCoverageFixtureIndexMismatch: "
                "result fixture lacks a stable id/path/hash"
            )
        if fixture_id in seen_ids:
            raise SurfaceError(
                "OperationProductionCoverageDuplicateRecord: "
                f"duplicate fixture id {fixture_id}"
            )
        seen_ids.add(fixture_id)
        record = {
            "id": fixture_id,
            "surface_key": _canonical_surface_key_object(key),
            "runtime_target": specs[key]["runtime_target"],
            "result_type_identity": specs[key][
                "result_type_identity"
            ],
            "result_contract_kind": specs[key][
                "result_contract_kind"
            ],
            "role": role,
            "file": file,
            "file_sha256": file_sha256,
            "expected_intrinsic_codes": list(allowed_roles[role]),
        }
        records.append(record)
        per_key[key].append(record)
        role_counts[role] += 1

    expected_role_counts = {
        "client_request_result": 22,
        "operation_missing_required": 261,
        "operation_nullable_null": 180,
        "operation_optional_omitted": 201,
        "operation_wrong_type": 502,
    }
    if role_counts != expected_role_counts or len(records) != 1166:
        raise SurfaceError(
            "OperationProductionCoverageRoleMismatch: "
            f"expected exact result roles {expected_role_counts}, got {role_counts}"
        )

    operations: list[dict[str, Any]] = []
    for key in sorted(specs):
        operation_records = sorted(per_key[key], key=lambda record: record["id"])
        base = [
            record
            for record in operation_records
            if record["role"] == "client_request_result"
        ]
        if len(base) != 1:
            raise SurfaceError(
                "OperationProductionCoverageMissingRecord: "
                f"{key} lacks exactly one base result fixture"
            )
        counts = {
            role: sum(record["role"] == role for record in operation_records)
            for role in sorted(allowed_roles)
        }
        operations.append(
            {
                "surface_key": _canonical_surface_key_object(key),
                **specs[key],
                "role_counts": counts,
                "fixture_ids": [
                    record["id"] for record in operation_records
                ],
            }
        )

    conversation_role_codes = {
        "union_branch": ("Decoded",),
        "union_optional_omitted": ("Decoded",),
        "union_nullable_null": ("Decoded",),
        "malformed_known_missing_required": ("MalformedKnownPayload",),
        "malformed_known_missing_discriminator": (
            "MalformedKnownPayload",
        ),
        "malformed_known_wrong_type": ("MalformedKnownPayload",),
        "malformed_known_wrong_discriminator_type": (
            "MalformedKnownPayload",
        ),
        "nested_union_failure": ("MalformedKnownPayload",),
        "unknown_discriminator": ("Decoded", "UnknownDiscriminator"),
    }
    conversation_records: list[dict[str, Any]] = []
    conversation_per_key: dict[
        tuple[str, str, str, str], list[dict[str, Any]]
    ] = {key: [] for key in conversation_specs}
    conversation_role_counts = {
        role: 0 for role in sorted(conversation_role_codes)
    }

    def b4_expected_field_path(
        fixture: dict[str, Any],
        key: tuple[str, str, str, str],
    ) -> str | None:
        role = fixture.get("role")
        if role in {
            "union_branch",
            "union_optional_omitted",
            "union_nullable_null",
        }:
            return None
        domain = key[1]
        name = key[3]
        fixture_id = str(fixture.get("id", ""))
        if role == "malformed_known_missing_discriminator":
            return "$.type" if domain == "ThreadStatus" else "$"
        if role == "malformed_known_wrong_discriminator_type":
            return "$.type" if domain == "ThreadStatus" else f"$.{name}"
        if domain == "ThreadStatus":
            return (
                "$.activeFlags[0]"
                if fixture_id.endswith("activeflags-0")
                else "$.activeFlags"
            )
        if domain == "SessionSource" and name == "subAgent":
            if fixture_id.endswith("thread_spawn-depth"):
                return "$.subAgent.thread_spawn.depth"
            return "$.subAgent"
        if domain == "SubAgentSource" and name == "thread_spawn":
            if fixture_id.endswith("-depth"):
                return "$.thread_spawn.depth"
            if fixture_id.endswith("-parent_thread_id"):
                return "$.thread_spawn.parent_thread_id"
            if fixture_id.endswith("-agent_path"):
                return "$.thread_spawn.agent_path"
            if fixture_id.endswith("-agent_nickname"):
                return "$.thread_spawn.agent_nickname"
            if fixture_id.endswith("-agent_role"):
                return "$.thread_spawn.agent_role"
        raise SurfaceError(
            "ConversationProductionCoverageFieldPathMismatch: "
            f"cannot derive the exact diagnostic path for {fixture_id}"
        )

    for fixture in indexed_records:
        if not isinstance(fixture, dict):
            continue
        key_object = fixture.get("protocol_surface_key")
        if not isinstance(key_object, dict):
            continue
        key = surface_key(key_object)
        if key not in conversation_specs:
            continue
        role = fixture.get("role")
        if role not in conversation_role_codes:
            raise SurfaceError(
                "ConversationProductionCoverageRoleMismatch: "
                f"unsupported B4 union fixture role {role!r}"
            )
        fixture_id = fixture.get("id")
        file = fixture.get("file")
        file_sha256 = fixture.get("file_sha256")
        if (
            not isinstance(fixture_id, str)
            or not isinstance(file, str)
            or not isinstance(file_sha256, str)
            or not file_sha256
        ):
            raise SurfaceError(
                "ConversationProductionCoverageFixtureIndexMismatch: "
                "B4 union fixture lacks a stable id/path/hash"
            )
        record = {
            "id": fixture_id,
            "surface_key": _canonical_surface_key_object(key),
            "domain": key[1],
            "runtime_target": conversation_specs[key]["runtime_target"],
            "role": role,
            "file": file,
            "file_sha256": file_sha256,
            "expected_intrinsic_codes": list(
                conversation_role_codes[role]
            ),
            "expected_diagnostic_surface": (
                "SubAgentSource"
                if key[1] == "SessionSource"
                and key[3] == "subAgent"
                and role
                in {
                    "nested_union_failure",
                    "unknown_discriminator",
                    "malformed_known_wrong_discriminator_type",
                }
                else key[1]
            ),
            "expected_field_path": b4_expected_field_path(fixture, key),
        }
        conversation_records.append(record)
        conversation_per_key[key].append(record)
        conversation_role_counts[role] += 1

    unkeyed_union_specs = {
        "union:SessionSource:conflicting-discriminators": (
            "SessionSource",
            ("MalformedKnownPayload",),
            "$",
        ),
        "union:SessionSource:future-object-unknown": (
            "SessionSource",
            ("Decoded", "UnknownDiscriminator"),
            "$",
        ),
        "union:SessionSource:future-unknown": (
            "SessionSource",
            ("Decoded", "UnknownDiscriminator"),
            "$",
        ),
        "union:SessionSource:wrong-outer-shape": (
            "SessionSource",
            ("MalformedKnownPayload",),
            "$",
        ),
        "union:SubAgentSource:conflicting-discriminators": (
            "SubAgentSource",
            ("MalformedKnownPayload",),
            "$",
        ),
        "union:SubAgentSource:future-object-unknown": (
            "SubAgentSource",
            ("Decoded", "UnknownDiscriminator"),
            "$",
        ),
        "union:SubAgentSource:future-unknown": (
            "SubAgentSource",
            ("Decoded", "UnknownDiscriminator"),
            "$",
        ),
        "union:SubAgentSource:wrong-outer-shape": (
            "SubAgentSource",
            ("MalformedKnownPayload",),
            "$",
        ),
        "union:ThreadStatus:future-unknown": (
            "ThreadStatus",
            ("Decoded", "UnknownDiscriminator"),
            "$.type",
        ),
        "union:ThreadStatus:wrong-outer-shape": (
            "ThreadStatus",
            ("MalformedKnownPayload",),
            "$",
        ),
    }
    unkeyed_union_records = {
        fixture.get("id"): fixture
        for fixture in indexed_records
        if isinstance(fixture, dict)
        and fixture.get("id") in unkeyed_union_specs
    }
    if set(unkeyed_union_records) != set(unkeyed_union_specs):
        raise SurfaceError(
            "ConversationProductionCoverageMissingRecord: "
            "the ten unkeyed B4 future/conflict/outer-shape fixtures must be indexed"
        )
    for fixture_id in sorted(unkeyed_union_specs):
        fixture = unkeyed_union_records[fixture_id]
        domain, codes, field_path = unkeyed_union_specs[fixture_id]
        conversation_records.append(
            {
                "id": fixture_id,
                "surface_key": None,
                "domain": domain,
                "runtime_target": None,
                "role": fixture["role"],
                "file": fixture["file"],
                "file_sha256": fixture["file_sha256"],
                "expected_intrinsic_codes": list(codes),
                "expected_diagnostic_surface": domain,
                "expected_field_path": field_path,
            }
        )
        conversation_role_counts.setdefault(fixture["role"], 0)
        conversation_role_counts[fixture["role"]] += 1

    expected_conversation_role_counts = {
        "malformed_known_conflicting_discriminators": 2,
        "malformed_known_missing_discriminator": 8,
        "malformed_known_missing_required": 3,
        "malformed_known_wrong_discriminator_type": 8,
        "malformed_known_wrong_outer_shape": 3,
        "malformed_known_wrong_type": 7,
        "nested_union_failure": 1,
        "union_branch": 16,
        "union_nullable_null": 3,
        "union_optional_omitted": 3,
        "unknown_discriminator": 6,
    }
    if (
        conversation_role_counts != expected_conversation_role_counts
        or len(conversation_records) != 60
    ):
        raise SurfaceError(
            "ConversationProductionCoverageRoleMismatch: "
            "expected the exact 60-record B4 union corpus "
            f"{expected_conversation_role_counts}, got {conversation_role_counts}"
        )

    conversation_unions: list[dict[str, Any]] = []
    for key in sorted(conversation_specs):
        union_records = sorted(
            conversation_per_key[key], key=lambda record: record["id"]
        )
        base = [
            record
            for record in union_records
            if record["role"] == "union_branch"
        ]
        if len(base) != 1:
            raise SurfaceError(
                "ConversationProductionCoverageMissingRecord: "
                f"{key} lacks exactly one base branch fixture"
            )
        conversation_unions.append(
            {
                "surface_key": _canonical_surface_key_object(key),
                **conversation_specs[key],
                "role_counts": {
                    role: sum(
                        record["role"] == role
                        for record in union_records
                    )
                    for role in sorted(conversation_role_codes)
                },
                "fixture_ids": [
                    record["id"] for record in union_records
                ],
                "base_fixture_id": base[0]["id"],
            }
        )

    active_key = (
        "tagged_union_discriminator",
        "ThreadStatus",
        "type",
        "active",
    )
    open_enum_role_codes = {
        "open_enum_known_value": ("Decoded",),
        "unknown_enum_value": ("Decoded", "UnknownEnumValue"),
    }
    open_enum_records: list[dict[str, Any]] = []
    open_enum_role_counts = {
        role: 0 for role in sorted(open_enum_role_codes)
    }
    for fixture in indexed_records:
        if (
            not isinstance(fixture, dict)
            or not str(fixture.get("id", "")).startswith(
                "enum:ThreadActiveFlag:"
            )
        ):
            continue
        role = fixture.get("role")
        if role not in open_enum_role_codes:
            raise SurfaceError(
                "ConversationProductionCoverageRoleMismatch: "
                f"unsupported ThreadActiveFlag fixture role {role!r}"
            )
        fixture_path = fixture_index_path.parent / str(fixture["file"])
        try:
            fixture_value = load_json(fixture_path)
        except OSError as error:
            raise SurfaceError(
                "ConversationProductionCoverageFixtureIndexMismatch: "
                f"unable to read {fixture_path}: {error}"
            ) from error
        if not isinstance(fixture_value, str):
            raise SurfaceError(
                "ConversationProductionCoverageFixtureIndexMismatch: "
                f"{fixture['id']} must contain one open string-enum value"
            )
        open_enum_records.append(
            {
                "id": fixture["id"],
                "owner_surface_key": _canonical_surface_key_object(
                    active_key
                ),
                "runtime_target": conversation_specs[active_key][
                    "runtime_target"
                ],
                "role": role,
                "value": fixture_value,
                "file": fixture["file"],
                "file_sha256": fixture["file_sha256"],
                "expected_intrinsic_codes": list(
                    open_enum_role_codes[role]
                ),
                "expected_field_path": (
                    None
                    if role == "open_enum_known_value"
                    else "$.activeFlags[0]"
                ),
            }
        )
        open_enum_role_counts[role] += 1
    expected_open_enum_role_counts = {
        "open_enum_known_value": 2,
        "unknown_enum_value": 2,
    }
    if (
        open_enum_role_counts != expected_open_enum_role_counts
        or len(open_enum_records) != 4
    ):
        raise SurfaceError(
            "ConversationProductionCoverageRoleMismatch: "
            "expected the exact four-record ThreadActiveFlag corpus "
            f"{expected_open_enum_role_counts}, got {open_enum_role_counts}"
        )

    helper_specs = {
        "ReasoningSummary": (
            (
                "client_request",
                "ClientRequest",
                "method",
                "turn/start",
            ),
            "Encode",
            ("Encoded",),
        ),
        "ThreadListCwdFilter": (
            (
                "client_request",
                "ClientRequest",
                "method",
                "thread/list",
            ),
            "Encode",
            ("Encoded",),
        ),
        "TurnItemsView": (
            (
                "client_request",
                "ClientRequest",
                "method",
                "turn/start",
            ),
            "Decode",
            ("Decoded",),
        ),
    }
    helper_records: list[dict[str, Any]] = []
    helper_counts = {domain: 0 for domain in sorted(helper_specs)}
    for fixture in indexed_records:
        if not isinstance(fixture, dict):
            continue
        fixture_id = str(fixture.get("id", ""))
        if not fixture_id.startswith("helper-union:"):
            continue
        role = fixture.get("role")
        if role not in {
            "operation_helper_union_branch",
            "unknown_enum_value",
            "operation_wrong_type",
        }:
            raise SurfaceError(
                "OperationProductionCoverageHelperMismatch: "
                f"unsupported helper fixture role {role!r}"
            )
        parts = fixture_id.split(":")
        if len(parts) != 3 or parts[0] != "helper-union":
            raise SurfaceError(
                "OperationProductionCoverageHelperMismatch: "
                f"malformed helper fixture id {fixture_id}"
            )
        domain = parts[1]
        if domain not in helper_specs:
            raise SurfaceError(
                "OperationProductionCoverageHelperMismatch: "
                f"unreviewed operation helper family {domain}"
            )
        if (
            domain == "ThreadListCwdFilter"
            and role == "operation_wrong_type"
        ):
            # This independent schema-negative record proves numeric array
            # elements are rejected. The public C++ value variant cannot
            # represent it, which the aggregate corpus checks at compile time.
            continue
        owner_key, direction, known_codes = helper_specs[domain]
        codes = (
            known_codes
            if role == "operation_helper_union_branch"
            else (
                (known_codes[0], "UnknownEnumValue")
                if direction == "Decode"
                else known_codes
            )
        )
        helper_records.append(
            {
                "id": fixture_id,
                "domain": domain,
                "branch": parts[2],
                "owner_surface_key": _canonical_surface_key_object(
                    owner_key
                ),
                "runtime_target": specs[owner_key]["runtime_target"],
                "direction": direction,
                "role": role,
                "file": fixture["file"],
                "file_sha256": fixture["file_sha256"],
                "expected_intrinsic_codes": list(codes),
                "expected_field_path": (
                    None
                    if role == "operation_helper_union_branch"
                    or direction == "Encode"
                    else (
                        "$.summary"
                        if domain == "ReasoningSummary"
                        else "$.itemsView"
                    )
                ),
            }
        )
        helper_counts[domain] += 1
    expected_helper_counts = {
        "ReasoningSummary": 6,
        "ThreadListCwdFilter": 3,
        "TurnItemsView": 5,
    }
    if helper_counts != expected_helper_counts or len(helper_records) != 14:
        raise SurfaceError(
            "OperationProductionCoverageHelperMismatch: "
            f"expected exact helper corpus {expected_helper_counts}, got {helper_counts}"
        )

    open_value_specs = {
        "ApprovalsReviewer": (
            ("client_request", "ClientRequest", "method", "thread/start"),
            "Both",
            "$.approvalsReviewer",
        ),
        "Personality": (
            ("client_request", "ClientRequest", "method", "thread/start"),
            "Encode",
            "$.personality",
        ),
        "SandboxMode": (
            ("client_request", "ClientRequest", "method", "thread/start"),
            "Encode",
            "$.sandbox",
        ),
        "SortDirection": (
            ("client_request", "ClientRequest", "method", "thread/list"),
            "Encode",
            "$.sortDirection",
        ),
        "ThreadGoalStatus": (
            ("client_request", "ClientRequest", "method", "thread/goal/set"),
            "Both",
            "$.status",
        ),
        "ThreadSortKey": (
            ("client_request", "ClientRequest", "method", "thread/list"),
            "Encode",
            "$.sortKey",
        ),
        "ThreadSourceKind": (
            ("client_request", "ClientRequest", "method", "thread/list"),
            "Encode",
            "$.sourceKinds[0]",
        ),
        "ThreadStartSource": (
            ("client_request", "ClientRequest", "method", "thread/start"),
            "Encode",
            "$.sessionStartSource",
        ),
        "ThreadUnsubscribeStatus": (
            (
                "client_request",
                "ClientRequest",
                "method",
                "thread/unsubscribe",
            ),
            "Decode",
            "$.status",
        ),
        "TurnStatus": (
            ("client_request", "ClientRequest", "method", "turn/start"),
            "Decode",
            "$.status",
        ),
    }
    open_value_records: list[dict[str, Any]] = []
    open_value_counts = {domain: 0 for domain in sorted(open_value_specs)}
    for fixture in indexed_records:
        if not isinstance(fixture, dict):
            continue
        fixture_id = str(fixture.get("id", ""))
        if not fixture_id.startswith("enum:"):
            continue
        parts = fixture_id.split(":")
        if len(parts) != 3 or parts[1] not in open_value_specs:
            continue
        domain = parts[1]
        role = fixture.get("role")
        if role not in {"open_enum_known_value", "unknown_enum_value"}:
            raise SurfaceError(
                "OperationProductionCoverageAggregateValueMismatch: "
                f"unsupported {domain} fixture role {role!r}"
            )
        owner_key, direction, field_path = open_value_specs[domain]
        open_value_records.append(
            {
                "id": fixture_id,
                "domain": domain,
                "branch": parts[2],
                "owner_surface_key": _canonical_surface_key_object(
                    owner_key
                ),
                "runtime_target": specs[owner_key]["runtime_target"],
                "direction": direction,
                "role": role,
                "file": fixture["file"],
                "file_sha256": fixture["file_sha256"],
                "expected_intrinsic_codes": [
                    *(
                        ["Encoded", "Decoded"]
                        if direction == "Both"
                        else [
                            "Encoded"
                            if direction == "Encode"
                            else "Decoded"
                        ]
                    ),
                    *(
                        ["UnknownEnumValue"]
                        if role == "unknown_enum_value"
                        and direction in {"Decode", "Both"}
                        else []
                    ),
                ],
                "expected_field_path": (
                    field_path
                    if role == "unknown_enum_value"
                    and direction in {"Decode", "Both"}
                    else None
                ),
            }
        )
        open_value_counts[domain] += 1
    expected_open_value_counts = {
        "ApprovalsReviewer": 5,
        "Personality": 5,
        "SandboxMode": 5,
        "SortDirection": 4,
        "ThreadGoalStatus": 8,
        "ThreadSortKey": 5,
        "ThreadSourceKind": 12,
        "ThreadStartSource": 4,
        "ThreadUnsubscribeStatus": 5,
        "TurnStatus": 6,
    }
    if (
        open_value_counts != expected_open_value_counts
        or len(open_value_records) != 59
    ):
        raise SurfaceError(
            "OperationProductionCoverageAggregateValueMismatch: "
            "expected the exact 59-record non-ThreadActiveFlag operation open-value corpus "
            f"{expected_open_value_counts}, got {open_value_counts}"
        )
    aggregate_value_records = sorted(
        [*helper_records, *open_value_records],
        key=lambda record: record["id"],
    )
    if len(aggregate_value_records) != 73 or len(
        {record["id"] for record in aggregate_value_records}
    ) != 73:
        raise SurfaceError(
            "OperationProductionCoverageAggregateValueMismatch: "
            "expected exactly 73 unique aggregate/value fixture records"
        )

    _validate_a1_1_registered_coverage_tests(
        repo_root,
        (
            (
                "CodexA11OperationResultCorpusTest",
                "tests/component/codex/CodexA11OperationResultCorpusTest.cpp",
                False,
            ),
            (
                "CodexA11OperationAggregateValueCorpusTest",
                "tests/component/codex/CodexA11OperationAggregateValueCorpusTest.cpp",
                False,
            ),
            (
                "CodexConversationB4NestedCodecTest",
                "tests/component/codex/CodexConversationB4NestedCodecTest.cpp",
                True,
            ),
        ),
        "CODEX_A1_OPERATION_PRODUCTION_COVERAGE",
        "OperationProductionCoverage",
    )
    source_records = [
        _a1_1_production_coverage_source_record(
            repo_root,
            relative,
            "OperationProductionCoverageSourceMissing",
        )
        for relative in OPERATION_PRODUCTION_COVERAGE_SOURCES
    ]
    try:
        index_data = fixture_index_path.read_bytes()
    except OSError as error:
        raise SurfaceError(
            "OperationProductionCoverageFixtureIndexMismatch: "
            f"unable to read {fixture_index_path}: {error}"
        ) from error
    if load_json(fixture_index_path) != fixture_index:
        raise SurfaceError(
            "OperationProductionCoverageFixtureIndexMismatch: "
            "in-memory fixture index differs from the pinned index path"
        )

    return {
        "format_version": 1,
        "codex_version": CODEX_VERSION,
        "generated_notice": (
            "Generated by tools/codex/app_server_surface.py "
            "operation-production-coverage; do not edit."
        ),
        "authority_note": (
            "This is checked production-coverage evidence consumed by the "
            "registered C++ operation-result and B4 conversation-union corpus "
            "tests plus the registry generator; it is not a runtime "
            "disposition or dispatch registry."
        ),
        "fixture_index": {
            "path": fixture_index_path.relative_to(repo_root).as_posix(),
            "bytes": len(index_data),
            "sha256": sha256_bytes(index_data),
        },
        "registered_tests": [
            {
                "ctest_name": "CodexA11OperationResultCorpusTest",
                "source": "tests/component/codex/CodexA11OperationResultCorpusTest.cpp",
            },
            {
                "ctest_name": "CodexA11OperationAggregateValueCorpusTest",
                "source": "tests/component/codex/CodexA11OperationAggregateValueCorpusTest.cpp",
            },
            {
                "ctest_name": "CodexConversationB4NestedCodecTest",
                "source": "tests/component/codex/CodexConversationB4NestedCodecTest.cpp",
            },
        ],
        "source_records": source_records,
        "counts": {
            "operations": 22,
            "unit_result_roots": 7,
            "concrete_result_roots": 15,
            "positive_result_records": 403,
            "negative_result_records": 763,
            "records": 1166,
            "roles": expected_role_counts,
            "b4_conversation_union_identities": 16,
            "b4_conversation_union_records": 60,
            "b4_thread_active_flag_records": 4,
            "b4_conversation_roles": expected_conversation_role_counts,
            "b4_thread_active_flag_roles": expected_open_enum_role_counts,
            "operation_aggregate_value_records": 73,
            "operation_helper_families": expected_helper_counts,
            "operation_open_value_families": expected_open_value_counts,
        },
        "encoded_protocol_opaque_paths": [
            "#/definitions/v2/ThreadForkParams/properties/config/additionalProperties",
            "#/definitions/v2/ThreadInjectItemsParams/properties/items/items",
            "#/definitions/v2/ThreadResumeParams/properties/config/additionalProperties",
            "#/definitions/v2/ThreadStartParams/properties/config/additionalProperties",
            "#/definitions/v2/TurnStartParams/properties/outputSchema",
        ],
        "operations": operations,
        "records": sorted(records, key=lambda record: record["id"]),
        "b4_conversation_unions": conversation_unions,
        "b4_conversation_records": sorted(
            conversation_records, key=lambda record: record["id"]
        ),
        "b4_thread_active_flag_records": sorted(
            open_enum_records, key=lambda record: record["id"]
        ),
        "operation_aggregate_value_records": aggregate_value_records,
    }


def generate_notification_production_coverage(
    manifest: dict[str, Any],
    evidence: dict[str, Any],
    fixture_index: dict[str, Any],
    repo_root: Path = DEFAULT_REPO_ROOT,
    fixture_index_path: Path = DEFAULT_A1_FIXTURE_INDEX,
) -> dict[str, Any]:
    """Generate checked A1.1 notification decode/no-drop coverage evidence."""

    assignments = assignment_by_key(manifest, evidence["assignments"])
    expected_keys = {
        key
        for key, assignment in assignments.items()
        if key[0] == "server_notification"
        and assignment.get("slice") == "A1.1"
        and assignment.get("classification") == "StablePublicRoot"
        and assignment.get("module") == "ThreadsTurnsSessions"
        and assignment.get("stability") == "stable"
    }
    if len(expected_keys) != 37 or not expected_keys <= set(
        SERVER_NOTIFICATION_CODECS
    ):
        raise SurfaceError(
            "NotificationProductionCoverageAssignmentMismatch: "
            "expected the exact 37 stable A1.1 server notifications"
        )

    section = fixture_index.get("a1_1_notifications")
    if not isinstance(section, dict):
        raise SurfaceError(
            "NotificationProductionCoverageFixtureIndexMismatch: "
            "fixture index lacks a1_1_notifications"
        )
    indexed_keys = {
        surface_key(key)
        for key in section.get("assignment_derived_keys", [])
        if isinstance(key, dict)
    }
    if indexed_keys != expected_keys:
        raise SurfaceError(
            "NotificationProductionCoverageFixtureIndexMismatch: "
            "assignment-derived fixture keys do not equal the exact A1.1 set"
        )
    indexed_schema_coverage = section.get("indexed_schema_coverage")
    root_fixture_plan = section.get("root_fixture_plan")
    negative_coverage = section.get("negative_coverage")
    if (
        not isinstance(indexed_schema_coverage, dict)
        or set(indexed_schema_coverage) != {":".join(key) for key in expected_keys}
        or not isinstance(root_fixture_plan, dict)
        or set(root_fixture_plan) != {":".join(key) for key in expected_keys}
        or not isinstance(negative_coverage, dict)
    ):
        raise SurfaceError(
            "NotificationProductionCoverageFixtureIndexMismatch: "
            "indexed schema coverage or root fixture plan is incomplete"
        )

    fixture_records = fixture_index.get("fixtures")
    if not isinstance(fixture_records, list):
        raise SurfaceError(
            "NotificationProductionCoverageFixtureIndexMismatch: "
            "fixture records are absent"
        )
    fixtures_by_id: dict[str, dict[str, Any]] = {}
    for fixture in fixture_records:
        if not isinstance(fixture, dict) or not isinstance(
            fixture.get("id"), str
        ):
            continue
        fixture_id = fixture["id"]
        if fixture_id in fixtures_by_id:
            raise SurfaceError(
                "NotificationProductionCoverageDuplicateRecord: "
                f"duplicate fixture id {fixture_id}"
            )
        fixtures_by_id[fixture_id] = fixture

    type_closure = evidence.get("type_closure")
    if not isinstance(type_closure, dict):
        raise SurfaceError(
            "NotificationProductionCoverageTypeClosureMismatch: "
            "A1.1 type closure is absent"
        )

    def normalize_schema_path(path: str) -> str:
        return path.replace("#/definitions/v2/", "#/definitions/")

    closure_paths: dict[str, dict[str, Any]] = {}
    for mapping in type_closure.get("schema_paths", []):
        if not isinstance(mapping, dict) or not isinstance(
            mapping.get("schema_path"), str
        ):
            continue
        path = normalize_schema_path(mapping["schema_path"])
        if path in closure_paths and closure_paths[path] != mapping:
            raise SurfaceError(
                "NotificationProductionCoverageTypeClosureMismatch: "
                f"conflicting C++ mapping for {path}"
            )
        closure_paths[path] = mapping
    opaque_paths = {
        normalize_schema_path(record["schema_path"]): record
        for record in type_closure.get("protocol_defined_opaque_json", [])
        if isinstance(record, dict)
        and isinstance(record.get("schema_path"), str)
    }
    expected_notification_opaque_paths = {
        "#/definitions/ThreadRealtimeItemAddedNotification/properties/item",
        "#/definitions/TurnModerationMetadataNotification/properties/metadata",
    }
    if not expected_notification_opaque_paths <= set(opaque_paths):
        raise SurfaceError(
            "NotificationProductionCoverageTypeClosureMismatch: "
            "the two protocol-defined notification opaque paths are absent"
        )

    role_fields = {
        "base": "base_fixture_id",
        "optional_omitted": "optional_omitted_fixture_ids",
        "nullable_null": "nullable_null_fixture_ids",
        "required_nullable_null": "required_nullable_null_fixture_ids",
        "missing_required": "missing_required_fixture_ids",
        "wrong_type": "wrong_type_fixture_ids",
    }
    expected_role_counts = {
        "base": 37,
        "missing_required": 279,
        "nullable_null": 57,
        "optional_omitted": 65,
        "required_nullable_null": 2,
        "wrong_type": 358,
    }
    role_counts = {role: 0 for role in sorted(role_fields)}
    records: list[dict[str, Any]] = []
    notifications: list[dict[str, Any]] = []
    seen_record_ids: set[str] = set()

    def is_recoverable_nested_malformed(
        key: tuple[str, str, str, str], fixture_id: str
    ) -> bool:
        """Return whether a known outer aggregate preserves a nested warning."""

        marker = (
            ":missing-required:"
            if ":missing-required:" in fixture_id
            else ":wrong-type:"
        )
        mutation_path = fixture_id.partition(marker)[2]
        method = key[3]
        if method in {"item/started", "item/completed"}:
            return mutation_path.startswith("params-item-")
        if method == "thread/started":
            return mutation_path.startswith(
                "params-thread-turns-0-items-0-"
            ) or mutation_path.startswith(
                "params-thread-turns-0-error-codexerrorinfo"
            )
        if method in {"turn/started", "turn/completed"}:
            return mutation_path.startswith(
                "params-turn-items-0-"
            ) or mutation_path.startswith(
                "params-turn-error-codexerrorinfo"
            )
        return False

    for key in sorted(expected_keys):
        compact_key = ":".join(key)
        coverage = indexed_schema_coverage[compact_key]
        plan = root_fixture_plan[compact_key]
        if (
            not isinstance(coverage, dict)
            or coverage.get("directions_exercised") != ["Decode"]
            or coverage.get("schema_direction_coverage") is not True
            or not isinstance(plan, dict)
        ):
            raise SurfaceError(
                "NotificationProductionCoverageDirectionMismatch: "
                f"{compact_key} lacks exact Decode direction evidence"
            )
        schema_facts = coverage.get("schema_fixture_facts")
        expected_facts = {
            "nullable_semantics_exercised",
            "optional_omitted_exercised",
            "optional_present_exercised",
            "reachable_union_alternatives_exercised",
            "schema_properties_exercised",
        }
        if (
            not isinstance(schema_facts, dict)
            or set(schema_facts) != expected_facts
            or not all(schema_facts.values())
        ):
            raise SurfaceError(
                "NotificationProductionCoverageFixtureIndexMismatch: "
                f"{compact_key} lacks complete supported schema facts"
            )

        property_mappings: list[dict[str, Any]] = []
        declared_opaque: list[dict[str, Any]] = []
        for original_path in coverage.get("property_schema_paths", []):
            if not isinstance(original_path, str):
                raise SurfaceError(
                    "NotificationProductionCoverageTypeClosureMismatch: "
                    f"{compact_key} has a non-string schema path"
                )
            if original_path.startswith("#/oneOf/") and original_path.endswith(
                ("/properties/method", "/properties/params")
            ):
                continue
            path = normalize_schema_path(original_path)
            mapping = closure_paths.get(path)
            opaque = opaque_paths.get(path)
            if mapping is None and opaque is None:
                raise SurfaceError(
                    "NotificationProductionCoverageKnownFieldDropped: "
                    f"{compact_key} has no public/private mapping for {path}"
                )
            if mapping is not None:
                property_mappings.append(
                    {
                        "schema_path": path,
                        "cpp_mapping": mapping["cpp_mapping"],
                        "presence_model": mapping.get("presence_model"),
                        "directionality": mapping.get("directionality"),
                    }
                )
            else:
                declared_opaque.append(
                    {
                        "schema_path": path,
                        "reason": opaque["reason"],
                    }
                )

        notification_fixture_ids: dict[str, list[str]] = {}
        for role, field in role_fields.items():
            raw_ids = plan.get(field)
            ids = [raw_ids] if role == "base" else raw_ids
            if not isinstance(ids, list) or not all(
                isinstance(fixture_id, str) for fixture_id in ids
            ):
                raise SurfaceError(
                    "NotificationProductionCoverageFixtureIndexMismatch: "
                    f"{compact_key} has invalid {field}"
                )
            notification_fixture_ids[role] = list(ids)
            role_counts[role] += len(ids)
            for fixture_id in ids:
                if fixture_id in seen_record_ids:
                    raise SurfaceError(
                        "NotificationProductionCoverageDuplicateRecord: "
                        f"duplicate selected fixture {fixture_id}"
                    )
                fixture = fixtures_by_id.get(fixture_id)
                if fixture is None:
                    raise SurfaceError(
                        "NotificationProductionCoverageMissingRecord: "
                        f"missing indexed fixture {fixture_id}"
                    )
                seen_record_ids.add(fixture_id)
                envelope_rejected = fixture_id.endswith(
                    (":missing-required:method", ":wrong-type:method")
                )
                if role in {
                    "base",
                    "optional_omitted",
                    "nullable_null",
                    "required_nullable_null",
                }:
                    expected_codes = ["Decoded"]
                elif envelope_rejected:
                    expected_codes = ["ProtocolEnvelopeRejected"]
                elif is_recoverable_nested_malformed(key, fixture_id):
                    expected_codes = [
                        "Decoded",
                        "MalformedKnownPayload",
                        "ProtocolWarning",
                    ]
                else:
                    expected_codes = [
                        "MalformedKnownPayload",
                        "ProtocolWarning",
                    ]
                records.append(
                    {
                        "id": fixture_id,
                        "surface_key": _canonical_surface_key_object(key),
                        "runtime_target": SERVER_NOTIFICATION_CODECS[key][0],
                        "role": role,
                        "file": fixture["file"],
                        "file_sha256": fixture["file_sha256"],
                        "expected_intrinsic_codes": expected_codes,
                    }
                )

        notifications.append(
            {
                "surface_key": _canonical_surface_key_object(key),
                "runtime_target": SERVER_NOTIFICATION_CODECS[key][0],
                "payload_type_identity": SERVER_NOTIFICATION_CODECS[key][1],
                "event_alternative": SERVER_NOTIFICATION_EVENT_ALTERNATIVES_BY_METHOD[
                    key[3]
                ],
                "a1_1_conversation_domain": True,
                "directions_exercised": ["Decode"],
                "base_fixture_id": plan["base_fixture_id"],
                "fixture_ids_by_role": notification_fixture_ids,
                "represented_property_mappings": sorted(
                    property_mappings, key=lambda record: record["schema_path"]
                ),
                "protocol_defined_opaque_json": sorted(
                    declared_opaque, key=lambda record: record["schema_path"]
                ),
                "required_reachable_fixture_ids": coverage.get(
                    "required_reachable_fixture_ids", []
                ),
                "schema_fixture_facts": schema_facts,
                "runtime_decoder_matches_registry": True,
                "no_known_schema_fields_dropped": True,
            }
        )

    if role_counts != expected_role_counts or len(records) != 798:
        raise SurfaceError(
            "NotificationProductionCoverageRoleMismatch: "
            f"expected exact roles {expected_role_counts}, got {role_counts}"
        )

    enum_owner_methods = {
        "ModeKind": "thread/settings/updated",
        "RealtimeConversationVersion": "thread/realtime/started",
        "TurnPlanStepStatus": "turn/plan/updated",
    }
    enum_records: list[dict[str, Any]] = []
    open_enums = negative_coverage.get("open_string_enums")
    if not isinstance(open_enums, dict) or set(open_enums) != set(
        enum_owner_methods
    ):
        raise SurfaceError(
            "NotificationProductionCoverageOpenEnumMismatch: "
            "expected exact ModeKind/RealtimeConversationVersion/TurnPlanStepStatus coverage"
        )
    for family, owner_method in sorted(enum_owner_methods.items()):
        enum = open_enums[family]
        owner_key = next(
            key for key in expected_keys if key[3] == owner_method
        )
        ids_and_codes = [
            *[
                (fixture_id, ["Decoded"])
                for fixture_id in enum["known_value_fixture_ids"]
            ],
            (
                enum["future_value_fixture_id"],
                ["Decoded", "UnknownEnumValue", "ForwardCompatibility"],
            ),
            (
                enum["empty_value_fixture_id"],
                ["Decoded", "UnknownEnumValue", "ForwardCompatibility"],
            ),
        ]
        for fixture_id, expected_codes in ids_and_codes:
            fixture = fixtures_by_id.get(fixture_id)
            if fixture is None:
                raise SurfaceError(
                    "NotificationProductionCoverageMissingRecord: "
                    f"missing open-enum fixture {fixture_id}"
                )
            enum_records.append(
                {
                    "id": fixture_id,
                    "family": family,
                    "owner_surface_key": _canonical_surface_key_object(
                        owner_key
                    ),
                    "runtime_target": SERVER_NOTIFICATION_CODECS[owner_key][
                        0
                    ],
                    "file": fixture["file"],
                    "file_sha256": fixture["file_sha256"],
                    "expected_intrinsic_codes": expected_codes,
                }
            )
    if len(enum_records) != 13:
        raise SurfaceError(
            "NotificationProductionCoverageOpenEnumMismatch: "
            "expected exactly 7 known and 6 future/empty enum records"
        )

    _validate_a1_1_registered_coverage_tests(
        repo_root,
        (
            (
                "CodexA11NotificationCodecTest",
                "tests/component/codex/CodexA11NotificationCodecTest.cpp",
                False,
            ),
        ),
        "CODEX_A1_NOTIFICATION_PRODUCTION_COVERAGE",
        "NotificationProductionCoverage",
    )
    source_records = [
        _a1_1_production_coverage_source_record(
            repo_root,
            relative,
            "NotificationProductionCoverageSourceMissing",
        )
        for relative in NOTIFICATION_PRODUCTION_COVERAGE_SOURCES
    ]
    try:
        index_data = fixture_index_path.read_bytes()
    except OSError as error:
        raise SurfaceError(
            "NotificationProductionCoverageFixtureIndexMismatch: "
            f"unable to read {fixture_index_path}: {error}"
        ) from error
    if load_json(fixture_index_path) != fixture_index:
        raise SurfaceError(
            "NotificationProductionCoverageFixtureIndexMismatch: "
            "in-memory fixture index differs from the pinned index path"
        )

    opaque_records = [
        record
        for record in negative_coverage.get("payload_mutations", {}).get(
            "opaque_exclusions", []
        )
        if isinstance(record, dict)
    ]
    if {
        normalize_schema_path(record["schema_path"])
        for record in opaque_records
    } != expected_notification_opaque_paths:
        raise SurfaceError(
            "NotificationProductionCoverageOpaqueMismatch: "
            "expected the exact two protocol-defined opaque paths"
        )

    return {
        "format_version": 1,
        "codex_version": CODEX_VERSION,
        "generated_notice": (
            "Generated by tools/codex/app_server_surface.py "
            "notification-production-coverage; do not edit."
        ),
        "authority_note": (
            "This checked fixture/type/production association is consumed by "
            "the notification corpus test and canonical registry generator; "
            "it is not a second runtime disposition or dispatch registry."
        ),
        "fixture_index": {
            "path": fixture_index_path.relative_to(repo_root).as_posix(),
            "bytes": len(index_data),
            "sha256": sha256_bytes(index_data),
        },
        "registered_tests": [
            {
                "ctest_name": "CodexA11NotificationCodecTest",
                "source": "tests/component/codex/CodexA11NotificationCodecTest.cpp",
            }
        ],
        "source_records": source_records,
        "counts": {
            "a1_1_server_notifications": 37,
            "descriptor_rows": 39,
            "residual_partial_descriptor_rows": 2,
            "fixture_records": 798,
            "positive_records": 161,
            "negative_records": 637,
            "roles": expected_role_counts,
            "open_enum_records": 13,
            "open_enum_known_records": 7,
            "open_enum_unknown_records": 6,
            "protocol_defined_opaque_paths": 2,
        },
        "notifications": notifications,
        "records": sorted(records, key=lambda record: record["id"]),
        "open_enum_records": sorted(
            enum_records, key=lambda record: record["id"]
        ),
        "protocol_defined_opaque_json": sorted(
            opaque_records, key=lambda record: record["schema_path"]
        ),
    }


def validate_notification_production_coverage(
    document: dict[str, Any],
    manifest: dict[str, Any],
    evidence: dict[str, Any],
    fixture_index: dict[str, Any] | None = None,
    repo_root: Path = DEFAULT_REPO_ROOT,
    fixture_index_path: Path = DEFAULT_A1_FIXTURE_INDEX,
) -> dict[tuple[str, str, str, str], dict[str, Any]]:
    if not isinstance(document, dict):
        raise SurfaceError(
            "NotificationProductionCoverageMissing: evidence is not an object"
        )
    fixture_index = (
        fixture_index
        if fixture_index is not None
        else load_json(fixture_index_path)
    )
    expected = generate_notification_production_coverage(
        manifest,
        evidence,
        fixture_index,
        repo_root,
        fixture_index_path,
    )
    records = document.get("records")
    if not isinstance(records, list):
        raise SurfaceError(
            "NotificationProductionCoverageMissingRecord: records are absent"
        )
    ids = [
        record.get("id") for record in records if isinstance(record, dict)
    ]
    if len(ids) != len(set(ids)):
        raise SurfaceError(
            "NotificationProductionCoverageDuplicateRecord: duplicate record id"
        )
    expected_records = {record["id"]: record for record in expected["records"]}
    actual_records = {
        record["id"]: record
        for record in records
        if isinstance(record, dict) and isinstance(record.get("id"), str)
    }
    missing = sorted(set(expected_records) - set(actual_records))
    if missing:
        raise SurfaceError(
            "NotificationProductionCoverageMissingRecord: "
            f"missing {missing[0]}"
        )
    stale = sorted(set(actual_records) - set(expected_records))
    if stale:
        raise SurfaceError(
            "NotificationProductionCoverageStaleRecord: "
            f"unexpected {stale[0]}"
        )
    for fixture_id, expected_record in expected_records.items():
        actual = actual_records[fixture_id]
        if actual.get("runtime_target") != expected_record["runtime_target"]:
            raise SurfaceError(
                "NotificationProductionCoverageTargetMismatch: "
                f"{fixture_id} has the wrong target"
            )
        if actual.get("expected_intrinsic_codes") != expected_record[
            "expected_intrinsic_codes"
        ]:
            raise SurfaceError(
                "NotificationProductionCoverageOutcomeMismatch: "
                f"{fixture_id} falsely claims a production outcome"
            )

    notifications = document.get("notifications")
    if not isinstance(notifications, list) or len(notifications) != 37:
        raise SurfaceError(
            "NotificationProductionCoverageMissingNotification: "
            "expected exactly 37 notification rows"
        )
    result: dict[tuple[str, str, str, str], dict[str, Any]] = {}
    for notification in notifications:
        key = surface_key(notification["surface_key"])
        if key in result:
            raise SurfaceError(
                "NotificationProductionCoverageDuplicateNotification: "
                f"duplicate {key}"
            )
        result[key] = notification
    if set(result) != {
        surface_key(record["surface_key"])
        for record in expected["notifications"]
    }:
        raise SurfaceError(
            "NotificationProductionCoverageMissingNotification: "
            "notification identity set differs from generated evidence"
        )
    if document != expected:
        raise SurfaceError(
            "StaleNotificationProductionCoverage: checked evidence differs "
            "from the indexed corpus, type closure, and hashed production/test sources"
        )
    return result


def validate_operation_production_coverage(
    document: dict[str, Any],
    manifest: dict[str, Any],
    evidence: dict[str, Any],
    fixture_index: dict[str, Any] | None = None,
    repo_root: Path = DEFAULT_REPO_ROOT,
    fixture_index_path: Path = DEFAULT_A1_FIXTURE_INDEX,
) -> dict[tuple[str, str, str, str], dict[str, Any]]:
    if not isinstance(document, dict):
        raise SurfaceError(
            "OperationProductionCoverageMissing: evidence is not an object"
        )
    fixture_index = (
        fixture_index
        if fixture_index is not None
        else load_json(fixture_index_path)
    )
    expected = generate_operation_production_coverage(
        manifest,
        evidence,
        fixture_index,
        repo_root,
        fixture_index_path,
    )
    records = document.get("records")
    if not isinstance(records, list):
        raise SurfaceError(
            "OperationProductionCoverageMissingRecord: records are absent"
        )
    ids = [record.get("id") for record in records if isinstance(record, dict)]
    if len(ids) != len(set(ids)):
        raise SurfaceError(
            "OperationProductionCoverageDuplicateRecord: duplicate record id"
        )
    expected_by_id = {
        record["id"]: record for record in expected["records"]
    }
    actual_by_id = {
        record["id"]: record
        for record in records
        if isinstance(record, dict) and isinstance(record.get("id"), str)
    }
    missing = sorted(set(expected_by_id) - set(actual_by_id))
    if missing:
        raise SurfaceError(
            "OperationProductionCoverageMissingRecord: "
            f"missing {missing[0]}"
        )
    stale = sorted(set(actual_by_id) - set(expected_by_id))
    if stale:
        raise SurfaceError(
            "OperationProductionCoverageStaleRecord: "
            f"unexpected {stale[0]}"
        )
    for fixture_id, expected_record in expected_by_id.items():
        actual = actual_by_id[fixture_id]
        if actual.get("runtime_target") != expected_record["runtime_target"]:
            raise SurfaceError(
                "OperationProductionCoverageTargetMismatch: "
                f"{fixture_id} has the wrong production target"
            )
        if actual.get("expected_intrinsic_codes") != expected_record[
            "expected_intrinsic_codes"
        ]:
            raise SurfaceError(
                "OperationProductionCoverageOutcomeMismatch: "
                f"{fixture_id} falsely claims a production outcome"
            )
    operations = document.get("operations")
    if not isinstance(operations, list) or len(operations) != 22:
        raise SurfaceError(
            "OperationProductionCoverageMissingOperation: "
            "expected exactly 22 operation rows"
        )
    operation_result: dict[
        tuple[str, str, str, str], dict[str, Any]
    ] = {}
    for operation in operations:
        key = surface_key(operation["surface_key"])
        if key in operation_result:
            raise SurfaceError(
                "OperationProductionCoverageDuplicateOperation: "
                f"duplicate {key}"
            )
        operation_result[key] = operation

    expected_conversation_records = {
        record["id"]: record
        for record in expected["b4_conversation_records"]
    }
    conversation_records = document.get("b4_conversation_records")
    if not isinstance(conversation_records, list):
        raise SurfaceError(
            "ConversationProductionCoverageMissingRecord: "
            "B4 conversation records are absent"
        )
    conversation_ids = [
        record.get("id")
        for record in conversation_records
        if isinstance(record, dict)
    ]
    if len(conversation_ids) != len(set(conversation_ids)):
        raise SurfaceError(
            "ConversationProductionCoverageDuplicateRecord: "
            "duplicate B4 conversation record id"
        )
    actual_conversation_records = {
        record["id"]: record
        for record in conversation_records
        if isinstance(record, dict)
        and isinstance(record.get("id"), str)
    }
    missing_conversation = sorted(
        set(expected_conversation_records)
        - set(actual_conversation_records)
    )
    if missing_conversation:
        raise SurfaceError(
            "ConversationProductionCoverageMissingRecord: "
            f"missing {missing_conversation[0]}"
        )
    stale_conversation = sorted(
        set(actual_conversation_records)
        - set(expected_conversation_records)
    )
    if stale_conversation:
        raise SurfaceError(
            "ConversationProductionCoverageStaleRecord: "
            f"unexpected {stale_conversation[0]}"
        )
    for fixture_id, expected_record in expected_conversation_records.items():
        actual = actual_conversation_records[fixture_id]
        if actual.get("runtime_target") != expected_record["runtime_target"]:
            raise SurfaceError(
                "ConversationProductionCoverageTargetMismatch: "
                f"{fixture_id} has the wrong production target"
            )
        if actual.get("expected_intrinsic_codes") != expected_record[
            "expected_intrinsic_codes"
        ]:
            raise SurfaceError(
                "ConversationProductionCoverageOutcomeMismatch: "
                f"{fixture_id} falsely claims a production outcome"
            )

    expected_open_records = {
        record["id"]: record
        for record in expected["b4_thread_active_flag_records"]
    }
    open_records = document.get("b4_thread_active_flag_records")
    if not isinstance(open_records, list):
        raise SurfaceError(
            "ConversationProductionCoverageMissingRecord: "
            "ThreadActiveFlag records are absent"
        )
    open_ids = [
        record.get("id")
        for record in open_records
        if isinstance(record, dict)
    ]
    if len(open_ids) != len(set(open_ids)):
        raise SurfaceError(
            "ConversationProductionCoverageDuplicateRecord: "
            "duplicate ThreadActiveFlag record id"
        )
    actual_open_records = {
        record["id"]: record
        for record in open_records
        if isinstance(record, dict)
        and isinstance(record.get("id"), str)
    }
    missing_open = sorted(set(expected_open_records) - set(actual_open_records))
    if missing_open:
        raise SurfaceError(
            "ConversationProductionCoverageMissingRecord: "
            f"missing {missing_open[0]}"
        )
    stale_open = sorted(set(actual_open_records) - set(expected_open_records))
    if stale_open:
        raise SurfaceError(
            "ConversationProductionCoverageStaleRecord: "
            f"unexpected {stale_open[0]}"
        )
    for fixture_id, expected_record in expected_open_records.items():
        actual = actual_open_records[fixture_id]
        if actual.get("runtime_target") != expected_record["runtime_target"]:
            raise SurfaceError(
                "ConversationProductionCoverageTargetMismatch: "
                f"{fixture_id} has the wrong production target"
            )
        if actual.get("expected_intrinsic_codes") != expected_record[
            "expected_intrinsic_codes"
        ]:
            raise SurfaceError(
                "ConversationProductionCoverageOutcomeMismatch: "
                f"{fixture_id} falsely claims a production outcome"
            )

    conversation_unions = document.get("b4_conversation_unions")
    if (
        not isinstance(conversation_unions, list)
        or len(conversation_unions) != 16
    ):
        raise SurfaceError(
            "ConversationProductionCoverageMissingUnion: "
            "expected exactly 16 B4 conversation-union rows"
        )
    conversation_result: dict[
        tuple[str, str, str, str], dict[str, Any]
    ] = {}
    for union in conversation_unions:
        key = surface_key(union["surface_key"])
        if key in conversation_result:
            raise SurfaceError(
                "ConversationProductionCoverageDuplicateUnion: "
                f"duplicate {key}"
            )
        conversation_result[key] = union

    expected_aggregate_value_records = {
        record["id"]: record
        for record in expected["operation_aggregate_value_records"]
    }
    aggregate_value_records = document.get(
        "operation_aggregate_value_records"
    )
    if not isinstance(aggregate_value_records, list):
        raise SurfaceError(
            "OperationProductionCoverageMissingAggregateValue: "
            "operation aggregate/value records are absent"
        )
    aggregate_value_ids = [
        record.get("id")
        for record in aggregate_value_records
        if isinstance(record, dict)
    ]
    if len(aggregate_value_ids) != len(set(aggregate_value_ids)):
        raise SurfaceError(
            "OperationProductionCoverageDuplicateAggregateValue: "
            "duplicate operation aggregate/value record id"
        )
    actual_aggregate_value_records = {
        record["id"]: record
        for record in aggregate_value_records
        if isinstance(record, dict)
        and isinstance(record.get("id"), str)
    }
    missing_aggregate_values = sorted(
        set(expected_aggregate_value_records)
        - set(actual_aggregate_value_records)
    )
    if missing_aggregate_values:
        raise SurfaceError(
            "OperationProductionCoverageMissingAggregateValue: "
            f"missing {missing_aggregate_values[0]}"
        )
    stale_aggregate_values = sorted(
        set(actual_aggregate_value_records)
        - set(expected_aggregate_value_records)
    )
    if stale_aggregate_values:
        raise SurfaceError(
            "OperationProductionCoverageStaleAggregateValue: "
            f"unexpected {stale_aggregate_values[0]}"
        )
    for fixture_id, expected_record in expected_aggregate_value_records.items():
        actual = actual_aggregate_value_records[fixture_id]
        if actual.get("runtime_target") != expected_record["runtime_target"]:
            raise SurfaceError(
                "OperationProductionCoverageAggregateValueTargetMismatch: "
                f"{fixture_id} has the wrong production target"
            )
        if actual.get("expected_intrinsic_codes") != expected_record[
            "expected_intrinsic_codes"
        ]:
            raise SurfaceError(
                "OperationProductionCoverageAggregateValueOutcomeMismatch: "
                f"{fixture_id} falsely claims a production outcome"
            )

    if document != expected:
        raise SurfaceError(
            "StaleOperationProductionCoverage: checked evidence differs "
            "from the indexed corpus and hashed production/test sources"
        )
    return {
        "operations": operation_result,
        "b4_conversation_unions": conversation_result,
    }


def generate_item_codec_descriptor_data(
    manifest: dict[str, Any],
    schema_root: Path,
    domain: str,
    target_prefix: str,
    macro: str,
    expected_count: int,
    evidence: dict[str, Any] | None = None,
) -> str:
    """Generate one direction-specific item target family from exact registry keys."""

    evidence = evidence if evidence is not None else load_a1_registry_evidence()
    assignments = assignment_by_key(manifest, evidence["assignments"])
    expected_keys = {
        key
        for key, assignment in assignments.items()
        if key[0] == "item_discriminator"
        and key[1] == domain
        and assignment.get("slice") == "A1.1"
        and assignment.get("classification") == "StableItemRoot"
        and assignment.get("stability") == "stable"
    }
    targets = {
        key: target
        for key, target in RUNTIME_TARGETS.items()
        if key[0] == "item_discriminator" and key[1] == domain
    }
    if (
        set(targets) != expected_keys
        or len(targets) != expected_count
        or any(not target.startswith(target_prefix + "::") for target in targets.values())
        or len(set(targets.values())) != expected_count
    ):
        raise SurfaceError(
            "ItemCodecDescriptorAssignmentMismatch: "
            f"{domain} must map exactly {expected_count} stable A1.1 item identities "
            f"to unique {target_prefix} targets"
        )

    manifest_entries = {
        surface_key(entry): entry for entry in manifest.get("entries", [])
    }
    lines = [
        "// Generated by tools/codex/app_server_surface.py item-descriptors; do not edit.",
        "// Exact keys remain subordinate to ProtocolSurfaceRegistryData.inc.",
        "// Item shape and direction are private codec metadata, not production dispositions.",
    ]
    for key in sorted(expected_keys):
        entry = manifest_entries.get(key)
        if entry is None or entry.get("stability") != "stable":
            raise SurfaceError(
                "ItemCodecDescriptorAssignmentMismatch: "
                f"missing stable manifest row for {key}"
            )
        branch = _conversation_union_schema_branch(entry, schema_root)
        _validate_conversation_union_descriptor_shape(
            entry, branch, "ConversationUnionCodecShape::InternallyTaggedObject"
        )
        arguments = (
            CPP_CATEGORIES[key[0]],
            cpp_string(key[1]),
            cpp_string(key[2]),
            cpp_string(key[3]),
            targets[key],
            "ConversationUnionCodecShape::InternallyTaggedObject",
            "ConversationUnionCodecDirection::DecodeOnly",
        )
        lines.append(f"{macro}(" + ", ".join(arguments) + ")")
    return "\n".join(lines) + "\n"


def split_cpp_arguments(payload: str) -> list[str]:
    arguments: list[str] = []
    start = 0
    quote: str | None = None
    escaped = False
    nesting = 0
    for index, character in enumerate(payload):
        if quote is not None:
            if escaped:
                escaped = False
            elif character == "\\":
                escaped = True
            elif character == quote:
                quote = None
            continue
        if character == '"':
            quote = character
        elif character in "({[":
            nesting += 1
        elif character in ")}]":
            nesting -= 1
        elif character == "," and nesting == 0:
            arguments.append(payload[start:index].strip())
            start = index + 1
    arguments.append(payload[start:].strip())
    return arguments


def parse_schema_completeness_argument(argument: str, path: Path, line_number: int) -> dict[str, bool]:
    prefix = "schemaCompletenessEvidence("
    if not argument.startswith(prefix) or not argument.endswith(")"):
        raise SurfaceError(
            f"invalid schema completeness evidence at {path}:{line_number}"
        )
    values = split_cpp_arguments(argument[len(prefix) : -1])
    if len(values) != len(COMPLETENESS_EVIDENCE_FIELDS) or any(
        value not in {"true", "false"} for value in values
    ):
        raise SurfaceError(
            f"invalid schema completeness evidence at {path}:{line_number}"
        )
    return {
        field: value == "true"
        for field, value in zip(COMPLETENESS_EVIDENCE_FIELDS, values, strict=True)
    }


def parse_registry_data(path: Path) -> list[dict[str, Any]]:
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as error:
        raise SurfaceError(f"unable to read production registry data {path}: {error}") from error
    return parse_registry_data_text(text, str(path))


def parse_registry_data_text(
    text: str, source: str = "<generated-registry>"
) -> list[dict[str, Any]]:
    reverse_categories = {value: key for key, value in CPP_CATEGORIES.items()}
    entries: list[dict[str, Any]] = []
    lines = text.splitlines()
    prefix = "CODEX_PROTOCOL_SURFACE_ENTRY("
    for line_number, line in enumerate(lines, start=1):
        if not line.startswith(prefix):
            continue
        if not line.endswith(")"):
            raise SurfaceError(f"malformed registry macro at {source}:{line_number}")
        arguments = split_cpp_arguments(line[len(prefix) : -1])
        if len(arguments) != 22:
            raise SurfaceError(
                f"registry macro at {source}:{line_number} has {len(arguments)} arguments, expected 22"
            )
        try:
            category = reverse_categories[arguments[0]]
            domain = json.loads(arguments[1])
            field = json.loads(arguments[2])
            name = json.loads(arguments[3])
        except (KeyError, json.JSONDecodeError) as error:
            raise SurfaceError(f"invalid registry identity at {source}:{line_number}") from error
        entries.append(
            {
                "category": category,
                "domain": domain,
                "discriminator_field": field,
                "name": name,
                "stability": "stable"
                if arguments[4] == "Stability::Stable"
                else "experimental_only",
                "deprecated": arguments[5] == "true",
                "runtime_disposition": arguments[6].removeprefix("RuntimeDisposition::"),
                "typed_status": arguments[7].removeprefix("TypedImplementationStatus::"),
                "backend_status": arguments[8].removeprefix("LayerStatus::"),
                "canonical_state_status": arguments[9].removeprefix("LayerStatus::"),
                "frontend_exposure": arguments[10].removeprefix("FrontendExposure::"),
                "frontend_security": arguments[11].removeprefix(
                    "FrontendSecurityDecision::"
                ),
                "runtime_target": arguments[12],
                "parameter_type_identity": json.loads(arguments[13]),
                "result_type_identity": json.loads(arguments[14]),
                "result_contract_kind": arguments[15].removeprefix(
                    "ResultContractKind::"
                ),
                "association_evidence_kind": arguments[16].removeprefix(
                    "AssociationEvidenceKind::"
                ),
                "association_evidence_key": json.loads(arguments[17]),
                "typed_module": json.loads(arguments[18]),
                "a1_slice": arguments[19].removeprefix("A1Slice::").replace(
                    "_", "."
                ),
                "typed_schema_status": arguments[20].removeprefix(
                    "TypedSchemaStatus::"
                ),
                "schema_completeness": parse_schema_completeness_argument(
                    arguments[21], Path(source), line_number
                ),
            }
        )
    if not entries:
        raise SurfaceError(
            f"production registry data contains no entries: {source}"
        )
    return entries


def registry_by_key(entries: Sequence[dict[str, Any]]) -> dict[tuple[str, str, str, str], dict[str, Any]]:
    result: dict[tuple[str, str, str, str], dict[str, Any]] = {}
    for entry in entries:
        key = (
            entry["category"],
            entry["domain"],
            entry["discriminator_field"],
            entry["name"],
        )
        if key in result:
            raise SurfaceError(f"duplicate production registry entry: {key}")
        result[key] = entry
    return result


def percent(numerator: int, denominator: int) -> str:
    return "n/a" if denominator == 0 else f"{100.0 * numerator / denominator:.1f}%"


def coverage_metrics(
    manifest: dict[str, Any], registry_entries: Sequence[dict[str, Any]]
) -> list[dict[str, Any]]:
    manifest_entries = manifest["entries"]
    registry = registry_by_key(registry_entries)

    def selected(stability: str, predicate: Any) -> list[dict[str, Any]]:
        return [
            entry
            for entry in manifest_entries
            if (
                stability == "experimental_inclusive"
                or entry["stability"] == stability
            )
            and predicate(entry)
        ]

    def registered(entries: Sequence[dict[str, Any]]) -> int:
        return sum(
            (
                entry["category"],
                entry["domain"],
                entry["discriminator_field"],
                entry["name"],
            )
            in registry
            for entry in entries
        )

    stable_all = selected("stable", lambda _: True)
    experimental_all = selected("experimental_inclusive", lambda _: True)
    stable_client_requests = selected(
        "stable", lambda entry: entry["category"] == "client_request"
    )
    stable_inbound = selected(
        "stable",
        lambda entry: entry["category"] in {"server_notification", "server_request"},
    )
    stable_item_delta = selected(
        "stable",
        lambda entry: entry["category"] == "item_discriminator"
        or entry["delta_or_progress"],
    )
    stable_backend_commands = selected(
        "stable",
        lambda entry: entry["category"] == "client_request"
        and entry["name"] != "initialize",
    )
    stable_state_eligible = selected(
        "stable",
        lambda entry: registry[
            (
                entry["category"],
                entry["domain"],
                entry["discriminator_field"],
                entry["name"],
            )
        ]["canonical_state_status"]
        != "NotApplicable",
    )
    stable_frontend_operations = selected(
        "stable",
        lambda entry: (
            entry["category"] == "server_request"
            or (entry["category"] == "client_request" and entry["name"] != "initialize")
        ),
    )
    stable_server_requests = selected(
        "stable", lambda entry: entry["category"] == "server_request"
    )
    stable_frontend_events = selected(
        "stable",
        lambda entry: entry["category"]
        in {"server_notification", "item_discriminator"},
    )

    def implemented(entries: Sequence[dict[str, Any]], field: str, value: str) -> int:
        return sum(
            registry[
                (
                    entry["category"],
                    entry["domain"],
                    entry["discriminator_field"],
                    entry["name"],
                )
            ][field]
            == value
            for entry in entries
        )

    raw_metrics = [
        (
            "Stable upstream inventory registration",
            registered(stable_all),
            len(stable_all),
            "All stable manifest entries present in the production registry.",
        ),
        (
            "Experimental-inclusive upstream inventory registration",
            registered(experimental_all),
            len(experimental_all),
            "Stable plus experimental-only manifest entries present in the production registry.",
        ),
        (
            "Typed wire request/result coverage",
            implemented(stable_client_requests, "typed_status", "Implemented"),
            len(stable_client_requests),
            "Stable client requests; includes typed initialization lifecycle support.",
        ),
        (
            "Typed notification/server-request coverage",
            implemented(stable_inbound, "typed_status", "Implemented"),
            len(stable_inbound),
            "Stable server notifications and server-initiated requests.",
        ),
        (
            "Typed item/delta coverage",
            implemented(stable_item_delta, "typed_status", "Implemented"),
            len(stable_item_delta),
            "Stable item discriminators plus delta/progress notification methods.",
        ),
        (
            "BackendCore command coverage",
            implemented(stable_backend_commands, "backend_status", "Implemented"),
            len(stable_backend_commands),
            "Stable application client requests; initialize is internal lifecycle and excluded.",
        ),
        (
            "Canonical-state/reducer coverage",
            implemented(stable_state_eligible, "canonical_state_status", "Implemented"),
            len(stable_state_eligible),
            "Stable registry entries whose canonical-state status is applicable.",
        ),
        (
            "Frontend Protocol existing-subset exposure coverage",
            implemented(
                stable_frontend_operations,
                "frontend_exposure",
                "ExistingOperationSubset",
            ),
            len(stable_frontend_operations),
            "Stable operations with an existing reviewed normalized v1 subset; not full upstream-method exposure.",
        ),
        (
            "Frontend Protocol normalized event/state subset coverage",
            implemented(
                stable_frontend_events,
                "frontend_exposure",
                "ExistingEventSubset",
            ),
            len(stable_frontend_events),
            "Stable notifications/items with existing typed normalized v1 snapshot or event semantics.",
        ),
        (
            "Frontend Protocol bounded generic extension coverage",
            implemented(
                stable_frontend_events,
                "frontend_exposure",
                "GenericExtension",
            ),
            len(stable_frontend_events),
            "Stable notifications retained with bounded, redacted payloads through the v1 codex.extension contract.",
        ),
        (
            "Frontend Protocol unknown-item metadata subset coverage",
            implemented(
                stable_frontend_events,
                "frontend_exposure",
                "ExistingUnknownItemSubset",
            ),
            len(stable_frontend_events),
            "Stable untyped ThreadItem discriminators exposed only as codexType and "
            "decodingError metadata; raw item payloads are not exposed.",
        ),
        (
            "Generic unknown server-request preservation exposure",
            implemented(
                stable_server_requests,
                "frontend_exposure",
                "GenericUnknownRequest",
            ),
            len(stable_server_requests),
            "Stable server requests currently visible only through the bounded/redacted v1 unknown-request contract.",
        ),
        (
            "Owner-approved frontend-security subset disposition coverage",
            implemented(
                stable_frontend_operations,
                "frontend_security",
                "ExistingOperationSubsetExpansionUnresolved",
            ),
            len(stable_frontend_operations),
            "Existing v1 subsets count; every new or expanded dedicated exposure remains UNRESOLVED.",
        ),
    ]
    return [
        {
            "metric": name,
            "numerator": numerator,
            "denominator": denominator,
            "percentage": percent(numerator, denominator),
            "scope": scope,
        }
        for name, numerator, denominator, scope in raw_metrics
    ]


def render_coverage_document(
    manifest: dict[str, Any],
    registry_entries: Sequence[dict[str, Any]],
    provenance: dict[str, Any],
) -> str:
    metrics = coverage_metrics(manifest, registry_entries)
    counts = manifest["counts"]
    category_names = sorted(
        key for key in counts["experimental_inclusive"] if key != "total"
    )
    upstream = provenance["upstream"]
    lines = [
        "# Codex App Server API coverage",
        "",
        "<!-- Generated by tools/codex/app_server_surface.py docs. Do not edit by hand. -->",
        "",
        f"Authoritative target: `{manifest['codex_version']}` generated from the locally installed Codex binary.",
        f"Starting SNode.C commit: `{manifest['starting_snodec_sha']}`.",
        "",
        f"Upstream source: [{upstream['project']}]({upstream['repository']}) release "
        f"`{upstream['release']['tag']}` at source commit `{upstream['release']['source_commit_sha']}`.",
        f"License: `{upstream['license']['spdx_identifier']}`; the exact upstream license and NOTICE "
        "are retained beside the vendored schemas.",
        "",
        "The vendored JSON Schema is the inventory authority. The private production",
        "`ProtocolSurfaceRegistry` is the local disposition authority and the runtime",
        "method/discriminator dispatch source. Registration is not implementation.",
        "",
        "## Inventory counts",
        "",
        "| Category | Stable | Experimental-only | Experimental-inclusive |",
        "|---|---:|---:|---:|",
    ]
    for category in category_names:
        lines.append(
            f"| `{category}` | {counts['stable'][category]} | "
            f"{counts['experimental_only'][category]} | "
            f"{counts['experimental_inclusive'][category]} |"
        )
    lines.extend(
        [
            f"| **Total** | **{counts['stable']['total']}** | "
            f"**{counts['experimental_only']['total']}** | "
            f"**{counts['experimental_inclusive']['total']}** |",
            "",
            "Twelve server-notification methods are mechanically marked as delta/progress",
            "traits. They remain server-notification entries rather than duplicate inventory",
            "rows. `ThreadItem` contributes 18 item discriminators and `ResponseItem` 16.",
            "",
            "## Separate coverage metrics",
            "",
            "| Metric | Coverage | Percentage | Scope |",
            "|---|---:|---:|---|",
        ]
    )
    for metric in metrics:
        lines.append(
            f"| {metric['metric']} | {metric['numerator']}/{metric['denominator']} | "
            f"{metric['percentage']} | {metric['scope']} |"
        )
    event_denominator = next(
        metric["denominator"]
        for metric in metrics
        if metric["metric"]
        == "Frontend Protocol normalized event/state subset coverage"
    )
    stable_response_items = sum(
        entry["stability"] == "stable"
        and entry["category"] == "item_discriminator"
        and entry["domain"] == "ResponseItem"
        for entry in manifest["entries"]
    )
    lines.extend(
        [
            "",
            f"The three frontend event/item metrics deliberately share the same {event_denominator}-entry",
            "stable server-notification/item denominator. Their registry pairings are exact:",
            "`ExistingEventSubset` ↔ `ExistingEventSubsetContract`, `GenericExtension` ↔",
            "`ExistingRedactedExtensionContract`, and `ExistingUnknownItemSubset` ↔",
            "`ExistingUnknownItemMetadataContract`. The unknown-item subset contains only",
            "`codexType` and `decodingError`; it does not expose the raw item payload.",
            f"The {stable_response_items} stable `ResponseItem` discriminators have no current runtime/frontend",
            "path and are recorded as `NotExposed` ↔ `Unresolved`.",
            "",
            "Raw-preserved, opaque-preserved, unsupported, deferred, and unresolved entries",
            "do not count as typed or layer implementation. A0 requires complete inventory",
            "registration only; it does not claim complete typed, backend, state, or frontend",
            "coverage.",
            "",
            "## Pinned artifacts",
            "",
            f"- Stable schema aggregate SHA-256: `{provenance['schema_trees']['stable']['aggregate_sha256']}`",
            f"- Experimental schema aggregate SHA-256: `{provenance['schema_trees']['experimental']['aggregate_sha256']}`",
            f"- Stable TypeScript aggregate SHA-256: `{provenance['typescript_cross_check']['stable']['aggregate_sha256']}`",
            f"- Experimental TypeScript aggregate SHA-256: `{provenance['typescript_cross_check']['experimental']['aggregate_sha256']}`",
            "",
            "Both duplicate schema generations differed only in object-member order at",
            "`/definitions` in `codex_app_server_protocol.v2.schemas.json`; parsed JSON",
            "values were equal. Exact changed indices and both key orders are retained in",
            "`PROVENANCE.json`.",
            "",
            "## JSON Schema / TypeScript cross-check",
            "",
            "The TypeScript envelope unions contain four entries absent from the JSON Schema",
            "envelope unions:",
            "",
            "- client requests `getAuthStatus`, `getConversationSummary`, and `gitDiffToRemote`;",
            "- server notification `rawResponseItem/completed`.",
            "",
            "The TypeScript-only root `SessionSource` tagged union is reachable from the",
            "TypeScript-only `getConversationSummary` surface. The distinct v2 union matches",
            "the JSON Schema. `ResponsesApiWebSearchAction` matches root",
            "`WebSearchAction.ts` by its unique discriminator signature; v2",
            "`WebSearchAction` separately matches `v2/WebSearchAction.ts`.",
            "",
            "These discrepancies are recorded, not merged into the schema-authoritative",
            "manifest. All 185 JSON-derived non-method discriminator values have a matching",
            "TypeScript representation.",
            "",
            "## Phase boundary",
            "",
            "A0 establishes the census, registry, guards, and owner worksheet. A1 adds the",
            "frozen typed surface; A2 adds commands and canonical state; A3 exposes only",
            "owner-approved Frontend Protocol operations. Full Phase A is not complete.",
            "",
        ]
    )
    return "\n".join(lines)


RISK_PATTERNS = {
    "executes processes": (
        r"command|process|shell|exec",
        r"approvalPolicy|approvalsReviewer|sandboxPolicy",
    ),
    "mutates files": (
        r"(?:^|/)fs/|file|patch|rollback|install|uninstall",
        r"(?:^|:)cwd$|path|root|sandboxPolicy|permissions",
    ),
    "alters configuration": (
        r"config|experimentalFeature|settings|install|uninstall|marketplace",
        r"approvalPolicy|approvalsReviewer|sandbox|permission|instructions|serviceTier|modelProvider",
    ),
    "authenticates": (r"login|logout|oauth|auth|attestation|accountId|token",),
    "spends credits": (
        r"turn/(?:start|steer)|credit|sendAddCredits",
    ),
    "accesses secrets": (
        r"token|secret|password|answer|attestation",
        r"config/read|fs/readFile",
    ),
    "affects remote services": (
        r"mcp|marketplace|plugin/share|account/|feedback/upload|turn/(?:start|steer)|review/start",
        r"url|network|remote|server|provider",
    ),
}


def risk_fact(entry: dict[str, Any], label: str) -> str:
    evidence = [f"method:{entry['name']}"]
    params = entry.get("params")
    if isinstance(params, dict):
        if params.get("type"):
            evidence.append(f"params-type:{params['type']}")
        evidence.extend(f"field:{field}" for field in params.get("fields", []))
        evidence.extend(
            f"field-path:{field}" for field in params.get("property_paths", [])
        )
    experimental_params = entry.get("experimental_params")
    if isinstance(experimental_params, dict):
        evidence.extend(
            f"experimental-field:{field}"
            for field in experimental_params.get("fields", [])
            if not isinstance(params, dict) or field not in params.get("fields", [])
        )
        evidence.extend(
            f"experimental-field-path:{field}"
            for field in experimental_params.get("property_paths", [])
            if not isinstance(params, dict)
            or field not in params.get("property_paths", [])
        )
    matches: list[str] = []
    for pattern in RISK_PATTERNS[label]:
        matches.extend(
            value
            for value in evidence
            if re.search(pattern, value.partition(":")[2], re.IGNORECASE)
        )
    matches = list(dict.fromkeys(matches))
    if matches:
        rendered = ", ".join(f"`{value}`" for value in matches[:6])
        if len(matches) > 6:
            rendered += f", +{len(matches) - 6} more"
        return f"YES/CONTROL — generated signals: {rendered}"
    return "NOT INDICATED by generated method/parameter names"


def markdown_cell(value: str) -> str:
    return value.replace("|", "\\|").replace("\n", " ")


def render_security_document(
    manifest: dict[str, Any], registry_entries: Sequence[dict[str, Any]]
) -> str:
    registry = registry_by_key(registry_entries)
    operations = [
        entry
        for entry in manifest["entries"]
        if entry["category"] in {"client_request", "server_request"}
    ]
    lines = [
        "# Codex App Server frontend security decisions",
        "",
        "<!-- Generated by tools/codex/app_server_surface.py docs. Do not edit by hand. -->",
        "",
        "This is an owner-review worksheet, not a policy decision. A0 preserves existing",
        "reviewed Frontend Protocol v1 subsets and bounded fallback behavior. Every new,",
        "expanded, or dedicated exposure decision remains `UNRESOLVED`; an existing subset",
        "does not approve the rest of the upstream operation.",
        "",
        "Risk cells report only signals visible in the generated method/parameter surface.",
        "`NOT INDICATED` is not a security guarantee. Result types are not guessed: the",
        "generated request unions do not encode method-to-result associations.",
        "",
        "| Method | Direction | Stability | Parameter/result summary | Process | Files | Config | Auth | Credits | Secrets | Remote | Current frontend exposure | Candidate layers | Final decision |",
        "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|",
    ]
    for entry in operations:
        key = (
            entry["category"],
            entry["domain"],
            entry["discriminator_field"],
            entry["name"],
        )
        local = registry[key]
        parameter_type = entry["params"]["type"] if entry["params"] else None
        required = entry["params"]["required_fields"] if entry["params"] else []
        fields = entry["params"].get("fields", []) if entry["params"] else []
        property_paths = (
            entry["params"].get("property_paths", []) if entry["params"] else []
        )
        experimental_params = entry.get("experimental_params")
        experimental_fields = (
            experimental_params.get("fields", [])
            if isinstance(experimental_params, dict)
            else []
        )
        experimental_additions = sorted(set(experimental_fields) - set(fields))
        summary = (
            f"params `{parameter_type or 'inline/none'}`"
            + (f"; required: {', '.join(required)}" if required else "; no required fields")
            + (f"; fields: {', '.join(fields)}" if fields else "; no parameter fields")
            + (
                f"; {len(property_paths)} recursively discovered property paths"
                if property_paths
                else ""
            )
            + (
                f"; experimental generation additionally exposes: {', '.join(experimental_additions)}"
                if experimental_additions
                else ""
            )
            + "; result association not emitted upstream"
        )
        if (
            local["frontend_security"]
            == "ExistingOperationSubsetExpansionUnresolved"
        ):
            final_decision = (
                "EXISTING REVIEWED SUBSET — unchanged by A0; any new or expanded exposure UNRESOLVED"
            )
            current_exposure = EXISTING_FRONTEND_OPERATION_DETAILS[
                (entry["category"], entry["name"])
            ]
        elif (
            local["frontend_security"]
            == "ExistingGenericContractDedicatedUnresolved"
        ):
            final_decision = (
                "EXISTING BOUNDED/REDACTED UNKNOWN-REQUEST CONTRACT — unchanged by A0; "
                "dedicated exposure UNRESOLVED"
            )
            current_exposure = (
                "v1 `request.pending` unknown summary preserves bounded/redacted method and params; "
                "response via `request.unknown.respond` or `request.unknown.reject`"
            )
        elif entry["name"] == "initialize" and entry["category"] == "client_request":
            final_decision = "EXISTING INTERNAL LIFECYCLE — not a frontend exposure"
            current_exposure = "internal App Server lifecycle only"
        else:
            final_decision = "UNRESOLVED"
            current_exposure = "none"
        lines.append(
            "| "
            + " | ".join(
                markdown_cell(value)
                for value in (
                    f"`{entry['name']}`",
                    "client → server"
                    if entry["category"] == "client_request"
                    else "server → client request",
                    entry["stability"].replace("_", "-"),
                    summary,
                    risk_fact(entry, "executes processes"),
                    risk_fact(entry, "mutates files"),
                    risk_fact(entry, "alters configuration"),
                    risk_fact(entry, "authenticates"),
                    risk_fact(entry, "spends credits"),
                    risk_fact(entry, "accesses secrets"),
                    risk_fact(entry, "affects remote services"),
                    current_exposure,
                    "raw protocol → typed API; BackendCore if application-facing; Frontend Protocol only after owner review",
                    final_decision,
                )
            )
            + " |"
        )
    unresolved_statuses = {
        "Unresolved",
        "ExistingOperationSubsetExpansionUnresolved",
        "ExistingGenericContractDedicatedUnresolved",
    }
    unresolved = sum(
        registry[
            (
                entry["category"],
                entry["domain"],
                entry["discriminator_field"],
                entry["name"],
            )
        ]["frontend_security"]
        in unresolved_statuses
        for entry in operations
    )
    lines.extend(
        [
            "",
            f"Unresolved owner decisions: **{unresolved}/{len(operations)}** operations.",
            "A0 selects no new frontend-exposure, approval, sandbox, or security-boundary policy.",
            "",
        ]
    )
    return "\n".join(lines)


def command_extract(arguments: argparse.Namespace) -> None:
    manifest = extract_surface(arguments.schema_root)
    if arguments.ts_stable and arguments.ts_experimental:
        stable_entries = [entry for entry in manifest["entries"] if entry["stability"] == "stable"]
        manifest["typescript_cross_check"] = {
            "stable": cross_check_typescript(stable_entries, arguments.ts_stable),
            "experimental": cross_check_typescript(manifest["entries"], arguments.ts_experimental),
        }
    if arguments.check:
        expected = load_json(arguments.output)
        if expected != manifest:
            raise SurfaceError(f"generated surface differs from committed manifest: {arguments.output}")
    else:
        write_json(arguments.output, manifest)


def command_provenance(arguments: argparse.Namespace) -> None:
    manifest = extract_surface(arguments.schema_root)
    stable_entries = [entry for entry in manifest["entries"] if entry["stability"] == "stable"]
    stable_cross_check = cross_check_typescript(stable_entries, arguments.ts_stable)
    experimental_cross_check = cross_check_typescript(manifest["entries"], arguments.ts_experimental)
    provenance = build_provenance(
        arguments.schema_root,
        arguments.stable_first,
        arguments.stable_second,
        arguments.experimental_first,
        arguments.experimental_second,
        arguments.ts_stable,
        arguments.ts_experimental,
        stable_cross_check,
        experimental_cross_check,
    )
    write_json(arguments.output, provenance)


def command_verify(arguments: argparse.Namespace) -> None:
    verify_provenance(arguments.schema_root, arguments.provenance)
    generated = extract_surface(arguments.schema_root)
    committed = load_json(arguments.manifest)
    if generated != committed:
        raise SurfaceError(f"surface manifest is stale: {arguments.manifest}")


def command_registry(arguments: argparse.Namespace) -> None:
    manifest = load_json(arguments.manifest)
    evidence = load_a1_registry_evidence(arguments.evidence_root)
    generated = generate_registry_data(manifest, evidence)
    if arguments.check:
        try:
            committed = arguments.output.read_text(encoding="utf-8")
        except OSError as error:
            raise SurfaceError(f"unable to read registry data {arguments.output}: {error}") from error
        if generated != committed:
            raise SurfaceError(f"production registry data is stale: {arguments.output}")
    else:
        arguments.output.parent.mkdir(parents=True, exist_ok=True)
        arguments.output.write_text(generated, encoding="utf-8")


def command_conversation_descriptors(arguments: argparse.Namespace) -> None:
    manifest = load_json(arguments.manifest)
    evidence = load_a1_registry_evidence(arguments.evidence_root)
    generated = generate_conversation_union_descriptor_data(
        manifest, arguments.schema_root, evidence
    )
    write_or_check_conversation_union_descriptors(
        arguments.output, generated, arguments.check
    )


def command_operation_descriptors(arguments: argparse.Namespace) -> None:
    manifest = load_json(arguments.manifest)
    evidence = load_a1_registry_evidence(arguments.evidence_root)
    generated = generate_client_operation_descriptor_data(manifest, evidence)
    write_or_check_client_operation_descriptors(
        arguments.output, generated, arguments.check
    )


def command_notification_descriptors(arguments: argparse.Namespace) -> None:
    manifest = load_json(arguments.manifest)
    evidence = load_a1_registry_evidence(arguments.evidence_root)
    generated = generate_server_notification_descriptor_data(manifest, evidence)
    write_or_check_server_notification_descriptors(
        arguments.output, generated, arguments.check
    )


def command_operation_production_coverage(
    arguments: argparse.Namespace,
) -> None:
    manifest = load_json(arguments.manifest)
    evidence = load_a1_registry_evidence(
        arguments.evidence_root,
        require_operation_production_coverage=False,
        require_notification_production_coverage=False,
    )
    fixture_index = load_json(arguments.fixture_index)
    generated = generate_operation_production_coverage(
        manifest,
        evidence,
        fixture_index,
        arguments.repo_root,
        arguments.fixture_index,
    )
    if arguments.check:
        committed = load_json(arguments.output)
        validate_operation_production_coverage(
            committed,
            manifest,
            evidence,
            fixture_index,
            arguments.repo_root,
            arguments.fixture_index,
        )
    else:
        write_json(arguments.output, generated)


def command_notification_production_coverage(
    arguments: argparse.Namespace,
) -> None:
    manifest = load_json(arguments.manifest)
    evidence = load_a1_registry_evidence(
        arguments.evidence_root,
        require_operation_production_coverage=False,
        require_notification_production_coverage=False,
    )
    fixture_index = load_json(arguments.fixture_index)
    generated = generate_notification_production_coverage(
        manifest,
        evidence,
        fixture_index,
        arguments.repo_root,
        arguments.fixture_index,
    )
    if arguments.check:
        committed = load_json(arguments.output)
        validate_notification_production_coverage(
            committed,
            manifest,
            evidence,
            fixture_index,
            arguments.repo_root,
            arguments.fixture_index,
        )
    else:
        write_json(arguments.output, generated)


def write_or_check_client_operation_descriptors(
    output: Path, generated: str, check: bool
) -> None:
    if check:
        try:
            committed = output.read_text(encoding="utf-8")
        except OSError as error:
            raise SurfaceError(
                "StaleGeneratedClientOperationDescriptors: "
                f"unable to read {output}: {error}"
            ) from error
        if generated != committed:
            raise SurfaceError(
                "StaleGeneratedClientOperationDescriptors: "
                f"generated descriptor data differs from {output}"
            )
    else:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(generated, encoding="utf-8")


def write_or_check_server_notification_descriptors(
    output: Path, generated: str, check: bool
) -> None:
    if check:
        try:
            committed = output.read_text(encoding="utf-8")
        except OSError as error:
            raise SurfaceError(
                "StaleGeneratedServerNotificationDescriptors: "
                f"unable to read {output}: {error}"
            ) from error
        if generated != committed:
            raise SurfaceError(
                "StaleGeneratedServerNotificationDescriptors: "
                f"generated descriptor data differs from {output}"
            )
    else:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(generated, encoding="utf-8")


def write_or_check_conversation_union_descriptors(
    output: Path, generated: str, check: bool
) -> None:
    if check:
        try:
            committed = output.read_text(encoding="utf-8")
        except OSError as error:
            raise SurfaceError(
                "StaleGeneratedConversationUnionDescriptors: "
                f"unable to read {output}: {error}"
            ) from error
        if generated != committed:
            raise SurfaceError(
                "StaleGeneratedConversationUnionDescriptors: "
                f"generated descriptor data differs from {output}"
            )
    else:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(generated, encoding="utf-8")


def command_item_descriptors(arguments: argparse.Namespace) -> None:
    manifest = load_json(arguments.manifest)
    evidence = load_a1_registry_evidence(arguments.evidence_root)
    outputs = (
        (
            arguments.thread_output,
            generate_item_codec_descriptor_data(
                manifest,
                arguments.schema_root,
                "ThreadItem",
                "ItemDiscriminatorTarget",
                "CODEX_THREAD_ITEM_CODEC_DESCRIPTOR",
                18,
                evidence,
            ),
        ),
        (
            arguments.response_output,
            generate_item_codec_descriptor_data(
                manifest,
                arguments.schema_root,
                "ResponseItem",
                "ResponseItemTarget",
                "CODEX_RESPONSE_ITEM_CODEC_DESCRIPTOR",
                16,
                evidence,
            ),
        ),
    )
    for output, generated in outputs:
        write_or_check_item_codec_descriptors(output, generated, arguments.check)


def write_or_check_item_codec_descriptors(
    output: Path, generated: str, check: bool
) -> None:
    if check:
        try:
            committed = output.read_text(encoding="utf-8")
        except OSError as error:
            raise SurfaceError(
                "StaleGeneratedItemCodecDescriptors: "
                f"unable to read {output}: {error}"
            ) from error
        if generated != committed:
            raise SurfaceError(
                "StaleGeneratedItemCodecDescriptors: "
                f"generated descriptor data differs from {output}"
            )
    else:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(generated, encoding="utf-8")


def command_docs(arguments: argparse.Namespace) -> None:
    manifest = load_json(arguments.manifest)
    provenance = load_json(arguments.provenance)
    registry_entries = parse_registry_data(arguments.registry)
    coverage = render_coverage_document(manifest, registry_entries, provenance)
    security = render_security_document(manifest, registry_entries)
    outputs = (
        (arguments.coverage_output, coverage),
        (arguments.security_output, security),
    )
    if arguments.check:
        for path, generated in outputs:
            try:
                committed = path.read_text(encoding="utf-8")
            except OSError as error:
                raise SurfaceError(f"unable to read generated document {path}: {error}") from error
            if committed != generated:
                raise SurfaceError(f"generated document is stale: {path}")
    else:
        for path, generated in outputs:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(generated, encoding="utf-8")


def parser() -> argparse.ArgumentParser:
    result = argparse.ArgumentParser(description=__doc__)
    subparsers = result.add_subparsers(dest="command", required=True)

    extract = subparsers.add_parser("extract", help="extract the schema-authoritative surface")
    extract.add_argument("--schema-root", type=Path, required=True)
    extract.add_argument("--output", type=Path, required=True)
    extract.add_argument("--ts-stable", type=Path)
    extract.add_argument("--ts-experimental", type=Path)
    extract.add_argument("--check", action="store_true")
    extract.set_defaults(function=command_extract)

    provenance = subparsers.add_parser("provenance", help="write deterministic hashes and generation evidence")
    provenance.add_argument("--schema-root", type=Path, required=True)
    provenance.add_argument("--stable-first", type=Path, required=True)
    provenance.add_argument("--stable-second", type=Path, required=True)
    provenance.add_argument("--experimental-first", type=Path, required=True)
    provenance.add_argument("--experimental-second", type=Path, required=True)
    provenance.add_argument("--ts-stable", type=Path, required=True)
    provenance.add_argument("--ts-experimental", type=Path, required=True)
    provenance.add_argument("--output", type=Path, required=True)
    provenance.set_defaults(function=command_provenance)

    verify = subparsers.add_parser("verify", help="verify vendored hashes and committed manifest")
    verify.add_argument("--schema-root", type=Path, required=True)
    verify.add_argument("--provenance", type=Path, required=True)
    verify.add_argument("--manifest", type=Path, required=True)
    verify.set_defaults(function=command_verify)

    registry = subparsers.add_parser(
        "registry", help="generate the canonical production C++ registry data"
    )
    registry.add_argument("--manifest", type=Path, required=True)
    registry.add_argument(
        "--evidence-root", type=Path, default=DEFAULT_A1_EVIDENCE_ROOT
    )
    registry.add_argument("--output", type=Path, required=True)
    registry.add_argument("--check", action="store_true")
    registry.set_defaults(function=command_registry)

    conversation_descriptors = subparsers.add_parser(
        "conversation-descriptors",
        help="generate private A1.1 shared conversation-union codec descriptors",
    )
    conversation_descriptors.add_argument(
        "--manifest", type=Path, required=True
    )
    conversation_descriptors.add_argument(
        "--schema-root", type=Path, required=True
    )
    conversation_descriptors.add_argument(
        "--evidence-root", type=Path, default=DEFAULT_A1_EVIDENCE_ROOT
    )
    conversation_descriptors.add_argument("--output", type=Path, required=True)
    conversation_descriptors.add_argument("--check", action="store_true")
    conversation_descriptors.set_defaults(
        function=command_conversation_descriptors
    )

    operation_descriptors = subparsers.add_parser(
        "operation-descriptors",
        help="generate private exact A1.1 client operation codec descriptors",
    )
    operation_descriptors.add_argument("--manifest", type=Path, required=True)
    operation_descriptors.add_argument(
        "--evidence-root", type=Path, default=DEFAULT_A1_EVIDENCE_ROOT
    )
    operation_descriptors.add_argument("--output", type=Path, required=True)
    operation_descriptors.add_argument("--check", action="store_true")
    operation_descriptors.set_defaults(
        function=command_operation_descriptors
    )

    notification_descriptors = subparsers.add_parser(
        "notification-descriptors",
        help="generate private exact typed server-notification codec descriptors",
    )
    notification_descriptors.add_argument("--manifest", type=Path, required=True)
    notification_descriptors.add_argument(
        "--evidence-root", type=Path, default=DEFAULT_A1_EVIDENCE_ROOT
    )
    notification_descriptors.add_argument("--output", type=Path, required=True)
    notification_descriptors.add_argument("--check", action="store_true")
    notification_descriptors.set_defaults(
        function=command_notification_descriptors
    )

    operation_coverage = subparsers.add_parser(
        "operation-production-coverage",
        help=(
            "generate the checked A1.1 operation result corpus and "
            "production-decoder coverage table"
        ),
    )
    operation_coverage.add_argument("--manifest", type=Path, required=True)
    operation_coverage.add_argument(
        "--evidence-root", type=Path, default=DEFAULT_A1_EVIDENCE_ROOT
    )
    operation_coverage.add_argument(
        "--fixture-index", type=Path, default=DEFAULT_A1_FIXTURE_INDEX
    )
    operation_coverage.add_argument(
        "--repo-root", type=Path, default=DEFAULT_REPO_ROOT
    )
    operation_coverage.add_argument("--output", type=Path, required=True)
    operation_coverage.add_argument("--check", action="store_true")
    operation_coverage.set_defaults(
        function=command_operation_production_coverage
    )

    notification_coverage = subparsers.add_parser(
        "notification-production-coverage",
        help=(
            "generate the checked A1.1 server-notification fixture, type, "
            "and production-decoder coverage table"
        ),
    )
    notification_coverage.add_argument("--manifest", type=Path, required=True)
    notification_coverage.add_argument(
        "--evidence-root", type=Path, default=DEFAULT_A1_EVIDENCE_ROOT
    )
    notification_coverage.add_argument(
        "--fixture-index", type=Path, default=DEFAULT_A1_FIXTURE_INDEX
    )
    notification_coverage.add_argument(
        "--repo-root", type=Path, default=DEFAULT_REPO_ROOT
    )
    notification_coverage.add_argument("--output", type=Path, required=True)
    notification_coverage.add_argument("--check", action="store_true")
    notification_coverage.set_defaults(
        function=command_notification_production_coverage
    )

    item_descriptors = subparsers.add_parser(
        "item-descriptors",
        help="generate private A1.1 ThreadItem and ResponseItem codec descriptors",
    )
    item_descriptors.add_argument("--manifest", type=Path, required=True)
    item_descriptors.add_argument("--schema-root", type=Path, required=True)
    item_descriptors.add_argument(
        "--evidence-root", type=Path, default=DEFAULT_A1_EVIDENCE_ROOT
    )
    item_descriptors.add_argument("--thread-output", type=Path, required=True)
    item_descriptors.add_argument("--response-output", type=Path, required=True)
    item_descriptors.add_argument("--check", action="store_true")
    item_descriptors.set_defaults(function=command_item_descriptors)

    docs = subparsers.add_parser(
        "docs", help="generate coverage and owner security worksheet documents"
    )
    docs.add_argument("--manifest", type=Path, required=True)
    docs.add_argument("--registry", type=Path, required=True)
    docs.add_argument("--provenance", type=Path, required=True)
    docs.add_argument("--coverage-output", type=Path, required=True)
    docs.add_argument("--security-output", type=Path, required=True)
    docs.add_argument("--check", action="store_true")
    docs.set_defaults(function=command_docs)
    return result


def main(argv: Sequence[str] | None = None) -> int:
    arguments = parser().parse_args(argv)
    if bool(getattr(arguments, "ts_stable", None)) != bool(getattr(arguments, "ts_experimental", None)):
        raise SurfaceError("--ts-stable and --ts-experimental must be supplied together")
    arguments.function(arguments)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except SurfaceError as error:
        print(f"app-server-surface: error: {error}", file=sys.stderr)
        raise SystemExit(1)
