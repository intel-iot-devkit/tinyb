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

#include "BTIoctl.hpp"
#include "HCIIoctl.hpp"
#include "L2CAPIoctl.hpp"

#include "HCIComm.hpp"
#include "L2CAPComm.hpp"
#include "DBTAdapter.hpp"

extern "C" {
    #include <unistd.h>
    #include <sys/socket.h>
    #include <poll.h>
    #include <signal.h>
}

// #define VERBOSE_ON 1
#include <dbt_debug.hpp>

using namespace direct_bt;

int L2CAPComm::l2cap_open_dev(const EUI48 & adapterAddress, const uint16_t psm, const uint16_t cid, const bool pubaddrAdapter) {
    sockaddr_l2 a;
    int dd, err;

    // Create a loose L2CAP socket
    dd = ::socket(AF_BLUETOOTH, // AF_BLUETOOTH == PF_BLUETOOTH
                  SOCK_SEQPACKET, BTPROTO_L2CAP);

    if( 0 > dd ) {
        ERR_PRINT("L2CAPComm::l2cap_open_dev: socket failed");
        return dd;
    }

    // Bind socket to the L2CAP adapter
    // BT Core Spec v5.2: Vol 3, Part A: L2CAP_CONNECTION_REQ
    bzero((void *)&a, sizeof(a));
    a.l2_family=AF_BLUETOOTH;
    a.l2_psm = cpu_to_le(psm);
    a.l2_bdaddr = adapterAddress;
    a.l2_cid = cpu_to_le(cid);
    a.l2_bdaddr_type = pubaddrAdapter ? BDADDR_LE_PUBLIC : BDADDR_LE_RANDOM;
    if ( bind(dd, (struct sockaddr *) &a, sizeof(a)) < 0 ) {
        ERR_PRINT("L2CAPComm::l2cap_open_dev: bind failed");
        goto failed;
    }
    return dd;

failed:
    err = errno;
    close(dd);
    errno = err;

    return -1;
}

int L2CAPComm::l2cap_close_dev(int dd)
{
    return close(dd);
}


// *************************************************
// *************************************************
// *************************************************

bool L2CAPComm::connect() {
    /** BT Core Spec v5.2: Vol 3, Part A: L2CAP_CONNECTION_REQ */
    bool expConn = false; // C++11, exp as value since C++20
    if( !isConnected.compare_exchange_strong(expConn, true) ) {
        // already connected
        DBG_PRINT("L2CAPComm::connect: Already connected: %s, dd %d, %s, psm %u, cid %u, pubDevice %d",
                getStateString(), _dd.load(), device->getAddress().toString().c_str(), psm, cid, pubaddr);
        if( 0 > _dd ) {
            throw InternalError("connected but _dd "+std::to_string(_dd.load())+" < 0", E_FILE_LINE);
        }
        return true;
    }
    hasIOError = false;
    DBG_PRINT("L2CAPComm::connect: Start: %s, dd %d, %s, psm %u, cid %u, pubDevice %d",
            getStateString(), _dd.load(), device->getAddress().toString().c_str(), psm, cid, pubaddr);

    sockaddr_l2 req;
    int err, res;
    int to_retry_count=0; // ETIMEDOUT retry count

    _dd = l2cap_open_dev(device->getAdapter().getAddress(), psm, cid, true /* pubaddrAdapter */);
    if( 0 > _dd ) {
        goto failure; // open failed
    }

    DBG_PRINT("L2CAPComm::connect: Connect %s, psm %u, cid %u, pubDevice %d",
            device->getAddress().toString().c_str(), psm, cid, pubaddr);

    tid_connect = pthread_self(); // temporary safe tid to allow interruption

    // actual request to connect to remote device
    bzero((void *)&req, sizeof(req));
    req.l2_family = AF_BLUETOOTH;
    req.l2_psm = cpu_to_le(psm);
    req.l2_bdaddr = device->getAddress();
    req.l2_cid = cpu_to_le(cid);
    req.l2_bdaddr_type = pubaddr ? BDADDR_LE_PUBLIC : BDADDR_LE_RANDOM;

    while( !interruptFlag ) {
        // blocking
        res = ::connect(_dd, (struct sockaddr*)&req, sizeof(req));

        DBG_PRINT("L2CAPComm::connect: Result %d, errno 0%X %s, %s",
                res, errno, strerror(errno), device->getAddress().toString().c_str());

        if( !res )
        {
            break; // done

        } else if( ETIMEDOUT == errno ) {
            to_retry_count++;
            if( to_retry_count < number(Defaults::L2CAP_CONNECT_MAX_RETRY) ) {
                INFO_PRINT("L2CAPComm::connect: timeout, retry %d", to_retry_count);
                continue;
            } else {
                ERR_PRINT("L2CAPComm::connect: timeout, retried %d", to_retry_count);
                goto failure; // exit
            }

        } else  {
            // EALREADY == errno || ENETUNREACH == errno || EHOSTUNREACH == errno || ..
            ERR_PRINT("L2CAPComm::connect: connect failed");
            goto failure; // exit
        }
    }
    tid_connect = 0;

    return true;

failure:
    tid_connect = 0;
    err = errno;
    disconnect();
    errno = err;
    return false;
}

