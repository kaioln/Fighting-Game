"""Roupas low-poly de Nadja e Otto sobre os corpos base Quaternius (CC0), a partir das fichas docs/arte/fichas.

Cada peça é uma casca do próprio corpo: faces escolhidas pelo osso dominante e pela posição, deslocadas ao
longo da normal. Os pesos de skin vêm junto com a malha, e o corpo coberto pela roupa é apagado (com uma folga
de 2 anéis de faces nas bordas). Cores chapadas numa textura de paleta (256x256, células de 32 px), no
estilo dos kits da arena. Capacete (Nadja) e máscara de solda (Otto) também saem de cascas da cabeça.

Uso: bl.sh --python build_outfits.py -- <src_dir> <out_assets_dir> <prev_dir>
"""
import collections
import math
import os
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector

argv = sys.argv[sys.argv.index("--") + 1:]
SRC, OUT, PREV = argv
UBC = os.path.join(SRC, "Universal_Base_Characters_Standard_", "Universal Base Characters[Standard]")
BODY = os.path.join(UBC, "Base Characters", "Godot - UE")
HAIR = os.path.join(UBC, "Hairstyles", "Rigged to Head Bone", "glTF (Godot -Unreal)")
CELL, GRID = 32, 8  # paleta 256x256

TORSO = {"spine_01", "spine_02", "spine_03", "clavicle_l", "clavicle_r"}
LEGS = {"pelvis", "thigh_l", "thigh_r", "calf_l", "calf_r"}
FEET = {"foot_l", "foot_r", "ball_l", "ball_r"}
FINGER_TIPS = ("_03_", "_04_")


def is_finger(b):
    return any(b.startswith(f) for f in ("thumb", "index", "middle", "ring", "pinky"))


