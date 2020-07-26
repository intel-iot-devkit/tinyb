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

#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>
#include <cstdio>

#include  <algorithm>

// #define VERBOSE_ON 1
#include <dbt_debug.hpp>

#include "GATTCharacteristic.hpp"
#include "GATTHandler.hpp"
#include "DBTDevice.hpp"

using namespace direct_bt;

#define CHAR_DECL_PROPS_ENUM(X) \
        X(Broadcast,broadcast) \
        X(Read,read) \
        X(WriteNoAck,write-without-response) \
        X(WriteWithAck,write) \
        X(Notify,notify) \
        X(Indicate,indicate) \
        X(AuthSignedWrite,authenticated-signed-writes) \
        X(ExtProps,extended-properties)

/**
        "reliable-write"
        "writable-auxiliaries"
        "encrypt-read"
        "encrypt-write"
        "encrypt-authenticated-read"
        "encrypt-authenticated-write"
        "secure-read" (Server only)
        "secure-write" (Server only)
        "authorize"
 */

#define CASE_TO_STRING2(V,S) case V: return #S;

std::string GATTCharacteristic::getPropertyString(const PropertyBitVal prop) {
    switch(prop) {
        CHAR_DECL_PROPS_ENUM(CASE_TO_STRING2)
        default: ; // fall through intended
    }
    return "Unknown property";
}

std::string GATTCharacteristic::getPropertiesString(const PropertyBitVal properties) {
    const PropertyBitVal none = static_cast<PropertyBitVal>(0);
    const uint8_t one = 1;
    bool has_pre = false;
    std::string out("[");
    for(int i=0; i<8; i++) {
        const PropertyBitVal propertyBit = static_cast<PropertyBitVal>( one << i );
        if( none != ( properties & propertyBit ) ) {
            if( has_pre ) { out.append(", "); }
            out.append(getPropertyString(propertyBit));
            has_pre = true;
        }
    }
    out.append("]");
    return out;
}

std::vector<std::unique_ptr<std::string>> GATTCharacteristic::getPropertiesStringList(const PropertyBitVal properties) {
    std::vector<std::unique_ptr<std::string>> out;
    const PropertyBitVal none = static_cast<PropertyBitVal>(0);
    const uint8_t one = 1;
    for(int i=0; i<8; i++) {
        const PropertyBitVal propertyBit = static_cast<PropertyBitVal>( one << i );
        if( none != ( properties & propertyBit ) ) {
            out.push_back( std::unique_ptr<std::string>( new std::string( getPropertyString(propertyBit) ) ) );
        }
    }
    return out;
}

std::string GATTCharacteristic::toString() const {
    std::shared_ptr<const uuid_t> service_uuid;
    uint16_t service_handle_end = 0xffff;
    GATTServiceRef serviceRef = getServiceUnchecked();
    std::string service_uuid_str = "";
    std::string service_name = "";
    std::string char_name = "";
    std::string desc_str = ", descr[ ";

    if( nullptr != serviceRef ) {
        service_uuid = serviceRef->type;
        service_uuid_str = service_uuid->toString();
        service_handle_end = serviceRef->endHandle;

        if( uuid_t::UUID16_SZ == service_uuid->getTypeSize() ) {
            const uint16_t uuid16 = (static_cast<const uuid16_t*>(service_uuid.get()))->value;
            service_name = ", "+GattServiceTypeToString(static_cast<GattServiceType>(uuid16));
        }
    }
    if( uuid_t::UUID16_SZ == value_type->getTypeSize() ) {
        const uint16_t uuid16 = (static_cast<const uuid16_t*>(value_type.get()))->value;
        char_name = ", "+GattCharacteristicTypeToString(static_cast<GattCharacteristicType>(uuid16));
    }
    for(size_t i=0; i<descriptorList.size(); i++) {
        const GATTDescriptorRef cd = descriptorList[i];
        desc_str += cd->toString() + ", ";
    }
    desc_str += " ]";
    return "handle "+uint16HexString(handle)+", props "+uint8HexString(properties)+" "+getPropertiesString()+
           ", value[type 0x"+value_type->toString()+", handle "+uint16HexString(value_handle)+char_name+desc_str+
           "], service[type 0x"+service_uuid_str+
           ", handle[ "+uint16HexString(service_handle)+".."+uint16HexString(service_handle_end)+" ]"+
           service_name+", enabled[notify "+std::to_string(enabledNotifyState)+", indicate "+std::to_string(enabledIndicateState)+"] ]";
}

std::shared_ptr<GATTService> GATTCharacteristic::getServiceChecked() const {
    std::shared_ptr<GATTService> ref = wbr_service.lock();
    if( nullptr == ref ) {
        throw IllegalStateException("GATTCharacteristic's service already destructed: "+toString(), E_FILE_LINE);
    }
    return ref;
}

std::shared_ptr<DBTDevice> GATTCharacteristic::getDeviceUnchecked() const {
    std::shared_ptr<GATTService> s = getServiceUnchecked();
    if( nullptr != s ) {
        return s->getDeviceUnchecked();
    }
    return nullptr;
}

std::shared_ptr<DBTDevice> GATTCharacteristic::getDeviceChecked() const {
    return getServiceChecked()->getDeviceChecked();
}

