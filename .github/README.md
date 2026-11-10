# xLights Cloud

**xLights Cloud** is the official [xLights](https://github.com/xLightsSequencer/xLights) with two more
commands in the File menu:

- **Backup to Cloud** uploads the whole show folder (media, rendered `.fseq` files and a layout
  thumbnail included) to Cloudflare R2 or any S3-compatible storage, in the background with progress.
  It can keep only the last N backups of each show.
- **Restore from Cloud** lists the backups in the bucket, checks the one you pick and brings it back
  into a new folder. Old backups can be deleted from there.

Set it up in **Preferences > Cloud Storage**. Cloudflare R2 is free up to 10 GB of storage.

xLights Cloud also includes a **French translation** of the interface (see below, in French).

## Version française intégrée

xLights Cloud contient une traduction française de l'interface : menus, menus contextuels (clic
droit), fenêtres, panneaux, propriétés des modèles et des contrôleurs, messages et préférences.

**Pour l'activer :**

1. Ouvrez **File > Preferences...** (ou `Ctrl+,`).
2. Dans la page **Other**, choisissez **Français** dans la liste **Language (restart required)**.
3. Cliquez sur **OK**, puis fermez et relancez xLights Cloud.

Pour revenir à l'anglais, choisissez **English** au même endroit (en français : **Fichier >
Préférences... > Autres > Langue**). Tant qu'aucune langue n'est choisie, l'interface reste en
anglais. Le réglage ne change rien à vos fichiers : séquences, show et préférences restent
compatibles avec le xLights officiel.

**Ce qui reste en anglais, volontairement :**

- les noms des effets (Bars, Butterfly, Shimmer…) et des types de modèles, connus sous ces noms
  dans la communauté ;
- les valeurs enregistrées dans les séquences (options des effets, styles de rendu…) ;
- les noms que vous avez donnés vous-même (modèles, vues, perspectives) ;
- quelques messages produits par le cœur commun à xLights et à l'application iPad (test des pixels,
  description des sorties).

Un texte en anglais qui devrait être traduit, ou une traduction maladroite ? Signalez-le dans les
[issues](../../issues), avec une capture d'écran si possible.

Les détails techniques de la traduction (fichiers du catalogue, outils, règles pour le code) sont
dans [`resources/locale/README.md`](../resources/locale/README.md), et les choix de vocabulaire
dans le [glossaire français](../resources/locale/fr/GLOSSARY.md).

## Download

Get it from the [Releases](../../releases) page. There is one xLights Cloud release for each
official xLights release, built from that exact version with the cloud backup and the French
translation added. Help > Check for Updates looks for new xLights Cloud releases here.

### Windows 64-bit

`xLightsCloud64_<version>.exe`

- It installs next to the official xLights (in `C:\Program Files\xLights Cloud`) and shares its
  settings; `.xsq` files keep opening with the official xLights.
- The installer is not code-signed: Windows SmartScreen asks for confirmation (More info > Run anyway).

### Linux x86_64 (AppImage)

`xLightsCloud-<version>-x86_64.AppImage`, built the same way as the official xLights AppImage.

```bash
chmod +x xLightsCloud-*-x86_64.AppImage
./xLightsCloud-*-x86_64.AppImage
```

- It shares its settings (`~/.config/xLights`) and log (`~/.local/state/xLights`) with the
  official xLights AppImage.
- If FUSE is missing (`libfuse2t64` on Ubuntu 24.04), run it with `--appimage-extract-and-run`.
- The cloud secret key goes to the system keyring (GNOME Keyring, KWallet...) through libsecret.
  On a machine without one, it is saved, unencrypted, in `~/.config/xLights/settings.json`, and
  Preferences > Cloud Storage says so. Set `XLIGHTS_CLOUD_NO_KEYRING=1` to always use the settings
  file, for example on a headless machine where a locked keyring would ask for its password in a
  window nobody sees.

**Without a screen** (e.g. an Ubuntu 24.04 server or VM), run it in Xvfb:

```bash
sudo apt install xvfb libgl1 libegl1 libglu1-mesa libgl1-mesa-dri imagemagick
Xvfb :99 -screen 0 1920x1080x24 &
DISPLAY=:99 XLIGHTS_CLOUD_NO_KEYRING=1 ./xLightsCloud-*-x86_64.AppImage --appimage-extract-and-run &
DISPLAY=:99 import -window root screenshot.png   # screenshot of the whole screen
```

The language is set in `~/.config/xLights/settings.json` (`"xLightsUILanguage": "fr"` in the
`main` section) or in Preferences > Other > Language. Each release is checked this way before it
is published: the French catalogs are in the AppImage and xLights starts in French under Xvfb.

## Support

xLights Cloud is **not made or supported by the xLights team**. Please report problems in this
repository's [issues](../../issues), not to the xLights project. Crash reports are kept on your
computer instead of being sent to the official crash server.

## How this repository works

The `xlights-cloud` branch is the official master plus the cloud backup changes. A daily workflow
([`xlights-cloud.yml`](workflows/xlights-cloud.yml)) checks that the official master still merges
(and opens an issue if not), and builds a new release when the xLights team publishes a version.

xLights is free software under the GPL; see [License.txt](../License.txt). xLights Cloud keeps the
same license, and its source is this repository.

---

*En français :* xLights Cloud est le xLights officiel avec en plus la sauvegarde et la restauration
du dossier de show vers Cloudflare R2 ou tout stockage compatible S3, et une interface en français
(voir [Version française intégrée](#version-française-intégrée)). Un installeur Windows et une
AppImage Linux sont publiés pour chaque version officielle. Ce n'est pas un produit de l'équipe xLights : signalez les
problèmes ici.
