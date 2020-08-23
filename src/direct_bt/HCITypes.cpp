/*
 * Author: Sven Gothel <sgothel@jausoft.com>
 * Copyright (c) 2020 Gothel Software e.K.
 * Copyright (c) 2020 ZAFENA AB
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>
#include <cstdio>

#include <algorithm>

// #define VERBOSE_ON 1
#include <dbt_debug.hpp>

#include "HCITypes.hpp"

extern "C" {
    #include <inttypes.h>
    #include <unistd.h>
    #include <sys/param.h>
    #include <sys/uio.h>
    #include <sys/types.h>
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <poll.h>
}

namespace direct_bt {

#define HCI_STATUS_CODE(X) \
        X(SUCCESS) \
        X(UNKNOWN_HCI_COMMAND) \
        X(UNKNOWN_CONNECTION_IDENTIFIER) \
        X(HARDWARE_FAILURE) \
        X(PAGE_TIMEOUT) \
        X(AUTHENTICATION_FAILURE) \
        X(PIN_OR_KEY_MISSING) \
        X(MEMORY_CAPACITY_EXCEEDED) \
        X(CONNECTION_TIMEOUT) \
        X(CONNECTION_LIMIT_EXCEEDED) \
        X(SYNC_DEVICE_CONNECTION_LIMIT_EXCEEDED) \
        X(CONNECTION_ALREADY_EXISTS) \
        X(COMMAND_DISALLOWED) \
        X(CONNECTION_REJECTED_LIMITED_RESOURCES) \
        X(CONNECTION_REJECTED_SECURITY) \
        X(CONNECTION_REJECTED_UNACCEPTABLE_BD_ADDR) \
        X(CONNECTION_ACCEPT_TIMEOUT_EXCEEDED) \
        X(UNSUPPORTED_FEATURE_OR_PARAM_VALUE) \
        X(INVALID_HCI_COMMAND_PARAMETERS) \
        X(REMOTE_USER_TERMINATED_CONNECTION) \
        X(REMOTE_DEVICE_TERMINATED_CONNECTION_LOW_RESOURCES) \
        X(REMOTE_DEVICE_TERMINATED_CONNECTION_POWER_OFF) \
        X(CONNECTION_TERMINATED_BY_LOCAL_HOST) \
        X(REPEATED_ATTEMPTS) \
        X(PAIRING_NOT_ALLOWED) \
        X(UNKNOWN_LMP_PDU) \
        X(UNSUPPORTED_REMOTE_OR_LMP_FEATURE) \
        X(SCO_OFFSET_REJECTED) \
        X(SCO_INTERVAL_REJECTED) \
        X(SCO_AIR_MODE_REJECTED) \
        X(INVALID_LMP_OR_LL_PARAMETERS) \
        X(UNSPECIFIED_ERROR) \
        X(UNSUPPORTED_LMP_OR_LL_PARAMETER_VALUE) \
        X(ROLE_CHANGE_NOT_ALLOWED) \
        X(LMP_OR_LL_RESPONSE_TIMEOUT) \
        X(LMP_OR_LL_COLLISION) \
        X(LMP_PDU_NOT_ALLOWED) \
        X(ENCRYPTION_MODE_NOT_ACCEPTED) \
        X(LINK_KEY_CANNOT_BE_CHANGED) \
        X(REQUESTED_QOS_NOT_SUPPORTED) \
        X(INSTANT_PASSED) \
        X(PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED) \
        X(DIFFERENT_TRANSACTION_COLLISION) \
        X(QOS_UNACCEPTABLE_PARAMETER) \
        X(QOS_REJECTED) \
        X(CHANNEL_ASSESSMENT_NOT_SUPPORTED) \
        X(INSUFFICIENT_SECURITY) \
        X(PARAMETER_OUT_OF_RANGE) \
        X(ROLE_SWITCH_PENDING) \
        X(RESERVED_SLOT_VIOLATION) \
        X(ROLE_SWITCH_FAILED) \
        X(EIR_TOO_LARGE) \
        X(SIMPLE_PAIRING_NOT_SUPPORTED_BY_HOST) \
        X(HOST_BUSY_PAIRING) \
        X(CONNECTION_REJECTED_NO_SUITABLE_CHANNEL) \
        X(CONTROLLER_BUSY) \
        X(UNACCEPTABLE_CONNECTION_PARAM) \
        X(ADVERTISING_TIMEOUT) \
        X(CONNECTION_TERMINATED_MIC_FAILURE) \
        X(CONNECTION_EST_FAILED_OR_SYNC_TIMEOUT) \
        X(MAX_CONNECTION_FAILED) \
        X(COARSE_CLOCK_ADJ_REJECTED) \
        X(TYPE0_SUBMAP_NOT_DEFINED) \
        X(UNKNOWN_ADVERTISING_IDENTIFIER) \
        X(LIMIT_REACHED) \
        X(OPERATION_CANCELLED_BY_HOST) \
        X(PACKET_TOO_LONG) \
        X(INTERNAL_TIMEOUT) \
        X(INTERNAL_FAILURE) \
        X(UNKNOWN)

#define HCI_STATUS_CODE_CASE_TO_STRING(V) case HCIStatusCode::V: return #V;

std::string getHCIStatusCodeString(const HCIStatusCode ec) {
    switch(ec) {
    HCI_STATUS_CODE(HCI_STATUS_CODE_CASE_TO_STRING)
        default: ; // fall through intended
    }
    return "Unknown HCIStatusCode";
}

std::string getHCIPacketTypeString(const HCIPacketType op) {
    switch(op) {
        case HCIPacketType::COMMAND: return "COMMAND";
        case HCIPacketType::ACLDATA: return "ACLDATA";
        case HCIPacketType::SCODATA: return "SCODATA";
        case HCIPacketType::EVENT: return "EVENT";
        case HCIPacketType::DIAG: return "DIAG";
        case HCIPacketType::VENDOR: return "VENDOR";
    }
    return "Unknown HCIPacketType";
}

std::string getHCIOGFString(const HCIOGF op) {
    (void)op;
    return "";
}

#define HCI_OPCODE(X) \
    X(SPECIAL) \
    X(CREATE_CONN) \
    X(DISCONNECT) \
    X(SET_EVENT_MASK) \
    X(RESET) \
    X(READ_LOCAL_VERSION) \
    X(LE_SET_EVENT_MASK) \
    X(LE_READ_BUFFER_SIZE) \
    X(LE_READ_LOCAL_FEATURES) \
    X(LE_SET_RANDOM_ADDR) \
    X(LE_SET_ADV_PARAM) \
    X(LE_READ_ADV_TX_POWER) \
    X(LE_SET_ADV_DATA) \
    X(LE_SET_SCAN_RSP_DATA) \
    X(LE_SET_ADV_ENABLE) \
    X(LE_SET_SCAN_PARAM) \
    X(LE_SET_SCAN_ENABLE) \
    X(LE_CREATE_CONN) \
    X(LE_CREATE_CONN_CANCEL) \
    X(LE_READ_WHITE_LIST_SIZE) \
    X(LE_CLEAR_WHITE_LIST) \
    X(LE_ADD_TO_WHITE_LIST) \
    X(LE_DEL_FROM_WHITE_LIST) \
    X(LE_CONN_UPDATE) \
    X(LE_READ_REMOTE_FEATURES) \
    X(LE_START_ENC)

#define HCI_OPCODE_CASE_TO_STRING(V) case HCIOpcode::V: return #V;

std::string getHCIOpcodeString(const HCIOpcode op) {
    switch(op) {
    HCI_OPCODE(HCI_OPCODE_CASE_TO_STRING)
        default: ; // fall through intended
    }
    return "Unknown HCIOpcode";
}

#define HCI_EVENTTYPE(X) \
    X(INVALID) \
    X(INQUIRY_COMPLETE) \
    X(INQUIRY_RESULT) \
    X(CONN_COMPLETE) \
    X(CONN_REQUEST) \
    X(DISCONN_COMPLETE) \
    X(AUTH_COMPLETE) \
    X(REMOTE_NAME) \
    X(ENCRYPT_CHANGE) \
    X(CHANGE_LINK_KEY_COMPLETE) \
    X(REMOTE_FEATURES) \
    X(REMOTE_VERSION) \
    X(QOS_SETUP_COMPLETE) \
    X(CMD_COMPLETE) \
    X(CMD_STATUS) \
    X(HARDWARE_ERROR) \
    X(ROLE_CHANGE) \
    X(NUM_COMP_PKTS) \
    X(MODE_CHANGE) \
    X(PIN_CODE_REQ) \
    X(LINK_KEY_REQ) \
    X(LINK_KEY_NOTIFY) \
    X(CLOCK_OFFSET) \
    X(PKT_TYPE_CHANGE) \
    X(DISCONN_PHY_LINK_COMPLETE) \
    X(DISCONN_LOGICAL_LINK_COMPLETE) \
    X(LE_META)

#define HCI_EVENTTYPE_CASE_TO_STRING(V) case HCIEventType::V: return #V;

std::string getHCIEventTypeString(const HCIEventType op) {
        switch(op) {
        HCI_EVENTTYPE(HCI_EVENTTYPE_CASE_TO_STRING)
            default: ; // fall through intended
        }
        return "Unknown HCIEventType";
}

#define HCI_METATYPE(X) \
    X(INVALID) \
    X(LE_CONN_COMPLETE) \
    X(LE_ADVERTISING_REPORT) \
    X(LE_CONN_UPDATE_COMPLETE) \
    X(LE_REMOTE_FEAT_COMPLETE) \
    X(LE_LTKEY_REQUEST) \
    X(LE_REMOTE_CONN_PARAM_REQ) \
    X(LE_DATA_LENGTH_CHANGE) \
    X(LE_READ_LOCAL_P256_PUBKEY_COMPLETE) \
    X(LE_GENERATE_DHKEY_COMPLETE) \
    X(LE_ENHANCED_CONN_COMPLETE) \
    X(LE_DIRECT_ADV_REPORT) \
    X(LE_PHY_UPDATE_COMPLETE) \
    X(LE_EXT_ADV_REPORT) \
    X(LE_PERIODIC_ADV_SYNC_ESTABLISHED) \
    X(LE_PERIODIC_ADV_REPORT) \
    X(LE_PERIODIC_ADV_SYNC_LOST) \
    X(LE_SCAN_TIMEOUT) \
    X(LE_ADV_SET_TERMINATED) \
    X(LE_SCAN_REQ_RECEIVED) \
    X(LE_CHANNEL_SEL_ALGO) \
    X(LE_CONNLESS_IQ_REPORT) \
    X(LE_CONN_IQ_REPORT) \
    X(LE_CTE_REQ_FAILED) \
    X(LE_PERIODIC_ADV_SYNC_TRANSFER_RECV) \
    X(LE_CIS_ESTABLISHED) \
    X(LE_CIS_REQUEST) \
    X(LE_CREATE_BIG_COMPLETE) \
    X(LE_TERMINATE_BIG_COMPLETE) \
    X(LE_BIG_SYNC_ESTABLISHED) \
    X(LE_BIG_SYNC_LOST) \
    X(LE_REQUEST_PEER_SCA_COMPLETE) \
    X(LE_PATH_LOSS_THRESHOLD) \
    X(LE_TRANSMIT_POWER_REPORTING) \
    X(LE_BIGINFO_ADV_REPORT)

#define HCI_METATYPE_CASE_TO_STRING(V) case HCIMetaEventType::V: return #V;

std::string getHCIMetaEventTypeString(const HCIMetaEventType op) {
    switch(op) {
    HCI_METATYPE(HCI_METATYPE_CASE_TO_STRING)
        default: ; // fall through intended
    }
    return "Unknown HCIMetaType";
}

HCIEvent* HCIEvent::getSpecialized(const uint8_t * buffer, int const buffer_size) {
    const HCIPacketType pc = static_cast<HCIPacketType>( get_uint8(buffer, 0) );
    if( HCIPacketType::EVENT != pc ) {
        return nullptr;
    }
    const HCIEventType ec = static_cast<HCIEventType>( get_uint8(buffer, 1) );

    switch( ec ) {
        case HCIEventType::DISCONN_COMPLETE:
            return new HCIDisconnectionCompleteEvent(buffer, buffer_size);
        case HCIEventType::CMD_COMPLETE:
            return new HCICommandCompleteEvent(buffer, buffer_size);
        case HCIEventType::CMD_STATUS:
            return new HCICommandStatusEvent(buffer, buffer_size);
        case HCIEventType::LE_META:
            // No need to HCIMetaType specializations as we use HCIStructCmdCompleteMetaEvt template
            // based on HCIMetaEvent.
            return new HCIMetaEvent(buffer, buffer_size);
        default:
            // No further specialization, use HCIStructCmdCompleteEvt template
            return new HCIEvent(buffer, buffer_size);
    }
}

} /* namespace direct_bt */
