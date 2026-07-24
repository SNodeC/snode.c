#!/usr/bin/env python3
"""Shared deterministic validation primitives for frozen Codex A1 slices.

This module owns mechanics that are independent of a particular slice:
exact protocol keys, JSON rendering and hashing, intrinsic diagnostics,
generic dependency traversal, monotonic registry/fixture transitions, and
dependency-ordered progress. Slice-specific identity sets, closure policy,
type ownership, and report assembly remain in their reviewed slice tools.
"""

from __future__ import annotations

import hashlib
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Iterable, Mapping, Sequence, TypeVar

import app_server_surface as surface


Node = TypeVar("Node")


class AuditError(RuntimeError):
    """A frozen A1 audit invariant was violated."""

    def __init__(
        self,
        message: str,
        code: str = "AuditInvariant",
        codes: Sequence[str] | None = None,
    ) -> None:
        super().__init__(message)
        self.code = code
        self.codes = tuple(codes) if codes is not None else (code,)


@dataclass(frozen=True, order=True)
class AuditDiagnostic:
    """Stable intrinsic diagnostic emitted by an A1 audit validator."""

    code: str
    location: str
    message: str


@dataclass(frozen=True, order=True)
class Key:
    """Exact canonical ProtocolSurfaceKey."""

    category: str
    domain: str
    field: str
    name: str

    @classmethod
    def from_row(cls, row: Mapping[str, Any]) -> "Key":
        category, domain, field, name = surface.surface_key(dict(row))
        return cls(category, domain, field, name)

    def object(self) -> dict[str, str]:
        return {
            "category": self.category,
            "domain": self.domain,
            "discriminator_field": self.field,
            "name": self.name,
        }

    def compact(self) -> str:
        return f"{self.category}:{self.domain}:{self.field}:{self.name}"


def fail(message: str, code: str = "AuditInvariant") -> None:
    raise AuditError(message, code)


def require(
    condition: bool,
    message: str,
    code: str = "AuditInvariant",
) -> None:
    if not condition:
        fail(message, code)


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise AuditError(f"unable to load {path}: {error}") from error
    if not isinstance(value, dict):
        fail(f"expected an object-valued JSON document: {path}")
    return value


def canonical_json(value: Any) -> str:
    return json.dumps(value, indent=2, sort_keys=True, ensure_ascii=False) + "\n"


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def sha256_json(value: Any) -> str:
    return hashlib.sha256(
        json.dumps(
            value,
            sort_keys=True,
            separators=(",", ":"),
            ensure_ascii=False,
        ).encode("utf-8")
    ).hexdigest()


def historical_source_records(
    paths: Mapping[str, Path],
    repo_root: Path,
    *,
    frozen_hashes: Mapping[str, str],
    mutable_names: Iterable[str],
    source_set_error: Callable[[], Exception],
    immutable_source_error: Callable[[str], Exception],
    resolve_paths: bool,
) -> dict[str, dict[str, str]]:
    """Project a reviewed source snapshot while retaining live hash guards."""

    if set(paths) != set(frozen_hashes):
        raise source_set_error()
    mutable = frozenset(mutable_names)
    result: dict[str, dict[str, str]] = {}
    for name, path in sorted(paths.items()):
        reported_path = path.resolve() if resolve_paths else path
        relative = reported_path.relative_to(repo_root).as_posix()
        frozen_hash = frozen_hashes[name]
        if name not in mutable and sha256_file(path) != frozen_hash:
            raise immutable_source_error(relative)
        result[name] = {"path": relative, "sha256": frozen_hash}
    return result


def transitive_closure(
    starts: Iterable[Node],
    edges: Mapping[Node, set[Node]],
    *,
    unknown_node_error: Callable[[Node], Exception],
) -> frozenset[Node]:
    """Return a deterministic dependency closure or reject an unknown node."""

    reached: set[Node] = set()
    pending = list(starts)
    while pending:
        current = pending.pop()
        if current in reached:
            continue
        if current not in edges:
            raise unknown_node_error(current)
        reached.add(current)
        pending.extend(sorted(edges[current] - reached))
    return frozenset(reached)


