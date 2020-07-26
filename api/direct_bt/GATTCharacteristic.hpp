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

#ifndef GATT_CHARACTERISTIC_HPP_
#define GATT_CHARACTERISTIC_HPP_

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

#include "GATTDescriptor.hpp"

#include "JavaUplink.hpp"

/**
 * - - - - - - - - - - - - - - -
 *
 * Module GATTCharacteristic:
 *
 * - BT Core Spec v5.2: Vol 3, Part G Generic Attribute Protocol (GATT)
 * - BT Core Spec v5.2: Vol 3, Part G GATT: 2.6 GATT Profile Hierarchy
 */
namespace direct_bt {

    class GATTCharacteristicListener; // forward

    class GATTService; // forward
    typedef std::shared_ptr<GATTService> GATTServiceRef;

    /**
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.1 Characteristic Declaration Attribute Value
     * </p>
     * handle -> CDAV value
     * <p>
     * BT Core Spec v5.2: Vol 3, Part G GATT: 4.6.1 Discover All Characteristics of a Service
     *
     * Here the handle is a service's characteristics-declaration
     * and the value the Characteristics Property, Characteristics Value Handle _and_ Characteristics UUID.
     * </p>
     */
    class GATTCharacteristic : public DBTObject {
        private:
            /** Characteristics's service weak back-reference */
            std::weak_ptr<GATTService> wbr_service;
            bool enabledNotifyState = false;
            bool enabledIndicateState = false;

            /**
             * For an unknown reason, using the virtual function 'toString()'
             * while constructing an exception message causes a SIGSEGV.
             * <p>
             * This method represents a non-virtual variation,
             * which also does not call any other virtual function.
             * </p>
             */
            std::string toSafeString() const;

        public:
            /** BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.1.1 Characteristic Properties */
            enum PropertyBitVal : uint8_t {
                Broadcast = 0x01,
                Read = 0x02,
                WriteNoAck = 0x04,
                WriteWithAck = 0x08,
                Notify = 0x10,
                Indicate = 0x20,
                AuthSignedWrite = 0x40,
                ExtProps = 0x80
            };
            /**
             * Returns string values as defined in <https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/gatt-api.txt>
             * <pre>
             * org.bluez.GattCharacteristic1 :: array{string} Flags [read-only]
             * </pre>
             */
            static std::string getPropertyString(const PropertyBitVal prop);

            static std::string getPropertiesString(const PropertyBitVal properties);
            static std::vector<std::unique_ptr<std::string>> getPropertiesStringList(const PropertyBitVal properties);

            /**
             * Characteristics's Service Handle - key to service's handle range, retrieved from Characteristics data.
             * <p>
             * Attribute handles are unique for each device (server) (BT Core Spec v5.2: Vol 3, Part F Protocol..: 3.2.2 Attribute Handle).
             * </p>
             */
            const uint16_t service_handle;

            /**
             * Characteristic Handle of this instance.
             * <p>
             * Attribute handles are unique for each device (server) (BT Core Spec v5.2: Vol 3, Part F Protocol..: 3.2.2 Attribute Handle).
             * </p>
             */
            const uint16_t handle;

            /* Characteristics Property */
            const PropertyBitVal properties;

            /**
             * Characteristics Value Handle.
             * <p>
             * Attribute handles are unique for each device (server) (BT Core Spec v5.2: Vol 3, Part F Protocol..: 3.2.2 Attribute Handle).
             * </p>
             */
            const uint16_t value_handle;

            /* Characteristics Value Type UUID */
            std::shared_ptr<const uuid_t> value_type;

            /** List of Characteristic Descriptions as shared reference */
            std::vector<GATTDescriptorRef> descriptorList;

            /* Optional Client Characteristic Configuration index within descriptorList */
            int clientCharacteristicsConfigIndex = -1;

            GATTCharacteristic(const GATTServiceRef & service, const uint16_t service_handle, const uint16_t handle,
                                   const PropertyBitVal properties, const uint16_t value_handle, std::shared_ptr<const uuid_t> value_type)
            : wbr_service(service), service_handle(service_handle), handle(handle),
              properties(properties), value_handle(value_handle), value_type(value_type) {}

            std::string get_java_class() const override {
                return java_class();
            }
            static std::string java_class() {
                return std::string(JAVA_DBT_PACKAGE "DBTGattCharacteristic");
            }

