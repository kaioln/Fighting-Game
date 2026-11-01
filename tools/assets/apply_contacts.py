import json
import sys

# usage: python -I apply_contacts.py <contacts.json> <presentation.json>...
# Grava contact/from/to (quadros a 60/s) nos golpes; preserva o cabeçalho de comentários "//".
TRIM = {  # ajustes à mão sobre a detecção automática (clipe: from, contact, to)
    "Punch_Jab": (4, 13, 40),
    "Punch_Cross": (8, 18, 46),
    "Melee_Hook": (5, 18, 28),
    "Slide_Start": (6, 15, 40),
    "Shield_Dash": (0, 13, 45),
    "Shield_OneShot": (10, 35, 50),
    "OverhandThrow": (0, 23, 50),
    "Sword_Heavy_Combo": (125, 156, 200),
    "CMU_Kick_74_03": (80, 110, 136),
    "CMU_Kick_74_05": (60, 88, 118),
    "CMU_Kick_74_06": (60, 86, 115),
    "CMU_JumpKick_75_16": (69, 102, 139),
    "CMU_JumpSpinKick_88_06": (0, 34, 75),
    "CMU_JumpSpinKick_87_01": (74, 124, 200),
}
auto = json.load(open(sys.argv[1]))
for p in sys.argv[2:]:
    raw = open(p, encoding="utf-8").read().splitlines()
    head = [l for l in raw if l.startswith("//")]
    j = json.loads("\n".join(l for l in raw if not l.startswith("//")))
    for mid, ref in j["moves"].items():
        clip = ref["clip"]
        if clip in TRIM:
            f, c, t = TRIM[clip]
            ref["from"], ref["contact"], ref["to"] = f, c, t
        elif clip in auto and auto[clip]["contact"] > 0:
            ref["contact"] = auto[clip]["contact"]
    open(p, "w", encoding="utf-8", newline="\n").write("\n".join(head) + "\n" + json.dumps(j, indent=2, ensure_ascii=False) + "\n")
    print("OK", p)
