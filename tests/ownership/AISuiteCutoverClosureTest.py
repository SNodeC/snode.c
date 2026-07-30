#!/usr/bin/env python3
"""Mutation and real-Git topology tests for the AISuite cutover closure."""

from __future__ import annotations

import argparse
import copy
import hashlib
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from typing import Any, Sequence


def load_tool(path: Path) -> Any:
    spec = importlib.util.spec_from_file_location(
        "snodec_aisuite_closure_under_test", path
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import closure tool {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def run(
    command: Sequence[str],
    *,
    cwd: Path,
    check: bool = True,
    env: dict[str, str] | None = None,
    stdin: str | None = None,
) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        list(command),
        cwd=cwd,
        check=False,
        text=True,
        input=stdin,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
    )
    if check and result.returncode != 0:
        raise AssertionError(
            f"command failed ({' '.join(command)}):\n{result.stderr}"
        )
    return result


def git(repo: Path, *arguments: str, env: dict[str, str] | None = None) -> str:
    return run(["git", *arguments], cwd=repo, env=env).stdout.strip()


def write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def commit_environment(ordinal: int) -> dict[str, str]:
    timestamp = f"2001-01-{ordinal:02d}T00:00:00+00:00"
    return {
        **os.environ,
        "GIT_AUTHOR_NAME": "SNode.C Closure Test",
        "GIT_AUTHOR_EMAIL": "closure@example.invalid",
        "GIT_COMMITTER_NAME": "SNode.C Closure Test",
        "GIT_COMMITTER_EMAIL": "closure@example.invalid",
        "GIT_AUTHOR_DATE": timestamp,
        "GIT_COMMITTER_DATE": timestamp,
    }


def commit_all(repo: Path, subject: str, ordinal: int) -> str:
    git(repo, "add", "-A")
    git(repo, "commit", "-m", subject, env=commit_environment(ordinal))
    return git(repo, "rev-parse", "HEAD")


def commit_tree(
    repo: Path,
    tree: str,
    parents: Sequence[str],
    subject: str,
    ordinal: int,
) -> str:
    command = ["git", "commit-tree", tree]
    for parent in parents:
        command.extend(["-p", parent])
    result = run(
        command,
        cwd=repo,
        env=commit_environment(ordinal),
        stdin=f"{subject}\n",
    )
    return result.stdout.strip()


def tree_with_changes(
    repo: Path,
    base_tree: str,
    *,
    additions: dict[str, str] | None = None,
    removals: Sequence[str] = (),
) -> str:
    index = repo / f".git/closure-index-{os.getpid()}"
    if index.exists():
        index.unlink()
    env = {**os.environ, "GIT_INDEX_FILE": str(index)}
    run(["git", "read-tree", base_tree], cwd=repo, env=env)
    for path in removals:
        run(
            ["git", "update-index", "--force-remove", "--", path],
            cwd=repo,
            env=env,
        )
    for path, content in sorted((additions or {}).items()):
        blob = run(
            ["git", "hash-object", "-w", "--stdin"],
            cwd=repo,
            env=env,
            stdin=content,
        ).stdout.strip()
        run(
            [
                "git",
                "update-index",
                "--add",
                "--cacheinfo",
                "100644",
                blob,
                path,
            ],
            cwd=repo,
            env=env,
        )
    tree = run(["git", "write-tree"], cwd=repo, env=env).stdout.strip()
    if index.exists():
        index.unlink()
    return tree


def closure_additions(tool: Any) -> dict[str, str]:
    return {
        "docs/migrations/aisuite-cutover-closure.json": "{}\n",
        "docs/migrations/aisuite-cutover-final-ctest.json": "{}\n",
        "tests/ownership/AISuiteCutoverClosureTest.py": "# fixture\n",
        "tools/ownership/verify_aisuite_cutover_closure.py": "# fixture\n",
        "tests/ownership/CMakeLists.txt": (
            "# existing ownership registrations\n"
            + tool.CLOSURE_REGISTRATION_APPEND
        ),
        "tests/AISuiteCutoverSourcePackageTest.cmake": (
            tool.CLOSURE_SOURCE_PACKAGE_NEW_SUFFIX
        ),
    }


def make_canonical_repository(root: Path, tool: Any) -> dict[str, Any]:
    repo = root / "repo"
    repo.mkdir()
    git(repo, "init", "-q")
    git(repo, "config", "user.name", "SNode.C Closure Test")
    git(repo, "config", "user.email", "closure@example.invalid")

    write(repo / "retained.txt", "retained production input\n")
    write(
        repo / "tests/ownership/CMakeLists.txt",
        "# existing ownership registrations\n",
    )
    write(
        repo / "tests/AISuiteCutoverSourcePackageTest.cmake",
        tool.CLOSURE_SOURCE_PACKAGE_OLD_SUFFIX,
    )
    base = commit_all(repo, "Synthetic SNode.C base", 1)
    base_tree = git(repo, "show", "-s", "--format=%T", base)

    write(repo / "audit.txt", "frozen boundary\n")
    commit1 = commit_all(repo, tool.CUTOVER_SUBJECTS[0], 2)
    write(repo / "functional.txt", "functional detachment complete\n")
    commit2 = commit_all(repo, tool.CUTOVER_SUBJECTS[1], 3)
    write(repo / "residue.txt", "residue boundary complete\n")
    commit3 = commit_all(repo, tool.CUTOVER_SUBJECTS[2], 4)
    for path, content in closure_additions(tool).items():
        write(repo / path, content)
    commit4 = commit_all(repo, tool.CUTOVER_SUBJECTS[3], 5)
    commits = [commit1, commit2, commit3, commit4]
    return {
        "repo": repo,
        "base": base,
        "base_tree": base_tree,
        "commits": commits,
        "tree4": git(repo, "show", "-s", "--format=%T", commit4),
        "pinned_prefix": [
            {
                "ordinal": ordinal,
                "commit": commit,
                "tree": git(repo, "show", "-s", "--format=%T", commit),
                "parent": base if ordinal == 1 else commits[ordinal - 2],
                "subject": tool.CUTOVER_SUBJECTS[ordinal - 1],
            }
            for ordinal, commit in enumerate(commits[:3], start=1)
        ],
    }


def expect_pass(tool: Any, model: dict[str, Any], revision: str) -> dict[str, Any]:
    return tool.validate_history(
        model["repo"],
        revision=revision,
        base_commit=model["base"],
        base_tree=model["base_tree"],
        pinned_prefix=model["pinned_prefix"],
    )


def expect_history_failure(
    tool: Any,
    model: dict[str, Any],
    revision: str,
    *,
    description: str,
) -> None:
    try:
        expect_pass(tool, model, revision)
    except tool.BOUNDARY.CutoverError as error:
        if error.code != tool.HISTORY_DIAGNOSTIC:
            raise AssertionError(
                f"{description}: expected {tool.HISTORY_DIAGNOSTIC}, "
                f"got {error.code}"
            ) from error
        rendered = str(error)
        if rendered.count(tool.HISTORY_DIAGNOSTIC) != 1:
            raise AssertionError(
                f"{description}: diagnostic was not emitted exactly once"
            )
        for code in tool.BOUNDARY.BOUNDARY_DIAGNOSTIC_CODES:
            if code != tool.HISTORY_DIAGNOSTIC and code in rendered:
                raise AssertionError(
                    f"{description}: earlier unrelated diagnostic {code}"
                )
        return
    raise AssertionError(f"{description}: mutation unexpectedly passed")


def run_topology_suite(tool: Any) -> None:
    with tempfile.TemporaryDirectory(
        prefix="snodec-cutover-history-topology-"
    ) as temporary:
        model = make_canonical_repository(Path(temporary), tool)
        repo = model["repo"]
        base = model["base"]
        commit1, commit2, commit3, commit4 = model["commits"]
        tree4 = model["tree4"]

        unmerged = expect_pass(tool, model, commit4)
        if unmerged["mode"] != "unmerged":
            raise AssertionError("valid unmerged topology was misclassified")

        merge_subject = (
            "Merge pull request #41 from "
            "SNodeC/extraction/remove-codex-from-snodec"
        )
        merge = commit_tree(
            repo, tree4, [base, commit4], merge_subject, ordinal=6
        )
        if git(repo, "show", "-s", "--format=%P", merge).split() != [
            base,
            commit4,
        ]:
            raise AssertionError("valid merge fixture parents changed")
        if git(repo, "show", "-s", "--format=%T", merge) != tree4:
            raise AssertionError("valid merge fixture tree changed")
        merged = expect_pass(tool, model, merge)
        if merged["mode"] != "merged":
            raise AssertionError("valid merge topology was misclassified")

        future_tree = tree_with_changes(
            repo,
            tree4,
            additions={"future-reviewed.txt": "later reviewed change\n"},
        )
        descendant = commit_tree(
            repo,
            future_tree,
            [merge],
            "Later reviewed SNode.C development",
            ordinal=7,
        )
        later = expect_pass(tool, model, descendant)
        if later["mode"] != "descendant":
            raise AssertionError("valid later descendant was misclassified")

        altered_commit1_tree = tree_with_changes(
            repo,
            model["base_tree"],
            additions={"audit.txt": "rewritten frozen boundary\n"},
        )
        altered_commit1 = commit_tree(
            repo,
            altered_commit1_tree,
            [base],
            tool.CUTOVER_SUBJECTS[0],
            ordinal=20,
        )
        altered_commit2_tree = tree_with_changes(
            repo,
            altered_commit1_tree,
            additions={"functional.txt": "functional detachment complete\n"},
        )
        altered_commit2 = commit_tree(
            repo,
            altered_commit2_tree,
            [altered_commit1],
            tool.CUTOVER_SUBJECTS[1],
            ordinal=21,
        )
        altered_commit3_tree = tree_with_changes(
            repo,
            altered_commit2_tree,
            additions={"residue.txt": "residue boundary complete\n"},
        )
        altered_commit3 = commit_tree(
            repo,
            altered_commit3_tree,
            [altered_commit2],
            tool.CUTOVER_SUBJECTS[2],
            ordinal=22,
        )
        altered_commit4_tree = tree_with_changes(
            repo,
            altered_commit3_tree,
            additions=closure_additions(tool),
        )
        altered_commit4 = commit_tree(
            repo,
            altered_commit4_tree,
            [altered_commit3],
            tool.CUTOVER_SUBJECTS[3],
            ordinal=23,
        )
        altered_chain = git(
            repo, "rev-list", "--reverse", f"{base}..{altered_commit4}"
        ).splitlines()
        if (
            len(altered_chain) != 4
            or altered_chain[0] == commit1
            or [
                git(repo, "show", "-s", "--format=%s", commit)
                for commit in altered_chain
            ]
            != tool.CUTOVER_SUBJECTS
        ):
            raise AssertionError("altered-prefix-chain mutation did not apply")
        expect_history_failure(
            tool,
            model,
            altered_commit4,
            description="altered immutable Commit 1-3 prefix",
        )

        wrong_order = commit_tree(
            repo, tree4, [commit4, base], merge_subject, ordinal=8
        )
        if git(repo, "show", "-s", "--format=%P", wrong_order).split() != [
            commit4,
            base,
        ]:
            raise AssertionError("wrong-parent-order mutation did not apply")
        expect_history_failure(
            tool,
            model,
            wrong_order,
            description="wrong merge parent order",
        )

        wrong_second = commit_tree(
            repo, tree4, [base, commit3], merge_subject, ordinal=9
        )
        if git(repo, "show", "-s", "--format=%P", wrong_second).split()[1] != (
            commit3
        ):
            raise AssertionError("wrong-second-parent mutation did not apply")
        expect_history_failure(
            tool,
            model,
            wrong_second,
            description="wrong merge second parent",
        )

        wrong_merge_tree = tree_with_changes(
            repo, tree4, additions={"merge-only.txt": "wrong merge tree\n"}
        )
        wrong_tree_merge = commit_tree(
            repo,
            wrong_merge_tree,
            [base, commit4],
            merge_subject,
            ordinal=10,
        )
        if git(repo, "show", "-s", "--format=%T", wrong_tree_merge) == tree4:
            raise AssertionError("wrong-merge-tree mutation did not apply")
        expect_history_failure(
            tool,
            model,
            wrong_tree_merge,
            description="wrong merge tree",
        )

        inserted = commit_tree(
            repo,
            git(repo, "show", "-s", "--format=%T", commit2),
            [commit2],
            "Inserted unrelated commit",
            ordinal=11,
        )
        commit3_inserted = commit_tree(
            repo,
            git(repo, "show", "-s", "--format=%T", commit3),
            [inserted],
            tool.CUTOVER_SUBJECTS[2],
            ordinal=12,
        )
        inserted_tree4 = tree_with_changes(
            repo,
            git(repo, "show", "-s", "--format=%T", commit3_inserted),
            additions=closure_additions(tool),
        )
        commit4_inserted = commit_tree(
            repo,
            inserted_tree4,
            [commit3_inserted],
            tool.CUTOVER_SUBJECTS[3],
            ordinal=13,
        )
        inserted_count = int(
            git(repo, "rev-list", "--count", f"{base}..{commit4_inserted}")
        )
        if inserted_count != 5:
            raise AssertionError("inserted-commit mutation did not apply")
        expect_history_failure(
            tool,
            model,
            commit4_inserted,
            description="inserted commit",
        )

        merge2 = commit_tree(
            repo,
            tree4,
            [base, commit4],
            (
                "Merge pull request #42 from "
                "SNodeC/extraction/remove-codex-from-snodec"
            ),
            ordinal=14,
        )
        duplicate_head = commit_tree(
            repo,
            tree4,
            [merge, merge2],
            "Aggregate duplicate merge candidates",
            ordinal=15,
        )
        reachable_merges = [
            candidate
            for candidate in git(repo, "rev-list", "--merges", duplicate_head).splitlines()
            if "extraction/remove-codex-from-snodec"
            in git(repo, "show", "-s", "--format=%s", candidate)
        ]
        if len(reachable_merges) != 2:
            raise AssertionError("duplicate-merge mutation did not apply")
        expect_history_failure(
            tool,
            model,
            duplicate_head,
            description="duplicate merge candidate",
        )

        deletion_tree = tree_with_changes(
            repo,
            git(repo, "show", "-s", "--format=%T", commit3),
            additions=closure_additions(tool),
            removals=["retained.txt"],
        )
        deletion_commit4 = commit_tree(
            repo,
            deletion_tree,
            [commit3],
            tool.CUTOVER_SUBJECTS[3],
            ordinal=16,
        )
        deleted_paths = git(
            repo,
            "diff-tree",
            "--no-commit-id",
            "--name-status",
            "-r",
            commit3,
            deletion_commit4,
        )
        if "D\tretained.txt" not in deleted_paths:
            raise AssertionError("functional-deletion mutation did not apply")
        expect_history_failure(
            tool,
            model,
            deletion_commit4,
            description="functional deletion in Commit 4",
        )

        bad_commit3_tree = tree_with_changes(
            repo,
            git(repo, "show", "-s", "--format=%T", commit2),
            additions={
                "residue.txt": "residue boundary incomplete\n",
                "src/CMakeLists.txt": "add_subdirectory(ai)\n",
            },
        )
        bad_commit3 = commit_tree(
            repo,
            bad_commit3_tree,
            [commit2],
            tool.CUTOVER_SUBJECTS[2],
            ordinal=17,
        )
        corrected_additions = closure_additions(tool)
        corrected_additions["src/CMakeLists.txt"] = "# corrected in Commit 4\n"
        corrected_tree4 = tree_with_changes(
            repo,
            bad_commit3_tree,
            additions=corrected_additions,
        )
        corrected_commit4 = commit_tree(
            repo,
            corrected_tree4,
            [bad_commit3],
            tool.CUTOVER_SUBJECTS[3],
            ordinal=18,
        )
        correction_diff = git(
            repo,
            "diff-tree",
            "--no-commit-id",
            "--name-status",
            "-r",
            bad_commit3,
            corrected_commit4,
        )
        if "M\tsrc/CMakeLists.txt" not in correction_diff:
            raise AssertionError("hidden-correction mutation did not apply")
        expect_history_failure(
            tool,
            model,
            corrected_commit4,
            description="functional correction hidden in Commit 4",
        )

        weakened_registrations = closure_additions(tool)
        weakened_registrations["tests/ownership/CMakeLists.txt"] = (
            "# existing ownership registrations\n"
            + tool.CLOSURE_REGISTRATION_APPEND
            + "set_tests_properties(AISuiteCutoverClosureTest "
            "PROPERTIES DISABLED TRUE)\n"
        )
        weakened_tree4 = tree_with_changes(
            repo,
            git(repo, "show", "-s", "--format=%T", commit3),
            additions=weakened_registrations,
        )
        weakened_commit4 = commit_tree(
            repo,
            weakened_tree4,
            [commit3],
            tool.CUTOVER_SUBJECTS[3],
            ordinal=19,
        )
        weakened_diff = git(
            repo,
            "diff-tree",
            "--no-commit-id",
            "--name-status",
            "-r",
            commit3,
            weakened_commit4,
        )
        if (
            "M\ttests/ownership/CMakeLists.txt" not in weakened_diff
            or "DISABLED TRUE"
            not in git(
                repo,
                "show",
                f"{weakened_commit4}:tests/ownership/CMakeLists.txt",
            )
        ):
            raise AssertionError(
                "allowed-path registration weakening did not apply"
            )
        expect_history_failure(
            tool,
            model,
            weakened_commit4,
            description="allowed-path closure registration weakening",
        )

        conditional_package = closure_additions(tool)
        conditional_package[
            "tests/AISuiteCutoverSourcePackageTest.cmake"
        ] = (
            "if(FALSE)\n"
            + tool.CLOSURE_SOURCE_PACKAGE_NEW_SUFFIX
            + "endif()\n"
        )
        conditional_package_tree4 = tree_with_changes(
            repo,
            git(repo, "show", "-s", "--format=%T", commit3),
            additions=conditional_package,
        )
        conditional_package_commit4 = commit_tree(
            repo,
            conditional_package_tree4,
            [commit3],
            tool.CUTOVER_SUBJECTS[3],
            ordinal=24,
        )
        conditional_package_source = git(
            repo,
            "show",
            (
                f"{conditional_package_commit4}:"
                "tests/AISuiteCutoverSourcePackageTest.cmake"
            ),
        )
        if (
            not conditional_package_source.startswith("if(FALSE)")
            or conditional_package_source.count("check-package") != 1
        ):
            raise AssertionError(
                "allowed-path source-package weakening did not apply"
            )
        expect_history_failure(
            tool,
            model,
            conditional_package_commit4,
            description="allowed-path source-package closure weakening",
        )

        final_authority = expect_pass(tool, model, commit4)
        if final_authority["mode"] != "unmerged":
            raise AssertionError("unmodified authority did not pass afterward")


def valid_downstream_proof(tool: Any) -> dict[str, Any]:
    repo = Path(tool.__file__).resolve().parents[2]
    closure = tool.read_canonical_json(
        repo / "docs/migrations/aisuite-cutover-closure.json",
        code=tool.MIGRATION_DIAGNOSTIC,
    )
    proof = closure.get("downstream_aisuite")
    if not isinstance(proof, dict):
        raise AssertionError("checked closure lacks downstream AISuite proof")
    return copy.deepcopy(proof)


def expect_code(tool: Any, callback: Any, expected: str, description: str) -> None:
    try:
        callback()
    except tool.BOUNDARY.CutoverError as error:
        if error.code != expected:
            raise AssertionError(
                f"{description}: expected {expected}, got {error.code}"
            ) from error
        if str(error).count(expected) != 1:
            raise AssertionError(
                f"{description}: expected diagnostic exactly once"
            )
        return
    raise AssertionError(f"{description}: mutation unexpectedly passed")


def run_closure_unit_suite(tool: Any) -> None:
    authority = valid_downstream_proof(tool)
    tool.validate_downstream_summary(authority)

    old_resolution = copy.deepcopy(authority)
    old_resolution["paths"]["legacy_snodec_source_build_hits"] = [
        "${PROVENANCE_SNODEC_SOURCE}"
    ]
    if old_resolution == authority:
        raise AssertionError("downstream path mutation did not change input")
    expect_code(
        tool,
        lambda: tool.validate_downstream_summary(old_resolution),
        tool.AISUITE_CONSUMER_DIAGNOSTIC,
        "historical/source-tree downstream resolution",
    )

    changed_owner = copy.deepcopy(authority)
    changed_owner["authority"]["aisuite_tree"] = "0" * 40
    if changed_owner == authority:
        raise AssertionError("AISuite authority mutation did not change input")
    expect_code(
        tool,
        lambda: tool.validate_downstream_summary(changed_owner),
        tool.AISUITE_CONSUMER_DIAGNOSTIC,
        "changed AISuite downstream authority",
    )

    absolute_path = copy.deepcopy(authority)
    absolute_path["configuration"]["inspection_note"] = "/usr/local"
    if absolute_path == authority:
        raise AssertionError("absolute-path mutation did not change input")
    expect_code(
        tool,
        lambda: tool.validate_downstream_summary(absolute_path),
        tool.AISUITE_CONSUMER_DIAGNOSTIC,
        "unnormalized /usr/local downstream path",
    )

    stable_calls = 0

    def stable_collector() -> tuple[dict[str, Any], dict[str, Any]]:
        nonlocal stable_calls
        stable_calls += 1
        return {"pass": "same"}, {"pass": "same"}

    stable = tool.generate_twice(stable_collector)
    if stable_calls != 2 or stable != (
        {"pass": "same"},
        {"pass": "same"},
    ):
        raise AssertionError("valid two-pass generation did not pass")

    changing_calls = 0

    def changing_collector() -> tuple[dict[str, Any], dict[str, Any]]:
        nonlocal changing_calls
        changing_calls += 1
        return (
            {"pass": "same"},
            {"pass": "first" if changing_calls == 1 else "second"},
        )

    expect_code(
        tool,
        lambda: tool.generate_twice(changing_collector),
        tool.SECOND_PASS_DIAGNOSTIC,
        "second-pass nondeterminism",
    )
    if changing_calls != 2:
        raise AssertionError("nondeterminism mutation did not reach pass two")

    comparison = tool.manifest_comparison(
        ["a", "b", "removed"],
        ["a", "b", "added"],
        expected_preserved=["a", "b"],
    )
    if comparison["removed"] != ["removed"] or comparison["added"] != [
        "added"
    ]:
        raise AssertionError("exact manifest comparison changed")
    if comparison["missing_preserved"]:
        raise AssertionError("valid preserved manifest was rejected")

    with tempfile.TemporaryDirectory(
        prefix="snodec-cutover-optional-targets-"
    ) as temporary:
        build = Path(temporary)
        write(
            build / "CMakeCache.txt",
            "DOXYGEN_EXECUTABLE:FILEPATH=DOXYGEN_EXECUTABLE-NOTFOUND\n",
        )
        canonical = tool.canonicalize_environment_conditional_targets(
            ["all"], build
        )
        if canonical != ["all", "doc", "doc-fast"]:
            raise AssertionError(
                "cache-proven absent Doxygen targets were not canonicalized"
            )
        partial = ["all", "doc"]
        if partial == canonical:
            raise AssertionError(
                "partial optional-target mutation did not change input"
            )
        expect_code(
            tool,
            lambda: tool.canonicalize_environment_conditional_targets(
                partial, build
            ),
            tool.NON_CODEX_DIAGNOSTIC,
            "partially configured Doxygen target pair",
        )
        write(
            build / "CMakeCache.txt",
            "DOXYGEN_EXECUTABLE:FILEPATH=/usr/bin/doxygen\n",
        )
        if tool.canonicalize_environment_conditional_targets(
            ["all"], build
        ) != ["all"]:
            raise AssertionError(
                "available Doxygen unexpectedly weakened target preservation"
            )

    with tempfile.TemporaryDirectory(
        prefix="snodec-cutover-package-path-mutation-"
    ) as temporary:
        package = Path(temporary)
        for path in tool.CLOSURE_NEW_PATHS:
            write(package / path, "fixture\n")
        tool.require_closure_source_paths(package)
        removed_path = package / tool.CLOSURE_NEW_PATHS[0]
        removed_path.unlink()
        if removed_path.exists():
            raise AssertionError("missing-package-path mutation did not apply")
        expect_code(
            tool,
            lambda: tool.require_closure_source_paths(package),
            tool.PACKAGE_DIAGNOSTIC,
            "missing one closure package file",
        )

    tool.validate_downstream_summary(authority)


def source_package_stage(source: Path, build: Path) -> Path:
    if os.environ.get("RUNNER_TEMP"):
        temporary_root = Path(os.environ["RUNNER_TEMP"])
    elif os.environ.get("TMPDIR"):
        temporary_root = Path(os.environ["TMPDIR"])
    else:
        temporary_root = Path("/tmp")
    key = hashlib.sha256(
        f"{source.resolve()}|{build.resolve()}".encode("utf-8")
    ).hexdigest()[:16]
    return temporary_root / f"snodec-aisuite-cutover-source-package-{key}"


def find_extracted_source(stage: Path) -> Path:
    candidates: list[Path] = []
    for cmake in sorted((stage / "extracted").rglob("CMakeLists.txt")):
        try:
            content = cmake.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError):
            continue
        if (
            "project(" in content
            and "SNode.C" in content
            and (cmake.parent / "LICENSE").is_file()
            and (cmake.parent / "README.md").is_file()
        ):
            candidates.append(cmake.parent)
    candidates = sorted(set(candidates))
    if len(candidates) != 1:
        raise AssertionError(
            f"expected one extracted SNode.C source, found {candidates}"
        )
    return candidates[0]


