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

// #define SHOW_LE_ADVERTISING 1

// #define PERF_PRINT_ON 1
// #define VERBOSE_ON 1
#include <dbt_debug.hpp>

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

const pid_t HCIHandler::pidSelf = getpid();

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
                const HCIAddressType hciAddrType = static_cast<HCIAddressType>(ev_cc->bdaddr_type);
                const BDAddressType addrType = getBDAddressType(hciAddrType);
                if( HCIStatusCode::SUCCESS == status ) {
                    return std::shared_ptr<MgmtEvent>( new MgmtEvtDeviceConnected(dev_id, ev_cc->bdaddr, addrType, ev_cc->handle) );
                } else {
                    return std::shared_ptr<MgmtEvent>( new MgmtEvtDeviceConnectFailed(dev_id, ev_cc->bdaddr, addrType, status) );
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
            if( HCIStatusCode::SUCCESS == status ) {
                return std::shared_ptr<MgmtEvent>( new MgmtEvtDeviceConnected(dev_id, ev_cc->bdaddr, BDAddressType::BDADDR_BREDR, ev_cc->handle) );
            } else {
                return std::shared_ptr<MgmtEvent>( new MgmtEvtDeviceConnectFailed(dev_id, ev_cc->bdaddr, BDAddressType::BDADDR_BREDR, status) );
            }
        }
        case HCIEventType::DISCONN_COMPLETE: {
            HCIStatusCode status;
            const hci_ev_disconn_complete * ev_cc = getReplyStruct<hci_ev_disconn_complete>(ev, evt, &status);
            if( HCIStatusCode::SUCCESS == status ) {
                const HCIStatusCode hciRootReason = static_cast<HCIStatusCode>(ev_cc->reason);
                const uint16_t handle = ev_cc->handle;
                EUI48 address; // ANY
                BDAddressType addrType = BDAddressType::BDADDR_UNDEFINED;
                {
                    const std::lock_guard<std::recursive_mutex> lock(mtx_disconnectHandleAddrList); // RAII-style acquire and relinquish via destructor
                    for (auto it = disconnectHandleAddrList.begin(); it != disconnectHandleAddrList.end(); ) {
                        if ( it->handle == handle ) {
                            address = it->address;
                            addrType = it->addressType;
                            it = disconnectHandleAddrList.erase(it);
                            break; // done
                        } else {
                            ++it;
                        }
                    }
                }
                if( BDAddressType::BDADDR_UNDEFINED != addrType ) {
                    return std::shared_ptr<MgmtEvent>( new MgmtEvtDeviceDisconnected(dev_id, address, addrType, hciRootReason) );
                }
                return nullptr; // unknown handle!
            }
            return nullptr;
        }
        default: return nullptr;
    }
}

