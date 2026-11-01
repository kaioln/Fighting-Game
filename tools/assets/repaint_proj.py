"""Projeta vistas repintadas (frente e costas, pintura estilizada) de volta na textura UV de um personagem preso.

O personagem é posto na pose A em que foi gerado (os ângulos medidos por rig_gen3d.py), a silhueta da malha
é alinhada à silhueta de cada vista pela caixa envolvente, e cada texel da textura recebe a cor da vista
para onde a sua face aponta (peso pela normal, com teste de profundidade). Onde nenhuma vista enxerga
(axilas, debaixo do queixo, a coxa atrás do avental) a cor pintada vizinha se espalha pela superfície da malha. As costuras das ilhas de UV são estendidas para fora (sem fio escuro no mip-map).

Rosto (opcional, <rosto.png>): close frontal da cabeça pintado a partir da vista de frente. O close é registrado na
vista de frente por correlação (escala e posição), a cabeça é realinhada à silhueta da cabeça na vista (o
alinhamento pelo corpo inteiro errava alguns pixels no rosto: sobrancelha dupla, riscos) e a frente da cabeça é
amostrada do close, sem mistura com a textura gerada.

Uso: bl.sh --python repaint_proj.py -- <char.glb> <frente.png> <costas.png> <theta> <phi> <out.glb> <prev.png> [<rosto.png>]
theta = graus dos braços abaixo da horizontal, phi = graus das pernas para fora (POSE_A no log do rig_gen3d).
"""
import math
import sys

import bpy
import numpy as np
from mathutils import Matrix

_a = sys.argv[sys.argv.index("--") + 1:]
CH, FRONT, BACK, THETA, PHI, OUT, PREV = _a[:7]
FACE = _a[7] if len(_a) > 7 else None
THETA, PHI = float(THETA), float(PHI)
TEX = 2048

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=CH)
arm = next(o for o in bpy.data.objects if o.type == "ARMATURE")
ob = max((o for o in bpy.data.objects if o.type == "MESH"), key=lambda o: len(o.data.vertices))

# pose A (mesma rotação de rig_gen3d.set_pose_A)
for side, sx in (("l", 1), ("r", -1)):
    for bn, ang in ((f"upperarm_{side}", THETA * sx), (f"thigh_{side}", -PHI * sx)):
        pb = arm.pose.bones[bn]
        pb.rotation_mode = "QUATERNION"
        rest = arm.data.bones[bn].matrix_local.to_3x3()
        pb.rotation_quaternion = (rest.inverted() @ Matrix.Rotation(math.radians(ang), 3, "Y") @ rest).to_quaternion()
bpy.context.view_layer.update()
dg = bpy.context.evaluated_depsgraph_get()
em = ob.evaluated_get(dg).to_mesh()
mw = np.array(ob.matrix_world)
P = np.array([v.co[:] for v in em.vertices])
P = P @ mw[:3, :3].T + mw[:3, 3]
em.calc_loop_triangles()
tris = np.array([t.vertices[:] for t in em.loop_triangles])
tloops = np.array([t.loops[:] for t in em.loop_triangles])
uv = np.array([d.uv[:] for d in em.uv_layers.active.data])
ob.evaluated_get(dg).to_mesh_clear()
# Blender Z para cima, frente = -Y. Na glTF importada a malha já vem em Z para cima.
A, Bv, C = P[tris[:, 0]], P[tris[:, 1]], P[tris[:, 2]]
N = np.cross(Bv - A, C - A)
N /= np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-12)


def load(path):
    im = bpy.data.images.load(path)
    w, h = im.size
    px = np.array(im.pixels[:], dtype=np.float32).reshape(h, w, 4)[::-1, :, :3]  # linha 0 = topo
    return px


def silhouette_box(px):
    bg = px[2:6, 2:6].reshape(-1, 3).mean(axis=0)
    m = np.abs(px - bg).max(axis=2) > 0.08
    # fundo = o que se liga à borda da imagem por pixels claros e pouco saturados (pega a sombra cinza no vão
    # entre as pernas, que passava no limiar simples e virava uma lâmina clara na face interna da perna)
    lightish = (px.min(axis=2) > 0.55) & (px.max(axis=2) - px.min(axis=2) < 0.10)
    bgm = np.zeros(m.shape, dtype=bool)
    bgm[0, :] = bgm[-1, :] = bgm[:, 0] = bgm[:, -1] = True
    bgm &= lightish | ~m
    for _ in range(2000):
        grow = (np.roll(bgm, 1, 0) | np.roll(bgm, -1, 0) | np.roll(bgm, 1, 1) | np.roll(bgm, -1, 1)) & (lightish | ~m)
        if (grow & ~bgm).sum() == 0:
            break
        bgm |= grow
    m &= ~bgm
    ys, xs = np.nonzero(m)
    return xs.min(), xs.max(), ys.min(), ys.max(), m


