"""Golpes e reações animados à mão (poses-chave com IK), assados no esqueleto Quaternius de 65 ossos.

Cada clipe é uma lista de poses-chave (quadro a 60/s): alvos de IK de mãos e pés, altura/giro/inclinação do
quadril, giro e inclinação do tronco, cabeça. O Blender resolve braços e pernas e o resultado é assado em
rotações dos ossos. Frente = -Y. Base: pé e mão esquerdos (+X) à frente.
O alvo do pé gira por cima da orientação de repouso do osso do pé (antes copiava uma rotação absoluta e
torcia o pé).

Uso: bl.sh --python keyanim2.py -- <ual.glb> <out_dir> <prev_dir>
Saídas: <out_dir>/nadja_golpes.glb, <out_dir>/otto_golpes.glb e <out_dir>/contatos.json
"""
import json
import math
import os
import sys

import bpy
from mathutils import Euler, Quaternion, Vector

UAL, OUT_DIR, PREV = sys.argv[sys.argv.index("--") + 1:]
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
SH_Z = B["upperarm_l"].z
SX = B["upperarm_l"].x
FOOT_REST = {s: (arm.matrix_world @ arm.data.bones["foot_" + s].matrix_local).to_quaternion() for s in ("l", "r")}


def empty(name):
    e = bpy.data.objects.new(name, None)
    scene.collection.objects.link(e)
    e.rotation_mode = "QUATERNION"
    return e


T = {k: empty("ik_" + k) for k in ("hand_l", "hand_r", "foot_l", "foot_r")}
P = {k: empty("pole_" + k) for k in ("hand_l", "hand_r", "foot_l", "foot_r")}
CHAIN = {"hand_l": "lowerarm_l", "hand_r": "lowerarm_r", "foot_l": "calf_l", "foot_r": "calf_r"}
for k, bone in CHAIN.items():
    c = arm.pose.bones[bone].constraints.new("IK")
    c.target, c.pole_target = T[k], P[k]
    c.pole_angle = math.radians(-90)
    c.chain_count = 2
for side in ("l", "r"):
    c = arm.pose.bones["foot_" + side].constraints.new("COPY_ROTATION")
    c.target = T["foot_" + side]
    c.mix_mode = "REPLACE"


def world_rot_local(bone, pitch, yaw, roll=0.0):
    r = arm.data.bones[bone].matrix_local.to_3x3()
    m = Euler((math.radians(pitch), math.radians(roll), math.radians(yaw)), "XYZ").to_matrix()
    return (r.inverted() @ m @ r).to_euler()


def pose_at(frame, p):
    scene.frame_set(frame)
    pb = arm.pose.bones
    pel = pb["pelvis"]
    pel.rotation_mode = "XYZ"
    rest = arm.data.bones["pelvis"].matrix_local.to_3x3()
    pel.location = rest.inverted() @ (Vector((0, 0, p["pelvis_dz"])) + Vector(p.get("pelvis_shift", (0, 0, 0))))
    pel.rotation_euler = world_rot_local("pelvis", p.get("pelvis_pitch", 0), p["pelvis_yaw"], p.get("pelvis_roll", 0))
    pel.keyframe_insert("location", frame=frame)
    pel.keyframe_insert("rotation_euler", frame=frame)
    for bn, yaw, pitch in (("spine_02", p["spine_yaw"] * 0.5, p["spine_pitch"] * 0.5),
                           ("spine_03", p["spine_yaw"] * 0.5, p["spine_pitch"] * 0.5),
                           ("Head", p["head_yaw"], -p["spine_pitch"] * 0.6 + p.get("head_pitch", 0))):
        b = pb[bn]
        b.rotation_mode = "XYZ"
        b.rotation_euler = world_rot_local(bn, pitch, yaw)
        b.keyframe_insert("rotation_euler", frame=frame)
    for k in T:
        T[k].location = p[k]
        T[k].keyframe_insert("location", frame=frame)
    for s in ("l", "r"):
        q = (Quaternion((0, 0, 1), math.radians(p[f"foot_{s}_yaw"])) @
             Quaternion((1, 0, 0), math.radians(p.get(f"foot_{s}_pitch", 0))) @ FOOT_REST[s])
        T["foot_" + s].rotation_quaternion = q
        T["foot_" + s].keyframe_insert("rotation_quaternion", frame=frame)
    lift = p.get("pole_up", 0.0)
    P["hand_l"].location = (p["hand_l"][0] + 0.35, p["hand_l"][1] + 0.25, SH_Z - 0.45 + lift)
    P["hand_r"].location = (p["hand_r"][0] - 0.35, p["hand_r"][1] + 0.25, SH_Z - 0.45 + lift)
    P["foot_l"].location = (p["foot_l"][0] + 0.10, p["foot_l"][1] - 0.6, p["foot_l"][2] + 0.45)
    P["foot_r"].location = (p["foot_r"][0] - 0.10, p["foot_r"][1] - 0.6, p["foot_r"][2] + 0.45)
    for k in P:
        P[k].keyframe_insert("location", frame=frame)


# ================================================================ Nadja (rushdown, base baixa)
def ng(**o):
    p = dict(pelvis_dz=-0.07, pelvis_yaw=28.0, spine_yaw=-18.0, spine_pitch=6.0, head_yaw=-8.0,
             hand_l=(0.10, -0.30, SH_Z + 0.06), hand_r=(-0.06, -0.20, SH_Z + 0.10),
             foot_l=(0.10, -0.22, 0.10), foot_r=(-0.13, 0.22, 0.10), foot_l_yaw=15.0, foot_r_yaw=30.0)
    p.update(o)
    return p


def nc(**o):  # agachada
    p = ng(pelvis_dz=-0.42, spine_pitch=18, hand_l=(0.10, -0.32, SH_Z - 0.30), hand_r=(-0.06, -0.22, SH_Z - 0.26),
           foot_l=(0.15, -0.25, 0.10), foot_r=(-0.17, 0.25, 0.10))
    p.update(o)
    return p


def nair(**o):  # no ar: pernas recolhidas
    p = ng(pelvis_dz=0.0, foot_l=(0.10, -0.12, 0.45), foot_r=(-0.12, 0.10, 0.38), foot_l_pitch=20, foot_r_pitch=30,
           spine_pitch=10)
    p.update(o)
    return p


