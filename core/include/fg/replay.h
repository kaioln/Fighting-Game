// Replays: configuração + inputs. Checksums periódicos detectam divergência de comportamento.
#pragma once
#include <string>
#include <vector>

#include "fg/sim.h"

namespace fg {

struct Replay {
  static constexpr u32 kMagic = 0x50524746u;  // "FGRP"
  static constexpr u32 kVersion = 1;
  static constexpr u32 kChecksumInterval = 60;

  u64 data_hash = 0;
  MatchSetup setup;
  std::vector<FrameInput> inputs;
  std::vector<u64> checksums;  // um a cada kChecksumInterval frames, e no fim
};

// Executa a partida e preenche os checksums.
Replay record_replay(const GameData& data, const MatchSetup& setup, const std::vector<FrameInput>& inputs);

struct ReplayCheck {
  bool ok = true;
  std::string error;
  u32 mismatch_frame = 0;
  MatchState final_state;
};

ReplayCheck verify_replay(const GameData& data, const Replay& r);

std::vector<u8> encode_replay(const Replay& r);
bool decode_replay(const std::vector<u8>& bytes, Replay& out, std::string& error);

bool save_replay(const std::string& path, const Replay& r);
bool load_replay(const std::string& path, Replay& out, std::string& error);

}  // namespace fg
