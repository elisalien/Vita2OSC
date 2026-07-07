#include "colors.h"
#include "osc.h"
#include "net.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <vita2d.h>

#define CONTENT_Y 78

#define COL_BAR    RGBA8(0x22, 0x26, 0x30, 0xFF)
#define COL_ACCENT RGBA8(0x3d, 0x9b, 0xff, 0xFF)
#define COL_TEXT   RGBA8(0xEC, 0xEF, 0xF4, 0xFF)
#define COL_DIM    RGBA8(0x8a, 0x92, 0xa6, 0xFF)
#define COL_OK     RGBA8(0x53, 0xd7, 0x69, 0xFF)

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

typedef struct { int x, y, w, h; } Rect;

static Rect s_eff_tab[COLOR_MAX_EFFECTS];
static Rect s_hue_bar, s_sat_bar, s_bright_bar, s_alpha_bar;
static Rect s_group_btn[COLOR_MAX_GROUPS];
static Rect s_apply_btn;
static int  s_group_vis = 0;

static void txt(vita2d_pgf *font, int x, int y, unsigned int col, const char *s) {
    vita2d_pgf_draw_text(font, x, y, col, 1.0f, s);
}

static void draw_bar_gradient(int x, int y, int w, int h, int mode, float sat, float bright) {
    int steps = w / 4;
    if (steps < 1) steps = 1;
    int sw = w / steps;
    for (int i = 0; i < steps; i++) {
        float t = (float)i / (float)(steps - 1 > 0 ? steps - 1 : 1);
        uint8_t r, g, b;
        if (mode == 0) hsb_to_rgb(t, 1.0f, 1.0f, &r, &g, &b);
        else if (mode == 1) hsb_to_rgb(0.0f, t, 1.0f, &r, &g, &b);
        else if (mode == 2) hsb_to_rgb(0.0f, sat, t, &r, &g, &b);
        else {
            hsb_to_rgb(0.0f, sat, bright, &r, &g, &b);
            r = (uint8_t)((float)r * t);
            g = (uint8_t)((float)g * t);
            b = (uint8_t)((float)b * t);
        }
        vita2d_draw_rectangle(x + i * sw, y, sw + 1, h, RGBA8(r, g, b, 0xFF));
    }
    vita2d_draw_rectangle(x, y, w, 1, COL_DIM);
    vita2d_draw_rectangle(x, y + h - 1, w, 1, COL_DIM);
    vita2d_draw_rectangle(x, y, 1, h, COL_DIM);
    vita2d_draw_rectangle(x + w - 1, y, 1, h, COL_DIM);
}

static void draw_knob(int x, int y, int w, int h, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    int kx = x + (int)(t * (float)(w - 1));
    vita2d_draw_rectangle(kx - 2, y - 2, 4, h + 4, COL_TEXT);
}

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

