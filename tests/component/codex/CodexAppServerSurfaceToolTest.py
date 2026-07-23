#!/usr/bin/env python3

"""Focused, offline guards for the vendored Codex App Server surface."""

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
from pathlib import Path
from types import ModuleType
from typing import Callable

sys.dont_write_bytecode = True


def load_tool(path: Path) -> ModuleType:
    specification = importlib.util.spec_from_file_location(
        "snodec_codex_app_server_surface", path
    )
    if specification is None or specification.loader is None:
        raise RuntimeError(f"unable to load surface tool: {path}")
    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


def load_json(path: Path) -> object:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, value: object) -> None:
    path.write_text(
        json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )


def expect_surface_error(
    tool: ModuleType, action: Callable[[], object], description: str
) -> None:
    try:
        action()
    except tool.SurfaceError:
        return
    raise AssertionError(f"negative guard mutation did not fail: {description}")


def independent_tree_records(root: Path) -> list[dict[str, object]]:
    records: list[dict[str, object]] = []
    for path in sorted(candidate for candidate in root.rglob("*") if candidate.is_file()):
        data = path.read_bytes()
        records.append(
            {
                "path": path.relative_to(root).as_posix(),
                "bytes": len(data),
                "sha256": hashlib.sha256(data).hexdigest(),
            }
        )
    if not records:
        raise AssertionError(f"schema tree is empty: {root}")
    return records


def independent_aggregate(records: list[dict[str, object]]) -> str:
    payload = "".join(
        f"{record['sha256']}  {record['path']}\n" for record in records
    ).encode("utf-8")
    return hashlib.sha256(payload).hexdigest()


def test_vendored(
    tool: ModuleType, schema_root: Path, provenance_path: Path, manifest_path: Path
) -> None:
    provenance = load_json(provenance_path)
    if not isinstance(provenance, dict):
        raise AssertionError("provenance must be a JSON object")

    for stability in ("stable", "experimental"):
        records = independent_tree_records(schema_root / stability)
        expected = provenance["schema_trees"][stability]
        if records != expected["files"]:
            raise AssertionError(
                f"{stability} independently computed per-file hashes differ"
            )
        aggregate = independent_aggregate(records)
        if aggregate != expected["aggregate_sha256"]:
            raise AssertionError(
                f"{stability} independently computed aggregate hash differs"
            )

    tool.verify_provenance(schema_root, provenance_path)
    if tool.extract_surface(schema_root) != load_json(manifest_path):
        raise AssertionError("committed manifest is stale")

    with tempfile.TemporaryDirectory(prefix="snodec-codex-vendored-guard-") as raw:
        temporary = Path(raw)
        copied_root = temporary / "schema"
        shutil.copytree(schema_root, copied_root)
        copied_provenance = copied_root / "PROVENANCE.json"

        first_record = provenance["schema_trees"]["stable"]["files"][0]
        relative = Path(first_record["path"])
        source_file = schema_root / "stable" / relative
        copied_file = copied_root / "stable" / relative

        copied_file.unlink()
        expect_surface_error(
            tool,
            lambda: tool.verify_provenance(copied_root, copied_provenance),
            "remove a vendored schema",
        )

        shutil.copy2(source_file, copied_file)
        copied_file.write_bytes(copied_file.read_bytes() + b"\n")
        expect_surface_error(
            tool,
            lambda: tool.verify_provenance(copied_root, copied_provenance),
            "corrupt a vendored schema",
        )

        shutil.copy2(source_file, copied_file)
        (copied_root / "stable" / "unlisted.json").write_text(
            "{}\n", encoding="utf-8"
        )
        expect_surface_error(
            tool,
            lambda: tool.verify_provenance(copied_root, copied_provenance),
            "add an unhashed vendored schema",
        )
        (copied_root / "stable" / "unlisted.json").unlink()

        corrupted_provenance = copy.deepcopy(provenance)
        corrupted_provenance["schema_trees"]["stable"]["files"][0][
            "sha256"
        ] = "0" * 64
        write_json(copied_provenance, corrupted_provenance)
        expect_surface_error(
            tool,
            lambda: tool.verify_provenance(copied_root, copied_provenance),
            "corrupt the checked-in hash manifest",
        )
        copied_provenance.unlink()
        expect_surface_error(
            tool,
            lambda: tool.verify_provenance(copied_root, copied_provenance),
            "remove the checked-in hash manifest",
        )


