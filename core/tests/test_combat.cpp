#include "helpers.h"

using namespace fg;
using fgtest::feed_p1;
using fgtest::fight;
using fgtest::move_index;
using fgtest::place;
using fgtest::raw_dir;
using fgtest::run;

namespace {
// P2 olha para a esquerda: "trás" é a direita.
constexpr InputBits kP2Back = btn::Right;
constexpr InputBits kP2DownBack = btn::Right | btn::Down;

// Executa até um evento de contato do P1 e retorna o tipo (ou RoundStart se nada ocorrer).
EventType until_contact(MatchState& s, InputBits p1, InputBits p2, int max_frames, Event* out = nullptr) {
  for (int i = 0; i < max_frames; ++i) {
    StepEvents ev;
    FrameInput in;
    in.p[0] = i == 0 ? p1 : static_cast<InputBits>(p1 & btn::Directions);
    in.p[1] = p2;
    step(s, fgtest::game_data(), in, &ev);
    for (int k = 0; k < ev.count; ++k) {
      const Event& e = ev.items[k];
      switch (e.type) {
        case EventType::Hit:
        case EventType::CounterHit:
        case EventType::PunishCounter:
        case EventType::Block:
        case EventType::Armor:
        case EventType::ThrowConnect:
          if (out) *out = e;
          return e.type;
        default:
          break;
      }
    }
  }
  return EventType::RoundStart;
}
}  // namespace

TEST("defesa: golpe baixo exige defesa agachada") {
  MatchState s = fight("nadja", "otto");
  place(s, 6000);
  CHECK(until_contact(s, raw_dir(2) | btn::LK, kP2Back, 20) == EventType::Hit);
  MatchState t = fight("nadja", "otto");
  place(t, 6000);
  CHECK(until_contact(t, raw_dir(2) | btn::LK, kP2DownBack, 20) == EventType::Block);
}

TEST("defesa: overhead exige defesa em pé") {
  MatchState s = fight("nadja", "otto");
  place(s, 6000);
  CHECK(until_contact(s, raw_dir(6) | btn::MP, kP2DownBack, 30) == EventType::Hit);
  MatchState t = fight("nadja", "otto");
  place(t, 6000);
  CHECK(until_contact(t, raw_dir(6) | btn::MP, kP2Back, 30) == EventType::Block);
}

TEST("defesa: bloqueio aplica blockstun e empurra o defensor") {
  MatchState s = fight("nadja", "otto");
  place(s, 6000);
  const i32 x0 = s.f[1].x;
  CHECK(until_contact(s, btn::MP, kP2Back, 20) == EventType::Block);
  CHECK(s.f[1].act == Act::Blockstun);
  run(s, 30, 0, kP2Back);
  CHECK(s.f[1].x > x0);
}

TEST("contra-ataque: acertar durante o startup do oponente é counter hit") {
  MatchState s = fight("nadja", "otto");
  place(s, 6000);
  run(s, 1, 0, btn::MK);  // Otto inicia 5MK (startup 9)
  CHECK(until_contact(s, btn::LP, 0, 10) == EventType::CounterHit);
  CHECK(s.f[1].act == Act::Hitstun);
  CHECK(s.f[1].timer == 12 + 2);  // hitstun do 5LP + bônus de counter hit
}

TEST("contra-ataque: acertar na recuperação é punish counter e move a Maré") {
  MatchState s = fight("nadja", "otto");
  place(s, 14400);  // 12000 + alargamento das caixas de corpo (2400)
  run(s, 1, 0, raw_dir(1) | btn::HK);  // Otto 2HK (rasteira) erra à distância
  run(s, 14, 0, 0);                    // entra na recuperação
  const i32 mare0 = s.mare;
  Event e;
  CHECK(until_contact(s, raw_dir(2) | btn::MK, 0, 12, &e) == EventType::PunishCounter);
  CHECK(s.mare - mare0 >= 200);
}

TEST("armadura: 5HP do Otto absorve um golpe e continua") {
  MatchState s = fight("otto", "nadja");
  place(s, 6000);
  run(s, 1, btn::HP, 0);  // Otto 5HP, armadura nos frames 5-12
  run(s, 4, 0, 0);
  // Nadja usa 5LP (startup 4) — atinge durante a armadura.
  bool armor = false;
  for (int i = 0; i < 6 && !armor; ++i) {
    StepEvents ev;
    FrameInput in;
    in.p[1] = i == 0 ? btn::LP : 0;
    step(s, fgtest::game_data(), in, &ev);
    for (int k = 0; k < ev.count; ++k) armor |= ev.items[k].type == EventType::Armor;
  }
  CHECK(armor);
  CHECK(s.f[0].act == Act::Move);
  CHECK(s.f[0].move == move_index("otto", "5HP"));
}

