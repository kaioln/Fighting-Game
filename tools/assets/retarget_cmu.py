"""Retarget de clipes CMU (BVH) para o esqueleto Quaternius (UAL/UBC) e exporta kicks.glb.

Método: rotação no espaço do mundo. Para cada par (osso fonte S, osso alvo T):
  R_T(t) = R_S(t) * R_S_rest^-1 * Q * R_T_rest
onde Q alinha a direção do osso alvo em repouso com a do osso fonte em repouso (os dois em T-pose).
O clipe é girado para começar olhando para -Y, sem deslocamento horizontal no 1º quadro, e a altura
do quadril é escalada pela razão de comprimento das pernas. Ossos não mapeados ficam no repouso.

Uso: bl.sh --python retarget_cmu.py -- <bvh_dir> <ual.glb> <out kicks.glb> <out preview dir>
"""
import math
import os
import sys

import bpy
from mathutils import Matrix, Quaternion, Vector

argv = sys.argv[sys.argv.index("--") + 1:]
BVH_DIR, UAL, OUT, PREV = argv
FPS_OUT = 30

# nome do clipe -> (arquivo, segundo inicial, segundo final ou None)
CLIPS = {
    "CMU_Kick_74_03": ("74_03", 0.0, None),
    "CMU_Kick_74_05": ("74_05", 0.0, None),
    "CMU_Kick_74_06": ("74_06", 0.0, None),
    "CMU_JumpKick_75_16": ("75_16", 0.0, None),
    "CMU_JumpSpinKick_87_01": ("87_01", 0.0, None),
    "CMU_JumpSpinKick_88_06": ("88_06", 0.0, None),
    "CMU_FallOnFace_90_16": ("90_16", 0.0, None),
    "CMU_RugPullFall_90_18": ("90_18", 0.0, None),
    "CMU_Boxing_13_18": ("13_18", 2.0, 6.0),
}

MAP = {  # alvo: fonte
    "pelvis": "Hips",
    "spine_01": "LowerBack", "spine_02": "Spine", "spine_03": "Spine1",
    "neck_01": "Neck1", "Head": "Head",  # Neck1 (parte de cima): leva a curva do pescoço até a cabeça
    "clavicle_l": "LeftShoulder", "upperarm_l": "LeftArm", "lowerarm_l": "LeftForeArm", "hand_l": "LeftHand",
    "clavicle_r": "RightShoulder", "upperarm_r": "RightArm", "lowerarm_r": "RightForeArm", "hand_r": "RightHand",
    "thigh_l": "LeftUpLeg", "calf_l": "LeftLeg", "foot_l": "LeftFoot", "ball_l": "LeftToeBase",
    "thigh_r": "RightUpLeg", "calf_r": "RightLeg", "foot_r": "RightFoot", "ball_r": "RightToeBase",
}
NO_ALIGN = {"pelvis", "Head", "hand_l", "hand_r", "ball_l", "ball_r"}  # sem correção de direção
ALIGN_FROM = {"hand_l": "lowerarm_l", "hand_r": "lowerarm_r", "ball_l": "foot_l", "ball_r": "foot_r"}

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.render.fps = FPS_OUT

before = set(bpy.data.objects)
bpy.ops.import_scene.gltf(filepath=UAL)
tgt_objs = [o for o in bpy.data.objects if o not in before]
tgt = [o for o in tgt_objs if o.type == "ARMATURE"][0]
if tgt.animation_data:
    tgt.animation_data.action = None
    for t in list(tgt.animation_data.nla_tracks):
        tgt.animation_data.nla_tracks.remove(t)
for a in list(bpy.data.actions):
    bpy.data.actions.remove(a)
tgt_mw = tgt.matrix_world.copy()


def rest_world(arm, name):
    return arm.matrix_world @ arm.data.bones[name].matrix_local


def rest_dir(arm, name):
    b = arm.data.bones[name]
    mw = arm.matrix_world
    return ((mw @ b.tail_local) - (mw @ b.head_local)).normalized()


def leg_len(arm, thigh, toe):
    mw = arm.matrix_world
    return (mw @ arm.data.bones[thigh].head_local).z - (mw @ arm.data.bones[toe].head_local).z


tbones = tgt.data.bones
T_rest = {n: rest_world(tgt, n).to_quaternion() for n in MAP}
order = [b.name for b in tbones]  # pais antes dos filhos


