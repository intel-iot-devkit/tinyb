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

#ifndef HCI_COMM_HPP_
#define HCI_COMM_HPP_

#include <cstring>
#include <string>
#include <cstdint>
#include <memory>
#include <mutex>

#include "BTTypes.hpp"
#include "BTIoctl.hpp"
#include "HCIIoctl.hpp"

/**
 * - - - - - - - - - - - - - - -
 *
 * Module HCIComm:
 *
 * - BT Core Spec v5.2: Vol 4, Part E Host Controller Interface (HCI)
 */
namespace direct_bt {

    enum HCIDefaults : int {
        /** 3s poll timeout for HCI readout */
        HCI_TO_SEND_REQ_POLL_MS = 3000,
        /** 10s le connection timeout, supervising max is 32s (v5.2 Vol 4, Part E - 7.8.12) */
        HCI_LE_CONN_TIMEOUT_MS = 10000
    };

    enum HCI_Event_Types : uint8_t {
        LE_Advertising_Report   = 0x3E
    };

    /**
     * BT Core Spec v5.2: Vol 1, Part F Controller Error Codes: 1.3 List of Error Codes
     * <p>
     * BT Core Spec v5.2: Vol 1, Part F Controller Error Codes: 2 Error code descriptions
     * </p>
     */
    enum class HCIErrorCode : uint8_t {
        SUCCESS = 0x00,
        UNKNOWN_HCI_COMMAND = 0x01,
        UNKNOWN_CONNECTION_IDENTIFIER = 0x02,
        HARDWARE_FAILURE = 0x03, 
        PAGE_TIMEOUT = 0x04, 
        AUTHENTICATION_FAILURE = 0x05, 
        PIN_OR_KEY_MISSING = 0x06, 
        MEMORY_CAPACITY_EXCEEDED = 0x07,
        CONNECTION_TIMEOUT = 0x08, 
        CONNECTION_LIMIT_EXCEEDED = 0x09,
        SYNC_DEVICE_CONNECTION_LIMIT_EXCEEDED = 0x0a,
        CONNECTION_ALREADY_EXISTS = 0x0b,
        COMMAND_DISALLOWED = 0x0c, 
        CONNECTION_REJECTED_LIMITED_RESOURCES = 0x0d,
        CONNECTION_REJECTED_SECURITY = 0x0e,
        CONNECTION_REJECTED_UNACCEPTABLE_BD_ADDR = 0x0f,
        CONNECTION_ACCEPT_TIMEOUT_EXCEEDED = 0x10,
        UNSUPPORTED_FEATURE_OR_PARAM_VALUE = 0x11,
        INVALID_HCI_COMMAND_PARAMETERS = 0x12,
        REMOTE_USER_TERMINATED_CONNECTION = 0x13,
        REMOTE_DEVICE_TERMINATED_CONNECTION_LOW_RESOURCES = 0x14,
        REMOTE_DEVICE_TERMINATED_CONNECTION_POWER_OFF = 0x15,
        CONNECTION_TERMINATED_BY_LOCAL_HOST = 0x16,
        REPEATED_ATTEMPTS = 0x17, 
        PAIRING_NOT_ALLOWED = 0x18, 
        UNKNOWN_LMP_PDU = 0x19, 
        UNSUPPORTED_REMOTE_OR_LMP_FEATURE = 0x1a,
        SCO_OFFSET_REJECTED = 0x1b, 
        SCO_INTERVAL_REJECTED = 0x1c, 
        SCO_AIR_MODE_REJECTED = 0x1d,
        INVALID_LMP_OR_LL_PARAMETERS = 0x1e,
        UNSPECIFIED_ERROR = 0x1f, 
        UNSUPPORTED_LMP_OR_LL_PARAMETER_VALUE = 0x20,
        ROLE_CHANGE_NOT_ALLOWED = 0x21, 
        LMP_OR_LL_RESPONSE_TIMEOUT = 0x22,
        LMP_OR_LL_COLLISION = 0x23,
        LMP_PDU_NOT_ALLOWED = 0x24, 
        ENCRYPTION_MODE_NOT_ACCEPTED = 0x25, 
        LINK_KEY_CANNOT_BE_CHANGED = 0x26,
        REQUESTED_QOS_NOT_SUPPORTED = 0x27,
        INSTANT_PASSED = 0x28, 
        PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED = 0x29,
        DIFFERENT_TRANSACTION_COLLISION = 0x2a,
        QOS_UNACCEPTABLE_PARAMETER = 0x2c, 
        QOS_REJECTED = 0x2d, 
        CHANNEL_ASSESSMENT_NOT_SUPPORTED = 0x2e,
        INSUFFICIENT_SECURITY = 0x2f, 
        PARAMETER_OUT_OF_RANGE = 0x30, 
        ROLE_SWITCH_PENDING = 0x32, 
        RESERVED_SLOT_VIOLATION = 0x34,
        ROLE_SWITCH_FAILED = 0x35, 
        EIR_TOO_LARGE = 0x36, 
        SIMPLE_PAIRING_NOT_SUPPORTED_BY_HOST = 0x37,
        HOST_BUSY_PAIRING = 0x38, 
        CONNECTION_REJECTED_NO_SUITABLE_CHANNEL = 0x39,
        CONTROLLER_BUSY = 0x3a,
        UNACCEPTABLE_CONNECTION_PARAM = 0x3b,
        ADVERTISING_TIMEOUT = 0x3c,
        CONNECTION_TERMINATED_MIC_FAILURE = 0x3d,
        CONNECTION_EST_FAILED_OR_SYNC_TIMETOUT = 0x3e,
        MAX_CONNECTION_FAILED  = 0x3f,
        COARSE_CLOCK_ADJ_REJECTED = 0x40,
        TYPE0_SUBMAP_NOT_DEFINED = 0x41,
        UNKNOWN_ADVERTISING_IDENTIFIER = 0x42,
        LIMIT_REACHED = 0x43,
        OPERATION_CANCELLED_BY_HOST = 0x44,
        PACKET_TOO_LONG = 0x45,

