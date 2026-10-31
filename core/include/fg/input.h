// Representação de inputs: um quadro de input é um inteiro de 16 bits.
#pragma once
#include "fg/types.h"

namespace fg {

namespace btn {
inline constexpr u16 Up = 1u << 0;
inline constexpr u16 Down = 1u << 1;
inline constexpr u16 Left = 1u << 2;
inline constexpr u16 Right = 1u << 3;
inline constexpr u16 LP = 1u << 4;
inline constexpr u16 MP = 1u << 5;
inline constexpr u16 HP = 1u << 6;
inline constexpr u16 LK = 1u << 7;
inline constexpr u16 MK = 1u << 8;
inline constexpr u16 HK = 1u << 9;
// Botão "Especial" do esquema Simplificado.
inline constexpr u16 SP = 1u << 10;

inline constexpr u16 Directions = Up | Down | Left | Right;
inline constexpr u16 Punches = LP | MP | HP;
inline constexpr u16 Kicks = LK | MK | HK;
inline constexpr u16 Attacks = Punches | Kicks | SP;
}  // namespace btn

using InputBits = u16;

// Direção em notação numérica (teclado numérico), relativa ao lado do lutador:
// 7 8 9
// 4 5 6   (6 = para a frente, 4 = para trás)
// 1 2 3
// SOCD: esquerda+direita = neutro horizontal; cima+baixo = cima.
inline constexpr u8 numpad_direction(InputBits raw, i32 facing) {
  const bool left = (raw & btn::Left) != 0;
  const bool right = (raw & btn::Right) != 0;
  const bool up = (raw & btn::Up) != 0;
  const bool down = (raw & btn::Down) != 0 && !up;
  i32 h = 0;
  if (right && !left) h = 1;
  if (left && !right) h = -1;
  h *= (facing >= 0 ? 1 : -1);  // converte para frente/trás
  const i32 v = up ? 1 : (down ? -1 : 0);
  // linha: v=-1 -> 1..3, v=0 -> 4..6, v=1 -> 7..9
  return static_cast<u8>((v + 1) * 3 + (h + 2));
}

inline constexpr bool dir_is_back(u8 d) { return d == 1 || d == 4 || d == 7; }
inline constexpr bool dir_is_forward(u8 d) { return d == 3 || d == 6 || d == 9; }
inline constexpr bool dir_is_down(u8 d) { return d >= 1 && d <= 3; }
inline constexpr bool dir_is_up(u8 d) { return d >= 7; }

}  // namespace fg
