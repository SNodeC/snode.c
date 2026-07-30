#!/usr/bin/env python3
"""Isolated planted failures for the permanent AISuite ownership boundary."""

from __future__ import annotations

import argparse
import copy
import importlib.util
import json
import sys
import tempfile
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


def verify_target_inventory_parsers(tool: Any) -> None:
    ninja = """\
[1/1] All primary targets available:
alpha: phony
clean: CLEAN
install/local: phony
all: phony
libalpha.so: phony
/tmp/build/CMakeFiles/alpha: phony
build.ninja: RERUN_CMAKE
help: HELP
"""
    make = """\
The following are some of the valid targets for this Makefile:
... all (the default if no target is provided)
... clean
... depend
... alpha
... install/local
"""
    expected = ["alpha", "clean", "install/local"]
    for generator, source in [("Ninja", ninja), ("Unix Makefiles", make)]:
        observed = tool.extract_target_names_text(source)
        if observed != expected:
            raise RuntimeError(
                f"{generator} target normalization changed: {observed}"
            )

        marker = (
            "ai-openai-codex: phony\n"
            if generator == "Ninja"
            else "... ai-openai-codex\n"
        )
        mutated = tool.extract_target_names_text(source + marker)
        if mutated == observed:
            raise RuntimeError(
                f"{generator} forbidden-target mutation did not change input"
            )
        forbidden = sorted(
            set(mutated)
            & set(
                tool.REMOVED_COMPONENTS
                + tool.REMOVED_APPLICATIONS
                + tool.REMOVED_PRIVATE_APP_TARGETS
                + tool.REMOVED_TARGETS
            )
        )
        if forbidden != ["ai-openai-codex"]:
            raise RuntimeError(
                f"{generator} forbidden target was not detected exactly: "
                f"{forbidden}"
            )


def verify_nested_build_path_normalization(tool: Any) -> None:
    with tempfile.TemporaryDirectory(
        prefix="snodec-cutover-path-normalization-"
    ) as temporary:
        source_root = Path(temporary) / "source"
        build_root = source_root / "build"
        source_path = source_root / "tests/SourceTest.cpp"
        build_path = build_root / "tests/SourceTest"
        observed_source = tool.normalize_path_text(
            str(source_path),
            source_root=source_root,
            build_root=build_root,
        )
        observed_build = tool.normalize_path_text(
            str(build_path),
            source_root=source_root,
            build_root=build_root,
        )
    if observed_source != "${SNODEC_SOURCE}/tests/SourceTest.cpp":
        raise RuntimeError(
            "source-root path normalization changed: "
            f"{observed_source}"
        )
    if observed_build != "${SNODEC_BUILD}/tests/SourceTest":
        raise RuntimeError(
            "nested build-root path normalization changed: "
            f"{observed_build}"
        )


