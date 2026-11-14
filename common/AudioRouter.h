// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#ifndef AUDIOROUTER_H
#define AUDIOROUTER_H

#include "third_party/concurrentqueue.h"
#include "netutils/LowLatSocket.h"
#include "packet_structs.h"

#include <functional>
#include <optional>

struct OutboundAudioRoute {
    uint8_t source_channel;
    uint16_t destination_uid;
};

struct InboundAudioRoute {
    uint8_t source_channel;
    uint16_t source_uid;
    uint8_t local_channel;
};

struct RouteNode {
    uint8_t channel;
    uint16_t uid;

    inline bool operator==(const RouteNode& other) const {
        return channel == other.channel && uid == other.uid;
    }
};

class AudioRouter {
public:
    AudioRouter(uint16_t self_uid);
    virtual ~AudioRouter() = default;

    bool init_router(const std::string& eth_interface, const std::shared_ptr<NetworkMapper>& nmapper);

    void poll_audio_data(bool async);
    void poll_local_audio_buffer();
    void poll_control_packets(bool async = true);

    void send_audio_packet(const AudioPacket &packet, uint16_t dest_uid);
    void route_audio_packet(AudioPacket &packet);

    template<class T>
    void send_control_packet(const T& pck, uint16_t dest_uid) {
        m_control_iface->send_data(pck, dest_uid);
    }

    void set_routing_callback(const std::function<void(AudioPacket&, LowLatHeader&, bool)> &callback);
    void set_control_callback(const std::function<void(ControlPacket&, LowLatHeader&)>& callback);
    void set_control_response_callback(const std::function<void(ControlResponsePacket&, LowLatHeader&)>& callback);
    void set_pipe_create_callback(const std::function<void(ControlPipeCreatePacket&, LowLatHeader&)>& callback);
    void set_control_query_callback(const std::function<void(ControlQueryPacket&, LowLatHeader&)>& callback);

    void create_inbound_route(uint8_t source_channel, uint16_t source_uid, uint8_t local_channel);
    void create_outbound_route(uint8_t source_channel, uint16_t destination_uid);
    void create_global_route(RouteNode& source, RouteNode& destination);

    void clear_all_routes();

    std::optional<OutboundAudioRoute> get_outbound_route(uint8_t source_channel);
    std::optional<uint8_t> get_channel_inbound_route(uint8_t source_channel, uint16_t source_uid);
    std::optional<RouteNode> get_inbound_node_from_local_channel(uint8_t local_channel);
private:
    std::unique_ptr<LowLatSocket> m_audio_iface;
    std::unique_ptr<LowLatSocket> m_control_iface;
    uint16_t m_self_uid;

    moodycamel::ConcurrentQueue<AudioPacket> m_local_audio_fifo;

    std::shared_ptr<NetworkMapper> m_nmapper;
    std::vector<OutboundAudioRoute> m_outbound_routes;
    std::vector<InboundAudioRoute> m_inbound_routes;

    uint16_t m_transaction_tracker;
protected:
    std::function<void(AudioPacket&, LowLatHeader&, bool)> m_routing_callback;
    std::function<void(ControlPacket&, LowLatHeader&)> m_channel_control_callback;
    std::function<void(ControlPipeCreatePacket&, LowLatHeader&)> m_pipe_create_callback;
    std::function<void(ControlResponsePacket&, LowLatHeader&)> m_control_response_callback;
    std::function<void(ControlQueryPacket&, LowLatHeader&)> m_control_query_callback;
};



#endif //AUDIOROUTER_H
