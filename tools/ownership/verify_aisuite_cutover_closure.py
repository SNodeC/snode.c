#!/usr/bin/env python3
"""Generate and verify the bounded SNode.C -> AISuite cutover closure.

The permanent source/package boundary is implemented by
``verify_aisuite_cutover.py``.  This companion deliberately adds only closure:

* exact baseline-versus-final artifact evidence;
* the bounded four-commit history policy;
* normalized downstream AISuite review evidence; and
* package-safe validation of the checked-in result.

The generated evidence never contains the fourth commit's object name or tree.
That avoids a Git-object self-reference while still allowing the live checker
to validate an unmerged branch, its normal GitHub merge, and later descendants.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tarfile
import tempfile
from types import ModuleType, SimpleNamespace
from typing import Any, Iterable, Sequence


FORMAT_VERSION = 1
FINAL_CTEST_FILE = "aisuite-cutover-final-ctest.json"
CLOSURE_FILE = "aisuite-cutover-closure.json"
CUTOVER_PROJECT_VERSION = "1.0.1"
CUTOVER_SOVERSION = "1"

CUTOVER_SUBJECTS = [
    "Freeze the SNode.C Codex cutover boundary",
    "Remove Codex from the SNode.C build and package surface",
    "Remove duplicated Codex residue and document AISuite ownership",
    "Close and verify the SNode.C Codex cutover",
]
PINNED_CUTOVER_PREFIX = [
    {
        "ordinal": 1,
        "commit": "9ca34e4d1112b9d4cac55a65e5c9ba53d36f3cd4",
        "tree": "0e5d30a7f520ae64caeb8c648e802a685e702cee",
        "parent": "d18b231a1d2ec2235fd6f204786b0a761cc24ff5",
        "subject": CUTOVER_SUBJECTS[0],
    },
    {
        "ordinal": 2,
        "commit": "9e1688612152accc3e9db3f7462c27b9a24ac7d0",
        "tree": "9d9a3c1b5210591389e39e51598d8c006574032f",
        "parent": "9ca34e4d1112b9d4cac55a65e5c9ba53d36f3cd4",
        "subject": CUTOVER_SUBJECTS[1],
    },
    {
        "ordinal": 3,
        "commit": "cb5bfba49e20f9f859b2967d9413b35d50a93168",
        "tree": "47af18d6e390d2ac9178a7425f10f5b74c047714",
        "parent": "9e1688612152accc3e9db3f7462c27b9a24ac7d0",
        "subject": CUTOVER_SUBJECTS[2],
    },
]
MERGE_SUBJECT_PATTERN = (
    r"^Merge pull request #[1-9][0-9]* from "
    r"SNodeC/extraction/remove-codex-from-snodec$"
)

CLOSURE_NEW_PATHS = [
    "docs/migrations/aisuite-cutover-closure.json",
    "docs/migrations/aisuite-cutover-final-ctest.json",
    "tests/ownership/AISuiteCutoverClosureTest.py",
    "tools/ownership/verify_aisuite_cutover_closure.py",
]
CLOSURE_REGISTRATION_PATH = "tests/ownership/CMakeLists.txt"
CLOSURE_SOURCE_PACKAGE_PATH = "tests/AISuiteCutoverSourcePackageTest.cmake"
CLOSURE_TEST_NAMES = [
    "AISuiteCutoverHistoryTopologyTest",
    "AISuiteCutoverClosureTest",
]
ALL_CUTOVER_TEST_NAMES = [
    "AISuiteCutoverBinaryPackageTest",
    "AISuiteCutoverClosureTest",
    "AISuiteCutoverHistoryTopologyTest",
    "AISuiteCutoverMutationTest",
    "AISuiteCutoverSourcePackageTest",
    "AISuiteCutoverStartStateAuditTest",
    "AISuiteCutoverStartStateMutationTest",
    "AISuiteOwnershipBoundaryPolicyTest",
]

# Frozen from the generator-neutral logical target model shared by Ninja and
# Unix Makefiles. The value is independent of Commit 4's Git object and tree.
EXPECTED_FINAL_TARGETS_SHA256 = (
    "3aefc298bb88987a50e92eab05cacfc1fae0c3f8ed5eaae8b6fc5931a60d55a4"
)
DOXYGEN_OPTIONAL_TARGETS = ["doc", "doc-fast"]

CLOSURE_REGISTRATION_APPEND = """
set(AISUITE_CUTOVER_CLOSURE_TOOL
    "${PROJECT_SOURCE_DIR}/tools/ownership/verify_aisuite_cutover_closure.py"
)

add_test(
    NAME AISuiteCutoverHistoryTopologyTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/AISuiteCutoverClosureTest.py --tool
        ${AISUITE_CUTOVER_CLOSURE_TOOL} --suite topology
)
set_tests_properties(
    AISuiteCutoverHistoryTopologyTest
    PROPERTIES LABELS "policy;architecture;ownership;cutover;package"
               ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1" TIMEOUT 120
)

add_test(
    NAME AISuiteCutoverClosureTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/AISuiteCutoverClosureTest.py --tool
        ${AISUITE_CUTOVER_CLOSURE_TOOL} --suite closure --repo-root
        ${PROJECT_SOURCE_DIR} --build-root ${CMAKE_BINARY_DIR}
)
set_tests_properties(
    AISuiteCutoverClosureTest
    PROPERTIES
        LABELS
        "policy;architecture;ownership;cutover;package"
        ENVIRONMENT
        "PYTHONDONTWRITEBYTECODE=1"
        TIMEOUT
        180
        DEPENDS
        "AISuiteCutoverHistoryTopologyTest;AISuiteOwnershipBoundaryPolicyTest;AISuiteCutoverMutationTest;StagedInstalledConsumerTest;AISuiteCutoverSourcePackageTest;AISuiteCutoverBinaryPackageTest"
)
"""
CLOSURE_SOURCE_PACKAGE_OLD_SUFFIX = """list(LENGTH source_package_files source_package_file_count)
message(
    STATUS
        "AISuite cutover source package verified: files=${source_package_file_count}, package-safe=passed"
)
"""
CLOSURE_SOURCE_PACKAGE_NEW_SUFFIX = """set(closure_checker
    "${source_root}/tools/ownership/verify_aisuite_cutover_closure.py"
)
if(NOT EXISTS "${closure_checker}")
    message(FATAL_ERROR "source package lacks AISuite closure checker")
endif()
execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env "HOME=${empty_home}" "CMAKE_PREFIX_PATH="
        "GIT_DIR=${source_root}/.git-forbidden"
        "GIT_CEILING_DIRECTORIES=${extracted_stage}"
        "GIT_CONFIG_GLOBAL=/dev/null" "GIT_CONFIG_NOSYSTEM=1"
        "http_proxy=http://127.0.0.1:9" "https_proxy=http://127.0.0.1:9"
        "ALL_PROXY=http://127.0.0.1:9" "NO_PROXY="
        "CMAKE_FIND_USE_PACKAGE_REGISTRY=FALSE"
        "CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=TRUE"
        "CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=FALSE"
        "CMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY=TRUE"
        "PYTHONDONTWRITEBYTECODE=1" "${AISUITE_CUTOVER_PYTHON}" -I -B
        "${closure_checker}" check-package --repo-root "${source_root}"
    WORKING_DIRECTORY "${source_root}"
    RESULT_VARIABLE closure_check_result
    OUTPUT_VARIABLE closure_check_output
    ERROR_VARIABLE closure_check_error
)
if(NOT closure_check_result EQUAL 0)
    message(
        FATAL_ERROR
            "package-safe AISuite closure check failed\\n${closure_check_output}\\n${closure_check_error}"
    )
endif()

