"""Exercise the source-owned export hook without compiling an OpenWrt SDK."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


class DevelopmentExportTest(unittest.TestCase):
    def test_export_excludes_foreign_installed_files_and_shared_directories(self):
        with tempfile.TemporaryDirectory() as temporary:
            sdk = Path(temporary)
            target, bundle = sdk/'staging_dir/target-test', sdk/'bundle'
            for directory in ['source/supplement/openwrt','source/ci/package','recipes/snode.c','bundle','bin',
                              'staging_dir/target-test/usr/include','staging_dir/target-test/usr/lib',
                              'staging_dir/target-test/pkginfo','staging_dir/target-test/packages']:
                (sdk/directory).mkdir(parents=True,exist_ok=True)
            (sdk/'source/supplement/openwrt/Makefile').write_text('owned recipe')
            (sdk/'source/ci/package/openwrt-tests.py').write_text('pass\n')
            (bundle/'context.json').write_text(json.dumps(dict(source_tags={'snode.c':'v1.0.0'},versions={'snode.c':'1.0.0'})))
            (bundle/'snode.c-1.0.0.tar.gz').write_bytes(b'fixture source archive')
            for name in ['usr/include/own.h','usr/include/foreign.h','usr/lib/libown.so','usr/lib/libforeign.so',
                         'pkginfo/snodec.provides','pkginfo/foreign.provides']:
                (target/name).write_text(name)
            (target/'usr/lib/libown-link.so').symlink_to('libown.so')
            (target/'packages/snode.c.list').write_text(
                './\n./usr\n./usr/include\n./usr/include/own.h\n./usr/lib\n./usr/lib/libown.so\n./usr/lib/libown-link.so\n')
            (sdk/'bin/make').write_text('#!/bin/sh\nexit 0\n')
            (sdk/'bin/jq').write_text('#!/bin/sh\necho fixture\n')
            # Only record the generic export call; other Python calls run normally.
            (sdk/'bin/python3').write_text(f'#!{sys.executable}\n'
                'import json,os,sys\nfrom pathlib import Path\n'
                'if sys.argv[1:4] == ["-m","ci.repository","sdk-export"]:\n'
                '    Path(os.environ["EXPORT_ARGS"]).write_text(json.dumps(sys.argv[6:]))\n'
                'else:\n    os.execv(sys.executable,[sys.executable,*sys.argv[1:]])\n')
            for path in (sdk/'bin').iterdir():
                path.chmod(0o755)
            arguments = sdk/'export-args.json'
            env = os.environ | dict(sdk=str(sdk),bundle=str(bundle),PACKAGE_RELEASE='2',BUILD_PROJECT='snode.c',
                EXPORT_ARGS=str(arguments),PYTHONDONTWRITEBYTECODE='1',PATH=str(sdk/'bin')+os.pathsep+os.environ['PATH'])
            hook = Path(__file__).resolve().parents[2]/'ci/package/openwrt.sh'
            subprocess.run(['bash','-c','set -euo pipefail; source "$1"; compile','fixture',str(hook)],
                           cwd=sdk,env=env,check=True,capture_output=True)
            selected = [sdk/name for name in json.loads(arguments.read_text())]
            members = {str(p.relative_to(sdk)) for root in selected
                       for p in (root.rglob('*') if root.is_dir() else [root]) if p.is_file() or p.is_symlink()}
            self.assertEqual(members,{'recipes/snode.c/Makefile','staging_dir/target-test/usr/include/own.h',
                'staging_dir/target-test/usr/lib/libown.so','staging_dir/target-test/usr/lib/libown-link.so',
                'staging_dir/target-test/pkginfo/snodec.provides'})
        return None


if __name__ == '__main__':
    unittest.main()