R = 0.62
NG, NC, NA = ng(), nc(), nair()
N = {
    "Nadja_Guarda": ([(0, NG), (30, ng(pelvis_dz=-0.085, hand_l=(0.10, -0.31, SH_Z + 0.07))), (60, NG)], None),
    "Nadja_Jab": ([(0, NG), (2, ng(hand_l=(0.10, -0.24, SH_Z + 0.05), spine_yaw=-22)),
                   (4, ng(hand_l=(SX * 0.4, -0.30 - R, SH_Z + 0.02), spine_yaw=-2, pelvis_yaw=22, head_yaw=-4)),
                   (6, ng(hand_l=(SX * 0.4, -0.28 - R, SH_Z + 0.02), spine_yaw=-4, pelvis_yaw=22)), (14, NG)], 4),
    "Nadja_Direto": ([(0, NG), (3, ng(pelvis_dz=-0.10, spine_yaw=-26, hand_r=(-0.08, -0.16, SH_Z + 0.06))),
                      (7, ng(pelvis_yaw=0, spine_yaw=18, spine_pitch=10, pelvis_shift=(0, -0.08, 0),
                             hand_r=(-0.02, -0.32 - R, SH_Z), hand_l=(0.14, -0.18, SH_Z + 0.08), foot_r_yaw=45, foot_r_pitch=-20)),
                      (10, ng(pelvis_yaw=2, spine_yaw=15, spine_pitch=10, pelvis_shift=(0, -0.07, 0),
                              hand_r=(-0.02, -0.30 - R, SH_Z), hand_l=(0.14, -0.18, SH_Z + 0.08), foot_r_yaw=45, foot_r_pitch=-20)),
                      (22, NG)], 7),
    # gancho de trás em arco, na altura da cabeça
    "Nadja_Gancho": ([(0, NG), (4, ng(pelvis_dz=-0.11, spine_yaw=-38, hand_r=(-0.40, -0.05, SH_Z - 0.02))),
                      (9, ng(pelvis_yaw=-5, spine_yaw=30, spine_pitch=12, pelvis_shift=(0, -0.06, 0),
                             hand_r=(0.15, -0.55, SH_Z + 0.05), hand_l=(0.16, -0.16, SH_Z + 0.08), foot_r_yaw=50, foot_r_pitch=-25, pole_up=0.35)),
                      (12, ng(pelvis_yaw=-5, spine_yaw=34, spine_pitch=12, pelvis_shift=(0, -0.06, 0),
                              hand_r=(0.25, -0.48, SH_Z + 0.05), hand_l=(0.16, -0.16, SH_Z + 0.08), foot_r_yaw=50, pole_up=0.35)),
                      (26, NG)], 9),
    # chute baixo rápido com a perna da frente
    "Nadja_ChuteBaixo": ([(0, NG), (3, ng(foot_l=(0.10, -0.30, 0.30), foot_l_pitch=40, pelvis_dz=-0.05)),
                          (5, ng(foot_l=(0.10, -0.85, 0.32), foot_l_pitch=70, pelvis_yaw=40, spine_pitch=-6, pelvis_dz=-0.04)),
                          (8, ng(foot_l=(0.10, -0.80, 0.30), foot_l_pitch=70, pelvis_yaw=40, spine_pitch=-6, pelvis_dz=-0.04)),
                          (16, NG)], 5),
    "Nadja_ChuteMedio": ([(0, NG), (5, ng(pelvis_yaw=55, spine_yaw=-30, spine_pitch=-6, pelvis_dz=-0.04,
                                         foot_r=(-0.05, -0.05, 0.62), foot_r_yaw=90, foot_r_pitch=60)),
                          (10, ng(pelvis_yaw=85, spine_yaw=-45, spine_pitch=-18, pelvis_dz=-0.02, foot_r=(0.0, -0.95, 0.98),
                                  foot_r_yaw=95, foot_r_pitch=70, hand_r=(-0.20, 0.05, SH_Z - 0.05), foot_l_yaw=110)),
                          (13, ng(pelvis_yaw=85, spine_yaw=-45, spine_pitch=-18, pelvis_dz=-0.02, foot_r=(0.0, -0.90, 0.96),
                                  foot_r_yaw=95, foot_r_pitch=70, hand_r=(-0.20, 0.05, SH_Z - 0.05), foot_l_yaw=110)),
                          (19, ng(pelvis_yaw=60, spine_yaw=-30, foot_r=(-0.05, -0.10, 0.55), foot_r_yaw=90, foot_r_pitch=50)),
                          (30, NG)], 10),
    # circular alto
    "Nadja_ChuteAlto": ([(0, NG), (6, ng(pelvis_yaw=60, spine_yaw=-35, spine_pitch=-10, pelvis_dz=-0.03,
                                        foot_r=(-0.05, -0.10, 0.85), foot_r_yaw=90, foot_r_pitch=70)),
                         (12, ng(pelvis_yaw=100, spine_yaw=-55, spine_pitch=-30, pelvis_dz=0.0, foot_r=(0.05, -0.85, 1.48),
                                 foot_r_yaw=100, foot_r_pitch=80, hand_r=(-0.30, 0.10, SH_Z - 0.10), hand_l=(0.20, -0.10, SH_Z),
                                 foot_l_yaw=120)),
                         (15, ng(pelvis_yaw=100, spine_yaw=-55, spine_pitch=-30, pelvis_dz=0.0, foot_r=(0.05, -0.80, 1.44),
                                 foot_r_yaw=100, foot_r_pitch=80, hand_r=(-0.30, 0.10, SH_Z - 0.10), hand_l=(0.20, -0.10, SH_Z),
                                 foot_l_yaw=120)),
                         (23, ng(pelvis_yaw=65, spine_yaw=-30, foot_r=(-0.05, -0.12, 0.7), foot_r_yaw=90, foot_r_pitch=60)),
                         (36, NG)], 12),
    "Nadja_Agachada": ([(0, NC), (30, nc(pelvis_dz=-0.44)), (60, NC)], None),
    "Nadja_AgSocoLeve": ([(0, NC), (2, nc(hand_l=(0.10, -0.26, SH_Z - 0.32))),
                          (4, nc(hand_l=(SX * 0.4, -0.30 - R, SH_Z - 0.36), spine_yaw=-2, pelvis_yaw=22)),
                          (6, nc(hand_l=(SX * 0.4, -0.28 - R, SH_Z - 0.36), spine_yaw=-4, pelvis_yaw=22)), (13, NC)], 4),
    "Nadja_AgSocoMedio": ([(0, NC), (3, nc(spine_yaw=-26)),
                           (7, nc(pelvis_yaw=0, spine_yaw=18, hand_r=(-0.02, -0.30 - R, SH_Z - 0.34), foot_r_yaw=45)),
                           (10, nc(pelvis_yaw=2, spine_yaw=15, hand_r=(-0.02, -0.28 - R, SH_Z - 0.34), foot_r_yaw=45)), (20, NC)], 7),
    # gancho para cima saindo da agachada (anti-aéreo)
    "Nadja_Upper": ([(0, NC), (4, nc(pelvis_dz=-0.46, spine_yaw=-30, hand_r=(-0.10, -0.12, SH_Z - 0.55))),
                     (8, ng(pelvis_dz=0.02, pelvis_yaw=0, spine_yaw=20, spine_pitch=-8, hand_r=(0.02, -0.42, SH_Z + 0.45),
                            hand_l=(0.16, -0.16, SH_Z + 0.06), foot_r_pitch=-30, pole_up=0.6)),
                     (11, ng(pelvis_dz=0.02, pelvis_yaw=0, spine_yaw=20, spine_pitch=-10, hand_r=(0.02, -0.36, SH_Z + 0.50),
                             hand_l=(0.16, -0.16, SH_Z + 0.06), foot_r_pitch=-30, pole_up=0.6)),
                     (26, NC)], 8),
    "Nadja_AgChuteLeve": ([(0, NC), (3, nc(foot_l=(0.12, -0.40, 0.12), foot_l_pitch=30)),
                           (5, nc(foot_l=(0.12, -0.92, 0.10), foot_l_pitch=75, pelvis_yaw=40)),
                           (8, nc(foot_l=(0.12, -0.88, 0.10), foot_l_pitch=75, pelvis_yaw=40)), (15, NC)], 5),
    "Nadja_AgChuteMedio": ([(0, NC), (4, nc(foot_r=(-0.05, 0.05, 0.25), foot_r_pitch=40, pelvis_yaw=50)),
                            (8, nc(foot_r=(0.0, -1.0, 0.12), foot_r_yaw=90, foot_r_pitch=80, pelvis_yaw=85, spine_yaw=-30,
                                   hand_r=(-0.25, 0.10, SH_Z - 0.40), foot_l_yaw=100)),
                            (11, nc(foot_r=(0.0, -0.96, 0.12), foot_r_yaw=90, foot_r_pitch=80, pelvis_yaw=85, spine_yaw=-30,
                                    hand_r=(-0.25, 0.10, SH_Z - 0.40), foot_l_yaw=100)), (22, NC)], 8),
    # rasteira: gira rente ao chão
    "Nadja_Rasteira": ([(0, NC), (4, nc(pelvis_dz=-0.55, pelvis_yaw=70, spine_yaw=-20, spine_pitch=28,
                                       hand_l=(0.30, -0.30, 0.30), hand_r=(0.05, -0.20, 0.30), foot_r=(-0.30, 0.20, 0.10))),
                        (10, nc(pelvis_dz=-0.58, pelvis_yaw=150, spine_yaw=-40, spine_pitch=30, hand_l=(0.30, -0.10, 0.25),
                                hand_r=(0.10, -0.25, 0.25), foot_r=(0.20, -1.05, 0.08), foot_r_yaw=150, foot_r_pitch=80)),
                        (14, nc(pelvis_dz=-0.58, pelvis_yaw=160, spine_yaw=-40, spine_pitch=30, hand_l=(0.30, -0.10, 0.25),
                                hand_r=(0.10, -0.25, 0.25), foot_r=(0.35, -0.95, 0.08), foot_r_yaw=160, foot_r_pitch=80)),
                        (30, NC)], 10),
    "Nadja_PuloInicio": ([(0, NG), (4, ng(pelvis_dz=-0.20, spine_pitch=14, hand_l=(0.12, -0.25, SH_Z - 0.10))),
                          (6, ng(pelvis_dz=-0.02, spine_pitch=-4, foot_l_pitch=-30, foot_r_pitch=-30))], None),
    "Nadja_PuloAr": ([(0, NA), (20, nair(foot_l=(0.10, -0.14, 0.50), foot_r=(-0.12, 0.08, 0.42))), (40, NA)], None),
    "Nadja_Aterrissa": ([(0, NA), (3, ng(pelvis_dz=-0.24, spine_pitch=16)), (10, NG)], None),
    "Nadja_AereoSoco": ([(0, NA), (3, nair(hand_l=(0.10, -0.26, SH_Z))),
                         (5, nair(hand_l=(SX * 0.4, -0.30 - R, SH_Z - 0.20), spine_pitch=20, pelvis_yaw=20)),
                         (10, nair(hand_l=(SX * 0.4, -0.28 - R, SH_Z - 0.20), spine_pitch=20, pelvis_yaw=20)), (18, NA)], 5),
    "Nadja_AereoChute": ([(0, NA), (3, nair(foot_l=(0.10, -0.20, 0.55), foot_l_pitch=40)),
                          (6, nair(foot_l=(0.10, -0.85, 0.15), foot_l_pitch=70, pelvis_yaw=45, spine_pitch=-12)),
                          (14, nair(foot_l=(0.10, -0.82, 0.18), foot_l_pitch=70, pelvis_yaw=45, spine_pitch=-12)), (22, NA)], 6),
    "Nadja_Dash": ([(0, NG), (4, ng(pelvis_dz=-0.12, spine_pitch=22, pelvis_shift=(0, -0.10, 0), foot_r=(-0.13, 0.35, 0.18),
                                     foot_r_pitch=-30)),
                    (10, ng(pelvis_dz=-0.10, spine_pitch=18, pelvis_shift=(0, -0.05, 0), foot_l=(0.10, -0.40, 0.12))),
                    (16, NG)], None),
    "Nadja_Recuo": ([(0, NG), (4, ng(pelvis_dz=-0.05, spine_pitch=-10, pelvis_shift=(0, 0.10, 0), foot_l=(0.10, -0.30, 0.22))),
                     (10, ng(pelvis_dz=-0.12, spine_pitch=4, pelvis_shift=(0, 0.06, 0))), (16, NG)], None),
    "Nadja_Bloqueio": ([(0, NG), (3, ng(pelvis_dz=-0.10, spine_pitch=10, pelvis_shift=(0, 0.05, 0),
                                         hand_l=(0.08, -0.22, SH_Z + 0.16), hand_r=(-0.05, -0.18, SH_Z + 0.18))), (12, NG)], None),
    "Nadja_AgBloqueio": ([(0, NC), (3, nc(pelvis_shift=(0, 0.05, 0), hand_l=(0.08, -0.24, SH_Z - 0.18),
                                          hand_r=(-0.05, -0.20, SH_Z - 0.14))), (12, NC)], None),
    "Nadja_Hit": ([(0, NG), (2, ng(spine_pitch=-22, head_pitch=-25, head_yaw=10, pelvis_shift=(0, 0.08, 0),
                                   hand_l=(0.25, -0.05, SH_Z - 0.10), hand_r=(-0.20, 0.05, SH_Z - 0.05))),
                   (8, ng(spine_pitch=-10, head_pitch=-10, pelvis_shift=(0, 0.04, 0))), (20, NG)], None),
}


