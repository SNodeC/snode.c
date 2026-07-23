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


def expect_surface_error_code(
    tool: ModuleType,
    action: Callable[[], object],
    expected_code: str,
    description: str,
) -> None:
    try:
        action()
    except tool.SurfaceError as error:
        actual_code = str(error).split(":", 1)[0]
        if actual_code != expected_code:
            raise AssertionError(
                f"{description} emitted {actual_code}, expected {expected_code}"
            ) from error
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
    if provenance.get("format_version") != tool.PROVENANCE_FORMAT_VERSION:
        raise AssertionError("provenance format version is not the pinned version")
    if provenance.get("upstream") != tool.UPSTREAM_PROVENANCE:
        raise AssertionError("provenance lacks the exact pinned upstream attribution")

    for field in ("license_file", "notice_file"):
        expected_file = provenance["upstream"]["license"][field]
        data = (schema_root / expected_file["path"]).read_bytes()
        if (
            len(data) != expected_file["bytes"]
            or hashlib.sha256(data).hexdigest() != expected_file["sha256"]
        ):
            raise AssertionError(
                f"required upstream {field} does not match provenance"
            )

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

        license_record = provenance["upstream"]["license"]["license_file"]
        copied_license = copied_root / license_record["path"]
        source_license = schema_root / license_record["path"]
        copied_license.unlink()
        expect_surface_error(
            tool,
            lambda: tool.verify_provenance(copied_root, copied_provenance),
            "remove the required upstream license",
        )
        shutil.copy2(source_license, copied_license)

        notice_record = provenance["upstream"]["license"]["notice_file"]
        copied_notice = copied_root / notice_record["path"]
        source_notice = schema_root / notice_record["path"]
        copied_notice.write_bytes(copied_notice.read_bytes() + b"\n")
        expect_surface_error(
            tool,
            lambda: tool.verify_provenance(copied_root, copied_provenance),
            "corrupt the required upstream notice",
        )
        shutil.copy2(source_notice, copied_notice)

        corrupted_attribution = copy.deepcopy(provenance)
        corrupted_attribution["upstream"]["release"]["source_commit_sha"] = "0" * 40
        write_json(copied_provenance, corrupted_attribution)
        expect_surface_error(
            tool,
            lambda: tool.verify_provenance(copied_root, copied_provenance),
            "change the pinned upstream source revision",
        )
        write_json(copied_provenance, provenance)

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


def test_conversation_descriptor_guards(
    tool: ModuleType,
    manifest: dict[str, object],
    schema_root: Path,
    evidence: dict[str, object],
    descriptor_path: Path,
) -> None:
    generated = tool.generate_conversation_union_descriptor_data(
        manifest, schema_root, evidence
    )
    if generated != tool.generate_conversation_union_descriptor_data(
        manifest, schema_root, evidence
    ):
        raise AssertionError("conversation-union descriptor generation is not deterministic")
    if generated != descriptor_path.read_text(encoding="utf-8"):
        raise AssertionError(
            "private conversation-union descriptor data is stale"
        )
    if (
        len(tool.CONVERSATION_UNION_CODECS) != 42
        or len(
            {
                metadata[0]
                for metadata in tool.CONVERSATION_UNION_CODECS.values()
            }
        )
        != 42
    ):
        raise AssertionError(
            "conversation-union descriptor map is not an exact 42-key/42-target bijection"
        )

    wrong_assignment = copy.deepcopy(evidence)
    assignment = next(
        row
        for row in wrong_assignment["assignments"]["assignments"]
        if tool.surface_key(row) in tool.CONVERSATION_UNION_CODECS
    )
    assignment["classification"] = "SharedWithinSlice"
    expect_surface_error_code(
        tool,
        lambda: tool.generate_conversation_union_descriptor_data(
            manifest, schema_root, wrong_assignment
        ),
        "ConversationUnionDescriptorAssignmentMismatch",
        "remove one descriptor identity from the exact SharedCommon assignment set",
    )

    with tempfile.TemporaryDirectory(
        prefix="snodec-codex-conversation-descriptors-"
    ) as raw:
        temporary = Path(raw)
        copied_schema_root = temporary / "schema"
        stable = copied_schema_root / "stable"
        stable.mkdir(parents=True)
        aggregate_name = "codex_app_server_protocol.schemas.json"
        aggregate = load_json(schema_root / "stable" / aggregate_name)
        aggregate["definitions"]["v2"]["AskForApproval"]["oneOf"][1][
            "type"
        ] = "array"
        write_json(stable / aggregate_name, aggregate)
        expect_surface_error_code(
            tool,
            lambda: tool.generate_conversation_union_descriptor_data(
                manifest, copied_schema_root, evidence
            ),
            "ConversationUnionDescriptorSchemaMismatch",
            "change a reviewed descriptor branch shape",
        )

        stale = temporary / "ConversationUnionCodecDescriptors.inc"
        stale.write_text(generated + " ", encoding="utf-8")
        expect_surface_error_code(
            tool,
            lambda: tool.write_or_check_conversation_union_descriptors(
                stale, generated, True
            ),
            "StaleGeneratedConversationUnionDescriptors",
            "change the checked-in generated descriptor artifact",
        )

    original_codecs = dict(tool.CONVERSATION_UNION_CODECS)
    keys = sorted(original_codecs)
    try:
        first = original_codecs[keys[0]]
        second = original_codecs[keys[1]]
        tool.CONVERSATION_UNION_CODECS[keys[1]] = (
            first[0],
            second[1],
            second[2],
        )
        expect_surface_error_code(
            tool,
            lambda: tool.generate_conversation_union_descriptor_data(
                manifest, schema_root, evidence
            ),
            "DuplicateConversationUnionDescriptorTarget",
            "duplicate one private descriptor runtime target",
        )
        tool.CONVERSATION_UNION_CODECS[keys[1]] = second
        bidirectional_key = next(
            key
            for key, metadata in original_codecs.items()
            if metadata[2]
            == "ConversationUnionCodecDirection::Bidirectional"
        )
        bidirectional = original_codecs[bidirectional_key]
        tool.CONVERSATION_UNION_CODECS[bidirectional_key] = (
            bidirectional[0],
            bidirectional[1],
            "ConversationUnionCodecDirection::DecodeOnly",
        )
        expect_surface_error_code(
            tool,
            lambda: tool.generate_conversation_union_descriptor_data(
                manifest, schema_root, evidence
            ),
            "ConversationUnionDescriptorDirectionMismatch",
            "change the reviewed codec-direction split",
        )
    finally:
        tool.CONVERSATION_UNION_CODECS.clear()
        tool.CONVERSATION_UNION_CODECS.update(original_codecs)