def project(px, sign):
    """sign=-1: vista de frente (câmera em -Y olhando +Y); +1: costas. Retorna cor por texel-amostra e peso."""
    h, w, _ = px.shape
    x0, x1, y0, y1, sil = silhouette_box(px)
    for _ in range(3):  # tira a borda da silhueta: o fundo branco não entra na textura
        sil = sil & np.roll(sil, 1, 0) & np.roll(sil, -1, 0) & np.roll(sil, 1, 1) & np.roll(sil, -1, 1)
    mx = P[:, 0] * (-sign)  # na vista de costas a direita da imagem é -X do personagem
    mz = P[:, 2]
    # alinhamento pela altura (cabeça aos pés) e pelo centro horizontal
    s = (y1 - y0) / (mz.max() - mz.min())
    cxm = (mx.max() + mx.min()) / 2
    ix = lambda X: (X - cxm) * s + (x0 + x1) / 2  # noqa: E731
    iy = lambda Z: y1 - (Z - mz.min()) * s  # noqa: E731
    sx_, sy_ = ix(mx), iy(mz)
    depth = P[:, 1] * sign  # maior = mais perto da câmera da vista
    zbuf = np.full((h, w), -1e9, dtype=np.float32)
    for t in range(len(tris)):
        a, b, c = tris[t]
        xs = np.array((sx_[a], sx_[b], sx_[c]))
        ys = np.array((sy_[a], sy_[b], sy_[c]))
        bx0, bx1 = int(max(0, np.floor(xs.min()))), int(min(w - 1, np.ceil(xs.max())))
        by0, by1 = int(max(0, np.floor(ys.min()))), int(min(h - 1, np.ceil(ys.max())))
        if bx1 < bx0 or by1 < by0:
            continue
        gx, gy = np.meshgrid(np.arange(bx0, bx1 + 1), np.arange(by0, by1 + 1))
        d = (ys[1] - ys[2]) * (xs[0] - xs[2]) + (xs[2] - xs[1]) * (ys[0] - ys[2])
        if abs(d) < 1e-9:
            continue
        l1 = ((ys[1] - ys[2]) * (gx - xs[2]) + (xs[2] - xs[1]) * (gy - ys[2])) / d
        l2 = ((ys[2] - ys[0]) * (gx - xs[2]) + (xs[0] - xs[2]) * (gy - ys[2])) / d
        l3 = 1 - l1 - l2
        ins = (l1 >= -0.01) & (l2 >= -0.01) & (l3 >= -0.01)
        z = l1 * depth[a] + l2 * depth[b] + l3 * depth[c]
        sub = zbuf[by0:by1 + 1, bx0:bx1 + 1]
        np.maximum(sub, np.where(ins, z, -1e9), out=sub)
    return dict(px=px, ix=ix, iy=iy, mxs=-sign, zbuf=zbuf, sil=sil, silfull=silhouette_box(px)[4])


views = [(project(load(FRONT), -1), -1), (project(load(BACK), +1), +1)]

