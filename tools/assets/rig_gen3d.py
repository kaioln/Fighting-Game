"""Prende um personagem gerado (malha única, pose A, sem esqueleto) no esqueleto Quaternius de 65 ossos.

O esqueleto NÃO muda (as animações da UAL, da CMU e as feitas à mão dependem dele). Passos:
 1. escala a malha para a altura do corpo base e apoia no chão; reduz para ~N triângulos;
 2. mede o ângulo dos braços e o afastamento das pernas da malha (pose A);
 3. posa o esqueleto nessa pose A e congela o corpo base posado;
 4. transfere os pesos do corpo base para a malha nova (superfície mais próxima);
 5. leva a malha à T-pose de repouso (armadura auxiliar com repouso = pose A, posada em T) e prende no
    esqueleto original;
 6. textura: clareia e reduz a 2K, sem iluminação embutida forte (corrige níveis).
Uso: bl.sh --python rig_gen3d.py -- <gen.glb> <body.gltf> <hair.gltf|-> <out.glb> <altura_m> <max_tris> <prev_dir> <nome>
"""
import math
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

GEN, BODY, HAIR, OUT, HEIGHT, MAXT, PREV, NAME = sys.argv[sys.argv.index("--") + 1:]
HEIGHT, MAXT = float(HEIGHT), int(MAXT)

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene


