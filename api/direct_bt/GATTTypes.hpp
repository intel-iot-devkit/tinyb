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
        /* BT Core Spec v5.2: Vol 3, Part G GATT: 4.6.1 Discover All Characteristics of a Service */
        CHARACTERISTIC                              = 0x2803,
        /* BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration */
        CLIENT_CHARACTERISTIC_CONFIGURATION         = 0x2902
    };

    /**
     * handle -> uuid
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.7.1 Discover All Characteristic Descriptors
     *
     * Here the handle references the characteristic-descriptor-declaration.
     * The is the Characteristic Descriptor UUID.
     * </p>
     */
    class GATTUUIDHandle {
        public:
            const uint16_t handle;
            std::shared_ptr<const uuid_t> uuid;

            GATTUUIDHandle(const uint16_t handle, std::shared_ptr<const uuid_t> uuid)
            : handle(handle), uuid(uuid) {}

            std::string toString() const {
                return "handle "+uint16HexString(handle, true)+"-> uuid "+uuid->toString();
            }
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
     * handle -> value
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.6.1 Discover All Characteristics of a Service
     *
     * Here the handle is a service's characteristics-declaration
     * and the value the Characteristics Property, Characteristics Value Handle _and_ Characteristics UUID.
     * </p>
     */
    class GATTHandleValuePair {
        public:
            const uint16_t handle;
            const POctets value;

            GATTHandleValuePair(const uint16_t handle, const POctets & value)
            : handle(handle), value(value) {}

            std::string toString() const {
                return "handle "+uint16HexString(handle, true)+", value "+value.toString();
            }
    };

    /**
     * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration
     */
    class GATTClientCharacteristicConfigDecl {
        public:
            /* Client Characteristics's Config Handle  */
            const uint16_t handle;

            /* Client Characteristics's Config Value */
            uint16_t value;

            GATTClientCharacteristicConfigDecl(const uint16_t handle, const uint16_t value)
            : handle(handle), value(value) {}

            std::string toString() const {
                return "handle "+uint16HexString(handle, true)+", value "+uint16HexString(value, true);
            }
    };

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
    class GATTCharacterisicsDecl : public JavaUplink {
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
            static std::string getPropertyString(const PropertyBitVal prop);

            static std::string getPropertiesString(const PropertyBitVal properties);

            /* Characteristics's Service UUID - key to service */
            std::shared_ptr<const uuid_t> service_uuid;

            /* Characteristics's Service Handle - key to service's handle range */
            const uint16_t service_handle;
            /* Characteristics's Service Handle End of Range - key to service's handle range */
            const uint16_t service_handle_end;

            /* Characteristics Property */
            const PropertyBitVal properties;
            /* Characteristics Value Handle */
            const uint16_t handle;
            /* Characteristics UUID */
            std::shared_ptr<const uuid_t> uuid;

            /* Optional Client Characteristic Configuration declaration */
            std::shared_ptr<GATTClientCharacteristicConfigDecl> config;

            GATTCharacterisicsDecl(std::shared_ptr<const uuid_t> service_uuid,
                               const uint16_t service_handle, const uint16_t service_handle_end,
                               const PropertyBitVal properties, const uint16_t handle, std::shared_ptr<const uuid_t> uuid)
            : service_uuid(service_uuid), service_handle(service_handle), service_handle_end(service_handle_end),
              properties(properties), handle(handle), uuid(uuid), config(nullptr) {}

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
            std::string toString() const {
            	std::string service_name = "";
            	std::string char_name = "";
            	std::string config_str = "";
            	if( uuid_t::UUID16_SZ == service_uuid->getTypeSize() ) {
            		const uint16_t uuid16 = (static_cast<const uuid16_t*>(service_uuid.get()))->value;
            		service_name = ", "+GattServiceTypeToString(static_cast<GattServiceType>(uuid16));
            	}
            	if( uuid_t::UUID16_SZ == uuid->getTypeSize() ) {
            		const uint16_t uuid16 = (static_cast<const uuid16_t*>(uuid.get()))->value;
            		char_name = ", "+GattCharacteristicTypeToString(static_cast<GattCharacteristicType>(uuid16));
            	}
            	if( nullptr != config ) {
            	    config_str = ", config[ "+config->toString()+" ]";
            	}
                return "props "+uint8HexString(properties, true)+" "+getPropertiesString()+", handle "+uint16HexString(handle, true)+
                	   ", uuid "+uuid->toString()+char_name+config_str+
                       ", service[ "+service_uuid->toString()+
                       ", handle[ "+uint16HexString(service_handle, true)+".."+uint16HexString(service_handle_end, true)+" ]"+
                       service_name+" ]";
            }
    };
    typedef std::shared_ptr<GATTCharacterisicsDecl> GATTCharacterisicsDeclRef;

    /**
     * Representing a complete [Primary] Service Declaration
     * including its list of Characteristic Declarations,
     * which also may include its client config if available.
     */
    class GATTServiceDecl : public JavaUplink {
        public:
            /** The primary service declaration itself */
            const GATTUUIDHandleRange declaration;
            /** List of Characteristic Declarations as shared reference */
            std::vector<GATTCharacterisicsDeclRef> characteristicDeclList;

            GATTServiceDecl(const GATTUUIDHandleRange serviceDecl)
            : declaration(serviceDecl), characteristicDeclList() {
                characteristicDeclList.reserve(10);
            }

            std::string get_java_class() const override {
                return java_class();
            }
            static std::string java_class() {
                return std::string(JAVA_DBT_PACKAGE "DBTGattService");
            }

            std::string toString() const {
                std::string res = declaration.toString()+"[ ";
                for(size_t i=0; i<characteristicDeclList.size(); i++) {
                    if( 0 < i ) {
                        res += ", ";
                    }
                    res += std::to_string(i)+"[ "+characteristicDeclList.at(i)->toString()+" ]";
                }
                res += " ]";
                return res;
            }
    };
    typedef std::shared_ptr<GATTServiceDecl> GATTServiceDeclRef;

} // namespace direct_bt

#endif /* GATT_TYPES_HPP_ */
