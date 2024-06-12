#include "Crc.h"
#include "RsssCrc8.h"
#include "RsssCrc16.h"
#include "RsssCrc32.h"

#include <godot_cpp/core/class_db.hpp>


int32_t RsssCrc::calc8(const PackedByteArray &data, int32_t seed, int32_t off) {
  return rsss::calcCrc8(&data.ptr()[off], data.size() - off, static_cast<uint8_t>(seed));
}


PackedByteArray RsssCrc::append8(const PackedByteArray &data, int32_t seed, int32_t off) {
  PackedByteArray result;
  result.resize(data.size() + 1);
  memcpy(result.ptrw(), data.ptr(), data.size());
  (void) rsss::appendCrc8(&result.ptrw()[off], result.size() - 1 - off, static_cast<uint8_t>(seed));
  return result;
}


bool RsssCrc::validate8(const PackedByteArray &data, int32_t seed, int32_t off) {
  return rsss::validateCrc8(&data.ptr()[off], data.size() - off, static_cast<uint8_t>(seed));
}


int32_t RsssCrc::calc16(const PackedByteArray &data, int32_t seed, int32_t off) {
  return rsss::calcCrc16(&data.ptr()[off], data.size() - off, static_cast<uint16_t>(seed));
}


PackedByteArray RsssCrc::append16(const PackedByteArray &data, int32_t seed, int32_t off) {
  PackedByteArray result;
  result.resize(data.size() + 2);
  memcpy(result.ptrw(), data.ptr(), data.size());
  (void) rsss::appendCrc16(&result.ptrw()[off], result.size() - 2 - off, static_cast<uint16_t>(seed));
  return result;
}


bool RsssCrc::validate16(const PackedByteArray &data, int32_t seed, int32_t off) {
  return rsss::validateCrc16(&data.ptr()[off], data.size() - off, static_cast<uint16_t>(seed));
}


int64_t RsssCrc::calc32(const PackedByteArray &data, int64_t seed, int32_t off) {
  return rsss::calculateCrc32(&data.ptr()[off], data.size() - off, static_cast<uint32_t>(seed));
}


PackedByteArray RsssCrc::append32(const PackedByteArray &data, int64_t seed, int32_t off) {
  PackedByteArray result;
  result.resize(data.size() + 4);
  memcpy(result.ptrw(), data.ptr(), data.size());
  (void) rsss::appendCrc32(&result.ptrw()[off], result.size() - 4 - off, static_cast<uint32_t>(seed));
  return result;
}


bool RsssCrc::validate32(const PackedByteArray &data, int64_t seed, int32_t off) {
  return rsss::validateCrc32(&data.ptr()[off], data.size() - off, static_cast<uint32_t>(seed));
}


void RsssCrc::_bind_methods() {
  ClassDB::bind_static_method("RsssCrc", D_METHOD("calc8", "data", "seed", "offset"), &RsssCrc::calc8, DEFVAL(0));
  ClassDB::bind_static_method("RsssCrc", D_METHOD("append8", "data", "seed", "offset"), &RsssCrc::append8, DEFVAL(0));
  ClassDB::bind_static_method("RsssCrc", D_METHOD("validate8", "data", "seed", "offset"), &RsssCrc::validate8, DEFVAL(0));

  ClassDB::bind_static_method("RsssCrc", D_METHOD("calc16", "data", "seed", "offset"), &RsssCrc::calc16, DEFVAL(0));
  ClassDB::bind_static_method("RsssCrc", D_METHOD("append16", "data", "seed", "offset"), &RsssCrc::append16, DEFVAL(0));
  ClassDB::bind_static_method("RsssCrc", D_METHOD("validate16", "data", "seed", "offset"), &RsssCrc::validate16, DEFVAL(0));

  ClassDB::bind_static_method("RsssCrc", D_METHOD("calc32", "data", "seed", "offset"), &RsssCrc::calc32, DEFVAL(0));
  ClassDB::bind_static_method("RsssCrc", D_METHOD("append32", "data", "seed", "offset"), &RsssCrc::append32, DEFVAL(0));
  ClassDB::bind_static_method("RsssCrc", D_METHOD("validate32", "data", "seed", "offset"), &RsssCrc::validate32, DEFVAL(0));
}


