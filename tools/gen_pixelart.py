#!/usr/bin/env python3
"""
Vita2OSC — génération des assets pixel art :
  - sce_sys/icon0.png                  (128x128, PNG palette)
  - sce_sys/livearea/contents/bg.png   (840x500)
  - sce_sys/livearea/contents/startup.png (280x158)
  - src/pixelart_data.h                (logo 32x32 embarqué pour le splash in-app)
Style : mini PS Vita kawaii + ondes OSC, palette bleu nuit.
"""
from PIL import Image
import os

OUT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))

# ---------------------------------------------------------------- palette
# index 0 = transparent
PAL = {
    0:  (0, 0, 0, 0),          # transparent
    1:  (0x0d, 0x12, 0x20, 255),  # fond nuit
    2:  (0x05, 0x07, 0x0c, 255),  # contour sombre
    3:  (0x1b, 0x22, 0x32, 255),  # corps console
    4:  (0x27, 0x30, 0x46, 255),  # corps highlight
    5:  (0x4d, 0xa3, 0xff, 255),  # bleu accent (écran / onde)
    6:  (0xa8, 0xd4, 0xff, 255),  # bleu clair (shine / onde interne)
    7:  (0x8b, 0x5c, 0xf6, 255),  # violet (onde externe)
    8:  (0xf2, 0xf5, 0xfa, 255),  # blanc
    9:  (0x8d, 0x97, 0xac, 255),  # gris (d-pad)
    10: (0x39, 0x41, 0x5c, 255),  # sticks
    11: (0x35, 0xd3, 0x9a, 255),  # vert (triangle)
    12: (0xff, 0x6b, 0x81, 255),  # rose-rouge (rond)
    13: (0x7a, 0xa8, 0xff, 255),  # bleu (croix)
    14: (0xe0, 0x8c, 0xff, 255),  # rose-violet (carré)
    15: (0xff, 0x9d, 0xb4, 255),  # joues roses
    16: (0x2f, 0x7f, 0xe0, 255),  # bleu écran ombre
    17: (0x14, 0x1b, 0x2e, 255),  # fond nuit clair (ambiance)
    18: (0x23, 0x2c, 0x44, 255),  # bord icône
}

W = H = 32

def new_grid(fill=0):
    return [[fill] * W for _ in range(H)]

def put(g, x, y, c):
    if 0 <= x < len(g[0]) and 0 <= y < len(g):
        g[y][x] = c

def rect(g, x0, y0, x1, y1, c):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            put(g, x, y, c)

def rounded_rect(g, x0, y0, x1, y1, r, c):
    rect(g, x0 + r, y0, x1 - r, y1, c)
    rect(g, x0, y0 + r, x1, y1 - r, c)
    # coins arrondis (quart de disque)
    for dy in range(r + 1):
        for dx in range(r + 1):
            if dx * dx + dy * dy <= (r + 0.4) ** 2:
                put(g, x0 + r - dx, y0 + r - dy, c)
                put(g, x1 - r + dx, y0 + r - dy, c)
                put(g, x0 + r - dx, y1 - r + dy, c)
                put(g, x1 - r + dx, y1 - r + dy, c)

def arc_up(g, cx, cy, radius, c, span=0.95):
    """Arc de cercle vers le haut (ondes wi-fi), span en fraction de 90° de chaque côté."""
    import math
    steps = radius * 14
    for i in range(steps + 1):
        a = -math.pi / 2 + (i / steps - 0.5) * math.pi * span
        x = int(round(cx + radius * math.cos(a + math.pi / 2) * 0))  # placeholder
    # version simple : paramétrer l'angle autour de la verticale
    for i in range(steps + 1):
        t = (i / steps) * 2 - 1              # -1..1
        a = t * (math.pi / 2) * span          # angle depuis la verticale
        x = cx + radius * math.sin(a)
        y = cy - radius * math.cos(a)
        put(g, int(round(x)), int(round(y)), c)

def sparkle(g, x, y, c):
    put(g, x, y, c)
    put(g, x - 1, y, c); put(g, x + 1, y, c)
    put(g, x, y - 1, c); put(g, x, y + 1, c)

