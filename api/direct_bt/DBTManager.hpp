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

#ifndef DBT_MANAGER_HPP_
#define DBT_MANAGER_HPP_

#include <cstring>
#include <string>
#include <cstdint>
#include <mutex>

#include "BTTypes.hpp"
#include "BTIoctl.hpp"
#include "OctetTypes.hpp"
#include "HCIComm.hpp"
#include "JavaUplink.hpp"
#include "MgmtTypes.hpp"

namespace direct_bt {

    /**
     * A thread safe singleton handler of the Linux Kernel's BlueZ manager control channel.
     */
    class DBTManager : public JavaUplink {
        private:
            std::recursive_mutex mtx;
            const int ibuffer_size = 512;
            uint8_t ibuffer[512];
            std::vector<std::shared_ptr<const AdapterInfo>> adapters;
            HCIComm comm;

            int read(uint8_t* buffer, const int capacity, const int timeoutMS);
            int write(const uint8_t * buffer, const int length);

            DBTManager();
            DBTManager(const DBTManager&) = delete;
            void operator=(const DBTManager&) = delete;
            void close();

            bool initAdapter(const uint16_t dev_id);

        public:
            /**
             * Retrieves the singleton instance.
             * <p>
             * First call will open and initialize the bluetooth kernel.
             * </p>
             */
            static DBTManager& get() {
                /**
                 * Thread safe starting with C++11 6.7:
                 *
                 * If control enters the declaration concurrently while the variable is being initialized,
                 * the concurrent execution shall wait for completion of the initialization.
                 *
                 * (Magic Statics)
                 *
                 * Avoiding non-working double checked locking.
                 */
                static DBTManager s;
                return s;
            }
            ~DBTManager() { close(); }

            std::string get_java_class() const override {
                return java_class();
            }
            static std::string java_class() {
                return std::string(JAVA_DBT_PACKAGE "DBTManager");
            }

            /** Returns true if this mgmt instance is open and hence valid, otherwise false */
            bool isOpen() const {
                return comm.isOpen();
            }
            bool setMode(const int dev_id, const MgmtModeReq::Opcode opc, const uint8_t mode);

            /**
             * In case response size check or devID and optional opcode validation fails,
             * function returns NULL.
             */
            std::shared_ptr<MgmtEvent> send(MgmtRequest &req, uint8_t* buffer, const int capacity, const int timeoutMS);

            const std::vector<std::shared_ptr<const AdapterInfo>> getAdapters() const { return adapters; }
            int getDefaultAdapterIdx() const { return adapters.size() > 0 ? 0 : -1; }
            int findAdapterIdx(const EUI48 &mac) const;
            std::shared_ptr<const AdapterInfo> getAdapter(const int idx) const;

            std::string toString() const override { return "MgmtHandler["+std::to_string(adapters.size())+" adapter, "+javaObjectToString()+"]"; }
    };

} // namespace direct_bt

#endif /* DBT_MANAGER_HPP_ */

