#include "mapping.h"
#include "osc.h"
#include "net.h"

#include <math.h>
#include <string.h>

#define CHANGE_EPS 0.0005f

static int s_touch_osc = 1;

void mapping_set_touch_osc(int enabled) { s_touch_osc = enabled ? 1 : 0; }

static int is_touch_src(SrcId src) {
    return src >= SRC_TF0_X && src <= SRC_TB0_Y;
}

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

/* Calcule la valeur NORMALISÉE (0..1) d'un contrôle, deadzone + lissage + invert
 * appliqués. Le lissage utilise l'accumulateur du contrôle (donc non-const). */
static float compute_norm(Control *c, const InputState *st) {
    float raw = input_source_value(st, c->src);

    float smin, smax;
    input_source_range(c->src, &smin, &smax);

    if (c->absolute && smin < 0.0f) {
        raw = fabsf(raw);
        smin = 0.0f;
        smax = 1.0f;
    }

    /* Deadzone : sur les sources centrées (min < 0), autour de 0. */
    if (c->deadzone > 0.0f && smin < 0.0f) {
        float thresh = c->deadzone * smax; /* smax = amplitude positive */
        if (fabsf(raw) < thresh) raw = 0.0f;
    }

    float span = (smax - smin);
    float norm = span != 0.0f ? (raw - smin) / span : 0.0f;
    norm = clampf(norm, 0.0f, 1.0f);

    /* Deadzone sur sources unipolaires (0..1), autour de 0. */
    if (c->deadzone > 0.0f && smin >= 0.0f && norm < c->deadzone) norm = 0.0f;

    /* Lissage exponentiel : alpha=1 -> pas de lissage. */
    if (c->smoothing > 0.0f) {
        float alpha = 1.0f - clampf(c->smoothing, 0.0f, 0.98f);
        if (!c->has_last) c->smoothed = norm;
        c->smoothed += alpha * (norm - c->smoothed);
        norm = c->smoothed;
    }

    if (c->invert) norm = 1.0f - norm;
    return norm;
}

static float norm_to_out(const Control *c, float norm) {
    return c->out_min + norm * (c->out_max - c->out_min);
}

/* Écrit un message (adresse + 1 argument) dans le writer. */
static void write_control_message(OscWriter *w, const Control *c, float out) {
    const char *tt = c->type == OSC_TYPE_INT ? "i" : "f";
    osc_message(w, c->address, tt);
    if (c->type == OSC_TYPE_INT) osc_push_int(w, (int32_t)lroundf(out));
    else                         osc_push_float(w, out);
}

/* Envoie out_min une fois au relâchement tactile (évite paramètre « collé »). */
static int emit_touch_release_zero(Control *c, float *out_value) {
    if (!c->zero_on_release || !is_touch_src(c->src) || !c->has_last)
        return 0;
    *out_value = c->out_min;
    c->has_last = 0;
    c->smoothed = 0.0f;
    c->last_norm = 0.0f;
    return 1;
}

/* Détermine si un contrôle non groupé doit émettre à ce tick, et sa valeur. */
static int should_emit(Control *c, const InputState *st, float *out_value) {
    if (!input_source_touch_active(st, c->src)) {
        if (emit_touch_release_zero(c, out_value)) return 1;
        c->has_last = 0;
        return 0;
    }

    float norm = compute_norm(c, st);

    switch (c->mode) {
        case MODE_CONTINUOUS:
            *out_value = norm_to_out(c, norm);
            c->last_norm = norm; c->has_last = 1;
            return 1;

        case MODE_ON_CHANGE:
            if (!c->has_last || fabsf(norm - c->last_norm) > CHANGE_EPS) {
                *out_value = norm_to_out(c, norm);
                c->last_norm = norm; c->has_last = 1;
                return 1;
            }
            return 0;

        case MODE_TRIGGER: {
            int high = norm >= 0.5f;
            int fire = high && !c->prev_high; /* front montant */
            c->prev_high = high;
            if (fire) { *out_value = c->out_max; return 1; }
            return 0;
        }
    }
    return 0;
}

