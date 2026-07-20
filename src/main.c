/*
 * Vita2OSC — point d'entrée.
 *
 * Boucle : lecture des entrées -> navigation (L/R + rail tactile) -> moteur
 * de mapping (émission OSC à la fréquence configurée) -> rendu UI.
 * Quitter : maintenir SELECT+START (~0,8 s).
 */
#include <stdio.h>
#include <string.h>

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/power.h>

#include "boot_log.h"
#include "input.h"
#include "config.h"
#include "mapping.h"
#include "net.h"
#include "ui.h"
#include "storage.h"
#include "httpd.h"
#include "colors.h"
#include "settings_edit.h"
#include "ime_input.h"

#define HTTPD_PORT 8080
#define EXIT_HOLD_US 800000   /* maintien SELECT+START pour quitter */

/* Heap processus : vita2d/GXM consomment plusieurs Mo au démarrage. */
int _newlib_heap_size_user = 16 * 1024 * 1024;

/* Applique IP/port de l'éditeur à la config + réseau + sauvegarde.
 * À appeler sous httpd_lock(). Retourne 1 si la cible est joignable. */
static int apply_target(Config *cfg, SettingsEdit *sedit) {
    settings_edit_to_config(sedit, cfg);
    net_set_target(cfg->target_ip, cfg->target_port);
    storage_save_config(cfg);
    return net_target_ready();
}

/* Ouvre le clavier système pour l'IP ou le port, applique si valide. */
static void edit_with_keyboard(Config *cfg, SettingsEdit *sedit,
                               int is_port, int *net_ok) {
    char initial[24], out[64];

    if (is_port) snprintf(initial, sizeof(initial), "%u", (unsigned)sedit->port);
    else         settings_edit_ip_string(sedit, initial, sizeof(initial));

    int ok = ime_input_text(is_port ? "Port OSC (1-65535)"
                                    : "IP cible OSC (ex : 192.168.1.42)",
                            initial, is_port ? IME_MODE_NUMBER : IME_MODE_IP,
                            out, sizeof(out));
    if (ok != 1) return; /* annulé */

    int valid = is_port ? settings_edit_set_port(sedit, out)
                        : settings_edit_set_ip(sedit, out);
    if (!valid) {
        ui_toast(is_port ? "Port invalide (1-65535)"
                         : "IP invalide - format a.b.c.d", 2);
        return;
    }

    httpd_lock();
    *net_ok = apply_target(cfg, sedit);
    sedit->dirty = 0;
    httpd_unlock();
    ui_toast(*net_ok ? "Cible OSC appliquee"
                     : "Applique - cible injoignable", *net_ok ? 1 : 2);
}

