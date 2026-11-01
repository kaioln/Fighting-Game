#include "fg/replay.h"

#include <fstream>
#include <iterator>

#include "fg/loader.h"
#include "fg/serialize.h"

namespace fg {

Replay record_replay(const GameData& data, const MatchSetup& setup, const std::vector<FrameInput>& inputs) {
  Replay r;
  r.data_hash = hash_game_data(data);
  r.setup = setup;
  r.inputs = inputs;
  MatchState s;
  init_match(s, data, setup);
  for (size_t i = 0; i < inputs.size(); ++i) {
    step(s, data, inputs[i], nullptr);
    if ((i + 1) % Replay::kChecksumInterval == 0) r.checksums.push_back(checksum(s));
  }
  r.checksums.push_back(checksum(s));
  return r;
}

ReplayCheck verify_replay(const GameData& data, const Replay& r) {
  ReplayCheck c;
  if (r.data_hash != hash_game_data(data)) {
    c.ok = false;
    c.error = "replay gravado com outros dados de jogo";
    return c;
  }
  MatchState s;
  init_match(s, data, r.setup);
  size_t next = 0;
  for (size_t i = 0; i < r.inputs.size(); ++i) {
    step(s, data, r.inputs[i], nullptr);
    if ((i + 1) % Replay::kChecksumInterval == 0) {
      if (next >= r.checksums.size() || r.checksums[next] != checksum(s)) {
        c.ok = false;
        c.mismatch_frame = static_cast<u32>(i);
        c.error = "checksum divergente no frame " + std::to_string(i);
        c.final_state = s;
        return c;
      }
      ++next;
    }
  }
  if (next >= r.checksums.size() || r.checksums[next] != checksum(s)) {
    c.ok = false;
    c.mismatch_frame = static_cast<u32>(r.inputs.size());
    c.error = "checksum final divergente";
  }
  c.final_state = s;
  return c;
}

std::vector<u8> encode_replay(const Replay& r) {
  ByteWriter w;
  w.u32v(Replay::kMagic);
  w.u32v(Replay::kVersion);
  w.u64v(r.data_hash);
  w.i32v(r.setup.character[0]);
  w.i32v(r.setup.character[1]);
  w.u8v(static_cast<u8>(r.setup.scheme[0]));
  w.u8v(static_cast<u8>(r.setup.scheme[1]));
  w.u32v(r.setup.seed);
  w.u8v(r.setup.options);
  w.u32v(static_cast<u32>(r.inputs.size()));
  for (const auto& in : r.inputs) {
    w.u16v(in.p[0]);
    w.u16v(in.p[1]);
  }
  w.u32v(static_cast<u32>(r.checksums.size()));
  for (u64 c : r.checksums) w.u64v(c);
  // Checksum do próprio arquivo (integridade).
  const u64 integrity = fnv1a64(w.bytes().data(), w.bytes().size());
  w.u64v(integrity);
  return w.bytes();
}

bool decode_replay(const std::vector<u8>& bytes, Replay& out, std::string& error) {
  if (bytes.size() < 8) {
    error = "arquivo curto demais";
    return false;
  }
  const size_t body = bytes.size() - 8;
  u64 stored = 0;
  for (int i = 0; i < 8; ++i) stored |= static_cast<u64>(bytes[body + static_cast<size_t>(i)]) << (8 * i);
  if (fnv1a64(bytes.data(), body) != stored) {
    error = "arquivo corrompido (checksum de integridade)";
    return false;
  }
  ByteReader rd(bytes);
  if (rd.u32v() != Replay::kMagic) {
    error = "não é um replay";
    return false;
  }
  if (rd.u32v() != Replay::kVersion) {
    error = "versão de replay não suportada";
    return false;
  }
  Replay r;
  r.data_hash = rd.u64v();
  r.setup.character[0] = rd.i32v();
  r.setup.character[1] = rd.i32v();
  r.setup.scheme[0] = static_cast<ControlScheme>(rd.u8v());
  r.setup.scheme[1] = static_cast<ControlScheme>(rd.u8v());
  r.setup.seed = rd.u32v();
  r.setup.options = rd.u8v();
  const u32 n = rd.u32v();
  if (n > 60u * 60u * 60u) {
    error = "replay longo demais";
    return false;
  }
  r.inputs.resize(n);
  for (u32 i = 0; i < n; ++i) {
    r.inputs[i].p[0] = rd.u16v();
    r.inputs[i].p[1] = rd.u16v();
  }
  const u32 nc = rd.u32v();
  if (nc > n / Replay::kChecksumInterval + 2) {
    error = "quantidade de checksums inválida";
    return false;
  }
  r.checksums.resize(nc);
  for (u32 i = 0; i < nc; ++i) r.checksums[i] = rd.u64v();
  if (!rd.ok()) {
    error = "arquivo truncado";
    return false;
  }
  out = std::move(r);
  return true;
}

bool save_replay(const std::string& path, const Replay& r) {
  const std::vector<u8> bytes = encode_replay(r);
  std::ofstream f(path, std::ios::binary);
  if (!f) return false;
  f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return static_cast<bool>(f);
}

bool load_replay(const std::string& path, Replay& out, std::string& error) {
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    error = "não foi possível abrir " + path;
    return false;
  }
  std::vector<u8> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  return decode_replay(bytes, out, error);
}

}  // namespace fg
