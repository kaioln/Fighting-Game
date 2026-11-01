#include "helpers.h"

#include <cstdio>
#include <cstdlib>

namespace fgtest {

const fg::GameData& game_data() {
  static fg::GameData data;
  static bool loaded = false;
  if (!loaded) {
    fg::LoadResult r = fg::load_game_data(FG_DATA_DIR, data);
    if (!r.ok) {
      for (const auto& e : r.errors) std::printf("erro de dados: %s\n", e.c_str());
      std::abort();
    }
    loaded = true;
  }
  return data;
}

fg::i32 char_index(const std::string& id) { return game_data().find_character(id); }

fg::i16 move_index(const std::string& character, const std::string& move) {
  const fg::i32 c = char_index(character);
  if (c < 0) return -1;
  return game_data().characters[static_cast<size_t>(c)].find_move(move);
}

fg::MatchState fight(const std::string& p1, const std::string& p2, fg::u8 extra_options) {
  fg::MatchSetup setup;
  setup.character[0] = char_index(p1);
  setup.character[1] = char_index(p2);
  setup.options = static_cast<fg::u8>(fg::option::SkipIntro | fg::option::NoTimer | extra_options);
  fg::MatchState s;
  fg::init_match(s, game_data(), setup);
  return s;
}

void place(fg::MatchState& s, fg::i32 distance, fg::i32 center) {
  s.f[0].x = center - distance / 2;
  s.f[1].x = center + (distance - distance / 2);
  s.f[0].facing = 1;
  s.f[1].facing = -1;
}

void run(fg::MatchState& s, int frames, fg::InputBits p1, fg::InputBits p2, fg::StepEvents* last) {
  fg::StepEvents ev;
  for (int i = 0; i < frames; ++i) {
    fg::FrameInput in;
    in.p[0] = p1;
    in.p[1] = p2;
    fg::step(s, game_data(), in, last ? last : &ev);
  }
}

bool step_has(fg::MatchState& s, fg::InputBits p1, fg::InputBits p2, fg::EventType t, fg::Event* out) {
  fg::StepEvents ev;
  fg::FrameInput in;
  in.p[0] = p1;
  in.p[1] = p2;
  fg::step(s, game_data(), in, &ev);
  for (int i = 0; i < ev.count; ++i) {
    if (ev.items[i].type == t) {
      if (out) *out = ev.items[i];
      return true;
    }
  }
  return false;
}

fg::InputBits raw_dir(int numpad) {
  using namespace fg;
  InputBits b = 0;
  if (numpad == 1 || numpad == 4 || numpad == 7) b |= btn::Left;
  if (numpad == 3 || numpad == 6 || numpad == 9) b |= btn::Right;
  if (numpad <= 3) b |= btn::Down;
  if (numpad >= 7) b |= btn::Up;
  return b;
}

}  // namespace fgtest

namespace fgtest {

std::vector<fg::FrameInput> random_inputs(fg::u32 seed, int frames) {
  using namespace fg;
  std::vector<FrameInput> out(static_cast<size_t>(frames));
  u32 st = seed * 2654435761u + 12345u;
  auto next = [&]() {
    st ^= st << 13;
    st ^= st >> 17;
    st ^= st << 5;
    return st;
  };
  for (int p = 0; p < 2; ++p) {
    int hold = 0;
    InputBits dir = 0;
    for (int i = 0; i < frames; ++i) {
      if (hold <= 0) {
        dir = raw_dir(static_cast<int>(next() % 9) + 1);
        hold = static_cast<int>(next() % 20) + 1;
      }
      --hold;
      InputBits b = dir;
      const u32 r = next() % 100;
      static const InputBits buttons[] = {btn::LP, btn::MP, btn::HP, btn::LK, btn::MK, btn::HK,
                                          static_cast<InputBits>(btn::LP | btn::LK),
                                          static_cast<InputBits>(btn::MP | btn::MK),
                                          static_cast<InputBits>(btn::LP | btn::MP)};
      if (r < 12) b = static_cast<InputBits>(b | buttons[next() % 9]);
      out[static_cast<size_t>(i)].p[p] = b;
    }
  }
  return out;
}

void feed_p1(fg::MatchState& s, const std::string& numpad, fg::InputBits buttons, fg::InputBits p2,
             fg::StepEvents* last) {
  for (size_t i = 0; i < numpad.size(); ++i) {
    fg::InputBits b = raw_dir(numpad[i] - '0');
    if (i + 1 == numpad.size()) b = static_cast<fg::InputBits>(b | buttons);
    run(s, 1, b, p2, last);
  }
}

}  // namespace fgtest
