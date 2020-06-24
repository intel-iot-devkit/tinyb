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

#ifndef HCI_HANDLER_HPP_
#define HCI_HANDLER_HPP_

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
#include "HCITypes.hpp"
#include "MgmtTypes.hpp"
#include "LFRingbuffer.hpp"

/**
 * - - - - - - - - - - - - - - -
 *
 * Module HCIHandler:
 *
 * - BT Core Spec v5.2: Vol 4, Part E Host Controller Interface (HCI)
 */
namespace direct_bt {

    class HCIHandleBDAddr {
        public:
            uint16_t handle;
            EUI48 address;
            BDAddressType addressType;

        public:
            HCIHandleBDAddr(const uint16_t handle, const EUI48 &address, const BDAddressType addressType)
            : handle(handle), address(address), addressType(addressType) {}

            HCIHandleBDAddr(const HCIHandleBDAddr &o) = default;
            HCIHandleBDAddr(HCIHandleBDAddr &&o) = default;
            HCIHandleBDAddr& operator=(const HCIHandleBDAddr &o) = default;
            HCIHandleBDAddr& operator=(HCIHandleBDAddr &&o) = default;

            bool operator==(const HCIHandleBDAddr& rhs) const
            { return handle == rhs.handle; }

            bool operator!=(const HCIHandleBDAddr& rhs) const
            { return !(*this == rhs); }

            std::string toString() const {
                return "HCIHandleBDAddr[handle "+uint16HexString(handle)+
                       ", address="+address.toString()+", addressType "+getBDAddressTypeString(addressType)+"]";
            }
    };

    /**
     * A thread safe singleton handler of the HCI control channel to one controller (BT adapter)
     * <p>
     * Implementation utilizes a lock free ringbuffer receiving data within its separate thread.
     * </p>
     */
    class HCIHandler {
        public:
            enum Defaults : int {
                HCI_MAX_MTU = static_cast<uint8_t>(HCIConstU8::PACKET_MAX_SIZE),

                /** 10s poll timeout for HCI reader thread */
                HCI_READER_THREAD_POLL_TIMEOUT = 10000,
                /** 3s timeout for HCI command replies. This timeout is rather longer, as it may include waiting for pending command complete. */
                HCI_COMMAND_REPLY_TIMEOUT = 3000,
                /** Small ringbuffer capacity for synchronized commands */
                HCI_EVT_RING_CAPACITY = 64,
                /** Maximum number of packets to wait for until matching a sequential command. Won't block as timeout will limit. */
                HCI_READ_PACKET_MAX_RETRY = HCI_EVT_RING_CAPACITY
            };

            static const pid_t pidSelf;
            static MgmtEvent::Opcode translate(HCIEventType evt, HCIMetaEventType met);
            std::shared_ptr<MgmtEvent> translate(std::shared_ptr<HCIEvent> ev);

        private:
            const BTMode btMode;
            const uint16_t dev_id;
            POctets rbuffer;
            HCIComm comm;
            const int replyTimeoutMS;
            std::recursive_mutex mtx;
            hci_ufilter filter_mask;
            std::atomic<uint32_t> metaev_filter_mask;

            inline bool filter_test_metaev(HCIMetaEventType mec) { return 0 != test_bit_uint32(number(mec)-1, metaev_filter_mask); }
            inline void filter_put_metaevs(const uint32_t mask) { metaev_filter_mask=mask; }

            inline static void filter_clear_metaevs(uint32_t &mask) { mask=0; }
            inline static void filter_all_metaevs(uint32_t &mask) { mask=0xffff; }
            inline static void filter_set_metaev(HCIMetaEventType mec, uint32_t &mask) { set_bit_uint32(number(mec)-1, mask); }

            LFRingbuffer<std::shared_ptr<HCIEvent>, nullptr> hciEventRing;
            std::atomic<pthread_t> hciReaderThreadId;
            std::atomic<bool> hciReaderRunning;
            std::atomic<bool> hciReaderShallStop;
            std::mutex mtx_hciReaderInit;
            std::condition_variable cv_hciReaderInit;
            std::recursive_mutex mtx_sendReply; // for sendWith*Reply, process*Command, ..

            std::vector<HCIHandleBDAddr> disconnectHandleAddrList;
            std::recursive_mutex mtx_disconnectHandleAddrList;

            /** One MgmtAdapterEventCallbackList per event type, allowing multiple callbacks to be invoked for each event */
            std::array<MgmtEventCallbackList, static_cast<uint16_t>(MgmtEvent::Opcode::MGMT_EVENT_TYPE_COUNT)> mgmtEventCallbackLists;
            std::recursive_mutex mtx_callbackLists;
            inline void checkMgmtEventCallbackListsIndex(const MgmtEvent::Opcode opc) const {
                if( static_cast<uint16_t>(opc) >= mgmtEventCallbackLists.size() ) {
                    throw IndexOutOfBoundsException(static_cast<uint16_t>(opc), 1, mgmtEventCallbackLists.size(), E_FILE_LINE);
                }
            }

            void hciReaderThreadImpl();

            bool sendCommand(HCICommand &req);
            std::shared_ptr<HCIEvent> getNextReply(HCICommand &req, int & retryCount);

            std::shared_ptr<HCIEvent> sendWithCmdCompleteReply(HCICommand &req, HCICommandCompleteEvent **res);

            template<typename hci_cmd_event_struct>
            std::shared_ptr<HCIEvent> processSimpleCommand(HCIOpcode opc, const hci_cmd_event_struct **res, HCIStatusCode *status);

