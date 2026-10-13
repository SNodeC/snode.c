#!/usr/bin/env python3
"""Generate the reviewed inventory and occurrence ledger without building SNode.C."""
import collections, gzip, hashlib, json, pathlib, re, subprocess
from api_catalog import ROWS, PURE_C, CURSES
from scan_calls import ROOT, OUT, clean
OBS=json.loads(gzip.decompress((OUT/'call-candidates.json.gz').read_bytes()))
COVERAGE=json.loads((OUT/'source-coverage.json').read_text())
REV=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
ROW_NAMES={r['name'] for r in ROWS}
AMBIGUOUS={
 'accept':set(), 'accept4':set(), 'recv':set(),
 'socket':{'tests/component/net/AddressResolutionPolicyTest.cpp','tests/unit/core/ConnectionAttemptLifecycleTest.cpp'},
 'bind':{'tests/component/net/AddressResolutionPolicyTest.cpp','tests/unit/core/ConnectionAttemptLifecycleTest.cpp'},
 'connect':{'tests/component/net/AddressResolutionPolicyTest.cpp'},
 'listen':{'tests/component/net/AddressResolutionPolicyTest.cpp'},
 'open':{'src/web/websocket/client/SocketContextUpgradeFactory.cpp'},
 'pipe':set(),
 'read':{'src/apps/towercalculator/KeyboardReader.cpp','src/utils/Config.cpp','src/web/websocket/client/SocketContextUpgradeFactory.cpp'},
 'write':set(),
 'send':{'tests/component/net/AddressResolutionPolicyTest.cpp'},
 'close':{'src/utils/Daemon.cpp','src/web/websocket/client/SocketContextUpgradeFactory.cpp','tests/component/net/AddressResolutionPolicyTest.cpp'},
 'select':set(), 'shutdown':set(),
 'free':{'src/core/socket/stream/tls/ssl_utils.cpp','src/utils/CLI11.hpp'},
 'exit':{'src/core/SNodeC.cpp'},
 'clear':{'src/tools/snodec-control/src/ui/CursesUi.cpp'},
 'erase':{'src/tools/snodec-control/src/ui/CursesUi.cpp'},
}
def is_external(o):
    if o['member'] or o.get('template_member',False):return False
    name,base,path=o['spelling'],o['base'],o['path']
    if name.startswith('::'):return True
    if '::' in name:
        if name.startswith(('core::system::','utils::system::','core::ssl::')):return True
        if name.startswith('std::') and name.count('::')==1 and base not in {'bind','clock','advance','getline','move','remove','call_once'}:return True
        return False
    if '/system/' in path:return True
    if base in AMBIGUOUS:return path in AMBIGUOUS[base]
    return True

def role(o):
    if re.search(r'extern\s+"C"',o['text']):return 'test interposer definition'
    if '/system/' in o['path']:
        if o['path'].endswith('.h'):return 'wrapper declaration'
        if re.search(r'^\s*(?:[\w:*]+\s+)+\*?'+re.escape(o['base'])+r'\s*\(',o['text']) and not o['text'].startswith('return '):return 'wrapper definition'
        return 'wrapper implementation call'
    if o['name'].startswith(('core::system::','utils::system::','core::ssl::')):return 'extracted caller'
    if o['base']=='getentropy' and o['text'].startswith('int getentropy'):return 'local fallback declaration/definition'
    if o['base'].isupper():return 'macro use'
    return 'inline call'

def row_key(o):
    if o['base']=='syscall':
        target=re.search(r'SYS_\w+',o['text'])
        if not target:raise RuntimeError('Unclassified syscall dispatch '+str(o))
        return 'syscall:'+target[0]
    return o['base']

selected=collections.defaultdict(list); excluded=[]; libraries=collections.defaultdict(list)
libprefix=re.compile(r'^(?:SSL_|TLS_|OPENSSL_|X509_|ASN1_|EVP_|ERR_|sk_GENERAL_NAME_|mysql_|magic_)')
for o in OBS:
    key=row_key(o)
    if key in ROW_NAMES or o['base'] in PURE_C or o['base'] in CURSES or libprefix.match(o['base']):
        if is_external(o):
            item=dict(o,role=role(o))
            (selected if key in ROW_NAMES else libraries)[key].append(item)
        else:excluded.append(dict(o,reason='Project/member/standard-library homonym, not this native API'))
