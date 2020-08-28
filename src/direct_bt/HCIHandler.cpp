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

// #define PERF_PRINT_ON 1
#include <dbt_debug.hpp>

#include "DBTEnv.hpp"

#include "BTIoctl.hpp"

#include "HCIIoctl.hpp"
#include "HCIComm.hpp"
#include "HCIHandler.hpp"
#include "DBTTypes.hpp"
#include "BasicAlgos.hpp"

extern "C" {
    #include <inttypes.h>
    #include <unistd.h>
    #include <poll.h>
    #include <signal.h>
}

using namespace direct_bt;

HCIEnv::HCIEnv()
: exploding( DBTEnv::getExplodingProperties("direct_bt.hci") ),
  HCI_READER_THREAD_POLL_TIMEOUT( DBTEnv::getInt32Property("direct_bt.hci.reader.timeout", 10000, 1500 /* min */, INT32_MAX /* max */) ),
  HCI_COMMAND_STATUS_REPLY_TIMEOUT( DBTEnv::getInt32Property("direct_bt.hci.cmd.status.timeout", 3000, 1500 /* min */, INT32_MAX /* max */) ),
  HCI_COMMAND_COMPLETE_REPLY_TIMEOUT( DBTEnv::getInt32Property("direct_bt.hci.cmd.complete.timeout", 10000, 1500 /* min */, INT32_MAX /* max */) ),
  HCI_EVT_RING_CAPACITY( DBTEnv::getInt32Property("direct_bt.hci.ringsize", 64, 64 /* min */, 1024 /* max */) ),
  DEBUG_EVENT( DBTEnv::getBooleanProperty("direct_bt.debug.hci.event", false) ),
  HCI_READ_PACKET_MAX_RETRY( HCI_EVT_RING_CAPACITY )
{
}

const pid_t HCIHandler::pidSelf = getpid();

struct hci_rp_status {
    __u8    status;
} __packed;

HCIConnectionRef HCIHandler::addOrUpdateTrackerConnection(const EUI48 & address, BDAddressType addrType, const uint16_t handle) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_connectionList); // RAII-style acquire and relinquish via destructor
    // remove all old entry with given address first
    for (auto it = connectionList.begin(); it != connectionList.end(); ) {
        HCIConnectionRef conn = *it;
        if ( conn->equals(address, addrType) ) {
            // reuse same entry
            INFO_PRINT("HCIHandler::addTrackerConnection: address[%s, %s], handle %s: reuse entry %s",
               address.toString().c_str(), getBDAddressTypeString(addrType).c_str(), uint16HexString(handle).c_str(), conn->toString().c_str());
            // Overwrite tracked connection handle with given _valid_ handle only, i.e. non zero!
            if( 0 != handle ) {
                if( 0 != conn->getHandle() && handle != conn->getHandle() ) {
                    WARN_PRINT("HCIHandler::addTrackerConnection: address[%s, %s], handle %s: reusing entry %s, overwriting non-zero handle",
                       address.toString().c_str(), getBDAddressTypeString(addrType).c_str(), uint16HexString(handle).c_str(), conn->toString().c_str());
                }
                conn->setHandle( handle );
            }
            return conn; // done
        } else {
            ++it;
        }
    }
    HCIConnectionRef res( new HCIConnection(address, addrType, handle) );
    connectionList.push_back( res );
    return res;
}

HCIConnectionRef HCIHandler::findTrackerConnection(const EUI48 & address, BDAddressType addrType) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_connectionList); // RAII-style acquire and relinquish via destructor
    const size_t size = connectionList.size();
    for (size_t i = 0; i < size; i++) {
        HCIConnectionRef & e = connectionList[i];
        if( e->equals(address, addrType) ) {
            return e;
        }
    }
    return nullptr;
}

HCIConnectionRef HCIHandler::findTrackerConnection(const uint16_t handle) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_connectionList); // RAII-style acquire and relinquish via destructor
    const size_t size = connectionList.size();
    for (size_t i = 0; i < size; i++) {
        HCIConnectionRef & e = connectionList[i];
        if ( handle == e->getHandle() ) {
            return e;
        }
    }
    return nullptr;
}

HCIConnectionRef HCIHandler::removeTrackerConnection(const HCIConnectionRef conn) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_connectionList); // RAII-style acquire and relinquish via destructor
    for (auto it = connectionList.begin(); it != connectionList.end(); ) {
        HCIConnectionRef e = *it;
        if ( *e == *conn ) {
            it = connectionList.erase(it); // old entry
            return e; // done
        } else {
            ++it;
        }
    }
    return nullptr;
}

HCIConnectionRef HCIHandler::removeTrackerConnection(const uint16_t handle) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_connectionList); // RAII-style acquire and relinquish via destructor
    for (auto it = connectionList.begin(); it != connectionList.end(); ) {
        HCIConnectionRef e = *it;
        if ( e->getHandle() == handle ) {
            it = connectionList.erase(it); // old entry
            return e; // done
        } else {
            ++it;
        }
    }
    return nullptr;
}

MgmtEvent::Opcode HCIHandler::translate(HCIEventType evt, HCIMetaEventType met) {
    if( HCIEventType::LE_META == evt ) {
        switch( met ) {
            case HCIMetaEventType::LE_CONN_COMPLETE: return MgmtEvent::Opcode::DEVICE_CONNECTED;
            default: return MgmtEvent::Opcode::INVALID;
        }
    }
    switch( evt ) {
        case HCIEventType::CONN_COMPLETE: return MgmtEvent::Opcode::DEVICE_CONNECTED;
        case HCIEventType::DISCONN_COMPLETE: return MgmtEvent::Opcode::DEVICE_DISCONNECTED;
        case HCIEventType::CMD_COMPLETE: return MgmtEvent::Opcode::CMD_COMPLETE;
        case HCIEventType::CMD_STATUS: return MgmtEvent::Opcode::CMD_STATUS;
        default: return MgmtEvent::Opcode::INVALID;
    }
}

