#include "boot_log.h"

#include <string.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>

#define BOOT_LOG_DIR  "ux0:data/vita2osc"
#define BOOT_LOG_PATH "ux0:data/vita2osc/boot.log"

void boot_log(const char *msg) {
    if (!msg) return;
    sceIoMkdir(BOOT_LOG_DIR, 0777);
    SceUID fd = sceIoOpen(BOOT_LOG_PATH,
                          SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
    if (fd < 0) return;
    sceIoWrite(fd, msg, (SceSize)strlen(msg));
    sceIoWrite(fd, "\n", 1);
    sceIoClose(fd);
}