def fall(base, sx=1.0):
    """Queda de costas até o chão e a pose deitada."""
    lie = base(pelvis_dz=-0.76, pelvis_pitch=-82, pelvis_yaw=0, spine_yaw=0, spine_pitch=-4, head_yaw=0,
               pelvis_shift=(0, 0.45, 0), hand_l=(0.45, 0.85, 0.10), hand_r=(-0.45, 0.85, 0.10),
               foot_l=(0.15, -0.40, 0.08), foot_r=(-0.15, -0.35, 0.08), foot_l_pitch=-80, foot_r_pitch=-80,
               foot_l_yaw=0, foot_r_yaw=0, pole_up=-0.5)
    mid = base(pelvis_dz=-0.35, pelvis_pitch=-40, spine_pitch=-20, head_pitch=-20, pelvis_shift=(0, 0.25, 0),
               hand_l=(0.40, 0.10, SH_Z - 0.25), hand_r=(-0.40, 0.10, SH_Z - 0.25), foot_l=(0.12, -0.35, 0.30),
               foot_r=(-0.15, -0.10, 0.10), foot_l_pitch=-40)
    return [(0, base(spine_pitch=-20, head_pitch=-25, pelvis_shift=(0, 0.06, 0))), (8, mid), (18, lie), (40, lie)], lie


