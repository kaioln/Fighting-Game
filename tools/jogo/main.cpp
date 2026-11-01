// Projeto Estaleiro — executável do jogo (apresentação 3D leve sobre o núcleo fgcore).
//
// A simulação roda no núcleo determinístico, 60 Hz fixos, um passo por quadro. Esta camada só LÊ o
// estado e desenha: modelos e animações 3D (glTF/GLB) descritos em data/presentation/*.json e
// carregados de assets/. O código nunca gera geometria para representar personagens ou cenário;
// se um arquivo de arte faltar, a tela avisa em vez de inventar formas.
//
// Feito com raylib (zlib) para ser pequeno e leve: roda em GPUs integradas.

#include <raylib.h>
#include <cctype>
#include <cmath>
#include <cstdlib>

#include <algorithm>
#include <array>
#include <cstdio>
#include <deque>
#include <string>
#include <vector>

#include "fg/json.h"
#include "fg/loader.h"
#include "fg/sim.h"

namespace {

using fg::Act;
using fg::BoxKind;
using fg::FighterState;
using fg::GameData;
using fg::i32;
using fg::InputBits;
using fg::MatchState;
using fg::MoveDef;
namespace btn = fg::btn;

// ---------------------------------------------------------------- aparência
// Paleta de leitura (mesmas cores para o mesmo tipo de caixa em todo o jogo).
const Color kBg = {18, 22, 30, 255};
const Color kFloor = {44, 52, 66, 255};
const Color kGrid = {34, 40, 52, 255};
const Color kText = {226, 232, 240, 255};
const Color kDim = {140, 150, 166, 255};
const Color kP1 = {72, 176, 255, 255};
const Color kP2 = {255, 150, 64, 255};
const Color kHit = {240, 60, 70, 255};
const Color kThrow = {80, 220, 120, 255};
const Color kPush = {235, 235, 235, 255};
const Color kHealth = {236, 200, 70, 255};
const Color kHealthBack = {60, 30, 34, 255};
const Color kMeter = {120, 110, 255, 255};
const Color kMareP1 = {60, 150, 230, 255};
const Color kMareP2 = {240, 130, 50, 255};

Font g_font;
bool g_font_loaded = false;

void text(const std::string& s, float x, float y, float size, Color c) {
  DrawTextEx(g_font, s.c_str(), {x, y}, size, size / 16.0f, c);
}
float text_width(const std::string& s, float size) { return MeasureTextEx(g_font, s.c_str(), size, size / 16.0f).x; }
void text_center(const std::string& s, float cx, float y, float size, Color c) { text(s, cx - text_width(s, size) / 2, y, size, c); }
void text_right(const std::string& s, float rx, float y, float size, Color c) { text(s, rx - text_width(s, size), y, size, c); }

void load_font() {
  const char* candidates[] = {
      "C:/Windows/Fonts/segoeui.ttf",
      "C:/Windows/Fonts/arial.ttf",
      "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
      "/usr/share/fonts/TTF/DejaVuSans.ttf",
  };
  std::vector<int> cps;
  for (int c = 32; c < 256; ++c) cps.push_back(c);
  for (const char* path : candidates) {
    if (!FileExists(path)) continue;
    g_font = LoadFontEx(path, 48, cps.data(), static_cast<int>(cps.size()));
    if (g_font.texture.id != 0) {
      SetTextureFilter(g_font.texture, TEXTURE_FILTER_BILINEAR);
      g_font_loaded = true;
      return;
    }
  }
  g_font = GetFontDefault();
}

std::string fmt(const char* f, int a) {
  char b[64];
  std::snprintf(b, sizeof b, f, a);
  return b;
}

// ---------------------------------------------------------------- input
struct KeyMap {
  int up, down, left, right, lp, mp, hp, lk, mk, hk, sp;
};
const KeyMap kKeysP1 = {KEY_W, KEY_S, KEY_A, KEY_D, KEY_U, KEY_I, KEY_O, KEY_J, KEY_K, KEY_L, KEY_SPACE};
const KeyMap kKeysP2 = {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_KP_4, KEY_KP_5, KEY_KP_6,
                        KEY_KP_1, KEY_KP_2, KEY_KP_3, KEY_KP_0};

InputBits read_keys(const KeyMap& k) {
  InputBits b = 0;
  if (IsKeyDown(k.up)) b |= btn::Up;
  if (IsKeyDown(k.down)) b |= btn::Down;
  if (IsKeyDown(k.left)) b |= btn::Left;
  if (IsKeyDown(k.right)) b |= btn::Right;
  if (IsKeyDown(k.lp)) b |= btn::LP;
  if (IsKeyDown(k.mp)) b |= btn::MP;
  if (IsKeyDown(k.hp)) b |= btn::HP;
  if (IsKeyDown(k.lk)) b |= btn::LK;
  if (IsKeyDown(k.mk)) b |= btn::MK;
  if (IsKeyDown(k.hk)) b |= btn::HK;
  if (IsKeyDown(k.sp)) b |= btn::SP;
  return b;
}

// Layout de controle (Xbox): X=LP Y=MP RB=HP / A=LK B=MK RT=HK / LB=SP. Direcional e analógico esquerdo.
InputBits read_pad(int pad) {
  if (!IsGamepadAvailable(pad)) return 0;
  InputBits b = 0;
  const float ax = GetGamepadAxisMovement(pad, GAMEPAD_AXIS_LEFT_X);
  const float ay = GetGamepadAxisMovement(pad, GAMEPAD_AXIS_LEFT_Y);
  if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_FACE_UP) || ay < -0.5f) b |= btn::Up;
  if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_FACE_DOWN) || ay > 0.5f) b |= btn::Down;
  if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_FACE_LEFT) || ax < -0.5f) b |= btn::Left;
  if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) || ax > 0.5f) b |= btn::Right;
  if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) b |= btn::LP;
  if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_RIGHT_FACE_UP)) b |= btn::MP;
  if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) b |= btn::HP;
  if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) b |= btn::LK;
  if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) b |= btn::MK;
  if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_RIGHT_TRIGGER_2) ||
      GetGamepadAxisMovement(pad, GAMEPAD_AXIS_RIGHT_TRIGGER) > 0.3f)
    b |= btn::HK;
  if (IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) b |= btn::SP;
  return b;
}

// Histórico visível de inputs (direção numérica + botões, com duração).
struct InputLog {
  struct Entry {
    int dir;
    InputBits buttons;
    int frames;
  };
  std::deque<Entry> entries;
  void push(int dir, InputBits buttons) {
    const InputBits atk = buttons & btn::Attacks;
    if (!entries.empty() && entries.front().dir == dir && entries.front().buttons == atk) {
      entries.front().frames = std::min(entries.front().frames + 1, 99);
      return;
    }
    entries.push_front({dir, atk, 1});
    if (entries.size() > 18) entries.pop_back();
  }
};

std::string buttons_text(InputBits b) {
  std::string s;
  auto add = [&](InputBits m, const char* n) {
    if (b & m) {
      if (!s.empty()) s += "+";
      s += n;
    }
  };
  add(btn::LP, "LP");
  add(btn::MP, "MP");
  add(btn::HP, "HP");
  add(btn::LK, "LK");
  add(btn::MK, "MK");
  add(btn::HK, "HK");
  add(btn::SP, "SP");
  return s;
}

// Setas para a direção numérica (ASCII, para funcionar com qualquer fonte).
const char* dir_glyph(int d) {
  switch (d) {
    case 1: return "1 ↙";
    case 2: return "2 ↓";
    case 3: return "3 ↘";
    case 4: return "4 ←";
    case 6: return "6 →";
    case 7: return "7 ↖";
    case 8: return "8 ↑";
    case 9: return "9 ↗";
    default: return "5 ·";
  }
}