def find_dependency_cycle(
    nodes: Iterable[Node],
    edges: Mapping[Node, set[Node]],
) -> list[Node] | None:
    """Return one deterministic cycle restricted to ``nodes``, if present."""

    allowed = set(nodes)
    visited: set[Node] = set()
    active: set[Node] = set()
    stack: list[Node] = []

    def visit(node: Node) -> list[Node] | None:
        visited.add(node)
        active.add(node)
        stack.append(node)
        for dependency in sorted(edges[node] & allowed):
            if dependency not in visited:
                cycle = visit(dependency)
                if cycle:
                    return cycle
            elif dependency in active:
                return stack[stack.index(dependency):] + [dependency]
        stack.pop()
        active.remove(node)
        return None

    for node in sorted(allowed):
        if node not in visited:
            cycle = visit(node)
            if cycle:
                return cycle
    return None


def has_dependency_cycle(graph: Mapping[Node, set[Node]]) -> bool:
    """Return whether a dependency graph contains any cycle."""

    active: set[Node] = set()
    visited: set[Node] = set()

    def visit(node: Node) -> bool:
        if node in active:
            return True
        if node in visited:
            return False
        active.add(node)
        for dependency in sorted(graph.get(node, set())):
            if visit(dependency):
                return True
        active.remove(node)
        visited.add(node)
        return False

    return any(visit(node) for node in sorted(graph))


def definition_reference_parts(reference: str) -> tuple[str, str] | None:
    """Decode a combined-aggregate definition reference."""

    match = re.fullmatch(r"#/definitions/(?:(v2)/)?([^/]+)", reference)
    if match is None:
        return None
    return (
        "v2" if match.group(1) else "legacy",
        match.group(2).replace("~1", "/").replace("~0", "~"),
    )


def definition_path(namespace: str, name: str) -> str:
    """Return the canonical combined-aggregate pointer for a definition."""

    require(
        namespace in {"legacy", "v2"},
        f"unsupported definition namespace: {namespace}",
        "UnresolvedSchemaReference",
    )
    prefix = "#/definitions/v2" if namespace == "v2" else "#/definitions"
    escaped = name.replace("~", "~0").replace("/", "~1")
    return f"{prefix}/{escaped}"


def closure_fixture_obligations(
    path_rows: Sequence[Mapping[str, Any]],
    open_enums: Sequence[Mapping[str, Any]] = (),
    registered_unions: Sequence[Mapping[str, Any]] = (),
) -> dict[str, Any]:
    """Derive fixture and mutation obligations from reviewed closure rows."""

    property_rows = [
        row
        for row in path_rows
        if row.get("schema_node_kind") == "property"
    ]
    required_paths = sorted(
        str(row["schema_path"])
        for row in property_rows
        if row.get("required") is True
    )
    optional_paths = sorted(
        str(row["schema_path"])
        for row in property_rows
        if row.get("required") is False
    )
    nullable_rows = [
        row
        for row in property_rows
        if row.get("nullable") is True
    ]
    integer_rows = [
        row
        for row in path_rows
        if str(row.get("schema_value_kind"))
        in {"integer", "nullable_integer"}
    ]
    integer_paths = sorted(
        str(row["schema_path"]) for row in integer_rows
    )
    unsigned_integer_paths = sorted(
        str(row["schema_path"])
        for row in integer_rows
        if (
            isinstance(row.get("schema_constraints"), Mapping)
            and (
                str(row["schema_constraints"].get("format", "")).startswith(
                    "u"
                )
                or row["schema_constraints"].get("minimum") == 0
                or row["schema_constraints"].get("minimum") == 0.0
            )
        )
    )
    positive = {
        "required_properties_present_schema_paths": required_paths,
        "optional_properties_present_schema_paths": optional_paths,
        "optional_properties_omitted_schema_paths": optional_paths,
        "nullable_schema_valid_states": [
            {
                "schema_path": str(row["schema_path"]),
                "schema_valid_states": (
                    ["null", "value"]
                    if row.get("required") is True
                    else ["omitted", "null", "value"]
                ),
            }
            for row in nullable_rows
        ],
        "defaulted_schema_paths": [
            {
                "schema_path": str(row["schema_path"]),
                "schema_default": row["schema_default"],
                "required_cases": [
                    "omitted-applies-reviewed-default",
                    "explicit-default-value",
                    "explicit-nondefault-value",
                ],
            }
            for row in property_rows
            if "schema_default" in row
        ],
        "integer_boundary_schema_paths": [
            {
                "schema_path": str(row["schema_path"]),
                "cpp_type": str(row["cpp_base_type"]),
                "schema_constraints": dict(
                    row.get("schema_constraints", {})
                ),
                "required_cases": [
                    "minimum-schema-or-cpp-boundary",
                    "maximum-schema-or-cpp-boundary",
                ],
            }
            for row in integer_rows
        ],
        "protocol_defined_opaque_json_schema_paths": [
            {
                "schema_path": str(row["schema_path"]),
                "required_json_value_kinds": [
                    "null",
                    "boolean",
                    "integer",
                    "number",
                    "string",
                    "array",
                    "object",
                ],
            }
            for row in path_rows
            if row.get("disposition") == "ProtocolDefinedOpaqueJson"
        ],
        "collection_empty_nonempty_schema_paths": [
            {
                "schema_path": str(row["schema_path"]),
                "cpp_type": str(row["cpp_base_type"]),
                "required_cases": ["empty", "nonempty"],
            }
            for row in property_rows
            if str(row.get("cpp_base_type", "")).startswith(
                ("std::vector<", "std::map<")
            )
        ],
        "open_string_enum_known_values": [
            dict(row) for row in open_enums
        ],
        "registered_tagged_union_alternatives": [
            dict(row) for row in registered_unions
        ],
    }
    negative = {
        "required_field_removal_schema_paths": required_paths,
        "wrong_type_schema_paths": sorted(
            str(row["schema_path"])
            for row in path_rows
            if row.get("disposition")
            != "ProtocolDefinedOpaqueJson"
        ),
        "optional_omission_schema_paths": optional_paths,
        "nullable_omitted_null_value_schema_paths": sorted(
            str(row["schema_path"]) for row in nullable_rows
        ),
        "numeric_mutations": {
            "integer_schema_paths": integer_paths,
            "wrong_sign_schema_paths": unsigned_integer_paths,
            "out_of_range_schema_paths": integer_paths,
            "fractional_integer_schema_paths": integer_paths,
        },
    }
    return {"positive": positive, "negative": negative}