# Explicit callback symbol, not call syntax, found by a token scan.
for f in COVERAGE:
    if f['scope']!='C/C++':continue
    path=f['path'];raw=(ROOT/path).read_text();stripped=clean(raw)
    for ln,line in enumerate(stripped.splitlines(),1):
        if re.search(r'\bGENERAL_NAME_free\b',line):
            libraries['GENERAL_NAME_free'].append(dict(name='GENERAL_NAME_free',base='GENERAL_NAME_free',spelling='GENERAL_NAME_free',path=path,line=ln,text=raw.splitlines()[ln-1].strip(),guard='see enclosing source branch',member=False,role='callback reference'))
# Unqualified libc resolver deleters are callbacks, not direct call syntax.
for path in ['src/net/in/SocketAddrInfo.cpp','src/net/in6/SocketAddrInfo.cpp']:
    for ln,line in enumerate((ROOT/path).read_text().splitlines(),1):
        if 'addrInfo.reset(resolved, freeaddrinfo)' in line:
            selected['freeaddrinfo'].append(dict(name='freeaddrinfo',base='freeaddrinfo',spelling='freeaddrinfo',path=path,line=ln,text=line.strip(),guard='',member=False,role='callback reference'))
# Record dlsym-resolved native callbacks even though the eventual call has another spelling.
for o in OBS:
    if o['base']=='dlsym' and is_external(o):
        target=re.search(r'dlsym\([^,]+,\s*"([A-Za-z_][A-Za-z_0-9]*)"',o['text'])
        if target and target[1] in ROW_NAMES:
            selected[target[1]].append(dict(o,role='native symbol resolved for indirect call',name=target[1]))

for r in ROWS:
    assert selected[r['name']], 'Catalog entry lacks source evidence: '+r['name']

RUNTIME=set('getaddrinfo freeaddrinfo getnameinfo gai_strerror inet_ntop inet_pton getpeereid signal sigemptyset sigaddset sigfillset strsignal time gmtime gmtime_r localtime mktime strftime strptime usleep dlopen dlsym dlclose dlerror isatty tcgetattr tcsetattr tcsetpgrp getpwnam getpwuid getgrnam getgrgid atexit abort exit mkstemp mkdtemp'.split())
for r in ROWS:
    if r['name'] in RUNTIME or r['name'].startswith(('posix_spawn_', 'posix_spawnattr_')):r['kind']='Runtime/library entry'
    if r['name']=='sigabbrev_np':r['kind']='Project helper, not syscall'
    if r['name']=='pthread_sigmask':r['kind']='Runtime/library entry'

def scope(o):
    if o['path']=='src/utils/CLI11.hpp':return 'bundled CLI11'
    if o['path'].startswith('tests/') or '/tests/' in o['path']:return 'test'
    if o['path'].startswith(('src/apps/','examples/')):return 'demo'
    if o['path'].startswith('src/tools/'):return 'tool'
    return 'framework'

def status(name,items):
    wrappers=[o for o in items if o['role']=='wrapper declaration']
    inline=[o for o in items if o['role'] in ('inline call','macro use','native symbol resolved for indirect call','callback reference') and '/system/' not in o['path']]
    wrapped=[o for o in items if o['role']=='extracted caller']
    if wrappers:
        ns='core::ssl' if name.startswith('SSL_') else 'utils::system' if wrappers[0]['path'].startswith('src/utils/') else 'core::system'
        s=('Partial' if inline else 'Yes')+f': `{ns}::{name}`'
        if inline:s+='; inline in '+', '.join(sorted({scope(o) for o in inline}))
        if not wrapped:s+='; no extracted caller found'
        return s
    if name=='getpeereid':return 'Local abstraction: `net::un::peerCredentials`; outside system owner'
    if name in ('sendmsg','recvmsg'):return 'Local methods `Socket::sendFd/recvFd`; native message layout remains outside system owner'
    if name=='getentropy':return 'Local conditional fallback; outside system owner'
    return 'No system wrapper; '+', '.join(sorted({scope(o) for o in items}))

