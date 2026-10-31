# 05 — Engine e arquitetura

## 1. Requisitos que decidem a escolha

1. Simulação **determinística a 60 Hz**, independente do FPS, serializável e reexecutável (rollback).
2. **Visual 2.5D** com personagens e cenários 3D autorais, estilizados (cel shading / pintado), legíveis.
3. Rodar a 60 FPS em **PC intermediário** (alvo provisório: GTX 1060 / RX 580, 8 GB RAM, CPU 4 núcleos — a confirmar com o hardware do Kaio).
4. Pipeline artístico viável a partir do **Blender** (ferramenta livre, padrão para indies).
5. Ferramentas internas fáceis de construir (editor de golpes, frame analyzer, training).
6. Build executável fora do editor, Windows primeiro.
7. Equipe pequena: iteração rápida, manutenção simples.

## 2. Alternativas avaliadas

Notas de 1 (fraco) a 5 (forte) para **este projeto**, não em absoluto.

| Critério (do brief) | Unreal Engine 5 | Godot 4 (+ GDExtension C++) | Framework pronto (Night Sky / Sakuga / Castagne) | Engine própria (C++ + renderer) |
|---|---|---|---|---|
| 1. Qualidade de renderização | **5** — melhor renderer disponível; Nanite/Lumen dispensáveis para estilo cel [S] | 4 — Forward+ com GI e fog volumétrico; suficiente para estilizado [S] | Herda a engine base | 2 — renderer seria um projeto em si |
| 2. Ferramentas de modelagem/animação | 5 — Control Rig, Sequencer para cinemáticas | 3 — AnimationPlayer/Tree; cinemáticas precisam de ferramenta nossa; importa glTF do Blender nativamente | Herda | 1 |
| 3. Previsibilidade da simulação | 3 — tick e física da engine não servem para rollback (relatos [S]); exige simulação própria | 4 — também exige simulação própria, mas a engine é pequena e fácil de isolar | 3–4 — já resolvido, mas preso às escolhas do framework | 5 |
| 4. Suporte a rollback | 4 — Night Sky prova GGPO + UE5 [V] | 4 — addons existentes [V]; com núcleo próprio, independe da engine | 4 — embutido | 4 — tudo nosso |
| 5. Desempenho em PC intermediário | 3 — engine pesada; precisa de configuração cuidadosa (Night Sky mantém branch `lowend` [V]) | **5** — leve | Herda | 5 |
| 6. Complexidade de desenvolvimento | 2 — C++ da engine, compilação lenta, builds grandes | **4** | 3 — aprender e contornar o framework | 1 |
| 7. Ferramentas internas | 4 — Slate/Editor Utility, mas pesado | **5** — editor extensível com plugins, UI rápida | 3 | 2 |
| 8. Exportação/distribuição | 4 — maduro; royalty de 5% acima de US$ 1 mi [S] | 5 — MIT, sem royalty, exportação Windows simples | Herda | 3 |
| 9. Manutenibilidade | 3 | 4 | 2 — dependência de mantenedor único | 2 |
| 10. Viabilidade artística | 4 — pipeline FBX/glTF, mas iteração mais lenta | 4 — glTF direto do Blender | Herda | 2 |
| **Soma** | **37** | **42** | — | **27** |

Unity não entrou no topo: não foi pedido, não traz vantagem técnica decisiva sobre Godot para este
caso e o licenciamento passou por mudanças recentes que adicionam risco ao projeto [C].

## 3. Decisão recomendada (precisa da aprovação do Kaio)

> **Núcleo de simulação próprio em C++20, independente de engine, + Godot 4 como camada de
> apresentação, integrado via GDExtension.**

Por quê:

- O requisito mais difícil é o determinismo. **Nenhuma engine o entrega pronto**; Unreal e Godot
  exigem a mesma coisa: uma simulação própria isolada da física e do tick da engine. Se o núcleo for
  nosso e não depender da engine, a engine vira "só" renderização, áudio, input do SO e UI.
