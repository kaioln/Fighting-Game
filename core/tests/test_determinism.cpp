#include <deque>

#include "fg/replay.h"
#include "fg/rollback.h"
#include "fg/serialize.h"
#include "helpers.h"

using namespace fg;

namespace {
MatchSetup setup_for(u32 seed) {
  MatchSetup s;
  s.character[0] = static_cast<i32>(seed % 2);
  s.character[1] = static_cast<i32>((seed + 1) % 2);
  s.seed = seed;
  return s;
}

u64 run_offline(const GameData& d, const MatchSetup& setup, const std::vector<FrameInput>& in, MatchState* out = nullptr) {
  MatchState s;
  init_match(s, d, setup);
  for (const auto& f : in) step(s, d, f, nullptr);
  if (out) *out = s;
  return checksum(s);
}
}  // namespace

TEST("determinismo: mesmos inputs produzem o mesmo estado") {
  const GameData& d = fgtest::game_data();
  for (u32 seed = 1; seed <= 6; ++seed) {
    const auto in = fgtest::random_inputs(seed, 5000);
    CHECK(run_offline(d, setup_for(seed), in) == run_offline(d, setup_for(seed), in));
  }
}

TEST("determinismo: inputs diferentes produzem estados diferentes") {
  const GameData& d = fgtest::game_data();
  auto a = fgtest::random_inputs(7, 3000);
  auto b = a;
  b[1500].p[0] = static_cast<InputBits>(b[1500].p[0] ^ btn::HP);
  CHECK(run_offline(d, setup_for(7), a) != run_offline(d, setup_for(7), b));
}

TEST("rollback: sync test volta e ressimula a cada frame sem divergência (distâncias 1 a 8)") {
  const GameData& d = fgtest::game_data();
  for (u32 dist = 1; dist <= 8; ++dist) {
    const auto in = fgtest::random_inputs(100 + dist, 2500);
    const SyncTestResult r = run_sync_test(d, setup_for(dist), in, dist);
    if (!r.ok) std::printf("    %s\n", r.detail.c_str());
    CHECK(r.ok);
    CHECK_EQ(r.frames, 2500u);
  }
}

TEST("rollback: snapshot é uma cópia fiel (carregar e avançar = avançar direto)") {
  const GameData& d = fgtest::game_data();
  const auto in = fgtest::random_inputs(55, 1200);
  MatchState s;
  init_match(s, d, setup_for(55));
  for (size_t i = 0; i < 600; ++i) step(s, d, in[i], nullptr);
  const MatchState snapshot = s;
  for (size_t i = 600; i < in.size(); ++i) step(s, d, in[i], nullptr);
  MatchState r = snapshot;
  for (size_t i = 600; i < in.size(); ++i) step(r, d, in[i], nullptr);
  CHECK(checksum(r) == checksum(s));
}

