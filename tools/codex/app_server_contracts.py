#!/usr/bin/env python3
"""Derive pinned Codex App Server operation contracts entirely offline.

Client request result associations come from the exact vendored upstream Rust
macro declaration. Server-request response associations come from the paired
vendored JSON Schema roots and are cross-checked against the same Rust source.
No filename similarity is used for client results.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Sequence


FORMAT_VERSION = 1
VERSION = "0.144.6"
CODEX_VERSION = f"codex-cli {VERSION}"
SOURCE_COMMIT = "5d1fbf26c43abc65a203928b2e31561cb039e06d"
TAG = "rust-v0.144.6"
TAG_OBJECT = "1e66aaa95b5ab39d3ef3057cd50bdecd576a8356"
COMMON_PATH = "codex-rs/app-server-protocol/src/protocol/common.rs"
COMMON_BYTES = 140314
COMMON_BLOB = "27a8b0205da7998b91edca9b94733b1d3a0b92e3"
COMMON_SHA256 = "5679cbc5b3e935a2edd69fbe62b82b956eff4d5d050c9969b090ccc9d367fdee"
LICENSE_BYTES = 10926
LICENSE_BLOB = "4606e72e042564097e8780d66c1d4dcb611869bd"
LICENSE_SHA256 = "d17f227e4df5da1600391338865ce0f3055211760a36688f816941d58232d8dc"
NOTICE_BYTES = 242
NOTICE_BLOB = "2805899d56d0332d175cfc613c67d45d6f006db7"
NOTICE_SHA256 = "9d71575ecfd9a843fc1677b0efb08053c6ba9fd686a0de1a6f5382fd3c220915"
STABLE_SCHEMA_AGGREGATE_SHA256 = (
    "cee1ac3bcaf95e5fcdcf07499c7e6b00fc423b90c670ea3380f1799434b72add"
)
TYPESCRIPT_AUDIT = {
    "stable": {
        "aggregate_sha256": "73c95b6a19d5559939519a771e0f7285cc60bbfaa1aac8bd9c52ba308c6e6811",
        "file_count": 598,
        "path_set_sha256": "07387afea381dca83e7ee178f0e3f06af39e4227a7e7a64fb9959c5e7f429489",
        "response_named_file_count": 101,
        "client_request": {
            "path": "ClientRequest.ts",
            "bytes": 13794,
            "sha256": "b91a43a7f5006cdd0fec35a482f585affc2e1055ffd854936f7b32bd986f6bf2",
            "method_arms": 90,
        },
        "client_method_literal_files": [
            {
                "path": "ClientRequest.ts",
                "method_literal_count": 90,
                "sha256": "b91a43a7f5006cdd0fec35a482f585affc2e1055ffd854936f7b32bd986f6bf2",
            }
        ],
        "client_response_named_paths": [],
        "client_response_declarations": [],
        "client_parameter_type_count": 82,
        "response_type_name_count": 99,
        "parameter_response_coreference_files": [
            {
                "classification": "flat_export_index",
                "client_parameter_reference_count": 5,
                "path": "index.ts",
                "response_type_reference_count": 7,
                "sha256": "8ce45a1eba375e35bccb5cd66a07b39dafad49c8adaa591f90620457e42eb319",
            },
            {
                "classification": "flat_export_index",
                "client_parameter_reference_count": 77,
                "path": "v2/index.ts",
                "response_type_reference_count": 92,
                "sha256": "9380aa176e8f845ebbe4842edb8f2a7ed6fedd5c1c36f2d4e4922072574dadf5",
            },
        ],
        "broader_name_review_files": [
            {
                "path": "v2/CommandExecutionRequestApprovalResponse.ts",
                "sha256": "454ceee4decc66a9c070273d4344fa8a0187732cd884b8b60edda3a17766aa95",
            },
            {
                "path": "v2/FileChangeRequestApprovalResponse.ts",
                "sha256": "527a9af90f753e2b7f1852415523443fc9745376bb2c4b0b735f900c8da8fc71",
            },
            {
                "path": "v2/McpServerElicitationRequestResponse.ts",
                "sha256": "391007144a7dac5cdc57a065de5aaab130d4f7a45b5c9d7fae801c674498a3ee",
            },
            {
                "path": "v2/PermissionsRequestApprovalResponse.ts",
                "sha256": "60259c885b04fe35e3596d74921b3ca8c8cba3cb5bd582305b9d90e14fff9930",
            },
            {
                "path": "v2/ToolRequestUserInputResponse.ts",
                "sha256": "39e4959095a1b31de3f7bf28422a6536d51d651581ad1f50bc5e30c02140498b",
            },
        ],
        "association_construct_candidates": [],
        "request_response_mapping_candidates": [],
    },
    "experimental": {
        "aggregate_sha256": "d561ef0b4ef8a921fff50de4e3c662a0fdff643e82868f2b05a3e71f912aec8c",
        "file_count": 671,
        "path_set_sha256": "e3df803b3bd79b66df8117cd71c03e5489c726e933ffdf4263e07168f8be692e",
        "response_named_file_count": 137,
        "client_request": {
            "path": "ClientRequest.ts",
            "bytes": 19941,
            "sha256": "ba1f52da673f4a64b730dcdf5c89a87ea45a82b4dc7c8686bde530a677ddb6a7",
            "method_arms": 125,
        },
        "client_method_literal_files": [
            {
                "path": "ClientRequest.ts",
                "method_literal_count": 125,
                "sha256": "ba1f52da673f4a64b730dcdf5c89a87ea45a82b4dc7c8686bde530a677ddb6a7",
            }
        ],
        "client_response_named_paths": [],
        "client_response_declarations": [],
        "client_parameter_type_count": 115,
        "response_type_name_count": 135,
        "parameter_response_coreference_files": [
            {
                "classification": "flat_export_index",
                "client_parameter_reference_count": 8,
                "path": "index.ts",
                "response_type_reference_count": 10,
                "sha256": "80712d84db9dce8d91bf8a5368e66a790b3c8df4875eb7c18258f405201c372d",
            },
            {
                "classification": "flat_export_index",
                "client_parameter_reference_count": 107,
                "path": "v2/index.ts",
                "response_type_reference_count": 125,
                "sha256": "a28f8dccd991f1c5bdc068afa644e478b46b491f67a81b1c16251e82a6742362",
            },
        ],
        "broader_name_review_files": [
            {
                "path": "v2/CommandExecutionRequestApprovalResponse.ts",
                "sha256": "454ceee4decc66a9c070273d4344fa8a0187732cd884b8b60edda3a17766aa95",
            },
            {
                "path": "v2/FileChangeRequestApprovalResponse.ts",
                "sha256": "527a9af90f753e2b7f1852415523443fc9745376bb2c4b0b735f900c8da8fc71",
            },
            {
                "path": "v2/McpServerElicitationRequestResponse.ts",
                "sha256": "391007144a7dac5cdc57a065de5aaab130d4f7a45b5c9d7fae801c674498a3ee",
            },
            {
                "path": "v2/PermissionsRequestApprovalResponse.ts",
                "sha256": "60259c885b04fe35e3596d74921b3ca8c8cba3cb5bd582305b9d90e14fff9930",
            },
            {
                "path": "v2/ToolRequestUserInputResponse.ts",
                "sha256": "39e4959095a1b31de3f7bf28422a6536d51d651581ad1f50bc5e30c02140498b",
            },
        ],
        "association_construct_candidates": [],
        "request_response_mapping_candidates": [],
    },
}
EXPECTED_UNIT_RESULT_METHODS = (
    "account/logout",
    "command/exec/resize",
    "command/exec/terminate",
    "command/exec/write",
    "config/mcpServer/reload",
    "fs/copy",
    "fs/createDirectory",
    "fs/remove",
    "fs/unwatch",
    "fs/writeFile",
    "plugin/share/delete",
    "plugin/uninstall",
    "skills/extraRoots/set",
    "thread/approveGuardianDeniedAction",
    "thread/archive",
    "thread/compact/start",
    "thread/delete",
    "thread/inject_items",
    "thread/name/set",
    "thread/shellCommand",
    "turn/interrupt",
)

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_SOURCE_ROOT = (
    REPOSITORY_ROOT / "tools/codex/app-server-protocol-source" / VERSION
)
DEFAULT_SCHEMA_ROOT = REPOSITORY_ROOT / "tools/codex/app-server-schema" / VERSION
DEFAULT_MANIFEST = (
    REPOSITORY_ROOT / "tools/codex/app-server-surface" / f"{VERSION}.json"
)
DEFAULT_EVIDENCE_ROOT = (
    REPOSITORY_ROOT / "tools/codex/app-server-evidence" / VERSION
)
DEFAULT_SCHEMA_PROVENANCE = DEFAULT_SCHEMA_ROOT / "PROVENANCE.json"


class ContractError(RuntimeError):
    pass


@dataclass(frozen=True)
class RustAssociation:
    variant: str
    method: str
    parameter_type: str
    result_type: str
    experimental: bool


def canonical_json(value: Any) -> str:
    return json.dumps(
        value,
        ensure_ascii=False,
        allow_nan=False,
        indent=2,
        sort_keys=True,
    ) + "\n"


def load_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise ContractError(f"unable to load JSON {path}: {error}") from error


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def typescript_tree_records(root: Path) -> list[dict[str, Any]]:
    """Return the exact sorted records used by the pinned A0 tree hash."""

    if not root.is_dir():
        raise ContractError(f"generated TypeScript tree is missing: {root}")
    records: list[dict[str, Any]] = []
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
        raise ContractError(f"generated TypeScript tree contains no files: {root}")
    return records


def typescript_tree_aggregate(records: Sequence[dict[str, Any]]) -> str:
    payload = "".join(
        f"{record['sha256']}  {record['path']}\n" for record in records
    )
    return hashlib.sha256(payload.encode("utf-8")).hexdigest()


def scan_typescript_tree(root: Path) -> dict[str, Any]:
    """Mechanically audit a pinned generate-ts tree for a client response map.

    A mapping would need to retain a client method identity beside a successful
    response/result type. The scan therefore anchors the complete path set,
    locates every exact client-method literal, inspects method-union
    declarations for response/result fields, and separately rejects a
    ClientResponse path or declaration.
    """

    records = typescript_tree_records(root)
    by_path = {record["path"]: record for record in records}
    client_request_record = by_path.get("ClientRequest.ts")
    if client_request_record is None:
        raise ContractError("generated TypeScript tree lacks ClientRequest.ts")
    try:
        texts = {
            record["path"]: (root / record["path"]).read_text(encoding="utf-8")
            for record in records
            if record["path"].endswith(".ts")
        }
    except (OSError, UnicodeError) as error:
        raise ContractError(f"unable to scan generated TypeScript: {error}") from error

    client_request = texts["ClientRequest.ts"]
    client_methods = re.findall(
        r'"method"\s*:\s*"([^"]+)"',
        client_request,
    )
    if not client_methods or len(client_methods) != len(set(client_methods)):
        raise ContractError(
            "ClientRequest.ts method union is empty or contains duplicate methods"
        )
    client_parameter_types = set(
        re.findall(
            r"\bparams\s*:\s*([A-Za-z_][A-Za-z0-9_]*)",
            client_request,
        )
    )
    client_parameter_types.discard("undefined")
    response_type_names = {
        Path(path).stem
        for path in texts
        if Path(path).name.endswith("Response.ts")
    }

    literal_files: list[dict[str, Any]] = []
    mapping_candidates: list[dict[str, Any]] = []
    coreference_files: list[dict[str, Any]] = []
    association_construct_candidates: list[dict[str, Any]] = []
    for path, text in sorted(texts.items()):
        identifiers = set(
            re.findall(r"\b[A-Za-z_][A-Za-z0-9_]*\b", text)
        )
        parameter_references = client_parameter_types & identifiers
        request_identity_references = set(parameter_references)
        if "ClientRequest" in identifiers:
            request_identity_references.add("ClientRequest")
        response_references = response_type_names & identifiers
        if request_identity_references and response_references:
            without_comments = re.sub(
                r"/\*.*?\*/|//[^\n]*",
                "",
                text,
                flags=re.DOTALL,
            )
            statements = [
                line.strip()
                for line in without_comments.splitlines()
                if line.strip()
            ]
            flat_export_index = bool(statements) and all(
                statement.startswith("export type {")
                or statement.startswith("export * as ")
                for statement in statements
            )
            classification = (
                "flat_export_index"
                if flat_export_index
                else "review_required"
            )
            coreference_files.append(
                {
                    "classification": classification,
                    "client_parameter_reference_count": len(
                        parameter_references
                    ),
                    "path": path,
                    "response_type_reference_count": len(
                        response_references
                    ),
                    "sha256": by_path[path]["sha256"],
                }
            )
            if not flat_export_index:
                mapping_candidates.append(
                    {
                        "path": path,
                        "reason": (
                            "co-references ClientRequest identities and "
                            "response-named types outside a flat export index"
                        ),
                    }
                )
            if re.search(
                r"\b(?:infer|Record)\b|\[[^\]]+\bin\b[^\]]+\]|"
                r"\bextends\b[^;\n]*\?",
                text,
            ):
                association_construct_candidates.append(
                    {
                        "path": path,
                        "reason": (
                            "conditional/mapped construct co-references "
                            "client request and response identities"
                        ),
                    }
                )

        method_count = sum(text.count(json.dumps(method)) for method in client_methods)
        if method_count:
            literal_files.append(
                {
                    "path": path,
                    "method_literal_count": method_count,
                    "sha256": by_path[path]["sha256"],
                }
            )
            if path != "ClientRequest.ts":
                mapping_candidates.append(
                    {
                        "path": path,
                        "reason": "contains an exact ClientRequest method literal",
                    }
                )

        for match in re.finditer(
            r"export\s+type\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*?);",
            text,
            flags=re.DOTALL,
        ):
            alias, declaration = match.groups()
            if not re.search(r'"method"\s*:', declaration):
                continue
            if re.search(
                r"\b(?:response|result)\b\s*:",
                declaration,
                flags=re.IGNORECASE,
            ):
                mapping_candidates.append(
                    {
                        "path": path,
                        "reason": (
                            f"method union {alias} has a response/result field"
                        ),
                    }
                )

    client_response_named_paths = sorted(
        path
        for path in by_path
        if Path(path).name.casefold() == "clientresponse.ts"
    )
    client_response_declarations = sorted(
        path
        for path, text in texts.items()
        if re.search(
            r"\b(?:type|interface)\s+ClientResponse\b",
            text,
        )
    )
    for path in client_response_named_paths:
        mapping_candidates.append(
            {"path": path, "reason": "ClientResponse.ts path is present"}
        )
    for path in client_response_declarations:
        mapping_candidates.append(
            {"path": path, "reason": "ClientResponse declaration is present"}
        )
    broad_name_pattern = re.compile(
        r"(?:Request.*(?:Response|Result)|(?:Response|Result).*Request|"
        r"(?:Response|Result).*Map|Map.*(?:Response|Result))",
        flags=re.IGNORECASE,
    )
    broader_name_review_files = [
        {"path": path, "sha256": by_path[path]["sha256"]}
        for path in sorted(texts)
        if broad_name_pattern.search(Path(path).stem)
    ]

    paths = [record["path"] for record in records]
    return {
        "aggregate_sha256": typescript_tree_aggregate(records),
        "file_count": len(records),
        "path_set_sha256": hashlib.sha256(
            "".join(f"{path}\n" for path in paths).encode("utf-8")
        ).hexdigest(),
        "response_named_file_count": sum(
            "Response" in Path(path).name for path in paths
        ),
        "client_request": {
            **client_request_record,
            "method_arms": len(client_methods),
        },
        "client_method_literal_files": literal_files,
        "client_response_named_paths": client_response_named_paths,
        "client_response_declarations": client_response_declarations,
        "client_parameter_type_count": len(client_parameter_types),
        "response_type_name_count": len(response_type_names),
        "parameter_response_coreference_files": coreference_files,
        "broader_name_review_files": broader_name_review_files,
        "association_construct_candidates": sorted(
            association_construct_candidates,
            key=lambda candidate: (candidate["path"], candidate["reason"]),
        ),
        "request_response_mapping_candidates": sorted(
            mapping_candidates,
            key=lambda candidate: (candidate["path"], candidate["reason"]),
        ),
    }


def git_blob_sha(path: Path) -> str:
    data = path.read_bytes()
    header = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(header + data).hexdigest()


def verify_file(
    path: Path,
    expected_bytes: int,
    expected_sha256: str,
    expected_blob: str,
    description: str,
) -> None:
    try:
        size = path.stat().st_size
    except OSError as error:
        raise ContractError(f"missing {description}: {path}: {error}") from error
    digest = sha256(path)
    blob = git_blob_sha(path)
    if (
        size != expected_bytes
        or digest != expected_sha256
        or blob != expected_blob
    ):
        raise ContractError(
            f"{description} integrity mismatch: expected {expected_bytes} bytes/"
            f"{expected_sha256}/{expected_blob}, got {size}/{digest}/{blob}"
        )


def source_provenance(source_root: Path) -> dict[str, Any]:
    common = source_root / COMMON_PATH
    license_path = source_root / "LICENSE.openai-codex"
    notice_path = source_root / "NOTICE.openai-codex"
    verify_file(
        common,
        COMMON_BYTES,
        COMMON_SHA256,
        COMMON_BLOB,
        "vendored Rust association source",
    )
    verify_file(
        license_path,
        LICENSE_BYTES,
        LICENSE_SHA256,
        LICENSE_BLOB,
        "upstream license",
    )
    verify_file(
        notice_path,
        NOTICE_BYTES,
        NOTICE_SHA256,
        NOTICE_BLOB,
        "upstream notice",
    )
    expected_files = {
        COMMON_PATH,
        "LICENSE.openai-codex",
        "NOTICE.openai-codex",
        "PROVENANCE.json",
    }
    expected_directories = {
        parent.as_posix()
        for relative in expected_files
        for parent in Path(relative).parents
        if parent.as_posix() != "."
    }
    actual_files: set[str] = set()
    actual_directories: set[str] = set()
    for path in sorted(source_root.rglob("*")):
        relative = path.relative_to(source_root).as_posix()
        if path.is_symlink():
            raise ContractError(
                f"vendored Rust evidence contains a symlink: {relative}"
            )
        if path.is_file():
            actual_files.add(relative)
        elif path.is_dir():
            actual_directories.add(relative)
        else:
            raise ContractError(
                f"vendored Rust evidence contains an unsupported entry: {relative}"
            )
    if actual_files != expected_files or actual_directories != expected_directories:
        raise ContractError(
            "vendored Rust evidence file set mismatch: "
            f"missing_files={sorted(expected_files - actual_files)}, "
            f"extra_files={sorted(actual_files - expected_files)}, "
            f"missing_directories={sorted(expected_directories - actual_directories)}, "
            f"extra_directories={sorted(actual_directories - expected_directories)}"
        )
    return {
        "format_version": FORMAT_VERSION,
        "generated_notice": (
            "GENERATED by tools/codex/app_server_contracts.py; do not edit."
        ),
        "project": "OpenAI Codex",
        "repository": "https://github.com/openai/codex",
        "release": {
            "tag": TAG,
            "annotated_tag_object_sha": TAG_OBJECT,
            "source_commit_sha": SOURCE_COMMIT,
        },
        "files": [
            {
                "path": COMMON_PATH,
                "original_path": COMMON_PATH,
                "source_url": (
                    "https://github.com/openai/codex/blob/"
                    f"{SOURCE_COMMIT}/{COMMON_PATH}"
                ),
                "bytes": COMMON_BYTES,
                "git_blob_sha": COMMON_BLOB,
                "sha256": COMMON_SHA256,
                "purpose": (
                    "authoritative client and server request params/response "
                    "macro declarations"
                ),
            },
            {
                "path": "LICENSE.openai-codex",
                "original_path": "LICENSE",
                "source_url": (
                    f"https://github.com/openai/codex/blob/{SOURCE_COMMIT}/LICENSE"
                ),
                "bytes": LICENSE_BYTES,
                "git_blob_sha": LICENSE_BLOB,
                "sha256": LICENSE_SHA256,
                "spdx_identifier": "Apache-2.0",
            },
            {
                "path": "NOTICE.openai-codex",
                "original_path": "NOTICE",
                "source_url": (
                    f"https://github.com/openai/codex/blob/{SOURCE_COMMIT}/NOTICE"
                ),
                "bytes": NOTICE_BYTES,
                "git_blob_sha": NOTICE_BLOB,
                "sha256": NOTICE_SHA256,
            },
        ],
        "minimality": (
            "common.rs contains the macro definitions and complete invocations. "
            "Referenced v1/v2 Rust type definitions are not needed to recover "
            "associations; their exact wire schemas are already vendored."
        ),
        "integrity": "exact upstream bytes; no local edits",
    }


def matching_brace(text: str, opening: int) -> int:
    if opening >= len(text) or text[opening] != "{":
        raise ContractError("internal parser error: expected opening brace")
    depth = 0
    state = "normal"
    index = opening
    block_depth = 0
    while index < len(text):
        current = text[index]
        following = text[index + 1] if index + 1 < len(text) else ""
        if state == "line_comment":
            if current == "\n":
                state = "normal"
        elif state == "block_comment":
            if current == "/" and following == "*":
                block_depth += 1
                index += 1
            elif current == "*" and following == "/":
                if block_depth == 0:
                    state = "normal"
                else:
                    block_depth -= 1
                index += 1
        elif state == "string":
            if current == "\\":
                index += 1
            elif current == '"':
                state = "normal"
        elif state == "char":
            if current == "\\":
                index += 1
            elif current == "'":
                state = "normal"
        else:
            if current == "/" and following == "/":
                state = "line_comment"
                index += 1
            elif current == "/" and following == "*":
                state = "block_comment"
                block_depth = 0
                index += 1
            elif current == '"':
                state = "string"
            elif current == "'":
                state = "char"
            elif current == "{":
                depth += 1
            elif current == "}":
                depth -= 1
                if depth == 0:
                    return index
        index += 1
    raise ContractError("unterminated Rust macro invocation")


def split_top_level(payload: str) -> list[str]:
    result: list[str] = []
    start = 0
    depths = {"{": 0, "(": 0, "[": 0}
    closing = {"}": "{", ")": "(", "]": "["}
    state = "normal"
    block_depth = 0
    index = 0
    while index < len(payload):
        current = payload[index]
        following = payload[index + 1] if index + 1 < len(payload) else ""
        if state == "line_comment":
            if current == "\n":
                state = "normal"
        elif state == "block_comment":
            if current == "/" and following == "*":
                block_depth += 1
                index += 1
            elif current == "*" and following == "/":
                if block_depth == 0:
                    state = "normal"
                else:
                    block_depth -= 1
                index += 1
        elif state == "string":
            if current == "\\":
                index += 1
            elif current == '"':
                state = "normal"
        elif state == "char":
            if current == "\\":
                index += 1
            elif current == "'":
                state = "normal"
        else:
            if current == "/" and following == "/":
                state = "line_comment"
                index += 1
            elif current == "/" and following == "*":
                state = "block_comment"
                block_depth = 0
                index += 1
            elif current == '"':
                state = "string"
            elif current == "'":
                state = "char"
            elif current in depths:
                depths[current] += 1
            elif current in closing:
                depths[closing[current]] -= 1
                if depths[closing[current]] < 0:
                    raise ContractError("unbalanced Rust macro entry")
            elif current == "," and not any(depths.values()):
                result.append(payload[start:index])
                start = index + 1
        index += 1
    if any(depths.values()) or state in {"block_comment", "string", "char"}:
        raise ContractError("unterminated token while splitting Rust macro")
    result.append(payload[start:])
    return result


def camel_case_variant(variant: str) -> str:
    if not variant:
        raise ContractError("empty Rust request variant")
    return variant[0].lower() + variant[1:]


def normalize_rust_type(value: str, parameter: bool = False) -> str:
    value = re.sub(r"#\[[^\]]+\]\s*", "", value).strip()
    if parameter and value in {"()", "Option<()>"}:
        return "Unit"
    if value == "()":
        return "()"
    if "<" in value or ">" in value or any(character.isspace() for character in value):
        raise ContractError(f"unsupported authoritative Rust type syntax: {value!r}")
    return value.rsplit("::", 1)[-1]


def parse_request_macro(text: str, macro_name: str) -> list[RustAssociation]:
    marker = macro_name + "! {"
    invocation = text.find(marker)
    if invocation < 0:
        raise ContractError(f"missing authoritative Rust invocation {marker}")
    opening = text.find("{", invocation + len(macro_name))
    closing = matching_brace(text, opening)
    payload = text[opening + 1 : closing]
    associations: list[RustAssociation] = []
    for raw_entry in split_top_level(payload):
        if not raw_entry.strip():
            continue
        entry_opening = raw_entry.find("{")
        if entry_opening < 0:
            raise ContractError(
                f"malformed {macro_name} entry without body: {raw_entry[-80:]!r}"
            )
        header = raw_entry[:entry_opening]
        header_match = re.search(
            r"([A-Za-z_][A-Za-z0-9_]*)\s*(?:=>\s*\"([^\"]+)\")?\s*$",
            header,
            re.DOTALL,
        )
        if header_match is None:
            raise ContractError(
                f"unable to parse {macro_name} entry header: {header[-160:]!r}"
            )
        variant = header_match.group(1)
        method = header_match.group(2) or camel_case_variant(variant)
        body_closing = matching_brace(raw_entry, entry_opening)
        if raw_entry[body_closing + 1 :].strip():
            raise ContractError(f"unexpected tokens after Rust request {variant}")
        body = raw_entry[entry_opening + 1 : body_closing]
        parameter_match = re.search(
            r"(?:^|\n)\s*params:\s*((?:#\[[^\]]+\]\s*)*[^,\n]+),",
            body,
        )
        response_match = re.search(
            r"(?:^|\n)\s*response:\s*([^,\n]+),",
            body,
        )
        if parameter_match is None or response_match is None:
            raise ContractError(
                f"authoritative Rust request {variant} lacks params/response"
            )
        associations.append(
            RustAssociation(
                variant=variant,
                method=method,
                parameter_type=parameter_match.group(1).strip(),
                result_type=response_match.group(1).strip(),
                experimental="#[experimental(" in header,
            )
        )
    methods = [association.method for association in associations]
    if len(methods) != len(set(methods)):
        raise ContractError(f"duplicate methods in authoritative {macro_name}")
    return associations


def exact_surface_key(entry: dict[str, Any]) -> dict[str, str]:
    return {
        "category": entry["category"],
        "domain": entry["domain"],
        "field": entry["discriminator_field"],
        "name": entry["name"],
    }


def schema_for_type(schema_root: Path, type_name: str) -> str:
    candidates = [
        relative
        for relative in (
            Path("stable") / f"{type_name}.json",
            Path("stable/v1") / f"{type_name}.json",
            Path("stable/v2") / f"{type_name}.json",
        )
        if (schema_root / relative).is_file()
    ]
    if len(candidates) != 1:
        raise ContractError(
            f"schema type {type_name} resolves to {len(candidates)} roots: "
            + ", ".join(str(path) for path in candidates)
        )
    document = load_json(schema_root / candidates[0])
    if not isinstance(document, dict) or document.get("title") != type_name:
        raise ContractError(
            f"standalone schema title mismatch for {type_name}: {candidates[0]}"
        )
    return candidates[0].as_posix()


def result_contract_kind(schema_root: Path, relative: str) -> str:
    document = load_json(schema_root / relative)
    if not isinstance(document, dict):
        raise ContractError(f"result schema is not an object: {relative}")

    schema_type = document.get("type")
    if schema_type == "null" or (
        isinstance(schema_type, list) and "null" in schema_type
    ):
        return "Nullable"
    for keyword in ("oneOf", "anyOf"):
        branches = document.get(keyword)
        if isinstance(branches, list) and any(
            isinstance(branch, dict) and branch.get("type") == "null"
            for branch in branches
        ):
            return "Nullable"

    properties = document.get("properties")
    required = document.get("required")
    if (
        schema_type == "object"
        and (properties is None or properties == {})
        and (required is None or required == [])
        and not any(
            keyword in document for keyword in ("$ref", "allOf", "anyOf", "oneOf")
        )
    ):
        return "Unit"
    return "Concrete"


def schema_definition_name(reference: Any, context: str) -> str:
    if not isinstance(reference, str) or not reference.startswith("#/definitions/"):
        raise ContractError(f"{context} has a non-definition params reference")
    tokens = reference.split("/")
    if len(tokens) != 3 or not tokens[-1]:
        raise ContractError(f"{context} has an ambiguous params reference")
    return tokens[-1].replace("~1", "/").replace("~0", "~")


def schema_server_pairs(
    schema_root: Path, manifest_entries: Sequence[dict[str, Any]]
) -> list[dict[str, Any]]:
    server_schema_path = schema_root / "stable/ServerRequest.json"
    server_schema = load_json(server_schema_path)
    branches = server_schema.get("oneOf") if isinstance(server_schema, dict) else None
    if not isinstance(branches, list):
        raise ContractError("stable ServerRequest schema has no oneOf branches")
    by_method: dict[str, tuple[int, str]] = {}
    for index, branch in enumerate(branches):
        try:
            properties = branch["properties"]
            methods = properties["method"]["enum"]
            params_reference = properties["params"]["$ref"]
        except (KeyError, TypeError) as error:
            raise ContractError(
                f"malformed stable ServerRequest branch {index}"
            ) from error
        if not isinstance(methods, list) or len(methods) != 1 or not isinstance(
            methods[0], str
        ):
            raise ContractError(
                f"stable ServerRequest branch {index} lacks one method identity"
            )
        method = methods[0]
        if method in by_method:
            raise ContractError(f"duplicate stable ServerRequest method {method}")
        by_method[method] = (
            index,
            schema_definition_name(
                params_reference, f"stable ServerRequest branch {index}"
            ),
        )

    stable_rows = [
        entry
        for entry in manifest_entries
        if entry["category"] == "server_request"
        and entry["stability"] == "stable"
    ]
    expected_methods = {entry["name"] for entry in stable_rows}
    if set(by_method) != expected_methods:
        raise ContractError(
            "stable ServerRequest schema/manifest method disagreement: "
            f"missing={sorted(expected_methods - set(by_method))}, "
            f"stale={sorted(set(by_method) - expected_methods)}"
        )

    pairs: list[dict[str, Any]] = []
    for entry in stable_rows:
        method = entry["name"]
        index, parameter_type = by_method[method]
        manifest_parameter = entry["params"]["type"]
        if manifest_parameter != parameter_type:
            raise ContractError(
                f"{method} schema branch parameter {parameter_type} disagrees "
                f"with manifest {manifest_parameter}"
            )
        if not parameter_type.endswith("Params"):
            raise ContractError(
                f"{method} params root cannot be paired exactly by schema suffix"
            )
        response_type = parameter_type[: -len("Params")] + "Response"
        parameter_schema = schema_for_type(schema_root, parameter_type)
        result_schema = schema_for_type(schema_root, response_type)
        pairs.append(
            {
                "entry": entry,
                "branch": index,
                "parameter_type": parameter_type,
                "response_type": response_type,
                "parameter_schema": parameter_schema,
                "result_schema": result_schema,
            }
        )
    return pairs


def build_contracts(
    source_root: Path, schema_root: Path, manifest_path: Path
) -> dict[str, Any]:
    source_provenance(source_root)
    common_path = source_root / COMMON_PATH
    common = common_path.read_text(encoding="utf-8")
    client_rust = parse_request_macro(common, "client_request_definitions")
    server_rust = parse_request_macro(common, "server_request_definitions")
    if len(client_rust) != 125:
        raise ContractError(
            f"expected 125 authoritative client declarations, got {len(client_rust)}"
        )
    if len(server_rust) != 11:
        raise ContractError(
            f"expected 11 authoritative server declarations, got {len(server_rust)}"
        )

    manifest = load_json(manifest_path)
    entries = manifest.get("entries") if isinstance(manifest, dict) else None
    if not isinstance(manifest, dict) or manifest.get("codex_version") != CODEX_VERSION:
        raise ContractError("surface manifest does not match the pinned Codex version")
    if (
        manifest.get("schema_trees", {}).get("stable_aggregate_sha256")
        != STABLE_SCHEMA_AGGREGATE_SHA256
    ):
        raise ContractError("surface manifest does not match the pinned stable schema tree")
    if not isinstance(entries, list):
        raise ContractError("surface manifest has no entries array")
    stable_clients = [
        entry
        for entry in entries
        if entry["category"] == "client_request"
        and entry["stability"] == "stable"
    ]
    if len(stable_clients) != 87:
        raise ContractError(
            f"expected 87 stable client requests, got {len(stable_clients)}"
        )

    rust_clients_by_method = {association.method: association for association in client_rust}
    contracts: list[dict[str, Any]] = []
    for entry in stable_clients:
        method = entry["name"]
        association = rust_clients_by_method.get(method)
        if association is None:
            raise ContractError(
                f"stable client request lacks vendored Rust association: {method}"
            )
        if association.experimental:
            raise ContractError(
                f"stable client request is marked experimental in Rust: {method}"
            )
        parameter_type = normalize_rust_type(
            association.parameter_type, parameter=True
        )
        manifest_parameter = entry["params"]["type"] or "Unit"
        if parameter_type != manifest_parameter:
            raise ContractError(
                f"{method} Rust parameter {parameter_type} disagrees with "
                f"schema {manifest_parameter}"
            )
        result_type = normalize_rust_type(association.result_type)
        parameter_schema = (
            None
            if parameter_type == "Unit"
            else schema_for_type(schema_root, parameter_type)
        )
        result_schema = schema_for_type(schema_root, result_type)
        contract_kind = result_contract_kind(schema_root, result_schema)
        contract_result_type = "Unit" if contract_kind == "Unit" else result_type
        contracts.append(
            {
                "surface_key": exact_surface_key(entry),
                "parameter_type_identity": parameter_type,
                "result_type_identity": contract_result_type,
                "result_schema_type_identity": result_type,
                "result_contract_kind": contract_kind,
                "association_evidence_kind": "VendoredRust",
                "association_evidence_key": (
                    f"{COMMON_PATH}#client_request_definitions!/"
                    f"{association.variant}"
                ),
                "parameter_schema": parameter_schema,
                "result_schema": result_schema,
                "rust": {
                    "variant": association.variant,
                    "wire_method": association.method,
                    "parameter_type": association.parameter_type,
                    "result_type": association.result_type,
                },
                "cross_check_evidence": [],
            }
        )

    stable_methods = {entry["name"] for entry in stable_clients}
    nonexperimental_extras = sorted(
        association.method
        for association in client_rust
        if not association.experimental and association.method not in stable_methods
    )
    expected_extras = ["getAuthStatus", "getConversationSummary", "gitDiffToRemote"]
    if nonexperimental_extras != expected_extras:
        raise ContractError(
            "unexpected non-experimental Rust declarations outside stable schema: "
            + ", ".join(nonexperimental_extras)
        )

    rust_servers_by_method = {association.method: association for association in server_rust}
    for pair in schema_server_pairs(schema_root, entries):
        entry = pair["entry"]
        method = entry["name"]
        rust = rust_servers_by_method.get(method)
        if rust is None or rust.experimental:
            raise ContractError(
                f"stable schema server request lacks non-experimental Rust cross-check: {method}"
            )
        rust_parameter = normalize_rust_type(rust.parameter_type, parameter=True)
        rust_result = normalize_rust_type(rust.result_type)
        if (
            rust_parameter != pair["parameter_type"]
            or rust_result != pair["response_type"]
        ):
            raise ContractError(
                f"Rust/schema server association conflict for {method}: "
                f"{rust_parameter}->{rust_result} versus "
                f"{pair['parameter_type']}->{pair['response_type']}"
            )
        contract_kind = result_contract_kind(schema_root, pair["result_schema"])
        contract_result_type = (
            "Unit" if contract_kind == "Unit" else pair["response_type"]
        )
        contracts.append(
            {
                "surface_key": exact_surface_key(entry),
                "parameter_type_identity": pair["parameter_type"],
                "result_type_identity": contract_result_type,
                "result_schema_type_identity": pair["response_type"],
                "result_contract_kind": contract_kind,
                "association_evidence_kind": "VendoredSchemaPair",
                "association_evidence_key": (
                    f"stable/ServerRequest.json#/oneOf/{pair['branch']}+"
                    f"{pair['result_schema']}"
                ),
                "parameter_schema": pair["parameter_schema"],
                "result_schema": pair["result_schema"],
                "schema_pair": {
                    "request_union": "stable/ServerRequest.json",
                    "branch": pair["branch"],
                    "parameter_schema": pair["parameter_schema"],
                    "response_schema": pair["result_schema"],
                },
                "cross_check_evidence": [
                    {
                        "kind": "VendoredRust",
                        "key": (
                            f"{COMMON_PATH}#server_request_definitions!/"
                            f"{rust.variant}"
                        ),
                        "parameter_type_identity": rust_parameter,
                        "result_type_identity": contract_result_type,
                        "result_schema_type_identity": rust_result,
                        "result_contract_kind": contract_kind,
                        "agrees": True,
                    }
                ],
            }
        )

    contracts.sort(
        key=lambda contract: (
            contract["surface_key"]["category"],
            contract["surface_key"]["domain"],
            contract["surface_key"]["field"],
            contract["surface_key"]["name"],
        )
    )
    if len(contracts) != 97:
        raise ContractError(f"expected 97 stable operation contracts, got {len(contracts)}")
    unit_result_methods = tuple(
        contract["surface_key"]["name"]
        for contract in contracts
        if contract["result_contract_kind"] == "Unit"
    )
    if unit_result_methods != EXPECTED_UNIT_RESULT_METHODS:
        raise ContractError(
            "schema-derived Unit result identity set changed: "
            f"expected {list(EXPECTED_UNIT_RESULT_METHODS)}, "
            f"got {list(unit_result_methods)}"
        )
    result_contract_counts = {
        kind: sum(
            contract["result_contract_kind"] == kind for contract in contracts
        )
        for kind in ("Concrete", "Unit", "Nullable", "ProtocolSpecial")
    }
    return {
        "format_version": FORMAT_VERSION,
        "generated_notice": (
            "GENERATED by tools/codex/app_server_contracts.py; do not edit."
        ),
        "codex_version": VERSION,
        "source_commit_sha": SOURCE_COMMIT,
        "authority": {
            "client_requests": "VendoredRust",
            "server_requests": "VendoredSchemaPair",
            "typescript": "negative audit only; no request-to-response mapping",
        },
        "counts": {
            "stable_client_requests": 87,
            "stable_server_requests": 10,
            "total": 97,
        },
        "inputs": {
            "rust_source_sha256": COMMON_SHA256,
            "stable_schema_aggregate_sha256": STABLE_SCHEMA_AGGREGATE_SHA256,
            "surface_manifest_sha256": sha256(manifest_path),
        },
        "result_contract_counts": result_contract_counts,
        "unit_result_surfaces": list(unit_result_methods),
        "rust_nonexperimental_not_in_stable_schema": nonexperimental_extras,
        "contracts": contracts,
    }


def typescript_audit(
    schema_provenance_path: Path,
    common_path: Path,
    ts_stable: Path | None = None,
    ts_experimental: Path | None = None,
) -> dict[str, Any]:
    provenance = load_json(schema_provenance_path)
    if (
        not isinstance(provenance, dict)
        or provenance.get("codex_version") != CODEX_VERSION
        or provenance.get("upstream", {})
        .get("release", {})
        .get("source_commit_sha")
        != SOURCE_COMMIT
        or provenance.get("upstream", {}).get("release", {}).get("tag") != TAG
        or provenance.get("schema_trees", {})
        .get("stable", {})
        .get("aggregate_sha256")
        != STABLE_SCHEMA_AGGREGATE_SHA256
    ):
        raise ContractError("schema provenance does not match the pinned upstream source")
    cross_check = provenance.get("typescript_cross_check", {})
    for mode, values in TYPESCRIPT_AUDIT.items():
        actual = cross_check.get(mode, {})
        for key in ("aggregate_sha256", "file_count"):
            value = values[key]
            if actual.get(key) != value:
                raise ContractError(
                    f"pinned TypeScript {mode} {key} changed: "
                    f"expected {value}, got {actual.get(key)}"
                )
        discrepancy = actual.get("discrepancies", {}).get("methods", {}).get(
            "client_request", {}
        )
        if discrepancy.get("typescript_only") != [
            "getAuthStatus",
            "getConversationSummary",
            "gitDiffToRemote",
        ]:
            raise ContractError(
                f"pinned TypeScript {mode} discrepancy record changed"
            )
    if not isinstance(cross_check.get("authority_rule"), str):
        raise ContractError("pinned TypeScript authority rule is absent")
    if (ts_stable is None) != (ts_experimental is None):
        raise ContractError(
            "both stable and experimental generated TypeScript trees are required"
        )
    if ts_stable is not None and ts_experimental is not None:
        supplied_trees = {
            "stable": ts_stable,
            "experimental": ts_experimental,
        }
        for mode, root in supplied_trees.items():
            actual_scan = scan_typescript_tree(root)
            if actual_scan != TYPESCRIPT_AUDIT[mode]:
                raise ContractError(
                    f"pinned TypeScript {mode} negative audit changed: "
                    f"expected {TYPESCRIPT_AUDIT[mode]}, got {actual_scan}"
                )
    common = common_path.read_text(encoding="utf-8")
    if "pub fn export_client_responses" not in common:
        raise ContractError("vendored Rust no longer exports individual client responses")
    return {
        "format_version": FORMAT_VERSION,
        "generated_notice": (
            "GENERATED by tools/codex/app_server_contracts.py; do not edit."
        ),
        "codex_version": VERSION,
        "source_commit_sha": SOURCE_COMMIT,
        "audited_trees": TYPESCRIPT_AUDIT,
        "client_response_file_present": False,
        "audit_procedure": {
            "command": (
                "python3 tools/codex/app_server_contracts.py --check "
                "--ts-stable <stable-generate-ts-root> "
                "--ts-experimental <experimental-generate-ts-root>"
            ),
            "tree_identity": (
                "sha256 of sorted '<file-sha256>  <relative-path>\\n' records"
            ),
            "path_identity": "sha256 of sorted '<relative-path>\\n' records",
            "mapping_criteria": [
                (
                    "locate every exact ClientRequest method literal across "
                    "the complete generated tree"
                ),
                (
                    "reject a client method outside ClientRequest.ts or a "
                    "method-union response/result field"
                ),
                (
                    "reject any ClientResponse.ts path or ClientResponse "
                    "type/interface declaration"
                ),
                (
                    "review every file co-referencing a ClientRequest "
                    "parameter type and a response-named type; only flat "
                    "index exports are accepted"
                ),
                (
                    "enumerate broader Request/Response/Result/Map names and "
                    "reject conditional or mapped association constructs"
                ),
            ],
            "scope": (
                "all files in both exact pinned generate-ts trees; the trees "
                "are audit inputs and are not runtime or source-package inputs"
            ),
        },
        "existing_surface_cross_check": cross_check,
        "finding": {
            "independent_request_response_mapping": False,
            "client_request_mapping": "method -> params only",
            "parameter_response_coreferences": (
                "only generated flat export indexes; no association construct"
            ),
            "broader_name_review": (
                "five server-request response type names in each tree; none "
                "co-reference a ClientRequest parameter or method"
            ),
            "response_export": (
                "individual response type files and index exports; no "
                "ClientResponse.ts envelope or method association"
            ),
            "action": (
                "do not vendor the TypeScript tree; Rust remains authoritative "
                "for client successful-result associations"
            ),
        },
        "audit_note": (
            "The deterministic scanner was run over the exact pinned stable "
            "and experimental generated trees at the recorded aggregate and "
            "path-set hashes. Every client method literal occurs only in "
            "ClientRequest.ts, whose union arms contain method/id/params and "
            "no response/result association. The only parameter/response "
            "co-reference files are flat export indexes; no conditional, "
            "mapped, or broadly named association construct was found."
        ),
    }


def write_or_check(path: Path, document: dict[str, Any], check: bool) -> None:
    generated = canonical_json(document)
    if check:
        try:
            committed = path.read_text(encoding="utf-8")
        except OSError as error:
            raise ContractError(f"missing generated artifact {path}: {error}") from error
        if committed != generated:
            raise ContractError(f"generated artifact is stale: {path}")
    else:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(generated, encoding="utf-8")


def generate(arguments: argparse.Namespace) -> None:
    provenance = source_provenance(arguments.source_root)
    contracts = build_contracts(
        arguments.source_root, arguments.schema_root, arguments.manifest
    )
    audit = typescript_audit(
        arguments.schema_provenance,
        arguments.source_root / COMMON_PATH,
        getattr(arguments, "ts_stable", None),
        getattr(arguments, "ts_experimental", None),
    )
    write_or_check(
        arguments.source_root / "PROVENANCE.json", provenance, arguments.check
    )
    write_or_check(
        arguments.evidence_root / "operation-contracts.json",
        contracts,
        arguments.check,
    )
    write_or_check(
        arguments.evidence_root / "typescript-audit.json",
        audit,
        arguments.check,
    )


def parser() -> argparse.ArgumentParser:
    result = argparse.ArgumentParser(description=__doc__)
    result.add_argument("--source-root", type=Path, default=DEFAULT_SOURCE_ROOT)
    result.add_argument("--schema-root", type=Path, default=DEFAULT_SCHEMA_ROOT)
    result.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    result.add_argument(
        "--schema-provenance", type=Path, default=DEFAULT_SCHEMA_PROVENANCE
    )
    result.add_argument("--evidence-root", type=Path, default=DEFAULT_EVIDENCE_ROOT)
    result.add_argument(
        "--ts-stable",
        type=Path,
        help="optional exact stable generate-ts tree for a reproducible negative audit",
    )
    result.add_argument(
        "--ts-experimental",
        type=Path,
        help=(
            "optional exact experimental generate-ts tree for a reproducible "
            "negative audit"
        ),
    )
    result.add_argument("--check", action="store_true")
    return result


def main(argv: Sequence[str] | None = None) -> int:
    arguments = parser().parse_args(argv)
    try:
        generate(arguments)
    except ContractError as error:
        print(f"app-server-contracts: error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
