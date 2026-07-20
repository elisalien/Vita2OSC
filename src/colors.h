/*
 * colors.h — Page Couleurs : effets Colorize / Dreamy Glow, pickers HSBA,
 * toggles de groupes. Tout au tactile.
 */
#ifndef VITA2OSC_COLORS_H
#define VITA2OSC_COLORS_H

#include <stdint.h>

struct vita2d_pgf;

#define COLOR_ADDR_LEN   64
#define COLOR_MAX_GROUPS 8
#define COLOR_MAX_EFFECTS 2

typedef struct {
    char  address[COLOR_ADDR_LEN];
    float value;
} ColorParam;

typedef struct {
    char address[COLOR_ADDR_LEN];
    int  enabled;
} ColorGroup;

typedef struct {
    char        id[24];
    char        name[32];
    char        effect_address[COLOR_ADDR_LEN];
    ColorParam  hue, sat, bright, alpha;
    ColorGroup  groups[COLOR_MAX_GROUPS];
    int         group_count;
} ColorEffect;

typedef struct {
    ColorEffect effects[COLOR_MAX_EFFECTS];
    int         effect_count;
    int         selected;
    int         auto_send;
} ColorConfig;

void colors_load_default(ColorConfig *cc);

/* Dessine la page Couleurs dans le rectangle de contenu donné. */
void colors_draw(const ColorConfig *cc, struct vita2d_pgf *font,
                 int x, int y, int w, int h);

/* Tap (front montant) : onglets d'effet, toggles de groupe, boutons.
 * Retourne 1 si consommé. */
int colors_touch_press(ColorConfig *cc, int px, int py);

/* Glissement (doigt maintenu) : barres H/S/B/A. Retourne 1 si consommé. */
int colors_touch_drag(ColorConfig *cc, int px, int py);

/* Envoie les paramètres couleur + toggles groupes pour un effet. */
int colors_send_effect(const ColorEffect *fx, int include_toggles);

/* Envoie tous les effets (groupes inclus). */
int colors_send_all(const ColorConfig *cc);

#endif /* VITA2OSC_COLORS_H  */
