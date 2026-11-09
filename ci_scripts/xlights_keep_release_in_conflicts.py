#!/usr/bin/env python3
"""Resolve `git apply --3way` conflicts by keeping the release's side.

  python ci_scripts/xlights_keep_release_in_conflicts.py <file> [<file> ...]

Used by the xLights Cloud workflow for files that only the French
translation changes: when the xLights team changed the same lines after the
release tag, the release's lines are kept (they stay in English in that
build) and the rest of the translation applies as usual.
"""
import sys


def resolve(path):
    with open(path, encoding="utf-8", newline="") as f:
        lines = f.read().splitlines(keepends=True)
    out, state, blocks = [], None, 0
    for line in lines:
        if line.startswith("<<<<<<< "):
            state = "ours"
            blocks += 1
        elif line.startswith("||||||| ") and state == "ours":
            state = "base"
        elif line.rstrip("\r\n") == "=======" and state in ("ours", "base"):
            state = "theirs"
        elif line.startswith(">>>>>>> ") and state == "theirs":
            state = None
        elif state in (None, "ours"):
            out.append(line)
    if state is not None:
        raise SystemExit(f"{path}: unterminated conflict block")
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.writelines(out)
    return blocks


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(2)
    for p in sys.argv[1:]:
        print(f"{p}: kept the release's lines in {resolve(p)} conflict(s)")