def property_presence_model(
    required: bool,
    nullable: bool,
    defaulted: bool = False,
) -> str:
    """Map JSON-Schema presence/null/default facts to the A1 C++ model."""

    if required:
        return "RequiredNullable" if nullable else "RequiredValue"
    if nullable:
        return "OptionalNullable"
    return "DefaultedValue" if defaulted else "OptionalValue"


def wrap_property_cpp_type(
    base_type: str,
    presence_model: str,
) -> str:
    """Apply the reviewed OptionalNullable/std::optional C++ wrapper."""

    if presence_model == "OptionalNullable":
        return f"typed::OptionalNullable<{base_type}>"
    if presence_model in {"OptionalValue", "RequiredNullable"}:
        return f"std::optional<{base_type}>"
    require(
        presence_model in {"DefaultedValue", "RequiredValue"},
        f"unknown property presence model: {presence_model}",
        "OptionalNullableMappingMismatch",
    )
    return base_type


def allows_null(schema: Any) -> bool:
    """Return whether a Draft-07 schema accepts JSON null."""

    if not isinstance(schema, Mapping):
        return schema is True
    annotation_keywords = {
        "$comment",
        "$id",
        "$schema",
        "default",
        "description",
        "examples",
        "readOnly",
        "title",
        "writeOnly",
    }
    if set(schema).issubset(annotation_keywords):
        return True
    schema_type = schema.get("type")
    if schema_type == "null":
        return True
    if isinstance(schema_type, list) and "null" in schema_type:
        return True
    return any(
        allows_null(branch)
        for keyword in ("oneOf", "anyOf")
        for branch in schema.get(keyword, [])
    )


def non_null_schema(schema: Any) -> Any:
    """Strip only a schema-level null alternative."""

    if not isinstance(schema, Mapping):
        return schema
    schema_type = schema.get("type")
    if isinstance(schema_type, list) and "null" in schema_type:
        non_null_types = [
            value for value in schema_type if value != "null"
        ]
        result = dict(schema)
        result["type"] = (
            non_null_types[0]
            if len(non_null_types) == 1
            else non_null_types
        )
        return result
    for keyword in ("oneOf", "anyOf"):
        branches = schema.get(keyword)
        if not isinstance(branches, list):
            continue
        non_null = [
            branch
            for branch in branches
            if not (
                isinstance(branch, Mapping)
                and branch.get("type") == "null"
                and set(branch) == {"type"}
            )
        ]
        if len(non_null) != len(branches):
            if len(non_null) == 1:
                return non_null[0]
            result = dict(schema)
            result[keyword] = non_null
            return result
    return schema


