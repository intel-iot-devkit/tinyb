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

#include  <algorithm>

extern "C" {
    #include <unistd.h>
    #include <sys/socket.h>
    #include <poll.h>
    #include <signal.h>
}

// #define PERF_PRINT_ON 1
// #define PERF2_PRINT_ON 1
#include <dbt_debug.hpp>

// PERF2_PRINT_ON for read/write single values
#ifdef PERF2_PRINT_ON
    #define PERF2_TS_T0() PERF_TS_T0()
    #define PERF2_TS_TD(m) PERF_TS_TD(m)
#else
    #define PERF2_TS_T0()
    #define PERF2_TS_TD(m)
#endif

#include "BasicAlgos.hpp"

#include "L2CAPIoctl.hpp"
#include "GATTNumbers.hpp"

#include "GATTHandler.hpp"

#include "HCIComm.hpp"
#include "DBTTypes.hpp"
#include "DBTDevice.hpp"

using namespace direct_bt;

#define CASE_TO_STRING(V) case V: return #V;

bool GATTHandler::validateConnected() {
    bool l2capIsConnected = l2cap.getIsConnected();
    bool l2capHasIOError = l2cap.getHasIOError();

    if( hasIOError || l2capHasIOError ) {
        ERR_PRINT("IOError state: GattHandler %s, l2cap %s: %s",
                getStateString().c_str(), l2cap.getStateString().c_str(), deviceString.c_str());
        return false;
    }

    if( !isConnected || !l2capIsConnected ) {
        ERR_PRINT("Disconnected state: GattHandler %s, l2cap %s: %s",
                getStateString().c_str(), l2cap.getStateString().c_str(), deviceString.c_str());
        return false;
    }
    return true;
}

bool GATTHandler::addCharacteristicListener(std::shared_ptr<GATTCharacteristicListener> l) {
    if( nullptr == l ) {
        throw IllegalArgumentException("GATTEventListener ref is null", E_FILE_LINE);
    }
    const std::lock_guard<std::recursive_mutex> lock(mtx_eventListenerList); // RAII-style acquire and relinquish via destructor
    for(auto it = characteristicListenerList.begin(); it != characteristicListenerList.end(); ) {
        if ( **it == *l ) {
            return false; // already included
        } else {
            ++it;
        }
    }
    characteristicListenerList.push_back(l);
    return true;
}

bool GATTHandler::removeCharacteristicListener(std::shared_ptr<GATTCharacteristicListener> l) {
    if( nullptr == l ) {
        throw IllegalArgumentException("GATTEventListener ref is null", E_FILE_LINE);
    }
    return removeCharacteristicListener( l.get() );
}

bool GATTHandler::removeCharacteristicListener(const GATTCharacteristicListener * l) {
    if( nullptr == l ) {
        throw IllegalArgumentException("GATTEventListener ref is null", E_FILE_LINE);
    }
    const std::lock_guard<std::recursive_mutex> lock(mtx_eventListenerList); // RAII-style acquire and relinquish via destructor
    for(auto it = characteristicListenerList.begin(); it != characteristicListenerList.end(); ) {
        if ( **it == *l ) {
            it = characteristicListenerList.erase(it);
            return true;
        } else {
            ++it;
        }
    }
    return false;
}

int GATTHandler::removeAllAssociatedCharacteristicListener(std::shared_ptr<GATTCharacteristic> associatedCharacteristic) {
    if( nullptr == associatedCharacteristic ) {
        throw IllegalArgumentException("GATTCharacteristic ref is null", E_FILE_LINE);
    }
    return removeAllAssociatedCharacteristicListener( associatedCharacteristic.get() );
}

int GATTHandler::removeAllAssociatedCharacteristicListener(const GATTCharacteristic * associatedCharacteristic) {
    if( nullptr == associatedCharacteristic ) {
        throw IllegalArgumentException("GATTCharacteristic ref is null", E_FILE_LINE);
    }
    const std::lock_guard<std::recursive_mutex> lock(mtx_eventListenerList); // RAII-style acquire and relinquish via destructor
    for(auto it = characteristicListenerList.begin(); it != characteristicListenerList.end(); ) {
        if ( (*it)->match(*associatedCharacteristic) ) {
            it = characteristicListenerList.erase(it);
            return true;
        } else {
            ++it;
        }
    }
    return false;
}

int GATTHandler::removeAllCharacteristicListener() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_eventListenerList); // RAII-style acquire and relinquish via destructor
    int count = characteristicListenerList.size();
    characteristicListenerList.clear();
    return count;
}

void GATTHandler::setSendIndicationConfirmation(const bool v) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_eventListenerList); // RAII-style acquire and relinquish via destructor
    sendIndicationConfirmation = v;
}

bool GATTHandler::getSendIndicationConfirmation() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_eventListenerList); // RAII-style acquire and relinquish via destructor
    return sendIndicationConfirmation;
}

