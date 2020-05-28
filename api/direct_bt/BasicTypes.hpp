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

#ifndef BASIC_TYPES_HPP_
#define BASIC_TYPES_HPP_

#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>

extern "C" {
    #include <endian.h>
    #include <byteswap.h>
}

namespace direct_bt {

    /**
     * Returns current monotonic time in milliseconds.
     */
    int64_t getCurrentMilliseconds();

    #define E_FILE_LINE __FILE__,__LINE__

    class RuntimeException : public std::exception {
      protected:
        std::string msg;

        RuntimeException(std::string const type, std::string const m, const char* file, int line) noexcept
        : msg(std::string(type).append(" @ ").append(file).append(":").append(std::to_string(line)).append(": ").append(m)) { }

      public:
        RuntimeException(std::string const m, const char* file, int line) noexcept
        : RuntimeException("RuntimeException", m, file, line) {}

        virtual ~RuntimeException() noexcept { }

        RuntimeException(const RuntimeException &o) = default;
        RuntimeException(RuntimeException &&o) = default;
        RuntimeException& operator=(const RuntimeException &o) = default;
        RuntimeException& operator=(RuntimeException &&o) = default;

        virtual const char* what() const noexcept override;
    };

    class InternalError : public RuntimeException {
      public:
        InternalError(std::string const m, const char* file, int line) noexcept
        : RuntimeException("InternalError", m, file, line) {}
    };

    class NullPointerException : public RuntimeException {
      public:
        NullPointerException(std::string const m, const char* file, int line) noexcept
        : RuntimeException("NullPointerException", m, file, line) {}
    };

    class IllegalArgumentException : public RuntimeException {
      public:
        IllegalArgumentException(std::string const m, const char* file, int line) noexcept
        : RuntimeException("IllegalArgumentException", m, file, line) {}
    };

    class IllegalStateException : public RuntimeException {
      public:
        IllegalStateException(std::string const m, const char* file, int line) noexcept
        : RuntimeException("IllegalStateException", m, file, line) {}
    };

    class UnsupportedOperationException : public RuntimeException {
      public:
        UnsupportedOperationException(std::string const m, const char* file, int line) noexcept
        : RuntimeException("UnsupportedOperationException", m, file, line) {}
    };

    class IndexOutOfBoundsException : public RuntimeException {
      public:
        IndexOutOfBoundsException(const int index, const int count, const int length, const char* file, int line) noexcept
        : RuntimeException("IndexOutOfBoundsException", "Index "+std::to_string(index)+", count "+std::to_string(count)+", data length "+std::to_string(length), file, line) {}
    };

    class BluetoothException : public RuntimeException {
      public:
        BluetoothException(std::string const m, const char* file, int line) noexcept
        : RuntimeException("BluetoothException", m, file, line) {}

        BluetoothException(const char *m, const char* file, int line) noexcept
        : RuntimeException("BluetoothException", m, file, line) {}
    };

    /**
    // *************************************************
    // *************************************************
    // *************************************************
     */

    struct __attribute__((packed)) uint128_t {
        uint8_t data[16];

        bool operator==(uint128_t const &o) const {
            if( this == &o ) {
                return true;
            }
            return !std::memcmp(data, o.data, 16);
        }
        bool operator!=(uint128_t const &o) const
        { return !(*this == o); }
    };

    inline uint128_t bswap(uint128_t const & source) {
        uint128_t dest;
        uint8_t const * const s = source.data;
        uint8_t * const d = dest.data;
        for(int i=0; i<16; i++) {
            d[i] = s[15-i];
        }
        return dest;
    }

    /**
     * On the i386 the host byte order is Least Significant Byte first (LSB) or Little-Endian,
     * whereas the network byte order, as used on the Internet, is Most Significant Byte first (MSB) or Big-Endian.
     * See #include <arpa/inet.h>
     *
     * Bluetooth is LSB or Little-Endian!
     */

#if __BYTE_ORDER == __BIG_ENDIAN
    inline uint16_t be_to_cpu(uint16_t const & n) {
        return n;
    }
    inline uint16_t cpu_to_be(uint16_t const & h) {
        return h;
    }
    inline uint16_t le_to_cpu(uint16_t const & l) {
        return bswap_16(l);
    }
    inline uint16_t cpu_to_le(uint16_t const & h) {
        return bswap_16(h);
    }

    inline uint32_t be_to_cpu(uint32_t const & n) {
        return n;
    }
    inline uint32_t cpu_to_be(uint32_t const & h) {
        return h;
    }
    inline uint32_t le_to_cpu(uint32_t const & l) {
        return bswap_32(l);
    }
    inline uint32_t cpu_to_le(uint32_t const & h) {
        return bswap_32(h);
    }