face = None
if FACE:
    fv = views[0][0]
    neck_z = (arm.matrix_world @ arm.pose.bones["neck_01"].head).z
    hm = P[:, 2] > neck_z + 0.02
    Xm, Ym = fv["ix"](P[hm, 0]), fv["iy"](P[hm, 2])
    neck_row = fv["iy"](neck_z)
    top_m = Ym.min()
    band = (Ym > top_m + 0.35 * (neck_row - top_m)) & (Ym < top_m + 0.65 * (neck_row - top_m))
    x0m, x1m = Xm[band].min(), Xm[band].max()
    sf = fv["silfull"]
    cx = int((x0m + x1m) / 2)
    win = sf[:, max(0, cx - 80):cx + 80]
    rows = np.nonzero(win[:int(neck_row)].any(axis=1))[0]
    top_i = rows.min()
    bi = (np.arange(sf.shape[0]) > top_i + 0.35 * (neck_row - top_i)) & (np.arange(sf.shape[0]) < top_i + 0.65 * (neck_row - top_i))
    cols = np.nonzero(win[bi].any(axis=0))[0] + max(0, cx - 80)
    x0i, x1i = cols.min(), cols.max()
    k = (x1i - x0i) / max(x1m - x0m, 1e-6)
    cxm_, cxi_ = (x0m + x1m) / 2, (x0i + x1i) / 2
    # vista de frente -> close: correlação normalizada em escala e posição
    C = load(FACE).mean(axis=2)
    Fg = fv["px"].mean(axis=2)
    fy0, fy1 = int(top_i) - 3, int(neck_row) + 4
    hw = int((x1i - x0i) * 0.8)
    fx0, fx1 = int(cxi_) - hw, int(cxi_) + hw
    F = Fg[fy0:fy1, fx0:fx1]
    F = (F - F.mean()) / (F.std() + 1e-6)
    best = (-2, None)
    for sc in np.arange(3.0, 12.0, 0.1):
        ys_ = (np.arange(int(C.shape[0] / sc)) + 0.5) * sc
        xs_ = (np.arange(int(C.shape[1] / sc)) + 0.5) * sc
        if len(ys_) < F.shape[0] or len(xs_) < F.shape[1]:
            continue
        Cs = np.zeros((len(ys_), len(xs_)), dtype=np.float32)
        for oy in (-0.25, 0.25):
            for ox in (-0.25, 0.25):
                Cs += C[np.clip((ys_ + oy * sc).astype(int), 0, C.shape[0] - 1)][:, np.clip((xs_ + ox * sc).astype(int), 0, C.shape[1] - 1)]
        Cs /= 4
        Wv = np.lib.stride_tricks.sliding_window_view(Cs, F.shape)
        mu = Wv.mean(axis=(2, 3))
        sd = Wv.std(axis=(2, 3)) + 1e-6
        ncc = ((Wv * F).mean(axis=(2, 3)) - mu * F.mean()) / sd
        j = np.unravel_index(ncc.argmax(), ncc.shape)
        if ncc[j] > best[0]:
            best = (float(ncc[j]), (sc, j[0], j[1]))
    sc, oy, ox = best[1]
    print("FACE_REG ncc %.3f scale %.2f head_k %.3f" % (best[0], sc, k))
    face = dict(px=load(FACE), sc=sc, oy=oy, ox=ox, fy0=fy0, fx0=fx0, neck_z=neck_z,
                X=lambda X: cxi_ + (X - cxm_) * k, Y=lambda Y: top_i + (Y - top_m) * k)
facem = np.zeros((TEX, TEX), dtype=bool)

