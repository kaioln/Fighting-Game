"""Acha quadro de contato e trecho útil (from/to) de clipes, na base de 60 quadros/s (como a raylib amostra glTF).

Contato = quadro de maior alcance para a frente do efetor (mão ou pé) em relação ao quadril.
from/to = começo e fim do movimento do efetor (alcance acima de 15% da amplitude), com folga de 4 quadros.
Uso: bl.sh --python contact_frames.py -- <out.json> <prev_dir> <glb> [<glb> ...]
"""
import json
import math
import os
import sys

import bpy
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
OUT, PREV, files = argv[0], argv[1], argv[2:]

CLIPS = {  # clipe: efetores
    "Punch_Jab": ("hand_l", "hand_r"), "Punch_Cross": ("hand_l", "hand_r"), "Melee_Hook": ("hand_l", "hand_r"),
    "Melee_Hook_Rec": ("hand_l", "hand_r"),
    "OverhandThrow": ("hand_l", "hand_r"), "Shield_Dash": ("hand_l", "hand_r"), "Shield_OneShot": ("hand_l", "hand_r"),
    "Sword_Heavy_Combo": ("hand_l", "hand_r"), "Sword_Regular_A": ("hand_l", "hand_r"),
    "Sword_Regular_B": ("hand_l", "hand_r"), "Sword_Regular_C": ("hand_l", "hand_r"),
    "Spell_Simple_Shoot": ("hand_l", "hand_r"), "Zombie_Scratch": ("hand_l", "hand_r"),
    "NinjaJump_Start": ("hand_l", "hand_r", "foot_l", "foot_r"),
    "Slide_Start": ("foot_l", "foot_r"), "Roll": ("foot_l", "foot_r"),
    "CMU_Kick_74_03": ("foot_l", "foot_r"), "CMU_Kick_74_05": ("foot_l", "foot_r"),
    "CMU_Kick_74_06": ("foot_l", "foot_r"), "CMU_JumpKick_75_16": ("foot_l", "foot_r"),
    "CMU_JumpSpinKick_87_01": ("foot_l", "foot_r"), "CMU_JumpSpinKick_88_06": ("foot_l", "foot_r"),
    "CMU_Boxing_13_18": ("hand_l", "hand_r"),
}

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.render.fps = 60
result = {}
os.makedirs(PREV, exist_ok=True)
cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
scene.collection.objects.link(cam)
scene.camera = cam
scene.render.engine = "BLENDER_WORKBENCH"
scene.render.resolution_x, scene.render.resolution_y = 300, 300

for f in files:
    for o in list(bpy.data.objects):
        if o is not cam:
            bpy.data.objects.remove(o, do_unlink=True)
    for a in list(bpy.data.actions):
        bpy.data.actions.remove(a)
    bpy.ops.import_scene.gltf(filepath=f)
    arm = [o for o in bpy.data.objects if o.type == "ARMATURE"][0]
    ad = arm.animation_data
    for t in ad.nla_tracks:
        t.mute = True
    for act in list(bpy.data.actions):
        name = act.name.split("_Armature")[0] if "_Armature" in act.name else act.name
        name = name.rsplit(".", 1)[0] if name.rsplit(".", 1)[-1].isdigit() else name
        if name not in CLIPS:
            continue
        ad.action = act
        if hasattr(ad, "action_slot") and act.slots:
            ad.action_slot = act.slots[0]
        f0, f1 = (int(round(v)) for v in act.frame_range)
        n = f1 - f0 + 1
        reach = []
        for i in range(n):
            scene.frame_set(f0 + i)
            mw = arm.matrix_world
            pel = mw @ arm.pose.bones["pelvis"].head
            best = -1e9
            for e in CLIPS[name]:
                p = mw @ arm.pose.bones[e].head
                d = Vector((p.x - pel.x, p.y - pel.y, 0)).length * (1 if (pel.y - p.y) > 0 else -1)
                best = max(best, -(p.y - pel.y))  # frente do personagem = -Y no Blender
            reach.append(best)
        lo, hi = min(reach), max(reach)
        contact = max(range(n), key=lambda i: reach[i])
        th = lo + 0.15 * (hi - lo)
        start = contact
        while start > 0 and reach[start - 1] > th:
            start -= 1
        end = contact
        while end < n - 1 and reach[end + 1] > th:
            end += 1
        frm = max(0, start - 4)
        to = min(n - 1, end + 4)
        result[name] = {"frames": n, "contact": contact, "from": frm, "to": to,
                        "reach_m": round(hi, 3), "base_m": round(reach[0], 3)}
        # miniatura do quadro de contato (vista lateral)
        scene.frame_set(f0 + contact)
        pel = arm.matrix_world @ arm.pose.bones["pelvis"].head
        cam.location = (pel.x + 4.0, pel.y, 1.1)
        cam.rotation_euler = (math.radians(90), 0, math.radians(90))
        scene.render.filepath = os.path.join(PREV, f"{name}_0.png")
        bpy.ops.render.render(write_still=True)
        print("CLIP", name, result[name])
    ad.action = None

json.dump(result, open(OUT, "w"), indent=1)
print("DONE", len(result))
