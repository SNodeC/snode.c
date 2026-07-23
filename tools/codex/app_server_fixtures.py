#!/usr/bin/env python3
"""Generate and guard offline Codex App Server schema evidence fixtures.

Inputs are limited to the vendored schema tree, the pinned A0 surface manifest,
the reviewed operation-contract evidence, and deterministic rules in this
file, including the exact existing-34 fixture scope and frozen slice routing.
Production C++ decoders and test ratchet headers are deliberately not
inspected.

Generated files are review evidence, not implementation or completeness
claims.  Use ``generate`` to refresh them, ``check`` to compare regeneration
byte-for-byte, and ``validate`` to revalidate the committed corpus and all
required-field/wrong-type mutations.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import importlib.util
import json
import re
import shutil
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from types import ModuleType
from typing import Any, Iterable, Iterator, Mapping, MutableMapping, Sequence

sys.dont_write_bytecode = True


FORMAT_VERSION = 1
CODEX_VERSION = "codex-cli 0.144.6"
RULES_VERSION = 2

CLIENT_REQUEST = "client_request"
CLIENT_NOTIFICATION = "client_notification"
SERVER_NOTIFICATION = "server_notification"
SERVER_REQUEST = "server_request"
ITEM_DISCRIMINATOR = "item_discriminator"
TAGGED_UNION_DISCRIMINATOR = "tagged_union_discriminator"

METHOD_SCHEMA_FILES = {
    CLIENT_REQUEST: "ClientRequest.json",
    CLIENT_NOTIFICATION: "ClientNotification.json",
    SERVER_NOTIFICATION: "ServerNotification.json",
    SERVER_REQUEST: "ServerRequest.json",
}

# Reviewed deterministic fixture scope for the A0 runtime floor. This is input
# to fixture synthesis, not an implementation claim. The focused tool test
# independently compares it with the non-generated C++ ratchet so neither can
# drift silently.
EXISTING_TYPED_FIXTURE_IDENTITIES = (
    (CLIENT_NOTIFICATION, "ClientNotification", "method", "initialized"),
    (CLIENT_REQUEST, "ClientRequest", "method", "initialize"),
    (CLIENT_REQUEST, "ClientRequest", "method", "thread/list"),
    (CLIENT_REQUEST, "ClientRequest", "method", "thread/read"),
    (CLIENT_REQUEST, "ClientRequest", "method", "thread/resume"),
    (CLIENT_REQUEST, "ClientRequest", "method", "thread/start"),
    (CLIENT_REQUEST, "ClientRequest", "method", "turn/interrupt"),
    (CLIENT_REQUEST, "ClientRequest", "method", "turn/start"),
    (SERVER_NOTIFICATION, "ServerNotification", "method", "error"),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "item/agentMessage/delta",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "item/commandExecution/outputDelta",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "item/completed",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "item/fileChange/patchUpdated",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "item/reasoning/summaryTextDelta",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "item/reasoning/textDelta",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "item/started",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "model/rerouted",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "thread/started",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "thread/status/changed",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "thread/tokenUsage/updated",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "turn/completed",
    ),
    (
        SERVER_NOTIFICATION,
        "ServerNotification",
        "method",
        "turn/started",
    ),
    (
        SERVER_REQUEST,
        "ServerRequest",
        "method",
        "account/chatgptAuthTokens/refresh",
    ),
    (
        SERVER_REQUEST,
        "ServerRequest",
        "method",
        "item/commandExecution/requestApproval",
    ),
    (
        SERVER_REQUEST,
        "ServerRequest",
        "method",
        "item/fileChange/requestApproval",
    ),
    (
        SERVER_REQUEST,
        "ServerRequest",
        "method",
        "item/tool/requestUserInput",
    ),
    (ITEM_DISCRIMINATOR, "ThreadItem", "type", "agentMessage"),
    (ITEM_DISCRIMINATOR, "ThreadItem", "type", "commandExecution"),
    (ITEM_DISCRIMINATOR, "ThreadItem", "type", "dynamicToolCall"),
    (ITEM_DISCRIMINATOR, "ThreadItem", "type", "fileChange"),
    (ITEM_DISCRIMINATOR, "ThreadItem", "type", "mcpToolCall"),
    (ITEM_DISCRIMINATOR, "ThreadItem", "type", "reasoning"),
    (ITEM_DISCRIMINATOR, "ThreadItem", "type", "userMessage"),
    (ITEM_DISCRIMINATOR, "ThreadItem", "type", "webSearch"),
)

CODEX_ERROR_INFO_IDENTITIES = (
    "activeTurnNotSteerable",
    "badRequest",
    "contextWindowExceeded",
    "cyberPolicy",
    "httpConnectionFailed",
    "internalServerError",
    "other",
    "responseStreamConnectionFailed",
    "responseStreamDisconnected",
    "responseTooManyFailedAttempts",
    "sandboxError",
    "serverOverloaded",
    "sessionBudgetExceeded",
    "threadRollbackFailed",
    "unauthorized",
    "usageLimitExceeded",
)

CODEX_ERROR_INFO_HTTP_IDENTITIES = (
    "httpConnectionFailed",
    "responseStreamConnectionFailed",
    "responseStreamDisconnected",
    "responseTooManyFailedAttempts",
)

# Commit B2 owns the exact A1.1 SharedCommon tagged-union batch.  The keys
# themselves are derived from module-slice-assignment evidence below; these
# reviewed counts and directions are only bidirectional audit ratchets for the
# fixture plan, not a runtime disposition or dispatch registry.
B2_SHARED_COMMON_FAMILY_COUNTS = {
    "AskForApproval": 4,
    "CommandAction": 4,
    "DynamicToolCallOutputContentItem": 2,
    "PatchChangeKind": 3,
    "SandboxPolicy": 4,
    "UserInput": 5,
    "WebSearchAction": 4,
}
B2_SHARED_COMMON_DIRECTIONS = {
    "AskForApproval": ("Decode", "Encode"),
    "CommandAction": ("Decode",),
    "DynamicToolCallOutputContentItem": ("Decode",),
    "PatchChangeKind": ("Decode",),
    "SandboxPolicy": ("Decode", "Encode"),
    "UserInput": ("Decode", "Encode"),
    "WebSearchAction": ("Decode",),
}
B2_OPEN_STRING_ENUMS = {
    "ImageDetail": ("auto", "low", "high", "original"),
    "NetworkAccess": ("restricted", "enabled"),
}

SLICE_ORDER = {"A1.0": 0, "A1.1": 1, "A1.2": 2, "A1.3": 3, "A1.4": 4}
SLICE_MODULES = {
    "A1.0": "Common",
    "A1.1": "ThreadsTurnsSessions",
    "A1.2": "AccountsModelsConfiguration",
    "A1.3": "CommandsFilesystemReviewsApprovals",
    "A1.4": "IntegrationsAndLongTail",
}

# These are the reviewed stable public roots assigned to the A1.4 long-tail
# slice. Keep the sets category-specific and exact: adding a new stable public
# root to the pinned manifest must fail until its slice ownership is reviewed.
A1_4_PUBLIC_ROOTS = {
    CLIENT_REQUEST: frozenset(
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
    SERVER_NOTIFICATION: frozenset(
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
    SERVER_REQUEST: frozenset(
        {
            "attestation/generate",
            "item/tool/call",
            "item/tool/requestUserInput",
            "mcpServer/elicitation/request",
        }
    ),
}


class FixtureError(RuntimeError):
    pass


def load_draft07(path: Path) -> ModuleType:
    specification = importlib.util.spec_from_file_location(
        "snodec_codex_draft07_fixture_tool", path
    )
    if specification is None or specification.loader is None:
        raise FixtureError(f"unable to load Draft-07 validator: {path}")
    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


def load_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise FixtureError(f"unable to load JSON artifact {path}: {error}") from error


def encoded_json(value: Any) -> bytes:
    return (
        json.dumps(
            value,
            ensure_ascii=False,
            allow_nan=False,
            indent=2,
            sort_keys=True,
        )
        + "\n"
    ).encode("utf-8")


def canonical_json(value: Any) -> str:
    return json.dumps(
        value,
        ensure_ascii=False,
        allow_nan=False,
        separators=(",", ":"),
        sort_keys=True,
    )


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: Path) -> str:
    try:
        return sha256_bytes(path.read_bytes())
    except OSError as error:
        raise FixtureError(f"unable to hash {path}: {error}") from error


def slug(value: str) -> str:
    rendered = re.sub(r"[^A-Za-z0-9._-]+", "-", value).strip("-").lower()
    return rendered or "root"


def pointer_token(value: str) -> str:
    return value.replace("~", "~0").replace("/", "~1")


def pointer_child(path: str, token: str | int) -> str:
    return f"{path}/{pointer_token(str(token))}"


def instance_child(path: tuple[str | int, ...], token: str | int) -> tuple[str | int, ...]:
    return (*path, token)


def json_path(path: Sequence[str | int]) -> str:
    result = "$"
    for token in path:
        result += f"/{pointer_token(str(token))}"
    return result


def get_instance_path(instance: Any, path: Sequence[str | int]) -> Any:
    node = instance
    for token in path:
        if isinstance(token, int):
            node = node[token]
        else:
            node = node[token]
    return node


def get_parent_path(instance: Any, path: Sequence[str | int]) -> tuple[Any, str | int]:
    if not path:
        raise FixtureError("root instance has no parent")
    return get_instance_path(instance, path[:-1]), path[-1]


@dataclass(frozen=True, order=True)
class SurfaceKey:
    category: str
    domain: str
    discriminator_field: str
    name: str

    @classmethod
    def from_manifest(cls, entry: Mapping[str, Any]) -> "SurfaceKey":
        return cls(
            str(entry["category"]),
            str(entry["domain"]),
            str(entry["discriminator_field"]),
            str(entry["name"]),
        )

    @classmethod
    def from_contract(cls, value: Mapping[str, Any]) -> "SurfaceKey":
        field = value.get("discriminator_field", value.get("field"))
        if not isinstance(field, str):
            raise FixtureError("operation contract surface_key lacks field")
        return cls(
            str(value["category"]),
            str(value["domain"]),
            field,
            str(value["name"]),
        )

    def to_json(self) -> dict[str, str]:
        return {
            "category": self.category,
            "domain": self.domain,
            "discriminator_field": self.discriminator_field,
            "name": self.name,
        }

    def compact(self) -> str:
        return (
            f"{self.category}:{self.domain}:"
            f"{self.discriminator_field}:{self.name}"
        )


def derive_b2_shared_common_keys(
    assignments: Mapping[SurfaceKey, Mapping[str, Any]],
) -> tuple[SurfaceKey, ...]:
    keys = tuple(
        sorted(
            key
            for key, assignment in assignments.items()
            if assignment["a1_slice"] == "A1.1"
            and assignment["classification"] == "SharedCommon"
        )
    )
    family_counts: dict[str, int] = {}
    for key in keys:
        if key.category != TAGGED_UNION_DISCRIMINATOR:
            raise FixtureError(
                "A1.1 SharedCommon batch contains a non-union identity: "
                f"{key.compact()}"
            )
        family_counts[key.domain] = family_counts.get(key.domain, 0) + 1
    if (
        len(keys) != 26
        or family_counts != B2_SHARED_COMMON_FAMILY_COUNTS
        or set(family_counts) != set(B2_SHARED_COMMON_DIRECTIONS)
    ):
        raise FixtureError(
            "A1.1 SharedCommon assignment mismatch: "
            f"count={len(keys)} "
            f"families={dict(sorted(family_counts.items()))}"
        )
    return keys


@dataclass(frozen=True)
class SchemaTarget:
    document: Any
    schema: Any
    schema_path: str
    relative_file: str | None
    file_sha256: str | None
    inline_schema: Any | None = None

    def to_json(self) -> dict[str, Any]:
        if self.inline_schema is not None:
            return {
                "inline": self.inline_schema,
                "pointer": self.schema_path,
            }
        assert self.relative_file is not None and self.file_sha256 is not None
        return {
            "file": self.relative_file,
            "pointer": self.schema_path,
            "sha256": self.file_sha256,
        }


class SchemaCatalog:
    def __init__(self, schema_root: Path, draft07: ModuleType):
        self.root = schema_root.resolve()
        self.stable_root = (self.root / "stable").resolve()
        self.draft07 = draft07
        if not self.stable_root.is_dir():
            raise FixtureError(f"stable schema tree is missing: {self.stable_root}")
        self._documents: dict[Path, Any] = {}
        self._validators: dict[Path, Any] = {}
        self._type_paths: dict[str, Path] = {}
        for path in sorted(self.stable_root.rglob("*.json")):
            previous = self._type_paths.setdefault(path.stem, path)
            if previous != path:
                raise FixtureError(
                    f"duplicate stable schema basename {path.stem}: "
                    f"{previous} and {path}"
                )

    def load(self, path: Path) -> Any:
        path = path.resolve()
        try:
            path.relative_to(self.root)
        except ValueError as error:
            raise FixtureError(f"schema path escapes vendored root: {path}") from error
        if path not in self._documents:
            self._documents[path] = load_json(path)
        return self._documents[path]

    def validator(self, path: Path) -> Any:
        path = path.resolve()
        if path not in self._validators:
            self._validators[path] = self.draft07.Validator(self.load(path))
        return self._validators[path]

    def relative(self, path: Path) -> str:
        return path.resolve().relative_to(self.root).as_posix()

    def standalone(self, type_identity: str) -> SchemaTarget:
        if type_identity == "Unit":
            schema = {"type": "null"}
            return SchemaTarget(schema, schema, "#", None, None, schema)
        path = self._type_paths.get(type_identity)
        if path is None:
            raise FixtureError(
                f"no unique stable standalone schema root for {type_identity!r}"
            )
        document = self.load(path)
        title = document.get("title")
        if title != type_identity:
            raise FixtureError(
                f"standalone schema title mismatch for {type_identity}: "
                f"{self.relative(path)} declares {title!r}"
            )
        self.validator(path)
        return SchemaTarget(
            document,
            document,
            "#",
            self.relative(path),
            sha256_file(path),
        )

    def method_target(
        self, category: str, method: str
    ) -> tuple[SchemaTarget, int, Any]:
        relative = METHOD_SCHEMA_FILES.get(category)
        if relative is None:
            raise FixtureError(f"{category} is not a method category")
        path = self.stable_root / relative
        document = self.load(path)
        validator = self.validator(path)
        matches: list[tuple[int, Any]] = []
        for index, branch in enumerate(document.get("oneOf", [])):
            values = (
                branch.get("properties", {})
                .get("method", {})
                .get("enum", [])
            )
            if values == [method]:
                matches.append((index, branch))
        if len(matches) != 1:
            raise FixtureError(
                f"{category}:{method} selects {len(matches)} branches in {relative}"
            )
        index, branch = matches[0]
        target = SchemaTarget(
            document,
            document,
            "#",
            self.relative(path),
            sha256_file(path),
        )
        return target, index, branch

    def union_target(self, domain: str) -> SchemaTarget:
        path = self.stable_root / "codex_app_server_protocol.v2.schemas.json"
        document = self.load(path)
        definitions = document.get("definitions", {})
        if domain not in definitions:
            raise FixtureError(
                f"stable standalone v2 aggregate lacks union {domain}"
            )
        self.validator(path)
        return SchemaTarget(
            document,
            definitions[domain],
            f"#/definitions/{pointer_token(domain)}",
            self.relative(path),
            sha256_file(path),
        )

    def target_validator(self, target: SchemaTarget) -> Any:
        if target.relative_file is None:
            return self.draft07.Validator(target.document)
        return self.validator(self.root / target.relative_file)

    def resolve(
        self, target: SchemaTarget, schema: Any, schema_path: str
    ) -> tuple[Any, str]:
        validator = self.target_validator(target)
        seen: set[str] = set()
        node = schema
        path = schema_path
        while isinstance(node, dict) and "$ref" in node:
            reference = node["$ref"]
            if reference in seen:
                raise FixtureError(
                    f"direct schema reference cycle while synthesizing {path}"
                )
            seen.add(reference)
            node, path = validator.resolve_reference(
                reference, pointer_child(path, "$ref")
            )
        return node, path


class Synthesizer:
    def __init__(self, catalog: SchemaCatalog):
        self.catalog = catalog

    def sample(
        self,
        target: SchemaTarget,
        schema: Any | None = None,
        schema_path: str | None = None,
        forced_scalar: Any | None = None,
        forced_property: tuple[str, Any] | None = None,
    ) -> Any:
        schema = target.schema if schema is None else schema
        schema_path = target.schema_path if schema_path is None else schema_path
        value = self._synthesize(
            target,
            schema,
            schema_path,
            full=True,
            active=frozenset(),
            depth=0,
        )
        if forced_scalar is not None:
            value = forced_scalar
        if forced_property is not None:
            if not isinstance(value, dict):
                raise FixtureError(
                    f"cannot force property on non-object fixture at {schema_path}"
                )
            value[forced_property[0]] = forced_property[1]
        diagnostics = self.catalog.target_validator(target).validate_subschema(
            value, schema, schema_path
        )
        if diagnostics:
            rendered = [item.to_json() for item in diagnostics[:5]]
            raise FixtureError(
                f"unable to synthesize valid fixture for {schema_path}: {rendered}"
            )
        return value

    def _synthesize(
        self,
        target: SchemaTarget,
        schema: Any,
        schema_path: str,
        full: bool,
        active: frozenset[tuple[str, str]],
        depth: int,
    ) -> Any:
        if depth > 80:
            raise FixtureError(f"schema synthesis depth exceeded at {schema_path}")
        if schema is True:
            return {}
        if schema is False:
            raise FixtureError(f"cannot synthesize false schema at {schema_path}")
        if not isinstance(schema, dict):
            raise FixtureError(f"non-schema during synthesis at {schema_path}")

        if "$ref" in schema:
            validator = self.catalog.target_validator(target)
            referenced, referenced_path = validator.resolve_reference(
                schema["$ref"], pointer_child(schema_path, "$ref")
            )
            marker = (schema_path, referenced_path)
            if marker in active or (referenced_path, referenced_path) in active:
                return self._recursive_base(
                    target,
                    referenced,
                    referenced_path,
                    active,
                    depth + 1,
                )
            return self._synthesize(
                target,
                referenced,
                referenced_path,
                full,
                active | {marker, (referenced_path, referenced_path)},
                depth + 1,
            )

        if "const" in schema:
            return copy.deepcopy(schema["const"])
        if "enum" in schema:
            return copy.deepcopy(schema["enum"][0])

        if "oneOf" in schema:
            validator = self.catalog.target_validator(target)
            outer = self._outer_combinator_value(
                target,
                schema,
                schema_path,
                "oneOf",
                full,
                active,
                depth,
            )
            for index, branch in enumerate(schema["oneOf"]):
                try:
                    candidate = self._synthesize(
                        target,
                        branch,
                        pointer_child(pointer_child(schema_path, "oneOf"), index),
                        full,
                        active,
                        depth + 1,
                    )
                    candidate = combine_values(
                        outer, candidate, schema_path
                    )
                except FixtureError:
                    continue
                diagnostics = validator.validate_subschema(
                    candidate, schema, schema_path
                )
                if not diagnostics:
                    return candidate
            raise FixtureError(
                f"no deterministic exclusive oneOf sample at {schema_path}"
            )

        if "anyOf" in schema:
            validator = self.catalog.target_validator(target)
            outer = self._outer_combinator_value(
                target,
                schema,
                schema_path,
                "anyOf",
                full,
                active,
                depth,
            )
            indexed = list(enumerate(schema["anyOf"]))
            indexed.sort(
                key=lambda item: (
                    self._schema_is_null(target, item[1], schema_path),
                    item[0],
                )
            )
            for index, branch in indexed:
                try:
                    candidate = self._synthesize(
                        target,
                        branch,
                        pointer_child(pointer_child(schema_path, "anyOf"), index),
                        full,
                        active,
                        depth + 1,
                    )
                    candidate = combine_values(
                        outer, candidate, schema_path
                    )
                except FixtureError:
                    continue
                if not validator.validate_subschema(candidate, schema, schema_path):
                    return candidate
            raise FixtureError(f"no deterministic anyOf sample at {schema_path}")

        all_of_values: list[Any] = []
        for index, branch in enumerate(schema.get("allOf", [])):
            all_of_values.append(
                self._synthesize(
                    target,
                    branch,
                    pointer_child(pointer_child(schema_path, "allOf"), index),
                    full,
                    active,
                    depth + 1,
                )
            )

        expected = schema.get("type")
        expected_types = [expected] if isinstance(expected, str) else expected
        if expected_types is None:
            if "properties" in schema or "required" in schema:
                expected_types = ["object"]
            elif "items" in schema:
                expected_types = ["array"]
            elif "minimum" in schema or "maximum" in schema:
                expected_types = ["number"]
            elif (
                "minLength" in schema
                or "maxLength" in schema
                or "pattern" in schema
            ):
                expected_types = ["string"]
            elif all_of_values:
                return self._merge_all_of(all_of_values, schema_path)
            else:
                return {}
        assert isinstance(expected_types, list)
        selected_type = next(
            (value for value in expected_types if value != "null"),
            expected_types[0],
        )

        if selected_type == "null":
            return None
        if selected_type == "boolean":
            return False
        if selected_type in {"integer", "number"}:
            minimum = schema.get("minimum", 0)
            maximum = schema.get("maximum")
            value: int | float = max(minimum, 0)
            if maximum is not None and value > maximum:
                value = maximum
            if selected_type == "integer":
                value = int(math_ceiling(value))
                if maximum is not None and value > maximum:
                    value = int(maximum)
            return value
        if selected_type == "string":
            return self._string_value(schema, schema_path)
        if selected_type == "array":
            items = schema.get("items", True)
            minimum_count = schema.get("minItems", 0)
            desired_count = max(1 if full else 0, minimum_count)
            maximum_count = schema.get("maxItems")
            if maximum_count is not None:
                desired_count = min(desired_count, maximum_count)
            if isinstance(items, list):
                return [
                    self._synthesize(
                        target,
                        child,
                        pointer_child(pointer_child(schema_path, "items"), index),
                        full,
                        active,
                        depth + 1,
                    )
                    for index, child in enumerate(items[:desired_count])
                ]
            return [
                self._synthesize(
                    target,
                    items,
                    pointer_child(schema_path, "items"),
                    full,
                    active,
                    depth + 1,
                )
                for _ in range(desired_count)
            ]
        if selected_type == "object":
            result: dict[str, Any] = {}
            for value in all_of_values:
                if not isinstance(value, dict):
                    raise FixtureError(
                        f"non-object allOf branch in object schema {schema_path}"
                    )
                merge_object(result, value, schema_path)
            properties = schema.get("properties", {})
            required = set(schema.get("required", []))
            for name, child in sorted(properties.items()):
                if full or name in required:
                    result[name] = self._synthesize(
                        target,
                        child,
                        pointer_child(
                            pointer_child(schema_path, "properties"), name
                        ),
                        full,
                        active,
                        depth + 1,
                    )
            additional = schema.get("additionalProperties", True)
            if (
                full
                and not properties
                and isinstance(additional, dict)
            ):
                result["syntheticKey"] = self._synthesize(
                    target,
                    additional,
                    pointer_child(schema_path, "additionalProperties"),
                    full,
                    active,
                    depth + 1,
                )
            return result
        raise FixtureError(
            f"unsupported synthesis type {selected_type!r} at {schema_path}"
        )

    def _outer_combinator_value(
        self,
        target: SchemaTarget,
        schema: Mapping[str, Any],
        schema_path: str,
        keyword: str,
        full: bool,
        active: frozenset[tuple[str, str]],
        depth: int,
    ) -> Any | None:
        constraint_keys = {
            "allOf",
            "const",
            "enum",
            "items",
            "maximum",
            "maxItems",
            "maxLength",
            "minimum",
            "minItems",
            "minLength",
            "pattern",
            "properties",
            "required",
            "type",
        }
        outer_schema = {
            key: value for key, value in schema.items() if key != keyword
        }
        if not (set(outer_schema) & constraint_keys):
            return None
        return self._synthesize(
            target,
            outer_schema,
            schema_path,
            full,
            active,
            depth + 1,
        )

    def _recursive_base(
        self,
        target: SchemaTarget,
        schema: Any,
        schema_path: str,
        active: frozenset[tuple[str, str]],
        depth: int,
    ) -> Any:
        if self._schema_allows_null(target, schema, schema_path):
            return None
        resolved, resolved_path = self.catalog.resolve(target, schema, schema_path)
        if isinstance(resolved, dict):
            expected = resolved.get("type")
            expected_types = [expected] if isinstance(expected, str) else expected
            if expected_types and "object" in expected_types:
                result: dict[str, Any] = {}
                properties = resolved.get("properties", {})
                for name in resolved.get("required", []):
                    child = properties.get(name, True)
                    result[name] = self._synthesize(
                        target,
                        child,
                        pointer_child(
                            pointer_child(resolved_path, "properties"), name
                        ),
                        False,
                        active,
                        depth + 1,
                    )
                return result
            if expected_types and "array" in expected_types:
                return []
        return self._synthesize(
            target,
            resolved,
            resolved_path,
            False,
            frozenset(),
            depth + 1,
        )

    def _schema_is_null(
        self, target: SchemaTarget, schema: Any, schema_path: str
    ) -> bool:
        resolved, _ = self.catalog.resolve(target, schema, schema_path)
        return isinstance(resolved, dict) and resolved.get("type") == "null"

    def _schema_allows_null(
        self, target: SchemaTarget, schema: Any, schema_path: str
    ) -> bool:
        validator = self.catalog.target_validator(target)
        return not validator.validate_subschema(None, schema, schema_path)

    def _string_value(self, schema: Mapping[str, Any], schema_path: str) -> str:
        minimum = int(schema.get("minLength", 0))
        maximum = schema.get("maxLength")
        digest = hashlib.sha256(schema_path.encode("utf-8")).hexdigest()[:10]
        candidates = [
            f"synthetic-{digest}",
            "item-42",
            "synthetic",
            "a",
            "0",
            "",
        ]
        pattern = schema.get("pattern")
        for candidate in candidates:
            if len(candidate) < minimum:
                candidate += "x" * (minimum - len(candidate))
            if maximum is not None:
                candidate = candidate[: int(maximum)]
            if len(candidate) < minimum:
                continue
            if pattern is None or re.search(pattern, candidate):
                return candidate
        raise FixtureError(
            f"unable to synthesize safe string matching pattern at {schema_path}"
        )

    def _merge_all_of(self, values: Sequence[Any], schema_path: str) -> Any:
        if all(isinstance(value, dict) for value in values):
            result: dict[str, Any] = {}
            for value in values:
                merge_object(result, value, schema_path)
            return result
        first = values[0]
        if all(canonical_json(value) == canonical_json(first) for value in values):
            return first
        raise FixtureError(f"conflicting allOf samples at {schema_path}")


def math_ceiling(value: int | float) -> int:
    integer = int(value)
    return integer if integer >= value else integer + 1


def merge_object(target: MutableMapping[str, Any], source: Mapping[str, Any], path: str) -> None:
    for key, value in source.items():
        if key in target and canonical_json(target[key]) != canonical_json(value):
            raise FixtureError(f"conflicting generated object field {key!r} at {path}")
        target[key] = copy.deepcopy(value)


def combine_values(outer: Any | None, branch: Any, path: str) -> Any:
    if outer is None:
        return branch
    if isinstance(outer, dict) and isinstance(branch, dict):
        result = copy.deepcopy(outer)
        merge_object(result, branch, path)
        return result
    if canonical_json(outer) == canonical_json(branch):
        return branch
    raise FixtureError(f"combinator branch conflicts with outer schema at {path}")


def matching_one_of_branches(
    catalog: SchemaCatalog, target: SchemaTarget, instance: Any, schema: Any, schema_path: str
) -> tuple[int, ...]:
    resolved, resolved_path = catalog.resolve(target, schema, schema_path)
    if not isinstance(resolved, dict) or "oneOf" not in resolved:
        return ()
    validator = catalog.target_validator(target)
    return tuple(
        index
        for index, branch in enumerate(resolved["oneOf"])
        if not validator.validate_subschema(
            instance,
            branch,
            pointer_child(pointer_child(resolved_path, "oneOf"), index),
        )
    )


@dataclass(frozen=True)
class RequiredLocation:
    instance_path: tuple[str | int, ...]
    schema: Any
    schema_path: str
    required_owner_instance_path: tuple[str | int, ...]
    required_owner_schema: Any
    required_owner_schema_path: str


def _collect_property_locations(
    catalog: SchemaCatalog,
    target: SchemaTarget,
    instance: Any,
    *,
    required_properties: bool,
) -> list[RequiredLocation]:
    validator = catalog.target_validator(target)
    found: dict[tuple[str | int, ...], RequiredLocation] = {}
    active: set[tuple[tuple[str | int, ...], str]] = set()

    def walk(
        value: Any,
        schema: Any,
        schema_path: str,
        instance_path: tuple[str | int, ...],
    ) -> None:
        if isinstance(schema, bool):
            return
        if not isinstance(schema, dict):
            return
        if "$ref" in schema:
            resolved, resolved_path = validator.resolve_reference(
                schema["$ref"], pointer_child(schema_path, "$ref")
            )
            marker = (instance_path, resolved_path)
            if marker in active:
                return
            active.add(marker)
            walk(value, resolved, resolved_path, instance_path)
            active.remove(marker)
            return

        for index, child in enumerate(schema.get("allOf", [])):
            walk(
                value,
                child,
                pointer_child(pointer_child(schema_path, "allOf"), index),
                instance_path,
            )
        for keyword in ("oneOf", "anyOf"):
            branches = schema.get(keyword, [])
            matches = [
                (index, child)
                for index, child in enumerate(branches)
                if not validator.validate_subschema(
                    value,
                    child,
                    pointer_child(pointer_child(schema_path, keyword), index),
                )
            ]
            # oneOf has exactly one; for anyOf the first deterministic matching
            # branch supplies the fixture's exercised constraints.
            if keyword == "anyOf":
                matches = matches[:1]
            for index, child in matches:
                walk(
                    value,
                    child,
                    pointer_child(pointer_child(schema_path, keyword), index),
                    instance_path,
                )

        if isinstance(value, dict):
            properties = schema.get("properties", {})
            required_names = set(schema.get("required", []))
            selected_names = (
                required_names
                if required_properties
                else set(properties) - required_names
            )
            for name in sorted(selected_names):
                if name in value:
                    property_schema = properties.get(name, True)
                    location = RequiredLocation(
                        instance_child(instance_path, name),
                        property_schema,
                        pointer_child(
                            pointer_child(schema_path, "properties"), name
                        ),
                        instance_path,
                        schema,
                        schema_path,
                    )
                    found.setdefault(location.instance_path, location)
            for name, child in properties.items():
                if name in value:
                    walk(
                        value[name],
                        child,
                        pointer_child(
                            pointer_child(schema_path, "properties"), name
                        ),
                        instance_child(instance_path, name),
                    )
            additional = schema.get("additionalProperties", True)
            if isinstance(additional, dict):
                for name in sorted(set(value) - set(properties)):
                    walk(
                        value[name],
                        additional,
                        pointer_child(schema_path, "additionalProperties"),
                        instance_child(instance_path, name),
                    )
        elif isinstance(value, list):
            items = schema.get("items")
            if isinstance(items, list):
                for index, (child_value, child) in enumerate(zip(value, items)):
                    walk(
                        child_value,
                        child,
                        pointer_child(pointer_child(schema_path, "items"), index),
                        instance_child(instance_path, index),
                    )
            elif isinstance(items, dict):
                for index, child_value in enumerate(value):
                    walk(
                        child_value,
                        items,
                        pointer_child(schema_path, "items"),
                        instance_child(instance_path, index),
                    )

    walk(instance, target.schema, target.schema_path, ())
    return [found[path] for path in sorted(found, key=lambda item: tuple(map(str, item)))]


def collect_required_locations(
    catalog: SchemaCatalog,
    target: SchemaTarget,
    instance: Any,
) -> list[RequiredLocation]:
    return _collect_property_locations(
        catalog,
        target,
        instance,
        required_properties=True,
    )


def collect_optional_present_locations(
    catalog: SchemaCatalog,
    target: SchemaTarget,
    instance: Any,
) -> list[RequiredLocation]:
    return _collect_property_locations(
        catalog,
        target,
        instance,
        required_properties=False,
    )


def schema_fixture_coverage(
    catalog: SchemaCatalog,
    target: SchemaTarget,
    instance: Any,
    *,
    omitted_schema_paths: Sequence[str] = (),
    directions_exercised: Sequence[str] = (),
) -> dict[str, Any]:
    """Describe schema facts proved by one independently validated fixture.

    Property paths are schema paths rather than C++ field names.  That keeps
    the evidence tied to the vendored pin and lets completeness be recomputed
    solely from the indexed corpus.  Omitted paths are explicit because an
    absent property cannot be discovered by walking an instance.
    """

    required = collect_required_locations(catalog, target, instance)
    optional = collect_optional_present_locations(catalog, target, instance)
    locations = {
        location.schema_path: location
        for location in (*required, *optional)
    }
    validator = catalog.target_validator(target)
    nullable = {
        schema_path
        for schema_path, location in locations.items()
        if not validator.validate_subschema(
            None, location.schema, location.schema_path
        )
    }
    nullable_null = {
        location.schema_path
        for location in (*required, *optional)
        if location.schema_path in nullable
        and get_instance_path(instance, location.instance_path) is None
    }
    nullable_value = nullable - nullable_null
    return {
        "property_schema_paths_present": sorted(locations),
        "optional_property_schema_paths_present": sorted(
            location.schema_path for location in optional
        ),
        "optional_property_schema_paths_omitted": sorted(
            set(omitted_schema_paths)
        ),
        "nullable_property_schema_paths": sorted(nullable),
        "nullable_value_schema_paths": sorted(nullable_value),
        "nullable_null_schema_paths": sorted(nullable_null),
        "directions_exercised": sorted(set(directions_exercised)),
    }


WRONG_TYPE_CANDIDATES = (None, False, 0, "wrong-type", [], {})
NO_WRONG_TYPE_MUTATION = object()


def mutation_evidence(
    catalog: SchemaCatalog, target: SchemaTarget, instance: Any
) -> dict[str, Any]:
    validator = catalog.target_validator(target)
    required = collect_required_locations(catalog, target, instance)
    optional_present = collect_optional_present_locations(
        catalog, target, instance
    )
    removal_records: list[dict[str, Any]] = []
    alternative_branch_records: list[dict[str, Any]] = []
    optional_omission_records: list[dict[str, Any]] = []
    optional_not_global_records: list[dict[str, Any]] = []
    wrong_type_records: list[dict[str, Any]] = []
    for location in required:
        mutated = copy.deepcopy(instance)
        parent, field = get_parent_path(mutated, location.instance_path)
        if not isinstance(parent, dict) or not isinstance(field, str):
            raise FixtureError(
                f"required location is not an object property: {json_path(location.instance_path)}"
            )
        parent.pop(field)
        root_diagnostics = validator.validate_subschema(
            mutated, target.schema, target.schema_path
        )
        required_owner = get_instance_path(
            mutated, location.required_owner_instance_path
        )
        intended_branch_diagnostics = validator.validate_subschema(
            required_owner,
            location.required_owner_schema,
            location.required_owner_schema_path,
        )
        if not intended_branch_diagnostics:
            raise FixtureError(
                "selected schema fragment accepted removal of its required "
                f"property at {json_path(location.instance_path)}"
            )
        if not root_diagnostics:
            # Draft-07 anyOf legitimately permits the mutated instance to
            # select another branch. Keep that full-root result, but prove
            # independently that the exact schema fragment selected during
            # fixture generation rejects removal of its required property.
            alternative_branch_records.append(
                {
                    "instance_path": json_path(location.instance_path),
                    "reason": (
                        "root schema accepts an alternative branch; the "
                        "selected fixture branch rejects the removal"
                    ),
                }
            )
        diagnostics = root_diagnostics or intended_branch_diagnostics
        removal_records.append(
            {
                "instance_path": json_path(location.instance_path),
                "diagnostic_codes": sorted({item.code for item in diagnostics}),
                "validation_scope": (
                    "root_schema"
                    if root_diagnostics
                    else "selected_schema_fragment"
                ),
            }
        )

        original = get_instance_path(instance, location.instance_path)
        selected: Any = NO_WRONG_TYPE_MUTATION
        selected_codes: list[str] = []
        for candidate in WRONG_TYPE_CANDIDATES:
            if canonical_json(candidate) == canonical_json(original):
                continue
            mutated = copy.deepcopy(instance)
            parent, field = get_parent_path(mutated, location.instance_path)
            parent[field] = copy.deepcopy(candidate)
            root_diagnostics = validator.validate_subschema(
                mutated, target.schema, target.schema_path
            )
            required_owner = get_instance_path(
                mutated, location.required_owner_instance_path
            )
            intended_branch_diagnostics = validator.validate_subschema(
                required_owner,
                location.required_owner_schema,
                location.required_owner_schema_path,
            )
            diagnostics = root_diagnostics or intended_branch_diagnostics
            if diagnostics:
                selected = candidate
                selected_codes = sorted({item.code for item in diagnostics})
                break
        wrong_type_records.append(
            {
                "instance_path": json_path(location.instance_path),
                "mutation_found": selected is not NO_WRONG_TYPE_MUTATION,
                "replacement_type": (
                    instance_type(selected)
                    if selected is not NO_WRONG_TYPE_MUTATION
                    else None
                ),
                "diagnostic_codes": selected_codes,
                "exclusion": (
                    None
                    if selected is not NO_WRONG_TYPE_MUTATION
                    else "schema accepts every JSON type candidate"
                ),
            }
        )
    for location in optional_present:
        mutated = copy.deepcopy(instance)
        parent, field = get_parent_path(mutated, location.instance_path)
        if not isinstance(parent, dict) or not isinstance(field, str):
            raise FixtureError(
                f"optional location is not an object property: {json_path(location.instance_path)}"
            )
        parent.pop(field)
        diagnostics = validator.validate_subschema(
            mutated, target.schema, target.schema_path
        )
        if diagnostics:
            optional_not_global_records.append(
                {
                    "instance_path": json_path(location.instance_path),
                    "diagnostic_codes": sorted(
                        {item.code for item in diagnostics}
                    ),
                    "reason": (
                        "property is optional in one selected schema fragment "
                        "but constrained by another active fragment"
                    ),
                }
            )
            continue
        optional_omission_records.append(
            {
                "instance_path": json_path(location.instance_path),
                "accepted": True,
            }
        )
    return {
        "selected_branch_required_locations": len(required),
        "required_locations": len(removal_records),
        "required_field_mutations": len(removal_records),
        "all_required_field_mutations_rejected": (
            len(removal_records) == len(required)
        ),
        "alternative_branch_acceptances": len(alternative_branch_records),
        "selected_branch_optional_present_locations": len(optional_present),
        "globally_optional_present_locations": len(
            optional_omission_records
        ),
        "optional_omission_mutations": len(optional_omission_records),
        "optional_not_globally_optional_locations": len(
            optional_not_global_records
        ),
        "all_globally_optional_omissions_accepted": True,
        "wrong_type_mutations": sum(
            record["mutation_found"] for record in wrong_type_records
        ),
        "wrong_type_unconstrained_exclusions": sum(
            record["exclusion"] is not None for record in wrong_type_records
        ),
        "removals": removal_records,
        "alternative_branch_removals": alternative_branch_records,
        "optional_omissions": optional_omission_records,
        "optional_not_globally_optional": optional_not_global_records,
        "wrong_types": wrong_type_records,
    }


def instance_type(value: Any) -> str:
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
    return type(value).__name__


def contract_map(
    manifest: Mapping[str, Any], contracts: Mapping[str, Any]
) -> dict[SurfaceKey, Mapping[str, Any]]:
    rows = contracts.get("contracts")
    if not isinstance(rows, list):
        raise FixtureError("operation-contracts.json lacks contracts array")
    result: dict[SurfaceKey, Mapping[str, Any]] = {}
    for row in rows:
        key = SurfaceKey.from_contract(row["surface_key"])
        if key in result:
            raise FixtureError(f"duplicate operation contract {key.compact()}")
        result[key] = row
    stable_requests = {
        SurfaceKey.from_manifest(entry): entry
        for entry in manifest["entries"]
        if entry["stability"] == "stable"
        and entry["category"] in {CLIENT_REQUEST, SERVER_REQUEST}
    }
    if set(result) != set(stable_requests):
        missing = sorted(key.compact() for key in set(stable_requests) - set(result))
        stale = sorted(key.compact() for key in set(result) - set(stable_requests))
        raise FixtureError(
            f"operation contracts mismatch stable request roots; "
            f"missing={missing}, stale={stale}"
        )
    for key, row in result.items():
        manifest_type = stable_requests[key]["params"]["type"] or "Unit"
        parameter_type = row.get("parameter_type_identity")
        if parameter_type != manifest_type:
            raise FixtureError(
                f"contract parameter type mismatch for {key.compact()}: "
                f"{parameter_type!r} != {manifest_type!r}"
            )
        result_type = row.get("result_type_identity")
        kind = row.get("result_contract_kind")
        if kind == "Unresolved" or result_type in {None, "", "Unresolved"}:
            raise FixtureError(
                f"unresolved operation result contract for {key.compact()}"
            )
        if kind == "Unit" and result_type != "Unit":
            raise FixtureError(
                f"Unit result contract has non-Unit type for {key.compact()}"
            )
    if len(result) != 97:
        raise FixtureError(f"expected 97 stable operation contracts, got {len(result)}")
    return result


def branch_for_union_identity(
    catalog: SchemaCatalog,
    target: SchemaTarget,
    field: str,
    name: str,
) -> tuple[int, Any, Any | None, tuple[str, Any] | None]:
    union, union_path = catalog.resolve(
        target, target.schema, target.schema_path
    )
    if not isinstance(union, dict):
        raise FixtureError(f"union target is not an object: {target.schema_path}")
    branches = union.get("oneOf", union.get("anyOf"))
    keyword = "oneOf" if "oneOf" in union else "anyOf"
    if not isinstance(branches, list):
        raise FixtureError(f"{target.schema_path} is not a union")
    matches: list[tuple[int, Any, Any | None, tuple[str, Any] | None]] = []
    validator = catalog.target_validator(target)
    for index, branch in enumerate(branches):
        resolved, branch_path = validator.resolve_reference(
            branch["$ref"],
            pointer_child(
                pointer_child(pointer_child(union_path, keyword), index), "$ref"
            ),
        ) if isinstance(branch, dict) and "$ref" in branch else (
            branch,
            pointer_child(pointer_child(union_path, keyword), index),
        )
        if not isinstance(resolved, dict):
            continue
        if field == "$variant":
            enum_values = resolved.get("enum")
            if isinstance(enum_values, list) and name in enum_values:
                matches.append((index, branch, name, None))
                continue
            required = resolved.get("required", [])
            properties = resolved.get("properties", {})
            if (
                isinstance(required, list)
                and name in required
                and name in properties
            ):
                matches.append((index, branch, None, None))
            continue
        properties = resolved.get("properties", {})
        property_schema = properties.get(field)
        if not isinstance(property_schema, dict):
            continue
        property_resolved, _ = catalog.resolve(
            target,
            property_schema,
            pointer_child(pointer_child(branch_path, "properties"), field),
        )
        if not isinstance(property_resolved, dict):
            continue
        if property_resolved.get("const") == name or property_resolved.get(
            "enum"
        ) == [name]:
            matches.append((index, branch, None, (field, name)))
    if len(matches) != 1:
        raise FixtureError(
            f"{target.schema_path} {field}={name!r} selects "
            f"{len(matches)} union branches"
        )
    return matches[0]


def validate_codex_error_rule_sets(
    manifest: Mapping[str, Any], schema: Mapping[str, Any]
) -> None:
    manifest_names = {
        str(entry["name"])
        for entry in manifest["entries"]
        if entry["stability"] == "stable"
        and entry["category"] == TAGGED_UNION_DISCRIMINATOR
        and entry["domain"] == "CodexErrorInfo"
        and entry["discriminator_field"] == "$variant"
    }
    reviewed_names = set(CODEX_ERROR_INFO_IDENTITIES)
    if (
        len(CODEX_ERROR_INFO_IDENTITIES) != len(reviewed_names)
        or manifest_names != reviewed_names
    ):
        raise FixtureError(
            "CODEX_ERROR_INFO_MANIFEST_RULE_MISMATCH: "
            f"missing={sorted(manifest_names - reviewed_names)!r} "
            f"stale={sorted(reviewed_names - manifest_names)!r}"
        )

    reviewed_http_names = set(CODEX_ERROR_INFO_HTTP_IDENTITIES)
    if (
        len(CODEX_ERROR_INFO_HTTP_IDENTITIES) != len(reviewed_http_names)
        or not reviewed_http_names < reviewed_names
        or "activeTurnNotSteerable" in reviewed_http_names
    ):
        raise FixtureError(
            "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: invalid reviewed HTTP "
            "identity subset"
        )

    branches = schema.get("oneOf")
    if not isinstance(branches, list):
        raise FixtureError(
            "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: CodexErrorInfo is not "
            "an exact oneOf union"
        )

    scalar_names: set[str] = set()
    object_schemas: dict[str, Mapping[str, Any]] = {}
    for branch in branches:
        if not isinstance(branch, dict):
            raise FixtureError(
                "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: non-object branch"
            )
        if branch.get("type") == "string":
            values = branch.get("enum")
            if not isinstance(values, list) or not all(
                isinstance(value, str) for value in values
            ):
                raise FixtureError(
                    "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: scalar branch "
                    "lacks a string enum"
                )
            scalar_names.update(values)
            continue
        if branch.get("type") != "object":
            raise FixtureError(
                "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: unsupported branch "
                "shape"
            )
        required = branch.get("required")
        properties = branch.get("properties")
        if (
            not isinstance(required, list)
            or len(required) != 1
            or not isinstance(required[0], str)
            or not isinstance(properties, dict)
            or set(properties) != {required[0]}
            or branch.get("additionalProperties") is not False
            or not isinstance(properties[required[0]], dict)
        ):
            raise FixtureError(
                "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: object branch does "
                "not have one exact discriminator property"
            )
        name = required[0]
        if name in object_schemas:
            raise FixtureError(
                "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: duplicate object "
                f"branch {name!r}"
            )
        object_schemas[name] = properties[name]

    expected_scalar_names = (
        reviewed_names - reviewed_http_names - {"activeTurnNotSteerable"}
    )
    if (
        scalar_names != expected_scalar_names
        or set(object_schemas)
        != reviewed_http_names | {"activeTurnNotSteerable"}
        or scalar_names | set(object_schemas) != manifest_names
    ):
        raise FixtureError(
            "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: reviewed scalar/object "
            "partition differs from the pinned schema"
        )

    for name in reviewed_http_names:
        nested = object_schemas[name]
        properties = nested.get("properties")
        status = (
            properties.get("httpStatusCode")
            if isinstance(properties, dict)
            else None
        )
        if (
            nested.get("type") != "object"
            or nested.get("required", []) != []
            or not isinstance(properties, dict)
            or set(properties) != {"httpStatusCode"}
            or not isinstance(status, dict)
            or set(status.get("type", [])) != {"integer", "null"}
            or status.get("format") != "uint16"
            or status.get("minimum") != 0.0
        ):
            raise FixtureError(
                "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: reviewed HTTP "
                f"shape differs for {name!r}"
            )

    active = object_schemas["activeTurnNotSteerable"]
    active_properties = active.get("properties")
    turn_kind = (
        active_properties.get("turnKind")
        if isinstance(active_properties, dict)
        else None
    )
    if (
        active.get("type") != "object"
        or active.get("required") != ["turnKind"]
        or not isinstance(active_properties, dict)
        or set(active_properties) != {"turnKind"}
        or not isinstance(turn_kind, dict)
        or turn_kind.get("$ref") != "#/definitions/NonSteerableTurnKind"
    ):
        raise FixtureError(
            "CODEX_ERROR_INFO_SCHEMA_RULE_MISMATCH: reviewed "
            "activeTurnNotSteerable shape differs"
        )


def schema_target_from_record(
    catalog: SchemaCatalog, schema_record: Mapping[str, Any]
) -> SchemaTarget:
    if "inline" in schema_record:
        document = schema_record["inline"]
        return SchemaTarget(
            document,
            document,
            str(schema_record.get("pointer", "#")),
            None,
            None,
            document,
        )
    relative = str(schema_record["file"])
    path = catalog.root / relative
    if sha256_file(path) != schema_record["sha256"]:
        raise FixtureError(f"fixture schema hash is stale: {relative}")
    document = catalog.load(path)
    validator = catalog.validator(path)
    pointer = str(schema_record["pointer"])
    if pointer == "#":
        schema = document
    else:
        schema, resolved_path = validator.resolve_reference(
            pointer, "#/fixturePointer"
        )
        if resolved_path != pointer:
            raise FixtureError(
                f"fixture schema pointer normalized unexpectedly: {pointer}"
            )
    return SchemaTarget(
        document,
        schema,
        pointer,
        relative,
        schema_record["sha256"],
    )


def root_slice(category: str, name: str) -> tuple[str, str]:
    if category == CLIENT_NOTIFICATION and name == "initialized":
        return "A1.0", "Common"
    if category == SERVER_NOTIFICATION and name == "error":
        return "A1.0", "Common"
    if name == "configWarning":
        return "A1.2", SLICE_MODULES["A1.2"]
    if name in {
        "guardianWarning",
        "permissionProfile/list",
        "thread/approveGuardianDeniedAction",
    }:
        return "A1.3", SLICE_MODULES["A1.3"]

    prefix = name.split("/", 1)[0]
    if prefix in {"thread", "turn"}:
        return "A1.1", SLICE_MODULES["A1.1"]
    if prefix == "item" and category == SERVER_NOTIFICATION:
        if any(
            marker in name
            for marker in (
                "autoApprovalReview",
                "requestApproval",
                "guardian",
                "permissions",
            )
        ):
            return "A1.3", SLICE_MODULES["A1.3"]
        return "A1.1", SLICE_MODULES["A1.1"]
    if prefix in {
        "account",
        "model",
        "modelProvider",
        "config",
        "configRequirements",
        "experimentalFeature",
    }:
        return "A1.2", SLICE_MODULES["A1.2"]
    if prefix in {
        "command",
        "fs",
        "review",
        "fuzzyFileSearch",
        "execCommandApproval",
        "applyPatchApproval",
    }:
        return "A1.3", SLICE_MODULES["A1.3"]
    if prefix == "item" and category == SERVER_REQUEST:
        if any(
            marker in name
            for marker in (
                "commandExecution",
                "fileChange",
                "permissions",
            )
        ):
            return "A1.3", SLICE_MODULES["A1.3"]
    if name == "initialize":
        return "A1.0", "Common"
    if name in A1_4_PUBLIC_ROOTS.get(category, frozenset()):
        return "A1.4", SLICE_MODULES["A1.4"]
    raise FixtureError(
        "A1_SLICE_UNRECOGNIZED_STABLE_ROOT: "
        f"category={category!r} name={name!r}"
    )


@dataclass(frozen=True, order=True)
class DefinitionId:
    namespace: str
    name: str

    def to_json(self) -> dict[str, str]:
        return {"namespace": self.namespace, "name": self.name}


@dataclass(frozen=True)
class RootReachability:
    root_id: str
    surface_key: SurfaceKey | None
    role: str
    slice: str
    definitions: frozenset[DefinitionId]

    def to_json(self) -> dict[str, Any]:
        result: dict[str, Any] = {
            "root_id": self.root_id,
            "role": self.role,
            "slice": self.slice,
        }
        if self.surface_key is not None:
            result["surface_key"] = self.surface_key.to_json()
        return result


def definition_graph(
    aggregate: Mapping[str, Any],
) -> tuple[dict[DefinitionId, Any], dict[DefinitionId, set[DefinitionId]]]:
    raw_definitions = aggregate.get("definitions")
    if not isinstance(raw_definitions, dict):
        raise FixtureError("combined aggregate lacks definitions")
    v2 = raw_definitions.get("v2")
    if not isinstance(v2, dict):
        raise FixtureError("combined aggregate lacks definitions.v2 namespace")
    nodes: dict[DefinitionId, Any] = {}
    for name, schema in raw_definitions.items():
        if name != "v2":
            nodes[DefinitionId("legacy", name)] = schema
    for name, schema in v2.items():
        nodes[DefinitionId("v2", name)] = schema

    def references(value: Any) -> Iterator[DefinitionId]:
        if isinstance(value, dict):
            reference = value.get("$ref")
            if isinstance(reference, str):
                match = re.fullmatch(
                    r"#/definitions/(?:(v2)/)?([^/]+)", reference
                )
                if match:
                    yield DefinitionId(
                        "v2" if match.group(1) else "legacy",
                        match.group(2).replace("~1", "/").replace("~0", "~"),
                    )
            for child in value.values():
                yield from references(child)
        elif isinstance(value, list):
            for child in value:
                yield from references(child)

    edges: dict[DefinitionId, set[DefinitionId]] = {}
    for identity, schema in nodes.items():
        outgoing = set(references(schema))
        unknown = outgoing - set(nodes)
        if unknown:
            raise FixtureError(
                f"definition graph has unresolved refs from {identity}: {sorted(unknown)}"
            )
        edges[identity] = outgoing
    return nodes, edges


def transitive_definitions(
    starts: Iterable[DefinitionId],
    edges: Mapping[DefinitionId, set[DefinitionId]],
) -> frozenset[DefinitionId]:
    reached: set[DefinitionId] = set()
    pending = list(starts)
    while pending:
        current = pending.pop()
        if current in reached:
            continue
        if current not in edges:
            raise FixtureError(f"unknown definition graph root {current}")
        reached.add(current)
        pending.extend(sorted(edges[current] - reached))
    return frozenset(reached)


def schema_references(value: Any) -> set[DefinitionId]:
    result: set[DefinitionId] = set()

    def walk(node: Any) -> None:
        if isinstance(node, dict):
            reference = node.get("$ref")
            if isinstance(reference, str):
                match = re.fullmatch(
                    r"#/definitions/(?:(v2)/)?([^/]+)", reference
                )
                if match:
                    result.add(
                        DefinitionId(
                            "v2" if match.group(1) else "legacy",
                            match.group(2).replace("~1", "/").replace("~0", "~"),
                        )
                    )
            for child in node.values():
                walk(child)
        elif isinstance(node, list):
            for child in node:
                walk(child)

    walk(value)
    return result


def locate_definition_for_type(
    catalog: SchemaCatalog,
    nodes: Mapping[DefinitionId, Any],
    type_identity: str,
) -> DefinitionId | None:
    if type_identity == "Unit":
        return None
    path = catalog._type_paths.get(type_identity)
    if path is None:
        raise FixtureError(f"no standalone root for graph type {type_identity}")
    relative = path.relative_to(catalog.stable_root)
    namespace = "v2" if relative.parts[0] == "v2" else "legacy"
    identity = DefinitionId(namespace, type_identity)
    if identity not in nodes:
        raise FixtureError(
            f"aggregate graph lacks {namespace} definition {type_identity}"
        )
    return identity


def build_reachability_and_assignments(
    catalog: SchemaCatalog,
    manifest: Mapping[str, Any],
    contracts: Mapping[SurfaceKey, Mapping[str, Any]],
) -> tuple[dict[str, Any], dict[str, Any]]:
    aggregate_path = (
        catalog.stable_root / "codex_app_server_protocol.schemas.json"
    )
    aggregate = catalog.load(aggregate_path)
    nodes, edges = definition_graph(aggregate)
    aggregate_definitions = aggregate["definitions"]

    roots: list[RootReachability] = []
    stable_entries = [
        entry
        for entry in manifest["entries"]
        if entry["stability"] == "stable"
    ]
    stable_by_key = {
        SurfaceKey.from_manifest(entry): entry for entry in stable_entries
    }

    method_unions = {
        category: aggregate_definitions[
            {
                CLIENT_REQUEST: "ClientRequest",
                CLIENT_NOTIFICATION: "ClientNotification",
                SERVER_NOTIFICATION: "ServerNotification",
                SERVER_REQUEST: "ServerRequest",
            }[category]
        ]
        for category in METHOD_SCHEMA_FILES
    }

    for key, entry in sorted(stable_by_key.items()):
        if key.category not in METHOD_SCHEMA_FILES:
            continue
        union = method_unions[key.category]
        matching = [
            branch
            for branch in union["oneOf"]
            if (
                branch.get("properties", {})
                .get("method", {})
                .get("enum")
                == [key.name]
            )
        ]
        if len(matching) != 1:
            raise FixtureError(
                f"aggregate method root mismatch for {key.compact()}"
            )
        branch = matching[0]
        starts = schema_references(branch.get("properties", {}).get("params", {}))
        if key.category in {CLIENT_REQUEST, SERVER_REQUEST}:
            result_type = str(
                contracts[key].get(
                    "result_schema_type_identity",
                    contracts[key]["result_type_identity"],
                )
            )
            result_definition = locate_definition_for_type(
                catalog, nodes, result_type
            )
            if result_definition is not None:
                starts.add(result_definition)
        reached = transitive_definitions(starts, edges) if starts else frozenset()
        slice_name, _ = root_slice(key.category, key.name)
        roots.append(
            RootReachability(
                key.compact(),
                key,
                "method",
                slice_name,
                reached,
            )
        )

    for domain in ("ThreadItem", "ResponseItem"):
        identity = DefinitionId("v2", domain)
        reached = transitive_definitions([identity], edges)
        roots.append(
            RootReachability(
                f"item_union:{domain}",
                None,
                "item_union",
                "A1.1",
                reached,
            )
        )

    reaching_by_domain: dict[str, list[RootReachability]] = {}
    for root in roots:
        for definition in root.definitions:
            reaching_by_domain.setdefault(definition.name, []).append(root)

    reachability_records: list[dict[str, Any]] = []
    assignments: list[dict[str, Any]] = []
    for entry in sorted(
        manifest["entries"],
        key=lambda value: SurfaceKey.from_manifest(value),
    ):
        key = SurfaceKey.from_manifest(entry)
        stability = str(entry["stability"])
        if stability == "experimental_only":
            assignments.append(
                {
                    "surface_key": key.to_json(),
                    "stability": stability,
                    "module": "ExperimentalInventory",
                    "a1_slice": "InventoryOnly",
                    "classification": "ExperimentalInventoryOnly",
                    "reason": "experimental-only roots remain registered inventory and are not typed during A1",
                }
            )
            continue

        if key.category in METHOD_SCHEMA_FILES:
            slice_name, module = root_slice(key.category, key.name)
            assignments.append(
                {
                    "surface_key": key.to_json(),
                    "stability": stability,
                    "module": module,
                    "a1_slice": slice_name,
                    "classification": "StablePublicRoot",
                    "reason": "fixed A1 domain-root assignment",
                }
            )
            continue
        if key.category == ITEM_DISCRIMINATOR:
            assignments.append(
                {
                    "surface_key": key.to_json(),
                    "stability": stability,
                    "module": SLICE_MODULES["A1.1"],
                    "a1_slice": "A1.1",
                    "classification": "StableItemRoot",
                    "reason": f"{key.domain} is owned by thread/turn/session flows",
                }
            )
            continue
        if key.category != TAGGED_UNION_DISCRIMINATOR:
            raise FixtureError(
                f"unhandled stable assignment category {key.category}"
            )

        reaching = sorted(
            {
                root.root_id: root
                for root in reaching_by_domain.get(key.domain, [])
            }.values(),
            key=lambda root: root.root_id,
        )
        if key.domain == "CodexErrorInfo":
            slice_name = "A1.0"
            module = "Common"
            classification = "CrossCuttingA1_0Exception"
            reason = "CodexErrorInfo is the fixed cross-cutting A1.0 exception"
        elif not reaching:
            slice_name = "InventoryOnly"
            module = "StableUnreachableInventory"
            classification = "StableUnreachableInventory"
            reason = "no stable public request/result/notification/item root reaches this union"
        else:
            slices = sorted(
                {root.slice for root in reaching},
                key=lambda value: SLICE_ORDER[value],
            )
            slice_name = slices[0]
            if len(slices) > 1:
                module = "Common"
                classification = "SharedCommon"
                reason = (
                    "assigned to earliest fixed slice among multiple reaching domains"
                )
            elif len(reaching) > 1:
                module = SLICE_MODULES[slice_name]
                classification = "SharedWithinSlice"
                reason = "shared by multiple stable roots in one fixed slice"
            else:
                module = SLICE_MODULES[slice_name]
                classification = "RootOwnedNestedUnion"
                reason = "assigned to its sole reaching stable root slice"
        assignments.append(
            {
                "surface_key": key.to_json(),
                "stability": stability,
                "module": module,
                "a1_slice": slice_name,
                "classification": classification,
                "reason": reason,
            }
        )
        reachability_records.append(
            {
                "surface_key": key.to_json(),
                "definition_domain": key.domain,
                "reaching_roots": [root.to_json() for root in reaching],
                "reaching_root_count": len(reaching),
                "reaching_slices": sorted(
                    {root.slice for root in reaching},
                    key=lambda value: SLICE_ORDER[value],
                ),
                "assigned_slice": slice_name,
                "classification": classification,
            }
        )

    if len(assignments) != 387:
        raise FixtureError(
            f"slice assignment must contain all 387 A0 identities, got {len(assignments)}"
        )
    assignment_keys = [
        SurfaceKey.from_contract(record["surface_key"]) for record in assignments
    ]
    if len(set(assignment_keys)) != 387:
        raise FixtureError("slice assignment contains duplicate exact identities")
    for record in assignments:
        # `slice` is the canonical consumer field. `a1_slice` is retained as
        # a descriptive compatibility alias for reports and fixture records.
        record["slice"] = record["a1_slice"]

    assignment_counts: dict[str, int] = {}
    classification_counts: dict[str, int] = {}
    for record in assignments:
        assignment_counts[record["a1_slice"]] = (
            assignment_counts.get(record["a1_slice"], 0) + 1
        )
        classification_counts[record["classification"]] = (
            classification_counts.get(record["classification"], 0) + 1
        )
    reachability = {
        "generated_notice": "Generated by tools/codex/app_server_fixtures.py; do not edit.",
        "format_version": FORMAT_VERSION,
        "codex_version": CODEX_VERSION,
        "root_count": len(roots),
        "roots": [root.to_json() for root in sorted(roots, key=lambda item: item.root_id)],
        "records": reachability_records,
        "counts": {
            "nested_union_identities": len(reachability_records),
            "stable_unreachable": sum(
                record["classification"] == "StableUnreachableInventory"
                for record in reachability_records
            ),
            "shared_common": sum(
                record["classification"] == "SharedCommon"
                for record in reachability_records
            ),
        },
    }
    assignment = {
        "generated_notice": "Generated by tools/codex/app_server_fixtures.py; do not edit.",
        "format_version": FORMAT_VERSION,
        "codex_version": CODEX_VERSION,
        "counts": {
            "total": len(assignments),
            "by_slice": dict(sorted(assignment_counts.items())),
            "by_classification": dict(sorted(classification_counts.items())),
        },
        "assignments": assignments,
    }
    return reachability, assignment


def schema_position_nodes(
    schema: Any,
    path: str = "#",
    combined_v2_namespace: bool = False,
) -> Iterator[tuple[str, Mapping[str, Any]]]:
    if isinstance(schema, bool):
        return
    if not isinstance(schema, dict):
        raise FixtureError(f"non-schema at {path}")
    yield path, schema
    for keyword in ("definitions", "properties"):
        children = schema.get(keyword, {})
        if not isinstance(children, dict):
            continue
        for name, child in sorted(children.items()):
            child_schema_path = pointer_child(pointer_child(path, keyword), name)
            if (
                combined_v2_namespace
                and keyword == "definitions"
                and name == "v2"
                and isinstance(child, dict)
            ):
                for nested_name, nested_schema in sorted(child.items()):
                    yield from schema_position_nodes(
                        nested_schema,
                        pointer_child(child_schema_path, nested_name),
                        False,
                    )
            else:
                yield from schema_position_nodes(
                    child, child_schema_path, False
                )
    additional = schema.get("additionalProperties")
    if isinstance(additional, (dict, bool)):
        yield from schema_position_nodes(
            additional, pointer_child(path, "additionalProperties"), False
        )
    items = schema.get("items")
    if isinstance(items, list):
        for index, child in enumerate(items):
            yield from schema_position_nodes(
                child, pointer_child(pointer_child(path, "items"), index), False
            )
    elif isinstance(items, (dict, bool)):
        yield from schema_position_nodes(
            items, pointer_child(path, "items"), False
        )
    for keyword in ("allOf", "anyOf", "oneOf"):
        children = schema.get(keyword, [])
        if isinstance(children, list):
            for index, child in enumerate(children):
                yield from schema_position_nodes(
                    child,
                    pointer_child(pointer_child(path, keyword), index),
                    False,
                )
    if isinstance(schema.get("not"), (dict, bool)):
        yield from schema_position_nodes(
            schema["not"], pointer_child(path, "not"), False
        )


def keyword_report(
    schema_root: Path, draft07: ModuleType
) -> dict[str, Any]:
    tree_reports: dict[str, Any] = {}
    for stability in ("stable", "experimental"):
        root = schema_root / stability
        counts: dict[str, int] = {}
        files = sorted(root.rglob("*.json"))
        unsupported_occurrences: list[dict[str, str]] = []
        for path in files:
            document = load_json(path)
            combined = path.name == "codex_app_server_protocol.schemas.json"
            for schema_path, node in schema_position_nodes(
                document, "#", combined
            ):
                for keyword in node:
                    counts[keyword] = counts.get(keyword, 0) + 1
                    if keyword not in draft07.SUPPORTED_SCHEMA_KEYWORDS:
                        unsupported_occurrences.append(
                            {
                                "file": path.relative_to(schema_root).as_posix(),
                                "schema_path": schema_path,
                                "keyword": keyword,
                            }
                        )
        if unsupported_occurrences:
            raise FixtureError(
                f"{stability} schemas use unsupported schema-position keywords: "
                f"{unsupported_occurrences[:10]}"
            )
        tree_reports[stability] = {
            "file_count": len(files),
            "actual_keyword_counts": dict(sorted(counts.items())),
            "unsupported_occurrences": [],
        }
    return {
        "generated_notice": "Generated by tools/codex/app_server_fixtures.py; do not edit.",
        "format_version": FORMAT_VERSION,
        "codex_version": CODEX_VERSION,
        "supported": {
            "validating": sorted(draft07.VALIDATING_KEYWORDS),
            "structural": sorted(draft07.STRUCTURAL_KEYWORDS),
            "annotations": sorted(draft07.ANNOTATION_KEYWORDS),
        },
        "trees": tree_reports,
        "notes": [
            "Counts are schema-position occurrences and include definitions duplicated by the upstream generator.",
            "Protocol properties named const, pattern, maximum, maxItems, maxLength, or dependencies are not counted as schema keywords.",
            "format values in the pin are schemars numeric metadata and are retained as annotations, not asserted.",
        ],
    }


class CorpusBuilder:
    def __init__(
        self,
        root: Path,
        schema_root: Path,
        manifest_path: Path,
        contracts_path: Path,
        draft07_path: Path,
    ):
        self.root = root
        self.schema_root = schema_root
        self.manifest_path = manifest_path
        self.contracts_path = contracts_path
        self.draft07_path = draft07_path
        self.draft07 = load_draft07(draft07_path)
        self.catalog = SchemaCatalog(schema_root, self.draft07)
        self.synthesizer = Synthesizer(self.catalog)
        self.manifest = load_json(manifest_path)
        self.contract_document = load_json(contracts_path)
        self.contracts = contract_map(self.manifest, self.contract_document)
        self.manifest_by_key = {
            SurfaceKey.from_manifest(entry): entry
            for entry in self.manifest["entries"]
        }
        self.assignments: dict[SurfaceKey, Mapping[str, Any]] = {}
        self.b2_shared_common_keys: tuple[SurfaceKey, ...] = ()
        self.b2_negative_coverage: dict[str, Any] = {}
        self.b2_indexed_coverage: dict[str, Any] = {}
        self.files: dict[str, bytes] = {}
        self.records: list[dict[str, Any]] = []

    def add_positive(
        self,
        fixture_id: str,
        relative_file: str,
        role: str,
        target: SchemaTarget,
        value: Any,
        surface_key: SurfaceKey | None,
        intended_branch_indices: tuple[int, ...] = (),
        omitted_schema_paths: Sequence[str] = (),
        directions_exercised: Sequence[str] = (),
    ) -> None:
        validator = self.catalog.target_validator(target)
        diagnostics = validator.validate_subschema(
            value, target.schema, target.schema_path
        )
        if diagnostics:
            raise FixtureError(
                f"generated positive fixture {fixture_id} is invalid: "
                f"{[item.to_json() for item in diagnostics[:5]]}"
            )
        if intended_branch_indices:
            matches = matching_one_of_branches(
                self.catalog,
                target,
                value,
                target.schema,
                target.schema_path,
            )
            if matches != intended_branch_indices:
                raise FixtureError(
                    f"fixture {fixture_id} matches branches {matches}, "
                    f"expected {intended_branch_indices}"
                )
        mutation = mutation_evidence(self.catalog, target, value)
        payload = encoded_json(value)
        self.files[relative_file] = payload
        assignment = (
            self.assignments.get(surface_key) if surface_key is not None else None
        )
        completeness_evidence = {
            "authoritative_root_association": True,
            "positive_fixture_coverage": True,
            "required_fields_exercised": True,
            "schema_properties_exercised": False,
            "optional_present_exercised": (
                mutation["selected_branch_optional_present_locations"]
                == mutation["globally_optional_present_locations"]
                + mutation["optional_not_globally_optional_locations"]
            ),
            "optional_omitted_exercised": (
                mutation["globally_optional_present_locations"]
                == mutation["optional_omission_mutations"]
            ),
            "nullable_semantics_exercised": False,
            "reachable_union_alternatives_exercised": role
            in {"union_branch"},
            "fixture_current": True,
            "independently_schema_validated": True,
        }
        record: dict[str, Any] = {
            "id": fixture_id,
            "file": relative_file,
            "file_sha256": sha256_bytes(payload),
            "fixture_sha256": sha256_bytes(payload),
            "role": role,
            "schema": target.to_json(),
            "validation": {
                "independent": True,
                "one_of_branch_indices": list(intended_branch_indices),
                **mutation,
            },
            "completeness_evidence": completeness_evidence,
        }
        if omitted_schema_paths or directions_exercised:
            record["schema_fixture_coverage"] = schema_fixture_coverage(
                self.catalog,
                target,
                value,
                omitted_schema_paths=omitted_schema_paths,
                directions_exercised=directions_exercised,
            )
        if surface_key is not None:
            record["protocol_surface_key"] = surface_key.to_json()
        if assignment is not None:
            record["a1_slice"] = assignment["a1_slice"]
        self.records.append(record)

    def add_negative(
        self,
        fixture_id: str,
        relative_file: str,
        role: str,
        target: SchemaTarget,
        value: Any,
        expected_codes: Sequence[str],
        surface_key: SurfaceKey | None = None,
        negative_case: str | None = None,
    ) -> None:
        diagnostics = self.catalog.target_validator(target).validate_subschema(
            value, target.schema, target.schema_path
        )
        actual_codes = sorted({item.code for item in diagnostics})
        if actual_codes != sorted(expected_codes):
            raise FixtureError(
                f"negative fixture {fixture_id} diagnostics {actual_codes}, "
                f"expected {sorted(expected_codes)}"
            )
        payload = encoded_json(value)
        self.files[relative_file] = payload
        record: dict[str, Any] = {
            "id": fixture_id,
            "file": relative_file,
            "file_sha256": sha256_bytes(payload),
            "role": role,
            "schema": target.to_json(),
            "expected_valid": False,
            "expected_diagnostic_codes": actual_codes,
        }
        if surface_key is not None:
            record["protocol_surface_key"] = surface_key.to_json()
            assignment = self.assignments.get(surface_key)
            if assignment is not None:
                record["a1_slice"] = assignment["a1_slice"]
        if negative_case is not None:
            record["negative_case"] = negative_case
        self.records.append(record)

    def build(
        self,
    ) -> tuple[
        dict[str, bytes],
        dict[str, Any],
        dict[str, Any],
        dict[str, Any],
        dict[str, Any],
        dict[str, Any],
    ]:
        reachability, assignment_document = build_reachability_and_assignments(
            self.catalog, self.manifest, self.contracts
        )
        self.assignments = {
            SurfaceKey.from_contract(record["surface_key"]): record
            for record in assignment_document["assignments"]
        }
        self.b2_shared_common_keys = derive_b2_shared_common_keys(
            self.assignments
        )

        self._build_operation_fixtures()
        self._build_baseline_fixtures()
        self._build_union_fixtures()
        self._build_b2_open_enum_fixtures()

        self.records.sort(key=lambda record: record["id"])
        fixture_counts: dict[str, int] = {}
        for record in self.records:
            fixture_counts[record["role"]] = (
                fixture_counts.get(record["role"], 0) + 1
            )
        positive_records = [
            record
            for record in self.records
            if record.get("expected_valid", True)
        ]
        positive_fixture_ids = {
            str(record["id"]) for record in positive_records
        }
        all_codex_error_alternatives_exercised = all(
            f"union:CodexErrorInfo:{name}" in positive_fixture_ids
            for name in CODEX_ERROR_INFO_IDENTITIES
        )
        for record in positive_records:
            key_record = record.get("protocol_surface_key")
            if not isinstance(key_record, dict):
                continue
            key = SurfaceKey.from_contract(key_record)
            if (
                key.category != TAGGED_UNION_DISCRIMINATOR
                or key.domain != "CodexErrorInfo"
                or key.name not in CODEX_ERROR_INFO_IDENTITIES
            ):
                continue
            if key.name in CODEX_ERROR_INFO_HTTP_IDENTITIES:
                required_fixtures = {
                    f"union:CodexErrorInfo:{key.name}",
                    f"union:CodexErrorInfo:{key.name}:http-status-null",
                    f"union:CodexErrorInfo:{key.name}:http-status-omitted",
                }
            elif key.name == "activeTurnNotSteerable":
                required_fixtures = {
                    "union:CodexErrorInfo:activeTurnNotSteerable",
                    "union:CodexErrorInfo:activeTurnNotSteerable:turn-kind-compact",
                }
            else:
                required_fixtures = {f"union:CodexErrorInfo:{key.name}"}
            properties_exercised = required_fixtures <= positive_fixture_ids
            facts = record["completeness_evidence"]
            facts["schema_properties_exercised"] = properties_exercised
            facts["nullable_semantics_exercised"] = (
                properties_exercised
                if key.name in CODEX_ERROR_INFO_HTTP_IDENTITIES
                else True
            )
            facts["reachable_union_alternatives_exercised"] = (
                all_codex_error_alternatives_exercised
            )
        self._apply_b2_indexed_completeness(
            positive_records, positive_fixture_ids
        )
        mutation_counts = {
            "selected_branch_required_locations": sum(
                record["validation"]["selected_branch_required_locations"]
                for record in positive_records
            ),
            "required_locations": sum(
                record["validation"]["required_locations"]
                for record in positive_records
            ),
            "required_field_removals_rejected": sum(
                record["validation"]["required_field_mutations"]
                for record in positive_records
            ),
            "wrong_type_mutations_rejected": sum(
                record["validation"]["wrong_type_mutations"]
                for record in positive_records
            ),
            "wrong_type_unconstrained_exclusions": sum(
                record["validation"][
                    "wrong_type_unconstrained_exclusions"
                ]
                for record in positive_records
            ),
            "alternative_branch_acceptances": sum(
                record["validation"]["alternative_branch_acceptances"]
                for record in positive_records
            ),
            "optional_present_locations": sum(
                record["validation"][
                    "selected_branch_optional_present_locations"
                ]
                for record in positive_records
            ),
            "globally_optional_locations": sum(
                record["validation"]["globally_optional_present_locations"]
                for record in positive_records
            ),
            "optional_omissions_accepted": sum(
                record["validation"]["optional_omission_mutations"]
                for record in positive_records
            ),
            "optional_cross_fragment_exclusions": sum(
                record["validation"][
                    "optional_not_globally_optional_locations"
                ]
                for record in positive_records
            ),
        }
        if (
            mutation_counts["selected_branch_required_locations"]
            != mutation_counts["required_locations"]
            or mutation_counts["required_locations"]
            != mutation_counts["required_field_removals_rejected"]
        ):
            raise FixtureError(
                "not every selected-branch required field removal was rejected"
            )
        if (
            mutation_counts["required_locations"]
            != mutation_counts["wrong_type_mutations_rejected"]
            + mutation_counts["wrong_type_unconstrained_exclusions"]
        ):
            raise FixtureError(
                "wrong-type mutations and explicit unconstrained exclusions "
                "do not cover every globally required value"
            )
        if (
            mutation_counts["globally_optional_locations"]
            != mutation_counts["optional_omissions_accepted"]
            or mutation_counts["optional_present_locations"]
            != mutation_counts["globally_optional_locations"]
            + mutation_counts["optional_cross_fragment_exclusions"]
        ):
            raise FixtureError(
                "optional-present and optional-omitted mutation evidence is incomplete"
            )
        index = {
            "generated_notice": "Generated by tools/codex/app_server_fixtures.py; do not edit.",
            "format_version": FORMAT_VERSION,
            "codex_version": CODEX_VERSION,
            "rules_version": RULES_VERSION,
            "sources": self._source_hashes(),
            "counts": {
                "total": len(self.records),
                "positive": len(positive_records),
                "negative": len(self.records) - len(positive_records),
                "by_role": dict(sorted(fixture_counts.items())),
            },
            "mutation_counts": mutation_counts,
            "a1_1_shared_common": {
                "assignment_derived_keys": [
                    key.to_json() for key in self.b2_shared_common_keys
                ],
                "indexed_schema_coverage": self.b2_indexed_coverage,
                "negative_coverage": self.b2_negative_coverage,
            },
            "fixtures": self.records,
        }
        self.files["index.json"] = encoded_json(index)

        coverage_by_key: dict[SurfaceKey, list[Mapping[str, Any]]] = {}
        for record in positive_records:
            key_record = record.get("protocol_surface_key")
            if key_record is not None:
                coverage_by_key.setdefault(
                    SurfaceKey.from_contract(key_record), []
                ).append(record)
        coverage_records: list[dict[str, Any]] = []
        for key in sorted(self.manifest_by_key):
            records = coverage_by_key.get(key, [])
            record_ids = {str(record["id"]) for record in records}
            is_codex_error = (
                key.category == TAGGED_UNION_DISCRIMINATOR
                and key.domain == "CodexErrorInfo"
                and key.name in CODEX_ERROR_INFO_IDENTITIES
            )
            if key.name in CODEX_ERROR_INFO_HTTP_IDENTITIES and is_codex_error:
                expected_property_fixtures = {
                    f"union:CodexErrorInfo:{key.name}",
                    f"union:CodexErrorInfo:{key.name}:http-status-null",
                    f"union:CodexErrorInfo:{key.name}:http-status-omitted",
                }
            elif key.name == "activeTurnNotSteerable" and is_codex_error:
                expected_property_fixtures = {
                    "union:CodexErrorInfo:activeTurnNotSteerable",
                    "union:CodexErrorInfo:activeTurnNotSteerable:turn-kind-compact",
                }
            elif is_codex_error:
                expected_property_fixtures = {
                    f"union:CodexErrorInfo:{key.name}"
                }
            else:
                expected_property_fixtures = set()
            codex_properties_exercised = (
                is_codex_error
                and expected_property_fixtures <= record_ids
            )
            codex_nullable_exercised = (
                codex_properties_exercised
                if key.name in CODEX_ERROR_INFO_HTTP_IDENTITIES
                else is_codex_error
            )
            is_b2_shared_common = key in self.b2_shared_common_keys
            b2_schema_facts = (
                self.b2_indexed_coverage.get(key.compact(), {}).get(
                    "schema_fixture_facts", {}
                )
                if is_b2_shared_common
                else {}
            )
            coverage_records.append(
                {
                    "protocol_surface_key": key.to_json(),
                    "fixture_ids": sorted(record["id"] for record in records),
                    "positive_fixture_count": len(records),
                    "fixture_current": bool(records),
                    "independently_schema_validated": bool(records),
                    "required_field_mutations": sum(
                        record["validation"]["required_field_mutations"]
                        for record in records
                    ),
                    "optional_omission_mutations": sum(
                        record["validation"]["optional_omission_mutations"]
                        for record in records
                    ),
                    "wrong_type_mutations": sum(
                        record["validation"]["wrong_type_mutations"]
                        for record in records
                    ),
                    "schema_directions_exercised": (
                        self.b2_indexed_coverage.get(
                            key.compact(), {}
                        ).get("directions_exercised", [])
                    ),
                    "schema_direction_coverage": bool(
                        self.b2_indexed_coverage.get(
                            key.compact(), {}
                        ).get("schema_direction_coverage", False)
                    ),
                    "completeness_evidence": {
                        "authoritative_root_association": bool(records),
                        "positive_fixture_coverage": bool(records),
                        "required_fields_exercised": bool(records),
                        "schema_properties_exercised": (
                            codex_properties_exercised
                            or bool(
                                b2_schema_facts.get(
                                    "schema_properties_exercised", False
                                )
                            )
                        ),
                        "optional_present_exercised": bool(records)
                        and all(
                            record["completeness_evidence"][
                                "optional_present_exercised"
                            ]
                            for record in records
                        ),
                        "optional_omitted_exercised": bool(records)
                        and all(
                            record["completeness_evidence"][
                                "optional_omitted_exercised"
                            ]
                            for record in records
                        ),
                        "nullable_semantics_exercised": (
                            codex_nullable_exercised
                            or bool(
                                b2_schema_facts.get(
                                    "nullable_semantics_exercised", False
                                )
                            )
                        ),
                        "reachable_union_alternatives_exercised": (
                            (
                                is_codex_error
                                and all_codex_error_alternatives_exercised
                            )
                            or bool(
                                b2_schema_facts.get(
                                    "reachable_union_alternatives_exercised",
                                    False,
                                )
                            )
                        ),
                        "fixture_current": bool(records),
                        "independently_schema_validated": bool(records),
                    },
                }
            )
        fixture_coverage = {
            "generated_notice": "Generated by tools/codex/app_server_fixtures.py; do not edit.",
            "format_version": FORMAT_VERSION,
            "codex_version": CODEX_VERSION,
            "sources": index["sources"],
            "counts": {
                "surface_identities": len(coverage_records),
                "identities_with_positive_fixtures": sum(
                    bool(record["positive_fixture_count"])
                    for record in coverage_records
                ),
                "positive_fixtures": len(positive_records),
                "operation_role_expected": 194,
                "operation_role_actual": sum(
                    record["role"]
                    in {
                        "client_request_params",
                        "client_request_result",
                        "server_request_params",
                        "server_request_response",
                    }
                    for record in positive_records
                ),
                "required_field_removals_rejected": mutation_counts[
                    "required_field_removals_rejected"
                ],
                "optional_present_locations": mutation_counts[
                    "optional_present_locations"
                ],
                "optional_omissions_accepted": mutation_counts[
                    "optional_omissions_accepted"
                ],
                "wrong_type_mutations_rejected": mutation_counts[
                    "wrong_type_mutations_rejected"
                ],
                "wrong_type_unconstrained_exclusions": mutation_counts[
                    "wrong_type_unconstrained_exclusions"
                ],
            },
            "a1_1_shared_common": {
                "assignment_derived_keys": [
                    key.to_json() for key in self.b2_shared_common_keys
                ],
                "indexed_schema_coverage": self.b2_indexed_coverage,
                "negative_coverage": self.b2_negative_coverage,
            },
            "fixtures": [
                record
                for record in positive_records
                if "protocol_surface_key" in record
            ],
            "identity_coverage": coverage_records,
        }
        if fixture_coverage["counts"]["operation_role_actual"] != 194:
            raise FixtureError(
                "operation fixture corpus must contain 194 request/result roles"
            )
        keywords = keyword_report(self.schema_root, self.draft07)
        return self.files, index, fixture_coverage, keywords, reachability, assignment_document

    def _source_hashes(self) -> dict[str, Any]:
        provenance = load_json(self.schema_root / "PROVENANCE.json")
        return {
            "stable_schema_aggregate_sha256": provenance["schema_trees"]["stable"][
                "aggregate_sha256"
            ],
            "experimental_schema_aggregate_sha256": provenance["schema_trees"][
                "experimental"
            ]["aggregate_sha256"],
            "surface_manifest_sha256": sha256_file(self.manifest_path),
            "operation_contracts_sha256": sha256_file(self.contracts_path),
            "validator_sha256": sha256_file(self.draft07_path),
            "generator_sha256": sha256_file(Path(__file__).resolve()),
        }

    def _apply_b2_indexed_completeness(
        self,
        positive_records: Sequence[MutableMapping[str, Any]],
        positive_fixture_ids: set[str],
    ) -> None:
        records_by_key: dict[
            SurfaceKey, list[MutableMapping[str, Any]]
        ] = {}
        for record in positive_records:
            key_record = record.get("protocol_surface_key")
            if isinstance(key_record, dict):
                records_by_key.setdefault(
                    SurfaceKey.from_contract(key_record), []
                ).append(record)

        indexed_coverage: dict[str, Any] = {}
        for key in self.b2_shared_common_keys:
            records = records_by_key.get(key, [])
            base_id = f"union:{key.domain}:{key.name}"
            base_records = [
                record for record in records if record["id"] == base_id
            ]
            if len(base_records) != 1:
                raise FixtureError(
                    f"B2 identity lacks exactly one base fixture: {key.compact()}"
                )
            base_coverage = base_records[0]["schema_fixture_coverage"]
            expected_properties = set(
                base_coverage["property_schema_paths_present"]
            )
            expected_optional = set(
                base_coverage[
                    "optional_property_schema_paths_present"
                ]
            )
            expected_nullable = set(
                base_coverage["nullable_property_schema_paths"]
            )
            present = {
                path
                for record in records
                for path in record["schema_fixture_coverage"][
                    "property_schema_paths_present"
                ]
            }
            optional_present = {
                path
                for record in records
                for path in record["schema_fixture_coverage"][
                    "optional_property_schema_paths_present"
                ]
            }
            optional_omitted = {
                path
                for record in records
                for path in record["schema_fixture_coverage"][
                    "optional_property_schema_paths_omitted"
                ]
            }
            nullable_value = {
                path
                for record in records
                for path in record["schema_fixture_coverage"][
                    "nullable_value_schema_paths"
                ]
            }
            nullable_null = {
                path
                for record in records
                for path in record["schema_fixture_coverage"][
                    "nullable_null_schema_paths"
                ]
            }
            directions = {
                direction
                for record in records
                for direction in record["schema_fixture_coverage"][
                    "directions_exercised"
                ]
            }
            family_base_ids = {
                f"union:{candidate.domain}:{candidate.name}"
                for candidate in self.b2_shared_common_keys
                if candidate.domain == key.domain
            }
            facts = {
                "schema_properties_exercised": (
                    expected_properties <= present
                ),
                "optional_present_exercised": (
                    expected_optional <= optional_present
                ),
                "optional_omitted_exercised": (
                    expected_optional <= optional_omitted
                ),
                "nullable_semantics_exercised": (
                    expected_nullable <= nullable_value
                    and expected_nullable <= nullable_null
                    and expected_nullable <= optional_omitted
                ),
                "reachable_union_alternatives_exercised": (
                    family_base_ids <= positive_fixture_ids
                ),
            }
            required_directions = set(
                B2_SHARED_COMMON_DIRECTIONS[key.domain]
            )
            direction_coverage = required_directions <= directions
            if not all(facts.values()) or not direction_coverage:
                raise FixtureError(
                    "B2 indexed fixture completeness is incomplete for "
                    f"{key.compact()}: facts={facts} "
                    f"directions={sorted(directions)}"
                )
            for record in records:
                record_facts = record["completeness_evidence"]
                record_facts.update(facts)
            indexed_coverage[key.compact()] = {
                "base_fixture_id": base_id,
                "property_schema_paths": sorted(expected_properties),
                "optional_property_schema_paths": sorted(
                    expected_optional
                ),
                "nullable_property_schema_paths": sorted(
                    expected_nullable
                ),
                "required_directions": sorted(required_directions),
                "directions_exercised": sorted(directions),
                "schema_direction_coverage": direction_coverage,
                "schema_fixture_facts": facts,
            }
        self.b2_indexed_coverage = dict(sorted(indexed_coverage.items()))

    def _build_operation_fixtures(self) -> None:
        for key, contract in sorted(self.contracts.items()):
            family = "client" if key.category == CLIENT_REQUEST else "server"
            operation_slug = slug(key.name)
            parameter_target = self.catalog.standalone(
                str(contract["parameter_type_identity"])
            )
            parameter_value = self.synthesizer.sample(parameter_target)
            parameter_role = (
                "client_request_params"
                if key.category == CLIENT_REQUEST
                else "server_request_params"
            )
            self.add_positive(
                f"operation:{key.category}:{key.name}:params",
                f"cases/operations/{family}/{operation_slug}/params.json",
                parameter_role,
                parameter_target,
                parameter_value,
                key,
            )

            # Unit is the canonical operation result contract, while the
            # generator still emits a named empty-object response schema.
            # Validate the authoritative schema root and retain Unit only as
            # the public semantic identity.
            result_schema_type = contract.get(
                "result_schema_type_identity",
                contract["result_type_identity"],
            )
            result_target = self.catalog.standalone(
                str(result_schema_type)
            )
            result_value = self.synthesizer.sample(result_target)
            result_role = (
                "client_request_result"
                if key.category == CLIENT_REQUEST
                else "server_request_response"
            )
            self.add_positive(
                f"operation:{key.category}:{key.name}:result",
                f"cases/operations/{family}/{operation_slug}/result.json",
                result_role,
                result_target,
                result_value,
                key,
            )

    def _build_baseline_fixtures(self) -> None:
        keys = sorted(
            SurfaceKey(category, domain, field, name)
            for category, domain, field, name in EXISTING_TYPED_FIXTURE_IDENTITIES
        )
        if len(keys) != 34 or len(set(keys)) != 34:
            raise FixtureError(
                "EXISTING_TYPED_FIXTURE_IDENTITIES must contain exactly "
                "34 unique reviewed identities"
            )
        for key in keys:
            if key.category == ITEM_DISCRIMINATOR:
                target = self.catalog.union_target(key.domain)
                index, branch, forced_scalar, forced_property = (
                    branch_for_union_identity(
                        self.catalog,
                        target,
                        key.discriminator_field,
                        key.name,
                    )
                )
                value = self.synthesizer.sample(
                    target,
                    branch,
                    pointer_child(
                        pointer_child(target.schema_path, "oneOf"), index
                    ),
                    forced_scalar,
                    forced_property,
                )
                self.add_positive(
                    f"baseline:{key.compact()}",
                    (
                        f"cases/baseline/{key.category}/"
                        f"{slug(key.domain)}-{slug(key.name)}.json"
                    ),
                    "existing_typed_identity",
                    target,
                    value,
                    key,
                    (index,),
                )
                continue

            target, index, branch = self.catalog.method_target(
                key.category, key.name
            )
            value = self.synthesizer.sample(
                target,
                branch,
                pointer_child(pointer_child("#", "oneOf"), index),
            )
            self.add_positive(
                f"baseline:{key.compact()}",
                (
                    f"cases/baseline/{key.category}/"
                    f"{slug(key.name)}.json"
                ),
                "existing_typed_identity",
                target,
                value,
                key,
                (index,),
            )

    def _build_union_fixtures(self) -> None:
        codex_error_target = self.catalog.union_target("CodexErrorInfo")
        validate_codex_error_rule_sets(
            self.manifest, codex_error_target.schema
        )
        known_union_values: dict[tuple[str, str], tuple[SchemaTarget, Any]] = {}
        b2_domains = tuple(
            sorted({key.domain for key in self.b2_shared_common_keys})
        )
        for domain in (
            "CodexErrorInfo",
            "ResponseItem",
            "ThreadItem",
            *b2_domains,
        ):
            target = self.catalog.union_target(domain)
            if domain in b2_domains:
                identities = sorted(
                    (
                        key
                        for key in self.b2_shared_common_keys
                        if key.domain == domain
                    ),
                    key=lambda key: key.name,
                )
            else:
                identities = sorted(
                    (
                        SurfaceKey.from_manifest(entry)
                        for entry in self.manifest["entries"]
                        if entry["stability"] == "stable"
                        and entry["domain"] == domain
                        and entry["category"]
                        in {
                            ITEM_DISCRIMINATOR,
                            TAGGED_UNION_DISCRIMINATOR,
                        }
                    ),
                    key=lambda key: key.name,
                )
            for key in identities:
                index, branch, forced_scalar, forced_property = (
                    branch_for_union_identity(
                        self.catalog,
                        target,
                        key.discriminator_field,
                        key.name,
                    )
                )
                keyword = (
                    "oneOf"
                    if isinstance(target.schema, dict)
                    and "oneOf" in target.schema
                    else "anyOf"
                )
                branch_path = pointer_child(
                    pointer_child(target.schema_path, keyword), index
                )
                value = self.synthesizer.sample(
                    target,
                    branch,
                    branch_path,
                    forced_scalar,
                    forced_property,
                )
                known_union_values[(domain, key.name)] = (
                    target,
                    copy.deepcopy(value),
                )
                intended = (index,) if keyword == "oneOf" else ()
                self.add_positive(
                    f"union:{domain}:{key.name}",
                    (
                        f"cases/unions/{slug(domain)}/"
                        f"{slug(key.name)}.json"
                    ),
                    "union_branch",
                    target,
                    value,
                    key,
                    intended,
                    directions_exercised=B2_SHARED_COMMON_DIRECTIONS.get(
                        domain, ()
                    ),
                )

        self._build_b2_union_supplements(known_union_values)

        target = codex_error_target
        for name in CODEX_ERROR_INFO_HTTP_IDENTITIES:
            key = SurfaceKey(
                TAGGED_UNION_DISCRIMINATOR,
                "CodexErrorInfo",
                "$variant",
                name,
            )
            index, _, _, _ = branch_for_union_identity(
                self.catalog, target, "$variant", name
            )
            for case, nested in (
                ("http-status-null", {"httpStatusCode": None}),
                ("http-status-omitted", {}),
            ):
                self.add_positive(
                    f"union:CodexErrorInfo:{name}:{case}",
                    (
                        "cases/unions/codexerrorinfo/"
                        f"{slug(name)}-{case}.json"
                    ),
                    "union_branch_supplement",
                    target,
                    {name: nested},
                    key,
                    (index,),
                )

        active_key = SurfaceKey(
            TAGGED_UNION_DISCRIMINATOR,
            "CodexErrorInfo",
            "$variant",
            "activeTurnNotSteerable",
        )
        active_index, _, _, _ = branch_for_union_identity(
            self.catalog, target, "$variant", "activeTurnNotSteerable"
        )
        self.add_positive(
            "union:CodexErrorInfo:activeTurnNotSteerable:turn-kind-compact",
            "cases/unions/codexerrorinfo/activeturnnotsteerable-turn-kind-compact.json",
            "union_branch_supplement",
            target,
            {"activeTurnNotSteerable": {"turnKind": "compact"}},
            active_key,
            (active_index,),
        )

        self.add_negative(
            "union:CodexErrorInfo:future-unknown",
            "cases/unions/codexerrorinfo/future-unknown.json",
            "unknown_discriminator",
            target,
            "futureCodexErrorInfo",
            ["one_of_zero"],
        )
        self.add_negative(
            "union:CodexErrorInfo:activeTurnNotSteerable:future-turn-kind",
            "cases/unions/codexerrorinfo/activeturnnotsteerable-future-turn-kind.json",
            "unknown_enum_value",
            target,
            {"activeTurnNotSteerable": {"turnKind": "futureTurnKind"}},
            ["one_of_zero"],
        )

        for domain, known_name, future_name in (
            ("ThreadItem", "agentMessage", "futureThreadItem"),
            ("ResponseItem", "agent_message", "future_response_item"),
        ):
            union_target, known_value = known_union_values[(domain, known_name)]
            if not isinstance(known_value, dict) or "type" not in known_value:
                raise FixtureError(
                    f"{domain}:{known_name} lacks the reviewed type discriminator"
                )
            known_value["type"] = future_name
            self.add_negative(
                f"union:{domain}:future-unknown",
                (
                    f"cases/unions/{slug(domain)}/"
                    "future-unknown.json"
                ),
                "unknown_discriminator",
                union_target,
                known_value,
                ["one_of_zero"],
            )

        for category, type_identity, future_method in (
            (CLIENT_REQUEST, "ClientRequest", "future/clientRequest"),
            (
                CLIENT_NOTIFICATION,
                "ClientNotification",
                "future/clientNotification",
            ),
            (SERVER_REQUEST, "ServerRequest", "future/serverRequest"),
            (
                SERVER_NOTIFICATION,
                "ServerNotification",
                "future/serverNotification",
            ),
        ):
            method_target = self.catalog.standalone(type_identity)
            known_value = self.synthesizer.sample(method_target)
            if not isinstance(known_value, dict) or "method" not in known_value:
                raise FixtureError(
                    f"{type_identity} sample lacks the reviewed method discriminator"
                )
            known_value["method"] = future_method
            self.add_negative(
                f"method:{category}:future-unknown",
                f"cases/mutations/{slug(type_identity)}/future-method.json",
                "unknown_method",
                method_target,
                known_value,
                ["one_of_zero"],
            )

        thread_target, malformed_thread = known_union_values[
            ("ThreadItem", "agentMessage")
        ]
        if not isinstance(malformed_thread, dict) or "id" not in malformed_thread:
            raise FixtureError("ThreadItem:agentMessage sample lacks required id")
        malformed_thread["id"] = 0
        self.add_negative(
            "union:ThreadItem:nested-wrong-type",
            "cases/unions/threaditem/nested-wrong-type.json",
            "nested_union_failure",
            thread_target,
            malformed_thread,
            ["one_of_zero"],
        )

        response_target, malformed_response = known_union_values[
            ("ResponseItem", "agent_message")
        ]
        if (
            not isinstance(malformed_response, dict)
            or "content" not in malformed_response
        ):
            raise FixtureError(
                "ResponseItem:agent_message sample lacks required content"
            )
        malformed_response["content"] = 0
        self.add_negative(
            "union:ResponseItem:nested-wrong-type",
            "cases/unions/responseitem/nested-wrong-type.json",
            "nested_union_failure",
            response_target,
            malformed_response,
            ["one_of_zero"],
        )
        self.add_negative(
            "union:CodexErrorInfo:malformed-known",
            "cases/unions/codexerrorinfo/malformed-known.json",
            "malformed_known",
            target,
            {"activeTurnNotSteerable": {}},
            ["one_of_zero"],
        )
        self.add_negative(
            "union:CodexErrorInfo:nested-wrong-type",
            "cases/unions/codexerrorinfo/nested-wrong-type.json",
            "nested_union_failure",
            target,
            {"activeTurnNotSteerable": {"turnKind": 0}},
            ["one_of_zero"],
        )

    def _build_b2_union_supplements(
        self,
        known_union_values: Mapping[
            tuple[str, str], tuple[SchemaTarget, Any]
        ],
    ) -> None:
        alternative_coverage: dict[str, Any] = {}
        family_coverage: dict[str, Any] = {}

        for key in self.b2_shared_common_keys:
            target, base_value = known_union_values[(key.domain, key.name)]
            index, _, _, _ = branch_for_union_identity(
                self.catalog,
                target,
                key.discriminator_field,
                key.name,
            )
            intended = (index,) if "oneOf" in target.schema else ()
            directions = B2_SHARED_COMMON_DIRECTIONS[key.domain]
            optional_locations = collect_optional_present_locations(
                self.catalog, target, base_value
            )
            validator = self.catalog.target_validator(target)
            optional_fixture_ids: list[str] = []
            nullable_null_fixture_ids: list[str] = []
            nullable_paths: list[str] = []

            for location in optional_locations:
                path_slug = slug(json_path(location.instance_path))
                omitted = copy.deepcopy(base_value)
                parent, field = get_parent_path(
                    omitted, location.instance_path
                )
                if not isinstance(parent, dict) or not isinstance(field, str):
                    raise FixtureError(
                        "B2 optional property is not an object field: "
                        f"{key.compact()}:{json_path(location.instance_path)}"
                    )
                parent.pop(field)
                if validator.validate_subschema(
                    omitted, target.schema, target.schema_path
                ):
                    raise FixtureError(
                        "B2 optional omission was rejected by the full schema: "
                        f"{key.compact()}:{json_path(location.instance_path)}"
                    )
                omitted_id = (
                    f"union:{key.domain}:{key.name}:"
                    f"optional-omitted:{path_slug}"
                )
                self.add_positive(
                    omitted_id,
                    (
                        f"cases/unions/{slug(key.domain)}/supplements/"
                        f"{slug(key.name)}-optional-omitted-{path_slug}.json"
                    ),
                    "union_optional_omitted",
                    target,
                    omitted,
                    key,
                    intended,
                    omitted_schema_paths=(location.schema_path,),
                    directions_exercised=directions,
                )
                optional_fixture_ids.append(omitted_id)

                nullable = not validator.validate_subschema(
                    None, location.schema, location.schema_path
                )
                if not nullable:
                    continue
                if (
                    get_instance_path(base_value, location.instance_path)
                    is None
                ):
                    raise FixtureError(
                        "B2 base fixture must exercise a non-null nullable "
                        f"value: {key.compact()}:{json_path(location.instance_path)}"
                    )
                nullable_paths.append(location.schema_path)
                null_value = copy.deepcopy(base_value)
                parent, field = get_parent_path(
                    null_value, location.instance_path
                )
                parent[field] = None
                null_id = (
                    f"union:{key.domain}:{key.name}:"
                    f"nullable-null:{path_slug}"
                )
                self.add_positive(
                    null_id,
                    (
                        f"cases/unions/{slug(key.domain)}/supplements/"
                        f"{slug(key.name)}-nullable-null-{path_slug}.json"
                    ),
                    "union_nullable_null",
                    target,
                    null_value,
                    key,
                    intended,
                    directions_exercised=directions,
                )
                nullable_null_fixture_ids.append(null_id)

            required_locations = collect_required_locations(
                self.catalog, target, base_value
            )
            discriminator_locations = [
                location
                for location in required_locations
                if (
                    (
                        key.discriminator_field != "$variant"
                        and location.instance_path
                        == (key.discriminator_field,)
                    )
                    or (
                        key.discriminator_field == "$variant"
                        and location.instance_path == (key.name,)
                    )
                )
            ]
            if len(discriminator_locations) > 1:
                raise FixtureError(
                    "B2 alternative has multiple discriminator locations: "
                    f"{key.compact()}"
                )
            missing_required_locations = [
                location
                for location in required_locations
                if not (
                    (
                        key.discriminator_field != "$variant"
                        and location.instance_path
                        == (key.discriminator_field,)
                    )
                    or (
                        key.discriminator_field == "$variant"
                        and location.instance_path == (key.name,)
                    )
                )
            ]
            wrong_type_locations = {
                location.schema_path: location
                for location in (
                    *missing_required_locations,
                    *optional_locations,
                )
            }
            missing_fixture_ids: list[str] = []
            wrong_type_fixture_ids: list[str] = []
            missing_discriminator_fixture_ids: list[str] = []
            wrong_discriminator_fixture_ids: list[str] = []

            for location in discriminator_locations:
                missing_discriminator = copy.deepcopy(base_value)
                parent, field = get_parent_path(
                    missing_discriminator, location.instance_path
                )
                if not isinstance(parent, dict) or not isinstance(field, str):
                    raise FixtureError(
                        "B2 discriminator is not an object field: "
                        f"{key.compact()}"
                    )
                parent.pop(field)
                missing_discriminator_id = (
                    f"union:{key.domain}:{key.name}:"
                    "missing-discriminator"
                )
                self.add_negative(
                    missing_discriminator_id,
                    (
                        f"cases/unions/{slug(key.domain)}/mutations/"
                        f"{slug(key.name)}-missing-discriminator.json"
                    ),
                    "malformed_known_missing_discriminator",
                    target,
                    missing_discriminator,
                    ["one_of_zero"],
                    key,
                    "missing_discriminator",
                )
                missing_discriminator_fixture_ids.append(
                    missing_discriminator_id
                )

                wrong_discriminator = copy.deepcopy(base_value)
                parent, field = get_parent_path(
                    wrong_discriminator, location.instance_path
                )
                parent[field] = None
                wrong_discriminator_id = (
                    f"union:{key.domain}:{key.name}:"
                    "wrong-discriminator-type"
                )
                self.add_negative(
                    wrong_discriminator_id,
                    (
                        f"cases/unions/{slug(key.domain)}/mutations/"
                        f"{slug(key.name)}-wrong-discriminator-type.json"
                    ),
                    "malformed_known_wrong_discriminator_type",
                    target,
                    wrong_discriminator,
                    ["one_of_zero"],
                    key,
                    "wrong_discriminator_type",
                )
                wrong_discriminator_fixture_ids.append(
                    wrong_discriminator_id
                )

            for location in missing_required_locations:
                path_slug = slug(json_path(location.instance_path))
                missing = copy.deepcopy(base_value)
                parent, field = get_parent_path(
                    missing, location.instance_path
                )
                if not isinstance(parent, dict) or not isinstance(field, str):
                    raise FixtureError(
                        "B2 required property is not an object field: "
                        f"{key.compact()}:{json_path(location.instance_path)}"
                    )
                parent.pop(field)
                missing_id = (
                    f"union:{key.domain}:{key.name}:"
                    f"missing-required:{path_slug}"
                )
                self.add_negative(
                    missing_id,
                    (
                        f"cases/unions/{slug(key.domain)}/mutations/"
                        f"{slug(key.name)}-missing-required-{path_slug}.json"
                    ),
                    "malformed_known_missing_required",
                    target,
                    missing,
                    ["one_of_zero"],
                    key,
                    "missing_required",
                )
                missing_fixture_ids.append(missing_id)

            for location in (
                wrong_type_locations[path]
                for path in sorted(wrong_type_locations)
            ):
                path_slug = slug(json_path(location.instance_path))
                original = get_instance_path(
                    base_value, location.instance_path
                )
                wrong_value: Any = NO_WRONG_TYPE_MUTATION
                for candidate in WRONG_TYPE_CANDIDATES:
                    if canonical_json(candidate) == canonical_json(original):
                        continue
                    mutated = copy.deepcopy(base_value)
                    parent, field = get_parent_path(
                        mutated, location.instance_path
                    )
                    parent[field] = copy.deepcopy(candidate)
                    if validator.validate_subschema(
                        mutated, target.schema, target.schema_path
                    ):
                        wrong_value = mutated
                        break
                if wrong_value is NO_WRONG_TYPE_MUTATION:
                    raise FixtureError(
                        "B2 property has no rejecting wrong-type "
                        f"mutation: {key.compact()}:{json_path(location.instance_path)}"
                    )
                wrong_id = (
                    f"union:{key.domain}:{key.name}:"
                    f"wrong-nested-type:{path_slug}"
                )
                self.add_negative(
                    wrong_id,
                    (
                        f"cases/unions/{slug(key.domain)}/mutations/"
                        f"{slug(key.name)}-wrong-nested-type-{path_slug}.json"
                    ),
                    "malformed_known_wrong_type",
                    target,
                    wrong_value,
                    ["one_of_zero"],
                    key,
                    "wrong_nested_type",
                )
                wrong_type_fixture_ids.append(wrong_id)

            alternative_coverage[key.compact()] = {
                "base_fixture_id": f"union:{key.domain}:{key.name}",
                "optional_omitted_fixture_ids": sorted(
                    optional_fixture_ids
                ),
                "nullable_null_fixture_ids": sorted(
                    nullable_null_fixture_ids
                ),
                "nullable_schema_paths": sorted(nullable_paths),
                "missing_required_fixture_ids": sorted(
                    missing_fixture_ids
                ),
                "missing_discriminator_fixture_ids": sorted(
                    missing_discriminator_fixture_ids
                ),
                "wrong_nested_type_fixture_ids": sorted(
                    wrong_type_fixture_ids
                ),
                "wrong_discriminator_type_fixture_ids": sorted(
                    wrong_discriminator_fixture_ids
                ),
                "missing_discriminator_exclusion": (
                    None
                    if discriminator_locations
                    else (
                        "scalar externally selected alternative has no "
                        "discriminator property"
                    )
                ),
                "missing_required_exclusion": (
                    None
                    if missing_required_locations
                    else (
                        "alternative has no non-discriminator required "
                        "payload field"
                    )
                ),
                "malformed_known_exclusion": (
                    None
                    if wrong_type_locations
                    else (
                        "alternative has no non-discriminator payload field"
                    )
                ),
                "conflicting_discriminator_exclusion": (
                    "JSON value shape provides exactly one scalar or object "
                    "discriminator slot"
                ),
            }

        for domain in sorted(B2_SHARED_COMMON_FAMILY_COUNTS):
            keys = [
                key
                for key in self.b2_shared_common_keys
                if key.domain == domain
            ]
            representative = keys[0]
            target, future_value = known_union_values[
                (domain, representative.name)
            ]
            future_name = f"future{domain}"
            if representative.discriminator_field == "$variant":
                future_value = future_name
            else:
                future_value = copy.deepcopy(future_value)
                if not isinstance(future_value, dict):
                    raise FixtureError(
                        f"{domain} sample lacks its object discriminator"
                    )
                future_value[representative.discriminator_field] = future_name
            future_id = f"union:{domain}:future-unknown"
            self.add_negative(
                future_id,
                f"cases/unions/{slug(domain)}/future-unknown.json",
                "unknown_discriminator",
                target,
                future_value,
                ["one_of_zero"],
                negative_case="future_discriminator",
            )
            family_coverage[domain] = {
                "future_discriminator_fixture_id": future_id,
                "known_alternative_count": len(keys),
                "conflicting_discriminator_fixture_ids": [],
                "conflicting_discriminator_exclusion": (
                    "the pinned JSON representation has exactly one "
                    "discriminator slot, so multiple discriminator values "
                    "are not structurally representable"
                ),
            }

        self.b2_negative_coverage = {
            "assignment_derived_key_count": len(
                self.b2_shared_common_keys
            ),
            "family_counts": dict(
                sorted(B2_SHARED_COMMON_FAMILY_COUNTS.items())
            ),
            "alternatives": dict(sorted(alternative_coverage.items())),
            "families": dict(sorted(family_coverage.items())),
        }

    def _build_b2_open_enum_fixtures(self) -> None:
        enum_coverage: dict[str, Any] = {}
        for domain, expected_values in sorted(B2_OPEN_STRING_ENUMS.items()):
            target = self.catalog.union_target(domain)
            resolved, _ = self.catalog.resolve(
                target, target.schema, target.schema_path
            )
            actual_values = (
                tuple(resolved.get("enum", ()))
                if isinstance(resolved, dict)
                else ()
            )
            if actual_values != expected_values:
                raise FixtureError(
                    f"B2 open-enum pin mismatch for {domain}: "
                    f"{actual_values!r}"
                )
            known_ids: list[str] = []
            for value in expected_values:
                fixture_id = f"enum:{domain}:{value}"
                self.add_positive(
                    fixture_id,
                    f"cases/enums/{slug(domain)}/{slug(value)}.json",
                    "open_enum_known_value",
                    target,
                    value,
                    None,
                    directions_exercised=("Decode", "Encode"),
                )
                known_ids.append(fixture_id)
            future_id = f"enum:{domain}:future-unknown"
            self.add_negative(
                future_id,
                f"cases/enums/{slug(domain)}/future-unknown.json",
                "unknown_enum_value",
                target,
                f"future{domain}",
                ["enum_mismatch"],
                negative_case="future_open_enum_value",
            )
            enum_coverage[domain] = {
                "known_value_fixture_ids": known_ids,
                "future_value_fixture_id": future_id,
                "future_value_schema_diagnostic_codes": [
                    "enum_mismatch"
                ],
            }
        self.b2_negative_coverage["open_string_enums"] = dict(
            sorted(enum_coverage.items())
        )


def generated_outputs(
    arguments: argparse.Namespace,
) -> tuple[dict[Path, bytes], dict[str, Any]]:
    builder = CorpusBuilder(
        arguments.repo_root,
        arguments.schema_root,
        arguments.manifest,
        arguments.contracts,
        arguments.validator,
    )
    (
        fixture_files,
        index,
        fixture_coverage,
        keywords,
        reachability,
        assignments,
    ) = builder.build()
    completeness_records = [
        {
            "protocol_surface_key": record["protocol_surface_key"],
            "fixture_ids": record["fixture_ids"],
            "positive_fixture_count": record["positive_fixture_count"],
            "schema_fixture_facts": record["completeness_evidence"],
        }
        for record in fixture_coverage["identity_coverage"]
    ]
    completeness_facts = (
        completeness_records[0]["schema_fixture_facts"]
        if completeness_records
        else {}
    )
    completeness_report = {
        "generated_notice": "Generated by tools/codex/app_server_fixtures.py; do not edit.",
        "format_version": FORMAT_VERSION,
        "codex_version": CODEX_VERSION,
        "authority_note": (
            "This report contains schema- and fixture-derived facts only. "
            "The production registry combines them with implementation-owned "
            "evidence and mechanically derives TypedSchemaStatus; this report "
            "does not assert or improve a status."
        ),
        "counts": {
            "surface_identities": len(completeness_records),
            "identities_with_positive_fixtures": sum(
                record["positive_fixture_count"] > 0
                for record in completeness_records
            ),
            "facts_true_by_field": {
                field: sum(
                    record["schema_fixture_facts"][field]
                    for record in completeness_records
                )
                for field in sorted(completeness_facts)
            },
        },
        "records": completeness_records,
    }
    outputs: dict[Path, bytes] = {
        arguments.fixture_root / relative: payload
        for relative, payload in fixture_files.items()
    }
    outputs.update(
        {
            arguments.evidence_root / "fixture-coverage.json": encoded_json(
                fixture_coverage
            ),
            arguments.evidence_root / "schema-keywords.json": encoded_json(
                keywords
            ),
            arguments.evidence_root / "nested-reachability.json": encoded_json(
                reachability
            ),
            arguments.evidence_root
            / "module-slice-assignment.json": encoded_json(assignments),
            arguments.evidence_root
            / "schema-completeness-evidence.json": encoded_json(
                completeness_report
            ),
        }
    )
    return outputs, index


def write_outputs(outputs: Mapping[Path, bytes], managed_roots: Sequence[Path]) -> None:
    expected = {path.resolve() for path in outputs}
    for root in managed_roots:
        if root.exists():
            for path in sorted(root.rglob("*.json")):
                if path.resolve() not in expected:
                    path.unlink()
    for path, payload in sorted(outputs.items(), key=lambda item: str(item[0])):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(payload)


def check_outputs(outputs: Mapping[Path, bytes], managed_roots: Sequence[Path]) -> None:
    expected = {path.resolve() for path in outputs}
    managed_actual: set[Path] = set()
    for root in managed_roots:
        if root.exists():
            managed_actual.update(
                path.resolve() for path in root.rglob("*.json")
            )
    missing = sorted(
        str(path) for path in expected if not path.is_file()
    )
    extra = sorted(str(path) for path in managed_actual - expected)
    changed = sorted(
        str(path)
        for path, payload in outputs.items()
        if path.is_file() and path.read_bytes() != payload
    )
    if missing or extra or changed:
        raise FixtureError(
            f"generated fixture evidence is stale; "
            f"missing={missing}, extra={extra}, changed={changed}"
        )


def validate_committed(arguments: argparse.Namespace) -> None:
    index_path = arguments.fixture_root / "index.json"
    index = load_json(index_path)
    if index.get("format_version") != FORMAT_VERSION:
        raise FixtureError("unsupported committed fixture index format")
    expected_sources = CorpusBuilder(
        arguments.repo_root,
        arguments.schema_root,
        arguments.manifest,
        arguments.contracts,
        arguments.validator,
    )._source_hashes()
    if index.get("sources") != expected_sources:
        raise FixtureError("fixture index source/provenance hashes are stale")
    draft07 = load_draft07(arguments.validator)
    catalog = SchemaCatalog(arguments.schema_root, draft07)
    manifest = load_json(arguments.manifest)
    manifest_by_key = {
        SurfaceKey.from_manifest(entry): entry
        for entry in manifest["entries"]
    }
    seen_files: set[str] = set()
    seen_ids: set[str] = set()
    for record in index["fixtures"]:
        fixture_id = str(record["id"])
        if fixture_id in seen_ids:
            raise FixtureError(f"duplicate fixture identity in index: {fixture_id}")
        seen_ids.add(fixture_id)
        key_record = record.get("protocol_surface_key")
        if key_record is not None:
            surface_key = SurfaceKey.from_contract(key_record)
            entry = manifest_by_key.get(surface_key)
            if entry is None:
                raise FixtureError(
                    "fixture references unknown protocol surface identity: "
                    f"{surface_key.compact()}"
                )
            if entry["stability"] != "stable":
                raise FixtureError(
                    "fixture incorrectly counts an experimental-only identity: "
                    f"{surface_key.compact()}"
                )
        relative = str(record["file"])
        if relative in seen_files:
            raise FixtureError(f"duplicate fixture file in index: {relative}")
        seen_files.add(relative)
        path = (arguments.fixture_root / relative).resolve()
        try:
            path.relative_to(arguments.fixture_root.resolve())
        except ValueError as error:
            raise FixtureError(
                f"fixture path escapes the corpus root: {relative}"
            ) from error
        payload = path.read_bytes()
        if sha256_bytes(payload) != record["file_sha256"]:
            raise FixtureError(f"fixture hash mismatch: {relative}")
        value = json.loads(payload.decode("utf-8"))
        target = schema_target_from_record(catalog, record["schema"])
        diagnostics = catalog.target_validator(target).validate_subschema(
            value, target.schema, target.schema_path
        )
        expected_valid = record.get("expected_valid", True)
        if expected_valid and diagnostics:
            raise FixtureError(
                f"committed positive fixture is invalid: {relative}: "
                f"{[item.to_json() for item in diagnostics[:5]]}"
            )
        if not expected_valid:
            actual_codes = sorted({item.code for item in diagnostics})
            if actual_codes != sorted(record["expected_diagnostic_codes"]):
                raise FixtureError(
                    f"committed negative fixture diagnostics changed: {relative}"
                )
        else:
            if record.get("fixture_sha256") != record["file_sha256"]:
                raise FixtureError(
                    f"positive fixture evidence hash mismatch: {relative}"
                )
            validation = record.get("validation")
            if not isinstance(validation, dict) or validation.get(
                "independent"
            ) is not True:
                raise FixtureError(
                    f"positive fixture lacks independent validation: {relative}"
                )
            intended_branches = tuple(
                validation.get("one_of_branch_indices", [])
            )
            if intended_branches:
                actual_branches = matching_one_of_branches(
                    catalog,
                    target,
                    value,
                    target.schema,
                    target.schema_path,
                )
                if actual_branches != intended_branches:
                    raise FixtureError(
                        f"fixture {relative} matches oneOf branches "
                        f"{actual_branches}, expected {intended_branches}"
                    )
            recomputed = mutation_evidence(catalog, target, value)
            committed_mutation = {
                key: value
                for key, value in validation.items()
                if key not in {"independent", "one_of_branch_indices"}
            }
            if recomputed != committed_mutation:
                raise FixtureError(
                    f"committed mutation evidence is stale: {relative}"
                )
            committed_schema_coverage = record.get(
                "schema_fixture_coverage"
            )
            if committed_schema_coverage is not None:
                if not isinstance(committed_schema_coverage, dict):
                    raise FixtureError(
                        f"positive fixture has invalid schema coverage: {relative}"
                    )
                recomputed_schema_coverage = schema_fixture_coverage(
                    catalog,
                    target,
                    value,
                    omitted_schema_paths=committed_schema_coverage.get(
                        "optional_property_schema_paths_omitted", []
                    ),
                    directions_exercised=committed_schema_coverage.get(
                        "directions_exercised", []
                    ),
                )
                if recomputed_schema_coverage != committed_schema_coverage:
                    raise FixtureError(
                        "committed schema fixture coverage is stale: "
                        f"{relative}"
                    )
    actual_files = {
        path.relative_to(arguments.fixture_root).as_posix()
        for path in arguments.fixture_root.rglob("*.json")
    }
    expected_files = seen_files | {"index.json"}
    if actual_files != expected_files:
        raise FixtureError(
            f"fixture corpus file set mismatch; "
            f"missing={sorted(expected_files-actual_files)}, "
            f"extra={sorted(actual_files-expected_files)}"
        )


def _parser() -> argparse.ArgumentParser:
    repo_root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "mode", choices=("generate", "check", "validate")
    )
    parser.add_argument("--repo-root", type=Path, default=repo_root)
    parser.add_argument(
        "--schema-root",
        type=Path,
        default=repo_root / "tools/codex/app-server-schema/0.144.6",
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        default=repo_root / "tools/codex/app-server-surface/0.144.6.json",
    )
    parser.add_argument(
        "--contracts",
        type=Path,
        default=repo_root
        / "tools/codex/app-server-evidence/0.144.6/operation-contracts.json",
    )
    parser.add_argument(
        "--validator",
        type=Path,
        default=repo_root / "tools/codex/draft07.py",
    )
    parser.add_argument(
        "--fixture-root",
        type=Path,
        default=repo_root / "tools/codex/app-server-fixtures/0.144.6",
    )
    parser.add_argument(
        "--evidence-root",
        type=Path,
        default=repo_root / "tools/codex/app-server-evidence/0.144.6",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    arguments = _parser().parse_args(argv)
    arguments.repo_root = arguments.repo_root.resolve()
    arguments.schema_root = arguments.schema_root.resolve()
    arguments.manifest = arguments.manifest.resolve()
    arguments.contracts = arguments.contracts.resolve()
    arguments.validator = arguments.validator.resolve()
    arguments.fixture_root = arguments.fixture_root.resolve()
    arguments.evidence_root = arguments.evidence_root.resolve()
    try:
        if arguments.mode == "validate":
            validate_committed(arguments)
            return 0
        outputs, _ = generated_outputs(arguments)
        managed_roots = (arguments.fixture_root,)
        if arguments.mode == "generate":
            write_outputs(outputs, managed_roots)
        else:
            check_outputs(outputs, managed_roots)
        return 0
    except (
        FixtureError,
        OSError,
        UnicodeError,
        json.JSONDecodeError,
        KeyError,
        TypeError,
        ValueError,
    ) as error:
        print(f"app-server-fixtures: error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
