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

#ifndef UUID_HPP_
#define UUID_HPP_

#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>

#include "BasicTypes.hpp"

namespace direct_bt {

class uuid128_t; // forward

/**
 * Bluetooth UUID <https://www.bluetooth.com/specifications/assigned-numbers/service-discovery/>
 * <p>
 * Bluetooth is LSB or Little-Endian!
 * </p>
 * <p>
 * BASE_UUID '00000000-0000-1000-8000-00805F9B34FB'
 * </p>
 */
extern uuid128_t BT_BASE_UUID;

class uuid_t {
public:
        /** Underlying integer value present octet count */
        enum TypeSize : int {
            UUID16_SZ=2, UUID32_SZ=4, UUID128_SZ=16
        };

private:
    TypeSize type;

protected:
    uuid_t(TypeSize const type) : type(type) {}

public:
    static TypeSize toTypeSize(const int size);
    static std::shared_ptr<const uuid_t> create(TypeSize const t, uint8_t const * const buffer, int const byte_offset, bool const littleEndian);

    virtual ~uuid_t() {}

    uuid_t(const uuid_t &o) noexcept = default;
    uuid_t(uuid_t &&o) noexcept = default;
    uuid_t& operator=(const uuid_t &o) noexcept = default;
    uuid_t& operator=(uuid_t &&o) noexcept = default;

    virtual bool operator==(uuid_t const &o) const {
        if( this == &o ) {
            return true;
        }
        return type == o.type;
    }
    bool operator!=(uuid_t const &o) const
    { return !(*this == o); }

    TypeSize getTypeSize() const { return type; }
    uuid128_t toUUID128(uuid128_t const & base_uuid=BT_BASE_UUID, int const uuid32_le_octet_index=12) const;
    /** returns the pointer to the uuid data of size getTypeSize() */
    virtual const uint8_t * data() const { return nullptr; }
    virtual std::string toString() const { return ""; }
    virtual std::string toUUID128String(uuid128_t const & base_uuid=BT_BASE_UUID, int const le_octet_index=12) const;
};

class uuid16_t : public uuid_t {
public:
    uint16_t value;

    uuid16_t(uint16_t const v)
    : uuid_t(TypeSize::UUID16_SZ), value(v) { }

    uuid16_t(uint8_t const * const buffer, int const byte_offset, bool const littleEndian)
    : uuid_t(TypeSize::UUID16_SZ), value(get_uint16(buffer, byte_offset, littleEndian)) { }

    uuid16_t(const uuid16_t &o) noexcept = default;
    uuid16_t(uuid16_t &&o) noexcept = default;
    uuid16_t& operator=(const uuid16_t &o) noexcept = default;
    uuid16_t& operator=(uuid16_t &&o) noexcept = default;

    bool operator==(uuid_t const &o) const override {
        if( this == &o ) {
            return true;
        }
        return getTypeSize() == o.getTypeSize() && value == static_cast<uuid16_t const *>(&o)->value;
    }

    const uint8_t * data() const override { return static_cast<uint8_t*>(static_cast<void*>(const_cast<uint16_t*>(&value))); }
    std::string toString() const override;
    std::string toUUID128String(uuid128_t const & base_uuid=BT_BASE_UUID, int const le_octet_index=12) const override;
};

class uuid32_t : public uuid_t {
public:
    uint32_t value;

    uuid32_t(uint32_t const v)
    : uuid_t(TypeSize::UUID32_SZ), value(v) {}

    uuid32_t(uint8_t const * const buffer, int const byte_offset, bool const littleEndian)
    : uuid_t(TypeSize::UUID32_SZ), value(get_uint32(buffer, byte_offset, littleEndian)) { }

    uuid32_t(const uuid32_t &o) noexcept = default;
    uuid32_t(uuid32_t &&o) noexcept = default;
    uuid32_t& operator=(const uuid32_t &o) noexcept = default;
    uuid32_t& operator=(uuid32_t &&o) noexcept = default;

    bool operator==(uuid_t const &o) const override {
        if( this == &o ) {
            return true;
        }
        return getTypeSize() == o.getTypeSize() && value == static_cast<uuid32_t const *>(&o)->value;
    }

