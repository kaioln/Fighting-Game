import sys, bpy, numpy as np
src, out, x0 = sys.argv[sys.argv.index("--") + 1:]
x0 = int(x0)
img = bpy.data.images.load(src); w, h = img.size
px = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)
crop = px[:, x0:x0 + h, :].copy()
o = bpy.data.images.new("c", h, h, alpha=True); o.pixels = crop.ravel()
o.filepath_raw = out; o.file_format = "PNG"; o.save(); print("CROP", out, h)
