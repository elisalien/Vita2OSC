/*
 * input.h — Lecture et normalisation des entrées de la PS Vita.
 *
 * Une passe input_poll() remplit un InputState. Le moteur de mapping lit
 * ensuite chaque "source" via input_source_value(). Chaque source a une plage
 * canonique connue (input_source_range) que le mapping remappe vers la sortie.
 */
#ifndef VITA2OSC_INPUT_H
#define VITA2OSC_INPUT_H

#include <stdint.h>

/* Nombre de points tactiles exposés (sur les 6 possibles côté avant). */
#define INPUT_TOUCH_FRONT_POINTS 2
#define INPUT_TOUCH_BACK_POINTS  1

typedef struct {
    int   active;   /* doigt présent */
    float x, y;     /* 0..1 (normalisé sur la surface) */
    float force;    /* 0..1 (avant uniquement ; arrière = 0) */
} TouchPoint;

typedef struct {
    /* Boutons : 0.0 ou 1.0, indexés par SrcButton (voir enum). */
    float button[12];

    /* Sticks : -1..1. */
    float stick_lx, stick_ly, stick_rx, stick_ry;

    /* Tactile. */
    TouchPoint front[INPUT_TOUCH_FRONT_POINTS];
    TouchPoint back[INPUT_TOUCH_BACK_POINTS];

    /* Mouvement. */
    float accel_x, accel_y, accel_z;   /* G, ~ -2..2 */
    float gyro_x,  gyro_y,  gyro_z;    /* rad/s, ~ -5..5 */
    float roll, pitch, yaw;            /* -1..1 (euler / PI) */
} InputState;

/* Identifiants de sources (référencés par la config/mapping). */
typedef enum {
    SRC_NONE = 0,

    /* Boutons */
    SRC_BTN_CROSS, SRC_BTN_CIRCLE, SRC_BTN_SQUARE, SRC_BTN_TRIANGLE,
    SRC_BTN_UP, SRC_BTN_DOWN, SRC_BTN_LEFT, SRC_BTN_RIGHT,
    SRC_BTN_L, SRC_BTN_R, SRC_BTN_START, SRC_BTN_SELECT,

    /* Sticks */
    SRC_STICK_LX, SRC_STICK_LY, SRC_STICK_RX, SRC_STICK_RY,

    /* Tactile avant (point 0 et 1) : x, y, force */
    SRC_TF0_X, SRC_TF0_Y, SRC_TF0_Z,
    SRC_TF1_X, SRC_TF1_Y, SRC_TF1_Z,
    /* Tactile arrière (point 0) : x, y */
    SRC_TB0_X, SRC_TB0_Y,

    /* Mouvement */
    SRC_ACC_X, SRC_ACC_Y, SRC_ACC_Z,
    SRC_GYR_X, SRC_GYR_Y, SRC_GYR_Z,
    SRC_ORI_ROLL, SRC_ORI_PITCH, SRC_ORI_YAW,

    SRC_COUNT
} SrcId;

int  input_init(void);
void input_finish(void);
void input_poll(InputState *st);

/* Valeur canonique courante d'une source. */
float input_source_value(const InputState *st, SrcId src);
/* Plage canonique d'une source (pour normalisation par le mapping). */
void  input_source_range(SrcId src, float *out_min, float *out_max);
/* 1 si la source est un point tactile actuellement actif, sinon 0.
 * (permet de ne pas émettre des coordonnées "collées" quand le doigt est levé) */
int   input_source_touch_active(const InputState *st, SrcId src);
/* Libellé court pour l'UI. */
const char *input_source_label(SrcId src);
/* Nom canonique pour le JSON (ex. "button.cross", "stick.left.x"). */
const char *input_source_name(SrcId src);
/* Inverse : nom canonique -> SrcId (SRC_NONE si inconnu). */
SrcId       input_source_from_name(const char *name);

#endif /* VITA2OSC_INPUT_H */
