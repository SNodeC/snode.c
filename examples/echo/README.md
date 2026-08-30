# External SNode.C echo pair

This directory is a complete standalone CMake project that consumes an installed
SNode.C package. It builds a plain-IPv4 echo server and client and includes four
application-level CTests.

## Requirements

- a C++20 compiler;
- CMake 3.18 or newer;
- an installed SNode.C package containing the `net-in-stream-legacy` component;
- Python 3 when `BUILD_TESTING=ON`.

If SNode.C is installed below a non-system prefix, pass that prefix through
`CMAKE_PREFIX_PATH`.

## Build

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$HOME/.local"

cmake --build build --parallel 8
```

The project resolves SNode.C with:

```cmake
find_package(
    snodec REQUIRED
    COMPONENTS net-in-stream-legacy
)
```

and links the exported target `snodec::net-in-stream-legacy`.

## Run

Start the server:

```sh
mkdir -p build/echo-config

XDG_CONFIG_HOME="$PWD/build/echo-config" \
  ./build/echoserver \
  --monochrom=true \
  echoserver local --host 127.0.0.1 --port 18001
```

Then start the client:

```sh
XDG_CONFIG_HOME="$PWD/build/echo-config" \
  ./build/echoclient \
  --monochrom=true \
  echoclient remote --host 127.0.0.1 --port 18001
```

Both sides use the same `EchoSocketContext`. The client sends an initial greeting
from `onConnected()`, and each received chunk is logged through the context's
semantic application logger and sent back unchanged.

## Test

```sh
ctest --test-dir build --output-on-failure
```

The tests verify the generated configuration surface, the real server against a
deterministic external peer, the real client against a deterministic external
peer, and a bounded real-pair smoke run.

For the annotated source-to-build walkthrough, see
[`src/apps/README.md`](../../src/apps/README.md).
