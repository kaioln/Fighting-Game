"""Animação de golpes à mão (poses-chave com IK), assada em keyframes no esqueleto Quaternius de 65 ossos.

Cada golpe é uma lista de poses-chave (quadro a 60/s): posição das mãos e pés (alvos de IK), altura e giro do
quadril, giro do tronco. O Blender resolve braços e pernas, e o resultado é assado em rotações dos ossos.
Frente do personagem = -Y. Base de lutador canhoto-ortodoxo: pé e mão esquerdos (+X) à frente.

Uso: bl.sh --python keyanim.py -- <ual.glb> <out.glb> <prev_dir>
"""
import math
import os
import sys

import bpy
from mathutils import Euler, Vector

UAL, OUT, PREV = sys.argv[sys.argv.index("--") + 1:]
FPS = 60

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.render.fps = FPS
bpy.ops.import_scene.gltf(filepath=UAL)
arm = next(o for o in bpy.data.objects if o.type == "ARMATURE")
if arm.animation_data:
    arm.animation_data.action = None
    for t in list(arm.animation_data.nla_tracks):
        arm.animation_data.nla_tracks.remove(t)
for a in list(bpy.data.actions):
    bpy.data.actions.remove(a)

B = {b.name: arm.matrix_world @ b.head_local for b in arm.data.bones}
PELVIS0 = B["pelvis"].copy()
SH_Z = B["upperarm_l"].z


def empty(name, loc):
    e = bpy.data.objects.new(name, None)
    e.empty_display_size = 0.05
    scene.collection.objects.link(e)
    e.location = loc
    return e


T = {k: empty("ik_" + k, (0, 0, 0)) for k in ("hand_l", "hand_r", "foot_l", "foot_r")}
P = {k: empty("pole_" + k, (0, 0, 0)) for k in ("hand_l", "hand_r", "foot_l", "foot_r")}
CHAIN = {"hand_l": "lowerarm_l", "hand_r": "lowerarm_r", "foot_l": "calf_l", "foot_r": "calf_r"}
for k, bone in CHAIN.items():
    c = arm.pose.bones[bone].constraints.new("IK")
    c.target = T[k]
    c.pole_target = P[k]
    c.pole_angle = math.radians(-90)
    c.chain_count = 2
# alvos dos pés guardam a orientação do pé no chão
for side in ("l", "r"):
    c = arm.pose.bones["foot_" + side].constraints.new("COPY_ROTATION")
    c.target = T["foot_" + side]
    c.mix_mode = "REPLACE"

# ---------------------------------------------------------------- poses
SX = B["upperarm_l"].x  # meia largura dos ombros


def guard(**over):
    """Base de luta: base lateral, joelhos dobrados, punhos na altura do queixo."""
    p = dict(
        pelvis_dz=-0.07, pelvis_yaw=28.0, spine_yaw=-18.0, spine_pitch=6.0, head_yaw=-8.0,
        hand_l=(0.10, -0.30, SH_Z + 0.06), hand_r=(-0.06, -0.20, SH_Z + 0.10),
        foot_l=(0.10, -0.22, 0.10), foot_r=(-0.13, 0.22, 0.10), foot_l_yaw=20.0, foot_r_yaw=55.0,
        foot_r_pitch=0.0,
    )
    p.update(over)
    return p


