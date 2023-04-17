#include "RSSS.h"
#include "RsssCrc8.h"


RSSS::RSSS(Stream &s):
  _serial(&s),
  _readSync(0),
  _writeSync(0) {
  memset(&_last[0], 0, sizeof(_last));
}


int RSSS::available(void) {
  if(_readSync <= 0) {
    // attempt to find a synchronization point
    _readSync = _findSync();
  }

  // was a synchronization point was found?
  if(_readSync > 0) {
    int avail = _serial->available();
    return _readSync <= avail ? _readSync : avail;
  }

  // no synchronization point found
  return 0;
}


int RSSS::read(uint8_t *buffer, int max) {
  int avail = available();
  if(avail < max) {
    max = avail;
  }

  if(max > 0) {
    int count = _serial->readBytes(buffer, max);
    if(count > 0) {
      _readSync -= count;
    }

    return count;
  }

  return 0;
}


int RSSS::availableForWrite(void) {
  int avail = _serial->availableForWrite();

  if(avail >= _writeSync) {
    if(avail - _writeSync >= sizeof(_last)) {
      avail -= sizeof(_last);
    }
    else {
      avail = _writeSync;
    }
  }

  return avail;
}


int RSSS::write(uint8_t *data, int length) {
  int avail = availableForWrite();
  int count = length <= avail ? length : avail;
  int retVal = 0;

  if(count == 0) {
    goto complete; // nothing to do here
  }


  // exhaust any remaining synchronized bytes
  if(_writeSync > 0) {
    int sent = _serial->write(data, count >= _writeSync ? _writeSync : count);
    if(sent > 0) {
      _writeSync -= sent;
      count -= sent;
      data += sent;
      retVal = sent;

      if(_writeSync != 0) {
        goto complete; // still in the synchronized region
      }
    }
    else if(sent == 0) {
      goto complete; // wrote nothing?
    }
    else {
      goto failure; // something bad happened
    }
  }

  if(count > 0) {
    // emit a synchronization point
    _emitSync(length);
    _writeSync = length;

    // write data
    int sent = _serial->write(data, count);
    if(sent > 0) {
      _writeSync -= sent;
      retVal += sent;
    }
    else if(sent == 0) {
      goto complete; // wrote nothing?
    }
    else if(!retVal) {
      goto failure; // something bad happened
    }
  }

complete:
  return retVal;

failure:
  retVal = -1;
  goto complete;
}


int16_t RSSS::_findSync() {
  while(_serial->available() > 0) {
    _last[0] = _last[1];
    _last[1] = _last[2];
    _last[2] = _last[3];
    _last[3] = _serial->read();

    if(_last[0] == 0xAA && rsss::validateCrc8(&_last[0], 4, 0x78)) {
      return _last[1] | (_last[2] << 8);
    }
  }

  return -1;
}


void RSSS::_emitSync(int16_t len) {
  uint8_t packet[4] = { 0xAA, (uint8_t) (len & 0xFF), (uint8_t) ((len >> 8) & 0xFF), 0 };
  rsss::appendCrc8(&packet[0], 3, 0x78);
  _serial->write(&packet[0], sizeof(packet));
}

