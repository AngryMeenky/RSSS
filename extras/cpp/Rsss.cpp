#include "RSSS.h"
#include "RsssCrc8.h"
#include "RsssCrc16.h"

#include <errno.h>
#include <cstring>
#include <unistd.h>

#define CRC8_SEED    0x78
#define CRC16_SEED 0x8795


using namespace rsss;


RSSS::RSSS(int s, bool t):
  serial(s),
  last{ 0, 0, 0, 0 },
  readCrc(0),
  readSync(0),
  writeCrc(0),
  writeSync(0),
  remain(0),
  hold(0),
  addTail(t),
  valid(!t) {}


int RSSS::read(std::uint8_t *data, std::uint16_t length) {
  if(addTail && remain != 0) {
    auto count = ::read(serial, &last[2 - remain], remain);
    if(count > 0) {
      if(!(remain -= count)) {
        valid = !calcCrc16(&last[0], 2, readCrc);
        *data = hold;
        return 1;
      }
    }
    else if(count < 0 && errno != EAGAIN) {
      return count; // blocking isn't an error, but everything else is
    }

    return 0;  // didn't read enough to validate
  }

  if(readSync <= 0) {
    readSync = findSync();
  }

  if(readSync > 0) {
    auto count = ::read(serial, data, std::min(length, readSync));

    if(count > 0) {
      if(addTail) {
        readCrc = calcCrc16(data, count, readCrc);
      }

      readSync -= count;

      if(!readSync && addTail) {
        remain = 2;
        hold = data[count -= 1];
        return count + read(&data[count], 1);
      }
    }
    else if(count < 0 && errno == EAGAIN) {
      count = 0; // blocking isn't an error
    }

    return count;
  }

  return 0;
}


int RSSS::write(const std::uint8_t *data, std::uint16_t length) {
  int retVal = 0;
  auto count = length;

  if(length == 0) {
    goto complete;
  }

  if(writeSync > 0) {
    if(auto sent = ::write(serial, data, std::min(count, writeSync)); sent > 0) {
      if(addTail) { writeCrc = calcCrc16(data, sent, writeCrc); }

      writeSync -= sent;
      count -= sent;
      data += sent;
      retVal = sent;

      if(writeSync != 0) {
        goto complete;
      }
      else if(addTail) {
        // the syncronized chunk was completed
        std::uint8_t buffer[2] = { static_cast<std::uint8_t>( writeCrc       & 0xFF),
                                   static_cast<std::uint8_t>((writeCrc >> 8) & 0xFF) };
        ::write(serial, buffer, 2);
      }
    }
    else if(sent < 0 && errno != EAGAIN) {
      goto failure;
    }
  }

  if(count > 0) {
    // emit a synchronization point
    if(emitSync(length)) {
      writeSync = length;
    }
    else if(retVal || errno == EAGAIN) {
      goto complete;
    }
    else {
      goto failure;
    }

    if(auto sent = ::write(serial, data, count); sent > 0) {
      writeSync -= sent;
      retVal += sent;

      if(addTail) {
        writeCrc = calcCrc16(data, sent, writeCrc);

        if(!writeSync) {
          // the syncronized chunk was completed
          std::uint8_t buffer[2] = { static_cast<std::uint8_t>( writeCrc       & 0xFF),
                                     static_cast<std::uint8_t>((writeCrc >> 8) & 0xFF) };
          ::write(serial, buffer, 2);
        }
      }
    }
    else if(sent < 0 && errno != EAGAIN && !retVal) {
      goto failure; // something bad happened
    }
  }

complete:
  return retVal;

failure:
  retVal = -1;
  goto complete;
}


std::uint16_t RSSS::findSync() {
  do {
    if(last[0] == 0xAA && validateCrc8(&last[0], 4, CRC8_SEED)) {
      auto retVal = last[1] | (last[2] << 8);
      memset(&last[0], 0, 4);
      readCrc = CRC16_SEED;
      valid = !addTail;
      return retVal;
    }

    memmove(&last[0], &last[1], 3);
  } while(::read(serial, &last[3], 1) == 1);

  return 0;
}


bool RSSS::emitSync(std::uint16_t length) {
  std::array<std::uint8_t, 4> packet{
    0xAA, static_cast<std::uint8_t>(length), static_cast<std::uint8_t>(length >> 8), 0
  };
  appendCrc8(&packet[0], 3, 0x78);

  if(::write(serial, &packet[0], 4) == 4) {
    writeCrc = CRC16_SEED;
    return true;
  }

  return false;
}

