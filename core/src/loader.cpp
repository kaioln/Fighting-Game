#include "fg/loader.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <sstream>

#include "fg/command.h"
#include "fg/serialize.h"

namespace fg {

std::string read_text_file(const std::string& path, bool& ok) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    ok = false;
    return {};
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  ok = true;
  return ss.str();
}

namespace {

// Leitor com contexto: acumula erros em vez de abortar no primeiro.
class Reader {
 public:
  Reader(std::vector<std::string>& errors, std::string ctx) : errors_(errors), ctx_(std::move(ctx)) {}

  void error(const std::string& msg) const { errors_.push_back(ctx_ + ": " + msg); }
  Reader sub(const std::string& name) const { return Reader(errors_, ctx_ + "." + name); }
  const std::string& ctx() const { return ctx_; }

  template <typename T>
  bool get_int(const json::Value& obj, const char* key, T& out, bool required = false,
               i64 lo = std::numeric_limits<T>::min(), i64 hi = std::numeric_limits<T>::max()) const {
    const json::Value* v = obj.find(key);
    if (v == nullptr) {
      if (required) error(std::string("campo obrigatório '") + key + "' ausente");
      return false;
    }
    if (!v->is_int()) {
      error(std::string("'") + key + "' deve ser inteiro");
      return false;
    }
    const i64 x = v->as_int();
    if (x < lo || x > hi) {
      error(std::string("'") + key + "' fora do intervalo [" + std::to_string(lo) + ", " + std::to_string(hi) + "]");
      return false;
    }
    out = static_cast<T>(x);
    return true;
  }

  bool get_bool(const json::Value& obj, const char* key, bool& out) const {
    const json::Value* v = obj.find(key);
    if (v == nullptr) return false;
    if (!v->is_bool()) {
      error(std::string("'") + key + "' deve ser booleano");
      return false;
    }
    out = v->as_bool();
    return true;
  }

  bool get_string(const json::Value& obj, const char* key, std::string& out, bool required = false) const {
    const json::Value* v = obj.find(key);
    if (v == nullptr) {
      if (required) error(std::string("campo obrigatório '") + key + "' ausente");
      return false;
    }
    if (!v->is_string()) {
      error(std::string("'") + key + "' deve ser texto");
      return false;
    }
    out = v->as_string();
    return true;
  }

  bool get_box(const json::Value& obj, const char* key, Box& out, bool required = false) const {
    const json::Value* v = obj.find(key);
    if (v == nullptr) {
      if (required) error(std::string("caixa obrigatória '") + key + "' ausente");
      return false;
    }
    return parse_box(*v, out, key);
  }

  bool parse_box(const json::Value& v, Box& out, const std::string& what) const {
    if (!v.is_array() || v.items().size() != 4) {
      error("'" + what + "' deve ser [x, y, largura, altura]");
      return false;
    }
    i32 vals[4];
    for (size_t i = 0; i < 4; ++i) {
      if (!v.items()[i].is_int()) {
        error("'" + what + "' deve conter inteiros");
        return false;
      }
      vals[i] = static_cast<i32>(v.items()[i].as_int());
    }
    if (vals[2] <= 0 || vals[3] <= 0) {
      error("'" + what + "' precisa de largura e altura positivas");
      return false;
    }
    out = Box{vals[0], vals[1], vals[2], vals[3]};
    return true;
  }

  bool parse_span(const json::Value& v, i16& from, i16& to, const std::string& what) const {
    if (!v.is_array() || v.items().size() != 2 || !v.items()[0].is_int() || !v.items()[1].is_int()) {
      error("'" + what + "' deve ser [de, até]");
      return false;
    }
    const i64 a = v.items()[0].as_int();
    const i64 b = v.items()[1].as_int();
    if (a < 1 || b < a || b > 600) {
      error("'" + what + "' intervalo inválido [" + std::to_string(a) + ", " + std::to_string(b) + "]");
      return false;
    }
    from = static_cast<i16>(a);
    to = static_cast<i16>(b);
    return true;
  }

  void check_unknown(const json::Value& obj, std::initializer_list<const char*> known) const {
    for (const auto& [k, v] : obj.members()) {
      (void)v;
      bool found = false;
      for (const char* name : known) {
        if (k == name) {
          found = true;
          break;
        }
      }
      if (!found) error("campo desconhecido '" + k + "'");
    }
  }

