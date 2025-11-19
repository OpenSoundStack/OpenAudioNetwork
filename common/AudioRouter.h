// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2025 - Mathis DELGADO
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, version 3 of the License.
//
// This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

#ifndef AUDIOROUTER_H
#define AUDIOROUTER_H

#include "netutils/LowLatSocket.h"
#include "packet_structs.h"

#include <functional>
#include <queue>
#include <mutex>
#include <shared_mutex>

class AudioRouter {
public:
    AudioRouter(uint16_t self_uid);
    virtual ~AudioRouter() = default;

    bool init_router(const std::string& eth_interface, const std::shared_ptr<NetworkMapper>& nmapper);

    void poll_audio_data(bool async);
    void poll_local_audio_buffer();
    void poll_control_packets(bool async = true);

    void send_audio_packet(const AudioPacket &packet, uint16_t dest_uid);

    template<class T>
    void send_control_packet(const T& pck, uint16_t dest_uid) {
        m_control_iface->send_data(pck, dest_uid);
    }

    void set_routing_callback(const std::function<void(AudioPacket&, LowLatHeader&)> &callback);
    void set_control_callback(const std::function<void(ControlPacket&, LowLatHeader&)>& callback);
    void set_control_response_callback(const std::function<void(ControlResponsePacket&, LowLatHeader&)>& callback);
    void set_pipe_create_callback(const std::function<void(ControlPipeCreatePacket&, LowLatHeader&)>& callback);
    void set_control_query_callback(const std::function<void(ControlQueryPacket&, LowLatHeader&)>& callback);
private:
    std::unique_ptr<LowLatSocket> m_audio_iface;
    std::unique_ptr<LowLatSocket> m_control_iface;
    uint16_t m_self_uid;

    std::queue<AudioPacket> m_local_audio_fifo;

#ifndef NO_THREADS
    std::shared_mutex m_local_fifo_mutex;
#endif // NO_THREADS
protected:
    std::function<void(AudioPacket&, LowLatHeader&)> m_routing_callback;
    std::function<void(ControlPacket&, LowLatHeader&)> m_channel_control_callback;
    std::function<void(ControlPipeCreatePacket&, LowLatHeader&)> m_pipe_create_callback;
    std::function<void(ControlResponsePacket&, LowLatHeader&)> m_control_response_callback;
    std::function<void(ControlQueryPacket&, LowLatHeader&)> m_control_query_callback;
};



#endif //AUDIOROUTER_H