def run_checked_closure(
    tool: Any, tool_path: Path, repo: Path, build: Path
) -> None:
    repo = repo.resolve()
    build = build.resolve()
    source_stage = source_package_stage(repo, build)
    source_manifest = source_stage / "source-manifest.txt"
    binary_manifest = (
        build
        / "aisuite-cutover-binary-package-audit"
        / "binary-manifest.txt"
    )
    install_root = build / "staged-installed-consumer/prefix"
    snodec_config = build / "src/snodecConfig.cmake"
    cpack_config = build / "CPackConfig.cmake"
    required = [
        source_manifest,
        binary_manifest,
        install_root,
        snodec_config,
        cpack_config,
    ]
    missing = [str(path) for path in required if not path.exists()]
    if missing:
        raise AssertionError(
            "closure prerequisites did not leave expected artifacts: "
            f"{missing}"
        )

    with tempfile.TemporaryDirectory(
        prefix="snodec-cutover-closure-check-"
    ) as temporary:
        target_inventory = Path(temporary) / "targets.txt"
        target_result = run(
            ["cmake", "--build", str(build), "--target", "help"],
            cwd=repo,
            check=False,
        )
        if target_result.returncode != 0:
            raise AssertionError(
                "could not collect final target inventory: "
                f"{target_result.stderr}"
            )
        target_inventory.write_text(target_result.stdout, encoding="utf-8")
        command = [
            sys.executable,
            "-I",
            "-B",
            str(tool_path),
            "check",
            "--repo-root",
            str(repo),
            "--build-root",
            str(build),
            "--install-root",
            str(install_root),
            "--source-package-manifest",
            str(source_manifest),
            "--binary-package-manifest",
            str(binary_manifest),
            "--target-inventory",
            str(target_inventory),
            "--snodec-config",
            str(snodec_config),
            "--cpack-config",
            str(cpack_config),
        ]
        result = run(command, cwd=repo, check=False)
        if result.returncode != 0:
            raise AssertionError(
                "live exact closure check failed:\n"
                f"{result.stdout}\n{result.stderr}"
            )

    extracted = find_extracted_source(source_stage)
    package_tool = (
        extracted / "tools/ownership/verify_aisuite_cutover_closure.py"
    )
    empty_home = source_stage / "closure-empty-home"
    empty_home.mkdir(exist_ok=True)
    isolated = {
        **os.environ,
        "HOME": str(empty_home),
        "CMAKE_PREFIX_PATH": "",
        "GIT_DIR": str(extracted / ".git-forbidden"),
        "GIT_CEILING_DIRECTORIES": str(source_stage / "extracted"),
        "GIT_CONFIG_GLOBAL": "/dev/null",
        "GIT_CONFIG_NOSYSTEM": "1",
        "http_proxy": "http://127.0.0.1:9",
        "https_proxy": "http://127.0.0.1:9",
        "ALL_PROXY": "http://127.0.0.1:9",
        "NO_PROXY": "",
        "CMAKE_FIND_USE_PACKAGE_REGISTRY": "FALSE",
        "CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY": "TRUE",
        "CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY": "FALSE",
        "CMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY": "TRUE",
        "PYTHONDONTWRITEBYTECODE": "1",
    }
    package_result = run(
        [
            sys.executable,
            "-I",
            "-B",
            str(package_tool),
            "check-package",
            "--repo-root",
            str(extracted),
        ],
        cwd=extracted,
        check=False,
        env=isolated,
    )
    if package_result.returncode != 0:
        raise AssertionError(
            "package-safe closure check failed:\n"
            f"{package_result.stdout}\n{package_result.stderr}"
        )


def parse_arguments(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tool", type=Path, required=True)
    parser.add_argument(
        "--suite", choices=["topology", "closure", "all"], default="all"
    )
    parser.add_argument("--repo-root", type=Path)
    parser.add_argument("--build-root", type=Path)
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_arguments(sys.argv[1:] if argv is None else argv)
    tool = load_tool(args.tool.resolve())
    try:
        if args.suite in {"topology", "all"}:
            run_topology_suite(tool)
            print(
                "AISuite cutover real-Git topology mutations verified: "
                "unmerged, merge, descendant, and 10 rejected mutations"
            )
        if args.suite in {"closure", "all"}:
            run_closure_unit_suite(tool)
            if (args.repo_root is None) != (args.build_root is None):
                raise AssertionError(
                    "--repo-root and --build-root must be supplied together"
                )
            if args.repo_root is not None and args.build_root is not None:
                run_checked_closure(
                    tool,
                    args.tool.resolve(),
                    args.repo_root,
                    args.build_root,
                )
            print(
                "AISuite cutover closure mutations verified: "
                "downstream-path, owner, manifests, and second-pass"
            )
        return 0
    except (AssertionError, OSError, RuntimeError) as error:
        print(f"AISuiteCutoverClosureTest: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
