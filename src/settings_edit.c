#include "settings_edit.h"
#include "theme.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psp2/kernel/processmgr.h>

/* ---------------- répétition D-pad (appui long) ---------------- */
#define REPEAT_DELAY_US  350000   /* délai avant répétition   */
#define REPEAT_RATE_US    60000   /* période de répétition    */

typedef struct {
    int       held;
    SceUInt64 t_press;
    SceUInt64 t_last;
} KeyRepeat;

static KeyRepeat s_up, s_down, s_left, s_right;
static int s_prev_start, s_prev_triangle;

/* front montant OU répétition d'appui long */
static int key_fire(KeyRepeat *k, int now) {
    SceUInt64 t = sceKernelGetProcessTimeWide();
    if (now && !k->held) {                    /* front montant */
        k->held = 1; k->t_press = t; k->t_last = t;
        return 1;
    }
    if (!now) { k->held = 0; return 0; }
    if (t - k->t_press >= REPEAT_DELAY_US && t - k->t_last >= REPEAT_RATE_US) {
        k->t_last = t;
        return 1;
    }
    return 0;
}

static int edge(int now, int *prev) {
    int e = now && !*prev;
    *prev = now;
    return e;
}

/* ---------------- conversions ---------------- */
static int parse_ip(const char *ip, uint8_t o[4]) {
    unsigned a, b, c, d;
    char extra;
    if (!ip || sscanf(ip, "%u.%u.%u.%u%c", &a, &b, &c, &d, &extra) != 4) return 0;
    if (a > 255 || b > 255 || c > 255 || d > 255) return 0;
    o[0] = (uint8_t)a; o[1] = (uint8_t)b; o[2] = (uint8_t)c; o[3] = (uint8_t)d;
    return 1;
}

void settings_edit_ip_string(const SettingsEdit *e, char *out, int out_size) {
    snprintf(out, out_size, "%u.%u.%u.%u",
             e->octets[0], e->octets[1], e->octets[2], e->octets[3]);
}

void settings_edit_from_config(SettingsEdit *e, const Config *cfg) {
    memset(e, 0, sizeof(*e));
    if (!parse_ip(cfg->target_ip, e->octets)) {
        e->octets[0] = 192; e->octets[1] = 168; e->octets[2] = 1; e->octets[3] = 1;
    }
    e->port = cfg->target_port ? cfg->target_port : 9000;
    e->sel = SEL_OCTET0;
    memset(&s_up, 0, sizeof(s_up));   memset(&s_down, 0, sizeof(s_down));
    memset(&s_left, 0, sizeof(s_left)); memset(&s_right, 0, sizeof(s_right));
    s_prev_start = s_prev_triangle = 0;
}

void settings_edit_to_config(const SettingsEdit *e, Config *cfg) {
    settings_edit_ip_string(e, cfg->target_ip, sizeof(cfg->target_ip));
    cfg->target_port = e->port;
}

int settings_edit_set_ip(SettingsEdit *e, const char *ip) {
    uint8_t o[4];
    if (!parse_ip(ip, o)) return 0;
    memcpy(e->octets, o, 4);
    e->dirty = 1;
    return 1;
}

int settings_edit_set_port(SettingsEdit *e, const char *port_txt) {
    if (!port_txt || !port_txt[0]) return 0;
    long p = strtol(port_txt, NULL, 10);
    if (p < 1 || p > 65535) return 0;
    e->port = (uint16_t)p;
    e->dirty = 1;
    return 1;
}

/* ---------------- entrées ---------------- */
SedAction settings_edit_handle(SettingsEdit *e, const InputState *st) {
    int up    = input_source_value(st, SRC_BTN_UP)       > 0.5f;
    int down  = input_source_value(st, SRC_BTN_DOWN)     > 0.5f;
    int left  = input_source_value(st, SRC_BTN_LEFT)     > 0.5f;
    int right = input_source_value(st, SRC_BTN_RIGHT)    > 0.5f;
    int start = input_source_value(st, SRC_BTN_START)    > 0.5f;
    int tri   = input_source_value(st, SRC_BTN_TRIANGLE) > 0.5f;

    if (key_fire(&s_left, left))
        e->sel = (SettingsSel)((e->sel + SEL_COUNT - 1) % SEL_COUNT);
    if (key_fire(&s_right, right))
        e->sel = (SettingsSel)((e->sel + 1) % SEL_COUNT);

    int up_f = key_fire(&s_up, up);
    int dn_f = key_fire(&s_down, down);
    if (up_f || dn_f) {
        int delta = up_f ? 1 : -1;
        if (e->sel == SEL_PORT) {
            int p = (int)e->port + delta;
            if (p < 1) p = 1;
            if (p > 65535) p = 65535;
            e->port = (uint16_t)p;
        } else {
            int o = (int)e->sel;
            e->octets[o] = (uint8_t)(((int)e->octets[o] + 256 + delta) % 256);
        }
        e->dirty = 1;
    }

    if (edge(tri, &s_prev_triangle))
        return (e->sel == SEL_PORT) ? SED_IME_PORT : SED_IME_IP;

    if (edge(start, &s_prev_start) && e->dirty) {
        e->dirty = 0;
        return SED_APPLY;
    }
    return SED_NONE;
}

