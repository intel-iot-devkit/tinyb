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

#ifndef OCTET_TYPES_HPP_
#define OCTET_TYPES_HPP_

#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>
#include <algorithm>

#include <mutex>
#include <atomic>

#include "BasicTypes.hpp"
#include "UUID.hpp"
#include "BTAddress.hpp"

// #define TRACE_MEM 1
#ifdef TRACE_MEM
    #define TRACE_PRINT(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); }
#else
    #define TRACE_PRINT(...)
#endif

namespace direct_bt {

    /**
     * Transient read only octet data, i.e. non persistent passthrough, owned by caller.
     * <p>
     * Either ATT value (Vol 3, Part F 3.2.4) or PDU data.
     * </p>
     */
    class TROOctets
    {
        private:
            int _size;
            uint8_t * _data;

        protected:
            inline uint8_t * data() { return _data; }
            inline void setData(uint8_t *d, int s) {
                TRACE_PRINT("POctets setData: %p -> %p", _data, d);
                _data = d; _size = s;
            }
            inline void setSize(int s) { _size = s; }

        public:
            /** Transient passthrough read-only memory, w/o ownership ..*/
            TROOctets(const uint8_t *source, const int len)
            : _size( len ), _data( const_cast<uint8_t *>(source) ) { }

            TROOctets(const TROOctets &o) noexcept = default;
            TROOctets(TROOctets &&o) noexcept = default;
            TROOctets& operator=(const TROOctets &o) noexcept = default;
            TROOctets& operator=(TROOctets &&o) noexcept = default;

            inline void check_range(const int i, const int count) const {
                if( 0 > i || i+count > _size ) {
                    throw IndexOutOfBoundsException(i, count, _size, E_FILE_LINE);
                }
            }
            int getSize() const { return _size; }

            uint8_t get_uint8(const int i) const {
                check_range(i, 1);
                return _data[i];
            }
            int8_t get_int8(const int i) const {
                check_range(i, 1);
                return direct_bt::get_int8(_data, i);
            }

            uint16_t get_uint16(const int i) const {
                check_range(i, 2);
                return direct_bt::get_uint16(_data, i, true /* littleEndian */);
            }

            uint32_t get_uint32(const int i) const {
                check_range(i, 4);
                return direct_bt::get_uint32(_data, i, true /* littleEndian */);
            }

            EUI48 get_eui48(const int i) const {
                check_range(i, sizeof(EUI48));
                return EUI48(_data+i);
            }

            /** Assumes a null terminated string */
            std::string get_string(const int i) const {
                check_range(i, 1); // minimum size
                return std::string( (const char*)(_data+i) );
            }

            /** Assumes a string with defined length, not necessarily null terminated */
            std::string get_string(const int i, const int length) const {
                check_range(i, length);
                return std::string( (const char*)(_data+i), length );
            }

            uuid16_t get_uuid16(const int i) const {
                return uuid16_t(get_uint16(i));
            }
            uuid128_t get_uuid128(const int i) const {
                check_range(i, uuid_t::TypeSize::UUID128_SZ);
                return uuid128_t(get_uint128(_data, i, true /* littleEndian */));
            }
            std::shared_ptr<const uuid_t> get_uuid(const int i, const uuid_t::TypeSize tsize) const {
                check_range(i, tsize);
                return uuid_t::create(tsize, _data, i, true /* littleEndian */);
            }

            uint8_t const * get_ptr() const { return _data; }
            uint8_t const * get_ptr(const int i) const {
                check_range(i, 1);
                return _data + i;
            }

            bool operator==(const TROOctets& rhs) const {
                return _size == rhs._size && 0 == memcmp(_data, rhs._data, _size);
            }
            bool operator!=(const TROOctets& rhs) const {
                return !(*this == rhs);
            }

            std::string toString() const {
                return "size "+std::to_string(_size)+", ro: "+bytesHexString(_data, 0, _size, true /* lsbFirst */, true /* leading0X */);
            }
    };

    /**
     * Transient octet data, i.e. non persistent passthrough, owned by caller.
     * <p>
     * Either ATT value (Vol 3, Part F 3.2.4) or PDU data.
     * </p>
     */
    class TOctets : public TROOctets
    {
        public:
            /** Transient passthrough r/w memory, w/o ownership ..*/
            TOctets(uint8_t *source, const int len)
            : TROOctets(source, len) {}

            TOctets(const TOctets &o) noexcept = default;
            TOctets(TOctets &&o) noexcept = default;
            TOctets& operator=(const TOctets &o) noexcept = default;
            TOctets& operator=(TOctets &&o) noexcept = default;

            void put_uint8(const int i, const uint8_t v) {
                check_range(i, 1);
                data()[i] = v;;
            }

            void put_uint16(const int i, const uint16_t v) {
                check_range(i, 2);
                direct_bt::put_uint16(data(), i, v, true /* littleEndian */);
            }

            void put_uint32(const int i, const uint32_t v) {
                check_range(i, 4);
                direct_bt::put_uint32(data(), i, v, true /* littleEndian */);
            }