def replace_json(path: Path, transform: Callable[[dict[str, object]], None]) -> None:
    document = load_json(path)
    if not isinstance(document, dict):
        raise AssertionError(f"expected JSON object: {path}")
    transform(document)
    write_json(path, document)


def test_extraction(
    tool: ModuleType, schema_root: Path, manifest_path: Path
) -> None:
    first = tool.extract_surface(schema_root)
    second = tool.extract_surface(schema_root)
    if tool.canonical_json(first) != tool.canonical_json(second):
        raise AssertionError("two schema extractions were not byte-deterministic")
    if first != load_json(manifest_path):
        raise AssertionError("re-extracted surface differs from committed manifest")

    entries = first.get("entries")
    if not isinstance(entries, list) or not entries:
        raise AssertionError("surface extraction produced no entries")
    identities = [
        (
            entry["category"],
            entry["domain"],
            entry["discriminator_field"],
            entry["name"],
        )
        for entry in entries
    ]
    if identities != sorted(identities) or len(identities) != len(set(identities)):
        raise AssertionError("surface entries are not sorted and unique")
    expect_surface_error(
        tool,
        lambda: tool.detect_category_collisions(
            [
                {
                    "category": "item_discriminator",
                    "domain": "SyntheticUnion",
                    "discriminator_field": "type",
                    "name": "same",
                },
                {
                    "category": "tagged_union_discriminator",
                    "domain": "SyntheticUnion",
                    "discriminator_field": "type",
                    "name": "same",
                },
            ]
        ),
        "classify one discriminator identity into two categories",
    )

    with tempfile.TemporaryDirectory(prefix="snodec-codex-extraction-guard-") as raw:
        copied_root = Path(raw) / "schema"
        shutil.copytree(schema_root, copied_root)

        def restore(relative: str, stability: str = "stable") -> Path:
            source = schema_root / stability / relative
            destination = copied_root / stability / relative
            shutil.copy2(source, destination)
            return destination

        client_request = copied_root / "stable" / "ClientRequest.json"

        def unresolved_reference(document: dict[str, object]) -> None:
            document["oneOf"][0]["properties"]["id"]["$ref"] = (
                "#/definitions/DefinitelyMissing"
            )

        replace_json(client_request, unresolved_reference)
        expect_surface_error(
            tool,
            lambda: tool.extract_surface(copied_root),
            "introduce an unresolved repository-local reference",
        )
        restore("ClientRequest.json")

        def unrecognized_layout(document: dict[str, object]) -> None:
            document["title"] = "UnexpectedClientRequestLayout"

        replace_json(client_request, unrecognized_layout)
        expect_surface_error(
            tool,
            lambda: tool.extract_surface(copied_root),
            "change an authoritative top-level schema layout",
        )
        restore("ClientRequest.json")

        def remove_envelope_type(document: dict[str, object]) -> None:
            document["oneOf"][0].pop("type")

        replace_json(client_request, remove_envelope_type)
        expect_surface_error(
            tool,
            lambda: tool.extract_surface(copied_root),
            "remove the object type from a method envelope branch",
        )
        restore("ClientRequest.json")

        def remove_envelope_required(document: dict[str, object]) -> None:
            document["oneOf"][0].pop("required")

        replace_json(client_request, remove_envelope_required)
        expect_surface_error(
            tool,
            lambda: tool.extract_surface(copied_root),
            "remove required envelope fields from a method branch",
        )
        restore("ClientRequest.json")

        def duplicate_method(document: dict[str, object]) -> None:
            document["oneOf"].append(copy.deepcopy(document["oneOf"][0]))

        replace_json(client_request, duplicate_method)
        expect_surface_error(
            tool,
            lambda: tool.extract_surface(copied_root),
            "duplicate a schema-derived method",
        )
        restore("ClientRequest.json")

        def collide_method_category(document: dict[str, object]) -> None:
            document["oneOf"][0]["properties"]["method"]["enum"] = ["initialize"]

        for stability in ("stable", "experimental"):
            notification = copied_root / stability / "ClientNotification.json"
            replace_json(notification, collide_method_category)
        expect_surface_error(
            tool,
            lambda: tool.extract_surface(copied_root),
            "collide a client notification with a client request method",
        )


