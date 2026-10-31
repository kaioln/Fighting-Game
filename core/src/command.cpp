#include "fg/command.h"

#include <cctype>

namespace fg {

namespace {
constexpr u16 dir_bit(u8 d) { return static_cast<u16>(1u << d); }
}  // namespace

bool compile_motion(const std::string& text, std::vector<u16>& tokens, std::string& error) {
  tokens.clear();
  size_t i = 0;
  while (i < text.size()) {
    const char c = text[i];
    if (c == ' ') {
      ++i;
      continue;
    }
    u16 mask = 0;
    if (c == '[') {
      ++i;
      while (i < text.size() && text[i] != ']') {
        if (text[i] < '1' || text[i] > '9') {
          error = "direção inválida em '" + text + "'";
          return false;
        }
        mask = static_cast<u16>(mask | dir_bit(static_cast<u8>(text[i] - '0')));
        ++i;
      }
      if (i >= text.size()) {
        error = "colchete não fechado em '" + text + "'";
        return false;
      }
      ++i;  // ']'
    } else if (c >= '1' && c <= '9') {
      mask = dir_bit(static_cast<u8>(c - '0'));
      ++i;
    } else {
      error = "caractere inválido em '" + text + "'";
      return false;
    }
    if (mask == 0) {
      error = "token vazio em '" + text + "'";
      return false;
    }
    tokens.push_back(mask);
  }
  return true;
}

bool motion_matches(const InputHistory& h, const std::vector<u16>& tokens, i32 window, u32 end_ago) {
  if (tokens.empty()) return true;
  const u32 limit = end_ago + static_cast<u32>(window);
  u32 cursor = end_ago;
  // Percorre do último token para o primeiro, voltando no tempo.
  for (size_t t = tokens.size(); t-- > 0;) {
    bool found = false;
    for (u32 k = cursor; k <= limit && k < static_cast<u32>(kInputHistory); ++k) {
      if (tokens[t] & dir_bit(h.dir_at(k))) {
        cursor = k + 1;
        found = true;
        break;
      }
    }
    if (!found) return false;
  }
  return true;
}

bool spin_matches(const InputHistory& h, i32 window, u32 end_ago) {
  bool fwd = false, back = false, down = false, up = false;
  for (u32 k = end_ago; k <= end_ago + static_cast<u32>(window) && k < static_cast<u32>(kInputHistory); ++k) {
    const u8 d = h.dir_at(k);
    fwd |= dir_is_forward(d);
    back |= dir_is_back(d);
    down |= dir_is_down(d);
    up |= dir_is_up(d);
  }
  return fwd && back && down && up;
}

namespace {
int popcount16(u16 v) {
  int n = 0;
  while (v) {
    n += v & 1u;
    v = static_cast<u16>(v >> 1);
  }
  return n;
}
}  // namespace

bool trigger_matches(const InputHistory& h, const CommandDef& c, u32 ago) {
  const u16 pressed = h.pressed_at(ago);
  // Dois botões contam como simultâneos se pressionados com até 1 frame de diferença.
  const u16 pair = static_cast<u16>(pressed | h.pressed_at(ago + 1));
  const u16 held = h.btn_at(ago);
  switch (c.trigger) {
    case TriggerButtons::AnyPunch:
      if (pressed & btn::Punches) return true;
      return c.negative_edge && (h.released_at(ago) & btn::Punches);
    case TriggerButtons::AnyKick:
      if (pressed & btn::Kicks) return true;
      return c.negative_edge && (h.released_at(ago) & btn::Kicks);
    case TriggerButtons::TwoPunches:
      return (pressed & btn::Punches) && popcount16(static_cast<u16>(pair & held & btn::Punches)) >= 2;
    case TriggerButtons::TwoKicks:
      return (pressed & btn::Kicks) && popcount16(static_cast<u16>(pair & held & btn::Kicks)) >= 2;
    case TriggerButtons::Exact: {
      if (c.buttons == 0) return false;
      // Todos os botões exigidos seguros, ao menos um pressionado agora e o resto há no máximo 1 frame.
      if ((held & c.buttons) != c.buttons) {
        if (c.negative_edge && popcount16(c.buttons) == 1) return (h.released_at(ago) & c.buttons) != 0;
        return false;
      }
      return (pressed & c.buttons) != 0 && (pair & c.buttons) == c.buttons;
    }
  }
  return false;
}

i32 default_priority(MoveKind k) {
  switch (k) {
    case MoveKind::Super: return 600;
    case MoveKind::Enhanced: return 500;
    case MoveKind::Special: return 400;
    case MoveKind::Mare: return 350;
    case MoveKind::Throw: return 300;
    case MoveKind::CommandNormal: return 200;
    case MoveKind::Movement: return 150;
    case MoveKind::Normal: return 100;
  }
  return 0;
}

CommandMatch find_command(const CharacterDef& ch, const FighterState& f, const Rules& rules,
                          const CommandFilter& filter, i32 buffer) {
  CommandMatch best;
  i32 best_priority = -1;
  for (size_t ci = 0; ci < ch.commands.size(); ++ci) {
    const CommandDef& c = ch.commands[ci];
    if (c.move_id < 0) continue;
    if (c.priority <= best_priority) continue;  // comandos já vêm ordenados, mas garantimos
    if (c.simplified_only && f.scheme != ControlScheme::Simplified) continue;
    if (c.classic_only && f.scheme != ControlScheme::Classic) continue;
    if (filter.grounded && !c.ground) continue;
    if (!filter.grounded && !c.air) continue;
    const MoveDef& m = ch.moves[static_cast<size_t>(c.move_id)];
    switch (m.kind) {
      case MoveKind::Normal:
      case MoveKind::CommandNormal:
      case MoveKind::Movement:
        if (!filter.allow_normals) continue;
        break;
      case MoveKind::Special:
      case MoveKind::Enhanced:
        if (!filter.allow_specials) continue;
        break;
      case MoveKind::Super:
        if (!filter.allow_supers) continue;
        break;
      case MoveKind::Throw:
        // Arremessos de comando (sem tech) seguem as regras dos especiais.
        if (m.techable ? !filter.allow_throws : !filter.allow_specials) continue;
        break;
      case MoveKind::Mare:
        if (!filter.allow_mare) continue;
        // Arrebentação exige Maré a favor; Contracorrente exige Maré contra e blockstun.
        if (c.move_id == ch.move_contracorrente) {
          if (!filter.in_blockstun || filter.mare_favor > -filter.mare_threshold) continue;
        } else if (filter.in_blockstun || filter.mare_favor < filter.mare_threshold) {
          continue;
        }
        break;
    }
    if (m.meter_cost > filter.meter_available) continue;
    (void)rules;

    for (i32 ago = 0; ago < buffer; ++ago) {
      const u32 a = static_cast<u32>(ago);
      // Ignora pressões já consumidas por outra ação.
      if (f.input.head - a <= f.consumed_press) break;
      if (!trigger_matches(f.input, c, a)) continue;
      bool ok = false;
      if (!c.spin.empty()) {
        ok = spin_matches(f.input, c.window, a);
      } else if (c.motion_tokens.empty()) {
        ok = true;
      } else {
        for (const auto& tokens : c.motion_tokens) {
          if (motion_matches(f.input, tokens, c.window, a)) {
            ok = true;
            break;
          }
        }
      }
      if (ok) {
        best.command = static_cast<i32>(ci);
        best.press_ago = a;
        best_priority = c.priority;
        break;
      }
    }
  }
  return best;
}

}  // namespace fg
