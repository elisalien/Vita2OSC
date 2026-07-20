#include "input.h"

#include <string.h>
#include <math.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/motion.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Index des boutons dans InputState.button[] (alignés sur SRC_BTN_*). */
enum {
    BI_CROSS, BI_CIRCLE, BI_SQUARE, BI_TRIANGLE,
    BI_UP, BI_DOWN, BI_LEFT, BI_RIGHT,
    BI_L, BI_R, BI_START, BI_SELECT
};

/* Bornes des panneaux tactiles, récupérées au démarrage. */
static float s_front_max_x = 1919.0f, s_front_max_y = 1087.0f;
static float s_front_max_force = 128.0f;
static float s_back_max_x = 1919.0f, s_back_max_y = 1087.0f;

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static int s_motion_ok = 0;

int input_init(void) {
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE); /* active les 2 sticks */

    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK,  SCE_TOUCH_SAMPLING_STATE_START);

    SceTouchPanelInfo pinfo;
    if (sceTouchGetPanelInfo(SCE_TOUCH_PORT_FRONT, &pinfo) >= 0) {
        s_front_max_x = (float)pinfo.maxAaX;
        s_front_max_y = (float)pinfo.maxAaY;
        if (pinfo.maxForce > 0) s_front_max_force = (float)pinfo.maxForce;
    }
    if (sceTouchGetPanelInfo(SCE_TOUCH_PORT_BACK, &pinfo) >= 0) {
        s_back_max_x = (float)pinfo.maxAaX;
        s_back_max_y = (float)pinfo.maxAaY;
    }

    s_motion_ok = (sceMotionStartSampling() >= 0);
    return 0;
}

void input_finish(void) {
    if (s_motion_ok) sceMotionStopSampling();
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_STOP);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK,  SCE_TOUCH_SAMPLING_STATE_STOP);
}

static float axis_to_signed(uint8_t v) {
    /* 0..255, centre 128 -> -1..1 */
    return clampf(((float)v - 128.0f) / 127.0f, -1.0f, 1.0f);
}

static void read_touch(SceTouchPortType port, TouchPoint *pts, int max_pts,
                       float max_x, float max_y, float max_force, int has_force) {
    SceTouchData td;
    memset(&td, 0, sizeof(td));
    sceTouchPeek(port, &td, 1);

    for (int i = 0; i < max_pts; i++) {
        if (i < (int)td.reportNum) {
            pts[i].active = 1;
            pts[i].x = clampf((float)td.report[i].x / max_x, 0.0f, 1.0f);
            pts[i].y = clampf((float)td.report[i].y / max_y, 0.0f, 1.0f);
            pts[i].force = has_force
                ? clampf((float)td.report[i].force / max_force, 0.0f, 1.0f)
                : 0.0f;
        } else {
            pts[i].active = 0;
            /* on garde la dernière position, mais force=0 et active=0 */
            pts[i].force = 0.0f;
        }
    }
}

/* Conversion quaternion -> angles d'Euler, normalisés par PI dans [-1,1]. */
static void quat_to_euler_norm(float x, float y, float z, float w,
                               float *roll, float *pitch, float *yaw) {
    /* roll (x-axis) */
    float sinr = 2.0f * (w * x + y * z);
    float cosr = 1.0f - 2.0f * (x * x + y * y);
    float r = atan2f(sinr, cosr);
    /* pitch (y-axis) */
    float sinp = 2.0f * (w * y - z * x);
    float p = fabsf(sinp) >= 1.0f ? copysignf((float)M_PI / 2.0f, sinp) : asinf(sinp);
    /* yaw (z-axis) */
    float siny = 2.0f * (w * z + x * y);
    float cosy = 1.0f - 2.0f * (y * y + z * z);
    float yw = atan2f(siny, cosy);

    *roll  = clampf(r  / (float)M_PI, -1.0f, 1.0f);
    *pitch = clampf(p  / ((float)M_PI / 2.0f), -1.0f, 1.0f);
    *yaw   = clampf(yw / (float)M_PI, -1.0f, 1.0f);
}

