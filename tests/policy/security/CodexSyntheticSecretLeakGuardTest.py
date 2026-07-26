#!/usr/bin/env python3

# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#
# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT

"""Fail closed when reviewed Codex synthetic secrets escape exact fixtures."""

from __future__ import annotations

import argparse
import hashlib
import os
import stat
import sys
import tempfile
from collections import Counter
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from typing import Iterable

sys.dont_write_bytecode = True


SYNTHETIC_SENTINELS = (
    b"".join((b"test-api-key-", b"not-real")),
    b"".join((b"test-access-token-", b"not-real")),
)
MAX_SENTINEL_SIZE = max(map(len, SYNTHETIC_SENTINELS))
DEFAULT_CHUNK_SIZE = 4 * 1024 * 1024

FIXTURE_PREFIX = PurePosixPath(
    "tools/codex/app-server-fixtures/0.144.6"
)
ACCOUNT_LOGIN_START_FIXTURES = (
    FIXTURE_PREFIX / "cases/operations/client/account-login-start"
)
AUTH_REFRESH_FIXTURES = (
    FIXTURE_PREFIX
    / "cases/operations/server/account-chatgptauthtokens-refresh"
)
LOGIN_ACCOUNT_PARAMS_FIXTURES = (
    FIXTURE_PREFIX / "cases/unions/loginaccountparams"
)
EXACT_FIXTURE_ALLOWLIST = (
    ACCOUNT_LOGIN_START_FIXTURES
    / "mutations/params-missing-required-type.json",
    ACCOUNT_LOGIN_START_FIXTURES
    / "mutations/params-wrong-type-type.json",
    ACCOUNT_LOGIN_START_FIXTURES / "params.json",
    AUTH_REFRESH_FIXTURES
    / "mutations/result-missing-required-chatgptaccountid.json",
    AUTH_REFRESH_FIXTURES
    / "mutations/result-wrong-type-chatgptaccountid.json",
    AUTH_REFRESH_FIXTURES
    / "mutations/result-wrong-type-chatgptplantype.json",
    AUTH_REFRESH_FIXTURES / "result.json",
    AUTH_REFRESH_FIXTURES
    / "supplements/result-nullable-null-chatgptplantype.json",
    AUTH_REFRESH_FIXTURES
    / "supplements/result-optional-omitted-chatgptplantype.json",
    LOGIN_ACCOUNT_PARAMS_FIXTURES / "apikey.json",
    LOGIN_ACCOUNT_PARAMS_FIXTURES / "chatgptauthtokens.json",
    LOGIN_ACCOUNT_PARAMS_FIXTURES / "future-unknown.json",
    LOGIN_ACCOUNT_PARAMS_FIXTURES
    / "mutations/apikey-missing-discriminator.json",
    LOGIN_ACCOUNT_PARAMS_FIXTURES
    / "mutations/apikey-wrong-discriminator-type.json",
    LOGIN_ACCOUNT_PARAMS_FIXTURES
    / "mutations/chatgptauthtokens-missing-discriminator.json",
    LOGIN_ACCOUNT_PARAMS_FIXTURES
    / "mutations/chatgptauthtokens-missing-required-chatgptaccountid.json",
    LOGIN_ACCOUNT_PARAMS_FIXTURES
    / "mutations/chatgptauthtokens-wrong-discriminator-type.json",
    LOGIN_ACCOUNT_PARAMS_FIXTURES
    / "mutations/chatgptauthtokens-wrong-nested-type-chatgptaccountid.json",
    LOGIN_ACCOUNT_PARAMS_FIXTURES
    / "mutations/chatgptauthtokens-wrong-nested-type-chatgptplantype.json",
    LOGIN_ACCOUNT_PARAMS_FIXTURES
    / "supplements/chatgptauthtokens-nullable-null-chatgptplantype.json",
    LOGIN_ACCOUNT_PARAMS_FIXTURES
    / "supplements/chatgptauthtokens-optional-omitted-chatgptplantype.json",
)
EXACT_FIXTURE_ALLOWLIST_SET = frozenset(EXACT_FIXTURE_ALLOWLIST)

LEAK_CODE = "SyntheticSecretLeak"
ALLOWLIST_DRIFT_CODE = "SyntheticSecretAllowlistDrift"
SCAN_FAILURE_CODE = "SyntheticSecretScanFailure"


