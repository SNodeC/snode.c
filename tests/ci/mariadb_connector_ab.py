"""Diagnostic-only A/B of the distribution connector and the reviewed patch."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys

root = Path('/work/connector-ab')
root.mkdir()
shutil.copytree('/connector', root / 'source', symlinks=True)
contexts = list((root / 'source').rglob('ma_context.c'))
assert len(contexts) == 1, contexts
source = contexts[0].parent.parent
assert (source / 'CMakeLists.txt').is_file()
fmt = sys.argv[1]
flags = subprocess.check_output(['dpkg-buildflags', '--get', 'CFLAGS'] if fmt == 'deb'
                                else ['rpm', '--eval', '%{build_cflags}'], text=True).strip()
print('CONNECTOR_SOURCE=' + str(source), flush=True)
print('CONNECTOR_FLAGS=' + flags, flush=True)
outcomes = []
for arm in ('unpatched', 'patched'):
    if arm == 'patched':
        patch_file = (Path(__file__).with_name('mariadb-context-3.4.4.patch') if fmt == 'rpm'
                      else Path(__file__).resolve().parents[2] / 'ci/package/mariadb-context-registers.patch')
        with patch_file.open('rb') as patch:
            subprocess.run(['patch', '--batch', '--fuzz=0', '-p1'], cwd=source, stdin=patch, check=True)
    print('CONTEXT_' + arm + '_SHA256=' + hashlib.sha256(contexts[0].read_bytes()).hexdigest(), flush=True)
    build = root / arm
    subprocess.run(['cmake', '-S', str(source), '-B', str(build), '-G', 'Ninja',
                    '-DCMAKE_BUILD_TYPE=RelWithDebInfo', '-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON', '-DCMAKE_C_STANDARD=99',
                    '-DCMAKE_C_FLAGS=' + flags, '-DWITH_EXTERNAL_ZLIB=ON', '-DWITH_UNIT_TESTS=OFF', '-DWITH_SSL=OPENSSL'], check=True)
    subprocess.run(['cmake', '--build', str(build), '--parallel', '4'], check=True)
    library, = build.rglob('libmariadb.so.3')
    environment = dict(os.environ, LD_LIBRARY_PATH=str(library.parent))
    completed = subprocess.run(['ctest', '--test-dir', '/work/build-product', '-R', '^MariaDB',
                                '--output-on-failure', '--output-junit', str(root / (arm + '.xml'))], env=environment)
    outcomes.append((arm, completed.returncode))
    print('CONNECTOR_' + arm.upper() + '_CTEST_EXIT=' + str(completed.returncode), flush=True)
    subprocess.run(['python3', str(Path(__file__).with_name('diagnose_mariadb.py')),
                    '/work/build-product/tests/component/database/MariaDBConnectTest'], env=environment)
print('CONNECTOR_AB_OUTCOMES=' + repr(outcomes), flush=True)
sys.exit(0 if outcomes == [('unpatched', 8), ('patched', 0)] else 1)
