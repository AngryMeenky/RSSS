#ifndef RSSS_CRC_H
#define RSSS_CRC_H

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>


using namespace godot;


class RsssCrc: public Object {
  GDCLASS(RsssCrc, Object);

  static int32_t         calc8(const PackedByteArray &data, int32_t seed, int32_t offset = 0);
  static PackedByteArray append8(const PackedByteArray &data, int32_t seed, int32_t offset = 0);
  static bool            validate8(const PackedByteArray &data, int32_t seed, int32_t offset = 0);
  static int32_t         calc16(const PackedByteArray &data, int32_t seed, int32_t offset = 0);
  static PackedByteArray append16(const PackedByteArray &data, int32_t seed, int32_t offset = 0);
  static bool            validate16(const PackedByteArray &data, int32_t seed, int32_t offset = 0);
  static int64_t         calc32(const PackedByteArray &data, int64_t seed, int32_t offset = 0);
  static PackedByteArray append32(const PackedByteArray &data, int64_t seed, int32_t offset = 0);
  static bool            validate32(const PackedByteArray &data, int64_t seed, int32_t offset = 0);

protected:
  static void _bind_methods();
};

#endif
