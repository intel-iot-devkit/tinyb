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

#define PERF_PRINT_ON 1
// #define VERBOSE_ON 1
#include <dbt_debug.hpp>

#include "BTIoctl.hpp"

#include "MgmtTypes.hpp"
#include "HCIIoctl.hpp"
#include "HCIComm.hpp"
#include "DBTTypes.hpp"

extern "C" {
    #include <inttypes.h>
    #include <unistd.h>
}

using namespace direct_bt;

// *************************************************
// *************************************************
// *************************************************

#define CASE_TO_STRING(V) case V: return #V;

#define STATUS_ENUM(X) \
    X(SUCCESS) \
    X(UNKNOWN_COMMAND) \
    X(NOT_CONNECTED) \
    X(FAILED) \
    X(CONNECT_FAILED) \
    X(AUTH_FAILED) \
    X(NOT_PAIRED) \
    X(NO_RESOURCES) \
    X(TIMEOUT) \
    X(ALREADY_CONNECTED) \
    X(BUSY) \
    X(REJECTED) \
    X(NOT_SUPPORTED) \
    X(INVALID_PARAMS) \
    X(DISCONNECTED) \
    X(NOT_POWERED) \
    X(CANCELLED) \
    X(INVALID_INDEX) \
    X(RFKILLED) \
    X(ALREADY_PAIRED) \
    X(PERMISSION_DENIED)

std::string direct_bt::getMgmtStatusString(const MgmtStatus opc) {
    switch(opc) {
        STATUS_ENUM(CASE_TO_STRING)
        default: ; // fall through intended
    }
    return "Unknown Status";
}

// *************************************************
// *************************************************
// *************************************************

#define OPERATION_ENUM(X) \
    X(READ_VERSION) \
    X(READ_COMMANDS) \
    X(READ_INDEX_LIST) \
    X(READ_INFO) \
    X(SET_POWERED) \
    X(SET_DISCOVERABLE) \
    X(SET_CONNECTABLE) \
    X(SET_FAST_CONNECTABLE) \
    X(SET_BONDABLE) \
    X(SET_LINK_SECURITY) \
    X(SET_SSP) \
    X(SET_HS) \
    X(SET_LE) \
    X(SET_DEV_CLASS) \
    X(SET_LOCAL_NAME) \
    X(ADD_UUID) \
    X(REMOVE_UUID) \
    X(LOAD_LINK_KEYS) \
    X(LOAD_LONG_TERM_KEYS) \
    X(DISCONNECT) \
    X(GET_CONNECTIONS) \
    X(PIN_CODE_REPLY) \
    X(PIN_CODE_NEG_REPLY) \
    X(SET_IO_CAPABILITY) \
    X(PAIR_DEVICE) \
    X(CANCEL_PAIR_DEVICE) \
    X(UNPAIR_DEVICE) \
    X(USER_CONFIRM_REPLY) \
    X(USER_CONFIRM_NEG_REPLY) \
    X(USER_PASSKEY_REPLY) \
    X(USER_PASSKEY_NEG_REPLY) \
    X(READ_LOCAL_OOB_DATA) \
    X(ADD_REMOTE_OOB_DATA) \
    X(REMOVE_REMOTE_OOB_DATA) \
    X(START_DISCOVERY) \
    X(STOP_DISCOVERY) \
    X(CONFIRM_NAME) \
    X(BLOCK_DEVICE) \
    X(UNBLOCK_DEVICE) \
    X(SET_DEVICE_ID) \
    X(SET_ADVERTISING) \
    X(SET_BREDR) \
    X(SET_STATIC_ADDRESS) \
    X(SET_SCAN_PARAMS) \
    X(SET_SECURE_CONN) \
    X(SET_DEBUG_KEYS) \
    X(SET_PRIVACY) \
    X(LOAD_IRKS) \
    X(GET_CONN_INFO) \
    X(GET_CLOCK_INFO) \
    X(ADD_DEVICE) \
    X(REMOVE_DEVICE) \
    X(LOAD_CONN_PARAM) \
    X(READ_UNCONF_INDEX_LIST) \
    X(READ_CONFIG_INFO) \
    X(SET_EXTERNAL_CONFIG) \
    X(SET_PUBLIC_ADDRESS) \
    X(START_SERVICE_DISCOVERY) \
    X(READ_LOCAL_OOB_EXT_DATA) \
    X(READ_EXT_INDEX_LIST) \
    X(READ_ADV_FEATURES) \
    X(ADD_ADVERTISING) \
    X(REMOVE_ADVERTISING) \
    X(GET_ADV_SIZE_INFO) \
    X(START_LIMITED_DISCOVERY) \
    X(READ_EXT_INFO) \
    X(SET_APPEARANCE) \
    X(GET_PHY_CONFIGURATION) \
    X(SET_PHY_CONFIGURATION) \
    X(SET_BLOCKED_KEYS)