 private:
  std::vector<std::string>& errors_;
  std::string ctx_;
};

bool parse_kind(const std::string& s, MoveKind& k) {
  static const std::pair<const char*, MoveKind> table[] = {
      {"normal", MoveKind::Normal},     {"command_normal", MoveKind::CommandNormal},
      {"special", MoveKind::Special},   {"enhanced", MoveKind::Enhanced},
      {"super", MoveKind::Super},       {"throw", MoveKind::Throw},
      {"movement", MoveKind::Movement}, {"mare", MoveKind::Mare},
  };
  for (const auto& [name, kind] : table) {
    if (s == name) {
      k = kind;
      return true;
    }
  }
  return false;
}

bool parse_level(const std::string& s, HitLevel& l) {
  if (s == "mid") l = HitLevel::Mid;
  else if (s == "low") l = HitLevel::Low;
  else if (s == "overhead") l = HitLevel::Overhead;
  else return false;
  return true;
}

void load_spans(const Reader& r, const json::Value& obj, const char* key, std::vector<FrameSpan>& out) {
  const json::Value* v = obj.find(key);
  if (v == nullptr) return;
  if (!v->is_array()) {
    r.error(std::string("'") + key + "' deve ser lista de intervalos");
    return;
  }
  for (const auto& item : v->items()) {
    FrameSpan s;
    if (r.parse_span(item, s.from, s.to, key)) out.push_back(s);
  }
}

void load_hit(const Reader& r, const json::Value& h, HitSegment& out) {
  r.check_unknown(h, {"frames", "damage", "chip", "hitstun", "blockstun", "hitstop", "pushback_hit",
                      "pushback_block", "level", "knockdown", "launch", "launch_vx", "launch_vy",
                      "juggle_cost", "hits_airborne"});
  const json::Value* frames = h.find("frames");
  if (frames == nullptr) r.error("hit sem 'frames'");
  else r.parse_span(*frames, out.from, out.to, "frames");
  r.get_int(h, "damage", out.damage, true, 0, 100000);
  r.get_int(h, "chip", out.chip, false, 0, 100000);
  r.get_int(h, "hitstun", out.hitstun, true, 0, 300);
  r.get_int(h, "blockstun", out.blockstun, true, 0, 300);
  r.get_int(h, "hitstop", out.hitstop, true, 0, 60);
  r.get_int(h, "pushback_hit", out.pushback_hit, false, 0, 100000);
  r.get_int(h, "pushback_block", out.pushback_block, false, 0, 100000);
  std::string level = "mid";
  r.get_string(h, "level", level);
  if (!parse_level(level, out.level)) r.error("nível de golpe inválido '" + level + "' (mid, low, overhead)");
  r.get_bool(h, "knockdown", out.knockdown);
  r.get_bool(h, "launch", out.launch);
  r.get_int(h, "launch_vx", out.launch_vx, false, -10000, 10000);
  r.get_int(h, "launch_vy", out.launch_vy, false, 0, 10000);
  r.get_int(h, "juggle_cost", out.juggle_cost, false, 0, 20);
  r.get_bool(h, "hits_airborne", out.hits_airborne);
}

bool parse_cancel_into(const std::string& s, u8& mask) {
  if (s == "chain") mask |= cancel::Chain;
  else if (s == "special") mask |= cancel::Special;
  else if (s == "super") mask |= cancel::Super;
  else if (s == "jump") mask |= cancel::Jump;
  else if (s == "mare") mask |= cancel::Mare;
  else return false;
  return true;
}

void load_move(const Reader& r, const json::Value& m, MoveDef& out) {
  r.check_unknown(m, {"id", "kind", "startup", "active", "recovery", "landing_recovery", "airborne", "crouching",
                      "ends_on_landing", "hits", "boxes", "cancels", "chain", "strike_invuln", "throw_invuln",
                      "airborne_frames", "armor", "velocity", "throw_damage", "throw_swaps_sides", "techable",
                      "throw_end_distance", "meter_cost", "meter_gain_hit", "meter_gain_block", "meter_gain_whiff",
                      "starter_scaling_permil", "min_scaling_permil", "super_freeze", "animation", "vfx", "sfx",
                      "notes"});
  r.get_string(m, "id", out.id, true);
  std::string kind = "normal";
  r.get_string(m, "kind", kind);
  if (!parse_kind(kind, out.kind)) r.error("tipo de golpe inválido '" + kind + "'");
  r.get_int(m, "startup", out.startup, true, 1, 200);
  r.get_int(m, "active", out.active, true, 1, 200);
  r.get_int(m, "recovery", out.recovery, true, 0, 200);
  r.get_int(m, "landing_recovery", out.landing_recovery, false, 0, 60);
  r.get_bool(m, "airborne", out.airborne);
  r.get_bool(m, "crouching", out.crouching);
  r.get_bool(m, "ends_on_landing", out.ends_on_landing);

  if (const json::Value* hits = m.find("hits")) {
    if (!hits->is_array()) r.error("'hits' deve ser lista");
    else
      for (size_t i = 0; i < hits->items().size(); ++i) {
        HitSegment seg;
        load_hit(r.sub("hits[" + std::to_string(i) + "]"), hits->items()[i], seg);
        out.hits.push_back(seg);
      }
  }
  if (const json::Value* boxes = m.find("boxes")) {
    if (!boxes->is_array()) r.error("'boxes' deve ser lista");
    else
      for (size_t i = 0; i < boxes->items().size(); ++i) {
        const json::Value& b = boxes->items()[i];
        Reader br = r.sub("boxes[" + std::to_string(i) + "]");
        br.check_unknown(b, {"frames", "kind", "box"});
        BoxSpan span;
        const json::Value* frames = b.find("frames");
        if (frames == nullptr) br.error("caixa sem 'frames'");
        else br.parse_span(*frames, span.from, span.to, "frames");
        std::string k;
        br.get_string(b, "kind", k, true);
        if (k == "hit") span.kind = BoxKind::Hit;
        else if (k == "hurt") span.kind = BoxKind::Hurt;
        else if (k == "throw") span.kind = BoxKind::Throw;
        else br.error("tipo de caixa inválido '" + k + "' (hit, hurt, throw)");
        br.get_box(b, "box", span.box, true);
        out.boxes.push_back(span);
      }
  }
  if (const json::Value* cancels = m.find("cancels")) {
    if (!cancels->is_array()) r.error("'cancels' deve ser lista");
    else
      for (size_t i = 0; i < cancels->items().size(); ++i) {
        const json::Value& c = cancels->items()[i];
        Reader cr = r.sub("cancels[" + std::to_string(i) + "]");
        cr.check_unknown(c, {"frames", "into", "on_whiff"});
        CancelSpan span;
        const json::Value* frames = c.find("frames");
        if (frames == nullptr) cr.error("cancelamento sem 'frames'");
        else cr.parse_span(*frames, span.from, span.to, "frames");
        const json::Value* into = c.find("into");
        if (into == nullptr || !into->is_array()) {
          cr.error("'into' deve ser lista (chain, special, super, jump, mare)");
        } else {
          for (const auto& item : into->items()) {
            if (!item.is_string() || !parse_cancel_into(item.as_string(), span.mask)) {
              cr.error("destino de cancelamento inválido");
            }
          }
        }
        cr.get_bool(c, "on_whiff", span.on_whiff);
        out.cancels.push_back(span);
      }
  }
  if (const json::Value* chain = m.find("chain")) {
    if (!chain->is_array()) r.error("'chain' deve ser lista de ids");
    else
      for (const auto& item : chain->items()) {
        if (item.is_string()) out.chain_targets.push_back(item.as_string());
        else r.error("'chain' deve conter ids de golpes");
      }
  }
  load_spans(r, m, "strike_invuln", out.strike_invuln);
  load_spans(r, m, "throw_invuln", out.throw_invuln);
  load_spans(r, m, "airborne_frames", out.airborne_frames);
  if (const json::Value* armor = m.find("armor")) {
    if (!armor->is_array()) r.error("'armor' deve ser lista");
    else
      for (const auto& a : armor->items()) {
        ArmorSpan s;
        if (!a.is_object()) {
          r.error("'armor' deve conter objetos {frames, hits}");
          continue;
        }
        const json::Value* frames = a.find("frames");
        if (frames) r.parse_span(*frames, s.from, s.to, "armor.frames");
        r.get_int(a, "hits", s.hits, true, 1, 5);
        out.armor.push_back(s);
      }
  }
  if (const json::Value* vel = m.find("velocity")) {
    if (!vel->is_array()) r.error("'velocity' deve ser lista");
    else
      for (const auto& v : vel->items()) {
        VelocitySpan s;
        if (!v.is_object()) {
          r.error("'velocity' deve conter objetos");
          continue;
        }
        const json::Value* frames = v.find("frames");
        if (frames) r.parse_span(*frames, s.from, s.to, "velocity.frames");
        else r.error("velocity sem 'frames'");
        r.get_int(v, "vx", s.vx, false, -20000, 20000);
        r.get_int(v, "vy", s.vy, false, -20000, 20000);
        r.get_bool(v, "impulse", s.impulse);
        out.velocity.push_back(s);
      }
  }
  r.get_int(m, "throw_damage", out.throw_damage, false, 0, 100000);
  r.get_bool(m, "throw_swaps_sides", out.throw_swaps_sides);
  r.get_bool(m, "techable", out.techable);
  r.get_int(m, "throw_end_distance", out.throw_end_distance, false, 0, 100000);
  r.get_int(m, "meter_cost", out.meter_cost, false, 0, 100000);
  r.get_int(m, "meter_gain_hit", out.meter_gain_hit, false, 0, 10000);
  r.get_int(m, "meter_gain_block", out.meter_gain_block, false, 0, 10000);
  r.get_int(m, "meter_gain_whiff", out.meter_gain_whiff, false, 0, 10000);
  r.get_int(m, "starter_scaling_permil", out.starter_scaling_permil, false, 0, 1000);
  r.get_int(m, "min_scaling_permil", out.min_scaling_permil, false, 0, 1000);
  r.get_int(m, "super_freeze", out.super_freeze, false, 0, 120);
  r.get_string(m, "animation", out.animation);
  r.get_string(m, "vfx", out.vfx);
  r.get_string(m, "sfx", out.sfx);
}

bool parse_buttons(const std::string& s, CommandDef& c) {
  if (s == "P") c.trigger = TriggerButtons::AnyPunch;
  else if (s == "K") c.trigger = TriggerButtons::AnyKick;
  else if (s == "PP") c.trigger = TriggerButtons::TwoPunches;
  else if (s == "KK") c.trigger = TriggerButtons::TwoKicks;
  else {
    c.trigger = TriggerButtons::Exact;
    u16 mask = 0;
    std::stringstream ss(s);
    std::string part;
    while (std::getline(ss, part, '+')) {
      if (part == "LP") mask |= btn::LP;
      else if (part == "MP") mask |= btn::MP;
      else if (part == "HP") mask |= btn::HP;
      else if (part == "LK") mask |= btn::LK;
      else if (part == "MK") mask |= btn::MK;
      else if (part == "HK") mask |= btn::HK;
      else if (part == "SP") mask |= btn::SP;
      else return false;
    }
    if (mask == 0) return false;
    c.buttons = mask;
  }
  return true;
}

void load_command(const Reader& r, const json::Value& j, CommandDef& c) {
  r.check_unknown(j, {"id", "motion", "spin", "buttons", "window", "negative_edge", "scheme", "ground", "air",
                      "move", "priority", "simplified_damage_permil"});
  r.get_string(j, "id", c.id, true);
  if (const json::Value* motion = j.find("motion")) {
    if (motion->is_string()) {
      c.motions.push_back(motion->as_string());
    } else if (motion->is_array()) {
      for (const auto& m : motion->items()) {
        if (m.is_string()) c.motions.push_back(m.as_string());
        else r.error("'motion' deve conter textos");
      }
    } else {
      r.error("'motion' deve ser texto ou lista de textos");
    }
  }
  for (const auto& m : c.motions) {
    std::vector<u16> tokens;
    std::string err;
    if (!compile_motion(m, tokens, err)) r.error(err);
    else c.motion_tokens.push_back(tokens);
  }
  r.get_string(j, "spin", c.spin);
  if (!c.spin.empty() && c.spin != "360") r.error("'spin' só aceita \"360\"");
  std::string buttons;
  r.get_string(j, "buttons", buttons, true);
  if (!buttons.empty() && !parse_buttons(buttons, c)) r.error("botões inválidos '" + buttons + "'");
  r.get_int(j, "window", c.window, false, 0, 60);
  r.get_bool(j, "negative_edge", c.negative_edge);
  std::string scheme = "both";
  r.get_string(j, "scheme", scheme);
  if (scheme == "classic") c.classic_only = true;
  else if (scheme == "simplified") c.simplified_only = true;
  else if (scheme != "both") r.error("'scheme' deve ser classic, simplified ou both");
  r.get_bool(j, "ground", c.ground);
  r.get_bool(j, "air", c.air);
  r.get_string(j, "move", c.move, true);
  c.priority = -1;
  r.get_int(j, "priority", c.priority, false, 0, 10000);
  r.get_int(j, "simplified_damage_permil", c.simplified_damage_permil, false, 0, 1000);
}

void load_jump(const Reader& r, const json::Value& root, const char* key, JumpDef& j) {
  const json::Value* v = root.find(key);
  if (v == nullptr || !v->is_object()) {
    r.error(std::string("objeto obrigatório '") + key + "' ausente");
    return;
  }
  Reader jr = r.sub(key);
  jr.check_unknown(*v, {"prejump", "vy", "gravity", "vx_forward", "vx_back", "landing"});
  jr.get_int(*v, "prejump", j.prejump, true, 1, 20);
  jr.get_int(*v, "vy", j.vy, true, 1, 20000);
  jr.get_int(*v, "gravity", j.gravity, true, 1, 5000);
  jr.get_int(*v, "vx_forward", j.vx_forward, true, 0, 10000);
  jr.get_int(*v, "vx_back", j.vx_back, true, 0, 10000);
  jr.get_int(*v, "landing", j.landing, true, 0, 30);
}

const char* kButtonNames[6] = {"LP", "MP", "HP", "LK", "MK", "HK"};
const char* kSimplifiedNames[3] = {"L", "M", "H"};
const char* kStancePrefix[3] = {"5", "2", "j."};

}  // namespace

