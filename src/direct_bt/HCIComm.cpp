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

#include "HCIComm.hpp"

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

#define HCI_ERROR_CODE(X) \
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
        X(CONNECTION_EST_FAILED_OR_SYNC_TIMETOUT) \
        X(MAX_CONNECTION_FAILED) \
        X(COARSE_CLOCK_ADJ_REJECTED) \
        X(TYPE0_SUBMAP_NOT_DEFINED) \
        X(UNKNOWN_ADVERTISING_IDENTIFIER) \
        X(LIMIT_REACHED) \
        X(OPERATION_CANCELLED_BY_HOST) \
        X(PACKET_TOO_LONG) \
        X(INTERNAL_FAILURE) \
        X(UNKNOWN)

#define HCI_ERROR_CODE_CASE_TO_STRING(V) case HCIErrorCode::V: return #V;

std::string getHCIErrorCodeString(const HCIErrorCode ec) {
    switch(ec) {
    HCI_ERROR_CODE(HCI_ERROR_CODE_CASE_TO_STRING)
        default: ; // fall through intended
    }
    return "Unknown HCI error code";
}

int HCIComm::hci_open_dev(const uint16_t dev_id, const uint16_t channel)
{
	sockaddr_hci a;
	int dd, err;

	/**
	 * dev_id is unsigned and hence always >= 0
	if ( 0 > dev_id ) {
		errno = ENODEV;
		ERR_PRINT("hci_open_dev: invalid dev_id");
		return -1;
	} */

	// Create a loose HCI socket
	dd = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (0 > dd ) {
        ERR_PRINT("HCIComm::hci_open_dev: socket failed");
		return dd;
	}

	// Bind socket to the HCI device
	bzero((void *)&a, sizeof(a));
	a.hci_family = AF_BLUETOOTH;
	a.hci_dev = dev_id;
	a.hci_channel = channel;
	if (bind(dd, (struct sockaddr *) &a, sizeof(a)) < 0) {
	    ERR_PRINT("hci_open_dev: bind failed");
		goto failed;
	}

	return dd;

failed:
	err = errno;
	::close(dd);
	errno = err;

	return -1;
}

int HCIComm::hci_close_dev(int dd)
{
	return ::close(dd);
}

void HCIComm::close() {
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    if( 0 > _dd ) {
        return;
    }
    hci_close_dev(_dd);
    _dd = -1;
}

