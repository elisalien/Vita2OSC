/*
 * ime_input.h — Saisie texte via le clavier tactile officiel (SceImeDialog).
 *
 * ime_input_text() est bloquant : il affiche le clavier système par-dessus
 * l'app et rend ses propres frames (fond assombri) jusqu'à validation ou
 * annulation. À appeler depuis le thread principal, hors verrou httpd.
 */
#ifndef VITA2OSC_IME_INPUT_H
#define VITA2OSC_IME_INPUT_H

typedef enum {
    IME_MODE_TEXT = 0,   /* latin basique                       */
    IME_MODE_NUMBER,     /* chiffres uniquement (port)          */
    IME_MODE_IP          /* chiffres + symboles ('.' pour l'IP) */
} ImeMode;

/* À appeler une fois après vita2d_init() (configure le common dialog). */
int ime_init(void);

/* Ouvre le clavier tactile.
 *  title   : titre affiché au-dessus du champ (UTF-8, ASCII conseillé)
 *  initial : texte pré-rempli (UTF-8)
 *  out     : reçoit le texte validé (UTF-8, tronqué à out_size-1)
 * Retourne 1 si validé, 0 si annulé, <0 en cas d'erreur d'init. */
int ime_input_text(const char *title, const char *initial, ImeMode mode,
                   char *out, int out_size);

#endif /* VITA2OSC_IME_INPUT_H */
