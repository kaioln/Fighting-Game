// fgsim combos: verificação de rotas de combo por simulação.
#pragma once
#include "fg/data.h"

namespace fg {
// Imprime um relatório Markdown; retorna 0 se não houver divergências.
int report_combos(const GameData& d);
}  // namespace fg
