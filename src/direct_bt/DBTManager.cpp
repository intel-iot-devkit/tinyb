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
#define VERBOSE_ON 1
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
    #include <signal.h>
}

using namespace direct_bt;

void DBTManager::mgmtReaderThreadImpl() {
    mgmtReaderShallStop = false;
    mgmtReaderRunning = true;
    INFO_PRINT("DBTManager::reader: Started");

    while( !mgmtReaderShallStop ) {
        int len;
        if( !comm.isOpen() ) {
            // not open
            ERR_PRINT("DBTManager::reader: Not connected");
            mgmtReaderShallStop = true;
            break;
        }

        len = comm.read(rbuffer.get_wptr(), rbuffer.getSize());
        if( 0 < len ) {
            const uint16_t paramSize = len >= 6 ? rbuffer.get_uint16(4) : 0;
            if( len < 6 + paramSize ) {
                WARN_PRINT("DBTManager::reader: length mismatch %d < 6 + %d", len, paramSize);
                continue; // discard data
            }
            std::shared_ptr<MgmtEvent> event( MgmtEvent::getSpecialized(rbuffer.get_ptr(), len) );
            const MgmtEvent::Opcode opc = event->getOpcode();
            if( MgmtEvent::Opcode::CMD_COMPLETE == opc || MgmtEvent::Opcode::CMD_STATUS == opc ) {
                DBG_PRINT("DBTManager::reader: CmdResult %s", event->toString().c_str());
                mgmtEventRing.putBlocking( event );
            } else {
                // issue a callback
                MgmtEventCallbackList mgmtEventCallbackList = mgmtEventCallbackLists[opc];
                DBG_PRINT("DBTManager::reader: Event %s -> %zd callbacks", event->toString().c_str(), mgmtEventCallbackList.size());
                for (auto it = mgmtEventCallbackList.begin(); it != mgmtEventCallbackList.end(); ++it) {
                    MgmtEventCallback callback = *it;
                    callback.invoke(event);
                }
            }
        } else if( ETIMEDOUT != errno && !mgmtReaderShallStop ) { // expected exits
            ERR_PRINT("DBTManager::reader: HCIComm error");
        }
    }

    INFO_PRINT("DBTManager::reader: Ended");
    mgmtReaderRunning = false;
}

static void mgmthandler_sigaction(int sig, siginfo_t *info, void *ucontext) {
    INFO_PRINT("DBTManager.sigaction: sig %d, info[code %d, errno %d, signo %d, pid %d, uid %d, fd %d]",
            sig, info->si_code, info->si_errno, info->si_signo,
            info->si_pid, info->si_uid, info->si_fd);
    (void)ucontext;

    if( SIGINT != sig ) {
        return;
    }
    {
        struct sigaction sa_setup;
        bzero(&sa_setup, sizeof(sa_setup));
        sa_setup.sa_handler = SIG_DFL;
        sigemptyset(&(sa_setup.sa_mask));
        sa_setup.sa_flags = 0;
        if( 0 != sigaction( SIGINT, &sa_setup, NULL ) ) {
            ERR_PRINT("DBTManager.sigaction: Resetting sighandler");
        }
    }
}

std::shared_ptr<MgmtEvent> DBTManager::send(MgmtCommand &req) {
    DBG_PRINT("DBTManager::send: Command %s", req.toString().c_str());
    {
        const std::lock_guard<std::recursive_mutex> lock(comm.mutex()); // RAII-style acquire and relinquish via destructor
        TROOctets & pdu = req.getPDU();
        if ( comm.write( pdu.get_ptr(), pdu.getSize() ) < 0 ) {
            ERR_PRINT("DBTManager::send: HCIComm error");
            return nullptr;
        }
    }
    // Ringbuffer read is thread safe
    std::shared_ptr<MgmtEvent> res = receiveNext();
    if( !res->validate(req) ) {
        WARN_PRINT("DBTManager::send: res mismatch: res %s; req %s", res->toString().c_str(), req.toString().c_str());
        return nullptr; // FIXME:: Shall we push back the event to wait for the right one?
    }
    return res;
}

std::shared_ptr<MgmtEvent> DBTManager::receiveNext() {
    return mgmtEventRing.getBlocking();
}

