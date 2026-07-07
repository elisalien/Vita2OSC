/*
 * storage.h — Persistance de la config sur la carte mémoire.
 * Fichier : ux0:data/vita2osc/config.json
 */
#ifndef VITA2OSC_STORAGE_H
#define VITA2OSC_STORAGE_H

#include "config.h"

#define STORAGE_DIR  "ux0:data/vita2osc"
#define STORAGE_PATH "ux0:data/vita2osc/config.json"

/* Sauvegarde cfg en JSON. Retourne 0 si OK. */
int storage_save_config(const Config *cfg);

/* Charge la config si le fichier existe. Retourne 0 si chargée, <0 sinon
 * (dans ce cas, l'appelant garde le preset par défaut). */
int storage_load_config(Config *cfg);

#endif /* VITA2OSC_STORAGE_H */