def import_bvh(stem):
    before = set(bpy.data.objects)
    bpy.ops.import_anim.bvh(filepath=os.path.join(BVH_DIR, stem + ".bvh"), global_scale=1.0,
                            use_fps_scale=False, update_scene_fps=False, update_scene_duration=False,
                            rotate_mode="NATIVE", axis_forward="-Z", axis_up="Y")
    return [o for o in bpy.data.objects if o not in before][0]


def build_clip(name, stem, t0, t1):
    src = import_bvh(stem)
    act = src.animation_data.action
    f_first, f_last = act.frame_range
    src_fps = 120.0
    a = int(f_first + t0 * src_fps)
    b = int(f_last if t1 is None else min(f_last, f_first + t1 * src_fps))

    S_rest = {n: rest_world(src, s).to_quaternion() for n, s in MAP.items()}
    Q = {}
    for n, s in MAP.items():
        if n in NO_ALIGN:
            continue
        Q[n] = rest_dir(tgt, n).rotation_difference(rest_dir(src, s))
    for n, p in ALIGN_FROM.items():
        Q[n] = Q[p]
    scale = leg_len(tgt, "thigh_l", "ball_l") / leg_len(src, "LeftUpLeg", "LeftToeBase")

    frames = list(range(a, b + 1, int(src_fps / FPS_OUT)))
    samples = []
    ground = 1e9
    for f in frames:
        scene.frame_set(f)
        pw = {s: (src.matrix_world @ src.pose.bones[s].matrix) for s in set(MAP.values())}
        ground = min(ground, pw["LeftToeBase"].translation.z, pw["RightToeBase"].translation.z)
        samples.append(pw)

    # corta o T-pose de calibração do início: começa quando as duas mãos caem bem abaixo dos ombros
    def arms_down(pw):
        arm = (pw["LeftArm"].translation - pw["LeftHand"].translation).length
        dl = pw["LeftArm"].translation.z - pw["LeftHand"].translation.z
        dr = pw["RightArm"].translation.z - pw["RightHand"].translation.z
        return dl > 0.45 * arm and dr > 0.45 * arm
    start = next((i for i, pw in enumerate(samples) if arms_down(pw)), 0)
    if t0 == 0.0:
        samples = samples[start:]  # a conversão cgspeed põe um T-pose no 1º quadro
        ground = min(min(pw["LeftToeBase"].translation.z, pw["RightToeBase"].translation.z) for pw in samples)

    # orientação inicial: frente do quadril (eixo dos quadris cruzado com o vertical) vira -Y
    h0 = samples[0]
    side = (h0["LeftUpLeg"].translation - h0["RightUpLeg"].translation)
    side.z = 0
    fwd = side.normalized().cross(Vector((0, 0, 1)))  # para onde o ator olha (esquerda x cima)
    yaw = math.atan2(fwd.x, -fwd.y)
    Yfix = Quaternion((0, 0, 1), -yaw)
    p0 = h0["Hips"].translation.copy()

    act_out = bpy.data.actions.new(name)
    tgt.animation_data_create()
    tgt.animation_data.action = act_out
    pel_rest_head = (tgt_mw @ tbones["pelvis"].head_local)
    toe_h = (tgt_mw @ tbones["ball_l"].head_local).z

    pelvis_w = []
    for i, pw in enumerate(samples):
        world = {}
        for n, s in MAP.items():
            rs = Yfix @ pw[s].to_quaternion()
            q = Q.get(n, Quaternion())
            world[n] = rs @ S_rest[n].inverted() @ q @ T_rest[n]
        # quaternions de pose (matrix_basis) dos ossos mapeados, em ordem de hierarquia
        pose_world = {}
        for bn in order:
            bone = tbones[bn]
            rest_w = rest_world(tgt, bn).to_quaternion()
            if bone.parent is None:
                pose_world[bn] = rest_w
                continue
            par_rest_w = rest_world(tgt, bone.parent.name).to_quaternion()
            rel_rest = par_rest_w.inverted() @ rest_w
            par_pose_w = pose_world[bone.parent.name]
            if bn in world:
                pose_world[bn] = world[bn]
                basis = rel_rest.inverted() @ par_pose_w.inverted() @ world[bn]
            else:
                pose_world[bn] = par_pose_w @ rel_rest
                basis = Quaternion()
            pb = tgt.pose.bones[bn]
            pb.rotation_mode = "QUATERNION"
            pb.rotation_quaternion = basis
            pb.keyframe_insert("rotation_quaternion", frame=i + 1)
        # quadril: posição no mundo -> location do osso pelvis (espaço do osso em repouso)
        d = Yfix @ (pw["Hips"].translation - p0)
        z = (pw["Hips"].translation.z - ground) * scale + toe_h
        target_w = Vector((pel_rest_head.x + d.x * scale, pel_rest_head.y + d.y * scale, z))
        pel = tbones["pelvis"]
        par_pose = tgt_mw @ tbones["root"].matrix_local  # raiz não anima
        rel = tbones["root"].matrix_local.inverted() @ pel.matrix_local
        local = (par_pose @ rel).inverted() @ target_w
        pbp = tgt.pose.bones["pelvis"]
        pbp.location = local
        pbp.keyframe_insert("location", frame=i + 1)
        pelvis_w.append(target_w)
    # apoio no chão: a proporção das pernas difere, então mede o pé mais baixo do clipe já retargetado
    # e desce (ou sobe) o quadril para o pé encostar no chão
    lowest = 1e9
    for i in range(len(samples)):
        scene.frame_set(i + 1)
        for b in ("ball_l", "ball_r", "foot_l", "foot_r"):
            lowest = min(lowest, (tgt_mw @ tgt.pose.bones[b].head).z)
    dz = lowest - toe_h
    if abs(dz) > 0.005:
        pel = tbones["pelvis"]
        par_pose = tgt_mw @ tbones["root"].matrix_local
        rel = tbones["root"].matrix_local.inverted() @ pel.matrix_local
        for i, tw in enumerate(pelvis_w):
            pbp.location = (par_pose @ rel).inverted() @ Vector((tw.x, tw.y, tw.z - dz))
            pbp.keyframe_insert("location", frame=i + 1)
    print("GROUND", name, "dz", round(dz, 3))
    tgt.animation_data.action = None
    tr = tgt.animation_data.nla_tracks.new()
    tr.name = name
    st = tr.strips.new(name, 1, act_out)
    if hasattr(st, "action_slot") and act_out.slots:
        st.action_slot = act_out.slots[0]
    bpy.data.objects.remove(src, do_unlink=True)
    return len(samples)