# ---------------------------------------------------------------- logo 32x32
def build_logo(with_bg):
    g = new_grid(0)
    if with_bg:
        rounded_rect(g, 0, 0, 31, 31, 6, 18)   # bord
        rounded_rect(g, 1, 1, 30, 30, 5, 1)    # fond nuit

    # --- ondes OSC (au-dessus de la console) ---
    arc_up(g, 15, 19, 10, 7, span=0.72)
    arc_up(g, 15, 19, 7, 5, span=0.78)
    arc_up(g, 15, 19, 4, 6, span=0.85)
    put(g, 15, 17, 8); put(g, 16, 17, 8)       # point émetteur

    # --- corps console (contour + fill) ---
    rounded_rect(g, 3, 19, 28, 28, 3, 2)       # contour
    rounded_rect(g, 4, 20, 27, 27, 2, 3)       # corps
    for x in range(6, 26):                     # highlight haut
        put(g, x, 20, 4)

    # --- écran ---
    rect(g, 10, 21, 21, 26, 5)
    put(g, 10, 21, 6); put(g, 11, 21, 6)       # shine
    for x in range(10, 22):
        put(g, x, 26, 16)                      # ombre bas d'écran

    # --- petit visage kawaii sur l'écran ---
    put(g, 13, 22, 2); put(g, 13, 23, 2)       # oeil gauche
    put(g, 18, 22, 2); put(g, 18, 23, 2)       # oeil droit
    put(g, 14, 24, 2); put(g, 15, 25, 2)       # sourire
    put(g, 16, 25, 2); put(g, 17, 24, 2)
    put(g, 12, 24, 15); put(g, 19, 24, 15)     # joues roses

    # --- d-pad (gauche) ---
    put(g, 6, 21, 9); put(g, 6, 22, 9); put(g, 6, 23, 9)
    put(g, 5, 22, 9); put(g, 7, 22, 9)

    # --- boutons (droite, losange coloré) ---
    put(g, 25, 21, 11)   # triangle (haut)
    put(g, 26, 22, 12)   # rond (droite)
    put(g, 25, 23, 13)   # croix (bas)
    put(g, 24, 22, 14)   # carré (gauche)

    # --- sticks ---
    rect(g, 6, 25, 7, 26, 10); put(g, 6, 25, 9)
    rect(g, 24, 25, 25, 26, 10); put(g, 24, 25, 9)

    # --- sparkles ---
    if with_bg:
        sparkle(g, 5, 7, 5)
        put(g, 27, 5, 7)
        put(g, 8, 3, 6)
        put(g, 26, 12, 8)
    return g

# ---------------------------------------------------------------- rendu PNG
def grid_to_image(g, scale):
    h = len(g); w = len(g[0])
    img = Image.new("RGBA", (w, h))
    for y in range(h):
        for x in range(w):
            img.putpixel((x, y), PAL[g[y][x]])
    return img.resize((w * scale, h * scale), Image.NEAREST)

def save_indexed(img, path, keep_alpha):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    if keep_alpha:
        # PNG palette avec transparence (exigé léger par LiveArea)
        alpha = img.split()[3]
        p = img.convert("RGB").convert("P", palette=Image.ADAPTIVE, colors=255)
        mask = alpha.point(lambda a: 255 if a < 128 else 0)
        p.paste(255, mask)  # index 255 réservé transparent
        p.info["transparency"] = 255
        p.save(path, optimize=True, transparency=255)
    else:
        img.convert("RGB").convert("P", palette=Image.ADAPTIVE, colors=256)\
           .save(path, optimize=True)
    print("ok:", path, os.path.getsize(path), "octets")

# ---------------------------------------------------------------- mini police 3x5
FONT = {
    'A': ["010","101","111","101","101"],
    'C': ["011","100","100","100","011"],
    'E': ["111","100","110","100","111"],
    'F': ["111","100","110","100","100"],
    'G': ["011","100","101","101","011"],
    'H': ["101","101","111","101","101"],
    'I': ["111","010","010","010","111"],
    'L': ["100","100","100","100","111"],
    'N': ["101","111","111","111","101"],
    'O': ["010","101","101","101","010"],
    'R': ["110","101","110","101","101"],
    'S': ["011","100","010","001","110"],
    'T': ["111","010","010","010","010"],
    'U': ["101","101","101","101","111"],
    'V': ["101","101","101","101","010"],
    '2': ["110","001","010","100","111"],
    '-': ["000","000","111","000","000"],
    ' ': ["000","000","000","000","000"],
}

def draw_text(g, x, y, s, c):
    cx = x
    for ch in s:
        gl = FONT.get(ch.upper())
        if gl:
            for gy, row in enumerate(gl):
                for gx, bit in enumerate(row):
                    if bit == '1':
                        put(g, cx + gx, y + gy, c)
        cx += 4
    return cx - x - 1  # largeur

def text_w(s):
    return len(s) * 4 - 1

# ---------------------------------------------------------------- icon0 128x128
logo_bg = build_logo(with_bg=True)
save_indexed(grid_to_image(logo_bg, 4), f"{OUT}/sce_sys/icon0.png", keep_alpha=True)

