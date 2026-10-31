# Fighting Game (codinome provisório: **Projeto Estaleiro**)

Jogo de luta 2.5D original, nativo para PC (Windows), com personagens e cenários 3D autorais,
simulação determinística a 60 Hz e arquitetura preparada para rollback desde o núcleo.

Referências de excelência: Street Fighter 6 (Capcom) e The King of Fighters XV (SNK).
Nada deles é copiado: estudamos princípios e construímos implementações e conteúdo próprios.

## Estado atual

- **Fase 1 — Pesquisa e engenharia:** concluída e aprovada (documentos em `docs/`).
- **Fase 2 — Núcleo técnico:** em andamento. O núcleo determinístico `fgcore` (C++20) já roda
  combate, comandos, Maré, rounds, rollback e replays, com 70 testes. Os dois lutadores (Nadja e Otto)
  existem como dados. Ainda não há camada visual: nenhuma arte provisória com primitivas será usada.
  Detalhes em [`docs/10-nucleo-tecnico.md`](docs/10-nucleo-tecnico.md).

## Compilar e testar

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/tools/fgsim/fgsim framedata data
```

## Documentação

Ver o índice em [`docs/README.md`](docs/README.md).
