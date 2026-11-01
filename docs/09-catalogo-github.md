# 09 — Catálogo GitHub: jogos de luta (SF6, KOF, mods, engenharia reversa, mecânicas, motores)

> Projeto: jogo de luta 2.5D original, PC nativo, núcleo de simulação C++20 determinístico a 60 Hz, pronto para rollback, camada de apresentação fina.
> Data da pesquisa: **2026-10-31**. Autor: pesquisa automatizada, revisada para o projeto.
> Convenção: **[Fato]** = observado diretamente (metadado da API do GitHub ou trecho de arquivo lido); **[Inferência]** = conclusão nossa a partir dos fatos.

## 1. Metodologia

**Ferramentas.** Busca via `gh api -X GET search/repositories` (ordenado por estrelas, 100 resultados por consulta) e `search/code` (5 consultas). Para os repositórios mais importantes, clone raso (`git clone --depth 1`) só para **leitura** de código-fonte em `/tmp/research/src` (nada foi compilado nem executado; scripts de análise ficaram em diretório separado). Licenças `NOASSERTION` da API foram conferidas lendo o arquivo LICENSE quando relevante (`raw.githubusercontent.com`).

**Volume.** ~185 consultas de repositório + 5 consultas de código → 9.043 repositórios únicos brutos → **300 catalogados** após curadoria (descartados: homônimos — "fire fighting", "SNKRS", "ggplot", "mugen" de outras áreas —, deploy/rollback de banco de dados, trabalhos escolares sem estrelas e sem valor técnico único).

**Consultas usadas (agrupadas).**
- Motores/gerais: `fighting game engine`, `fighting game framework`, `2.5D fighting game`, `fighting game godot|unity|unreal|bevy|raylib|opengl|c++ sdl`, `fighting game language:{C++,C,Rust,C#,GDScript,Lua,JavaScript,TypeScript,Python,Zig,Go,Haxe,Java}`, `platform fighter engine`, `universal fighting engine`, `footsies`, `openbor`, `castagne`, `topic:fighting-game`, `topic:fighting-games`, `topic:fighting-game-engine`, `topic:fgc`, `fgc`.
- Netcode: `rollback netcode`, `ggpo`, `ggrs`, `backroll`, `rollback networking`, `rollback netcode godot`, `rollback unity netcode`, `peer to peer rollback`, `delay based netcode`, `deterministic lockstep`, `netfox`, `godot rollback`, `delta rollback`, `ggpo fork`, `ggpo c++`, `fighting game netcode`, `fighting game rollback c++`, `platform fighter rollback`, `snapnet`, `netplay emulator`, `netcode.io`, `yojimbo`, `GameNetworkingSockets`, `enet`, `topic:rollback-netcode`, `topic:ggpo`, `topic:netcode`, `topic:rollback`, `topic:netplay`.
- Determinismo: `fixed point math library`, `fixed point deterministic`, `fixed-point C++`, `fixed point math`, `fpm fixed point`, `deterministic physics`, `deterministic floating point`, `soft float`, `streflop`, `deterministic simulation`, `deterministic game engine`, `fighting game deterministic`, `fixed timestep game loop`, `box2d`, `topic:fixed-point`.
- Mecânicas/input/dados: `motion input fighting`, `input buffer fighting game`, `command input fighting game`, `special move input`, `input buffer`, `motion input`, `special moves parser`, `combo system`, `combo trainer`, `combo notation`, `move list`, `cancel window`, `hitstop`, `fighting game state machine`, `fighting game character state`, `hierarchical state machine c++`, `hitbox editor`, `hitbox fighting game`, `blender hitbox`, `frame data`, `frame data street fighter`, `sf6 frame data`, `kof frame data`, `fighting game frame data api`, `dustloop`, `wavu`, `topic:frame-data`, `socd`, `gp2040`, `hitbox controller firmware`, `fighting game input display`, `fighting game camera`, `fighting game ui|hud`, `fighting game animation|blender`, `entt`, `bitsery`, `ozz-animation`.
- SF6/RE Engine: `street fighter 6`, `sf6`, `street fighter 6 mod`, `street fighter 6 overlay`, `street fighter 6 api`, `REFramework`, `re engine mod`, `rsz re engine`, `sf6 replay`, `sf6 hitbox`, `sf6 mod manager`, `buckler street fighter`, `cfn sf6`, `street fighter`, `street fighter v`, `street fighter iii`, `topic:street-fighter`.
- KOF/SNK/UE4: `king of fighters`, `king of fighters xv`, `kof xv`, `KOF XV`, `kofxv`, `kof15`, `kof xiii`, `kof mugen`, `kof xv mod`, `snk`, `unreal engine pak tool`, `unreal pak extractor`, `fmodel`, `CUE4Parse`, `UE4SS`.
- RE/training/outros jogos: `hitbox viewer`, `training mode hitbox viewer`, `fighting game training mode`, `fighting game replay`, `fighting game replay parser`, `fighting game hitbox overlay`, `mame hitbox`, `cps3 hitbox`, `fightcade`, `fbneo`, `fighting game lua script`, `guilty gear`, `guilty gear strive mod`, `guilty gear xrd`, `tekken`, `tekken frame data`, `tekken 8 mod`, `blazblue`, `melty blood`, `under night`, `skullgirls`, `dbfz`, `melee slippi`, `smash ultimate mod`, `super smash bros clone`, `doldecomp melee`, `fighting game decompilation`, `rivals of aether workshop`.
- MUGEN/IA/recursos: `mugen`, `ikemen`, `mugen character editor`, `fighter factory`, `mugen sff`, `topic:mugen`, `fighting game ai`, `fighting game reinforcement learning`, `street fighter ai`, `street fighter reinforcement learning`, `fightingice`, `awesome fighting games`, `fighting game resources`, `fighting game tool`, `fighting game data`, `fighting game tournament`, `start.gg`.
- Código (`search/code`): `save_game_state advance_frame language:C++`, `QCF motion input buffer language:C++`, `hurtbox hitbox pushbox language:C++`, `hitstun blockstun language:C++`, `sf6 hitbox reframework language:Lua`.

**Limitações.**
1. A busca do GitHub retorna no máximo 100 itens por consulta, ordenados por estrelas; repositórios pequenos mas valiosos podem ter ficado de fora. Repositórios fora do GitHub (ex.: addon de rollback do Snopek no GitLab) não entram, só seus forks/espelhos.
2. Estrelas e "último push" são instantâneos de 2026-10-31. Há repositórios recentes com contagem de estrelas suspeita (ver categoria R).
3. A coluna "Licença (API)" é o `spdx_id` do GitHub. `none` = sem arquivo de licença detectado (**juridicamente: todos os direitos reservados → só estudo**). `NOASSERTION` = licença não reconhecida; conferimos manualmente as que importam e registramos na coluna Observação.
4. Domínios de wikis/fabricantes (SuperCombo, Capcom, SNK, Dream Cancel, infil.net) estavam bloqueados; não houve validação cruzada de frame data com fontes oficiais.
5. Não lemos nem reutilizamos dados de jogo (arquivos `.fchar`, dumps, sprites). Em `psychodrive`, a pasta `data/` (8,7 GB, dumps do SF6) foi apagada sem ser aberta.
6. Não houve acesso a issues/PRs/wikis dos repositórios (API `repos/...` bloqueada).

## 2. Resumo quantitativo

| Categoria | Qtde |
|---|---:|
| A. Motores/frameworks | 26 |
| B. Jogos/projetos-exemplo | 21 |
| C. Rollback/netplay/lockstep | 38 |
| D. Determinismo (fixed-point etc.) | 22 |
| E. Infra C++ | 12 |
| F. Input/SOCD/hardware | 14 |
| G. Hitbox/frame meter | 16 |
| H. Frame data/estatística | 18 |
| I. SF6 modding | 16 |
| J. KOF | 10 |
| K. Ferramentas RE Engine/Unreal | 13 |
| L. RE/mods de outros FGs | 26 |
| M. Decomp/recomp | 9 |
| N. MUGEN/IKEMEN | 13 |
| O. IA/bots/RL | 11 |
| P. Emuladores/training Lua | 11 |
| Q. Apresentação/UI/aprendizado | 14 |
| R. Suspeitos (evitar) | 10 |
| **Total** | **300** |

Observação [Fato]: para **KOF XV** especificamente, o GitHub tem muito pouco: 1 plugin de hitbox (`473671866/hitbox`), 1 bot de frame data (`dens0ne/kofxv_framebot`) e as ferramentas genéricas de Unreal (FModel/CUE4Parse/UE4SS). Não encontramos dataset público de frame data do KOF XV além do bot. Para **SF6**, o ecossistema gira em torno do REFramework (praydog) e do MMDK (alphazolam).

## 3. Catálogo por categoria

Legenda de relevância: **Alta** = influencia diretamente o design do nosso core; **Média** = referência útil; **Baixa** = contexto; **(estudo)** = só leitura, nunca reutilizar código/dados; **Evitar** = não baixar.

### A. Motores e frameworks de jogos de luta (26)