def test_generated_artifacts(
    tool: ModuleType,
    manifest_path: Path,
    provenance_path: Path,
    registry_path: Path,
    coverage_path: Path,
    security_path: Path,
) -> None:
    manifest = load_json(manifest_path)
    provenance = load_json(provenance_path)
    generated_registry = tool.generate_registry_data(manifest)
    if generated_registry != registry_path.read_text(encoding="utf-8"):
        raise AssertionError(
            "canonical production registry data is stale against local status mappings"
        )
    registry_entries = tool.parse_registry_data(registry_path)
    registry = tool.registry_by_key(registry_entries)
    operations = [
        entry
        for entry in manifest["entries"]
        if entry["category"] in {"client_request", "server_request"}
    ]
    unresolved_statuses = {
        "Unresolved",
        "ExistingOperationSubsetExpansionUnresolved",
        "ExistingGenericContractDedicatedUnresolved",
    }
    unresolved = [
        entry
        for entry in operations
        if registry[
            (
                entry["category"],
                entry["domain"],
                entry["discriminator_field"],
                entry["name"],
            )
        ]["frontend_security"]
        in unresolved_statuses
    ]
    resolved = [
        (entry["category"], entry["name"])
        for entry in operations
        if entry not in unresolved
    ]
    if len(operations) != 133 or len(unresolved) != 132 or resolved != [
        ("client_request", "initialize")
    ]:
        raise AssertionError(
            "owner worksheet must leave 132/133 operations unresolved, with only internal initialize not applicable"
        )
    generated_coverage = tool.render_coverage_document(
        manifest, registry_entries, provenance
    )
    if generated_coverage != coverage_path.read_text(encoding="utf-8"):
        raise AssertionError("generated App Server coverage document is stale")
    generated_security = tool.render_security_document(manifest, registry_entries)
    if "Unresolved owner decisions: **132/133** operations." not in generated_security:
        raise AssertionError(
            "generated owner worksheet does not report the frozen 132/133 unresolved decision count"
        )
    if generated_security != security_path.read_text(encoding="utf-8"):
        raise AssertionError("generated owner security worksheet is stale")


SUPPORTED_SCHEMA_KEYS = {
    "$ref",
    "$schema",
    "additionalProperties",
    "allOf",
    "anyOf",
    "const",
    "default",
    "definitions",
    "description",
    "enum",
    "enumNames",
    "format",
    "items",
    "maxItems",
    "maxLength",
    "maximum",
    "minItems",
    "minLength",
    "minimum",
    "not",
    "oneOf",
    "pattern",
    "properties",
    "required",
    "title",
    "type",
}


def assert_supported_schema(schema: object, path: str = "$") -> None:
    """Reject schema keywords that this offline Draft-07 subset cannot enforce."""

    if isinstance(schema, bool):
        return
    if not isinstance(schema, dict):
        raise AssertionError(f"{path} is not a JSON Schema object or boolean")

    unsupported = sorted(set(schema) - SUPPORTED_SCHEMA_KEYS)
    if unsupported:
        raise AssertionError(
            f"{path} uses unsupported JSON Schema keywords: {unsupported}"
        )

    for keyword in ("definitions", "properties"):
        children = schema.get(keyword, {})
        if not isinstance(children, dict):
            raise AssertionError(f"{path}.{keyword} is not an object")
        for name, child in children.items():
            assert_supported_schema(child, f"{path}.{keyword}.{name}")

    additional = schema.get("additionalProperties", True)
    if not isinstance(additional, bool):
        assert_supported_schema(additional, f"{path}.additionalProperties")

    items = schema.get("items")
    if isinstance(items, list):
        for index, child in enumerate(items):
            assert_supported_schema(child, f"{path}.items[{index}]")
    elif items is not None:
        assert_supported_schema(items, f"{path}.items")

    for keyword in ("allOf", "anyOf", "oneOf"):
        children = schema.get(keyword, [])
        if not isinstance(children, list):
            raise AssertionError(f"{path}.{keyword} is not an array")
        for index, child in enumerate(children):
            assert_supported_schema(child, f"{path}.{keyword}[{index}]")

    if "not" in schema:
        assert_supported_schema(schema["not"], f"{path}.not")