int main(void) {
    static Config cfg;          /* statique : grosse structure */
    static InputState st;
    char local_ip[16] = "0.0.0.0";

    /* --------- démarrage avec splash pixel art --------- */
    boot_log("boot: start");

    if (ui_init() < 0) {
        boot_log("boot: ui_init failed");
        sceKernelExitProcess(1);
        return 1;
    }
    boot_log("boot: ui ok");
    ui_splash_to(0.12f, "Interface");

    config_load_default(&cfg);
    if (storage_load_config(&cfg) < 0)
        boot_log("boot: config default (no save or invalid json)");
    else
        boot_log("boot: config loaded");
    ui_splash_to(0.35f, "Configuration");

    if (input_init() < 0)
        boot_log("boot: input partial");
    else
        boot_log("boot: input ok");
    ui_splash_to(0.55f, "Entrees");

    int net_ok = 0;
    if (net_init() == 0) {
        boot_log("boot: net ok");
        net_get_local_ip(local_ip, sizeof(local_ip));
        net_set_target(cfg.target_ip, cfg.target_port);
        net_ok = net_target_ready();
        if (httpd_start(&cfg, HTTPD_PORT) == 0)
            boot_log("boot: httpd ok");
        else
            boot_log("boot: httpd failed");
    } else {
        boot_log("boot: net failed (offline?)");
    }
    ui_splash_to(0.80f, "Reseau");
    mapping_reset(&cfg);
    ui_splash_to(1.0f, "Pret");
    boot_log("boot: ready");
    sceKernelDelayThread(250 * 1000);

    UiTab tab = TAB_BUTTONS;
    int prev_l = 0, prev_r = 0;
    SettingsEdit sedit;
    settings_edit_from_config(&sedit, &cfg);

    /* Cadence d'envoi indépendante du rendu. */
    SceUInt64 last_us = sceKernelGetProcessTimeWide();
    double acc_us = 0.0;
    double period_us = 1000000.0 / (cfg.send_rate_hz > 0 ? cfg.send_rate_hz : 60);

    /* Compteur paquets/s. */
    int pkt_count = 0, pps = 0;
    double sec_acc = 0.0;

    /* Batterie (rafraîchie ~2 s). */
    int battery = -1, charging = 0;
    double bat_acc = 1e9;

    /* Tactile UI (front montant / glissé). */
    int prev_touch = 0;

    /* Maintien SELECT+START pour quitter. */
    SceUInt64 exit_since = 0;

    for (;;) {
        input_poll(&st);

        /* Navigation onglets sur front montant de L / R. */
        int l = input_source_value(&st, SRC_BTN_L) > 0.5f;
        int r = input_source_value(&st, SRC_BTN_R) > 0.5f;
        if (l && !prev_l) tab = (UiTab)((tab + TAB_COUNT - 1) % TAB_COUNT);
        if (r && !prev_r) tab = (UiTab)((tab + 1) % TAB_COUNT);
        prev_l = l; prev_r = r;

        /* Le tactile sert à l'UI sur Couleurs/Réglages : pas d'OSC touch. */
        mapping_set_touch_osc(tab != TAB_COLORS && tab != TAB_SETTINGS);

        /* --------- tactile UI --------- */
        UiAction act = UI_ACT_NONE;
        int touching = st.front[0].active;
        if (touching) {
            int px = (int)(st.front[0].x * (float)960);
            int py = (int)(st.front[0].y * (float)544);
            if (!prev_touch) {
                httpd_lock();
                act = ui_touch_press(&cfg, &sedit, &tab, px, py);
                httpd_unlock();
            } else {
                httpd_lock();
                ui_touch_drag(&cfg, tab, px, py);
                httpd_unlock();
            }
        }
        prev_touch = touching;

        /* --------- quitter : maintien SELECT+START --------- */
        float exit_t = 0.0f;
        if (input_source_value(&st, SRC_BTN_SELECT) > 0.5f &&
            input_source_value(&st, SRC_BTN_START)  > 0.5f) {
            SceUInt64 now = sceKernelGetProcessTimeWide();
            if (!exit_since) exit_since = now;
            exit_t = (float)(now - exit_since) / (float)EXIT_HOLD_US;
            if (exit_t >= 1.0f) break;
        } else {
            exit_since = 0;
        }

        /* --------- édition Réglages (D-pad + △) --------- */
        if (tab == TAB_SETTINGS) {
            SedAction sa = settings_edit_handle(&sedit, &st);
            if (sa == SED_APPLY)         act = UI_ACT_APPLY_SETTINGS;
            else if (sa == SED_IME_IP)   act = UI_ACT_IME_IP;
            else if (sa == SED_IME_PORT) act = UI_ACT_IME_PORT;
        }

        /* --------- actions (IME hors verrou : rend ses propres frames) ---- */
        if (act == UI_ACT_IME_IP)
            edit_with_keyboard(&cfg, &sedit, 0, &net_ok);
        else if (act == UI_ACT_IME_PORT)
            edit_with_keyboard(&cfg, &sedit, 1, &net_ok);

        /* Delta temps. */
        SceUInt64 now_us = sceKernelGetProcessTimeWide();
        double dt = (double)(now_us - last_us);
        last_us = now_us;
        acc_us += dt;
        sec_acc += dt;
        bat_acc += dt;

        if (bat_acc >= 2000000.0) {
            battery = scePowerGetBatteryLifePercent();
            charging = scePowerIsBatteryCharging();
            if (battery < 0 || battery > 100) battery = -1;
            bat_acc = 0.0;
        }

        /* Section protégée : la config peut être remplacée par le serveur HTTP. */
        httpd_lock();

        /* Config modifiée à distance ? -> appliquer à chaud + sauvegarder. */
        if (httpd_take_dirty()) {
            settings_edit_from_config(&sedit, &cfg);
            net_set_target(cfg.target_ip, cfg.target_port);
            net_ok = net_target_ready();
            mapping_reset(&cfg);
            period_us = 1000000.0 / (cfg.send_rate_hz > 0 ? cfg.send_rate_hz : 60);
            storage_save_config(&cfg);
            ui_toast("Config webapp appliquee", 1);
        }
        if (act == UI_ACT_APPLY_SETTINGS) {
            net_ok = apply_target(&cfg, &sedit);
            ui_toast(net_ok ? "Cible OSC appliquee" : "Cible injoignable", net_ok ? 1 : 2);
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

        UiStatus status = {
            .net_ok = net_ok, .pps = pps,
            .battery = battery, .charging = charging,
            .exit_t = exit_t,
        };
        ui_render(&cfg, &st, tab, local_ip, &status, &sedit);
        httpd_unlock();
        /* vita2d_swap_buffers() (dans ui_render) synchronise ~60 fps. */
    }

    httpd_stop();
    input_finish();
    net_finish();
    ui_finish();
    boot_log("boot: exit");
    sceKernelExitProcess(0);
    return 0;
}
