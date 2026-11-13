#!/bin/bash
# Builds the xLights Cloud AppImage inside the official build image
# (ghcr.io/xlightssequencer/xlights-build-docker), the way the official
# release workflow builds xLights.
#
# The image's /Recipe.appimage starts with ./Recipe, which clones the official
# repository and builds it. Here the source is already mounted at
# /xLights/xlights-git (the official tag with xLights Cloud applied), so that
# first step is replaced by a build of that folder; the rest of the recipe
# (make install, linuxdeploy with the GTK plugin) runs unchanged.
#
#   docker run -v <source>:/xLights/xlights-git -e XLIGHTS_VERSION=<version> \
#       <image> /bin/bash /xLights/xlights-git/build_scripts/linux/xlights_cloud_appimage.sh
#
# Output: /xLights/xLights/AppImage/xLights-<version>-x86_64.AppImage

set -e

SRC=/xLights/xlights-git
# The mounted checkout belongs to the runner's user, not root.
git config --global --add safe.directory '*'

# After make install, also copy ONNX Runtime into the AppDir: the binary links
# libonnxruntime.so.1 since 2026.20, but that release's Makefile does not
# install it (fixed after the tag, xLightsSequencer/xLights fe78fdddb) and
# linuxdeploy then fails. Harmless once the Makefile installs it itself.
ORT_COPY='if ls lib/linux/libonnxruntime.so* > /dev/null 2>\&1; then install -d -m 755 ${BASEDIR}/$APP/$APP.AppDir/usr/lib \&\& cp -fP lib/linux/libonnxruntime.so* ${BASEDIR}/$APP/$APP.AppDir/usr/lib/; fi'
sed -e 's|^\./Recipe$|(cd /xLights/xlights-git \&\& colormake-short -j"$(nproc)")|' \
    -e "/^make install DESTDIR=/a $ORT_COPY" \
    /Recipe.appimage > /tmp/Recipe.cloud
if ! grep -q 'cd /xLights/xlights-git && colormake-short' /tmp/Recipe.cloud; then
    echo "xlights_cloud_appimage: /Recipe.appimage no longer starts with ./Recipe, update this script" >&2
    exit 1
fi
if ! grep -q 'libonnxruntime' /tmp/Recipe.cloud; then
    echo "xlights_cloud_appimage: no 'make install DESTDIR=' line in /Recipe.appimage, update this script" >&2
    exit 1
fi

cd /
bash /tmp/Recipe.cloud

ls -l /xLights/xLights/AppImage/
