#include "fg/analyzer.h"

#include "fg/input.h"

namespace fg {

std::string frame_meter(const MoveDef& m) {
  std::string s;
  for (i16 i = 1; i < m.startup; ++i) s.push_back('-');
  for (i16 i = 0; i < m.active; ++i) s.push_back('#');
  for (i16 i = 0; i < m.recovery; ++i) s.push_back('.');
  return s;
}

namespace {

struct Outcome {
  bool contact = false;
  bool blocked = false;
  bool knockdown = false;
  i16 contact_frame = 0;
  i32 damage = 0;
  i32 advantage = 0;
};

// Coloca os lutadores encostados (pushboxes tocando), inicia o golpe e mede a vantagem.
Outcome run_case(const GameData& d, i32 attacker, i16 move, i32 defender, InputBits hold, i32 gap) {
  Outcome o;
  MatchSetup setup;
  setup.character[0] = attacker;
  setup.character[1] = defender;
  setup.options = option::SkipIntro | option::NoTimer | option::NoRoundEnd;
  MatchState s;
  init_match(s, d, setup);
  const CharacterDef& ca = d.characters[static_cast<size_t>(attacker)];
  const CharacterDef& cd = d.characters[static_cast<size_t>(defender)];
  const i32 touch = ca.push_stand.x + ca.push_stand.w + (-cd.push_stand.x);
  s.f[0].x = -touch / 2 - gap / 2;
  s.f[1].x = s.f[0].x + touch + gap;
  debug_start_move(s, d, 0, move);
  StepEvents ev;
  i32 attacker_free = -1;
  i32 defender_free = -1;
  const i32 limit = 400;
  for (i32 t = 1; t <= limit; ++t) {
    FrameInput in;
    in.p[1] = hold;
    step(s, d, in, &ev);
    for (int i = 0; i < ev.count; ++i) {
      const Event& e = ev.items[i];
      if (e.player != 0 || o.contact) continue;
      if (e.type == EventType::Hit || e.type == EventType::CounterHit || e.type == EventType::PunishCounter ||
          e.type == EventType::Block) {
        o.contact = true;
        o.blocked = e.type == EventType::Block;
        o.contact_frame = s.f[0].move_frame;
        o.damage = e.type == EventType::Block ? 0 : e.value;
      }
      if (e.type == EventType::Knockdown) o.knockdown = true;
    }
    if (o.contact) {
      // "Livre" = poderia agir no próximo frame.
      if (attacker_free < 0 && is_actionable(s, d, 0)) attacker_free = t;
      if (defender_free < 0 && is_actionable(s, d, 1)) defender_free = t;
      if (attacker_free >= 0 && defender_free >= 0) break;
    }
  }
  if (o.contact && attacker_free >= 0 && defender_free >= 0) o.advantage = defender_free - attacker_free;
  if (s.f[1].act == Act::Knockdown || s.f[1].act == Act::Wakeup) o.knockdown = true;
  return o;
}

}  // namespace

MoveReport analyze_move(const GameData& d, i32 attacker, i16 move, i32 defender) {
  const MoveDef& m = d.characters[static_cast<size_t>(attacker)].moves[static_cast<size_t>(move)];
  MoveReport r;
  r.id = m.id;
  r.startup = m.startup;
  r.active = m.active;
  r.recovery = m.recovery;
  r.total = m.total_frames();
  r.meter = frame_meter(m);
  if (m.airborne) {
    r.note = "golpe aéreo: medir em cenário de salto";
    return r;
  }
  if (m.kind == MoveKind::Throw) {
    r.note = "arremesso";
    return r;
  }
  if (m.hits.empty()) {
    r.note = "sem hit (movimento)";
    return r;
  }
  // Tenta encostado e, se não alcançar, sem afastar mais (golpes de curto alcance).
  // O defensor está à direita, olhando para a esquerda: "trás" = direita.
  const bool low = m.hits.front().level == HitLevel::Low;
  const InputBits block_hold = low ? static_cast<InputBits>(btn::Right | btn::Down) : btn::Right;
  const Outcome hit = run_case(d, attacker, move, defender, 0, 0);
  const Outcome blk = run_case(d, attacker, move, defender, block_hold, 0);
  r.connects = hit.contact;
  if (!hit.contact) {
    r.note = "não conecta no boneco em pé (alvo aéreo/agachado ou alcance)";
    return r;
  }
  r.hit_frame = hit.contact_frame;
  r.damage = hit.damage;
  r.on_hit = hit.advantage;
  r.knockdown = hit.knockdown;
  r.on_block = blk.contact && blk.blocked ? blk.advantage : 0;
  if (blk.contact && !blk.blocked) r.note = "não foi bloqueado (verificar nível)";
  if (r.knockdown) r.note = "derruba";
  return r;
}

std::vector<MoveReport> analyze_character(const GameData& d, i32 attacker, i32 defender) {
  std::vector<MoveReport> out;
  const CharacterDef& ch = d.characters[static_cast<size_t>(attacker)];
  for (size_t i = 0; i < ch.moves.size(); ++i) out.push_back(analyze_move(d, attacker, static_cast<i16>(i), defender));
  return out;
}

}  // namespace fg
