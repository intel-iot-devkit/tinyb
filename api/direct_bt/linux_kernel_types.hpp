/*
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
 *
 */

#ifndef LINUX_KERNEL_TYPES_HPP_
#define LINUX_KERNEL_TYPES_HPP_

#include "BTAddress.hpp"

typedef uint8_t  __u8;
typedef int8_t   __s8;
typedef uint16_t __u16;
typedef uint16_t __le16;
typedef uint16_t __be16;
typedef uint32_t __u32;
typedef uint32_t __le32;
typedef uint32_t __be32;
typedef uint64_t __u64;
typedef uint64_t __le64;
typedef uint64_t __be64;
typedef direct_bt::EUI48 bdaddr_t;
#define __packed __attribute__ ((packed))

#endif /* LINUX_KERNEL_TYPES_HPP_ */
