"""Copyable proposal bodies. Not production implementations or test results."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'supplement'/'porting'
ENTRIES={}
def put(names,api,linux,windows,mac=None,note=''):
    for name in names.split():
        ENTRIES[name]=dict(api=api,linux=linux.strip(),windows=windows.strip(),mac=(linux if mac is None else mac).strip(),note=note)
def fn(ret,name,args,body):return f'{ret} {name}({args}) {{\n    '+body.replace('\n','\n    ')+'\n}'
def simple(name,ret,args,call,win=None,note='',mac=None,api=None):
    method=api or 'do'+name[0].upper()+name[1:]
    linux=fn(ret,method,args,call)
    windows=fn(ret,method,args,win) if win is not None else ''
    macbody=fn(ret,method,args,mac) if mac is not None else None
    put(name, f'{ret} {method}({args});',linux,windows,macbody,note)

for name,native,buf in [('read','ReadFile','MutableBytes'),('write','WriteFile','Bytes')]:
    method='do'+name.title()
    pos=f'''const auto n = std::min(b.size(), static_cast<std::size_t>(PTRDIFF_MAX));
const auto r = ::{name}(f.native, b.data(), n);
if (r < 0) return posixFailure<Count>();
return ok(static_cast<Count>(r));'''
    eof='\n    if (e == ERROR_BROKEN_PIPE || e == ERROR_HANDLE_EOF) return ok(Count{0});' if name=='read' else ''
    win=f'''const auto n = static_cast<DWORD>(std::min(b.size(),
    std::min(static_cast<std::size_t>(MAXDWORD), static_cast<std::size_t>(PTRDIFF_MAX))));
DWORD done = 0;
if (!::{native}(f.native, b.data(), n, &done, nullptr)) {{
    const DWORD e = ::GetLastError();{eof}
    return winFailure<Count>(e);
}}
return ok(static_cast<Count>(done));'''
    simple(name,'Result<Count>',f'File f, {buf} b',pos,win,note='One native call; partial count retained. Zero-length request returns zero without implying EOF. Windows body requires synchronous file/byte-pipe handles; it does not implement nonblocking event-loop pipe I/O. Socket I/O uses doRecv/doSend. POSIX write retains existing SIGPIPE policy; suppressing it is a separate lifecycle decision.')
put('close','Status doClose(File f); Status doClose(Socket s);',
'''Status doClose(File f) { return posixStatus(::close(f.native)); }
Status doClose(Socket s) { return posixStatus(::close(s.native)); }''',
'''Status doClose(File f) { return winStatus(::CloseHandle(f.native)); }
Status doClose(Socket s) { return socketStatus(::closesocket(s.native)); }''',note='One close attempt, owner retires the reference. Do not retry on EINTR; final OS-specific release semantics require target validation.')
simple('ftruncate','Status','File f, std::uint64_t length',
'''if (length > static_cast<std::uint64_t>(std::numeric_limits<off_t>::max())) return posixFailure(EOVERFLOW);
return posixStatus(::ftruncate(f.native, static_cast<off_t>(length)));''',
'''if (length > static_cast<std::uint64_t>(INT64_MAX)) return winFailure(ERROR_INVALID_PARAMETER);
FILE_END_OF_FILE_INFO info{};
info.EndOfFile.QuadPart = static_cast<LONGLONG>(length);
return winStatus(::SetFileInformationByHandle(f.native, FileEndOfFileInfo, &info, sizeof(info)));''',api='doTruncate')
for name,win in [('unlink','DeleteFileW'),('rmdir','RemoveDirectoryW'),('chdir','SetCurrentDirectoryW')]:
    simple(name,'Status','const Path& p',f'return posixStatus(::{name}(p.c_str()));',f'return winStatus(::{win}(p.c_str()));')
simple('dup','Result<File>','File f','''const int n = ::dup(f.native);
if (n == -1) return posixFailure<File>();
return ok(File{n});''','''HANDLE n = INVALID_HANDLE_VALUE;
if (!::DuplicateHandle(::GetCurrentProcess(), f.native, ::GetCurrentProcess(), &n, 0, FALSE, DUPLICATE_SAME_ACCESS)) return winFailure<File>();
return ok(File{n});''',note='File handles only; no SOCKET-to-HANDLE cast. POSIX dup clears close-on-exec, Windows result is non-inheritable; caller migration must choose a consistent inheritance contract.')
simple('pipe','Result<Pipe>','', '''int p[2];
if (::pipe(p) == -1) return posixFailure<Pipe>();
return ok(Pipe{File{p[0]}, File{p[1]}});''', '''HANDLE r, w;
if (!::CreatePipe(&r, &w, nullptr, 0)) return winFailure<Pipe>();
return ok(Pipe{File{r}, File{w}});''',note='Blocking byte pipe only. Windows handles are non-inheritable. ProcessRunner needs an integrated launch/capture capsule before replacing select on these pipes.')
simple('getpid','std::uint64_t','', 'return static_cast<std::uint64_t>(::getpid());','return ::GetCurrentProcessId();',api='doProcessId')
for name,ret,args,call in [('exit','void','int status','::exit(status);'),('abort','void','','::abort();'),('atexit','int','void (*callback)()','return ::atexit(callback);'),('fclose','int','FILE* stream','return ::fclose(stream);'),('fflush','int','FILE* stream','return ::fflush(stream);'),('strftime','std::size_t','char* out, std::size_t n, const char* format, const std::tm& time','return ::strftime(out, n, format, &time);'),('setlocale','char*','int category, const char* locale','return ::setlocale(category, locale);')]:
    simple(name,ret,args,call,call,note='C runtime operation, not a kernel syscall. Return/lifetime/error semantics remain those of this runtime call.')
simple('_exit','void','int status','::_exit(status);','::ExitProcess(static_cast<UINT>(status));',api='doExitImmediately',note='Windows DLL detach differs; not a fork-child-safe POSIX _exit equivalence.')
simple('strdup','char*','const char* text','return ::strdup(text);','return ::_strdup(text);',note='Caller frees with matching CRT free. Allocation failure remains nullptr.')
simple('strcasecmp','int','const char* a, const char* b','return ::strcasecmp(a, b);','return ::_stricmp(a, b);',note='Locale-dependent comparison retained. HTTP ASCII-only comparison should be one shared protocol helper, not three OS implementations.')
simple('fopen','FILE*','const Path& path, bool write','return ::fopen(path.c_str(), write ? "wb" : "rb");','return ::_wfopen(path.c_str(), write ? L"wb" : L"rb");',note='Binary read or truncating binary write only; extend the semantic mode enum if another observed call needs append/update.')
simple('freopen','FILE*','FILE* stream, bool input','return ::freopen("/dev/null", input ? "r" : "w+", stream);','return ::_wfreopen(L"NUL", input ? L"r" : L"w+", stream);',api='doRedirectToNull',note='Replaces the complete native null-device choice in Daemon.cpp. The stream remains owned by its current owner.')
simple('fileno','Result<File>','FILE* stream','''const int fd = ::fileno(stream);
if (fd == -1) return posixFailure<File>();
return ok(File{fd});''','''const int fd = ::_fileno(stream);
if (fd == -1) return posixFailure<File>();
const intptr_t h = ::_get_osfhandle(fd);
if (h == -1) return posixFailure<File>();
return ok(File{reinterpret_cast<HANDLE>(h)});''',note='Borrowed reference; never close independently of FILE*. CRT invalid-parameter behavior must be respected; stream must be valid.')
for source,name,win in [('gmtime','doUtcTime','_gmtime64_s'),('gmtime_r','doUtcTime','_gmtime64_s'),('localtime','doLocalTime','_localtime64_s')]:
    call='gmtime_r' if source!='localtime' else 'localtime_r'
    simple(source,'Result<std::tm>','std::int64_t seconds',f'''const auto native = static_cast<time_t>(seconds);
if (static_cast<std::int64_t>(native) != seconds) return posixFailure<std::tm>(EOVERFLOW);
std::tm out{{}};
if (!::{call}(&native, &out)) return posixFailure<std::tm>();
return ok(out);''',f'''const __time64_t native = seconds;
std::tm out{{}};
const int e = ::{win}(&out, &native);
if (e) return posixFailure<std::tm>(e);
return ok(out);''',api=name,note='Owned calendar fields; OS accepted timestamp ranges differ.')
simple('time','std::int64_t','','return static_cast<std::int64_t>(::time(nullptr));','return static_cast<std::int64_t>(::_time64(nullptr));',api='doUnixSeconds')
simple('mktime','std::int64_t','std::tm& calendar','return static_cast<std::int64_t>(::mktime(&calendar));','return static_cast<std::int64_t>(::_mktime64(&calendar));',api='doLocalTimeToUnix',note='Mutates/normalizes calendar as before; -1 can be either failure or valid pre-epoch time. HTTP UTC parsing is a separate semantic issue.')
simple('gettimeofday','Result<WallTime>','', '''timeval tv{};
if (::gettimeofday(&tv, nullptr) == -1) return posixFailure<WallTime>();
return ok(WallTime{tv.tv_sec, static_cast<std::uint32_t>(tv.tv_usec)});''', '''FILETIME ft{};
::GetSystemTimePreciseAsFileTime(&ft);
const auto ticks = (static_cast<std::uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
const auto delta = static_cast<std::int64_t>(ticks) - INT64_C(116444736000000000);
auto seconds = delta / INT64_C(10000000);
auto remainder = delta % INT64_C(10000000);
if (remainder < 0) { --seconds; remainder += INT64_C(10000000); }
return ok(WallTime{seconds, static_cast<std::uint32_t>(remainder / 10)});''',api='doWallTime')
put('getentropy syscall:SYS_getrandom','Status doRandomBytes(MutableBytes bytes);', '''Status doRandomBytes(MutableBytes bytes) {
    while (!bytes.empty()) {
        const auto n = std::min(bytes.size(), std::size_t{256});
        if (::getentropy(bytes.data(), n) != 0) {
            if (errno == EINTR) continue;
            return posixFailure();
        }
        bytes = bytes.subspan(n);
    }
    return ok();
}''', '''Status doRandomBytes(MutableBytes bytes) {
    while (!bytes.empty()) {
        const auto n = static_cast<ULONG>(std::min(bytes.size(), static_cast<std::size_t>(ULONG_MAX)));
        const NTSTATUS e = ::BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(bytes.data()), n, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        if (e < 0) return fail(ErrorDomain::ntstatus, static_cast<std::uint32_t>(e));
        bytes = bytes.subspan(n);
    }
    return ok();
}''',note='Fill-all-or-error; buffer may contain a partially filled prefix on error and must be discarded. Replaces the entire WebSocket fallback. Linux minimum libc getentropy availability must be set; old libc variant below uses SYS_getrandom.')
ENTRIES['syscall:SYS_getrandom']['linux']='''Status doRandomBytes(MutableBytes bytes) {
    while (!bytes.empty()) {
        const auto n = std::min(bytes.size(), std::size_t{256});
        const long done = ::syscall(SYS_getrandom, bytes.data(), n, 0);
        if (done < 0) {
            if (errno == EINTR) continue;
            return posixFailure();
        }
        if (done == 0) return posixFailure(EIO);
        bytes = bytes.subspan(static_cast<std::size_t>(done));
    }
    return ok();
}'''
# Additional source includes for this alternative: <sys/syscall.h>.
for source,method,buf in [('recv','doRecv','MutableBytes'),('send','doSend','Bytes')]:
    mode='ReadMode' if source=='recv' else 'WriteMode'
    flags='mode == ReadMode::peek ? MSG_PEEK : mode == ReadMode::urgent ? MSG_OOB : 0' if source=='recv' else 'mode == WriteMode::urgent ? MSG_OOB : 0'
    cast='char*' if source=='recv' else 'const char*'
    pos=f'''const int flags = {flags};
const auto r = ::{source}(s.native, b.data(), std::min(b.size(), static_cast<std::size_t>(PTRDIFF_MAX)), flags);
if (r < 0) return posixFailure<Count>();
return ok(static_cast<Count>(r));'''
    win=f'''const int flags = {flags};
const auto n = static_cast<int>(std::min(b.size(), static_cast<std::size_t>(INT_MAX)));
const int r = ::{source}(s.native, reinterpret_cast<{cast}>(b.data()), n, flags);
if (r == SOCKET_ERROR) return wsaFailure<Count>();
return ok(static_cast<Count>(r));'''
    simple(source,'Result<Count>',f'Socket s, {buf} b, {mode} mode',pos,win,api=method,note='Normal/peek/urgent modes cover observed intent; datagram boundary handling must not silently split an oversized datagram. These bounded snippets are for stream I/O. SIGPIPE policy on POSIX remains caller lifecycle policy.')
for source in ['bind','connect']:
    simple(source,'Status','Socket s, const Address& address',f'return socketStatus(::{source}(s.native, reinterpret_cast<const sockaddr*>(&address.storage), static_cast<socklen_t>(address.length)));',f'return socketStatus(::{source}(s.native, reinterpret_cast<const sockaddr*>(&address.storage), static_cast<int>(address.length)));',note='Address must be constructed by backend address conversion, length bounded by storage. EINPROGRESS/WSAEWOULDBLOCK is retained as a domain-tagged error for nonblocking connect.')
simple('listen','Status','Socket s, int backlog','return socketStatus(::listen(s.native, backlog));','return socketStatus(::listen(s.native, backlog));')
simple('shutdown','Status','Socket s, Direction direction','''const int how = direction == Direction::receive ? SHUT_RD : direction == Direction::send ? SHUT_WR : SHUT_RDWR;
return socketStatus(::shutdown(s.native, how));''','''const int how = direction == Direction::receive ? SD_RECEIVE : direction == Direction::send ? SD_SEND : SD_BOTH;
return socketStatus(::shutdown(s.native, how));''')
simple('socket','Result<Socket>','Family family, SocketKind kind', '''const int af = family == Family::ipv4 ? AF_INET : AF_INET6;
const int type = kind == SocketKind::stream ? SOCK_STREAM : SOCK_DGRAM;
const int s = ::socket(af, type, 0);
if (s == -1) return posixFailure<Socket>();
return ok(Socket{s});''','''const int af = family == Family::ipv4 ? AF_INET::AF_INET : AF_INET6;
const int type = kind == SocketKind::stream ? SOCK_STREAM : SOCK_DGRAM;
const SOCKET s = ::socket(af, type, 0);
if (s == INVALID_SOCKET) return wsaFailure<Socket>();
return ok(Socket{s});'''.replace('AF_INET::AF_INET','AF_INET'),note='Concrete INET/INET6 TCP/UDP variant. WSAStartup/WSACleanup must bracket framework lifetime once. UNIX, Bluetooth, inheritance and nonblocking creation require separate operation capsules; this is not the whole original socket contract.')
simple('accept','Result<Socket>','Socket listener', '''const int s = ::accept(listener.native, nullptr, nullptr);
if (s == -1) return posixFailure<Socket>();
return ok(Socket{s});''','''const SOCKET s = ::accept(listener.native, nullptr, nullptr);
if (s == INVALID_SOCKET) return wsaFailure<Socket>();
return ok(Socket{s});''',note='Base accept without requested flags. Peer can be obtained through doGetpeername. Flags/inheritance and asynchronous cancellation are outside this basic variant.')
for name in ['getsockname','getpeername']:
    simple(name,'Result<Address>','Socket s',f'''Address a;
socklen_t n = sizeof(a.storage);
if (::{name}(s.native, reinterpret_cast<sockaddr*>(&a.storage), &n) != 0) return posixFailure<Address>();
a.length = n;
return ok(a);''',f'''Address a;
int n = sizeof(a.storage);
if (::{name}(s.native, reinterpret_cast<sockaddr*>(&a.storage), &n) == SOCKET_ERROR) return wsaFailure<Address>();
a.length = static_cast<std::uint32_t>(n);
return ok(a);''')
for name,direction,bits in [('htobe16','HostToBig',16),('htobe32','HostToBig',32),('htobe64','HostToBig',64),('be16toh','BigToHost',16),('be32toh','BigToHost',32),('be64toh','BigToHost',64),('htons','HostToBig',16),('htonl','HostToBig',32),('ntohs','BigToHost',16),('htobs','HostToLittle',16),('btohs','LittleToHost',16)]:
    swap=' | '.join(f'((v >> {i*8}) & UINT{bits}_C(0xff)) << {(bits//8-1-i)*8}' for i in range(bits//8))
    body=f'''if constexpr (std::endian::native == std::endian::{"little" if "Little" in direction else "big"}) return v;
return static_cast<std::uint{bits}_t>({swap});'''
    simple(name,f'std::uint{bits}_t',f'std::uint{bits}_t v',body,body,api=f'do{direction}{bits}',note='Pure C++20 byte-order conversion. Same body on all three platforms; no kernel call exists or is needed.')
for name,ret,args,body in [('WIFEXITED','bool','const ExitStatus& s','return !s.signaled;'),('WEXITSTATUS','std::uint32_t','const ExitStatus& s','return s.code;'),('WIFSIGNALED','bool','const ExitStatus& s','return s.signaled;'),('WTERMSIG','std::optional<int>','const ExitStatus& s','return s.signaled ? std::optional<int>{s.signal} : std::nullopt;'),('S_ISREG','bool','const FileInfo& info','return info.kind == FileInfo::Kind::regular;'),('S_ISSOCK','bool','const FileInfo& info','return info.kind == FileInfo::Kind::socket;')]:
    simple(name,ret,args,body,body,note='Pure query of normalized capsule output. Producer must establish this information; Windows exception codes are not signal numbers.')
# Environment access returns owned text; empty and unset remain distinct.
put('getenv _dupenv_s','Result<std::optional<std::string>> doEnvironment(const std::string& name);', '''Result<std::optional<std::string>> doEnvironment(const std::string& name) {
    if (name.empty() || name.find('=') != std::string::npos || name.find('\0') != std::string::npos) return posixFailure<std::optional<std::string>>(EINVAL);
    const char* p = ::getenv(name.c_str());
    return ok(p ? std::optional<std::string>{p} : std::nullopt);
}''', r'''Result<std::optional<std::string>> doEnvironment(const std::string& name) {
    using Value = std::optional<std::string>;
    if (name.empty() || name.find('=') != std::string::npos || name.find('\0') != std::string::npos) return winFailure<Value>(ERROR_INVALID_PARAMETER);
    auto key = toWide(name);
    if (!key) return {{}, key.error};
    std::vector<wchar_t> buffer(256);
    for (;;) {
        ::SetLastError(ERROR_SUCCESS);
        const DWORD n = ::GetEnvironmentVariableW(key.value.c_str(), buffer.data(), static_cast<DWORD>(buffer.size()));
        if (!n) {
            const DWORD e = ::GetLastError();
            if (e == ERROR_ENVVAR_NOT_FOUND) return ok(Value{});
            if (e == ERROR_SUCCESS) return ok(Value{std::string{}});
            return winFailure<Value>(e);
        }
        if (n >= buffer.size()) { buffer.resize(n); continue; }
        auto text = toUtf8(std::wstring_view(buffer.data(), n));
        if (!text) return {{}, text.error};
        return ok(Value{std::move(text.value)});
    }
}'''.replace("'\\0'","'\0'"),note='Windows backend reads the OS environment; it does not synchronize CRT getenv caches. Migrate all framework consumers to this authority before adopting OS environment mutation. Bundled CLI11 may retain its existing implementation.')
simple('setenv','Status','const std::string& name, const std::string& value, bool overwrite',
'''if (name.find('\0') != std::string::npos || value.find('\0') != std::string::npos) return posixFailure(EINVAL);
return posixStatus(::setenv(name.c_str(), value.c_str(), overwrite));''',
'''if (name.empty() || name.find('=') != std::string::npos || name.find('\0') != std::string::npos || value.find('\0') != std::string::npos) return winFailure(ERROR_INVALID_PARAMETER);
if (!overwrite) {
    auto old = doEnvironment(name);
    if (!old) return {{}, old.error};
    if (old.value) return ok();
}
auto key = toWide(name); auto text = toWide(value);
if (!key) return {{}, key.error};
if (!text) return {{}, text.error};
return winStatus(::SetEnvironmentVariableW(key.value.c_str(), text.value.c_str()));''',api='doSetEnvironment',note='Requires serialized startup-time mutation. Windows OS environment only; retained CRT/environ consumers must be migrated together. Check-then-set overwrite=false is not an atomic concurrent operation.')
for name,method in [('inet_ntop','doFormatIp'),('inet_pton','doParseIp')]:
    if name=='inet_ntop':
        simple(name,'Result<std::string>','const Ip& ip', '''char text[INET6_ADDRSTRLEN]{};
const int af = ip.family == Family::ipv4 ? AF_INET : AF_INET6;
if (!::inet_ntop(af, ip.bytes.data(), text, sizeof(text))) return posixFailure<std::string>();
return ok(std::string(text));''','''char text[INET6_ADDRSTRLEN]{};
const int af = ip.family == Family::ipv4 ? AF_INET : AF_INET6;
if (!::InetNtopA(af, const_cast<std::byte*>(ip.bytes.data()), text, sizeof(text))) return wsaFailure<std::string>();
return ok(std::string(text));''',api=method,note='IP numeric text is ASCII, so InetNtopA has no pathname/codepage loss.')
    else:
        simple(name,'Result<Ip>','Family family, const std::string& text', '''Ip ip; ip.family = family;
const int r = ::inet_pton(family == Family::ipv4 ? AF_INET : AF_INET6, text.c_str(), ip.bytes.data());
if (r == 0) return posixFailure<Ip>(EINVAL);
if (r < 0) return posixFailure<Ip>();
return ok(ip);''','''Ip ip; ip.family = family;
const int r = ::InetPtonA(family == Family::ipv4 ? AF_INET : AF_INET6, text.c_str(), ip.bytes.data());
if (r == 0) return wsaFailure<Ip>(WSAEINVAL);
if (r < 0) return wsaFailure<Ip>();
return ok(ip);''',api=method,note='Numeric ASCII only. Reject embedded NUL at the public text boundary; never parse a truncated untrusted string.')
put('getaddrinfo freeaddrinfo','Result<std::vector<Address>> doResolve(const char* host, const char* service, Family family, SocketKind kind, bool passive);', '''Result<std::vector<Address>> doResolve(const char* host, const char* service, Family family, SocketKind kind, bool passive) {
    addrinfo hints{};
    hints.ai_family = family == Family::ipv4 ? AF_INET : AF_INET6;
    hints.ai_socktype = kind == SocketKind::stream ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_flags = passive ? AI_PASSIVE : 0;
    addrinfo* result = nullptr;
    const int e = ::getaddrinfo(host, service, &hints, &result);
    if (e) {
        if (e == EAI_SYSTEM) return posixFailure<std::vector<Address>>();
        return fail<std::vector<Address>>(ErrorDomain::resolver, static_cast<std::uint32_t>(e));
    }
    std::unique_ptr<addrinfo, decltype(&::freeaddrinfo)> guard(result, ::freeaddrinfo);
    std::vector<Address> addresses;
    for (auto* p = result; p; p = p->ai_next) {
        if (p->ai_addrlen > sizeof(sockaddr_storage)) return posixFailure<std::vector<Address>>(EOVERFLOW);
        Address a; a.length = static_cast<std::uint32_t>(p->ai_addrlen);
        std::memcpy(&a.storage, p->ai_addr, p->ai_addrlen);
        addresses.push_back(a);
    }
    return ok(std::move(addresses));
}''', '''Result<std::vector<Address>> doResolve(const char* host, const char* service, Family family, SocketKind kind, bool passive) {
    auto h = toWide(host ? host : ""); auto s = toWide(service ? service : "");
    if (!h) return {{}, h.error};
    if (!s) return {{}, s.error};
    ADDRINFOW hints{};
    hints.ai_family = family == Family::ipv4 ? AF_INET : AF_INET6;
    hints.ai_socktype = kind == SocketKind::stream ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_flags = passive ? AI_PASSIVE : 0;
    ADDRINFOW* result = nullptr;
    const int e = ::GetAddrInfoW(host ? h.value.c_str() : nullptr, service ? s.value.c_str() : nullptr, &hints, &result);
    if (e) return fail<std::vector<Address>>(ErrorDomain::resolver, static_cast<std::uint32_t>(e));
    std::unique_ptr<ADDRINFOW, decltype(&::FreeAddrInfoW)> guard(result, ::FreeAddrInfoW);
    std::vector<Address> addresses;
    for (auto* p = result; p; p = p->ai_next) {
        if (p->ai_addrlen > sizeof(sockaddr_storage)) return winFailure<std::vector<Address>>(ERROR_BUFFER_OVERFLOW);
        Address a; a.length = static_cast<std::uint32_t>(p->ai_addrlen);
        std::memcpy(&a.storage, p->ai_addr, p->ai_addrlen);
        addresses.push_back(a);
    }
    return ok(std::move(addresses));
}''',note='Includes <memory>. This basic variant folds matching release into resolution; vector owns its data. Current numeric-only/canonical/V4-mapped/ordering policies need a ResolveOptions extension before replacing all callers; no freeaddrinfo callback escapes.')
simple('dlopen','Result<Library>','const Path& path', '''void* module = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
if (!module) return posixFailure<Library>(ENOENT);
return ok(Library{module});''', '''HMODULE module = ::LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
if (!module) return winFailure<Library>();
return ok(Library{module});''',api='doLoadLibrary',note='Proposal accepts absolute paths only. POSIX error text must additionally be captured with dlerror; ENOENT alone loses the diagnostic. Existing loader visibility/search options must be audited before adopting this narrowed variant.')
simple('dlclose','Status','Library lib','return posixStatus(::dlclose(lib.native));','return winStatus(::FreeLibrary(lib.native));',api='doUnloadLibrary',note='POSIX dlclose error detail comes from dlerror, not errno; this body needs the loader error capsule below before use.')
simple('isatty','Result<bool>','File f', '''if (::isatty(f.native)) return ok(true);
if (errno == ENOTTY) return ok(false);
return posixFailure<bool>();''','''DWORD mode;
if (::GetConsoleMode(f.native, &mode)) return ok(true);
const DWORD e = ::GetLastError();
::SetLastError(ERROR_SUCCESS);
const DWORD type = ::GetFileType(f.native);
if (type == FILE_TYPE_UNKNOWN && ::GetLastError() != ERROR_SUCCESS) return winFailure<bool>();
if (e == ERROR_INVALID_HANDLE) return ok(false);
return winFailure<bool>(e);''',api='doIsTerminal',note='Windows console capability, not every pseudo-terminal protocol. POSIX errno ENOTTY is a successful negative result.')
simple('getppid','std::uint64_t','','return static_cast<std::uint64_t>(::getppid());',None,api='doParentProcessId',note='For cross-platform fork-test replacement, pass the known parent identity in the launch operation; snapshot parent-PID queries cannot guarantee live identity.')
# fstat does not expose a native stat structure to callers.
statpos='''struct stat st{};
if (::fstat(f.native, &st) == -1) return posixFailure<FileInfo>();
FileInfo out;
out.kind = S_ISREG(st.st_mode) ? FileInfo::Kind::regular : S_ISDIR(st.st_mode) ? FileInfo::Kind::directory : S_ISSOCK(st.st_mode) ? FileInfo::Kind::socket : FileInfo::Kind::other;
out.size = static_cast<std::uint64_t>(st.st_size); out.links = st.st_nlink; out.device = st.st_dev;
static_assert(sizeof(st.st_ino) <= sizeof(out.id));
std::memcpy(out.id.data(), &st.st_ino, sizeof(st.st_ino));
out.modifiedSeconds = st.st_mtim.tv_sec; out.modifiedNanoseconds = st.st_mtim.tv_nsec;
return ok(out);'''
simple('fstat','Result<FileInfo>','File f',statpos, '''FILE_STANDARD_INFO standard{}; FILE_ID_INFO id{}; FILE_BASIC_INFO basic{};
if (!::GetFileInformationByHandleEx(f.native, FileStandardInfo, &standard, sizeof(standard)) ||
    !::GetFileInformationByHandleEx(f.native, FileIdInfo, &id, sizeof(id)) ||
    !::GetFileInformationByHandleEx(f.native, FileBasicInfo, &basic, sizeof(basic))) return winFailure<FileInfo>();
FileInfo out;
out.kind = (basic.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ? FileInfo::Kind::other : standard.Directory ? FileInfo::Kind::directory : FileInfo::Kind::regular;
out.size = standard.EndOfFile.QuadPart; out.links = standard.NumberOfLinks; out.device = id.VolumeSerialNumber;
std::memcpy(out.id.data(), id.FileId.Identifier, out.id.size());
auto delta = basic.LastWriteTime.QuadPart - INT64_C(116444736000000000);
auto seconds = delta / INT64_C(10000000); auto remainder = delta % INT64_C(10000000);
if (remainder < 0) { --seconds; remainder += INT64_C(10000000); }
out.modifiedSeconds = seconds; out.modifiedNanoseconds = static_cast<std::uint32_t>(remainder * 100);
return ok(out);''',mac=statpos.replace('st.st_mtim','st.st_mtimespec'),api='doFileInfo',note='Disk-file handles only on Windows. Reparse points remain other until their tags are inspected; this does not yet fulfill UNIX socket-node ownership detection. Unsupported filesystem ID queries fail explicitly.')
for name,follow in [('stat',True),('lstat',False),('_stat64',True)]:
    base=statpos.replace('::fstat(f.native, &st)',f'::{"stat" if follow else "lstat"}(p.c_str(), &st)').replace('FileInfo::Kind::other;','S_ISLNK(st.st_mode) ? FileInfo::Kind::symlink : FileInfo::Kind::other;')
    simple(name,'Result<FileInfo>','const Path& p',base, f'''const HANDLE h = ::CreateFileW(p.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS{'' if follow else ' | FILE_FLAG_OPEN_REPARSE_POINT'}, nullptr);
if (h == INVALID_HANDLE_VALUE) return winFailure<FileInfo>();
auto result = doFileInfo(File{{h}});
const BOOL closed = ::CloseHandle(h);
if (!closed && result) return winFailure<FileInfo>();
return result;''',mac=base.replace('st.st_mtim','st.st_mtimespec'),api='doPathInfo' if follow else 'doLinkInfo',note='Windows symlink/socket reparse-tag classification is still needed before replacing security-sensitive lstat ownership checks. _stat64 is a Windows-only vendor spelling; no additional POSIX syscall.')
simple('flock','Status','File f, bool exclusive, bool unlock, bool wait', '''const int op = unlock ? LOCK_UN : (exclusive ? LOCK_EX : LOCK_SH) | (wait ? 0 : LOCK_NB);
return posixStatus(::flock(f.native, op));''','''OVERLAPPED offset{};
if (unlock) return winStatus(::UnlockFileEx(f.native, 0, MAXDWORD, MAXDWORD, &offset));
const DWORD flags = (exclusive ? LOCKFILE_EXCLUSIVE_LOCK : 0) | (wait ? 0 : LOCKFILE_FAIL_IMMEDIATELY);
return winStatus(::LockFileEx(f.native, flags, 0, MAXDWORD, MAXDWORD, &offset));''',api='doFileLock',note='Windows body requires a synchronous disk handle. Windows range locks affect I/O; POSIX flock is advisory. Duplicate-handle and process lifetime semantics must be validated before UNIX lock-file migration.')
# Explicit-offset I/O: Windows waits on a private OVERLAPPED event, never shared file position.
for name,buf,winfn in [('pread','MutableBytes','ReadFile'),('pwrite','Bytes','WriteFile')]:
    eof='if (e == ERROR_HANDLE_EOF) return ok(Count{0});' if name=='pread' else ''
    simple(name,'Result<Count>',f'File f, {buf} b, std::uint64_t offset',f'''if (offset > static_cast<std::uint64_t>(std::numeric_limits<off_t>::max())) return posixFailure<Count>(EOVERFLOW);
const auto r = ::{name}(f.native, b.data(), std::min(b.size(), static_cast<std::size_t>(PTRDIFF_MAX)), static_cast<off_t>(offset));
if (r < 0) return posixFailure<Count>();
return ok(static_cast<Count>(r));''',f'''if (offset > static_cast<std::uint64_t>(INT64_MAX)) return winFailure<Count>(ERROR_INVALID_PARAMETER);
OVERLAPPED operation{{}};
operation.Offset = static_cast<DWORD>(offset); operation.OffsetHigh = static_cast<DWORD>(offset >> 32);
operation.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
if (!operation.hEvent) return winFailure<Count>();
DWORD done = 0;
const DWORD n = static_cast<DWORD>(std::min(b.size(), std::min(static_cast<std::size_t>(MAXDWORD), static_cast<std::size_t>(PTRDIFF_MAX))));
BOOL success = ::{winfn}(f.native, b.data(), n, &done, &operation);
DWORD e = success ? ERROR_SUCCESS : ::GetLastError();
if (!success && e == ERROR_IO_PENDING) {{
    success = ::GetOverlappedResult(f.native, &operation, &done, TRUE);
    e = success ? ERROR_SUCCESS : ::GetLastError();
}}
::CloseHandle(operation.hEvent);
if (!success) {{ {eof} return winFailure<Count>(e); }}
return ok(static_cast<Count>(done));''',note='Windows file MUST have been opened FILE_FLAG_OVERLAPPED; doRead/doWrite synchronous bodies cannot operate on that same handle. This distinction must be integrated into doOpen and resource ownership before copying into production. This operation waits; it is not an event-loop submission API.')
# Error capture and loader lifetimes are kept in the operation, not ambient errno.
ENTRIES['dlopen']['linux']='''Result<Library> doLoadLibrary(const Path& path) {
    void* module = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!module) {
        const char* message = ::dlerror();
        return {{}, {ErrorDomain::loader, 0, message ? message : "dlopen failed"}};
    }
    return ok(Library{module});
}'''
ENTRIES['dlopen']['mac']=ENTRIES['dlopen']['linux']
ENTRIES['dlopen']['note']='Absolute path, eager binding and local visibility are the proposed contract. Existing RTLD_GLOBAL/RTLD_LAZY uses must stay visible as an adoption gap, not be silently changed.'
ENTRIES['dlclose']['linux']='''Status doUnloadLibrary(Library lib) {
    if (::dlclose(lib.native) == 0) return ok();
    const char* message = ::dlerror();
    return {{}, {ErrorDomain::loader, 0, message ? message : "dlclose failed"}};
}'''
ENTRIES['dlclose']['mac']=ENTRIES['dlclose']['linux']
ENTRIES['dlclose']['note']='Release one module reference only after its symbols are retired. POSIX loader text is captured immediately.'
put('dlsym dlerror','Result<void*> doLibrarySymbol(Library library, const char* name);', '''Result<void*> doLibrarySymbol(Library library, const char* name) {
    ::dlerror();
    void* address = ::dlsym(library.native, name);
    const char* message = ::dlerror();
    if (message) return {{}, {ErrorDomain::loader, 0, message}};
    return ok(address);
}''', '''Result<void*> doLibrarySymbol(Library library, const char* name) {
    const auto address = ::GetProcAddress(library.native, name);
    if (!address) return winFailure<void*>();
    return ok(reinterpret_cast<void*>(address));
}''',note='Replaces both symbol lookup and dlerror clear/query. POSIX null can be a valid symbol value. RTLD_NEXT test interposition has no equivalent here; redesign that fixture separately. Calling-symbol casts must preserve the actual function type/calling convention.')
simple('GetLastError','Error','ErrorDomain domain', '''if (domain == ErrorDomain::posix) return {domain, static_cast<std::uint32_t>(errno)};
return {ErrorDomain::posix, EINVAL};''','''if (domain == ErrorDomain::win32) return {domain, ::GetLastError()};
if (domain == ErrorDomain::winsock) return {domain, static_cast<std::uint32_t>(::WSAGetLastError())};
if (domain == ErrorDomain::posix) return {domain, static_cast<std::uint32_t>(errno)};
return {ErrorDomain::win32, ERROR_INVALID_PARAMETER};''',api='doCaptureError',note='Backend helper only. Never call for resolver APIs that return their error code directly, or after another native call has overwritten the error.')
put('strerror gai_strerror','std::string doErrorText(const Error& error);', '''std::string doErrorText(const Error& error) {
    if (!error.text.empty()) return error.text;
    if (error.domain == ErrorDomain::resolver) return ::gai_strerror(static_cast<int>(error.code));
    if (error.domain == ErrorDomain::posix) return std::generic_category().message(static_cast<int>(error.code));
    return "native error " + std::to_string(error.code);
}''', '''std::string doErrorText(const Error& error) {
    if (!error.text.empty()) return error.text;
    if (error.domain == ErrorDomain::posix) return std::generic_category().message(static_cast<int>(error.code));
    if (error.domain == ErrorDomain::ntstatus) return "NTSTATUS " + std::to_string(error.code);
    wchar_t* buffer = nullptr;
    const DWORD n = ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error.code, 0, reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);
    if (!n) return "native error " + std::to_string(error.code);
    std::unique_ptr<wchar_t, decltype(&::LocalFree)> guard(buffer, ::LocalFree);
    auto text = toUtf8(std::wstring_view(buffer, n));
    return text ? std::move(text.value) : "native error " + std::to_string(error.code);
}''',note='Includes <system_error> and <memory>. Returned text is owned; not a pointer into a shared mutable error buffer.')
put('GetCommandLineW CommandLineToArgvW LocalFree','Result<std::vector<std::string>> doArguments(int argc, char* const* argv);', '''Result<std::vector<std::string>> doArguments(int argc, char* const* argv) {
    if (argc < 0 || (argc && !argv)) return posixFailure<std::vector<std::string>>(EINVAL);
    std::vector<std::string> result;
    for (int i = 0; i < argc; ++i) result.emplace_back(argv[i]);
    return ok(std::move(result));
}''', '''Result<std::vector<std::string>> doArguments(int, char* const*) {
    int count = 0;
    wchar_t** values = ::CommandLineToArgvW(::GetCommandLineW(), &count);
    if (!values) return winFailure<std::vector<std::string>>();
    std::unique_ptr<wchar_t*, decltype(&::LocalFree)> guard(values, ::LocalFree);
    std::vector<std::string> result;
    for (int i = 0; i < count; ++i) {
        auto text = toUtf8(values[i]);
        if (!text) return {{}, text.error};
        result.push_back(std::move(text.value));
    }
    return ok(std::move(result));
}''',note='Includes <memory>. Consolidates Windows command-line allocation, Unicode conversion and LocalFree. POSIX argv must be valid and is preserved byte-for-byte; this does not enforce UTF-8 on arbitrary POSIX argument bytes.')
simple('getnameinfo','Result<std::pair<std::string, std::string>>','const Address& address, bool numeric', '''char host[NI_MAXHOST], service[NI_MAXSERV];
const int flags = numeric ? NI_NUMERICHOST | NI_NUMERICSERV : 0;
const int e = ::getnameinfo(reinterpret_cast<const sockaddr*>(&address.storage), address.length, host, sizeof(host), service, sizeof(service), flags);
if (e) {
    if (e == EAI_SYSTEM) return posixFailure<std::pair<std::string, std::string>>();
    return fail<std::pair<std::string, std::string>>(ErrorDomain::resolver, static_cast<std::uint32_t>(e));
}
return ok(std::pair<std::string, std::string>{host, service});''','''wchar_t host[NI_MAXHOST], service[NI_MAXSERV];
const int flags = numeric ? NI_NUMERICHOST | NI_NUMERICSERV : 0;
const int e = ::GetNameInfoW(reinterpret_cast<const sockaddr*>(&address.storage), static_cast<int>(address.length), host, NI_MAXHOST, service, NI_MAXSERV, flags);
if (e) return fail<std::pair<std::string, std::string>>(ErrorDomain::resolver, static_cast<std::uint32_t>(e));
auto h = toUtf8(host); auto s = toUtf8(service);
if (!h) return {{}, h.error};
if (!s) return {{}, s.error};
return ok(std::pair<std::string, std::string>{std::move(h.value), std::move(s.value)});''',api='doEndpointName',note='Basic numeric/reverse-lookup form. NI_NAMEREQD and other observed flags require semantic options before all callers can migrate.')
# Preserve all socket option commands in the inventory; portable socket-error query is concrete.
put('getsockopt','Result<Error> doSocketError(Socket socket);', '''Result<Error> doSocketError(Socket socket) {
    int error = 0; socklen_t size = sizeof(error);
    if (::getsockopt(socket.native, SOL_SOCKET, SO_ERROR, &error, &size) == -1) return posixFailure<Error>();
    return ok(error ? Error{ErrorDomain::posix, static_cast<std::uint32_t>(error)} : Error{});
}''', '''Result<Error> doSocketError(Socket socket) {
    int error = 0; int size = sizeof(error);
    if (::getsockopt(socket.native, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &size) == SOCKET_ERROR) return wsaFailure<Error>();
    return ok(error ? Error{ErrorDomain::winsock, static_cast<std::uint32_t>(error)} : Error{});
}''',note='This body implements SO_ERROR only. SO_DOMAIN/SO_TYPE/SO_PROTOCOL, linger, reuse and IPv6 options remain separately enumerated in platform-symbols.json; this is not a complete getsockopt replacement.')
put('setsockopt','Status doTcpNoDelay(Socket socket, bool enabled);', '''Status doTcpNoDelay(Socket socket, bool enabled) {
    const int value = enabled;
    return socketStatus(::setsockopt(socket.native, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value)));
}''', '''Status doTcpNoDelay(Socket socket, bool enabled) {
    const int value = enabled;
    return socketStatus(::setsockopt(socket.native, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&value), sizeof(value)));
}''',note='Includes <netinet/tcp.h> on POSIX. TCP_NODELAY body only; SO_REUSEADDR/SO_REUSEPORT cannot be interchanged with Windows exclusive-address semantics. Other option bodies remain an implementation gap.')
simple('fcntl','Status','Socket socket, bool enabled', '''const int flags = ::fcntl(socket.native, F_GETFL);
if (flags == -1) return posixFailure();
return posixStatus(::fcntl(socket.native, F_SETFL, enabled ? flags | O_NONBLOCK : flags & ~O_NONBLOCK));''','''u_long value = enabled;
return socketStatus(::ioctlsocket(socket.native, FIONBIO, &value));''',api='doSetNonblocking',note='Socket variant only, preserves existing flags on POSIX. File/pipe nonblocking and F_GETFD inspection require separate resource-specific capsules; ioctlsocket is not a universal fcntl replacement.')
simple('access','Result<bool>','const Path& path', '''if (::access(path.c_str(), F_OK) == 0) return ok(true);
if (errno == ENOENT || errno == ENOTDIR) return ok(false);
return posixFailure<bool>();''','''const DWORD attributes = ::GetFileAttributesW(path.c_str());
if (attributes != INVALID_FILE_ATTRIBUTES) return ok(true);
const DWORD error = ::GetLastError();
if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) return ok(false);
return winFailure<bool>(error);''',api='doPathExists',note='Existence check only. R_OK/W_OK require access-token checks on Windows; X_OK has no executable-mode-bit equivalent. Never authorize a later open using this racy check.')
simple('fchdir','Status','File directory','return posixStatus(::fchdir(directory.native));',None,api='doRestoreDirectory',note='No rename-safe Windows handle-based SetCurrentDirectory operation. Replacing with a saved path changes semantics; not supplied as an equivalent.')
simple('system','int','const std::string& command','return ::system(command.c_str());','''auto text = toWide(command);
if (!text) { errno = EINVAL; return -1; }
return ::_wsystem(text.value.c_str());''',api='doRunShell',note='Only invocation is portable. Shell language, exit encoding and Warema device commands are not; caller needs an operation-specific contract.')
# Helpers replacing fd_set manipulation for a socket-only poll value.
for name,ret,args,body in [
('FD_ZERO','void','std::vector<PollItem>& items','items.clear();'),
('FD_SET','void','std::vector<PollItem>& items, PollItem item','auto found = std::find_if(items.begin(), items.end(), [&](const auto& v) { return v.socket.native == item.socket.native; });\nif (found == items.end()) items.push_back(item);\nelse { found->read |= item.read; found->write |= item.write; found->urgent |= item.urgent; }'),
('FD_CLR','void','std::vector<PollItem>& items, Socket socket','std::erase_if(items, [&](const auto& item) { return item.socket.native == socket.native; });'),
('FD_ISSET','bool','const PollItem& item','return item.returned != 0;')]:
    simple(name,ret,args,body,body,note='Socket collection helper only. Returned masks still need semantic decoding; these helpers cannot make Windows select work with process pipes.')
# Bluetooth text representation is one shared algorithm, not platform syscalls.
put('ba2str','std::string doFormatBluetooth(const std::array<std::uint8_t, 6>& address);', '''std::string doFormatBluetooth(const std::array<std::uint8_t, 6>& address) {
    static constexpr char digits[] = "0123456789ABCDEF";
    std::string out;
    for (std::size_t i = 0; i < address.size(); ++i) {
        if (i) out += ':';
        out += digits[address[i] >> 4]; out += digits[address[i] & 15];
    }
    return out;
}''','',note='Address bytes use printed order; reverse BlueZ bdaddr_t byte order in its adapter. No kernel call.')
ENTRIES['ba2str']['windows']=ENTRIES['ba2str']['linux']
put('str2ba','Result<std::array<std::uint8_t, 6>> doParseBluetooth(std::string_view text);', '''Result<std::array<std::uint8_t, 6>> doParseBluetooth(std::string_view text) {
    std::array<std::uint8_t, 6> out{};
    auto digit = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
    };
    if (text.size() != 17) return posixFailure<decltype(out)>(EINVAL);
    for (std::size_t i = 0; i < out.size(); ++i) {
        const int high = digit(text[3*i]), low = digit(text[3*i+1]);
        if (high < 0 || low < 0 || (i < 5 && text[3*i+2] != ':')) return posixFailure<decltype(out)>(EINVAL);
        out[i] = static_cast<std::uint8_t>((high << 4) | low);
    }
    return ok(out);
}''','',note='Canonical six-byte grammar only. BlueZ may accept additional spellings; characterize current accepted inputs before claiming equivalent replacement.')
ENTRIES['str2ba']['windows']=ENTRIES['str2ba']['linux']
# Whole configuration setup: individual NSS/identity calls do not escape.
config_names='getuid geteuid getgid getegid getpwuid getgrgid getgrnam chown mkdir'
put(config_names,'ConfigSetup doPrepareConfiguration(const std::string& serviceGroup);',
    (OUT/'snippets/config-linux.cpp.txt').read_text(),(OUT/'snippets/config-windows.cpp.txt').read_text(),
    (OUT/'snippets/config-mac.cpp.txt').read_text(),
    note='Whole Config.cpp:953–1065 replacement; see config-caller.cpp.txt and config-types.hpp.txt. POSIX preserves root/XDG/create-only ownership policy with copied NSS strings. Windows LocalSystem/known-folder/ACL policy is a proposed behavior, not a proven POSIX equivalence. Other uses of these calls remain in the source ledger and need the daemon/test-specific capsule as applicable.')
put('getpwnam seteuid setegid','Status doEnterDaemonIdentity(const std::string& user, const std::string& group); Status doRestoreRealIdentity();', '''Status doEnterDaemonIdentity(const std::string& user, const std::string& group) {
    errno = 0;
    const struct group* gr = ::getgrnam(group.c_str());
    if (!gr) return posixFailure(errno ? errno : ENOENT);
    if (::setegid(gr->gr_gid) != 0) return posixFailure();
    errno = 0;
    const passwd* pw = ::getpwnam(user.c_str());
    if (!pw) return posixFailure(errno ? errno : ENOENT);
    if (::seteuid(pw->pw_uid) != 0) return posixFailure();
    return ok();
}
Status doRestoreRealIdentity() {
    if (::seteuid(::getuid()) != 0) return posixFailure();
    return posixStatus(::setegid(::getgid()));
}''','',note='Entire Daemon.cpp identity sequence, including restore order. Startup-only; preserves effective-only changes and supplementary groups as today. Failure can occur after group changed; fail the daemon startup, never proceed as if fully switched. Windows thread impersonation does not implement this process-wide contract. A service account/launch token requires an explicitly different launch API.')
simple('umask','std::uint32_t','std::uint32_t mask','return ::umask(static_cast<mode_t>(mask));',None,api='doCreationMask',note='Process-wide POSIX creation mask. No Win32 process-wide ACL equivalent. Windows config capsule supplies security descriptors at creation; CRT _umask would not control native CreateFileW.')
simple('usleep','Status','std::uint32_t microseconds','return posixStatus(::usleep(microseconds));', '''HANDLE timer = ::CreateWaitableTimerW(nullptr, TRUE, nullptr);
if (!timer) return winFailure();
LARGE_INTEGER due; due.QuadPart = -static_cast<LONGLONG>(microseconds ? microseconds : 1) * 10;
if (!::SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE)) {
    const DWORD e = ::GetLastError(); ::CloseHandle(timer); return winFailure(e);
}
const DWORD result = ::WaitForSingleObject(timer, INFINITE);
const DWORD e = result == WAIT_FAILED ? ::GetLastError() : ERROR_SUCCESS;
::CloseHandle(timer);
return result == WAIT_OBJECT_0 ? ok() : winFailure(e ? e : ERROR_INVALID_FUNCTION);''',api='doSleepMicroseconds',note='Observed test uses 100000 microseconds. Windows wait is not POSIX signal-interruptible; zero is rounded to a minimum timer interval here. Scheduler precision is not promised.')
simple('strptime','Result<std::tm>','const std::string& text', '''std::tm value{};
if (!::strptime(text.c_str(), "%a, %d %b %Y %H:%M:%S", &value)) return posixFailure<std::tm>(EINVAL);
return ok(value);''', '''std::tm value{};
std::istringstream input(text);
input.imbue(std::locale::classic());
input >> std::get_time(&value, "%a, %d %b %Y %H:%M:%S");
if (input.fail()) return posixFailure<std::tm>(EINVAL);
return ok(value);''',api='doParseHttpCalendar',note='Includes <sstream>, <locale>, <iomanip> on Windows. Native strptime has no UCRT equivalent; this is a standard-library parser, not a syscall. Locale/accepted grammar and current mktime timezone behavior require HTTP-date tests before adoption.')
# Actual terminal scope used by CursesUi::readKey: ICANON/ECHO and VMIN/VTIME.
put('tcgetattr tcsetattr','Result<TerminalState> doBeginCharacterInput(File terminal); Status doRestoreTerminal(File terminal, const TerminalState& saved);', '''struct TerminalState { termios native; };
Result<TerminalState> doBeginCharacterInput(File terminal) {
    TerminalState saved{};
    if (::tcgetattr(terminal.native, &saved.native) != 0) return posixFailure<TerminalState>();
    termios changed = saved.native;
    changed.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
    changed.c_cc[VMIN] = 1; changed.c_cc[VTIME] = 0;
    if (::tcsetattr(terminal.native, TCSANOW, &changed) != 0) return posixFailure<TerminalState>();
    return ok(saved);
}
Status doRestoreTerminal(File terminal, const TerminalState& saved) {
    return posixStatus(::tcsetattr(terminal.native, TCSANOW, &saved.native));
}''', '''struct TerminalState { DWORD native; };
Result<TerminalState> doBeginCharacterInput(File terminal) {
    TerminalState saved{};
    if (!::GetConsoleMode(terminal.native, &saved.native)) return winFailure<TerminalState>();
    const DWORD changed = saved.native & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    if (!::SetConsoleMode(terminal.native, changed)) return winFailure<TerminalState>();
    return ok(saved);
}
Status doRestoreTerminal(File terminal, const TerminalState& saved) {
    return winStatus(::SetConsoleMode(terminal.native, saved.native));
}''',note='Restore on every caller exit, including exceptions. Only character-input mode intent is shared; Windows console events/Unicode/escape sequences differ from termios bytes. Check VMIN/VTIME against CursesUi before adoption.')
# Handle passing: preserve destination and native ancillary-message ownership.
put('sendmsg CMSG_SPACE','Status doSendHandle(Socket channel, const Address& destination, File resource);', '''Status doSendHandle(Socket channel, const Address& destination, File resource) {
    char marker = 0;
    iovec payload{&marker, 1};
    alignas(cmsghdr) std::array<std::byte, CMSG_SPACE(sizeof(int))> control{};
    msghdr message{};
    message.msg_name = const_cast<sockaddr_storage*>(&destination.storage);
    message.msg_namelen = destination.length;
    message.msg_iov = &payload; message.msg_iovlen = 1;
    message.msg_control = control.data(); message.msg_controllen = control.size();
    cmsghdr* header = CMSG_FIRSTHDR(&message);
    header->cmsg_level = SOL_SOCKET; header->cmsg_type = SCM_RIGHTS;
    header->cmsg_len = CMSG_LEN(sizeof(int));
    std::memcpy(CMSG_DATA(header), &resource.native, sizeof(int));
    const auto sent = ::sendmsg(channel.native, &message, 0);
    if (sent < 0) return posixFailure();
    return sent == 1 ? ok() : posixFailure(EIO);
}''','',note='Sender retains ownership. UNIX datagram address stays inside the operation. No documented Windows SCM_RIGHTS counterpart; WSASendMsg does not transfer handles. A DuplicateHandle/WSADuplicateSocket protocol needs recipient identity and transport changes.')
put('recvmsg CMSG_LEN CMSG_FIRSTHDR CMSG_DATA','Result<File> doReceiveHandle(Socket channel);', '''Result<File> doReceiveHandle(Socket channel) {
    char marker;
    iovec payload{&marker, 1};
    alignas(cmsghdr) std::array<std::byte, CMSG_SPACE(16 * sizeof(int))> control{};
    msghdr message{};
    message.msg_iov = &payload; message.msg_iovlen = 1;
    message.msg_control = control.data(); message.msg_controllen = control.size();
    const auto received = ::recvmsg(channel.native, &message, 0);
    if (received < 0) return posixFailure<File>();
    std::array<int, 16> fds{};
    std::size_t count = 0;
    bool malformed = false;
    for (auto* header = CMSG_FIRSTHDR(&message); header; header = CMSG_NXTHDR(&message, header)) {
        if (header->cmsg_level != SOL_SOCKET || header->cmsg_type != SCM_RIGHTS) continue;
        const auto* begin = reinterpret_cast<const std::byte*>(header);
        const auto available = static_cast<std::size_t>(control.data() + message.msg_controllen - begin);
        if (header->cmsg_len < CMSG_LEN(0) || header->cmsg_len > available) { malformed = true; break; }
        const auto bytes = header->cmsg_len - CMSG_LEN(0);
        if (bytes % sizeof(int)) malformed = true;
        for (std::size_t offset = 0; offset + sizeof(int) <= bytes; offset += sizeof(int)) {
            int fd; std::memcpy(&fd, reinterpret_cast<const std::byte*>(CMSG_DATA(header)) + offset, sizeof(fd));
            if (count < fds.size()) fds[count++] = fd;
            else { ::close(fd); malformed = true; }
        }
    }
    if (received != 1 || malformed || (message.msg_flags & (MSG_CTRUNC | MSG_TRUNC)) || count != 1) {
        for (std::size_t i = 0; i < count; ++i) ::close(fds[i]);
        return posixFailure<File>(EBADMSG);
    }
    return ok(File{fds.front()});
}''','',note='Returns one new owned descriptor, closes received extras on invalid messages. Before production use: descriptor type detection and atomic close-on-exec policy must be completed and malformed/truncated-message cleanup validated. Windows transport remains unresolved; a stub is not an implementation.')
put('getpeereid','Result<PeerIdentity> doPeerIdentity(Socket channel);', '''struct PeerIdentity { std::uint64_t user, group; std::optional<std::uint64_t> process; };
Result<PeerIdentity> doPeerIdentity(Socket channel) {
    ucred credentials{}; socklen_t size = sizeof(credentials);
    if (::getsockopt(channel.native, SOL_SOCKET, SO_PEERCRED, &credentials, &size) < 0) return posixFailure<PeerIdentity>();
    return ok(PeerIdentity{credentials.uid, credentials.gid, static_cast<std::uint64_t>(credentials.pid)});
}''','', '''struct PeerIdentity { std::uint64_t user, group; std::optional<std::uint64_t> process; };
Result<PeerIdentity> doPeerIdentity(Socket channel) {
    uid_t user; gid_t group;
    if (::getpeereid(channel.native, &user, &group) < 0) return posixFailure<PeerIdentity>();
    return ok(PeerIdentity{user, group, std::nullopt});
}''',note='Reshape existing net::un::peerCredentials. Windows named-pipe impersonation authenticates a different transport and returns SIDs; this POSIX numeric identity output is deliberately not claimed portable.')
# POSIX-only operations: real bodies, not fictitious Windows substitutes.
# These rows explicitly require a larger portable operation before caller migration.
for name,ret,args,body in [
 ('open','Result<File>','const Path& path, int flags, std::uint32_t mode','const int fd = ::open(path.c_str(), flags, static_cast<mode_t>(mode));\nif (fd == -1) return posixFailure<File>();\nreturn ok(File{fd});'),
 ('openat','Result<File>','File directory, const Path& path, int flags, std::uint32_t mode','const int fd = ::openat(directory.native, path.c_str(), flags, static_cast<mode_t>(mode));\nif (fd == -1) return posixFailure<File>();\nreturn ok(File{fd});'),
 ('mkstemp','Result<File>','std::string& pattern','const int fd = ::mkstemp(pattern.data());\nif (fd == -1) return posixFailure<File>();\nreturn ok(File{fd});'),
 ('mkdtemp','Status','std::string& pattern','return ::mkdtemp(pattern.data()) ? ok() : posixFailure();'),
 ('dup2','Status','File from, int target','return posixStatus(::dup2(from.native, target));'),
 ('pipe2','Result<Pipe>','int flags','int fds[2];\nif (::pipe2(fds, flags) != 0) return posixFailure<Pipe>();\nreturn ok(Pipe{File{fds[0]}, File{fds[1]}});'),
 ('accept4','Result<Socket>','Socket listener, int flags','const int fd = ::accept4(listener.native, nullptr, nullptr, flags);\nif (fd == -1) return posixFailure<Socket>();\nreturn ok(Socket{fd});'),
 ('socketpair','Result<std::pair<Socket, Socket>>','','int fds[2];\nif (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) return posixFailure<std::pair<Socket, Socket>>();\nreturn ok(std::pair<Socket, Socket>{Socket{fds[0]}, Socket{fds[1]}});'),
 ('epoll_create1','Result<File>','','const int fd = ::epoll_create1(EPOLL_CLOEXEC);\nif (fd == -1) return posixFailure<File>();\nreturn ok(File{fd});'),
 ('epoll_ctl','Status','File queue, int operation, int descriptor, epoll_event* event','return posixStatus(::epoll_ctl(queue.native, operation, descriptor, event));'),
 ('epoll_wait','Result<int>','File queue, std::span<epoll_event> events, int timeoutMilliseconds','if (events.size() > INT_MAX) return posixFailure<int>(EINVAL);\nconst int count = ::epoll_wait(queue.native, events.data(), static_cast<int>(events.size()), timeoutMilliseconds);\nif (count < 0) return posixFailure<int>();\nreturn ok(count);'),
 ('epoll_pwait','Result<int>','File queue, std::span<epoll_event> events, int timeoutMilliseconds, const sigset_t* mask','if (events.size() > INT_MAX) return posixFailure<int>(EINVAL);\nconst int count = ::epoll_pwait(queue.native, events.data(), static_cast<int>(events.size()), timeoutMilliseconds, mask);\nif (count < 0) return posixFailure<int>();\nreturn ok(count);'),
 ('poll','Result<int>','std::span<pollfd> items, int timeoutMilliseconds','const int count = ::poll(items.data(), items.size(), timeoutMilliseconds);\nif (count < 0) return posixFailure<int>();\nreturn ok(count);'),
 ('ppoll','Result<int>','std::span<pollfd> items, const timespec* timeout, const sigset_t* mask','const int count = ::ppoll(items.data(), items.size(), timeout, mask);\nif (count < 0) return posixFailure<int>();\nreturn ok(count);'),
 ('select','Result<int>','int count, fd_set* read, fd_set* write, fd_set* error, timeval* timeout','const int result = ::select(count, read, write, error, timeout);\nif (result < 0) return posixFailure<int>();\nreturn ok(result);'),
 ('pselect','Result<int>','int count, fd_set* read, fd_set* write, fd_set* error, const timespec* timeout, const sigset_t* mask','const int result = ::pselect(count, read, write, error, timeout, mask);\nif (result < 0) return posixFailure<int>();\nreturn ok(result);'),
 ('sigaction','Status','int signal, const struct sigaction* action, struct sigaction* previous','return posixStatus(::sigaction(signal, action, previous));'),
 ('signal','Result<void (*)(int)>','int signal, void (*handler)(int)','const auto previous = ::signal(signal, handler);\nif (previous == SIG_ERR) return posixFailure<void (*)(int)>();\nreturn ok(previous);'),
 ('pthread_sigmask','Status','int operation, const sigset_t* mask, sigset_t* previous','const int error = ::pthread_sigmask(operation, mask, previous);\nreturn error ? posixFailure(error) : ok();'),
 ('sigemptyset','Status','sigset_t& set','return posixStatus(::sigemptyset(&set));'),
 ('sigaddset','Status','sigset_t& set, int signal','return posixStatus(::sigaddset(&set, signal));'),
 ('sigfillset','Status','sigset_t& set','return posixStatus(::sigfillset(&set));'),
 ('strsignal','std::string','int signal','const char* text = ::strsignal(signal);\nreturn text ? text : "unknown signal";'),
 ('sigabbrev_np','std::string','int signal','return utils::system::sigabbrev_np(signal);'),
 ('getpgrp','std::uint64_t','','return static_cast<std::uint64_t>(::getpgrp());'),
 ('tcsetpgrp','Status','File terminal, std::uint64_t processGroup','return posixStatus(::tcsetpgrp(terminal.native, static_cast<pid_t>(processGroup)));'),
 ('kill','Status','std::uint64_t process, int signal','if (process == 0 || process > static_cast<std::uint64_t>(std::numeric_limits<pid_t>::max())) return posixFailure(EINVAL);\nreturn posixStatus(::kill(static_cast<pid_t>(process), signal));'),
 ('syscall:SYS_pidfd_open','Result<File>','std::uint64_t process','if (process == 0 || process > static_cast<std::uint64_t>(std::numeric_limits<pid_t>::max())) return posixFailure<File>(EINVAL);\nconst int fd = static_cast<int>(::syscall(SYS_pidfd_open, static_cast<pid_t>(process), 0));\nif (fd < 0) return posixFailure<File>();\nreturn ok(File{fd});'),
 ('waitpid','Result<ExitStatus>','std::uint64_t process','int status;\nif (process == 0 || process > static_cast<std::uint64_t>(std::numeric_limits<pid_t>::max())) return posixFailure<ExitStatus>(EINVAL);\nif (::waitpid(static_cast<pid_t>(process), &status, 0) < 0) return posixFailure<ExitStatus>();\nif (WIFEXITED(status)) return ok(ExitStatus{false, static_cast<std::uint32_t>(WEXITSTATUS(status)), 0});\nif (WIFSIGNALED(status)) return ok(ExitStatus{true, 0, WTERMSIG(status)});\nreturn posixFailure<ExitStatus>(ECHILD);')]:
    method='do'+''.join(word.title() for word in name.replace(':','_').split('_'))
    simple(name,ret,args,body,None,api=method,note='POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.')
# Mac has no native epoll, ppoll, pipe2, accept4 or pidfd_open.
for name in 'epoll_create1 epoll_ctl epoll_wait epoll_pwait ppoll pipe2 accept4 syscall:SYS_pidfd_open'.split(): ENTRIES[name]['mac']=''
# Whole daemon body, preserving its current exception/caller protocol.
daemon=(ROOT/'src/utils/Daemon.cpp').read_text()
a=daemon.index('    void Daemon::startDaemon('); b=daemon.index('\n    pid_t Daemon::stopDaemon',a)
body=daemon[:daemon.index('#include')] + '\nnamespace utils {\n' + daemon[a:b].replace('void Daemon::startDaemon(', 'void doStartDaemon(') + '\n}\n'
put('fork setsid','void doStartDaemon(const std::string& pidFileName, const std::string& userName, const std::string& groupName);',body,'',note='Whole existing Daemon::startDaemon operation, placed in namespace utils and using existing DaemonError/DaemonFailure/DaemonSignaled from utils/Daemon.h. Requires its current includes. Extracted verbatim except name; does not fix current PID-publication races. Windows cannot resume this C++ continuation after CreateProcessW; a re-exec/service startup contract is still required. Fork-based tests need a separate launched-child fixture.')
# Existing ProcessRunner already has the correct operation boundary. Preserve it.
process=(ROOT/'src/tools/snodec-control/src/ProcessRunner.cpp').read_text()
process_path=OUT/'snippets/process-posix.cpp.txt'
process_path.write_text(process)
spawn_names='posix_spawn posix_spawn_file_actions_init posix_spawn_file_actions_adddup2 posix_spawn_file_actions_addclose posix_spawn_file_actions_destroy posix_spawnattr_init posix_spawnattr_setpgroup posix_spawnattr_setsigdefault posix_spawnattr_setsigmask posix_spawnattr_setflags posix_spawnattr_destroy'
put(spawn_names,'''ProcessResult runProcess(const std::string& executable, const std::vector<std::string>& arguments);
ProcessResult runProcessAttached(const std::string& executable, const std::vector<std::string>& arguments);
ProcessResult runProcessAttachedAsForegroundJob(const std::string& executable, const std::vector<std::string>& arguments);''',process,'',note='Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.')
# Stable process resources, rather than casting Windows handles to PIDs/ints.
put('waitpid','Result<ExitStatus> doWaitProcess(Process process);', '''Result<ExitStatus> doWaitProcess(Process process) {
    int status;
    if (::waitpid(process.native, &status, 0) < 0) return posixFailure<ExitStatus>();
    if (WIFEXITED(status)) return ok(ExitStatus{false, static_cast<std::uint32_t>(WEXITSTATUS(status)), 0});
    if (WIFSIGNALED(status)) return ok(ExitStatus{true, 0, WTERMSIG(status)});
    return posixFailure<ExitStatus>(ECHILD);
}''', '''Result<ExitStatus> doWaitProcess(Process process) {
    const DWORD state = ::WaitForSingleObject(process.native, INFINITE);
    if (state == WAIT_FAILED) return winFailure<ExitStatus>();
    if (state != WAIT_OBJECT_0) return winFailure<ExitStatus>(ERROR_INVALID_FUNCTION);
    DWORD code;
    if (!::GetExitCodeProcess(process.native, &code)) return winFailure<ExitStatus>();
    return ok(ExitStatus{false, code, 0});
}''',note='POSIX reaps this child once; Windows process handle is still owned and must be closed separately. Windows signaled=false means no POSIX signal classification, not proof of graceful termination. No eight-bit truncation.')
put('kill','Status doForceTerminate(Process process);', '''Status doForceTerminate(Process process) {
    return posixStatus(::kill(process.native, SIGKILL));
}''', '''Status doForceTerminate(Process process) {
    return winStatus(::TerminateProcess(process.native, 1));
}''',note='Forced termination only; owned valid Process required. Graceful SIGTERM and terminal SIGINT need distinct doRequestStop(process,reason) policy, not TerminateProcess. Their source occurrences remain unresolved.')
put('syscall:SYS_pidfd_open','Result<ProcessWatch> doWatchProcess(std::uint64_t process);', '''Result<ProcessWatch> doWatchProcess(std::uint64_t process) {
    if (!process || process > static_cast<std::uint64_t>(std::numeric_limits<pid_t>::max())) return posixFailure<ProcessWatch>(EINVAL);
    const int descriptor = static_cast<int>(::syscall(SYS_pidfd_open, static_cast<pid_t>(process), 0));
    if (descriptor < 0) return posixFailure<ProcessWatch>();
    return ok(ProcessWatch{descriptor});
}''', '''Result<ProcessWatch> doWatchProcess(std::uint64_t process) {
    if (!process || process > MAXDWORD) return winFailure<ProcessWatch>(ERROR_INVALID_PARAMETER);
    HANDLE handle = ::OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(process));
    if (!handle) return winFailure<ProcessWatch>();
    return ok(ProcessWatch{handle});
}''', '''Result<ProcessWatch> doWatchProcess(std::uint64_t process) {
    if (!process || process > static_cast<std::uint64_t>(std::numeric_limits<pid_t>::max())) return posixFailure<ProcessWatch>(EINVAL);
    const int queue = ::kqueue();
    if (queue == -1) return posixFailure<ProcessWatch>();
    struct kevent change;
    EV_SET(&change, process, EVFILT_PROC, EV_ADD | EV_ONESHOT, NOTE_EXIT, 0, nullptr);
    if (::kevent(queue, &change, 1, nullptr, 0, nullptr) < 0) {
        const int error = errno; ::close(queue); return posixFailure<ProcessWatch>(error);
    }
    return ok(ProcessWatch{queue});
}''',note='Includes <sys/syscall.h> on Linux, <sys/event.h> on macOS. Owned watch, closed with close/CloseHandle. PID reuse between reading a PID file and acquiring the native reference remains possible on every target; macOS registration behavior needs tests. Waiting on the reference is a separate operation.')
# Socket-only poll implementation; generic mixed-resource wait remains a separate gap.
put('poll','Result<int> doPollSockets(std::span<PollItem> items, int timeoutMilliseconds);', '''Result<int> doPollSockets(std::span<PollItem> items, int timeoutMilliseconds) {
    std::vector<pollfd> native;
    for (const auto& item : items) native.push_back({item.socket.native, static_cast<short>((item.read ? POLLIN : 0) | (item.write ? POLLOUT : 0) | (item.urgent ? POLLPRI : 0)), 0});
    const int count = ::poll(native.data(), native.size(), timeoutMilliseconds);
    if (count < 0) return posixFailure<int>();
    for (std::size_t i = 0; i < items.size(); ++i) {
        const short events = native[i].revents;
        items[i].returned = static_cast<short>((events & POLLIN ? 1 : 0) | (events & POLLOUT ? 2 : 0) | (events & POLLPRI ? 4 : 0) | (events & POLLERR ? 8 : 0) | (events & POLLHUP ? 16 : 0) | (events & POLLNVAL ? 32 : 0));
    }
    return ok(count);
}''', '''Result<int> doPollSockets(std::span<PollItem> items, int timeoutMilliseconds) {
    if (items.empty() || items.size() > ULONG_MAX) return wsaFailure<int>(WSAEINVAL);
    std::vector<WSAPOLLFD> native;
    for (const auto& item : items) {
        if (item.urgent) return wsaFailure<int>(WSAEOPNOTSUPP);
        native.push_back({item.socket.native, static_cast<short>((item.read ? POLLRDNORM : 0) | (item.write ? POLLWRNORM : 0)), 0});
    }
    const int count = ::WSAPoll(native.data(), static_cast<ULONG>(native.size()), timeoutMilliseconds);
    if (count == SOCKET_ERROR) return wsaFailure<int>();
    for (std::size_t i = 0; i < items.size(); ++i) {
        const short events = native[i].revents;
        items[i].returned = static_cast<short>((events & POLLRDNORM ? 1 : 0) | (events & POLLWRNORM ? 2 : 0) | (events & POLLERR ? 8 : 0) | (events & POLLHUP ? 16 : 0) | (events & POLLNVAL ? 32 : 0));
    }
    return ok(count);
}''',note='Normalized returned bits: read=1, write=2, urgent=4, error=8, hangup=16, invalid=32. Microsoft WSAPoll cannot supply POLLPRI; explicit error is a capability gap, not completed compatibility. This variant cannot replace pipe/pidfd/mixed-handle polling or atomic signal-mask waits.')
# Generic operation/native gaps are recorded rather than emitted as fake success stubs.
BLOCKERS={
 'open':'CreateFileW body still needs all observed creation/no-follow/nonblocking/inheritance/0600 ACL semantics, plus synchronous versus positioned-I/O handle choice. A plain CreateFileW(...OPEN_EXISTING...) would omit real callers.',
 'openat':'NtCreateFile with OBJECT_ATTRIBUTES.RootDirectory is the native building block. A handle-relative, no-follow implementation and NTSTATUS/path conversion remain required; concatenating paths is not equivalent.',
 'mkstemp':'CreateFileW(CREATE_NEW) with unpredictable candidate names and explicit private security descriptor must be implemented with bounded collision handling and cleanup.',
 'mkdtemp':'CreateDirectoryW with private ACL and unpredictable name/collision handling is required. GetTempFileNameW does not implement private temporary-directory semantics.',
 'dup2':'Use explicit child standard-handle redirection within the launch operation. Numeric POSIX descriptor-slot replacement has no general HANDLE/SOCKET equivalent; _dup2 covers CRT descriptors only.',
 'pipe2':'CreateNamedPipeW/ConnectNamedPipe/CreateFileW with OVERLAPPED lifecycle for event-loop pipes; CreatePipe body above only covers synchronous pipes. On macOS pipe+fcntl cannot alone prove atomic close-on-exec against concurrent spawn.',
 'accept4':'accept+ioctlsocket+SetHandleInformation on Windows, accept+fcntl on macOS. Full cleanup and concurrent-spawn noninheritance semantics still need an integrated accept capsule.',
 'socketpair':'No native socketpair. A Windows TCP loopback socket/bind/listen/connect/accept composition needs endpoint verification, cleanup and explicit stream-only semantics.',
 'epoll_create1':'Replace the whole event registration/wait boundary. Windows socket events/overlapped events and macOS kqueue have different native object models; no epoll descriptor can be returned.',
 'epoll_ctl':'Typed registration on kqueue/kevent or Windows WSAEventSelect plus non-socket completion state. Exposing epoll_event in the project API cannot implement all three targets.',
 'epoll_wait':'kevent on macOS; Windows WaitForMultipleObjects/WSAEnumNetworkEvents plus overlapped completions. Mixed descriptor kinds, nesting, event reset and scaling need the complete existing multiplexer boundary.',
 'epoll_pwait':'No direct equivalent atomic signal-mask/event wait. Windows signal delivery and macOS kqueue+pselect behavior must be designed as one no-lost-wakeup operation.',
 'ppoll':'No Windows POSIX mask wait and no audited Darwin ppoll entry. A signal-aware readiness operation must replace the full surrounding sequence.',
 'select':'Winsock select accepts sockets only; it cannot drain ProcessRunner pipes. Use doPollSockets for the socket subset and a full child-capture operation for pipes.',
 'pselect':'No Windows equivalent of atomic per-thread POSIX mask switching during descriptor wait.',
 'sigaction':'Windows console-control callbacks and limited CRT signals cannot reproduce arbitrary POSIX sigaction. Propose lifecycle-event subscriptions at the whole signal owner boundary.',
 'signal':'Limited CRT signal exists; the observed SIGHUP/SIGPIPE/SIGCHLD/job-control uses need separate explicit lifecycle policies. No generic same-number mapping.',
 'pthread_sigmask':'No Windows per-thread POSIX signal mask; do not pretend an event wait emulates mask semantics.',
 'sigemptyset':'SignalSet must be a semantic lifecycle set once Windows delivery policy is defined; no Windows syscall for this pure value operation.',
 'sigaddset':'Same semantic SignalSet as the whole signal-delivery capsule; raw POSIX signal numbers do not port.',
 'sigfillset':'All supported lifecycle events must be defined; Windows exceptions are not a POSIX full signal set.',
 'strsignal':'Define text for the chosen semantic lifecycle events; no strsignal Win32 syscall.',
 'sigabbrev_np':'Existing utils::system::sigabbrev_np is the single project mapping. Reshape that mapping for semantic events; do not create three competing maps.',
 'getpgrp':'No Windows POSIX foreground process group. Encapsulate the entire attached foreground job operation.',
 'tcsetpgrp':'No equivalent Windows foreground-terminal process-group syscall. Console attachment/control-event ownership needs a whole attached-job policy.',
}
for name,entry in ENTRIES.items():
    entry['linux']=entry['linux'].replace('\x00',r'\0')
    entry['windows']=entry['windows'].replace('\x00',r'\0')
    entry['mac']=entry['mac'].replace('\x00',r'\0')