std::shared_ptr<const AdapterInfo> DBTManager::initAdapter(const uint16_t dev_id, const BTMode btMode) {
    std::shared_ptr<const AdapterInfo> adapterInfo = nullptr;
    MgmtCommand req0(MgmtOpcode::READ_INFO, dev_id);
    DBG_PRINT("DBTManager::initAdapter dev_id %d: req: %s", dev_id, req0.toString().c_str());
    {
        std::shared_ptr<MgmtEvent> res = send(req0);
        if( nullptr == res ) {
            goto fail;
        }
        DBG_PRINT("DBTManager::initAdapter dev_id %d: res: %s", dev_id, res->toString().c_str());
        if( MgmtEvent::Opcode::CMD_COMPLETE != res->getOpcode() || res->getTotalSize() < MgmtEvtAdapterInfo::getRequiredSize()) {
            ERR_PRINT("Insufficient data for adapter info: req %d, res %s", MgmtEvtAdapterInfo::getRequiredSize(), res->toString().c_str());
            goto fail;
        }
        adapterInfo = std::shared_ptr<const AdapterInfo>( new AdapterInfo( *static_cast<MgmtEvtAdapterInfo*>(res.get()) ) );
    }

    switch ( btMode ) {
        case BTMode::BT_MODE_DUAL:
            setMode(dev_id, MgmtOpcode::SET_SSP, 1);
            setMode(dev_id, MgmtOpcode::SET_BREDR, 1);
            setMode(dev_id, MgmtOpcode::SET_LE, 1);
            break;
        case BTMode::BT_MODE_BREDR:
            setMode(dev_id, MgmtOpcode::SET_SSP, 1);
            setMode(dev_id, MgmtOpcode::SET_BREDR, 1);
            setMode(dev_id, MgmtOpcode::SET_LE, 0);
            break;
        case BTMode::BT_MODE_LE:
            setMode(dev_id, MgmtOpcode::SET_SSP, 0);
            setMode(dev_id, MgmtOpcode::SET_BREDR, 0);
            setMode(dev_id, MgmtOpcode::SET_LE, 1);
            break;
    }

    setMode(dev_id, MgmtOpcode::SET_CONNECTABLE, 0);
    setMode(dev_id, MgmtOpcode::SET_FAST_CONNECTABLE, 0);
    setMode(dev_id, MgmtOpcode::SET_POWERED, 1);

fail:
    return adapterInfo;
}

void DBTManager::shutdownAdapter(const uint16_t dev_id) {
    setMode(dev_id, MgmtOpcode::SET_CONNECTABLE, 0);
    setMode(dev_id, MgmtOpcode::SET_FAST_CONNECTABLE, 0);
    setMode(dev_id, MgmtOpcode::SET_DISCOVERABLE, 0);
    setMode(dev_id, MgmtOpcode::SET_POWERED, 0);
}