def test_item_descriptor_guards(
    tool: ModuleType,
    manifest: dict[str, object],
    schema_root: Path,
    evidence: dict[str, object],
    descriptor_root: Path,
) -> None:
    families = (
        (
            "ThreadItem",
            "ItemDiscriminatorTarget",
            "CODEX_THREAD_ITEM_CODEC_DESCRIPTOR",
            18,
            descriptor_root / "ThreadItemCodecDescriptors.inc",
        ),
        (
            "ResponseItem",
            "ResponseItemTarget",
            "CODEX_RESPONSE_ITEM_CODEC_DESCRIPTOR",
            16,
            descriptor_root / "ResponseItemCodecDescriptors.inc",
        ),
    )
    generated_by_domain: dict[str, str] = {}
    for domain, target_prefix, macro, count, path in families:
        generated = tool.generate_item_codec_descriptor_data(
            manifest,
            schema_root,
            domain,
            target_prefix,
            macro,
            count,
            evidence,
        )
        generated_by_domain[domain] = generated
        if generated != tool.generate_item_codec_descriptor_data(
            manifest,
            schema_root,
            domain,
            target_prefix,
            macro,
            count,
            evidence,
        ):
            raise AssertionError(
                f"{domain} descriptor generation is not deterministic"
            )
        if generated != path.read_text(encoding="utf-8"):
            raise AssertionError(
                f"private {domain} descriptor data is stale"
            )
        descriptor_lines = [
            line
            for line in generated.splitlines()
            if line.startswith(macro + "(")
        ]
        if len(descriptor_lines) != count:
            raise AssertionError(
                f"{domain} descriptor artifact does not contain exactly "
                f"{count} generated rows"
            )
        targets = {
            match.group(1)
            for line in descriptor_lines
            if (
                match := re.search(
                    rf", ({re.escape(target_prefix)}::[A-Za-z0-9_]+), "
                    r"ConversationUnionCodecShape::",
                    line,
                )
            )
        }
        if len(targets) != count:
            raise AssertionError(
                f"{domain} descriptor targets are not an exact bijection"
            )

        wrong_assignment = copy.deepcopy(evidence)
        assignment = next(
            row
            for row in wrong_assignment["assignments"]["assignments"]
            if tool.surface_key(row)[0:2]
            == ("item_discriminator", domain)
        )
        assignment["classification"] = "SharedWithinSlice"
        expect_surface_error_code(
            tool,
            lambda: tool.generate_item_codec_descriptor_data(
                manifest,
                schema_root,
                domain,
                target_prefix,
                macro,
                count,
                wrong_assignment,
            ),
            "ItemCodecDescriptorAssignmentMismatch",
            f"remove one exact {domain} assignment",
        )

    with tempfile.TemporaryDirectory(
        prefix="snodec-codex-item-descriptors-"
    ) as raw:
        temporary = Path(raw)
        copied_schema_root = temporary / "schema"
        stable = copied_schema_root / "stable"
        stable.mkdir(parents=True)
        aggregate_name = "codex_app_server_protocol.schemas.json"
        source_aggregate = load_json(
            schema_root / "stable" / aggregate_name
        )
        for domain, target_prefix, macro, count, _ in families:
            aggregate = copy.deepcopy(source_aggregate)
            aggregate["definitions"]["v2"][domain]["oneOf"][0][
                "type"
            ] = "array"
            write_json(stable / aggregate_name, aggregate)
            expect_surface_error_code(
                tool,
                lambda: tool.generate_item_codec_descriptor_data(
                    manifest,
                    copied_schema_root,
                    domain,
                    target_prefix,
                    macro,
                    count,
                    evidence,
                ),
                "ConversationUnionDescriptorSchemaMismatch",
                f"change one reviewed {domain} descriptor branch shape",
            )

        for domain, _, _, _, _ in families:
            generated = generated_by_domain[domain]
            stale = temporary / f"{domain}CodecDescriptors.inc"
            stale.write_text(generated + " ", encoding="utf-8")
            expect_surface_error_code(
                tool,
                lambda: tool.write_or_check_item_codec_descriptors(
                    stale, generated, True
                ),
                "StaleGeneratedItemCodecDescriptors",
                f"change the checked-in {domain} descriptor artifact",
            )

    original_targets = dict(tool.RUNTIME_TARGETS)
    try:
        thread_keys = sorted(
            key
            for key in tool.RUNTIME_TARGETS
            if key[0:2] == ("item_discriminator", "ThreadItem")
        )
        tool.RUNTIME_TARGETS[thread_keys[1]] = tool.RUNTIME_TARGETS[
            thread_keys[0]
        ]
        expect_surface_error_code(
            tool,
            lambda: tool.generate_item_codec_descriptor_data(
                manifest,
                schema_root,
                "ThreadItem",
                "ItemDiscriminatorTarget",
                "CODEX_THREAD_ITEM_CODEC_DESCRIPTOR",
                18,
                evidence,
            ),
            "ItemCodecDescriptorAssignmentMismatch",
            "duplicate one private ThreadItem descriptor target",
        )
    finally:
        tool.RUNTIME_TARGETS.clear()
        tool.RUNTIME_TARGETS.update(original_targets)