@dataclass(frozen=True, order=True)
class Finding:
    code: str
    display_path: str
    byte_record: int

    def render(self) -> str:
        return (
            f"{self.code}|{safe_text(self.display_path)}|"
            f"byte-record={self.byte_record}"
        )


@dataclass(frozen=True)
class ScanResult:
    offsets: tuple[int, ...]
    digest: str | None


def safe_text(value: str) -> str:
    for sentinel in SYNTHETIC_SENTINELS:
        value = value.replace(sentinel.decode("ascii"), "[redacted]")
    return "".join(
        character
        if character.isprintable() and character not in {"|", "\r", "\n"}
        else "_"
        for character in value
    )


def scan_stream(
    path: Path,
    *,
    include_digest: bool,
    chunk_size: int = DEFAULT_CHUNK_SIZE,
) -> ScanResult:
    if chunk_size <= 0:
        raise ValueError("chunk size must be positive")

    digest = hashlib.sha256() if include_digest else None
    offsets: list[int] = []
    carry = b""
    consumed = 0

    with path.open("rb") as stream:
        while True:
            chunk = stream.read(chunk_size)
            if not chunk:
                break
            if digest is not None:
                digest.update(chunk)

            window = carry + chunk
            window_start = consumed - len(carry)
            for sentinel in SYNTHETIC_SENTINELS:
                search_start = 0
                while True:
                    match = window.find(sentinel, search_start)
                    if match < 0:
                        break
                    offsets.append(window_start + match + 1)
                    search_start = match + 1

            carry = window[-(MAX_SENTINEL_SIZE - 1) :]
            consumed += len(chunk)

    return ScanResult(
        offsets=tuple(sorted(set(offsets))),
        digest=digest.hexdigest() if digest is not None else None,
    )


def logical_path_endswith(
    path: PurePosixPath, suffix: PurePosixPath
) -> bool:
    return (
        len(path.parts) >= len(suffix.parts)
        and path.parts[-len(suffix.parts) :] == suffix.parts
    )


def logical_path_startswith(
    path: PurePosixPath, prefix: PurePosixPath
) -> bool:
    return (
        len(path.parts) >= len(prefix.parts)
        and path.parts[: len(prefix.parts)] == prefix.parts
    )


def source_allowlist_path(
    path: Path, repo_root: Path
) -> PurePosixPath | None:
    try:
        relative = path.relative_to(repo_root)
    except ValueError:
        return None
    logical = PurePosixPath(relative.as_posix())
    return logical if logical in EXACT_FIXTURE_ALLOWLIST_SET else None


def packaged_allowlist_path(
    path: Path, build_root: Path
) -> PurePosixPath | None:
    try:
        relative = PurePosixPath(path.relative_to(build_root).as_posix())
    except ValueError:
        return None

    in_cpack_stage = (
        bool(relative.parts)
        and relative.parts[0] == "_CPack_Packages"
    )
    in_extracted_source_audit = (
        len(relative.parts) >= 2
        and relative.parts[0] == "codex-source-package-audit"
        and relative.parts[1] == "extracted"
    )
    if not in_cpack_stage and not in_extracted_source_audit:
        return None

    matches = tuple(
        allowed
        for allowed in EXACT_FIXTURE_ALLOWLIST
        if logical_path_endswith(relative, allowed)
    )
    return matches[0] if len(matches) == 1 else None


