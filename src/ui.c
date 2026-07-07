#include "ui.h"
#include "mapping.h"

#include <stdio.h>
#include <string.h>
#include <vita2d.h>

#define SCR_W 960
#define SCR_H 544

static vita2d_pgf *s_font = NULL;

#define APP_VERSION "1.2"

static const char *TAB_NAMES[TAB_COUNT] = {
    "Boutons", "Sticks", "Touch Av", "Touch Ar", "Mouvement", "Couleurs", "Reglages"
};
/* 7 onglets × 128 px */
#define TAB_W 128
#define TAB_H 46
#define CONTENT_Y 78

/* Palette */
#define COL_BG     RGBA8(0x14, 0x16, 0x1c, 0xFF)
#define COL_BAR    RGBA8(0x22, 0x26, 0x30, 0xFF)
#define COL_ACCENT RGBA8(0x3d, 0x9b, 0xff, 0xFF)
#define COL_TEXT   RGBA8(0xEC, 0xEF, 0xF4, 0xFF)
#define COL_DIM    RGBA8(0x8a, 0x92, 0xa6, 0xFF)
#define COL_OK     RGBA8(0x53, 0xd7, 0x69, 0xFF)
#define COL_WARN   RGBA8(0xff, 0x9f, 0x43, 0xFF)

int ui_init(void) {
    vita2d_init();
    vita2d_set_clear_color(COL_BG);
    s_font = vita2d_load_default_pgf();
    return s_font ? 0 : -1;
}

void ui_finish(void) {
    if (s_font) { vita2d_free_pgf(s_font); s_font = NULL; }
    vita2d_fini();
}

static void text(int x, int y, unsigned int col, const char *s) {
    vita2d_pgf_draw_text(s_font, x, y, col, 1.0f, s);
}

/* Quelle catégorie d'onglet pour une source donnée. */
static int src_tab(SrcId src) {
    if (src >= SRC_BTN_CROSS && src <= SRC_BTN_SELECT) return TAB_BUTTONS;
    if (src >= SRC_STICK_LX && src <= SRC_STICK_RY)    return TAB_STICKS;
    if (src >= SRC_TF0_X && src <= SRC_TF1_Z)          return TAB_TOUCH_FRONT;
    if (src >= SRC_TB0_X && src <= SRC_TB0_Y)          return TAB_TOUCH_BACK;
    if (src >= SRC_ACC_X && src <= SRC_ORI_YAW)        return TAB_MOTION;
    return -1;
}

static void draw_tab_bar(UiTab tab) {
    vita2d_draw_rectangle(0, 0, SCR_W, TAB_H, COL_BAR);
    int x = 8;
    for (int i = 0; i < TAB_COUNT; i++) {
        if ((int)tab == i) {
            vita2d_draw_rectangle(x, 6, TAB_W - 4, 34, COL_ACCENT);
            text(x + 6, 30, RGBA8(0,0,0,0xFF), TAB_NAMES[i]);
        } else {
            text(x + 6, 30, COL_DIM, TAB_NAMES[i]);
        }
        x += TAB_W;
    }
    text(8, SCR_H - 10, COL_DIM, "L / R : onglet    SELECT+START : quitter");
}

/* Liste générique : label | valeur live | adresse, pour les contrôles d'un onglet. */
static void draw_control_list(const Config *cfg, const InputState *st, UiTab tab) {
    int y = CONTENT_Y + 14;
    char line[128];
    for (int i = 0; i < cfg->control_count; i++) {
        const Control *c = &cfg->controls[i];
        if (src_tab(c->src) != (int)tab) continue;

        float v = mapping_preview_value(cfg, c, st);
        unsigned int col = c->enabled ? COL_TEXT : COL_DIM;

        snprintf(line, sizeof(line), "%-12s % 7.3f", input_source_label(c->src), v);
        text(24, y, col, line);

        /* petite barre de niveau (normalisée entre out_min/out_max) */
        float lo = c->out_min, hi = c->out_max;
        float t = (hi != lo) ? (v - lo) / (hi - lo) : 0.0f;
        if (t < 0) t = 0; if (t > 1) t = 1;
        vita2d_draw_rectangle(250, y - 12, 160, 12, COL_BAR);
        vita2d_draw_rectangle(250, y - 12, (int)(160 * t), 12, COL_ACCENT);

        text(430, y, COL_DIM, c->address);
        snprintf(line, sizeof(line), "%s/%s", config_type_tag(c->type), config_mode_label(c->mode));
        text(760, y, COL_DIM, line);

        y += 24;
        if (y > SCR_H - 20) break;
    }
}

