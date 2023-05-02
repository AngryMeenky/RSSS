#ifndef RSSS_CRC16
#  define RSSS_CRC16

namespace rsss {

  uint16_t calcCrc16(const uint8_t *, int, uint16_t);
  uint16_t appendCrc16(uint8_t *, int, uint16_t);
  bool     validateCrc16(const uint8_t *, int, uint16_t);

}

#endif /* RSSS_CRC16 */

