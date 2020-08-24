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

#include <cstdint>
#include <cinttypes>

#include <algorithm>

// #define _USE_BACKTRACE_ 1

extern "C" {
    #if _USE_BACKTRACE_
        #include <execinfo.h>
    #endif
}

#include "direct_bt/BasicTypes.hpp"

using namespace direct_bt;

static const int64_t NanoPerMilli = 1000000L;
static const int64_t MilliPerOne = 1000L;

/**
 * See <http://man7.org/linux/man-pages/man2/clock_gettime.2.html>
 * <p>
 * Regarding avoiding kernel via VDSO,
 * see <http://man7.org/linux/man-pages/man7/vdso.7.html>,
 * clock_gettime seems to be well supported at least on kernel >= 4.4.
 * Only bfin and sh are missing, while ia64 seems to be complicated.
 */
int64_t direct_bt::getCurrentMilliseconds() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * MilliPerOne + t.tv_nsec / NanoPerMilli;
}

const char* direct_bt::RuntimeException::what() const noexcept {
#if    _USE_BACKTRACE_
    // std::string out(std::runtime_error::what());
    std::string out(msg);
    void *buffers[10];
    size_t nptrs = backtrace(buffers, 10);
    char **symbols = backtrace_symbols(buffers, nptrs);
    if( NULL != symbols ) {
        out.append("\nBacktrace:\n");
        for(size_t i=0; i<nptrs; i++) {
            out.append(symbols[i]).append("\n");
        }
        free(symbols);
    }
    return out.c_str();
#else
    // return std::runtime_error::what();
    return msg.c_str();
#endif
}

std::string direct_bt::get_string(const uint8_t *buffer, int const buffer_len, int const max_len) {
    const int cstr_len = std::min(buffer_len, max_len);
    char cstr[max_len+1]; // EOS
    memcpy(cstr, buffer, cstr_len);
    cstr[cstr_len] = 0; // EOS
    return std::string(cstr);
}

uint128_t direct_bt::merge_uint128(uint16_t const uuid16, uint128_t const & base_uuid, int const uuid16_le_octet_index)
{
    if( 0 > uuid16_le_octet_index || uuid16_le_octet_index > 14 ) {
        std::string msg("uuid16_le_octet_index ");
        msg.append(std::to_string(uuid16_le_octet_index));
        msg.append(", not within [0..14]");
        throw IllegalArgumentException(msg, E_FILE_LINE);
    }
    uint128_t dest = base_uuid;

    // base_uuid: 00000000-0000-1000-8000-00805F9B34FB
    //    uuid16: DCBA
    // uuid16_le_octet_index: 12
    //    result: 0000DCBA-0000-1000-8000-00805F9B34FB
    //
    // LE: low-mem - FB349B5F8000-0080-0010-0000-ABCD0000 - high-mem
    //                                           ^ index 12
    // LE: uuid16 -> value.data[12+13]
    //
    // BE: low-mem - 0000DCBA-0000-1000-8000-00805F9B34FB - high-mem
    //                   ^ index 2
    // BE: uuid16 -> value.data[2+3]
    //
#if __BYTE_ORDER == __BIG_ENDIAN
    int offset = 15 - 1 - uuid16_le_octet_index;
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    int offset = uuid16_le_octet_index;
#else
#error "Unexpected __BYTE_ORDER"
#endif
    uint16_t * destu16 = (uint16_t*)(dest.data + offset);
    *destu16 += uuid16;
    return dest;
}

uint128_t direct_bt::merge_uint128(uint32_t const uuid32, uint128_t const & base_uuid, int const uuid32_le_octet_index)
{
    if( 0 > uuid32_le_octet_index || uuid32_le_octet_index > 12 ) {
        std::string msg("uuid32_le_octet_index ");
        msg.append(std::to_string(uuid32_le_octet_index));
        msg.append(", not within [0..12]");
        throw IllegalArgumentException(msg, E_FILE_LINE);
    }
    uint128_t dest = base_uuid;

    // base_uuid: 00000000-0000-1000-8000-00805F9B34FB
    //    uuid32: 87654321
    // uuid32_le_octet_index: 12
    //    result: 87654321-0000-1000-8000-00805F9B34FB
    //
    // LE: low-mem - FB349B5F8000-0080-0010-0000-12345678 - high-mem
    //                                           ^ index 12
    // LE: uuid32 -> value.data[12..15]
    //
    // BE: low-mem - 87654321-0000-1000-8000-00805F9B34FB - high-mem
    //               ^ index 0
    // BE: uuid32 -> value.data[0..3]
    //
#if __BYTE_ORDER == __BIG_ENDIAN
    int offset = 15 - 3 - uuid32_le_octet_index;
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    int offset = uuid32_le_octet_index;
#else
#error "Unexpected __BYTE_ORDER"
#endif
    uint32_t * destu32 = (uint32_t*)(dest.data + offset);
    *destu32 += uuid32;
    return dest;
}

std::string direct_bt::uint8HexString(const uint8_t v, const bool leading0X) {
    const int length = leading0X ? 4 : 2; // ( '0x00' | '00' )
    std::string str;
    str.reserve(length+1); // including EOS for snprintf
    str.resize(length);

    const int count = snprintf(&str[0], str.capacity(), ( leading0X ? "0x%.2X" : "%.2X" ), v);
    if( length != count ) {
        throw InternalError("uint8_t string not of length "+std::to_string(length)+" but "+std::to_string(count), E_FILE_LINE);
    }
    return str;
}