// ---------------------------------------------------------------- treino
enum class Dummy { Human, Stand, GuardStand, GuardCrouch, Jump, Count };
const char* dummy_name(Dummy d) {
  switch (d) {
    case Dummy::Human: return "Humano (setas + teclado numérico / controle 2)";
    case Dummy::Stand: return "Boneco parado";
    case Dummy::GuardStand: return "Boneco defende em pé";
    case Dummy::GuardCrouch: return "Boneco defende agachado";
    case Dummy::Jump: return "Boneco pula sem parar";
    default: return "";
  }
}

InputBits dummy_input(Dummy d, const FighterState& self) {
  const InputBits back = self.facing > 0 ? btn::Left : btn::Right;
  switch (d) {
    case Dummy::GuardStand: return back;
    case Dummy::GuardCrouch: return static_cast<InputBits>(back | btn::Down);
    case Dummy::Jump: return btn::Up;
    default: return 0;
  }
}

// Categoria de cada frame para o medidor de frames.
enum class FrameCat : unsigned char { Free, Startup, Active, Recovery, Hitstun, Blockstun, Movement, Hitstop };

FrameCat classify(const MatchState& s, const GameData& d, int p) {
  const FighterState& f = s.f[p];
  if (f.hitstop > 0 || s.freeze > 0) return FrameCat::Hitstop;
  switch (f.act) {
    case Act::Hitstun:
    case Act::Juggle:
    case Act::Knockdown:
    case Act::Thrown:
    case Act::KO:
      return FrameCat::Hitstun;
    case Act::Blockstun:
      return FrameCat::Blockstun;
    case Act::Move: {
      const MoveDef* m = fg::current_move(s, d, p);
      if (!m) return FrameCat::Movement;
      if (f.move_frame < m->first_active()) return FrameCat::Startup;
      if (f.move_frame <= m->last_active()) return FrameCat::Active;
      return FrameCat::Recovery;
    }
    case Act::PreJump:
    case Act::Air:
    case Act::Landing:
    case Act::Wakeup:
    case Act::ThrowAttack:
    case Act::TechRecover:
      return FrameCat::Movement;
    default:
      return fg::is_actionable(s, d, p) ? FrameCat::Free : FrameCat::Movement;
  }
}

Color cat_color(FrameCat c) {
  switch (c) {
    case FrameCat::Startup: return {70, 200, 120, 255};
    case FrameCat::Active: return {235, 70, 80, 255};
    case FrameCat::Recovery: return {70, 120, 230, 255};
    case FrameCat::Hitstun: return {240, 210, 70, 255};
    case FrameCat::Blockstun: return {190, 120, 230, 255};
    case FrameCat::Movement: return {110, 170, 180, 255};
    case FrameCat::Hitstop: return {90, 90, 100, 255};
    default: return {48, 54, 66, 255};
  }
}

const char* act_name(Act a) {
  switch (a) {
    case Act::Idle: return "parado";
    case Act::Crouch: return "agachado";
    case Act::WalkForward: return "andando";
    case Act::WalkBack: return "recuando";
    case Act::Run: return "correndo";
    case Act::Guard: return "guarda";
    case Act::PreJump: return "pré-salto";
    case Act::Air: return "no ar";
    case Act::Landing: return "aterrissagem";
    case Act::Move: return "golpe";
    case Act::Hitstun: return "atordoado";
    case Act::Blockstun: return "bloqueando";
    case Act::Juggle: return "no ar (combo)";
    case Act::Knockdown: return "no chão";
    case Act::Wakeup: return "levantando";
    case Act::ThrowAttack: return "agarrando";
    case Act::Thrown: return "agarrado";
    case Act::TechRecover: return "tech";
    case Act::KO: return "K.O.";
  }
  return "";
}

const char* event_name(fg::EventType t) {
  using E = fg::EventType;
  switch (t) {
    case E::Hit: return "ACERTO";
    case E::CounterHit: return "COUNTER";
    case E::PunishCounter: return "PUNISH COUNTER";
    case E::Block: return "BLOQUEIO";
    case E::Armor: return "ARMADURA";
    case E::ThrowConnect: return "AGARRÃO";
    case E::ThrowTech: return "TECH!";
    case E::ThrowComplete: return "ARREMESSO";
    case E::Knockdown: return "QUEDA";
    case E::Arrebentacao: return "ARREBENTAÇÃO";
    case E::Contracorrente: return "CONTRACORRENTE";
    case E::SuperFreeze: return "SUPER";
    default: return nullptr;
  }
}

struct Feed {
  std::string line;
  int player;
  int ttl;
};

// Mede a vantagem de frames depois de um contato: quem fica livre primeiro e por quantos frames.
struct AdvantageMeter {
  bool tracking = false;
  int attacker = 0;
  int free_frame[2] = {-1, -1};
  int start = 0;
  bool has_result = false;
  int result = 0;
  std::string label;
};

struct App {
  GameData data;
  std::string data_error;
  MatchState state{};
  fg::MatchSetup setup;
  bool training = true;
  Dummy dummy = Dummy::Human;
  bool paused = false;
  bool show_help = false;
  bool show_push = true;
  bool show_boxes = false;
  int frame_meter_len = 0;
  static constexpr int kMeterLen = 90;
  std::array<std::array<FrameCat, kMeterLen>, 2> meter{};
  int idle_frames = 0;
  InputLog log[2];
  std::deque<Feed> feed;
  AdvantageMeter adv;
  int combo_show[2] = {0, 0};
  int combo_hits[2] = {0, 0};
  int combo_damage[2] = {0, 0};
  int banner_ttl = 0;
  std::string banner;
  Color banner_color = kText;

  void restart() {
    setup.options = training ? static_cast<fg::u8>(fg::option::NoTimer | fg::option::SkipIntro | fg::option::NoRoundEnd)
                             : static_cast<fg::u8>(0);
    fg::init_match(state, data, setup);
    frame_meter_len = 0;
    idle_frames = 0;
    feed.clear();
    adv = {};
    banner_ttl = 0;
    for (int p = 0; p < 2; ++p) {
      combo_show[p] = 0;
      log[p].entries.clear();
    }
  }

  const fg::CharacterDef& ch(int p) const { return data.characters[static_cast<size_t>(state.f[p].character)]; }

  void push_feed(const std::string& s, int p) {
    feed.push_front({s, p, 240});
    if (feed.size() > 7) feed.pop_back();
  }

