#ifndef RSSS_CRC8_H
#  define RSSS_CRC8_H

#  include <cstdint>


namespace rsss {


uint8_t calcCrc8(const uint8_t *data, int len, uint8_t crc);
uint8_t appendCrc8(uint8_t *data, int len, uint8_t crc);
bool    validateCrc8(const uint8_t *data, int len, uint8_t crc);


}


#endif /* RSSS_CRC8_H */