@dataclass(frozen=True)
class CppTypePolicy:
    """Slice policy for the shared JSON-Schema-to-C++ type mechanics."""

    semantic_type: Callable[
        [Any, str, str, str], str | None
    ]
    reference_type: Callable[[str, str], str]
    inline_type: Callable[[str, str, str], str]
    special_string_union_type: Callable[
        [Mapping[str, Any], str, str], str | None
    ]
    map_key_type: Callable[[str], str]
    opaque_paths: frozenset[str]
    allow_unreviewed_true: bool
    allow_empty_schema_json: bool
    allow_annotation_only_json: bool
    nullable_array_elements: bool
    missing_mapping_code: str = "MissingClosureDisposition"
    opaque_misuse_code: str = "OpaqueJsonMisuse"


def schema_cpp_base_type(
    schema: Any,
    path: str,
    owner: str,
    member: str,
    policy: CppTypePolicy,
) -> str:
    """Map one reviewed schema value to its non-presence C++ type."""

    semantic = policy.semantic_type(schema, path, owner, member)
    if semantic is not None:
        return semantic
    if path in policy.opaque_paths:
        return "Json"
    if schema is True:
        if policy.allow_unreviewed_true:
            return "Json"
        fail(
            f"unreviewed unconstrained JSON path: {path}",
            policy.opaque_misuse_code,
        )
    require(
        isinstance(schema, Mapping),
        f"schema path is not object-valued: {path}",
        policy.missing_mapping_code,
    )
    schema = non_null_schema(schema)
    require(
        isinstance(schema, Mapping),
        f"nullable schema is malformed: {path}",
        policy.missing_mapping_code,
    )

    reference = schema.get("$ref")
    if isinstance(reference, str):
        return policy.reference_type(reference, path)

    all_of = schema.get("allOf")
    if isinstance(all_of, list):
        require(
            len(all_of) == 1,
            f"schema composition needs an explicit C++ mapping: {path}",
            policy.missing_mapping_code,
        )
        return schema_cpp_base_type(
            all_of[0],
            f"{path}/allOf/0",
            owner,
            member,
            policy,
        )

    special_union = policy.special_string_union_type(
        schema, owner, member
    )
    if special_union is not None:
        return special_union
    for keyword in ("oneOf", "anyOf"):
        branches = schema.get(keyword)
        if not isinstance(branches, list):
            continue
        branch_types: list[str] = []
        for index, branch in enumerate(branches):
            branch_type = schema_cpp_base_type(
                branch,
                f"{path}/{keyword}/{index}",
                owner,
                member,
                policy,
            )
            if branch_type not in branch_types:
                branch_types.append(branch_type)
        require(
            bool(branch_types),
            f"schema union has no C++ alternatives: {path}",
            policy.missing_mapping_code,
        )
        return (
            branch_types[0]
            if len(branch_types) == 1
            else "std::variant<" + ", ".join(branch_types) + ">"
        )

    enum_values = schema.get("enum")
    if isinstance(enum_values, list):
        require(
            enum_values
            and all(isinstance(value, str) for value in enum_values),
            f"schema enum has malformed values: {path}",
            policy.missing_mapping_code,
        )
        return (
            f'detail::ExactDiscriminatorLiteral<"{enum_values[0]}">'
            if len(enum_values) == 1
            else policy.inline_type(owner, member, "Value")
        )

    schema_type = schema.get("type")
    if schema_type == "array":
        require(
            "items" in schema,
            f"array schema has no item mapping: {path}",
            policy.missing_mapping_code,
        )
        item_schema = schema["items"]
        item_type = schema_cpp_base_type(
            item_schema,
            f"{path}/items",
            owner,
            member + "[]",
            policy,
        )
        if (
            policy.nullable_array_elements
            and allows_null(item_schema)
            and item_type != "Json"
        ):
            item_type = f"std::optional<{item_type}>"
        return f"std::vector<{item_type}>"
    if schema_type == "object" or "properties" in schema:
        additional = schema.get("additionalProperties")
        if not schema.get("properties") and (
            additional is True or isinstance(additional, Mapping)
        ):
            value_type = schema_cpp_base_type(
                additional,
                f"{path}/additionalProperties",
                owner,
                member + "[*]",
                policy,
            )
            return (
                f"std::map<{policy.map_key_type(path)}, "
                f"{value_type}>"
            )
        return policy.inline_type(owner, member, "")
    if schema_type == "string":
        return "std::string"
    if schema_type == "boolean":
        return "bool"
    if schema_type == "integer":
        integer_format = schema.get("format")
        if integer_format == "int32":
            return "std::int32_t"
        if integer_format == "int64":
            return "std::int64_t"
        if integer_format in {"uint8", "uint16"}:
            return "std::uint16_t"
        if integer_format == "uint32":
            return "std::uint32_t"
        if integer_format in {"uint", "uint64"}:
            return "std::uint64_t"
        minimum = schema.get("minimum")
        return (
            "std::uint64_t"
            if isinstance(minimum, (int, float)) and minimum >= 0
            else "std::int64_t"
        )
    if schema_type == "number":
        return "double"
    if schema_type == "null":
        return "std::monostate"
    if isinstance(schema_type, list):
        types = [
            schema_cpp_base_type(
                {"type": value},
                path,
                owner,
                member,
                policy,
            )
            for value in schema_type
        ]
        return "std::variant<" + ", ".join(types) + ">"
    annotation_keywords = {
        "$comment",
        "$id",
        "$schema",
        "default",
        "description",
        "examples",
        "readOnly",
        "title",
        "writeOnly",
    }
    if (
        policy.allow_empty_schema_json
        and not schema
    ) or (
        policy.allow_annotation_only_json
        and bool(schema)
        and set(schema).issubset(annotation_keywords)
    ):
        return "Json"
    if not schema or set(schema).issubset(annotation_keywords):
        fail(
            f"unreviewed arbitrary JSON mapping: {path}",
            policy.opaque_misuse_code,
        )
    fail(
        f"schema path has no exact C++ mapping: {path}",
        policy.missing_mapping_code,
    )