class SecretLeakGuard:
    def __init__(
        self, repo_root: Path, build_roots: Iterable[Path]
    ) -> None:
        self.repo_root = repo_root.resolve()
        self.build_roots = tuple(
            dict.fromkeys(root.resolve() for root in build_roots)
        )
        self.findings: list[Finding] = []
        self.source_digests: dict[PurePosixPath, str] = {}
        self.scan_cache: dict[
            tuple[int, int, int, int], ScanResult
        ] = {}
        self.encountered_files = 0
        self.unique_file_scans = 0
        self.accepted_packaged_copies = 0

    def add_finding(
        self, code: str, display_path: str, byte_record: int = 0
    ) -> None:
        self.findings.append(Finding(code, display_path, byte_record))

    def scan_file(
        self,
        path: Path,
        display_path: str,
        scan_root: Path,
        *,
        include_digest: bool,
    ) -> ScanResult | None:
        self.encountered_files += 1
        try:
            link_status = path.lstat()
            if stat.S_ISLNK(link_status.st_mode):
                resolved = path.resolve(strict=True)
                if not resolved.is_relative_to(scan_root.resolve()):
                    self.add_finding(
                        SCAN_FAILURE_CODE, display_path
                    )
                    return None

            before = path.stat()
            if not stat.S_ISREG(before.st_mode):
                self.add_finding(SCAN_FAILURE_CODE, display_path)
                return None

            cache_key = (
                before.st_dev,
                before.st_ino,
                before.st_size,
                before.st_mtime_ns,
            )
            cached = self.scan_cache.get(cache_key)
            if cached is not None and (
                not include_digest or cached.digest is not None
            ):
                return cached

            result = scan_stream(
                path, include_digest=include_digest
            )
            after = path.stat()
            after_key = (
                after.st_dev,
                after.st_ino,
                after.st_size,
                after.st_mtime_ns,
            )
            if after_key != cache_key:
                self.add_finding(SCAN_FAILURE_CODE, display_path)
                return None

            self.unique_file_scans += 1
            self.scan_cache[cache_key] = result
            return result
        except (OSError, RuntimeError, ValueError):
            self.add_finding(SCAN_FAILURE_CODE, display_path)
            return None

    def validate_source_allowlist(self) -> None:
        if (
            len(EXACT_FIXTURE_ALLOWLIST) != 21
            or len(EXACT_FIXTURE_ALLOWLIST_SET) != 21
            or any(
                not logical_path_startswith(allowed, FIXTURE_PREFIX)
                for allowed in EXACT_FIXTURE_ALLOWLIST
            )
        ):
            self.add_finding(
                ALLOWLIST_DRIFT_CODE,
                "source/tools/codex/app-server-fixtures/0.144.6",
            )
            return

        fixture_root = self.repo_root / Path(FIXTURE_PREFIX.as_posix())
        for logical in EXACT_FIXTURE_ALLOWLIST:
            path = self.repo_root / Path(logical.as_posix())
            display_path = f"source/{logical.as_posix()}"
            try:
                source_status = path.lstat()
            except OSError:
                self.add_finding(
                    ALLOWLIST_DRIFT_CODE, display_path
                )
                continue
            if not stat.S_ISREG(source_status.st_mode):
                self.add_finding(
                    ALLOWLIST_DRIFT_CODE, display_path
                )
                continue

            result = self.scan_file(
                path,
                display_path,
                fixture_root,
                include_digest=True,
            )
            if result is None:
                continue
            if not result.offsets or result.digest is None:
                self.add_finding(
                    ALLOWLIST_DRIFT_CODE, display_path
                )
                continue
            self.source_digests[logical] = result.digest

    def record_offsets(
        self, display_path: str, result: ScanResult
    ) -> None:
        for offset in result.offsets:
            self.add_finding(LEAK_CODE, display_path, offset)

    def scan_tree(
        self,
        root: Path,
        display_prefix: str,
        *,
        build_root: Path | None,
    ) -> None:
        try:
            root = root.resolve(strict=True)
        except OSError:
            self.add_finding(SCAN_FAILURE_CODE, display_prefix)
            return
        if not root.is_dir():
            self.add_finding(SCAN_FAILURE_CODE, display_prefix)
            return

        def fail_walk(error: OSError) -> None:
            raise error

        try:
            for directory, directory_names, file_names in os.walk(
                root,
                topdown=True,
                followlinks=False,
                onerror=fail_walk,
            ):
                directory_names.sort()
                file_names.sort()
                directory_path = Path(directory)

                for directory_name in tuple(directory_names):
                    candidate = directory_path / directory_name
                    if candidate.is_symlink():
                        directory_names.remove(directory_name)
                        relative = candidate.relative_to(root).as_posix()
                        self.add_finding(
                            SCAN_FAILURE_CODE,
                            f"{display_prefix}/{relative}",
                        )

                for file_name in file_names:
                    path = directory_path / file_name
                    relative = path.relative_to(root).as_posix()
                    display_path = f"{display_prefix}/{relative}"

                    allowed = (
                        source_allowlist_path(path, self.repo_root)
                        if build_root is None
                        else packaged_allowlist_path(path, build_root)
                    )
                    try:
                        if (
                            allowed is not None
                            and stat.S_ISLNK(path.lstat().st_mode)
                        ):
                            self.add_finding(
                                ALLOWLIST_DRIFT_CODE, display_path
                            )
                            allowed = None
                    except OSError:
                        pass
                    result = self.scan_file(
                        path,
                        display_path,
                        root,
                        include_digest=allowed is not None,
                    )
                    if result is None:
                        continue
                    if allowed is None:
                        self.record_offsets(display_path, result)
                        continue

                    expected_digest = self.source_digests.get(allowed)
                    if build_root is None:
                        if expected_digest is None:
                            self.add_finding(
                                ALLOWLIST_DRIFT_CODE, display_path
                            )
                        continue

                    if (
                        expected_digest is None
                        or result.digest != expected_digest
                    ):
                        self.add_finding(
                            ALLOWLIST_DRIFT_CODE, display_path
                        )
                        self.record_offsets(display_path, result)
                        continue
                    self.accepted_packaged_copies += 1
        except OSError:
            self.add_finding(SCAN_FAILURE_CODE, display_prefix)

    def run(self) -> tuple[Finding, ...]:
        self.validate_source_allowlist()
        self.scan_tree(
            self.repo_root / Path(FIXTURE_PREFIX.as_posix()),
            "source/tools/codex/app-server-fixtures/0.144.6",
            build_root=None,
        )
        self.scan_tree(
            self.repo_root
            / "tools/codex/app-server-evidence/0.144.6",
            "source/tools/codex/app-server-evidence/0.144.6",
            build_root=None,
        )
        self.scan_tree(
            self.repo_root / "docs",
            "source/docs",
            build_root=None,
        )
        for index, build_root in enumerate(self.build_roots, start=1):
            self.scan_tree(
                build_root,
                f"build[{index}]",
                build_root=build_root,
            )
        return tuple(sorted(set(self.findings)))