DBTManager::DBTManager(const BTMode btMode)
:btMode(btMode), rbuffer(ClientMaxMTU), comm(HCI_DEV_NONE, HCI_CHANNEL_CONTROL, Defaults::MGMT_READER_THREAD_POLL_TIMEOUT),
 mgmtEventRing(MGMTEVT_RING_CAPACITY), mgmtReaderRunning(false), mgmtReaderShallStop(false)
{
    if( !comm.isOpen() ) {
        ERR_PRINT("DBTManager::open: Could not open mgmt control channel");
        return;
    }

    {
        struct sigaction sa_setup;
        bzero(&sa_setup, sizeof(sa_setup));
        sa_setup.sa_sigaction = mgmthandler_sigaction;
        sigemptyset(&(sa_setup.sa_mask));
        sa_setup.sa_flags = SA_SIGINFO;
        if( 0 != sigaction( SIGINT, &sa_setup, NULL ) ) {
            ERR_PRINT("DBTManager::open: Setting sighandler");
        }
    }
    mgmtReaderThread = std::thread(&DBTManager::mgmtReaderThreadImpl, this);

    PERF_TS_T0();

    bool ok = true;
    // Mandatory
    {
        MgmtCommand req0(MgmtOpcode::READ_VERSION, MgmtConstU16::INDEX_NONE);
        DBG_PRINT("DBTManager::open req: %s", req0.toString().c_str());
        std::shared_ptr<MgmtEvent> res = send(req0);
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
        MgmtCommand req0(MgmtOpcode::READ_COMMANDS, MgmtConstU16::INDEX_NONE);
        DBG_PRINT("DBTManager::open req: %s", req0.toString().c_str());
        std::shared_ptr<MgmtEvent> res = send(req0);
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
                    const MgmtOpcode op = static_cast<MgmtOpcode>( get_uint16(data, 4+i*2, true /* littleEndian */) );
                    DBG_PRINT("kernel op %d: %s", i, getMgmtOpcodeString(op).c_str());
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
        MgmtCommand req0(MgmtOpcode::READ_INDEX_LIST, MgmtConstU16::INDEX_NONE);
        DBG_PRINT("DBTManager::open req: %s", req0.toString().c_str());
        std::shared_ptr<MgmtEvent> res = send(req0);
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
        adapters.resize(num_adapter, nullptr);
        for(int i=0; ok && i < num_adapter; i++) {
            const uint16_t dev_id = get_uint16(data, 2+i*2, true /* littleEndian */);
            if( dev_id >= num_adapter ) {
                throw InternalError("dev_id "+std::to_string(dev_id)+" >= num_adapter "+std::to_string(num_adapter), E_FILE_LINE);
            }
            if( adapters[dev_id] != nullptr ) {
                throw InternalError("adapters[dev_id="+std::to_string(dev_id)+"] != nullptr: "+adapters[dev_id]->toString(), E_FILE_LINE);
            }
            std::shared_ptr<const AdapterInfo> adapterInfo = initAdapter(dev_id, btMode);
            adapters[dev_id] = adapterInfo;
            if( nullptr != adapterInfo ) {
                DBG_PRINT("DBTManager::adapters %d/%d: dev_id %d: %s", i, num_adapter, dev_id, adapterInfo->toString().c_str());
                ok = true;
            } else {
                DBG_PRINT("DBTManager::adapters %d/%d: dev_id %d: FAILED", i, num_adapter, dev_id);
                ok = false;
            }
        }
    }

    if( ok ) {
        addMgmtEventCallback(MgmtEvent::Opcode::CLASS_OF_DEV_CHANGED, bindClassFunction(this, &DBTManager::mgmtEvClassOfDeviceChangedCB));
        addMgmtEventCallback(MgmtEvent::Opcode::LOCAL_NAME_CHANGED, bindClassFunction(this, &DBTManager::mgmtEvLocalNameChangedCB));
        addMgmtEventCallback(MgmtEvent::Opcode::DISCOVERING, bindClassFunction(this, &DBTManager::mgmtEvDeviceDiscoveringCB));
        addMgmtEventCallback(MgmtEvent::Opcode::NEW_SETTINGS, bindClassFunction(this, &DBTManager::mgmtEvNewSettingsCB));
        addMgmtEventCallback(MgmtEvent::Opcode::DEVICE_FOUND, bindClassFunction(this, &DBTManager::mgmtEvDeviceFoundCB));
        addMgmtEventCallback(MgmtEvent::Opcode::DEVICE_DISCONNECTED, bindClassFunction(this, &DBTManager::mgmtEvDeviceDisconnectedCB));
        addMgmtEventCallback(MgmtEvent::Opcode::DEVICE_CONNECTED, bindClassFunction(this, &DBTManager::mgmtEvDeviceConnectedCB));
        addMgmtEventCallback(MgmtEvent::Opcode::CONNECT_FAILED, bindClassFunction(this, &DBTManager::mgmtEvConnectFailedCB));
        addMgmtEventCallback(MgmtEvent::Opcode::PIN_CODE_REQUEST, bindClassFunction(this, &DBTManager::mgmtEvDevicePinCodeRequestCB));
        addMgmtEventCallback(MgmtEvent::Opcode::DEVICE_UNPAIRED, bindClassFunction(this, &DBTManager::mgmtEvDeviceUnpairedCB));
        addMgmtEventCallback(MgmtEvent::Opcode::NEW_CONN_PARAM, bindClassFunction(this, &DBTManager::mgmtEvNewConnectionParamCB));
        addMgmtEventCallback(MgmtEvent::Opcode::DEVICE_WHITELIST_ADDED, bindClassFunction(this, &DBTManager::mgmtEvDeviceWhitelistAddedCB));
        addMgmtEventCallback(MgmtEvent::Opcode::DEVICE_WHITELIST_REMOVED, bindClassFunction(this, &DBTManager::mgmtEvDeviceWhilelistRemovedCB));
        PERF_TS_TD("DBTManager::open.ok");
        return;
    }

fail:
    close();
    PERF_TS_TD("DBTManager::open.fail");
    return;
}

