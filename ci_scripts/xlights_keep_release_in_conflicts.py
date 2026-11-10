#!/usr/bin/env python3
"""Resolve `git apply --3way` conflicts by keeping the release's side.

  python ci_scripts/xlights_keep_release_in_conflicts.py <file> [<file> ...]
  python ci_scripts/xlights_keep_release_in_conflicts.py --check <base> <head> <file> [<file> ...]

Used by the xLights Cloud workflow when the xLights team changed, after the
release tag, lines that the branch also changes. Keeping the release's lines
there is safe when the branch's side of every conflict only holds the
translation (or code the branch got from the official master): those texts
stay in English in that build and the rest applies as usual.

--check lists the conflicts whose branch side contains a line added by one of
the branch's own commits other than the translation ones (cloud backup and
other features), and exits with 1 if there is any: those must be fixed by
hand. It does not change the files.
"""
import re
import subprocess
import sys

TRANSLATION_SUBJECT = re.compile(r"^(French translation|Translation (tools|docs))")


def conflict_blocks(lines):
    """Yield (start line, ours lines, theirs lines) for each conflict block."""
    state, ours, theirs, start = None, [], [], 0
    for n, line in enumerate(lines, 1):
        if line.startswith("<<<<<<< "):
            state, ours, theirs, start = "ours", [], [], n
        elif line.startswith("||||||| ") and state == "ours":
            state = "base"
        elif line.rstrip("\r\n") == "=======" and state in ("ours", "base"):
            state = "theirs"
        elif line.startswith(">>>>>>> ") and state == "theirs":
            yield start, ours, theirs
            state = None
        elif state == "ours":
            ours.append(line)
        elif state == "theirs":
            theirs.append(line)
    if state is not None:
        raise SystemExit("unterminated conflict block")


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


def git(*args):
    return subprocess.run(["git", *args], check=True, capture_output=True, text=True,
                          encoding="utf-8", errors="replace").stdout


def feature_lines(base, head, path):
    """Lines added to path by the branch's own non-translation commits."""
    added = set()
    for entry in git("log", "--no-merges", "--format=%H %s", f"{base}..{head}", "--", path).splitlines():
        sha, _, subject = entry.partition(" ")
        if TRANSLATION_SUBJECT.match(subject):
            continue
        for line in git("show", "--format=", "--unified=0", sha, "--", path).splitlines():
            if line.startswith("+") and not line.startswith("+++"):
                text = line[1:].strip()
                if len(text) > 3:
                    added.add(text)
    return added


def check(base, head, paths):
    problems = []
    for path in paths:
        with open(path, encoding="utf-8", newline="") as f:
            lines = f.read().splitlines(keepends=True)
        added = feature_lines(base, head, path)
        for start, _ours, theirs in conflict_blocks(lines):
            hit = [t.strip() for t in theirs if t.strip() in added]
            if hit:
                problems.append(f"{path}:{start}: {hit[0][:120]}")
    for p in problems:
        print(p)
    return 1 if problems else 0


if __name__ == "__main__":
    args = sys.argv[1:]
    if len(args) >= 4 and args[0] == "--check":
        sys.exit(check(args[1], args[2], args[3:]))
    if not args or args[0].startswith("--"):
        print(__doc__)
        sys.exit(2)
    for p in args:
        print(f"{p}: kept the release's lines in {resolve(p)} conflict(s)")
