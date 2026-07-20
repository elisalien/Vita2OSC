/*
 * theme.h — Design system "glasscore" dark (vita2d).
 *
 * Palette, métriques de layout et primitives de dessin partagées par
 * ui.c, colors.c et settings_edit.c : panneaux verre dépoli, pills,
 * barres arrondies, halos d'ambiance, texte aligné, logo pixel art.
 */
#ifndef VITA2OSC_THEME_H
#define VITA2OSC_THEME_H

#include <vita2d.h>

/* ---------- Écran ---------- */
#define TH_SCR_W 960
#define TH_SCR_H 544

/* ---------- Layout global (rail latéral + contenu) ---------- */
#define TH_RAIL_W     76      /* rail de navigation gauche              */
#define TH_CONTENT_X  (TH_RAIL_W + 16)
#define TH_CONTENT_W  (TH_SCR_W - TH_CONTENT_X - 16)
#define TH_HEADER_H   58      /* zone titre de page + statut            */
#define TH_CONTENT_Y  (TH_HEADER_H + 14)
#define TH_FOOTER_Y   (TH_SCR_H - 24)
#define TH_RADIUS     14      /* rayon standard des cartes              */

/* ---------- Palette ---------- */
#define TH_BG          RGBA8(0x0b, 0x0e, 0x14, 0xFF)  /* fond profond          */
#define TH_GLASS       RGBA8(0x17, 0x1c, 0x28, 0xC6)  /* carte verre           */
#define TH_GLASS_HI    RGBA8(0xFF, 0xFF, 0xFF, 0x1C)  /* liseré haut           */
#define TH_GLASS_EDGE  RGBA8(0xFF, 0xFF, 0xFF, 0x0E)  /* bord latéral          */
#define TH_INSET       RGBA8(0x07, 0x0a, 0x10, 0xB4)  /* puits/track enfoncé   */
#define TH_ROW_LINE    RGBA8(0xFF, 0xFF, 0xFF, 0x0A)  /* séparateur de lignes  */
#define TH_SCRIM       RGBA8(0x05, 0x07, 0x0c, 0xB4)  /* voile modal           */

#define TH_ACCENT      RGBA8(0x4d, 0xa3, 0xff, 0xFF)  /* bleu signature        */
#define TH_ACCENT_SOFT RGBA8(0x4d, 0xa3, 0xff, 0x2E)
#define TH_VIOLET      RGBA8(0x8b, 0x5c, 0xf6, 0xFF)
#define TH_TEAL        RGBA8(0x2d, 0xd4, 0xbf, 0xFF)
#define TH_PINK        RGBA8(0xff, 0x9d, 0xb4, 0xFF)
#define TH_ON_ACCENT   RGBA8(0x05, 0x10, 0x20, 0xFF)  /* texte sur accent      */

#define TH_TEXT        RGBA8(0xf2, 0xf5, 0xfa, 0xFF)
#define TH_DIM         RGBA8(0x8d, 0x97, 0xac, 0xFF)
#define TH_FAINT       RGBA8(0x52, 0x5c, 0x72, 0xFF)

#define TH_OK          RGBA8(0x35, 0xd3, 0x9a, 0xFF)
#define TH_WARN        RGBA8(0xfb, 0xbf, 0x24, 0xFF)
#define TH_ERR         RGBA8(0xff, 0x6b, 0x81, 0xFF)
#define TH_OK_SOFT     RGBA8(0x35, 0xd3, 0x9a, 0x28)
#define TH_WARN_SOFT   RGBA8(0xfb, 0xbf, 0x24, 0x28)
#define TH_ERR_SOFT    RGBA8(0xff, 0x6b, 0x81, 0x28)

/* ---------- Échelles typo (police pgf unique) ---------- */
#define TH_FS_HERO  1.35f
#define TH_FS_TITLE 1.05f
#define TH_FS_BODY  0.92f
#define TH_FS_SMALL 0.78f
#define TH_FS_MICRO 0.70f

/* ---------- Primitives ---------- */

/* Fond d'ambiance : halos colorés doux derrière les cartes (à appeler
 * juste après vita2d_clear_screen). */
void th_background(void);

/* Rectangle à coins arrondis rempli (sans recouvrement alpha). */
void th_rrect(int x, int y, int w, int h, int r, unsigned int col);

/* Contour arrondi 1px (anneau de focus, bordures). */
void th_rrect_outline(int x, int y, int w, int h, int r, unsigned int col);

/* Carte "verre" standard : fond translucide + liseré haut + bords. */
void th_panel(int x, int y, int w, int h);

/* Pill (rrect au rayon max). */
void th_pill(int x, int y, int w, int h, unsigned int col);

/* Barre de niveau horizontale arrondie : track enfoncé + remplissage. */
void th_hbar(int x, int y, int w, int h, float t, unsigned int fill);

/* Barre bipolaire : remplissage depuis le centre, v dans [-1, 1]. */
void th_hbar_center(int x, int y, int w, int h, float v, unsigned int fill);

/* Interrupteur on/off (pill + bouton). */
void th_switch(int x, int y, int w, int h, int on);

/* Point lumineux avec halo (statut, curseurs tactiles). */
void th_glow_dot(int cx, int cy, int r, unsigned int core);

/* Logo pixel art embarqué (32x32 * scale). Dessine à (x, y). */
void th_pixel_logo(int x, int y, int scale);

/* Texte : gauche / centré / aligné droite (x = bord droit). */
void th_text (vita2d_pgf *f, int x, int y, unsigned int col, float scale, const char *s);
void th_text_c(vita2d_pgf *f, int cx, int y, unsigned int col, float scale, const char *s);
void th_text_r(vita2d_pgf *f, int xr, int y, unsigned int col, float scale, const char *s);

/* Largeur d'un texte à l'échelle donnée. */
int th_text_w(vita2d_pgf *f, float scale, const char *s);

#endif /* VITA2OSC_THEME_H */