def pose_at(frame, p):
    scene.frame_set(frame)
    pb = arm.pose.bones
    pel = pb["pelvis"]
    pel.location = (0, 0, 0)
    pel.rotation_mode = "XYZ"
    # deslocamento vertical do quadril no espaço do osso: converte de mundo
    w = Vector((0, 0, p["pelvis_dz"])) + Vector(p.get("pelvis_shift", (0, 0, 0)))
    rest = arm.data.bones["pelvis"].matrix_local.to_3x3()
    pel.location = rest.inverted() @ w
    # giros no eixo vertical do mundo, convertidos para o espaço local do osso
    world_rot = Euler((math.radians(p.get("pelvis_pitch", 0)), 0, math.radians(p["pelvis_yaw"])), "XYZ").to_matrix()
    pel.rotation_euler = (rest.inverted() @ world_rot @ rest).to_euler()
    for bn, yaw, pitch in (("spine_02", p["spine_yaw"] * 0.5, p["spine_pitch"] * 0.5),
                           ("spine_03", p["spine_yaw"] * 0.5, p["spine_pitch"] * 0.5),
                           ("Head", p["head_yaw"], -p["spine_pitch"] * 0.6)):
        b = pb[bn]
        b.rotation_mode = "XYZ"
        r = arm.data.bones[bn].matrix_local.to_3x3()
        b.rotation_euler = (r.inverted() @ Euler((math.radians(pitch), 0, math.radians(yaw)), "XYZ").to_matrix() @ r).to_euler()
        b.keyframe_insert("rotation_euler", frame=frame)
    pel.keyframe_insert("location", frame=frame)
    pel.keyframe_insert("rotation_euler", frame=frame)
    for k in ("hand_l", "hand_r", "foot_l", "foot_r"):
        T[k].location = p[k]
        T[k].keyframe_insert("location", frame=frame)
    for side in ("l", "r"):
        T["foot_" + side].rotation_euler = (math.radians(p.get(f"foot_{side}_pitch", 0)), 0,
                                            math.radians(p[f"foot_{side}_yaw"]))
        T["foot_" + side].keyframe_insert("rotation_euler", frame=frame)
    # polos: cotovelos para baixo e para fora, joelhos para a frente
    P["hand_l"].location = (p["hand_l"][0] + 0.35, p["hand_l"][1] + 0.25, SH_Z - 0.45)
    P["hand_r"].location = (p["hand_r"][0] - 0.35, p["hand_r"][1] + 0.25, SH_Z - 0.45)
    P["foot_l"].location = (p["foot_l"][0] + 0.10, p["foot_l"][1] - 0.6, 0.55)
    P["foot_r"].location = (p["foot_r"][0] - 0.10, p["foot_r"][1] - 0.6, 0.55)
    for k in P:
        P[k].keyframe_insert("location", frame=frame)


G = guard()
REACH = 0.62  # alcance do braço à frente do ombro

MOVES = {
    # jab de mão da frente: rápido, volta seco
    "Nadja_Jab": [
        (0, G),
        (2, guard(hand_l=(0.10, -0.24, SH_Z + 0.05), spine_yaw=-22)),
        (4, guard(hand_l=(SX * 0.4, -0.30 - REACH, SH_Z + 0.02), spine_yaw=-2, pelvis_yaw=22, head_yaw=-4)),
        (6, guard(hand_l=(SX * 0.4, -0.28 - REACH, SH_Z + 0.02), spine_yaw=-4, pelvis_yaw=22)),
        (14, G),
    ],
    # direto de trás: gira quadril e ombros, peso para a frente
    "Nadja_Direto": [
        (0, G),
        (3, guard(pelvis_dz=-0.10, spine_yaw=-26, hand_r=(-0.08, -0.16, SH_Z + 0.06))),
        (7, guard(pelvis_yaw=0, spine_yaw=18, spine_pitch=10, pelvis_shift=(0, -0.08, 0),
                  hand_r=(-0.02, -0.26 - REACH - 0.06, SH_Z + 0.0), hand_l=(0.14, -0.18, SH_Z + 0.08),
                  foot_r_yaw=80, foot_r_pitch=-25)),
        (10, guard(pelvis_yaw=2, spine_yaw=15, spine_pitch=10, pelvis_shift=(0, -0.07, 0),
                   hand_r=(-0.02, -0.24 - REACH - 0.06, SH_Z + 0.0), hand_l=(0.14, -0.18, SH_Z + 0.08),
                   foot_r_yaw=80, foot_r_pitch=-25)),
        (22, G),
    ],
    # chute circular médio com a perna de trás
    "Nadja_ChuteMedio": [
        (0, G),
        (5, guard(pelvis_yaw=55, spine_yaw=-30, spine_pitch=-6, pelvis_dz=-0.04,
                  foot_r=(-0.05, -0.05, 0.62), foot_r_yaw=90, foot_r_pitch=60)),
        (10, guard(pelvis_yaw=85, spine_yaw=-45, spine_pitch=-18, pelvis_dz=-0.02,
                   foot_r=(0.0, -0.95, 0.98), foot_r_yaw=95, foot_r_pitch=70,
                   hand_r=(-0.20, 0.05, SH_Z - 0.05), foot_l_yaw=110)),
        (13, guard(pelvis_yaw=85, spine_yaw=-45, spine_pitch=-18, pelvis_dz=-0.02,
                   foot_r=(0.0, -0.90, 0.96), foot_r_yaw=95, foot_r_pitch=70,
                   hand_r=(-0.20, 0.05, SH_Z - 0.05), foot_l_yaw=110)),
        (19, guard(pelvis_yaw=60, spine_yaw=-30, foot_r=(-0.05, -0.10, 0.55), foot_r_yaw=90, foot_r_pitch=50)),
        (30, G),
    ],
    "Nadja_Guarda": [(0, G), (30, guard(pelvis_dz=-0.085, hand_l=(0.10, -0.31, SH_Z + 0.07))), (60, G)],
}


