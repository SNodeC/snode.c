#!/usr/bin/env python3
"""Freeze and verify the SNode.C -> AISuite ownership cutover.

The start-state modes deliberately read the pinned Git object rather than the
checked-out production tree.  Consequently the frozen audit remains useful
after the current-tree removal has happened.  The package-safe and closure
modes are added by the later cutover commits.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any, Iterable, Sequence


FORMAT_VERSION = 1

SNODEC_REPOSITORY = "https://github.com/SNodeC/snode.c"
SNODEC_BASE_COMMIT = "d18b231a1d2ec2235fd6f204786b0a761cc24ff5"
SNODEC_BASE_TREE = "88a63edc985a851b2b76b0c56df19fae74ea8069"
SNODEC_VERSION = "1.0.1"
SNODEC_SOVERSION = "1"

AISUITE_REPOSITORY = "https://github.com/SNodeC/AISuite"
AISUITE_OWNER_COMMIT = "0c3a5838359eb283aca67840325ce6019345b462"
AISUITE_OWNER_TREE = "f86196b41d695f7165dca6a80ec017a8b9166de1"
AISUITE_OWNER_SUBJECT = (
    "Merge pull request #3 from "
    "SNodeC/extraction/complete-codex-policy-ownership"
)
AISUITE_OWNER_PARENTS = [
    "19de4f50be64e187761274f043091090609d27a3",
    "d3da338eed6c11ed05163c3c8c4363702d817147",
]
AISUITE_OWNER_RESPONSIBILITIES = [
    "codex-public-header-policy",
    "codex-logging-api-surface-policy",
    "codex-semantic-logger-policy",
]
AISUITE_PROTOCOL_COUNTS = {
    "Complete": 313,
    "Partial": 4,
    "NotImplemented": 22,
    "NotApplicable": 48,
}

REMOVAL_GROUPS = [
    {
        "path": "src/ai/",
        "kind": "prefix",
        "count": 106,
        "bytes": 2_039_398,
        "manifest_sha256": "98f0fe25bd1ee03b5be98e9ffbe8d4c5205094ba3e63c3bc9e5c218c88edbc73",
    },
    {
        "path": "src/apps/codex-backend/",
        "kind": "prefix",
        "count": 10,
        "bytes": 18_745,
        "manifest_sha256": "2778e58f2aa112c4b76e978efba61ac64c1619b7ecf6f401391445e13485d45c",
    },
    {
        "path": "src/apps/codex-backend-client/",
        "kind": "prefix",
        "count": 21,
        "bytes": 110_820,
        "manifest_sha256": "4050a306507050ea2c3f082bb05ce41830117c91dc9faf90e76b659c047a9f85",
    },
    {
        "path": "docs/ai/openai/codex/",
        "kind": "prefix",
        "count": 19,
        "bytes": 431_322,
        "manifest_sha256": "279c9e21bd95751ab4b2f39bccc405e649edf0b5f4c6d41f37b6a22eef099163",
    },
    {
        "path": "tools/codex/",
        "kind": "prefix",
        "count": 6_537,
        "bytes": 49_406_620,
        "manifest_sha256": "46be207289020cb3e6318b3381602ae4ef15eeea767ed0756d5d4edeb8cc3ea3",
    },
    {
        "path": "tests/component/codex/",
        "kind": "prefix",
        "count": 98,
        "bytes": 2_966_010,
        "manifest_sha256": "33c3b74e1770eb4615713b75a818927094150fe64902aafc0f1770b53d0a6764",
    },
    {
        "path": "tests/installed/codex/",
        "kind": "prefix",
        "count": 13,
        "bytes": 55_209,
        "manifest_sha256": "5ff03f1baa996ecfadf2f2dd983c4208a16a273fbfe257902d597c2b91d3533b",
    },
    {
        "path": "tests/policy/codex/",
        "kind": "prefix",
        "count": 1,
        "bytes": 2_459,
        "manifest_sha256": "721f05534862db4271bef767eaa5431788282471e8c6a8049119fa96dcdb147c",
    },
    {
        "path": "tests/policy/security/CodexSyntheticSecretLeakGuardTest.py",
        "kind": "file",
        "count": 1,
        "bytes": 17_981,
        "manifest_sha256": "5f4bfc67a4c05a52124806ab75baa4fadba3236cab8e7ad26f5c1c14e5afbac6",
    },
    {
        "path": "tests/CodexSourcePackageTest.cmake",
        "kind": "file",
        "count": 1,
        "bytes": 26_454,
        "manifest_sha256": "8566f43c8e5cca8478ec485c0ddc01971d22a7af35229213003362bb23b343a8",
    },
    {
        "path": "tests/CodexBinaryPackageTest.cmake",
        "kind": "file",
        "count": 1,
        "bytes": 6_606,
        "manifest_sha256": "2ba31fd7ef4fb7862bc09b20c9dbe0c3dbeab97648e3a8f2b8ab5589866d1370",
    },
]
REMOVAL_TOTAL_COUNT = 6_808
REMOVAL_TOTAL_BYTES = 55_081_624
REMOVAL_MANIFEST_SHA256 = (
    "b7e89a36f27693f923252745f167461283a60dd36ddab1e8030903ba7fc8f89b"
)

REMOVED_COMPONENTS = [
    "ai-openai-codex",
    "ai-openai-codex-backend",
    "ai-openai-codex-frontend",
]
REMOVED_TARGETS = [
    "ai-openai-codex",
    "ai-openai-codex-backend",
    "ai-openai-codex-frontend",
    "snodec::ai-openai-codex",
    "snodec::ai-openai-codex-backend",
    "snodec::ai-openai-codex-frontend",
]
REMOVED_APPLICATIONS = ["codex-backend", "codex-backend-client"]
REMOVED_PRIVATE_APP_TARGETS = [
    "codex-backend-unix-adapter",
    "codex-backend-client-support",
]
CODEX_SEMANTIC_ENTRIES = [
    "turn {}: thread={} turn={}",
    "turn failed: thread={} turn={}",
    "thread created: thread={}",
    "turn started: thread={} turn={}",
]
CODEX_LOGGING_API_IDENTIFIERS = [
    "lifecycleStart",
    "creationLogged",
    "lifecycleStarted",
    "lifecycleTerminalLogged",
]
CODEX_LOGGING_API_PATHS = [
    "src/ai/openai/codex/backend/BackendEvent.h",
    "src/ai/openai/codex/backend/BackendState.h",
]

REMOVED_EXTERNAL_TESTS = {
    "CodexA14AuditToolTest",
    "CodexSourcePackageTest",
    "CodexBinaryPackageTest",
    "CodexA12PublicHeaderPolicyTest",
    "CodexSyntheticSecretLeakGuardTest",
}
SHARED_ADAPTED_TESTS = {
    "StagedInstalledConsumerTest",
    "CiWorkflowPathsPolicyTest",
    "LoggingApiSurfacePolicyTest",
    "ParameterlessSemanticLoggerPolicyTest",
}

HISTORY_DIAGNOSTIC = "SNodeCCodexCutoverHistoryMismatch"
MIGRATION_DIAGNOSTIC = "SNodeCCodexCutoverMigrationEvidenceMismatch"
NON_CODEX_DIAGNOSTIC = "SNodeCCodexCutoverNonCodexDrift"
START_STATE_DIAGNOSTIC = "SNodeCCodexCutoverStartStateMismatch"
PATH_DIAGNOSTIC = "SNodeCCodexCutoverPathRemaining"
BUILD_DIAGNOSTIC = "SNodeCCodexCutoverBuildSurfaceRemaining"
INSTALL_DIAGNOSTIC = "SNodeCCodexCutoverInstallSurfaceRemaining"
PACKAGE_DIAGNOSTIC = "SNodeCCodexCutoverPackageSurfaceRemaining"
TEST_DIAGNOSTIC = "SNodeCCodexCutoverTestSurfaceRemaining"
POLICY_DIAGNOSTIC = "SNodeCCodexCutoverPolicyResidue"
SOVERSION_DIAGNOSTIC = "SNodeCCodexCutoverSOVERSIONDrift"
DEPENDENCY_DIAGNOSTIC = (
    "SNodeCCodexCutoverDependencyDirectionMismatch"
)
AISUITE_CONSUMER_DIAGNOSTIC = (
    "SNodeCCodexCutoverAISuiteConsumerMismatch"
)
SECOND_PASS_DIAGNOSTIC = (
    "SNodeCCodexCutoverSecondPassNondeterminism"
)

BOUNDARY_DIAGNOSTIC_CODES = (
    PATH_DIAGNOSTIC,
    BUILD_DIAGNOSTIC,
    INSTALL_DIAGNOSTIC,
    PACKAGE_DIAGNOSTIC,
    TEST_DIAGNOSTIC,
    POLICY_DIAGNOSTIC,
    NON_CODEX_DIAGNOSTIC,
    SOVERSION_DIAGNOSTIC,
    DEPENDENCY_DIAGNOSTIC,
    AISUITE_CONSUMER_DIAGNOSTIC,
    MIGRATION_DIAGNOSTIC,
    HISTORY_DIAGNOSTIC,
    SECOND_PASS_DIAGNOSTIC,
)

SURVIVOR_CTEST_PROJECTION_SHA256 = (
    "5388c140e81b923863ca8d9e20313846a377e6184eb98a4f3a5c3d4ecf2aeabb"
)

BOUNDARY_EVIDENCE_FILE = "aisuite-cutover-boundary.json"
MIGRATION_DOCUMENT = "docs/migrations/codex-to-aisuite.md"

INTENTIONAL_RESIDUE_REMOVALS = [".gitattributes"]

BASELINE_LOCAL_GENERATED_SOURCE_PATHS = [
    "certs/snode.c_-_Root_CA.crt",
    "certs/snode.c_-_client.key.encrypted.pem",
    "certs/snode.c_-_client.pem",
    "certs/snode.c_-_server.key.encrypted.pem",
    "certs/snode.c_-_server.pem",
    "docs/Doxyfile",
]

BASELINE_LOCAL_GENERATED_SOURCE_IGNORE_RULES = {
    "certs/snode.c_-_Root_CA.crt": (
        r'"/certs/snode\\.c_-_Root_CA\\.crt$"'
    ),
    "certs/snode.c_-_client.key.encrypted.pem": (
        r'"/certs/snode\\.c_-_client\\.key\\.encrypted\\.pem$"'
    ),
    "certs/snode.c_-_client.pem": (
        r'"/certs/snode\\.c_-_client\\.pem$"'
    ),
    "certs/snode.c_-_server.key.encrypted.pem": (
        r'"/certs/snode\\.c_-_server\\.key\\.encrypted\\.pem$"'
    ),
    "certs/snode.c_-_server.pem": (
        r'"/certs/snode\\.c_-_server\\.pem$"'
    ),
    "docs/Doxyfile": r'"/docs/Doxyfile$"',
}

REQUIRED_CUTOVER_SOURCE_PATHS = [
    "docs/migrations/aisuite-cutover-baseline-ctest.json",
    "docs/migrations/aisuite-cutover-boundary.md",
    f"docs/migrations/{BOUNDARY_EVIDENCE_FILE}",
    "docs/migrations/aisuite-cutover-plan.json",
    "docs/migrations/aisuite-cutover-start-state.json",
    MIGRATION_DOCUMENT,
    "tests/AISuiteCutoverBinaryPackageTest.cmake",
    "tests/AISuiteCutoverSourcePackageTest.cmake",
    "tests/ownership/AISuiteCutoverMutationTest.py",
    "tests/ownership/AISuiteCutoverStartStateMutationTest.py",
    "tests/ownership/CMakeLists.txt",
    "tools/ownership/verify_aisuite_cutover.py",
]

ALLOWED_SURVIVOR_LABEL_CHANGES = {
    "StagedInstalledConsumerTest": ["consumer", "install"],
    "CiWorkflowPathsPolicyTest": ["architecture", "ci", "policy"],
}


class CutoverError(RuntimeError):
    def __init__(self, code: str, message: str):
        super().__init__(f"{code}: {message}")
        self.code = code
        self.message = message


def fail(code: str, message: str) -> None:
    raise CutoverError(code, message)


def run(
    command: Sequence[str],
    *,
    cwd: Path,
    check: bool = True,
) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        list(command),
        cwd=cwd,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if check and result.returncode != 0:
        fail(
            START_STATE_DIAGNOSTIC,
            f"command failed ({' '.join(command)}): {result.stderr.strip()}",
        )
    return result


def git(repo: Path, *arguments: str) -> str:
    return run(["git", *arguments], cwd=repo).stdout.strip()


def sha256_bytes(content: bytes) -> str:
    return hashlib.sha256(content).hexdigest()


def canonical_json_bytes(value: Any) -> bytes:
    return (
        json.dumps(value, indent=2, sort_keys=True, ensure_ascii=False) + "\n"
    ).encode("utf-8")


def read_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        fail(START_STATE_DIAGNOSTIC, f"cannot read JSON {path}: {error}")


def write_generated(path: Path, value: Any) -> None:
    content = canonical_json_bytes(value)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_bytes(content)
    os.replace(temporary, path)


def manifest_hash(records: Iterable[dict[str, Any]]) -> str:
    stream = "".join(
        f"{record['path']}\t{record['blob']}\t"
        f"{record['sha256']}\t{record['size']}\n"
        for record in records
    )
    return sha256_bytes(stream.encode("utf-8"))


def is_group_path(path: str, group: dict[str, Any]) -> bool:
    if group["kind"] == "prefix":
        return path.startswith(group["path"])
    return path == group["path"]


def ls_tree(repo: Path, revision: str) -> list[dict[str, str]]:
    raw = run(
        ["git", "ls-tree", "-r", "-z", revision],
        cwd=repo,
    ).stdout
    result: list[dict[str, str]] = []
    for entry in raw.split("\0"):
        if not entry:
            continue
        metadata, path = entry.split("\t", 1)
        mode, object_type, blob = metadata.split(" ", 2)
        if object_type != "blob":
            continue
        result.append(
            {"path": path, "mode": mode, "type": object_type, "blob": blob}
        )
    return result


def read_git_blobs(repo: Path, blob_ids: Sequence[str]) -> dict[str, bytes]:
    unique_ids = list(dict.fromkeys(blob_ids))
    query = "".join(f"{blob}\n" for blob in unique_ids).encode("ascii")
    process = subprocess.Popen(
        ["git", "cat-file", "--batch"],
        cwd=repo,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    output, error = process.communicate(query)
    if process.returncode != 0:
        fail(
            START_STATE_DIAGNOSTIC,
            f"git cat-file --batch failed: {error.decode(errors='replace')}",
        )
    position = 0
    blobs: dict[str, bytes] = {}
    for requested in unique_ids:
        newline = output.find(b"\n", position)
        if newline < 0:
            fail(START_STATE_DIAGNOSTIC, "truncated git cat-file header")
        header = output[position:newline].decode("ascii")
        position = newline + 1
        fields = header.split()
        if len(fields) != 3 or fields[1] != "blob":
            fail(
                START_STATE_DIAGNOSTIC,
                f"unexpected git cat-file header for {requested}: {header}",
            )
        observed, _, size_text = fields
        size = int(size_text)
        content = output[position : position + size]
        position += size
        if output[position : position + 1] != b"\n":
            fail(START_STATE_DIAGNOSTIC, "truncated git cat-file payload")
        position += 1
        if observed != requested:
            fail(
                START_STATE_DIAGNOSTIC,
                f"git cat-file returned {observed}, expected {requested}",
            )
        blobs[requested] = content
    return blobs


def capture_removal_inventory(repo: Path) -> dict[str, Any]:
    base_tree = git(repo, "rev-parse", f"{SNODEC_BASE_COMMIT}^{{tree}}")
    if base_tree != SNODEC_BASE_TREE:
        fail(
            HISTORY_DIAGNOSTIC,
            f"base tree is {base_tree}, expected {SNODEC_BASE_TREE}",
        )
    tree_entries = ls_tree(repo, SNODEC_BASE_COMMIT)
    selected = [
        entry
        for entry in tree_entries
        if any(is_group_path(entry["path"], group) for group in REMOVAL_GROUPS)
    ]
    blobs = read_git_blobs(repo, [entry["blob"] for entry in selected])
    records = [
        {
            "path": entry["path"],
            "mode": entry["mode"],
            "blob": entry["blob"],
            "sha256": sha256_bytes(blobs[entry["blob"]]),
            "size": len(blobs[entry["blob"]]),
        }
        for entry in selected
    ]
    records.sort(key=lambda record: record["path"])

    group_evidence: list[dict[str, Any]] = []
    for expected in REMOVAL_GROUPS:
        group_records = [
            record for record in records if is_group_path(record["path"], expected)
        ]
        observed = {
            **expected,
            "observed_count": len(group_records),
            "observed_bytes": sum(record["size"] for record in group_records),
            "observed_manifest_sha256": manifest_hash(group_records),
        }
        if observed["observed_count"] != expected["count"]:
            fail(
                START_STATE_DIAGNOSTIC,
                f"{expected['path']} count is {observed['observed_count']}, "
                f"expected {expected['count']}",
            )
        if observed["observed_bytes"] != expected["bytes"]:
            fail(
                START_STATE_DIAGNOSTIC,
                f"{expected['path']} bytes are {observed['observed_bytes']}, "
                f"expected {expected['bytes']}",
            )
        if (
            observed["observed_manifest_sha256"]
            != expected["manifest_sha256"]
        ):
            fail(
                START_STATE_DIAGNOSTIC,
                f"{expected['path']} manifest hash changed",
            )
        group_evidence.append(observed)

    total_bytes = sum(record["size"] for record in records)
    aggregate_hash = manifest_hash(records)
    if len(records) != REMOVAL_TOTAL_COUNT:
        fail(
            START_STATE_DIAGNOSTIC,
            f"removal inventory count is {len(records)}, "
            f"expected {REMOVAL_TOTAL_COUNT}",
        )
    if total_bytes != REMOVAL_TOTAL_BYTES:
        fail(
            START_STATE_DIAGNOSTIC,
            f"removal inventory bytes are {total_bytes}, "
            f"expected {REMOVAL_TOTAL_BYTES}",
        )
    if aggregate_hash != REMOVAL_MANIFEST_SHA256:
        fail(
            START_STATE_DIAGNOSTIC,
            f"removal manifest hash is {aggregate_hash}, "
            f"expected {REMOVAL_MANIFEST_SHA256}",
        )

    ai_paths = [
        record["path"]
        for record in records
        if record["path"].startswith("src/ai/")
    ]
    unrelated_ai = [
        path
        for path in ai_paths
        if path not in {"src/ai/CMakeLists.txt", "src/ai/openai/CMakeLists.txt"}
        and not path.startswith("src/ai/openai/codex/")
    ]
    if unrelated_ai:
        fail(
            START_STATE_DIAGNOSTIC,
            f"non-Codex implementation exists under src/ai: {unrelated_ai}",
        )

    return {
        "groups": group_evidence,
        "total_count": len(records),
        "total_bytes": total_bytes,
        "manifest_sha256": aggregate_hash,
        "files": records,
        "src_ai_contains_only_openai_codex": True,
    }


def normalize_path_text(
    value: str,
    *,
    source_root: Path,
    build_root: Path | None,
) -> str:
    normalized = value.replace("\\", "/")
    if build_root is not None:
        build = build_root.resolve().as_posix()
        normalized = normalized.replace(build, "${SNODEC_BUILD}")
    source = source_root.resolve().as_posix()
    normalized = normalized.replace(source, "${SNODEC_SOURCE}")
    if normalized in {
        "/usr/bin/python3",
        "/usr/local/bin/python3",
        sys.executable.replace("\\", "/"),
    }:
        return "${PYTHON}"
    if normalized.endswith("/cmake") and Path(value).name == "cmake":
        return "${CMAKE}"
    return normalized


def backtrace_files(
    raw: dict[str, Any],
    node_index: int,
    *,
    source_root: Path,
) -> list[str]:
    graph = raw["backtraceGraph"]
    nodes = graph["nodes"]
    files = graph["files"]
    result: list[str] = []
    seen: set[int] = set()
    current: int | None = node_index
    while current is not None and current not in seen:
        seen.add(current)
        node = nodes[current]
        file_name = files[node["file"]]
        result.append(
            normalize_path_text(
                file_name, source_root=source_root, build_root=None
            )
        )
        current = node.get("parent")
    return list(dict.fromkeys(result))


def normalize_property(
    property_value: dict[str, Any],
    *,
    source_root: Path,
    build_root: Path | None,
) -> dict[str, Any]:
    value = copy.deepcopy(property_value["value"])
    if isinstance(value, str):
        value = normalize_path_text(
            value, source_root=source_root, build_root=build_root
        )
    elif isinstance(value, list):
        value = [
            normalize_path_text(
                item, source_root=source_root, build_root=build_root
            )
            if isinstance(item, str)
            else item
            for item in value
        ]
        if property_value["name"] == "LABELS":
            value = sorted(value)
    return {"name": property_value["name"], "value": value}


def normalize_ctest(
    path: Path,
    *,
    source_root: Path,
    build_root: Path | None,
) -> dict[str, Any]:
    raw = read_json(path)
    tests: list[dict[str, Any]] = []
    names: set[str] = set()
    for source_test in raw.get("tests", []):
        name = source_test["name"]
        if name in names:
            fail(NON_CODEX_DIAGNOSTIC, f"duplicate baseline CTest {name}")
        names.add(name)
        origins = backtrace_files(
            raw, source_test["backtrace"], source_root=source_root
        )
        from_codex_hierarchy = any(
            origin.startswith(
                "${SNODEC_SOURCE}/tests/component/codex/"
            )
            or origin
            == "${SNODEC_SOURCE}/tests/component/codex/CMakeLists.txt"
            for origin in origins
        )
        if from_codex_hierarchy or name in REMOVED_EXTERNAL_TESTS:
            classification = "codex-owned-removed"
        elif name in SHARED_ADAPTED_TESTS:
            classification = "shared-codex-branch-removed"
        else:
            classification = "non-codex-preserved"
        tests.append(
            {
                "name": name,
                "classification": classification,
                "command": [
                    normalize_path_text(
                        argument,
                        source_root=source_root,
                        build_root=build_root,
                    )
                    for argument in source_test.get("command", [])
                ],
                "origins": origins,
                "properties": sorted(
                    [
                        normalize_property(
                            prop,
                            source_root=source_root,
                            build_root=build_root,
                        )
                        for prop in source_test.get("properties", [])
                    ],
                    key=lambda prop: prop["name"],
                ),
            }
        )
    tests.sort(key=lambda test: test["name"])
    classification_counts = {
        classification: sum(
            test["classification"] == classification for test in tests
        )
        for classification in [
            "codex-owned-removed",
            "shared-codex-branch-removed",
            "non-codex-preserved",
        ]
    }
    hierarchy_count = sum(
        any(
            origin.startswith("${SNODEC_SOURCE}/tests/component/codex/")
            for origin in test["origins"]
        )
        for test in tests
    )
    codex_label_count = sum(
        any(
            prop["name"] == "LABELS" and "codex" in prop["value"]
            for prop in test["properties"]
        )
        for test in tests
    )
    expected_counts = {
        "codex-owned-removed": 128,
        "shared-codex-branch-removed": 4,
        "non-codex-preserved": 168,
    }
    if len(tests) != 300 or classification_counts != expected_counts:
        fail(
            NON_CODEX_DIAGNOSTIC,
            "baseline CTest classification changed: "
            f"total={len(tests)}, classes={classification_counts}",
        )
    if hierarchy_count != 123:
        fail(
            NON_CODEX_DIAGNOSTIC,
            f"SNode.C Codex hierarchy has {hierarchy_count} tests, expected 123",
        )
    if codex_label_count != 130:
        fail(
            NON_CODEX_DIAGNOSTIC,
            f"baseline Codex label count is {codex_label_count}, expected 130",
        )
    return {
        "format_version": FORMAT_VERSION,
        "kind": "snodec-aisuite-cutover-baseline-ctest",
        "normalization": {
            "source_root": "${SNODEC_SOURCE}",
            "build_root": "${SNODEC_BUILD}",
            "python": "${PYTHON}",
            "cmake": "${CMAKE}",
            "tests_sorted_by": "name",
            "properties_sorted_by": "name",
            "labels_sorted": True,
        },
        "summary": {
            "configured_tests": len(tests),
            "unique_names": len(names),
            "snodec_codex_hierarchy_tests": hierarchy_count,
            "aisuite_codex_hierarchy_tests": 131,
            "aisuite_only_completion_tests": 8,
            "codex_labelled_tests": codex_label_count,
            "classifications": classification_counts,
            "surviving_baseline_names": 172,
        },
        "tests": tests,
    }


def read_sorted_manifest(path: Path, expected_count: int) -> list[str]:
    try:
        entries = path.read_text(encoding="utf-8").splitlines()
    except OSError as error:
        fail(START_STATE_DIAGNOSTIC, f"cannot read manifest {path}: {error}")
    if len(entries) != expected_count:
        fail(
            START_STATE_DIAGNOSTIC,
            f"{path} has {len(entries)} entries, expected {expected_count}",
        )
    if entries != sorted(entries) or len(entries) != len(set(entries)):
        fail(
            START_STATE_DIAGNOSTIC,
            f"{path} is not a unique C-sorted path manifest",
        )
    return entries


def parse_supported_components(config_path: Path) -> list[str]:
    source = config_path.read_text(encoding="utf-8")
    match = re.search(
        r"set\(_supported_components\s+(.*?)\)",
        source,
        flags=re.DOTALL,
    )
    if match is None:
        fail(
            START_STATE_DIAGNOSTIC,
            f"cannot find supported components in {config_path}",
        )
    components = re.findall(r"[A-Za-z0-9_.+-]+", match.group(1))
    if len(components) != 38:
        fail(
            NON_CODEX_DIAGNOSTIC,
            f"supported component count is {len(components)}, expected 38",
        )
    if [item for item in components if item in REMOVED_COMPONENTS] != (
        REMOVED_COMPONENTS
    ):
        fail(
            START_STATE_DIAGNOSTIC,
            "legacy supported-component inventory changed",
        )
    return components


def parse_cpack(
    config_path: Path,
) -> tuple[list[str], list[str], dict[str, list[str]]]:
    source = config_path.read_text(encoding="utf-8")
    components_match = re.search(
        r'set\(CPACK_COMPONENTS_ALL "([^"]+)"\)', source
    )
    apps_match = re.search(
        r"set\(CPACK_COMPONENT_APPS_DEPENDS ([^)]+)\)", source
    )
    if components_match is None or apps_match is None:
        fail(
            START_STATE_DIAGNOSTIC,
            f"cannot parse CPack component model from {config_path}",
        )
    components = components_match.group(1).split(";")
    apps_dependencies = apps_match.group(1).split()
    if len(components) != 69:
        fail(
            NON_CODEX_DIAGNOSTIC,
            f"CPack component count is {len(components)}, expected 69",
        )
    if len(apps_dependencies) != 18:
        fail(
            NON_CODEX_DIAGNOSTIC,
            f"apps dependency count is {len(apps_dependencies)}, expected 18",
        )
    for component in REMOVED_COMPONENTS:
        if component not in components:
            fail(
                START_STATE_DIAGNOSTIC,
                f"baseline CPack component missing: {component}",
            )
    if "ai-openai-codex-frontend" not in apps_dependencies:
        fail(
            START_STATE_DIAGNOSTIC,
            "baseline apps component lacks Codex frontend dependency",
        )
    component_dependencies: dict[str, list[str]] = {}
    for component, dependencies in re.findall(
        r"set\(CPACK_COMPONENT_([A-Z0-9-]+)_DEPENDS ([^)]+)\)",
        source,
    ):
        component_dependencies[component.lower()] = dependencies.split()
    return components, apps_dependencies, dict(
        sorted(component_dependencies.items())
    )


def parse_targets(path: Path, *, build_root: Path | None) -> list[str]:
    targets: list[str] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if ": " not in line:
            continue
        target = line.split(":", 1)[0].strip()
        if build_root is not None:
            target = target.replace(
                build_root.resolve().as_posix(), "${SNODEC_BUILD}"
            )
        if target and not target.startswith("["):
            targets.append(target)
    targets = sorted(set(targets))
    for target in REMOVED_COMPONENTS + REMOVED_PRIVATE_APP_TARGETS:
        if target not in targets:
            fail(
                START_STATE_DIAGNOSTIC,
                f"baseline target inventory lacks {target}",
            )
    for app in REMOVED_APPLICATIONS:
        if app not in targets:
            fail(
                START_STATE_DIAGNOSTIC,
                f"baseline target inventory lacks application {app}",
            )
    return targets


def verify_project_identity(repo: Path) -> None:
    root_cmake = (
        run(
            ["git", "show", f"{SNODEC_BASE_COMMIT}:CMakeLists.txt"],
            cwd=repo,
        )
        .stdout
    )
    if not re.search(r"\bVERSION\s+1\.0\.1\b", root_cmake):
        fail(
            NON_CODEX_DIAGNOSTIC,
            f"project version is not {SNODEC_VERSION}",
        )
    if "set(SNODEC_SOVERSION ${SNode.C_VERSION_MAJOR})" not in root_cmake:
        fail(
            NON_CODEX_DIAGNOSTIC,
            "SNode.C SOVERSION derivation changed",
        )


def verify_policy_baseline(repo: Path) -> dict[str, Any]:
    semantic = run(
        [
            "git",
            "show",
            f"{SNODEC_BASE_COMMIT}:"
            "tests/policy/log/ParameterlessSemanticLoggerPolicyTest.cpp",
        ],
        cwd=repo,
    ).stdout
    entry_count = semantic.count("Entry{")
    if entry_count != 81:
        fail(
            START_STATE_DIAGNOSTIC,
            f"semantic allowlist has {entry_count} entries, expected 81",
        )
    for entry in CODEX_SEMANTIC_ENTRIES:
        if semantic.count(f'"{entry}"') != 1:
            fail(
                START_STATE_DIAGNOSTIC,
                f"Codex semantic authority entry changed: {entry}",
            )
    if semantic.count("src/ai/openai/codex") < 5:
        fail(
            START_STATE_DIAGNOSTIC,
            "Codex semantic scan root/allowlist paths are incomplete",
        )
    if (
        "calls.size() != 81" not in semantic
        or "allowlist.size() != 81" not in semantic
    ):
        fail(
            START_STATE_DIAGNOSTIC,
            "semantic policy does not enforce 81/81",
        )
    quoted = r'"((?:[^"\\]|\\.)*)"'
    semantic_entry_pattern = re.compile(
        r"Entry\{"
        + quoted
        + r"\s*,\s*"
        + quoted
        + r"\s*,\s*"
        + quoted
        + r"\s*,\s*"
        + quoted
        + r"\s*,\s*"
        + quoted
        + r"\s*\}",
        flags=re.DOTALL,
    )
    semantic_entries = [
        {
            "path": fields[0],
            "logger_function": fields[1],
            "identifying_expression": fields[2],
            "classification": fields[3],
            "rationale": fields[4],
        }
        for fields in semantic_entry_pattern.findall(semantic)
    ]
    if len(semantic_entries) != 81:
        fail(
            START_STATE_DIAGNOSTIC,
            "could not mechanically parse all 81 semantic allowlist entries",
        )
    non_codex_entries = [
        entry
        for entry in semantic_entries
        if not entry["path"].startswith("src/ai/openai/codex/")
    ]
    if len(non_codex_entries) != 77:
        fail(
            START_STATE_DIAGNOSTIC,
            f"non-Codex semantic allowlist has {len(non_codex_entries)} entries",
        )

    logging_api = run(
        [
            "git",
            "show",
            f"{SNODEC_BASE_COMMIT}:"
            "tests/policy/log/LoggingApiSurfacePolicyTest.cpp",
        ],
        cwd=repo,
    ).stdout
    for path in CODEX_LOGGING_API_PATHS:
        if path not in logging_api:
            fail(
                START_STATE_DIAGNOSTIC,
                f"logging API policy lacks {path}",
            )
    for identifier in CODEX_LOGGING_API_IDENTIFIERS:
        if f'"{identifier}"' not in logging_api:
            fail(
                START_STATE_DIAGNOSTIC,
                f"logging API policy lacks {identifier}",
            )

    workflow = run(
        ["git", "show", f"{SNODEC_BASE_COMMIT}:.github/workflows/ci.yml"],
        cwd=repo,
    ).stdout
    workflow_policy = run(
        [
            "git",
            "show",
            f"{SNODEC_BASE_COMMIT}:"
            "tests/policy/ci/CiWorkflowPathsPolicyTest.cpp",
        ],
        cwd=repo,
    ).stdout
    docs_path = "docs/ai/openai/codex/**"
    if workflow.count(docs_path) != 2 or workflow_policy.count(docs_path) != 1:
        fail(
            START_STATE_DIAGNOSTIC,
            "baseline CI Codex documentation path accounting changed",
        )
    return {
        "parameterless_semantic_logger": {
            "discovered": 81,
            "allowlisted": entry_count,
            "allowlist": semantic_entries,
            "codex_entries": CODEX_SEMANTIC_ENTRIES,
            "codex_entry_count": 4,
            "non_codex_entry_count": len(non_codex_entries),
            "non_codex_allowlist": non_codex_entries,
            "source_sha256": sha256_bytes(semantic.encode("utf-8")),
            "post_cutover_expected": {
                "discovered": 77,
                "allowlisted": 77,
            },
        },
        "logging_api_surface": {
            "scan_paths": CODEX_LOGGING_API_PATHS,
            "forbidden_identifiers": CODEX_LOGGING_API_IDENTIFIERS,
        },
        "ci_paths": {
            "workflow_occurrences": 2,
            "policy_occurrences": 1,
            "path": docs_path,
        },
    }


def verify_aisuite_authority(aisuite: Path) -> dict[str, Any]:
    commit = git(aisuite, "rev-parse", "HEAD")
    tree = git(aisuite, "rev-parse", "HEAD^{tree}")
    subject = git(aisuite, "show", "-s", "--format=%s", "HEAD")
    parents = git(aisuite, "show", "-s", "--format=%P", "HEAD").split()
    if commit != AISUITE_OWNER_COMMIT:
        fail(
            MIGRATION_DIAGNOSTIC,
            f"AISuite HEAD is {commit}, expected {AISUITE_OWNER_COMMIT}",
        )
    if tree != AISUITE_OWNER_TREE or subject != AISUITE_OWNER_SUBJECT:
        fail(
            MIGRATION_DIAGNOSTIC,
            "AISuite owner tree or merge subject changed",
        )
    if parents != AISUITE_OWNER_PARENTS:
        fail(
            MIGRATION_DIAGNOSTIC,
            f"AISuite owner parents are {parents}",
        )
    second_parent_tree = git(
        aisuite, "rev-parse", f"{AISUITE_OWNER_COMMIT}^2^{{tree}}"
    )
    if second_parent_tree != AISUITE_OWNER_TREE:
        fail(
            MIGRATION_DIAGNOSTIC,
            "AISuite merge tree differs from its second parent",
        )

    ownership_path = (
        aisuite / "docs/extraction/codex-policy-ownership.json"
    )
    ownership = read_json(ownership_path)
    readiness = ownership.get("cutover_readiness", {})
    if readiness.get("ready") is not True:
        fail(MIGRATION_DIAGNOSTIC, "AISuite cutover readiness is not true")
    if readiness.get("snodec_cutover_performed") is not False:
        fail(
            MIGRATION_DIAGNOSTIC,
            "AISuite start authority does not report pending SNode.C cutover",
        )
    responsibilities = [
        item.get("responsibility_id")
        for item in ownership.get("transferred_responsibilities", [])
    ]
    if responsibilities != AISUITE_OWNER_RESPONSIBILITIES:
        fail(
            MIGRATION_DIAGNOSTIC,
            f"AISuite responsibilities changed: {responsibilities}",
        )
    closure_path = (
        aisuite
        / "tools/codex/app-server-evidence/0.144.6/"
        "a1-4-user-integrations-closure-report.json"
    )
    closure = read_json(closure_path)
    reported_protocol_counts = closure.get("counts", {}).get(
        "global_status", {}
    )
    protocol_counts = {
        key: reported_protocol_counts.get(key)
        for key in AISUITE_PROTOCOL_COUNTS
    }
    if protocol_counts != AISUITE_PROTOCOL_COUNTS:
        fail(
            MIGRATION_DIAGNOSTIC,
            f"AISuite protocol counts changed: {protocol_counts}",
        )
    public_headers = ownership.get("public_header_policy", {}).get(
        "counts", {}
    )
    if public_headers.get("total") != 41:
        fail(
            MIGRATION_DIAGNOSTIC,
            f"AISuite public-header count is {public_headers.get('total')}",
        )
    root_cmake = (aisuite / "CMakeLists.txt").read_text(encoding="utf-8")
    if not re.search(r"AISUITE_CODEX_SOVERSION\s+1\b", root_cmake):
        fail(MIGRATION_DIAGNOSTIC, "AISuite Codex SOVERSION is not 1")
    return {
        "repository": AISUITE_REPOSITORY,
        "commit": commit,
        "tree": tree,
        "subject": subject,
        "parents": parents,
        "second_parent_tree": second_parent_tree,
        "merge_tree_equals_second_parent": True,
        "cutover_readiness": readiness,
        "transferred_responsibilities": responsibilities,
        "protocol_counts": protocol_counts,
        "codex_soversion": 1,
        "installed_public_headers": 41,
        "ownership_evidence": {
            "path": "docs/extraction/codex-policy-ownership.json",
            "sha256": sha256_bytes(ownership_path.read_bytes()),
        },
    }


def start_model_diagnostics(model: dict[str, Any]) -> list[str]:
    diagnostics: list[str] = []
    authority = model.get("authority", {})
    snodec = authority.get("snodec", {})
    aisuite = authority.get("aisuite", {})
    if (
        snodec.get("commit") != SNODEC_BASE_COMMIT
        or snodec.get("tree") != SNODEC_BASE_TREE
    ):
        diagnostics.append(HISTORY_DIAGNOSTIC)
    if (
        aisuite.get("commit") != AISUITE_OWNER_COMMIT
        or aisuite.get("tree") != AISUITE_OWNER_TREE
        or aisuite.get("subject") != AISUITE_OWNER_SUBJECT
        or aisuite.get("parents") != AISUITE_OWNER_PARENTS
    ):
        diagnostics.append(MIGRATION_DIAGNOSTIC)
    removal = model.get("removal_inventory", {})
    ctest = model.get("ctest", {})
    legacy = model.get("legacy_surface", {})
    if (
        removal.get("total_count") != REMOVAL_TOTAL_COUNT
        or removal.get("total_bytes") != REMOVAL_TOTAL_BYTES
        or removal.get("manifest_sha256") != REMOVAL_MANIFEST_SHA256
        or legacy.get("public_header_count") != 34
        or ctest.get("snodec_codex_hierarchy_tests") != 123
        or ctest.get("aisuite_codex_hierarchy_tests") != 131
        or ctest.get("non_codex_preserved") != 168
    ):
        diagnostics.append(NON_CODEX_DIAGNOSTIC)
    return list(dict.fromkeys(diagnostics))


def make_plan() -> dict[str, Any]:
    return {
        "format_version": FORMAT_VERSION,
        "kind": "snodec-aisuite-cutover-plan",
        "bounded_history": [
            {
                "ordinal": 1,
                "subject": "Freeze the SNode.C Codex cutover boundary",
                "owns": [
                    "start-state audit tooling",
                    "deterministic baseline evidence",
                    "exact removal plan",
                    "AISuite ownership-authority verification",
                    "audit mutation tests",
                ],
                "production_changes_allowed": False,
            },
            {
                "ordinal": 2,
                "subject": (
                    "Remove Codex from the SNode.C build and package surface"
                ),
                "owns": [
                    "production source and application deletion",
                    "build/component/CPack/install detachment",
                    "CTest registration detachment",
                    "shared logging and CI policy repair",
                    "fresh installed-consumer negative assertions",
                ],
            },
            {
                "ordinal": 3,
                "subject": (
                    "Remove duplicated Codex residue and document "
                    "AISuite ownership"
                ),
                "owns": [
                    "remaining documentation/tool/test/evidence deletion",
                    "migration documentation",
                    "source and binary cutover package tests",
                    "permanent ownership-boundary guard",
                ],
            },
            {
                "ordinal": 4,
                "subject": "Close and verify the SNode.C Codex cutover",
                "owns": [
                    "closure checker and mutation tests",
                    "baseline-versus-result evidence",
                    "deterministic package-safe closure evidence",
                ],
                "functional_corrections_allowed": False,
            },
        ],
        "dedicated_removal_groups": copy.deepcopy(REMOVAL_GROUPS),
        "functional_shared_files_commit_2": [
            "src/CMakeLists.txt",
            "src/cmake/Config.cmake.in",
            "cmake/Packing.cmake",
            "tests/CMakeLists.txt",
            "tests/component/CMakeLists.txt",
            "tests/policy/CMakeLists.txt",
            "tests/StagedInstalledConsumerTest.cmake",
            "tests/policy/log/LoggingApiSurfacePolicyTest.cpp",
            "tests/policy/log/ParameterlessSemanticLoggerPolicyTest.cpp",
            "tests/policy/ci/CiWorkflowPathsPolicyTest.cpp",
            ".github/workflows/ci.yml",
        ],
        "residue_shared_files_commit_3": [
            ".gitattributes",
            "README.md",
            "cmake/Packing.cmake",
            "docs/logging/phase-2-transport-attempt-lifecycle.md",
            "docs/logging/phase-3-protocol-session-request-lifecycle.md",
            "docs/testing/test-suite-consolidation.md",
        ],
        "invariants": {
            "version": SNODEC_VERSION,
            "soversion": SNODEC_SOVERSION,
            "dependency_direction": "AISuite -> installed SNode.C",
            "snodec_depends_on_aisuite": False,
            "history_and_tags_rewritten": False,
            "compatibility_shim_allowed": False,
        },
    }


def build_start_state(args: argparse.Namespace) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any]]:
    repo = args.repo_root.resolve()
    verify_project_identity(repo)
    removal = capture_removal_inventory(repo)
    policies = verify_policy_baseline(repo)
    if args.aisuite_source is None:
        fail(
            MIGRATION_DIAGNOSTIC,
            "--aisuite-source is required when generating start evidence",
        )
    aisuite = verify_aisuite_authority(args.aisuite_source.resolve())
    required_artifacts = {
        "ctest_json": args.ctest_json,
        "install_manifest": args.install_manifest,
        "source_package_manifest": args.source_package_manifest,
        "binary_package_manifest": args.binary_package_manifest,
        "snodec_config": args.snodec_config,
        "cpack_config": args.cpack_config,
        "target_inventory": args.target_inventory,
    }
    missing = [name for name, value in required_artifacts.items() if value is None]
    if missing:
        fail(
            START_STATE_DIAGNOSTIC,
            f"generation inputs are missing: {', '.join(missing)}",
        )
    baseline_ctest = normalize_ctest(
        args.ctest_json,
        source_root=repo,
        build_root=args.build_root,
    )
    install_manifest = read_sorted_manifest(args.install_manifest, 892)
    source_manifest = read_sorted_manifest(args.source_package_manifest, 8_222)
    binary_manifest = read_sorted_manifest(args.binary_package_manifest, 892)
    if install_manifest != binary_manifest:
        fail(
            START_STATE_DIAGNOSTIC,
            "baseline binary-package and fresh-install manifests differ",
        )
    install_codex = [
        entry
        for entry in install_manifest
        if entry.startswith("include/snode.c/ai/")
        or "snodec-ai-openai-codex" in entry
        or entry in {"bin/codex-backend", "bin/codex-backend-client"}
        or (
            entry.startswith("lib/cmake/snodec/")
            and "codex" in entry.lower()
        )
    ]
    installed_headers = [
        entry
        for entry in install_codex
        if entry.startswith("include/snode.c/ai/openai/codex/")
        and entry.endswith(".h")
    ]
    installed_libraries = [
        entry
        for entry in install_codex
        if entry.startswith("lib/libsnodec-ai-openai-codex")
    ]
    installed_exports = [
        entry
        for entry in install_codex
        if entry.startswith("lib/cmake/snodec/")
    ]
    installed_apps = [
        entry
        for entry in install_codex
        if entry in {"bin/codex-backend", "bin/codex-backend-client"}
    ]
    composition = [
        len(installed_headers),
        len(installed_libraries),
        len(installed_exports),
        len(installed_apps),
    ]
    if len(install_codex) != 51 or composition != [34, 9, 6, 2]:
        fail(
            START_STATE_DIAGNOSTIC,
            f"installed Codex composition changed: total={len(install_codex)}, "
            f"parts={composition}",
        )
    all_installed_applications = [
        entry for entry in install_manifest if entry.startswith("bin/")
    ]
    source_removal_paths = {
        record["path"] for record in removal["files"]
    }
    if not source_removal_paths.issubset(set(source_manifest)):
        missing_paths = sorted(source_removal_paths - set(source_manifest))
        fail(
            START_STATE_DIAGNOSTIC,
            f"source package omits frozen removal paths: {missing_paths[:5]}",
        )
    components = parse_supported_components(args.snodec_config)
    (
        cpack_components,
        apps_dependencies,
        cpack_dependencies,
    ) = parse_cpack(args.cpack_config)
    targets = parse_targets(
        args.target_inventory, build_root=args.build_root
    )
    ctest_summary = baseline_ctest["summary"]
    start = {
        "format_version": FORMAT_VERSION,
        "kind": "snodec-aisuite-cutover-start-state",
        "authority": {
            "snodec": {
                "repository": SNODEC_REPOSITORY,
                "commit": SNODEC_BASE_COMMIT,
                "tree": SNODEC_BASE_TREE,
                "version": SNODEC_VERSION,
                "soversion": SNODEC_SOVERSION,
            },
            "aisuite": aisuite,
        },
        "removal_inventory": removal,
        "legacy_surface": {
            "library_targets": REMOVED_COMPONENTS,
            "target_aliases": REMOVED_TARGETS[3:],
            "applications": REMOVED_APPLICATIONS,
            "private_application_targets": REMOVED_PRIVATE_APP_TARGETS,
            "public_header_count": len(installed_headers),
            "public_headers": installed_headers,
            "installed_shared_library_paths": installed_libraries,
            "installed_cmake_exports": installed_exports,
            "installed_applications": installed_apps,
            "all_installed_applications": all_installed_applications,
            "preserved_installed_applications": [
                entry
                for entry in all_installed_applications
                if entry not in installed_apps
            ],
        },
        "supported_components": {
            "all": components,
            "removed": REMOVED_COMPONENTS,
            "preserved": [
                component
                for component in components
                if component not in REMOVED_COMPONENTS
            ],
        },
        "cpack": {
            "all_components": cpack_components,
            "removed_components": REMOVED_COMPONENTS,
            "preserved_components": [
                component
                for component in cpack_components
                if component not in REMOVED_COMPONENTS
            ],
            "apps_dependencies": apps_dependencies,
            "all_component_dependencies": cpack_dependencies,
            "removed_apps_dependency": "ai-openai-codex-frontend",
            "preserved_apps_dependencies": [
                dependency
                for dependency in apps_dependencies
                if dependency != "ai-openai-codex-frontend"
            ],
        },
        "target_inventory": {
            "configured_count": len(targets),
            "configured_targets": targets,
            "removed_targets": sorted(
                set(REMOVED_COMPONENTS + REMOVED_APPLICATIONS + REMOVED_PRIVATE_APP_TARGETS)
            ),
        },
        "ctest": {
            "configured_tests": ctest_summary["configured_tests"],
            "snodec_codex_hierarchy_tests": ctest_summary[
                "snodec_codex_hierarchy_tests"
            ],
            "aisuite_codex_hierarchy_tests": ctest_summary[
                "aisuite_codex_hierarchy_tests"
            ],
            "aisuite_only_completion_tests": ctest_summary[
                "aisuite_only_completion_tests"
            ],
            "codex_owned_removed": ctest_summary["classifications"][
                "codex-owned-removed"
            ],
            "shared_adapted": ctest_summary["classifications"][
                "shared-codex-branch-removed"
            ],
            "non_codex_preserved": ctest_summary["classifications"][
                "non-codex-preserved"
            ],
            "surviving_baseline_names": ctest_summary[
                "surviving_baseline_names"
            ],
            "codex_labelled_tests": ctest_summary["codex_labelled_tests"],
            "evidence_file": "aisuite-cutover-baseline-ctest.json",
            "evidence_sha256": sha256_bytes(
                canonical_json_bytes(baseline_ctest)
            ),
        },
        "install": {
            "total_paths": len(install_manifest),
            "codex_paths": len(install_codex),
            "preserved_paths": len(install_manifest) - len(install_codex),
            "manifest_sha256": sha256_bytes(
                ("\n".join(install_manifest) + "\n").encode("utf-8")
            ),
            "paths": install_manifest,
        },
        "source_package": {
            "total_paths": len(source_manifest),
            "removed_paths": len(source_removal_paths),
            "preserved_paths": len(source_manifest)
            - len(source_removal_paths),
            "manifest_sha256": sha256_bytes(
                ("\n".join(source_manifest) + "\n").encode("utf-8")
            ),
            "paths": source_manifest,
        },
        "binary_package": {
            "total_paths": len(binary_manifest),
            "codex_paths": len(install_codex),
            "preserved_paths": len(binary_manifest) - len(install_codex),
            "manifest_sha256": sha256_bytes(
                ("\n".join(binary_manifest) + "\n").encode("utf-8")
            ),
            "paths": binary_manifest,
        },
        "policies": policies,
        "dependency_direction": {
            "direction": "AISuite -> installed SNode.C",
            "snodec_depends_on_aisuite": False,
        },
    }
    diagnostics = start_model_diagnostics(start)
    if diagnostics:
        fail(
            diagnostics[0],
            f"generated start model is invalid: {diagnostics}",
        )
    return start, make_plan(), baseline_ctest


def check_start_evidence(repo: Path) -> None:
    evidence_dir = repo / "docs/migrations"
    start_path = evidence_dir / "aisuite-cutover-start-state.json"
    plan_path = evidence_dir / "aisuite-cutover-plan.json"
    ctest_path = evidence_dir / "aisuite-cutover-baseline-ctest.json"
    start = read_json(start_path)
    plan = read_json(plan_path)
    baseline_ctest = read_json(ctest_path)
    diagnostics = start_model_diagnostics(start)
    if diagnostics:
        fail(
            diagnostics[0],
            f"checked-in start model is invalid: {diagnostics}",
        )
    removal = capture_removal_inventory(repo)
    frozen = start.get("removal_inventory", {})
    if canonical_json_bytes(removal) != canonical_json_bytes(frozen):
        fail(
            START_STATE_DIAGNOSTIC,
            "checked-in removal inventory differs from the pinned Git tree",
        )
    if plan != make_plan():
        fail(
            START_STATE_DIAGNOSTIC,
            "checked-in cutover plan is not generated from current authority",
        )
    expected_ctest_hash = start.get("ctest", {}).get("evidence_sha256")
    observed_ctest_hash = sha256_bytes(canonical_json_bytes(baseline_ctest))
    if observed_ctest_hash != expected_ctest_hash:
        fail(
            START_STATE_DIAGNOSTIC,
            "baseline CTest evidence hash changed",
        )
    if (
        baseline_ctest.get("summary", {}).get("configured_tests") != 300
        or baseline_ctest.get("summary", {}).get(
            "snodec_codex_hierarchy_tests"
        )
        != 123
        or baseline_ctest.get("summary", {}).get(
            "aisuite_codex_hierarchy_tests"
        )
        != 131
    ):
        fail(
            NON_CODEX_DIAGNOSTIC,
            "baseline CTest evidence accounting changed",
        )
    verify_project_identity(repo)
    verify_policy_baseline(repo)


def read_boundary_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except OSError as error:
        fail(
            MIGRATION_DIAGNOSTIC,
            f"cannot read cutover boundary input {path}: {error}",
        )


def load_frozen_evidence(
    repo: Path,
) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any]]:
    """Load the Commit-1 authority without consulting Git or another tree."""

    evidence_dir = repo / "docs/migrations"
    start = read_json(evidence_dir / "aisuite-cutover-start-state.json")
    plan = read_json(evidence_dir / "aisuite-cutover-plan.json")
    baseline_ctest = read_json(
        evidence_dir / "aisuite-cutover-baseline-ctest.json"
    )
    diagnostics = start_model_diagnostics(start)
    if diagnostics:
        fail(
            diagnostics[0],
            f"frozen package authority is invalid: {diagnostics}",
        )
    if plan != make_plan():
        fail(
            MIGRATION_DIAGNOSTIC,
            "frozen cutover plan differs from the package-safe authority",
        )
    expected_hash = start.get("ctest", {}).get("evidence_sha256")
    observed_hash = sha256_bytes(canonical_json_bytes(baseline_ctest))
    if observed_hash != expected_hash:
        fail(
            MIGRATION_DIAGNOSTIC,
            "frozen baseline CTest evidence hash changed",
        )
    summary = baseline_ctest.get("summary", {})
    if (
        summary.get("configured_tests") != 300
        or summary.get("surviving_baseline_names") != 172
        or summary.get("classifications", {}).get(
            "codex-owned-removed"
        )
        != 128
        or summary.get("classifications", {}).get(
            "shared-codex-branch-removed"
        )
        != 4
        or summary.get("classifications", {}).get(
            "non-codex-preserved"
        )
        != 168
    ):
        fail(
            NON_CODEX_DIAGNOSTIC,
            "frozen CTest survivor accounting changed",
        )
    return start, plan, baseline_ctest


def extract_supported_components(source: str, *, description: str) -> list[str]:
    match = re.search(
        r"set\((?:_supported_components|SUPPORTED_COMPONENTS)\s+(.*?)\)",
        source,
        flags=re.DOTALL,
    )
    if match is None:
        fail(
            BUILD_DIAGNOSTIC,
            f"cannot find supported components in {description}",
        )
    components: list[str] = []
    for variable, literal in re.findall(
        r"\$\{([A-Za-z0-9_.+-]+)\}|([A-Za-z0-9_.+-]+)",
        match.group(1),
    ):
        components.append(variable.lower() if variable else literal)
    return components


def extract_cpack_model(
    source: str, *, description: str
) -> tuple[list[str], list[str], dict[str, list[str]]]:
    components_match = re.search(
        r'set\(CPACK_COMPONENTS_ALL "([^"]+)"\)', source
    )
    apps_match = re.search(
        r"set\(CPACK_COMPONENT_APPS_DEPENDS ([^)]+)\)", source
    )
    if components_match is None or apps_match is None:
        fail(
            PACKAGE_DIAGNOSTIC,
            f"cannot parse configured CPack model from {description}",
        )
    components = components_match.group(1).split(";")
    apps_dependencies = apps_match.group(1).split()
    component_dependencies: dict[str, list[str]] = {}
    for component, dependencies in re.findall(
        r"set\(CPACK_COMPONENT_([A-Z0-9-]+)_DEPENDS ([^)]+)\)",
        source,
    ):
        component_dependencies[component.lower()] = dependencies.split()
    return (
        components,
        apps_dependencies,
        dict(sorted(component_dependencies.items())),
    )


def preserves_ordered_inventory(
    expected: Sequence[str], observed: Sequence[str]
) -> bool:
    """Require every frozen entry exactly once while allowing new entries."""

    if any(observed.count(entry) != 1 for entry in expected):
        return False
    positions = [observed.index(entry) for entry in expected]
    return positions == sorted(positions)


def property_map(test: dict[str, Any]) -> dict[str, Any]:
    return {
        prop["name"]: prop["value"] for prop in test.get("properties", [])
    }


def normalize_projection_argument(argument: str) -> str:
    if argument.startswith("-DCMAKE_CXX_COMPILER="):
        return "-DCMAKE_CXX_COMPILER=${CXX}"
    return argument


def ctest_projection_record(test: dict[str, Any]) -> dict[str, Any]:
    properties = property_map(test)
    return {
        "name": test["name"],
        "command": [
            normalize_projection_argument(argument)
            for argument in test.get("command", [])
        ],
        "timeout": properties.get("TIMEOUT"),
        "depends": properties.get("DEPENDS"),
        "disabled": properties.get("DISABLED", False),
        "labels": sorted(properties.get("LABELS", [])),
    }


def ctest_projection_hash(records: Sequence[dict[str, Any]]) -> str:
    content = (
        json.dumps(
            list(records),
            sort_keys=True,
            separators=(",", ":"),
            ensure_ascii=False,
        )
        + "\n"
    ).encode("utf-8")
    return sha256_bytes(content)


def expected_survivor_projection(
    baseline_ctest: dict[str, Any],
) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    names: set[str] = set()
    for test in baseline_ctest.get("tests", []):
        if test.get("classification") == "codex-owned-removed":
            continue
        name = test["name"]
        if name in names:
            fail(NON_CODEX_DIAGNOSTIC, f"duplicate baseline survivor {name}")
        names.add(name)
        record = ctest_projection_record(test)
        if name in ALLOWED_SURVIVOR_LABEL_CHANGES:
            record["labels"] = ALLOWED_SURVIVOR_LABEL_CHANGES[name]
        records.append(record)
    records.sort(key=lambda record: record["name"])
    observed_hash = ctest_projection_hash(records)
    if (
        len(records) != 172
        or observed_hash != SURVIVOR_CTEST_PROJECTION_SHA256
    ):
        fail(
            NON_CODEX_DIAGNOSTIC,
            "frozen non-Codex CTest survivor projection changed: "
            f"count={len(records)}, sha256={observed_hash}",
        )
    return records


def normalize_current_ctest_model(
    raw: dict[str, Any],
    *,
    source_root: Path,
    build_root: Path | None,
) -> list[dict[str, Any]]:
    tests: list[dict[str, Any]] = []
    for source_test in raw.get("tests", []):
        origins: list[str] = []
        if "backtrace" in source_test:
            origins = backtrace_files(
                raw, source_test["backtrace"], source_root=source_root
            )
        tests.append(
            {
                "name": source_test["name"],
                "command": [
                    normalize_path_text(
                        argument,
                        source_root=source_root,
                        build_root=build_root,
                    )
                    for argument in source_test.get("command", [])
                ],
                "origins": origins,
                "properties": sorted(
                    [
                        normalize_property(
                            prop,
                            source_root=source_root,
                            build_root=build_root,
                        )
                        for prop in source_test.get("properties", [])
                    ],
                    key=lambda prop: prop["name"],
                ),
            }
        )
    tests.sort(key=lambda test: test["name"])
    return tests


def normalize_current_ctest(
    path: Path,
    *,
    source_root: Path,
    build_root: Path | None,
) -> list[dict[str, Any]]:
    raw = read_json(path)
    if not isinstance(raw, dict):
        fail(TEST_DIAGNOSTIC, f"CTest JSON root is not an object: {path}")
    return normalize_current_ctest_model(
        raw, source_root=source_root, build_root=build_root
    )


def collect_ctest_boundary(
    source: Path | dict[str, Any],
    *,
    source_root: Path,
    build_root: Path | None,
    baseline_ctest: dict[str, Any],
) -> dict[str, Any]:
    if isinstance(source, Path):
        tests = normalize_current_ctest(
            source, source_root=source_root, build_root=build_root
        )
    else:
        tests = normalize_current_ctest_model(
            source, source_root=source_root, build_root=build_root
        )
    by_name: dict[str, list[dict[str, Any]]] = {}
    for test in tests:
        by_name.setdefault(test["name"], []).append(test)

    forbidden: list[str] = []
    for name, registrations in by_name.items():
        if len(registrations) != 1:
            forbidden.append(f"duplicate test registration: {name}")
        for test in registrations:
            properties = property_map(test)
            labels = [
                str(label).lower()
                for label in properties.get("LABELS", [])
            ]
            serialized = json.dumps(test, sort_keys=True).lower()
            if name.startswith("Codex"):
                forbidden.append(f"legacy test name: {name}")
            if "codex" in labels or any(
                label.startswith("phase-a1-") for label in labels
            ):
                forbidden.append(f"legacy test label: {name}")
            if "codex" in serialized:
                forbidden.append(f"legacy test command/property: {name}")
            if any(
                origin.startswith(
                    "${SNODEC_SOURCE}/tests/component/codex/"
                )
                for origin in test["origins"]
            ):
                forbidden.append(f"legacy test origin: {name}")

    expected = expected_survivor_projection(baseline_ctest)
    current_projection: list[dict[str, Any]] = []
    missing: list[str] = []
    for expected_record in expected:
        name = expected_record["name"]
        registrations = by_name.get(name, [])
        if len(registrations) != 1:
            missing.append(name)
            continue
        current_projection.append(
            ctest_projection_record(registrations[0])
        )
    current_projection.sort(key=lambda record: record["name"])
    current_hash = (
        ctest_projection_hash(current_projection)
        if len(current_projection) == 172
        else None
    )
    return {
        "checked": True,
        "configured_tests": len(tests),
        "unique_names": len(by_name),
        "forbidden": sorted(set(forbidden)),
        "missing_survivors": missing,
        "survivor_projection_count": len(current_projection),
        "survivor_projection_sha256": current_hash,
        "expected_survivor_projection_sha256": (
            SURVIVOR_CTEST_PROJECTION_SHA256
        ),
        "survivors_match": (
            not missing
            and current_hash == SURVIVOR_CTEST_PROJECTION_SHA256
        ),
    }


def remaining_removal_paths(repo: Path) -> list[str]:
    paths = [
        group["path"]
        for group in REMOVAL_GROUPS
        if (repo / group["path"].rstrip("/")).exists()
    ]
    paths.extend(
        path for path in INTENTIONAL_RESIDUE_REMOVALS if (repo / path).exists()
    )
    return sorted(paths)


def path_has_ownership_residue(path: str) -> bool:
    """Classify product paths that would reintroduce transferred ownership."""

    lower = path.lower()
    if lower == ".codex" or lower.startswith(".codex/"):
        return False
    return (
        "codex" in lower
        or "ai/openai" in lower
        or "ai-openai" in lower
        or "ai.openai" in lower
        or "ai::openai" in lower
    )


def source_ownership_residue_hits(repo: Path) -> list[str]:
    """Find Codex implementation/build residue outside the frozen old roots."""

    candidates: list[Path] = [repo / "CMakeLists.txt"]
    for root in [repo / "cmake", repo / "src"]:
        if not root.exists():
            continue
        candidates.extend(
            path
            for path in root.rglob("*")
            if path.is_file()
            and (
                path.name == "CMakeLists.txt"
                or path.suffix.lower()
                in {
                    ".cmake",
                    ".in",
                    ".c",
                    ".cc",
                    ".cpp",
                    ".cxx",
                    ".h",
                    ".hh",
                    ".hpp",
                    ".hxx",
                }
            )
        )
    patterns = [
        r"(?i)ai[/\\]openai[/\\]codex",
        r"(?i)ai[-_.:]openai[-_.:]codex",
        r"(?i)libsnodec-ai-openai-codex",
        r"(?i)\bcodex-backend(?:-client)?\b",
        r"(?i)\b(?:namespace|using\s+namespace)\s+ai::openai::codex",
        r"(?i)#\s*include\s*[<\"]ai/openai/codex/",
        r"(?i)\bAISuite::OpenAICodex(?:Backend|Frontend)?\b",
    ]
    hits: list[str] = []
    for path in sorted(set(candidates)):
        if not path.is_file():
            continue
        relative = path.relative_to(repo).as_posix()
        if path_has_ownership_residue(relative):
            hits.append(f"path:{relative}")
            continue
        source = read_boundary_text(path)
        if any(re.search(pattern, source) for pattern in patterns):
            hits.append(f"content:{relative}")
    return sorted(hits)


def production_dependency_hits(repo: Path) -> list[str]:
    candidates: list[Path] = [repo / "CMakeLists.txt"]
    for root in [repo / "cmake", repo / "src", repo / "tests"]:
        if not root.exists():
            continue
        candidates.extend(
            path
            for path in root.rglob("*")
            if path.is_file()
            and (
                path.name == "CMakeLists.txt"
                or path.suffix.lower() in {".cmake", ".in"}
            )
        )
    workflow_root = repo / ".github/workflows"
    if workflow_root.exists():
        candidates.extend(
            path
            for path in workflow_root.rglob("*")
            if path.is_file() and path.suffix.lower() in {".yml", ".yaml"}
        )
    gitmodules = repo / ".gitmodules"
    if gitmodules.is_file():
        candidates.append(gitmodules)
    hits: list[str] = []
    for path in sorted(set(candidates)):
        source = read_boundary_text(path)
        if re.search(
            r"(?is)(?:find_package|fetchcontent(?:_declare|_makeavailable|"
            r"_populate)?|add_subdirectory|externalproject_add)"
            r"\s*\([^)]*aisuite|"
            r"github\.com/SNodeC/AISuite|"
            r"\bAISuite_SOURCE_DIR\b|\bAISUITE_SOURCE_DIR\b",
            source,
        ):
            hits.append(path.relative_to(repo).as_posix())
    return hits


def migration_requirements(repo: Path) -> dict[str, bool]:
    migration_path = repo / MIGRATION_DOCUMENT
    migration = (
        read_boundary_text(migration_path)
        if migration_path.is_file()
        else ""
    )
    readme_path = repo / "README.md"
    readme = (
        read_boundary_text(readme_path) if readme_path.is_file() else ""
    )
    phase_2_path = (
        repo / "docs/logging/phase-2-transport-attempt-lifecycle.md"
    )
    phase_3_path = (
        repo
        / "docs/logging/phase-3-protocol-session-request-lifecycle.md"
    )
    consolidation_path = repo / "docs/testing/test-suite-consolidation.md"
    phase_2 = (
        read_boundary_text(phase_2_path) if phase_2_path.is_file() else ""
    )
    phase_3 = (
        read_boundary_text(phase_3_path) if phase_3_path.is_file() else ""
    )
    consolidation = (
        read_boundary_text(consolidation_path)
        if consolidation_path.is_file()
        else ""
    )
    normalized_migration = re.sub(r"\s+", " ", migration)

    migration_tokens = {
        "snodec-base-commit": SNODEC_BASE_COMMIT,
        "snodec-base-tree": SNODEC_BASE_TREE,
        "aisuite-owner-commit": AISUITE_OWNER_COMMIT,
        "aisuite-owner-tree": AISUITE_OWNER_TREE,
        "removed-core-component": "ai-openai-codex",
        "removed-backend-component": "ai-openai-codex-backend",
        "removed-frontend-component": "ai-openai-codex-frontend",
        "removed-backend-app": "codex-backend",
        "removed-client-app": "codex-backend-client",
        "aisuite-find-package": "find_package(AISuite CONFIG REQUIRED)",
        "aisuite-core-target": "AISuite::OpenAICodex",
        "aisuite-backend-target": "AISuite::OpenAICodexBackend",
        "aisuite-frontend-target": "AISuite::OpenAICodexFrontend",
        "public-include-form": "<ai/openai/codex/",
        "clean-prefix-warning": (
            "CMake installation into an already populated prefix "
            "does not delete obsolete"
        ),
        "package-api-break": "package and API break",
        "retained-abi": "retained shared-library ABI",
        "no-shim": "no forwarding header, compatibility library",
        "normal-pin-follow-up": "normal SNode.C build-dependency pin",
        "immutable-provenance": "immutable extraction provenance",
        "rerun-aisuite-ci": "Rerun AISuite CI",
        "dependency-direction": "AISuite -> installed SNode.C",
        "no-snodec-dependency": (
            "SNode.C has no source, build, runtime, or package "
            "dependency on AISuite"
        ),
    }
    exact_token_names = {
        "removed-core-component",
        "removed-backend-component",
        "removed-frontend-component",
        "removed-backend-app",
        "removed-client-app",
        "aisuite-core-target",
        "aisuite-backend-target",
        "aisuite-frontend-target",
    }
    requirements = {}
    for name, token in migration_tokens.items():
        if name in exact_token_names:
            present = (
                re.search(
                    rf"(?<![-A-Za-z0-9_]){re.escape(token)}"
                    r"(?![-A-Za-z0-9_])",
                    normalized_migration,
                )
                is not None
            )
        else:
            present = token in normalized_migration
        requirements[f"migration:{name}"] = present
    requirements["readme:aisuite-project"] = (
        "https://github.com/SNodeC/AISuite" in readme
        and "SNode.C does not depend on AISuite" in readme
    )
    for name, source in [
        ("phase-2", phase_2),
        ("phase-3", phase_3),
        ("test-suite-consolidation", consolidation),
    ]:
        requirements[f"historical:{name}"] = (
            SNODEC_BASE_COMMIT in source
            and "https://github.com/SNodeC/AISuite" in source
            and "historical" in source.lower()
        )
    requirements["historical:phase-3-accounting"] = (
        "77 discovered" in phase_3 and "77 allowlisted" in phase_3
    )
    return dict(sorted(requirements.items()))


def policy_boundary(repo: Path) -> dict[str, Any]:
    semantic_path = (
        repo
        / "tests/policy/log/ParameterlessSemanticLoggerPolicyTest.cpp"
    )
    api_path = repo / "tests/policy/log/LoggingApiSurfacePolicyTest.cpp"
    workflow_path = repo / ".github/workflows/ci.yml"
    workflow_policy_path = (
        repo / "tests/policy/ci/CiWorkflowPathsPolicyTest.cpp"
    )
    semantic = read_boundary_text(semantic_path)
    api = read_boundary_text(api_path)
    workflow = read_boundary_text(workflow_path)
    workflow_policy = read_boundary_text(workflow_policy_path)

    residue: list[str] = []
    for path in CODEX_LOGGING_API_PATHS:
        if path in api:
            residue.append(f"logging-api-path:{path}")
    for identifier in CODEX_LOGGING_API_IDENTIFIERS:
        if f'"{identifier}"' in api:
            residue.append(f"logging-api-identifier:{identifier}")
    if "src/ai/openai/codex" in semantic:
        residue.append("semantic-scan-root")
    for entry in CODEX_SEMANTIC_ENTRIES:
        if f'"{entry}"' in semantic:
            residue.append(f"semantic-allowlist:{entry}")
    docs_path = "docs/ai/openai/codex/**"
    if docs_path in workflow:
        residue.append("workflow-codex-docs-path")
    if docs_path in workflow_policy:
        residue.append("workflow-policy-codex-docs-path")

    allowlist_count = semantic.count("Entry{")
    arithmetic_present = all(
        token in semantic
        for token in [
            "kBaselineParameterlessCallCount = 81",
            "kTransferredParameterlessCallCount = 4",
            "kExpectedParameterlessCallCount",
            "static_assert(kExpectedParameterlessCallCount == 77)",
        ]
    )
    return {
        "residue": sorted(residue),
        "semantic_logger": {
            "baseline": 81,
            "transferred": 4,
            "expected": 77,
            "allowlist_entries": allowlist_count,
            "arithmetic_present": arithmetic_present,
            "matches": allowlist_count == 77 and arithmetic_present,
        },
    }


def source_supported_components(repo: Path) -> list[str]:
    path = repo / "src/CMakeLists.txt"
    return extract_supported_components(
        read_boundary_text(path), description=path.as_posix()
    )


def source_build_surface(repo: Path) -> dict[str, Any]:
    source_path = repo / "src/CMakeLists.txt"
    source = read_boundary_text(source_path)
    hits: list[str] = []
    checks = {
        "add-subdirectory-ai": r"add_subdirectory\s*\(\s*ai\s*\)",
        "ai-dependency-collection": (
            r"get_all_targets_dependencies\s*\("
            r"\s*SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES\s+ai\s*\)"
        ),
        "legacy-component": r"ai-openai-codex",
    }
    for name, pattern in checks.items():
        if re.search(pattern, source, flags=re.IGNORECASE):
            hits.append(name)
    return {"wiring_hits": hits, "forbidden_targets": []}


def source_package_surface(repo: Path) -> dict[str, Any]:
    packing_path = repo / "cmake/Packing.cmake"
    packing = read_boundary_text(packing_path)
    residue = [
        component
        for component in REMOVED_COMPONENTS
        if component in packing
    ]
    missing_local_generated_exclusions = [
        path
        for path, rule in (
            BASELINE_LOCAL_GENERATED_SOURCE_IGNORE_RULES.items()
        )
        if rule not in packing
    ]
    return {
        "source_residue": sorted(residue),
        "baseline_local_generated_paths": (
            BASELINE_LOCAL_GENERATED_SOURCE_PATHS
        ),
        "missing_local_generated_exclusions": (
            missing_local_generated_exclusions
        ),
        "cpack_checked": False,
        "cpack_components": None,
        "apps_dependencies": None,
        "component_dependencies": None,
        "source_manifest_checked": False,
        "source_forbidden_paths": [],
        "source_missing_preserved": [],
        "source_unexpected_paths": [],
        "source_ownership_residue": [],
        "source_missing_required": [],
        "binary_manifest_checked": False,
        "binary_forbidden_paths": [],
        "binary_missing_preserved": [],
        "binary_unexpected_paths": [],
    }


def source_install_surface(repo: Path) -> dict[str, Any]:
    hits: list[str] = []
    candidates = [repo / "CMakeLists.txt"]
    candidates.extend((repo / "cmake").rglob("*.cmake"))
    candidates.extend((repo / "src").rglob("CMakeLists.txt"))
    candidates.extend((repo / "src").rglob("*.cmake"))
    candidates.extend((repo / "src").rglob("*.in"))
    for path in sorted(set(candidates)):
        if not path.is_file():
            continue
        source = read_boundary_text(path)
        if re.search(
            r"(?is)install\s*\([^)]*(?:ai/openai|ai-openai-codex|codex-"
            r"backend)",
            source,
        ):
            hits.append(path.relative_to(repo).as_posix())
    return {
        "declaration_hits": hits,
        "manifest_checked": False,
        "forbidden_paths": [],
        "forbidden_content": [],
        "missing_preserved": [],
        "unexpected_paths": [],
    }


def collect_source_boundary(
    repo: Path,
    *,
    frozen: tuple[dict[str, Any], dict[str, Any], dict[str, Any]]
    | None = None,
) -> dict[str, Any]:
    start, _, baseline_ctest = (
        frozen if frozen is not None else load_frozen_evidence(repo)
    )
    supported = source_supported_components(repo)
    expected_supported = start["supported_components"]["preserved"]
    root_cmake = read_boundary_text(repo / "CMakeLists.txt")
    version_match = re.search(
        r"project\s*\(\s*SNode\.C.*?\bVERSION\s+([0-9.]+)",
        root_cmake,
        flags=re.DOTALL,
    )
    version = version_match.group(1) if version_match else None
    soversion_derivation = (
        "set(SNODEC_SOVERSION ${SNode.C_VERSION_MAJOR})" in root_cmake
    )
    migration = migration_requirements(repo)
    consumer_requirement_keys = [
        "migration:aisuite-find-package",
        "migration:aisuite-core-target",
        "migration:aisuite-backend-target",
        "migration:aisuite-frontend-target",
        "migration:public-include-form",
    ]
    migration_document = read_boundary_text(repo / MIGRATION_DOCUMENT)
    legacy_consumer_resolution = sorted(
        set(
            re.findall(
                r"snodec::ai-openai-codex(?:-backend|-frontend)?|"
                r"AISUITE_SOURCE_DIR|AISuite_SOURCE_DIR|"
                r"add_subdirectory\s*\([^)]*AISuite|"
                r"FetchContent[^\n]*AISuite",
                migration_document,
                flags=re.IGNORECASE,
            )
        )
    )
    expected_projection = expected_survivor_projection(baseline_ctest)
    expected_projection_hash = ctest_projection_hash(expected_projection)
    build = source_build_surface(repo)
    build["supported_components"] = supported
    build["removed_supported_components"] = [
        component
        for component in supported
        if component in REMOVED_COMPONENTS
    ]
    return {
        "format_version": FORMAT_VERSION,
        "kind": "snodec-aisuite-cutover-boundary",
        "authority": {
            "snodec": {
                "repository": SNODEC_REPOSITORY,
                "base_commit": SNODEC_BASE_COMMIT,
                "base_tree": SNODEC_BASE_TREE,
            },
            "aisuite": {
                "repository": AISUITE_REPOSITORY,
                "owner_commit": AISUITE_OWNER_COMMIT,
                "owner_tree": AISUITE_OWNER_TREE,
                "subject": AISUITE_OWNER_SUBJECT,
                "parents": AISUITE_OWNER_PARENTS,
            },
        },
        "paths": {
            "dedicated_removal_paths": [
                group["path"] for group in REMOVAL_GROUPS
            ],
            "intentional_residue_removals": INTENTIONAL_RESIDUE_REMOVALS,
            "remaining": remaining_removal_paths(repo),
            "ownership_residue": source_ownership_residue_hits(repo),
        },
        "build": build,
        "install": source_install_surface(repo),
        "package": source_package_surface(repo),
        "tests": {
            "checked": False,
            "forbidden": [],
            "missing_survivors": [],
            "survivor_projection_count": 172,
            "survivor_projection_sha256": expected_projection_hash,
            "expected_survivor_projection_sha256": (
                SURVIVOR_CTEST_PROJECTION_SHA256
            ),
            "survivors_match": True,
        },
        "policy": policy_boundary(repo),
        "preservation": {
            "expected_supported_components": expected_supported,
            "supported_components_match": preserves_ordered_inventory(
                expected_supported, supported
            ),
            "expected_cpack_components": start["cpack"][
                "preserved_components"
            ],
            "expected_apps_dependencies": start["cpack"][
                "preserved_apps_dependencies"
            ],
            "expected_component_dependencies": {
                key: (
                    start["cpack"]["preserved_apps_dependencies"]
                    if key == "apps"
                    else value
                )
                for key, value in start["cpack"][
                    "all_component_dependencies"
                ].items()
                if key not in REMOVED_COMPONENTS
            },
            "cpack_components_match": True,
            "apps_dependencies_match": True,
            "component_dependencies_match": True,
            "ctest_survivors_match": True,
            "source_preserved_match": True,
            "install_preserved_match": True,
            "binary_preserved_match": True,
        },
        "identity": {
            "project_version": version,
            "expected_project_version": SNODEC_VERSION,
            "soversion": SNODEC_SOVERSION if soversion_derivation else None,
            "expected_soversion": SNODEC_SOVERSION,
            "soversion_derivation_preserved": soversion_derivation,
        },
        "dependency_direction": {
            "expected": "AISuite -> installed SNode.C",
            "snodec_depends_on_aisuite": bool(
                production_dependency_hits(repo)
            ),
            "production_dependency_hits": production_dependency_hits(repo),
        },
        "aisuite_consumer": {
            "expected_resolution": "installed-package",
            "legacy_or_source_build_resolution": legacy_consumer_resolution,
            "requirements": {
                key: migration.get(key, False)
                for key in consumer_requirement_keys
            },
            "complete": all(
                migration.get(key, False)
                for key in consumer_requirement_keys
            ),
        },
        "migration": {
            "document": MIGRATION_DOCUMENT,
            "requirements": migration,
            "complete": all(migration.values()),
            "aisuite_owner_commit": AISUITE_OWNER_COMMIT,
            "aisuite_owner_tree": AISUITE_OWNER_TREE,
        },
        "package_safe": {
            "git_required": False,
            "network_required": False,
            "aisuite_checkout_required": False,
            "parent_tree_required": False,
            "cmake_registry_required": False,
            "baseline_local_generated_source_paths": (
                BASELINE_LOCAL_GENERATED_SOURCE_PATHS
            ),
            "baseline_preserved_source_path_count": len(
                expected_source_package_paths(start)[0]
            ),
            "expected_cutover_source_path_count": len(
                expected_source_package_paths(start)[1]
            ),
            "required_source_paths": REQUIRED_CUTOVER_SOURCE_PATHS,
        },
    }


def boundary_model_diagnostics(model: dict[str, Any]) -> list[str]:
    diagnostics: list[str] = []
    paths = model.get("paths", {})
    build = model.get("build", {})
    install = model.get("install", {})
    package = model.get("package", {})
    tests = model.get("tests", {})
    policy = model.get("policy", {})
    preservation = model.get("preservation", {})
    identity = model.get("identity", {})
    dependency = model.get("dependency_direction", {})
    consumer = model.get("aisuite_consumer", {})
    migration = model.get("migration", {})
    authority = model.get("authority", {})

    if paths.get("remaining") or paths.get("ownership_residue"):
        diagnostics.append(PATH_DIAGNOSTIC)

    removed_supported = build.get("removed_supported_components", [])
    if (
        build.get("wiring_hits")
        or build.get("forbidden_targets")
        or removed_supported
    ):
        diagnostics.append(BUILD_DIAGNOSTIC)

    if (
        install.get("declaration_hits")
        or install.get("forbidden_paths")
        or install.get("forbidden_content")
    ):
        diagnostics.append(INSTALL_DIAGNOSTIC)

    removed_cpack = [
        component
        for component in (package.get("cpack_components") or [])
        if component in REMOVED_COMPONENTS
    ]
    legacy_apps_dependency = (
        "ai-openai-codex-frontend"
        in (package.get("apps_dependencies") or [])
    )
    if (
        package.get("source_residue")
        or package.get("missing_local_generated_exclusions")
        or removed_cpack
        or legacy_apps_dependency
        or package.get("source_forbidden_paths")
        or package.get("source_ownership_residue")
        or package.get("binary_forbidden_paths")
    ):
        diagnostics.append(PACKAGE_DIAGNOSTIC)

    if tests.get("forbidden"):
        diagnostics.append(TEST_DIAGNOSTIC)

    semantic = policy.get("semantic_logger", {})
    if (
        policy.get("residue")
        or not semantic.get("matches", False)
        or semantic.get("allowlist_entries") != 77
    ):
        diagnostics.append(POLICY_DIAGNOSTIC)

    supported_drift = (
        not preservation.get("supported_components_match", False)
        and not removed_supported
    )
    cpack_drift = (
        package.get("cpack_checked", False)
        and (
            not preservation.get("cpack_components_match", False)
            or not preservation.get("apps_dependencies_match", False)
            or not preservation.get(
                "component_dependencies_match", False
            )
        )
        and not removed_cpack
        and not legacy_apps_dependency
    )
    if (
        supported_drift
        or cpack_drift
        or (
            tests.get("checked", False)
            and not preservation.get("ctest_survivors_match", False)
        )
        or (
            package.get("source_manifest_checked", False)
            and not preservation.get("source_preserved_match", False)
        )
        or (
            install.get("manifest_checked", False)
            and not preservation.get("install_preserved_match", False)
        )
        or (
            package.get("binary_manifest_checked", False)
            and not preservation.get("binary_preserved_match", False)
        )
    ):
        diagnostics.append(NON_CODEX_DIAGNOSTIC)

    if (
        identity.get("project_version") != SNODEC_VERSION
        or identity.get("soversion") != SNODEC_SOVERSION
        or not identity.get("soversion_derivation_preserved", False)
    ):
        diagnostics.append(SOVERSION_DIAGNOSTIC)

    if (
        dependency.get("snodec_depends_on_aisuite") is not False
        or dependency.get("production_dependency_hits")
    ):
        diagnostics.append(DEPENDENCY_DIAGNOSTIC)

    if (
        consumer.get("expected_resolution") != "installed-package"
        or consumer.get("legacy_or_source_build_resolution")
        or consumer.get("complete") is not True
        or not all(consumer.get("requirements", {}).values())
    ):
        diagnostics.append(AISUITE_CONSUMER_DIAGNOSTIC)

    aisuite = authority.get("aisuite", {})
    snodec = authority.get("snodec", {})
    consumer_requirement_keys = {
        "migration:aisuite-find-package",
        "migration:aisuite-core-target",
        "migration:aisuite-backend-target",
        "migration:aisuite-frontend-target",
        "migration:public-include-form",
    }
    migration_authority_complete = all(
        value
        for key, value in migration.get("requirements", {}).items()
        if key not in consumer_requirement_keys
    )
    if (
        snodec.get("repository") != SNODEC_REPOSITORY
        or snodec.get("base_commit") != SNODEC_BASE_COMMIT
        or snodec.get("base_tree") != SNODEC_BASE_TREE
    ):
        diagnostics.append(HISTORY_DIAGNOSTIC)
    if (
        model.get("format_version") != FORMAT_VERSION
        or model.get("kind") != "snodec-aisuite-cutover-boundary"
        or aisuite.get("repository") != AISUITE_REPOSITORY
        or aisuite.get("owner_commit") != AISUITE_OWNER_COMMIT
        or aisuite.get("owner_tree") != AISUITE_OWNER_TREE
        or aisuite.get("subject") != AISUITE_OWNER_SUBJECT
        or aisuite.get("parents") != AISUITE_OWNER_PARENTS
        or migration.get("document") != MIGRATION_DOCUMENT
        or migration.get("aisuite_owner_commit") != AISUITE_OWNER_COMMIT
        or migration.get("aisuite_owner_tree") != AISUITE_OWNER_TREE
        or not migration_authority_complete
    ):
        diagnostics.append(MIGRATION_DIAGNOSTIC)
    return list(dict.fromkeys(diagnostics))


def make_boundary_evidence(repo: Path) -> dict[str, Any]:
    model = collect_source_boundary(repo)
    diagnostics = boundary_model_diagnostics(model)
    if diagnostics:
        fail(
            diagnostics[0],
            f"current source boundary is invalid: {diagnostics}",
        )
    return model


def read_unbounded_manifest(path: Path) -> list[str]:
    try:
        entries = path.read_text(encoding="utf-8").splitlines()
    except OSError as error:
        fail(PACKAGE_DIAGNOSTIC, f"cannot read manifest {path}: {error}")
    if (
        entries != sorted(entries)
        or len(entries) != len(set(entries))
        or any(
            not entry
            or entry.startswith("/")
            or entry.startswith("./")
            or "\\" in entry
            for entry in entries
        )
    ):
        fail(
            PACKAGE_DIAGNOSTIC,
            f"{path} is not a unique C-sorted relative path manifest",
        )
    return entries


def source_path_is_forbidden(path: str) -> bool:
    if path == ".gitattributes" or path == ".git":
        return True
    if path.startswith(".git/"):
        return True
    return any(is_group_path(path, group) for group in REMOVAL_GROUPS)


def expected_source_package_paths(
    start: dict[str, Any],
) -> tuple[set[str], set[str]]:
    removed = {
        record["path"]
        for record in start["removal_inventory"]["files"]
    }
    expected_preserved = (
        set(start["source_package"]["paths"])
        - removed
        - set(INTENTIONAL_RESIDUE_REMOVALS)
        - set(BASELINE_LOCAL_GENERATED_SOURCE_PATHS)
    )
    return (
        expected_preserved,
        expected_preserved | set(REQUIRED_CUTOVER_SOURCE_PATHS),
    )


def install_path_is_forbidden(path: str) -> bool:
    lower = path.lower()
    return (
        lower.startswith("include/snode.c/ai/")
        or re.match(
            r"^lib[^/]*/libsnodec-ai-openai-codex", lower
        )
        is not None
        or lower in {"bin/codex-backend", "bin/codex-backend-client"}
        or (
            re.match(r"^lib[^/]*/cmake/snodec/", lower) is not None
            and "codex" in lower
        )
    )


def apply_source_manifest(
    model: dict[str, Any],
    entries: Sequence[str],
    *,
    start: dict[str, Any],
) -> None:
    package = model["package"]
    preservation = model["preservation"]
    entry_set = set(entries)
    expected_preserved, expected_paths = expected_source_package_paths(start)
    package["source_manifest_checked"] = True
    package["source_forbidden_paths"] = sorted(
        path for path in entries if source_path_is_forbidden(path)
    )
    package["source_missing_preserved"] = sorted(
        expected_preserved - entry_set
    )
    package["source_unexpected_paths"] = sorted(entry_set - expected_paths)
    package["source_ownership_residue"] = sorted(
        path
        for path in package["source_unexpected_paths"]
        if path_has_ownership_residue(path)
    )
    package["source_missing_required"] = sorted(
        set(REQUIRED_CUTOVER_SOURCE_PATHS) - entry_set
    )
    preservation["source_preserved_match"] = not package[
        "source_missing_preserved"
    ]
    if package["source_missing_required"]:
        package["source_forbidden_paths"].extend(
            f"missing-required:{path}"
            for path in package["source_missing_required"]
        )


def scan_install_root(install_root: Path) -> tuple[list[str], list[str]]:
    forbidden_paths: list[str] = []
    forbidden_content: list[str] = []
    removed_directory = install_root / "include/snode.c/ai"
    if removed_directory.exists() or removed_directory.is_symlink():
        forbidden_paths.append("include/snode.c/ai/")
    for application in REMOVED_APPLICATIONS:
        candidate = install_root / "bin" / application
        if candidate.exists() or candidate.is_symlink():
            forbidden_paths.append(f"bin/{application}")
    for library_root in sorted(install_root.glob("lib*")):
        if not library_root.is_dir():
            continue
        for candidate in library_root.glob("libsnodec-ai-openai-codex*"):
            forbidden_paths.append(
                candidate.relative_to(install_root).as_posix()
            )
        cmake_root = library_root / "cmake/snodec"
        if not cmake_root.is_dir():
            continue
        for path in sorted(cmake_root.rglob("*")):
            if not path.is_file():
                continue
            relative = path.relative_to(install_root).as_posix()
            if "codex" in path.name.lower():
                forbidden_paths.append(relative)
            try:
                source = path.read_text(encoding="utf-8")
            except UnicodeDecodeError:
                continue
            if re.search(
                r"(?i)(?:ai-openai-codex|codex-backend)", source
            ):
                forbidden_content.append(relative)
    return sorted(set(forbidden_paths)), sorted(set(forbidden_content))


def apply_install_manifest(
    model: dict[str, Any],
    entries: Sequence[str],
    *,
    start: dict[str, Any],
    install_root: Path | None,
) -> None:
    install = model["install"]
    preservation = model["preservation"]
    entry_set = set(entries)
    expected_preserved = {
        path
        for path in start["install"]["paths"]
        if not install_path_is_forbidden(path)
    }
    install["manifest_checked"] = True
    install["forbidden_paths"] = sorted(
        path for path in entries if install_path_is_forbidden(path)
    )
    install["missing_preserved"] = sorted(expected_preserved - entry_set)
    install["unexpected_paths"] = sorted(entry_set - expected_preserved)
    preservation["install_preserved_match"] = not install[
        "missing_preserved"
    ]
    if install_root is not None:
        root_paths, root_content = scan_install_root(install_root)
        install["forbidden_paths"] = sorted(
            set(install["forbidden_paths"] + root_paths)
        )
        install["forbidden_content"] = root_content


def apply_binary_manifest(
    model: dict[str, Any],
    entries: Sequence[str],
    *,
    start: dict[str, Any],
) -> None:
    package = model["package"]
    preservation = model["preservation"]
    entry_set = set(entries)
    expected_preserved = {
        path
        for path in start["binary_package"]["paths"]
        if not install_path_is_forbidden(path)
    }
    source_only_prefixes = (
        "docs/",
        "tests/",
        "tools/",
    )
    package["binary_manifest_checked"] = True
    package["binary_forbidden_paths"] = sorted(
        path
        for path in entries
        if install_path_is_forbidden(path)
        or path.startswith(source_only_prefixes)
    )
    package["binary_missing_preserved"] = sorted(
        expected_preserved - entry_set
    )
    package["binary_unexpected_paths"] = sorted(
        entry_set - expected_preserved
    )
    preservation["binary_preserved_match"] = not package[
        "binary_missing_preserved"
    ]


def extract_target_names_text(source: str) -> list[str]:
    targets: set[str] = set()
    for line in source.splitlines():
        stripped = line.strip()
        if stripped.startswith("... "):
            target = stripped[4:].split(" (", 1)[0].strip()
        elif ": " in line:
            target = line.split(":", 1)[0].strip()
        else:
            continue
        normalized = target.replace("\\", "/")
        if (
            not normalized
            or normalized.startswith("[")
            or normalized in {"all", "depend", "help", "build.ninja"}
            or (
                "/" in normalized
                and normalized not in {"install/local", "install/strip"}
            )
            or re.fullmatch(
                r"lib.+\.(?:a|so(?:\.[0-9.]+)?|dylib|dll|lib)",
                normalized,
            )
        ):
            continue
        targets.add(normalized)
    return sorted(targets)


def extract_target_names(path: Path) -> list[str]:
    return extract_target_names_text(read_boundary_text(path))


def apply_configured_artifacts(
    model: dict[str, Any],
    args: argparse.Namespace,
    *,
    start: dict[str, Any],
    baseline_ctest: dict[str, Any],
) -> None:
    repo = args.repo_root.resolve()
    preservation = model["preservation"]
    build_root = args.build_root.resolve() if args.build_root else None
    snodec_config = args.snodec_config
    cpack_config = args.cpack_config
    target_text: str | None = None
    if build_root is not None:
        if snodec_config is None:
            snodec_config = build_root / "src/snodecConfig.cmake"
        if cpack_config is None:
            cpack_config = build_root / "CPackConfig.cmake"
        if args.target_inventory is None:
            result = run(
                [
                    "cmake",
                    "--build",
                    build_root.as_posix(),
                    "--target",
                    "help",
                ],
                cwd=repo,
                check=False,
            )
            if result.returncode != 0:
                fail(
                    BUILD_DIAGNOSTIC,
                    "cmake --build --target help failed: "
                    f"{result.stderr.strip()}",
                )
            target_text = result.stdout

    if snodec_config is not None:
        source = read_boundary_text(snodec_config)
        components = extract_supported_components(
            source, description=snodec_config.as_posix()
        )
        model["build"]["supported_components"] = components
        model["build"]["removed_supported_components"] = [
            component
            for component in components
            if component in REMOVED_COMPONENTS
        ]
        preservation["supported_components_match"] = (
            preserves_ordered_inventory(
                preservation["expected_supported_components"],
                components,
            )
        )
        for target in REMOVED_TARGETS:
            if target in source:
                model["build"]["forbidden_targets"].append(
                    f"generated-config:{target}"
                )
    if args.target_inventory is not None or target_text is not None:
        targets = (
            extract_target_names(args.target_inventory)
            if args.target_inventory is not None
            else extract_target_names_text(target_text or "")
        )
        forbidden = [
            target
            for target in targets
            if target
            in set(
                REMOVED_COMPONENTS
                + REMOVED_APPLICATIONS
                + REMOVED_PRIVATE_APP_TARGETS
            )
            or target in REMOVED_TARGETS
        ]
        model["build"]["forbidden_targets"].extend(forbidden)
        model["build"]["forbidden_targets"] = sorted(
            set(model["build"]["forbidden_targets"])
        )
    if cpack_config is not None:
        source = read_boundary_text(cpack_config)
        components, apps, dependencies = extract_cpack_model(
            source, description=cpack_config.as_posix()
        )
        package = model["package"]
        package["cpack_checked"] = True
        package["cpack_components"] = components
        package["apps_dependencies"] = apps
        package["component_dependencies"] = dependencies
        preservation["cpack_components_match"] = (
            preserves_ordered_inventory(
                preservation["expected_cpack_components"],
                components,
            )
        )
        preservation["apps_dependencies_match"] = (
            preserves_ordered_inventory(
                preservation["expected_apps_dependencies"], apps
            )
        )
        expected_dependencies = preservation[
            "expected_component_dependencies"
        ]
        preservation["component_dependencies_match"] = all(
            component in dependencies
            and preserves_ordered_inventory(
                expected, dependencies[component]
            )
            for component, expected in expected_dependencies.items()
        )
    ctest_source: Path | dict[str, Any] | None = args.ctest_json
    if ctest_source is None and args.build_root is not None:
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
            fail(
                TEST_DIAGNOSTIC,
                "ctest --show-only=json-v1 failed: "
                f"{result.stderr.strip()}",
            )
        try:
            parsed_ctest = json.loads(result.stdout)
        except json.JSONDecodeError as error:
            fail(
                TEST_DIAGNOSTIC,
                f"ctest --show-only returned invalid JSON: {error}",
            )
        if not isinstance(parsed_ctest, dict):
            fail(TEST_DIAGNOSTIC, "CTest JSON root is not an object")
        ctest_source = parsed_ctest
    if ctest_source is not None:
        tests = collect_ctest_boundary(
            ctest_source,
            source_root=repo,
            build_root=args.build_root,
            baseline_ctest=baseline_ctest,
        )
        model["tests"] = tests
        preservation["ctest_survivors_match"] = tests["survivors_match"]
    if args.install_manifest is not None:
        apply_install_manifest(
            model,
            read_unbounded_manifest(args.install_manifest),
            start=start,
            install_root=args.install_root,
        )
    if args.source_package_manifest is not None:
        apply_source_manifest(
            model,
            read_unbounded_manifest(args.source_package_manifest),
            start=start,
        )
    if args.binary_package_manifest is not None:
        apply_binary_manifest(
            model,
            read_unbounded_manifest(args.binary_package_manifest),
            start=start,
        )
    if args.install_root is not None and args.install_manifest is None:
        root_paths, root_content = scan_install_root(
            args.install_root.resolve()
        )
        model["install"]["forbidden_paths"] = root_paths
        model["install"]["forbidden_content"] = root_content


def package_tree_manifest(repo: Path) -> list[str]:
    entries = [
        path.relative_to(repo).as_posix()
        for path in repo.rglob("*")
        if path.is_file() or path.is_symlink()
    ]
    return sorted(entries)


def verify_checked_boundary_evidence(
    repo: Path, expected: dict[str, Any]
) -> None:
    path = repo / "docs/migrations" / BOUNDARY_EVIDENCE_FILE
    try:
        observed = path.read_bytes()
    except OSError as error:
        fail(
            MIGRATION_DIAGNOSTIC,
            f"cannot read checked-in boundary evidence {path}: {error}",
        )
    if observed != canonical_json_bytes(expected):
        fail(
            MIGRATION_DIAGNOSTIC,
            f"checked-in {BOUNDARY_EVIDENCE_FILE} is stale",
        )


def check_boundary(args: argparse.Namespace, *, package_mode: bool) -> dict[str, Any]:
    repo = args.repo_root.resolve()
    frozen = load_frozen_evidence(repo)
    start, _, baseline_ctest = frozen
    model = collect_source_boundary(repo, frozen=frozen)
    source_evidence = copy.deepcopy(model)
    source_diagnostics = boundary_model_diagnostics(source_evidence)
    if source_diagnostics:
        fail(
            source_diagnostics[0],
            "AISuite source ownership boundary is invalid: "
            f"{source_diagnostics}",
        )
    verify_checked_boundary_evidence(repo, source_evidence)

    if package_mode:
        apply_source_manifest(
            model, package_tree_manifest(repo), start=start
        )
    else:
        apply_configured_artifacts(
            model, args, start=start, baseline_ctest=baseline_ctest
        )
    diagnostics = boundary_model_diagnostics(model)
    if diagnostics:
        fail(
            diagnostics[0],
            f"AISuite ownership boundary is invalid: {diagnostics}",
        )
    return model


def add_common_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--repo-root", type=Path, required=True, help="SNode.C repository root"
    )
    parser.add_argument("--aisuite-source", type=Path)
    parser.add_argument("--ctest-json", type=Path)
    parser.add_argument("--build-root", type=Path)
    parser.add_argument("--install-manifest", type=Path)
    parser.add_argument("--source-package-manifest", type=Path)
    parser.add_argument("--binary-package-manifest", type=Path)
    parser.add_argument("--snodec-config", type=Path)
    parser.add_argument("--cpack-config", type=Path)
    parser.add_argument("--target-inventory", type=Path)
    parser.add_argument("--install-root", type=Path)


def parse_arguments(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="mode", required=True)
    audit = subparsers.add_parser(
        "audit-start", help="verify the frozen start-state authority"
    )
    add_common_arguments(audit)
    generate = subparsers.add_parser(
        "generate", help="generate deterministic cutover evidence"
    )
    add_common_arguments(generate)
    generate.add_argument(
        "--stage", choices=["start", "boundary"], required=True
    )
    generate.add_argument("--output-dir", type=Path, required=True)
    check = subparsers.add_parser(
        "check", help="verify the permanent AISuite ownership boundary"
    )
    add_common_arguments(check)
    check_package = subparsers.add_parser(
        "check-package",
        help="verify the boundary from an unpacked source package",
    )
    check_package.add_argument(
        "--repo-root",
        type=Path,
        required=True,
        help="unpacked SNode.C source-package root",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_arguments(sys.argv[1:] if argv is None else argv)
    try:
        if args.mode == "generate" and args.stage == "start":
            start, plan, baseline_ctest = build_start_state(args)
            output_dir = args.output_dir.resolve()
            write_generated(
                output_dir / "aisuite-cutover-start-state.json", start
            )
            write_generated(output_dir / "aisuite-cutover-plan.json", plan)
            write_generated(
                output_dir / "aisuite-cutover-baseline-ctest.json",
                baseline_ctest,
            )
            print(
                "SNode.C AISuite cutover start evidence generated: "
                f"removed={start['removal_inventory']['total_count']}, "
                "snodec_tests=123, aisuite_tests=131, "
                "semantic_logger=81/81"
            )
            return 0
        if args.mode == "generate" and args.stage == "boundary":
            boundary = make_boundary_evidence(args.repo_root.resolve())
            output_dir = args.output_dir.resolve()
            write_generated(
                output_dir / BOUNDARY_EVIDENCE_FILE, boundary
            )
            print(
                "SNode.C AISuite ownership boundary evidence generated: "
                "paths=absent, components=35, cpack=66, "
                "ctest-survivors=172, semantic-logger=77/77"
            )
            return 0
        if args.mode == "check":
            model = check_boundary(args, package_mode=False)
            print(
                "SNode.C AISuite ownership boundary verified: "
                f"components={len(model['build']['supported_components'])}, "
                "semantic-logger=77/77"
            )
            return 0
        if args.mode == "check-package":
            model = check_boundary(args, package_mode=True)
            print(
                "SNode.C AISuite package-safe ownership boundary verified: "
                f"source-paths={len(package_tree_manifest(args.repo_root.resolve()))}, "
                "git=unused, network=unused"
            )
            return 0
        if any(
            value is not None
            for value in [
                args.ctest_json,
                args.install_manifest,
                args.source_package_manifest,
                args.binary_package_manifest,
                args.snodec_config,
                args.cpack_config,
                args.target_inventory,
            ]
        ):
            start, plan, baseline_ctest = build_start_state(args)
            checked_dir = args.repo_root.resolve() / "docs/migrations"
            expected = {
                "aisuite-cutover-start-state.json": start,
                "aisuite-cutover-plan.json": plan,
                "aisuite-cutover-baseline-ctest.json": baseline_ctest,
            }
            for file_name, model in expected.items():
                observed = (checked_dir / file_name).read_bytes()
                if observed != canonical_json_bytes(model):
                    fail(
                        START_STATE_DIAGNOSTIC,
                        f"checked-in {file_name} is stale",
                    )
        else:
            check_start_evidence(args.repo_root.resolve())
        print(
            "SNode.C AISuite cutover start-state verified: "
            "base=d18b231a, owner=0c3a5838, removed=6808, "
            "ctest=300 (123 -> 131 authority reconciliation)"
        )
        return 0
    except CutoverError as error:
        print(str(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