def imp(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    return [o for o in bpy.data.objects if o not in before]


body_objs = imp(BODY)
arm = next(o for o in body_objs if o.type == "ARMATURE")
body = max((o for o in body_objs if o.type == "MESH"), key=lambda o: len(o.data.vertices))
others = [o for o in body_objs if o.type == "MESH" and o is not body]
B = {b.name: arm.matrix_world @ b.head_local for b in arm.data.bones}

gen_objs = imp(GEN)
gen = next(o for o in gen_objs if o.type == "MESH")
# junta tudo da malha gerada num objeto sem pais, com transformações aplicadas
mw = gen.matrix_world.copy()
gen.parent = None
gen.matrix_world = mw
bpy.ops.object.select_all(action="DESELECT")
gen.select_set(True)
bpy.context.view_layer.objects.active = gen
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
for o in gen_objs:
    if o is not gen:
        bpy.data.objects.remove(o, do_unlink=True)

# 1. escala e apoio
vs = np.array([v.co[:] for v in gen.data.vertices])
zmin, zmax = vs[:, 2].min(), vs[:, 2].max()
s = HEIGHT / (zmax - zmin)
cx, cy = (vs[:, 0].max() + vs[:, 0].min()) / 2, np.median(vs[:, 1])
for v in gen.data.vertices:
    v.co = Vector(((v.co.x - cx) * s, (v.co.y - cy) * s + B["pelvis"].y * 0.3, (v.co.z - zmin) * s))
gen.data.update()

# redução
gen.data.calc_loop_triangles()
nt = len(gen.data.loop_triangles)
if nt > MAXT:
    mod = gen.modifiers.new("dec", "DECIMATE")
    mod.ratio = MAXT / nt
    bpy.ops.object.modifier_apply(modifier="dec")
gen.data.calc_loop_triangles()
print("TRIS_GEN", len(gen.data.loop_triangles))

# 2. ângulos da pose A medidos na malha
vs = np.array([v.co[:] for v in gen.data.vertices])
sh = B["upperarm_l"]
hip_z = B["thigh_l"].z
armv = vs[(vs[:, 0] > sh.x + 0.12) & (vs[:, 2] > hip_z - 0.35)]
hand = armv[armv[:, 0] > np.percentile(armv[:, 0], 85)].mean(axis=0) if len(armv) else np.array([sh.x + 0.5, 0, sh.z])
arm_len = (B["hand_l"] - B["upperarm_l"]).length
theta = math.degrees(math.atan2(sh.z - hand[2], hand[0] - sh.x))  # graus abaixo da horizontal
feet = vs[(vs[:, 2] < 0.12) & (vs[:, 0] > 0)]
foot_x = feet[:, 0].mean() if len(feet) else B["foot_l"].x
phi = math.degrees(math.atan2(foot_x - B["thigh_l"].x, B["thigh_l"].z - 0.1))
print("POSE_A theta", round(theta, 1), "phi", round(phi, 1), "hand", np.round(hand, 3))


def set_pose_A(a, sign=1.0):
    """Posa os braços theta graus abaixo da horizontal e as pernas phi graus para fora (em torno de Y do mundo)."""
    for side, sx in (("l", 1), ("r", -1)):
        # rotação em torno de Y do mundo: ângulo positivo leva +X para -Z (braço esquerdo desce)
        for bn, ang in ((f"upperarm_{side}", theta * sx), (f"thigh_{side}", -phi * sx)):
            pb = a.pose.bones[bn]
            pb.rotation_mode = "QUATERNION"
            rest = a.data.bones[bn].matrix_local.to_3x3()
            rw = Matrix.Rotation(math.radians(ang * sign), 3, "Y")
            pb.rotation_quaternion = (rest.inverted() @ rw @ rest).to_quaternion()
    bpy.context.view_layer.update()


# 3. corpo base posado em A, congelado
set_pose_A(arm)
dg = bpy.context.evaluated_depsgraph_get()
bodyA_me = bpy.data.meshes.new_from_object(body.evaluated_get(dg))
bodyA = bpy.data.objects.new("bodyA", bodyA_me)
bodyA.matrix_world = body.matrix_world.copy()
scene.collection.objects.link(bodyA)
for g in body.vertex_groups:
    bodyA.vertex_groups.new(name=g.name)
for v in body.data.vertices:
    for g in v.groups:
        bodyA.vertex_groups[g.group].add([v.index], g.weight, "REPLACE")

# 4. transferência de pesos
for g in body.vertex_groups:
    gen.vertex_groups.new(name=g.name)
dt = gen.modifiers.new("dt", "DATA_TRANSFER")
dt.object = bodyA
dt.use_vert_data = True
dt.data_types_verts = {"VGROUP_WEIGHTS"}
dt.vert_mapping = "POLYINTERP_NEAREST"
dt.layers_vgroup_select_src = "ALL"
dt.layers_vgroup_select_dst = "NAME"
bpy.context.view_layer.objects.active = gen
bpy.ops.object.datalayer_transfer(data_type="VGROUP_WEIGHTS") if False else None
bpy.ops.object.modifier_apply(modifier="dt")

# objetos soltos longe do corpo (ex.: capacete pendurado no quadril) seguem o quadril
bvh_src = bodyA
from mathutils.bvhtree import BVHTree  # noqa: E402

bm = bmesh.new()
bm.from_mesh(bodyA.data)
bm.transform(bodyA.matrix_world)
bm.faces.ensure_lookup_table()
tree = BVHTree.FromBMesh(bm)
face_verts = [[v.index for v in f.verts] for f in bm.faces]
bm.free()
names = [g.name for g in bodyA.vertex_groups]
bdom = []
for v in bodyA.data.vertices:
    bdom.append(names[max(v.groups, key=lambda g: g.weight).group] if v.groups else "")
ARMISH = ("upperarm", "lowerarm", "hand", "thumb", "index", "middle", "ring", "pinky")
pel = gen.vertex_groups["pelvis"]
moved = 0
# capacete da Nadja: pendurado no quadril esquerdo, entre a coxa e a mão (na pose A a mão encosta nele);
# tudo que está nessa caixa e afastado da pele vai inteiro para o quadril
hx0, hx1 = 0.17, float(hand[0]) - 0.08
hz0, hz1 = B["thigh_l"].z - 0.27, B["thigh_l"].z + 0.10
for v in gen.data.vertices:
    if NAME != "nadja":
        break
    if not (hx0 < v.co.x < hx1 and hz0 < v.co.z < hz1):
        continue
    loc, nrm, idx, dist = tree.find_nearest(v.co)
    if dist is None or dist < 0.045:
        continue
    if True:
        for g in list(v.groups):
            gen.vertex_groups[g.group].remove([v.index])
        pel.add([v.index], 1.0, "REPLACE")
        moved += 1
print("LOOSE_TO_PELVIS", moved)

# 5. pose A -> T-pose de repouso: armadura auxiliar com repouso = pose A, posada de volta em T
fit = arm.copy()
fit.data = arm.data.copy()
scene.collection.objects.link(fit)
bpy.ops.object.select_all(action="DESELECT")
fit.select_set(True)
bpy.context.view_layer.objects.active = fit
bpy.ops.object.mode_set(mode="POSE")
bpy.ops.pose.armature_apply(selected=False)
bpy.ops.object.mode_set(mode="OBJECT")
set_pose_A(fit, sign=-1.0)  # gira de volta (inverso) em torno do mesmo eixo
edge_len0 = [(gen.data.vertices[e.vertices[0]].co - gen.data.vertices[e.vertices[1]].co).length for e in gen.data.edges]
am = gen.modifiers.new("toT", "ARMATURE")
am.object = fit
bpy.context.view_layer.objects.active = gen
bpy.ops.object.modifier_apply(modifier="toT")
bpy.data.objects.remove(fit, do_unlink=True)
bpy.data.objects.remove(bodyA, do_unlink=True)

# faces que esticaram na ida para a T-pose ligavam partes que se tocavam na pose A (mão encostada no
# capacete): corta essa película
bm = bmesh.new()
bm.from_mesh(gen.data)
bm.edges.ensure_lookup_table()
stretched = {e.index for e in bm.edges
             if e.calc_length() > 2.5 * max(edge_len0[e.index], 1e-4) and e.calc_length() - edge_len0[e.index] > 0.04}
long_faces = [f for f in bm.faces if any(e.index in stretched for e in f.edges)]
bmesh.ops.delete(bm, geom=long_faces, context="FACES")
# solda vértices coincidentes (a malha vem partida nas costuras de UV) e tira fragmentos soltos
bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0004)
bm.verts.ensure_lookup_table()
seen, small = set(), []
for v in bm.verts:
    if v.index in seen:
        continue
    comp, stack = [], [v]
    seen.add(v.index)
    while stack:
        x = stack.pop()
        comp.append(x)
        for e in x.link_edges:
            y = e.other_vert(x)
            if y.index not in seen:
                seen.add(y.index)
                stack.append(y)
    if len(comp) < 40:
        small.extend(comp)