def bake(name, keys):
    scene.frame_start, scene.frame_end = 0, keys[-1][0]
    for f, p in keys:
        pose_at(f, p)
    bpy.context.view_layer.objects.active = arm
    arm.select_set(True)
    bpy.ops.object.mode_set(mode="POSE")
    for b in arm.pose.bones:
        b.bone.select = True
    bpy.ops.nla.bake(frame_start=0, frame_end=keys[-1][0], only_selected=True, visual_keying=True,
                     clear_constraints=False, use_current_action=False, bake_types={"POSE"})
    bpy.ops.object.mode_set(mode="OBJECT")
    act = arm.animation_data.action
    act.name = name
    arm.animation_data.action = None
    tr = arm.animation_data.nla_tracks.new()
    tr.name = name
    st = tr.strips.new(name, 0, act)
    if hasattr(st, "action_slot") and act.slots:
        st.action_slot = act.slots[0]
    # limpa as chaves temporárias dos alvos
    for e in list(T.values()) + list(P.values()):
        e.animation_data_clear()
    return act


arm.animation_data_create()
baked = [bake(n, k) for n, k in MOVES.items()]
for b in arm.pose.bones:
    for c in list(b.constraints):
        b.constraints.remove(c)
for e in list(T.values()) + list(P.values()):
    bpy.data.objects.remove(e, do_unlink=True)
for a in list(bpy.data.actions):
    if a not in baked:
        bpy.data.actions.remove(a)

bpy.ops.export_scene.gltf(filepath=OUT, export_format="GLB", export_skins=True, export_animations=True,
                          export_animation_mode="NLA_TRACKS", export_force_sampling=True,
                          export_anim_single_armature=True, export_reset_pose_bones=True, export_morph=False,
                          export_cameras=False, export_lights=False, export_yup=True)
print("EXPORTED", OUT, os.path.getsize(OUT))

# folha de prévia de perfil (câmera do jogo é lateral)
os.makedirs(PREV, exist_ok=True)
cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
scene.collection.objects.link(cam)
scene.camera = cam
cam.location = (4.5, -0.3, 1.0)
cam.rotation_euler = (math.radians(90), 0, math.radians(90))
cam.data.lens = 40
scene.render.engine = "BLENDER_WORKBENCH"
scene.render.resolution_x, scene.render.resolution_y = 260, 300
ad = arm.animation_data
for tr in ad.nla_tracks:
    tr.mute = True
for r, tr in enumerate(ad.nla_tracks):
    tr.mute = False
    keys = [f for f, _ in MOVES[tr.name]]
    for c, f in enumerate(keys):
        scene.frame_set(f)
        scene.render.filepath = os.path.join(PREV, f"row{r}_{c}.png")
        bpy.ops.render.render(write_still=True)
    tr.mute = True
    print("PREV", tr.name, keys)
