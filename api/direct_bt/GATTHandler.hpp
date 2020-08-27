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

#ifndef GATT_HANDLER_HPP_
#define GATT_HANDLER_HPP_

#include <cstring>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>

#include <mutex>
#include <atomic>
#include <thread>

#include "UUID.hpp"
#include "BTTypes.hpp"
#include "L2CAPComm.hpp"
#include "ATTPDUTypes.hpp"
#include "GATTTypes.hpp"
#include "LFRingbuffer.hpp"

/**
 * - - - - - - - - - - - - - - -
 *
 * Module GATTHandler:
 *
 * - BT Core Spec v5.2: Vol 3, Part G Generic Attribute Protocol (GATT)
 * - BT Core Spec v5.2: Vol 3, Part G GATT: 2.6 GATT Profile Hierarchy
 * - BT Core Spec v5.2: Vol 3, Part G GATT: 3.4 Summary of GATT Profile Attribute Types
 */
namespace direct_bt {

    class DBTDevice; // forward

    /**
     * GATT Singleton runtime environment properties
     */
    class GATTEnv {
        private:
            GATTEnv();

        public:
            /** L2CAP poll timeout for reader thread, defaults to 10s. */
            const int32_t L2CAP_READER_THREAD_POLL_TIMEOUT;
            /** Timeout for GATT read command replies, defaults to 500ms. */
            const int32_t GATT_READ_COMMAND_REPLY_TIMEOUT;
            /** Timeout for GATT write command replies, defaults to 500ms. */
            const int32_t GATT_WRITE_COMMAND_REPLY_TIMEOUT;
            /** Timeout for l2cap _initial_ command reply, defaults to 2500ms. */
            const int32_t GATT_INITIAL_COMMAND_REPLY_TIMEOUT;

            /** Medium ringbuffer capacity, defaults to 128 messages. */
            const int32_t ATTPDU_RING_CAPACITY;

            /** Debug all GATT Data communication */
            const bool DEBUG_DATA;

        public:
            static GATTEnv& get() {
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
                static GATTEnv e;
                return e;
            }
    };

    /**
     * A thread safe GATT handler associated to one device via one L2CAP connection.
     * <p>
     * Implementation utilizes a lock free ringbuffer receiving data within its separate thread.
     * </p>
     * <p>
     * Controlling Environment variables:
     * <pre>
     * - 'direct_bt.debug.gatt.data': Debug messages about detailed GATT data, see debug_data
     * </pre>
     * </p>
     */
    class GATTHandler {
        public:
            enum class Defaults : int32_t {
                /* BT Core Spec v5.2: Vol 3, Part F 3.2.8: Maximum length of an attribute value. */
                MAX_ATT_MTU = 512,

                /* BT Core Spec v5.2: Vol 3, Part G GATT: 5.2.1 ATT_MTU */
                MIN_ATT_MTU = 23
            };
            static inline int number(const Defaults d) { return static_cast<int>(d); }

       private:
            const GATTEnv & env;

            /** GATTHandle's device weak back-reference */
            std::weak_ptr<DBTDevice> wbr_device;

            const std::string deviceString;
            std::recursive_mutex mtx_command;
            POctets rbuffer;

            L2CAPComm l2cap;
            std::atomic<bool> isConnected; // reflects state
            std::atomic<bool> hasIOError;  // reflects state

            LFRingbuffer<std::shared_ptr<const AttPDUMsg>, nullptr> attPDURing;
            std::atomic<pthread_t> l2capReaderThreadId;
            std::atomic<bool> l2capReaderRunning;
            std::atomic<bool> l2capReaderShallStop;
            std::mutex mtx_l2capReaderInit;
            std::condition_variable cv_l2capReaderInit;

            /** send immediate confirmation of indication events from device, defaults to true. */
            bool sendIndicationConfirmation = true;
            std::vector<std::shared_ptr<GATTCharacteristicListener>> characteristicListenerList;
            std::recursive_mutex mtx_eventListenerList;

            uint16_t serverMTU;
            uint16_t usedMTU;
            std::vector<GATTServiceRef> services;

            std::shared_ptr<DBTDevice> getDevice() const { return wbr_device.lock(); }

            bool validateConnected();

            void l2capReaderThreadImpl();