TEST("arremesso: conecta, abre janela de tech e causa dano") {
  MatchState s = fight("nadja", "otto");
  place(s, 4500);
  const i32 h0 = s.f[1].health;
  CHECK(until_contact(s, btn::LP | btn::LK, 0, 10) == EventType::ThrowConnect);
  bool done = false;
  for (int i = 0; i < 10 && !done; ++i) done = fgtest::step_has(s, 0, 0, EventType::ThrowComplete);
  CHECK(done);
  CHECK_EQ(h0 - s.f[1].health, 1200);
  CHECK(s.f[1].act == Act::Knockdown);
}

TEST("arremesso: tech com LP+LK dentro da janela anula o dano") {
  MatchState s = fight("nadja", "otto");
  place(s, 4500);
  const i32 h0 = s.f[1].health;
  CHECK(until_contact(s, btn::LP | btn::LK, 0, 10) == EventType::ThrowConnect);
  run(s, 2, 0, 0);
  CHECK(fgtest::step_has(s, 0, btn::LP | btn::LK, EventType::ThrowTech));
  CHECK_EQ(s.f[1].health, h0);
  CHECK(s.f[0].act == Act::TechRecover);
}

TEST("arremesso: tech tardio (fora da janela) não funciona") {
  MatchState s = fight("nadja", "otto");
  place(s, 4500);
  CHECK(until_contact(s, btn::LP | btn::LK, 0, 10) == EventType::ThrowConnect);
  // A janela é de 7 frames após o contato: no 7º frame sem tech, o arremesso se completa.
  bool complete = false;
  for (int i = 0; i < 7; ++i) complete |= fgtest::step_has(s, 0, 0, EventType::ThrowComplete);
  CHECK(complete);
  CHECK(!fgtest::step_has(s, 0, btn::LP | btn::LK, EventType::ThrowTech));
}

TEST("arremesso: agarrão de comando do Otto não tem tech") {
  MatchState s = fight("otto", "nadja");
  place(s, 5000);
  feed_p1(s, "632147", 0);
  run(s, 1, raw_dir(8) | btn::LP, btn::LP | btn::LK);
  bool complete = false;
  for (int i = 0; i < 10; ++i) complete |= fgtest::step_has(s, 0, btn::LP | btn::LK, EventType::ThrowComplete);
  CHECK(complete);
  CHECK(s.f[1].health < fgtest::game_data().characters[1].health);
}

TEST("arremesso: não pega oponente em blockstun") {
  MatchState s = fight("nadja", "otto");
  place(s, 4500);
  s.f[1].act = Act::Blockstun;
  s.f[1].timer = 20;
  CHECK(until_contact(s, btn::LP | btn::LK, kP2Back, 10) != EventType::ThrowConnect);
}

TEST("troca: golpes que conectam no mesmo frame acertam os dois (como counter hit)") {
  MatchState s = fight("nadja", "nadja");
  place(s, 5000);
  bool hit0 = false, hit1 = false;
  for (int i = 0; i < 8; ++i) {
    StepEvents ev;
    FrameInput in;
    in.p[0] = i == 0 ? btn::LP : 0;
    in.p[1] = i == 0 ? btn::LP : 0;
    step(s, fgtest::game_data(), in, &ev);
    for (int k = 0; k < ev.count; ++k) {
      const EventType t = ev.items[k].type;
      if (t == EventType::Hit || t == EventType::CounterHit) (ev.items[k].player == 0 ? hit0 : hit1) = true;
    }
  }
  CHECK(hit0 && hit1);
}

TEST("combo: escala de dano e inicio leve") {
  // 2LP -> 2LP (chain). Golpe leve inicia com -10%: 280 * 0.9 = 252; segundo hit 280 * 0.9 = 252.
  MatchState s = fight("nadja", "otto");
  place(s, 5000);
  Event e1, e2;
  CHECK(until_contact(s, raw_dir(2) | btn::LP, raw_dir(5), 10, &e1) == EventType::Hit);
  CHECK_EQ(e1.value, 252);
  run(s, 1, raw_dir(2), 0);
  CHECK(until_contact(s, raw_dir(2) | btn::LP, 0, 12, &e2) == EventType::Hit);
  CHECK_EQ(e2.value, 252);
  CHECK_EQ(s.f[1].combo_hits, 2);
}