  void tick(const fg::FrameInput& in) {
    fg::StepEvents ev;
    fg::step(state, data, in, &ev);

    for (int i = 0; i < ev.count; ++i) {
      const fg::Event& e = ev.items[i];
      using E = fg::EventType;
      const int p = e.player < 0 ? 0 : e.player;
      if (const char* n = event_name(e.type)) {
        std::string line = n;
        if (e.move >= 0 && e.player >= 0) {
          const auto& moves = data.characters[static_cast<size_t>(state.f[p].character)].moves;
          if (static_cast<size_t>(e.move) < moves.size()) line += "  " + moves[static_cast<size_t>(e.move)].id;
        }
        if (e.value > 0 && (e.type == E::Hit || e.type == E::CounterHit || e.type == E::PunishCounter ||
                            e.type == E::ThrowComplete || e.type == E::Armor))
          line += fmt("  %d de dano", e.value);
        push_feed(line, p);
      }
      if (e.type == E::Hit || e.type == E::CounterHit || e.type == E::PunishCounter || e.type == E::Block) {
        adv = {};
        adv.tracking = true;
        adv.attacker = p;
        adv.start = static_cast<int>(state.frame);
        adv.label = e.type == E::Block ? "no bloqueio" : "no acerto";
      }
      if (e.type == E::CounterHit) show_banner("COUNTER", kHealth);
      if (e.type == E::PunishCounter) show_banner("PUNISH COUNTER", kHit);
      if (e.type == E::Arrebentacao) show_banner("ARREBENTAÇÃO", p == 0 ? kMareP1 : kMareP2);
      if (e.type == E::Contracorrente) show_banner("CONTRACORRENTE", p == 0 ? kMareP1 : kMareP2);
      if (e.type == E::Fight) show_banner("LUTEM!", kText);
      if (e.type == E::RoundStart) show_banner(fmt("ROUND %d", state.round), kText);
      if (e.type == E::KO) show_banner("K.O.", kHit);
      if (e.type == E::TimeOver) show_banner("TEMPO!", kHealth);
      if (e.type == E::MatchEnd) {
        const int w = state.match_winner;
        show_banner(w == 2 ? "EMPATE" : fmt("VITÓRIA DO P%d", w + 1), w == 1 ? kP2 : kP1, 100000);
      }
    }

    // Vantagem de frames: espera os dois ficarem livres.
    if (adv.tracking) {
      for (int p = 0; p < 2; ++p)
        if (adv.free_frame[p] < 0 && fg::is_actionable(state, data, p)) adv.free_frame[p] = static_cast<int>(state.frame);
      if (adv.free_frame[0] >= 0 && adv.free_frame[1] >= 0) {
        const int a = adv.attacker, dfd = 1 - adv.attacker;
        adv.result = adv.free_frame[dfd] - adv.free_frame[a];
        adv.has_result = true;
        adv.tracking = false;
      } else if (static_cast<int>(state.frame) - adv.start > 300) {
        adv.tracking = false;
      }
    }

    // Combo: o contador fica na tela um pouco depois de terminar.
    for (int p = 0; p < 2; ++p) {
      const FighterState& def = state.f[1 - p];
      if (def.combo_hits >= 1) {
        combo_hits[p] = def.combo_hits;
        combo_damage[p] = def.combo_damage;
        combo_show[p] = 90;
      } else if (combo_show[p] > 0) {
        --combo_show[p];
      }
    }

    // Medidor de frames: reinicia quando os dois ficam livres por um tempo.
    const FrameCat c0 = classify(state, data, 0), c1 = classify(state, data, 1);
    const bool both_free = c0 == FrameCat::Free && c1 == FrameCat::Free;
    if (both_free) {
      ++idle_frames;
    } else {
      if (idle_frames > 20) frame_meter_len = 0;
      idle_frames = 0;
      if (frame_meter_len < kMeterLen) {
        meter[0][static_cast<size_t>(frame_meter_len)] = c0;
        meter[1][static_cast<size_t>(frame_meter_len)] = c1;
        ++frame_meter_len;
      }
    }

    for (auto& f : feed) --f.ttl;
    while (!feed.empty() && feed.back().ttl <= 0) feed.pop_back();
    if (banner_ttl > 0 && banner_ttl < 100000) --banner_ttl;
  }

  void show_banner(const std::string& s, Color c, int ttl = 70) {
    banner = s;
    banner_color = c;
    banner_ttl = ttl;
  }
};

// ---------------------------------------------------------------- desenho
// ---------------------------------------------------------------- apresentação 3D
// Tela (para o HUD).
struct View {
  float w, h;
};

// Uma entrada de animação: nome do clipe no arquivo do modelo e, para golpes, o quadro do clipe
// em que o golpe faz contato (alinha o primeiro frame ativo do núcleo com o impacto da animação).
struct ClipRef {
  std::string clip;
  int contact = -1;      // quadro do clipe (60 por segundo) em que o golpe atinge
  int from = 0;          // trecho do clipe usado (quadros); -1 em `to` = até o fim
  int to = -1;
  bool loop = false;
  bool reverse = false;  // toca de trás para a frente (ex.: andar para trás)
};

struct CharacterLook {
  std::string model_path;
  std::vector<std::string> anim_files;  // arquivos extras com clipes no mesmo esqueleto
  float scale = 1.0f;
  float yaw_offset = 0.0f;
  Color tint = WHITE;
  std::vector<std::pair<std::string, ClipRef>> states;  // idle, walk_forward, ...
  std::vector<std::pair<std::string, ClipRef>> moves;   // por id de golpe
};

struct ArenaLook {
  std::string model_path;
  std::string sky_path;  // imagem equirretangular de céu (opcional)
  float scale = 1.0f;
  Vector3 offset = {0, 0, 0};
  Color sky = {120, 140, 160, 255};
  Color fog = {120, 140, 160, 255};
};

// Lê inteiros do JSON (o parser do projeto não aceita números fracionários).
float milli(const fg::json::Value* v, float def) { return v && v->is_int() ? static_cast<float>(v->as_int()) / 1000.0f : def; }

Color read_color(const fg::json::Value* v, Color def) {
  if (!v || !v->is_array() || v->items().size() < 3) return def;
  const auto& it = v->items();
  auto c = [&](size_t i) { return static_cast<unsigned char>(std::clamp<long long>(it[i].as_int(), 0, 255)); };
  return {c(0), c(1), c(2), static_cast<unsigned char>(it.size() > 3 ? c(3) : 255)};
}

ClipRef read_clip(const fg::json::Value& v) {
  ClipRef r;
  if (v.is_string()) {
    r.clip = v.as_string();
  } else if (v.is_object()) {
    if (const auto* c = v.find("clip"); c && c->is_string()) r.clip = c->as_string();
    if (const auto* c = v.find("contact"); c && c->is_int()) r.contact = static_cast<int>(c->as_int());
    if (const auto* c = v.find("loop"); c && c->is_bool()) r.loop = c->as_bool();
    if (const auto* c = v.find("from"); c && c->is_int()) r.from = static_cast<int>(c->as_int());
    if (const auto* c = v.find("to"); c && c->is_int()) r.to = static_cast<int>(c->as_int());
    if (const auto* c = v.find("reverse"); c && c->is_bool()) r.reverse = c->as_bool();
  }
  return r;
}

bool load_json(const std::string& path, fg::json::Value& out, std::string& err) {
  bool ok = false;
  const std::string txt = fg::read_text_file(path, ok);
  if (!ok) {
    err = "não foi possível ler " + path;
    return false;
  }
  auto r = fg::json::parse(txt);
  if (!r.ok) {
    err = path + ":" + r.error;
    return false;
  }
  out = std::move(r.value);
  return true;
}

bool load_character_look(const std::string& path, CharacterLook& look, std::string& err) {
  fg::json::Value root;
  if (!load_json(path, root, err)) return false;
  if (const auto* m = root.find("model"); m && m->is_string()) look.model_path = m->as_string();
  look.scale = milli(root.find("scale_milli"), 1.0f);
  look.yaw_offset = milli(root.find("yaw_offset_milli_deg"), 0.0f);
  look.tint = read_color(root.find("tint"), WHITE);
  if (const auto* a = root.find("animations"); a && a->is_array())
    for (const auto& it : a->items())
      if (it.is_string()) look.anim_files.push_back(it.as_string());
  if (const auto* s = root.find("states"); s && s->is_object())
    for (const auto& [k, v] : s->members()) look.states.emplace_back(k, read_clip(v));
  if (const auto* s = root.find("moves"); s && s->is_object())
    for (const auto& [k, v] : s->members()) look.moves.emplace_back(k, read_clip(v));
  if (look.model_path.empty()) {
    err = path + ": campo 'model' ausente";
    return false;
  }
  return true;
}

