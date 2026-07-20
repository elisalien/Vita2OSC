#include "colors.h"
#include "theme.h"
#include "osc.h"
#include "net.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <vita2d.h>

/* ------------------------------------------------------------------ */
/* Modèle                                                              */
/* ------------------------------------------------------------------ */
static void hsb_to_rgb(float h, float s, float b,
                       uint8_t *r, uint8_t *g, uint8_t *bl) {
    h = h - floorf(h);
    if (h < 0.0f) h += 1.0f;
    int i = (int)(h * 6.0f);
    float f = h * 6.0f - (float)i;
    float p = b * (1.0f - s);
    float q = b * (1.0f - f * s);
    float t = b * (1.0f - (1.0f - f) * s);
    float rf, gf, bf;
    switch (i % 6) {
        case 0: rf = b;  gf = t;  bf = p; break;
        case 1: rf = q;  gf = b;  bf = p; break;
        case 2: rf = p;  gf = b;  bf = t; break;
        case 3: rf = p;  gf = q;  bf = b; break;
        case 4: rf = t;  gf = p;  bf = b; break;
        default: rf = b; gf = p;  bf = q; break;
    }
    *r = (uint8_t)(rf * 255.0f);
    *g = (uint8_t)(gf * 255.0f);
    *bl = (uint8_t)(bf * 255.0f);
}

static void setup_effect(ColorEffect *fx, const char *id, const char *name, int groups) {
    char base[48];
    memset(fx, 0, sizeof(*fx));
    strncpy(fx->id, id, sizeof(fx->id) - 1);
    strncpy(fx->name, name, sizeof(fx->name) - 1);
    snprintf(base, sizeof(base), "/vita/color/%s", id);
    snprintf(fx->effect_address, sizeof(fx->effect_address), "%s/active", base);
    snprintf(fx->hue.address, sizeof(fx->hue.address), "%s/hue", base);
    snprintf(fx->sat.address, sizeof(fx->sat.address), "%s/sat", base);
    snprintf(fx->bright.address, sizeof(fx->bright.address), "%s/bright", base);
    snprintf(fx->alpha.address, sizeof(fx->alpha.address), "%s/alpha", base);
    fx->hue.value = 0.0f;
    fx->sat.value = 1.0f;
    fx->bright.value = 0.5f;
    fx->alpha.value = 1.0f;
    fx->group_count = groups;
    for (int i = 0; i < groups; i++) {
        char ga[COLOR_ADDR_LEN];
        snprintf(ga, sizeof(ga), "%s/group/%d", base, i + 1);
        strncpy(fx->groups[i].address, ga, COLOR_ADDR_LEN - 1);
        fx->groups[i].enabled = 1;
    }
}

void colors_load_default(ColorConfig *cc) {
    memset(cc, 0, sizeof(*cc));
    setup_effect(&cc->effects[0], "colorize", "Colorize", 3);
    setup_effect(&cc->effects[1], "dreamy_glow", "Dreamy Glow", 3);
    cc->effect_count = 2;
    cc->selected = 0;
    cc->auto_send = 1;
}

/* ------------------------------------------------------------------ */
/* Envoi OSC                                                           */
/* ------------------------------------------------------------------ */
static int send_float(const char *addr, float v) {
    if (!addr || !addr[0] || !net_target_ready()) return 0;
    OscWriter w;
    osc_begin(&w);
    osc_message(&w, addr, "f");
    osc_push_float(&w, v);
    if (w.overflow) return 0;
    return net_send(osc_data(&w), osc_size(&w)) >= 0 ? 1 : 0;
}

int colors_send_effect(const ColorEffect *fx, int include_toggles) {
    if (!fx) return 0;
    int n = 0;
    if (fx->effect_address[0]) n += send_float(fx->effect_address, 1.0f);
    n += send_float(fx->hue.address,    fx->hue.value);
    n += send_float(fx->sat.address,    fx->sat.value);
    n += send_float(fx->bright.address, fx->bright.value);
    n += send_float(fx->alpha.address,  fx->alpha.value);
    if (include_toggles) {
        for (int i = 0; i < fx->group_count; i++) {
            const ColorGroup *g = &fx->groups[i];
            if (g->address[0])
                n += send_float(g->address, g->enabled ? 1.0f : 0.0f);
        }
    }
    return n;
}

int colors_send_all(const ColorConfig *cc) {
    int n = 0;
    for (int i = 0; i < cc->effect_count; i++)
        n += colors_send_effect(&cc->effects[i], 1);
    return n;
}

/* ------------------------------------------------------------------ */
/* Zones tactiles (mises à jour à chaque draw)                         */
/* ------------------------------------------------------------------ */
typedef struct { int x, y, w, h; } Rect;

static Rect s_eff_tab[COLOR_MAX_EFFECTS];
static Rect s_bars[4];               /* hue, sat, bright, alpha */
static Rect s_group_btn[COLOR_MAX_GROUPS];
static Rect s_send_btn;
static Rect s_autosend_btn;
static int  s_group_vis = 0;

