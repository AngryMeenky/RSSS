#include "PacketPeerRsss.h"


PacketPeerRsss::PacketPeerRsss():
  parser(),
  buffer(new uint8_t[parser.maximumSync()]),
  go(false) {
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


Ref<PacketPeerRsss> PacketPeerRsss::wrap(const Ref<StreamPeer> &stream) {
  Ref<PacketPeerRsss> rsss;

  rsss.instantiate();
  if(rsss.is_valid() && !rsss->initialize(stream)) {
    rsss.unref();
  }

  return rsss;
}


bool PacketPeerRsss::initialize(const Ref<StreamPeer> &stream) {
  if(!go && stream.is_valid() && parser.initialize(stream)) {
    go = true;
    worker_in  = std::thread(&PacketPeerRsss::readPackets,  this);
    worker_out = std::thread(&PacketPeerRsss::writePackets, this);
    return true;
  }

  return false;
}


int32_t PacketPeerRsss::_get_max_packet_size() const {
  return parser.maximumSync();
}


int32_t PacketPeerRsss::_get_available_packet_count() const {
  std::unique_lock<std::mutex> guard(const_cast<PacketPeerRsss *>(this)->mutex_in);
  return static_cast<int64_t>(packets_in.size());
}


Error PacketPeerRsss::_get_packet(const uint8_t **r_buffer, int32_t* r_buffer_size) {
  std::unique_lock<std::mutex> guard(mutex_in);
  if(packets_in.empty()) {
    return ERR_UNAVAILABLE;
  }

  auto &packet = packets_in.front();
  memcpy(&buffer[0], &packet.data[0], packet.size);
  *r_buffer_size = packet.size;
  *r_buffer = &buffer[0];
  packets_in.pop_front();
  return OK;
}


Error PacketPeerRsss::_put_packet(const uint8_t *p_buffer, int p_buffer_size) {
  if(!p_buffer || p_buffer_size <= 0) {
    return ERR_PARAMETER_RANGE_ERROR;
  }

  std::unique_lock<std::mutex> guard(mutex_out);
  packets_out.emplace_back(std::make_unique<uint8_t[]>(p_buffer_size), p_buffer_size);
  memcpy(&packets_out.back().data[0], p_buffer, p_buffer_size);
  cv_out.notify_one();
  return OK;
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


void PacketPeerRsss::_bind_methods() {
  ClassDB::bind_static_method("PacketPeerRsss", D_METHOD("wrap", "stream"), &PacketPeerRsss::wrap);
}

