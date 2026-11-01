#include "fg/rollback.h"

#include "fg/serialize.h"

namespace fg {

RollbackSession::RollbackSession(const GameData& data, const MatchState& initial, int local_player,
                                 u32 max_prediction)
    : data_(data), state_(initial), local_(local_player), max_prediction_(max_prediction), ring_(kRing) {}

void RollbackSession::add_local_input(u32 f, InputBits bits) {
  Slot& sl = slot(f);
  if (sl.frame != f) {
    sl = Slot{};
    sl.frame = f;
  }
  sl.local = bits;
  sl.local_known = true;
}

void RollbackSession::add_remote_input(u32 f, InputBits bits) {
  if (f + kRing / 2 < frame_) return;  // antigo demais (não deve ocorrer com transporte confiável)
  Slot& sl = slot(f);
  if (sl.frame != f) {
    // Frame ainda não visto: inicializa o slot.
    sl = Slot{};
    sl.frame = f;
  }
  if (sl.remote_known) return;  // duplicado
  sl.remote = bits;
  sl.remote_known = true;
  // Atualiza o último frame confirmado contíguo.
  while (true) {
    const u32 next = static_cast<u32>(confirmed_remote_ + 1);
    const Slot& ns = slot(next);
    if (ns.frame == next && ns.remote_known) ++confirmed_remote_;
    else break;
  }
  // Se o frame já foi simulado com uma predição diferente, agenda rollback.
  if (f < frame_ && sl.remote_used != bits) {
    if (rollback_from_ < 0 || static_cast<i64>(f) < rollback_from_) rollback_from_ = f;
  }
}

InputBits RollbackSession::predicted_remote(u32 f) const {
  const Slot& sl = slot(f);
  if (sl.frame == f && sl.remote_known) return sl.remote;
  // Predição: repete o último input remoto conhecido.
  if (confirmed_remote_ >= 0) {
    const Slot& last = slot(static_cast<u32>(confirmed_remote_));
    if (last.frame == static_cast<u32>(confirmed_remote_)) return last.remote;
  }
  return 0;
}

void RollbackSession::simulate_frame(u32 f) {
  Slot& sl = slot(f);
  sl.before = state_;
  sl.remote_used = predicted_remote(f);
  FrameInput in;
  in.p[local_] = sl.local;
  in.p[1 - local_] = sl.remote_used;
  step(state_, data_, in, nullptr);
}

bool RollbackSession::advance() {
  if (rollback_from_ >= 0) {
    const u32 from = static_cast<u32>(rollback_from_);
    state_ = slot(from).before;
    ++rollbacks_;
    for (u32 f = from; f < frame_; ++f) {
      simulate_frame(f);
      ++resimulated_;
    }
    rollback_from_ = -1;
  }
  if (static_cast<i64>(frame_) - confirmed_remote_ > static_cast<i64>(max_prediction_)) return false;
  Slot& sl = slot(frame_);
  if (sl.frame != frame_) {
    sl = Slot{};
    sl.frame = frame_;
  }
  if (!sl.local_known) return false;  // sem input local não há o que simular
  simulate_frame(frame_);
  ++frame_;
  return true;
}

u64 RollbackSession::checksum_before(u32 f) const { return checksum(slot(f).before); }

SyncTestResult run_sync_test(const GameData& data, const MatchSetup& setup, const std::vector<FrameInput>& inputs,
                             u32 distance) {
  SyncTestResult res;
  MatchState s;
  init_match(s, data, setup);
  std::vector<MatchState> history;
  history.reserve(inputs.size() + 1);
  history.push_back(s);
  for (size_t i = 0; i < inputs.size(); ++i) {
    step(s, data, inputs[i], nullptr);
    history.push_back(s);
    // Volta `distance` frames e ressimula até o presente.
    const size_t back = i + 1 >= distance ? i + 1 - distance : 0;
    MatchState r = history[back];
    for (size_t k = back; k <= i; ++k) step(r, data, inputs[k], nullptr);
    if (checksum(r) != checksum(s)) {
      res.ok = false;
      res.first_mismatch = static_cast<u32>(i);
      res.detail = "estado ressimulado diverge no frame " + std::to_string(i);
      res.frames = static_cast<u32>(i + 1);
      return res;
    }
  }
  res.frames = static_cast<u32>(inputs.size());
  return res;
}

}  // namespace fg
