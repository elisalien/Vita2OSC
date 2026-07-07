#include "json.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    char     *p;        /* curseur de lecture (buffer mutable) */
    JsonNode *pool;
    int       pool_size;
    int       used;
    int       error;
} Parser;

static JsonNode *alloc_node(Parser *ps) {
    if (ps->used >= ps->pool_size) { ps->error = 1; return NULL; }
    JsonNode *n = &ps->pool[ps->used++];
    memset(n, 0, sizeof(*n));
    return n;
}

static void skip_ws(Parser *ps) {
    while (*ps->p == ' ' || *ps->p == '\t' || *ps->p == '\n' || *ps->p == '\r')
        ps->p++;
}

static JsonNode *parse_value(Parser *ps);

/* Lit une chaîne JSON ; termine par '\0' dans le buffer, gère \" \\ \/ \n \t. */
static const char *parse_string_raw(Parser *ps) {
    if (*ps->p != '"') { ps->error = 1; return NULL; }
    ps->p++;
    char *out = ps->p;      /* on réécrit décodé sur place */
    const char *start = out;
    while (*ps->p && *ps->p != '"') {
        if (*ps->p == '\\') {
            ps->p++;
            switch (*ps->p) {
                case 'n': *out++ = '\n'; break;
                case 't': *out++ = '\t'; break;
                case 'r': *out++ = '\r'; break;
                case '/': *out++ = '/';  break;
                case '\\': *out++ = '\\'; break;
                case '"': *out++ = '"';  break;
                default:  *out++ = *ps->p; break; /* unicode \u non géré : brut */
            }
            if (*ps->p) ps->p++;
        } else {
            *out++ = *ps->p++;
        }
    }
    if (*ps->p != '"') { ps->error = 1; return NULL; }
    ps->p++;          /* consomme le guillemet fermant */
    *out = '\0';      /* termine la chaîne décodée */
    return start;
}

static JsonNode *parse_object(Parser *ps) {
    JsonNode *obj = alloc_node(ps);
    if (!obj) return NULL;
    obj->type = JSON_OBJ;
    ps->p++; /* '{' */
    skip_ws(ps);
    JsonNode *last = NULL;
    if (*ps->p == '}') { ps->p++; return obj; }
    for (;;) {
        skip_ws(ps);
        const char *key = parse_string_raw(ps);
        if (ps->error) return NULL;
        skip_ws(ps);
        if (*ps->p != ':') { ps->error = 1; return NULL; }
        ps->p++;
        JsonNode *val = parse_value(ps);
        if (!val) return NULL;
        val->key = key;
        if (last) last->next = val; else obj->child = val;
        last = val;
        skip_ws(ps);
        if (*ps->p == ',') { ps->p++; continue; }
        if (*ps->p == '}') { ps->p++; break; }
        ps->error = 1; return NULL;
    }
    return obj;
}

static JsonNode *parse_array(Parser *ps) {
    JsonNode *arr = alloc_node(ps);
    if (!arr) return NULL;
    arr->type = JSON_ARR;
    ps->p++; /* '[' */
    skip_ws(ps);
    JsonNode *last = NULL;
    if (*ps->p == ']') { ps->p++; return arr; }
    for (;;) {
        JsonNode *val = parse_value(ps);
        if (!val) return NULL;
        if (last) last->next = val; else arr->child = val;
        last = val;
        skip_ws(ps);
        if (*ps->p == ',') { ps->p++; continue; }
        if (*ps->p == ']') { ps->p++; break; }
        ps->error = 1; return NULL;
    }
    return arr;
}

static JsonNode *parse_value(Parser *ps) {
    skip_ws(ps);
    char c = *ps->p;
    if (c == '{') return parse_object(ps);
    if (c == '[') return parse_array(ps);
    if (c == '"') {
        JsonNode *n = alloc_node(ps);
        if (!n) return NULL;
        n->type = JSON_STR;
        n->sval = parse_string_raw(ps);
        return ps->error ? NULL : n;
    }
    if (c == 't' || c == 'f') {
        JsonNode *n = alloc_node(ps);
        if (!n) return NULL;
        n->type = JSON_BOOL;
        if (strncmp(ps->p, "true", 4) == 0)  { n->bval = 1; ps->p += 4; }
        else if (strncmp(ps->p, "false", 5) == 0) { n->bval = 0; ps->p += 5; }
        else { ps->error = 1; return NULL; }
        return n;
    }
    if (c == 'n') {
        JsonNode *n = alloc_node(ps);
        if (!n) return NULL;
        n->type = JSON_NULL;
        if (strncmp(ps->p, "null", 4) == 0) ps->p += 4; else { ps->error = 1; return NULL; }
        return n;
    }
    if (c == '-' || (c >= '0' && c <= '9')) {
        JsonNode *n = alloc_node(ps);
        if (!n) return NULL;
        n->type = JSON_NUM;
        char *end = NULL;
        n->nval = strtod(ps->p, &end);
        if (end == ps->p) { ps->error = 1; return NULL; }
        ps->p = end;
        return n;
    }
    ps->error = 1;
    return NULL;
}

JsonNode *json_parse(char *text, JsonNode *pool, int pool_size) {
    Parser ps = { text, pool, pool_size, 0, 0 };
    JsonNode *root = parse_value(&ps);
    if (ps.error) return NULL;
    return root;
}

JsonNode *json_obj_get(const JsonNode *obj, const char *key) {
    if (!obj || obj->type != JSON_OBJ) return NULL;
    for (JsonNode *c = obj->child; c; c = c->next)
        if (c->key && strcmp(c->key, key) == 0) return c;
    return NULL;
}

double json_num(const JsonNode *obj, const char *key, double def) {
    JsonNode *n = json_obj_get(obj, key);
    return (n && n->type == JSON_NUM) ? n->nval : def;
}

int json_bool(const JsonNode *obj, const char *key, int def) {
    JsonNode *n = json_obj_get(obj, key);
    if (!n) return def;
    if (n->type == JSON_BOOL) return n->bval;
    if (n->type == JSON_NUM)  return n->nval != 0.0;
    return def;
}

const char *json_str(const JsonNode *obj, const char *key, const char *def) {
    JsonNode *n = json_obj_get(obj, key);
    return (n && n->type == JSON_STR) ? n->sval : def;
}
