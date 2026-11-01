// fgsim combos: verifica por simulação, com inputs reais, quais sequências combam.
//
// Para cada personagem (contra o outro, em pé, sem defender), no corpo a corpo:
//  - cancelamentos: normal no chão -> cada especial/super do esquema Clássico, digitando o
//    movimento durante o hitstop do primeiro golpe (como um jogador faz);
//  - links: normal -> normal, apertando o segundo no primeiro frame em que o atacante pode agir;
//  - chains: normal -> normal durante a janela de chain.
// O resultado compara o que o frame data promete (vantagem no acerto x startup) com o que a
// simulação entrega; divergências são listadas como suspeitas de bug.

#include "combos.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "fg/analyzer.h"
#include "fg/sim.h"

namespace fg {
namespace {

InputBits raw(int numpad, i32 facing) {
  InputBits b = 0;
  const bool fwd = numpad == 3 || numpad == 6 || numpad == 9;
  const bool back = numpad == 1 || numpad == 4 || numpad == 7;
  if (fwd) b |= facing >= 0 ? btn::Right : btn::Left;
  if (back) b |= facing >= 0 ? btn::Left : btn::Right;
  if (numpad <= 3) b |= btn::Down;
  if (numpad >= 7) b |= btn::Up;
  return b;
}

InputBits trigger_buttons(const CommandDef& c) {
  switch (c.trigger) {
    case TriggerButtons::AnyPunch: return btn::MP;
    case TriggerButtons::AnyKick: return btn::MK;
    case TriggerButtons::TwoPunches: return static_cast<InputBits>(btn::LP | btn::MP);
    case TriggerButtons::TwoKicks: return static_cast<InputBits>(btn::LK | btn::MK);
    case TriggerButtons::Exact: return c.buttons;
  }
  return 0;
}

// Sequência de inputs (um por frame) que executa o comando, para P1 olhando com `facing`.
std::vector<InputBits> command_inputs(const CommandDef& c, i32 facing) {
  std::vector<InputBits> seq;
  if (!c.motion_tokens.empty()) {
    for (u16 mask : c.motion_tokens.front()) {
      int d = 5;
      for (int k = 1; k <= 9; ++k)
        if (mask & (1u << k)) {
          d = k;
          break;
        }
      seq.push_back(raw(d, facing));
    }
  }
  if (seq.empty()) seq.push_back(0);
  seq.back() = static_cast<InputBits>(seq.back() | trigger_buttons(c));
  return seq;
}

InputBits normal_input(const CharacterDef& ch, i16 move) {
  static const InputBits buttons[6] = {btn::LP, btn::MP, btn::HP, btn::LK, btn::MK, btn::HK};
  for (int posture = 0; posture < 2; ++posture)
    for (int b = 0; b < 6; ++b)
      if (ch.normals[posture][b] == move) return static_cast<InputBits>(buttons[b] | (posture == 1 ? btn::Down : 0));
  return 0;
}

struct Outcome {
  bool first_hit = false;   // o primeiro golpe acertou
  bool second_hit = false;  // o segundo golpe acertou
  bool combo = false;       // ...como parte do combo (sem o defensor se recuperar)
  i32 damage = 0;           // dano total do combo
};

MatchState fresh(const GameData& d, i32 a, i32 b) {
  MatchState s{};
  MatchSetup setup;
  setup.character[0] = a;
  setup.character[1] = b;
  setup.options = static_cast<u8>(option::SkipIntro | option::NoTimer);
  init_match(s, d, setup);
  // Corpos encostados, como no frame analyzer.
  const CharacterDef& ca = d.characters[static_cast<size_t>(a)];
  const CharacterDef& cb = d.characters[static_cast<size_t>(b)];
  const i32 touch = ca.push_stand.x + ca.push_stand.w + (-cb.push_stand.x);
  s.f[0].x = -touch / 2;
  s.f[1].x = s.f[0].x + touch;
  s.f[0].meter = max_meter(d.rules);
  return s;
}

// Executa: aperta `first` (1 frame); quando ele acerta, envia `second` (sequência) começando
// `offset` frames depois do acerto, ou (link) no primeiro frame em que P1 pode agir.
Outcome run_pair(const GameData& d, i32 a, i32 b, InputBits first, i16 first_move, const std::vector<InputBits>& second,
                 i16 second_move, bool link, int offset) {
  Outcome o;
  MatchState s = fresh(d, a, b);
  std::vector<InputBits> queue;
  int hit_frame = -1;
  bool sent = false;
  for (int t = 0; t < 240; ++t) {
    FrameInput in;
    if (t == 0) in.p[0] = first;
    if (!queue.empty()) {
      in.p[0] = queue.front();
      queue.erase(queue.begin());
    }
    StepEvents ev;
    step(s, d, in, &ev);
    for (int i = 0; i < ev.count; ++i) {
      const Event& e = ev.items[i];
      const bool hit = e.type == EventType::Hit || e.type == EventType::CounterHit || e.type == EventType::PunishCounter;
      if (e.player != 0 || !hit) continue;
      o.damage += e.value;
      if (!o.first_hit && e.move == first_move) {
        o.first_hit = true;
        hit_frame = t;
      } else if (o.first_hit && e.move == second_move && !o.second_hit) {
        o.second_hit = true;
        o.combo = s.f[1].combo_hits >= 2;
      }
    }
    if (o.first_hit && !sent) {
      const bool ready = link ? is_actionable(s, d, 0) : (t >= hit_frame + offset);
      if (ready) {
        queue = second;
        sent = true;
      }
    }
    if (o.second_hit) break;
  }
  return o;
}

const char* ok(bool v) { return v ? "sim" : "não"; }

// "sim", "lento" (acertou depois de o defensor se recuperar) ou "errou" (não alcançou).
const char* verdict(const Outcome& o) { return o.combo ? "sim" : (o.second_hit ? "não (lento)" : "não (errou)"); }

// Um normal apertado num frame, com a direção mantida por alguns frames (como um jogador faz;
// um botão bufferizado usa a direção do frame em que sai).
std::vector<InputBits> hold_normal(InputBits in) {
  std::vector<InputBits> v(8, static_cast<InputBits>(in & btn::Directions));
  v[0] = in;
  return v;
}

}  // namespace

int report_combos(const GameData& d) {
  int suspicious = 0;
  for (i32 a = 0; a < static_cast<i32>(d.characters.size()); ++a) {
    const i32 b = (a + 1) % static_cast<i32>(d.characters.size());
    const CharacterDef& ch = d.characters[static_cast<size_t>(a)];
    std::printf("### %s — contra %s\n\n", ch.name.c_str(), d.characters[static_cast<size_t>(b)].name.c_str());

    // Normais no chão que acertam no corpo a corpo.
    std::vector<i16> normals;
    for (int posture = 0; posture < 2; ++posture)
      for (int k = 0; k < 6; ++k)
        if (ch.normals[posture][k] >= 0) normals.push_back(ch.normals[posture][k]);
    const auto reports = analyze_character(d, a, b);
    auto report_of = [&](i16 m) -> const MoveReport* {
      for (const auto& r : reports)
        if (r.id == ch.moves[static_cast<size_t>(m)].id) return &r;
      return nullptr;
    };

    // Cancelamentos em especiais e supers.
    std::printf("**Cancelamentos (normal → especial/super), digitados durante o hitstop**\n\n");
    std::printf("| Normal | Especial | Combou | Dano total |\n|---|---|---|---:|\n");
    int cancel_ok = 0, cancel_total = 0;
    for (i16 na : normals) {
      const MoveDef& ma = ch.moves[static_cast<size_t>(na)];
      bool has_special_cancel = false;
      for (const auto& cs : ma.cancels)
        if (cs.mask & (cancel::Special | cancel::Super)) has_special_cancel = true;
      if (!has_special_cancel) continue;
      for (const CommandDef& c : ch.commands) {
        if (c.simplified_only || !c.spin.empty() || !c.ground || c.move_id < 0) continue;
        const MoveDef& mb = ch.moves[static_cast<size_t>(c.move_id)];
        if (mb.kind != MoveKind::Special && mb.kind != MoveKind::Enhanced && mb.kind != MoveKind::Super) continue;
        bool allowed = false;
        for (const auto& cs : ma.cancels) {
          if (mb.kind == MoveKind::Super && (cs.mask & cancel::Super)) allowed = true;
          if (mb.kind != MoveKind::Super && (cs.mask & cancel::Special)) allowed = true;
        }
        if (!allowed) continue;
        const auto seq = command_inputs(c, 1);
        const Outcome o = run_pair(d, a, b, normal_input(ch, na), na, seq, c.move_id, false, 0);
        if (!o.first_hit) continue;
        ++cancel_total;
        if (o.combo) ++cancel_ok;
        std::printf("| %s | %s (%s) | %s | %d |\n", ma.id.c_str(), mb.id.c_str(), c.id.c_str(), verdict(o),
                    o.combo ? o.damage : 0);
      }
    }
    std::printf("\n%d de %d cancelamentos combam.\n\n", cancel_ok, cancel_total);

    // Links e chains entre normais.
    std::printf("**Links normal → normal** (segundo golpe apertado no primeiro frame livre)\n\n");
    std::printf("| Primeiro | Segundo | Vantagem no acerto | Startup | Esperado | Simulação |\n|---|---|---:|---:|---|---|\n");
    for (i16 na : normals) {
      const MoveReport* ra = report_of(na);
      if (!ra || !ra->connects || ra->knockdown) continue;
      for (i16 nb : normals) {
        const MoveDef& mb = ch.moves[static_cast<size_t>(nb)];
        const bool expected = ra->on_hit >= mb.startup;
        const Outcome o = run_pair(d, a, b, normal_input(ch, na), na, hold_normal(normal_input(ch, nb)), nb, true, 0);
        if (!o.first_hit) continue;
        // Só lista links possíveis e divergências (o resto seria uma tabela enorme de "não").
        if (!expected && !o.combo) continue;
        const bool mismatch = expected != o.combo && o.second_hit;
        if (mismatch) ++suspicious;
        std::printf("| %s | %s | %+d | %d | %s | %s%s |\n", ch.moves[static_cast<size_t>(na)].id.c_str(), mb.id.c_str(),
                    ra->on_hit, mb.startup, ok(expected), verdict(o),
                    mismatch ? " ⚠" : "");
      }
    }
    std::printf("\n");

    std::printf("**Chains** (segundo golpe apertado dentro da janela de chain)\n\n");
    std::printf("| Primeiro | Segundo | Combou |\n|---|---|---|\n");
    for (i16 na : normals) {
      const MoveDef& ma = ch.moves[static_cast<size_t>(na)];
      for (const auto& cs : ma.cancels) {
        if (!(cs.mask & cancel::Chain)) continue;
        for (i16 nb : ma.chain_target_ids) {
          // aperta no início da janela de chain (relativo ao acerto, que ocorre no startup)
          const int offset = std::max(1, cs.from - ma.startup);
          const Outcome o = run_pair(d, a, b, normal_input(ch, na), na, hold_normal(normal_input(ch, nb)), nb, false, offset);
          if (!o.first_hit) continue;
          if (!o.combo && o.second_hit) ++suspicious;
          std::printf("| %s | %s | %s |\n", ma.id.c_str(), ch.moves[static_cast<size_t>(nb)].id.c_str(),
                      verdict(o));
        }
      }
    }
    std::printf("\n");
  }
  std::printf("Divergências entre frame data e simulação: %d\n", suspicious);
  return suspicious == 0 ? 0 : 1;
}

}  // namespace fg
