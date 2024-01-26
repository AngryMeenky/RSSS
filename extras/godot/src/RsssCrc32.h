#pragma once

#  include <cstdint>
#  include <unistd.h>


namespace rsss {

std::uint32_t calculateCrc32(const std::uint8_t *data, size_t length, std::uint32_t seed);
std::uint32_t appendCrc32(std::uint8_t *data, size_t length, std::uint32_t seed);
bool          validateCrc32(const std::uint8_t *data, size_t length, std::uint32_t seed);

}