LoadResult load_character(const json::Value& root, CharacterDef& ch) {
  LoadResult res;
  std::string id = "personagem";
  if (const json::Value* idv = root.find("id"); idv && idv->is_string()) id = idv->as_string();
  Reader r(res.errors, id);
  if (!root.is_object()) {
    r.error("raiz deve ser objeto");
    return res;
  }
  r.check_unknown(root, {"id", "name", "archetype", "health", "walk_forward", "walk_back", "run_speed", "jump",
                         "hop", "push", "hurt", "moves", "commands", "required", "simplified_normals", "notes"});
  r.get_string(root, "id", ch.id, true);
  r.get_string(root, "name", ch.name, true);
  r.get_string(root, "archetype", ch.archetype);
  r.get_int(root, "health", ch.health, true, 1000, 100000);
  r.get_int(root, "walk_forward", ch.walk_forward, true, 1, 2000);
  r.get_int(root, "walk_back", ch.walk_back, true, 1, 2000);
  r.get_int(root, "run_speed", ch.run_speed, false, 0, 5000);
  load_jump(r, root, "jump", ch.jump);
  load_jump(r, root, "hop", ch.hop);

  for (const char* group : {"push", "hurt"}) {
    const json::Value* g = root.find(group);
    if (g == nullptr || !g->is_object()) {
      r.error(std::string("objeto obrigatório '") + group + "' ausente");
      continue;
    }
    Reader gr = r.sub(group);
    const bool push = std::string(group) == "push";
    gr.get_box(*g, "stand", push ? ch.push_stand : ch.hurt_stand, true);
    gr.get_box(*g, "crouch", push ? ch.push_crouch : ch.hurt_crouch, true);
    gr.get_box(*g, "air", push ? ch.push_air : ch.hurt_air, true);
  }

  if (const json::Value* moves = root.find("moves"); moves && moves->is_array()) {
    for (const auto& m : moves->items()) {
      MoveDef md;
      std::string mid = "?";
      if (const json::Value* v = m.find("id"); v && v->is_string()) mid = v->as_string();
      load_move(r.sub("moves[" + mid + "]"), m, md);
      if (ch.find_move(md.id) >= 0) r.error("golpe duplicado '" + md.id + "'");
      ch.moves.push_back(std::move(md));
    }
  } else {
    r.error("lista 'moves' obrigatória");
  }

  if (const json::Value* cmds = root.find("commands"); cmds && cmds->is_array()) {
    for (const auto& c : cmds->items()) {
      CommandDef cd;
      std::string cid = "?";
      if (const json::Value* v = c.find("id"); v && v->is_string()) cid = v->as_string();
      load_command(r.sub("commands[" + cid + "]"), c, cd);
      ch.commands.push_back(std::move(cd));
    }
  } else {
    r.error("lista 'commands' obrigatória");
  }

  // Resolução de referências
  for (auto& m : ch.moves) {
    for (const auto& t : m.chain_targets) {
      const i16 idx = ch.find_move(t);
      if (idx < 0) r.error("moves[" + m.id + "]: alvo de chain inexistente '" + t + "'");
      m.chain_target_ids.push_back(idx);
    }
  }
  for (auto& c : ch.commands) {
    c.move_id = ch.find_move(c.move);
    if (c.move_id < 0) {
      r.error("commands[" + c.id + "]: golpe inexistente '" + c.move + "'");
      continue;
    }
    if (c.priority < 0) c.priority = default_priority(ch.moves[static_cast<size_t>(c.move_id)].kind);
  }
  std::stable_sort(ch.commands.begin(), ch.commands.end(),
                   [](const CommandDef& a, const CommandDef& b) { return a.priority > b.priority; });

  for (int s = 0; s < 3; ++s) {
    for (int b = 0; b < 6; ++b) ch.normals[s][b] = ch.find_move(std::string(kStancePrefix[s]) + kButtonNames[b]);
  }
  if (const json::Value* sn = root.find("simplified_normals"); sn && sn->is_object()) {
    for (int s = 0; s < 3; ++s) {
      for (int b = 0; b < 3; ++b) {
        const std::string key = std::string(kStancePrefix[s]) + kSimplifiedNames[b];
        std::string target;
        if (r.get_string(*sn, key.c_str(), target)) {
          ch.simplified_normals[s][b] = ch.find_move(target);
          if (ch.simplified_normals[s][b] < 0) r.error("simplified_normals: golpe inexistente '" + target + "'");
        }
      }
    }
  }

  if (const json::Value* req = root.find("required"); req && req->is_object()) {
    Reader rr = r.sub("required");
    rr.check_unknown(*req, {"dash", "backdash", "throw", "back_throw", "arrebentacao", "contracorrente"});
    auto resolve = [&](const char* key, i16& out) {
      std::string target;
      if (rr.get_string(*req, key, target, true)) {
        out = ch.find_move(target);
        if (out < 0) rr.error(std::string(key) + ": golpe inexistente '" + target + "'");
      }
    };
    resolve("dash", ch.move_dash);
    resolve("backdash", ch.move_backdash);
    resolve("throw", ch.move_throw);
    resolve("back_throw", ch.move_back_throw);
    resolve("arrebentacao", ch.move_arrebentacao);
    resolve("contracorrente", ch.move_contracorrente);
  } else {
    r.error("objeto 'required' obrigatório");
  }

  if (res.errors.empty()) validate_character(ch, res.errors);
  res.ok = res.errors.empty();
  return res;
}

