#include <iostream>
#include <string>
#include <fstream>
#include <optional>

#include <jsoncpp/json/json.h>

#include "peer_conf.h"
#include "common/NetworkMapper.h"

#include "common/AudioPipe.h"
#include "common/base_pipes/AudioInPipe.h"
#include "common/base_pipes/AudioPortalPipe.h"

std::optional<PeerConf> config_from_json(const std::string& path) {
    std::ifstream f{path};
    if(!f.is_open()) {
        std::cerr << "Failed to open " << path << std::endl;
        return {};
    }

    Json::Value root;
    Json::String err;

    Json::CharReaderBuilder builder{};
    if(!parseFromStream(builder, f, &root, &err)) {
        f.close();

        std::cerr << "Failed to read local config. Err : " << err << std::endl;
        return {};
    }

    f.close();

    Json::Value ident = root["ident"];
    Json::Value networking = root["networking"];
    Json::Value topo = root["topology"];

    PeerConf pconf{};
    pconf.dev_type = static_cast<DeviceType>(ident.get("type", 0).asInt());
    pconf.uid = ident.get("uid", 0).asInt();

    pconf.topo.phy_in_count = topo.get("phy_in", 0).asInt();
    pconf.topo.phy_out_count = topo.get("phy_out", 0).asInt();
    pconf.topo.pipes_count = topo.get("pipes", 0).asInt();

    int srate = ident.get("sampling_rate", 96).asInt();
    if(srate == 96) {
        pconf.sample_rate = SamplingRate::SAMPLING_96K;
    } else {
        pconf.sample_rate = SamplingRate::SAMPLING_48K;
    }

    std::string dev_name = ident["name"].asString();
    strcpy(pconf.dev_name, dev_name.data());

    pconf.mapping_port = root.get("mapping_port", 11000).asInt();
    pconf.audio_port = root.get("audio_port", 8000).asInt();
    pconf.address = ip(
            networking["self_ip"][0].asInt(),
            networking["self_ip"][1].asInt(),
            networking["self_ip"][2].asInt(),
            networking["self_ip"][3].asInt()
    );

    pconf.netmask = ip(
            networking["netmask"][0].asInt(),
            networking["netmask"][1].asInt(),
            networking["netmask"][2].asInt(),
            networking["netmask"][3].asInt()
    );

    return pconf;
}

PeerConf get_default_conf() {
    const char *name = "Unknown";

    PeerConf conf{};
    conf.netmask = 0x000000FF;
    conf.address = ip(10, 0, 0, 1);
    conf.mapping_port = 11000;
    memcpy(&conf.dev_name, name, strlen(name));
    conf.sample_rate = SamplingRate::SAMPLING_48K;
    conf.dev_type = DeviceType::CONTROL_SURFACE;
    conf.uid = 1;

    return conf;
}

int main(int argc, char* argv[]) {
    std::cout << "OpenAudioNetwork Peer client-server" << std::endl;

    PeerConf conf{};
    if(argc >= 2) {
        std::string conf_path(argv[1]);
        conf_path += "/conf.json";

        auto read_conf = config_from_json(conf_path);
        if(read_conf.has_value()) {
            conf = read_conf.value();
        } else {
            conf = get_default_conf();
        }
    } else {
        std::cerr << "No specified config, using default" << std::endl;

        conf = get_default_conf();
    }

    // Init auto-discover mechanism
    NetworkMapper nmapper{conf};
    if(nmapper.init_mapper()) {
        nmapper.launch_mapping_process();
    } else {
        std::cerr << "Failed to init mapper" << std::endl;
        exit(-1);
    }

    // AUDIO PIPES TEST
    std::shared_ptr<UDPSocket> audio_socket = std::make_shared<UDPSocket>();
    audio_socket->init_socket(INADDR_ANY, conf.audio_port);
    audio_socket->set_high_prio();

    std::unique_ptr<AudioInPipe> pipe = std::make_unique<AudioInPipe>();
    auto portal = std::make_unique<AudioPortalPipe>(
        1, ip(127, 0, 0, 1), conf.audio_port, audio_socket
    );

    pipe->set_next_pipe(std::move(portal));
    pipe->set_gain_lin(100);

    uint64_t time_delta = 0;
    while(true) {
        uint64_t now = NetworkMapper::local_now();
        pipe->acquire_sample((float)(0xFFFFFFFF));

        time_delta = (NetworkMapper::local_now() - now) / 1000;
        usleep(600 - time_delta);
    }

    return 0;
}