# ---------------------------------------------------------------- utilidades
def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def import_gltf(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    return [o for o in bpy.data.objects if o not in before]


def tint_hair(rgb):
    for m in bpy.data.materials:
        if not m.use_nodes or not any(k in m.name for k in ("Hair", "Eyebrow", "Beard")):
            continue
        nt = m.node_tree
        bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
        link = next((l for l in nt.links if l.to_socket == bsdf.inputs["Base Color"]), None)
        if link is None:
            continue
        src = link.from_socket
        nt.links.remove(link)
        mix = nt.nodes.new("ShaderNodeMix")
        mix.data_type, mix.blend_type = "RGBA", "MULTIPLY"
        mix.inputs["Factor"].default_value = 1.0
        nt.links.new(src, mix.inputs["A"])
        mix.inputs["B"].default_value = (*rgb, 1)
        nt.links.new(mix.outputs["Result"], bsdf.inputs["Base Color"])


def attach_hair(arm, name):
    for o in import_gltf(os.path.join(HAIR, name + ".gltf")):
        if o.type == "MESH":
            mw = o.matrix_world.copy()
            o.parent = arm
            o.matrix_world = mw
            for m in o.modifiers:
                if m.type == "ARMATURE":
                    m.object = arm
        elif o.type == "ARMATURE":
            bpy.data.objects.remove(o, do_unlink=True)
    for o in list(bpy.data.objects):
        if o.type == "EMPTY" and not o.children:
            bpy.data.objects.remove(o, do_unlink=True)


class Palette:
    def __init__(self):
        self.colors = []

    def uv(self, rgb):
        rgb = tuple(round(c, 3) for c in rgb)
        if rgb not in self.colors:
            self.colors.append(rgb)
        i = self.colors.index(rgb)
        assert i < GRID * GRID
        cx, cy = i % GRID, i // GRID
        return ((cx + 0.5) / GRID, 1.0 - (cy + 0.5) / GRID)

    def material(self, name, glow=()):
        n = CELL * GRID
        img = bpy.data.images.new(name, n, n, alpha=False)
        img.colorspace_settings.name = "sRGB"
        px = [0.0] * (n * n * 4)
        for i, rgb in enumerate(self.colors):
            cx, cy = i % GRID, i // GRID
            for y in range(CELL):
                row = n - 1 - (cy * CELL + y)
                for x in range(CELL):
                    k = (row * n + cx * CELL + x) * 4
                    px[k:k + 4] = [*rgb, 1.0]
        img.pixels = px
        # grava como PNG e recarrega do arquivo (a imagem gerada chegava preta no glTF)
        path = os.path.join(PREV, name + ".png")
        os.makedirs(PREV, exist_ok=True)
        img.filepath_raw = path
        img.file_format = "PNG"
        img.save()
        img = bpy.data.images.load(path, check_existing=False)
        img.name = name
        emit = None
        if glow:
            # mapa emissivo: só as células de brasa acesas (glTF emissiveTexture; o jogo soma à cor final)
            gl = {tuple(round(c, 3) for c in g) for g in glow}
            epx = [0.0] * (n * n * 4)
            for i, rgb in enumerate(self.colors):
                if rgb not in gl:
                    continue
                cx, cy = i % GRID, i // GRID
                for y in range(CELL):
                    row = n - 1 - (cy * CELL + y)
                    for x in range(CELL):
                        k = (row * n + cx * CELL + x) * 4
                        epx[k:k + 4] = [*rgb, 1.0]
            for k in range(3, len(epx), 4):
                epx[k] = 1.0
            eimg = bpy.data.images.new(name + "_emissao", n, n, alpha=False)
            eimg.pixels = epx
            epath = os.path.join(PREV, name + "_emissao.png")
            eimg.filepath_raw = epath
            eimg.file_format = "PNG"
            eimg.save()
            emit = bpy.data.images.load(epath, check_existing=False)
        m = bpy.data.materials.new(name)
        m.use_nodes = True
        t = m.node_tree.nodes.new("ShaderNodeTexImage")
        t.image = img
        t.interpolation = "Closest"
        bsdf = m.node_tree.nodes["Principled BSDF"]
        m.node_tree.links.new(t.outputs["Color"], bsdf.inputs["Base Color"])
        bsdf.inputs["Roughness"].default_value = 0.8
        if emit is not None:
            et = m.node_tree.nodes.new("ShaderNodeTexImage")
            et.image = emit
            et.interpolation = "Closest"
            m.node_tree.links.new(et.outputs["Color"], bsdf.inputs["Emission Color"])
            bsdf.inputs["Emission Strength"].default_value = 1.0
        return m


def srgb(r, g, b):
    """Cores em sRGB 0-255, como nas fichas (a textura da paleta é sRGB)."""
    return (r / 255.0, g / 255.0, b / 255.0)


# ---------------------------------------------------------------- análise do corpo
class Body:
    def __init__(self, obj, arm):
        self.obj, self.arm = obj, arm
        me = obj.data
        names = [g.name for g in obj.vertex_groups]
        mw = obj.matrix_world
        self.vdom = []
        for v in me.vertices:
            self.vdom.append(names[max(v.groups, key=lambda g: g.weight).group] if v.groups else "")
        self.fdom, self.fc = [], []
        for p in me.polygons:
            c = collections.Counter(self.vdom[i] for i in p.vertices)
            self.fdom.append(c.most_common(1)[0][0])
            self.fc.append(mw @ p.center)
        bm = bmesh.new()
        bm.from_mesh(me)
        bm.faces.ensure_lookup_table()
        self.adj = [[e.other_face(f).index for e in f.edges for _ in [0] if len(e.link_faces) == 2 and e.other_face(f) is not None]
                    for f in bm.faces] if False else self._adj(bm)
        bm.free()
        aw = arm.matrix_world
        self.L = {b.name: aw @ b.head_local for b in arm.data.bones}

    @staticmethod
    def _adj(bm):
        out = []
        for f in bm.faces:
            s = set()
            for e in f.edges:
                for g in e.link_faces:
                    if g is not f:
                        s.add(g.index)
            out.append(sorted(s))
        return out

    def select(self, pred):
        return {i for i in range(len(self.fc)) if pred(self.region(i), self.fc[i])}

    def region(self, i):
        """Região por posição (T-pose): cortes retos em anel em vez da fronteira irregular dos pesos."""
        c, L, b = self.fc[i], self.L, self.fdom[i]
        side = "l" if c.x > 0 else "r"
        if b == "Head" or b == "neck_01" and c.z > L["neck_01"].z + 0.03:
            return "Head"
        if abs(c.x) >= L["hand_l"].x - 0.005:
            return b if is_finger(b) else "hand_" + side
        if abs(c.x) >= L["lowerarm_l"].x:
            return "lowerarm_" + side
        if abs(c.x) >= L["upperarm_l"].x - 0.03 and c.z > L["spine_03"].z - 0.05:
            return "upperarm_" + side
        if c.z >= L["neck_01"].z + 0.01:
            return "neck_01"
        if c.z >= L["spine_03"].z:
            return "spine_03"
        if c.z >= L["spine_02"].z:
            return "spine_02"
        if c.z >= L["spine_01"].z:
            return "spine_01"
        if c.z >= L["thigh_l"].z - 0.02:
            return "pelvis"
        if c.z >= L["calf_l"].z:
            return "thigh_" + side
        if c.z >= L["foot_l"].z + 0.02:
            return "calf_" + side
        return b if b.startswith("ball") else "foot_" + side

    def erode(self, faces, rings):
        cur = set(faces)
        for _ in range(rings):
            cur = {f for f in cur if all(n in faces and n in cur for n in self.adj[f])}
        return cur


def shell(body, name, faces, offset, paint, pal, scale_about=None, xform=None, reweight=None, smooth=0, cuts=()):
    """Cria a casca: duplica o corpo, mantém só `faces`, empurra ao longo da normal e pinta cada face."""
    src = body.obj
    o = src.copy()
    o.data = src.data.copy()
    o.name = o.data.name = name
    bpy.context.scene.collection.objects.link(o)
    me = o.data
    bm = bmesh.new()
    bm.from_mesh(me)
    bm.faces.ensure_lookup_table()
    bm.verts.ensure_lookup_table()
    # desloca com as normais do corpo inteiro (sem abrir nas bordas) ANTES de apagar: depois do delete o
    # índice dos vértices não corresponde mais à lista original
    for v in bm.verts:
        v.co += v.normal * offset
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f.index not in faces], context="FACES")
    mw = src.matrix_world
    mwi = mw.inverted()
    if cuts:
        # bainha reta: cada vértice da borda encosta no plano de corte mais próximo
        border = {v for e in bm.edges if len(e.link_faces) == 1 for v in e.verts}
        for v in border:
            w = mw @ v.co
            best = None
            for ax, val in cuts:
                if ax == "ax":
                    d = abs(abs(w.x) - val)
                else:
                    d = abs(w["xyz".index(ax)] - val)
                if d < 0.05 and (best is None or d < best[0]):
                    best = (d, ax, val)
            if best:
                _, ax, val = best
                if ax == "ax":
                    w.x = val if w.x >= 0 else -val
                else:
                    w["xyz".index(ax)] = val
                v.co = mwi @ w
    if smooth:
        # arredonda (tira os dedos do pé de dentro da bota) e reinfla o volume que a suavização come
        for _ in range(smooth):
            bmesh.ops.smooth_vert(bm, verts=bm.verts, factor=0.5, use_axis_x=True, use_axis_y=True, use_axis_z=True)
        bm.normal_update()
        for v in bm.verts:
            v.co += v.normal * offset * 0.8
    # UV de paleta
    while len(me.uv_layers) > 0:
        me.uv_layers.remove(me.uv_layers[0])
    for layer in list(bm.loops.layers.uv):
        bm.loops.layers.uv.remove(layer)
    uvl = bm.loops.layers.uv.new("UVMap")
    mw = src.matrix_world
    for f in bm.faces:
        c = mw @ f.calc_center_median()
        uv = pal.uv(paint(c, f))
        for lp in f.loops:
            lp[uvl].uv = uv
    if offset > 0.003:
        # anel de bainha: estende a borda para dentro, fechando a fresta entre a roupa e a pele
        bm.normal_update()
        edges = [e for e in bm.edges if len(e.link_faces) == 1]
        if edges:
            nrm = {v: v.normal.copy() for e in edges for v in e.verts}
            res = bmesh.ops.extrude_edge_only(bm, edges=edges)
            for v in res["geom"]:
                if isinstance(v, bmesh.types.BMVert):
                    src_v = next((e.other_vert(v) for e in v.link_edges if e.other_vert(v) in nrm), None)
                    if src_v is not None:
                        v.co -= nrm[src_v] * (offset + 0.003)
    if scale_about is not None:
        center, s = scale_about
        for v in bm.verts:
            v.co = center + (v.co - center) * s
    if xform is not None:
        for v in bm.verts:
            v.co = xform @ v.co
    bm.to_mesh(me)
    bm.free()
    while len(me.uv_layers) > 1:
        me.uv_layers.remove(me.uv_layers[0])
    me.materials.clear()
    if reweight:
        for g in list(o.vertex_groups):
            o.vertex_groups.remove(g)
        g = o.vertex_groups.new(name=reweight)
        g.add(range(len(me.vertices)), 1.0, "REPLACE")
    return o


