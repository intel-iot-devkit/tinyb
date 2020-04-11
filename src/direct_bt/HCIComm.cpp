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

#include "dbt_debug.hpp"

namespace direct_bt {

int HCIComm::hci_open_dev(const uint16_t dev_id, const uint16_t channel)
{
	sockaddr_hci a;
	int dd, err;

	/**
	 * dev_id is unsigned and hence always >= 0
	if ( 0 > dev_id ) {
		errno = ENODEV;
		ERR_PRINT("hci_open_dev: invalid dev_id errno %d %s", errno, strerror(errno));
		return -1;
	} */

	// Create a loose HCI socket
	dd = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC, BTPROTO_HCI);
	if (0 > dd ) {
	    perror("HCIComm::hci_open_dev: socket failed");
		return dd;
	}

	// Bind socket to the HCI device
	bzero((void *)&a, sizeof(a));
	a.hci_family = AF_BLUETOOTH;
	a.hci_dev = dev_id;
	a.hci_channel = channel;
	if (bind(dd, (struct sockaddr *) &a, sizeof(a)) < 0) {
	    ERR_PRINT("hci_open_dev: bind failed errno %d %s", errno, strerror(errno));
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
    if( 0 > _dd ) {
        return;
    }
    hci_close_dev(_dd);
    _dd = -1;
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

	hc.opcode = htobs(opcode);
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

	while ( ( bw = writev(_dd, iv, ivn) ) < 0 ) {
	    ERR_PRINT("hci_send_cmd: writev res %d, errno %d %s", bw, errno, strerror(errno));
		if (errno == EAGAIN || errno == EINTR) {
			continue;
		}
		return false;
	}
	DBG_PRINT("hci_send_cmd: writev: %d", bw);
	return true;
}

#define _HCI_PKT_TRY_COUNT 10

bool HCIComm::send_req(const uint16_t opcode, const void *command, const uint8_t command_len,
                       const uint16_t exp_event, void *response, const uint8_t response_len)
{
    if( 0 > _dd ) {
        ERR_PRINT("hci_send_req: device not open");
        return false;
    }
	uint8_t buf[HCI_MAX_EVENT_SIZE];
	const uint16_t opcode_le16 = htobs(opcode);
	hci_ufilter nf, of;
	socklen_t olen;
	int err, tryCount=0;

	DBG_PRINT("hci_send_req: opcode 0x%X ", opcode_le16);

	olen = sizeof(of);
	if (getsockopt(_dd, SOL_HCI, HCI_FILTER, &of, &olen) < 0) {
	    ERR_PRINT("hci_send_req: errno %d %s", errno, strerror(errno));
		return false;
	}

	filter_clear(&nf);
	filter_set_ptype(HCI_EVENT_PKT,  &nf);
	filter_set_event(HCI_EV_CMD_STATUS, &nf);
	filter_set_event(HCI_EV_CMD_COMPLETE, &nf);
	filter_set_event(HCI_EV_LE_META, &nf);
	if( 0 != exp_event ) {
	    filter_set_event(exp_event, &nf);
	}
	filter_set_opcode(opcode_le16, &nf);
	if (setsockopt(_dd, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
	    ERR_PRINT("hci_send_req: errno %d %s", errno, strerror(errno));
		return false;
	}

    int _timeoutMS = timeoutMS;

	if ( !send_cmd(opcode, command, command_len) ) {
	    ERR_PRINT("hci_send_req: errno %d %s", errno, strerror(errno));
		goto failed;
	}

	// FIXME: Review the 10 packet trial for HCI responses,
	//        this is an artifact from the BlueZ client library.
	while ( _HCI_PKT_TRY_COUNT > tryCount++ ) {
		if ( _timeoutMS ) {
			struct pollfd p;
			int n;

			p.fd = _dd; p.events = POLLIN;
			while ((n = poll(&p, 1, _timeoutMS)) < 0) {
			    ERR_PRINT("hci_send_req: poll: errno %d %s", errno, strerror(errno));
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

		while ((len = read(_dd, buf, sizeof(buf))) < 0) {
		    ERR_PRINT("hci_send_req: read: res %d, errno %d %s", len, errno, strerror(errno));
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

                DBG_PRINT("hci_send_req: HCI_EV_CMD_STATUS: opcode 0x%X, exp_event 0x%X, status 0x%2.2X, rlen %d/%d",
                        cs->opcode, exp_event, cs->status, response_len, len);

                if (cs->opcode != opcode_le16) {
                    continue;
                }

                if (exp_event != HCI_EV_CMD_STATUS) {
                    if (cs->status) {
                        errno = EIO;
                        goto failed;
                    }
                    break;
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
                    continue;
                }

                ptr += sizeof(hci_ev_cmd_complete);
                len -= sizeof(hci_ev_cmd_complete);

                const int rlen = MIN(len, response_len);
                memcpy(response, ptr, rlen);
                DBG_PRINT("hci_send_req: HCI_EV_CMD_COMPLETE: copied %d bytes: %s",
                        rlen, bytesHexString((uint8_t*)ptr, 0, rlen, false /* lsbFirst */, true /* leading0X */).c_str());
                goto done;
		    }
		case HCI_EV_REMOTE_NAME: {
                DBG_PRINT("hci_send_req: HCI_EV_REMOTE_NAME: event 0x%X (equal %d), exp_event 0x%X, r-len %d/%d",
                          hdr->evt, exp_event, (hdr->evt == exp_event), response_len, len);

                if (hdr->evt != exp_event) {
                    break;
                }

                const hci_ev_remote_name *rn = static_cast<const hci_ev_remote_name *>(static_cast<const void *>( ptr ));
                const hci_cp_remote_name_req *cp = static_cast<const hci_cp_remote_name_req *>(command);

                if ( rn->bdaddr != cp->bdaddr ) {
                    DBG_PRINT("hci_send_req: HCI_EV_REMOTE_NAME: address mismatch: cmd %s != req %s",
                              cp->bdaddr.toString().c_str(), rn->bdaddr.toString().c_str());
                    continue;
                }

                const int rlen = MIN(len, response_len);
                memcpy(response, ptr, rlen);
                DBG_PRINT("hci_send_req: HCI_EV_REMOTE_NAME: copied %d bytes: %s",
                        rlen, bytesHexString((uint8_t*)ptr, 0, rlen, false /* lsbFirst */, true /* leading0X */).c_str());
                goto done;
		    }

		case HCI_EV_LE_META: {
                const hci_ev_le_meta *me = static_cast<const hci_ev_le_meta *>(static_cast<const void *>( ptr ));

                DBG_PRINT("hci_send_req: HCI_EV_LE_META: subevent 0x%X, exp_event 0x%X (equal %d), r-len %d/%d",
                        me->subevent, exp_event, (me->subevent == exp_event), response_len, len);

                if (me->subevent != exp_event) {
                    continue;
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
                    break;
                }

                const int rlen = MIN(len, response_len);
                memcpy(response, ptr, rlen);
                DBG_PRINT("hci_send_req: DEFAULT: copied %d bytes: %s",
                        rlen, bytesHexString((uint8_t*)ptr, 0, rlen, false /* lsbFirst */, true /* leading0X */).c_str());
                goto done;
		    }
		}
	}
	errno = ETIMEDOUT;

failed:
	err = errno;
	setsockopt(_dd, SOL_HCI, HCI_FILTER, &of, sizeof(of));
	errno = err;
	return false;

done:
	setsockopt(_dd, SOL_HCI, HCI_FILTER, &of, sizeof(of));
	return true;
}

bool HCIComm::le_disconnect(const uint8_t reason)
{
    if( 0 > _dd ) {
        return true;
    }
    if( 0 >= le_conn_handle ) {
        return true;
    }
    hci_ev_disconn_complete rp;
	hci_cp_disconnect cp;

	bzero(&cp, sizeof(cp));
	cp.handle = le_conn_handle;
	cp.reason = reason;

	le_conn_handle = 0;

	if( !send_req( hci_opcode_pack(OGF_LINK_CTL, HCI_OP_DISCONNECT), &cp, sizeof(cp),
	               HCI_EV_DISCONN_COMPLETE, &rp, sizeof(rp) ) )
	{
	    DBG_PRINT("hci_le_disconnect: errno %d %s", errno, strerror(errno));
		return false;
	}

	if (rp.status) {
		errno = EIO;
		DBG_PRINT("hci_le_disconnect: error status 0x%2.2X, errno %d %s", rp.status, errno, strerror(errno));
		return false;
	}
	return true;
}

bool HCIComm::le_set_scan_enable(const uint8_t enable, const uint8_t filter_dup) {
    if( 0 > _dd ) {
        ERR_PRINT("hci_le_set_scan_enable: device not open");
        return false;
    }
	hci_cp_le_set_scan_enable cp;
	uint8_t status;

	bzero(&cp, sizeof(cp));
	cp.enable = enable;
	cp.filter_dup = filter_dup;

    if( !send_req( hci_opcode_pack(OGF_LE_CTL, HCI_OP_LE_SET_SCAN_ENABLE), &cp, sizeof(cp),
                   0, &status, sizeof(status) ) )
    {
        ERR_PRINT("hci_le_set_scan_enable: errno %d %s", errno, strerror(errno));
        return false;
    }

	if (status) {
		errno = EIO;
		ERR_PRINT("hci_le_set_scan_enable: error status 0x%2.2X, errno %d %s", status, errno, strerror(errno));
		return false;
	}
	return true;
}

bool HCIComm::le_set_scan_parameters(const uint8_t type, const uint16_t interval,
                                     const uint16_t window, const uint8_t own_type,
                                     const uint8_t filter)
{
    if( 0 > _dd ) {
        ERR_PRINT("hci_le_set_scan_parameters: device not open");
        return false;
    }
	hci_cp_le_set_scan_param cp;
	uint8_t status;

	bzero(&cp, sizeof(cp));
	cp.type = type;
	cp.interval = cpu_to_le(interval);
	cp.window = cpu_to_le(window);
	cp.own_address_type = own_type;
	cp.filter_policy = filter;

    if( !send_req( hci_opcode_pack(OGF_LE_CTL, HCI_OP_LE_SET_SCAN_PARAM), &cp, sizeof(cp),
                   0, &status, sizeof(status) ) )
    {
        ERR_PRINT("hci_le_set_scan_parameters: errno %d %s", errno, strerror(errno));
        return false;
    }

	if (status) {
		errno = EIO;
        ERR_PRINT("hci_le_set_scan_parameters: error status 0x%2.2X, errno %d %s", status, errno, strerror(errno));
		return false;
	}

	return true;
}

void HCIComm::le_disable_scan() {
    if( 0 > _dd ) {
        return;
    }
    if( !le_scanning ) {
        return;
    }
    const uint8_t filter_dup = 0x01;

    if( !le_set_scan_enable(0x00, filter_dup) ) {
        perror("Stop scan failed");
    } else {
        le_scanning = false;
    }
}

bool HCIComm::le_enable_scan(const uint8_t own_type,
                             const uint16_t interval, const uint16_t window) {
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
        perror("Set scan parameters failed");
        return false;
    }

    ok = le_set_scan_enable(0x01, filter_dup);
    if ( !ok ) {
        perror("Start scan failed");
        return false;
    }
    le_scanning = true;
    return true;
}

uint16_t HCIComm::le_create_conn(const EUI48 &peer_bdaddr,
                                 const uint8_t peer_mac_type,
                                 const uint8_t own_mac_type,
                                 const uint16_t interval, const uint16_t window,
                                 const uint16_t min_interval, const uint16_t max_interval,
                                 const uint16_t latency, const uint16_t supervision_timeout,
                                 const uint16_t min_ce_length, const uint16_t max_ce_length,
                                 const uint8_t initiator_filter)
{
    if( 0 > _dd ) {
        ERR_PRINT("hci_le_create_conn: device not open");
        return 0;
    }
    if( 0 < le_conn_handle ) {
        DBG_PRINT("hci_le_create_conn: already connected 0x%X", le_conn_handle);
        return le_conn_handle;
    }
	hci_cp_le_create_conn cp;
	hci_ev_le_conn_complete rp;

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

    if( !send_req( hci_opcode_pack(OGF_LE_CTL, HCI_OP_LE_CREATE_CONN), &cp, sizeof(cp),
                   HCI_EV_LE_CONN_COMPLETE, &rp, sizeof(rp) ) )
    {
        ERR_PRINT("hci_le_create_conn: errno %d %s", errno, strerror(errno));
        return 0;
    }

	if (rp.status) {
		errno = EIO;
        ERR_PRINT("hci_le_create_conn: error status 0x%2.2X, errno %d %s", rp.status, errno, strerror(errno));
		return 0;
	}

	le_conn_handle = rp.handle;

	return le_conn_handle;
}

} /* namespace direct_bt */