bool load_arena_look(const std::string& path, ArenaLook& look, std::string& err) {
  fg::json::Value root;
  if (!load_json(path, root, err)) return false;
  if (const auto* m = root.find("model"); m && m->is_string()) look.model_path = m->as_string();
  look.scale = milli(root.find("scale_milli"), 1.0f);
  if (const auto* o = root.find("offset_mm"); o && o->is_array() && o->items().size() == 3)
    look.offset = {static_cast<float>(o->items()[0].as_int()) / 1000.0f, static_cast<float>(o->items()[1].as_int()) / 1000.0f,
                   static_cast<float>(o->items()[2].as_int()) / 1000.0f};
  if (const auto* m = root.find("sky_image"); m && m->is_string()) look.sky_path = m->as_string();
  look.sky = read_color(root.find("sky"), look.sky);
  look.fog = read_color(root.find("fog"), look.sky);
  return true;
}

// Iluminação simples (luz direcional + ambiente + contorno), barata para qualquer GPU.
const char* kLitVS = R"(#version 330
in vec3 vertexPosition; in vec2 vertexTexCoord; in vec3 vertexNormal; in vec4 vertexColor;
uniform mat4 mvp; uniform mat4 matModel; uniform mat4 matNormal;
out vec3 fragPos; out vec2 fragTexCoord; out vec4 fragColor; out vec3 fragNormal;
void main() {
  fragPos = vec3(matModel * vec4(vertexPosition, 1.0));
  fragTexCoord = vertexTexCoord; fragColor = vertexColor;
  fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));
  gl_Position = mvp * vec4(vertexPosition, 1.0);
})";

const char* kLitFS = R"(#version 330
in vec3 fragPos; in vec2 fragTexCoord; in vec4 fragColor; in vec3 fragNormal;
uniform sampler2D texture0; uniform vec4 colDiffuse;
uniform vec3 lightDir; uniform vec3 lightColor; uniform vec3 ambient; uniform vec3 viewPos;
uniform vec3 fogColor; uniform float fogNear; uniform float fogFar;
out vec4 finalColor;
void main() {
  vec4 base = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
  if (base.a < 0.4) discard;
  vec3 n = normalize(fragNormal);
  float d = max(dot(n, -normalize(lightDir)), 0.0);
  vec3 v = normalize(viewPos - fragPos);
  float rim = pow(1.0 - max(dot(n, v), 0.0), 3.0) * 0.35;
  vec3 c = base.rgb * (ambient + lightColor * d) + lightColor * rim * 0.5;
  float f = clamp((length(viewPos - fragPos) - fogNear) / (fogFar - fogNear), 0.0, 1.0);
  finalColor = vec4(mix(c, fogColor, f * 0.6), base.a);
})";

struct Fighter3D {
  bool loaded = false;
  Model model{};
  ModelAnimation* anims = nullptr;  // clipes do próprio modelo
  int anim_count = 0;
  std::vector<std::pair<ModelAnimation*, int>> extra;  // arquivos extras (descarregados no fim)
  std::vector<ModelAnimation> all;                      // todos os clipes, para busca
  CharacterLook look;
  // estado de reprodução (somente apresentação)
  std::string current_key;
  int local_frame = 0;

  int find_anim(const std::string& name) const {
    for (size_t i = 0; i < all.size(); ++i)
      if (name == all[i].name) return static_cast<int>(i);
    return -1;
  }
  void unload() {
    if (!loaded) return;
    if (anims) UnloadModelAnimations(anims, anim_count);
    for (auto& [a, n] : extra) UnloadModelAnimations(a, n);
    UnloadModel(model);
    loaded = false;
  }
  const ClipRef* clip_for(const std::vector<std::pair<std::string, ClipRef>>& table, const std::string& key) const {
    for (const auto& [k, v] : table)
      if (k == key) return &v;
    return nullptr;
  }
};

