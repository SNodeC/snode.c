"""Run the complete upstream CTest suite for one OpenWrt SDK build.

QEMU executes target instructions and the SDK supplies musl and target libraries.
The runner's Linux kernel supplies syscalls; device/driver validation is separate.
"""
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET


def run(sdk):
    if os.geteuid() == 0:
        raise RuntimeError('Run upstream tests as a non-root user so initialization tests execute')
    sdk = Path(sdk).resolve()
    builds = list(sdk.glob('build_dir/target-*/snode.c-*/CMakeCache.txt'))
    if len(builds) != 1:
        raise RuntimeError(f'Expected one SNode.C build, found {builds}')
    build = builds[0].parent
    targets = list(sdk.glob('staging_dir/target-*'))
    toolchains = list(sdk.glob('staging_dir/toolchain-*'))
    if len(targets) != 1 or len(toolchains) != 1:
        raise RuntimeError('Ambiguous SDK target or toolchain')
    target, toolchain = targets[0], toolchains[0]
    config = dict(line.split('=', 1) for line in (sdk / '.config').read_text().splitlines()
                  if '=' in line and not line.startswith('#'))
    arch = config['CONFIG_TARGET_ARCH_PACKAGES'].strip('"')
    family = arch.split('_')[0]
    emulator = {'x86': 'x86_64', 'i386': 'i386', 'aarch64': 'aarch64', 'arm': 'arm',
                'mips': 'mips', 'mipsel': 'mipsel', 'mips64': 'mips64',
                'powerpc': 'ppc', 'riscv64': 'riscv64', 'loongarch64': 'loongarch64'}[family]
    qemu = shutil.which('qemu-' + emulator) or shutil.which('qemu-' + emulator + '-static')
    if not qemu:
        raise RuntimeError(f'Missing QEMU emulator for {arch}')
    cpu = {'powerpc_8548': 'e500v2', 'powerpc_464fp': '440ep',
           'mips64_octeonplus': 'Octeon68XX'}.get(arch)
    command = [qemu, '-U', 'LD_PRELOAD'] + (['-cpu', cpu] if cpu else []) + ['-L', str(toolchain)]
    logs = sdk / 'logs/upstream'
    logs.mkdir(parents=True, exist_ok=True)
    launcher = logs / 'target-exec'
    cmake = sdk / 'staging_dir/host/bin/cmake'
    ctest = sdk / 'staging_dir/host/bin/ctest'
    if not ctest.exists():
        ctest = Path(shutil.which('ctest'))
    env = dict(os.environ, STAGING_DIR=str(target), STAGING_PREFIX=str(target / 'usr'),
               STAGING_DIR_HOST=str(sdk / 'staging_dir/host'),
               STAGING_DIR_HOSTPKG=str(sdk / 'staging_dir/hostpkg'),
               PKG_CONFIG_LIBDIR=str(target / 'usr/lib/pkgconfig'),
               PATH=str(sdk / 'staging_dir/host/bin') + os.pathsep + os.environ['PATH'])
    # Preserve the SDK's build configuration and enable target execution.
    subprocess.run([str(cmake), '-S', str(build), '-B', str(build),
                    '-DSNODEC_BUILD_TESTS=ON', '-DBUILD_TESTING=ON',
                    '-DCMAKE_CROSSCOMPILING_EMULATOR=' + str(launcher)], check=True, env=env)
    subprocess.run([str(cmake), '--build', str(build), '--parallel', str(min(os.cpu_count() or 2, 4))], check=True, env=env)
    # The toolchain owns the dynamic loader. Include every SDK-installed shared
    # library directory, including SNode.C's plugin directories.
    library_dirs = sorted({str(p.parent) for root in (build / 'src', build / 'snode.c', target / 'usr/lib', toolchain / 'lib')
                           for p in root.rglob('*.so*') if p.is_file()})
    launcher.write_text('#!/bin/sh\nexec ' + shlex.join(command) +
                        ' -E LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+$LD_LIBRARY_PATH:}"' +
                        shlex.quote(':'.join(library_dirs)) + ' "$@"\n')
    launcher.chmod(0o755)
    manifest = subprocess.check_output([str(ctest), '--test-dir', str(build), '--show-only=json-v1'], env=env)
    (logs / 'manifest.json').write_bytes(manifest)
    tests = json.loads(manifest)['tests']
    if not tests:
        raise RuntimeError('No upstream tests registered')
    result = subprocess.run([str(ctest), '--test-dir', str(build), '--output-on-failure',
                             '--no-tests=error', '--parallel', '1',
                             '--output-junit', str(logs / 'results.xml')], env=env)
    shutil.copytree(build / 'Testing', logs / 'Testing', dirs_exist_ok=True)
    report = ET.parse(logs / 'results.xml')
    cases = report.findall('.//testcase')
    skipped = [case.attrib['name'] for case in cases
               if case.find('skipped') is not None or
               any(line.startswith('SKIP:') for line in case.findtext('system-out', '').splitlines())]
    if result.returncode or len(cases) != len(tests) or skipped:
        raise RuntimeError(f'Upstream tests incomplete or failed: {len(cases)}/{len(tests)}, skipped={skipped}')
    print(f'{arch}: all {len(cases)} upstream tests passed without skips', flush=True)
    return None


if __name__ == '__main__':
    run(sys.argv[1])