void DBTManager::close() {
    DBG_PRINT("DBTManager::close: Start");
    const std::lock_guard<std::recursive_mutex> lock(mtx_api); // RAII-style acquire and relinquish via destructor
    clearAllMgmtEventCallbacks();
    if( mgmtReaderRunning && mgmtReaderThread.joinable() ) {
        mgmtReaderShallStop = true;
        pthread_t tid = mgmtReaderThread.native_handle();
        pthread_kill(tid, SIGINT);
    }

    for (auto it = adapters.begin(); it != adapters.end(); ) {
        shutdownAdapter((*it)->dev_id);
    }
    comm.close();

    if( mgmtReaderRunning && mgmtReaderThread.joinable() ) {
        // still running ..
        DBG_PRINT("DBTManager::close: join mgmtReaderThread");
        mgmtReaderThread.join();
    }
    mgmtReaderThread = std::thread(); // empty
    DBG_PRINT("DBTManager::close: End");
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

bool DBTManager::setMode(const int dev_id, const MgmtOpcode opc, const uint8_t mode) {
    MgmtUint8Cmd req(opc, dev_id, mode);
    DBG_PRINT("DBTManager::setMode: %s", req.toString().c_str());
    std::shared_ptr<MgmtEvent> res = send(req);
    if( nullptr != res ) {
        DBG_PRINT("DBTManager::setMode res: %s", res->toString().c_str());
        return true;
    } else {
        DBG_PRINT("DBTManager::setMode res: NULL");
        return false;
    }
}

ScanType DBTManager::startDiscovery(const int dev_id) {
    ScanType scanType;
    switch ( btMode ) {
        case BTMode::BT_MODE_DUAL:
            scanType = ScanType::SCAN_TYPE_DUAL;
            break;
        case BTMode::BT_MODE_BREDR:
            scanType = ScanType::SCAN_TYPE_BREDR;
            break;
        case BTMode::BT_MODE_LE:
            scanType = ScanType::SCAN_TYPE_LE;
            break;
    }
    return startDiscovery(dev_id, scanType);
}

ScanType DBTManager::startDiscovery(const int dev_id, const ScanType scanType) {
    MgmtUint8Cmd req(MgmtOpcode::START_DISCOVERY, dev_id, scanType);
    DBG_PRINT("DBTManager::startDiscovery: %s", req.toString().c_str());
    const std::lock_guard<std::recursive_mutex> lock(mtx_api); // RAII-style acquire and relinquish via destructor
    std::shared_ptr<MgmtEvent> res = send(req);
    if( nullptr == res ) {
        DBG_PRINT("DBTManager::startDiscovery res: NULL");
        return ScanType::SCAN_TYPE_NONE;
    }
    DBG_PRINT("DBTManager::startDiscovery res: %s", res->toString().c_str());
    ScanType type = ScanType::SCAN_TYPE_NONE;
    if( res->getOpcode() == MgmtEvent::Opcode::CMD_COMPLETE ) {
        const MgmtEvtCmdComplete &res1 = *static_cast<const MgmtEvtCmdComplete *>(res.get());
        if( MgmtStatus::SUCCESS == res1.getStatus() && 1 == res1.getDataSize() ) {
            type = static_cast<ScanType>( *res1.getData() );
            DBG_PRINT("DBTManager::startDiscovery res: ScanType %d", (int)type);
        } else {
            DBG_PRINT("DBTManager::startDiscovery no-success or invalid res: %s", res1.toString().c_str());
        }
    } else {
        DBG_PRINT("DBTManager::startDiscovery res: NULL");
    }
    return type;
}
bool DBTManager::stopDiscovery(const int dev_id, const ScanType type) {
    MgmtUint8Cmd req(MgmtOpcode::STOP_DISCOVERY, dev_id, type);
    DBG_PRINT("DBTManager::stopDiscovery: %s", req.toString().c_str());
    const std::lock_guard<std::recursive_mutex> lock(mtx_api); // RAII-style acquire and relinquish via destructor
    std::shared_ptr<MgmtEvent> res = send(req);
    if( nullptr == res ) {
        DBG_PRINT("DBTManager::stopDiscovery res: NULL");
        return false;
    }
    DBG_PRINT("DBTManager::stopDiscovery res: %s", res->toString().c_str());
    if( res->getOpcode() == MgmtEvent::Opcode::CMD_COMPLETE ) {
        const MgmtEvtCmdComplete &res1 = *static_cast<const MgmtEvtCmdComplete *>(res.get());
        return MgmtStatus::SUCCESS == res1.getStatus();
    } else {
        DBG_PRINT("DBTManager::stopDiscovery res: NULL");
        return false;
    }
}

uint16_t DBTManager::create_connection(const int dev_id,
                        const EUI48 &peer_bdaddr,
                        const BDAddressType peer_mac_type,
                        const BDAddressType own_mac_type,
                        const uint16_t interval, const uint16_t window,
                        const uint16_t min_interval, const uint16_t max_interval,
                        const uint16_t latency, const uint16_t supervision_timeout,
                        const uint16_t min_ce_length, const uint16_t max_ce_length,
                        const uint8_t initiator_filter) {
    // MgmtUint8Cmd req(MgmtOpcode::, dev_id, scanType);
    DBG_PRINT("DBTManager::le_create_conn: %s", peer_bdaddr.toString().c_str());
    const std::lock_guard<std::recursive_mutex> lock(mtx_api); // RAII-style acquire and relinquish via destructor
    (void)dev_id;
    (void)peer_mac_type;
    (void)own_mac_type;
    (void)interval;
    (void)window;
    (void)min_interval;
    (void)max_interval;
    (void)latency;
    (void)supervision_timeout;
    (void)min_ce_length;
    (void)max_ce_length;
    (void)initiator_filter;
    return 0;
}

bool DBTManager::disconnect(const int dev_id, const EUI48 &peer_bdaddr, const BDAddressType peer_mac_type) {
    MgmtDisconnectCmd req(dev_id, peer_bdaddr, peer_mac_type);
    DBG_PRINT("DBTManager::disconnect: %s", req.toString().c_str());
    const std::lock_guard<std::recursive_mutex> lock(mtx_api); // RAII-style acquire and relinquish via destructor
    std::shared_ptr<MgmtEvent> res = send(req);
    if( nullptr == res ) {
        DBG_PRINT("DBTManager::stopDiscovery res: NULL");
        return false;
    }
    DBG_PRINT("DBTManager::disconnect res: %s", res->toString().c_str());
    if( res->getOpcode() == MgmtEvent::Opcode::CMD_COMPLETE ) {
        const MgmtEvtCmdComplete &res1 = *static_cast<const MgmtEvtCmdComplete *>(res.get());
        return MgmtStatus::SUCCESS == res1.getStatus();
    } else {
        DBG_PRINT("DBTManager::disconnect res: NULL");
        return false;
    }
}

/***
 *
 * MgmtEventCallback section
 *
 */

void DBTManager::addMgmtEventCallback(const MgmtEvent::Opcode opc, const MgmtEventCallback &cb) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_api); // RAII-style acquire and relinquish via destructor
    checkMgmtEventCallbackListsIndex(opc);
    mgmtEventCallbackLists[opc].push_back(cb);
}
int DBTManager::removeMgmtEventCallback(const MgmtEvent::Opcode opc, const MgmtEventCallback &cb) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_api); // RAII-style acquire and relinquish via destructor
    checkMgmtEventCallbackListsIndex(opc);
    int count = 0;
    MgmtEventCallbackList &l = mgmtEventCallbackLists[opc];
    for (auto it = l.begin(); it != l.end(); ) {
        if ( *it == cb ) {
            it = l.erase(it);
            count++;
        } else {
            ++it;
        }
    }
    return count;
}
void DBTManager::clearMgmtEventCallbacks(const MgmtEvent::Opcode opc) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_api); // RAII-style acquire and relinquish via destructor
    checkMgmtEventCallbackListsIndex(opc);
    mgmtEventCallbackLists[opc].clear();
}
void DBTManager::clearAllMgmtEventCallbacks() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_api); // RAII-style acquire and relinquish via destructor
    for(size_t i=0; i<mgmtEventCallbackLists.size(); i++) {
        mgmtEventCallbackLists[i].clear();
    }
}

