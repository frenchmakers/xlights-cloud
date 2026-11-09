# xLights translations

The desktop UI uses wxWidgets' gettext support. Strings marked `_("...")` in
`src-ui-wx/` are looked up in `locale/<lang>/xlights.mo` at startup. The
language is chosen in **Preferences > Other > Language** and applies after a
restart. When the setting is unset the UI stays in English, so a partial
translation is never switched on by itself.

| File | Purpose |
|---|---|
| `xlights.pot` | Template with every marked string (generated) |
| `<lang>/xlights.po` | Translations for one language (edit this) |
| `<lang>/xlights.mo` | Compiled catalog loaded by the app (generated, committed so builds need no tool) |
| `<lang>/wxstd.po` / `wxstd.mo` | wxWidgets' own strings (stock buttons, progress and print dialogs...), copied unchanged from `locale/<lang>.po` of the wxWidgets build xLights uses |

## Workflow

```bash
python ci_scripts/xlights_i18n.py extract   # rescan src-ui-wx -> xlights.pot
python ci_scripts/xlights_i18n.py update    # add new strings to every <lang>/xlights.po
python ci_scripts/xlights_i18n.py compile   # <lang>/xlights.po -> <lang>/xlights.mo
```

French terms are fixed in `fr/GLOSSARY.md`; follow it so a word is always
translated the same way.

Edit the `.po` with any gettext editor (Poedit, Weblate, a text editor). Keep
the `\t...` accelerator suffix of menu items unchanged
(`"Save\tCTRL-S"` -> `"Enregistrer\tCTRL-S"`). Entries marked `fuzzy` or left
empty fall back to English.

To add a language: create `<lang>/xlights.po` with a header (copy the one in
`fr/xlights.po`), run `update` and `compile`, and add the language to
`UILanguage::Available()` in `src-ui-wx/settings/UILanguage.cpp`.

## Release check (xLights Cloud)

For each official release, the xLights Cloud workflow runs `extract` and `update` on that
release's sources, then `ci_scripts/xlights_i18n_check.py`. It lists the new texts to translate
and clears any translation the code now reads back or compares as English (those would break a
feature silently), then reports both in the issue "Textes de l'interface à traduire".

## wxWidgets catalog

`wxstd.po` comes from the xLightsSequencer/wxWidgets fork (`locale/fr.po`, wxWidgets 3.3).
It is loaded before `xlights`, so a string both catalogs know uses the xLights
translation. When xLights moves to a new wxWidgets version, copy that version's
`locale/<lang>.po` over `wxstd.po` and rebuild `wxstd.mo` with a gettext tool that
handles plurals and contexts (`msgfmt`, Poedit, or Python `polib`):

```bash
python -c "import polib; polib.pofile('resources/locale/fr/wxstd.po').save_as_mofile('resources/locale/fr/wxstd.mo')"
```

## Rules for code

- **Only translate what is displayed.** Anything written to `.xsq`,
  `xlights_rgbeffects.xml`, `xlights_networks.xml` or the settings store (model
  type names, effect names, `E_`/`T_`/`B_`/`C_` keys, choice values) must stay
  English.
- **Do not save a translated label.** Several panels store
  `wxChoice::GetStringSelection()` and restore it with `SetStringSelection()`.
  If the items are translated, the saved value changes with the language and
  no longer matches on the next start. Save the index or a fixed English key
  instead before translating those items. Until a panel is fixed, leave its
  choice items untranslated in the `.po` files.
- **Do not compare against a translated label.** Look a page or item up by an
  id, or compare with `wxGetTranslation(englishName)`, as
  `xLightsPreferences.cpp` does for the initial page.
- A msgid is shared by every place that uses the same English text, so a
  translation applies to all of them at once. When one of those places reads
  the text back as English (the release check then clears the translation),
  mark the places where it is only displayed with
  `wxTRANSLATE_IN_CONTEXT("context", "text")` and look it up with
  `wxGetTranslation(text, wxString(), "context")`: they get their own entry
  (`msgctxt`) in the `.po`. The model property choices use the context
  `model property` (`TranslatedChoices()` in `PropertyGridHelpers.cpp`).
- `src-core/` cannot use wx and has no translation hook yet. Messages that
  come from the core stay in English.
- The C runtime locale is not changed, so number parsing keeps using `.`.

## Not translated yet

- Most dialogs, message boxes (many `wxMessageBox("...")` calls are not marked)
  and property grids (model/object properties).
- Effect panels built from `resources/effectmetadata/*.json`.

- The iPad app (it would use an Xcode String Catalog).