std::shared_ptr<MgmtEvent> HCIHandler::translate(std::shared_ptr<HCIEvent> ev) {
    const HCIEventType evt = ev->getEventType();
    const HCIMetaEventType mevt = ev->getMetaEventType();

    if( HCIEventType::LE_META == evt ) {
        switch( mevt ) {
            case HCIMetaEventType::LE_CONN_COMPLETE: {
                HCIStatusCode status;
                const hci_ev_le_conn_complete * ev_cc = getMetaReplyStruct<hci_ev_le_conn_complete>(ev, mevt, &status);
                if( nullptr == ev_cc ) {
                    ERR_PRINT("HCIHandler::translate(reader): LE_CONN_COMPLETE: Null reply-struct: %s", ev->toString().c_str());
                    return nullptr;
                }
                const HCILEPeerAddressType hciAddrType = static_cast<HCILEPeerAddressType>(ev_cc->bdaddr_type);
                const BDAddressType addrType = getBDAddressType(hciAddrType);
                HCIConnectionRef conn = addOrUpdateTrackerConnection(ev_cc->bdaddr, addrType, ev_cc->handle);
                if( HCIStatusCode::SUCCESS == status ) {
                    return std::shared_ptr<MgmtEvent>( new MgmtEvtDeviceConnected(dev_id, ev_cc->bdaddr, addrType, ev_cc->handle) );
                } else {
                    std::shared_ptr<MgmtEvent> res( new MgmtEvtDeviceConnectFailed(dev_id, ev_cc->bdaddr, addrType, status) );
                    removeTrackerConnection(conn);
                    return res;
                }
            }
            default:
                return nullptr;
        }
    }
    switch( evt ) {
        case HCIEventType::CONN_COMPLETE: {
            HCIStatusCode status;
            const hci_ev_conn_complete * ev_cc = getReplyStruct<hci_ev_conn_complete>(ev, evt, &status);
            if( nullptr == ev_cc ) {
                ERR_PRINT("HCIHandler::translate(reader): CONN_COMPLETE: Null reply-struct: %s", ev->toString().c_str());
                return nullptr;
            }
            HCIConnectionRef conn = addOrUpdateTrackerConnection(ev_cc->bdaddr, BDAddressType::BDADDR_BREDR, ev_cc->handle);
            if( HCIStatusCode::SUCCESS == status ) {
                return std::shared_ptr<MgmtEvent>( new MgmtEvtDeviceConnected(dev_id, conn->getAddress(), conn->getAddressType(), conn->getHandle()) );
            } else {
                std::shared_ptr<MgmtEvent> res( new MgmtEvtDeviceConnectFailed(dev_id, conn->getAddress(), conn->getAddressType(), status) );
                removeTrackerConnection(conn);
                return res;
            }
        }
        case HCIEventType::DISCONN_COMPLETE: {
            HCIStatusCode status;
            const hci_ev_disconn_complete * ev_cc = getReplyStruct<hci_ev_disconn_complete>(ev, evt, &status);
            if( nullptr == ev_cc ) {
                ERR_PRINT("HCIHandler::translate(reader): DISCONN_COMPLETE: Null reply-struct: %s", ev->toString().c_str());
                return nullptr;
            }
            HCIConnectionRef conn = removeTrackerConnection(ev_cc->handle);
            if( nullptr == conn ) {
                INFO_PRINT("HCIHandler::translate(reader): DISCONN_COMPLETE: Not tracked handle %s: %s",
                           uint16HexString(ev_cc->handle).c_str(), ev->toString().c_str());
                return nullptr;
            } else {
                if( HCIStatusCode::SUCCESS != status ) {
                    // FIXME: Ever occuring? Still sending out essential disconnect event!
                    ERR_PRINT("HCIHandler::translate(reader): DISCONN_COMPLETE: !SUCCESS[%s, %s], %s: %s",
                            uint8HexString(static_cast<uint8_t>(status)).c_str(), getHCIStatusCodeString(status).c_str(),
                            conn->toString().c_str(), ev->toString().c_str());
                }
                const HCIStatusCode hciRootReason = static_cast<HCIStatusCode>(ev_cc->reason);
                return std::shared_ptr<MgmtEvent>( new MgmtEvtDeviceDisconnected(dev_id, conn->getAddress(), conn->getAddressType(), hciRootReason, conn->getHandle()) );
            }
        }
        default:
            return nullptr;
    }
}

