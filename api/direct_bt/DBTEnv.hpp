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
        private:
            DBTEnv();

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

            /**
             * Returns the value of the environment's variable 'name'.
             * <p>
             * If there is no environment variable 'name',
             * implementation will try the potentially Java JVM imported 'jvm_name'.
             * </p>
             * <p>
             * Note that all Java JVM system properties are passed to the environment
             * and all property names have replaced dots '.' to underscore '_'
             * and 'jvm_' prepended.
             * </p>
             */
            static std::string getProperty(const std::string & name);

            /**
             * Returns the value of the environment's variable 'name',
             * or the 'default_value' if the environment variable's value is null.
             * <p>
             * Implementation uses getProperty(const char *name).
             * </p>
             */
            static std::string getProperty(const std::string & name, const std::string & default_value);

            /**
             * Returns the boolean value of the environment's variable 'name',
             * or the 'default_value' if the environment variable's value is null.
             * <p>
             * If the environment variable is set (value != null),
             * true is determined if the value equals 'true'.
             * </p>
             * <p>
             * Implementation uses getProperty(const char *name).
             * </p>
             */
            static bool getBooleanProperty(const std::string & name, const bool default_value);

            static DBTEnv& get() {
                /**
                 * Thread safe starting with C++11 6.7:
                 *
                 * If control enters the declaration concurrently while the variable is being initialized,
                 * the concurrent execution shall wait for completion of the initialization.
                 *
                 * (Magic Statics)
                 *
                 * Avoiding non-working double checked locking.
                 */
                static DBTEnv e;
                return e;
            }

            /**
             * Debug logging enabled or disabled.
             * <p>
             * Environment variable 'direct_bt_debug', boolean, default 'false'.
             * </p>
             */
            const bool DEBUG;

            /**
             * Verbose info logging enabled or disabled.
             * <p>
             * Environment variable 'direct_bt_verbose', boolean, default 'false'.
             * </p>
             * <p>
             * VERBOSE is also enabled if DEBUG is enabled!
             * </p>
             */
            const bool VERBOSE;
    };

} // namespace direct_bt

#endif /* DBT_ENV_HPP_ */

