# xLights Cloud — dossier « prêt à jouer » et `/currentshow`

Statut : **validé** par l'utilisateur le 2026-11-23 (relu par l'agent STM32).
Format version 1. Aucun show existant à conserver : on repart d'un bucket vide,
sans période de transition (`/show.fseq`, `/show.mp3` et `/controllers/<nom>.json`
disparaissent).

## 1. Principe

| Qui | Connaît | Fait |
|---|---|---|
| xLights Cloud | clés R2 (écriture), noms des contrôleurs | sauvegarde `source/`, publie `readytoplay/<version>/`, écrit `currentshow.json` sur demande |
| Worker | table empreinte du jeton → contrôleur, lecture du bucket | dit à chaque carte quoi télécharger, sert les fichiers de **sa** carte seulement |
| Carte | son nom et son jeton (identité en flash, posée au ST-LINK) | `GET /currentshow`, télécharge, joue |

Le **nom du contrôleur** dans xLights (≤ 32 caractères, `[A-Za-z0-9._-]`) est le seul
lien entre xLights et le worker. xLights ne voit jamais les jetons.
Pas de changement de jeton à distance : un jeton se change à l'atelier (ST-LINK).

## 2. Arborescence R2

```
currentshow.json                                  ← racine du bucket, écrit par « Mettre en lecture »
<préfixe>/shows/<show>/source/
    <stamp>.zip                                   ← archive du dossier de show (sans .fseq)
    <stamp>.manifest.json
    <stamp>.thumbnail.png                         ← facultatif
    latest.manifest.json
<préfixe>/shows/<show>/readytoplay/<stamp>/
    manifest.json                                 ← écrit en DERNIER : une version sans manifest est incomplète
    media/<NN>-<nom>.mp3                          ← un par séquence, partagé par toutes les cartes
    controllers/<contrôleur>/config.json          ← format actuel (pixelPorts, serialPorts…)
    controllers/<contrôleur>/<NN>-<nom>.fseq      ← un par séquence, réduit aux canaux du contrôleur
```

- `<stamp>` : UTC `AAAA-MM-JJ-HHMMSS`, le même pour `source` et `readytoplay` d'une sauvegarde.
- `<NN>` : position dans la playlist (01, 02…) ; `<nom>` : nom de la séquence filtré en `[A-Za-z0-9._-]`.
- Une version `readytoplay/<stamp>/` n'est **jamais modifiée** après écriture.
- « Garder les N dernières sauvegardes » s'applique à `source` et `readytoplay`, et ne supprime
  jamais la version désignée par `currentshow.json`.

## 3. Le `.fseq` d'un contrôleur

- FSEQ v2, plages éparses (sparse ranges) = la plage de canaux du contrôleur, **numéros de
  canaux d'origine** conservés. C'est l'écriture qu'utilise déjà FPP Connect pour les cartes
  FPP distantes (`FPP::PrepareUploadSequence`), sortie dans une fonction réutilisable.
- Compression Zstd (déjà lue par la carte). Même cadence que la séquence.
- `startChannel` du `config.json` reste donc valable tel quel.

### Fichiers identiques d'une version à l'autre

La carte télécharge à ~60 Ko/s et range ses fichiers par SHA-256 : un fichier inchangé n'est
pas re-téléchargé, à condition qu'xLights le reproduise **octet pour octet**.
- `.fseq` : `uniqueId` de l'en-tête FSEQ dérivé du contenu (pas de l'heure), compression Zstd
  à niveau fixe et sans multithread, aucune date dans les en-têtes variables.
- `config.json` : le champ `generated` en sort (la date est dans `manifest.json`).
- mp3 : copié tel quel.

## 4. `readytoplay/<stamp>/manifest.json`

