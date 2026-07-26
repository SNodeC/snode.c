#!/usr/bin/env python3
"""Deterministic, state-carrying traversal of JSON Schema value paths.

The walker is intentionally independent of fixture synthesis and slice policy.
Callers supply the state transition that maps a schema edge to their reviewed
C++ owner/member or other slice-specific metadata.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Callable, Generic, Iterator, Mapping, TypeVar


PROPERTY = "property"
MAP_VALUE = "map_value"
ARRAY_ELEMENT = "array_element"
ALLOF_BRANCH = "allOf_branch"
ANYOF_BRANCH = "anyOf_branch"
ONEOF_BRANCH = "oneOf_branch"

VALUE_PATH_KINDS = frozenset({PROPERTY, MAP_VALUE, ARRAY_ELEMENT})
COMBINATOR_BRANCH_KINDS = frozenset(
    {ALLOF_BRANCH, ANYOF_BRANCH, ONEOF_BRANCH}
)
DEFAULT_VALUE_KEYWORD_ORDER = ("additionalProperties", "items")
DEFAULT_COMBINATOR_KEYWORD_ORDER = ("allOf", "anyOf", "oneOf")


State = TypeVar("State")


@dataclass(frozen=True)
class SchemaPathVisit(Generic[State]):
    """One deterministic schema edge and its caller-derived child state."""

    path: str
    kind: str
    schema: Any
    required: bool | None
    state: State


SchemaPathStateTransition = Callable[
    [State, str, str | int | None, str, Any, bool | None],
    State,
]


class SchemaPathWalkError(ValueError):
    """The supplied schema has a malformed traversal-critical keyword."""


def _pointer_token(value: str) -> str:
    return value.replace("~", "~0").replace("/", "~1")


def _pointer_child(path: str, token: str | int) -> str:
    return f"{path}/{_pointer_token(str(token))}"


def _required_property_names(
    schema: Mapping[str, Any], path: str
) -> frozenset[str]:
    required = schema.get("required", [])
    if not isinstance(required, list) or not all(
        isinstance(name, str) for name in required
    ):
        raise SchemaPathWalkError(
            f"required must be an array of strings at {path}"
        )
    return frozenset(required)


def walk_schema_paths(
    schema: Any,
    *,
    path: str,
    state: State,
    transition: SchemaPathStateTransition[State] | None = None,
    skip_references: bool = False,
    value_keyword_order: tuple[str, ...] = DEFAULT_VALUE_KEYWORD_ORDER,
    combinator_keyword_order: tuple[
        str, ...
    ] = DEFAULT_COMBINATOR_KEYWORD_ORDER,
) -> Iterator[SchemaPathVisit[State]]:
    """Yield deterministic value paths and combinator branches.

    ``transition`` receives the parent state, edge kind, edge token, child
    path, child schema, and property-requiredness.  Its return value is stored
    on the yielded visit and carried into that child's descendants.  The token
    is the property name or branch/tuple index; it is ``None`` for a map value
    or a single-schema array item.

    Properties are sorted by name.  Tuple items and combinator branches retain
    schema order.  ``additionalProperties: false`` is an object-closing rule,
    not a map-value schema, and is therefore skipped.  When
    ``skip_references`` is true, a schema containing a string ``$ref`` remains
    a leaf; reference resolution remains the caller's responsibility.
    ``value_keyword_order`` and ``combinator_keyword_order`` let existing
    generators retain their historical byte order while sharing this walker.
    """

    if (
        len(value_keyword_order) != 2
        or set(value_keyword_order) != {"additionalProperties", "items"}
    ):
        raise SchemaPathWalkError(
            "value keyword order must contain additionalProperties and items "
            "exactly once"
        )
    if (
        len(combinator_keyword_order) != 3
        or set(combinator_keyword_order) != {"allOf", "anyOf", "oneOf"}
    ):
        raise SchemaPathWalkError(
            "combinator keyword order must contain allOf, anyOf, and oneOf "
            "exactly once"
        )

    def next_state(
        parent_state: State,
        kind: str,
        token: str | int | None,
        child_path: str,
        child_schema: Any,
        required: bool | None,
    ) -> State:
        if transition is None:
            return parent_state
        return transition(
            parent_state,
            kind,
            token,
            child_path,
            child_schema,
            required,
        )

    def child_visits(
        child_schema: Any,
        child_path: str,
        kind: str,
        token: str | int | None,
        required: bool | None,
        parent_state: State,
    ) -> Iterator[SchemaPathVisit[State]]:
        child_state = next_state(
            parent_state,
            kind,
            token,
            child_path,
            child_schema,
            required,
        )
        yield SchemaPathVisit(
            child_path,
            kind,
            child_schema,
            required,
            child_state,
        )
        yield from descend(child_schema, child_path, child_state)

    def descend(
        node: Any,
        node_path: str,
        node_state: State,
    ) -> Iterator[SchemaPathVisit[State]]:
        if isinstance(node, bool) or not isinstance(node, Mapping):
            return
        if skip_references and isinstance(node.get("$ref"), str):
            return

        required_names = _required_property_names(node, node_path)
        properties = node.get("properties")
        if isinstance(properties, Mapping):
            for raw_name, child_schema in sorted(
                properties.items(), key=lambda item: str(item[0])
            ):
                if not isinstance(raw_name, str):
                    raise SchemaPathWalkError(
                        f"property name must be a string at {node_path}"
                    )
                child_path = _pointer_child(
                    _pointer_child(node_path, "properties"), raw_name
                )
                required = raw_name in required_names
                yield from child_visits(
                    child_schema,
                    child_path,
                    PROPERTY,
                    raw_name,
                    required,
                    node_state,
                )

        for keyword in value_keyword_order:
            if keyword == "additionalProperties":
                additional = node.get(keyword)
                if additional is None or additional is False:
                    continue
                child_path = _pointer_child(node_path, keyword)
                yield from child_visits(
                    additional,
                    child_path,
                    MAP_VALUE,
                    None,
                    None,
                    node_state,
                )
                continue

            items = node.get(keyword)
            if isinstance(items, list):
                for index, child_schema in enumerate(items):
                    child_path = _pointer_child(
                        _pointer_child(node_path, keyword), index
                    )
                    yield from child_visits(
                        child_schema,
                        child_path,
                        ARRAY_ELEMENT,
                        index,
                        None,
                        node_state,
                    )
            elif isinstance(items, (Mapping, bool)):
                child_path = _pointer_child(node_path, keyword)
                yield from child_visits(
                    items,
                    child_path,
                    ARRAY_ELEMENT,
                    None,
                    None,
                    node_state,
                )

        branch_kind = {
            "allOf": ALLOF_BRANCH,
            "anyOf": ANYOF_BRANCH,
            "oneOf": ONEOF_BRANCH,
        }
        for keyword in combinator_keyword_order:
            branches = node.get(keyword)
            if not isinstance(branches, list):
                continue
            for index, child_schema in enumerate(branches):
                child_path = _pointer_child(
                    _pointer_child(node_path, keyword), index
                )
                yield from child_visits(
                    child_schema,
                    child_path,
                    branch_kind[keyword],
                    index,
                    None,
                    node_state,
                )

    yield from descend(schema, path, state)
