"""Gera data/presentation de teste para a CI a partir dos clipes de um modelo glTF.

Uso: python smoke_presentation.py <lista_de_clipes.txt> <modelo relativo a assets/> <pasta data/presentation>
Só para o teste de fumaça da CI: o jogo de verdade usa arquivos escritos à mão.
"""
import json
import pathlib
import sys

clips = [l.split()[0] for l in pathlib.Path(sys.argv[1]).read_text().splitlines() if l.strip()]


def pick(*words):
    for w in words:
        for c in clips:
            if w.lower() in c.lower():
                return c
    return clips[0]


look = {
    "model": sys.argv[2],
    "scale_milli": 350,
    "states": {
        "idle": pick("idle"),
        "walk_forward": pick("walk"),
        "crouch": pick("sit", "idle"),
        "hit": pick("no", "death"),
        "knockdown": pick("death"),
        "jump": pick("jump"),
    },
    "moves": {"5MP": {"clip": pick("punch"), "contact": 10}, "5HK": {"clip": pick("punch"), "contact": 10}},
}
out = pathlib.Path(sys.argv[3])
out.mkdir(parents=True, exist_ok=True)
for cid in ("nadja", "otto"):
    (out / f"{cid}.json").write_text(json.dumps(look, indent=2))
(out / "arena.json").write_text(json.dumps({"model": sys.argv[2], "scale_milli": 0, "sky": [70, 90, 120]}))
print(json.dumps(look, indent=2))