void HCIHandler::hciReaderThreadImpl() {
    {
        const std::lock_guard<std::mutex> lock(mtx_hciReaderInit); // RAII-style acquire and relinquish via destructor
        hciReaderShallStop = false;
        hciReaderRunning = true;
        DBG_PRINT("HCIHandler::reader: Started");
        cv_hciReaderInit.notify_all();
    }

    while( !hciReaderShallStop ) {
        int len;
        if( !comm.isOpen() ) {
            // not open
            ERR_PRINT("HCIHandler::reader: Not connected");
            hciReaderShallStop = true;
            break;
        }

        len = comm.read(rbuffer.get_wptr(), rbuffer.getSize(), env.HCI_READER_THREAD_POLL_TIMEOUT);
        if( 0 < len ) {
            const uint16_t paramSize = len >= 3 ? rbuffer.get_uint8(2) : 0;
            if( len < number(HCIConstU8::EVENT_HDR_SIZE) + paramSize ) {
                WARN_PRINT("HCIHandler::reader: length mismatch %d < %d + %d",
                        len, number(HCIConstU8::EVENT_HDR_SIZE), paramSize);
                continue; // discard data
            }
            std::shared_ptr<HCIEvent> event( HCIEvent::getSpecialized(rbuffer.get_ptr(), len) );
            if( nullptr == event ) {
                // not an event ...
                ERR_PRINT("HCIHandler-IO RECV Drop (non-event) %s", bytesHexString(rbuffer.get_ptr(), 0, len, true /* lsbFirst*/).c_str());
                continue;
            }

            const HCIMetaEventType mec = event->getMetaEventType();
            if( HCIMetaEventType::INVALID != mec && !filter_test_metaev(mec) ) {
                // DROP
                COND_PRINT(env.DEBUG_EVENT, "HCIHandler-IO RECV Drop (meta filter) %s", event->toString().c_str());
                continue; // next packet
            }

            if( event->isEvent(HCIEventType::CMD_STATUS) || event->isEvent(HCIEventType::CMD_COMPLETE) )
            {
                COND_PRINT(env.DEBUG_EVENT, "HCIHandler-IO RECV (CMD) %s", event->toString().c_str());
                if( hciEventRing.isFull() ) {
                    const int dropCount = hciEventRing.capacity()/4;
                    hciEventRing.drop(dropCount);
                    WARN_PRINT("HCIHandler-IO RECV Drop (%d oldest elements of %d capacity, ring full)", dropCount, hciEventRing.capacity());
                }
                hciEventRing.putBlocking( event );
            } else if( event->isMetaEvent(HCIMetaEventType::LE_ADVERTISING_REPORT) ) {
                // issue callbacks for the translated AD events
                std::vector<std::shared_ptr<EInfoReport>> eirlist = EInfoReport::read_ad_reports(event->getParam(), event->getParamSize());
                int i=0;
                for_each_idx(eirlist, [&](std::shared_ptr<EInfoReport> &eir) {
                    // COND_PRINT(env.DEBUG_EVENT, "HCIHandler-IO RECV (AD EIR) %s", eir->toString().c_str());
                    std::shared_ptr<MgmtEvent> mevent( new MgmtEvtDeviceFound(dev_id, eir) );
                    sendMgmtEvent( mevent );
                    i++;
                });
            } else {
                // issue a callback for the translated event
                std::shared_ptr<MgmtEvent> mevent = translate(event);
                if( nullptr != mevent ) {
                    COND_PRINT(env.DEBUG_EVENT, "HCIHandler-IO RECV (CB) %s", event->toString().c_str());
                    sendMgmtEvent( mevent );
                } else {
                    COND_PRINT(env.DEBUG_EVENT, "HCIHandler-IO RECV Drop (no translation) %s", event->toString().c_str());
                }
            }
        } else if( ETIMEDOUT != errno && !hciReaderShallStop ) { // expected exits
            ERR_PRINT("HCIHandler::reader: HCIComm read error");
        }
    }
    INFO_PRINT("HCIHandler::reader: Ended. Ring has %d entries flushed", hciEventRing.getSize());
    hciReaderRunning = false;
    hciEventRing.clear();
}

void HCIHandler::sendMgmtEvent(std::shared_ptr<MgmtEvent> event) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_callbackLists); // RAII-style acquire and relinquish via destructor
    MgmtEventCallbackList & mgmtEventCallbackList = mgmtEventCallbackLists[static_cast<uint16_t>(event->getOpcode())];
    int invokeCount = 0;
    if( mgmtEventCallbackList.size() > 0 ) {
        for (auto it = mgmtEventCallbackList.begin(); it != mgmtEventCallbackList.end(); ++it) {
            try {
                it->invoke(event);
            } catch (std::exception &e) {
                ERR_PRINT("HCIHandler::sendMgmtEvent-CBs %d/%zd: MgmtEventCallback %s : Caught exception %s",
                        invokeCount+1, mgmtEventCallbackList.size(),
                        it->toString().c_str(), e.what());
            }
            invokeCount++;
        }
    }
    COND_PRINT(env.DEBUG_EVENT, "HCIHandler::sendMgmtEvent: Event %s -> %d/%zd callbacks", event->toString().c_str(), invokeCount, mgmtEventCallbackList.size());
    (void)invokeCount;
}

bool HCIHandler::sendCommand(HCICommand &req) {
    COND_PRINT(env.DEBUG_EVENT, "HCIHandler-IO SENT %s", req.toString().c_str());

    TROOctets & pdu = req.getPDU();
    if ( comm.write( pdu.get_ptr(), pdu.getSize() ) < 0 ) {
        ERR_PRINT("HCIHandler::sendCommand: HCIComm write error, req %s", req.toString().c_str());
        return false;
    }
    return true;
}

