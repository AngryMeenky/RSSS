#ifndef RSSS_H
#  define RSSS_H

#include <array>
#include <cstdint>

#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/stream_peer.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"


namespace rsss {

class RSSS {
  public:
    RSSS(bool t = false);

    bool initialize(const godot::Ref<godot::StreamPeer> &stream);

    int64_t read(       godot::PackedByteArray &, int64_t, int64_t);
    int64_t write(const godot::PackedByteArray &, int64_t, int64_t);
    bool    crcValid() const { return valid; }

    int64_t maximumSync() const { return 0xFFFF; }
    int64_t readSyncRemaining() const { return readSync; }
    int64_t writeSyncRemaining() const { return writeSync; }

    bool waitForSync(int64_t);

    explicit operator bool() const { return serial.is_valid(); }

  private:
    godot::Ref<godot::StreamPeer> serial;
    std::array<std::uint8_t, 4>   last;
    std::uint16_t                 readCrc;
    std::uint16_t                 readSync;
    std::uint16_t                 writeCrc;
    std::uint16_t                 writeSync;
    std::int8_t                   remain;
    std::uint8_t                  hold;
    bool                          addTail;
    bool                          valid;

    std::uint16_t findSync();
    bool          emitSync(std::uint16_t);
};

}


#endif /* RSSS_H */

