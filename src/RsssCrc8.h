#ifndef RSSS_CRC8
#  define RSSS_CRC8

#  include <cstdint>


namespace rsss {

  uint8_t calcCrc8(const uint8_t *, int, uint8_t);
  uint8_t appendCrc8(uint8_t *, int, uint8_t);
  bool    validateCrc8(const uint8_t *, int, uint8_t);

}

#endif /* RSSS_CRC8 */