std::shared_ptr<HCIEvent> HCIHandler::getNextReply(HCICommand &req, int32_t & retryCount, const int32_t replyTimeoutMS)
{
    // Ringbuffer read is thread safe
    while( retryCount < env.HCI_READ_PACKET_MAX_RETRY ) {
        std::shared_ptr<HCIEvent> ev = hciEventRing.getBlocking(replyTimeoutMS);
        if( nullptr == ev ) {
            errno = ETIMEDOUT;
            ERR_PRINT("HCIHandler::getNextReply: nullptr result (timeout %d ms -> abort): req %s",
                    replyTimeoutMS, req.toString().c_str());
            return nullptr;
        } else if( !ev->validate(req) ) {
            // This could occur due to an earlier timeout w/ a nullptr == res (see above),
            // i.e. the pending reply processed here and naturally not-matching.
            retryCount++;
            COND_PRINT(env.DEBUG_EVENT, "HCIHandler-IO RECV getNextReply: res mismatch (drop, retry %d): res %s; req %s",
                       retryCount, ev->toString().c_str(), req.toString().c_str());
        } else {
            COND_PRINT(env.DEBUG_EVENT, "HCIHandler-IO RECV getNextReply: res %s; req %s", ev->toString().c_str(), req.toString().c_str());
            return ev;
        }
    }
    return nullptr;
}

std::shared_ptr<HCIEvent> HCIHandler::sendWithCmdCompleteReply(HCICommand &req, HCICommandCompleteEvent **res) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_sendReply); // RAII-style acquire and relinquish via destructor

    *res = nullptr;

    int32_t retryCount = 0;
    std::shared_ptr<HCIEvent> ev = nullptr;

    if( !sendCommand(req) ) {
        goto exit;
    }

    while( retryCount < env.HCI_READ_PACKET_MAX_RETRY ) {
        ev = getNextReply(req, retryCount, env.HCI_COMMAND_COMPLETE_REPLY_TIMEOUT);
        if( nullptr == ev ) {
            break;  // timeout, leave loop
        } else if( ev->isEvent(HCIEventType::CMD_COMPLETE) ) {
            // gotcha, leave loop
            *res = static_cast<HCICommandCompleteEvent*>(ev.get());
            break;
        } else if( ev->isEvent(HCIEventType::CMD_STATUS) ) {
            // pending command .. wait for result
            HCICommandStatusEvent * ev_cs = static_cast<HCICommandStatusEvent*>(ev.get());
            HCIStatusCode status = ev_cs->getStatus();
            if( HCIStatusCode::SUCCESS != status ) {
                WARN_PRINT("HCIHandler::sendWithCmdCompleteReply: CMD_STATUS 0x%2.2X (%s), errno %d %s: res %s, req %s",
                        number(status), getHCIStatusCodeString(status).c_str(), errno, strerror(errno),
                        ev_cs->toString().c_str(), req.toString().c_str());
                break; // error status, leave loop
            } else {
                DBG_PRINT("HCIHandler::sendWithCmdCompleteReply: CMD_STATUS 0x%2.2X (%s, retryCount %d), errno %d %s: res %s, req %s",
                        number(status), getHCIStatusCodeString(status).c_str(), retryCount, errno, strerror(errno),
                        ev_cs->toString().c_str(), req.toString().c_str());
            }
            retryCount++;
            continue; // next packet
        } else {
            retryCount++;
            DBG_PRINT("HCIHandler::sendWithCmdCompleteReply: !(CMD_COMPLETE, CMD_STATUS) (drop, retry %d): res %s; req %s",
                       retryCount, ev->toString().c_str(), req.toString().c_str());
            continue; // next packet
        }
    }

exit:
    return ev;
}