int HCIComm::read(uint8_t* buffer, const int capacity) {
    return read(buffer, capacity, timeoutMS);
}
int HCIComm::read(uint8_t* buffer, const int capacity, const int timeoutMS) {
    int len = 0;
    if( 0 > _dd || 0 > capacity ) {
        goto errout;
    }
    if( 0 == capacity ) {
        goto done;
    }

    if( timeoutMS ) {
        struct pollfd p;
        int n;

        p.fd = _dd; p.events = POLLIN;
#if 0
        sigset_t sigmask;
        sigemptyset(&sigmask);
        // sigaddset(&sigmask, SIGALRM);
        struct timespec timeout_ts;
        timeout_ts.tv_sec=0;
        timeout_ts.tv_nsec=(long)timeoutMS*1000000L;
        while ((n = ppoll(&p, 1, &timeout_ts, &sigmask)) < 0) {
#else
        while ((n = poll(&p, 1, timeoutMS)) < 0) {
#endif
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

    while ((len = ::read(_dd, buffer, capacity)) < 0) {
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

int HCIComm::write(const uint8_t* buffer, const int size) {
    int len = 0;
    if( 0 > _dd || 0 > size ) {
        goto errout;
    }
    if( 0 == size ) {
        goto done;
    }

    while( ( len = ::write(_dd, buffer, size) ) < 0 ) {
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

bool HCIComm::send_cmd(const uint16_t opcode, const void *command, const uint8_t command_len)
{
    if( 0 > _dd ) {
        ERR_PRINT("hci_send_cmd: device not open");
        return false;
    }
	const uint8_t type = HCI_COMMAND_PKT;
	hci_command_hdr hc;
	struct iovec iv[3];
	int ivn;
	int bw=0;

	hc.opcode = cpu_to_le(opcode);
	hc.plen= command_len;

	iv[0].iov_base = (void*)&type;
	iv[0].iov_len  = 1;
	iv[1].iov_base = (void*)&hc;
	iv[1].iov_len  = HCI_COMMAND_HDR_SIZE;
	ivn = 2;

	if (command_len) {
		iv[2].iov_base = (void*)command;
		iv[2].iov_len  = command_len;
		ivn = 3;
	}

#ifdef VERBOSE_ON
	{
        fprintf(stderr, "hci_send_cmd: type 0x%2.2X, opcode 0x%X, plen %d\n", type, hc.opcode, command_len);
        std::string paramstr = command_len > 0 ? bytesHexString((uint8_t*)command, 0, command_len, false /* lsbFirst */, true /* leading0X */) : "";
        fprintf(stderr, "hci_send_cmd: param: %s\n", paramstr.c_str());
	}
#endif

	while ( ( bw = ::writev(_dd, iv, ivn) ) < 0 ) {
	    ERR_PRINT("hci_send_cmd: writev res %d", bw);
		if (errno == EAGAIN || errno == EINTR) {
			continue;
		}
		return false;
	}
	DBG_PRINT("hci_send_cmd: writev: %d", bw);
	return true;
}

#define _HCI_PKT_TRY_COUNT 10

HCIErrorCode HCIComm::send_req(const uint16_t opcode, const void *command, const uint8_t command_len,
                               const uint16_t exp_event, void *response, const uint8_t response_len)
{
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    if( 0 > _dd ) {
        ERR_PRINT("hci_send_req: device not open");
        return HCIErrorCode::INTERNAL_FAILURE;
    }
	uint8_t buf[HCI_MAX_EVENT_SIZE];
	const uint16_t opcode_le16 = cpu_to_le(opcode);
	hci_ufilter nf, of;
	socklen_t olen;
	int err, tryCount=0;

	DBG_PRINT("hci_send_req: opcode 0x%X ", opcode_le16);

	olen = sizeof(of);
	if (getsockopt(_dd, SOL_HCI, HCI_FILTER, &of, &olen) < 0) {
	    ERR_PRINT("hci_send_req");
		return HCIErrorCode::INTERNAL_FAILURE;
	}

	filter_clear(&nf);
	filter_set_ptype(HCI_EVENT_PKT,  &nf);
	filter_set_event(HCI_EV_CMD_STATUS, &nf);
	filter_set_event(HCI_EV_CMD_COMPLETE, &nf);
	filter_set_event(HCI_EV_LE_META, &nf);
    filter_set_event(exp_event, &nf);
	filter_set_opcode(opcode_le16, &nf);
	if (setsockopt(_dd, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
	    ERR_PRINT("hci_send_req");
		return HCIErrorCode::INTERNAL_FAILURE;
	}

    int _timeoutMS = timeoutMS;
    HCIErrorCode res = HCIErrorCode::INTERNAL_FAILURE;

	if ( !send_cmd(opcode, command, command_len) ) {
	    ERR_PRINT("hci_send_req");
		goto failed;
	}

	// Reading up to 10 packets for HCI responses,
	while ( _HCI_PKT_TRY_COUNT > tryCount++ ) {
		if ( _timeoutMS ) {
			struct pollfd p;
			int n;

            p.fd = _dd; p.events = POLLIN;
#if 0
			sigset_t sigmask;
			sigemptyset(&sigmask);
			sigaddset(&sigmask, SIGALRM);
			struct timespec timeout_ts;
			timeout_ts.tv_sec=0;
			timeout_ts.tv_nsec=(long)_timeoutMS*1000000L;
			while ((n = ppoll(&p, 1, &timeout_ts, &sigmask)) < 0) {
#else
            while ((n = poll(&p, 1, _timeoutMS)) < 0) {
#endif
			    ERR_PRINT("HCIComm::send_req(dev_id %d, channel %d): poll: res %d", dev_id, channel, n);
				if (errno == EAGAIN || errno == EINTR) {
					continue;
				}
				goto failed;
			}

			if (!n) {
			    DBG_PRINT("hci_send_req: poll: timeout");
				errno = ETIMEDOUT;
				goto failed;
			}

			_timeoutMS -= _timeoutMS/_HCI_PKT_TRY_COUNT; // reduce timeout consecutively
			if ( _timeoutMS < 0 ) {
			    _timeoutMS = 0;
			}
		}

		int len;

		while ((len = ::read(_dd, buf, sizeof(buf))) < 0) {
		    ERR_PRINT("HCIComm::send_req(dev_id %d, channel %d): read: res %d", dev_id, channel, len);
			if (errno == EAGAIN || errno == EINTR) {
				continue;
			}
			goto failed;
		}

	    const hci_event_hdr *hdr = static_cast<hci_event_hdr *>(static_cast<void *>(buf + 1));
		const uint8_t * ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
		len -= (1 + HCI_EVENT_HDR_SIZE);

        DBG_PRINT("hci_send_req: read: %d bytes, evt 0x%2.2X", len, hdr->evt);

		switch (hdr->evt) {
		case HCI_EV_CMD_STATUS: {
                const hci_ev_cmd_status *cs = static_cast<const hci_ev_cmd_status *>(static_cast<const void *>( ptr ));
                const HCIErrorCode status = static_cast<HCIErrorCode>(cs->status);

                DBG_PRINT("hci_send_req: HCI_EV_CMD_STATUS: opcode 0x%X, exp_event 0x%X, status 0x%2.2X (%s), rlen %d/%d",
                        cs->opcode, exp_event,
                        cs->status, getHCIErrorCodeString(status).c_str(),
                        response_len, len);

                if (cs->opcode != opcode_le16) {
                    continue; // next packet
                }

                if (exp_event != HCI_EV_CMD_STATUS) {
                    if ( HCIErrorCode::SUCCESS != status ) {
                        errno = EIO;
                        DBG_PRINT("hci_send_req: event exp 0x%X != has 0x%X, error status 0x%2.2X (%s)", exp_event,
                                  hdr->evt, cs->status, getHCIErrorCodeString(status).c_str());
                        res = status;
                        goto failed;
                    }
                    continue; // next packet
                }

                const int rlen = MIN(len, response_len);
                memcpy(response, ptr, rlen);
                DBG_PRINT("hci_send_req: HCI_EV_CMD_STATUS: copied %d bytes: %s",
                        rlen, bytesHexString((uint8_t*)ptr, 0, rlen, false /* lsbFirst */, true /* leading0X */).c_str());
                goto done;
		    }

		case HCI_EV_CMD_COMPLETE: {
                const hci_ev_cmd_complete *cc = static_cast<const hci_ev_cmd_complete *>(static_cast<const void *>( ptr ));

                DBG_PRINT("hci_send_req: HCI_EV_CMD_COMPLETE: opcode 0x%X (equal %d), exp_event 0x%X, r-len %d/%d",
                        cc->opcode, (cc->opcode == opcode_le16), exp_event, response_len, len);

                if (cc->opcode != opcode_le16) {
                    continue; // next packet
                }

                ptr += sizeof(hci_ev_cmd_complete);
                len -= sizeof(hci_ev_cmd_complete);

                const int rlen = MIN(len, response_len);
                memcpy(response, ptr, rlen);
                DBG_PRINT("hci_send_req: HCI_EV_CMD_COMPLETE: copied %d bytes: %s",
                        rlen, bytesHexString((uint8_t*)ptr, 0, rlen, false /* lsbFirst */, true /* leading0X */).c_str());
                goto done;
		    }

		case HCI_EV_LE_META: {
                const hci_ev_le_meta *me = static_cast<const hci_ev_le_meta *>(static_cast<const void *>( ptr ));

                DBG_PRINT("hci_send_req: HCI_EV_LE_META: subevent 0x%X, exp_event 0x%X (equal %d), r-len %d/%d",
                        me->subevent, exp_event, (me->subevent == exp_event), response_len, len);

                if (me->subevent != exp_event) {
                    continue; // next packet
                }

                ptr += 1;
                len -= 1;

                const int rlen = MIN(len, response_len);
                memcpy(response, ptr, rlen);
                DBG_PRINT("hci_send_req: HCI_EV_LE_META: copied %d bytes: %s",
                        rlen, bytesHexString((uint8_t*)ptr, 0, rlen, false /* lsbFirst */, true /* leading0X */).c_str());
                goto done;
		    }

		default: {
                DBG_PRINT("hci_send_req: DEFAULT: evt 0x%X, exp_event 0x%X (equal %d), r-len %d/%d",
                        hdr->evt, exp_event, (hdr->evt == exp_event), response_len, len);

                if (hdr->evt != exp_event) {
                    continue; // next packet
                }

                const int rlen = MIN(len, response_len);
                memcpy(response, ptr, rlen);
                DBG_PRINT("hci_send_req: DEFAULT: copied %d bytes: %s",
                        rlen, bytesHexString((uint8_t*)ptr, 0, rlen, false /* lsbFirst */, true /* leading0X */).c_str());
                goto done;
		    }
		} // switch event
	} // while packets ...
	errno = ETIMEDOUT;

failed:
	err = errno;
	setsockopt(_dd, SOL_HCI, HCI_FILTER, &of, sizeof(of));
	errno = err;
	return res;

done:
	setsockopt(_dd, SOL_HCI, HCI_FILTER, &of, sizeof(of));
	return HCIErrorCode::SUCCESS;
}

bool HCIComm::disconnect(const uint16_t le_conn_handle, const HCIErrorCode reason)
{
    /** BT Core Spec v5.2: Vol 4, Part E HCI: 7.1.6 Disconnect command */
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    DBG_PRINT("hci_disconnect: handle 0x%x, reason 0x%x (%s)",
              le_conn_handle, static_cast<uint8_t>(reason), getHCIErrorCodeString(reason).c_str());
    if( 0 > _dd ) {
        return true;
    }
    if( 0 == le_conn_handle ) {
        return true;
    }
    hci_ev_disconn_complete rp;
	hci_cp_disconnect cp;

	bzero(&cp, sizeof(cp));
	cp.handle = le_conn_handle;
	cp.reason = static_cast<uint8_t>(reason);

    HCIErrorCode res = send_req( hci_opcode_pack(OGF_LINK_CTL, HCI_OP_DISCONNECT), &cp, sizeof(cp),
                                 HCI_EV_DISCONN_COMPLETE, &rp, sizeof(rp) );
    if( HCIErrorCode::SUCCESS != res ) {
        ERR_PRINT("hci_disconnect: 0x%2.2X (%s), errno %d %s",
                static_cast<uint8_t>(res), getHCIErrorCodeString(res).c_str(), errno, strerror(errno));
        return false;
    }
    HCIErrorCode status = static_cast<HCIErrorCode>(rp.status);
    if( HCIErrorCode::SUCCESS != status ) {
        errno = EIO;
        ERR_PRINT("hci_disconnect: error status 0x%2.2X (%s), errno %d %s",
                rp.status, getHCIErrorCodeString(status).c_str(), errno, strerror(errno));
        return false;
    }
	return true;
}

bool HCIComm::le_set_scan_enable(const uint8_t enable, const uint8_t filter_dup) {
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    if( 0 > _dd ) {
        ERR_PRINT("hci_le_set_scan_enable(%d): device not open", enable);
        return false;
    }
	hci_cp_le_set_scan_enable cp;
	HCIErrorCode status;

	bzero(&cp, sizeof(cp));
	cp.enable = enable;
	cp.filter_dup = filter_dup;

    HCIErrorCode res = send_req( hci_opcode_pack(OGF_LE_CTL, HCI_OP_LE_SET_SCAN_ENABLE), &cp, sizeof(cp),
                                 0, &status, sizeof(status) );
    if( HCIErrorCode::SUCCESS != res ) {
        ERR_PRINT("hci_le_set_scan_enable(%d): 0x%2.2X (%s), errno %d %s",
                enable, static_cast<uint8_t>(res), getHCIErrorCodeString(res).c_str(), errno, strerror(errno));
        return false;
    }
    if( HCIErrorCode::SUCCESS != status ) {
        errno = EIO;
        ERR_PRINT("hci_le_set_scan_enable(%d): error status 0x%2.2X (%s), errno %d %s",
                enable, static_cast<uint8_t>(status), getHCIErrorCodeString(status).c_str(), errno, strerror(errno));
        return false;
    }
	return true;
}

bool HCIComm::le_set_scan_parameters(const uint8_t type, const uint16_t interval,
                                     const uint16_t window, const uint8_t own_type,
                                     const uint8_t filter)
{
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    if( 0 > _dd ) {
        ERR_PRINT("hci_le_set_scan_parameters: device not open");
        return false;
    }
	hci_cp_le_set_scan_param cp;
	HCIErrorCode status;

	bzero(&cp, sizeof(cp));
	cp.type = type;
	cp.interval = cpu_to_le(interval);
	cp.window = cpu_to_le(window);
	cp.own_address_type = own_type;
	cp.filter_policy = filter;

    HCIErrorCode res = send_req( hci_opcode_pack(OGF_LE_CTL, HCI_OP_LE_SET_SCAN_PARAM), &cp, sizeof(cp),
                                 0, &status, sizeof(status) );
    if( HCIErrorCode::SUCCESS != res ) {
        ERR_PRINT("hci_le_set_scan_parameters: 0x%2.2X (%s), errno %d %s",
                static_cast<uint8_t>(res), getHCIErrorCodeString(res).c_str(), errno, strerror(errno));
        return false;
    }
    if( HCIErrorCode::SUCCESS != status ) {
        errno = EIO;
        ERR_PRINT("hci_le_set_scan_parameters: error status 0x%2.2X (%s), errno %d %s",
                static_cast<uint8_t>(status), getHCIErrorCodeString(status).c_str(), errno, strerror(errno));
        return false;
    }
	return true;
}

void HCIComm::le_disable_scan() {
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    if( 0 > _dd ) {
        return;
    }
    if( !le_scanning ) {
        return;
    }
    const uint8_t filter_dup = 0x01;

    if( !le_set_scan_enable(0x00, filter_dup) ) {
        ERR_PRINT("Stop scan failed");
    } else {
        le_scanning = false;
    }
}

bool HCIComm::le_enable_scan(const HCIAddressType own_type,
                             const uint16_t interval, const uint16_t window) {
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    if( 0 > _dd ) {
        DBG_PRINT("hci_le_enable_scan: device not open");
        return false;
    }
    if( le_scanning ) {
        DBG_PRINT("hci_le_enable_scan: device already le-scanning");
        return false;
    }
    const uint8_t scan_type = 0x01;
    // const uint8_t filter_type = 0;
    const uint8_t filter_policy = 0x00;
    const uint8_t filter_dup = 0x01;

    bool ok = le_set_scan_parameters(scan_type, interval, window,
                                     own_type, filter_policy);
    if ( !ok ) {
        ERR_PRINT("Set scan parameters failed");
        return false;
    }

    ok = le_set_scan_enable(0x01, filter_dup);
    if ( !ok ) {
        ERR_PRINT("Start scan failed");
        return false;
    }
    le_scanning = true;
    return true;
}

HCIErrorCode HCIComm::le_create_conn(uint16_t * handle_return, const EUI48 &peer_bdaddr,
                                     const HCIAddressType peer_mac_type,
                                     const HCIAddressType own_mac_type,
                                     const uint16_t interval, const uint16_t window,
                                     const uint16_t min_interval, const uint16_t max_interval,
                                     const uint16_t latency, const uint16_t supervision_timeout)
{

    /** BT Core Spec v5.2: Vol 4, Part E HCI: 7.8.12 LE Create Connection command */
    /** BT Core Spec v5.2: Vol 4, Part E HCI: 7.7.65 LE Meta event: 7.7.65.1 LE Connection Complete event */
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    if( nullptr != handle_return ) {
        *handle_return = 0;
    }
    if( 0 > _dd ) {
        ERR_PRINT("hci_le_create_conn: device not open");
        return HCIErrorCode::INTERNAL_FAILURE;
    }
	hci_cp_le_create_conn cp;
	hci_ev_le_conn_complete rp;

    const uint16_t min_ce_length = 0x0000;
    const uint16_t max_ce_length = 0x0000;
    const uint8_t initiator_filter = 0x00; // whitelist not used but peer_bdaddr*

	bzero((void*)&cp, sizeof(cp));
	cp.scan_interval = cpu_to_le(interval);
	cp.scan_window = cpu_to_le(window);
	cp.filter_policy = initiator_filter;
	cp.peer_addr_type = peer_mac_type;
	cp.peer_addr = peer_bdaddr;
	cp.own_address_type = own_mac_type;
	cp.conn_interval_min = cpu_to_le(min_interval);
	cp.conn_interval_max = cpu_to_le(max_interval);
	cp.conn_latency = cpu_to_le(latency);
	cp.supervision_timeout = cpu_to_le(supervision_timeout);
	cp.min_ce_len = cpu_to_le(min_ce_length);
	cp.max_ce_len = cpu_to_le(max_ce_length);

	HCIErrorCode res = send_req( hci_opcode_pack(OGF_LE_CTL, HCI_OP_LE_CREATE_CONN), &cp, sizeof(cp),
                                 HCI_EV_LE_CONN_COMPLETE, &rp, sizeof(rp) );
	if( HCIErrorCode::COMMAND_DISALLOWED == res ) {
        WARN_PRINT("hci_le_create_conn: COMMAND_DISALLOWED reply, connect may still be in progress. error status 0x%2.2X (%s), errno %d %s",
                  static_cast<uint8_t>(res), getHCIErrorCodeString(res).c_str(), errno, strerror(errno));
        return res;
	}
	if( HCIErrorCode::SUCCESS != res ) {
        ERR_PRINT("hci_le_create_conn: error status 0x%2.2X (%s), errno %d %s",
                static_cast<uint8_t>(res), getHCIErrorCodeString(res).c_str(), errno, strerror(errno));
        return res;
    }
    HCIErrorCode status = static_cast<HCIErrorCode>(rp.status);
    if( HCIErrorCode::SUCCESS != status ) {
        errno = EIO;
        ERR_PRINT("hci_le_create_conn: error status 0x%2.2X (%s), errno %d %s",
                rp.status, getHCIErrorCodeString(status).c_str(), errno, strerror(errno));
        return status;
    }
    DBG_PRINT("hci_le_create_conn: Success: handle 0x%x", rp.handle);
    if( nullptr != handle_return ) {
        *handle_return = rp.handle;
    }
    return HCIErrorCode::SUCCESS;
}

HCIErrorCode HCIComm::create_conn(uint16_t * handle_return, const EUI48 &bdaddr, const uint16_t pkt_type,
                                  const uint16_t clock_offset, const uint8_t role_switch)
{
    const std::lock_guard<std::recursive_mutex> lock(mtx); // RAII-style acquire and relinquish via destructor
    if( nullptr != handle_return ) {
        *handle_return = 0;
    }
    if( 0 > _dd ) {
        ERR_PRINT("hci_create_conn: device not open");
        return HCIErrorCode::INTERNAL_FAILURE;
    }
    hci_cp_create_conn cp;
    hci_ev_conn_complete rp;

    bzero((void*)&cp, sizeof(cp));
    cp.bdaddr = bdaddr;
    cp.pkt_type = cpu_to_le((uint16_t)(pkt_type & (uint16_t)ACL_PTYPE_MASK)); /* TODO OK excluding SCO_PTYPE_MASK   (HCI_HV1 | HCI_HV2 | HCI_HV3) ? */
    cp.pscan_rep_mode = 0x02; /* TODO magic? */
    cp.pscan_mode = 0x00; /* TODO magic? */
    cp.clock_offset = cpu_to_le(clock_offset);
    cp.role_switch = role_switch;

    HCIErrorCode res = send_req( hci_opcode_pack(OGF_LINK_CTL, HCI_OP_CREATE_CONN), &cp, sizeof(cp),
                                 HCI_EV_CONN_COMPLETE, &rp, sizeof(rp) );
    if( HCIErrorCode::SUCCESS != res ) {
        ERR_PRINT("hci_create_conn: error status 0x%2.2X (%s), errno %d %s",
                static_cast<uint8_t>(res), getHCIErrorCodeString(res).c_str(), errno, strerror(errno));
        return res;
    }
    HCIErrorCode status = static_cast<HCIErrorCode>(rp.status);
    if( HCIErrorCode::SUCCESS != status ) {
        errno = EIO;
        ERR_PRINT("hci_create_conn: error status 0x%2.2X (%s), errno %d %s",
                rp.status, getHCIErrorCodeString(status).c_str(), errno, strerror(errno));
        return status;
    }
    if( nullptr != handle_return ) {
        *handle_return = rp.handle;
    }
    return HCIErrorCode::SUCCESS;
}

} /* namespace direct_bt */
