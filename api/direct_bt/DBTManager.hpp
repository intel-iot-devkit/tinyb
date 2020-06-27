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

namespace direct_bt {

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
                /** 3s timeout for mgmt command replies */
                MGMT_COMMAND_REPLY_TIMEOUT = 3000,
                /** Small ringbuffer capacity for synchronized commands */
                MGMTEVT_RING_CAPACITY = 64
            };

            static const pid_t pidSelf;

        private:
            static std::mutex mtx_singleton;

            struct WhitelistElem {
                int dev_id;
                EUI48 address;
                BDAddressType address_type;
                HCIWhitelistConnectType ctype;
            };
            std::vector<std::shared_ptr<WhitelistElem>> whitelist;

            const BTMode btMode;
            POctets rbuffer;
            HCIComm comm;

            LFRingbuffer<std::shared_ptr<MgmtEvent>, nullptr> mgmtEventRing;
            std::thread mgmtReaderThread;
            std::atomic<bool> mgmtReaderRunning;
            std::atomic<bool> mgmtReaderShallStop;
            std::mutex mtx_mgmtReaderInit;
            std::condition_variable cv_mgmtReaderInit;
            std::recursive_mutex mtx_sendReply; // for sendWithReply

            /** One MgmtAdapterEventCallbackList per event type, allowing multiple callbacks to be invoked for each event */
            std::array<MgmtAdapterEventCallbackList, static_cast<uint16_t>(MgmtEvent::Opcode::MGMT_EVENT_TYPE_COUNT)> mgmtAdapterEventCallbackLists;
            std::recursive_mutex mtx_callbackLists;
            inline void checkMgmtEventCallbackListsIndex(const MgmtEvent::Opcode opc) const {
                if( static_cast<uint16_t>(opc) >= mgmtAdapterEventCallbackLists.size() ) {
                    throw IndexOutOfBoundsException(static_cast<uint16_t>(opc), 1, mgmtAdapterEventCallbackLists.size(), E_FILE_LINE);
                }
            }

            std::vector<std::shared_ptr<AdapterInfo>> adapterInfos;
            void mgmtReaderThreadImpl();

            /**
             * In case response size check or devID and optional opcode validation fails,
             * function returns NULL.
             */
            std::shared_ptr<MgmtEvent> sendWithReply(MgmtCommand &req);

            DBTManager(const BTMode btMode);
            DBTManager(const DBTManager&) = delete;
            void operator=(const DBTManager&) = delete;

            std::shared_ptr<AdapterInfo> initAdapter(const uint16_t dev_id, const BTMode btMode);
            void shutdownAdapter(const uint16_t dev_id);

            bool mgmtEvClassOfDeviceChangedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceDiscoveringCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceFoundCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceDisconnectedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceConnectedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvConnectFailedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceBlockedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceUnblockedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceUnpairedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvNewConnectionParamCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceWhitelistAddedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceWhilelistRemovedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvPinCodeRequestCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvUserPasskeyRequestCB(std::shared_ptr<MgmtEvent> e);
            void sendMgmtEvent(std::shared_ptr<MgmtEvent> event);

        public:
            /**
             * Retrieves the singleton instance.
             * <p>
             * First call will open and initialize the bluetooth kernel.
             * </p>
             */
            static DBTManager& get(const BTMode btMode) {
                const std::lock_guard<std::mutex> lock(mtx_singleton); // ensure thread safety
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

            void close();

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

            std::string toString() const override {
                return "MgmtHandler["+BTModeString(btMode)+", "+std::to_string(adapterInfos.size())+" adapter, "+javaObjectToString()+"]";
            }

            /** retrieve information gathered at startup */

            /**
             * Returns list of AdapterInfo with index == dev_id.
             */
            const std::vector<std::shared_ptr<AdapterInfo>> getAdapterInfos() const { return adapterInfos; }

            /**
             * Returns number of AdapterInfo with index == dev_id.
             */
            int getAdapterCount() const { return adapterInfos.size(); }

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

            bool setMode(const int dev_id, const MgmtOpcode opc, const uint8_t mode);

            /** Start discovery on given adapter dev_id with a ScanType matching the used BTMode. Returns set ScanType. */
            ScanType startDiscovery(const int dev_id);
            /** Start discovery on given adapter dev_id with given ScanType. Returns set ScanType. */
            ScanType startDiscovery(const int dev_id, const ScanType type);
            /** Stop discovery on given adapter dev_id. */
            bool stopDiscovery(const int dev_id, const ScanType type);

            /**
             * Uploads given connection parameter for given device to the kernel.
             *
             * @param dev_id
             * @param address
             * @param address_type
             * @param conn_interval_min default value 0x000F
             * @param conn_interval_max default value 0x000F
             * @param conn_latency default value 0x0000
             * @param timeout in units of 10ms, default value 1000 for 10000ms or 10s.
             * @return
             */
            bool uploadConnParam(const int dev_id, const EUI48 &address, const BDAddressType address_type,
                                 const uint16_t conn_interval_min=0x000F, const uint16_t conn_interval_max=0x000F,
                                 const uint16_t conn_latency=0x0000, const uint16_t timeout=number(HCIConstInt::LE_CONN_TIMEOUT_MS)/10);

            /**
             * Returns true, if the adapter's device is already whitelisted.
             */
            bool isDeviceWhitelisted(const int dev_id, const EUI48 &address);

            /**
             * Add the given device to the adapter's autoconnect whitelist.
             * <p>
             * Make sure {@link uploadConnParam(..)} is invoked first, otherwise performance will lack.
             * </p>
             * <p>
             * Method will reject duplicate devices, in which case it should be removed first.
             * </p>
             */
            bool addDeviceToWhitelist(const int dev_id, const EUI48 &address, const BDAddressType address_type, const HCIWhitelistConnectType ctype);

            /** Remove the given device from the adapter's autoconnect whitelist. */
            bool removeDeviceFromWhitelist(const int dev_id, const EUI48 &address, const BDAddressType address_type);

            /** Remove all previously added devices from the autoconnect whitelist. Returns number of removed devices. */
            int removeAllDevicesFromWhitelist();

            bool disconnect(const bool ioErrorCause,
                            const int dev_id, const EUI48 &peer_bdaddr, const BDAddressType peer_mac_type,
                            const HCIStatusCode reason=HCIStatusCode::REMOTE_USER_TERMINATED_CONNECTION );

            std::shared_ptr<ConnectionInfo> getConnectionInfo(const int dev_id, const EUI48 &address, const BDAddressType address_type);
            std::shared_ptr<NameAndShortName> setLocalName(const int dev_id, const std::string & name, const std::string & short_name);

            /** MgmtEventCallback handling  */

            /**
             * Appends the given MgmtEventCallback for the given adapter dev_id to the named MgmtEvent::Opcode list,
             * if it is not present already (dev_id + opcode + callback).
             * <p>
             * The adapter dev_id allows filtering the events only directed to the given adapter.
             * Use dev_id <code>-1</code> to receive the event for all adapter.
             * </p>
             */
            void addMgmtEventCallback(const int dev_id, const MgmtEvent::Opcode opc, const MgmtEventCallback &cb);
            /** Returns count of removed given MgmtEventCallback from the named MgmtEvent::Opcode list. */
            int removeMgmtEventCallback(const MgmtEvent::Opcode opc, const MgmtEventCallback &cb);
            /** Returns count of removed MgmtEventCallback from the named MgmtEvent::Opcode list matching the given adapter dev_id . */
            int removeMgmtEventCallback(const int dev_id);
            /** Removes all MgmtEventCallbacks from the to the named MgmtEvent::Opcode list. */
            void clearMgmtEventCallbacks(const MgmtEvent::Opcode opc);
            /** Removes all MgmtEventCallbacks from all MgmtEvent::Opcode lists. */
            void clearAllMgmtEventCallbacks();
    };

} // namespace direct_bt

#endif /* DBT_MANAGER_HPP_ */

