// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#include "ControlTransaction.h"

ControlTransaction::ControlTransaction(uint16_t tr_id) {
    m_tr_id = tr_id;
    m_tr_state = ControlTransactionState::CTR_INIT;
    m_tr_sent_stamp = 0;
}

void ControlTransaction::set_transaction_sent() {
    m_tr_state = ControlTransactionState::CTR_SENT;
    m_tr_sent_stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

void ControlTransaction::ack_transaction() {
    m_tr_state = ControlTransactionState::CTR_ACK;
}

void ControlTransaction::update_transaction() {

}

ControlTransactionState ControlTransaction::get_transaction_state() const {
    return m_tr_state;
}

uint16_t ControlTransaction::get_transaction_id() const {
    return m_tr_id;
}
