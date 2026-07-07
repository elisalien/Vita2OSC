/*
 * osc.h — Sérialisation OSC 1.0 (Open Sound Control) minimale.
 *
 * On construit des messages dans un buffer octet, prêts à être envoyés en UDP.
 * Types supportés ici : 'f' (float32), 'i' (int32). Extensible facilement.
 *
 * Format d'un message OSC :
 *   [address string, padded 4]  [type tag string ",xx" padded 4]  [args big-endian]
 */
#ifndef VITA2OSC_OSC_H
#define VITA2OSC_OSC_H

#include <stdint.h>
#include <stddef.h>

#define OSC_MAX_PACKET 1024

/* Buffer d'écriture OSC. Réinitialiser avec osc_begin avant chaque message. */
typedef struct {
    uint8_t buf[OSC_MAX_PACKET];
    size_t  len;      /* octets écrits */
    int     overflow; /* 1 si on a dépassé la capacité */
} OscWriter;

/* --- Message simple --- */
void   osc_begin(OscWriter *w);
/* Démarre un message : écrit l'adresse et le type tag (ex. ",f" ou ",ff"). */
int    osc_message(OscWriter *w, const char *address, const char *typetags);
/* Ajoute les arguments dans l'ordre du type tag. */
int    osc_push_float(OscWriter *w, float v);
int    osc_push_int(OscWriter *w, int32_t v);

/* --- Bundle : regroupe plusieurs messages dans un paquet horodaté --- */
/* Ouvre un bundle avec timetag "immédiat". Puis, pour chaque message :
 *   osc_bundle_element_begin -> osc_message/osc_push_* -> osc_bundle_element_end */
int    osc_bundle_begin(OscWriter *w);
int    osc_bundle_element_begin(OscWriter *w);   /* réserve la taille d'élément */
int    osc_bundle_element_end(OscWriter *w);     /* écrit la taille de l'élément */

/* Pointeur/longueur finaux pour l'envoi. */
static inline const uint8_t *osc_data(const OscWriter *w) { return w->buf; }
static inline size_t         osc_size(const OscWriter *w) { return w->len; }

#endif /* VITA2OSC_OSC_H */
