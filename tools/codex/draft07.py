#!/usr/bin/env python3
"""Small, deterministic, offline JSON Schema Draft-07 validation subset.

This module deliberately implements only the schema vocabulary reviewed for
the pinned Codex App Server inputs.  Unsupported schema-position keywords are
errors; they are never treated as annotations.  The implementation has no
network access and resolves only document-local ``#`` references.
"""

from __future__ import annotations

import argparse
import json
import math
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Iterator, Mapping, Sequence


FORMAT_VERSION = 1

VALIDATING_KEYWORDS = frozenset(
    {
        "$ref",
        "additionalProperties",
        "allOf",
        "anyOf",
        "const",
        "enum",
        "items",
        "maximum",
        "maxItems",
        "maxLength",
        "minimum",
        "minItems",
        "minLength",
        "not",
        "oneOf",
        "pattern",
        "properties",
        "required",
        "type",
    }
)

STRUCTURAL_KEYWORDS = frozenset({"definitions"})

# Draft-07 annotations plus the schemars extensions present in the pin.
ANNOTATION_KEYWORDS = frozenset(
    {
        "$comment",
        "$schema",
        "default",
        "deprecated",
        "description",
        "enumNames",
        "examples",
        "format",
        "readOnly",
        "title",
        "writeOnly",
    }
)

SUPPORTED_SCHEMA_KEYWORDS = (
    VALIDATING_KEYWORDS | STRUCTURAL_KEYWORDS | ANNOTATION_KEYWORDS
)


def _pointer_token(value: str) -> str:
    return value.replace("~", "~0").replace("/", "~1")


def child_path(path: str, token: str | int) -> str:
    return f"{path}/{_pointer_token(str(token))}"


def canonical_json(value: Any) -> str:
    return json.dumps(
        value,
        ensure_ascii=False,
        allow_nan=False,
        sort_keys=True,
        separators=(",", ":"),
    )


@dataclass(frozen=True, order=True)
class Diagnostic:
    """One deterministic instance-validation failure."""

    code: str
    instance_path: str
    schema_path: str
    message: str
    branch_indices: tuple[int, ...] | None = None

    def to_json(self) -> dict[str, Any]:
        result: dict[str, Any] = {
            "code": self.code,
            "instance_path": self.instance_path,
            "schema_path": self.schema_path,
            "message": self.message,
        }
        if self.branch_indices is not None:
            result["branch_indices"] = list(self.branch_indices)
        return result


class SchemaError(ValueError):
    """A malformed, unsupported, or unresolved schema."""

    def __init__(self, code: str, schema_path: str, message: str):
        super().__init__(f"{code} at {schema_path}: {message}")
        self.code = code
        self.schema_path = schema_path
        self.message = message

    def to_json(self) -> dict[str, str]:
        return {
            "code": self.code,
            "schema_path": self.schema_path,
            "message": self.message,
        }


def _schema_error(
    condition: bool, code: str, schema_path: str, message: str
) -> None:
    if not condition:
        raise SchemaError(code, schema_path, message)


def _json_equal(left: Any, right: Any) -> bool:
    left_number = isinstance(left, (int, float)) and not isinstance(left, bool)
    right_number = isinstance(right, (int, float)) and not isinstance(right, bool)
    if left_number or right_number:
        return (
            left_number
            and right_number
            and (not isinstance(left, float) or math.isfinite(left))
            and (not isinstance(right, float) or math.isfinite(right))
            and left == right
        )
    if type(left) is not type(right):
        return False
    if left is None or isinstance(left, (bool, str)):
        return left == right
    if isinstance(left, list):
        return len(left) == len(right) and all(
            _json_equal(left_item, right_item)
            for left_item, right_item in zip(left, right)
        )
    if isinstance(left, dict):
        return left.keys() == right.keys() and all(
            _json_equal(left[key], right[key]) for key in left
        )
    return False


def _is_number(value: Any) -> bool:
    return (
        isinstance(value, (int, float))
        and not isinstance(value, bool)
        and (not isinstance(value, float) or math.isfinite(value))
    )