void HCIHandler::hciReaderThreadImpl() {
    {
        const std::lock_guard<std::mutex> lock(mtx_hciReaderInit); // RAII-style acquire and relinquish via destructor
        hciReaderShallStop = false;
        hciReaderRunning = true;
        INFO_PRINT("HCIHandler::reader: Started");
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

        len = comm.read(rbuffer.get_wptr(), rbuffer.getSize());
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
                ERR_PRINT("HCIHandler::reader: Drop (non-event) %s", bytesHexString(rbuffer.get_ptr(), 0, len, true /* lsbFirst*/).c_str());
                continue;
            }
            const HCIMetaEventType mec = event->getMetaEventType();
            if( HCIMetaEventType::INVALID != mec && !filter_test_metaev(mec) ) {
                // DROP
                DBG_PRINT("HCIHandler::reader: Drop (meta filter) %s", event->toString().c_str());
                continue; // next packet
            }
#ifdef SHOW_LE_ADVERTISING
            if( event->isMetaEvent(HCIMetaEventType::LE_ADVERTISING_REPORT) ) {
                std::vector<std::shared_ptr<EInfoReport>> eirlist = EInfoReport::read_ad_reports(event->getParam(), event->getParamSize());
                int i=0;
                for_each_idx(eirlist, [&](std::shared_ptr<EInfoReport> &eir) {
                    INFO_PRINT("LE_ADV[%d]: %s", i, eir->toString().c_str());
                    i++;
                });
                continue; // next packet
            }
#endif /* SHOW_LE_ADVERTISING */
            if( event->isEvent(HCIEventType::CMD_STATUS) || event->isEvent(HCIEventType::CMD_COMPLETE) )
            {
                if( hciEventRing.isFull() ) {
                    const int dropCount = hciEventRing.capacity()/2;
                    hciEventRing.drop(dropCount);
                    INFO_PRINT("HCIHandler::reader: Drop (%d oldest elements of %d capacity, ring full)", dropCount, hciEventRing.capacity());
                }
                DBG_PRINT("HCIHandler::reader: CmdResult %s", event->toString().c_str());
                hciEventRing.putBlocking( event );
            } else {
                // issue a callback
                std::shared_ptr<MgmtEvent> mevent = translate(event);
                if( nullptr != mevent ) {
                    const std::lock_guard<std::recursive_mutex> lock(mtx_callbackLists); // RAII-style acquire and relinquish via destructor
                    MgmtEventCallbackList & mgmtEventCallbackList = mgmtEventCallbackLists[static_cast<uint16_t>(mevent->getOpcode())];
                    int invokeCount = 0;
                    if( mgmtEventCallbackList.size() > 0 ) {
                        for (auto it = mgmtEventCallbackList.begin(); it != mgmtEventCallbackList.end(); ++it) {
                            try {
                                it->invoke(mevent);
                            } catch (std::exception &e) {
                                ERR_PRINT("HCIHandler::fwdPacketReceived-CBs %d/%zd: MgmtEventCallback %s : Caught exception %s",
                                        invokeCount+1, mgmtEventCallbackList.size(),
                                        it->toString().c_str(), e.what());
                            }
                            invokeCount++;
                        }
                    }
                    DBG_PRINT("HCIHandler::reader: Event %s -> %d/%zd callbacks; source %s",
                            mevent->toString().c_str(), invokeCount, mgmtEventCallbackList.size(), event->toString().c_str());
                    (void)invokeCount;
                } else {
                    DBG_PRINT("HCIHandler::reader: Drop (no translation) %s", event->toString().c_str());
                }
            }
        } else if( ETIMEDOUT != errno && !hciReaderShallStop ) { // expected exits
            ERR_PRINT("HCIHandler::reader: HCIComm error");
        }
    }
    INFO_PRINT("HCIHandler::reader: Ended. Ring has %d entries flushed", hciEventRing.getSize());
    hciReaderRunning = false;
    hciEventRing.clear();
}

bool HCIHandler::sendCommand(HCICommand &req) {
    const std::lock_guard<std::recursive_mutex> lock(comm.mutex()); // RAII-style acquire and relinquish via destructor
    TROOctets & pdu = req.getPDU();
    if ( comm.write( pdu.get_ptr(), pdu.getSize() ) < 0 ) {
        ERR_PRINT("HCIHandler::sendCommand: HCIComm write error, req %s", req.toString().c_str());
        return false;
    }
    return true;
}

std::shared_ptr<HCIEvent> HCIHandler::getNextReply(HCICommand &req, int & retryCount) {
    // Ringbuffer read is thread safe
    while( retryCount < HCI_READ_PACKET_MAX_RETRY ) {
        std::shared_ptr<HCIEvent> ev = hciEventRing.getBlocking(replyTimeoutMS);
        if( nullptr == ev ) {
            errno = ETIMEDOUT;
            DBG_PRINT("HCIHandler::getNextReply: nullptr result (timeout -> abort): req %s", req.toString().c_str());
            return nullptr;
        } else if( !ev->validate(req) ) {
            // This could occur due to an earlier timeout w/ a nullptr == res (see above),
            // i.e. the pending reply processed here and naturally not-matching.
            retryCount++;
            DBG_PRINT("HCIHandler::getNextReply: res mismatch (drop, retry %d): res %s; req %s",
                       retryCount, ev->toString().c_str(), req.toString().c_str());
        } else {
            DBG_PRINT("HCIHandler::getNextReply: res: %s, req %s", ev->toString().c_str(), req.toString().c_str());
            return ev;
        }
    }
    return nullptr;
}

