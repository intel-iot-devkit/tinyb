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

#include <mutex>
#include <atomic>

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
            inline void setData(uint8_t *d, int s) { _data = d; _size = s; }
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

            uint16_t get_uint16(const int i) const {
                check_range(i, 2);
                return direct_bt::get_uint16(_data, i, true /* littleEndian */);
            }

            uint32_t get_uint32(const int i) const {
                check_range(i, 4);
                return direct_bt::get_uint32(_data, i, true /* littleEndian */);
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

        public:
            /** Takes ownership (malloc and copy, free) ..*/
            POctets(const uint8_t *_source, const int _size)
            : TOctets( static_cast<uint8_t*>( std::malloc(_size) ), _size),
              capacity( _size )
            {
                std::memcpy(get_wptr(), _source, _size);
            }

            /** Blank zeroed buffer (calloc, free) */
            POctets(const int _capacity, const int _size)
            : TOctets( static_cast<uint8_t*>( std::malloc(_capacity) ), _size),
              capacity( _capacity )
            {
                if( capacity < getSize() ) {
                    throw IllegalArgumentException("capacity "+std::to_string(capacity)+" < size "+std::to_string(getSize()), E_FILE_LINE);
                }
            }

            /** Blank zeroed buffer (calloc, free) */
            POctets(const int size)
            : POctets(size, size)
            { }

            /** Makes a transient TOctets persistent by taking ownership (malloc and copy, free) ..*/
            POctets(const TOctets & _source)
            : TOctets( static_cast<uint8_t*>( std::malloc(_source.getSize()) ), _source.getSize()),
              capacity( _source.getSize() )
            {
                std::memcpy(get_wptr(), _source.get_ptr(), _source.getSize());
            }
            /** Makes a transient TOctetSlice persistent by taking ownership (malloc and copy, free) ..*/
            POctets(const TOctetSlice & _source)
            : TOctets( static_cast<uint8_t*>( std::malloc(_source.getSize()) ), _source.getSize()),
              capacity( _source.getSize() )
            {
                std::memcpy(get_wptr(), _source.getParent().get_ptr() + _source.getOffset(), _source.getSize());
            }
            ~POctets() { release(); }

            POctets(const POctets &_source) noexcept
            : TOctets( static_cast<uint8_t*>( std::malloc(_source.getSize()) ), _source.getSize()),
              capacity( _source.getCapacity() )
            {
                std::memcpy(get_wptr(), _source.get_ptr(), _source.getSize());
            }

            POctets& operator=(const POctets &_source) {
                if( this == &_source ) {
                    return *this;
                }
                release();
                setData(static_cast<uint8_t*>( std::malloc(_source.getSize()) ), _source.getSize());
                capacity = _source.getSize();
                return *this;
            }

            POctets(POctets &&o) noexcept = default;
            POctets& operator=(POctets &&o) noexcept = default;

            void release() {
                free(get_wptr());
                setData(nullptr, 0);
                capacity=0;
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
                free(get_wptr());
                setData(data2, getSize());
                capacity = newCapacity;
                return *this;
            }

            int getCapacity() const { return capacity; }

            POctets & operator+=(const TOctets &b) {
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
                return "size "+std::to_string(getSize())+", capacity "+std::to_string(getCapacity())+": "+bytesHexString(get_ptr(), 0, getSize(), true /* lsbFirst */, true /* leading0X */);
            }
    };


}


#endif /* OCTET_TYPES_HPP_ */
