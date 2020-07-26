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

#ifndef GATT_SERVICE_HPP_
#define GATT_SERVICE_HPP_

#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>

#include <mutex>
#include <atomic>

#include "UUID.hpp"
#include "BTTypes.hpp"
#include "OctetTypes.hpp"
#include "ATTPDUTypes.hpp"

#include "DBTTypes.hpp"

#include "GATTCharacteristic.hpp"

#include "JavaUplink.hpp"

/**
 * - - - - - - - - - - - - - - -
 *
 * Module GATTService:
 *
 * - BT Core Spec v5.2: Vol 3, Part G Generic Attribute Protocol (GATT)
 * - BT Core Spec v5.2: Vol 3, Part G GATT: 2.6 GATT Profile Hierarchy
 */
namespace direct_bt {

    class DBTDevice; // forward

    /**
     * Representing a complete [Primary] Service Declaration
     * including its list of Characteristic Declarations,
     * which also may include its client config if available.
     */
    class GATTService : public DBTObject {
        private:
            /** Service's device weak back-reference */
            std::weak_ptr<DBTDevice> wbr_device;

        public:
            const bool isPrimary;

            /**
             * Service start handle
             * <p>
             * Attribute handles are unique for each device (server) (BT Core Spec v5.2: Vol 3, Part F Protocol..: 3.2.2 Attribute Handle).
             * </p>
             */
            const uint16_t startHandle;

            /**
             * Service end handle
             * <p>
             * Attribute handles are unique for each device (server) (BT Core Spec v5.2: Vol 3, Part F Protocol..: 3.2.2 Attribute Handle).
             * </p>
             */
            const uint16_t endHandle;

            /** Service type UUID */
            std::shared_ptr<const uuid_t> type;

            /** List of Characteristic Declarations as shared reference */
            std::vector<GATTCharacteristicRef> characteristicList;

            GATTService(const std::shared_ptr<DBTDevice> &device, const bool isPrimary,
                        const uint16_t startHandle, const uint16_t endHandle, std::shared_ptr<const uuid_t> type)
            : wbr_device(device), isPrimary(isPrimary), startHandle(startHandle), endHandle(endHandle), type(type), characteristicList() {
                characteristicList.reserve(10);
            }

            std::string get_java_class() const override {
                return java_class();
            }
            static std::string java_class() {
                return std::string(JAVA_DBT_PACKAGE "DBTGattService");
            }

            std::shared_ptr<DBTDevice> getDeviceUnchecked() const { return wbr_device.lock(); }
            std::shared_ptr<DBTDevice> getDeviceChecked() const;

            std::string toString() const;
    };

    inline bool operator==(const GATTService& lhs, const GATTService& rhs)
    { return lhs.startHandle == rhs.startHandle && lhs.endHandle == rhs.endHandle; /** unique attribute handles */ }

    inline bool operator!=(const GATTService& lhs, const GATTService& rhs)
    { return !(lhs == rhs); }

} // namespace direct_bt

#endif /* GATT_SERVICE_HPP_ */
