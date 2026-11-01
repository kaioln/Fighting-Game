"""HDRI equirretangular -> PNG 2048x1024 tonemapeado (AgX). Uso: bl.sh --python sky_png.py -- <in.hdr> <out.png>"""
import sys

import bpy

src, out = sys.argv[sys.argv.index("--") + 1:]
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.view_settings.view_transform = "AgX"
scene.view_settings.look = "AgX - Base Contrast"
scene.view_settings.exposure = 0.0
scene.render.image_settings.file_format = "PNG"
scene.render.image_settings.color_mode = "RGB"
scene.render.image_settings.color_depth = "8"
img = bpy.data.images.load(src)
img.scale(2048, 1024)
img.save_render(out, scene=scene)
print("SKY", out, img.size[:])