def indexed(
    records: Iterable[Mapping[str, Any]],
    description: str,
) -> dict[Key, dict[str, Any]]:
    result: dict[Key, dict[str, Any]] = {}
    for raw in records:
        row = dict(raw)
        key = Key.from_row(row)
        if key in result:
            fail(f"duplicate {description} identity: {key.compact()}")
        result[key] = row
    return result


def records(
    document: Mapping[str, Any],
    names: Sequence[str],
    description: str,
) -> list[dict[str, Any]]:
    for name in names:
        value = document.get(name)
        if isinstance(value, list) and all(
            isinstance(row, dict) for row in value
        ):
            return [dict(row) for row in value]
    fail(
        f"{description} lacks one of the expected arrays: "
        f"{', '.join(names)}"
    )


@dataclass
class SurfaceEvidenceInputs:
    """Shared parsed/indexed stable-surface evidence."""

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


def validate_input_versions(
    documents: Iterable[Mapping[str, Any]],
    allowed_versions: frozenset[str],
) -> None:
    for document in documents:
        version = str(document.get("codex_version", ""))
        require(
            version in allowed_versions,
            f"unexpected Codex evidence version: {version!r}",
            "EvidenceVersionMismatch",
        )


def load_surface_evidence_inputs(
    *,
    manifest_path: Path,
    assignments_path: Path,
    reachability_path: Path,
    contracts_path: Path,
    completeness_path: Path,
    fixture_coverage_path: Path,
    registry_path: Path,
    allowed_versions: frozenset[str],
) -> SurfaceEvidenceInputs:
    """Load and cross-check the common evidence graph for any A1 slice."""

    manifest = load_json(manifest_path)
    assignments_document = load_json(assignments_path)
    reachability_document = load_json(reachability_path)
    contracts_document = load_json(contracts_path)
    completeness_document = load_json(completeness_path)
    fixture_document = load_json(fixture_coverage_path)
    validate_input_versions(
        (
            manifest,
            assignments_document,
            reachability_document,
            contracts_document,
            completeness_document,
            fixture_document,
        ),
        allowed_versions,
    )

    registry_rows = surface.parse_registry_data(registry_path)
    manifest_rows = indexed(
        records(manifest, ("entries",), "surface manifest"),
        "manifest",
    )
    assignments = indexed(
        records(
            assignments_document,
            ("assignments",),
            "module/slice assignment",
        ),
        "assignment",
    )
    reachability = indexed(
        records(
            reachability_document,
            ("records",),
            "nested reachability",
        ),
        "reachability",
    )
    contracts = indexed(
        records(
            contracts_document,
            ("contracts",),
            "operation contracts",
        ),
        "operation contract",
    )
    completeness = indexed(
        records(
            completeness_document,
            ("records",),
            "schema completeness",
        ),
        "schema completeness",
    )
    fixture_coverage = indexed(
        records(
            fixture_document,
            ("identity_coverage",),
            "fixture coverage",
        ),
        "fixture coverage",
    )
    registry = indexed(registry_rows, "canonical registry")

    expected_keys = set(manifest_rows)
    for name, values, code in (
        ("registry", registry, "LiveIdentityMismatch"),
        ("assignment", assignments, "AssignmentMismatch"),
        (
            "schema completeness",
            completeness,
            "LiveIdentityMismatch",
        ),
        (
            "fixture coverage",
            fixture_coverage,
            "LiveIdentityMismatch",
        ),
    ):
        require(
            set(values) == expected_keys,
            f"{name}/manifest exact-key mismatch",
            code,
        )

    association_errors = surface.association_diagnostics(
        manifest, contracts_document, registry_rows
    )
    if association_errors:
        codes = tuple(
            sorted(str(error["code"]) for error in association_errors)
        )
        raise AuditError(
            f"operation-contract mismatch: {association_errors}",
            codes[0],
            codes,
        )
    assignment_errors = surface.assignment_reachability_diagnostics(
        manifest, assignments_document, reachability_document
    )
    if assignment_errors:
        codes = tuple(
            sorted(str(error["code"]) for error in assignment_errors)
        )
        raise AuditError(
            f"assignment/reachability mismatch: {assignment_errors}",
            codes[0],
            codes,
        )

    for key in sorted(expected_keys):
        fixture_facts = completeness[key].get(
            "schema_fixture_facts"
        )
        require(
            fixture_facts
            == fixture_coverage[key].get("completeness_evidence"),
            f"schema-completeness/fixture mismatch: {key.compact()}",
            "CompletenessEvidenceMismatch",
        )
        assignment = assignments[key]
        row = registry[key]
        require(
            row.get("typed_module") == assignment.get("module")
            and row.get("a1_slice") == assignment.get("slice")
            and row.get("stability") == assignment.get("stability"),
            f"assignment/registry projection mismatch: {key.compact()}",
            "AssignmentMismatch",
        )
        registry_facts = row.get("schema_completeness")
        require(
            isinstance(fixture_facts, Mapping)
            and isinstance(registry_facts, Mapping)
            and all(
                registry_facts.get(field) == value
                for field, value in fixture_facts.items()
            ),
            f"registry/schema-completeness facts disagree: "
            f"{key.compact()}",
            "CompletenessEvidenceMismatch",
        )
        derived_status = surface.derived_schema_status(
            str(row.get("typed_status")),
            str(row.get("a1_slice")),
            dict(registry_facts),
        )
        require(
            row.get("typed_schema_status") == derived_status,
            f"registry schema status disagrees with completeness facts: "
            f"{key.compact()}",
            "FalseCompleteness",
        )

    return SurfaceEvidenceInputs(
        manifest=manifest,
        assignments_document=assignments_document,
        reachability_document=reachability_document,
        contracts_document=contracts_document,
        completeness_document=completeness_document,
        fixture_document=fixture_document,
        registry_rows=registry_rows,
        manifest_rows=manifest_rows,
        assignments=assignments,
        reachability=reachability,
        contracts=contracts,
        completeness=completeness,
        fixture_coverage=fixture_coverage,
        registry=registry,
    )


