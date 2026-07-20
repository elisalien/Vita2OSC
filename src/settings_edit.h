/*
 * settings_edit.h — Édition de la cible OSC (IP + port) sur la Vita.
 *
 * Deux modes complémentaires :
 *  - D-pad : gauche/droite sélectionne un champ, haut/bas ajuste (avec
 *    répétition à l'appui long), START applique.
 *  - Tactile : toucher un champ ouvre le clavier système (IME),
 *    toucher [Appliquer] applique. △ ouvre aussi le clavier.
 */
#ifndef VITA2OSC_SETTINGS_EDIT_H
#define VITA2OSC_SETTINGS_EDIT_H

#include "config.h"
#include "input.h"
#include <vita2d.h>

typedef enum {
    SEL_OCTET0 = 0,
    SEL_OCTET1,
    SEL_OCTET2,
    SEL_OCTET3,
    SEL_PORT,
    SEL_COUNT
} SettingsSel;

/* Actions demandées par l'éditeur (retour de handle/touch). */
typedef enum {
    SED_NONE = 0,
    SED_APPLY,        /* appliquer IP/port à la config        */
    SED_IME_IP,       /* ouvrir le clavier pour l'IP          */
    SED_IME_PORT      /* ouvrir le clavier pour le port       */
} SedAction;

typedef struct { int x, y, w, h; } SedRect;

typedef struct {
    uint8_t     octets[4];
    uint16_t    port;
    SettingsSel sel;
    int         dirty;

    /* zones tactiles, mises à jour par settings_edit_draw() */
    SedRect z_octet[4];
    SedRect z_port;
    SedRect z_keyboard;
    SedRect z_apply;
} SettingsEdit;

void settings_edit_from_config(SettingsEdit *e, const Config *cfg);
void settings_edit_to_config(const SettingsEdit *e, Config *cfg);

/* Applique une chaîne "a.b.c.d" (retour 1 si valide) / un port texte. */
int settings_edit_set_ip(SettingsEdit *e, const char *ip);
int settings_edit_set_port(SettingsEdit *e, const char *port_txt);
void settings_edit_ip_string(const SettingsEdit *e, char *out, int out_size);

/* Entrées D-pad + △ (répétition à l'appui long). */
SedAction settings_edit_handle(SettingsEdit *e, const InputState *st);

/* Tap tactile (coordonnées écran). */
SedAction settings_edit_touch(SettingsEdit *e, int px, int py);

/* Dessine le bloc d'édition dans la carte Réglages. (x, y) = coin
 * haut-gauche du bloc ; enregistre les zones tactiles. */
void settings_edit_draw(SettingsEdit *e, int x, int y, int w, vita2d_pgf *font);

#endif /* VITA2OSC_SETTINGS_EDIT_H */
