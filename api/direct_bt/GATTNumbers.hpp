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

#ifndef GATT_NUMBERS_HPP_
#define GATT_NUMBERS_HPP_

#include <cstdint>
#include "UUID.hpp"
#include "BasicTypes.hpp"
#include "OctetTypes.hpp"
#include "BTTypes.hpp"
#include "ieee11073/DataTypes.hpp"

namespace direct_bt {

/**
 * Higher level GATT values for services and so forth ...
 */

/**
 * GATT Service Type, each encapsulating a set of Characteristics.
 * <p>
 * https://www.bluetooth.com/specifications/gatt/services/
 * </p>
 */
enum GattServiceType : uint16_t {
    /** The generic_access service contains generic information about the device. All available Characteristics are readonly. */
    GENERIC_ACCESS                              = 0x1800,
    /** The Health Thermometer service exposes temperature and other data from a thermometer intended for healthcare and fitness applications. */
    HEALTH_THERMOMETER                          = 0x1809,
	/** The Device Information Service exposes manufacturer and/or vendor information about a device. */
	DEVICE_INFORMATION                          = 0x180A,
    /** The Battery Service exposes the state of a battery within a device. */
    BATTERY_SERVICE                             = 0x180F,
};
std::string GattServiceTypeToString(const GattServiceType v);

/**
 * GATT Assigned Characteristic Attribute Type for single logical value.
 * <p>
 * https://www.bluetooth.com/specifications/gatt/characteristics/
 * </p>
 */
enum GattCharacteristicType : uint16_t {
    //
    // GENERIC_ACCESS
    //
    DEVICE_NAME                                 = 0x2A00,
    APPEARANCE                                  = 0x2A01,
    PERIPHERAL_PRIVACY_FLAG                     = 0x2A02,
    RECONNECTION_ADDRESS                        = 0x2A03,
    PERIPHERAL_PREFERRED_CONNECTION_PARAMETERS  = 0x2A04,

    /** Mandatory: sint16 10^-2: Celsius */
    TEMPERATURE                                 = 0x2A6E,

    /** Mandatory: sint16 10^-1: Celsius */
    TEMPERATURE_CELSIUS                         = 0x2A1F,
    TEMPERATURE_FAHRENHEIT                      = 0x2A20,

    //
    // HEALTH_THERMOMETER
    //
    TEMPERATURE_MEASUREMENT                     = 0x2A1C,
    /** Mandatory: 8bit: 1 armpit, 2 body (general), 3(ear), 4 (finger), ... */
    TEMPERATURE_TYPE                            = 0x2A1D,
    INTERMEDIATE_TEMPERATURE                    = 0x2A1E,
    MEASUREMENT_INTERVAL                        = 0x2A21,

	//
	// DEVICE_INFORMATION
	//
	/** Mandatory: uint40 */
	SYSTEM_ID 									= 0x2A23,
	MODEL_NUMBER_STRING 						= 0x2A24,
	SERIAL_NUMBER_STRING 						= 0x2A25,
	FIRMWARE_REVISION_STRING 					= 0x2A26,
	HARDWARE_REVISION_STRING 					= 0x2A27,
	SOFTWARE_REVISION_STRING 					= 0x2A28,
	MANUFACTURER_NAME_STRING 					= 0x2A29,
	REGULATORY_CERT_DATA_LIST 					= 0x2A2A,
	PNP_ID 										= 0x2A50,
};
std::string GattCharacteristicTypeToString(const GattCharacteristicType v);

enum GattCharacteristicProperty : uint8_t {
    Broadcast = 0x01,
    Read = 0x02,
    WriteNoAck = 0x04,
    WriteWithAck = 0x08,
    Notify = 0x10,
    Indicate = 0x20,
    AuthSignedWrite = 0x40,
    ExtProps = 0x80,
    /** FIXME: extension? */
    ReliableWriteExt = 0x81,
    /** FIXME: extension? */
    AuxWriteExt = 0x82
};
std::string GattCharacteristicPropertyToString(const GattCharacteristicProperty v);

enum GattRequirementSpec : uint8_t {
    Excluded    = 0x00,
    Mandatory   = 0x01,
    Optional    = 0x02,
    Conditional = 0x03,
    if_characteristic_supported = 0x11,
    if_notify_or_indicate_supported = 0x12,
    C1 = 0x21,
};
std::string GattRequirementSpecToString(const GattRequirementSpec v);

struct GattCharacteristicPropertySpec {
    const GattCharacteristicProperty property;
    const GattRequirementSpec requirement;