void GATTHandler::l2capReaderThreadImpl() {
    bool ioErrorCause = false;
    {
        const std::lock_guard<std::mutex> lock(mtx_l2capReaderInit); // RAII-style acquire and relinquish via destructor
        l2capReaderShallStop = false;
        l2capReaderRunning = true;
        DBG_PRINT("l2capReaderThreadImpl Started");
        cv_l2capReaderInit.notify_all();
    }

    while( !l2capReaderShallStop ) {
        int len;
        if( !validateConnected() ) {
            ERR_PRINT("GATTHandler::l2capReaderThread: Invalid IO state -> Stop");
            l2capReaderShallStop = true;
            break;
        }

        len = l2cap.read(rbuffer.get_wptr(), rbuffer.getSize(), number(Defaults::L2CAP_READER_THREAD_POLL_TIMEOUT));
        if( 0 < len ) {
            const AttPDUMsg * attPDU = AttPDUMsg::getSpecialized(rbuffer.get_ptr(), len);
            const AttPDUMsg::Opcode opc = attPDU->getOpcode();

            if( AttPDUMsg::Opcode::ATT_HANDLE_VALUE_NTF == opc ) {
                const AttHandleValueRcv * a = static_cast<const AttHandleValueRcv*>(attPDU);
                COND_PRINT(debug_data, "GATTHandler: NTF: %s, listener %zd", a->toString().c_str(), characteristicListenerList.size());
                GATTCharacteristicRef decl = findCharacterisicsByValueHandle(a->getHandle());
                const std::shared_ptr<TROOctets> data(new POctets(a->getValue()));
                const uint64_t timestamp = a->ts_creation;
                int i=0;
                for_each_idx_mtx(mtx_eventListenerList, characteristicListenerList, [&](std::shared_ptr<GATTCharacteristicListener> &l) {
                    try {
                        if( l->match(*decl) ) {
                            l->notificationReceived(decl, data, timestamp);
                        }
                    } catch (std::exception &e) {
                        ERR_PRINT("GATTHandler::notificationReceived-CBs %d/%zd: GATTCharacteristicListener %s: Caught exception %s",
                                i+1, characteristicListenerList.size(),
                                aptrHexString((void*)l.get()).c_str(), e.what());
                    }
                    i++;
                });
                attPDU = nullptr;
            } else if( AttPDUMsg::Opcode::ATT_HANDLE_VALUE_IND == opc ) {
                const AttHandleValueRcv * a = static_cast<const AttHandleValueRcv*>(attPDU);
                COND_PRINT(debug_data, "GATTHandler: IND: %s, sendIndicationConfirmation %d, listener %zd", a->toString().c_str(), sendIndicationConfirmation, characteristicListenerList.size());
                bool cfmSent = false;
                if( sendIndicationConfirmation ) {
                    AttHandleValueCfm cfm;
                    send(cfm);
                    cfmSent = true;
                }
                GATTCharacteristicRef decl = findCharacterisicsByValueHandle(a->getHandle());
                const std::shared_ptr<TROOctets> data(new POctets(a->getValue()));
                const uint64_t timestamp = a->ts_creation;
                int i=0;
                for_each_idx_mtx(mtx_eventListenerList, characteristicListenerList, [&](std::shared_ptr<GATTCharacteristicListener> &l) {
                    try {
                        if( l->match(*decl) ) {
                            l->indicationReceived(decl, data, timestamp, cfmSent);
                        }
                    } catch (std::exception &e) {
                        ERR_PRINT("GATTHandler::indicationReceived-CBs %d/%zd: GATTCharacteristicListener %s, cfmSent %d: Caught exception %s",
                                i+1, characteristicListenerList.size(),
                                aptrHexString((void*)l.get()).c_str(), cfmSent, e.what());
                    }
                    i++;
                });
                attPDU = nullptr;
            } else if( AttPDUMsg::Opcode::ATT_MULTIPLE_HANDLE_VALUE_NTF == opc ) {
                // FIXME TODO ..
                ERR_PRINT("GATTHandler: MULTI-NTF not implemented: %s", attPDU->toString().c_str());
            } else {
                attPDURing.putBlocking( std::shared_ptr<const AttPDUMsg>( attPDU ) );
                attPDU = nullptr;
            }
            if( nullptr != attPDU ) {
                delete attPDU; // free unhandled PDU
            }
        } else if( ETIMEDOUT != errno && !l2capReaderShallStop ) { // expected exits
            ERR_PRINT("GATTHandler::l2capReaderThread: l2cap read error -> Stop");
            l2capReaderShallStop = true;
            ioErrorCause = true;
        }
    }

    INFO_PRINT("l2capReaderThreadImpl Ended. Ring has %d entries flushed", attPDURing.getSize());
    l2capReaderRunning = false;
    attPDURing.clear();
    disconnect(true /* disconnectDevice */, ioErrorCause);
}

GATTHandler::GATTHandler(const std::shared_ptr<DBTDevice> &device, const int replyTimeoutMS)
: debug_data(DBTEnv::getBooleanProperty("direct_bt.debug.gatt.data", false)),
  wbr_device(device), deviceString(device->getAddressString()), rbuffer(number(Defaults::MAX_ATT_MTU)),
  l2cap(device, L2CAP_PSM_UNDEF, L2CAP_CID_ATT), replyTimeoutMS(replyTimeoutMS),
  isConnected(false), hasIOError(false),
  attPDURing(number(Defaults::ATTPDU_RING_CAPACITY)),
  l2capReaderThreadId(0), l2capReaderRunning(false), l2capReaderShallStop(false),
  serverMTU(number(Defaults::MIN_ATT_MTU)), usedMTU(number(Defaults::MIN_ATT_MTU))
{ }

GATTHandler::~GATTHandler() {
    disconnect(false /* disconnectDevice */, false /* ioErrorCause */);
    services.clear();
}

bool GATTHandler::connect() {
    // Avoid connect re-entry -> potential deadlock
    bool expConn = false; // C++11, exp as value since C++20
    if( !isConnected.compare_exchange_strong(expConn, true) ) {
        // already connected
        DBG_PRINT("GATTHandler::connect: Already connected: GattHandler[%s], l2cap[%s]: %s",
                  getStateString().c_str(), l2cap.getStateString().c_str(), deviceString.c_str());
        return true;
    }
    // Lock to avoid other threads using instance while connecting
    const std::lock_guard<std::recursive_mutex> lock(mtx_command); // RAII-style acquire and relinquish via destructor

    hasIOError = false;
    DBG_PRINT("GATTHandler::connect: Start: GattHandler[%s], l2cap[%s]: %s",
                getStateString().c_str(), l2cap.getStateString().c_str(), deviceString.c_str());

    if( !l2cap.connect() || !validateConnected() ) {
        DBG_PRINT("GATTHandler.connect: Could not connect");
        return false;
    }

    /**
     * We utilize DBTManager's mgmthandler_sigaction SIGALRM handler,
     * as we only can install one handler.
     */
    {
        std::unique_lock<std::mutex> lock(mtx_l2capReaderInit); // RAII-style acquire and relinquish via destructor

        std::thread l2capReaderThread = std::thread(&GATTHandler::l2capReaderThreadImpl, this);
        l2capReaderThreadId = l2capReaderThread.native_handle();
        // Avoid 'terminate called without an active exception'
        // as l2capReaderThread may end due to I/O errors.
        l2capReaderThread.detach();

        while( false == l2capReaderRunning ) {
            cv_l2capReaderInit.wait(lock);
        }
    }

    // First point of failure if device exposes no GATT functionality. Allow a longer timeout!
    serverMTU = exchangeMTU(number(Defaults::MAX_ATT_MTU), number(Defaults::GATT_INITIAL_COMMAND_REPLY_TIMEOUT));
    usedMTU = std::min(number(Defaults::MAX_ATT_MTU), (int)serverMTU);
    if( 0 == serverMTU ) {
        ERR_PRINT("GATTHandler::connect: Zero serverMTU -> disconnect: %s", deviceString.c_str());
        disconnect(true /* disconnectDevice */, false /* ioErrorCause */);
        return false;
    }
    return true;
}

