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

#ifndef IEEE11073_TYPES_HPP_
#define IEEE11073_TYPES_HPP_

#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>
#include <cmath>

extern "C" {
    #include <endian.h>
    #include <byteswap.h>
}

/**
 * IEEE11073 Data Types
 * <p>
 * https://en.wikipedia.org/wiki/ISO/IEEE_11073_Personal_Health_Data_(PHD)_Standards
 * https://person.hst.aau.dk/ska/MIE2008/ParalleSessions/PresentationsForDownloads/Mon-1530/Sta-30_Clarke.pdf
 * </p>
 * <p>
 * https://en.wikipedia.org/wiki/ISO/IEEE_11073
 * http://www.11073.org/
 * </p>
 */
namespace ieee11073 {

    #define E_FILE_LINE __FILE__,__LINE__

    class RuntimeException : public std::exception {
      protected:
        RuntimeException(std::string const type, std::string const m, const char* file, int line) noexcept
        : msg(std::string(type).append(" @ ").append(file).append(":").append(std::to_string(line)).append(": ").append(m)) { }

      public:
        const std::string msg;

        RuntimeException(std::string const m, const char* file, int line) noexcept
        : RuntimeException("RuntimeException", m, file, line) {}

        virtual ~RuntimeException() noexcept { }

        virtual const char* what() const noexcept override;
    };

    /** date / timestamp format */
    class AbsoluteTime {
        public:
            int16_t year=0;
            int8_t month=0;
            int8_t day=0;
            int8_t hour=0;
            int8_t minute=0;
            int8_t second=0;
            int8_t second_fractions=0;

            /** Default ctor w/ zero value */
            AbsoluteTime() {}

            /** Reads up to 8 bytes, as available */
            AbsoluteTime(const uint8_t * data_le, const int size);

            AbsoluteTime(const AbsoluteTime &o) noexcept = default;
            AbsoluteTime(AbsoluteTime &&o) noexcept = default;
            AbsoluteTime& operator=(const AbsoluteTime &o) noexcept = default;
            AbsoluteTime& operator=(AbsoluteTime &&o) noexcept = default;

            std::string toString() const;
    };

    /**
     * IEEE11073 Float Data Types
     */
    class FloatTypes {
        public:
            enum ReservedFloatValues : int32_t {
                MDER_POSITIVE_INFINITY  = 0x007FFFFE,
                MDER_NaN                = 0x007FFFFF,
                MDER_NRes               = 0x00800000,
                MDER_RESERVED_VALUE     = 0x00800001,
                MDER_NEGATIVE_INFINITY  = 0x00800002
            };

            enum ReservedSFloatValues : int16_t {
                MDER_S_POSITIVE_INFINITY = 0x07FE,
                MDER_S_NaN               = 0x07FF,
                MDER_S_NRes              = 0x0800,
                MDER_S_RESERVED_VALUE    = 0x0801,
                MDER_S_NEGATIVE_INFINITY = 0x0802
            };

            /**
             * Converts a 'IEEE-11073 16-bit SFLOAT' to std IEEE754 float.
             * <p>
             * raw_bt_float16_le is in little-endian, 2 bytes. Exponent at highest byte.
             * </p>
             */
            static float float16_IEEE11073_to_IEEE754(const uint16_t raw_bt_float16_le);

            /**
             * Converts a 'IEEE-11073 32-bit FLOAT' to std IEEE754 float.
             * <p>
             * Example: Temperature measurement, GattCharacteristicType::TEMPERATURE_MEASUREMENT.
             * </p>
             * <p>
             * raw_bt_float32_le is in little-endian, 4 bytes. Exponent at highest byte.
             * </p>
             */
            static float float32_IEEE11073_to_IEEE754(const uint32_t raw_bt_float32_le);
    };

} // namespace direct_bt

#endif /* IEEE11073_TYPES_HPP_ */
