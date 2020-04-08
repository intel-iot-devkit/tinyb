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

#include "BTIoctl.hpp"

#include "MgmtComm.hpp"
#include "HCIIoctl.hpp"
#include "HCIComm.hpp"
#include "HCITypes.hpp"

extern "C" {
    #include <inttypes.h>
    #include <unistd.h>
    #include <poll.h>
}

#include "dbt_debug.hpp"

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

std::string direct_bt::mgmt_get_status_string(const MgmtStatus opc) {
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

std::string direct_bt::mgmt_get_operation_string(const MgmtOperation op) {
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
        case CMD_COMPLETE:
            if( buffer_size >= MgmtEvtAdapterInfo::getRequiredSize() ) {
                const MgmtRequest::Opcode opc = MgmtEvtCmdComplete::getReqOpcode(buffer);
                if( MgmtRequest::Opcode::READ_INFO == opc ) {
                    return new MgmtEvtAdapterInfo(buffer, buffer_size);
                }
            }
            return new MgmtEvtCmdComplete(buffer, buffer_size);
        case CMD_STATUS:
            return new MgmtEvtCmdStatus(buffer, buffer_size);
        default:
            return new MgmtEvent(buffer, buffer_size);
    }
}

// *************************************************
// *************************************************
// *************************************************

#undef OPCODE_ENUM
#define OPCODE_ENUM(X) \
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
    X(SET_LOCAL_NAME)

std::string MgmtRequest::getOpcodeString(const Opcode opc) {
    switch(opc) {
        OPCODE_ENUM(CASE_TO_STRING)
        default: ; // fall through intended
    }
    return "Unknown Opcode";
}

int MgmtRequest::read(const int dd, uint8_t* buffer, const int capacity, const int timeoutMS) {
    int len = 0;
    if( 0 > dd || 0 > capacity ) {
        goto errout;
    }
    if( 0 == capacity ) {
        goto done;
    }

    if( timeoutMS ) {
        struct pollfd p;
        int n;

        p.fd = dd; p.events = POLLIN;
        while ((n = poll(&p, 1, timeoutMS)) < 0) {
            if (errno == EAGAIN || errno == EINTR ) {
                // cont temp unavail or interruption
                continue;
            }
            goto errout;
        }
        if (!n) {
            errno = ETIMEDOUT;
            goto errout;
        }
    }

    while ((len = ::read(dd, buffer, capacity)) < 0) {
        if (errno == EAGAIN || errno == EINTR ) {
            // cont temp unavail or interruption
            continue;
        }
        goto errout;
    }

done:
    return len;

errout:
    return -1;
}

int MgmtRequest::write(const int dd) {
    int len = 0;
    if( 0 > dd || 0 > pdu.getSize() ) {
        goto errout;
    }
    if( 0 == pdu.getSize() ) {
        goto done;
    }

    while( ( len = ::write(dd, pdu.get_ptr(), pdu.getSize()) ) < 0 ) {
        if( EAGAIN == errno || EINTR == errno ) {
            continue;
        }
        goto errout;
    }

done:
    return len;

errout:
    return -1;
}

int MgmtRequest::send(const int dd, uint8_t* buffer, const int capacity, const int timeoutMS)
{
    int len;

    if ( write(dd) < 0 ) {
        perror("MgmtRequest::write: error");
        goto failed;
    }

    if( ( len = read(dd, buffer, capacity, timeoutMS) ) < 0 ) {
        perror("MgmtRequest::read: error");
        goto failed;
    }
    return len;

failed:
    return -1;
}

// *************************************************
// *************************************************
// *************************************************

std::shared_ptr<MgmtEvent> MgmtHandler::send(MgmtRequest &req, uint8_t* buffer, const int capacity, const int timeoutMS) {
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor

    int len = req.send(comm.dd(), buffer, capacity, timeoutMS);
    if( 0 >= len ) {
        return nullptr;
    }
    const uint16_t paramSize = len >= 6 ? get_uint16(buffer, 4, true /* littleEndian */) : 0;
    if( len < 6 + paramSize ) {
        WARN_PRINT("mgmt_send_cmd: length mismatch %d < 6 + %d; req %s", len, paramSize, req.toString().c_str());
        return nullptr;
    }
    MgmtEvent * res = MgmtEvent::getSpecialized(buffer, len);
    if( !res->validate(req) ) {
        WARN_PRINT("mgmt_send_cmd: res mismatch: res %s; req %s", res->toString().c_str(), req.toString().c_str());
        delete res;
        return nullptr;
    }
    return std::shared_ptr<MgmtEvent>(res);
}