// O núcleo decide a posição: remove o deslocamento horizontal da pelve ao longo do clipe
// (root motion das capturas), mantendo a altura e a rotação.
void lock_root_motion(ModelAnimation& a) {
  int pelvis = -1;
  for (int b = 0; b < a.boneCount; ++b) {
    std::string n = a.bones[b].name;
    std::transform(n.begin(), n.end(), n.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (n == "pelvis" || n == "hips" || n.find("pelvis") != std::string::npos) {
      pelvis = b;
      break;
    }
  }
  if (pelvis < 0 || a.frameCount < 2) return;
  const Vector3 base = a.framePoses[0][pelvis].translation;
  for (int f = 1; f < a.frameCount; ++f) {
    const Vector3 cur = a.framePoses[f][pelvis].translation;
    const float dx = cur.x - base.x, dz = cur.z - base.z;
    for (int b = 0; b < a.boneCount; ++b) {
      a.framePoses[f][b].translation.x -= dx;
      a.framePoses[f][b].translation.z -= dz;
    }
  }
}

// Nome do estado de apresentação para cada estado do núcleo.
std::string state_key(const MatchState& s, int p) {
  const FighterState& f = s.f[p];
  const bool low = fg::dir_is_down(f.input.dir_at(0));
  switch (f.act) {
    case Act::Idle: return s.phase == fg::Phase::RoundEnd && s.round_winner == p ? "win" : "idle";
    case Act::Crouch: return "crouch";
    case Act::WalkForward: return "walk_forward";
    case Act::WalkBack: return "walk_back";
    case Act::Run: return "run";
    case Act::Guard: return "guard";
    case Act::PreJump: return "jump_start";
    case Act::Air: return "jump";
    case Act::Landing: return "land";
    case Act::Hitstun: return f.crouch_stun ? "hit_crouch" : "hit";
    case Act::Blockstun: return low ? "block_crouch" : "block";
    case Act::Juggle: return "juggle";
    case Act::Knockdown: return "knockdown";
    case Act::Wakeup: return "wakeup";
    case Act::ThrowAttack: return "throw";
    case Act::Thrown: return "thrown";
    case Act::TechRecover: return "tech";
    case Act::KO: return "ko";
    case Act::Move: return "move";
  }
  return "idle";
}

bool loops(const std::string& key) {
  return key == "idle" || key == "crouch" || key == "walk_forward" || key == "walk_back" || key == "run" ||
         key == "guard" || key == "jump" || key == "win";
}

// Fallbacks entre estados parecidos, para quando um pacote não tem um clipe específico.
const char* fallback(const std::string& key) {
  if (key == "walk_back") return "walk_forward";
  if (key == "run") return "walk_forward";
  if (key == "guard" || key == "block") return "block_crouch";
  if (key == "block_crouch") return "block";
  if (key == "hit_crouch") return "hit";
  if (key == "juggle") return "knockdown";
  if (key == "jump_start" || key == "land") return "crouch";
  if (key == "thrown") return "hit";
  if (key == "tech") return "hit";
  if (key == "throw") return nullptr;
  if (key == "wakeup") return "idle";
  if (key == "win") return "idle";
  return nullptr;
}

void animate(Fighter3D& g, const App& a, int p) {
  if (!g.loaded || g.all.empty()) return;
  const FighterState& f = a.state.f[p];
  std::string key = state_key(a.state, p);
  const ClipRef* ref = nullptr;
  const MoveDef* m = fg::current_move(a.state, a.data, p);
  if (key == "move" && m) {
    ref = g.clip_for(g.look.moves, m->id);
    if (!ref) ref = g.clip_for(g.look.states, "idle");
    key = "move:" + m->id;
  } else {
    std::string k = key;
    for (int guard = 0; guard < 4 && !ref; ++guard) {
      ref = g.clip_for(g.look.states, k);
      if (!ref) {
        const char* fb = fallback(k);
        if (!fb) break;
        k = fb;
      }
    }
    if (!ref) ref = g.clip_for(g.look.states, "idle");
  }
  if (key != g.current_key) {
    g.current_key = key;
    g.local_frame = 0;
  } else if (f.hitstop == 0 && a.state.freeze == 0) {
    ++g.local_frame;
  }
  if (!ref) return;
  const int ai = g.find_anim(ref->clip);
  if (ai < 0) return;
  const ModelAnimation& anim = g.all[static_cast<size_t>(ai)];
  const int first = std::clamp(ref->from, 0, std::max(0, anim.frameCount - 1));
  const int last = ref->to >= 0 ? std::clamp(ref->to, first, anim.frameCount - 1) : anim.frameCount - 1;
  const int n = std::max(1, last - first + 1);
  int frame = 0;
  if (m && key.rfind("move:", 0) == 0) {
    // Tempo do golpe ditado pelo núcleo: o contato da animação cai no primeiro frame ativo.
    const int total = std::max(2, static_cast<int>(m->total_frames()));
    const int mf = std::clamp(static_cast<int>(f.move_frame), 1, total);
    const int contact = ref->contact >= 0 ? std::clamp(ref->contact - first, 0, n - 1) : (n - 1) / 3;
    const int startup = std::max(1, static_cast<int>(m->startup));
    if (mf <= startup) frame = startup <= 1 ? contact : (mf - 1) * contact / (startup - 1);
    else frame = contact + (mf - startup) * (n - 1 - contact) / std::max(1, total - startup);
  } else if (ref->loop || loops(key)) {
    frame = g.local_frame % n;
  } else {
    frame = std::min(g.local_frame, n - 1);
  }
  if (ref->reverse) frame = n - 1 - frame;
  UpdateModelAnimation(g.model, anim, first + frame);
}

void draw_bar(float x, float y, float w, float h, float frac, Color fg_c, Color bg_c, bool from_right) {
  frac = std::clamp(frac, 0.0f, 1.0f);
  DrawRectangleRec({x, y, w, h}, bg_c);
  const float fw = w * frac;
  DrawRectangleRec({from_right ? x + w - fw : x, y, fw, h}, fg_c);
  DrawRectangleLinesEx({x, y, w, h}, 2, {20, 20, 24, 255});
}

void draw_hud(const App& a, const View& v) {
  const MatchState& s = a.state;
  const float W = v.w;
  const float bar_w = W * 0.36f;
  for (int p = 0; p < 2; ++p) {
    const FighterState& f = s.f[p];
    const fg::CharacterDef& c = a.ch(p);
    const float x = p == 0 ? 40 : W - 40 - bar_w;
    draw_bar(x, 34, bar_w, 26, static_cast<float>(f.health) / static_cast<float>(c.health), kHealth, kHealthBack, p == 0);
    const std::string hp = fmt("%d", f.health) + fmt(" / %d", c.health);
    const std::string name = c.name + "  (" + (f.scheme == fg::ControlScheme::Classic ? "Clássico" : "Simplificado") + ")";
    if (p == 0) {
      text(name, x, 8, 22, kP1);
      text_right(hp, x + bar_w, 64, 18, kDim);
    } else {
      text_right(name, x + bar_w, 8, 22, kP2);
      text(hp, x, 64, 18, kDim);
    }
    // vitórias
    for (int r = 0; r < a.data.rules.rounds_to_win; ++r) {
      const float cx = p == 0 ? x + bar_w - 12 - static_cast<float>(r) * 22 : x + 12 + static_cast<float>(r) * 22;
      if (r < s.wins[p]) DrawCircle(static_cast<int>(cx), 96, 7, p == 0 ? kP1 : kP2);
      else DrawCircleLines(static_cast<int>(cx), 96, 7, kDim);
    }
    // super meter (barras)
    const i32 maxm = fg::max_meter(a.data.rules);
    const float mw = W * 0.22f;
    const float mx = p == 0 ? 40 : W - 40 - mw;
    const float my = v.h - 46;
    draw_bar(mx, my, mw, 16, static_cast<float>(f.meter) / static_cast<float>(maxm), kMeter, {30, 30, 50, 255}, p == 1);
    for (int i = 1; i < a.data.rules.meter_bars; ++i) {
      const float tx = mx + mw * static_cast<float>(i) / static_cast<float>(a.data.rules.meter_bars);
      DrawLineEx({tx, my}, {tx, my + 16}, 2, {20, 20, 24, 255});
    }
    const std::string bars = fmt("SUPER %d", f.meter / a.data.rules.meter_per_bar);
    if (p == 0) text(bars, mx, my - 22, 18, kMeter);
    else text_right(bars, mx + mw, my - 22, 18, kMeter);
  }

  // tempo
  std::string t = (s.options & fg::option::NoTimer) ? "TREINO" : fmt("%d", (s.timer_frames + 59) / 60);
  text_center(t, W / 2, 26, (s.options & fg::option::NoTimer) ? 26 : 42, kText);

  // Maré: linha d'água entre os dois lados
  const auto& mr = a.data.rules.mare;
  const float mw = W * 0.30f, mx = W / 2 - mw / 2, my = 104;
  DrawRectangleRec({mx, my, mw, 14}, {26, 32, 44, 255});
  const float frac = static_cast<float>(s.mare) / static_cast<float>(mr.limit);
  const float cx = W / 2;
  if (s.mare > 0) DrawRectangleRec({cx - mw / 2 * frac, my, mw / 2 * frac, 14}, kMareP1);
  if (s.mare < 0) DrawRectangleRec({cx, my, -mw / 2 * frac, 14}, kMareP2);
  const float th = mw / 2 * static_cast<float>(mr.threshold) / static_cast<float>(mr.limit);
  DrawLineEx({cx - th, my - 3}, {cx - th, my + 17}, 2, kText);
  DrawLineEx({cx + th, my - 3}, {cx + th, my + 17}, 2, kText);
  DrawLineEx({cx, my - 5}, {cx, my + 19}, 2, kDim);
  DrawRectangleLinesEx({mx, my, mw, 14}, 2, {20, 20, 24, 255});
  text_center(fmt("MARÉ %+d", s.mare), cx, my + 18, 18, kDim);
  for (int p = 0; p < 2; ++p) {
    const i32 favor = fg::mare_favor(s, p);
    std::string tag;
    if (favor >= mr.threshold) tag = "Arrebentação pronta (MP+MK após normal que fez contato)";
    if (favor <= -mr.threshold) tag = "Contracorrente pronta (MP+MK no bloqueio)";
    if (tag.empty()) continue;
    if (p == 0) text(tag, 40, 120, 17, kMareP1);
    else text_right(tag, W - 40, 120, 17, kMareP2);
  }

  // combo
  for (int p = 0; p < 2; ++p) {
    if (a.combo_show[p] <= 0 || a.combo_hits[p] < 2) continue;
    const float y = v.h * 0.30f;
    const std::string hits = fmt("%d HITS", a.combo_hits[p]);
    const std::string dmg = fmt("%d de dano", a.combo_damage[p]);
    if (p == 0) {
      text(hits, 40, y, 40, kP1);
      text(dmg, 40, y + 42, 22, kText);
    } else {
      text_right(hits, W - 40, y, 40, kP2);
      text_right(dmg, W - 40, y + 42, 22, kText);
    }
  }

  // vantagem
  if (a.adv.has_result) {
    const int r = a.adv.result;
    const Color c = r > 0 ? Color{90, 220, 130, 255} : (r < 0 ? kHit : kText);
    text_center(fmt("P%d ", a.adv.attacker + 1) + fmt("%+d ", r) + a.adv.label, W / 2, 150, 24, c);
  }

  // faixa central (round, counter, Maré...)
  if (a.banner_ttl > 0) {
    const float size = 64;
    text_center(a.banner, W / 2 + 3, v.h * 0.36f + 3, size, {0, 0, 0, 160});
    text_center(a.banner, W / 2, v.h * 0.36f, size, a.banner_color);
    if (s.phase == fg::Phase::MatchEnd) text_center("Enter: revanche", W / 2, v.h * 0.36f + 72, 24, kText);
  }
}

void draw_frame_meter(const App& a, const View& v) {
  const float cell = std::min(13.0f, (v.w - 160) / static_cast<float>(App::kMeterLen));
  const float x0 = v.w / 2 - cell * static_cast<float>(App::kMeterLen) / 2;
  const float y0 = v.h - 108;
  for (int p = 0; p < 2; ++p) {
    const float y = y0 + static_cast<float>(p) * (cell + 4);
    text_right(fmt("P%d", p + 1), x0 - 8, y - 2, 16, p == 0 ? kP1 : kP2);
    for (int i = 0; i < App::kMeterLen; ++i) {
      const FrameCat c = i < a.frame_meter_len ? a.meter[static_cast<size_t>(p)][static_cast<size_t>(i)] : FrameCat::Free;
      DrawRectangleRec({x0 + static_cast<float>(i) * cell, y, cell - 1, cell}, i < a.frame_meter_len ? cat_color(c) : Color{30, 34, 44, 255});
    }
  }
  const char* legend[] = {"startup", "ativo", "recovery", "atordoado", "bloqueio", "movimento", "hitstop"};
  const FrameCat cats[] = {FrameCat::Startup, FrameCat::Active, FrameCat::Recovery, FrameCat::Hitstun,
                           FrameCat::Blockstun, FrameCat::Movement, FrameCat::Hitstop};
  float lx = x0;
  for (int i = 0; i < 7; ++i) {
    DrawRectangleRec({lx, y0 - 20, 10, 10}, cat_color(cats[i]));
    text(legend[i], lx + 14, y0 - 24, 15, kDim);
    lx += 24 + text_width(legend[i], 15);
  }
}

void draw_input_log(const App& a, const View& v, int p) {
  const float x = p == 0 ? 40.0f : v.w - 40.0f;
  float y = v.h * 0.42f;
  for (const auto& e : a.log[p].entries) {
    const std::string s = fmt("%2d  ", e.frames) + dir_glyph(e.dir) + "  " + buttons_text(e.buttons);
    if (p == 0) text(s, x, y, 17, kDim);
    else text_right(s, x, y, 17, kDim);
    y += 19;
    if (y > v.h - 140) break;
  }
}

void draw_feed(const App& a, const View& v) {
  float y = 180;
  for (const auto& f : a.feed) {
    const Color c = Fade(f.player == 0 ? kP1 : kP2, std::min(1.0f, static_cast<float>(f.ttl) / 60.0f));
    text_center(fmt("P%d  ", f.player + 1) + f.line, v.w / 2, y, 19, c);
    y += 22;
  }
}

void draw_help(const App& a, const View& v) {
  const float w = 760, h = 520;
  const float x = v.w / 2 - w / 2, y = v.h / 2 - h / 2;
  DrawRectangleRec({x, y, w, h}, {10, 12, 18, 235});
  DrawRectangleLinesEx({x, y, w, h}, 2, kDim);
  float ty = y + 18;
  auto line = [&](const std::string& s, Color c = kText, float size = 19) {
    text(s, x + 24, ty, size, c);
    ty += size + 7;
  };
  line("Projeto Estaleiro — controles", kText, 28);
  line("Visual em construção: modelos e animações de pacotes gratuitos com licença comercial.", kDim, 17);
  ty += 6;
  line("P1 (teclado):  W A S D movem    U I O = LP MP HP    J K L = LK MK HK    Espaço = SP", kP1);
  line("P2 (teclado):  setas movem    Num 4 5 6 = LP MP HP    Num 1 2 3 = LK MK HK    Num 0 = SP", kP2);
  line("Controle:  X Y RB = LP MP HP    A B RT = LK MK HK    LB = SP    (1º controle = P1)", kText);
  ty += 6;
  line("F1  treino / partida (melhor de 3)        F2  esquema do P1: Clássico / Simplificado");
  line("F3  comportamento do P2                    F4  recomeçar");
  line("F5  pausar        F6  avançar 1 frame (pausado)        F7 / F8  trocar personagem P1 / P2");
  line("F9  mostrar caixas de colisão (treino)       Tab  esta ajuda");
  ty += 6;
  line("Comandos: 236 = baixo, baixo-frente, frente. 623 = frente, baixo, baixo-frente.", kDim, 17);
  line("Maré alta: MP+MK cancela um normal que fez contato (Arrebentação).", kDim, 17);
  line("Maré baixa: MP+MK durante o bloqueio empurra o oponente (Contracorrente).", kDim, 17);
  line("Arremesso: LP+LK perto. Tech: LP+LK ao ser agarrado.", kDim, 17);
  ty += 6;
  line(std::string("P2 agora: ") + dummy_name(a.dummy), kP2, 18);
  line(std::string("Modo: ") + (a.training ? "Treino (vida volta ao fim do combo)" : "Partida"), kText, 18);
}

}  // namespace

