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

#include "DataTypes.hpp"

extern "C" {
    // bt_compidtostr
    #include <bluetooth/bluetooth.h>
}

#define VERBOSE_ON 1

#ifdef VERBOSE_ON
    #define DBG_PRINT(...) fprintf(stderr, __VA_ARGS__); fflush(stderr)
#else
    #define DBG_PRINT(...)
#endif


using namespace tinyb_hci;

static inline const int8_t * const_uint8_to_const_int8_ptr(const uint8_t* p) {
    return static_cast<const int8_t *>( static_cast<void *>( const_cast<uint8_t*>( p ) ) );
}

std::string EUI48::toString() const {
    char cstr[17+1];

    const int count = sprintf(cstr, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
        b[5], b[4], b[3], b[2], b[1], b[0]);

    if( 17 != count ) {
        std::string msg("EUI48 string not of length 17 but ");
        msg.append(std::to_string(count));
        throw InternalError(msg, E_FILE_LINE);
    }
    return std::string(cstr);
}

EUI48::EUI48(const std::string str) {
    if( 17 != str.length() ) {
        std::string msg("EUI48 string not of length 17 but ");
        msg.append(std::to_string(str.length()));
        msg.append(": "+str);
        throw IllegalArgumentException(msg, E_FILE_LINE);
    }
    if ( sscanf(str.c_str(), "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                     &b[5], &b[4], &b[3], &b[2], &b[1], &b[0]) != 6 )
    {
        std::string msg("EUI48 string not in format '00:00:00:00:00:00' but "+str);
        throw IllegalArgumentException(msg, E_FILE_LINE);
    }

    // sscanf provided host data type, in which we store the values,
    // hence no endian conversion
}

const EUI48 EUI48_ANY_DEVICE; // default ctor is zero bytes!

// *************************************************
// *************************************************
// *************************************************

ManufactureSpecificData::ManufactureSpecificData(uint16_t const company, uint8_t const * const data, int const data_len)
: company(company), companyName(std::string(bt_compidtostr(company))), data_len(data_len), data(new uint8_t[data_len]) {
    memcpy(this->data.get(), data, data_len);
}

std::string ManufactureSpecificData::toString() const {
  std::string out("MSD[");
  out.append(std::to_string(company)+" "+companyName);
  out.append(", data "+std::to_string(data_len)+" bytes]");
  return out;
}

// *************************************************
// *************************************************
// *************************************************

std::string EInfoReport::getSourceString() const {
    switch (source) {
        case Source::NA: return "N/A";
        case Source::AD: return "AD";
        case Source::EIR: return "EIR";
    }
    return "N/A";
}

void EInfoReport::setName(const uint8_t *buffer, int buffer_len) {
    name = get_string(buffer, buffer_len, 30);
    set(Element::NAME);
}

void EInfoReport::setShortName(const uint8_t *buffer, int buffer_len) {
    name_short = get_string(buffer, buffer_len, 30);
    set(Element::NAME_SHORT);
}

void EInfoReport::addService(std::shared_ptr<UUID> const &uuid)
{
    auto begin = services.begin();
    auto it = std::find_if(begin, services.end(), [&](std::shared_ptr<UUID> const& p) {
        return *p == *uuid;
    });
    if ( it == std::end(services) ) {
        services.push_back(uuid);
    }
}

std::string EInfoReport::dataSetToString(const uint32_t data_set) {
    std::string out("[");
    if( isSet(data_set, Element::EVT_TYPE) ) {
        out.append("EVT_TYPE, ");
    }
    if( isSet(data_set, Element::BDADDR) ) {
        out.append("BDADDR, ");
    }
    if( isSet(data_set, Element::NAME) ) {
        out.append("NAME, ");
    }
    if( isSet(data_set, Element::NAME_SHORT) ) {
        out.append("NAME_SHORT, ");
    }
    if( isSet(data_set, Element::RSSI) ) {
        out.append("RSSI, ");
    }
    if( isSet(data_set, Element::TX_POWER) ) {
        out.append("TX_POWER, ");
    }
    if( isSet(data_set, Element::MANUF_DATA) ) {
        out.append("MANUF_DATA, ");
    }
    out.append("]");
    return out;
}
std::string EInfoReport::dataSetToString() const {
    return std::string("DataSet"+dataSetToString(data_set));
}
std::string EInfoReport::toString() const {
    std::string msdstr = nullptr != msd ? msd->toString() : "MSD[null]";
    std::string out("EInfoReport::"+getSourceString()+"["+getAddressString()+", "+name+"/"+name_short+", "+dataSetToString()+
                    ", evt-type "+std::to_string(evt_type)+", rssi "+std::to_string(rssi)+
                    ", tx-power "+std::to_string(tx_power)+", "+msdstr+"]");
    if(services.size() > 0 ) {
        out.append("\n");
        for(auto it = services.begin(); it != services.end(); it++) {
            std::shared_ptr<UUID> p = *it;
            out.append("  ").append(p->toUUID128String()).append(", ").append(std::to_string(static_cast<int>(p->type))).append(" bytes\n");
        }
    }
    return out;
}

// *************************************************
// *************************************************
// *************************************************

int EInfoReport::next_data_elem(uint8_t *eir_elem_len, uint8_t *eir_elem_type, uint8_t const **eir_elem_data,
                               uint8_t const * data, int offset, int const size)
{
    if (offset < size) {
        uint8_t len = data[offset]; // covers: type + data, less len field itself

        if (len == 0) {
            return 0; // end of significant part
        }

        if (len + offset > size) {
            return -ENOENT;
        }

        *eir_elem_type = data[offset + 1];
        *eir_elem_data = data + offset + 2; // net data ptr
        *eir_elem_len = len - 1; // less type -> net data length

        return offset + 1 + len; // next ad_struct offset: + len + type + data
    }
    return -ENOENT;
}

int EInfoReport::read_data(uint8_t const * data, uint8_t const data_length) {
    int count = 0;
    int offset = 0;
    uint8_t elem_len, elem_type;
    uint8_t const *elem_data;

    while( 0 < ( offset = next_data_elem( &elem_len, &elem_type, &elem_data, data, offset, data_length ) ) )
    {
        DBG_PRINT("%s-Element[%d] @ [%d/%d]: type 0x%.2X with %d bytes net\n",
                getSourceString().c_str(), count, offset, data_length, elem_type, elem_len);
        count++;

        // Guaranteed: eir_elem_len >= 0!
        switch ( elem_type ) {
            case GAP_T::FLAGS:
                // FIXME
                break;
            case GAP_T::UUID16_INCOMPLETE:
            case GAP_T::UUID16_COMPLETE:
                for(int j=0; j<elem_len/2; j++) {
                    const std::shared_ptr<UUID> uuid(new UUID16(elem_data, j*2, true));
                    addService(std::move(uuid));
                }
                break;
            case GAP_T::UUID32_INCOMPLETE:
            case GAP_T::UUID32_COMPLETE:
                for(int j=0; j<elem_len/4; j++) {
                    const std::shared_ptr<UUID> uuid(new UUID32(elem_data, j*4, true));
                    addService(std::move(uuid));
                }
                break;
            case GAP_T::UUID128_INCOMPLETE:
            case GAP_T::UUID128_COMPLETE:
                for(int j=0; j<elem_len/16; j++) {
                    const std::shared_ptr<UUID> uuid(new UUID128(elem_data, j*16, true));
                    addService(std::move(uuid));
                }
                break;
            case GAP_T::NAME_LOCAL_SHORT:
            case GAP_T::NAME_LOCAL_COMPLETE: {
                if( GAP_T::NAME_LOCAL_COMPLETE == elem_type ) {
                    setName(elem_data, elem_len);
                } else {
                    setShortName(elem_data, elem_len);
                }
            } break;
            case GAP_T::TX_POWER_LEVEL:
                setTxPower(*const_uint8_to_const_int8_ptr(elem_data));
                break;

            case GAP_T::SSP_CLASS_OF_DEVICE:
            case GAP_T::DEVICE_ID:
            case GAP_T::SOLICIT_UUID16:
            case GAP_T::SOLICIT_UUID128:
            case GAP_T::SVC_DATA_UUID16:
            case GAP_T::PUB_TRGT_ADDR:
            case GAP_T::RND_TRGT_ADDR:
            case GAP_T::GAP_APPEARANCE:
            case GAP_T::SOLICIT_UUID32:
            case GAP_T::SVC_DATA_UUID32:
            case GAP_T::SVC_DATA_UUID128:
                break;

            case GAP_T::MANUFACTURE_SPECIFIC: {
                uint16_t company = get_uint16(elem_data, 0, true /* littleEndian */);
                setManufactureSpecificData(company, elem_data+2, elem_len-2);
            } break;

            default: {
                // FIXME: Use a data blob!!!!
                fprintf(stderr, "%s-Element @ [%d/%d]: Warning: Unhandled type 0x%.2X with %d bytes net\n",
                        getSourceString().c_str(), offset, data_length, elem_type, elem_len);
            } break;
        }
    }
    return count;
}

std::vector<std::shared_ptr<EInfoReport>> EInfoReport::read_ad_reports(uint8_t const * data, uint8_t const data_length) {
    int const num_reports = (int) data[0];
    std::vector<std::shared_ptr<EInfoReport>> ad_reports;

    if( 0 >= num_reports || num_reports > 0x19 ) {
        DBG_PRINT("AD-Reports: Invalid reports count: %d\n", num_reports);
        return ad_reports;
    }
    uint8_t const *limes = data + data_length;
    uint8_t const *i_octets = data + 1;
    uint8_t ad_data_len[0x19];
    const int segment_count = 6;
    int read_segments = 0;
    int i;

    for(i = 0; i < num_reports && i_octets < limes; i++) {
        ad_reports.push_back(std::shared_ptr<EInfoReport>(new EInfoReport()));
        ad_reports[i]->setSource(Source::AD);
        ad_reports[i]->setTimestamp(getCurrentMilliseconds());
        ad_reports[i]->setEvtType(*i_octets++);
        read_segments++;
    }
    for(i = 0; i < num_reports && i_octets < limes; i++) {
        ad_reports[i]->setAddressType(*i_octets++);
        read_segments++;
    }
    for(i = 0; i < num_reports && i_octets + 5 < limes; i++) {
        ad_reports[i]->setAddress( *((EUI48 const *)i_octets) );
        i_octets += 6;
        read_segments++;
    }
    for(i = 0; i < num_reports && i_octets < limes; i++) {
        ad_data_len[i] = *i_octets++;
        read_segments++;
    }
    for(i = 0; i < num_reports && i_octets + ad_data_len[i] < limes; i++) {
        ad_reports[i]->read_data(i_octets, ad_data_len[i]);
        i_octets += ad_data_len[i];
        read_segments++;
    }
    for(i = 0; i < num_reports && i_octets < limes; i++) {
        ad_reports[i]->setRSSI(*const_uint8_to_const_int8_ptr(i_octets));
        i_octets++;
        read_segments++;
    }
    const int bytes_left = limes - i_octets;

    if( segment_count != read_segments ) {
        fprintf(stderr, "AD-Reports: Warning: Incomplete %d reports within %d bytes: Segment read %d < %d, data-ptr %d bytes to limes\n",
                num_reports, data_length, read_segments, segment_count, bytes_left);
    } else {
        DBG_PRINT("AD-Reports: Completed %d reports within %d bytes: Segment read %d == %d, data-ptr %d bytes to limes\n",
                num_reports, data_length, read_segments, segment_count, bytes_left);
    }
    return ad_reports;
}

// *************************************************
// *************************************************
// *************************************************

