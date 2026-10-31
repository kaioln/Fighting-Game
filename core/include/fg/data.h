// Dados imutáveis do jogo: personagens, golpes, comandos e regras.
// Carregados uma única vez (ver loader.h). Nunca fazem parte do estado de rollback:
// o estado guarda apenas índices para estes dados.
#pragma once
#include <string>
#include <vector>

#include "fg/types.h"

namespace fg {

enum class HitLevel : u8 { Mid, Low, Overhead };

enum class MoveKind : u8 {
  Normal,
  CommandNormal,
  Special,
  Enhanced,
  Super,
  Throw,
  Movement,  // dash, backdash, Arrebentação etc.
  Mare,      // Arrebentação / Contracorrente
};

enum class BoxKind : u8 { Hit, Hurt, Throw };

// Máscara de destinos de cancelamento.
namespace cancel {
inline constexpr u8 Chain = 1u << 0;     // para golpes listados em `chain_targets`
inline constexpr u8 Special = 1u << 1;   // para especiais e reforçados
inline constexpr u8 Super = 1u << 2;     // para supers
inline constexpr u8 Jump = 1u << 3;      // para salto/hop
inline constexpr u8 Mare = 1u << 4;      // para Arrebentação
inline constexpr u8 Any = 0xFF;
}  // namespace cancel

// Intervalos usam frames do golpe numerados a partir de 1 (frame 1 = primeiro frame do golpe).
struct BoxSpan {
  i16 from = 1;
  i16 to = 1;
  BoxKind kind = BoxKind::Hurt;
  Box box;
};

struct CancelSpan {
  i16 from = 1;
  i16 to = 1;
  u8 mask = 0;
  bool on_whiff = false;  // se falso, o cancelamento exige contato (hit ou bloqueio)
};

struct FrameSpan {
  i16 from = 1;
  i16 to = 1;
};

struct ArmorSpan {
  i16 from = 1;
  i16 to = 1;
  u8 hits = 1;
};

// Velocidade aplicada pelo golpe (root motion assado em dados, nunca lido da animação).
struct VelocitySpan {
  i16 from = 1;
  i16 to = 1;
  i32 vx = 0;  // unidades por frame, positivo = para a frente
  i32 vy = 0;  // aplicado uma vez no frame `from` se `impulse`, senão por frame
  bool impulse = false;
};

// Um "hit" do golpe. Golpes multi-hit têm vários segmentos.
struct HitSegment {
  i16 from = 1;  // janela ativa deste hit
  i16 to = 1;
  i32 damage = 0;
  i32 chip = 0;           // dano em bloqueio (só especiais/supers)
  i16 hitstun = 0;
  i16 blockstun = 0;
  i16 hitstop = 0;
  i32 pushback_hit = 0;   // distância total de empurrão (unidades)
  i32 pushback_block = 0;
  HitLevel level = HitLevel::Mid;
  bool knockdown = false;  // derruba no chão
  bool launch = false;     // lança no ar (juggle)
  i32 launch_vx = 0;       // velocidade horizontal do lançamento (para trás do defensor)
  i32 launch_vy = 0;
  u8 juggle_cost = 1;
  bool hits_airborne = true;
};

struct MoveDef {
  std::string id;
  MoveKind kind = MoveKind::Normal;
  i16 startup = 1;  // inclui o primeiro frame ativo (convenção)
  i16 active = 1;
  i16 recovery = 0;
  i16 landing_recovery = 0;  // para golpes aéreos que tocam o chão
  bool airborne = false;     // golpe executado no ar
  bool crouching = false;    // usa a postura agachada (hurtbox baixa)
  bool ends_on_landing = false;

  std::vector<HitSegment> hits;
  std::vector<BoxSpan> boxes;
  std::vector<CancelSpan> cancels;
  std::vector<std::string> chain_targets;
  std::vector<i16> chain_target_ids;  // resolvido no carregamento
  std::vector<FrameSpan> strike_invuln;
  std::vector<FrameSpan> throw_invuln;
  std::vector<FrameSpan> airborne_frames;  // frames em que o lutador conta como "no ar" (ex.: pulo do golpe)
  std::vector<ArmorSpan> armor;
  std::vector<VelocitySpan> velocity;

  // Arremessos
  i32 throw_damage = 0;
  bool throw_swaps_sides = false;
  bool techable = true;        // arremessos de comando não podem ser escapados
  i32 throw_end_distance = 0;  // distância entre os lutadores ao fim do arremesso

  // Recursos
  i32 meter_cost = 0;      // em unidades de super meter (1 barra = Rules::meter_per_bar)
  i32 meter_gain_hit = 0;
  i32 meter_gain_block = 0;
  i32 meter_gain_whiff = 0;

  // Escala
  i32 starter_scaling_permil = 0;  // redução adicional quando o golpe inicia um combo
  i32 min_scaling_permil = 0;      // piso específico (ex.: supers)

  i16 super_freeze = 0;  // congelamento cinematográfico no início (supers)

  // Referências para a apresentação (sem efeito na simulação).
  std::string animation;
  std::string vfx;
  std::string sfx;

