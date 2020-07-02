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

#ifndef L2CAP_COMM_HPP_
#define L2CAP_COMM_HPP_

#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>

#include <mutex>
#include <atomic>

#include "UUID.hpp"
#include "BTTypes.hpp"

/**
 * - - - - - - - - - - - - - - -
 *
 * Module L2CAPComm:
 *
 * - BT Core Spec v5.2: Vol 3, Part A: BT Logical Link Control and Adaption Protocol (L2CAP)
 */
namespace direct_bt {

    class DBTDevice; // forward

    class L2CAPComm {
        public:
            enum class Defaults : int {
                L2CAP_CONNECT_MAX_RETRY = 3
            };
            static inline int number(const Defaults d) { return static_cast<int>(d); }

            static std::string getStateString(bool isConnected, bool hasIOError) {
                return "State[connected "+std::to_string(isConnected)+", ioError "+std::to_string(hasIOError)+"]";
            }

        private:
            static int l2cap_open_dev(const EUI48 & adapterAddress, const uint16_t psm, const uint16_t cid, const bool pubaddr);
            static int l2cap_close_dev(int dd);

            std::shared_ptr<DBTDevice> device;
            const std::string deviceString;
            const uint16_t psm;
            const uint16_t cid;
            std::atomic<int> _dd; // the l2cap socket
            std::atomic<bool> isConnected; // reflects state
            std::atomic<bool> hasIOError;  // reflects state
            std::atomic<bool> interruptFlag; // for forced disconnect
            std::atomic<pthread_t> tid_connect;

        public:
            L2CAPComm(std::shared_ptr<DBTDevice> device, const uint16_t psm, const uint16_t cid);

            std::shared_ptr<DBTDevice> getDevice() { return device; }

            bool getIsConnected() const { return isConnected; }
            bool getHasIOError() const { return hasIOError; }
            std::string getStateString() const { return getStateString(isConnected, hasIOError); }

            /** BT Core Spec v5.2: Vol 3, Part A: L2CAP_CONNECTION_REQ */
            bool connect();

            bool disconnect();

            bool isOpen() const { return 0 <= _dd; }
            int dd() const { return _dd; }

            int read(uint8_t* buffer, const int capacity, const int timeoutMS);
            int write(const uint8_t *buffer, const int length);
    };

} // namespace direct_bt

#endif /* L2CAP_COMM_HPP_ */