bmesh.ops.delete(bm, geom=small, context="VERTS")
print("SMALL_PIECES_REMOVED_VERTS", len(small))
bm.to_mesh(gen.data)
bm.free()
print("CUT_STRETCHED", len(long_faces))

# volta o esqueleto original ao repouso e prende a malha nova nele
for pb in arm.pose.bones:
    pb.rotation_quaternion = (1, 0, 0, 0)
    pb.location = (0, 0, 0)
bpy.context.view_layer.update()
gen.parent = arm
gen.matrix_parent_inverse = arm.matrix_world.inverted()
m = gen.modifiers.new("Armature", "ARMATURE")
m.object = arm
gen.name = gen.data.name = NAME
# sai o corpo base e o que vinha com ele (olhos, sobrancelhas): o personagem gerado é uma malha só
for o in [body] + others:
    bpy.data.objects.remove(o, do_unlink=True)
for o in list(bpy.data.objects):
    if o.type == "MESH" and o is not gen:
        bpy.data.objects.remove(o, do_unlink=True)  # ex.: Icosphere auxiliar do pacote

# limita a 4 influências por vértice (raylib) e normaliza
bpy.context.view_layer.objects.active = gen
bpy.ops.object.vertex_group_limit_total(group_select_mode="ALL", limit=4)
bpy.ops.object.vertex_group_normalize_all(group_select_mode="ALL", lock_active=False)

# 6. textura: níveis (clareia sombras embutidas), até 2K
for im in bpy.data.images:
    if im.size[0] == 0:
        continue
    if max(im.size) > 2048:
        im.scale(2048, 2048)
    px = np.array(im.pixels[:], dtype=np.float32).reshape(-1, 4)
    rgb = px[:, :3]
    lo, hi = np.percentile(rgb, 1), np.percentile(rgb, 99.5)
    rgb = np.clip((rgb - lo) / max(hi - lo, 1e-3), 0, 1) ** 0.8  # estica e levanta os médios
    px[:, :3] = rgb
    im.pixels = px.ravel()
    im.update()
for mat in bpy.data.materials:
    if mat.use_nodes:
        b = next((n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED"), None)
        if b:
            b.inputs["Metallic"].default_value = 0.0
            for l in list(mat.node_tree.links):
                if l.to_socket == b.inputs["Metallic"]:
                    mat.node_tree.links.remove(l)

os.makedirs(os.path.dirname(OUT), exist_ok=True)
bpy.ops.export_scene.gltf(filepath=OUT, export_format="GLB", export_skins=True, export_animations=False,
                          export_morph=False, export_cameras=False, export_lights=False, export_yup=True,
                          export_image_format="AUTO")
print("EXPORTED", OUT, os.path.getsize(OUT))
