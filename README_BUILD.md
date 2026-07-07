# Vita2OSC — Build & lancement

Application homebrew qui transforme la PS Vita en contrôleur OSC sans-fil.
État actuel : **plan complet (phases 1 à 6)** — émission OSC/UDP de toutes les
entrées, moteur de mapping, UI à onglets, **persistance de la config**, **serveur
HTTP embarqué** et **webapp companion** pour éditer/pousser la config sans-fil.

## Arborescence

```
Vita2OSC/
├─ CMakeLists.txt          # build VitaSDK -> VPK
├─ src/
│  ├─ main.c               # boucle principale, navigation onglets, intégration
│  ├─ osc.c / osc.h        # sérialisation OSC 1.0 (messages + bundles)
│  ├─ net.c / net.h        # SceNet : init réseau + socket UDP
│  ├─ input.c / input.h    # SceCtrl/SceTouch/SceMotion, normalisation
│  ├─ config.c / config.h  # modèle de config + preset par défaut
│  ├─ config_json.* / json.* # (dé)sérialisation JSON de la config
│  ├─ storage.c / storage.h# persistance ux0:data/vita2osc/config.json
│  ├─ httpd.c / httpd.h    # serveur HTTP threadé (GET/POST /config, /status)
│  ├─ mapping.c / mapping.h# deadzone, lissage, invert, modes, émission
│  └─ ui.c / ui.h          # onglets vita2d + feedback live
├─ webapp/index.html       # webapp companion (éditeur de config)
├─ presets/                # presets OSC de référence (TouchDesigner, Resolume)
└─ PLAN_DE_TRAVAIL.md      # plan complet
```

## Prérequis

- **PS Vita en custom firmware** (HENkaku / Enso) pour lancer du homebrew.
- **VitaSDK** installé + variable d'environnement `VITASDK` définie.
  Voir https://vitasdk.org/ (installer `vdpm` puis les libs, dont **vita2d**).
- CMake ≥ 3.10 et `cmake`/`make` disponibles.

Installer vita2d si absent :
```bash
git clone https://github.com/xerpi/libvita2d && cd libvita2d/libvita2d && make install
```

## Compiler

```bash
cd Vita2OSC
mkdir build && cd build
cmake ..
make
```

Le fichier **`Vita2OSC.vpk`** est produit dans `build/`.

## Installer sur la Vita

1. Copier `Vita2OSC.vpk` sur la Vita (FTP via VitaShell, ou clé USB).
2. Dans **VitaShell**, sélectionner le VPK et l'installer.
3. Lancer **Vita2OSC** depuis le menu LiveArea.

## Utilisation

- La Vita doit être sur le **même réseau Wi-Fi** que la machine réceptrice.
- Onglet **Reglages** : affiche l'IP locale de la Vita, la cible OSC (IP/port),
  et l'état d'envoi (PRET / non joignable) + paquets/s.
- **L / R** : changer d'onglet. **SELECT + START** : quitter.
- Les onglets Boutons / Sticks / Touch Av / Touch Ar / Mouvement montrent les
  valeurs envoyées en temps réel.

Au premier lancement, la config par défaut (preset TouchDesigner, cible
`192.168.1.42:9000`) est utilisée. Modifie-la ensuite **depuis la webapp** (voir
ci-dessous) : elle est appliquée à chaud et sauvegardée sur la carte mémoire
(`ux0:data/vita2osc/config.json`), donc conservée au prochain lancement.

Côté logiciel récepteur : voir `presets/README_PRESETS.md` (TouchDesigner OSC In
port 9000, Resolume port 7000, etc.).

## Webapp companion (édition de la config sans-fil)

1. Sur la Vita, ouvre Vita2OSC → onglet **Reglages** : note l'**IP locale**
   (ex. `192.168.1.50`). Le serveur HTTP écoute sur le port **8080**.
2. Sur ton PC (même Wi-Fi), ouvre `webapp/index.html` dans un navigateur.
3. Saisis l'IP de la Vita, clique **Récupérer la config** (ou charge un preset).
4. Édite les contrôles (adresse OSC, type, min/max, deadzone, lissage, mode…),
   règle la cible OSC dans l'onglet **Reglages**.
5. Clique **Envoyer à la Vita** : la config est appliquée immédiatement et
   sauvegardée. Boutons **Importer/Exporter JSON** pour gérer des presets.

> Si le navigateur bloque la requête depuis `file://`, sers le dossier `webapp/`
> via un petit serveur local, ex. `python3 -m http.server 8000` puis ouvre
> `http://localhost:8000/`.

## Icône / LiveArea (optionnel)

Le VPK utilise l'icône par défaut. Pour personnaliser, ajoute un dossier
`sce_sys/` (icon0.png 128×128, LiveArea) et des lignes `FILE` dans le
`CMakeLists.txt` (voir la doc VitaSDK `vita_create_vpk`).

## Notes de fiabilité / points à valider sur matériel

- **Cadence d'envoi** découplée du rendu (timer `sceKernelGetProcessTimeWide`),
  réglée sur `send_rate_hz`. Mesurer la latence réelle sous charge (tous capteurs).
- **Codes d'erreur "déjà initialisé"** de `sceNetInit`/`sceNetCtlInit` tolérés ;
  si tu relances net_init en boucle, vérifier le comportement.
- **Points tactiles levés** : les coordonnées ne sont pas ré-émises (pas de valeur
  « collée »), le lissage est réinitialisé.
- Le rendu s'appuie sur le vsync de `vita2d_swap_buffers` (~60 fps) pour pacer la
  boucle ; si ta version de vita2d ne synchronise pas, ajouter un
  `sceDisplayWaitVblankStart()`.

## Statut des phases

- ✅ **Phase 1-3** : OSC/UDP, lecture de toutes les entrées, mapping, UI onglets.
- ✅ **Phase 4** : persistance de la config (`ux0:data/vita2osc/config.json`).
- ✅ **Phase 5** : serveur HTTP embarqué (`GET/POST /config`, `GET /status`).
- ✅ **Phase 6** : webapp companion (éditeur, presets, import/export, envoi Vita).
- ⏳ **Phase 7 (option, non implémentée)** : preview live via WebSocket
  (visualiser l'état des contrôles dans la webapp en temps réel).

## À valider sur matériel (checklist de test)

1. Build VPK, installation, lancement, écran d'onglets visible.
2. Onglet Reglages : IP locale affichée, état « PRET » quand la cible répond.
3. Réception OSC dans TouchDesigner (port 9000) puis Resolume (port 7000).
4. Webapp : GET config, édition, POST → changement pris en compte à chaud.
5. Redémarrer l'app → la config éditée est bien rechargée (persistance).
6. Latence/perte sous charge (tous capteurs actifs) ; ajuster `sendRateHz`.
