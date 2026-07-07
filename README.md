# Vita2OSC

Application homebrew **PS Vita** qui transforme la console en contrôleur **OSC** sans-fil, avec une **webapp companion** pour configurer les mappings à distance.

Compatible TouchDesigner, Resolume, Max/MSP, Chataigne, Ableton (via OSC), etc.

## Fonctionnalités

- Émission OSC/UDP de tous les contrôles Vita (boutons, sticks, tactile, mouvement)
- Interface à onglets sur la console (vita2d)
- Onglet **Couleurs** : effets Colorize & Dreamy Glow, pickers H/S/B/Alpha tactiles, toggles par groupe
- **Webapp companion** (`webapp/index.html`) : édition de la config, presets, import/export JSON
- Serveur HTTP embarqué sur la Vita (port 8080) : `GET/POST /config`, `POST /osc`, `GET /status`
- Persistance de la config sur `ux0:data/vita2osc/config.json`

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

1. Lance **Vita2OSC** sur la Vita → onglet **Réglages** → note l'IP locale
2. Sur ton PC (même Wi-Fi), ouvre `webapp/index.html` (ou `python3 -m http.server` dans `webapp/`)
3. Saisis l'IP de la Vita, configure la cible OSC (IP/port de ton logiciel)
4. **Envoyer à la Vita** pour appliquer à chaud

**Navigation Vita :** L/R = onglets · SELECT+START = quitter

## Presets OSC

Des presets de référence sont dans `presets/` (TouchDesigner port 9000, Resolume port 7000).

## Licence

Projet open source — voir le dépôt pour les détails.
