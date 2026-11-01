// Tipos fundamentais do núcleo. Nenhum ponto flutuante participa da lógica competitiva.
#pragma once
#include <cstdint>

namespace fg {

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

// Unidade de distância da simulação: 0,1 mm. 10.000 unidades = 1 metro.
inline constexpr i32 kUnitsPerMeter = 10000;
// A simulação avança exatamente 60 vezes por segundo.
inline constexpr i32 kTicksPerSecond = 60;

// Retângulo alinhado aos eixos, relativo à origem do lutador (centro dos pés).
// x cresce para a frente do lutador; y cresce para cima.
struct Box {
  i32 x = 0;
  i32 y = 0;
  i32 w = 0;
  i32 h = 0;
};

// Retângulo em coordenadas de mundo.
struct WorldBox {
  i32 left = 0;
  i32 bottom = 0;
  i32 right = 0;
  i32 top = 0;
};

inline constexpr bool overlaps(const WorldBox& a, const WorldBox& b) {
  return a.left < b.right && b.left < a.right && a.bottom < b.top && b.bottom < a.top;
}

// Converte uma caixa relativa para o mundo, espelhando conforme o lado (facing = +1 direita, -1 esquerda).
inline constexpr WorldBox to_world(const Box& b, i32 origin_x, i32 origin_y, i32 facing) {
  WorldBox w{};
  if (facing >= 0) {
    w.left = origin_x + b.x;
    w.right = origin_x + b.x + b.w;
  } else {
    w.left = origin_x - b.x - b.w;
    w.right = origin_x - b.x;
  }
  w.bottom = origin_y + b.y;
  w.top = origin_y + b.y + b.h;
  return w;
}

template <typename T>
constexpr T clamp_value(T v, T lo, T hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

}  // namespace fg
