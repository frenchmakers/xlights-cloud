#!/usr/bin/env python3
"""Check a translation catalog against the sources it ships with.

  python ci_scripts/xlights_i18n_check.py <lang> <previous.po> <report.md>

Run after `xlights_i18n.py extract` and `update` on the sources of a release:

  - New texts: msgids missing from <previous.po> (the catalog before update)
    that have no translation yet. They show in English until translated.
  - Risky translations: translated msgids that the code reads back or compares
    as English text (see find_label_keys). A translation there breaks the
    feature silently, so it is cleared in <lang>/xlights.po (run `compile`
    afterwards) and listed in the report.

The report is Markdown, empty when there is nothing to say.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import xlights_i18n as X  # noqa: E402

SRC = os.path.join(X.ROOT, "src-ui-wx")

# Flagged by the scan but safe to translate: the code compares an internal
# type name that happens to be spelled like the label.
KNOWN_SAFE = {
    "Controller": "PixelTestDialog compares its own test type, not the tab label",
    "SubModel": "SubModelsPanel compares a drag-and-drop type, not the column label",
}

STR = r'"((?:[^"\\]|\\.)*)"'
READERS = re.compile(r"(\w+)(?:->|\.)(?:GetStringSelection|SetStringSelection|GetString|FindString|GetValue|SetValue"
                     r"|GetLabel|GetLabelText|GetItemLabelText|GetPageText|GetChoiceCtrl\(\)->GetStringSelection)\s*\(")
APPEND = re.compile(r"(\w+)(?:->|\.)(?:Append|AppendString|Insert|Add)\s*\(\s*_\(" + STR + r"\)")
COMPARE = re.compile(r"(?:==|!=)\s*_\(" + STR + r"\)|_\(" + STR + r"\)\s*(?:==|!=)")
CFGREAD = re.compile(r"Read\s*\([^;]*?_\(" + STR + r"\)")


def find_label_keys(root=SRC):
    """msgid -> reasons, for texts the code uses as English keys."""
    found = {}

    def add(text, why):
        found.setdefault(X.c_unescape(text), set()).add(why)

    for dirpath, _, files in os.walk(root):
        for fn in files:
            if not fn.endswith((".cpp", ".h", ".mm")):
                continue
            path = os.path.join(dirpath, fn)
            rel = os.path.relpath(path, X.ROOT).replace("\\", "/")
            with open(path, encoding="utf-8", errors="replace") as f:
                src = f.read()
            readers = set(READERS.findall(src))
            for ctrl, text in APPEND.findall(src):
                if ctrl in readers:
                    add(text, f"{rel}: {ctrl} read back by its label")
            if re.search(r"Get(?:Row|Col)LabelValue\s*\(", src):
                for s in re.findall(r"Set(?:Row|Col)LabelValue\s*\([^;]*?_\(" + STR + r"\)", src):
                    add(s, f"{rel}: grid label read back")
            if re.search(r"GetItemText\s*\(|GetItemLabel\s*\(", src):
                for s in re.findall(r"(?:InsertItem|AppendItem|SetItemText|AddRoot|PrependItem)\s*\([^;]*?_\(" + STR + r"\)", src):
                    add(s, f"{rel}: tree/list item read back")
            for s in re.findall(r"GetPropertyByLabel\s*\(\s*" + STR, src):
                add(s, f"{rel}: property found by its label")
            translated_here = set(re.findall(r"_\(" + STR + r"\)", src))
            literals = (re.findall(r"(?:==|!=)\s*(?:wxString\()?" + STR, src)
                        + re.findall(STR + r"\s*(?:==|!=)", src)
                        + re.findall(r"(?:FindString|SetStringSelection|StartsWith|EndsWith|Contains|Matches|IsSameAs)\s*\(\s*" + STR, src))
            for lit in literals:
                if lit in translated_here:
                    add(lit, f"{rel}: compared with the English text")
            for a, b in COMPARE.findall(src):
                add(a or b, f"{rel}: compared")
            for s in CFGREAD.findall(src):
                add(s, f"{rel}: default value saved in the settings")
    return found


def main():
    lang, previous, report = sys.argv[1], sys.argv[2], sys.argv[3]
    po_path = os.path.join(X.LOCALE_DIR, lang, X.DOMAIN + ".po")
    before = {e.key for e in X.parse_po(previous) if e.msgid}
    entries = X.parse_po(po_path)
    keys = find_label_keys()

    new = [e for e in entries if e.msgid and not e.obsolete and not e.msgstr and e.key not in before]
    # A text with a context (wxTRANSLATE_IN_CONTEXT) is only shown where it is
    # marked, so the English key used elsewhere doesn't apply to it.
    risky = [e for e in entries if e.msgid and not e.obsolete and e.msgstr and e.msgctxt is None
             and e.msgid in keys and e.msgid not in KNOWN_SAFE]
    for e in risky:
        e.msgstr = ""
    if risky:
        X.write_po(po_path, entries)

    def show(text):
        text = text.replace("\n", "\\n").replace("`", "'")
        return text if len(text) <= 120 else text[:117] + "..."

    lines = []
    if risky:
        lines.append(f"### Traductions désactivées ({len(risky)})\n")
        lines.append("Le code de cette version compare ces textes à l'anglais : leur traduction est retirée "
                     "de cette version pour ne pas casser la fonction. Gardez-les en anglais dans "
                     f"`resources/locale/{lang}/xlights.po` (et notez-le dans `GLOSSARY.md`).\n")
        for e in risky:
            lines.append(f"- `{show(e.msgid)}` : {'; '.join(sorted(keys[e.msgid]))}")
        lines.append("")
    if new:
        lines.append(f"### Nouveaux textes à traduire ({len(new)})\n")
        lines.append("Ils s'affichent en anglais en attendant. Traduisez-les dans "
                     f"`resources/locale/{lang}/xlights.po` après `xlights_i18n.py extract` et `update`.\n")
        for e in new[:150]:
            where = e.refs[0].split()[0] if e.refs else ""
            lines.append(f"- `{show(e.msgid)}` ({where})")
        if len(new) > 150:
            lines.append(f"- ... et {len(new) - 150} autres")
        lines.append("")
    with open(report, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))
    print(f"{lang}: {len(new)} new texts, {len(risky)} risky translations cleared")


if __name__ == "__main__":
    main()
