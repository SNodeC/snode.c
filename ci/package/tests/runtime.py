"""Exercise the replacement transaction and real installed public API in a disposable Fedora container."""
import hashlib
from pathlib import Path
import subprocess
import sys


def output(*arguments):
    value = subprocess.check_output(arguments, text=True).strip()
    return value


def snapshot():
    records = {}
    names = ('mariadb-connector-c-devel', 'mariadb-connector-c-config')
    records['packages'] = output('rpm', '-q', *names)
    for name in output('rpm', '-ql', *names).splitlines():
        path = Path(name)
        if path.is_symlink():
            records[name] = ('link', str(path.readlink()))
        elif path.is_file():
            records[name] = (path.stat().st_mode, hashlib.sha256(path.read_bytes()).hexdigest())
    return records


def main():
    source, packages, build = map(Path, sys.argv[1:])
    before = snapshot()
    original = output('rpm', '-q', 'mariadb-connector-c')
    files = sorted(packages.glob('*.rpm'))
    assert files, 'expected real worker RPM outputs'
    names = [output('rpm', '-qp', '--queryformat', '%{NAME}', str(path)) for path in files]
    assert names.count('snodec-libmariadb') == 1
    subprocess.run(['dnf', '-y', 'install', *map(str, files)], check=True)
    assert snapshot() == before, 'runtime replacement changed system configuration or development files'
    provider = output('rpm', '-q', '--whatprovides', 'libmariadb.so.3()(64bit)', '--queryformat', '%{NAME}\n')
    assert provider == 'snodec-libmariadb', 'exactly one authoritative runtime provider is required'
    version = output('rpm', '-q', '--queryformat', '%{VERSION}-%{RELEASE}', 'snodec-libmariadb')
    assert 'snodec-libmariadb = ' + version in output('rpm', '-q', '--requires', 'snodec-db-mariadb')
    assert not any(path.startswith('/etc/') for path in output('rpm', '-ql', 'snodec-libmariadb').splitlines())
    subprocess.run(['cmake', '-S', str(source / 'ci/package/tests/installed-runtime'), '-B', str(build),
                    '-G', 'Ninja', '-DSNODEC_SOURCE_DIR=' + str(source)], check=True)
    subprocess.run(['cmake', '--build', str(build), '--parallel', '2'], check=True)
    libraries = output('ldd', str(build / 'database/MariaDBConnectTest'))
    for line in libraries.splitlines():
        if 'libmariadb.so.3 =>' in line or 'libsnodec-' in line:
            assert str(Path(line.split('=>')[1].split()[0]).resolve()).startswith('/usr/lib64/')
    subprocess.run(['ctest', '--test-dir', str(build), '--output-on-failure', '-R', '^MariaDB', '--no-tests=error'], check=True)
    print('Installed runtime: 26 public-API cases; one /usr provider; configuration and development files preserved', flush=True)
    dependents = [name for name in names if name != 'snodec-libmariadb']
    subprocess.run(['dnf', '-y', '--setopt=clean_requirements_on_remove=False', 'remove', *dependents], check=True)
    subprocess.run(['dnf', '-y', 'swap', 'snodec-libmariadb', 'mariadb-connector-c'], check=True)
    assert output('rpm', '-q', 'mariadb-connector-c') == original
    assert snapshot() == before
    print('Runtime removal/rollback: original Fedora runtime restored; configuration/development unchanged', flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
