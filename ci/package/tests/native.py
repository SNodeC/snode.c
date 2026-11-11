"""Exercise the source-owned native hook with tool stubs and inspect its build request."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    hook = Path(__file__).resolve().parents[1] / "native.sh"
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        target = root / "target.json"
        target.write_text(json.dumps({"build": {"format": "deb"}}))
        for name in ("cmake", "ctest", "dpkg-architecture"):
            tool = root / name
            tool.write_text('#!/bin/sh\nprintf "%s\\n" "' + name + ' $*"\n')
            tool.chmod(0o755)
        (root / 'nproc').write_text('#!/bin/sh\nprintf "4\\n"\n')
        (root / 'nproc').chmod(0o755)
        python = root / "python3"
        python.write_text('#!/bin/sh\nif [ "$1" = /tool/native_container.py ]; then exit 0; fi\nexec ' + sys.executable + ' "$@"\n')
        python.chmod(0o755)
        result = subprocess.run(["/bin/sh", str(hook)], check=True, capture_output=True, text=True,
                                env={**os.environ, "PATH": str(root)+":"+os.environ["PATH"],
                                     "SOURCE_DIR": str(root), "TARGET_JSON": str(target), "OUTPUT_DIR": str(root),
                                     "CI_DIR": str(hook.parents[2])})
        assert "cmake --build /work/build-product --parallel 4" in result.stdout
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