# rasteriza cada triângulo no espaço UV: posição 3D, normal e cor das vistas
col = np.zeros((TEX, TEX, 3), dtype=np.float32)
wsum = np.zeros((TEX, TEX), dtype=np.float32)
cov = np.zeros((TEX, TEX), dtype=bool)
for t in range(len(tris)):
    u = uv[tloops[t]] * (TEX - 1)
    bx0, by0 = np.floor(u.min(axis=0)).astype(int)
    bx1, by1 = np.ceil(u.max(axis=0)).astype(int)
    if bx1 - bx0 > 300 or by1 - by0 > 300:
        continue
    gx, gy = np.meshgrid(np.arange(bx0, bx1 + 1), np.arange(by0, by1 + 1))
    a, b, c = u
    d = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1])
    if abs(d) < 1e-12:
        continue
    l1 = ((b[1] - c[1]) * (gx - c[0]) + (c[0] - b[0]) * (gy - c[1])) / d
    l2 = ((c[1] - a[1]) * (gx - c[0]) + (a[0] - c[0]) * (gy - c[1])) / d
    l3 = 1 - l1 - l2
    ins = (l1 >= -0.02) & (l2 >= -0.02) & (l3 >= -0.02) & (gx >= 0) & (gx < TEX) & (gy >= 0) & (gy < TEX)
    if not ins.any():
        continue
    gx, gy, l1, l2, l3 = gx[ins], gy[ins], l1[ins], l2[ins], l3[ins]
    ia, ib, ic = tris[t]
    pos = l1[:, None] * P[ia] + l2[:, None] * P[ib] + l3[:, None] * P[ic]
    cov[gy, gx] = True
    for v, sign in views:
        facing = N[t, 1] * sign  # >0: normal aponta para a câmera da vista
        # vale até um pouco além do perfil: as laterais (que nenhuma vista vê de frente) pegam a cor da borda
        # da silhueta em vez da textura gerada, escura nas laterais
        if facing < -0.25:
            continue
        wgt = (facing + 0.25) / 1.25
        X = v["ix"](pos[:, 0] * v["mxs"])
        Y = v["iy"](pos[:, 2])
        h, w, _ = v["px"].shape
        xi = np.clip(np.round(X).astype(int), 0, w - 1)
        yi = np.clip(np.round(Y).astype(int), 0, h - 1)
        dep = pos[:, 1] * sign
        vis = (dep >= v["zbuf"][yi, xi] - 0.02) & v["sil"][yi, xi]  # tolerância de 2 cm
        if not vis.any():
            continue
        wv = np.where(vis, wgt ** 1.5, 0.0).astype(np.float32)
        cv = v["px"][yi, xi]
        if face is not None and sign < 0 and facing > 0.0:
            hd = (pos[:, 2] > face["neck_z"] + 0.03) & vis
            if hd.any():
                # cabeça realinhada à silhueta e amostrada do close (bilinear)
                Xh, Yh = face["X"](X[hd]), face["Y"](Y[hd])
                cxc = (Xh - face["fx0"] + face["ox"] + 0.5) * face["sc"] - 0.5
                cyc = (Yh - face["fy0"] + face["oy"] + 0.5) * face["sc"] - 0.5
                fp = face["px"]
                x0_ = np.clip(np.floor(cxc).astype(int), 0, fp.shape[1] - 2)
                y0_ = np.clip(np.floor(cyc).astype(int), 0, fp.shape[0] - 2)
                tx, ty = (cxc - x0_).clip(0, 1)[:, None], (cyc - y0_).clip(0, 1)[:, None]
                cv = cv.copy()
                cv[hd] = (fp[y0_, x0_] * (1 - tx) * (1 - ty) + fp[y0_, x0_ + 1] * tx * (1 - ty) +
                          fp[y0_ + 1, x0_] * (1 - tx) * ty + fp[y0_ + 1, x0_ + 1] * tx * ty)
                wv = np.where(hd, wv * 4.0, wv)
                facem[gy[hd], gx[hd]] = True
        np.add.at(col, (gy, gx), cv * wv[:, None])
        np.add.at(wsum, (gy, gx), wv)
print("TEXELS_COVERED", int(cov.sum()), "PROJECTED", int((wsum > 0.05).sum()))

# textura antiga, no mesmo tamanho
mat = ob.data.materials[0]
bsdf = next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
img = bsdf.inputs["Base Color"].links[0].from_node.image
if img.size[0] != TEX:
    img.scale(TEX, TEX)
old = np.array(img.pixels[:], dtype=np.float32).reshape(TEX, TEX, 4)
proj = col / np.maximum(wsum, 1e-6)[..., None]
alpha = np.clip(wsum / 0.6, 0, 1)
alpha[facem] = 1.0  # rosto: só o close, sem fantasma da textura gerada
# ajuste de cor da textura antiga (média e contraste) para casar com a pintura onde as duas existem
m = (alpha > 0.8) & cov
o_rgb = old[..., :3]
mo, so = o_rgb[m].mean(axis=0), o_rgb[m].std(axis=0) + 1e-4
mp, sp = proj[m].mean(axis=0), proj[m].std(axis=0) + 1e-4
adj = np.clip((o_rgb - mo) / so * sp + mp, 0, 1)

# Onde nenhuma vista enxerga (frente da coxa atrás do avental, axilas, debaixo do queixo) a textura gerada é
# ruído colorido. A cor pintada dos vizinhos se espalha pela superfície da malha, vértice a vértice, e só um
# pouco do relevo de luz da textura antiga fica por cima.
# a malha importada vem partida nas costuras de UV: a propagação usa as posições soldadas
_, wid = np.unique(np.round(P * 1e4).astype(np.int64), axis=0, return_inverse=True)
wid = wid.ravel()
NVt = int(wid.max()) + 1
vcol = np.zeros((NVt, 3), dtype=np.float32)
vw = np.zeros(NVt, dtype=np.float32)
for t in range(len(tris)):
    cu = uv[tloops[t]].mean(axis=0)
    for k_ in range(3):
        uu = (uv[tloops[t, k_]] * 0.6 + cu * 0.4) * (TEX - 1)  # um pouco para dentro do triângulo
        xi_, yi_ = int(round(uu[0])), int(round(uu[1]))
        if 0 <= xi_ < TEX and 0 <= yi_ < TEX and alpha[yi_, xi_] > 0.6:
            vcol[wid[tris[t, k_]]] += proj[yi_, xi_]
            vw[wid[tris[t, k_]]] += 1