void validate_character(const CharacterDef& ch, std::vector<std::string>& errors) {
  auto err = [&](const std::string& ctx, const std::string& msg) { errors.push_back(ch.id + "." + ctx + ": " + msg); };

  for (int b = 0; b < 6; ++b) {
    for (int s = 0; s < 3; ++s) {
      if (ch.normals[s][b] < 0) err("normals", std::string("normal obrigatório ausente: ") + kStancePrefix[s] + kButtonNames[b]);
    }
  }

  for (const auto& m : ch.moves) {
    const std::string ctx = "moves[" + m.id + "]";
    const i16 total = m.total_frames();
    for (size_t i = 0; i < m.hits.size(); ++i) {
      const auto& h = m.hits[i];
      if (h.from < m.first_active() || h.to > m.last_active()) {
        err(ctx, "hit " + std::to_string(i) + " fora da janela ativa [" + std::to_string(m.first_active()) + ", " +
                     std::to_string(m.last_active()) + "]");
      }
      if (i > 0 && h.from <= m.hits[i - 1].to) err(ctx, "hits sobrepostos ou fora de ordem");
      if (h.launch && h.launch_vy <= 0) err(ctx, "hit com 'launch' precisa de launch_vy > 0");
    }
    if (m.hits.size() > 8) err(ctx, "no máximo 8 hits por golpe");
    bool has_hitbox = false;
    bool has_throwbox = false;
    for (const auto& b : m.boxes) {
      if (b.to > total) err(ctx, "caixa termina após o último frame do golpe (" + std::to_string(total) + ")");
      if (b.kind == BoxKind::Hit) {
        has_hitbox = true;
        if (b.from < m.first_active() || b.to > m.last_active()) err(ctx, "hitbox fora dos frames ativos");
        bool covered = false;
        for (const auto& h : m.hits) covered |= (b.from <= h.to && b.to >= h.from);
        if (!covered) err(ctx, "hitbox sem hit correspondente");
      }
      if (b.kind == BoxKind::Throw) {
        has_throwbox = true;
        if (b.from < m.first_active() || b.to > m.last_active()) err(ctx, "throwbox fora dos frames ativos");
      }
    }
    for (const auto& h : m.hits) {
      bool has_box = false;
      for (const auto& b : m.boxes) has_box |= (b.kind == BoxKind::Hit && b.from <= h.to && b.to >= h.from);
      if (!has_box) err(ctx, "hit sem hitbox nos seus frames");
    }
    if (m.kind == MoveKind::Throw) {
      if (!has_throwbox) err(ctx, "arremesso sem throwbox");
      if (m.throw_damage <= 0) err(ctx, "arremesso sem dano");
      if (has_hitbox) err(ctx, "arremesso não deve ter hitbox de golpe");
    } else if (has_throwbox) {
      err(ctx, "throwbox só é permitida em arremessos");
    }
    if ((m.kind == MoveKind::Normal || m.kind == MoveKind::CommandNormal) && m.hits.empty()) {
      err(ctx, "normais precisam de ao menos um hit");
    }
    for (const auto& c : m.cancels) {
      if (c.to > total) err(ctx, "janela de cancelamento após o fim do golpe");
      if ((c.mask & cancel::Chain) && m.chain_target_ids.empty()) err(ctx, "cancelamento 'chain' sem alvos em 'chain'");
    }
    for (const auto& v : m.velocity) {
      if (v.to > total) err(ctx, "velocity após o fim do golpe");
    }
    for (const auto& s : m.strike_invuln) {
      if (s.to > total) err(ctx, "invulnerabilidade após o fim do golpe");
    }
    if (m.kind == MoveKind::Super && m.meter_cost <= 0) err(ctx, "super precisa de custo de medidor");
    if (m.kind == MoveKind::Enhanced && m.meter_cost <= 0) err(ctx, "reforçado precisa de custo de medidor");
    if (m.airborne && !m.ends_on_landing && m.landing_recovery == 0 && m.kind == MoveKind::Normal) {
      err(ctx, "golpe aéreo normal deve terminar ao aterrissar ('ends_on_landing')");
    }
  }
  for (const auto& c : ch.commands) {
    if (c.motion_tokens.empty() && c.spin.empty() && c.move_id >= 0) {
      const auto k = ch.moves[static_cast<size_t>(c.move_id)].kind;
      if (k == MoveKind::Special || k == MoveKind::Super) {
        if (!c.simplified_only) err("commands[" + c.id + "]", "especial clássico sem movimento direcional");
      }
    }
  }
}