bool GATTHandler::disconnect(const bool disconnectDevice, const bool ioErrorCause) {
    // Avoid disconnect re-entry -> potential deadlock
    bool expConn = true; // C++11, exp as value since C++20
    if( !isConnected.compare_exchange_strong(expConn, false) ) {
        // not connected
        DBG_PRINT("GATTHandler::disconnect: Not connected: disconnectDevice %d, ioErrorCause %d: GattHandler[%s], l2cap[%s]: %s",
                  disconnectDevice, ioErrorCause, getStateString().c_str(), l2cap.getStateString().c_str(), deviceString.c_str());
        l2cap.disconnect(); // interrupt GATT's L2CAP ::connect(..), avoiding prolonged hang
        characteristicListenerList.clear();
        return false;
    }
    // Lock to avoid other threads using instance while disconnecting
    const std::lock_guard<std::recursive_mutex> lock(mtx_command); // RAII-style acquire and relinquish via destructor

    hasIOError = false;
    DBG_PRINT("GATTHandler::disconnect: Start: disconnectDevice %d, ioErrorCause %d: GattHandler[%s], l2cap[%s]: %s",
              disconnectDevice, ioErrorCause, getStateString().c_str(), l2cap.getStateString().c_str(), deviceString.c_str());

    l2cap.disconnect(); // interrupt GATT's L2CAP ::connect(..), avoiding prolonged hang

    const pthread_t tid_self = pthread_self();
    const pthread_t tid_l2capReader = l2capReaderThreadId;
    l2capReaderThreadId = 0;
    const bool is_l2capReader = tid_l2capReader == tid_self;
    DBG_PRINT("GATTHandler.disconnect: l2capReader[running %d, shallStop %d, isReader %d, tid %p)",
              l2capReaderRunning.load(), l2capReaderShallStop.load(), is_l2capReader, (void*)tid_l2capReader);
    if( l2capReaderRunning ) {
        l2capReaderShallStop = true;
        if( !is_l2capReader && 0 != tid_l2capReader ) {
            int kerr;
            if( 0 != ( kerr = pthread_kill(tid_l2capReader, SIGALRM) ) ) {
                ERR_PRINT("GATTHandler::disconnect: pthread_kill %p FAILED: %d", (void*)tid_l2capReader, kerr);
            }
        }
    }
    removeAllCharacteristicListener();

    std::shared_ptr<DBTDevice> device = getDevice();

    if( disconnectDevice && nullptr != device ) {
        // Cleanup device resources, proper connection state
        // Intentionally giving the POWER_OFF reason for the device in case of ioErrorCause!
        const HCIStatusCode reason = ioErrorCause ?
                               HCIStatusCode::REMOTE_DEVICE_TERMINATED_CONNECTION_POWER_OFF :
                               HCIStatusCode::REMOTE_USER_TERMINATED_CONNECTION;
        device->disconnect(false /* sentFromManager */, ioErrorCause, reason);
    }

    DBG_PRINT("GATTHandler::disconnect: End: %s", deviceString.c_str());
    return true;
}

void GATTHandler::send(const AttPDUMsg & msg) {
    if( !validateConnected() ) {
        throw IllegalStateException("GATTHandler::send: Invalid IO State: req "+msg.toString()+" to "+deviceString, E_FILE_LINE);
    }
    if( msg.pdu.getSize() > usedMTU ) {
        throw IllegalArgumentException("clientMaxMTU "+std::to_string(msg.pdu.getSize())+" > usedMTU "+std::to_string(usedMTU)+
                                       " to "+deviceString, E_FILE_LINE);
    }

    // Thread safe write operation only for concurrent access
    const std::lock_guard<std::recursive_mutex> lock(mtx_write); // RAII-style acquire and relinquish via destructor

    const int res = l2cap.write(msg.pdu.get_ptr(), msg.pdu.getSize());
    if( 0 > res ) {
        ERR_PRINT("GATTHandler::send: l2cap write error -> disconnect: %s to %s", msg.toString().c_str(), deviceString.c_str());
        hasIOError = true;
        disconnect(true /* disconnectDevice */, true /* ioErrorCause */); // state -> Disconnected
        throw BluetoothException("GATTHandler::send: l2cap write error: req "+msg.toString()+" to "+deviceString, E_FILE_LINE);
    }
    if( res != msg.pdu.getSize() ) {
        ERR_PRINT("GATTHandler::send: l2cap write count error, %d != %d: %s -> disconnect: %s",
                res, msg.pdu.getSize(), msg.toString().c_str(), deviceString.c_str());
        hasIOError = true;
        disconnect(true /* disconnectDevice */, true /* ioErrorCause */); // state -> Disconnected
        throw BluetoothException("GATTHandler::send: l2cap write count error, "+std::to_string(res)+" != "+std::to_string(res)
                                 +": "+msg.toString()+" -> disconnect: "+deviceString, E_FILE_LINE);
    }
}

std::shared_ptr<const AttPDUMsg> GATTHandler::sendWithReply(const AttPDUMsg & msg, const int timeout) {
    send( msg );

    // Ringbuffer read is thread safe
    std::shared_ptr<const AttPDUMsg> res = attPDURing.getBlocking(timeout);
    if( nullptr == res ) {
        errno = ETIMEDOUT;
        ERR_PRINT("GATTHandler::send: nullptr result (timeout %d): req %s to %s", timeout, msg.toString().c_str(), deviceString.c_str());
        disconnect(true /* disconnectDevice */, true /* ioErrorCause */);
        throw BluetoothException("GATTHandler::send: nullptr result (timeout "+std::to_string(timeout)+"): req "+msg.toString()+" to "+deviceString, E_FILE_LINE);
    }
    return res;
}

