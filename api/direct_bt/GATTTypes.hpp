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

#ifndef GATT_TYPES_HPP_
#define GATT_TYPES_HPP_

#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>

#include <mutex>
#include <atomic>

#include "UUID.hpp"
#include "BTTypes.hpp"
#include "OctetTypes.hpp"
#include "ATTPDUTypes.hpp"

/* Only to resolve high level service and characteristic names */
#include "GATTNumbers.hpp"

#include "GATTService.hpp"

#include "JavaUplink.hpp"

/**
 * BT Core Spec v5.2: Vol 3, Part F Attribute Protocol (ATT)
 * BT Core Spec v5.2: Vol 3, Part G Generic Attribute Protocol (GATT)
 *
 * BT Core Spec v5.2: Vol 3, Part G GATT: 3.4 Summary of GATT Profile Attribute Types
 */

namespace direct_bt {

    /**
     * Following UUID16 GATT profile attribute types are listed under:
     * BT Core Spec v5.2: Vol 3, Part G GATT: 3.4 Summary of GATT Profile Attribute Types
     */
    enum GattAttributeType : uint16_t {
        /* BT Core Spec v5.2: Vol 3, Part G GATT: 4.4.1 Discover All Primary Services */
        PRIMARY_SERVICE                             = 0x2800,
        SECONDARY_SERVICE                           = 0x2801,
        INCLUDE_DECLARATION                         = 0x2802,
        /* BT Core Spec v5.2: Vol 3, Part G GATT: 4.6.1 Discover All Characteristics of a Service */
        CHARACTERISTIC                              = 0x2803,

        CHARACTERISTIC_APPEARANCE                   = 0x2A01,
        CHARACTERISTIC_PERIPHERAL_PRIV_FLAG         = 0x2A02,
        CHARACTERISTIC_RECONNECTION_ADDRESS         = 0x2A03,
        CHARACTERISTIC_PERIPHERAL_PREF_CONN         = 0x2A04,
        CHARACTERISTIC_SERVICE_CHANGED              = 0x2A05,

        /* BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.1 Characteristic Extended Properties */
        CHARACTERISTIC_EXTENDED_PROPERTIES          = 0x2900,
        /* BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.2 Characteristic User Description (Characteristic Descriptor, optional, single, string) */
        CHARACTERISTIC_USER_DESCRIPTION             = 0x2901,
        /* BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration (Characteristic Descriptor, optional, single, bitfield) */
        CLIENT_CHARACTERISTIC_CONFIGURATION         = 0x2902,
        /* BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.4 Server Characteristic Configuration (Characteristic Descriptor, optional, single, bitfield) */
        SERVER_CHARACTERISTIC_CONFIGURATION         = 0x2903,
        /* BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.5 Characteristic Presentation Format (Characteristic Descriptor, optional, single, complex) */
        CHARACTERISTIC_PRESENTATION_FORMAT          = 0x2904,
        CHARACTERISTIC_AGGREGATE_FORMAT             = 0x2905

    };

} // namespace direct_bt

#endif /* GATT_TYPES_HPP_ */