def feasibility(r):
    if r['kind'].startswith(('Helper','Project helper')):return 'Yes; helper or absorbed by enclosing capsule; no kernel call'
    if 'BLOCKED' in r['note'] or r['name'] in {'fork','setsid','getpgrp','kill','sendmsg','recvmsg','getpeereid','syscall:SYS_pidfd_open','epoll_create1','epoll_ctl','epoll_wait','epoll_pwait','ppoll','pselect','poll','select','posix_spawnattr_setsigdefault','posix_spawnattr_setsigmask','access','umask','flock','signal','pipe2','accept4','strptime','getppid'}:
        return 'Contract change or feasibility decision required; no proven exact three-target substitution'
    return 'Yes; normalize native types/errors and preserve stated contract'

def anchor(name):return re.sub(r'[^a-z0-9_-]+','',name.lower())
def src(o):return f"[{o['path']}:{o['line']}](../../{o['path']}#L{o['line']})"
def cell(s):return s.replace('|','&#124;').replace('\n',' ')
def link(name):return f'[`{name}`](CALL_SITES.md#{anchor(name)})'

SOURCES={
 'Files and descriptors': [('Windows file APIs','https://learn.microsoft.com/en-us/windows/win32/api/fileapi/'),('Windows handle APIs','https://learn.microsoft.com/en-us/windows/win32/api/handleapi/'),('NtCreateFile','https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntcreatefile'),('Windows pipe APIs','https://learn.microsoft.com/en-us/windows/win32/api/namedpipeapi/'),('Darwin syscall definitions','https://github.com/apple-oss-distributions/xnu/blob/main/bsd/kern/syscalls.master')],
 'Sockets and name resolution':[('Winsock APIs','https://learn.microsoft.com/en-us/windows/win32/api/winsock2/'),('Windows resolver APIs','https://learn.microsoft.com/en-us/windows/win32/api/ws2tcpip/'),('Apple peer credentials','https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man3/getpeereid.3.html'),('Microsoft AF_UNIX limitations','https://devblogs.microsoft.com/commandline/af_unix-comes-to-windows/')],
 'Readiness and signals':[('Apple kqueue and kevent','https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/kevent.2.html'),('WSAPoll','https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-wsapoll'),('Windows select','https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-select'),('Windows wait handles','https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitformultipleobjects'),('Console control handlers','https://learn.microsoft.com/en-us/windows/console/setconsolectrlhandler')],
 'Processes and identity':[('Windows process APIs','https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/'),('CreateProcessW','https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw'),('Apple posix_spawn','https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/posix_spawn.2.html'),('Windows token information','https://learn.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-gettokeninformation'),('Windows ownership','https://learn.microsoft.com/en-us/windows/win32/api/aclapi/nf-aclapi-setnamedsecurityinfow')],
 'Time entropy loading and terminal':[('Windows library loader','https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/'),('Windows entropy','https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptgenrandom'),('Darwin syscall definitions','https://github.com/apple-oss-distributions/xnu/blob/main/bsd/kern/syscalls.master'),('Windows time','https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getsystemtimepreciseasfiletime'),('Console input modes','https://learn.microsoft.com/en-us/windows/console/getconsolemode')],
 'Runtime environment and stdio':[('UCRT function reference','https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/crt-alphabetical-function-reference?view=msvc-170'),('Windows environment','https://learn.microsoft.com/en-us/windows/win32/api/processenv/'),('Windows argument parser','https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-commandlinetoargvw')],
 'Macros and representation helpers':[('Winsock helper APIs','https://learn.microsoft.com/en-us/windows/win32/api/winsock2/'),('Windows ancillary helpers','https://learn.microsoft.com/en-us/windows/win32/api/ws2def/'),('Apple Bluetooth utilities','https://developer.apple.com/documentation/iobluetooth/iobluetoothutilities-h?language=objc')]
}
intro=f'''# Complete source inventory and capsule API proposal

Audited repository: `snode.c`, commit `{REV}`. Scope: all {len(COVERAGE)} tracked files, including {sum(f['scope']=='C/C++' for f in COVERAGE)} C/C++ source/header/template files, all textual preprocessor branches, bundled CLI11, framework, tools, demos and tests. The older sibling checkout is not mixed into this snapshot. This is a source inventory and API proposal, not an implementation or a claim that every proposed mapping already works.

The main table contains **{len(ROWS)} named native-call, runtime and OS-helper entries**. Raw `syscall` dispatches have separate rows for `SYS_pidfd_open` and `SYS_getrandom`. Entries explicitly labeled helper/runtime are not being counted as kernel syscalls. [Library dependencies](LIBRARY_CALLS.md), [indirect OS dependencies and capability variants](PLATFORM_DEPENDENCIES.md), and [coverage and limitations](AUDIT.md) complete the inventory.

“Really the syscall” means concrete native entry-point names in the platform columns, never a placeholder such as “Windows adapter.” Win32, Winsock, libc and framework functions are labeled accurately: many source operations are not kernel syscalls, and Windows supported user-space interfaces are usually Win32/Winsock rather than raw NT syscall numbers. “No equivalent” is an explicit finding, not a proposed no-op implementation. Where several native calls are listed they are candidate building blocks, not a proven drop-in equivalent.

The rightmost column proposes directly callable C++ declarations and their behavioral contracts. `sys` abbreviates `core::system` in this proposal; see [capsule types and ownership](CAPSULE_API.md). Repeated declarations intentionally consolidate related source calls into one authority. Three target-specific implementations would fulfill each OS-facing contract; pure value helpers and portable third-party calls do not need fabricated kernel calls or three duplicated algorithms. No production API has been added.

“Already extracted” is based on actual system declarations and callers. Partial includes bypasses in tests/tools as well as production. Local abstractions outside the system owner are identified explicitly. Every call name links to all retained source occurrences, with declarations, native wrapper bodies, extracted callers, inline uses and interposition references distinguished.

Native mappings below are engineering proposals derived from the linked primary API references. API availability and semantic equivalence still require target SDK and QEMU validation; no runtime portability result is claimed. In particular the older Microsoft AF_UNIX documentation establishes a documented limitation, not exhaustive proof about every future Windows build.
'''
parts=[intro.replace('\n\n', '\n\n**Latest implementation proposal:** [five-column table with function bodies](IMPLEMENTATION_PROPOSAL.md). This inventory retains extraction/source evidence and earlier conceptual contracts; use the implementation proposal for the current draft signatures.\n\n', 1)]
for group in dict.fromkeys(r['group'] for r in ROWS):
    parts.append('\n## '+group+'\n\n')
    parts.append('Native references: '+', '.join(f'[{label}]({url})' for label,url in SOURCES[group])+'.\n\n')
    parts.append('| Call and kind | Already extracted | Extractable | Linux native entry | macOS native alternative | Windows native alternative | Proposed capsule API and caller contract |\n| --- | --- | --- | --- | --- | --- | --- |\n')
    for r in ROWS:
        if r['group'] != group: continue
        name=r['name'];items=selected[name]
        fields=[link(name)+' — '+r['kind'],status(name,items),feasibility(r),r['linux'],r['mac'],r['windows'],'`'+r['api']+'` '+r['note']]
        parts.append('| '+' | '.join(map(cell,fields))+' |\n')