            void put_eui48(const int i, const EUI48 & v) {
                check_range(i, sizeof(v.b));
                memcpy(data() + i, v.b, sizeof(v.b));
            }

            void put_octets(const int i, const TROOctets & v) {
                check_range(i, v.getSize());
                memcpy(data() + i, v.get_ptr(), v.getSize());
            }

            void put_string(const int i, const std::string & v, const int max_len, const bool includeEOS) {
                const int size1 = v.size() + ( includeEOS ? 1 : 0 );
                const int size = std::min(size1, max_len);
                check_range(i, size);
                memcpy(data() + i, v.c_str(), size);
                if( size < size1 && includeEOS ) {
                    *(data() + i + size - 1) = 0; // ensure EOS
                }
            }

            void put_uuid(const int i, const uuid_t & v) {
                check_range(i, v.getTypeSize());
                direct_bt::put_uuid(data(), i, v, true /* littleEndian */);
            }

            uint8_t * get_wptr() { return data(); }
            uint8_t * get_wptr(const int i) {
                check_range(i, 1);
                return data() + i;
            }

            std::string toString() const {
                return "size "+std::to_string(getSize())+", rw: "+bytesHexString(get_ptr(), 0, getSize(), true /* lsbFirst */, true /* leading0X */);
            }
    };

    class TOctetSlice
    {
        private:
            const TOctets & parent;
            int const offset;
            int const size;

        public:
            TOctetSlice(const TOctets &buffer, const int offset, const int len)
            : parent(buffer), offset(offset), size(len)
            {
                if( offset+size > buffer.getSize() ) {
                    throw IndexOutOfBoundsException(offset, size, buffer.getSize(), E_FILE_LINE);
                }
            }

            int getSize() const { return size; }
            int getOffset() const { return offset; }
            const TOctets& getParent() const { return parent; }

            uint8_t get_uint8(const int i) const {
                return parent.get_uint8(offset+i);
            }

            uint16_t get_uint16(const int i) const {
                return parent.get_uint16(offset+i);
            }

            uint8_t const * get_ptr(const int i) const {
                return parent.get_ptr(offset+i);
            }

            std::string toString() const {
                return "offset "+std::to_string(offset)+", size "+std::to_string(size)+": "+bytesHexString(parent.get_ptr(), offset, size, true /* lsbFirst */, true /* leading0X */);
            }
    };

    /**
     * Persistent octet data, i.e. owned memory allocation.
     * <p>
     * GATT value (Vol 3, Part F 3.2.4)
     * </p>
     */
    class POctets : public TOctets
    {
        private:
            int capacity;

            void release() {
                uint8_t * ptr = get_wptr();
                if( nullptr == ptr ) {
                    throw InternalError("POctets::release: Null memory", E_FILE_LINE);
                }
                TRACE_PRINT("POctets release: %p", ptr);
                free(ptr);
                setData(nullptr, 0);
                capacity=0;
            }

        public:
            /** Takes ownership (malloc and copy, free) ..*/
            POctets(const uint8_t *_source, const int _size)
            : TOctets( static_cast<uint8_t*>( std::malloc(_size) ), _size),
              capacity( _size )
            {
                std::memcpy(get_wptr(), _source, _size);
                TRACE_PRINT("POctets ctor0: %p", get_wptr());
            }

            /** New buffer (malloc, free) */
            POctets(const int _capacity, const int _size)
            : TOctets( static_cast<uint8_t*>( std::malloc(_capacity) ), _size),
              capacity( _capacity )
            {
                if( capacity < getSize() ) {
                    throw IllegalArgumentException("capacity "+std::to_string(capacity)+" < size "+std::to_string(getSize()), E_FILE_LINE);
                }
                TRACE_PRINT("POctets ctor1: %p", get_wptr());
            }

            /** New buffer (malloc, free) */
            POctets(const int size)
            : POctets(size, size)
            {
                TRACE_PRINT("POctets ctor2: %p", get_wptr());
            }

            POctets(const POctets &_source) noexcept
            : TOctets( static_cast<uint8_t*>( std::malloc(_source.getSize()) ), _source.getSize()),
              capacity( _source.getCapacity() )
            {
                std::memcpy(get_wptr(), _source.get_ptr(), _source.getSize());
                TRACE_PRINT("POctets ctor-cpy0: %p", get_wptr());
            }

            POctets(POctets &&o) noexcept
            : TOctets( o.get_wptr(), o.getSize()),
              capacity( o.getCapacity() )
            {
                // moved origin data references
                // purge origin
                o.setData(nullptr, 0);
                o.capacity = 0;
                TRACE_PRINT("POctets ctor-move0: %p", get_wptr());
            }

            POctets& operator=(const POctets &_source) {
                if( this == &_source ) {
                    return *this;
                }
                release();
                setData(static_cast<uint8_t*>( std::malloc(_source.getSize()) ), _source.getSize());
                capacity = _source.getCapacity();
                std::memcpy(get_wptr(), _source.get_ptr(), _source.getSize());
                TRACE_PRINT("POctets assign0: %p", get_wptr());
                return *this;
            }

