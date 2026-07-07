# Presets OSC de référence

Deux configurations prêtes à charger dans Vita2OSC, une par logiciel. Elles suivent le schéma défini dans `PLAN_DE_TRAVAIL.md`.

- `preset_touchdesigner.json`
- `preset_resolume.json`

> Avant usage : remplace `target.ip` par l'IP de ta machine qui reçoit l'OSC (celle où tourne TouchDesigner ou Resolume), visible sur le même réseau que la Vita.

---

## Pourquoi deux presets différents

| Aspect | TouchDesigner | Resolume |
|---|---|---|
| **Port OSC par défaut** | libre (ici **9000**) | **7000** (port d'entrée OSC natif de Resolume) |
| **Plage des valeurs** | sticks en **−1…+1**, capteurs en unités « réelles » | **tout en 0…1** (Resolume remappe 0…1 vers la plage du paramètre) |
| **Boutons** | float 0/1, mode `onChange` | float, mode `trigger` (envoie **1.0** à l'appui) |
| **Fréquence** | **60 Hz** (TD encaisse bien) | **30 Hz** (suffisant, plus léger) |
| **Bundles** | oui (X/Y/pression groupés) | non nécessaire (assignation param par param) |
| **Type de données** | `f` partout (TD accepte aussi `i`/bool) | `f` partout (Resolume préfère un seul argument float) |

La logique : TouchDesigner est un environnement de flux de données générique qui aime les valeurs brutes normalisées et les hauts débits ; Resolume est un logiciel de VJing où chaque message OSC pilote un paramètre précis, avec une convention 0…1 et des déclencheurs.

---

## Brancher côté TouchDesigner

1. Ajoute un CHOP **OSC In**.
2. Règle **Network Port** sur `9000` (même valeur que `target.port` du preset).
3. Les canaux apparaissent, nommés d'après les adresses OSC (`/vita/stick/lx`, `/vita/pad/cross`, …).
4. Les sticks arrivent en −1…+1, le tactile et l'orientation en 0…1 ou plages indiquées.

Astuce : un CHOP **Rename** ou **Select** derrière l'OSC In pour ne garder que les canaux utiles.

---

## Brancher côté Resolume

1. `Menu > Preferences > OSC` : active **OSC Input**, port **7000** (= `target.port`).
2. `Shortcuts > Edit OSC` (mode apprentissage des raccourcis OSC).
3. Sur la Vita, **actionne le contrôle** à assigner (bouge un stick, appuie un bouton) → Resolume capte l'adresse (`/vita/...`).
4. Clique dans Resolume le **paramètre** cible (un fader d'opacité, un bouton de clip…) pour lier l'adresse.
5. Dans l'éditeur de raccourci, règle **Low/High** (par défaut 0…1) selon la plage du paramètre.

Notes :
- Les boutons sont en mode `trigger` (envoi de **1.0**) → parfait pour **connecter un clip** ou déclencher une action.
- Pour un contrôle continu (fader, effet), utilise un stick, le tactile ou l'orientation (tous déjà en 0…1).
- Resolume gère mal les messages **sans argument** : c'est pourquoi les boutons envoient une valeur (`1.0`) et non un simple « bang ».

---

## Adapter à un autre logiciel

Ces presets sont des points de départ. Pour Chataigne, Max/MSP, Ableton (via OSC), etc. :

- change **le port** pour celui qu'écoute le logiciel ;
- ajuste **la plage** (0…1 ou −1…+1 selon la convention attendue) ;
- garde les **adresses** ou renomme-les via le préfixe global.