    std::string toString() const;
};

struct GattClientCharacteristicConfigSpec {
    const GattRequirementSpec requirement;
    const GattCharacteristicPropertySpec read;
    const GattCharacteristicPropertySpec writeWithAck;

    std::string toString() const;
};

struct GattCharacteristicSpec {
    const GattCharacteristicType characteristic;
    const GattRequirementSpec requirement;

    enum PropertySpecIdx : int {
        ReadIdx = 0,
        WriteNoAckIdx,
        WriteWithAckIdx,
        AuthSignedWriteIdx,
        ReliableWriteExtIdx,
        NotifyIdx,
        IndicateIdx,
        AuxWriteExtIdx,
        BroadcastIdx
    };
    /** Aggregated in PropertySpecIdx order */
    const std::vector<GattCharacteristicPropertySpec> propertySpec;

    const GattClientCharacteristicConfigSpec clientConfig;

    std::string toString() const;
};

struct GattServiceCharacteristic {
    const GattServiceType service;
    const std::vector<GattCharacteristicSpec> characteristics;

    std::string toString() const;
};

/**
 * Intentionally ease compile and linker burden by using 'extern' instead of 'inline',
 * as the latter would require compile to crunch the structure
 * and linker to chose where to place the actual artifact.
 */
extern const GattServiceCharacteristic GATT_GENERIC_ACCESS_SRVC;
extern const GattServiceCharacteristic GATT_HEALTH_THERMOMETER_SRVC;
extern const GattServiceCharacteristic GATT_DEVICE_INFORMATION_SRVC;
extern const std::vector<const GattServiceCharacteristic*> GATT_SERVICES;

/**
 * Find the GattServiceCharacteristic entry by given uuid16,
 * denominating either a GattServiceType or GattCharacteristicType.
 */
const GattServiceCharacteristic * findGattServiceChar(const uint16_t uuid16);

/**
 * Find the GattCharacteristicSpec entry by given uuid16,
 * denominating either a GattCharacteristicType.
 */
const GattCharacteristicSpec * findGattCharSpec(const uint16_t uuid16);

/********************************************************/
/********************************************************/
/********************************************************/

/**
 * Converts a GATT Name (not null-terminated) UTF8 to a null-terminated C++ string
 */
std::string GattNameToString(const TROOctets &v);

struct PeriphalPreferredConnectionParameters {
    /** mandatory [6..3200] x 1.25ms */
    const uint16_t minConnectionInterval;
    /** mandatory [6..3200] x 1.25ms and >= minConnectionInterval */
    const uint16_t maxConnectionInterval;
    /** mandatory [1..1000] */
    const uint16_t slaveLatency;
    /** mandatory [10..3200] */
    const uint16_t connectionSupervisionTimeoutMultiplier;

    PeriphalPreferredConnectionParameters(const TROOctets &source);
    std::string toString() const;
};

/** https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Services/org.bluetooth.service.generic_access.xml */
class GenericAccess {
	public:
		const std::string deviceName;
		const AppearanceCat appearance;
		const PeriphalPreferredConnectionParameters prefConnParam;

		GenericAccess(const std::string & deviceName, const AppearanceCat appearance, const PeriphalPreferredConnectionParameters & prefConnParam)
		: deviceName(deviceName), appearance(appearance), prefConnParam(prefConnParam) {}

		std::string toString() const;
};

struct PnP_ID {
    const uint8_t vendor_id_source;
    const uint16_t vendor_id;
    const uint16_t product_id;
    const uint16_t product_version;

    PnP_ID()
    : vendor_id_source(0), vendor_id(0), product_id(0), product_version(0) {}
    PnP_ID(const TROOctets &source);
    PnP_ID(const uint8_t vendor_id_source, const uint16_t vendor_id, const uint16_t product_id, const uint16_t product_version)
    : vendor_id_source(vendor_id_source), vendor_id(vendor_id), product_id(product_id), product_version(product_version) {}

