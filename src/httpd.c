#include "httpd.h"
#include "config_json.h"
#include "net.h"
#include "osc.h"
#include "json.h"
#include "version.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <psp2/net/net.h>
#include <psp2/kernel/threadmgr.h>

#define REQ_BUF   20480
#define RESP_BUF  20480
#define OSC_JSON_POOL 64

static Config    *s_cfg = NULL;
static Config     s_post_cfg;
static int        s_port = 8080;
static int        s_listen = -1;
static int        s_running = 0;
static SceUID     s_thread = -1;
static SceUID     s_mutex = -1;
static volatile int s_dirty = 0;

static const char *CORS =
    "Access-Control-Allow-Origin: *\r\n"
    "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
    "Access-Control-Allow-Headers: Content-Type\r\n";

void httpd_lock(void)   { if (s_mutex >= 0) sceKernelLockMutex(s_mutex, 1, NULL); }
void httpd_unlock(void) { if (s_mutex >= 0) sceKernelUnlockMutex(s_mutex, 1); }

int httpd_take_dirty(void) {
    int d = s_dirty;
    s_dirty = 0;
    return d;
}

static void send_all(int cs, const char *buf, int len) {
    int off = 0;
    while (off < len) {
        int r = sceNetSend(cs, buf + off, len - off, 0);
        if (r <= 0) break;
        off += r;
    }
}

static void send_response(int cs, const char *status, const char *ctype,
                          const char *body, int body_len) {
    static char hdr[512];
    int hl = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "%s"
        "\r\n",
        status, ctype, body_len, CORS);
    send_all(cs, hdr, hl);
    if (body && body_len > 0) send_all(cs, body, body_len);
}

/* Traite une requête déjà entièrement lue dans req (nul-terminée).
 * body pointe vers le corps (peut être NULL), body_len sa longueur. */
static void handle_request(int cs, char *req, char *body, int body_len) {
    static char resp[RESP_BUF];

    /* Méthode + chemin sur la première ligne. */
    char method[8] = {0}, path[64] = {0};
    sscanf(req, "%7s %63s", method, path);

    if (strcmp(method, "OPTIONS") == 0) {
        send_response(cs, "204 No Content", "text/plain", "", 0);
        return;
    }

    if (strcmp(path, "/config") == 0 && strcmp(method, "GET") == 0) {
        httpd_lock();
        int len = config_to_json(s_cfg, resp, sizeof(resp));
        httpd_unlock();
        if (len < 0) { send_response(cs, "500 Internal Server Error", "text/plain", "err", 3); return; }
        send_response(cs, "200 OK", "application/json", resp, len);
        return;
    }

    if (strcmp(path, "/config") == 0 && strcmp(method, "POST") == 0) {
        if (!body || body_len <= 0) {
            send_response(cs, "400 Bad Request", "text/plain", "no body", 7);
            return;
        }
        int rc = config_from_json(&s_post_cfg, body); /* body est mutable */
        if (rc != 0) {
            send_response(cs, "400 Bad Request", "application/json",
                          "{\"ok\":false}", 12);
            return;
        }
        httpd_lock();
        *s_cfg = s_post_cfg;
        s_dirty = 1;
        httpd_unlock();
        send_response(cs, "200 OK", "application/json", "{\"ok\":true}", 11);
        return;
    }

    /* Proxy OSC : POST {"address":"/foo","value":0.5} ou {"messages":[...]} */
    if (strcmp(path, "/osc") == 0 && strcmp(method, "POST") == 0) {
        if (!body || body_len <= 0) {
            send_response(cs, "400 Bad Request", "text/plain", "no body", 7);
            return;
        }
        static JsonNode pool[OSC_JSON_POOL];
        JsonNode *root = json_parse(body, pool, OSC_JSON_POOL);
        if (!root || root->type != JSON_OBJ) {
            send_response(cs, "400 Bad Request", "application/json",
                          "{\"ok\":false,\"error\":\"invalid json\"}", 35);
            return;
        }

        int sent = 0;
        JsonNode *msgs = json_obj_get(root, "messages");
        if (msgs && msgs->type == JSON_ARR) {
            for (JsonNode *e = msgs->child; e; e = e->next) {
                if (e->type != JSON_OBJ) continue;
                const char *addr = json_str(e, "address", NULL);
                if (!addr || !addr[0]) continue;
                float val = (float)json_num(e, "value", 0.0);
                const char *tt = json_str(e, "typeTag", "f");
                OscWriter w;
                osc_begin(&w);
                osc_message(&w, addr, tt);
                if (tt[0] == 'i') osc_push_int(&w, (int32_t)lroundf(val));
                else                osc_push_float(&w, val);
                if (!w.overflow && net_send(osc_data(&w), osc_size(&w)) >= 0) sent++;
            }
        } else {
            const char *addr = json_str(root, "address", NULL);
            if (!addr || !addr[0]) {
                send_response(cs, "400 Bad Request", "application/json",
                              "{\"ok\":false,\"error\":\"no address\"}", 33);
                return;
            }
            float val = (float)json_num(root, "value", 0.0);
            const char *tt = json_str(root, "typeTag", "f");
            OscWriter w;
            osc_begin(&w);
            osc_message(&w, addr, tt);
            if (tt[0] == 'i') osc_push_int(&w, (int32_t)lroundf(val));
            else                osc_push_float(&w, val);
            if (!w.overflow && net_send(osc_data(&w), osc_size(&w)) >= 0) sent = 1;
        }

        int len = snprintf(resp, sizeof(resp), "{\"ok\":true,\"sent\":%d}", sent);
        send_response(cs, sent > 0 ? "200 OK" : "502 Bad Gateway",
                      "application/json", resp, len);
        return;
    }

    if (strcmp(path, "/status") == 0 && strcmp(method, "GET") == 0) {
        char ip[16] = "0.0.0.0";
        net_get_local_ip(ip, sizeof(ip));
        httpd_lock();
        int cc = s_cfg->control_count;
        unsigned int port = s_cfg->target_port;
        char tip[16]; strncpy(tip, s_cfg->target_ip, sizeof(tip));
        httpd_unlock();
        int len = snprintf(resp, sizeof(resp),
            "{\"app\":\"Vita2OSC\",\"version\":\"" VITA2OSC_VERSION "\",\"ip\":\"%s\","
            "\"target\":{\"ip\":\"%s\",\"port\":%u},\"controls\":%d,"
            "\"oscReady\":%s}",
            ip, tip, port, cc, net_target_ready() ? "true" : "false");
        send_response(cs, "200 OK", "application/json", resp, len);
        return;
    }

    send_response(cs, "404 Not Found", "text/plain", "not found", 9);
}

