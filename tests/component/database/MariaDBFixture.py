"""Run one MariaDB component case with its own socket-only test server."""
import os
from pathlib import Path
import pwd
import shutil
import socket
import subprocess
import sys
import tempfile
import time


def main():
    result = 1
    try:
        executable, mode = sys.argv[1:]
        tools = {}
        for name in ("mariadb-install-db", "mariadbd"):
            found = shutil.which(name)
            if found is None and (Path("/usr/sbin") / name).is_file():
                found = str(Path("/usr/sbin") / name)
            if found is None:
                raise RuntimeError("MariaDB component tests require the installed tool: " + name)
            tools[name] = found
        account = pwd.getpwuid(os.getuid()).pw_name
        with tempfile.TemporaryDirectory(prefix="snodec-mariadb-") as directory:
            root = Path(directory)
            endpoint = root / "s"
            server = None
            try:
                if len(os.fsencode(endpoint)) + 29 > 107:
                    raise RuntimeError("Build path is too long for the MariaDB test Unix socket")
                with (root / "initialize.log").open("w") as output:
                    subprocess.run([tools["mariadb-install-db"], "--no-defaults", "--datadir=" + str(root / "data"),
                                    "--tmpdir=" + str(root), "--auth-root-authentication-method=socket", "--auth-root-socket-user=" + account,
                                    "--skip-test-db"], stdout=output, stderr=subprocess.STDOUT, check=True, timeout=40)
                with (root / "process.log").open("w") as output:
                    server = subprocess.Popen([tools["mariadbd"], "--no-defaults", "--user=" + account,
                                               "--datadir=" + str(root / "data"), "--tmpdir=" + str(root), "--socket=" + str(endpoint),
                                               "--pid-file=" + str(root / "server.pid"), "--log-error=" + str(root / "server.log"),
                                               "--skip-networking", "--innodb-buffer-pool-size=64M", "--innodb-log-file-size=16M"],
                                              stdout=output, stderr=subprocess.STDOUT)
                    deadline = time.monotonic() + 15
                    ready = False
                    while not ready and server.poll() is None and time.monotonic() < deadline:
                        try:
                            with socket.socket(socket.AF_UNIX) as probe:
                                probe.settimeout(0.2)
                                probe.connect(str(endpoint))
                                ready = len(probe.recv(4)) == 4
                        except OSError:
                            pass
                        if not ready:
                            time.sleep(0.05)
                    if not ready:
                        raise RuntimeError("Disposable MariaDB did not become ready")
                    result = subprocess.run([executable, mode, str(endpoint), account], cwd=root, timeout=10).returncode
            finally:
                if server is not None:
                    if server.poll() is None:
                        server.terminate()
                        try:
                            server.wait(timeout=15)
                        except subprocess.TimeoutExpired:
                            server.kill()
                            server.wait(timeout=5)
                    if server.returncode != 0:
                        result = 1
                if result != 0:
                    for name in ("initialize.log", "process.log", "server.log"):
                        log = root / name
                        if log.exists():
                            print(name + ":\n" + log.read_text(errors="replace"), file=sys.stderr)
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
        print("MariaDB fixture failed: " + str(error), file=sys.stderr)
        result = 1
    return result


if __name__ == "__main__":
    sys.exit(main())
