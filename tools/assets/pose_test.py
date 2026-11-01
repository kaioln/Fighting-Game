"""Aplica clipes de um GLB de animação num personagem e renderiza quadros (3/4 e perfil).
Uso: bl.sh --python pose_test.py -- <char.glb> <anims.glb> <out_dir> <row> clip:frame [clip:frame ...]"""
import math
import os
import sys

import bpy
from mathutils import Vector

args = sys.argv[sys.argv.index("--") + 1:]
CH, AN, OUT, ROW = args[:4]
shots = [a.split(":") for a in args[4:]]
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.render.fps = 60
bpy.ops.import_scene.gltf(filepath=CH)
arm = next(o for o in bpy.data.objects if o.type == "ARMATURE")
before = set(bpy.data.objects)
bpy.ops.import_scene.gltf(filepath=AN)
for o in [o for o in bpy.data.objects if o not in before]:
    bpy.data.objects.remove(o, do_unlink=True)
cam = bpy.data.objects.new("c", bpy.data.cameras.new("c"))
sc.collection.objects.link(cam)
sc.camera = cam
cam.data.lens = 45
w = bpy.data.worlds.new("w")
sc.world = w
w.color = (0.5, 0.5, 0.52)
sun = bpy.data.objects.new("s", bpy.data.lights.new("s", "SUN"))
sun.data.energy = 3.0
sun.rotation_euler = (math.radians(50), 0, math.radians(-40))
sc.collection.objects.link(sun)
sc.render.engine = "BLENDER_EEVEE_NEXT"
sc.render.resolution_x, sc.render.resolution_y = 360, 460
sc.view_settings.view_transform = "Standard"
os.makedirs(OUT, exist_ok=True)
arm.animation_data_create()
col = 0
for clip, fr in shots:
    if clip == "rest":
        arm.animation_data.action = None
        for pb in arm.pose.bones:
            pb.rotation_quaternion = (1, 0, 0, 0)
    else:
        act = next(a for a in bpy.data.actions if a.name.startswith(clip))
        arm.animation_data.action = act
        if act.slots:
            arm.animation_data.action_slot = act.slots[0]
        sc.frame_set(int(act.frame_range[0]) + int(fr))
    bpy.context.view_layer.update()
    for view, d in (("34", Vector((0.7, -1.0, 0.15))), ("perfil", Vector((1.0, 0.0, 0.05)))):
        d = d.normalized() * 4.2
        cam.location = Vector((0, 0, 1.0)) + d
        cam.rotation_euler = (-d).to_track_quat("-Z", "Y").to_euler()
        sc.render.filepath = os.path.join(OUT, f"row{ROW}_{col}.png")
        bpy.ops.render.render(write_still=True)
        col += 1
print("DONE", col)