    inline uint128_t be_to_cpu(uint128_t const & n) {
        return n;
    }
    inline uint128_t cpu_to_be(uint128_t const & h) {
        return n;
    }
    inline uint128_t le_to_cpu(uint128_t const & l) {
        return bswap(l);
    }
    inline uint128_t cpu_to_le(uint128_t const & h) {
        return bswap(h);
    }
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    inline uint16_t be_to_cpu(uint16_t const & n) {
        return bswap_16(n);
    }
    inline uint16_t cpu_to_be(uint16_t const & h) {
        return bswap_16(h);
    }
    inline uint16_t le_to_cpu(uint16_t const & l) {
        return l;
    }
    inline uint16_t cpu_to_le(uint16_t const & h) {
        return h;
    }

    inline uint32_t be_to_cpu(uint32_t const & n) {
        return bswap_32(n);
    }
    inline uint32_t cpu_to_be(uint32_t const & h) {
        return bswap_32(h);
    }
    inline uint32_t le_to_cpu(uint32_t const & l) {
        return l;
    }
    inline uint32_t cpu_to_le(uint32_t const & h) {
        return h;
    }

    inline uint128_t be_to_cpu(uint128_t const & n) {
        return bswap(n);
    }
    inline uint128_t cpu_to_be(uint128_t const & h) {
        return bswap(h);
    }
    inline uint128_t le_to_cpu(uint128_t const & l) {
        return l;
    }
    inline uint128_t cpu_to_le(uint128_t const & h) {
        return h;
    }
#else
    #error "Unexpected __BYTE_ORDER"
#endif

    inline void put_uint8(uint8_t * buffer, int const byte_offset, const uint8_t v)
    {
        uint8_t * p = (uint8_t *) ( buffer + byte_offset );
        *p = v;
    }
    inline uint8_t get_uint8(uint8_t const * buffer, int const byte_offset)
    {
        uint8_t const * p = (uint8_t const *) ( buffer + byte_offset );
        return *p;
    }
    inline int8_t get_int8(uint8_t const * buffer, int const byte_offset)
    {
        int8_t const * p = (int8_t const *) ( buffer + byte_offset );
        return *p;
    }
    inline void put_uint16(uint8_t * buffer, int const byte_offset, const uint16_t v)
    {
        uint16_t * p = (uint16_t *) ( buffer + byte_offset );
        *p = v;
    }
    inline void put_uint16(uint8_t * buffer, int const byte_offset, const uint16_t v, bool littleEndian)
    {
        uint16_t * p = (uint16_t *) ( buffer + byte_offset );
        *p = littleEndian ? cpu_to_le(v) : cpu_to_be(v);
    }
    inline uint16_t get_uint16(uint8_t const * buffer, int const byte_offset)
    {
        uint16_t const * p = (uint16_t const *) ( buffer + byte_offset );
        return *p;
    }
    inline uint16_t get_uint16(uint8_t const * buffer, int const byte_offset, bool littleEndian)
    {
        uint16_t const * p = (uint16_t const *) ( buffer + byte_offset );
        return littleEndian ? le_to_cpu(*p) : be_to_cpu(*p);
    }

    inline void put_uint32(uint8_t * buffer, int const byte_offset, const uint32_t v)
    {
        uint32_t * p = (uint32_t *) ( buffer + byte_offset );
        *p = v;
    }
    inline void put_uint32(uint8_t * buffer, int const byte_offset, const uint32_t v, bool littleEndian)
    {
        uint32_t * p = (uint32_t *) ( buffer + byte_offset );
        *p = littleEndian ? cpu_to_le(v) : cpu_to_be(v);
    }
    inline uint32_t get_uint32(uint8_t const * buffer, int const byte_offset)
    {
        uint32_t const * p = (uint32_t const *) ( buffer + byte_offset );
        return *p;
    }
    inline uint32_t get_uint32(uint8_t const * buffer, int const byte_offset, bool littleEndian)
    {
        uint32_t const * p = (uint32_t const *) ( buffer + byte_offset );
        return littleEndian ? le_to_cpu(*p) : be_to_cpu(*p);
    }