def test_generated_artifacts(
    tool: ModuleType,
    schema_root: Path,
    manifest_path: Path,
    provenance_path: Path,
    registry_path: Path,
    coverage_path: Path,
    security_path: Path,
) -> None:
    manifest = load_json(manifest_path)
    provenance = load_json(provenance_path)
    evidence = tool.load_a1_registry_evidence()
    test_conversation_descriptor_guards(
        tool,
        manifest,
        schema_root,
        evidence,
        registry_path.with_name("ConversationUnionCodecDescriptors.inc"),
    )
    test_item_descriptor_guards(
        tool,
        manifest,
        schema_root,
        evidence,
        registry_path.parent,
    )
    generated_registry = tool.generate_registry_data(manifest)
    if generated_registry != registry_path.read_text(encoding="utf-8"):
        raise AssertionError(
            "canonical production registry data is stale against local status mappings"
        )
    registry_entries = tool.parse_registry_data(registry_path)
    test_operation_association_guards(tool, manifest, evidence, registry_entries)
    test_assignment_reachability_guards(tool, manifest, evidence)
    test_evidence_authority_boundaries(tool, manifest, evidence)
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


def test_operation_association_guards(
    tool: ModuleType,
    manifest: dict[str, object],
    evidence: dict[str, object],
    registry_entries: list[dict[str, object]],
) -> None:
    contracts_document = evidence["operation_contracts"]
    if not isinstance(contracts_document, dict):
        raise AssertionError("operation-contract evidence must be an object")

    def codes(
        mutated: dict[str, object],
        registry: list[dict[str, object]] | None = registry_entries,
    ) -> list[str]:
        return sorted(
            diagnostic["code"]
            for diagnostic in tool.association_diagnostics(
                manifest, mutated, registry
            )
        )

    if codes(contracts_document) != []:
        raise AssertionError(
            "operation evidence and canonical registry do not agree bidirectionally"
        )
    contracts = contracts_document["contracts"]
    if not isinstance(contracts, list):
        raise AssertionError("operation-contract evidence has no contracts array")
    stable_client = [
        contract
        for contract in contracts
        if contract["surface_key"]["category"] == "client_request"
    ]
    stable_server = [
        contract
        for contract in contracts
        if contract["surface_key"]["category"] == "server_request"
    ]
    if len(stable_client) != 87 or len(stable_server) != 10:
        raise AssertionError(
            "operation evidence does not contain exact 87/87 client and 10/10 server associations"
        )
    concrete_results = sum(
        contract["result_contract_kind"] == "Concrete" for contract in contracts
    )
    unit_results = sum(
        contract["result_contract_kind"] == "Unit" for contract in contracts
    )
    if (
        concrete_results != 76
        or unit_results != 21
        or any(not contract["result_type_identity"] for contract in contracts)
        or any(
            contract["result_type_identity"] != "Unit"
            for contract in contracts
            if contract["result_contract_kind"] == "Unit"
        )
    ):
        raise AssertionError(
            "operation evidence must preserve 76 Concrete and 21 explicit Unit result identities"
        )

    first_key = tool.surface_key(contracts[0])
    missing_registry = [
        entry
        for entry in copy.deepcopy(registry_entries)
        if (
            entry["category"],
            entry["domain"],
            entry["discriminator_field"],
            entry["name"],
        )
        != first_key
    ]
    if codes(contracts_document, missing_registry) != ["StaleAssociation"]:
        raise AssertionError(
            "evidence without one resolving registry row emitted unrelated diagnostics"
        )

    wrong_category_registry = copy.deepcopy(registry_entries)
    registry_contract = next(
        entry
        for entry in wrong_category_registry
        if (
            entry["category"],
            entry["domain"],
            entry["discriminator_field"],
            entry["name"],
        )
        == first_key
    )
    registry_contract["category"] = "server_notification"
    if codes(contracts_document, wrong_category_registry) != [
        "WrongAssociationCategory"
    ]:
        raise AssertionError(
            "registry/evidence category disagreement emitted unrelated diagnostics"
        )

    def mutate_contracts(action: Callable[[list[dict[str, object]]], None]) -> dict[str, object]:
        mutated = copy.deepcopy(contracts_document)
        action(mutated["contracts"])
        return mutated

    missing = mutate_contracts(lambda rows: rows.pop(0))
    if codes(missing) != ["MissingAssociation"]:
        raise AssertionError("missing association mutation emitted unrelated diagnostics")

    duplicate = mutate_contracts(lambda rows: rows.append(copy.deepcopy(rows[0])))
    if codes(duplicate) != ["DuplicateAssociation"]:
        raise AssertionError("duplicate association mutation emitted unrelated diagnostics")

    def duplicate_primary_evidence_identity(
        rows: list[dict[str, object]],
    ) -> None:
        rows[1]["association_evidence_key"] = rows[0][
            "association_evidence_key"
        ]

    duplicate_primary = mutate_contracts(duplicate_primary_evidence_identity)
    duplicate_primary_registry = copy.deepcopy(registry_entries)
    duplicate_owner_key = tool.surface_key(
        duplicate_primary["contracts"][1]
    )
    duplicate_owner_registry = next(
        entry
        for entry in duplicate_primary_registry
        if (
            entry["category"],
            entry["domain"],
            entry["discriminator_field"],
            entry["name"],
        )
        == duplicate_owner_key
    )
    duplicate_owner_registry["association_evidence_key"] = (
        duplicate_primary["contracts"][1]["association_evidence_key"]
    )
    if codes(duplicate_primary, duplicate_primary_registry) != [
        "DuplicateAssociation"
    ]:
        raise AssertionError(
            "duplicate authoritative evidence identity emitted unrelated diagnostics"
        )

    def duplicate_cross_check_evidence_identity(
        rows: list[dict[str, object]],
    ) -> None:
        server_rows = [
            row
            for row in rows
            if row["surface_key"]["category"] == "server_request"
        ]
        server_rows[1]["cross_check_evidence"][0]["key"] = server_rows[0][
            "cross_check_evidence"
        ][0]["key"]

    if codes(mutate_contracts(duplicate_cross_check_evidence_identity)) != [
        "DuplicateAssociation"
    ]:
        raise AssertionError(
            "duplicate cross-check evidence identity emitted unrelated diagnostics"
        )

    def add_stale(rows: list[dict[str, object]]) -> None:
        stale = copy.deepcopy(rows[0])
        stale["surface_key"]["name"] = "phaseA1/stale-association"
        rows.append(stale)

    if codes(mutate_contracts(add_stale)) != ["StaleAssociation"]:
        raise AssertionError("stale association mutation emitted unrelated diagnostics")

    def wrong_category(rows: list[dict[str, object]]) -> None:
        rows[0]["surface_key"]["category"] = "server_notification"

    if codes(mutate_contracts(wrong_category)) != ["WrongAssociationCategory"]:
        raise AssertionError("wrong association category emitted unrelated diagnostics")

    def wrong_parameter(rows: list[dict[str, object]]) -> None:
        rows[0]["parameter_type_identity"] = "PhaseA1WrongParameter"

    if codes(mutate_contracts(wrong_parameter)) != ["WrongParameterType"]:
        raise AssertionError("wrong parameter identity emitted unrelated diagnostics")

    coordinated_missing_parameter_schema = mutate_contracts(lambda rows: None)
    coordinated_parameter_server = next(
        row
        for row in coordinated_missing_parameter_schema["contracts"]
        if row["surface_key"]["category"] == "server_request"
    )
    coordinated_parameter_schema = (
        "stable/nonexistent/"
        f"{coordinated_parameter_server['parameter_type_identity']}.json"
    )
    coordinated_parameter_server[
        "parameter_schema"
    ] = coordinated_parameter_schema
    coordinated_parameter_server["schema_pair"][
        "parameter_schema"
    ] = coordinated_parameter_schema
    if codes(coordinated_missing_parameter_schema) != ["WrongParameterType"]:
        raise AssertionError(
            "coordinated nonexistent parameter-schema path emitted an "
            "inexact diagnostic multiset"
        )

    def wrong_rust_parameter(rows: list[dict[str, object]]) -> None:
        rows[0]["rust"]["parameter_type"] = "v2::PhaseA1WrongParameter"

    if codes(mutate_contracts(wrong_rust_parameter)) != [
        "ConflictingAssociationEvidence"
    ]:
        raise AssertionError(
            "vendored Rust parameter disagreement emitted unrelated diagnostics"
        )

    coordinated_rust_variant = mutate_contracts(lambda rows: None)
    coordinated_client = next(
        row
        for row in coordinated_rust_variant["contracts"]
        if row["surface_key"]["category"] == "client_request"
    )
    coordinated_client["rust"]["variant"] = "PhaseA1MissingVariant"
    coordinated_client["association_evidence_key"] = (
        "codex-rs/app-server-protocol/src/protocol/common.rs"
        "#client_request_definitions!/PhaseA1MissingVariant"
    )
    coordinated_variant_registry = copy.deepcopy(registry_entries)
    coordinated_client_key = tool.surface_key(coordinated_client)
    next(
        entry
        for entry in coordinated_variant_registry
        if (
            entry["category"],
            entry["domain"],
            entry["discriminator_field"],
            entry["name"],
        )
        == coordinated_client_key
    )["association_evidence_key"] = coordinated_client[
        "association_evidence_key"
    ]
    if codes(coordinated_rust_variant, coordinated_variant_registry) != [
        "ConflictingAssociationEvidence"
    ]:
        raise AssertionError(
            "coordinated nonexistent Rust variant emitted an inexact "
            "diagnostic multiset"
        )

    def wrong_result(rows: list[dict[str, object]]) -> None:
        rows[0]["result_type_identity"] = "PhaseA1WrongResult"

    if codes(mutate_contracts(wrong_result)) != ["WrongResultType"]:
        raise AssertionError("wrong result identity emitted unrelated diagnostics")

    def wrong_named_result(rows: list[dict[str, object]]) -> None:
        rows[0]["result_schema_type_identity"] = "PhaseA1WrongNamedResult"

    if codes(mutate_contracts(wrong_named_result)) != [
        "ConflictingAssociationEvidence",
        "WrongResultType",
    ]:
        raise AssertionError(
            "wrong named result identity emitted an inexact diagnostic multiset"
        )

    def wrong_result_schema(rows: list[dict[str, object]]) -> None:
        rows[0]["result_schema"] = "stable/v2/PhaseA1WrongResult.json"

    if codes(mutate_contracts(wrong_result_schema)) != ["WrongResultType"]:
        raise AssertionError(
            "wrong result schema root emitted unrelated diagnostics"
        )

    coordinated_missing_schema = mutate_contracts(lambda rows: None)
    coordinated_server = next(
        row
        for row in coordinated_missing_schema["contracts"]
        if row["surface_key"]["category"] == "server_request"
    )
    coordinated_result_schema = (
        "stable/nonexistent/"
        f"{coordinated_server['result_schema_type_identity']}.json"
    )
    coordinated_server["result_schema"] = coordinated_result_schema
    coordinated_server["schema_pair"][
        "response_schema"
    ] = coordinated_result_schema
    coordinated_server["association_evidence_key"] = (
        f"{coordinated_server['schema_pair']['request_union']}"
        f"#/oneOf/{coordinated_server['schema_pair']['branch']}+"
        f"{coordinated_result_schema}"
    )
    coordinated_registry = copy.deepcopy(registry_entries)
    coordinated_key = tool.surface_key(coordinated_server)
    next(
        entry
        for entry in coordinated_registry
        if (
            entry["category"],
            entry["domain"],
            entry["discriminator_field"],
            entry["name"],
        )
        == coordinated_key
    )["association_evidence_key"] = coordinated_server[
        "association_evidence_key"
    ]
    if codes(coordinated_missing_schema, coordinated_registry) != [
        "WrongResultType"
    ]:
        raise AssertionError(
            "coordinated nonexistent schema-pair path emitted an inexact "
            "diagnostic multiset"
        )

    coordinated_nullable = mutate_contracts(lambda rows: None)
    nullable_client = next(
        row
        for row in coordinated_nullable["contracts"]
        if row["surface_key"]["category"] == "client_request"
        and row["result_contract_kind"] == "Concrete"
    )
    nullable_client["result_contract_kind"] = "Nullable"
    nullable_registry = copy.deepcopy(registry_entries)
    nullable_key = tool.surface_key(nullable_client)
    next(
        entry
        for entry in nullable_registry
        if (
            entry["category"],
            entry["domain"],
            entry["discriminator_field"],
            entry["name"],
        )
        == nullable_key
    )["result_contract_kind"] = "Nullable"
    if codes(coordinated_nullable, nullable_registry) != [
        "WrongResultType"
    ]:
        raise AssertionError(
            "schema-inconsistent Nullable contract emitted an inexact "
            "diagnostic multiset"
        )

    def wrong_rust_result(rows: list[dict[str, object]]) -> None:
        rows[0]["rust"]["result_type"] = "v2::PhaseA1WrongResult"

    if codes(mutate_contracts(wrong_rust_result)) != [
        "ConflictingAssociationEvidence",
    ]:
        raise AssertionError(
            "vendored Rust result disagreement emitted unrelated diagnostics"
        )

    def wrong_schema_pair_response(rows: list[dict[str, object]]) -> None:
        server = next(
            row
            for row in rows
            if row["surface_key"]["category"] == "server_request"
        )
        server["schema_pair"]["response_schema"] = (
            "stable/PhaseA1WrongResponse.json"
        )

    if codes(mutate_contracts(wrong_schema_pair_response)) != [
        "ConflictingAssociationEvidence"
    ]:
        raise AssertionError(
            "server schema-pair response disagreement emitted unrelated diagnostics"
        )

    def wrong_schema_pair_branch(rows: list[dict[str, object]]) -> None:
        server = next(
            row
            for row in rows
            if row["surface_key"]["category"] == "server_request"
        )
        server["schema_pair"]["branch"] = 999

    if codes(mutate_contracts(wrong_schema_pair_branch)) != [
        "ConflictingAssociationEvidence"
    ]:
        raise AssertionError(
            "server schema-pair branch disagreement emitted unrelated diagnostics"
        )

    def wrong_rust_cross_check_result(rows: list[dict[str, object]]) -> None:
        server = next(
            row
            for row in rows
            if row["surface_key"]["category"] == "server_request"
        )
        server["cross_check_evidence"][0][
            "result_schema_type_identity"
        ] = "PhaseA1WrongResponse"

    if codes(mutate_contracts(wrong_rust_cross_check_result)) != [
        "ConflictingAssociationEvidence"
    ]:
        raise AssertionError(
            "Rust/schema result cross-check disagreement emitted unrelated diagnostics"
        )

    def conflicting_primary(rows: list[dict[str, object]]) -> None:
        rows[0]["association_evidence_kind"] = "VendoredSchemaPair"

    if codes(mutate_contracts(conflicting_primary)) != [
        "ConflictingAssociationEvidence"
    ]:
        raise AssertionError("conflicting primary evidence emitted unrelated diagnostics")

    def conflicting_cross_check(rows: list[dict[str, object]]) -> None:
        rows[0].setdefault("cross_check_evidence", []).append(
            {
                "kind": "VendoredTypeScriptCrossCheck",
                "key": "negative-test",
                "result_type_identity": "PhaseA1ConflictingResult",
            }
        )

    if codes(mutate_contracts(conflicting_cross_check)) != [
        "ConflictingAssociationEvidence"
    ]:
        raise AssertionError("conflicting cross-check evidence emitted unrelated diagnostics")

    def unit_with_non_unit(rows: list[dict[str, object]]) -> None:
        rows[0]["result_contract_kind"] = "Unit"
        rows[0]["result_type_identity"] = "PhaseA1NotUnit"

    if codes(mutate_contracts(unit_with_non_unit)) != [
        "UnitWithNonUnitResultType"
    ]:
        raise AssertionError("Unit result inconsistency emitted unrelated diagnostics")

    def concrete_without_type(rows: list[dict[str, object]]) -> None:
        rows[0]["result_contract_kind"] = "Concrete"
        rows[0]["result_type_identity"] = ""

    if codes(mutate_contracts(concrete_without_type)) != [
        "ConcreteWithoutResultType"
    ]:
        raise AssertionError("empty Concrete result emitted unrelated diagnostics")

    non_request_key = next(
        {
            "category": entry["category"],
            "domain": entry["domain"],
            "field": entry["discriminator_field"],
            "name": entry["name"],
        }
        for entry in manifest["entries"]
        if entry["stability"] == "stable"
        and entry["category"] == "server_notification"
    )

    def add_non_request(rows: list[dict[str, object]]) -> None:
        non_request = copy.deepcopy(rows[0])
        non_request["surface_key"] = non_request_key
        rows.append(non_request)

    if codes(mutate_contracts(add_non_request)) != ["ContractOnNonRequest"]:
        raise AssertionError("contract on non-request emitted unrelated diagnostics")

    experimental_key = next(
        {
            "category": entry["category"],
            "domain": entry["domain"],
            "field": entry["discriminator_field"],
            "name": entry["name"],
        }
        for entry in manifest["entries"]
        if entry["stability"] == "experimental_only"
        and entry["category"] in {"client_request", "server_request"}
    )

    def add_experimental(rows: list[dict[str, object]]) -> None:
        experimental = copy.deepcopy(rows[0])
        experimental["surface_key"] = experimental_key
        rows.append(experimental)

    if codes(mutate_contracts(add_experimental)) != [
        "ExperimentalAssociationCountedAsStable"
    ]:
        raise AssertionError(
            "experimental association stable-count mutation emitted unrelated diagnostics"
        )