def run_negative_self_test() -> None:
    with tempfile.TemporaryDirectory(
        prefix="codex-synthetic-secret-guard-"
    ) as temporary:
        root = Path(temporary)
        planted = (
            root
            / "tools/codex/app-server-fixtures/0.144.6"
            / "cases/unexpected.json"
        )
        planted.parent.mkdir(parents=True)
        prefix = b"x" * (MAX_SENTINEL_SIZE - 3)
        planted.write_bytes(prefix + SYNTHETIC_SENTINELS[0] + b"\n")

        if source_allowlist_path(planted, root) is not None:
            raise RuntimeError
        result = scan_stream(
            planted, include_digest=False, chunk_size=11
        )
        findings = tuple(
            Finding(
                LEAK_CODE,
                "source/tools/codex/app-server-fixtures/0.144.6/cases/unexpected.json",
                offset,
            )
            for offset in result.offsets
        )
        if Counter(finding.code for finding in findings) != Counter(
            {LEAK_CODE: 1}
        ):
            raise RuntimeError

        rendered = "\n".join(
            finding.render() for finding in findings
        )
        if any(
            sentinel.decode("ascii") in rendered
            for sentinel in SYNTHETIC_SENTINELS
        ):
            raise RuntimeError


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument(
        "--build-root",
        required=True,
        action="append",
        type=Path,
        help="Build tree to scan; repeat for retained compiler/sanitizer trees.",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        run_negative_self_test()
        guard = SecretLeakGuard(
            arguments.repo_root, arguments.build_root
        )
        findings = guard.run()
    except Exception:
        print(
            f"{SCAN_FAILURE_CODE}|guard-internal|byte-record=0",
            file=sys.stderr,
        )
        return 1

    if findings:
        for finding in findings:
            print(finding.render(), file=sys.stderr)
        return 1

    print(
        "Codex synthetic-secret leak guard: PASS "
        f"(paths={guard.encountered_files}, "
        f"unique-scans={guard.unique_file_scans}, "
        f"exact-source-allowlist={len(EXACT_FIXTURE_ALLOWLIST)}, "
        f"verified-package-copies={guard.accepted_packaged_copies})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
