#!/usr/bin/env python3
"""Isolated mutations for the frozen AISuite cutover start-state model."""

from __future__ import annotations

import argparse
import copy
import importlib.util
import json
import sys
from pathlib import Path
from typing import Any, Callable


def load_tool(path: Path) -> Any:
    specification = importlib.util.spec_from_file_location(
        "verify_aisuite_cutover", path
    )
    if specification is None or specification.loader is None:
        raise RuntimeError(f"cannot load cutover tool: {path}")
    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tool", type=Path, required=True)
    parser.add_argument("--evidence", type=Path, required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_arguments()
    tool = load_tool(args.tool.resolve())
    authority = json.loads(args.evidence.read_text(encoding="utf-8"))
    if tool.start_model_diagnostics(authority):
        raise RuntimeError("unmodified start-state authority does not pass")

    mutations: list[
        tuple[str, str, Callable[[dict[str, Any]], None]]
    ] = [
        (
            "base-commit",
            tool.HISTORY_DIAGNOSTIC,
            lambda model: model["authority"]["snodec"].__setitem__(
                "commit", "0" * 40
            ),
        ),
        (
            "base-tree",
            tool.HISTORY_DIAGNOSTIC,
            lambda model: model["authority"]["snodec"].__setitem__(
                "tree", "1" * 40
            ),
        ),
        (
            "aisuite-owner",
            tool.MIGRATION_DIAGNOSTIC,
            lambda model: model["authority"]["aisuite"].__setitem__(
                "commit", "2" * 40
            ),
        ),
        (
            "removed-file-count",
            tool.NON_CODEX_DIAGNOSTIC,
            lambda model: model["removal_inventory"].__setitem__(
                "total_count",
                model["removal_inventory"]["total_count"] - 1,
            ),
        ),
        (
            "public-header-count",
            tool.NON_CODEX_DIAGNOSTIC,
            lambda model: model["legacy_surface"].__setitem__(
                "public_header_count", 33
            ),
        ),
        (
            "snodec-component-test-count",
            tool.NON_CODEX_DIAGNOSTIC,
            lambda model: model["ctest"].__setitem__(
                "snodec_codex_hierarchy_tests", 131
            ),
        ),
        (
            "preserved-test-count",
            tool.NON_CODEX_DIAGNOSTIC,
            lambda model: model["ctest"].__setitem__(
                "non_codex_preserved", 167
            ),
        ),
    ]

    for mutation_name, expected_code, mutate in mutations:
        planted = copy.deepcopy(authority)
        before = tool.canonical_json_bytes(planted)
        mutate(planted)
        after = tool.canonical_json_bytes(planted)
        if before == after:
            raise RuntimeError(
                f"{mutation_name}: mutation did not change its input"
            )
        diagnostics = tool.start_model_diagnostics(planted)
        if diagnostics != [expected_code]:
            raise RuntimeError(
                f"{mutation_name}: expected only {expected_code}, "
                f"observed {diagnostics}"
            )
        if tool.start_model_diagnostics(authority):
            raise RuntimeError(
                f"{mutation_name}: unmodified authority stopped passing"
            )

    print(
        "AISuite cutover start-state mutations passed: "
        f"{len(mutations)} isolated diagnostics"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