def delete_body_faces(body, faces):
    me = body.obj.data
    bm = bmesh.new()
    bm.from_mesh(me)
    bm.faces.ensure_lookup_table()
    bmesh.ops.delete(bm, geom=[bm.faces[i] for i in faces], context="FACES_ONLY")
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context="VERTS")
    bm.to_mesh(me)
    bm.free()


def join(objs, name, mat):
    bpy.ops.object.select_all(action="DESELECT")
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join()
    o = bpy.context.view_layer.objects.active
    o.name = o.data.name = name
    o.data.materials.clear()
    o.data.materials.append(mat)
    return o


def tris_of(objs):
    t = 0
    for o in objs:
        o.data.calc_loop_triangles()
        t += len(o.data.loop_triangles)
    return t


# ---------------------------------------------------------------- Nadja
def build_nadja(body, pal):
    L = body.L
    S2, P, A = L["spine_02"].z, L["pelvis"].z, L["foot_l"].z
    K = L["calf_l"].z
    wrist = L["hand_l"].x
    shoulder = L["upperarm_l"].x
    leather = srgb(38, 33, 33)
    leather_hi = srgb(64, 55, 52)
    brick = srgb(132, 44, 34)
    brick_dk = srgb(96, 32, 26)
    yellow = srgb(228, 178, 40)
    top_c = srgb(44, 44, 48)
    belt_c = srgb(70, 46, 30)
    steel = srgb(150, 152, 156)
    wrap = srgb(206, 196, 172)
    wrap_dk = srgb(176, 164, 140)
    boot = srgb(58, 42, 30)
    sole = srgb(30, 26, 24)
    glove = srgb(32, 30, 30)
    black = srgb(18, 18, 20)
    pieces, covered = [], set()

    # top escuro (cropped): tronco acima do umbigo
    top = body.select(lambda b, c: b in TORSO and c.z >= S2 - 0.01)
    up_x = L["upperarm_l"].x - 0.03
    N = L["neck_01"].z
    pieces.append(shell(body, "top", top, 0.004, lambda c, f: top_c, pal,
                        cuts=(("z", S2 - 0.01), ("z", N + 0.01), ("ax", up_x))))
    covered |= top

    # jaqueta: aberta na frente, manga longa na esquerda, ombro cortado na direita
    def jacket_pred(b, c):
        if b in TORSO and c.z >= S2 + 0.05:
            return not (c.y < -0.03 and abs(c.x) < 0.075 and c.z < L["neck_01"].z - 0.02)
        if b in ("upperarm_l", "lowerarm_l"):
            return c.x < wrist - 0.015
        if b == "upperarm_r":
            return c.x > -(shoulder + 0.07)
        return False
    jacket = body.select(jacket_pred)

    jcuts = (("z", S2 + 0.05), ("ax", 0.075), ("z", N - 0.02), ("z", N + 0.01),
             ("x", wrist - 0.015), ("x", -(shoulder + 0.07)))
    pieces.append(shell(body, "jaqueta", jacket, 0.013, lambda c, f: leather, pal, cuts=jcuts))
    covered |= jacket
    # painel tijolo nas costas (pala), faixa amarela e punho na manga longa, lapela
    yoke = body.select(lambda b, c: b in TORSO and c.y > 0.0 and c.z >= S2 + 0.17)
    pieces.append(shell(body, "jaqueta_pala", yoke, 0.016, lambda c, f: brick, pal,
                        cuts=(("z", S2 + 0.17), ("y", 0.0), ("z", N + 0.01), ("ax", up_x))))
    stripe = body.select(lambda b, c: b in ("upperarm_l", "lowerarm_l") and shoulder + 0.08 <= c.x <= shoulder + 0.12)
    pieces.append(shell(body, "jaqueta_faixa", stripe, 0.016, lambda c, f: yellow, pal,
                        cuts=(("x", shoulder + 0.08), ("x", shoulder + 0.12))))
    cuff = body.select(lambda b, c: b == "lowerarm_l" and wrist - 0.06 <= c.x < wrist - 0.015)
    pieces.append(shell(body, "jaqueta_punho", cuff, 0.016, lambda c, f: brick, pal,
                        cuts=(("x", wrist - 0.06), ("x", wrist - 0.015))))
    lapel = body.select(lambda b, c: b in TORSO and c.y < -0.03 and N - 0.08 <= c.z <= N - 0.02 and 0.075 <= abs(c.x) <= 0.13)
    pieces.append(shell(body, "jaqueta_lapela", lapel, 0.016, lambda c, f: brick, pal,
                        cuts=(("z", N - 0.08), ("z", N - 0.02), ("ax", 0.075), ("ax", 0.13))))

    # cinto
    belt = body.select(lambda b, c: b in ("pelvis", "spine_01") and P + 0.06 <= c.z <= P + 0.12)
    pieces.append(shell(body, "cinto", belt, 0.017, lambda c, f: belt_c, pal,
                        cuts=(("z", P + 0.06), ("z", P + 0.12))))
    buckle = body.select(lambda b, c: b in ("pelvis", "spine_01") and P + 0.065 <= c.z <= P + 0.115 and c.y < -0.05 and abs(c.x) < 0.035)
    pieces.append(shell(body, "fivela", buckle, 0.021, lambda c, f: steel, pal,
                        cuts=(("z", P + 0.065), ("z", P + 0.115), ("ax", 0.035))))

    # calça cargo (até o começo das faixas)
    pants = body.select(lambda b, c: b in LEGS and A + 0.26 <= c.z <= P + 0.12)

    pieces.append(shell(body, "calca", pants, 0.009, lambda c, f: brick, pal,
                        cuts=(("z", P + 0.12), ("z", A + 0.26))))
    covered |= pants

    # faixas nas canelas
    wraps = body.select(lambda b, c: b in ("calf_l", "calf_r") and A + 0.07 <= c.z < A + 0.28)
    pieces.append(shell(body, "faixas", wraps, 0.012, lambda c, f: wrap, pal,
                        cuts=(("z", A + 0.07), ("z", A + 0.28))))
    covered |= wraps

    # botas
    boots = body.select(lambda b, c: b in FEET or (b in ("calf_l", "calf_r") and c.z < A + 0.11))
    pieces.append(shell(body, "botas", boots, 0.020,
                        lambda c, f: sole if c.z < 0.025 else boot, pal, smooth=8, cuts=(("z", A + 0.11),)))
    covered |= boots

    # luvas sem dedo
    gl = body.select(lambda b, c: b in ("hand_l", "hand_r") or b.startswith("thumb_01") or
                     (is_finger(b) and b.endswith(("_01_l", "_01_r"))))
    pieces.append(shell(body, "luvas", gl, 0.004, lambda c, f: glove, pal, cuts=(("ax", wrist - 0.005),)))
    covered |= gl

    # capacete amarelo e preto pendurado no cinto, do lado esquerdo
    H = L["Head"]
    dome = body.select(lambda b, c: b == "Head" and c.z > H.z + 0.07)
    hc = Vector((0, H.y, H.z + 0.10))

    def helmet_paint(c, f):
        if c.y < H.y - 0.06 and c.z < H.z + 0.15:
            return black  # viseira
        if abs(c.x) < 0.018:
            return black  # faixa central
        return yellow
    # pendurado atrás, no cinto: os lutadores giram (não espelham), então só atrás ele aparece de perfil
    # dos dois lados da tela
    hang = (Matrix.Translation((0.0, L["pelvis"].y + 0.135, P + 0.03)) @ Matrix.Rotation(math.radians(-80), 4, "X")
            @ Matrix.Rotation(math.radians(180), 4, "Z") @ Matrix.Translation(-hc))
    pieces.append(shell(body, "capacete", dome, 0.0, helmet_paint, pal, scale_about=(hc, 1.22), xform=hang,
                        reweight="pelvis"))
    return pieces, covered


