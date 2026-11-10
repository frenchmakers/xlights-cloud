#!/bin/bash
# Checks an xLights Cloud AppImage on a machine without a screen (Xvfb), the
# way it is used for the French documentation:
#   - the French catalogs are where xLights looks for them;
#   - the cloud backup's libraries are bundled;
#   - xLights starts with the interface in French (settings language = fr);
#   - a screenshot of the window is kept.
#
#   xlights_cloud_smoke_test.sh <xLightsCloud.AppImage> <output dir>
#
# Needs: xvfb, imagemagick, libgl1, libegl1, libglu1-mesa (see the README).
# Runs in a throw-away HOME so it never touches real settings.

set -u
APPIMAGE=$(readlink -f "$1")
OUT=$(mkdir -p "$2" && readlink -f "$2")
WORK=$(mktemp -d)
export HOME="$WORK/home"
mkdir -p "$HOME/.config/xLights"
unset XDG_CONFIG_HOME XDG_STATE_HOME
fail=0
check() { if eval "$2"; then echo "ok    $1"; else echo "FAIL  $1"; fail=1; fi; }

cd "$WORK"
"$APPIMAGE" --appimage-extract > /dev/null
ROOT="$WORK/squashfs-root"

echo "== Files"
LOCALE="$ROOT/usr/share/xLights/locale/fr"
check "French xLights catalog: usr/share/xLights/locale/fr/xlights.mo" "[ -s '$LOCALE/xlights.mo' ]"
check "French wxWidgets catalog: usr/share/xLights/locale/fr/wxstd.mo" "[ -s '$LOCALE/wxstd.mo' ]"
check "effect metadata next to it (same resources folder)" "[ -d '$ROOT/usr/share/xLights/effectmetadata' ]"
check "libcurl bundled" "ls '$ROOT'/usr/lib/libcurl.so* > /dev/null 2>&1"
missing=$(LD_LIBRARY_PATH="$ROOT/usr/lib" ldd "$ROOT/usr/bin/xLights" | grep "not found" || true)
check "every library of usr/bin/xLights resolves${missing:+ (missing: $missing)}" "[ -z \"\$missing\" ]"
if [ -e /usr/lib/x86_64-linux-gnu/libsecret-1.so.0 ]; then
    echo "info  libsecret is installed on this machine (it is loaded at run time, not bundled)"
else
    echo "info  libsecret is not installed on this machine: the cloud secret key goes to the settings file"
fi

echo "== Start in French under Xvfb"
printf '{\n  "main": {\n    "xLightsUILanguage": "fr"\n  }\n}\n' > "$HOME/.config/xLights/settings.json"
Xvfb :99 -screen 0 1920x1080x24 -nolisten tcp > "$WORK/xvfb.log" 2>&1 &
XVFB=$!
sleep 2
LOG="$HOME/.local/state/xLights/xLights_spdlog.log"
DISPLAY=:99 "$ROOT/AppRun" > "$WORK/xlights.out" 2>&1 &
APP=$!
for _ in $(seq 1 90); do
    grep -q "UI language:" "$LOG" 2> /dev/null && break
    kill -0 $APP 2> /dev/null || break
    sleep 1
done
# Let the main window (and whatever first-run dialog) draw before the screenshot.
sleep 30
DISPLAY=:99 import -window root "$OUT/xlights-cloud-fr.png" 2> /dev/null || echo "info  no screenshot (imagemagick missing?)"
check "xLights is still running after start-up" "kill -0 $APP 2> /dev/null"
check "French catalog loaded (log: UI language: 'fr' (xlights catalog loaded))" "grep -q \"UI language: 'fr' (xlights catalog loaded)\" '$LOG'"
grep -h "UI language:" "$LOG" 2> /dev/null | tail -1

kill $APP 2> /dev/null; sleep 2; kill -9 $APP 2> /dev/null
kill $XVFB 2> /dev/null
cp "$LOG" "$OUT/xLights_spdlog.log" 2> /dev/null
cp "$WORK/xlights.out" "$OUT/xlights-stdout.log" 2> /dev/null
rm -rf "$WORK"

[ $fail -eq 0 ] && echo "== All checks passed" || echo "== Some checks failed"
exit $fail