def resolve_schema_reference(document: object, reference: object) -> object:
    if reference == "#":
        return document
    if not isinstance(reference, str) or not reference.startswith("#/"):
        raise AssertionError(f"unsupported non-local schema reference: {reference!r}")

    node = document
    for raw_token in reference[2:].split("/"):
        token = raw_token.replace("~1", "/").replace("~0", "~")
        if not isinstance(node, dict) or token not in node:
            raise AssertionError(f"unresolved local schema reference: {reference}")
        node = node[token]
    return node


def json_values_equal(left: object, right: object) -> bool:
    return json.dumps(
        left, ensure_ascii=False, sort_keys=True, separators=(",", ":")
    ) == json.dumps(right, ensure_ascii=False, sort_keys=True, separators=(",", ":"))


def matches_json_type(instance: object, expected: str) -> bool:
    if expected == "null":
        return instance is None
    if expected == "boolean":
        return isinstance(instance, bool)
    if expected == "integer":
        return isinstance(instance, int) and not isinstance(instance, bool)
    if expected == "number":
        return isinstance(instance, (int, float)) and not isinstance(instance, bool)
    if expected == "string":
        return isinstance(instance, str)
    if expected == "array":
        return isinstance(instance, list)
    if expected == "object":
        return isinstance(instance, dict)
    raise AssertionError(f"unsupported JSON Schema type: {expected!r}")


def validate_json_schema(
    instance: object,
    schema: object,
    document: object,
    path: str = "$",
    reference_stack: frozenset[tuple[str, int]] = frozenset(),
) -> list[str]:
    if schema is True:
        return []
    if schema is False:
        return [f"{path}: rejected by false schema"]
    if not isinstance(schema, dict):
        raise AssertionError(f"{path}: schema is not an object or boolean")

    if "$ref" in schema:
        reference = schema["$ref"]
        if not isinstance(reference, str):
            raise AssertionError(f"{path}: schema reference is not a string")
        cycle_key = (reference, id(instance))
        if cycle_key in reference_stack:
            raise AssertionError(
                f"{path}: cyclic schema reference for the same instance: {reference}"
            )
        return validate_json_schema(
            instance,
            resolve_schema_reference(document, reference),
            document,
            path,
            reference_stack | {cycle_key},
        )

    expected_types = schema.get("type")
    if expected_types is not None:
        if isinstance(expected_types, str):
            expected_types = [expected_types]
        if not isinstance(expected_types, list) or not all(
            isinstance(expected, str) for expected in expected_types
        ):
            raise AssertionError(f"{path}: schema type is not a string or string array")
        if not any(matches_json_type(instance, expected) for expected in expected_types):
            return [f"{path}: value does not match type {expected_types}"]

    if "enum" in schema:
        choices = schema["enum"]
        if not isinstance(choices, list):
            raise AssertionError(f"{path}: schema enum is not an array")
        if not any(json_values_equal(instance, choice) for choice in choices):
            return [f"{path}: value is not one of the enumerated choices"]
    if "const" in schema and not json_values_equal(instance, schema["const"]):
        return [f"{path}: value does not match const"]

    errors: list[str] = []
    for index, child in enumerate(schema.get("allOf", [])):
        errors.extend(
            f"{path}.allOf[{index}] {error}"
            for error in validate_json_schema(instance, child, document, path)
        )

    any_of = schema.get("anyOf")
    if any_of is not None and not any(
        not validate_json_schema(instance, child, document, path)
        for child in any_of
    ):
        errors.append(f"{path}: value does not match any anyOf branch")

    one_of = schema.get("oneOf")
    if one_of is not None:
        matches = sum(
            not validate_json_schema(instance, child, document, path)
            for child in one_of
        )
        if matches != 1:
            errors.append(f"{path}: value matches {matches} oneOf branches, expected 1")

    if "not" in schema and not validate_json_schema(
        instance, schema["not"], document, path
    ):
        errors.append(f"{path}: value matches prohibited not schema")

    if isinstance(instance, dict):
        required = schema.get("required", [])
        if not isinstance(required, list) or not all(
            isinstance(name, str) for name in required
        ):
            raise AssertionError(f"{path}: schema required is not a string array")
        for name in required:
            if name not in instance:
                errors.append(f"{path}: missing required property {name!r}")

        properties = schema.get("properties", {})
        if not isinstance(properties, dict):
            raise AssertionError(f"{path}: schema properties is not an object")
        for name, child in properties.items():
            if name in instance:
                errors.extend(
                    validate_json_schema(
                        instance[name], child, document, f"{path}.{name}"
                    )
                )

        additional = schema.get("additionalProperties", True)
        for name in set(instance) - set(properties):
            if additional is False:
                errors.append(f"{path}: unexpected property {name!r}")
            elif additional is not True:
                errors.extend(
                    validate_json_schema(
                        instance[name],
                        additional,
                        document,
                        f"{path}.{name}",
                    )
                )

    if isinstance(instance, list):
        if "minItems" in schema and len(instance) < schema["minItems"]:
            errors.append(f"{path}: array is shorter than minItems")
        if "maxItems" in schema and len(instance) > schema["maxItems"]:
            errors.append(f"{path}: array is longer than maxItems")
        items = schema.get("items")
        if isinstance(items, list):
            for index, (value, child) in enumerate(zip(instance, items)):
                errors.extend(
                    validate_json_schema(value, child, document, f"{path}[{index}]")
                )
        elif items is not None:
            for index, value in enumerate(instance):
                errors.extend(
                    validate_json_schema(value, items, document, f"{path}[{index}]")
                )

    if isinstance(instance, str):
        if "minLength" in schema and len(instance) < schema["minLength"]:
            errors.append(f"{path}: string is shorter than minLength")
        if "maxLength" in schema and len(instance) > schema["maxLength"]:
            errors.append(f"{path}: string is longer than maxLength")
        if "pattern" in schema and re.search(schema["pattern"], instance) is None:
            errors.append(f"{path}: string does not match pattern")

    if isinstance(instance, (int, float)) and not isinstance(instance, bool):
        if "minimum" in schema and instance < schema["minimum"]:
            errors.append(f"{path}: number is below minimum")
        if "maximum" in schema and instance > schema["maximum"]:
            errors.append(f"{path}: number is above maximum")

    return errors