uint16_t GATTHandler::exchangeMTU(const uint16_t clientMaxMTU, const int timeout) {
    /***
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.3.1 Exchange MTU (Server configuration)
     */
    if( clientMaxMTU > number(Defaults::MAX_ATT_MTU) ) {
        throw IllegalArgumentException("clientMaxMTU "+std::to_string(clientMaxMTU)+" > ClientMaxMTU "+std::to_string(number(Defaults::MAX_ATT_MTU)), E_FILE_LINE);
    }
    const AttExchangeMTU req(clientMaxMTU);
    const std::lock_guard<std::recursive_mutex> lock(mtx_command); // RAII-style acquire and relinquish via destructor
    PERF_TS_T0();

    uint16_t mtu = 0;
    DBG_PRINT("GATT send: %s", req.toString().c_str());

    std::shared_ptr<const AttPDUMsg> pdu = sendWithReply(req, timeout);
    if( nullptr != pdu ) {
        if( pdu->getOpcode() == AttPDUMsg::ATT_EXCHANGE_MTU_RSP ) {
            const AttExchangeMTU * p = static_cast<const AttExchangeMTU*>(pdu.get());
            mtu = p->getMTUSize();
        }
    } else {
        ERR_PRINT("GATT exchangeMTU send failed: %s - %s", req.toString().c_str(), deviceString.c_str());
    }
    PERF_TS_TD("GATT exchangeMTU");

    return mtu;
}

GATTCharacteristicRef GATTHandler::findCharacterisicsByValueHandle(const uint16_t charValueHandle) {
    return findCharacterisicsByValueHandle(charValueHandle, services);
}

GATTCharacteristicRef GATTHandler::findCharacterisicsByValueHandle(const uint16_t charValueHandle, std::vector<GATTServiceRef> &services) {
    for(auto it = services.begin(); it != services.end(); it++) {
        GATTCharacteristicRef decl = findCharacterisicsByValueHandle(charValueHandle, *it);
        if( nullptr != decl ) {
            return decl;
        }
    }
    return nullptr;
}

GATTCharacteristicRef GATTHandler::findCharacterisicsByValueHandle(const uint16_t charValueHandle, GATTServiceRef service) {
    for(auto it = service->characteristicList.begin(); it != service->characteristicList.end(); it++) {
        GATTCharacteristicRef decl = *it;
        if( charValueHandle == decl->value_handle ) {
            return decl;
        }
    }
    return nullptr;
}

std::vector<GATTServiceRef> & GATTHandler::discoverCompletePrimaryServices() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_command); // RAII-style acquire and relinquish via destructor
    if( !discoverPrimaryServices(services) ) {
        return services;
    }
    for(auto it = services.begin(); it != services.end(); it++) {
        GATTServiceRef primSrv = *it;
        if( discoverCharacteristics(primSrv) ) {
            discoverDescriptors(primSrv);
        }
    }
    return services;
}

bool GATTHandler::discoverPrimaryServices(std::vector<GATTServiceRef> & result) {
    /***
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.4.1 Discover All Primary Services
     *
     * This sub-procedure is complete when the ATT_ERROR_RSP PDU is received
     * and the error code is set to Attribute Not Found or when the End Group Handle
     * in the Read by Type Group Response is 0xFFFF.
     */
    const uuid16_t groupType = uuid16_t(GattAttributeType::PRIMARY_SERVICE);
    const std::lock_guard<std::recursive_mutex> lock(mtx_command); // RAII-style acquire and relinquish via destructor
    PERF_TS_T0();

    std::shared_ptr<DBTDevice> device = getDevice();
    if( nullptr == device ) {
        ERR_PRINT("GATT discoverPrimary: Device destructed: %s", deviceString.c_str());
        return false;
    }
    bool done=false;
    uint16_t startHandle=0x0001;
    result.clear();
    while(!done) {
        const AttReadByNTypeReq req(true /* group */, startHandle, 0xffff, groupType);
        COND_PRINT(debug_data, "GATT PRIM SRV discover send: %s", req.toString().c_str());

        std::shared_ptr<const AttPDUMsg> pdu = sendWithReply(req, replyTimeoutMS);
        if( nullptr != pdu ) {
            COND_PRINT(debug_data, "GATT PRIM SRV discover recv: %s", pdu->toString().c_str());
            if( pdu->getOpcode() == AttPDUMsg::ATT_READ_BY_GROUP_TYPE_RSP ) {
                const AttReadByGroupTypeRsp * p = static_cast<const AttReadByGroupTypeRsp*>(pdu.get());
                const int count = p->getElementCount();

                for(int i=0; i<count; i++) {
                    const int ePDUOffset = p->getElementPDUOffset(i);
                    const int esz = p->getElementTotalSize();
                    result.push_back( GATTServiceRef( new GATTService( device, true,
                            p->pdu.get_uint16(ePDUOffset), // start-handle
                            p->pdu.get_uint16(ePDUOffset + 2), // end-handle
                            p->pdu.get_uuid( ePDUOffset + 2 + 2, uuid_t::toTypeSize(esz-2-2) ) // uuid
                        ) ) );
                    COND_PRINT(debug_data, "GATT PRIM SRV discovered[%d/%d]: %s", i, count, result.at(result.size()-1)->toString().c_str());
                }
                startHandle = p->getElementEndHandle(count-1);
                if( startHandle < 0xffff ) {
                    startHandle++;
                } else {
                    done = true; // OK by spec: End of communication
                }
            } else if( pdu->getOpcode() == AttPDUMsg::ATT_ERROR_RSP ) {
                done = true; // OK by spec: End of communication
            } else {
                WARN_PRINT("GATT discoverPrimary unexpected reply %s", pdu->toString().c_str());
                done = true;
            }
        } else {
            ERR_PRINT("GATT discoverPrimary send failed: %s - %s", req.toString().c_str(), deviceString.c_str());
            done = true; // send failed
        }
    }
    PERF_TS_TD("GATT discoverPrimaryServices");

    return result.size() > 0;
}

