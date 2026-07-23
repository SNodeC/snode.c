#!/usr/bin/env python3

# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#
# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT

from __future__ import annotations

import argparse
import pathlib
import re


DIRECT_ACCESSOR = re.compile(r"(?:\.|->)\s*(threads|turns|events|requests)\s*\(")


def mask_comments_and_literals(source: str) -> str:
    masked = list(source)
    index = 0
    state = "code"

    while index < len(source):
        character = source[index]
        following = source[index + 1] if index + 1 < len(source) else ""
        previous = source[index - 1] if index else ""

        if state == "code":
            if character == "/" and following == "/":
                masked[index] = masked[index + 1] = " "
                index += 2
                state = "line-comment"
                continue
            if character == "/" and following == "*":
                masked[index] = masked[index + 1] = " "
                index += 2
                state = "block-comment"
                continue
            if character == '"':
                masked[index] = " "
                index += 1
                state = "string"
                continue
            if character == "'" and not (previous.isdigit() and following.isdigit()):
                masked[index] = " "
                index += 1
                state = "character"
                continue
        elif state == "line-comment":
            if character == "\n":
                state = "code"
            else:
                masked[index] = " "
        elif state == "block-comment":
            if character == "*" and following == "/":
                masked[index] = masked[index + 1] = " "
                index += 2
                state = "code"
                continue
            if character != "\n":
                masked[index] = " "
        elif state in {"string", "character"}:
            masked[index] = " " if character != "\n" else "\n"
            if character == "\\" and following:
                masked[index + 1] = " " if following != "\n" else "\n"
                index += 2
                continue
            if (state == "string" and character == '"') or (
                state == "character" and character == "'"
            ):
                state = "code"

        index += 1

    return "".join(masked)


def direct_accessor_violations(source: str, display_path: pathlib.Path) -> list[str]:
    masked = mask_comments_and_literals(source)
    violations: list[str] = []
    for match in DIRECT_ACCESSOR.finditer(masked):
        prefix = masked[: match.start()].rstrip()
        if prefix.endswith("typed()"):
            continue

        line = masked.count("\n", 0, match.start()) + 1
        violations.append(
            f"{display_path}:{line}: direct {match.group(1)}() accessor"
        )
    return violations


def self_test() -> None:
    synthetic = """
        client.typed().threads();
        client
            .turns();
        const auto value = 32'001;
        client.events();
        // ignored.requests();
        const char* text = "ignored.requests()";
        /* ignored->threads(); */
    """
    expected = [
        "synthetic.cpp:4: direct turns() accessor",
        "synthetic.cpp:6: direct events() accessor",
    ]
    actual = direct_accessor_violations(synthetic, pathlib.Path("synthetic.cpp"))
    if actual != expected:
        raise SystemExit(
            "grouped-facade usage guard self-test failed:\n"
            f"expected={expected!r}\nactual={actual!r}"
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", required=True, type=pathlib.Path)
    arguments = parser.parse_args()

    self_test()

    violations: list[str] = []
    for path in sorted(arguments.source_root.rglob("*")):
        if path.suffix not in {".cpp", ".h"}:
            continue

        source = path.read_text(encoding="utf-8")
        relative = path.relative_to(arguments.source_root)
        violations.extend(direct_accessor_violations(source, relative))

    if violations:
        raise SystemExit(
            "production Codex code must use client.typed() grouped facades:\n"
            + "\n".join(violations)
        )

    print("production Codex grouped-facade usage: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
