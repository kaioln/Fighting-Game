"""Monta a arena "doca" com kits Kenney (CC0) + texturas ambientCG (CC0) e exporta um GLB.

Coordenadas do Blender (Z para cima). No glTF (Y para cima): x=x, y=z, z=-y.
  - Luta: chão plano z=0 ao longo de X de -8 a +8; faixa livre entre y=-1,5 e y=+1,5 (glTF z ±1,5).
  - Paredes do palco: pilhas de contêineres com a face interna em x=±7,5.
  - Cenário de fundo: y de +3 a +40 (glTF z de -3 a -40). A câmera fica em y negativo olhando para +Y.
Uso: bl.sh --python build_arena.py -- <src_dir> <out.glb> <preview.png> <hdri.hdr>
"""
import glob
import math
import os
import sys

import bmesh
import bpy
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
SRC, OUT, PREVIEW, HDRI, BRUMA = argv
MAX_VERTS = 60000  # raylib usa índices de 16 bits

# escala de cada kit para metros reais
KIT_SCALE = {
    "city-kit-industrial": 2.59 / 0.35,  # contêiner de 20 pés: 2,59 m de altura
    "watercraft-kit": 7.0,               # cargueiro ~74 m de comprimento
    "factory-kit": 2.0,                  # passarela de 2 m de largura
    "pirate-kit": 1.6,                   # módulo de cais de 4 m
    "survival-kit": 2.0,
}

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
kit_material = {}  # um material (colormap) por kit
placed = []


def find(name):
    hits = glob.glob(os.path.join(SRC, "*", "Models", "GLB format", name + ".glb"))
    assert hits, name
    return hits[0]


PP = os.path.normpath(os.path.join(SRC, "..", "..", "assets", "third_party", "polypizza"))


def place(name, x, y, z=0.0, rot=0.0, s=1.0, mat=None, tile=2.5, keep=None):
    """Kenney: escala do kit. "pp:<id>": modelo do Poly Pizza, escala absoluta s, materiais próprios.
    mat: troca o material por um PBR com UV de caixa (tile em metros); keep: materiais preservados."""
    if name.startswith("pp:"):
        path, kit = os.path.join(PP, name[3:] + ".glb"), "polypizza"
    else:
        path = find(name)
        kit = path.split(os.sep)[-4]
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    new = [o for o in bpy.data.objects if o not in before]
    root = bpy.data.objects.new(f"{name}_root", None)
    scene.collection.objects.link(root)
    for o in new:
        if o.parent is None:
            o.parent = root
        if o.type == "MESH":
            for slot in o.material_slots:
                m = slot.material
                if m is None:
                    continue
                if mat is not None and not (keep and any(k_ in m.name.lower() for k_ in keep)):
                    slot.material = mat
                    o["boxuv"] = tile
                    continue
                if kit == "polypizza":
                    continue
                if kit not in kit_material:
                    m.name = f"kenney_{kit}"
                    kit_material[kit] = m
                slot.material = kit_material[kit]
            placed.append(o)
    k = (1.0 if kit == "polypizza" else KIT_SCALE[kit]) * s
    root.scale = (k, k, k)
    root.rotation_euler = (0, 0, math.radians(rot))
    root.location = (x, y, z)
    return root