bool DBTManager::mgmtEvClassOfDeviceChangedCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBRManager::EventCB:ClassOfDeviceChanged: %s", e->toString().c_str());
    return true;
}
bool DBTManager::mgmtEvLocalNameChangedCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBRManager::EventCB:LocalNameChanged: %s", e->toString().c_str());
    const MgmtEvtLocalNameChanged &event = *static_cast<const MgmtEvtLocalNameChanged *>(e.get());
    (void)event;
    return true;
}
bool DBTManager::mgmtEvDeviceDiscoveringCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBRManager::EventCB:DeviceDiscovering: %s", e->toString().c_str());
    const MgmtEvtDiscovering &event = *static_cast<const MgmtEvtDiscovering *>(e.get());
    (void)event;
    return true;
}
bool DBTManager::mgmtEvNewSettingsCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBRManager::EventCB:NewSettings: %s", e->toString().c_str());
    const MgmtEvtNewSettings &event = *static_cast<const MgmtEvtNewSettings *>(e.get());
    (void)event;
    return true;
}
bool DBTManager::mgmtEvDeviceFoundCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBRManager::EventCB:DeviceFound: %s", e->toString().c_str());
    const MgmtEvtDeviceFound &event = *static_cast<const MgmtEvtDeviceFound *>(e.get());
    (void)event;
    return true;
}
bool DBTManager::mgmtEvDeviceDisconnectedCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBRManager::EventCB:DeviceDisconnected: %s", e->toString().c_str());
    const MgmtEvtDeviceDisconnected &event = *static_cast<const MgmtEvtDeviceDisconnected *>(e.get());
    (void)event;
    return true;
}
bool DBTManager::mgmtEvDeviceConnectedCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBRManager::EventCB:DeviceConnected: %s", e->toString().c_str());
    const MgmtEvtDeviceConnected &event = *static_cast<const MgmtEvtDeviceConnected *>(e.get());
    (void)event;
    return true;
}
bool DBTManager::mgmtEvConnectFailedCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBRManager::EventCB:ConnectFailed: %s", e->toString().c_str());
    const MgmtEvtDeviceConnectFailed &event = *static_cast<const MgmtEvtDeviceConnectFailed *>(e.get());
    (void)event;
    return true;
}
bool DBTManager::mgmtEvDevicePinCodeRequestCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBRManager::EventCB:PinCodeRequest: %s", e->toString().c_str());
    const MgmtEvtPinCodeRequest &event = *static_cast<const MgmtEvtPinCodeRequest *>(e.get());
    (void)event;
    return true;
}
bool DBTManager::mgmtEvDeviceUnpairedCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBRManager::EventCB:DeviceUnpaired: %s", e->toString().c_str());
    const MgmtEvtDeviceUnpaired &event = *static_cast<const MgmtEvtDeviceUnpaired *>(e.get());
    (void)event;
    return true;
}
bool DBTManager::mgmtEvNewConnectionParamCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBRManager::EventCB:NewConnectionParam: %s", e->toString().c_str());
    const MgmtEvtNewConnectionParam &event = *static_cast<const MgmtEvtNewConnectionParam *>(e.get());
    (void)event;
    return true;
}
bool DBTManager::mgmtEvDeviceWhitelistAddedCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBRManager::EventCB:DeviceAdded: %s", e->toString().c_str());
    const MgmtEvtDeviceWhitelistAdded &event = *static_cast<const MgmtEvtDeviceWhitelistAdded *>(e.get());
    (void)event;
    return true;
}
bool DBTManager::mgmtEvDeviceWhilelistRemovedCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("DBRManager::EventCB:DeviceRemoved: %s", e->toString().c_str());
    const MgmtEvtDeviceWhitelistRemoved &event = *static_cast<const MgmtEvtDeviceWhitelistRemoved *>(e.get());
    (void)event;
    return true;
}
