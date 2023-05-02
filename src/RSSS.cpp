#include "RSSS.h"
#include "RsssCrc8.h"
#include "RsssCrc16.h"


#define CRC8_SEED    0x78
#define CRC16_SEED 0x8795


RSSS::RSSS(Stream &s, bool tail):
  _serial(&s),
  _readCrc(0),
  _readSync(0),
  _writeCrc(0),
  _writeSync(0),
  _remain(0),
  _hold(0),
  _addTail(tail),
  _valid(!tail) {
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
  // handle optional CRC processing
  if(_addTail && _remain != 0) {
    if(_serial->available() >= _remain) {
      uint8_t buffer[2];
      int count = _serial->readBytes(&buffer[2 - _remain], _remain);

      if(count > 0) {
        if(!(_remain -= count)) {
          _valid = !rsss::calcCrc16(&buffer[0], 2, _readCrc);
          buffer[0] = _hold;
          _hold = 0;
          return 1;
        }
      }
    }

    return 0; // didn't read enough to validate
  }

  // handle reading data bytes
  int avail = available();
  if(avail < max) {
    max = avail;
  }

  if(max > 0) {
    int count = _serial->readBytes(buffer, max);
    if(count > 0) {
      if(_addTail) {
        _readCrc = rsss::calcCrc16(buffer, count, _readCrc);
      }

      _readSync -= count;
      if(!_readSync && _addTail) {
        _remain = 2;
        _hold = buffer[count -= 1];
        return count + read(&buffer[count], 1); // dirty hack
      }
    }

    return count;
  }

  return 0;
}


bool RSSS::crcValid(void) {
  return _valid;
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
  //int avail = availableForWrite();
  int count = length;// <= avail ? length : avail;
  int retVal = 0;

  if(count == 0) {
    goto complete; // nothing to do here
  }


  // exhaust any remaining synchronized bytes
  if(_writeSync > 0) {
    int sent = _serial->write(data, count >= _writeSync ? _writeSync : count);
    if(sent > 0) {
      // update the written CRC if required
      if(_addTail) { _writeCrc = rsss::calcCrc16(data, sent, _writeCrc); }

      _writeSync -= sent;
      count -= sent;
      data += sent;
      retVal = sent;

      if(_writeSync != 0) {
        goto complete; // still in the synchronized region
      }
      else if(_addTail) {
        // the syncronized chunk was completed
        uint8_t buffer[2] = { static_cast<uint8_t>( _writeCrc       & 0xFF),
                              static_cast<uint8_t>((_writeCrc >> 8) & 0xFF) };
        _serial->write(&buffer[0], 2); // write the tail bytes
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
      if(_addTail) {
        // update the written CRC if required
        _writeCrc = rsss::calcCrc16(data, sent, _writeCrc);
        if(!_writeSync) {
          // the syncronized chunk was completed
          uint8_t buffer[2] = { static_cast<uint8_t>( _writeCrc       & 0xFF),
                                static_cast<uint8_t>((_writeCrc >> 8) & 0xFF) };
          _serial->write(&buffer[0], 2); // write the tail bytes
        }
      }
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

    if(_last[0] == 0xAA && rsss::validateCrc8(&_last[0], 4, CRC8_SEED)) {
      _valid = !_addTail;
      _readCrc = CRC16_SEED;
      return _last[1] | (_last[2] << 8);
    }
  }

  return -1;
}


void RSSS::_emitSync(int16_t len) {
  uint8_t packet[4] = { 0xAA, (uint8_t) (len & 0xFF), (uint8_t) ((len >> 8) & 0xFF), 0 };
  rsss::appendCrc8(&packet[0], 3, CRC8_SEED);
  _serial->write(&packet[0], sizeof(packet));
  _writeCrc = CRC16_SEED;
}

