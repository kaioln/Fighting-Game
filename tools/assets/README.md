# Pipeline de assets

Scripts que geram os arquivos de `assets/` a partir dos pacotes originais (ver `assets/LICENSES.md`).
Rodam no Blender 4.5 LTS em modo background, sem interface:

```bash
blender -b --factory-startup --python export_chars.py -- <pasta_dos_pacotes> <repo>/assets
blender -b --factory-startup --python retarget_cmu.py -- <pasta_bvh_cmu> <repo>/assets/animations/ual.glb <repo>/assets/animations/kicks.glb <pasta_previas>
blender -b --factory-startup --python build_arena.py -- <pasta_dos_pacotes> <repo>/assets/arena/doca.glb <repo>/assets/arena/preview.png <abandoned_slipway_2k.hdr> <porto_bruma.png>
blender -b --factory-startup --python sky_png.py -- <abandoned_slipway_2k.hdr> <repo>/assets/sky/abandoned_slipway.png
python glbinfo.py <arquivo.glb>...   # ossos, clipes, triângulos; confere se a ordem dos ossos é igual entre arquivos
```

`<pasta_dos_pacotes>` contém os zips extraídos (Quaternius UBC/UAL1/UAL2, kits Kenney, texturas ambientCG).
Todos os GLB de personagem e de animação usam o mesmo esqueleto de 65 ossos, na mesma ordem. É isso que a
raylib exige para aplicar clipes de um arquivo no modelo de outro.

Apresentação (quadros de contato a 60 quadros/s, como a raylib amostra o glTF):

```bash
blender -b --factory-startup --python contact_frames.py -- contacts.json <pasta_previas> <repo>/assets/animations/ual.glb <repo>/assets/animations/kicks.glb
python apply_contacts.py contacts.json <repo>/data/presentation/nadja.json <repo>/data/presentation/otto.json
```

Roupas de Nadja e Otto (cascas do corpo base pintadas com paleta chapada, a partir de `docs/arte/fichas`):

```bash
blender -b --factory-startup --python build_outfits.py -- <pasta_dos_pacotes> <repo>/assets <pasta_previas>
```

Golpes animados à mão (poses-chave com IK, assadas no esqueleto de 65 ossos):

```bash
blender -b --factory-startup --python keyanim.py -- <repo>/assets/animations/ual.glb <repo>/assets/animations/nadja_golpes.glb <pasta_previas>
```
