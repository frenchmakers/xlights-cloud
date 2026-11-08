"""Write xLights_Cloud_64bit.iss from the official xLights_4_64bit_VS.iss.

xLights Cloud installs next to the official xLights instead of over it:
its own Inno AppId, folder, Start menu group and desktop icon. It leaves the
.xsq/.xsqz file associations to the official xLights, and its uninstaller
keeps the xLights settings both apps share (HKCU\\Software\\Xlights).

Usage: python xlights_cloud_installer.py <version>     e.g. 2026.19

Each replacement must match exactly once, so a change in the official script
stops the build here instead of producing a wrong installer.
"""
import re
import sys

SRC = "xLights_4_64bit_VS.iss"
DST = "xLights_Cloud_64bit.iss"
NAME = "xLights Cloud"


def replace_once(text, pattern, repl, what):
    new, count = re.subn(pattern, lambda _: repl, text, flags=re.M)
    if count != 1:
        sys.exit(f"xlights_cloud_installer: expected one '{what}' in {SRC}, found {count}")
    return new


def main():
    if len(sys.argv) != 2 or not re.fullmatch(r"\d{4}\.\d+(\.\d+)?", sys.argv[1]):
        sys.exit("usage: xlights_cloud_installer.py <version, e.g. 2026.19>")
    version = sys.argv[1]

    with open(SRC, encoding="utf-8-sig") as f:
        text = f.read()

    text = replace_once(text, r"^AppName=.*$", f"AppId=xLightsCloud\nAppName={NAME}", "AppName")
    text = replace_once(text, r"^AppVersion=.*$", f"AppVersion={version}", "AppVersion")
    text = replace_once(text, r"^DefaultDirName=.*$", f"DefaultDirName={{commonpf64}}\\{NAME}", "DefaultDirName")
    text = replace_once(text, r"^DefaultGroupName=.*$", f"DefaultGroupName={NAME}", "DefaultGroupName")
    text = replace_once(text, r"^OutputBaseFilename=.*$", f"OutputBaseFilename=xLightsCloud64_{version}", "OutputBaseFilename")
    text = replace_once(text, r"^ChangesAssociations\s*=.*$", "ChangesAssociations=no", "ChangesAssociations")
    text = replace_once(text, r'^Name: "\{group\}\\xLights64";', f'Name: "{{group}}\\{NAME}";', "Start menu icon")
    text = replace_once(text, r'^Name: "\{commondesktop\}\\xLights64";', f'Name: "{{commondesktop}}\\{NAME}";', "desktop icon")

    # Drop the whole [Registry] section: file associations and the uninstall
    # flag that would delete the settings shared with the official xLights.
    text = replace_once(text, r"^\[Registry\]\n(?:(?!^\[).*\n?)*", "", "[Registry] section")

    with open(DST, "w", encoding="utf-8") as f:
        f.write(text)
    print(f"Wrote {DST} for {NAME} {version}")


if __name__ == "__main__":
    main()
