# 03 — Frameworks, REA e comunidades

Fontes: READMEs públicos acessados nesta sessão via GitHub (ver [fontes.md](fontes.md)).
A API do GitHub para repositórios de terceiros não estava habilitada nesta sessão, então **não
inspecionei o código-fonte, issues ou PRs** desses projetos; inspecionei READMEs e estrutura de
diretórios. A leitura de código é tarefa listada para o início da Fase 2.

## Resumo

| Projeto | Licença | Base | Rollback | 3D / 2.5D | Observações | Uso para nós |
|---|---|---|---|---|---|---|
| **GGPO** (pond3r) | MIT [V] | C++, build Windows/VS2019 [V] | É o próprio rollback [V] | n/a | Build oficial só Windows [V]. Guia exige callbacks save/load/advance e simulação 100% determinística [V] | **Referência de API e candidato real** para o netcode (alvo é Windows). Decidir na Fase 2 entre integrar ou reimplementar a camada |
| **Sakuga Engine** | MIT [V] | Godot 4.7 .NET, C# [V] | Sim [V] | Suporta "3D sprites e modelos 3D" [V] | FAQ: só 1v1 e 2D [V]; IA experimental; cinemáticas não implementadas [V] | Estudo de estrutura de estados e rollback em Godot. Não adotar como base (acoplado a C#/Godot, sem cinemáticas) |
| **Castagne** | MPL 2.0 (contribuições MIT) [V] | Camada sobre Godot [V] | Usa addon de rollback de Snopek [V] | Não informado | 155 estrelas, projetos publicados (Kronian Titans) [V] | Estudo de linguagem de script de golpes e editor. MPL exige publicar alterações nos arquivos do framework |
| **Night Sky Engine** | MIT [V] | Unreal Engine 5.7 [V] | GGPO via plugin GGPOUE4 [V] | 2D e 2.5D [V] | Mira PCs de alto desempenho; branch `lowend` separada [V] | **Melhor referência se escolhermos Unreal**. Prova que rollback + UE5 é viável fora da física Chaos |
| **IKEMEN GO** | MIT (assets CC) [V] | Go [V] | Não mencionado no README | Orientado a recursos MUGEN [V/H] | Maduro, 6.8k commits [V] | Não adequado ao 2.5D 3D autoral. Útil para estudar parsing de comandos estilo MUGEN |
| **Godot Rollback Netcode** (Snopek) | MIT [V] | Godot (addon) | Sim | n/a | 13 releases desde 2021 [V] | Referência de SyncManager em Godot |
| **Klotho** (asset Godot) | Não verificado | Godot 4.4+ .NET | Sim | n/a | Declara simulação 100% em ponto fixo 32.32 e RNG Xorshift128+ [S, afirmação do autor] | Confirma a abordagem ponto fixo; não adotar sem auditoria |

## GGPO — o que extraímos (Developer Guide [V])

1. A simulação precisa ser **totalmente determinística**: mesmo estado + mesmos inputs = mesmo resultado.
2. O estado precisa ser **encapsulado e serializável**; o jogo precisa conseguir **avançar um frame sem renderizar**.
3. `advance_frame` é chamado várias vezes num rollback → **efeitos e sons devem ser adiados** até o rollback terminar.
4. Armadilhas citadas: RNG sem estado salvo, relógio de parede, ponteiros pendentes, estado estático escondido.
5. **Sync test**: executar rollback a cada frame comparando estados; recomendado rodar continuamente durante o desenvolvimento.
6. Input delay: lutas sofrem acima de ~1 frame de delay; o resto é coberto por predição.

**Como isso molda nossa arquitetura:** ver [05](05-engine-e-arquitetura.md#arquitetura-determinística).
O sync test será um **teste automatizado do CI** desde o primeiro commit do núcleo.

## REA — Reverse Engineer Anything

| Item | Constatação [V, README] |
|---|---|
| O que é | Servidor MCP local + CLI que deixa agentes inspecionarem binários e apps, retornando conclusões com evidências e limitações |
| Licença | MIT para código próprio; ferramentas de terceiros mantêm suas licenças; ferramentas comerciais (IDA, Hopper, JEB) exigem licença própria |
| Requisitos | Node.js 22.x/24.x/26.x + npm; análise nativa usa Ghidra/IDA/Hopper/Kuna já instalados; suporte Windows a Ghidra é experimental |
| Análise estática embutida | .NET, PE, ELF, Go, Rust, IL2CPP, **Godot PCK**, PyInstaller etc. |
| Uso declarado | Pesquisa legal, educação, interoperabilidade, preservação, software próprio ou autorizado |

**Avaliação honesta:**
- REA **não** recupera o código de SF6 ou KOF, e não vamos usá-lo contra esses jogos. Inspecionar binários proprietários
  com proteção anticheat/DRM viola as regras do projeto (seção 2.3) e possivelmente termos de uso.
- Usos legítimos para nós: (a) inspecionar **nossos próprios builds** (ex.: confirmar que o PCK/binário
  exportado não carrega assets de debug, verificar símbolos e dependências); (b) estudar binários de
  **projetos open-source** quando o código não basta para entender o comportamento compilado.
- Instalação: **não instalei** nesta fase. Requer Node 22.23.4+ (o container tem Node 22, versão exata a conferir)
  e não é necessário para a Fase 2. Recomendo adiar até existir um build nosso para auditar.

## Comunidades — problemas recorrentes de jogos de luta independentes

A pesquisa direta no Reddit foi limitada (buscas não retornaram threads utilizáveis; vários domínios bloqueados).
O que foi corroborado por mais de uma fonte acessada:

| Problema | Evidência | Medida preventiva desde o projeto |
|---|---|---|
| Hitstop mal calibrado parece lag; acumula em multi-hit | [S] itch.io (vários relatos), critpoints.net | Hitstop explícito por golpe, decaimento em multi-hit, teto por sequência, teste automatizado |
| Input lag adicional por vsync/modo de tela | [S] itch.io, Eventhubs (Samurai Shodown 8 frames) | Medir latência de ponta a ponta no build; modo de baixa latência; sem triple buffering por padrão |
| Personagens que "não reagem rápido o bastante" | [S] itch.io | Golpes leves com startup curto; transições de estado sem frames ocultos; frame meter para auditar |
| Buffer grande demais faz o input parecer atrasado | [S] bugnet.io | Janelas pequenas e por tipo de comando, ajustáveis por dados |
| Netcode ruim mata a comunidade online; Skullgirls citado como referência de bom netcode | [S] Destructoid (Battlenonsense) | Rollback planejado no núcleo |
| Supers/especiais saindo sem querer | [S] Eventhubs (patch SF6) | Prioridade de comandos explícita e testes de regressão de input |

Pendências de pesquisa comunitária (Fase 2, quando domínios forem liberados): r/Fighters, r/gamedev,
servidores Discord públicos de Castagne e Sakuga, postmortems de indies.
