"""Keep the fixture server alive while collecting a failed client's backtrace."""
import importlib.util
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True

fixture = Path(__file__).resolve().parents[1] / 'component/database/MariaDBFixture.py'
spec = importlib.util.spec_from_file_location('mariadb_fixture', fixture)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
execute = subprocess.run


def diagnose(arguments, **kwargs):
    completed = execute(arguments, **kwargs)
    if Path(arguments[0]).name == 'MariaDBConnectTest':
        print('ORIGINAL_CLIENT_RETURN_CODE=' + str(completed.returncode), flush=True)
        if completed.returncode != 0:
            trace = execute(['gdb', '--batch', '--return-child-result', '-ex', 'set disable-randomization off',
                             '-ex', 'run', '-ex', 'thread apply all bt full', '-ex', 'info registers',
                             '-ex', 'x/20i $pc-32', '--args', *arguments],
                            cwd=kwargs.get('cwd'), timeout=60)
            print('GDB_RETURN_CODE=' + str(trace.returncode), flush=True)
    return completed


module.subprocess.run = diagnose
sys.argv = [str(fixture), 'explicit', sys.argv[1]]
sys.exit(module.main())