@dataclass(frozen=True)
class RegistryTransitionPolicy:
    """Slice policy for a monotonic canonical-registry transition."""

    scope: str
    mutable_layer_fields: tuple[str, ...] = (
        "backend_status",
        "canonical_state_status",
    )
    require_exact_completeness_shape: bool = True
    enforce_false_completeness: bool = True
    invalid_runtime_targets: frozenset[Any] = frozenset(
        ("", "std::monostate{}")
    )


def registry_transition_diagnostics(
    key: Key,
    frozen: Mapping[str, Any],
    live: Mapping[str, Any],
    policy: RegistryTransitionPolicy,
) -> list[AuditDiagnostic]:
    """Validate one monotonic transition from an immutable slice boundary."""

    diagnostics: list[AuditDiagnostic] = []

    def add(code: str, message: str) -> None:
        diagnostics.append(
            AuditDiagnostic(code, key.compact(), message)
        )

    schema_order = {"NotImplemented": 0, "Partial": 1, "Complete": 2}
    implementation_order = {"NotImplemented": 0, "Implemented": 1}
    layer_order = {"NotImplemented": 0, "Implemented": 1}
    progress_fields = {
        "runtime_disposition",
        "runtime_target",
        "schema_completeness",
        "typed_schema_status",
        "typed_status",
        *policy.mutable_layer_fields,
    }
    frozen_static = {
        field: value
        for field, value in frozen.items()
        if field not in progress_fields
    }
    live_static = {
        field: value
        for field, value in live.items()
        if field not in progress_fields
    }
    if frozen_static != live_static:
        add(
            "StaticIdentityDrift",
            f"static {policy.scope} registry fields changed",
        )

    frozen_implementation = frozen.get("typed_status")
    live_implementation = live.get("typed_status")
    implementation_advanced = (
        frozen_implementation in implementation_order
        and live_implementation in implementation_order
        and implementation_order[str(live_implementation)]
        > implementation_order[str(frozen_implementation)]
    )
    if (
        frozen_implementation not in implementation_order
        or live_implementation not in implementation_order
        or implementation_order[str(live_implementation)]
        < implementation_order[str(frozen_implementation)]
    ):
        add(
            "ImplementationDemotion",
            f"{policy.scope} typed implementation status regressed",
        )

    frozen_schema = frozen.get("typed_schema_status")
    live_schema = live.get("typed_schema_status")
    if (
        frozen_schema not in schema_order
        or live_schema not in schema_order
        or schema_order[str(live_schema)] < schema_order[str(frozen_schema)]
    ):
        add(
            "SchemaStatusDemotion",
            f"{policy.scope} typed schema status regressed",
        )

    for field in policy.mutable_layer_fields:
        frozen_layer = frozen.get(field)
        live_layer = live.get(field)
        if frozen_layer == "NotApplicable":
            invalid = live_layer != "NotApplicable"
        else:
            invalid = (
                frozen_layer not in layer_order
                or live_layer not in layer_order
                or layer_order[str(live_layer)]
                < layer_order[str(frozen_layer)]
            )
        if invalid:
            add(
                "LayerDispositionDemotion",
                f"{policy.scope} {field} regressed",
            )

    runtime_changed = (
        live.get("runtime_disposition")
        != frozen.get("runtime_disposition")
        or live.get("runtime_target") != frozen.get("runtime_target")
    )
    if runtime_changed and not implementation_advanced:
        add(
            "StaticIdentityDrift",
            f"{policy.scope} runtime target/disposition changed without "
            "implementation advancement",
        )
    elif implementation_advanced and (
        live.get("runtime_disposition") != "Typed"
        or live.get("runtime_target") in policy.invalid_runtime_targets
    ):
        add(
            "InvalidRuntimePromotion",
            f"implemented {policy.scope} identity lacks a nonempty Typed "
            "runtime target",
        )

    frozen_facts = frozen.get("schema_completeness")
    live_facts = live.get("schema_completeness")
    if not isinstance(frozen_facts, Mapping) or not isinstance(
        live_facts, Mapping
    ):
        add(
            "CompletenessDemotion",
            f"{policy.scope} schema-completeness projection is malformed",
        )
    elif (
        policy.require_exact_completeness_shape
        and set(frozen_facts) != set(live_facts)
    ):
        add(
            "CompletenessShapeDrift",
            f"{policy.scope} schema-completeness fact set changed",
        )
    else:
        for field, frozen_value in frozen_facts.items():
            live_value = live_facts.get(field)
            if not isinstance(frozen_value, bool) or not isinstance(
                live_value, bool
            ):
                add(
                    (
                        "CompletenessShapeDrift"
                        if policy.require_exact_completeness_shape
                        else "CompletenessDemotion"
                    ),
                    f"{policy.scope} schema-completeness fact {field} "
                    "is not boolean",
                )
                break
            if frozen_value and not live_value:
                add(
                    "CompletenessDemotion",
                    f"{policy.scope} schema-completeness fact {field} "
                    "regressed",
                )
                break
        if (
            policy.enforce_false_completeness
            and live_schema == "Complete"
            and any(value is not True for value in live_facts.values())
        ):
            add(
                "FalseCompleteness",
                f"Complete {policy.scope} identity has an unsatisfied "
                "completeness fact",
            )

    return sorted(set(diagnostics))