TEST("combo: chain bufferizado no hitstop mantém a postura do aperto") {
  // 5LP acerta; durante o hitstop o jogador aperta 2LP e solta o baixo antes do fim do hitstop.
  // O chain precisa sair como 2LP (direção do frame do aperto), não 5LP.
  const GameData& d = fgtest::game_data();
  MatchState s = fight("nadja", "otto");
  place(s, 5000);
  CHECK(until_contact(s, btn::LP, raw_dir(5), 10) == EventType::Hit);
  run(s, 1, raw_dir(2) | btn::LP, 0);
  run(s, 1, raw_dir(5), 0);
  for (int i = 0; i < 20 && s.f[0].move_frame != 1; ++i) run(s, 1, raw_dir(5), 0);
  const MoveDef* m = current_move(s, d, 0);
  CHECK(m != nullptr);
  if (m) CHECK(m->id == "2LP");
}

TEST("combo: nenhuma repetição de golpe vira infinito, nem no canto") {
  // Para cada golpe normal, o atacante aperta o botão a cada 2 frames por 10 s contra o boneco.
  // O defensor precisa voltar a agir e o combo não pode exceder o teto de dano.
  const GameData& d = fgtest::game_data();
  const InputBits buttons[] = {btn::LP, btn::MP, btn::HP, btn::LK, btn::MK, btn::HK};
  const InputBits dirs[] = {0, raw_dir(2), raw_dir(6), raw_dir(3)};
  for (const char* a : {"nadja", "otto"}) {
    for (const char* b : {"nadja", "otto"}) {
      for (i32 center : {0, 68000}) {
        for (InputBits dir : dirs) {
          for (InputBits bt : buttons) {
            MatchState s = fight(a, b, option::NoRoundEnd);
            place(s, 6000, center);
            s.f[0].meter = 0;
            bool recovered = false;
            i32 max_combo = 0;
            for (int t = 0; t < 600; ++t) {
              FrameInput in;
              in.p[0] = static_cast<InputBits>(dir | ((t % 2 == 0) ? bt : 0));
              step(s, d, in, nullptr);
              if (s.f[1].combo_damage > max_combo) max_combo = s.f[1].combo_damage;
              if (s.f[1].juggle > d.rules.juggle_limit) {
                fgtest::report_failure(__FILE__, __LINE__, "juggle acima do limite");
              }
              if (t > 10 && is_actionable(s, d, 1)) recovered = true;
            }
            const i32 cap = d.characters[static_cast<size_t>(s.f[1].character)].health * d.rules.max_combo_damage_permil / 1000;
            CHECK(recovered);
            CHECK(max_combo <= cap);
          }
        }
      }
    }
  }
}

TEST("invariantes: entradas aleatórias nunca violam limites da arena e dos recursos") {
  const GameData& d = fgtest::game_data();
  for (u32 seed = 1; seed <= 12; ++seed) {
    MatchSetup setup;
    setup.character[0] = static_cast<i32>(seed % 2);
    setup.character[1] = static_cast<i32>((seed / 2) % 2);
    setup.seed = seed;
    MatchState s;
    init_match(s, d, setup);
    const auto inputs = fgtest::random_inputs(seed, 6000);
    for (const auto& in : inputs) {
      step(s, d, in, nullptr);
      for (int p = 0; p < 2; ++p) {
        const FighterState& f = s.f[p];
        if (!(f.x >= -d.rules.stage_half_width && f.x <= d.rules.stage_half_width)) {
          std::printf("    seed %u frame %u p%d x=%d act=%d phase=%d\n", seed, s.frame, p, f.x, static_cast<int>(f.act),
                      static_cast<int>(s.phase));
          fgtest::report_failure(__FILE__, __LINE__, "fora da arena");
          return;
        }
        CHECK(f.y >= 0);
        CHECK(f.health >= 0);
        CHECK(f.meter >= 0 && f.meter <= max_meter(d.rules));
        CHECK(f.juggle <= d.rules.juggle_limit);
      }
      const i32 dist = s.f[0].x > s.f[1].x ? s.f[0].x - s.f[1].x : s.f[1].x - s.f[0].x;
      CHECK(dist <= d.rules.max_distance);
      CHECK(s.mare >= -d.rules.mare.limit && s.mare <= d.rules.mare.limit);
      if (s.phase == Phase::MatchEnd) break;
    }
  }
}

TEST("colisão: lutadores andando um contra o outro não se atravessam") {
  MatchState s = fight("nadja", "otto");
  place(s, 20000);
  const GameData& d = fgtest::game_data();
  for (int i = 0; i < 120; ++i) {
    run(s, 1, raw_dir(6), btn::Left);
    const i32 gap = s.f[1].x - s.f[0].x;
    CHECK(gap >= d.characters[0].push_stand.x + d.characters[0].push_stand.w - d.characters[1].push_stand.x - 1);
  }
  CHECK(s.f[0].x < s.f[1].x);
}
