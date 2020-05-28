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

int L2CAPComm::l2cap_open_dev(const EUI48 & adapterAddress, const uint16_t psm, const uint16_t cid, const bool pubaddrAdapter, const bool blocking) {
    sockaddr_l2 a;
    int dd, err;

    // Create a loose L2CAP socket
    dd = ::socket(AF_BLUETOOTH, // AF_BLUETOOTH == PF_BLUETOOTH
                  blocking ? SOCK_SEQPACKET : SOCK_SEQPACKET | SOCK_NONBLOCK,
                  BTPROTO_L2CAP);

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

#define STATE_ENUM(X) \
    X(Error) \
    X(Disconnected) \
    X(Connecting) \
    X(Connected)

#define CASE_TO_STRING(V) case V: return #V;

std::string L2CAPComm::getStateString(const State state) {
    switch(state) {
        STATE_ENUM(CASE_TO_STRING)
        default: ; // fall through intended
    }
    return "Unknown State";
}

L2CAPComm::State L2CAPComm::connect() {
    /** BT Core Spec v5.2: Vol 3, Part A: L2CAP_CONNECTION_REQ */

    DBG_PRINT("L2CAPComm::connect: Start dd %d, %s, psm %u, cid %u, pubDevice %d",
            _dd.load(), device->getAddress().toString().c_str(), psm, cid, pubaddr);

    if( 0 <= _dd ) {
        return state; // already open
    }

    sockaddr_l2 req;
    int err, res;

    _dd = l2cap_open_dev(device->getAdapter().getAddress(), psm, cid, true /* pubaddrAdapter */, blocking);
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

    // may block if O_NONBLOCK has not been specified in open_dev(..)
    res = ::connect(_dd, (struct sockaddr*)&req, sizeof(req));

    tid_connect = 0;

    DBG_PRINT("L2CAPComm::connect: Result %d, errno 0%X %s, %s",
            res, errno, strerror(errno), device->getAddress().toString().c_str());

    if( !res )
    {
        state = Connected;

    } else if( EINPROGRESS == errno ) {
        // non-blocking connection in progress (O_NONBLOCK), check via select / poll later
        state = Connecting;

    } else  {
        // EALREADY == errno || ENETUNREACH == errno || EHOSTUNREACH == errno || ..
        ERR_PRINT("L2CAPComm::connect: connect failed");
        goto failure;
    }

    return state;

failure:
    err = errno;
    disconnect();
    errno = err;
    state = Error;
    return state;
}

bool L2CAPComm::disconnect() {
    if( 0 > _dd ) {
        DBG_PRINT("L2CAPComm::disconnect: Not connected");
        state = Disconnected;
        return false;
    }
    DBG_PRINT("L2CAPComm::disconnect: Start dd %d", _dd.load());
    interruptReadFlag = true;

    // interrupt L2CAP ::connect(..), avoiding prolonged hang
    pthread_t _tid_connect = tid_connect;
    tid_connect = 0;
    if( 0 != _tid_connect ) {
        pthread_t tid_self = pthread_self();
        if( tid_self != _tid_connect ) {
            pthread_kill(_tid_connect, SIGALRM);
        }
    }

    l2cap_close_dev(_dd);
    _dd = -1;
    state = Disconnected;
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
    interruptReadFlag = false;

    if( timeoutMS ) {
        struct pollfd p;
        int n;

        p.fd = _dd; p.events = POLLIN;
        while ( !interruptReadFlag && (n = poll(&p, 1, timeoutMS)) < 0 ) {
            if ( !interruptReadFlag && ( errno == EAGAIN || errno == EINTR ) ) {
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
    state = Error;
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
    state = Error;
    return -1;
}

