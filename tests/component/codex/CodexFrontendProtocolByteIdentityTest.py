#!/usr/bin/env python3
"""Guard the pre-A1 Frontend Protocol v1 schema bytes."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path


PHASE_A0_SHA256 = "a27721164607b79a8b268c3adb035211a6efa82cb4645632b9b9a59302734c04"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--schema", type=Path, required=True)
    arguments = parser.parse_args()

    digest = hashlib.sha256(arguments.schema.read_bytes()).hexdigest()
    if digest != PHASE_A0_SHA256:
        raise SystemExit(
            "Frontend Protocol v1 schema bytes changed during A1.0: "
            f"expected {PHASE_A0_SHA256}, got {digest}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
