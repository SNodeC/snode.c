"""OpenWrt release metadata uses different package architectures for sifiveu."""
import json
from pathlib import Path


def main():
    document = json.loads((Path(__file__).resolve().parents[2] / "packages.json").read_text())
    matrix = {entry["id"]: entry for entry in document["matrix"]}
    for suite, architecture in [("24.10", "riscv64_riscv64"), ("25.12", "riscv64_generic")]:
        entry = matrix[f"openwrt-{suite}-riscv64_generic"]
        assert entry["arch"] == architecture, "package architecture must match official profiles.json"
        assert entry["build"]["target"] == "sifiveu/generic"
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
