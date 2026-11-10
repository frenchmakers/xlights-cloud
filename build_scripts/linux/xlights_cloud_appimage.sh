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

sed 's|^\./Recipe$|(cd /xLights/xlights-git \&\& colormake-short -j"$(nproc)")|' /Recipe.appimage > /tmp/Recipe.cloud
if ! grep -q 'cd /xLights/xlights-git && colormake-short' /tmp/Recipe.cloud; then
    echo "xlights_cloud_appimage: /Recipe.appimage no longer starts with ./Recipe, update this script" >&2
    exit 1
fi

cd /
bash /tmp/Recipe.cloud

ls -l /xLights/xLights/AppImage/
