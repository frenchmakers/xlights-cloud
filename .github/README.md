# xLights Cloud

**xLights Cloud** is the official [xLights](https://github.com/xLightsSequencer/xLights) with two more
commands in the File menu:

- **Backup to Cloud** uploads the whole show folder (media, rendered `.fseq` files and a layout
  thumbnail included) to Cloudflare R2 or any S3-compatible storage, in the background with progress.
  It can keep only the last N backups of each show.
- **Restore from Cloud** lists the backups in the bucket, checks the one you pick and brings it back
  into a new folder. Old backups can be deleted from there.

Set it up in **Preferences > Cloud Storage**. Cloudflare R2 is free up to 10 GB of storage.

## Download (Windows 64-bit)

Get the installer from the [Releases](../../releases) page. There is one xLights Cloud release for
each official xLights release, built from that exact version with the cloud backup added.

- It installs next to the official xLights (in `C:\Program Files\xLights Cloud`) and shares its
  settings; `.xsq` files keep opening with the official xLights.
- Help > Check for Updates looks for new xLights Cloud releases here.
- The installer is not code-signed: Windows SmartScreen asks for confirmation (More info > Run anyway).

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
du dossier de show vers Cloudflare R2 ou tout stockage compatible S3. Une version Windows est
publiée pour chaque version officielle. Ce n'est pas un produit de l'équipe xLights : signalez les
problèmes ici.
