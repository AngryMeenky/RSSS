#include "RSSS.h"
#include "RsssCrc8.h"
#include "RsssCrc16.h"

#include <chrono>
#include <cstring>
#include <unistd.h>

#define CRC8_SEED    0x78
#define CRC16_SEED 0x8795


using namespace rsss;
using namespace godot;


RSSS::RSSS(StreamPeerSerial *s, bool t):
  serial(s),
  last{ 0, 0, 0, 0 },
  readCrc(0),
  readSync(0),
  writeCrc(0),
  writeSync(0),
  remain(0),
  hold(0),
  addTail(t),
  valid(!t) {
}


int64_t RSSS::read(PackedByteArray &data, int64_t offset, int64_t length) {
  if(serial.is_null()) {
    return -1;
  }

  if(addTail && remain != 0) {
    if(auto avail = serial->get_available_bytes(); avail > 0) {
      while(avail > 0 && remain > 0) {
        last[2 - remain] = serial->get_u8();
        avail--;
        remain--;
      }

      if(!remain) {
        valid = !calcCrc16(&last[0], 2, readCrc);
        data[offset] = hold;
        return 1;
      }
    }
    else if(avail < 0) {
      return avail; // some kind of read error?
    }

    return 0;  // didn't read enough to validate
  }

  if(readSync <= 0) {
    readSync = findSync();
  }

  if(readSync > 0) {
    auto bytes = serial->get_partial_data(std::min(length, static_cast<int64_t>(readSync)));

    if(auto count = bytes.size(); count > 0) {
      { PackedByteArray arr(bytes);
      memcpy(&data[offset], arr.ptr(), count); }

      if(addTail) {
        readCrc = calcCrc16(&data[offset], count, readCrc);
      }

      readSync -= count;

      if(!readSync && addTail) {
        remain = 2;
        hold = data[count -= 1];
        return count + read(data, offset + count, 1);
      }

      return count;
    }
  }

  return 0;
}


int64_t RSSS::write(const PackedByteArray &data, int64_t offset, int64_t length) {
  int64_t retVal = 0;
  auto count = length & 0xFFFF;

  if(serial.is_null()) {
    goto failure;
  }

  if(length == 0) {
    goto complete;
  }

  if(writeSync > 0) {
    length = std::min(length, static_cast<int64_t>(writeSync));
    Array arr;
    if(offset == 0 && length == data.size()) {
      arr = serial->put_partial_data(data);
    }
    else {
      arr = serial->put_partial_data(data.slice(offset, offset + length));
    }

    if(auto sent = length - arr.size(); sent > 0) {
      if(addTail) { writeCrc = calcCrc16(&data[offset], sent, writeCrc); }

      writeSync -= sent;
      count -= sent;
      offset += sent;
      retVal = sent;

      if(writeSync != 0) {
        goto complete;
      }
      else if(addTail) {
        // the syncronized chunk was completed
        PackedByteArray crc;
        crc.resize(2);
        crc[0] = writeCrc & 0xFF;
        crc[1] = writeCrc >> 8;
        serial->put_data(crc);
      }
    }
    else {
      goto complete;
    }
  }

  if(count > 0) {
    // emit a synchronization point
    if(emitSync(count)) {
      writeSync = count;
    }
    else if(retVal) {
      goto complete;
    }
    else {
      goto failure;
    }

    Array arr;
    if(offset == 0 && count == data.size()) {
      arr = serial->put_partial_data(data);
    }
    else {
      arr = serial->put_partial_data(data.slice(offset, offset + count));
    }

    if(auto sent = count - arr.size(); sent > 0) {
      writeSync -= sent;
      retVal += sent;

      if(addTail) {
        writeCrc = calcCrc16(&data[offset], sent, writeCrc);

        if(!writeSync) {
          // the syncronized chunk was completed
          PackedByteArray crc;
          crc.resize(2);
          crc[0] = writeCrc & 0xFF;
          crc[1] = writeCrc >> 8;
          serial->put_data(crc);
        }
      }
    }
  }

complete:
  return retVal;

failure:
  retVal = -1;
  goto complete;
}


static bool nextByte(StreamPeerSerial *s, uint8_t *d) {
  if(s->get_available_bytes() > 0) {
    *d = s->get_u8();
    return true;
  }

  return false;
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
  } while(nextByte(serial.ptr(), &last[3]));

  return 0;
}


bool RSSS::emitSync(std::uint16_t length) {
  PackedByteArray header;
  header.resize(4);
  header[0] = 0xAA;
  header[1] = length & 0xFF;
  header[2] = length >> 8;
  header[3] = 0;
  appendCrc8(&header[0], 3, 0x78);

  if(serial->put_data(header) == OK) {
    writeCrc = CRC16_SEED;
    return true;
  }

  return false;
}

bool RSSS::waitForSync(int64_t ms) {
  if(readSync > 0) {
    return true;
  }

  auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
  do {
    if((readSync = findSync()) == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  } while(readSync == 0 && std::chrono::steady_clock::now() < end);

  return readSync != 0;
}

