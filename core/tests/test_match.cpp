#include "helpers.h"

using namespace fg;
using fgtest::fight;
using fgtest::place;
using fgtest::run;

TEST("partida: introdução de 90 frames e então 'Lutem'") {
  const GameData& d = fgtest::game_data();
  MatchSetup setup;
  MatchState s;
  init_match(s, d, setup);
  CHECK(s.phase == Phase::Intro);
  bool fight_ev = false;
  for (int i = 0; i < d.rules.round_intro_frames; ++i) fight_ev |= fgtest::step_has(s, 0, 0, EventType::Fight);
  CHECK(fight_ev);
  CHECK(s.phase == Phase::Fight);
}

TEST("partida: inputs durante a introdução são ignorados") {
  const GameData& d = fgtest::game_data();
  MatchSetup setup;
  MatchState s;
  init_match(s, d, setup);
  run(s, d.rules.round_intro_frames - 1, btn::LP, 0);
  run(s, 1, btn::LP, 0);  // segurando, sem nova pressão
  run(s, 1, btn::LP, 0);
  CHECK(s.f[0].act != Act::Move);
}

TEST("partida: KO encerra o round, conta vitória e reinicia posições e vida") {
  const GameData& d = fgtest::game_data();
  MatchState s = fight("nadja", "otto");
  place(s, 6000);
  s.f[1].health = 1;
  bool ko = false;
  for (int i = 0; i < 10 && !ko; ++i) ko = fgtest::step_has(s, i == 0 ? btn::MP : 0, 0, EventType::KO);
  CHECK(ko);
  run(s, 1);
  CHECK(s.phase == Phase::RoundEnd);
  CHECK_EQ(s.wins[0], 1);
  run(s, d.rules.round_end_frames + 1);
  CHECK(s.phase == Phase::Intro);
  CHECK_EQ(s.round, 2);
  CHECK_EQ(s.f[1].health, d.characters[1].health);
  CHECK_EQ(s.f[0].x, -d.rules.start_distance / 2);
}

TEST("partida: duas vitórias encerram a partida") {
  const GameData& d = fgtest::game_data();
  MatchState s = fight("nadja", "otto");
  s.wins[0] = 1;
  place(s, 6000);
  s.f[1].health = 1;
  for (int i = 0; i < 10; ++i) run(s, 1, i == 0 ? btn::MP : 0, 0);
  run(s, d.rules.round_end_frames + 2);
  CHECK(s.phase == Phase::MatchEnd);
  CHECK_EQ(s.match_winner, 0);
}

TEST("partida: fim do tempo decide pela porcentagem de vida") {
  MatchSetup setup;
  setup.options = option::SkipIntro;
  MatchState s;
  const GameData& d = fgtest::game_data();
  init_match(s, d, setup);
  s.timer_frames = 3;
  s.f[0].health = d.characters[0].health / 2;
  bool over = false;
  for (int i = 0; i < 3; ++i) over |= fgtest::step_has(s, 0, 0, EventType::TimeOver);
  CHECK(over);
  CHECK(s.phase == Phase::RoundEnd);
  CHECK_EQ(s.round_winner, 1);
}

TEST("super: congelamento cinematográfico para os dois lutadores") {
  MatchState s = fight("otto", "nadja");
  place(s, 30000);
  s.f[0].meter = 2000;
  const i32 x1 = s.f[1].x;
  fgtest::feed_p1(s, "23623", 0);
  bool freeze = fgtest::step_has(s, fgtest::raw_dir(6) | btn::LP, fgtest::raw_dir(4), EventType::SuperFreeze);
  CHECK(freeze);
  run(s, 39, 0, btn::Right);  // P2 tenta andar: congelado
  CHECK_EQ(s.f[1].x, x1);
  CHECK(s.freeze == 1);
}

TEST("medidor: ganho ao acertar e ao ser acertado; limite máximo") {
  const GameData& d = fgtest::game_data();
  MatchState s = fight("nadja", "otto");
  place(s, 6000);
  for (int i = 0; i < 10; ++i) run(s, 1, i == 0 ? btn::MP : 0, 0);
  CHECK_EQ(s.f[0].meter, 40);
  CHECK_EQ(s.f[1].meter, 12);  // 30% do ganho do atacante
  s.f[0].meter = max_meter(d.rules) - 1;
  place(s, 6000);
  run(s, 30);
  for (int i = 0; i < 10; ++i) run(s, 1, i == 0 ? btn::MP : 0, 0);
  CHECK_EQ(s.f[0].meter, max_meter(d.rules));
}