bool L2CAPComm::disconnect() {
    bool expConn = true; // C++11, exp as value since C++20
    if( !isConnected.compare_exchange_strong(expConn, false) ) {
        DBG_PRINT("L2CAPComm::disconnect: Not connected: %s, dd %d, %s, psm %u, cid %u, pubDevice %d",
                getStateString().c_str(), _dd.load(), device->getAddress().toString().c_str(), psm, cid, pubaddr);
        if( 0 <= _dd ) {
            throw InternalError("!connected but _dd "+std::to_string(_dd.load())+" >= 0", E_FILE_LINE);
        }
        return false;
    }
    hasIOError = false;
    DBG_PRINT("L2CAPComm::disconnect: Start: %s, dd %d, %s, psm %u, cid %u, pubDevice %d",
            getStateString().c_str(), _dd.load(), device->getAddress().toString().c_str(), psm, cid, pubaddr);
    interruptFlag = true;

    // interrupt L2CAP ::connect(..), avoiding prolonged hang
    pthread_t _tid_connect = tid_connect;
    tid_connect = 0;
    if( 0 != _tid_connect ) {
        pthread_t tid_self = pthread_self();
        if( tid_self != _tid_connect ) {
            int kerr;
            if( 0 != ( kerr = pthread_kill(_tid_connect, SIGALRM) ) ) {
                ERR_PRINT("L2CAP::disconnect: pthread_kill %p FAILED: %d", (void*)_tid_connect, kerr);
            }
        }
    }

    l2cap_close_dev(_dd);
    _dd = -1;
    interruptFlag = false;
    DBG_PRINT("L2CAPComm::disconnect: End dd %d", _dd.load());
    return true;
}

int L2CAPComm::read(uint8_t* buffer, const int capacity, const int timeoutMS) {
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
        while ( !interruptFlag && (n = poll(&p, 1, timeoutMS)) < 0 ) {
            if ( !interruptFlag && ( errno == EAGAIN || errno == EINTR ) ) {
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
    if( errno != ETIMEDOUT ) {
        hasIOError = true;
    }
    return -1;

}

int L2CAPComm::write(const uint8_t * buffer, const int length) {
    int len = 0;
    if( 0 > _dd || 0 > length ) {
        goto errout;
    }
    if( 0 == length ) {
        goto done;
    }

    while( ( len = ::write(_dd, buffer, length) ) < 0 ) {
        if( EAGAIN == errno || EINTR == errno ) {
            continue;
        }
        goto errout;
    }

done:
    return len;

errout:
    hasIOError = true;
    return -1;
}

