#!/usr/bin/env python3
"""Render five-column proposal with literal code bodies and copyable text files."""
import hashlib, html, json, re
from api_catalog import ROWS
from implementation_catalog import ENTRIES, BLOCKERS, OUT
assert set(ENTRIES) == {r['name'] for r in ROWS}
existing=json.loads((OUT/'inventory.json').read_text())
evidence={r['name']:r for r in existing}
def slug(name):return re.sub('[^a-z0-9_-]','',name.lower())
def escape(value):return html.escape(value,quote=False)
head='''# System-call capsule implementation proposal

This is the requested five-column proposal: syscall/source operation, proposed callable API, Linux body, Windows body, macOS body. Bodies are included in the cells and also saved as plain C++ text for copying. [Open the wide HTML table](IMPLEMENTATION_PROPOSAL.html) for readable side-by-side code.

**First draft, not a completed three-platform port.** Every one of the 163 inventoried native/runtime/helper entries appears once. Some bodies implement only a stated subset; missing bodies are explicit implementation gaps, not successful unsupported stubs. An entry in this table does not imply that all its source uses can already migrate. No backend has been validated in QEMU. These snippets are documentation, outside the production build.

Use [common declarations](snippets/common.hpp.txt), [Windows text conversion](snippets/windows-text.hpp.txt) where needed, and the per-operation includes listed with the row. Select one platform body for each API; repeated bodies represent consolidation, not functions to paste several times. Code is C++20. Put bare function bodies inside `namespace core::system`; full files already containing a namespace retain theirs. Include the support declarations once. Native SDK declarations inside backend signatures are explicitly marked as unfinished portable boundaries.

The architectural invariant is one existing owner for resource representation, lifetime and OS behavior. Small wrappers stay under the existing system owners; configuration, child execution, descriptor transfer, terminal state and daemon startup are operation capsules. Do not create a parallel descriptor registry or wrap every line of a sequence independently. No production implementation is authorized by adding these documentation files.

`Result<T>` captures native error domain/code immediately. Callers must migrate their errno branches to the captured error; partial I/O remains observable. Signed `Count` avoids disguising failure as an unsigned byte count. The support declarations are concrete proposal types, not types that already exist in SNode.C. Paths are `std::filesystem::path`; use an explicit UTF-8 conversion at string boundaries. Native flag values and native address construction still appearing in some rows are adoption gaps, not a finished portable API.

The source evidence and extraction status remain in [the complete inventory](SYSCALL_INVENTORY.md), [call sites](CALL_SITES.md), [library calls](LIBRARY_CALLS.md), and [platform dependencies](PLATFORM_DEPENDENCIES.md). Earlier API-only declarations are superseded by this implementation draft where signatures differ. [Implementation gaps](IMPLEMENTATION_GAPS.md) lists all empty cells and preserves the per-row restrictions. Each source call links to its actual occurrences.

The longer Config.cpp capsule is also directly available as [types](snippets/config-types.hpp.txt), [Linux](snippets/config-linux.cpp.txt), [Windows](snippets/config-windows.cpp.txt), [macOS](snippets/config-mac.cpp.txt), and [replacement caller](snippets/config-caller.cpp.txt). It replaces account lookup, directory policy, directory creation, permissions and group ownership together. Windows uses SIDs and creation-time ACLs; its LocalSystem/known-folder policy is proposed behavior requiring review, not an exact UID/GID emulation. The POSIX variant retains existing directory locations on macOS intentionally; adopting Apple-specific locations would be a separate policy change.

Native API references: [Windows ReadFile](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile), [file information](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getfileinformationbyhandleex), [known folders](https://learn.microsoft.com/en-us/windows/win32/shell/known-folders), [Winsock](https://learn.microsoft.com/en-us/windows/win32/api/winsock2/), [CreateProcessW](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw), [NtCreateFile](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntcreatefile), [Darwin native entries](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/kern/syscalls.master), [Apple kevent](https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/kevent.2.html). These establish building blocks, not equivalence or successful execution of these bodies.

'''
columns=['syscall on POSIX/Linux','proposed API','implementation Linux','implementation Windows','implementation macOS']
parts=[head]
htmlrows=[]
records=[]
gaps=[]
for group in dict.fromkeys(r['group'] for r in ROWS):
    parts.append('\n## '+group+'\n\n<table>\n<thead><tr>'+''.join('<th>'+c+'</th>' for c in columns)+'</tr></thead>\n<tbody>\n')
    htmlrows.append('<tr class="group"><th colspan="5">'+escape(group)+'</th></tr>')
    for r in ROWS:
        if r['group'] != group: continue
        name=r['name']; e=ENTRIES[name]; record=dict(name=name,api=e['api'],note=e['note'],platforms={})
        first=f'<a href="CALL_SITES.md#{slug(name)}"><code>{escape(name)}</code></a><p>{escape(r["kind"])}</p>'
        first+=f'<details><summary>Extraction / source contract</summary><p>{escape(evidence[name]["extracted"])}</p><p>{escape(r["note"])}</p></details>'
        api='<pre><code>'+escape(e['api'])+'</code></pre><p>'+escape(e['note'])+'</p>'
        cells=[first,api]
        for os in ['linux','windows','mac']:
            body=e[os]
            if body:
                # One file per distinct body/platform; repeated operation rows link to the same content.
                digest=hashlib.sha256(body.encode()).hexdigest()[:12]
                file='snippets/'+os+'-'+digest+'.cpp.txt'
                (OUT/file).write_text(body+'\n')
                cells.append(f'<a href="{file}">Copyable C++</a><details><summary>Function body</summary><pre><code>'+escape(body)+'</code></pre></details>')
                record['platforms'][os]=dict(status='draft body; restrictions apply',file=file)
            else:
                reason=BLOCKERS.get(name,e['note'])
                native=r[os]
                cells.append('<strong>NO COMPLETE BODY</strong><p>'+escape(reason)+'</p><p>Native building blocks: '+escape(native)+'</p>')
                record['platforms'][os]=dict(status='unresolved',reason=reason,native=native)
                gaps.append((name,os,reason,native))
        row='<tr id="'+slug(name)+'">'+''.join('<td>'+c+'</td>' for c in cells)+'</tr>\n'
        parts.append(row);htmlrows.append(row);records.append(record)
    parts.append('</tbody>\n</table>\n')
