#ifndef RSSS_H
#  define RSSS_H

#include <array>
#include <cstdint>
#include <QSerialPort>


namespace rsss {

class RSSS {
  public:
    RSSS(QSerialPort *s);
    ~RSSS();

    void reset(QSerialPort *);

    qint64 bytesAvailable();

    QByteArray read(qint64); // find a synchronization point and then read bytes
    qint64     read(char *, qint64); // find a synchronization point and then read bytes

    qint64 write(const QByteArray &); // emit a synchronization point and then write bytes
    qint64 write(const char *, qint64); // emit a synchronization point and then write bytes

    operator bool() const { return !!_serial; }

    explicit operator       QSerialPort *()       { return _serial; }
    explicit operator const QSerialPort *() const { return _serial; }

  private:
    QSerialPort                 *_serial;
    std::array<std::uint8_t, 4>  _last;
    qint64                       _readSync;
    qint64                       _writeSync;

    qint64 _findSync();
    bool   _emitSync(qint64);
};

}


#endif /* RSSS_H */

