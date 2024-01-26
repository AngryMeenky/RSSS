#include "PacketPeerRsss.h"

#include "godot_cpp/variant/utility_functions.hpp"


PacketPeerRsss::PacketPeerRsss(StreamPeerSerial *s):
  parser(s),
  buffer(new uint8_t[parser.maximumSync()]),
  go(s != nullptr),
  encode_override(0x7FFFFFFF),
  pkt_err(OK) {
  if(go) {
    worker_in  = std::thread(&PacketPeerRsss::readPackets,  this);
    worker_out = std::thread(&PacketPeerRsss::writePackets, this);
  }
}


PacketPeerRsss::~PacketPeerRsss() {
  go = false;

  if(worker_in.joinable()) {
    worker_in.join();
  }

  if(worker_out.joinable()) {
    worker_out.join();
  }
}


Variant PacketPeerRsss::get_var(bool allow_objects) {
  auto packet = get_packet();
  if(packet.size() > 0) {
    return allow_objects ?
             UtilityFunctions::bytes_to_var_with_objects(packet) :
             UtilityFunctions::bytes_to_var(packet);
  }

  return Variant();
}


Error PacketPeerRsss::put_var(const Variant &var, bool full_objects) {
  return put_packet(full_objects ?
                      UtilityFunctions::var_to_bytes_with_objects(var) :
                      UtilityFunctions::var_to_bytes(var));
}


PackedByteArray PacketPeerRsss::get_packet() {
  PackedByteArray result;
  std::unique_lock<std::mutex> guard(mutex_in);
  if(packets_in.empty()) {
    pkt_err = ERR_UNAVAILABLE;
  }
  else {
    auto &packet = packets_in.front();
    result.resize(packet.size);
    memcpy(result.ptrw(), &packet.data[0], packet.size);
    packets_in.pop_front();
    pkt_err = OK;
  }

  return result;
}


Error PacketPeerRsss::put_packet(const PackedByteArray &buffer) {
  if(buffer.size() == 0) {
    return ERR_INVALID_DATA;
  }

  std::unique_lock<std::mutex> guard(mutex_out);
  packets_out.emplace_back(std::make_unique<uint8_t[]>(buffer.size()), buffer.size());
  memcpy(&packets_out.back().data[0], buffer.ptr(), buffer.size());
  cv_out.notify_one();
  return OK;
}


Error PacketPeerRsss::get_packet_error() const {
  return pkt_err;
}


int32_t PacketPeerRsss::get_available_packet_count() const {
  std::unique_lock<std::mutex> guard(const_cast<PacketPeerRsss *>(this)->mutex_in);
  return static_cast<int64_t>(packets_in.size());
}


int32_t PacketPeerRsss::get_encode_buffer_max_size() const {
  return std::min(parser.maximumSync(), static_cast<int64_t>(encode_override));
}

void PacketPeerRsss::set_encode_buffer_max_size(int32_t max_size) {
  encode_override = max_size;
}


void PacketPeerRsss::readPackets() {
  std::unique_ptr<uint8_t[]> packet;
  int64_t remaining;

  while(go) {
    if(!parser.waitForSync(100)) {
      continue;
    }

    if((remaining = parser.readSyncRemaining()) <= 0) {
      continue; // how does that even happen?
    }

    PackedByteArray packet;
    packet.resize(remaining);

    do {
      if(auto result = parser.read(packet, packet.size() - remaining, remaining); result < 0) {
        remaining = -1;
        break; // some kind of error
      }
      else {
        remaining -= result;
      }
    } while(remaining > 0);

    // check for error state
    if(remaining == 0) {
      std::unique_lock<std::mutex> guard(mutex_in);
      packets_in.emplace_back(std::make_unique<uint8_t[]>(packet.size()), packet.size());
      memcpy(&packets_in.back().data[0], packet.ptr(), packet.size());
    }
  }
}


void PacketPeerRsss::writePackets() {
  std::unique_lock<std::mutex> guard(mutex_out);
  while(go) {
    if(packets_out.empty()) {
      cv_out.wait_for(guard, std::chrono::milliseconds(100));
      continue;
    }

    PackedByteArray packet;
    packet.resize(packets_out.front().size);
    memcpy(packet.ptrw(), &packets_out.front().data[0], packet.size());
    // release the mutex during the actuall sending of the serial data
    guard.unlock();

    int64_t sent = 0;
    do {
      if(auto result = parser.write(packet, sent, packet.size() - sent); result < 0) {
        break; // some kind of error
      }
      else {
        sent += result;
      }
    } while(sent < packet.size());
    guard.lock();
    packets_out.pop_front();
  }
}


void PacketPeerRsss::set_stream_peer(StreamPeerSerial *peer) {
  if(go) {
    go = false;

    if(worker_in.joinable()) {
      worker_in.join();
    }

    if(worker_out.joinable()) {
      worker_out.join();
    }
  }

  parser = rsss::RSSS(peer);

  if(parser) {
    go = true;
    worker_in  = std::thread(&PacketPeerRsss::readPackets,  this);
    worker_out = std::thread(&PacketPeerRsss::writePackets, this);
  }
}


StreamPeerSerial *PacketPeerRsss::get_stream_peer() {
  return parser.getStream();
}


void PacketPeerRsss::_bind_methods() {
  ClassDB::bind_static_method("PacketPeerRsss", D_METHOD("wrap", "stream"), &PacketPeerRsss::wrap);

  ClassDB::bind_method(D_METHOD("get_var", "allow_objects"), &PacketPeerRsss::get_var);
  ClassDB::bind_method(D_METHOD("put_var", "var", "full_objects"), &PacketPeerRsss::put_var);
  ClassDB::bind_method(D_METHOD("get_packet"), &PacketPeerRsss::get_packet);
  ClassDB::bind_method(D_METHOD("put_packet", "buffer"), &PacketPeerRsss::put_packet);
  ClassDB::bind_method(D_METHOD("get_packet_error"), &PacketPeerRsss::get_packet_error);
  ClassDB::bind_method(D_METHOD("get_available_packet_count"), &PacketPeerRsss::get_available_packet_count);
  ClassDB::bind_method(D_METHOD("get_encode_buffer_max_size"), &PacketPeerRsss::get_encode_buffer_max_size);
  ClassDB::bind_method(D_METHOD("set_encode_buffer_max_size", "max_size"), &PacketPeerRsss::set_encode_buffer_max_size);

  ClassDB::bind_method(D_METHOD("get_stream_peer"), &PacketPeerRsss::get_stream_peer);
  ClassDB::bind_method(D_METHOD("set_stream_peer", "stream"), &PacketPeerRsss::set_stream_peer);
  ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "stream"), "set_stream_peer", "get_stream_peer");
}

