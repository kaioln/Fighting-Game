// Frame analyzer: mede o frame data *simulando* cada golpe contra um boneco,
// em vez de calcular à mão. É a fonte dos números exibidos no modo treino e na documentação.
#pragma once
#include <string>
#include <vector>

#include "fg/sim.h"

namespace fg {

struct MoveReport {
  std::string id;
  i16 startup = 0;
  i16 active = 0;
  i16 recovery = 0;
  i16 total = 0;
  bool connects = false;    // acertou o boneco em pé à distância de teste
  i16 hit_frame = 0;        // frame do golpe em que o primeiro contato ocorreu
  i32 damage = 0;           // dano do primeiro contato (sem escala de combo)
  i32 on_hit = 0;           // vantagem em frames ao acertar (positivo = atacante age antes)
  i32 on_block = 0;         // vantagem ao ser bloqueado
  bool knockdown = false;
  std::string meter;        // ex.: "----###......" (- startup, # ativo, . recovery)
  std::string note;
};

// Mede um golpe do personagem `attacker` contra `defender`. Golpes aéreos e arremessos são
// reportados sem vantagem (exigem cenários próprios).
MoveReport analyze_move(const GameData& data, i32 attacker, i16 move, i32 defender);

std::vector<MoveReport> analyze_character(const GameData& data, i32 attacker, i32 defender);

std::string frame_meter(const MoveDef& m);

}  // namespace fg