def test_fixtures(schema_root: Path, fixture_root: Path) -> None:
    index = load_json(fixture_root / "fixtures.json")
    if not isinstance(index, dict) or index.get("format_version") != 1:
        raise AssertionError("unsupported transcript fixture index")

    known_methods: set[str] = set()
    for fixture in index.get("fixtures", []):
        relative = Path(fixture["file"])
        if relative.is_absolute() or ".." in relative.parts:
            raise AssertionError(f"fixture path escapes fixture root: {relative}")
        data = (fixture_root / relative).read_bytes()
        if not data.endswith(b"\n") or data.count(b"\n") != 1 or b"\r" in data:
            raise AssertionError(
                f"fixture is not exactly one LF-framed JSONL record: {relative}"
            )
        instance = json.loads(data[:-1].decode("utf-8"))
        schema = load_json(schema_root / "stable" / fixture["schema"])
        assert_supported_schema(schema)
        errors = validate_json_schema(instance, schema, schema)
        if errors:
            details = "; ".join(errors)
            raise AssertionError(f"{relative} fails {fixture['schema']}: {details}")
        if fixture.get("surface") == "known":
            known_methods.add(instance["method"])
            branches = [
                branch
                for branch in schema.get("oneOf", [])
                if branch.get("properties", {})
                .get("method", {})
                .get("enum", [])
                == [instance["method"]]
            ]
            if len(branches) != 1:
                raise AssertionError(
                    f"{relative} method does not select exactly one vendored branch"
                )
            branch = branches[0]
            envelope_required = set(branch.get("required", []))
            envelope_allowed = envelope_required | set(
                fixture.get("optional_envelope", [])
            )
            if set(instance) != envelope_allowed:
                raise AssertionError(
                    f"{relative} is not the mechanically minimal envelope"
                )
            malformed = copy.deepcopy(instance)
            malformed.pop(sorted(envelope_required)[0])
            if not validate_json_schema(malformed, schema, schema):
                raise AssertionError(
                    f"{relative} schema guard accepted a missing required envelope field"
                )

            def resolve_local(node: object) -> object:
                seen: set[str] = set()
                while isinstance(node, dict) and "$ref" in node:
                    reference = node["$ref"]
                    if not isinstance(reference, str) or not reference.startswith("#/"):
                        raise AssertionError(
                            f"{relative} uses unsupported fixture-validation ref {reference!r}"
                        )
                    if reference in seen:
                        raise AssertionError(
                            f"{relative} has a cyclic fixture-validation ref"
                        )
                    seen.add(reference)
                    node = schema
                    for token in reference[2:].split("/"):
                        token = token.replace("~1", "/").replace("~0", "~")
                        node = node[token]
                return node

            params_schema = resolve_local(branch["properties"]["params"])
            params_required = set(params_schema.get("required", []))
            params_allowed = params_required | set(fixture.get("optional_params", []))
            if set(instance["params"]) != params_allowed:
                raise AssertionError(
                    f"{relative} params differ from vendored required fields plus reviewed optionals"
                )

            item = instance["params"].get("item")
            if isinstance(item, dict):
                item_union = resolve_local(params_schema["properties"]["item"])
                item_branches = [
                    candidate
                    for candidate in item_union.get("oneOf", [])
                    if candidate.get("properties", {})
                    .get("type", {})
                    .get("enum", [])
                    == [item.get("type")]
                ]
                if len(item_branches) != 1:
                    raise AssertionError(
                        f"{relative} item discriminator does not select one vendored branch"
                    )
                item_required = set(item_branches[0].get("required", []))
                item_allowed = item_required | set(fixture.get("optional_item", []))
                if set(item) != item_allowed:
                    raise AssertionError(
                        f"{relative} item differs from the mechanically minimal discriminator branch"
                    )
        else:
            required = set(schema.get("required", []))
            allowed = required | set(fixture.get("optional_envelope", []))
            if set(instance) != allowed:
                raise AssertionError(
                    f"{relative} unknown envelope differs from required fields plus reviewed optionals"
                )

    if not {
        "thread/list",
        "item/agentMessage/delta",
        "item/started",
        "item/commandExecution/requestApproval",
    }.issubset(known_methods):
        raise AssertionError("transcript fixtures do not cover every required flow")

    server_notification = load_json(schema_root / "stable" / "ServerNotification.json")
    assert_supported_schema(server_notification)
    for relative in ("unknown-notification-a.jsonl", "unknown-notification-b.jsonl"):
        instance = json.loads((fixture_root / relative).read_text(encoding="utf-8"))
        if not validate_json_schema(instance, server_notification, server_notification):
            raise AssertionError(
                f"unknown fixture unexpectedly matches enumerated server surface: {relative}"
            )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "mode", choices=("vendored", "extraction", "artifacts", "fixtures")
    )
    parser.add_argument("--tool", type=Path, required=True)
    parser.add_argument("--schema-root", type=Path, required=True)
    parser.add_argument("--provenance", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--fixtures", type=Path, required=True)
    parser.add_argument("--registry", type=Path, required=True)
    parser.add_argument("--coverage-doc", type=Path, required=True)
    parser.add_argument("--security-doc", type=Path, required=True)
    arguments = parser.parse_args()

    tool = load_tool(arguments.tool)
    if arguments.mode == "vendored":
        test_vendored(
            tool, arguments.schema_root, arguments.provenance, arguments.manifest
        )
    elif arguments.mode == "extraction":
        test_extraction(tool, arguments.schema_root, arguments.manifest)
    elif arguments.mode == "artifacts":
        test_generated_artifacts(
            tool,
            arguments.manifest,
            arguments.provenance,
            arguments.registry,
            arguments.coverage_doc,
            arguments.security_doc,
        )
    else:
        test_fixtures(arguments.schema_root, arguments.fixtures)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (
        AssertionError,
        OSError,
        RuntimeError,
        ValueError,
        KeyError,
        TypeError,
    ) as error:
        print(f"FAILED: {error}", file=sys.stderr)
        raise SystemExit(1) from error
