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

#include "UUID.hpp"

using namespace tinyb_hci;

// BASE_UUID '00000000-0000-1000-8000-00805F9B34FB'
static uint8_t bt_base_uuid_be[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
                                     0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB };
UUID128 tinyb_hci::BT_BASE_UUID = UUID128( bt_base_uuid_be );

UUID128::UUID128(UUID128 const & base_uuid, UUID16 const & uuid16, int const uuid16_le_octet_index)
: UUID(Type::UUID128), value(merge_uint128(base_uuid.value, uuid16.value, uuid16_le_octet_index)) {}

UUID128::UUID128(UUID128 const & base_uuid, UUID32 const & uuid32, int const uuid32_le_octet_index)
: UUID(Type::UUID128), value(merge_uint128(base_uuid.value, uuid32.value, uuid32_le_octet_index)) {}

std::string UUID16::toString() const {
    char buffer[4+1];
    int count = snprintf(buffer, sizeof(buffer), "%.4X", value);
    if( 4 != count ) {
        std::string msg("UUID string not of length 4 but ");
        msg.append(std::to_string(count));
        throw InternalError(msg);
    }
    return std::string(buffer);
}

std::string UUID16::toUUID128String(UUID128 const & base_uuid, int const le_octet_index) const
{
    UUID128 u128(base_uuid, *this, le_octet_index);
    return u128.toString();
}

std::string UUID32::toString() const {
    char buffer[8+1];
    int count = snprintf(buffer, sizeof(buffer), "%.8X", value);
    if( 8 != count ) {
        std::string msg("UUID string not of length 8 but ");
        msg.append(std::to_string(count));
        throw InternalError(msg);
    }
    return std::string(buffer);
}

std::string UUID32::toUUID128String(UUID128 const & base_uuid, int const le_octet_index) const
{
    UUID128 u128(base_uuid, *this, le_octet_index);
    return u128.toString();
}

std::string UUID128::toString() const {
    //               87654321-0000-1000-8000-00805F9B34FB
    //                   0      1    2    3      4    5
    // LE: low-mem - FB349B5F0800-0080-0010-0000-12345678 - high-mem
    //                  5    4      3    2    1      0
    //
    // BE: low-mem - 87654321-0000-1000-8000-00805F9B34FB - high-mem
    //                   0      1    2    3      4    5
    //
    char buffer[36+1];
    uint32_t part0, part4;
    uint16_t part1, part2, part3, part5;
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
    int count = snprintf(buffer, sizeof(buffer), "%.8X-%.4X-%.4X-%.4X-%.8X%.4X",
                            part0, part1, part2, part3, part4, part5);
    if( 36 != count ) {
        std::string msg("UUID string not of length 36 but ");
        msg.append(std::to_string(count));
        throw InternalError(msg, E_FILE_LINE);
    }
    return std::string(buffer);
}