def wake(base, lie):
    sit = base(pelvis_dz=-0.62, pelvis_pitch=-20, spine_pitch=25, pelvis_shift=(0, 0.25, 0),
               hand_l=(0.35, 0.40, 0.15), hand_r=(-0.35, 0.40, 0.15), foot_l=(0.15, -0.30, 0.08),
               foot_r=(-0.15, -0.05, 0.08), foot_l_yaw=0, foot_r_yaw=0)
    kneel = base(pelvis_dz=-0.38, spine_pitch=20, pelvis_shift=(0, 0.10, 0), foot_r=(-0.13, 0.30, 0.10))
    return [(0, lie), (10, sit), (20, kneel), (30, base())]


nq, nlie = fall(ng)
N["Nadja_Queda"] = (nq, None)
N["Nadja_Levanta"] = (wake(ng, nlie), None)


# ================================================================ Otto (grappler, base larga e pesada)
def og(**o):
    p = dict(pelvis_dz=-0.10, pelvis_yaw=12.0, spine_yaw=-6.0, spine_pitch=12.0, head_yaw=-4.0,
             hand_l=(0.30, -0.32, SH_Z - 0.12), hand_r=(-0.26, -0.28, SH_Z - 0.16),
             foot_l=(0.20, -0.18, 0.10), foot_r=(-0.22, 0.20, 0.10), foot_l_yaw=10.0, foot_r_yaw=25.0)
    p.update(o)
    return p


OG = og()
O = {
    "Otto_Guarda": ([(0, OG), (40, og(pelvis_dz=-0.12, spine_pitch=13, hand_l=(0.30, -0.33, SH_Z - 0.10))), (80, OG)], None),
    # jab curto e pesado com a mão da frente
    "Otto_Jab": ([(0, OG), (3, og(hand_l=(0.25, -0.22, SH_Z - 0.06), spine_yaw=-12)),
                  (6, og(hand_l=(0.12, -0.32 - 0.66, SH_Z - 0.02), spine_yaw=4, pelvis_yaw=6, spine_pitch=16)),
                  (9, og(hand_l=(0.12, -0.30 - 0.66, SH_Z - 0.02), spine_yaw=2, pelvis_yaw=6, spine_pitch=16)), (20, OG)], 6),
    # martelo de cima para baixo com a mão de trás
    "Otto_Martelo": ([(0, OG), (5, og(pelvis_dz=-0.06, spine_pitch=-6, spine_yaw=-20, hand_r=(-0.20, -0.10, SH_Z + 0.45),
                                       pole_up=0.6)),
                      (10, og(pelvis_dz=-0.18, spine_pitch=28, spine_yaw=10, pelvis_shift=(0, -0.08, 0),
                              hand_r=(-0.02, -0.80, SH_Z - 0.25), foot_r_pitch=-20)),
                      (13, og(pelvis_dz=-0.18, spine_pitch=28, spine_yaw=10, pelvis_shift=(0, -0.08, 0),
                              hand_r=(-0.02, -0.78, SH_Z - 0.30))), (30, OG)], 10),
    # cabeçada: recua a cabeça e projeta o tronco
    "Otto_Cabecada": ([(0, OG), (5, og(spine_pitch=-14, head_pitch=-20, pelvis_shift=(0, 0.06, 0),
                                        hand_l=(0.40, -0.40, SH_Z - 0.05), hand_r=(-0.40, -0.36, SH_Z - 0.05))),
                       (9, og(spine_pitch=40, head_pitch=25, pelvis_dz=-0.14, pelvis_shift=(0, -0.16, 0),
                              hand_l=(0.30, -0.60, SH_Z - 0.20), hand_r=(-0.30, -0.56, SH_Z - 0.20), foot_r_pitch=-25)),
                       (12, og(spine_pitch=38, head_pitch=22, pelvis_dz=-0.14, pelvis_shift=(0, -0.15, 0),
                               hand_l=(0.30, -0.58, SH_Z - 0.20), hand_r=(-0.30, -0.54, SH_Z - 0.20))), (30, OG)], 9),
    # agarrão de comando: abre os braços, fecha na frente, levanta
    "Otto_Agarrao": ([(0, OG), (4, og(hand_l=(0.55, -0.50, SH_Z), hand_r=(-0.55, -0.48, SH_Z), spine_pitch=18)),
                      (8, og(hand_l=(0.15, -0.75, SH_Z - 0.10), hand_r=(-0.15, -0.75, SH_Z - 0.10), spine_pitch=24,
                             pelvis_shift=(0, -0.08, 0), pelvis_dz=-0.16)),
                      (14, og(hand_l=(0.15, -0.70, SH_Z - 0.05), hand_r=(-0.15, -0.70, SH_Z - 0.05), spine_pitch=20,
                              pelvis_shift=(0, -0.06, 0), pelvis_dz=-0.16)), (26, OG)], 8),
    # arremesso: ergue sobre o ombro e bate no chão à frente
    "Otto_Arremesso": ([(0, og(hand_l=(0.15, -0.70, SH_Z - 0.05), hand_r=(-0.15, -0.70, SH_Z - 0.05), spine_pitch=20)),
                        (10, og(pelvis_dz=-0.02, spine_pitch=-18, hand_l=(0.20, -0.15, SH_Z + 0.55),
                                hand_r=(-0.20, -0.15, SH_Z + 0.55), pole_up=0.7)),
                        (18, og(pelvis_dz=-0.30, spine_pitch=45, pelvis_shift=(0, -0.10, 0), hand_l=(0.20, -0.85, 0.40),
                                hand_r=(-0.20, -0.85, 0.40), foot_r_pitch=-25)),
                        (24, og(pelvis_dz=-0.30, spine_pitch=45, pelvis_shift=(0, -0.10, 0), hand_l=(0.20, -0.85, 0.35),
                                hand_r=(-0.20, -0.85, 0.35))), (44, OG)], 18),
    # 623P: ombrada para cima (anti-aéreo)
    "Otto_Upper": ([(0, OG), (3, og(pelvis_dz=-0.26, spine_pitch=26, spine_yaw=-20, hand_r=(-0.20, -0.15, SH_Z - 0.50))),
                    (7, og(pelvis_dz=0.02, spine_pitch=-12, spine_yaw=20, pelvis_shift=(0, -0.08, 0),
                           hand_r=(0.0, -0.45, SH_Z + 0.50), hand_l=(0.32, -0.20, SH_Z), pole_up=0.6, foot_r_pitch=-30)),
                    (11, og(pelvis_dz=0.02, spine_pitch=-14, spine_yaw=20, pelvis_shift=(0, -0.08, 0),
                            hand_r=(0.0, -0.40, SH_Z + 0.55), hand_l=(0.32, -0.20, SH_Z), pole_up=0.6)), (30, OG)], 7),
    # bloqueio: antebraços fechados à frente do peito e do queixo, tronco quase ereto (antes cobria a cabeça e
    # se encolhia demais)
    "Otto_Bloqueio": ([(0, OG), (3, og(pelvis_dz=-0.13, spine_pitch=8, pelvis_shift=(0, 0.04, 0), head_pitch=6,
                                       hand_l=(0.20, -0.40, SH_Z - 0.04), hand_r=(-0.14, -0.36, SH_Z - 0.10))),
                       (9, og(pelvis_dz=-0.13, spine_pitch=8, pelvis_shift=(0, 0.04, 0), head_pitch=6,
                              hand_l=(0.20, -0.39, SH_Z - 0.04), hand_r=(-0.14, -0.35, SH_Z - 0.10))), (16, OG)], None),
    "Otto_Hit": ([(0, OG), (3, og(spine_pitch=-12, head_pitch=-22, head_yaw=10, pelvis_shift=(0, 0.06, 0),
                                  hand_l=(0.40, -0.10, SH_Z - 0.25), hand_r=(-0.35, -0.05, SH_Z - 0.25))),
                 (9, og(spine_pitch=0, head_pitch=-8, pelvis_shift=(0, 0.03, 0))), (22, OG)], None),
}
def oc(**o):  # agachado: base larga, tronco sobre os joelhos
    p = og(pelvis_dz=-0.40, spine_pitch=24, hand_l=(0.28, -0.36, SH_Z - 0.40), hand_r=(-0.24, -0.30, SH_Z - 0.44),
           foot_l=(0.24, -0.24, 0.10), foot_r=(-0.26, 0.24, 0.10))
    p.update(o)
    return p


