#include "fg/serialize.h"

namespace fg {

u64 fnv1a64(const u8* data, size_t size, u64 seed) {
  u64 h = seed;
  for (size_t i = 0; i < size; ++i) {
    h ^= data[i];
    h *= 0x100000001b3ull;
  }
  return h;
}

namespace {

void serialize_fighter(const FighterState& f, ByteWriter& w) {
  w.i32v(f.character);
  w.u8v(static_cast<u8>(f.scheme));
  w.i32v(f.x);
  w.i32v(f.y);
  w.i32v(f.vx);
  w.i32v(f.vy);
  w.i32v(f.gravity);
  w.i8v(f.facing);
  w.u8v(static_cast<u8>(f.act));
  w.u8v(static_cast<u8>(f.jump_kind));
  w.i8v(f.jump_dir);
  w.i16v(f.move);
  w.i16v(f.move_frame);
  w.i16v(f.timer);
  w.u8v(f.hits_done);
  w.u8v(f.armor_used);
  w.boolv(f.move_connected);
  w.boolv(f.move_whiffed_recovery);
  w.i32v(f.health);
  w.i32v(f.meter);
  w.i16v(f.hitstop);
  w.i32v(f.push_remaining);
  w.i16v(f.combo_hits);
  w.i32v(f.combo_damage);
  w.i32v(f.combo_scaling_extra);
  w.i16v(f.juggle);
  w.boolv(f.combo_counter_hit);
  w.boolv(f.knockdown_pending);
  w.i16v(f.throw_timer);
  w.i16v(f.throw_move);
  w.i16v(f.retreat_frames);
  for (int i = 0; i < kInputHistory; ++i) w.u8v(f.input.dir[i]);
  for (int i = 0; i < kInputHistory; ++i) w.u16v(f.input.btn[i]);
  w.u32v(f.input.head);
  w.u32v(f.consumed_press);
  w.i16v(f.buffer_bonus);
  w.i16v(f.throw_protect);
  w.boolv(f.air_action_used);
  w.boolv(f.crouch_stun);
  w.i16v(f.damage_permil_override);
}

}  // namespace

void serialize(const MatchState& s, ByteWriter& w) {
  w.u32v(s.frame);
  w.u8v(static_cast<u8>(s.phase));
  w.i16v(s.phase_timer);
  w.u8v(s.round);
  w.u8v(s.wins[0]);
  w.u8v(s.wins[1]);
  w.i8v(s.round_winner);
  w.i8v(s.match_winner);
  w.i32v(s.timer_frames);
  w.i32v(s.mare);
  w.i16v(s.freeze);
  w.i8v(s.freeze_owner);
  w.u8v(s.options);
  w.u32v(s.rng);
  serialize_fighter(s.f[0], w);
  serialize_fighter(s.f[1], w);
}

u64 checksum(const MatchState& s) {
  ByteWriter w;
  serialize(s, w);
  return fnv1a64(w.bytes().data(), w.bytes().size());
}

}  // namespace fg
