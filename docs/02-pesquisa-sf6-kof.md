# 02 — Pesquisa: Street Fighter 6 e The King of Fighters XV

Marcadores de evidência: ver [README](README.md#convenção-de-evidência). Fontes em [fontes.md](fontes.md).

Importante: nada aqui descreve a **implementação interna** desses jogos. Descrevemos comportamento
observável e regras publicadas. O "como implementar" é sempre a **nossa** proposta.

---

## Parte A — Fundamentos universais de jogos de luta

Estes conceitos são comuns ao gênero e formam o vocabulário do nosso frame data.

| Conceito | Definição operacional (como vamos implementar) | Evidência |
|---|---|---|
| Frame | 1/60 s. Toda a lógica competitiva avança em passos inteiros de 1 frame | [S] (Eventhubs, SF6 a 60 fps) |
| Startup | Frames entre o input aceito e o primeiro frame ativo (contamos o primeiro ativo como parte do startup, convenção usada por Capcom) | [C] |
| Active | Frames em que a hitbox existe | [C] |
| Recovery | Frames após o último ativo até poder agir | [C] |
| Hitstop | Congelamento de ambos os personagens no contato; contadores de estado pausam, inputs continuam bufferizados | [S] (critpoints.net; itch.io) |
| Hitstun / Blockstun | Frames em que o defensor não pode agir após hit/bloqueio | [C] |
| Frame advantage | `(hitstun ou blockstun do defensor) − (active restantes + recovery do atacante)`. Calculado pelo próprio simulador, nunca à mão | [C] |
| Input buffer | Janela em que um input é guardado para ser executado assim que possível | [S] (Eventhubs SF6 inputs) |
| Negative edge | Soltar um botão conta como ativação para comandos especiais | [C] |
| Combo scaling | Redução progressiva de dano por hit dentro do combo | [S] |
| Okizeme | Pressão sobre o oponente levantando do chão | [C] |

**Lição de comunidade [S]:** hitstop é frequentemente percebido como "lag" por jogadores de indies
quando é longo demais ou se acumula em golpes multi-hit (relatos em itch.io). Medida: hitstop por golpe
explícito nos dados, teto por combo, e redução automática em multi-hits (ver GDD).

---

## Parte B — Street Fighter 6

### B1. Drive System (estudo de princípio, não de cópia)

| Elemento | O que se observa | Evidência |
|---|---|---|
| Medidor | Barra única "Drive" de 6 segmentos, regenerativa | [S] (dashfight, dexerto) |
| Drive Impact | Golpe com armadura, custa 1 barra | [S] (eventhubs, esports.gg) |
| Drive Parry | Defesa ativa que drena enquanto mantida; parry perfeito gera punição forte e lentidão | [S] (dexerto, esports.gg) — janela exata não confirmada |
| Drive Rush | 1 barra após Parry; 3 barras quando cancelado de um normal | [S] (wepc, eventhubs) |
| Drive Reversal | Contra-ataque a partir do bloqueio, 2 barras | [S] |
| Overdrive | Versão reforçada de especial, 2 barras | [S] |
| Burnout | Barra vazia: sem ações de Drive até recuperar; defensor fica vulnerável | [S] (eventhubs) |

**Princípio técnico extraído [H]:** o Drive System une **ataque, defesa e mobilidade em um único
recurso**. Cada uso ofensivo reduz a capacidade defensiva. Isso cria decisões contínuas, e o Burnout
é a punição que torna a economia significativa.

**Problemas que o sistema introduz [S/H]:** discussão comunitária extensa sobre Drive Impact ser
dominante em certos níveis de jogo e sobre a dificuldade de reagir a ele (ex.: artigo Eventhubs
"Tokido anti Drive Impact"). Para nós: qualquer golpe com armadura universal precisa de resposta
clara, legível e treinável.

**O que adaptamos:** a ideia de **um recurso que conecta ataque e defesa**. **O que não adaptamos:**
os cinco usos específicos, a armadura universal e a mecânica de Burnout. Nossa proposta é diferente
(ver [06](06-mecanica-original.md)).

### B2. Combo e dano

- Escala por hit em degraus, com mínimos garantidos para supers; SF6 tem tipos múltiplos de escala
  (combo, inicial, imediata, multiplicador) [S] (Eventhubs, diamondlobby; números variam entre fontes).
- Inícios com leves encadeados recebem escala adicional [S].
- Counter Hit e Punish Counter aumentam hitstun/dano; Punish Counter recompensa punir recovery [C].

**Como implementar (nosso):** escala como função pura `scale(hit_index, starter_flags) -> permil`
em inteiros, com tabela por personagem opcional e piso mínimo por tipo de golpe. Teste de propriedade:
nenhum combo de N hits pode exceder um teto configurado de dano.

### B3. Controles e acessibilidade

| Elemento | Observação | Evidência |
|---|---|---|
| Classic / Modern / Dynamic | Três esquemas; Modern faz especiais com botão dedicado + direção | [S] (primagames, mp1st) |
| Penalidade do Modern | Especiais assistidos causam menos dano [C] (≈20%, não confirmado) | [C] |
| Buffers de motion | SF6 é 1–3 frames mais estrito que SF5 em geral; meia-lua 12 f, 360° 32 f | [S] (Eventhubs, análise de inputs) |
| Buffer de levantamento | Patch alterou de 4 para 7 frames | [S] (Eventhubs, patch dez/2023) |
| Supers acidentais | Patch dedicado a reduzir inputs acidentais de Super | [S] (Eventhubs set/2023) |

**Lição [H]:** buffer e prioridade de comandos não são detalhe; foram alvo de patches pós-lançamento.
Nosso interpretador terá **tabelas de janela por motion** como dados, prioridade explícita e testes de
regressão de inputs (sequências gravadas → comando esperado).

**Acessibilidade sem perda de profundidade [H]:** oferecer esquema simplificado é viável se o custo
for explícito e equilibrado (menos dano ou menos opções), nunca comportamento escondido.

### B4. Apresentação

- Personagens com silhuetas muito distintas, poses de idle carregadas de personalidade [C].
- Efeitos de impacto com cor por tipo de ação (ex.: Drive em verde), feedback forte de Punish Counter [C].
- Ferramentas de treino extensas: frame meter visível durante o jogo, gravação de dummy [C].

**O que adaptamos:** o **frame meter** em treino é obrigatório no nosso projeto (ferramenta e ensino).

---

## Parte C — The King of Fighters XV

### C1. Movimentação

| Elemento | Observação | Evidência |
|---|---|---|
| Run (corrida) em vez de dash fixo | Avanço contínuo, ritmo veloz | [C] |
| Quatro alturas de salto | Hop, salto normal, hyper hop, hyper jump | [C] (frame counts não obtidos: dreamcancel bloqueado) |
| Rolamento (Emergency Evasion) | Evasão invulnerável a golpes, vulnerável a arremesso | [C] |

**Princípio técnico [H]:** KOF mantém o ritmo alto com **muitas opções de aproximação aérea baixa**
(hops) contra as quais anti-aéreos tradicionais são menos confiáveis. Isso desloca o neutral do chão
para o ar baixo.

**O que adaptamos:** um **salto curto** (hop) com arco baixo, mais curto que o salto normal, e
corrida para personagens rushdown. **Não adaptamos** as quatro alturas: duas alturas mais corrida
dão variedade sem multiplicar o trabalho de animação (cada altura exige animações autorais próprias).

### C2. Recursos e mecânicas

| Elemento | Observação | Evidência |
|---|---|---|
| Power Gauge em estoques | Estoque máximo aumenta conforme a posição na equipe (Point/Middle/Anchor) [C] | [C] |
| MAX Mode | Estado temporário de reforço, indicador aparece com 2+ estoques | [S] (dreamcancel via busca) |
| Quick MAX | Ativado cancelando um golpe; duração menor e sem bônus de dano | [S] |
| Shatter Strike | Contra-golpe a partir da guarda, 1 barra, devolve meia barra ao acertar | [S] |
| Guard Cancel roll / blowback | 1 barra cada | [S] |
| Rush combo | Combo automático com botão leve repetido | [C] |
| Guard Crush | Medidor de guarda que, esgotado, deixa o defensor vulnerável | [C] |

**Princípio [H]:** o 3v3 transforma o medidor em **recurso de equipe**: perder um lutador dá mais
recurso ao seguinte, o que cria viradas.

**O que adaptamos para o vertical slice:** nada do 3v3 ainda (escopo). O formato de equipe fica
como **decisão de Fase 6**, e a arquitetura de `MatchManager` já suporta "N lutadores por lado"
para não fechar a porta.

### C3. Direção artística

- KOF XV usa modelos 3D com pose e silhueta herdadas de sprites clássicos, câmera lateral com
  cortes cinematográficos nos supers [C].

---

## Parte D — Referência artística adicional: Guilty Gear Xrd

Incluída porque resolve exatamente o nosso problema (3D com legibilidade de 2D).

| Descoberta | Evidência |
|---|---|
| Modelos 3D com cel shading feitos para parecer 2D, em Unreal Engine 3 | [S] (Wikipedia, BlenderNation) |
| Animação com **frames cortados deliberadamente** (animação limitada) para parecer sprite | [S] (BlenderNation, mugenguild) |
| No cel shading "a superfície está iluminada ou não"; ruído na superfície vira distração | [S] (citação atribuída ao PDF de Motomura em ggxrd.com, vista via resultado de busca; PDF não aberto) |

**Como aplicar (nosso):** a simulação a 60 Hz já é discreta; a apresentação pode exibir as poses
de golpe com **timing autoral por chave** (stepped keys) controlado pelo animador, não por interpolação
automática. Isso casa perfeitamente com frame data explícito: cada pose-chave do golpe é alinhada a
frames de startup/active/recovery.
