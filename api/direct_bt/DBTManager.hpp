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
#include <array>

#include <mutex>
#include <atomic>
#include <thread>

#include "BTTypes.hpp"
#include "BTIoctl.hpp"
#include "OctetTypes.hpp"
#include "HCIComm.hpp"
#include "JavaUplink.hpp"
#include "MgmtTypes.hpp"
#include "LFRingbuffer.hpp"
#include "ClassFunction.hpp"

namespace direct_bt {

    typedef ClassFunction<bool, std::shared_ptr<MgmtEvent>> MgmtEventCallback;
    typedef std::vector<MgmtEventCallback> MgmtEventCallbackList;

    /**
     * A thread safe singleton handler of the Linux Kernel's BlueZ manager control channel.
     * <p>
     * Implementation utilizes a lock free ringbuffer receiving data within its separate thread.
     * </p>
     */
    class DBTManager : public JavaUplink {
        public:
            enum Defaults : int {
                /* BT Core Spec v5.2: Vol 3, Part F 3.2.8: Maximum length of an attribute value. */
                ClientMaxMTU = 512,

                /** 10s poll timeout for mgmt reader thread */
                MGMT_READER_THREAD_POLL_TIMEOUT = 10000,
                MGMTEVT_RING_CAPACITY = 256
            };

            static const pid_t pidSelf;

        private:
            const BTMode btMode;
            POctets rbuffer;
            HCIComm comm;

            LFRingbuffer<std::shared_ptr<MgmtEvent>, nullptr> mgmtEventRing;
            std::thread mgmtReaderThread;
            volatile bool mgmtReaderRunning;
            volatile bool mgmtReaderShallStop;

            /** One MgmtEventCallbackList per event type, allowing multiple callbacks to be invoked for each event */
            std::array<MgmtEventCallbackList, MgmtEvent::Opcode::MGMT_EVENT_TYPE_COUNT> mgmtEventCallbackLists;
            std::recursive_mutex mtx_callbackLists;
            inline void checkMgmtEventCallbackListsIndex(const MgmtEvent::Opcode opc) const {
                if( opc >= mgmtEventCallbackLists.size() ) {
                    throw IndexOutOfBoundsException(opc, 1, mgmtEventCallbackLists.size(), E_FILE_LINE);
                }
            }

            std::vector<std::shared_ptr<AdapterInfo>> adapterInfos;
            void mgmtReaderThreadImpl();
            std::shared_ptr<MgmtEvent> receiveNext();

            DBTManager(const BTMode btMode);
            DBTManager(const DBTManager&) = delete;
            void operator=(const DBTManager&) = delete;
            void close();

            std::shared_ptr<AdapterInfo> initAdapter(const uint16_t dev_id, const BTMode btMode);
            void shutdownAdapter(const uint16_t dev_id);

            bool mgmtEvClassOfDeviceChangedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvLocalNameChangedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceDiscoveringCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvNewSettingsCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceFoundCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceDisconnectedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceConnectedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvConnectFailedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDevicePinCodeRequestCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceUnpairedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvNewConnectionParamCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceWhitelistAddedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceWhilelistRemovedCB(std::shared_ptr<MgmtEvent> e);
            void sendMgmtEvent(std::shared_ptr<MgmtEvent> event);

        public:
            /**
             * Retrieves the singleton instance.
             * <p>
             * First call will open and initialize the bluetooth kernel.
             * </p>
             */
            static DBTManager& get(const BTMode btMode) {
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
                static DBTManager s(btMode);
                return s;
            }
            ~DBTManager() { close(); }

            std::string get_java_class() const override {
                return java_class();
            }
            static std::string java_class() {
                return std::string(JAVA_DBT_PACKAGE "DBTManager");
            }

            BTMode getBTMode() { return btMode; }

            /** Returns true if this mgmt instance is open and hence valid, otherwise false */
            bool isOpen() const {
                return comm.isOpen();
            }

            std::string toString() const override { return "MgmtHandler["+std::to_string(adapterInfos.size())+" adapter, "+javaObjectToString()+"]"; }

            /** retrieve information gathered at startup */

            const std::vector<std::shared_ptr<AdapterInfo>> getAdapterInfos() const { return adapterInfos; }

            /**
             * Returns the AdapterInfo index (== dev_id) with the given address or -1 if not found.
             */
            int findAdapterInfoIdx(const EUI48 &mac) const;
            /**
             * Returns the AdapterInfo (index == dev_id) with the given address or nullptr if not found.
             */
            std::shared_ptr<AdapterInfo> findAdapterInfo(const EUI48 &mac) const;
            /**
             * Returns the AdapterInfo (index == dev_id) with the given index.
             * <p>
             * Throws IndexOutOfBoundsException if index is > adapter count.
             * </p>
             */
            std::shared_ptr<AdapterInfo> getAdapterInfo(const int idx) const;
            /**
             * Returns the default AdapterInfo (0 == index == dev_id) or nullptr if no adapter is available.
             */
            std::shared_ptr<AdapterInfo> getDefaultAdapterInfo() const { return adapterInfos.size() > 0 ? getAdapterInfo(0) : nullptr; }

            /**
             * In case response size check or devID and optional opcode validation fails,
             * function returns NULL.
             */
            std::shared_ptr<MgmtEvent> send(MgmtCommand &req);

            bool setMode(const int dev_id, const MgmtOpcode opc, const uint8_t mode);

            /** Start discovery on given adapter dev_id with a ScanType matching the used BTMode. Returns set ScanType. */
            ScanType startDiscovery(const int dev_id);
            /** Start discovery on given adapter dev_id with given ScanType. Returns set ScanType. */
            ScanType startDiscovery(const int dev_id, const ScanType type);
            /** Stop discovery on given adapter dev_id. */
            bool stopDiscovery(const int dev_id, const ScanType type);

            uint16_t create_connection(const int dev_id,
                                       const EUI48 &peer_bdaddr,
                                       const BDAddressType peer_mac_type,
                                       const BDAddressType own_mac_type=BDADDR_LE_PUBLIC,
                                       const uint16_t interval=0x0004, const uint16_t window=0x0004,
                                       const uint16_t min_interval=0x000F, const uint16_t max_interval=0x000F,
                                       const uint16_t latency=0x0000, const uint16_t supervision_timeout=0x0C80,
                                       const uint16_t min_ce_length=0x0001, const uint16_t max_ce_length=0x0001,
                                       const uint8_t initiator_filter=0);

            bool disconnect(const int dev_id, const EUI48 &peer_bdaddr, const BDAddressType peer_mac_type, const uint8_t reason=0);

            /** MgmtEventCallback handling  */

            /** Appends the given MgmtEventCallback to the named MgmtEvent::Opcode list. */
            void addMgmtEventCallback(const MgmtEvent::Opcode opc, const MgmtEventCallback &cb);
            /** Returns count of removed given MgmtEventCallback from the named MgmtEvent::Opcode list. */
            int removeMgmtEventCallback(const MgmtEvent::Opcode opc, const MgmtEventCallback &cb);
            /** Removes all MgmtEventCallbacks from the to the named MgmtEvent::Opcode list. */
            void clearMgmtEventCallbacks(const MgmtEvent::Opcode opc);
            /** Removes all MgmtEventCallbacks from all MgmtEvent::Opcode lists. */
            void clearAllMgmtEventCallbacks();
    };

} // namespace direct_bt

#endif /* DBT_MANAGER_HPP_ */

