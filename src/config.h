/*
 * config.h — Modèle de configuration OSC et preset par défaut.
 *
 * Pour l'instant la config est construite en dur (preset par défaut). Le
 * chargement/écriture JSON via le serveur HTTP arrive en phase 5 : la structure
 * ci-dessous est déjà alignée sur le schéma JSON de PLAN_DE_TRAVAIL.md.
 */
#ifndef VITA2OSC_CONFIG_H
#define VITA2OSC_CONFIG_H

#include "input.h"
#include "colors.h"

typedef enum {
    OSC_TYPE_FLOAT = 0, /* 'f' */
    OSC_TYPE_INT        /* 'i' */
} OscType;

typedef enum {
    MODE_ON_CHANGE = 0, /* émet quand la valeur change */
    MODE_CONTINUOUS,    /* émet à chaque tick (fréquence globale) */
    MODE_TRIGGER        /* émet une impulsion (out_max) au front montant */
} SendMode;

#define CFG_ADDR_LEN 64
#define CFG_MAX_CONTROLS 48

typedef struct {
    SrcId    src;
    int      enabled;
    char     address[CFG_ADDR_LEN];
    OscType  type;
    float    out_min, out_max;
    float    deadzone;   /* fraction de la plage (sources centrées) */
    float    smoothing;  /* 0 = aucun, ->1 = très lissé */
    int      invert;
    SendMode mode;
    int      bundle_group; /* 0 = aucun ; >0 = émis groupé avec ce même id */

    /* --- état runtime (rempli par le moteur) --- */
    float last_norm;   /* dernière valeur normalisée émise */
    float smoothed;    /* accumulateur de lissage */
    int   has_last;    /* 1 après le premier envoi */
    int   prev_high;   /* pour le mode trigger */
} Control;

typedef struct {
    char   target_ip[16];
    uint16_t target_port;
    char   global_prefix[32];
    int    send_rate_hz;

    Control controls[CFG_MAX_CONTROLS];
    int     control_count;

    ColorConfig colors;
} Config;

/* Remplit cfg avec le preset par défaut (équivalent preset_touchdesigner). */
void config_load_default(Config *cfg);

const char *config_type_tag(OscType t);   /* "f" / "i" */
const char *config_mode_label(SendMode m);

#endif /* VITA2OSC_CONFIG_H */
