// Serialização explícita, campo a campo, em little-endian. Independe de padding e de compilador,
// por isso o checksum é comparável entre Windows (MSVC) e Linux (GCC/Clang).
#pragma once
#include <string>
#include <vector>

#include "fg/state.h"

namespace fg {

class ByteWriter {
 public:
  void u8v(u8 v) { buf_.push_back(v); }
  void i8v(i8 v) { u8v(static_cast<u8>(v)); }
  void boolv(bool v) { u8v(v ? 1 : 0); }
  void u16v(u16 v) {
    u8v(static_cast<u8>(v & 0xFF));
    u8v(static_cast<u8>(v >> 8));
  }
  void i16v(i16 v) { u16v(static_cast<u16>(v)); }
  void u32v(u32 v) {
    for (int i = 0; i < 4; ++i) u8v(static_cast<u8>((v >> (8 * i)) & 0xFF));
  }
  void i32v(i32 v) { u32v(static_cast<u32>(v)); }
  void u64v(u64 v) {
    for (int i = 0; i < 8; ++i) u8v(static_cast<u8>((v >> (8 * i)) & 0xFF));
  }
  void i64v(i64 v) { u64v(static_cast<u64>(v)); }
  void str(const std::string& s) {
    u32v(static_cast<u32>(s.size()));
    for (char c : s) u8v(static_cast<u8>(c));
  }
  const std::vector<u8>& bytes() const { return buf_; }
  std::vector<u8>& bytes() { return buf_; }

 private:
  std::vector<u8> buf_;
};

class ByteReader {
 public:
  explicit ByteReader(const std::vector<u8>& b) : buf_(b) {}
  bool ok() const { return ok_; }
  bool at_end() const { return pos_ == buf_.size(); }
  u8 u8v() {
    if (pos_ >= buf_.size()) {
      ok_ = false;
      return 0;
    }
    return buf_[pos_++];
  }
  u16 u16v() {
    const u16 lo = u8v();
    const u16 hi = u8v();
    return static_cast<u16>(lo | (hi << 8));
  }
  u32 u32v() {
    u32 v = 0;
    for (int i = 0; i < 4; ++i) v |= static_cast<u32>(u8v()) << (8 * i);
    return v;
  }
  u64 u64v() {
    u64 v = 0;
    for (int i = 0; i < 8; ++i) v |= static_cast<u64>(u8v()) << (8 * i);
    return v;
  }
  i32 i32v() { return static_cast<i32>(u32v()); }

 private:
  const std::vector<u8>& buf_;
  size_t pos_ = 0;
  bool ok_ = true;
};

u64 fnv1a64(const u8* data, size_t size, u64 seed = 0xcbf29ce484222325ull);

void serialize(const MatchState& s, ByteWriter& w);
u64 checksum(const MatchState& s);

}  // namespace fg
