#include "osc.h"
#include <string.h>

/* OSC est big-endian. On sérialise à la main pour ne dépendre de rien. */
static void put_u32be(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)(v);
}

/* Écrit n octets si la place existe, sinon marque overflow. */
static int raw(OscWriter *w, const void *data, size_t n) {
    if (w->overflow) return -1;
    if (w->len + n > OSC_MAX_PACKET) { w->overflow = 1; return -1; }
    memcpy(w->buf + w->len, data, n);
    w->len += n;
    return 0;
}

/* Chaîne OSC : contenu + au moins un '\0', paddé à un multiple de 4. */
static int put_string(OscWriter *w, const char *s) {
    size_t slen = strlen(s);
    size_t total = (slen + 1 + 3) & ~((size_t)3); /* +1 pour le nul, arrondi sup 4 */
    if (w->overflow || w->len + total > OSC_MAX_PACKET) { w->overflow = 1; return -1; }
    memcpy(w->buf + w->len, s, slen);
    memset(w->buf + w->len + slen, 0, total - slen); /* nul + padding */
    w->len += total;
    return 0;
}

static int put_u32(OscWriter *w, uint32_t v) {
    uint8_t tmp[4];
    put_u32be(tmp, v);
    return raw(w, tmp, 4);
}

void osc_begin(OscWriter *w) {
    w->len = 0;
    w->overflow = 0;
}

int osc_message(OscWriter *w, const char *address, const char *typetags) {
    if (put_string(w, address) != 0) return -1;
    /* le type tag OSC commence toujours par ',' */
    char tt[16];
    tt[0] = ',';
    size_t n = strlen(typetags);
    if (n > sizeof(tt) - 2) n = sizeof(tt) - 2;
    memcpy(tt + 1, typetags, n);
    tt[n + 1] = '\0';
    return put_string(w, tt);
}

int osc_push_float(OscWriter *w, float v) {
    uint32_t bits;
    memcpy(&bits, &v, 4); /* réinterprétation bit-à-bit */
    return put_u32(w, bits);
}

int osc_push_int(OscWriter *w, int32_t v) {
    return put_u32(w, (uint32_t)v);
}

/* ---- Bundles ---- */

int osc_bundle_begin(OscWriter *w) {
    if (put_string(w, "#bundle") != 0) return -1;
    /* timetag OSC = 1 (immédiat) : 8 octets, valeur 0x0000000000000001 */
    if (put_u32(w, 0) != 0) return -1;
    if (put_u32(w, 1) != 0) return -1;
    return 0;
}

/* On stocke l'offset de la taille dans les 4 octets qu'on réserve, et on la
 * corrige à _end. Comme on ne peut pas conserver d'état imbriqué complexe ici,
 * on gère un seul niveau d'élément à la fois (suffisant pour notre usage). */
static size_t g_element_size_pos = 0;

int osc_bundle_element_begin(OscWriter *w) {
    g_element_size_pos = w->len;
    return put_u32(w, 0); /* placeholder taille */
}

int osc_bundle_element_end(OscWriter *w) {
    if (w->overflow) return -1;
    uint32_t size = (uint32_t)(w->len - g_element_size_pos - 4);
    put_u32be(w->buf + g_element_size_pos, size);
    return 0;
}
