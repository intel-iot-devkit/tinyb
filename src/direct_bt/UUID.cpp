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

#include <dbt_debug.hpp>
#include "UUID.hpp"


using namespace direct_bt;

// BASE_UUID '00000000-0000-1000-8000-00805F9B34FB'
static uint8_t bt_base_uuid_be[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
                                     0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB };
uuid128_t direct_bt::BT_BASE_UUID( bt_base_uuid_be, 0, false );

uuid_t::TypeSize uuid_t::toTypeSize(const int size) {
    switch(size) {
        case TypeSize::UUID16_SZ: return TypeSize::UUID16_SZ;
        case TypeSize::UUID32_SZ: return TypeSize::UUID32_SZ;
        case TypeSize::UUID128_SZ: return TypeSize::UUID128_SZ;
    }
    throw IllegalArgumentException("Given size "+std::to_string(size)+", not matching uuid16_t, uuid32_t or uuid128_t", E_FILE_LINE);
}

std::shared_ptr<const uuid_t> uuid_t::create(TypeSize t, uint8_t const * const buffer, int const byte_offset, bool littleEndian) {
    if( TypeSize::UUID16_SZ == t ) {
        return std::shared_ptr<const uuid_t>(new uuid16_t(buffer, byte_offset, littleEndian));
    } else if( TypeSize::UUID32_SZ == t ) {
        return std::shared_ptr<const uuid_t>(new uuid32_t(buffer, byte_offset, littleEndian));
    } else if( TypeSize::UUID128_SZ == t ) {
        return std::shared_ptr<const uuid_t>(new uuid128_t(buffer, byte_offset, littleEndian));
    }
    throw IllegalArgumentException("Unknown Type "+std::to_string(static_cast<int>(t)), E_FILE_LINE);
}

uuid128_t uuid_t::toUUID128(uuid128_t const & base_uuid, int const uuid32_le_octet_index) const {
    switch(type) {
        case TypeSize::UUID16_SZ: return uuid128_t(*((uuid16_t*)this), base_uuid, uuid32_le_octet_index);
        case TypeSize::UUID32_SZ: return uuid128_t(*((uuid32_t*)this), base_uuid, uuid32_le_octet_index);
        case TypeSize::UUID128_SZ: return uuid128_t(*((uuid128_t*)this));
    }
    throw InternalError("Unknown Type "+std::to_string(static_cast<int>(type)), E_FILE_LINE);
}

std::string uuid_t::toUUID128String(uuid128_t const & base_uuid, int const le_octet_index) const {
    (void)base_uuid;
    (void)le_octet_index;
    return "";
}

uuid128_t::uuid128_t(uuid16_t const & uuid16, uuid128_t const & base_uuid, int const uuid16_le_octet_index)
: uuid_t(TypeSize::UUID128_SZ), value(merge_uint128(uuid16.value, base_uuid.value, uuid16_le_octet_index)) {}

uuid128_t::uuid128_t(uuid32_t const & uuid32, uuid128_t const & base_uuid, int const uuid32_le_octet_index)
: uuid_t(TypeSize::UUID128_SZ), value(merge_uint128(uuid32.value, base_uuid.value, uuid32_le_octet_index)) {}

std::string uuid16_t::toString() const {
    const int length = 4;
    std::string str;
    str.reserve(length+1); // including EOS for snprintf
    str.resize(length);

    const int count = snprintf(&str[0], str.capacity(), "%.4x", value);
    if( length != count ) {
        throw InternalError("UUID16 string not of length "+std::to_string(length)+" but "+std::to_string(count), E_FILE_LINE);
    }
    return str;
}

std::string uuid16_t::toUUID128String(uuid128_t const & base_uuid, int const le_octet_index) const
{
    uuid128_t u128(*this, base_uuid, le_octet_index);
    return u128.toString();
}