    const uint8_t * data() const override { return static_cast<uint8_t*>(static_cast<void*>(const_cast<uint32_t*>(&value))); }
    std::string toString() const override;
    std::string toUUID128String(uuid128_t const & base_uuid=BT_BASE_UUID, int const le_octet_index=12) const override;
};

class uuid128_t : public uuid_t {
public:
    uint128_t value;

    uuid128_t() : uuid_t(TypeSize::UUID128_SZ) { bzero(value.data, sizeof(value)); }

    uuid128_t(uint128_t const v)
    : uuid_t(TypeSize::UUID128_SZ), value(v) {}

    uuid128_t(const std::string str);

    uuid128_t(uint8_t const * const buffer, int const byte_offset, bool const littleEndian)
    : uuid_t(TypeSize::UUID128_SZ), value(get_uint128(buffer, byte_offset, littleEndian)) { }

    uuid128_t(uuid16_t const & uuid16, uuid128_t const & base_uuid=BT_BASE_UUID, int const uuid16_le_octet_index=12);

    uuid128_t(uuid32_t const & uuid32, uuid128_t const & base_uuid=BT_BASE_UUID, int const uuid32_le_octet_index=12);

    uuid128_t(const uuid128_t &o) noexcept = default;
    uuid128_t(uuid128_t &&o) noexcept = default;
    uuid128_t& operator=(const uuid128_t &o) noexcept = default;
    uuid128_t& operator=(uuid128_t &&o) noexcept = default;

    bool operator==(uuid_t const &o) const override {
        if( this == &o ) {
            return true;
        }
        return getTypeSize() == o.getTypeSize() && value == static_cast<uuid128_t const *>(&o)->value;
    }

    const uint8_t * data() const override { return value.data; }
    std::string toString() const override;
    std::string toUUID128String(uuid128_t const & base_uuid=BT_BASE_UUID, int const le_octet_index=12) const override {
        (void)base_uuid;
        (void)le_octet_index;
        return toString();
    }
};

inline void put_uuid(uint8_t * buffer, int const byte_offset, const uuid_t &v)
{
    switch(v.getTypeSize()) {
        case uuid_t::TypeSize::UUID16_SZ:
            put_uint16(buffer, byte_offset, static_cast<const uuid16_t &>(v).value);
            break;
        case uuid_t::TypeSize::UUID32_SZ:
            put_uint32(buffer, byte_offset, static_cast<const uuid32_t &>(v).value);
            break;
        case uuid_t::TypeSize::UUID128_SZ:
            put_uint128(buffer, byte_offset, static_cast<const uuid128_t &>(v).value);
            break;
    }
}
inline void put_uuid(uint8_t * buffer, int const byte_offset, const uuid_t &v, bool littleEndian)
{
    switch(v.getTypeSize()) {
        case uuid_t::TypeSize::UUID16_SZ:
            put_uint16(buffer, byte_offset, static_cast<const uuid16_t &>(v).value, littleEndian);
            break;
        case uuid_t::TypeSize::UUID32_SZ:
            put_uint32(buffer, byte_offset, static_cast<const uuid32_t &>(v).value, littleEndian);
            break;
        case uuid_t::TypeSize::UUID128_SZ:
            put_uint128(buffer, byte_offset, static_cast<const uuid128_t &>(v).value, littleEndian);
            break;
    }
}

inline uuid16_t get_uuid16(uint8_t const * buffer, int const byte_offset)
{
    return uuid16_t(get_uint16(buffer, byte_offset));
}
inline uuid16_t get_uuid16(uint8_t const * buffer, int const byte_offset, bool littleEndian)
{
    return uuid16_t(get_uint16(buffer, byte_offset, littleEndian));
}
inline uuid32_t get_uuid32(uint8_t const * buffer, int const byte_offset)
{
    return uuid32_t(get_uint32(buffer, byte_offset));
}
inline uuid32_t get_uuid32(uint8_t const * buffer, int const byte_offset, bool littleEndian)
{
    return uuid32_t(get_uint32(buffer, byte_offset, littleEndian));
}
inline uuid128_t get_uuid128(uint8_t const * buffer, int const byte_offset)
{
    return uuid128_t(get_uint128(buffer, byte_offset));
}
inline uuid128_t get_uuid128(uint8_t const * buffer, int const byte_offset, bool littleEndian)
{
    return uuid128_t(get_uint128(buffer, byte_offset, littleEndian));
}

} /* namespace direct_bt */

#endif /* UUID_HPP_ */
