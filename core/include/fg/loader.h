// Carregamento e validação dos dados de jogo (JSON). Fora da simulação.
#pragma once
#include <string>
#include <vector>

#include "fg/data.h"
#include "fg/json.h"

namespace fg {

struct LoadResult {
  bool ok = false;
  std::vector<std::string> errors;
};

// Lê um personagem a partir de JSON já parseado. Erros têm o formato "<contexto>: <mensagem>".
LoadResult load_character(const json::Value& root, CharacterDef& out);
LoadResult load_rules(const json::Value& root, Rules& out);

// Lê `rules.json` e `characters/*.json` (lista em `roster.json`) a partir de um diretório.
LoadResult load_game_data(const std::string& data_dir, GameData& out);

// Validação semântica de um personagem já carregado (também chamada por load_character).
void validate_character(const CharacterDef& ch, std::vector<std::string>& errors);

// Hash estável dos dados (para replays: um replay só é válido com os mesmos dados).
u64 hash_game_data(const GameData& data);

std::string read_text_file(const std::string& path, bool& ok);

}  // namespace fg