- Com o núcleo em C++ puro, **testamos tudo headless** (inclusive aqui no container, que tem gcc/clang/cmake),
  com sync test de rollback no CI.
- Godot é leve, livre (MIT), rápido para iterar e construir ferramentas, e importa glTF do Blender
  sem conversões. Para um visual estilizado/cel shading (referência Guilty Gear Xrd), o renderer
  do Godot 4 é suficiente; o diferencial visual virá da arte e dos shaders autorais, não de Nanite/Lumen.
- **Saída de emergência:** se a arte exigir algo que Godot não entrega, o núcleo C++ se integra a
  Unreal (como faz Night Sky Engine). A decisão de engine fica reversível para tudo que é lógica.

Quando Unreal seria a escolha certa: se houver um artista técnico experiente em Unreal na equipe,
se o hardware-alvo for alto e se cinemáticas pesadas (Sequencer) forem centrais. Se o Kaio preferir
Unreal, a arquitetura abaixo continua válida sem mudanças no núcleo.

Frameworks prontos foram descartados como base: Sakuga é 2D/1v1 e sem cinemáticas [V]; Castagne
é MPL e menos documentado no que pude verificar; Night Sky está preso a UE5.7. Todos serão **estudados**
como referência de código na Fase 2.

## 4. Arquitetura

```
┌──────────────────────── Apresentação (Godot 4) ─────────────────────────┐
│ Render 3D, câmera, animação, VFX, áudio, HUD/UI, menus, debug overlay    │
│   lê SimState (somente leitura)  ←  eventos de apresentação (hit, som)   │
└───────────────▲─────────────────────────────────────────────┬───────────┘
                │ GDExtension (ponte fina, sem lógica de jogo) │ InputFrame
┌───────────────┴──────────── Núcleo `fgcore` (C++20) ────────▼───────────┐
│ InputManager → CommandInterpreter → CharacterFSM → MoveData → Collision  │
│ → Damage/Combo → Resources(meter/mecânica) → MatchManager (rounds, timer)│
│ SimState (POD, sem ponteiros) · RNG determinístico · Checksum · Snapshot │
│ Replay (inputs + seed) · RollbackSession (API estilo GGPO)               │
└──────────────────────────────────────────────────────────────────────────┘
   Ferramentas: fgdata (validador/compilador de dados), frame analyzer,
   sync-test runner, replay runner, editor de golpes (plugin Godot)
```

### 4.1 Arquitetura determinística

| Regra | Implementação |
|---|---|
| 60 atualizações fixas por segundo | Acumulador de tempo na apresentação chama `fgcore::step(inputs)` 0..N vezes por quadro de render; a simulação nunca lê o relógio |
| Sem ponto flutuante na lógica | Posições e velocidades em **inteiros** (1 unidade = 0,1 mm). Plano de luta 2D (x, y); sem trigonometria na lógica |
| RNG determinístico | Xorshift/PCG com estado dentro do `SimState` |
| Estado serializável | `SimState` é um struct POD de tamanho fixo, sem ponteiros (referências por índice); snapshot = cópia de memória |
| Checksum | Hash de 64 bits do `SimState` por frame; usado no sync test, replays e detecção de desync online |
| Dados imutáveis fora do estado | Tabelas de golpes carregadas uma vez; o estado só guarda `move_id` e `frame` |
| Apresentação nunca altera a simulação | Godot recebe o estado e eventos; sons/VFX disparados só para frames confirmados (lição do GGPO [V]) |
| Animação segue a simulação | O clipe é amostrado no frame exato do golpe (`seek`), com chaves em degrau para golpes. Root motion é **assado em dados** a partir do Blender por ferramenta, nunca lido da animação em runtime |
| Sem física da engine | Colisão própria: AABBs inteiras (hitbox, hurtbox, pushbox, throwbox) por intervalo de frames |

### 4.2 Módulos do núcleo e contratos

