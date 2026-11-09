#!/usr/bin/env python3
"""Translation catalog tool for xLights (no gettext install needed).

  python ci_scripts/xlights_i18n.py extract   # src-ui-wx -> resources/locale/xlights.pot
  python ci_scripts/xlights_i18n.py update    # merge the .pot into every <lang>/xlights.po
  python ci_scripts/xlights_i18n.py compile   # every <lang>/xlights.po -> <lang>/xlights.mo

The .po files can be edited with any gettext editor (Poedit, Weblate, ...).
The .mo files are committed so the builds need no extra tool.
"""

import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC_DIRS = ["src-ui-wx"]
LOCALE_DIR = os.path.join(ROOT, "resources", "locale")
POT = os.path.join(LOCALE_DIR, "xlights.pot")
DOMAIN = "xlights"

C_STR = r'"(?:[^"\\\n]|\\.)*"'
# _("...") and wxTRANSLATE("..."); adjacent literals are concatenated. _T("...") is not matched.
KEYWORD_RE = re.compile(r'(?<![\w.>])(?:_|wxTRANSLATE)\(\s*(' + C_STR + r'(?:\s*' + C_STR + r')*)\s*\)')
LITERAL_RE = re.compile(C_STR)

C_ESCAPES = {"n": "\n", "t": "\t", "r": "\r", '"': '"', "'": "'", "\\": "\\", "a": "\a", "b": "\b", "f": "\f", "v": "\v", "?": "?"}


def c_unescape(body):
    out, i = [], 0
    while i < len(body):
        ch = body[i]
        if ch != "\\":
            out.append(ch)
            i += 1
            continue
        nxt = body[i + 1]
        if nxt in C_ESCAPES:
            out.append(C_ESCAPES[nxt])
            i += 2
        elif nxt == "x":
            m = re.match(r"[0-9a-fA-F]+", body[i + 2:])
            out.append(chr(int(m.group(0), 16)))
            i += 2 + len(m.group(0))
        elif nxt in "uU":
            n = 4 if nxt == "u" else 8
            out.append(chr(int(body[i + 2:i + 2 + n], 16)))
            i += 2 + n
        elif nxt in "01234567":
            m = re.match(r"[0-7]{1,3}", body[i + 1:])
            out.append(chr(int(m.group(0), 8)))
            i += 1 + len(m.group(0))
        else:
            out.append(nxt)
            i += 2
    return "".join(out)


def po_escape(s):
    return s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n").replace("\t", "\\t").replace("\r", "\\r")


def po_quote(s):
    if "\n" in s[:-1]:
        parts = s.split("\n")
        lines = [p + "\n" for p in parts[:-1]] + ([parts[-1]] if parts[-1] else [])
        return '""\n' + "\n".join('"' + po_escape(l) + '"' for l in lines)
    return '"' + po_escape(s) + '"'


# ---------------------------------------------------------------- extract

def extract():
    found = {}  # msgid -> [refs]
    for d in SRC_DIRS:
        for dirpath, _, files in os.walk(os.path.join(ROOT, d)):
            for name in sorted(files):
                if not name.endswith((".cpp", ".h", ".mm")):
                    continue
                path = os.path.join(dirpath, name)
                rel = os.path.relpath(path, ROOT).replace("\\", "/")
                with open(path, encoding="utf-8", errors="replace") as f:
                    text = f.read()
                for m in KEYWORD_RE.finditer(text):
                    msgid = "".join(c_unescape(lit[1:-1]) for lit in LITERAL_RE.findall(m.group(1)))
                    if not msgid.strip():
                        continue
                    line = text.count("\n", 0, m.start()) + 1
                    found.setdefault(msgid, []).append(f"{rel}:{line}")
    extract_effect_metadata(found)
    os.makedirs(LOCALE_DIR, exist_ok=True)
    with open(POT, "w", encoding="utf-8", newline="\n") as f:
        f.write(header_block("", "") + "\n")
        for msgid in sorted(found, key=lambda k: found[k][0]):
            refs = found[msgid]
            shown = refs[:5] + ([f"... (+{len(refs) - 5})"] if len(refs) > 5 else [])
            f.write("#: " + " ".join(shown) + "\n")
            if "%" in msgid:
                f.write("#, c-format\n")
            f.write("msgid " + po_quote(msgid) + "\nmsgstr \"\"\n\n")
    print(f"{len(found)} strings -> {os.path.relpath(POT, ROOT)}")


# Keys of resources/effectmetadata/*.json shown by JsonEffectPanel on desktop.
# Property "description" texts are not shown on desktop (iPad / AI help), and
# "options" are values saved in the sequences, so neither is extracted.
EFFECT_TEXT_KEYS = ("label", "tooltip", "checkboxLabel", "fileMessage")


def extract_effect_metadata(found):
    import json
    base = os.path.join(ROOT, "resources", "effectmetadata")
    for dirpath, _, files in os.walk(base):
        for name in sorted(files):
            if not name.endswith(".json"):
                continue
            path = os.path.join(dirpath, name)
            rel = os.path.relpath(path, ROOT).replace("\\", "/")
            with open(path, encoding="utf-8") as f:
                try:
                    data = json.load(f)
                except ValueError:
                    continue
            texts = []
            if isinstance(data, dict) and isinstance(data.get("description"), str):
                texts.append(data["description"])  # panel-level text shown at the top

            def walk(node):
                if isinstance(node, dict):
                    for key, value in node.items():
                        if key in EFFECT_TEXT_KEYS and isinstance(value, str):
                            texts.append(value)
                        elif key != "options":
                            walk(value)
                elif isinstance(node, list):
                    for item in node:
                        walk(item)

            walk(data)
            for text in texts:
                if text.strip() and rel not in found.get(text, []):
                    found.setdefault(text, []).append(rel)


