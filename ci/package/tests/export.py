"""Exercise the hook's export block with the SDK's find ./ inventory format."""

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile


def main():
    hook = (Path(__file__).resolve().parents[1] / "openwrt.sh").read_text()
    block = hook.split("python3 - <<'EXPORT'\n", 1)[1].split("\nEXPORT\n", 1)[0]
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        staging, output, recipe = root / "sdk/staging", root / "output", root / "recipe"
        files = {"usr/include/snodec/example.h": b"header", "usr/lib/cmake/snodec/snodecConfig.cmake": b"config",
                 "usr/lib/libsnodec.so.2": b"library", "usr/share/snodec/data": b"data",
                 "pkginfo/snodec.provides": b"libsnodec.so.2", "bin/not-development": b"excluded"}
        for name, data in files.items():
            path = staging / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        (staging / "usr/lib/libsnodec.so").symlink_to("libsnodec.so.2")
        inventory = ["./", "./usr", "./usr/include", *["./" + name for name in files], "./usr/lib/libsnodec.so"]
        (staging / "packages").mkdir()
        listing = staging / "packages/snode.c.list"
        listing.write_text("\n".join(inventory) + "\n")
        output.mkdir()
        recipe.mkdir()
        (recipe / "Makefile").write_text("recipe")
        script = block.replace("/work/development", str(root / "export")).replace("/work/recipes/snode.c", str(recipe))
        environment = dict(os.environ, SDK_DIR=str(root / "sdk"), SDK_STAGING_DIR=str(staging), OUTPUT_DIR=str(output))
        subprocess.run([sys.executable, "-c", script], env=environment, check=True)
        with tarfile.open(output / "development.tar.gz") as archive:
            for name, data in files.items():
                if name.startswith("usr/"):
                    assert archive.extractfile(name[4:]).read() == data, name
            assert archive.getmember("lib/libsnodec.so").issym()
            assert archive.getmember("lib/libsnodec.so").linkname == "libsnodec.so.2"
            assert archive.extractfile("share/core-ci/pkginfo/snodec.provides").read() == b"libsnodec.so.2"
            assert archive.extractfile("share/core-ci/openwrt/snode.c/Makefile").read() == b"recipe"
            assert "bin/not-development" not in archive.getnames()
        # An incomplete inventory must fail before creating an export archive.
        listing.write_text("\n".join(line for line in inventory if not line.endswith("snodecConfig.cmake")) + "\n")
        shutil.rmtree(root / "export")
        (output / "development.tar.gz").unlink()
        result = subprocess.run([sys.executable, "-c", script], env=environment, capture_output=True)
        assert result.returncode != 0 and \
            b"incomplete development export: missing snodec CMake configuration, headers or libraries" in result.stderr
        assert not (output / "development.tar.gz").exists()
        # Normalization must not allow parent traversal in a corrupt inventory.
        listing.write_text("./usr/../../outside\n")
        shutil.rmtree(root / "export")
        result = subprocess.run([sys.executable, "-c", script], env=environment, capture_output=True)
        assert result.returncode != 0 and b"unsafe SDK staging inventory path" in result.stderr
    print("OpenWrt export: ./ inventory retains headers, libraries, CMake config and metadata; traversal rejected")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