| Módulo | Responsabilidade | Testes obrigatórios |
|---|---|---|
| `input` | `InputFrame` (u16: 4 direções + 6 botões), histórico circular, normalização por lado | Normalização ao trocar de lado; inputs simultâneos |
| `command` | Reconhecimento de motions (236, 623, 214, 41236, 360, cargas), buffer por comando, negative edge, prioridade | Sequências gravadas → comando esperado; não reconhecer supers acidentais |
| `fsm` | Estados do personagem (idle, walk, crouch, jump, attack, hitstun, blockstun, knockdown, wakeup…) e transições por dados | Transições válidas; sem estado "preso" |
| `moves` | Frame data por golpe: startup, active, recovery, dano, hitstun, blockstun, hitstop, pushback, cancel windows, nível (alto/médio/baixo/arremesso), invulnerabilidade, armadura, custo, escala, caixas por intervalo, referências a animação/VFX/SFX | Validador: dados consistentes; caixas dentro dos intervalos |
| `collision` | Detecção e resolução com prioridade (throw vs strike, trade, projéteis), empurrão, limites de arena e canto | Prioridade; trade; empurrão no canto |
| `combat` | Hit/block, counter hit, punish counter, combos, escala, juggle, limites | Frame advantage calculado = esperado; nenhum combo ultrapassa teto; detecção de infinitos por busca automatizada |
| `resources` | Super meter e mecânica original | Custos, ganho, limites |
| `match` | Rounds, tempo, KO, vitória, reset, N lutadores por lado (preparado para equipe) | Regras de round; empate |
| `rollback` | Snapshots circulares, resimulação, API estilo GGPO (save/load/advance) | **Sync test** a cada frame no CI |
| `replay` | Gravar seed + inputs; reproduzir e conferir checksum | Replays "dourados" quebram o CI se o comportamento mudar sem querer |

### 4.3 Dados

- Golpes e personagens descritos em arquivos de texto versionados (`data/characters/<id>/*.json`).
- `fgdata` valida esquema e regras (ex.: hitbox fora de frame ativo, cancel window impossível) e compila em
  binário compacto carregado pelo núcleo.
- O editor de golpes (plugin do Godot) edita esses arquivos com preview da animação e das caixas.

### 4.4 Estrutura do repositório (Fase 2)

```
core/           fgcore (C++20, sem dependência de engine) + testes
tools/          fgdata, sync-test, replay-runner, frame-analyzer, scripts Blender
data/           personagens, golpes, regras (texto)
game/           projeto Godot 4 (apresentação, UI, ferramentas de editor)
game/addons/fgbridge/   GDExtension
art-source/     arquivos-fonte editáveis (.blend, .kra/.psd, .spp) — via Git LFS
docs/
```

### 4.5 Qualidade e CI

- CMake + CTest, `-Wall -Wextra -Werror`, sanitizers (ASan/UBSan) em debug.
- CI no GitHub Actions: build Linux e Windows, testes unitários, sync test, replays dourados.
- Teste de determinismo cruzado: mesmo replay em Linux (gcc) e Windows (MSVC) deve gerar o mesmo checksum.

## 5. Rollback: GGPO ou próprio?

- GGPO é MIT e maduro, mas seu build oficial é só Windows [V]. Nossa API do núcleo segue o formato de
  callbacks do GGPO (save/load/advance), então **qualquer** das duas opções se encaixa.
- Recomendação: implementar a sessão de rollback local (sync test) na Fase 2, decidir a camada de rede
  (GGPO integrado ou implementação própria sobre UDP) quando o offline estiver estável. Online não faz
  parte do vertical slice.

## 6. Riscos

| Risco | Mitigação |
|---|---|
| Determinismo quebrar entre compiladores | Inteiros apenas na lógica; teste cruzado Linux/Windows no CI |
| Godot não atingir a qualidade visual desejada | Spike de renderização na Fase 3 com um modelo real do artista; núcleo continua válido para Unreal |
| Gargalo de produção artística (maior risco do projeto) | Escopo do vertical slice reduzido em quantidade, não em qualidade; lista de dependências em [07](07-direcao-artistica.md) |
| Ponte GDExtension adicionar latência | Medição de latência de input fim a fim no build |
