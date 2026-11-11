"""Build the corrected Fedora runtime consumed by tests and the RPM component."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import urllib.request

sys.path.insert(0, '/tool')
from contract import output, read_json, require
from versions import versions


def main():
    destination = Path(sys.argv[1])
    work = destination.parent / 'connector-source'
    work.mkdir()
    archive = work / 'source.rpm'
    url = ('https://dl.fedoraproject.org/pub/fedora/linux/updates/44/Everything/source/tree/Packages/m/'
           'mariadb-connector-c-3.4.9-2.fc44.src.rpm')
    with urllib.request.urlopen(url, timeout=60) as response:
        payload = response.read(4 * 1024 * 1024)
    require(hashlib.sha256(payload).hexdigest() ==
            '93306d5793053af05c2826c091b048e7dd0079d7c4c52e12e00486f7173da101', 'connector source RPM checksum mismatch')
    archive.write_bytes(payload)
    contents = subprocess.check_output(['rpm2cpio', str(archive)])
    subprocess.run(['cpio', '-id', '--no-absolute-filenames'], input=contents, cwd=work, check=True)
    source_archive = work / 'mariadb-connector-c-3.4.9-src.tar.gz'
    require(hashlib.sha256(source_archive.read_bytes()).hexdigest() ==
            'a84bba97e59b6a322637a189964d4fd72bd8d92f2d22a9f8d6a5f0657c821e97', 'connector source checksum mismatch')
    with tarfile.open(source_archive) as source:
        source.extractall(work, filter='data')
    source = work / 'mariadb-connector-c-3.4.9-src'
    for patch in (work / 'conc-821-fix-bind-result-length.patch',
                  Path(__file__).with_name('mariadb-context-registers.patch')):
        with patch.open('rb') as incoming:
            subprocess.run(['patch', '--batch', '--fuzz=0', '-p1'], stdin=incoming, cwd=source, check=True)
    target = read_json(os.environ['TARGET_JSON'])
    base = output(['rpm', '-q', '--queryformat', '%{VERSION}-%{RELEASE}', 'mariadb-connector-c'])
    require(base == '3.4.9-2.fc' + target['suite'], 'review connector pin against the installed Fedora version')
    encoded = versions(os.environ['SOURCE_VERSION'], int(os.environ['PACKAGE_REVISION']), 'rpm', target)
    build = work / 'build'
    subprocess.run(['cmake', '-S', str(source), '-B', str(build), '-G', 'Ninja',
                    '-DCMAKE_BUILD_TYPE=RelWithDebInfo', '-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON',
                    '-DCMAKE_COMPILE_WARNING_AS_ERROR=OFF',
                    '-DCMAKE_C_FLAGS=' + output(['rpm', '--eval', '%{build_cflags}']),
                    '-DCMAKE_SHARED_LINKER_FLAGS=' + output(['rpm', '--eval', '%{build_ldflags}']),
                    '-DCMAKE_INSTALL_PREFIX=/usr', '-DINSTALL_LAYOUT=RPM', '-DINSTALL_LIBDIR=lib64',
                    '-DINSTALL_PLUGINDIR=lib64/mariadb/plugin', '-DINSTALL_INCLUDEDIR=include/mysql',
                    '-DINSTALL_PCDIR=lib64/pkgconfig', '-DMARIADB_UNIX_ADDR=/var/lib/mysql/mysql.sock',
                    '-DMARIADB_PORT=3306', '-DWITH_EXTERNAL_ZLIB=ON', '-DWITH_MYSQLCOMPAT=ON',
                    '-DPLUGIN_CLIENT_ED25519=DYNAMIC', '-DDEFAULT_SSL_VERIFY_SERVER_CERT=OFF',
                    '-DWITH_UNIT_TESTS=OFF', '-DWITH_SSL=OPENSSL'], check=True)
    subprocess.run(['cmake', '--build', str(build), '--parallel', output(['nproc'])], check=True)
    subprocess.run(['cmake', '--install', str(build)], env={**os.environ, 'DESTDIR': str(destination)}, check=True)
    plugins = {'auth_gssapi_client', 'caching_sha2_password', 'client_ed25519', 'dialog', 'mysql_clear_password',
               'parsec', 'remote_io', 'sha256_password', 'zstd'}
    require({p.stem for p in (destination / 'usr/lib64/mariadb/plugin').glob('*.so')} == plugins,
            'corrected connector must preserve every Fedora runtime plugin')
    for name in ('COPYING.LIB', 'README'):
        shutil.copyfile(source / name, destination / name)
    (destination / 'metadata.cmake').write_text('set(SNODEC_MARIADB_BASE "' + base + '")\n'
                                              'set(SNODEC_MARIADB_VERSION "' + encoded['full'] + '")\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