static int in_rect(SedRect r, int px, int py) {
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

SedAction settings_edit_touch(SettingsEdit *e, int px, int py) {
    for (int i = 0; i < 4; i++) {
        if (in_rect(e->z_octet[i], px, py)) {
            e->sel = (SettingsSel)i;
            return SED_IME_IP;
        }
    }
    if (in_rect(e->z_port, px, py)) {
        e->sel = SEL_PORT;
        return SED_IME_PORT;
    }
    if (in_rect(e->z_keyboard, px, py))
        return (e->sel == SEL_PORT) ? SED_IME_PORT : SED_IME_IP;
    if (in_rect(e->z_apply, px, py) && e->dirty) {
        e->dirty = 0;
        return SED_APPLY;
    }
    return SED_NONE;
}

/* ---------------- dessin ---------------- */
void settings_edit_draw(SettingsEdit *e, int x, int y, int w, vita2d_pgf *font) {
    char buf[32];
    const int bw = 64, bh = 42, gap = 8;

    /* --- champs octets --- */
    int cx = x;
    for (int i = 0; i < 4; i++) {
        int sel = (e->sel == (SettingsSel)i);
        th_rrect(cx, y, bw, bh, 9, TH_INSET);
        if (sel) th_rrect_outline(cx - 1, y - 1, bw + 2, bh + 2, 10, TH_ACCENT);
        snprintf(buf, sizeof(buf), "%u", e->octets[i]);
        th_text_c(font, cx + bw / 2, y + 28, sel ? TH_TEXT : TH_DIM, TH_FS_TITLE, buf);
        e->z_octet[i].x = cx; e->z_octet[i].y = y;
        e->z_octet[i].w = bw; e->z_octet[i].h = bh;
        cx += bw + gap;
        if (i < 3) {
            th_text(font, cx - gap + 1, y + 30, TH_FAINT, TH_FS_TITLE, ".");
            cx += 8;
        }
    }

    /* --- champ port --- */
    cx += 10;
    th_text(font, cx, y + 28, TH_FAINT, TH_FS_SMALL, ":");
    cx += 14;
    int pw = 92;
    int psel = (e->sel == SEL_PORT);
    th_rrect(cx, y, pw, bh, 9, TH_INSET);
    if (psel) th_rrect_outline(cx - 1, y - 1, pw + 2, bh + 2, 10, TH_ACCENT);
    snprintf(buf, sizeof(buf), "%u", (unsigned)e->port);
    th_text_c(font, cx + pw / 2, y + 28, psel ? TH_TEXT : TH_DIM, TH_FS_TITLE, buf);
    e->z_port.x = cx; e->z_port.y = y; e->z_port.w = pw; e->z_port.h = bh;

    /* --- boutons --- */
    int by = y + bh + 16;
    int kb_w = th_text_w(font, TH_FS_SMALL, "Clavier tactile") + 44;
    th_pill(x, by, kb_w, 34, TH_GLASS);
    th_rrect_outline(x, by, kb_w, 34, 17, TH_GLASS_EDGE);
    th_text(font, x + 16, by + 23, TH_TEXT, TH_FS_SMALL, "Clavier tactile");
    th_text_r(font, x + kb_w - 14, by + 23, TH_FAINT, TH_FS_MICRO, "^");
    e->z_keyboard.x = x; e->z_keyboard.y = by; e->z_keyboard.w = kb_w; e->z_keyboard.h = 34;

    int ap_x = x + kb_w + 12;
    int ap_w = th_text_w(font, TH_FS_SMALL, "Appliquer") + 44;
    th_pill(ap_x, by, ap_w, 34, e->dirty ? TH_ACCENT : TH_GLASS);
    th_text(font, ap_x + 16, by + 23, e->dirty ? TH_ON_ACCENT : TH_FAINT,
            TH_FS_SMALL, "Appliquer");
    th_text_r(font, ap_x + ap_w - 14, by + 23,
              e->dirty ? TH_ON_ACCENT : TH_FAINT, TH_FS_MICRO, ">");
    e->z_apply.x = ap_x; e->z_apply.y = by; e->z_apply.w = ap_w; e->z_apply.h = 34;

    /* --- état / aide --- */
    int hy = by + 56;
    if (e->dirty) {
        th_glow_dot(x + 6, hy - 5, 3, TH_WARN);
        th_text(font, x + 18, hy, TH_WARN, TH_FS_MICRO,
                "Modifications non appliquees - START ou [Appliquer]");
    } else {
        th_text(font, x, hy, TH_FAINT, TH_FS_MICRO,
                "FLECHES choisir/regler   TRIANGLE ou toucher un champ = clavier");
    }
    (void)w;
}