bool GATTHandler::discoverCharacteristics(GATTServiceRef & service) {
    /***
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.6.1 Discover All Characteristics of a Service
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.1 Characteristic Declaration Attribute Value
     * </p>
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration
     * </p>
     */
    const uuid16_t characteristicTypeReq = uuid16_t(GattAttributeType::CHARACTERISTIC);
    const std::lock_guard<std::recursive_mutex> lock(mtx_command); // RAII-style acquire and relinquish via destructor
    COND_PRINT(debug_data, "GATT discoverCharacteristics Service: %s", service->toString().c_str());

    PERF_TS_T0();

    bool done=false;
    uint16_t handle=service->startHandle;
    service->characteristicList.clear();
    while(!done) {
        const AttReadByNTypeReq req(false /* group */, handle, service->endHandle, characteristicTypeReq);
        COND_PRINT(debug_data, "GATT C discover send: %s", req.toString().c_str());

        std::shared_ptr<const AttPDUMsg> pdu = sendWithReply(req, replyTimeoutMS);
        if( nullptr != pdu ) {
            COND_PRINT(debug_data, "GATT C discover recv: %s", pdu->toString().c_str());
            if( pdu->getOpcode() == AttPDUMsg::ATT_READ_BY_TYPE_RSP ) {
                const AttReadByTypeRsp * p = static_cast<const AttReadByTypeRsp*>(pdu.get());
                const int e_count = p->getElementCount();

                for(int e_iter=0; e_iter<e_count; e_iter++) {
                    // handle: handle for the Characteristics declaration
                    // value: Characteristics Property, Characteristics Value Handle _and_ Characteristics UUID
                    const int ePDUOffset = p->getElementPDUOffset(e_iter);
                    const int esz = p->getElementTotalSize();
                    service->characteristicList.push_back( GATTCharacteristicRef( new GATTCharacteristic(
                        service,
                        p->pdu.get_uint16(ePDUOffset), // Characteristics's Service Handle
                        p->getElementHandle(e_iter), // Characteristic Handle
                        static_cast<GATTCharacteristic::PropertyBitVal>(p->pdu.get_uint8(ePDUOffset  + 2)), // Characteristics Property
                        p->pdu.get_uint16(ePDUOffset + 2 + 1), // Characteristics Value Handle
                        p->pdu.get_uuid(ePDUOffset   + 2 + 1 + 2, uuid_t::toTypeSize(esz-2-1-2) ) ) ) ); // Characteristics Value Type UUID
                    COND_PRINT(debug_data, "GATT C discovered[%d/%d]: %s", e_iter, e_count, service->characteristicList.at(service->characteristicList.size()-1)->toString().c_str());
                }
                handle = p->getElementHandle(e_count-1); // Last Characteristic Handle
                if( handle < service->endHandle ) {
                    handle++;
                } else {
                    done = true; // OK by spec: End of communication
                }
            } else if( pdu->getOpcode() == AttPDUMsg::ATT_ERROR_RSP ) {
                done = true; // OK by spec: End of communication
            } else {
                WARN_PRINT("GATT discoverCharacteristics unexpected reply %s", pdu->toString().c_str());
                done = true;
            }
        } else {
            ERR_PRINT("GATT discoverCharacteristics send failed: %s - %s", req.toString().c_str(), deviceString.c_str());
            service->characteristicList.clear();
            done = true;
        }
    }

    PERF_TS_TD("GATT discoverCharacteristics");

    return service->characteristicList.size() > 0;
}

bool GATTHandler::discoverDescriptors(GATTServiceRef & service) {
    /***
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.7.1 Discover All Characteristic Descriptors
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.1 Characteristic Declaration Attribute Value
     * </p>
     */
    COND_PRINT(debug_data, "GATT discoverDescriptors Service: %s", service->toString().c_str());
    const std::lock_guard<std::recursive_mutex> lock(mtx_command); // RAII-style acquire and relinquish via destructor
    PERF_TS_T0();

    bool done=false;
    const int charCount = service->characteristicList.size();
    for(int charIter=0; !done && charIter < charCount; charIter++ ) {
        GATTCharacteristicRef charDecl = service->characteristicList[charIter];
        charDecl->clearDescriptors();
        COND_PRINT(debug_data, "GATT discoverDescriptors Characteristic[%d/%d]: %s", charIter, charCount, charDecl->toString().c_str());

        uint16_t cd_handle_iter = charDecl->value_handle + 1; // Start @ Characteristic Value Handle + 1
        uint16_t cd_handle_end;
        if( charIter+1 < charCount ) {
            cd_handle_end = service->characteristicList.at(charIter+1)->value_handle;
        } else {
            cd_handle_end = service->endHandle;
        }

        while( !done && cd_handle_iter <= cd_handle_end ) {
            const AttFindInfoReq req(cd_handle_iter, cd_handle_end);
            COND_PRINT(debug_data, "GATT CD discover send: %s", req.toString().c_str());

            std::shared_ptr<const AttPDUMsg> pdu = sendWithReply(req, replyTimeoutMS);
            if( nullptr == pdu ) {
                ERR_PRINT("GATT discoverDescriptors send failed: %s - %s", req.toString().c_str(), deviceString.c_str());
                done = true;
                break;
            }
            COND_PRINT(debug_data, "GATT CD discover recv: %s", pdu->toString().c_str());

            if( pdu->getOpcode() == AttPDUMsg::ATT_FIND_INFORMATION_RSP ) {
                const AttFindInfoRsp * p = static_cast<const AttFindInfoRsp*>(pdu.get());
                const int e_count = p->getElementCount();

                for(int e_iter=0; e_iter<e_count; e_iter++) {
                    // handle: handle of Characteristic Descriptor.
                    // value: Characteristic Descriptor UUID.
                    const uint16_t cd_handle = p->getElementHandle(e_iter);
                    const std::shared_ptr<const uuid_t> cd_uuid = p->getElementValue(e_iter);

                    std::shared_ptr<GATTDescriptor> cd( new GATTDescriptor(charDecl, cd_uuid, cd_handle) );
                    if( cd_handle <= charDecl->value_handle || cd_handle > cd_handle_end ) { // should never happen!
                        ERR_PRINT("GATT discoverDescriptors CD handle %s not in range ]%s..%s]: %s - %s",
                                uint16HexString(cd_handle).c_str(),
                                uint16HexString(charDecl->value_handle).c_str(), uint16HexString(cd_handle_end).c_str(),
                                cd->toString().c_str(), deviceString.c_str());
                        done = true;
                        break;

                    }
                    if( !readDescriptorValue(*cd, 0) ) {
                        ERR_PRINT("GATT discoverDescriptors readDescriptorValue failed: %s . %s - %s",
                                req.toString().c_str(), cd->toString().c_str(), deviceString.c_str());
                        done = true;
                        break;
                    }
                    if( cd->isClientCharacteristicConfiguration() ) {
                        charDecl->clientCharacteristicsConfigIndex = charDecl->descriptorList.size();
                    }
                    charDecl->descriptorList.push_back(cd);
                    COND_PRINT(debug_data, "GATT CD discovered[%d/%d]: %s", e_iter, e_count, cd->toString().c_str());
                }
                cd_handle_iter = p->getElementHandle(e_count-1); // Last Descriptor Handle
                if( cd_handle_iter < cd_handle_end ) {
                    cd_handle_iter++;
                } else {
                    done = true; // OK by spec: End of communication
                }
            } else if( pdu->getOpcode() == AttPDUMsg::ATT_ERROR_RSP ) {
                done = true; // OK by spec: End of communication
            } else {
                WARN_PRINT("GATT discoverDescriptors unexpected opcode reply %s", pdu->toString().c_str());
                done = true;
            }
        }
    }
    PERF_TS_TD("GATT discoverDescriptors");

    return service->characteristicList.size() > 0;
}

