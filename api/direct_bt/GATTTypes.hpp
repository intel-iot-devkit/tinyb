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

    /**
     * uuid -> handle-range[ startHandle .. endHandle ]
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.4.1 Discover All Primary Services
     *
     * Here the uuid is a service uuid
     * and the handle-range it's characteristics-declaration
     * </p>
     */
    class GATTUUIDHandleRange {
        public:
    		enum Type : uint8_t {
    			Service = 0,
    		    Characteristic = 1
    		};
    		const Type type;
            const uint16_t startHandle;
            const uint16_t endHandle;
            std::shared_ptr<const uuid_t> uuid;

            GATTUUIDHandleRange(const Type t, const uint16_t startHandle, const uint16_t endHandle, std::shared_ptr<const uuid_t> uuid)
            : type(t), startHandle(startHandle), endHandle(endHandle), uuid(uuid) {}

            std::string toString() const {
            	std::string name = "";
            	if( uuid_t::UUID16_SZ == uuid->getTypeSize() ) {
            		const uint16_t uuid16 = (static_cast<const uuid16_t*>(uuid.get()))->value;
            		if( Type::Service == type ) {
            			name = " - "+GattServiceTypeToString(static_cast<GattServiceType>(uuid16));
            		} else if( Type::Characteristic == type ) {
            			name = " - "+GattCharacteristicTypeToString(static_cast<GattCharacteristicType>(uuid16));
            		}
            	}
                return "uuid "+uuid->toString()+", handle [ "+uint16HexString(startHandle, true)+".."+uint16HexString(endHandle, true)+" ]"+name;
            }
    };


    /**
     *
     * JavaUplink Semantic Types
     *
     */


    class GATTCharacteristic; // forward
    typedef std::shared_ptr<GATTCharacteristic> GATTCharacteristicRef;

    /**
     * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3 Characteristic Descriptor
     */
    class GATTDescriptor : public JavaUplink {
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

            /* Characteristic Descriptor's Characteristic back-reference */
            GATTCharacteristicRef characteristic;

            /** Actual type of descriptor */
            std::shared_ptr<const uuid_t> type;

            /* Characteristics Descriptor's Handle  */
            const uint16_t handle;

            /* Characteristics Descriptor's Value */
            POctets value;

            GATTDescriptor(const GATTCharacteristicRef & characteristic, const std::shared_ptr<const uuid_t> & type,
                           const uint16_t handle)
            : characteristic(characteristic), type(type), handle(handle), value(0) {}

            GATTDescriptor(const GATTCharacteristicRef & characteristic, const std::shared_ptr<const uuid_t> & type,
                           const uint16_t handle, const int valueSize)
            : characteristic(characteristic), type(type), handle(handle), value(valueSize) {}

            GATTDescriptor(const GATTCharacteristicRef & characteristic, const std::shared_ptr<const uuid_t> & type,
                           const uint16_t handle, const uint8_t *valueSource, const int valueSize)
            : characteristic(characteristic), type(type), handle(handle), value(valueSource, valueSize) {}

            std::string get_java_class() const override {
                return java_class();
            }
            static std::string java_class() {
                return std::string(JAVA_DBT_PACKAGE "DBTGattDescriptor");
            }

            virtual std::string toString() const {
                return "Descriptor[type "+type->toString()+", handle "+uint16HexString(handle)+", value["+value.toString()+"]]";
            }

            /** Value is uint16_t bitfield */
            bool isExtendedProperties() { return TYPE_EXT_PROP == *type; }

            /* BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration (Characteristic Descriptor, optional, single, uint16_t bitfield) */
            bool isClientCharacteristicConfiguration() { return TYPE_CCC_DESC == *type; }
    };
    typedef std::shared_ptr<GATTDescriptor> GATTDescriptorRef;

    class GATTService; // forward
    typedef std::shared_ptr<GATTService> GATTServiceRef;

    /**
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.1 Characteristic Declaration Attribute Value
     * </p>
     * handle -> CDAV value
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.6.1 Discover All Characteristics of a Service
     *
     * Here the handle is a service's characteristics-declaration
     * and the value the Characteristics Property, Characteristics Value Handle _and_ Characteristics UUID.
     * </p>
     */
    class GATTCharacteristic : public JavaUplink {
        public:
            /** BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.1.1 Characteristic Properties */
            enum PropertyBitVal : uint8_t {
                Broadcast = 0x01,
                Read = 0x02,
                WriteNoAck = 0x04,
                WriteWithAck = 0x08,
                Notify = 0x10,
                Indicate = 0x20,
                AuthSignedWrite = 0x40,
                ExtProps = 0x80
            };
            /**
             * Returns string values as defined in <https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/gatt-api.txt>
             * <pre>
             * org.bluez.GattCharacteristic1 :: array{string} Flags [read-only]
             * </pre>
             */
            static std::string getPropertyString(const PropertyBitVal prop);

            static std::string getPropertiesString(const PropertyBitVal properties);
            static std::vector<std::unique_ptr<std::string>> getPropertiesStringList(const PropertyBitVal properties);

            /* Characteristics's Service back-reference */
            GATTServiceRef service;

            /* Characteristics's Service Handle - key to service's handle range, retrieved from Characteristics data */
            const uint16_t service_handle;

            /* Characteristics Property */
            const PropertyBitVal properties;
            /* Characteristics Value Handle */
            const uint16_t handle;
            /* Characteristics UUID */
            std::shared_ptr<const uuid_t> uuid;

            /** List of Characteristic Descriptions as shared reference */
            std::vector<GATTDescriptorRef> characteristicDescList;

            /* Optional Client Characteristic Configuration index within characteristicDescList */
            int clientCharacteristicsConfigIndex = -1;

            GATTCharacteristic(const GATTServiceRef & service, const uint16_t service_handle,
                                   const PropertyBitVal properties, const uint16_t handle, std::shared_ptr<const uuid_t> uuid)
            : service(service), service_handle(service_handle),
              properties(properties), handle(handle), uuid(uuid) {}

            std::string get_java_class() const override {
                return java_class();
            }
            static std::string java_class() {
                return std::string(JAVA_DBT_PACKAGE "DBTGattCharacteristic");
            }

            bool hasProperties(const PropertyBitVal v) const { return v == ( properties & v ); }

            std::string getPropertiesString() const {
                return getPropertiesString(properties);
            }
            std::string toString() const;

            GATTDescriptorRef getClientCharacteristicConfig() {
                if( 0 > clientCharacteristicsConfigIndex ) {
                    return nullptr;
                }
                return characteristicDescList.at(clientCharacteristicsConfigIndex);
            }
    };

    class DBTDevice; // forward

    /**
     * Representing a complete [Primary] Service Declaration
     * including its list of Characteristic Declarations,
     * which also may include its client config if available.
     */
    class GATTService : public JavaUplink {
        public:
            /* Service's Device back-reference */
            std::shared_ptr<DBTDevice> device;

            const bool isPrimary;

            /** The primary service declaration itself */
            const GATTUUIDHandleRange declaration;

            /** List of Characteristic Declarations as shared reference */
            std::vector<GATTCharacteristicRef> characteristicDeclList;

            GATTService(const std::shared_ptr<DBTDevice> &device, const bool isPrimary, const GATTUUIDHandleRange serviceDecl)
            : device(device), isPrimary(isPrimary), declaration(serviceDecl), characteristicDeclList() {
                characteristicDeclList.reserve(10);
            }

            std::string get_java_class() const override {
                return java_class();
            }
            static std::string java_class() {
                return std::string(JAVA_DBT_PACKAGE "DBTGattService");
            }

            std::string toString() const;
    };

} // namespace direct_bt

#endif /* GATT_TYPES_HPP_ */
