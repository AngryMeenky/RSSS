#ifndef RSSS_H
#  define RSSS_H

#  include <Stream.h>


class RSSS {
  public:
    RSSS(Stream &s);

    int available(void);
    int read(uint8_t *, int);

    int availableForWrite(void);
    int write(uint8_t *, int);  // write a data chunk and emit a synchronization point as needed

  private:
    Stream *_serial;
    uint8_t _last[4];
    int16_t _readSync;
    int16_t _writeSync;

    int16_t _findSync();
    void _emitSync(int16_t);
};



#endif /* RSSS_H */