@dataclass(frozen=True)
class FixtureTransitionPolicy:
    """Fixture evidence fields that must advance monotonically."""

    count_fields: tuple[str, ...] = ()


def fixture_transition_diagnostics(
    key: Key,
    frozen_completeness: Mapping[str, Any],
    live_completeness: Mapping[str, Any],
    frozen_fixture: Mapping[str, Any],
    live_fixture: Mapping[str, Any],
    policy: FixtureTransitionPolicy = FixtureTransitionPolicy(),
) -> list[AuditDiagnostic]:
    """Validate monotonic schema-completeness and fixture evidence."""

    diagnostics: list[AuditDiagnostic] = []

    def add(message: str) -> None:
        diagnostics.append(
            AuditDiagnostic(
                "FixtureCoverageDemotion",
                key.compact(),
                message,
            )
        )

    frozen_fixture_ids = set(frozen_completeness.get("fixture_ids", []))
    live_fixture_ids = set(live_completeness.get("fixture_ids", []))
    if not frozen_fixture_ids.issubset(live_fixture_ids):
        add("schema-completeness fixture IDs regressed")

    frozen_coverage_ids = set(frozen_fixture.get("fixture_ids", []))
    live_coverage_ids = set(live_fixture.get("fixture_ids", []))
    if not frozen_coverage_ids.issubset(live_coverage_ids):
        add("fixture-coverage IDs regressed")

    for field in policy.count_fields:
        frozen_count = frozen_fixture.get(field, 0)
        live_count = live_fixture.get(field, 0)
        if (
            not isinstance(frozen_count, int)
            or not isinstance(live_count, int)
            or live_count < frozen_count
        ):
            add(f"fixture metric {field} regressed")
            break

    return sorted(set(diagnostics))