    std::string toString() const;
};

/** https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Services/org.bluetooth.service.device_information.xml */
class DeviceInformation {
    public:
        const POctets systemID;
        const std::string modelNumber;
        const std::string serialNumber;
        const std::string firmwareRevision;
        const std::string hardwareRevision;
        const std::string softwareRevision;
        const std::string manufacturer;
        const POctets regulatoryCertDataList;
        const PnP_ID pnpID;

        DeviceInformation(const POctets &systemID, const std::string &modelNumber, const std::string &serialNumber,
                          const std::string &firmwareRevision, const std::string &hardwareRevision, const std::string &softwareRevision,
                          const std::string &manufacturer, const POctets &regulatoryCertDataList, const PnP_ID &pnpID)
        : systemID(systemID), modelNumber(modelNumber), serialNumber(serialNumber), firmwareRevision(firmwareRevision),
          hardwareRevision(hardwareRevision), softwareRevision(softwareRevision), manufacturer(manufacturer),
          regulatoryCertDataList(regulatoryCertDataList), pnpID(pnpID) {}

        std::string toString() const;
};

/** https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Services/org.bluetooth.service.battery_service.xml */
class BatteryService {
    // TODO
};

/** https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.temperature_measurement.xml */
class TemperatureMeasurementCharateristic {
    public:
        enum Bits : uint8_t {
            /** bit 0: If set, temperature is in Fahrenheit, otherwise Celsius. */
            IS_TEMP_FAHRENHEIT  = 1,
            /** bit 1: If set, timestamp field present, otherwise not.. */
            HAS_TIMESTAMP       = 2,
            /** bit 2: If set, temperature type field present, otherwise not.. */
            HAS_TEMP_TYPE       = 4
        };
        /** Bitfields of Bits. 1 byte. */
        const uint8_t flags;

        /** In Celsius if IS_TEMP_FAHRENHEIT is set, otherwise Fahrenheit. 4 bytes. */
        const float temperatureValue;

        /** Timestamp, if HAS_TIMESTAMP is set. 7 bytes(!?) here w/o fractions. */
        const ieee11073::AbsoluteTime timestamp;

        /** Temperature Type, if HAS_TEMP_TYPE is set: Format ????. 1 byte (!?). */
        const uint8_t temperature_type;

        static std::shared_ptr<TemperatureMeasurementCharateristic> get(const TROOctets &source);
        static std::shared_ptr<TemperatureMeasurementCharateristic> get(const TOctetSlice &source) {
            const TROOctets o(source.get_ptr(0), source.getSize());
            return get(o);
        }

        TemperatureMeasurementCharateristic(const uint8_t flags, const float temperatureValue, const ieee11073::AbsoluteTime &timestamp, const uint8_t temperature_type)
        : flags(flags), temperatureValue(temperatureValue), timestamp(timestamp), temperature_type(temperature_type) {}

        bool isFahrenheit() const { return 0 != ( flags & Bits::IS_TEMP_FAHRENHEIT ); }
        bool hasTimestamp() const { return 0 != ( flags & Bits::HAS_TIMESTAMP ); }
        bool hasTemperatureType() const { return 0 != ( flags & Bits::HAS_TEMP_TYPE ); }

        std::string toString() const;
};


/* Application error */

#define ATT_ECODE_IO				0x80
#define ATT_ECODE_TIMEOUT			0x81
#define ATT_ECODE_ABORTED			0x82

#define ATT_MAX_VALUE_LEN			512
#define ATT_DEFAULT_L2CAP_MTU			48
#define ATT_DEFAULT_LE_MTU			23

/* Flags for Execute Write Request Operation */

#define ATT_CANCEL_ALL_PREP_WRITES              0x00
#define ATT_WRITE_ALL_PREP_WRITES               0x01

/* Find Information Response Formats */

#define ATT_FIND_INFO_RESP_FMT_16BIT		0x01
#define ATT_FIND_INFO_RESP_FMT_128BIT		0x02

} // namespace direct_bt

#endif /* GATT_IOCTL_HPP_ */
