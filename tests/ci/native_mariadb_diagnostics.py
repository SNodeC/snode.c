"""Reproduce native ARM64 failures using the existing worker and source hook."""
import argparse
import json
from pathlib import Path
import platform
import shlex
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--worker', type=Path, required=True)
parser.add_argument('--target', required=True)
args = parser.parse_args()
source = Path(__file__).resolve().parents[2]
worker = args.worker.resolve()
sys.path.insert(0, str(worker))
from coreci.build import TOOLS

assert platform.machine() in ('aarch64', 'arm64'), 'this diagnostic requires a native ARM64 host'
catalog = json.loads((worker / 'ci/targets.json').read_text())['targets']
target = next(entry for entry in catalog if entry['id'] == args.target)
assert target['build']['platform'] == 'linux/arm64'
declaration = json.loads((source / 'ci/packages.json').read_text())
work = Path('/tmp/codex/build/snode.c/native-mariadb') / args.target
work.mkdir(parents=True, exist_ok=False)
fmt = target['build']['format']
tools = TOOLS[fmt]
if target['distribution'] == 'rocky':
    tools = 'dnf install -y dnf-plugins-core epel-release && dnf config-manager --set-enabled crb && ' + tools
dependencies = declaration['build_dependencies'][fmt] + ['gdb']
tools += ' && ' + ('apt-get install -y ' if fmt == 'deb' else 'dnf install -y ') + shlex.join(dependencies)
if fmt == 'deb':
    sources = ("mkdir /connector && sed -i 's/^Types: deb$/Types: deb deb-src/' /etc/apt/sources.list.d/ubuntu.sources"
               " && apt-get update && cd /connector && apt-get source mariadb=$(dpkg-query -W -f='${Version}' libmariadb3)")
else:
    sources = ("mkdir /connector && cd /connector && dnf download --source mariadb-connector-c"
               " && rpm2cpio *.src.rpm | cpio -id --no-absolute-filenames"
               " && rpmbuild -bp --nodeps --define '_sourcedir /connector' --define '_builddir /connector/prepared' *.spec")
(work / 'target.json').write_text(json.dumps(target))
(work / 'environment.json').write_text('{}')
(work / 'Dockerfile').write_text('FROM ' + target['build']['image'] + '\nRUN ' + tools + '\nRUN ' + sources + '\n'
    + 'RUN if getent passwd 1000; then usermod -d /work "$(getent passwd 1000 | cut -d: -f1)"; '
    'else useradd -u 1000 -U -M -d /work diagnostic; fi; mkdir -m 1777 /work /output\n')
image = 'snodec-native-diagnostic:' + args.target
subprocess.run(['docker', 'build', '--platform', 'linux/arm64', '-t', image, str(work)], check=True)
script = ('mkdir -p /work/output; mkdir -m 0700 /work/config /work/state /work/run /work/data; uname -m; '
          + ('dpkg-query -W libmariadb3; ' if fmt == 'deb' else 'rpm -q mariadb-connector-c; ')
          + 'python3 /tool/container.py --tag v2.1.0 --version 2.1.0 --revision 2 --package snodec '
          '--build-id snodec~diagnostic~2.1.0~r2 --feed packages; result=$?; '
          'if test -x /work/build-product/tests/component/database/MariaDBConnectTest; then '
          'python3 /source/tests/ci/diagnose_mariadb.py /work/build-product/tests/component/database/MariaDBConnectTest; '
          'python3 /source/tests/ci/mariadb_connector_ab.py ' + fmt + '; result=$?; fi; exit "$result"')
command = ['docker', 'run', '--rm', '--init', '--platform', 'linux/arm64', '--user', '1000:1000',
           '--read-only', '--cap-drop=ALL', '--security-opt=no-new-privileges', '--memory=4g', '--pids-limit=256',
           '--tmpfs', '/tmp:rw,exec,size=2g,mode=1777', '--tmpfs', '/var/tmp:rw,exec,size=2g,mode=1777', '--volume', '/work', '--volume', '/output',
           '--mount', f'type=bind,source={source},target=/source,readonly',
           '--mount', f'type=bind,source={source},target=/definition,readonly',
           '--mount', f'type=bind,source={worker / "coreci"},target=/tool,readonly',
           '--mount', f'type=bind,source={work / "target.json"},target=/target.json,readonly',
           '--mount', f'type=bind,source={work / "environment.json"},target=/environment.json,readonly',
           '-e', 'SOURCE_DIR=/source', '-e', 'CI_DIR=/source', '-e', 'TARGET_JSON=/target.json',
           '-e', 'OUTPUT_DIR=/work/output', '-e', 'SOURCE_VERSION=' + (source / 'VERSION').read_text().strip(),
           '-e', 'PACKAGE_REVISION=1', '-e', 'XDG_CONFIG_HOME=/work/config', '-e', 'XDG_STATE_HOME=/work/state',
           '-e', 'XDG_RUNTIME_DIR=/work/run', '-e', 'XDG_DATA_HOME=/work/data', image, '/bin/sh', '-c', script]
print('DIAGNOSTIC_HOST=' + platform.machine(), flush=True)
subprocess.run(['git', '-C', str(source), 'rev-parse', 'HEAD'], check=True)
with (work / 'native.log').open('w') as output:
    completed = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT)
print((work / 'native.log').read_text()[-30000:])
print('NATIVE_HOOK_EXIT=' + str(completed.returncode), flush=True)
sys.exit(completed.returncode)