def test_assignment_reachability_guards(
    tool: ModuleType,
    manifest: dict[str, object],
    evidence: dict[str, object],
) -> None:
    assignments_document = evidence["assignments"]
    reachability_document = evidence["reachability"]
    if not isinstance(assignments_document, dict) or not isinstance(
        reachability_document, dict
    ):
        raise AssertionError("assignment/reachability evidence must be objects")

    def codes(
        assignments: dict[str, object] = assignments_document,
        reachability: dict[str, object] = reachability_document,
    ) -> list[str]:
        return sorted(
            diagnostic["code"]
            for diagnostic in tool.assignment_reachability_diagnostics(
                manifest, assignments, reachability
            )
        )

    def assert_codes(
        expected: list[str],
        assignments: dict[str, object] = assignments_document,
        reachability: dict[str, object] = reachability_document,
        description: str = "assignment/reachability mutation",
    ) -> None:
        actual = codes(assignments, reachability)
        if actual != expected:
            raise AssertionError(
                f"{description} emitted {actual}, expected exact diagnostic multiset {expected}"
            )

    assert_codes([], description="unmodified assignment/reachability evidence")

    def mutate_assignments(
        action: Callable[[list[dict[str, object]]], None],
    ) -> dict[str, object]:
        mutated = copy.deepcopy(assignments_document)
        action(mutated["assignments"])
        return mutated

    duplicate_assignment = mutate_assignments(
        lambda rows: rows.append(copy.deepcopy(rows[0]))
    )
    assert_codes(
        ["DuplicateModuleSliceAssignment"],
        assignments=duplicate_assignment,
        description="duplicate module/slice assignment",
    )

    missing_assignment = mutate_assignments(lambda rows: rows.pop(0))
    assert_codes(
        ["MissingModuleSliceAssignment"],
        assignments=missing_assignment,
        description="missing module/slice assignment",
    )

    def add_stale_assignment(rows: list[dict[str, object]]) -> None:
        stale = copy.deepcopy(rows[0])
        stale["surface_key"]["name"] = "phaseA1/stale-assignment"
        rows.append(stale)

    assert_codes(
        ["StaleModuleSliceAssignment"],
        assignments=mutate_assignments(add_stale_assignment),
        description="stale module/slice assignment",
    )

    wrong_stability = mutate_assignments(
        lambda rows: rows[0].__setitem__("stability", "experimental_only")
    )
    assert_codes(
        ["AssignmentStabilityMismatch"],
        assignments=wrong_stability,
        description="assignment stability mismatch",
    )

    wrong_classification = mutate_assignments(
        lambda rows: rows[0].__setitem__(
            "classification", "RootOwnedNestedUnion"
        )
    )
    assert_codes(
        ["InvalidAssignmentClassification"],
        assignments=wrong_classification,
        description="invalid category/classification combination",
    )

    wrong_slice_alias = mutate_assignments(
        lambda rows: rows[0].__setitem__("a1_slice", "A1.4")
    )
    assert_codes(
        ["AssignmentSliceMismatch"],
        assignments=wrong_slice_alias,
        description="assignment slice aliases disagree",
    )

    wrong_module = mutate_assignments(
        lambda rows: rows[0].__setitem__("module", "ArbitraryWrongModule")
    )
    assert_codes(
        ["AssignmentModuleMismatch"],
        assignments=wrong_module,
        description="assignment uses an arbitrary typed module",
    )

    def move_thread_root_to_wrong_domain_slice(
        rows: list[dict[str, object]],
    ) -> None:
        row = next(
            candidate
            for candidate in rows
            if candidate["surface_key"]["category"] == "client_request"
            and candidate["surface_key"]["name"] == "thread/start"
        )
        row["slice"] = "A1.4"
        row["a1_slice"] = "A1.4"
        row["module"] = "IntegrationsAndLongTail"

    assert_codes(
        ["AssignmentDomainSliceMismatch"],
        assignments=mutate_assignments(move_thread_root_to_wrong_domain_slice),
        description="thread root moved to a coherent but wrong domain slice/module",
    )

    def mutate_reachability(
        action: Callable[[dict[str, object]], None],
    ) -> dict[str, object]:
        mutated = copy.deepcopy(reachability_document)
        action(mutated)
        return mutated

    duplicate_root = mutate_reachability(
        lambda document: document["roots"].append(
            copy.deepcopy(document["roots"][0])
        )
    )
    assert_codes(
        ["DuplicateReachabilityRoot"],
        reachability=duplicate_root,
        description="duplicate reachability root",
    )

    missing_root = mutate_reachability(lambda document: document["roots"].pop(0))
    assert_codes(
        ["MissingReachabilityRoot"],
        reachability=missing_root,
        description="missing reachability root",
    )

    def add_stale_root(document: dict[str, object]) -> None:
        stale = copy.deepcopy(document["roots"][0])
        stale["root_id"] = "method:phaseA1:stale-root"
        document["roots"].append(stale)

    assert_codes(
        ["StaleReachabilityRoot"],
        reachability=mutate_reachability(add_stale_root),
        description="stale reachability root",
    )

    wrong_root_slice = mutate_reachability(
        lambda document: document["roots"][0].__setitem__("slice", "A1.4")
    )
    assert_codes(
        ["ReachabilityRootSetMismatch"],
        reachability=wrong_root_slice,
        description="root catalog slice mismatch",
    )

    duplicate_record = mutate_reachability(
        lambda document: document["records"].append(
            copy.deepcopy(document["records"][0])
        )
    )
    assert_codes(
        ["DuplicateReachabilityRecord"],
        reachability=duplicate_record,
        description="duplicate nested reachability record",
    )

    missing_record = mutate_reachability(
        lambda document: document["records"].pop(0)
    )
    assert_codes(
        ["MissingReachabilityRecord"],
        reachability=missing_record,
        description="missing nested reachability record",
    )

    def add_stale_record(document: dict[str, object]) -> None:
        stale = copy.deepcopy(document["records"][0])
        stale["surface_key"]["name"] = "phaseA1/stale-reachability"
        document["records"].append(stale)

    assert_codes(
        ["StaleReachabilityRecord"],
        reachability=mutate_reachability(add_stale_record),
        description="stale nested reachability record",
    )

    wrong_assigned_slice = mutate_reachability(
        lambda document: document["records"][0].__setitem__(
            "assigned_slice", "A1.4"
        )
    )
    assert_codes(
        ["ReachabilityAssignedSliceMismatch"],
        reachability=wrong_assigned_slice,
        description="reachability/assignment slice mismatch",
    )

    root_owned_index = next(
        index
        for index, record in enumerate(reachability_document["records"])
        if record["classification"] == "RootOwnedNestedUnion"
    )

    wrong_reachability_classification = mutate_reachability(
        lambda document: document["records"][root_owned_index].__setitem__(
            "classification", "SharedWithinSlice"
        )
    )
    assert_codes(
        ["ReachabilityClassificationMismatch"],
        reachability=wrong_reachability_classification,
        description="assignment/reachability classification mismatch",
    )

    wrong_root_count = mutate_reachability(
        lambda document: document["records"][root_owned_index].__setitem__(
            "reaching_root_count",
            document["records"][root_owned_index]["reaching_root_count"] + 1,
        )
    )
    assert_codes(
        ["ReachabilityRootSetMismatch"],
        reachability=wrong_root_count,
        description="reaching root/count mismatch",
    )

    def coordinated_root_deletion(document: dict[str, object]) -> None:
        record = next(
            candidate
            for candidate in document["records"]
            if candidate["classification"] == "SharedCommon"
            and any(
                sum(
                    root["slice"] == selected["slice"]
                    for root in candidate["reaching_roots"]
                )
                > 1
                for selected in candidate["reaching_roots"]
            )
        )
        removable_index = next(
            index
            for index, selected in enumerate(record["reaching_roots"])
            if sum(
                root["slice"] == selected["slice"]
                for root in record["reaching_roots"]
            )
            > 1
        )
        record["reaching_roots"].pop(removable_index)
        record["reaching_root_count"] = len(record["reaching_roots"])
        record["reaching_slices"] = sorted(
            {root["slice"] for root in record["reaching_roots"]},
            key=lambda value: tool.A1_SLICE_ORDER[value],
        )

    assert_codes(
        ["ReachabilityRootSetMismatch"],
        reachability=mutate_reachability(coordinated_root_deletion),
        description="coordinated deletion from a schema-derived reaching-root set",
    )

    later_assignments = copy.deepcopy(assignments_document)
    later_reachability = copy.deepcopy(reachability_document)
    later_record = next(
        record
        for record in later_reachability["records"]
        if record["classification"] == "RootOwnedNestedUnion"
        and record["assigned_slice"] == "A1.2"
    )
    later_key = tool.surface_key(later_record)
    later_assignment = next(
        assignment
        for assignment in later_assignments["assignments"]
        if tool.surface_key(assignment) == later_key
    )
    later_assignment["slice"] = "A1.3"
    later_assignment["a1_slice"] = "A1.3"
    later_assignment["module"] = "CommandsFilesystemReviewsApprovals"
    later_record["assigned_slice"] = "A1.3"
    assert_codes(
        ["ReachabilityNotEarliestSlice"],
        assignments=later_assignments,
        reachability=later_reachability,
        description="nested identity assigned later than earliest reaching root",
    )

    earlier_assignments = copy.deepcopy(assignments_document)
    earlier_reachability = copy.deepcopy(reachability_document)
    earlier_record = next(
        record
        for record in earlier_reachability["records"]
        if record["classification"] == "RootOwnedNestedUnion"
        and record["assigned_slice"] == "A1.2"
    )
    earlier_key = tool.surface_key(earlier_record)
    earlier_assignment = next(
        assignment
        for assignment in earlier_assignments["assignments"]
        if tool.surface_key(assignment) == earlier_key
    )
    earlier_assignment["slice"] = "A1.1"
    earlier_assignment["a1_slice"] = "A1.1"
    earlier_assignment["module"] = "ThreadsTurnsSessions"
    earlier_record["assigned_slice"] = "A1.1"
    assert_codes(
        ["ReachabilityNotEarliestSlice"],
        assignments=earlier_assignments,
        reachability=earlier_reachability,
        description="nested identity assigned earlier than every reaching root",
    )

    def make_false_unreachable(document: dict[str, object]) -> None:
        record = next(
            candidate
            for candidate in document["records"]
            if candidate["classification"] == "StableUnreachableInventory"
        )
        root = copy.deepcopy(document["roots"][0])
        record["reaching_roots"] = [root]
        record["reaching_root_count"] = 1
        record["reaching_slices"] = [root["slice"]]

    assert_codes(
        ["FalseStableUnreachable"],
        reachability=mutate_reachability(make_false_unreachable),
        description="reachable identity falsely classified stable-unreachable",
    )


