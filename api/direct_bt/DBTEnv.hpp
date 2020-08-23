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

#ifndef DBT_ENV_HPP_
#define DBT_ENV_HPP_

#include <cstdint>
#include <cinttypes>
#include <cstring>
#include <string>
#include <cstdio>

extern "C" {
    #include <errno.h>
}

#include "BasicTypes.hpp"

namespace direct_bt {

    class DBTEnv {
        public:
            /**
             * Module startup time t0 in monotonic time in milliseconds.
             */
            static const uint64_t startupTimeMilliseconds;

            /**
             * Returns current elapsed monotonic time in milliseconds since module startup, see {@link #startupTimeMilliseconds}.
             */
            static uint64_t getElapsedMillisecond() {
                return getCurrentMilliseconds() - startupTimeMilliseconds;
            }
    };

} // namespace direct_bt

#endif /* DBT_ENV_HPP_ */