void input_poll(InputState *st) {
    memset(st->button, 0, sizeof(st->button));

    /* --- Boutons + sticks --- */
    SceCtrlData pad;
    memset(&pad, 0, sizeof(pad));
    sceCtrlPeekBufferPositive(0, &pad, 1);

    unsigned int b = pad.buttons;
    st->button[BI_CROSS]    = (b & SCE_CTRL_CROSS)    ? 1.0f : 0.0f;
    st->button[BI_CIRCLE]   = (b & SCE_CTRL_CIRCLE)   ? 1.0f : 0.0f;
    st->button[BI_SQUARE]   = (b & SCE_CTRL_SQUARE)   ? 1.0f : 0.0f;
    st->button[BI_TRIANGLE] = (b & SCE_CTRL_TRIANGLE) ? 1.0f : 0.0f;
    st->button[BI_UP]       = (b & SCE_CTRL_UP)       ? 1.0f : 0.0f;
    st->button[BI_DOWN]     = (b & SCE_CTRL_DOWN)     ? 1.0f : 0.0f;
    st->button[BI_LEFT]     = (b & SCE_CTRL_LEFT)     ? 1.0f : 0.0f;
    st->button[BI_RIGHT]    = (b & SCE_CTRL_RIGHT)    ? 1.0f : 0.0f;
    st->button[BI_L]        = (b & SCE_CTRL_LTRIGGER) ? 1.0f : 0.0f;
    st->button[BI_R]        = (b & SCE_CTRL_RTRIGGER) ? 1.0f : 0.0f;
    st->button[BI_START]    = (b & SCE_CTRL_START)    ? 1.0f : 0.0f;
    st->button[BI_SELECT]   = (b & SCE_CTRL_SELECT)   ? 1.0f : 0.0f;

    st->stick_lx = axis_to_signed(pad.lx);
    st->stick_ly = axis_to_signed(pad.ly);
    st->stick_rx = axis_to_signed(pad.rx);
    st->stick_ry = axis_to_signed(pad.ry);

    /* --- Tactile --- */
    read_touch(SCE_TOUCH_PORT_FRONT, st->front, INPUT_TOUCH_FRONT_POINTS,
               s_front_max_x, s_front_max_y, s_front_max_force, 1);
    read_touch(SCE_TOUCH_PORT_BACK, st->back, INPUT_TOUCH_BACK_POINTS,
               s_back_max_x, s_back_max_y, 0.0f, 0);

    /* --- Mouvement --- */
    SceMotionSensorState ms;
    memset(&ms, 0, sizeof(ms));
    if (s_motion_ok && sceMotionGetSensorState(&ms, 1) >= 0) {
        st->accel_x = ms.accelerometer.x;
        st->accel_y = ms.accelerometer.y;
        st->accel_z = ms.accelerometer.z;
        st->gyro_x  = ms.gyro.x;
        st->gyro_y  = ms.gyro.y;
        st->gyro_z  = ms.gyro.z;
    }

    SceMotionState mst;
    memset(&mst, 0, sizeof(mst));
    if (s_motion_ok && sceMotionGetState(&mst) >= 0) {
        quat_to_euler_norm(mst.deviceQuat.x, mst.deviceQuat.y,
                           mst.deviceQuat.z, mst.deviceQuat.w,
                           &st->roll, &st->pitch, &st->yaw);
    }
}

float input_source_value(const InputState *st, SrcId src) {
    switch (src) {
        case SRC_BTN_CROSS:    return st->button[BI_CROSS];
        case SRC_BTN_CIRCLE:   return st->button[BI_CIRCLE];
        case SRC_BTN_SQUARE:   return st->button[BI_SQUARE];
        case SRC_BTN_TRIANGLE: return st->button[BI_TRIANGLE];
        case SRC_BTN_UP:       return st->button[BI_UP];
        case SRC_BTN_DOWN:     return st->button[BI_DOWN];
        case SRC_BTN_LEFT:     return st->button[BI_LEFT];
        case SRC_BTN_RIGHT:    return st->button[BI_RIGHT];
        case SRC_BTN_L:        return st->button[BI_L];
        case SRC_BTN_R:        return st->button[BI_R];
        case SRC_BTN_START:    return st->button[BI_START];
        case SRC_BTN_SELECT:   return st->button[BI_SELECT];

        case SRC_STICK_LX: return st->stick_lx;
        case SRC_STICK_LY: return st->stick_ly;
        case SRC_STICK_RX: return st->stick_rx;
        case SRC_STICK_RY: return st->stick_ry;

        case SRC_TF0_X: return st->front[0].x;
        case SRC_TF0_Y: return st->front[0].y;
        case SRC_TF0_Z: return st->front[0].force;
        case SRC_TF1_X: return st->front[1].x;
        case SRC_TF1_Y: return st->front[1].y;
        case SRC_TF1_Z: return st->front[1].force;
        case SRC_TB0_X: return st->back[0].x;
        case SRC_TB0_Y: return st->back[0].y;

        case SRC_ACC_X: return st->accel_x;
        case SRC_ACC_Y: return st->accel_y;
        case SRC_ACC_Z: return st->accel_z;
        case SRC_GYR_X: return st->gyro_x;
        case SRC_GYR_Y: return st->gyro_y;
        case SRC_GYR_Z: return st->gyro_z;
        case SRC_ORI_ROLL:  return st->roll;
        case SRC_ORI_PITCH: return st->pitch;
        case SRC_ORI_YAW:   return st->yaw;
        default: return 0.0f;
    }
}

void input_source_range(SrcId src, float *out_min, float *out_max) {
    float lo = 0.0f, hi = 1.0f;
    if (src >= SRC_BTN_CROSS && src <= SRC_BTN_SELECT) { lo = 0.0f; hi = 1.0f; }
    else if (src >= SRC_STICK_LX && src <= SRC_STICK_RY) { lo = -1.0f; hi = 1.0f; }
    else if (src >= SRC_TF0_X && src <= SRC_TB0_Y) { lo = 0.0f; hi = 1.0f; }
    else if (src >= SRC_ACC_X && src <= SRC_ACC_Z) { lo = -2.0f; hi = 2.0f; }
    else if (src >= SRC_GYR_X && src <= SRC_GYR_Z) { lo = -5.0f; hi = 5.0f; }
    else if (src >= SRC_ORI_ROLL && src <= SRC_ORI_YAW) { lo = -1.0f; hi = 1.0f; }
    if (out_min) *out_min = lo;
    if (out_max) *out_max = hi;
}

