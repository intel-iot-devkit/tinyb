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

#ifndef GATT_DESCRIPTOR_HPP_
#define GATT_DESCRIPTOR_HPP_

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

#include "DBTTypes.hpp"

/**
 * - - - - - - - - - - - - - - -
 *
 * Module GATTDescriptor:
 *
 * - BT Core Spec v5.2: Vol 3, Part G Generic Attribute Protocol (GATT)
 * - BT Core Spec v5.2: Vol 3, Part G GATT: 2.6 GATT Profile Hierarchy
 */
namespace direct_bt {

    class DBTDevice; // forward
    class GATTCharacteristic; // forward
    typedef std::shared_ptr<GATTCharacteristic> GATTCharacteristicRef;

    /**
     * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3 Characteristic Descriptor
     */
    class GATTDescriptor : public DBTObject {
        private:
            /** Descriptor's characteristic weak back-reference */
            std::weak_ptr<GATTCharacteristic> wbr_characteristic;

        public:
            static const uuid16_t TYPE_EXT_PROP;
            static const uuid16_t TYPE_USER_DESC;
            static const uuid16_t TYPE_CCC_DESC;
            static std::shared_ptr<const uuid_t> getStaticType(const uuid16_t & type) {
                return std::shared_ptr<const uuid_t>(&type, [](const uuid_t *){});
            }

            /**
             * Following UUID16 GATT profile attribute types are listed under:
             * BT Core Spec v5.2: Vol 3, Part G GATT: 3.4 Summary of GATT Profile Attribute Types
             */
            enum Type : uint16_t {
                CHARACTERISTIC_APPEARANCE                   = 0x2A01,
                CHARACTERISTIC_PERIPHERAL_PRIV_FLAG         = 0x2A02,
                CHARACTERISTIC_RECONNECTION_ADDRESS         = 0x2A03,
                CHARACTERISTIC_PERIPHERAL_PREF_CONN         = 0x2A04,
                CHARACTERISTIC_SERVICE_CHANGED              = 0x2A05,

                /* BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.1 Characteristic Extended Properties */
                CHARACTERISTIC_EXTENDED_PROPERTIES          = 0x2900,
                /* BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.2 Characteristic User Description (Characteristic Descriptor, optional, single, string) */
                CHARACTERISTIC_USER_DESCRIPTION             = 0x2901,
                /* BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration (Characteristic Descriptor, optional, single, uint16_t bitfield) */
                CLIENT_CHARACTERISTIC_CONFIGURATION         = 0x2902,
                /* BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.4 Server Characteristic Configuration (Characteristic Descriptor, optional, single, bitfield) */
                SERVER_CHARACTERISTIC_CONFIGURATION         = 0x2903,
                /* BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.5 Characteristic Presentation Format (Characteristic Descriptor, optional, single, complex) */
                CHARACTERISTIC_PRESENTATION_FORMAT          = 0x2904,
                CHARACTERISTIC_AGGREGATE_FORMAT             = 0x2905,

                /** Our identifier to mark a custom vendor Characteristic Descriptor */
                CUSTOM_CHARACTERISTIC_DESCRIPTION           = 0x8888
            };

            /** Type of descriptor */
            std::shared_ptr<const uuid_t> type;

            /**
             * Characteristic Descriptor Handle
             * <p>
             * Attribute handles are unique for each device (server) (BT Core Spec v5.2: Vol 3, Part F Protocol..: 3.2.2 Attribute Handle).
             * </p>
             */
            const uint16_t handle;

            /* Characteristics Descriptor's Value */
            POctets value;

            GATTDescriptor(const GATTCharacteristicRef & characteristic, const std::shared_ptr<const uuid_t> & type,
                           const uint16_t handle)
            : wbr_characteristic(characteristic), type(type), handle(handle), value(0) {}

            std::string get_java_class() const override {
                return java_class();
            }
            static std::string java_class() {
                return std::string(JAVA_DBT_PACKAGE "DBTGattDescriptor");
            }

            std::shared_ptr<GATTCharacteristic> getCharacteristic() const { return wbr_characteristic.lock(); }

            std::shared_ptr<DBTDevice> getDevice() const;

            virtual std::string toString() const {
                return "[type 0x"+type->toString()+", handle "+uint16HexString(handle)+", value["+value.toString()+"]]";
            }

            /** Value is uint16_t bitfield */
            bool isExtendedProperties() const { return TYPE_EXT_PROP == *type; }

            /* BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration (Characteristic Descriptor, optional, single, uint16_t bitfield) */
            bool isClientCharacteristicConfiguration() const { return TYPE_CCC_DESC == *type; }

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.12.1 Read Characteristic Descriptor
             * <p>
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.12.2 Read Long Characteristic Descriptor
             * </p>
             * <p>
             * If expectedLength = 0, then only one ATT_READ_REQ/RSP will be used.
             * </p>
             * <p>
             * If expectedLength < 0, then long values using multiple ATT_READ_BLOB_REQ/RSP will be used until
             * the response returns zero. This is the default parameter.
             * </p>
             * <p>
             * If expectedLength > 0, then long values using multiple ATT_READ_BLOB_REQ/RSP will be used
             * if required until the response returns zero.
             * </p>
             * <p>
             * Convenience delegation call to GATTHandler via DBTDevice
             * If the DBTDevice's GATTHandler is null, i.e. not connected, an IllegalStateException is thrown.
             * </p>
             */
            bool readValue(int expectedLength=-1);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.12.3 Write Characteristic Descriptors
             * <p>
             * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3 Characteristic Descriptor
             * </p>
             * <p>
             * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration
             * </p>
             * <p>
             * Convenience delegation call to GATTHandler via DBTDevice
             * If the DBTDevice's GATTHandler is null, i.e. not connected, an IllegalStateException is thrown.
             * </p>
             */
            bool writeValue();
    };
    typedef std::shared_ptr<GATTDescriptor> GATTDescriptorRef;

    inline bool operator==(const GATTDescriptor& lhs, const GATTDescriptor& rhs)
    { return lhs.handle == rhs.handle; /** unique attribute handles */ }

    inline bool operator!=(const GATTDescriptor& lhs, const GATTDescriptor& rhs)
    { return !(lhs == rhs); }

} // namespace direct_bt

#endif /* GATT_DESCRIPTOR_HPP_ */