std::shared_ptr<HCIEvent> HCIHandler::sendWithCmdCompleteReply(HCICommand &req, HCICommandCompleteEvent **res) {
    const std::lock_guard<std::recursive_mutex> lock(mtx_sendReply); // RAII-style acquire and relinquish via destructor

    *res = nullptr;

    int retryCount = 0;
    std::shared_ptr<HCIEvent> ev = nullptr;

    if( !sendCommand(req) ) {
        goto exit;
    }

    while( retryCount < HCI_READ_PACKET_MAX_RETRY ) {
        ev = getNextReply(req, retryCount);
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
            }
            retryCount++;
            continue; // next packet
        } else {
            retryCount++;
            DBG_PRINT("HCIHandler::sendWithCmdCompleteReply: !CMD_COMPLETE (drop, retry %d): res %s; req %s",
                       retryCount, ev->toString().c_str(), req.toString().c_str());
            continue; // next packet
        }
    }

exit:
    return ev;
}

HCIHandler::HCIHandler(const BTMode btMode, const uint16_t dev_id, const int replyTimeoutMS)
:btMode(btMode), dev_id(dev_id), rbuffer(HCI_MAX_MTU),
 comm(dev_id, HCI_CHANNEL_RAW, Defaults::HCI_READER_THREAD_POLL_TIMEOUT), replyTimeoutMS(replyTimeoutMS),
 hciEventRing(HCI_EVT_RING_CAPACITY), hciReaderRunning(false), hciReaderShallStop(false)
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
        DBG_PRINT("HCIHandler::ctor: Reader Started");
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
#ifdef SHOW_LE_ADVERTISING
        filter_set_metaev(HCIMetaEventType::LE_ADVERTISING_REPORT, mask);
#endif
        filter_put_metaevs(mask);
    }
    // Add own callbacks
    {
        addMgmtEventCallback(MgmtEvent::Opcode::DEVICE_DISCONNECTED, bindMemberFunc(this, &HCIHandler::mgmtEvDeviceDisconnectedCB));
        addMgmtEventCallback(MgmtEvent::Opcode::DEVICE_CONNECTED, bindMemberFunc(this, &HCIHandler::mgmtEvDeviceConnectedCB));
        addMgmtEventCallback(MgmtEvent::Opcode::CONNECT_FAILED, bindMemberFunc(this, &HCIHandler::mgmtEvConnectFailedCB));
    }
    {
        HCICommand req0(HCIOpcode::READ_LOCAL_VERSION, 0);
        const hci_rp_read_local_version * ev_lv;
        HCIStatusCode status;
        std::shared_ptr<HCIEvent> ev = processSimpleCommand<hci_rp_read_local_version>(
                HCIOpcode::READ_LOCAL_VERSION, &ev_lv, &status);
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
        return HCIStatusCode::INTERNAL_FAILURE;
    }
    return ev_cc->getReturnStatus(0);
}