(OUT/'IMPLEMENTATION_PROPOSAL.md').write_text(''.join(parts))
page='''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>SNode.C capsule implementation proposal</title>
<style>body{font:15px system-ui;margin:20px;color:#20242b}table{border-collapse:collapse;width:100%;table-layout:fixed}th,td{border:1px solid #bbb;padding:10px;vertical-align:top}th{background:#eaf0f6;text-align:left}td:first-child,th:first-child{width:10%}td:nth-child(2),th:nth-child(2){width:18%}pre{white-space:pre-wrap;overflow-wrap:anywhere;font:12px/1.5 ui-monospace,monospace;background:#f6f8fa;padding:8px}p{overflow-wrap:anywhere}.group th{background:#d6e4ef}summary{cursor:pointer}strong{color:#a12c25}a{color:#125ca1}</style>
<h1>SNode.C: capsule implementation proposal</h1><p>Exactly five columns. Expand Function body to select/copy code, or open Copyable C++. Repeated bodies are one consolidated operation. <strong>Draft with explicit gaps; not a validated or complete port.</strong></p>
<p>Read <a href="IMPLEMENTATION_PROPOSAL.md">scope and dependencies</a>, <a href="snippets/common.hpp.txt">common declarations</a>, <a href="snippets/windows-text.hpp.txt">Windows text conversions</a>, and <a href="IMPLEMENTATION_GAPS.md">remaining gaps</a> before copying.</p>
<table><thead><tr>'''+''.join('<th>'+c+'</th>' for c in columns)+'</tr></thead><tbody>'+''.join(htmlrows)+'</tbody></table></html>'
(OUT/'IMPLEMENTATION_PROPOSAL.html').write_text(page)
(OUT/'implementation-proposal.json').write_text(json.dumps(records,indent=2)+'\n')
gaptext=['# Implementation gaps\n\nThis is a first implementation proposal, not a complete copy/paste port. The source inventory has 163 entries; ',str(len(gaps)),' platform cells have no complete body. Bodies that exist can still have narrower scope than the original call. This list distinguishes absent implementations from draft restrictions; neither category is verified portability.\n\n## Missing platform bodies\n\n| Call | Platform | What is missing | Native building blocks |\n| --- | --- | --- | --- |\n']
for name,os,reason,native in gaps:
    gaptext.append('| `'+name+'` | '+os+' | '+reason.replace('|','\\|')+' | '+native.replace('|','\\|')+' |\n')
gaptext.append('\n## Restrictions on supplied bodies\n\nEvery nonempty restriction is retained below so a narrow body cannot be mistaken for a complete source-use replacement. Some are ordinary preconditions; others are substantive unfinished work or proposed behavior changes.\n\n| Call | Restriction / follow-up |\n| --- | --- |\n')
for name,e in ENTRIES.items():
    if e['note']:gaptext.append('| `'+name+'` | '+e['note'].replace('|','\\|')+' |\n')
(OUT/'IMPLEMENTATION_GAPS.md').write_text(''.join(gaptext))
# Remove only stale files whose names and contents prove generator ownership.
used={p['file'] for r in records for p in r['platforms'].values() if 'file' in p}
for path in (OUT/'snippets').glob('*.cpp.txt'):
    match=re.fullmatch(r'(linux|windows|mac)-([0-9a-f]{12})\.cpp\.txt',path.name)
    if match and 'snippets/'+path.name not in used:
        if hashlib.sha256(path.read_text().rstrip().encode()).hexdigest()[:12] == match[2]:path.unlink()
print(json.dumps(dict(rows=len(records),missing_platform_bodies=len(gaps),files=len(list((OUT/'snippets').glob('*'))))))
