# Vita2OSC — Plan de travail

Application homebrew PS Vita transformant la console en surface de contrôle OSC sans-fil, pilotable par une webapp companion, compatible avec tout logiciel OSC (TouchDesigner, Resolume, Max/MSP, Ableton via OSC, Chataigne, etc.).

---

## 1. Objectif

Faire de la PS Vita un **contrôleur OSC complet et générique** :

- envoi de **tous les contrôles physiques et capteurs** de la Vita en OSC/UDP ;
- interface à **onglets** regroupant les contrôles par type (boutons, sticks, tactile avant, tactile arrière, capteurs de mouvement) ;
- **webapp companion** pour créer/éditer la configuration OSC et la **pousser sans-fil** vers la Vita ;
- configuration **portable** et adaptable à n'importe quel logiciel OSC ;
- assignation fine de chaque contrôle avec ses **paramètres OSC détaillés**.

---

## 2. Architecture générale

```
┌────────────────────┐        Wi-Fi (LAN)        ┌────────────────────────┐
│   Webapp companion │  ── HTTP POST /config ──▶  │      PS Vita           │
│  (navigateur PC)   │  ◀─ HTTP GET  /config ──   │  App Vita2OSC          │
│                    │  ◀─ WS état/preview ────    │  ┌──────────────────┐  │
│  éditeur de mapping│                            │  │ Serveur HTTP     │  │
└────────────────────┘                            │  │ (config)         │  │
                                                  │  ├──────────────────┤  │
                                                  │  │ Lecture entrées  │  │
                                                  │  │ (ctrl/touch/motion)│ │
                                                  │  ├──────────────────┤  │
                                                  │  │ Moteur de mapping│  │
                                                  │  ├──────────────────┤  │
                                                  │  │ Émetteur OSC/UDP │──┼──▶ Logiciel OSC
                                                  │  └──────────────────┘  │   (TouchDesigner…)
                                                  └────────────────────────┘
```

Trois blocs :

1. **App Vita** (cœur) : lit les entrées, applique le mapping, émet les messages OSC en UDP.
2. **Webapp companion** : éditeur visuel de configuration, exécuté dans un navigateur.
3. **Canal de configuration** : petit serveur HTTP embarqué sur la Vita ; la webapp lit et écrit la config à distance.

---

## 3. Stack technique recommandée

**Côté Vita : VitaSDK (C/C++) + vita2d.** C'est la voie la plus fiable et documentée pour du homebrew Vita.

| Besoin | Solution | Pourquoi |
|---|---|---|
| Toolchain | **VitaSDK** (open source) | Standard de fait du homebrew Vita, activement maintenu, doc en ligne. |
| Rendu UI (onglets, jauges) | **vita2d** | Lib graphique 2D légère, idéale pour une UI de contrôleur. |
| Entrées physiques | `SceCtrl` | Boutons, sticks, D-pad. |
| Tactile avant/arrière | `SceTouch` | Multitouch avant (`SCE_TOUCH_PORT_FRONT`) et pavé arrière (`SCE_TOUCH_PORT_BACK`). |
| Capteurs de mouvement | `SceMotion` | Accéléromètre, gyroscope, orientation fusionnée. |
| Réseau | `SceNet` / `SceNetCtl` (sockets BSD) | UDP pour l'OSC, TCP pour le serveur HTTP de config. |
| Émission OSC | Sérialisation OSC **maison** (~150 lignes) sur socket UDP | OSC 1.0 est simple ; pas de dépendance lourde à porter. |
| Serveur HTTP config | Micro-serveur TCP maison ou portage léger | Sert/reçoit le JSON de config. |

**Prérequis** : Vita avec custom firmware (**HENkaku / Enso**) pour lancer du homebrew. L'app se package en **VPK**.

**Alternative écartée** : Lua (Lua Player Plus / VitaShell). Prototypage plus rapide mais contrôle bas niveau et performances réseau (latence, débit des messages de mouvement) insuffisants pour un contrôleur temps réel stable. VitaSDK reste la base recommandée.

**Côté webapp** : **HTML/CSS/JS pur** (aucun build), ou un léger framework (Svelte/Vue) si tu préfères. L'important : elle doit tourner localement dans un navigateur et dialoguer avec la Vita en HTTP sur le LAN.

---

## 4. Rappel OSC (Open Sound Control)

