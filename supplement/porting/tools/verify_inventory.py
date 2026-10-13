#!/usr/bin/env python3
"""Check documentation evidence consistency, not framework runtime behavior."""
import collections, gzip, hashlib, json, pathlib, re, subprocess
from scan_calls import ROOT, OUT, clean
from api_catalog import ROWS
checks=[]
def check(condition,message):
    checks.append(dict(check=message,passed=bool(condition)))
coverage=json.loads((OUT/'source-coverage.json').read_text())
tracked=subprocess.check_output(['git','ls-files','-z'],cwd=ROOT).decode().split('\0')[:-1]
check(set(tracked)=={f['path'] for f in coverage},'Every tracked file has a coverage record')
changed=[]
for f in coverage:
    if 'sha256' in f and hashlib.sha256((ROOT/f['path']).read_bytes()).hexdigest()!=f['sha256']:changed.append(f['path'])
check(not changed,'All audited tracked file bytes still match snapshot')
rev=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
p=json.loads((OUT/'provenance.json').read_text())
check(rev==p['revision'],'Snapshot revision still matches HEAD')
occ=json.loads((OUT/'occurrences.json').read_text());lib=json.loads((OUT/'library-occurrences.json').read_text())
rows=json.loads((OUT/'inventory.json').read_text())
check(len(rows)==len({r['name'] for r in rows})==len(ROWS),'One machine-readable row per reviewed catalog operation')
check(set(occ)=={r['name'] for r in rows} and all(occ.values()),'Every catalog row has source evidence and every selected native entry has a row')
main=(OUT/'SYSCALL_INVENTORY.md').read_text(); appendix=(OUT/'LIBRARY_CALLS.md').read_text()
main_names=re.findall(r'^\| \[`([^`]+)`\]',main,re.M)
lib_names=re.findall(r'^\| \[`([^`]+)`\]',appendix,re.M)
check(collections.Counter(main_names)==collections.Counter(r['name'] for r in rows),'Main table contains each catalog row exactly once')
check(collections.Counter(lib_names)==collections.Counter(lib.keys()),'Library table contains each appendix entry exactly once')
check(all(all(r.get(k) for k in ['linux','mac','windows','api','note','extracted','extractable']) for r in rows),'Every native row has extraction state, three platform columns and a capsule contract')
missing_lines=[]
for name,items in {**occ,**lib}.items():
    for o in items:
        lines=(ROOT/o['path']).read_text().splitlines()
        if not (0<o['line']<=len(lines)) or lines[o['line']-1].strip()!=o['text']:missing_lines.append((name,o['path'],o['line']))
check(not missing_lines,'Every selected reference matches its recorded source line')
raw=json.loads(gzip.decompress((OUT/'call-candidates.json.gz').read_bytes()))
dispatch=[o for o in raw if o['base']=='syscall' and not o['member']]
check(len(dispatch)==2 and {re.search(r'SYS_\w+',o['text'])[0] for o in dispatch}=={'SYS_getrandom','SYS_pidfd_open'},'Both direct raw syscall targets are individually accounted for')
callbacks=[(n,o['path']) for n,items in {**occ,**lib}.items() for o in items if o['role']=='callback reference']
check(sum(n=='freeaddrinfo' for n,_ in callbacks)==2 and any(n=='GENERAL_NAME_free' for n,_ in callbacks),'Resolver deleters and OpenSSL callback references are included')
check(not any(o.get('template_member') for items in occ.values() for o in items),'Template-qualified member definitions are excluded from native calls')
broken=[]
for path in OUT.glob('*.md'):
    markdown=re.sub(r'<pre>.*?</pre>', '', path.read_text(), flags=re.S)
    markdown=re.sub(r'`[^`]*`', '', markdown)
    for target in re.findall(r'\]\(([^)]+)\)',markdown):
        if target.startswith(('http:','https:','mailto:')):continue
        file,sep,fragment=target.partition('#')
        dest=(path.parent/file).resolve() if file else path
        if not dest.exists():broken.append((str(path),target));continue
        if fragment and dest.suffix=='.md':
            headings=re.findall(r'^#+ (.+)$',dest.read_text(),re.M)
            anchors={re.sub(r'[^a-z0-9_-]+','',h.lower().replace(' ','-')) for h in headings}
            if fragment not in anchors:broken.append((str(path),target))
check(not broken,'All local documentation links and Markdown table anchors resolve')
check(not subprocess.check_output(['git','diff','--name-only','HEAD','--','src','tests','examples','CMakeLists.txt'],cwd=ROOT,text=True).strip(),'No production, test, example or build source changed')
report=dict(revision=rev,checks=checks,passed=all(c['passed'] for c in checks),changed_source_files=changed,missing_source_lines=missing_lines,broken_links=broken,runtime_tests='not run; documentation/static source audit only')
(OUT/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
if not report['passed']:raise SystemExit(1)
