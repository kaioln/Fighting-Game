#include "fg/command.h"
#include "helpers.h"

using namespace fg;
using fgtest::feed_p1;
using fgtest::fight;
using fgtest::move_index;
using fgtest::place;
using fgtest::raw_dir;
using fgtest::run;

namespace {
bool p1_in_move(const MatchState& s, const std::string& ch, const std::string& mv) {
  return s.f[0].act == Act::Move && s.f[0].move == move_index(ch, mv);
}
}  // namespace

TEST("comando: 236+P executa Coice de Moto") {
  MatchState s = fight("nadja", "otto");
  place(s, 30000);
  feed_p1(s, "236", btn::LP);
  CHECK(p1_in_move(s, "nadja", "236P"));
}

TEST("comando: movimento lento demais vira golpe normal") {
  MatchState s = fight("nadja", "otto");
  place(s, 30000);
  feed_p1(s, "2", 0);
  run(s, 12, 0);  // 12 frames neutros: estoura a janela de 11
  feed_p1(s, "36", btn::LP);
  CHECK(!p1_in_move(s, "nadja", "236P"));
  CHECK(p1_in_move(s, "nadja", "5LP"));
}

TEST("comando: atalho 323 aceito para 623") {
  MatchState s = fight("nadja", "otto");
  place(s, 30000);
  feed_p1(s, "323", btn::HP);
  CHECK(p1_in_move(s, "nadja", "623P"));
}

TEST("comando: 6236+P prioriza 623 sobre 236") {
  MatchState s = fight("nadja", "otto");
  place(s, 30000);
  feed_p1(s, "6236", btn::MP);
  CHECK(p1_in_move(s, "nadja", "623P"));
}

TEST("comando: super exige medidor; sem medidor sai o especial") {
  MatchState s = fight("nadja", "otto");
  place(s, 30000);
  feed_p1(s, "236236", btn::HK);
  CHECK(p1_in_move(s, "nadja", "236K"));

  MatchState t = fight("nadja", "otto");
  place(t, 30000);
  t.f[0].meter = 2000;
  feed_p1(t, "236236", btn::HK);
  CHECK(p1_in_move(t, "nadja", "236236K"));
  CHECK_EQ(t.f[0].meter, 0);
}

TEST("comando: reforçado com dois socos consome 1 barra") {
  MatchState s = fight("nadja", "otto");
  place(s, 30000);
  s.f[0].meter = 1500;
  feed_p1(s, "236", static_cast<InputBits>(btn::LP | btn::MP));
  CHECK(p1_in_move(s, "nadja", "236PP"));
  CHECK_EQ(s.f[0].meter, 500);
}

TEST("comando: direções são relativas ao lado (P1 olhando para a esquerda)") {
  MatchState s = fight("nadja", "otto");
  place(s, 30000);
  // Troca os lados: P1 à direita.
  s.f[0].x = 15000;
  s.f[1].x = -15000;
  run(s, 1);  // atualiza o lado
  REQUIRE(s.f[0].facing == -1);
  // 236 para quem olha para a esquerda: baixo, baixo-esquerda, esquerda.
  run(s, 1, btn::Down);
  run(s, 1, static_cast<InputBits>(btn::Down | btn::Left));
  run(s, 1, static_cast<InputBits>(btn::Left | btn::LP));
  CHECK(p1_in_move(s, "nadja", "236P"));
}

TEST("comando: buffer de 4 frames durante a recuperação") {
  // 5MP total 20 frames. Pressionar LP 3 frames antes do fim executa 5LP no primeiro frame livre.
  MatchState s = fight("nadja", "otto");
  place(s, 40000);
  feed_p1(s, "5", btn::MP);
  REQUIRE(p1_in_move(s, "nadja", "5MP"));
  run(s, 16);                 // move_frame = 17
  run(s, 1, btn::LP);         // frame 18: pressiona
  run(s, 1, btn::LP);         // 19
  run(s, 1, btn::LP);         // 20 (último)
  run(s, 1, btn::LP);         // livre: executa
  CHECK(p1_in_move(s, "nadja", "5LP"));
  CHECK_EQ(s.f[0].move_frame, 1);

  MatchState t = fight("nadja", "otto");
  place(t, 40000);
  feed_p1(t, "5", btn::MP);
  run(t, 12);
  run(t, 1, btn::LP);  // pressionado 7 frames antes de ficar livre: expira
  run(t, 6, btn::LP);  // segurar não conta como nova pressão
  run(t, 1, btn::LP);
  CHECK(!p1_in_move(t, "nadja", "5LP"));
}

