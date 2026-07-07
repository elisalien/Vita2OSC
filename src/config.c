#include "config.h"
#include "colors.h"
#include <string.h>

static void add(Config *cfg, SrcId src, const char *addr, OscType type,
                float omin, float omax, float dz, float sm, int invert,
                SendMode mode, int bundle) {
    if (cfg->control_count >= CFG_MAX_CONTROLS) return;
    Control *c = &cfg->controls[cfg->control_count++];
    memset(c, 0, sizeof(*c));
    c->src = src;
    c->enabled = 1;
    strncpy(c->address, addr, CFG_ADDR_LEN - 1);
    c->type = type;
    c->out_min = omin;
    c->out_max = omax;
    c->deadzone = dz;
    c->smoothing = sm;
    c->invert = invert;
    c->mode = mode;
    c->bundle_group = bundle;
}

void config_load_default(Config *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    strncpy(cfg->target_ip, "192.168.1.42", sizeof(cfg->target_ip) - 1);
    cfg->target_port = 9000;
    strncpy(cfg->global_prefix, "/vita", sizeof(cfg->global_prefix) - 1);
    cfg->send_rate_hz = 60;

    /* Boutons — float 0/1, onChange */
    add(cfg, SRC_BTN_CROSS,    "/vita/pad/cross",    OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_ON_CHANGE, 0);
    add(cfg, SRC_BTN_CIRCLE,   "/vita/pad/circle",   OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_ON_CHANGE, 0);
    add(cfg, SRC_BTN_SQUARE,   "/vita/pad/square",   OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_ON_CHANGE, 0);
    add(cfg, SRC_BTN_TRIANGLE, "/vita/pad/triangle", OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_ON_CHANGE, 0);
    add(cfg, SRC_BTN_UP,       "/vita/pad/up",       OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_ON_CHANGE, 0);
    add(cfg, SRC_BTN_DOWN,     "/vita/pad/down",     OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_ON_CHANGE, 0);
    add(cfg, SRC_BTN_LEFT,     "/vita/pad/left",     OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_ON_CHANGE, 0);
    add(cfg, SRC_BTN_RIGHT,    "/vita/pad/right",    OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_ON_CHANGE, 0);
    add(cfg, SRC_BTN_L,        "/vita/pad/l",        OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_ON_CHANGE, 0);
    add(cfg, SRC_BTN_R,        "/vita/pad/r",        OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_ON_CHANGE, 0);
    add(cfg, SRC_BTN_START,    "/vita/pad/start",    OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_ON_CHANGE, 0);
    add(cfg, SRC_BTN_SELECT,   "/vita/pad/select",   OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_ON_CHANGE, 0);

    /* Sticks — sortie -1..1, deadzone 8%, Y inversé */
    add(cfg, SRC_STICK_LX, "/vita/stick/lx", OSC_TYPE_FLOAT, -1,1, 0.08f,0,0, MODE_CONTINUOUS, 0);
    add(cfg, SRC_STICK_LY, "/vita/stick/ly", OSC_TYPE_FLOAT, -1,1, 0.08f,0,1, MODE_CONTINUOUS, 0);
    add(cfg, SRC_STICK_RX, "/vita/stick/rx", OSC_TYPE_FLOAT, -1,1, 0.08f,0,0, MODE_CONTINUOUS, 0);
    add(cfg, SRC_STICK_RY, "/vita/stick/ry", OSC_TYPE_FLOAT, -1,1, 0.08f,0,1, MODE_CONTINUOUS, 0);

    /* Tactile — 0..1, groupés par point (bundle) */
    add(cfg, SRC_TF0_X, "/vita/touch/front/0/x", OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_CONTINUOUS, 1);
    add(cfg, SRC_TF0_Y, "/vita/touch/front/0/y", OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_CONTINUOUS, 1);
    add(cfg, SRC_TF0_Z, "/vita/touch/front/0/z", OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_CONTINUOUS, 1);
    add(cfg, SRC_TF1_X, "/vita/touch/front/1/x", OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_CONTINUOUS, 2);
    add(cfg, SRC_TF1_Y, "/vita/touch/front/1/y", OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_CONTINUOUS, 2);
    add(cfg, SRC_TF1_Z, "/vita/touch/front/1/z", OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_CONTINUOUS, 2);
    add(cfg, SRC_TB0_X, "/vita/touch/back/0/x",  OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_CONTINUOUS, 3);
    add(cfg, SRC_TB0_Y, "/vita/touch/back/0/y",  OSC_TYPE_FLOAT, 0,1, 0,0,0, MODE_CONTINUOUS, 3);

    /* Mouvement — accel/gyro bruts, orientation -1..1, un peu de lissage */
    add(cfg, SRC_ACC_X, "/vita/accel/x", OSC_TYPE_FLOAT, -2,2, 0,0.1f,0, MODE_CONTINUOUS, 0);
    add(cfg, SRC_ACC_Y, "/vita/accel/y", OSC_TYPE_FLOAT, -2,2, 0,0.1f,0, MODE_CONTINUOUS, 0);
    add(cfg, SRC_ACC_Z, "/vita/accel/z", OSC_TYPE_FLOAT, -2,2, 0,0.1f,0, MODE_CONTINUOUS, 0);
    add(cfg, SRC_GYR_X, "/vita/gyro/x",  OSC_TYPE_FLOAT, -5,5, 0,0.1f,0, MODE_CONTINUOUS, 0);
    add(cfg, SRC_GYR_Y, "/vita/gyro/y",  OSC_TYPE_FLOAT, -5,5, 0,0.1f,0, MODE_CONTINUOUS, 0);
    add(cfg, SRC_GYR_Z, "/vita/gyro/z",  OSC_TYPE_FLOAT, -5,5, 0,0.1f,0, MODE_CONTINUOUS, 0);
    add(cfg, SRC_ORI_ROLL,  "/vita/orient/roll",  OSC_TYPE_FLOAT, -1,1, 0,0.15f,0, MODE_CONTINUOUS, 0);
    add(cfg, SRC_ORI_PITCH, "/vita/orient/pitch", OSC_TYPE_FLOAT, -1,1, 0,0.15f,0, MODE_CONTINUOUS, 0);
    add(cfg, SRC_ORI_YAW,   "/vita/orient/yaw",   OSC_TYPE_FLOAT, -1,1, 0,0.15f,0, MODE_CONTINUOUS, 0);

    colors_load_default(&cfg->colors);
}

const char *config_type_tag(OscType t) {
    return t == OSC_TYPE_INT ? "i" : "f";
}

const char *config_mode_label(SendMode m) {
    switch (m) {
        case MODE_ON_CHANGE:  return "onChange";
        case MODE_CONTINUOUS: return "continuous";
        case MODE_TRIGGER:    return "trigger";
        default: return "?";
    }
}