  i16 total_frames() const { return static_cast<i16>(startup - 1 + active + recovery); }
  i16 first_active() const { return startup; }
  i16 last_active() const { return static_cast<i16>(startup + active - 1); }
};

enum class TriggerButtons : u8 {
  AnyPunch,
  AnyKick,
  TwoPunches,
  TwoKicks,
  Exact,  // máscara exata em `buttons`
};

// Um comando reconhecível: movimento direcional + botão.
struct CommandDef {
  std::string id;
  std::vector<std::string> motions;  // alternativas, ex.: {"6 2 3", "3 2 3"}; "" = sem movimento
  std::string spin;                   // "360" para giro completo
  // Forma compilada de `motions`: cada token é uma máscara de direções numéricas (bit d = direção d).
  std::vector<std::vector<u16>> motion_tokens;
  u16 buttons = 0;
  TriggerButtons trigger = TriggerButtons::Exact;
  i16 window = 12;          // frames entre o primeiro direcional e o botão
  bool negative_edge = false;
  bool simplified_only = false;  // comando do esquema Simplificado
  bool classic_only = false;
  bool ground = true;
  bool air = false;
  std::string move;  // id do golpe executado
  i16 move_id = -1;  // resolvido no carregamento
  i32 priority = 0;  // maior primeiro (resolvido a partir do tipo do golpe se omitido)
  i32 simplified_damage_permil = 1000;  // penalidade do Simplificado
};

struct JumpDef {
  i16 prejump = 4;
  i32 vy = 0;
  i32 gravity = 0;
  i32 vx_forward = 0;
  i32 vx_back = 0;
  i16 landing = 3;
};

struct CharacterDef {
  std::string id;
  std::string name;
  std::string archetype;
  i32 health = 10000;
  i32 walk_forward = 0;  // unidades/frame
  i32 walk_back = 0;
  i32 run_speed = 0;     // 0 = sem corrida
  JumpDef jump;
  JumpDef hop;
  Box push_stand, push_crouch, push_air;
  Box hurt_stand, hurt_crouch, hurt_air;
  std::vector<MoveDef> moves;
  std::vector<CommandDef> commands;  // ordenado por prioridade no carregamento

  // Tabela de normais: [postura: 0 em pé, 1 agachado, 2 no ar][botão: LP, MP, HP, LK, MK, HK]
  i16 normals[3][6] = {{-1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1}};
  // Esquema Simplificado: [postura][L, M, H]
  i16 simplified_normals[3][3] = {{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}};

  // Golpes obrigatórios (índices resolvidos)
  i16 move_dash = -1;
  i16 move_backdash = -1;
  i16 move_arrebentacao = -1;
  i16 move_contracorrente = -1;
  i16 move_throw = -1;       // arremesso para a frente
  i16 move_back_throw = -1;  // arremesso para trás

  i16 find_move(const std::string& move_id) const {
    for (size_t i = 0; i < moves.size(); ++i) {
      if (moves[i].id == move_id) return static_cast<i16>(i);
    }
    return -1;
  }
};

struct MareRules {
  i32 limit = 1000;
  i32 threshold = 500;
  i32 walk_forward_per_frame = 1;
  i32 dash_forward = 20;
  i32 attack_blocked = 40;
  i32 attack_hit = 60;
  i32 counter_hit = 120;
  i32 punish_counter = 200;
  i32 throw_landed = 100;
  i32 corner_per_frame = 2;
  i16 retreat_grace_frames = 90;
  i32 retreat_per_frame = 2;
};

struct Rules {
  i32 stage_half_width = 75000;   // 7,5 m de cada lado do centro
  i32 max_distance = 52000;       // distância máxima entre lutadores (câmera)
  i32 start_distance = 22000;
  i32 round_seconds = 99;
  u8 rounds_to_win = 2;
  i16 round_intro_frames = 90;
  i16 round_end_frames = 150;
  i16 button_buffer = 4;
  i16 wakeup_buffer = 6;
  i16 dash_window = 12;
  i16 throw_tech_window = 7;
  i16 throw_tech_separation = 20;
  i32 throw_tech_push = 12000;
  i16 knockdown_frames = 40;
  i16 wakeup_frames = 20;
  i16 hard_wakeup_invuln = 8;  // invulnerabilidade nos primeiros frames de levantar
  i16 counter_hit_bonus = 2;
  i16 punish_counter_bonus = 4;
  i32 counter_damage_permil = 1200;
  i16 juggle_limit = 6;
  i16 hitstun_decay_start = 5;    // a partir do N-ésimo hit, hitstun diminui
  i16 hitstun_min = 6;
  i32 max_combo_damage_permil = 600;  // nenhum combo tira mais que 60% da vida máxima
  std::vector<i32> scaling_permil = {1000, 1000, 800, 700, 600, 500, 400, 300, 200};
  i32 min_scaling_permil = 200;
  i32 meter_per_bar = 1000;
  i32 meter_bars = 2;
  i32 meter_gain_on_damaged_permil = 300;  // defensor ganha 30% do que o atacante ganha
  i32 multi_hit_hitstop_permil = 600;
  i16 max_hitstop = 18;
  i32 air_hit_vx = 120;   // empurrão horizontal padrão ao atingir alguém no ar
  i32 air_hit_vy = 260;   // impulso vertical padrão ao atingir alguém no ar
  i32 juggle_gravity = 60;
  i32 corner_zone = 3000;
  i32 proximity_guard = 30000;  // segurar trás perto de um ataque ativa a guarda em vez de andar  // distância da parede considerada "canto"
  MareRules mare;
};

struct GameData {
  Rules rules;
  std::vector<CharacterDef> characters;

  i32 find_character(const std::string& char_id) const {
    for (size_t i = 0; i < characters.size(); ++i) {
      if (characters[i].id == char_id) return static_cast<i32>(i);
    }
    return -1;
  }
};

}  // namespace fg