for name, (stem, t0, t1) in CLIPS.items():
    n = build_clip(name, stem, t0, t1)
    print("CLIP", name, n, "frames", round(n / FPS_OUT, 2), "s")

# Remove root motion horizontal residual? mantido (o núcleo decide o deslocamento; ver relatório).
bpy.ops.export_scene.gltf(filepath=OUT, export_format="GLB", export_skins=True, export_animations=True,
                          export_animation_mode="NLA_TRACKS", export_force_sampling=True,
                          export_anim_single_armature=True, export_reset_pose_bones=True,
                          export_optimize_animation_size=True, export_morph=False,
                          export_cameras=False, export_lights=False, export_yup=True)
print("EXPORTED", OUT, os.path.getsize(OUT))

# Prévia: folha de contato de cada clipe (Workbench, 6 quadros por clipe)
os.makedirs(PREV, exist_ok=True)
cam_data = bpy.data.cameras.new("cam")
cam = bpy.data.objects.new("cam", cam_data)
scene.collection.objects.link(cam)
scene.camera = cam
cam.location = (3.2, -3.2, 1.2)
cam.rotation_euler = (math.radians(85), 0, math.radians(45))
cam_data.lens = 35
scene.render.engine = "BLENDER_WORKBENCH"
scene.render.resolution_x, scene.render.resolution_y = 320, 320
scene.render.image_settings.file_format = "PNG"
ad = tgt.animation_data
for tr in ad.nla_tracks:
    tr.mute = True
for tr in ad.nla_tracks:
    tr.mute = False
    st = tr.strips[0]
    n = int(st.frame_end)
    for k in range(6):
        f = 1 + int((n - 1) * k / 5)
        scene.frame_set(f)
        scene.render.filepath = os.path.join(PREV, f"{tr.name}_{k}.png")
        bpy.ops.render.render(write_still=True)
    tr.mute = True
print("PREVIEWS DONE")