std::string direct_bt::getMgmtOpcodeString(const MgmtOpcode op) {
    switch(op) {
        OPERATION_ENUM(CASE_TO_STRING)
        default: ; // fall through intended
    }
    return "Unknown Operation";
}

// *************************************************
// *************************************************
// *************************************************

#define OPCODE_ENUM(X) \
    X(CMD_COMPLETE) \
    X(CMD_STATUS) \
    X(CONTROLLER_ERROR) \
    X(INDEX_ADDED) \
    X(INDEX_REMOVED) \
    X(NEW_SETTINGS) \
    X(CLASS_OF_DEV_CHANGED) \
    X(LOCAL_NAME_CHANGED) \
    X(NEW_LINK_KEY) \
    X(NEW_LONG_TERM_KEY) \
    X(DEVICE_CONNECTED) \
    X(DEVICE_DISCONNECTED) \
    X(CONNECT_FAILED) \
    X(PIN_CODE_REQUEST) \
    X(USER_CONFIRM_REQUEST) \
    X(USER_PASSKEY_REQUEST) \
    X(AUTH_FAILED) \
    X(DEVICE_FOUND) \
    X(DISCOVERING) \
    X(DEVICE_BLOCKED) \
    X(DEVICE_UNBLOCKED) \
    X(DEVICE_UNPAIRED) \
    X(PASSKEY_NOTIFY) \
    X(NEW_IRK) \
    X(NEW_CSRK) \
    X(DEVICE_ADDED) \
    X(DEVICE_REMOVED) \
    X(NEW_CONN_PARAM) \
    X(UNCONF_INDEX_ADDED) \
    X(UNCONF_INDEX_REMOVED) \
    X(NEW_CONFIG_OPTIONS) \
    X(EXT_INDEX_ADDED) \
    X(EXT_INDEX_REMOVED) \
    X(LOCAL_OOB_DATA_UPDATED) \
    X(ADVERTISING_ADDED) \
    X(ADVERTISING_REMOVED) \
    X(EXT_INFO_CHANGED) \
    X(PHY_CONFIGURATION_CHANGED)

std::string MgmtEvent::getOpcodeString(const Opcode opc) {
    switch(opc) {
        OPCODE_ENUM(CASE_TO_STRING)
        default: ; // fall through intended
    }
    return "Unknown Opcode";
}

MgmtEvent* MgmtEvent::getSpecialized(const uint8_t * buffer, int const buffer_size) {
    const uint8_t opc = *buffer;
    switch( opc ) {
        case MgmtEvent::Opcode::CMD_COMPLETE:
            if( buffer_size >= MgmtEvtAdapterInfo::getRequiredSize() ) {
                const MgmtOpcode opc = MgmtEvtCmdComplete::getReqOpcode(buffer);
                if( MgmtOpcode::READ_INFO == opc ) {
                    return new MgmtEvtAdapterInfo(buffer, buffer_size);
                }
            }
            return new MgmtEvtCmdComplete(buffer, buffer_size);
        case MgmtEvent::Opcode::CMD_STATUS:
            return new MgmtEvtCmdStatus(buffer, buffer_size);
        case MgmtEvent::Opcode::DISCOVERING:
            return new MgmtEvtDiscovering(buffer, buffer_size);
        case MgmtEvent::Opcode::DEVICE_FOUND:
            return new MgmtEvtDeviceFound(buffer, buffer_size);
        case MgmtEvent::Opcode::DEVICE_CONNECTED:
            return new MgmtEvtDeviceConnected(buffer, buffer_size);
        case MgmtEvent::Opcode::DEVICE_DISCONNECTED:
            return new MgmtEvtDeviceDisconnected(buffer, buffer_size);
        default:
            return new MgmtEvent(buffer, buffer_size);
    }
}