Un message OSC = une **adresse** + un **type tag** + des **arguments**, envoyé en **UDP**.

```
/vita/pad/cross ,f 1.0
└── adresse ──┘ │  └ argument
             type tag (f = float32)
```

Types utiles ici :

- `i` = entier 32 bits (ex. bouton 0/1)
- `f` = flottant 32 bits (ex. axe de stick, pression tactile)
- `T` / `F` = booléen vrai/faux (sans argument)
- `s` = chaîne
- **Bundle** = plusieurs messages horodatés envoyés ensemble (utile pour grouper les 6 points tactiles ou les 3 axes d'un capteur dans le même paquet).

TouchDesigner (CHOP **OSC In**), Resolume, Chataigne, Max, etc. attendent tous ce format standard : viser **OSC 1.0**, UDP, floats normalisés.

---

## 5. Inventaire des contrôles de la Vita à exposer

| Catégorie | Contrôles | Nature du signal |
|---|---|---|
| **Boutons** | Croix, Rond, Carré, Triangle, D-pad (H/B/G/D), L, R, Start, Select | booléen (0/1) — option vélocité via durée |
| **Sticks** | Stick gauche (X, Y), stick droit (X, Y) | float, ex. −1…+1 (option 0…1) |
| **Tactile avant** | Jusqu'à 6 points : X, Y, pression, ID de doigt, actif/inactif | float par point + int ID |
| **Tactile arrière** | Pavé arrière multitouch : X, Y, actif/inactif | float par point |
| **Accéléromètre** | Axes X, Y, Z | float |
| **Gyroscope** | Vitesse angulaire X, Y, Z | float |
| **Orientation** | Roll / Pitch / Yaw (fusion capteurs) ou quaternion | float |
| **Système (option)** | Niveau de batterie, timestamp/heartbeat | int/float |

> Note matérielle : la Vita n'a pas de gâchettes analogiques (L/R sont des boutons tout-ou-rien). Les boutons volume et PS ne sont pas exploitables de façon fiable en homebrew — à exclure du périmètre.

---

## 6. Organisation en onglets

Chaque **type de contrôle** = un onglet, comme demandé :

1. **Boutons** — grille des boutons + D-pad ; état visuel (allumé au press), édition de l'adresse/mapping OSC.
2. **Sticks** — 2 pads visuels XY ; affichage des valeurs live ; zone morte réglable.
3. **Tactile avant** — visualisation des points de contact sur une zone représentant l'écran ; mapping X/Y/pression.
4. **Tactile arrière** — idem pour le pavé arrière.
5. **Mouvement** — accéléromètre / gyroscope / orientation ; toggles d'activation, filtrage/lissage.
6. **Réglages** — IP + port cible OSC, préfixe d'adresse global, fréquence d'envoi, statut Wi-Fi/IP de la Vita, chargement/sauvegarde de config.

Navigation par **L/R** entre onglets (pratique pour laisser l'écran tactile libre comme surface de contrôle).

---

## 7. Format de configuration (fichier unique JSON)

Format portable, éditable par la webapp, lu par la Vita.

```jsonc
{
  "version": 1,
  "target": { "ip": "192.168.1.42", "port": 9000 },
  "globalPrefix": "/vita",
  "sendRateHz": 60,
  "controls": [
    {
      "id": "btn_cross",
      "source": "button.cross",
      "enabled": true,
      "address": "/vita/pad/cross",
      "typeTag": "f",       // i | f | T/F
      "min": 0.0, "max": 1.0,
      "mode": "onChange",   // onChange | continuous | trigger
      "invert": false
    },
    {
      "id": "stick_l_x",
      "source": "stick.left.x",
      "enabled": true,
      "address": "/vita/stick/lx",
      "typeTag": "f",
      "min": -1.0, "max": 1.0,
      "deadzone": 0.08,
      "smoothing": 0.0,
      "mode": "continuous"
    },
    {
      "id": "touch_front_0",
      "source": "touch.front.0",     // point 0
      "enabled": true,
      "addressX": "/vita/touch/0/x",
      "addressY": "/vita/touch/0/y",
      "addressPressure": "/vita/touch/0/z",
      "typeTag": "f",
      "min": 0.0, "max": 1.0,
      "bundle": true                 // X,Y,Z dans un même paquet
    }
  ]
}
```

La Vita stocke aussi un **preset par défaut** au premier lancement (aucune config = tout mappé sur des adresses standard).

---

## 8. Webapp companion

Rôle : **éditer le mapping OSC sans toucher à la Vita**, puis l'envoyer.

Fonctions :

- liste des contrôles regroupés par onglet (miroir de l'app) ;
- pour chaque contrôle : activer/désactiver, éditer adresse OSC, type, plage min/max, mode d'envoi, zone morte, lissage, invert ;
- réglages cible (IP/port), préfixe global, fréquence ;
- **import/export** du fichier JSON (sauvegarde de presets par logiciel : un preset « TouchDesigner », un preset « Resolume »…) ;
- **Découverte de la Vita** : saisie manuelle de l'IP de la Vita (affichée dans l'onglet Réglages), ou détection via mDNS/bonjour si implémenté ;
- boutons **« Récupérer la config » (GET)** et **« Envoyer à la Vita » (POST)** ;
- (option) **preview live** : la Vita pousse l'état des contrôles en WebSocket → la webapp visualise ce qui est envoyé.

Aucune dépendance cloud : tout se passe sur le réseau local.

---

## 9. Transport sans-fil de la config (recommandation)

**Recommandé : petit serveur HTTP embarqué sur la Vita.** C'est l'approche la plus fiable et la plus fluide.

- La Vita, connectée au même Wi-Fi, lance un serveur HTTP sur un port (ex. `8080`).
- L'onglet **Réglages** affiche son **IP** (ex. `192.168.1.50`).
- Endpoints :
  - `GET /config` → renvoie le JSON courant (la webapp charge l'existant) ;
  - `POST /config` → reçoit le nouveau JSON, le valide, l'applique **à chaud** et le sauvegarde sur la carte mémoire ;
  - `GET /status` → IP, version, état de l'émission OSC ;
  - (option) `WS /live` → flux d'état pour la preview.
- Avantages : édition en un clic, application immédiate sans redémarrage, pas de câble, pas de manip de fichiers.

**Repli (fallback) simple à prévoir** : import/export d'un fichier `config.json` déposé sur la carte mémoire (via FTP/USB) et chargé depuis l'onglet Réglages — utile si le réseau pose problème.

Sécurité minimale : serveur limité au LAN, jeton/token optionnel dans l'en-tête pour éviter qu'un autre appareil ne pousse une config.

---

## 10. Détail des paramètres OSC à l'assignation

Ce que l'utilisateur règle pour **chaque** contrôle, et son effet :

- **Adresse OSC** (`address`) — chemin du message, ex. `/vita/stick/lx`. C'est l'identifiant que le logiciel OSC va écouter. Convention conseillée : `/prefix/categorie/nom`. Un **préfixe global** évite de tout ressaisir (ex. passer de `/vita` à `/vitaA` pour une 2ᵉ console).

- **Type de donnée** (`typeTag`) —
  - `f` (float) : signaux continus (sticks, tactile, capteurs).
  - `i` (int) : compteurs, index.
  - `T/F` (bool) : boutons on/off « purs » (certains logiciels préfèrent recevoir `f` 0/1 — configurable).

- **Plage min/max** (`min`, `max`) — bornes de la valeur envoyée. Normaliser sur **0…1** ou **−1…+1** selon ce qu'attend le logiciel. Ex. un stick brut `[-128,127]` remappé en `[-1,1]`. C'est le réglage clé pour la **compatibilité** : TouchDesigner aime le 0…1, d'autres le −1…+1.

- **Mode d'envoi** (`mode`) —
  - `onChange` : n'émet que quand la valeur change (économise le réseau, idéal boutons).
  - `continuous` : émet à la fréquence fixée (idéal capteurs/sticks pour un flux régulier).
  - `trigger` : émet une impulsion à l'appui (ex. un « bang »).

- **Fréquence d'envoi** (`sendRateHz`, global + surcharge possible) — combien de fois/seconde les signaux continus partent (30/60/120 Hz). Compromis **fluidité ↔ charge réseau/latence**.

- **Zone morte** (`deadzone`, sticks/mouvement) — seuil sous lequel la valeur est forcée à 0, pour éliminer le bruit de repos des sticks/capteurs.

- **Lissage** (`smoothing`) — filtre (moyenne glissante / passe-bas) réduisant la nervosité des capteurs de mouvement, au prix d'un léger retard.

- **Inversion** (`invert`) — inverse l'axe (utile pour Y d'écran vs Y de logiciel, orientations de caméra, etc.).

- **Bundle** (`bundle`) — regroupe plusieurs valeurs liées (X/Y/pression d'un point, ou X/Y/Z d'un capteur) dans **un seul paquet horodaté** → cohérence temporelle et moins de paquets.

- **Cible** (`target.ip`, `target.port`) — IP + port du logiciel OSC récepteur. Doit correspondre au port d'écoute (ex. TouchDesigner OSC In sur `9000`).

---

## 11. Compatibilité multi-logiciels

- **Adresses et types configurables** → on s'adapte à n'importe quel récepteur OSC.
- **Presets par logiciel** exportables/importables (TouchDesigner, Resolume, Chataigne, Max/MSP, Ableton+OSC…).
- **Normalisation réglable** (0…1 vs −1…+1) pour coller aux conventions de chaque outil.
- **Documenter un preset de référence TouchDesigner** (adresses + port + CHOP OSC In) comme exemple livré.

---

## 12. Feuille de route (phases)

**Phase 0 — Préparation**
- Installer VitaSDK, valider la chaîne de build (VPK « Hello World » via vita2d sur Vita CFW).

**Phase 1 — Émission OSC minimale**
- Sérialiseur OSC (adresse + `f`/`i` + bundle) + socket UDP.
- Envoyer les **boutons** vers une IP/port fixes. Valider la réception dans TouchDesigner.

**Phase 2 — Toutes les entrées**
- Ajouter sticks, tactile avant, tactile arrière, accéléromètre, gyroscope, orientation.
- Normalisation, zone morte, lissage, modes d'envoi.

**Phase 3 — UI à onglets**
- vita2d : navigation L/R, onglets Boutons/Sticks/Tactile av./Tactile arr./Mouvement/Réglages.
- Feedback visuel live + affichage IP/port et statut Wi-Fi.

**Phase 4 — Config persistante**
- Modèle de config JSON, chargement/sauvegarde sur carte mémoire, preset par défaut, application à chaud.

**Phase 5 — Serveur HTTP de config sur la Vita**
- Endpoints `GET/POST /config`, `GET /status`. Validation + application à chaud.

**Phase 6 — Webapp companion**
- Éditeur à onglets, réglages cible/préfixe/fréquence, import/export JSON, GET/POST vers la Vita.

**Phase 7 — Preview live (option)**
- WebSocket état Vita → visualisation dans la webapp.

**Phase 8 — Finition**
- Presets fournis (TouchDesigner, Resolume…), doc utilisateur, packaging VPK, tests de latence/stabilité, gestion des erreurs réseau.

---

## 13. Points d'attention / risques

- **Latence & débit** : les capteurs de mouvement à 60–120 Hz × plusieurs axes peuvent saturer le réseau — d'où `onChange`, throttling et bundles. À mesurer tôt (Phase 2).
- **Wi-Fi Vita** : la pile réseau Vita nécessite l'init des modules `SceNet`/`SceNetCtl` et un buffer mémoire dédié ; prévoir la reconnexion.
- **Multitouch** : bien gérer l'**apparition/disparition** des points (IDs de doigts) pour ne pas laisser des valeurs « collées ».
- **Coordonnées tactiles** : le tactile avant et le pavé arrière ont des résolutions/plages différentes — normaliser chacun correctement.
- **CFW requis** : documenter clairement le prérequis HENkaku/Enso pour les utilisateurs.
- **Sécurité LAN** : le serveur HTTP écoute sur le réseau — restreindre au LAN, token optionnel.

---

## 14. Vérification (à faire en continu)

- Test de réception OSC réel dans **TouchDesigner** (CHOP OSC In) et au moins un 2ᵉ logiciel (Chataigne/Protokol) dès la Phase 1.
- **Protokol** (Hexler) ou un moniteur OSC pour inspecter adresses/types/valeurs.
- Mesure de **latence** et de **perte de paquets** sous charge (tous capteurs actifs).
- Test d'**aller-retour config** webapp ↔ Vita (GET puis POST puis application à chaud).

---

### Sources techniques
- [Vita SDK — site officiel](https://vitasdk.org/)
- [VitaSDK — module Network](https://docs.vitasdk.org/group__Net.html)
- [VitaSDK — Socket Library (SceNet)](https://docs.vitasdk.org/group__SceNet.html)
- [VitaSDK — samples](https://github.com/vitasdk/samples)