def _matches_type(instance: Any, expected: str) -> bool:
    if expected == "null":
        return instance is None
    if expected == "boolean":
        return isinstance(instance, bool)
    if expected == "integer":
        return (
            isinstance(instance, int)
            and not isinstance(instance, bool)
        ) or (
            isinstance(instance, float)
            and math.isfinite(instance)
            and instance.is_integer()
        )
    if expected == "number":
        return _is_number(instance)
    if expected == "string":
        return isinstance(instance, str)
    if expected == "array":
        return isinstance(instance, list)
    if expected == "object":
        return isinstance(instance, dict)
    raise SchemaError(
        "unsupported_type_name",
        "#/type",
        f"unsupported JSON Schema type {expected!r}",
    )


class Validator:
    """Validate JSON-compatible Python values against one schema document."""

    def __init__(self, document: Any):
        self.document = document
        self.check_schema()

    def check_schema(self) -> None:
        self._check_schema(self.document, "#")

    def _check_schema(self, schema: Any, schema_path: str) -> None:
        if isinstance(schema, bool):
            return
        _schema_error(
            isinstance(schema, dict),
            "invalid_schema",
            schema_path,
            "schema must be an object or boolean",
        )

        unsupported = sorted(set(schema) - SUPPORTED_SCHEMA_KEYWORDS)
        if unsupported:
            raise SchemaError(
                "unsupported_keyword",
                schema_path,
                f"unsupported schema-position keywords: {unsupported}",
            )

        if "$ref" in schema:
            reference = schema["$ref"]
            _schema_error(
                isinstance(reference, str),
                "invalid_reference",
                child_path(schema_path, "$ref"),
                "$ref must be a string",
            )
            self.resolve_reference(reference, child_path(schema_path, "$ref"))

        if "type" in schema:
            value = schema["type"]
            values = [value] if isinstance(value, str) else value
            _schema_error(
                isinstance(values, list)
                and bool(values)
                and all(isinstance(item, str) for item in values),
                "invalid_schema",
                child_path(schema_path, "type"),
                "type must be a non-empty string or string array",
            )
            known_types = {
                "array",
                "boolean",
                "integer",
                "null",
                "number",
                "object",
                "string",
            }
            unknown = sorted(set(values) - known_types)
            _schema_error(
                not unknown,
                "unsupported_type_name",
                child_path(schema_path, "type"),
                f"unsupported JSON Schema types: {unknown}",
            )

        if "required" in schema:
            required = schema["required"]
            _schema_error(
                isinstance(required, list)
                and all(isinstance(name, str) for name in required)
                and len(required) == len(set(required)),
                "invalid_schema",
                child_path(schema_path, "required"),
                "required must be a unique string array",
            )

        for keyword in ("definitions", "properties"):
            children = schema.get(keyword)
            if children is None:
                continue
            _schema_error(
                isinstance(children, dict),
                "invalid_schema",
                child_path(schema_path, keyword),
                f"{keyword} must be an object",
            )
            for name, child in sorted(children.items()):
                self._check_schema(
                    child, child_path(child_path(schema_path, keyword), name)
                )

        if "additionalProperties" in schema:
            additional = schema["additionalProperties"]
            _schema_error(
                isinstance(additional, (bool, dict)),
                "invalid_schema",
                child_path(schema_path, "additionalProperties"),
                "additionalProperties must be a schema",
            )
            self._check_schema(
                additional, child_path(schema_path, "additionalProperties")
            )

        if "items" in schema:
            items = schema["items"]
            _schema_error(
                isinstance(items, (bool, dict, list)),
                "invalid_schema",
                child_path(schema_path, "items"),
                "items must be a schema or schema array",
            )
            if isinstance(items, list):
                for index, child in enumerate(items):
                    self._check_schema(
                        child, child_path(child_path(schema_path, "items"), index)
                    )
            else:
                self._check_schema(items, child_path(schema_path, "items"))

        for keyword in ("allOf", "anyOf", "oneOf"):
            children = schema.get(keyword)
            if children is None:
                continue
            _schema_error(
                isinstance(children, list) and bool(children),
                "invalid_schema",
                child_path(schema_path, keyword),
                f"{keyword} must be a non-empty schema array",
            )
            for index, child in enumerate(children):
                self._check_schema(
                    child, child_path(child_path(schema_path, keyword), index)
                )

        if "not" in schema:
            self._check_schema(schema["not"], child_path(schema_path, "not"))

        if "enum" in schema:
            values = schema["enum"]
            _schema_error(
                isinstance(values, list) and bool(values),
                "invalid_schema",
                child_path(schema_path, "enum"),
                "enum must be a non-empty array",
            )

        for keyword in ("minimum", "maximum"):
            if keyword in schema:
                _schema_error(
                    _is_number(schema[keyword]),
                    "invalid_schema",
                    child_path(schema_path, keyword),
                    f"{keyword} must be a finite number",
                )

        for keyword in ("minItems", "maxItems", "minLength", "maxLength"):
            if keyword in schema:
                value = schema[keyword]
                _schema_error(
                    isinstance(value, int)
                    and not isinstance(value, bool)
                    and value >= 0,
                    "invalid_schema",
                    child_path(schema_path, keyword),
                    f"{keyword} must be a non-negative integer",
                )

        if (
            "minItems" in schema
            and "maxItems" in schema
            and schema["minItems"] > schema["maxItems"]
        ):
            raise SchemaError(
                "invalid_schema",
                schema_path,
                "minItems exceeds maxItems",
            )
        if (
            "minLength" in schema
            and "maxLength" in schema
            and schema["minLength"] > schema["maxLength"]
        ):
            raise SchemaError(
                "invalid_schema",
                schema_path,
                "minLength exceeds maxLength",
            )
        if (
            "minimum" in schema
            and "maximum" in schema
            and schema["minimum"] > schema["maximum"]
        ):
            raise SchemaError(
                "invalid_schema",
                schema_path,
                "minimum exceeds maximum",
            )

        if "pattern" in schema:
            pattern = schema["pattern"]
            _schema_error(
                isinstance(pattern, str),
                "invalid_schema",
                child_path(schema_path, "pattern"),
                "pattern must be a string",
            )
            try:
                re.compile(pattern)
            except re.error as error:
                raise SchemaError(
                    "invalid_pattern",
                    child_path(schema_path, "pattern"),
                    f"pattern cannot be compiled: {error}",
                ) from error

    def resolve_reference(
        self, reference: str, reference_schema_path: str = "#/$ref"
    ) -> tuple[Any, str]:
        if reference == "#":
            return self.document, "#"
        if not reference.startswith("#/"):
            raise SchemaError(
                "unsupported_reference",
                reference_schema_path,
                f"only document-local JSON Pointer references are supported: {reference!r}",
            )
        node = self.document
        path = "#"
        for raw_token in reference[2:].split("/"):
            token = raw_token.replace("~1", "/").replace("~0", "~")
            if isinstance(node, dict) and token in node:
                node = node[token]
            elif isinstance(node, list) and token.isdigit() and int(token) < len(node):
                node = node[int(token)]
            else:
                raise SchemaError(
                    "invalid_reference",
                    reference_schema_path,
                    f"unresolved schema reference {reference!r}",
                )
            path = child_path(path, token)
        if not isinstance(node, (dict, bool)):
            raise SchemaError(
                "invalid_reference_target",
                reference_schema_path,
                f"schema reference {reference!r} does not target a schema",
            )
        return node, path

    def validate(self, instance: Any) -> list[Diagnostic]:
        return self._validate(instance, self.document, "$", "#", frozenset())

    def validate_subschema(
        self, instance: Any, schema: Any, schema_path: str
    ) -> list[Diagnostic]:
        """Validate against a schema node from this validator's document."""

        return self._validate(
            instance, schema, "$", schema_path, frozenset()
        )

    def is_valid(self, instance: Any) -> bool:
        return not self.validate(instance)

    def _validate(
        self,
        instance: Any,
        schema: Any,
        instance_path: str,
        schema_path: str,
        active_references: frozenset[tuple[str, str]],
    ) -> list[Diagnostic]:
        if schema is True:
            return []
        if schema is False:
            return [
                Diagnostic(
                    "false_schema",
                    instance_path,
                    schema_path,
                    "instance is rejected by a false schema",
                )
            ]
        if not isinstance(schema, dict):
            raise SchemaError(
                "invalid_schema", schema_path, "schema must be an object or boolean"
            )

        if "$ref" in schema:
            reference_path = child_path(schema_path, "$ref")
            target, target_path = self.resolve_reference(
                schema["$ref"], reference_path
            )
            marker = (instance_path, target_path)
            if marker in active_references:
                raise SchemaError(
                    "reference_cycle",
                    reference_path,
                    (
                        "reference cycle revisits the same schema target for "
                        f"the same instance path: {target_path}"
                    ),
                )
            return self._validate(
                instance,
                target,
                instance_path,
                target_path,
                active_references | {marker},
            )

        expected = schema.get("type")
        if expected is not None:
            expected_values = [expected] if isinstance(expected, str) else expected
            assert isinstance(expected_values, list)
            if not any(_matches_type(instance, item) for item in expected_values):
                return [
                    Diagnostic(
                        "type_mismatch",
                        instance_path,
                        child_path(schema_path, "type"),
                        f"expected type {expected_values}, got {_instance_type(instance)}",
                    )
                ]

        errors: list[Diagnostic] = []

        if "enum" in schema and not any(
            _json_equal(instance, candidate) for candidate in schema["enum"]
        ):
            errors.append(
                Diagnostic(
                    "enum_mismatch",
                    instance_path,
                    child_path(schema_path, "enum"),
                    "instance is not one of the enumerated values",
                )
            )
        if "const" in schema and not _json_equal(instance, schema["const"]):
            errors.append(
                Diagnostic(
                    "const_mismatch",
                    instance_path,
                    child_path(schema_path, "const"),
                    "instance does not equal the required constant",
                )
            )

        for index, child in enumerate(schema.get("allOf", [])):
            errors.extend(
                self._validate(
                    instance,
                    child,
                    instance_path,
                    child_path(child_path(schema_path, "allOf"), index),
                    active_references,
                )
            )

        if "anyOf" in schema:
            matches = tuple(
                index
                for index, child in enumerate(schema["anyOf"])
                if not self._validate(
                    instance,
                    child,
                    instance_path,
                    child_path(child_path(schema_path, "anyOf"), index),
                    active_references,
                )
            )
            if not matches:
                errors.append(
                    Diagnostic(
                        "any_of_zero",
                        instance_path,
                        child_path(schema_path, "anyOf"),
                        "instance matched no anyOf branch",
                    )
                )

        if "oneOf" in schema:
            matches = tuple(
                index
                for index, child in enumerate(schema["oneOf"])
                if not self._validate(
                    instance,
                    child,
                    instance_path,
                    child_path(child_path(schema_path, "oneOf"), index),
                    active_references,
                )
            )
            if len(matches) != 1:
                code = "one_of_zero" if not matches else "one_of_multiple"
                errors.append(
                    Diagnostic(
                        code,
                        instance_path,
                        child_path(schema_path, "oneOf"),
                        (
                            f"instance matched branches {list(matches)}; "
                            "expected exactly one"
                        ),
                        matches,
                    )
                )

        if "not" in schema:
            prohibited = self._validate(
                instance,
                schema["not"],
                instance_path,
                child_path(schema_path, "not"),
                active_references,
            )
            if not prohibited:
                errors.append(
                    Diagnostic(
                        "not_match",
                        instance_path,
                        child_path(schema_path, "not"),
                        "instance matched the prohibited schema",
                    )
                )

        if isinstance(instance, dict):
            properties = schema.get("properties", {})
            required = schema.get("required", [])
            for name in required:
                if name not in instance:
                    errors.append(
                        Diagnostic(
                            "required_missing",
                            child_path(instance_path, name),
                            child_path(schema_path, "required"),
                            f"required property {name!r} is missing",
                        )
                    )
            for name, child in properties.items():
                if name in instance:
                    errors.extend(
                        self._validate(
                            instance[name],
                            child,
                            child_path(instance_path, name),
                            child_path(
                                child_path(schema_path, "properties"), name
                            ),
                            active_references,
                        )
                    )
            additional = schema.get("additionalProperties", True)
            for name in sorted(set(instance) - set(properties)):
                if additional is False:
                    errors.append(
                        Diagnostic(
                            "additional_property",
                            child_path(instance_path, name),
                            child_path(schema_path, "additionalProperties"),
                            f"additional property {name!r} is not allowed",
                        )
                    )
                elif additional is not True:
                    errors.extend(
                        self._validate(
                            instance[name],
                            additional,
                            child_path(instance_path, name),
                            child_path(schema_path, "additionalProperties"),
                            active_references,
                        )
                    )

        if isinstance(instance, list):
            if "minItems" in schema and len(instance) < schema["minItems"]:
                errors.append(
                    Diagnostic(
                        "min_items",
                        instance_path,
                        child_path(schema_path, "minItems"),
                        f"array length {len(instance)} is below {schema['minItems']}",
                    )
                )
            if "maxItems" in schema and len(instance) > schema["maxItems"]:
                errors.append(
                    Diagnostic(
                        "max_items",
                        instance_path,
                        child_path(schema_path, "maxItems"),
                        f"array length {len(instance)} exceeds {schema['maxItems']}",
                    )
                )
            items = schema.get("items")
            if isinstance(items, list):
                for index, (value, child) in enumerate(zip(instance, items)):
                    errors.extend(
                        self._validate(
                            value,
                            child,
                            child_path(instance_path, index),
                            child_path(child_path(schema_path, "items"), index),
                            active_references,
                        )
                    )
            elif items is not None:
                for index, value in enumerate(instance):
                    errors.extend(
                        self._validate(
                            value,
                            items,
                            child_path(instance_path, index),
                            child_path(schema_path, "items"),
                            active_references,
                        )
                    )

        if isinstance(instance, str):
            if "minLength" in schema and len(instance) < schema["minLength"]:
                errors.append(
                    Diagnostic(
                        "min_length",
                        instance_path,
                        child_path(schema_path, "minLength"),
                        (
                            f"string length {len(instance)} is below "
                            f"{schema['minLength']}"
                        ),
                    )
                )
            if "maxLength" in schema and len(instance) > schema["maxLength"]:
                errors.append(
                    Diagnostic(
                        "max_length",
                        instance_path,
                        child_path(schema_path, "maxLength"),
                        (
                            f"string length {len(instance)} exceeds "
                            f"{schema['maxLength']}"
                        ),
                    )
                )
            if "pattern" in schema and re.search(schema["pattern"], instance) is None:
                errors.append(
                    Diagnostic(
                        "pattern",
                        instance_path,
                        child_path(schema_path, "pattern"),
                        "string does not match the required pattern",
                    )
                )

        if _is_number(instance):
            if "minimum" in schema and instance < schema["minimum"]:
                errors.append(
                    Diagnostic(
                        "minimum",
                        instance_path,
                        child_path(schema_path, "minimum"),
                        f"number is below minimum {schema['minimum']}",
                    )
                )
            if "maximum" in schema and instance > schema["maximum"]:
                errors.append(
                    Diagnostic(
                        "maximum",
                        instance_path,
                        child_path(schema_path, "maximum"),
                        f"number exceeds maximum {schema['maximum']}",
                    )
                )

        return errors