(OUT/'SYSCALL_INVENTORY.md').write_text(''.join(parts))
ledger=['# Source occurrences\n\nCommit `'+REV+'`. This ledger retains declarations and definitions as evidence, separately from calls. Line numbers refer to the audited source revision. Preprocessor guards are recorded in `occurrences.json`; they are textual branch context, not an evaluated build configuration.\n']
for name,items in sorted({**selected,**libraries}.items()):
    ledger.append('\n## '+name+'\n\n| Source | Role | Source line |\n| --- | --- | --- |\n')
    for o in sorted(items,key=lambda o:(o['path'],o['line'],o.get('offset',0))):
        ledger.append('| '+src(o)+' | '+o['role']+' | `'+cell(o['text']).replace('`','\\`')+'` |\n')
(OUT/'CALL_SITES.md').write_text(''.join(ledger))
(OUT/'occurrences.json').write_text(json.dumps(dict(selected),indent=2)+'\n')
(OUT/'library-occurrences.json').write_text(json.dumps(dict(libraries),indent=2)+'\n')
(OUT/'excluded-homonyms.json').write_text(json.dumps(excluded,indent=2)+'\n')
(OUT/'inventory.json').write_text(json.dumps([dict(r,extracted=status(r['name'],selected[r['name']]),extractable=feasibility(r),occurrences=len(selected[r['name']])) for r in ROWS],indent=2)+'\n')
# Library table is an explicit boundary: functions are recorded but never falsely named syscalls.
parts=['# Library calls retained outside the syscall count\n\nEvery listed operation has source evidence in [the occurrence ledger](CALL_SITES.md). Native OS variants are not applicable to a portable library entry point; use the same library contract on each target. The operating-system interactions inside separately supplied library binaries are not SNode.C source calls. Their build/runtime availability must be verified on each target.\n\n| Call/library | Already extracted | Extractable | Linux | macOS | Windows | Capsule API proposal |\n| --- | --- | --- | --- | --- | --- | --- |\n']
for name,items in sorted(libraries.items()):
    family='OpenSSL' if re.match(r'^(SSL_|TLS_|OPENSSL_|X509_|ASN1_|EVP_|ERR_|GENERAL_NAME_|sk_GENERAL_NAME_)',name) else 'MariaDB Connector/C' if name.startswith('mysql_') else 'libmagic' if name.startswith('magic_') else 'curses' if name in CURSES else 'C/C++ runtime'
    proposal='Retain `'+name+'` with its existing signature and ownership contract; no OS capsule required.'
    if name in ('SSL_read','SSL_write'):proposal='Reuse `int core::ssl::'+name+'(SSL* ssl, '+('void*' if name=='SSL_read' else 'const void*')+' buffer, int count);` Preserve OpenSSL retry/error-queue semantics.'
    if name=='SSL_set_fd':proposal='`Result<void> core::ssl::attach(SSL* ssl, sys::Handle socket);` Borrow socket lifetime; adapt native socket width without narrowing; explicit BIO choice needs review.'
    if name=='SSL_get_fd':proposal='`Result<sys::Handle> core::ssl::socket(SSL* ssl);` Return borrowed capsule handle; avoid native integer assumptions.'
    if name=='mysql_get_socket':proposal='`Result<sys::Handle> database::mariadb::system::socket(MYSQL* connection);` Borrow Connector/C socket lifetime; preserve native socket width and readiness integration.'
    if family=='curses':proposal='Retain `'+name+'` through the existing UI boundary where the chosen curses distribution supports it; Windows library availability is a dependency decision, not a Win32 syscall mapping.'
    parts.append('| '+' | '.join(map(cell,[link(name)+' / '+family,status(name,items),'Library boundary; no new OS wrapper for portable calls',name+' / '+family,name+' / '+family,name+' / '+family+' (target build required)',proposal]))+' |\n')