```json
{
  "format": 1,
  "show": "Noel 2026",
  "stamp": "2026-11-23-101500",
  "created": "2026-11-23T10:15:00Z",
  "xlights": "2026.21-cloud",
  "playlist": [
    { "name": "Vive le vent", "frameMs": 25, "durationMs": 15000,
      "media": "media/01-Vive_le_vent.mp3" }
  ],
  "controllers": {
    "frenchmakers": {
      "config": "controllers/frenchmakers/config.json",
      "fseq": [ "controllers/frenchmakers/01-Vive_le_vent.fseq" ]
    }
  },
  "files": {
    "media/01-Vive_le_vent.mp3": { "size": 240000, "sha256": "…" },
    "controllers/frenchmakers/config.json": { "size": 310, "sha256": "…" },
    "controllers/frenchmakers/01-Vive_le_vent.fseq": { "size": 92000, "sha256": "…" }
  }
}
```

`controllers.<nom>.fseq[i]` correspond à `playlist[i]`. `media` est absent pour une séquence
sans audio. Seul le mp3 est publié pour l'instant ; un autre format audio donne un avertissement
et la séquence part sans son.

## 5. `currentshow.json` (racine du bucket)

```json
{ "format": 1,
  "version": "xlights/shows/Noel 2026/readytoplay/2026-11-23-101500/",
  "show": "Noel 2026", "stamp": "2026-11-23-101500",
  "published": "2026-11-23T10:20:00Z" }
```

Écrit uniquement par « Mettre en lecture » dans xLights. C'est la seule écriture qui change ce
que jouent les cartes ; revenir à une version précédente = la remettre en lecture.

## 6. Worker

### Authentification

- KV `DEVICES` : clé = SHA-256 hexadécimal du jeton, valeur `{"controller":"frenchmakers"}`.
- Chaque requête : `Authorization: Bearer <jeton>` → empreinte → recherche dans `DEVICES`.
  Absent → `401`. Le contrôleur vient **de la table**, jamais de l'URL.
- Ajout d'une carte : script atelier qui génère le jeton (`.local/tokens/<nom>.txt`), le pose
  dans la carte (`provision-board.ps1 -Controller <nom> -TokenFile …`) et ajoute la ligne au KV
  (`wrangler kv key put`). Révocation = suppression de la ligne.

### `GET /currentshow`

Lit `currentshow.json`, puis le `manifest.json` de la version, et répond pour **cette** carte :

```json
{
  "format": 1,
  "controller": "frenchmakers",
  "show": "Noel 2026",
  "version": "2026-11-23-101500",
  "config": { "path": "/v/2026-11-23-101500/controllers/frenchmakers/config.json",
              "size": 310, "sha256": "…" },
  "playlist": [
    { "name": "Vive le vent", "frameMs": 25, "durationMs": 15000,
      "fseq":  { "path": "/v/2026-11-23-101500/controllers/frenchmakers/01-Vive_le_vent.fseq",
                 "size": 92000, "sha256": "…" },
      "media": { "path": "/v/2026-11-23-101500/media/01-Vive_le_vent.mp3",
                 "size": 240000, "sha256": "…" } }
  ]
}
```

