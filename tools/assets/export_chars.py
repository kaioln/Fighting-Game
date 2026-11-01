"""Exporta personagens base e biblioteca de animações (Quaternius, CC0) para GLB.

Uso: bl.sh --python export_chars.py -- <src_dir> <out_assets_dir>
Saídas:
  characters/base_female.glb, characters/base_male.glb   (sem animações, com esqueleto)
  characters/nadja_base.glb, characters/otto_base.glb      (base + cabelo rigado no osso da cabeça)
  animations/ual.glb                                       (UAL 1 + 2 num esqueleto só)
"""
import os
import sys

import bpy

argv = sys.argv[sys.argv.index("--") + 1:]
SRC, OUT = argv[0], argv[1]
UBC = os.path.join(SRC, "Universal_Base_Characters_Standard_", "Universal Base Characters[Standard]")
BODY = os.path.join(UBC, "Base Characters", "Godot - UE")
HAIR = os.path.join(UBC, "Hairstyles", "Rigged to Head Bone", "glTF (Godot -Unreal)")
UAL1 = os.path.join(SRC, "Universal_Animation_Library_Standard_", "Universal Animation Library[Standard]", "Unreal-Godot", "UAL1_Standard.glb")
UAL2 = os.path.join(SRC, "Universal_Animation_Library_2_Standard_", "Universal Animation Library 2[Standard]", "Unreal-Godot", "UAL2_Standard.glb")
MAX_TEX = 1024


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    for a in list(bpy.data.actions):
        bpy.data.actions.remove(a)


def import_gltf(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    return [o for o in bpy.data.objects if o not in before]


def shrink_textures():
    for img in bpy.data.images:
        if img.size[0] > MAX_TEX or img.size[1] > MAX_TEX:
            f = MAX_TEX / max(img.size[0], img.size[1])
            img.scale(max(1, int(img.size[0] * f)), max(1, int(img.size[1] * f)))


def armature_of(objs):
    arms = [o for o in objs if o.type == "ARMATURE"]
    assert len(arms) == 1, arms
    return arms[0]


def attach_hair(arm, hair_name):
    objs = import_gltf(os.path.join(HAIR, hair_name + ".gltf"))
    for o in objs:
        if o.type != "MESH":
            continue
        mw = o.matrix_world.copy()
        o.parent = arm
        o.matrix_world = mw
        for m in o.modifiers:
            if m.type == "ARMATURE":
                m.object = arm
    for o in objs:
        if o.type == "ARMATURE":
            bpy.data.objects.remove(o, do_unlink=True)
    for o in list(bpy.data.objects):
        if o.type == "EMPTY" and not o.children:
            bpy.data.objects.remove(o, do_unlink=True)


def export(path, animations):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    kw = dict(
        filepath=path,
        export_format="GLB",
        export_skins=True,
        export_animations=animations,
        export_morph=False,
        export_cameras=False,
        export_lights=False,
        export_yup=True,
        export_apply=False,
        export_image_format="AUTO",
    )
    if animations:
        kw.update(export_animation_mode="NLA_TRACKS", export_force_sampling=True,
                  export_frame_step=1, export_optimize_animation_size=True,
                  export_anim_single_armature=True, export_reset_pose_bones=True)
    bpy.ops.export_scene.gltf(**kw)
    print("EXPORTED", path, os.path.getsize(path))


def character(body, out_name, hair=()):
    reset()
    objs = import_gltf(os.path.join(BODY, body + ".gltf"))
    arm = armature_of(objs)
    for h in hair:
        attach_hair(arm, h)
    if arm.animation_data:
        arm.animation_data_clear()
    for a in list(bpy.data.actions):
        bpy.data.actions.remove(a)
    shrink_textures()
    export(os.path.join(OUT, "characters", out_name + ".glb"), animations=False)


character("Superhero_Female_FullBody", "base_female")
character("Superhero_Male_FullBody", "base_male")
character("Superhero_Female_FullBody", "nadja_base", hair=("Hair_Buns", "Eyebrows_Female"))
character("Superhero_Male_FullBody", "otto_base", hair=("Hair_Buzzed", "Hair_Beard", "Eyebrows_Regular"))

# Biblioteca de animações: armadura da UAL1 recebe todos os clipes (UAL1 + UAL2) como trilhas NLA.
reset()
objs1 = import_gltf(UAL1)
arm = armature_of(objs1)
acts1 = list(bpy.data.actions)
objs2 = import_gltf(UAL2)
arm2 = armature_of(objs2)
acts2 = [a for a in bpy.data.actions if a not in acts1]
for o in objs2:
    bpy.data.objects.remove(o, do_unlink=True)

if arm.animation_data is None:
    arm.animation_data_create()
ad = arm.animation_data
ad.action = None
for t in list(ad.nla_tracks):
    ad.nla_tracks.remove(t)

seen = set()
names = []
for act in acts1 + acts2:
    name = act.name.split(".")[0] if act.name.split(".")[-1].isdigit() else act.name
    if name in seen:
        print("SKIP duplicate", act.name)
        continue
    seen.add(name)
    act.name = name
    tr = ad.nla_tracks.new()
    tr.name = name
    st = tr.strips.new(name, int(act.frame_range[0]), act)
    if hasattr(st, "action_slot") and act.slots:
        st.action_slot = act.slots[0]
    tr.mute = False
    names.append(name)
print("CLIPS", len(names), ",".join(names))
shrink_textures()
export(os.path.join(OUT, "animations", "ual.glb"), animations=True)
