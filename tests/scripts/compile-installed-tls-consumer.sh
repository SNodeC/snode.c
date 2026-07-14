#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: $0 <installed-prefix>" >&2
    exit 2
fi

prefix=$1
include_dir="${prefix}/include/snode.c"
lib_dir="${prefix}/lib"
if [[ ! -d "${include_dir}" ]]; then
    echo "missing installed include directory: ${include_dir}" >&2
    exit 1
fi
if find "${include_dir}" -path '*/core/socket/stream/tls/detail/TLSResult.h' -print -quit | grep -q .; then
    echo "detail/TLSResult.h must not be installed" >&2
    exit 1
fi

tmp_dir=$(mktemp -d)
trap 'rm -rf "${tmp_dir}"' EXIT
cat > "${tmp_dir}/consumer.cpp" <<'CPP'
#include <core/socket/stream/tls/TLSHandshake.h>
#include <core/socket/stream/tls/TLSShutdown.h>

int main() {
    return 0;
}
CPP

c++ -std=c++20 "${tmp_dir}/consumer.cpp" \
    -I"${include_dir}" \
    -L"${lib_dir}" \
    -Wl,-rpath,"${lib_dir}" \
    -lsnodec-core-socket-stream-tls \
    -o "${tmp_dir}/consumer"
"${tmp_dir}/consumer"