- `ETag` = `version` + contrôleur ; `If-None-Match` → `304`.
- Limites : réponse ≤ 16 Ko, playlist ≤ 32 entrées, `name` ≤ 64 octets UTF-8, `path` ≤ 160,
  `sha256` en 64 caractères hexadécimaux minuscules, `size` ≤ 2^32−1. xLights refuse de publier
  au-delà. `frameMs` et `durationMs` sont informatifs (la carte suit l'en-tête FSEQ).
- `503` : pas de `currentshow.json` ou version incomplète (pas de manifest).
- `404` : la version en cours ne contient pas ce contrôleur.
- Réponse < 2 Ko pour une playlist de 8 séquences.

### `GET /v/<stamp>/<chemin>`

- `<stamp>` doit être la version en cours (sinon `404`). Une carte qui télécharge pendant une
  mise en lecture reçoit donc un `404` : elle abandonne, garde sa version et reprend à la
  prochaine synchronisation.
- `<chemin>` autorisé : `media/…` ou `controllers/<contrôleur de la carte>/…` ; tout autre → `404`.
- Chemins relatifs à l'origine du worker : la carte ne suit jamais une URL externe.
- `ETag` / `If-None-Match` comme aujourd'hui ; tailles max : `.fseq` 512 Mio, mp3 64 Mio, json 8 Ko.

### Routes supprimées et ordre de bascule

`/show.fseq`, `/show.mp3`, `/controllers/<nom>.json` disparaissent. Les routes du firmware
(`/firmware/app-a.bin`, `/firmware/app-b.bin`) ne changent pas, mais passent elles aussi par
la table `DEVICES`. Pour ne pas perdre l'OTA de la carte déjà installée :
1. ajouter au KV l'empreinte du jeton actuel → `frenchmakers` ;
2. republier `firmware/app-a.bin` et `app-b.bin` dans le bucket vidé ;
3. publier le firmware qui gère `/currentshow` (la carte actuelle tolère un `404` sur
   `/show.fseq` et va jusqu'au contrôle du firmware) ;
4. ensuite seulement, retirer les anciennes routes.

## 7. Firmware

- Au démarrage et sur la commande web « synchroniser » : `GET /currentshow`. La réponse est
  écrite sur la clé (`CURRENT.TMP`) puis analysée une fois le TLS fermé.
- « Synchroniser » met le show en pause, synchronise, et redémarre la carte si la version a
  changé (les caches décompressés ne se construisent qu'au démarrage).
- Si `version` a changé : télécharger dans `NEXT/` ce qui n'est pas déjà présent (recherche par
  SHA-256), vérifier taille et SHA-256, puis `CURRENT/` → `OLD/` et `NEXT/` → `CURRENT/`.
  Noms locaux 8.3 propres à la carte (`S01.FSQ`, `S01.MP3`, `CONFIG.JSN`, `MANIFEST.JSN`).
- Au démarrage : un cache décompressé par séquence (`RAW01`…), marqué par le SHA-256 de sa
  source, reconstruit seulement s'il a changé. Les mp3 sont lus directement.
- Playlist : séquences dans l'ordre puis reboucle, court noir entre deux ; le mp3 sert
  d'horloge à sa séquence, sinon horloge interne. Page web : « séquence n/N : nom »,
  Précédent / Suivant. Une carte compilée sans audio ignore `media`.
- Hors ligne, erreur ou `404` (contrôleur absent de la version) : `CURRENT/` continue de jouer,
  l'état web le signale.
- Estimation agent STM32 : 4 à 6 jours, tests compris.

## 8. xLights Cloud

- **Préférences** : « Inclure les .fseq » devient « Publier un dossier prêt à jouer ». Si
  l'option est active, la sauvegarde demande les séquences à publier, dans l'ordre (choix mémorisé
  par show).
- **Sauvegarde** : écrit `source/`, puis, si demandé, `readytoplay/<stamp>/` pour chaque contrôleur
  cloud actif (fabricant FrenchMakers), avec les contrôles actuels : 6 ports, ≤ 1000 pixels,
  cadence et débit, nom ≤ 32 caractères. Les `.fseq` absents ou plus anciens que leur séquence
  bloquent la publication, comme aujourd'hui pour l'archive.
- **Mettre en lecture** : dans la fenêtre « Restaurer depuis le cloud », une version prête à jouer
  peut être mise en lecture (confirmation, puis écriture de `currentshow.json`). Case facultative
  « Mettre en lecture à la fin » dans la sauvegarde, décochée par défaut.
- **Restaurer** : liste les sauvegardes de `source/`, indique laquelle est en lecture.

## 9. Décisions

1. **Une seule playlist par version.**
2. **FPP : porte ouverte, rien d'implémenté.** `config` est facultatif dans la réponse de
   `/currentshow` (un contrôleur FPP n'aurait pas de `config.json`) ; plus tard, un FPP pourrait
   recevoir son `.fseq` réduit (secondaire) ou complet (principal) et sa config au format FPP.