TEST("rollback: dois pares com atraso e jitter terminam idênticos à partida offline") {
  const GameData& d = fgtest::game_data();
  for (u32 seed = 1; seed <= 4; ++seed) {
    const int frames = 1800;
    const auto in = fgtest::random_inputs(900 + seed, frames);
    const MatchSetup setup = setup_for(seed);
    MatchState initial;
    init_match(initial, d, setup);
    RollbackSession peer[2] = {RollbackSession(d, initial, 0, 12), RollbackSession(d, initial, 1, 12)};

    // Rede simulada: cada mensagem chega após 2..7 frames (jitter), em ordem arbitrária.
    struct Msg {
      int to;
      u32 frame;
      InputBits bits;
      int arrive;
    };
    std::vector<Msg> net;
    u32 rng = seed * 7919u + 1;
    auto next = [&]() {
      rng ^= rng << 13;
      rng ^= rng >> 17;
      rng ^= rng << 5;
      return rng;
    };
    int stalls = 0;
    u32 sent[2] = {0, 0};
    for (int tick = 0; tick < frames * 3 && (peer[0].frame() < static_cast<u32>(frames) || peer[1].frame() < static_cast<u32>(frames)); ++tick) {
      for (int p = 0; p < 2; ++p) {
        // Cada par gera seu input local para o próximo frame e envia ao outro.
        if (sent[p] < static_cast<u32>(frames) && sent[p] <= peer[p].frame()) {
          const InputBits bits = in[sent[p]].p[p];
          peer[p].add_local_input(sent[p], bits);
          net.push_back({1 - p, sent[p], bits, tick + 2 + static_cast<int>(next() % 6)});
          ++sent[p];
        }
      }
      for (size_t i = 0; i < net.size();) {
        if (net[i].arrive <= tick) {
          peer[net[i].to].add_remote_input(net[i].frame, net[i].bits);
          net.erase(net.begin() + static_cast<long>(i));
        } else {
          ++i;
        }
      }
      for (int p = 0; p < 2; ++p) {
        if (peer[p].frame() < static_cast<u32>(frames) && !peer[p].advance()) ++stalls;
      }
    }
    // Entrega o restante e ressimula.
    for (const auto& m : net) peer[m.to].add_remote_input(m.frame, m.bits);
    for (int p = 0; p < 2; ++p) {
      CHECK_EQ(peer[p].frame(), static_cast<u32>(frames));
    }
    // Força a ressimulação final pendente avançando um frame extra com inputs neutros.
    for (int p = 0; p < 2; ++p) {
      peer[p].add_local_input(static_cast<u32>(frames), 0);
      peer[p].add_remote_input(static_cast<u32>(frames), 0);
      peer[p].advance();
    }
    std::vector<FrameInput> ext = in;
    ext.push_back(FrameInput{});
    const u64 ref = run_offline(d, setup, ext);
    CHECK(checksum(peer[0].state()) == ref);
    CHECK(checksum(peer[1].state()) == ref);
    CHECK(peer[0].rollback_count() > 0);
    (void)stalls;
  }
}

TEST("replay: gravar, codificar, decodificar e verificar") {
  const GameData& d = fgtest::game_data();
  const auto in = fgtest::random_inputs(42, 3600);
  const Replay r = record_replay(d, setup_for(42), in);
  const std::vector<u8> bytes = encode_replay(r);
  Replay back;
  std::string err;
  REQUIRE(decode_replay(bytes, back, err));
  const ReplayCheck c = verify_replay(d, back);
  CHECK(c.ok);
  CHECK(back.inputs.size() == in.size());
}

TEST("replay: arquivo corrompido é rejeitado") {
  const GameData& d = fgtest::game_data();
  const Replay r = record_replay(d, setup_for(3), fgtest::random_inputs(3, 600));
  std::vector<u8> bytes = encode_replay(r);
  bytes[40] = static_cast<u8>(bytes[40] ^ 0xFF);
  Replay back;
  std::string err;
  CHECK(!decode_replay(bytes, back, err));
  CHECK(err.find("corrompido") != std::string::npos);
}

TEST("replay: dados de jogo diferentes invalidam o replay") {
  GameData d = fgtest::game_data();
  const Replay r = record_replay(d, setup_for(4), fgtest::random_inputs(4, 600));
  d.characters[0].moves[0].recovery = static_cast<i16>(d.characters[0].moves[0].recovery + 1);
  const ReplayCheck c = verify_replay(d, r);
  CHECK(!c.ok);
}

TEST("serialização: tamanho fixo, independente do conteúdo") {
  MatchState a, b;
  init_match(a, fgtest::game_data(), setup_for(1));
  b = a;
  b.f[0].x = 12345;
  b.f[1].input.push(3, btn::HK);
  ByteWriter wa, wb;
  serialize(a, wa);
  serialize(b, wb);
  CHECK(wa.bytes().size() == wb.bytes().size());
  CHECK(checksum(a) != checksum(b));
}
