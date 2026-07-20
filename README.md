# Vita2OSC

Application homebrew **PS Vita** qui transforme la console en contrôleur **OSC** sans-fil, avec une **webapp companion** pour configurer les mappings à distance.

Compatible TouchDesigner, Resolume, Max/MSP, Chataigne, Ableton (via OSC), etc.

## Fonctionnalités

- Émission OSC/UDP de tous les contrôles Vita (boutons, sticks, tactile, mouvement)
- **UI "glasscore"** (vita2d) : rail de navigation tactile à icônes, transitions animées,
  toasts de feedback, batterie/réseau en direct, splash screen pixel art
- Pages visuelles : console interactive (Boutons), pads XY avec zone morte (Sticks),
  zones tactiles avant/arrière, barres bipolaires + horizon (Mouvement)
- **Réglage de l'IP/port directement sur la console** : clavier tactile officiel (IME)
  en touchant un champ ou avec △, ou ajustement fin au D-pad (appui long = répétition)
- Page **Couleurs** : effets Colorize & Dreamy Glow, sliders H/S/B/Alpha tactiles, toggles par groupe
- **Webapp companion** (`webapp/index.html`) : édition de la config, presets, import/export JSON
- Serveur HTTP embarqué sur la Vita (port 8080) : `GET/POST /config`, `POST /osc`, `GET /status`
- Persistance de la config sur `ux0:data/vita2osc/config.json`
- Icône + LiveArea en **pixel art** (générés par `gen_pixelart.py`, embarqués dans le VPK)

## Prérequis

- PS Vita en custom firmware (HENkaku / Enso)
- [VitaSDK](https://vitasdk.org/) installé (`VITASDK` défini), avec **vita2d**

## Build & installation

```bash
mkdir build && cd build
cmake ..
make
```

Le fichier `build/Vita2OSC.vpk` est généré. Installe-le via **VitaShell**.

Voir [README_BUILD.md](README_BUILD.md) pour le guide détaillé.

## Utilisation rapide

1. Lance **Vita2OSC** sur la Vita → page **Réglages** → note l'IP locale
2. Règle la cible OSC directement sur la console (clavier tactile : toucher le champ IP ou △),
   ou depuis la webapp
3. Sur ton PC (même Wi-Fi), ouvre `webapp/index.html` (ou `python3 -m http.server` dans `webapp/`)
4. **Envoyer à la Vita** pour appliquer à chaud

**Navigation Vita :** L/R ou toucher le rail gauche = pages · maintenir SELECT+START = quitter

## Presets OSC

Des presets de référence sont dans `presets/` (TouchDesigner port 9000, Resolume port 7000).

## Licence

Projet open source — voir le dépôt pour les détails.