TEST("comando: 360 do Otto sai mesmo terminando em cima (cancela o pré-salto)") {
  MatchState s = fight("otto", "nadja");
  place(s, 30000);
  feed_p1(s, "6321478", btn::LP);
  CHECK(p1_in_move(s, "otto", "360P"));
}

TEST("comando: esquema Simplificado faz especial com SP + direção e aplica penalidade") {
  MatchSetup setup;
  setup.character[0] = fgtest::char_index("nadja");
  setup.character[1] = fgtest::char_index("otto");
  setup.scheme[0] = ControlScheme::Simplified;
  setup.options = option::SkipIntro | option::NoTimer;
  MatchState s;
  init_match(s, fgtest::game_data(), setup);
  place(s, 30000);
  feed_p1(s, "6", btn::SP);
  CHECK(p1_in_move(s, "nadja", "236P"));
  CHECK_EQ(s.f[0].damage_permil_override, 800);
}

TEST("comando: esquema Clássico ignora comandos do Simplificado") {
  MatchState s = fight("nadja", "otto");
  place(s, 30000);
  feed_p1(s, "6", btn::SP);
  CHECK(s.f[0].act != Act::Move);
}

TEST("input: SOCD esquerda+direita = neutro; cima+baixo = cima") {
  CHECK_EQ(numpad_direction(static_cast<InputBits>(btn::Left | btn::Right), 1), 5);
  CHECK_EQ(numpad_direction(static_cast<InputBits>(btn::Up | btn::Down), 1), 8);
  CHECK_EQ(numpad_direction(btn::Left, -1), 6);
  CHECK_EQ(numpad_direction(static_cast<InputBits>(btn::Down | btn::Right), 1), 3);
}

TEST("movimento: 66 faz dash no Otto e corrida na Nadja") {
  MatchState s = fight("otto", "nadja");
  place(s, 40000);
  feed_p1(s, "656", 0);
  CHECK(p1_in_move(s, "otto", "dash"));

  MatchState t = fight("nadja", "otto");
  place(t, 40000);
  feed_p1(t, "656", 0);
  CHECK(t.f[0].act == Act::Run);
  const i32 x0 = t.f[0].x;
  run(t, 5, raw_dir(6));
  CHECK(t.f[0].x - x0 == 5 * 1150);
}

TEST("movimento: toque rápido para cima = hop; segurar = salto") {
  MatchState s = fight("nadja", "otto");
  place(s, 40000);
  run(s, 2, raw_dir(8));
  run(s, 3, 0);
  CHECK(s.f[0].act == Act::Air);
  CHECK(s.f[0].jump_kind == JumpKind::Hop);

  MatchState t = fight("nadja", "otto");
  place(t, 40000);
  run(t, 5, raw_dir(8));
  CHECK(t.f[0].act == Act::Air);
  CHECK(t.f[0].jump_kind == JumpKind::Normal);
}

TEST("comando: negative edge (soltar o botão) quando habilitado") {
  CommandDef c;
  c.trigger = TriggerButtons::AnyPunch;
  c.negative_edge = true;
  InputHistory h;
  h.push(5, btn::LP);
  h.push(5, 0);  // soltou
  CHECK(trigger_matches(h, c, 0));
  c.negative_edge = false;
  CHECK(!trigger_matches(h, c, 0));
}

TEST("comando: compile_motion valida sintaxe") {
  std::vector<u16> t;
  std::string err;
  CHECK(compile_motion("2 3 6", t, err));
  CHECK(t.size() == 3);
  CHECK(compile_motion("[123] 6", t, err));
  CHECK(t.size() == 2);
  CHECK(!compile_motion("2 x 6", t, err));
  CHECK(!compile_motion("[12", t, err));
}
