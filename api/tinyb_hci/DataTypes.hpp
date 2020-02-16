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

#ifndef DATATYPES_HPP_
#define DATATYPES_HPP_

#pragma once
#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>

#include "HCIUtil.hpp"
#include "UUID.hpp"

namespace tinyb_hci {

enum AD_Type_Const : uint8_t {
    AD_FLAGS_LIMITED_MODE_BIT = 0x01,
    AD_FLAGS_GENERAL_MODE_BIT = 0x02
};

enum AD_Types : uint8_t {
    FLAGS                   = 0x01,
    /** 16-bit UUID, more available */
    UUID16_SOME             = 0x02,
    /** 16-bit UUID, all listed */
    UUID16_ALL              = 0x03,
    /** 32-bit UUID, more available */
    UUID32_SOME             = 0x04,
    /** 32-bit UUID, all listed */
    UUID32_ALL              = 0x05,
    /** 128-bit UUID, more available */
    UUID128_SOME            = 0x06,
    /** 128-bit UUID, all listed */
    UUID128_ALL             = 0x07,

    /** shortened local name */
    NAME_SHORT              = 0x08,
    /** complete local name */
    NAME_COMPLETE           = 0x09,
    /** transmit power level */
    TX_POWER                = 0x0A,
    /** device ID */
    DEVICE_ID               = 0x10,

    /** Manufacturer id code and specific opaque data */
    MANUFACTURE_SPECIFIC    = 0xFF
};


// *************************************************
// *************************************************
// *************************************************


/**
 * A packed 48 bit EUI-48 identifier, formerly known as MAC-48
 * or simply network device MAC address (Media Access Control address).
 */
struct __attribute__((packed)) EUI48 {
    uint8_t b[6];

    EUI48() { bzero(b, sizeof(EUI48)); }
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


// *************************************************
// *************************************************
// *************************************************


class ManufactureSpecificData
{
public:
    uint16_t const company;
    std::string const companyName;
    int const data_len;
    std::shared_ptr<uint8_t> const data;

    ManufactureSpecificData()
    : company(0), companyName(), data_len(0), data(nullptr) {}

    ManufactureSpecificData(uint16_t const company, uint8_t const * const data, int const data_len);

    std::string getCompanyString() const;
    std::string toString() const;
};

// *************************************************
// *************************************************
// *************************************************

/**
 * Collection of 'Advertising Data' (AD)
 * or 'Extended Inquiry Response' (EIR) information.
 */
class EInfoReport
{
public:
    enum class Source : int {
        /** not available */
        NA,
        /* Advertising Data (AD) */
        AD,
        /** Extended Inquiry Response (EIR) */
        EIR
    };
    enum class Element : uint32_t {
        EVT_TYPE    = (1 << 0),
        BDADDR_TYPE = (1 << 1),
        BDADDR      = (1 << 2),
        NAME        = (1 << 3),
        NAME_SHORT  = (1 << 4),
        RSSI        = (1 << 5),
        TX_POWER    = (1 << 6),
        MANUF_DATA  = (1 << 7)
    };

private:
    Source source = Source::NA;
    uint64_t timestamp = 0;
    uint32_t data_set = 0;

    uint8_t evt_type = 0;
    uint8_t mac_type = 0;
    EUI48 mac;

    std::string name;
    std::string name_short;
    uint8_t rssi = 0;
    int8_t tx_power = 0;
    std::shared_ptr<ManufactureSpecificData> msd = nullptr;
    std::vector<std::shared_ptr<UUID>> services;

    void set(Element bit) { data_set |= static_cast<uint32_t>(bit); }
    void setSource(Source s) { source = s; }
    void setTimestamp(uint64_t ts) { timestamp = ts; }
    void setEvtType(uint8_t et) { evt_type = et; set(Element::EVT_TYPE); }
    void setAddressType(uint8_t at) { mac_type = at; set(Element::BDADDR_TYPE); }
    void setAddress(EUI48 const &a) { mac = a; set(Element::BDADDR); }
    void setName(const uint8_t *buffer, int buffer_len);
    void setShortName(const uint8_t *buffer, int buffer_len);
    void setRSSI(uint8_t v) { rssi = v; set(Element::RSSI); }
    void setTxPower(int8_t v) { tx_power = v; set(Element::TX_POWER); }
    void setManufactureSpecificData(uint16_t const company, uint8_t const * const data, int const data_len) {
        msd = std::shared_ptr<ManufactureSpecificData>(new ManufactureSpecificData(company, data, data_len));
        set(Element::MANUF_DATA);
    }

    void addService(std::shared_ptr<UUID> const &uuid);


    int next_data_elem(uint8_t *eir_elem_len, uint8_t *eir_elem_type, uint8_t const **eir_elem_data,
                       uint8_t const * data, int offset, int const size);

public:
    /**
     * Reads a complete Advertising Data (AD) Report
     * and returns the number of AD reports in form of a sharable list of EInfoReport;
     * <p>
     * See Bluetooth Core Specification V5.2 [Vol. 4, Part E, 7.7.65.2, p 2382]
     * <p>
     * https://www.bluetooth.com/specifications/archived-specifications/
     * </p>
     */
    static std::vector<std::shared_ptr<EInfoReport>> read_ad_reports(uint8_t const * data, uint8_t const data_length);

    /**
     * Reads the Extended Inquiry Response (EIR) or Advertising Data (AD) segments
     * and returns the number of parsed data segments;
     * <p>
     * AD as well as EIR information is passed in little endian order
     * in the same fashion data block:
     * <pre>
     * a -> {
     *             uint8_t len
     *             uint8_t type
     *             uint8_t data[len-1];
     *         }
     * b -> next block = a + 1 + len;
     * </pre>
     * </p>
     * <p>
     * See Bluetooth Core Specification V5.2 [Vol. 3, Part C, 11, p 1392]
     * and Bluetooth Core Specification Supplement V9, Part A: 1, p 9 + 2 Examples, p25..
     * and Assigned Numbers <https://www.bluetooth.com/specifications/assigned-numbers/>
     * </p>
     * <p>
     * https://www.bluetooth.com/specifications/archived-specifications/
     * </p>
     */
    int read_data(uint8_t const * data, uint8_t const data_length);

    Source getSource() const { return source; }
    uint64_t getTimestamp() const { return timestamp; }
    bool isSet(Element bit) const { return 0 != (data_set & static_cast<uint32_t>(bit)); }

    uint8_t getEvtType() const { return evt_type; }
    uint8_t getAddressType() const { return mac_type; }
    EUI48 const & getAddress() const { return mac; }
    std::string const & getName() const { return name; }
    std::string const & getShortName() const { return name_short; }
    uint8_t getRSSI() const { return rssi; }
    uint8_t getTxPower() const { return tx_power; }

    std::shared_ptr<ManufactureSpecificData> getManufactureSpecificData() const { return msd; }
    std::vector<std::shared_ptr<UUID>> getServices() const { return services; }

    std::string getSourceString() const;
    std::string getAddressString() const { return mac.toString(); }
    std::string dataSetToString() const;
    std::string toString() const;
};

// *************************************************
// *************************************************
// *************************************************

} // namespace tinyb_hci

#endif /* DATATYPES_HPP_ */
