#!/usr/bin/env python3
"""Write the notes of an xLights Cloud release.

  python ci_scripts/xlights_cloud_release_notes.py <version> <out.md>

Reads from the environment: GH_REPO (this repository), UPSTREAM_REPO,
GITHUB_SHA, SIGN ("true" when the Windows installer is signed) and LINUX
("true" when the release has the Linux AppImage).
"""
import os
import sys


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(2)
    version, out = sys.argv[1], sys.argv[2]
    repo = os.environ["GH_REPO"]
    upstream = os.environ["UPSTREAM_REPO"]
    sha = os.environ["GITHUB_SHA"]
    signed = os.environ.get("SIGN") == "true"
    linux = os.environ.get("LINUX") == "true"

    if signed:
        signing = ("Signed by SYNAPSE DEVELOPPEMENT (FrenchMakers). Windows SmartScreen may still ask "
                   "for confirmation while this new publisher builds its reputation.")
    else:
        signing = ("The installer is not code-signed, so Windows SmartScreen warns before running it "
                   "(More info > Run anyway).")

    lines = [
        f"**xLights Cloud {version}** is the official [xLights {version}](https://github.com/{upstream}/releases/tag/{version}) "
        "plus **File > Backup to Cloud** and **File > Restore from Cloud** (Cloudflare R2 or any S3-compatible storage, "
        "set up in Preferences > Cloud Storage), with a French interface (Preferences > Other > Language).",
        "",
        f"- **Windows 64-bit** (`xLightsCloud64_{version}.exe`): installs next to the official xLights, in "
        "*C:\\\\Program Files\\\\xLights Cloud*, and shares its settings. `.xsq` files still open with the official xLights. "
        + signing,
    ]
    if linux:
        lines.append(
            f"- **Linux x86_64** (`xLightsCloud-{version}-x86_64.AppImage`): built like the official AppImage. "
            "Make it executable and run it; it shares the settings of the official xLights. Without a keyring service "
            "(e.g. a machine without a desktop) the cloud secret key is kept in the xLights settings file. "
            "See the [README](https://github.com/" + repo + "#download) for running it without a screen.")
    lines += [
        f"- Not made or supported by the xLights team: report problems [here](https://github.com/{repo}/issues), "
        "not to the xLights project. Crash reports stay on your computer.",
        "",
        f"Built from the official tag `{version}` with [`{sha[:9]}`](https://github.com/{repo}/commit/{sha}) "
        "of this repository applied on top.",
    ]
    with open(out, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
