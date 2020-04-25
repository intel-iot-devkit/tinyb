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

#ifndef BT_ADDRESS_HPP_
#define BT_ADDRESS_HPP_

#include <cstring>
#include <string>
#include <cstdint>

/**
 * A packed 48 bit EUI-48 identifier, formerly known as MAC-48
 * or simply network device MAC address (Media Access Control address).
 * <p>
 * Since we utilize this type within *ioctl* _and_ our high-level *types*,
 * declaration is not within our *direct_bt* namespace.
 * </p>
 */
struct __attribute__((packed)) EUI48 {
    uint8_t b[6]; // == sizeof(EUI48)

    EUI48() { bzero(b, sizeof(EUI48)); }
    EUI48(const uint8_t * b);
    EUI48(const std::string mac);
    EUI48(const EUI48 &o) noexcept = default;
    EUI48(EUI48 &&o) noexcept = default;
    EUI48& operator=(const EUI48 &o) noexcept = default;
    EUI48& operator=(EUI48 &&o) noexcept = default;

    std::string toString() const;
};

inline bool operator<(const EUI48& lhs, const EUI48& rhs)
{ return memcmp(&lhs, &rhs, sizeof(EUI48))<0; }

inline bool operator==(const EUI48& lhs, const EUI48& rhs)
{ return !memcmp(&lhs, &rhs, sizeof(EUI48)); }

inline bool operator!=(const EUI48& lhs, const EUI48& rhs)
{ return !(lhs == rhs); }

/** EUI48 MAC address matching any device, i.e. '0:0:0:0:0:0'. */
extern const EUI48 EUI48_ANY_DEVICE;
/** EUI48 MAC address matching all device, i.e. 'ff:ff:ff:ff:ff:ff'. */
extern const EUI48 EUI48_ALL_DEVICE;
/** EUI48 MAC address matching local device, i.e. '0:0:0:ff:ff:ff'. */
extern const EUI48 EUI48_LOCAL_DEVICE;

namespace direct_bt {

    /**
     * BT Core Spec v5.2:  Vol 3, Part C Generic Access Profile (GAP): 15.1.1.1 Public Bluetooth address
     * 1) BT public address used as BD_ADDR for BR/EDR physical channel is defined in Vol 2, Part B 1.2
     * - EUI-48 or MAC (6 octets)
     *
     * 2) BT public address used as BD_ADDR for the LE physical channel is defined in Vol 6, Part B 1.3
     *
     * BT Core Spec v5.2:  Vol 3, Part C Generic Access Profile (GAP): 15.1.1.2 Random Bluetooth address
     * 3) BT random address used as BD_ADDR on the LE physical channel is defined in Vol 3, Part C 10.8
     *
     * +++
     *
     * For HCI LE Address-Type it is: PUBLIC: 0x00, RANDOM: 0x01
     *
     * BT Core Spec v5.2:  Vol 4, Part E Host Controller Interface (HCI) Functionality:
     *
     * > 7.8.5: LE Set Advertising Parameters command
     * -- Own_Address_Type: public: 0x00 (default), random: 0x01, resolvable-1: 0x02, resolvable-2: 0x03
     * > 7.8.10: LE Set Scan Parameters command
     * -- Own_Address_Type: public: 0x00 (default), random: 0x01, resolvable-1: 0x02, resolvable-2: 0x03
     *
     * +++
     *
     * Couldn't yet find a reference to the BDADDR/L2CAP Address Type, which differs from HCI Address Type!
     */
    enum BDAddressType : uint8_t {
        BDADDR_BREDR      = 0x00,
        BDADDR_LE_PUBLIC  = 0x01,
        BDADDR_LE_RANDOM  = 0x02,
        BDADDR_UNDEFINED  = 0xff
    };

    enum HCIAddressType : uint8_t {
        HCIADDR_LE_PUBLIC = 0x00,
        HCIADDR_LE_RANDOM = 0x01,
        HCIADDR_UNDEFINED = 0xff
    };

    std::string getBDAddressTypeString(const BDAddressType op);

} // namespace direct_bt

#endif /* BT_ADDRESS_HPP_ */