bool GATTHandler::readDescriptorValue(GATTDescriptor & desc, int expectedLength) {
    COND_PRINT(debug_data, "GATTHandler::readDescriptorValue expLen %d, desc %s", expectedLength, desc.toString().c_str());
    return readValue(desc.handle, desc.value, expectedLength);
}

bool GATTHandler::readCharacteristicValue(const GATTCharacteristic & decl, POctets & res, int expectedLength) {
    COND_PRINT(debug_data, "GATTHandler::readCharacteristicValue expLen %d, decl %s", expectedLength, decl.toString().c_str());
    return readValue(decl.value_handle, res, expectedLength);
}

bool GATTHandler::readValue(const uint16_t handle, POctets & res, int expectedLength) {
    /* BT Core Spec v5.2: Vol 3, Part G GATT: 4.8.1 Read Characteristic Value */
    /* BT Core Spec v5.2: Vol 3, Part G GATT: 4.8.3 Read Long Characteristic Value */
    const std::lock_guard<std::recursive_mutex> lock(mtx_command); // RAII-style acquire and relinquish via destructor
    PERF2_TS_T0();

    bool done=false;
    int offset=0;

    COND_PRINT(debug_data, "GATTHandler::readValue expLen %d, handle %s", expectedLength, uint16HexString(handle).c_str());

    while(!done) {
        if( 0 < expectedLength && expectedLength <= offset ) {
            break; // done
        } else if( 0 == expectedLength && 0 < offset ) {
            break; // done w/ only one request
        } // else 0 > expectedLength: implicit

        std::shared_ptr<const AttPDUMsg> pdu = nullptr;

        if( 0 == offset ) {
            const AttReadReq req (handle);
            COND_PRINT(debug_data, "GATT RV send: %s", req.toString().c_str());
            pdu = sendWithReply(req, replyTimeoutMS);
        } else {
            const AttReadBlobReq req (handle, offset);
            COND_PRINT(debug_data, "GATT RV send: %s", req.toString().c_str());
            pdu = sendWithReply(req, replyTimeoutMS);
        }

        if( nullptr != pdu ) {
            COND_PRINT(debug_data, "GATT RV recv: %s", pdu->toString().c_str());
            if( pdu->getOpcode() == AttPDUMsg::ATT_READ_RSP ) {
                const AttReadRsp * p = static_cast<const AttReadRsp*>(pdu.get());
                const TOctetSlice & v = p->getValue();
                res += v;
                offset += v.getSize();
                if( p->getPDUValueSize() < p->getMaxPDUValueSize(usedMTU) ) {
                    done = true; // No full ATT_MTU PDU used - end of communication
                }
            } else if( pdu->getOpcode() == AttPDUMsg::ATT_READ_BLOB_RSP ) {
                const AttReadBlobRsp * p = static_cast<const AttReadBlobRsp*>(pdu.get());
                const TOctetSlice & v = p->getValue();
                if( 0 == v.getSize() ) {
                    done = true; // OK by spec: No more data - end of communication
                } else {
                    res += v;
                    offset += v.getSize();
                    if( p->getPDUValueSize() < p->getMaxPDUValueSize(usedMTU) ) {
                        done = true; // No full ATT_MTU PDU used - end of communication
                    }
                }
            } else if( pdu->getOpcode() == AttPDUMsg::ATT_ERROR_RSP ) {
                /**
                 * BT Core Spec v5.2: Vol 3, Part G GATT: 4.8.3 Read Long Characteristic Value
                 *
                 * If the Characteristic Value is not longer than (ATT_MTU – 1)
                 * an ATT_ERROR_RSP PDU with the error
                 * code set to Attribute Not Long shall be received on the first
                 * ATT_READ_BLOB_REQ PDU.
                 */
                const AttErrorRsp * p = static_cast<const AttErrorRsp *>(pdu.get());
                if( AttErrorRsp::ATTRIBUTE_NOT_LONG == p->getErrorCode() ) {
                    done = true; // OK by spec: No more data - end of communication
                } else {
                    WARN_PRINT("GATT readValue unexpected error %s", pdu->toString().c_str());
                    done = true;
                }
            } else {
                WARN_PRINT("GATT readValue unexpected reply %s", pdu->toString().c_str());
                done = true;
            }
        } else {
            ERR_PRINT("GATT readValue send failed: handle %u, offset %d: %s", handle, offset, deviceString.c_str());
            done = true;
        }
    }
    PERF2_TS_TD("GATT readValue");

    return offset > 0;
}

bool GATTHandler::writeDescriptorValue(const GATTDescriptor & cd) {
    /* BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration */
    /* BT Core Spec v5.2: Vol 3, Part G GATT: 4.9.3 Write Characteristic Value */
    /* BT Core Spec v5.2: Vol 3, Part G GATT: 4.11 Characteristic Value Indication */
    /* BT Core Spec v5.2: Vol 3, Part G GATT: 4.12.3 Write Characteristic Descriptor */
    COND_PRINT(debug_data, "GATTHandler::writeDesccriptorValue desc %s", cd.toString().c_str());
    return writeValue(cd.handle, cd.value, true);
}

