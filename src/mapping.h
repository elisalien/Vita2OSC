/*
 * mapping.h — Moteur : transforme l'état des entrées en messages OSC et les émet.
 */
#ifndef VITA2OSC_MAPPING_H
#define VITA2OSC_MAPPING_H

#include "config.h"
#include "input.h"

/* Réinitialise l'état runtime (lissage, dernières valeurs) de tous les contrôles.
 * À appeler après un (re)chargement de config. */
void mapping_reset(Config *cfg);

/* Un tick : calcule les valeurs, applique deadzone/lissage/invert/mode et
 * envoie les messages/bundles OSC dus. Retourne le nombre de paquets envoyés. */
int  mapping_tick(Config *cfg, const InputState *st);

/* Valeur de sortie courante d'un contrôle (pour l'affichage UI), sans émettre. */
float mapping_preview_value(const Config *cfg, const Control *c, const InputState *st);

/* Quand 0, le tactile n'est pas émis en OSC (onglet Couleurs). */
void mapping_set_touch_osc(int enabled);

#endif /* VITA2OSC_MAPPING_H */