        INTERNAL_FAILURE = 0xfe,
        UNKNOWN = 0xff
    };
    std::string getHCIErrorCodeString(const HCIErrorCode ec);

    class HCIComm {
        private:
            static int hci_open_dev(const uint16_t dev_id, const uint16_t channel);
            static int hci_close_dev(int dd);

            std::recursive_mutex mtx;
            const int timeoutMS;
            const uint16_t dev_id;
            const uint16_t channel;
            int _dd; // the hci socket
            bool le_scanning;

            bool send_cmd(const uint16_t opcode, const void *command, const uint8_t command_len);
            HCIErrorCode send_req(const uint16_t opcode, const void *command, const uint8_t command_len,
                                  const uint16_t exp_event, void *response, const uint8_t response_len);

            bool le_set_scan_enable(const uint8_t enable, const uint8_t filter_dup);
            bool le_set_scan_parameters(const uint8_t type, const uint16_t interval,
                                        const uint16_t window, const uint8_t own_type,
                                        const uint8_t filter_policy);

        public:
            HCIComm(const uint16_t dev_id, const uint16_t channel, const int timeoutMS=HCIDefaults::HCI_TO_SEND_REQ_POLL_MS)
            : timeoutMS(timeoutMS), dev_id(dev_id), channel(channel), _dd(-1), le_scanning(false) {
                _dd = hci_open_dev(dev_id, channel);
            }

            /**
             * Releases this instance after {@link #le_disable_scan()} and {@link #close()}.
             * <p>
             * Since no connection handles are being stored, {@link #le_disconnect(..)} can't be issued.
             * </p>
             */
            ~HCIComm() { le_disable_scan(); close(); }

            void close();

            bool isOpen() const { return 0 <= _dd; }

            /** Return this HCI device descriptor, for multithreading access use {@link #dd()}. */
            int dd() const { return _dd; }

            /** Return the recursive mutex for multithreading access of {@link #mutex()}. */
            std::recursive_mutex & mutex() { return mtx; }

            /** Generic read w/ own timeoutMS. Not protected by mutex. */
            int read(uint8_t* buffer, const int capacity, const int timeoutMS);
            /** Generic read, reusing set timeoutMS from ctor. Not protected by mutex */
            int read(uint8_t* buffer, const int capacity);
            /** Generic write */
            int write(const uint8_t* buffer, const int size);