# ---------------------------------------------------------------- startup 280x158
# grille 70x39 (scale 4) -> 280x156, on colle sur un canevas 280x158
sw, sh = 70, 39
g = [[1] * sw for _ in range(sh)]
logo = build_logo(with_bg=False)
# coller le logo (32x32) centré horizontalement, y=1
ox = (sw - 32) // 2
for y in range(32):
    for x in range(32):
        c = logo[y][x]
        if c:
            g[y + 1][x + ox] = c
tw = text_w("VITA2OSC")
draw_text(g, (sw - tw) // 2, 33, "VITA2OSC", 8)
img = Image.new("RGBA", (280, 158), PAL[1])
img.paste(grid_to_image(g, 4), (0, 1))
save_indexed(img, f"{OUT}/sce_sys/livearea/contents/startup.png", keep_alpha=False)

# ---------------------------------------------------------------- bg 840x500
# grille 168x100 (scale 5)
bw, bh = 168, 100
g = [[1] * bw for _ in range(bh)]
# ambiance : larges disques discrets
def soft_disc(g, cx, cy, r, c):
    for y in range(max(0, cy - r), min(bh, cy + r + 1)):
        for x in range(max(0, cx - r), min(bw, cx + r + 1)):
            if (x - cx) ** 2 + (y - cy) ** 2 <= r * r:
                g[y][x] = c
soft_disc(g, 20, 10, 34, 17)
soft_disc(g, 150, 92, 40, 17)
# étoiles
import random
random.seed(7)
for _ in range(28):
    x, y = random.randrange(2, bw - 2), random.randrange(2, bh - 2)
    g[y][x] = random.choice([9, 5, 7, 6])
sparkle(g, 30, 70, 5); sparkle(g, 140, 18, 7); sparkle(g, 100, 8, 6)

# logo x2 (64x64) à gauche
logo = build_logo(with_bg=False)
ox, oy = 18, 22
for y in range(32):
    for x in range(32):
        c = logo[y][x]
        if c:
            for dy in range(2):
                for dx in range(2):
                    put(g, ox + x * 2 + dx, oy + y * 2 + dy, c)

# titre + sous-titre à droite
tx = 92
draw_text(g, tx, 34, "VITA", 8)
draw_text(g, tx + text_w("VITA") + 2, 34, "2OSC", 5)
draw_text(g, tx, 44, "CONTROLEUR OSC", 9)
draw_text(g, tx, 52, "SANS-FIL", 9)
draw_text(g, tx, 64, "L R  NAVIGUER", 10)
draw_text(g, tx, 71, "TOUCH  REGLER", 10)

img = grid_to_image(g, 5)
save_indexed(img, f"{OUT}/sce_sys/livearea/contents/bg.png", keep_alpha=False)

# ---------------------------------------------------------------- template.xml
tpl = """<?xml version="1.0" encoding="utf-8"?>
<livearea style="a1" format-ver="01.00" content-rev="1">
  <livearea-background>
    <image>bg.png</image>
  </livearea-background>
  <gate>
    <startup-image>startup.png</startup-image>
  </gate>
</livearea>
"""
with open(f"{OUT}/sce_sys/livearea/contents/template.xml", "w") as f:
    f.write(tpl)
print("ok: template.xml")

# ---------------------------------------------------------------- pixelart_data.h
logo = build_logo(with_bg=False)
lines = []
lines.append("/* pixelart_data.h — logo pixel art 32x32 généré par gen_pixelart.py.")
lines.append(" * Ne pas éditer à la main : regénérer avec le script. */")
lines.append("#ifndef VITA2OSC_PIXELART_DATA_H")
lines.append("#define VITA2OSC_PIXELART_DATA_H")
lines.append("")
lines.append("#define PX_LOGO_W 32")
lines.append("#define PX_LOGO_H 32")
lines.append("#define PX_PAL_COUNT %d" % (max(PAL) + 1))
lines.append("")
lines.append("/* RGBA (0 = transparent) */")
pal_entries = []
for i in range(max(PAL) + 1):
    r, gg, b, a = PAL.get(i, (0, 0, 0, 0))
    pal_entries.append("0x%02X%02X%02X%02Xu" % (a, b, gg, r))  # ABGR pour RGBA8
lines.append("static const unsigned int PX_PALETTE[PX_PAL_COUNT] = {")
for i in range(0, len(pal_entries), 6):
    lines.append("    " + ", ".join(pal_entries[i:i+6]) + ",")
lines.append("};")
lines.append("")
lines.append("static const unsigned char PX_LOGO[PX_LOGO_H][PX_LOGO_W] = {")
for row in logo:
    lines.append("    {" + ",".join(str(v) for v in row) + "},")
lines.append("};")
lines.append("")
lines.append("#endif /* VITA2OSC_PIXELART_DATA_H */")
with open(f"{OUT}/src/pixelart_data.h", "w") as f:
    f.write("\n".join(lines) + "\n")
print("ok: pixelart_data.h")
