# Documentação

| # | Documento | Conteúdo |
|---|-----------|----------|
| 01 | [Ambiente](01-ambiente.md) | Ferramentas reais disponíveis, permissões, limitações |
| 02 | [Pesquisa SF6 e KOF XV](02-pesquisa-sf6-kof.md) | Sistemas, mecânicas, controles, apresentação, com evidência |
| 03 | [Frameworks, REA e comunidades](03-frameworks-rea-comunidades.md) | GGPO, Sakuga, Castagne, Night Sky, IKEMEN GO, REA, problemas recorrentes de indies |
| 04 | [Matriz comparativa SF6 × KOF XV](04-matriz-sf6-kof.md) | Comparação sistemática e o que adaptamos |
| 05 | [Engine e arquitetura](05-engine-e-arquitetura.md) | Comparação de engines, decisão recomendada, arquitetura determinística |
| 06 | [Mecânica original](06-mecanica-original.md) | Três propostas comparadas e recomendação |
| 07 | [Direção artística](07-direcao-artistica.md) | Identidade visual, personagens, arena, dependências artísticas |
| 08 | [Vertical slice](08-vertical-slice.md) | Menor entrega que demonstra a qualidade desejada |
| 09 | [Catálogo de repositórios GitHub](09-catalogo-github.md) | 300 repositórios: engines, rollback, determinismo, SF6/KOF (mods, RE, frame data), licenças e recomendações |
| 10 | [Núcleo técnico](10-nucleo-tecnico.md) | Fase 2: o que existe, como rodar, regras implementadas, frame data, testes, limitações |
| — | [GDD inicial](GDD.md) | Game Design Document v0.1 consolidado |
| — | [Fontes](fontes.md) | Lista de fontes consultadas e bloqueadas |

## Convenção de evidência

Todo fato sobre jogos de terceiros é marcado com o nível de evidência:

- **[V]** Verificado nesta sessão em fonte acessada (link em [fontes.md](fontes.md)).
- **[S]** Fonte secundária acessada nesta sessão (imprensa, wiki comunitária, fórum), não confirmada em fonte primária.
- **[C]** Conhecimento comunitário amplamente difundido, **não reverificado nesta sessão** porque a fonte primária estava bloqueada pela rede. Deve ser confirmado antes de virar número de balanceamento.
- **[H]** Hipótese ou inferência nossa. Nunca é tratada como fato sobre a implementação interna de SF6/KOF.

Nenhum número de SF6/KOF é usado diretamente no nosso jogo. Eles servem como ordem de grandeza
para calibrar o nosso próprio frame data, que será validado por testes.