list(LENGTH source_package_files source_package_file_count)
message(
    STATUS
        "AISuite cutover source package verified: files=${source_package_file_count}, boundary=passed, closure=passed"
)
"""

DOWNSTREAM_TARGETS = [
    "ai-openai-codex",
    "ai-openai-codex-backend",
    "ai-openai-codex-frontend",
    "codex-backend",
    "codex-backend-client",
]
DOWNSTREAM_CHECKS = [
    "installed_headers",
    "source_package",
    "binary_package",
    "ownership",
    "extraction",
]
DOWNSTREAM_INSPECTIONS = [
    "cmake_cache",
    "compile_commands",
    "verbose_link",
    "readelf",
    "ldd",
]
AISUITE_FOCUSED_LABEL_EXPRESSION = "ai|openai|codex|extraction"
AISUITE_FOCUSED_TEST_COUNT = 146
AISUITE_FOCUSED_TEST_SHA256 = (
    "d32db763f2043cde2d6e2324309ca1f68ed5a34b7bcda73e6d026fc79529661e"
)
AISUITE_FOCUSED_REQUIRED_TESTS = [
    "AISuiteBinaryPackageTest",
    "AISuiteExtractionGuardTest",
    "AISuiteInstalledConsumerTest",
    "AISuiteSourcePackageTest",
    "CodexLoggingApiSurfacePolicyTest",
    "CodexPolicyOwnershipTest",
    "CodexPublicHeaderPolicyTest",
    "CodexSemanticLoggerPolicyTest",
    "CodexSyntheticSecretLeakGuardTest",
]
MIGRATION_REQUIREMENT_KEYS = [
    "historical:phase-2",
    "historical:phase-3",
    "historical:phase-3-accounting",
    "historical:test-suite-consolidation",
    "migration:aisuite-backend-target",
    "migration:aisuite-core-target",
    "migration:aisuite-find-package",
    "migration:aisuite-frontend-target",
    "migration:aisuite-owner-commit",
    "migration:aisuite-owner-tree",
    "migration:clean-prefix-warning",
    "migration:dependency-direction",
    "migration:immutable-provenance",
    "migration:no-shim",
    "migration:no-snodec-dependency",
    "migration:normal-pin-follow-up",
    "migration:package-api-break",
    "migration:public-include-form",
    "migration:removed-backend-app",
    "migration:removed-backend-component",
    "migration:removed-client-app",
    "migration:removed-core-component",
    "migration:removed-frontend-component",
    "migration:rerun-aisuite-ci",
    "migration:retained-abi",
    "migration:snodec-base-commit",
    "migration:snodec-base-tree",
    "readme:aisuite-project",
]
REQUIRED_CI_PATH_FAMILIES = [
    "CMakeLists.txt",
    "cmake/**",
    "src/**",
    "tests/**",
    "tools/**",
    ".github/workflows/**",
]


def load_boundary_tool(tool_path: Path) -> ModuleType:
    spec = importlib.util.spec_from_file_location(
        "snodec_aisuite_boundary", tool_path
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import boundary tool {tool_path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


BOUNDARY = load_boundary_tool(
    Path(__file__).resolve().with_name("verify_aisuite_cutover.py")
)

HISTORY_DIAGNOSTIC = BOUNDARY.HISTORY_DIAGNOSTIC
MIGRATION_DIAGNOSTIC = BOUNDARY.MIGRATION_DIAGNOSTIC
NON_CODEX_DIAGNOSTIC = BOUNDARY.NON_CODEX_DIAGNOSTIC
PATH_DIAGNOSTIC = BOUNDARY.PATH_DIAGNOSTIC
BUILD_DIAGNOSTIC = BOUNDARY.BUILD_DIAGNOSTIC
INSTALL_DIAGNOSTIC = BOUNDARY.INSTALL_DIAGNOSTIC
PACKAGE_DIAGNOSTIC = BOUNDARY.PACKAGE_DIAGNOSTIC
TEST_DIAGNOSTIC = BOUNDARY.TEST_DIAGNOSTIC
POLICY_DIAGNOSTIC = BOUNDARY.POLICY_DIAGNOSTIC
SOVERSION_DIAGNOSTIC = BOUNDARY.SOVERSION_DIAGNOSTIC
DEPENDENCY_DIAGNOSTIC = BOUNDARY.DEPENDENCY_DIAGNOSTIC
AISUITE_CONSUMER_DIAGNOSTIC = BOUNDARY.AISUITE_CONSUMER_DIAGNOSTIC
SECOND_PASS_DIAGNOSTIC = BOUNDARY.SECOND_PASS_DIAGNOSTIC


def fail(code: str, message: str) -> None:
    raise BOUNDARY.CutoverError(code, message)


def run(
    command: Sequence[str],
    *,
    cwd: Path,
    check: bool = True,
    env: dict[str, str] | None = None,
    binary: bool = False,
) -> subprocess.CompletedProcess[Any]:
    result = subprocess.run(
        list(command),
        cwd=cwd,
        check=False,
        text=not binary,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
    )
    if check and result.returncode != 0:
        stderr = (
            result.stderr.decode("utf-8", errors="replace")
            if binary
            else result.stderr
        )
        fail(
            HISTORY_DIAGNOSTIC,
            f"command failed ({' '.join(command)}): {stderr.strip()}",
        )
    return result


def git(repo: Path, *arguments: str) -> str:
    return run(["git", *arguments], cwd=repo).stdout.strip()


def canonical_json_bytes(value: Any) -> bytes:
    return BOUNDARY.canonical_json_bytes(value)


def sha256_bytes(content: bytes) -> str:
    return hashlib.sha256(content).hexdigest()


def canonical_string_manifest(entries: Iterable[str]) -> bytes:
    return (
        "".join(f"{entry}\n" for entry in sorted(set(entries)))
    ).encode("utf-8")


def string_manifest_sha256(entries: Iterable[str]) -> str:
    return sha256_bytes(canonical_string_manifest(entries))


def read_canonical_json(path: Path, *, code: str) -> Any:
    try:
        content = path.read_bytes()
        value = json.loads(content)
    except (OSError, json.JSONDecodeError) as error:
        fail(code, f"cannot read JSON {path}: {error}")
    if content != canonical_json_bytes(value):
        fail(code, f"{path} is not canonical generated JSON")
    return value


def _git_parents(repo: Path, commit: str) -> list[str]:
    fields = git(repo, "show", "-s", "--format=%P", commit).split()
    return fields


def _git_subject(repo: Path, commit: str) -> str:
    return git(repo, "show", "-s", "--format=%s", commit)


def _git_tree(repo: Path, commit: str) -> str:
    return git(repo, "show", "-s", "--format=%T", commit)


def _resolve_commit(repo: Path, revision: str) -> str:
    result = run(
        ["git", "rev-parse", "--verify", f"{revision}^{{commit}}"],
        cwd=repo,
        check=False,
    )
    if result.returncode != 0:
        fail(HISTORY_DIAGNOSTIC, f"cannot resolve revision {revision}")
    return result.stdout.strip()


def _is_ancestor(repo: Path, ancestor: str, descendant: str) -> bool:
    return (
        run(
            ["git", "merge-base", "--is-ancestor", ancestor, descendant],
            cwd=repo,
            check=False,
        ).returncode
        == 0
    )


def _parse_diff_status(
    repo: Path, parent: str, commit: str
) -> list[tuple[str, list[str]]]:
    output = git(
        repo,
        "diff-tree",
        "--no-commit-id",
        "--name-status",
        "-r",
        parent,
        commit,
    )
    records: list[tuple[str, list[str]]] = []
    for line in output.splitlines():
        fields = line.split("\t")
        if len(fields) < 2:
            fail(HISTORY_DIAGNOSTIC, f"malformed diff-tree record: {line}")
        records.append((fields[0], fields[1:]))
    return records


def _validate_closure_registration_append(
    repo: Path, commit3: str, commit4: str
) -> None:
    before = run(
        ["git", "show", f"{commit3}:{CLOSURE_REGISTRATION_PATH}"],
        cwd=repo,
        binary=True,
    ).stdout
    after = run(
        ["git", "show", f"{commit4}:{CLOSURE_REGISTRATION_PATH}"],
        cwd=repo,
        binary=True,
    ).stdout
    expected = before + CLOSURE_REGISTRATION_APPEND.encode("utf-8")
    if after != expected:
        fail(
            HISTORY_DIAGNOSTIC,
            "Commit 4 closure registrations differ from the exact reviewed "
            "commands, labels, properties, dependencies, or timeouts",
        )


def _validate_source_package_closure_patch(
    repo: Path, commit3: str, commit4: str
) -> None:
    before = run(
        ["git", "show", f"{commit3}:{CLOSURE_SOURCE_PACKAGE_PATH}"],
        cwd=repo,
        binary=True,
    ).stdout
    after = run(
        ["git", "show", f"{commit4}:{CLOSURE_SOURCE_PACKAGE_PATH}"],
        cwd=repo,
        binary=True,
    ).stdout
    old_suffix = CLOSURE_SOURCE_PACKAGE_OLD_SUFFIX.encode("utf-8")
    if not before.endswith(old_suffix):
        fail(
            HISTORY_DIAGNOSTIC,
            "Commit 3 source-package closure anchor changed",
        )
    expected = (
        before[: -len(old_suffix)]
        + CLOSURE_SOURCE_PACKAGE_NEW_SUFFIX.encode("utf-8")
    )
    if after != expected:
        fail(
            HISTORY_DIAGNOSTIC,
            "Commit 4 source-package change is not the exact unconditional "
            "package-safe closure invocation",
        )


def validate_commit4_diff(
    repo: Path, commit3: str, commit4: str
) -> None:
    records = _parse_diff_status(repo, commit3, commit4)
    observed: dict[str, str] = {}
    for status, paths in records:
        if len(paths) != 1 or status not in {"A", "M"}:
            fail(
                HISTORY_DIAGNOSTIC,
                f"Commit 4 contains non-closure change {status}: {paths}",
            )
        observed[paths[0]] = status

    expected = {
        **{path: "A" for path in CLOSURE_NEW_PATHS},
        CLOSURE_REGISTRATION_PATH: "M",
        CLOSURE_SOURCE_PACKAGE_PATH: "M",
    }
    if observed != expected:
        fail(
            HISTORY_DIAGNOSTIC,
            "Commit 4 path/status set is not closure-only: "
            f"expected={expected}, observed={observed}",
        )
    _validate_closure_registration_append(repo, commit3, commit4)
    _validate_source_package_closure_patch(repo, commit3, commit4)


def _candidate_cutover_chain(
    repo: Path, commit4: str, *, base_commit: str
) -> list[str] | None:
    chain = [commit4]
    for ordinal in range(3, 0, -1):
        parents = _git_parents(repo, chain[-1])
        if len(parents) != 1:
            return None
        chain.append(parents[0])
        if _git_subject(repo, chain[-1]) != CUTOVER_SUBJECTS[ordinal - 1]:
            return None
    chain.reverse()
    if _git_parents(repo, chain[0]) != [base_commit]:
        return None
    if [_git_subject(repo, commit) for commit in chain] != CUTOVER_SUBJECTS:
        return None
    return chain


def validate_history(
    repo: Path,
    *,
    revision: str = "HEAD",
    base_commit: str = BOUNDARY.SNODEC_BASE_COMMIT,
    base_tree: str = BOUNDARY.SNODEC_BASE_TREE,
    validate_closure_diff: bool = True,
    pinned_prefix: Sequence[dict[str, Any]] | None = None,
) -> dict[str, Any]:
    """Validate only the immutable four-commit cutover range."""

    repo = repo.resolve()
    head = _resolve_commit(repo, revision)
    resolved_base = _resolve_commit(repo, base_commit)
    if resolved_base != base_commit:
        fail(HISTORY_DIAGNOSTIC, "SNode.C cutover base resolved ambiguously")
    if _git_tree(repo, base_commit) != base_tree:
        fail(HISTORY_DIAGNOSTIC, "SNode.C cutover base tree changed")

    reachable = git(repo, "rev-list", head).splitlines()
    commit4_candidates: list[tuple[str, list[str]]] = []
    for commit in reachable:
        if _git_subject(repo, commit) != CUTOVER_SUBJECTS[-1]:
            continue
        chain = _candidate_cutover_chain(
            repo, commit, base_commit=base_commit
        )
        if chain is not None:
            commit4_candidates.append((commit, chain))
    if len(commit4_candidates) != 1:
        fail(
            HISTORY_DIAGNOSTIC,
            "expected exactly one structurally valid cutover head, found "
            f"{len(commit4_candidates)}",
        )

    commit4, commits = commit4_candidates[0]
    count = int(git(repo, "rev-list", "--count", f"{base_commit}..{commit4}"))
    if count != 4:
        fail(
            HISTORY_DIAGNOSTIC,
            f"bounded cutover range contains {count} commits, expected 4",
        )
    if validate_closure_diff:
        validate_commit4_diff(repo, commits[2], commits[3])
    expected_pinned_prefix = (
        PINNED_CUTOVER_PREFIX
        if pinned_prefix is None
        and base_commit == BOUNDARY.SNODEC_BASE_COMMIT
        else list(pinned_prefix or [])
    )
    if expected_pinned_prefix:
        observed_prefix = commits[:3]
        expected_prefix = [
            record["commit"] for record in expected_pinned_prefix
        ]
        if observed_prefix != expected_prefix:
            fail(
                HISTORY_DIAGNOSTIC,
                "the immutable Commit 1-3 cutover prefix changed: "
                f"expected={expected_prefix}, observed={observed_prefix}",
            )
        for record in expected_pinned_prefix:
            commit = record["commit"]
            if (
                _git_tree(repo, commit) != record["tree"]
                or _git_parents(repo, commit) != [record["parent"]]
                or _git_subject(repo, commit) != record["subject"]
            ):
                fail(
                    HISTORY_DIAGNOSTIC,
                    f"pinned cutover prefix commit changed: {commit}",
                )

    merge_candidates = [
        commit
        for commit in reachable
        if len(_git_parents(repo, commit)) > 1
        and re.fullmatch(MERGE_SUBJECT_PATTERN, _git_subject(repo, commit))
    ]
    if head == commit4:
        if merge_candidates:
            fail(
                HISTORY_DIAGNOSTIC,
                "unmerged cutover unexpectedly reaches a merge candidate",
            )
        mode = "unmerged"
        merge_commit = None
    else:
        if len(merge_candidates) != 1:
            fail(
                HISTORY_DIAGNOSTIC,
                "expected exactly one normal GitHub merge candidate, found "
                f"{len(merge_candidates)}",
            )
        merge_commit = merge_candidates[0]
        parents = _git_parents(repo, merge_commit)
        if parents != [base_commit, commit4]:
            fail(
                HISTORY_DIAGNOSTIC,
                "normal merge parent order/content changed: "
                f"expected={[base_commit, commit4]}, observed={parents}",
            )
        if _git_tree(repo, merge_commit) != _git_tree(repo, commit4):
            fail(
                HISTORY_DIAGNOSTIC,
                "normal merge tree differs from the fourth cutover commit",
            )
        if not _is_ancestor(repo, merge_commit, head):
            fail(HISTORY_DIAGNOSTIC, "normal merge is not reachable from HEAD")
        mode = "merged" if head == merge_commit else "descendant"

    return {
        "mode": mode,
        "base": base_commit,
        "commits": commits,
        "cutover_head": commit4,
        "merge": merge_commit,
        "revision": head,
    }


def validate_commit3_boundary(
    repo: Path, commit3: str, *, python: str = sys.executable
) -> None:
    """Prove that Commit 3 was already the complete functional tree."""

    archive = run(
        ["git", "archive", "--format=tar", commit3],
        cwd=repo,
        binary=True,
    ).stdout
    with tempfile.TemporaryDirectory(
        prefix="snodec-cutover-commit3-boundary-"
    ) as temporary:
        root = Path(temporary)
        with tarfile.open(fileobj=__import__("io").BytesIO(archive)) as tar:
            for member in tar.getmembers():
                destination = (root / member.name).resolve()
                if root.resolve() not in destination.parents and destination != root:
                    fail(HISTORY_DIAGNOSTIC, "unsafe path in Commit-3 archive")
            tar.extractall(root)
        tool = root / "tools/ownership/verify_aisuite_cutover.py"
        result = run(
            [
                python,
                "-I",
                "-B",
                str(tool),
                "check",
                "--repo-root",
                str(root),
            ],
            cwd=root,
            check=False,
            env={**os.environ, "PYTHONDONTWRITEBYTECODE": "1"},
        )
        if result.returncode != 0:
            fail(
                HISTORY_DIAGNOSTIC,
                "Commit 3 did not already satisfy the permanent boundary: "
                f"{result.stderr.strip()}",
            )


def normalize_manifest(entries: Iterable[str], *, description: str) -> list[str]:
    normalized = [entry.strip().replace("\\", "/") for entry in entries]
    if any(
        not entry
        or entry.startswith("/")
        or entry.startswith("./")
        or "/../" in f"/{entry}/"
        for entry in normalized
    ):
        fail(PACKAGE_DIAGNOSTIC, f"invalid {description} manifest path")
    if normalized != sorted(normalized) or len(normalized) != len(
        set(normalized)
    ):
        fail(
            PACKAGE_DIAGNOSTIC,
            f"{description} manifest is not unique and C-sorted",
        )
    return normalized


def read_manifest(path: Path, *, description: str) -> list[str]:
    try:
        entries = path.read_text(encoding="utf-8").splitlines()
    except OSError as error:
        fail(PACKAGE_DIAGNOSTIC, f"cannot read {description} {path}: {error}")
    return normalize_manifest(entries, description=description)


def scan_tree_manifest(root: Path) -> list[str]:
    root = root.resolve()
    entries = [
        path.relative_to(root).as_posix()
        for path in root.rglob("*")
        if path.is_file() or path.is_symlink()
    ]
    return sorted(set(entries))


def manifest_comparison(
    baseline: Sequence[str],
    final: Sequence[str],
    *,
    expected_preserved: Sequence[str],
) -> dict[str, Any]:
    baseline_set = set(baseline)
    final_set = set(final)
    expected_set = set(expected_preserved)
    removed = sorted(baseline_set - final_set)
    added = sorted(final_set - baseline_set)
    return {
        "baseline_count": len(baseline),
        "baseline_sha256": string_manifest_sha256(baseline),
        "final_count": len(final),
        "final_sha256": string_manifest_sha256(final),
        "removed_count": len(removed),
        "removed_sha256": string_manifest_sha256(removed),
        "removed": removed,
        "added_count": len(added),
        "added_sha256": string_manifest_sha256(added),
        "added": added,
        "missing_preserved": sorted(expected_set - final_set),
        "preserved_count": len(expected_set & final_set),
        "preserved_sha256": string_manifest_sha256(expected_set & final_set),
    }


def expected_artifact_paths(
    start: dict[str, Any],
) -> dict[str, list[str]]:
    install = sorted(
        path
        for path in start["install"]["paths"]
        if not BOUNDARY.install_path_is_forbidden(path)
    )
    binary = sorted(
        path
        for path in start["binary_package"]["paths"]
        if not BOUNDARY.install_path_is_forbidden(path)
    )
    source_preserved, source_commit3 = (
        BOUNDARY.expected_source_package_paths(start)
    )
    source = sorted(source_commit3 | set(CLOSURE_NEW_PATHS))
    return {
        "install_preserved": install,
        "install_final": install,
        "source_preserved": sorted(source_preserved),
        "source_final": source,
        "binary_preserved": binary,
        "binary_final": binary,
    }


def _ctest_raw(args: argparse.Namespace, repo: Path) -> tuple[dict[str, Any], Path | None]:
    if args.ctest_json is not None:
        if args.build_root is None:
            fail(
                TEST_DIAGNOSTIC,
                "--ctest-json requires --build-root for path normalization",
            )
        try:
            value = json.loads(args.ctest_json.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as error:
            fail(TEST_DIAGNOSTIC, f"cannot read CTest JSON: {error}")
        return value, args.build_root
    if args.build_root is None:
        fail(TEST_DIAGNOSTIC, "a CTest JSON file or build root is required")
    result = run(
        [
            "ctest",
            "--test-dir",
            str(args.build_root.resolve()),
            "--show-only=json-v1",
        ],
        cwd=repo,
        check=False,
    )
    if result.returncode != 0:
        fail(TEST_DIAGNOSTIC, f"CTest inventory failed: {result.stderr.strip()}")
    try:
        return json.loads(result.stdout), args.build_root
    except json.JSONDecodeError as error:
        fail(TEST_DIAGNOSTIC, f"CTest returned invalid JSON: {error}")


def normalize_closure_ctest_argument(argument: str) -> str:
    normalized = BOUNDARY.normalize_projection_argument(argument)
    if normalized.startswith("-DCMAKE_CPACK_COMMAND="):
        return "-DCMAKE_CPACK_COMMAND=${CPACK}"
    return normalized


def build_final_ctest_evidence(
    repo: Path, raw: dict[str, Any], *, build_root: Path | None
) -> dict[str, Any]:
    _, _, baseline = BOUNDARY.load_frozen_evidence(repo)
    normalized = BOUNDARY.normalize_current_ctest_model(
        raw, source_root=repo, build_root=build_root
    )
    for test in normalized:
        test["command"] = [
            normalize_closure_ctest_argument(argument)
            for argument in test.get("command", [])
        ]
    boundary_summary = BOUNDARY.collect_ctest_boundary(
        raw,
        source_root=repo,
        build_root=build_root,
        baseline_ctest=baseline,
    )
    diagnostics: list[str] = []
    if boundary_summary["forbidden"]:
        diagnostics.append(TEST_DIAGNOSTIC)
    if not boundary_summary["survivors_match"]:
        diagnostics.append(NON_CODEX_DIAGNOSTIC)
    if diagnostics:
        fail(
            diagnostics[0],
            f"final CTest model violates the boundary: {diagnostics}",
        )

    baseline_by_name = {
        test["name"]: test for test in baseline.get("tests", [])
    }
    tests: list[dict[str, Any]] = []
    classifications: dict[str, int] = {}
    for test in normalized:
        name = test["name"]
        if name in baseline_by_name:
            classification = baseline_by_name[name]["classification"]
        else:
            classification = "cutover-added"
        record = {**test, "classification": classification}
        tests.append(record)
        classifications[classification] = (
            classifications.get(classification, 0) + 1
        )
    added_names = sorted(
        test["name"]
        for test in tests
        if test["classification"] == "cutover-added"
    )
    if added_names != ALL_CUTOVER_TEST_NAMES:
        fail(
            TEST_DIAGNOSTIC,
            "final CTest cutover-added inventory changed: "
            f"{added_names}",
        )
    return {
        "format_version": FORMAT_VERSION,
        "kind": "snodec-aisuite-cutover-final-ctest",
        "authority": {
            "base_commit": BOUNDARY.SNODEC_BASE_COMMIT,
            "base_tree": BOUNDARY.SNODEC_BASE_TREE,
            "baseline_evidence_sha256": sha256_bytes(
                canonical_json_bytes(baseline)
            ),
        },
        "summary": {
            "baseline_configured_tests": baseline["summary"][
                "configured_tests"
            ],
            "baseline_codex_owned_removed": baseline["summary"][
                "classifications"
            ]["codex-owned-removed"],
            "baseline_survivors_expected": 172,
            "baseline_survivors_present": boundary_summary[
                "survivor_projection_count"
            ],
            "configured_tests": len(tests),
            "unique_names": len({test["name"] for test in tests}),
            "classifications": dict(sorted(classifications.items())),
            "cutover_added_names": added_names,
            "survivor_projection_sha256": boundary_summary[
                "survivor_projection_sha256"
            ],
            "expected_survivor_projection_sha256": (
                BOUNDARY.SURVIVOR_CTEST_PROJECTION_SHA256
            ),
            "manifest_sha256": sha256_bytes(
                canonical_json_bytes(tests)
            ),
        },
        "tests": tests,
    }


def _contains_raw_absolute_path(value: Any) -> bool:
    if isinstance(value, dict):
        return any(_contains_raw_absolute_path(item) for item in value.values())
    if isinstance(value, list):
        return any(_contains_raw_absolute_path(item) for item in value)
    if not isinstance(value, str):
        return False
    return bool(
        re.search(
            r"(?:^|[\s=:'\"\[(])/(?!/)(?:[^\s,;)\]}]+|$)",
            value,
        )
        or re.search(r"(?i)(?:^|[\s='\",:;(])[a-z]:[\\/]", value)
    )


def validate_downstream_summary(proof: dict[str, Any]) -> dict[str, Any]:
    if (
        proof.get("format_version") != FORMAT_VERSION
        or proof.get("kind") != "snodec-aisuite-downstream-proof"
    ):
        fail(AISUITE_CONSUMER_DIAGNOSTIC, "invalid downstream proof identity")
    authority = proof.get("authority", {})
    if (
        authority.get("aisuite_commit") != BOUNDARY.AISUITE_OWNER_COMMIT
        or authority.get("aisuite_tree") != BOUNDARY.AISUITE_OWNER_TREE
        or authority.get("provenance_snodec_commit")
        != BOUNDARY.SNODEC_BASE_COMMIT
        or authority.get("provenance_snodec_tree")
        != BOUNDARY.SNODEC_BASE_TREE
    ):
        fail(
            AISUITE_CONSUMER_DIAGNOSTIC,
            "downstream proof authority differs from the frozen cutover",
        )
    if proof.get("dependency_direction") != "AISuite -> installed SNode.C":
        fail(
            AISUITE_CONSUMER_DIAGNOSTIC,
            "downstream dependency direction changed",
        )
    clean = proof.get("aisuite_clean", {})
    if clean != {"before": True, "after": True, "modified": False}:
        fail(AISUITE_CONSUMER_DIAGNOSTIC, "AISuite was not read-only")
    configuration = proof.get("configuration", {})
    if (
        configuration.get("AISUITE_BUILD_TESTS") is not True
        or configuration.get("AISUITE_BUILD_APPS") is not True
        or configuration.get("registries_disabled") is not True
        or configuration.get("snodec_prefix") != "${SNODEC_PREFIX}"
        or configuration.get("provenance_source")
        != "${PROVENANCE_SNODEC_SOURCE}"
    ):
        fail(
            AISUITE_CONSUMER_DIAGNOSTIC,
            "downstream configuration did not separate dependency/provenance",
        )
    targets = proof.get("targets", {})
    if sorted(targets) != DOWNSTREAM_TARGETS or not all(targets.values()):
        fail(
            AISUITE_CONSUMER_DIAGNOSTIC,
            "not all required AISuite targets were built",
        )
    focused = proof.get("focused_ctest", {})
    selected = focused.get("selected", [])
    if (
        focused.get("label_expression")
        != AISUITE_FOCUSED_LABEL_EXPRESSION
        or len(selected) != AISUITE_FOCUSED_TEST_COUNT
        or focused.get("selected_count") != AISUITE_FOCUSED_TEST_COUNT
        or focused.get("selected_sha256")
        != AISUITE_FOCUSED_TEST_SHA256
        or string_manifest_sha256(selected)
        != AISUITE_FOCUSED_TEST_SHA256
        or not set(AISUITE_FOCUSED_REQUIRED_TESTS) <= set(selected)
        or focused.get("failed") != 0
        or focused.get("passed", 0) + focused.get("skipped", 0)
        != len(selected)
        or selected != sorted(set(selected))
    ):
        fail(
            AISUITE_CONSUMER_DIAGNOSTIC,
            "focused AISuite CTest result is incomplete",
        )
    checks = proof.get("checks", {})
    if (
        set(checks) != set(DOWNSTREAM_CHECKS)
        or len(checks) != len(DOWNSTREAM_CHECKS)
        or not all(checks.values())
    ):
        fail(
            AISUITE_CONSUMER_DIAGNOSTIC,
            "downstream installed/package/ownership checks are incomplete",
        )
    smoke = proof.get("smoke", {})
    if smoke != {
        "codex-backend --help": True,
        "codex-backend-client --help": True,
    }:
        fail(AISUITE_CONSUMER_DIAGNOSTIC, "application smoke proof changed")
    consumer = proof.get("external_consumer", {})
    if (
        consumer.get("configured") is not True
        or consumer.get("built") is not True
        or consumer.get("ran") is not True
        or consumer.get("installed_header")
        != "<ai/openai/codex/Protocol.h>"
        or consumer.get("targets") != [
            "AISuite::OpenAICodex",
            "AISuite::OpenAICodexBackend",
            "AISuite::OpenAICodexFrontend",
        ]
    ):
        fail(AISUITE_CONSUMER_DIAGNOSTIC, "external consumer proof is incomplete")
    inspections = proof.get("inspections", {})
    if (
        set(inspections) != set(DOWNSTREAM_INSPECTIONS)
        or len(inspections) != len(DOWNSTREAM_INSPECTIONS)
        or not all(inspections.values())
    ):
        fail(AISUITE_CONSUMER_DIAGNOSTIC, "path inspections are incomplete")
    paths = proof.get("paths", {})
    required_path_model = {
        "aisuite_build_snodec_roots": ["${SNODEC_PREFIX}"],
        "consumer_snodec_roots": ["${SNODEC_PREFIX}"],
        "consumer_codex_roots": ["${AISUITE_PREFIX}"],
        "provenance_reads": ["${PROVENANCE_SNODEC_SOURCE}"],
        "production_provenance_hits": [],
        "legacy_snodec_source_build_hits": [],
        "duplicate_codex_ownership_hits": [],
        "unclassified_absolute_paths": [],
    }
    for key, expected in required_path_model.items():
        if paths.get(key) != expected:
            fail(
                AISUITE_CONSUMER_DIAGNOSTIC,
                f"downstream path role mismatch for {key}",
            )
    aisuite_codex_roots = paths.get("aisuite_build_codex_roots", [])
    if aisuite_codex_roots != ["${AISUITE_BUILD}", "${AISUITE_SOURCE}"]:
        fail(
            AISUITE_CONSUMER_DIAGNOSTIC,
            "AISuite build Codex roots are not AISuite-owned",
        )
    clean_install = proof.get("clean_snodec_install", {})
    if clean_install != {
        "codex_artifacts": [],
        "file_count": 841,
        "manifest_sha256": (
            "ffac38e0e5514a6ada2c72833a7b2f5e2fcc6a1b0f5ac86380702108b40e7944"
        ),
    }:
        fail(
            AISUITE_CONSUMER_DIAGNOSTIC,
            "downstream clean SNode.C install proof changed",
        )
    if _contains_raw_absolute_path(proof):
        fail(
            AISUITE_CONSUMER_DIAGNOSTIC,
            "downstream proof contains an unnormalized local path",
        )
    return copy.deepcopy(proof)


def _read_downstream_proof(path: Path) -> dict[str, Any]:
    value = read_canonical_json(path, code=AISUITE_CONSUMER_DIAGNOSTIC)
    if not isinstance(value, dict):
        fail(AISUITE_CONSUMER_DIAGNOSTIC, "downstream proof is not an object")
    return validate_downstream_summary(value)


def _artifact_namespace(args: argparse.Namespace) -> SimpleNamespace:
    return SimpleNamespace(
        repo_root=args.repo_root,
        aisuite_source=None,
        ctest_json=args.ctest_json,
        build_root=args.build_root,
        install_manifest=args.install_manifest,
        source_package_manifest=args.source_package_manifest,
        binary_package_manifest=args.binary_package_manifest,
        snodec_config=args.snodec_config,
        cpack_config=args.cpack_config,
        target_inventory=args.target_inventory,
        install_root=args.install_root,
    )


def _artifact_manifests(
    args: argparse.Namespace, start: dict[str, Any]
) -> tuple[dict[str, list[str]], dict[str, Any]]:
    if args.install_manifest is not None:
        install = read_manifest(
            args.install_manifest, description="install manifest"
        )
        if args.install_root is not None:
            scanned_install = scan_tree_manifest(args.install_root)
            if install != scanned_install:
                fail(
                    INSTALL_DIAGNOSTIC,
                    "install manifest differs from the fresh install root",
                )
    elif args.install_root is not None:
        install = scan_tree_manifest(args.install_root)
    else:
        fail(INSTALL_DIAGNOSTIC, "an install manifest or root is required")
    if args.source_package_manifest is None:
        fail(PACKAGE_DIAGNOSTIC, "a source-package manifest is required")
    if args.binary_package_manifest is None:
        fail(PACKAGE_DIAGNOSTIC, "a binary-package manifest is required")
    source = read_manifest(
        args.source_package_manifest, description="source-package manifest"
    )
    binary = read_manifest(
        args.binary_package_manifest, description="binary-package manifest"
    )
    expected = expected_artifact_paths(start)
    observed = {
        "install": install,
        "source": source,
        "binary": binary,
    }
    for name, expected_name in [
        ("install", "install_final"),
        ("source", "source_final"),
        ("binary", "binary_final"),
    ]:
        missing = sorted(set(expected[expected_name]) - set(observed[name]))
        if missing:
            fail(
                NON_CODEX_DIAGNOSTIC,
                f"{name} artifact lost preserved paths: {missing}",
            )
        if observed[name] != expected[expected_name]:
            fail(
                PACKAGE_DIAGNOSTIC,
                f"{name} artifact differs from the exact cutover manifest",
            )
    if install != binary:
        fail(
            PACKAGE_DIAGNOSTIC,
            "complete binary-package manifest differs from fresh install",
        )
    manifests = {"install": install, "source": source, "binary": binary}
    comparisons = {
        "install": manifest_comparison(
            start["install"]["paths"],
            install,
            expected_preserved=expected["install_preserved"],
        ),
        "source_package": manifest_comparison(
            start["source_package"]["paths"],
            source,
            expected_preserved=expected["source_preserved"],
        ),
        "binary_package": manifest_comparison(
            start["binary_package"]["paths"],
            binary,
            expected_preserved=expected["binary_preserved"],
        ),
    }
    for name, comparison in comparisons.items():
        if comparison["missing_preserved"]:
            fail(
                NON_CODEX_DIAGNOSTIC,
                f"{name} lost preserved baseline paths: "
                f"{comparison['missing_preserved']}",
            )
    return manifests, comparisons


def _baseline_logical_targets(start: dict[str, Any]) -> list[str]:
    source = "".join(
        f"{target}: phony\n"
        for target in start["target_inventory"]["configured_targets"]
    )
    return BOUNDARY.extract_target_names_text(source)


def canonicalize_environment_conditional_targets(
    targets: Sequence[str], build_root: Path
) -> list[str]:
    """Restore the canonical target model for a cache-proven optional tool.

    The project creates ``doc`` and ``doc-fast`` only when Doxygen is
    available. GitHub's normal runner intentionally does not install Doxygen,
    while the frozen audit host does. Treat the pair as logically present only
    when CMake's cache proves that Doxygen was unavailable; every other target
    remains exact.
    """

    cache = build_root / "CMakeCache.txt"
    try:
        cache_text = cache.read_text(encoding="utf-8")
    except OSError as error:
        fail(
            BUILD_DIAGNOSTIC,
            f"cannot read configured CMake cache {cache}: {error}",
        )
    doxygen_unavailable = re.search(
        r"^DOXYGEN_EXECUTABLE(?::[^=]+)?=DOXYGEN_EXECUTABLE-NOTFOUND$",
        cache_text,
        flags=re.MULTILINE,
    )
    normalized = set(targets)
    present = normalized.intersection(DOXYGEN_OPTIONAL_TARGETS)
    if doxygen_unavailable:
        if present and present != set(DOXYGEN_OPTIONAL_TARGETS):
            fail(
                NON_CODEX_DIAGNOSTIC,
                "Doxygen optional target pair is only partially configured",
            )
        normalized.update(DOXYGEN_OPTIONAL_TARGETS)
    return sorted(normalized)


def _configured_summary(
    args: argparse.Namespace,
    boundary_model: dict[str, Any],
    start: dict[str, Any],
) -> dict[str, Any]:
    if args.snodec_config is None:
        fail(BUILD_DIAGNOSTIC, "generated snodecConfig.cmake is required")
    if args.cpack_config is None:
        fail(PACKAGE_DIAGNOSTIC, "generated CPackConfig.cmake is required")
    if args.target_inventory is None:
        fail(BUILD_DIAGNOSTIC, "configured target inventory is required")
    if args.build_root is None:
        fail(
            BUILD_DIAGNOSTIC,
            "configured target inventory requires --build-root",
        )
    components = BOUNDARY.extract_supported_components(
        args.snodec_config.read_text(encoding="utf-8"),
        description=args.snodec_config.as_posix(),
    )
    cpack_components, apps_dependencies, dependencies = (
        BOUNDARY.extract_cpack_model(
            args.cpack_config.read_text(encoding="utf-8"),
            description=args.cpack_config.as_posix(),
        )
    )
    build_root = args.build_root.resolve().as_posix()
    observed_targets = sorted(
        target.replace(build_root, "${SNODEC_BUILD}")
        for target in BOUNDARY.extract_target_names(args.target_inventory)
    )
    targets = canonicalize_environment_conditional_targets(
        observed_targets, args.build_root.resolve()
    )
    baseline_targets = _baseline_logical_targets(start)
    target_comparison = manifest_comparison(
        baseline_targets,
        targets,
        expected_preserved=targets,
    )
    return {
        "supported_components": components,
        "supported_components_sha256": string_manifest_sha256(components),
        "cpack_components": cpack_components,
        "cpack_components_sha256": string_manifest_sha256(cpack_components),
        "apps_dependencies": apps_dependencies,
        "component_dependencies": dependencies,
        "targets": targets,
        "targets_sha256": string_manifest_sha256(targets),
        "target_comparison": target_comparison,
        "boundary": {
            "paths": boundary_model["paths"],
            "build": boundary_model["build"],
            "install": boundary_model["install"],
            "package": boundary_model["package"],
            "tests": boundary_model["tests"],
            "preservation": boundary_model["preservation"],
        },
    }


def _history_policy() -> dict[str, Any]:
    return {
        "base_commit": BOUNDARY.SNODEC_BASE_COMMIT,
        "base_tree": BOUNDARY.SNODEC_BASE_TREE,
        "subjects": CUTOVER_SUBJECTS,
        "pinned_prefix": PINNED_CUTOVER_PREFIX,
        "commit_count": 4,
        "merge_subject_pattern": MERGE_SUBJECT_PATTERN,
        "supported_topologies": [
            "unmerged-fourth-commit",
            "normal-github-merge",
            "later-merge-descendant",
        ],
        "normal_merge_parent_roles": ["base", "fourth-cutover-commit"],
        "normal_merge_tree_equals_fourth_commit": True,
        "fourth_commit_sha_embedded": False,
        "fourth_commit_tree_embedded": False,
        "closure_new_paths": CLOSURE_NEW_PATHS,
        "closure_registration_path": CLOSURE_REGISTRATION_PATH,
        "closure_source_package_path": CLOSURE_SOURCE_PACKAGE_PATH,
    }


def _shared_policy_summary(
    repo: Path, start: dict[str, Any], policy: dict[str, Any]
) -> dict[str, Any]:
    api_path = repo / "tests/policy/log/LoggingApiSurfacePolicyTest.cpp"
    workflow_path = repo / ".github/workflows/ci.yml"
    workflow_policy_path = (
        repo / "tests/policy/ci/CiWorkflowPathsPolicyTest.cpp"
    )
    api = api_path.read_text(encoding="utf-8")
    workflow = workflow_path.read_text(encoding="utf-8")
    workflow_policy = workflow_policy_path.read_text(encoding="utf-8")
    removed_api_paths = start["policies"]["logging_api_surface"]["scan_paths"]
    removed_api_identifiers = start["policies"]["logging_api_surface"][
        "forbidden_identifiers"
    ]
    removed_ci_path = start["policies"]["ci_paths"]["path"]
    return {
        "semantic_logger": {
            "baseline_discovered": 81,
            "baseline_allowlisted": 81,
            "final_discovered": policy["semantic_logger"]["expected"],
            "final_allowlisted": policy["semantic_logger"][
                "allowlist_entries"
            ],
            "removed_entries": BOUNDARY.CODEX_SEMANTIC_ENTRIES,
        },
        "logging_api": {
            "removed_scan_paths": removed_api_paths,
            "removed_identifiers": removed_api_identifiers,
            "remaining_path_hits": sorted(
                path for path in removed_api_paths if path in api
            ),
            "remaining_identifier_hits": sorted(
                identifier
                for identifier in removed_api_identifiers
                if f'"{identifier}"' in api
            ),
            "mqtt_http_policy_preserved": (
                "Mqtt.h" in api and "Request.h" in api
            ),
        },
        "ci_paths": {
            "removed_path": removed_ci_path,
            "workflow_occurrences": workflow.count(removed_ci_path),
            "policy_occurrences": workflow_policy.count(removed_ci_path),
            "required_families": REQUIRED_CI_PATH_FAMILIES,
            "required_families_present": all(
                family in workflow and family in workflow_policy
                for family in REQUIRED_CI_PATH_FAMILIES
            ),
        },
        "residue": policy["residue"],
    }


def _skipped_campaigns() -> list[dict[str, str]]:
    return [
        {
            "campaign": "credential-bearing live Codex integration",
            "reason": "not deterministic and may consume credentials/quota",
            "residual_risk": "authenticated service behavior is not exercised",
            "coverage": "AISuite follow-up CI or an explicit credentialed run",
        },
        {
            "campaign": "stress, soak, fuzz, and benchmark campaigns",
            "reason": "unrelated to the ownership cutover",
            "residual_risk": "long-duration behavior is unchanged but not rerun",
            "coverage": "normal project campaigns",
        },
        {
            "campaign": "book build and unrelated MQTTSuite tests",
            "reason": "explicitly outside this pull request",
            "residual_risk": "separate documentation/project integration",
            "coverage": "release follow-up",
        },
    ]


def build_closure_outputs(
    args: argparse.Namespace,
    *,
    downstream_summary: dict[str, Any] | None = None,
) -> tuple[dict[str, Any], dict[str, Any]]:
    repo = args.repo_root.resolve()
    start, plan, baseline_ctest = BOUNDARY.load_frozen_evidence(repo)
    raw_ctest, build_root = _ctest_raw(args, repo)
    final_ctest = build_final_ctest_evidence(
        repo, raw_ctest, build_root=build_root
    )
    if downstream_summary is None:
        if args.downstream_proof is None:
            fail(
                AISUITE_CONSUMER_DIAGNOSTIC,
                "an explicit normalized downstream proof is required",
            )
        downstream_summary = _read_downstream_proof(args.downstream_proof)
    else:
        downstream_summary = validate_downstream_summary(downstream_summary)

    artifact_args = _artifact_namespace(args)
    boundary_model = BOUNDARY.check_boundary(
        artifact_args, package_mode=False
    )
    _, comparisons = _artifact_manifests(args, start)
    configured = _configured_summary(args, boundary_model, start)
    policy = boundary_model["policy"]
    migration = boundary_model["migration"]
    identity = boundary_model["identity"]

    closure = {
        "format_version": FORMAT_VERSION,
        "kind": "snodec-aisuite-cutover-closure",
        "authority": {
            "snodec": {
                "repository": BOUNDARY.SNODEC_REPOSITORY,
                "base_commit": BOUNDARY.SNODEC_BASE_COMMIT,
                "base_tree": BOUNDARY.SNODEC_BASE_TREE,
            },
            "aisuite": {
                "repository": BOUNDARY.AISUITE_REPOSITORY,
                "owner_commit": BOUNDARY.AISUITE_OWNER_COMMIT,
                "owner_tree": BOUNDARY.AISUITE_OWNER_TREE,
                "subject": BOUNDARY.AISUITE_OWNER_SUBJECT,
                "parents": BOUNDARY.AISUITE_OWNER_PARENTS,
            },
        },
        "history": _history_policy(),
        "start_state": {
            "evidence_sha256": sha256_bytes(canonical_json_bytes(start)),
            "plan_sha256": sha256_bytes(canonical_json_bytes(plan)),
            "baseline_ctest_sha256": sha256_bytes(
                canonical_json_bytes(baseline_ctest)
            ),
            "deleted_path_count": start["removal_inventory"]["total_count"],
            "deleted_path_manifest_sha256": start["removal_inventory"][
                "manifest_sha256"
            ],
            "intentional_residue_removals": [".gitattributes"],
            "removed_targets": BOUNDARY.REMOVED_TARGETS,
            "removed_components": BOUNDARY.REMOVED_COMPONENTS,
            "removed_applications": BOUNDARY.REMOVED_APPLICATIONS,
            "removed_public_headers": start["legacy_surface"][
                "public_header_count"
            ],
            "removed_ctests": baseline_ctest["summary"][
                "classifications"
            ]["codex-owned-removed"],
        },
        "final_ctest": {
            "document": f"docs/migrations/{FINAL_CTEST_FILE}",
            "sha256": sha256_bytes(canonical_json_bytes(final_ctest)),
            "summary": final_ctest["summary"],
        },
        "comparisons": comparisons,
        "configured": configured,
        "policy": _shared_policy_summary(repo, start, policy),
        "identity": identity,
        "dependency_direction": boundary_model["dependency_direction"],
        "migration": {
            **migration,
            "readme_aisuite_reference": migration["requirements"][
                "readme:aisuite-project"
            ],
            "historical_references_classified": all(
                migration["requirements"][key]
                for key in [
                    "historical:phase-2",
                    "historical:phase-3",
                    "historical:phase-3-accounting",
                    "historical:test-suite-consolidation",
                ]
            ),
        },
        "downstream_aisuite": downstream_summary,
        "package_safe": {
            "git_required": False,
            "network_required": False,
            "aisuite_checkout_required": False,
            "parent_tree_required": False,
            "cmake_registry_required": False,
            "required_closure_paths": CLOSURE_NEW_PATHS,
        },
        "generation": {
            "canonical_json": True,
            "timestamps_embedded": False,
            "absolute_local_paths_embedded": False,
            "second_pass_byte_identical": True,
        },
        "skipped_campaigns": _skipped_campaigns(),
    }
    diagnostics = closure_model_diagnostics(closure, final_ctest)
    if diagnostics:
        fail(
            diagnostics[0],
            f"generated closure model is invalid: {diagnostics}",
        )
    validate_closure_against_frozen_authority(repo, closure, final_ctest)
    validate_closure_against_current_source(repo, closure)
    return final_ctest, closure


def closure_model_diagnostics(
    closure: dict[str, Any], final_ctest: dict[str, Any]
) -> list[str]:
    diagnostics: list[str] = []
    authority = closure.get("authority", {})
    snodec = authority.get("snodec", {})
    aisuite = authority.get("aisuite", {})
    if (
        closure.get("format_version") != FORMAT_VERSION
        or closure.get("kind") != "snodec-aisuite-cutover-closure"
        or snodec.get("base_commit") != BOUNDARY.SNODEC_BASE_COMMIT
        or snodec.get("base_tree") != BOUNDARY.SNODEC_BASE_TREE
    ):
        diagnostics.append(HISTORY_DIAGNOSTIC)
    if (
        aisuite.get("owner_commit") != BOUNDARY.AISUITE_OWNER_COMMIT
        or aisuite.get("owner_tree") != BOUNDARY.AISUITE_OWNER_TREE
        or aisuite.get("subject") != BOUNDARY.AISUITE_OWNER_SUBJECT
        or aisuite.get("parents") != BOUNDARY.AISUITE_OWNER_PARENTS
    ):
        diagnostics.append(MIGRATION_DIAGNOSTIC)
    if closure.get("history") != _history_policy():
        diagnostics.append(HISTORY_DIAGNOSTIC)
    final_reference = closure.get("final_ctest", {})
    if (
        final_reference.get("document")
        != f"docs/migrations/{FINAL_CTEST_FILE}"
        or final_reference.get("sha256")
        != sha256_bytes(canonical_json_bytes(final_ctest))
        or final_reference.get("summary") != final_ctest.get("summary")
        or final_ctest.get("kind")
        != "snodec-aisuite-cutover-final-ctest"
        or final_ctest.get("summary", {}).get(
            "survivor_projection_sha256"
        )
        != BOUNDARY.SURVIVOR_CTEST_PROJECTION_SHA256
    ):
        diagnostics.append(TEST_DIAGNOSTIC)
    comparisons = closure.get("comparisons", {})
    for name in ["install", "source_package", "binary_package"]:
        comparison = comparisons.get(name, {})
        if comparison.get("missing_preserved"):
            diagnostics.append(NON_CODEX_DIAGNOSTIC)
        if not isinstance(comparison.get("final_sha256"), str):
            diagnostics.append(PACKAGE_DIAGNOSTIC)
    configured = closure.get("configured", {})
    if any(
        component in configured.get("supported_components", [])
        for component in BOUNDARY.REMOVED_COMPONENTS
    ) or any(
        target in configured.get("targets", [])
        for target in BOUNDARY.REMOVED_TARGETS
    ):
        diagnostics.append(BUILD_DIAGNOSTIC)
    if any(
        component in configured.get("cpack_components", [])
        for component in BOUNDARY.REMOVED_COMPONENTS
    ) or "ai-openai-codex-frontend" in configured.get(
        "apps_dependencies", []
    ):
        diagnostics.append(PACKAGE_DIAGNOSTIC)
    semantic = closure.get("policy", {}).get("semantic_logger", {})
    if (
        semantic.get("baseline_discovered") != 81
        or semantic.get("baseline_allowlisted") != 81
        or semantic.get("final_discovered") != 77
        or semantic.get("final_allowlisted") != 77
        or semantic.get("removed_entries") != BOUNDARY.CODEX_SEMANTIC_ENTRIES
    ):
        diagnostics.append(POLICY_DIAGNOSTIC)
    logging_api = closure.get("policy", {}).get("logging_api", {})
    ci_paths = closure.get("policy", {}).get("ci_paths", {})
    if (
        logging_api.get("remaining_path_hits")
        or logging_api.get("remaining_identifier_hits")
        or logging_api.get("removed_scan_paths")
        != BOUNDARY.CODEX_LOGGING_API_PATHS
        or logging_api.get("removed_identifiers")
        != BOUNDARY.CODEX_LOGGING_API_IDENTIFIERS
        or logging_api.get("mqtt_http_policy_preserved") is not True
        or ci_paths.get("removed_path") != "docs/ai/openai/codex/**"
        or ci_paths.get("workflow_occurrences") != 0
        or ci_paths.get("policy_occurrences") != 0
        or ci_paths.get("required_families")
        != REQUIRED_CI_PATH_FAMILIES
        or ci_paths.get("required_families_present") is not True
        or closure.get("policy", {}).get("residue")
    ):
        diagnostics.append(POLICY_DIAGNOSTIC)
    identity = closure.get("identity", {})
    if (
        identity.get("project_version") != CUTOVER_PROJECT_VERSION
        or identity.get("soversion") != CUTOVER_SOVERSION
    ):
        diagnostics.append(SOVERSION_DIAGNOSTIC)
    dependency = closure.get("dependency_direction", {})
    if (
        dependency.get("expected") != "AISuite -> installed SNode.C"
        or dependency.get("snodec_depends_on_aisuite") is not False
        or dependency.get("production_dependency_hits")
    ):
        diagnostics.append(DEPENDENCY_DIAGNOSTIC)
    migration = closure.get("migration", {})
    migration_requirements = migration.get("requirements", {})
    if (
        migration.get("complete") is not True
        or sorted(migration_requirements) != MIGRATION_REQUIREMENT_KEYS
        or not all(migration_requirements.values())
        or migration.get("aisuite_owner_commit")
        != BOUNDARY.AISUITE_OWNER_COMMIT
        or migration.get("aisuite_owner_tree") != BOUNDARY.AISUITE_OWNER_TREE
        or migration.get("readme_aisuite_reference") is not True
        or migration.get("historical_references_classified") is not True
    ):
        diagnostics.append(MIGRATION_DIAGNOSTIC)
    try:
        validate_downstream_summary(closure.get("downstream_aisuite", {}))
    except BOUNDARY.CutoverError as error:
        diagnostics.append(error.code)
    package_safe = closure.get("package_safe", {})
    if (
        package_safe.get("required_closure_paths") != CLOSURE_NEW_PATHS
        or any(
            package_safe.get(key) is not False
            for key in [
                "git_required",
                "network_required",
                "aisuite_checkout_required",
                "parent_tree_required",
                "cmake_registry_required",
            ]
        )
    ):
        diagnostics.append(PACKAGE_DIAGNOSTIC)
    generation = closure.get("generation", {})
    if (
        generation.get("canonical_json") is not True
        or generation.get("timestamps_embedded") is not False
        or generation.get("absolute_local_paths_embedded") is not False
        or generation.get("second_pass_byte_identical") is not True
    ):
        diagnostics.append(SECOND_PASS_DIAGNOSTIC)
    if _contains_raw_absolute_path(closure):
        diagnostics.append(AISUITE_CONSUMER_DIAGNOSTIC)
    return list(dict.fromkeys(diagnostics))


def _validate_final_ctest_internal(
    final_ctest: dict[str, Any],
    baseline_ctest: dict[str, Any],
) -> None:
    tests = final_ctest.get("tests", [])
    summary = final_ctest.get("summary", {})
    authority = final_ctest.get("authority", {})
    if (
        final_ctest.get("format_version") != FORMAT_VERSION
        or final_ctest.get("kind")
        != "snodec-aisuite-cutover-final-ctest"
        or authority.get("base_commit") != BOUNDARY.SNODEC_BASE_COMMIT
        or authority.get("base_tree") != BOUNDARY.SNODEC_BASE_TREE
        or authority.get("baseline_evidence_sha256")
        != sha256_bytes(canonical_json_bytes(baseline_ctest))
    ):
        fail(TEST_DIAGNOSTIC, "final CTest authority changed")
    if not isinstance(tests, list):
        fail(TEST_DIAGNOSTIC, "final CTest evidence tests are not a list")
    names = [test.get("name") for test in tests]
    if (
        tests != sorted(tests, key=lambda test: test.get("name", ""))
        or any(not isinstance(name, str) or not name for name in names)
        or len(names) != len(set(names))
        or summary.get("configured_tests") != len(tests)
        or summary.get("unique_names") != len(set(names))
        or summary.get("manifest_sha256")
        != sha256_bytes(canonical_json_bytes(tests))
    ):
        fail(TEST_DIAGNOSTIC, "final CTest inventory/hash is inconsistent")
    classifications: dict[str, int] = {}
    added_names: list[str] = []
    for test in tests:
        classification = test.get("classification")
        if not isinstance(classification, str):
            fail(TEST_DIAGNOSTIC, "final CTest classification is missing")
        classifications[classification] = (
            classifications.get(classification, 0) + 1
        )
        if classification == "cutover-added":
            added_names.append(test["name"])
        properties = {
            prop.get("name"): prop.get("value")
            for prop in test.get("properties", [])
        }
        labels = [str(label).lower() for label in properties.get("LABELS", [])]
        serialized = json.dumps(
            {
                key: value
                for key, value in test.items()
                if key != "classification"
            },
            sort_keys=True,
        ).lower()
        if (
            test["name"].startswith("Codex")
            or "codex" in labels
            or any(label.startswith("phase-a1-") for label in labels)
            or "codex" in serialized
        ):
            fail(TEST_DIAGNOSTIC, f"legacy CTest residue: {test['name']}")
    baseline_by_name = {
        test["name"]: test for test in baseline_ctest.get("tests", [])
    }
    survivor_projection: list[dict[str, Any]] = []
    for test in tests:
        baseline_test = baseline_by_name.get(test["name"])
        if baseline_test is None:
            continue
        expected_classification = baseline_test.get("classification")
        if expected_classification == "codex-owned-removed":
            fail(
                TEST_DIAGNOSTIC,
                f"removed baseline CTest returned: {test['name']}",
            )
        if test.get("classification") != expected_classification:
            fail(
                TEST_DIAGNOSTIC,
                f"baseline CTest classification changed: {test['name']}",
            )
        survivor_projection.append(
            BOUNDARY.ctest_projection_record(test)
        )
    survivor_projection.sort(key=lambda record: record["name"])
    survivor_hash = BOUNDARY.ctest_projection_hash(survivor_projection)
    tests_by_name = {test["name"]: test for test in tests}
    expected_closure_tests = {
        "AISuiteCutoverHistoryTopologyTest": {
            "classification": "cutover-added",
            "command": [
                "${PYTHON}",
                "${SNODEC_SOURCE}/tests/ownership/"
                "AISuiteCutoverClosureTest.py",
                "--tool",
                "${SNODEC_SOURCE}/tools/ownership/"
                "verify_aisuite_cutover_closure.py",
                "--suite",
                "topology",
            ],
            "name": "AISuiteCutoverHistoryTopologyTest",
            "origins": [
                "${SNODEC_SOURCE}/tests/ownership/CMakeLists.txt"
            ],
            "properties": [
                {
                    "name": "ENVIRONMENT",
                    "value": ["PYTHONDONTWRITEBYTECODE=1"],
                },
                {
                    "name": "LABELS",
                    "value": [
                        "architecture",
                        "cutover",
                        "ownership",
                        "package",
                        "policy",
                    ],
                },
                {"name": "TIMEOUT", "value": 120.0},
                {
                    "name": "WORKING_DIRECTORY",
                    "value": "${SNODEC_BUILD}/tests/ownership",
                },
            ],
        },
        "AISuiteCutoverClosureTest": {
            "classification": "cutover-added",
            "command": [
                "${PYTHON}",
                "${SNODEC_SOURCE}/tests/ownership/"
                "AISuiteCutoverClosureTest.py",
                "--tool",
                "${SNODEC_SOURCE}/tools/ownership/"
                "verify_aisuite_cutover_closure.py",
                "--suite",
                "closure",
                "--repo-root",
                "${SNODEC_SOURCE}",
                "--build-root",
                "${SNODEC_BUILD}",
            ],
            "name": "AISuiteCutoverClosureTest",
            "origins": [
                "${SNODEC_SOURCE}/tests/ownership/CMakeLists.txt"
            ],
            "properties": [
                {
                    "name": "DEPENDS",
                    "value": [
                        "AISuiteCutoverHistoryTopologyTest",
                        "AISuiteOwnershipBoundaryPolicyTest",
                        "AISuiteCutoverMutationTest",
                        "StagedInstalledConsumerTest",
                        "AISuiteCutoverSourcePackageTest",
                        "AISuiteCutoverBinaryPackageTest",
                    ],
                },
                {
                    "name": "ENVIRONMENT",
                    "value": ["PYTHONDONTWRITEBYTECODE=1"],
                },
                {
                    "name": "LABELS",
                    "value": [
                        "architecture",
                        "cutover",
                        "ownership",
                        "package",
                        "policy",
                    ],
                },
                {"name": "TIMEOUT", "value": 180.0},
                {
                    "name": "WORKING_DIRECTORY",
                    "value": "${SNODEC_BUILD}/tests/ownership",
                },
            ],
        },
    }
    if (
        summary.get("classifications")
        != dict(sorted(classifications.items()))
        or summary.get("cutover_added_names") != sorted(added_names)
        or sorted(added_names) != ALL_CUTOVER_TEST_NAMES
        or len(tests) != 180
        or classifications
        != {
            "cutover-added": 8,
            "non-codex-preserved": 168,
            "shared-codex-branch-removed": 4,
        }
        or summary.get("baseline_survivors_expected") != 172
        or summary.get("baseline_survivors_present") != 172
        or summary.get("baseline_codex_owned_removed") != 128
        or summary.get("baseline_configured_tests") != 300
        or len(survivor_projection) != 172
        or survivor_hash != BOUNDARY.SURVIVOR_CTEST_PROJECTION_SHA256
        or summary.get("survivor_projection_sha256")
        != survivor_hash
        or summary.get("expected_survivor_projection_sha256")
        != BOUNDARY.SURVIVOR_CTEST_PROJECTION_SHA256
        or any(
            tests_by_name.get(name) != expected
            for name, expected in expected_closure_tests.items()
        )
    ):
        fail(TEST_DIAGNOSTIC, "final CTest accounting changed")
    if _contains_raw_absolute_path(final_ctest):
        fail(TEST_DIAGNOSTIC, "final CTest evidence contains a local path")


def _validate_comparison_record(
    comparison: dict[str, Any],
    *,
    baseline: Sequence[str],
    expected_preserved: Sequence[str],
    expected_final: Sequence[str],
    description: str,
) -> None:
    removed = comparison.get("removed", [])
    added = comparison.get("added", [])
    if (
        not isinstance(removed, list)
        or not isinstance(added, list)
        or removed != sorted(set(removed))
        or added != sorted(set(added))
    ):
        fail(PACKAGE_DIAGNOSTIC, f"{description} delta is not canonical")
    baseline_set = set(baseline)
    removed_set = set(removed)
    added_set = set(added)
    if not removed_set <= baseline_set or added_set & baseline_set:
        fail(PACKAGE_DIAGNOSTIC, f"{description} delta is not a true delta")
    reconstructed = sorted((baseline_set - removed_set) | added_set)
    if reconstructed != sorted(expected_final):
        fail(
            PACKAGE_DIAGNOSTIC,
            f"{description} final manifest differs from the exact authority",
        )
    missing = sorted(set(expected_preserved) - set(reconstructed))
    expected = {
        "baseline_count": len(baseline),
        "baseline_sha256": string_manifest_sha256(baseline),
        "final_count": len(reconstructed),
        "final_sha256": string_manifest_sha256(reconstructed),
        "removed_count": len(removed),
        "removed_sha256": string_manifest_sha256(removed),
        "added_count": len(added),
        "added_sha256": string_manifest_sha256(added),
        "missing_preserved": missing,
        "preserved_count": len(
            set(expected_preserved) & set(reconstructed)
        ),
        "preserved_sha256": string_manifest_sha256(
            set(expected_preserved) & set(reconstructed)
        ),
    }
    for key, value in expected.items():
        if comparison.get(key) != value:
            fail(
                PACKAGE_DIAGNOSTIC,
                f"{description} comparison field {key} is inconsistent",
            )
    if missing:
        fail(
            NON_CODEX_DIAGNOSTIC,
            f"{description} lost preserved paths: {missing}",
        )


def validate_closure_against_frozen_authority(
    repo: Path,
    closure: dict[str, Any],
    final_ctest: dict[str, Any],
) -> None:
    start, plan, baseline_ctest = BOUNDARY.load_frozen_evidence(repo)
    _validate_final_ctest_internal(final_ctest, baseline_ctest)
    expected_authority = {
        "snodec": {
            "repository": BOUNDARY.SNODEC_REPOSITORY,
            "base_commit": BOUNDARY.SNODEC_BASE_COMMIT,
            "base_tree": BOUNDARY.SNODEC_BASE_TREE,
        },
        "aisuite": {
            "repository": BOUNDARY.AISUITE_REPOSITORY,
            "owner_commit": BOUNDARY.AISUITE_OWNER_COMMIT,
            "owner_tree": BOUNDARY.AISUITE_OWNER_TREE,
            "subject": BOUNDARY.AISUITE_OWNER_SUBJECT,
            "parents": BOUNDARY.AISUITE_OWNER_PARENTS,
        },
    }
    if closure.get("authority") != expected_authority:
        fail(MIGRATION_DIAGNOSTIC, "closure ownership authority changed")

    start_state = closure.get("start_state", {})
    expected_start_state = {
        "evidence_sha256": sha256_bytes(canonical_json_bytes(start)),
        "plan_sha256": sha256_bytes(canonical_json_bytes(plan)),
        "baseline_ctest_sha256": sha256_bytes(
            canonical_json_bytes(baseline_ctest)
        ),
        "deleted_path_count": start["removal_inventory"]["total_count"],
        "deleted_path_manifest_sha256": start["removal_inventory"][
            "manifest_sha256"
        ],
        "intentional_residue_removals": BOUNDARY.INTENTIONAL_RESIDUE_REMOVALS,
        "removed_targets": BOUNDARY.REMOVED_TARGETS,
        "removed_components": BOUNDARY.REMOVED_COMPONENTS,
        "removed_applications": BOUNDARY.REMOVED_APPLICATIONS,
        "removed_public_headers": start["legacy_surface"][
            "public_header_count"
        ],
        "removed_ctests": baseline_ctest["summary"]["classifications"][
            "codex-owned-removed"
        ],
    }
    if start_state != expected_start_state:
        fail(MIGRATION_DIAGNOSTIC, "closure start-state authority changed")

    policy = closure.get("policy", {})
    semantic = policy.get("semantic_logger", {})
    logging_api = policy.get("logging_api", {})
    ci_paths = policy.get("ci_paths", {})
    frozen_semantic = start["policies"]["parameterless_semantic_logger"]
    frozen_logging_api = start["policies"]["logging_api_surface"]
    frozen_ci = start["policies"]["ci_paths"]
    if (
        semantic.get("baseline_discovered") != frozen_semantic["discovered"]
        or semantic.get("baseline_allowlisted")
        != frozen_semantic["allowlisted"]
        or semantic.get("final_discovered")
        != frozen_semantic["post_cutover_expected"]["discovered"]
        or semantic.get("final_allowlisted")
        != frozen_semantic["post_cutover_expected"]["allowlisted"]
        or semantic.get("removed_entries")
        != frozen_semantic["codex_entries"]
        or logging_api.get("removed_scan_paths")
        != frozen_logging_api["scan_paths"]
        or logging_api.get("removed_identifiers")
        != frozen_logging_api["forbidden_identifiers"]
        or logging_api.get("remaining_path_hits")
        or logging_api.get("remaining_identifier_hits")
        or logging_api.get("mqtt_http_policy_preserved") is not True
        or ci_paths.get("removed_path") != frozen_ci["path"]
        or ci_paths.get("workflow_occurrences") != 0
        or ci_paths.get("policy_occurrences") != 0
        or ci_paths.get("required_families")
        != REQUIRED_CI_PATH_FAMILIES
        or ci_paths.get("required_families_present") is not True
        or policy.get("residue")
    ):
        fail(POLICY_DIAGNOSTIC, "closure logging/CI policy authority changed")

    migration = closure.get("migration", {})
    migration_requirements = migration.get("requirements", {})
    if (
        sorted(migration_requirements) != MIGRATION_REQUIREMENT_KEYS
        or not all(migration_requirements.values())
        or migration.get("document") != BOUNDARY.MIGRATION_DOCUMENT
        or migration.get("complete") is not True
        or migration.get("aisuite_owner_commit")
        != BOUNDARY.AISUITE_OWNER_COMMIT
        or migration.get("aisuite_owner_tree")
        != BOUNDARY.AISUITE_OWNER_TREE
        or migration.get("readme_aisuite_reference") is not True
        or migration.get("historical_references_classified") is not True
    ):
        fail(MIGRATION_DIAGNOSTIC, "closure migration authority changed")

    expected_artifacts = expected_artifact_paths(start)
    comparisons = closure.get("comparisons", {})
    if sorted(comparisons) != [
        "binary_package",
        "install",
        "source_package",
    ]:
        fail(PACKAGE_DIAGNOSTIC, "closure comparison inventory changed")
    _validate_comparison_record(
        comparisons.get("install", {}),
        baseline=start["install"]["paths"],
        expected_preserved=expected_artifacts["install_preserved"],
        expected_final=expected_artifacts["install_final"],
        description="install",
    )
    _validate_comparison_record(
        comparisons.get("source_package", {}),
        baseline=start["source_package"]["paths"],
        expected_preserved=expected_artifacts["source_preserved"],
        expected_final=expected_artifacts["source_final"],
        description="source package",
    )
    _validate_comparison_record(
        comparisons.get("binary_package", {}),
        baseline=start["binary_package"]["paths"],
        expected_preserved=expected_artifacts["binary_preserved"],
        expected_final=expected_artifacts["binary_final"],
        description="binary package",
    )

    configured = closure.get("configured", {})
    supported = configured.get("supported_components", [])
    cpack = configured.get("cpack_components", [])
    apps_dependencies = configured.get("apps_dependencies", [])
    targets = configured.get("targets", [])
    baseline_targets = _baseline_logical_targets(start)
    target_comparison = configured.get("target_comparison", {})
    _validate_comparison_record(
        target_comparison,
        baseline=baseline_targets,
        expected_preserved=targets,
        expected_final=targets,
        description="configured logical targets",
    )
    expected_component_dependencies = {
        component: dependencies
        for component, dependencies in start["cpack"][
            "all_component_dependencies"
        ].items()
        if component not in BOUNDARY.REMOVED_COMPONENTS
    }
    expected_component_dependencies["apps"] = start["cpack"][
        "preserved_apps_dependencies"
    ]
    if (
        supported != start["supported_components"]["preserved"]
        or configured.get("supported_components_sha256")
        != string_manifest_sha256(supported)
        or cpack != start["cpack"]["preserved_components"]
        or configured.get("cpack_components_sha256")
        != string_manifest_sha256(cpack)
        or apps_dependencies
        != start["cpack"]["preserved_apps_dependencies"]
        or configured.get("component_dependencies")
        != dict(sorted(expected_component_dependencies.items()))
        or targets != sorted(set(targets))
        or configured.get("targets_sha256")
        != string_manifest_sha256(targets)
        or configured.get("targets_sha256")
        != EXPECTED_FINAL_TARGETS_SHA256
        or target_comparison.get("removed_count") != 80
        or target_comparison.get("added_count") != 0
    ):
        fail(NON_CODEX_DIAGNOSTIC, "configured non-Codex inventory drifted")

    final_reference = closure.get("final_ctest", {})
    if final_reference != {
        "document": f"docs/migrations/{FINAL_CTEST_FILE}",
        "sha256": sha256_bytes(canonical_json_bytes(final_ctest)),
        "summary": final_ctest["summary"],
    }:
        fail(TEST_DIAGNOSTIC, "closure final CTest reference changed")
    if final_ctest.get("authority") != {
        "base_commit": BOUNDARY.SNODEC_BASE_COMMIT,
        "base_tree": BOUNDARY.SNODEC_BASE_TREE,
        "baseline_evidence_sha256": sha256_bytes(
            canonical_json_bytes(baseline_ctest)
        ),
    }:
        fail(TEST_DIAGNOSTIC, "final CTest authority changed")

    downstream_install = closure.get("downstream_aisuite", {}).get(
        "clean_snodec_install", {}
    )
    if (
        downstream_install.get("file_count")
        != comparisons["install"]["final_count"]
        or downstream_install.get("manifest_sha256")
        != comparisons["install"]["final_sha256"]
    ):
        fail(
            AISUITE_CONSUMER_DIAGNOSTIC,
            "downstream install proof differs from closure artifacts",
        )


def validate_closure_against_current_source(
    repo: Path,
    closure: dict[str, Any],
) -> None:
    """Compare frozen closure evidence with the exact current cutover tree."""

    start, plan, baseline_ctest = BOUNDARY.load_frozen_evidence(repo)
    configured = closure.get("configured", {})
    cpack = configured.get("cpack_components", [])
    apps_dependencies = configured.get("apps_dependencies", [])
    source_boundary = BOUNDARY.collect_source_boundary(
        repo, frozen=(start, plan, baseline_ctest)
    )
    expected_policy = _shared_policy_summary(
        repo, start, BOUNDARY.policy_boundary(repo)
    )
    if closure.get("policy") != expected_policy:
        fail(POLICY_DIAGNOSTIC, "shared policy closure evidence changed")
    if closure.get("identity") != source_boundary["identity"]:
        fail(SOVERSION_DIAGNOSTIC, "version/SOVERSION closure evidence changed")
    if (
        closure.get("dependency_direction")
        != source_boundary["dependency_direction"]
    ):
        fail(
            DEPENDENCY_DIAGNOSTIC,
            "dependency-direction closure evidence changed",
        )

    source_migration = source_boundary["migration"]
    requirements = source_migration["requirements"]
    expected_migration = {
        **source_migration,
        "readme_aisuite_reference": requirements[
            "readme:aisuite-project"
        ],
        "historical_references_classified": all(
            requirements[key]
            for key in [
                "historical:phase-2",
                "historical:phase-3",
                "historical:phase-3-accounting",
                "historical:test-suite-consolidation",
            ]
        ),
    }
    if (
        sorted(requirements) != MIGRATION_REQUIREMENT_KEYS
        or closure.get("migration") != expected_migration
    ):
        fail(MIGRATION_DIAGNOSTIC, "migration closure evidence changed")

    boundary = configured.get("boundary", {})
    package_boundary = boundary.get("package", {})
    test_boundary = boundary.get("tests", {})
    install_boundary = boundary.get("install", {})
    if (
        sorted(boundary)
        != ["build", "install", "package", "paths", "preservation", "tests"]
        or boundary.get("paths") != source_boundary["paths"]
        or boundary.get("build") != source_boundary["build"]
        or test_boundary.get("forbidden")
        or test_boundary.get("missing_survivors")
        or test_boundary.get("survivor_projection_count") != 172
        or test_boundary.get("survivor_projection_sha256")
        != BOUNDARY.SURVIVOR_CTEST_PROJECTION_SHA256
        or test_boundary.get("survivors_match") is not True
        or install_boundary.get("declaration_hits")
        or install_boundary.get("forbidden_paths")
        or install_boundary.get("forbidden_content")
        or install_boundary.get("missing_preserved")
        or package_boundary.get("source_forbidden_paths")
        or package_boundary.get("source_missing_preserved")
        or package_boundary.get("source_ownership_residue")
        or package_boundary.get("source_missing_required")
        or package_boundary.get("binary_forbidden_paths")
        or package_boundary.get("binary_missing_preserved")
        or package_boundary.get("cpack_components") != cpack
        or package_boundary.get("apps_dependencies") != apps_dependencies
        or package_boundary.get("component_dependencies")
        != configured.get("component_dependencies")
        or any(
            value is not True
            for key, value in boundary.get("preservation", {}).items()
            if key.endswith("_match")
        )
    ):
        fail(NON_CODEX_DIAGNOSTIC, "embedded boundary evidence drifted")


def generate_twice(
    collector: Any,
) -> tuple[dict[str, Any], dict[str, Any]]:
    first_ctest, first_closure = collector()
    first = (
        canonical_json_bytes(first_ctest),
        canonical_json_bytes(first_closure),
    )
    second_ctest, second_closure = collector()
    second = (
        canonical_json_bytes(second_ctest),
        canonical_json_bytes(second_closure),
    )
    if first != second:
        fail(
            SECOND_PASS_DIAGNOSTIC,
            "closure generation changed between two identical passes",
        )
    return first_ctest, first_closure


def write_outputs(
    output_dir: Path,
    final_ctest: dict[str, Any],
    closure: dict[str, Any],
) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    BOUNDARY.write_generated(output_dir / FINAL_CTEST_FILE, final_ctest)
    BOUNDARY.write_generated(output_dir / CLOSURE_FILE, closure)


def read_checked_outputs(
    repo: Path,
) -> tuple[dict[str, Any], dict[str, Any]]:
    evidence_dir = repo / "docs/migrations"
    final_ctest = read_canonical_json(
        evidence_dir / FINAL_CTEST_FILE, code=TEST_DIAGNOSTIC
    )
    closure = read_canonical_json(
        evidence_dir / CLOSURE_FILE, code=MIGRATION_DIAGNOSTIC
    )
    if not isinstance(final_ctest, dict) or not isinstance(closure, dict):
        fail(MIGRATION_DIAGNOSTIC, "closure evidence roots must be objects")
    diagnostics = closure_model_diagnostics(closure, final_ctest)
    if diagnostics:
        fail(
            diagnostics[0],
            f"checked closure evidence is invalid: {diagnostics}",
        )
    validate_closure_against_frozen_authority(repo, closure, final_ctest)
    return final_ctest, closure


def require_closure_source_paths(repo: Path) -> None:
    missing = [
        path for path in CLOSURE_NEW_PATHS if not (repo / path).is_file()
    ]
    if missing:
        fail(
            PACKAGE_DIAGNOSTIC,
            f"source package lacks closure paths: {missing}",
        )


def _has_artifact_inputs(args: argparse.Namespace) -> bool:
    return any(
        getattr(args, name, None) is not None
        for name in [
            "ctest_json",
            "build_root",
            "install_manifest",
            "install_root",
            "source_package_manifest",
            "binary_package_manifest",
            "target_inventory",
            "snodec_config",
            "cpack_config",
        ]
    )


def validate_enduring_boundary(args: argparse.Namespace) -> dict[str, Any]:
    """Reject transferred ownership without freezing later SNode.C evolution."""

    repo = args.repo_root.resolve()
    remaining_paths = BOUNDARY.remaining_removal_paths(repo)
    ownership_residue = BOUNDARY.source_ownership_residue_hits(repo)
    if remaining_paths:
        fail(PATH_DIAGNOSTIC, f"removed cutover paths returned: {remaining_paths}")
    if ownership_residue:
        fail(
            BUILD_DIAGNOSTIC,
            f"Codex production ownership residue returned: {ownership_residue}",
        )

    build = BOUNDARY.source_build_surface(repo)
    supported = BOUNDARY.source_supported_components(repo)
    removed_supported = [
        component
        for component in supported
        if component in BOUNDARY.REMOVED_COMPONENTS
    ]
    if (
        build.get("wiring_hits")
        or build.get("forbidden_targets")
        or removed_supported
    ):
        fail(BUILD_DIAGNOSTIC, "removed Codex build surface returned")

    install = BOUNDARY.source_install_surface(repo)
    if install.get("declaration_hits"):
        fail(INSTALL_DIAGNOSTIC, "removed Codex install declaration returned")
    package = BOUNDARY.source_package_surface(repo)
    if package.get("source_residue"):
        fail(PACKAGE_DIAGNOSTIC, "removed Codex CPack source residue returned")
    policy = BOUNDARY.policy_boundary(repo)
    if policy.get("residue"):
        fail(POLICY_DIAGNOSTIC, f"Codex policy residue returned: {policy['residue']}")
    dependency_hits = BOUNDARY.production_dependency_hits(repo)
    if dependency_hits:
        fail(
            DEPENDENCY_DIAGNOSTIC,
            f"SNode.C acquired an AISuite dependency: {dependency_hits}",
        )

    snodec_config = args.snodec_config
    cpack_config = args.cpack_config
    if args.build_root is not None:
        if snodec_config is None:
            snodec_config = args.build_root / "src/snodecConfig.cmake"
        if cpack_config is None:
            cpack_config = args.build_root / "CPackConfig.cmake"
    if snodec_config is not None:
        configured_components = BOUNDARY.extract_supported_components(
            snodec_config.read_text(encoding="utf-8"),
            description=snodec_config.as_posix(),
        )
        if any(
            component in BOUNDARY.REMOVED_COMPONENTS
            for component in configured_components
        ) or any(
            target in snodec_config.read_text(encoding="utf-8")
            for target in BOUNDARY.REMOVED_TARGETS
        ):
            fail(BUILD_DIAGNOSTIC, "generated Codex component/target returned")
    if args.target_inventory is not None:
        configured_targets = BOUNDARY.extract_target_names(
            args.target_inventory
        )
        forbidden_target_names = set(
            BOUNDARY.REMOVED_COMPONENTS
            + BOUNDARY.REMOVED_TARGETS
            + BOUNDARY.REMOVED_APPLICATIONS
            + BOUNDARY.REMOVED_PRIVATE_APP_TARGETS
        )
        if forbidden_target_names & set(configured_targets):
            fail(BUILD_DIAGNOSTIC, "configured Codex target returned")
    if cpack_config is not None:
        components, apps_dependencies, _ = BOUNDARY.extract_cpack_model(
            cpack_config.read_text(encoding="utf-8"),
            description=cpack_config.as_posix(),
        )
        if any(
            component in BOUNDARY.REMOVED_COMPONENTS
            for component in components
        ) or "ai-openai-codex-frontend" in apps_dependencies:
            fail(PACKAGE_DIAGNOSTIC, "configured Codex CPack surface returned")

    if args.ctest_json is not None or args.build_root is not None:
        raw_ctest, build_root = _ctest_raw(args, repo)
        _, _, baseline_ctest = BOUNDARY.load_frozen_evidence(repo)
        ctest = BOUNDARY.collect_ctest_boundary(
            raw_ctest,
            source_root=repo,
            build_root=build_root,
            baseline_ctest=baseline_ctest,
        )
        if ctest.get("forbidden"):
            fail(
                TEST_DIAGNOSTIC,
                f"active Codex CTest returned: {ctest['forbidden']}",
            )

    install_entries: list[str] = []
    if args.install_manifest is not None:
        install_entries = read_manifest(
            args.install_manifest, description="install manifest"
        )
    elif args.install_root is not None:
        install_entries = scan_tree_manifest(args.install_root)
    if any(BOUNDARY.install_path_is_forbidden(path) for path in install_entries):
        fail(INSTALL_DIAGNOSTIC, "installed Codex artifact returned")
    if args.install_root is not None:
        root_paths, root_content = BOUNDARY.scan_install_root(
            args.install_root
        )
        if root_paths or root_content:
            fail(INSTALL_DIAGNOSTIC, "installed Codex artifact/content returned")

    if args.source_package_manifest is not None:
        source_entries = read_manifest(
            args.source_package_manifest,
            description="source-package manifest",
        )
        if any(
            BOUNDARY.source_path_is_forbidden(path)
            for path in source_entries
        ):
            fail(PACKAGE_DIAGNOSTIC, "Codex source-package path returned")
    if args.binary_package_manifest is not None:
        binary_entries = read_manifest(
            args.binary_package_manifest,
            description="binary-package manifest",
        )
        if any(
            BOUNDARY.install_path_is_forbidden(path)
            for path in binary_entries
        ):
            fail(PACKAGE_DIAGNOSTIC, "Codex binary-package artifact returned")
    return {
        "remaining_paths": remaining_paths,
        "ownership_residue": ownership_residue,
        "dependency_hits": dependency_hits,
        "policy_residue": policy["residue"],
    }


def check_live(args: argparse.Namespace) -> dict[str, Any]:
    repo = args.repo_root.resolve()
    history = validate_history(repo)
    final_ctest, closure = read_checked_outputs(repo)
    validate_commit3_boundary(repo, history["commits"][2])
    if history["mode"] == "descendant":
        validate_enduring_boundary(args)
    else:
        validate_closure_against_current_source(repo, closure)
    if (
        history["mode"] != "descendant"
        and args.downstream_proof is not None
    ):
        supplied_proof = _read_downstream_proof(args.downstream_proof)
        if canonical_json_bytes(supplied_proof) != canonical_json_bytes(
            closure["downstream_aisuite"]
        ):
            fail(
                AISUITE_CONSUMER_DIAGNOSTIC,
                "supplied downstream proof differs from checked evidence",
            )
    if _has_artifact_inputs(args):
        if history["mode"] != "descendant":
            expected_ctest, expected_closure = generate_twice(
                lambda: build_closure_outputs(
                    args, downstream_summary=closure["downstream_aisuite"]
                )
            )
            if canonical_json_bytes(expected_ctest) != canonical_json_bytes(
                final_ctest
            ):
                fail(TEST_DIAGNOSTIC, "checked final CTest evidence is stale")
            if canonical_json_bytes(expected_closure) != canonical_json_bytes(
                closure
            ):
                fail(MIGRATION_DIAGNOSTIC, "checked closure evidence is stale")
    return history


def check_package(repo: Path) -> dict[str, Any]:
    repo = repo.resolve()
    if (repo / ".git").exists():
        fail(PACKAGE_DIAGNOSTIC, "package-safe closure received a .git tree")
    require_closure_source_paths(repo)
    final_ctest, closure = read_checked_outputs(repo)
    boundary_args = SimpleNamespace(
        repo_root=repo,
        aisuite_source=None,
        ctest_json=None,
        build_root=None,
        install_manifest=None,
        source_package_manifest=None,
        binary_package_manifest=None,
        snodec_config=None,
        cpack_config=None,
        target_inventory=None,
        install_root=None,
    )
    boundary_model = validate_enduring_boundary(boundary_args)
    current_paths = scan_tree_manifest(repo)
    return {
        "boundary": boundary_model,
        "final_ctest_sha256": sha256_bytes(
            canonical_json_bytes(final_ctest)
        ),
        "source_paths": len(current_paths),
    }


def add_artifact_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--ctest-json", type=Path)
    parser.add_argument("--build-root", type=Path)
    parser.add_argument("--install-manifest", type=Path)
    parser.add_argument("--install-root", type=Path)
    parser.add_argument("--source-package-manifest", type=Path)
    parser.add_argument("--binary-package-manifest", type=Path)
    parser.add_argument("--target-inventory", type=Path)
    parser.add_argument("--snodec-config", type=Path)
    parser.add_argument("--cpack-config", type=Path)
    parser.add_argument("--downstream-proof", type=Path)


def parse_arguments(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="mode", required=True)

    generate = subparsers.add_parser(
        "generate", help="generate deterministic final closure evidence"
    )
    add_artifact_arguments(generate)
    generate.add_argument("--output-dir", type=Path, required=True)

    check = subparsers.add_parser(
        "check", help="verify checked evidence and live bounded history"
    )
    add_artifact_arguments(check)

    history = subparsers.add_parser(
        "check-history", help="verify only the live bounded Git topology"
    )
    history.add_argument("--repo-root", type=Path, required=True)
    history.add_argument("--revision", default="HEAD")

    package = subparsers.add_parser(
        "check-package", help="verify closure from an unpacked source package"
    )
    package.add_argument("--repo-root", type=Path, required=True)
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_arguments(sys.argv[1:] if argv is None else argv)
    try:
        if args.mode == "generate":
            final_ctest, closure = generate_twice(
                lambda: build_closure_outputs(args)
            )
            write_outputs(args.output_dir.resolve(), final_ctest, closure)
            print(
                "SNode.C AISuite cutover closure generated: "
                f"tests={final_ctest['summary']['configured_tests']}, "
                "semantic-logger=81->77, second-pass=identical"
            )
            return 0
        if args.mode == "check":
            history = check_live(args)
            print(
                "SNode.C AISuite cutover closure verified: "
                f"topology={history['mode']}, four-commit-range=valid"
            )
            return 0
        if args.mode == "check-history":
            history = validate_history(
                args.repo_root.resolve(), revision=args.revision
            )
            print(
                "SNode.C AISuite cutover history verified: "
                f"topology={history['mode']}, four-commit-range=valid"
            )
            return 0
        if args.mode == "check-package":
            result = check_package(args.repo_root)
            print(
                "SNode.C AISuite package-safe closure verified: "
                f"source-paths={result['source_paths']}, "
                "git=unused, network=unused, aisuite=unused"
            )
            return 0
    except BOUNDARY.CutoverError as error:
        print(str(error), file=sys.stderr)
        return 1
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
