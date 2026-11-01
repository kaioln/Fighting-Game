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
GAMMA = float(os.environ.get("FG_GAMMA", "0.8"))
BACK_EXP = float(os.environ.get("FG_BACK_EXP", "0.55"))
BACK_GAIN = float(os.environ.get("FG_BACK_GAIN", "1.08"))

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
# normais personalizadas do gerador não acompanham a ida A -> T (ficavam viradas e o jogo sombreava a face de
# preto): saem, e a malha é exportada com sombreamento suave (sem frestas no contorno de casco invertido)
if gen.data.has_custom_normals:
    bpy.ops.mesh.customdata_custom_splitnormals_clear()

# 1. escala e apoio
vs = np.array([v.co[:] for v in gen.data.vertices])
zmin, zmax = vs[:, 2].min(), vs[:, 2].max()
s = HEIGHT / (zmax - zmin)
cx, cy = (vs[:, 0].max() + vs[:, 0].min()) / 2, np.median(vs[:, 1])
for v in gen.data.vertices:
    v.co = Vector(((v.co.x - cx) * s, (v.co.y - cy) * s + B["pelvis"].y * 0.3, (v.co.z - zmin) * s))
gen.data.update()
# solda as costuras de UV antes de reduzir: a malha precisa ser contínua para a vizinhança (rótulos, suavização)
bm = bmesh.new()
bm.from_mesh(gen.data)
bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0004)
bm.to_mesh(gen.data)
bm.free()

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