# ---------------------------------------------------------------- Otto
def build_otto(body, pal):
    L = body.L
    S1, S2, P, A = L["spine_01"].z, L["spine_02"].z, L["pelvis"].z, L["foot_l"].z
    K = L["calf_l"].z
    N = L["neck_01"].z
    elbow = L["lowerarm_l"].x
    tank = srgb(170, 156, 128)
    apron = srgb(92, 58, 34)
    apron_dk = srgb(66, 42, 26)
    rivet = srgb(150, 160, 172)
    pants_c = srgb(58, 70, 88)
    pants_dk = srgb(44, 54, 68)
    belt_c = srgb(60, 40, 26)
    boot = srgb(78, 54, 34)
    toe = srgb(108, 114, 120)
    sole = srgb(30, 26, 24)
    glove = srgb(48, 38, 32)
    ember = srgb(255, 120, 30)
    mask_c = srgb(52, 56, 60)
    glass = srgb(20, 24, 22)
    pieces, covered = [], set()

    # regata: tronco, com alças do avental pintadas nas costas (X) e nos ombros
    tk = body.select(lambda b, c: b in TORSO and c.z >= S1 - 0.04 and
                     not (b.startswith("clavicle") and abs(c.x) > 0.14))

    up_x = L["upperarm_l"].x - 0.03
    pieces.append(shell(body, "regata", tk, 0.005, lambda c, f: tank, pal,
                        cuts=(("z", S1 - 0.04), ("z", N + 0.01), ("ax", up_x))))
    covered |= tk
    # alças do avental: retas, por cima dos ombros e descendo pelas costas
    straps = body.select(lambda b, c: b in TORSO and 0.075 <= abs(c.x) <= 0.12 and (c.y > 0.0 or c.z > N - 0.09)
                         and c.z >= S2 - 0.06)
    pieces.append(shell(body, "alcas", straps, 0.010, lambda c, f: apron_dk, pal,
                        cuts=(("ax", 0.075), ("ax", 0.12), ("z", S2 - 0.06), ("z", N + 0.01))))

    # calça azul-aço
    pants = body.select(lambda b, c: b in LEGS and A + 0.13 <= c.z <= P + 0.13)
    pieces.append(shell(body, "calca", pants, 0.010, lambda c, f: pants_c, pal,
                        cuts=(("z", P + 0.13), ("z", A + 0.13))))
    covered |= pants

    # cinto
    belt = body.select(lambda b, c: b in ("pelvis", "spine_01") and P + 0.07 <= c.z <= P + 0.13)
    pieces.append(shell(body, "cinto", belt, 0.020, lambda c, f: belt_c, pal,
                        cuts=(("z", P + 0.07), ("z", P + 0.13))))

    # avental de couro: frente do peito até abaixo do joelho, com rebites
    ap = body.select(lambda b, c: c.y < -0.01 and (
        (b in TORSO and c.z < N - 0.06 and abs(c.x) < 0.17) or
        (b in ("pelvis", "thigh_l", "thigh_r", "calf_l", "calf_r") and c.z > K - 0.12 and abs(c.x) < 0.17)))

    def apron_paint(c, f):
        for rx, rz in ((0.10, N - 0.10), (-0.10, N - 0.10), (0.14, S1), (-0.14, S1)):
            if abs(c.x - rx) < 0.014 and abs(c.z - rz) < 0.014:
                return rivet
        return apron
    pieces.append(shell(body, "avental", ap, 0.030, apron_paint, pal,
                        cuts=(("ax", 0.17), ("z", N - 0.06), ("z", K - 0.12), ("y", -0.01))))
    pocket = body.select(lambda b, c: c.y < -0.01 and abs(c.x) <= 0.07 and S1 - 0.10 <= c.z <= S1 + 0.02 and
                         b in TORSO | {"pelvis"})
    pieces.append(shell(body, "avental_bolso", pocket, 0.034, lambda c, f: apron_dk, pal,
                        cuts=(("ax", 0.07), ("z", S1 - 0.10), ("z", S1 + 0.02))))

    # luvas longas com brasa nos dedos
    gl = body.select(lambda b, c: b in ("hand_l", "hand_r") or is_finger(b) or
                     (b in ("lowerarm_l", "lowerarm_r") and abs(c.x) > elbow + 0.07))

    def glove_paint(c, f):
        return ember if any(t in b for t in FINGER_TIPS for b in [body.fdom[f.index]] if False) else glove
    # brasa: pontas dos dedos (pintadas por osso) e uma costura no punho
    tips = {i for i in gl if any(t in body.fdom[i] + "_" for t in FINGER_TIPS)}
    seam = set()  # costura de brasa vira peça própria (borda reta), abaixo
    glove_faces = sorted(gl)
    o = shell(body, "luvas", gl, 0.013, lambda c, f: glove, pal, smooth=2, cuts=(("ax", elbow + 0.07),))
    # repinta (a casca preserva a ordem relativa das faces mantidas)
    me = o.data
    uv = me.uv_layers[0].data
    hot = pal.uv(ember)
    for new_i, old_i in enumerate(glove_faces):
        if old_i in tips or old_i in seam:
            for li in me.polygons[new_i].loop_indices:
                uv[li].uv = hot
    pieces.append(o)
    covered |= gl
    band = body.select(lambda b, c: b in ("lowerarm_l", "lowerarm_r") and elbow + 0.11 <= abs(c.x) <= elbow + 0.13)
    pieces.append(shell(body, "luvas_brasa", band, 0.016, lambda c, f: ember, pal,
                        cuts=(("ax", elbow + 0.11), ("ax", elbow + 0.13))))

    # botas com biqueira de aço
    boots = body.select(lambda b, c: b in FEET or (b in ("calf_l", "calf_r") and c.z < A + 0.14))
    pieces.append(shell(body, "botas", boots, 0.024,
                        lambda c, f: sole if c.z < 0.025 else (toe if c.y < L["ball_l"].y - 0.02 else boot), pal,
                        smooth=8, cuts=(("z", A + 0.14),)))
    covered |= boots

    # máscara de solda levantada na testa: casca da frente do rosto, girada para cima
    H = L["Head"]
    face = body.select(lambda b, c: b == "Head" and c.y < H.y - 0.035 and H.z - 0.02 < c.z < H.z + 0.16)
    fc = Vector((0, H.y, H.z + 0.07))

    def mask_paint(c, f):
        if abs(c.x) < 0.045 and H.z + 0.06 < c.z < H.z + 0.10:
            return glass  # visor
        return mask_c
    pivot = Vector((0, H.y + 0.01, H.z + 0.07))
    flip = Matrix.Translation(pivot) @ Matrix.Rotation(math.radians(-75), 4, "X") @ Matrix.Translation(-pivot)
    pieces.append(shell(body, "mascara", face, 0.0, mask_paint, pal, scale_about=(fc, 1.3), xform=flip,
                        reweight="Head"))
    return pieces, covered