LoadResult load_rules(const json::Value& root, Rules& r) {
  LoadResult res;
  Reader rd(res.errors, "rules");
  if (!root.is_object()) {
    rd.error("raiz deve ser objeto");
    return res;
  }
  rd.get_int(root, "stage_half_width", r.stage_half_width, true, 10000, 500000);
  rd.get_int(root, "max_distance", r.max_distance, true, 10000, 500000);
  rd.get_int(root, "start_distance", r.start_distance, true, 1000, 500000);
  rd.get_int(root, "round_seconds", r.round_seconds, true, 10, 999);
  rd.get_int(root, "rounds_to_win", r.rounds_to_win, true, 1, 5);
  rd.get_int(root, "round_intro_frames", r.round_intro_frames, false, 0, 600);
  rd.get_int(root, "round_end_frames", r.round_end_frames, false, 0, 600);
  rd.get_int(root, "button_buffer", r.button_buffer, true, 1, 15);
  rd.get_int(root, "wakeup_buffer", r.wakeup_buffer, false, 1, 15);
  rd.get_int(root, "dash_window", r.dash_window, false, 4, 30);
  rd.get_int(root, "throw_tech_window", r.throw_tech_window, true, 1, 30);
  rd.get_int(root, "throw_tech_separation", r.throw_tech_separation, false, 1, 60);
  rd.get_int(root, "throw_tech_push", r.throw_tech_push, false, 0, 100000);
  rd.get_int(root, "knockdown_frames", r.knockdown_frames, true, 1, 200);
  rd.get_int(root, "wakeup_frames", r.wakeup_frames, true, 1, 100);
  rd.get_int(root, "hard_wakeup_invuln", r.hard_wakeup_invuln, false, 0, 60);
  rd.get_int(root, "counter_hit_bonus", r.counter_hit_bonus, true, 0, 30);
  rd.get_int(root, "punish_counter_bonus", r.punish_counter_bonus, true, 0, 30);
  rd.get_int(root, "counter_damage_permil", r.counter_damage_permil, false, 1000, 3000);
  rd.get_int(root, "juggle_limit", r.juggle_limit, true, 1, 30);
  rd.get_int(root, "hitstun_decay_start", r.hitstun_decay_start, true, 1, 50);
  rd.get_int(root, "hitstun_min", r.hitstun_min, true, 1, 60);
  rd.get_int(root, "max_combo_damage_permil", r.max_combo_damage_permil, true, 100, 1000);
  rd.get_int(root, "min_scaling_permil", r.min_scaling_permil, true, 0, 1000);
  rd.get_int(root, "meter_per_bar", r.meter_per_bar, true, 1, 100000);
  rd.get_int(root, "meter_bars", r.meter_bars, true, 1, 10);
  rd.get_int(root, "meter_gain_on_damaged_permil", r.meter_gain_on_damaged_permil, false, 0, 1000);
  rd.get_int(root, "multi_hit_hitstop_permil", r.multi_hit_hitstop_permil, false, 0, 1000);
  rd.get_int(root, "max_hitstop", r.max_hitstop, false, 1, 60);
  rd.get_int(root, "proximity_guard", r.proximity_guard, false, 0, 500000);
  rd.get_int(root, "corner_zone", r.corner_zone, false, 0, 50000);
  if (const json::Value* sc = root.find("scaling_permil"); sc && sc->is_array()) {
    r.scaling_permil.clear();
    for (const auto& v : sc->items()) {
      if (!v.is_int() || v.as_int() < 0 || v.as_int() > 1000) rd.error("scaling_permil deve conter inteiros 0..1000");
      else r.scaling_permil.push_back(static_cast<i32>(v.as_int()));
    }
    if (r.scaling_permil.empty()) rd.error("scaling_permil não pode ser vazio");
  } else {
    rd.error("lista 'scaling_permil' obrigatória");
  }
  if (const json::Value* m = root.find("mare"); m && m->is_object()) {
    Reader mr = rd.sub("mare");
    MareRules& mare = r.mare;
    mr.get_int(*m, "limit", mare.limit, true, 1, 100000);
    mr.get_int(*m, "threshold", mare.threshold, true, 1, 100000);
    mr.get_int(*m, "walk_forward_per_frame", mare.walk_forward_per_frame, true, 0, 1000);
    mr.get_int(*m, "dash_forward", mare.dash_forward, true, 0, 10000);
    mr.get_int(*m, "attack_blocked", mare.attack_blocked, true, 0, 10000);
    mr.get_int(*m, "attack_hit", mare.attack_hit, true, 0, 10000);
    mr.get_int(*m, "counter_hit", mare.counter_hit, true, 0, 10000);
    mr.get_int(*m, "punish_counter", mare.punish_counter, true, 0, 10000);
    mr.get_int(*m, "throw_landed", mare.throw_landed, true, 0, 10000);
    mr.get_int(*m, "corner_per_frame", mare.corner_per_frame, true, 0, 1000);
    mr.get_int(*m, "retreat_grace_frames", mare.retreat_grace_frames, true, 0, 1000);
    mr.get_int(*m, "retreat_per_frame", mare.retreat_per_frame, true, 0, 1000);
    if (mare.threshold > mare.limit) mr.error("threshold maior que limit");
  } else {
    rd.error("objeto 'mare' obrigatório");
  }
  if (r.start_distance > r.max_distance) rd.error("start_distance maior que max_distance");
  if (r.max_distance > 2 * r.stage_half_width) rd.error("max_distance maior que a arena");
  res.ok = res.errors.empty();
  return res;
}

