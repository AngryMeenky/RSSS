#ifndef RSSS_H
#  define RSSS_H

#  include <Stream.h>


class RSSS {
  public:
    RSSS(Stream &s, bool = false);

    int  available(void);
    int  read(uint8_t *, int);
    bool crcValid();

    int availableForWrite(void);
    int write(uint8_t *, int);  // write a data chunk and emit a synchronization point as needed

  private:
    Stream  *_serial;
    uint8_t  _last[4];
    uint16_t _readCrc;
    int16_t  _readSync;
    uint16_t _writeCrc;
    int16_t  _writeSync;
    int8_t   _remain;
    uint8_t  _hold;
    bool     _addTail;
    bool     _valid;

    int16_t _findSync();
    void _emitSync(int16_t);
};


#endif /* RSSS_H */