void mapping_reset(Config *cfg) {
    for (int i = 0; i < cfg->control_count; i++) {
        cfg->controls[i].has_last = 0;
        cfg->controls[i].smoothed = 0.0f;
        cfg->controls[i].last_norm = 0.0f;
        cfg->controls[i].prev_high = 0;
    }
}

int mapping_tick(Config *cfg, const InputState *st) {
    int sent = 0;
    if (!net_target_ready()) return 0;

    /* --- 1) Contrôles individuels (bundle_group == 0) --- */
    for (int i = 0; i < cfg->control_count; i++) {
        Control *c = &cfg->controls[i];
        if (!c->enabled || c->bundle_group != 0) continue;
        if (!s_touch_osc && is_touch_src(c->src)) continue;

        float out;
        if (!should_emit(c, st, &out)) continue;

        OscWriter w;
        osc_begin(&w);
        write_control_message(&w, c, out);
        if (!w.overflow && net_send(osc_data(&w), osc_size(&w)) >= 0) sent++;
    }

    /* --- 2) Groupes de bundle (>0) : un paquet par groupe --- */
    /* Les groupes utilisés dans le preset vont de 1 à 3, mais on balaie large. */
    for (int g = 1; g <= 8; g++) {
        if (!s_touch_osc) {
            int touch_bundle = 0;
            for (int i = 0; i < cfg->control_count; i++) {
                if (cfg->controls[i].bundle_group == g && is_touch_src(cfg->controls[i].src)) {
                    touch_bundle = 1; break;
                }
            }
            if (touch_bundle) continue;
        }
        /* Le groupe émet si au moins un de ses membres est actif/à émettre.
         * Pour un point tactile, tous les membres partagent le même "actif". */
        int any = 0, active = 1;
        for (int i = 0; i < cfg->control_count; i++) {
            Control *c = &cfg->controls[i];
            if (c->enabled && c->bundle_group == g) {
                any = 1;
                if (!input_source_touch_active(st, c->src)) active = 0;
            }
        }
        if (!any) continue;
        if (!active) {
            int release = 0;
            for (int i = 0; i < cfg->control_count; i++) {
                Control *c = &cfg->controls[i];
                if (c->enabled && c->bundle_group == g && c->zero_on_release &&
                    is_touch_src(c->src) && c->has_last) {
                    release = 1;
                    break;
                }
            }
            if (release) {
                OscWriter w;
                osc_begin(&w);
                osc_bundle_begin(&w);
                int members = 0;
                for (int i = 0; i < cfg->control_count; i++) {
                    Control *c = &cfg->controls[i];
                    if (!c->enabled || c->bundle_group != g || !c->zero_on_release ||
                        !is_touch_src(c->src))
                        continue;
                    osc_bundle_element_begin(&w);
                    write_control_message(&w, c, c->out_min);
                    osc_bundle_element_end(&w);
                    members++;
                }
                if (members > 0 && !w.overflow &&
                    net_send(osc_data(&w), osc_size(&w)) >= 0)
                    sent++;
            }
            for (int i = 0; i < cfg->control_count; i++)
                if (cfg->controls[i].bundle_group == g) {
                    cfg->controls[i].has_last = 0;
                    cfg->controls[i].smoothed = 0.0f;
                }
            continue;
        }

        OscWriter w;
        osc_begin(&w);
        osc_bundle_begin(&w);
        int members = 0;
        for (int i = 0; i < cfg->control_count; i++) {
            Control *c = &cfg->controls[i];
            if (!c->enabled || c->bundle_group != g) continue;
            float norm = compute_norm(c, st);
            float out = norm_to_out(c, norm);
            c->last_norm = norm; c->has_last = 1;
            osc_bundle_element_begin(&w);
            write_control_message(&w, c, out);
            osc_bundle_element_end(&w);
            members++;
        }
        if (members > 0 && !w.overflow && net_send(osc_data(&w), osc_size(&w)) >= 0)
            sent++;
    }

    return sent;
}

float mapping_preview_value(const Config *cfg, const Control *c, const InputState *st) {
    (void)cfg;
    /* Copie locale pour ne pas toucher l'état runtime pendant l'affichage. */
    Control tmp = *c;
    float norm = compute_norm(&tmp, st);
    return norm_to_out(c, norm);
}
