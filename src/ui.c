#include "ui.h"
#include "mapping.h"
#include "theme.h"
#include "version.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <vita2d.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static vita2d_pgf *s_font = NULL;

/* ---------------- état d'animation ---------------- */
static float s_fade = 0.0f;          /* voile de transition entre pages   */
static UiTab s_last_tab = TAB_BUTTONS;
static float s_rail_y = 0.0f;        /* position animée de l'indicateur   */
static float s_splash_t = 0.0f;

/* ---------------- toast ---------------- */
static char  s_toast_msg[80] = "";
static int   s_toast_kind = 0;
static int   s_toast_frames = 0;     /* frames restantes                  */
#define TOAST_LIFE 150               /* ~2.5 s à 60 fps                   */

/* ---------------- rail ---------------- */
#define RAIL_ITEM_H   54
#define RAIL_ITEMS_Y  96

static const char *TAB_NAMES[TAB_COUNT] = {
    "Boutons", "Sticks", "Tactile", "Mouvement", "Couleurs", "Reglages"
};
static const char *TAB_SUBTITLES[TAB_COUNT] = {
    "Appuie sur la console pour tester chaque bouton",
    "Position live des sticks, zone morte visualisee",
    "Ecran avant et pave arriere en temps reel",
    "Accelerometre, gyroscope et orientation",
    "Effets couleur OSC - tout au tactile",
    "Cible OSC, reseau et webapp companion"
};

/* en-tête : zone du pill cible (tap = aller aux réglages) */
static struct { int x, y, w, h; } s_target_pill;

/* ------------------------------------------------------------------ */
static unsigned int with_alpha(unsigned int col, unsigned int a) {
    return (col & 0x00FFFFFFu) | (a << 24);
}

static void trunc_str(char *dst, int n, const char *src) {
    int i = 0;
    for (; src[i] && i < n - 1; i++) dst[i] = src[i];
    dst[i] = '\0';
    if (src[i] && i >= 3) { dst[i - 1] = '.'; dst[i - 2] = '.'; }
}

static void draw_circle_outline(float cx, float cy, float r, unsigned int col) {
    const int seg = 40;
    float px = cx + r, py = cy;
    for (int i = 1; i <= seg; i++) {
        float a = (float)i / seg * 2.0f * (float)M_PI;
        float nx = cx + r * cosf(a), ny = cy + r * sinf(a);
        vita2d_draw_line(px, py, nx, ny, col);
        px = nx; py = ny;
    }
}

/* ------------------------------------------------------------------ */
int ui_init(void) {
    if (vita2d_init() < 0)
        return -1;
    vita2d_set_clear_color(TH_BG);
    s_font = vita2d_load_default_pgf();
    s_rail_y = (float)RAIL_ITEMS_Y;
    if (!s_font) {
        vita2d_fini();
        return -1;
    }
    return 0;
}

void ui_finish(void) {
    if (s_font) { vita2d_free_pgf(s_font); s_font = NULL; }
    vita2d_fini();
}

void ui_toast(const char *msg, int kind) {
    trunc_str(s_toast_msg, sizeof(s_toast_msg), msg ? msg : "");
    s_toast_kind = kind;
    s_toast_frames = TOAST_LIFE;
}

/* ------------------------------------------------------------------ */
/* Splash de démarrage : logo pixel art + progression.                 */
/* ------------------------------------------------------------------ */
static void splash_frame(const char *label) {
    vita2d_start_drawing();
    vita2d_clear_screen();
    th_background();

    int lx = (TH_SCR_W - 32 * 5) / 2;
    th_pixel_logo(lx, 96, 5);

    /* marque */
    int tw = th_text_w(s_font, TH_FS_HERO, "VITA") +
             th_text_w(s_font, TH_FS_HERO, "2OSC");
    int tx = (TH_SCR_W - tw) / 2;
    th_text(s_font, tx, 316, TH_TEXT, TH_FS_HERO, "VITA");
    th_text(s_font, tx + th_text_w(s_font, TH_FS_HERO, "VITA"), 316,
            TH_ACCENT, TH_FS_HERO, "2OSC");
    th_text_c(s_font, TH_SCR_W / 2, 344, TH_DIM, TH_FS_SMALL,
              "Controleur OSC sans-fil");

    /* progression */
    int bw = 300;
    th_hbar((TH_SCR_W - bw) / 2, 396, bw, 8, s_splash_t, TH_ACCENT);
    th_text_c(s_font, TH_SCR_W / 2, 428, TH_FAINT, TH_FS_MICRO,
              label ? label : "");

    th_text_c(s_font, TH_SCR_W / 2, TH_SCR_H - 22, TH_FAINT, TH_FS_MICRO,
              "v" VITA2OSC_VERSION);

    vita2d_end_drawing();
    vita2d_swap_buffers();
}

void ui_splash_to(float target, const char *label) {
    if (target > 1.0f) target = 1.0f;
    int guard = 0;
    while (s_splash_t < target - 0.005f && guard++ < 40) {
        s_splash_t += (target - s_splash_t) * 0.22f + 0.004f;
        splash_frame(label);
    }
    s_splash_t = target;
    splash_frame(label);
}