HCIHandler::HCIHandler(const BTMode btMode, const uint16_t dev_id)
: env(HCIEnv::get()),
  btMode(btMode), dev_id(dev_id), rbuffer(HCI_MAX_MTU),
  comm(dev_id, HCI_CHANNEL_RAW),
  hciEventRing(env.HCI_EVT_RING_CAPACITY), hciReaderRunning(false), hciReaderShallStop(false)
{
    INFO_PRINT("HCIHandler.ctor: pid %d", HCIHandler::pidSelf);
    if( !comm.isOpen() ) {
        ERR_PRINT("HCIHandler::ctor: Could not open hci control channel");
        return;
    }

    {
        std::unique_lock<std::mutex> lock(mtx_hciReaderInit); // RAII-style acquire and relinquish via destructor

        std::thread hciReaderThread = std::thread(&HCIHandler::hciReaderThreadImpl, this);
        hciReaderThreadId = hciReaderThread.native_handle();
        // Avoid 'terminate called without an active exception'
        // as hciReaderThreadImpl may end due to I/O errors.
        hciReaderThread.detach();

        while( false == hciReaderRunning ) {
            cv_hciReaderInit.wait(lock);
        }
    }

    PERF_TS_T0();

    // Mandatory socket filter (not adapter filter!)
    {
#if 0
        // No use for pre-existing hci_ufilter
        hci_ufilter of;
        socklen_t olen;

        olen = sizeof(of);
        if (getsockopt(comm.dd(), SOL_HCI, HCI_FILTER, &of, &olen) < 0) {
            ERR_PRINT("HCIHandler::ctor: getsockopt");
            goto fail;
        }
#endif
        HCIComm::filter_clear(&filter_mask);
        HCIComm::filter_set_ptype(number(HCIPacketType::EVENT),  &filter_mask); // only EVENTs
        // Setup generic filter mask for all events, this is also required for
        // HCIComm::filter_all_events(&filter_mask); // all events
        HCIComm::filter_set_event(number(HCIEventType::CONN_COMPLETE), &filter_mask);
        HCIComm::filter_set_event(number(HCIEventType::DISCONN_COMPLETE), &filter_mask);
        HCIComm::filter_set_event(number(HCIEventType::CMD_COMPLETE), &filter_mask);
        HCIComm::filter_set_event(number(HCIEventType::CMD_STATUS), &filter_mask);
        HCIComm::filter_set_event(number(HCIEventType::HARDWARE_ERROR), &filter_mask);
        HCIComm::filter_set_event(number(HCIEventType::LE_META), &filter_mask);
        // HCIComm::filter_set_event(number(HCIEventType::DISCONN_PHY_LINK_COMPLETE), &filter_mask);
        // HCIComm::filter_set_event(number(HCIEventType::DISCONN_LOGICAL_LINK_COMPLETE), &filter_mask);
        HCIComm::filter_set_opcode(0, &filter_mask); // all opcode

        if (setsockopt(comm.dd(), SOL_HCI, HCI_FILTER, &filter_mask, sizeof(filter_mask)) < 0) {
            ERR_PRINT("HCIHandler::ctor: setsockopt");
            goto fail;
        }
    }
    // Mandatory own LE_META filter
    {
        uint32_t mask = 0;
        // filter_all_metaevs(mask);
        filter_set_metaev(HCIMetaEventType::LE_CONN_COMPLETE, mask);
        filter_set_metaev(HCIMetaEventType::LE_ADVERTISING_REPORT, mask);
        filter_put_metaevs(mask);
    }
    // Mandatory own HCIOpcodeBit/HCIOpcode filter
    {
        uint64_t mask = 0;
        // filter_all_opcbit(mask);
        filter_set_opcbit(HCIOpcodeBit::CREATE_CONN, mask);
        filter_set_opcbit(HCIOpcodeBit::DISCONNECT, mask);
        filter_set_opcbit(HCIOpcodeBit::RESET, mask);
        filter_set_opcbit(HCIOpcodeBit::READ_LOCAL_VERSION, mask);
        filter_set_opcbit(HCIOpcodeBit::LE_SET_SCAN_PARAM, mask);
        filter_set_opcbit(HCIOpcodeBit::LE_SET_SCAN_ENABLE, mask);
        filter_set_opcbit(HCIOpcodeBit::LE_CREATE_CONN, mask);
        filter_put_opcbit(mask);
    }
    {
        HCICommand req0(HCIOpcode::READ_LOCAL_VERSION, 0);
        const hci_rp_read_local_version * ev_lv;
        HCIStatusCode status;
        std::shared_ptr<HCIEvent> ev = processCommandComplete(req0, &ev_lv, &status);
        if( nullptr == ev || nullptr == ev_lv ) {
            ERR_PRINT("HCIHandler::ctor: failed READ_LOCAL_VERSION: 0x%x (%s)", number(status), getHCIStatusCodeString(status).c_str());
            goto fail;
        }
        INFO_PRINT("HCIHandler: LOCAL_VERSION: %d (rev %d), manuf 0x%x, lmp %d (subver %d)",
                ev_lv->hci_ver, le_to_cpu(ev_lv->hci_rev), le_to_cpu(ev_lv->manufacturer),
                ev_lv->lmp_ver, le_to_cpu(ev_lv->lmp_subver));
    }

    PERF_TS_TD("HCIHandler::open.ok");
    return;

fail:
    close();
    PERF_TS_TD("HCIHandler::open.fail");
    return;
}

void HCIHandler::close() {
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    DBG_PRINT("HCIHandler::close: Start");

    clearAllMgmtEventCallbacks();

    const pthread_t tid_self = pthread_self();
    const pthread_t tid_reader = hciReaderThreadId;
    hciReaderThreadId = 0;
    const bool is_reader = tid_reader == tid_self;
    DBG_PRINT("HCIHandler.disconnect: Start hciReader[running %d, shallStop %d, isReader %d, tid %p)",
            hciReaderRunning.load(), hciReaderShallStop.load(), is_reader, (void*)tid_reader);
    if( hciReaderRunning ) {
        hciReaderShallStop = true;
        if( !is_reader && 0 != tid_reader ) {
            int kerr;
            if( 0 != ( kerr = pthread_kill(tid_reader, SIGALRM) ) ) {
                ERR_PRINT("HCIHandler::disconnect: pthread_kill %p FAILED: %d", (void*)tid_reader, kerr);
            }
        }
    }
    comm.close();
    DBG_PRINT("HCIHandler::close: End");
}

HCIStatusCode HCIHandler::reset() {
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    if( !comm.isOpen() ) {
        ERR_PRINT("HCIHandler::reset: device not open");
        return HCIStatusCode::INTERNAL_FAILURE;
    }
    HCICommand req0(HCIOpcode::RESET, 0);
    HCICommandCompleteEvent * ev_cc;
    std::shared_ptr<HCIEvent> ev = sendWithCmdCompleteReply(req0, &ev_cc);
    if( nullptr == ev || nullptr == ev_cc ) {
        return HCIStatusCode::INTERNAL_TIMEOUT; // timeout
    }
    return ev_cc->getReturnStatus(0);
}

HCIStatusCode HCIHandler::le_set_scan_param(const bool le_scan_active,
                                            const HCILEOwnAddressType own_mac_type,
                                            const uint16_t le_scan_interval, const uint16_t le_scan_window,
                                            const uint8_t filter_policy) {
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    if( !comm.isOpen() ) {
        ERR_PRINT("HCIHandler::le_set_scan_param: device not open");
        return HCIStatusCode::INTERNAL_FAILURE;
    }
    HCIStructCommand<hci_cp_le_set_scan_param> req0(HCIOpcode::LE_SET_SCAN_PARAM);
    hci_cp_le_set_scan_param * cp = req0.getWStruct();
    cp->type = le_scan_active ? LE_SCAN_ACTIVE : LE_SCAN_PASSIVE;
    cp->interval = cpu_to_le(le_scan_interval);
    cp->window = cpu_to_le(le_scan_window);
    cp->own_address_type = static_cast<uint8_t>(own_mac_type);
    cp->filter_policy = filter_policy;

    const hci_rp_status * ev_status;
    HCIStatusCode status;
    std::shared_ptr<HCIEvent> ev = processCommandComplete(req0, &ev_status, &status);
    return status;
}