def oair(**o):  # no ar: pernas recolhidas, braços abertos para o peso
    p = og(pelvis_dz=0.0, spine_pitch=14, foot_l=(0.20, -0.12, 0.42), foot_r=(-0.20, 0.12, 0.36), foot_l_pitch=20,
           foot_r_pitch=30, hand_l=(0.32, -0.30, SH_Z - 0.04), hand_r=(-0.28, -0.26, SH_Z - 0.08))
    p.update(o)
    return p


OC, OA = oc(), oair()
RO = 0.62  # alcance do braço do Otto à frente da guarda
# chutes em pé
O["Otto_ChuteBaixo"] = ([(0, OG), (3, og(foot_l=(0.20, -0.30, 0.32), foot_l_pitch=30, pelvis_dz=-0.08)),
                         (5, og(foot_l=(0.18, -0.80, 0.22), foot_l_pitch=60, pelvis_yaw=30, spine_pitch=4, pelvis_dz=-0.07)),
                         (8, og(foot_l=(0.18, -0.76, 0.22), foot_l_pitch=60, pelvis_yaw=30, spine_pitch=4, pelvis_dz=-0.07)),
                         (18, OG)], 5)
# chute frontal de sola (empurrão) com a perna de trás, na altura da barriga
O["Otto_ChuteMedio"] = ([(0, OG), (4, og(foot_r=(-0.10, -0.10, 0.55), foot_r_pitch=50, pelvis_yaw=20, spine_pitch=-2,
                                         pelvis_dz=-0.06)),
                         (8, og(foot_r=(-0.05, -0.92, 0.78), foot_r_pitch=80, foot_r_yaw=10, spine_pitch=-14, pelvis_yaw=30,
                                pelvis_shift=(0, -0.06, 0), pelvis_dz=-0.05)),
                         (11, og(foot_r=(-0.05, -0.88, 0.76), foot_r_pitch=80, foot_r_yaw=10, spine_pitch=-14, pelvis_yaw=30,
                                 pelvis_shift=(0, -0.06, 0), pelvis_dz=-0.05)),
                         (18, og(foot_r=(-0.10, -0.25, 0.50), foot_r_pitch=40, spine_pitch=0)), (28, OG)], 8)
# circular pesado na altura das costelas
O["Otto_ChuteAlto"] = ([(0, OG), (6, og(pelvis_yaw=50, spine_yaw=-30, pelvis_dz=-0.06, foot_r=(-0.10, -0.05, 0.60),
                                        foot_r_yaw=80, foot_r_pitch=50)),
                        (13, og(pelvis_yaw=95, spine_yaw=-50, spine_pitch=-24, pelvis_dz=-0.03, foot_r=(0.05, -0.88, 1.12),
                                foot_r_yaw=95, foot_r_pitch=75, hand_r=(-0.30, 0.10, SH_Z - 0.15), foot_l_yaw=110)),
                        (17, og(pelvis_yaw=95, spine_yaw=-50, spine_pitch=-24, pelvis_dz=-0.03, foot_r=(0.05, -0.84, 1.08),
                                foot_r_yaw=95, foot_r_pitch=75, hand_r=(-0.30, 0.10, SH_Z - 0.15), foot_l_yaw=110)),
                        (26, og(pelvis_yaw=55, spine_yaw=-28, foot_r=(-0.05, -0.15, 0.55), foot_r_yaw=80, foot_r_pitch=45)),
                        (40, OG)], 13)
# 6HP: marreta de duas mãos por cima da cabeça
O["Otto_Marreta"] = ([(0, OG), (10, og(pelvis_dz=-0.04, spine_pitch=-14, head_pitch=-10, hand_l=(0.12, 0.02, SH_Z + 0.50),
                                       hand_r=(-0.12, 0.02, SH_Z + 0.50), pole_up=0.8)),
                      (19, og(pelvis_dz=-0.22, spine_pitch=40, pelvis_shift=(0, -0.10, 0), hand_l=(0.10, -0.80, SH_Z - 0.45),
                              hand_r=(-0.10, -0.80, SH_Z - 0.45), foot_r_pitch=-25)),
                      (22, og(pelvis_dz=-0.22, spine_pitch=40, pelvis_shift=(0, -0.10, 0), hand_l=(0.10, -0.78, SH_Z - 0.48),
                              hand_r=(-0.10, -0.78, SH_Z - 0.48))), (38, OG)], 19)