std::string direct_bt::uint16HexString(const uint16_t v, const bool leading0X) {
    const int length = leading0X ? 6 : 4; // ( '0x0000' | '0000' )
    std::string str;
    str.reserve(length+1); // including EOS for snprintf
    str.resize(length);

    const int count = snprintf(&str[0], str.capacity(), ( leading0X ? "0x%.4X" : "%.4X" ), v);
    if( length != count ) {
        throw InternalError("uint16_t string not of length "+std::to_string(length)+" but "+std::to_string(count), E_FILE_LINE);
    }
    return str;
}

std::string direct_bt::uint32HexString(const uint32_t v, const bool leading0X) {
    const int length = leading0X ? 10 : 8; // ( '0x00000000' | '00000000' )
    std::string str;
    str.reserve(length+1); // including EOS for snprintf
    str.resize(length);

    const int count = snprintf(&str[0], str.capacity(), ( leading0X ? "0x%.8X" : "%.8X" ), v);
    if( length != count ) {
        throw InternalError("uint32_t string not of length "+std::to_string(length)+" but "+std::to_string(count), E_FILE_LINE);
    }
    return str;
}

std::string direct_bt::uint64HexString(const uint64_t v, const bool leading0X) {
    const int length = leading0X ? 18 : 16; // ( '0x0000000000000000' | '0000000000000000' )
    std::string str;
    str.reserve(length+1); // including EOS for snprintf
    str.resize(length);

    const int count = snprintf(&str[0], str.capacity(), ( leading0X ? "0x%.16" PRIX64 : "%.16" PRIX64 ), v);
    if( length != count ) {
        throw InternalError("uint64_t string not of length "+std::to_string(length)+" but "+std::to_string(count), E_FILE_LINE);
    }
    return str;
}

static const char* HEX_ARRAY = "0123456789ABCDEF";

std::string direct_bt::bytesHexString(const uint8_t * bytes, const int offset, const int length, const bool lsbFirst, const bool leading0X) {
    std::string str;

    if( nullptr == bytes ) {
        return "null";
    }
    if( 0 == length ) {
        return "nil";
    }
    if( leading0X ) {
        str.reserve(2 + length * 2 +1);
        str.push_back('0');
        str.push_back('x');
    } else {
        str.reserve(length * 2 +1);
    }
    if( lsbFirst ) {
        // LSB left -> MSB right
        for (int j = 0; j < length; j++) {
            const int v = bytes[offset+j] & 0xFF;
            str.push_back(HEX_ARRAY[v >> 4]);
            str.push_back(HEX_ARRAY[v & 0x0F]);
        }
    } else {
        // MSB left -> LSB right
        for (int j = length-1; j >= 0; j--) {
            const int v = bytes[offset+j] & 0xFF;
            str.push_back(HEX_ARRAY[v >> 4]);
            str.push_back(HEX_ARRAY[v & 0x0F]);
        }
    }
    return str;
}

std::string direct_bt::int32SeparatedString(const int32_t v, const char separator) {
    // INT32_MIN:    -2147483648 int32_t 11 chars
    // INT32_MIN: -2,147,483,648 int32_t 14 chars
    // INT32_MAX:     2147483647 int32_t 10 chars
    // INT32_MAX:  2,147,483,647 int32_t 13 chars
    char src[16]; // aligned 4 byte
    char dst[16]; // aligned 4 byte, also +1 for erroneous trailing comma
    char *p_src = src;
    char *p_dst = dst;

    int num_len = snprintf(src, sizeof(src), "%" PRId32, v);

    if (*p_src == '-') {
        *p_dst++ = *p_src++;
        num_len--;
    }

    for ( int commas = 2 - num_len % 3; 0 != *p_src; commas = (commas + 1) % 3 )
    {
        *p_dst++ = *p_src++;
        if ( 1 == commas ) {
            *p_dst++ = separator;
        }
    }
    *--p_dst = 0; // place EOS on erroneous trailing comma

    return std::string(dst, p_dst - dst);
}

std::string direct_bt::uint32SeparatedString(const uint32_t v, const char separator) {
    // UINT32_MAX:    4294967295 uint32_t 10 chars
    // UINT32_MAX: 4,294,967,295 uint32_t 13 chars
    char src[16]; // aligned 4 byte
    char dst[16]; // aligned 4 byte, also +1 for erroneous trailing comma
    char *p_src = src;
    char *p_dst = dst;

    int num_len = snprintf(src, sizeof(src), "%" PRIu32, v);

    for ( int commas = 2 - num_len % 3; 0 != *p_src; commas = (commas + 1) % 3 )
    {
        *p_dst++ = *p_src++;
        if ( 1 == commas ) {
            *p_dst++ = separator;
        }
    }
    *--p_dst = 0; // place EOS on erroneous trailing comma

    return std::string(dst, p_dst - dst);
}

std::string direct_bt::uint64SeparatedString(const uint64_t v, const char separator) {
    // UINT64_MAX:       18446744073709551615 uint64_t 20 chars
    // UINT64_MAX: 18,446,744,073,709,551,615 uint64_t 26 chars
    char src[28]; // aligned 4 byte
    char dst[28]; // aligned 4 byte, also +1 for erroneous trailing comma
    char *p_src = src;
    char *p_dst = dst;

    int num_len = snprintf(src, sizeof(src), "%" PRIu64, v);

    for ( int commas = 2 - num_len % 3; 0 != *p_src; commas = (commas + 1) % 3 )
    {
        *p_dst++ = *p_src++;
        if ( 1 == commas ) {
            *p_dst++ = separator;
        }
    }
    *--p_dst = 0; // place EOS on erroneous trailing comma

    return std::string(dst, p_dst - dst);
}

std::string direct_bt::aptrHexString(const void * v, const bool leading0X) {
    return uint64HexString((uint64_t)v, leading0X);
}

void direct_bt::trimInPlace(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

std::string direct_bt::trimCopy(const std::string &_s) {
    std::string s(_s);
    trimInPlace(s);
    return s;
}