known = vw > 0
vcol[known] /= vw[known][:, None]
tw = wid[tris]
eds = np.concatenate([tw[:, [0, 1]], tw[:, [1, 2]], tw[:, [2, 0]]])
eds = np.concatenate([eds, eds[:, ::-1]])
for _ in range(600):
    if known.all():
        break
    acc_ = np.zeros_like(vcol)
    cnt_ = np.zeros(NVt, dtype=np.float32)
    ok_ = known[eds[:, 1]]
    np.add.at(acc_, eds[ok_, 0], vcol[eds[ok_, 1]])
    np.add.at(cnt_, eds[ok_, 0], 1)
    grow_ = (~known) & (cnt_ > 0)
    if not grow_.any():
        break
    vcol[grow_] = acc_[grow_] / cnt_[grow_][:, None]
    known |= grow_
print("VERTS_PAINTED", int((vw > 0).sum()), "SPREAD", int(known.sum() - (vw > 0).sum()), "OF", NVt)
fill = adj.copy()
lum_mean = float(adj[cov].mean())
for t in range(len(tris)):
    u = uv[tloops[t]] * (TEX - 1)
    bx0, by0 = np.floor(u.min(axis=0)).astype(int)
    bx1, by1 = np.ceil(u.max(axis=0)).astype(int)
    if bx1 - bx0 > 300 or by1 - by0 > 300:
        continue
    gx, gy = np.meshgrid(np.arange(max(bx0, 0), min(bx1, TEX - 1) + 1), np.arange(max(by0, 0), min(by1, TEX - 1) + 1))
    a, b, c = u
    d = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1])
    if abs(d) < 1e-12 or gx.size == 0:
        continue
    l1 = ((b[1] - c[1]) * (gx - c[0]) + (c[0] - b[0]) * (gy - c[1])) / d
    l2 = ((c[1] - a[1]) * (gx - c[0]) + (a[0] - c[0]) * (gy - c[1])) / d
    l3 = 1 - l1 - l2
    ins = (l1 >= -0.02) & (l2 >= -0.02) & (l3 >= -0.02)
    ins &= alpha[gy, gx] < 0.999
    if not ins.any():
        continue
    gx, gy, l1, l2, l3 = gx[ins], gy[ins], l1[ins], l2[ins], l3[ins]
    ia, ib, ic = tw[t]
    if not (known[ia] and known[ib] and known[ic]):
        continue  # peça solta sem nenhum vizinho pintado: fica a textura antiga ajustada
    base = l1[:, None] * vcol[ia] + l2[:, None] * vcol[ib] + l3[:, None] * vcol[ic]
    lum = adj[gy, gx].mean(axis=1, keepdims=True)
    fill[gy, gx] = np.clip(base * (1.0 + 0.4 * (lum - lum_mean)), 0, 1)  # relevo da textura antiga, de leve
new = fill * (1 - alpha[..., None]) + proj * alpha[..., None]
# estende as ilhas para fora (16 passos) para as costuras não puxarem cor de fora no mip-map
filled = cov.copy()
for _ in range(16):
    acc = np.zeros_like(new)
    cnt = np.zeros(filled.shape, dtype=np.float32)
    for dy, dx in ((0, 1), (0, -1), (1, 0), (-1, 0)):
        sh = np.roll(filled, (dy, dx), axis=(0, 1))
        acc += np.roll(new, (dy, dx), axis=(0, 1)) * sh[..., None]
        cnt += sh
    grow = (~filled) & (cnt > 0)
    new[grow] = acc[grow] / cnt[grow][:, None]
    filled |= grow
old[..., :3] = new
img.pixels = old.ravel()
img.update()
img.pack()

# exporta em repouso
for pb in arm.pose.bones:
    pb.rotation_quaternion = (1, 0, 0, 0)
bpy.context.view_layer.update()
bpy.ops.object.select_all(action="DESELECT")
arm.select_set(True)
ob.select_set(True)
bpy.ops.export_scene.gltf(filepath=OUT, export_format="GLB", export_skins=True, export_animations=False, use_selection=True,
                          export_morph=False, export_cameras=False, export_lights=False, export_yup=True,
                          export_image_format="AUTO")
prev = bpy.data.images.new("prev", TEX // 2, TEX // 2)
small = old.reshape(TEX // 2, 2, TEX // 2, 2, 4).mean(axis=(1, 3))
prev.pixels = small.ravel()
prev.filepath_raw = PREV
prev.file_format = "PNG"
prev.save()
print("EXPORTED", OUT)
