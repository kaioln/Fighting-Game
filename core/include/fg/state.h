// Estado competitivo completo. É a fonte de verdade do jogo.
// Regras: tipos de tamanho fixo, sem ponteiros, sem alocação dinâmica, trivialmente copiável.
// Um snapshot de rollback é uma cópia deste struct.
#pragma once
#include <type_traits>

#include "fg/input.h"
#include "fg/types.h"

namespace fg {

inline constexpr int kInputHistory = 64;

enum class Act : u8 {
  Idle,
  Crouch,
  WalkForward,
  WalkBack,
  Run,
  Guard,       // guarda em pé por proximidade (segurando trás com um ataque vindo)
  PreJump,
  Air,
  Landing,
  Move,        // executando um golpe ou movimento definido em dados
  Hitstun,
  Blockstun,
  Juggle,      // atingido no ar, caindo
  Knockdown,
  Wakeup,
  ThrowAttack, // segurando o oponente (janela de tech)
  Thrown,
  TechRecover, // separação após tech de arremesso
  KO,
};

enum class JumpKind : u8 { None, Normal, Hop };

enum class Phase : u8 { Intro, Fight, RoundEnd, MatchEnd };

namespace option {
inline constexpr u8 NoTimer = 1u << 0;      // treino: tempo infinito
inline constexpr u8 SkipIntro = 1u << 1;    // começa direto na luta
inline constexpr u8 NoRoundEnd = 1u << 2;   // treino: vida não zera rounds (restaura ao fim do combo)
}  // namespace option

enum class ControlScheme : u8 { Classic, Simplified };

struct InputHistory {
  u8 dir[kInputHistory] = {};    // direção numérica relativa ao lado no momento do input
  u16 btn[kInputHistory] = {};   // botões pressionados naquele frame
  u32 head = 0;                  // número de frames registrados (índice = head % N)

  void push(u8 d, u16 b) {
    dir[head % kInputHistory] = d;
    btn[head % kInputHistory] = b;
    ++head;
  }
  // `ago` = 0 é o frame mais recente.
  u8 dir_at(u32 ago) const { return ago < head && ago < kInputHistory ? dir[(head - 1 - ago) % kInputHistory] : 5; }
  u16 btn_at(u32 ago) const { return ago < head && ago < kInputHistory ? btn[(head - 1 - ago) % kInputHistory] : 0; }
  u16 pressed_at(u32 ago) const { return static_cast<u16>(btn_at(ago) & ~btn_at(ago + 1)); }
  u16 released_at(u32 ago) const { return static_cast<u16>(~btn_at(ago) & btn_at(ago + 1)); }
};

struct FighterState {
  i32 character = 0;
  ControlScheme scheme = ControlScheme::Classic;

  i32 x = 0;
  i32 y = 0;
  i32 vx = 0;  // em coordenadas de mundo
  i32 vy = 0;
  i32 gravity = 0;
  i8 facing = 1;
  Act act = Act::Idle;
  JumpKind jump_kind = JumpKind::None;
  i8 jump_dir = 0;  // -1 trás, 0 neutro, 1 frente (relativo ao lado no início do salto)

  i16 move = -1;       // índice do golpe atual
  i16 move_frame = 0;  // frame atual do golpe (1 = primeiro)
  i16 timer = 0;       // contador genérico do estado atual (stun, knockdown...)
  u8 hits_done = 0;    // máscara de segmentos que já conectaram neste golpe
  u8 armor_used = 0;
  bool move_connected = false;  // golpe atual acertou ou foi bloqueado (libera cancelamentos)
  bool move_whiffed_recovery = false;

  i32 health = 0;
  i32 meter = 0;

  i16 hitstop = 0;
  i32 push_remaining = 0;  // empurrão pendente (mundo, com sinal)

  // Como defensor
  i16 combo_hits = 0;            // hits recebidos no combo atual
  i32 combo_damage = 0;
  i32 combo_scaling_extra = 0;   // redução adicional do golpe inicial
  i16 juggle = 0;
  bool combo_counter_hit = false;
  bool knockdown_pending = false;

  // Arremesso
  i16 throw_timer = 0;
  i16 throw_move = -1;

  // Maré: frames seguidos recuando
  i16 retreat_frames = 0;

  // Inputs
  InputHistory input;
  u32 consumed_press = 0;  // número do frame (head) até o qual pressões já foram consumidas
  i16 buffer_bonus = 0;
  i16 throw_protect = 0;    // frames de imunidade a arremesso (ao levantar)     // estende o buffer após hitstop ou ao levantar
  bool air_action_used = false;
  bool crouch_stun = false;  // recebeu o golpe agachado (mantém a hurtbox baixa no stun)
  i16 damage_permil_override = 1000;  // penalidade do Simplificado aplicada ao golpe atual


};

struct MatchState {
  u32 frame = 0;
  Phase phase = Phase::Intro;
  i16 phase_timer = 0;
  u8 round = 1;
  u8 wins[2] = {0, 0};
  i8 round_winner = -1;  // -1 sem vencedor, 2 empate
  i8 match_winner = -1;  // -1 em andamento, 2 empate
  i32 timer_frames = 0;
  i32 mare = 0;          // positivo favorece o jogador 1 (índice 0)
  i16 freeze = 0;        // congelamento global (super)
  i8 freeze_owner = -1;
  u8 options = 0;        // ver namespace `option`
  u32 rng = 0x9E3779B9u;
  FighterState f[2];
};

static_assert(std::is_trivially_copyable_v<MatchState>, "MatchState precisa ser trivialmente copiável");

}  // namespace fg
