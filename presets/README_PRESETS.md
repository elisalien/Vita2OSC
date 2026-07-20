# Presets OSC de référence

Configurations prêtes à charger dans Vita2OSC :

| Fichier | Usage |
|---|---|
| `preset_touchdesigner.json` | TouchDesigner (adresses `/vita/...`, port 9000) |
| `preset_resolume.json` | **Resolume OSC2Vita** — adresses natives, **sans Edit OSC** |
| `preset_resolume_osc2vita.json` | Identique à `preset_resolume.json` (copie explicite) |
| `preset_resolume_generic.json` | Resolume générique `/vita/...` + apprentissage manuel |

> Avant usage : remplace `target.ip` par l'IP de ta machine qui reçoit l'OSC (celle où tourne TouchDesigner ou Resolume), visible sur le même réseau que la Vita.

---

## Pourquoi plusieurs presets Resolume

| Aspect | OSC2Vita (`preset_resolume.json`) | Générique (`preset_resolume_generic.json`) |
|---|---|---|
| **Adresses** | Chemins natifs `/composition/...` | Préfixe `/vita/...` |
| **Edit OSC** | **Non requis** | Requis (apprentissage manuel) |
| **Composition** | OSC2Vita (L2 clips 1–12, effects 2–4) | N'importe quelle compo |
| **Port** | 9000 (ou celui déjà configuré) | idem |
| **Valeurs** | tout en **0…1** | tout en **0…1** |
| **Boutons** | `onChange` (Piano) | `onChange` (Piano) |

---

## TouchDesigner vs Resolume

| Aspect | TouchDesigner | Resolume OSC2Vita |
|---|---|---|
| **Port OSC** | **9000** | **9000** (ou ton port Arena) |
| **Plage sticks** | **−1…+1** | **0…1** |
| **Fréquence** | **60 Hz** | **30 Hz** |
| **Bundles** | oui | non |

---

## Brancher côté TouchDesigner

1. CHOP **OSC In**, port **9000**.
2. Canaux nommés d'après `/vita/stick/lx`, `/vita/pad/cross`, etc.

---

## Brancher côté Resolume (OSC2Vita)

1. Ouvrir la composition **OSC2Vita** dans Arena.
2. `Preferences > OSC` : **OSC Input** activé, port = celui de la Vita.
3. Importer **`preset_resolume.json`** ou webapp → **Preset Resolume OSC2Vita**.
4. Régler **Cible IP** (PC Resolume) et **port** → **Envoyer à la Vita**.

Les messages partent directement vers les paramètres Resolume — pas de **Edit OSC**.

### Adressage automatique (résumé)

- Boutons → `/composition/layers/2/clips/{1-12}/connect` (float 0/1, Piano)
- Stick G X → `/composition/dashboard/link1`
- Stick D X → `/composition/dashboard/link2`
- Touch X/Y → `/composition/dashboard/link3`, `link4`

Détail complet : `COMPOSITION_OSC2VITA.md`.

---

## Resolume générique (/vita)

Pour une autre composition :

1. Charger `preset_resolume_generic.json` ou webapp **Preset Resolume (/vita)**.
2. `Shortcuts > Edit OSC` : apprendre chaque `/vita/...` vers le paramètre voulu.

---

## Adapter à un autre logiciel

Change le **port**, la **plage** (0…1 ou −1…+1), et les **adresses** selon le logiciel cible.
