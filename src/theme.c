#include "theme.h"
#include "pixelart_data.h"

#include <math.h>

/* ------------------------------------------------------------------ */
/* Rounded rect par balayage de lignes : aucune zone n'est dessinée    */
/* deux fois, indispensable pour les remplissages translucides.        */
/* ------------------------------------------------------------------ */
void th_rrect(int x, int y, int w, int h, int r, unsigned int col) {
    if (w <= 0 || h <= 0) return;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    if (r < 1) {
        vita2d_draw_rectangle(x, y, w, h, col);
        return;
    }

    /* corps central */
    vita2d_draw_rectangle(x, y + r, w, h - 2 * r, col);

    /* coins : une fine bande par ligne, largeur issue du cercle */
    for (int i = 0; i < r; i++) {
        float yy = (float)(r - i) - 0.5f;
        float dxf = (float)r - sqrtf((float)(r * r) - yy * yy);
        int dx = (int)(dxf + 0.5f);
        int bw = w - 2 * dx;
        if (bw <= 0) continue;
        vita2d_draw_rectangle(x + dx, y + i, bw, 1, col);             /* haut */
        vita2d_draw_rectangle(x + dx, y + h - 1 - i, bw, 1, col);     /* bas  */
    }
}

void th_rrect_outline(int x, int y, int w, int h, int r, unsigned int col) {
    if (w <= 2 || h <= 2) return;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;

    /* bords droits */
    vita2d_draw_rectangle(x + r, y, w - 2 * r, 1, col);
    vita2d_draw_rectangle(x + r, y + h - 1, w - 2 * r, 1, col);
    vita2d_draw_rectangle(x, y + r, 1, h - 2 * r, col);
    vita2d_draw_rectangle(x + w - 1, y + r, 1, h - 2 * r, col);

    /* coins : 1 pixel par ligne, sur le rayon */
    for (int i = 0; i < r; i++) {
        float yy = (float)(r - i) - 0.5f;
        float dxf = (float)r - sqrtf((float)(r * r) - yy * yy);
        int dx = (int)(dxf + 0.5f);
        vita2d_draw_rectangle(x + dx, y + i, 1, 1, col);
        vita2d_draw_rectangle(x + w - 1 - dx, y + i, 1, 1, col);
        vita2d_draw_rectangle(x + dx, y + h - 1 - i, 1, 1, col);
        vita2d_draw_rectangle(x + w - 1 - dx, y + h - 1 - i, 1, 1, col);
    }
}

void th_panel(int x, int y, int w, int h) {
    th_rrect(x, y, w, h, TH_RADIUS, TH_GLASS);
    /* liseré lumineux en haut (effet verre) */
    vita2d_draw_rectangle(x + TH_RADIUS, y, w - 2 * TH_RADIUS, 1, TH_GLASS_HI);
    /* bords latéraux discrets */
    vita2d_draw_rectangle(x, y + TH_RADIUS, 1, h - 2 * TH_RADIUS, TH_GLASS_EDGE);
    vita2d_draw_rectangle(x + w - 1, y + TH_RADIUS, 1, h - 2 * TH_RADIUS, TH_GLASS_EDGE);
    /* ombre portée légère sous la carte */
    vita2d_draw_rectangle(x + TH_RADIUS, y + h, w - 2 * TH_RADIUS, 1,
                          RGBA8(0x00, 0x00, 0x00, 0x50));
}

void th_pill(int x, int y, int w, int h, unsigned int col) {
    th_rrect(x, y, w, h, h / 2, col);
}

void th_hbar(int x, int y, int w, int h, float t, unsigned int fill) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    th_rrect(x, y, w, h, h / 2, TH_INSET);
    if (t <= 0.001f) return;
    int fw = (int)(t * (float)w + 0.5f);
    if (fw < h) fw = h; /* garde la forme de pilule sur les petites valeurs */
    th_rrect(x, y, fw, h, h / 2, fill);
}

