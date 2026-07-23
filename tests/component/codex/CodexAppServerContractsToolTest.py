#!/usr/bin/env python3

"""Focused offline guards for pinned Codex operation-contract evidence."""

from __future__ import annotations

import argparse
import copy
import hashlib
import importlib.util
import json
import shutil
import sys
import tempfile
from pathlib import Path
from types import ModuleType, SimpleNamespace
from typing import Callable

sys.dont_write_bytecode = True


def load_tool(path: Path) -> ModuleType:
    specification = importlib.util.spec_from_file_location(
        "snodec_codex_app_server_contracts", path
    )
    if specification is None or specification.loader is None:
        raise RuntimeError(f"unable to load operation-contract tool: {path}")
    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


def load_json(path: Path) -> object:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, value: object) -> None:
    path.write_text(
        json.dumps(value, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def git_blob_sha(data: bytes) -> str:
    return hashlib.sha1(f"blob {len(data)}\0".encode("ascii") + data).hexdigest()


def expect_contract_error(
    tool: ModuleType,
    action: Callable[[], object],
    expected_fragment: str,
    description: str,
) -> None:
    try:
        action()
    except tool.ContractError as error:
        if expected_fragment not in str(error):
            raise AssertionError(
                f"{description} emitted an unrelated diagnostic: {error}"
            ) from error
        return
    raise AssertionError(f"negative mutation did not fail: {description}")


def assert_exact_provenance(
    tool: ModuleType, source_root: Path, provenance_path: Path
) -> None:
    generated = tool.source_provenance(source_root)
    committed = load_json(provenance_path)
    if generated != committed:
        raise AssertionError("source provenance differs from exact regenerated evidence")

    records = generated.get("files")
    if not isinstance(records, list) or len(records) != 3:
        raise AssertionError("source provenance must contain exactly three pinned files")
    for record in records:
        path = source_root / record["path"]
        data = path.read_bytes()
        if (
            len(data) != record["bytes"]
            or hashlib.sha256(data).hexdigest() != record["sha256"]
            or git_blob_sha(data) != record["git_blob_sha"]
        ):
            raise AssertionError(
                f"independent source provenance mismatch for {record['path']}"
            )


def assert_exact_contracts(
    tool: ModuleType,
    source_root: Path,
    schema_root: Path,
    manifest_path: Path,
    contracts_path: Path,
) -> None:
    first = tool.build_contracts(source_root, schema_root, manifest_path)
    second = tool.build_contracts(source_root, schema_root, manifest_path)
    if tool.canonical_json(first) != tool.canonical_json(second):
        raise AssertionError("operation-contract extraction is not deterministic")
    if first != load_json(contracts_path):
        raise AssertionError("committed operation contracts are stale")

    contracts = first.get("contracts")
    if not isinstance(contracts, list):
        raise AssertionError("operation-contract evidence lacks a contracts array")
    client = [
        row
        for row in contracts
        if row["surface_key"]["category"] == "client_request"
    ]
    server = [
        row
        for row in contracts
        if row["surface_key"]["category"] == "server_request"
    ]
    if len(client) != 87 or len(server) != 10 or len(contracts) != 97:
        raise AssertionError("operation-contract evidence is not exact 87/10/97")

    keys = [
        (
            row["surface_key"]["category"],
            row["surface_key"]["domain"],
            row["surface_key"]["field"],
            row["surface_key"]["name"],
        )
        for row in contracts
    ]
    if keys != sorted(keys) or len(keys) != len(set(keys)):
        raise AssertionError("operation-contract keys are not sorted and unique")
    if any(row["association_evidence_kind"] != "VendoredRust" for row in client):
        raise AssertionError("a stable client association is not Rust-authoritative")
    if any(
        row["association_evidence_kind"] != "VendoredSchemaPair" for row in server
    ):
        raise AssertionError("a stable server association is not schema-pair-authoritative")

    unit = [
        row for row in contracts if row["result_contract_kind"] == "Unit"
    ]
    unit_names = [row["surface_key"]["name"] for row in unit]
    if unit_names != list(tool.EXPECTED_UNIT_RESULT_METHODS) or len(unit) != 21:
        raise AssertionError("schema-derived Unit result identity set changed")
    for row in unit:
        if row["result_type_identity"] != "Unit":
            raise AssertionError("Unit result lacks its explicit canonical identity")
        if not row.get("result_schema_type_identity") or not row.get("result_schema"):
            raise AssertionError("Unit result lost its named Rust/schema identity")
        schema = load_json(schema_root / row["result_schema"])
        if (
            schema.get("type") != "object"
            or schema.get("properties") not in (None, {})
            or schema.get("required") not in (None, [])
        ):
            raise AssertionError("Unit result is not backed by an empty-object schema")
    if first.get("result_contract_counts") != {
        "Concrete": 76,
        "Nullable": 0,
        "ProtocolSpecial": 0,
        "Unit": 21,
    }:
        raise AssertionError("result-contract kind metrics changed")

    for row in server:
        checks = row.get("cross_check_evidence")
        if not isinstance(checks, list) or len(checks) != 1:
            raise AssertionError("server schema pair lacks one Rust cross-check")
        check = checks[0]
        if (
            check.get("kind") != "VendoredRust"
            or check.get("agrees") is not True
            or check.get("parameter_type_identity")
            != row["parameter_type_identity"]
            or check.get("result_type_identity") != row["result_type_identity"]
            or check.get("result_schema_type_identity")
            != row["result_schema_type_identity"]
        ):
            raise AssertionError("server Rust/schema cross-check does not agree exactly")


def assert_typescript_audit(
    tool: ModuleType,
    schema_provenance: Path,
    common_path: Path,
    audit_path: Path,
) -> None:
    generated = tool.typescript_audit(schema_provenance, common_path)
    if generated != load_json(audit_path):
        raise AssertionError("committed TypeScript negative audit is stale")
    finding = generated.get("finding", {})
    if finding.get("independent_request_response_mapping") is not False:
        raise AssertionError("TypeScript audit invented a request/response mapping")
    if generated.get("client_response_file_present") is not False:
        raise AssertionError("TypeScript audit invented ClientResponse.ts")
    procedure = generated.get("audit_procedure", {})
    if (
        "--ts-stable" not in procedure.get("command", "")
        or "--ts-experimental" not in procedure.get("command", "")
        or len(procedure.get("mapping_criteria", [])) != 5
    ):
        raise AssertionError("TypeScript audit dropped its reproducible scan procedure")
    for mode, expected in tool.TYPESCRIPT_AUDIT.items():
        if generated["audited_trees"].get(mode) != expected:
            raise AssertionError(f"TypeScript {mode} tree evidence changed")
        if (
            expected["client_method_literal_files"]
            != [
                {
                    "path": "ClientRequest.ts",
                    "method_literal_count": expected["client_request"][
                        "method_arms"
                    ],
                    "sha256": expected["client_request"]["sha256"],
                }
            ]
            or expected["client_response_named_paths"]
            or expected["client_response_declarations"]
            or any(
                record["classification"] != "flat_export_index"
                for record in expected[
                    "parameter_response_coreference_files"
                ]
            )
            or len(expected["broader_name_review_files"]) != 5
            or expected["association_construct_candidates"]
            or expected["request_response_mapping_candidates"]
        ):
            raise AssertionError(
                f"TypeScript {mode} negative scan no longer proves absence"
            )
    cross_check = generated.get("existing_surface_cross_check")
    if not isinstance(cross_check, dict) or not isinstance(
        cross_check.get("authority_rule"), str
    ):
        raise AssertionError("TypeScript audit dropped the A0 authority rule")
    for mode in ("stable", "experimental"):
        methods = cross_check[mode]["discrepancies"]["methods"]["client_request"]
        if methods.get("typescript_only") != [
            "getAuthStatus",
            "getConversationSummary",
            "gitDiffToRemote",
        ]:
            raise AssertionError("TypeScript audit dropped the A0 discrepancy record")


def assert_typescript_scanner(tool: ModuleType) -> None:
    with tempfile.TemporaryDirectory(
        prefix="snodec-codex-typescript-negative-audit-"
    ) as raw:
        root = Path(raw)
        (root / "ClientRequest.ts").write_text(
            (
                'export type ClientRequest = { "method": "thread/start", '
                "id: number, params: object } | "
                '{ "method": "turn/start", id: number, params: object };\n'
            ),
            encoding="utf-8",
        )
        (root / "ThreadStartResponse.ts").write_text(
            "export type ThreadStartResponse = { threadId: string };\n",
            encoding="utf-8",
        )
        clean = tool.scan_typescript_tree(root)
        if clean["request_response_mapping_candidates"]:
            raise AssertionError("TypeScript scanner invented a response mapping")
        if clean["client_method_literal_files"][0]["path"] != "ClientRequest.ts":
            raise AssertionError("TypeScript scanner lost the request union")

        (root / "UnexpectedMap.ts").write_text(
            (
                'export type UnexpectedMap = { "method": "thread/start", '
                "response: ThreadStartResponse };\n"
            ),
            encoding="utf-8",
        )
        mapped = tool.scan_typescript_tree(root)
        reasons = {
            candidate["reason"]
            for candidate in mapped["request_response_mapping_candidates"]
        }
        if reasons != {
            "contains an exact ClientRequest method literal",
            "method union UnexpectedMap has a response/result field",
        }:
            raise AssertionError(
                f"TypeScript scanner missed a synthetic mapping: {reasons}"
            )

        (root / "Pairing.ts").write_text(
            (
                "import type { ClientRequest } from './ClientRequest';\n"
                "import type { ThreadStartResponse } from "
                "'./ThreadStartResponse';\n"
                "export type Pairing<T extends ClientRequest> = "
                "T extends ClientRequest ? [T, ThreadStartResponse] : never;\n"
            ),
            encoding="utf-8",
        )
        paired = tool.scan_typescript_tree(root)
        pairing_reasons = {
            candidate["reason"]
            for candidate in paired["request_response_mapping_candidates"]
            if candidate["path"] == "Pairing.ts"
        }
        construct_reasons = {
            candidate["reason"]
            for candidate in paired["association_construct_candidates"]
            if candidate["path"] == "Pairing.ts"
        }
        if pairing_reasons != {
            "co-references ClientRequest identities and response-named types outside a flat export index"
        } or construct_reasons != {
            "conditional/mapped construct co-references client request and response identities"
        }:
            raise AssertionError(
                "TypeScript scanner missed a request-union/response conditional "
                f"association: mapping={pairing_reasons}, "
                f"construct={construct_reasons}"
            )

        (root / "ClientResponse.ts").write_text(
            "export interface ClientResponse { result: unknown }\n",
            encoding="utf-8",
        )
        named = tool.scan_typescript_tree(root)
        if named["client_response_named_paths"] != ["ClientResponse.ts"]:
            raise AssertionError("TypeScript scanner missed ClientResponse.ts")
        if named["client_response_declarations"] != ["ClientResponse.ts"]:
            raise AssertionError("TypeScript scanner missed ClientResponse declaration")


def mutation_checks(
    tool: ModuleType,
    source_root: Path,
    schema_root: Path,
    manifest_path: Path,
    schema_provenance: Path,
    evidence_root: Path,
) -> None:
    with tempfile.TemporaryDirectory(
        prefix="snodec-codex-operation-contracts-"
    ) as raw:
        temporary = Path(raw)
        copied_source = temporary / "source"
        shutil.copytree(source_root, copied_source)
        common = copied_source / tool.COMMON_PATH
        common.write_bytes(common.read_bytes() + b"\n")
        expect_contract_error(
            tool,
            lambda: tool.source_provenance(copied_source),
            "integrity mismatch",
            "corrupt the exact vendored Rust bytes",
        )
        shutil.copy2(source_root / tool.COMMON_PATH, common)

        extra = (
            copied_source
            / "codex-rs/app-server-protocol/src/protocol/untracked-authority.rs"
        )
        extra.write_text("// not pinned\n", encoding="utf-8")
        expect_contract_error(
            tool,
            lambda: tool.source_provenance(copied_source),
            "vendored Rust evidence file set mismatch",
            "add an untracked authority source",
        )
        extra.unlink()

        symlink = copied_source / "untracked-authority.rs"
        symlink.symlink_to(tool.COMMON_PATH)
        expect_contract_error(
            tool,
            lambda: tool.source_provenance(copied_source),
            "vendored Rust evidence contains a symlink",
            "add a symlink to pinned authority",
        )
        symlink.unlink()

        (copied_source / "NOTICE.openai-codex").unlink()
        expect_contract_error(
            tool,
            lambda: tool.source_provenance(copied_source),
            "missing upstream notice",
            "remove the required upstream NOTICE",
        )

        corrupted_manifest = copy.deepcopy(load_json(manifest_path))
        target = next(
            entry
            for entry in corrupted_manifest["entries"]
            if entry["id"] == "client_request:thread/start"
        )
        target["params"]["type"] = "WrongThreadStartParams"
        copied_manifest = temporary / "surface.json"
        write_json(copied_manifest, corrupted_manifest)
        expect_contract_error(
            tool,
            lambda: tool.build_contracts(source_root, schema_root, copied_manifest),
            "Rust parameter ThreadStartParams disagrees",
            "change the schema-authoritative client parameter identity",
        )

        copied_schema = temporary / "schema"
        shutil.copytree(schema_root, copied_schema)
        response = (
            copied_schema
            / "stable"
            / "CommandExecutionRequestApprovalResponse.json"
        )
        response.unlink()
        expect_contract_error(
            tool,
            lambda: tool.build_contracts(source_root, copied_schema, manifest_path),
            "resolves to 0 roots",
            "remove a schema-authoritative server response pair",
        )
        shutil.copy2(
            schema_root / "stable/CommandExecutionRequestApprovalResponse.json",
            response,
        )
        duplicate = (
            copied_schema
            / "stable/v2/CommandExecutionRequestApprovalResponse.json"
        )
        duplicate.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(response, duplicate)
        expect_contract_error(
            tool,
            lambda: tool.build_contracts(source_root, copied_schema, manifest_path),
            "resolves to 2 roots",
            "make a schema-authoritative server response pair ambiguous",
        )

        corrupted_ts_provenance = copy.deepcopy(load_json(schema_provenance))
        corrupted_ts_provenance["typescript_cross_check"]["stable"][
            "aggregate_sha256"
        ] = "0" * 64
        copied_ts_provenance = temporary / "schema-provenance.json"
        write_json(copied_ts_provenance, corrupted_ts_provenance)
        expect_contract_error(
            tool,
            lambda: tool.typescript_audit(
                copied_ts_provenance, source_root / tool.COMMON_PATH
            ),
            "stable aggregate_sha256 changed",
            "change the pinned TypeScript tree identity",
        )

        copied_evidence = temporary / "evidence"
        shutil.copytree(evidence_root, copied_evidence)
        stale_contracts = copied_evidence / "operation-contracts.json"
        stale_contracts.write_bytes(stale_contracts.read_bytes() + b"\n")
        check_arguments = SimpleNamespace(
            source_root=source_root,
            schema_root=schema_root,
            manifest=manifest_path,
            schema_provenance=schema_provenance,
            evidence_root=copied_evidence,
            check=True,
        )
        expect_contract_error(
            tool,
            lambda: tool.generate(check_arguments),
            "generated artifact is stale",
            "edit the checked-in operation-contract evidence",
        )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tool", required=True, type=Path)
    parser.add_argument("--source-root", required=True, type=Path)
    parser.add_argument("--schema-root", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--schema-provenance", required=True, type=Path)
    parser.add_argument("--evidence-root", required=True, type=Path)
    arguments = parser.parse_args()

    tool = load_tool(arguments.tool)
    assert_exact_provenance(
        tool,
        arguments.source_root,
        arguments.source_root / "PROVENANCE.json",
    )
    assert_exact_contracts(
        tool,
        arguments.source_root,
        arguments.schema_root,
        arguments.manifest,
        arguments.evidence_root / "operation-contracts.json",
    )
    assert_typescript_audit(
        tool,
        arguments.schema_provenance,
        arguments.source_root / tool.COMMON_PATH,
        arguments.evidence_root / "typescript-audit.json",
    )
    assert_typescript_scanner(tool)
    check_arguments = SimpleNamespace(
        source_root=arguments.source_root,
        schema_root=arguments.schema_root,
        manifest=arguments.manifest,
        schema_provenance=arguments.schema_provenance,
        evidence_root=arguments.evidence_root,
        check=True,
    )
    tool.generate(check_arguments)
    mutation_checks(
        tool,
        arguments.source_root,
        arguments.schema_root,
        arguments.manifest,
        arguments.schema_provenance,
        arguments.evidence_root,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
