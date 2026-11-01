# Pipeline de assets

Scripts que geram os arquivos de `assets/` a partir dos pacotes originais (ver `assets/LICENSES.md`).
Rodam no Blender 4.5 LTS em modo background, sem interface:

```bash
blender -b --factory-startup --python export_chars.py -- <pasta_dos_pacotes> <repo>/assets
blender -b --factory-startup --python retarget_cmu.py -- <pasta_bvh_cmu> <repo>/assets/animations/ual.glb <repo>/assets/animations/kicks.glb <pasta_previas>
blender -b --factory-startup --python build_arena_estilo.py -- <pasta_pintados> <pasta_polypizza> <repo>/assets/arena/doca.glb <repo>/assets/arena/preview.png
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

Personagens gerados (imagem frontal em pose A -> TRELLIS no Hugging Face -> esqueleto de 65 ossos):

```bash
python img2_3d.py trellis <frontal_quadrada.png> <gerado.glb>
blender -b --factory-startup --python rig_gen3d.py -- <gerado.glb> <corpo_base_UBC.gltf> - <repo>/assets/characters/<nome>.glb <altura_m> <max_tris> <pasta_previas> <nome>
blender -b --factory-startup --python pose_test.py -- <personagem.glb> <animacoes.glb> <pasta> 0 rest:0 Clip:quadro ...
```
Pesos por calor (bone heat) na própria malha, rótulos limpos e suavizados; furos do corte (axilas) são fechados.
Repintura (vistas de frente e costas pintadas no Real Oficial a partir da imagem frontal, projetadas de volta na UV):

```bash
blender -b --factory-startup --python repaint_proj.py -- <personagem.glb> <frente.png> <costas.png> <theta> <phi> <saida.glb> <previa.png>
blender -b --factory-startup --python keyanim2.py -- <repo>/assets/animations/ual.glb <pasta_saida> <pasta_previas>
```
`theta`/`phi` = linha `POSE_A` do log do rig_gen3d (Otto 56,4/7,2; Nadja 55,8/10,5). As vistas ficam em
`assets_src/repintura`; a Nadja leva o close do rosto como último argumento (`nadja_rosto_a.png`).
Capturas de um golpe: `fgjogo --smoke <png> --frames N --p2 "6*40 5*10 2 3 6+LP"` (o neutro `5*10` evita que
andar + 236 vire 623); `--barra` enche a barra de super.

O esqueleto não muda: o personagem gerado é posto na pose A medida, recebe os pesos do corpo base posado igual,
volta à T-pose de repouso e é preso no mesmo esqueleto. Limite da raylib: até 65.535 vértices por primitiva.
