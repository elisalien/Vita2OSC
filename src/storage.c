#include "storage.h"
#include "config_json.h"

#include <string.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>

#define STORAGE_BUF 24576

int storage_save_config(const Config *cfg) {
    static char buf[STORAGE_BUF];
    int len = config_to_json(cfg, buf, sizeof(buf));
    if (len < 0) return -1;

    sceIoMkdir(STORAGE_DIR, 0777); /* no-op si déjà présent */

    SceUID fd = sceIoOpen(STORAGE_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd < 0) return (int)fd;

    int written = 0;
    while (written < len) {
        int r = sceIoWrite(fd, buf + written, len - written);
        if (r <= 0) { sceIoClose(fd); return -2; }
        written += r;
    }
    sceIoClose(fd);
    return 0;
}

int storage_load_config(Config *cfg) {
    static char buf[STORAGE_BUF];

    SceUID fd = sceIoOpen(STORAGE_PATH, SCE_O_RDONLY, 0);
    if (fd < 0) return (int)fd; /* fichier absent */

    int total = 0, r;
    while (total < (int)sizeof(buf) - 1 &&
           (r = sceIoRead(fd, buf + total, sizeof(buf) - 1 - total)) > 0) {
        total += r;
    }
    sceIoClose(fd);
    if (total <= 0) return -1;
    buf[total] = '\0';

    return config_from_json(cfg, buf); /* buf est mutable : OK pour le parseur */
}
