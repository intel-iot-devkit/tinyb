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

#include "DBTManager.hpp"
#include "HCIIoctl.hpp"
#include "HCIComm.hpp"
#include "DBTTypes.hpp"

extern "C" {
    #include <inttypes.h>
    #include <unistd.h>
    #include <poll.h>
}

using namespace direct_bt;

std::shared_ptr<MgmtEvent> DBTManager::send(MgmtRequest &req, uint8_t* buffer, const int capacity, const int timeoutMS) {
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

bool DBTManager::initAdapter(const uint16_t dev_id) {
    bool ok = true;
    std::shared_ptr<const AdapterInfo> adapter;
    MgmtRequest req0(MgmtModeReq::READ_INFO, dev_id);
    DBG_PRINT("DBTManager::initAdapter %d: req: %s", dev_id, req0.toString().c_str());
    {
        std::shared_ptr<MgmtEvent> res = send(req0, ibuffer, ibuffer_size, HCI_TO_SEND_REQ_POLL_MS);
        if( nullptr == res ) {
            goto fail;
        }
        DBG_PRINT("DBTManager::initAdapter %d: res: %s", dev_id, res->toString().c_str());
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

DBTManager::DBTManager()
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
        DBG_PRINT("DBTManager::open req: %s", req0.toString().c_str());
        std::shared_ptr<MgmtEvent> res = send(req0, ibuffer, ibuffer_size, HCIDefaults::HCI_TO_SEND_REQ_POLL_MS);
        if( nullptr == res ) {
            goto fail;
        }
        DBG_PRINT("DBTManager::open res: %s", res->toString().c_str());
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
        DBG_PRINT("DBTManager::open req: %s", req0.toString().c_str());
        std::shared_ptr<MgmtEvent> res = send(req0, ibuffer, ibuffer_size, HCI_TO_SEND_REQ_POLL_MS);
        if( nullptr == res ) {
            goto next1;
        }
        DBG_PRINT("DBTManager::open res: %s", res->toString().c_str());
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
        DBG_PRINT("DBTManager::open req: %s", req0.toString().c_str());
        std::shared_ptr<MgmtEvent> res = send(req0, ibuffer, ibuffer_size, HCI_TO_SEND_REQ_POLL_MS);
        if( nullptr == res ) {
            goto fail;
        }
        DBG_PRINT("DBTManager::open res: %s", res->toString().c_str());
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
        PERF_TS_TD("DBTManager::open.ok");
        return;
    }

fail:
    close();
    PERF_TS_TD("DBTManager::open.fail");
    return;
}


void DBTManager::close() {
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    comm.close();
}

bool DBTManager::setMode(const int dev_id, const MgmtModeReq::Opcode opc, const uint8_t mode) {
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor

    MgmtModeReq req(opc, dev_id, mode);
    DBG_PRINT("DBTManager::open req: %s", req.toString().c_str());
    std::shared_ptr<MgmtEvent> res = send(req, ibuffer, ibuffer_size, HCI_TO_SEND_REQ_POLL_MS);
    if( nullptr != res ) {
        DBG_PRINT("DBTManager::setMode res: %s", res->toString().c_str());
        return true;
    } else {
        DBG_PRINT("DBTManager::setMode res: NULL");
        return false;
    }
}

int DBTManager::findAdapterIdx(const EUI48 &mac) const {
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

std::shared_ptr<const AdapterInfo> DBTManager::getAdapter(const int idx) const {
    if( 0 > idx || idx >= static_cast<int>(adapters.size()) ) {
        throw IndexOutOfBoundsException(idx, adapters.size(), 1, E_FILE_LINE);
    }
    return adapters.at(idx);
}
