#include "fg/analyzer.h"
#include "helpers.h"

using namespace fg;

namespace {
bool simple_single_hit(const MoveDef& m) {
  return m.hits.size() == 1 && !m.hits[0].knockdown && !m.hits[0].launch && m.armor.empty() &&
         m.kind != MoveKind::Super;
}
}  // namespace

TEST("frame data: o primeiro contato acontece exatamente no frame de startup") {
  const GameData& d = fgtest::game_data();
  int checked = 0;
  for (i32 a = 0; a < static_cast<i32>(d.characters.size()); ++a) {
    for (i32 b = 0; b < static_cast<i32>(d.characters.size()); ++b) {
      for (const auto& r : analyze_character(d, a, b)) {
        if (!r.connects) continue;
        if (r.hit_frame != r.startup) {
          std::printf("    %s.%s vs %s: contato no frame %d, startup %d\n", d.characters[static_cast<size_t>(a)].id.c_str(),
                      r.id.c_str(), d.characters[static_cast<size_t>(b)].id.c_str(), r.hit_frame, r.startup);
        }
        CHECK_EQ(r.hit_frame, r.startup);
        ++checked;
      }
    }
  }
  CHECK(checked > 40);
}

TEST("frame data: vantagem simulada = hitstun - (ativos restantes + recovery)") {
  // Verifica que a simulação implementa exatamente a definição de frame advantage.
  const GameData& d = fgtest::game_data();
  int checked = 0;
  for (i32 a = 0; a < static_cast<i32>(d.characters.size()); ++a) {
    const CharacterDef& ch = d.characters[static_cast<size_t>(a)];
    for (size_t mi = 0; mi < ch.moves.size(); ++mi) {
      const MoveDef& m = ch.moves[mi];
      if (!simple_single_hit(m) || m.airborne) continue;
      const MoveReport r = analyze_move(d, a, static_cast<i16>(mi), 1 - a);
      if (!r.connects) continue;
      const i32 remaining = m.last_active() - r.hit_frame + m.recovery;
      CHECK_EQ(r.on_hit, m.hits[0].hitstun - remaining);
      if (r.on_block != m.hits[0].blockstun - remaining) std::printf("    %s.%s: %s\n", ch.id.c_str(), m.id.c_str(), r.note.c_str());
      CHECK_EQ(r.on_block, m.hits[0].blockstun - remaining);
      ++checked;
    }
  }
  CHECK(checked > 15);
}

TEST("frame data: valores de design da Nadja (5LP +4/+1, 5MK +1/-4, 2HK punível)") {
  const GameData& d = fgtest::game_data();
  const i32 n = fgtest::char_index("nadja");
  const i32 o = fgtest::char_index("otto");
  const MoveReport lp = analyze_move(d, n, fgtest::move_index("nadja", "5LP"), o);
  CHECK_EQ(lp.on_hit, 4);
  CHECK_EQ(lp.on_block, 1);
  const MoveReport mk = analyze_move(d, n, fgtest::move_index("nadja", "5MK"), o);
  CHECK_EQ(mk.on_hit, 1);
  CHECK_EQ(mk.on_block, -4);
  const MoveReport sweep = analyze_move(d, n, fgtest::move_index("nadja", "2HK"), o);
  CHECK(sweep.knockdown);
  CHECK(sweep.on_block <= -10);
}

TEST("frame data: reversals são muito puníveis no bloqueio") {
  const GameData& d = fgtest::game_data();
  for (const char* who : {"nadja", "otto"}) {
    const i32 c = fgtest::char_index(who);
    const MoveReport r = analyze_move(d, c, fgtest::move_index(who, "623P"), 1 - c);
    CHECK(r.connects);
    CHECK(r.on_block <= -20);
  }
}

TEST("frame meter: representação textual do golpe") {
  MoveDef m;
  m.startup = 4;
  m.active = 2;
  m.recovery = 3;
  CHECK(frame_meter(m) == "---##...");
}
