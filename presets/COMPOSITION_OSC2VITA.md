# Composition Resolume OSC2Vita

Architecture stable : **L2 Console** (adresses OSC fixes), oscillos + accéléromètre sur layers **ajoutés** sans renuméroter L2.

## Règle critique

**Ne jamais déplacer** un layer ou des clips déjà adressés en OSC. Pour étendre la compo, **insérer** de nouveaux layers (ou ajouter en haut) et mettre à jour preset + doc **en même temps** si les indices changent.

| Layer verrouillé | Adresses |
|---|---|
| **L2 Console** | `/composition/layers/2/clips/{1-12}/connect` |

## Stack des couches (bas → haut)

| Layer | Nom | Contenu col. 1 | Pilotage Vita |
|---|---|---|---|
| **L1** | Fond | Solid Color noir | — |
| **L2** | Console | 12 clips blancs (Piano) | Boutons |
| **L3** | Accel | Abstract Field | Accéléromètre |
| **L4** | Oscillo Gauche | Abstract Field | Stick gauche |
| **L5** | Oscillo Droit | Abstract Field | Stick droit |

## Effets composition

| Dashboard | Effet | Paramètre |
|---|---|---|
| **link1** | Displace avant | X factor |
| **link2** | Displace avant | Y factor |
| **link3** | Displace arrière | X factor |
| **link4** | Displace arrière | Y factor |

### Câblage dashboard layer

Pour **L3, L4, L5** : lier le dashboard du layer aux paramètres Abstract Field (col. 1) :

| Link layer | Paramètre Abstract Field |
|---|---|
| link2 | Amplitude |
| link3 | Density |
| link4 (L3 seulement) | Octaves |

L’**opacité** se pilote en OSC direct (`/composition/layers/N/video/opacity`).

## Mapping OSC (preset `preset_resolume.json`)

### Boutons → Console L2 (int 0/1)

`/composition/layers/2/clips/{1-12}/connect`

### Stick gauche → L4 Oscillo Gauche

| Vita | OSC |
|---|---|
| Stick G X | `/composition/layers/4/video/opacity` |
| Stick G X | `/composition/layers/4/dashboard/link2` |
| Stick G Y | `/composition/layers/4/dashboard/link3` |

### Stick droit → L5 Oscillo Droit

| Vita | OSC |
|---|---|
| Stick D X | `/composition/layers/5/video/opacity` |
| Stick D X | `/composition/layers/5/dashboard/link2` |
| Stick D Y | `/composition/layers/5/dashboard/link3` |

### Accéléromètre → L3 Accel

| Vita | OSC |
|---|---|
| Accel X | `/composition/layers/3/video/opacity` |
| Accel Y | `/composition/layers/3/dashboard/link2` |
| Accel Z | `/composition/layers/3/dashboard/link3` |
| Accel X | `/composition/layers/3/dashboard/link4` (Octaves) |

### Tactile → Displace (composition)

| Vita | OSC | Relâchement |
|---|---|---|
| Touch avant X | `/composition/dashboard/link1` | → 0 |
| Touch avant Y | `/composition/dashboard/link2` | → 0 |
| Touch arrière X | `/composition/dashboard/link3` | → 0 |
| Touch arrière Y | `/composition/dashboard/link4` | → 0 |

## Mise en service

1. Ouvrir **OSC2Vita** dans Arena (structure ci-dessus).
2. **Sauvegarder** `OSC2Vita.avc`.
3. Webapp → **Preset Resolume OSC2Vita** → IP + port **9000** → **Envoyer à la Vita**.
4. Recompiler/réinstaller Vita2OSC si besoin (`absolute`, `zeroOnRelease`).