def _instance_type(instance: Any) -> str:
    if instance is None:
        return "null"
    if isinstance(instance, bool):
        return "boolean"
    if isinstance(instance, int):
        return "integer"
    if isinstance(instance, float):
        return "number"
    if isinstance(instance, str):
        return "string"
    if isinstance(instance, list):
        return "array"
    if isinstance(instance, dict):
        return "object"
    return type(instance).__name__


def validate(instance: Any, schema: Any) -> list[Diagnostic]:
    return Validator(schema).validate(instance)


def schema_nodes(
    schema: Any, schema_path: str = "#"
) -> Iterator[tuple[str, Mapping[str, Any]]]:
    """Yield raw schema positions without following references."""

    if isinstance(schema, bool):
        return
    if not isinstance(schema, dict):
        raise SchemaError(
            "invalid_schema", schema_path, "schema must be an object or boolean"
        )
    yield schema_path, schema
    for keyword in ("definitions", "properties"):
        children = schema.get(keyword, {})
        if isinstance(children, dict):
            for name, child in sorted(children.items()):
                yield from schema_nodes(
                    child, child_path(child_path(schema_path, keyword), name)
                )
    additional = schema.get("additionalProperties")
    if isinstance(additional, (dict, bool)):
        yield from schema_nodes(
            additional, child_path(schema_path, "additionalProperties")
        )
    items = schema.get("items")
    if isinstance(items, list):
        for index, child in enumerate(items):
            yield from schema_nodes(
                child, child_path(child_path(schema_path, "items"), index)
            )
    elif isinstance(items, (dict, bool)):
        yield from schema_nodes(items, child_path(schema_path, "items"))
    for keyword in ("allOf", "anyOf", "oneOf"):
        children = schema.get(keyword, [])
        if isinstance(children, list):
            for index, child in enumerate(children):
                yield from schema_nodes(
                    child, child_path(child_path(schema_path, keyword), index)
                )
    if isinstance(schema.get("not"), (dict, bool)):
        yield from schema_nodes(schema["not"], child_path(schema_path, "not"))