GLOW = {"otto": (srgb(255, 120, 30),)}


# ---------------------------------------------------------------- montagem
def build(body_name, out_name, hair, hair_rgb, builder):
    reset()
    objs = import_gltf(os.path.join(BODY, body_name + ".gltf"))
    arm = next(o for o in objs if o.type == "ARMATURE")
    for h in hair:
        attach_hair(arm, h)
    tint_hair(hair_rgb)
    for o in list(bpy.data.objects):
        if o.type == "MESH" and o.name.startswith("Icosphere"):
            bpy.data.objects.remove(o, do_unlink=True)
    body_obj = max((o for o in bpy.data.objects if o.type == "MESH"), key=lambda o: len(o.data.vertices))
    body = Body(body_obj, arm)
    pal = Palette()
    pieces, covered = builder(body, pal)
    mat = pal.material(f"roupa_{out_name}", glow=GLOW.get(out_name, ()))
    outfit = join(pieces, f"roupa_{out_name}", mat)
    delete_body_faces(body, body.erode(covered, 2))
    for img in bpy.data.images:
        if img.size[0] > 1024:
            img.scale(1024, 1024)
    meshes = [o for o in bpy.data.objects if o.type == "MESH"]
    print("TRIS", out_name, tris_of(meshes), {o.name: tris_of([o]) for o in meshes}, "paleta", len(pal.colors))
    path = os.path.join(OUT, "characters", out_name + ".glb")
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", export_skins=True, export_animations=False,
                              export_morph=False, export_cameras=False, export_lights=False, export_yup=True,
                              export_image_format="AUTO")
    print("EXPORTED", path, os.path.getsize(path))
    preview(arm, out_name)