bool GATTCharacteristic::configNotificationIndication(const bool enableNotification, const bool enableIndication, bool enabledState[2]) {
    enabledState[0] = false;
    enabledState[1] = false;

    const bool hasEnableNotification = hasProperties(GATTCharacteristic::PropertyBitVal::Notify);
    const bool hasEnableIndication = hasProperties(GATTCharacteristic::PropertyBitVal::Indicate);
    if( !hasEnableNotification && !hasEnableIndication ) {
        DBG_PRINT("Characteristic has neither Notify nor Indicate property present: %s", toString().c_str());
        return false;
    }

    std::shared_ptr<DBTDevice> device = getDeviceUnchecked();
    std::shared_ptr<GATTHandler> gatt = nullptr != device ? device->getGATTHandler() : nullptr;
    if( nullptr == gatt ) {
        if( !enableNotification && !enableIndication ) {
            // OK to have GATTHandler being shutdown @ disable
            DBG_PRINT("Characteristic's device GATTHandle not connected: %s, %s", toString().c_str(), device->toString().c_str());
            return false;
        }
        throw IllegalStateException("Characteristic's device GATTHandle not connected: "+
                toString() + ", " + device->toString(), E_FILE_LINE);
    }
    const bool resEnableNotification = hasEnableNotification && enableNotification;
    const bool resEnableIndication = hasEnableIndication && enableIndication;

    if( resEnableNotification == enabledNotifyState &&
        resEnableIndication == enabledIndicateState )
    {
        enabledState[0] = resEnableNotification;
        enabledState[1] = resEnableIndication;
        DBG_PRINT("GATTCharacteristic::configNotificationIndication: Unchanged: notification[shall %d, has %d: %d == %d], indication[shall %d, has %d: %d == %d]",
                enableNotification, hasEnableNotification, enabledNotifyState, resEnableNotification,
                enableIndication, hasEnableIndication, enabledIndicateState, resEnableIndication);
        return true;
    }

    GATTDescriptorRef cccd = this->getClientCharacteristicConfig();
    if( nullptr == cccd ) {
        DBG_PRINT("Characteristic has no ClientCharacteristicConfig descriptor: %s", toString().c_str());
        return false;
    }
    bool res = gatt->configNotificationIndication(*cccd, resEnableNotification, resEnableIndication);
    DBG_PRINT("GATTCharacteristic::configNotificationIndication: res %d, notification[shall %d, has %d: %d -> %d], indication[shall %d, has %d: %d -> %d]",
            res,
            enableNotification, hasEnableNotification, enabledNotifyState, resEnableNotification,
            enableIndication, hasEnableIndication, enabledIndicateState, resEnableIndication);
    if( res ) {
        enabledNotifyState = resEnableNotification;
        enabledIndicateState = resEnableIndication;
        enabledState[0] = resEnableNotification;
        enabledState[1] = resEnableIndication;
    }
    return res;
}

bool GATTCharacteristic::enableNotificationOrIndication(bool enabledState[2]) {
    const bool hasEnableNotification = hasProperties(GATTCharacteristic::PropertyBitVal::Notify);
    const bool hasEnableIndication = hasProperties(GATTCharacteristic::PropertyBitVal::Indicate);

    const bool enableNotification = hasEnableNotification;
    const bool enableIndication = !enableNotification && hasEnableIndication;

    return configNotificationIndication(enableNotification, enableIndication, enabledState);
}

bool GATTCharacteristic::addCharacteristicListener(std::shared_ptr<GATTCharacteristicListener> l) {
    return getDeviceChecked()->addCharacteristicListener(l);
}

bool GATTCharacteristic::addCharacteristicListener(std::shared_ptr<GATTCharacteristicListener> l, bool enabledState[2]) {
    if( !enableNotificationOrIndication(enabledState) ) {
        return false;
    }
    return addCharacteristicListener(l);
}

bool GATTCharacteristic::removeCharacteristicListener(std::shared_ptr<GATTCharacteristicListener> l, bool disableIndicationNotification) {
    if( disableIndicationNotification ) {
        bool enabledState[2];
        configNotificationIndication(false, false, enabledState);
    }
    return getDeviceChecked()->removeCharacteristicListener(l);
}

int GATTCharacteristic::removeAllCharacteristicListener(bool disableIndicationNotification) {
    if( disableIndicationNotification ) {
        bool enabledState[2];
        configNotificationIndication(false, false, enabledState);
    }
    return getDeviceChecked()->removeAllCharacteristicListener();
}

bool GATTCharacteristic::readValue(POctets & res, int expectedLength) {
    std::shared_ptr<DBTDevice> device = getDeviceChecked();
    std::shared_ptr<GATTHandler> gatt = device->getGATTHandler();
    if( nullptr == gatt ) {
        throw IllegalStateException("Characteristic's device GATTHandle not connected: "+
                toString() + ", " + device->toString(), E_FILE_LINE);
    }
    return gatt->readCharacteristicValue(*this, res, expectedLength);
}
/**
 * BT Core Spec v5.2: Vol 3, Part G GATT: 4.9.3 Write Characteristic Value
 */
bool GATTCharacteristic::writeValue(const TROOctets & value) {
    std::shared_ptr<DBTDevice> device = getDeviceChecked();
    std::shared_ptr<GATTHandler> gatt = device->getGATTHandler();
    if( nullptr == gatt ) {
        throw IllegalStateException("Characteristic's device GATTHandle not connected: "+
                toString() + ", " + device->toString(), E_FILE_LINE);
    }
    return gatt->writeCharacteristicValue(*this, value);
}

/**
 * BT Core Spec v5.2: Vol 3, Part G GATT: 4.9.1 Write Characteristic Value Without Response
 */
bool GATTCharacteristic::writeValueNoResp(const TROOctets & value) {
    std::shared_ptr<DBTDevice> device = getDeviceChecked();
    std::shared_ptr<GATTHandler> gatt = device->getGATTHandler();
    if( nullptr == gatt ) {
        throw IllegalStateException("Characteristic's device GATTHandle not connected: "+
                toString() + ", " + device->toString(), E_FILE_LINE);
    }
    return gatt->writeCharacteristicValueNoResp(*this, value);
}
