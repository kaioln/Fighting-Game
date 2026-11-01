import json, struct, sys

# usage: python -I glbinfo.py a.glb b.glb ... -> joints, anims (with duration), tris, images; compares joint order
def load(p):
    b = open(p, "rb").read()
    ln = struct.unpack("<I", b[12:16])[0]
    return json.loads(b[20:20 + ln]), len(b)

ref = None
for p in sys.argv[1:]:
    j, size = load(p)
    nodes = j.get("nodes", [])
    joints = [nodes[i].get("name") for s in j.get("skins", [])[:1] for i in s["joints"]]
    tris = 0
    maxv = 0
    for m in j.get("meshes", []):
        for pr in m["primitives"]:
            acc = j["accessors"]
            if "indices" in pr:
                tris += acc[pr["indices"]]["count"] // 3
            maxv = max(maxv, acc[pr["attributes"]["POSITION"]]["count"])
    anims = []
    for a in j.get("animations", []):
        t = max(j["accessors"][s["input"]].get("max", [0])[0] for s in a["samplers"])
        anims.append(f"{a.get('name')}({t:.2f}s)")
    imgs = [(im.get("name"), im.get("mimeType")) for im in j.get("images", [])]
    print(f"== {p}  {size/1e6:.2f} MB  skins={len(j.get('skins', []))} joints={len(joints)} meshes={len(j.get('meshes', []))} tris={tris} max_verts_per_prim={maxv} images={len(imgs)}")
    if anims:
        print(f"   anims={len(anims)}: " + ", ".join(anims))
    if joints:
        if ref is None:
            ref = joints
        print("   joint order == first file:", joints == ref)
