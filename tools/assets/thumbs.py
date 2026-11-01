"""Miniatura (Workbench, cor do material) de cada GLB. Uso: bl.sh --python thumbs.py -- <out_dir> <glb>..."""
import math
import os
import sys

import bpy
from mathutils import Vector

args = sys.argv[sys.argv.index("--") + 1:]
out, files = args[0], args[1:]
os.makedirs(out, exist_ok=True)
for i, f in enumerate(files):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=f)
    mn, mx = Vector((1e9,) * 3), Vector((-1e9,) * 3)
    for o in bpy.data.objects:
        if o.type == "MESH":
            for c in o.bound_box:
                w = o.matrix_world @ Vector(c)
                mn, mx = Vector(map(min, mn, w)), Vector(map(max, mx, w))
    ctr, size = (mn + mx) / 2, max((mx - mn).length, 1e-3)
    sc = bpy.context.scene
    cam = bpy.data.objects.new("c", bpy.data.cameras.new("c"))
    sc.collection.objects.link(cam)
    sc.camera = cam
    d = Vector((1.0, -1.4, 0.7)).normalized() * size * 1.5
    cam.location = ctr + d
    cam.rotation_euler = (-d).to_track_quat("-Z", "Y").to_euler()
    sc.render.engine = "BLENDER_WORKBENCH"
    sc.display.shading.color_type = "TEXTURE"
    sc.render.resolution_x = sc.render.resolution_y = 300
    sc.render.filepath = os.path.join(out, f"row{i // 5}_{i % 5}.png")
    bpy.ops.render.render(write_still=True)
    print("THUMB", i, os.path.basename(f), tuple(round(v, 2) for v in (mx - mn)))
