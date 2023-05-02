#include "Rsss.h"
#include "RsssCrc8.h"
#include "RsssCrc16.h"

#include <errno.h>
#include <unistd.h>

#define CRC8_SEED    0x78
#define CRC16_SEED 0x8795


using namespace rsss;


RSSS::RSSS(QSerialPort *s, bool t):
  _serial(s),
  _last{ 0, 0, 0, 0 },
  _readSync(0),
  _writeSync(0),
  _readCrc(0),
  _writeCrc(0),
  _remain(0),
  _hold(0),
  _addTail(t),
  _valid(!t) {}


RSSS::~RSSS() {
  if(_serial) {
    delete _serial;
    _serial = nullptr;
  }
}


void RSSS::reset(QSerialPort *serial, bool t) {
  if(_serial) {
    delete _serial;
  }

  _serial = serial;
  memset(&_last[0], 0, 4);
  _writeSync = _readSync = 0;
  _writeCrc = _readSync = 0;
  _remain = 0;
  _addTail = t;
  _valid = !t;
}


qint64 RSSS::bytesAvailable() {
  qint64 retVal = 0;

  if(_readSync <= 0) {
    // attempt to find a synchronization point
    _readSync = _findSync();
  }

  // was a synchronization point was found?
  if(_readSync > 0) {
    int avail = _serial->bytesAvailable();
    retVal =  _readSync <= avail ? _readSync : avail;
  }

  return retVal;
}


QByteArray RSSS::read(qint64 length) {
  QByteArray buffer(length, '\0');

  if((length = read(buffer.data(), length)) > 0) {
    buffer.resize(length);
  }
  else {
    buffer.resize(0);
  }

  return buffer;
}


qint64 RSSS::read(char *data, qint64 length) {
  // handle optional CRC processing
  if(_addTail && _remain != 0) {
    if(_serial->bytesAvailable() >= _remain) {
      if(auto count = _serial->read(reinterpret_cast<char *>(&_last[2 - _remain]), _remain); count > 0) {
        if(!(_remain -= count)) {
          _valid = !calcCrc16(&_last[0], 2, _readCrc);
          *data = _hold;
          return 1;
        }
      }
    }

    return 0; // didn't read enough to validate CRC
  }

  auto retVal = 0;

  if(_readSync <= 0) {
    _readSync = _findSync();
  }

  if(_readSync > 0) {
    if(auto count = _serial->read(data, std::min(length, _readSync)); count > 0) {
      if(_addTail) {
        _readCrc = calcCrc16(reinterpret_cast<uint8_t *>(data), count, _readCrc);
      }

      if(!(_readSync -= count) && _addTail) {
        _remain = 2;
        _hold = data[count -= 1];
        count += read(&data[count], 1); // dirty hack
      }

      retVal = count;
    }
    else if(count < 0) {
      retVal = count;
    }
  }

  return retVal;
}


qint64 RSSS::write(const QByteArray &buffer) {
  return write(buffer.data(), buffer.size());
}


qint64 RSSS::write(const char *data, qint64 length) {
  int retVal = 0;
  auto count = length;

  if(count == 0) {
    goto complete;
  }

  if(_writeSync > 0) {
    if(auto sent = _serial->write(data, std::min(count, _writeSync)); sent > 0) {
      // update the written CRC if required
      if(_addTail) {
        _writeCrc = rsss::calcCrc16(reinterpret_cast<const uint8_t *>(data), sent, _writeCrc);
      }

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
        _serial->write(reinterpret_cast<char *>(&buffer[0]), 2); // write the tail bytes
      }
    }
    else if(sent <= 0) {
      goto failure;
    }
  }

  if(count > 0) {
    // emit a synchronization point
    if(_emitSync(length)) {
      _writeSync = std::min(length, 0xFFFFLL);
    }
    else if(retVal) {
      goto complete;
    }
    else {
      goto failure;
    }

    if(auto sent = _serial->write(data, count); sent > 0) {
      _writeSync -= sent;
      retVal += sent;

      if(_addTail) {
        // update the written CRC if required
        _writeCrc = calcCrc16(reinterpret_cast<const uint8_t*>(data), sent, _writeCrc);
        if(!_writeSync) {
          // the syncronized chunk was completed
          uint8_t buffer[2] = { static_cast<uint8_t>( _writeCrc       & 0xFF),
                                static_cast<uint8_t>((_writeCrc >> 8) & 0xFF) };
          _serial->write(reinterpret_cast<char *>(&buffer[0]), 2); // write the tail bytes
        }
      }
    }
    else if(sent == 0 && !retVal) {
      goto complete;
    }
    else if(sent <= 0) {
      goto failure; // something bad happened
    }
  }

complete:
  return retVal;

failure:
  retVal = -1;
  goto complete;
}


qint64 RSSS::_findSync() {
  qint64 retVal = 0;

  while(_serial->bytesAvailable() > 0) {
    memmove(&_last[0], &_last[1], 3);
    _serial->read(reinterpret_cast<char *>(&_last[3]), 1);
    if(_last[0] == 0xAA && validateCrc8(&_last[0], 4, CRC8_SEED)) {
      retVal = _last[1] | (_last[2] << 8);
      memset(&_last[0], 0, 4);
      _readCrc = CRC16_SEED;
      _valid = !_addTail;
      break;
    }
  }

  return retVal;
}


bool RSSS::_emitSync(qint64 length) {
  length = std::min(length, 0xFFFFLL);

  std::array<std::uint8_t, 4> packet{
    0xAA, static_cast<std::uint8_t>(length), static_cast<std::uint8_t>(length >> 8), 0
  };
  appendCrc8(&packet[0], 3, CRC8_SEED);

  if(_serial->write(reinterpret_cast<char *>(&packet[0]), 4) == 4) {
    _writeCrc = CRC16_SEED;
    return true;
  }

  return false;
}

