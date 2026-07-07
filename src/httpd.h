/*
 * httpd.h — Serveur HTTP embarqué (thread dédié) pour la config à distance.
 *
 * Endpoints :
 *   GET  /config  -> renvoie la config courante en JSON
 *   POST /config  -> reçoit une nouvelle config JSON, l'applique à chaud
 *   GET  /status  -> IP, version, nb de contrôles
 *   OPTIONS *     -> préflight CORS
 *
 * La config est partagée avec le thread principal et protégée par un mutex.
 */
#ifndef VITA2OSC_HTTPD_H
#define VITA2OSC_HTTPD_H

#include "config.h"

/* Démarre le serveur sur `port`, partageant `shared_cfg` avec le thread
 * principal. Retourne 0 si OK. */
int  httpd_start(Config *shared_cfg, int port);
void httpd_stop(void);

/* Verrou à utiliser par le thread principal autour de toute lecture/écriture
 * de la config partagée (ex. pendant mapping_tick). */
void httpd_lock(void);
void httpd_unlock(void);

/* 1 (et remet à 0) si la config a été modifiée via POST depuis le dernier appel.
 * Le thread principal réagit alors : reset mapping, MAJ cible réseau, sauvegarde. */
int  httpd_take_dirty(void);

#endif /* VITA2OSC_HTTPD_H */