            void send(const AttPDUMsg & msg);
            std::shared_ptr<const AttPDUMsg> sendWithReply(const AttPDUMsg & msg, const int timeout);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 3.4.2 MTU Exchange
             * <p>
             * Returns the server-mtu if successful, otherwise 0.
             * </p>
             */
            uint16_t exchangeMTU(const uint16_t clientMaxMTU);

        public:
            GATTHandler(const std::shared_ptr<DBTDevice> & device);

            ~GATTHandler();

            bool getIsConnected() const { return isConnected; }
            bool getHasIOError() const { return hasIOError; }
            std::string getStateString() const { return L2CAPComm::getStateString(isConnected, hasIOError); }

            /**
             * After successful l2cap connection, the MTU will be exchanged.
             * See getServerMTU() and getUsedMTU(), the latter is in use.
             */
            bool connect();

            /**
             * Disconnect this GATTHandler and optionally the associated device
             * @param disconnectDevice if true, associated device will also be disconnected, otherwise not.
             * @param ioErrorCause if true, reason for disconnection is an IO error
             * @return true if successful, otherwise false
             */
            bool disconnect(const bool disconnectDevice, const bool ioErrorCause);

            bool isOpen() const { return isConnected && l2cap.isOpen(); }

            uint16_t getServerMTU() const { return serverMTU; }
            uint16_t getUsedMTU()  const { return usedMTU; }

            /**
             * Find and return the GATTCharacterisicsDecl within internal primary services
             * via given characteristic value handle.
             * <p>
             * Returns nullptr if not found.
             * </p>
             */
            GATTCharacteristicRef findCharacterisicsByValueHandle(const uint16_t charValueHandle);

            /**
             * Find and return the GATTCharacterisicsDecl within given list of primary services
             * via given characteristic value handle.
             * <p>
             * Returns nullptr if not found.
             * </p>
             */
            GATTCharacteristicRef findCharacterisicsByValueHandle(const uint16_t charValueHandle, std::vector<GATTServiceRef> &services);

            /**
             * Find and return the GATTCharacterisicsDecl within given primary service
             * via given characteristic value handle.
             * <p>
             * Returns nullptr if not found.
             * </p>
             */
            GATTCharacteristicRef findCharacterisicsByValueHandle(const uint16_t charValueHandle, GATTServiceRef service);

            /**
             * Discover all primary services _and_ all its characteristics declarations
             * including their client config.
             * <p>
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.4.1 Discover All Primary Services
             * </p>
             * Method returns reference to GATTHandler internal data.
             */
            std::vector<GATTServiceRef> & discoverCompletePrimaryServices();

            /**
             * Returns a reference of the internal kept GATTService list.
             * <p>
             * The internal list will be populated via {@link #discoverCompletePrimaryServices()}.
             * </p>
             */
            std::vector<GATTServiceRef> & getServices() { return services; }

            /**
             * Discover all primary services _only_.
             * <p>
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.4.1 Discover All Primary Services
             * </p>
             */
            bool discoverPrimaryServices(std::vector<GATTServiceRef> & result);

            /**
             * Discover all characteristics of a service and declaration attributes _only_.
             * <p>
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.6.1 Discover All Characteristics of a Service
             * </p>
             * <p>
             * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.1 Characterisic Declaration Attribute Value
             * </p>
             */
            bool discoverCharacteristics(GATTServiceRef & service);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.7.1 Discover All Characteristic Descriptors
             */
            bool discoverDescriptors(GATTServiceRef & service);

            /**
             * Generic read GATT value and long value
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
             */
            bool readValue(const uint16_t handle, POctets & res, int expectedLength=-1);

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
             */
            bool readCharacteristicValue(const GATTCharacteristic & c, POctets & res, int expectedLength=-1);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.12.1 Read Characteristic Descriptor
             * <p>
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.12.2 Read Long Characteristic Descriptor
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
             */
            bool readDescriptorValue(GATTDescriptor & cd, int expectedLength=-1);

