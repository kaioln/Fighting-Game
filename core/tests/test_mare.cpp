#include "helpers.h"

using namespace fg;
using fgtest::feed_p1;
using fgtest::fight;
using fgtest::move_index;
using fgtest::place;
using fgtest::raw_dir;
using fgtest::run;

TEST("Maré: andar para a frente puxa a Maré para o seu lado") {
  MatchState s = fight("nadja", "otto");
  place(s, 40000);
  run(s, 60, raw_dir(6), 0);
  CHECK(s.mare > 0);
  CHECK(mare_favor(s, 0) == -mare_favor(s, 1));
}

TEST("Maré: recuar por muito tempo afasta a Maré") {
  MatchState s = fight("nadja", "otto");
  place(s, 20000);
  run(s, 90, 0, btn::Right);  // P2 recua 90 frames: ainda dentro da tolerância
  CHECK_EQ(s.mare, 0);
  run(s, 30, 0, btn::Right);
  CHECK(s.mare > 0);  // P2 perdeu Maré, logo P1 ganhou
}

TEST("Maré: Arrebentação exige Maré alta e só sai como cancelamento de normal que fez contato") {
  const GameData& d = fgtest::game_data();
  // Sem Maré suficiente: nada acontece.
  MatchState s = fight("nadja", "otto");
  place(s, 6000);
  s.mare = d.rules.mare.threshold - 100;
  run(s, 1, btn::MP, 0);
  run(s, 6, 0, 0);  // conecta no frame 6
  run(s, 12, btn::MP | btn::MK, 0);
  CHECK(s.f[0].move != move_index("nadja", "arrebentacao"));

  // Com Maré alta: cancela e consome a Maré.
  MatchState t = fight("nadja", "otto");
  place(t, 6000);
  t.mare = d.rules.mare.threshold + 100;
  run(t, 1, btn::MP, 0);
  run(t, 6, 0, 0);
  bool started = false;
  for (int i = 0; i < 12 && !started; ++i) started |= fgtest::step_has(t, btn::MP | btn::MK, 0, EventType::Arrebentacao);
  CHECK(started);
  CHECK(t.f[0].move == move_index("nadja", "arrebentacao"));
  CHECK(t.mare < d.rules.mare.threshold);

  // Do neutro, não sai.
  MatchState u = fight("nadja", "otto");
  place(u, 20000);
  u.mare = d.rules.mare.limit;
  run(u, 1, btn::MP | btn::MK, 0);
  CHECK(u.f[0].move != move_index("nadja", "arrebentacao"));
}

TEST("Maré: Arrebentação pode ser cancelada em normal (extensão de pressão)") {
  const GameData& d = fgtest::game_data();
  MatchState t = fight("nadja", "otto");
  place(t, 6000);
  t.mare = d.rules.mare.limit;
  run(t, 1, btn::MP, 0);
  run(t, 6, 0, 0);
  bool started = false;
  for (int i = 0; i < 12 && !started; ++i) started |= fgtest::step_has(t, btn::MP | btn::MK, 0, EventType::Arrebentacao);
  REQUIRE(started);
  run(t, 6, 0, 0);
  run(t, 1, btn::LP, 0);
  CHECK(t.f[0].move == move_index("nadja", "5LP"));
}

TEST("Maré: Contracorrente sai do blockstun quando a Maré está contra você") {
  const GameData& d = fgtest::game_data();
  MatchState s = fight("otto", "nadja");
  place(s, 6000);
  s.mare = -(d.rules.mare.threshold + 200);  // contra o P1 (Otto)
  // Nadja ataca, Otto bloqueia (P1 olha para a direita: trás = esquerda).
  run(s, 1, btn::Left, btn::MP);
  bool blocked = false;
  for (int i = 0; i < 10 && !blocked; ++i) blocked |= fgtest::step_has(s, btn::Left, 0, EventType::Block);
  REQUIRE(blocked);
  REQUIRE(s.f[0].act == Act::Blockstun);
  // Pressionado durante o hitstop do bloqueio: sai assim que o hitstop termina (buffer estendido).
  bool contra = false;
  for (int i = 0; i < 16 && !contra; ++i) contra |= fgtest::step_has(s, btn::Left | btn::MP | btn::MK, 0, EventType::Contracorrente);
  CHECK(contra);
  CHECK_EQ(s.mare, 0);
}

TEST("Maré: Contracorrente indisponível sem Maré contra") {
  MatchState s = fight("otto", "nadja");
  place(s, 6000);
  run(s, 1, btn::Left, btn::MP);
  bool blocked = false;
  for (int i = 0; i < 10 && !blocked; ++i) blocked |= fgtest::step_has(s, btn::Left, 0, EventType::Block);
  REQUIRE(blocked);
  bool contra = false;
  for (int i = 0; i < 16; ++i) contra |= fgtest::step_has(s, btn::Left | btn::MP | btn::MK, 0, EventType::Contracorrente);
  CHECK(!contra);
}

TEST("Maré: limite nunca é ultrapassado") {
  const GameData& d = fgtest::game_data();
  MatchState s = fight("nadja", "otto");
  place(s, 40000);
  s.mare = d.rules.mare.limit - 1;
  run(s, 200, raw_dir(6), 0);
  CHECK_EQ(s.mare, d.rules.mare.limit);
}
