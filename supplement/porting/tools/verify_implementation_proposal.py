#!/usr/bin/env python3
"""Static artifact and host syntax checks; no executables or framework tests."""
import collections, hashlib, json, pathlib, re, shutil, subprocess, tempfile
from html.parser import HTMLParser
from api_catalog import ROWS
from implementation_catalog import ENTRIES, ROOT, OUT
checks=[]
def check(ok,label):checks.append(dict(check=label,passed=bool(ok)))
records=json.loads((OUT/'implementation-proposal.json').read_text())
check(collections.Counter(r['name'] for r in records)==collections.Counter(r['name'] for r in ROWS),'All 163 catalog entries occur exactly once')
class Table(HTMLParser):
    def __init__(self):super().__init__();self.cells=[];self.current=None;self.links=[]
    def handle_starttag(self,tag,attrs):
        attrs=dict(attrs)
        if tag=='tr':self.current=0
        if tag=='td' and self.current is not None:self.current+=1
        if tag=='a' and 'href' in attrs:self.links.append(attrs['href'])
    def handle_endtag(self,tag):
        if tag=='tr':
            if self.current:self.cells.append(self.current)
            self.current=None
for name in ['IMPLEMENTATION_PROPOSAL.md','IMPLEMENTATION_PROPOSAL.html']:
    table=Table();table.feed((OUT/name).read_text())
    check(len(table.cells)==163 and set(table.cells)=={5},name+' has exactly five cells for every source row')
    check(all((OUT/href.split('#')[0]).exists() for href in table.links if not href.startswith('http')),name+' local HTML links resolve')
for row in records:
    for os,platform in row['platforms'].items():
        if 'file' in platform:
            body=(OUT/platform['file']).read_text()
            check(body.rstrip()==ENTRIES[row['name']][os].rstrip(),row['name']+'/'+os+' copyable body matches catalog')
check(all('\x00' not in p.read_text() for p in (OUT/'snippets').glob('*.txt')),'No accidental NUL characters in generated C++')
compiler=shutil.which('g++');syntax=[]
if compiler:
    with tempfile.TemporaryDirectory(prefix='snodec-proposal-') as temp:
        temp=pathlib.Path(temp)
        support='#include "'+str(OUT/'snippets/common.hpp.txt')+'"\n'
        source=support+'#include <grp.h>\n#include <pwd.h>\n#include "utils/system/signal.h"\nnamespace core::system {\n'
        seen=set();signatures=set();selected=[]
        for name,e in ENTRIES.items():
            body=e['linux']
            if 'namespace ' in body or 'doStartDaemon' in body or body in seen or e['api'] in signatures:continue
            seen.add(body);signatures.add(e['api']);selected.append(name)
            source+='\n// '+name+'\n'+body+'\n'
        source+='}\n'
        cases=[('linux-primitives',source),
               ('linux-config',support+'#include "'+str(OUT/'snippets/config-types.hpp.txt')+'"\n#include <grp.h>\n#include <pwd.h>\n'+(OUT/'snippets/config-linux.cpp.txt').read_text()),
               ('linux-random-syscall-alternative',support+'namespace core::system {\n'+ENTRIES['syscall:SYS_getrandom']['linux']+'\n}'),
               ('linux-process-runner',(OUT/'snippets/process-posix.cpp.txt').read_text()),
               ('linux-daemon',support+'#include "utils/Daemon.h"\n#include <csignal>\n#include <fstream>\n#include <grp.h>\n#include <pwd.h>\n'+ENTRIES['fork']['linux'])]
        for label,code in cases:
            path=temp/(label+'.cpp');path.write_text(code)
            command=[compiler,'-std=c++20','-fsyntax-only','-I',str(ROOT/'src'),'-I',str(ROOT/'src/tools/snodec-control/src'),str(path)]
            result=subprocess.run(command,capture_output=True,text=True)
            syntax.append(dict(case=label,passed=result.returncode==0,diagnostic=result.stderr,sha256=hashlib.sha256(code.encode()).hexdigest()))
            check(result.returncode==0,label+' host syntax-only compilation')
else:syntax.append(dict(status='not run; g++ unavailable'))
check(not subprocess.check_output(['git','diff','--name-only','HEAD','--','src','tests','examples','CMakeLists.txt'],cwd=ROOT,text=True).strip(),'Production/test/example/build sources unchanged')
report=dict(passed=all(x['passed'] for x in checks),checks=checks,syntax=syntax,platform_validation='Linux host syntax only; no Windows/macOS compilation, no linking, no runtime/QEMU tests',missing_platform_bodies=sum(not e[os] for e in ENTRIES.values() for os in ['linux','windows','mac']))
(OUT/'implementation-verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(passed=report['passed'],checks=len(checks),failed=[x for x in checks if not x['passed']],syntax=syntax),indent=2))
if not report['passed']:raise SystemExit(1)