            template<typename hci_command_struct>
            std::shared_ptr<HCIEvent> processStructCommand(HCIStructCommand<hci_command_struct> &req, HCIStatusCode *status);

            template<typename hci_cmd_event_struct>
            const hci_cmd_event_struct* getReplyStruct(std::shared_ptr<HCIEvent> event, HCIEventType evc, HCIStatusCode *status);

            template<typename hci_cmd_event_struct>
            const hci_cmd_event_struct* getMetaReplyStruct(std::shared_ptr<HCIEvent> event, HCIMetaEventType mec, HCIStatusCode *status);

            HCIHandler(const HCIHandler&) = delete;
            void operator=(const HCIHandler&) = delete;

            bool mgmtEvDeviceDisconnectedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvDeviceConnectedCB(std::shared_ptr<MgmtEvent> e);
            bool mgmtEvConnectFailedCB(std::shared_ptr<MgmtEvent> e);
            void sendMgmtEvent(std::shared_ptr<MgmtEvent> event);

        public:
            HCIHandler(const BTMode btMode, const uint16_t dev_id, const int replyTimeoutMS=Defaults::HCI_COMMAND_REPLY_TIMEOUT);

            /**
             * Releases this instance after issuing {@link #close()}.
             */
            ~HCIHandler() { close(); }

            void close();

            BTMode getBTMode() { return btMode; }

            /** Returns true if this mgmt instance is open and hence valid, otherwise false */
            bool isOpen() const {
                return comm.isOpen();
            }

            std::string toString() const { return "HCIHandler[BTMode "+BTModeString(btMode)+", dev_id "+std::to_string(dev_id)+"]"; }

            /**
             * BT Core Spec v5.2: Vol 4, Part E HCI: 7.3.2 Reset command
             */
            HCIStatusCode reset();

            /**
             * Establish a connection to the given LE peer.
             * <p>
             * BT Core Spec v5.2: Vol 4, Part E HCI: 7.8.12 LE Create Connection command
             * </p>
             * <p>
             * Even if not utilizing a HCI channel, it has been observed that maintaining such
             * enhanced performance on subsequent communication, i.e. GATT over L2CAP.
             * </p>
             * <p>
             * Set window to the same value as the interval, enables continuous scanning.
             * </p>
             *
             * @param peer_bdaddr
             * @param peer_mac_type
             * @param own_mac_type
             * @param le_scan_interval in units of 0.625ms, default value 48 for 30ms, min value 4 for 2.5ms -> 0x4000 for 10.24s
             * @param le_scan_window in units of 0.625ms, default value 48 for 30ms,  min value 4 for 2.5ms -> 0x4000 for 10.24s. Shall be <= le_scan_interval
             * @param conn_interval_min in units of 1.25ms, default value 15 for 19.75ms
             * @param conn_interval_max in units of 1.25ms, default value 15 for 19.75ms
             * @param conn_latency slave latency in units of connection events, default value 0
             * @param supervision_timeout in units of 10ms, default value 1000 for 10000ms or 10s.
             * @return
             */
            HCIStatusCode le_create_conn(const EUI48 &peer_bdaddr,
                                        const HCIAddressType peer_mac_type=HCIAddressType::HCIADDR_LE_PUBLIC,
                                        const HCIAddressType own_mac_type=HCIAddressType::HCIADDR_LE_PUBLIC,
                                        const uint16_t le_scan_interval=48, const uint16_t le_scan_window=48,
                                        const uint16_t conn_interval_min=0x000F, const uint16_t conn_interval_max=0x000F,
                                        const uint16_t conn_latency=0x0000, const uint16_t supervision_timeout=number(HCIConstInt::LE_CONN_TIMEOUT_MS)/10);

            /**
             * Establish a connection to the given BREDR (non LE).
             * <p>
             * BT Core Spec v5.2: Vol 4, Part E HCI: 7.1.5 Create Connection command
             * </p>
             */
            HCIStatusCode create_conn(const EUI48 &bdaddr,
                                     const uint16_t pkt_type=HCI_DM1 | HCI_DM3 | HCI_DM5 | HCI_DH1 | HCI_DH3 | HCI_DH5,
                                     const uint16_t clock_offset=0x0000, const uint8_t role_switch=0x01);

            /**
             * Disconnect an established connection.
             * <p>
             * BT Core Spec v5.2: Vol 4, Part E HCI: 7.1.6 Disconnect command
             * </p>
             */
            HCIStatusCode disconnect(const uint16_t conn_handle, const EUI48 &peer_bdaddr, const BDAddressType peer_mac_type,
                                     const HCIStatusCode reason=HCIStatusCode::REMOTE_USER_TERMINATED_CONNECTION);

            /** MgmtEventCallback handling  */

            /**
             * Appends the given MgmtEventCallback to the named MgmtEvent::Opcode list,
             * if it is not present already (opcode + callback).
             */
            void addMgmtEventCallback(const MgmtEvent::Opcode opc, const MgmtEventCallback &cb);
            /** Returns count of removed given MgmtEventCallback from the named MgmtEvent::Opcode list. */
            int removeMgmtEventCallback(const MgmtEvent::Opcode opc, const MgmtEventCallback &cb);
            /** Removes all MgmtEventCallbacks from the to the named MgmtEvent::Opcode list. */
            void clearMgmtEventCallbacks(const MgmtEvent::Opcode opc);
            /** Removes all MgmtEventCallbacks from all MgmtEvent::Opcode lists. */
            void clearAllMgmtEventCallbacks();
    };

} // namespace direct_bt

#endif /* DBT_HANDLER_HPP_ */

