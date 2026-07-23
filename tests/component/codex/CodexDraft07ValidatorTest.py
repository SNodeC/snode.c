#!/usr/bin/env python3
"""Isolated self-tests for the offline Codex Draft-07 validator."""

from __future__ import annotations

import importlib.util
import sys
import unittest
from pathlib import Path
from types import ModuleType

sys.dont_write_bytecode = True


def load_validator() -> ModuleType:
    root = Path(__file__).resolve().parents[3]
    path = root / "tools" / "codex" / "draft07.py"
    specification = importlib.util.spec_from_file_location(
        "snodec_codex_draft07", path
    )
    if specification is None or specification.loader is None:
        raise RuntimeError(f"unable to load validator: {path}")
    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


draft07 = load_validator()


class Draft07ValidatorTest(unittest.TestCase):
    def assert_codes(
        self, schema: object, instance: object, expected: list[str]
    ) -> list[object]:
        diagnostics = draft07.Validator(schema).validate(instance)
        self.assertEqual([item.code for item in diagnostics], expected)
        return diagnostics

    def test_valid_object(self) -> None:
        schema = {
            "type": "object",
            "required": ["enabled", "name"],
            "properties": {
                "enabled": {"type": "boolean"},
                "name": {"type": "string"},
            },
            "additionalProperties": False,
        }
        self.assertFalse(
            draft07.Validator(schema).validate(
                {"enabled": True, "name": "fixture"}
            )
        )

    def test_missing_required_has_structured_paths(self) -> None:
        diagnostics = self.assert_codes(
            {
                "type": "object",
                "required": ["name"],
                "properties": {"name": {"type": "string"}},
            },
            {},
            ["required_missing"],
        )
        self.assertEqual(diagnostics[0].instance_path, "$/name")
        self.assertEqual(diagnostics[0].schema_path, "#/required")

    def test_wrong_type(self) -> None:
        diagnostics = self.assert_codes(
            {
                "type": "object",
                "required": ["count"],
                "properties": {"count": {"type": "integer"}},
            },
            {"count": "one"},
            ["type_mismatch"],
        )
        self.assertEqual(diagnostics[0].instance_path, "$/count")
        self.assertEqual(
            diagnostics[0].schema_path, "#/properties/count/type"
        )

    def test_enum_and_const_mismatch(self) -> None:
        self.assert_codes({"enum": ["a", "b"]}, "c", ["enum_mismatch"])
        self.assert_codes({"const": {"kind": "known"}}, {}, ["const_mismatch"])

    def test_json_numeric_equality_and_boolean_distinction(self) -> None:
        self.assert_codes({"enum": [1]}, 1.0, [])
        self.assert_codes({"const": 1}, 1.0, [])
        self.assert_codes({"enum": [0]}, -0.0, [])
        self.assert_codes({"const": 0}, -0.0, [])
        self.assert_codes({"enum": [True]}, 1, ["enum_mismatch"])
        self.assert_codes({"const": True}, 1, ["const_mismatch"])

    def test_local_reference_resolution(self) -> None:
        schema = {
            "$ref": "#/definitions/value",
            "definitions": {"value": {"type": "string", "minLength": 1}},
        }
        validator = draft07.Validator(schema)
        self.assertFalse(validator.validate("ok"))
        diagnostics = validator.validate("")
        self.assertEqual([item.code for item in diagnostics], ["min_length"])
        self.assertEqual(
            diagnostics[0].schema_path, "#/definitions/value/minLength"
        )

    def test_nested_recursive_reference(self) -> None:
        schema = {
            "$ref": "#/definitions/node",
            "definitions": {
                "node": {
                    "type": "object",
                    "required": ["next", "value"],
                    "properties": {
                        "next": {
                            "anyOf": [
                                {"type": "null"},
                                {"$ref": "#/definitions/node"},
                            ]
                        },
                        "value": {"type": "integer"},
                    },
                    "additionalProperties": False,
                }
            },
        }
        instance = {
            "value": 1,
            "next": {"value": 2, "next": {"value": 3, "next": None}},
        }
        self.assertFalse(draft07.Validator(schema).validate(instance))

    def test_one_of_zero_reports_exact_empty_branch_set(self) -> None:
        diagnostics = self.assert_codes(
            {"oneOf": [{"type": "string"}, {"type": "integer"}]},
            False,
            ["one_of_zero"],
        )
        self.assertEqual(diagnostics[0].schema_path, "#/oneOf")
        self.assertEqual(diagnostics[0].branch_indices, ())
        self.assertEqual(diagnostics[0].to_json()["branch_indices"], [])

    def test_one_of_multiple_reports_exact_matching_branch_set(self) -> None:
        diagnostics = self.assert_codes(
            {
                "oneOf": [
                    {"type": "number"},
                    {"type": "integer"},
                    {"minimum": 100},
                ]
            },
            104,
            ["one_of_multiple"],
        )
        self.assertEqual(diagnostics[0].branch_indices, (0, 1, 2))
        self.assertEqual(
            diagnostics[0].to_json()["branch_indices"], [0, 1, 2]
        )

    def test_any_of(self) -> None:
        validator = draft07.Validator(
            {"anyOf": [{"type": "string"}, {"type": "integer"}]}
        )
        self.assertFalse(validator.validate(4))
        self.assertEqual(
            [item.code for item in validator.validate(False)],
            ["any_of_zero"],
        )

    def test_all_of(self) -> None:
        validator = draft07.Validator(
            {
                "allOf": [
                    {"type": "integer"},
                    {"minimum": 2},
                    {"maximum": 4},
                ]
            }
        )
        self.assertFalse(validator.validate(3))
        self.assertEqual(
            [item.code for item in validator.validate(5)], ["maximum"]
        )

    def test_not(self) -> None:
        validator = draft07.Validator({"not": {"enum": ["reserved"]}})
        self.assertFalse(validator.validate("available"))
        self.assertEqual(
            [item.code for item in validator.validate("reserved")],
            ["not_match"],
        )

    def test_additional_properties_false(self) -> None:
        diagnostics = self.assert_codes(
            {
                "type": "object",
                "properties": {"known": {"type": "string"}},
                "additionalProperties": False,
            },
            {"known": "yes", "future": True},
            ["additional_property"],
        )
        self.assertEqual(diagnostics[0].instance_path, "$/future")

    def test_array_item_failure(self) -> None:
        diagnostics = self.assert_codes(
            {"type": "array", "items": {"type": "integer"}},
            [1, "two", 3],
            ["type_mismatch"],
        )
        self.assertEqual(diagnostics[0].instance_path, "$/1")
        self.assertEqual(diagnostics[0].schema_path, "#/items/type")

    def test_numeric_bounds(self) -> None:
        validator = draft07.Validator(
            {"type": "number", "minimum": -2, "maximum": 2}
        )
        self.assertFalse(validator.validate(-2))
        self.assertFalse(validator.validate(2))
        self.assertEqual(
            [item.code for item in validator.validate(-3)], ["minimum"]
        )
        self.assertEqual(
            [item.code for item in validator.validate(3)], ["maximum"]
        )

    def test_string_bounds(self) -> None:
        validator = draft07.Validator(
            {"type": "string", "minLength": 2, "maxLength": 4}
        )
        self.assertFalse(validator.validate("okay"))
        self.assertEqual(
            [item.code for item in validator.validate("x")], ["min_length"]
        )
        self.assertEqual(
            [item.code for item in validator.validate("large")],
            ["max_length"],
        )

    def test_array_bounds(self) -> None:
        validator = draft07.Validator(
            {"type": "array", "minItems": 1, "maxItems": 2}
        )
        self.assertFalse(validator.validate([1]))
        self.assertEqual(
            [item.code for item in validator.validate([])], ["min_items"]
        )
        self.assertEqual(
            [item.code for item in validator.validate([1, 2, 3])],
            ["max_items"],
        )

    def test_pattern(self) -> None:
        validator = draft07.Validator(
            {"type": "string", "pattern": "^[a-z]+-[0-9]+$"}
        )
        self.assertFalse(validator.validate("item-42"))
        self.assertEqual(
            [item.code for item in validator.validate("ITEM")], ["pattern"]
        )

    def test_unsupported_validating_keyword_fails_loudly(self) -> None:
        with self.assertRaises(draft07.SchemaError) as context:
            draft07.Validator({"type": "array", "uniqueItems": True})
        self.assertEqual(context.exception.code, "unsupported_keyword")
        self.assertEqual(context.exception.schema_path, "#")
        self.assertIn("uniqueItems", context.exception.message)

    def test_id_scope_keyword_fails_loudly(self) -> None:
        with self.assertRaises(draft07.SchemaError) as context:
            draft07.Validator({"$id": "nested-resource", "type": "string"})
        self.assertEqual(context.exception.code, "unsupported_keyword")
        self.assertEqual(context.exception.schema_path, "#")
        self.assertIn("$id", context.exception.message)

    def test_reference_cycle_fails_loudly(self) -> None:
        schema = {
            "$ref": "#/definitions/a",
            "definitions": {
                "a": {"$ref": "#/definitions/b"},
                "b": {"$ref": "#/definitions/a"},
            },
        }
        with self.assertRaises(draft07.SchemaError) as context:
            draft07.Validator(schema).validate("value")
        self.assertEqual(context.exception.code, "reference_cycle")
        self.assertEqual(
            context.exception.schema_path, "#/definitions/b/$ref"
        )

    def test_invalid_reference_fails_loudly(self) -> None:
        with self.assertRaises(draft07.SchemaError) as context:
            draft07.Validator({"$ref": "#/definitions/missing"})
        self.assertEqual(context.exception.code, "invalid_reference")
        self.assertEqual(context.exception.schema_path, "#/$ref")

    def test_non_local_reference_fails_loudly(self) -> None:
        with self.assertRaises(draft07.SchemaError) as context:
            draft07.Validator({"$ref": "https://example.invalid/schema"})
        self.assertEqual(context.exception.code, "unsupported_reference")


if __name__ == "__main__":
    unittest.main()
