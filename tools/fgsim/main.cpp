// fgsim — ferramenta de linha de comando do núcleo (sem gráficos).
// Usada por desenvolvedores, pelo CI e para gerar a documentação de frame data.
//
//   fgsim validate  <data>
//   fgsim framedata <data> [personagem]            tabela Markdown medida por simulação
//   fgsim combos    <data>                          rotas de combo (cancel, link, chain) por simulação
//   fgsim synctest  <data> [frames] [distância] [seed]
//   fgsim checksum  <data> [frames] [seed]          checksum final de uma partida aleatória
//   fgsim record    <data> <arquivo.fgr> [frames] [seed]
//   fgsim verify    <data> <arquivo.fgr>
//   fgsim trace     <data> <p1> <p2> <roteiro>      frame analyzer em texto
//
// Roteiro do trace: uma instrução por linha, "<frames> <input P1> <input P2>", onde input é
// uma direção numérica (1-9) seguida de botões opcionais, ex.: "1 2LK 5", "10 5 4", "1 236LP 5"
// não é permitido: movimentos são escritos frame a frame ("1 2 5", "1 3 5", "1 6LP 5").
// Linhas iniciadas com '#' são comentários. "place <distância>" posiciona os lutadores.

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#include "combos.h"
#include "fg/analyzer.h"
#include "fg/loader.h"
#include "fg/replay.h"
#include "fg/rollback.h"
#include "fg/serialize.h"
#include "fg/sim.h"

using namespace fg;

