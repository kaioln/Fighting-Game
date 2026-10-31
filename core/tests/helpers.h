#pragma once
#include <string>
#include <vector>

#include "fg/loader.h"
#include "fg/sim.h"
#include "test.h"

namespace fgtest {

// Dados reais do jogo (data/), carregados uma vez.
const fg::GameData& game_data();
fg::i32 char_index(const std::string& id);
fg::i16 move_index(const std::string& character, const std::string& move);

// Partida pronta para teste: sem intro, sem tempo.
fg::MatchState fight(const std::string& p1, const std::string& p2, fg::u8 extra_options = 0);

// Coloca os dois lutadores a `distance` unidades (origem a origem), centralizados.
void place(fg::MatchState& s, fg::i32 distance, fg::i32 center = 0);

// Executa `frames` frames com os inputs fixos.
void run(fg::MatchState& s, int frames, fg::InputBits p1 = 0, fg::InputBits p2 = 0, fg::StepEvents* last = nullptr);

// Executa um frame e retorna se houve evento do tipo.
bool step_has(fg::MatchState& s, fg::InputBits p1, fg::InputBits p2, fg::EventType t, fg::Event* out = nullptr);

// Inputs pseudoaleatórios plausíveis (direções seguradas por vários frames, botões, movimentos).
std::vector<fg::FrameInput> random_inputs(fg::u32 seed, int frames);

// Envia uma sequência de direções numéricas (do ponto de vista de quem olha para a direita)
// para o P1, um frame cada; `buttons` é pressionado junto com a última direção.
void feed_p1(fg::MatchState& s, const std::string& numpad, fg::InputBits buttons, fg::InputBits p2 = 0,
             fg::StepEvents* last = nullptr);

// Direções brutas para quem olha para a direita (P1 no início).
fg::InputBits raw_dir(int numpad);

}  // namespace fgtest