            std::shared_ptr<GATTService> getServiceUnchecked() const { return wbr_service.lock(); }
            std::shared_ptr<GATTService> getServiceChecked() const;
            std::shared_ptr<DBTDevice> getDeviceUnchecked() const;
            std::shared_ptr<DBTDevice> getDeviceChecked() const;

            bool hasProperties(const PropertyBitVal v) const { return v == ( properties & v ); }

            std::string getPropertiesString() const {
                return getPropertiesString(properties);
            }
            std::string toString() const override;

            void clearDescriptors() {
                descriptorList.clear();
                clientCharacteristicsConfigIndex = -1;
            }

            GATTDescriptorRef getClientCharacteristicConfig() {
                if( 0 > clientCharacteristicsConfigIndex ) {
                    return nullptr;
                }
                return descriptorList.at(clientCharacteristicsConfigIndex);
            }

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration
             * <p>
             * Method enables notification and/or indication for this characteristic at BLE level.
             * </p>
             * <p>
             * Implementation masks this Characteristic properties PropertyBitVal::Notify and PropertyBitVal::Indicate
             * with the respective user request parameters, hence removes unsupported requests.
             * </p>
             * <p>
             * Notification and/or indication configuration is only performed per characteristic if changed.
             * </p>
             * <p>
             * It is recommended to utilize notification over indication, as its link-layer handshake
             * and higher potential bandwidth may deliver material higher performance.
             * </p>
             * @param enableNotification
             * @param enableIndication
             * @param enabledState array of size 2, holding the resulting enabled state for notification and indication.
             * @return false if this characteristic has no PropertyBitVal::Notify or PropertyBitVal::Indication present,
             * or there is no GATTDescriptor of type ClientCharacteristicConfiguration, or if the operation has failed.
             * Otherwise returns true.
             * @throws IllegalStateException if notification or indication is set to be enabled
             * and the {@link DBTDevice's}'s {@link GATTHandler} is null, i.e. not connected
             */
            bool configNotificationIndication(const bool enableNotification, const bool enableIndication, bool enabledState[2]);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration
             * <p>
             * Method will attempt to enable notification on the BLE level, if available,
             * otherwise indication if available.
             * </p>
             * <p>
             * Notification and/or indication configuration is only performed per characteristic if changed.
             * </p>
             * <p>
             * It is recommended to utilize notification over indication, as its link-layer handshake
             * and higher potential bandwidth may deliver material higher performance.
             * </p>
             * @param enabledState array of size 2, holding the resulting enabled state for notification and indication.
             * @return false if this characteristic has no PropertyBitVal::Notify or PropertyBitVal::Indication present,
             * or there is no GATTDescriptor of type ClientCharacteristicConfiguration, or if the operation has failed.
             * Otherwise returns true.
             * @throws IllegalStateException if notification or indication is set to be enabled
             * and the {@link DBTDevice's}'s {@link GATTHandler} is null, i.e. not connected
             */
            bool enableNotificationOrIndication(bool enabledState[2]);

            /**
             * Add the given GATTCharacteristicListener to the listener list if not already present.
             * <p>
             * Occurring notifications and indications, if enabled via configNotificationIndication(bool, bool, bool[])
             * or enableNotificationOrIndication(bool[]),
             * will call the respective GATTCharacteristicListener callback method.
             * </p>
             * <p>
             * Returns true if the given listener is not element of the list and has been newly added,
             * otherwise false.
             * </p>
             * <p>
             * Convenience delegation call to GATTHandler via DBTDevice
             * </p>
             * <p>
             * To restrict the listener to listen only to this GATTCharacteristic instance,
             * user has to implement GATTCharacteristicListener::match(GATTCharacteristicRef) accordingly.
             * <br>
             * For this purpose, use may derive from AssociatedGATTCharacteristicListener,
             * which provides these simple matching filter facilities.
             * </p>
             * @throws IllegalStateException if the {@link DBTDevice's}'s {@link GATTHandler} is null, i.e. not connected
             */
            bool addCharacteristicListener(std::shared_ptr<GATTCharacteristicListener> l);

            /**
             * Add the given GATTCharacteristicListener to the listener list if not already present
             * and if enabling the notification <i>or</i> indication for this characteristic at BLE level was successful.<br>
             * Notification and/or indication configuration is only performed per characteristic if changed.
             * <p>
             * Implementation will attempt to enable notification only, if available,
             * otherwise indication if available. <br>
             * Implementation uses enableNotificationOrIndication(bool[]) to enable either.
             * </p>
             * <p>
             * Occurring notifications and indications will call the respective GATTCharacteristicListener
             * callback method.
             * </p>
             * <p>
             * Returns true if enabling the notification and/or indication was successful
             * and if the given listener is not element of the list and has been newly added,
             * otherwise false.
             * </p>
             * <p>
             * To restrict the listener to listen only to this GATTCharacteristic instance,
             * user has to implement GATTCharacteristicListener::match(GATTCharacteristicRef) accordingly.
             * <br>
             * For this purpose, use may derive from AssociatedGATTCharacteristicListener,
             * which provides these simple matching filter facilities.
             * </p>
             * @param enabledState array of size 2, holding the resulting enabled state for notification and indication
             * using enableNotificationOrIndication(bool[])
             * @throws IllegalStateException if the {@link DBTDevice's}'s {@link GATTHandler} is null, i.e. not connected
             */
            bool addCharacteristicListener(std::shared_ptr<GATTCharacteristicListener> l, bool enabledState[2]);

            /**
             * Disables the notification and/or indication for this characteristic at BLE level
             * if {@code disableIndicationNotification == true}
             * and removes the given {@link GATTCharacteristicListener} from the listener list.
             * <p>
             * Returns true if the given listener is an element of the list and has been removed,
             * otherwise false.
             * </p>
             * <p>
             * Convenience delegation call to GATTHandler via DBTDevice
             * performing addCharacteristicListener(..)
             * and {@link #configNotificationIndication(bool, bool, bool[]) if {@code disableIndicationNotification == true}.
             * </p>
             * <p>
             * If the DBTDevice's GATTHandler is null, i.e. not connected, {@code false} is being returned.
             * </p>
             * @param l
             * @param disableIndicationNotification if true, disables the notification and/or indication for this characteristic
             * using {@link #configNotificationIndication(bool, bool, bool[])
             * @return
             */
            bool removeCharacteristicListener(std::shared_ptr<GATTCharacteristicListener> l, bool disableIndicationNotification);

            /**
             * Disables the notification and/or indication for this characteristic at BLE level
             * if {@code disableIndicationNotification == true}
             * and removes all {@link GATTCharacteristicListener} from the listener list.
             * <p>
             * Returns the number of removed event listener.
             * </p>
             * <p>
             * Convenience delegation call to GATTHandler via DBTDevice
             * performing addCharacteristicListener(..)
             * and configNotificationIndication(..) if {@code disableIndicationNotification == true}.
             * </p>
             * <p>
             * If the DBTDevice's GATTHandler is null, i.e. not connected, {@code zero} is being returned.
             * </p>
             * @param disableIndicationNotification if true, disables the notification and/or indication for this characteristic
             * using {@link #configNotificationIndication(bool, bool, bool[])
             * @return
             */
            int removeAllCharacteristicListener(bool disableIndicationNotification);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.8.1 Read Characteristic Value
             * <p>
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.8.3 Read Long Characteristic Value
             * </p>
             * <p>
             * If expectedLength = 0, then only one ATT_READ_REQ/RSP will be used.
             * </p>
             * <p>
             * If expectedLength < 0, then long values using multiple ATT_READ_BLOB_REQ/RSP will be used until
             * the response returns zero. This is the default parameter.
             * </p>
             * <p>
             * If expectedLength > 0, then long values using multiple ATT_READ_BLOB_REQ/RSP will be used
             * if required until the response returns zero.
             * </p>
             * <p>
             * Convenience delegation call to GATTHandler via DBTDevice
             * <p>
             * </p>
             * If the DBTDevice's GATTHandler is null, i.e. not connected, an IllegalStateException is thrown.
             * </p>
             */
            bool readValue(POctets & res, int expectedLength=-1);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.9.3 Write Characteristic Value
             * <p>
             * Convenience delegation call to GATTHandler via DBTDevice
             * <p>
             * </p>
             * If the DBTDevice's GATTHandler is null, i.e. not connected, an IllegalStateException is thrown.
             * </p>
             */
            bool writeValue(const TROOctets & value);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.9.1 Write Characteristic Value Without Response
             * <p>
             * Convenience delegation call to GATTHandler via DBTDevice
             * <p>
             * </p>
             * If the DBTDevice's GATTHandler is null, i.e. not connected, an IllegalStateException is thrown.
             * </p>
             */
            bool writeValueNoResp(const TROOctets & value);
    };
    typedef std::shared_ptr<GATTCharacteristic> GATTCharacteristicRef;

    inline bool operator==(const GATTCharacteristic& lhs, const GATTCharacteristic& rhs)
    { return lhs.handle == rhs.handle; /** unique attribute handles */ }

    inline bool operator!=(const GATTCharacteristic& lhs, const GATTCharacteristic& rhs)
    { return !(lhs == rhs); }

    /**
     * {@link GATTCharacteristic} event listener for notification and indication events.
     * <p>
     * A listener instance may be attached to a {@link BluetoothGattCharacteristic} via
     * {@link GATTCharacteristic::addCharacteristicListener(std::shared_ptr<GATTCharacteristicListener>)} to listen to events,
     * see method's API doc for {@link GATTCharacteristic} filtering.
     * </p>
     * <p>
     * User may utilize {@link AssociatedGATTCharacteristicListener} to listen to only one {@link GATTCharacteristic}.
     * </p>
     * <p>
     * A listener instance may be attached to a {@link GATTHandler} via
     * {@link GATTHandler::addCharacteristicListener(std::shared_ptr<GATTCharacteristicListener>)}
     * to listen to all events of the device or the matching filtered events.
     * </p>
     * <p>
     * The listener receiver maintains a unique set of listener instances without duplicates.
     * </p>
     */
    class GATTCharacteristicListener {
        public:
            /**
             * Custom filter for all event methods,
             * which will not be called if this method returns false.
             * <p>
             * User may override this method to test whether the methods shall be called
             * for the given GATTCharacteristic.
             * </p>
             * <p>
             * Defaults to true;
             * </p>
             */
            virtual bool match(const GATTCharacteristic & characteristic) {
                (void)characteristic;
                return true;
            }

            /**
             * Called from native BLE stack, initiated by a received notification associated
             * with the given {@link GATTCharacteristic}.
             * @param charDecl {@link GATTCharacteristic} related to this notification
             * @param charValue the notification value
             * @param timestamp the indication monotonic timestamp, see getCurrentMilliseconds()
             */
            virtual void notificationReceived(GATTCharacteristicRef charDecl,
                                              std::shared_ptr<TROOctets> charValue, const uint64_t timestamp) = 0;

            /**
             * Called from native BLE stack, initiated by a received indication associated
             * with the given {@link GATTCharacteristic}.
             * @param charDecl {@link GATTCharacteristic} related to this indication
             * @param charValue the indication value
             * @param timestamp the indication monotonic timestamp, see {@link BluetoothUtils#getCurrentMilliseconds()}
             * @param confirmationSent if true, the native stack has sent the confirmation, otherwise user is required to do so.
             */
            virtual void indicationReceived(GATTCharacteristicRef charDecl,
                                            std::shared_ptr<TROOctets> charValue, const uint64_t timestamp,
                                            const bool confirmationSent) = 0;

            virtual ~GATTCharacteristicListener() {}

            /**
             * Default comparison operator, merely testing for same memory reference.
             * <p>
             * Specializations may override.
             * </p>
             */
            virtual bool operator==(const GATTCharacteristicListener& rhs) const
            { return this == &rhs; }

            bool operator!=(const GATTCharacteristicListener& rhs) const
            { return !(*this == rhs); }
    };

    class AssociatedGATTCharacteristicListener : public GATTCharacteristicListener{
        private:
            const GATTCharacteristic * associatedCharacteristic;

        public:
            /**
             * Passing the associated GATTCharacteristic to filter out non matching events.
             */
            AssociatedGATTCharacteristicListener(const GATTCharacteristic * characteristicMatch)
            : associatedCharacteristic(characteristicMatch) { }

            bool match(const GATTCharacteristic & characteristic) override {
                if( nullptr == associatedCharacteristic ) {
                    return true;
                }
                return *associatedCharacteristic == characteristic;
            }
    };

} // namespace direct_bt

#endif /* GATT_CHARACTERISTIC_HPP_ */
