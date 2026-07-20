#include "net.h"

#include <string.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/sysmodule.h>

/* Pool mémoire requis par sceNetInit (1 Mo suffit largement). */
#define NET_POOL_SIZE (256 * 1024)
static char        s_net_pool[NET_POOL_SIZE];
static int         s_net_ready = 0;

static int              s_sock = -1;
static SceNetSockaddrIn s_dest;
static int              s_dest_ready = 0;

int net_init(void) {
    int ret;

    ret = sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    if (ret < 0) return ret;

    SceNetInitParam initparam;
    initparam.memory = s_net_pool;
    initparam.size   = NET_POOL_SIZE;
    initparam.flags  = 0;

    ret = sceNetInit(&initparam);
    /* SCE_NET_ERROR_EBUSY (déjà init) est tolérable. */
    if (ret < 0 && ret != (int)0x80410108) return ret;

    ret = sceNetCtlInit();
    if (ret < 0 && ret != (int)0x80412102) { /* déjà init toléré */ }

    /* Socket UDP réutilisable pour tous les envois. */
    s_sock = sceNetSocket("vita2osc", SCE_NET_AF_INET, SCE_NET_SOCK_DGRAM, 0);
    if (s_sock < 0) return s_sock;

    s_net_ready = 1;
    return 0;
}

void net_finish(void) {
    if (s_sock >= 0) { sceNetSocketClose(s_sock); s_sock = -1; }
    if (s_net_ready) {
        sceNetCtlTerm();
        sceNetTerm();
        sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
        s_net_ready = 0;
    }
    s_dest_ready = 0;
}

int net_get_local_ip(char *out, size_t out_size) {
    if (!out || out_size < 16) return -1;
    SceNetCtlInfo info;
    int ret = sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info);
    if (ret < 0) { strncpy(out, "0.0.0.0", out_size); return ret; }
    strncpy(out, info.ip_address, out_size);
    out[out_size - 1] = '\0';
    return 0;
}

int net_set_target(const char *ip, uint16_t port) {
    if (!ip) return -1;
    memset(&s_dest, 0, sizeof(s_dest));
    s_dest.sin_family = SCE_NET_AF_INET;
    s_dest.sin_port   = sceNetHtons(port);
    int ret = sceNetInetPton(SCE_NET_AF_INET, ip, &s_dest.sin_addr);
    if (ret <= 0) { s_dest_ready = 0; return -1; }
    s_dest_ready = 1;
    return 0;
}

int net_target_ready(void) {
    return s_net_ready && s_dest_ready;
}

int net_send(const uint8_t *data, size_t len) {
    if (!net_target_ready() || s_sock < 0 || len == 0) return -1;
    return sceNetSendto(s_sock, data, len, 0,
                        (SceNetSockaddr *)&s_dest, sizeof(s_dest));
}
