// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#ifndef CONTROL_TRANSACTION_H
#define CONTROL_TRANSACTION_H

#include <cstdint>
#include <chrono>
#include <cstring>
#include <vector>

enum class ControlTransactionState {
    CTR_INIT,
    CTR_SENT,
    CTR_ACK,
    CTR_TIMEOUT
};

class ControlTransaction {
public:
    ControlTransaction(uint16_t tr_id);
    ~ControlTransaction() = default;

    template<class T>
    void set_transaction_payload(const T& data) {
        m_payload.resize(sizeof(data));
        memcpy(m_payload.data(), (void*)&data, sizeof(T));
    }

    void set_transaction_sent();
    void update_transaction();
    void ack_transaction();

    ControlTransactionState get_transaction_state();
private:
    uint16_t m_tr_id;
    ControlTransactionState m_tr_state;
    uint64_t m_tr_sent_stamp;

    std::vector<uint8_t> m_payload;
};

#endif //CONTROL_TRANSACTION_H