HCIStatusCode HCIHandler::le_create_conn(const EUI48 &peer_bdaddr,
                            const HCIAddressType peer_mac_type,
                            const HCIAddressType own_mac_type,
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
    bzero((void*)cp, sizeof(*cp));
    cp->scan_interval = cpu_to_le(le_scan_interval);
    cp->scan_window = cpu_to_le(le_scan_window);
    cp->filter_policy = initiator_filter;
    cp->peer_addr_type = peer_mac_type;
    cp->peer_addr = peer_bdaddr;
    cp->own_address_type = own_mac_type;
    cp->conn_interval_min = cpu_to_le(conn_interval_min);
    cp->conn_interval_max = cpu_to_le(conn_interval_max);
    cp->conn_latency = cpu_to_le(conn_latency);
    cp->supervision_timeout = cpu_to_le(supervision_timeout);
    cp->min_ce_len = cpu_to_le(min_ce_length);
    cp->max_ce_len = cpu_to_le(max_ce_length);

    HCIStatusCode status;
    std::shared_ptr<HCIEvent> ev = processStructCommand(req0, &status);
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
    bzero((void*)cp, sizeof(*cp));
    cp->bdaddr = bdaddr;
    cp->pkt_type = cpu_to_le((uint16_t)(pkt_type & (uint16_t)ACL_PTYPE_MASK)); /* TODO OK excluding SCO_PTYPE_MASK   (HCI_HV1 | HCI_HV2 | HCI_HV3) ? */
    cp->pscan_rep_mode = 0x02; /* TODO magic? */
    cp->pscan_mode = 0x00; /* TODO magic? */
    cp->clock_offset = cpu_to_le(clock_offset);
    cp->role_switch = role_switch;

    HCIStatusCode status;
    std::shared_ptr<HCIEvent> ev = processStructCommand(req0, &status);
    return status;
}

HCIStatusCode HCIHandler::disconnect(const uint16_t conn_handle, const EUI48 &peer_bdaddr, const BDAddressType peer_mac_type,
                                     const HCIStatusCode reason)
{
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    if( !comm.isOpen() ) {
        ERR_PRINT("HCIHandler::create_conn: device not open");
        return HCIStatusCode::INTERNAL_FAILURE;
    }
    if( 0 == conn_handle ) {
        return HCIStatusCode::SUCCESS;
    }
    {
        const std::lock_guard<std::recursive_mutex> lock(mtx_disconnectHandleAddrList); // RAII-style acquire and relinquish via destructor
        for (auto it = disconnectHandleAddrList.begin(); it != disconnectHandleAddrList.end(); ) {
            if ( it->handle == conn_handle ) {
                it = disconnectHandleAddrList.erase(it); // old entry
                break; // done
            } else {
                ++it;
            }
        }
        disconnectHandleAddrList.push_back(HCIHandleBDAddr(conn_handle, peer_bdaddr, peer_mac_type));
    }
    HCIStructCommand<hci_cp_disconnect> req0(HCIOpcode::DISCONNECT);
    hci_cp_disconnect * cp = req0.getWStruct();
    bzero(cp, sizeof(*cp));
    cp->handle = conn_handle;
    cp->reason = number(reason);

    HCIStatusCode status;
    std::shared_ptr<HCIEvent> ev = processStructCommand(req0, &status);
    return status;
}


template<typename hci_cmd_event_struct>
std::shared_ptr<HCIEvent> HCIHandler::processSimpleCommand(HCIOpcode opc, const hci_cmd_event_struct **res, HCIStatusCode *status)
{
    *res = nullptr;
    *status = HCIStatusCode::INTERNAL_FAILURE;

    const HCIEventType evc = HCIEventType::CMD_COMPLETE;
    HCICommand req0(opc, 0);
    HCICommandCompleteEvent * ev_cc;
    std::shared_ptr<HCIEvent> ev = sendWithCmdCompleteReply(req0, &ev_cc);
    if( nullptr == ev ) {
        WARN_PRINT("HCIHandler::processSimpleCommand %s -> %s: Status 0x%2.2X (%s), errno %d %s: res nullptr, req %s",
                getHCIOpcodeString(opc).c_str(), getHCIEventTypeString(evc).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                req0.toString().c_str());
        return nullptr; // timeout
    } else if( nullptr == ev_cc ) {
        WARN_PRINT("HCIHandler::processSimpleCommand %s -> %s: Status 0x%2.2X (%s), errno %d %s: res %s, req %s",
                getHCIOpcodeString(opc).c_str(), getHCIEventTypeString(evc).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                ev->toString().c_str(), req0.toString().c_str());
        return ev;
    }
    const uint8_t returnParamSize = ev_cc->getReturnParamSize();
    if( returnParamSize < sizeof(hci_cmd_event_struct) ) {
        WARN_PRINT("HCIHandler::processSimpleCommand %s -> %s: Status 0x%2.2X (%s), errno %d %s: res %s, req %s",
                getHCIOpcodeString(opc).c_str(), getHCIEventTypeString(evc).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                ev_cc->toString().c_str(), req0.toString().c_str());
        return ev;
    }
    *res = (const hci_cmd_event_struct*)(ev_cc->getReturnParam());
    *status = static_cast<HCIStatusCode>((*res)->status);
    DBG_PRINT("HCIHandler::processSimpleCommand %s -> %s: Status 0x%2.2X (%s): res %s, req %s",
            getHCIOpcodeString(opc).c_str(), getHCIEventTypeString(evc).c_str(),
            number(*status), getHCIStatusCodeString(*status).c_str(),
            ev_cc->toString().c_str(), req0.toString().c_str());
    return ev;
}