            /**
             * Enable scanning for LE devices, i.e. starting discovery.
             * <p>
             * It is recommended to utilize the DBTManager manager channel for device discovery!
             * </p>
             */
            bool le_enable_scan(const HCIAddressType own_type=HCIAddressType::HCIADDR_LE_PUBLIC,
                                const uint16_t interval=0x0004, const uint16_t window=0x0004);
            /**
             * Disable scanning for LE devices.
             * <p>
             * It is recommended to utilize the DBTManager manager channel to handle scanning!
             * </p>
             */
            void le_disable_scan();

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
             */
            HCIErrorCode le_create_conn(uint16_t * handle_return, const EUI48 &peer_bdaddr,
                                        const HCIAddressType peer_mac_type=HCIAddressType::HCIADDR_LE_PUBLIC,
                                        const HCIAddressType own_mac_type=HCIAddressType::HCIADDR_LE_PUBLIC,
                                        const uint16_t interval=0x0004, const uint16_t window=0x0004,
                                        const uint16_t min_interval=0x000F, const uint16_t max_interval=0x000F,
                                        const uint16_t latency=0x0000, const uint16_t supervision_timeout=HCI_LE_CONN_TIMEOUT_MS/10);

            /**
             * Establish a connection to the given BREDR (non LE).
             */
            HCIErrorCode create_conn(uint16_t * handle_return, const EUI48 &bdaddr,
                                     const uint16_t pkt_type=HCI_DM1 | HCI_DM3 | HCI_DM5 | HCI_DH1 | HCI_DH3 | HCI_DH5,
                                     const uint16_t clock_offset=0x0000, const uint8_t role_switch=0x01);

            /**
             * Disconnect an established connection.
             */
            bool disconnect(const uint16_t conn_handle, const HCIErrorCode reason=HCIErrorCode::REMOTE_USER_TERMINATED_CONNECTION);

        private:
            static inline void set_bit(int nr, void *addr)
            {
                *((uint32_t *) addr + (nr >> 5)) |= (1 << (nr & 31));
            }

            static inline void clear_bit(int nr, void *addr)
            {
                *((uint32_t *) addr + (nr >> 5)) &= ~(1 << (nr & 31));
            }

            static inline int test_bit(int nr, void *addr)
            {
                return *((uint32_t *) addr + (nr >> 5)) & (1 << (nr & 31));
            }

        public:
            static inline void filter_clear(hci_ufilter *f)
            {
                memset(f, 0, sizeof(*f));
            }
            static inline void filter_set_ptype(int t, hci_ufilter *f)
            {
                set_bit((t == HCI_VENDOR_PKT) ? 0 : (t & HCI_FLT_TYPE_BITS), &f->type_mask);
            }
            static inline void filter_clear_ptype(int t, hci_ufilter *f)
            {
                clear_bit((t == HCI_VENDOR_PKT) ? 0 : (t & HCI_FLT_TYPE_BITS), &f->type_mask);
            }
            static inline int filter_test_ptype(int t, hci_ufilter *f)
            {
                return test_bit((t == HCI_VENDOR_PKT) ? 0 : (t & HCI_FLT_TYPE_BITS), &f->type_mask);
            }
            static inline void filter_all_ptypes(hci_ufilter *f)
            {
                memset((void *) &f->type_mask, 0xff, sizeof(f->type_mask));
            }
            static inline void filter_set_event(int e, hci_ufilter *f)
            {
                set_bit((e & HCI_FLT_EVENT_BITS), &f->event_mask);
            }
            static inline void filter_clear_event(int e, hci_ufilter *f)
            {
                clear_bit((e & HCI_FLT_EVENT_BITS), &f->event_mask);
            }
            static inline int filter_test_event(int e, hci_ufilter *f)
            {
                return test_bit((e & HCI_FLT_EVENT_BITS), &f->event_mask);
            }
            static inline void filter_all_events(hci_ufilter *f)
            {
                memset((void *) f->event_mask, 0xff, sizeof(f->event_mask));
            }
            static inline void filter_set_opcode(int opcode, hci_ufilter *f)
            {
                f->opcode = opcode;
            }
            static inline void filter_clear_opcode(hci_ufilter *f)
            {
                f->opcode = 0;
            }
            static inline int filter_test_opcode(int opcode, hci_ufilter *f)
            {
                return (f->opcode == opcode);
            }
    };

} // namespace direct_bt

#endif /* HCI_COMM_HPP_ */
