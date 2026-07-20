cmake_minimum_required(VERSION 3.10)

## Toolchain VitaSDK — se configure aussi via -DCMAKE_TOOLCHAIN_FILE.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Definir la variable d'environnement VITASDK (voir README_BUILD.md)")
  endif()
endif()

project(Vita2OSC C)

include("${VITASDK}/share/vita.cmake" OPTIONAL)

set(VITA_APP_NAME  "Vita2OSC")
set(VITA_TITLEID   "VOSC00001")
set(VITA_VERSION   "02.01")

set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Wextra -O2)

add_executable(${PROJECT_NAME}
  src/main.c
  src/osc.c
  src/net.c
  src/input.c
  src/config.c
  src/config_json.c
  src/json.c
  src/storage.c
  src/httpd.c
  src/mapping.c
  src/theme.c
  src/ui.c
  src/colors.c
  src/settings_edit.c
  src/ime_input.c
  src/boot_log.c
)

target_link_libraries(${PROJECT_NAME}
  vita2d
  SceDisplay_stub
  SceGxm_stub
  SceCtrl_stub
  SceTouch_stub
  SceMotion_stub
  SceCommonDialog_stub
  SceIme_stub
  ScePower_stub
  SceNet_stub
  SceNetCtl_stub
  SceSysmodule_stub
  ScePgf_stub
  SceAppMgr_stub
  SceAppUtil_stub
  m
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})

## VPK avec icône et LiveArea pixel art (voir sce_sys/, générés par
## outputs/gen_pixelart.py).
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
