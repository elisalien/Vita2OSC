#include "config_json.h"
#include "config.h"
#include "json.h"
#include "colors.h"
#include <stdio.h>
#include <string.h>

#define JSON_POOL 2048

static OscType parse_type(const char *tt) {
    return (tt && tt[0] == 'i') ? OSC_TYPE_INT : OSC_TYPE_FLOAT;
}

static SendMode parse_mode(const char *m) {
    if (!m) return MODE_ON_CHANGE;
    if (strcmp(m, "continuous") == 0) return MODE_CONTINUOUS;
    if (strcmp(m, "trigger") == 0)    return MODE_TRIGGER;
    return MODE_ON_CHANGE;
}

static void parse_color_param(JsonNode *obj, const char *key,
                              ColorParam *p, const ColorParam *def) {
    JsonNode *n = json_obj_get(obj, key);
    if (!n || n->type != JSON_OBJ) {
        *p = *def;
        return;
    }
    strncpy(p->address, json_str(n, "address", def->address), COLOR_ADDR_LEN - 1);
    p->value = (float)json_num(n, "value", def->value);
}

static void parse_colors(ColorConfig *cc, JsonNode *root) {
    JsonNode *col = json_obj_get(root, "colors");
    if (!col || col->type != JSON_OBJ) return;

    ColorConfig def;
    colors_load_default(&def);
    *cc = def;

    cc->auto_send = json_bool(col, "autoSend", cc->auto_send);
    int gc = (int)json_num(col, "groupCount", cc->effects[0].group_count);
    if (gc < 1) gc = 1;
    if (gc > COLOR_MAX_GROUPS) gc = COLOR_MAX_GROUPS;

    JsonNode *effs = json_obj_get(col, "effects");
    if (!effs || effs->type != JSON_ARR) return;

    int ei = 0;
    for (JsonNode *e = effs->child; e && ei < COLOR_MAX_EFFECTS; e = e->next, ei++) {
        if (e->type != JSON_OBJ) continue;
        ColorEffect *fx = &cc->effects[ei];
        ColorEffect dfx = def.effects[ei < def.effect_count ? ei : 0];

        const char *id = json_str(e, "id", dfx.id);
        const char *nm = json_str(e, "name", dfx.name);
        strncpy(fx->id, id, sizeof(fx->id) - 1);
        strncpy(fx->name, nm, sizeof(fx->name) - 1);
        strncpy(fx->effect_address,
                json_str(e, "effectAddress", dfx.effect_address),
                COLOR_ADDR_LEN - 1);

        parse_color_param(e, "hue",    &fx->hue,    &dfx.hue);
        parse_color_param(e, "sat",    &fx->sat,    &dfx.sat);
        parse_color_param(e, "bright", &fx->bright, &dfx.bright);
        parse_color_param(e, "alpha",  &fx->alpha,  &dfx.alpha);

        fx->group_count = gc;
        JsonNode *grps = json_obj_get(e, "groups");
        if (grps && grps->type == JSON_ARR) {
            int gi = 0;
            for (JsonNode *g = grps->child; g && gi < gc; g = g->next, gi++) {
                if (g->type != JSON_OBJ) continue;
                strncpy(fx->groups[gi].address,
                        json_str(g, "address", dfx.groups[gi].address),
                        COLOR_ADDR_LEN - 1);
                fx->groups[gi].enabled = json_bool(g, "enabled", 1);
            }
            for (; gi < gc; gi++) {
                fx->groups[gi] = dfx.groups[gi];
            }
        }
    }
    cc->effect_count = ei > 0 ? ei : def.effect_count;
}

static void append_colors(const ColorConfig *cc, char *out, int out_size, int *n) {
    #define APP(...) do { \
        int r = snprintf(out + *n, out_size - *n, __VA_ARGS__); \
        if (r < 0 || r >= out_size - *n) return; \
        *n += r; \
    } while (0)

    int gc = cc->effects[0].group_count;
    APP(",\n  \"colors\": {\n");
    APP("    \"groupCount\": %d,\n", gc);
    APP("    \"autoSend\": %s,\n", cc->auto_send ? "true" : "false");
    APP("    \"effects\": [\n");

    for (int ei = 0; ei < cc->effect_count; ei++) {
        const ColorEffect *fx = &cc->effects[ei];
        APP("      {\n");
        APP("        \"id\": \"%s\", \"name\": \"%s\",\n", fx->id, fx->name);
        APP("        \"effectAddress\": \"%s\",\n", fx->effect_address);
        APP("        \"hue\": {\"address\":\"%s\",\"value\":%.4f},\n",
            fx->hue.address, fx->hue.value);
        APP("        \"sat\": {\"address\":\"%s\",\"value\":%.4f},\n",
            fx->sat.address, fx->sat.value);
        APP("        \"bright\": {\"address\":\"%s\",\"value\":%.4f},\n",
            fx->bright.address, fx->bright.value);
        APP("        \"alpha\": {\"address\":\"%s\",\"value\":%.4f},\n",
            fx->alpha.address, fx->alpha.value);
        APP("        \"groups\": [\n");
        for (int gi = 0; gi < fx->group_count; gi++) {
            const ColorGroup *g = &fx->groups[gi];
            APP("          {\"address\":\"%s\",\"enabled\":%s}%s\n",
                g->address, g->enabled ? "true" : "false",
                (gi + 1 < fx->group_count) ? "," : "");
        }
        APP("        ]\n");
        APP("      }%s\n", (ei + 1 < cc->effect_count) ? "," : "");
    }
    APP("    ]\n  }\n");
    #undef APP
}

