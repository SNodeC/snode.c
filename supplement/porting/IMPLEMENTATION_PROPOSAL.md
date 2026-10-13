# System-call capsule implementation proposal

This is the requested five-column proposal: syscall/source operation, proposed callable API, Linux body, Windows body, macOS body. Bodies are included in the cells and also saved as plain C++ text for copying. [Open the wide HTML table](IMPLEMENTATION_PROPOSAL.html) for readable side-by-side code.

**First draft, not a completed three-platform port.** Every one of the 163 inventoried native/runtime/helper entries appears once. Some bodies implement only a stated subset; missing bodies are explicit implementation gaps, not successful unsupported stubs. An entry in this table does not imply that all its source uses can already migrate. No backend has been validated in QEMU. These snippets are documentation, outside the production build.

Use [common declarations](snippets/common.hpp.txt), [Windows text conversion](snippets/windows-text.hpp.txt) where needed, and the per-operation includes listed with the row. Select one platform body for each API; repeated bodies represent consolidation, not functions to paste several times. Code is C++20. Put bare function bodies inside `namespace core::system`; full files already containing a namespace retain theirs. Include the support declarations once. Native SDK declarations inside backend signatures are explicitly marked as unfinished portable boundaries.

The architectural invariant is one existing owner for resource representation, lifetime and OS behavior. Small wrappers stay under the existing system owners; configuration, child execution, descriptor transfer, terminal state and daemon startup are operation capsules. Do not create a parallel descriptor registry or wrap every line of a sequence independently. No production implementation is authorized by adding these documentation files.

`Result<T>` captures native error domain/code immediately. Callers must migrate their errno branches to the captured error; partial I/O remains observable. Signed `Count` avoids disguising failure as an unsigned byte count. The support declarations are concrete proposal types, not types that already exist in SNode.C. Paths are `std::filesystem::path`; use an explicit UTF-8 conversion at string boundaries. Native flag values and native address construction still appearing in some rows are adoption gaps, not a finished portable API.

The source evidence and extraction status remain in [the complete inventory](SYSCALL_INVENTORY.md), [call sites](CALL_SITES.md), [library calls](LIBRARY_CALLS.md), and [platform dependencies](PLATFORM_DEPENDENCIES.md). Earlier API-only declarations are superseded by this implementation draft where signatures differ. [Implementation gaps](IMPLEMENTATION_GAPS.md) lists all empty cells and preserves the per-row restrictions. Each source call links to its actual occurrences.

The longer Config.cpp capsule is also directly available as [types](snippets/config-types.hpp.txt), [Linux](snippets/config-linux.cpp.txt), [Windows](snippets/config-windows.cpp.txt), [macOS](snippets/config-mac.cpp.txt), and [replacement caller](snippets/config-caller.cpp.txt). It replaces account lookup, directory policy, directory creation, permissions and group ownership together. Windows uses SIDs and creation-time ACLs; its LocalSystem/known-folder policy is proposed behavior requiring review, not an exact UID/GID emulation. The POSIX variant retains existing directory locations on macOS intentionally; adopting Apple-specific locations would be a separate policy change.

Native API references: [Windows ReadFile](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile), [file information](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getfileinformationbyhandleex), [known folders](https://learn.microsoft.com/en-us/windows/win32/shell/known-folders), [Winsock](https://learn.microsoft.com/en-us/windows/win32/api/winsock2/), [CreateProcessW](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw), [NtCreateFile](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntcreatefile), [Darwin native entries](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/kern/syscalls.master), [Apple kevent](https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/kevent.2.html). These establish building blocks, not equivalence or successful execution of these bodies.


## Files and descriptors

<table>
<thead><tr><th>syscall on POSIX/Linux</th><th>proposed API</th><th>implementation Linux</th><th>implementation Windows</th><th>implementation macOS</th></tr></thead>
<tbody>
<tr id="open"><td><a href="CALL_SITES.md#open"><code>open</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::open`; inline in framework, test</p><p>Options include creation mode, exclusive creation, no-follow, nonblocking and inheritance; UTF-8 Path. Creation/security flags are part of the contract.</p></details></td><td><pre><code>Result&lt;File&gt; doOpen(const Path&amp; path, int flags, std::uint32_t mode);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-fed5d31776e9.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doOpen(const Path&amp; path, int flags, std::uint32_t mode) {
    const int fd = ::open(path.c_str(), flags, static_cast&lt;mode_t&gt;(mode));
    if (fd == -1) return posixFailure&lt;File&gt;();
    return ok(File{fd});
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>CreateFileW body still needs all observed creation/no-follow/nonblocking/inheritance/0600 ACL semantics, plus synchronous versus positioned-I/O handle choice. A plain CreateFileW(...OPEN_EXISTING...) would omit real callers.</p><p>Native building blocks: CreateFileW</p></td><td><a href="snippets/mac-fed5d31776e9.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doOpen(const Path&amp; path, int flags, std::uint32_t mode) {
    const int fd = ::open(path.c_str(), flags, static_cast&lt;mode_t&gt;(mode));
    if (fd == -1) return posixFailure&lt;File&gt;();
    return ok(File{fd});
}</code></pre></details></td></tr>
<tr id="openat"><td><a href="CALL_SITES.md#openat"><code>openat</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `core::system::openat`</p><p>Preserve descriptor-relative lookup and absolute-path behavior; joining directory strings is not equivalent. NT API policy needs review.</p></details></td><td><pre><code>Result&lt;File&gt; doOpenat(File directory, const Path&amp; path, int flags, std::uint32_t mode);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-3a0153817c2c.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doOpenat(File directory, const Path&amp; path, int flags, std::uint32_t mode) {
    const int fd = ::openat(directory.native, path.c_str(), flags, static_cast&lt;mode_t&gt;(mode));
    if (fd == -1) return posixFailure&lt;File&gt;();
    return ok(File{fd});
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>NtCreateFile with OBJECT_ATTRIBUTES.RootDirectory is the native building block. A handle-relative, no-follow implementation and NTSTATUS/path conversion remain required; concatenating paths is not equivalent.</p><p>Native building blocks: NtCreateFile with OBJECT_ATTRIBUTES.RootDirectory</p></td><td><a href="snippets/mac-3a0153817c2c.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doOpenat(File directory, const Path&amp; path, int flags, std::uint32_t mode) {
    const int fd = ::openat(directory.native, path.c_str(), flags, static_cast&lt;mode_t&gt;(mode));
    if (fd == -1) return posixFailure&lt;File&gt;();
    return ok(File{fd});
}</code></pre></details></td></tr>
<tr id="close"><td><a href="CALL_SITES.md#close"><code>close</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::close`; inline in framework, test, tool</p><p>Consume ownership once; native resource kind is hidden; no caller retry after interrupted close.</p></details></td><td><pre><code>Status doClose(File f); Status doClose(Socket s);</code></pre><p>One close attempt, owner retires the reference. Do not retry on EINTR; final OS-specific release semantics require target validation.</p></td><td><a href="snippets/linux-7d708c59c602.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doClose(File f) { return posixStatus(::close(f.native)); }
Status doClose(Socket s) { return posixStatus(::close(s.native)); }</code></pre></details></td><td><a href="snippets/windows-646ec0d501ec.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doClose(File f) { return winStatus(::CloseHandle(f.native)); }
Status doClose(Socket s) { return socketStatus(::closesocket(s.native)); }</code></pre></details></td><td><a href="snippets/mac-7d708c59c602.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doClose(File f) { return posixStatus(::close(f.native)); }
Status doClose(Socket s) { return posixStatus(::close(s.native)); }</code></pre></details></td></tr>
<tr id="read"><td><a href="CALL_SITES.md#read"><code>read</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::read`; inline in demo, framework, test, tool</p><p>Borrow buffer; distinguish bytes, EOF, would-block and interrupted. No automatic retry or read-exactly loop.</p></details></td><td><pre><code>Result&lt;Count&gt; doRead(File f, MutableBytes b);</code></pre><p>One native call; partial count retained. Zero-length request returns zero without implying EOF. Windows body requires synchronous file/byte-pipe handles; it does not implement nonblocking event-loop pipe I/O. Socket I/O uses doRecv/doSend. POSIX write retains existing SIGPIPE policy; suppressing it is a separate lifecycle decision.</p></td><td><a href="snippets/linux-8394a078867d.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Count&gt; doRead(File f, MutableBytes b) {
    const auto n = std::min(b.size(), static_cast&lt;std::size_t&gt;(PTRDIFF_MAX));
    const auto r = ::read(f.native, b.data(), n);
    if (r &lt; 0) return posixFailure&lt;Count&gt;();
    return ok(static_cast&lt;Count&gt;(r));
}</code></pre></details></td><td><a href="snippets/windows-5e414e02d49c.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Count&gt; doRead(File f, MutableBytes b) {
    const auto n = static_cast&lt;DWORD&gt;(std::min(b.size(),
        std::min(static_cast&lt;std::size_t&gt;(MAXDWORD), static_cast&lt;std::size_t&gt;(PTRDIFF_MAX))));
    DWORD done = 0;
    if (!::ReadFile(f.native, b.data(), n, &amp;done, nullptr)) {
        const DWORD e = ::GetLastError();
        if (e == ERROR_BROKEN_PIPE || e == ERROR_HANDLE_EOF) return ok(Count{0});
        return winFailure&lt;Count&gt;(e);
    }
    return ok(static_cast&lt;Count&gt;(done));
}</code></pre></details></td><td><a href="snippets/mac-8394a078867d.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Count&gt; doRead(File f, MutableBytes b) {
    const auto n = std::min(b.size(), static_cast&lt;std::size_t&gt;(PTRDIFF_MAX));
    const auto r = ::read(f.native, b.data(), n);
    if (r &lt; 0) return posixFailure&lt;Count&gt;();
    return ok(static_cast&lt;Count&gt;(r));
}</code></pre></details></td></tr>
<tr id="write"><td><a href="CALL_SITES.md#write"><code>write</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::write`; inline in test, tool</p><p>Preserve partial writes, would-block, broken pipe and interruption without process termination from SIGPIPE.</p></details></td><td><pre><code>Result&lt;Count&gt; doWrite(File f, Bytes b);</code></pre><p>One native call; partial count retained. Zero-length request returns zero without implying EOF. Windows body requires synchronous file/byte-pipe handles; it does not implement nonblocking event-loop pipe I/O. Socket I/O uses doRecv/doSend. POSIX write retains existing SIGPIPE policy; suppressing it is a separate lifecycle decision.</p></td><td><a href="snippets/linux-e94f9cea110c.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Count&gt; doWrite(File f, Bytes b) {
    const auto n = std::min(b.size(), static_cast&lt;std::size_t&gt;(PTRDIFF_MAX));
    const auto r = ::write(f.native, b.data(), n);
    if (r &lt; 0) return posixFailure&lt;Count&gt;();
    return ok(static_cast&lt;Count&gt;(r));
}</code></pre></details></td><td><a href="snippets/windows-7f02e2b59f5f.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Count&gt; doWrite(File f, Bytes b) {
    const auto n = static_cast&lt;DWORD&gt;(std::min(b.size(),
        std::min(static_cast&lt;std::size_t&gt;(MAXDWORD), static_cast&lt;std::size_t&gt;(PTRDIFF_MAX))));
    DWORD done = 0;
    if (!::WriteFile(f.native, b.data(), n, &amp;done, nullptr)) {
        const DWORD e = ::GetLastError();
        return winFailure&lt;Count&gt;(e);
    }
    return ok(static_cast&lt;Count&gt;(done));
}</code></pre></details></td><td><a href="snippets/mac-e94f9cea110c.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Count&gt; doWrite(File f, Bytes b) {
    const auto n = std::min(b.size(), static_cast&lt;std::size_t&gt;(PTRDIFF_MAX));
    const auto r = ::write(f.native, b.data(), n);
    if (r &lt; 0) return posixFailure&lt;Count&gt;();
    return ok(static_cast&lt;Count&gt;(r));
}</code></pre></details></td></tr>
<tr id="pread"><td><a href="CALL_SITES.md#pread"><code>pread</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Read without changing the shared file position; preserve partial completion.</p></details></td><td><pre><code>Result&lt;Count&gt; doPread(File f, MutableBytes b, std::uint64_t offset);</code></pre><p>Windows file MUST have been opened FILE_FLAG_OVERLAPPED; doRead/doWrite synchronous bodies cannot operate on that same handle. This distinction must be integrated into doOpen and resource ownership before copying into production. This operation waits; it is not an event-loop submission API.</p></td><td><a href="snippets/linux-599418672878.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Count&gt; doPread(File f, MutableBytes b, std::uint64_t offset) {
    if (offset &gt; static_cast&lt;std::uint64_t&gt;(std::numeric_limits&lt;off_t&gt;::max())) return posixFailure&lt;Count&gt;(EOVERFLOW);
    const auto r = ::pread(f.native, b.data(), std::min(b.size(), static_cast&lt;std::size_t&gt;(PTRDIFF_MAX)), static_cast&lt;off_t&gt;(offset));
    if (r &lt; 0) return posixFailure&lt;Count&gt;();
    return ok(static_cast&lt;Count&gt;(r));
}</code></pre></details></td><td><a href="snippets/windows-f2518fe3b909.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Count&gt; doPread(File f, MutableBytes b, std::uint64_t offset) {
    if (offset &gt; static_cast&lt;std::uint64_t&gt;(INT64_MAX)) return winFailure&lt;Count&gt;(ERROR_INVALID_PARAMETER);
    OVERLAPPED operation{};
    operation.Offset = static_cast&lt;DWORD&gt;(offset); operation.OffsetHigh = static_cast&lt;DWORD&gt;(offset &gt;&gt; 32);
    operation.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!operation.hEvent) return winFailure&lt;Count&gt;();
    DWORD done = 0;
    const DWORD n = static_cast&lt;DWORD&gt;(std::min(b.size(), std::min(static_cast&lt;std::size_t&gt;(MAXDWORD), static_cast&lt;std::size_t&gt;(PTRDIFF_MAX))));
    BOOL success = ::ReadFile(f.native, b.data(), n, &amp;done, &amp;operation);
    DWORD e = success ? ERROR_SUCCESS : ::GetLastError();
    if (!success &amp;&amp; e == ERROR_IO_PENDING) {
        success = ::GetOverlappedResult(f.native, &amp;operation, &amp;done, TRUE);
        e = success ? ERROR_SUCCESS : ::GetLastError();
    }
    ::CloseHandle(operation.hEvent);
    if (!success) { if (e == ERROR_HANDLE_EOF) return ok(Count{0}); return winFailure&lt;Count&gt;(e); }
    return ok(static_cast&lt;Count&gt;(done));
}</code></pre></details></td><td><a href="snippets/mac-599418672878.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Count&gt; doPread(File f, MutableBytes b, std::uint64_t offset) {
    if (offset &gt; static_cast&lt;std::uint64_t&gt;(std::numeric_limits&lt;off_t&gt;::max())) return posixFailure&lt;Count&gt;(EOVERFLOW);
    const auto r = ::pread(f.native, b.data(), std::min(b.size(), static_cast&lt;std::size_t&gt;(PTRDIFF_MAX)), static_cast&lt;off_t&gt;(offset));
    if (r &lt; 0) return posixFailure&lt;Count&gt;();
    return ok(static_cast&lt;Count&gt;(r));
}</code></pre></details></td></tr>
<tr id="pwrite"><td><a href="CALL_SITES.md#pwrite"><code>pwrite</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Write without changing the shared file position; preserve partial completion.</p></details></td><td><pre><code>Result&lt;Count&gt; doPwrite(File f, Bytes b, std::uint64_t offset);</code></pre><p>Windows file MUST have been opened FILE_FLAG_OVERLAPPED; doRead/doWrite synchronous bodies cannot operate on that same handle. This distinction must be integrated into doOpen and resource ownership before copying into production. This operation waits; it is not an event-loop submission API.</p></td><td><a href="snippets/linux-3804440c46fa.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Count&gt; doPwrite(File f, Bytes b, std::uint64_t offset) {
    if (offset &gt; static_cast&lt;std::uint64_t&gt;(std::numeric_limits&lt;off_t&gt;::max())) return posixFailure&lt;Count&gt;(EOVERFLOW);
    const auto r = ::pwrite(f.native, b.data(), std::min(b.size(), static_cast&lt;std::size_t&gt;(PTRDIFF_MAX)), static_cast&lt;off_t&gt;(offset));
    if (r &lt; 0) return posixFailure&lt;Count&gt;();
    return ok(static_cast&lt;Count&gt;(r));
}</code></pre></details></td><td><a href="snippets/windows-e58d1eba4f07.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Count&gt; doPwrite(File f, Bytes b, std::uint64_t offset) {
    if (offset &gt; static_cast&lt;std::uint64_t&gt;(INT64_MAX)) return winFailure&lt;Count&gt;(ERROR_INVALID_PARAMETER);
    OVERLAPPED operation{};
    operation.Offset = static_cast&lt;DWORD&gt;(offset); operation.OffsetHigh = static_cast&lt;DWORD&gt;(offset &gt;&gt; 32);
    operation.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!operation.hEvent) return winFailure&lt;Count&gt;();
    DWORD done = 0;
    const DWORD n = static_cast&lt;DWORD&gt;(std::min(b.size(), std::min(static_cast&lt;std::size_t&gt;(MAXDWORD), static_cast&lt;std::size_t&gt;(PTRDIFF_MAX))));
    BOOL success = ::WriteFile(f.native, b.data(), n, &amp;done, &amp;operation);
    DWORD e = success ? ERROR_SUCCESS : ::GetLastError();
    if (!success &amp;&amp; e == ERROR_IO_PENDING) {
        success = ::GetOverlappedResult(f.native, &amp;operation, &amp;done, TRUE);
        e = success ? ERROR_SUCCESS : ::GetLastError();
    }
    ::CloseHandle(operation.hEvent);
    if (!success) {  return winFailure&lt;Count&gt;(e); }
    return ok(static_cast&lt;Count&gt;(done));
}</code></pre></details></td><td><a href="snippets/mac-3804440c46fa.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Count&gt; doPwrite(File f, Bytes b, std::uint64_t offset) {
    if (offset &gt; static_cast&lt;std::uint64_t&gt;(std::numeric_limits&lt;off_t&gt;::max())) return posixFailure&lt;Count&gt;(EOVERFLOW);
    const auto r = ::pwrite(f.native, b.data(), std::min(b.size(), static_cast&lt;std::size_t&gt;(PTRDIFF_MAX)), static_cast&lt;off_t&gt;(offset));
    if (r &lt; 0) return posixFailure&lt;Count&gt;();
    return ok(static_cast&lt;Count&gt;(r));
}</code></pre></details></td></tr>
<tr id="ftruncate"><td><a href="CALL_SITES.md#ftruncate"><code>ftruncate</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Set length without changing the shared file position.</p></details></td><td><pre><code>Status doTruncate(File f, std::uint64_t length);</code></pre><p></p></td><td><a href="snippets/linux-0ae162b824aa.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doTruncate(File f, std::uint64_t length) {
    if (length &gt; static_cast&lt;std::uint64_t&gt;(std::numeric_limits&lt;off_t&gt;::max())) return posixFailure(EOVERFLOW);
    return posixStatus(::ftruncate(f.native, static_cast&lt;off_t&gt;(length)));
}</code></pre></details></td><td><a href="snippets/windows-c8f549b0fd7e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doTruncate(File f, std::uint64_t length) {
    if (length &gt; static_cast&lt;std::uint64_t&gt;(INT64_MAX)) return winFailure(ERROR_INVALID_PARAMETER);
    FILE_END_OF_FILE_INFO info{};
    info.EndOfFile.QuadPart = static_cast&lt;LONGLONG&gt;(length);
    return winStatus(::SetFileInformationByHandle(f.native, FileEndOfFileInfo, &amp;info, sizeof(info)));
}</code></pre></details></td><td><a href="snippets/mac-0ae162b824aa.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doTruncate(File f, std::uint64_t length) {
    if (length &gt; static_cast&lt;std::uint64_t&gt;(std::numeric_limits&lt;off_t&gt;::max())) return posixFailure(EOVERFLOW);
    return posixStatus(::ftruncate(f.native, static_cast&lt;off_t&gt;(length)));
}</code></pre></details></td></tr>
<tr id="fstat"><td><a href="CALL_SITES.md#fstat"><code>fstat</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Include stable object identity, type, size, link count and modification time used by UNIX lock ownership checks.</p></details></td><td><pre><code>Result&lt;FileInfo&gt; doFileInfo(File f);</code></pre><p>Disk-file handles only on Windows. Reparse points remain other until their tags are inspected; this does not yet fulfill UNIX socket-node ownership detection. Unsupported filesystem ID queries fail explicitly.</p></td><td><a href="snippets/linux-b4d97046652c.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;FileInfo&gt; doFileInfo(File f) {
    struct stat st{};
    if (::fstat(f.native, &amp;st) == -1) return posixFailure&lt;FileInfo&gt;();
    FileInfo out;
    out.kind = S_ISREG(st.st_mode) ? FileInfo::Kind::regular : S_ISDIR(st.st_mode) ? FileInfo::Kind::directory : S_ISSOCK(st.st_mode) ? FileInfo::Kind::socket : FileInfo::Kind::other;
    out.size = static_cast&lt;std::uint64_t&gt;(st.st_size); out.links = st.st_nlink; out.device = st.st_dev;
    static_assert(sizeof(st.st_ino) &lt;= sizeof(out.id));
    std::memcpy(out.id.data(), &amp;st.st_ino, sizeof(st.st_ino));
    out.modifiedSeconds = st.st_mtim.tv_sec; out.modifiedNanoseconds = st.st_mtim.tv_nsec;
    return ok(out);
}</code></pre></details></td><td><a href="snippets/windows-87e961885c67.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;FileInfo&gt; doFileInfo(File f) {
    FILE_STANDARD_INFO standard{}; FILE_ID_INFO id{}; FILE_BASIC_INFO basic{};
    if (!::GetFileInformationByHandleEx(f.native, FileStandardInfo, &amp;standard, sizeof(standard)) ||
        !::GetFileInformationByHandleEx(f.native, FileIdInfo, &amp;id, sizeof(id)) ||
        !::GetFileInformationByHandleEx(f.native, FileBasicInfo, &amp;basic, sizeof(basic))) return winFailure&lt;FileInfo&gt;();
    FileInfo out;
    out.kind = (basic.FileAttributes &amp; FILE_ATTRIBUTE_REPARSE_POINT) ? FileInfo::Kind::other : standard.Directory ? FileInfo::Kind::directory : FileInfo::Kind::regular;
    out.size = standard.EndOfFile.QuadPart; out.links = standard.NumberOfLinks; out.device = id.VolumeSerialNumber;
    std::memcpy(out.id.data(), id.FileId.Identifier, out.id.size());
    auto delta = basic.LastWriteTime.QuadPart - INT64_C(116444736000000000);
    auto seconds = delta / INT64_C(10000000); auto remainder = delta % INT64_C(10000000);
    if (remainder &lt; 0) { --seconds; remainder += INT64_C(10000000); }
    out.modifiedSeconds = seconds; out.modifiedNanoseconds = static_cast&lt;std::uint32_t&gt;(remainder * 100);
    return ok(out);
}</code></pre></details></td><td><a href="snippets/mac-9a741df7b28a.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;FileInfo&gt; doFileInfo(File f) {
    struct stat st{};
    if (::fstat(f.native, &amp;st) == -1) return posixFailure&lt;FileInfo&gt;();
    FileInfo out;
    out.kind = S_ISREG(st.st_mode) ? FileInfo::Kind::regular : S_ISDIR(st.st_mode) ? FileInfo::Kind::directory : S_ISSOCK(st.st_mode) ? FileInfo::Kind::socket : FileInfo::Kind::other;
    out.size = static_cast&lt;std::uint64_t&gt;(st.st_size); out.links = st.st_nlink; out.device = st.st_dev;
    static_assert(sizeof(st.st_ino) &lt;= sizeof(out.id));
    std::memcpy(out.id.data(), &amp;st.st_ino, sizeof(st.st_ino));
    out.modifiedSeconds = st.st_mtimespec.tv_sec; out.modifiedNanoseconds = st.st_mtimespec.tv_nsec;
    return ok(out);
}</code></pre></details></td></tr>
<tr id="stat"><td><a href="CALL_SITES.md#stat"><code>stat</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; bundled CLI11, framework</p><p>Follow final symlink for stat; preserve missing versus inaccessible errors.</p></details></td><td><pre><code>Result&lt;FileInfo&gt; doPathInfo(const Path&amp; p);</code></pre><p>Windows symlink/socket reparse-tag classification is still needed before replacing security-sensitive lstat ownership checks. _stat64 is a Windows-only vendor spelling; no additional POSIX syscall.</p></td><td><a href="snippets/linux-2f802d880b80.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;FileInfo&gt; doPathInfo(const Path&amp; p) {
    struct stat st{};
    if (::stat(p.c_str(), &amp;st) == -1) return posixFailure&lt;FileInfo&gt;();
    FileInfo out;
    out.kind = S_ISREG(st.st_mode) ? FileInfo::Kind::regular : S_ISDIR(st.st_mode) ? FileInfo::Kind::directory : S_ISSOCK(st.st_mode) ? FileInfo::Kind::socket : S_ISLNK(st.st_mode) ? FileInfo::Kind::symlink : FileInfo::Kind::other;
    out.size = static_cast&lt;std::uint64_t&gt;(st.st_size); out.links = st.st_nlink; out.device = st.st_dev;
    static_assert(sizeof(st.st_ino) &lt;= sizeof(out.id));
    std::memcpy(out.id.data(), &amp;st.st_ino, sizeof(st.st_ino));
    out.modifiedSeconds = st.st_mtim.tv_sec; out.modifiedNanoseconds = st.st_mtim.tv_nsec;
    return ok(out);
}</code></pre></details></td><td><a href="snippets/windows-c42c1d9c804b.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;FileInfo&gt; doPathInfo(const Path&amp; p) {
    const HANDLE h = ::CreateFileW(p.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (h == INVALID_HANDLE_VALUE) return winFailure&lt;FileInfo&gt;();
    auto result = doFileInfo(File{h});
    const BOOL closed = ::CloseHandle(h);
    if (!closed &amp;&amp; result) return winFailure&lt;FileInfo&gt;();
    return result;
}</code></pre></details></td><td><a href="snippets/mac-535d6847c53e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;FileInfo&gt; doPathInfo(const Path&amp; p) {
    struct stat st{};
    if (::stat(p.c_str(), &amp;st) == -1) return posixFailure&lt;FileInfo&gt;();
    FileInfo out;
    out.kind = S_ISREG(st.st_mode) ? FileInfo::Kind::regular : S_ISDIR(st.st_mode) ? FileInfo::Kind::directory : S_ISSOCK(st.st_mode) ? FileInfo::Kind::socket : S_ISLNK(st.st_mode) ? FileInfo::Kind::symlink : FileInfo::Kind::other;
    out.size = static_cast&lt;std::uint64_t&gt;(st.st_size); out.links = st.st_nlink; out.device = st.st_dev;
    static_assert(sizeof(st.st_ino) &lt;= sizeof(out.id));
    std::memcpy(out.id.data(), &amp;st.st_ino, sizeof(st.st_ino));
    out.modifiedSeconds = st.st_mtimespec.tv_sec; out.modifiedNanoseconds = st.st_mtimespec.tv_nsec;
    return ok(out);
}</code></pre></details></td></tr>
<tr id="lstat"><td><a href="CALL_SITES.md#lstat"><code>lstat</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, test</p><p>No-follow mode inspects the final link/reparse point; file identity must support cleanup ownership checks.</p></details></td><td><pre><code>Result&lt;FileInfo&gt; doLinkInfo(const Path&amp; p);</code></pre><p>Windows symlink/socket reparse-tag classification is still needed before replacing security-sensitive lstat ownership checks. _stat64 is a Windows-only vendor spelling; no additional POSIX syscall.</p></td><td><a href="snippets/linux-b2703927e1b8.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;FileInfo&gt; doLinkInfo(const Path&amp; p) {
    struct stat st{};
    if (::lstat(p.c_str(), &amp;st) == -1) return posixFailure&lt;FileInfo&gt;();
    FileInfo out;
    out.kind = S_ISREG(st.st_mode) ? FileInfo::Kind::regular : S_ISDIR(st.st_mode) ? FileInfo::Kind::directory : S_ISSOCK(st.st_mode) ? FileInfo::Kind::socket : S_ISLNK(st.st_mode) ? FileInfo::Kind::symlink : FileInfo::Kind::other;
    out.size = static_cast&lt;std::uint64_t&gt;(st.st_size); out.links = st.st_nlink; out.device = st.st_dev;
    static_assert(sizeof(st.st_ino) &lt;= sizeof(out.id));
    std::memcpy(out.id.data(), &amp;st.st_ino, sizeof(st.st_ino));
    out.modifiedSeconds = st.st_mtim.tv_sec; out.modifiedNanoseconds = st.st_mtim.tv_nsec;
    return ok(out);
}</code></pre></details></td><td><a href="snippets/windows-9783edb48fb6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;FileInfo&gt; doLinkInfo(const Path&amp; p) {
    const HANDLE h = ::CreateFileW(p.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (h == INVALID_HANDLE_VALUE) return winFailure&lt;FileInfo&gt;();
    auto result = doFileInfo(File{h});
    const BOOL closed = ::CloseHandle(h);
    if (!closed &amp;&amp; result) return winFailure&lt;FileInfo&gt;();
    return result;
}</code></pre></details></td><td><a href="snippets/mac-5875a10ad6e4.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;FileInfo&gt; doLinkInfo(const Path&amp; p) {
    struct stat st{};
    if (::lstat(p.c_str(), &amp;st) == -1) return posixFailure&lt;FileInfo&gt;();
    FileInfo out;
    out.kind = S_ISREG(st.st_mode) ? FileInfo::Kind::regular : S_ISDIR(st.st_mode) ? FileInfo::Kind::directory : S_ISSOCK(st.st_mode) ? FileInfo::Kind::socket : S_ISLNK(st.st_mode) ? FileInfo::Kind::symlink : FileInfo::Kind::other;
    out.size = static_cast&lt;std::uint64_t&gt;(st.st_size); out.links = st.st_nlink; out.device = st.st_dev;
    static_assert(sizeof(st.st_ino) &lt;= sizeof(out.id));
    std::memcpy(out.id.data(), &amp;st.st_ino, sizeof(st.st_ino));
    out.modifiedSeconds = st.st_mtimespec.tv_sec; out.modifiedNanoseconds = st.st_mtimespec.tv_nsec;
    return ok(out);
}</code></pre></details></td></tr>
<tr id="unlink"><td><a href="CALL_SITES.md#unlink"><code>unlink</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, test, tool</p><p>Remove name, not recursively; Windows sharing and open-file deletion differences remain observable errors.</p></details></td><td><pre><code>Status doUnlink(const Path&amp; p);</code></pre><p></p></td><td><a href="snippets/linux-479382b00e42.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doUnlink(const Path&amp; p) {
    return posixStatus(::unlink(p.c_str()));
}</code></pre></details></td><td><a href="snippets/windows-13b747811ed3.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doUnlink(const Path&amp; p) {
    return winStatus(::DeleteFileW(p.c_str()));
}</code></pre></details></td><td><a href="snippets/mac-479382b00e42.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doUnlink(const Path&amp; p) {
    return posixStatus(::unlink(p.c_str()));
}</code></pre></details></td></tr>
<tr id="access"><td><a href="CALL_SITES.md#access"><code>access</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test, tool</p><p>X_OK has no literal Windows executable-bit equivalent; executable launchability must have a separately agreed policy.</p></details></td><td><pre><code>Result&lt;bool&gt; doPathExists(const Path&amp; path);</code></pre><p>Existence check only. R_OK/W_OK require access-token checks on Windows; X_OK has no executable-mode-bit equivalent. Never authorize a later open using this racy check.</p></td><td><a href="snippets/linux-a346961899fd.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;bool&gt; doPathExists(const Path&amp; path) {
    if (::access(path.c_str(), F_OK) == 0) return ok(true);
    if (errno == ENOENT || errno == ENOTDIR) return ok(false);
    return posixFailure&lt;bool&gt;();
}</code></pre></details></td><td><a href="snippets/windows-4f550acef6bf.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;bool&gt; doPathExists(const Path&amp; path) {
    const DWORD attributes = ::GetFileAttributesW(path.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES) return ok(true);
    const DWORD error = ::GetLastError();
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) return ok(false);
    return winFailure&lt;bool&gt;(error);
}</code></pre></details></td><td><a href="snippets/mac-a346961899fd.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;bool&gt; doPathExists(const Path&amp; path) {
    if (::access(path.c_str(), F_OK) == 0) return ok(true);
    if (errno == ENOENT || errno == ENOTDIR) return ok(false);
    return posixFailure&lt;bool&gt;();
}</code></pre></details></td></tr>
<tr id="mkdir"><td><a href="CALL_SITES.md#mkdir"><code>mkdir</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Existing-directory races must remain distinguishable; permission translation is explicit.</p></details></td><td><pre><code>ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup);</code></pre><p>Whole Config.cpp:953–1065 replacement; see config-caller.cpp.txt and config-types.hpp.txt. POSIX preserves root/XDG/create-only ownership policy with copied NSS strings. Windows LocalSystem/known-folder/ACL policy is a proposed behavior, not a proven POSIX equivalence. Other uses of these calls remain in the source ledger and need the daemon/test-specific capsule as applicable.</p></td><td><a href="snippets/linux-5790162f8c9e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, config-types.hpp, &lt;pwd.h&gt;, &lt;grp.h&gt;.
// Place this function in the existing utils::system owner at adoption.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    const passwd* pw = ::getpwuid(::getuid());
    if (!pw) return out;
    // Copy NSS-owned data before any subsequent lookup can invalidate it.
    out.userName = pw-&gt;pw_name;
    const std::string accountHome = pw-&gt;pw_dir;
    const gid_t primaryGroup = pw-&gt;pw_gid;
    const group* gr = ::getgrgid(primaryGroup);
    if (!gr) return out;
    out.groupName = gr-&gt;gr_name;
    const uid_t owner = ::geteuid();
    const bool privileged = owner == 0;
    if (privileged) {
        out.configDirectory = "/etc/snode.c";
        out.logDirectory = "/var/log/snode.c";
        out.pidDirectory = "/var/run/snode.c";
    } else {
        const char* environmentHome = ::getenv("HOME");
        const std::string home = environmentHome &amp;&amp; *environmentHome == '/' ? environmentHome : accountHome;
        auto directory = [&amp;](const char* variable, const char* fallback) -&gt; Path {
            const char* value = ::getenv(variable);
            if (value &amp;&amp; *value == '/') return Path(value) / "snode.c";
            if (!home.empty() &amp;&amp; home.front() == '/') return Path(home + fallback) / "snode.c";
            return {};
        };
        out.configDirectory = directory("XDG_CONFIG_HOME", "/.config");
        out.logDirectory = directory("XDG_STATE_HOME", "/.local/state");
        out.pidDirectory = directory("XDG_RUNTIME_DIR", "/.local/run");
        if (out.configDirectory.empty() || out.logDirectory.empty() || out.pidDirectory.empty()) return out;
    }
    // Preserve current create-only permissions, failure ordering and warning policy.
    using P = std::filesystem::perms;
    auto prepare = [&amp;](const Path&amp; path, P permissions) -&gt; bool {
        if (!std::filesystem::create_directories(path)) return true;
        std::filesystem::permissions(path, permissions);
        if (privileged) {
            const group* service = ::getgrnam(serviceGroup.c_str());
            if (!service) {
                out.diagnostics.push_back("Cannot find service group '" + serviceGroup + "'. Create it and add the current user.");
                std::filesystem::remove(path);
                return false;
            }
            if (::chown(path.c_str(), owner, service-&gt;gr_gid) &lt; 0) {
                const int error = errno;
                out.diagnostics.push_back("Cannot set group ownership of '" + path.string() + "': " + std::generic_category().message(error));
            }
        }
        return true;
    };
    if (!prepare(out.configDirectory, P::owner_all | P::group_read | P::group_exec)) return out;
    if (!prepare(out.logDirectory, P::owner_all | P::group_all)) return out;
    if (!prepare(out.pidDirectory, privileged ? P::owner_all | P::group_all : P::owner_all)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td><td><a href="snippets/windows-94ae28bc38fd.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, windows-text.hpp, config-types.hpp,
// &lt;shlobj.h&gt;, &lt;sddl.h&gt;, &lt;aclapi.h&gt;.
// Link Advapi32, Shell32, Ole32. Uses process identity, not thread impersonation.
// POLICY PROPOSAL: normal users get LocalAppData/snode.c/{config,log,run};
// LocalSystem gets ProgramData/snode.c/{config,log,run}. Elevated non-System
// users remain per-user. No synthetic UID zero or GID is introduced.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    auto report = [&amp;](DWORD error) {
        out.diagnostics.push_back("Windows configuration setup failed: " + std::to_string(error));
    };
    HANDLE rawToken = nullptr;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &amp;rawToken)) {
        report(::GetLastError()); return out;
    }
    struct CloseToken { void operator()(void* p) const { ::CloseHandle(p); } };
    std::unique_ptr&lt;void, CloseToken&gt; token(rawToken);
    auto tokenInfo = [&amp;](TOKEN_INFORMATION_CLASS kind) -&gt; Result&lt;std::vector&lt;std::byte&gt;&gt; {
        DWORD size = 0;
        ::GetTokenInformation(token.get(), kind, nullptr, 0, &amp;size);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) return winFailure&lt;std::vector&lt;std::byte&gt;&gt;();
        std::vector&lt;std::byte&gt; bytes(size);
        if (!::GetTokenInformation(token.get(), kind, bytes.data(), size, &amp;size)) return winFailure&lt;std::vector&lt;std::byte&gt;&gt;();
        return ok(std::move(bytes));
    };
    auto user = tokenInfo(TokenUser);
    auto group = tokenInfo(TokenPrimaryGroup);
    if (!user || !group) { report((!user ? user.error : group.error).code); return out; }
    PSID userSid = reinterpret_cast&lt;TOKEN_USER*&gt;(user.value.data())-&gt;User.Sid;
    PSID groupSid = reinterpret_cast&lt;TOKEN_PRIMARY_GROUP*&gt;(group.value.data())-&gt;PrimaryGroup;
    auto nameFor = [&amp;](PSID sid) -&gt; Result&lt;std::string&gt; {
        DWORD nameSize = 0, domainSize = 0; SID_NAME_USE type;
        ::LookupAccountSidW(nullptr, sid, nullptr, &amp;nameSize, nullptr, &amp;domainSize, &amp;type);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) return winFailure&lt;std::string&gt;();
        std::vector&lt;wchar_t&gt; name(nameSize), domain(std::max(DWORD{1}, domainSize));
        if (!::LookupAccountSidW(nullptr, sid, name.data(), &amp;nameSize, domain.data(), &amp;domainSize, &amp;type)) return winFailure&lt;std::string&gt;();
        const std::wstring qualified = domainSize ? std::wstring(domain.data()) + L"\\" + name.data() : name.data();
        return toUtf8(qualified);
    };
    auto userName = nameFor(userSid), groupName = nameFor(groupSid);
    if (!userName || !groupName) { report((!userName ? userName.error : groupName.error).code); return out; }
    out.userName = std::move(userName.value); out.groupName = std::move(groupName.value);
    const bool service = ::IsWellKnownSid(userSid, WinLocalSystemSid) != FALSE;
    PWSTR folder = nullptr;
    const HRESULT status = ::SHGetKnownFolderPath(service ? FOLDERID_ProgramData : FOLDERID_LocalAppData, 0, nullptr, &amp;folder);
    if (FAILED(status)) { report(static_cast&lt;DWORD&gt;(status)); return out; }
    struct FreeFolder { void operator()(wchar_t* p) const { ::CoTaskMemFree(p); } };
    std::unique_ptr&lt;wchar_t, FreeFolder&gt; folderOwner(folder);
    const Path root = Path(folder) / L"snode.c";
    out.configDirectory = root / L"config";
    out.logDirectory = root / L"log";
    out.pidDirectory = root / L"run";

    // Resolve a configured service group to an actual SID. This is not a GID cast.
    std::vector&lt;std::byte&gt; serviceSid;
    if (service) {
        auto name = toWide(serviceGroup);
        if (!name) { report(name.error.code); return out; }
        DWORD sidSize = 0, domainSize = 0; SID_NAME_USE type;
        ::LookupAccountNameW(nullptr, name.value.c_str(), nullptr, &amp;sidSize, nullptr, &amp;domainSize, &amp;type);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) { report(::GetLastError()); return out; }
        serviceSid.resize(sidSize);
        std::vector&lt;wchar_t&gt; domain(std::max(DWORD{1}, domainSize));
        if (!::LookupAccountNameW(nullptr, name.value.c_str(), serviceSid.data(), &amp;sidSize, domain.data(), &amp;domainSize, &amp;type)) {
            report(::GetLastError()); return out;
        }
        if (type != SidTypeGroup &amp;&amp; type != SidTypeAlias &amp;&amp; type != SidTypeWellKnownGroup) {
            report(ERROR_INVALID_PRIMARY_GROUP); return out;
        }
    }
    auto sidText = [&amp;](PSID sid) -&gt; std::wstring {
        LPWSTR value = nullptr;
        if (!::ConvertSidToStringSidW(sid, &amp;value)) return {};
        std::unique_ptr&lt;wchar_t, decltype(&amp;::LocalFree)&gt; guard(value, ::LocalFree);
        return value;
    };
    const auto owner = sidText(userSid);
    const auto shared = service ? sidText(serviceSid.data()) : std::wstring{};
    if (owner.empty() || (service &amp;&amp; shared.empty())) { report(::GetLastError()); return out; }
    auto create = [&amp;](const Path&amp; path, bool readOnlyGroup) -&gt; bool {
        // Protected DACL: owner and SYSTEM full access. Service group read/execute
        // on config, modify on log/run. OI/CI apply to new descendants.
        std::wstring sddl = L"D:P(A;OICI;FA;;;" + owner + L")(A;OICI;FA;;;SY)";
        if (service) sddl += L"(A;OICI;" + std::wstring(readOnlyGroup ? L"0x1200a9" : L"0x1301bf") + L";;;" + shared + L")";
        PSECURITY_DESCRIPTOR descriptor = nullptr;
        if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &amp;descriptor, nullptr)) {
            report(::GetLastError()); return false;
        }
        std::unique_ptr&lt;void, decltype(&amp;::LocalFree)&gt; descriptorOwner(descriptor, ::LocalFree);
        SECURITY_ATTRIBUTES attributes{sizeof(attributes), descriptor, FALSE};
        if (::CreateDirectoryW(path.c_str(), &amp;attributes)) return true;
        const DWORD error = ::GetLastError();
        if (error != ERROR_ALREADY_EXISTS) { report(error); return false; }
        const DWORD attrs = ::GetFileAttributesW(path.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) { report(::GetLastError()); return false; }
        // Existing directories retain permissions; reject reparse redirection.
        if (!(attrs &amp; FILE_ATTRIBUTE_DIRECTORY) || (attrs &amp; FILE_ATTRIBUTE_REPARSE_POINT)) {
            report(ERROR_ACCESS_DENIED); return false;
        }
        return true;
    };
    // Known-folder parent exists. Create root before the three children.
    if (!create(root, false) || !create(out.configDirectory, true) || !create(out.logDirectory, false) || !create(out.pidDirectory, false)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td><td><a href="snippets/mac-5790162f8c9e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, config-types.hpp, &lt;pwd.h&gt;, &lt;grp.h&gt;.
// Place this function in the existing utils::system owner at adoption.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    const passwd* pw = ::getpwuid(::getuid());
    if (!pw) return out;
    // Copy NSS-owned data before any subsequent lookup can invalidate it.
    out.userName = pw-&gt;pw_name;
    const std::string accountHome = pw-&gt;pw_dir;
    const gid_t primaryGroup = pw-&gt;pw_gid;
    const group* gr = ::getgrgid(primaryGroup);
    if (!gr) return out;
    out.groupName = gr-&gt;gr_name;
    const uid_t owner = ::geteuid();
    const bool privileged = owner == 0;
    if (privileged) {
        out.configDirectory = "/etc/snode.c";
        out.logDirectory = "/var/log/snode.c";
        out.pidDirectory = "/var/run/snode.c";
    } else {
        const char* environmentHome = ::getenv("HOME");
        const std::string home = environmentHome &amp;&amp; *environmentHome == '/' ? environmentHome : accountHome;
        auto directory = [&amp;](const char* variable, const char* fallback) -&gt; Path {
            const char* value = ::getenv(variable);
            if (value &amp;&amp; *value == '/') return Path(value) / "snode.c";
            if (!home.empty() &amp;&amp; home.front() == '/') return Path(home + fallback) / "snode.c";
            return {};
        };
        out.configDirectory = directory("XDG_CONFIG_HOME", "/.config");
        out.logDirectory = directory("XDG_STATE_HOME", "/.local/state");
        out.pidDirectory = directory("XDG_RUNTIME_DIR", "/.local/run");
        if (out.configDirectory.empty() || out.logDirectory.empty() || out.pidDirectory.empty()) return out;
    }
    // Preserve current create-only permissions, failure ordering and warning policy.
    using P = std::filesystem::perms;
    auto prepare = [&amp;](const Path&amp; path, P permissions) -&gt; bool {
        if (!std::filesystem::create_directories(path)) return true;
        std::filesystem::permissions(path, permissions);
        if (privileged) {
            const group* service = ::getgrnam(serviceGroup.c_str());
            if (!service) {
                out.diagnostics.push_back("Cannot find service group '" + serviceGroup + "'. Create it and add the current user.");
                std::filesystem::remove(path);
                return false;
            }
            if (::chown(path.c_str(), owner, service-&gt;gr_gid) &lt; 0) {
                const int error = errno;
                out.diagnostics.push_back("Cannot set group ownership of '" + path.string() + "': " + std::generic_category().message(error));
            }
        }
        return true;
    };
    if (!prepare(out.configDirectory, P::owner_all | P::group_read | P::group_exec)) return out;
    if (!prepare(out.logDirectory, P::owner_all | P::group_all)) return out;
    if (!prepare(out.pidDirectory, privileged ? P::owner_all | P::group_all : P::owner_all)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td></tr>
<tr id="rmdir"><td><a href="CALL_SITES.md#rmdir"><code>rmdir</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Remove an empty directory only.</p></details></td><td><pre><code>Status doRmdir(const Path&amp; p);</code></pre><p></p></td><td><a href="snippets/linux-8c80401b7964.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doRmdir(const Path&amp; p) {
    return posixStatus(::rmdir(p.c_str()));
}</code></pre></details></td><td><a href="snippets/windows-49fed73803db.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doRmdir(const Path&amp; p) {
    return winStatus(::RemoveDirectoryW(p.c_str()));
}</code></pre></details></td><td><a href="snippets/mac-8c80401b7964.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doRmdir(const Path&amp; p) {
    return posixStatus(::rmdir(p.c_str()));
}</code></pre></details></td></tr>
<tr id="mkstemp"><td><a href="CALL_SITES.md#mkstemp"><code>mkstemp</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>Atomically create a unique private file; return its owned handle and actual path.</p></details></td><td><pre><code>Result&lt;File&gt; doMkstemp(std::string&amp; pattern);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-f9363f994c39.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doMkstemp(std::string&amp; pattern) {
    const int fd = ::mkstemp(pattern.data());
    if (fd == -1) return posixFailure&lt;File&gt;();
    return ok(File{fd});
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>CreateFileW(CREATE_NEW) with unpredictable candidate names and explicit private security descriptor must be implemented with bounded collision handling and cleanup.</p><p>Native building blocks: CreateFileW(CREATE_NEW)</p></td><td><a href="snippets/mac-f9363f994c39.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doMkstemp(std::string&amp; pattern) {
    const int fd = ::mkstemp(pattern.data());
    if (fd == -1) return posixFailure&lt;File&gt;();
    return ok(File{fd});
}</code></pre></details></td></tr>
<tr id="mkdtemp"><td><a href="CALL_SITES.md#mkdtemp"><code>mkdtemp</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Exclusive creation; return actual path; collision retry stays inside capsule.</p></details></td><td><pre><code>Status doMkdtemp(std::string&amp; pattern);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-1eace5baf039.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doMkdtemp(std::string&amp; pattern) {
    return ::mkdtemp(pattern.data()) ? ok() : posixFailure();
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>CreateDirectoryW with private ACL and unpredictable name/collision handling is required. GetTempFileNameW does not implement private temporary-directory semantics.</p><p>Native building blocks: CreateDirectoryW with an explicitly private security descriptor</p></td><td><a href="snippets/mac-1eace5baf039.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doMkdtemp(std::string&amp; pattern) {
    return ::mkdtemp(pattern.data()) ? ok() : posixFailure();
}</code></pre></details></td></tr>
<tr id="chdir"><td><a href="CALL_SITES.md#chdir"><code>chdir</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, test</p><p>Process-wide working directory change, not thread-local.</p></details></td><td><pre><code>Status doChdir(const Path&amp; p);</code></pre><p></p></td><td><a href="snippets/linux-ac6a1b72273c.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doChdir(const Path&amp; p) {
    return posixStatus(::chdir(p.c_str()));
}</code></pre></details></td><td><a href="snippets/windows-5d18f66674b1.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doChdir(const Path&amp; p) {
    return winStatus(::SetCurrentDirectoryW(p.c_str()));
}</code></pre></details></td><td><a href="snippets/mac-ac6a1b72273c.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doChdir(const Path&amp; p) {
    return posixStatus(::chdir(p.c_str()));
}</code></pre></details></td></tr>
<tr id="fchdir"><td><a href="CALL_SITES.md#fchdir"><code>fchdir</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>BLOCKED for exact rename-safe fchdir semantics on Windows; test cleanup may use a path snapshot only with explicit weaker semantics.</p></details></td><td><pre><code>Status doRestoreDirectory(File directory);</code></pre><p>No rename-safe Windows handle-based SetCurrentDirectory operation. Replacing with a saved path changes semantics; not supplied as an equivalent.</p></td><td><a href="snippets/linux-ebfae5f27cc1.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doRestoreDirectory(File directory) {
    return posixStatus(::fchdir(directory.native));
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>No rename-safe Windows handle-based SetCurrentDirectory operation. Replacing with a saved path changes semantics; not supplied as an equivalent.</p><p>Native building blocks: No direct handle-based Win32 equivalent; GetFinalPathNameByHandleW plus SetCurrentDirectoryW is weaker</p></td><td><a href="snippets/mac-ebfae5f27cc1.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doRestoreDirectory(File directory) {
    return posixStatus(::fchdir(directory.native));
}</code></pre></details></td></tr>
<tr id="chown"><td><a href="CALL_SITES.md#chown"><code>chown</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>UID/GID are not Windows SIDs; privileges and inheritance differ.</p></details></td><td><pre><code>ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup);</code></pre><p>Whole Config.cpp:953–1065 replacement; see config-caller.cpp.txt and config-types.hpp.txt. POSIX preserves root/XDG/create-only ownership policy with copied NSS strings. Windows LocalSystem/known-folder/ACL policy is a proposed behavior, not a proven POSIX equivalence. Other uses of these calls remain in the source ledger and need the daemon/test-specific capsule as applicable.</p></td><td><a href="snippets/linux-5790162f8c9e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, config-types.hpp, &lt;pwd.h&gt;, &lt;grp.h&gt;.
// Place this function in the existing utils::system owner at adoption.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    const passwd* pw = ::getpwuid(::getuid());
    if (!pw) return out;
    // Copy NSS-owned data before any subsequent lookup can invalidate it.
    out.userName = pw-&gt;pw_name;
    const std::string accountHome = pw-&gt;pw_dir;
    const gid_t primaryGroup = pw-&gt;pw_gid;
    const group* gr = ::getgrgid(primaryGroup);
    if (!gr) return out;
    out.groupName = gr-&gt;gr_name;
    const uid_t owner = ::geteuid();
    const bool privileged = owner == 0;
    if (privileged) {
        out.configDirectory = "/etc/snode.c";
        out.logDirectory = "/var/log/snode.c";
        out.pidDirectory = "/var/run/snode.c";
    } else {
        const char* environmentHome = ::getenv("HOME");
        const std::string home = environmentHome &amp;&amp; *environmentHome == '/' ? environmentHome : accountHome;
        auto directory = [&amp;](const char* variable, const char* fallback) -&gt; Path {
            const char* value = ::getenv(variable);
            if (value &amp;&amp; *value == '/') return Path(value) / "snode.c";
            if (!home.empty() &amp;&amp; home.front() == '/') return Path(home + fallback) / "snode.c";
            return {};
        };
        out.configDirectory = directory("XDG_CONFIG_HOME", "/.config");
        out.logDirectory = directory("XDG_STATE_HOME", "/.local/state");
        out.pidDirectory = directory("XDG_RUNTIME_DIR", "/.local/run");
        if (out.configDirectory.empty() || out.logDirectory.empty() || out.pidDirectory.empty()) return out;
    }
    // Preserve current create-only permissions, failure ordering and warning policy.
    using P = std::filesystem::perms;
    auto prepare = [&amp;](const Path&amp; path, P permissions) -&gt; bool {
        if (!std::filesystem::create_directories(path)) return true;
        std::filesystem::permissions(path, permissions);
        if (privileged) {
            const group* service = ::getgrnam(serviceGroup.c_str());
            if (!service) {
                out.diagnostics.push_back("Cannot find service group '" + serviceGroup + "'. Create it and add the current user.");
                std::filesystem::remove(path);
                return false;
            }
            if (::chown(path.c_str(), owner, service-&gt;gr_gid) &lt; 0) {
                const int error = errno;
                out.diagnostics.push_back("Cannot set group ownership of '" + path.string() + "': " + std::generic_category().message(error));
            }
        }
        return true;
    };
    if (!prepare(out.configDirectory, P::owner_all | P::group_read | P::group_exec)) return out;
    if (!prepare(out.logDirectory, P::owner_all | P::group_all)) return out;
    if (!prepare(out.pidDirectory, privileged ? P::owner_all | P::group_all : P::owner_all)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td><td><a href="snippets/windows-94ae28bc38fd.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, windows-text.hpp, config-types.hpp,
// &lt;shlobj.h&gt;, &lt;sddl.h&gt;, &lt;aclapi.h&gt;.
// Link Advapi32, Shell32, Ole32. Uses process identity, not thread impersonation.
// POLICY PROPOSAL: normal users get LocalAppData/snode.c/{config,log,run};
// LocalSystem gets ProgramData/snode.c/{config,log,run}. Elevated non-System
// users remain per-user. No synthetic UID zero or GID is introduced.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    auto report = [&amp;](DWORD error) {
        out.diagnostics.push_back("Windows configuration setup failed: " + std::to_string(error));
    };
    HANDLE rawToken = nullptr;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &amp;rawToken)) {
        report(::GetLastError()); return out;
    }
    struct CloseToken { void operator()(void* p) const { ::CloseHandle(p); } };
    std::unique_ptr&lt;void, CloseToken&gt; token(rawToken);
    auto tokenInfo = [&amp;](TOKEN_INFORMATION_CLASS kind) -&gt; Result&lt;std::vector&lt;std::byte&gt;&gt; {
        DWORD size = 0;
        ::GetTokenInformation(token.get(), kind, nullptr, 0, &amp;size);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) return winFailure&lt;std::vector&lt;std::byte&gt;&gt;();
        std::vector&lt;std::byte&gt; bytes(size);
        if (!::GetTokenInformation(token.get(), kind, bytes.data(), size, &amp;size)) return winFailure&lt;std::vector&lt;std::byte&gt;&gt;();
        return ok(std::move(bytes));
    };
    auto user = tokenInfo(TokenUser);
    auto group = tokenInfo(TokenPrimaryGroup);
    if (!user || !group) { report((!user ? user.error : group.error).code); return out; }
    PSID userSid = reinterpret_cast&lt;TOKEN_USER*&gt;(user.value.data())-&gt;User.Sid;
    PSID groupSid = reinterpret_cast&lt;TOKEN_PRIMARY_GROUP*&gt;(group.value.data())-&gt;PrimaryGroup;
    auto nameFor = [&amp;](PSID sid) -&gt; Result&lt;std::string&gt; {
        DWORD nameSize = 0, domainSize = 0; SID_NAME_USE type;
        ::LookupAccountSidW(nullptr, sid, nullptr, &amp;nameSize, nullptr, &amp;domainSize, &amp;type);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) return winFailure&lt;std::string&gt;();
        std::vector&lt;wchar_t&gt; name(nameSize), domain(std::max(DWORD{1}, domainSize));
        if (!::LookupAccountSidW(nullptr, sid, name.data(), &amp;nameSize, domain.data(), &amp;domainSize, &amp;type)) return winFailure&lt;std::string&gt;();
        const std::wstring qualified = domainSize ? std::wstring(domain.data()) + L"\\" + name.data() : name.data();
        return toUtf8(qualified);
    };
    auto userName = nameFor(userSid), groupName = nameFor(groupSid);
    if (!userName || !groupName) { report((!userName ? userName.error : groupName.error).code); return out; }
    out.userName = std::move(userName.value); out.groupName = std::move(groupName.value);
    const bool service = ::IsWellKnownSid(userSid, WinLocalSystemSid) != FALSE;
    PWSTR folder = nullptr;
    const HRESULT status = ::SHGetKnownFolderPath(service ? FOLDERID_ProgramData : FOLDERID_LocalAppData, 0, nullptr, &amp;folder);
    if (FAILED(status)) { report(static_cast&lt;DWORD&gt;(status)); return out; }
    struct FreeFolder { void operator()(wchar_t* p) const { ::CoTaskMemFree(p); } };
    std::unique_ptr&lt;wchar_t, FreeFolder&gt; folderOwner(folder);
    const Path root = Path(folder) / L"snode.c";
    out.configDirectory = root / L"config";
    out.logDirectory = root / L"log";
    out.pidDirectory = root / L"run";

    // Resolve a configured service group to an actual SID. This is not a GID cast.
    std::vector&lt;std::byte&gt; serviceSid;
    if (service) {
        auto name = toWide(serviceGroup);
        if (!name) { report(name.error.code); return out; }
        DWORD sidSize = 0, domainSize = 0; SID_NAME_USE type;
        ::LookupAccountNameW(nullptr, name.value.c_str(), nullptr, &amp;sidSize, nullptr, &amp;domainSize, &amp;type);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) { report(::GetLastError()); return out; }
        serviceSid.resize(sidSize);
        std::vector&lt;wchar_t&gt; domain(std::max(DWORD{1}, domainSize));
        if (!::LookupAccountNameW(nullptr, name.value.c_str(), serviceSid.data(), &amp;sidSize, domain.data(), &amp;domainSize, &amp;type)) {
            report(::GetLastError()); return out;
        }
        if (type != SidTypeGroup &amp;&amp; type != SidTypeAlias &amp;&amp; type != SidTypeWellKnownGroup) {
            report(ERROR_INVALID_PRIMARY_GROUP); return out;
        }
    }
    auto sidText = [&amp;](PSID sid) -&gt; std::wstring {
        LPWSTR value = nullptr;
        if (!::ConvertSidToStringSidW(sid, &amp;value)) return {};
        std::unique_ptr&lt;wchar_t, decltype(&amp;::LocalFree)&gt; guard(value, ::LocalFree);
        return value;
    };
    const auto owner = sidText(userSid);
    const auto shared = service ? sidText(serviceSid.data()) : std::wstring{};
    if (owner.empty() || (service &amp;&amp; shared.empty())) { report(::GetLastError()); return out; }
    auto create = [&amp;](const Path&amp; path, bool readOnlyGroup) -&gt; bool {
        // Protected DACL: owner and SYSTEM full access. Service group read/execute
        // on config, modify on log/run. OI/CI apply to new descendants.
        std::wstring sddl = L"D:P(A;OICI;FA;;;" + owner + L")(A;OICI;FA;;;SY)";
        if (service) sddl += L"(A;OICI;" + std::wstring(readOnlyGroup ? L"0x1200a9" : L"0x1301bf") + L";;;" + shared + L")";
        PSECURITY_DESCRIPTOR descriptor = nullptr;
        if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &amp;descriptor, nullptr)) {
            report(::GetLastError()); return false;
        }
        std::unique_ptr&lt;void, decltype(&amp;::LocalFree)&gt; descriptorOwner(descriptor, ::LocalFree);
        SECURITY_ATTRIBUTES attributes{sizeof(attributes), descriptor, FALSE};
        if (::CreateDirectoryW(path.c_str(), &amp;attributes)) return true;
        const DWORD error = ::GetLastError();
        if (error != ERROR_ALREADY_EXISTS) { report(error); return false; }
        const DWORD attrs = ::GetFileAttributesW(path.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) { report(::GetLastError()); return false; }
        // Existing directories retain permissions; reject reparse redirection.
        if (!(attrs &amp; FILE_ATTRIBUTE_DIRECTORY) || (attrs &amp; FILE_ATTRIBUTE_REPARSE_POINT)) {
            report(ERROR_ACCESS_DENIED); return false;
        }
        return true;
    };
    // Known-folder parent exists. Create root before the three children.
    if (!create(root, false) || !create(out.configDirectory, true) || !create(out.logDirectory, false) || !create(out.pidDirectory, false)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td><td><a href="snippets/mac-5790162f8c9e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, config-types.hpp, &lt;pwd.h&gt;, &lt;grp.h&gt;.
// Place this function in the existing utils::system owner at adoption.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    const passwd* pw = ::getpwuid(::getuid());
    if (!pw) return out;
    // Copy NSS-owned data before any subsequent lookup can invalidate it.
    out.userName = pw-&gt;pw_name;
    const std::string accountHome = pw-&gt;pw_dir;
    const gid_t primaryGroup = pw-&gt;pw_gid;
    const group* gr = ::getgrgid(primaryGroup);
    if (!gr) return out;
    out.groupName = gr-&gt;gr_name;
    const uid_t owner = ::geteuid();
    const bool privileged = owner == 0;
    if (privileged) {
        out.configDirectory = "/etc/snode.c";
        out.logDirectory = "/var/log/snode.c";
        out.pidDirectory = "/var/run/snode.c";
    } else {
        const char* environmentHome = ::getenv("HOME");
        const std::string home = environmentHome &amp;&amp; *environmentHome == '/' ? environmentHome : accountHome;
        auto directory = [&amp;](const char* variable, const char* fallback) -&gt; Path {
            const char* value = ::getenv(variable);
            if (value &amp;&amp; *value == '/') return Path(value) / "snode.c";
            if (!home.empty() &amp;&amp; home.front() == '/') return Path(home + fallback) / "snode.c";
            return {};
        };
        out.configDirectory = directory("XDG_CONFIG_HOME", "/.config");
        out.logDirectory = directory("XDG_STATE_HOME", "/.local/state");
        out.pidDirectory = directory("XDG_RUNTIME_DIR", "/.local/run");
        if (out.configDirectory.empty() || out.logDirectory.empty() || out.pidDirectory.empty()) return out;
    }
    // Preserve current create-only permissions, failure ordering and warning policy.
    using P = std::filesystem::perms;
    auto prepare = [&amp;](const Path&amp; path, P permissions) -&gt; bool {
        if (!std::filesystem::create_directories(path)) return true;
        std::filesystem::permissions(path, permissions);
        if (privileged) {
            const group* service = ::getgrnam(serviceGroup.c_str());
            if (!service) {
                out.diagnostics.push_back("Cannot find service group '" + serviceGroup + "'. Create it and add the current user.");
                std::filesystem::remove(path);
                return false;
            }
            if (::chown(path.c_str(), owner, service-&gt;gr_gid) &lt; 0) {
                const int error = errno;
                out.diagnostics.push_back("Cannot set group ownership of '" + path.string() + "': " + std::generic_category().message(error));
            }
        }
        return true;
    };
    if (!prepare(out.configDirectory, P::owner_all | P::group_read | P::group_exec)) return out;
    if (!prepare(out.logDirectory, P::owner_all | P::group_all)) return out;
    if (!prepare(out.pidDirectory, privileged ? P::owner_all | P::group_all : P::owner_all)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td></tr>
<tr id="umask"><td><a href="CALL_SITES.md#umask"><code>umask</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Contract applies to capsule-created resources; cannot promise it governs third-party Win32 file creation.</p></details></td><td><pre><code>std::uint32_t doCreationMask(std::uint32_t mask);</code></pre><p>Process-wide POSIX creation mask. No Win32 process-wide ACL equivalent. Windows config capsule supplies security descriptors at creation; CRT _umask would not control native CreateFileW.</p></td><td><a href="snippets/linux-d70fe829f622.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint32_t doCreationMask(std::uint32_t mask) {
    return ::umask(static_cast&lt;mode_t&gt;(mask));
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Process-wide POSIX creation mask. No Win32 process-wide ACL equivalent. Windows config capsule supplies security descriptors at creation; CRT _umask would not control native CreateFileW.</p><p>Native building blocks: _umask affects CRT creation only; no process-wide Win32 ACL equivalent</p></td><td><a href="snippets/mac-d70fe829f622.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint32_t doCreationMask(std::uint32_t mask) {
    return ::umask(static_cast&lt;mode_t&gt;(mask));
}</code></pre></details></td></tr>
<tr id="flock"><td><a href="CALL_SITES.md#flock"><code>flock</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `core::system::flock`</p><p>Whole-file exclusive/shared lock intent; advisory versus mandatory and duplicate-handle lifetime differences need validation.</p></details></td><td><pre><code>Status doFileLock(File f, bool exclusive, bool unlock, bool wait);</code></pre><p>Windows body requires a synchronous disk handle. Windows range locks affect I/O; POSIX flock is advisory. Duplicate-handle and process lifetime semantics must be validated before UNIX lock-file migration.</p></td><td><a href="snippets/linux-eed428d124f0.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doFileLock(File f, bool exclusive, bool unlock, bool wait) {
    const int op = unlock ? LOCK_UN : (exclusive ? LOCK_EX : LOCK_SH) | (wait ? 0 : LOCK_NB);
    return posixStatus(::flock(f.native, op));
}</code></pre></details></td><td><a href="snippets/windows-db0bf31e34d8.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doFileLock(File f, bool exclusive, bool unlock, bool wait) {
    OVERLAPPED offset{};
    if (unlock) return winStatus(::UnlockFileEx(f.native, 0, MAXDWORD, MAXDWORD, &amp;offset));
    const DWORD flags = (exclusive ? LOCKFILE_EXCLUSIVE_LOCK : 0) | (wait ? 0 : LOCKFILE_FAIL_IMMEDIATELY);
    return winStatus(::LockFileEx(f.native, flags, 0, MAXDWORD, MAXDWORD, &amp;offset));
}</code></pre></details></td><td><a href="snippets/mac-eed428d124f0.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doFileLock(File f, bool exclusive, bool unlock, bool wait) {
    const int op = unlock ? LOCK_UN : (exclusive ? LOCK_EX : LOCK_SH) | (wait ? 0 : LOCK_NB);
    return posixStatus(::flock(f.native, op));
}</code></pre></details></td></tr>
<tr id="fcntl"><td><a href="CALL_SITES.md#fcntl"><code>fcntl</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, test</p><p>Enumerate actual commands in evidence; regular files and pipes do not acquire socket semantics by casting a handle.</p></details></td><td><pre><code>Status doSetNonblocking(Socket socket, bool enabled);</code></pre><p>Socket variant only, preserves existing flags on POSIX. File/pipe nonblocking and F_GETFD inspection require separate resource-specific capsules; ioctlsocket is not a universal fcntl replacement.</p></td><td><a href="snippets/linux-778e353ce45e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSetNonblocking(Socket socket, bool enabled) {
    const int flags = ::fcntl(socket.native, F_GETFL);
    if (flags == -1) return posixFailure();
    return posixStatus(::fcntl(socket.native, F_SETFL, enabled ? flags | O_NONBLOCK : flags &amp; ~O_NONBLOCK));
}</code></pre></details></td><td><a href="snippets/windows-36ca4b84f023.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSetNonblocking(Socket socket, bool enabled) {
    u_long value = enabled;
    return socketStatus(::ioctlsocket(socket.native, FIONBIO, &amp;value));
}</code></pre></details></td><td><a href="snippets/mac-778e353ce45e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSetNonblocking(Socket socket, bool enabled) {
    const int flags = ::fcntl(socket.native, F_GETFL);
    if (flags == -1) return posixFailure();
    return posixStatus(::fcntl(socket.native, F_SETFL, enabled ? flags | O_NONBLOCK : flags &amp; ~O_NONBLOCK));
}</code></pre></details></td></tr>
<tr id="dup"><td><a href="CALL_SITES.md#dup"><code>dup</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Return independently owned reference with specified shared-position behavior.</p></details></td><td><pre><code>Result&lt;File&gt; doDup(File f);</code></pre><p>File handles only; no SOCKET-to-HANDLE cast. POSIX dup clears close-on-exec, Windows result is non-inheritable; caller migration must choose a consistent inheritance contract.</p></td><td><a href="snippets/linux-c8d839b8b880.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doDup(File f) {
    const int n = ::dup(f.native);
    if (n == -1) return posixFailure&lt;File&gt;();
    return ok(File{n});
}</code></pre></details></td><td><a href="snippets/windows-795acebd38d4.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doDup(File f) {
    HANDLE n = INVALID_HANDLE_VALUE;
    if (!::DuplicateHandle(::GetCurrentProcess(), f.native, ::GetCurrentProcess(), &amp;n, 0, FALSE, DUPLICATE_SAME_ACCESS)) return winFailure&lt;File&gt;();
    return ok(File{n});
}</code></pre></details></td><td><a href="snippets/mac-c8d839b8b880.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doDup(File f) {
    const int n = ::dup(f.native);
    if (n == -1) return posixFailure&lt;File&gt;();
    return ok(File{n});
}</code></pre></details></td></tr>
<tr id="dup2"><td><a href="CALL_SITES.md#dup2"><code>dup2</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Observed uses redirect standard streams or replace a socket test fixture; socket fixture must adopt a handle instead of assuming a numeric slot.</p></details></td><td><pre><code>Status doDup2(File from, int target);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-29c0788f7542.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doDup2(File from, int target) {
    return posixStatus(::dup2(from.native, target));
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Use explicit child standard-handle redirection within the launch operation. Numeric POSIX descriptor-slot replacement has no general HANDLE/SOCKET equivalent; _dup2 covers CRT descriptors only.</p><p>Native building blocks: _dup2 for CRT streams; SetStdHandle for Win32 standard handles</p></td><td><a href="snippets/mac-29c0788f7542.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doDup2(File from, int target) {
    return posixStatus(::dup2(from.native, target));
}</code></pre></details></td></tr>
<tr id="pipe"><td><a href="CALL_SITES.md#pipe"><code>pipe</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>Return independently owned read/write ends; byte stream, EOF, inheritance and readiness are contractual.</p></details></td><td><pre><code>Result&lt;Pipe&gt; doPipe();</code></pre><p>Blocking byte pipe only. Windows handles are non-inheritable. ProcessRunner needs an integrated launch/capture capsule before replacing select on these pipes.</p></td><td><a href="snippets/linux-1b202aa42e96.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Pipe&gt; doPipe() {
    int p[2];
    if (::pipe(p) == -1) return posixFailure&lt;Pipe&gt;();
    return ok(Pipe{File{p[0]}, File{p[1]}});
}</code></pre></details></td><td><a href="snippets/windows-507c97a460aa.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Pipe&gt; doPipe() {
    HANDLE r, w;
    if (!::CreatePipe(&amp;r, &amp;w, nullptr, 0)) return winFailure&lt;Pipe&gt;();
    return ok(Pipe{File{r}, File{w}});
}</code></pre></details></td><td><a href="snippets/mac-1b202aa42e96.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Pipe&gt; doPipe() {
    int p[2];
    if (::pipe(p) == -1) return posixFailure&lt;Pipe&gt;();
    return ok(Pipe{File{p[0]}, File{p[1]}});
}</code></pre></details></td></tr>
<tr id="pipe2"><td><a href="CALL_SITES.md#pipe2"><code>pipe2</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `core::system::pipe2`</p><p>Nonblocking and non-inheritable properties must hold before publication; pipe plus fcntl alone is not atomically close-on-exec against concurrent spawn.</p></details></td><td><pre><code>Result&lt;Pipe&gt; doPipe2(int flags);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-2e8d8ba0733f.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Pipe&gt; doPipe2(int flags) {
    int fds[2];
    if (::pipe2(fds, flags) != 0) return posixFailure&lt;Pipe&gt;();
    return ok(Pipe{File{fds[0]}, File{fds[1]}});
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>CreateNamedPipeW/ConnectNamedPipe/CreateFileW with OVERLAPPED lifecycle for event-loop pipes; CreatePipe body above only covers synchronous pipes. On macOS pipe+fcntl cannot alone prove atomic close-on-exec against concurrent spawn.</p><p>Native building blocks: CreateNamedPipeW and CreateFileW; SetHandleInformation</p></td><td><strong>NO COMPLETE BODY</strong><p>CreateNamedPipeW/ConnectNamedPipe/CreateFileW with OVERLAPPED lifecycle for event-loop pipes; CreatePipe body above only covers synchronous pipes. On macOS pipe+fcntl cannot alone prove atomic close-on-exec against concurrent spawn.</p><p>Native building blocks: pipe and fcntl</p></td></tr>
</tbody>
</table>

## Sockets and name resolution

<table>
<thead><tr><th>syscall on POSIX/Linux</th><th>proposed API</th><th>implementation Linux</th><th>implementation Windows</th><th>implementation macOS</th></tr></thead>
<tbody>
<tr id="socket"><td><a href="CALL_SITES.md#socket"><code>socket</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::socket`; inline in test</p><p>Owned handle; INET, INET6, UNIX and Bluetooth require the feature-specific rows below; Winsock startup belongs inside platform lifetime.</p></details></td><td><pre><code>Result&lt;Socket&gt; doSocket(Family family, SocketKind kind);</code></pre><p>Concrete INET/INET6 TCP/UDP variant. WSAStartup/WSACleanup must bracket framework lifetime once. UNIX, Bluetooth, inheritance and nonblocking creation require separate operation capsules; this is not the whole original socket contract.</p></td><td><a href="snippets/linux-b085cdd85496.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Socket&gt; doSocket(Family family, SocketKind kind) {
    const int af = family == Family::ipv4 ? AF_INET : AF_INET6;
    const int type = kind == SocketKind::stream ? SOCK_STREAM : SOCK_DGRAM;
    const int s = ::socket(af, type, 0);
    if (s == -1) return posixFailure&lt;Socket&gt;();
    return ok(Socket{s});
}</code></pre></details></td><td><a href="snippets/windows-70c1fd4d68ec.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Socket&gt; doSocket(Family family, SocketKind kind) {
    const int af = family == Family::ipv4 ? AF_INET : AF_INET6;
    const int type = kind == SocketKind::stream ? SOCK_STREAM : SOCK_DGRAM;
    const SOCKET s = ::socket(af, type, 0);
    if (s == INVALID_SOCKET) return wsaFailure&lt;Socket&gt;();
    return ok(Socket{s});
}</code></pre></details></td><td><a href="snippets/mac-b085cdd85496.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Socket&gt; doSocket(Family family, SocketKind kind) {
    const int af = family == Family::ipv4 ? AF_INET : AF_INET6;
    const int type = kind == SocketKind::stream ? SOCK_STREAM : SOCK_DGRAM;
    const int s = ::socket(af, type, 0);
    if (s == -1) return posixFailure&lt;Socket&gt;();
    return ok(Socket{s});
}</code></pre></details></td></tr>
<tr id="bind"><td><a href="CALL_SITES.md#bind"><code>bind</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::bind`; inline in test</p><p>Preserve address-in-use and address-family errors; Address owns portable data, not a native sockaddr pointer.</p></details></td><td><pre><code>Status doBind(Socket s, const Address&amp; address);</code></pre><p>Address must be constructed by backend address conversion, length bounded by storage. EINPROGRESS/WSAEWOULDBLOCK is retained as a domain-tagged error for nonblocking connect.</p></td><td><a href="snippets/linux-3acd6b7b4ced.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doBind(Socket s, const Address&amp; address) {
    return socketStatus(::bind(s.native, reinterpret_cast&lt;const sockaddr*&gt;(&amp;address.storage), static_cast&lt;socklen_t&gt;(address.length)));
}</code></pre></details></td><td><a href="snippets/windows-03f70c829a1f.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doBind(Socket s, const Address&amp; address) {
    return socketStatus(::bind(s.native, reinterpret_cast&lt;const sockaddr*&gt;(&amp;address.storage), static_cast&lt;int&gt;(address.length)));
}</code></pre></details></td><td><a href="snippets/mac-3acd6b7b4ced.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doBind(Socket s, const Address&amp; address) {
    return socketStatus(::bind(s.native, reinterpret_cast&lt;const sockaddr*&gt;(&amp;address.storage), static_cast&lt;socklen_t&gt;(address.length)));
}</code></pre></details></td></tr>
<tr id="listen"><td><a href="CALL_SITES.md#listen"><code>listen</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::listen`; inline in test</p><p>Preserve existing listener activation and backlog intent.</p></details></td><td><pre><code>Status doListen(Socket s, int backlog);</code></pre><p></p></td><td><a href="snippets/linux-d6bb9f086446.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doListen(Socket s, int backlog) {
    return socketStatus(::listen(s.native, backlog));
}</code></pre></details></td><td><a href="snippets/windows-d6bb9f086446.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doListen(Socket s, int backlog) {
    return socketStatus(::listen(s.native, backlog));
}</code></pre></details></td><td><a href="snippets/mac-d6bb9f086446.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doListen(Socket s, int backlog) {
    return socketStatus(::listen(s.native, backlog));
}</code></pre></details></td></tr>
<tr id="accept"><td><a href="CALL_SITES.md#accept"><code>accept</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `core::system::accept`</p><p>Return owned accepted handle and peer address; listener remains borrowed.</p></details></td><td><pre><code>Result&lt;Socket&gt; doAccept(Socket listener);</code></pre><p>Base accept without requested flags. Peer can be obtained through doGetpeername. Flags/inheritance and asynchronous cancellation are outside this basic variant.</p></td><td><a href="snippets/linux-43e15b645689.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Socket&gt; doAccept(Socket listener) {
    const int s = ::accept(listener.native, nullptr, nullptr);
    if (s == -1) return posixFailure&lt;Socket&gt;();
    return ok(Socket{s});
}</code></pre></details></td><td><a href="snippets/windows-862427d8331c.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Socket&gt; doAccept(Socket listener) {
    const SOCKET s = ::accept(listener.native, nullptr, nullptr);
    if (s == INVALID_SOCKET) return wsaFailure&lt;Socket&gt;();
    return ok(Socket{s});
}</code></pre></details></td><td><a href="snippets/mac-43e15b645689.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Socket&gt; doAccept(Socket listener) {
    const int s = ::accept(listener.native, nullptr, nullptr);
    if (s == -1) return posixFailure&lt;Socket&gt;();
    return ok(Socket{s});
}</code></pre></details></td></tr>
<tr id="accept4"><td><a href="CALL_SITES.md#accept4"><code>accept4</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `core::system::accept4`</p><p>Same capsule as accept; requested nonblocking/non-inheritance before publication, concurrent-spawn atomicity requires proof.</p></details></td><td><pre><code>Result&lt;Socket&gt; doAccept4(Socket listener, int flags);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-babfada3786c.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Socket&gt; doAccept4(Socket listener, int flags) {
    const int fd = ::accept4(listener.native, nullptr, nullptr, flags);
    if (fd == -1) return posixFailure&lt;Socket&gt;();
    return ok(Socket{fd});
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>accept+ioctlsocket+SetHandleInformation on Windows, accept+fcntl on macOS. Full cleanup and concurrent-spawn noninheritance semantics still need an integrated accept capsule.</p><p>Native building blocks: accept or WSAAccept, ioctlsocket and SetHandleInformation</p></td><td><strong>NO COMPLETE BODY</strong><p>accept+ioctlsocket+SetHandleInformation on Windows, accept+fcntl on macOS. Full cleanup and concurrent-spawn noninheritance semantics still need an integrated accept capsule.</p><p>Native building blocks: accept and fcntl</p></td></tr>
<tr id="connect"><td><a href="CALL_SITES.md#connect"><code>connect</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::connect`; inline in test</p><p>Distinguish connected, in-progress and failure; completion still checked through readiness and socket error.</p></details></td><td><pre><code>Status doConnect(Socket s, const Address&amp; address);</code></pre><p>Address must be constructed by backend address conversion, length bounded by storage. EINPROGRESS/WSAEWOULDBLOCK is retained as a domain-tagged error for nonblocking connect.</p></td><td><a href="snippets/linux-bd17f810a34a.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doConnect(Socket s, const Address&amp; address) {
    return socketStatus(::connect(s.native, reinterpret_cast&lt;const sockaddr*&gt;(&amp;address.storage), static_cast&lt;socklen_t&gt;(address.length)));
}</code></pre></details></td><td><a href="snippets/windows-0952e185537d.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doConnect(Socket s, const Address&amp; address) {
    return socketStatus(::connect(s.native, reinterpret_cast&lt;const sockaddr*&gt;(&amp;address.storage), static_cast&lt;int&gt;(address.length)));
}</code></pre></details></td><td><a href="snippets/mac-bd17f810a34a.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doConnect(Socket s, const Address&amp; address) {
    return socketStatus(::connect(s.native, reinterpret_cast&lt;const sockaddr*&gt;(&amp;address.storage), static_cast&lt;socklen_t&gt;(address.length)));
}</code></pre></details></td></tr>
<tr id="shutdown"><td><a href="CALL_SITES.md#shutdown"><code>shutdown</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `core::system::shutdown`</p><p>Disable read, write or both without closing or changing ownership.</p></details></td><td><pre><code>Status doShutdown(Socket s, Direction direction);</code></pre><p></p></td><td><a href="snippets/linux-1892d5d34601.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doShutdown(Socket s, Direction direction) {
    const int how = direction == Direction::receive ? SHUT_RD : direction == Direction::send ? SHUT_WR : SHUT_RDWR;
    return socketStatus(::shutdown(s.native, how));
}</code></pre></details></td><td><a href="snippets/windows-69add476b7be.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doShutdown(Socket s, Direction direction) {
    const int how = direction == Direction::receive ? SD_RECEIVE : direction == Direction::send ? SD_SEND : SD_BOTH;
    return socketStatus(::shutdown(s.native, how));
}</code></pre></details></td><td><a href="snippets/mac-1892d5d34601.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doShutdown(Socket s, Direction direction) {
    const int how = direction == Direction::receive ? SHUT_RD : direction == Direction::send ? SHUT_WR : SHUT_RDWR;
    return socketStatus(::shutdown(s.native, how));
}</code></pre></details></td></tr>
<tr id="recv"><td><a href="CALL_SITES.md#recv"><code>recv</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::recv`; inline in test</p><p>Preserve EOF, partial read, would-block and out-of-band flags where requested.</p></details></td><td><pre><code>Result&lt;Count&gt; doRecv(Socket s, MutableBytes b, ReadMode mode);</code></pre><p>Normal/peek/urgent modes cover observed intent; datagram boundary handling must not silently split an oversized datagram. These bounded snippets are for stream I/O. SIGPIPE policy on POSIX remains caller lifecycle policy.</p></td><td><a href="snippets/linux-f1a6d9b6be9c.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Count&gt; doRecv(Socket s, MutableBytes b, ReadMode mode) {
    const int flags = mode == ReadMode::peek ? MSG_PEEK : mode == ReadMode::urgent ? MSG_OOB : 0;
    const auto r = ::recv(s.native, b.data(), std::min(b.size(), static_cast&lt;std::size_t&gt;(PTRDIFF_MAX)), flags);
    if (r &lt; 0) return posixFailure&lt;Count&gt;();
    return ok(static_cast&lt;Count&gt;(r));
}</code></pre></details></td><td><a href="snippets/windows-d9b12dca24c4.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Count&gt; doRecv(Socket s, MutableBytes b, ReadMode mode) {
    const int flags = mode == ReadMode::peek ? MSG_PEEK : mode == ReadMode::urgent ? MSG_OOB : 0;
    const auto n = static_cast&lt;int&gt;(std::min(b.size(), static_cast&lt;std::size_t&gt;(INT_MAX)));
    const int r = ::recv(s.native, reinterpret_cast&lt;char*&gt;(b.data()), n, flags);
    if (r == SOCKET_ERROR) return wsaFailure&lt;Count&gt;();
    return ok(static_cast&lt;Count&gt;(r));
}</code></pre></details></td><td><a href="snippets/mac-f1a6d9b6be9c.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Count&gt; doRecv(Socket s, MutableBytes b, ReadMode mode) {
    const int flags = mode == ReadMode::peek ? MSG_PEEK : mode == ReadMode::urgent ? MSG_OOB : 0;
    const auto r = ::recv(s.native, b.data(), std::min(b.size(), static_cast&lt;std::size_t&gt;(PTRDIFF_MAX)), flags);
    if (r &lt; 0) return posixFailure&lt;Count&gt;();
    return ok(static_cast&lt;Count&gt;(r));
}</code></pre></details></td></tr>
<tr id="send"><td><a href="CALL_SITES.md#send"><code>send</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::send`; inline in test</p><p>Preserve partial writes and would-block; broken pipe is reported, not an implicit process-killing signal.</p></details></td><td><pre><code>Result&lt;Count&gt; doSend(Socket s, Bytes b, WriteMode mode);</code></pre><p>Normal/peek/urgent modes cover observed intent; datagram boundary handling must not silently split an oversized datagram. These bounded snippets are for stream I/O. SIGPIPE policy on POSIX remains caller lifecycle policy.</p></td><td><a href="snippets/linux-6948e2d89e37.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Count&gt; doSend(Socket s, Bytes b, WriteMode mode) {
    const int flags = mode == WriteMode::urgent ? MSG_OOB : 0;
    const auto r = ::send(s.native, b.data(), std::min(b.size(), static_cast&lt;std::size_t&gt;(PTRDIFF_MAX)), flags);
    if (r &lt; 0) return posixFailure&lt;Count&gt;();
    return ok(static_cast&lt;Count&gt;(r));
}</code></pre></details></td><td><a href="snippets/windows-10a048aba2bb.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Count&gt; doSend(Socket s, Bytes b, WriteMode mode) {
    const int flags = mode == WriteMode::urgent ? MSG_OOB : 0;
    const auto n = static_cast&lt;int&gt;(std::min(b.size(), static_cast&lt;std::size_t&gt;(INT_MAX)));
    const int r = ::send(s.native, reinterpret_cast&lt;const char*&gt;(b.data()), n, flags);
    if (r == SOCKET_ERROR) return wsaFailure&lt;Count&gt;();
    return ok(static_cast&lt;Count&gt;(r));
}</code></pre></details></td><td><a href="snippets/mac-6948e2d89e37.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Count&gt; doSend(Socket s, Bytes b, WriteMode mode) {
    const int flags = mode == WriteMode::urgent ? MSG_OOB : 0;
    const auto r = ::send(s.native, b.data(), std::min(b.size(), static_cast&lt;std::size_t&gt;(PTRDIFF_MAX)), flags);
    if (r &lt; 0) return posixFailure&lt;Count&gt;();
    return ok(static_cast&lt;Count&gt;(r));
}</code></pre></details></td></tr>
<tr id="getsockname"><td><a href="CALL_SITES.md#getsockname"><code>getsockname</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::getsockname`; inline in test</p><p>Return owned address value with actual length and family.</p></details></td><td><pre><code>Result&lt;Address&gt; doGetsockname(Socket s);</code></pre><p></p></td><td><a href="snippets/linux-affec86ce385.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Address&gt; doGetsockname(Socket s) {
    Address a;
    socklen_t n = sizeof(a.storage);
    if (::getsockname(s.native, reinterpret_cast&lt;sockaddr*&gt;(&amp;a.storage), &amp;n) != 0) return posixFailure&lt;Address&gt;();
    a.length = n;
    return ok(a);
}</code></pre></details></td><td><a href="snippets/windows-96d273095548.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Address&gt; doGetsockname(Socket s) {
    Address a;
    int n = sizeof(a.storage);
    if (::getsockname(s.native, reinterpret_cast&lt;sockaddr*&gt;(&amp;a.storage), &amp;n) == SOCKET_ERROR) return wsaFailure&lt;Address&gt;();
    a.length = static_cast&lt;std::uint32_t&gt;(n);
    return ok(a);
}</code></pre></details></td><td><a href="snippets/mac-affec86ce385.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Address&gt; doGetsockname(Socket s) {
    Address a;
    socklen_t n = sizeof(a.storage);
    if (::getsockname(s.native, reinterpret_cast&lt;sockaddr*&gt;(&amp;a.storage), &amp;n) != 0) return posixFailure&lt;Address&gt;();
    a.length = n;
    return ok(a);
}</code></pre></details></td></tr>
<tr id="getpeername"><td><a href="CALL_SITES.md#getpeername"><code>getpeername</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `core::system::getpeername`</p><p>Preserve not-connected errors; no pointer into native temporary storage.</p></details></td><td><pre><code>Result&lt;Address&gt; doGetpeername(Socket s);</code></pre><p></p></td><td><a href="snippets/linux-f67a7398e479.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Address&gt; doGetpeername(Socket s) {
    Address a;
    socklen_t n = sizeof(a.storage);
    if (::getpeername(s.native, reinterpret_cast&lt;sockaddr*&gt;(&amp;a.storage), &amp;n) != 0) return posixFailure&lt;Address&gt;();
    a.length = n;
    return ok(a);
}</code></pre></details></td><td><a href="snippets/windows-6c1730760bcd.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Address&gt; doGetpeername(Socket s) {
    Address a;
    int n = sizeof(a.storage);
    if (::getpeername(s.native, reinterpret_cast&lt;sockaddr*&gt;(&amp;a.storage), &amp;n) == SOCKET_ERROR) return wsaFailure&lt;Address&gt;();
    a.length = static_cast&lt;std::uint32_t&gt;(n);
    return ok(a);
}</code></pre></details></td><td><a href="snippets/mac-f67a7398e479.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Address&gt; doGetpeername(Socket s) {
    Address a;
    socklen_t n = sizeof(a.storage);
    if (::getpeername(s.native, reinterpret_cast&lt;sockaddr*&gt;(&amp;a.storage), &amp;n) != 0) return posixFailure&lt;Address&gt;();
    a.length = n;
    return ok(a);
}</code></pre></details></td></tr>
<tr id="getsockopt"><td><a href="CALL_SITES.md#getsockopt"><code>getsockopt</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::getsockopt`; inline in framework, test</p><p>Typed options and ErrorCode translation, including SO_ERROR; arbitrary numeric platform options are not portable.</p></details></td><td><pre><code>Result&lt;Error&gt; doSocketError(Socket socket);</code></pre><p>This body implements SO_ERROR only. SO_DOMAIN/SO_TYPE/SO_PROTOCOL, linger, reuse and IPv6 options remain separately enumerated in platform-symbols.json; this is not a complete getsockopt replacement.</p></td><td><a href="snippets/linux-677555be6320.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Error&gt; doSocketError(Socket socket) {
    int error = 0; socklen_t size = sizeof(error);
    if (::getsockopt(socket.native, SOL_SOCKET, SO_ERROR, &amp;error, &amp;size) == -1) return posixFailure&lt;Error&gt;();
    return ok(error ? Error{ErrorDomain::posix, static_cast&lt;std::uint32_t&gt;(error)} : Error{});
}</code></pre></details></td><td><a href="snippets/windows-4c54c35348f0.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Error&gt; doSocketError(Socket socket) {
    int error = 0; int size = sizeof(error);
    if (::getsockopt(socket.native, SOL_SOCKET, SO_ERROR, reinterpret_cast&lt;char*&gt;(&amp;error), &amp;size) == SOCKET_ERROR) return wsaFailure&lt;Error&gt;();
    return ok(error ? Error{ErrorDomain::winsock, static_cast&lt;std::uint32_t&gt;(error)} : Error{});
}</code></pre></details></td><td><a href="snippets/mac-677555be6320.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Error&gt; doSocketError(Socket socket) {
    int error = 0; socklen_t size = sizeof(error);
    if (::getsockopt(socket.native, SOL_SOCKET, SO_ERROR, &amp;error, &amp;size) == -1) return posixFailure&lt;Error&gt;();
    return ok(error ? Error{ErrorDomain::posix, static_cast&lt;std::uint32_t&gt;(error)} : Error{});
}</code></pre></details></td></tr>
<tr id="setsockopt"><td><a href="CALL_SITES.md#setsockopt"><code>setsockopt</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::setsockopt`; inline in test</p><p>Audit SO_REUSEADDR, SO_REUSEPORT, TCP_NODELAY, IPV6_V6ONLY and SO_LINGER separately; no silent flag substitution.</p></details></td><td><pre><code>Status doTcpNoDelay(Socket socket, bool enabled);</code></pre><p>Includes &lt;netinet/tcp.h&gt; on POSIX. TCP_NODELAY body only; SO_REUSEADDR/SO_REUSEPORT cannot be interchanged with Windows exclusive-address semantics. Other option bodies remain an implementation gap.</p></td><td><a href="snippets/linux-c4d1ba060338.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doTcpNoDelay(Socket socket, bool enabled) {
    const int value = enabled;
    return socketStatus(::setsockopt(socket.native, IPPROTO_TCP, TCP_NODELAY, &amp;value, sizeof(value)));
}</code></pre></details></td><td><a href="snippets/windows-c9647dc6cda6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doTcpNoDelay(Socket socket, bool enabled) {
    const int value = enabled;
    return socketStatus(::setsockopt(socket.native, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast&lt;const char*&gt;(&amp;value), sizeof(value)));
}</code></pre></details></td><td><a href="snippets/mac-c4d1ba060338.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doTcpNoDelay(Socket socket, bool enabled) {
    const int value = enabled;
    return socketStatus(::setsockopt(socket.native, IPPROTO_TCP, TCP_NODELAY, &amp;value, sizeof(value)));
}</code></pre></details></td></tr>
<tr id="socketpair"><td><a href="CALL_SITES.md#socketpair"><code>socketpair</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Two owned connected byte-stream endpoints for tests; does not promise arbitrary AF_UNIX descriptor passing.</p></details></td><td><pre><code>Result&lt;std::pair&lt;Socket, Socket&gt;&gt; doSocketpair();</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-6e6110345c44.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::pair&lt;Socket, Socket&gt;&gt; doSocketpair() {
    int fds[2];
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) return posixFailure&lt;std::pair&lt;Socket, Socket&gt;&gt;();
    return ok(std::pair&lt;Socket, Socket&gt;{Socket{fds[0]}, Socket{fds[1]}});
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>No native socketpair. A Windows TCP loopback socket/bind/listen/connect/accept composition needs endpoint verification, cleanup and explicit stream-only semantics.</p><p>Native building blocks: No socketpair; socket, bind, listen, connect, accept compose a stream pair</p></td><td><a href="snippets/mac-6e6110345c44.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::pair&lt;Socket, Socket&gt;&gt; doSocketpair() {
    int fds[2];
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) return posixFailure&lt;std::pair&lt;Socket, Socket&gt;&gt;();
    return ok(std::pair&lt;Socket, Socket&gt;{Socket{fds[0]}, Socket{fds[1]}});
}</code></pre></details></td></tr>
<tr id="sendmsg"><td><a href="CALL_SITES.md#sendmsg"><code>sendmsg</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Local methods `Socket::sendFd/recvFd`; native message layout remains outside system owner</p><p>Observed operation transfers a duplicate resource reference; sender retains ownership. Windows needs an agreed IPC contract, not a fabricated syscall.</p></details></td><td><pre><code>Status doSendHandle(Socket channel, const Address&amp; destination, File resource);</code></pre><p>Sender retains ownership. UNIX datagram address stays inside the operation. No documented Windows SCM_RIGHTS counterpart; WSASendMsg does not transfer handles. A DuplicateHandle/WSADuplicateSocket protocol needs recipient identity and transport changes.</p></td><td><a href="snippets/linux-2aac97df536d.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSendHandle(Socket channel, const Address&amp; destination, File resource) {
    char marker = 0;
    iovec payload{&amp;marker, 1};
    alignas(cmsghdr) std::array&lt;std::byte, CMSG_SPACE(sizeof(int))&gt; control{};
    msghdr message{};
    message.msg_name = const_cast&lt;sockaddr_storage*&gt;(&amp;destination.storage);
    message.msg_namelen = destination.length;
    message.msg_iov = &amp;payload; message.msg_iovlen = 1;
    message.msg_control = control.data(); message.msg_controllen = control.size();
    cmsghdr* header = CMSG_FIRSTHDR(&amp;message);
    header-&gt;cmsg_level = SOL_SOCKET; header-&gt;cmsg_type = SCM_RIGHTS;
    header-&gt;cmsg_len = CMSG_LEN(sizeof(int));
    std::memcpy(CMSG_DATA(header), &amp;resource.native, sizeof(int));
    const auto sent = ::sendmsg(channel.native, &amp;message, 0);
    if (sent &lt; 0) return posixFailure();
    return sent == 1 ? ok() : posixFailure(EIO);
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Sender retains ownership. UNIX datagram address stays inside the operation. No documented Windows SCM_RIGHTS counterpart; WSASendMsg does not transfer handles. A DuplicateHandle/WSADuplicateSocket protocol needs recipient identity and transport changes.</p><p>Native building blocks: No documented AF_UNIX SCM_RIGHTS equivalent; WSASendMsg is not a descriptor-transfer replacement</p></td><td><a href="snippets/mac-2aac97df536d.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSendHandle(Socket channel, const Address&amp; destination, File resource) {
    char marker = 0;
    iovec payload{&amp;marker, 1};
    alignas(cmsghdr) std::array&lt;std::byte, CMSG_SPACE(sizeof(int))&gt; control{};
    msghdr message{};
    message.msg_name = const_cast&lt;sockaddr_storage*&gt;(&amp;destination.storage);
    message.msg_namelen = destination.length;
    message.msg_iov = &amp;payload; message.msg_iovlen = 1;
    message.msg_control = control.data(); message.msg_controllen = control.size();
    cmsghdr* header = CMSG_FIRSTHDR(&amp;message);
    header-&gt;cmsg_level = SOL_SOCKET; header-&gt;cmsg_type = SCM_RIGHTS;
    header-&gt;cmsg_len = CMSG_LEN(sizeof(int));
    std::memcpy(CMSG_DATA(header), &amp;resource.native, sizeof(int));
    const auto sent = ::sendmsg(channel.native, &amp;message, 0);
    if (sent &lt; 0) return posixFailure();
    return sent == 1 ? ok() : posixFailure(EIO);
}</code></pre></details></td></tr>
<tr id="recvmsg"><td><a href="CALL_SITES.md#recvmsg"><code>recvmsg</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Local methods `Socket::sendFd/recvFd`; native message layout remains outside system owner</p><p>Return a newly owned reference; reject truncated/malformed ancillary data. Windows transport contract is unresolved.</p></details></td><td><pre><code>Result&lt;File&gt; doReceiveHandle(Socket channel);</code></pre><p>Returns one new owned descriptor, closes received extras on invalid messages. Before production use: descriptor type detection and atomic close-on-exec policy must be completed and malformed/truncated-message cleanup validated. Windows transport remains unresolved; a stub is not an implementation.</p></td><td><a href="snippets/linux-c061d3f1c497.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doReceiveHandle(Socket channel) {
    char marker;
    iovec payload{&amp;marker, 1};
    alignas(cmsghdr) std::array&lt;std::byte, CMSG_SPACE(16 * sizeof(int))&gt; control{};
    msghdr message{};
    message.msg_iov = &amp;payload; message.msg_iovlen = 1;
    message.msg_control = control.data(); message.msg_controllen = control.size();
    const auto received = ::recvmsg(channel.native, &amp;message, 0);
    if (received &lt; 0) return posixFailure&lt;File&gt;();
    std::array&lt;int, 16&gt; fds{};
    std::size_t count = 0;
    bool malformed = false;
    for (auto* header = CMSG_FIRSTHDR(&amp;message); header; header = CMSG_NXTHDR(&amp;message, header)) {
        if (header-&gt;cmsg_level != SOL_SOCKET || header-&gt;cmsg_type != SCM_RIGHTS) continue;
        const auto* begin = reinterpret_cast&lt;const std::byte*&gt;(header);
        const auto available = static_cast&lt;std::size_t&gt;(control.data() + message.msg_controllen - begin);
        if (header-&gt;cmsg_len &lt; CMSG_LEN(0) || header-&gt;cmsg_len &gt; available) { malformed = true; break; }
        const auto bytes = header-&gt;cmsg_len - CMSG_LEN(0);
        if (bytes % sizeof(int)) malformed = true;
        for (std::size_t offset = 0; offset + sizeof(int) &lt;= bytes; offset += sizeof(int)) {
            int fd; std::memcpy(&amp;fd, reinterpret_cast&lt;const std::byte*&gt;(CMSG_DATA(header)) + offset, sizeof(fd));
            if (count &lt; fds.size()) fds[count++] = fd;
            else { ::close(fd); malformed = true; }
        }
    }
    if (received != 1 || malformed || (message.msg_flags &amp; (MSG_CTRUNC | MSG_TRUNC)) || count != 1) {
        for (std::size_t i = 0; i &lt; count; ++i) ::close(fds[i]);
        return posixFailure&lt;File&gt;(EBADMSG);
    }
    return ok(File{fds.front()});
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Returns one new owned descriptor, closes received extras on invalid messages. Before production use: descriptor type detection and atomic close-on-exec policy must be completed and malformed/truncated-message cleanup validated. Windows transport remains unresolved; a stub is not an implementation.</p><p>Native building blocks: No documented AF_UNIX SCM_RIGHTS equivalent; WSARecvMsg does not receive arbitrary UNIX descriptors</p></td><td><a href="snippets/mac-c061d3f1c497.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doReceiveHandle(Socket channel) {
    char marker;
    iovec payload{&amp;marker, 1};
    alignas(cmsghdr) std::array&lt;std::byte, CMSG_SPACE(16 * sizeof(int))&gt; control{};
    msghdr message{};
    message.msg_iov = &amp;payload; message.msg_iovlen = 1;
    message.msg_control = control.data(); message.msg_controllen = control.size();
    const auto received = ::recvmsg(channel.native, &amp;message, 0);
    if (received &lt; 0) return posixFailure&lt;File&gt;();
    std::array&lt;int, 16&gt; fds{};
    std::size_t count = 0;
    bool malformed = false;
    for (auto* header = CMSG_FIRSTHDR(&amp;message); header; header = CMSG_NXTHDR(&amp;message, header)) {
        if (header-&gt;cmsg_level != SOL_SOCKET || header-&gt;cmsg_type != SCM_RIGHTS) continue;
        const auto* begin = reinterpret_cast&lt;const std::byte*&gt;(header);
        const auto available = static_cast&lt;std::size_t&gt;(control.data() + message.msg_controllen - begin);
        if (header-&gt;cmsg_len &lt; CMSG_LEN(0) || header-&gt;cmsg_len &gt; available) { malformed = true; break; }
        const auto bytes = header-&gt;cmsg_len - CMSG_LEN(0);
        if (bytes % sizeof(int)) malformed = true;
        for (std::size_t offset = 0; offset + sizeof(int) &lt;= bytes; offset += sizeof(int)) {
            int fd; std::memcpy(&amp;fd, reinterpret_cast&lt;const std::byte*&gt;(CMSG_DATA(header)) + offset, sizeof(fd));
            if (count &lt; fds.size()) fds[count++] = fd;
            else { ::close(fd); malformed = true; }
        }
    }
    if (received != 1 || malformed || (message.msg_flags &amp; (MSG_CTRUNC | MSG_TRUNC)) || count != 1) {
        for (std::size_t i = 0; i &lt; count; ++i) ::close(fds[i]);
        return posixFailure&lt;File&gt;(EBADMSG);
    }
    return ok(File{fds.front()});
}</code></pre></details></td></tr>
<tr id="getpeereid"><td><a href="CALL_SITES.md#getpeereid"><code>getpeereid</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Local abstraction: `net::un::peerCredentials`; outside system owner</p><p>Existing net::un::peerCredentials is the authority to reshape. Native principal IDs and optional PID; never infer authenticated identity from PID alone.</p></details></td><td><pre><code>Result&lt;PeerIdentity&gt; doPeerIdentity(Socket channel);</code></pre><p>Reshape existing net::un::peerCredentials. Windows named-pipe impersonation authenticates a different transport and returns SIDs; this POSIX numeric identity output is deliberately not claimed portable.</p></td><td><a href="snippets/linux-8a7174f3373e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>struct PeerIdentity { std::uint64_t user, group; std::optional&lt;std::uint64_t&gt; process; };
Result&lt;PeerIdentity&gt; doPeerIdentity(Socket channel) {
    ucred credentials{}; socklen_t size = sizeof(credentials);
    if (::getsockopt(channel.native, SOL_SOCKET, SO_PEERCRED, &amp;credentials, &amp;size) &lt; 0) return posixFailure&lt;PeerIdentity&gt;();
    return ok(PeerIdentity{credentials.uid, credentials.gid, static_cast&lt;std::uint64_t&gt;(credentials.pid)});
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Reshape existing net::un::peerCredentials. Windows named-pipe impersonation authenticates a different transport and returns SIDs; this POSIX numeric identity output is deliberately not claimed portable.</p><p>Native building blocks: No equivalent UNIX UID/GID API; GetNamedPipeClientProcessId and ImpersonateNamedPipeClient apply to named pipes only</p></td><td><a href="snippets/mac-a8ce936ad0ec.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>struct PeerIdentity { std::uint64_t user, group; std::optional&lt;std::uint64_t&gt; process; };
Result&lt;PeerIdentity&gt; doPeerIdentity(Socket channel) {
    uid_t user; gid_t group;
    if (::getpeereid(channel.native, &amp;user, &amp;group) &lt; 0) return posixFailure&lt;PeerIdentity&gt;();
    return ok(PeerIdentity{user, group, std::nullopt});
}</code></pre></details></td></tr>
<tr id="getaddrinfo"><td><a href="CALL_SITES.md#getaddrinfo"><code>getaddrinfo</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::getaddrinfo`; inline in test</p><p>Owned result list; preserve address ordering, numeric-only flags and resolver-domain errors.</p></details></td><td><pre><code>Result&lt;std::vector&lt;Address&gt;&gt; doResolve(const char* host, const char* service, Family family, SocketKind kind, bool passive);</code></pre><p>Includes &lt;memory&gt;. This basic variant folds matching release into resolution; vector owns its data. Current numeric-only/canonical/V4-mapped/ordering policies need a ResolveOptions extension before replacing all callers; no freeaddrinfo callback escapes.</p></td><td><a href="snippets/linux-d3d21c751279.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::vector&lt;Address&gt;&gt; doResolve(const char* host, const char* service, Family family, SocketKind kind, bool passive) {
    addrinfo hints{};
    hints.ai_family = family == Family::ipv4 ? AF_INET : AF_INET6;
    hints.ai_socktype = kind == SocketKind::stream ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_flags = passive ? AI_PASSIVE : 0;
    addrinfo* result = nullptr;
    const int e = ::getaddrinfo(host, service, &amp;hints, &amp;result);
    if (e) {
        if (e == EAI_SYSTEM) return posixFailure&lt;std::vector&lt;Address&gt;&gt;();
        return fail&lt;std::vector&lt;Address&gt;&gt;(ErrorDomain::resolver, static_cast&lt;std::uint32_t&gt;(e));
    }
    std::unique_ptr&lt;addrinfo, decltype(&amp;::freeaddrinfo)&gt; guard(result, ::freeaddrinfo);
    std::vector&lt;Address&gt; addresses;
    for (auto* p = result; p; p = p-&gt;ai_next) {
        if (p-&gt;ai_addrlen &gt; sizeof(sockaddr_storage)) return posixFailure&lt;std::vector&lt;Address&gt;&gt;(EOVERFLOW);
        Address a; a.length = static_cast&lt;std::uint32_t&gt;(p-&gt;ai_addrlen);
        std::memcpy(&amp;a.storage, p-&gt;ai_addr, p-&gt;ai_addrlen);
        addresses.push_back(a);
    }
    return ok(std::move(addresses));
}</code></pre></details></td><td><a href="snippets/windows-cfe9e5882861.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::vector&lt;Address&gt;&gt; doResolve(const char* host, const char* service, Family family, SocketKind kind, bool passive) {
    auto h = toWide(host ? host : ""); auto s = toWide(service ? service : "");
    if (!h) return {{}, h.error};
    if (!s) return {{}, s.error};
    ADDRINFOW hints{};
    hints.ai_family = family == Family::ipv4 ? AF_INET : AF_INET6;
    hints.ai_socktype = kind == SocketKind::stream ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_flags = passive ? AI_PASSIVE : 0;
    ADDRINFOW* result = nullptr;
    const int e = ::GetAddrInfoW(host ? h.value.c_str() : nullptr, service ? s.value.c_str() : nullptr, &amp;hints, &amp;result);
    if (e) return fail&lt;std::vector&lt;Address&gt;&gt;(ErrorDomain::resolver, static_cast&lt;std::uint32_t&gt;(e));
    std::unique_ptr&lt;ADDRINFOW, decltype(&amp;::FreeAddrInfoW)&gt; guard(result, ::FreeAddrInfoW);
    std::vector&lt;Address&gt; addresses;
    for (auto* p = result; p; p = p-&gt;ai_next) {
        if (p-&gt;ai_addrlen &gt; sizeof(sockaddr_storage)) return winFailure&lt;std::vector&lt;Address&gt;&gt;(ERROR_BUFFER_OVERFLOW);
        Address a; a.length = static_cast&lt;std::uint32_t&gt;(p-&gt;ai_addrlen);
        std::memcpy(&amp;a.storage, p-&gt;ai_addr, p-&gt;ai_addrlen);
        addresses.push_back(a);
    }
    return ok(std::move(addresses));
}</code></pre></details></td><td><a href="snippets/mac-d3d21c751279.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::vector&lt;Address&gt;&gt; doResolve(const char* host, const char* service, Family family, SocketKind kind, bool passive) {
    addrinfo hints{};
    hints.ai_family = family == Family::ipv4 ? AF_INET : AF_INET6;
    hints.ai_socktype = kind == SocketKind::stream ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_flags = passive ? AI_PASSIVE : 0;
    addrinfo* result = nullptr;
    const int e = ::getaddrinfo(host, service, &amp;hints, &amp;result);
    if (e) {
        if (e == EAI_SYSTEM) return posixFailure&lt;std::vector&lt;Address&gt;&gt;();
        return fail&lt;std::vector&lt;Address&gt;&gt;(ErrorDomain::resolver, static_cast&lt;std::uint32_t&gt;(e));
    }
    std::unique_ptr&lt;addrinfo, decltype(&amp;::freeaddrinfo)&gt; guard(result, ::freeaddrinfo);
    std::vector&lt;Address&gt; addresses;
    for (auto* p = result; p; p = p-&gt;ai_next) {
        if (p-&gt;ai_addrlen &gt; sizeof(sockaddr_storage)) return posixFailure&lt;std::vector&lt;Address&gt;&gt;(EOVERFLOW);
        Address a; a.length = static_cast&lt;std::uint32_t&gt;(p-&gt;ai_addrlen);
        std::memcpy(&amp;a.storage, p-&gt;ai_addr, p-&gt;ai_addrlen);
        addresses.push_back(a);
    }
    return ok(std::move(addresses));
}</code></pre></details></td></tr>
<tr id="freeaddrinfo"><td><a href="CALL_SITES.md#freeaddrinfo"><code>freeaddrinfo</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::freeaddrinfo`; inline in framework, test; no extracted caller found</p><p>Release only matching owned resolver results; invalidate list; no mixed allocator free.</p></details></td><td><pre><code>Result&lt;std::vector&lt;Address&gt;&gt; doResolve(const char* host, const char* service, Family family, SocketKind kind, bool passive);</code></pre><p>Includes &lt;memory&gt;. This basic variant folds matching release into resolution; vector owns its data. Current numeric-only/canonical/V4-mapped/ordering policies need a ResolveOptions extension before replacing all callers; no freeaddrinfo callback escapes.</p></td><td><a href="snippets/linux-d3d21c751279.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::vector&lt;Address&gt;&gt; doResolve(const char* host, const char* service, Family family, SocketKind kind, bool passive) {
    addrinfo hints{};
    hints.ai_family = family == Family::ipv4 ? AF_INET : AF_INET6;
    hints.ai_socktype = kind == SocketKind::stream ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_flags = passive ? AI_PASSIVE : 0;
    addrinfo* result = nullptr;
    const int e = ::getaddrinfo(host, service, &amp;hints, &amp;result);
    if (e) {
        if (e == EAI_SYSTEM) return posixFailure&lt;std::vector&lt;Address&gt;&gt;();
        return fail&lt;std::vector&lt;Address&gt;&gt;(ErrorDomain::resolver, static_cast&lt;std::uint32_t&gt;(e));
    }
    std::unique_ptr&lt;addrinfo, decltype(&amp;::freeaddrinfo)&gt; guard(result, ::freeaddrinfo);
    std::vector&lt;Address&gt; addresses;
    for (auto* p = result; p; p = p-&gt;ai_next) {
        if (p-&gt;ai_addrlen &gt; sizeof(sockaddr_storage)) return posixFailure&lt;std::vector&lt;Address&gt;&gt;(EOVERFLOW);
        Address a; a.length = static_cast&lt;std::uint32_t&gt;(p-&gt;ai_addrlen);
        std::memcpy(&amp;a.storage, p-&gt;ai_addr, p-&gt;ai_addrlen);
        addresses.push_back(a);
    }
    return ok(std::move(addresses));
}</code></pre></details></td><td><a href="snippets/windows-cfe9e5882861.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::vector&lt;Address&gt;&gt; doResolve(const char* host, const char* service, Family family, SocketKind kind, bool passive) {
    auto h = toWide(host ? host : ""); auto s = toWide(service ? service : "");
    if (!h) return {{}, h.error};
    if (!s) return {{}, s.error};
    ADDRINFOW hints{};
    hints.ai_family = family == Family::ipv4 ? AF_INET : AF_INET6;
    hints.ai_socktype = kind == SocketKind::stream ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_flags = passive ? AI_PASSIVE : 0;
    ADDRINFOW* result = nullptr;
    const int e = ::GetAddrInfoW(host ? h.value.c_str() : nullptr, service ? s.value.c_str() : nullptr, &amp;hints, &amp;result);
    if (e) return fail&lt;std::vector&lt;Address&gt;&gt;(ErrorDomain::resolver, static_cast&lt;std::uint32_t&gt;(e));
    std::unique_ptr&lt;ADDRINFOW, decltype(&amp;::FreeAddrInfoW)&gt; guard(result, ::FreeAddrInfoW);
    std::vector&lt;Address&gt; addresses;
    for (auto* p = result; p; p = p-&gt;ai_next) {
        if (p-&gt;ai_addrlen &gt; sizeof(sockaddr_storage)) return winFailure&lt;std::vector&lt;Address&gt;&gt;(ERROR_BUFFER_OVERFLOW);
        Address a; a.length = static_cast&lt;std::uint32_t&gt;(p-&gt;ai_addrlen);
        std::memcpy(&amp;a.storage, p-&gt;ai_addr, p-&gt;ai_addrlen);
        addresses.push_back(a);
    }
    return ok(std::move(addresses));
}</code></pre></details></td><td><a href="snippets/mac-d3d21c751279.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::vector&lt;Address&gt;&gt; doResolve(const char* host, const char* service, Family family, SocketKind kind, bool passive) {
    addrinfo hints{};
    hints.ai_family = family == Family::ipv4 ? AF_INET : AF_INET6;
    hints.ai_socktype = kind == SocketKind::stream ? SOCK_STREAM : SOCK_DGRAM;
    hints.ai_flags = passive ? AI_PASSIVE : 0;
    addrinfo* result = nullptr;
    const int e = ::getaddrinfo(host, service, &amp;hints, &amp;result);
    if (e) {
        if (e == EAI_SYSTEM) return posixFailure&lt;std::vector&lt;Address&gt;&gt;();
        return fail&lt;std::vector&lt;Address&gt;&gt;(ErrorDomain::resolver, static_cast&lt;std::uint32_t&gt;(e));
    }
    std::unique_ptr&lt;addrinfo, decltype(&amp;::freeaddrinfo)&gt; guard(result, ::freeaddrinfo);
    std::vector&lt;Address&gt; addresses;
    for (auto* p = result; p; p = p-&gt;ai_next) {
        if (p-&gt;ai_addrlen &gt; sizeof(sockaddr_storage)) return posixFailure&lt;std::vector&lt;Address&gt;&gt;(EOVERFLOW);
        Address a; a.length = static_cast&lt;std::uint32_t&gt;(p-&gt;ai_addrlen);
        std::memcpy(&amp;a.storage, p-&gt;ai_addr, p-&gt;ai_addrlen);
        addresses.push_back(a);
    }
    return ok(std::move(addresses));
}</code></pre></details></td></tr>
<tr id="getnameinfo"><td><a href="CALL_SITES.md#getnameinfo"><code>getnameinfo</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::getnameinfo`; inline in framework</p><p>UTF-8 owned host/service strings; distinguish numeric and reverse lookup requests.</p></details></td><td><pre><code>Result&lt;std::pair&lt;std::string, std::string&gt;&gt; doEndpointName(const Address&amp; address, bool numeric);</code></pre><p>Basic numeric/reverse-lookup form. NI_NAMEREQD and other observed flags require semantic options before all callers can migrate.</p></td><td><a href="snippets/linux-a58a3c290e8d.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::pair&lt;std::string, std::string&gt;&gt; doEndpointName(const Address&amp; address, bool numeric) {
    char host[NI_MAXHOST], service[NI_MAXSERV];
    const int flags = numeric ? NI_NUMERICHOST | NI_NUMERICSERV : 0;
    const int e = ::getnameinfo(reinterpret_cast&lt;const sockaddr*&gt;(&amp;address.storage), address.length, host, sizeof(host), service, sizeof(service), flags);
    if (e) {
        if (e == EAI_SYSTEM) return posixFailure&lt;std::pair&lt;std::string, std::string&gt;&gt;();
        return fail&lt;std::pair&lt;std::string, std::string&gt;&gt;(ErrorDomain::resolver, static_cast&lt;std::uint32_t&gt;(e));
    }
    return ok(std::pair&lt;std::string, std::string&gt;{host, service});
}</code></pre></details></td><td><a href="snippets/windows-8ad00d0c2286.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::pair&lt;std::string, std::string&gt;&gt; doEndpointName(const Address&amp; address, bool numeric) {
    wchar_t host[NI_MAXHOST], service[NI_MAXSERV];
    const int flags = numeric ? NI_NUMERICHOST | NI_NUMERICSERV : 0;
    const int e = ::GetNameInfoW(reinterpret_cast&lt;const sockaddr*&gt;(&amp;address.storage), static_cast&lt;int&gt;(address.length), host, NI_MAXHOST, service, NI_MAXSERV, flags);
    if (e) return fail&lt;std::pair&lt;std::string, std::string&gt;&gt;(ErrorDomain::resolver, static_cast&lt;std::uint32_t&gt;(e));
    auto h = toUtf8(host); auto s = toUtf8(service);
    if (!h) return {{}, h.error};
    if (!s) return {{}, s.error};
    return ok(std::pair&lt;std::string, std::string&gt;{std::move(h.value), std::move(s.value)});
}</code></pre></details></td><td><a href="snippets/mac-a58a3c290e8d.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::pair&lt;std::string, std::string&gt;&gt; doEndpointName(const Address&amp; address, bool numeric) {
    char host[NI_MAXHOST], service[NI_MAXSERV];
    const int flags = numeric ? NI_NUMERICHOST | NI_NUMERICSERV : 0;
    const int e = ::getnameinfo(reinterpret_cast&lt;const sockaddr*&gt;(&amp;address.storage), address.length, host, sizeof(host), service, sizeof(service), flags);
    if (e) {
        if (e == EAI_SYSTEM) return posixFailure&lt;std::pair&lt;std::string, std::string&gt;&gt;();
        return fail&lt;std::pair&lt;std::string, std::string&gt;&gt;(ErrorDomain::resolver, static_cast&lt;std::uint32_t&gt;(e));
    }
    return ok(std::pair&lt;std::string, std::string&gt;{host, service});
}</code></pre></details></td></tr>
<tr id="gai_strerror"><td><a href="CALL_SITES.md#gai_strerror"><code>gai_strerror</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Retain resolver versus system error domain; never feed an EAI value to strerror.</p></details></td><td><pre><code>std::string doErrorText(const Error&amp; error);</code></pre><p>Includes &lt;system_error&gt; and &lt;memory&gt;. Returned text is owned; not a pointer into a shared mutable error buffer.</p></td><td><a href="snippets/linux-d9f080799cdb.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::string doErrorText(const Error&amp; error) {
    if (!error.text.empty()) return error.text;
    if (error.domain == ErrorDomain::resolver) return ::gai_strerror(static_cast&lt;int&gt;(error.code));
    if (error.domain == ErrorDomain::posix) return std::generic_category().message(static_cast&lt;int&gt;(error.code));
    return "native error " + std::to_string(error.code);
}</code></pre></details></td><td><a href="snippets/windows-9d0b775dccb8.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::string doErrorText(const Error&amp; error) {
    if (!error.text.empty()) return error.text;
    if (error.domain == ErrorDomain::posix) return std::generic_category().message(static_cast&lt;int&gt;(error.code));
    if (error.domain == ErrorDomain::ntstatus) return "NTSTATUS " + std::to_string(error.code);
    wchar_t* buffer = nullptr;
    const DWORD n = ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error.code, 0, reinterpret_cast&lt;wchar_t*&gt;(&amp;buffer), 0, nullptr);
    if (!n) return "native error " + std::to_string(error.code);
    std::unique_ptr&lt;wchar_t, decltype(&amp;::LocalFree)&gt; guard(buffer, ::LocalFree);
    auto text = toUtf8(std::wstring_view(buffer, n));
    return text ? std::move(text.value) : "native error " + std::to_string(error.code);
}</code></pre></details></td><td><a href="snippets/mac-d9f080799cdb.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::string doErrorText(const Error&amp; error) {
    if (!error.text.empty()) return error.text;
    if (error.domain == ErrorDomain::resolver) return ::gai_strerror(static_cast&lt;int&gt;(error.code));
    if (error.domain == ErrorDomain::posix) return std::generic_category().message(static_cast&lt;int&gt;(error.code));
    return "native error " + std::to_string(error.code);
}</code></pre></details></td></tr>
<tr id="inet_ntop"><td><a href="CALL_SITES.md#inet_ntop"><code>inet_ntop</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Numeric presentation only, no name lookup.</p></details></td><td><pre><code>Result&lt;std::string&gt; doFormatIp(const Ip&amp; ip);</code></pre><p>IP numeric text is ASCII, so InetNtopA has no pathname/codepage loss.</p></td><td><a href="snippets/linux-5aae8adbffb8.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::string&gt; doFormatIp(const Ip&amp; ip) {
    char text[INET6_ADDRSTRLEN]{};
    const int af = ip.family == Family::ipv4 ? AF_INET : AF_INET6;
    if (!::inet_ntop(af, ip.bytes.data(), text, sizeof(text))) return posixFailure&lt;std::string&gt;();
    return ok(std::string(text));
}</code></pre></details></td><td><a href="snippets/windows-8451809cdc16.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::string&gt; doFormatIp(const Ip&amp; ip) {
    char text[INET6_ADDRSTRLEN]{};
    const int af = ip.family == Family::ipv4 ? AF_INET : AF_INET6;
    if (!::InetNtopA(af, const_cast&lt;std::byte*&gt;(ip.bytes.data()), text, sizeof(text))) return wsaFailure&lt;std::string&gt;();
    return ok(std::string(text));
}</code></pre></details></td><td><a href="snippets/mac-5aae8adbffb8.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::string&gt; doFormatIp(const Ip&amp; ip) {
    char text[INET6_ADDRSTRLEN]{};
    const int af = ip.family == Family::ipv4 ? AF_INET : AF_INET6;
    if (!::inet_ntop(af, ip.bytes.data(), text, sizeof(text))) return posixFailure&lt;std::string&gt;();
    return ok(std::string(text));
}</code></pre></details></td></tr>
<tr id="inet_pton"><td><a href="CALL_SITES.md#inet_pton"><code>inet_pton</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Numeric parsing; invalid syntax is distinct from OS failure.</p></details></td><td><pre><code>Result&lt;Ip&gt; doParseIp(Family family, const std::string&amp; text);</code></pre><p>Numeric ASCII only. Reject embedded NUL at the public text boundary; never parse a truncated untrusted string.</p></td><td><a href="snippets/linux-634664c96e00.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Ip&gt; doParseIp(Family family, const std::string&amp; text) {
    Ip ip; ip.family = family;
    const int r = ::inet_pton(family == Family::ipv4 ? AF_INET : AF_INET6, text.c_str(), ip.bytes.data());
    if (r == 0) return posixFailure&lt;Ip&gt;(EINVAL);
    if (r &lt; 0) return posixFailure&lt;Ip&gt;();
    return ok(ip);
}</code></pre></details></td><td><a href="snippets/windows-ef95c92c0337.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Ip&gt; doParseIp(Family family, const std::string&amp; text) {
    Ip ip; ip.family = family;
    const int r = ::InetPtonA(family == Family::ipv4 ? AF_INET : AF_INET6, text.c_str(), ip.bytes.data());
    if (r == 0) return wsaFailure&lt;Ip&gt;(WSAEINVAL);
    if (r &lt; 0) return wsaFailure&lt;Ip&gt;();
    return ok(ip);
}</code></pre></details></td><td><a href="snippets/mac-634664c96e00.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Ip&gt; doParseIp(Family family, const std::string&amp; text) {
    Ip ip; ip.family = family;
    const int r = ::inet_pton(family == Family::ipv4 ? AF_INET : AF_INET6, text.c_str(), ip.bytes.data());
    if (r == 0) return posixFailure&lt;Ip&gt;(EINVAL);
    if (r &lt; 0) return posixFailure&lt;Ip&gt;();
    return ok(ip);
}</code></pre></details></td></tr>
</tbody>
</table>

## Readiness and signals

<table>
<thead><tr><th>syscall on POSIX/Linux</th><th>proposed API</th><th>implementation Linux</th><th>implementation Windows</th><th>implementation macOS</th></tr></thead>
<tbody>
<tr id="epoll_create1"><td><a href="CALL_SITES.md#epoll_create1"><code>epoll_create1</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `core::system::epoll_create1`</p><p>Create owned registration set; Windows backend design must cover mixed resource kinds and limits.</p></details></td><td><pre><code>Result&lt;File&gt; doEpollCreate1();</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-823375b993fa.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doEpollCreate1() {
    const int fd = ::epoll_create1(EPOLL_CLOEXEC);
    if (fd == -1) return posixFailure&lt;File&gt;();
    return ok(File{fd});
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Replace the whole event registration/wait boundary. Windows socket events/overlapped events and macOS kqueue have different native object models; no epoll descriptor can be returned.</p><p>Native building blocks: No epoll; WSACreateEvent for socket events, CreateEventW for capsule wakeups</p></td><td><strong>NO COMPLETE BODY</strong><p>Replace the whole event registration/wait boundary. Windows socket events/overlapped events and macOS kqueue have different native object models; no epoll descriptor can be returned.</p><p>Native building blocks: kqueue</p></td></tr>
<tr id="epoll_ctl"><td><a href="CALL_SITES.md#epoll_ctl"><code>epoll_ctl</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `core::system::epoll_ctl`</p><p>Add/change/remove registration without changing handle ownership; replace native event payloads with caller keys.</p></details></td><td><pre><code>Status doEpollCtl(File queue, int operation, int descriptor, epoll_event* event);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-738b5c4b9163.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doEpollCtl(File queue, int operation, int descriptor, epoll_event* event) {
    return posixStatus(::epoll_ctl(queue.native, operation, descriptor, event));
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Typed registration on kqueue/kevent or Windows WSAEventSelect plus non-socket completion state. Exposing epoll_event in the project API cannot implement all three targets.</p><p>Native building blocks: WSAEventSelect for sockets; no single control call for arbitrary resource readiness</p></td><td><strong>NO COMPLETE BODY</strong><p>Typed registration on kqueue/kevent or Windows WSAEventSelect plus non-socket completion state. Exposing epoll_event in the project API cannot implement all three targets.</p><p>Native building blocks: kevent with EV_ADD, EV_DELETE, EV_ENABLE</p></td></tr>
<tr id="epoll_wait"><td><a href="CALL_SITES.md#epoll_wait"><code>epoll_wait</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `core::system::epoll_wait`</p><p>Level/read/write/error/hangup contract; mixed descriptors, nested wait sets and scaling need feasibility proof.</p></details></td><td><pre><code>Result&lt;int&gt; doEpollWait(File queue, std::span&lt;epoll_event&gt; events, int timeoutMilliseconds);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-b9db5e2eac52.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;int&gt; doEpollWait(File queue, std::span&lt;epoll_event&gt; events, int timeoutMilliseconds) {
    if (events.size() &gt; INT_MAX) return posixFailure&lt;int&gt;(EINVAL);
    const int count = ::epoll_wait(queue.native, events.data(), static_cast&lt;int&gt;(events.size()), timeoutMilliseconds);
    if (count &lt; 0) return posixFailure&lt;int&gt;();
    return ok(count);
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>kevent on macOS; Windows WaitForMultipleObjects/WSAEnumNetworkEvents plus overlapped completions. Mixed descriptor kinds, nesting, event reset and scaling need the complete existing multiplexer boundary.</p><p>Native building blocks: WSAWaitForMultipleEvents or WaitForMultipleObjects with WSAEnumNetworkEvents; overlapped I/O events for pipes</p></td><td><strong>NO COMPLETE BODY</strong><p>kevent on macOS; Windows WaitForMultipleObjects/WSAEnumNetworkEvents plus overlapped completions. Mixed descriptor kinds, nesting, event reset and scaling need the complete existing multiplexer boundary.</p><p>Native building blocks: kevent</p></td></tr>
<tr id="epoll_pwait"><td><a href="CALL_SITES.md#epoll_pwait"><code>epoll_pwait</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `core::system::epoll_pwait`</p><p>Replace temporary native signal mask with explicit event-loop interruption policy; preserve no-lost-wakeup invariant.</p></details></td><td><pre><code>Result&lt;int&gt; doEpollPwait(File queue, std::span&lt;epoll_event&gt; events, int timeoutMilliseconds, const sigset_t* mask);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-a78b651ecf77.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;int&gt; doEpollPwait(File queue, std::span&lt;epoll_event&gt; events, int timeoutMilliseconds, const sigset_t* mask) {
    if (events.size() &gt; INT_MAX) return posixFailure&lt;int&gt;(EINVAL);
    const int count = ::epoll_pwait(queue.native, events.data(), static_cast&lt;int&gt;(events.size()), timeoutMilliseconds, mask);
    if (count &lt; 0) return posixFailure&lt;int&gt;();
    return ok(count);
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>No direct equivalent atomic signal-mask/event wait. Windows signal delivery and macOS kqueue+pselect behavior must be designed as one no-lost-wakeup operation.</p><p>Native building blocks: No atomic POSIX signal-mask wait; native events plus WaitForMultipleObjects</p></td><td><strong>NO COMPLETE BODY</strong><p>No direct equivalent atomic signal-mask/event wait. Windows signal delivery and macOS kqueue+pselect behavior must be designed as one no-lost-wakeup operation.</p><p>Native building blocks: pselect on a kqueue descriptor, then kevent; no epoll_pwait</p></td></tr>
<tr id="poll"><td><a href="CALL_SITES.md#poll"><code>poll</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::poll`; inline in framework; no extracted caller found</p><p>Mixed files, pipes, sockets and pidfd require resource-aware capsule support; WSAPoll alone is insufficient.</p></details></td><td><pre><code>Result&lt;int&gt; doPollSockets(std::span&lt;PollItem&gt; items, int timeoutMilliseconds);</code></pre><p>Normalized returned bits: read=1, write=2, urgent=4, error=8, hangup=16, invalid=32. Microsoft WSAPoll cannot supply POLLPRI; explicit error is a capability gap, not completed compatibility. This variant cannot replace pipe/pidfd/mixed-handle polling or atomic signal-mask waits.</p></td><td><a href="snippets/linux-6040396864d2.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;int&gt; doPollSockets(std::span&lt;PollItem&gt; items, int timeoutMilliseconds) {
    std::vector&lt;pollfd&gt; native;
    for (const auto&amp; item : items) native.push_back({item.socket.native, static_cast&lt;short&gt;((item.read ? POLLIN : 0) | (item.write ? POLLOUT : 0) | (item.urgent ? POLLPRI : 0)), 0});
    const int count = ::poll(native.data(), native.size(), timeoutMilliseconds);
    if (count &lt; 0) return posixFailure&lt;int&gt;();
    for (std::size_t i = 0; i &lt; items.size(); ++i) {
        const short events = native[i].revents;
        items[i].returned = static_cast&lt;short&gt;((events &amp; POLLIN ? 1 : 0) | (events &amp; POLLOUT ? 2 : 0) | (events &amp; POLLPRI ? 4 : 0) | (events &amp; POLLERR ? 8 : 0) | (events &amp; POLLHUP ? 16 : 0) | (events &amp; POLLNVAL ? 32 : 0));
    }
    return ok(count);
}</code></pre></details></td><td><a href="snippets/windows-c928f9c19965.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;int&gt; doPollSockets(std::span&lt;PollItem&gt; items, int timeoutMilliseconds) {
    if (items.empty() || items.size() &gt; ULONG_MAX) return wsaFailure&lt;int&gt;(WSAEINVAL);
    std::vector&lt;WSAPOLLFD&gt; native;
    for (const auto&amp; item : items) {
        if (item.urgent) return wsaFailure&lt;int&gt;(WSAEOPNOTSUPP);
        native.push_back({item.socket.native, static_cast&lt;short&gt;((item.read ? POLLRDNORM : 0) | (item.write ? POLLWRNORM : 0)), 0});
    }
    const int count = ::WSAPoll(native.data(), static_cast&lt;ULONG&gt;(native.size()), timeoutMilliseconds);
    if (count == SOCKET_ERROR) return wsaFailure&lt;int&gt;();
    for (std::size_t i = 0; i &lt; items.size(); ++i) {
        const short events = native[i].revents;
        items[i].returned = static_cast&lt;short&gt;((events &amp; POLLRDNORM ? 1 : 0) | (events &amp; POLLWRNORM ? 2 : 0) | (events &amp; POLLERR ? 8 : 0) | (events &amp; POLLHUP ? 16 : 0) | (events &amp; POLLNVAL ? 32 : 0));
    }
    return ok(count);
}</code></pre></details></td><td><a href="snippets/mac-6040396864d2.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;int&gt; doPollSockets(std::span&lt;PollItem&gt; items, int timeoutMilliseconds) {
    std::vector&lt;pollfd&gt; native;
    for (const auto&amp; item : items) native.push_back({item.socket.native, static_cast&lt;short&gt;((item.read ? POLLIN : 0) | (item.write ? POLLOUT : 0) | (item.urgent ? POLLPRI : 0)), 0});
    const int count = ::poll(native.data(), native.size(), timeoutMilliseconds);
    if (count &lt; 0) return posixFailure&lt;int&gt;();
    for (std::size_t i = 0; i &lt; items.size(); ++i) {
        const short events = native[i].revents;
        items[i].returned = static_cast&lt;short&gt;((events &amp; POLLIN ? 1 : 0) | (events &amp; POLLOUT ? 2 : 0) | (events &amp; POLLPRI ? 4 : 0) | (events &amp; POLLERR ? 8 : 0) | (events &amp; POLLHUP ? 16 : 0) | (events &amp; POLLNVAL ? 32 : 0));
    }
    return ok(count);
}</code></pre></details></td></tr>
<tr id="ppoll"><td><a href="CALL_SITES.md#ppoll"><code>ppoll</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `core::system::ppoll`</p><p>Preserve deadline and interrupt delivery; no claim that mask-change then poll is atomic.</p></details></td><td><pre><code>Result&lt;int&gt; doPpoll(std::span&lt;pollfd&gt; items, const timespec* timeout, const sigset_t* mask);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-dec3fc6ba087.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;int&gt; doPpoll(std::span&lt;pollfd&gt; items, const timespec* timeout, const sigset_t* mask) {
    const int count = ::ppoll(items.data(), items.size(), timeout, mask);
    if (count &lt; 0) return posixFailure&lt;int&gt;();
    return ok(count);
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>No Windows POSIX mask wait and no audited Darwin ppoll entry. A signal-aware readiness operation must replace the full surrounding sequence.</p><p>Native building blocks: No ppoll; native event waits</p></td><td><strong>NO COMPLETE BODY</strong><p>No Windows POSIX mask wait and no audited Darwin ppoll entry. A signal-aware readiness operation must replace the full surrounding sequence.</p><p>Native building blocks: No ppoll in the audited Darwin syscall list; pselect/kevent are building blocks</p></td></tr>
<tr id="select"><td><a href="CALL_SITES.md#select"><code>select</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::select`; inline in tool; no extracted caller found</p><p>Replace fd_set and maximum-integer-descriptor arithmetic in surrounding code; include ProcessRunner pipes.</p></details></td><td><pre><code>Result&lt;int&gt; doSelect(int count, fd_set* read, fd_set* write, fd_set* error, timeval* timeout);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-57f2d9e24622.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;int&gt; doSelect(int count, fd_set* read, fd_set* write, fd_set* error, timeval* timeout) {
    const int result = ::select(count, read, write, error, timeout);
    if (result &lt; 0) return posixFailure&lt;int&gt;();
    return ok(result);
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Winsock select accepts sockets only; it cannot drain ProcessRunner pipes. Use doPollSockets for the socket subset and a full child-capture operation for pipes.</p><p>Native building blocks: select for sockets only</p></td><td><a href="snippets/mac-57f2d9e24622.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;int&gt; doSelect(int count, fd_set* read, fd_set* write, fd_set* error, timeval* timeout) {
    const int result = ::select(count, read, write, error, timeout);
    if (result &lt; 0) return posixFailure&lt;int&gt;();
    return ok(result);
}</code></pre></details></td></tr>
<tr id="pselect"><td><a href="CALL_SITES.md#pselect"><code>pselect</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `core::system::pselect`</p><p>Use the same portable readiness contract, not exposed native fd_set/sigset_t.</p></details></td><td><pre><code>Result&lt;int&gt; doPselect(int count, fd_set* read, fd_set* write, fd_set* error, const timespec* timeout, const sigset_t* mask);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-bd2ff8f57e9f.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;int&gt; doPselect(int count, fd_set* read, fd_set* write, fd_set* error, const timespec* timeout, const sigset_t* mask) {
    const int result = ::pselect(count, read, write, error, timeout, mask);
    if (result &lt; 0) return posixFailure&lt;int&gt;();
    return ok(result);
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>No Windows equivalent of atomic per-thread POSIX mask switching during descriptor wait.</p><p>Native building blocks: No pselect; select plus signal-mask changes is not equivalent</p></td><td><a href="snippets/mac-bd2ff8f57e9f.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;int&gt; doPselect(int count, fd_set* read, fd_set* write, fd_set* error, const timespec* timeout, const sigset_t* mask) {
    const int result = ::pselect(count, read, write, error, timeout, mask);
    if (result &lt; 0) return posixFailure&lt;int&gt;();
    return ok(result);
}</code></pre></details></td></tr>
<tr id="sigaction"><td><a href="CALL_SITES.md#sigaction"><code>sigaction</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, tool</p><p>BLOCKED for full POSIX semantics; define termination/reload policy and restore prior subscriptions on teardown.</p></details></td><td><pre><code>Status doSigaction(int signal, const struct sigaction* action, struct sigaction* previous);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-484346733c98.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSigaction(int signal, const struct sigaction* action, struct sigaction* previous) {
    return posixStatus(::sigaction(signal, action, previous));
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Windows console-control callbacks and limited CRT signals cannot reproduce arbitrary POSIX sigaction. Propose lifecycle-event subscriptions at the whole signal owner boundary.</p><p>Native building blocks: SetConsoleCtrlHandler for console lifecycle events; CRT signal for its limited signal set</p></td><td><a href="snippets/mac-484346733c98.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSigaction(int signal, const struct sigaction* action, struct sigaction* previous) {
    return posixStatus(::sigaction(signal, action, previous));
}</code></pre></details></td></tr>
<tr id="signal"><td><a href="CALL_SITES.md#signal"><code>signal</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `utils::system::signal`; inline in framework; no extracted caller found</p><p>Replace process-wide native handler token with owned subscription; ignore/default semantics require explicit policy.</p></details></td><td><pre><code>Result&lt;void (*)(int)&gt; doSignal(int signal, void (*handler)(int));</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-97e6d1778b88.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;void (*)(int)&gt; doSignal(int signal, void (*handler)(int)) {
    const auto previous = ::signal(signal, handler);
    if (previous == SIG_ERR) return posixFailure&lt;void (*)(int)&gt;();
    return ok(previous);
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Limited CRT signal exists; the observed SIGHUP/SIGPIPE/SIGCHLD/job-control uses need separate explicit lifecycle policies. No generic same-number mapping.</p><p>Native building blocks: signal (CRT); SetConsoleCtrlHandler for console events</p></td><td><a href="snippets/mac-97e6d1778b88.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;void (*)(int)&gt; doSignal(int signal, void (*handler)(int)) {
    const auto previous = ::signal(signal, handler);
    if (previous == SIG_ERR) return posixFailure&lt;void (*)(int)&gt;();
    return ok(previous);
}</code></pre></details></td></tr>
<tr id="pthread_sigmask"><td><a href="CALL_SITES.md#pthread_sigmask"><code>pthread_sigmask</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>BLOCKED for arbitrary POSIX signals; narrowly define deferred capsule lifecycle delivery and worker-thread exclusion.</p></details></td><td><pre><code>Status doPthreadSigmask(int operation, const sigset_t* mask, sigset_t* previous);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-cbd6ea84d6e9.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doPthreadSigmask(int operation, const sigset_t* mask, sigset_t* previous) {
    const int error = ::pthread_sigmask(operation, mask, previous);
    return error ? posixFailure(error) : ok();
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>No Windows per-thread POSIX signal mask; do not pretend an event wait emulates mask semantics.</p><p>Native building blocks: No POSIX per-thread signal mask equivalent</p></td><td><a href="snippets/mac-cbd6ea84d6e9.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doPthreadSigmask(int operation, const sigset_t* mask, sigset_t* previous) {
    const int error = ::pthread_sigmask(operation, mask, previous);
    return error ? posixFailure(error) : ok();
}</code></pre></details></td></tr>
<tr id="sigemptyset"><td><a href="CALL_SITES.md#sigemptyset"><code>sigemptyset</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, test, tool</p><p>Pure value operation; not a kernel call on any target.</p></details></td><td><pre><code>Status doSigemptyset(sigset_t&amp; set);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-1c4e437ce24c.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSigemptyset(sigset_t&amp; set) {
    return posixStatus(::sigemptyset(&amp;set));
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>SignalSet must be a semantic lifecycle set once Windows delivery policy is defined; no Windows syscall for this pure value operation.</p><p>Native building blocks: No syscall; initialize portable signal set</p></td><td><a href="snippets/mac-1c4e437ce24c.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSigemptyset(sigset_t&amp; set) {
    return posixStatus(::sigemptyset(&amp;set));
}</code></pre></details></td></tr>
<tr id="sigaddset"><td><a href="CALL_SITES.md#sigaddset"><code>sigaddset</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, tool</p><p>Use semantic signal identifiers, not assumed equal native numbers.</p></details></td><td><pre><code>Status doSigaddset(sigset_t&amp; set, int signal);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-15b981caa45a.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSigaddset(sigset_t&amp; set, int signal) {
    return posixStatus(::sigaddset(&amp;set, signal));
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Same semantic SignalSet as the whole signal-delivery capsule; raw POSIX signal numbers do not port.</p><p>Native building blocks: No syscall; update portable signal set</p></td><td><a href="snippets/mac-15b981caa45a.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSigaddset(sigset_t&amp; set, int signal) {
    return posixStatus(::sigaddset(&amp;set, signal));
}</code></pre></details></td></tr>
<tr id="sigfillset"><td><a href="CALL_SITES.md#sigfillset"><code>sigfillset</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Means all supported capsule lifecycle events; cannot claim all Windows exceptions are POSIX signals.</p></details></td><td><pre><code>Status doSigfillset(sigset_t&amp; set);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-ccf59bac70c7.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSigfillset(sigset_t&amp; set) {
    return posixStatus(::sigfillset(&amp;set));
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>All supported lifecycle events must be defined; Windows exceptions are not a POSIX full signal set.</p><p>Native building blocks: No syscall; populate portable signal set</p></td><td><a href="snippets/mac-ccf59bac70c7.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSigfillset(sigset_t&amp; set) {
    return posixStatus(::sigfillset(&amp;set));
}</code></pre></details></td></tr>
<tr id="strsignal"><td><a href="CALL_SITES.md#strsignal"><code>strsignal</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `utils::system::strsignal`; inline in demo</p><p>Owned text for a semantic signal, including unknown values.</p></details></td><td><pre><code>std::string doStrsignal(int signal);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-36a35e2d54f6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::string doStrsignal(int signal) {
    const char* text = ::strsignal(signal);
    return text ? text : "unknown signal";
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Define text for the chosen semantic lifecycle events; no strsignal Win32 syscall.</p><p>Native building blocks: No strsignal; capsule-owned descriptions</p></td><td><a href="snippets/mac-36a35e2d54f6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::string doStrsignal(int signal) {
    const char* text = ::strsignal(signal);
    return text ? text : "unknown signal";
}</code></pre></details></td></tr>
<tr id="sigabbrev_np"><td><a href="CALL_SITES.md#sigabbrev_np"><code>sigabbrev_np</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `utils::system::sigabbrev_np`</p><p>Already a project helper, not a syscall; retain a single authoritative mapping.</p></details></td><td><pre><code>std::string doSigabbrevNp(int signal);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-df32699e6ec1.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::string doSigabbrevNp(int signal) {
    return utils::system::sigabbrev_np(signal);
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Existing utils::system::sigabbrev_np is the single project mapping. Reshape that mapping for semantic events; do not create three competing maps.</p><p>Native building blocks: Capsule-owned table</p></td><td><a href="snippets/mac-df32699e6ec1.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::string doSigabbrevNp(int signal) {
    return utils::system::sigabbrev_np(signal);
}</code></pre></details></td></tr>
</tbody>
</table>

## Processes and identity

<table>
<thead><tr><th>syscall on POSIX/Linux</th><th>proposed API</th><th>implementation Linux</th><th>implementation Windows</th><th>implementation macOS</th></tr></thead>
<tbody>
<tr id="fork"><td><a href="CALL_SITES.md#fork"><code>fork</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, test</p><p>Replace both daemon forks and the fork-based test helper at operation boundaries. Cannot preserve arbitrary fork continuation or copied process state on Windows.</p></details></td><td><pre><code>void doStartDaemon(const std::string&amp; pidFileName, const std::string&amp; userName, const std::string&amp; groupName);</code></pre><p>Whole existing Daemon::startDaemon operation, placed in namespace utils and using existing DaemonError/DaemonFailure/DaemonSignaled from utils/Daemon.h. Requires its current includes. Extracted verbatim except name; does not fix current PID-publication races. Windows cannot resume this C++ continuation after CreateProcessW; a re-exec/service startup contract is still required. Fork-based tests need a separate launched-child fixture.</p></td><td><a href="snippets/linux-29e3d7e39838.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


namespace utils {
    void doStartDaemon(const std::string&amp; pidFileName, const std::string&amp; userName, const std::string&amp; groupName) {
        if (std::filesystem::exists(pidFileName)) {
            throw DaemonFailure("Pid file '" + pidFileName + "' exists. Daemon already running?");
        }
        errno = 0;

        /* Fork off the parent process */
        pid_t pid = fork();
        if (pid &lt; 0) {
            /* An error occurred */
            throw DaemonError("First fork()");
        }
        if (pid &gt; 0) {
            /* Success: Let the parent terminate */
            throw DaemonSignaled("Lead new session", pid);
        }

        if (setsid() &lt; 0) {
            /* On success: The child process becomes session leader */
            throw DaemonError("setsid()");
        }

        if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
            /* Ignore signal sent from parent to child process */
            throw DaemonError("signal()");
        }

        /* Fork off for the second time*/
        pid = fork();
        if (pid &lt; 0) {
            /* An error occurred */
            throw DaemonError("Second fork()");
        }
        if (pid &gt; 0) {
            /* Success: Let the second parent terminate */
            std::ofstream pidFile(pidFileName, std::ofstream::out);

            if (!pidFile.good()) {
                kill(pid, SIGTERM);
                throw DaemonError("Writing pid file '" + pidFileName);
            }
            pidFile &lt;&lt; pid &lt;&lt; std::endl;
            pidFile.close();

            throw DaemonSignaled("Drop session lead", pid);
        }

        const struct passwd* pw = nullptr;
        const struct group* gr = nullptr;

        if (((void) (errno = 0), gr = getgrnam(groupName.c_str())) == nullptr) {
            if (errno != 0) {
                throw DaemonError("getgrnam()");
            }
            throw DaemonFailure("getgrname() group not existing");
        }
        if (setegid(gr-&gt;gr_gid) != 0) {
            throw DaemonError("setegid()");
        }
        if (((void) (errno = 0), (pw = getpwnam(userName.c_str())) == nullptr)) {
            if (errno != 0) {
                throw DaemonError("getpwnam()");
            }
            throw DaemonFailure("getpwnam() user not existing");
        }
        if (seteuid(pw-&gt;pw_uid) != 0) {
            throw DaemonError("seteuid()");
        } /* Set new file permissions */
        umask(0);
        if (chdir("/") != 0) {
            throw DaemonError("chdir()");
        }

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        if (std::freopen("/dev/null", "r", stdin) == nullptr) {
        }
        if (std::freopen("/dev/null", "w+", stdout) == nullptr) {
        }
        if (std::freopen("/dev/null", "w+", stderr) == nullptr) {
        }
    }

}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Whole existing Daemon::startDaemon operation, placed in namespace utils and using existing DaemonError/DaemonFailure/DaemonSignaled from utils/Daemon.h. Requires its current includes. Extracted verbatim except name; does not fix current PID-publication races. Windows cannot resume this C++ continuation after CreateProcessW; a re-exec/service startup contract is still required. Fork-based tests need a separate launched-child fixture.</p><p>Native building blocks: No fork equivalent; CreateProcessW starts a fresh process</p></td><td><a href="snippets/mac-29e3d7e39838.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


namespace utils {
    void doStartDaemon(const std::string&amp; pidFileName, const std::string&amp; userName, const std::string&amp; groupName) {
        if (std::filesystem::exists(pidFileName)) {
            throw DaemonFailure("Pid file '" + pidFileName + "' exists. Daemon already running?");
        }
        errno = 0;

        /* Fork off the parent process */
        pid_t pid = fork();
        if (pid &lt; 0) {
            /* An error occurred */
            throw DaemonError("First fork()");
        }
        if (pid &gt; 0) {
            /* Success: Let the parent terminate */
            throw DaemonSignaled("Lead new session", pid);
        }

        if (setsid() &lt; 0) {
            /* On success: The child process becomes session leader */
            throw DaemonError("setsid()");
        }

        if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
            /* Ignore signal sent from parent to child process */
            throw DaemonError("signal()");
        }

        /* Fork off for the second time*/
        pid = fork();
        if (pid &lt; 0) {
            /* An error occurred */
            throw DaemonError("Second fork()");
        }
        if (pid &gt; 0) {
            /* Success: Let the second parent terminate */
            std::ofstream pidFile(pidFileName, std::ofstream::out);

            if (!pidFile.good()) {
                kill(pid, SIGTERM);
                throw DaemonError("Writing pid file '" + pidFileName);
            }
            pidFile &lt;&lt; pid &lt;&lt; std::endl;
            pidFile.close();

            throw DaemonSignaled("Drop session lead", pid);
        }

        const struct passwd* pw = nullptr;
        const struct group* gr = nullptr;

        if (((void) (errno = 0), gr = getgrnam(groupName.c_str())) == nullptr) {
            if (errno != 0) {
                throw DaemonError("getgrnam()");
            }
            throw DaemonFailure("getgrname() group not existing");
        }
        if (setegid(gr-&gt;gr_gid) != 0) {
            throw DaemonError("setegid()");
        }
        if (((void) (errno = 0), (pw = getpwnam(userName.c_str())) == nullptr)) {
            if (errno != 0) {
                throw DaemonError("getpwnam()");
            }
            throw DaemonFailure("getpwnam() user not existing");
        }
        if (seteuid(pw-&gt;pw_uid) != 0) {
            throw DaemonError("seteuid()");
        } /* Set new file permissions */
        umask(0);
        if (chdir("/") != 0) {
            throw DaemonError("chdir()");
        }

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        if (std::freopen("/dev/null", "r", stdin) == nullptr) {
        }
        if (std::freopen("/dev/null", "w+", stdout) == nullptr) {
        }
        if (std::freopen("/dev/null", "w+", stderr) == nullptr) {
        }
    }

}</code></pre></details></td></tr>
<tr id="setsid"><td><a href="CALL_SITES.md#setsid"><code>setsid</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Same capsule as daemon fork sequence; define detach, startup acknowledgement and ownership explicitly.</p></details></td><td><pre><code>void doStartDaemon(const std::string&amp; pidFileName, const std::string&amp; userName, const std::string&amp; groupName);</code></pre><p>Whole existing Daemon::startDaemon operation, placed in namespace utils and using existing DaemonError/DaemonFailure/DaemonSignaled from utils/Daemon.h. Requires its current includes. Extracted verbatim except name; does not fix current PID-publication races. Windows cannot resume this C++ continuation after CreateProcessW; a re-exec/service startup contract is still required. Fork-based tests need a separate launched-child fixture.</p></td><td><a href="snippets/linux-29e3d7e39838.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


namespace utils {
    void doStartDaemon(const std::string&amp; pidFileName, const std::string&amp; userName, const std::string&amp; groupName) {
        if (std::filesystem::exists(pidFileName)) {
            throw DaemonFailure("Pid file '" + pidFileName + "' exists. Daemon already running?");
        }
        errno = 0;

        /* Fork off the parent process */
        pid_t pid = fork();
        if (pid &lt; 0) {
            /* An error occurred */
            throw DaemonError("First fork()");
        }
        if (pid &gt; 0) {
            /* Success: Let the parent terminate */
            throw DaemonSignaled("Lead new session", pid);
        }

        if (setsid() &lt; 0) {
            /* On success: The child process becomes session leader */
            throw DaemonError("setsid()");
        }

        if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
            /* Ignore signal sent from parent to child process */
            throw DaemonError("signal()");
        }

        /* Fork off for the second time*/
        pid = fork();
        if (pid &lt; 0) {
            /* An error occurred */
            throw DaemonError("Second fork()");
        }
        if (pid &gt; 0) {
            /* Success: Let the second parent terminate */
            std::ofstream pidFile(pidFileName, std::ofstream::out);

            if (!pidFile.good()) {
                kill(pid, SIGTERM);
                throw DaemonError("Writing pid file '" + pidFileName);
            }
            pidFile &lt;&lt; pid &lt;&lt; std::endl;
            pidFile.close();

            throw DaemonSignaled("Drop session lead", pid);
        }

        const struct passwd* pw = nullptr;
        const struct group* gr = nullptr;

        if (((void) (errno = 0), gr = getgrnam(groupName.c_str())) == nullptr) {
            if (errno != 0) {
                throw DaemonError("getgrnam()");
            }
            throw DaemonFailure("getgrname() group not existing");
        }
        if (setegid(gr-&gt;gr_gid) != 0) {
            throw DaemonError("setegid()");
        }
        if (((void) (errno = 0), (pw = getpwnam(userName.c_str())) == nullptr)) {
            if (errno != 0) {
                throw DaemonError("getpwnam()");
            }
            throw DaemonFailure("getpwnam() user not existing");
        }
        if (seteuid(pw-&gt;pw_uid) != 0) {
            throw DaemonError("seteuid()");
        } /* Set new file permissions */
        umask(0);
        if (chdir("/") != 0) {
            throw DaemonError("chdir()");
        }

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        if (std::freopen("/dev/null", "r", stdin) == nullptr) {
        }
        if (std::freopen("/dev/null", "w+", stdout) == nullptr) {
        }
        if (std::freopen("/dev/null", "w+", stderr) == nullptr) {
        }
    }

}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Whole existing Daemon::startDaemon operation, placed in namespace utils and using existing DaemonError/DaemonFailure/DaemonSignaled from utils/Daemon.h. Requires its current includes. Extracted verbatim except name; does not fix current PID-publication races. Windows cannot resume this C++ continuation after CreateProcessW; a re-exec/service startup contract is still required. Fork-based tests need a separate launched-child fixture.</p><p>Native building blocks: No POSIX session equivalent; CreateProcessW with DETACHED_PROCESS or CREATE_NEW_PROCESS_GROUP serves different purposes</p></td><td><a href="snippets/mac-29e3d7e39838.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


namespace utils {
    void doStartDaemon(const std::string&amp; pidFileName, const std::string&amp; userName, const std::string&amp; groupName) {
        if (std::filesystem::exists(pidFileName)) {
            throw DaemonFailure("Pid file '" + pidFileName + "' exists. Daemon already running?");
        }
        errno = 0;

        /* Fork off the parent process */
        pid_t pid = fork();
        if (pid &lt; 0) {
            /* An error occurred */
            throw DaemonError("First fork()");
        }
        if (pid &gt; 0) {
            /* Success: Let the parent terminate */
            throw DaemonSignaled("Lead new session", pid);
        }

        if (setsid() &lt; 0) {
            /* On success: The child process becomes session leader */
            throw DaemonError("setsid()");
        }

        if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
            /* Ignore signal sent from parent to child process */
            throw DaemonError("signal()");
        }

        /* Fork off for the second time*/
        pid = fork();
        if (pid &lt; 0) {
            /* An error occurred */
            throw DaemonError("Second fork()");
        }
        if (pid &gt; 0) {
            /* Success: Let the second parent terminate */
            std::ofstream pidFile(pidFileName, std::ofstream::out);

            if (!pidFile.good()) {
                kill(pid, SIGTERM);
                throw DaemonError("Writing pid file '" + pidFileName);
            }
            pidFile &lt;&lt; pid &lt;&lt; std::endl;
            pidFile.close();

            throw DaemonSignaled("Drop session lead", pid);
        }

        const struct passwd* pw = nullptr;
        const struct group* gr = nullptr;

        if (((void) (errno = 0), gr = getgrnam(groupName.c_str())) == nullptr) {
            if (errno != 0) {
                throw DaemonError("getgrnam()");
            }
            throw DaemonFailure("getgrname() group not existing");
        }
        if (setegid(gr-&gt;gr_gid) != 0) {
            throw DaemonError("setegid()");
        }
        if (((void) (errno = 0), (pw = getpwnam(userName.c_str())) == nullptr)) {
            if (errno != 0) {
                throw DaemonError("getpwnam()");
            }
            throw DaemonFailure("getpwnam() user not existing");
        }
        if (seteuid(pw-&gt;pw_uid) != 0) {
            throw DaemonError("seteuid()");
        } /* Set new file permissions */
        umask(0);
        if (chdir("/") != 0) {
            throw DaemonError("chdir()");
        }

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        if (std::freopen("/dev/null", "r", stdin) == nullptr) {
        }
        if (std::freopen("/dev/null", "w+", stdout) == nullptr) {
        }
        if (std::freopen("/dev/null", "w+", stderr) == nullptr) {
        }
    }

}</code></pre></details></td></tr>
<tr id="getpid"><td><a href="CALL_SITES.md#getpid"><code>getpid</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, test</p><p>Identifier only, not an owned process handle or proof of identity.</p></details></td><td><pre><code>std::uint64_t doProcessId();</code></pre><p></p></td><td><a href="snippets/linux-e22d587bf35f.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint64_t doProcessId() {
    return static_cast&lt;std::uint64_t&gt;(::getpid());
}</code></pre></details></td><td><a href="snippets/windows-320838fea924.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint64_t doProcessId() {
    return ::GetCurrentProcessId();
}</code></pre></details></td><td><a href="snippets/mac-e22d587bf35f.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint64_t doProcessId() {
    return static_cast&lt;std::uint64_t&gt;(::getpid());
}</code></pre></details></td></tr>
<tr id="getppid"><td><a href="CALL_SITES.md#getppid"><code>getppid</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Observed fork-test use should receive the parent identity explicitly in LaunchSpec; native query is diagnostic only.</p></details></td><td><pre><code>std::uint64_t doParentProcessId();</code></pre><p>For cross-platform fork-test replacement, pass the known parent identity in the launch operation; snapshot parent-PID queries cannot guarantee live identity.</p></td><td><a href="snippets/linux-1c9298a02dad.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint64_t doParentProcessId() {
    return static_cast&lt;std::uint64_t&gt;(::getppid());
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>For cross-platform fork-test replacement, pass the known parent identity in the launch operation; snapshot parent-PID queries cannot guarantee live identity.</p><p>Native building blocks: CreateToolhelp32Snapshot and Process32FirstW/Process32NextW; parent ID can be stale</p></td><td><a href="snippets/mac-1c9298a02dad.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint64_t doParentProcessId() {
    return static_cast&lt;std::uint64_t&gt;(::getppid());
}</code></pre></details></td></tr>
<tr id="getpgrp"><td><a href="CALL_SITES.md#getpgrp"><code>getpgrp</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>Group identity for terminal handoff; Windows job object is not a POSIX process group.</p></details></td><td><pre><code>std::uint64_t doGetpgrp();</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-42dd6256ba35.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint64_t doGetpgrp() {
    return static_cast&lt;std::uint64_t&gt;(::getpgrp());
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>No Windows POSIX foreground process group. Encapsulate the entire attached foreground job operation.</p><p>Native building blocks: No POSIX foreground process-group query</p></td><td><a href="snippets/mac-42dd6256ba35.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint64_t doGetpgrp() {
    return static_cast&lt;std::uint64_t&gt;(::getpgrp());
}</code></pre></details></td></tr>
<tr id="kill"><td><a href="CALL_SITES.md#kill"><code>kill</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, test</p><p>Separate interrupt/termination requests from forced termination; preserve the requested StopReason and do not map SIGTERM to TerminateProcess silently.</p></details></td><td><pre><code>Status doForceTerminate(Process process);</code></pre><p>Forced termination only; owned valid Process required. Graceful SIGTERM and terminal SIGINT need distinct doRequestStop(process,reason) policy, not TerminateProcess. Their source occurrences remain unresolved.</p></td><td><a href="snippets/linux-7d3d01adf47f.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doForceTerminate(Process process) {
    return posixStatus(::kill(process.native, SIGKILL));
}</code></pre></details></td><td><a href="snippets/windows-050f487c74c9.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doForceTerminate(Process process) {
    return winStatus(::TerminateProcess(process.native, 1));
}</code></pre></details></td><td><a href="snippets/mac-7d3d01adf47f.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doForceTerminate(Process process) {
    return posixStatus(::kill(process.native, SIGKILL));
}</code></pre></details></td></tr>
<tr id="syscallsys_pidfd_open"><td><a href="CALL_SITES.md#syscallsys_pidfd_open"><code>syscall:SYS_pidfd_open</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Observe exit of a stable process identity; macOS PID registration races versus Linux pidfd guarantees require explicit validation.</p></details></td><td><pre><code>Result&lt;ProcessWatch&gt; doWatchProcess(std::uint64_t process);</code></pre><p>Includes &lt;sys/syscall.h&gt; on Linux, &lt;sys/event.h&gt; on macOS. Owned watch, closed with close/CloseHandle. PID reuse between reading a PID file and acquiring the native reference remains possible on every target; macOS registration behavior needs tests. Waiting on the reference is a separate operation.</p></td><td><a href="snippets/linux-3ade5de8247b.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;ProcessWatch&gt; doWatchProcess(std::uint64_t process) {
    if (!process || process &gt; static_cast&lt;std::uint64_t&gt;(std::numeric_limits&lt;pid_t&gt;::max())) return posixFailure&lt;ProcessWatch&gt;(EINVAL);
    const int descriptor = static_cast&lt;int&gt;(::syscall(SYS_pidfd_open, static_cast&lt;pid_t&gt;(process), 0));
    if (descriptor &lt; 0) return posixFailure&lt;ProcessWatch&gt;();
    return ok(ProcessWatch{descriptor});
}</code></pre></details></td><td><a href="snippets/windows-2b9d3d785962.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;ProcessWatch&gt; doWatchProcess(std::uint64_t process) {
    if (!process || process &gt; MAXDWORD) return winFailure&lt;ProcessWatch&gt;(ERROR_INVALID_PARAMETER);
    HANDLE handle = ::OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast&lt;DWORD&gt;(process));
    if (!handle) return winFailure&lt;ProcessWatch&gt;();
    return ok(ProcessWatch{handle});
}</code></pre></details></td><td><a href="snippets/mac-0adf53adebab.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;ProcessWatch&gt; doWatchProcess(std::uint64_t process) {
    if (!process || process &gt; static_cast&lt;std::uint64_t&gt;(std::numeric_limits&lt;pid_t&gt;::max())) return posixFailure&lt;ProcessWatch&gt;(EINVAL);
    const int queue = ::kqueue();
    if (queue == -1) return posixFailure&lt;ProcessWatch&gt;();
    struct kevent change;
    EV_SET(&amp;change, process, EVFILT_PROC, EV_ADD | EV_ONESHOT, NOTE_EXIT, 0, nullptr);
    if (::kevent(queue, &amp;change, 1, nullptr, 0, nullptr) &lt; 0) {
        const int error = errno; ::close(queue); return posixFailure&lt;ProcessWatch&gt;(error);
    }
    return ok(ProcessWatch{queue});
}</code></pre></details></td></tr>
<tr id="waitpid"><td><a href="CALL_SITES.md#waitpid"><code>waitpid</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test, tool</p><p>Return exited/signaled/forced-termination status explicitly; consume/reap owned child once.</p></details></td><td><pre><code>Result&lt;ExitStatus&gt; doWaitProcess(Process process);</code></pre><p>POSIX reaps this child once; Windows process handle is still owned and must be closed separately. Windows signaled=false means no POSIX signal classification, not proof of graceful termination. No eight-bit truncation.</p></td><td><a href="snippets/linux-124eabde1e9c.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;ExitStatus&gt; doWaitProcess(Process process) {
    int status;
    if (::waitpid(process.native, &amp;status, 0) &lt; 0) return posixFailure&lt;ExitStatus&gt;();
    if (WIFEXITED(status)) return ok(ExitStatus{false, static_cast&lt;std::uint32_t&gt;(WEXITSTATUS(status)), 0});
    if (WIFSIGNALED(status)) return ok(ExitStatus{true, 0, WTERMSIG(status)});
    return posixFailure&lt;ExitStatus&gt;(ECHILD);
}</code></pre></details></td><td><a href="snippets/windows-d71f05f47dbe.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;ExitStatus&gt; doWaitProcess(Process process) {
    const DWORD state = ::WaitForSingleObject(process.native, INFINITE);
    if (state == WAIT_FAILED) return winFailure&lt;ExitStatus&gt;();
    if (state != WAIT_OBJECT_0) return winFailure&lt;ExitStatus&gt;(ERROR_INVALID_FUNCTION);
    DWORD code;
    if (!::GetExitCodeProcess(process.native, &amp;code)) return winFailure&lt;ExitStatus&gt;();
    return ok(ExitStatus{false, code, 0});
}</code></pre></details></td><td><a href="snippets/mac-124eabde1e9c.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;ExitStatus&gt; doWaitProcess(Process process) {
    int status;
    if (::waitpid(process.native, &amp;status, 0) &lt; 0) return posixFailure&lt;ExitStatus&gt;();
    if (WIFEXITED(status)) return ok(ExitStatus{false, static_cast&lt;std::uint32_t&gt;(WEXITSTATUS(status)), 0});
    if (WIFSIGNALED(status)) return ok(ExitStatus{true, 0, WTERMSIG(status)});
    return posixFailure&lt;ExitStatus&gt;(ECHILD);
}</code></pre></details></td></tr>
<tr id="posix_spawn"><td><a href="CALL_SITES.md#posix_spawn"><code>posix_spawn</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>LaunchSpec owns executable, argv, environment, standard streams and job policy; launch failure differs from child exit.</p></details></td><td><pre><code>ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);</code></pre><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p></td><td><a href="snippets/linux-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p><p>Native building blocks: CreateProcessW; CreateProcessAsUserW when a user token is required</p></td><td><a href="snippets/mac-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td></tr>
<tr id="posix_spawn_file_actions_init"><td><a href="CALL_SITES.md#posix_spawn_file_actions_init"><code>posix_spawn_file_actions_init</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>Replace native action builder with one portable launch value; no eager child creation.</p></details></td><td><pre><code>ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);</code></pre><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p></td><td><a href="snippets/linux-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p><p>Native building blocks: STARTUPINFOEXW; InitializeProcThreadAttributeList when needed</p></td><td><a href="snippets/mac-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td></tr>
<tr id="posix_spawn_file_actions_adddup2"><td><a href="CALL_SITES.md#posix_spawn_file_actions_adddup2"><code>posix_spawn_file_actions_adddup2</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>Child receives the requested stream; parent handle remains borrowed until launch completes.</p></details></td><td><pre><code>ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);</code></pre><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p></td><td><a href="snippets/linux-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p><p>Native building blocks: STARTUPINFOEXW standard handles and UpdateProcThreadAttribute(PROC_THREAD_ATTRIBUTE_HANDLE_LIST)</p></td><td><a href="snippets/mac-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td></tr>
<tr id="posix_spawn_file_actions_addclose"><td><a href="CALL_SITES.md#posix_spawn_file_actions_addclose"><code>posix_spawn_file_actions_addclose</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>Do not inherit this parent resource; no promise to close arbitrary handles in another existing process.</p></details></td><td><pre><code>ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);</code></pre><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p></td><td><a href="snippets/linux-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p><p>Native building blocks: Explicit handle inheritance list in UpdateProcThreadAttribute</p></td><td><a href="snippets/mac-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td></tr>
<tr id="posix_spawn_file_actions_destroy"><td><a href="CALL_SITES.md#posix_spawn_file_actions_destroy"><code>posix_spawn_file_actions_destroy</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>Release launch preparation resources only; never terminate the launched child.</p></details></td><td><pre><code>ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);</code></pre><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p></td><td><a href="snippets/linux-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p><p>Native building blocks: DeleteProcThreadAttributeList</p></td><td><a href="snippets/mac-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td></tr>
<tr id="posix_spawnattr_init"><td><a href="CALL_SITES.md#posix_spawnattr_init"><code>posix_spawnattr_init</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>Consolidate with the same launch value used for file actions.</p></details></td><td><pre><code>ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);</code></pre><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p></td><td><a href="snippets/linux-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p><p>Native building blocks: STARTUPINFOEXW value initialization</p></td><td><a href="snippets/mac-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td></tr>
<tr id="posix_spawnattr_setpgroup"><td><a href="CALL_SITES.md#posix_spawnattr_setpgroup"><code>posix_spawnattr_setpgroup</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>Request isolated child job or shared foreground session; capability differences are explicit.</p></details></td><td><pre><code>ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);</code></pre><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p></td><td><a href="snippets/linux-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p><p>Native building blocks: CreateProcessW(CREATE_NEW_PROCESS_GROUP); not a POSIX group</p></td><td><a href="snippets/mac-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td></tr>
<tr id="posix_spawnattr_setsigdefault"><td><a href="CALL_SITES.md#posix_spawnattr_setsigdefault"><code>posix_spawnattr_setsigdefault</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>Normal interactive child behavior must be defined per target; do not expose raw signal numbers.</p></details></td><td><pre><code>ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);</code></pre><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p></td><td><a href="snippets/linux-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p><p>Native building blocks: No arbitrary POSIX disposition equivalent</p></td><td><a href="snippets/mac-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td></tr>
<tr id="posix_spawnattr_setsigmask"><td><a href="CALL_SITES.md#posix_spawnattr_setsigmask"><code>posix_spawnattr_setsigmask</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>Same launch policy consolidates inherited mask/disposition decisions.</p></details></td><td><pre><code>ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);</code></pre><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p></td><td><a href="snippets/linux-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p><p>Native building blocks: No POSIX child signal mask equivalent</p></td><td><a href="snippets/mac-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td></tr>
<tr id="posix_spawnattr_setflags"><td><a href="CALL_SITES.md#posix_spawnattr_setflags"><code>posix_spawnattr_setflags</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>Semantic options only; no shared numeric flags across OSes.</p></details></td><td><pre><code>ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);</code></pre><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p></td><td><a href="snippets/linux-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p><p>Native building blocks: CreateProcessW creation flags and STARTUPINFOEXW</p></td><td><a href="snippets/mac-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td></tr>
<tr id="posix_spawnattr_destroy"><td><a href="CALL_SITES.md#posix_spawnattr_destroy"><code>posix_spawnattr_destroy</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>One release operation replaces both native builder lifetimes.</p></details></td><td><pre><code>ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);
ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments);</code></pre><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p></td><td><a href="snippets/linux-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Use existing snodec::control operation boundary; do not expose posix_spawn action/attribute builders in project callers. Full current source retained, including private helpers and includes. Windows captured pipes + explicit handle inheritance + quoting + attached-console lifecycle need a complete backend; CreateProcessW alone does not implement these three functions. Current source error-path weaknesses are not certified by this extraction.</p><p>Native building blocks: DeleteProcThreadAttributeList when extended attributes were allocated; no syscall for plain value destruction</p></td><td><a href="snippets/mac-0bd91c55d508.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian &lt;me@vchrist.at&gt;
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "ProcessRunner.h"

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cerrno&gt;
#include &lt;csignal&gt;
#include &lt;cstring&gt;
#include &lt;spawn.h&gt;
#include &lt;sys/select.h&gt;
#include &lt;sys/types.h&gt;
#include &lt;sys/wait.h&gt; // IWYU pragma: keep
#include &lt;unistd.h&gt;

// IWYU pragma: no_include &lt;stdlib.h&gt;

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe&amp; pipe, std::string&amp; error) {
            std::array&lt;int, 2&gt; fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe&amp; outPipe, Pipe&amp; errPipe, std::string&amp; stdOut, std::string&amp; stdErr) {
            std::array&lt;char, 4096&gt; buffer{};

            bool outOpen = outPipe.readEnd &gt;= 0;
            bool errOpen = errPipe.readEnd &gt;= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&amp;readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &amp;readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &amp;readSet, nullptr, nullptr, nullptr);
                if (selected &lt; 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen &amp;&amp; FD_ISSET(outPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdOut.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen &amp;&amp; FD_ISSET(errPipe.readEnd, &amp;readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead &gt; 0) {
                        stdErr.append(buffer.data(), static_cast&lt;std::size_t&gt;(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector&lt;char*&gt; buildArgv(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
            std::vector&lt;char*&gt; argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast&lt;char*&gt;(executable.c_str()));
            for (const std::string&amp; argument : arguments) {
                argv.push_back(const_cast&lt;char*&gt;(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array&lt;int, 5&gt; jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            struct sigaction ignoreAction{};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&amp;ignoreAction.sa_mask);

            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;ignoreAction, &amp;previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array&lt;struct sigaction, 5&gt;&amp; previous) {
            for (std::size_t i = 0; i &lt; jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &amp;previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&amp;fileActions);
        posix_spawn_file_actions_adddup2(&amp;fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&amp;fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&amp;fileActions, errPipe.writeEnd);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), &amp;fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&amp;fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &amp;status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string&amp; executable, const std::vector&lt;std::string&gt;&amp; arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array&lt;struct sigaction, 5&gt; previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector&lt;char*&gt; argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&amp;attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&amp;attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&amp;attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&amp;defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&amp;defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&amp;attr, &amp;defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&amp;emptyMask);
        posix_spawnattr_setsigmask(&amp;attr, &amp;emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&amp;attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&amp;pid, executable.c_str(), nullptr, &amp;attr, argv.data(), environ);

        posix_spawnattr_destroy(&amp;attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &amp;status, 0);
        } while (waited &lt; 0 &amp;&amp; errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control</code></pre></details></td></tr>
<tr id="system"><td><a href="CALL_SITES.md#system"><code>system</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; demo</p><p>Shell syntax is not portable. Warema command/device dependencies need their own capability decision.</p></details></td><td><pre><code>int doRunShell(const std::string&amp; command);</code></pre><p>Only invocation is portable. Shell language, exit encoding and Warema device commands are not; caller needs an operation-specific contract.</p></td><td><a href="snippets/linux-2ea915ed2faa.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>int doRunShell(const std::string&amp; command) {
    return ::system(command.c_str());
}</code></pre></details></td><td><a href="snippets/windows-256aa1a3db25.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>int doRunShell(const std::string&amp; command) {
    auto text = toWide(command);
    if (!text) { errno = EINVAL; return -1; }
    return ::_wsystem(text.value.c_str());
}</code></pre></details></td><td><a href="snippets/mac-2ea915ed2faa.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>int doRunShell(const std::string&amp; command) {
    return ::system(command.c_str());
}</code></pre></details></td></tr>
<tr id="getuid"><td><a href="CALL_SITES.md#getuid"><code>getuid</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Return principal identity; no synthetic numeric UID derived from SID.</p></details></td><td><pre><code>ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup);</code></pre><p>Whole Config.cpp:953–1065 replacement; see config-caller.cpp.txt and config-types.hpp.txt. POSIX preserves root/XDG/create-only ownership policy with copied NSS strings. Windows LocalSystem/known-folder/ACL policy is a proposed behavior, not a proven POSIX equivalence. Other uses of these calls remain in the source ledger and need the daemon/test-specific capsule as applicable.</p></td><td><a href="snippets/linux-5790162f8c9e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, config-types.hpp, &lt;pwd.h&gt;, &lt;grp.h&gt;.
// Place this function in the existing utils::system owner at adoption.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    const passwd* pw = ::getpwuid(::getuid());
    if (!pw) return out;
    // Copy NSS-owned data before any subsequent lookup can invalidate it.
    out.userName = pw-&gt;pw_name;
    const std::string accountHome = pw-&gt;pw_dir;
    const gid_t primaryGroup = pw-&gt;pw_gid;
    const group* gr = ::getgrgid(primaryGroup);
    if (!gr) return out;
    out.groupName = gr-&gt;gr_name;
    const uid_t owner = ::geteuid();
    const bool privileged = owner == 0;
    if (privileged) {
        out.configDirectory = "/etc/snode.c";
        out.logDirectory = "/var/log/snode.c";
        out.pidDirectory = "/var/run/snode.c";
    } else {
        const char* environmentHome = ::getenv("HOME");
        const std::string home = environmentHome &amp;&amp; *environmentHome == '/' ? environmentHome : accountHome;
        auto directory = [&amp;](const char* variable, const char* fallback) -&gt; Path {
            const char* value = ::getenv(variable);
            if (value &amp;&amp; *value == '/') return Path(value) / "snode.c";
            if (!home.empty() &amp;&amp; home.front() == '/') return Path(home + fallback) / "snode.c";
            return {};
        };
        out.configDirectory = directory("XDG_CONFIG_HOME", "/.config");
        out.logDirectory = directory("XDG_STATE_HOME", "/.local/state");
        out.pidDirectory = directory("XDG_RUNTIME_DIR", "/.local/run");
        if (out.configDirectory.empty() || out.logDirectory.empty() || out.pidDirectory.empty()) return out;
    }
    // Preserve current create-only permissions, failure ordering and warning policy.
    using P = std::filesystem::perms;
    auto prepare = [&amp;](const Path&amp; path, P permissions) -&gt; bool {
        if (!std::filesystem::create_directories(path)) return true;
        std::filesystem::permissions(path, permissions);
        if (privileged) {
            const group* service = ::getgrnam(serviceGroup.c_str());
            if (!service) {
                out.diagnostics.push_back("Cannot find service group '" + serviceGroup + "'. Create it and add the current user.");
                std::filesystem::remove(path);
                return false;
            }
            if (::chown(path.c_str(), owner, service-&gt;gr_gid) &lt; 0) {
                const int error = errno;
                out.diagnostics.push_back("Cannot set group ownership of '" + path.string() + "': " + std::generic_category().message(error));
            }
        }
        return true;
    };
    if (!prepare(out.configDirectory, P::owner_all | P::group_read | P::group_exec)) return out;
    if (!prepare(out.logDirectory, P::owner_all | P::group_all)) return out;
    if (!prepare(out.pidDirectory, privileged ? P::owner_all | P::group_all : P::owner_all)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td><td><a href="snippets/windows-94ae28bc38fd.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, windows-text.hpp, config-types.hpp,
// &lt;shlobj.h&gt;, &lt;sddl.h&gt;, &lt;aclapi.h&gt;.
// Link Advapi32, Shell32, Ole32. Uses process identity, not thread impersonation.
// POLICY PROPOSAL: normal users get LocalAppData/snode.c/{config,log,run};
// LocalSystem gets ProgramData/snode.c/{config,log,run}. Elevated non-System
// users remain per-user. No synthetic UID zero or GID is introduced.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    auto report = [&amp;](DWORD error) {
        out.diagnostics.push_back("Windows configuration setup failed: " + std::to_string(error));
    };
    HANDLE rawToken = nullptr;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &amp;rawToken)) {
        report(::GetLastError()); return out;
    }
    struct CloseToken { void operator()(void* p) const { ::CloseHandle(p); } };
    std::unique_ptr&lt;void, CloseToken&gt; token(rawToken);
    auto tokenInfo = [&amp;](TOKEN_INFORMATION_CLASS kind) -&gt; Result&lt;std::vector&lt;std::byte&gt;&gt; {
        DWORD size = 0;
        ::GetTokenInformation(token.get(), kind, nullptr, 0, &amp;size);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) return winFailure&lt;std::vector&lt;std::byte&gt;&gt;();
        std::vector&lt;std::byte&gt; bytes(size);
        if (!::GetTokenInformation(token.get(), kind, bytes.data(), size, &amp;size)) return winFailure&lt;std::vector&lt;std::byte&gt;&gt;();
        return ok(std::move(bytes));
    };
    auto user = tokenInfo(TokenUser);
    auto group = tokenInfo(TokenPrimaryGroup);
    if (!user || !group) { report((!user ? user.error : group.error).code); return out; }
    PSID userSid = reinterpret_cast&lt;TOKEN_USER*&gt;(user.value.data())-&gt;User.Sid;
    PSID groupSid = reinterpret_cast&lt;TOKEN_PRIMARY_GROUP*&gt;(group.value.data())-&gt;PrimaryGroup;
    auto nameFor = [&amp;](PSID sid) -&gt; Result&lt;std::string&gt; {
        DWORD nameSize = 0, domainSize = 0; SID_NAME_USE type;
        ::LookupAccountSidW(nullptr, sid, nullptr, &amp;nameSize, nullptr, &amp;domainSize, &amp;type);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) return winFailure&lt;std::string&gt;();
        std::vector&lt;wchar_t&gt; name(nameSize), domain(std::max(DWORD{1}, domainSize));
        if (!::LookupAccountSidW(nullptr, sid, name.data(), &amp;nameSize, domain.data(), &amp;domainSize, &amp;type)) return winFailure&lt;std::string&gt;();
        const std::wstring qualified = domainSize ? std::wstring(domain.data()) + L"\\" + name.data() : name.data();
        return toUtf8(qualified);
    };
    auto userName = nameFor(userSid), groupName = nameFor(groupSid);
    if (!userName || !groupName) { report((!userName ? userName.error : groupName.error).code); return out; }
    out.userName = std::move(userName.value); out.groupName = std::move(groupName.value);
    const bool service = ::IsWellKnownSid(userSid, WinLocalSystemSid) != FALSE;
    PWSTR folder = nullptr;
    const HRESULT status = ::SHGetKnownFolderPath(service ? FOLDERID_ProgramData : FOLDERID_LocalAppData, 0, nullptr, &amp;folder);
    if (FAILED(status)) { report(static_cast&lt;DWORD&gt;(status)); return out; }
    struct FreeFolder { void operator()(wchar_t* p) const { ::CoTaskMemFree(p); } };
    std::unique_ptr&lt;wchar_t, FreeFolder&gt; folderOwner(folder);
    const Path root = Path(folder) / L"snode.c";
    out.configDirectory = root / L"config";
    out.logDirectory = root / L"log";
    out.pidDirectory = root / L"run";

    // Resolve a configured service group to an actual SID. This is not a GID cast.
    std::vector&lt;std::byte&gt; serviceSid;
    if (service) {
        auto name = toWide(serviceGroup);
        if (!name) { report(name.error.code); return out; }
        DWORD sidSize = 0, domainSize = 0; SID_NAME_USE type;
        ::LookupAccountNameW(nullptr, name.value.c_str(), nullptr, &amp;sidSize, nullptr, &amp;domainSize, &amp;type);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) { report(::GetLastError()); return out; }
        serviceSid.resize(sidSize);
        std::vector&lt;wchar_t&gt; domain(std::max(DWORD{1}, domainSize));
        if (!::LookupAccountNameW(nullptr, name.value.c_str(), serviceSid.data(), &amp;sidSize, domain.data(), &amp;domainSize, &amp;type)) {
            report(::GetLastError()); return out;
        }
        if (type != SidTypeGroup &amp;&amp; type != SidTypeAlias &amp;&amp; type != SidTypeWellKnownGroup) {
            report(ERROR_INVALID_PRIMARY_GROUP); return out;
        }
    }
    auto sidText = [&amp;](PSID sid) -&gt; std::wstring {
        LPWSTR value = nullptr;
        if (!::ConvertSidToStringSidW(sid, &amp;value)) return {};
        std::unique_ptr&lt;wchar_t, decltype(&amp;::LocalFree)&gt; guard(value, ::LocalFree);
        return value;
    };
    const auto owner = sidText(userSid);
    const auto shared = service ? sidText(serviceSid.data()) : std::wstring{};
    if (owner.empty() || (service &amp;&amp; shared.empty())) { report(::GetLastError()); return out; }
    auto create = [&amp;](const Path&amp; path, bool readOnlyGroup) -&gt; bool {
        // Protected DACL: owner and SYSTEM full access. Service group read/execute
        // on config, modify on log/run. OI/CI apply to new descendants.
        std::wstring sddl = L"D:P(A;OICI;FA;;;" + owner + L")(A;OICI;FA;;;SY)";
        if (service) sddl += L"(A;OICI;" + std::wstring(readOnlyGroup ? L"0x1200a9" : L"0x1301bf") + L";;;" + shared + L")";
        PSECURITY_DESCRIPTOR descriptor = nullptr;
        if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &amp;descriptor, nullptr)) {
            report(::GetLastError()); return false;
        }
        std::unique_ptr&lt;void, decltype(&amp;::LocalFree)&gt; descriptorOwner(descriptor, ::LocalFree);
        SECURITY_ATTRIBUTES attributes{sizeof(attributes), descriptor, FALSE};
        if (::CreateDirectoryW(path.c_str(), &amp;attributes)) return true;
        const DWORD error = ::GetLastError();
        if (error != ERROR_ALREADY_EXISTS) { report(error); return false; }
        const DWORD attrs = ::GetFileAttributesW(path.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) { report(::GetLastError()); return false; }
        // Existing directories retain permissions; reject reparse redirection.
        if (!(attrs &amp; FILE_ATTRIBUTE_DIRECTORY) || (attrs &amp; FILE_ATTRIBUTE_REPARSE_POINT)) {
            report(ERROR_ACCESS_DENIED); return false;
        }
        return true;
    };
    // Known-folder parent exists. Create root before the three children.
    if (!create(root, false) || !create(out.configDirectory, true) || !create(out.logDirectory, false) || !create(out.pidDirectory, false)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td><td><a href="snippets/mac-5790162f8c9e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, config-types.hpp, &lt;pwd.h&gt;, &lt;grp.h&gt;.
// Place this function in the existing utils::system owner at adoption.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    const passwd* pw = ::getpwuid(::getuid());
    if (!pw) return out;
    // Copy NSS-owned data before any subsequent lookup can invalidate it.
    out.userName = pw-&gt;pw_name;
    const std::string accountHome = pw-&gt;pw_dir;
    const gid_t primaryGroup = pw-&gt;pw_gid;
    const group* gr = ::getgrgid(primaryGroup);
    if (!gr) return out;
    out.groupName = gr-&gt;gr_name;
    const uid_t owner = ::geteuid();
    const bool privileged = owner == 0;
    if (privileged) {
        out.configDirectory = "/etc/snode.c";
        out.logDirectory = "/var/log/snode.c";
        out.pidDirectory = "/var/run/snode.c";
    } else {
        const char* environmentHome = ::getenv("HOME");
        const std::string home = environmentHome &amp;&amp; *environmentHome == '/' ? environmentHome : accountHome;
        auto directory = [&amp;](const char* variable, const char* fallback) -&gt; Path {
            const char* value = ::getenv(variable);
            if (value &amp;&amp; *value == '/') return Path(value) / "snode.c";
            if (!home.empty() &amp;&amp; home.front() == '/') return Path(home + fallback) / "snode.c";
            return {};
        };
        out.configDirectory = directory("XDG_CONFIG_HOME", "/.config");
        out.logDirectory = directory("XDG_STATE_HOME", "/.local/state");
        out.pidDirectory = directory("XDG_RUNTIME_DIR", "/.local/run");
        if (out.configDirectory.empty() || out.logDirectory.empty() || out.pidDirectory.empty()) return out;
    }
    // Preserve current create-only permissions, failure ordering and warning policy.
    using P = std::filesystem::perms;
    auto prepare = [&amp;](const Path&amp; path, P permissions) -&gt; bool {
        if (!std::filesystem::create_directories(path)) return true;
        std::filesystem::permissions(path, permissions);
        if (privileged) {
            const group* service = ::getgrnam(serviceGroup.c_str());
            if (!service) {
                out.diagnostics.push_back("Cannot find service group '" + serviceGroup + "'. Create it and add the current user.");
                std::filesystem::remove(path);
                return false;
            }
            if (::chown(path.c_str(), owner, service-&gt;gr_gid) &lt; 0) {
                const int error = errno;
                out.diagnostics.push_back("Cannot set group ownership of '" + path.string() + "': " + std::generic_category().message(error));
            }
        }
        return true;
    };
    if (!prepare(out.configDirectory, P::owner_all | P::group_read | P::group_exec)) return out;
    if (!prepare(out.logDirectory, P::owner_all | P::group_all)) return out;
    if (!prepare(out.pidDirectory, privileged ? P::owner_all | P::group_all : P::owner_all)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td></tr>
<tr id="geteuid"><td><a href="CALL_SITES.md#geteuid"><code>geteuid</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, test</p><p>Include impersonation context; administrator membership does not mean Unix UID zero.</p></details></td><td><pre><code>ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup);</code></pre><p>Whole Config.cpp:953–1065 replacement; see config-caller.cpp.txt and config-types.hpp.txt. POSIX preserves root/XDG/create-only ownership policy with copied NSS strings. Windows LocalSystem/known-folder/ACL policy is a proposed behavior, not a proven POSIX equivalence. Other uses of these calls remain in the source ledger and need the daemon/test-specific capsule as applicable.</p></td><td><a href="snippets/linux-5790162f8c9e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, config-types.hpp, &lt;pwd.h&gt;, &lt;grp.h&gt;.
// Place this function in the existing utils::system owner at adoption.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    const passwd* pw = ::getpwuid(::getuid());
    if (!pw) return out;
    // Copy NSS-owned data before any subsequent lookup can invalidate it.
    out.userName = pw-&gt;pw_name;
    const std::string accountHome = pw-&gt;pw_dir;
    const gid_t primaryGroup = pw-&gt;pw_gid;
    const group* gr = ::getgrgid(primaryGroup);
    if (!gr) return out;
    out.groupName = gr-&gt;gr_name;
    const uid_t owner = ::geteuid();
    const bool privileged = owner == 0;
    if (privileged) {
        out.configDirectory = "/etc/snode.c";
        out.logDirectory = "/var/log/snode.c";
        out.pidDirectory = "/var/run/snode.c";
    } else {
        const char* environmentHome = ::getenv("HOME");
        const std::string home = environmentHome &amp;&amp; *environmentHome == '/' ? environmentHome : accountHome;
        auto directory = [&amp;](const char* variable, const char* fallback) -&gt; Path {
            const char* value = ::getenv(variable);
            if (value &amp;&amp; *value == '/') return Path(value) / "snode.c";
            if (!home.empty() &amp;&amp; home.front() == '/') return Path(home + fallback) / "snode.c";
            return {};
        };
        out.configDirectory = directory("XDG_CONFIG_HOME", "/.config");
        out.logDirectory = directory("XDG_STATE_HOME", "/.local/state");
        out.pidDirectory = directory("XDG_RUNTIME_DIR", "/.local/run");
        if (out.configDirectory.empty() || out.logDirectory.empty() || out.pidDirectory.empty()) return out;
    }
    // Preserve current create-only permissions, failure ordering and warning policy.
    using P = std::filesystem::perms;
    auto prepare = [&amp;](const Path&amp; path, P permissions) -&gt; bool {
        if (!std::filesystem::create_directories(path)) return true;
        std::filesystem::permissions(path, permissions);
        if (privileged) {
            const group* service = ::getgrnam(serviceGroup.c_str());
            if (!service) {
                out.diagnostics.push_back("Cannot find service group '" + serviceGroup + "'. Create it and add the current user.");
                std::filesystem::remove(path);
                return false;
            }
            if (::chown(path.c_str(), owner, service-&gt;gr_gid) &lt; 0) {
                const int error = errno;
                out.diagnostics.push_back("Cannot set group ownership of '" + path.string() + "': " + std::generic_category().message(error));
            }
        }
        return true;
    };
    if (!prepare(out.configDirectory, P::owner_all | P::group_read | P::group_exec)) return out;
    if (!prepare(out.logDirectory, P::owner_all | P::group_all)) return out;
    if (!prepare(out.pidDirectory, privileged ? P::owner_all | P::group_all : P::owner_all)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td><td><a href="snippets/windows-94ae28bc38fd.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, windows-text.hpp, config-types.hpp,
// &lt;shlobj.h&gt;, &lt;sddl.h&gt;, &lt;aclapi.h&gt;.
// Link Advapi32, Shell32, Ole32. Uses process identity, not thread impersonation.
// POLICY PROPOSAL: normal users get LocalAppData/snode.c/{config,log,run};
// LocalSystem gets ProgramData/snode.c/{config,log,run}. Elevated non-System
// users remain per-user. No synthetic UID zero or GID is introduced.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    auto report = [&amp;](DWORD error) {
        out.diagnostics.push_back("Windows configuration setup failed: " + std::to_string(error));
    };
    HANDLE rawToken = nullptr;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &amp;rawToken)) {
        report(::GetLastError()); return out;
    }
    struct CloseToken { void operator()(void* p) const { ::CloseHandle(p); } };
    std::unique_ptr&lt;void, CloseToken&gt; token(rawToken);
    auto tokenInfo = [&amp;](TOKEN_INFORMATION_CLASS kind) -&gt; Result&lt;std::vector&lt;std::byte&gt;&gt; {
        DWORD size = 0;
        ::GetTokenInformation(token.get(), kind, nullptr, 0, &amp;size);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) return winFailure&lt;std::vector&lt;std::byte&gt;&gt;();
        std::vector&lt;std::byte&gt; bytes(size);
        if (!::GetTokenInformation(token.get(), kind, bytes.data(), size, &amp;size)) return winFailure&lt;std::vector&lt;std::byte&gt;&gt;();
        return ok(std::move(bytes));
    };
    auto user = tokenInfo(TokenUser);
    auto group = tokenInfo(TokenPrimaryGroup);
    if (!user || !group) { report((!user ? user.error : group.error).code); return out; }
    PSID userSid = reinterpret_cast&lt;TOKEN_USER*&gt;(user.value.data())-&gt;User.Sid;
    PSID groupSid = reinterpret_cast&lt;TOKEN_PRIMARY_GROUP*&gt;(group.value.data())-&gt;PrimaryGroup;
    auto nameFor = [&amp;](PSID sid) -&gt; Result&lt;std::string&gt; {
        DWORD nameSize = 0, domainSize = 0; SID_NAME_USE type;
        ::LookupAccountSidW(nullptr, sid, nullptr, &amp;nameSize, nullptr, &amp;domainSize, &amp;type);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) return winFailure&lt;std::string&gt;();
        std::vector&lt;wchar_t&gt; name(nameSize), domain(std::max(DWORD{1}, domainSize));
        if (!::LookupAccountSidW(nullptr, sid, name.data(), &amp;nameSize, domain.data(), &amp;domainSize, &amp;type)) return winFailure&lt;std::string&gt;();
        const std::wstring qualified = domainSize ? std::wstring(domain.data()) + L"\\" + name.data() : name.data();
        return toUtf8(qualified);
    };
    auto userName = nameFor(userSid), groupName = nameFor(groupSid);
    if (!userName || !groupName) { report((!userName ? userName.error : groupName.error).code); return out; }
    out.userName = std::move(userName.value); out.groupName = std::move(groupName.value);
    const bool service = ::IsWellKnownSid(userSid, WinLocalSystemSid) != FALSE;
    PWSTR folder = nullptr;
    const HRESULT status = ::SHGetKnownFolderPath(service ? FOLDERID_ProgramData : FOLDERID_LocalAppData, 0, nullptr, &amp;folder);
    if (FAILED(status)) { report(static_cast&lt;DWORD&gt;(status)); return out; }
    struct FreeFolder { void operator()(wchar_t* p) const { ::CoTaskMemFree(p); } };
    std::unique_ptr&lt;wchar_t, FreeFolder&gt; folderOwner(folder);
    const Path root = Path(folder) / L"snode.c";
    out.configDirectory = root / L"config";
    out.logDirectory = root / L"log";
    out.pidDirectory = root / L"run";

    // Resolve a configured service group to an actual SID. This is not a GID cast.
    std::vector&lt;std::byte&gt; serviceSid;
    if (service) {
        auto name = toWide(serviceGroup);
        if (!name) { report(name.error.code); return out; }
        DWORD sidSize = 0, domainSize = 0; SID_NAME_USE type;
        ::LookupAccountNameW(nullptr, name.value.c_str(), nullptr, &amp;sidSize, nullptr, &amp;domainSize, &amp;type);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) { report(::GetLastError()); return out; }
        serviceSid.resize(sidSize);
        std::vector&lt;wchar_t&gt; domain(std::max(DWORD{1}, domainSize));
        if (!::LookupAccountNameW(nullptr, name.value.c_str(), serviceSid.data(), &amp;sidSize, domain.data(), &amp;domainSize, &amp;type)) {
            report(::GetLastError()); return out;
        }
        if (type != SidTypeGroup &amp;&amp; type != SidTypeAlias &amp;&amp; type != SidTypeWellKnownGroup) {
            report(ERROR_INVALID_PRIMARY_GROUP); return out;
        }
    }
    auto sidText = [&amp;](PSID sid) -&gt; std::wstring {
        LPWSTR value = nullptr;
        if (!::ConvertSidToStringSidW(sid, &amp;value)) return {};
        std::unique_ptr&lt;wchar_t, decltype(&amp;::LocalFree)&gt; guard(value, ::LocalFree);
        return value;
    };
    const auto owner = sidText(userSid);
    const auto shared = service ? sidText(serviceSid.data()) : std::wstring{};
    if (owner.empty() || (service &amp;&amp; shared.empty())) { report(::GetLastError()); return out; }
    auto create = [&amp;](const Path&amp; path, bool readOnlyGroup) -&gt; bool {
        // Protected DACL: owner and SYSTEM full access. Service group read/execute
        // on config, modify on log/run. OI/CI apply to new descendants.
        std::wstring sddl = L"D:P(A;OICI;FA;;;" + owner + L")(A;OICI;FA;;;SY)";
        if (service) sddl += L"(A;OICI;" + std::wstring(readOnlyGroup ? L"0x1200a9" : L"0x1301bf") + L";;;" + shared + L")";
        PSECURITY_DESCRIPTOR descriptor = nullptr;
        if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &amp;descriptor, nullptr)) {
            report(::GetLastError()); return false;
        }
        std::unique_ptr&lt;void, decltype(&amp;::LocalFree)&gt; descriptorOwner(descriptor, ::LocalFree);
        SECURITY_ATTRIBUTES attributes{sizeof(attributes), descriptor, FALSE};
        if (::CreateDirectoryW(path.c_str(), &amp;attributes)) return true;
        const DWORD error = ::GetLastError();
        if (error != ERROR_ALREADY_EXISTS) { report(error); return false; }
        const DWORD attrs = ::GetFileAttributesW(path.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) { report(::GetLastError()); return false; }
        // Existing directories retain permissions; reject reparse redirection.
        if (!(attrs &amp; FILE_ATTRIBUTE_DIRECTORY) || (attrs &amp; FILE_ATTRIBUTE_REPARSE_POINT)) {
            report(ERROR_ACCESS_DENIED); return false;
        }
        return true;
    };
    // Known-folder parent exists. Create root before the three children.
    if (!create(root, false) || !create(out.configDirectory, true) || !create(out.logDirectory, false) || !create(out.pidDirectory, false)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td><td><a href="snippets/mac-5790162f8c9e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, config-types.hpp, &lt;pwd.h&gt;, &lt;grp.h&gt;.
// Place this function in the existing utils::system owner at adoption.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    const passwd* pw = ::getpwuid(::getuid());
    if (!pw) return out;
    // Copy NSS-owned data before any subsequent lookup can invalidate it.
    out.userName = pw-&gt;pw_name;
    const std::string accountHome = pw-&gt;pw_dir;
    const gid_t primaryGroup = pw-&gt;pw_gid;
    const group* gr = ::getgrgid(primaryGroup);
    if (!gr) return out;
    out.groupName = gr-&gt;gr_name;
    const uid_t owner = ::geteuid();
    const bool privileged = owner == 0;
    if (privileged) {
        out.configDirectory = "/etc/snode.c";
        out.logDirectory = "/var/log/snode.c";
        out.pidDirectory = "/var/run/snode.c";
    } else {
        const char* environmentHome = ::getenv("HOME");
        const std::string home = environmentHome &amp;&amp; *environmentHome == '/' ? environmentHome : accountHome;
        auto directory = [&amp;](const char* variable, const char* fallback) -&gt; Path {
            const char* value = ::getenv(variable);
            if (value &amp;&amp; *value == '/') return Path(value) / "snode.c";
            if (!home.empty() &amp;&amp; home.front() == '/') return Path(home + fallback) / "snode.c";
            return {};
        };
        out.configDirectory = directory("XDG_CONFIG_HOME", "/.config");
        out.logDirectory = directory("XDG_STATE_HOME", "/.local/state");
        out.pidDirectory = directory("XDG_RUNTIME_DIR", "/.local/run");
        if (out.configDirectory.empty() || out.logDirectory.empty() || out.pidDirectory.empty()) return out;
    }
    // Preserve current create-only permissions, failure ordering and warning policy.
    using P = std::filesystem::perms;
    auto prepare = [&amp;](const Path&amp; path, P permissions) -&gt; bool {
        if (!std::filesystem::create_directories(path)) return true;
        std::filesystem::permissions(path, permissions);
        if (privileged) {
            const group* service = ::getgrnam(serviceGroup.c_str());
            if (!service) {
                out.diagnostics.push_back("Cannot find service group '" + serviceGroup + "'. Create it and add the current user.");
                std::filesystem::remove(path);
                return false;
            }
            if (::chown(path.c_str(), owner, service-&gt;gr_gid) &lt; 0) {
                const int error = errno;
                out.diagnostics.push_back("Cannot set group ownership of '" + path.string() + "': " + std::generic_category().message(error));
            }
        }
        return true;
    };
    if (!prepare(out.configDirectory, P::owner_all | P::group_read | P::group_exec)) return out;
    if (!prepare(out.logDirectory, P::owner_all | P::group_all)) return out;
    if (!prepare(out.pidDirectory, privileged ? P::owner_all | P::group_all : P::owner_all)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td></tr>
<tr id="getgid"><td><a href="CALL_SITES.md#getgid"><code>getgid</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Windows primary group is not a POSIX authorization model; retain domain tag.</p></details></td><td><pre><code>ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup);</code></pre><p>Whole Config.cpp:953–1065 replacement; see config-caller.cpp.txt and config-types.hpp.txt. POSIX preserves root/XDG/create-only ownership policy with copied NSS strings. Windows LocalSystem/known-folder/ACL policy is a proposed behavior, not a proven POSIX equivalence. Other uses of these calls remain in the source ledger and need the daemon/test-specific capsule as applicable.</p></td><td><a href="snippets/linux-5790162f8c9e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, config-types.hpp, &lt;pwd.h&gt;, &lt;grp.h&gt;.
// Place this function in the existing utils::system owner at adoption.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    const passwd* pw = ::getpwuid(::getuid());
    if (!pw) return out;
    // Copy NSS-owned data before any subsequent lookup can invalidate it.
    out.userName = pw-&gt;pw_name;
    const std::string accountHome = pw-&gt;pw_dir;
    const gid_t primaryGroup = pw-&gt;pw_gid;
    const group* gr = ::getgrgid(primaryGroup);
    if (!gr) return out;
    out.groupName = gr-&gt;gr_name;
    const uid_t owner = ::geteuid();
    const bool privileged = owner == 0;
    if (privileged) {
        out.configDirectory = "/etc/snode.c";
        out.logDirectory = "/var/log/snode.c";
        out.pidDirectory = "/var/run/snode.c";
    } else {
        const char* environmentHome = ::getenv("HOME");
        const std::string home = environmentHome &amp;&amp; *environmentHome == '/' ? environmentHome : accountHome;
        auto directory = [&amp;](const char* variable, const char* fallback) -&gt; Path {
            const char* value = ::getenv(variable);
            if (value &amp;&amp; *value == '/') return Path(value) / "snode.c";
            if (!home.empty() &amp;&amp; home.front() == '/') return Path(home + fallback) / "snode.c";
            return {};
        };
        out.configDirectory = directory("XDG_CONFIG_HOME", "/.config");
        out.logDirectory = directory("XDG_STATE_HOME", "/.local/state");
        out.pidDirectory = directory("XDG_RUNTIME_DIR", "/.local/run");
        if (out.configDirectory.empty() || out.logDirectory.empty() || out.pidDirectory.empty()) return out;
    }
    // Preserve current create-only permissions, failure ordering and warning policy.
    using P = std::filesystem::perms;
    auto prepare = [&amp;](const Path&amp; path, P permissions) -&gt; bool {
        if (!std::filesystem::create_directories(path)) return true;
        std::filesystem::permissions(path, permissions);
        if (privileged) {
            const group* service = ::getgrnam(serviceGroup.c_str());
            if (!service) {
                out.diagnostics.push_back("Cannot find service group '" + serviceGroup + "'. Create it and add the current user.");
                std::filesystem::remove(path);
                return false;
            }
            if (::chown(path.c_str(), owner, service-&gt;gr_gid) &lt; 0) {
                const int error = errno;
                out.diagnostics.push_back("Cannot set group ownership of '" + path.string() + "': " + std::generic_category().message(error));
            }
        }
        return true;
    };
    if (!prepare(out.configDirectory, P::owner_all | P::group_read | P::group_exec)) return out;
    if (!prepare(out.logDirectory, P::owner_all | P::group_all)) return out;
    if (!prepare(out.pidDirectory, privileged ? P::owner_all | P::group_all : P::owner_all)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td><td><a href="snippets/windows-94ae28bc38fd.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, windows-text.hpp, config-types.hpp,
// &lt;shlobj.h&gt;, &lt;sddl.h&gt;, &lt;aclapi.h&gt;.
// Link Advapi32, Shell32, Ole32. Uses process identity, not thread impersonation.
// POLICY PROPOSAL: normal users get LocalAppData/snode.c/{config,log,run};
// LocalSystem gets ProgramData/snode.c/{config,log,run}. Elevated non-System
// users remain per-user. No synthetic UID zero or GID is introduced.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    auto report = [&amp;](DWORD error) {
        out.diagnostics.push_back("Windows configuration setup failed: " + std::to_string(error));
    };
    HANDLE rawToken = nullptr;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &amp;rawToken)) {
        report(::GetLastError()); return out;
    }
    struct CloseToken { void operator()(void* p) const { ::CloseHandle(p); } };
    std::unique_ptr&lt;void, CloseToken&gt; token(rawToken);
    auto tokenInfo = [&amp;](TOKEN_INFORMATION_CLASS kind) -&gt; Result&lt;std::vector&lt;std::byte&gt;&gt; {
        DWORD size = 0;
        ::GetTokenInformation(token.get(), kind, nullptr, 0, &amp;size);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) return winFailure&lt;std::vector&lt;std::byte&gt;&gt;();
        std::vector&lt;std::byte&gt; bytes(size);
        if (!::GetTokenInformation(token.get(), kind, bytes.data(), size, &amp;size)) return winFailure&lt;std::vector&lt;std::byte&gt;&gt;();
        return ok(std::move(bytes));
    };
    auto user = tokenInfo(TokenUser);
    auto group = tokenInfo(TokenPrimaryGroup);
    if (!user || !group) { report((!user ? user.error : group.error).code); return out; }
    PSID userSid = reinterpret_cast&lt;TOKEN_USER*&gt;(user.value.data())-&gt;User.Sid;
    PSID groupSid = reinterpret_cast&lt;TOKEN_PRIMARY_GROUP*&gt;(group.value.data())-&gt;PrimaryGroup;
    auto nameFor = [&amp;](PSID sid) -&gt; Result&lt;std::string&gt; {
        DWORD nameSize = 0, domainSize = 0; SID_NAME_USE type;
        ::LookupAccountSidW(nullptr, sid, nullptr, &amp;nameSize, nullptr, &amp;domainSize, &amp;type);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) return winFailure&lt;std::string&gt;();
        std::vector&lt;wchar_t&gt; name(nameSize), domain(std::max(DWORD{1}, domainSize));
        if (!::LookupAccountSidW(nullptr, sid, name.data(), &amp;nameSize, domain.data(), &amp;domainSize, &amp;type)) return winFailure&lt;std::string&gt;();
        const std::wstring qualified = domainSize ? std::wstring(domain.data()) + L"\\" + name.data() : name.data();
        return toUtf8(qualified);
    };
    auto userName = nameFor(userSid), groupName = nameFor(groupSid);
    if (!userName || !groupName) { report((!userName ? userName.error : groupName.error).code); return out; }
    out.userName = std::move(userName.value); out.groupName = std::move(groupName.value);
    const bool service = ::IsWellKnownSid(userSid, WinLocalSystemSid) != FALSE;
    PWSTR folder = nullptr;
    const HRESULT status = ::SHGetKnownFolderPath(service ? FOLDERID_ProgramData : FOLDERID_LocalAppData, 0, nullptr, &amp;folder);
    if (FAILED(status)) { report(static_cast&lt;DWORD&gt;(status)); return out; }
    struct FreeFolder { void operator()(wchar_t* p) const { ::CoTaskMemFree(p); } };
    std::unique_ptr&lt;wchar_t, FreeFolder&gt; folderOwner(folder);
    const Path root = Path(folder) / L"snode.c";
    out.configDirectory = root / L"config";
    out.logDirectory = root / L"log";
    out.pidDirectory = root / L"run";

    // Resolve a configured service group to an actual SID. This is not a GID cast.
    std::vector&lt;std::byte&gt; serviceSid;
    if (service) {
        auto name = toWide(serviceGroup);
        if (!name) { report(name.error.code); return out; }
        DWORD sidSize = 0, domainSize = 0; SID_NAME_USE type;
        ::LookupAccountNameW(nullptr, name.value.c_str(), nullptr, &amp;sidSize, nullptr, &amp;domainSize, &amp;type);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) { report(::GetLastError()); return out; }
        serviceSid.resize(sidSize);
        std::vector&lt;wchar_t&gt; domain(std::max(DWORD{1}, domainSize));
        if (!::LookupAccountNameW(nullptr, name.value.c_str(), serviceSid.data(), &amp;sidSize, domain.data(), &amp;domainSize, &amp;type)) {
            report(::GetLastError()); return out;
        }
        if (type != SidTypeGroup &amp;&amp; type != SidTypeAlias &amp;&amp; type != SidTypeWellKnownGroup) {
            report(ERROR_INVALID_PRIMARY_GROUP); return out;
        }
    }
    auto sidText = [&amp;](PSID sid) -&gt; std::wstring {
        LPWSTR value = nullptr;
        if (!::ConvertSidToStringSidW(sid, &amp;value)) return {};
        std::unique_ptr&lt;wchar_t, decltype(&amp;::LocalFree)&gt; guard(value, ::LocalFree);
        return value;
    };
    const auto owner = sidText(userSid);
    const auto shared = service ? sidText(serviceSid.data()) : std::wstring{};
    if (owner.empty() || (service &amp;&amp; shared.empty())) { report(::GetLastError()); return out; }
    auto create = [&amp;](const Path&amp; path, bool readOnlyGroup) -&gt; bool {
        // Protected DACL: owner and SYSTEM full access. Service group read/execute
        // on config, modify on log/run. OI/CI apply to new descendants.
        std::wstring sddl = L"D:P(A;OICI;FA;;;" + owner + L")(A;OICI;FA;;;SY)";
        if (service) sddl += L"(A;OICI;" + std::wstring(readOnlyGroup ? L"0x1200a9" : L"0x1301bf") + L";;;" + shared + L")";
        PSECURITY_DESCRIPTOR descriptor = nullptr;
        if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &amp;descriptor, nullptr)) {
            report(::GetLastError()); return false;
        }
        std::unique_ptr&lt;void, decltype(&amp;::LocalFree)&gt; descriptorOwner(descriptor, ::LocalFree);
        SECURITY_ATTRIBUTES attributes{sizeof(attributes), descriptor, FALSE};
        if (::CreateDirectoryW(path.c_str(), &amp;attributes)) return true;
        const DWORD error = ::GetLastError();
        if (error != ERROR_ALREADY_EXISTS) { report(error); return false; }
        const DWORD attrs = ::GetFileAttributesW(path.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) { report(::GetLastError()); return false; }
        // Existing directories retain permissions; reject reparse redirection.
        if (!(attrs &amp; FILE_ATTRIBUTE_DIRECTORY) || (attrs &amp; FILE_ATTRIBUTE_REPARSE_POINT)) {
            report(ERROR_ACCESS_DENIED); return false;
        }
        return true;
    };
    // Known-folder parent exists. Create root before the three children.
    if (!create(root, false) || !create(out.configDirectory, true) || !create(out.logDirectory, false) || !create(out.pidDirectory, false)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td><td><a href="snippets/mac-5790162f8c9e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, config-types.hpp, &lt;pwd.h&gt;, &lt;grp.h&gt;.
// Place this function in the existing utils::system owner at adoption.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    const passwd* pw = ::getpwuid(::getuid());
    if (!pw) return out;
    // Copy NSS-owned data before any subsequent lookup can invalidate it.
    out.userName = pw-&gt;pw_name;
    const std::string accountHome = pw-&gt;pw_dir;
    const gid_t primaryGroup = pw-&gt;pw_gid;
    const group* gr = ::getgrgid(primaryGroup);
    if (!gr) return out;
    out.groupName = gr-&gt;gr_name;
    const uid_t owner = ::geteuid();
    const bool privileged = owner == 0;
    if (privileged) {
        out.configDirectory = "/etc/snode.c";
        out.logDirectory = "/var/log/snode.c";
        out.pidDirectory = "/var/run/snode.c";
    } else {
        const char* environmentHome = ::getenv("HOME");
        const std::string home = environmentHome &amp;&amp; *environmentHome == '/' ? environmentHome : accountHome;
        auto directory = [&amp;](const char* variable, const char* fallback) -&gt; Path {
            const char* value = ::getenv(variable);
            if (value &amp;&amp; *value == '/') return Path(value) / "snode.c";
            if (!home.empty() &amp;&amp; home.front() == '/') return Path(home + fallback) / "snode.c";
            return {};
        };
        out.configDirectory = directory("XDG_CONFIG_HOME", "/.config");
        out.logDirectory = directory("XDG_STATE_HOME", "/.local/state");
        out.pidDirectory = directory("XDG_RUNTIME_DIR", "/.local/run");
        if (out.configDirectory.empty() || out.logDirectory.empty() || out.pidDirectory.empty()) return out;
    }
    // Preserve current create-only permissions, failure ordering and warning policy.
    using P = std::filesystem::perms;
    auto prepare = [&amp;](const Path&amp; path, P permissions) -&gt; bool {
        if (!std::filesystem::create_directories(path)) return true;
        std::filesystem::permissions(path, permissions);
        if (privileged) {
            const group* service = ::getgrnam(serviceGroup.c_str());
            if (!service) {
                out.diagnostics.push_back("Cannot find service group '" + serviceGroup + "'. Create it and add the current user.");
                std::filesystem::remove(path);
                return false;
            }
            if (::chown(path.c_str(), owner, service-&gt;gr_gid) &lt; 0) {
                const int error = errno;
                out.diagnostics.push_back("Cannot set group ownership of '" + path.string() + "': " + std::generic_category().message(error));
            }
        }
        return true;
    };
    if (!prepare(out.configDirectory, P::owner_all | P::group_read | P::group_exec)) return out;
    if (!prepare(out.logDirectory, P::owner_all | P::group_all)) return out;
    if (!prepare(out.pidDirectory, privileged ? P::owner_all | P::group_all : P::owner_all)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td></tr>
<tr id="getegid"><td><a href="CALL_SITES.md#getegid"><code>getegid</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Include effective identity context; do not fabricate GID values.</p></details></td><td><pre><code>ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup);</code></pre><p>Whole Config.cpp:953–1065 replacement; see config-caller.cpp.txt and config-types.hpp.txt. POSIX preserves root/XDG/create-only ownership policy with copied NSS strings. Windows LocalSystem/known-folder/ACL policy is a proposed behavior, not a proven POSIX equivalence. Other uses of these calls remain in the source ledger and need the daemon/test-specific capsule as applicable.</p></td><td><a href="snippets/linux-5790162f8c9e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, config-types.hpp, &lt;pwd.h&gt;, &lt;grp.h&gt;.
// Place this function in the existing utils::system owner at adoption.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    const passwd* pw = ::getpwuid(::getuid());
    if (!pw) return out;
    // Copy NSS-owned data before any subsequent lookup can invalidate it.
    out.userName = pw-&gt;pw_name;
    const std::string accountHome = pw-&gt;pw_dir;
    const gid_t primaryGroup = pw-&gt;pw_gid;
    const group* gr = ::getgrgid(primaryGroup);
    if (!gr) return out;
    out.groupName = gr-&gt;gr_name;
    const uid_t owner = ::geteuid();
    const bool privileged = owner == 0;
    if (privileged) {
        out.configDirectory = "/etc/snode.c";
        out.logDirectory = "/var/log/snode.c";
        out.pidDirectory = "/var/run/snode.c";
    } else {
        const char* environmentHome = ::getenv("HOME");
        const std::string home = environmentHome &amp;&amp; *environmentHome == '/' ? environmentHome : accountHome;
        auto directory = [&amp;](const char* variable, const char* fallback) -&gt; Path {
            const char* value = ::getenv(variable);
            if (value &amp;&amp; *value == '/') return Path(value) / "snode.c";
            if (!home.empty() &amp;&amp; home.front() == '/') return Path(home + fallback) / "snode.c";
            return {};
        };
        out.configDirectory = directory("XDG_CONFIG_HOME", "/.config");
        out.logDirectory = directory("XDG_STATE_HOME", "/.local/state");
        out.pidDirectory = directory("XDG_RUNTIME_DIR", "/.local/run");
        if (out.configDirectory.empty() || out.logDirectory.empty() || out.pidDirectory.empty()) return out;
    }
    // Preserve current create-only permissions, failure ordering and warning policy.
    using P = std::filesystem::perms;
    auto prepare = [&amp;](const Path&amp; path, P permissions) -&gt; bool {
        if (!std::filesystem::create_directories(path)) return true;
        std::filesystem::permissions(path, permissions);
        if (privileged) {
            const group* service = ::getgrnam(serviceGroup.c_str());
            if (!service) {
                out.diagnostics.push_back("Cannot find service group '" + serviceGroup + "'. Create it and add the current user.");
                std::filesystem::remove(path);
                return false;
            }
            if (::chown(path.c_str(), owner, service-&gt;gr_gid) &lt; 0) {
                const int error = errno;
                out.diagnostics.push_back("Cannot set group ownership of '" + path.string() + "': " + std::generic_category().message(error));
            }
        }
        return true;
    };
    if (!prepare(out.configDirectory, P::owner_all | P::group_read | P::group_exec)) return out;
    if (!prepare(out.logDirectory, P::owner_all | P::group_all)) return out;
    if (!prepare(out.pidDirectory, privileged ? P::owner_all | P::group_all : P::owner_all)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td><td><a href="snippets/windows-94ae28bc38fd.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, windows-text.hpp, config-types.hpp,
// &lt;shlobj.h&gt;, &lt;sddl.h&gt;, &lt;aclapi.h&gt;.
// Link Advapi32, Shell32, Ole32. Uses process identity, not thread impersonation.
// POLICY PROPOSAL: normal users get LocalAppData/snode.c/{config,log,run};
// LocalSystem gets ProgramData/snode.c/{config,log,run}. Elevated non-System
// users remain per-user. No synthetic UID zero or GID is introduced.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    auto report = [&amp;](DWORD error) {
        out.diagnostics.push_back("Windows configuration setup failed: " + std::to_string(error));
    };
    HANDLE rawToken = nullptr;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &amp;rawToken)) {
        report(::GetLastError()); return out;
    }
    struct CloseToken { void operator()(void* p) const { ::CloseHandle(p); } };
    std::unique_ptr&lt;void, CloseToken&gt; token(rawToken);
    auto tokenInfo = [&amp;](TOKEN_INFORMATION_CLASS kind) -&gt; Result&lt;std::vector&lt;std::byte&gt;&gt; {
        DWORD size = 0;
        ::GetTokenInformation(token.get(), kind, nullptr, 0, &amp;size);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) return winFailure&lt;std::vector&lt;std::byte&gt;&gt;();
        std::vector&lt;std::byte&gt; bytes(size);
        if (!::GetTokenInformation(token.get(), kind, bytes.data(), size, &amp;size)) return winFailure&lt;std::vector&lt;std::byte&gt;&gt;();
        return ok(std::move(bytes));
    };
    auto user = tokenInfo(TokenUser);
    auto group = tokenInfo(TokenPrimaryGroup);
    if (!user || !group) { report((!user ? user.error : group.error).code); return out; }
    PSID userSid = reinterpret_cast&lt;TOKEN_USER*&gt;(user.value.data())-&gt;User.Sid;
    PSID groupSid = reinterpret_cast&lt;TOKEN_PRIMARY_GROUP*&gt;(group.value.data())-&gt;PrimaryGroup;
    auto nameFor = [&amp;](PSID sid) -&gt; Result&lt;std::string&gt; {
        DWORD nameSize = 0, domainSize = 0; SID_NAME_USE type;
        ::LookupAccountSidW(nullptr, sid, nullptr, &amp;nameSize, nullptr, &amp;domainSize, &amp;type);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) return winFailure&lt;std::string&gt;();
        std::vector&lt;wchar_t&gt; name(nameSize), domain(std::max(DWORD{1}, domainSize));
        if (!::LookupAccountSidW(nullptr, sid, name.data(), &amp;nameSize, domain.data(), &amp;domainSize, &amp;type)) return winFailure&lt;std::string&gt;();
        const std::wstring qualified = domainSize ? std::wstring(domain.data()) + L"\\" + name.data() : name.data();
        return toUtf8(qualified);
    };
    auto userName = nameFor(userSid), groupName = nameFor(groupSid);
    if (!userName || !groupName) { report((!userName ? userName.error : groupName.error).code); return out; }
    out.userName = std::move(userName.value); out.groupName = std::move(groupName.value);
    const bool service = ::IsWellKnownSid(userSid, WinLocalSystemSid) != FALSE;
    PWSTR folder = nullptr;
    const HRESULT status = ::SHGetKnownFolderPath(service ? FOLDERID_ProgramData : FOLDERID_LocalAppData, 0, nullptr, &amp;folder);
    if (FAILED(status)) { report(static_cast&lt;DWORD&gt;(status)); return out; }
    struct FreeFolder { void operator()(wchar_t* p) const { ::CoTaskMemFree(p); } };
    std::unique_ptr&lt;wchar_t, FreeFolder&gt; folderOwner(folder);
    const Path root = Path(folder) / L"snode.c";
    out.configDirectory = root / L"config";
    out.logDirectory = root / L"log";
    out.pidDirectory = root / L"run";

    // Resolve a configured service group to an actual SID. This is not a GID cast.
    std::vector&lt;std::byte&gt; serviceSid;
    if (service) {
        auto name = toWide(serviceGroup);
        if (!name) { report(name.error.code); return out; }
        DWORD sidSize = 0, domainSize = 0; SID_NAME_USE type;
        ::LookupAccountNameW(nullptr, name.value.c_str(), nullptr, &amp;sidSize, nullptr, &amp;domainSize, &amp;type);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) { report(::GetLastError()); return out; }
        serviceSid.resize(sidSize);
        std::vector&lt;wchar_t&gt; domain(std::max(DWORD{1}, domainSize));
        if (!::LookupAccountNameW(nullptr, name.value.c_str(), serviceSid.data(), &amp;sidSize, domain.data(), &amp;domainSize, &amp;type)) {
            report(::GetLastError()); return out;
        }
        if (type != SidTypeGroup &amp;&amp; type != SidTypeAlias &amp;&amp; type != SidTypeWellKnownGroup) {
            report(ERROR_INVALID_PRIMARY_GROUP); return out;
        }
    }
    auto sidText = [&amp;](PSID sid) -&gt; std::wstring {
        LPWSTR value = nullptr;
        if (!::ConvertSidToStringSidW(sid, &amp;value)) return {};
        std::unique_ptr&lt;wchar_t, decltype(&amp;::LocalFree)&gt; guard(value, ::LocalFree);
        return value;
    };
    const auto owner = sidText(userSid);
    const auto shared = service ? sidText(serviceSid.data()) : std::wstring{};
    if (owner.empty() || (service &amp;&amp; shared.empty())) { report(::GetLastError()); return out; }
    auto create = [&amp;](const Path&amp; path, bool readOnlyGroup) -&gt; bool {
        // Protected DACL: owner and SYSTEM full access. Service group read/execute
        // on config, modify on log/run. OI/CI apply to new descendants.
        std::wstring sddl = L"D:P(A;OICI;FA;;;" + owner + L")(A;OICI;FA;;;SY)";
        if (service) sddl += L"(A;OICI;" + std::wstring(readOnlyGroup ? L"0x1200a9" : L"0x1301bf") + L";;;" + shared + L")";
        PSECURITY_DESCRIPTOR descriptor = nullptr;
        if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &amp;descriptor, nullptr)) {
            report(::GetLastError()); return false;
        }
        std::unique_ptr&lt;void, decltype(&amp;::LocalFree)&gt; descriptorOwner(descriptor, ::LocalFree);
        SECURITY_ATTRIBUTES attributes{sizeof(attributes), descriptor, FALSE};
        if (::CreateDirectoryW(path.c_str(), &amp;attributes)) return true;
        const DWORD error = ::GetLastError();
        if (error != ERROR_ALREADY_EXISTS) { report(error); return false; }
        const DWORD attrs = ::GetFileAttributesW(path.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) { report(::GetLastError()); return false; }
        // Existing directories retain permissions; reject reparse redirection.
        if (!(attrs &amp; FILE_ATTRIBUTE_DIRECTORY) || (attrs &amp; FILE_ATTRIBUTE_REPARSE_POINT)) {
            report(ERROR_ACCESS_DENIED); return false;
        }
        return true;
    };
    // Known-folder parent exists. Create root before the three children.
    if (!create(root, false) || !create(out.configDirectory, true) || !create(out.logDirectory, false) || !create(out.pidDirectory, false)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td><td><a href="snippets/mac-5790162f8c9e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, config-types.hpp, &lt;pwd.h&gt;, &lt;grp.h&gt;.
// Place this function in the existing utils::system owner at adoption.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    const passwd* pw = ::getpwuid(::getuid());
    if (!pw) return out;
    // Copy NSS-owned data before any subsequent lookup can invalidate it.
    out.userName = pw-&gt;pw_name;
    const std::string accountHome = pw-&gt;pw_dir;
    const gid_t primaryGroup = pw-&gt;pw_gid;
    const group* gr = ::getgrgid(primaryGroup);
    if (!gr) return out;
    out.groupName = gr-&gt;gr_name;
    const uid_t owner = ::geteuid();
    const bool privileged = owner == 0;
    if (privileged) {
        out.configDirectory = "/etc/snode.c";
        out.logDirectory = "/var/log/snode.c";
        out.pidDirectory = "/var/run/snode.c";
    } else {
        const char* environmentHome = ::getenv("HOME");
        const std::string home = environmentHome &amp;&amp; *environmentHome == '/' ? environmentHome : accountHome;
        auto directory = [&amp;](const char* variable, const char* fallback) -&gt; Path {
            const char* value = ::getenv(variable);
            if (value &amp;&amp; *value == '/') return Path(value) / "snode.c";
            if (!home.empty() &amp;&amp; home.front() == '/') return Path(home + fallback) / "snode.c";
            return {};
        };
        out.configDirectory = directory("XDG_CONFIG_HOME", "/.config");
        out.logDirectory = directory("XDG_STATE_HOME", "/.local/state");
        out.pidDirectory = directory("XDG_RUNTIME_DIR", "/.local/run");
        if (out.configDirectory.empty() || out.logDirectory.empty() || out.pidDirectory.empty()) return out;
    }
    // Preserve current create-only permissions, failure ordering and warning policy.
    using P = std::filesystem::perms;
    auto prepare = [&amp;](const Path&amp; path, P permissions) -&gt; bool {
        if (!std::filesystem::create_directories(path)) return true;
        std::filesystem::permissions(path, permissions);
        if (privileged) {
            const group* service = ::getgrnam(serviceGroup.c_str());
            if (!service) {
                out.diagnostics.push_back("Cannot find service group '" + serviceGroup + "'. Create it and add the current user.");
                std::filesystem::remove(path);
                return false;
            }
            if (::chown(path.c_str(), owner, service-&gt;gr_gid) &lt; 0) {
                const int error = errno;
                out.diagnostics.push_back("Cannot set group ownership of '" + path.string() + "': " + std::generic_category().message(error));
            }
        }
        return true;
    };
    if (!prepare(out.configDirectory, P::owner_all | P::group_read | P::group_exec)) return out;
    if (!prepare(out.logDirectory, P::owner_all | P::group_all)) return out;
    if (!prepare(out.pidDirectory, privileged ? P::owner_all | P::group_all : P::owner_all)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td></tr>
<tr id="getpwnam"><td><a href="CALL_SITES.md#getpwnam"><code>getpwnam</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Owned fields; distinguish not-found/error; home-directory discovery may require profile APIs.</p></details></td><td><pre><code>Status doEnterDaemonIdentity(const std::string&amp; user, const std::string&amp; group); Status doRestoreRealIdentity();</code></pre><p>Entire Daemon.cpp identity sequence, including restore order. Startup-only; preserves effective-only changes and supplementary groups as today. Failure can occur after group changed; fail the daemon startup, never proceed as if fully switched. Windows thread impersonation does not implement this process-wide contract. A service account/launch token requires an explicitly different launch API.</p></td><td><a href="snippets/linux-c5f78e9fffeb.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doEnterDaemonIdentity(const std::string&amp; user, const std::string&amp; group) {
    errno = 0;
    const struct group* gr = ::getgrnam(group.c_str());
    if (!gr) return posixFailure(errno ? errno : ENOENT);
    if (::setegid(gr-&gt;gr_gid) != 0) return posixFailure();
    errno = 0;
    const passwd* pw = ::getpwnam(user.c_str());
    if (!pw) return posixFailure(errno ? errno : ENOENT);
    if (::seteuid(pw-&gt;pw_uid) != 0) return posixFailure();
    return ok();
}
Status doRestoreRealIdentity() {
    if (::seteuid(::getuid()) != 0) return posixFailure();
    return posixStatus(::setegid(::getgid()));
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Entire Daemon.cpp identity sequence, including restore order. Startup-only; preserves effective-only changes and supplementary groups as today. Failure can occur after group changed; fail the daemon startup, never proceed as if fully switched. Windows thread impersonation does not implement this process-wide contract. A service account/launch token requires an explicitly different launch API.</p><p>Native building blocks: LookupAccountNameW and NetUserGetInfo</p></td><td><a href="snippets/mac-c5f78e9fffeb.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doEnterDaemonIdentity(const std::string&amp; user, const std::string&amp; group) {
    errno = 0;
    const struct group* gr = ::getgrnam(group.c_str());
    if (!gr) return posixFailure(errno ? errno : ENOENT);
    if (::setegid(gr-&gt;gr_gid) != 0) return posixFailure();
    errno = 0;
    const passwd* pw = ::getpwnam(user.c_str());
    if (!pw) return posixFailure(errno ? errno : ENOENT);
    if (::seteuid(pw-&gt;pw_uid) != 0) return posixFailure();
    return ok();
}
Status doRestoreRealIdentity() {
    if (::seteuid(::getuid()) != 0) return posixFailure();
    return posixStatus(::setegid(::getgid()));
}</code></pre></details></td></tr>
<tr id="getpwuid"><td><a href="CALL_SITES.md#getpwuid"><code>getpwuid</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Owned name, home and primary-group fields; no POSIX struct passwd outside capsule.</p></details></td><td><pre><code>ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup);</code></pre><p>Whole Config.cpp:953–1065 replacement; see config-caller.cpp.txt and config-types.hpp.txt. POSIX preserves root/XDG/create-only ownership policy with copied NSS strings. Windows LocalSystem/known-folder/ACL policy is a proposed behavior, not a proven POSIX equivalence. Other uses of these calls remain in the source ledger and need the daemon/test-specific capsule as applicable.</p></td><td><a href="snippets/linux-5790162f8c9e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, config-types.hpp, &lt;pwd.h&gt;, &lt;grp.h&gt;.
// Place this function in the existing utils::system owner at adoption.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    const passwd* pw = ::getpwuid(::getuid());
    if (!pw) return out;
    // Copy NSS-owned data before any subsequent lookup can invalidate it.
    out.userName = pw-&gt;pw_name;
    const std::string accountHome = pw-&gt;pw_dir;
    const gid_t primaryGroup = pw-&gt;pw_gid;
    const group* gr = ::getgrgid(primaryGroup);
    if (!gr) return out;
    out.groupName = gr-&gt;gr_name;
    const uid_t owner = ::geteuid();
    const bool privileged = owner == 0;
    if (privileged) {
        out.configDirectory = "/etc/snode.c";
        out.logDirectory = "/var/log/snode.c";
        out.pidDirectory = "/var/run/snode.c";
    } else {
        const char* environmentHome = ::getenv("HOME");
        const std::string home = environmentHome &amp;&amp; *environmentHome == '/' ? environmentHome : accountHome;
        auto directory = [&amp;](const char* variable, const char* fallback) -&gt; Path {
            const char* value = ::getenv(variable);
            if (value &amp;&amp; *value == '/') return Path(value) / "snode.c";
            if (!home.empty() &amp;&amp; home.front() == '/') return Path(home + fallback) / "snode.c";
            return {};
        };
        out.configDirectory = directory("XDG_CONFIG_HOME", "/.config");
        out.logDirectory = directory("XDG_STATE_HOME", "/.local/state");
        out.pidDirectory = directory("XDG_RUNTIME_DIR", "/.local/run");
        if (out.configDirectory.empty() || out.logDirectory.empty() || out.pidDirectory.empty()) return out;
    }
    // Preserve current create-only permissions, failure ordering and warning policy.
    using P = std::filesystem::perms;
    auto prepare = [&amp;](const Path&amp; path, P permissions) -&gt; bool {
        if (!std::filesystem::create_directories(path)) return true;
        std::filesystem::permissions(path, permissions);
        if (privileged) {
            const group* service = ::getgrnam(serviceGroup.c_str());
            if (!service) {
                out.diagnostics.push_back("Cannot find service group '" + serviceGroup + "'. Create it and add the current user.");
                std::filesystem::remove(path);
                return false;
            }
            if (::chown(path.c_str(), owner, service-&gt;gr_gid) &lt; 0) {
                const int error = errno;
                out.diagnostics.push_back("Cannot set group ownership of '" + path.string() + "': " + std::generic_category().message(error));
            }
        }
        return true;
    };
    if (!prepare(out.configDirectory, P::owner_all | P::group_read | P::group_exec)) return out;
    if (!prepare(out.logDirectory, P::owner_all | P::group_all)) return out;
    if (!prepare(out.pidDirectory, privileged ? P::owner_all | P::group_all : P::owner_all)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td><td><a href="snippets/windows-94ae28bc38fd.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, windows-text.hpp, config-types.hpp,
// &lt;shlobj.h&gt;, &lt;sddl.h&gt;, &lt;aclapi.h&gt;.
// Link Advapi32, Shell32, Ole32. Uses process identity, not thread impersonation.
// POLICY PROPOSAL: normal users get LocalAppData/snode.c/{config,log,run};
// LocalSystem gets ProgramData/snode.c/{config,log,run}. Elevated non-System
// users remain per-user. No synthetic UID zero or GID is introduced.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    auto report = [&amp;](DWORD error) {
        out.diagnostics.push_back("Windows configuration setup failed: " + std::to_string(error));
    };
    HANDLE rawToken = nullptr;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &amp;rawToken)) {
        report(::GetLastError()); return out;
    }
    struct CloseToken { void operator()(void* p) const { ::CloseHandle(p); } };
    std::unique_ptr&lt;void, CloseToken&gt; token(rawToken);
    auto tokenInfo = [&amp;](TOKEN_INFORMATION_CLASS kind) -&gt; Result&lt;std::vector&lt;std::byte&gt;&gt; {
        DWORD size = 0;
        ::GetTokenInformation(token.get(), kind, nullptr, 0, &amp;size);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) return winFailure&lt;std::vector&lt;std::byte&gt;&gt;();
        std::vector&lt;std::byte&gt; bytes(size);
        if (!::GetTokenInformation(token.get(), kind, bytes.data(), size, &amp;size)) return winFailure&lt;std::vector&lt;std::byte&gt;&gt;();
        return ok(std::move(bytes));
    };
    auto user = tokenInfo(TokenUser);
    auto group = tokenInfo(TokenPrimaryGroup);
    if (!user || !group) { report((!user ? user.error : group.error).code); return out; }
    PSID userSid = reinterpret_cast&lt;TOKEN_USER*&gt;(user.value.data())-&gt;User.Sid;
    PSID groupSid = reinterpret_cast&lt;TOKEN_PRIMARY_GROUP*&gt;(group.value.data())-&gt;PrimaryGroup;
    auto nameFor = [&amp;](PSID sid) -&gt; Result&lt;std::string&gt; {
        DWORD nameSize = 0, domainSize = 0; SID_NAME_USE type;
        ::LookupAccountSidW(nullptr, sid, nullptr, &amp;nameSize, nullptr, &amp;domainSize, &amp;type);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) return winFailure&lt;std::string&gt;();
        std::vector&lt;wchar_t&gt; name(nameSize), domain(std::max(DWORD{1}, domainSize));
        if (!::LookupAccountSidW(nullptr, sid, name.data(), &amp;nameSize, domain.data(), &amp;domainSize, &amp;type)) return winFailure&lt;std::string&gt;();
        const std::wstring qualified = domainSize ? std::wstring(domain.data()) + L"\\" + name.data() : name.data();
        return toUtf8(qualified);
    };
    auto userName = nameFor(userSid), groupName = nameFor(groupSid);
    if (!userName || !groupName) { report((!userName ? userName.error : groupName.error).code); return out; }
    out.userName = std::move(userName.value); out.groupName = std::move(groupName.value);
    const bool service = ::IsWellKnownSid(userSid, WinLocalSystemSid) != FALSE;
    PWSTR folder = nullptr;
    const HRESULT status = ::SHGetKnownFolderPath(service ? FOLDERID_ProgramData : FOLDERID_LocalAppData, 0, nullptr, &amp;folder);
    if (FAILED(status)) { report(static_cast&lt;DWORD&gt;(status)); return out; }
    struct FreeFolder { void operator()(wchar_t* p) const { ::CoTaskMemFree(p); } };
    std::unique_ptr&lt;wchar_t, FreeFolder&gt; folderOwner(folder);
    const Path root = Path(folder) / L"snode.c";
    out.configDirectory = root / L"config";
    out.logDirectory = root / L"log";
    out.pidDirectory = root / L"run";

    // Resolve a configured service group to an actual SID. This is not a GID cast.
    std::vector&lt;std::byte&gt; serviceSid;
    if (service) {
        auto name = toWide(serviceGroup);
        if (!name) { report(name.error.code); return out; }
        DWORD sidSize = 0, domainSize = 0; SID_NAME_USE type;
        ::LookupAccountNameW(nullptr, name.value.c_str(), nullptr, &amp;sidSize, nullptr, &amp;domainSize, &amp;type);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) { report(::GetLastError()); return out; }
        serviceSid.resize(sidSize);
        std::vector&lt;wchar_t&gt; domain(std::max(DWORD{1}, domainSize));
        if (!::LookupAccountNameW(nullptr, name.value.c_str(), serviceSid.data(), &amp;sidSize, domain.data(), &amp;domainSize, &amp;type)) {
            report(::GetLastError()); return out;
        }
        if (type != SidTypeGroup &amp;&amp; type != SidTypeAlias &amp;&amp; type != SidTypeWellKnownGroup) {
            report(ERROR_INVALID_PRIMARY_GROUP); return out;
        }
    }
    auto sidText = [&amp;](PSID sid) -&gt; std::wstring {
        LPWSTR value = nullptr;
        if (!::ConvertSidToStringSidW(sid, &amp;value)) return {};
        std::unique_ptr&lt;wchar_t, decltype(&amp;::LocalFree)&gt; guard(value, ::LocalFree);
        return value;
    };
    const auto owner = sidText(userSid);
    const auto shared = service ? sidText(serviceSid.data()) : std::wstring{};
    if (owner.empty() || (service &amp;&amp; shared.empty())) { report(::GetLastError()); return out; }
    auto create = [&amp;](const Path&amp; path, bool readOnlyGroup) -&gt; bool {
        // Protected DACL: owner and SYSTEM full access. Service group read/execute
        // on config, modify on log/run. OI/CI apply to new descendants.
        std::wstring sddl = L"D:P(A;OICI;FA;;;" + owner + L")(A;OICI;FA;;;SY)";
        if (service) sddl += L"(A;OICI;" + std::wstring(readOnlyGroup ? L"0x1200a9" : L"0x1301bf") + L";;;" + shared + L")";
        PSECURITY_DESCRIPTOR descriptor = nullptr;
        if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &amp;descriptor, nullptr)) {
            report(::GetLastError()); return false;
        }
        std::unique_ptr&lt;void, decltype(&amp;::LocalFree)&gt; descriptorOwner(descriptor, ::LocalFree);
        SECURITY_ATTRIBUTES attributes{sizeof(attributes), descriptor, FALSE};
        if (::CreateDirectoryW(path.c_str(), &amp;attributes)) return true;
        const DWORD error = ::GetLastError();
        if (error != ERROR_ALREADY_EXISTS) { report(error); return false; }
        const DWORD attrs = ::GetFileAttributesW(path.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) { report(::GetLastError()); return false; }
        // Existing directories retain permissions; reject reparse redirection.
        if (!(attrs &amp; FILE_ATTRIBUTE_DIRECTORY) || (attrs &amp; FILE_ATTRIBUTE_REPARSE_POINT)) {
            report(ERROR_ACCESS_DENIED); return false;
        }
        return true;
    };
    // Known-folder parent exists. Create root before the three children.
    if (!create(root, false) || !create(out.configDirectory, true) || !create(out.logDirectory, false) || !create(out.pidDirectory, false)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td><td><a href="snippets/mac-5790162f8c9e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, config-types.hpp, &lt;pwd.h&gt;, &lt;grp.h&gt;.
// Place this function in the existing utils::system owner at adoption.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    const passwd* pw = ::getpwuid(::getuid());
    if (!pw) return out;
    // Copy NSS-owned data before any subsequent lookup can invalidate it.
    out.userName = pw-&gt;pw_name;
    const std::string accountHome = pw-&gt;pw_dir;
    const gid_t primaryGroup = pw-&gt;pw_gid;
    const group* gr = ::getgrgid(primaryGroup);
    if (!gr) return out;
    out.groupName = gr-&gt;gr_name;
    const uid_t owner = ::geteuid();
    const bool privileged = owner == 0;
    if (privileged) {
        out.configDirectory = "/etc/snode.c";
        out.logDirectory = "/var/log/snode.c";
        out.pidDirectory = "/var/run/snode.c";
    } else {
        const char* environmentHome = ::getenv("HOME");
        const std::string home = environmentHome &amp;&amp; *environmentHome == '/' ? environmentHome : accountHome;
        auto directory = [&amp;](const char* variable, const char* fallback) -&gt; Path {
            const char* value = ::getenv(variable);
            if (value &amp;&amp; *value == '/') return Path(value) / "snode.c";
            if (!home.empty() &amp;&amp; home.front() == '/') return Path(home + fallback) / "snode.c";
            return {};
        };
        out.configDirectory = directory("XDG_CONFIG_HOME", "/.config");
        out.logDirectory = directory("XDG_STATE_HOME", "/.local/state");
        out.pidDirectory = directory("XDG_RUNTIME_DIR", "/.local/run");
        if (out.configDirectory.empty() || out.logDirectory.empty() || out.pidDirectory.empty()) return out;
    }
    // Preserve current create-only permissions, failure ordering and warning policy.
    using P = std::filesystem::perms;
    auto prepare = [&amp;](const Path&amp; path, P permissions) -&gt; bool {
        if (!std::filesystem::create_directories(path)) return true;
        std::filesystem::permissions(path, permissions);
        if (privileged) {
            const group* service = ::getgrnam(serviceGroup.c_str());
            if (!service) {
                out.diagnostics.push_back("Cannot find service group '" + serviceGroup + "'. Create it and add the current user.");
                std::filesystem::remove(path);
                return false;
            }
            if (::chown(path.c_str(), owner, service-&gt;gr_gid) &lt; 0) {
                const int error = errno;
                out.diagnostics.push_back("Cannot set group ownership of '" + path.string() + "': " + std::generic_category().message(error));
            }
        }
        return true;
    };
    if (!prepare(out.configDirectory, P::owner_all | P::group_read | P::group_exec)) return out;
    if (!prepare(out.logDirectory, P::owner_all | P::group_all)) return out;
    if (!prepare(out.pidDirectory, privileged ? P::owner_all | P::group_all : P::owner_all)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td></tr>
<tr id="getgrnam"><td><a href="CALL_SITES.md#getgrnam"><code>getgrnam</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, test</p><p>Explicit local/domain scope and owned result.</p></details></td><td><pre><code>ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup);</code></pre><p>Whole Config.cpp:953–1065 replacement; see config-caller.cpp.txt and config-types.hpp.txt. POSIX preserves root/XDG/create-only ownership policy with copied NSS strings. Windows LocalSystem/known-folder/ACL policy is a proposed behavior, not a proven POSIX equivalence. Other uses of these calls remain in the source ledger and need the daemon/test-specific capsule as applicable.</p></td><td><a href="snippets/linux-5790162f8c9e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, config-types.hpp, &lt;pwd.h&gt;, &lt;grp.h&gt;.
// Place this function in the existing utils::system owner at adoption.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    const passwd* pw = ::getpwuid(::getuid());
    if (!pw) return out;
    // Copy NSS-owned data before any subsequent lookup can invalidate it.
    out.userName = pw-&gt;pw_name;
    const std::string accountHome = pw-&gt;pw_dir;
    const gid_t primaryGroup = pw-&gt;pw_gid;
    const group* gr = ::getgrgid(primaryGroup);
    if (!gr) return out;
    out.groupName = gr-&gt;gr_name;
    const uid_t owner = ::geteuid();
    const bool privileged = owner == 0;
    if (privileged) {
        out.configDirectory = "/etc/snode.c";
        out.logDirectory = "/var/log/snode.c";
        out.pidDirectory = "/var/run/snode.c";
    } else {
        const char* environmentHome = ::getenv("HOME");
        const std::string home = environmentHome &amp;&amp; *environmentHome == '/' ? environmentHome : accountHome;
        auto directory = [&amp;](const char* variable, const char* fallback) -&gt; Path {
            const char* value = ::getenv(variable);
            if (value &amp;&amp; *value == '/') return Path(value) / "snode.c";
            if (!home.empty() &amp;&amp; home.front() == '/') return Path(home + fallback) / "snode.c";
            return {};
        };
        out.configDirectory = directory("XDG_CONFIG_HOME", "/.config");
        out.logDirectory = directory("XDG_STATE_HOME", "/.local/state");
        out.pidDirectory = directory("XDG_RUNTIME_DIR", "/.local/run");
        if (out.configDirectory.empty() || out.logDirectory.empty() || out.pidDirectory.empty()) return out;
    }
    // Preserve current create-only permissions, failure ordering and warning policy.
    using P = std::filesystem::perms;
    auto prepare = [&amp;](const Path&amp; path, P permissions) -&gt; bool {
        if (!std::filesystem::create_directories(path)) return true;
        std::filesystem::permissions(path, permissions);
        if (privileged) {
            const group* service = ::getgrnam(serviceGroup.c_str());
            if (!service) {
                out.diagnostics.push_back("Cannot find service group '" + serviceGroup + "'. Create it and add the current user.");
                std::filesystem::remove(path);
                return false;
            }
            if (::chown(path.c_str(), owner, service-&gt;gr_gid) &lt; 0) {
                const int error = errno;
                out.diagnostics.push_back("Cannot set group ownership of '" + path.string() + "': " + std::generic_category().message(error));
            }
        }
        return true;
    };
    if (!prepare(out.configDirectory, P::owner_all | P::group_read | P::group_exec)) return out;
    if (!prepare(out.logDirectory, P::owner_all | P::group_all)) return out;
    if (!prepare(out.pidDirectory, privileged ? P::owner_all | P::group_all : P::owner_all)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td><td><a href="snippets/windows-94ae28bc38fd.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, windows-text.hpp, config-types.hpp,
// &lt;shlobj.h&gt;, &lt;sddl.h&gt;, &lt;aclapi.h&gt;.
// Link Advapi32, Shell32, Ole32. Uses process identity, not thread impersonation.
// POLICY PROPOSAL: normal users get LocalAppData/snode.c/{config,log,run};
// LocalSystem gets ProgramData/snode.c/{config,log,run}. Elevated non-System
// users remain per-user. No synthetic UID zero or GID is introduced.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    auto report = [&amp;](DWORD error) {
        out.diagnostics.push_back("Windows configuration setup failed: " + std::to_string(error));
    };
    HANDLE rawToken = nullptr;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &amp;rawToken)) {
        report(::GetLastError()); return out;
    }
    struct CloseToken { void operator()(void* p) const { ::CloseHandle(p); } };
    std::unique_ptr&lt;void, CloseToken&gt; token(rawToken);
    auto tokenInfo = [&amp;](TOKEN_INFORMATION_CLASS kind) -&gt; Result&lt;std::vector&lt;std::byte&gt;&gt; {
        DWORD size = 0;
        ::GetTokenInformation(token.get(), kind, nullptr, 0, &amp;size);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) return winFailure&lt;std::vector&lt;std::byte&gt;&gt;();
        std::vector&lt;std::byte&gt; bytes(size);
        if (!::GetTokenInformation(token.get(), kind, bytes.data(), size, &amp;size)) return winFailure&lt;std::vector&lt;std::byte&gt;&gt;();
        return ok(std::move(bytes));
    };
    auto user = tokenInfo(TokenUser);
    auto group = tokenInfo(TokenPrimaryGroup);
    if (!user || !group) { report((!user ? user.error : group.error).code); return out; }
    PSID userSid = reinterpret_cast&lt;TOKEN_USER*&gt;(user.value.data())-&gt;User.Sid;
    PSID groupSid = reinterpret_cast&lt;TOKEN_PRIMARY_GROUP*&gt;(group.value.data())-&gt;PrimaryGroup;
    auto nameFor = [&amp;](PSID sid) -&gt; Result&lt;std::string&gt; {
        DWORD nameSize = 0, domainSize = 0; SID_NAME_USE type;
        ::LookupAccountSidW(nullptr, sid, nullptr, &amp;nameSize, nullptr, &amp;domainSize, &amp;type);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) return winFailure&lt;std::string&gt;();
        std::vector&lt;wchar_t&gt; name(nameSize), domain(std::max(DWORD{1}, domainSize));
        if (!::LookupAccountSidW(nullptr, sid, name.data(), &amp;nameSize, domain.data(), &amp;domainSize, &amp;type)) return winFailure&lt;std::string&gt;();
        const std::wstring qualified = domainSize ? std::wstring(domain.data()) + L"\\" + name.data() : name.data();
        return toUtf8(qualified);
    };
    auto userName = nameFor(userSid), groupName = nameFor(groupSid);
    if (!userName || !groupName) { report((!userName ? userName.error : groupName.error).code); return out; }
    out.userName = std::move(userName.value); out.groupName = std::move(groupName.value);
    const bool service = ::IsWellKnownSid(userSid, WinLocalSystemSid) != FALSE;
    PWSTR folder = nullptr;
    const HRESULT status = ::SHGetKnownFolderPath(service ? FOLDERID_ProgramData : FOLDERID_LocalAppData, 0, nullptr, &amp;folder);
    if (FAILED(status)) { report(static_cast&lt;DWORD&gt;(status)); return out; }
    struct FreeFolder { void operator()(wchar_t* p) const { ::CoTaskMemFree(p); } };
    std::unique_ptr&lt;wchar_t, FreeFolder&gt; folderOwner(folder);
    const Path root = Path(folder) / L"snode.c";
    out.configDirectory = root / L"config";
    out.logDirectory = root / L"log";
    out.pidDirectory = root / L"run";

    // Resolve a configured service group to an actual SID. This is not a GID cast.
    std::vector&lt;std::byte&gt; serviceSid;
    if (service) {
        auto name = toWide(serviceGroup);
        if (!name) { report(name.error.code); return out; }
        DWORD sidSize = 0, domainSize = 0; SID_NAME_USE type;
        ::LookupAccountNameW(nullptr, name.value.c_str(), nullptr, &amp;sidSize, nullptr, &amp;domainSize, &amp;type);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) { report(::GetLastError()); return out; }
        serviceSid.resize(sidSize);
        std::vector&lt;wchar_t&gt; domain(std::max(DWORD{1}, domainSize));
        if (!::LookupAccountNameW(nullptr, name.value.c_str(), serviceSid.data(), &amp;sidSize, domain.data(), &amp;domainSize, &amp;type)) {
            report(::GetLastError()); return out;
        }
        if (type != SidTypeGroup &amp;&amp; type != SidTypeAlias &amp;&amp; type != SidTypeWellKnownGroup) {
            report(ERROR_INVALID_PRIMARY_GROUP); return out;
        }
    }
    auto sidText = [&amp;](PSID sid) -&gt; std::wstring {
        LPWSTR value = nullptr;
        if (!::ConvertSidToStringSidW(sid, &amp;value)) return {};
        std::unique_ptr&lt;wchar_t, decltype(&amp;::LocalFree)&gt; guard(value, ::LocalFree);
        return value;
    };
    const auto owner = sidText(userSid);
    const auto shared = service ? sidText(serviceSid.data()) : std::wstring{};
    if (owner.empty() || (service &amp;&amp; shared.empty())) { report(::GetLastError()); return out; }
    auto create = [&amp;](const Path&amp; path, bool readOnlyGroup) -&gt; bool {
        // Protected DACL: owner and SYSTEM full access. Service group read/execute
        // on config, modify on log/run. OI/CI apply to new descendants.
        std::wstring sddl = L"D:P(A;OICI;FA;;;" + owner + L")(A;OICI;FA;;;SY)";
        if (service) sddl += L"(A;OICI;" + std::wstring(readOnlyGroup ? L"0x1200a9" : L"0x1301bf") + L";;;" + shared + L")";
        PSECURITY_DESCRIPTOR descriptor = nullptr;
        if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &amp;descriptor, nullptr)) {
            report(::GetLastError()); return false;
        }
        std::unique_ptr&lt;void, decltype(&amp;::LocalFree)&gt; descriptorOwner(descriptor, ::LocalFree);
        SECURITY_ATTRIBUTES attributes{sizeof(attributes), descriptor, FALSE};
        if (::CreateDirectoryW(path.c_str(), &amp;attributes)) return true;
        const DWORD error = ::GetLastError();
        if (error != ERROR_ALREADY_EXISTS) { report(error); return false; }
        const DWORD attrs = ::GetFileAttributesW(path.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) { report(::GetLastError()); return false; }
        // Existing directories retain permissions; reject reparse redirection.
        if (!(attrs &amp; FILE_ATTRIBUTE_DIRECTORY) || (attrs &amp; FILE_ATTRIBUTE_REPARSE_POINT)) {
            report(ERROR_ACCESS_DENIED); return false;
        }
        return true;
    };
    // Known-folder parent exists. Create root before the three children.
    if (!create(root, false) || !create(out.configDirectory, true) || !create(out.logDirectory, false) || !create(out.pidDirectory, false)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td><td><a href="snippets/mac-5790162f8c9e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, config-types.hpp, &lt;pwd.h&gt;, &lt;grp.h&gt;.
// Place this function in the existing utils::system owner at adoption.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    const passwd* pw = ::getpwuid(::getuid());
    if (!pw) return out;
    // Copy NSS-owned data before any subsequent lookup can invalidate it.
    out.userName = pw-&gt;pw_name;
    const std::string accountHome = pw-&gt;pw_dir;
    const gid_t primaryGroup = pw-&gt;pw_gid;
    const group* gr = ::getgrgid(primaryGroup);
    if (!gr) return out;
    out.groupName = gr-&gt;gr_name;
    const uid_t owner = ::geteuid();
    const bool privileged = owner == 0;
    if (privileged) {
        out.configDirectory = "/etc/snode.c";
        out.logDirectory = "/var/log/snode.c";
        out.pidDirectory = "/var/run/snode.c";
    } else {
        const char* environmentHome = ::getenv("HOME");
        const std::string home = environmentHome &amp;&amp; *environmentHome == '/' ? environmentHome : accountHome;
        auto directory = [&amp;](const char* variable, const char* fallback) -&gt; Path {
            const char* value = ::getenv(variable);
            if (value &amp;&amp; *value == '/') return Path(value) / "snode.c";
            if (!home.empty() &amp;&amp; home.front() == '/') return Path(home + fallback) / "snode.c";
            return {};
        };
        out.configDirectory = directory("XDG_CONFIG_HOME", "/.config");
        out.logDirectory = directory("XDG_STATE_HOME", "/.local/state");
        out.pidDirectory = directory("XDG_RUNTIME_DIR", "/.local/run");
        if (out.configDirectory.empty() || out.logDirectory.empty() || out.pidDirectory.empty()) return out;
    }
    // Preserve current create-only permissions, failure ordering and warning policy.
    using P = std::filesystem::perms;
    auto prepare = [&amp;](const Path&amp; path, P permissions) -&gt; bool {
        if (!std::filesystem::create_directories(path)) return true;
        std::filesystem::permissions(path, permissions);
        if (privileged) {
            const group* service = ::getgrnam(serviceGroup.c_str());
            if (!service) {
                out.diagnostics.push_back("Cannot find service group '" + serviceGroup + "'. Create it and add the current user.");
                std::filesystem::remove(path);
                return false;
            }
            if (::chown(path.c_str(), owner, service-&gt;gr_gid) &lt; 0) {
                const int error = errno;
                out.diagnostics.push_back("Cannot set group ownership of '" + path.string() + "': " + std::generic_category().message(error));
            }
        }
        return true;
    };
    if (!prepare(out.configDirectory, P::owner_all | P::group_read | P::group_exec)) return out;
    if (!prepare(out.logDirectory, P::owner_all | P::group_all)) return out;
    if (!prepare(out.pidDirectory, privileged ? P::owner_all | P::group_all : P::owner_all)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td></tr>
<tr id="getgrgid"><td><a href="CALL_SITES.md#getgrgid"><code>getgrgid</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Result identity is an opaque native-domain principal.</p></details></td><td><pre><code>ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup);</code></pre><p>Whole Config.cpp:953–1065 replacement; see config-caller.cpp.txt and config-types.hpp.txt. POSIX preserves root/XDG/create-only ownership policy with copied NSS strings. Windows LocalSystem/known-folder/ACL policy is a proposed behavior, not a proven POSIX equivalence. Other uses of these calls remain in the source ledger and need the daemon/test-specific capsule as applicable.</p></td><td><a href="snippets/linux-5790162f8c9e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, config-types.hpp, &lt;pwd.h&gt;, &lt;grp.h&gt;.
// Place this function in the existing utils::system owner at adoption.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    const passwd* pw = ::getpwuid(::getuid());
    if (!pw) return out;
    // Copy NSS-owned data before any subsequent lookup can invalidate it.
    out.userName = pw-&gt;pw_name;
    const std::string accountHome = pw-&gt;pw_dir;
    const gid_t primaryGroup = pw-&gt;pw_gid;
    const group* gr = ::getgrgid(primaryGroup);
    if (!gr) return out;
    out.groupName = gr-&gt;gr_name;
    const uid_t owner = ::geteuid();
    const bool privileged = owner == 0;
    if (privileged) {
        out.configDirectory = "/etc/snode.c";
        out.logDirectory = "/var/log/snode.c";
        out.pidDirectory = "/var/run/snode.c";
    } else {
        const char* environmentHome = ::getenv("HOME");
        const std::string home = environmentHome &amp;&amp; *environmentHome == '/' ? environmentHome : accountHome;
        auto directory = [&amp;](const char* variable, const char* fallback) -&gt; Path {
            const char* value = ::getenv(variable);
            if (value &amp;&amp; *value == '/') return Path(value) / "snode.c";
            if (!home.empty() &amp;&amp; home.front() == '/') return Path(home + fallback) / "snode.c";
            return {};
        };
        out.configDirectory = directory("XDG_CONFIG_HOME", "/.config");
        out.logDirectory = directory("XDG_STATE_HOME", "/.local/state");
        out.pidDirectory = directory("XDG_RUNTIME_DIR", "/.local/run");
        if (out.configDirectory.empty() || out.logDirectory.empty() || out.pidDirectory.empty()) return out;
    }
    // Preserve current create-only permissions, failure ordering and warning policy.
    using P = std::filesystem::perms;
    auto prepare = [&amp;](const Path&amp; path, P permissions) -&gt; bool {
        if (!std::filesystem::create_directories(path)) return true;
        std::filesystem::permissions(path, permissions);
        if (privileged) {
            const group* service = ::getgrnam(serviceGroup.c_str());
            if (!service) {
                out.diagnostics.push_back("Cannot find service group '" + serviceGroup + "'. Create it and add the current user.");
                std::filesystem::remove(path);
                return false;
            }
            if (::chown(path.c_str(), owner, service-&gt;gr_gid) &lt; 0) {
                const int error = errno;
                out.diagnostics.push_back("Cannot set group ownership of '" + path.string() + "': " + std::generic_category().message(error));
            }
        }
        return true;
    };
    if (!prepare(out.configDirectory, P::owner_all | P::group_read | P::group_exec)) return out;
    if (!prepare(out.logDirectory, P::owner_all | P::group_all)) return out;
    if (!prepare(out.pidDirectory, privileged ? P::owner_all | P::group_all : P::owner_all)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td><td><a href="snippets/windows-94ae28bc38fd.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, windows-text.hpp, config-types.hpp,
// &lt;shlobj.h&gt;, &lt;sddl.h&gt;, &lt;aclapi.h&gt;.
// Link Advapi32, Shell32, Ole32. Uses process identity, not thread impersonation.
// POLICY PROPOSAL: normal users get LocalAppData/snode.c/{config,log,run};
// LocalSystem gets ProgramData/snode.c/{config,log,run}. Elevated non-System
// users remain per-user. No synthetic UID zero or GID is introduced.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    auto report = [&amp;](DWORD error) {
        out.diagnostics.push_back("Windows configuration setup failed: " + std::to_string(error));
    };
    HANDLE rawToken = nullptr;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &amp;rawToken)) {
        report(::GetLastError()); return out;
    }
    struct CloseToken { void operator()(void* p) const { ::CloseHandle(p); } };
    std::unique_ptr&lt;void, CloseToken&gt; token(rawToken);
    auto tokenInfo = [&amp;](TOKEN_INFORMATION_CLASS kind) -&gt; Result&lt;std::vector&lt;std::byte&gt;&gt; {
        DWORD size = 0;
        ::GetTokenInformation(token.get(), kind, nullptr, 0, &amp;size);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) return winFailure&lt;std::vector&lt;std::byte&gt;&gt;();
        std::vector&lt;std::byte&gt; bytes(size);
        if (!::GetTokenInformation(token.get(), kind, bytes.data(), size, &amp;size)) return winFailure&lt;std::vector&lt;std::byte&gt;&gt;();
        return ok(std::move(bytes));
    };
    auto user = tokenInfo(TokenUser);
    auto group = tokenInfo(TokenPrimaryGroup);
    if (!user || !group) { report((!user ? user.error : group.error).code); return out; }
    PSID userSid = reinterpret_cast&lt;TOKEN_USER*&gt;(user.value.data())-&gt;User.Sid;
    PSID groupSid = reinterpret_cast&lt;TOKEN_PRIMARY_GROUP*&gt;(group.value.data())-&gt;PrimaryGroup;
    auto nameFor = [&amp;](PSID sid) -&gt; Result&lt;std::string&gt; {
        DWORD nameSize = 0, domainSize = 0; SID_NAME_USE type;
        ::LookupAccountSidW(nullptr, sid, nullptr, &amp;nameSize, nullptr, &amp;domainSize, &amp;type);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) return winFailure&lt;std::string&gt;();
        std::vector&lt;wchar_t&gt; name(nameSize), domain(std::max(DWORD{1}, domainSize));
        if (!::LookupAccountSidW(nullptr, sid, name.data(), &amp;nameSize, domain.data(), &amp;domainSize, &amp;type)) return winFailure&lt;std::string&gt;();
        const std::wstring qualified = domainSize ? std::wstring(domain.data()) + L"\\" + name.data() : name.data();
        return toUtf8(qualified);
    };
    auto userName = nameFor(userSid), groupName = nameFor(groupSid);
    if (!userName || !groupName) { report((!userName ? userName.error : groupName.error).code); return out; }
    out.userName = std::move(userName.value); out.groupName = std::move(groupName.value);
    const bool service = ::IsWellKnownSid(userSid, WinLocalSystemSid) != FALSE;
    PWSTR folder = nullptr;
    const HRESULT status = ::SHGetKnownFolderPath(service ? FOLDERID_ProgramData : FOLDERID_LocalAppData, 0, nullptr, &amp;folder);
    if (FAILED(status)) { report(static_cast&lt;DWORD&gt;(status)); return out; }
    struct FreeFolder { void operator()(wchar_t* p) const { ::CoTaskMemFree(p); } };
    std::unique_ptr&lt;wchar_t, FreeFolder&gt; folderOwner(folder);
    const Path root = Path(folder) / L"snode.c";
    out.configDirectory = root / L"config";
    out.logDirectory = root / L"log";
    out.pidDirectory = root / L"run";

    // Resolve a configured service group to an actual SID. This is not a GID cast.
    std::vector&lt;std::byte&gt; serviceSid;
    if (service) {
        auto name = toWide(serviceGroup);
        if (!name) { report(name.error.code); return out; }
        DWORD sidSize = 0, domainSize = 0; SID_NAME_USE type;
        ::LookupAccountNameW(nullptr, name.value.c_str(), nullptr, &amp;sidSize, nullptr, &amp;domainSize, &amp;type);
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) { report(::GetLastError()); return out; }
        serviceSid.resize(sidSize);
        std::vector&lt;wchar_t&gt; domain(std::max(DWORD{1}, domainSize));
        if (!::LookupAccountNameW(nullptr, name.value.c_str(), serviceSid.data(), &amp;sidSize, domain.data(), &amp;domainSize, &amp;type)) {
            report(::GetLastError()); return out;
        }
        if (type != SidTypeGroup &amp;&amp; type != SidTypeAlias &amp;&amp; type != SidTypeWellKnownGroup) {
            report(ERROR_INVALID_PRIMARY_GROUP); return out;
        }
    }
    auto sidText = [&amp;](PSID sid) -&gt; std::wstring {
        LPWSTR value = nullptr;
        if (!::ConvertSidToStringSidW(sid, &amp;value)) return {};
        std::unique_ptr&lt;wchar_t, decltype(&amp;::LocalFree)&gt; guard(value, ::LocalFree);
        return value;
    };
    const auto owner = sidText(userSid);
    const auto shared = service ? sidText(serviceSid.data()) : std::wstring{};
    if (owner.empty() || (service &amp;&amp; shared.empty())) { report(::GetLastError()); return out; }
    auto create = [&amp;](const Path&amp; path, bool readOnlyGroup) -&gt; bool {
        // Protected DACL: owner and SYSTEM full access. Service group read/execute
        // on config, modify on log/run. OI/CI apply to new descendants.
        std::wstring sddl = L"D:P(A;OICI;FA;;;" + owner + L")(A;OICI;FA;;;SY)";
        if (service) sddl += L"(A;OICI;" + std::wstring(readOnlyGroup ? L"0x1200a9" : L"0x1301bf") + L";;;" + shared + L")";
        PSECURITY_DESCRIPTOR descriptor = nullptr;
        if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &amp;descriptor, nullptr)) {
            report(::GetLastError()); return false;
        }
        std::unique_ptr&lt;void, decltype(&amp;::LocalFree)&gt; descriptorOwner(descriptor, ::LocalFree);
        SECURITY_ATTRIBUTES attributes{sizeof(attributes), descriptor, FALSE};
        if (::CreateDirectoryW(path.c_str(), &amp;attributes)) return true;
        const DWORD error = ::GetLastError();
        if (error != ERROR_ALREADY_EXISTS) { report(error); return false; }
        const DWORD attrs = ::GetFileAttributesW(path.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) { report(::GetLastError()); return false; }
        // Existing directories retain permissions; reject reparse redirection.
        if (!(attrs &amp; FILE_ATTRIBUTE_DIRECTORY) || (attrs &amp; FILE_ATTRIBUTE_REPARSE_POINT)) {
            report(ERROR_ACCESS_DENIED); return false;
        }
        return true;
    };
    // Known-folder parent exists. Create root before the three children.
    if (!create(root, false) || !create(out.configDirectory, true) || !create(out.logDirectory, false) || !create(out.pidDirectory, false)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td><td><a href="snippets/mac-5790162f8c9e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>// Include common.hpp, config-types.hpp, &lt;pwd.h&gt;, &lt;grp.h&gt;.
// Place this function in the existing utils::system owner at adoption.
namespace core::system {
ConfigSetup doPrepareConfiguration(const std::string&amp; serviceGroup) {
    ConfigSetup out;
    const passwd* pw = ::getpwuid(::getuid());
    if (!pw) return out;
    // Copy NSS-owned data before any subsequent lookup can invalidate it.
    out.userName = pw-&gt;pw_name;
    const std::string accountHome = pw-&gt;pw_dir;
    const gid_t primaryGroup = pw-&gt;pw_gid;
    const group* gr = ::getgrgid(primaryGroup);
    if (!gr) return out;
    out.groupName = gr-&gt;gr_name;
    const uid_t owner = ::geteuid();
    const bool privileged = owner == 0;
    if (privileged) {
        out.configDirectory = "/etc/snode.c";
        out.logDirectory = "/var/log/snode.c";
        out.pidDirectory = "/var/run/snode.c";
    } else {
        const char* environmentHome = ::getenv("HOME");
        const std::string home = environmentHome &amp;&amp; *environmentHome == '/' ? environmentHome : accountHome;
        auto directory = [&amp;](const char* variable, const char* fallback) -&gt; Path {
            const char* value = ::getenv(variable);
            if (value &amp;&amp; *value == '/') return Path(value) / "snode.c";
            if (!home.empty() &amp;&amp; home.front() == '/') return Path(home + fallback) / "snode.c";
            return {};
        };
        out.configDirectory = directory("XDG_CONFIG_HOME", "/.config");
        out.logDirectory = directory("XDG_STATE_HOME", "/.local/state");
        out.pidDirectory = directory("XDG_RUNTIME_DIR", "/.local/run");
        if (out.configDirectory.empty() || out.logDirectory.empty() || out.pidDirectory.empty()) return out;
    }
    // Preserve current create-only permissions, failure ordering and warning policy.
    using P = std::filesystem::perms;
    auto prepare = [&amp;](const Path&amp; path, P permissions) -&gt; bool {
        if (!std::filesystem::create_directories(path)) return true;
        std::filesystem::permissions(path, permissions);
        if (privileged) {
            const group* service = ::getgrnam(serviceGroup.c_str());
            if (!service) {
                out.diagnostics.push_back("Cannot find service group '" + serviceGroup + "'. Create it and add the current user.");
                std::filesystem::remove(path);
                return false;
            }
            if (::chown(path.c_str(), owner, service-&gt;gr_gid) &lt; 0) {
                const int error = errno;
                out.diagnostics.push_back("Cannot set group ownership of '" + path.string() + "': " + std::generic_category().message(error));
            }
        }
        return true;
    };
    if (!prepare(out.configDirectory, P::owner_all | P::group_read | P::group_exec)) return out;
    if (!prepare(out.logDirectory, P::owner_all | P::group_all)) return out;
    if (!prepare(out.pidDirectory, privileged ? P::owner_all | P::group_all : P::owner_all)) return out;
    out.ready = true;
    return out;
}
}</code></pre></details></td></tr>
<tr id="seteuid"><td><a href="CALL_SITES.md#seteuid"><code>seteuid</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>BLOCKED for equivalent process-wide privilege changes; prefer launch-time identity when agreed; restoration must not silently widen privileges.</p></details></td><td><pre><code>Status doEnterDaemonIdentity(const std::string&amp; user, const std::string&amp; group); Status doRestoreRealIdentity();</code></pre><p>Entire Daemon.cpp identity sequence, including restore order. Startup-only; preserves effective-only changes and supplementary groups as today. Failure can occur after group changed; fail the daemon startup, never proceed as if fully switched. Windows thread impersonation does not implement this process-wide contract. A service account/launch token requires an explicitly different launch API.</p></td><td><a href="snippets/linux-c5f78e9fffeb.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doEnterDaemonIdentity(const std::string&amp; user, const std::string&amp; group) {
    errno = 0;
    const struct group* gr = ::getgrnam(group.c_str());
    if (!gr) return posixFailure(errno ? errno : ENOENT);
    if (::setegid(gr-&gt;gr_gid) != 0) return posixFailure();
    errno = 0;
    const passwd* pw = ::getpwnam(user.c_str());
    if (!pw) return posixFailure(errno ? errno : ENOENT);
    if (::seteuid(pw-&gt;pw_uid) != 0) return posixFailure();
    return ok();
}
Status doRestoreRealIdentity() {
    if (::seteuid(::getuid()) != 0) return posixFailure();
    return posixStatus(::setegid(::getgid()));
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Entire Daemon.cpp identity sequence, including restore order. Startup-only; preserves effective-only changes and supplementary groups as today. Failure can occur after group changed; fail the daemon startup, never proceed as if fully switched. Windows thread impersonation does not implement this process-wide contract. A service account/launch token requires an explicitly different launch API.</p><p>Native building blocks: No process-wide seteuid; SetThreadToken/ImpersonateLoggedOnUser or CreateProcessAsUserW have different scope</p></td><td><a href="snippets/mac-c5f78e9fffeb.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doEnterDaemonIdentity(const std::string&amp; user, const std::string&amp; group) {
    errno = 0;
    const struct group* gr = ::getgrnam(group.c_str());
    if (!gr) return posixFailure(errno ? errno : ENOENT);
    if (::setegid(gr-&gt;gr_gid) != 0) return posixFailure();
    errno = 0;
    const passwd* pw = ::getpwnam(user.c_str());
    if (!pw) return posixFailure(errno ? errno : ENOENT);
    if (::seteuid(pw-&gt;pw_uid) != 0) return posixFailure();
    return ok();
}
Status doRestoreRealIdentity() {
    if (::seteuid(::getuid()) != 0) return posixFailure();
    return posixStatus(::setegid(::getgid()));
}</code></pre></details></td></tr>
<tr id="setegid"><td><a href="CALL_SITES.md#setegid"><code>setegid</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Consolidate user and group changes into one operation; platform policy remains explicit.</p></details></td><td><pre><code>Status doEnterDaemonIdentity(const std::string&amp; user, const std::string&amp; group); Status doRestoreRealIdentity();</code></pre><p>Entire Daemon.cpp identity sequence, including restore order. Startup-only; preserves effective-only changes and supplementary groups as today. Failure can occur after group changed; fail the daemon startup, never proceed as if fully switched. Windows thread impersonation does not implement this process-wide contract. A service account/launch token requires an explicitly different launch API.</p></td><td><a href="snippets/linux-c5f78e9fffeb.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doEnterDaemonIdentity(const std::string&amp; user, const std::string&amp; group) {
    errno = 0;
    const struct group* gr = ::getgrnam(group.c_str());
    if (!gr) return posixFailure(errno ? errno : ENOENT);
    if (::setegid(gr-&gt;gr_gid) != 0) return posixFailure();
    errno = 0;
    const passwd* pw = ::getpwnam(user.c_str());
    if (!pw) return posixFailure(errno ? errno : ENOENT);
    if (::seteuid(pw-&gt;pw_uid) != 0) return posixFailure();
    return ok();
}
Status doRestoreRealIdentity() {
    if (::seteuid(::getuid()) != 0) return posixFailure();
    return posixStatus(::setegid(::getgid()));
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Entire Daemon.cpp identity sequence, including restore order. Startup-only; preserves effective-only changes and supplementary groups as today. Failure can occur after group changed; fail the daemon startup, never proceed as if fully switched. Windows thread impersonation does not implement this process-wide contract. A service account/launch token requires an explicitly different launch API.</p><p>Native building blocks: No process-wide setegid; token groups/launch token are different semantics</p></td><td><a href="snippets/mac-c5f78e9fffeb.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doEnterDaemonIdentity(const std::string&amp; user, const std::string&amp; group) {
    errno = 0;
    const struct group* gr = ::getgrnam(group.c_str());
    if (!gr) return posixFailure(errno ? errno : ENOENT);
    if (::setegid(gr-&gt;gr_gid) != 0) return posixFailure();
    errno = 0;
    const passwd* pw = ::getpwnam(user.c_str());
    if (!pw) return posixFailure(errno ? errno : ENOENT);
    if (::seteuid(pw-&gt;pw_uid) != 0) return posixFailure();
    return ok();
}
Status doRestoreRealIdentity() {
    if (::seteuid(::getuid()) != 0) return posixFailure();
    return posixStatus(::setegid(::getgid()));
}</code></pre></details></td></tr>
<tr id="_exit"><td><a href="CALL_SITES.md#_exit"><code>_exit</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>No C++ unwinding or stdio flush; Windows DLL detach differs from POSIX immediate process teardown.</p></details></td><td><pre><code>void doExitImmediately(int status);</code></pre><p>Windows DLL detach differs; not a fork-child-safe POSIX _exit equivalence.</p></td><td><a href="snippets/linux-dbcb06e0a9d5.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>void doExitImmediately(int status) {
    ::_exit(status);
}</code></pre></details></td><td><a href="snippets/windows-913ecbaae64a.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>void doExitImmediately(int status) {
    ::ExitProcess(static_cast&lt;UINT&gt;(status));
}</code></pre></details></td><td><a href="snippets/mac-dbcb06e0a9d5.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>void doExitImmediately(int status) {
    ::_exit(status);
}</code></pre></details></td></tr>
<tr id="exit"><td><a href="CALL_SITES.md#exit"><code>exit</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Normal runtime shutdown, including registered exit handlers.</p></details></td><td><pre><code>void doExit(int status);</code></pre><p>C runtime operation, not a kernel syscall. Return/lifetime/error semantics remain those of this runtime call.</p></td><td><a href="snippets/linux-a3d30022c4d8.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>void doExit(int status) {
    ::exit(status);
}</code></pre></details></td><td><a href="snippets/windows-a3d30022c4d8.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>void doExit(int status) {
    ::exit(status);
}</code></pre></details></td><td><a href="snippets/mac-a3d30022c4d8.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>void doExit(int status) {
    ::exit(status);
}</code></pre></details></td></tr>
<tr id="abort"><td><a href="CALL_SITES.md#abort"><code>abort</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Abnormal termination; platform dump/reporting behavior is not promised identical.</p></details></td><td><pre><code>void doAbort();</code></pre><p>C runtime operation, not a kernel syscall. Return/lifetime/error semantics remain those of this runtime call.</p></td><td><a href="snippets/linux-a4e605bce3d8.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>void doAbort() {
    ::abort();
}</code></pre></details></td><td><a href="snippets/windows-a4e605bce3d8.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>void doAbort() {
    ::abort();
}</code></pre></details></td><td><a href="snippets/mac-a4e605bce3d8.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>void doAbort() {
    ::abort();
}</code></pre></details></td></tr>
<tr id="atexit"><td><a href="CALL_SITES.md#atexit"><code>atexit</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>LIFO runtime exit callback registration; no kernel syscall equivalent needed.</p></details></td><td><pre><code>int doAtexit(void (*callback)());</code></pre><p>C runtime operation, not a kernel syscall. Return/lifetime/error semantics remain those of this runtime call.</p></td><td><a href="snippets/linux-419b65fbeae6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>int doAtexit(void (*callback)()) {
    return ::atexit(callback);
}</code></pre></details></td><td><a href="snippets/windows-419b65fbeae6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>int doAtexit(void (*callback)()) {
    return ::atexit(callback);
}</code></pre></details></td><td><a href="snippets/mac-419b65fbeae6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>int doAtexit(void (*callback)()) {
    return ::atexit(callback);
}</code></pre></details></td></tr>
</tbody>
</table>

## Time entropy loading and terminal

<table>
<thead><tr><th>syscall on POSIX/Linux</th><th>proposed API</th><th>implementation Linux</th><th>implementation Windows</th><th>implementation macOS</th></tr></thead>
<tbody>
<tr id="gettimeofday"><td><a href="CALL_SITES.md#gettimeofday"><code>gettimeofday</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `utils::system::gettimeofday`</p><p>UTC epoch-based wall clock; may jump; never reinterpret FILETIME epoch as Unix epoch.</p></details></td><td><pre><code>Result&lt;WallTime&gt; doWallTime();</code></pre><p></p></td><td><a href="snippets/linux-dda269e36d1d.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;WallTime&gt; doWallTime() {
    timeval tv{};
    if (::gettimeofday(&amp;tv, nullptr) == -1) return posixFailure&lt;WallTime&gt;();
    return ok(WallTime{tv.tv_sec, static_cast&lt;std::uint32_t&gt;(tv.tv_usec)});
}</code></pre></details></td><td><a href="snippets/windows-1283ce57070d.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;WallTime&gt; doWallTime() {
    FILETIME ft{};
    ::GetSystemTimePreciseAsFileTime(&amp;ft);
    const auto ticks = (static_cast&lt;std::uint64_t&gt;(ft.dwHighDateTime) &lt;&lt; 32) | ft.dwLowDateTime;
    const auto delta = static_cast&lt;std::int64_t&gt;(ticks) - INT64_C(116444736000000000);
    auto seconds = delta / INT64_C(10000000);
    auto remainder = delta % INT64_C(10000000);
    if (remainder &lt; 0) { --seconds; remainder += INT64_C(10000000); }
    return ok(WallTime{seconds, static_cast&lt;std::uint32_t&gt;(remainder / 10)});
}</code></pre></details></td><td><a href="snippets/mac-dda269e36d1d.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;WallTime&gt; doWallTime() {
    timeval tv{};
    if (::gettimeofday(&amp;tv, nullptr) == -1) return posixFailure&lt;WallTime&gt;();
    return ok(WallTime{tv.tv_sec, static_cast&lt;std::uint32_t&gt;(tv.tv_usec)});
}</code></pre></details></td></tr>
<tr id="time"><td><a href="CALL_SITES.md#time"><code>time</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `utils::system::time`</p><p>Signed seconds since Unix epoch, independent of native time_t width.</p></details></td><td><pre><code>std::int64_t doUnixSeconds();</code></pre><p></p></td><td><a href="snippets/linux-d5ae6aaa88f3.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::int64_t doUnixSeconds() {
    return static_cast&lt;std::int64_t&gt;(::time(nullptr));
}</code></pre></details></td><td><a href="snippets/windows-3e5dd3a1c10e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::int64_t doUnixSeconds() {
    return static_cast&lt;std::int64_t&gt;(::_time64(nullptr));
}</code></pre></details></td><td><a href="snippets/mac-d5ae6aaa88f3.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::int64_t doUnixSeconds() {
    return static_cast&lt;std::int64_t&gt;(::time(nullptr));
}</code></pre></details></td></tr>
<tr id="gmtime"><td><a href="CALL_SITES.md#gmtime"><code>gmtime</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `utils::system::gmtime`; inline in framework</p><p>Return owned broken-down UTC value, not shared static storage.</p></details></td><td><pre><code>Result&lt;std::tm&gt; doUtcTime(std::int64_t seconds);</code></pre><p>Owned calendar fields; OS accepted timestamp ranges differ.</p></td><td><a href="snippets/linux-910ad993fb29.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::tm&gt; doUtcTime(std::int64_t seconds) {
    const auto native = static_cast&lt;time_t&gt;(seconds);
    if (static_cast&lt;std::int64_t&gt;(native) != seconds) return posixFailure&lt;std::tm&gt;(EOVERFLOW);
    std::tm out{};
    if (!::gmtime_r(&amp;native, &amp;out)) return posixFailure&lt;std::tm&gt;();
    return ok(out);
}</code></pre></details></td><td><a href="snippets/windows-891827d29ce6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::tm&gt; doUtcTime(std::int64_t seconds) {
    const __time64_t native = seconds;
    std::tm out{};
    const int e = ::_gmtime64_s(&amp;out, &amp;native);
    if (e) return posixFailure&lt;std::tm&gt;(e);
    return ok(out);
}</code></pre></details></td><td><a href="snippets/mac-910ad993fb29.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::tm&gt; doUtcTime(std::int64_t seconds) {
    const auto native = static_cast&lt;time_t&gt;(seconds);
    if (static_cast&lt;std::int64_t&gt;(native) != seconds) return posixFailure&lt;std::tm&gt;(EOVERFLOW);
    std::tm out{};
    if (!::gmtime_r(&amp;native, &amp;out)) return posixFailure&lt;std::tm&gt;();
    return ok(out);
}</code></pre></details></td></tr>
<tr id="gmtime_r"><td><a href="CALL_SITES.md#gmtime_r"><code>gmtime_r</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Same capsule; normalize argument order, failure and calendar fields.</p></details></td><td><pre><code>Result&lt;std::tm&gt; doUtcTime(std::int64_t seconds);</code></pre><p>Owned calendar fields; OS accepted timestamp ranges differ.</p></td><td><a href="snippets/linux-910ad993fb29.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::tm&gt; doUtcTime(std::int64_t seconds) {
    const auto native = static_cast&lt;time_t&gt;(seconds);
    if (static_cast&lt;std::int64_t&gt;(native) != seconds) return posixFailure&lt;std::tm&gt;(EOVERFLOW);
    std::tm out{};
    if (!::gmtime_r(&amp;native, &amp;out)) return posixFailure&lt;std::tm&gt;();
    return ok(out);
}</code></pre></details></td><td><a href="snippets/windows-891827d29ce6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::tm&gt; doUtcTime(std::int64_t seconds) {
    const __time64_t native = seconds;
    std::tm out{};
    const int e = ::_gmtime64_s(&amp;out, &amp;native);
    if (e) return posixFailure&lt;std::tm&gt;(e);
    return ok(out);
}</code></pre></details></td><td><a href="snippets/mac-910ad993fb29.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::tm&gt; doUtcTime(std::int64_t seconds) {
    const auto native = static_cast&lt;time_t&gt;(seconds);
    if (static_cast&lt;std::int64_t&gt;(native) != seconds) return posixFailure&lt;std::tm&gt;(EOVERFLOW);
    std::tm out{};
    if (!::gmtime_r(&amp;native, &amp;out)) return posixFailure&lt;std::tm&gt;();
    return ok(out);
}</code></pre></details></td></tr>
<tr id="localtime"><td><a href="CALL_SITES.md#localtime"><code>localtime</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; demo</p><p>Owned local-calendar value; zone and DST behavior explicit.</p></details></td><td><pre><code>Result&lt;std::tm&gt; doLocalTime(std::int64_t seconds);</code></pre><p>Owned calendar fields; OS accepted timestamp ranges differ.</p></td><td><a href="snippets/linux-f3ea1e1ec113.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::tm&gt; doLocalTime(std::int64_t seconds) {
    const auto native = static_cast&lt;time_t&gt;(seconds);
    if (static_cast&lt;std::int64_t&gt;(native) != seconds) return posixFailure&lt;std::tm&gt;(EOVERFLOW);
    std::tm out{};
    if (!::localtime_r(&amp;native, &amp;out)) return posixFailure&lt;std::tm&gt;();
    return ok(out);
}</code></pre></details></td><td><a href="snippets/windows-ba07670f93c6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::tm&gt; doLocalTime(std::int64_t seconds) {
    const __time64_t native = seconds;
    std::tm out{};
    const int e = ::_localtime64_s(&amp;out, &amp;native);
    if (e) return posixFailure&lt;std::tm&gt;(e);
    return ok(out);
}</code></pre></details></td><td><a href="snippets/mac-f3ea1e1ec113.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::tm&gt; doLocalTime(std::int64_t seconds) {
    const auto native = static_cast&lt;time_t&gt;(seconds);
    if (static_cast&lt;std::int64_t&gt;(native) != seconds) return posixFailure&lt;std::tm&gt;(EOVERFLOW);
    std::tm out{};
    if (!::localtime_r(&amp;native, &amp;out)) return posixFailure&lt;std::tm&gt;();
    return ok(out);
}</code></pre></details></td></tr>
<tr id="mktime"><td><a href="CALL_SITES.md#mktime"><code>mktime</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `utils::system::mktime`</p><p>Local timezone conversion; HTTP UTC parsing must not silently switch semantics during extraction.</p></details></td><td><pre><code>std::int64_t doLocalTimeToUnix(std::tm&amp; calendar);</code></pre><p>Mutates/normalizes calendar as before; -1 can be either failure or valid pre-epoch time. HTTP UTC parsing is a separate semantic issue.</p></td><td><a href="snippets/linux-25230944d275.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::int64_t doLocalTimeToUnix(std::tm&amp; calendar) {
    return static_cast&lt;std::int64_t&gt;(::mktime(&amp;calendar));
}</code></pre></details></td><td><a href="snippets/windows-16246fbce3a8.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::int64_t doLocalTimeToUnix(std::tm&amp; calendar) {
    return static_cast&lt;std::int64_t&gt;(::_mktime64(&amp;calendar));
}</code></pre></details></td><td><a href="snippets/mac-25230944d275.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::int64_t doLocalTimeToUnix(std::tm&amp; calendar) {
    return static_cast&lt;std::int64_t&gt;(::mktime(&amp;calendar));
}</code></pre></details></td></tr>
<tr id="strftime"><td><a href="CALL_SITES.md#strftime"><code>strftime</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Specify locale and zone text; size growth/error distinguished.</p></details></td><td><pre><code>std::size_t doStrftime(char* out, std::size_t n, const char* format, const std::tm&amp; time);</code></pre><p>C runtime operation, not a kernel syscall. Return/lifetime/error semantics remain those of this runtime call.</p></td><td><a href="snippets/linux-68d96e448077.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::size_t doStrftime(char* out, std::size_t n, const char* format, const std::tm&amp; time) {
    return ::strftime(out, n, format, &amp;time);
}</code></pre></details></td><td><a href="snippets/windows-68d96e448077.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::size_t doStrftime(char* out, std::size_t n, const char* format, const std::tm&amp; time) {
    return ::strftime(out, n, format, &amp;time);
}</code></pre></details></td><td><a href="snippets/mac-68d96e448077.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::size_t doStrftime(char* out, std::size_t n, const char* format, const std::tm&amp; time) {
    return ::strftime(out, n, format, &amp;time);
}</code></pre></details></td></tr>
<tr id="strptime"><td><a href="CALL_SITES.md#strptime"><code>strptime</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Return parsed value and consumed length; HTTP date grammar needs matching behavior, not a guessed Windows syscall.</p></details></td><td><pre><code>Result&lt;std::tm&gt; doParseHttpCalendar(const std::string&amp; text);</code></pre><p>Includes &lt;sstream&gt;, &lt;locale&gt;, &lt;iomanip&gt; on Windows. Native strptime has no UCRT equivalent; this is a standard-library parser, not a syscall. Locale/accepted grammar and current mktime timezone behavior require HTTP-date tests before adoption.</p></td><td><a href="snippets/linux-f1fd1ec48535.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::tm&gt; doParseHttpCalendar(const std::string&amp; text) {
    std::tm value{};
    if (!::strptime(text.c_str(), "%a, %d %b %Y %H:%M:%S", &amp;value)) return posixFailure&lt;std::tm&gt;(EINVAL);
    return ok(value);
}</code></pre></details></td><td><a href="snippets/windows-bfa2eefc63f0.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::tm&gt; doParseHttpCalendar(const std::string&amp; text) {
    std::tm value{};
    std::istringstream input(text);
    input.imbue(std::locale::classic());
    input &gt;&gt; std::get_time(&amp;value, "%a, %d %b %Y %H:%M:%S");
    if (input.fail()) return posixFailure&lt;std::tm&gt;(EINVAL);
    return ok(value);
}</code></pre></details></td><td><a href="snippets/mac-f1fd1ec48535.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::tm&gt; doParseHttpCalendar(const std::string&amp; text) {
    std::tm value{};
    if (!::strptime(text.c_str(), "%a, %d %b %Y %H:%M:%S", &amp;value)) return posixFailure&lt;std::tm&gt;(EINVAL);
    return ok(value);
}</code></pre></details></td></tr>
<tr id="usleep"><td><a href="CALL_SITES.md#usleep"><code>usleep</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Preserve interruption outcome; no promise of exact scheduler wake-up time.</p></details></td><td><pre><code>Status doSleepMicroseconds(std::uint32_t microseconds);</code></pre><p>Observed test uses 100000 microseconds. Windows wait is not POSIX signal-interruptible; zero is rounded to a minimum timer interval here. Scheduler precision is not promised.</p></td><td><a href="snippets/linux-0de34f4286bb.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSleepMicroseconds(std::uint32_t microseconds) {
    return posixStatus(::usleep(microseconds));
}</code></pre></details></td><td><a href="snippets/windows-aa1dbd0a80ad.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSleepMicroseconds(std::uint32_t microseconds) {
    HANDLE timer = ::CreateWaitableTimerW(nullptr, TRUE, nullptr);
    if (!timer) return winFailure();
    LARGE_INTEGER due; due.QuadPart = -static_cast&lt;LONGLONG&gt;(microseconds ? microseconds : 1) * 10;
    if (!::SetWaitableTimer(timer, &amp;due, 0, nullptr, nullptr, FALSE)) {
        const DWORD e = ::GetLastError(); ::CloseHandle(timer); return winFailure(e);
    }
    const DWORD result = ::WaitForSingleObject(timer, INFINITE);
    const DWORD e = result == WAIT_FAILED ? ::GetLastError() : ERROR_SUCCESS;
    ::CloseHandle(timer);
    return result == WAIT_OBJECT_0 ? ok() : winFailure(e ? e : ERROR_INVALID_FUNCTION);
}</code></pre></details></td><td><a href="snippets/mac-0de34f4286bb.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSleepMicroseconds(std::uint32_t microseconds) {
    return posixStatus(::usleep(microseconds));
}</code></pre></details></td></tr>
<tr id="getentropy"><td><a href="CALL_SITES.md#getentropy"><code>getentropy</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Local conditional fallback; outside system owner</p><p>Cryptographic entropy; fill all or fail; do not fall back to mt19937 or random_device assumptions.</p></details></td><td><pre><code>Status doRandomBytes(MutableBytes bytes);</code></pre><p>Fill-all-or-error; buffer may contain a partially filled prefix on error and must be discarded. Replaces the entire WebSocket fallback. Linux minimum libc getentropy availability must be set; old libc variant below uses SYS_getrandom.</p></td><td><a href="snippets/linux-76fdd68f70d9.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doRandomBytes(MutableBytes bytes) {
    while (!bytes.empty()) {
        const auto n = std::min(bytes.size(), std::size_t{256});
        if (::getentropy(bytes.data(), n) != 0) {
            if (errno == EINTR) continue;
            return posixFailure();
        }
        bytes = bytes.subspan(n);
    }
    return ok();
}</code></pre></details></td><td><a href="snippets/windows-57fe54e722ba.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doRandomBytes(MutableBytes bytes) {
    while (!bytes.empty()) {
        const auto n = static_cast&lt;ULONG&gt;(std::min(bytes.size(), static_cast&lt;std::size_t&gt;(ULONG_MAX)));
        const NTSTATUS e = ::BCryptGenRandom(nullptr, reinterpret_cast&lt;PUCHAR&gt;(bytes.data()), n, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        if (e &lt; 0) return fail(ErrorDomain::ntstatus, static_cast&lt;std::uint32_t&gt;(e));
        bytes = bytes.subspan(n);
    }
    return ok();
}</code></pre></details></td><td><a href="snippets/mac-76fdd68f70d9.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doRandomBytes(MutableBytes bytes) {
    while (!bytes.empty()) {
        const auto n = std::min(bytes.size(), std::size_t{256});
        if (::getentropy(bytes.data(), n) != 0) {
            if (errno == EINTR) continue;
            return posixFailure();
        }
        bytes = bytes.subspan(n);
    }
    return ok();
}</code></pre></details></td></tr>
<tr id="syscallsys_getrandom"><td><a href="CALL_SITES.md#syscallsys_getrandom"><code>syscall:SYS_getrandom</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Same capsule consolidates the local fallback; no generic public syscall dispatcher.</p></details></td><td><pre><code>Status doRandomBytes(MutableBytes bytes);</code></pre><p>Fill-all-or-error; buffer may contain a partially filled prefix on error and must be discarded. Replaces the entire WebSocket fallback. Linux minimum libc getentropy availability must be set; old libc variant below uses SYS_getrandom.</p></td><td><a href="snippets/linux-73832aa7fcc9.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doRandomBytes(MutableBytes bytes) {
    while (!bytes.empty()) {
        const auto n = std::min(bytes.size(), std::size_t{256});
        const long done = ::syscall(SYS_getrandom, bytes.data(), n, 0);
        if (done &lt; 0) {
            if (errno == EINTR) continue;
            return posixFailure();
        }
        if (done == 0) return posixFailure(EIO);
        bytes = bytes.subspan(static_cast&lt;std::size_t&gt;(done));
    }
    return ok();
}</code></pre></details></td><td><a href="snippets/windows-57fe54e722ba.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doRandomBytes(MutableBytes bytes) {
    while (!bytes.empty()) {
        const auto n = static_cast&lt;ULONG&gt;(std::min(bytes.size(), static_cast&lt;std::size_t&gt;(ULONG_MAX)));
        const NTSTATUS e = ::BCryptGenRandom(nullptr, reinterpret_cast&lt;PUCHAR&gt;(bytes.data()), n, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        if (e &lt; 0) return fail(ErrorDomain::ntstatus, static_cast&lt;std::uint32_t&gt;(e));
        bytes = bytes.subspan(n);
    }
    return ok();
}</code></pre></details></td><td><a href="snippets/mac-76fdd68f70d9.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doRandomBytes(MutableBytes bytes) {
    while (!bytes.empty()) {
        const auto n = std::min(bytes.size(), std::size_t{256});
        if (::getentropy(bytes.data(), n) != 0) {
            if (errno == EINTR) continue;
            return posixFailure();
        }
        bytes = bytes.subspan(n);
    }
    return ok();
}</code></pre></details></td></tr>
<tr id="dlopen"><td><a href="CALL_SITES.md#dlopen"><code>dlopen</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `core::system::dlopen`</p><p>Owned module reference; search and symbol visibility policy are explicit.</p></details></td><td><pre><code>Result&lt;Library&gt; doLoadLibrary(const Path&amp; path);</code></pre><p>Absolute path, eager binding and local visibility are the proposed contract. Existing RTLD_GLOBAL/RTLD_LAZY uses must stay visible as an adoption gap, not be silently changed.</p></td><td><a href="snippets/linux-90695ea9e4c1.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Library&gt; doLoadLibrary(const Path&amp; path) {
    void* module = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!module) {
        const char* message = ::dlerror();
        return {{}, {ErrorDomain::loader, 0, message ? message : "dlopen failed"}};
    }
    return ok(Library{module});
}</code></pre></details></td><td><a href="snippets/windows-c1560b40a67c.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Library&gt; doLoadLibrary(const Path&amp; path) {
    HMODULE module = ::LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!module) return winFailure&lt;Library&gt;();
    return ok(Library{module});
}</code></pre></details></td><td><a href="snippets/mac-90695ea9e4c1.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;Library&gt; doLoadLibrary(const Path&amp; path) {
    void* module = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!module) {
        const char* message = ::dlerror();
        return {{}, {ErrorDomain::loader, 0, message ? message : "dlopen failed"}};
    }
    return ok(Library{module});
}</code></pre></details></td></tr>
<tr id="dlsym"><td><a href="CALL_SITES.md#dlsym"><code>dlsym</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Partial: `core::system::dlsym`; inline in test</p><p>Borrow module lifetime; RTLD_NEXT test interposition has no direct GetProcAddress equivalent.</p></details></td><td><pre><code>Result&lt;void*&gt; doLibrarySymbol(Library library, const char* name);</code></pre><p>Replaces both symbol lookup and dlerror clear/query. POSIX null can be a valid symbol value. RTLD_NEXT test interposition has no equivalent here; redesign that fixture separately. Calling-symbol casts must preserve the actual function type/calling convention.</p></td><td><a href="snippets/linux-50b59366ff17.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;void*&gt; doLibrarySymbol(Library library, const char* name) {
    ::dlerror();
    void* address = ::dlsym(library.native, name);
    const char* message = ::dlerror();
    if (message) return {{}, {ErrorDomain::loader, 0, message}};
    return ok(address);
}</code></pre></details></td><td><a href="snippets/windows-3d56bd3e6d2a.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;void*&gt; doLibrarySymbol(Library library, const char* name) {
    const auto address = ::GetProcAddress(library.native, name);
    if (!address) return winFailure&lt;void*&gt;();
    return ok(reinterpret_cast&lt;void*&gt;(address));
}</code></pre></details></td><td><a href="snippets/mac-50b59366ff17.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;void*&gt; doLibrarySymbol(Library library, const char* name) {
    ::dlerror();
    void* address = ::dlsym(library.native, name);
    const char* message = ::dlerror();
    if (message) return {{}, {ErrorDomain::loader, 0, message}};
    return ok(address);
}</code></pre></details></td></tr>
<tr id="dlclose"><td><a href="CALL_SITES.md#dlclose"><code>dlclose</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `core::system::dlclose`</p><p>Release one owned reference only after all dependent symbols/objects are retired.</p></details></td><td><pre><code>Status doUnloadLibrary(Library lib);</code></pre><p>Release one module reference only after its symbols are retired. POSIX loader text is captured immediately.</p></td><td><a href="snippets/linux-567b7febc7ef.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doUnloadLibrary(Library lib) {
    if (::dlclose(lib.native) == 0) return ok();
    const char* message = ::dlerror();
    return {{}, {ErrorDomain::loader, 0, message ? message : "dlclose failed"}};
}</code></pre></details></td><td><a href="snippets/windows-b14a3dbf45f4.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doUnloadLibrary(Library lib) {
    return winStatus(::FreeLibrary(lib.native));
}</code></pre></details></td><td><a href="snippets/mac-567b7febc7ef.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doUnloadLibrary(Library lib) {
    if (::dlclose(lib.native) == 0) return ok();
    const char* message = ::dlerror();
    return {{}, {ErrorDomain::loader, 0, message ? message : "dlclose failed"}};
}</code></pre></details></td></tr>
<tr id="dlerror"><td><a href="CALL_SITES.md#dlerror"><code>dlerror</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>Yes: `core::system::dlerror`</p><p>Error captured by failing Result; replaces mutable loader-error clearing/query sequence.</p></details></td><td><pre><code>Result&lt;void*&gt; doLibrarySymbol(Library library, const char* name);</code></pre><p>Replaces both symbol lookup and dlerror clear/query. POSIX null can be a valid symbol value. RTLD_NEXT test interposition has no equivalent here; redesign that fixture separately. Calling-symbol casts must preserve the actual function type/calling convention.</p></td><td><a href="snippets/linux-50b59366ff17.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;void*&gt; doLibrarySymbol(Library library, const char* name) {
    ::dlerror();
    void* address = ::dlsym(library.native, name);
    const char* message = ::dlerror();
    if (message) return {{}, {ErrorDomain::loader, 0, message}};
    return ok(address);
}</code></pre></details></td><td><a href="snippets/windows-3d56bd3e6d2a.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;void*&gt; doLibrarySymbol(Library library, const char* name) {
    const auto address = ::GetProcAddress(library.native, name);
    if (!address) return winFailure&lt;void*&gt;();
    return ok(reinterpret_cast&lt;void*&gt;(address));
}</code></pre></details></td><td><a href="snippets/mac-50b59366ff17.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;void*&gt; doLibrarySymbol(Library library, const char* name) {
    ::dlerror();
    void* address = ::dlsym(library.native, name);
    const char* message = ::dlerror();
    if (message) return {{}, {ErrorDomain::loader, 0, message}};
    return ok(address);
}</code></pre></details></td></tr>
<tr id="isatty"><td><a href="CALL_SITES.md#isatty"><code>isatty</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, tool</p><p>Terminal capability for color and job control, not merely character-device identity.</p></details></td><td><pre><code>Result&lt;bool&gt; doIsTerminal(File f);</code></pre><p>Windows console capability, not every pseudo-terminal protocol. POSIX errno ENOTTY is a successful negative result.</p></td><td><a href="snippets/linux-94060133300a.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;bool&gt; doIsTerminal(File f) {
    if (::isatty(f.native)) return ok(true);
    if (errno == ENOTTY) return ok(false);
    return posixFailure&lt;bool&gt;();
}</code></pre></details></td><td><a href="snippets/windows-0fbd3c04599c.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;bool&gt; doIsTerminal(File f) {
    DWORD mode;
    if (::GetConsoleMode(f.native, &amp;mode)) return ok(true);
    const DWORD e = ::GetLastError();
    ::SetLastError(ERROR_SUCCESS);
    const DWORD type = ::GetFileType(f.native);
    if (type == FILE_TYPE_UNKNOWN &amp;&amp; ::GetLastError() != ERROR_SUCCESS) return winFailure&lt;bool&gt;();
    if (e == ERROR_INVALID_HANDLE) return ok(false);
    return winFailure&lt;bool&gt;(e);
}</code></pre></details></td><td><a href="snippets/mac-94060133300a.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;bool&gt; doIsTerminal(File f) {
    if (::isatty(f.native)) return ok(true);
    if (errno == ENOTTY) return ok(false);
    return posixFailure&lt;bool&gt;();
}</code></pre></details></td></tr>
<tr id="tcgetattr"><td><a href="CALL_SITES.md#tcgetattr"><code>tcgetattr</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>Preserve opaque restorable state plus semantic input settings.</p></details></td><td><pre><code>Result&lt;TerminalState&gt; doBeginCharacterInput(File terminal); Status doRestoreTerminal(File terminal, const TerminalState&amp; saved);</code></pre><p>Restore on every caller exit, including exceptions. Only character-input mode intent is shared; Windows console events/Unicode/escape sequences differ from termios bytes. Check VMIN/VTIME against CursesUi before adoption.</p></td><td><a href="snippets/linux-3b5e5568f216.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>struct TerminalState { termios native; };
Result&lt;TerminalState&gt; doBeginCharacterInput(File terminal) {
    TerminalState saved{};
    if (::tcgetattr(terminal.native, &amp;saved.native) != 0) return posixFailure&lt;TerminalState&gt;();
    termios changed = saved.native;
    changed.c_lflag &amp;= static_cast&lt;tcflag_t&gt;(~(ICANON | ECHO));
    changed.c_cc[VMIN] = 1; changed.c_cc[VTIME] = 0;
    if (::tcsetattr(terminal.native, TCSANOW, &amp;changed) != 0) return posixFailure&lt;TerminalState&gt;();
    return ok(saved);
}
Status doRestoreTerminal(File terminal, const TerminalState&amp; saved) {
    return posixStatus(::tcsetattr(terminal.native, TCSANOW, &amp;saved.native));
}</code></pre></details></td><td><a href="snippets/windows-58dee9d6ab5f.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>struct TerminalState { DWORD native; };
Result&lt;TerminalState&gt; doBeginCharacterInput(File terminal) {
    TerminalState saved{};
    if (!::GetConsoleMode(terminal.native, &amp;saved.native)) return winFailure&lt;TerminalState&gt;();
    const DWORD changed = saved.native &amp; ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    if (!::SetConsoleMode(terminal.native, changed)) return winFailure&lt;TerminalState&gt;();
    return ok(saved);
}
Status doRestoreTerminal(File terminal, const TerminalState&amp; saved) {
    return winStatus(::SetConsoleMode(terminal.native, saved.native));
}</code></pre></details></td><td><a href="snippets/mac-3b5e5568f216.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>struct TerminalState { termios native; };
Result&lt;TerminalState&gt; doBeginCharacterInput(File terminal) {
    TerminalState saved{};
    if (::tcgetattr(terminal.native, &amp;saved.native) != 0) return posixFailure&lt;TerminalState&gt;();
    termios changed = saved.native;
    changed.c_lflag &amp;= static_cast&lt;tcflag_t&gt;(~(ICANON | ECHO));
    changed.c_cc[VMIN] = 1; changed.c_cc[VTIME] = 0;
    if (::tcsetattr(terminal.native, TCSANOW, &amp;changed) != 0) return posixFailure&lt;TerminalState&gt;();
    return ok(saved);
}
Status doRestoreTerminal(File terminal, const TerminalState&amp; saved) {
    return posixStatus(::tcsetattr(terminal.native, TCSANOW, &amp;saved.native));
}</code></pre></details></td></tr>
<tr id="tcsetattr"><td><a href="CALL_SITES.md#tcsetattr"><code>tcsetattr</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>Canonical mode, echo and timing differ; only agreed semantic settings cross the boundary.</p></details></td><td><pre><code>Result&lt;TerminalState&gt; doBeginCharacterInput(File terminal); Status doRestoreTerminal(File terminal, const TerminalState&amp; saved);</code></pre><p>Restore on every caller exit, including exceptions. Only character-input mode intent is shared; Windows console events/Unicode/escape sequences differ from termios bytes. Check VMIN/VTIME against CursesUi before adoption.</p></td><td><a href="snippets/linux-3b5e5568f216.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>struct TerminalState { termios native; };
Result&lt;TerminalState&gt; doBeginCharacterInput(File terminal) {
    TerminalState saved{};
    if (::tcgetattr(terminal.native, &amp;saved.native) != 0) return posixFailure&lt;TerminalState&gt;();
    termios changed = saved.native;
    changed.c_lflag &amp;= static_cast&lt;tcflag_t&gt;(~(ICANON | ECHO));
    changed.c_cc[VMIN] = 1; changed.c_cc[VTIME] = 0;
    if (::tcsetattr(terminal.native, TCSANOW, &amp;changed) != 0) return posixFailure&lt;TerminalState&gt;();
    return ok(saved);
}
Status doRestoreTerminal(File terminal, const TerminalState&amp; saved) {
    return posixStatus(::tcsetattr(terminal.native, TCSANOW, &amp;saved.native));
}</code></pre></details></td><td><a href="snippets/windows-58dee9d6ab5f.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>struct TerminalState { DWORD native; };
Result&lt;TerminalState&gt; doBeginCharacterInput(File terminal) {
    TerminalState saved{};
    if (!::GetConsoleMode(terminal.native, &amp;saved.native)) return winFailure&lt;TerminalState&gt;();
    const DWORD changed = saved.native &amp; ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    if (!::SetConsoleMode(terminal.native, changed)) return winFailure&lt;TerminalState&gt;();
    return ok(saved);
}
Status doRestoreTerminal(File terminal, const TerminalState&amp; saved) {
    return winStatus(::SetConsoleMode(terminal.native, saved.native));
}</code></pre></details></td><td><a href="snippets/mac-3b5e5568f216.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>struct TerminalState { termios native; };
Result&lt;TerminalState&gt; doBeginCharacterInput(File terminal) {
    TerminalState saved{};
    if (::tcgetattr(terminal.native, &amp;saved.native) != 0) return posixFailure&lt;TerminalState&gt;();
    termios changed = saved.native;
    changed.c_lflag &amp;= static_cast&lt;tcflag_t&gt;(~(ICANON | ECHO));
    changed.c_cc[VMIN] = 1; changed.c_cc[VTIME] = 0;
    if (::tcsetattr(terminal.native, TCSANOW, &amp;changed) != 0) return posixFailure&lt;TerminalState&gt;();
    return ok(saved);
}
Status doRestoreTerminal(File terminal, const TerminalState&amp; saved) {
    return posixStatus(::tcsetattr(terminal.native, TCSANOW, &amp;saved.native));
}</code></pre></details></td></tr>
<tr id="tcsetpgrp"><td><a href="CALL_SITES.md#tcsetpgrp"><code>tcsetpgrp</code></a><p>OS entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>BLOCKED for exact POSIX job control; lease restores the parent terminal policy after child exit.</p></details></td><td><pre><code>Status doTcsetpgrp(File terminal, std::uint64_t processGroup);</code></pre><p>POSIX-shaped body only. Native flags/structures in this signature are not yet a portable project API; see the replacement contract and the explicitly unresolved platform cell. Do not copy this shape into Windows-facing caller code.</p></td><td><a href="snippets/linux-328b368d1e23.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doTcsetpgrp(File terminal, std::uint64_t processGroup) {
    return posixStatus(::tcsetpgrp(terminal.native, static_cast&lt;pid_t&gt;(processGroup)));
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>No equivalent Windows foreground-terminal process-group syscall. Console attachment/control-event ownership needs a whole attached-job policy.</p><p>Native building blocks: No POSIX terminal foreground-group syscall; console attachment and process-group APIs differ</p></td><td><a href="snippets/mac-328b368d1e23.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doTcsetpgrp(File terminal, std::uint64_t processGroup) {
    return posixStatus(::tcsetpgrp(terminal.native, static_cast&lt;pid_t&gt;(processGroup)));
}</code></pre></details></td></tr>
</tbody>
</table>

## Runtime environment and stdio

<table>
<thead><tr><th>syscall on POSIX/Linux</th><th>proposed API</th><th>implementation Linux</th><th>implementation Windows</th><th>implementation macOS</th></tr></thead>
<tbody>
<tr id="fopen"><td><a href="CALL_SITES.md#fopen"><code>fopen</code></a><p>Runtime/library entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Owned buffered stream; distinguish text and binary semantics.</p></details></td><td><pre><code>FILE* doFopen(const Path&amp; path, bool write);</code></pre><p>Binary read or truncating binary write only; extend the semantic mode enum if another observed call needs append/update.</p></td><td><a href="snippets/linux-ea7c38eac840.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>FILE* doFopen(const Path&amp; path, bool write) {
    return ::fopen(path.c_str(), write ? "wb" : "rb");
}</code></pre></details></td><td><a href="snippets/windows-2769c34d072e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>FILE* doFopen(const Path&amp; path, bool write) {
    return ::_wfopen(path.c_str(), write ? L"wb" : L"rb");
}</code></pre></details></td><td><a href="snippets/mac-ea7c38eac840.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>FILE* doFopen(const Path&amp; path, bool write) {
    return ::fopen(path.c_str(), write ? "wb" : "rb");
}</code></pre></details></td></tr>
<tr id="freopen"><td><a href="CALL_SITES.md#freopen"><code>freopen</code></a><p>Runtime/library entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Observed use redirects standard streams to null; expose NullDevice, not a hard-coded /dev/null path.</p></details></td><td><pre><code>FILE* doRedirectToNull(FILE* stream, bool input);</code></pre><p>Replaces the complete native null-device choice in Daemon.cpp. The stream remains owned by its current owner.</p></td><td><a href="snippets/linux-8820926cc946.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>FILE* doRedirectToNull(FILE* stream, bool input) {
    return ::freopen("/dev/null", input ? "r" : "w+", stream);
}</code></pre></details></td><td><a href="snippets/windows-d1380a949aa3.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>FILE* doRedirectToNull(FILE* stream, bool input) {
    return ::_wfreopen(L"NUL", input ? L"r" : L"w+", stream);
}</code></pre></details></td><td><a href="snippets/mac-8820926cc946.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>FILE* doRedirectToNull(FILE* stream, bool input) {
    return ::freopen("/dev/null", input ? "r" : "w+", stream);
}</code></pre></details></td></tr>
<tr id="fclose"><td><a href="CALL_SITES.md#fclose"><code>fclose</code></a><p>Runtime/library entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Flush and consume ownership; flush failure remains visible.</p></details></td><td><pre><code>int doFclose(FILE* stream);</code></pre><p>C runtime operation, not a kernel syscall. Return/lifetime/error semantics remain those of this runtime call.</p></td><td><a href="snippets/linux-ba7a2504bc65.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>int doFclose(FILE* stream) {
    return ::fclose(stream);
}</code></pre></details></td><td><a href="snippets/windows-ba7a2504bc65.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>int doFclose(FILE* stream) {
    return ::fclose(stream);
}</code></pre></details></td><td><a href="snippets/mac-ba7a2504bc65.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>int doFclose(FILE* stream) {
    return ::fclose(stream);
}</code></pre></details></td></tr>
<tr id="fflush"><td><a href="CALL_SITES.md#fflush"><code>fflush</code></a><p>Runtime/library entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Flush runtime buffer; does not imply fsync/FlushFileBuffers durability.</p></details></td><td><pre><code>int doFflush(FILE* stream);</code></pre><p>C runtime operation, not a kernel syscall. Return/lifetime/error semantics remain those of this runtime call.</p></td><td><a href="snippets/linux-bb6a54c6be43.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>int doFflush(FILE* stream) {
    return ::fflush(stream);
}</code></pre></details></td><td><a href="snippets/windows-bb6a54c6be43.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>int doFflush(FILE* stream) {
    return ::fflush(stream);
}</code></pre></details></td><td><a href="snippets/mac-bb6a54c6be43.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>int doFflush(FILE* stream) {
    return ::fflush(stream);
}</code></pre></details></td></tr>
<tr id="fileno"><td><a href="CALL_SITES.md#fileno"><code>fileno</code></a><p>Runtime/library entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, test</p><p>Borrow descriptor lifetime; never close the returned handle independently of its stream.</p></details></td><td><pre><code>Result&lt;File&gt; doFileno(FILE* stream);</code></pre><p>Borrowed reference; never close independently of FILE*. CRT invalid-parameter behavior must be respected; stream must be valid.</p></td><td><a href="snippets/linux-dae541a7d9e6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doFileno(FILE* stream) {
    const int fd = ::fileno(stream);
    if (fd == -1) return posixFailure&lt;File&gt;();
    return ok(File{fd});
}</code></pre></details></td><td><a href="snippets/windows-fe422666a3a7.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doFileno(FILE* stream) {
    const int fd = ::_fileno(stream);
    if (fd == -1) return posixFailure&lt;File&gt;();
    const intptr_t h = ::_get_osfhandle(fd);
    if (h == -1) return posixFailure&lt;File&gt;();
    return ok(File{reinterpret_cast&lt;HANDLE&gt;(h)});
}</code></pre></details></td><td><a href="snippets/mac-dae541a7d9e6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doFileno(FILE* stream) {
    const int fd = ::fileno(stream);
    if (fd == -1) return posixFailure&lt;File&gt;();
    return ok(File{fd});
}</code></pre></details></td></tr>
<tr id="getenv"><td><a href="CALL_SITES.md#getenv"><code>getenv</code></a><p>Runtime/library entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; bundled CLI11, framework</p><p>Owned UTF-8 value; unset differs from empty; preserve Windows case semantics explicitly.</p></details></td><td><pre><code>Result&lt;std::optional&lt;std::string&gt;&gt; doEnvironment(const std::string&amp; name);</code></pre><p>Windows backend reads the OS environment; it does not synchronize CRT getenv caches. Migrate all framework consumers to this authority before adopting OS environment mutation. Bundled CLI11 may retain its existing implementation.</p></td><td><a href="snippets/linux-8c8d96f48bd6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::optional&lt;std::string&gt;&gt; doEnvironment(const std::string&amp; name) {
    if (name.empty() || name.find('=') != std::string::npos || name.find('\0') != std::string::npos) return posixFailure&lt;std::optional&lt;std::string&gt;&gt;(EINVAL);
    const char* p = ::getenv(name.c_str());
    return ok(p ? std::optional&lt;std::string&gt;{p} : std::nullopt);
}</code></pre></details></td><td><a href="snippets/windows-09f149512c09.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::optional&lt;std::string&gt;&gt; doEnvironment(const std::string&amp; name) {
    using Value = std::optional&lt;std::string&gt;;
    if (name.empty() || name.find('=') != std::string::npos || name.find('\0') != std::string::npos) return winFailure&lt;Value&gt;(ERROR_INVALID_PARAMETER);
    auto key = toWide(name);
    if (!key) return {{}, key.error};
    std::vector&lt;wchar_t&gt; buffer(256);
    for (;;) {
        ::SetLastError(ERROR_SUCCESS);
        const DWORD n = ::GetEnvironmentVariableW(key.value.c_str(), buffer.data(), static_cast&lt;DWORD&gt;(buffer.size()));
        if (!n) {
            const DWORD e = ::GetLastError();
            if (e == ERROR_ENVVAR_NOT_FOUND) return ok(Value{});
            if (e == ERROR_SUCCESS) return ok(Value{std::string{}});
            return winFailure&lt;Value&gt;(e);
        }
        if (n &gt;= buffer.size()) { buffer.resize(n); continue; }
        auto text = toUtf8(std::wstring_view(buffer.data(), n));
        if (!text) return {{}, text.error};
        return ok(Value{std::move(text.value)});
    }
}</code></pre></details></td><td><a href="snippets/mac-8c8d96f48bd6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::optional&lt;std::string&gt;&gt; doEnvironment(const std::string&amp; name) {
    if (name.empty() || name.find('=') != std::string::npos || name.find('\0') != std::string::npos) return posixFailure&lt;std::optional&lt;std::string&gt;&gt;(EINVAL);
    const char* p = ::getenv(name.c_str());
    return ok(p ? std::optional&lt;std::string&gt;{p} : std::nullopt);
}</code></pre></details></td></tr>
<tr id="setenv"><td><a href="CALL_SITES.md#setenv"><code>setenv</code></a><p>Runtime/library entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Both child-launch environment and any retained CRT getenv consumers must see the agreed value.</p></details></td><td><pre><code>Status doSetEnvironment(const std::string&amp; name, const std::string&amp; value, bool overwrite);</code></pre><p>Requires serialized startup-time mutation. Windows OS environment only; retained CRT/environ consumers must be migrated together. Check-then-set overwrite=false is not an atomic concurrent operation.</p></td><td><a href="snippets/linux-75dd084448fd.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSetEnvironment(const std::string&amp; name, const std::string&amp; value, bool overwrite) {
    if (name.find('\0') != std::string::npos || value.find('\0') != std::string::npos) return posixFailure(EINVAL);
    return posixStatus(::setenv(name.c_str(), value.c_str(), overwrite));
}</code></pre></details></td><td><a href="snippets/windows-b2c5184dd58f.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSetEnvironment(const std::string&amp; name, const std::string&amp; value, bool overwrite) {
    if (name.empty() || name.find('=') != std::string::npos || name.find('\0') != std::string::npos || value.find('\0') != std::string::npos) return winFailure(ERROR_INVALID_PARAMETER);
    if (!overwrite) {
        auto old = doEnvironment(name);
        if (!old) return {{}, old.error};
        if (old.value) return ok();
    }
    auto key = toWide(name); auto text = toWide(value);
    if (!key) return {{}, key.error};
    if (!text) return {{}, text.error};
    return winStatus(::SetEnvironmentVariableW(key.value.c_str(), text.value.c_str()));
}</code></pre></details></td><td><a href="snippets/mac-75dd084448fd.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSetEnvironment(const std::string&amp; name, const std::string&amp; value, bool overwrite) {
    if (name.find('\0') != std::string::npos || value.find('\0') != std::string::npos) return posixFailure(EINVAL);
    return posixStatus(::setenv(name.c_str(), value.c_str(), overwrite));
}</code></pre></details></td></tr>
<tr id="_dupenv_s"><td><a href="CALL_SITES.md#_dupenv_s"><code>_dupenv_s</code></a><p>Runtime/library entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; bundled CLI11</p><p>Conditional bundled CLI11 use; do not introduce a competing environment authority in vendor code.</p></details></td><td><pre><code>Result&lt;std::optional&lt;std::string&gt;&gt; doEnvironment(const std::string&amp; name);</code></pre><p>Windows backend reads the OS environment; it does not synchronize CRT getenv caches. Migrate all framework consumers to this authority before adopting OS environment mutation. Bundled CLI11 may retain its existing implementation.</p></td><td><a href="snippets/linux-8c8d96f48bd6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::optional&lt;std::string&gt;&gt; doEnvironment(const std::string&amp; name) {
    if (name.empty() || name.find('=') != std::string::npos || name.find('\0') != std::string::npos) return posixFailure&lt;std::optional&lt;std::string&gt;&gt;(EINVAL);
    const char* p = ::getenv(name.c_str());
    return ok(p ? std::optional&lt;std::string&gt;{p} : std::nullopt);
}</code></pre></details></td><td><a href="snippets/windows-09f149512c09.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::optional&lt;std::string&gt;&gt; doEnvironment(const std::string&amp; name) {
    using Value = std::optional&lt;std::string&gt;;
    if (name.empty() || name.find('=') != std::string::npos || name.find('\0') != std::string::npos) return winFailure&lt;Value&gt;(ERROR_INVALID_PARAMETER);
    auto key = toWide(name);
    if (!key) return {{}, key.error};
    std::vector&lt;wchar_t&gt; buffer(256);
    for (;;) {
        ::SetLastError(ERROR_SUCCESS);
        const DWORD n = ::GetEnvironmentVariableW(key.value.c_str(), buffer.data(), static_cast&lt;DWORD&gt;(buffer.size()));
        if (!n) {
            const DWORD e = ::GetLastError();
            if (e == ERROR_ENVVAR_NOT_FOUND) return ok(Value{});
            if (e == ERROR_SUCCESS) return ok(Value{std::string{}});
            return winFailure&lt;Value&gt;(e);
        }
        if (n &gt;= buffer.size()) { buffer.resize(n); continue; }
        auto text = toUtf8(std::wstring_view(buffer.data(), n));
        if (!text) return {{}, text.error};
        return ok(Value{std::move(text.value)});
    }
}</code></pre></details></td><td><a href="snippets/mac-8c8d96f48bd6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::optional&lt;std::string&gt;&gt; doEnvironment(const std::string&amp; name) {
    if (name.empty() || name.find('=') != std::string::npos || name.find('\0') != std::string::npos) return posixFailure&lt;std::optional&lt;std::string&gt;&gt;(EINVAL);
    const char* p = ::getenv(name.c_str());
    return ok(p ? std::optional&lt;std::string&gt;{p} : std::nullopt);
}</code></pre></details></td></tr>
<tr id="_stat64"><td><a href="CALL_SITES.md#_stat64"><code>_stat64</code></a><p>Runtime/library entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; bundled CLI11</p><p>Conditional bundled CLI11 use; shared file-info contract.</p></details></td><td><pre><code>Result&lt;FileInfo&gt; doPathInfo(const Path&amp; p);</code></pre><p>Windows symlink/socket reparse-tag classification is still needed before replacing security-sensitive lstat ownership checks. _stat64 is a Windows-only vendor spelling; no additional POSIX syscall.</p></td><td><a href="snippets/linux-2f802d880b80.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;FileInfo&gt; doPathInfo(const Path&amp; p) {
    struct stat st{};
    if (::stat(p.c_str(), &amp;st) == -1) return posixFailure&lt;FileInfo&gt;();
    FileInfo out;
    out.kind = S_ISREG(st.st_mode) ? FileInfo::Kind::regular : S_ISDIR(st.st_mode) ? FileInfo::Kind::directory : S_ISSOCK(st.st_mode) ? FileInfo::Kind::socket : S_ISLNK(st.st_mode) ? FileInfo::Kind::symlink : FileInfo::Kind::other;
    out.size = static_cast&lt;std::uint64_t&gt;(st.st_size); out.links = st.st_nlink; out.device = st.st_dev;
    static_assert(sizeof(st.st_ino) &lt;= sizeof(out.id));
    std::memcpy(out.id.data(), &amp;st.st_ino, sizeof(st.st_ino));
    out.modifiedSeconds = st.st_mtim.tv_sec; out.modifiedNanoseconds = st.st_mtim.tv_nsec;
    return ok(out);
}</code></pre></details></td><td><a href="snippets/windows-c42c1d9c804b.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;FileInfo&gt; doPathInfo(const Path&amp; p) {
    const HANDLE h = ::CreateFileW(p.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (h == INVALID_HANDLE_VALUE) return winFailure&lt;FileInfo&gt;();
    auto result = doFileInfo(File{h});
    const BOOL closed = ::CloseHandle(h);
    if (!closed &amp;&amp; result) return winFailure&lt;FileInfo&gt;();
    return result;
}</code></pre></details></td><td><a href="snippets/mac-535d6847c53e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;FileInfo&gt; doPathInfo(const Path&amp; p) {
    struct stat st{};
    if (::stat(p.c_str(), &amp;st) == -1) return posixFailure&lt;FileInfo&gt;();
    FileInfo out;
    out.kind = S_ISREG(st.st_mode) ? FileInfo::Kind::regular : S_ISDIR(st.st_mode) ? FileInfo::Kind::directory : S_ISSOCK(st.st_mode) ? FileInfo::Kind::socket : S_ISLNK(st.st_mode) ? FileInfo::Kind::symlink : FileInfo::Kind::other;
    out.size = static_cast&lt;std::uint64_t&gt;(st.st_size); out.links = st.st_nlink; out.device = st.st_dev;
    static_assert(sizeof(st.st_ino) &lt;= sizeof(out.id));
    std::memcpy(out.id.data(), &amp;st.st_ino, sizeof(st.st_ino));
    out.modifiedSeconds = st.st_mtimespec.tv_sec; out.modifiedNanoseconds = st.st_mtimespec.tv_nsec;
    return ok(out);
}</code></pre></details></td></tr>
<tr id="getcommandlinew"><td><a href="CALL_SITES.md#getcommandlinew"><code>GetCommandLineW</code></a><p>Runtime/library entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; bundled CLI11</p><p>Owned UTF-8 arguments; Windows raw command string is not a portable representation.</p></details></td><td><pre><code>Result&lt;std::vector&lt;std::string&gt;&gt; doArguments(int argc, char* const* argv);</code></pre><p>Includes &lt;memory&gt;. Consolidates Windows command-line allocation, Unicode conversion and LocalFree. POSIX argv must be valid and is preserved byte-for-byte; this does not enforce UTF-8 on arbitrary POSIX argument bytes.</p></td><td><a href="snippets/linux-da2ada651582.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::vector&lt;std::string&gt;&gt; doArguments(int argc, char* const* argv) {
    if (argc &lt; 0 || (argc &amp;&amp; !argv)) return posixFailure&lt;std::vector&lt;std::string&gt;&gt;(EINVAL);
    std::vector&lt;std::string&gt; result;
    for (int i = 0; i &lt; argc; ++i) result.emplace_back(argv[i]);
    return ok(std::move(result));
}</code></pre></details></td><td><a href="snippets/windows-7d235682a939.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::vector&lt;std::string&gt;&gt; doArguments(int, char* const*) {
    int count = 0;
    wchar_t** values = ::CommandLineToArgvW(::GetCommandLineW(), &amp;count);
    if (!values) return winFailure&lt;std::vector&lt;std::string&gt;&gt;();
    std::unique_ptr&lt;wchar_t*, decltype(&amp;::LocalFree)&gt; guard(values, ::LocalFree);
    std::vector&lt;std::string&gt; result;
    for (int i = 0; i &lt; count; ++i) {
        auto text = toUtf8(values[i]);
        if (!text) return {{}, text.error};
        result.push_back(std::move(text.value));
    }
    return ok(std::move(result));
}</code></pre></details></td><td><a href="snippets/mac-da2ada651582.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::vector&lt;std::string&gt;&gt; doArguments(int argc, char* const* argv) {
    if (argc &lt; 0 || (argc &amp;&amp; !argv)) return posixFailure&lt;std::vector&lt;std::string&gt;&gt;(EINVAL);
    std::vector&lt;std::string&gt; result;
    for (int i = 0; i &lt; argc; ++i) result.emplace_back(argv[i]);
    return ok(std::move(result));
}</code></pre></details></td></tr>
<tr id="commandlinetoargvw"><td><a href="CALL_SITES.md#commandlinetoargvw"><code>CommandLineToArgvW</code></a><p>Runtime/library entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; bundled CLI11</p><p>Capsule defines Windows quoting/parser policy; bundled CLI11 already contains this branch.</p></details></td><td><pre><code>Result&lt;std::vector&lt;std::string&gt;&gt; doArguments(int argc, char* const* argv);</code></pre><p>Includes &lt;memory&gt;. Consolidates Windows command-line allocation, Unicode conversion and LocalFree. POSIX argv must be valid and is preserved byte-for-byte; this does not enforce UTF-8 on arbitrary POSIX argument bytes.</p></td><td><a href="snippets/linux-da2ada651582.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::vector&lt;std::string&gt;&gt; doArguments(int argc, char* const* argv) {
    if (argc &lt; 0 || (argc &amp;&amp; !argv)) return posixFailure&lt;std::vector&lt;std::string&gt;&gt;(EINVAL);
    std::vector&lt;std::string&gt; result;
    for (int i = 0; i &lt; argc; ++i) result.emplace_back(argv[i]);
    return ok(std::move(result));
}</code></pre></details></td><td><a href="snippets/windows-7d235682a939.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::vector&lt;std::string&gt;&gt; doArguments(int, char* const*) {
    int count = 0;
    wchar_t** values = ::CommandLineToArgvW(::GetCommandLineW(), &amp;count);
    if (!values) return winFailure&lt;std::vector&lt;std::string&gt;&gt;();
    std::unique_ptr&lt;wchar_t*, decltype(&amp;::LocalFree)&gt; guard(values, ::LocalFree);
    std::vector&lt;std::string&gt; result;
    for (int i = 0; i &lt; count; ++i) {
        auto text = toUtf8(values[i]);
        if (!text) return {{}, text.error};
        result.push_back(std::move(text.value));
    }
    return ok(std::move(result));
}</code></pre></details></td><td><a href="snippets/mac-da2ada651582.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::vector&lt;std::string&gt;&gt; doArguments(int argc, char* const* argv) {
    if (argc &lt; 0 || (argc &amp;&amp; !argv)) return posixFailure&lt;std::vector&lt;std::string&gt;&gt;(EINVAL);
    std::vector&lt;std::string&gt; result;
    for (int i = 0; i &lt; argc; ++i) result.emplace_back(argv[i]);
    return ok(std::move(result));
}</code></pre></details></td></tr>
<tr id="localfree"><td><a href="CALL_SITES.md#localfree"><code>LocalFree</code></a><p>Runtime/library entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; bundled CLI11</p><p>Release only capsule-owned argument storage with matching native allocator; preserve CLI11 ownership if retained.</p></details></td><td><pre><code>Result&lt;std::vector&lt;std::string&gt;&gt; doArguments(int argc, char* const* argv);</code></pre><p>Includes &lt;memory&gt;. Consolidates Windows command-line allocation, Unicode conversion and LocalFree. POSIX argv must be valid and is preserved byte-for-byte; this does not enforce UTF-8 on arbitrary POSIX argument bytes.</p></td><td><a href="snippets/linux-da2ada651582.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::vector&lt;std::string&gt;&gt; doArguments(int argc, char* const* argv) {
    if (argc &lt; 0 || (argc &amp;&amp; !argv)) return posixFailure&lt;std::vector&lt;std::string&gt;&gt;(EINVAL);
    std::vector&lt;std::string&gt; result;
    for (int i = 0; i &lt; argc; ++i) result.emplace_back(argv[i]);
    return ok(std::move(result));
}</code></pre></details></td><td><a href="snippets/windows-7d235682a939.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::vector&lt;std::string&gt;&gt; doArguments(int, char* const*) {
    int count = 0;
    wchar_t** values = ::CommandLineToArgvW(::GetCommandLineW(), &amp;count);
    if (!values) return winFailure&lt;std::vector&lt;std::string&gt;&gt;();
    std::unique_ptr&lt;wchar_t*, decltype(&amp;::LocalFree)&gt; guard(values, ::LocalFree);
    std::vector&lt;std::string&gt; result;
    for (int i = 0; i &lt; count; ++i) {
        auto text = toUtf8(values[i]);
        if (!text) return {{}, text.error};
        result.push_back(std::move(text.value));
    }
    return ok(std::move(result));
}</code></pre></details></td><td><a href="snippets/mac-da2ada651582.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::vector&lt;std::string&gt;&gt; doArguments(int argc, char* const* argv) {
    if (argc &lt; 0 || (argc &amp;&amp; !argv)) return posixFailure&lt;std::vector&lt;std::string&gt;&gt;(EINVAL);
    std::vector&lt;std::string&gt; result;
    for (int i = 0; i &lt; argc; ++i) result.emplace_back(argv[i]);
    return ok(std::move(result));
}</code></pre></details></td></tr>
<tr id="getlasterror"><td><a href="CALL_SITES.md#getlasterror"><code>GetLastError</code></a><p>Runtime/library entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; bundled CLI11</p><p>Backend-only capture; project code should consume Result.error rather than read ambient error state.</p></details></td><td><pre><code>Error doCaptureError(ErrorDomain domain);</code></pre><p>Backend helper only. Never call for resolver APIs that return their error code directly, or after another native call has overwritten the error.</p></td><td><a href="snippets/linux-685051b786e7.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Error doCaptureError(ErrorDomain domain) {
    if (domain == ErrorDomain::posix) return {domain, static_cast&lt;std::uint32_t&gt;(errno)};
    return {ErrorDomain::posix, EINVAL};
}</code></pre></details></td><td><a href="snippets/windows-95e796759679.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Error doCaptureError(ErrorDomain domain) {
    if (domain == ErrorDomain::win32) return {domain, ::GetLastError()};
    if (domain == ErrorDomain::winsock) return {domain, static_cast&lt;std::uint32_t&gt;(::WSAGetLastError())};
    if (domain == ErrorDomain::posix) return {domain, static_cast&lt;std::uint32_t&gt;(errno)};
    return {ErrorDomain::win32, ERROR_INVALID_PARAMETER};
}</code></pre></details></td><td><a href="snippets/mac-685051b786e7.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Error doCaptureError(ErrorDomain domain) {
    if (domain == ErrorDomain::posix) return {domain, static_cast&lt;std::uint32_t&gt;(errno)};
    return {ErrorDomain::posix, EINVAL};
}</code></pre></details></td></tr>
<tr id="strerror"><td><a href="CALL_SITES.md#strerror"><code>strerror</code></a><p>Runtime/library entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; demo, framework, test, tool</p><p>Owned message; Error retains native domain/code and normalized category.</p></details></td><td><pre><code>std::string doErrorText(const Error&amp; error);</code></pre><p>Includes &lt;system_error&gt; and &lt;memory&gt;. Returned text is owned; not a pointer into a shared mutable error buffer.</p></td><td><a href="snippets/linux-d9f080799cdb.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::string doErrorText(const Error&amp; error) {
    if (!error.text.empty()) return error.text;
    if (error.domain == ErrorDomain::resolver) return ::gai_strerror(static_cast&lt;int&gt;(error.code));
    if (error.domain == ErrorDomain::posix) return std::generic_category().message(static_cast&lt;int&gt;(error.code));
    return "native error " + std::to_string(error.code);
}</code></pre></details></td><td><a href="snippets/windows-9d0b775dccb8.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::string doErrorText(const Error&amp; error) {
    if (!error.text.empty()) return error.text;
    if (error.domain == ErrorDomain::posix) return std::generic_category().message(static_cast&lt;int&gt;(error.code));
    if (error.domain == ErrorDomain::ntstatus) return "NTSTATUS " + std::to_string(error.code);
    wchar_t* buffer = nullptr;
    const DWORD n = ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error.code, 0, reinterpret_cast&lt;wchar_t*&gt;(&amp;buffer), 0, nullptr);
    if (!n) return "native error " + std::to_string(error.code);
    std::unique_ptr&lt;wchar_t, decltype(&amp;::LocalFree)&gt; guard(buffer, ::LocalFree);
    auto text = toUtf8(std::wstring_view(buffer, n));
    return text ? std::move(text.value) : "native error " + std::to_string(error.code);
}</code></pre></details></td><td><a href="snippets/mac-d9f080799cdb.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::string doErrorText(const Error&amp; error) {
    if (!error.text.empty()) return error.text;
    if (error.domain == ErrorDomain::resolver) return ::gai_strerror(static_cast&lt;int&gt;(error.code));
    if (error.domain == ErrorDomain::posix) return std::generic_category().message(static_cast&lt;int&gt;(error.code));
    return "native error " + std::to_string(error.code);
}</code></pre></details></td></tr>
<tr id="setlocale"><td><a href="CALL_SITES.md#setlocale"><code>setlocale</code></a><p>Runtime/library entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; bundled CLI11</p><p>Process-global mutation and restoration; CLI11 use must not conflict with UTF-8 boundary policy.</p></details></td><td><pre><code>char* doSetlocale(int category, const char* locale);</code></pre><p>C runtime operation, not a kernel syscall. Return/lifetime/error semantics remain those of this runtime call.</p></td><td><a href="snippets/linux-4915a12aa656.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>char* doSetlocale(int category, const char* locale) {
    return ::setlocale(category, locale);
}</code></pre></details></td><td><a href="snippets/windows-4915a12aa656.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>char* doSetlocale(int category, const char* locale) {
    return ::setlocale(category, locale);
}</code></pre></details></td><td><a href="snippets/mac-4915a12aa656.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>char* doSetlocale(int category, const char* locale) {
    return ::setlocale(category, locale);
}</code></pre></details></td></tr>
<tr id="strcasecmp"><td><a href="CALL_SITES.md#strcasecmp"><code>strcasecmp</code></a><p>Runtime/library entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>HTTP field names require ASCII case folding; changing current locale-dependent behavior is a distinct review decision.</p></details></td><td><pre><code>int doStrcasecmp(const char* a, const char* b);</code></pre><p>Locale-dependent comparison retained. HTTP ASCII-only comparison should be one shared protocol helper, not three OS implementations.</p></td><td><a href="snippets/linux-335df6cb9f15.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>int doStrcasecmp(const char* a, const char* b) {
    return ::strcasecmp(a, b);
}</code></pre></details></td><td><a href="snippets/windows-03803c060ca8.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>int doStrcasecmp(const char* a, const char* b) {
    return ::_stricmp(a, b);
}</code></pre></details></td><td><a href="snippets/mac-335df6cb9f15.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>int doStrcasecmp(const char* a, const char* b) {
    return ::strcasecmp(a, b);
}</code></pre></details></td></tr>
<tr id="strdup"><td><a href="CALL_SITES.md#strdup"><code>strdup</code></a><p>Runtime/library entry</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Paired release must use the same allocator; TLS password userdata lifetime stays unchanged.</p></details></td><td><pre><code>char* doStrdup(const char* text);</code></pre><p>Caller frees with matching CRT free. Allocation failure remains nullptr.</p></td><td><a href="snippets/linux-33d4209e7429.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>char* doStrdup(const char* text) {
    return ::strdup(text);
}</code></pre></details></td><td><a href="snippets/windows-d89250c005cd.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>char* doStrdup(const char* text) {
    return ::_strdup(text);
}</code></pre></details></td><td><a href="snippets/mac-33d4209e7429.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>char* doStrdup(const char* text) {
    return ::strdup(text);
}</code></pre></details></td></tr>
</tbody>
</table>

## Macros and representation helpers

<table>
<thead><tr><th>syscall on POSIX/Linux</th><th>proposed API</th><th>implementation Linux</th><th>implementation Windows</th><th>implementation macOS</th></tr></thead>
<tbody>
<tr id="htobe16"><td><a href="CALL_SITES.md#htobe16"><code>htobe16</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Pure integer conversion; no platform kernel syscall is involved.</p></details></td><td><pre><code>std::uint16_t doHostToBig16(std::uint16_t v);</code></pre><p>Pure C++20 byte-order conversion. Same body on all three platforms; no kernel call exists or is needed.</p></td><td><a href="snippets/linux-a0844de800b5.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint16_t doHostToBig16(std::uint16_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint16_t&gt;(((v &gt;&gt; 0) &amp; UINT16_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 8) &amp; UINT16_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/windows-a0844de800b5.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint16_t doHostToBig16(std::uint16_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint16_t&gt;(((v &gt;&gt; 0) &amp; UINT16_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 8) &amp; UINT16_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/mac-a0844de800b5.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint16_t doHostToBig16(std::uint16_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint16_t&gt;(((v &gt;&gt; 0) &amp; UINT16_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 8) &amp; UINT16_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td></tr>
<tr id="htobe32"><td><a href="CALL_SITES.md#htobe32"><code>htobe32</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Pure integer conversion; no platform kernel syscall is involved.</p></details></td><td><pre><code>std::uint32_t doHostToBig32(std::uint32_t v);</code></pre><p>Pure C++20 byte-order conversion. Same body on all three platforms; no kernel call exists or is needed.</p></td><td><a href="snippets/linux-9e0d3f168950.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint32_t doHostToBig32(std::uint32_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint32_t&gt;(((v &gt;&gt; 0) &amp; UINT32_C(0xff)) &lt;&lt; 24 | ((v &gt;&gt; 8) &amp; UINT32_C(0xff)) &lt;&lt; 16 | ((v &gt;&gt; 16) &amp; UINT32_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 24) &amp; UINT32_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/windows-9e0d3f168950.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint32_t doHostToBig32(std::uint32_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint32_t&gt;(((v &gt;&gt; 0) &amp; UINT32_C(0xff)) &lt;&lt; 24 | ((v &gt;&gt; 8) &amp; UINT32_C(0xff)) &lt;&lt; 16 | ((v &gt;&gt; 16) &amp; UINT32_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 24) &amp; UINT32_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/mac-9e0d3f168950.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint32_t doHostToBig32(std::uint32_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint32_t&gt;(((v &gt;&gt; 0) &amp; UINT32_C(0xff)) &lt;&lt; 24 | ((v &gt;&gt; 8) &amp; UINT32_C(0xff)) &lt;&lt; 16 | ((v &gt;&gt; 16) &amp; UINT32_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 24) &amp; UINT32_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td></tr>
<tr id="htobe64"><td><a href="CALL_SITES.md#htobe64"><code>htobe64</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Pure integer conversion; no platform kernel syscall is involved.</p></details></td><td><pre><code>std::uint64_t doHostToBig64(std::uint64_t v);</code></pre><p>Pure C++20 byte-order conversion. Same body on all three platforms; no kernel call exists or is needed.</p></td><td><a href="snippets/linux-fe7e1e378418.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint64_t doHostToBig64(std::uint64_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint64_t&gt;(((v &gt;&gt; 0) &amp; UINT64_C(0xff)) &lt;&lt; 56 | ((v &gt;&gt; 8) &amp; UINT64_C(0xff)) &lt;&lt; 48 | ((v &gt;&gt; 16) &amp; UINT64_C(0xff)) &lt;&lt; 40 | ((v &gt;&gt; 24) &amp; UINT64_C(0xff)) &lt;&lt; 32 | ((v &gt;&gt; 32) &amp; UINT64_C(0xff)) &lt;&lt; 24 | ((v &gt;&gt; 40) &amp; UINT64_C(0xff)) &lt;&lt; 16 | ((v &gt;&gt; 48) &amp; UINT64_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 56) &amp; UINT64_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/windows-fe7e1e378418.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint64_t doHostToBig64(std::uint64_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint64_t&gt;(((v &gt;&gt; 0) &amp; UINT64_C(0xff)) &lt;&lt; 56 | ((v &gt;&gt; 8) &amp; UINT64_C(0xff)) &lt;&lt; 48 | ((v &gt;&gt; 16) &amp; UINT64_C(0xff)) &lt;&lt; 40 | ((v &gt;&gt; 24) &amp; UINT64_C(0xff)) &lt;&lt; 32 | ((v &gt;&gt; 32) &amp; UINT64_C(0xff)) &lt;&lt; 24 | ((v &gt;&gt; 40) &amp; UINT64_C(0xff)) &lt;&lt; 16 | ((v &gt;&gt; 48) &amp; UINT64_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 56) &amp; UINT64_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/mac-fe7e1e378418.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint64_t doHostToBig64(std::uint64_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint64_t&gt;(((v &gt;&gt; 0) &amp; UINT64_C(0xff)) &lt;&lt; 56 | ((v &gt;&gt; 8) &amp; UINT64_C(0xff)) &lt;&lt; 48 | ((v &gt;&gt; 16) &amp; UINT64_C(0xff)) &lt;&lt; 40 | ((v &gt;&gt; 24) &amp; UINT64_C(0xff)) &lt;&lt; 32 | ((v &gt;&gt; 32) &amp; UINT64_C(0xff)) &lt;&lt; 24 | ((v &gt;&gt; 40) &amp; UINT64_C(0xff)) &lt;&lt; 16 | ((v &gt;&gt; 48) &amp; UINT64_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 56) &amp; UINT64_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td></tr>
<tr id="be16toh"><td><a href="CALL_SITES.md#be16toh"><code>be16toh</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Pure integer conversion; no platform kernel syscall is involved.</p></details></td><td><pre><code>std::uint16_t doBigToHost16(std::uint16_t v);</code></pre><p>Pure C++20 byte-order conversion. Same body on all three platforms; no kernel call exists or is needed.</p></td><td><a href="snippets/linux-6679630bcf4b.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint16_t doBigToHost16(std::uint16_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint16_t&gt;(((v &gt;&gt; 0) &amp; UINT16_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 8) &amp; UINT16_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/windows-6679630bcf4b.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint16_t doBigToHost16(std::uint16_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint16_t&gt;(((v &gt;&gt; 0) &amp; UINT16_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 8) &amp; UINT16_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/mac-6679630bcf4b.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint16_t doBigToHost16(std::uint16_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint16_t&gt;(((v &gt;&gt; 0) &amp; UINT16_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 8) &amp; UINT16_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td></tr>
<tr id="be32toh"><td><a href="CALL_SITES.md#be32toh"><code>be32toh</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Pure integer conversion; no platform kernel syscall is involved.</p></details></td><td><pre><code>std::uint32_t doBigToHost32(std::uint32_t v);</code></pre><p>Pure C++20 byte-order conversion. Same body on all three platforms; no kernel call exists or is needed.</p></td><td><a href="snippets/linux-bc2d76615fca.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint32_t doBigToHost32(std::uint32_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint32_t&gt;(((v &gt;&gt; 0) &amp; UINT32_C(0xff)) &lt;&lt; 24 | ((v &gt;&gt; 8) &amp; UINT32_C(0xff)) &lt;&lt; 16 | ((v &gt;&gt; 16) &amp; UINT32_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 24) &amp; UINT32_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/windows-bc2d76615fca.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint32_t doBigToHost32(std::uint32_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint32_t&gt;(((v &gt;&gt; 0) &amp; UINT32_C(0xff)) &lt;&lt; 24 | ((v &gt;&gt; 8) &amp; UINT32_C(0xff)) &lt;&lt; 16 | ((v &gt;&gt; 16) &amp; UINT32_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 24) &amp; UINT32_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/mac-bc2d76615fca.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint32_t doBigToHost32(std::uint32_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint32_t&gt;(((v &gt;&gt; 0) &amp; UINT32_C(0xff)) &lt;&lt; 24 | ((v &gt;&gt; 8) &amp; UINT32_C(0xff)) &lt;&lt; 16 | ((v &gt;&gt; 16) &amp; UINT32_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 24) &amp; UINT32_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td></tr>
<tr id="be64toh"><td><a href="CALL_SITES.md#be64toh"><code>be64toh</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Pure integer conversion; no platform kernel syscall is involved.</p></details></td><td><pre><code>std::uint64_t doBigToHost64(std::uint64_t v);</code></pre><p>Pure C++20 byte-order conversion. Same body on all three platforms; no kernel call exists or is needed.</p></td><td><a href="snippets/linux-d51caf9098d5.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint64_t doBigToHost64(std::uint64_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint64_t&gt;(((v &gt;&gt; 0) &amp; UINT64_C(0xff)) &lt;&lt; 56 | ((v &gt;&gt; 8) &amp; UINT64_C(0xff)) &lt;&lt; 48 | ((v &gt;&gt; 16) &amp; UINT64_C(0xff)) &lt;&lt; 40 | ((v &gt;&gt; 24) &amp; UINT64_C(0xff)) &lt;&lt; 32 | ((v &gt;&gt; 32) &amp; UINT64_C(0xff)) &lt;&lt; 24 | ((v &gt;&gt; 40) &amp; UINT64_C(0xff)) &lt;&lt; 16 | ((v &gt;&gt; 48) &amp; UINT64_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 56) &amp; UINT64_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/windows-d51caf9098d5.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint64_t doBigToHost64(std::uint64_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint64_t&gt;(((v &gt;&gt; 0) &amp; UINT64_C(0xff)) &lt;&lt; 56 | ((v &gt;&gt; 8) &amp; UINT64_C(0xff)) &lt;&lt; 48 | ((v &gt;&gt; 16) &amp; UINT64_C(0xff)) &lt;&lt; 40 | ((v &gt;&gt; 24) &amp; UINT64_C(0xff)) &lt;&lt; 32 | ((v &gt;&gt; 32) &amp; UINT64_C(0xff)) &lt;&lt; 24 | ((v &gt;&gt; 40) &amp; UINT64_C(0xff)) &lt;&lt; 16 | ((v &gt;&gt; 48) &amp; UINT64_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 56) &amp; UINT64_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/mac-d51caf9098d5.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint64_t doBigToHost64(std::uint64_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint64_t&gt;(((v &gt;&gt; 0) &amp; UINT64_C(0xff)) &lt;&lt; 56 | ((v &gt;&gt; 8) &amp; UINT64_C(0xff)) &lt;&lt; 48 | ((v &gt;&gt; 16) &amp; UINT64_C(0xff)) &lt;&lt; 40 | ((v &gt;&gt; 24) &amp; UINT64_C(0xff)) &lt;&lt; 32 | ((v &gt;&gt; 32) &amp; UINT64_C(0xff)) &lt;&lt; 24 | ((v &gt;&gt; 40) &amp; UINT64_C(0xff)) &lt;&lt; 16 | ((v &gt;&gt; 48) &amp; UINT64_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 56) &amp; UINT64_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td></tr>
<tr id="htons"><td><a href="CALL_SITES.md#htons"><code>htons</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Pure integer conversion; no platform kernel syscall is involved.</p></details></td><td><pre><code>std::uint16_t doHostToBig16(std::uint16_t v);</code></pre><p>Pure C++20 byte-order conversion. Same body on all three platforms; no kernel call exists or is needed.</p></td><td><a href="snippets/linux-a0844de800b5.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint16_t doHostToBig16(std::uint16_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint16_t&gt;(((v &gt;&gt; 0) &amp; UINT16_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 8) &amp; UINT16_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/windows-a0844de800b5.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint16_t doHostToBig16(std::uint16_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint16_t&gt;(((v &gt;&gt; 0) &amp; UINT16_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 8) &amp; UINT16_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/mac-a0844de800b5.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint16_t doHostToBig16(std::uint16_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint16_t&gt;(((v &gt;&gt; 0) &amp; UINT16_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 8) &amp; UINT16_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td></tr>
<tr id="htonl"><td><a href="CALL_SITES.md#htonl"><code>htonl</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Pure integer conversion; no platform kernel syscall is involved.</p></details></td><td><pre><code>std::uint32_t doHostToBig32(std::uint32_t v);</code></pre><p>Pure C++20 byte-order conversion. Same body on all three platforms; no kernel call exists or is needed.</p></td><td><a href="snippets/linux-9e0d3f168950.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint32_t doHostToBig32(std::uint32_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint32_t&gt;(((v &gt;&gt; 0) &amp; UINT32_C(0xff)) &lt;&lt; 24 | ((v &gt;&gt; 8) &amp; UINT32_C(0xff)) &lt;&lt; 16 | ((v &gt;&gt; 16) &amp; UINT32_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 24) &amp; UINT32_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/windows-9e0d3f168950.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint32_t doHostToBig32(std::uint32_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint32_t&gt;(((v &gt;&gt; 0) &amp; UINT32_C(0xff)) &lt;&lt; 24 | ((v &gt;&gt; 8) &amp; UINT32_C(0xff)) &lt;&lt; 16 | ((v &gt;&gt; 16) &amp; UINT32_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 24) &amp; UINT32_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/mac-9e0d3f168950.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint32_t doHostToBig32(std::uint32_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint32_t&gt;(((v &gt;&gt; 0) &amp; UINT32_C(0xff)) &lt;&lt; 24 | ((v &gt;&gt; 8) &amp; UINT32_C(0xff)) &lt;&lt; 16 | ((v &gt;&gt; 16) &amp; UINT32_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 24) &amp; UINT32_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td></tr>
<tr id="ntohs"><td><a href="CALL_SITES.md#ntohs"><code>ntohs</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test</p><p>Pure integer conversion; no platform kernel syscall is involved.</p></details></td><td><pre><code>std::uint16_t doBigToHost16(std::uint16_t v);</code></pre><p>Pure C++20 byte-order conversion. Same body on all three platforms; no kernel call exists or is needed.</p></td><td><a href="snippets/linux-6679630bcf4b.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint16_t doBigToHost16(std::uint16_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint16_t&gt;(((v &gt;&gt; 0) &amp; UINT16_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 8) &amp; UINT16_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/windows-6679630bcf4b.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint16_t doBigToHost16(std::uint16_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint16_t&gt;(((v &gt;&gt; 0) &amp; UINT16_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 8) &amp; UINT16_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/mac-6679630bcf4b.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint16_t doBigToHost16(std::uint16_t v) {
    if constexpr (std::endian::native == std::endian::big) return v;
    return static_cast&lt;std::uint16_t&gt;(((v &gt;&gt; 0) &amp; UINT16_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 8) &amp; UINT16_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td></tr>
<tr id="htobs"><td><a href="CALL_SITES.md#htobs"><code>htobs</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Pure integer conversion; no platform kernel syscall is involved.</p></details></td><td><pre><code>std::uint16_t doHostToLittle16(std::uint16_t v);</code></pre><p>Pure C++20 byte-order conversion. Same body on all three platforms; no kernel call exists or is needed.</p></td><td><a href="snippets/linux-56558ad89db8.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint16_t doHostToLittle16(std::uint16_t v) {
    if constexpr (std::endian::native == std::endian::little) return v;
    return static_cast&lt;std::uint16_t&gt;(((v &gt;&gt; 0) &amp; UINT16_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 8) &amp; UINT16_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/windows-56558ad89db8.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint16_t doHostToLittle16(std::uint16_t v) {
    if constexpr (std::endian::native == std::endian::little) return v;
    return static_cast&lt;std::uint16_t&gt;(((v &gt;&gt; 0) &amp; UINT16_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 8) &amp; UINT16_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/mac-56558ad89db8.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint16_t doHostToLittle16(std::uint16_t v) {
    if constexpr (std::endian::native == std::endian::little) return v;
    return static_cast&lt;std::uint16_t&gt;(((v &gt;&gt; 0) &amp; UINT16_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 8) &amp; UINT16_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td></tr>
<tr id="btohs"><td><a href="CALL_SITES.md#btohs"><code>btohs</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Pure integer conversion; no platform kernel syscall is involved.</p></details></td><td><pre><code>std::uint16_t doLittleToHost16(std::uint16_t v);</code></pre><p>Pure C++20 byte-order conversion. Same body on all three platforms; no kernel call exists or is needed.</p></td><td><a href="snippets/linux-7ec41ff49437.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint16_t doLittleToHost16(std::uint16_t v) {
    if constexpr (std::endian::native == std::endian::little) return v;
    return static_cast&lt;std::uint16_t&gt;(((v &gt;&gt; 0) &amp; UINT16_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 8) &amp; UINT16_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/windows-7ec41ff49437.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint16_t doLittleToHost16(std::uint16_t v) {
    if constexpr (std::endian::native == std::endian::little) return v;
    return static_cast&lt;std::uint16_t&gt;(((v &gt;&gt; 0) &amp; UINT16_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 8) &amp; UINT16_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td><td><a href="snippets/mac-7ec41ff49437.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint16_t doLittleToHost16(std::uint16_t v) {
    if constexpr (std::endian::native == std::endian::little) return v;
    return static_cast&lt;std::uint16_t&gt;(((v &gt;&gt; 0) &amp; UINT16_C(0xff)) &lt;&lt; 8 | ((v &gt;&gt; 8) &amp; UINT16_C(0xff)) &lt;&lt; 0);
}</code></pre></details></td></tr>
<tr id="ba2str"><td><a href="CALL_SITES.md#ba2str"><code>ba2str</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Canonical textual address independent of native byte order.</p></details></td><td><pre><code>std::string doFormatBluetooth(const std::array&lt;std::uint8_t, 6&gt;&amp; address);</code></pre><p>Address bytes use printed order; reverse BlueZ bdaddr_t byte order in its adapter. No kernel call.</p></td><td><a href="snippets/linux-df093032c525.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::string doFormatBluetooth(const std::array&lt;std::uint8_t, 6&gt;&amp; address) {
    static constexpr char digits[] = "0123456789ABCDEF";
    std::string out;
    for (std::size_t i = 0; i &lt; address.size(); ++i) {
        if (i) out += ':';
        out += digits[address[i] &gt;&gt; 4]; out += digits[address[i] &amp; 15];
    }
    return out;
}</code></pre></details></td><td><a href="snippets/windows-df093032c525.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::string doFormatBluetooth(const std::array&lt;std::uint8_t, 6&gt;&amp; address) {
    static constexpr char digits[] = "0123456789ABCDEF";
    std::string out;
    for (std::size_t i = 0; i &lt; address.size(); ++i) {
        if (i) out += ':';
        out += digits[address[i] &gt;&gt; 4]; out += digits[address[i] &amp; 15];
    }
    return out;
}</code></pre></details></td><td><a href="snippets/mac-df093032c525.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::string doFormatBluetooth(const std::array&lt;std::uint8_t, 6&gt;&amp; address) {
    static constexpr char digits[] = "0123456789ABCDEF";
    std::string out;
    for (std::size_t i = 0; i &lt; address.size(); ++i) {
        if (i) out += ':';
        out += digits[address[i] &gt;&gt; 4]; out += digits[address[i] &amp; 15];
    }
    return out;
}</code></pre></details></td></tr>
<tr id="str2ba"><td><a href="CALL_SITES.md#str2ba"><code>str2ba</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Validate all six bytes and preserve current accepted grammar.</p></details></td><td><pre><code>Result&lt;std::array&lt;std::uint8_t, 6&gt;&gt; doParseBluetooth(std::string_view text);</code></pre><p>Canonical six-byte grammar only. BlueZ may accept additional spellings; characterize current accepted inputs before claiming equivalent replacement.</p></td><td><a href="snippets/linux-4d0c936a4000.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::array&lt;std::uint8_t, 6&gt;&gt; doParseBluetooth(std::string_view text) {
    std::array&lt;std::uint8_t, 6&gt; out{};
    auto digit = [](char c) -&gt; int {
        if (c &gt;= '0' &amp;&amp; c &lt;= '9') return c - '0';
        if (c &gt;= 'A' &amp;&amp; c &lt;= 'F') return c - 'A' + 10;
        if (c &gt;= 'a' &amp;&amp; c &lt;= 'f') return c - 'a' + 10;
        return -1;
    };
    if (text.size() != 17) return posixFailure&lt;decltype(out)&gt;(EINVAL);
    for (std::size_t i = 0; i &lt; out.size(); ++i) {
        const int high = digit(text[3*i]), low = digit(text[3*i+1]);
        if (high &lt; 0 || low &lt; 0 || (i &lt; 5 &amp;&amp; text[3*i+2] != ':')) return posixFailure&lt;decltype(out)&gt;(EINVAL);
        out[i] = static_cast&lt;std::uint8_t&gt;((high &lt;&lt; 4) | low);
    }
    return ok(out);
}</code></pre></details></td><td><a href="snippets/windows-4d0c936a4000.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::array&lt;std::uint8_t, 6&gt;&gt; doParseBluetooth(std::string_view text) {
    std::array&lt;std::uint8_t, 6&gt; out{};
    auto digit = [](char c) -&gt; int {
        if (c &gt;= '0' &amp;&amp; c &lt;= '9') return c - '0';
        if (c &gt;= 'A' &amp;&amp; c &lt;= 'F') return c - 'A' + 10;
        if (c &gt;= 'a' &amp;&amp; c &lt;= 'f') return c - 'a' + 10;
        return -1;
    };
    if (text.size() != 17) return posixFailure&lt;decltype(out)&gt;(EINVAL);
    for (std::size_t i = 0; i &lt; out.size(); ++i) {
        const int high = digit(text[3*i]), low = digit(text[3*i+1]);
        if (high &lt; 0 || low &lt; 0 || (i &lt; 5 &amp;&amp; text[3*i+2] != ':')) return posixFailure&lt;decltype(out)&gt;(EINVAL);
        out[i] = static_cast&lt;std::uint8_t&gt;((high &lt;&lt; 4) | low);
    }
    return ok(out);
}</code></pre></details></td><td><a href="snippets/mac-4d0c936a4000.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;std::array&lt;std::uint8_t, 6&gt;&gt; doParseBluetooth(std::string_view text) {
    std::array&lt;std::uint8_t, 6&gt; out{};
    auto digit = [](char c) -&gt; int {
        if (c &gt;= '0' &amp;&amp; c &lt;= '9') return c - '0';
        if (c &gt;= 'A' &amp;&amp; c &lt;= 'F') return c - 'A' + 10;
        if (c &gt;= 'a' &amp;&amp; c &lt;= 'f') return c - 'a' + 10;
        return -1;
    };
    if (text.size() != 17) return posixFailure&lt;decltype(out)&gt;(EINVAL);
    for (std::size_t i = 0; i &lt; out.size(); ++i) {
        const int high = digit(text[3*i]), low = digit(text[3*i+1]);
        if (high &lt; 0 || low &lt; 0 || (i &lt; 5 &amp;&amp; text[3*i+2] != ':')) return posixFailure&lt;decltype(out)&gt;(EINVAL);
        out[i] = static_cast&lt;std::uint8_t&gt;((high &lt;&lt; 4) | low);
    }
    return ok(out);
}</code></pre></details></td></tr>
<tr id="fd_zero"><td><a href="CALL_SITES.md#fd_zero"><code>FD_ZERO</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, tool</p><p>Replace native fd_set storage; no kernel operation.</p></details></td><td><pre><code>void doFD_ZERO(std::vector&lt;PollItem&gt;&amp; items);</code></pre><p>Socket collection helper only. Returned masks still need semantic decoding; these helpers cannot make Windows select work with process pipes.</p></td><td><a href="snippets/linux-3f3f8768759d.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>void doFD_ZERO(std::vector&lt;PollItem&gt;&amp; items) {
    items.clear();
}</code></pre></details></td><td><a href="snippets/windows-3f3f8768759d.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>void doFD_ZERO(std::vector&lt;PollItem&gt;&amp; items) {
    items.clear();
}</code></pre></details></td><td><a href="snippets/mac-3f3f8768759d.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>void doFD_ZERO(std::vector&lt;PollItem&gt;&amp; items) {
    items.clear();
}</code></pre></details></td></tr>
<tr id="fd_set"><td><a href="CALL_SITES.md#fd_set"><code>FD_SET</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, tool</p><p>No maximum numeric descriptor arithmetic in project code.</p></details></td><td><pre><code>void doFD_SET(std::vector&lt;PollItem&gt;&amp; items, PollItem item);</code></pre><p>Socket collection helper only. Returned masks still need semantic decoding; these helpers cannot make Windows select work with process pipes.</p></td><td><a href="snippets/linux-c47cb88e1f6a.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>void doFD_SET(std::vector&lt;PollItem&gt;&amp; items, PollItem item) {
    auto found = std::find_if(items.begin(), items.end(), [&amp;](const auto&amp; v) { return v.socket.native == item.socket.native; });
    if (found == items.end()) items.push_back(item);
    else { found-&gt;read |= item.read; found-&gt;write |= item.write; found-&gt;urgent |= item.urgent; }
}</code></pre></details></td><td><a href="snippets/windows-c47cb88e1f6a.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>void doFD_SET(std::vector&lt;PollItem&gt;&amp; items, PollItem item) {
    auto found = std::find_if(items.begin(), items.end(), [&amp;](const auto&amp; v) { return v.socket.native == item.socket.native; });
    if (found == items.end()) items.push_back(item);
    else { found-&gt;read |= item.read; found-&gt;write |= item.write; found-&gt;urgent |= item.urgent; }
}</code></pre></details></td><td><a href="snippets/mac-c47cb88e1f6a.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>void doFD_SET(std::vector&lt;PollItem&gt;&amp; items, PollItem item) {
    auto found = std::find_if(items.begin(), items.end(), [&amp;](const auto&amp; v) { return v.socket.native == item.socket.native; });
    if (found == items.end()) items.push_back(item);
    else { found-&gt;read |= item.read; found-&gt;write |= item.write; found-&gt;urgent |= item.urgent; }
}</code></pre></details></td></tr>
<tr id="fd_clr"><td><a href="CALL_SITES.md#fd_clr"><code>FD_CLR</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Remove interest only; do not close resource.</p></details></td><td><pre><code>void doFD_CLR(std::vector&lt;PollItem&gt;&amp; items, Socket socket);</code></pre><p>Socket collection helper only. Returned masks still need semantic decoding; these helpers cannot make Windows select work with process pipes.</p></td><td><a href="snippets/linux-60b3ce01e8f6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>void doFD_CLR(std::vector&lt;PollItem&gt;&amp; items, Socket socket) {
    std::erase_if(items, [&amp;](const auto&amp; item) { return item.socket.native == socket.native; });
}</code></pre></details></td><td><a href="snippets/windows-60b3ce01e8f6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>void doFD_CLR(std::vector&lt;PollItem&gt;&amp; items, Socket socket) {
    std::erase_if(items, [&amp;](const auto&amp; item) { return item.socket.native == socket.native; });
}</code></pre></details></td><td><a href="snippets/mac-60b3ce01e8f6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>void doFD_CLR(std::vector&lt;PollItem&gt;&amp; items, Socket socket) {
    std::erase_if(items, [&amp;](const auto&amp; item) { return item.socket.native == socket.native; });
}</code></pre></details></td></tr>
<tr id="fd_isset"><td><a href="CALL_SITES.md#fd_isset"><code>FD_ISSET</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, tool</p><p>Inspect latest wait result; no OS call.</p></details></td><td><pre><code>bool doFD_ISSET(const PollItem&amp; item);</code></pre><p>Socket collection helper only. Returned masks still need semantic decoding; these helpers cannot make Windows select work with process pipes.</p></td><td><a href="snippets/linux-28f17b36fd1b.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>bool doFD_ISSET(const PollItem&amp; item) {
    return item.returned != 0;
}</code></pre></details></td><td><a href="snippets/windows-28f17b36fd1b.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>bool doFD_ISSET(const PollItem&amp; item) {
    return item.returned != 0;
}</code></pre></details></td><td><a href="snippets/mac-28f17b36fd1b.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>bool doFD_ISSET(const PollItem&amp; item) {
    return item.returned != 0;
}</code></pre></details></td></tr>
<tr id="cmsg_space"><td><a href="CALL_SITES.md#cmsg_space"><code>CMSG_SPACE</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Ancillary-buffer sizing becomes private to the transfer capsule.</p></details></td><td><pre><code>Status doSendHandle(Socket channel, const Address&amp; destination, File resource);</code></pre><p>Sender retains ownership. UNIX datagram address stays inside the operation. No documented Windows SCM_RIGHTS counterpart; WSASendMsg does not transfer handles. A DuplicateHandle/WSADuplicateSocket protocol needs recipient identity and transport changes.</p></td><td><a href="snippets/linux-2aac97df536d.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSendHandle(Socket channel, const Address&amp; destination, File resource) {
    char marker = 0;
    iovec payload{&amp;marker, 1};
    alignas(cmsghdr) std::array&lt;std::byte, CMSG_SPACE(sizeof(int))&gt; control{};
    msghdr message{};
    message.msg_name = const_cast&lt;sockaddr_storage*&gt;(&amp;destination.storage);
    message.msg_namelen = destination.length;
    message.msg_iov = &amp;payload; message.msg_iovlen = 1;
    message.msg_control = control.data(); message.msg_controllen = control.size();
    cmsghdr* header = CMSG_FIRSTHDR(&amp;message);
    header-&gt;cmsg_level = SOL_SOCKET; header-&gt;cmsg_type = SCM_RIGHTS;
    header-&gt;cmsg_len = CMSG_LEN(sizeof(int));
    std::memcpy(CMSG_DATA(header), &amp;resource.native, sizeof(int));
    const auto sent = ::sendmsg(channel.native, &amp;message, 0);
    if (sent &lt; 0) return posixFailure();
    return sent == 1 ? ok() : posixFailure(EIO);
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Sender retains ownership. UNIX datagram address stays inside the operation. No documented Windows SCM_RIGHTS counterpart; WSASendMsg does not transfer handles. A DuplicateHandle/WSADuplicateSocket protocol needs recipient identity and transport changes.</p><p>Native building blocks: WSA_CMSG_SPACE exists, but not an SCM_RIGHTS implementation</p></td><td><a href="snippets/mac-2aac97df536d.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Status doSendHandle(Socket channel, const Address&amp; destination, File resource) {
    char marker = 0;
    iovec payload{&amp;marker, 1};
    alignas(cmsghdr) std::array&lt;std::byte, CMSG_SPACE(sizeof(int))&gt; control{};
    msghdr message{};
    message.msg_name = const_cast&lt;sockaddr_storage*&gt;(&amp;destination.storage);
    message.msg_namelen = destination.length;
    message.msg_iov = &amp;payload; message.msg_iovlen = 1;
    message.msg_control = control.data(); message.msg_controllen = control.size();
    cmsghdr* header = CMSG_FIRSTHDR(&amp;message);
    header-&gt;cmsg_level = SOL_SOCKET; header-&gt;cmsg_type = SCM_RIGHTS;
    header-&gt;cmsg_len = CMSG_LEN(sizeof(int));
    std::memcpy(CMSG_DATA(header), &amp;resource.native, sizeof(int));
    const auto sent = ::sendmsg(channel.native, &amp;message, 0);
    if (sent &lt; 0) return posixFailure();
    return sent == 1 ? ok() : posixFailure(EIO);
}</code></pre></details></td></tr>
<tr id="cmsg_len"><td><a href="CALL_SITES.md#cmsg_len"><code>CMSG_LEN</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Native ancillary header lengths do not cross the boundary.</p></details></td><td><pre><code>Result&lt;File&gt; doReceiveHandle(Socket channel);</code></pre><p>Returns one new owned descriptor, closes received extras on invalid messages. Before production use: descriptor type detection and atomic close-on-exec policy must be completed and malformed/truncated-message cleanup validated. Windows transport remains unresolved; a stub is not an implementation.</p></td><td><a href="snippets/linux-c061d3f1c497.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doReceiveHandle(Socket channel) {
    char marker;
    iovec payload{&amp;marker, 1};
    alignas(cmsghdr) std::array&lt;std::byte, CMSG_SPACE(16 * sizeof(int))&gt; control{};
    msghdr message{};
    message.msg_iov = &amp;payload; message.msg_iovlen = 1;
    message.msg_control = control.data(); message.msg_controllen = control.size();
    const auto received = ::recvmsg(channel.native, &amp;message, 0);
    if (received &lt; 0) return posixFailure&lt;File&gt;();
    std::array&lt;int, 16&gt; fds{};
    std::size_t count = 0;
    bool malformed = false;
    for (auto* header = CMSG_FIRSTHDR(&amp;message); header; header = CMSG_NXTHDR(&amp;message, header)) {
        if (header-&gt;cmsg_level != SOL_SOCKET || header-&gt;cmsg_type != SCM_RIGHTS) continue;
        const auto* begin = reinterpret_cast&lt;const std::byte*&gt;(header);
        const auto available = static_cast&lt;std::size_t&gt;(control.data() + message.msg_controllen - begin);
        if (header-&gt;cmsg_len &lt; CMSG_LEN(0) || header-&gt;cmsg_len &gt; available) { malformed = true; break; }
        const auto bytes = header-&gt;cmsg_len - CMSG_LEN(0);
        if (bytes % sizeof(int)) malformed = true;
        for (std::size_t offset = 0; offset + sizeof(int) &lt;= bytes; offset += sizeof(int)) {
            int fd; std::memcpy(&amp;fd, reinterpret_cast&lt;const std::byte*&gt;(CMSG_DATA(header)) + offset, sizeof(fd));
            if (count &lt; fds.size()) fds[count++] = fd;
            else { ::close(fd); malformed = true; }
        }
    }
    if (received != 1 || malformed || (message.msg_flags &amp; (MSG_CTRUNC | MSG_TRUNC)) || count != 1) {
        for (std::size_t i = 0; i &lt; count; ++i) ::close(fds[i]);
        return posixFailure&lt;File&gt;(EBADMSG);
    }
    return ok(File{fds.front()});
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Returns one new owned descriptor, closes received extras on invalid messages. Before production use: descriptor type detection and atomic close-on-exec policy must be completed and malformed/truncated-message cleanup validated. Windows transport remains unresolved; a stub is not an implementation.</p><p>Native building blocks: WSA_CMSG_LEN exists, but not an SCM_RIGHTS implementation</p></td><td><a href="snippets/mac-c061d3f1c497.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doReceiveHandle(Socket channel) {
    char marker;
    iovec payload{&amp;marker, 1};
    alignas(cmsghdr) std::array&lt;std::byte, CMSG_SPACE(16 * sizeof(int))&gt; control{};
    msghdr message{};
    message.msg_iov = &amp;payload; message.msg_iovlen = 1;
    message.msg_control = control.data(); message.msg_controllen = control.size();
    const auto received = ::recvmsg(channel.native, &amp;message, 0);
    if (received &lt; 0) return posixFailure&lt;File&gt;();
    std::array&lt;int, 16&gt; fds{};
    std::size_t count = 0;
    bool malformed = false;
    for (auto* header = CMSG_FIRSTHDR(&amp;message); header; header = CMSG_NXTHDR(&amp;message, header)) {
        if (header-&gt;cmsg_level != SOL_SOCKET || header-&gt;cmsg_type != SCM_RIGHTS) continue;
        const auto* begin = reinterpret_cast&lt;const std::byte*&gt;(header);
        const auto available = static_cast&lt;std::size_t&gt;(control.data() + message.msg_controllen - begin);
        if (header-&gt;cmsg_len &lt; CMSG_LEN(0) || header-&gt;cmsg_len &gt; available) { malformed = true; break; }
        const auto bytes = header-&gt;cmsg_len - CMSG_LEN(0);
        if (bytes % sizeof(int)) malformed = true;
        for (std::size_t offset = 0; offset + sizeof(int) &lt;= bytes; offset += sizeof(int)) {
            int fd; std::memcpy(&amp;fd, reinterpret_cast&lt;const std::byte*&gt;(CMSG_DATA(header)) + offset, sizeof(fd));
            if (count &lt; fds.size()) fds[count++] = fd;
            else { ::close(fd); malformed = true; }
        }
    }
    if (received != 1 || malformed || (message.msg_flags &amp; (MSG_CTRUNC | MSG_TRUNC)) || count != 1) {
        for (std::size_t i = 0; i &lt; count; ++i) ::close(fds[i]);
        return posixFailure&lt;File&gt;(EBADMSG);
    }
    return ok(File{fds.front()});
}</code></pre></details></td></tr>
<tr id="cmsg_firsthdr"><td><a href="CALL_SITES.md#cmsg_firsthdr"><code>CMSG_FIRSTHDR</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>Capsule validates the expected resource-transfer record.</p></details></td><td><pre><code>Result&lt;File&gt; doReceiveHandle(Socket channel);</code></pre><p>Returns one new owned descriptor, closes received extras on invalid messages. Before production use: descriptor type detection and atomic close-on-exec policy must be completed and malformed/truncated-message cleanup validated. Windows transport remains unresolved; a stub is not an implementation.</p></td><td><a href="snippets/linux-c061d3f1c497.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doReceiveHandle(Socket channel) {
    char marker;
    iovec payload{&amp;marker, 1};
    alignas(cmsghdr) std::array&lt;std::byte, CMSG_SPACE(16 * sizeof(int))&gt; control{};
    msghdr message{};
    message.msg_iov = &amp;payload; message.msg_iovlen = 1;
    message.msg_control = control.data(); message.msg_controllen = control.size();
    const auto received = ::recvmsg(channel.native, &amp;message, 0);
    if (received &lt; 0) return posixFailure&lt;File&gt;();
    std::array&lt;int, 16&gt; fds{};
    std::size_t count = 0;
    bool malformed = false;
    for (auto* header = CMSG_FIRSTHDR(&amp;message); header; header = CMSG_NXTHDR(&amp;message, header)) {
        if (header-&gt;cmsg_level != SOL_SOCKET || header-&gt;cmsg_type != SCM_RIGHTS) continue;
        const auto* begin = reinterpret_cast&lt;const std::byte*&gt;(header);
        const auto available = static_cast&lt;std::size_t&gt;(control.data() + message.msg_controllen - begin);
        if (header-&gt;cmsg_len &lt; CMSG_LEN(0) || header-&gt;cmsg_len &gt; available) { malformed = true; break; }
        const auto bytes = header-&gt;cmsg_len - CMSG_LEN(0);
        if (bytes % sizeof(int)) malformed = true;
        for (std::size_t offset = 0; offset + sizeof(int) &lt;= bytes; offset += sizeof(int)) {
            int fd; std::memcpy(&amp;fd, reinterpret_cast&lt;const std::byte*&gt;(CMSG_DATA(header)) + offset, sizeof(fd));
            if (count &lt; fds.size()) fds[count++] = fd;
            else { ::close(fd); malformed = true; }
        }
    }
    if (received != 1 || malformed || (message.msg_flags &amp; (MSG_CTRUNC | MSG_TRUNC)) || count != 1) {
        for (std::size_t i = 0; i &lt; count; ++i) ::close(fds[i]);
        return posixFailure&lt;File&gt;(EBADMSG);
    }
    return ok(File{fds.front()});
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Returns one new owned descriptor, closes received extras on invalid messages. Before production use: descriptor type detection and atomic close-on-exec policy must be completed and malformed/truncated-message cleanup validated. Windows transport remains unresolved; a stub is not an implementation.</p><p>Native building blocks: WSA_CMSG_FIRSTHDR exists, but not an SCM_RIGHTS implementation</p></td><td><a href="snippets/mac-c061d3f1c497.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doReceiveHandle(Socket channel) {
    char marker;
    iovec payload{&amp;marker, 1};
    alignas(cmsghdr) std::array&lt;std::byte, CMSG_SPACE(16 * sizeof(int))&gt; control{};
    msghdr message{};
    message.msg_iov = &amp;payload; message.msg_iovlen = 1;
    message.msg_control = control.data(); message.msg_controllen = control.size();
    const auto received = ::recvmsg(channel.native, &amp;message, 0);
    if (received &lt; 0) return posixFailure&lt;File&gt;();
    std::array&lt;int, 16&gt; fds{};
    std::size_t count = 0;
    bool malformed = false;
    for (auto* header = CMSG_FIRSTHDR(&amp;message); header; header = CMSG_NXTHDR(&amp;message, header)) {
        if (header-&gt;cmsg_level != SOL_SOCKET || header-&gt;cmsg_type != SCM_RIGHTS) continue;
        const auto* begin = reinterpret_cast&lt;const std::byte*&gt;(header);
        const auto available = static_cast&lt;std::size_t&gt;(control.data() + message.msg_controllen - begin);
        if (header-&gt;cmsg_len &lt; CMSG_LEN(0) || header-&gt;cmsg_len &gt; available) { malformed = true; break; }
        const auto bytes = header-&gt;cmsg_len - CMSG_LEN(0);
        if (bytes % sizeof(int)) malformed = true;
        for (std::size_t offset = 0; offset + sizeof(int) &lt;= bytes; offset += sizeof(int)) {
            int fd; std::memcpy(&amp;fd, reinterpret_cast&lt;const std::byte*&gt;(CMSG_DATA(header)) + offset, sizeof(fd));
            if (count &lt; fds.size()) fds[count++] = fd;
            else { ::close(fd); malformed = true; }
        }
    }
    if (received != 1 || malformed || (message.msg_flags &amp; (MSG_CTRUNC | MSG_TRUNC)) || count != 1) {
        for (std::size_t i = 0; i &lt; count; ++i) ::close(fds[i]);
        return posixFailure&lt;File&gt;(EBADMSG);
    }
    return ok(File{fds.front()});
}</code></pre></details></td></tr>
<tr id="cmsg_data"><td><a href="CALL_SITES.md#cmsg_data"><code>CMSG_DATA</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework</p><p>No reinterpret_cast of native descriptor integers in project code.</p></details></td><td><pre><code>Result&lt;File&gt; doReceiveHandle(Socket channel);</code></pre><p>Returns one new owned descriptor, closes received extras on invalid messages. Before production use: descriptor type detection and atomic close-on-exec policy must be completed and malformed/truncated-message cleanup validated. Windows transport remains unresolved; a stub is not an implementation.</p></td><td><a href="snippets/linux-c061d3f1c497.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doReceiveHandle(Socket channel) {
    char marker;
    iovec payload{&amp;marker, 1};
    alignas(cmsghdr) std::array&lt;std::byte, CMSG_SPACE(16 * sizeof(int))&gt; control{};
    msghdr message{};
    message.msg_iov = &amp;payload; message.msg_iovlen = 1;
    message.msg_control = control.data(); message.msg_controllen = control.size();
    const auto received = ::recvmsg(channel.native, &amp;message, 0);
    if (received &lt; 0) return posixFailure&lt;File&gt;();
    std::array&lt;int, 16&gt; fds{};
    std::size_t count = 0;
    bool malformed = false;
    for (auto* header = CMSG_FIRSTHDR(&amp;message); header; header = CMSG_NXTHDR(&amp;message, header)) {
        if (header-&gt;cmsg_level != SOL_SOCKET || header-&gt;cmsg_type != SCM_RIGHTS) continue;
        const auto* begin = reinterpret_cast&lt;const std::byte*&gt;(header);
        const auto available = static_cast&lt;std::size_t&gt;(control.data() + message.msg_controllen - begin);
        if (header-&gt;cmsg_len &lt; CMSG_LEN(0) || header-&gt;cmsg_len &gt; available) { malformed = true; break; }
        const auto bytes = header-&gt;cmsg_len - CMSG_LEN(0);
        if (bytes % sizeof(int)) malformed = true;
        for (std::size_t offset = 0; offset + sizeof(int) &lt;= bytes; offset += sizeof(int)) {
            int fd; std::memcpy(&amp;fd, reinterpret_cast&lt;const std::byte*&gt;(CMSG_DATA(header)) + offset, sizeof(fd));
            if (count &lt; fds.size()) fds[count++] = fd;
            else { ::close(fd); malformed = true; }
        }
    }
    if (received != 1 || malformed || (message.msg_flags &amp; (MSG_CTRUNC | MSG_TRUNC)) || count != 1) {
        for (std::size_t i = 0; i &lt; count; ++i) ::close(fds[i]);
        return posixFailure&lt;File&gt;(EBADMSG);
    }
    return ok(File{fds.front()});
}</code></pre></details></td><td><strong>NO COMPLETE BODY</strong><p>Returns one new owned descriptor, closes received extras on invalid messages. Before production use: descriptor type detection and atomic close-on-exec policy must be completed and malformed/truncated-message cleanup validated. Windows transport remains unresolved; a stub is not an implementation.</p><p>Native building blocks: WSA_CMSG_DATA exists, but not an SCM_RIGHTS implementation</p></td><td><a href="snippets/mac-c061d3f1c497.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>Result&lt;File&gt; doReceiveHandle(Socket channel) {
    char marker;
    iovec payload{&amp;marker, 1};
    alignas(cmsghdr) std::array&lt;std::byte, CMSG_SPACE(16 * sizeof(int))&gt; control{};
    msghdr message{};
    message.msg_iov = &amp;payload; message.msg_iovlen = 1;
    message.msg_control = control.data(); message.msg_controllen = control.size();
    const auto received = ::recvmsg(channel.native, &amp;message, 0);
    if (received &lt; 0) return posixFailure&lt;File&gt;();
    std::array&lt;int, 16&gt; fds{};
    std::size_t count = 0;
    bool malformed = false;
    for (auto* header = CMSG_FIRSTHDR(&amp;message); header; header = CMSG_NXTHDR(&amp;message, header)) {
        if (header-&gt;cmsg_level != SOL_SOCKET || header-&gt;cmsg_type != SCM_RIGHTS) continue;
        const auto* begin = reinterpret_cast&lt;const std::byte*&gt;(header);
        const auto available = static_cast&lt;std::size_t&gt;(control.data() + message.msg_controllen - begin);
        if (header-&gt;cmsg_len &lt; CMSG_LEN(0) || header-&gt;cmsg_len &gt; available) { malformed = true; break; }
        const auto bytes = header-&gt;cmsg_len - CMSG_LEN(0);
        if (bytes % sizeof(int)) malformed = true;
        for (std::size_t offset = 0; offset + sizeof(int) &lt;= bytes; offset += sizeof(int)) {
            int fd; std::memcpy(&amp;fd, reinterpret_cast&lt;const std::byte*&gt;(CMSG_DATA(header)) + offset, sizeof(fd));
            if (count &lt; fds.size()) fds[count++] = fd;
            else { ::close(fd); malformed = true; }
        }
    }
    if (received != 1 || malformed || (message.msg_flags &amp; (MSG_CTRUNC | MSG_TRUNC)) || count != 1) {
        for (std::size_t i = 0; i &lt; count; ++i) ::close(fds[i]);
        return posixFailure&lt;File&gt;(EBADMSG);
    }
    return ok(File{fds.front()});
}</code></pre></details></td></tr>
<tr id="s_isreg"><td><a href="CALL_SITES.md#s_isreg"><code>S_ISREG</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, test</p><p>Pure query of normalized metadata.</p></details></td><td><pre><code>bool doS_ISREG(const FileInfo&amp; info);</code></pre><p>Pure query of normalized capsule output. Producer must establish this information; Windows exception codes are not signal numbers.</p></td><td><a href="snippets/linux-a84bd1f44a1e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>bool doS_ISREG(const FileInfo&amp; info) {
    return info.kind == FileInfo::Kind::regular;
}</code></pre></details></td><td><a href="snippets/windows-a84bd1f44a1e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>bool doS_ISREG(const FileInfo&amp; info) {
    return info.kind == FileInfo::Kind::regular;
}</code></pre></details></td><td><a href="snippets/mac-a84bd1f44a1e.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>bool doS_ISREG(const FileInfo&amp; info) {
    return info.kind == FileInfo::Kind::regular;
}</code></pre></details></td></tr>
<tr id="s_issock"><td><a href="CALL_SITES.md#s_issock"><code>S_ISSOCK</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; framework, test</p><p>UNIX cleanup must identify the exact socket node it owns; do not treat every reparse point as a socket.</p></details></td><td><pre><code>bool doS_ISSOCK(const FileInfo&amp; info);</code></pre><p>Pure query of normalized capsule output. Producer must establish this information; Windows exception codes are not signal numbers.</p></td><td><a href="snippets/linux-24fa29f8e5fd.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>bool doS_ISSOCK(const FileInfo&amp; info) {
    return info.kind == FileInfo::Kind::socket;
}</code></pre></details></td><td><a href="snippets/windows-24fa29f8e5fd.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>bool doS_ISSOCK(const FileInfo&amp; info) {
    return info.kind == FileInfo::Kind::socket;
}</code></pre></details></td><td><a href="snippets/mac-24fa29f8e5fd.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>bool doS_ISSOCK(const FileInfo&amp; info) {
    return info.kind == FileInfo::Kind::socket;
}</code></pre></details></td></tr>
<tr id="wifexited"><td><a href="CALL_SITES.md#wifexited"><code>WIFEXITED</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test, tool</p><p>Decode normalized completion, not a POSIX bit layout.</p></details></td><td><pre><code>bool doWIFEXITED(const ExitStatus&amp; s);</code></pre><p>Pure query of normalized capsule output. Producer must establish this information; Windows exception codes are not signal numbers.</p></td><td><a href="snippets/linux-edc5145420f6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>bool doWIFEXITED(const ExitStatus&amp; s) {
    return !s.signaled;
}</code></pre></details></td><td><a href="snippets/windows-edc5145420f6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>bool doWIFEXITED(const ExitStatus&amp; s) {
    return !s.signaled;
}</code></pre></details></td><td><a href="snippets/mac-edc5145420f6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>bool doWIFEXITED(const ExitStatus&amp; s) {
    return !s.signaled;
}</code></pre></details></td></tr>
<tr id="wexitstatus"><td><a href="CALL_SITES.md#wexitstatus"><code>WEXITSTATUS</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; test, tool</p><p>Valid only for exited state; Windows exit code is not truncated to eight bits silently.</p></details></td><td><pre><code>std::uint32_t doWEXITSTATUS(const ExitStatus&amp; s);</code></pre><p>Pure query of normalized capsule output. Producer must establish this information; Windows exception codes are not signal numbers.</p></td><td><a href="snippets/linux-fdfea4650938.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint32_t doWEXITSTATUS(const ExitStatus&amp; s) {
    return s.code;
}</code></pre></details></td><td><a href="snippets/windows-fdfea4650938.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint32_t doWEXITSTATUS(const ExitStatus&amp; s) {
    return s.code;
}</code></pre></details></td><td><a href="snippets/mac-fdfea4650938.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::uint32_t doWEXITSTATUS(const ExitStatus&amp; s) {
    return s.code;
}</code></pre></details></td></tr>
<tr id="wifsignaled"><td><a href="CALL_SITES.md#wifsignaled"><code>WIFSIGNALED</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>Return true only when proven; do not invent a signal from a Windows exception/status value.</p></details></td><td><pre><code>bool doWIFSIGNALED(const ExitStatus&amp; s);</code></pre><p>Pure query of normalized capsule output. Producer must establish this information; Windows exception codes are not signal numbers.</p></td><td><a href="snippets/linux-42886e0bc9ec.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>bool doWIFSIGNALED(const ExitStatus&amp; s) {
    return s.signaled;
}</code></pre></details></td><td><a href="snippets/windows-42886e0bc9ec.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>bool doWIFSIGNALED(const ExitStatus&amp; s) {
    return s.signaled;
}</code></pre></details></td><td><a href="snippets/mac-42886e0bc9ec.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>bool doWIFSIGNALED(const ExitStatus&amp; s) {
    return s.signaled;
}</code></pre></details></td></tr>
<tr id="wtermsig"><td><a href="CALL_SITES.md#wtermsig"><code>WTERMSIG</code></a><p>Helper, not syscall</p><details><summary>Extraction / source contract</summary><p>No system wrapper; tool</p><p>Absent when OS cannot establish the signal reason.</p></details></td><td><pre><code>std::optional&lt;int&gt; doWTERMSIG(const ExitStatus&amp; s);</code></pre><p>Pure query of normalized capsule output. Producer must establish this information; Windows exception codes are not signal numbers.</p></td><td><a href="snippets/linux-02bf6fea6ef6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::optional&lt;int&gt; doWTERMSIG(const ExitStatus&amp; s) {
    return s.signaled ? std::optional&lt;int&gt;{s.signal} : std::nullopt;
}</code></pre></details></td><td><a href="snippets/windows-02bf6fea6ef6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::optional&lt;int&gt; doWTERMSIG(const ExitStatus&amp; s) {
    return s.signaled ? std::optional&lt;int&gt;{s.signal} : std::nullopt;
}</code></pre></details></td><td><a href="snippets/mac-02bf6fea6ef6.cpp.txt">Copyable C++</a><details><summary>Function body</summary><pre><code>std::optional&lt;int&gt; doWTERMSIG(const ExitStatus&amp; s) {
    return s.signaled ? std::optional&lt;int&gt;{s.signal} : std::nullopt;
}</code></pre></details></td></tr>
</tbody>
</table>
