# 08 — Vertical slice mínimo

Princípio: reduzir **quantidade**, nunca **qualidade**. O slice prova que o jogo é divertido,
responsivo e bonito, com o mínimo de conteúdo.

## Conteúdo

| Item | Escopo |
|---|---|
| Personagens | 2 (Nadja, rushdown; Otto, grappler), arte final |
| Arena | 1 (Dique Seco 7), arte final, com elementos animados |
| Modo | Versus local 1v1, melhor de 3 rounds, 99 s |
| Mecânicas | Movimento completo (incl. hop), normais, especiais, reforçados, 1 Super por personagem, arremessos e tech, Maré |
| Controles | Esquema Clássico (6 botões) e Simplificado; teclado, controle comum e arcade stick; remapeamento |
| HUD | Vida, Super, Maré, tempo, rounds, combo counter |
| Treino | Dummy configurável (bloqueio, gravação/reprodução), frame meter, exibição de hitboxes, reset de posição |
| Telas | Título, menu, seleção de personagem, versus, pausa, vitória, opções |
| Áudio | Trilha da arena, SFX de todos os golpes, anúncios de round, vozes básicas |
| Build | Executável Windows, rodando fora do editor |

## Fora do slice (de propósito)

Online, 3v3, supers cinematográficos completos, story mode, tutorial completo, terceiro personagem, segunda arena.

## Critérios de aceitação

1. Simulação a 60 Hz estável; 60 FPS no hardware-alvo.
2. Sync test de rollback passando em todas as partidas gravadas.
3. Replays reproduzidos com checksum idêntico em Linux e Windows.
4. Toda animação de golpe alinhada ao frame data (validação automatizada de frames + revisão visual).
5. Nenhum infinito encontrado pela busca automatizada de combos.
6. Latência de input medida e documentada.
7. Nenhum asset provisório na build de demonstração.
8. Sessões de teste com jogadores externos e relatório de feedback.

## Ordem de execução

| Fase | Entrega | Depende de arte? |
|---|---|---|
| 2 | Núcleo `fgcore` + testes + modo debug (caixas e texto, nunca apresentado como jogo) + 2 personagens **em dados** | Não |
| 2 | Validação da Maré por simulação de bots | Não |
| 3 | Nadja com arte final integrada, spike de renderização no Godot | **Sim** |
| 4 | Otto, arena, HUD, áudio, versus local | **Sim** |
| 5 | Polimento, treino completo, testes externos | Parcial |

A Fase 2 pode começar imediatamente após a aprovação, em paralelo à contratação/produção de arte.