def header_block(lang, plural):
    hdr = ("Project-Id-Version: xLights\\n"
           "MIME-Version: 1.0\\n"
           "Content-Type: text/plain; charset=UTF-8\\n"
           "Content-Transfer-Encoding: 8bit\\n")
    if lang:
        hdr += f"Language: {lang}\\n"
    if plural:
        hdr += f"Plural-Forms: {plural}\\n"
    return 'msgid ""\nmsgstr ""\n' + "\n".join('"' + l + '\\n"' for l in hdr.split("\\n") if l) + "\n"


# ---------------------------------------------------------------- .po parsing

class Entry:
    def __init__(self):
        self.comments = []   # translator comments ("# ...")
        self.refs = []
        self.flags = set()
        self.msgid = None
        self.msgstr = ""
        self.obsolete = False


def po_unescape(s):
    return c_unescape(s)


def parse_po(path):
    entries, cur, field = [], Entry(), None

    def flush():
        nonlocal cur
        if cur.msgid is not None:
            entries.append(cur)
        cur = Entry()

    with open(path, encoding="utf-8") as f:
        for raw in f:
            line = raw.rstrip("\n")
            obsolete = line.startswith("#~")
            if obsolete:
                line = line[2:].lstrip()
            if not line.strip():
                flush()
                field = None
                continue
            if line.startswith("#,"):
                cur.flags.update(x.strip() for x in line[2:].split(","))
            elif line.startswith("#:"):
                cur.refs.append(line[2:].strip())
            elif line.startswith("#.") or line.startswith("#|"):
                pass
            elif line.startswith("#"):
                cur.comments.append(line)
            elif line.startswith("msgid "):
                if cur.msgid is not None:
                    flush()
                cur.obsolete = obsolete
                cur.msgid = po_unescape(line[6:].strip()[1:-1])
                field = "msgid"
            elif line.startswith("msgstr "):
                cur.msgstr = po_unescape(line[7:].strip()[1:-1])
                field = "msgstr"
            elif line.startswith('"'):
                val = po_unescape(line.strip()[1:-1])
                if field == "msgid":
                    cur.msgid += val
                elif field == "msgstr":
                    cur.msgstr += val
    flush()
    return entries


# ---------------------------------------------------------------- update

def languages():
    if not os.path.isdir(LOCALE_DIR):
        return []
    return sorted(d for d in os.listdir(LOCALE_DIR)
                  if os.path.isfile(os.path.join(LOCALE_DIR, d, DOMAIN + ".po")))


def update():
    template = [e for e in parse_po(POT) if e.msgid]
    for lang in languages():
        path = os.path.join(LOCALE_DIR, lang, DOMAIN + ".po")
        old = parse_po(path)
        header = next((e for e in old if e.msgid == ""), None)
        known = {e.msgid: e for e in old if e.msgid}
        done = 0
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            if header is not None:
                f.write("\n".join(header.comments) + ("\n" if header.comments else ""))
                f.write('msgid ""\nmsgstr ' + po_quote(header.msgstr) + "\n\n")
            for t in template:
                prev = known.pop(t.msgid, None)
                if prev is not None:
                    for c in prev.comments:
                        f.write(c + "\n")
                for r in t.refs:
                    f.write("#: " + r + "\n")
                flags = set(t.flags) | ({"fuzzy"} if prev is not None and "fuzzy" in prev.flags else set())
                if flags:
                    f.write("#, " + ", ".join(sorted(flags)) + "\n")
                msgstr = prev.msgstr if prev is not None else ""
                done += 1 if msgstr and "fuzzy" not in flags else 0
                f.write("msgid " + po_quote(t.msgid) + "\nmsgstr " + po_quote(msgstr) + "\n\n")
            for e in known.values():
                if e.msgstr:
                    f.write("#~ msgid " + po_quote(e.msgid).replace("\n", "\n#~ ") + "\n")
                    f.write("#~ msgstr " + po_quote(e.msgstr).replace("\n", "\n#~ ") + "\n\n")
        print(f"{lang}: {done}/{len(template)} translated")


# ---------------------------------------------------------------- compile

def compile_mo():
    for lang in languages():
        po = os.path.join(LOCALE_DIR, lang, DOMAIN + ".po")
        pairs = {}
        for e in parse_po(po):
            if e.obsolete or "fuzzy" in e.flags:
                continue
            if e.msgid == "" or e.msgstr:
                pairs[e.msgid] = e.msgstr
        keys = sorted(pairs, key=lambda k: k.encode("utf-8"))
        ids = [k.encode("utf-8") for k in keys]
        strs = [pairs[k].encode("utf-8") for k in keys]
        n = len(keys)
        orig_tab = 7 * 4
        trans_tab = orig_tab + n * 8
        data = trans_tab + n * 8
        offsets, blob = [], b""
        for s in ids:
            offsets.append((len(s), data + len(blob)))
            blob += s + b"\0"
        toffsets = []
        for s in strs:
            toffsets.append((len(s), data + len(blob)))
            blob += s + b"\0"
        out = struct.pack("<7I", 0x950412DE, 0, n, orig_tab, trans_tab, 0, 0)
        for l, o in offsets + toffsets:
            out += struct.pack("<2I", l, o)
        out += blob
        mo = os.path.join(LOCALE_DIR, lang, DOMAIN + ".mo")
        with open(mo, "wb") as f:
            f.write(out)
        print(f"{lang}: {n - 1} translations -> {os.path.relpath(mo, ROOT)}")


if __name__ == "__main__":
    cmds = {"extract": extract, "update": update, "compile": compile_mo}
    if len(sys.argv) != 2 or sys.argv[1] not in cmds:
        print(__doc__)
        sys.exit(2)
    cmds[sys.argv[1]]()
