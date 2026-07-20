/*
 * ui.h — Interface Vita2OSC (vita2d), design "glasscore".
 *
 * Navigation : rail latéral gauche (tactile) + L/R. Chaque page occupe la
 * zone de contenu ; le rail affiche logo pixel art, icônes, batterie, réseau.
 */
#ifndef VITA2OSC_UI_H
#define VITA2OSC_UI_H

#include "config.h"
#include "input.h"
#include "colors.h"
#include "settings_edit.h"

typedef enum {
    TAB_BUTTONS = 0,
    TAB_STICKS,
    TAB_TOUCH,       /* tactile avant + arrière réunis */
    TAB_MOTION,
    TAB_COLORS,
    TAB_SETTINGS,
    TAB_COUNT
} UiTab;

/* État transverse affiché par l'UI (rempli par main). */
typedef struct {
    int   net_ok;      /* cible OSC joignable                 */
    int   pps;         /* paquets envoyés / seconde           */
    int   battery;     /* 0..100, -1 = inconnu                */
    int   charging;    /* en charge                           */
    float exit_t;      /* progression du maintien quitter 0..1 */
} UiStatus;

/* Actions émises par l'UI vers main (tap sur Réglages, etc.). */
typedef enum {
    UI_ACT_NONE = 0,
    UI_ACT_APPLY_SETTINGS,
    UI_ACT_IME_IP,
    UI_ACT_IME_PORT
} UiAction;

int  ui_init(void);
void ui_finish(void);

/* Splash de démarrage : anime la barre de progression jusqu'à `target`
 * (0..1) avec le libellé d'étape, en rendant quelques frames. */
void ui_splash_to(float target, const char *label);

/* Toast de feedback (kind : 0 = info, 1 = ok, 2 = warn/erreur). */
void ui_toast(const char *msg, int kind);

/* Dessine une frame complète. */
void ui_render(const Config *cfg, const InputState *st, UiTab tab,
               const char *local_ip, const UiStatus *status,
               SettingsEdit *sedit);

/* Tap tactile (front montant), coordonnées écran. Peut changer d'onglet
 * (rail) ou renvoyer une action Réglages. */
UiAction ui_touch_press(Config *cfg, SettingsEdit *sedit, UiTab *tab,
                        int px, int py);

/* Glissement (doigt maintenu) : sliders de la page Couleurs. */
void ui_touch_drag(Config *cfg, UiTab tab, int px, int py);

#endif /* VITA2OSC_UI_H */
