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

#include "HCIUtil.hpp"

// #define _USE_BACKTRACE_ 1

#if _USE_BACKTRACE_
extern "C" {
	#include <execinfo.h>
}
#endif

using namespace tinyb_hci;

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
int64_t tinyb_hci::getCurrentMilliseconds() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * MilliPerOne + t.tv_nsec / NanoPerMilli;
}

const char* RuntimeException::what() const noexcept {
#if	_USE_BACKTRACE_
	std::string out(msg);
	void *buffers[10];
	size_t nptrs = backtrace(buffers, 10);
	char **symbols = backtrace_symbols(buffers, nptrs);
	if( NULL != symbols ) {
		out.append("\nBacktrace:\n");
		for(int i=0; i<nptrs; i++) {
			out.append(symbols[i]).append("\n");
		}
		free(symbols);
	}
	return out.c_str();
#else
	return msg.c_str();
#endif
}

uint128_t tinyb_hci::merge_uint128(uint128_t const & base_uuid, uint16_t const uuid16, int const uuid16_le_octet_index)
{
    if( 0 > uuid16_le_octet_index || uuid16_le_octet_index > 14 ) {
        std::string msg("uuid16_le_octet_index ");
        msg.append(std::to_string(uuid16_le_octet_index));
        msg.append(", not within [0..14]");
        throw IllegalArgumentException(msg);
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

uint128_t tinyb_hci::merge_uint128(uint128_t const & base_uuid, uint32_t const uuid32, int const uuid32_le_octet_index)
{
    if( 0 > uuid32_le_octet_index || uuid32_le_octet_index > 12 ) {
        std::string msg("uuid32_le_octet_index ");
        msg.append(std::to_string(uuid32_le_octet_index));
        msg.append(", not within [0..12]");
        throw IllegalArgumentException(msg);
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

