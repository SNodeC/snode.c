# Inventory coverage and verification

Snapshot: `d4b2b247888fac35411e7e38b5ae8e5a4dee22c8` in `/home/voc/projects/snodec/snode.c`, audited on 2026-10-13. The tracked working tree was clean before this documentation work. The sibling `mac-win-port` is an older checkout and was not used to supply evidence for this inventory. No production source, test source or build configuration was edited.

The inventory covers all 1,453 tracked files. All 1,101 C/C++ source/header/template files were scanned, including framework, applications, examples, tools, tests and bundled CLI11. All textual preprocessor branches were retained, so Windows CLI11 branches, getentropy fallbacks, getpeereid and conditional test paths are not hidden by the current Linux configuration. The remaining files are accounted for in the file ledger, with Python/shell/JavaScript/build/install/CI execution surfaces additionally reviewed in the dependency appendix.

The main table has 163 distinct named entries: native OS entry points, runtime/library calls and explicitly labeled OS-related helpers. This is not a claim of 163 kernel syscalls. It includes both raw dispatch targets, `syscall(SYS_pidfd_open, ...)` and `syscall(SYS_getrandom, ...)`. Forty-two entries have existing system declarations, including the project's signal-name helper; their extracted versus inline usage is reported per row. The library appendix has 177 additional named C-library/dependency entries, including the two existing OpenSSL I/O wrappers. C++ OS-facing runtime families and 13 named filesystem operations are in the dependency appendix.

The retained occurrence ledgers contain 726 main-table source references and 504 library references. These numbers include declarations, wrapper definitions, direct calls, callbacks and interposer resolution sites, each labeled by role; they are not execution counts or pure call-site counts. There are 1,711 additional dependency references and 137 matched platform symbols. A matching name is never sufficient by itself to call something a syscall.

The audit used these independent checks:

1. `git ls-files` supplied the file universe and a SHA-256 digest was recorded for each file. No build directory or ignored local checkout was silently treated as authoritative source.
2. A lexer masked comments, quoted strings and C++ raw strings while preserving positions. It collected 3,338 distinct non-member call-like names without evaluating preprocessor branches. Raw candidate data is retained in compressed JSON; readable classifications and candidate examples are retained separately.
3. The C/native API catalog was reviewed against all lowercase C-style names, uppercase macros, Windows API names, existing system headers and source includes. Generic POSIX-looking names were checked in context. Template-qualified member definitions such as `PhysicalSocket<T>::bind`, project `close`/`read`/`send` methods, local variables named `socket`, and bundled CLI methods were excluded from native occurrences. The homonym ledger preserves the excluded evidence.
4. Native candidates were independently compared with exported symbols of the host libc, libm, libbluetooth, libncursesw and libtinfo when installed. Remaining matches were reviewed as C++/project homonyms: `advance`, `call_once`, `clear`, `clock`, `error`, `filter`, `getline`, `index`, `link`, `log`, `move`, `remove`, `step` and `warn`. This host-symbol check supplements source review; it cannot discover every foreign-platform API by itself.
5. Non-call symbol references were reviewed. The two `freeaddrinfo` deleters in IPv4/IPv6 resolution and OpenSSL `GENERAL_NAME_free` callbacks are explicitly retained. Test interposers resolving native `getaddrinfo`, `connect`, `getsockopt`, `close` and `mkdir` with `dlsym` are recorded. Dynamic project plugins selected by runtime names remain represented by the `dlsym` boundary; unknown future plugin binaries are outside the tracked source universe.
6. Raw syscall dispatch, native headers, platform constants, filesystem/process assumptions and CMake probes were checked separately. In particular the OAuth UUID procfs read, IPv6 procfs default, null/random devices, UNIX abstract names, descriptor passing, socket ownership, Bluetooth protocols, package account creation and terminal foreground-job behavior are included.
7. Python scripts were parsed for imports and call expressions. Shell scripts, package hooks, build files and browser/Node surfaces were recorded separately. These runtime APIs and executable commands are not mislabeled C++ kernel syscalls.
8. Platform mappings were researched in Microsoft and Apple primary API references linked next to the tables. Concrete native entry names are supplied where known. Missing equivalents and combinations with different semantics are explicitly marked. A documented function name is evidence of an available building block, not proof that the proposed capsule contract is implemented correctly.

The scanner is lexical rather than a complete C++ name resolver. Its reviewed catalog and explicit homonym decisions make this a source-backed inventory for the pinned revision; they are not a formal proof for arbitrary future source changes, macro expansions supplied by external dependencies, or every transitive kernel call made inside libc/OpenSSL/MariaDB/spdlog. The record does not claim a runtime syscall trace, a build of every preprocessor configuration or successful macOS/Windows execution. There are no remaining known unclassified direct native C entry points from the reviewed candidate set.

The Windows variants for fork continuation, arbitrary POSIX signals/masks, exact terminal job control, process-wide UID/GID switching, Linux abstract UNIX addresses, UNIX datagram descriptor passing and generic L2CAP remain semantic or feasibility decisions. Mixed-handle readiness, native-handle interoperation and atomic noninheritance also need proof. These are findings in the completed source inventory, not hidden omissions or completed ports. Returning unsupported or disabling a demo does not fulfill a behavior-preserving three-platform implementation.

Rebuild the source evidence and generated tables from the current checkout with:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 supplement/porting/tools/scan_calls.py
PYTHONDONTWRITEBYTECODE=1 python3 supplement/porting/tools/build_inventory.py
PYTHONDONTWRITEBYTECODE=1 python3 supplement/porting/tools/dependency_audit.py
PYTHONDONTWRITEBYTECODE=1 python3 supplement/porting/tools/verify_inventory.py
PYTHONDONTWRITEBYTECODE=1 python3 supplement/porting/tools/build_implementation_proposal.py
PYTHONDONTWRITEBYTECODE=1 python3 supplement/porting/tools/verify_implementation_proposal.py
```

`api_catalog.py` contains the reviewed native mappings and API proposals. `CAPSULE_API.md` and the first portion of `PLATFORM_DEPENDENCIES.md` are maintained prose; the dependency generator replaces only its generated sections. Regeneration on another revision requires human review of new candidates, changed native semantics and source exclusions; a successful script exit alone does not establish completeness.

The machine-readable source of each table row is `inventory.json`. `occurrences.json`, `library-occurrences.json`, `dependency-evidence.json`, `include-evidence.json`, `platform-symbols.json`, `source-coverage.json`, `candidate-classification.json`, `excluded-homonyms.json`, `candidate-review.txt` and `call-candidates.json.gz` retain the supporting evidence. `provenance.json` gives counts and snapshot identity. `verification.json` records the final static artifact checks.

Verification for this task is limited to source/evidence consistency, complete table coverage of the reviewed catalog, local links, raw dispatch accounting and unchanged production/test files. No SNode.C build, native framework test or QEMU test was run; no execution result is implied by the documentation checks.

## Implementation draft follow-up

The requested five-column code proposal is in [IMPLEMENTATION_PROPOSAL.md](IMPLEMENTATION_PROPOSAL.md) and its wide HTML rendition. The implementation catalog is separate from the source-evidence catalog: it must not be used to claim that an API is already extracted or working. [IMPLEMENTATION_GAPS.md](IMPLEMENTATION_GAPS.md) records incomplete bodies and scope restrictions. Host compiler checks here are syntax-only checks of documentation snippets, not native framework tests or QEMU portability verification. Production/test/build source remains unchanged.