def keyword_counts(schema: Any) -> dict[str, int]:
    counts: dict[str, int] = {}
    for _, node in schema_nodes(schema):
        for keyword in node:
            counts[keyword] = counts.get(keyword, 0) + 1
    return dict(sorted(counts.items()))


def _load_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise SchemaError(
            "invalid_json", "#", f"unable to load JSON {path}: {error}"
        ) from error


def _print_json(value: Any) -> None:
    print(json.dumps(value, ensure_ascii=False, indent=2, sort_keys=True))


def _command_check_schema(arguments: argparse.Namespace) -> int:
    document = _load_json(arguments.schema)
    Validator(document)
    _print_json({"valid_schema": True})
    return 0


def _command_validate(arguments: argparse.Namespace) -> int:
    document = _load_json(arguments.schema)
    instance = _load_json(arguments.instance)
    diagnostics = Validator(document).validate(instance)
    _print_json(
        {
            "valid": not diagnostics,
            "diagnostics": [diagnostic.to_json() for diagnostic in diagnostics],
        }
    )
    return 0 if not diagnostics else 1


def _command_keywords(arguments: argparse.Namespace) -> int:
    document = _load_json(arguments.schema)
    Validator(document)
    _print_json(
        {
            "format_version": FORMAT_VERSION,
            "actual": keyword_counts(document),
            "supported_annotations": sorted(ANNOTATION_KEYWORDS),
            "supported_structural": sorted(STRUCTURAL_KEYWORDS),
            "supported_validating": sorted(VALIDATING_KEYWORDS),
        }
    )
    return 0


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    check = subparsers.add_parser(
        "check-schema", help="verify that a schema uses the supported subset"
    )
    check.add_argument("--schema", type=Path, required=True)
    check.set_defaults(function=_command_check_schema)

    validate_parser = subparsers.add_parser(
        "validate", help="validate one JSON instance"
    )
    validate_parser.add_argument("--schema", type=Path, required=True)
    validate_parser.add_argument("--instance", type=Path, required=True)
    validate_parser.set_defaults(function=_command_validate)

    keywords = subparsers.add_parser(
        "keywords", help="report schema-position keyword usage"
    )
    keywords.add_argument("--schema", type=Path, required=True)
    keywords.set_defaults(function=_command_keywords)
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    arguments = _parser().parse_args(argv)
    try:
        return int(arguments.function(arguments))
    except SchemaError as error:
        _print_json({"schema_error": error.to_json()})
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
