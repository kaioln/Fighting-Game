# 07 — Direção artística (proposta para aprovação)

## Identidade

**Tema:** *Porto Bruma*, cidade portuária fictícia do Atlântico Sul, onde estaleiros desativados viraram
arenas de luta clandestinas sob controle de guildas de trabalhadores. Ferrugem, água salgada, neon de bar
de cais, luz de fim de tarde atravessando névoa.

**Estilo visual:** 3D estilizado com acabamento **pintado / cel shading de duas tonalidades**, linhas de contorno
controladas por material, proporções levemente heroicas (cabeça ~1/7,5 do corpo). Referência de técnica
(não de conteúdo): animação limitada com poses-chave fortes e timing em degrau, como Guilty Gear Xrd [S].

**Pilares de legibilidade**
1. Silhueta reconhecível em preto chapado, em qualquer pose de ataque.
2. Valor (claro/escuro) do personagem sempre contrasta com o fundo da arena na faixa de combate.
3. VFX nunca cobrem a área de hurtbox do personagem por mais de 2 frames.
4. Cores de feedback reservadas: cada cor de efeito significa uma coisa só (ex.: Maré = azul-verde; counter = laranja).

## Personagens do vertical slice

Escolhidos por **contraste máximo de gameplay e silhueta**: um rushdown leve e um grappler pesado.

### Nadja Ferraz — rushdown
- **História:** entregadora de moto de 24 anos que cresceu nas docas; luta para pagar a dívida da oficina da mãe com a guilda.
- **Personalidade:** impaciente, debochada, leal.
- **Estilo de luta:** mistura própria de capoeira de rua com joelhadas e cotoveladas curtas; muito movimento de quadril, base baixa.
- **Silhueta:** jaqueta de motoqueira assimétrica (uma manga longa, outra cortada), capacete pendurado no cinto, calça cargo, canelas enfaixadas. Leitura em "Z" (inclinada para frente).
- **Paleta:** vermelho-tijolo, couro escuro, detalhes amarelo-sinalização.
- **Mobilidade:** corrida, hop rápido, especiais de aproximação; vida menor.

### Otto Brand — grappler
- **História:** soldador aposentado de 58 anos, antigo campeão das arenas, agora "guardião" do estaleiro onde as lutas acontecem.
- **Personalidade:** calmo, irônico, paternal, implacável quando provocado.
- **Estilo de luta:** luta de agarre de cais: arremessos de comando, golpes com armadura, alcance longo dos braços.
- **Silhueta:** massiva, avental de couro de soldador, luvas grossas, máscara de solda levantada na testa. Leitura em "A" (base larga, estável).
- **Paleta:** marrom de couro, aço azulado, brasa laranja nas luvas.
- **Mobilidade:** caminhada lenta e pesada, sem corrida, salto curto ruim; vida maior.

Os dois são originais e não derivam visualmente de personagens existentes. Antes de modelar, cada concept
passa por uma revisão de semelhança contra elencos de SF, KOF, Tekken e Guilty Gear.

## Arena do vertical slice — Dique Seco 7

- Plano de luta no fundo de um dique seco de estaleiro, ao entardecer, depois da chuva.
- Primeiro plano: correntes, poças com reflexo (sem bloquear a faixa de combate).
- Plano médio: casco de navio inacabado com andaimes; público de trabalhadores em plataformas (animação de multidão em poucas variações autorais).
- Fundo: guindastes, céu alaranjado, luzes de bar acendendo na margem.
- Elementos animados à mão: faíscas de solda distantes, gaivotas, lona balançando, guindaste girando lentamente.
- Câmera: lateral, zoom leve conforme a distância entre os lutadores, limites que nunca mostram a "borda" do cenário.

## Pipeline de produção por personagem

Concept (turnaround, expressões, poses de combate) → escultura → retopologia → UV → texturas pintadas →
materiais/shader cel → rig (ombros, quadril, coluna, mãos e pés com controles dedicados; ossos de roupa e
acessórios) → skin → animação → integração → validação de hitboxes contra poses → polimento.

**Arquivos-fonte versionados** em `art-source/` via Git LFS: `.blend`, arquivos de pintura em camadas,
texturas em resolução de trabalho.

## Lista de animações por personagem (vertical slice)

Movimento: idle, walk fwd, walk back, (run, só Nadja), dash/backdash, jump startup, ascent, apex, descent, landing,
hop, crouch, crouch transition, guard alto, guard baixo, knockdown, wake-up, virar de lado.
Ataques: 6 normais em pé, 6 agachados, 3 aéreos, 2 command normals, 2 arremessos, 4 especiais (versão normal e reforçada), 1 Super.
Reações: hit alto, hit baixo, hit forte, hit aéreo, juggle, queda, bloqueio leve/pesado, tonto não incluso.
Apresentação: intro, vitória, derrota por tempo.

Estimativa: **~70 animações por personagem**, cada uma com timing amarrado ao frame data.

## Dependências artísticas (o que eu não consigo produzir aqui)

| Entrega | Quem precisa fazer | Bloqueia |
|---|---|---|
| Concept art dos 2 personagens e da arena | Concept artist | Fase 3 |
| Modelos, texturas, materiais finais | Artista 3D de personagens | Fase 3 |
| Rig e skin | Rigger / artista técnico | Fase 3 |
| ~140 animações de combate | Animador(es) de jogos de luta | Fase 3–4 |
| Arena 3D, iluminação, props | Artista de ambiente | Fase 4 |
| VFX (hit sparks, Maré, especiais) | Artista de VFX | Fase 4 |
| Trilha da arena, SFX de golpes, vozes | Compositor, sound designer, atores | Fase 4 |
| Tipografia e identidade de UI | Designer de UI | Fase 4 |

**O que eu posso fazer para apoiar os artistas:** folhas de especificação (proporções, silhueta, timing
de cada golpe a partir do frame data), scripts de Blender para exportação e validação (escala, nomes de
ossos, contagem de frames), ferramentas de preview que mostram a animação com as hitboxes sobrepostas.

**Pergunta aberta para o Kaio:** você tem acesso a artistas (ou é artista), ou prefere que eu prepare um
pacote de briefing para contratação? Isso define o cronograma da Fase 3.
