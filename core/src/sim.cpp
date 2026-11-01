#include "fg/sim.h"

#include "fg/command.h"

namespace fg {

namespace {

// ---------------------------------------------------------------------------
// Acesso a dados
// ---------------------------------------------------------------------------

const CharacterDef& char_of(const GameData& d, const FighterState& f) {
  return d.characters[static_cast<size_t>(f.character)];
}

const MoveDef* move_of(const GameData& d, const FighterState& f) {
  if (f.act != Act::Move || f.move < 0) return nullptr;
  return &char_of(d, f).moves[static_cast<size_t>(f.move)];
}

bool in_span(i16 frame, i16 from, i16 to) { return frame >= from && frame <= to; }

template <typename Spans>
bool any_span(const Spans& spans, i16 frame) {
  for (const auto& s : spans) {
    if (in_span(frame, s.from, s.to)) return true;
  }
  return false;
}


// ---------------------------------------------------------------------------
// Maré
// ---------------------------------------------------------------------------

void add_mare(MatchState& s, const Rules& r, int player, i32 amount) {
  s.mare += (player == 0 ? amount : -amount);
  s.mare = clamp_value(s.mare, -r.mare.limit, r.mare.limit);
}

// ---------------------------------------------------------------------------
// Estado do lutador
// ---------------------------------------------------------------------------

bool is_airborne(const GameData& d, const FighterState& f) {
  if (f.act == Act::Air || f.act == Act::Juggle) return true;
  if (f.act == Act::Move) {
    const MoveDef* m = move_of(d, f);
    if (m->airborne) return true;
    if (any_span(m->airborne_frames, f.move_frame)) return true;
  }
  return f.y > 0;
}

bool is_crouching(const GameData& d, const FighterState& f) {
  switch (f.act) {
    case Act::Crouch: return true;
    case Act::Hitstun:
    case Act::Blockstun: return f.crouch_stun;
    case Act::Move: return move_of(d, f)->crouching;
    default: return false;
  }
}

void enter_idle(FighterState& f) {
  f.act = Act::Idle;
  f.move = -1;
  f.move_frame = 0;
  f.timer = 0;
  f.vx = 0;
  f.vy = 0;
  f.gravity = 0;
  f.jump_kind = JumpKind::None;
  f.air_action_used = false;
  f.damage_permil_override = 1000;
}

void start_move(MatchState& s, const GameData& d, int p, i16 move_index, u32 press_ago, i32 damage_permil,
                StepEvents* ev) {
  FighterState& f = s.f[p];
  const CharacterDef& ch = char_of(d, f);
  const MoveDef& m = ch.moves[static_cast<size_t>(move_index)];
  const bool was_air = is_airborne(d, f);
  f.act = Act::Move;
  f.move = move_index;
  f.move_frame = 0;
  f.hits_done = 0;
  f.armor_used = 0;
  f.move_connected = false;
  f.damage_permil_override = static_cast<i16>(damage_permil);
  f.meter -= m.meter_cost;
  f.consumed_press = f.input.head - press_ago;
  f.buffer_bonus = 0;
  if (m.airborne) {
    f.air_action_used = true;  // mantém vx/vy do salto
  } else if (!was_air) {
    f.vx = 0;
    f.vy = 0;
    f.gravity = 0;
  }
  if (m.super_freeze > 0) {
    s.freeze = m.super_freeze;
    s.freeze_owner = static_cast<i8>(p);
    if (ev) ev->add({EventType::SuperFreeze, static_cast<i8>(p), move_index, f.x, f.y, m.super_freeze});
  }
  if (m.kind == MoveKind::Mare) {
    s.mare = 0;
    if (ev) {
      const bool contra = move_index == ch.move_contracorrente;
      ev->add({contra ? EventType::Contracorrente : EventType::Arrebentacao, static_cast<i8>(p), move_index, f.x,
               f.y, 0});
    }
  }
  if (move_index == ch.move_dash) add_mare(s, d.rules, p, d.rules.mare.dash_forward);
  if (ev) ev->add({EventType::MoveStart, static_cast<i8>(p), move_index, f.x, f.y, 0});
}

// Estados em que o lutador decide livremente o que fazer no chão.
bool is_ground_neutral(Act a) {
  return a == Act::Idle || a == Act::Crouch || a == Act::WalkForward || a == Act::WalkBack || a == Act::Run ||
         a == Act::Guard;
}

// ---------------------------------------------------------------------------
// Decisão (inputs -> ações)
// ---------------------------------------------------------------------------

int button_slot(u16 pressed) {
  // Quando vários botões são pressionados juntos, vence o mais forte.
  static const u16 order[6] = {btn::HP, btn::HK, btn::MP, btn::MK, btn::LP, btn::LK};
  static const int slot[6] = {2, 5, 1, 4, 0, 3};
  for (int i = 0; i < 6; ++i) {
    if (pressed & order[i]) return slot[i];
  }
  return -1;
}

int simplified_slot(u16 pressed) {
  // Esquema Simplificado: LP/MP/HP físicos representam L/M/H.
  if (pressed & btn::HP) return 2;
  if (pressed & btn::MP) return 1;
  if (pressed & btn::LP) return 0;
  return -1;
}

i32 buffer_for(const Rules& r, const FighterState& f) { return r.button_buffer + f.buffer_bonus; }

// Procura um normal pressionado dentro do buffer. Retorna o índice do golpe e quantos frames atrás.
bool find_normal(const CharacterDef& ch, const FighterState& f, int stance, i32 buffer, i16& move, u32& ago,
                 const std::vector<i16>* only) {
  for (i32 a = 0; a < buffer; ++a) {
    const u32 ua = static_cast<u32>(a);
    if (f.input.head - ua <= f.consumed_press) break;
    const u16 pressed = f.input.pressed_at(ua);
    if ((pressed & (btn::Punches | btn::Kicks)) == 0) continue;
    // No chão, a postura (em pé/agachado) vem da direção do frame em que o botão foi apertado:
    // um 2LP bufferizado durante o hitstop continua sendo 2LP mesmo que o jogador solte o baixo.
    const int st = stance == 2 ? 2 : (dir_is_down(f.input.dir_at(ua)) ? 1 : 0);
    i16 idx = -1;
    if (f.scheme == ControlScheme::Simplified && ch.simplified_normals[st][0] >= 0) {
      const int sl = simplified_slot(pressed);
      if (sl >= 0) idx = ch.simplified_normals[st][sl];
    } else {
      const int sl = button_slot(pressed);
      if (sl >= 0) idx = ch.normals[st][sl];
    }
    if (idx < 0) continue;
    if (only != nullptr) {
      bool allowed = false;
      for (i16 t : *only) allowed |= (t == idx);
      if (!allowed) continue;
    }
    move = idx;
    ago = ua;
    return true;
  }
  return false;
}

bool dash_input(const FighterState& f, const Rules& r, bool forward) {
  const u8 tap = forward ? 6 : 4;
  if (f.input.dir_at(0) != tap || f.input.dir_at(1) == tap) return false;
  // Exige um frame neutro entre os toques: "6 5 6" ou "4 5 4".
  bool neutral_seen = false;
  for (u32 k = 1; k <= static_cast<u32>(r.dash_window); ++k) {
    const u8 dd = f.input.dir_at(k);
    if (!neutral_seen) {
      if (dd == 5) neutral_seen = true;
      else if (dd != tap) return false;
    } else if (dd == tap) {
      return true;
    } else if (dd != 5) {
      return false;
    }
  }
  return false;
}

CommandFilter base_filter(const MatchState& s, const GameData& d, int p, bool grounded) {
  CommandFilter cf;
  cf.grounded = grounded;
  cf.meter_available = s.f[p].meter;
  cf.mare_favor = mare_favor(s, p);
  cf.mare_threshold = d.rules.mare.threshold;
  cf.allow_mare = false;
  return cf;
}

bool try_commands(MatchState& s, const GameData& d, int p, const CommandFilter& cf, StepEvents* ev) {
  FighterState& f = s.f[p];
  const CharacterDef& ch = char_of(d, f);
  const CommandMatch cm = find_command(ch, f, d.rules, cf, buffer_for(d.rules, f));
  if (cm.command < 0) return false;
  const CommandDef& c = ch.commands[static_cast<size_t>(cm.command)];
  const i32 dmg = c.simplified_only ? c.simplified_damage_permil : 1000;
  start_move(s, d, p, c.move_id, cm.press_ago, dmg, ev);
  return true;
}

void start_jump(FighterState& f, u8 dir) {
  f.act = Act::PreJump;
  f.timer = 0;
  f.vx = 0;
  f.jump_dir = static_cast<i8>(dir_is_forward(dir) ? 1 : (dir_is_back(dir) ? -1 : 0));
  f.jump_kind = JumpKind::Normal;
}

void decide_ground(MatchState& s, const GameData& d, int p, StepEvents* ev) {
  FighterState& f = s.f[p];
  const CharacterDef& ch = char_of(d, f);
  const Rules& r = d.rules;
  const u8 dir = f.input.dir_at(0);

  CommandFilter cf = base_filter(s, d, p, true);
  if (try_commands(s, d, p, cf, ev)) return;

  i16 move = -1;
  u32 ago = 0;
  const int stance = dir_is_down(dir) ? 1 : 0;
  if (find_normal(ch, f, stance, buffer_for(r, f), move, ago, nullptr)) {
    start_move(s, d, p, move, ago, 1000, ev);
    return;
  }

  if (dash_input(f, r, true)) {
    if (ch.run_speed > 0) {
      f.act = Act::Run;
      f.vx = f.facing * ch.run_speed;
      add_mare(s, r, p, r.mare.dash_forward);
    } else {
      start_move(s, d, p, ch.move_dash, 0, 1000, ev);
    }
    return;
  }
  if (dash_input(f, r, false)) {
    start_move(s, d, p, ch.move_backdash, 0, 1000, ev);
    return;
  }

  if (dir_is_up(dir)) {
    start_jump(f, dir);
    return;
  }

  if (f.act == Act::Run && dir_is_forward(dir) && !dir_is_down(dir)) {
    f.vx = f.facing * ch.run_speed;
    return;
  }
  if (dir_is_down(dir)) {
    f.act = Act::Crouch;
    f.vx = 0;
  } else if (dir == 6) {
    f.act = Act::WalkForward;
    f.vx = f.facing * ch.walk_forward;
  } else if (dir == 4) {
    // Guarda por proximidade: com um ataque do oponente se aproximando, segurar trás defende no lugar.
    const FighterState& o = s.f[1 - p];
    bool threat = false;
    if (o.act == Act::Move) {
      const MoveDef& om = char_of(d, o).moves[static_cast<size_t>(o.move)];
      const i32 dist = o.x > f.x ? o.x - f.x : f.x - o.x;
      threat = !om.hits.empty() && o.move_frame <= om.last_active() && dist <= r.proximity_guard;
    }
    f.act = threat ? Act::Guard : Act::WalkBack;
    f.vx = threat ? 0 : -f.facing * ch.walk_back;
  } else {
    f.act = Act::Idle;
    f.vx = 0;
  }
}

void decide_air(MatchState& s, const GameData& d, int p, StepEvents* ev) {
  FighterState& f = s.f[p];
  if (f.air_action_used) return;
  const CharacterDef& ch = char_of(d, f);
  CommandFilter cf = base_filter(s, d, p, false);
  if (try_commands(s, d, p, cf, ev)) return;
  i16 move = -1;
  u32 ago = 0;
  if (find_normal(ch, f, 2, buffer_for(d.rules, f), move, ago, nullptr)) start_move(s, d, p, move, ago, 1000, ev);
}

// Cancelamentos durante um golpe.
void decide_cancel(MatchState& s, const GameData& d, int p, StepEvents* ev) {
  FighterState& f = s.f[p];
  const CharacterDef& ch = char_of(d, f);
  const MoveDef& m = *move_of(d, f);
  // O frame usado é o próximo (o golpe novo substitui o frame que seria executado agora).
  const i16 frame = f.move_frame;
  u8 mask = 0;
  for (const auto& c : m.cancels) {
    if (in_span(frame, c.from, c.to) && (f.move_connected || c.on_whiff)) mask |= c.mask;
  }
  // Regra universal da Maré: qualquer normal que fez contato pode ser cancelado em Arrebentação.
  const bool normal = m.kind == MoveKind::Normal || m.kind == MoveKind::CommandNormal;
  if (normal && f.move_connected && frame >= m.first_active()) mask |= cancel::Mare;
  if (mask == 0) return;

  const bool grounded = !is_airborne(d, f);
  CommandFilter cf = base_filter(s, d, p, grounded);
  cf.allow_normals = false;
  cf.allow_throws = false;
  cf.allow_specials = (mask & cancel::Special) != 0;
  cf.allow_supers = (mask & (cancel::Super | cancel::Special)) != 0;
  cf.allow_mare = (mask & cancel::Mare) != 0;
  if ((cf.allow_specials || cf.allow_supers || cf.allow_mare) && try_commands(s, d, p, cf, ev)) return;

  if (mask & cancel::Chain) {
    i16 move = -1;
    u32 ago = 0;
    const int stance = grounded ? (dir_is_down(f.input.dir_at(0)) ? 1 : 0) : 2;
    if (find_normal(ch, f, stance, buffer_for(d.rules, f), move, ago, &m.chain_target_ids)) {
      start_move(s, d, p, move, ago, 1000, ev);
      return;
    }
  }
  if ((mask & cancel::Jump) && grounded && dir_is_up(f.input.dir_at(0))) {
    start_jump(f, f.input.dir_at(0));
  }
}

void decide(MatchState& s, const GameData& d, int p, StepEvents* ev) {
  FighterState& f = s.f[p];
  const Rules& r = d.rules;

  // Fim natural de estados temporários: o lutador recupera o controle neste frame.
  if (f.act == Act::Move) {
    const MoveDef& m = *move_of(d, f);
    if (f.move_frame >= m.total_frames() && !m.airborne && f.y <= 0) {
      enter_idle(f);
    }
  }
  if ((f.act == Act::Hitstun || f.act == Act::Blockstun || f.act == Act::Landing || f.act == Act::TechRecover) &&
      f.timer <= 0) {
    enter_idle(f);
    f.combo_hits = 0;
    f.combo_damage = 0;
    f.juggle = 0;
  }
  if (f.act == Act::Wakeup && f.timer <= 0) {
    enter_idle(f);
    f.combo_hits = 0;
    f.combo_damage = 0;
    f.juggle = 0;
    f.buffer_bonus = static_cast<i16>(r.wakeup_buffer > r.button_buffer ? r.wakeup_buffer - r.button_buffer : 0);
    f.throw_protect = r.hard_wakeup_invuln;
  }

  if (is_ground_neutral(f.act)) {
    decide_ground(s, d, p, ev);
  } else if (f.act == Act::PreJump) {
    // Especiais podem cancelar o pré-salto (ex.: giro de 360 graus).
    CommandFilter cf = base_filter(s, d, p, true);
    cf.allow_normals = false;
    cf.allow_throws = false;
    try_commands(s, d, p, cf, ev);
  } else if (f.act == Act::Air) {
    decide_air(s, d, p, ev);
  } else if (f.act == Act::Move) {
    decide_cancel(s, d, p, ev);
  } else if (f.act == Act::Blockstun) {
    CommandFilter cf = base_filter(s, d, p, true);
    cf.allow_normals = false;
    cf.allow_specials = false;
    cf.allow_supers = false;
    cf.allow_throws = false;
    cf.allow_mare = true;
    cf.in_blockstun = true;
    try_commands(s, d, p, cf, ev);
  }
}

// ---------------------------------------------------------------------------
// Avanço (contadores e física)
// ---------------------------------------------------------------------------

void apply_push(MatchState& s, const Rules& r, int p) {
  FighterState& f = s.f[p];
  if (f.push_remaining == 0) return;
  i32 step = f.push_remaining / 4;
  if (step == 0) step = f.push_remaining;
  f.x += step;
  f.push_remaining -= step;
  const i32 limit = r.stage_half_width;
  if (f.x > limit || f.x < -limit) {
    // Na parede, o restante do empurrão é transferido ao oponente.
    const i32 over = f.x > limit ? f.x - limit : f.x + limit;
    f.x -= over;
    s.f[1 - p].push_remaining -= over + f.push_remaining;
    f.push_remaining = 0;
  }
}

void land(MatchState& s, const GameData& d, int p, StepEvents* ev) {
  FighterState& f = s.f[p];
  const CharacterDef& ch = char_of(d, f);
  f.y = 0;
  f.vy = 0;
  f.vx = 0;
  f.gravity = 0;
  if (f.act == Act::Juggle) {
    f.act = Act::Knockdown;
    f.timer = d.rules.knockdown_frames;
    if (ev) ev->add({EventType::Knockdown, static_cast<i8>(p), -1, f.x, 0, 0});
    return;
  }
  if (f.act == Act::Move) {
    const MoveDef& m = *move_of(d, f);
    f.act = Act::Landing;
    f.timer = m.landing_recovery > 0 ? m.landing_recovery : ch.jump.landing;
    f.move = -1;
    return;
  }
  if (f.act == Act::KO) return;
  f.act = Act::Landing;
  f.timer = (f.jump_kind == JumpKind::Hop ? ch.hop.landing : ch.jump.landing);
}

void advance(MatchState& s, const GameData& d, int p, StepEvents* ev) {
  FighterState& f = s.f[p];
  const CharacterDef& ch = char_of(d, f);
  switch (f.act) {
    case Act::Idle:
    case Act::Crouch:
    case Act::WalkForward:
    case Act::WalkBack:
    case Act::Run:
    case Act::Guard:
      f.x += f.vx;
      break;
    case Act::PreJump: {
      ++f.timer;
      const JumpDef& jd = ch.jump;
      if (f.timer >= jd.prejump) {
        // Hop se o direcional para cima já foi solto ao fim do pré-salto.
        const bool hop = !dir_is_up(f.input.dir_at(0));
        const JumpDef& use = hop ? ch.hop : ch.jump;
        f.jump_kind = hop ? JumpKind::Hop : JumpKind::Normal;
        f.act = Act::Air;
        f.vy = use.vy;
        f.gravity = use.gravity;
        f.vx = f.facing * (f.jump_dir > 0 ? use.vx_forward : (f.jump_dir < 0 ? -use.vx_back : 0));
        f.timer = 0;
        f.air_action_used = false;
      }
      break;
    }
    case Act::Air:
    case Act::Juggle:
    case Act::KO:
      if (f.act == Act::KO && f.y <= 0) break;
      f.x += f.vx;
      f.y += f.vy;
      f.vy -= f.gravity;
      if (f.y <= 0 && f.vy < 0) land(s, d, p, ev);
      break;
    case Act::Landing:
    case Act::Hitstun:
    case Act::Blockstun:
    case Act::TechRecover:
    case Act::Knockdown:
    case Act::Wakeup:
      --f.timer;
      if (f.act == Act::Knockdown && f.timer <= 0) {
        f.act = Act::Wakeup;
        f.timer = d.rules.wakeup_frames;
      }
      break;
    case Act::Move: {
      const MoveDef& m = *move_of(d, f);
      ++f.move_frame;
      for (const auto& v : m.velocity) {
        if (!in_span(f.move_frame, v.from, v.to)) continue;
        if (v.impulse) {
          if (f.move_frame == v.from) {
            f.vx = f.facing * v.vx;
            f.vy = v.vy;
            if (v.vy != 0 && f.gravity == 0) f.gravity = ch.jump.gravity;
          }
        } else {
          f.vx = f.facing * v.vx;
          if (v.vy != 0) f.vy = v.vy;
        }
      }
      const bool air = m.airborne || f.y > 0 || f.vy != 0;
      if (air) {
        if (f.gravity == 0) f.gravity = ch.jump.gravity;
        f.x += f.vx;
        f.y += f.vy;
        f.vy -= f.gravity;
        if (f.y <= 0 && f.vy < 0) {
          f.y = 0;
          if (m.airborne || m.ends_on_landing) {
            land(s, d, p, ev);
            break;
          }
          f.vy = 0;
          f.gravity = 0;
        }
      } else {
        f.x += f.vx;
        // Sem span de velocidade ativo, o golpe no chão não desliza.
        bool vel_active = false;
        for (const auto& v : m.velocity) vel_active |= in_span(f.move_frame, v.from, v.to) && !v.impulse;
        if (!vel_active) f.vx = 0;
      }
      if (f.move_frame >= m.total_frames() && f.y > 0 && !m.ends_on_landing) {
        // Golpe terminou ainda no ar: volta à queda livre, sem nova ação aérea.
        f.act = Act::Air;
        f.move = -1;
        f.air_action_used = true;
      }
      break;
    }
    case Act::ThrowAttack:
    case Act::Thrown:
      // Resolvido em resolve_throws.
      break;
  }
  // Paredes da arena (vale também para quem está no ar ou caindo).
  f.x = clamp_value(f.x, -d.rules.stage_half_width, d.rules.stage_half_width);
  // Bônus de buffer e proteção contra arremesso decaem um por frame não congelado.
  if (f.buffer_bonus > 0) --f.buffer_bonus;
  if (f.throw_protect > 0) --f.throw_protect;
}

// ---------------------------------------------------------------------------
// Colisão
// ---------------------------------------------------------------------------

Box push_box(const GameData& d, const FighterState& f) {
  const CharacterDef& ch = char_of(d, f);
  if (is_airborne(d, f)) return ch.push_air;
  if (is_crouching(d, f)) return ch.push_crouch;
  return ch.push_stand;
}

void collect_hurtboxes(const GameData& d, const FighterState& f, WorldBox* out, int& n) {
  n = 0;
  if (f.act == Act::Move) {
    const MoveDef& m = *move_of(d, f);
    for (const auto& b : m.boxes) {
      if (b.kind == BoxKind::Hurt && in_span(f.move_frame, b.from, b.to) && n < 8) {
        out[n++] = to_world(b.box, f.x, f.y, f.facing);
      }
    }
    if (n > 0) return;
  }
  const CharacterDef& ch = char_of(d, f);
  const Box& b = is_airborne(d, f) ? ch.hurt_air : (is_crouching(d, f) ? ch.hurt_crouch : ch.hurt_stand);
  out[n++] = to_world(b, f.x, f.y, f.facing);
}

bool strike_invulnerable(const GameData& d, const FighterState& f) {
  switch (f.act) {
    case Act::Knockdown:
    case Act::Thrown:
    case Act::ThrowAttack:
    case Act::KO:
      return true;
    case Act::Move:
      return any_span(move_of(d, f)->strike_invuln, f.move_frame);
    default:
      return false;
  }
}

bool throw_invulnerable(const MatchState& s, const GameData& d, const FighterState& f) {
  (void)s;
  if (is_airborne(d, f)) return true;
  switch (f.act) {
    case Act::Hitstun:
    case Act::Blockstun:
    case Act::Juggle:
    case Act::Knockdown:
    case Act::Wakeup:
    case Act::Thrown:
    case Act::ThrowAttack:
    case Act::TechRecover:
    case Act::KO:
      return true;
    case Act::Move:
      return any_span(move_of(d, f)->throw_invuln, f.move_frame);
    default:
      // Proteção contra arremesso logo após levantar.
      return f.throw_protect > 0;
  }
}

// Contato detectado. Tudo o que depende do estado do defensor é calculado na detecção, antes de
// aplicar qualquer resultado, para que trocas (os dois acertando no mesmo frame) sejam simétricas.
struct Contact {
  bool strike = false;
  bool throw_ = false;
  int segment = -1;
  i16 move = -1;
  bool counter = false;
  bool punish = false;
  bool in_combo = false;
  bool armored = false;
  bool defender_air = false;
  bool defender_crouch = false;
  i32 x = 0;
  i32 y = 0;
};

Contact detect(const MatchState& s, const GameData& d, int a) {
  Contact c;
  const FighterState& at = s.f[a];
  const FighterState& df = s.f[1 - a];
  if (at.act != Act::Move || at.hitstop > 0) return c;
  const MoveDef& m = *move_of(d, at);
  const i16 fr = at.move_frame;

  WorldBox hurt[8];
  int nh = 0;
  collect_hurtboxes(d, df, hurt, nh);

  if (m.kind == MoveKind::Throw) {
    if (throw_invulnerable(s, d, df)) return c;
    const WorldBox push = to_world(push_box(d, df), df.x, df.y, df.facing);
    for (const auto& b : m.boxes) {
      if (b.kind != BoxKind::Throw || !in_span(fr, b.from, b.to)) continue;
      const WorldBox tb = to_world(b.box, at.x, at.y, at.facing);
      if (overlaps(tb, push)) {
        c.throw_ = true;
        c.x = df.x;
        c.y = 0;
        return c;
      }
    }
    return c;
  }

  for (size_t i = 0; i < m.hits.size(); ++i) {
    const HitSegment& h = m.hits[i];
    if (!in_span(fr, h.from, h.to) || (at.hits_done & (1u << i))) continue;
    if (strike_invulnerable(d, df)) return c;
    const bool def_air = is_airborne(d, df);
    if (def_air && !h.hits_airborne) continue;
    if (df.act == Act::Juggle && df.juggle + h.juggle_cost > d.rules.juggle_limit) continue;
    for (const auto& b : m.boxes) {
      if (b.kind != BoxKind::Hit || !in_span(fr, b.from, b.to)) continue;
      const WorldBox hb = to_world(b.box, at.x, at.y, at.facing);
      for (int k = 0; k < nh; ++k) {
        if (overlaps(hb, hurt[k])) {
          c.strike = true;
          c.segment = static_cast<int>(i);
          c.move = at.move;
          if (df.act == Act::Move) {
            const MoveDef& dm = *move_of(d, df);
            if (df.move_frame <= dm.last_active()) c.counter = true;
            else c.punish = true;
            for (const auto& ar : dm.armor) {
              if (in_span(df.move_frame, ar.from, ar.to) && df.armor_used < ar.hits) c.armored = true;
            }
          } else if (df.act == Act::Landing) {
            c.punish = true;
          }
          c.in_combo = df.act == Act::Hitstun || df.act == Act::Juggle || df.combo_hits > 0;
          c.defender_air = def_air;
          c.defender_crouch = is_crouching(d, df);
          c.x = (hb.left > hurt[k].left ? hb.left : hurt[k].left) / 2 + (hb.right < hurt[k].right ? hb.right : hurt[k].right) / 2;
          c.y = (hb.bottom > hurt[k].bottom ? hb.bottom : hurt[k].bottom) / 2 + (hb.top < hurt[k].top ? hb.top : hurt[k].top) / 2;
          return c;
        }
      }
    }
  }
  return c;
}

// Pode bloquear? `raw` é o input bruto do defensor neste frame.
bool can_block(const GameData& d, const FighterState& df, const FighterState& at, InputBits raw, HitLevel level) {
  switch (df.act) {
    case Act::Idle:
    case Act::Crouch:
    case Act::WalkBack:
    case Act::WalkForward:
    case Act::Guard:
    case Act::Blockstun:
      break;
    default:
      return false;
  }
  if (is_airborne(d, df)) return false;
  // "Trás" é relativo à posição do atacante (necessário para cross-ups).
  const i32 side = at.x > df.x ? 1 : (at.x < df.x ? -1 : -df.facing);
  const bool back = side > 0 ? (raw & btn::Left) && !(raw & btn::Right) : (raw & btn::Right) && !(raw & btn::Left);
  if (!back) return false;
  const bool crouch = (raw & btn::Down) && !(raw & btn::Up);
  if (level == HitLevel::Low && !crouch) return false;
  if (level == HitLevel::Overhead && crouch) return false;
  return true;
}

i32 scaled(i32 value, i32 permil) { return static_cast<i32>((static_cast<i64>(value) * permil) / 1000); }

void begin_hitstop(FighterState& at, FighterState& df, i16 frames, const Rules& r) {
  const i16 h = frames > r.max_hitstop ? r.max_hitstop : frames;
  at.hitstop = h;
  df.hitstop = h;
  // Inputs feitos durante o hitstop continuam válidos quando ele termina (cancelamentos e reversals).
  at.buffer_bonus = h;
  df.buffer_bonus = h;
}

void push_apart(MatchState& s, int a, i32 distance) {
  // Defensor é empurrado para longe do atacante.
  FighterState& at = s.f[a];
  FighterState& df = s.f[1 - a];
  const i32 dir = df.x > at.x ? 1 : (df.x < at.x ? -1 : at.facing);
  df.push_remaining = dir * distance;
}

void ko_check(MatchState& s, const GameData& d, int victim, StepEvents* ev) {
  FighterState& f = s.f[victim];
  if (f.health > 0) return;
  if (s.options & option::NoRoundEnd) {
    f.health = 1;
    return;
  }
  f.health = 0;
  f.act = Act::KO;
  f.move = -1;
  f.vy = f.y > 0 ? 0 : 400;
  if (f.y <= 0) f.y = 1;
  f.gravity = d.rules.juggle_gravity;
  const i32 dir = f.x > s.f[1 - victim].x ? 1 : -1;
  f.vx = dir * 150;
  if (ev) ev->add({EventType::KO, static_cast<i8>(1 - victim), -1, f.x, f.y, 0});
}

void apply_strike(MatchState& s, const GameData& d, int a, const Contact& c, const InputBits raw[2],
                  StepEvents* ev) {
  const Rules& r = d.rules;
  FighterState& at = s.f[a];
  FighterState& df = s.f[1 - a];
  const MoveDef& m = char_of(d, at).moves[static_cast<size_t>(c.move)];
  const HitSegment& h = m.hits[static_cast<size_t>(c.segment)];
  at.hits_done = static_cast<u8>(at.hits_done | (1u << c.segment));
  at.move_connected = true;
  const i16 hitstop = static_cast<i16>(c.segment > 0 ? scaled(h.hitstop, r.multi_hit_hitstop_permil) : h.hitstop);
  const i32 max_meter_v = max_meter(r);

  // Armadura: absorve o golpe (metade do dano, sem hitstun).
  if (c.armored) {
    ++df.armor_used;
    const i32 dmg = scaled(h.damage, r.scaling_permil[0]) / 2;
    df.health -= dmg;
    begin_hitstop(at, df, hitstop, r);
    if (ev) ev->add({EventType::Armor, static_cast<i8>(a), c.move, c.x, c.y, dmg});
    ko_check(s, d, 1 - a, ev);
    return;
  }

  if (can_block(d, df, at, raw[1 - a], h.level)) {
    df.act = Act::Blockstun;
    df.crouch_stun = (raw[1 - a] & btn::Down) != 0;
    df.timer = h.blockstun;
    df.vx = 0;
    df.move = -1;
    if (h.chip > 0) {
      const i32 chip = scaled(h.chip, at.damage_permil_override);
      df.health = df.health - chip < 1 ? (df.health > 1 ? 1 : df.health) : df.health - chip;
    }
    push_apart(s, a, h.pushback_block);
    begin_hitstop(at, df, hitstop, r);
    at.meter = clamp_value(at.meter + m.meter_gain_block, 0, max_meter_v);
    df.meter = clamp_value(df.meter + scaled(m.meter_gain_block, r.meter_gain_on_damaged_permil), 0, max_meter_v);
    add_mare(s, r, a, r.mare.attack_blocked);
    if (ev) ev->add({EventType::Block, static_cast<i8>(a), c.move, c.x, c.y, h.blockstun});
    return;
  }

  const bool counter = c.counter;
  const bool punish = c.punish;
  if (!c.in_combo) {
    df.combo_hits = 0;
    df.combo_damage = 0;
    df.juggle = 0;
    df.combo_scaling_extra = m.starter_scaling_permil;
    df.combo_counter_hit = counter || punish;
  }
  const size_t idx = static_cast<size_t>(df.combo_hits);
  i32 scaling = r.scaling_permil[idx < r.scaling_permil.size() ? idx : r.scaling_permil.size() - 1];
  scaling -= df.combo_scaling_extra;
  const i32 floor_scaling = m.min_scaling_permil > r.min_scaling_permil ? m.min_scaling_permil : r.min_scaling_permil;
  if (scaling < floor_scaling) scaling = floor_scaling;
  i32 damage = scaled(h.damage, scaling);
  if (counter || punish) damage = scaled(damage, r.counter_damage_permil);
  damage = scaled(damage, at.damage_permil_override);
  const CharacterDef& dch = char_of(d, df);
  const i32 cap = scaled(dch.health, r.max_combo_damage_permil);
  if (df.combo_damage + damage > cap) damage = cap - df.combo_damage;
  if (damage < 0) damage = 0;

  i16 hitstun = h.hitstun;
  if (punish) hitstun = static_cast<i16>(hitstun + r.punish_counter_bonus);
  else if (counter) hitstun = static_cast<i16>(hitstun + r.counter_hit_bonus);
  if (df.combo_hits + 1 >= r.hitstun_decay_start) {
    hitstun = static_cast<i16>(hitstun - (df.combo_hits + 2 - r.hitstun_decay_start));
  }
  if (hitstun < r.hitstun_min) hitstun = r.hitstun_min;

  const bool was_air = c.defender_air;
  const bool was_crouch = c.defender_crouch;
  df.move = -1;
  df.move_frame = 0;
  df.health -= damage;
  df.combo_hits = static_cast<i16>(df.combo_hits + 1);
  df.combo_damage += damage;
  const i32 away = df.x > at.x ? 1 : (df.x < at.x ? -1 : at.facing);

  if (was_air || h.launch) {
    df.act = Act::Juggle;
    df.juggle = static_cast<i16>(df.juggle + h.juggle_cost);
    df.vy = h.launch ? h.launch_vy : r.air_hit_vy;
    df.vx = away * (h.launch ? h.launch_vx : r.air_hit_vx);
    df.gravity = r.juggle_gravity;
    if (df.y <= 0) df.y = 1;
  } else if (h.knockdown) {
    df.act = Act::Knockdown;
    df.timer = r.knockdown_frames;
    df.vx = 0;
    if (ev) ev->add({EventType::Knockdown, static_cast<i8>(a), c.move, df.x, 0, 0});
    push_apart(s, a, h.pushback_hit);
  } else {
    df.act = Act::Hitstun;
    df.timer = hitstun;
    df.crouch_stun = was_crouch;
    df.vx = 0;
    push_apart(s, a, h.pushback_hit);
  }
  begin_hitstop(at, df, hitstop, r);

  at.meter = clamp_value(at.meter + m.meter_gain_hit, 0, max_meter_v);
  df.meter = clamp_value(df.meter + scaled(m.meter_gain_hit, r.meter_gain_on_damaged_permil), 0, max_meter_v);
  add_mare(s, r, a, punish ? r.mare.punish_counter : (counter ? r.mare.counter_hit : r.mare.attack_hit));

  if (ev) {
    const EventType t = punish ? EventType::PunishCounter : (counter ? EventType::CounterHit : EventType::Hit);
    ev->add({t, static_cast<i8>(a), c.move, c.x, c.y, damage});
  }
  ko_check(s, d, 1 - a, ev);
}

void apply_throw_connect(MatchState& s, const GameData& d, int a, StepEvents* ev) {
  FighterState& at = s.f[a];
  FighterState& df = s.f[1 - a];
  at.act = Act::ThrowAttack;
  at.throw_move = at.move;
  const MoveDef& tm = char_of(d, at).moves[static_cast<size_t>(at.move)];
  // Arremessos de comando não abrem janela de tech: concluem no próximo frame.
  const i16 window = tm.techable ? d.rules.throw_tech_window : static_cast<i16>(1);
  at.throw_timer = window;
  at.vx = 0;
  df.act = Act::Thrown;
  df.throw_timer = window;
  df.move = -1;
  df.vx = 0;
  if (ev) ev->add({EventType::ThrowConnect, static_cast<i8>(a), at.throw_move, df.x, 0, 0});
}

void tech(MatchState& s, const GameData& d, StepEvents* ev, int who) {
  for (int p = 0; p < 2; ++p) {
    FighterState& f = s.f[p];
    f.act = Act::TechRecover;
    f.timer = d.rules.throw_tech_separation;
    f.move = -1;
    f.throw_timer = 0;
    f.vx = 0;
  }
  const i32 dir = s.f[1].x >= s.f[0].x ? 1 : -1;
  s.f[0].push_remaining = -dir * d.rules.throw_tech_push / 2;
  s.f[1].push_remaining = dir * d.rules.throw_tech_push / 2;
  if (ev) ev->add({EventType::ThrowTech, static_cast<i8>(who), -1, (s.f[0].x + s.f[1].x) / 2, 0, 0});
}

// Tech = LP+LK (com até 1 frame entre os botões). Aceita pressões feitas até `buffer` frames antes,
// inclusive a pressão que iniciou o próprio arremesso do defensor.
bool tech_pressed(const FighterState& f, i32 buffer) {
  constexpr u16 pair_mask = btn::LP | btn::LK;
  for (u32 ago = 0; ago < static_cast<u32>(buffer); ++ago) {
    const u16 held = f.input.btn_at(ago);
    const u16 pressed = f.input.pressed_at(ago);
    const u16 recent = static_cast<u16>(pressed | f.input.pressed_at(ago + 1));
    if ((held & pair_mask) == pair_mask && (pressed & pair_mask) != 0 && (recent & pair_mask) == pair_mask) {
      return true;
    }
  }
  return false;
}

void update_throws(MatchState& s, const GameData& d, StepEvents* ev) {
  for (int a = 0; a < 2; ++a) {
    FighterState& at = s.f[a];
    FighterState& df = s.f[1 - a];
    if (at.act != Act::ThrowAttack || df.act != Act::Thrown) continue;
    const MoveDef& tm = char_of(d, at).moves[static_cast<size_t>(at.throw_move)];
    const i32 elapsed = d.rules.throw_tech_window - df.throw_timer;
    if (tm.techable && tech_pressed(df, elapsed + 1 + d.rules.button_buffer)) {
      tech(s, d, ev, 1 - a);
      return;
    }
    --at.throw_timer;
    --df.throw_timer;
    if (at.throw_timer > 0) continue;
    const CharacterDef& ch = char_of(d, at);
    const MoveDef& m = ch.moves[static_cast<size_t>(at.throw_move)];
    const i32 damage = scaled(m.throw_damage, at.damage_permil_override);
    df.health -= damage;
    if (m.throw_swaps_sides) {
      // Arremesso para trás: o defensor termina do outro lado.
      df.x = at.x - at.facing * m.throw_end_distance;
    } else {
      df.x = at.x + at.facing * m.throw_end_distance;
    }
    df.x = clamp_value(df.x, -d.rules.stage_half_width, d.rules.stage_half_width);
    df.act = Act::Knockdown;
    df.timer = d.rules.knockdown_frames;
    df.combo_hits = 0;
    df.combo_damage = 0;
    df.juggle = 0;
    at.act = Act::Move;
    at.move = at.throw_move;
    at.move_frame = m.last_active();
    at.move_connected = true;
    add_mare(s, d.rules, a, d.rules.mare.throw_landed);
    if (ev) ev->add({EventType::ThrowComplete, static_cast<i8>(a), at.throw_move, df.x, 0, damage});
    ko_check(s, d, 1 - a, ev);
  }
}

void resolve_contacts(MatchState& s, const GameData& d, const InputBits raw[2], StepEvents* ev) {
  const Contact c0 = detect(s, d, 0);
  const Contact c1 = detect(s, d, 1);
  // Regras de prioridade:
  // - golpe contra golpe no mesmo frame: troca (os dois acertam);
  // - golpe contra arremesso: o golpe vence;
  // - arremesso contra arremesso: os dois fazem tech.
  if (c0.throw_ && c1.throw_) {
    tech(s, d, ev, -1);
    return;
  }
  const bool s0 = c0.strike;
  const bool s1 = c1.strike;
  if (s0) apply_strike(s, d, 0, c0, raw, ev);
  if (s1) apply_strike(s, d, 1, c1, raw, ev);
  if (c0.throw_ && !s1) apply_throw_connect(s, d, 0, ev);
  if (c1.throw_ && !s0) apply_throw_connect(s, d, 1, ev);
}

// Separa as pushboxes e mantém os lutadores dentro da arena e da câmera.
void resolve_positions(MatchState& s, const GameData& d, const i32 prev_x[2]) {
  const Rules& r = d.rules;
  FighterState& a = s.f[0];
  FighterState& b = s.f[1];
  const bool locked = a.act == Act::ThrowAttack || a.act == Act::Thrown;
  if (!locked && a.act != Act::KO && b.act != Act::KO) {
    const WorldBox pa = to_world(push_box(d, a), a.x, a.y, a.facing);
    const WorldBox pb = to_world(push_box(d, b), b.x, b.y, b.facing);
    if (overlaps(pa, pb)) {
      const i32 overlap = (pa.right < pb.right ? pa.right : pb.right) - (pa.left > pb.left ? pa.left : pb.left);
      // Quem está à esquerda vai para a esquerda; empate desfeito pelo lado anterior.
      i32 dir = a.x < b.x ? -1 : (a.x > b.x ? 1 : (prev_x[0] <= prev_x[1] ? -1 : 1));
      const i32 half = overlap / 2;
      a.x += dir * half;
      b.x -= dir * (overlap - half);
    }
  }
  const i32 wall = r.stage_half_width;
  for (int p = 0; p < 2; ++p) {
    FighterState& f = s.f[p];
    f.x = clamp_value(f.x, -wall, wall);
  }
  // Limite de câmera: quem se afastou volta.
  const i32 dist = b.x > a.x ? b.x - a.x : a.x - b.x;
  if (dist > r.max_distance) {
    const i32 excess = dist - r.max_distance;
    const i32 moved0 = (a.x - prev_x[0]) * (a.x < b.x ? -1 : 1);  // positivo = afastando
    const i32 moved1 = (b.x - prev_x[1]) * (b.x < a.x ? -1 : 1);
    const int fix = moved0 >= moved1 ? 0 : 1;
    FighterState& f = s.f[fix];
    const FighterState& o = s.f[1 - fix];
    f.x += (f.x < o.x ? 1 : -1) * excess;
  }
  // Se um lutador ficou preso na parede ainda sobreposto, o outro é afastado por completo.
  if (!locked && a.act != Act::KO && b.act != Act::KO) {
    const WorldBox pa = to_world(push_box(d, a), a.x, a.y, a.facing);
    const WorldBox pb = to_world(push_box(d, b), b.x, b.y, b.facing);
    if (overlaps(pa, pb)) {
      const i32 overlap = (pa.right < pb.right ? pa.right : pb.right) - (pa.left > pb.left ? pa.left : pb.left);
      const bool a_wall = a.x <= -wall || a.x >= wall;
      FighterState& mover = a_wall ? b : a;
      const FighterState& anchor = a_wall ? a : b;
      mover.x += (mover.x < anchor.x ? -1 : (mover.x > anchor.x ? 1 : (anchor.x > 0 ? -1 : 1))) * overlap;
      mover.x = clamp_value(mover.x, -wall, wall);
    }
  }
}

void update_facing(MatchState& s, const GameData& d) {
  for (int p = 0; p < 2; ++p) {
    FighterState& f = s.f[p];
    const FighterState& o = s.f[1 - p];
    const bool can_turn = is_ground_neutral(f.act) || f.act == Act::Landing || f.act == Act::PreJump ||
                          f.act == Act::Wakeup;
    if (!can_turn || is_airborne(d, f)) continue;
    if (o.x > f.x) f.facing = 1;
    else if (o.x < f.x) f.facing = -1;
  }
}

void update_mare_passive(MatchState& s, const GameData& d) {
  const MareRules& mr = d.rules.mare;
  for (int p = 0; p < 2; ++p) {
    FighterState& f = s.f[p];
    const FighterState& o = s.f[1 - p];
    if (f.act == Act::WalkForward || f.act == Act::Run) add_mare(s, d.rules, p, mr.walk_forward_per_frame);
    const bool retreating = f.act == Act::WalkBack ||
                            (f.act == Act::Move && f.move == char_of(d, f).move_backdash);
    if (retreating) {
      if (f.retreat_frames < 30000) ++f.retreat_frames;
      if (f.retreat_frames > mr.retreat_grace_frames) add_mare(s, d.rules, p, -mr.retreat_per_frame);
    } else if (f.act != Act::Blockstun) {
      f.retreat_frames = 0;
    }
    const i32 wall = d.rules.stage_half_width;
    const bool opp_cornered = o.x <= -wall + d.rules.corner_zone || o.x >= wall - d.rules.corner_zone;
    const bool self_cornered = f.x <= -wall + d.rules.corner_zone || f.x >= wall - d.rules.corner_zone;
    if (opp_cornered && !self_cornered) add_mare(s, d.rules, p, mr.corner_per_frame);
  }
}

void reset_round(MatchState& s, const GameData& d) {
  for (int p = 0; p < 2; ++p) {
    FighterState& f = s.f[p];
    const CharacterDef& ch = char_of(d, f);
    enter_idle(f);
    f.x = (p == 0 ? -1 : 1) * d.rules.start_distance / 2;
    f.y = 0;
    f.facing = static_cast<i8>(p == 0 ? 1 : -1);
    f.health = ch.health;
    f.hitstop = 0;
    f.push_remaining = 0;
    f.combo_hits = 0;
    f.combo_damage = 0;
    f.juggle = 0;
    f.retreat_frames = 0;
    f.throw_timer = 0;
    f.consumed_press = f.input.head;
  }
  s.mare = 0;
  s.freeze = 0;
  s.freeze_owner = -1;
  s.timer_frames = d.rules.round_seconds * kTicksPerSecond;
  s.round_winner = -1;
}

void end_round(MatchState& s, const GameData& d, int winner, StepEvents* ev) {
  s.phase = Phase::RoundEnd;
  s.phase_timer = d.rules.round_end_frames;
  s.round_winner = static_cast<i8>(winner);
  if (winner == 0 || winner == 1) ++s.wins[winner];
  if (winner == 2) {
    ++s.wins[0];
    ++s.wins[1];
  }
  if (ev) ev->add({EventType::RoundEnd, static_cast<i8>(winner), -1, 0, 0, s.round});
}

}  // namespace

// ---------------------------------------------------------------------------
// API pública
// ---------------------------------------------------------------------------

i32 max_meter(const Rules& r) { return r.meter_per_bar * r.meter_bars; }

i32 mare_favor(const MatchState& s, int player) { return player == 0 ? s.mare : -s.mare; }

const MoveDef* current_move(const MatchState& s, const GameData& d, int player) {
  return move_of(d, s.f[player]);
}

bool is_actionable(const MatchState& s, const GameData& d, int p) {
  const FighterState& f = s.f[p];
  if (f.hitstop > 0) return false;
  if (is_ground_neutral(f.act)) return true;
  if (f.act == Act::Move) {
    const MoveDef& m = *move_of(d, f);
    return f.move_frame >= m.total_frames() && !m.airborne && f.y <= 0;
  }
  if (f.act == Act::Hitstun || f.act == Act::Blockstun || f.act == Act::Landing || f.act == Act::TechRecover ||
      f.act == Act::Wakeup) {
    return f.timer <= 0;
  }
  return false;
}

void init_match(MatchState& s, const GameData& d, const MatchSetup& setup) {
  s = MatchState{};
  s.rng = setup.seed == 0 ? 0x9E3779B9u : setup.seed;
  s.options = setup.options;
  for (int p = 0; p < 2; ++p) {
    s.f[p] = FighterState{};
    s.f[p].character = setup.character[p];
    s.f[p].scheme = setup.scheme[p];
    s.f[p].meter = 0;
  }
  reset_round(s, d);
  if (setup.options & option::SkipIntro) {
    s.phase = Phase::Fight;
    s.phase_timer = 0;
  } else {
    s.phase = Phase::Intro;
    s.phase_timer = d.rules.round_intro_frames;
  }
}

void step(MatchState& s, const GameData& d, const FrameInput& in, StepEvents* ev) {
  if (ev) ev->clear();
  ++s.frame;
  // Avança o RNG uma vez por frame (reservado para elementos futuros; nunca usa relógio).
  s.rng ^= s.rng << 13;
  s.rng ^= s.rng >> 17;
  s.rng ^= s.rng << 5;

  const InputBits raw[2] = {in.p[0], in.p[1]};
  for (int p = 0; p < 2; ++p) {
    FighterState& f = s.f[p];
    f.input.push(numpad_direction(raw[p], f.facing), raw[p]);
  }

  switch (s.phase) {
    case Phase::Intro:
      if (--s.phase_timer <= 0) {
        s.phase = Phase::Fight;
        for (int p = 0; p < 2; ++p) s.f[p].consumed_press = s.f[p].input.head;
        if (ev) ev->add({EventType::Fight, -1, -1, 0, 0, s.round});
      }
      return;
    case Phase::RoundEnd:
      for (int p = 0; p < 2; ++p) {
        if (s.f[p].hitstop > 0) {
          --s.f[p].hitstop;
          continue;
        }
        if (s.f[p].act == Act::KO || s.f[p].act == Act::Juggle || s.f[p].act == Act::Air) advance(s, d, p, nullptr);
      }
      if (--s.phase_timer <= 0) {
        if (s.wins[0] >= d.rules.rounds_to_win || s.wins[1] >= d.rules.rounds_to_win) {
          s.phase = Phase::MatchEnd;
          if (s.wins[0] >= d.rules.rounds_to_win && s.wins[1] >= d.rules.rounds_to_win) s.match_winner = 2;
          else s.match_winner = static_cast<i8>(s.wins[0] >= d.rules.rounds_to_win ? 0 : 1);
          if (ev) ev->add({EventType::MatchEnd, s.match_winner, -1, 0, 0, 0});
        } else {
          ++s.round;
          reset_round(s, d);
          s.phase = Phase::Intro;
          s.phase_timer = d.rules.round_intro_frames;
          if (ev) ev->add({EventType::RoundStart, -1, -1, 0, 0, s.round});
        }
      }
      return;
    case Phase::MatchEnd:
      return;
    case Phase::Fight:
      break;
  }

  if (s.freeze > 0) {
    // Congelamento do super: só o tempo de freeze passa.
    --s.freeze;
    if (s.freeze == 0) s.freeze_owner = -1;
    return;
  }

  bool frozen[2];
  for (int p = 0; p < 2; ++p) {
    frozen[p] = s.f[p].hitstop > 0;
    if (frozen[p]) --s.f[p].hitstop;
  }
  const i32 prev_x[2] = {s.f[0].x, s.f[1].x};

  for (int p = 0; p < 2; ++p) {
    if (!frozen[p]) decide(s, d, p, ev);
  }
  if (s.freeze > 0) return;  // um super começou: a ação congela imediatamente
  for (int p = 0; p < 2; ++p) {
    if (!frozen[p]) advance(s, d, p, ev);
    if (!frozen[p]) apply_push(s, d.rules, p);
  }
  update_throws(s, d, ev);
  resolve_positions(s, d, prev_x);
  update_facing(s, d);
  if (s.phase == Phase::Fight) resolve_contacts(s, d, raw, ev);
  update_mare_passive(s, d);

  // Treino: restaura a vida quando o combo termina.
  if (s.options & option::NoRoundEnd) {
    for (int p = 0; p < 2; ++p) {
      FighterState& f = s.f[p];
      if (is_actionable(s, d, p) && f.combo_hits == 0) f.health = char_of(d, f).health;
    }
  }

  // Fim de round
  const bool ko0 = s.f[0].act == Act::KO;
  const bool ko1 = s.f[1].act == Act::KO;
  if (ko0 || ko1) {
    end_round(s, d, ko0 && ko1 ? 2 : (ko1 ? 0 : 1), ev);
    return;
  }
  if (!(s.options & option::NoTimer)) {
    if (s.timer_frames > 0) --s.timer_frames;
    if (s.timer_frames == 0) {
      const i64 h0 = static_cast<i64>(s.f[0].health) * 1000 / char_of(d, s.f[0]).health;
      const i64 h1 = static_cast<i64>(s.f[1].health) * 1000 / char_of(d, s.f[1]).health;
      if (ev) ev->add({EventType::TimeOver, -1, -1, 0, 0, 0});
      end_round(s, d, h0 == h1 ? 2 : (h0 > h1 ? 0 : 1), ev);
    }
  }
}

void debug_start_move(MatchState& s, const GameData& d, int player, i16 move) {
  start_move(s, d, player, move, 0, 1000, nullptr);
}

void debug_boxes(const MatchState& s, const GameData& d, int player, DebugBoxes& out) {
  out.count = 0;
  const FighterState& f = s.f[player];
  auto add = [&](const WorldBox& w, BoxKind k, bool push) {
    if (out.count >= DebugBoxes::kMax) return;
    out.boxes[out.count] = w;
    out.kinds[out.count] = k;
    out.push[out.count] = push;
    ++out.count;
  };
  WorldBox hurt[8];
  int nh = 0;
  collect_hurtboxes(d, f, hurt, nh);
  for (int i = 0; i < nh; ++i) add(hurt[i], BoxKind::Hurt, false);
  add(to_world(push_box(d, f), f.x, f.y, f.facing), BoxKind::Hurt, true);
  if (const MoveDef* m = move_of(d, f)) {
    for (const auto& b : m->boxes) {
      if (b.kind != BoxKind::Hurt && in_span(f.move_frame, b.from, b.to)) add(to_world(b.box, f.x, f.y, f.facing), b.kind, false);
    }
  }
}

}  // namespace fg
