"""Imagem -> GLB texturizado via Space público do Hugging Face (TRELLIS ou Hunyuan3D-2).
Uso: python img2_3d.py <trellis|hunyuan> <imagem.png> <saida.glb> [seed]"""
import os
import shutil
import sys

from gradio_client import Client, handle_file

engine, img, out = sys.argv[1:4]
seed = int(sys.argv[4]) if len(sys.argv) > 4 else 7

if engine == "trellis":
    c = Client("trellis-community/TRELLIS", verbose=False)
    try:
        c.predict(api_name="/start_session")
    except Exception as e:  # algumas versões não expõem
        print("start_session:", e)
    res = c.predict(image=handle_file(img), multiimages=[], seed=seed, ss_guidance_strength=7.5,
                    ss_sampling_steps=12, slat_guidance_strength=3.0, slat_sampling_steps=12,
                    multiimage_algo="stochastic", mesh_simplify=0.9, texture_size=2048,
                    api_name="/generate_and_extract_glb")
else:
    c = Client("tencent/Hunyuan3D-2", verbose=False)
    res = c.predict(caption=None, image=handle_file(img), mv_image_front=None, mv_image_back=None,
                    mv_image_left=None, mv_image_right=None, steps=30, guidance_scale=5.0, seed=seed,
                    octree_resolution=256, check_box_rembg=True, num_chunks=8000,
                    api_name="/generation_all")
print("RESULT", res)
paths = []


def walk(x):
    if isinstance(x, str) and x.lower().endswith((".glb", ".obj", ".ply")):
        paths.append(x)
    elif isinstance(x, dict):
        for v in x.values():
            walk(v)
    elif isinstance(x, (list, tuple)):
        for v in x:
            walk(v)


walk(res)
glbs = [p for p in paths if p.lower().endswith(".glb")]
print("FILES", paths)
if glbs:
    # o último GLB costuma ser o texturizado
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    shutil.copy(glbs[-1], out)
    print("SAVED", out)