def staged_progress_diagnostics(
    keys: Sequence[Key],
    registry: Mapping[Key, Mapping[str, Any]],
    *,
    batch_for_key: Callable[[Key], str],
    identity_has_advanced: Callable[[Key], bool],
    batch_order: Sequence[str] = ("B2", "B3", "B4", "B5"),
    incomplete_message: str = (
        "{batch} advanced before all earlier batch identities were Complete"
    ),
) -> list[AuditDiagnostic]:
    """Reject progress that skips an incomplete dependency batch."""

    batch_keys: dict[str, list[Key]] = {
        batch: [] for batch in batch_order
    }
    for key in keys:
        batch_keys.setdefault(batch_for_key(key), []).append(key)

    diagnostics: list[AuditDiagnostic] = []
    for index, batch in enumerate(batch_order):
        if not any(
            identity_has_advanced(key)
            for key in batch_keys.get(batch, [])
        ):
            continue
        incomplete_earlier = [
            key
            for earlier in batch_order[:index]
            for key in batch_keys.get(earlier, [])
            if registry[key].get("typed_schema_status") != "Complete"
        ]
        if incomplete_earlier:
            diagnostics.append(
                AuditDiagnostic(
                    "NonContiguousBatchProgress",
                    batch,
                    incomplete_message.format(batch=batch),
                )
            )
    return sorted(set(diagnostics))


def validate_diagnostics(
    diagnostics: Iterable[AuditDiagnostic],
) -> None:
    """Raise one stable error carrying the exact intrinsic-code multiset."""

    ordered = sorted(set(diagnostics))
    if not ordered:
        return
    codes = tuple(diagnostic.code for diagnostic in ordered)
    message = "; ".join(
        f"{diagnostic.code} at {diagnostic.location}: "
        f"{diagnostic.message}"
        for diagnostic in ordered
    )
    raise AuditError(message, ordered[0].code, codes)


def write_or_check(
    path: Path,
    document: Mapping[str, Any],
    check: bool,
    *,
    artifact_label: str,
) -> None:
    """Write deterministic JSON or reject a stale checked-in artifact."""

    rendered = canonical_json(document)
    if check:
        try:
            committed = path.read_text(encoding="utf-8")
        except OSError as error:
            raise AuditError(
                f"missing {artifact_label} {path}: {error}",
                "StaleGeneratedAudit",
            ) from error
        if committed != rendered:
            fail(
                f"{artifact_label} is stale: {path}",
                "StaleGeneratedAudit",
            )
    else:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(rendered, encoding="utf-8")
