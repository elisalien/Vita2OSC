/*
 * config_json.h — (dé)sérialisation de Config <-> JSON.
 * Format aligné sur PLAN_DE_TRAVAIL.md (une "source" par contrôle).
 */
#ifndef VITA2OSC_CONFIG_JSON_H
#define VITA2OSC_CONFIG_JSON_H

#include "config.h"

/* Écrit le JSON de cfg dans out. Retourne la longueur, ou -1 si trop court. */
int config_to_json(const Config *cfg, char *out, int out_size);

/* Parse `text` (mutable, modifié en place) et remplit cfg.
 * Retourne 0 si OK, <0 sinon. En cas d'erreur, cfg peut être partiel. */
int config_from_json(Config *cfg, char *text);

#endif /* VITA2OSC_CONFIG_JSON_H */
