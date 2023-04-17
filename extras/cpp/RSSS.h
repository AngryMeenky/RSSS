#ifndef RSSS_H
#  define RSSS_H

#include <array>
#include <cstdint>


namespace rsss {

class RSSS {
  public:
    RSSS(int s);
    RSSS(): RSSS(-1) {}

    int read(       std::uint8_t *, std::uint16_t); // find a synchronization point and then read bytes
    int write(const std::uint8_t *, std::uint16_t); // emit a synchronization point and then write bytes

    explicit operator int() const { return serial; }

  private:
    int                         serial;
    std::array<std::uint8_t, 4> last;
    std::uint16_t               readSync;
    std::uint16_t               writeSync;

    std::uint16_t findSync();
    bool          emitSync(std::uint16_t);
};

}


#endif /* RSSS_H */

