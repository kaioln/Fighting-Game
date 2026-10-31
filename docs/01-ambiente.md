# 01 — Inspeção do ambiente (31/10/2026)

## O que existe de verdade

| Recurso | Estado | Implicação |
|---|---|---|
| Repositório `kaioln/Fighting-Game` | Vazio (sem commits) | Começamos do zero; nada a preservar ou migrar |
| Container de desenvolvimento (nuvem, Linux, 4 CPUs, 15 GB RAM, sem GPU) | Disponível | Serve para código, testes headless, ferramentas e documentação |
| Compiladores: `gcc`, `g++`, `clang`, `cmake` | Instalados | Núcleo C++ pode ser compilado e testado aqui |
| `python3`, `node 22`, `cargo` (Rust) | Instalados | Ferramentas internas e scripts de validação de dados |
| Godot, Unreal, Blender, .NET SDK | **Não instalados no container** | Não é possível renderizar, abrir editores nem exportar builds aqui |
| GPU | Ausente | Sem renderização, sem captura de vídeo, sem teste visual real no container |
| Computador do Kaio (`DESKTOP-4NTPIUS`, Windows) | Conectado ao projeto, pastas `E:\Apex` e `E:\Vamos ver` visíveis, nenhuma aprovada para este trabalho | Possível rodar build/editor na máquina dele via sessão remota, com aprovação. Há ferramentas de integração com Blender disponíveis, mas não verifiquei se o Blender está instalado |
| Rede | Restrita | Ver abaixo |

## Rede: o que foi bloqueado

Os seguintes domínios **não resolveram** nesta sessão e, portanto, não pude consultar as fontes primárias:

- `game.capcom.com` (manual oficial SF6)
- `www.snk-corp.co.jp` (manual oficial KOF XV)
- `wiki.supercombo.gg`, `glossary.infil.net`, `words.infil.net`, `dreamcancel.com`
- `godotengine.org` (direto)

Consegui acessar páginas do GitHub (READMEs públicos) e resultados de busca na web
(imprensa especializada, wikis espelhadas, fóruns). Por isso, muitos fatos sobre SF6/KOF estão marcados
como **[S]** ou **[C]** em vez de **[V]**.

**Ação recomendada:** liberar esses domínios nas configurações de rede do ambiente para que a
Fase 2 possa confirmar números na fonte primária.

## O que este ambiente NÃO consegue produzir com qualidade

Declarado explicitamente, conforme a regra 2.1/11 do brief:

- Concept art final, modelagem 3D, escultura, retopologia, UV, texturas, rigging, skinning e animação autorais.
- Trilha sonora, efeitos sonoros e vozes originais.
- Cenários 3D finais e VFX finais (texturas e sequências pintadas à mão).

Essas peças estão listadas como **dependências artísticas** em [07-direcao-artistica.md](07-direcao-artistica.md).
Eu não vou gerar substitutos primitivos e apresentá-los como finais. Durante a Fase 2, a única
representação visual será o **modo de depuração** (caixas de colisão e texto), separado do jogo e
nunca apresentado como arte.

## O que este ambiente consegue produzir bem

- Núcleo de simulação determinística (C++), com testes automatizados executados de fato.
- Interpretador de comandos, buffer de inputs, máquina de estados, sistema de dados de golpes, colisões, dano, rounds.
- Serialização de estado, checksums, replays, sync test (rollback a cada frame) headless.
- Ferramentas internas: validadores de dados, editor de golpes (dados), frame analyzer em texto/HTML.
- Scripts de pipeline do Blender (exportação, validação de nomes de ossos/escala) para os artistas usarem.
- Documentação de design, especificações para artistas (folhas de personagem, listas de animação com timing).
