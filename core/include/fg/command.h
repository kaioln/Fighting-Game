// Interpretador de comandos: reconhece movimentos direcionais e botões a partir do histórico.
#pragma once
#include <string>
#include <vector>

#include "fg/data.h"
#include "fg/state.h"

namespace fg {

// Converte "6 2 3" em tokens; cada token pode ter alternativas: "[123] 6" ou "2 3 [69]".
// Retorna false se a sintaxe for inválida.
bool compile_motion(const std::string& text, std::vector<u16>& tokens, std::string& error);

// O movimento termina no frame `end_ago` (0 = mais recente) e começa no máximo `window` frames antes.
bool motion_matches(const InputHistory& h, const std::vector<u16>& tokens, i32 window, u32 end_ago);

// Giro completo: frente, trás, baixo e cima (cima opcional quando `allow_no_up`) dentro da janela.
bool spin_matches(const InputHistory& h, i32 window, u32 end_ago);

// Pressão (ou soltura, com negative edge) de botões que satisfaz o gatilho no frame `ago`.
bool trigger_matches(const InputHistory& h, const CommandDef& c, u32 ago);

struct CommandMatch {
  i32 command = -1;   // índice em CharacterDef::commands
  u32 press_ago = 0;  // quantos frames atrás o botão foi pressionado
};

// Filtro de quais tipos de golpe podem sair agora (ex.: só especiais durante uma janela de cancelamento).
struct CommandFilter {
  bool grounded = true;
  bool allow_normals = true;    // CommandNormal
  bool allow_specials = true;   // Special, Enhanced
  bool allow_supers = true;
  bool allow_throws = true;
  bool allow_mare = true;
  i32 meter_available = 0;
  i32 mare_favor = 0;           // Maré do ponto de vista deste lutador
  i32 mare_threshold = 0;
  bool in_blockstun = false;
};

// Procura o comando de maior prioridade satisfeito dentro do buffer.
// Pressões já consumidas (anteriores a `consumed_press`) são ignoradas.
CommandMatch find_command(const CharacterDef& ch, const FighterState& f, const Rules& rules,
                          const CommandFilter& filter, i32 buffer);

// Prioridade padrão por tipo de golpe: Super > Reforçado > Especial > Maré > Arremesso > Command normal.
i32 default_priority(MoveKind k);

}  // namespace fg
