#include "RSSS.h"
#include "RsssCrc8.h"

#include <errno.h>
#include <cstring>
#include <unistd.h>


using namespace rsss;


RSSS::RSSS(int s):
  serial(s),
  last{ 0, 0, 0, 0 },
  readSync(0),
  writeSync(0) {}


int RSSS::read(std::uint8_t *data, std::uint16_t length) {
  if(readSync <= 0) {
    readSync = findSync();
  }

  if(readSync > 0) {
    auto count = ::read(serial, data, std::min(length, readSync));

    if(count > 0) {
      readSync -= count;
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
      writeSync -= sent;
      count -= sent;
      data += sent;
      retVal = sent;
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
    if(last[0] == 0xAA && validateCrc8(&last[0], 4, 0x78)) {
      auto retVal = last[1] | (last[2] << 8);
      memset(&last[0], 0, 4);
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

  return ::write(serial, &packet[0], 4) == 4;
}