/* Lit une requête HTTP complète (headers + éventuel corps selon Content-Length). */
static void serve_client(int cs) {
    static char req[REQ_BUF];
    int total = 0;
    int header_end = -1;

    /* Timeouts : un client lent/mort ne doit pas bloquer le serveur. */
    int tmo = 3 * 1000 * 1000; /* 3 s (microsecondes) */
    sceNetSetsockopt(cs, SCE_NET_SOL_SOCKET, SCE_NET_SO_RCVTIMEO, &tmo, sizeof(tmo));
    sceNetSetsockopt(cs, SCE_NET_SOL_SOCKET, SCE_NET_SO_SNDTIMEO, &tmo, sizeof(tmo));

    /* Lire jusqu'à la fin des headers. */
    while (total < REQ_BUF - 1) {
        int r = sceNetRecv(cs, req + total, REQ_BUF - 1 - total, 0);
        if (r <= 0) break;
        total += r;
        req[total] = '\0';
        char *he = strstr(req, "\r\n\r\n");
        if (he) { header_end = (int)(he - req) + 4; break; }
    }
    if (header_end < 0) return;

    /* Content-Length ? (recherche insensible à la casse simplifiée) */
    int content_len = 0;
    char *cl = strstr(req, "Content-Length:");
    if (!cl) cl = strstr(req, "content-length:");
    if (cl) content_len = atoi(cl + 15);

    /* Corps trop gros pour le buffer -> 413, on ne lit pas la suite. */
    if (content_len < 0 || content_len > REQ_BUF - header_end - 1) {
        send_response(cs, "413 Payload Too Large", "text/plain", "too large", 9);
        return;
    }

    int body_have = total - header_end;
    /* Lire le reste du corps si nécessaire. */
    while (body_have < content_len && total < REQ_BUF - 1) {
        int r = sceNetRecv(cs, req + total, REQ_BUF - 1 - total, 0);
        if (r <= 0) break;
        total += r;
        body_have = total - header_end;
    }
    req[total] = '\0';

    char *body = content_len > 0 ? (req + header_end) : NULL;
    if (body) body[content_len < (total - header_end) ? content_len : (total - header_end)] = '\0';

    handle_request(cs, req, body, content_len);
}

static int server_thread(SceSize args, void *argp) {
    (void)args; (void)argp;

    s_listen = sceNetSocket("vita2osc_httpd", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);
    if (s_listen < 0) return -1;

    int on = 1;
    sceNetSetsockopt(s_listen, SCE_NET_SOL_SOCKET, SCE_NET_SO_REUSEADDR, &on, sizeof(on));

    SceNetSockaddrIn addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = SCE_NET_AF_INET;
    addr.sin_port   = sceNetHtons((uint16_t)s_port);
    addr.sin_addr.s_addr = 0; /* INADDR_ANY */

    if (sceNetBind(s_listen, (SceNetSockaddr *)&addr, sizeof(addr)) < 0) {
        sceNetSocketClose(s_listen); s_listen = -1; return -1;
    }
    if (sceNetListen(s_listen, 2) < 0) {
        sceNetSocketClose(s_listen); s_listen = -1; return -1;
    }

    while (s_running) {
        SceNetSockaddrIn caddr;
        unsigned int clen = sizeof(caddr);
        int cs = sceNetAccept(s_listen, (SceNetSockaddr *)&caddr, &clen);
        if (cs < 0) break; /* socket fermé par httpd_stop */
        serve_client(cs);
        sceNetSocketClose(cs);
    }

    if (s_listen >= 0) { sceNetSocketClose(s_listen); s_listen = -1; }
    return 0;
}

int httpd_start(Config *shared_cfg, int port) {
    if (!shared_cfg) return -1;
    s_cfg = shared_cfg;
    s_port = port > 0 ? port : 8080;

    s_mutex = sceKernelCreateMutex("vita2osc_cfg", 0, 0, NULL);
    if (s_mutex < 0) return -1;

    s_running = 1;
    s_thread = sceKernelCreateThread("vita2osc_httpd", server_thread,
                                     0x10000100, 0x20000, 0, 0, NULL);
    if (s_thread < 0) { s_running = 0; return -1; }
    sceKernelStartThread(s_thread, 0, NULL);
    return 0;
}

void httpd_stop(void) {
    s_running = 0;
    if (s_listen >= 0) { sceNetSocketClose(s_listen); s_listen = -1; }
    if (s_thread >= 0) {
        sceKernelWaitThreadEnd(s_thread, NULL, NULL);
        sceKernelDeleteThread(s_thread);
        s_thread = -1;
    }
    if (s_mutex >= 0) { sceKernelDeleteMutex(s_mutex); s_mutex = -1; }
}