LoadResult load_game_data(const std::string& dir, GameData& out) {
  LoadResult res;
  auto load_json = [&](const std::string& path, json::Value& v) {
    bool ok = false;
    const std::string text = read_text_file(path, ok);
    if (!ok) {
      res.errors.push_back(path + ": não foi possível ler o arquivo");
      return false;
    }
    json::ParseResult pr = json::parse(text);
    if (!pr.ok) {
      res.errors.push_back(path + ":" + pr.error);
      return false;
    }
    v = std::move(pr.value);
    return true;
  };
  json::Value rules;
  if (load_json(dir + "/rules.json", rules)) {
    LoadResult rr = load_rules(rules, out.rules);
    res.errors.insert(res.errors.end(), rr.errors.begin(), rr.errors.end());
  }
  json::Value roster;
  if (load_json(dir + "/roster.json", roster)) {
    const json::Value* chars = roster.find("characters");
    if (chars == nullptr || !chars->is_array()) {
      res.errors.push_back("roster.json: lista 'characters' obrigatória");
    } else {
      for (const auto& c : chars->items()) {
        if (!c.is_string()) {
          res.errors.push_back("roster.json: ids devem ser textos");
          continue;
        }
        json::Value cj;
        if (!load_json(dir + "/characters/" + c.as_string() + ".json", cj)) continue;
        CharacterDef ch;
        LoadResult cr = load_character(cj, ch);
        res.errors.insert(res.errors.end(), cr.errors.begin(), cr.errors.end());
        if (cr.ok) out.characters.push_back(std::move(ch));
      }
    }
  }
  res.ok = res.errors.empty();
  return res;
}