template<typename hci_command_struct>
std::shared_ptr<HCIEvent> HCIHandler::processStructCommand(HCIStructCommand<hci_command_struct> &req, HCIStatusCode *status)
{
    const std::lock_guard<std::recursive_mutex> lock(mtx_sendReply); // RAII-style acquire and relinquish via destructor

    *status = HCIStatusCode::INTERNAL_FAILURE;

    int retryCount = 0;
    std::shared_ptr<HCIEvent> ev = nullptr;

    if( !sendCommand(req) ) {
        goto exit;
    }

    while( retryCount < HCI_READ_PACKET_MAX_RETRY ) {
        ev = getNextReply(req, retryCount);
        if( nullptr == ev ) {
            break; // timeout, leave loop
        } else if( ev->isEvent(HCIEventType::CMD_STATUS) ) {
            HCICommandStatusEvent * ev_cs = static_cast<HCICommandStatusEvent*>(ev.get());
            *status = ev_cs->getStatus();
            DBG_PRINT("HCIHandler::processStructCommand %s -> Status 0x%2.2X (%s), errno %d %s: res %s, req %s",
                    getHCIOpcodeString(req.getOpcode()).c_str(),
                    number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                    ev_cs->toString().c_str(), req.toString().c_str());
            break; // gotcha, leave loop - pending completion result handled via callback
        } else {
            retryCount++;
            continue; // next packet
        }
    }
    if( nullptr == ev ) {
        // timeout exit
        WARN_PRINT("HCIHandler::processStructCommand %s -> Status 0x%2.2X (%s), errno %d %s: res nullptr, req %s",
                getHCIOpcodeString(req.getOpcode()).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                req.toString().c_str());
    }

exit:
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
        DBG_PRINT("HCIHandler::getReplyStruct: %s: Status 0x%2.2X (%s): res %s",
                getHCIEventTypeString(evc).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(),
                ev_cc->toString().c_str());
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
        DBG_PRINT("HCIHandler::getMetaReplyStruct: %s: Status 0x%2.2X (%s): res %s",
                  getHCIMetaEventTypeString(mec).c_str(),
                  number(*status), getHCIStatusCodeString(*status).c_str(),
                  ev_cc->toString().c_str());
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

bool HCIHandler::mgmtEvDeviceDisconnectedCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("HCIHandler::EventCB:DeviceDisconnected: %s", e->toString().c_str());
    const MgmtEvtDeviceDisconnected &event = *static_cast<const MgmtEvtDeviceDisconnected *>(e.get());
    (void)event;
    return true;
}
bool HCIHandler::mgmtEvDeviceConnectedCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("HCIHandler::EventCB:DeviceConnected: %s", e->toString().c_str());
    const MgmtEvtDeviceConnected &event = *static_cast<const MgmtEvtDeviceConnected *>(e.get());
    (void)event;
    return true;
}
bool HCIHandler::mgmtEvConnectFailedCB(std::shared_ptr<MgmtEvent> e) {
    DBG_PRINT("HCIHandler::EventCB:ConnectFailed: %s", e->toString().c_str());
    const MgmtEvtDeviceConnectFailed &event = *static_cast<const MgmtEvtDeviceConnectFailed *>(e.get());
    (void)event;
    return true;
}