# agachado
O["Otto_Agachada"] = ([(0, OC), (40, oc(pelvis_dz=-0.42, spine_pitch=25)), (80, OC)], None)
O["Otto_AgBloqueio"] = ([(0, OC), (3, oc(pelvis_shift=(0, 0.04, 0), spine_pitch=16, hand_l=(0.20, -0.38, SH_Z - 0.30),
                                         hand_r=(-0.14, -0.34, SH_Z - 0.34))),
                         (9, oc(pelvis_shift=(0, 0.04, 0), spine_pitch=16, hand_l=(0.20, -0.37, SH_Z - 0.30),
                                hand_r=(-0.14, -0.33, SH_Z - 0.34))), (16, OC)], None)
O["Otto_AgSocoLeve"] = ([(0, OC), (2, oc(hand_l=(0.24, -0.28, SH_Z - 0.38))),
                         (4, oc(hand_l=(0.12, -0.30 - RO, SH_Z - 0.42), spine_yaw=4, pelvis_yaw=8)),
                         (6, oc(hand_l=(0.12, -0.28 - RO, SH_Z - 0.42), spine_yaw=2, pelvis_yaw=8)), (15, OC)], 4)
O["Otto_AgSocoMedio"] = ([(0, OC), (3, oc(spine_yaw=-22, hand_r=(-0.26, -0.18, SH_Z - 0.42))),
                          (6, oc(spine_yaw=16, pelvis_yaw=0, hand_r=(-0.02, -0.30 - RO, SH_Z - 0.40), foot_r_yaw=40)),
                          (9, oc(spine_yaw=14, pelvis_yaw=0, hand_r=(-0.02, -0.28 - RO, SH_Z - 0.40), foot_r_yaw=40)),
                          (23, OC)], 6)
# 2HP: gancho subindo da base agachada (anti-aéreo)
O["Otto_AgUpper"] = ([(0, OC), (4, oc(pelvis_dz=-0.46, spine_yaw=-26, hand_r=(-0.12, -0.14, SH_Z - 0.62))),
                      (8, og(pelvis_dz=-0.04, pelvis_yaw=0, spine_yaw=20, spine_pitch=-8, hand_r=(0.0, -0.42, SH_Z + 0.45),
                             hand_l=(0.30, -0.22, SH_Z - 0.02), foot_r_pitch=-30, pole_up=0.6)),
                      (13, og(pelvis_dz=-0.04, pelvis_yaw=0, spine_yaw=20, spine_pitch=-10, hand_r=(0.0, -0.38, SH_Z + 0.50),
                              hand_l=(0.30, -0.22, SH_Z - 0.02), foot_r_pitch=-30, pole_up=0.6)), (38, OC)], 8)
O["Otto_AgChuteLeve"] = ([(0, OC), (3, oc(foot_l=(0.22, -0.42, 0.14), foot_l_pitch=30)),
                          (5, oc(foot_l=(0.20, -0.95, 0.10), foot_l_pitch=75, pelvis_yaw=35)),
                          (7, oc(foot_l=(0.20, -0.92, 0.10), foot_l_pitch=75, pelvis_yaw=35)), (17, OC)], 5)
O["Otto_AgChuteMedio"] = ([(0, OC), (4, oc(foot_r=(-0.08, 0.05, 0.25), foot_r_pitch=40, pelvis_yaw=45)),
                           (7, oc(foot_r=(0.0, -1.0, 0.12), foot_r_yaw=90, foot_r_pitch=80, pelvis_yaw=80, spine_yaw=-28,
                                  hand_r=(-0.30, 0.08, SH_Z - 0.46), foot_l_yaw=95)),
                           (10, oc(foot_r=(0.0, -0.96, 0.12), foot_r_yaw=90, foot_r_pitch=80, pelvis_yaw=80, spine_yaw=-28,
                                   hand_r=(-0.30, 0.08, SH_Z - 0.46), foot_l_yaw=95)), (26, OC)], 7)
# 2HK: rasteira pesada, gira rente ao chão
O["Otto_Rasteira"] = ([(0, OC), (5, oc(pelvis_dz=-0.52, pelvis_yaw=70, spine_yaw=-20, spine_pitch=30,
                                       hand_l=(0.36, -0.30, 0.32), hand_r=(0.05, -0.22, 0.32), foot_r=(-0.32, 0.20, 0.10))),
                       (10, oc(pelvis_dz=-0.55, pelvis_yaw=150, spine_yaw=-40, spine_pitch=32, hand_l=(0.36, -0.10, 0.28),
                               hand_r=(0.10, -0.25, 0.28), foot_r=(0.20, -1.05, 0.08), foot_r_yaw=150, foot_r_pitch=80)),
                       (14, oc(pelvis_dz=-0.55, pelvis_yaw=160, spine_yaw=-40, spine_pitch=32, hand_l=(0.36, -0.10, 0.28),
                               hand_r=(0.10, -0.25, 0.28), foot_r=(0.35, -0.95, 0.08), foot_r_yaw=160, foot_r_pitch=80)),
                       (38, OC)], 10)
# pulo
O["Otto_PuloInicio"] = ([(0, OG), (4, og(pelvis_dz=-0.24, spine_pitch=20, hand_l=(0.30, -0.25, SH_Z - 0.22))),
                         (6, og(pelvis_dz=-0.02, spine_pitch=0, foot_l_pitch=-30, foot_r_pitch=-30))], None)
O["Otto_PuloAr"] = ([(0, OA), (20, oair(foot_l=(0.20, -0.14, 0.46), foot_r=(-0.20, 0.10, 0.40))), (40, OA)], None)
O["Otto_Aterrissa"] = ([(0, OA), (3, og(pelvis_dz=-0.28, spine_pitch=22)), (12, OG)], None)
# aéreos
O["Otto_AereoSoco"] = ([(0, OA), (2, oair(hand_l=(0.26, -0.26, SH_Z - 0.04))),
                        (4, oair(hand_l=(0.12, -0.30 - RO, SH_Z - 0.26), spine_pitch=22, pelvis_yaw=10)),
                        (9, oair(hand_l=(0.12, -0.28 - RO, SH_Z - 0.26), spine_pitch=22, pelvis_yaw=10)), (15, OA)], 4)
# j.MP: martelo de punho para baixo com a mão de trás
O["Otto_AereoPunho"] = ([(0, OA), (3, oair(spine_pitch=0, spine_yaw=-18, hand_r=(-0.20, -0.10, SH_Z + 0.35), pole_up=0.5)),
                         (6, oair(spine_pitch=30, spine_yaw=8, hand_r=(-0.02, -0.72, SH_Z - 0.32))),
                         (10, oair(spine_pitch=30, spine_yaw=8, hand_r=(-0.02, -0.70, SH_Z - 0.36))), (18, OA)], 6)