namespace {
void hash_box(ByteWriter& w, const Box& b) {
  w.i32v(b.x);
  w.i32v(b.y);
  w.i32v(b.w);
  w.i32v(b.h);
}
}  // namespace

u64 hash_game_data(const GameData& d) {
  ByteWriter w;
  const Rules& r = d.rules;
  w.i32v(r.stage_half_width);
  w.i32v(r.max_distance);
  w.i32v(r.start_distance);
  w.i32v(r.round_seconds);
  w.u8v(r.rounds_to_win);
  w.i16v(r.button_buffer);
  w.i16v(r.throw_tech_window);
  w.i16v(r.knockdown_frames);
  w.i16v(r.wakeup_frames);
  w.i16v(r.counter_hit_bonus);
  w.i16v(r.punish_counter_bonus);
  w.i16v(r.juggle_limit);
  w.i16v(r.hitstun_decay_start);
  w.i32v(r.max_combo_damage_permil);
  for (i32 s : r.scaling_permil) w.i32v(s);
  w.i32v(r.mare.limit);
  w.i32v(r.mare.threshold);
  w.i32v(r.mare.attack_hit);
  w.i32v(r.mare.punish_counter);
  for (const auto& ch : d.characters) {
    w.str(ch.id);
    w.i32v(ch.health);
    w.i32v(ch.walk_forward);
    w.i32v(ch.walk_back);
    w.i32v(ch.run_speed);
    hash_box(w, ch.push_stand);
    hash_box(w, ch.hurt_stand);
    hash_box(w, ch.hurt_crouch);
    for (const auto& m : ch.moves) {
      w.str(m.id);
      w.i16v(m.startup);
      w.i16v(m.active);
      w.i16v(m.recovery);
      w.i32v(m.meter_cost);
      for (const auto& h : m.hits) {
        w.i16v(h.from);
        w.i16v(h.to);
        w.i32v(h.damage);
        w.i16v(h.hitstun);
        w.i16v(h.blockstun);
        w.i16v(h.hitstop);
        w.i32v(h.pushback_hit);
        w.i32v(h.pushback_block);
        w.u8v(static_cast<u8>(h.level));
      }
      for (const auto& b : m.boxes) {
        w.i16v(b.from);
        w.i16v(b.to);
        w.u8v(static_cast<u8>(b.kind));
        hash_box(w, b.box);
      }
      for (const auto& c : m.cancels) {
        w.i16v(c.from);
        w.i16v(c.to);
        w.u8v(c.mask);
      }
    }
    for (const auto& c : ch.commands) {
      w.str(c.id);
      w.str(c.move);
      w.i32v(c.priority);
      w.i16v(c.window);
    }
  }
  return fnv1a64(w.bytes().data(), w.bytes().size());
}

}  // namespace fg