bool GATTHandler::writeCharacteristicValue(const GATTCharacteristic & c, const TROOctets & value) {
    /* BT Core Spec v5.2: Vol 3, Part G GATT: 4.9.3 Write Characteristic Value */
    COND_PRINT(debug_data, "GATTHandler::writeCharacteristicValue desc %s, value %s", c.toString().c_str(), value.toString().c_str());
    return writeValue(c.value_handle, value, true);
}

bool GATTHandler::writeCharacteristicValueNoResp(const GATTCharacteristic & c, const TROOctets & value) {
    COND_PRINT(debug_data, "GATT writeCharacteristicValueNoResp decl %s, value %s", c.toString().c_str(), value.toString().c_str());
    return writeValue(c.value_handle, value, false);
}

bool GATTHandler::writeValue(const uint16_t handle, const TROOctets & value, const bool expResponse) {
    /* BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration */
    /* BT Core Spec v5.2: Vol 3, Part G GATT: 4.9.3 Write Characteristic Value */
    /* BT Core Spec v5.2: Vol 3, Part G GATT: 4.11 Characteristic Value Indication */
    /* BT Core Spec v5.2: Vol 3, Part G GATT: 4.12.3 Write Characteristic Descriptor */

    if( value.getSize() <= 0 ) {
        WARN_PRINT("GATT writeValue size <= 0, no-op: %s", value.toString().c_str());
        return false;
    }
    const std::lock_guard<std::recursive_mutex> lock(mtx_command); // RAII-style acquire and relinquish via destructor

    // TODO: Long Value!
    PERF2_TS_T0();

    AttWriteReq req(handle, value);
    COND_PRINT(debug_data, "GATT WV send(resp %d): %s", expResponse, req.toString().c_str());

    if( !expResponse ) {
        send( req );
        return true;
    }

    bool res = false;
    std::shared_ptr<const AttPDUMsg> pdu = sendWithReply(req, replyTimeoutMS);
    if( nullptr != pdu ) {
        COND_PRINT(debug_data, "GATT WV recv: %s", pdu->toString().c_str());
        if( pdu->getOpcode() == AttPDUMsg::ATT_WRITE_RSP ) {
            // OK
            res = true;
        } else if( pdu->getOpcode() == AttPDUMsg::ATT_ERROR_RSP ) {
            const AttErrorRsp * p = static_cast<const AttErrorRsp *>(pdu.get());
            WARN_PRINT("GATT writeValue unexpected error %s", p->toString().c_str());
        } else {
            WARN_PRINT("GATT writeValue unexpected reply %s", pdu->toString().c_str());
        }
    } else {
        ERR_PRINT("GATT writeValue send failed: handle %u: %s", handle, deviceString.c_str());
    }
    PERF2_TS_TD("GATT writeValue");
    return res;
}

bool GATTHandler::configNotificationIndication(GATTDescriptor & cccd, const bool enableNotification, const bool enableIndication) {
    if( !cccd.isClientCharacteristicConfiguration() ) {
        throw IllegalArgumentException("Not a ClientCharacteristicConfiguration: "+cccd.toString(), E_FILE_LINE);
    }
    /* BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration */
    const uint16_t ccc_value = enableNotification | ( enableIndication << 1 );
    COND_PRINT(debug_data, "GATTHandler::configNotificationIndication decl %s, enableNotification %d, enableIndication %d",
            cccd.toString().c_str(), enableNotification, enableIndication);
    cccd.value.resize(2, 2);
    cccd.value.put_uint16(0, ccc_value);
    try {
        return writeDescriptorValue(cccd);
    } catch (BluetoothException & bte) {
        if( !enableNotification && !enableIndication ) {
            // OK to have lost connection @ disable
            INFO_PRINT("GATTHandler::configNotificationIndication(disable) on %s caught exception: %s", deviceString.c_str(), bte.what());
            return false;
        } else {
            throw; // re-throw current exception
        }
    }
}

/*********************************************************************************************************************/
/*********************************************************************************************************************/
/*********************************************************************************************************************/

static const uuid16_t _GENERIC_ACCESS(GattServiceType::GENERIC_ACCESS);
static const uuid16_t _DEVICE_NAME(GattCharacteristicType::DEVICE_NAME);
static const uuid16_t _APPEARANCE(GattCharacteristicType::APPEARANCE);
static const uuid16_t _PERIPHERAL_PREFERRED_CONNECTION_PARAMETERS(GattCharacteristicType::PERIPHERAL_PREFERRED_CONNECTION_PARAMETERS);

static const uuid16_t _DEVICE_INFORMATION(GattServiceType::DEVICE_INFORMATION);
static const uuid16_t _SYSTEM_ID(GattCharacteristicType::SYSTEM_ID);
static const uuid16_t _MODEL_NUMBER_STRING(GattCharacteristicType::MODEL_NUMBER_STRING);
static const uuid16_t _SERIAL_NUMBER_STRING(GattCharacteristicType::SERIAL_NUMBER_STRING);
static const uuid16_t _FIRMWARE_REVISION_STRING(GattCharacteristicType::FIRMWARE_REVISION_STRING);
static const uuid16_t _HARDWARE_REVISION_STRING(GattCharacteristicType::HARDWARE_REVISION_STRING);
static const uuid16_t _SOFTWARE_REVISION_STRING(GattCharacteristicType::SOFTWARE_REVISION_STRING);
static const uuid16_t _MANUFACTURER_NAME_STRING(GattCharacteristicType::MANUFACTURER_NAME_STRING);
static const uuid16_t _REGULATORY_CERT_DATA_LIST(GattCharacteristicType::REGULATORY_CERT_DATA_LIST);
static const uuid16_t _PNP_ID(GattCharacteristicType::PNP_ID);