int config_to_json(const Config *cfg, char *out, int out_size) {
    int n = 0;
    #define APP(...) do { \
        int r = snprintf(out + n, out_size - n, __VA_ARGS__); \
        if (r < 0 || r >= out_size - n) return -1; \
        n += r; \
    } while (0)

    APP("{\n");
    APP("  \"version\": 1,\n");
    APP("  \"target\": { \"ip\": \"%s\", \"port\": %u },\n",
        cfg->target_ip, cfg->target_port);
    APP("  \"globalPrefix\": \"%s\",\n", cfg->global_prefix);
    APP("  \"sendRateHz\": %d,\n", cfg->send_rate_hz);
    APP("  \"controls\": [\n");

    for (int i = 0; i < cfg->control_count; i++) {
        const Control *c = &cfg->controls[i];
        APP("    { \"source\": \"%s\", \"enabled\": %s, \"address\": \"%s\", "
            "\"typeTag\": \"%s\", \"min\": %.4f, \"max\": %.4f, "
            "\"deadzone\": %.4f, \"smoothing\": %.4f, \"invert\": %s, "
            "\"mode\": \"%s\", \"bundleGroup\": %d }%s\n",
            input_source_name(c->src),
            c->enabled ? "true" : "false",
            c->address,
            config_type_tag(c->type),
            c->out_min, c->out_max,
            c->deadzone, c->smoothing,
            c->invert ? "true" : "false",
            config_mode_label(c->mode),
            c->bundle_group,
            (i + 1 < cfg->control_count) ? "," : "");
    }

    APP("  ]");
    append_colors(&cfg->colors, out, out_size, &n);
    APP("}\n");
    #undef APP
    return n;
}

int config_from_json(Config *cfg, char *text) {
    static JsonNode pool[JSON_POOL];
    JsonNode *root = json_parse(text, pool, JSON_POOL);
    if (!root || root->type != JSON_OBJ) return -1;

    /* Repartir du preset par défaut puis écraser avec le JSON reçu. */
    Config in;
    config_load_default(&in);

    JsonNode *tgt = json_obj_get(root, "target");
    if (tgt) {
        const char *ip = json_str(tgt, "ip", in.target_ip);
        strncpy(in.target_ip, ip, sizeof(in.target_ip) - 1);
        in.target_port = (uint16_t)json_num(tgt, "port", in.target_port);
    }
    const char *pfx = json_str(root, "globalPrefix", in.global_prefix);
    strncpy(in.global_prefix, pfx, sizeof(in.global_prefix) - 1);
    in.send_rate_hz = (int)json_num(root, "sendRateHz", in.send_rate_hz);
    if (in.send_rate_hz < 1) in.send_rate_hz = 1;
    if (in.send_rate_hz > 240) in.send_rate_hz = 240;

    JsonNode *ctrls = json_obj_get(root, "controls");
    if (ctrls && ctrls->type == JSON_ARR && ctrls->child) {
        in.control_count = 0;
        for (JsonNode *e = ctrls->child; e && in.control_count < CFG_MAX_CONTROLS; e = e->next) {
            if (e->type != JSON_OBJ) continue;
            SrcId src = input_source_from_name(json_str(e, "source", "none"));
            if (src == SRC_NONE) continue;

            Control *c = &in.controls[in.control_count++];
            memset(c, 0, sizeof(*c));
            c->src = src;
            c->enabled = json_bool(e, "enabled", 1);
            strncpy(c->address, json_str(e, "address", ""), CFG_ADDR_LEN - 1);
            c->type = parse_type(json_str(e, "typeTag", "f"));
            c->out_min = (float)json_num(e, "min", 0.0);
            c->out_max = (float)json_num(e, "max", 1.0);
            c->deadzone = (float)json_num(e, "deadzone", 0.0);
            c->smoothing = (float)json_num(e, "smoothing", 0.0);
            c->invert = json_bool(e, "invert", 0);
            c->mode = parse_mode(json_str(e, "mode", "onChange"));
            c->bundle_group = (int)json_num(e, "bundleGroup", 0);
        }
    }

    parse_colors(&in.colors, root);

    if (in.control_count == 0) return -1;
    *cfg = in;
    return 0;
}
