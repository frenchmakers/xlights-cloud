# Glossaire français de xLights

Référence pour traduire `fr/xlights.po`. Un même terme anglais se traduit
toujours de la même façon.

## Règles générales

- **Vouvoiement**, ton neutre.
- **Menus, boutons, titres** : infinitif (« Enregistrer », « Importer des
  effets »). **Infobulles et aides** : phrase à la 3ᵉ personne du présent
  (« Décale tous les effets de la séquence. »).
- **Majuscule** seulement au premier mot (« Paramètres de l'effet », pas
  « Paramètres De L'Effet »).
- **Ponctuation** : espace insécable avant `:`, `;`, `?`, `!` ; guillemets
  « » ; points de suspension `...` comme dans l'original.
- **Raccourcis** : garder `&` (accélérateur, sur une lettre du mot français)
  et tout ce qui suit `\t` tel quel (`Enregistrer\tCTRL-S`).
- **Variables** : garder `%s`, `%d`, `{}`… et leur ordre ; ne pas toucher
  aux `\n`.

## Ne jamais traduire

- **Noms d'effets** (On, Off, Bars, Butterfly, Fire, Morph, Shockwave…) : ce
  sont des identifiants enregistrés dans les `.xsq`, et la communauté les
  connaît sous ce nom.
- **Types de modèles** (Matrix, Tree, Arches, Candy Canes, Custom, Poly
  Line…) et **valeurs de listes enregistrées** (voir `README.md`, « Do not save
  a translated label »).
- **Types de courbes de valeur** (Flat, Ramp, Ramp Up/Down, Saw Tooth,
  Parabolic…) : `ValueCurveDialog` les relit par leur nom.
- **Éléments de listes relus par leur libellé** (`GetStringSelection` /
  `SetStringSelection`) :
  - Préférences > Other : Always Prompt / Always Yes / Always No, Show Folder /
    AppData-shared, Faces/States/Submodels / Unassigned, Beginner /
    Intermediate / Advanced / Expert, None / Inputs and Outputs, codecs
    (Auto, H.264…), OpenGL / Vulkan ;
  - Sequences : Enabled / Disabled / Locked Effects Only, Unlimited et les
    tailles (100 MB…), « Locked Only » (valeur par défaut enregistrée) ;
  - View : Normal / Large (taille de palette).
  Comme une phrase anglaise n'a qu'une traduction, ces mots restent en anglais
  partout où ils apparaissent.
- **Libellés de grilles et d'arbres relus par le code** : lignes de la grille
  des visages (Face Outline, Mouth - AI…, Eyes - Open…), colonnes Eyes Open /
  Eyes Closed, grilles des états et des sous-modèles, planning HinksPix.
  Ils construisent des clés enregistrées dans le modèle.
- **Choix relus par leur libellé dans le séquenceur et la disposition** :
  types de séquence (Media / Animation / Effect), modes de rendu (Canvas /
  Erase), dispositions de groupes (Horizontal Stack, Grid as per preview…),
  filtres de visages, placements d'image, sources de notes (MIDI File…),
  timings fixes (25ms, Metronome…), découpes de sous-modèles (Horizontal
  Slices…).
- **Libellés comparés à du texte anglais écrit en dur** : onglets du panneau
  Moving Head (Position, Dimmer, Pathing, Pattern, Control, Link, Status,
  ColorWheel), onglet « Info / Media » des réglages de séquence, « Beginning »
  (chaînage des modèles). Avant de traduire un libellé, chercher dans son
  fichier une comparaison `== "…"` avec le même texte.
- **Choix des contrôleurs et des imports** : versions FSEQ et Local / Proxied
  de FPP Connect, Master / Remote de HinksPix, Forward / Reverse, modes de
  fusion SuperStar (« 2 reveals 1 »…).
- **Contenu écrit dans des fichiers** (en-têtes CSV des exports d'effets).
- **Valeurs initiales de champs de saisie** (`<group name>`, `Insert text
  here`) : enregistrées telles quelles si l'utilisateur ne les change pas.
- **Libellés internes jamais affichés** de wxSmith (Item label, Pane caption,
  Label, Status bar, RECENT1…).
- **Noms de produits et protocoles** : xLights, xSchedule, FPP, Falcon, WLED,
  ESPixelStick, HinksPix, DMX, E1.31, sACN, Art-Net, DDP, ZCPP, Peppol, S3,
  Cloudflare R2, Jukebox, Vamp, Papagayo.
- **Extensions et formats** : `.xsq`, `.fseq`, `.xtiming`, `.xmodel`, FSEQ.

## Termes

| Anglais | Français | Remarque |
|---|---|---|
| Show | show (masc., pl. shows) | jargon de la communauté |
| Show directory / folder | dossier du show | |
| Sequence | séquence | |
| Sequencer | séquenceur | onglet « Sequencer » → Séquenceur |
| Effect | effet | |
| Effects grid | grille des effets | |
| Effect panel / settings | panneau / paramètres de l'effet | |
| Model | modèle | |
| Model group | groupe de modèles | |
| Model Set / Set | ensemble de modèles / ensemble | modèles déplacés ensemble dans la disposition |
| Viewpoint | point de vue | |
| Submodel | sous-modèle | |
| Node | nœud | un point adressable, qui peut regrouper plusieurs LED |
| String | chaîne | ensemble de nœuds sur une sortie |
| Strand | brin | |
| Layout | disposition (fém.) | onglet « Disposition » ; « Layout Panel » → panneau de disposition |
| Preview | aperçu | « House Preview » → aperçu de la maison |
| Display elements | éléments affichés | |
| View | affichage (menu) / vue (vue de séquence) | « Views » de séquence → vues |
| Perspective | perspective | |
| Timing track | piste de timing | |
| Timing mark | repère de timing | |
| Lyrics / Phonemes | paroles / phonèmes | |
| Chorus / Peak (sections de chanson) | refrain / pic | générateur d'effets de lyres |
| Wizard | assistant | |
| Layer | calque | |
| Blend / Blending | fusion | « Layer Blending » → Fusion des calques |
| Transition | transition | |
| Buffer | grille | « Render buffer » → grille de rendu ; « Sub-buffer » → sous-grille ; « Buffer style » → style de grille. Seul, préciser « grille de rendu » pour ne pas confondre avec la grille des effets |
| Canvas | canevas | |
| Palette | palette | |
| Color curve / Value curve | courbe de couleur / courbe de valeur | |
| Preset | préréglage | |
| Render / Rendering | rendu | « Render All » → Tout rendre ; « Batch Render » → Rendu par lots |
| Render cache | cache de rendu | |
| Playback / Play | lecture / lire | |
| Controller | contrôleur | |
| Output | sortie | |
| Port | port | |
| Universe | univers | |
| Channel | canal (pl. canaux) | « Start Channel » → canal de début |
| Start / End | début / fin | |
| Brightness | luminosité | |
| Gamma | gamma | |
| Fade in / out | fondu d'entrée / de sortie | |
| Upload | envoi / envoyer | « Upload to controller » → Envoyer au contrôleur |
| Discover | rechercher | « Discover » (contrôleurs) → Rechercher |
| Ping | ping | |
| Moving head | lyre | « Moving Head » DMX → lyre |
| Faces / States | visages / états | |
| Mapping (import) | correspondance | « Channel Map » → correspondance des canaux |
| Import / Export | importer / exporter ; import / export (nom) | |
| Backup / Restore | sauvegarde, sauvegarder / restaurer | |
| Cloud Storage | stockage cloud | |
| Bucket | bucket | terme S3, gardé tel quel |
| Endpoint | point de terminaison | |
| Prefix | préfixe | |
| Access key / Secret key | clé d'accès / clé secrète | |
| Media | médias | |
| Settings / Preferences | paramètres / préférences | |
| Package / Packaged sequence | paquet / séquence empaquetée | |
| Log | journal | |