| Repo | ★ | Licença (API) | Último push | Ling. | O que é | Relevância | Observação |
|---|---:|---|---|---|---|---|---|
| [ikemen-engine/Ikemen-GO](https://github.com/ikemen-engine/Ikemen-GO) | 1655 | NOASSERTION | 2026-10-31 | Go | Motor 2D open source compatível com recursos MUGEN, com rollback (port Go do GGPO) | Alta | MIT (LICENCE.txt verificado; API diz NOASSERTION). Estudado: input.go, rollback.go |
| [DCurrent/openbor](https://github.com/DCurrent/openbor) | 1284 | BSD-3-Clause | 2026-10-18 | C | Engine de beat'em up 2D (C), muito madura | Média | BSD-3-Clause; referência de pipeline 2D/script |
| [WistfulHopes/NightSkyEngine](https://github.com/WistfulHopes/NightSkyEngine) | 335 | MIT | 2026-08-08 | C++ | Framework de jogo de luta em Unreal Engine (C++), com GGPO, estados por script, boxes por frame | Alta | MIT verificado. Estudado: InputBuffer.h, State.h, CollisionBox.h, BattleObject.h |
| [Pyxus/fray](https://github.com/Pyxus/fray) | 313 | MIT | 2024-01-30 | GDScript | Framework de combate para Godot (input sequences, state machine, hitboxes) | Média | MIT. Buffer baseado em tempo real (ms/seg), não em frames |
| [NoisyChain/Sakuga-Engine](https://github.com/NoisyChain/Sakuga-Engine) | 252 | MIT | 2026-08-01 | C# | Motor de luta para Godot 4 em C# com rollback (PleaseResync), física inteira | Alta | MIT. Estudado: InputManager.cs, GlobalVariables.cs, PhysicsBody.cs |
| [panthavma/castagne](https://github.com/panthavma/castagne) | 155 | NOASSERTION | 2026-07-28 | GDScript | Motor/toolkit de jogo de luta para Godot com linguagem própria (.casp) e editor | Alta | MPL-2.0 (verificado em LICENSE.md): copyleft por arquivo. Estudado: CMAttacks.gd, CastagneMemory.gd |
| [kazzmir/paintown](https://github.com/kazzmir/paintown) | 116 | NOASSERTION | 2026-10-31 | C++ | Motor 2D de luta/beat'em up em C++ com suporte parcial a MUGEN | Média | Licença BSD (LICENSE verificado) |
| [digiholic/universalSmashSystem](https://github.com/digiholic/universalSmashSystem) | 98 | GPL-3.0 | 2022-11-17 | Python | Engine de jogo de plataforma-luta (Python/pygame) | Baixa | GPL-3.0 |
| [whitedragon0000/OpenBOR_PLUS](https://github.com/whitedragon0000/OpenBOR_PLUS) | 65 | BSD-3-Clause | 2026-10-08 | C | Fork do OpenBOR | Baixa | BSD-3-Clause |
| [WistfulHopes/NightSkyEngineOLD](https://github.com/WistfulHopes/NightSkyEngineOLD) | 59 | MIT | 2023-06-11 | C++ | Versão antiga do NightSkyEngine (UE5) | Baixa | Histórico; usar a versão atual |
| [Chukobyte/crescent](https://github.com/Chukobyte/crescent) | 54 | MIT | 2025-02-13 | C | Engine de luta/beat'em up em C (SDL/OpenGL) com scripting | Média | MIT |
| [nonameentername/openfight](https://github.com/nonameentername/openfight) | 45 | LGPL-3.0 | 2026-10-05 | C++ | Motor de luta open source em C++/SDL | Média | LGPL-3.0 |
| [atom-tm/l2df-engine](https://github.com/atom-tm/l2df-engine) | 38 | NOASSERTION | 2026-07-24 | Lua | Engine 2D de luta em Lua (LÖVE) | Baixa | NOASSERTION |
| [rcmagic/ZigFightingGame](https://github.com/rcmagic/ZigFightingGame) | 36 | MIT | 2026-01-09 | Zig | Jogo/motor de luta em Zig orientado a dados | Média | MIT. Estudado: input.zig, character_data.zig, GameState.zig |
| [DanielMoura79/HAMOOPI](https://github.com/DanielMoura79/HAMOOPI) | 28 | GPL-2.0 | 2023-09-23 | C++ | Engine de luta gratuita (C++/Allegro), projeto brasileiro | Média | GPL-2.0: só estudo se o core for proprietário |
| [AndreaJens/SchwarzerblitzEngine](https://github.com/AndreaJens/SchwarzerblitzEngine) | 26 | NOASSERTION | 2022-05-12 | C++ | Motor de luta 3D (C++/Irrlicht) do jogo Schwarzerblitz | Média | Licença NOASSERTION (LICENSE não encontrado no raw): verificar |
| [PsychoDrive-Dictator/psychodrive](https://github.com/PsychoDrive-Dictator/psychodrive) | 23 | none | 2026-10-27 | C++ | Simulador C++ que executa scripts extraídos do SF6 para "lab" | Alta | Sem licença + depende de dumps do SF6: só estudo. Estudado: fixed.hpp, guy.hpp, main.hpp |
| [gregorik/Rollback-Core](https://github.com/gregorik/Rollback-Core) | 23 | MIT | 2026-09-17 | C++ | Plugin UE5 de rollback estilo GGPO (C++20) com estado via UPROPERTY(SaveGame) | Média | MIT |
| [MicahMartin/FightingGameEngine](https://github.com/MicahMartin/FightingGameEngine) | 22 | MIT | 2025-05-23 | C++ | Motor C++/SDL2 determinístico com DSL de script e compilador de comandos | Alta | MIT. Estudado: VirtualController.h, CommandCompiler.cpp, Character.h, def.json |
| [blueinsert/Mugen3D](https://github.com/blueinsert/Mugen3D) | 18 | MIT | 2020-02-23 | C# | Engine de luta 3D com modo 2D (Unity) | Baixa | MIT |
| [openkumite/openkumite](https://github.com/openkumite/openkumite) | 12 | MIT | 2026-10-27 | Rust | Motor de luta 3D em Rust que roda dados convertidos de um jogo existente | Média | MIT; usa ggrs; atenção à origem dos dados |
| [Homestrife/HomestrifeCore](https://github.com/Homestrife/HomestrifeCore) | 6 | none | 2014-08-03 | C++ | Código do jogo Homestrife (C++), parser de comandos em fighter.cpp | Baixa | Sem licença; antigo (2014) |
| [WistfulHopes/NightSkyEngineLite](https://github.com/WistfulHopes/NightSkyEngineLite) | 5 | none | 2023-02-18 | C++ | Framework "lite" de jogo de luta (C++/raylib) | Média | Sem licença: só estudo |
| [flmmkch/nugem](https://github.com/flmmkch/nugem) | 4 | none | 2023-10-07 | C++ | Reimplementação de MUGEN em C++/SDL2/OpenGL | Baixa | Sem licença |
| [EthanTheHuman/CrackEngine](https://github.com/EthanTheHuman/CrackEngine) | 4 | none | 2026-01-04 | C++ | Engine de luta C++/OpenGL pessoal | Baixa | Sem licença |
| [hisham-CSS/GameEngineFramework](https://github.com/hisham-CSS/GameEngineFramework) | 1 | NOASSERTION | 2026-09-06 | C++ | Framework com "UntitledFighter" (kernel de simulação C++) | Baixa | Achado por busca de código (Simulate.h); NOASSERTION |

### B. Jogos de luta completos / projetos-exemplo open source (21)

| Repo | ★ | Licença (API) | Último push | Ling. | O que é | Relevância | Observação |
|---|---:|---|---|---|---|---|---|
| [mgechev/mk.js](https://github.com/mgechev/mk.js) | 1555 | none | 2022-02-23 | JavaScript | Jogo de luta em Canvas com multiplayer (JS) | Baixa | Sem licença |
| [omf2097/openomf](https://github.com/omf2097/openomf) | 493 | MIT | 2026-10-31 | C | Remake open source de One Must Fall 2097 (C); marcado com tópico rollback-netcode | Média | MIT; jogo completo em C para estudar |
| [chriscourses/fighting-game](https://github.com/chriscourses/fighting-game) | 374 | none | 2026-10-13 | JavaScript | Tutorial popular de jogo de luta em JS | Baixa | Sem licença; didático |
| [kidagine/Darklings-FightingGame](https://github.com/kidagine/Darklings-FightingGame) | 351 | none | 2024-09-12 | ShaderLab | Jogo 2D F2P em Unity com rollback e fixed-point próprio | Média | Sem licença: só estudo. Estudado: DemonicsFixed.cs |
| [hifight/Footsies](https://github.com/hifight/Footsies) | 274 | GPL-3.0 | 2020-05-13 | C# | Footsies (Unity), jogo minimalista focado em neutro | Média | GPL-3.0 |
| [lc-soft/PunchAndKick](https://github.com/lc-soft/PunchAndKick) | 125 | GPL-2.0 | 2016-07-10 | C | Jogo de luta 2D simples (C) | Baixa | GPL-2.0 |
| [stanislawfortonski/StickMan-3D](https://github.com/stanislawfortonski/StickMan-3D) | 92 | BSD-3-Clause | 2021-01-31 | C++ | Jogo de luta indie C++/OpenGL | Baixa | BSD-3-Clause |
| [rcmagic/DemoFighterWithNetcode](https://github.com/rcmagic/DemoFighterWithNetcode) | 90 | MIT | 2021-05-21 | Lua | Exemplo de jogo de luta com rollback (Lua/LÖVE) | Alta | MIT. Estudado: Constants.lua, Network.lua, StateTimelines.lua |
| [iamschulz/attacke](https://github.com/iamschulz/attacke) | 73 | GPL-3.0 | 2024-02-25 | TypeScript | Jogo de luta simples 2 jogadores (TypeScript) | Baixa | GPL-3.0 |
| [blast-harbour/Godot-Rollback-Fighter-Demo](https://github.com/blast-harbour/Godot-Rollback-Fighter-Demo) | 57 | MIT | 2024-07-10 | GDScript | Demo de luta com "Delta Rollback" para Godot | Média | MIT |
| [LManaslu/wenova](https://github.com/LManaslu/wenova) | 29 | MIT | 2021-10-09 | C++ | Jogo de luta 2D em C++/SDL2 | Baixa | MIT |
| [jagoly/SuperTuxSmash](https://github.com/jagoly/SuperTuxSmash) | 25 | none | 2024-03-08 | C++ | Clone FOSS de Smash em C++ | Baixa | Sem licença |
| [StreakThunderstorm/SonicSmackdown](https://github.com/StreakThunderstorm/SonicSmackdown) | 21 | NOASSERTION | 2024-08-12 | C++ | Fonte do fangame Sonic Smackdown (C++) | Baixa | NOASSERTION; fangame com IP de terceiros |
| [trian-gles/Girls-with-swords-GGRS](https://github.com/trian-gles/Girls-with-swords-GGRS) | 19 | none | 2026-10-04 | C# | Jogo 2D de luta com rollback usando GGRS | Média | Sem licença |
| [plaimi/limbs-off](https://github.com/plaimi/limbs-off) | 19 | GPL-3.0 | 2020-10-01 | C++ | Jogo de luta baseado em física (C++) | Baixa | GPL-3.0 |
| [AndreaJens/rakugaki-rumble](https://github.com/AndreaJens/rakugaki-rumble) | 17 | NOASSERTION | 2024-07-13 | GDScript | Projeto pequeno de luta em Godot 4.2 | Baixa | NOASSERTION |
| [joh-dev/HYPERFIGHT](https://github.com/joh-dev/HYPERFIGHT) | 16 | NOASSERTION | 2020-12-27 | GDScript | Jogo de luta 2D de um golpe (Godot) | Baixa | Licença própria (HYPERFIGHT License) |
| [Binoui/SLOPArena](https://github.com/Binoui/SLOPArena) | 14 | MIT | 2026-10-31 | C# | Platform fighter 3D com rollback | Baixa | MIT; recente |
| [piecedigital/foosies.rpp](https://github.com/piecedigital/foosies.rpp) | 6 | none | 2020-09-15 | C++ | Jogo de luta simples com raylib (C++) | Baixa | Sem licença |
| [gschup/hitstun](https://github.com/gschup/hitstun) | 4 | MIT | 2021-05-26 | C# | Jogo 2D de luta em Unity totalmente determinístico (autor do GGRS) | Média | MIT |
| [m4jr0/space-it](https://github.com/m4jr0/space-it) | 4 | MIT | 2024-05-24 | C | Jogo de luta 2D minimalista em C inspirado em Footsies | Baixa | MIT |

### C. Rollback netcode, netplay e lockstep (38)

| Repo | ★ | Licença (API) | Último push | Ling. | O que é | Relevância | Observação |
|---|---:|---|---|---|---|---|---|
| [pond3r/ggpo](https://github.com/pond3r/ggpo) | 3617 | MIT | 2024-06-26 | C++ | SDK original de rollback (C++), API por callbacks save/load/advance | Alta | MIT. Estudado: ggponet.h, sync.cpp, input_queue.cpp, synctest.cpp, timesync.cpp, vectorwar.cpp |
| [foxssake/netfox](https://github.com/foxssake/netfox) | 1124 | MIT | 2026-09-26 | GDScript | Addons de multiplayer para Godot incluindo rollback | Média | MIT |
| [JiepengTan/LockstepEngine](https://github.com/JiepengTan/LockstepEngine) | 967 | none | 2020-03-23 | C# | Solução lockstep com libs determinísticas (math, colisão) | Média | Sem licença |
| [proepkes/UnityLockstep](https://github.com/proepkes/UnityLockstep) | 711 | MIT | 2019-03-21 | C# | Lockstep determinístico com predição e rollback | Baixa | MIT |
| [gschup/ggrs](https://github.com/gschup/ggrs) | 703 | NOASSERTION | 2026-10-19 | Rust | Reimaginação do GGPO em Rust com API por "requests" | Alta | MIT/Apache-2.0 (LICENSE verificado). Estudado: lib.rs, sync_layer.rs, builder.rs |
| [rameshvarun/netplayjs](https://github.com/rameshvarun/netplayjs) | 576 | ISC | 2024-10-10 | TypeScript | P2P multiplayer para browser com rollback | Média | ISC |
| [chromealex/ecs](https://github.com/chromealex/ecs) | 563 | MIT | 2023-06-01 | - | ECS Unity com rollback automático do estado | Baixa | MIT |
| [HouraiTeahouse/backroll-rs](https://github.com/HouraiTeahouse/backroll-rs) | 369 | ISC | 2023-03-13 | Rust | Implementação GGPO em Rust safe | Média | ISC; sem push desde 2023. Estudado: command.rs, input.rs |
| [gschup/bevy_ggrs](https://github.com/gschup/bevy_ggrs) | 367 | NOASSERTION | 2026-10-31 | Rust | Plugin Bevy para GGRS | Média | MIT/Apache-2.0 (LICENSE verificado) |
| [ErnWong/crystalorb](https://github.com/ErnWong/crystalorb) | 234 | NOASSERTION | 2024-09-03 | Rust | Predição client-side e reconciliação (Rust) | Baixa | NOASSERTION |
| [nilpunch/massive-ecs](https://github.com/nilpunch/massive-ecs) | 224 | MIT | 2026-04-01 | C# | ECS bitset com rollback (C#) | Baixa | MIT |
| [Confetti3/SF4-Ember-Netplay](https://github.com/Confetti3/SF4-Ember-Netplay) | 182 | MIT | 2026-10-31 | C++ | Salas rollback e overlay para USF4 | Baixa | MIT |
| [Delta3-Studio/Backdash](https://github.com/Delta3-Studio/Backdash) | 177 | MIT | 2026-10-26 | C# | SDK de rollback em C# (.NET) | Média | MIT; referência de API moderna |
| [nykwil/UnityGGPO](https://github.com/nykwil/UnityGGPO) | 171 | MIT | 2024-01-15 | C# | DLL/wrapper GGPO para Unity | Baixa | MIT |
| [johanhelsing/extreme_bevy](https://github.com/johanhelsing/extreme_bevy) | 154 | CC0-1.0 | 2025-10-25 | Rust | Tutorial de jogo p2p com GGRS + matchbox | Baixa | CC0-1.0 |
| [metal-crow/Dark-Souls-1-Overhaul](https://github.com/metal-crow/Dark-Souls-1-Overhaul) | 144 | NOASSERTION | 2026-10-16 | C++ | Overhaul de DS1 com tentativa de rollback (Rollback.h) | Baixa | NOASSERTION; estudo de rollback em jogo não projetado para isso |
| [genxium/DelayNoMoreUnity](https://github.com/genxium/DelayNoMoreUnity) | 136 | none | 2026-04-16 | C# | Demo de plataforma com rollback (Unity) | Baixa | Sem licença |
| [poliva/ggposrv](https://github.com/poliva/ggposrv) | 128 | GPL-2.0 | 2019-01-20 | Python | Reimplementação não oficial de servidor GGPO | Baixa | GPL-2.0 |
| [tangobattle/tango](https://github.com/tangobattle/tango) | 105 | GPL-3.0 | 2026-10-29 | Rust | Rollback para Mega Man Battle Network (emulação) | Baixa | GPL-3.0 |
| [xpTURN/Klotho](https://github.com/xpTURN/Klotho) | 74 | Apache-2.0 | 2026-10-31 | C# | Framework determinístico (rollback/lockstep) Unity/Godot | Média | Apache-2.0; recente |
| [BestoGames/BestoNet](https://github.com/BestoGames/BestoNet) | 70 | MIT | 2026-06-02 | C# | Rollback custom usado no jogo Idol Showdown | Média | MIT |
| [HeatXD/GekkoNet](https://github.com/HeatXD/GekkoNet) | 64 | BSD-2-Clause | 2026-10-24 | C++ | SDK P2P de rollback em C++20 com API C, replay, spectator e stress session | Alta | BSD-2-Clause. Estudado: gekkonet.h, event.cpp, exemplos |
| [fabeloper/sf4e](https://github.com/fabeloper/sf4e) | 63 | MIT | 2026-10-04 | C++ | Mod de rollback para Ultra Street Fighter IV | Média | MIT; mod de jogo comercial |
| [GrimFlash/BBCF-Improvement-Mod-GGPO](https://github.com/GrimFlash/BBCF-Improvement-Mod-GGPO) | 60 | MIT | 2022-01-20 | C++ | Implementação GGPO em mod de BlazBlue CF | Média | MIT; exemplo de retrofitting de rollback |
| [cscorley/bevy_ggrs_rapier_example](https://github.com/cscorley/bevy_ggrs_rapier_example) | 50 | MIT | 2024-09-07 | Rust | Exemplo GGRS + física Rapier | Baixa | MIT |
| [nilpunch/unity-prediction-rollback](https://github.com/nilpunch/unity-prediction-rollback) | 48 | MIT | 2024-04-29 | C# | Rollback determinístico para Unity | Baixa | MIT |
| [FlutterTal/godot_ggpo](https://github.com/FlutterTal/godot_ggpo) | 47 | MIT | 2021-03-13 | C++ | Módulo C++ GGPO para Godot | Baixa | MIT |
| [RJ/bevy_timewarp](https://github.com/RJ/bevy_timewarp) | 43 | MIT | 2024-03-07 | Rust | Lib de rollback que bufferiza estado de componentes | Baixa | MIT |
| [assemblaj/ggpo](https://github.com/assemblaj/ggpo) | 32 | none | 2024-11-21 | Go | Port do GGPO para Go | Baixa | Sem licença declarada no repo |
| [Fcornaire/TF.EX](https://github.com/Fcornaire/TF.EX) | 29 | GPL-3.0 | 2026-10-30 | C# | Mod de rollback para TowerFall | Baixa | GPL-3.0 |
| [thomasboyt/telegraph](https://github.com/thomasboyt/telegraph) | 28 | MIT | 2022-11-22 | TypeScript | Netcode p2p para browser adaptado do GGPO | Baixa | MIT |
| [Fractural/GodotRollbackNetcodeMono](https://github.com/Fractural/GodotRollbackNetcodeMono) | 25 | MIT | 2025-03-18 | GDScript | Port C# do addon de rollback do Snopek | Baixa | MIT |
| [HeatXD/PleaseResync](https://github.com/HeatXD/PleaseResync) | 21 | MIT | 2025-06-05 | C# | Lib de rollback em C# (base do Sakuga) | Baixa | MIT |
| [johanhelsing/bevy_roll_safe](https://github.com/johanhelsing/bevy_roll_safe) | 20 | Apache-2.0 | 2026-10-31 | Rust | Utilitários rollback-safe para Bevy | Baixa | Apache-2.0 |
| [lucaspoffo/tkr](https://github.com/lucaspoffo/tkr) | 13 | MIT | 2026-10-31 | Odin | Lib P2P de rollback em Odin | Baixa | MIT |
| [maximkulkin/godot-rollback-netcode](https://github.com/maximkulkin/godot-rollback-netcode) | 10 | MIT | 2023-05-26 | GDScript | Fork do addon de rollback do Snopek para Godot | Média | MIT |
| [ENVYFGC/bbcse-rollback](https://github.com/ENVYFGC/bbcse-rollback) | 9 | GPL-3.0 | 2026-10-24 | C++ | Rollback para BlazBlue CSE | Baixa | GPL-3.0 |
| [lessthen3/GGPO4ALL](https://github.com/lessthen3/GGPO4ALL) | 4 | MIT | 2026-03-20 | C++ | GGPO multiplataforma para motores | Baixa | MIT; achado por busca de código |

### D. Determinismo: fixed-point, soft-float, física e timestep (22)

| Repo | ★ | Licença (API) | Último push | Ling. | O que é | Relevância | Observação |
|---|---:|---|---|---|---|---|---|
| [erincatto/box2d](https://github.com/erincatto/box2d) | 10462 | MIT | 2026-10-30 | C | Física 2D (C); v3 com foco em determinismo | Média | MIT; para jogo de luta, colisão AABB própria tende a bastar |
| [PetteriAimonen/libfixmath](https://github.com/PetteriAimonen/libfixmath) | 889 | NOASSERTION | 2026-10-30 | C | Fixed-point Q16.16 em C com trig/sqrt/exp | Alta | MIT (LICENSE verificado). Estudado: fix16.h |
| [MikeLankamp/fpm](https://github.com/MikeLankamp/fpm) | 845 | MIT | 2026-03-29 | C++ | Fixed-point header-only C++ (fixed<Base,Intermediate,Frac>) | Alta | MIT. Estudado: fixed.hpp, math.hpp |
| [johnmcfarlane/cnl](https://github.com/johnmcfarlane/cnl) | 695 | BSL-1.0 | 2024-04-26 | C++ | Compositional Numeric Library (C++), fixed-point composável | Média | BSL-1.0 |
| [asik/FixedMath.Net](https://github.com/asik/FixedMath.Net) | 649 | NOASSERTION | 2018-04-10 | C# | Fix64 em C# | Baixa | Apache-2.0 (LICENSE verificado) |
| [Kimbatt/unity-deterministic-physics](https://github.com/Kimbatt/unity-deterministic-physics) | 556 | none | 2024-02-14 | C# | Física determinística cross-platform no Unity (DOTS + soft-float) | Baixa | Sem licença |
| [PetteriAimonen/libfixmatrix](https://github.com/PetteriAimonen/libfixmatrix) | 457 | MIT | 2014-01-17 | C | Matrizes/quaternions fixed-point em C | Baixa | MIT |
| [ivanyu/awesome-deterministic-simulation-testing](https://github.com/ivanyu/awesome-deterministic-simulation-testing) | 397 | CC-BY-4.0 | 2026-08-02 | - | Lista sobre testes de simulação determinística | Baixa | CC-BY-4.0 |
| [johnmcfarlane/fixed_point](https://github.com/johnmcfarlane/fixed_point) | 236 | BSL-1.0 | 2019-04-06 | C++ | Proposta antiga de fixed-point C++ | Baixa | BSL-1.0; substituída por cnl |
| [Kimbatt/soft-float-starter-pack](https://github.com/Kimbatt/soft-float-starter-pack) | 224 | MIT | 2022-10-24 | C# | Soft-float em C# | Baixa | MIT |
| [danielmansson/Unity.Mathematics.FixedPoint](https://github.com/danielmansson/Unity.Mathematics.FixedPoint) | 206 | MIT | 2021-04-22 | C# | Extensão fixed-point para Unity.Mathematics | Baixa | MIT |
| [gbmhunter/MFixedPoint](https://github.com/gbmhunter/MFixedPoint) | 176 | MIT | 2019-05-22 | C++ | Fixed-point header-only C++ | Baixa | MIT |
| [mizvekov/fp](https://github.com/mizvekov/fp) | 116 | ISC | 2018-06-18 | C++ | Fixed-point C++14 | Baixa | ISC |
| [arturbac/fixed_math](https://github.com/arturbac/fixed_math) | 100 | BSL-1.0 | 2025-06-14 | C++ | Fixed-point 48.16 C++17/23 | Média | BSL-1.0 |
| [mrdav30/FixedMathSharp](https://github.com/mrdav30/FixedMathSharp) | 95 | MIT | 2026-10-31 | C# | Fixed-point determinístico .NET | Baixa | MIT |
| [JiepengTan/Lockstep.Math](https://github.com/JiepengTan/Lockstep.Math) | 73 | none | 2023-01-02 | C# | Math determinística para lockstep (C#) | Baixa | Sem licença |
| [howerj/q](https://github.com/howerj/q) | 61 | Unlicense | 2025-04-14 | C | Q16.16 em C | Baixa | Unlicense |
| [devlinzhou/deterministic_float](https://github.com/devlinzhou/deterministic_float) | 50 | Apache-2.0 | 2023-10-11 | C++ | Soft-float determinístico rápido (C++) | Média | Apache-2.0 |
| [deftio/fr_math](https://github.com/deftio/fr_math) | 47 | BSD-2-Clause | 2026-07-21 | C++ | Fixed-point para C embarcado (trig, log, 2D) | Baixa | BSD-2-Clause |
| [IronWarrior/UnityCrossPlatformDeterministicFloats](https://github.com/IronWarrior/UnityCrossPlatformDeterministicFloats) | 46 | none | 2023-02-28 | C# | Testes de determinismo de float entre plataformas | Baixa | Sem licença; útil como metodologia |
| [jakubtomsu/fixed-timestep-demo](https://github.com/jakubtomsu/fixed-timestep-demo) | 44 | MIT | 2024-10-18 | Odin | Demo de diferentes loops de timestep fixo (Odin/raylib) | Média | MIT |
| [devlinzhou/deterministic_physics](https://github.com/devlinzhou/deterministic_physics) | 42 | Apache-2.0 | 2023-10-02 | C++ | Física 3D determinística para lockstep (C++) | Baixa | Apache-2.0 |

### E. Infraestrutura C++ útil (serialização, ECS, rede, animação, HSM) (12)

| Repo | ★ | Licença (API) | Último push | Ling. | O que é | Relevância | Observação |
|---|---:|---|---|---|---|---|---|
| [skypjack/entt](https://github.com/skypjack/entt) | 13235 | MIT | 2026-10-30 | C++ | ECS C++ com snapshots | Média | MIT |
| [ValveSoftware/GameNetworkingSockets](https://github.com/ValveSoftware/GameNetworkingSockets) | 10020 | BSD-3-Clause | 2026-10-03 | C++ | Transporte UDP confiável/não confiável (C++) | Média | BSD-3-Clause |
| [lsalzman/enet](https://github.com/lsalzman/enet) | 3299 | MIT | 2026-06-23 | C | UDP confiável em C | Média | MIT |
| [guillaumeblanc/ozz-animation](https://github.com/guillaumeblanc/ozz-animation) | 3093 | NOASSERTION | 2026-08-01 | C++ | Runtime de animação esquelética C++ e toolset | Média | MIT (LICENSE.md verificado; API diz NOASSERTION). Bom para camada de apresentação 2.5D |
| [mas-bandwidth/yojimbo](https://github.com/mas-bandwidth/yojimbo) | 2750 | BSD-3-Clause | 2026-10-29 | C++ | Biblioteca de rede cliente/servidor C++ (Glenn Fiedler) | Média | BSD-3-Clause |
| [fraillt/bitsery](https://github.com/fraillt/bitsery) | 1269 | MIT | 2025-10-09 | C++ | Serialização binária C++ header-only, sem macros, endianness configurável | Alta | MIT |
| [xissburg/edyn](https://github.com/xissburg/edyn) | 794 | MIT | 2026-09-08 | C++ | Física como ECS (C++) | Baixa | MIT |
| [r-lyeh-archived/fsm](https://github.com/r-lyeh-archived/fsm) | 288 | Zlib | 2015-11-29 | C++ | HFSM C++ leve | Baixa | Zlib |
| [Green-Sky/imgui_entt_entity_editor](https://github.com/Green-Sky/imgui_entt_entity_editor) | 281 | MIT | 2024-04-30 | C++ | Editor de entidades EnTT com Dear ImGui | Baixa | MIT; ideia para tooling de debug |
| [alanjfs/sequentity](https://github.com/alanjfs/sequentity) | 268 | none | 2024-09-17 | C++ | Widget de sequencer ImGui (timeline) para C++17 | Média | Sem licença: só estudo; ideia para editor de frame data |
| [igor-krechetov/hsmcpp](https://github.com/igor-krechetov/hsmcpp) | 130 | MIT | 2026-10-18 | C++ | HSM C++ | Baixa | MIT |
| [tinverse/tsm](https://github.com/tinverse/tsm) | 46 | MIT | 2026-07-09 | C++ | HSM tipada em C++ | Baixa | MIT |

### F. Input, comandos, SOCD, input display e hardware (14)

| Repo | ★ | Licença (API) | Último push | Ling. | O que é | Relevância | Observação |
|---|---:|---|---|---|---|---|---|
| [OpenStickCommunity/GP2040-CE](https://github.com/OpenStickCommunity/GP2040-CE) | 2580 | MIT | 2026-10-28 | C++ | Firmware de gamepad multiplataforma para RP2040 (popular em fightsticks/leverless) | Média | MIT; [Inferência, código não lido] referência de SOCD/latência |
| [FeralAI/GP2040](https://github.com/FeralAI/GP2040) | 622 | MIT | 2023-04-11 | C | GP2040 original | Baixa | MIT |
| [cafali/SnapKey](https://github.com/cafali/SnapKey) | 515 | MIT | 2025-08-07 | C++ | Resolução SOCD por software (teclado) | Baixa | MIT |
| [homemech/unity-pattern-combo](https://github.com/homemech/unity-pattern-combo) | 67 | MIT | 2024-01-21 | C# | Sistema de combo via command pattern (Unity) | Baixa | MIT |
| [drkitt/godot-input-buffer](https://github.com/drkitt/godot-input-buffer) | 61 | MIT | 2024-05-11 | C# | Input buffer genérico para Godot | Baixa | MIT; baseado em tempo, não frames |
| [Delta3-Studio/InputDisplay](https://github.com/Delta3-Studio/InputDisplay) | 45 | GPL-3.0 | 2025-12-17 | C# | Overlay de inputs para training mode | Média | GPL-3.0 |
| [apokrif6/ComboGraph](https://github.com/apokrif6/ComboGraph) | 44 | MIT | 2024-12-03 | C++ | Plugin UE de grafo de combos | Baixa | MIT |
| [calexil/FightstickDisplay](https://github.com/calexil/FightstickDisplay) | 39 | GPL-3.0 | 2026-05-06 | Python | Display de inputs de fightstick (Python) | Baixa | GPL-3.0 |
| [ioridev/Precision-Fighting-Board](https://github.com/ioridev/Precision-Fighting-Board) | 27 | MIT | 2025-09-26 | C | Placa de fightstick a 8000 Hz | Baixa | MIT |
| [ygg-m/fg-input-translator](https://github.com/ygg-m/fg-input-translator) | 24 | none | 2026-10-05 | TypeScript | Converte notação numérica em inputs visuais | Baixa | Sem licença |
| [andr30z/fgc-combo-companion](https://github.com/andr30z/fgc-combo-companion) | 24 | none | 2026-01-21 | TypeScript | Tradução de notação de combo (SF6, T8) | Baixa | Sem licença |
| [ccelik97/SOCD-Cleaner](https://github.com/ccelik97/SOCD-Cleaner) | 21 | MIT | 2023-10-21 | AutoHotkey | SOCD cleaner em AutoHotkey | Baixa | MIT |
| [m-acrophage/godot-fg-input-manager](https://github.com/m-acrophage/godot-fg-input-manager) | 9 | MIT | 2021-08-11 | GDScript | Protótipo de input manager determinístico para Godot | Baixa | MIT |
| [Aquat1c/EfzExtendedPractice](https://github.com/Aquat1c/EfzExtendedPractice) | 0 | none | 2026-10-30 | C++ | Mod de treino de Eternal Fighter Zero com input_buffer.cpp | Baixa | Sem licença; achado por busca de código |

### G. Editores/visualizadores de hitbox e frame meter (16)

| Repo | ★ | Licença (API) | Último push | Ling. | O que é | Relevância | Observação |
|---|---:|---|---|---|---|---|---|
| [agersant/gbvsr-frame-meter](https://github.com/agersant/gbvsr-frame-meter) | 130 | MIT | 2026-10-14 | C++ | Frame meter de training mode para GBVSR (C++) | Média | MIT; boa referência de UI de frame meter |
| [AltimorTASDK/strivehitboxes](https://github.com/AltimorTASDK/strivehitboxes) | 81 | MIT | 2021-09-21 | C++ | Hitbox viewer in-game para GG Strive (C++) | Média | MIT; RE de jogo comercial |
| [coelhucas/hitbox-editor](https://github.com/coelhucas/hitbox-editor) | 56 | none | 2020-11-19 | GDScript | Editor de mapeamento de caixas para jogos de luta (Godot) | Média | Sem licença: só estudo |
| [YouKnow232/ggxxacpr_overlay](https://github.com/YouKnow232/ggxxacpr_overlay) | 24 | GPL-3.0 | 2026-06-10 | C# | Overlay de hitbox/frame para GGXX AC+R | Baixa | GPL-3.0 |
| [kkots/ggxrd_hitbox_overlay_2211](https://github.com/kkots/ggxrd_hitbox_overlay_2211) | 23 | NOASSERTION | 2026-10-07 | C++ | Overlay de hitbox para GG Xrd Rev2 | Média | NOASSERTION |
| [bombzj/arcade-sprite-viewer](https://github.com/bombzj/arcade-sprite-viewer) | 23 | MIT | 2023-01-19 | JavaScript | Viewer de sprites arcade com dados de hitbox (CPS1/NeoGeo) | Baixa | MIT |
| [Eiton/MBTL-Hitbox-Viewer](https://github.com/Eiton/MBTL-Hitbox-Viewer) | 22 | none | 2024-05-11 | C++ | Hitbox viewer Melty Blood TL | Baixa | Sem licença |
| [AltimorTASDK/unisthitboxes](https://github.com/AltimorTASDK/unisthitboxes) | 18 | MIT | 2019-04-10 | C++ | Hitbox viewer UNIST | Baixa | MIT |
| [DhafinFawwaz/2D-HitBox-System-with-Editor](https://github.com/DhafinFawwaz/2D-HitBox-System-with-Editor) | 11 | MIT | 2024-02-01 | C# | Sistema de hitbox 2D com editor (Unity) | Baixa | MIT |
| [AltimorTASDK/dbfzhitboxes](https://github.com/AltimorTASDK/dbfzhitboxes) | 10 | MIT | 2019-03-20 | C++ | Hitbox viewer DBFZ | Baixa | MIT |
| [dante-chiesa/Hantei-chan](https://github.com/dante-chiesa/Hantei-chan) | 8 | none | 2021-10-04 | C++ | Editor GUI de arquivos .HA6 (Melty Blood) | Média | Sem licença; mostra formato de frames/boxes de um jogo comercial |
| [WistfulHopes/gbvshitboxes](https://github.com/WistfulHopes/gbvshitboxes) | 8 | none | 2022-09-02 | C++ | Hitbox viewer GBVS | Baixa | Sem licença |
| [Labreezy/bb-collision-editor](https://github.com/Labreezy/bb-collision-editor) | 6 | none | 2019-12-29 | C# | Editor de hitbox/hurtbox de BlazBlue | Baixa | Sem licença |
| [kaiiboraka/Aseprite_Hitbox_Editor](https://github.com/kaiiboraka/Aseprite_Hitbox_Editor) | 5 | CC0-1.0 | 2024-07-14 | Lua | Script Aseprite para gerar dados de hitbox de pixel art | Média | CC0-1.0; reutilizável |
| [WistfulHopes/GGSTCollisionEditor](https://github.com/WistfulHopes/GGSTCollisionEditor) | 5 | none | 2022-04-29 | C# | Editor de hitbox para GG Strive | Baixa | Sem licença |
| [Redesigner/UnrealMeleeAnimationHelpers](https://github.com/Redesigner/UnrealMeleeAnimationHelpers) | 4 | MIT | 2026-01-16 | C++ | Hitbox melee com visualização no editor (UE) | Baixa | MIT |

### H. Datasets, apps e bots de frame data / estatísticas (18)

| Repo | ★ | Licença (API) | Último push | Ling. | O que é | Relevância | Observação |
|---|---:|---|---|---|---|---|---|
| [halvnykterist/rating-update](https://github.com/halvnykterist/rating-update) | 113 | MIT | 2024-05-14 | CSS | Ratings de GG Strive | Baixa | MIT |
| [williamsjokvist/cfn-tracker](https://github.com/williamsjokvist/cfn-tracker) | 91 | NOASSERTION | 2026-10-30 | Go | Tracking de partidas ranqueadas SF6/T8 | Baixa | NOASSERTION |
| [D4RKONION/FAT](https://github.com/D4RKONION/FAT) | 86 | GPL-3.0 | 2026-10-22 | TypeScript | App de frame data (SF, GGST) com JSON por personagem | Alta | GPL-3.0 (código e dados): só referência de esquema. Estudado: SF6FrameData.json (só nomes de campos) |
| [alanoliveira/sfbuff](https://github.com/alanoliveira/sfbuff) | 56 | MIT | 2026-08-03 | Ruby | Site de estatísticas de partidas SF6 | Baixa | MIT |
| [pbruvoll/tekkendocs](https://github.com/pbruvoll/tekkendocs) | 39 | none | 2026-10-31 | HTML | Frame data e recursos de Tekken | Baixa | Sem licença |
| [Myke623/match-indexer](https://github.com/Myke623/match-indexer) | 17 | MIT | 2024-09-07 | Python | Indexador de partidas em vídeo | Baixa | MIT |
| [Sophon/FightingNerd](https://github.com/Sophon/FightingNerd) | 15 | NOASSERTION | 2026-10-28 | Kotlin | Frame data FGC para Discord/Android/iOS | Baixa | NOASSERTION |
| [4rays/sf6-move-data](https://github.com/4rays/sf6-move-data) | 14 | MIT | 2025-05-17 | Python | Dados de golpes do SF6 em TOML (EN/JP) | Média | MIT no repo, mas dados descrevem jogo da Capcom: não importar |
| [movildima/GGStriveUtilsBot](https://github.com/movildima/GGStriveUtilsBot) | 14 | Apache-2.0 | 2024-07-19 | C# | Bot de frame data GG Strive | Baixa | Apache-2.0 |
| [sfstatsapp/buckler-api-docs](https://github.com/sfstatsapp/buckler-api-docs) | 13 | MIT | 2026-08-22 | JavaScript | Documentação comunitária das APIs web Buckler (SF6) | Baixa | MIT; API web não oficial |
| [TLNBS2405/heihachi](https://github.com/TLNBS2405/heihachi) | 12 | MIT | 2025-08-04 | Python | Bot de frame data Tekken 8 (wavu.wiki) | Baixa | MIT |
| [avivace/sfx-framedata](https://github.com/avivace/sfx-framedata) | 8 | MIT | 2023-01-02 | Python | Frame data de SFV (bot/site) | Baixa | MIT |
| [jeff502/Street-Fighter-6-Frame-Data-Discord-Bot](https://github.com/jeff502/Street-Fighter-6-Frame-Data-Discord-Bot) | 7 | MIT | 2023-06-20 | Python | Bot Discord de frame data SF6 | Baixa | MIT |
| [FightCore/frame-data](https://github.com/FightCore/frame-data) | 5 | GPL-3.0 | 2026-10-07 | PLpgSQL | Banco de frame data (Melee) em SQL | Baixa | GPL-3.0 |
| [0xSilverest/sf6-frame-data-extractor](https://github.com/0xSilverest/sf6-frame-data-extractor) | 4 | NOASSERTION | 2025-02-05 | Clojure | Extrator de frame data do site oficial SF6 (Clojure) | Baixa | NOASSERTION; scraping de site oficial |
| [dens0ne/kofxv_framebot](https://github.com/dens0ne/kofxv_framebot) | 4 | MIT | 2024-11-24 | JavaScript | Bot Discord de frame data KOF XV | Média | MIT; único dataset KOF XV encontrado |
| [jrfrancisco1123/sf6_framedata](https://github.com/jrfrancisco1123/sf6_framedata) | 3 | none | 2025-08-11 | HTML | Página de frame data SF6 | Baixa | Sem licença |
| [RyoSogawa/sf6-sensei](https://github.com/RyoSogawa/sf6-sensei) | 3 | NOASSERTION | 2026-10-22 | TypeScript | API de frame data SF6 para GPT/MCP | Baixa | NOASSERTION |

### I. Street Fighter 6: modding e ferramentas (RE Engine) (16)

| Repo | ★ | Licença (API) | Último push | Ling. | O que é | Relevância | Observação |
|---|---:|---|---|---|---|---|---|
| [praydog/REFramework](https://github.com/praydog/REFramework) | 5704 | MIT | 2026-10-21 | C++ | Mod loader/scripting (Lua) para jogos RE Engine, inclui SF6 | Alta (estudo) | MIT; base de quase todos os mods SF6 |
| [alphazolam/EMV-Engine](https://github.com/alphazolam/EMV-Engine) | 177 | MIT | 2024-07-31 | Lua | Coleção de scripts REFramework | Baixa | MIT |
| [WistfulHopes/SF6Mods](https://github.com/WistfulHopes/SF6Mods) | 94 | none | 2026-01-07 | Lua | Mods Lua para SF6 (hitboxes, info display) | Média (estudo) | Sem licença. Estudado: disp_hitboxes.lua (só categorias de caixa) |
| [cursey/reframework-d2d](https://github.com/cursey/reframework-d2d) | 85 | MIT | 2025-04-07 | C++ | Plugin REFramework com Direct2D | Baixa | MIT |
| [alphazolam/MMDK](https://github.com/alphazolam/MMDK) | 65 | MIT | 2024-12-06 | Lua | Moveset Mod Dev Kit para SF6 via REFramework | Média (estudo) | MIT; gera os dumps usados pelo psychodrive |
| [RengarLee/sf6_scouter](https://github.com/RengarLee/sf6_scouter) | 53 | none | 2026-10-14 | HTML | Tracker de estatísticas SF6 | Baixa | Sem licença |
| [praydog/re-engine-mcp](https://github.com/praydog/re-engine-mcp) | 45 | MIT | 2026-03-11 | C# | Servidor MCP com acesso a jogo RE Engine via REFramework | Baixa | MIT |
| [Wael3rd/SF6_Tools](https://github.com/Wael3rd/SF6_Tools) | 22 | MIT | 2026-09-14 | Lua | Ferramentas Lua para treinar SF6 (distância etc.) | Baixa | MIT |
| [rkaganda/sf6_replays_data](https://github.com/rkaganda/sf6_replays_data) | 21 | none | 2025-08-30 | Lua | Captura/análise de replays SF6 | Baixa | Sem licença |
| [rappzIT/sf6-replay-bot-youtube](https://github.com/rappzIT/sf6-replay-bot-youtube) | 19 | MIT | 2026-07-26 | Python | Automação de gravação de replays SF6 | Baixa | MIT |
| [mkstksf6/mucharo-sf6-capture](https://github.com/mkstksf6/mucharo-sf6-capture) | 11 | none | 2026-06-18 | Lua | Captura de recursos (REFramework) para análise | Baixa | Sem licença |
| [Ali-Bueno/sf6Access](https://github.com/Ali-Bueno/sf6Access) | 10 | none | 2026-10-27 | C# | Mod de acessibilidade para SF6 | Baixa | Sem licença |
| [WistfulHopes/sf6_sim](https://github.com/WistfulHopes/sf6_sim) | 9 | none | 2023-07-02 | Rust | Simulador de dados de batalha SF6 (Rust/egui) | Média (estudo) | Sem licença; inclui arquivos .fchar do jogo em assets/: NÃO reutilizar. Estudado: simulator.rs |
| [ZGako/SF6-Training-Mode-Plus](https://github.com/ZGako/SF6-Training-Mode-Plus) | 9 | none | 2026-10-29 | Lua | Mod REFramework que expande o training mode do SF6 | Baixa | Sem licença |
| [lucasteles/sf6_re_scripts](https://github.com/lucasteles/sf6_re_scripts) | 8 | none | 2026-05-14 | Lua | Scripts REFramework para SF6 | Baixa | Sem licença |
| [skpeter/SmartCV-SF6](https://github.com/skpeter/SmartCV-SF6) | 8 | MIT | 2026-10-02 | Python | Visão computacional lendo dados ao vivo do SF6 | Baixa | MIT; abordagem sem tocar memória |

### J. King of Fighters (XV, XIV, XIII, 2002, clássicos) (10)

| Repo | ★ | Licença (API) | Último push | Ling. | O que é | Relevância | Observação |
|---|---:|---|---|---|---|---|---|
| [odabugs/kof-combo-hitboxes](https://github.com/odabugs/kof-combo-hitboxes) | 36 | NOASSERTION | 2018-10-19 | Lua | Hitbox viewer para vários KOF (98UM, 2002UM, XI, XIII...) | Média | Licença permissiva estilo HPND (LICENSE.txt verificado). Estudado: boxtypes_common.lua |
| [Slipcar/kof-xiii-global-match-rollback-enhanced](https://github.com/Slipcar/kof-xiii-global-match-rollback-enhanced) | 22 | none | 2026-10-31 | - | Melhoria de rollback KOF XIII (PT-BR) | Baixa | Sem licença; só downloads? |
| [odabugs/kof13-hitboxes](https://github.com/odabugs/kof13-hitboxes) | 20 | NOASSERTION | 2014-07-08 | Python | Hitbox viewer KOF XIII Steam | Baixa | NOASSERTION |
| [WydD/kofbox](https://github.com/WydD/kofbox) | 16 | MIT | 2018-04-13 | C++ | Hitbox viewer KOF XIV | Baixa | MIT |
| [videogamepreservation/aof](https://github.com/videogamepreservation/aof) | 16 | none | 2020-07-18 | Assembly | Código de Art of Fighting (1992) | Baixa | Sem licença; preservação, alto risco legal |
| [sp00nznet/neogeorecomp](https://github.com/sp00nznet/neogeorecomp) | 16 | MIT | 2026-10-21 | C | Runtime de recompilação estática de Neo Geo | Baixa | MIT |
| [Slipcar/kof-2002-um-hitbox-viewer](https://github.com/Slipcar/kof-2002-um-hitbox-viewer) | 11 | none | 2026-02-06 | Python | Overlay de hitbox KOF 2002 UM (PT-BR) | Baixa | Sem licença |
| [WistfulHopes/KOFXIVDecomp](https://github.com/WistfulHopes/KOFXIVDecomp) | 10 | none | 2022-12-10 | C++ | Decompilação de KOF XIV | Baixa | Sem licença; alto risco legal |
| [473671866/hitbox](https://github.com/473671866/hitbox) | 8 | MIT | 2024-12-15 | C++ | Plugin de hitbox para KOF XV (C++) | Média (estudo) | MIT; único RE de KOF XV encontrado. Cuidado: injeção em jogo online |
| [Kak2X/kof95](https://github.com/Kak2X/kof95) | 8 | none | 2025-02-14 | Assembly | Disassembly de KOF'95 (GB) | Baixa | Sem licença; alto risco legal |

### K. Ferramentas genéricas RE Engine / Unreal (pak, assets, injeção) (13)

| Repo | ★ | Licença (API) | Último push | Ling. | O que é | Relevância | Observação |
|---|---:|---|---|---|---|---|---|
| [4sval/FModel](https://github.com/4sval/FModel) | 3305 | GPL-3.0 | 2026-10-31 | C# | Explorador de arquivos Unreal (pak/utoc) — relevante para KOF XV (UE4) | Média (estudo) | GPL-3.0 |
| [UE4SS-RE/RE-UE4SS](https://github.com/UE4SS-RE/RE-UE4SS) | 3092 | MIT | 2026-10-31 | C++ | Scripting Lua injetável e gerador de SDK para jogos UE4/5 | Média (estudo) | MIT |
| [FabianFG/CUE4Parse](https://github.com/FabianFG/CUE4Parse) | 676 | Apache-2.0 | 2026-10-31 | C# | Parser de assets UE4/5 (C#) usado pelo FModel | Baixa | Apache-2.0 |
| [Ekey/REE.PAK.Tool](https://github.com/Ekey/REE.PAK.Tool) | 423 | none | 2026-10-21 | C# | Extrai/reempacota PAK de jogos RE Engine | Média (estudo) | Sem licença |
| [alphazolam/RE_RSZ](https://github.com/alphazolam/RE_RSZ) | 154 | MIT | 2026-03-30 | Roff | Template 010 Editor para arquivos RSZ do RE Engine | Baixa | MIT |
| [seifhassine/REasy](https://github.com/seifhassine/REasy) | 121 | MIT | 2026-10-31 | Python | Toolkit de modding RE Engine (Python) | Baixa | MIT |
| [kagenocookie/REE-Content-Editor](https://github.com/kagenocookie/REE-Content-Editor) | 119 | MIT | 2026-10-31 | C# | Editor 3D/patcher para jogos RE Engine | Baixa | MIT |
| [Havens-Night/REEngine-Modding-Documentation](https://github.com/Havens-Night/REEngine-Modding-Documentation) | 108 | none | 2026-08-24 | - | Documentação de modding RE Engine | Média | Sem licença; leitura |
| [tshao4/UnrealPakGUI](https://github.com/tshao4/UnrealPakGUI) | 83 | MIT | 2021-12-10 | C# | GUI para UnrealPak | Baixa | MIT |
| [WistfulHopes/UE4SSMods](https://github.com/WistfulHopes/UE4SSMods) | 54 | none | 2026-10-13 | C++ | Mods UE4SS (inclui frame data de Strive, hitboxes DNF Duel) | Baixa | Sem licença |
| [czastack/RszTool](https://github.com/czastack/RszTool) | 37 | none | 2025-03-29 | C# | Editor de recursos RSZ do RE Engine | Baixa | Sem licença |
| [Fexty12573/GhidraREFramework](https://github.com/Fexty12573/GhidraREFramework) | 14 | MIT | 2026-02-24 | Java | Scripts Ghidra para RE Engine | Baixa | MIT |
| [ENVYFGC/StriveLabs](https://github.com/ENVYFGC/StriveLabs) | 13 | none | 2026-10-27 | C++ | Mod UE4SS com overlay de frame data para Strive | Baixa | Sem licença |

### L. Engenharia reversa / mods / training de outros jogos (GG, BB, MB, Tekken, Smash...) (26)

| Repo | ★ | Licença (API) | Último push | Ling. | O que é | Relevância | Observação |
|---|---:|---|---|---|---|---|---|
| [project-slippi/project-slippi](https://github.com/project-slippi/project-slippi) | 599 | none | 2023-07-27 | JavaScript | Infra de replay/metadados para Melee | Média | Sem licença |
| [optix2000/totsugeki](https://github.com/optix2000/totsugeki) | 463 | BSD-3-Clause | 2022-12-31 | Go | Proxy para loading do GG Strive | Baixa | BSD-3-Clause |
| [jugeeya/UltimateTrainingModpack](https://github.com/jugeeya/UltimateTrainingModpack) | 422 | none | 2026-10-12 | Rust | Training mode avançado para Smash Ultimate (Rust) | Média | Sem licença; boa referência de features de training |
| [Raytwo/ARCropolis](https://github.com/Raytwo/ARCropolis) | 393 | GPL-3.0 | 2026-09-26 | Rust | Framework de modding Smash Ultimate | Baixa | GPL-3.0 |
| [project-slippi/Ishiiruka](https://github.com/project-slippi/Ishiiruka) | 379 | GPL-2.0 | 2026-10-26 | C++ | Fork do Dolphin com rollback Slippi | Média | GPL-2.0 |
| [altf4/libmelee](https://github.com/altf4/libmelee) | 261 | LGPL-3.0 | 2026-01-25 | Python | API Python para IA de Melee via Slippi | Média | LGPL-3.0 |
| [Kiloutre/TekkenMovesetExtractor](https://github.com/Kiloutre/TekkenMovesetExtractor) | 79 | GPL-3.0 | 2023-10-16 | Python | Extrai movesets Tekken 7 | Baixa | GPL-3.0 |
| [SokuDev/SokuMods](https://github.com/SokuDev/SokuMods) | 53 | Unlicense | 2024-09-06 | C++ | Mods/frameworks para Touhou 12.3 | Baixa | Unlicense |
| [dantarion/bbtools](https://github.com/dantarion/bbtools) | 49 | none | 2021-07-27 | Python | Parsers de arquivos do motor ArcSys (BlazBlue/P4A) | Média | Sem licença; formatos de script/hitbox |
| [kovidomi/BBCF-Improvement-Mod](https://github.com/kovidomi/BBCF-Improvement-Mod) | 43 | GPL-3.0 | 2023-05-14 | C++ | Mod de melhorias de BlazBlue CF (C++) | Média | GPL-3.0 |
| [vinceau/slp-realtime](https://github.com/vinceau/slp-realtime) | 42 | MIT | 2026-10-31 | TypeScript | Parser de slp em tempo real | Baixa | MIT |
| [sirmammingtonham/opendojo](https://github.com/sirmammingtonham/opendojo) | 41 | MIT | 2026-09-18 | C++ | Mod de practice mode T8 (import/export de drills) | Baixa | MIT |
| [Labreezy/rev2-wakeup-tool](https://github.com/Labreezy/rev2-wakeup-tool) | 34 | none | 2023-03-18 | C# | Timing de reversal no training mode GG Xrd | Baixa | Sem licença |
| [super-continent/ggxrd-mod](https://github.com/super-continent/ggxrd-mod) | 32 | none | 2026-04-29 | C++ | Modloader de scripts de personagem GG Xrd | Baixa | Sem licença |
| [pcrain/slippc](https://github.com/pcrain/slippc) | 32 | GPL-3.0 | 2026-02-07 | C++ | Parser de replays .slp em C++ | Baixa | GPL-3.0 |
| [Marisa-Chan/OpenSoku](https://github.com/Marisa-Chan/OpenSoku) | 30 | none | 2015-05-29 | C++ | Reimplementação aberta de Touhou 12.3 (C++) | Baixa | Sem licença |
| [fangdreth/MBAACC-Extended-Training-Mode](https://github.com/fangdreth/MBAACC-Extended-Training-Mode) | 25 | none | 2026-10-29 | C++ | Training mode estendido MBAACC | Baixa | Sem licença |
| [ParadiseAigo/Tekken-8-Frames-To-Speech](https://github.com/ParadiseAigo/Tekken-8-Frames-To-Speech) | 18 | MIT | 2026-02-27 | C++ | Anuncia frame advantage por voz (T8) | Baixa | MIT |
| [Zanaylo/UNI2-Improvement-Mod](https://github.com/Zanaylo/UNI2-Improvement-Mod) | 16 | GPL-3.0 | 2026-10-31 | C++ | Mod de melhoria UNI2 com overlay de hitbox | Baixa | GPL-3.0 |
| [super-continent/unPAC](https://github.com/super-continent/unPAC) | 14 | none | 2022-03-05 | Rust | Parser/rebuilder .pac de BBCF (Rust) | Baixa | Sem licença |
| [WistfulHopes/GGSTReplayTakeover](https://github.com/WistfulHopes/GGSTReplayTakeover) | 14 | MIT | 2024-01-29 | C# | Replay takeover para GG Strive | Baixa | MIT |
| [DPS-FGC/ACPR_IM](https://github.com/DPS-FGC/ACPR_IM) | 10 | GPL-3.0 | 2026-02-28 | C++ | Improvement mod para GGXX AC+R (CBR AI) | Baixa | GPL-3.0 |
| [ibrow19/GGXrdReplayTakeover](https://github.com/ibrow19/GGXrdReplayTakeover) | 8 | none | 2026-09-27 | C++ | Replay takeover/save states GG Xrd | Baixa | Sem licença |
| [Zanaylo/MBTL-Improvement-Mod](https://github.com/Zanaylo/MBTL-Improvement-Mod) | 5 | none | 2026-10-02 | C++ | Mod de melhoria MBTL | Baixa | Sem licença |
| [phytoporg/rev2-replays](https://github.com/phytoporg/rev2-replays) | 3 | MIT | 2025-01-08 | C | Parser de replays GG Xrd Rev2 (C) | Baixa | MIT; formato de replay como sequência de inputs |
| [kkots/ggxrd_replay_mod](https://github.com/kkots/ggxrd_replay_mod) | 0 | MIT | 2024-01-04 | C++ | Replay mod GG Xrd | Baixa | MIT |

### M. Decompilações e recompilações (só leitura histórica, alto risco) (9)

| Repo | ★ | Licença (API) | Último push | Ling. | O que é | Relevância | Observação |
|---|---:|---|---|---|---|---|---|
| [doldecomp/melee](https://github.com/doldecomp/melee) | 2450 | none | 2026-10-28 | C | Decompilação de Smash Melee | Baixa (estudo) | Sem licença; alto risco legal |
| [Ekspym/Tekken6-native-online](https://github.com/Ekspym/Tekken6-native-online) | 441 | none | 2026-10-28 | - | Tekken 6 nativo com rollback (requer ISO) | Baixa | Sem licença |
| [crowded-street/3s-decomp](https://github.com/crowded-street/3s-decomp) | 284 | AGPL-3.0 | 2026-04-03 | C | Decompilação de SF III 3rd Strike (PS2) | Baixa (estudo) | AGPL-3.0; código derivado de jogo comercial |
| [vyanhursky/DefJamFFNY-recomp](https://github.com/vyanhursky/DefJamFFNY-recomp) | 206 | NOASSERTION | 2026-10-31 | Python | Recompilação estática de Def Jam FFNY | Baixa | NOASSERTION |
| [tmcleroy/cps2recomp](https://github.com/tmcleroy/cps2recomp) | 136 | GPL-3.0 | 2026-10-31 | C++ | Ports nativos de jogos CPS-2 por recompilação | Baixa | GPL-3.0; exige ROM |
| [mr-finster/vf2-recomp](https://github.com/mr-finster/vf2-recomp) | 79 | NOASSERTION | 2026-10-30 | C++ | Recompilação de Virtua Fighter 2 | Baixa | NOASSERTION |
| [gibletto/sfIII3-cps3-decomp](https://github.com/gibletto/sfIII3-cps3-decomp) | 37 | AGPL-3.0 | 2026-10-31 | C | Decomp byte-matched de SF III 3rd Strike (CPS3) | Baixa (estudo) | AGPL-3.0; alto risco legal |
| [zxmarcos/mugen-unleashed](https://github.com/zxmarcos/mugen-unleashed) | 35 | none | 2026-10-23 | C | Decompilação do MUGEN 1.1b | Baixa | Sem licença |
| [S22F5/LethalLeague](https://github.com/S22F5/LethalLeague) | 0 | none | 2026-02-25 | C++ | Fonte recuperada de Lethal League por debug sections | Baixa | Sem licença; alto risco legal |

### N. Ecossistema MUGEN / IKEMEN (13)

| Repo | ★ | Licença (API) | Último push | Ling. | O que é | Relevância | Observação |
|---|---:|---|---|---|---|---|---|
| [CableDorado2/Ikemen-Plus-Ultra](https://github.com/CableDorado2/Ikemen-Plus-Ultra) | 109 | NOASSERTION | 2026-10-26 | C | Branch do IKEMEN clássico (S-SIZE) | Baixa | NOASSERTION |
| [jefersondaniel/godot-mugen](https://github.com/jefersondaniel/godot-mugen) | 46 | BSD-3-Clause | 2025-05-15 | GDScript | Clone de MUGEN em Godot | Média | BSD-3-Clause |
| [ermaccer/mugenhook](https://github.com/ermaccer/mugenhook) | 45 | none | 2023-12-27 | C++ | Hook que adiciona features ao MUGEN | Baixa | Sem licença |
| [scemino/xnamugen](https://github.com/scemino/xnamugen) | 44 | BSD-3-Clause | 2018-04-17 | C# | Clone MUGEN em MonoGame | Baixa | BSD-3-Clause |
| [Tatayecorp/demo_mugenJS](https://github.com/Tatayecorp/demo_mugenJS) | 43 | MIT | 2021-04-03 | JavaScript | Anima personagens MUGEN em JS (DEF/AIR/SFF/ACT) | Baixa | MIT; referência de formatos |
| [bitcraft/mugen-tools](https://github.com/bitcraft/mugen-tools) | 34 | none | 2017-03-16 | Python | Ferramentas Python para MUGEN | Baixa | Sem licença |
| [CaptainDreamcast/DolmexicaInfinite](https://github.com/CaptainDreamcast/DolmexicaInfinite) | 33 | MIT | 2026-09-08 | C++ | Port tipo MUGEN para Dreamcast (C++) | Baixa | MIT |
| [oldmud0/LilithPort](https://github.com/oldmud0/LilithPort) | 27 | none | 2017-10-01 | C++ | Netplay para jogos 2D Fighter Maker | Baixa | Sem licença |
| [ikemen-engine/Ikemen-GO-Screenpack](https://github.com/ikemen-engine/Ikemen-GO-Screenpack) | 25 | NOASSERTION | 2026-10-24 | Batchfile | Screenpack padrão do Ikemen GO | Baixa | NOASSERTION |
| [roundonejs/roundonejs](https://github.com/roundonejs/roundonejs) | 25 | MIT | 2021-02-27 | TypeScript | MUGEN no browser | Baixa | MIT |
| [energyjp/mugen-studio](https://github.com/energyjp/mugen-studio) | 21 | MIT | 2026-10-29 | JavaScript | Criador de jogo MUGEN drag-and-drop | Baixa | MIT |
| [SuperFromND/iguana](https://github.com/SuperFromND/iguana) | 19 | MIT | 2024-08-02 | Go | Gera movelists Ikemen GO automaticamente (Go) | Baixa | MIT; parser de .cmd |
| [humbertodias/unity-mugen](https://github.com/humbertodias/unity-mugen) | 17 | BSD-3-Clause | 2024-02-12 | C# | Mecânicas MUGEN portadas para Unity | Baixa | BSD-3-Clause |

### O. IA, bots e aprendizado por reforço (11)

| Repo | ★ | Licença (API) | Último push | Ling. | O que é | Relevância | Observação |
|---|---:|---|---|---|---|---|---|
| [linyiLYi/street-fighter-ai](https://github.com/linyiLYi/street-fighter-ai) | 6548 | Apache-2.0 | 2024-05-14 | Python | Agente RL (PPO) para SF II via gym-retro | Média | Apache-2.0; ROM comercial necessária |
| [OpenGenerativeAI/llm-colosseum](https://github.com/OpenGenerativeAI/llm-colosseum) | 1484 | MIT | 2025-03-21 | Jupyter Notebook | Benchmark de LLMs lutando em SF III | Baixa | MIT |
| [vietnh1009/Street-fighter-A3C-ICM-pytorch](https://github.com/vietnh1009/Street-fighter-A3C-ICM-pytorch) | 165 | MIT | 2019-11-11 | Python | A3C + curiosidade para SF III | Baixa | MIT |
| [TeamFightingICE/FightingICE](https://github.com/TeamFightingICE/FightingICE) | 71 | none | 2026-09-14 | Java | Plataforma acadêmica de IA para jogos de luta (Java) | Média | Sem licença; competição anual (CIG/CoG) |
| [nickslevine/budok-ai](https://github.com/nickslevine/budok-ai) | 63 | MIT | 2026-04-19 | Python | Mod LLM para YOMI Hustle | Baixa | MIT |
| [DRL-CASIA/GameAI-FightingAI](https://github.com/DRL-CASIA/GameAI-FightingAI) | 36 | MIT | 2020-09-05 | Java | IA para FightingICE | Baixa | MIT |
| [dantarion/UltraBotFramework](https://github.com/dantarion/UltraBotFramework) | 30 | MIT | 2015-10-09 | C# | Framework de bots para USF4 PC | Baixa | MIT |
| [corbosiny/AIVO-StreetFigherReinforcementLearning](https://github.com/corbosiny/AIVO-StreetFigherReinforcementLearning) | 23 | Unlicense | 2023-03-25 | Python | Ambiente de RL para SF | Baixa | Unlicense |
| [altf4/smashbot_analysis](https://github.com/altf4/smashbot_analysis) | 13 | GPL-3.0 | 2020-11-19 | Python | Barra de vantagem por IA para Melee | Baixa | GPL-3.0 |
| [Jimmyu2foru18/SF6-AI-Bot](https://github.com/Jimmyu2foru18/SF6-AI-Bot) | 10 | none | 2026-08-08 | Python | Bot SF6 por visão computacional + LLM | Baixa | Sem licença; uso online viola ToS |
| [como-research/FootsiesGym](https://github.com/como-research/FootsiesGym) | 5 | GPL-3.0 | 2026-10-23 | Python | Footsies como ambiente de RL (Python) | Média | GPL-3.0 |

### P. Emuladores, netplay arcade e training mode em Lua (11)

| Repo | ★ | Licença (API) | Último push | Ling. | O que é | Relevância | Observação |
|---|---:|---|---|---|---|---|---|
| [flyinghead/flycast](https://github.com/flyinghead/flycast) | 2633 | GPL-2.0 | 2026-10-31 | C++ | Emulador Dreamcast/Naomi com GGPO (core/network/ggpo.cpp) | Baixa | GPL-2.0 |
| [finalburnneo/FBNeo](https://github.com/finalburnneo/FBNeo) | 1559 | NOASSERTION | 2026-10-31 | C++ | Emulador arcade (CPS/NeoGeo) com netplay GGPO (FightCade) | Média | Licença não comercial própria (src/license.txt verificado) |
| [blueminder/flycast-dojo](https://github.com/blueminder/flycast-dojo) | 246 | GPL-2.0 | 2026-10-24 | C++ | Fork do Flycast com netplay/rollback e replays | Média | GPL-2.0 |
| [Grouflon/3rd_training_lua](https://github.com/Grouflon/3rd_training_lua) | 208 | none | 2024-06-13 | Lua | Training mode Lua para SF III 3S (FightCade) | Média | Sem licença; ótima referência de features de training |
| [MisterTea/MAMEHub](https://github.com/MisterTea/MAMEHub) | 128 | none | 2018-09-15 | C | Emulador multi-máquina com netplay | Baixa | Sem licença |
| [peon2/fbneo-training-mode](https://github.com/peon2/fbneo-training-mode) | 83 | none | 2026-07-31 | Lua | Training mode Lua multi-jogo no FBNeo | Média | Sem licença |
| [blueminder/supermodel-dojo](https://github.com/blueminder/supermodel-dojo) | 73 | none | 2026-10-29 | C++ | Fork do Supermodel (Model 3) com netplay | Baixa | Sem licença |
| [effie3rd/3rd_training_lua](https://github.com/effie3rd/3rd_training_lua) | 42 | GPL-3.0 | 2026-05-24 | Lua | Fork mantido do 3rd_training_lua | Baixa | GPL-3.0 |
| [Zendevve/OpenCade](https://github.com/Zendevve/OpenCade) | 29 | Apache-2.0 | 2026-09-01 | Rust | Plataforma de netplay arcade clean-room (Rust/Tauri) | Baixa | Apache-2.0 |
| [maximusmaxy/JoJoban-Training-Mode-Menu-FBNeo](https://github.com/maximusmaxy/JoJoban-Training-Mode-Menu-FBNeo) | 16 | none | 2022-02-16 | Lua | Training mode JoJo HFTF | Baixa | Sem licença |
| [poliva/fightcadestates](https://github.com/poliva/fightcadestates) | 15 | none | 2022-03-21 | Python | Savestates para FightCade | Baixa | Sem licença |

### Q. Apresentação: shaders, câmera, UI, streaming, aprendizado (14)

| Repo | ★ | Licença (API) | Último push | Ling. | O que é | Relevância | Observação |
|---|---:|---|---|---|---|---|---|
| [Aerthas/UNITY-Arc-system-Works-Shader](https://github.com/Aerthas/UNITY-Arc-system-Works-Shader) | 438 | GPL-3.0 | 2022-08-23 | GLSL | Shader estilo ArcSys (GG/DBFZ) para Unity | Média | GPL-3.0; referência de toon shading 2.5D |
| [muzudho/KifuwarabeFighter2](https://github.com/muzudho/KifuwarabeFighter2) | 53 | none | 2022-01-09 | C# | Tutorial JP de jogo 2D de luta em Unity | Baixa | Sem licença |
| [mspkvp/tk7movespretty](https://github.com/mspkvp/tk7movespretty) | 44 | MIT | 2022-12-06 | HTML | Site comunitário de aprendizado T7 | Baixa | MIT |
| [galloscript/GGXrdShading](https://github.com/galloscript/GGXrdShading) | 41 | none | 2024-08-06 | JavaScript | Demo de técnicas de toon shading do GG Xrd | Média | Sem licença; estudo |
| [GatitoMimoso00/3D-Fighting-Camera](https://github.com/GatitoMimoso00/3D-Fighting-Camera) | 36 | MIT | 2025-11-21 | GDScript | Câmera de jogo de luta 3D com matemática documentada (Godot) | Média | MIT |
| [ATHellboy/SampleProject-FightingGame](https://github.com/ATHellboy/SampleProject-FightingGame) | 36 | MIT | 2025-07-18 | C# | Projeto exemplo de arquitetura C# para jogo de luta | Baixa | MIT |
| [BrandonCookeDev/smashgg.js](https://github.com/BrandonCookeDev/smashgg.js) | 34 | none | 2024-03-29 | TypeScript | SDK da API start.gg (smash.gg) | Baixa | Sem licença |
| [Drigax/RooftopRampage_Source](https://github.com/Drigax/RooftopRampage_Source) | 29 | none | 2024-08-30 | TypeScript | Jogo 2.5D com sprites em Babylon.js | Baixa | Sem licença; exemplo 2.5D |
| [MadeInShineA/melee-resources](https://github.com/MadeInShineA/melee-resources) | 27 | none | 2025-09-12 | HTML | Hub de recursos Melee | Baixa | Sem licença |
| [nhanb/gorts](https://github.com/nhanb/gorts) | 18 | GPL-3.0 | 2026-08-21 | Go | Overlay de placar para streams | Baixa | GPL-3.0 |
| [WASD-Gaming/fgc-scoreboard](https://github.com/WASD-Gaming/fgc-scoreboard) | 16 | MIT | 2020-07-02 | JavaScript | Scoreboard HTML para torneios | Baixa | MIT |
| [SindreVatnaland/Froggi](https://github.com/SindreVatnaland/Froggi) | 14 | MIT | 2026-10-31 | Svelte | Ferramenta de stream/torneio Melee | Baixa | MIT |
| [rogueyoshi/dbfz-css](https://github.com/rogueyoshi/dbfz-css) | 10 | MIT | 2022-10-30 | CSS | Recriação em CSS da UI do DBFZ | Baixa | MIT; IP de terceiros |
| [timglasser/3DFightingGameCamera](https://github.com/timglasser/3DFightingGameCamera) | 3 | none | 2017-01-19 | C# | Câmera estilo Tekken/SC (Unity) | Baixa | Sem licença |

### R. ALERTA: repositórios suspeitos (iscas "hack/trainer/2026") (10)

| Repo | ★ | Licença (API) | Último push | Ling. | O que é | Relevância | Observação |
|---|---:|---|---|---|---|---|---|
| `carextwo/Unverum-Mod-Manager-For-Anime-Games` | 87 | MIT | 2026-03-23 | C# | "Unverum Mod Manager 2026" | Evitar | Possível reupload/isca do Unverum original (verificar fonte oficial) |
| `oaknember/Strive-Assist-Training-Hub` | 68 | none | 2026-10-22 | HTML | "Strive Assist Suite 2026" | Evitar | Padrão isca (HTML "guia de setup seguro") |
| `pinkeshqualityanalyst/combo-drill-arena` | 56 | none | 2026-10-31 | HTML | "Best Fighting Game Combo Trainer 2026" | Evitar | Padrão SEO/isca |
| `manuelro2110-wq/GGST-Precision-Toolset` | 55 | none | 2026-10-31 | HTML | "GGST Ultimate Training Mod 2026" | Evitar | Padrão isca |
| `PlasmaFoxRise/street-fighter-6-hack-2026-world-warrior-toolkit` | 13 | none | 2026-10-02 | - | "SF6 Hack 2026" companion | Evitar | Nome/descrição de isca; não baixar |
| `mushyatelie/Tekken-8-Fight-Mod-Money-Unlocker` | 12 | MIT | 2026-10-15 | - | Unlocker de moeda Tekken 8 | Evitar | Isca + viola ToS |
| `nuttymeaning/Street-Fighter-6-Mod` | 10 | MIT | 2026-10-15 | - | "Drive Ticket Generator" para SF6 | Evitar | Promete moeda premium grátis: padrão típico de isca/malware e viola ToS |
| `noah-diaz300/street-fighter-6-hack-combo-training-toolkit` | 10 | none | 2026-09-12 | - | "SF6 Hack" toolkit | Evitar | Isca provável |
| `lavashinobitram33/street-fighter-6-crash-fix` | 9 | NOASSERTION | 2026-08-03 | - | "SF6 Crash Fix" setup | Evitar | Isca provável (instalador opaco) |
| `mikerainbow07/SF6-Training-Tool-Complete-Frame-Data-Utility-for-Street-Fighter-6` | 9 | none | 2026-05-23 | - | "SF6 Training Tool" sem código | Evitar | Repo sem linguagem/código; suspeito |


## 4. Análise de código (leitura direta)

Repositórios lidos (clone raso, só leitura). Para cada um: descoberta, onde, evidência, como funciona, o que adaptar, como fazer versão original, limitações.

### 4.1 pond3r/ggpo (MIT, C++, push 2024-06-26)

- **Descoberta [Fato]:** a API pública é um conjunto de callbacks: `save_game_state(unsigned char **buffer, int *len, int *checksum, int frame)`, `load_game_state(buffer, len)`, `advance_frame(int flags)`, `log_game_state`, `free_buffer`, `on_event`.
- **Onde:** `src/include/ggponet.h` (linhas ~197–249); `src/lib/ggpo/sync.h|.cpp`; `src/lib/ggpo/input_queue.cpp`; `src/lib/ggpo/backends/synctest.cpp`; `src/lib/ggpo/timesync.cpp`; exemplo `src/apps/vectorwar/vectorwar.cpp`.
- **Evidência [Fato]:** `#define GGPO_MAX_PREDICTION_FRAMES 8`, `GGPO_MAX_PLAYERS 4`; `SavedState { SavedFrame frames[MAX_PREDICTION_FRAMES + 2]; int head; }` (ring buffer de estados); `INPUT_QUEUE_LENGTH 128`; `GAMEINPUT_MAX_BYTES 9` × `GAMEINPUT_MAX_PLAYERS 2` (`game_input.h`).
- **Como funciona [Fato]:**
  - *Predição*: `InputQueue::GetInput` devolve o input confirmado se existe; senão repete o último input recebido (`_prediction = _inputs[PREVIOUS_FRAME(_head)]`) e marca predição. Quando chega o input real e difere, registra `_first_incorrect_frame`.
  - *Rollback*: `Sync::CheckSimulationConsistency` pega o menor `first_incorrect` entre as filas; `Sync::AdjustSimulation(seek_to)` faz `LoadFrame(seek_to)` e chama `advance_frame` `count` vezes com `_rollingback = true`.
  - *Save*: `Sync::SaveCurrentFrame` libera o buffer antigo do slot e pede ao jogo um novo snapshot + checksum.
  - *Sync test*: `SyncTestBackend::IncrementFrame` guarda checksum de cada frame; a cada `check_distance` frames recarrega o último frame verificado, re-simula e compara checksums (`RaiseSyncError` em divergência).
  - *Time sync*: `TimeSync::recommend_frame_wait_duration` faz média das vantagens de frame local/remota e recomenda esperar frames se estivermos à frente.
  - *Estado no exemplo*: VectorWar serializa o `GameState` inteiro com `memcpy` de struct POD e checksum Fletcher-32 (`fletcher32_checksum`).
- **O que adaptar [Inferência]:** o contrato save/load/advance + sync test é o padrão de fato da indústria; nosso core deve expor exatamente essas três operações, independente da lib de rede.
- **Versão original:** `struct MatchState` POD (trivially copyable, `static_assert(std::is_trivially_copyable_v<MatchState>)`), `save()` = `memcpy` para ring buffer próprio de N=8+2 slots pré-alocados (sem `malloc` por frame, ao contrário do callback do GGPO), checksum barato (FNV-1a/xxHash) calculado sobre o bloco; modo "synctest" no nosso harness de testes.
- **Limitações [Fato/Inferência]:** API C com `__cdecl`, alocação por frame via callback, sem push desde 2024, Windows-cêntrico (VectorWar usa GDI); spectator e 2 jogadores fixos em `GAMEINPUT_MAX_PLAYERS 2`.

### 4.2 gschup/ggrs (MIT/Apache-2.0, Rust, push 2026-10-19)

- **Descoberta [Fato]:** inverte a API do GGPO: em vez de callbacks, a sessão devolve uma lista de `GgrsRequest` — `SaveGameState { cell, frame }`, `LoadGameState { cell, frame }`, `AdvanceFrame { inputs: Vec<(Input, InputStatus)> }`.
- **Onde:** `src/lib.rs` (linhas ~160–333), `src/sync_layer.rs` (`GameStateCell`), `src/sessions/builder.rs`.
- **Evidência [Fato]:** `InputStatus { Confirmed, Predicted, Disconnected }`; `DesyncDetection::On { interval }` ("at 60hz an interval of 10 results in 6 reports a second"); `GameStateCell::save(frame, data: Option<T>, checksum: Option<u128>)`; defaults em `builder.rs`: `DEFAULT_MAX_PREDICTION_FRAMES = 8`, `DEFAULT_INPUT_DELAY = 0`, `DEFAULT_CHECK_DISTANCE = 2`, `DEFAULT_MAX_FRAMES_BEHIND = 10`, `DEFAULT_FPS = 60`, `DEFAULT_DISCONNECT_TIMEOUT = 2000ms`; `with_sparse_saving_mode` ("only the minimum confirmed frame ... will be saved").
- **Como funciona:** o jogo processa as requests em ordem; desync detection troca checksums periodicamente com o par e emite `GgrsEvent::DesyncDetected` (`p2p_session.rs` ~1315).
- **O que adaptar [Inferência]:** o modelo "lista de requests" é mais fácil de integrar num core C++ puro (sem callbacks reentrantes) e facilita testes determinísticos. `InputStatus` por jogador é útil para a camada de apresentação (ex.: não tocar efeito sonoro "forte" em frame predito).
- **Versão original:** `std::span<const SimRequest>` com `std::variant<Save, Load, Advance>`; sparse saving como opção quando o snapshot for caro.
- **Limitações:** Rust (não linkável trivialmente em C++ sem FFI); estudar o design, não o código.

### 4.3 HeatXD/GekkoNet (BSD-2-Clause, C++20 com API C, push 2026-10-24)

- **Descoberta [Fato]:** SDK C++20 com API C, modelo de eventos por polling (como GGRS), e tipos de sessão `GekkoGameSession`, `GekkoStressSession` ("test your local simulation for state desyncs"), `GekkoSpectateSession`, `GekkoReplaySession`.
- **Onde:** `GekkoLib/include/gekkonet.h`; `GekkoLib/src/event.cpp`, `game_session.cpp`; exemplos `Examples/LocalSession/LocalSession.cpp`, `Examples/OnlineSession/OnlineSession.cpp`; `GekkoLib/CMakeLists.txt` (`CMAKE_CXX_STANDARD 20`).
- **Evidência [Fato]:** `GekkoConfig { num_players, max_spectators, max_input_prediction_window, spectator_delay, input_size, state_size, limited_saving, desync_detection, check_distance }`; eventos `GekkoAdvanceEvent {frame, inputs, rolling_back, running_ahead}`, `GekkoSaveEvent {frame, portable, checksum*, state_len*, state*}`, `GekkoLoadEvent`; `GekkoNetAdapter` com `send_data/receive_data/free_data` (transporte plugável); API de replay `gekko_start_recording`, `gekko_record_checksums`, `gekko_load_replay`; `gekko_set_runahead`. Nos exemplos o estado é `memcpy(event->data.save.state, &gs.state, sizeof(Gamestate::State))` com `config.state_size = sizeof(Gamestate::State)`.
- **Como funciona:** o buffer de estado tem tamanho fixo declarado no início (`state_size`) e é de propriedade da lib; o jogo só escreve nele. O flag `portable` é passado como `true` nos saves de spectator (`game_session.cpp` ~744/768) [Inferência: estado para transferir a espectadores tardios].
- **O que adaptar [Inferência]:** é a opção mais alinhada ao nosso stack (C++20, licença permissiva BSD-2, transporte desacoplado, replay com checksums embutidos, stress session = synctest). Candidata nº 1 para **reuso direto** ou como referência principal se escrevermos a nossa.
- **Versão original:** mesmo se usarmos GekkoNet, manter nossa interface `ISimulation { save(span<byte>), load(span<const byte>), advance(const FrameInputs&) }` para poder trocar de lib.
- **Limitações:** projeto de um mantenedor (risco de bus factor); estrelas modestas (64); revisar maturidade e testes antes de adotar.

### 4.4 HouraiTeahouse/backroll-rs (ISC, Rust, push 2023-03-13)

- **[Fato]** `backroll/src/command.rs`: enum `Command { Save(SaveState), Load(LoadState), AdvanceFrame(GameInput), Event(Event) }`; `SaveState` "MUST save before the command is dropped ... will result in a panic"; checksum opcional de 64 bits. `input.rs` usa ring buffer fixo `[FrameInput<T>; MAX_ROLLBACK_FRAMES]`. Transporte separado em crates (`backroll_transport_udp`, `backroll_transport_steam`).
- **[Inferência]** Boa ideia a copiar em espírito: falhar alto se o jogo esquecer de salvar; transporte como módulo trocável (UDP/Steam). Projeto parado desde 2023.

### 4.5 MikeLankamp/fpm (MIT, C++ header-only, push 2026-03-29)

- **[Fato]** `include/fpm/fixed.hpp`: `template <typename BaseType, typename IntermediateType, unsigned int FractionBits, bool EnableRounding = true> class fixed`; aliases `fixed_16_16 = fixed<int32_t,int64_t,16>`, `fixed_24_8`, `fixed_8_24`; `from_raw_value()`/`raw_value()`; multiplicação com arredondamento ("multiplying by two before dividing and adding the LSB"); construtor `constexpr explicit` a partir de float com arredondamento. `include/fpm/math.hpp` implementa `sqrt`, `sin`, `atan2`, `floor`, `round` etc. em fixed.
- **Como usar [Inferência]:** pode virar nosso tipo de posição/velocidade. Conversões float→fixed só em `constexpr` (tempo de compilação) ou em ferramentas offline, nunca no loop da simulação.
- **Limitações [Inferência]:** `int32` com 16 bits fracionários dá alcance de ±32768 unidades — suficiente para um palco 2.5D se a unidade for ~1 px/1 cm; overflow silencioso (sem checagem), então precisamos de testes de faixa.

### 4.6 PetteriAimonen/libfixmath (MIT verificado, C, push 2026-10-30)

- **[Fato]** `libfixmath/fix16.h`: tipo `fix16_t` Q16.16, sentinela `fix16_overflow = 0x80000000`, flags de compilação `FIXMATH_NO_OVERFLOW`, `FIXMATH_NO_ROUNDING`; arquivos `fix16_trig.c` (com `fix16_trig_sin_lut.h`), `fix16_sqrt.c`, `fix16_exp.c`.
- **[Inferência]** Alternativa em C com detecção de overflow; útil como referência de algoritmos de trig/sqrt em inteiro. Para C++20 preferimos um tipo próprio inspirado em fpm.

### 4.7 MicahMartin/FightingGameEngine (MIT, C++/SDL2, push 2025-05-23)

- **Descoberta [Fato]:** motor didático determinístico ("Functional, but definitely not shippable!", README) com DSL de script (`src/domain_language/`: Scanner, Compiler, VirtualMachine) e compilador de comandos de input.
- **Input [Fato]** `include/input/VirtualController.h`: botões e direções como bits em `uint16_t` (`RIGHT=0x1, LEFT=0x2, UP=0x4, DOWN=0x8, LP=0x10 ... HK=0x400`), `INPUT_BUFFER_MAX = 128`, histórico em `boost::circular_buffer<InputFrameT>` onde cada frame é um `std::vector<InputEvent{inputBit, pressed}>`; API `wasPressed(input, strict, index)`, `wasReleased`, `isPressed`, `checkCommand(commandIndex, faceRight)`.
- **Comandos [Fato]** `src/input/CommandCompiler.cpp`: strings de comando compiladas para pilhas de funções booleanas, ex. comentário: `P | ~P = ((wasPressed(LP)) || (wasReleased(LP)))`, `@F & !D`, `MP & *D`, `~D = (wasReleased(D))`; cada nó tem `bufferLength`; `CommandObj.clears` apaga o histórico consumido. `VirtualController::checkCommand` percorre os nós do último para o primeiro, procurando cada parte dentro de `bufferLength` frames a partir do offset da parte anterior.
- **Hitbox/estado [Fato]** `data/characters/samurai/def.json`: estado `STAND_P` com `collision_boxes` tipadas `POSITION`, `HIT`, `HURT`, `THROW_HURT`, `PROXIMITY`, cada uma com `start`/`end` (frames), `width/height/offsetX/offsetY`; caixa `HIT` carrega `hitstun 10`, `block_stun 4`, `hitstop 10`, `damage 3`, `push 11000`, `push_time 8`, `hit_velX 10000`, `group 1`, `meter 10` (inteiros escalados). Janela de cancel em script `scripts/attacks/stand_p_cancels.kai`: `inCancelWindow = (stateTime >= 3) and (stateTime <= 6)` combinado com `$get_hit_cancel`.
- **Rollback [Fato]** `include/game_objects/Character.h`: `struct CharStateObj` só com `int`/`bool`/`long` (hitstun, blockstun, hitStop, velocityX/Y, positionX/Y, health, meter, cancelPointer, inHitStop, faceRight...), com `saveState()`/`loadState()`.
- **Adaptar [Inferência]:** (a) bitmask de 16 bits por frame é o formato ideal para enviar pela rede; (b) parser de notação de comandos estilo MUGEN compilado para dados (não `std::function`!); (c) dados de hitbox com start/end e propriedades de hit no próprio box.
- **Versão original:** em vez de `std::function`/`boost::circular_buffer`/`std::vector` por frame (heap, não-POD), usar `std::array<uint16_t, 64>` circular dentro do estado do jogador + tabela de comandos compilada para bytecode/structs constexpr.
- **Limitações:** README diz explicitamente não ser "shippable"; usa Boost/SDL2/JSON em runtime; histórico com heap complica snapshot por `memcpy`.

### 4.8 WistfulHopes/NightSkyEngine (MIT, C++/Unreal, push 2026-08-08)

- **Input buffer [Fato]** `Plugins/NightSkyEngine/Source/NightSkyEngine/Battle/Misc/InputBuffer.h`: `InputBufferSize = 0x20` (32 frames), `InputSequenceSize = 0x10`; arrays fixos `int32 InputBufferInternal[32]`, `int8_t InputBufferValid[32]`, `int32 InputTime[32]`; `Update(int32 Input, bool bStopped)` (não avança em hitstop/super freeze); métodos `CheckInputSequence`, `...Strict`, `...Once`, `...OnceStrict`, `...PressAndRelease(Strict)`, `...Negative(Strict)`
- **Condições [Fato]** `Battle/Script/State.h`: `EInputMethod { Normal, Strict, Once, OnceStrict, ... }` — "Strict: Diagonal directions are not counted as either of the cardinal directions"; `FInputBitmask { InputFlag; Lenience = 3; Hold = 0; DisallowedInputs }`; `FInputCondition { Sequence; DisallowedInputs; ImpreciseInputCount }`.
- **Caixas [Fato]** `Battle/Misc/CollisionBox.h`: `EBoxType { BOX_Push, BOX_Hurt, BOX_Hit, BOX_Offset ("semi-root motion"), BOX_Custom }`, posições `int32 PosX/PosY`.
- **Snapshot [Fato]** `Battle/Objects/BattleObject.h`: membros entre os marcadores `uint32 ObjSync` e `uint32 ObjSyncEnd` são salvos por `FMemory::Memcpy(Buffer, &ObjSync, SizeOfBattleObject)` com `SizeOfBattleObject = offsetof(ABattleObject, ObjSyncEnd) - offsetof(ABattleObject, ObjSync)` e `static_assert` de consistência; `PlayerObject.h` tem marcador `PlayerSyncEnd` ("Anything past here isn't saved or loaded for rollback"). `Misc/Globals.h` define `COORD_SCALE` e uma tabela `constexpr int gSinTable[900]` (seno por LUT inteira). `NightSkyGameState.h`: `FRollbackData` com `TArray<uint8> BattleStateBuffer`, `CreateChecksum()`, `SetOtherChecksum(...)`.
- **Cancels [Fato]** `PlayerObject.h`: `uint32 CancelFlags`, `int32 AutoComboCancels[8]`, `EnableCancelIntoSelf`, `AddChainCancelOption(FGameplayTag)`; estados nomeados por `FGameplayTag` (`BufferedStateName`).
- **Adaptar [Inferência]:** o truque `ObjSync..ObjSyncEnd` + `offsetof` é a forma mais barata de snapshot; em C++20 puro podemos fazer melhor separando explicitamente `SimState` (POD) de `ViewState` (apresentação). "Lenience" por passo e métodos Strict/Once são um vocabulário bom para nosso parser de comandos. LUT de seno inteira evita trig em float.
- **Limitações:** acoplado ao Unreal (UPROPERTY, FGameplayTag, TArray); `offsetof` em classes não-standard-layout é comportamento condicionalmente suportado [Inferência: funciona no compilador do UE, mas não é portátil].

### 4.9 ikemen-engine/Ikemen-GO (MIT verificado, Go, push 2026-10-31)

- **Input [Fato]** `src/input.go`: `InputBuffer` guarda, por tecla, um contador `int32`: positivo = frames segurado, negativo = frames solto; `updateInputTime` → `update := func(held bool, buffer *int32)` (troca para `1`/`-1` na mudança, depois incrementa/decrementa). Hierarquia `CommandList > Command > CommandStep > CommandStepKey`; `CommandStepKey { key, slash(segurar), tilde(soltar), dollar(direção "inclui diagonais"), star, chargetime }`; `Command { steps, maxtime, maxbuftime, maxsteptime, buffer_hitpause, buffer_pauseend, ... }` — notação MUGEN `/`, `~`, `$`, `>`.
- **SOCD [Fato]** `InputReader.SocdResolution(U, D, B, F)` com métodos configuráveis via `sys.cfg.Input.SOCDResolution` (comentários no código: `0` = permitir ambos, `1` = última direção tem prioridade, `2` = prioridade absoluta "offense over defense", `3` = primeira direção tem prioridade, default = neutro/nega ambos).
- **Rollback [Fato]** `src/rollback.go` importa `github.com/ikemen-engine/ggpo` (port Go do GGPO); inputs de rede `InputBits` + `[6]int8` analógico; `saveStates map[int]*GameState` com pool; `state_clone.go` faz clone campo a campo. Checksum `GameState.Checksum()` em `state.go`: FNV-1a 32 bits sobre `gs.String()` (string de depuração de algumas variáveis), com a serialização gob comentada.
- **Física [Fato]** `char.go`: posições `pos [3]float32`, `vel`. **[Inferência]** determinismo depende de mesmo binário/plataforma; não é o modelo que queremos.
- **Adaptar:** contador assinado held/released por botão (muito compacto e serializável), notação MUGEN como formato de autoria, SOCD configurável. **Versão original:** mesma ideia com `int16_t` saturado por botão dentro do `PlayerInputState` POD.

### 4.10 panthavma/castagne (MPL-2.0 verificado, GDScript, push 2026-07-28)

- **[Fato]** `engine/CastagneMemory.gd`: estado guardado em dicionários `_memoryGlobal`, `_memoryPlayers[]`, `_memoryEntities[]` com `CopyFrom(other)` (cópia inteira para rollback). `modules/attacks/Base-Attacks.casp`: variáveis tipadas `var ATTACK_HitstunDuration int() = 0`, proração em milésimos (`ATTACK_ProrationDamage int() = 1000`, `def ATTACK_Proration_Scale int() = 1000`), prioridades (`ATK_HITSTUN_PRIORITY 50000`, `ATK_BLOCKSTUN_PRIORITY 40000`). `modules/attacks/CMAttacks.gd`: funções que calculam vantagem de frame automaticamente a partir da duração do golpe ("Sets an attack's frame advantage automatically on hit and block ... based on the total duration of the attack and the last use of the multihit flag"); cancels por situação com flags `ATTACKCANCEL_ON_HIT/_BLOCK/_WHIFF/_NEUTRAL` geradas como bitmask (`attackCancelConstants = ["_HIT", "_BLOCK", "_WHIFF", "_NEUTRAL"]`), lista de "done cancels" para impedir repetir o mesmo golpe na corrente.
- **Adaptar [Inferência]:** (a) autoria de vantagem de frame em vez de hitstun absoluto — designer escreve "+2 on block" e o sistema deriva blockstun; (b) cancels condicionados a hit/block/whiff como bitmask; (c) "done cancels list" para regras de chain. **Versão original:** implementar no nosso compilador de dados (offline), resultado vira inteiros no `MoveData` constexpr.
- **Limitação:** MPL-2.0 exige publicar alterações nos arquivos MPL; não copiar código para o core proprietário.

### 4.11 NoisyChain/Sakuga-Engine (MIT, C#/Godot 4, push 2026-08-01)

- **[Fato]** `Scripts/SakugaEngine/Globals/GlobalVariables.cs`: `TicksPerSecond = 60`, `SubSteps = 2` ("Discrete substeps for collisions"), `SimulationScale = 10000` ("World units multiplied by this"), `WallLimit = 75000`, `CeilingLimit = 120000`. `Collision/PhysicsBody.cs`: `Vector2I FixedPosition`, `FixedVelocity` (inteiros).
- **Input [Fato]** `Components/InputManager.cs`: histórico **run-length** — `InsertToHistory` só cria nova entrada quando o input muda; `struct InputRegistry { PlayerInputs rawInput; ushort duration; }`; `CheckMotionInputs` percorre as últimas N entradas e exige `duration <= motion.InputBuffer`; carga direcional via `hCharge/vCharge` (`short`). `Resources/MotionInputs.cs`: `ValidInputs[]` (várias formas aceitas para o mesmo comando), `InputBuffer = 8`, `DirectionalChargeLimit`, `ButtonChargeLimit`, `AbsoluteDirection`.
- **Serialização [Fato]:** estado via `MessagePackSerializer.Serialize(State)` (`Game/GameManager.cs` ~483, `Components/SakugaActor.cs` ~980).
- **Adaptar [Inferência]:** run-length é ótimo para comandos longos (360, cargas) com pouca memória e cabe num array fixo; múltiplas `ValidInputs` por comando = leniência explícita (ex.: 236 aceita 2365/23 6). Substeps de colisão inteiros previnem tunneling em golpes rápidos.

### 4.12 rcmagic/DemoFighterWithNetcode (MIT, Lua/LÖVE, push 2021) e rcmagic/ZigFightingGame (MIT, Zig, push 2026-01-09)

- **[Fato] DemoFighter** `game/Constants.lua`: `NET_INPUT_DELAY = 3` ("Should always be > 0"), `NET_ROLLBACK_MAX_FRAMES = 10`, `NET_DETECT_DESYNCS = true`, `ROLLBACK_TEST_ENABLED`/`ROLLBACK_TEST_FRAMES = 10` (teste local de rollback), e `SKIP_MATCH_INTRO` ("Must be the same value on both clients or a desync will occur"). `game/Network.lua`: `desyncCheckRate = 20`, histórico de "sync data" local/remoto. `game/main.lua`: "All rollbackable objects and systems will have a CopyState() method" (World, InputSystem, MatchSystem). `game/StateTimelines.lua`: cada estado tem `duration`, `images` com duração, `damageBoxes`/`attackBoxes` com `start`/`last` em frames.
- **[Fato] ZigFightingGame** `src/input.zig`: `MotionNames { QCF, QCB, DP, RDP }` e tabela `MotionInputs = [_][3]u32{ {2,3,6}, {2,1,4}, {6,2,3}, {4,2,1} }` (notação numpad); `CheckNumpadDirection(input, numpad_direction, facingLeft)` converte para relativo ao lado. `src/character_data.zig`: `Hitbox { top, left, bottom, right: i32 }`, `HitboxGroup { start_frame, duration }` + `isActiveOnFrame`, `HitProperty { damage, hitStun, guardStun, hitStop, knockBack, airKnockback, launchVelocityY }` todos `i32`. `src/GameState.zig`: dados estáticos (`GameData`: assets, mapas de ação) separados do estado da partida.
- **Adaptar [Inferência]:** a configuração de "teste de rollback forçado N frames" local deve existir no nosso build de debug desde o dia 1; dados de personagem 100% inteiros; tabela numpad + função de "espelhamento" por lado.

### 4.13 PsychoDrive-Dictator/psychodrive (sem licença, C++, push 2026-10-27) — só estudo

- **[Fato]** README: "a fighting game engine, currently able to express some of the scripts from SF6 for simulation and labbing purposes ... Uses dumps from MMDK". `fixed.hpp`: `class Fixed` com `int64_t data` e 16 bits fracionários; o construtor a partir de `float/double` **aborta** (`abort()`) se o valor não for exatamente representável (`diff > epsilon && !allowApproximate`). `main.hpp`: `template<typename T, size_t N, bool AssertOnOverflow=false> class FixedBuffer` (array fixo com `push_front` via `memmove`). `guy.hpp`: `Fixed posX, posY, velocityX, velocityY`; `int hitStop, hitStun`; `FixedBuffer<uint32_t, 150> inputBuffer` dentro de `struct defaultCopy`; `struct noCopy` com operador de atribuição vazio para dados que não entram no snapshot (logs).
- **[Inferência]** Mostra que a comunidade modela as posições do SF6 como valores com 16 bits fracionários exatos; a técnica "abortar se a conversão perde precisão" é excelente para nossas ferramentas de importação de dados. O padrão `noCopy` separa estado simulado de estado de debug.
- **Limitações:** sem licença e dependente de dados extraídos do SF6 → não copiar código nem dados.

### 4.14 kidagine/Darklings-FightingGame (sem licença, Unity/C#) — só estudo

- **[Fato]** `Assets/_Project/Scripts/CustomScripts/DemonicsFixed.cs`: `struct DemonFloat` com `long m_rawValue`, `FRACTIONAL_PLACES = 12` (`ONE = 1L << 12`), constantes `Pi = 12868`. O comentário diz "Represents a Q31.32 fixed-point number", mas o código usa 12 bits fracionários [Fato: inconsistência entre comentário e código].
- **[Inferência]** Lição: documentar o formato fixed-point por `static_assert`/teste, não por comentário.

### 4.15 D4RKONION/FAT (GPL-3.0, TypeScript) — esquema de frame data

- **[Fato]** `src/js/constants/framedata/SF6FrameData.json`: personagem → `{ moves: { normal: {...} }, stats }`. Campos por golpe: `startup, active, recovery, total, onHit, onBlock, onPC, onPP, hitstun, blockstun, hitstop, dmg, dmgScaling, atkLvl, DDoH, DDoB, DGain, DRoH, DRoB, SelfSoH, SelfSoB, OppSoH, OppSoB, xx (cancel), hcWinSpCa, numCmd, plnCmd, ezCmd, moveType, range, extraInfo`. `stats`: `fWalk, bWalk, fDash, bDash, fDashDist, nJump, fJump, throwRange, throwHurt, health, dRushDist*`...
- **[Inferência]** Bom checklist de quais métricas um jogo moderno expõe por golpe (vantagem em hit/block/counter/punish counter, custo/ganho de recursos, janelas de cancel). Não importar valores (GPL + dados da Capcom).

### 4.16 odabugs/kof-combo-hitboxes (licença permissiva estilo HPND, Lua) — taxonomia de caixas KOF

- **[Fato]** `lua/game/boxtypes_common.lua`: tipos `collision`, `counter`, `vulnerable`, `counterVuln`, `anywhereVuln`, `otgVuln`, `guard`, `attack`, `clash`, `projVuln`, `projAttack`, `throwable`, `throw`, `proximity`. `lua/game/pcsx2/kof_xi/types.lua` documenta offsets de memória (`hitbox grabBox; // +33Ch`, `word guard; // +00Ah: Guard gauge`). Suporta KOF 98UM, 2002UM, XI, NeoWave, XIII e outros.
- **[Inferência]** Taxonomia útil para nosso enum de caixas: além de hit/hurt/push, prever `proximity` (ativa guarda), `throwable`/`throw`, `projectile vuln/attack` separados, `clash`, `otg vuln`. (Os offsets de memória não nos interessam.)

### 4.17 SF6 (estudo de mods, sem reutilização): WistfulHopes/SF6Mods e WistfulHopes/sf6_sim

- **[Fato]** `SF6Mods/disp_hitboxes.lua` (REFramework) acessa `sdk.find_type_definition("gBattle")` e categoriza caixas pelos campos (`HitPos`, `Attr`, `HitNo`, `KeyData`) com toggles para hitboxes, hurtboxes, pushboxes, throwboxes, throw-hurtboxes, proximity, "unique" e clash.
- **[Fato]** `sf6_sim/src/app/simulator.rs`: structs `PushCollisionKey { condition, attribute, pushbox }`, `DamageCollisionKey { condition, collision_type, immune, extend, level, type_flag, boxes }`, `AttackCollisionKey { condition, collision_type, hit_id, guard_bit, kind_flag, hit_offset, boxes }`, `Trigger { action, condition_flag }`, `ActionInfo { first_active_frame, recovery_frame, end_frame, loop_count }`; README: "can currently simulate boxes, cancel lists, and movement". A pasta `assets/` contém arquivos `NNN.fchar.17` (dados do jogo) — **não usar**.
- **[Inferência]** O SF6 organiza ações como linha do tempo de "keys" (caixas, triggers de cancel, movimento) com condição por key; cancels são "triggers" com flags de condição. É o mesmo modelo de "timeline de eventos por frame" que devemos adotar com dados originais.

### 4.18 Outros lidos superficialmente

- **gregorik/Rollback-Core (MIT, UE5/C++20):** README: "mark anything you want rolled-back with `UPROPERTY(SaveGame)`, call `URollbackManager::AdvanceFrame()` ... `RollbackToFrame(N)` resimulates"; ~10 arquivos. Útil como referência de integração com motor, não para core puro.
- **fraillt/bitsery (MIT, C++):** README: "No code generation required", "Configurable endianness support", "No macros", "Configurable runtime error checking on deserialization". [Inferência] bom para **replays/rede/arquivos de dados**, não para snapshot de rollback (lá `memcpy` de POD é mais rápido).
- **johnmcfarlane/cnl (BSL-1.0):** fixed-point composável; mais pesado que fpm; útil se quisermos overflow checado em debug.
- **Delta3-Studio/Backdash (MIT, C#):** SDK de rollback .NET documentado; referência de API.
- **Pyxus/fray (MIT, GDScript):** `input_requirement.gd` usa `min_time_held`/`max_delay` em **milissegundos**; `buffered_input_advancer.gd` tem `max_buffer_time: float` em **segundos** e `is_negative_edge_enabled` em `sequence_branch.gd`. [Inferência] tempo real ≠ frames: inadequado para rollback; mas o conceito de "advancer pausável para controlar janelas de cancel" é útil.

## 5. Riscos legais

**Regra do projeto:** estudamos ideias e arquitetura; **nunca** usamos assets, dados extraídos, código sem licença ou copyleft no core, e **nunca** contornamos DRM/anticheat.

1. **Mods/RE de SF6 (REFramework, MMDK, SF6Mods, sf6_sim, psychodrive, SF6 Training Mode Plus):** funcionam lendo/escrevendo memória do processo do jogo ou dados extraídos dos pacotes. [Fato] `sf6_sim` inclui arquivos `.fchar` do jogo em `assets/`; `psychodrive` depende de dumps do MMDK. → **Só leitura de código/documentação para entender conceitos**. Não baixar/usar dumps, não abrir `.fchar`, não reproduzir valores de frame data do SF6 em nosso jogo. Usar mods em partidas online pode violar os Termos de Serviço da Capcom (e eventuais sistemas anticheat) — **não testamos nada em jogo online**.
2. **KOF XV (UE4) — FModel, CUE4Parse, UE4SS, `473671866/hitbox`:** extração de `.pak`/injeção de DLL. Mesmo tratamento: só estudar a técnica geral; não extrair assets da SNK; não injetar em jogo online.
3. **Datasets de frame data (FAT, sf6-move-data, kofxv_framebot, extratores do site oficial):** números descrevem obras protegidas e alguns repositórios são GPL. Usamos apenas os **nomes de campos** como checklist de métricas; nossos valores serão próprios.
4. **Decompilações/recompilações (doldecomp/melee, 3s-decomp, sfIII3-cps3-decomp, cps2recomp, KOFXIVDecomp, recomps de Tekken/VF/Def Jam, LethalLeague):** código derivado de jogos comerciais, várias com AGPL/GPL ou sem licença. **Não copiar nada**; no máximo leitura histórica de alto nível. Recomendação forte: quem escreve o nosso core **não deve** ler essas decomps em detalhe (evita alegação de cópia — prática de "clean room").
5. **Copyleft:** GPL-2/3, AGPL, LGPL e MPL (Castagne) → não incorporar ao core proprietário. LGPL/MPL só se mantidos como módulo separado e cumprindo obrigações; mais simples: não usar.
6. **Sem licença (`none`):** todos os direitos reservados. Ex.: psychodrive, Darklings, SF6Mods, sf6_sim, hitbox-editor (coelhucas), Hantei-chan, TeamFightingICE. Só estudo.
7. **FBNeo:** licença própria não comercial (`src/license.txt`: "You may not sell ... or otherwise seek to gain monetary profit") → incompatível com produto comercial.
8. **Repositórios-isca (categoria R):** padrões "Hack 2026", "Money Unlocker", "Drive Ticket Generator", "Crash Fix", "Safe Setup Guide" com HTML/sem código. Tratar como potencial malware; **não baixar nem executar**. Também evitar baixar "mod managers" de forks com nome "2026" em vez da fonte oficial.
9. **Marcas e personagens:** nada de nomes, golpes icônicos com nome/visual idêntico, sons ou UI que imitem SF/KOF/GG (ex.: `dbfz-css` recria UI do DBFZ — não usar). Mecânicas genéricas (rollback, 236P, cancel, drive/meter) não são protegidas por si, mas a expressão concreta (sprites, nomes, valores exatos copiados em massa) pode ser.

## 6. Recomendações para o nosso core C++20

### 6.1 Reutilizar (licença compatível com produto comercial)

| Necessidade | Opção | Licença | Por quê |
|---|---|---|---|
| Rollback P2P | **HeatXD/GekkoNet** (1ª opção) | BSD-2-Clause | C++20, API C, polling de eventos, `state_size` fixo, stress session, replay com checksums, spectator, transporte plugável |
| Rollback (alternativa/benchmark) | pond3r/ggpo | MIT | Padrão de fato; boa base para comparar comportamento |
| Fixed-point | MikeLankamp/fpm (ou tipo próprio inspirado nele) | MIT | Header-only, `fixed<int32,int64,16>`, math constexpr |
| Trig/sqrt inteiros (referência) | PetteriAimonen/libfixmath | MIT | Algoritmos e LUT de seno testados |
| Serialização de replays/dados | fraillt/bitsery | MIT | Binário compacto, endianness configurável, sem macros |
| Transporte UDP | ValveSoftware/GameNetworkingSockets ou lsalzman/enet | BSD-3 / MIT | Se o adaptador de rede do GekkoNet precisar de base |
| Animação esquelética (apresentação) | guillaumeblanc/ozz-animation | MIT | Runtime C++ leve; fica **fora** do core determinístico |
| ECS (opcional, apresentação/ferramentas) | skypjack/entt | MIT | Não recomendado dentro do snapshot de rollback (heap) |
| Firmware/SOCD (referência) | OpenStickCommunity/GP2040-CE | MIT | [Inferência, código não lido] referência comunitária de hardware/SOCD |
| Script de hitbox no Aseprite | kaiiboraka/Aseprite_Hitbox_Editor | CC0-1.0 | Ponto de partida para pipeline de autoria 2D |

### 6.2 Só estudar (ideias para implementar de forma original)

1. **Contrato do core** (GGPO/GGRS/GekkoNet): `save → bytes`, `load ← bytes`, `advance(inputs, status)`; nunca callbacks reentrantes de dentro da simulação. Implementar `SimRequest` como `std::variant` (estilo GGRS).
2. **Snapshot por POD** (NightSky `ObjSync..ObjSyncEnd`, GGPO VectorWar `memcpy`, MicahMartin `CharStateObj`): um único `struct MatchState` trivialmente copiável, arrays de tamanho fixo (players, projéteis, efeitos de hit), `static_assert(std::is_trivially_copyable_v<MatchState>)` e `static_assert(sizeof(MatchState) <= N)`. Dados estáticos (MoveData, tabelas) **fora** do snapshot, referenciados por índice (como ZigFightingGame separa `GameData`).
3. **Separar sim × debug** (psychodrive `noCopy`): logs/telemetria fora do estado.
4. **Checksum e sync test desde o dia 1** (GGPO synctest, GGRS `check_distance`, DemoFighter `ROLLBACK_TEST_FRAMES`, GekkoNet stress session): CI roda partidas gravadas com rollback forçado de 1..8 frames e compara hash por frame.
5. **Input**: bitmask `uint16_t` por frame (MicahMartin) + contadores assinados held/released por botão (Ikemen) + histórico run-length de direções (Sakuga) num `std::array` circular dentro do estado do jogador. Buffer avança inclusive em hitstop **só se** o design quiser (NightSky tem `bStopped`; Ikemen tem `buffer_hitpause`) — decidir explicitamente.
6. **Parser de comandos**: autoria em notação numpad/MUGEN (`236P`, `[4]6P` carga, `~` soltar, `/` segurar, `$` direção inclusiva, `>` estrito), compilada **offline** para tabela constexpr; por passo, `lenience`/`max_step_time` (NightSky `Lenience = 3`; Ikemen `maxsteptime`); métodos Normal/Strict/Once (NightSky); várias formas válidas por comando (Sakuga `ValidInputs[]`); SOCD configurável (Ikemen/GP2040).
7. **Dados de golpe como timeline de keys por frame** (sf6_sim keys, MicahMartin `collision_boxes start/end`, ZigFightingGame `HitboxGroup`, DemoFighter `StateTimelines`): caixas tipadas (hit, hurt, push, throw, throwable, proximity, projectile-vuln, clash, otg — taxonomia KOF de odabugs), propriedades de hit no próprio box (dano, hitstun, blockstun, hitstop, push, launch), triggers de cancel com condição (hit/block/whiff bitmask — Castagne) e janela de frames.
8. **Autoria por vantagem** (Castagne): designer informa `+N on block / +M on hit`; ferramenta deriva blockstun/hitstun a partir de `total - last_active`. Proração em milésimos (inteiro).
9. **Física inteira com substeps** (Sakuga `SimulationScale = 10000`, `SubSteps = 2`; NightSky `int32 PosX` + LUT de seno): escolher unidade (ex.: 1/65536 px como psychodrive/fpm Q16.16) e evitar trig em runtime.
10. **Ferramentas de importação que abortam em perda de precisão** (psychodrive `Fixed(float)` + `abort()`).
11. **Training mode como requisito de arquitetura** (3rd_training_lua, fbneo-training-mode, UltimateTrainingModpack, gbvsr-frame-meter): gravação/playback de dummy, frame meter, display de hitbox e de input — tudo lendo o `MatchState`, sem lógica própria (camada fina).
12. **Replays = inputs + seed + versão** (GekkoNet recording, rev2-replays, Slippi): checksum periódico no replay para detectar quebra de determinismo entre versões.

### 6.3 Não fazer

- Não usar `float`/`double` no core (Ikemen usa `float32` e depende do mesmo binário; Fray usa tempo real em ms/s).
- Não usar `std::function`, `std::vector`, `std::map` ou ponteiros crus dentro do estado simulado (MicahMartin usa `boost::circular_buffer`/`std::vector` por frame — difícil de snapshotar).
- Não depender de `offsetof` em classes não standard-layout (truque do NightSky é específico do UE).
- Não copiar código de repositórios sem licença/GPL/AGPL/MPL, nem dados de SF6/KOF.

## 7. Top 10 achados

1. **[Fato]** GekkoNet é um SDK de rollback em **C++20** com licença **BSD-2**, API C, `state_size` fixo, stress/replay/spectator e transporte plugável — melhor encaixe para nosso core.
2. **[Fato]** GGPO: predição = "repetir último input"; rollback = `LoadFrame(first_incorrect)` + re-simular; ring buffer de `MAX_PREDICTION_FRAMES + 2` estados; synctest compara checksums re-simulando a cada `check_distance`.
3. **[Fato]** GGRS troca callbacks por lista de requests e expõe `InputStatus {Confirmed, Predicted, Disconnected}` e desync detection por intervalo; defaults 8 frames de predição, check distance 2, 60 fps.
4. **[Fato]** NightSky salva o objeto inteiro com `memcpy` entre marcadores `ObjSync`/`ObjSyncEnd` + `static_assert`; usa coordenadas `int32` e LUT de seno `gSinTable[900]`.
5. **[Fato]** Input buffers observados: NightSky 32 frames com `Lenience = 3` por passo e métodos Strict/Once/Negative; Ikemen contador assinado held(+)/released(−) por tecla e notação MUGEN; Sakuga histórico run-length com `InputBuffer = 8`.
6. **[Fato]** SOCD é tratado em software com modos configuráveis (Ikemen: ambos/última/absoluta/primeira/neutro).
7. **[Fato]** Hitboxes com `start/end` em frames e propriedades de hit embutidas (MicahMartin: `hitstun`, `block_stun`, `hitstop`, `push`, `group`); taxonomia KOF tem 14 tipos de caixa (além de `dummy`) incluindo `proximity`, `throwable`, `otgVuln`, `clash`.
8. **[Fato]** Castagne deriva hitstun/blockstun a partir de vantagem de frame desejada e usa cancels condicionados a hit/block/whiff como bitmask; proração em milésimos.
9. **[Fato]** O ecossistema SF6 é REFramework/MMDK; simuladores comunitários (sf6_sim, psychodrive) modelam ações como timeline de "keys" com condições e posições Q16 — confirmam o modelo que devemos adotar, mas dependem de dados da Capcom (só estudo).
10. **[Fato]** KOF XV quase não tem ferramentas públicas (1 plugin de hitbox, 1 bot); há várias iscas "hack/trainer 2026" para SF6/GGST/Tekken — categoria R.
