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
#include "HCITypes.hpp"

/**
 * - - - - - - - - - - - - - - -
 *
 * Module HCIComm:
 *
 * - BT Core Spec v5.2: Vol 4, Part E Host Controller Interface (HCI)
 */
namespace direct_bt {

    class HCIComm {
        private:
            static int hci_open_dev(const uint16_t dev_id, const uint16_t channel);
            static int hci_close_dev(int dd);

            std::recursive_mutex mtx;
            const int timeoutMS;
            const uint16_t dev_id;
            const uint16_t channel;
            int _dd; // the hci socket

        public:
            HCIComm(const uint16_t dev_id, const uint16_t channel, const int timeoutMS=number(HCIConstInt::TO_SEND_REQ_POLL_MS))
            : timeoutMS(timeoutMS), dev_id(dev_id), channel(channel), _dd(-1) {
                _dd = hci_open_dev(dev_id, channel);
            }

            /**
             * Releases this instance after issuing {@link #close()}.
             */
            ~HCIComm() { close(); }

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
                bzero(f, sizeof(*f));
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
