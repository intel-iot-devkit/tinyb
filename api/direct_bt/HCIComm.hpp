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

namespace direct_bt {

    enum HCIDefaults : int {
        HCI_TO_SEND_REQ_POLL_MS = 1000
    };

    enum HCI_Event_Types : uint8_t {
        LE_Advertising_Report   = 0x3E
    };

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
            bool send_req(const uint16_t opcode, const void *command, const uint8_t command_len,
                          const uint16_t exp_event, void *response, const uint8_t response_len);

            bool le_set_scan_enable(const uint8_t enable, const uint8_t filter_dup);
            bool le_set_scan_parameters(const uint8_t type, const uint16_t interval,
                                        const uint16_t window, const uint8_t own_type,
                                        const uint8_t filter_policy);

        public:
            HCIComm(const uint16_t dev_id, const uint16_t channel, const int timeoutMS=HCI_TO_SEND_REQ_POLL_MS)
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
             * Even if not utilizing a HCI channel, it has been observed that maintaining such
             * enhanced performance on subsequent communication, i.e. GATT over L2CAP.
             * </p>
             */
            uint16_t le_create_conn(const EUI48 &peer_bdaddr,
                                    const HCIAddressType peer_mac_type=HCIAddressType::HCIADDR_LE_PUBLIC,
                                    const HCIAddressType own_mac_type=HCIAddressType::HCIADDR_LE_PUBLIC,
                                    const uint16_t interval=0x0004, const uint16_t window=0x0004,
                                    const uint16_t min_interval=0x000F, const uint16_t max_interval=0x000F,
                                    const uint16_t latency=0x0000, const uint16_t supervision_timeout=0x0C80,
                                    const uint16_t min_ce_length=0x0001, const uint16_t max_ce_length=0x0001,
                                    const uint8_t initiator_filter=0);

            /**
             * Establish a connection to the given BREDR (non LE).
             */
            uint16_t create_conn(const EUI48 &bdaddr,
                                 const uint16_t pkt_type=HCI_DM1 | HCI_DM3 | HCI_DM5 | HCI_DH1 | HCI_DH3 | HCI_DH5,
                                 const uint16_t clock_offset=0x0000, const uint8_t role_switch=0x01);

            /**
             * Disconnect an established connection.
             */
            bool disconnect(const uint16_t conn_handle, const uint8_t reason=0);

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