bool MgmtHandler::initAdapter(const uint16_t dev_id) {
    bool ok = true;
    std::shared_ptr<const AdapterInfo> adapter;
    MgmtRequest req0(MgmtModeReq::READ_INFO, dev_id);
    DBG_PRINT("MgmtHandler::initAdapter %d: req: %s", dev_id, req0.toString().c_str());
    {
        std::shared_ptr<MgmtEvent> res = send(req0, ibuffer, ibuffer_size, HCI_TO_SEND_REQ_POLL_MS);
        if( nullptr == res ) {
            goto fail;
        }
        DBG_PRINT("MgmtHandler::initAdapter %d: res: %s", dev_id, res->toString().c_str());
        if( MgmtEvent::Opcode::CMD_COMPLETE != res->getOpcode() || res->getTotalSize() < MgmtEvtAdapterInfo::getRequiredSize()) {
            ERR_PRINT("Insufficient data for adapter info: req %d, res %s", MgmtEvtAdapterInfo::getRequiredSize(), res->toString().c_str());
            goto fail;
        }
        adapter.reset( new AdapterInfo( *static_cast<MgmtEvtAdapterInfo*>(res.get()) ) );
        adapters.push_back(adapter);
        INFO_PRINT("Adapter %d: %s", dev_id, adapter->toString().c_str());
    }
    ok = setMode(dev_id, MgmtModeReq::SET_LE, 1) && ok;
    ok = setMode(dev_id, MgmtModeReq::SET_POWERED, 1) && ok;
    // ok = setMode(dev_id, MgmtModeReq::SET_CONNECTABLE, 1) && ok;
    return true;

fail:
    return false;
}