int input_source_touch_active(const InputState *st, SrcId src) {
    switch (src) {
        case SRC_TF0_X: case SRC_TF0_Y: case SRC_TF0_Z: return st->front[0].active;
        case SRC_TF1_X: case SRC_TF1_Y: case SRC_TF1_Z: return st->front[1].active;
        case SRC_TB0_X: case SRC_TB0_Y:                 return st->back[0].active;
        default: return 1; /* sources non tactiles : toujours "actives" */
    }
}

const char *input_source_name(SrcId src) {
    switch (src) {
        case SRC_BTN_CROSS: return "button.cross";     case SRC_BTN_CIRCLE: return "button.circle";
        case SRC_BTN_SQUARE: return "button.square";   case SRC_BTN_TRIANGLE: return "button.triangle";
        case SRC_BTN_UP: return "button.up";           case SRC_BTN_DOWN: return "button.down";
        case SRC_BTN_LEFT: return "button.left";       case SRC_BTN_RIGHT: return "button.right";
        case SRC_BTN_L: return "button.l";             case SRC_BTN_R: return "button.r";
        case SRC_BTN_START: return "button.start";     case SRC_BTN_SELECT: return "button.select";
        case SRC_STICK_LX: return "stick.left.x";      case SRC_STICK_LY: return "stick.left.y";
        case SRC_STICK_RX: return "stick.right.x";     case SRC_STICK_RY: return "stick.right.y";
        case SRC_TF0_X: return "touch.front.0.x";      case SRC_TF0_Y: return "touch.front.0.y";
        case SRC_TF0_Z: return "touch.front.0.z";      case SRC_TF1_X: return "touch.front.1.x";
        case SRC_TF1_Y: return "touch.front.1.y";      case SRC_TF1_Z: return "touch.front.1.z";
        case SRC_TB0_X: return "touch.back.0.x";       case SRC_TB0_Y: return "touch.back.0.y";
        case SRC_ACC_X: return "motion.accel.x";       case SRC_ACC_Y: return "motion.accel.y";
        case SRC_ACC_Z: return "motion.accel.z";       case SRC_GYR_X: return "motion.gyro.x";
        case SRC_GYR_Y: return "motion.gyro.y";        case SRC_GYR_Z: return "motion.gyro.z";
        case SRC_ORI_ROLL: return "motion.orientation.roll";
        case SRC_ORI_PITCH: return "motion.orientation.pitch";
        case SRC_ORI_YAW: return "motion.orientation.yaw";
        default: return "none";
    }
}

SrcId input_source_from_name(const char *name) {
    if (!name) return SRC_NONE;
    for (int s = SRC_NONE + 1; s < SRC_COUNT; s++) {
        if (strcmp(name, input_source_name((SrcId)s)) == 0) return (SrcId)s;
    }
    return SRC_NONE;
}

const char *input_source_label(SrcId src) {
    switch (src) {
        case SRC_BTN_CROSS: return "Croix";     case SRC_BTN_CIRCLE: return "Rond";
        case SRC_BTN_SQUARE: return "Carre";    case SRC_BTN_TRIANGLE: return "Triangle";
        case SRC_BTN_UP: return "Haut";         case SRC_BTN_DOWN: return "Bas";
        case SRC_BTN_LEFT: return "Gauche";     case SRC_BTN_RIGHT: return "Droite";
        case SRC_BTN_L: return "L";             case SRC_BTN_R: return "R";
        case SRC_BTN_START: return "Start";     case SRC_BTN_SELECT: return "Select";
        case SRC_STICK_LX: return "Stick G X";  case SRC_STICK_LY: return "Stick G Y";
        case SRC_STICK_RX: return "Stick D X";  case SRC_STICK_RY: return "Stick D Y";
        case SRC_TF0_X: return "Touch Av0 X";   case SRC_TF0_Y: return "Touch Av0 Y";
        case SRC_TF0_Z: return "Touch Av0 P";   case SRC_TF1_X: return "Touch Av1 X";
        case SRC_TF1_Y: return "Touch Av1 Y";   case SRC_TF1_Z: return "Touch Av1 P";
        case SRC_TB0_X: return "Touch Ar0 X";   case SRC_TB0_Y: return "Touch Ar0 Y";
        case SRC_ACC_X: return "Accel X";       case SRC_ACC_Y: return "Accel Y";
        case SRC_ACC_Z: return "Accel Z";       case SRC_GYR_X: return "Gyro X";
        case SRC_GYR_Y: return "Gyro Y";        case SRC_GYR_Z: return "Gyro Z";
        case SRC_ORI_ROLL: return "Roll";       case SRC_ORI_PITCH: return "Pitch";
        case SRC_ORI_YAW: return "Yaw";
        default: return "?";
    }
}
