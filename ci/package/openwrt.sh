#!/bin/sh
set -eu
: "${SOURCE_DIR:?}" "${SDK_DIR:?}" "${SDK_STAGING_DIR:?}" "${SOURCE_TAG:?}"
mkdir -p /work/recipes/snode.c "$SDK_DIR/dl"
cp -a "$SOURCE_DIR/supplement/openwrt/." /work/recipes/snode.c/
export SNODEC_PACKAGE_RELEASE="$PACKAGE_REVISION"
export SNODEC_SOURCE_TAG="$SOURCE_TAG"
export SNODEC_PACKAGE_VERSION="$PACKAGE_UPSTREAM_VERSION"
archive="$SDK_DIR/dl/snode.c-$PACKAGE_UPSTREAM_VERSION.tar.gz"
tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner \
    --transform="s,^\.,snode.c-$PACKAGE_UPSTREAM_VERSION," -czf "$archive" -C "$SOURCE_DIR" .
SNODEC_SOURCE_HASH=$(sha256sum "$archive" | cut -d ' ' -f1)
export SNODEC_SOURCE_HASH
cd "$SDK_DIR"
printf '\nsrc-link core_ci /work/recipes\n' >> feeds.conf.default
./scripts/feeds update -a
./scripts/feeds install -a
printf '\nCONFIG_PACKAGE_snodec=m\n' >> .config
make defconfig
make -j2 package/snode.c/compile CONFIG_AUTOREMOVE= V=s BUILD_LOG=1
python3 "$CI_DIR/ci/package/openwrt-tests.py" "$SDK_DIR"
python3 - <<'EXPORT'
import os,shutil,tarfile
from pathlib import Path
sdk=Path(os.environ['SDK_DIR'])
staging=Path(os.environ['SDK_STAGING_DIR'])
root=Path('/work/development')
root.mkdir()
listing=staging/'packages/snode.c.list'
assert listing.is_file(), 'SDK did not record the selected project staging inventory'
for line in listing.read_text().splitlines():
    line=line.lstrip('/')
    assert '..' not in Path(line).parts, 'unsafe SDK staging inventory path'
    relative=Path(line).relative_to('usr') if line.startswith('usr/') else None
    if relative is not None and relative.parts and relative.parts[0] in ('include','lib','share'):
        source=staging/line
        destination=root/relative
        if source.is_dir() and not source.is_symlink():
            destination.mkdir(parents=True,exist_ok=True)
        elif source.exists() or source.is_symlink():
            destination.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(source,destination,follow_symlinks=False)
shutil.copytree('/work/recipes/snode.c',root/'share/core-ci/openwrt/snode.c')
metadata=root/'share/core-ci/pkginfo'
metadata.mkdir()
for file in sorted((staging/'pkginfo').glob('snodec*.provides')):
    shutil.copy2(file,metadata/file.name)
with tarfile.open(Path(os.environ['OUTPUT_DIR'])/'development.tar.gz','w:gz') as archive:
    for file in sorted(root.iterdir()):
        archive.add(file,arcname=file.name)
EXPORT
python3 /tool/openwrt_container.py --outputs
