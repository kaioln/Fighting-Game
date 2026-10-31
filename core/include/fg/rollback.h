// Rollback: snapshots, predição de input e ressimulação (modelo de API inspirado no GGPO:
// salvar estado, carregar estado, avançar um frame sem renderizar).
#pragma once
#include <string>
#include <vector>

#include "fg/sim.h"

namespace fg {

// Sessão de um par (peer). O jogador local tem input sempre conhecido; o remoto é previsto
// (repete o último input confirmado) até chegar o input real. Se a predição errar, volta ao
// frame divergente e ressimula até o presente.
class RollbackSession {
 public:
  RollbackSession(const GameData& data, const MatchState& initial, int local_player, u32 max_prediction = 8);

  // `frame` é o índice do frame em que o input é usado (0 = primeiro step).
  void add_local_input(u32 frame, InputBits bits);
  void add_remote_input(u32 frame, InputBits bits);

  // Avança um frame. Retorna false (sem avançar) se exceder o limite de predição.
  bool advance();

  const MatchState& state() const { return state_; }
  u32 frame() const { return frame_; }
  // Último frame cujo input remoto é conhecido (todos os anteriores também).
  i64 confirmed_remote() const { return confirmed_remote_; }
  u32 rollback_count() const { return rollbacks_; }
  u32 resimulated_frames() const { return resimulated_; }
  // Checksum do estado *antes* do frame `f` (válido para frames ainda no histórico).
  u64 checksum_before(u32 f) const;

 private:
  static constexpr u32 kRing = 128;
  struct Slot {
    MatchState before;  // estado antes de simular o frame
    InputBits local = 0;
    InputBits remote = 0;
    bool local_known = false;
    bool remote_known = false;
    InputBits remote_used = 0;  // valor usado na simulação (real ou previsto)
    u32 frame = 0xFFFFFFFFu;
  };

  Slot& slot(u32 f) { return ring_[f % kRing]; }
  const Slot& slot(u32 f) const { return ring_[f % kRing]; }
  InputBits predicted_remote(u32 f) const;
  void simulate_frame(u32 f);

  const GameData& data_;
  MatchState state_;
  int local_;
  u32 max_prediction_;
  u32 frame_ = 0;
  i64 confirmed_remote_ = -1;
  i64 rollback_from_ = -1;
  u32 rollbacks_ = 0;
  u32 resimulated_ = 0;
  std::vector<Slot> ring_;
  // Inputs que chegaram antes do frame ser simulado.
  std::vector<std::pair<u32, InputBits>> pending_local_, pending_remote_;
};

struct SyncTestResult {
  bool ok = true;
  u32 frames = 0;
  u32 first_mismatch = 0;
  std::string detail;
};

// A cada frame, volta `distance` frames e ressimula, comparando checksums com a execução original.
// Equivalente ao "sync test" recomendado pelo guia do GGPO.
SyncTestResult run_sync_test(const GameData& data, const MatchSetup& setup, const std::vector<FrameInput>& inputs,
                             u32 distance);

}  // namespace fg