def tex_material(name, folder, tile_m, color_mul=(1, 1, 1)):
    d = glob.glob(os.path.join(SRC, folder, "*_Color.jpg"))[0]
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    t = nt.nodes.new("ShaderNodeTexImage")
    t.image = bpy.data.images.load(d)
    # textura x cor constante: o exportador glTF grava isso como baseColorFactor
    mix = nt.nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    mix.blend_type = "MULTIPLY"
    mix.inputs["Factor"].default_value = 1.0
    nt.links.new(t.outputs["Color"], mix.inputs["A"])
    mix.inputs["B"].default_value = (*color_mul, 1)
    nt.links.new(mix.outputs["Result"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.9
    m["tile_m"] = tile_m
    return m


def color_material(name, rgb, rough=0.6):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (*rgb, 1)
    b.inputs["Roughness"].default_value = rough
    return m


def box(name, mn, mx, mat, tile=None):
    """Caixa alinhada aos eixos com UV em escala de mundo (tile em metros)."""
    me = bpy.data.meshes.new(name)
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    for v in bm.verts:
        v.co = Vector((mn[i] + (v.co[i] + 0.5) * (mx[i] - mn[i]) for i in range(3)))
    uv = bm.loops.layers.uv.new("UVMap")
    t = tile or mat.get("tile_m", 4.0)
    for f in bm.faces:
        n = f.normal
        ax = max(range(3), key=lambda i: abs(n[i]))
        u_ax, v_ax = [(1, 2), (0, 2), (0, 1)][ax]
        for lp in f.loops:
            lp[uv].uv = (lp.vert.co[u_ax] / t, lp.vert.co[v_ax] / t)
    bm.to_mesh(me)
    bm.free()
    o = bpy.data.objects.new(name, me)
    o.data.materials.append(mat)
    scene.collection.objects.link(o)
    placed.append(o)
    return o


# ---------- materiais ambientCG ----------
concrete = tex_material("cais_concreto", "Concrete034_1K-JPG", 3.0, (0.62, 0.62, 0.6))
quay_wall = tex_material("cais_muro", "Concrete048_1K-JPG", 3.0, (0.6, 0.58, 0.55))
asphalt = tex_material("patio_asfalto", "Asphalt031_1K-JPG", 4.0)
steel = tex_material("chapa_ondulada", "CorrugatedSteel007A_1K-JPG", 2.0)
water = color_material("agua", (0.10, 0.22, 0.26), 0.15)
# contêineres: pintura branca com ferrugem (ambientCG PaintedMetal012) tingida por contêiner
cont_mat = {
    "a": tex_material("conteiner_vermelho", "PaintedMetal012_1K-JPG", 2.6, (0.62, 0.24, 0.17)),
    "b": tex_material("conteiner_verde", "PaintedMetal012_1K-JPG", 2.6, (0.24, 0.46, 0.38)),
    "c": tex_material("conteiner_azul", "PaintedMetal012_1K-JPG", 2.6, (0.26, 0.38, 0.58)),
}
# madeira molhada (ambientCG Wood080 escurecida) para caixotes e píer
wet_wood = tex_material("madeira_molhada", "Wood080_1K-JPG", 1.6, (0.50, 0.44, 0.38))


def container(variant, x, y, z=0.0, rot=0.0):
    return place("shipping-container-" + variant, x, y, z, rot=rot, mat=cont_mat[variant], tile=2.6)

# ---------- chão do cais (plano z=0, superfície contínua) ----------
box("cais_piso", (-60, -12, -0.4), (60, 4, 0.0), concrete)
box("patio_piso", (-60, 4, -0.4), (60, 12, 0.0), asphalt)
box("cais_borda", (-60, 12, -2.2), (60, 12.6, 0.0), quay_wall)
box("agua", (-200, 12.6, -0.7), (200, 120, -0.6), water, tile=50.0)

# ---------- paredes do palco: contêineres com face interna em x = ±7,5 ----------
C_W = 0.37 * KIT_SCALE["city-kit-industrial"]  # largura ~2,74 m
C_H = 2.59
for side in (-1, 1):
    cx = side * (7.5 + C_W / 2)
    for i, y in enumerate((-4.6, 1.5, 7.6)):
        for level in range(3 if i == 1 else 2):
            container("abc"[(i + level + (side > 0)) % 3], cx, y, level * C_H)
    # segunda fileira atrás da parede (dá volume quando a câmera encosta)
    for y in (-1.5, 4.6):
        container("cb"[side > 0], cx + side * C_W * 1.05, y, 0)

# ---------- fundo próximo (y 3..9): carga baixa, sem tapar os lutadores ----------
place("cargo-pile-a", -11.5, 6.0, rot=90, s=0.33)
place("cargo-pile-b", 11.5, 6.2, rot=90, s=0.33)
place("crate", -5.6, 3.6, rot=12, s=0.6, mat=wet_wood, tile=1.2)
place("crate", -4.7, 3.9, rot=-8, s=0.6, mat=wet_wood, tile=1.2)
place("crate", -5.2, 3.7, z=0.82, rot=30, s=0.6, mat=wet_wood, tile=1.2)
place("barrel", 4.8, 3.5, s=0.35, mat=cont_mat["a"], tile=0.8)
place("barrel", 5.4, 3.8, s=0.35, mat=cont_mat["a"], tile=0.8)
place("barrel", 5.0, 4.3, s=0.35, mat=cont_mat["a"], tile=0.8)
place("box-large", 2.2, 5.8, rot=5, s=0.6, mat=wet_wood, tile=1.2)
place("box-wide", 2.9, 5.9, rot=-10, s=0.6, mat=wet_wood, tile=1.2)

# pórtico: pilares de contêineres e passarela por cima em y = 10,5 (centro aberto para a água)
for k, x in enumerate((-17.0, -9.5, 9.5, 17.0)):
    for level in range(2):
        container("abc"[(k + level) % 3], x, 10.5, level * C_H, rot=90)
cat_z = 2 * C_H
x = -19.0
while x <= 19.0:
    place("catwalk-straight", x, 10.5, cat_z, rot=90)
    x += 2.0
place("catwalk-stairs", 20.5, 10.5, 0.0, rot=180)

# ---------- galpões nas laterais do cais ----------
place("building-c", -19.0, 6.0, rot=90)
place("building-a", 19.5, 5.5, rot=-90)
place("building-f", -27.0, 9.0, rot=180)
place("building-k", 24.0, 10.0)
place("chimney-large", -31.0, 3.0)
place("detail-tank-large", 28.0, 2.0)
place("water-tower", 31.0, 8.0)

# ---------- água: rebocador e cargueiro ao fundo ----------
place("boat-tug-a", -11.0, 18.0, z=-0.6, rot=70, s=0.35)
place("buoy", 14.0, 16.0, z=-0.6, s=0.35)

# a outra margem agora é o painel Porto Bruma (abaixo)

# ---------- detalhes do cais (Poly Pizza; licenças em assets/LICENSES.md) ----------
# guindastes: torre (J-Toastie, CC-BY 3.0) e treliça (Kieran Farr, CC-BY 3.0)
place("pp:gCcpjaxFdv", -24.0, 8.0, rot=200, s=2.0)
place("pp:cm5teXZ5Ctr", 27.0, 9.0, rot=-20, s=5.0)
# postes na borda do cais (Quaternius, CC0)
for x in (-12.5, 12.5):
    place("pp:nFwrlcLvM5", x, 11.7, rot=180, s=1.0)
# cabeços de amarração com corda (J-Toastie, CC-BY 3.0)
for x in (-5.5, 0.0, 5.5):
    place("pp:JSQJ07PFj2", x, 12.25, s=0.5)
place("pp:dkNzi6oDQj", -4.7, 12.0, rot=30, s=1.1)
place("pp:dkNzi6oDQj", 0.9, 11.9, rot=-50, s=1.1)
# correntes penduradas da passarela (mrzombieuk, CC-BY 3.0)
for x in (-14.5, 14.0):
    place("pp:U8MTqEf3dq", x, 10.5, z=cat_z - 3.6, s=0.1)
# empilhadeira e paleteira (KolosStudios, CC-BY 3.0)
place("pp:DTQBuenKJY", -12.5, 2.5, rot=60, s=1.35)
place("pp:snDq7v3GRO", 3.8, 5.6, rot=-70, s=0.9)
# píer de madeira sobre a água (Quaternius "Dock Wide", CC0) com madeira molhada
place("pp:XndOrGa7rN", -19.0, 17.0, z=-0.9, rot=90, s=0.55, mat=wet_wood, tile=1.6, keep=("rope",))

# ---------- fundo: Porto Bruma (imagem gerada para o projeto), painel a ~100 m ----------
bruma = bpy.data.materials.new("porto_bruma")
bruma.use_nodes = True
bt = bruma.node_tree.nodes.new("ShaderNodeTexImage")
bt.image = bpy.data.images.load(BRUMA)
bruma.node_tree.links.new(bt.outputs["Color"], bruma.node_tree.nodes["Principled BSDF"].inputs["Base Color"])
bme = bpy.data.meshes.new("porto_bruma")
# imagem 1088x608: costa na linha 520; usa as linhas 240-608 (céu acima da costa e um pouco de água)
BW, BY, BZ = 140.0, 100.0, 0.0
px_m = 1088 / BW
btop, bbot = (520 - 240) / px_m, (608 - 520) / px_m
bme.from_pydata([(-BW / 2, BY, BZ - bbot), (BW / 2, BY, BZ - bbot), (BW / 2, BY, BZ + btop), (-BW / 2, BY, BZ + btop)],
                [], [(0, 1, 2, 3)])  # face voltada para -Y (câmera); o jogo descarta o verso
buv = bme.uv_layers.new(name="UVMap")
vmax = (608 - 240) / 608
for li, vi in enumerate(bme.polygons[0].vertices):
    buv.data[li].uv = ((0, 0), (1, 0), (1, vmax), (0, vmax))[vi]
bo = bpy.data.objects.new("porto_bruma", bme)
bo.data.materials.append(bruma)
scene.collection.objects.link(bo)
placed.append(bo)

# galpão de chapa ondulada fechando o fundo à esquerda (textura ambientCG)
box("galpao_parede", (-42, 13.0, 0.0), (-30, 14.0, 9.0), steel)

# ---------- limpeza: aplica transformações e junta por material ----------
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

# faixa de luta livre? (x em [-7,5, 7,5], y em [-1,5, 1,5], acima do piso)
for o in meshes:
    if o.name.startswith(("cais_", "patio_", "agua", "porto_bruma")):
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
    chunk, verts = [], 0
    groups = []
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
        print("MESH", o.name, len(o.data.vertices), "verts", len(o.data.loop_triangles), "tris")
print("TRIS", tris)

for img in bpy.data.images:
    if img.size[0] > 1024 or img.size[1] > 1024:
        f = 1024 / max(img.size)
        img.scale(int(img.size[0] * f), int(img.size[1] * f))

os.makedirs(os.path.dirname(OUT), exist_ok=True)
# a raylib 5.5 não lê JPEG por padrão: converte as texturas JPEG para PNG antes de exportar
png_dir = os.path.join(os.path.dirname(OUT), "_png_tmp")
os.makedirs(png_dir, exist_ok=True)
for img in bpy.data.images:
    if img.file_format == "JPEG" or img.filepath.lower().endswith((".jpg", ".jpeg")):
        dst = os.path.join(png_dir, os.path.splitext(os.path.basename(img.filepath))[0] + ".png")
        img.file_format = "PNG"
        img.filepath_raw = dst
        img.save()
        img.filepath = dst
        img.reload()
bpy.ops.export_scene.gltf(filepath=OUT, export_format="GLB", export_yup=True, export_apply=True,
                          export_animations=False, export_skins=False, export_cameras=False,
                          export_lights=False, export_image_format="AUTO")
print("EXPORTED", OUT, os.path.getsize(OUT))
import shutil
shutil.rmtree(png_dir, ignore_errors=True)

# ---------- prévia: câmera glTF (0, 1.45, 7) -> (0, 1.05, 0), fovy 32 ----------
cam_data = bpy.data.cameras.new("cam")
cam_data.sensor_fit = "VERTICAL"
cam_data.angle = math.radians(32)
cam_data.clip_end = 500
cam = bpy.data.objects.new("cam", cam_data)
scene.collection.objects.link(cam)
cam.location = (0, -7, 1.45)
direction = Vector((0, 0, 1.05)) - cam.location
cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
scene.camera = cam

world = bpy.data.worlds.new("w")
scene.world = world
world.use_nodes = True
env = world.node_tree.nodes.new("ShaderNodeTexEnvironment")
env.image = bpy.data.images.load(HDRI)
world.node_tree.links.new(env.outputs["Color"], world.node_tree.nodes["Background"].inputs["Color"])
sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", "SUN"))
sun.data.energy = 2.5
sun.rotation_euler = (math.radians(50), 0, math.radians(-35))
scene.collection.objects.link(sun)

scene.render.engine = "BLENDER_EEVEE_NEXT" if "BLENDER_EEVEE_NEXT" in [e.identifier for e in bpy.types.RenderSettings.bl_rna.properties["engine"].enum_items] else "BLENDER_EEVEE"
scene.render.resolution_x, scene.render.resolution_y = 1280, 720
scene.render.resolution_percentage = 100
scene.view_settings.view_transform = "AgX"
scene.render.filepath = PREVIEW
scene.render.image_settings.file_format = "PNG"
bpy.ops.render.render(write_still=True)
print("PREVIEW", PREVIEW)
