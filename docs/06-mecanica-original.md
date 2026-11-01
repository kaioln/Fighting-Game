# 06 — Mecânica central original: três propostas

Objetivo de design (derivado de [04](04-matriz-sf6-kof.md)): recompensar **ganhar o neutral e o
território**, manter o ritmo alto de KOF sem perder a leitura de SF, e dar ao defensor uma saída
**com custo e contra-jogo claros**. Nenhuma proposta pode ser um Drive System ou MAX Mode renomeado.

Todos os números abaixo são **provisórios**: servem para o protótipo da Fase 2 e serão ajustados por teste.

---

## Proposta 1 — **Maré** (medidor compartilhado de território)

**Identidade visual:** uma faixa única no centro do HUD, entre as barras de vida, onde uma "linha
d'água" se desloca para o lado de quem domina. Ondas pintadas à mão, cor de cada jogador. Na arena,
a iluminação ambiente reage sutilmente (reflexos de água no chão do estaleiro), sem tocar na legibilidade.

**Regras**
- Um medidor **compartilhado** de −100 a +100 (positivo = lado P1). Começa em 0 a cada round.
- Empurra a Maré para o seu lado: avançar sem ser atingido (pouco), ter golpes bloqueados (médio),
  **whiff punish e punish counter** (muito), encurralar o oponente no canto (contínuo, pouco).
- Afasta a Maré de você: ser atingido, ser arremessado, recuar continuamente por mais de ~1,5 s.
- **Maré Alta** (≥ +50 a seu favor) libera **Arrebentação**: consome a Maré até 0 e permite um
  cancelamento de qualquer normal em avanço rápido ou em especial (extensão de combo ou de pressão).
- **Maré Baixa** (≤ −50 contra você) libera **Contracorrente**: a partir do blockstun, um empurrão
  defensivo (guard cancel) que zera a Maré e devolve espaço. Não causa dano.

| Aspecto | Descrição |
|---|---|
| Objetivo competitivo | Transformar vitória no neutral em recurso; desestimular jogo passivo |
| Custo e recuperação | Não há custo de barra própria; o recurso é a posição. Usar Arrebentação "devolve" a vantagem acumulada |
| Uso ofensivo | Arrebentação estende pressão/combos de quem domina |
| Uso defensivo | Contracorrente para quem está sendo dominado |
| Contra-jogo | Ao ver Maré Alta do oponente, bloquear e esperar a Arrebentação (que é visível e punível se mal usada); Contracorrente é punível se lida (tem recovery) |
| Riscos | Efeito "bola de neve" se o lado dominante ganhar demais; leitura difícil se o medidor oscilar rápido |
| Vantagens | Um único medidor legível para os dois jogadores; reforça footsies; tema conecta com a arena e com a identidade do jogo |
| Desvantagens | Interação com o Super meter precisa ser cuidadosa para não haver dois recursos concorrendo |
| Balanceamento | Poucos parâmetros globais (taxas de ganho por evento, limiares). Simulação automática de partidas consegue medir bola de neve |

Precedente honesto [C]: Guilty Gear penaliza recuo excessivo ("Negative Penalty"). O que é novo aqui é o
medidor **compartilhado** como território, com ferramenta ofensiva e defensiva nos extremos opostos.

---

## Proposta 2 — **Fôlego** (resistência individual)

**Identidade visual:** respiração visível do personagem (animação autoral de ofegar) e anel discreto ao redor do retrato.

**Regras:** cada lutador tem Fôlego (0–100) que é gasto por corrida, hop, bloqueio de golpes pesados e por
uma esquiva curta invulnerável a golpes. Regenera parado ou atacando. Em 0: guarda frágil (blockstun maior) por 3 s.

| Aspecto | Descrição |
|---|---|
| Objetivo | Limitar mobilidade e defesa infinitas |
| Ofensivo / defensivo | Pressão drena o Fôlego do defensor; esquiva gasta Fôlego |
| Contra-jogo | Gerenciar o recurso, não bloquear tudo |
| Riscos | Muito próximo, em efeito, de Guard Crush (KOF) e Burnout (SF6) |
| Vantagens | Fácil de entender; exige animações de cansaço (expressivas) |
| Desvantagens | **Pouco original**; pune a defesa mais do que recompensa a habilidade |
| Balanceamento | Simples, mas tende a favorecer rushdown em todo o elenco |

---

## Proposta 3 — **Marcas** (selos por personagem)

**Identidade visual:** golpes "marcadores" deixam um selo no corpo do oponente (decalque animado, específico de cada personagem).

**Regras:** cada personagem tem 2 ou 3 golpes que aplicam uma Marca (máx. 3, expiram em 6 s). Gastar Marcas
dispara um efeito **próprio do personagem** (ex.: o rushdown ganha um cancelamento grátis; o grappler ganha
armadura de 1 hit no próximo arremesso de comando). O defensor remove Marcas com um arremesso técnico bem-sucedido.

| Aspecto | Descrição |
|---|---|
| Objetivo | Expressar a identidade de cada personagem pela mecânica |
| Ofensivo / defensivo | Majoritariamente ofensivo |
| Contra-jogo | Evitar os golpes marcadores; tech de arremesso |
| Riscos | Cada personagem multiplica o custo de design, VFX e animação; difícil de ler para iniciantes |
| Vantagens | Personalidade forte por lutador |
| Desvantagens | Escala mal com o elenco; pouca defesa universal |
| Balanceamento | Complexo: um parâmetro por personagem |

---

## Comparação e recomendação

| Critério | Maré | Fôlego | Marcas |
|---|---|---|---|
| Originalidade | **Alta** | Baixa | Média |
| Coerência com o objetivo (neutral + ritmo) | **Alta** | Média | Baixa |
| Ofensiva e defensiva equilibradas | **Sim** | Pune defesa | Ofensiva |
| Legibilidade | Alta (um medidor central) | Alta | Baixa |
| Custo de produção artística | Baixo (HUD + VFX global) | Médio | **Alto** (por personagem) |
| Facilidade de balancear | Alta | Alta | Baixa |

**Recomendação: Maré.** É a que melhor recompensa footsies e iniciativa, é legível e cabe no escopo artístico.
Ideia aproveitável de Marcas para o futuro: efeitos de Arrebentação ligeiramente diferentes por personagem.

**Como validar (Fase 2, com o núcleo e o modo debug):**
1. Implementar Maré como regra parametrizada no `resources`.
2. Rodar milhares de partidas simuladas entre bots com scripts de estilo (agressivo, defensivo, misto) e medir:
   taxa de virada, duração média dos rounds, frequência de Arrebentação/Contracorrente, efeito bola de neve.
3. Sessões de jogo humano no modo debug (Kaio e convidados), com perguntas fixas de legibilidade e diversão.
4. Só então congelar regras e números.

## Recursos totais do jogo (proposta)

1. **Vida.**
2. **Super meter** individual (2 barras): especiais reforçados (1 barra) e Super (2 barras). Supers cinematográficos ficam para depois do vertical slice.
3. **Maré** compartilhada.

Sem outro recurso universal, para manter o sistema enxuto.
