"""Execute source hooks against command fixtures; verify their public SDK policy."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def probe(native, fmt='deb'):
    repository = Path(__file__).resolve().parents[2]
    package = json.loads((repository / 'ci/packages.json').read_text())['package']
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        source, sdk, staging, commands = (root / name for name in ('source', 'sdk', 'staging', 'commands'))
        for directory in [source / 'supplement/openwrt', source / 'misc/openwrt', sdk / 'scripts', root / 'bin',
                          staging / 'usr/share/core-ci/openwrt/snode.c', staging / 'usr/share/core-ci/pkginfo']:
            directory.mkdir(parents=True)
        (source / 'payload').write_text('hook fixture')
        (sdk / 'feeds.conf.default').write_text('src-git base fixture\nsrc-git packages fixture\nsrc-git luci fixture\n')
        (sdk / '.config').write_text('CONFIG_ALL=y\nCONFIG_SIGNED_PACKAGES=y\n')
        target = root / 'target.json'
        target.write_text(json.dumps({'build': {'format': fmt}}))
        stub = root / 'bin/command'
        stub.write_text(f'#!{sys.executable}\n' + r'''
import json, os, pathlib, subprocess, sys
name = pathlib.Path(sys.argv[0]).name
args = sys.argv[1:]
with open(os.environ['COMMANDS'], 'a') as log:
    log.write(json.dumps([name, *args]) + '\n')
if name == 'nproc':
    print('6')
elif name == 'dpkg-architecture':
    print('fixture-linux-gnu')
elif name == 'python3':
    if args and args[0] == '-c':
        subprocess.run([sys.executable, *args], check=True)
    elif args == ['-']:
        sys.stdin.read()
        print('1.2.3 7')
elif name == 'make':
    if 'defconfig' in args:
        config = pathlib.Path('.config').read_text()
        for flag in ('ALL', 'ALL_NONSHARED', 'ALL_KMODS', 'AUTOREMOVE', 'SIGNED_PACKAGES'):
            assert '# CONFIG_' + flag + ' is not set' in config
        assert 'CONFIG_PACKAGE_' + os.environ['PACKAGE_NAMESPACE'] + '=m' in config
        assert '=y' not in config
    if 'dependencies' in args:
        sys.stdin.read()
        print('package/fixture/compile')
elif name == 'ctest':
    if '--show-only=json-v1' in args:
        print(json.dumps({'tests': [{'name': 'fixture'}]}))
    elif '--output-junit' in args:
        pathlib.Path(args[args.index('--output-junit') + 1]).write_text('<testsuite><testcase name="fixture"/></testsuite>')
''')
        stub.chmod(0o755)
        for name in ('make', 'cmake', 'ctest', 'python3', 'nproc', 'dpkg-architecture'):
            (root / 'bin' / name).symlink_to(stub)
        (sdk / 'scripts/feeds').symlink_to(stub)
        kind = 'native' if native else 'openwrt'
        hook = (repository / f'ci/package/{kind}.sh').read_text()
        # The fixture provides the same mount paths beneath its private root.
        hook = hook.replace('/work/', str(root / 'work') + '/').replace('/tool/', str(root / 'tool') + '/')
        environment = os.environ | dict(SOURCE_DIR=str(source), SDK_DIR=str(sdk), SDK_STAGING_DIR=str(staging),
            SOURCE_TAG='v1.2.3', PACKAGE_REVISION='7', PACKAGE_UPSTREAM_VERSION='1.2.3', PACKAGE_NAMESPACE=package,
            OUTPUT_DIR=str(root / 'output'), TARGET_JSON=str(target), CI_DIR=str(repository),
            PREREQUISITES_DIR=str(root / 'prerequisites'), COMMANDS=str(commands),
            PATH=str(root / 'bin') + os.pathsep + os.environ['PATH'])
        subprocess.run(['/bin/sh', '-ec', hook], cwd=root, env=environment, check=True, capture_output=True)
        calls = [json.loads(line) for line in commands.read_text().splitlines()]
        if native:
            builds = [call for call in calls if call[:2] == ['cmake', '--build']]
            assert len(builds) == 1 and builds[0][-2:] == ['--parallel', '6'], calls
            architecture = [call for call in calls if call[0] == 'dpkg-architecture']
            assert architecture == ([['dpkg-architecture', '-qDEB_HOST_MULTIARCH']] if fmt == 'deb' else []), calls
            configure = [call for call in calls if call[0] == 'cmake' and '-S' in call]
            libdir = 'lib/fixture-linux-gnu' if fmt == 'deb' else 'lib64'
            assert '-DCMAKE_INSTALL_LIBDIR=' + libdir in configure[0], calls
        else:
            feeds = [call for call in calls if call[0] == 'feeds']
            assert feeds == [['feeds', 'update', 'base', 'packages', 'core_ci'],
                             ['feeds', 'install', '-a', '-p', 'core_ci']], feeds
            builds = [call for call in calls if call[0] == 'make' and '-j6' in call]
            assert len(builds) == (2 if package == 'mqttsuite' else 1), calls
            assert not any('-j2' in call or '-a' in call[2:] for call in calls if call[:2] == ['feeds', 'update'])
            assert calls[-1][0] == 'python3' and calls[-1][-1] == '--outputs'
            if package == 'snodec':
                build = sdk / 'build_dir/target-fixture/snode.c-1'
                (build / 'Testing').mkdir(parents=True)
                (build / 'CMakeCache.txt').write_text('fixture cache')
                (sdk / 'staging_dir/target-fixture').mkdir(parents=True)
                (sdk / 'staging_dir/toolchain-fixture').mkdir()
                host = sdk / 'staging_dir/host/bin'
                host.mkdir(parents=True)
                for name in ('cmake', 'ctest'):
                    (host / name).symlink_to(stub)
                (root / 'bin/qemu-x86_64').symlink_to(stub)
                with (sdk / '.config').open('a') as config:
                    config.write('CONFIG_TARGET_ARCH_PACKAGES="x86_64"\n')
                subprocess.run([sys.executable, str(repository / 'ci/package/openwrt-tests.py'), str(sdk)],
                               env=environment, check=True, capture_output=True)
                calls = [json.loads(line) for line in commands.read_text().splitlines()]
                builds = [call for call in calls if call[:2] == ['cmake', '--build']]
                assert len(builds) == 1 and builds[0][-2:] == ['--parallel', '6'], calls
    return None


def main():
    probe(False)
    probe(True)
    probe(True, 'rpm')
    print('Package hooks: bounded SDK config, selected feeds, available CPUs and output handoff passed')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