# j.HP: marreta de duas mãos no ar
O["Otto_AereoMarreta"] = ([(0, OA), (5, oair(spine_pitch=-12, hand_l=(0.12, 0.0, SH_Z + 0.45),
                                             hand_r=(-0.12, 0.0, SH_Z + 0.45), pole_up=0.8)),
                           (9, oair(spine_pitch=36, hand_l=(0.10, -0.75, SH_Z - 0.38), hand_r=(-0.10, -0.75, SH_Z - 0.38))),
                           (13, oair(spine_pitch=36, hand_l=(0.10, -0.72, SH_Z - 0.42), hand_r=(-0.10, -0.72, SH_Z - 0.42))),
                           (23, OA)], 9)
# j.LK: joelhada
O["Otto_AereoJoelho"] = ([(0, OA), (2, oair(foot_l=(0.20, -0.20, 0.40), foot_l_pitch=0)),
                          (4, oair(foot_l=(0.18, -0.38, 0.50), foot_l_pitch=-20, pelvis_yaw=18, spine_pitch=4,
                                   pelvis_shift=(0, -0.05, 0))),
                          (10, oair(foot_l=(0.18, -0.36, 0.50), foot_l_pitch=-20, pelvis_yaw=18, spine_pitch=4,
                                    pelvis_shift=(0, -0.05, 0))), (16, OA)], 4)
O["Otto_AereoChute"] = ([(0, OA), (3, oair(foot_l=(0.20, -0.22, 0.52), foot_l_pitch=40)),
                         (6, oair(foot_l=(0.18, -0.86, 0.18), foot_l_pitch=70, pelvis_yaw=40, spine_pitch=-10)),
                         (10, oair(foot_l=(0.18, -0.82, 0.20), foot_l_pitch=70, pelvis_yaw=40, spine_pitch=-10)),
                         (18, OA)], 6)
# j.HK: pisão de dois pés para baixo e à frente
O["Otto_AereoPisao"] = ([(0, OA), (5, oair(foot_l=(0.18, -0.15, 0.62), foot_r=(-0.18, -0.10, 0.60), foot_l_pitch=10,
                                           foot_r_pitch=10, spine_pitch=-6, hand_l=(0.40, -0.10, SH_Z + 0.15),
                                           hand_r=(-0.40, -0.10, SH_Z + 0.15))),
                         (9, oair(foot_l=(0.16, -0.58, -0.04), foot_r=(-0.16, -0.52, -0.02), foot_l_pitch=40,
                                  foot_r_pitch=40, spine_pitch=-12, hand_l=(0.42, -0.05, SH_Z + 0.20),
                                  hand_r=(-0.42, -0.05, SH_Z + 0.20))),
                         (14, oair(foot_l=(0.16, -0.56, -0.02), foot_r=(-0.16, -0.50, 0.0), foot_l_pitch=40,
                                   foot_r_pitch=40, spine_pitch=-12, hand_l=(0.42, -0.05, SH_Z + 0.20),
                                   hand_r=(-0.42, -0.05, SH_Z + 0.20))),
                         (23, OA)], 9)
# 236P: investida de ombro (o núcleo carrega o deslocamento)
O["Otto_Investida"] = ([(0, OG), (5, og(pelvis_dz=-0.18, spine_pitch=30, spine_yaw=-35, hand_l=(0.10, -0.25, SH_Z - 0.25),
                                        hand_r=(-0.10, -0.15, SH_Z - 0.20), foot_r=(-0.22, 0.40, 0.12), foot_r_pitch=-30)),
                        (9, og(pelvis_dz=-0.16, spine_pitch=28, spine_yaw=-38, hand_l=(0.10, -0.25, SH_Z - 0.25),
                               hand_r=(-0.10, -0.15, SH_Z - 0.20), foot_l=(0.20, -0.45, 0.20), foot_r=(-0.20, 0.30, 0.10))),
                        (13, og(pelvis_dz=-0.14, spine_pitch=22, spine_yaw=-45, pelvis_shift=(0, -0.10, 0),
                                hand_l=(0.30, -0.55, SH_Z - 0.10), hand_r=(-0.10, -0.15, SH_Z - 0.20), pole_up=0.3,
                                foot_l=(0.20, -0.35, 0.10), foot_r=(-0.22, 0.32, 0.10), foot_r_pitch=-25)),
                        (17, og(pelvis_dz=-0.14, spine_pitch=22, spine_yaw=-45, pelvis_shift=(0, -0.10, 0),
                                hand_l=(0.30, -0.53, SH_Z - 0.10), hand_r=(-0.10, -0.15, SH_Z - 0.20), pole_up=0.3,
                                foot_l=(0.20, -0.35, 0.10), foot_r=(-0.22, 0.32, 0.10))), (36, OG)], 13)
# 236PP: investida reforçada, fecha com as duas palmas à frente
O["Otto_InvestidaEX"] = ([(0, OG), (4, og(pelvis_dz=-0.20, spine_pitch=32, spine_yaw=-20, hand_l=(0.20, -0.20, SH_Z - 0.22),
                                          hand_r=(-0.20, -0.15, SH_Z - 0.22), foot_r=(-0.22, 0.42, 0.12),
                                          foot_r_pitch=-30)),
                          (8, og(pelvis_dz=-0.18, spine_pitch=28, spine_yaw=-15, hand_l=(0.20, -0.22, SH_Z - 0.22),
                                 hand_r=(-0.20, -0.18, SH_Z - 0.22), foot_l=(0.20, -0.45, 0.20), foot_r=(-0.20, 0.30, 0.10))),
                          (11, og(pelvis_dz=-0.14, spine_pitch=18, spine_yaw=0, pelvis_shift=(0, -0.12, 0),
                                  hand_l=(0.16, -0.85, SH_Z - 0.02), hand_r=(-0.16, -0.85, SH_Z - 0.02),
                                  foot_l=(0.20, -0.38, 0.10), foot_r=(-0.22, 0.34, 0.10), foot_r_pitch=-25)),
                          (15, og(pelvis_dz=-0.14, spine_pitch=18, spine_yaw=0, pelvis_shift=(0, -0.12, 0),
                                  hand_l=(0.16, -0.83, SH_Z - 0.02), hand_r=(-0.16, -0.83, SH_Z - 0.02),
                                  foot_l=(0.20, -0.38, 0.10), foot_r=(-0.22, 0.34, 0.10))), (32, OG)], 11)