# 4a. pesos por calor (bone heat) na própria malha gerada, com o esqueleto em repouso = pose A. A transferência
# acima pega a superfície mais próxima do corpo base, que é mais fino: o braço dele atravessa o flanco do Otto
# e o flanco ganhava peso de braço (tiras penduradas, furos nas axilas). O calor anda pela superfície da malha.
# Vértices sem solução de calor ficam com a transferência.
if os.environ.get("FG_WEIGHTS", "heat") == "heat":
    dtw = {v.index: [(gen.vertex_groups[g.group].name, g.weight) for g in v.groups] for v in gen.data.vertices}
    fitA = arm.copy()
    fitA.data = arm.data.copy()
    scene.collection.objects.link(fitA)
    bpy.ops.object.select_all(action="DESELECT")
    fitA.select_set(True)
    bpy.context.view_layer.objects.active = fitA
    bpy.ops.object.mode_set(mode="POSE")
    bpy.ops.pose.armature_apply(selected=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    for g in list(gen.vertex_groups):
        gen.vertex_groups.remove(g)
    bpy.ops.object.select_all(action="DESELECT")
    gen.select_set(True)
    fitA.select_set(True)
    bpy.context.view_layer.objects.active = fitA
    bpy.ops.object.parent_set(type="ARMATURE_AUTO")
    mw = gen.matrix_world.copy()
    gen.parent = None
    gen.matrix_world = mw
    for md in list(gen.modifiers):
        gen.modifiers.remove(md)
    bpy.data.objects.remove(fitA, do_unlink=True)
    for g in body.vertex_groups:
        if g.name not in gen.vertex_groups:
            gen.vertex_groups.new(name=g.name)
    fb = 0
    for v in gen.data.vertices:
        if sum(g.weight for g in v.groups) > 0.01:
            continue
        for n, w in dtw[v.index]:
            gen.vertex_groups[n].add([v.index], w, "REPLACE")
        fb += 1
    print("HEAT_FALLBACK_VERTS", fb)

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

# Otto: o avental (couro rígido) segue o quadril: o painel da frente inteiro (a coxa passa por trás dele num
# chute, sem esticar o couro), as laterais e a barra de trás com um pouco de coxa (inclui as ferramentas).
# A calça larga fica longe da pele do corpo base mas perto do eixo da perna: é perna, não avental.
def leg_axis_dist(p):
    """Distância ao eixo da perna mais próxima (coxa e canela). A calça larga do Otto fica longe da pele do
    corpo base, mas perto do eixo: é perna, não avental."""
    best = 9.0
    for s_ in ("l", "r"):
        for a_, b_ in ((B["thigh_" + s_], B["calf_" + s_]), (B["calf_" + s_], B["foot_" + s_])):
            ab = b_ - a_
            t_ = max(0.0, min(1.0, (p - a_).dot(ab) / ab.length_squared))
            best = min(best, (p - (a_ + ab * t_)).length)
    return best


def vertex_colors(o):
    """Cor média da textura gerada em cada vértice (pelas UVs dos cantos)."""
    im = None
    for m_ in o.data.materials:
        for n_ in (m_.node_tree.nodes if m_ and m_.use_nodes else []):
            if n_.type == "TEX_IMAGE" and n_.image is not None and n_.image.size[0] > 0:
                im = n_.image
                break
        if im is not None:
            break
    uvl_ = o.data.uv_layers.active
    if im is None or uvl_ is None:
        return None
    w_, h_ = im.size
    px_ = np.array(im.pixels[:], dtype=np.float32).reshape(h_, w_, 4)[..., :3]
    acc_ = np.zeros((len(o.data.vertices), 3), dtype=np.float32)
    cnt_ = np.zeros(len(o.data.vertices), dtype=np.float32)
    for lp in o.data.loops:
        u_, v_ = uvl_.data[lp.index].uv
        acc_[lp.vertex_index] += px_[min(h_ - 1, max(0, int(v_ * (h_ - 1)))), min(w_ - 1, max(0, int(u_ * (w_ - 1))))]
        cnt_[lp.vertex_index] += 1
    return acc_ / np.maximum(cnt_, 1)[:, None]


pinned_set = set()
if NAME == "otto":
    # couro do avental (marrom) x calça (jeans escuro): a lateral do avental fica dentro do volume da coxa, então
    # só a distância ao eixo da perna não separa os dois; a cor da textura gerada separa
    vcol_ = vertex_colors(gen)
    n_leather = [0]
    pinned = 0
    for v in gen.data.vertices:
        if not (B["calf_l"].z - 0.10 < v.co.z < B["thigh_l"].z + 0.05 and abs(v.co.x) < 0.50):
            continue
        loc, nrm, idx, dist = tree.find_nearest(v.co)
        front = v.co.y < B["pelvis"].y - 0.14  # painel da frente: rígido, a coxa passa por trás dele
        r_, g_, b_ = vcol_[v.index] if vcol_ is not None else (0, 0, 0)
        leather = r_ > b_ + 0.06 and r_ > g_ and v.co.y < B["pelvis"].y + 0.04
        n_leather[0] += leather
        if dist is None or dist < 0.02 or (not front and not leather and leg_axis_dist(v.co) < 0.17):
            continue
        if sum(g.weight for g in v.groups if gen.vertex_groups[g.group].name.startswith(ARMISH)) > 0.5:
            continue
        side = "thigh_l" if v.co.x > 0 else "thigh_r"
        for g in list(v.groups):
            gen.vertex_groups[g.group].remove([v.index])
        # laterais: quase rígidas também (com 0,4 de coxa a lateral rasgava em lasca no chute alto)
        pel.add([v.index], 1.0 if front else 0.85, "REPLACE")
        if not front:
            gen.vertex_groups[side].add([v.index], 0.15, "REPLACE")
        pinned_set.add(v.index)
        pinned += 1
    print("APRON_PINNED", pinned, "LEATHER", n_leather[0])

# Pernas por segmento: a calça larga fica longe dos ossos e o calor misturava quadril, coxa e canela de um
# jeito que, num chute, a perna inteira virava uma lâmina esticada. Cada vértice da perna segue o osso do
# segmento mais próximo (eixo coxa e canela na pose A), com mistura curta no quadril e no joelho; botas
# (abaixo do tornozelo) ficam com o calor.
J = {n: arm.matrix_world @ arm.pose.bones[n].head for s_ in ("l", "r") for n in ("thigh_" + s_, "calf_" + s_, "foot_" + s_)}
legged = 0
for v in gen.data.vertices:
    if v.index in pinned_set:
        continue
    if sum(g.weight for g in v.groups if gen.vertex_groups[g.group].name.startswith(ARMISH)) > 0.3:
        continue
    best = None
    for s_ in ("l", "r"):
        for seg, (a_, b_) in enumerate(((J["thigh_" + s_], J["calf_" + s_]), (J["calf_" + s_], J["foot_" + s_]))):
            ab = b_ - a_
            t_ = (v.co - a_).dot(ab) / ab.length_squared
            d_ = (v.co - (a_ + ab * max(0.0, min(1.0, t_)))).length
            if best is None or d_ < best[0]:
                best = (d_, s_, seg, t_)
    d_, s_, seg, t_ = best
    if d_ > 0.30 or (seg == 0 and t_ < -0.05) or (seg == 1 and t_ > 1.0):
        continue
    t_ = max(0.0, t_)
    w = {}
    if seg == 0:
        wp = max(0.0, 1.0 - t_ / 0.18) * 0.5   # quadril
        wc = max(0.0, (t_ - 0.85) / 0.30)      # joelho
        w["pelvis"], w["calf_" + s_], w["thigh_" + s_] = wp, wc, 1.0 - wp - wc
    else:
        wt = max(0.0, (0.15 - t_) / 0.30)      # joelho
        wf = max(0.0, (t_ - 0.90) / 0.20)      # tornozelo
        w["thigh_" + s_], w["foot_" + s_], w["calf_" + s_] = wt, wf, 1.0 - wt - wf
    # entre a perna e a lateral presa do avental (Otto): transição gradual, senão a borda rasga em lasca
    if NAME == "otto" and d_ > 0.11 and B["calf_l"].z - 0.10 < v.co.z < B["thigh_l"].z + 0.05:
        bl = min(1.0, (d_ - 0.11) / 0.06) * 0.7
        w = {n: x * (1 - bl) for n, x in w.items()}
        w["pelvis"] = w.get("pelvis", 0.0) + 0.85 * bl
        w["thigh_" + s_] = w.get("thigh_" + s_, 0.0) + 0.15 * bl
    for g in list(v.groups):
        gen.vertex_groups[g.group].remove([v.index])
    for n, x in w.items():
        if x > 0.0:
            gen.vertex_groups[n].add([v.index], x, "REPLACE")
    legged += 1
print("LEG_SEGMENT_WEIGHTS", legged)

# Otto: o avental vira peça própria. O gerador funde o couro com a coxa: qualquer peso na emenda rasgava em
# lasca no chute alto. As faces do avental (todos os vértices presos ao quadril acima) são descoladas da
# perna; o furo que fica na perna é tampado; o avental continua com os pesos de quadril.
if NAME == "otto" and pinned_set:
    bm = bmesh.new()
    bm.from_mesh(gen.data)
    flay = bm.faces.layers.int.new("avental")  # antes de coletar faces: criar camada invalida as referências
    bm.verts.ensure_lookup_table()
    z0_, z1_ = B["calf_l"].z - 0.10, B["thigh_l"].z + 0.05

    def front_panel(f):
        c_ = f.calc_center_median()
        return z0_ < c_.z < z1_ and abs(c_.x) < 0.40 and c_.y < B["pelvis"].y - 0.12

    sel = {f for f in bm.faces if all(v.index in pinned_set for v in f.verts) or front_panel(f)}
    # a placa do avental tem espessura e face de dentro: a seleção cresce pela malha até elas, sem entrar na
    # superfície da perna (perto do eixo)
    for _ in range(6):
        add = set()
        for f in sel:
            for e in f.edges:
                for g in e.link_faces:
                    if g in sel or g in add:
                        continue
                    c_ = g.calc_center_median()
                    if z0_ - 0.04 < c_.z < z1_ and c_.y < B["pelvis"].y + 0.02 and leg_axis_dist(c_) > 0.13:
                        add.add(g)
        if not add:
            break
        sel |= add
    # recorte sem serrilha e sem janelas: face com 2+ vizinhas do avental entra, com 1 ou nenhuma sai
    for _ in range(4):
        nb = {f: sum(1 for e in f.edges for g in e.link_faces if g is not f and g in sel) for f in bm.faces}
        sel = {f for f in bm.faces if nb[f] >= 2}
    apron = list(sel)
    for f in apron:
        f[flay] = 1
    bmesh.ops.split(bm, geom=apron, use_only_faces=True)
    # pesos do avental separado: quadril, com um pouco da coxa do mesmo lado crescendo do centro (0) para as
    # bordas (35%) e para baixo. Rígido de todo, a coxa da frente atravessava o couro já na guarda; solto, ele
    # dobra sem rasgar porque não está mais costurado na perna.
    dl = bm.verts.layers.deform.verify()
    gi = {g.name: g.index for g in gen.vertex_groups}
    for v in {v for f in bm.faces if f[flay] for v in f.verts}:
        v[dl].clear()
        t_ = min(1.0, abs(v.co.x) / 0.20) * min(1.0, max(0.0, (z1_ - v.co.z) / 0.25))
        v[dl][gi["pelvis"]] = 1.0 - 0.35 * t_
        if t_ > 0:
            v[dl][gi["thigh_l" if v.co.x > 0 else "thigh_r"]] = 0.35 * t_
    # tampa só os furos do lado do corpo (bordas cujas faces não são do avental)
    bm.edges.ensure_lookup_table()
    body_bnd = [e for e in bm.edges if e.is_boundary and not any(f[flay] for f in e.link_faces)]
    caps = bmesh.ops.holes_fill(bm, edges=body_bnd, sides=600)["faces"]
    uvl_bm = bm.loops.layers.uv.active
    cset = set(caps)
    for f in caps:
        for lp in f.loops:
            other = next((x for x in lp.vert.link_loops if x.face not in cset), None)
            if other is not None and uvl_bm is not None:
                lp[uvl_bm].uv = other[uvl_bm].uv
    if caps:
        bmesh.ops.triangulate(bm, faces=caps, quad_method="BEAUTY", ngon_method="BEAUTY")
    bm.to_mesh(gen.data)
    bm.free()
    gen.data.update()
    print("APRON_SPLIT caps", len(caps))

# 4b. limpeza dos pesos (malha contínua, pose A)
gnames = [g.name for g in gen.vertex_groups]
NV, NG = len(gen.data.vertices), len(gnames)
W = np.zeros((NV, NG), dtype=np.float32)
for v in gen.data.vertices:
    for g in v.groups:
        W[v.index, g.group] = g.weight
is_arm = np.array([n.startswith(ARMISH) for n in gnames])
ev = np.array([e.vertices[:] for e in gen.data.edges])
nbr = [[] for _ in range(NV)]
for a_, b_ in ev:
    nbr[a_].append(b_)
    nbr[b_].append(a_)

# a) o ombro do esqueleto (corpo base) fica dentro do deltoide largo do personagem gerado: o calor dava peso de
# braço a meio peito e às costas. Para dentro do ombro, o peso de braço some aos poucos e vai para o tronco.
co = np.array([v.co[:] for v in gen.data.vertices])
shx = B["upperarm_l"].x
ramp = np.clip((np.abs(co[:, 0]) - (shx - 0.10)) / 0.12, 0, 1)
ramp[co[:, 2] < hip_z] = 1.0  # abaixo do quadril só as mãos chegam perto: não mexe
cut = W[:, is_arm] * (1 - ramp[:, None])
W[:, is_arm] -= cut
W[:, gnames.index("spine_03")] += cut.sum(axis=1)
print("ARM_TO_TORSO", int((cut.sum(axis=1) > 0.05).sum()))

# b) rótulo grosso (braço E/D, tronco, quadril, perna E/D): ilhas pequenas de um rótulo cercadas por outro
# (tiras soltas) passam para o rótulo da vizinhança, com a média dos pesos dos vizinhos
def coarse(n):
    if n.startswith(ARMISH):
        return 1 if n.endswith("_l") else 2
    if n.startswith(("thigh", "calf", "foot", "ball")):
        return 3 if n.endswith("_l") else 4
    return 0


cg = np.array([coarse(n) for n in gnames])
lab = cg[W.argmax(axis=1)]
for it in range(6):
    changed = 0
    for i in range(NV):
        nb = nbr[i]
        if not nb:
            continue
        ln = lab[nb]
        vals, cnt = np.unique(ln, return_counts=True)
        k = cnt.argmax()
        if vals[k] != lab[i] and cnt[k] >= 0.7 * len(nb):
            sel = [j for j in nb if lab[j] == vals[k]]
            W[i] = W[sel].mean(axis=0)
            lab[i] = vals[k]
            changed += 1
    if not changed:
        break
print("LABEL_FIX", it, changed)

# c) suavização laplaciana dos pesos (some com as pontas isoladas)
deg = np.array([max(len(n), 1) for n in nbr], dtype=np.float32)
src = np.repeat(np.arange(NV), [len(n) for n in nbr])
dst = np.concatenate([np.array(n, dtype=np.int64) for n in nbr if n]) if NV else np.zeros(0, np.int64)
for it in range(4):
    acc = np.zeros_like(W)
    np.add.at(acc, src, W[dst])
    W = 0.5 * W + 0.5 * acc / deg[:, None]
W[W < 0.01] = 0
W /= np.maximum(W.sum(axis=1, keepdims=True), 1e-6)
gen_lab = cg[W.argmax(axis=1)]
for g in list(gen.vertex_groups):
    gen.vertex_groups.remove(g)
for n in gnames:
    gen.vertex_groups.new(name=n)
for j in range(NG):
    idx = np.nonzero(W[:, j] > 0)[0]
    vg = gen.vertex_groups[j]
    for i in idx:
        vg.add([int(i)], float(W[i, j]), "REPLACE")

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
bm.verts.ensure_lookup_table()
lay = bm.verts.layers.int.new("lab")
for v in bm.verts:
    v[lay] = int(gen_lab[v.index])
bm.edges.ensure_lookup_table()
stretched = {e.index for e in bm.edges
             if e.calc_length() > 2.5 * max(edge_len0[e.index], 1e-4) and e.calc_length() - edge_len0[e.index] > 0.04}
long_faces = [f for f in bm.faces if any(e.index in stretched for e in f.edges)]
bmesh.ops.delete(bm, geom=long_faces, context="FACES")
# película que sobrou presa ao braço (na pose A a mão encostava no quadril): na T-pose o braço é horizontal,
# então o que pende bem abaixo dele é resto de pele esticada
arm_z = B["upperarm_l"].z
hang_dz = 0.10 if NAME == "nadja" else 0.16  # raio do antebraço (luvas grossas do Otto)
hang = [v for v in bm.verts if v[lay] in (1, 2) and abs(v.co.x) > B["upperarm_l"].x + 0.15 and v.co.z < arm_z - hang_dz]
bmesh.ops.delete(bm, geom=hang, context="VERTS")
print("HANGING_REMOVED", len(hang))
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
# fecha os furos que o corte deixou (axilas). Tampa que juntaria partes diferentes (braço e tronco, as duas
# pernas) refaria a película: fica de fora. UV da tampa = UV que o vértice já tem em outra face.
bm.edges.ensure_lookup_table()
flay = bm.faces.layers.int.get("avental")
filled = bmesh.ops.holes_fill(bm, edges=[e for e in bm.edges if e.is_boundary and not (
    flay is not None and any(f[flay] for f in e.link_faces))], sides=160)["faces"]  # a borda do avental fica aberta
mixed = []
for f in filled:
    labs = [v[lay] for v in f.verts]
    if len(labs) > 8 and max(labs.count(x) for x in set(labs)) < 0.85 * len(labs):  # furinho: fecha sempre
        mixed.append(f)
bmesh.ops.delete(bm, geom=mixed, context="FACES_ONLY")
filled = [f for f in filled if f.is_valid]
uvl_bm = bm.loops.layers.uv.active
fset = set(filled)
for f in filled:
    for lp in f.loops:
        other = next((x for x in lp.vert.link_loops if x.face not in fset), None)
        if other is not None and uvl_bm is not None:
            lp[uvl_bm].uv = other[uvl_bm].uv
if filled:
    bmesh.ops.triangulate(bm, faces=filled, quad_method="BEAUTY", ngon_method="BEAUTY")
print("HOLES_FILLED", len(filled), "SKIPPED_MIXED", len(mixed))
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

for poly in gen.data.polygons:
    poly.use_smooth = True
# limita a 4 influências por vértice (raylib) e normaliza
bpy.context.view_layer.objects.active = gen
bpy.ops.object.vertex_group_limit_total(group_select_mode="ALL", limit=4)
bpy.ops.object.vertex_group_normalize_all(group_select_mode="ALL", lock_active=False)

# 6a. costas escuras: o gerador só via a frente; marca no UV os triângulos virados para trás (+Y)
back_mask = None
uvl = gen.data.uv_layers.active
img0 = next((im for im in bpy.data.images if im.size[0] > 0), None)
if uvl is not None and img0 is not None:
    W, H = img0.size
    if max(W, H) > 2048:
        W = H = 2048
    back_mask = np.zeros((H, W), dtype=np.float32)
    gen.data.calc_loop_triangles()
    for tri in gen.data.loop_triangles:
        ny = tri.normal.y
        if ny < 0.15:
            continue
        uvs = np.array([uvl.data[li].uv[:] for li in tri.loops]) * [W - 1, H - 1]
        x0, y0 = np.floor(uvs.min(axis=0)).astype(int)
        x1, y1 = np.ceil(uvs.max(axis=0)).astype(int)
        if x1 - x0 > 200 or y1 - y0 > 200:
            continue
        xs, ys = np.meshgrid(np.arange(x0, x1 + 1), np.arange(y0, y1 + 1))
        a, b, c = uvs
        d = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1])
        if abs(d) < 1e-9:
            continue
        l1 = ((b[1] - c[1]) * (xs - c[0]) + (c[0] - b[0]) * (ys - c[1])) / d
        l2 = ((c[1] - a[1]) * (xs - c[0]) + (a[0] - c[0]) * (ys - c[1])) / d
        inside = (l1 >= -0.02) & (l2 >= -0.02) & (1 - l1 - l2 >= -0.02)
        xs, ys = xs[inside], ys[inside]
        ok = (xs >= 0) & (xs < W) & (ys >= 0) & (ys < H)
        back_mask[ys[ok], xs[ok]] = np.maximum(back_mask[ys[ok], xs[ok]], min(1.0, ny * 1.4))
    print("BACK_MASK_TEXELS", int((back_mask > 0).sum()))

# 6. textura: níveis (clareia sombras embutidas), até 2K
for im in bpy.data.images:
    if im.size[0] == 0:
        continue
    if max(im.size) > 2048:
        im.scale(2048, 2048)
    px = np.array(im.pixels[:], dtype=np.float32).reshape(-1, 4)
    rgb = px[:, :3]
    lo, hi = np.percentile(rgb, 1), np.percentile(rgb, 99.5)
    rgb = np.clip((rgb - lo) / max(hi - lo, 1e-3), 0, 1) ** GAMMA  # estica e levanta os médios
    if back_mask is not None and back_mask.shape == (im.size[1], im.size[0]):
        m = back_mask.reshape(-1, 1)
        rgb = rgb * (1 - m) + (rgb ** BACK_EXP * BACK_GAIN).clip(0, 1) * m  # costas: levanta as sombras
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
bpy.ops.object.select_all(action="DESELECT")
arm.select_set(True)
gen.select_set(True)
bpy.ops.export_scene.gltf(filepath=OUT, export_format="GLB", export_skins=True, export_animations=False, use_selection=True,
                          export_morph=False, export_cameras=False, export_lights=False, export_yup=True,
                          export_image_format="AUTO")
print("EXPORTED", OUT, os.path.getsize(OUT))