def main() -> int:
    args = parse_arguments()
    tool = load_tool(args.tool.resolve())
    authority = json.loads(args.evidence.read_text(encoding="utf-8"))
    if tool.boundary_model_diagnostics(authority):
        raise RuntimeError("unmodified ownership boundary does not pass")
    verify_target_inventory_parsers(tool)
    verify_nested_build_path_normalization(tool)

    Mutation = tuple[
        str, str, Callable[[dict[str, Any]], None]
    ]
    mutations: list[Mutation] = []

    def register(
        name: str,
        code: str,
        mutate: Callable[[dict[str, Any]], None],
    ) -> None:
        mutations.append((name, code, mutate))

    def plant_build_line(model: dict[str, Any], line: str) -> None:
        with tempfile.TemporaryDirectory(
            prefix="snodec-cutover-build-mutation-"
        ) as temporary:
            root = Path(temporary)
            source_path = root / "src/CMakeLists.txt"
            source_path.parent.mkdir(parents=True)
            source_path.write_text(f"{line}\n", encoding="utf-8")
            observed = tool.source_build_surface(root)["wiring_hits"]
        if not observed:
            raise RuntimeError(
                f"build mutation was not discovered: {line}"
            )
        model["build"]["wiring_hits"] = observed

    register(
        "restore-add-subdirectory-ai",
        tool.BUILD_DIAGNOSTIC,
        lambda model: plant_build_line(model, "add_subdirectory(ai)"),
    )
    register(
        "restore-ai-dependency-collection",
        tool.BUILD_DIAGNOSTIC,
        lambda model: plant_build_line(
            model,
            "get_all_targets_dependencies("
            "SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES ai)",
        ),
    )

    def restore_supported_component(model: dict[str, Any]) -> None:
        component = tool.REMOVED_COMPONENTS[0]
        model["build"]["supported_components"].append(component)
        model["build"]["removed_supported_components"] = [component]
        model["preservation"]["supported_components_match"] = False

    register(
        "restore-supported-component",
        tool.BUILD_DIAGNOSTIC,
        restore_supported_component,
    )

    def restore_cpack_component(model: dict[str, Any]) -> None:
        component = tool.REMOVED_COMPONENTS[0]
        model["package"]["cpack_checked"] = True
        model["package"]["cpack_components"] = [
            *model["preservation"]["expected_cpack_components"],
            component,
        ]
        model["preservation"]["cpack_components_match"] = False

    register(
        "restore-cpack-component",
        tool.PACKAGE_DIAGNOSTIC,
        restore_cpack_component,
    )

    def restore_apps_dependency(model: dict[str, Any]) -> None:
        model["package"]["cpack_checked"] = True
        model["package"]["apps_dependencies"] = [
            *model["preservation"]["expected_apps_dependencies"],
            "ai-openai-codex-frontend",
        ]
        model["preservation"]["apps_dependencies_match"] = False

    register(
        "restore-apps-frontend-dependency",
        tool.PACKAGE_DIAGNOSTIC,
        restore_apps_dependency,
    )

    def restore_path(model: dict[str, Any]) -> None:
        with tempfile.TemporaryDirectory(
            prefix="snodec-cutover-path-mutation-"
        ) as temporary:
            root = Path(temporary)
            if tool.remaining_removal_paths(root):
                raise RuntimeError("empty mutation tree is not valid")
            planted = root / "src/ai"
            planted.mkdir(parents=True)
            observed = tool.remaining_removal_paths(root)
        if observed != ["src/ai/"]:
            raise RuntimeError(
                f"path mutation was not discovered exactly: {observed}"
            )
        model["paths"]["remaining"] = observed

    register(
        "restore-dedicated-source-directory",
        tool.PATH_DIAGNOSTIC,
        restore_path,
    )
    register(
        "restore-component-test-registration",
        tool.TEST_DIAGNOSTIC,
        lambda model: model["tests"]["forbidden"].append(
            "registration:tests/component/codex"
        ),
    )
    register(
        "restore-codex-test-label",
        tool.TEST_DIAGNOSTIC,
        lambda model: model["tests"]["forbidden"].append(
            "label:codex"
        ),
    )
    register(
        "restore-installed-public-header",
        tool.INSTALL_DIAGNOSTIC,
        lambda model: model["install"]["forbidden_paths"].append(
            "include/snode.c/ai/openai/codex/Protocol.h"
        ),
    )
    register(
        "restore-installed-library-application-or-target",
        tool.INSTALL_DIAGNOSTIC,
        lambda model: model["install"]["forbidden_paths"].append(
            "lib/libsnodec-ai-openai-codex.so.1"
        ),
    )
    register(
        "restore-source-package-path",
        tool.PACKAGE_DIAGNOSTIC,
        lambda model: model["package"][
            "source_forbidden_paths"
        ].append("tools/codex/app_server_surface.py"),
    )

    def restore_relocated_source(model: dict[str, Any]) -> None:
        path = "src/vendor/OpenAICodexProvider.cpp"
        if not tool.path_has_ownership_residue(path):
            raise RuntimeError(
                "relocated ownership path was not classified"
            )
        model["package"]["source_unexpected_paths"].append(path)
        model["package"]["source_ownership_residue"].append(path)

    register(
        "restore-relocated-codex-source",
        tool.PACKAGE_DIAGNOSTIC,
        restore_relocated_source,
    )
    register(
        "restore-logging-api-branch",
        tool.POLICY_DIAGNOSTIC,
        lambda model: model["policy"]["residue"].append(
            "logging-api-path:src/ai/openai/codex/backend/BackendEvent.h"
        ),
    )
    register(
        "restore-semantic-scan-root",
        tool.POLICY_DIAGNOSTIC,
        lambda model: model["policy"]["residue"].append(
            "semantic-scan-root"
        ),
    )
    register(
        "restore-semantic-allowlist-entry",
        tool.POLICY_DIAGNOSTIC,
        lambda model: model["policy"]["residue"].append(
            "semantic-allowlist:turn started"
        ),
    )

    def remove_supported_component(model: dict[str, Any]) -> None:
        model["build"]["supported_components"].pop()
        model["preservation"]["supported_components_match"] = False

    register(
        "remove-non-codex-supported-component",
        tool.NON_CODEX_DIAGNOSTIC,
        remove_supported_component,
    )

    def alter_cpack_component(model: dict[str, Any]) -> None:
        model["package"]["cpack_checked"] = True
        model["package"]["cpack_components"] = model["preservation"][
            "expected_cpack_components"
        ][:-1]
        model["preservation"]["cpack_components_match"] = False

    register(
        "alter-non-codex-cpack-component",
        tool.NON_CODEX_DIAGNOSTIC,
        alter_cpack_component,
    )

    def remove_surviving_ctest(model: dict[str, Any]) -> None:
        model["tests"]["checked"] = True
        model["tests"]["missing_survivors"] = [
            "StagedInstalledConsumerTest"
        ]
        model["tests"]["survivors_match"] = False
        model["preservation"]["ctest_survivors_match"] = False

    register(
        "remove-non-codex-baseline-ctest",
        tool.NON_CODEX_DIAGNOSTIC,
        remove_surviving_ctest,
    )

    def change_project_version(model: dict[str, Any]) -> None:
        model["identity"]["project_version"] = "1.0.2"

    register(
        "change-project-version",
        tool.SOVERSION_DIAGNOSTIC,
        change_project_version,
    )
    register(
        "change-soversion",
        tool.SOVERSION_DIAGNOSTIC,
        lambda model: model["identity"].__setitem__("soversion", "2"),
    )

    def add_aisuite_dependency(model: dict[str, Any]) -> None:
        with tempfile.TemporaryDirectory(
            prefix="snodec-cutover-dependency-mutation-"
        ) as temporary:
            root = Path(temporary)
            (root / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.25)\n",
                encoding="utf-8",
            )
            tests_cmake = root / "tests/CMakeLists.txt"
            tests_cmake.parent.mkdir(parents=True)
            tests_cmake.write_text(
                "find_package(AISuite CONFIG REQUIRED)\n",
                encoding="utf-8",
            )
            observed = tool.production_dependency_hits(root)
        if observed != ["tests/CMakeLists.txt"]:
            raise RuntimeError(
                "AISuite test-build dependency was not discovered exactly: "
                f"{observed}"
            )
        model["dependency_direction"][
            "snodec_depends_on_aisuite"
        ] = True
        model["dependency_direction"]["production_dependency_hits"] = observed

    register(
        "add-aisuite-source-build-dependency",
        tool.DEPENDENCY_DIAGNOSTIC,
        add_aisuite_dependency,
    )

    def restore_legacy_consumer_resolution(
        model: dict[str, Any],
    ) -> None:
        model["aisuite_consumer"][
            "legacy_or_source_build_resolution"
        ] = ["snodec::ai-openai-codex"]

    register(
        "restore-old-or-source-build-consumer-resolution",
        tool.AISUITE_CONSUMER_DIAGNOSTIC,
        restore_legacy_consumer_resolution,
    )

    register(
        "change-aisuite-owner-commit",
        tool.MIGRATION_DIAGNOSTIC,
        lambda model: model["migration"].__setitem__(
            "aisuite_owner_commit", "0" * 40
        ),
    )

    def remove_migration_requirement(model: dict[str, Any]) -> None:
        requirement = "migration:no-shim"
        model["migration"]["requirements"][requirement] = False
        model["migration"]["complete"] = False

    register(
        "remove-migration-requirement",
        tool.MIGRATION_DIAGNOSTIC,
        remove_migration_requirement,
    )

    for mutation_name, expected_code, mutate in mutations:
        planted = copy.deepcopy(authority)
        before = tool.canonical_json_bytes(planted)
        mutate(planted)
        after = tool.canonical_json_bytes(planted)
        if before == after:
            raise RuntimeError(
                f"{mutation_name}: mutation did not change its input"
            )
        diagnostics = tool.boundary_model_diagnostics(planted)
        if diagnostics != [expected_code]:
            raise RuntimeError(
                f"{mutation_name}: expected only {expected_code}, "
                f"observed {diagnostics}"
            )
        if tool.boundary_model_diagnostics(authority):
            raise RuntimeError(
                f"{mutation_name}: unmodified authority stopped passing"
            )

    future = copy.deepcopy(authority)
    future["build"]["supported_components"].append(
        "future-non-codex-component"
    )
    future["preservation"]["supported_components_match"] = (
        tool.preserves_ordered_inventory(
            future["preservation"]["expected_supported_components"],
            future["build"]["supported_components"],
        )
    )
    future["package"]["cpack_checked"] = True
    future["package"]["cpack_components"] = [
        *future["preservation"]["expected_cpack_components"],
        "future-non-codex-component",
    ]
    future["package"]["apps_dependencies"] = [
        *future["preservation"]["expected_apps_dependencies"],
        "future-non-codex-component",
    ]
    future["package"]["component_dependencies"] = {
        **future["preservation"]["expected_component_dependencies"],
        "future-non-codex-component": ["core"],
    }
    future["preservation"]["cpack_components_match"] = True
    future["preservation"]["apps_dependencies_match"] = True
    future["preservation"]["component_dependencies_match"] = True
    future["package"]["source_unexpected_paths"] = [
        "src/future-non-codex-component/CMakeLists.txt"
    ]
    future["install"]["unexpected_paths"] = [
        "include/snode.c/future/Feature.h"
    ]
    future["package"]["binary_unexpected_paths"] = [
        "include/snode.c/future/Feature.h"
    ]
    future["tests"]["configured_tests"] = (
        future["tests"].get("configured_tests", 172) + 1
    )
    if tool.boundary_model_diagnostics(future):
        raise RuntimeError(
            "reviewed future non-Codex additions invalidate the "
            "permanent cutover boundary"
        )

    print(
        "AISuite ownership-boundary mutations passed: "
        f"{len(mutations)} isolated diagnostics; "
        "future-non-Codex-additions=accepted"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
