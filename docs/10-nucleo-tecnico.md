# 10 — Núcleo técnico (Fase 2)

Este documento descreve o que **existe e roda** no repositório: o núcleo de simulação `fgcore`,
os dados dos dois lutadores, a ferramenta de linha de comando `fgsim` e a bateria de testes.
Tudo aqui foi executado nesta sessão (Linux, gcc 13.3 e clang 18); as limitações estão no fim.

Não há camada visual ainda. Pela preferência do Kaio, o Godot será usado só como camada fina de
apresentação (ou substituído); por isso o núcleo não depende de nenhuma engine.

## 1. Como compilar e rodar

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure      # 70 testes
./build/tools/fgsim/fgsim validate data          # valida os dados dos personagens
./build/tools/fgsim/fgsim framedata data         # tabela de frame data (seção 8)
./build/tools/fgsim/fgsim synctest data 7200 8 3 # rollback: 2 minutos, volta 8 frames
./build/tools/fgsim/fgsim checksum data 7200 9   # checksum final de uma partida aleatória
./build/tools/fgsim/fgsim record data a.fgr 3600 5 && ./build/tools/fgsim/fgsim verify data a.fgr
./build/tools/fgsim/fgsim trace data nadja otto tools/traces/punish_counter.txt
```

Opções do CMake: `FG_WARNINGS_AS_ERRORS` (padrão ON; `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`)
e `FG_SANITIZE` (AddressSanitizer + UBSan, usar com gcc).

A CI (`.github/workflows/ci.yml`) compila em Ubuntu e Windows (Debug e Release), roda os testes,
o validador, o sync test, uma build com sanitizers e um job que **compara o checksum final entre
Linux e Windows**: se divergirem, a CI falha. Esse é o teste real de determinismo entre plataformas.

## 2. Estrutura

| Caminho | Conteúdo |
|---|---|
| `core/include/fg/` | API pública do núcleo |
| `core/src/` | Implementação: `sim.cpp` (regras), `command.cpp` (leitura de comandos), `loader.cpp` + `json.cpp` (dados), `serialize.cpp`, `rollback.cpp`, `replay.cpp`, `analyzer.cpp` |
| `core/tests/` | Testes (sem dependências externas) |
| `data/rules.json` | Regras globais e da Maré |
| `data/roster.json`, `data/characters/*.json` | Elenco e personagens (golpes, caixas, comandos) |
| `tools/fgsim/` | Ferramenta de linha de comando |
| `tools/traces/` | Roteiros de análise frame a frame |

## 3. Convenções

- **Só inteiros.** Nenhum `float` toca o estado do jogo. Distância em 0,1 mm (10 000 = 1 m),
  velocidade em unidades por frame, porcentagens em permil (1000 = 100%).
- **60 Hz fixo.** `step(state, data, input)` avança exatamente um frame. A apresentação interpola,
  nunca decide.
- **Estado = struct POD.** `MatchState` é trivialmente copiável (`static_assert`): um snapshot
  de rollback é uma cópia de memória. Não há ponteiros, alocação ou `std::` dinâmico no estado.
- **Serialização explícita** little-endian campo a campo, com checksum FNV-1a 64. Não depende do
  layout de memória do compilador, então o checksum é comparável entre Windows e Linux.
- **Numeração de frames do golpe:** o frame 1 é o primeiro frame do golpe. `startup` = número do
  primeiro frame ativo (um 5LP de startup 4 acerta no 4º frame após o aperto). Medido por teste.
- **Vantagem:** `hitstun (ou blockstun) − (ativos restantes + recovery)`, calculada pelo analisador
  **por simulação** (dois lutadores reais, contato real), não por fórmula escrita à mão.
- **Lado:** caixas são descritas para o lutador olhando para a direita e espelhadas por `facing`.
  Comandos são lidos em notação numérica relativa ao lado (6 = para a frente).
- **SOCD:** esquerda+direita = neutro; cima+baixo = cima.

## 4. Ordem de um frame (`step`)

1. Registra os inputs no histórico de 64 frames de cada lutador.
2. Fase do round (introdução, luta, fim de round, fim de partida).
3. Congelamento de super (só o atacante do super avança a animação de ativação).
4. Hitstop: enquanto houver, ninguém avança; os botões apertados nesse período ficam no buffer
   (o buffer é estendido pelo hitstop para **os dois** lutadores).
5. Decisão de cada lutador (andar, saltar, golpe, comando, cancelamento), na mesma passada para P1 e P2.
6. Avanço físico (velocidades do golpe, gravidade, empurrão).
7. Arremessos em andamento.
8. Resolução de posições (os corpos não se atravessam; limites da arena e distância máxima).
9. Lado (facing), só quando ambos estão no chão e livres.
10. Contatos: **todos os contatos do frame são calculados antes de qualquer um ser aplicado**, por
    isso trocas são simétricas e a ordem P1/P2 nunca muda o resultado.
11. Ganhos passivos da Maré.
12. Treino: recarga de vida (opcional).
13. KO e fim de tempo.

### Prioridade de contato

| Situação | Resultado |
|---|---|
| Golpe × arremesso no mesmo frame | O golpe vence |
| Arremesso × arremesso | Tech mútuo (os dois são separados) |
| Golpe × golpe no mesmo frame | Troca: os dois são atingidos, ambos como counter hit |
| Golpe contra startup do oponente | Counter hit (+2 de hitstun, dano ×1,2) |
| Golpe contra recovery do oponente | Punish counter (+4 de hitstun, dano ×1,2, Maré +200) |
| Armadura ativa | Absorve um golpe, toma o dano, sem hitstun |
| Arremesso contra quem está em blockstun ou hitstun | Não pega |
| Arremesso normal | Janela de tech de 7 frames (LP+LK) |
| Agarrão de comando (`techable: false`) | Sem tech |

### Combos

- Escala por hit: 100%, 100%, 80%, 70%, 60%, 50%, 40%, 30%, 20% (mínimo 20%).
  Golpe leve como início aplica uma escala extra.
- Decaimento de hitstun a partir do 5º hit (mínimo 6 frames).
- Limite de juggle: 6 pontos. Gravidade extra em juggle.
- Teto de dano por combo: 60% da vida máxima do defensor.
- Teste de força bruta: todas as repetições de golpe, no meio e no canto, terminam (sem infinito).

## 5. Maré (implementação da mecânica original)

Regras em `data/rules.json → mare`. Medidor compartilhado de −1000 a +1000 (positivo = P1).

| Evento | Variação |
|---|---|
| Andar para a frente | +1 por frame |
| Dash/corrida para a frente | +20 |
| Golpe bloqueado | +40 para o atacante |
| Golpe que acerta | +60 |
| Counter hit | +120 |
| Punish counter | +200 |
| Arremesso | +100 |
| Encurralar no canto | +2 por frame |
| Recuar por mais de 90 frames | −2 por frame para quem recua |

- **Arrebentação** (Maré ≥ +500 a seu favor): cancelamento universal de qualquer normal que fez
  contato (acerto ou bloqueio), consome a Maré até 0. Pode virar especial ou pressão.
- **Contracorrente** (Maré ≤ −500 contra você): sai do blockstun, empurra o atacante para longe,
  não causa dano, zera a Maré e tem recovery punível se for lida.

Os números são provisórios. O próximo passo de validação está em [06](06-mecanica-original.md):
partidas entre bots com estilos diferentes para medir bola de neve e taxa de virada.

## 6. Rollback e replays

- `RollbackSession` (estilo GGPO): guarda snapshots, prevê o input remoto repetindo o último,
  e ao receber o input real diferente volta ao frame e ressimula. Testado com dois pares, atraso e
  jitter: o estado final é idêntico ao da partida offline.
- `run_sync_test`: a cada frame volta N frames e ressimula, comparando checksums.
  Passa para N de 1 a 8 em 7200 frames.
- Replays: inputs + checksum periódico + hash dos dados do jogo. Um replay feito com outros dados
  (outro balanceamento) é rejeitado; um arquivo corrompido é rejeitado.
- **Ainda não existe transporte de rede** (UDP, NAT, lobby). O núcleo está pronto para ele.

## 7. Personagens nos dados

| | Nadja Ferraz | Otto Brand |
|---|---|---|
| Arquétipo | Rushdown | Grappler |
| Vida | 9 500 | 11 000 |
| Mobilidade | Corrida (66) | Dash curto (66) |
| Ferramentas | 236P Coice (avanço), 236K rasteira, 214K, 623P reversal, 6MP overhead, super 236236K | 360P agarrão de comando sem tech, 236P com armadura, 214K baixo, 623P, 5HP com armadura, 6HP overhead, super 236236P |

Os dois têm o esquema **Clássico** (6 botões) e o **Simplificado** (SP + direção faz especial,
com dano a 80%).

## 8. Frame data gerada pelo simulador

Gerada com `fgsim framedata data` (valores medidos por simulação contra o outro personagem).

### Nadja Ferraz (rushdown) — contra Otto Brand

| Golpe | Startup | Ativo | Recovery | Total | Dano | No hit | No bloqueio | Medidor | Observação |
|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| 5LP | 4 | 2 | 7 | 12 | 270 | +4 | +1 | `---##.......` |  |
| 5MP | 6 | 3 | 12 | 20 | 600 | +3 | -2 | `-----###............` |  |
| 5HP | 10 | 3 | 18 | 30 | 900 | +2 | -4 | `---------###..................` |  |
| 5LK | 5 | 2 | 9 | 15 | 270 | +3 | -1 | `----##.........` |  |
| 5MK | 8 | 3 | 15 | 25 | 700 | +1 | -4 | `-------###...............` |  |
| 5HK | 12 | 4 | 19 | 34 | 1000 | +3 | -5 | `-----------####...................` |  |
| 2LP | 4 | 2 | 7 | 12 | 252 | +4 | +1 | `---##.......` |  |
| 2MP | 6 | 3 | 11 | 19 | 550 | +3 | -1 | `-----###...........` |  |
| 2HP | 8 | 4 | 20 | 31 | 850 | -1 | -9 | `-------####....................` |  |
| 2LK | 5 | 2 | 8 | 14 | 225 | +3 | -1 | `----##........` |  |
| 2MK | 7 | 3 | 14 | 23 | 600 | +1 | -3 | `------###..............` |  |
| 2HK | 9 | 3 | 22 | 33 | 900 | +36 | -12 | `--------###......................` | derruba |
| j.LP | 4 | 6 | 4 | 13 | — | — | — | `---######....` | golpe aéreo: medir em cenário de salto |
| j.MP | 6 | 4 | 6 | 15 | — | — | — | `-----####......` | golpe aéreo: medir em cenário de salto |
| j.HP | 8 | 4 | 8 | 19 | — | — | — | `-------####........` | golpe aéreo: medir em cenário de salto |
| j.LK | 4 | 8 | 4 | 15 | — | — | — | `---########....` | golpe aéreo: medir em cenário de salto |
| j.MK | 6 | 5 | 6 | 16 | — | — | — | `-----#####......` | golpe aéreo: medir em cenário de salto |
| j.HK | 9 | 4 | 8 | 20 | — | — | — | `--------####........` | golpe aéreo: medir em cenário de salto |
| 6MP | 18 | 2 | 14 | 33 | 600 | +0 | -4 | `-----------------##..............` |  |
| 236P | 12 | 8 | 16 | 35 | 400 | +42 | -2 | `-----------########................` | derruba |
| 236PP | 10 | 8 | 14 | 31 | 500 | +75 | +1 | `---------########..............` | derruba |
| 236K | 10 | 8 | 18 | 35 | 900 | +35 | -11 | `---------########..................` | derruba |
| 214K | 20 | 3 | 16 | 38 | 900 | +0 | -6 | `-------------------###................` |  |
| 623P | 5 | 6 | 28 | 38 | 1100 | +44 | -31 | `----######............................` | derruba |
| 623PP | 4 | 8 | 30 | 41 | 600 | +48 | -31 | `---########..............................` | derruba |
| 236236K | 7 | 9 | 40 | 55 | 800 | +56 | -22 | `------#########........................................` | derruba |
| throw_f | 5 | 3 | 20 | 27 | — | — | — | `----###....................` | arremesso |
| throw_b | 5 | 3 | 20 | 27 | — | — | — | `----###....................` | arremesso |
| dash | 1 | 1 | 16 | 17 | — | — | — | `#................` | sem hit (movimento) |
| backdash | 1 | 1 | 22 | 23 | — | — | — | `#......................` | sem hit (movimento) |
| arrebentacao | 1 | 1 | 16 | 17 | — | — | — | `#................` | sem hit (movimento) |
| contracorrente | 4 | 3 | 22 | 28 | 0 | -6 | -6 | `---###......................` |  |

### Otto Brand (grappler) — contra Nadja Ferraz

| Golpe | Startup | Ativo | Recovery | Total | Dano | No hit | No bloqueio | Medidor | Observação |
|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| 5LP | 5 | 2 | 8 | 14 | 315 | +4 | +1 | `----##........` |  |
| 5MP | 8 | 3 | 14 | 24 | 750 | +2 | -3 | `-------###..............` |  |
| 5HP | 13 | 4 | 20 | 36 | 1200 | +1 | -5 | `------------####....................` |  |
| 5LK | 6 | 3 | 9 | 17 | 315 | +2 | -2 | `-----###.........` |  |
| 5MK | 9 | 3 | 16 | 27 | 750 | +0 | -5 | `--------###................` |  |
| 5HK | 14 | 4 | 22 | 39 | 1200 | +35 | -9 | `-------------####......................` | derruba |
| 2LP | 5 | 2 | 8 | 14 | 288 | +3 | +0 | `----##........` |  |
| 2MP | 7 | 3 | 13 | 22 | 650 | +1 | -3 | `------###.............` |  |
| 2HP | 9 | 5 | 24 | 37 | 1100 | -8 | -14 | `--------#####........................` |  |
| 2LK | 6 | 2 | 9 | 16 | 270 | +2 | -2 | `-----##.........` |  |
| 2MK | 8 | 3 | 15 | 25 | 650 | -1 | -5 | `-------###...............` |  |
| 2HK | 11 | 3 | 24 | 37 | 1100 | +34 | -14 | `----------###........................` | derruba |
| j.LP | 5 | 5 | 5 | 14 | — | — | — | `----#####.....` | golpe aéreo: medir em cenário de salto |
| j.MP | 7 | 4 | 7 | 17 | — | — | — | `------####.......` | golpe aéreo: medir em cenário de salto |
| j.HP | 10 | 4 | 9 | 22 | — | — | — | `---------####.........` | golpe aéreo: medir em cenário de salto |
| j.LK | 5 | 6 | 5 | 15 | — | — | — | `----######.....` | golpe aéreo: medir em cenário de salto |
| j.MK | 7 | 4 | 7 | 17 | — | — | — | `------####.......` | golpe aéreo: medir em cenário de salto |
| j.HK | 10 | 5 | 8 | 22 | — | — | — | `---------#####........` | golpe aéreo: medir em cenário de salto |
| 6HP | 20 | 3 | 16 | 38 | 1000 | +2 | -4 | `-------------------###................` |  |
| 360P | 3 | 2 | 30 | 34 | — | — | — | `--##..............................` | arremesso |
| 360PP | 1 | 3 | 30 | 33 | — | — | — | `###..............................` | arremesso |
| 236P | 14 | 4 | 18 | 35 | 1000 | +39 | -7 | `-------------####..................` | derruba |
| 236PP | 12 | 4 | 16 | 31 | 1300 | +69 | -3 | `-----------####................` | derruba |
| 214K | 16 | 3 | 20 | 38 | 900 | +0 | -10 | `---------------###....................` |  |
| 623P | 6 | 5 | 30 | 40 | 1200 | +41 | -29 | `-----#####..............................` | derruba |
| 236236P | 8 | 4 | 45 | 56 | 3600 | +54 | -26 | `-------####.............................................` | derruba |
| throw_f | 5 | 3 | 22 | 29 | — | — | — | `----###......................` | arremesso |
| throw_b | 5 | 3 | 22 | 29 | — | — | — | `----###......................` | arremesso |
| dash | 1 | 1 | 19 | 20 | — | — | — | `#...................` | sem hit (movimento) |
| backdash | 1 | 1 | 24 | 25 | — | — | — | `#........................` | sem hit (movimento) |
| arrebentacao | 1 | 1 | 18 | 19 | — | — | — | `#..................` | sem hit (movimento) |
| contracorrente | 4 | 3 | 24 | 30 | 0 | -8 | -8 | `---###........................` |  |


## 9. Testes (70, todos passando)

Lista gerada pela própria execução de `fgcore_tests`:

- json: objeto, lista, inteiros, texto, comentários
- json: rejeita números fracionários nos dados
- json: rejeita campos duplicados e lixo após o valor
- json: erro informa linha e coluna
- comando: 236+P executa Coice de Moto
- comando: movimento lento demais vira golpe normal
- comando: atalho 323 aceito para 623
- comando: 6236+P prioriza 623 sobre 236
- comando: super exige medidor; sem medidor sai o especial
- comando: reforçado com dois socos consome 1 barra
- comando: direções são relativas ao lado (P1 olhando para a esquerda)
- comando: buffer de 4 frames durante a recuperação
- comando: 360 do Otto sai mesmo terminando em cima (cancela o pré-salto)
- comando: esquema Simplificado faz especial com SP + direção e aplica penalidade
- comando: esquema Clássico ignora comandos do Simplificado
- input: SOCD esquerda+direita = neutro; cima+baixo = cima
- movimento: 66 faz dash no Otto e corrida na Nadja
- movimento: toque rápido para cima = hop; segurar = salto
- comando: negative edge (soltar o botão) quando habilitado
- comando: compile_motion valida sintaxe
- dados: elenco real carrega sem erros
- dados: todos os normais existem e comandos foram resolvidos
- validador: hitbox fora dos frames ativos é rejeitada
- validador: campo desconhecido e golpe inexistente são apontados
- validador: normal obrigatório ausente
- dados: hash muda quando um número de frame data muda
- frame data: o primeiro contato acontece exatamente no frame de startup
- frame data: vantagem simulada = hitstun - (ativos restantes + recovery)
- frame data: valores de design da Nadja (5LP +4/+1, 5MK +1/-4, 2HK punível)
- frame data: reversals são muito puníveis no bloqueio
- frame meter: representação textual do golpe
- defesa: golpe baixo exige defesa agachada
- defesa: overhead exige defesa em pé
- defesa: bloqueio aplica blockstun e empurra o defensor
- contra-ataque: acertar durante o startup do oponente é counter hit
- contra-ataque: acertar na recuperação é punish counter e move a Maré
- armadura: 5HP do Otto absorve um golpe e continua
- arremesso: conecta, abre janela de tech e causa dano
- arremesso: tech com LP+LK dentro da janela anula o dano
- arremesso: tech tardio (fora da janela) não funciona
- arremesso: agarrão de comando do Otto não tem tech
- arremesso: não pega oponente em blockstun
- troca: golpes que conectam no mesmo frame acertam os dois (como counter hit)
- combo: escala de dano e inicio leve
- combo: nenhuma repetição de golpe vira infinito, nem no canto
- invariantes: entradas aleatórias nunca violam limites da arena e dos recursos
- colisão: lutadores andando um contra o outro não se atravessam
- Maré: andar para a frente puxa a Maré para o seu lado
- Maré: recuar por muito tempo afasta a Maré
- Maré: Arrebentação exige Maré alta e só sai como cancelamento de normal que fez contato
- Maré: Arrebentação pode ser cancelada em normal (extensão de pressão)
- Maré: Contracorrente sai do blockstun quando a Maré está contra você
- Maré: Contracorrente indisponível sem Maré contra
- Maré: limite nunca é ultrapassado
- partida: introdução de 90 frames e então 'Lutem'
- partida: inputs durante a introdução são ignorados
- partida: KO encerra o round, conta vitória e reinicia posições e vida
- partida: duas vitórias encerram a partida
- partida: fim do tempo decide pela porcentagem de vida
- super: congelamento cinematográfico para os dois lutadores
- medidor: ganho ao acertar e ao ser acertado; limite máximo
- determinismo: mesmos inputs produzem o mesmo estado
- determinismo: inputs diferentes produzem estados diferentes
- rollback: sync test volta e ressimula a cada frame sem divergência (distâncias 1 a 8)
- rollback: snapshot é uma cópia fiel (carregar e avançar = avançar direto)
- rollback: dois pares com atraso e jitter terminam idênticos à partida offline
- replay: gravar, codificar, decodificar e verificar
- replay: arquivo corrompido é rejeitado
- replay: dados de jogo diferentes invalidam o replay
- serialização: tamanho fixo, independente do conteúdo

## 10. Ferramenta de análise frame a frame (`fgsim trace`)

Um roteiro descreve os inputs de cada jogador por blocos de frames:

```text
# <frames> <input P1> <input P2>   (notação numérica; 5 = neutro)
place 12000     # distância inicial entre os lutadores
1 5 1HK         # Otto aperta HK agachado (2HK) e erra
14 5 5
1 2MK 5         # Nadja pune com 2MK
3 2 5
1 3 5
1 6LK 5         # 236K digitado como 2,3,6+K, cancelando o 2MK
40 5 5
```

A saída mostra, frame a frame, o estado de cada lutador, o frame do golpe, o hitstop e os eventos.
O exemplo `tools/traces/punish_counter.txt` confirma uma rota real: o 2HK do Otto erra,
a Nadja pune com 2MK na perna estendida (PUNISH 720, Maré +200) e cancela em 236K (HIT 900, queda).

## 11. Limitações honestas

- **MSVC não foi compilado nesta sessão** (o ambiente é Linux). A CI compila em Windows e compara
  o checksum com o Linux; enquanto essa CI não rodar verde, a portabilidade não está comprovada.
  Avisos do MSVC ainda não foram auditados (no Windows a CI não trata avisos como erro).
- **Sem camada visual, sem áudio, sem menus.** Nenhuma arte foi feita nem será simulada com
  primitivas. Ver dependências artísticas em [07](07-direcao-artistica.md).
- O analisador mede vantagem de golpes no chão; golpes aéreos ainda não têm vantagem medida.
- Não há botão de Assist no esquema Simplificado (só SP + direção).
- Não há transporte de rede; o rollback foi testado com rede simulada.
- Os números de dano e frame data são de protótipo e precisam de jogo humano para serem ajustados.
