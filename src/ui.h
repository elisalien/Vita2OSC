/*
 * ui.h — Interface à onglets (vita2d).
 */
#ifndef VITA2OSC_UI_H
#define VITA2OSC_UI_H

#include "config.h"
#include "input.h"
#include "colors.h"

typedef enum {
    TAB_BUTTONS = 0,
    TAB_STICKS,
    TAB_TOUCH_FRONT,
    TAB_TOUCH_BACK,
    TAB_MOTION,
    TAB_COLORS,
    TAB_SETTINGS,
    TAB_COUNT
} UiTab;

int  ui_init(void);
void ui_finish(void);

/* Dessine une frame complète. status_line = infos réseau/paquets. */
void ui_render(const Config *cfg, const InputState *st, UiTab tab,
               const char *local_ip, int net_ok, int pps);

#endif /* VITA2OSC_UI_H */