def test_evidence_authority_boundaries(
    tool: ModuleType,
    manifest: dict[str, object],
    evidence: dict[str, object],
) -> None:
    contract_disposition = copy.deepcopy(evidence["operation_contracts"])
    contract_disposition["contracts"][0]["runtime_disposition"] = "Typed"
    expect_surface_error(
        tool,
        lambda: tool.association_diagnostics(manifest, contract_disposition),
        "operation evidence attempts to carry a local runtime disposition",
    )

    nested_contract_disposition = copy.deepcopy(
        evidence["operation_contracts"]
    )
    server_contract = next(
        contract
        for contract in nested_contract_disposition["contracts"]
        if contract["surface_key"]["category"] == "server_request"
    )
    server_contract["schema_pair"]["backend_core"] = "Implemented"
    expect_surface_error(
        tool,
        lambda: tool.association_diagnostics(
            manifest, nested_contract_disposition
        ),
        "nested operation evidence attempts to carry a BackendCore disposition",
    )

    assignment_disposition = copy.deepcopy(evidence)
    assignment_disposition["assignments"]["assignments"][0][
        "frontend_security"
    ] = "NotApplicable"
    expect_surface_error(
        tool,
        lambda: tool.generate_registry_data(manifest, assignment_disposition),
        "module/slice evidence attempts to carry a frontend-security disposition",
    )

    assignment_completeness = copy.deepcopy(evidence)
    assignment_completeness["assignments"]["assignments"][0][
        "completeness_evidence"
    ] = {"runtime_decoder_matches_registry": True}
    expect_surface_error(
        tool,
        lambda: tool.generate_registry_data(manifest, assignment_completeness),
        "module/slice evidence attempts to improve schema completeness",
    )

    fixture_completeness = copy.deepcopy(evidence)
    fixture_completeness["fixture_coverage"]["fixtures"][0][
        "completeness_evidence"
    ]["runtime_decoder_matches_registry"] = True
    expect_surface_error(
        tool,
        lambda: tool.generate_registry_data(manifest, fixture_completeness),
        "schema fixture evidence attempts to carry a runtime-decoder disposition",
    )


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
            arguments.schema_root,
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
