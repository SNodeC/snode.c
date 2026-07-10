#!/bin/sh
# snodec-control - Out-of-tree companion tool for SNode.C applications
#
# A tiny, dependency-free stand-in for a real SNode.C application, used only for manual interactive UI
# smoke testing (see smoke-ui-notes.md in this directory). It is never built, packaged, or run by CTest;
# it exists purely so `--ui` can be exercised without a real SNode.C target on hand.
#
# It emits representative '-s' output covering every hierarchy derivation rule and every UI interaction
# this fix pass targets:
#   - application options (dot-less: log-level)
#   - one instance ("echoserver") with one section ("local")
#   - a required, initially-unset option (echoserver.local.port)
#   - a plain boolean option with no active value yet (feature.enabled, default false)
#   - a tristate option (feature.mode: false/true/default)
#
# Like every real SNode.C application, it exits non-zero for both '-s' and '--write-config' (writing a
# config file, or just describing one, never proceeds to actually running) - snodec-control treats that
# as expected, not an error, exactly as documented in the Phase 1/2 reports.
#
# When actually "run" (neither '-s' nor '--write-config'), it logs one line per second forever, so it
# stays alive long enough to manually press Ctrl-C against it (see smoke-ui-notes.md's Ctrl-C scenario).
# Set FAKE_TARGET_TICKS to a positive number of seconds to make it exit normally on its own instead, for
# testing the plain "target exits by itself" path without needing Ctrl-C at all.

SHOW_CONFIG='# Log level
#log-level=4
log-level=4

# Enable the echoserver instance
#echoserver.enabled=true
echoserver.enabled=true

# Local listen port (required)
#echoserver.local.port="<REQUIRED>"

# Local bind address
#echoserver.local.address=0.0.0.0
echoserver.local.address=0.0.0.0

# Plain boolean feature flag, currently unset (effective value comes from its default)
#feature.enabled=false

# Tristate feature: valid values are false, true, or "default" (let the target decide)
#feature.mode=default
feature.mode=default
'

case " $* " in
    *" -s "*|*" --show-config "*)
        printf '%s' "$SHOW_CONFIG"
        exit 1
        ;;
esac

write_config_path=""
prev=""
for arg in "$@"; do
    if [ "$prev" = "--write-config" ]; then
        write_config_path="$arg"
    fi
    prev="$arg"
done

if [ -n "$write_config_path" ]; then
    printf '# fake canonical config written by fake-snodec-target.sh\n' > "$write_config_path"
    exit 1
fi

# Exit code 130 is the conventional "killed by SIGINT" shell exit status (128 + signal 2), matching what
# a real SNode.C application (or any well-behaved POSIX program) reports when Ctrl-C stops it.
trap 'echo "fake-snodec-target: got SIGINT (Ctrl-C) - stopping"; exit 130' INT

echo "fake-snodec-target: running normally (this stands in for the real server loop)"
echo "fake-snodec-target: arguments: $*"
echo "fake-snodec-target: pid $$, press Ctrl-C to stop me"

i=0
while [ -z "$FAKE_TARGET_TICKS" ] || [ "$i" -lt "$FAKE_TARGET_TICKS" ]; do
    echo "fake-snodec-target: tick $i"
    i=$((i + 1))
    sleep 1
done

echo "fake-snodec-target: exiting normally after $i tick(s)"
exit 0