            POctets& operator=(POctets &&o) noexcept {
                // move origin data references
                setData(o.get_wptr(), o.getSize());
                capacity = o.capacity;
                // purge origin
                o.setData(nullptr, 0);
                o.capacity = 0;
                TRACE_PRINT("POctets assign-move0: %p", get_wptr());
                return *this;
            }

            ~POctets() { release(); }

            /** Makes a persistent POctets by copying the data from TROOctets. */
            POctets(const TROOctets & _source)
            : TOctets( static_cast<uint8_t*>( std::malloc(_source.getSize()) ), _source.getSize()),
              capacity( _source.getSize() )
            {
                std::memcpy(get_wptr(), _source.get_ptr(), _source.getSize());
                TRACE_PRINT("POctets ctor-cpy1: %p", get_wptr());
            }

            POctets& operator=(const TROOctets &_source) {
                if( static_cast<TROOctets *>(this) == &_source ) {
                    return *this;
                }
                release();
                setData(static_cast<uint8_t*>( std::malloc(_source.getSize()) ), _source.getSize());
                capacity = _source.getSize();
                std::memcpy(get_wptr(), _source.get_ptr(), _source.getSize());
                TRACE_PRINT("POctets assign1: %p", get_wptr());
                return *this;
            }

            /** Makes a persistent POctets by copying the data from TOctetSlice. */
            POctets(const TOctetSlice & _source)
            : TOctets( static_cast<uint8_t*>( std::malloc(_source.getSize()) ), _source.getSize()),
              capacity( _source.getSize() )
            {
                std::memcpy(get_wptr(), _source.getParent().get_ptr() + _source.getOffset(), _source.getSize());
                TRACE_PRINT("POctets ctor-cpy2: %p", get_wptr());
            }

            POctets& operator=(const TOctetSlice &_source) {
                release();
                setData(static_cast<uint8_t*>( std::malloc(_source.getSize()) ), _source.getSize());
                capacity = _source.getSize();
                std::memcpy(get_wptr(), _source.get_ptr(0), _source.getSize());
                TRACE_PRINT("POctets assign2: %p", get_wptr());
                return *this;
            }

            POctets & resize(const int newSize, const int newCapacity) {
                if( newCapacity < newSize ) {
                    throw IllegalArgumentException("newCapacity "+std::to_string(newCapacity)+" < newSize "+std::to_string(newSize), E_FILE_LINE);
                }
                if( newCapacity != capacity ) {
                    if( newSize > getSize() ) {
                        recapacity(newCapacity);
                        setSize(newSize);
                    } else {
                        setSize(newSize);
                        recapacity(newCapacity);
                    }
                } else {
                    setSize(newSize);
                }
                return *this;
            }

            POctets & resize(const int newSize) {
                if( capacity < newSize ) {
                    throw IllegalArgumentException("capacity "+std::to_string(capacity)+" < newSize "+std::to_string(newSize), E_FILE_LINE);
                }
                setSize(newSize);
                return *this;
            }

            POctets & recapacity(const int newCapacity) {
                if( newCapacity < getSize() ) {
                    throw IllegalArgumentException("newCapacity "+std::to_string(newCapacity)+" < size "+std::to_string(getSize()), E_FILE_LINE);
                }
                if( newCapacity == capacity ) {
                    return *this;
                }
                uint8_t* data2 = static_cast<uint8_t*>( std::malloc(newCapacity) );
                if( getSize() > 0 ) {
                    memcpy(data2, get_ptr(), getSize());
                }
                TRACE_PRINT("POctets recapacity: %p -> %p", get_wptr(), data2);
                free(get_wptr());
                setData(data2, getSize());
                capacity = newCapacity;
                return *this;
            }

            int getCapacity() const { return capacity; }

            POctets & operator+=(const TROOctets &b) {
                if( 0 < b.getSize() ) {
                    const int newSize = getSize() + b.getSize();
                    if( capacity < newSize ) {
                        recapacity( newSize );
                    }
                    memcpy(get_wptr()+getSize(), b.get_ptr(), b.getSize());
                    setSize(newSize);
                }
                return *this;
            }
            POctets & operator+=(const TOctetSlice &b) {
                if( 0 < b.getSize() ) {
                    const int newSize = getSize() + b.getSize();
                    if( capacity < newSize ) {
                        recapacity( newSize );
                    }
                    memcpy(get_wptr()+getSize(), b.getParent().get_ptr()+b.getOffset(), b.getSize());
                    setSize(newSize);
                }
                return *this;
            }

            std::string toString() const {
                return "size "+std::to_string(getSize())+", capacity "+std::to_string(getCapacity())+", l->h: "+bytesHexString(get_ptr(), 0, getSize(), true /* lsbFirst */, true /* leading0X */);
            }
    };


}


#endif /* OCTET_TYPES_HPP_ */
