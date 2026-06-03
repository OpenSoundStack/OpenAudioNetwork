// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#include "RawMacTransport.h"

#include <iostream>

bool RawMacTransport::open(const std::string& iface, EthProtocol proto,
                           uint16_t self_uid, IfaceMeta& out_meta) {
    (void)iface; (void)proto; (void)self_uid; (void)out_meta;
    std::cerr << "RawMacTransport: BPF not implemented (Track 2)" << std::endl;
    return false;
}

int RawMacTransport::send(const uint8_t* data, size_t len, uint16_t dest_uid) {
    (void)data; (void)len; (void)dest_uid;
    return -1;
}

int RawMacTransport::recv(uint8_t* data, size_t len, bool async) {
    (void)data; (void)len; (void)async;
    return -1;
}

void RawMacTransport::close() {}
