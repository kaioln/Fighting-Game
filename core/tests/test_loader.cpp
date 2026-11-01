#include "fg/loader.h"
#include "helpers.h"

using namespace fg;

namespace {
LoadResult load_text(const std::string& text, CharacterDef& out) {
  auto pr = json::parse(text);
  LoadResult r;
  if (!pr.ok) {
    r.errors.push_back(pr.error);
    return r;
  }
  return load_character(pr.value, out);
}

std::string nadja_text() {
  bool ok = false;
  std::string t = read_text_file(std::string(FG_DATA_DIR) + "/characters/nadja.json", ok);
  return t;
}

bool has_error(const LoadResult& r, const std::string& needle) {
  for (const auto& e : r.errors)
    if (e.find(needle) != std::string::npos) return true;
  return false;
}
}  // namespace

TEST("dados: elenco real carrega sem erros") {
  const GameData& d = fgtest::game_data();
  CHECK(d.characters.size() == 2);
  CHECK(fgtest::char_index("nadja") >= 0);
  CHECK(fgtest::char_index("otto") >= 0);
}

TEST("dados: todos os normais existem e comandos foram resolvidos") {
  for (const auto& ch : fgtest::game_data().characters) {
    for (int s = 0; s < 3; ++s)
      for (int b = 0; b < 6; ++b) CHECK(ch.normals[s][b] >= 0);
    for (const auto& c : ch.commands) CHECK(c.move_id >= 0);
    // ordenados por prioridade
    for (size_t i = 1; i < ch.commands.size(); ++i) CHECK(ch.commands[i - 1].priority >= ch.commands[i].priority);
  }
}

TEST("validador: hitbox fora dos frames ativos é rejeitada") {
  std::string t = nadja_text();
  const std::string from = "{ \"frames\": [4, 5], \"kind\": \"hit\", \"box\": [2700, 9000, 3800, 2000] }";
  const std::string to = "{ \"frames\": [4, 9], \"kind\": \"hit\", \"box\": [1500, 9000, 3800, 2000] }";
  REQUIRE(t.find(from) != std::string::npos);
  t.replace(t.find(from), from.size(), to);
  CharacterDef ch;
  LoadResult r = load_text(t, ch);
  CHECK(!r.ok);
  CHECK(has_error(r, "hitbox fora dos frames ativos"));
}

TEST("validador: campo desconhecido e golpe inexistente são apontados") {
  std::string t = nadja_text();
  t.replace(t.find("\"archetype\""), 11, "\"arquetipo\"");
  t.replace(t.find("\"move\": \"214K\""), 14, "\"move\": \"999K\"");
  CharacterDef ch;
  LoadResult r = load_text(t, ch);
  CHECK(!r.ok);
  CHECK(has_error(r, "campo desconhecido 'arquetipo'"));
  CHECK(has_error(r, "golpe inexistente '999K'"));
}

TEST("validador: normal obrigatório ausente") {
  std::string t = nadja_text();
  t.replace(t.find("\"id\": \"2HK\""), 11, "\"id\": \"2XX\"");
  CharacterDef ch;
  LoadResult r = load_text(t, ch);
  CHECK(!r.ok);
  CHECK(has_error(r, "2HK"));
}

TEST("dados: hash muda quando um número de frame data muda") {
  GameData a = fgtest::game_data();
  const u64 h1 = hash_game_data(a);
  a.characters[0].moves[0].recovery = static_cast<i16>(a.characters[0].moves[0].recovery + 1);
  CHECK(hash_game_data(a) != h1);
}