void colors_draw(const ColorConfig *cc, vita2d_pgf *font) {
    const ColorEffect *fx = &cc->effects[cc->selected < cc->effect_count ? cc->selected : 0];
    int y0 = CONTENT_Y + 8;

    int ex = 20;
    for (int i = 0; i < cc->effect_count; i++) {
        int w = 200, h = 34;
        s_eff_tab[i].x = ex; s_eff_tab[i].y = y0; s_eff_tab[i].w = w; s_eff_tab[i].h = h;
        int sel = (i == cc->selected);
        vita2d_draw_rectangle(ex, y0, w, h, sel ? COL_ACCENT : COL_BAR);
        txt(font, ex + 12, y0 + 24, sel ? RGBA8(0,0,0,0xFF) : COL_TEXT, cc->effects[i].name);
        ex += w + 12;
    }

    int y = y0 + 48;
    uint8_t r, g, b;
    hsb_to_rgb(fx->hue.value, fx->sat.value, fx->bright.value, &r, &g, &b);
    unsigned int prev = RGBA8(r, g, b, (uint8_t)(fx->alpha.value * 255.0f));
    vita2d_draw_rectangle(24, y, 100, 100, COL_BAR);
    vita2d_draw_rectangle(28, y + 4, 92, 92, prev);
    txt(font, 24, y + 118, COL_DIM, "Apercu");

    char line[80];
    snprintf(line, sizeof(line), "H:%.2f S:%.2f B:%.2f A:%.2f",
             fx->hue.value, fx->sat.value, fx->bright.value, fx->alpha.value);
    txt(font, 24, y + 140, COL_TEXT, line);

    int bx = 150, bw = 780, bh = 28, gap = 38;
    const char *labels[] = {"Hue", "Sat", "Bright", "Alpha"};
    Rect *bars[] = {&s_hue_bar, &s_sat_bar, &s_bright_bar, &s_alpha_bar};
    float vals[] = {fx->hue.value, fx->sat.value, fx->bright.value, fx->alpha.value};

    for (int i = 0; i < 4; i++) {
        int by = y + i * gap;
        bars[i]->x = bx; bars[i]->y = by; bars[i]->w = bw; bars[i]->h = bh;
        txt(font, bx - 4, by + 20, COL_DIM, labels[i]);
        draw_bar_gradient(bx, by, bw, bh, i, fx->sat.value, fx->bright.value);
        draw_knob(bx, by, bw, bh, vals[i]);
    }

    y = y0 + 48 + gap * 4 + 20;
    txt(font, 24, y, COL_TEXT, "Groupes (touch = toggle)");
    y += 10;
    s_group_vis = fx->group_count;
    int gx = 24;
    for (int i = 0; i < fx->group_count; i++) {
        int gw = 72, gh = 40;
        s_group_btn[i].x = gx; s_group_btn[i].y = y; s_group_btn[i].w = gw; s_group_btn[i].h = gh;
        int on = fx->groups[i].enabled;
        vita2d_draw_rectangle(gx, y, gw, gh, on ? COL_OK : COL_BAR);
        snprintf(line, sizeof(line), "G%d", i + 1);
        txt(font, gx + 22, y + 26, on ? RGBA8(0,0,0,0xFF) : COL_DIM, line);
        gx += gw + 10;
    }

    s_apply_btn.x = 24; s_apply_btn.y = y + 52; s_apply_btn.w = 280; s_apply_btn.h = 36;
    vita2d_draw_rectangle(s_apply_btn.x, s_apply_btn.y, s_apply_btn.w, s_apply_btn.h, COL_ACCENT);
    txt(font, s_apply_btn.x + 16, s_apply_btn.y + 24, RGBA8(0,0,0,0xFF), "Appliquer a tous les groupes");
    txt(font, 320, s_apply_btn.y + 24, COL_DIM, "Toucher les barres pour H/S/B/A");
}

static void maybe_send(const ColorConfig *cc, const ColorEffect *fx, int toggles) {
    if (cc->auto_send) colors_send_effect(fx, toggles);
}

int colors_handle_touch(ColorConfig *cc, int px, int py, int active) {
    if (!active) return 0;
    ColorEffect *fx = &cc->effects[cc->selected < cc->effect_count ? cc->selected : 0];

    for (int i = 0; i < cc->effect_count; i++) {
        if (in_rect(s_eff_tab[i], px, py)) {
            cc->selected = i;
            return 1;
        }
    }

    if (in_rect(s_hue_bar, px, py)) {
        fx->hue.value = rect_norm_x(s_hue_bar, px);
        maybe_send(cc, fx, 0);
        return 1;
    }
    if (in_rect(s_sat_bar, px, py)) {
        fx->sat.value = rect_norm_x(s_sat_bar, px);
        maybe_send(cc, fx, 0);
        return 1;
    }
    if (in_rect(s_bright_bar, px, py)) {
        fx->bright.value = rect_norm_x(s_bright_bar, px);
        maybe_send(cc, fx, 0);
        return 1;
    }
    if (in_rect(s_alpha_bar, px, py)) {
        fx->alpha.value = rect_norm_x(s_alpha_bar, px);
        maybe_send(cc, fx, 0);
        return 1;
    }

    for (int i = 0; i < s_group_vis; i++) {
        if (in_rect(s_group_btn[i], px, py)) {
            fx->groups[i].enabled = !fx->groups[i].enabled;
            maybe_send(cc, fx, 1);
            return 1;
        }
    }

    if (in_rect(s_apply_btn, px, py)) {
        colors_send_all(cc);
        return 1;
    }

    return 0;
}