namespace {

std::string join_path(const std::string& a, const std::string& b) {
  if (a.empty()) return b;
  const char last = a.back();
  return (last == '/' || last == '\\') ? a + b : a + "/" + b;
}

void apply_shader(Model& m, Shader sh) {
  for (int i = 0; i < m.materialCount; ++i) {
    m.materials[i].shader = sh;
    // O glTF chega sem mipmaps: sem eles, texturas repetidas no chão serrilham à distância.
    Texture2D& t = m.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture;
    if (t.id > 1 && t.mipmaps <= 1) {
      GenTextureMipmaps(&t);
      SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
    }
  }
}

// Caixas de colisão em 3D (somente no treino, tecla F9): ferramenta de leitura sobre os modelos.
void draw_boxes_3d(const App& a, int p) {
  fg::DebugBoxes boxes;
  fg::debug_boxes(a.state, a.data, p, boxes);
  for (int i = 0; i < boxes.count; ++i) {
    const fg::WorldBox& b = boxes.boxes[i];
    const float u = 1.0f / static_cast<float>(fg::kUnitsPerMeter);
    const Vector3 c = {(static_cast<float>(b.left + b.right) / 2) * u, (static_cast<float>(b.bottom + b.top) / 2) * u, 0};
    const float w = static_cast<float>(b.right - b.left) * u, h = static_cast<float>(b.top - b.bottom) * u;
    Color col = boxes.push[i] ? kPush : (boxes.kinds[i] == BoxKind::Hit ? kHit : boxes.kinds[i] == BoxKind::Throw ? kThrow : (p == 0 ? kP1 : kP2));
    if (boxes.push[i] && !a.show_push) continue;
    DrawCubeWiresV(c, {w, h, 0.02f}, col);
  }
}

struct Options {
  std::string smoke_out;  // --smoke <png>: roda uma luta roteirizada e salva uma captura
  int smoke_frames = 240;
};

// Roteiro do modo --smoke (usado pela CI para conferir que tudo carrega e anima).
InputBits smoke_input(int p, int frame) {
  if (p == 1) return frame % 120 < 40 ? btn::Left : 0;
  const int t = frame % 90;
  if (t < 30) return btn::Right;
  if (t == 34) return btn::MP;
  if (t == 60) return btn::HK;
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  Options opt;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--smoke" && i + 1 < argc) opt.smoke_out = argv[++i];
    else if (a == "--frames" && i + 1 < argc) opt.smoke_frames = std::max(1, std::atoi(argv[++i]));
  }
  const bool smoke = !opt.smoke_out.empty();
  // --anims <arquivo.glb>: lista os clipes de animação (ajuda a escrever data/presentation/*.json).
  if (argc == 3 && std::string(argv[1]) == "--anims") {
    int n = 0;
    ModelAnimation* an = LoadModelAnimations(argv[2], &n);
    for (int i = 0; i < n; ++i) std::printf("%-40s %4d frames\n", an[i].name, an[i].frameCount);
    if (an) UnloadModelAnimations(an, n);
    return n > 0 ? 0 : 1;
  }

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
  InitWindow(1600, 900, "Projeto Estaleiro");
  // Monitor de ~60 Hz: o VSync dita o ritmo. Somar o limitador da raylib faz os dois brigarem, e no
  // Windows 11 o Sleep perde precisão com a janela em segundo plano (caía para ~42 FPS).
  // Em monitores de outra frequência, o limitador segura a simulação em 60 Hz.
  const int hz = GetMonitorRefreshRate(GetCurrentMonitor());  // 59,94 Hz aparece como 59
  SetTargetFPS(hz >= 59 && hz <= 61 ? 0 : 60);
  SetExitKey(KEY_NULL);
  load_font();