/* ------------------------------------------------------------------ */
/* Icônes du rail (primitives, ~22 px)                                 */
/* ------------------------------------------------------------------ */
static void icon_draw(int tab, int cx, int cy, unsigned int col) {
    switch (tab) {
        case TAB_BUTTONS: /* 4 points en losange */
            vita2d_draw_fill_circle(cx, cy - 8, 3.4f, col);
            vita2d_draw_fill_circle(cx + 8, cy, 3.4f, col);
            vita2d_draw_fill_circle(cx, cy + 8, 3.4f, col);
            vita2d_draw_fill_circle(cx - 8, cy, 3.4f, col);
            break;
        case TAB_STICKS:  /* anneau + dot excentré */
            draw_circle_outline(cx, cy, 10, col);
            vita2d_draw_fill_circle(cx + 4, cy - 3, 3.6f, col);
            break;
        case TAB_TOUCH:   /* écran arrondi + point */
            th_rrect_outline(cx - 11, cy - 8, 22, 16, 4, col);
            vita2d_draw_fill_circle(cx + 2, cy + 1, 3.0f, col);
            break;
        case TAB_MOTION:  /* trois ondes */
            for (int i = 0; i < 3; i++) {
                int yy = cy - 6 + i * 6;
                for (int xx = -9; xx < 9; xx += 3) {
                    float y0 = yy + 2.2f * sinf((float)(xx) * 0.5f);
                    float y1 = yy + 2.2f * sinf((float)(xx + 3) * 0.5f);
                    vita2d_draw_line(cx + xx, y0, cx + xx + 3, y1, col);
                }
            }
            break;
        case TAB_COLORS:  /* triade de pastilles */
            vita2d_draw_fill_circle(cx - 4, cy - 4, 4.4f, col);
            vita2d_draw_fill_circle(cx + 5, cy - 2, 4.4f, with_alpha(col, 0xB0));
            vita2d_draw_fill_circle(cx, cy + 5, 4.4f, with_alpha(col, 0x70));
            break;
        case TAB_SETTINGS: { /* engrenage simple */
            draw_circle_outline(cx, cy, 7, col);
            vita2d_draw_fill_circle(cx, cy, 2.4f, col);
            for (int i = 0; i < 8; i++) {
                float a = (float)i * (float)M_PI / 4.0f;
                vita2d_draw_line(cx + 7.0f * cosf(a), cy + 7.0f * sinf(a),
                                 cx + 10.5f * cosf(a), cy + 10.5f * sinf(a), col);
            }
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Rail latéral : logo, navigation, batterie, réseau                   */
/* ------------------------------------------------------------------ */
static void draw_rail(UiTab tab, const UiStatus *stt) {
    vita2d_draw_rectangle(0, 0, TH_RAIL_W, TH_SCR_H, TH_GLASS);
    vita2d_draw_rectangle(TH_RAIL_W - 1, 0, 1, TH_SCR_H, TH_GLASS_EDGE);

    /* logo pixel art (32x32 x2) */
    th_pixel_logo((TH_RAIL_W - 64) / 2, 12, 2);

    /* indicateur animé */
    float target = (float)(RAIL_ITEMS_Y + (int)tab * RAIL_ITEM_H);
    s_rail_y += (target - s_rail_y) * 0.30f;
    th_rrect(8, (int)s_rail_y + 3, TH_RAIL_W - 16, RAIL_ITEM_H - 6, 12,
             TH_ACCENT_SOFT);
    th_rrect_outline(8, (int)s_rail_y + 3, TH_RAIL_W - 16, RAIL_ITEM_H - 6, 12,
                     with_alpha(TH_ACCENT, 0x60));

    for (int i = 0; i < TAB_COUNT; i++) {
        int cy = RAIL_ITEMS_Y + i * RAIL_ITEM_H + RAIL_ITEM_H / 2;
        icon_draw(i, TH_RAIL_W / 2, cy - 2,
                  ((int)tab == i) ? TH_ACCENT : TH_FAINT);
    }

    /* réseau + batterie en bas */
    int by = TH_SCR_H - 74;
    th_glow_dot(TH_RAIL_W / 2, by, 4, stt->net_ok ? TH_OK : TH_WARN);

    by += 22;
    int bx = (TH_RAIL_W - 30) / 2;
    th_rrect_outline(bx, by, 26, 13, 3, TH_FAINT);
    vita2d_draw_rectangle(bx + 26, by + 4, 3, 5, TH_FAINT);
    if (stt->battery >= 0) {
        int fw = (int)((float)(22) * (float)stt->battery / 100.0f);
        unsigned int c = stt->charging ? TH_TEAL
                        : (stt->battery <= 20 ? TH_ERR : TH_OK);
        if (fw > 0) vita2d_draw_rectangle(bx + 2, by + 2, fw, 9, c);
        char pct[8];
        snprintf(pct, sizeof(pct), "%d", stt->battery);
        th_text_c(s_font, TH_RAIL_W / 2, by + 32, TH_FAINT, TH_FS_MICRO, pct);
    }
}

/* ------------------------------------------------------------------ */
/* En-tête et pied de page                                             */
/* ------------------------------------------------------------------ */
static void draw_header(const Config *cfg, UiTab tab, const UiStatus *stt) {
    char buf[64];

    th_text(s_font, TH_CONTENT_X, 38, TH_TEXT, TH_FS_HERO, TAB_NAMES[tab]);
    th_text(s_font, TH_CONTENT_X, 58, TH_FAINT, TH_FS_MICRO, TAB_SUBTITLES[tab]);

    /* pill cible OSC (tap = réglages) */
    snprintf(buf, sizeof(buf), "%s:%u", cfg->target_ip, cfg->target_port);
    int tw = th_text_w(s_font, TH_FS_SMALL, buf);
    int pw = tw + 44;
    int px = TH_SCR_W - 16 - pw;
    th_pill(px, 18, pw, 28, TH_GLASS);
    th_rrect_outline(px, 18, pw, 28, 14, TH_GLASS_EDGE);
    th_glow_dot(px + 16, 32, 3, stt->net_ok ? TH_OK : TH_WARN);
    th_text(s_font, px + 28, 37, stt->net_ok ? TH_TEXT : TH_DIM, TH_FS_SMALL, buf);
    s_target_pill.x = px; s_target_pill.y = 14;
    s_target_pill.w = pw; s_target_pill.h = 36;

    /* pps */
    snprintf(buf, sizeof(buf), "%d pps", stt->pps);
    th_text_r(s_font, px - 14, 37, TH_FAINT, TH_FS_MICRO, buf);

    vita2d_draw_rectangle(TH_CONTENT_X, TH_HEADER_H + 6,
                          TH_CONTENT_W, 1, TH_ROW_LINE);
}

static void draw_footer(UiTab tab) {
    const char *hint = "";
    switch (tab) {
        case TAB_COLORS:   hint = "toucher les barres / groupes"; break;
        case TAB_SETTINGS: hint = "TRIANGLE clavier - START appliquer"; break;
        default:           hint = "toucher le rail pour naviguer"; break;
    }
    th_text(s_font, TH_CONTENT_X, TH_FOOTER_Y + 14, TH_FAINT, TH_FS_MICRO,
            "L / R  page suivante");
    th_text_r(s_font, TH_SCR_W - 16, TH_FOOTER_Y + 14, TH_FAINT, TH_FS_MICRO,
              hint);
    th_text_c(s_font, TH_CONTENT_X + TH_CONTENT_W / 2, TH_FOOTER_Y + 14,
              TH_FAINT, TH_FS_MICRO, "SELECT + START  maintenir pour quitter");
}

/* ------------------------------------------------------------------ */
/* Liste générique de contrôles (n colonnes)                           */
/* ------------------------------------------------------------------ */
static int collect_controls(const Config *cfg, SrcId lo, SrcId hi,
                            int *idx, int max) {
    int n = 0;
    for (int i = 0; i < cfg->control_count && n < max; i++) {
        SrcId s = cfg->controls[i].src;
        if (s >= lo && s <= hi) idx[n++] = i;
    }
    return n;
}

#define ROW_H 27

static void draw_control_rows(const Config *cfg, const InputState *st,
                              const int *idx, int count,
                              int x, int y, int w, int h, int cols) {
    if (cols < 1) cols = 1;
    int col_w = w / cols;
    int rows_per_col = (h - 36) / ROW_H;
    char line[64];

    /* en-tête */
    th_text(s_font, x + 16, y + 20, TH_FAINT, TH_FS_MICRO, "SOURCE");
    th_text_r(s_font, x + w - 16, y + 20, TH_FAINT, TH_FS_MICRO,
              "VALEUR - ADRESSE OSC");
    vita2d_draw_rectangle(x + 10, y + 28, w - 20, 1, TH_ROW_LINE);

    for (int n = 0; n < count; n++) {
        int col = n / rows_per_col;
        int row = n % rows_per_col;
        if (col >= cols) break;

        const Control *c = &cfg->controls[idx[n]];
        int rx = x + col * col_w;
        int ry = y + 48 + row * ROW_H;

        float v = mapping_preview_value(cfg, c, st);
        float lo = c->out_min, hi = c->out_max;
        float t = (hi != lo) ? (v - lo) / (hi - lo) : 0.0f;

        vita2d_draw_fill_circle(rx + 20, ry - 5, 3.2f,
                                c->enabled ? TH_ACCENT : TH_FAINT);
        th_text(s_font, rx + 34, ry, c->enabled ? TH_TEXT : TH_FAINT,
                TH_FS_SMALL, input_source_label(c->src));

        th_hbar(rx + 138, ry - 11, 84, 9, t, c->enabled ? TH_ACCENT : TH_FAINT);

        snprintf(line, sizeof(line), "%.2f", v);
        th_text_r(s_font, rx + 272, ry, c->enabled ? TH_ACCENT : TH_FAINT,
                  TH_FS_SMALL, line);

        char addr[28];
        trunc_str(addr, (col_w > 400) ? 28 : 18, c->address);
        th_text_r(s_font, rx + col_w - 16, ry, TH_FAINT, TH_FS_MICRO, addr);

        vita2d_draw_rectangle(rx + 12, ry + 8, col_w - 24, 1, TH_ROW_LINE);
    }
}

/* ------------------------------------------------------------------ */
/* Page Boutons : console interactive + liste                          */
/* ------------------------------------------------------------------ */
static int pressed(const InputState *st, SrcId s) {
    return input_source_value(st, s) > 0.5f;
}

static void draw_face_glyph(int kind, float cx, float cy, unsigned int col) {
    switch (kind) {
        case 0: /* triangle */
            vita2d_draw_line(cx, cy - 7, cx + 6, cy + 5, col);
            vita2d_draw_line(cx + 6, cy + 5, cx - 6, cy + 5, col);
            vita2d_draw_line(cx - 6, cy + 5, cx, cy - 7, col);
            break;
        case 1: /* rond */
            draw_circle_outline(cx, cy, 6.5f, col);
            break;
        case 2: /* croix */
            vita2d_draw_line(cx - 6, cy - 6, cx + 6, cy + 6, col);
            vita2d_draw_line(cx - 6, cy + 6, cx + 6, cy - 6, col);
            break;
        case 3: /* carré */
            th_rrect_outline((int)cx - 6, (int)cy - 6, 13, 13, 2, col);
            break;
    }
}

static void page_buttons(const Config *cfg, const InputState *st) {
    int x = TH_CONTENT_X, y = TH_CONTENT_Y;
    int w = TH_CONTENT_W;
    int vh = 188;

    th_panel(x, y, w, vh);
    th_text(s_font, x + 16, y + 22, TH_FAINT, TH_FS_MICRO, "CONSOLE");

    /* gâchettes */
    int lp = pressed(st, SRC_BTN_L), rp = pressed(st, SRC_BTN_R);
    th_pill(x + 28, y + 14, 110, 24, lp ? TH_ACCENT : TH_INSET);
    th_text_c(s_font, x + 28 + 55, y + 31, lp ? TH_ON_ACCENT : TH_DIM,
              TH_FS_SMALL, "L");
    th_pill(x + w - 138, y + 14, 110, 24, rp ? TH_ACCENT : TH_INSET);
    th_text_c(s_font, x + w - 138 + 55, y + 31, rp ? TH_ON_ACCENT : TH_DIM,
              TH_FS_SMALL, "R");

    /* d-pad */
    int dx = x + 190, dy = y + 104, bs = 30, gap2 = 3;
    struct { SrcId s; int ox, oy; } dpad[4] = {
        { SRC_BTN_UP, 0, -(bs + gap2) }, { SRC_BTN_DOWN, 0, bs + gap2 },
        { SRC_BTN_LEFT, -(bs + gap2), 0 }, { SRC_BTN_RIGHT, bs + gap2, 0 },
    };
    th_rrect(dx - bs / 2, dy - bs / 2, bs, bs, 6, TH_INSET);
    for (int i = 0; i < 4; i++) {
        int p = pressed(st, dpad[i].s);
        th_rrect(dx + dpad[i].ox - bs / 2, dy + dpad[i].oy - bs / 2, bs, bs, 6,
                 p ? TH_ACCENT : TH_INSET);
        if (p) th_glow_dot(dx + dpad[i].ox, dy + dpad[i].oy, 3, TH_ACCENT);
    }

    /* boutons d'action */
    int fx = x + w - 190, fy = dy, off = 36;
    struct { SrcId s; int kind; int ox, oy; unsigned int col; } face[4] = {
        { SRC_BTN_TRIANGLE, 0, 0, -off, TH_OK },
        { SRC_BTN_CIRCLE,   1, off, 0,  TH_ERR },
        { SRC_BTN_CROSS,    2, 0, off,  TH_ACCENT },
        { SRC_BTN_SQUARE,   3, -off, 0, TH_VIOLET },
    };
    for (int i = 0; i < 4; i++) {
        int p = pressed(st, face[i].s);
        float bx = (float)(fx + face[i].ox), by2 = (float)(fy + face[i].oy);
        vita2d_draw_fill_circle(bx, by2, 17.0f,
                                p ? with_alpha(face[i].col, 0x50) : TH_INSET);
        if (p) th_glow_dot((int)bx, (int)by2, 4, face[i].col);
        draw_face_glyph(face[i].kind, bx, by2,
                        p ? face[i].col : TH_DIM);
    }

    /* start / select */
    int sp = pressed(st, SRC_BTN_START), se = pressed(st, SRC_BTN_SELECT);
    int cy2 = y + vh - 30;
    th_pill(x + w / 2 - 96, cy2, 88, 20, se ? TH_ACCENT : TH_INSET);
    th_text_c(s_font, x + w / 2 - 52, cy2 + 15, se ? TH_ON_ACCENT : TH_FAINT,
              TH_FS_MICRO, "SELECT");
    th_pill(x + w / 2 + 8, cy2, 88, 20, sp ? TH_ACCENT : TH_INSET);
    th_text_c(s_font, x + w / 2 + 52, cy2 + 15, sp ? TH_ON_ACCENT : TH_FAINT,
              TH_FS_MICRO, "START");

    /* écran central : mini logo */
    th_rrect(x + w / 2 - 70, y + 58, 140, 84, 10, TH_INSET);
    th_pixel_logo(x + w / 2 - 32, y + 68, 2);

    /* liste en 2 colonnes */
    int ly = y + vh + 12;
    int lh = TH_FOOTER_Y - 8 - ly;
    th_panel(x, ly, w, lh);
    int idx[16];
    int n = collect_controls(cfg, SRC_BTN_CROSS, SRC_BTN_SELECT, idx, 16);
    draw_control_rows(cfg, st, idx, n, x, ly, w, lh, 2);
}

/* ------------------------------------------------------------------ */
/* Page Sticks : deux pads XY + infos                                  */
/* ------------------------------------------------------------------ */
static void draw_stick_pad(const Config *cfg, const InputState *st,
                           int x, int y, int w, int h,
                           const char *title, float vx, float vy,
                           SrcId src_x) {
    th_panel(x, y, w, h);
    th_text(s_font, x + 16, y + 24, TH_FAINT, TH_FS_MICRO, title);

    float cx = (float)x + (float)w / 2.0f;
    float cy = (float)y + 46.0f + (float)(h - 130) / 2.0f;
    float r = (float)(h - 150) / 2.0f;
    if (r > (float)w / 2.0f - 30.0f) r = (float)w / 2.0f - 30.0f;

    vita2d_draw_fill_circle(cx, cy, r + 8, TH_INSET);
    draw_circle_outline(cx, cy, r + 8, TH_GLASS_EDGE);

    /* zone morte configurée */
    float dz = 0.0f;
    for (int i = 0; i < cfg->control_count; i++)
        if (cfg->controls[i].src == src_x) { dz = cfg->controls[i].deadzone; break; }
    if (dz > 0.01f) draw_circle_outline(cx, cy, r * dz, TH_ROW_LINE);

    /* croix de repère */
    vita2d_draw_line(cx - r, cy, cx + r, cy, TH_ROW_LINE);
    vita2d_draw_line(cx, cy - r, cx, cy + r, TH_ROW_LINE);

    /* position */
    float px = cx + vx * r, py = cy + vy * r;
    vita2d_draw_line(cx, cy, px, py, with_alpha(TH_ACCENT, 0x50));
    th_glow_dot((int)px, (int)py, 6, TH_ACCENT);

    char line[48];
    snprintf(line, sizeof(line), "X %+0.3f    Y %+0.3f", vx, vy);
    th_text_c(s_font, x + w / 2, y + h - 44, TH_TEXT, TH_FS_BODY, line);
    float mag = sqrtf(vx * vx + vy * vy);
    if (mag > 1.0f) mag = 1.0f;
    th_hbar(x + 40, y + h - 26, w - 80, 8, mag, TH_ACCENT);
}

static void page_sticks(const Config *cfg, const InputState *st) {
    int x = TH_CONTENT_X, y = TH_CONTENT_Y;
    int h = TH_FOOTER_Y - 8 - y;
    int pw = 318;

    draw_stick_pad(cfg, st, x, y, pw, h, "STICK GAUCHE",
                   st->stick_lx, st->stick_ly, SRC_STICK_LX);
    draw_stick_pad(cfg, st, x + pw + 12, y, pw, h, "STICK DROIT",
                   st->stick_rx, st->stick_ry, SRC_STICK_RX);

    /* infos mapping */
    int ix = x + 2 * (pw + 12);
    int iw = TH_CONTENT_W - 2 * (pw + 12);
    th_panel(ix, y, iw, h);
    th_text(s_font, ix + 16, y + 24, TH_FAINT, TH_FS_MICRO, "MAPPING");

    int iy = y + 52;
    for (int i = 0; i < cfg->control_count; i++) {
        const Control *c = &cfg->controls[i];
        if (c->src < SRC_STICK_LX || c->src > SRC_STICK_RY) continue;
        th_text(s_font, ix + 16, iy, c->enabled ? TH_TEXT : TH_FAINT,
                TH_FS_SMALL, input_source_label(c->src));
        char addr[24];
        trunc_str(addr, sizeof(addr), c->address);
        th_text(s_font, ix + 16, iy + 18, TH_FAINT, TH_FS_MICRO, addr);
        char mm[32];
        snprintf(mm, sizeof(mm), "%s  %+.0f..%+.0f%s",
                 config_mode_label(c->mode), c->out_min, c->out_max,
                 c->invert ? "  inv" : "");
        th_text(s_font, ix + 16, iy + 34, TH_FAINT, TH_FS_MICRO, mm);
        vita2d_draw_rectangle(ix + 12, iy + 44, iw - 24, 1, TH_ROW_LINE);
        iy += 56;
        if (iy > y + h - 30) break;
    }
    th_text(s_font, ix + 16, y + h - 16, TH_FAINT, TH_FS_MICRO,
            "Edition via la webapp");
}

/* ------------------------------------------------------------------ */
/* Page Tactile : avant + arrière                                      */
/* ------------------------------------------------------------------ */
static void draw_touch_zone(int x, int y, int w, int h, const char *title,
                            const TouchPoint *pts, int count,
                            const unsigned int *cols, int show_force) {
    th_panel(x, y, w, h);
    th_text(s_font, x + 16, y + 22, TH_FAINT, TH_FS_MICRO, title);

    int bx = x + 16, by = y + 34;
    int bw = w - 32, bh = h - 50;
    th_rrect(bx, by, bw, bh, 10, TH_INSET);
    vita2d_draw_rectangle(bx + bw / 2, by + 6, 1, bh - 12, TH_ROW_LINE);
    vita2d_draw_rectangle(bx + 6, by + bh / 2, bw - 12, 1, TH_ROW_LINE);

    for (int i = 0; i < count; i++) {
        if (!pts[i].active) continue;
        int tx = bx + (int)(pts[i].x * (float)bw);
        int ty = by + (int)(pts[i].y * (float)bh);
        int r = 6 + (show_force ? (int)(pts[i].force * 10.0f) : 0);
        th_glow_dot(tx, ty, r, cols[i % 3]);
        char id[8];
        snprintf(id, sizeof(id), "%d", i + 1);
        th_text(s_font, tx + r + 8, ty + 5, TH_DIM, TH_FS_MICRO, id);
    }
}

static void page_touch(const Config *cfg, const InputState *st) {
    int x = TH_CONTENT_X, y = TH_CONTENT_Y;
    static const unsigned int front_cols[3] = { TH_ACCENT, TH_VIOLET, TH_TEAL };
    static const unsigned int back_cols[3] = { TH_TEAL, TH_TEAL, TH_TEAL };

    int fh = 268;
    draw_touch_zone(x, y, 552, fh, "ECRAN AVANT  (multitouch)",
                    st->front, INPUT_TOUCH_FRONT_POINTS, front_cols, 1);
    draw_touch_zone(x + 564, y, TH_CONTENT_W - 564, fh, "PAVE ARRIERE",
                    st->back, INPUT_TOUCH_BACK_POINTS, back_cols, 0);

    int ly = y + fh + 12;
    int lh = TH_FOOTER_Y - 8 - ly;
    th_panel(x, ly, TH_CONTENT_W, lh);
    int idx[12];
    int n = collect_controls(cfg, SRC_TF0_X, SRC_TB0_Y, idx, 12);
    draw_control_rows(cfg, st, idx, n, x, ly, TH_CONTENT_W, lh, 2);
}

/* ------------------------------------------------------------------ */
/* Page Mouvement : accéléro / gyro / orientation                      */
/* ------------------------------------------------------------------ */
static void motion_bars(int x, int y, int w, const char *title,
                        const char *names[3], const float v[3],
                        const float range, unsigned int col) {
    th_text(s_font, x + 16, y + 22, TH_FAINT, TH_FS_MICRO, title);
    int by = y + 46;
    char buf[24];
    for (int i = 0; i < 3; i++) {
        th_text(s_font, x + 16, by + 12, TH_DIM, TH_FS_SMALL, names[i]);
        float t = v[i] / range;
        th_hbar_center(x + 46, by, w - 130, 14, t, col);
        snprintf(buf, sizeof(buf), "%+0.2f", v[i]);
        th_text_r(s_font, x + w - 16, by + 12, TH_TEXT, TH_FS_SMALL, buf);
        by += 40;
    }
}

static void page_motion(const Config *cfg, const InputState *st) {
    (void)cfg;
    int x = TH_CONTENT_X, y = TH_CONTENT_Y;
    int h = TH_FOOTER_Y - 8 - y;
    int cw = 272;

    static const char *xyz[3] = { "X", "Y", "Z" };
    static const char *rpy[3] = { "R", "P", "Y" };

    th_panel(x, y, cw, h);
    float acc[3] = { st->accel_x, st->accel_y, st->accel_z };
    motion_bars(x, y, cw, "ACCELEROMETRE  (G)", xyz, acc, 2.0f, TH_ACCENT);
    /* bulle de niveau (accel x/y) */
    float bcx = (float)x + cw / 2.0f, bcy = (float)y + h - 118.0f;
    vita2d_draw_fill_circle(bcx, bcy, 74, TH_INSET);
    draw_circle_outline(bcx, bcy, 74, TH_GLASS_EDGE);
    vita2d_draw_line(bcx - 74, bcy, bcx + 74, bcy, TH_ROW_LINE);
    vita2d_draw_line(bcx, bcy - 74, bcx, bcy + 74, TH_ROW_LINE);
    float ax = st->accel_x / 2.0f, ay = st->accel_y / 2.0f;
    if (ax > 1) ax = 1;
    if (ax < -1) ax = -1;
    if (ay > 1) ay = 1;
    if (ay < -1) ay = -1;
    th_glow_dot((int)(bcx + ax * 66.0f), (int)(bcy + ay * 66.0f), 5, TH_ACCENT);

    int x2 = x + cw + 12;
    th_panel(x2, y, cw, h);
    float gyr[3] = { st->gyro_x, st->gyro_y, st->gyro_z };
    motion_bars(x2, y, cw, "GYROSCOPE  (rad/s)", xyz, gyr, 5.0f, TH_VIOLET);

    int x3 = x2 + cw + 12;
    int cw3 = TH_CONTENT_W - 2 * (cw + 12);
    th_panel(x3, y, cw3, h);
    float ori[3] = { st->roll, st->pitch, st->yaw };
    motion_bars(x3, y, cw3, "ORIENTATION  (-1..1)", rpy, ori, 1.0f, TH_TEAL);

    /* horizon artificiel */
    float hx = (float)x3 + cw3 / 2.0f, hy = (float)y + h - 118.0f;
    vita2d_draw_fill_circle(hx, hy, 74, TH_INSET);
    draw_circle_outline(hx, hy, 74, TH_GLASS_EDGE);
    float ang = st->roll * (float)M_PI;
    float po = st->pitch * 40.0f;
    float ca = cosf(ang), sa = sinf(ang);
    vita2d_draw_line(hx - 62.0f * ca, hy + po - 62.0f * sa,
                     hx + 62.0f * ca, hy + po + 62.0f * sa, TH_TEAL);
    th_glow_dot((int)hx, (int)(hy + po), 3, TH_TEAL);
    th_text_c(s_font, (int)hx, y + h - 22, TH_FAINT, TH_FS_MICRO,
              "roll / pitch");
}

/* ------------------------------------------------------------------ */
/* Page Réglages                                                       */
/* ------------------------------------------------------------------ */
static void page_settings(const Config *cfg, const char *local_ip,
                          const UiStatus *stt, SettingsEdit *sedit) {
    int x = TH_CONTENT_X, y = TH_CONTENT_Y;
    int h = TH_FOOTER_Y - 8 - y;
    char line[128];

    /* --- carte cible OSC --- */
    int lw = 470;
    th_panel(x, y, lw, h);
    th_text(s_font, x + 20, y + 26, TH_FAINT, TH_FS_MICRO, "CIBLE OSC");

    snprintf(line, sizeof(line), "%s : %u", cfg->target_ip, cfg->target_port);
    th_text(s_font, x + 24, y + 62, TH_TEXT, TH_FS_TITLE, line);

    if (sedit) settings_edit_draw(sedit, x + 24, y + 84, lw - 48, s_font);

    int py = y + 84 + 42 + 16 + 34 + 40;
    vita2d_draw_rectangle(x + 20, py, lw - 40, 1, TH_ROW_LINE);
    py += 24;
    th_text(s_font, x + 20, py, TH_FAINT, TH_FS_MICRO, "PROFIL");
    py += 26;
    th_text(s_font, x + 24, py, TH_DIM, TH_FS_SMALL, "Prefixe global");
    th_text_r(s_font, x + lw - 24, py, TH_TEXT, TH_FS_SMALL, cfg->global_prefix);
    py += 26;
    th_text(s_font, x + 24, py, TH_DIM, TH_FS_SMALL, "Debit d'envoi");
    snprintf(line, sizeof(line), "%d Hz", cfg->send_rate_hz);
    th_text_r(s_font, x + lw - 24, py, TH_TEXT, TH_FS_SMALL, line);
    py += 26;
    th_text(s_font, x + 24, py, TH_DIM, TH_FS_SMALL, "Controles actifs");
    int en = 0;
    for (int i = 0; i < cfg->control_count; i++)
        if (cfg->controls[i].enabled) en++;
    snprintf(line, sizeof(line), "%d / %d", en, cfg->control_count);
    th_text_r(s_font, x + lw - 24, py, TH_TEXT, TH_FS_SMALL, line);

    /* --- carte réseau + webapp --- */
    int rx = x + lw + 12, rw = TH_CONTENT_W - lw - 12;
    th_panel(rx, y, rw, h);
    th_text(s_font, rx + 20, y + 26, TH_FAINT, TH_FS_MICRO, "RESEAU VITA");

    int ry = y + 64;
    th_text(s_font, rx + 24, ry, TH_DIM, TH_FS_SMALL, "IP locale");
    th_text(s_font, rx + 24, ry + 30, TH_TEXT, TH_FS_HERO, local_ip);
    ry += 58;

    const char *st_txt = stt->net_ok ? "PRET - envoi actif" : "cible non joignable";
    int stw = th_text_w(s_font, TH_FS_SMALL, st_txt) + 44;
    th_pill(rx + 24, ry, stw, 30, stt->net_ok ? TH_OK_SOFT : TH_WARN_SOFT);
    th_glow_dot(rx + 40, ry + 15, 3, stt->net_ok ? TH_OK : TH_WARN);
    th_text(s_font, rx + 54, ry + 20, stt->net_ok ? TH_OK : TH_WARN,
            TH_FS_SMALL, st_txt);
    ry += 46;
    snprintf(line, sizeof(line), "%d paquets / s", stt->pps);
    th_text(s_font, rx + 24, ry, TH_DIM, TH_FS_SMALL, line);
    ry += 30;

    vita2d_draw_rectangle(rx + 20, ry, rw - 40, 1, TH_ROW_LINE);
    ry += 24;
    th_text(s_font, rx + 20, ry, TH_FAINT, TH_FS_MICRO, "WEBAPP COMPANION");
    ry += 26;
    snprintf(line, sizeof(line), "http://%s:8080", local_ip);
    th_text(s_font, rx + 24, ry, TH_ACCENT, TH_FS_BODY, line);
    ry += 24;
    th_text(s_font, rx + 24, ry, TH_FAINT, TH_FS_MICRO,
            "GET / POST /config    POST /osc    GET /status");
    ry += 34;

    vita2d_draw_rectangle(rx + 20, ry, rw - 40, 1, TH_ROW_LINE);
    ry += 24;
    th_text(s_font, rx + 20, ry, TH_FAINT, TH_FS_MICRO, "A PROPOS");
    ry += 26;
    th_text(s_font, rx + 24, ry, TH_DIM, TH_FS_SMALL, "Vita2OSC");
    th_text_r(s_font, rx + rw - 24, ry, TH_TEXT, TH_FS_SMALL,
              "v" VITA2OSC_VERSION);
}

/* ------------------------------------------------------------------ */
/* Toast + overlay quitter                                             */
/* ------------------------------------------------------------------ */
static void draw_toast(void) {
    if (s_toast_frames <= 0) return;
    s_toast_frames--;

    float life = (float)s_toast_frames / (float)TOAST_LIFE;
    float a = 1.0f;
    if (life > 0.90f) a = (1.0f - life) / 0.10f;        /* entrée */
    else if (life < 0.15f) a = life / 0.15f;            /* sortie */
    unsigned int aa = (unsigned int)(a * 255.0f);

    unsigned int dot = s_toast_kind == 1 ? TH_OK
                     : s_toast_kind == 2 ? TH_WARN : TH_ACCENT;

    int tw = th_text_w(s_font, TH_FS_SMALL, s_toast_msg);
    int pw = tw + 52;
    int px = TH_CONTENT_X + (TH_CONTENT_W - pw) / 2;
    int py = TH_FOOTER_Y - 46;

    th_pill(px, py, pw, 34, with_alpha(RGBA8(0x10, 0x14, 0x1f, 0xFF),
                                       (aa * 0xE6) >> 8));
    th_rrect_outline(px, py, pw, 34, 17, with_alpha(dot, (aa * 0x50) >> 8));
    vita2d_draw_fill_circle(px + 20, py + 17, 4.0f, with_alpha(dot, aa));
    th_text(s_font, px + 34, py + 23, with_alpha(TH_TEXT, aa),
            TH_FS_SMALL, s_toast_msg);
}

static void draw_exit_overlay(float t) {
    if (t <= 0.0f) return;
    vita2d_draw_rectangle(0, 0, TH_SCR_W, TH_SCR_H,
                          with_alpha(TH_SCRIM, (unsigned int)(t * 0xB4)));
    int pw = 320, ph = 96;
    int px = (TH_SCR_W - pw) / 2, py = (TH_SCR_H - ph) / 2;
    th_panel(px, py, pw, ph);
    th_text_c(s_font, TH_SCR_W / 2, py + 36, TH_TEXT, TH_FS_BODY,
              "Quitter Vita2OSC");
    th_hbar(px + 30, py + 56, pw - 60, 10, t, TH_ERR);
}

/* ------------------------------------------------------------------ */
void ui_render(const Config *cfg, const InputState *st, UiTab tab,
               const char *local_ip, const UiStatus *status,
               SettingsEdit *sedit) {
    if (tab != s_last_tab) {
        s_fade = 1.0f;
        s_last_tab = tab;
    }

    vita2d_start_drawing();
    vita2d_clear_screen();

    th_background();
    draw_rail(tab, status);
    draw_header(cfg, tab, status);
    draw_footer(tab);

    switch (tab) {
        case TAB_BUTTONS:  page_buttons(cfg, st); break;
        case TAB_STICKS:   page_sticks(cfg, st); break;
        case TAB_TOUCH:    page_touch(cfg, st); break;
        case TAB_MOTION:   page_motion(cfg, st); break;
        case TAB_COLORS:
            colors_draw(&cfg->colors, s_font, TH_CONTENT_X, TH_CONTENT_Y,
                        TH_CONTENT_W, TH_FOOTER_Y - 8 - TH_CONTENT_Y);
            break;
        case TAB_SETTINGS:
            page_settings(cfg, local_ip, status, sedit);
            break;
        default: break;
    }

    /* voile de transition (fondu à travers le fond) */
    if (s_fade > 0.02f) {
        vita2d_draw_rectangle(TH_CONTENT_X - 4, 0,
                              TH_SCR_W - TH_CONTENT_X + 4, TH_SCR_H,
                              with_alpha(TH_BG, (unsigned int)(s_fade * 0xD0)));
        s_fade *= 0.78f;
    } else {
        s_fade = 0.0f;
    }

    draw_toast();
    draw_exit_overlay(status->exit_t);

    vita2d_end_drawing();
    vita2d_swap_buffers();
}

/* ------------------------------------------------------------------ */
/* Tactile                                                             */
/* ------------------------------------------------------------------ */
UiAction ui_touch_press(Config *cfg, SettingsEdit *sedit, UiTab *tab,
                        int px, int py) {
    /* rail de navigation */
    if (px < TH_RAIL_W) {
        int i = (py - RAIL_ITEMS_Y) / RAIL_ITEM_H;
        if (py >= RAIL_ITEMS_Y && i >= 0 && i < TAB_COUNT)
            *tab = (UiTab)i;
        return UI_ACT_NONE;
    }

    /* pill cible en en-tête -> réglages */
    if (px >= s_target_pill.x && px < s_target_pill.x + s_target_pill.w &&
        py >= s_target_pill.y && py < s_target_pill.y + s_target_pill.h) {
        *tab = TAB_SETTINGS;
        return UI_ACT_NONE;
    }

    if (*tab == TAB_COLORS) {
        colors_touch_press(&cfg->colors, px, py);
        return UI_ACT_NONE;
    }
    if (*tab == TAB_SETTINGS && sedit) {
        SedAction a = settings_edit_touch(sedit, px, py);
        switch (a) {
            case SED_APPLY:    return UI_ACT_APPLY_SETTINGS;
            case SED_IME_IP:   return UI_ACT_IME_IP;
            case SED_IME_PORT: return UI_ACT_IME_PORT;
            default:           return UI_ACT_NONE;
        }
    }
    return UI_ACT_NONE;
}

void ui_touch_drag(Config *cfg, UiTab tab, int px, int py) {
    if (tab == TAB_COLORS)
        colors_touch_drag(&cfg->colors, px, py);
}