std::string uuid32_t::toString() const {
    const int length = 8;
    std::string str;
    str.reserve(length+1); // including EOS for snprintf
    str.resize(length);

    const int count = snprintf(&str[0], str.capacity(), "%.8x", value);
    if( length != count ) {
        throw InternalError("UUID32 string not of length "+std::to_string(length)+" but "+std::to_string(count), E_FILE_LINE);
    }
    return str;
}

std::string uuid32_t::toUUID128String(uuid128_t const & base_uuid, int const le_octet_index) const
{
    uuid128_t u128(*this, base_uuid, le_octet_index);
    return u128.toString();
}

std::string uuid128_t::toString() const {
    //               87654321-0000-1000-8000-00805F9B34FB
    //                   0      1    2    3      4    5
    // LE: low-mem - FB349B5F0800-0080-0010-0000-12345678 - high-mem
    //                  5    4      3    2    1      0
    //
    // BE: low-mem - 87654321-0000-1000-8000-00805F9B34FB - high-mem
    //                   0      1    2    3      4    5
    //
    const int length = 36;
    std::string str;
    str.reserve(length+1); // including EOS for snprintf
    str.resize(length);
    uint32_t part0, part4;
    uint16_t part1, part2, part3, part5;

    // snprintf uses host data type, in which values are stored,
    // hence no endian conversion
#if __BYTE_ORDER == __BIG_ENDIAN
    part0 = get_uint32(value.data,  0);
    part1 = get_uint16(value.data,  4);
    part2 = get_uint16(value.data,  6);
    part3 = get_uint16(value.data,  8);
    part4 = get_uint32(value.data, 10);
    part5 = get_uint16(value.data, 14);
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    part5 = get_uint16(value.data,  0);
    part4 = get_uint32(value.data,  2);
    part3 = get_uint16(value.data,  6);
    part2 = get_uint16(value.data,  8);
    part1 = get_uint16(value.data, 10);
    part0 = get_uint32(value.data, 12);
#else
#error "Unexpected __BYTE_ORDER"
#endif
    const int count = snprintf(&str[0], str.capacity(), "%.8x-%.4x-%.4x-%.4x-%.8x%.4x",
                                part0, part1, part2, part3, part4, part5);
    if( length != count ) {
        throw InternalError("UUID128 string not of length "+std::to_string(length)+" but "+std::to_string(count), E_FILE_LINE);
    }
    return str;
}

uuid128_t::uuid128_t(const std::string str)
: uuid_t(TypeSize::UUID128_SZ)
{
    uint32_t part0, part4;
    uint16_t part1, part2, part3, part5;

    if( 36 != str.length() ) {
        std::string msg("UUID128 string not of length 36 but ");
        msg.append(std::to_string(str.length()));
        msg.append(": "+str);
        throw IllegalArgumentException(msg, E_FILE_LINE);
    }
    if ( sscanf(str.c_str(), "%08x-%04hx-%04hx-%04hx-%08x%04hx",
                     &part0, &part1, &part2, &part3, &part4, &part5) != 6 )
    {
        std::string msg("UUID128 string not in format '00000000-0000-1000-8000-00805F9B34FB' but "+str);
        throw IllegalArgumentException(msg, E_FILE_LINE);
    }
    uint128_t value;

    // sscanf provided host data type, in which we store the values,
    // hence no endian conversion
#if __BYTE_ORDER == __BIG_ENDIAN
    put_uint32(value.data,  0, part0);
    put_uint16(value.data,  4, part1);
    put_uint16(value.data,  6, part2);
    put_uint16(value.data,  8, part3);
    put_uint32(value.data, 10, part4);
    put_uint16(value.data, 14, part5);
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    put_uint16(value.data,  0, part5);
    put_uint32(value.data,  2, part4);
    put_uint16(value.data,  6, part3);
    put_uint16(value.data,  8, part2);
    put_uint16(value.data, 10, part1);
    put_uint32(value.data, 12, part0);
#else
#error "Unexpected __BYTE_ORDER"
#endif
}

