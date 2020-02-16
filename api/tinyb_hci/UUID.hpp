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

#pragma once
#include <cstring>
#include <memory>
#include <cstdint>
#include <vector>
#include <functional>

#include "HCIUtil.hpp"

namespace tinyb_hci {

class UUID128;

/**
 * Bluetooth UUID <https://www.bluetooth.com/specifications/assigned-numbers/service-discovery/>
 * <p>
 * Bluetooth is LSB or Little-Endian!
 * </p>
 * <p>
 * BASE_UUID '00000000-0000-1000-8000-00805F9B34FB'
 * </p>
 */
extern UUID128 BT_BASE_UUID;

class UUID {
public:
	/** Underlying integer value present octet count */
	enum class Type : int {
		UUID16=2, UUID32=4, UUID128=16
	};
	Type const type;
	UUID(Type const type) : type(type) {}
	virtual ~UUID() {};

	UUID(const UUID &o) noexcept = default;
	UUID(UUID &&o) noexcept = default;
	UUID& operator=(const UUID &o) noexcept = default;
	UUID& operator=(UUID &&o) noexcept = default;

	virtual bool operator==(UUID const &o) const = 0;
	virtual bool operator!=(UUID const &o) const = 0;

	Type getType() const { return type; }
	virtual std::string toString() const = 0;
	virtual std::string toUUID128String(UUID128 const & base_uuid=BT_BASE_UUID, int const le_octet_index=12) const = 0;
};

class UUID16 : public UUID {
public:
	uint16_t const value;

	UUID16(uint16_t const v)
	: UUID(Type::UUID16), value(v) { }

	UUID16(uint8_t const * const buffer, int const byte_offset, bool littleEndian)
	: UUID(Type::UUID16), value(get_uint16(buffer, byte_offset, littleEndian)) { }

	UUID16(const UUID16 &o) noexcept = default;
	UUID16(UUID16 &&o) noexcept = default;
	UUID16& operator=(const UUID16 &o) noexcept = default;
	UUID16& operator=(UUID16 &&o) noexcept = default;

	bool operator==(UUID const &o) const override {
		if( this == &o ) {
			return true;
		}
		return type == o.type && value == static_cast<UUID16 const *>(&o)->value;
	}
	bool operator!=(UUID const &o) const override
	{ return !(*this == o); }

	std::string toString() const override;
	std::string toUUID128String(UUID128 const & base_uuid=BT_BASE_UUID, int const le_octet_index=12) const override;
};

class UUID32 : public UUID {
public:
	uint32_t const value;

	UUID32(uint32_t const v)
	: UUID(Type::UUID32), value(v) {}

	UUID32(uint8_t const * const buffer, int const byte_offset, bool const littleEndian)
	: UUID(Type::UUID32), value(get_uint32(buffer, byte_offset, littleEndian)) { }

	UUID32(const UUID32 &o) noexcept = default;
	UUID32(UUID32 &&o) noexcept = default;
	UUID32& operator=(const UUID32 &o) noexcept = default;
	UUID32& operator=(UUID32 &&o) noexcept = default;

	bool operator==(UUID const &o) const override {
		if( this == &o ) {
			return true;
		}
		return type == o.type && value == static_cast<UUID32 const *>(&o)->value;
	}
	bool operator!=(UUID const &o) const override
	{ return !(*this == o); }

	std::string toString() const override;
	std::string toUUID128String(UUID128 const & base_uuid=BT_BASE_UUID, int const le_octet_index=12) const override;
};

class UUID128 : public UUID {
public:
	uint128_t const value;

	/** Creates an instance by given big-endian byte array of 16 bytes */
	UUID128(uint8_t const bigEndianBytes[])
	: UUID128(bigEndianBytes, 0, false)
	{ }

	UUID128(uint128_t const v)
	: UUID(Type::UUID128), value(v) {}

	UUID128(uint8_t const * const buffer, int const byte_offset, bool const littleEndian)
	: UUID(Type::UUID128), value(get_uint128(buffer, byte_offset, littleEndian)) { }

	UUID128(UUID128 const & base_uuid, UUID16 const & uuid16, int const uuid16_le_octet_index);

	UUID128(UUID128 const & base_uuid, UUID32 const & uuid32, int const uuid32_le_octet_index);

	UUID128(const UUID128 &o) noexcept = default;
	UUID128(UUID128 &&o) noexcept = default;
	UUID128& operator=(const UUID128 &o) noexcept = default;
	UUID128& operator=(UUID128 &&o) noexcept = default;

	bool operator==(UUID const &o) const override {
		if( this == &o ) {
			return true;
		}
		return type == o.type && value == static_cast<UUID128 const *>(&o)->value;
	}
	bool operator!=(UUID const &o) const override
	{ return !(*this == o); }

	std::string toString() const override;
	std::string toUUID128String(UUID128 const & base_uuid=BT_BASE_UUID, int const le_octet_index=12) const override {
		(void)base_uuid;
		(void)le_octet_index;
		return toString();
	}
};

} /* namespace tinyb_hci */

#endif /* UUID_HPP_ */
