/*************************************************************************/
/*  serial_port.h                                                        */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2023 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2023 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef PACKET_PEER_RSSS_H
#define PACKET_PEER_RSSS_H

#include <deque>
#include <mutex>
#include <atomic>
#include <memory>
#include <thread>
#include <cstring>
#include <condition_variable>

#include <godot_cpp/variant/builtin_types.hpp>
#include <godot_cpp/classes/packet_peer_extension.hpp>

#include "RSSS.h"


using namespace godot;


class PacketPeerRsss : public PacketPeerExtension {
  GDCLASS(PacketPeerRsss, PacketPeerExtension);

  struct Packet {
    Packet(): Packet(nullptr, 0) {}
    Packet(const uint8_t *src, int64_t len):
      data(std::make_unique<uint8_t[]>(len)),
      size(len) {
      if(src) {
        memcpy(&data[0], src, len);
      }
    }
    Packet(std::unique_ptr<uint8_t[]> &&d, int64_t s):
      data(), size(s) {
      data.reset(d.release());
    }

    std::unique_ptr<uint8_t[]> data;
    int64_t                    size;
  };

  rsss::RSSS                 parser;
  std::mutex                 mutex_in;
  std::mutex                 mutex_out;
  std::condition_variable    cv_out;
  std::deque<Packet>         packets_in;
  std::deque<Packet>         packets_out;
  std::unique_ptr<uint8_t[]> buffer;
  std::thread                worker_in;
  std::thread                worker_out;
  std::atomic_bool           go;

public:
  PacketPeerRsss(StreamPeer * = nullptr);
  ~PacketPeerRsss();

  static PacketPeerRsss *wrap(StreamPeer *stream) { return new PacketPeerRsss(stream); }

  int32_t _get_max_packet_size() const override;
  int32_t _get_available_packet_count() const override;

  Error _get_packet(const uint8_t **r_buffer, int32_t* r_buffer_size) override;
  Error _put_packet(const uint8_t *p_buffer, int p_buffer_size) override;

protected:
  void readPackets();
  void writePackets();

  static void _bind_methods();
};

#endif // PACKET_PEER_RSSS_H