  App app;
  std::string base_dir;
  {
    std::vector<std::string> dirs = {GetApplicationDirectory(), "."};
#ifdef FG_ROOT_DIR
    dirs.push_back(FG_ROOT_DIR);
#endif
    for (const auto& d : dirs) {
      const std::string data_dir = join_path(d, "data");
      if (!DirectoryExists(data_dir.c_str())) continue;
      GameData gd;
      const fg::LoadResult r = fg::load_game_data(data_dir, gd);
      if (r.ok) {
        app.data = std::move(gd);
        app.data_error.clear();
        base_dir = d;
        break;
      }
      app.data_error = "Erro nos dados em " + data_dir + ":\n";
      for (const auto& e : r.errors) app.data_error += "  " + e + "\n";
    }
    if (app.data.characters.size() < 2 && app.data_error.empty())
      app.data_error = "Pasta 'data' não encontrada ao lado do executável.";
  }

  // Visual: modelos, animações e arena descritos em data/presentation (nunca formas geradas no código).
  Shader lit = LoadShaderFromMemory(kLitVS, kLitFS);
  const int loc_light = GetShaderLocation(lit, "lightDir");
  const int loc_lcol = GetShaderLocation(lit, "lightColor");
  const int loc_amb = GetShaderLocation(lit, "ambient");
  const int loc_view = GetShaderLocation(lit, "viewPos");
  const int loc_fog = GetShaderLocation(lit, "fogColor");
  const int loc_fnear = GetShaderLocation(lit, "fogNear");
  const int loc_ffar = GetShaderLocation(lit, "fogFar");
  lit.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(lit, "matModel");
  lit.locs[SHADER_LOC_MATRIX_NORMAL] = GetShaderLocation(lit, "matNormal");

  Fighter3D fighters[2];
  ArenaLook arena_look;
  Model arena{};
  bool arena_loaded = false;
  Texture2D sky_tex{};
  std::vector<std::string> art_missing;
  const std::string assets_dir = join_path(base_dir, "assets");
  const std::string pres_dir = join_path(join_path(base_dir, "data"), "presentation");

  auto load_fighter = [&](int p) {
    Fighter3D& g = fighters[p];
    g.unload();
    g = Fighter3D{};
    const std::string id = app.data.characters[static_cast<size_t>(app.setup.character[p])].id;
    std::string err;
    if (!load_character_look(join_path(pres_dir, id + ".json"), g.look, err)) {
      art_missing.push_back(err);
      return;
    }
    const std::string path = join_path(assets_dir, g.look.model_path);
    if (!FileExists(path.c_str())) {
      art_missing.push_back("modelo não encontrado: " + path);
      return;
    }
    g.model = LoadModel(path.c_str());
    g.anims = LoadModelAnimations(path.c_str(), &g.anim_count);
    for (int i = 0; i < g.anim_count; ++i) {
      lock_root_motion(g.anims[i]);
      g.all.push_back(g.anims[i]);
    }
    for (const auto& f : g.look.anim_files) {
      const std::string ap = join_path(assets_dir, f);
      int n = 0;
      ModelAnimation* an = FileExists(ap.c_str()) ? LoadModelAnimations(ap.c_str(), &n) : nullptr;
      if (!an) {
        art_missing.push_back("animações não encontradas: " + ap);
        continue;
      }
      g.extra.emplace_back(an, n);
      for (int i = 0; i < n; ++i) {
        if (an[i].boneCount != g.model.boneCount) continue;
        lock_root_motion(an[i]);
        g.all.push_back(an[i]);
      }
    }
    apply_shader(g.model, lit);
    g.loaded = g.model.meshCount > 0;
    if (!g.loaded) art_missing.push_back("modelo inválido: " + path);
  };

  if (app.data_error.empty()) {
    app.restart();
    for (int p = 0; p < 2; ++p) load_fighter(p);
    std::string err;
    if (load_arena_look(join_path(pres_dir, "arena.json"), arena_look, err)) {
      const std::string path = join_path(assets_dir, arena_look.model_path);
      if (FileExists(path.c_str())) {
        arena = LoadModel(path.c_str());
        apply_shader(arena, lit);
        arena_loaded = arena.meshCount > 0;
      } else {
        art_missing.push_back("arena não encontrada: " + path);
      }
      if (!arena_look.sky_path.empty()) {
        const std::string sp = join_path(assets_dir, arena_look.sky_path);
        if (FileExists(sp.c_str())) {
          sky_tex = LoadTexture(sp.c_str());
          SetTextureFilter(sky_tex, TEXTURE_FILTER_BILINEAR);
        }
      }
    } else {
      art_missing.push_back(err);
    }
  }

  // Sombra suave sob cada lutador (textura radial, não geometria).
  Image shadow_img = GenImageGradientRadial(128, 128, 0.0f, {0, 0, 0, 150}, {0, 0, 0, 0});
  Texture2D shadow_tex = LoadTextureFromImage(shadow_img);
  UnloadImage(shadow_img);

  Camera3D cam{};
  cam.up = {0, 1, 0};
  cam.fovy = 32.0f;
  cam.projection = CAMERA_PERSPECTIVE;
  float cam_x = 0.0f, cam_z = 6.0f;
  int smoke_frame = 0;

