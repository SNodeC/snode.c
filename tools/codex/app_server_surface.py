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
import json
import re
import sys
from pathlib import Path
from typing import Any, Iterator, Sequence


FORMAT_VERSION = 1
CODEX_VERSION = "codex-cli 0.144.6"
STARTING_SNODEC_SHA = "138f5022c19b24847bee42a21242aaaf7dde5a04"
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
RUNTIME_TARGETS = {
    ("client_request", "ClientRequest", "method", "initialize"): "ClientRequestTarget::Initialize",
    ("client_request", "ClientRequest", "method", "thread/start"): "ClientRequestTarget::ThreadStart",
    ("client_request", "ClientRequest", "method", "thread/resume"): "ClientRequestTarget::ThreadResume",
    ("client_request", "ClientRequest", "method", "thread/list"): "ClientRequestTarget::ThreadList",
    ("client_request", "ClientRequest", "method", "thread/read"): "ClientRequestTarget::ThreadRead",
    ("client_request", "ClientRequest", "method", "turn/start"): "ClientRequestTarget::TurnStart",
    ("client_request", "ClientRequest", "method", "turn/interrupt"): "ClientRequestTarget::TurnInterrupt",
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
        "item/reasoning/textDelta",
    ): "ServerNotificationTarget::ReasoningTextDelta",
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
        "item/fileChange/patchUpdated",
    ): "ServerNotificationTarget::FileChangePatchUpdated",
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
    ("item_discriminator", "ThreadItem", "type", "userMessage"): "ItemDiscriminatorTarget::UserMessage",
    ("item_discriminator", "ThreadItem", "type", "reasoning"): "ItemDiscriminatorTarget::Reasoning",
    (
        "item_discriminator",
        "ThreadItem",
        "type",
        "commandExecution",
    ): "ItemDiscriminatorTarget::CommandExecution",
    ("item_discriminator", "ThreadItem", "type", "fileChange"): "ItemDiscriminatorTarget::FileChange",
    ("item_discriminator", "ThreadItem", "type", "mcpToolCall"): "ItemDiscriminatorTarget::McpToolCall",
    (
        "item_discriminator",
        "ThreadItem",
        "type",
        "dynamicToolCall",
    ): "ItemDiscriminatorTarget::DynamicToolCall",
    ("item_discriminator", "ThreadItem", "type", "webSearch"): "ItemDiscriminatorTarget::WebSearch",
}
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
    if provenance.get("format_version") != FORMAT_VERSION:
        raise SurfaceError("unsupported provenance format_version")
    if provenance.get("codex_version") != CODEX_VERSION:
        raise SurfaceError("provenance Codex version does not match the pinned tool")
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
        "format_version": FORMAT_VERSION,
        "codex_version": CODEX_VERSION,
        "starting_snodec_sha": STARTING_SNODEC_SHA,
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


def registry_statuses(entry: dict[str, Any]) -> tuple[str, ...]:
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
        if target is not None:
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
        if target is not None:
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

    return (
        runtime_disposition,
        typed_status,
        backend_status,
        canonical_state_status,
        frontend_exposure,
        frontend_security,
        target or "std::monostate{}",
    )


def generate_registry_data(manifest: dict[str, Any]) -> str:
    entries = manifest.get("entries")
    if not isinstance(entries, list):
        raise SurfaceError("surface manifest has no entries array")
    lines = [
        "// Generated by tools/codex/app_server_surface.py registry.",
        "// Inventory fields come only from the vendored JSON Schema manifest.",
        "// Local disposition fields describe existing implementation; A0 adds no missing API.",
    ]
    for entry in entries:
        try:
            category = CPP_CATEGORIES[entry["category"]]
        except KeyError as error:
            raise SurfaceError(f"registry has unknown category: {entry.get('category')!r}") from error
        statuses = registry_statuses(entry)
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


def parse_registry_data(path: Path) -> list[dict[str, Any]]:
    reverse_categories = {value: key for key, value in CPP_CATEGORIES.items()}
    entries: list[dict[str, Any]] = []
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as error:
        raise SurfaceError(f"unable to read production registry data {path}: {error}") from error
    prefix = "CODEX_PROTOCOL_SURFACE_ENTRY("
    for line_number, line in enumerate(lines, start=1):
        if not line.startswith(prefix):
            continue
        if not line.endswith(")"):
            raise SurfaceError(f"malformed registry macro at {path}:{line_number}")
        arguments = split_cpp_arguments(line[len(prefix) : -1])
        if len(arguments) != 13:
            raise SurfaceError(
                f"registry macro at {path}:{line_number} has {len(arguments)} arguments, expected 13"
            )
        try:
            category = reverse_categories[arguments[0]]
            domain = json.loads(arguments[1])
            field = json.loads(arguments[2])
            name = json.loads(arguments[3])
        except (KeyError, json.JSONDecodeError) as error:
            raise SurfaceError(f"invalid registry identity at {path}:{line_number}") from error
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
            }
        )
    if not entries:
        raise SurfaceError(f"production registry data contains no entries: {path}")
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
    lines = [
        "# Codex App Server API coverage",
        "",
        "<!-- Generated by tools/codex/app_server_surface.py docs. Do not edit by hand. -->",
        "",
        f"Authoritative target: `{manifest['codex_version']}` generated from the locally installed Codex binary.",
        f"Starting SNode.C commit: `{manifest['starting_snodec_sha']}`.",
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
    generated = generate_registry_data(manifest)
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
    registry.add_argument("--output", type=Path, required=True)
    registry.add_argument("--check", action="store_true")
    registry.set_defaults(function=command_registry)

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
