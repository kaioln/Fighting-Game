"""Converte imagens para JPG, limitando a largura. Uso: bl.sh --python to_jpg.py -- <max_w> <out_dir> <img>..."""
import os
import sys

import bpy

args = sys.argv[sys.argv.index("--") + 1:]
max_w, out_dir, files = int(args[0]), args[1], args[2:]
os.makedirs(out_dir, exist_ok=True)
scene = bpy.context.scene
scene.render.image_settings.file_format = "JPEG"
scene.render.image_settings.quality = 88
scene.view_settings.view_transform = "Standard"
for f in files:
    img = bpy.data.images.load(f)
    w, h = img.size
    if w > max_w:
        img.scale(max_w, round(h * max_w / w))
    out = os.path.join(out_dir, os.path.splitext(os.path.basename(f))[0] + ".jpg")
    img.save_render(out, scene=scene)
    print("JPG", out, img.size[:])