static void draw_touch_visual_front(const InputState *st) {
    /* Cadre représentant l'écran avant. */
    int bx = 560, by = 90, bw = 360, bh = 200;
    vita2d_draw_rectangle(bx, by, bw, bh, COL_BAR);
    for (int i = 0; i < INPUT_TOUCH_FRONT_POINTS; i++) {
        if (!st->front[i].active) continue;
        int px = bx + (int)(st->front[i].x * bw);
        int py = by + (int)(st->front[i].y * bh);
        int r = 8 + (int)(st->front[i].force * 14);
        vita2d_draw_fill_circle(px, py, r, COL_ACCENT);
    }
    text(bx, by - 8, COL_DIM, "Zone tactile avant");
}

static void draw_settings(const Config *cfg, const char *local_ip, int net_ok, int pps) {
    char line[160];
    int y = CONTENT_Y + 8;
    snprintf(line, sizeof(line), "Version %s", APP_VERSION);
    text(24, y, COL_DIM, line); y += 28;

    text(24, y, COL_TEXT, "Cible OSC"); y += 28;
    snprintf(line, sizeof(line), "  IP     : %s", cfg->target_ip);   text(24, y, COL_TEXT, line); y += 24;
    snprintf(line, sizeof(line), "  Port   : %u", cfg->target_port); text(24, y, COL_TEXT, line); y += 24;
    snprintf(line, sizeof(line), "  Prefixe: %s", cfg->global_prefix); text(24, y, COL_TEXT, line); y += 24;
    snprintf(line, sizeof(line), "  Debit  : %d Hz", cfg->send_rate_hz); text(24, y, COL_TEXT, line); y += 32;

    text(24, y, COL_TEXT, "Reseau Vita"); y += 28;
    snprintf(line, sizeof(line), "  IP locale : %s", local_ip); text(24, y, COL_TEXT, line); y += 24;
    snprintf(line, sizeof(line), "  Etat OSC  : %s", net_ok ? "PRET (envoi actif)" : "cible non joignable");
    text(24, y, net_ok ? COL_OK : COL_WARN, line); y += 24;
    snprintf(line, sizeof(line), "  Paquets/s : %d", pps); text(24, y, COL_DIM, line); y += 24;
    snprintf(line, sizeof(line), "  Webapp : http://%s:8080", local_ip);
    text(24, y, COL_DIM, line); y += 20;
    text(24, y, COL_DIM, "  GET/POST /config  POST /osc");
    y += 32;

    text(24, y, COL_DIM, "Onglet Couleurs : toucher barres H/S/B/A, groupes.");
}

void ui_render(const Config *cfg, const InputState *st, UiTab tab,
               const char *local_ip, int net_ok, int pps) {
    vita2d_start_drawing();
    vita2d_clear_screen();

    draw_tab_bar(tab);

    /* Bandeau état réseau (masqué sur Couleurs pour libérer l'espace). */
    if (tab != TAB_COLORS) {
        char status[96];
        snprintf(status, sizeof(status), "Vita2OSC  ->  %s:%u   [%s]",
                 cfg->target_ip, cfg->target_port, net_ok ? "OK" : "...");
        text(24, CONTENT_Y - 4, net_ok ? COL_OK : COL_WARN, status);
    }

    switch (tab) {
        case TAB_TOUCH_FRONT:
            draw_control_list(cfg, st, tab);
            draw_touch_visual_front(st);
            break;
        case TAB_COLORS:
            colors_draw(&cfg->colors, s_font);
            break;
        case TAB_SETTINGS:
            draw_settings(cfg, local_ip, net_ok, pps);
            break;
        default:
            draw_control_list(cfg, st, tab);
            break;
    }

    vita2d_end_drawing();
    vita2d_swap_buffers();
}
