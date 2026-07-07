/*
 * net.h — Initialisation réseau Vita (SceNet/SceNetCtl) et socket UDP OSC.
 */
#ifndef VITA2OSC_NET_H
#define VITA2OSC_NET_H

#include <stdint.h>
#include <stddef.h>

/* Initialise la pile réseau. À appeler une fois au démarrage.
 * Retourne 0 si OK, <0 sinon. */
int  net_init(void);
void net_finish(void);

/* Récupère l'IP locale de la Vita (ex. "192.168.1.50") dans out (taille >= 16).
 * Utile pour l'afficher à l'écran. Retourne 0 si OK. */
int  net_get_local_ip(char *out, size_t out_size);

/* (Re)configure la destination OSC. À rappeler si l'IP/port changent. */
int  net_set_target(const char *ip, uint16_t port);

/* Envoie un datagramme UDP vers la destination courante. Retourne octets envoyés. */
int  net_send(const uint8_t *data, size_t len);

/* Indique si une cible valide est configurée. */
int  net_target_ready(void);

#endif /* VITA2OSC_NET_H */
