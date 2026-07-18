#!/usr/bin/env bash
set -euo pipefail
build_dir="$1"
prefix="$(mktemp -d)"
work="$(mktemp -d)"
trap 'rm -rf "$prefix" "$work"' EXIT
cmake --install "$build_dir" --prefix "$prefix"
for private_header in core/EventLoop.h core/EventMultiplexer.h core/DescriptorEventPublisher.h core/TimerEventPublisher.h; do
    if [ -e "$prefix/include/snode.c/$private_header" ]; then
        echo "private header installed: $private_header" >&2
        exit 1
    fi
done
cat > "$work/consumer.cpp" <<'CPP'
#include <core/socket/stream/SocketServer.h>
#include <core/socket/stream/SocketClient.h>
#include <net/in/stream/legacy/SocketServer.h>
#include <net/in/stream/legacy/SocketClient.h>
#include <express/legacy/in/Server.h>

int main() {
    return 0;
}
CPP
c++ -std=c++20 -I"$prefix/include/snode.c" "$work/consumer.cpp" -L"$prefix/lib" -L"$prefix/lib/snode.c/web/http" -Wl,-rpath,"$prefix/lib:$prefix/lib/snode.c/web/http" -lsnodec-http-server-express-legacy-in -lsnodec-http-server-express -lsnodec-http-server -lsnodec-http -lsnodec-net-in-stream-legacy -lsnodec-net-in-stream -lsnodec-net-in-phy-stream -lsnodec-net-in-phy -lsnodec-net-in -lsnodec-net -lsnodec-core-socket-stream-legacy -lsnodec-core-socket-stream -lsnodec-core-socket -lsnodec-core -lsnodec-utils -lsnodec-logger -o "$work/consumer"
"$work/consumer"
echo "consumer compile command: c++ -std=c++20 -I$prefix/include/snode.c $work/consumer.cpp -L$prefix/lib -L$prefix/lib/snode.c/web/http -Wl,-rpath,$prefix/lib:$prefix/lib/snode.c/web/http -lsnodec-http-server-express-legacy-in -lsnodec-http-server-express -lsnodec-http-server -lsnodec-http -lsnodec-net-in-stream-legacy -lsnodec-net-in-stream -lsnodec-net-in-phy-stream -lsnodec-net-in-phy -lsnodec-net-in -lsnodec-net -lsnodec-core-socket-stream-legacy -lsnodec-core-socket-stream -lsnodec-core-socket -lsnodec-core -lsnodec-utils -lsnodec-logger -o $work/consumer"
