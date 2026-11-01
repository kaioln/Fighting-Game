"""Arena "Dique Seco 7" no estilo pintado (Arcane/Fortnite): um estilo só, sem nada da Kenney.

- Modelos: Quaternius (CC0, via Poly Pizza), com texturas pintadas à mão geradas para o projeto
  (concreto, metal de contêiner, madeira) aplicadas por projeção de caixa em escala de mundo.
- Fundo em camadas com paralaxe real (profundidade 3D): pintura do porto a 100 m e silhuetas
  recortadas (contêineres, guindastes, galpão) a 45-55 m.
- Faixa de luta livre: x em [-7,5, 7,5], y em [-1,5, 1,5] (Blender; glTF z = -y). Paredes de contêineres
  com a face interna em x = ±7,5.

Uso: bl.sh --python build_arena_estilo.py -- <dir_pintados> <dir_polypizza> <out.glb> <preview.png>
"""
import math
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Vector

PAINT, PP, OUT, PREVIEW = sys.argv[sys.argv.index("--") + 1:]
MAX_VERTS = 60000

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
placed = []


# ---------------------------------------------------------------- materiais
def img(name):
    return bpy.data.images.load(os.path.join(PAINT, name))


def painted(name, image, tile, tint=(1, 1, 1)):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    t = nt.nodes.new("ShaderNodeTexImage")
    t.image = image
    mix = nt.nodes.new("ShaderNodeMix")
    mix.data_type, mix.blend_type = "RGBA", "MULTIPLY"
    mix.inputs["Factor"].default_value = 1.0
    nt.links.new(t.outputs["Color"], mix.inputs["A"])
    mix.inputs["B"].default_value = (*tint, 1)
    nt.links.new(mix.outputs["Result"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.85
    m["tile_m"] = tile
    return m


def flat(name, rgb):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    m.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (*rgb, 1)
    return m


def keyed(name, src, out_png):
    """Recorta o fundo magenta (#FF00FF) e tira o reflexo magenta das bordas."""
    im = bpy.data.images.load(os.path.join(PAINT, src))
    w, h = im.size
    px = np.array(im.pixels[:], dtype=np.float32).reshape(h, w, 4)
    r, g, b = px[..., 0], px[..., 1], px[..., 2]
    d = np.sqrt((r - 1.0) ** 2 + g ** 2 + (b - 1.0) ** 2)
    alpha = (d > 0.55).astype(np.float32)
    spill = np.clip(np.minimum(r, b) - g, 0, 1)  # excesso de magenta nas bordas
    px[..., 0] = r - spill * 0.8
    px[..., 2] = b - spill * 0.8
    px[..., 3] = alpha
    o = bpy.data.images.new(name, w, h, alpha=True)
    o.pixels = px.ravel()
    o.filepath_raw = out_png
    o.file_format = "PNG"
    o.save()
    o2 = bpy.data.images.load(out_png, check_existing=False)
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    t = nt.nodes.new("ShaderNodeTexImage")
    t.image = o2
    bsdf = nt.nodes["Principled BSDF"]
    nt.links.new(t.outputs["Color"], bsdf.inputs["Base Color"])
    nt.links.new(t.outputs["Alpha"], bsdf.inputs["Alpha"])
    m.blend_method = "CLIP" if hasattr(m, "blend_method") else None
    return m, w, h


concrete = painted("concreto_pintado", img("tex_concreto.png"), 6.0)
quay_edge = painted("cais_borda_pintada", img("tex_concreto.png"), 4.0, (0.70, 0.68, 0.64))
metal_img = img("tex_metal.png")
cont_tints = {
    "ferrugem": (0.62, 0.27, 0.19),
    "petroleo": (0.24, 0.44, 0.45),
    "ocre": (0.72, 0.52, 0.22),
}
cont_mat = {k: painted("conteiner_" + k, metal_img, 2.4, v) for k, v in cont_tints.items()}
frame_mat = painted("conteiner_moldura", metal_img, 1.2, (0.30, 0.32, 0.34))
wood = painted("madeira_pintada", img("tex_madeira.png"), 1.6)
water = flat("agua", (0.11, 0.20, 0.22))


# ---------------------------------------------------------------- utilidades
def box(name, mn, mx, mat):
    me = bpy.data.meshes.new(name)
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    for v in bm.verts:
        v.co = Vector((mn[i] + (v.co[i] + 0.5) * (mx[i] - mn[i]) for i in range(3)))
    bm.to_mesh(me)
    bm.free()
    o = bpy.data.objects.new(name, me)
    o.data.materials.append(mat)
    o["boxuv"] = mat.get("tile_m", 50.0)
    scene.collection.objects.link(o)
    placed.append(o)
    return o


def place(pid, x, y, z=0.0, rot=0.0, s=1.0, remap=None):
    """Modelo do Poly Pizza; remap: {nome_do_material_original: material_pintado}."""
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(PP, pid + ".glb"))
    new = [o for o in bpy.data.objects if o not in before]
    root = bpy.data.objects.new(pid + "_root", None)
    scene.collection.objects.link(root)
    for o in new:
        if o.parent is None:
            o.parent = root
        if o.type == "MESH":
            for slot in o.material_slots:
                if slot.material and remap:
                    base = slot.material.name.split(".")[0]
                    if base in remap:
                        slot.material = remap[base]
                        o["boxuv"] = remap[base].get("tile_m", 2.0)
            placed.append(o)
    root.scale = (s, s, s)
    root.rotation_euler = (0, 0, math.radians(rot))
    root.location = (x, y, z)
    return root


def panel(name, mat, w_m, h_px, w_px, ground_row, y, x0, z_ground):
    """Painel vertical virado para a câmera (-Y). ground_row: linha da imagem que fica em z_ground."""
    px_m = w_px / w_m
    top = ground_row / px_m
    bot = (h_px - ground_row) / px_m
    me = bpy.data.meshes.new(name)
    me.from_pydata([(x0 - w_m / 2, y, z_ground - bot), (x0 + w_m / 2, y, z_ground - bot),
                    (x0 + w_m / 2, y, z_ground + top), (x0 - w_m / 2, y, z_ground + top)], [], [(0, 1, 2, 3)])
    uv = me.uv_layers.new(name="UVMap")
    for li, vi in enumerate(me.polygons[0].vertices):
        uv.data[li].uv = ((0, 0), (1, 0), (1, 1), (0, 1))[vi]
    o = bpy.data.objects.new(name, me)
    o.data.materials.append(mat)
    scene.collection.objects.link(o)
    placed.append(o)


# ---------------------------------------------------------------- cais
box("cais_piso", (-60, -12, -0.4), (60, 12, 0.0), concrete)
box("cais_borda", (-60, 12, -1.6), (60, 12.5, 0.0), quay_edge)
box("agua", (-200, 12.5, -0.7), (200, 140, -0.6), water)

# paredes do palco: contêineres Quaternius (20 pés) com a face interna em x = ±7,5
CS = 1.3
C_W, C_H = 2.09 * CS, 2.13 * CS


def container(kind, x, y, z, rot=90):
    return place("dQXRtm5GbO", x, y, z, rot=rot, s=CS, remap={"Red": cont_mat[kind], "Grey": frame_mat})


order = ["ferrugem", "petroleo", "ocre"]
for side in (-1, 1):
    cx = side * (7.5 + C_W / 2)
    for i, y in enumerate((-4.6, 1.6, 7.8)):
        for level in range(3 if i == 1 else 2):
            container(order[(i + level + (side > 0)) % 3], cx, y, level * C_H)
    for y in (-1.5, 4.7):
        container(order[(side > 0) + 1], cx + side * C_W * 1.05, y, 0)

# carga baixa atrás da faixa de luta (não tapa os lutadores)
wood_map = {"Wood": wood, "DarkWood": wood, "LightWood": wood}
place("3VGWnZPXmG", -5.6, 3.8, rot=12, s=1.2, remap=wood_map)
place("3VGWnZPXmG", -4.6, 4.1, rot=-6, s=1.2, remap=wood_map)
place("3VGWnZPXmG", -5.1, 3.9, z=0.98, rot=30, s=1.1, remap=wood_map)
place("cUAsYHDqfD", -3.4, 5.4, rot=8, s=0.75, remap=wood_map)
place("cUAsYHDqfD", -3.4, 5.4, z=0.15, rot=14, s=0.75, remap=wood_map)
place("Gp43iUyyAX", 2.4, 5.8, rot=-20, s=0.75, remap=wood_map)
place("MraIiFnpAY", 4.9, 3.6, s=1.0)
place("MraIiFnpAY", 5.6, 3.9, rot=40, s=1.0)
place("MraIiFnpAY", 5.2, 4.6, rot=80, s=1.0)
place("ONdghDBByN", 3.9, 6.6, rot=10, s=0.9, remap={"Wood": wood, "DarkWood": wood})
# (pneus removidos: material preto chapado destoava do estilo pintado)

# borda do cais: postes de luz, hidrante, âncora e cais de madeira com postes de amarração
for x in (-12.5, 12.5):
    place("nFwrlcLvM5", x, 11.6, rot=180, s=1.0)
place("DKkMQbEklp", -2.8, 11.3, rot=20, s=1.0)
place("ejWQny9Gcz", 2.6, 11.4, rot=-30, s=1.6)
place("MBZUZAOWew", 9.5, 13.6, z=-0.7, rot=0, s=1.1)
place("XndOrGa7rN", -18.0, 17.0, z=-0.9, rot=90, s=0.55, remap=wood_map)

# ---------------------------------------------------------------- fundo pintado em camadas
far = bpy.data.materials.new("fundo_porto")
far.use_nodes = True
ft = far.node_tree.nodes.new("ShaderNodeTexImage")
ft.image = img("fundo_a.png")
far.node_tree.links.new(ft.outputs["Color"], far.node_tree.nodes["Principled BSDF"].inputs["Base Color"])
panel("fundo_porto", far, 150.0, 608, 1088, 484, 105.0, 0.0, -0.6)

mid_png = os.path.join(os.path.dirname(OUT), "_meio_recortado.png")
mid, mw_px, mh_px = keyed("fundo_meio", "meio_a.png", mid_png)
panel("fundo_meio_esq", mid, 62.0, mh_px, mw_px, 548, 62.0, -38.0, -0.6)
panel("fundo_meio_dir", mid, 56.0, mh_px, mw_px, 548, 70.0, 40.0, -0.6)

# ---------------------------------------------------------------- limpeza e UV de caixa
bpy.context.view_layer.update()
for o in list(bpy.data.objects):
    if o.type == "MESH" and o.parent is not None:
        mw = o.matrix_world.copy()
        o.parent = None
        o.matrix_world = mw
for o in list(bpy.data.objects):
    if o.type == "EMPTY":
        bpy.data.objects.remove(o, do_unlink=True)
meshes = [o for o in bpy.data.objects if o.type == "MESH"]
for o in meshes:
    if o.data.users > 1:
        o.data = o.data.copy()
bpy.ops.object.select_all(action="DESELECT")
for o in meshes:
    o.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

for o in meshes:
    t = o.get("boxuv")
    if not t:
        continue
    bm = bmesh.new()
    bm.from_mesh(o.data)
    for layer in list(bm.loops.layers.uv):
        bm.loops.layers.uv.remove(layer)
    uv = bm.loops.layers.uv.new("UVMap")
    for f in bm.faces:
        ax = max(range(3), key=lambda i: abs(f.normal[i]))
        u_ax, v_ax = [(1, 2), (0, 2), (0, 1)][ax]
        for lp in f.loops:
            lp[uv].uv = (lp.vert.co[u_ax] / t, lp.vert.co[v_ax] / t)
    bm.to_mesh(o.data)
    bm.free()

for o in meshes:
    if o.name.startswith(("cais_", "agua", "fundo_")):
        continue
    for v in o.data.vertices:
        w = o.matrix_world @ v.co
        if -7.49 < w.x < 7.49 and -1.5 < w.y < 1.5 and w.z > 0.01:
            raise SystemExit(f"OBJETO NA FAIXA DE LUTA: {o.name} {tuple(round(c, 2) for c in w)}")

by_mat = {}
for o in meshes:
    m = o.data.materials[0].name if o.data.materials else "none"
    by_mat.setdefault(m, []).append(o)
for m, objs in by_mat.items():
    groups, chunk, verts = [], [], 0
    for o in objs:
        n = len(o.data.vertices)
        if chunk and verts + n > MAX_VERTS:
            groups.append(chunk)
            chunk, verts = [], 0
        chunk.append(o)
        verts += n
    groups.append(chunk)
    for gi, g in enumerate(groups):
        bpy.ops.object.select_all(action="DESELECT")
        for o in g:
            o.select_set(True)
        bpy.context.view_layer.objects.active = g[0]
        if len(g) > 1:
            bpy.ops.object.join()
        g[0].name = f"{m}_{gi}"

tris = 0
for o in bpy.data.objects:
    if o.type == "MESH":
        o.data.calc_loop_triangles()
        tris += len(o.data.loop_triangles)
print("TRIS", tris)
for im in bpy.data.images:
    if max(im.size) > 2048:
        f = 2048 / max(im.size)
        im.scale(int(im.size[0] * f), int(im.size[1] * f))

os.makedirs(os.path.dirname(OUT), exist_ok=True)
bpy.ops.export_scene.gltf(filepath=OUT, export_format="GLB", export_yup=True, export_apply=True,
                          export_animations=False, export_skins=False, export_cameras=False,
                          export_lights=False, export_image_format="AUTO")
print("EXPORTED", OUT, os.path.getsize(OUT))
if os.path.exists(mid_png):
    os.remove(mid_png)

# prévia com a câmera do jogo
cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
cam.data.sensor_fit = "VERTICAL"
cam.data.angle = math.radians(32)
cam.data.clip_end = 500
scene.collection.objects.link(cam)
cam.location = (0, -7, 1.45)
cam.rotation_euler = (Vector((0, 0, 1.05)) - cam.location).to_track_quat("-Z", "Y").to_euler()
scene.camera = cam
world = bpy.data.worlds.new("w")
scene.world = world
world.color = (0.55, 0.6, 0.6)
sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", "SUN"))
sun.data.energy = 2.5
sun.rotation_euler = (math.radians(50), 0, math.radians(-35))
scene.collection.objects.link(sun)
scene.render.engine = "BLENDER_EEVEE_NEXT"
scene.render.resolution_x, scene.render.resolution_y = 1280, 720
scene.view_settings.view_transform = "Standard"
scene.render.filepath = PREVIEW
bpy.ops.render.render(write_still=True)
print("PREVIEW", PREVIEW)