void th_hbar_center(int x, int y, int w, int h, float v, unsigned int fill) {
    if (v < -1.0f) v = -1.0f;
    if (v > 1.0f) v = 1.0f;
    th_rrect(x, y, w, h, h / 2, TH_INSET);
    int cx = x + w / 2;
    /* tick central */
    vita2d_draw_rectangle(cx, y - 2, 1, h + 4, TH_ROW_LINE);
    int fw = (int)(fabsf(v) * (float)(w / 2) + 0.5f);
    if (fw < 2) return;
    if (v > 0) th_rrect(cx, y, fw, h, h / 2, fill);
    else       th_rrect(cx - fw, y, fw, h, h / 2, fill);
}

void th_switch(int x, int y, int w, int h, int on) {
    th_pill(x, y, w, h, on ? TH_ACCENT : TH_INSET);
    int r = h - 6;
    int kx = on ? (x + w - r - 3) : (x + 3);
    th_rrect(kx, y + 3, r, r, r / 2, on ? TH_ON_ACCENT : TH_DIM);
}

void th_glow_dot(int cx, int cy, int r, unsigned int core) {
    unsigned int cr = core & 0xFF, cg = (core >> 8) & 0xFF, cb = (core >> 16) & 0xFF;
    vita2d_draw_fill_circle(cx, cy, (float)(r * 3), RGBA8(cr, cg, cb, 0x16));
    vita2d_draw_fill_circle(cx, cy, (float)(r * 2), RGBA8(cr, cg, cb, 0x2C));
    vita2d_draw_fill_circle(cx, cy, (float)r, core);
}

/* ------------------------------------------------------------------ */
/* Logo pixel art embarqué : dessin par plages horizontales (RLE).     */
/* ------------------------------------------------------------------ */
void th_pixel_logo(int x, int y, int scale) {
    if (scale < 1) scale = 1;
    for (int py = 0; py < PX_LOGO_H; py++) {
        int px = 0;
        while (px < PX_LOGO_W) {
            unsigned char c = PX_LOGO[py][px];
            if (c == 0) { px++; continue; }
            int run = 1;
            while (px + run < PX_LOGO_W && PX_LOGO[py][px + run] == c) run++;
            vita2d_draw_rectangle(x + px * scale, y + py * scale,
                                  run * scale, scale, PX_PALETTE[c]);
            px += run;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Fond : halos "aurora" très doux, matière derrière les cartes verre. */
/* ------------------------------------------------------------------ */
static void halo(int cx, int cy, int r, unsigned int rgb, int steps, int base_a) {
    unsigned int cr = rgb & 0xFF, cg = (rgb >> 8) & 0xFF, cb = (rgb >> 16) & 0xFF;
    for (int s = steps; s >= 1; s--) {
        float rad = (float)r * (float)s / (float)steps;
        vita2d_draw_fill_circle(cx, cy, rad, RGBA8(cr, cg, cb, base_a));
    }
}

void th_background(void) {
    halo(190, -30, 300, TH_ACCENT, 5, 0x07);          /* bleu, haut-gauche   */
    halo(900, 560, 330, TH_VIOLET, 5, 0x06);          /* violet, bas-droite  */
    halo(650, 180, 180, TH_TEAL, 4, 0x04);            /* teal                */
}

/* ------------------------------------------------------------------ */
/* Texte                                                               */
/* ------------------------------------------------------------------ */
void th_text(vita2d_pgf *f, int x, int y, unsigned int col, float scale, const char *s) {
    if (!f || !s) return;
    vita2d_pgf_draw_text(f, x, y, col, scale, s);
}

int th_text_w(vita2d_pgf *f, float scale, const char *s) {
    if (!f || !s) return 0;
    return vita2d_pgf_text_width(f, scale, s);
}

void th_text_c(vita2d_pgf *f, int cx, int y, unsigned int col, float scale, const char *s) {
    if (!f || !s) return;
    vita2d_pgf_draw_text(f, cx - th_text_w(f, scale, s) / 2, y, col, scale, s);
}

void th_text_r(vita2d_pgf *f, int xr, int y, unsigned int col, float scale, const char *s) {
    if (!f || !s) return;
    vita2d_pgf_draw_text(f, xr - th_text_w(f, scale, s), y, col, scale, s);
}