MgmtHandler::MgmtHandler()
:comm(HCI_DEV_NONE, HCI_CHANNEL_CONTROL)
{
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor

    if( !comm.isOpen() ) {
        perror("Could not open mgmt control channel");
        return;
    }
    PERF_TS_T0();

    bool ok = true;
    // Mandatory
    {
        MgmtRequest req0(MgmtModeReq::READ_VERSION, MgmtConst::INDEX_NONE);
        DBG_PRINT("MgmtHandler::open req: %s", req0.toString().c_str());
        std::shared_ptr<MgmtEvent> res = send(req0, ibuffer, ibuffer_size, HCIDefaults::HCI_TO_SEND_REQ_POLL_MS);
        if( nullptr == res ) {
            goto fail;
        }
        DBG_PRINT("MgmtHandler::open res: %s", res->toString().c_str());
        if( MgmtEvent::Opcode::CMD_COMPLETE != res->getOpcode() || res->getDataSize() < 3) {
            ERR_PRINT("Wrong version response: %s", res->toString().c_str());
            goto fail;
        }
        const uint8_t *data = res->getData();
        const uint8_t version = data[0];
        const uint16_t revision = get_uint16(data, 1, true /* littleEndian */);
        INFO_PRINT("Bluetooth version %d.%d", version, revision);
        if( version < 1 ) {
            ERR_PRINT("Bluetooth version >= 1.0 required")
            goto fail;
        }
    }
    // Optional
    {
        MgmtRequest req0(MgmtModeReq::READ_COMMANDS, MgmtConst::INDEX_NONE);
        DBG_PRINT("MgmtHandler::open req: %s", req0.toString().c_str());
        std::shared_ptr<MgmtEvent> res = send(req0, ibuffer, ibuffer_size, HCI_TO_SEND_REQ_POLL_MS);
        if( nullptr == res ) {
            goto next1;
        }
        DBG_PRINT("MgmtHandler::open res: %s", res->toString().c_str());
        if( MgmtEvent::Opcode::CMD_COMPLETE == res->getOpcode() && res->getDataSize() >= 4) {
            const uint8_t *data = res->getData();
            const uint16_t num_commands = get_uint16(data, 0, true /* littleEndian */);
            const uint16_t num_events = get_uint16(data, 2, true /* littleEndian */);
            INFO_PRINT("Bluetooth %d commands, %d events", num_commands, num_events);
#ifdef VERBOSE_ON
            const int expDataSize = 4 + num_commands * 2 + num_events * 2;
            if( res->getDataSize() >= expDataSize ) {
                for(int i=0; i< num_commands; i++) {
                    const MgmtOperation op = static_cast<MgmtOperation>( get_uint16(data, 4+i*2, true /* littleEndian */) );
                    DBG_PRINT("kernel op %d: %s", i, mgmt_get_operation_string(op).c_str());
                }
            }
#endif
        }
    }

next1:
    // Register to add/remove adapter optionally:
    // MgmtEvent::INDEX_ADDED, MgmtConst::INDEX_NONE;
    // MgmtEvent::INDEX_REMOVED, MgmtConst::INDEX_NONE;

    // Mandatory
    {
        MgmtRequest req0(MgmtModeReq::READ_INDEX_LIST, MgmtConst::INDEX_NONE);
        DBG_PRINT("MgmtHandler::open req: %s", req0.toString().c_str());
        std::shared_ptr<MgmtEvent> res = send(req0, ibuffer, ibuffer_size, HCI_TO_SEND_REQ_POLL_MS);
        if( nullptr == res ) {
            goto fail;
        }
        DBG_PRINT("MgmtHandler::open res: %s", res->toString().c_str());
        if( MgmtEvent::Opcode::CMD_COMPLETE != res->getOpcode() || res->getDataSize() < 2) {
            ERR_PRINT("Insufficient data for adapter index: res %s", res->toString().c_str());
            goto fail;
        }
        const uint8_t *data = res->getData();
        const uint16_t num_adapter = get_uint16(data, 0, true /* littleEndian */);
        INFO_PRINT("Bluetooth %d adapter", num_adapter);

        const int expDataSize = 2 + num_adapter * 2;
        if( res->getDataSize() < expDataSize ) {
            ERR_PRINT("Insufficient data for %d adapter indices: res %s", num_adapter, res->toString().c_str());
            goto fail;
        }
        for(int i=0; ok && i < num_adapter; i++) {
            const uint16_t dev_id = get_uint16(data, 2+i*2, true /* littleEndian */);
            ok = initAdapter(dev_id);
        }
    }
    if( ok ) {
        PERF_TS_TD("MgmtHandler::open.ok");
        return;
    }

fail:
    close();
    PERF_TS_TD("MgmtHandler::open.fail");
    return;
}


void MgmtHandler::close() {
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    comm.close();
}

bool MgmtHandler::setMode(const int dev_id, const MgmtModeReq::Opcode opc, const uint8_t mode) {
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor

    MgmtModeReq req(opc, dev_id, mode);
    DBG_PRINT("MgmtHandler::open req: %s", req.toString().c_str());
    std::shared_ptr<MgmtEvent> res = send(req, ibuffer, ibuffer_size, HCI_TO_SEND_REQ_POLL_MS);
    if( nullptr != res ) {
        DBG_PRINT("MgmtHandler::setMode res: %s", res->toString().c_str());
        return true;
    } else {
        DBG_PRINT("MgmtHandler::setMode res: NULL");
        return false;
    }
}

int MgmtHandler::findAdapterIdx(const EUI48 &mac) const {
    auto begin = adapters.begin();
    auto it = std::find_if(begin, adapters.end(), [&](std::shared_ptr<const AdapterInfo> const& p) {
        return p->mac == mac;
    });
    if ( it == std::end(adapters) ) {
        return -1;
    } else {
        return std::distance(begin, it);
    }
}

std::shared_ptr<const AdapterInfo> MgmtHandler::getAdapter(const int idx) const {
    if( 0 > idx || idx >= static_cast<int>(adapters.size()) ) {
        throw IndexOutOfBoundsException(idx, adapters.size(), 1, E_FILE_LINE);
    }
    return adapters.at(idx);
}