            /**
             * Generic write GATT value and long value
             */
            bool writeValue(const uint16_t handle, const TROOctets & value, const bool withResponse);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.12.3 Write Characteristic Descriptors
             * <p>
             * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3 Characteristic Descriptor
             * </p>
             * <p>
             * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration
             * </p>
             */
            bool writeDescriptorValue(const GATTDescriptor & cd);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.9.3 Write Characteristic Value
             */
            bool writeCharacteristicValue(const GATTCharacteristic & c, const TROOctets & value);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.9.1 Write Characteristic Value Without Response
             */
            bool writeCharacteristicValueNoResp(const GATTCharacteristic & c, const TROOctets & value);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration
             * <p>
             * Method enables notification and/or indication for the corresponding characteristic at BLE level.
             * </p>
             * <p>
             * It is recommended to utilize notification over indication, as its link-layer handshake
             * and higher potential bandwidth may deliver material higher performance.
             * </p>
             * <p>
             * Throws an IllegalArgumentException if the given GATTDescriptor is not a ClientCharacteristicConfiguration.
             * </p>
             */
            bool configNotificationIndication(GATTDescriptor & cd, const bool enableNotification, const bool enableIndication);

            /**
             * Add the given listener to the list if not already present.
             * <p>
             * Returns true if the given listener is not element of the list and has been newly added,
             * otherwise false.
             * </p>
             */
            bool addCharacteristicListener(std::shared_ptr<GATTCharacteristicListener> l);

            /**
             * Remove the given listener from the list.
             * <p>
             * Returns true if the given listener is an element of the list and has been removed,
             * otherwise false.
             * </p>
             */
            bool removeCharacteristicListener(std::shared_ptr<GATTCharacteristicListener> l);

            /**
             * Remove the given listener from the list.
             * <p>
             * Returns true if the given listener is an element of the list and has been removed,
             * otherwise false.
             * </p>
             */
            bool removeCharacteristicListener(const GATTCharacteristicListener * l);

            
            /**
             * Remove all {@link GATTCharacteristicListener} from the list, which are associated to the given {@link GATTCharacteristic}.
             * <p>
             * Implementation tests all listener's GATTCharacteristicListener::match(const GATTCharacteristic & characteristic)
             * to match with the given associated characteristic.
             * </p>
             * @param associatedCharacteristic the match criteria to remove any GATTCharacteristicListener from the list
             * @return number of removed listener.
             */
            int removeAllAssociatedCharacteristicListener(std::shared_ptr<GATTCharacteristic> associatedCharacteristic);

            int removeAllAssociatedCharacteristicListener(const GATTCharacteristic * associatedCharacteristic);

            /**
             * Remove all event listener from the list.
             * <p>
             * Returns the number of removed event listener.
             * </p>
             */
            int removeAllCharacteristicListener();

            /**
             * Enable or disable sending an immediate confirmation for received indication events from the device.
             * <p>
             * Default value is true.
             * </p>
             * <p>
             * This setting is per GATTHandler and hence per DBTDevice.
             * </p>
             */
            void setSendIndicationConfirmation(const bool v);

            /**
             * Returns whether sending an immediate confirmation for received indication events from the device is enabled.
             * <p>
             * Default value is true.
             * </p>
             * <p>
             * This setting is per GATTHandler and hence per DBTDevice.
             * </p>
             */
            bool getSendIndicationConfirmation();

            /*****************************************************/
            /** Higher level semantic functionality **/
            /*****************************************************/

            std::shared_ptr<GenericAccess> getGenericAccess(std::vector<GATTServiceRef> & primServices);
            std::shared_ptr<GenericAccess> getGenericAccess(std::vector<GATTCharacteristicRef> & genericAccessCharDeclList);

            std::shared_ptr<DeviceInformation> getDeviceInformation(std::vector<GATTServiceRef> & primServices);
            std::shared_ptr<DeviceInformation> getDeviceInformation(std::vector<GATTCharacteristicRef> & deviceInfoCharDeclList);

            /**
             * Issues a ping to the device, validating whether it is still reachable.
             * <p>
             * This method could be periodically utilized to shorten the underlying OS disconnect period
             * after turning the device off, which lies within 7-13s.
             * </p>
             * <p>
             * In case the device is no more reachable, disconnect will be initiated due to the occurring IO error.
             * </p>
             * @return {@code true} if successful, otherwise false in case no GATT services exists etc.
             */
            bool ping();
    };

} // namespace direct_bt

#endif /* GATT_HANDLER_HPP_ */
