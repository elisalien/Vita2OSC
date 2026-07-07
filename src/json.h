/*
 * json.h — Parseur JSON minimal, sans allocation dynamique.
 *
 * Le texte d'entrée est modifié en place (les chaînes sont terminées par '\0').
 * Les noeuds sont alloués depuis un pool fourni par l'appelant.
 * Suffisant pour lire le schéma de config de Vita2OSC.
 */
#ifndef VITA2OSC_JSON_H
#define VITA2OSC_JSON_H

typedef enum {
    JSON_NULL, JSON_BOOL, JSON_NUM, JSON_STR, JSON_ARR, JSON_OBJ
} JsonType;

typedef struct JsonNode JsonNode;
struct JsonNode {
    JsonType  type;
    const char *key;   /* clé si membre d'objet, sinon NULL */
    const char *sval;  /* si JSON_STR */
    double    nval;    /* si JSON_NUM */
    int       bval;    /* si JSON_BOOL */
    JsonNode *child;   /* premier enfant (élément de tableau / membre d'objet) */
    JsonNode *next;    /* frère suivant */
};

/* Parse `text` (mutable). Retourne le noeud racine ou NULL si erreur/pool plein. */
JsonNode *json_parse(char *text, JsonNode *pool, int pool_size);

/* Accès objet par clé (NULL si absent). */
JsonNode *json_obj_get(const JsonNode *obj, const char *key);

/* Helpers avec valeur par défaut. */
double      json_num(const JsonNode *obj, const char *key, double def);
int         json_bool(const JsonNode *obj, const char *key, int def);
const char *json_str(const JsonNode *obj, const char *key, const char *def);

#endif /* VITA2OSC_JSON_H */