static int in_rect(Rect r, int px, int py) {
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

static float rect_norm_x(Rect r, int px) {
    if (r.w <= 1) return 0.0f;
    float t = (float)(px - r.x) / (float)r.w;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t;
}

/* ------------------------------------------------------------------ */
/* Dessin                                                              */
/* ------------------------------------------------------------------ */
static void draw_bar_gradient(int x, int y, int w, int h, int mode,
                              float sat, float bright) {
    int steps = w / 6;
    if (steps < 2) steps = 2;
    float sw = (float)w / (float)steps;
    for (int i = 0; i < steps; i++) {
        float t = (float)i / (float)(steps - 1);
        uint8_t r, g, b;
        if (mode == 0)      hsb_to_rgb(t, 1.0f, 1.0f, &r, &g, &b);
        else if (mode == 1) hsb_to_rgb(0.0f, t, 1.0f, &r, &g, &b);
        else if (mode == 2) hsb_to_rgb(0.0f, sat, t, &r, &g, &b);
        else {
            hsb_to_rgb(0.0f, sat, bright, &r, &g, &b);
            r = (uint8_t)((float)r * t);
            g = (uint8_t)((float)g * t);
            b = (uint8_t)((float)b * t);
        }
        int x0 = x + (int)(i * sw);
        int x1 = x + (int)((i + 1) * sw);
        vita2d_draw_rectangle(x0, y, x1 - x0 + 1, h, RGBA8(r, g, b, 0xFF));
    }
}

static void draw_slider(vita2d_pgf *font, Rect *zone, int x, int y, int w,
                        const char *label, int mode, float value,
                        float sat, float bright) {
    const int h = 22;
    th_text(font, x, y + 15, TH_DIM, TH_FS_SMALL, label);
    char vbuf[16];
    snprintf(vbuf, sizeof(vbuf), "%.2f", value);
    th_text_r(font, x + w, y + 15, TH_TEXT, TH_FS_SMALL, vbuf);

    int by = y + 22;
    /* track : inset arrondi + gradient inséré */
    th_rrect(x, by, w, h, h / 2, TH_INSET);
    draw_bar_gradient(x + 3, by + 3, w - 6, h - 6, mode, sat, bright);

    /* curseur rond */
    int kx = x + 3 + (int)(value * (float)(w - 6));
    vita2d_draw_fill_circle(kx, by + h / 2, 12.0f, RGBA8(0, 0, 0, 0x60));
    vita2d_draw_fill_circle(kx, by + h / 2, 10.0f, TH_TEXT);
    vita2d_draw_fill_circle(kx, by + h / 2, 4.0f, TH_ON_ACCENT);

    zone->x = x; zone->y = by - 8; zone->w = w; zone->h = h + 16;
}

void colors_draw(const ColorConfig *cc, vita2d_pgf *font,
                 int x, int y, int w, int h) {
    const ColorEffect *fx =
        &cc->effects[cc->selected < cc->effect_count ? cc->selected : 0];
    char line[96];

    /* ============ carte gauche : effet + aperçu + envoi ============ */
    int lw = 264;
    th_panel(x, y, lw, h);
    th_text(font, x + 20, y + 26, TH_FAINT, TH_FS_MICRO, "EFFET");

    int ey = y + 40;
    for (int i = 0; i < cc->effect_count; i++) {
        int sel = (i == cc->selected);
        int ew = lw - 40;
        th_pill(x + 20, ey, ew, 34, sel ? TH_ACCENT : TH_INSET);
        th_text(font, x + 36, ey + 23, sel ? TH_ON_ACCENT : TH_DIM,
                TH_FS_SMALL, cc->effects[i].name);
        if (sel) th_glow_dot(x + 20 + ew - 16, ey + 17, 3, TH_ON_ACCENT);
        s_eff_tab[i].x = x + 20; s_eff_tab[i].y = ey;
        s_eff_tab[i].w = ew;     s_eff_tab[i].h = 34;
        ey += 42;
    }

    /* aperçu */
    ey += 8;
    th_text(font, x + 20, ey + 12, TH_FAINT, TH_FS_MICRO, "APERCU");
    ey += 22;
    uint8_t r, g, b;
    hsb_to_rgb(fx->hue.value, fx->sat.value, fx->bright.value, &r, &g, &b);
    int sw_size = 96;
    int sx = x + (lw - sw_size) / 2;
    th_rrect(sx - 4, ey - 4, sw_size + 8, sw_size + 8, 14, TH_INSET);
    th_rrect(sx, ey, sw_size, sw_size, 12,
             RGBA8(r, g, b, (uint8_t)(fx->alpha.value * 255.0f)));
    ey += sw_size + 24;
    snprintf(line, sizeof(line), "H %.2f  S %.2f  B %.2f  A %.2f",
             fx->hue.value, fx->sat.value, fx->bright.value, fx->alpha.value);
    th_text_c(font, x + lw / 2, ey, TH_DIM, TH_FS_MICRO, line);

    /* auto-send + envoyer tout */
    int by = y + h - 96;
    th_text(font, x + 20, by + 14, TH_DIM, TH_FS_SMALL, "Envoi auto");
    th_switch(x + lw - 74, by, 54, 26, cc->auto_send);
    s_autosend_btn.x = x + lw - 74; s_autosend_btn.y = by - 4;
    s_autosend_btn.w = 54;          s_autosend_btn.h = 34;

    by += 40;
    th_pill(x + 20, by, lw - 40, 36, TH_ACCENT);
    th_text_c(font, x + lw / 2, by + 24, TH_ON_ACCENT, TH_FS_SMALL,
              "Tout envoyer");
    s_send_btn.x = x + 20; s_send_btn.y = by;
    s_send_btn.w = lw - 40; s_send_btn.h = 36;

    /* ============ carte droite : sliders + groupes ============ */
    int rx = x + lw + 12;
    int rw = w - lw - 12;
    th_panel(rx, y, rw, h);
    th_text(font, rx + 20, y + 26, TH_FAINT, TH_FS_MICRO,
            "PARAMETRES  (toucher / glisser)");

    int sy = y + 44;
    int sx2 = rx + 24, sw2 = rw - 48;
    const char *labels[4] = {"Teinte", "Saturation", "Luminosite", "Alpha"};
    const float vals[4] = {fx->hue.value, fx->sat.value,
                           fx->bright.value, fx->alpha.value};
    for (int i = 0; i < 4; i++) {
        draw_slider(font, &s_bars[i], sx2, sy, sw2, labels[i], i, vals[i],
                    fx->sat.value, fx->bright.value);
        sy += 62;
    }

    /* groupes */
    sy += 4;
    vita2d_draw_rectangle(rx + 20, sy - 10, rw - 40, 1, TH_ROW_LINE);
    th_text(font, rx + 24, sy + 8, TH_FAINT, TH_FS_MICRO,
            "GROUPES  (toucher pour activer)");
    sy += 20;
    s_group_vis = fx->group_count;
    int gx = rx + 24;
    for (int i = 0; i < fx->group_count; i++) {
        int gw = 76, gh = 38;
        int on = fx->groups[i].enabled;
        th_pill(gx, sy, gw, gh, on ? TH_OK_SOFT : TH_INSET);
        if (on) th_rrect_outline(gx, sy, gw, gh, gh / 2, TH_OK);
        snprintf(line, sizeof(line), "G%d", i + 1);
        th_glow_dot(gx + 18, sy + gh / 2, 3, on ? TH_OK : TH_FAINT);
        th_text(font, gx + 32, sy + 25, on ? TH_TEXT : TH_FAINT,
                TH_FS_SMALL, line);
        s_group_btn[i].x = gx; s_group_btn[i].y = sy;
        s_group_btn[i].w = gw; s_group_btn[i].h = gh;
        gx += gw + 10;
    }
}

/* ------------------------------------------------------------------ */
/* Tactile                                                             */
/* ------------------------------------------------------------------ */
static void maybe_send(const ColorConfig *cc, const ColorEffect *fx, int toggles) {
    if (cc->auto_send) colors_send_effect(fx, toggles);
}

int colors_touch_drag(ColorConfig *cc, int px, int py) {
    ColorEffect *fx =
        &cc->effects[cc->selected < cc->effect_count ? cc->selected : 0];
    ColorParam *params[4] = {&fx->hue, &fx->sat, &fx->bright, &fx->alpha};
    for (int i = 0; i < 4; i++) {
        if (in_rect(s_bars[i], px, py)) {
            params[i]->value = rect_norm_x(s_bars[i], px);
            maybe_send(cc, fx, 0);
            return 1;
        }
    }
    return 0;
}

int colors_touch_press(ColorConfig *cc, int px, int py) {
    ColorEffect *fx =
        &cc->effects[cc->selected < cc->effect_count ? cc->selected : 0];

    for (int i = 0; i < cc->effect_count; i++) {
        if (in_rect(s_eff_tab[i], px, py)) {
            cc->selected = i;
            return 1;
        }
    }
    for (int i = 0; i < s_group_vis; i++) {
        if (in_rect(s_group_btn[i], px, py)) {
            fx->groups[i].enabled = !fx->groups[i].enabled;
            maybe_send(cc, fx, 1);
            return 1;
        }
    }
    if (in_rect(s_autosend_btn, px, py)) {
        cc->auto_send = !cc->auto_send;
        return 1;
    }
    if (in_rect(s_send_btn, px, py)) {
        colors_send_all(cc);
        return 1;
    }
    /* un tap sur une barre règle aussi la valeur */
    return colors_touch_drag(cc, px, py);
}