HCIStatusCode HCIHandler::le_enable_scan(const bool enable, const bool filter_dup) {
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    if( !comm.isOpen() ) {
        ERR_PRINT("HCIHandler::le_enable_scan: device not open");
        return HCIStatusCode::INTERNAL_FAILURE;
    }
    HCIStructCommand<hci_cp_le_set_scan_enable> req0(HCIOpcode::LE_SET_SCAN_ENABLE);
    hci_cp_le_set_scan_enable * cp = req0.getWStruct();
    cp->enable = enable ? LE_SCAN_ENABLE : LE_SCAN_DISABLE;
    cp->filter_dup = filter_dup ? LE_SCAN_FILTER_DUP_ENABLE : LE_SCAN_FILTER_DUP_DISABLE;

    const hci_rp_status * ev_status;
    HCIStatusCode status;
    std::shared_ptr<HCIEvent> ev = processCommandComplete(req0, &ev_status, &status);

    if( HCIStatusCode::SUCCESS == status ) {
        MgmtEvtDiscovering *e = new MgmtEvtDiscovering(dev_id, ScanType::LE, enable);
        sendMgmtEvent(std::shared_ptr<MgmtEvent>(e));
    }
    return status;
}

HCIStatusCode HCIHandler::le_create_conn(const EUI48 &peer_bdaddr,
                            const HCILEPeerAddressType peer_mac_type,
                            const HCILEOwnAddressType own_mac_type,
                            const uint16_t le_scan_interval, const uint16_t le_scan_window,
                            const uint16_t conn_interval_min, const uint16_t conn_interval_max,
                            const uint16_t conn_latency, const uint16_t supervision_timeout) {
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    if( !comm.isOpen() ) {
        ERR_PRINT("HCIHandler::le_create_conn: device not open");
        return HCIStatusCode::INTERNAL_FAILURE;
    }
    const uint16_t min_ce_length = 0x0000;
    const uint16_t max_ce_length = 0x0000;
    const uint8_t initiator_filter = 0x00; // whitelist not used but peer_bdaddr*

    HCIStructCommand<hci_cp_le_create_conn> req0(HCIOpcode::LE_CREATE_CONN);
    hci_cp_le_create_conn * cp = req0.getWStruct();
    cp->scan_interval = cpu_to_le(le_scan_interval);
    cp->scan_window = cpu_to_le(le_scan_window);
    cp->filter_policy = initiator_filter;
    cp->peer_addr_type = static_cast<uint8_t>(peer_mac_type);
    cp->peer_addr = peer_bdaddr;
    cp->own_address_type = static_cast<uint8_t>(own_mac_type);
    cp->conn_interval_min = cpu_to_le(conn_interval_min);
    cp->conn_interval_max = cpu_to_le(conn_interval_max);
    cp->conn_latency = cpu_to_le(conn_latency);
    cp->supervision_timeout = cpu_to_le(supervision_timeout);
    cp->min_ce_len = cpu_to_le(min_ce_length);
    cp->max_ce_len = cpu_to_le(max_ce_length);

    addOrUpdateTrackerConnection(peer_bdaddr, getBDAddressType(peer_mac_type), 0);
    HCIStatusCode status;
    std::shared_ptr<HCIEvent> ev = processCommandStatus(req0, &status);
    return status;
}

HCIStatusCode HCIHandler::create_conn(const EUI48 &bdaddr,
                                     const uint16_t pkt_type,
                                     const uint16_t clock_offset, const uint8_t role_switch) {
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    if( !comm.isOpen() ) {
        ERR_PRINT("HCIHandler::create_conn: device not open");
        return HCIStatusCode::INTERNAL_FAILURE;
    }
    HCIStructCommand<hci_cp_create_conn> req0(HCIOpcode::CREATE_CONN);
    hci_cp_create_conn * cp = req0.getWStruct();
    cp->bdaddr = bdaddr;
    cp->pkt_type = cpu_to_le((uint16_t)(pkt_type & (uint16_t)ACL_PTYPE_MASK)); /* TODO OK excluding SCO_PTYPE_MASK   (HCI_HV1 | HCI_HV2 | HCI_HV3) ? */
    cp->pscan_rep_mode = 0x02; /* TODO magic? */
    cp->pscan_mode = 0x00; /* TODO magic? */
    cp->clock_offset = cpu_to_le(clock_offset);
    cp->role_switch = role_switch;

    addOrUpdateTrackerConnection(bdaddr, BDAddressType::BDADDR_BREDR, 0);
    HCIStatusCode status;
    std::shared_ptr<HCIEvent> ev = processCommandStatus(req0, &status);
    return status;
}

