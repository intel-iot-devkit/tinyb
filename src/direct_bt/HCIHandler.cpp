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
#define VERBOSE_ON 1
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

void HCIHandler::hciReaderThreadImpl() {
    hciReaderShallStop = false;
    hciReaderRunning = true;
    INFO_PRINT("HCIHandler::reader: Started");

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
                ERR_PRINT("HCIHandler::reader: drop non-event %s", bytesHexString(rbuffer.get_ptr(), 0, len, true /* lsbFirst*/).c_str());
                continue;
            }
            const HCIMetaEventType mec = event->getMetaEventType();
            if( HCIMetaEventType::INVALID != mec && !filter_test_metaev(mec) ) {
                // DROP
                DBG_PRINT("HCIHandler::reader: drop %s", event->toString().c_str());
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
            if( hciEventRing.isFull() ) {
                std::shared_ptr<HCIEvent> ev = hciEventRing.get();
                INFO_PRINT("HCIHandler::reader: full ring, dropping oldest %s",
                        ( nullptr != ev ) ? ev->toString().c_str() : "nil");
            }
            DBG_PRINT("HCIHandler::reader: got %s", event->toString().c_str());
            hciEventRing.putBlocking( event );
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
        ERR_PRINT("HCIHandler::sendWithReply: HCIComm write error, req %s", req.toString().c_str());
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

std::shared_ptr<HCIEvent> HCIHandler::sendWithReply(HCICommand &req) {
    if( !sendCommand(req) ) {
        return nullptr;
    }
    int retryCount = 0;
    return getNextReply(req, retryCount);
}

std::shared_ptr<HCIEvent> HCIHandler::sendWithCmdCompleteReply(HCICommand &req, HCICommandCompleteEvent **res) {
    *res = nullptr;
    if( !sendCommand(req) ) {
        return nullptr;
    }

    int retryCount = 0;

    while( retryCount < HCI_READ_PACKET_MAX_RETRY ) {
        std::shared_ptr<HCIEvent> ev = getNextReply(req, retryCount);
        if( nullptr == ev ) {
            return nullptr;  // timeout
        } else if( !ev->isEvent(HCIEventType::CMD_COMPLETE) ) {
            DBG_PRINT("HCIHandler::sendWithCmdCompleteReply: !CMD_COMPLETE (drop, retry %d): res %s; req %s",
                       retryCount, ev->toString().c_str(), req.toString().c_str());
            continue; // next packet
        } else {
            *res = static_cast<HCICommandCompleteEvent*>(ev.get());
            return ev;
        }
    }
    return nullptr;  // max retry
}

std::shared_ptr<HCIEvent> HCIHandler::sendWithCmdStatusReply(HCICommand &req, HCICommandStatusEvent **res) {
    *res = nullptr;
    if( !sendCommand(req) ) {
        return nullptr;
    }

    int retryCount = 0;

    while( retryCount < HCI_READ_PACKET_MAX_RETRY ) {
        std::shared_ptr<HCIEvent> ev = getNextReply(req, retryCount);
        if( nullptr == ev ) {
            return nullptr;  // timeout
        } else if( !ev->isEvent(HCIEventType::CMD_STATUS) ) {
            DBG_PRINT("HCIHandler::sendWithCmdStatusReply: !CMD_STATUS (drop, retry %d): res %s; req %s",
                       retryCount, ev->toString().c_str(), req.toString().c_str());
            continue; // next packet
        } else {
            *res = static_cast<HCICommandStatusEvent*>(ev.get());
            return ev;
        }
    }
    return nullptr;  // max retry
}

HCIHandler::HCIHandler(const BTMode btMode, const uint16_t dev_id, const int replyTimeoutMS)
:btMode(btMode), dev_id(dev_id), rbuffer(HCI_MAX_MTU),
 comm(dev_id, HCI_CHANNEL_RAW, Defaults::HCI_READER_THREAD_POLL_TIMEOUT), replyTimeoutMS(replyTimeoutMS),
 hciEventRing(HCI_EVT_RING_CAPACITY), hciReaderRunning(false), hciReaderShallStop(false)
{
    INFO_PRINT("HCIHandler.ctor: pid %d", HCIHandler::pidSelf);
    if( !comm.isOpen() ) {
        ERR_PRINT("HCIHandler::open: Could not open hci control channel");
        return;
    }

    std::thread hciReaderThread = std::thread(&HCIHandler::hciReaderThreadImpl, this);
    hciReaderThreadId = hciReaderThread.native_handle();
    // Avoid 'terminate called without an active exception'
    // as l2capReaderThread may end due to I/O errors.
    hciReaderThread.detach();

    PERF_TS_T0();

    // Mandatory socket filter (not adapter filter!)
    {
        hci_ufilter nf, of;
        socklen_t olen;

        olen = sizeof(of);
        if (getsockopt(comm.dd(), SOL_HCI, HCI_FILTER, &of, &olen) < 0) {
            ERR_PRINT("HCIHandler::ctor: getsockopt");
            goto fail;
        }
        // uint16_t opcode_le16 = 0;
        HCIComm::filter_clear(&nf);
        HCIComm::filter_set_ptype(number(HCIPacketType::EVENT),  &nf); // only EVENTs
#if 0
        HCIComm::filter_all_events(&nf); // all events
#else
        HCIComm::filter_set_event(number(HCIEventType::CONN_COMPLETE), &nf);
        HCIComm::filter_set_event(number(HCIEventType::DISCONN_COMPLETE), &nf);
        HCIComm::filter_set_event(number(HCIEventType::CMD_COMPLETE), &nf);
        HCIComm::filter_set_event(number(HCIEventType::CMD_STATUS), &nf);
        HCIComm::filter_set_event(number(HCIEventType::HARDWARE_ERROR), &nf);
        HCIComm::filter_set_event(number(HCIEventType::LE_META), &nf);
        HCIComm::filter_set_event(number(HCIEventType::DISCONN_PHY_LINK_COMPLETE), &nf);
        HCIComm::filter_set_event(number(HCIEventType::DISCONN_LOGICAL_LINK_COMPLETE), &nf);
#endif
        HCIComm::filter_set_opcode(0, &nf); // all opcode
        if (setsockopt(comm.dd(), SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
            ERR_PRINT("HCIHandler::ctor: setsockopt");
            goto fail;
        }
    }
    // Mandatory own LE_META filter
    {
        filter_clear_metaevs();
        // filter_all_metaevs();
        filter_set_metaev(HCIMetaEventType::LE_CONN_COMPLETE);
#ifdef SHOW_LE_ADVERTISING
        filter_set_metaev(HCIMetaEventType::LE_ADVERTISING_REPORT);
#endif
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
        INFO_PRINT("HCIHandler::ctor: LOCAL_VERSION: %d.%d, manuf 0x%x, lmp %d.%d",
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

HCIStatusCode HCIHandler::le_create_conn(uint16_t * handle_return, const EUI48 &peer_bdaddr,
                            const HCIAddressType peer_mac_type,
                            const HCIAddressType own_mac_type,
                            const uint16_t le_scan_interval, const uint16_t le_scan_window,
                            const uint16_t conn_interval_min, const uint16_t conn_interval_max,
                            const uint16_t conn_latency, const uint16_t supervision_timeout) {
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    if( nullptr != handle_return ) {
        *handle_return = 0;
    }
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

    const hci_ev_le_conn_complete * ev_cc;
    HCIStatusCode status;
    std::shared_ptr<HCIEvent> ev = processStructMetaCmd(req0, HCIMetaEventType::LE_CONN_COMPLETE, &ev_cc, &status);

    if( HCIStatusCode::SUCCESS != status ) {
        return status;
    }
    if( nullptr != handle_return ) {
        *handle_return = ev_cc->handle;
    }
    return HCIStatusCode::SUCCESS;
}

HCIStatusCode HCIHandler::create_conn(uint16_t * handle_return, const EUI48 &bdaddr,
                                     const uint16_t pkt_type,
                                     const uint16_t clock_offset, const uint8_t role_switch) {
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    if( nullptr != handle_return ) {
        *handle_return = 0;
    }
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

    const hci_ev_conn_complete * ev_cc;
    HCIStatusCode status;
    std::shared_ptr<HCIEvent> ev = processStructCommand(req0, HCIEventType::CONN_COMPLETE, &ev_cc, &status);

    if( HCIStatusCode::SUCCESS != status ) {
        return status;
    }
    if( nullptr != handle_return ) {
        *handle_return = ev_cc->handle;
    }
    return HCIStatusCode::SUCCESS;
}

HCIStatusCode HCIHandler::disconnect(const uint16_t conn_handle, const HCIStatusCode reason) {
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    if( !comm.isOpen() ) {
        ERR_PRINT("HCIHandler::create_conn: device not open");
        return HCIStatusCode::INTERNAL_FAILURE;
    }
    if( 0 == conn_handle ) {
        return HCIStatusCode::SUCCESS;
    }
    HCIStructCommand<hci_cp_disconnect> req0(HCIOpcode::DISCONNECT);
    hci_cp_disconnect * cp = req0.getWStruct();
    bzero(cp, sizeof(*cp));
    cp->handle = conn_handle;
    cp->reason = number(reason);

    const hci_ev_disconn_complete * ev_cc;
    HCIStatusCode status;
    std::shared_ptr<HCIEvent> ev = processStructCommand(req0, HCIEventType::DISCONN_COMPLETE, &ev_cc, &status);
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
        WARN_PRINT("HCIHandler::processStructCommand %s -> %s: Status 0x%2.2X (%s), errno %d %s: res nullptr, req %s",
                getHCIOpcodeString(opc).c_str(), getHCIEventTypeString(evc).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                req0.toString().c_str());
        return nullptr; // timeout
    } else if( nullptr == ev_cc ) {
        WARN_PRINT("HCIHandler::processStructCommand %s -> %s: Status 0x%2.2X (%s), errno %d %s: res %s, req %s",
                getHCIOpcodeString(opc).c_str(), getHCIEventTypeString(evc).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                ev->toString().c_str(), req0.toString().c_str());
        return ev;
    }
    const uint8_t returnParamSize = ev_cc->getReturnParamSize();
    if( returnParamSize < sizeof(hci_cmd_event_struct) ) {
        WARN_PRINT("HCIHandler::processStructCommand %s -> %s: Status 0x%2.2X (%s), errno %d %s: res %s, req %s",
                getHCIOpcodeString(opc).c_str(), getHCIEventTypeString(evc).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                ev_cc->toString().c_str(), req0.toString().c_str());
        return ev;
    }
    *res = (const hci_cmd_event_struct*)(ev_cc->getReturnParam());
    *status = static_cast<HCIStatusCode>((*res)->status);
    DBG_PRINT("HCIHandler::processStructCommand %s -> %s: Status 0x%2.2X (%s): res %s, req %s",
            getHCIOpcodeString(opc).c_str(), getHCIEventTypeString(evc).c_str(),
            number(*status), getHCIStatusCodeString(*status).c_str(),
            ev_cc->toString().c_str(), req0.toString().c_str());
    return ev;
}

template<typename hci_command_struct, typename hci_cmd_event_struct>
std::shared_ptr<HCIEvent> HCIHandler::processStructCommand(HCIStructCommand<hci_command_struct> &req,
                                                           HCIEventType evc, const hci_cmd_event_struct **res, HCIStatusCode *status)
{
    *res = nullptr;
    *status = HCIStatusCode::INTERNAL_FAILURE;

    if( !sendCommand(req) ) {
        return nullptr;
    }
    int retryCount = 0;
    std::shared_ptr<HCIEvent> ev = nullptr;

    while( retryCount < HCI_READ_PACKET_MAX_RETRY ) {
        ev = getNextReply(req, retryCount);
        if( nullptr == ev ) {
            break; // timeout, leave loop
        } else if( ev->isEvent(evc) ) {
            break; // gotcha, leave loop
        } else if( ev->isEvent(HCIEventType::CMD_STATUS) ) {
            // pending command .. wait for result
            HCICommandStatusEvent * ev_cs = static_cast<HCICommandStatusEvent*>(ev.get());
            if( HCIStatusCode::SUCCESS != ev_cs->getStatus() ) {
                *status = ev_cs->getStatus();
                WARN_PRINT("HCIHandler::processStructCommand %s -> %s: Status 0x%2.2X (%s), errno %d %s: res %s, req %s",
                        getHCIOpcodeString(req.getOpcode()).c_str(), getHCIEventTypeString(evc).c_str(),
                        number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                        ev_cs->toString().c_str(), req.toString().c_str());
                return ev;
            }
            continue; // next packet
        } else {
            continue; // next packet
        }
    }
    if( nullptr == ev ) {
        // timeout exit
        WARN_PRINT("HCIHandler::processStructCommand %s -> %s: Status 0x%2.2X (%s), errno %d %s: res nullptr, req %s",
                getHCIOpcodeString(req.getOpcode()).c_str(), getHCIEventTypeString(evc).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                req.toString().c_str());
        return nullptr;
    }
    typedef HCIStructCmdCompleteEvt<hci_cmd_event_struct> HCIConnCompleteEvt;
    HCIConnCompleteEvt * ev_cc = static_cast<HCIConnCompleteEvt*>(ev.get());
    if( ev_cc->isTypeAndSizeValid(evc) ) {
        *status = ev_cc->getStatus();
        *res = ev_cc->getStruct();
        DBG_PRINT("HCIHandler::processStructCommand %s -> %s: Status 0x%2.2X (%s): res %s, req %s",
                getHCIOpcodeString(req.getOpcode()).c_str(), getHCIEventTypeString(evc).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(),
                ev_cc->toString().c_str(), req.toString().c_str());
    } else {
        WARN_PRINT("HCIHandler::processStructCommand %s -> %s: Status 0x%2.2X (%s), errno %d %s: res %s, req %s",
                getHCIOpcodeString(req.getOpcode()).c_str(), getHCIEventTypeString(evc).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                ev_cc->toString().c_str(), req.toString().c_str());
    }
    return ev;
}

template<typename hci_command_struct, typename hci_cmd_event_struct>
std::shared_ptr<HCIEvent> HCIHandler::processStructMetaCmd(HCIStructCommand<hci_command_struct> &req,
                                                           HCIMetaEventType mec, const hci_cmd_event_struct **res, HCIStatusCode *status)
{
    *res = nullptr;
    *status = HCIStatusCode::INTERNAL_FAILURE;

    if( !sendCommand(req) ) {
        return nullptr;
    }
    int retryCount = 0;
    std::shared_ptr<HCIEvent> ev = nullptr;

    while( retryCount < HCI_READ_PACKET_MAX_RETRY ) {
        ev = getNextReply(req, retryCount);
        if( nullptr == ev ) {
            break; // timeout, leave loop
        } else if( ev->isMetaEvent(mec) ) {
            break; // gotcha, leave loop
        } else if( ev->isEvent(HCIEventType::CMD_STATUS) ) {
            // pending command .. wait for result
            HCICommandStatusEvent * ev_cs = static_cast<HCICommandStatusEvent*>(ev.get());
            if( HCIStatusCode::SUCCESS != ev_cs->getStatus() ) {
                *status = ev_cs->getStatus();
                WARN_PRINT("HCIHandler::processStructCommand %s -> %s: Status 0x%2.2X (%s), errno %d %s: res %s, req %s",
                        getHCIOpcodeString(req.getOpcode()).c_str(), getHCIMetaEventTypeString(mec).c_str(),
                        number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                        ev_cs->toString().c_str(), req.toString().c_str());
                return ev;
            }
            continue; // next packet
        } else {
            continue; // next packet
        }
    }
    if( nullptr == ev ) {
        // timeout exit
        WARN_PRINT("HCIHandler::processStructCommand %s -> %s: Status 0x%2.2X (%s), errno %d %s: res nullptr, req %s",
                getHCIOpcodeString(req.getOpcode()).c_str(), getHCIMetaEventTypeString(mec).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                req.toString().c_str());
        return nullptr;
    }
    typedef HCIStructCmdCompleteMetaEvt<hci_cmd_event_struct> HCIConnCompleteMetaEvt;
    HCIConnCompleteMetaEvt * ev_cc = static_cast<HCIConnCompleteMetaEvt*>(ev.get());
    if( ev_cc->isTypeAndSizeValid(mec) ) {
        *status = ev_cc->getStatus();
        *res = ev_cc->getStruct();
        WARN_PRINT("HCIHandler::processStructCommand %s -> %s: Status 0x%2.2X (%s): res %s, req %s",
                getHCIOpcodeString(req.getOpcode()).c_str(), getHCIMetaEventTypeString(mec).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(),
                ev_cc->toString().c_str(), req.toString().c_str());
    } else {
        WARN_PRINT("HCIHandler::processStructCommand %s -> %s: Type or size mismatch: Status 0x%2.2X (%s), errno %d %s: res %s, req %s",
                getHCIOpcodeString(req.getOpcode()).c_str(), getHCIMetaEventTypeString(mec).c_str(),
                number(*status), getHCIStatusCodeString(*status).c_str(), errno, strerror(errno),
                ev_cc->toString().c_str(), req.toString().c_str());
    }
    return ev;
}