std::shared_ptr<GenericAccess> GATTHandler::getGenericAccess(std::vector<GATTCharacteristicRef> & genericAccessCharDeclList) {
    std::shared_ptr<GenericAccess> res = nullptr;
    POctets value(number(Defaults::MAX_ATT_MTU), 0);
    std::string deviceName = "";
    AppearanceCat appearance = AppearanceCat::UNKNOWN;
    PeriphalPreferredConnectionParameters * prefConnParam = nullptr;

    const std::lock_guard<std::recursive_mutex> lock(mtx_command); // RAII-style acquire and relinquish via destructor

    for(size_t i=0; i<genericAccessCharDeclList.size(); i++) {
        const GATTCharacteristic & charDecl = *genericAccessCharDeclList.at(i);
        std::shared_ptr<GATTService> service = charDecl.getServiceUnchecked();
        if( nullptr == service || _GENERIC_ACCESS != *(service->type) ) {
        	continue;
        }
        if( _DEVICE_NAME == *charDecl.value_type ) {
            if( readCharacteristicValue(charDecl, value.resize(0)) ) {
            	deviceName = GattNameToString(value);
            }
        } else if( _APPEARANCE == *charDecl.value_type ) {
            if( readCharacteristicValue(charDecl, value.resize(0)) ) {
            	appearance = static_cast<AppearanceCat>(value.get_uint16(0));
            }
        } else if( _PERIPHERAL_PREFERRED_CONNECTION_PARAMETERS == *charDecl.value_type ) {
            if( readCharacteristicValue(charDecl, value.resize(0)) ) {
            	prefConnParam = new PeriphalPreferredConnectionParameters(value);
            }
        }
    }
    if( deviceName.size() > 0 && nullptr != prefConnParam ) {
    	res = std::shared_ptr<GenericAccess>(new GenericAccess(deviceName, appearance, *prefConnParam));
    }
    if( nullptr != prefConnParam ) {
        delete prefConnParam;
    }
    return res;
}

std::shared_ptr<GenericAccess> GATTHandler::getGenericAccess(std::vector<GATTServiceRef> & primServices) {
	std::shared_ptr<GenericAccess> res = nullptr;
	for(size_t i=0; i<primServices.size() && nullptr == res; i++) {
		res = getGenericAccess(primServices.at(i)->characteristicList);
	}
	return res;
}

bool GATTHandler::ping() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_command); // RAII-style acquire and relinquish via destructor

    for(size_t i=0; i<services.size(); i++) {
        std::vector<GATTCharacteristicRef> & genericAccessCharDeclList = services.at(i)->characteristicList;
        POctets value(32, 0);

        for(size_t i=0; i<genericAccessCharDeclList.size(); i++) {
            const GATTCharacteristic & charDecl = *genericAccessCharDeclList.at(i);
            std::shared_ptr<GATTService> service = charDecl.getServiceUnchecked();
            if( nullptr == service || _GENERIC_ACCESS != *(service->type) ) {
                continue;
            }
            if( _APPEARANCE == *charDecl.value_type ) {
                if( readCharacteristicValue(charDecl, value.resize(0)) ) {
                    return true;
                }
            }
        }
    }
    return false;
}

std::shared_ptr<DeviceInformation> GATTHandler::getDeviceInformation(std::vector<GATTCharacteristicRef> & characteristicDeclList) {
    std::shared_ptr<DeviceInformation> res = nullptr;
    POctets value(number(Defaults::MAX_ATT_MTU), 0);

    POctets systemID(8, 0);
    std::string modelNumber;
    std::string serialNumber;
    std::string firmwareRevision;
    std::string hardwareRevision;
    std::string softwareRevision;
    std::string manufacturer;
    POctets regulatoryCertDataList(128, 0);
    PnP_ID * pnpID = nullptr;
    bool found = false;

    const std::lock_guard<std::recursive_mutex> lock(mtx_command); // RAII-style acquire and relinquish via destructor

    for(size_t i=0; i<characteristicDeclList.size(); i++) {
        const GATTCharacteristic & charDecl = *characteristicDeclList.at(i);
        std::shared_ptr<GATTService> service = charDecl.getServiceUnchecked();
        if( nullptr == service || _DEVICE_INFORMATION != *(service->type) ) {
            continue;
        }
        found = true;
        if( _SYSTEM_ID == *charDecl.value_type ) {
            if( readCharacteristicValue(charDecl, systemID.resize(0)) ) {
                // nop
            }
        } else if( _REGULATORY_CERT_DATA_LIST == *charDecl.value_type ) {
            if( readCharacteristicValue(charDecl, regulatoryCertDataList.resize(0)) ) {
                // nop
            }
        } else if( _PNP_ID == *charDecl.value_type ) {
            if( readCharacteristicValue(charDecl, value.resize(0)) ) {
                pnpID = new PnP_ID(value);
            }
        } else if( _MODEL_NUMBER_STRING == *charDecl.value_type ) {
            if( readCharacteristicValue(charDecl, value.resize(0)) ) {
                modelNumber = GattNameToString(value);
            }
        } else if( _SERIAL_NUMBER_STRING == *charDecl.value_type ) {
            if( readCharacteristicValue(charDecl, value.resize(0)) ) {
                serialNumber = GattNameToString(value);
            }
        } else if( _FIRMWARE_REVISION_STRING == *charDecl.value_type ) {
            if( readCharacteristicValue(charDecl, value.resize(0)) ) {
                firmwareRevision = GattNameToString(value);
            }
        } else if( _HARDWARE_REVISION_STRING == *charDecl.value_type ) {
            if( readCharacteristicValue(charDecl, value.resize(0)) ) {
                hardwareRevision = GattNameToString(value);
            }
        } else if( _SOFTWARE_REVISION_STRING == *charDecl.value_type ) {
            if( readCharacteristicValue(charDecl, value.resize(0)) ) {
                softwareRevision = GattNameToString(value);
            }
        } else if( _MANUFACTURER_NAME_STRING == *charDecl.value_type ) {
            if( readCharacteristicValue(charDecl, value.resize(0)) ) {
                manufacturer = GattNameToString(value);
            }
        }
    }
    if( nullptr == pnpID ) {
        pnpID = new PnP_ID();
    }
    if( found ) {
        res = std::shared_ptr<DeviceInformation>(new DeviceInformation(systemID, modelNumber, serialNumber,
                                                      firmwareRevision, hardwareRevision, softwareRevision,
                                                      manufacturer, regulatoryCertDataList, *pnpID) );
    }
    delete pnpID;
    return res;
}

std::shared_ptr<DeviceInformation> GATTHandler::getDeviceInformation(std::vector<GATTServiceRef> & primServices) {
    std::shared_ptr<DeviceInformation> res = nullptr;
    for(size_t i=0; i<primServices.size() && nullptr == res; i++) {
        res = getDeviceInformation(primServices.at(i)->characteristicList);
    }
    return res;
}

