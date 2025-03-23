#include <iostream>
#include <string>
#include <fstream>
#include <optional>
#include <cmath>

#include <jsoncpp/json/json.h>

#include "peer_conf.h"
#include "common/NetworkMapper.h"

#include "common/AudioPipe.h"
#include "common/base_pipes/AudioInPipe.h"
#include "common/base_pipes/AudioPortalPipe.h"

#include "netutils/LowLatSocket.h"

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

    pconf.iface = networking["audio_if"].asString();

    return pconf;
}

PeerConf get_default_conf() {
    const char *name = "Unknown";

    PeerConf conf{};
    conf.iface = "lo";
    memcpy(&conf.dev_name, name, strlen(name));
    conf.sample_rate = SamplingRate::SAMPLING_96K;
    conf.dev_type = DeviceType::CONTROL_SURFACE;
    conf.uid = 1;

    return conf;
}

uint64_t local_now_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

float sig_gen(float f, int sig_level) {
    constexpr float T = 1.0f / 96000.0f;
    float pulse = 2.0f * 3.141592 * f;

    static int n = 0;


    float sample = (float)sin(pulse * n * T) * (float)(1 << sig_level);

    n++;
    return sample;
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

    std::cout << "Initializing on " << conf.iface << std::endl;
    if(nmapper.init_mapper(conf.iface)) {
        nmapper.launch_mapping_process();
    } else {
        std::cerr << "Failed to init mapper" << std::endl;
        exit(-1);
    }

    // AUDIO PIPES TEST
    std::shared_ptr<LowLatSocket> audio_socket = std::make_shared<LowLatSocket>(conf.uid);
    if (!audio_socket->init_socket("enp1s0", EthProtocol::ETH_PROTO_OANAUDIO)) {
        std::cerr << "Failed ll socket init" << std::endl;
    }

    std::unique_ptr<AudioInPipe> pipe = std::make_unique<AudioInPipe>();
    auto portal = std::make_unique<AudioPortalPipe>(
        1, 1, audio_socket
    );

    pipe->set_next_pipe(std::move(portal));
    pipe->set_gain_lin(100);

    uint64_t last = local_now_us();
    while(true) {
        if (local_now_us() - last >= 10) {
            // 1kHz gen at 0dB (24 bits)
            //pipe->acquire_sample(sig_gen(1000.0f, 24));
            last = local_now_us();
        }
    }

    return 0;
}