HCIStatusCode HCIHandler::disconnect(const bool ioErrorCause,
                                     const uint16_t conn_handle, const EUI48 &peer_bdaddr, const BDAddressType peer_mac_type,
                                     const HCIStatusCode reason)
{
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    if( !comm.isOpen() ) {
        ERR_PRINT("HCIHandler::create_conn: device not open");
        return HCIStatusCode::INTERNAL_FAILURE;
    }
    if( 0 == conn_handle ) {
        ERR_PRINT("HCIHandler::disconnect: Null conn_handle given address[%s, %s] (drop)",
                   peer_bdaddr.toString().c_str(), getBDAddressTypeString(peer_mac_type).c_str());
        return HCIStatusCode::INVALID_HCI_COMMAND_PARAMETERS;
    }
    HCIConnectionRef conn;
    {
        const std::lock_guard<std::recursive_mutex> lock(mtx_connectionList); // RAII-style acquire and relinquish via destructor
        conn = findTrackerConnection(conn_handle);
        if( nullptr == conn ) {
            // disconnect called w/o being connected through this HCIHandler
            conn = addOrUpdateTrackerConnection(peer_bdaddr, peer_mac_type, conn_handle);
            INFO_PRINT("HCIHandler::disconnect: Not tracked address[%s, %s], added %s",
                       peer_bdaddr.toString().c_str(), getBDAddressTypeString(peer_mac_type).c_str(),
                       conn->toString().c_str());
        } else if( !conn->equals(peer_bdaddr, peer_mac_type) ) {
            ERR_PRINT("HCIHandler::disconnect: Mismatch given address[%s, %s] and tracked %s (drop)",
                       peer_bdaddr.toString().c_str(), getBDAddressTypeString(peer_mac_type).c_str(),
                       conn->toString().c_str());
            return HCIStatusCode::INVALID_HCI_COMMAND_PARAMETERS;
        }
    }
    DBG_PRINT("HCIHandler::disconnect: address[%s, %s], handle %s, %s, ioError %d",
               peer_bdaddr.toString().c_str(), getBDAddressTypeString(peer_mac_type).c_str(),
               uint16HexString(conn_handle).c_str(),
               conn->toString().c_str(), ioErrorCause);

    HCIStatusCode status;

    // Always issue DISCONNECT command, even in case of an ioError (lost-connection),
    // see Issue #124 fast re-connect on CSR adapter.
    // This will always notify the adapter of a disconnected device.
    {
        HCIStructCommand<hci_cp_disconnect> req0(HCIOpcode::DISCONNECT);
        hci_cp_disconnect * cp = req0.getWStruct();
        cp->handle = cpu_to_le(conn_handle);
        cp->reason = number(reason);

        std::shared_ptr<HCIEvent> ev = processCommandStatus(req0, &status);
    }
    if( ioErrorCause ) {
        // In case of an ioError (lost-connection), don't wait for the lagging
        // DISCONN_COMPLETE event but send it directly.
        removeTrackerConnection(conn);
        MgmtEvtDeviceDisconnected *e = new MgmtEvtDeviceDisconnected(dev_id, peer_bdaddr, peer_mac_type, reason, conn_handle);
        sendMgmtEvent(std::shared_ptr<MgmtEvent>(e));
    }

    return status;
}

std::shared_ptr<HCIEvent> HCIHandler::processCommandStatus(HCICommand &req, HCIStatusCode *status)
{
    const std::lock_guard<std::recursive_mutex> lock(mtx_sendReply); // RAII-style acquire and relinquish via destructor

    *status = HCIStatusCode::INTERNAL_FAILURE;

    int32_t retryCount = 0;
    std::shared_ptr<HCIEvent> ev = nullptr;

    if( !sendCommand(req) ) {
        goto exit;
    }

    while( retryCount < env.HCI_READ_PACKET_MAX_RETRY ) {
        ev = getNextReply(req, retryCount, env.HCI_COMMAND_STATUS_REPLY_TIMEOUT);
        if( nullptr == ev ) {
            *status = HCIStatusCode::INTERNAL_TIMEOUT;
            break; // timeout, leave loop
        } else if( ev->isEvent(HCIEventType::CMD_STATUS) ) {
            HCICommandStatusEvent * ev_cs = static_cast<HCICommandStatusEvent*>(ev.get());
            *status = ev_cs->getStatus();
            DBG_PRINT("HCIHandler::processCommandStatus %s -> Status 0x%2.2X (%s), errno %d %s: res %s, req %s",
                    getHCIOpcodeString(req.getOpcode()).c_str(),
                    number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                    ev_cs->toString().c_str(), req.toString().c_str());
            break; // gotcha, leave loop - pending completion result handled via callback
        } else {
            retryCount++;
            DBG_PRINT("HCIHandler::processCommandStatus: !CMD_STATUS (drop, retry %d): res %s; req %s",
                       retryCount, ev->toString().c_str(), req.toString().c_str());
            continue; // next packet
        }
    }
    if( nullptr == ev ) {
        // timeout exit
        WARN_PRINT("HCIHandler::processCommandStatus %s -> Status 0x%2.2X (%s), errno %d %s: res nullptr, req %s",
                getHCIOpcodeString(req.getOpcode()).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                req.toString().c_str());
    }

exit:
    return ev;
}