    inline void put_uint128(uint8_t * buffer, int const byte_offset, const uint128_t v)
    {
        uint128_t * p = (uint128_t *) ( buffer + byte_offset );
        *p = v;
    }
    inline void put_uint128(uint8_t * buffer, int const byte_offset, const uint128_t v, bool littleEndian)
    {
        uint128_t * p = (uint128_t *) ( buffer + byte_offset );
        *p = littleEndian ? cpu_to_le(v) : cpu_to_be(v);
    }
    inline uint128_t get_uint128(uint8_t const * buffer, int const byte_offset)
    {
        uint128_t const * p = (uint128_t const *) ( buffer + byte_offset );
        return *p;
    }
    inline uint128_t get_uint128(uint8_t const * buffer, int const byte_offset, bool littleEndian)
    {
        uint128_t const * p = (uint128_t const *) ( buffer + byte_offset );
        return littleEndian ? le_to_cpu(*p) : be_to_cpu(*p);
    }

    /**
     * Returns a C++ String taken from buffer with maximum length of min(max_len, max_len).
     * <p>
     * The maximum length only delimits the string length and does not contain the EOS null byte.
     * An EOS null byte will will be added.
     * </p>
     * <p>
     * The source string within buffer is not required to contain an EOS null byte;
     * </p>
     */
    std::string get_string(const uint8_t *buffer, int const buffer_len, int const max_len);

    /**
     * Merge the given 'uuid16' into a 'base_uuid' copy at the given little endian 'uuid16_le_octet_index' position.
     * <p>
     * The given 'uuid16' value will be added with the 'base_uuid' copy at the given position.
     * </p>
     * <pre>
     * base_uuid: 00000000-0000-1000-8000-00805F9B34FB
     *    uuid16: DCBA
     * uuid16_le_octet_index: 12
     *    result: 0000DCBA-0000-1000-8000-00805F9B34FB
     *
     * LE: low-mem - FB349B5F8000-0080-0010-0000-ABCD0000 - high-mem
     *                                           ^ index 12
     * LE: uuid16 -> value.data[12+13]
     *
     * BE: low-mem - 0000DCBA-0000-1000-8000-00805F9B34FB - high-mem
     *                   ^ index 2
     * BE: uuid16 -> value.data[2+3]
     * </pre>
     */
    uint128_t merge_uint128(uint16_t const uuid16, uint128_t const & base_uuid, int const uuid16_le_octet_index);

    /**
     * Merge the given 'uuid32' into a 'base_uuid' copy at the given little endian 'uuid32_le_octet_index' position.
     * <p>
     * The given 'uuid32' value will be added with the 'base_uuid' copy at the given position.
     * </p>
     * <pre>
     * base_uuid: 00000000-0000-1000-8000-00805F9B34FB
     *    uuid32: 87654321
     * uuid32_le_octet_index: 12
     *    result: 87654321-0000-1000-8000-00805F9B34FB
     *
     * LE: low-mem - FB349B5F8000-0080-0010-0000-12345678 - high-mem
     *                                           ^ index 12
     * LE: uuid32 -> value.data[12..15]
     *
     * BE: low-mem - 87654321-0000-1000-8000-00805F9B34FB - high-mem
     *               ^ index 0
     * BE: uuid32 -> value.data[0..3]
     * </pre>
     */
    uint128_t merge_uint128(uint32_t const uuid32, uint128_t const & base_uuid, int const uuid32_le_octet_index);

    std::string uint8HexString(const uint8_t v, const bool leading0X=true);
    std::string uint16HexString(const uint16_t v, const bool leading0X=true);
    std::string uint32HexString(const uint32_t v, const bool leading0X=true);
    std::string uint64HexString(const uint64_t v, const bool leading0X=true);
    std::string aptrHexString(const void * v, const bool leading0X=true);

    /**
     * If lsbFirst is true, orders LSB left -> MSB right, usual for byte streams.
     * <p>
     * Otherwise orders MSB left -> LSB right, usual for readable integer values.
     * </p>
     */
    std::string bytesHexString(const uint8_t * bytes, const int offset, const int length, const bool lsbFirst, const bool leading0X);

    std::string int32SeparatedString(const int32_t v, const char separator=',');
    std::string uint32SeparatedString(const uint32_t v, const char separator=',');
    std::string uint64SeparatedString(const uint64_t v, const char separator=',');

    /** trim in place */
    void trimInPlace(std::string &s);

    /** trim copy */
    std::string trimCopy(const std::string &s);

    /**
     * Returns all valid consecutive UTF-8 characters within buffer
     * in the range up to buffer_size or until EOS.
     * <p>
     * In case a non UTF-8 character has been detected,
     * the content will be cut off and the decoding loop ends.
     * </p>
     */
    std::string getUTF8String(const uint8_t *buffer, const size_t buffer_size);

} // namespace direct_bt

#endif /* BASIC_TYPES_HPP_ */
