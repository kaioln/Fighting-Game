"""Renderiza um GLB de frente, perfil e costas (EEVEE, luz neutra). Uso: bl.sh --python view3.py -- <out_dir> <prefix> <glb>"""
import math
import os
import sys

import bpy
from mathutils import Vector

out, prefix, f = sys.argv[sys.argv.index("--") + 1:]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=f)
sc = bpy.context.scene
mn, mx = Vector((1e9,) * 3), Vector((-1e9,) * 3)
for o in bpy.data.objects:
    if o.type == "MESH":
        for c in o.bound_box:
            w = o.matrix_world @ Vector(c)
            mn, mx = Vector(map(min, mn, w)), Vector(map(max, mx, w))
ctr, size = (mn + mx) / 2, (mx - mn)
print("BBOX", tuple(round(v, 3) for v in size), "min", tuple(round(v, 3) for v in mn))
cam = bpy.data.objects.new("c", bpy.data.cameras.new("c"))
sc.collection.objects.link(cam)
sc.camera = cam
cam.data.type = "ORTHO"
cam.data.ortho_scale = max(size) * 1.15
w = bpy.data.worlds.new("w")
sc.world = w
w.color = (0.45, 0.45, 0.48)
sun = bpy.data.objects.new("s", bpy.data.lights.new("s", "SUN"))
sun.data.energy = 2.0
sun.rotation_euler = (math.radians(45), 0, math.radians(-30))
sc.collection.objects.link(sun)
sc.render.engine = "BLENDER_EEVEE_NEXT"
sc.render.resolution_x, sc.render.resolution_y = 480, 640
sc.view_settings.view_transform = "Standard"
os.makedirs(out, exist_ok=True)
for i, (tag, d) in enumerate((("frente", Vector((0, -1, 0))), ("perfil", Vector((1, 0, 0))), ("costas", Vector((0, 1, 0))))):
    cam.location = ctr + d * max(size) * 3
    cam.rotation_euler = (-d).to_track_quat("-Z", "Y").to_euler()
    sc.render.filepath = os.path.join(out, f"{prefix}_{i}.png")
    bpy.ops.render.render(write_still=True)
