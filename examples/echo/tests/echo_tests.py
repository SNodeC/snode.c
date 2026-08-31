#!/usr/bin/env python3

import argparse
import os
import signal
import socket
import subprocess
import tempfile
import time
from pathlib import Path

HOST = "127.0.0.1"
GREETING = b"Hello peer! Nice to see you!!!"
GRACEFUL_TERMINATION_CODES = (
    0,
    -signal.SIGTERM,
    (-signal.SIGTERM) & 0xFF,
)


def test_environment():
    config_root = tempfile.TemporaryDirectory(prefix="snodec-echo-config-")
    env = os.environ.copy()
    env["XDG_CONFIG_HOME"] = config_root.name
    return env, config_root


def reserve_port():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind((HOST, 0))
        return int(sock.getsockname()[1])


def recv_exact(sock, size):
    data = bytearray()
    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if not chunk:
            raise AssertionError(
                f"connection closed after {len(data)} of {size} bytes"
            )
        data.extend(chunk)
    return bytes(data)


def stop_process(process):
    if process.poll() is None:
        process.terminate()
        try:
            return process.communicate(timeout=2)[0]
        except subprocess.TimeoutExpired:
            process.kill()
    return process.communicate(timeout=2)[0]


def wait_for_connection(port, timeout=3.0):
    deadline = time.monotonic() + timeout
    last_error = None

    while time.monotonic() < deadline:
        try:
            return socket.create_connection((HOST, port), timeout=0.5)
        except OSError as error:
            last_error = error
            time.sleep(0.05)

    raise AssertionError(
        f"server did not accept connections on {HOST}:{port}: {last_error}"
    )


def server_command(server, port, quiet_application_log=False):
    command = [str(server), "--monochrom=true"]

    if quiet_application_log:
        command.append("--log-origin-level=application=warn")

    command.extend(
        [
            "echoserver",
            "local",
            "--host",
            HOST,
            "--port",
            str(port),
        ]
    )
    return command


def client_command(client, port, quiet_application_log=False):
    command = [str(client), "--monochrom=true"]

    if quiet_application_log:
        command.append("--log-origin-level=application=warn")

    command.extend(
        [
            "echoclient",
            "remote",
            "--host",
            HOST,
            "--port",
            str(port),
        ]
    )
    return command


def check_help(executable, expected, env):
    result = subprocess.run(
        [str(executable), "--monochrom=true", "--help=expanded"],
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=5,
        check=False,
    )
    # SNode.C reports an intentionally skipped bootstrap as status 2 after
    # printing command-line help.
    if result.returncode not in (0, 2):
        raise AssertionError(
            f"{executable.name} --help=expanded failed:\n{result.stdout}"
        )

    for token in expected:
        if token not in result.stdout:
            raise AssertionError(
                f"{executable.name} help does not contain {token!r}"
            )


def test_config(server, client):
    env, config_root = test_environment()
    try:
        check_help(server, ("echoserver", "local"), env)
        check_help(client, ("echoclient", "remote"), env)
    finally:
        config_root.cleanup()


def test_server(server):
    env, config_root = test_environment()
    port = reserve_port()
    process = subprocess.Popen(
        server_command(server, port),
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )

    try:
        with wait_for_connection(port) as peer:
            peer.settimeout(2)
            payload = b"SNode.C echo test:\x00binary-safe\n"
            peer.sendall(payload)
            reflected = recv_exact(peer, len(payload))
            if reflected != payload:
                raise AssertionError(
                    f"server reflected {reflected!r}, expected {payload!r}"
                )
    finally:
        output = stop_process(process)
        config_root.cleanup()

    if process.returncode not in GRACEFUL_TERMINATION_CODES:
        raise AssertionError(
            f"echoserver exited with {process.returncode}:\n{output}"
        )


def test_client(client):
    env, config_root = test_environment()

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        listener.bind((HOST, 0))
        listener.listen(1)
        listener.settimeout(3)
        port = int(listener.getsockname()[1])

        process = subprocess.Popen(
            client_command(client, port),
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )

        try:
            peer, _ = listener.accept()
            with peer:
                peer.settimeout(2)
                greeting = recv_exact(peer, len(GREETING))
                if greeting != GREETING:
                    raise AssertionError(
                        f"client sent {greeting!r}, expected {GREETING!r}"
                    )

                peer.sendall(GREETING)
                reflected = recv_exact(peer, len(GREETING))
                if reflected != GREETING:
                    raise AssertionError(
                        f"client reflected {reflected!r}, expected {GREETING!r}"
                    )
        finally:
            output = stop_process(process)
            config_root.cleanup()

    if process.returncode not in GRACEFUL_TERMINATION_CODES:
        raise AssertionError(
            f"echoclient exited with {process.returncode}:\n{output}"
        )


def test_pair(server, client):
    env, config_root = test_environment()
    port = reserve_port()

    server_process = subprocess.Popen(
        server_command(server, port, quiet_application_log=True),
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.STDOUT,
    )

    client_process = None

    try:
        probe = wait_for_connection(port)
        probe.close()

        client_process = subprocess.Popen(
            client_command(client, port, quiet_application_log=True),
            env=env,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.STDOUT,
        )

        time.sleep(0.25)

        if server_process.poll() is not None:
            raise AssertionError(
                f"echoserver exited early with {server_process.returncode}"
            )
        if client_process.poll() is not None:
            raise AssertionError(
                f"echoclient exited early with {client_process.returncode}"
            )
    finally:
        if client_process is not None:
            stop_process(client_process)
        stop_process(server_process)
        config_root.cleanup()


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("config", "server", "client", "pair"))
    parser.add_argument("--server", type=Path)
    parser.add_argument("--client", type=Path)
    return parser.parse_args()


def main():
    args = parse_args()

    if args.mode == "config":
        if args.server is None or args.client is None:
            raise SystemExit("config mode requires --server and --client")
        test_config(args.server, args.client)
    elif args.mode == "server":
        if args.server is None:
            raise SystemExit("server mode requires --server")
        test_server(args.server)
    elif args.mode == "client":
        if args.client is None:
            raise SystemExit("client mode requires --client")
        test_client(args.client)
    else:
        if args.server is None or args.client is None:
            raise SystemExit("pair mode requires --server and --client")
        test_pair(args.server, args.client)


if __name__ == "__main__":
    main()
