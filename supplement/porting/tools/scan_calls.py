#!/usr/bin/env python3
"""Lexical, all-branches source evidence collector; not a C++ semantic parser."""
import bisect, collections, gzip, hashlib, json, pathlib, re, subprocess
ROOT = pathlib.Path(__file__).resolve().parents[3]
OUT = ROOT / 'supplement' / 'porting'
# Keep newlines and offsets. Raw strings precede ordinary strings in this lexer.
LEX = re.compile(r'R"([^\s()\\]{0,16})\(.*?\)\1"|//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)
CALL = re.compile(r'(?<![\w])(?P<name>(?:::)?[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*\(')
EXT = {'.c','.h','.cpp','.hpp','.cc','.cxx','.hxx','.hh','.ipp','.tpp','.m','.mm'}
def clean(text):
    return LEX.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m.group()), text)
def scan():
    files=subprocess.check_output(['git','ls-files','-z'],cwd=ROOT).decode().split('\0')[:-1]
    occurrences=[]; coverage=[]; includes=[]
    for name in files:
        p=ROOT/name
        if not p.is_file():
            coverage.append({'path':name,'scope':'missing-or-submodule'});continue
        raw=p.read_bytes(); source=raw.decode('utf8',errors='replace')
        cxx=p.suffix in EXT or (p.suffix=='.in' and any(x in p.name for x in ('.h.','.cpp.','.c.')))
        coverage.append({'path':name,'scope':'C/C++' if cxx else 'build/script/docs/other','sha256':hashlib.sha256(raw).hexdigest()})
        if not cxx: continue
        stripped=clean(source); starts=[0]+[m.end() for m in re.finditer('\n',source)]
        lines=source.splitlines();guards=[];line_guards={}
        for ln,line in enumerate(lines,1):
            directive=re.match(r'\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)',line)
            if directive:
                op,expr=directive.groups()
                if op in ('if','ifdef','ifndef'):guards.append(op+' '+expr.strip())
                elif op=='endif' and guards:guards.pop()
                elif op in ('elif','else') and guards:guards[-1]+=' / '+op+' '+expr.strip()
            line_guards[ln]=' > '.join(guards)
            inc=re.match(r'\s*#\s*include\s*[<"]([^">]+)[">]',line)
            if inc:includes.append({'path':name,'line':ln,'header':inc[1],'guard':line_guards[ln]})
        for m in CALL.finditer(stripped):
            ln=bisect.bisect_right(starts,m.start());prefix=stripped[max(0,m.start()-4):m.start()].rstrip()
            occurrences.append({'spelling':m['name'], 'offset':m.start(), 'name':m['name'].lstrip(':'),'base':m['name'].split('::')[-1], 'path':name,'line':ln,'member':prefix.endswith('.') or prefix.endswith('->'), 'template_member':prefix.endswith('>') and m['name'].startswith('::'),'text':lines[ln-1].strip(),'guard':line_guards[ln]})
    (OUT/'source-coverage.json').write_text(json.dumps(coverage,indent=2)+'\n')
    (OUT/'call-candidates.json.gz').write_bytes(gzip.compress((json.dumps(occurrences, separators=(',', ':'))+'\n').encode(),mtime=0))
    (OUT/'include-evidence.json').write_text(json.dumps(includes,indent=2)+'\n')
    groups=collections.defaultdict(list)
    for o in occurrences:
        if not o['member']:groups[o['base']].append(o)
    (OUT/'candidate-review.txt').write_text('\n'.join(f"{n}\t{len(v)}\t"+' | '.join(f"{x['path']}:{x['line']} {x['name']}" for x in v[:2]) for n,v in sorted(groups.items()))+'\n')
    print(f'{len(files)} tracked files; {sum(x["scope"]=="C/C++" for x in coverage)} C/C++ files; {len(groups)} distinct non-member lexical names')
if __name__=='__main__':scan()
