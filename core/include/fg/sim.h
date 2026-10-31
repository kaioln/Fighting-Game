// Simulação determinística: avança o estado exatamente um frame (1/60 s).
#pragma once
#include "fg/data.h"
#include "fg/state.h"

namespace fg {

struct FrameInput {
  InputBits p[2] = {0, 0};
};

enum class EventType : u8 {
  RoundStart,
  Fight,
  MoveStart,
  Hit,
  CounterHit,
  PunishCounter,
  Block,
  Armor,
  ThrowConnect,
  ThrowTech,
  ThrowComplete,
  Knockdown,
  KO,
  TimeOver,
  RoundEnd,
  MatchEnd,
  SuperFreeze,
  Arrebentacao,
  Contracorrente,
};

// Eventos para a apresentação (som, VFX, câmera). Não fazem parte do estado.
struct Event {
  EventType type = EventType::Hit;
  i8 player = -1;  // quem causou o evento
  i16 move = -1;
  i32 x = 0;
  i32 y = 0;
  i32 value = 0;   // dano, frames etc.
};

struct StepEvents {
  static constexpr int kMax = 32;
  Event items[kMax];
  int count = 0;
  void add(const Event& e) {
    if (count < kMax) items[count++] = e;
  }
  void clear() { count = 0; }
};

struct MatchSetup {
  i32 character[2] = {0, 1};
  ControlScheme scheme[2] = {ControlScheme::Classic, ControlScheme::Classic};
  u32 seed = 1;
  u8 options = 0;
};

void init_match(MatchState& s, const GameData& d, const MatchSetup& setup);

// Avança um frame. `events` pode ser nulo.
void step(MatchState& s, const GameData& d, const FrameInput& in, StepEvents* events = nullptr);

// Utilidades para ferramentas e testes.
bool is_actionable(const MatchState& s, const GameData& d, int player);
i32 mare_favor(const MatchState& s, int player);
i32 max_meter(const Rules& r);
const MoveDef* current_move(const MatchState& s, const GameData& d, int player);

// Somente ferramentas/testes: inicia um golpe diretamente, sem passar pelo interpretador de comandos.
void debug_start_move(MatchState& s, const GameData& d, int player, i16 move);

// Caixas atuais em coordenadas de mundo (para o modo de depuração e testes).
struct DebugBoxes {
  static constexpr int kMax = 16;
  WorldBox boxes[kMax];
  BoxKind kinds[kMax];
  bool push[kMax];
  int count = 0;
};
void debug_boxes(const MatchState& s, const GameData& d, int player, DebugBoxes& out);

}  // namespace fg