template<typename hci_cmd_event_struct>
std::shared_ptr<HCIEvent> HCIHandler::processCommandComplete(HCICommand &req,
                                                             const hci_cmd_event_struct **res, HCIStatusCode *status)
{
    *res = nullptr;
    *status = HCIStatusCode::INTERNAL_FAILURE;

    const HCIEventType evc = HCIEventType::CMD_COMPLETE;
    HCICommandCompleteEvent * ev_cc;
    std::shared_ptr<HCIEvent> ev = sendWithCmdCompleteReply(req, &ev_cc);
    if( nullptr == ev ) {
        *status = HCIStatusCode::INTERNAL_TIMEOUT;
        WARN_PRINT("HCIHandler::processCommandComplete %s -> %s: Status 0x%2.2X (%s), errno %d %s: res nullptr, req %s",
                getHCIOpcodeString(req.getOpcode()).c_str(), getHCIEventTypeString(evc).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                req.toString().c_str());
        return nullptr; // timeout
    } else if( nullptr == ev_cc ) {
        WARN_PRINT("HCIHandler::processCommandComplete %s -> %s: Status 0x%2.2X (%s), errno %d %s: res %s, req %s",
                getHCIOpcodeString(req.getOpcode()).c_str(), getHCIEventTypeString(evc).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                ev->toString().c_str(), req.toString().c_str());
        return ev;
    }
    const uint8_t returnParamSize = ev_cc->getReturnParamSize();
    if( returnParamSize < sizeof(hci_cmd_event_struct) ) {
        WARN_PRINT("HCIHandler::processCommandComplete %s -> %s: Status 0x%2.2X (%s), errno %d %s: res %s, req %s",
                getHCIOpcodeString(req.getOpcode()).c_str(), getHCIEventTypeString(evc).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                ev_cc->toString().c_str(), req.toString().c_str());
        return ev;
    }
    *res = (const hci_cmd_event_struct*)(ev_cc->getReturnParam());
    *status = static_cast<HCIStatusCode>((*res)->status);
    DBG_PRINT("HCIHandler::processCommandComplete %s -> %s: Status 0x%2.2X (%s): res %s, req %s",
            getHCIOpcodeString(req.getOpcode()).c_str(), getHCIEventTypeString(evc).c_str(),
            number(*status), getHCIStatusCodeString(*status).c_str(),
            ev_cc->toString().c_str(), req.toString().c_str());
    return ev;
}

template<typename hci_cmd_event_struct>
const hci_cmd_event_struct* HCIHandler::getReplyStruct(std::shared_ptr<HCIEvent> event, HCIEventType evc, HCIStatusCode *status)
{
    const hci_cmd_event_struct* res = nullptr;
    *status = HCIStatusCode::INTERNAL_FAILURE;

    typedef HCIStructCmdCompleteEvt<hci_cmd_event_struct> HCITypeCmdCompleteEvt;
    HCITypeCmdCompleteEvt * ev_cc = static_cast<HCITypeCmdCompleteEvt*>(event.get());
    if( ev_cc->isTypeAndSizeValid(evc) ) {
        *status = ev_cc->getStatus();
        res = ev_cc->getStruct();
    } else {
        WARN_PRINT("HCIHandler::getReplyStruct: %s: Type or size mismatch: Status 0x%2.2X (%s), errno %d %s: res %s",
                getHCIEventTypeString(evc).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                ev_cc->toString().c_str());
    }
    return res;
}

template<typename hci_cmd_event_struct>
const hci_cmd_event_struct* HCIHandler::getMetaReplyStruct(std::shared_ptr<HCIEvent> event, HCIMetaEventType mec, HCIStatusCode *status)
{
    const hci_cmd_event_struct* res = nullptr;
    *status = HCIStatusCode::INTERNAL_FAILURE;

    typedef HCIStructCmdCompleteMetaEvt<hci_cmd_event_struct> HCITypeCmdCompleteMetaEvt;
    HCITypeCmdCompleteMetaEvt * ev_cc = static_cast<HCITypeCmdCompleteMetaEvt*>(event.get());
    if( ev_cc->isTypeAndSizeValid(mec) ) {
        *status = ev_cc->getStatus();
        res = ev_cc->getStruct();
    } else {
        WARN_PRINT("HCIHandler::getMetaReplyStruct: %s: Type or size mismatch: Status 0x%2.2X (%s), errno %d %s: res %s",
                  getHCIMetaEventTypeString(mec).c_str(),
                  number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                  ev_cc->toString().c_str());
    }
    return res;
}

/***
 *
 * MgmtEventCallback section
 *
 */

void HCIHandler::addMgmtEventCallback(const MgmtEvent::Opcode opc, const MgmtEventCallback &cb) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_callbackLists); // RAII-style acquire and relinquish via destructor
    checkMgmtEventCallbackListsIndex(opc);
    MgmtEventCallbackList &l = mgmtEventCallbackLists[static_cast<uint16_t>(opc)];
    for (auto it = l.begin(); it != l.end(); ++it) {
        if ( *it == cb ) {
            // already exists for given adapter
            return;
        }
    }
    l.push_back( cb );
}
int HCIHandler::removeMgmtEventCallback(const MgmtEvent::Opcode opc, const MgmtEventCallback &cb) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_callbackLists); // RAII-style acquire and relinquish via destructor
    checkMgmtEventCallbackListsIndex(opc);
    int count = 0;
    MgmtEventCallbackList &l = mgmtEventCallbackLists[static_cast<uint16_t>(opc)];
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
void HCIHandler::clearMgmtEventCallbacks(const MgmtEvent::Opcode opc) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_callbackLists); // RAII-style acquire and relinquish via destructor
    checkMgmtEventCallbackListsIndex(opc);
    mgmtEventCallbackLists[static_cast<uint16_t>(opc)].clear();
}
void HCIHandler::clearAllMgmtEventCallbacks() {
    const std::lock_guard<std::recursive_mutex> lock(mtx_callbackLists); // RAII-style acquire and relinquish via destructor
    for(size_t i=0; i<mgmtEventCallbackLists.size(); i++) {
        mgmtEventCallbackLists[i].clear();
    }
}