namespace {

int usage() {
  std::fprintf(stderr,
               "uso: fgsim <validate|framedata|combos|synctest|checksum|record|verify|trace> <data> [args]\n"
               "veja o cabeçalho de tools/fgsim/main.cpp\n");
  return 2;
}

bool load(const std::string& dir, GameData& d) {
  const LoadResult r = load_game_data(dir, d);
  for (const auto& e : r.errors) std::fprintf(stderr, "erro: %s\n", e.c_str());
  return r.ok;
}

std::vector<FrameInput> random_inputs(u32 seed, int frames) {
  std::vector<FrameInput> out(static_cast<size_t>(frames));
  u32 st = seed * 2654435761u + 12345u;
  auto next = [&]() {
    st ^= st << 13;
    st ^= st >> 17;
    st ^= st << 5;
    return st;
  };
  static const InputBits dirs[9] = {
      static_cast<InputBits>(btn::Down | btn::Left), btn::Down, static_cast<InputBits>(btn::Down | btn::Right),
      btn::Left, 0, btn::Right,
      static_cast<InputBits>(btn::Up | btn::Left), btn::Up, static_cast<InputBits>(btn::Up | btn::Right)};
  static const InputBits buttons[] = {btn::LP, btn::MP, btn::HP, btn::LK, btn::MK, btn::HK,
                                      static_cast<InputBits>(btn::LP | btn::LK),
                                      static_cast<InputBits>(btn::MP | btn::MK)};
  for (int p = 0; p < 2; ++p) {
    int hold = 0;
    InputBits dir = 0;
    for (int i = 0; i < frames; ++i) {
      if (hold <= 0) {
        dir = dirs[next() % 9];
        hold = static_cast<int>(next() % 20) + 1;
      }
      --hold;
      InputBits b = dir;
      if (next() % 100 < 12) b = static_cast<InputBits>(b | buttons[next() % 8]);
      out[static_cast<size_t>(i)].p[p] = b;
    }
  }
  return out;
}

const char* act_name(Act a) {
  switch (a) {
    case Act::Idle: return "Idle";
    case Act::Crouch: return "Crouch";
    case Act::WalkForward: return "WalkF";
    case Act::WalkBack: return "WalkB";
    case Act::Run: return "Run";
    case Act::Guard: return "Guard";
    case Act::PreJump: return "PreJump";
    case Act::Air: return "Air";
    case Act::Landing: return "Landing";
    case Act::Move: return "Move";
    case Act::Hitstun: return "Hitstun";
    case Act::Blockstun: return "Blockstun";
    case Act::Juggle: return "Juggle";
    case Act::Knockdown: return "Knockdown";
    case Act::Wakeup: return "Wakeup";
    case Act::ThrowAttack: return "ThrowAtk";
    case Act::Thrown: return "Thrown";
    case Act::TechRecover: return "TechRec";
    case Act::KO: return "KO";
  }
  return "?";
}

const char* event_name(EventType t) {
  switch (t) {
    case EventType::RoundStart: return "round";
    case EventType::Fight: return "lutem";
    case EventType::MoveStart: return "inicio";
    case EventType::Hit: return "HIT";
    case EventType::CounterHit: return "COUNTER";
    case EventType::PunishCounter: return "PUNISH";
    case EventType::Block: return "BLOCK";
    case EventType::Armor: return "ARMOR";
    case EventType::ThrowConnect: return "AGARROU";
    case EventType::ThrowTech: return "TECH";
    case EventType::ThrowComplete: return "ARREMESSO";
    case EventType::Knockdown: return "QUEDA";
    case EventType::KO: return "KO";
    case EventType::TimeOver: return "TEMPO";
    case EventType::RoundEnd: return "fim-round";
    case EventType::MatchEnd: return "fim-partida";
    case EventType::SuperFreeze: return "SUPER";
    case EventType::Arrebentacao: return "ARREBENTACAO";
    case EventType::Contracorrente: return "CONTRACORRENTE";
  }
  return "?";
}

// "6LP" -> direita + LP para quem olha para a direita; o lado é ajustado pelo facing atual.
bool parse_input(const std::string& tok, i32 facing, InputBits& out) {
  if (tok.empty() || tok[0] < '1' || tok[0] > '9') return false;
  const int d = tok[0] - '0';
  InputBits b = 0;
  const bool fwd = d == 3 || d == 6 || d == 9;
  const bool back = d == 1 || d == 4 || d == 7;
  if (fwd) b |= facing >= 0 ? btn::Right : btn::Left;
  if (back) b |= facing >= 0 ? btn::Left : btn::Right;
  if (d <= 3) b |= btn::Down;
  if (d >= 7) b |= btn::Up;
  std::string rest = tok.substr(1);
  static const std::pair<const char*, InputBits> names[] = {{"LP", btn::LP}, {"MP", btn::MP}, {"HP", btn::HP},
                                                            {"LK", btn::LK}, {"MK", btn::MK}, {"HK", btn::HK},
                                                            {"SP", btn::SP}};
  while (!rest.empty()) {
    bool found = false;
    for (const auto& [n, bit] : names) {
      if (rest.rfind(n, 0) == 0) {
        b |= bit;
        rest = rest.substr(2);
        found = true;
        break;
      }
    }
    if (!found) return false;
  }
  out = b;
  return true;
}

int cmd_framedata(const GameData& d, const std::string& only) {
  for (size_t ci = 0; ci < d.characters.size(); ++ci) {
    const CharacterDef& ch = d.characters[ci];
    if (!only.empty() && ch.id != only) continue;
    const i32 defender = static_cast<i32>((ci + 1) % d.characters.size());
    std::printf("### %s (%s) — contra %s\n\n", ch.name.c_str(), ch.archetype.c_str(),
                d.characters[static_cast<size_t>(defender)].name.c_str());
    std::printf("| Golpe | Startup | Ativo | Recovery | Total | Dano | No hit | No bloqueio | Medidor | Observação |\n");
    std::printf("|---|---:|---:|---:|---:|---:|---:|---:|---|---|\n");
    for (const auto& r : analyze_character(d, static_cast<i32>(ci), defender)) {
      if (r.connects) {
        std::printf("| %s | %d | %d | %d | %d | %d | %+d | %+d | `%s` | %s |\n", r.id.c_str(), r.startup, r.active,
                    r.recovery, r.total, r.damage, r.on_hit, r.on_block, r.meter.c_str(), r.note.c_str());
      } else {
        std::printf("| %s | %d | %d | %d | %d | — | — | — | `%s` | %s |\n", r.id.c_str(), r.startup, r.active,
                    r.recovery, r.total, r.meter.c_str(), r.note.c_str());
      }
    }
    std::printf("\n");
  }
  return 0;
}

int cmd_trace(const GameData& d, const std::string& p1, const std::string& p2, const std::string& script) {
  MatchSetup setup;
  setup.character[0] = d.find_character(p1);
  setup.character[1] = d.find_character(p2);
  if (setup.character[0] < 0 || setup.character[1] < 0) {
    std::fprintf(stderr, "personagem desconhecido\n");
    return 1;
  }
  setup.options = option::SkipIntro | option::NoTimer;
  MatchState s;
  init_match(s, d, setup);
  std::ifstream in(script);
  if (!in) {
    std::fprintf(stderr, "não foi possível abrir %s\n", script.c_str());
    return 1;
  }
  std::printf("frame | P1 estado      golpe       f  hs stun     x |  P2 estado      golpe       f  hs stun     x | Maré | eventos\n");
  std::string line;
  int lineno = 0;
  while (std::getline(in, line)) {
    ++lineno;
    if (line.empty() || line[0] == '#') continue;
    std::istringstream ls(line);
    std::string first;
    ls >> first;
    if (first == "place") {
      i32 dist = 0;
      ls >> dist;
      s.f[0].x = -dist / 2;
      s.f[1].x = dist - dist / 2;
      continue;
    }
    const int frames = std::atoi(first.c_str());
    std::string a, b;
    ls >> a >> b;
    for (int i = 0; i < frames; ++i) {
      FrameInput fi;
      if (!parse_input(a, s.f[0].facing, fi.p[0]) || !parse_input(b, s.f[1].facing, fi.p[1])) {
        std::fprintf(stderr, "%s:%d: input inválido\n", script.c_str(), lineno);
        return 1;
      }
      StepEvents ev;
      step(s, d, fi, &ev);
      auto fighter = [&](int p) {
        const FighterState& f = s.f[p];
        const MoveDef* m = current_move(s, d, p);
        const i16 stun = (f.act == Act::Hitstun || f.act == Act::Blockstun) ? f.timer : 0;
        std::printf(" %-9s %-11s %3d %3d %4d %6d |", act_name(f.act), m ? m->id.c_str() : "", m ? f.move_frame : 0,
                    f.hitstop, stun, f.x);
      };
      std::printf("%5u |", s.frame);
      fighter(0);
      fighter(1);
      std::printf(" %4d |", s.mare);
      for (int k = 0; k < ev.count; ++k) {
        if (ev.items[k].type == EventType::MoveStart) continue;
        std::printf(" P%d:%s(%d)", ev.items[k].player + 1, event_name(ev.items[k].type), ev.items[k].value);
      }
      std::printf("\n");
    }
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) return usage();
  const std::string cmd = argv[1];
  GameData d;
  if (!load(argv[2], d)) return 1;

  if (cmd == "validate") {
    std::printf("dados válidos: %zu personagens, hash %016llx\n", d.characters.size(),
                static_cast<unsigned long long>(hash_game_data(d)));
    for (const auto& ch : d.characters) std::printf("  %s: %zu golpes, %zu comandos\n", ch.id.c_str(), ch.moves.size(), ch.commands.size());
    return 0;
  }
  if (cmd == "combos") return report_combos(d);
  if (cmd == "framedata") return cmd_framedata(d, argc > 3 ? argv[3] : "");
  if (cmd == "synctest") {
    const int frames = argc > 3 ? std::atoi(argv[3]) : 3600;
    const u32 dist = argc > 4 ? static_cast<u32>(std::atoi(argv[4])) : 7;
    const u32 seed = argc > 5 ? static_cast<u32>(std::atoi(argv[5])) : 1;
    MatchSetup setup;
    setup.seed = seed;
    const SyncTestResult r = run_sync_test(d, setup, random_inputs(seed, frames), dist);
    std::printf("sync test: %s (%u frames, distância %u)%s%s\n", r.ok ? "OK" : "FALHOU", r.frames, dist,
                r.ok ? "" : " — ", r.detail.c_str());
    return r.ok ? 0 : 1;
  }
  if (cmd == "checksum") {
    const int frames = argc > 3 ? std::atoi(argv[3]) : 3600;
    const u32 seed = argc > 4 ? static_cast<u32>(std::atoi(argv[4])) : 1;
    MatchSetup setup;
    setup.seed = seed;
    MatchState s;
    init_match(s, d, setup);
    for (const auto& in : random_inputs(seed, frames)) step(s, d, in, nullptr);
    std::printf("%016llx\n", static_cast<unsigned long long>(checksum(s)));
    return 0;
  }
  if (cmd == "record") {
    if (argc < 4) return usage();
    const int frames = argc > 4 ? std::atoi(argv[4]) : 3600;
    const u32 seed = argc > 5 ? static_cast<u32>(std::atoi(argv[5])) : 1;
    MatchSetup setup;
    setup.seed = seed;
    const Replay r = record_replay(d, setup, random_inputs(seed, frames));
    if (!save_replay(argv[3], r)) {
      std::fprintf(stderr, "falha ao gravar %s\n", argv[3]);
      return 1;
    }
    std::printf("replay gravado: %s (%zu frames, checksum final %016llx)\n", argv[3], r.inputs.size(),
                static_cast<unsigned long long>(r.checksums.back()));
    return 0;
  }
  if (cmd == "verify") {
    if (argc < 4) return usage();
    Replay r;
    std::string err;
    if (!load_replay(argv[3], r, err)) {
      std::fprintf(stderr, "erro: %s\n", err.c_str());
      return 1;
    }
    const ReplayCheck c = verify_replay(d, r);
    std::printf("replay %s: %s\n", argv[3], c.ok ? "OK (checksums idênticos)" : c.error.c_str());
    return c.ok ? 0 : 1;
  }
  if (cmd == "trace") {
    if (argc < 6) return usage();
    return cmd_trace(d, argv[3], argv[4], argv[5]);
  }
  return usage();
}
