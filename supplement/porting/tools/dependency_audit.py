#!/usr/bin/env python3
"""Record indirect C++ runtime, platform symbols, and non-C++ execution surfaces."""
import ast, collections, gzip, json, pathlib, re
from scan_calls import ROOT,OUT,clean
coverage=json.loads((OUT/'source-coverage.json').read_text()); obs=json.loads(gzip.decompress((OUT/'call-candidates.json.gz').read_bytes()))
evidence=[];symbols=collections.defaultdict(list);fs=collections.defaultdict(list)
for o in obs:
    if o['name'].startswith('std::filesystem::'):
        fs[o['base']].append(o)
        evidence.append(dict(category='C++ filesystem',path=o['path'],line=o['line'],text=o['text']))
cpp_pattern=re.compile(r'\bstd::(?:[iof]*fstream|filesystem::(?:directory_iterator|recursive_directory_iterator)|random_device|(?:j?thread)|(?:recursive_)?mutex|condition_variable|call_once|chrono::(?:steady_clock|system_clock|high_resolution_clock)::now|this_thread::sleep_for)\b|\b(?:errno|environ)\b|/proc/|/dev/|/run/|/var/|/etc/')
flag_pattern=re.compile(r'\b(?:(?:SO_|SOCK_|O_|F_|AF_|PF_|EPOLL|POLL|MSG_|SCM_|SIG|RTLD_|IPV6_|TCP_|AI_|NI_|EAI_|SYS_|POSIX_SPAWN|FD_|LOCK_|SHUT_|BTPROTO_|IPPROTO_)[A-Za-z0-9_]*|SOL_SOCKET|STDIN_FILENO|STDOUT_FILENO|STDERR_FILENO|AT_FDCWD|X_OK|TCSANOW|ICANON|ECHO|VMIN|VTIME)\b')
scripts=[]
for f in coverage:
    p=ROOT/f['path'];raw=p.read_text(errors='replace');lines=raw.splitlines()
    if f['scope']=='C/C++':
        stripped=clean(raw).splitlines()
        for ln,line in enumerate(stripped,1):
            for token in set(flag_pattern.findall(line)):symbols[token].append(dict(path=f['path'],line=ln))
            if cpp_pattern.search(line) or re.search(r'"/(?:proc|dev|run|var|etc)/',lines[ln-1]):
                evidence.append(dict(category='C++ indirect OS or path',path=f['path'],line=ln,text=lines[ln-1].strip()))
    elif p.suffix=='.py':
        scripts.append((f['path'],'Python'))
        tree=ast.parse(raw)
        for node in ast.walk(tree):
            if isinstance(node,(ast.Import,ast.ImportFrom,ast.Call)):
                evidence.append(dict(category='Python import/call',path=f['path'],line=node.lineno,text=lines[node.lineno-1].strip(),expression=ast.unparse(node.func) if isinstance(node,ast.Call) else ast.unparse(node)))
    elif raw.startswith('#!') and re.search(r'\b(?:sh|bash)\b',lines[0]):
        scripts.append((f['path'],'Shell'))
        for ln,line in enumerate(lines,1):
            if line.strip() and not line.lstrip().startswith('#'):
                evidence.append(dict(category='Shell source line',path=f['path'],line=ln,text=line.strip()))
    elif p.suffix in {'.js','.ts','.vue','.html'}:
        for ln,line in enumerate(lines,1):
            if re.search(r'\b(?:import|require|fetch|WebSocket|EventSource|listen|sendFile|process\.env|localStorage|XMLHttpRequest|axios|http\.|https\.)\b',line):
                evidence.append(dict(category='JavaScript/browser runtime',path=f['path'],line=ln,text=line.strip()))
    if p.name=='CMakeLists.txt' or p.suffix in {'.cmake','.yml'} or f['path']=='supplement/openwrt/Makefile':
        for ln,line in enumerate(lines,1):
            if re.search(r'find_package|find_library|pkg_check_modules|check_symbol_exists|check_include|execute_process|add_test|target_link_libraries|IO_MULTIPLEXERS|WIN32|APPLE|UNIX|Linux|COMMAND|COMMANDS|run:|apt-get|apt |systemctl|ldconfig|SHARED|LIBRARY|RPATH|\.so',line):
                evidence.append(dict(category='Build/install/CI surface',path=f['path'],line=ln,text=line.strip()))