  while (!WindowShouldClose()) {
    if (!app.data_error.empty()) {
      BeginDrawing();
      ClearBackground(kBg);
      text(app.data_error, 40, 40, 22, kHit);
      EndDrawing();
      if (smoke) break;
      continue;
    }

    const int nchar = static_cast<int>(app.data.characters.size());
    if (IsKeyPressed(KEY_TAB)) app.show_help = !app.show_help;
    if (IsKeyPressed(KEY_F1)) {
      app.training = !app.training;
      app.restart();
    }
    if (IsKeyPressed(KEY_F2)) {
      app.setup.scheme[0] = app.setup.scheme[0] == fg::ControlScheme::Classic ? fg::ControlScheme::Simplified
                                                                               : fg::ControlScheme::Classic;
      app.restart();
    }
    if (IsKeyPressed(KEY_F3)) app.dummy = static_cast<Dummy>((static_cast<int>(app.dummy) + 1) % static_cast<int>(Dummy::Count));
    if (IsKeyPressed(KEY_F4)) app.restart();
    if (IsKeyPressed(KEY_F5)) app.paused = !app.paused;
    if (IsKeyPressed(KEY_F7) || IsKeyPressed(KEY_F8)) {
      const int p = IsKeyPressed(KEY_F7) ? 0 : 1;
      app.setup.character[p] = (app.setup.character[p] + 1) % nchar;
      app.restart();
      art_missing.clear();
      load_fighter(p);
    }
    if (IsKeyPressed(KEY_F9)) app.show_boxes = !app.show_boxes;
    if (app.state.phase == fg::Phase::MatchEnd && IsKeyPressed(KEY_ENTER)) app.restart();

    fg::FrameInput in;
    if (smoke) {
      in.p[0] = smoke_input(0, smoke_frame);
      in.p[1] = smoke_input(1, smoke_frame);
    } else {
      in.p[0] = static_cast<InputBits>(read_keys(kKeysP1) | read_pad(0));
      if (app.dummy == Dummy::Human) in.p[1] = static_cast<InputBits>(read_keys(kKeysP2) | read_pad(1));
      else in.p[1] = dummy_input(app.dummy, app.state.f[1]);
    }

    const bool advance = !app.paused || IsKeyPressed(KEY_F6);
    if (advance) {
      for (int p = 0; p < 2; ++p) app.log[p].push(fg::numpad_direction(in.p[p], app.state.f[p].facing), in.p[p]);
      app.tick(in);
      for (int p = 0; p < 2; ++p) animate(fighters[p], app, p);
    }

    // câmera lateral que acompanha os dois e se afasta quando eles se separam
    const float u = 1.0f / static_cast<float>(fg::kUnitsPerMeter);
    const float x0 = static_cast<float>(app.state.f[0].x) * u, x1 = static_cast<float>(app.state.f[1].x) * u;
    const float sep = std::abs(x1 - x0);
    const float half = static_cast<float>(app.data.rules.stage_half_width) * u;
    const float want_z = std::clamp(4.6f + sep * 0.55f, 5.2f, 8.5f);
    const float want_x = std::clamp((x0 + x1) / 2, -half + 2.5f, half - 2.5f);
    cam_x += (want_x - cam_x) * 0.18f;
    cam_z += (want_z - cam_z) * 0.10f;
    cam.position = {cam_x, 1.45f, cam_z};
    cam.target = {cam_x, 1.05f, 0.0f};

    const float light_dir[3] = {-0.35f, -1.0f, -0.45f};
    const float light_col[3] = {1.0f, 0.95f, 0.88f};
    const float amb[3] = {0.42f, 0.45f, 0.52f};
    const float view[3] = {cam.position.x, cam.position.y, cam.position.z};
    const float fogc[3] = {arena_look.fog.r / 255.0f, arena_look.fog.g / 255.0f, arena_look.fog.b / 255.0f};
    const float fnear = 12.0f, ffar = 60.0f;
    SetShaderValue(lit, loc_light, light_dir, SHADER_UNIFORM_VEC3);
    SetShaderValue(lit, loc_lcol, light_col, SHADER_UNIFORM_VEC3);
    SetShaderValue(lit, loc_amb, amb, SHADER_UNIFORM_VEC3);
    SetShaderValue(lit, loc_view, view, SHADER_UNIFORM_VEC3);
    SetShaderValue(lit, loc_fog, fogc, SHADER_UNIFORM_VEC3);
    SetShaderValue(lit, loc_fnear, &fnear, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lit, loc_ffar, &ffar, SHADER_UNIFORM_FLOAT);

    View v{static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
    BeginDrawing();
    ClearBackground(arena_look.sky);
    if (sky_tex.id != 0) {
      // céu equirretangular: faixa do horizonte, com leve paralaxe da câmera
      const float tw = static_cast<float>(sky_tex.width), th = static_cast<float>(sky_tex.height);
      const float view_w = tw * (cam.fovy * v.w / v.h) / 360.0f;
      const float src_x = tw * 0.5f - view_w / 2 + cam_x * 6.0f;
      const float src_h = view_w * v.h / v.w;
      DrawTexturePro(sky_tex, {src_x, th * 0.5f - src_h * 0.62f, view_w, src_h}, {0, 0, v.w, v.h}, {0, 0}, 0, WHITE);
    }
    BeginMode3D(cam);
    if (arena_loaded) DrawModel(arena, arena_look.offset, arena_look.scale, WHITE);
    for (int p = 0; p < 2; ++p) {
      const FighterState& f = app.state.f[p];
      const Vector3 pos = {static_cast<float>(f.x) * u, static_cast<float>(f.y) * u, 0.0f};
      DrawBillboardPro(cam, shadow_tex, {0, 0, 128, 128}, {pos.x, 0.01f, 0}, {0, 0, -1}, {1.1f, 0.5f}, {0.55f, 0.25f}, 0, WHITE);
      Fighter3D& g = fighters[p];
      if (!g.loaded) continue;
      const float yaw = (f.facing > 0 ? 90.0f : -90.0f) + g.look.yaw_offset;
      Color tint = g.look.tint;
      if (app.setup.character[0] == app.setup.character[1] && p == 1) tint = {205, 215, 255, 255};
      DrawModelEx(g.model, pos, {0, 1, 0}, yaw, {g.look.scale, g.look.scale, g.look.scale}, tint);
    }
    if (app.show_boxes)
      for (int p = 0; p < 2; ++p) draw_boxes_3d(app, p);
    EndMode3D();

    if (app.state.freeze > 0) DrawRectangle(0, 0, static_cast<int>(v.w), static_cast<int>(v.h), {0, 0, 0, 90});
    draw_hud(app, v);
    if (app.training) {
      draw_frame_meter(app, v);
      draw_input_log(app, v, 0);
      draw_input_log(app, v, 1);
      draw_feed(app, v);
    }
    if (!art_missing.empty()) {
      float y = v.h * 0.5f;
      text_center("Arte ausente (coloque os arquivos em assets/):", v.w / 2, y, 22, kHit);
      for (const auto& m : art_missing) text_center(m, v.w / 2, y += 26, 18, kText);
    }
    text("Tab: controles    F1: " + std::string(app.training ? "treino" : "partida") +
             (app.paused ? "    PAUSADO (F6 avança)" : ""),
         40, v.h - 22, 17, kDim);
    if (app.show_help) draw_help(app, v);
    EndDrawing();

    if (smoke && ++smoke_frame >= opt.smoke_frames) {
      TakeScreenshot(opt.smoke_out.c_str());
      break;
    }
  }

  for (auto& g : fighters) g.unload();
  if (arena_loaded) UnloadModel(arena);
  if (sky_tex.id != 0) UnloadTexture(sky_tex);
  UnloadTexture(shadow_tex);
  UnloadShader(lit);
  if (g_font_loaded) UnloadFont(g_font);
  CloseWindow();
  return art_missing.empty() ? 0 : 2;
}
