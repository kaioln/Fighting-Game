import sys, bpy
f, clip = sys.argv[sys.argv.index("--") + 1:]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.context.scene.render.fps = 60
bpy.ops.import_scene.gltf(filepath=f)
arm = [o for o in bpy.data.objects if o.type == "ARMATURE"][0]
act = next(a for a in bpy.data.actions if a.name.startswith(clip))
arm.animation_data.action = act
if act.slots: arm.animation_data.action_slot = act.slots[0]
f0, f1 = (int(v) for v in act.frame_range)
for i in list(range(0, 12, 2)) + list(range(20, f1 - f0, 30)):
    bpy.context.scene.frame_set(f0 + i)
    mw = arm.matrix_world
    z = min((mw @ arm.pose.bones[b].head).z for b in ("foot_l", "foot_r", "ball_l", "ball_r"))
    print("FRAME", i, "min_foot_z", round(z, 3), "pelvis_z", round((mw @ arm.pose.bones["pelvis"].head).z, 3))
