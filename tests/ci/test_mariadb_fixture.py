"""Exercise the fixture's native and emulator command boundary with real exec."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from types import SimpleNamespace
from unittest.mock import MagicMock, patch

sys.dont_write_bytecode = True


def main():
    result = 0
    if len(sys.argv) > 1 and sys.argv[1] == '--launcher':
        assert sys.argv[2] == 'launcher argument with spaces'
        os.execv(sys.argv[3], sys.argv[3:])
    elif len(sys.argv) > 1 and sys.argv[1] == '--child':
        Path(sys.argv[2]).write_text(json.dumps(sys.argv[3:]))
        result = int(sys.argv[3])
    else:
        fixture = Path(__file__).resolve().parents[1] / 'component/database/MariaDBFixture.py'
        spec = importlib.util.spec_from_file_location('mariadb_fixture', fixture)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        with tempfile.TemporaryDirectory(prefix='MariaDB command with spaces ') as directory:
            output = Path(directory) / 'child arguments.json'
            launcher = Path(directory) / 'launcher with spaces.py'
            launcher.symlink_to(Path(__file__).resolve())
            executable = Path(directory) / 'single executable launcher'
            executable.write_text('#!' + sys.executable + '\nimport os, sys\nos.execv(sys.argv[1], sys.argv[1:])\n')
            executable.chmod(0o755)
            commands = [[], [str(executable), sys.executable],
                        [sys.executable, str(launcher), '--launcher', 'launcher argument with spaces', sys.executable]]
            for prefix in commands:
                command = ([sys.executable] if not prefix else prefix) + [str(launcher), '--child', str(output)]
                server = MagicMock(returncode=0)
                server.poll.return_value = None
                probe = MagicMock()
                probe.__enter__.return_value.recv.return_value = b'head'
                calls = []

                def execute(arguments, **kwargs):
                    calls.append(arguments)
                    status = 0 if '--datadir=' in ' '.join(arguments) else os.spawnv(os.P_WAIT, arguments[0], arguments)
                    return SimpleNamespace(returncode=status)

                with patch.object(sys, 'argv', [str(fixture), '0', *command]), \
                     patch.object(module.shutil, 'which', side_effect=lambda name: '/fixture/' + name), \
                     patch.object(module.subprocess, 'run', side_effect=execute), \
                     patch.object(module.subprocess, 'Popen', return_value=server), \
                     patch.object(module.socket, 'socket', return_value=probe):
                    assert module.main() == 0
                arguments = json.loads(output.read_text())
                assert arguments[0] == '0' and Path(arguments[1]).name == 's' and arguments[2]
                assert calls[1] == command + arguments
                server.terminate.assert_called_once()
                server.wait.assert_called_once()
        print('MariaDB fixture: native and emulator argv, spaces and cleanup passed')
    return result


if __name__ == '__main__':
    sys.exit(main())
