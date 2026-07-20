#include "ime_input.h"
#include "theme.h"

#include <string.h>
#include <vita2d.h>
#include <psp2/apputil.h>
#include <psp2/common_dialog.h>
#include <psp2/ime_dialog.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/sysmodule.h>

#define IME_MAX_LEN 63

static uint16_t s_title[64];
static uint16_t s_initial[IME_MAX_LEN + 1];
static uint16_t s_input[IME_MAX_LEN + 4];
static int      s_inited = 0;

/* --- conversions ASCII <-> UTF-16 (suffisant pour IP/port/adresses) ----- */
static void utf8_to_utf16(const char *src, uint16_t *dst, int dst_len) {
    int i = 0;
    while (src && *src && i < dst_len - 1) {
        unsigned char c = (unsigned char)*src++;
        if (c < 0x80) dst[i++] = (uint16_t)c;
        /* les séquences multi-octets sont ignorées (non nécessaires ici) */
    }
    dst[i] = 0;
}

static void utf16_to_utf8(const uint16_t *src, char *dst, int dst_len) {
    int i = 0;
    while (src && *src && i < dst_len - 1) {
        uint16_t c = *src++;
        if (c < 0x80) dst[i++] = (char)c;
    }
    dst[i] = '\0';
}

int ime_init(void) {
    if (s_inited) return 0;

    sceSysmoduleLoadModule(SCE_SYSMODULE_APPUTIL);

    SceAppUtilInitParam initParam;
    SceAppUtilBootParam bootParam;
    memset(&initParam, 0, sizeof(initParam));
    memset(&bootParam, 0, sizeof(bootParam));
    sceAppUtilInit(&initParam, &bootParam);

    SceCommonDialogConfigParam cfg;
    sceCommonDialogConfigParamInit(&cfg);
    int rc = sceCommonDialogSetConfigParam(&cfg);
    s_inited = (rc >= 0);
    return rc;
}

/* Frame de fond pendant que le clavier système est ouvert. */
static void draw_backdrop(const char *title) {
    vita2d_start_drawing();
    vita2d_clear_screen();
    th_background();
    /* voile sombre */
    vita2d_draw_rectangle(0, 0, TH_SCR_W, TH_SCR_H, RGBA8(0x05, 0x07, 0x0c, 0xA0));
    vita2d_end_drawing();
    vita2d_common_dialog_update();
    vita2d_swap_buffers();
    (void)title;
}

int ime_input_text(const char *title, const char *initial, ImeMode mode,
                   char *out, int out_size) {
    if (!out || out_size < 2) return -1;
    if (!s_inited) ime_init();

    utf8_to_utf16(title ? title : "", s_title, 64);
    utf8_to_utf16(initial ? initial : "", s_initial, IME_MAX_LEN + 1);
    memset(s_input, 0, sizeof(s_input));

    SceImeDialogParam param;
    sceImeDialogParamInit(&param);
    param.supportedLanguages = 0x0001FFFF;
    param.languagesForced    = SCE_TRUE;
    switch (mode) {
        case IME_MODE_NUMBER: param.type = SCE_IME_TYPE_NUMBER;          break;
        case IME_MODE_IP:     param.type = SCE_IME_TYPE_EXTENDED_NUMBER; break;
        default:              param.type = SCE_IME_TYPE_BASIC_LATIN;     break;
    }
    param.title           = s_title;
    param.maxTextLength   = IME_MAX_LEN;
    param.initialText     = s_initial;
    param.inputTextBuffer = s_input;

    int rc = sceImeDialogInit(&param);
    if (rc < 0) return rc;

    int result = 0;
    for (;;) {
        draw_backdrop(title);

        SceCommonDialogStatus st = sceImeDialogGetStatus();
        if (st == SCE_COMMON_DIALOG_STATUS_FINISHED) {
            SceImeDialogResult res;
            memset(&res, 0, sizeof(res));
            sceImeDialogGetResult(&res);
            if (res.button == SCE_IME_DIALOG_BUTTON_ENTER) {
                utf16_to_utf8(s_input, out, out_size);
                result = 1;
            }
            sceImeDialogTerm();
            break;
        }
        if (st == SCE_COMMON_DIALOG_STATUS_NONE) {
            /* dialog fermé de façon inattendue */
            sceImeDialogTerm();
            break;
        }
    }

    /* petite pause pour laisser le dialog se démonter proprement */
    sceKernelDelayThread(20 * 1000);
    return result;
}