# 214K: pisão que treme o chão (joelho alto, desce com o corpo todo)
O["Otto_Pisao"] = ([(0, OG), (8, og(foot_l=(0.20, -0.25, 0.70), foot_l_pitch=10, spine_pitch=-6, pelvis_dz=-0.04,
                                    hand_l=(0.40, -0.20, SH_Z + 0.05), hand_r=(-0.40, -0.10, SH_Z + 0.05))),
                    (12, og(foot_l=(0.20, -0.25, 0.76), foot_l_pitch=10, spine_pitch=-8, pelvis_dz=-0.03,
                            hand_l=(0.42, -0.20, SH_Z + 0.10), hand_r=(-0.42, -0.10, SH_Z + 0.10))),
                    (15, og(foot_l=(0.22, -0.45, 0.10), foot_l_pitch=0, spine_pitch=30, pelvis_dz=-0.26,
                            hand_l=(0.36, -0.40, SH_Z - 0.35), hand_r=(-0.32, -0.30, SH_Z - 0.35))),
                    (19, og(foot_l=(0.22, -0.45, 0.10), foot_l_pitch=0, spine_pitch=30, pelvis_dz=-0.27,
                            hand_l=(0.36, -0.40, SH_Z - 0.37), hand_r=(-0.32, -0.30, SH_Z - 0.37))), (39, OG)], 15)
# super 236236P: corre de braços abertos, fecha num laço de braço e termina flexionando
O["Otto_Super"] = ([(0, OG), (3, og(pelvis_dz=-0.16, spine_pitch=28, hand_l=(0.60, -0.40, SH_Z), hand_r=(-0.60, -0.36, SH_Z),
                                    foot_r=(-0.22, 0.40, 0.14), foot_r_pitch=-30)),
                    (7, og(pelvis_dz=-0.12, spine_pitch=18, spine_yaw=30, pelvis_yaw=-10, pelvis_shift=(0, -0.12, 0),
                           hand_l=(0.15, -0.85, SH_Z + 0.05), hand_r=(-0.55, -0.30, SH_Z), foot_l=(0.20, -0.40, 0.10))),
                    (11, og(pelvis_dz=-0.12, spine_pitch=18, spine_yaw=34, pelvis_yaw=-10, pelvis_shift=(0, -0.12, 0),
                            hand_l=(0.05, -0.85, SH_Z + 0.05), hand_r=(-0.55, -0.30, SH_Z), foot_l=(0.20, -0.40, 0.10))),
                    (22, og(spine_pitch=14, spine_yaw=60, pelvis_yaw=-20, hand_l=(-0.30, -0.60, SH_Z),
                            hand_r=(-0.50, -0.10, SH_Z - 0.10))),
                    (36, og(pelvis_dz=-0.14, spine_pitch=-6, head_pitch=-10, hand_l=(0.45, -0.20, SH_Z + 0.30),
                            hand_r=(-0.45, -0.20, SH_Z + 0.30), pole_up=-0.4)),
                    (46, og(pelvis_dz=-0.14, spine_pitch=-6, head_pitch=-10, hand_l=(0.45, -0.20, SH_Z + 0.32),
                            hand_r=(-0.45, -0.20, SH_Z + 0.32), pole_up=-0.4)), (56, OG)], 7)
# dash: passada pesada à frente; recuo: salto curto para trás
O["Otto_Dash"] = ([(0, OG), (4, og(pelvis_dz=-0.14, spine_pitch=24, pelvis_shift=(0, -0.08, 0), foot_r=(-0.22, 0.38, 0.16),
                                   foot_r_pitch=-30)),
                   (10, og(pelvis_dz=-0.12, spine_pitch=20, pelvis_shift=(0, -0.05, 0), foot_l=(0.20, -0.40, 0.12))),
                   (20, OG)], None)
O["Otto_Recuo"] = ([(0, OG), (4, og(pelvis_dz=-0.05, spine_pitch=-8, pelvis_shift=(0, 0.10, 0), foot_l=(0.20, -0.28, 0.24))),
                    (12, og(pelvis_dz=-0.16, spine_pitch=8, pelvis_shift=(0, 0.06, 0))), (24, OG)], None)
oq, olie = fall(og)
O["Otto_Queda"] = (oq, None)
O["Otto_Levanta"] = (wake(og, olie), None)


# ================================================================ assar e exportar
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
    for e in list(T.values()) + list(P.values()):
        e.animation_data_clear()
    for pb in arm.pose.bones:
        pb.location = (0, 0, 0)
        pb.rotation_euler = (0, 0, 0)
        pb.rotation_quaternion = (1, 0, 0, 0)
    return act


arm.animation_data_create()
baked = {}
for lib in (N, O):
    for name, (keys, _) in lib.items():
        baked[name] = bake(name, keys)
for b in arm.pose.bones:
    for c in list(b.constraints):
        b.constraints.remove(c)
for e in list(T.values()) + list(P.values()):
    bpy.data.objects.remove(e, do_unlink=True)

os.makedirs(OUT_DIR, exist_ok=True)
contacts = {}
for tag, lib in (("nadja", N), ("otto", O)):
    ad = arm.animation_data
    for t in list(ad.nla_tracks):
        ad.nla_tracks.remove(t)
    for name, (keys, contact) in lib.items():
        tr = ad.nla_tracks.new()
        tr.name = name
        st = tr.strips.new(name, 0, baked[name])
        if hasattr(st, "action_slot") and baked[name].slots:
            st.action_slot = baked[name].slots[0]
        contacts[name] = {"frames": keys[-1][0] + 1, "contact": contact}
    out = os.path.join(OUT_DIR, f"{tag}_golpes.glb")
    bpy.ops.export_scene.gltf(filepath=out, export_format="GLB", export_skins=True, export_animations=True,
                              export_animation_mode="NLA_TRACKS", export_force_sampling=True,
                              export_anim_single_armature=True, export_reset_pose_bones=True, export_morph=False,
                              export_cameras=False, export_lights=False, export_yup=True)
    print("EXPORTED", out, os.path.getsize(out), len(lib))
json.dump(contacts, open(os.path.join(OUT_DIR, "contatos_golpes.json"), "w"), indent=1)

# folha de prévia de perfil: um clipe por linha, poses-chave nas colunas
os.makedirs(PREV, exist_ok=True)
cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
scene.collection.objects.link(cam)
scene.camera = cam
cam.location = (5.0, -0.2, 0.9)
cam.rotation_euler = (math.radians(90), 0, math.radians(90))
cam.data.lens = 38
scene.render.engine = "BLENDER_WORKBENCH"
scene.render.resolution_x, scene.render.resolution_y = 220, 260
ad = arm.animation_data
for t in list(ad.nla_tracks):
    ad.nla_tracks.remove(t)
row = 0
for lib in (N, O):
    for name, (keys, _) in lib.items():
        ad.action = baked[name]
        if baked[name].slots:
            ad.action_slot = baked[name].slots[0]
        for c, (f, _) in enumerate(keys[:6]):
            scene.frame_set(f)
            scene.render.filepath = os.path.join(PREV, f"row{row:02d}_{c}.png")
            bpy.ops.render.render(write_still=True)
        row += 1
print("PREVIEW_ROWS", row)