(OUT/'LIBRARY_CALLS.md').write_text(''.join(parts))
# All non-member candidate names are accounted for; this is a review ledger, not compiler name resolution.
bybase=collections.defaultdict(list)
for o in OBS:
    if not o['member']:bybase[o['base']].append(o)
review=[]
for name,items in sorted(bybase.items()):
    if name in ROW_NAMES or name=='syscall':category='Native catalog; occurrence-level homonyms filtered'
    elif name in libraries:category='Library appendix; occurrence-level homonyms filtered'
    elif all(o['name'].startswith('std::') for o in items):category='C++ standard library/type; OS-bearing families in PLATFORM_DEPENDENCIES'
    elif name in {'if','for','while','switch','catch','return','sizeof','decltype','alignof','defined','static_assert','noexcept','typeid','void','bool','int','char','else','throw','new','delete'}:category='C++ language/preprocessor token, not an external function'
    else:category='Project/bundled C++ symbol, macro, constructor, local callable or standard-library homonym; not an identified native C entry'
    review.append(dict(name=name,classification=category,occurrences=len(items),examples=[dict(path=o['path'],line=o['line']) for o in items[:3]]))
(OUT/'candidate-classification.json').write_text(json.dumps(review,indent=2)+'\n')
provenance=dict(revision=REV,tracked_files=len(COVERAGE),cxx_files=sum(f['scope']=='C/C++' for f in COVERAGE),native_runtime_helper_rows=len(ROWS),library_rows=len(libraries),selected_occurrences=sum(map(len,selected.values())),library_occurrences=sum(map(len,libraries.values())),excluded_homonyms=len(excluded),kind_counts=dict(collections.Counter(r['kind'] for r in ROWS)),method='all-branches lexical scan, reviewed C API catalog, homonym review, native library symbol cross-check, includes/build/scripts audit; not a semantic AST or runtime syscall trace')
(OUT/'provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
print(json.dumps(provenance,indent=2))
