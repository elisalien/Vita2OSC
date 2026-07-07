/*
 * Vita2OSC — point d'entrée.
 *
 * Boucle : lecture des entrées -> navigation onglets (L/R) -> moteur de mapping
 * (émission OSC à la fréquence configurée) -> rendu UI. Quitter : SELECT+START.
 */
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>

#include "input.h"
#include "config.h"
#include "mapping.h"
#include "net.h"
#include "ui.h"
#include "storage.h"
#include "httpd.h"
#include "colors.h"

#define HTTPD_PORT 8080
#define SCR_W 960
#define SCR_H 544

int main(void) {
    Config cfg;
    InputState st;
    char local_ip[16] = "0.0.0.0";

    config_load_default(&cfg);
    storage_load_config(&cfg); /* écrase par la config sauvegardée si présente */

    ui_init();
    input_init();

    int net_ok = 0;
    if (net_init() == 0) {
        net_get_local_ip(local_ip, sizeof(local_ip));
        net_set_target(cfg.target_ip, cfg.target_port);
        net_ok = net_target_ready();
        httpd_start(&cfg, HTTPD_PORT); /* config à distance via la webapp */
    }
    mapping_reset(&cfg);

    UiTab tab = TAB_BUTTONS;
    int prev_l = 0, prev_r = 0;

    /* Cadence d'envoi indépendante du rendu. */
    SceUInt64 last_us = sceKernelGetProcessTimeWide();
    double acc_us = 0.0;
    double period_us = 1000000.0 / (cfg.send_rate_hz > 0 ? cfg.send_rate_hz : 60);

    /* Compteur paquets/s. */
    int pkt_count = 0, pps = 0;
    double sec_acc = 0.0;

    for (;;) {
        input_poll(&st);

        /* Navigation onglets sur front montant de L / R. */
        int l = input_source_value(&st, SRC_BTN_L) > 0.5f;
        int r = input_source_value(&st, SRC_BTN_R) > 0.5f;
        if (l && !prev_l) tab = (UiTab)((tab + TAB_COUNT - 1) % TAB_COUNT);
        if (r && !prev_r) tab = (UiTab)((tab + 1) % TAB_COUNT);
        prev_l = l; prev_r = r;

        mapping_set_touch_osc(tab != TAB_COLORS);

        /* Tactile dédié à l'onglet Couleurs (pas d'OSC touch). */
        if (tab == TAB_COLORS && st.front[0].active) {
            int px = (int)(st.front[0].x * (float)SCR_W);
            int py = (int)(st.front[0].y * (float)SCR_H);
            colors_handle_touch(&cfg.colors, px, py, 1);
        }

        /* Quitter : SELECT + START maintenus. */
        if (input_source_value(&st, SRC_BTN_SELECT) > 0.5f &&
            input_source_value(&st, SRC_BTN_START)  > 0.5f) {
            break;
        }

        /* Delta temps. */
        SceUInt64 now_us = sceKernelGetProcessTimeWide();
        double dt = (double)(now_us - last_us);
        last_us = now_us;
        acc_us += dt;
        sec_acc += dt;

        /* Section protégée : la config peut être remplacée par le serveur HTTP. */
        httpd_lock();

        /* Config modifiée à distance ? -> appliquer à chaud + sauvegarder. */
        if (httpd_take_dirty()) {
            net_set_target(cfg.target_ip, cfg.target_port);
            net_ok = net_target_ready();
            mapping_reset(&cfg);
            period_us = 1000000.0 / (cfg.send_rate_hz > 0 ? cfg.send_rate_hz : 60);
            storage_save_config(&cfg);
        }
        int guard = 0;
        while (acc_us >= period_us && guard++ < 4) {
            if (net_ok) pkt_count += mapping_tick(&cfg, &st);
            acc_us -= period_us;
        }

        if (sec_acc >= 1000000.0) {
            pps = pkt_count;
            pkt_count = 0;
            sec_acc -= 1000000.0;
        }

        ui_render(&cfg, &st, tab, local_ip, net_ok, pps);
        httpd_unlock();
        /* vita2d_swap_buffers() (dans ui_render) synchronise ~60 fps. */
    }

    httpd_stop();
    input_finish();
    net_finish();
    ui_finish();
    sceKernelExitProcess(0);
    return 0;
}