def preview(arm, name):
    """Frente e perfil (a câmera do jogo é lateral)."""
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x, scene.render.resolution_y = 520, 720
    scene.view_settings.view_transform = "Standard"
    world = bpy.data.worlds.new("w")
    scene.world = world
    world.color = (0.35, 0.37, 0.4)
    sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", "SUN"))
    sun.data.energy = 3.0
    sun.rotation_euler = (math.radians(55), 0, math.radians(-30))
    scene.collection.objects.link(sun)
    cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
    cam.data.lens = 50
    scene.collection.objects.link(cam)
    scene.camera = cam
    os.makedirs(PREV, exist_ok=True)
    for tag, loc in (("frente", (0, -5.2, 1.0)), ("perfil", (5.2, 0, 1.0)), ("costas", (0, 5.2, 1.0))):
        cam.location = loc
        d = Vector((0, 0, 0.95)) - Vector(loc)
        cam.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
        scene.render.filepath = os.path.join(PREV, f"{name}_{tag}.png")
        bpy.ops.render.render(write_still=True)


build("Superhero_Female_FullBody", "nadja", ("Hair_Buns",), (0.05, 0.03, 0.025), build_nadja)
build("Superhero_Male_FullBody", "otto", ("Hair_Buzzed", "Hair_Beard"), (0.55, 0.53, 0.50), build_otto)