(OUT/'dependency-evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
(OUT/'platform-symbols.json').write_text(json.dumps(symbols,indent=2)+'\n')
base=(OUT/'PLATFORM_DEPENDENCIES.md').read_text().split('\n## Indirect C++ filesystem operations')[0]
base+='\n## Indirect C++ filesystem operations\n\nThese calls are already library abstractions. Retain them where their standard contract is sufficient; add no duplicate OS capsule merely for accounting. The concrete native functions listed below are backend building blocks, not a claim about the exact implementation of an unpinned C++ runtime.\n\n| Source library call | Source evidence | Linux native building blocks | macOS native building blocks | Windows native building blocks | Capsule API proposal if project semantics require extraction |\n| --- | --- | --- | --- | --- | --- |\n'
mapping={
 'canonical':('realpath/readlink/stat','realpath/readlink/stat','CreateFileW and GetFinalPathNameByHandleW','Result<Path> sys::canonical_path(Path);'),
 'create_directories':('mkdir/stat','mkdir/stat','CreateDirectoryW/GetFileAttributesW','Result<void> sys::create_directories(Path, Permissions);'),
 'exists':('stat','stat','GetFileAttributesW','Result<bool> sys::exists(Path);'),
 'file_size':('stat/fstat','stat/fstat','GetFileInformationByHandleEx','Result<FileInfo> sys::path_info(Path, FollowLinks);'),
 'is_directory':('stat/S_ISDIR','stat/S_ISDIR','GetFileAttributesW(FILE_ATTRIBUTE_DIRECTORY)','bool sys::is_directory(const FileInfo&);'),
 'is_regular_file':('stat/S_ISREG','stat/S_ISREG','GetFileInformationByHandleEx/GetFileType','bool sys::is_regular(const FileInfo&);'),
 'last_write_time':('stat','stat','GetFileTime/GetFileInformationByHandleEx','Result<FileInfo> sys::path_info(Path, FollowLinks);'),
 'path':('No syscall for lexical path construction','No syscall','No syscall','Retain std::filesystem::path; agree UTF-8/native encoding at OS boundary.'),
 'permissions':('chmod/fchmodat','chmod/fchmodat','SetNamedSecurityInfoW; POSIX permissions are not Windows ACLs','Result<void> sys::set_permissions(Path, Permissions);'),
 'remove':('unlink/rmdir','unlink/rmdir','DeleteFileW/RemoveDirectoryW','Result<bool> sys::remove(Path);'),
 'remove_all':('directory traversal, unlink/rmdir','directory traversal, unlink/rmdir','FindFirstFileW/FindNextFileW, DeleteFileW/RemoveDirectoryW','Result<uint64_t> sys::remove_tree(Path); Only where explicitly intended by existing callers.'),
 'status':('stat','stat','GetFileInformationByHandleEx','Result<FileInfo> sys::path_info(Path, FollowLinks);'),
 'temp_directory_path':('environment lookup and path validation','environment lookup and path validation','GetTempPathW/GetTempPath2W according to target SDK','Result<Path> sys::temporary_path();'),
}
assert set(fs)<=set(mapping),set(fs)-set(mapping)
for name,items in sorted(fs.items()):
    o=items[0];link=f'[{o["path"]}:{o["line"]}](../../{o["path"]}#L{o["line"]})'
    base+='| `std::filesystem::'+name+'` | '+link+'; all sites in dependency-evidence.json | '+' | '.join(mapping[name])+' |\n'
base+='''
## Other runtime and dependency boundaries

| Source dependency | Used OS-facing behavior | Actual target-native building blocks | Capsule API proposal or retained authority |
| --- | --- | --- | --- |
| C++ ifstream/ofstream and standard streams | Configuration, PID files, HTTP/demo files, logs and procfs reads; stream methods are indirect I/O | POSIX open/read/write/close; Win32 CreateFileW/ReadFile/WriteFile or CRT equivalents according to runtime | Retain portable streams except native pseudo-file/device assumptions, which have explicit rows above. |
| C++ chrono clocks and sleep | Timestamps and test deadlines | Linux clock_gettime/nanosleep; macOS clock_gettime/mach time facilities; Windows QueryPerformanceCounter/GetSystemTimePreciseAsFileTime/wait timers, selected by runtime | Retain chrono; if extraction is required, `Result<MonotonicTime> sys::monotonic_time();` and existing wall_time/sleep_for contracts. |
| C++ thread, call_once and library synchronization | TLS peer test thread, one-time MariaDB setup, asynchronous spdlog internals | pthread_create/join and synchronization on POSIX; Windows thread/synchronization APIs; exact runtime lowering is not fixed here | Retain standard C++ and spdlog ownership; do not add a second thread pool or scheduler for portability. |
| C++ random_device and mt19937 | utils::Random, utils::Uuid and WebSocket masking | OS entropy source is implementation-dependent; mt19937 itself has no syscall | Keep PRNG policy distinct from cryptographic `sys::random_bytes`. Source scan does not prove random_device is cryptographically secure. |
| OpenSSL | TLS protocol, certificate handling, password userdata, randomness inside dependency | Same OpenSSL APIs on all targets; internal native calls are dependency-owned | All named C entry points are in LIBRARY_CALLS.md. Adapt socket attachment/retrieval at the existing TLS owner; preserve SSL_get_error timing and error queues. |
| MariaDB Connector/C | Async query continuations, sockets, timeout and process exit cleanup | Same Connector/C APIs on target builds | All named calls are in LIBRARY_CALLS.md. mysql_get_socket crosses the native-handle boundary; verify native socket width and event-loop readiness. |
| libmagic | MIME database loading and file type lookup | magic_open/load/file/error/close in target library | Keep MimeTypes authoritative; package the correct database and library instead of replacing it with an unrelated OS MIME query. |
| Curses | Terminal drawing, input, mode save/restore and key codes in snodec-control | curses distribution on POSIX; Windows-compatible curses build or reviewed console implementation | All named calls/macros are listed individually in LIBRARY_CALLS.md. A Windows library build is unverified; not all curses variants are interchangeable. |
| CLI11 bundled header | Filesystem validation, environment, locale, Windows argv conversion | Existing conditional POSIX/CRT/Win32 branches | Inventory vendor calls but preserve its configuration/parser authority. Prefer supported upstream platform branches over a private fork. |
| spdlog, nlohmann JSON, STL | Logging, parsing, containers and formatting | Their own portable runtime/dependency implementations | No direct native syscall hidden in a project wrapper is intentionally omitted; internals of external dependency binaries are a separate transitive audit, not fabricated rows. |

## Scripts build and deployment

These are host/runtime execution surfaces, not C++ syscall entry points. Source lines, imported APIs and Python call expressions are retained in dependency-evidence.json. They must be included in target workflow validation.

| Surface | Used operations | Platform consequence and proposed boundary |
| --- | --- | --- |
| examples/echo/tests/echo_tests.py | Python socket/bind/listen/accept/recv/sendall/getsockname/setsockopt/settimeout/close; subprocess.run/Popen/poll/communicate/terminate/kill; tempfile, environment, monotonic time and sleep | Retain Python APIs. POSIX negative signal return codes and Windows terminate behavior differ; QEMU checks must assert the agreed target contract rather than assume SIGTERM equivalence. |
| Express compatibility Python suite | http.client connections and requests, response reads, open/read JSON, sys.exit | Retain Python APIs; source calls are captured by AST, including object-method calls. |
| Shell test launchers and manual fixture | bash/sh, python3, c++, find, grep, mktemp, rm, trap, printf/echo, shell exit and signal behavior | Shell commands are executable dependencies, not kernel calls. Preserve equivalent installed-consumer coverage with a target-appropriate harness; do not silently skip Windows. |
| Debian and RPM postinst | getent, addgroup/adduser or groupadd/useradd, ldconfig | Linux package hooks; macOS and Windows deployment needs its own account/group and loader policy, not renamed syscall wrappers. |
| CMake and CTest | execute_process, add_test, install, library discovery, getentropy/SYS_getrandom/getpeereid checks, epoll/poll/select choices | Detect target OS and SDK capabilities. The getentropy probe currently names sys/random.h; header placement matters independently of function availability. |
| OpenWrt package recipe and GitHub workflows | Toolchains, dependencies, installation/package commands, Linux runners | Existing platform-specific workflows remain explicit; do not count them as tested on a new target without corresponding guest/toolchain execution. |
| Node/Express reference and supplement servers | HTTP listeners, sendFile, environment and console | Runtime owns native networking/filesystem calls. Supplement has absolute developer paths requiring configuration; these are not SNode.C C++ syscalls. |
| Browser JavaScript/TypeScript/Vue | WebSocket, fetch/HTTP client calls and browser storage | Browser APIs, not host syscalls; keep them in the inventory scope classification without inventing Win32/POSIX counterparts. |
'''
base+='\nAudited executable-script files: '+', '.join('`'+p+'` ('+kind+')' for p,kind in scripts)+'.\n'
(OUT/'PLATFORM_DEPENDENCIES.md').write_text(base)
print(f'{len(evidence)} dependency references; {len(symbols)} platform symbols; {len(fs)} filesystem entry names; {len(scripts)} Python/shell files')
