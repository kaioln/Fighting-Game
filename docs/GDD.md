# Game Design Document — v0.1 (Fase 1)

Status: **aprovado pelo Kaio (v0.1)**. Números são provisórios até a validação em jogo; o núcleo da Fase 2 está em [10](10-nucleo-tecnico.md).

## 1. Visão

Jogo de luta 2.5D original para PC, com combate rápido e legível, personagens 3D artesanais e uma
mecânica central — **Maré** — que transforma domínio de território em recurso.
Codinome: *Projeto Estaleiro*. Ambientação: Porto Bruma ([07](07-direcao-artistica.md)).

**Pilares:** (1) responsividade e precisão; (2) legibilidade acima de espetáculo; (3) cada golpe com peso;
(4) qualidade acima de quantidade.

## 2. Plataforma e técnica

PC Windows, nativo. Simulação determinística a 60 Hz em C++ (`fgcore`), apresentação em camada fina e trocável (Godot 4 usado minimamente, por preferência do Kaio).
Rollback-ready desde o núcleo. Detalhes em [05](05-engine-e-arquitetura.md).

## 3. Controles

**Clássico:** 4 direções + 6 botões: Soco Leve/Médio/Pesado, Chute Leve/Médio/Pesado. Arremesso = Leve+Leve.
**Simplificado:** Leve/Médio/Pesado + botão Especial (especial = botão + direção) + Assistência (encadeamento automático).
Custo do Simplificado: especiais com dano menor (valor a balancear), mesmo frame data — sem vantagem oculta.

**Buffer e motions (valores iniciais, tabela em dados):**

| Item | Valor inicial |
|---|---|
| Buffer de botão para a próxima ação | 4 frames |
| Motion 236/214 | 11 frames do primeiro direcional ao botão |
| Motion 623 | 13 frames, aceita atalho 323 |
| 360 | 30 frames, qualquer ordem dos 4 cardeais |
| Negative edge | Ativo para especiais, desligável nas opções |
| Prioridade | Super > reforçado > especial > command normal > normal |
| Buffer de levantamento | 6 frames |

## 4. Movimento

Walk, dash, backdash (invulnerável a golpes nos primeiros frames, vulnerável a arremesso), salto (≈ 40 frames no ar),
**hop** (≈ 28 frames, sem air guard), corrida (só rushdown), agachar, guarda alta/baixa. **Sem air guard** no slice.
Knockdown com opções de levantamento: rápido, atrasado, rolamento para trás.

## 5. Ataques e frame data

Estrutura de cada golpe (contrato de dados):

```
id, animation_ref, vfx_refs, sfx_refs
startup, active, recovery
damage, chip, hitstun, blockstun, hitstop_hit, hitstop_block
pushback_hit, pushback_block, knockback (vx, vy), launch, wall/corner behavior
hit_level: high | mid | low | overhead | throw | unblockable
cancel_windows: [{from, to, into: [normal|special|super|jump|arrebentacao]}]
invuln: [{from, to, type: strike|throw|projectile}]
armor: [{from, to, hits}]
cost: {super_permil, mare}
scaling: {starter_permil, min_permil}
boxes: [{from, to, kind: hit|hurt|push|throw, x, y, w, h}]
counter_hit: {extra_hitstun, extra_damage_permil}
```

Faixas iniciais de referência (ajustadas por teste):

| Tipo | Startup | Vantagem no bloqueio |
|---|---|---|
| Leve | 4–5 | +1 a −1 |
| Médio | 6–9 | −1 a −4 |
| Pesado | 10–14 | −3 a −8 |
| Arremesso | 5 | — (tech: 7 frames após o início) |

**Hitstop:** leve 9, médio 11, pesado 13 frames; em multi-hit, golpes seguintes com 60% do valor; teto de 18 frames somados por golpe.

## 6. Combos

- Links, chains (leve→leve; leve→médio→pesado só para alguns personagens), cancels para especial/super, Arrebentação.
- **Counter hit:** +2 frames de hitstun. **Punish counter** (atingir em recovery): +4 frames e Maré a favor.
- **Escala:** 100%, 100%, 80%, 70%, 60%, 50%… mínimo 20% (Super: mínimo 40%). Início com leve: −10% adicional.
- **Juggle:** contador de juggle por golpe; teto global evita infinitos.
- Busca automatizada de combos no CI para detectar infinitos e dano acima do teto.

## 7. Recursos

Vida, Super meter (2 barras), **Maré** compartilhada. Regras completas em [06](06-mecanica-original.md).

## 8. Personagens e arena

Nadja Ferraz (rushdown) e Otto Brand (grappler); arena Dique Seco 7. Ver [07](07-direcao-artistica.md).

## 9. Modos

Slice: Versus local, Treino. Futuro: Arcade, Tutorial, Online (rollback), Equipes 3v3 (decisão de Fase 6), Replays.

## 10. UI/UX

Navegável 100% por controle e teclado; heurísticas de Nielsen (visibilidade de estado, consistência,
prevenção de erro); tipografia própria; nenhum texto cortado; contraste AA. HUD minimalista com a Maré ao centro.

## 11. Áudio

Trilha por arena; SFX em camadas por golpe (impacto, material, voz); sons disparados só em frames confirmados
(compatível com rollback).

## 12. Ferramentas internas

Editor de golpes (plugin Godot sobre os arquivos de dados), visualizador de colisões, frame analyzer
(estado, input history, vantagem, cancel windows), training mode, replay runner, sync test.

## 13. Testes

Ver tabela de módulos em [05](05-engine-e-arquitetura.md#42-módulos-do-núcleo-e-contratos) e critérios em [08](08-vertical-slice.md).
