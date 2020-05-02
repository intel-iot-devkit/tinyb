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
#include "DBTTypes.hpp"
#include "L2CAPComm.hpp"
#include "ATTPDUTypes.hpp"
#include "GATTTypes.hpp"
#include "LFRingbuffer.hpp"

/**
 * BT Core Spec v5.2: Vol 3, Part F Attribute Protocol (ATT)
 * BT Core Spec v5.2: Vol 3, Part G Generic Attribute Protocol (GATT)
 *
 * BT Core Spec v5.2: Vol 3, Part G GATT: 3.4 Summary of GATT Profile Attribute Types
 */

namespace direct_bt {

    typedef std::shared_ptr<GATTCharacterisicsDecl> GATTCharacterisicsDeclRef;

    class GATTNotificationListener {
        public:
            virtual void notificationReceived(std::shared_ptr<DBTDevice> dev, GATTCharacterisicsDeclRef charDecl,
                                              std::shared_ptr<const AttHandleValueRcv> charValue) = 0;
            virtual ~GATTNotificationListener() {}
    };
    class GATTIndicationListener {
        public:
            virtual void indicationReceived(std::shared_ptr<DBTDevice> dev, GATTCharacterisicsDeclRef charDecl,
                                            std::shared_ptr<const AttHandleValueRcv> charValue, const bool confirmationSent) = 0;
            virtual ~GATTIndicationListener() {}
    };

    /**
     * Representing a complete Primary Service Declaration
     * including its list of Characteristic Declarations,
     * which also may include its client config if available.
     */
    class GATTPrimaryService {
        public:
            /** The primary service declaration itself */
            const GATTUUIDHandleRange declaration;
            /** List of Characteristic Declarations as shared reference */
            std::vector<GATTCharacterisicsDeclRef> characteristicDeclList;

            GATTPrimaryService(const GATTUUIDHandleRange serviceDecl)
            : declaration(serviceDecl), characteristicDeclList() {
                characteristicDeclList.reserve(10);
            }

            std::string toString() const {
                std::string res = declaration.toString()+"[ ";
                for(size_t i=0; i<characteristicDeclList.size(); i++) {
                    if( 0 < i ) {
                        res += ", ";
                    }
                    res += std::to_string(i)+"[ "+characteristicDeclList.at(i)->toString()+" ]";
                }
                res += " ]";
                return res;
            }
    };

    typedef std::shared_ptr<GATTPrimaryService> GATTPrimaryServiceRef;


    /**
     * A thread safe GATT handler.
     * <p>
     * Implementation utilizes a lock free ringbuffer receiving data within its separate thread.
     * </p>
     */
    class GATTHandler {
        public:
            enum State : int {
                Error       = -1,
                Disconnected = 0,
                Connecting = 1,
                Connected = 2,
                RequestInProgress = 3,
                DiscoveringCharacteristics = 4,
                GetClientCharaceristicConfiguration = 5,
                WaitWriteResponse = 6,
                WaitReadResponse = 7
            };

            enum Defaults : int {
                /* BT Core Spec v5.2: Vol 3, Part F 3.2.8: Maximum length of an attribute value. */
                ClientMaxMTU = 512,

                /* BT Core Spec v5.2: Vol 3, Part G GATT: 5.2.1 ATT_MTU */
                DEFAULT_MIN_ATT_MTU = 23,

                /** 3s poll timeout for l2cap reader thread */
                L2CAP_READER_THREAD_POLL_TIMEOUT = 3000,
                ATTPDU_RING_CAPACITY = 256
            };

            static std::string getStateString(const State state);

       private:
            std::recursive_mutex mtx_write;
            POctets rbuffer;

            State state;
            std::shared_ptr<L2CAPComm> l2cap;
            const int timeoutMS;

            LFRingbuffer<std::shared_ptr<const AttPDUMsg>, nullptr> attPDURing;
            std::thread l2capReaderThread;
            volatile bool l2capReaderRunning;
            volatile bool l2capReaderShallStop;

            std::shared_ptr<GATTNotificationListener> gattNotificationListener = nullptr;
            std::shared_ptr<GATTIndicationListener> gattIndicationListener = nullptr;
            bool sendIndicationConfirmation = false;

            uint16_t serverMTU;
            uint16_t usedMTU;
            std::vector<GATTPrimaryServiceRef> services;

            State validateState();

            void l2capReaderThreadImpl();
            std::shared_ptr<const AttPDUMsg> receiveNext();

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 3.4.2 MTU Exchange
             * <p>
             * Returns the server-mtu if successful, otherwise 0.
             * </p>
             */
            uint16_t exchangeMTU(const uint16_t clientMaxMTU=ClientMaxMTU);

        public:
            GATTHandler(std::shared_ptr<L2CAPComm> l2cap, const int timeoutMS = Defaults::L2CAP_READER_THREAD_POLL_TIMEOUT);

            GATTHandler(std::shared_ptr<DBTDevice> device, const int timeoutMS = Defaults::L2CAP_READER_THREAD_POLL_TIMEOUT);

            ~GATTHandler();

            State getState() const { return state; }
            std::string getStateString() const { return getStateString(state); }

            /**
             * Replaces the GATTNotificationListener with the given instance, returning the replaced one.
             */
            std::shared_ptr<GATTNotificationListener> setGATTNotificationListener(std::shared_ptr<GATTNotificationListener> l);

            /**
             * Replaces the GATTNotificationListener with the given instance, returning the replaced one.
             * <p>
             * If {@code sendIndicationConfirmation} is {@code true}, a {@code ATT_HANDLE_VALUE_CFM}
             * will be sent automatically right after receiving the event.
             * </p>
             */
            std::shared_ptr<GATTIndicationListener> setGATTIndicationListener(std::shared_ptr<GATTIndicationListener> l, bool sendConfirmation);

            /**
             * After successful l2cap connection, the MTU will be exchanged.
             * See getServerMTU() and getUsedMTU(), the latter is in use.
             */
            bool connect();
            bool disconnect();
            bool isOpen() const { return Disconnected < state && l2cap->isOpen(); }

            bool send(const AttPDUMsg & msg);
            std::shared_ptr<const AttPDUMsg> sendWithReply(const AttPDUMsg & msg);

            uint16_t getServerMTU() const { return serverMTU; }
            uint16_t getUsedMTU()  const { return usedMTU; }

            /**
             * Find and return the GATTCharacterisicsDecl within internal primary services
             * via given characteristic handle.
             * <p>
             * Returns nullptr if not found.
             * </p>
             */
            GATTCharacterisicsDeclRef findCharacterisics(const uint16_t charHandle);

            /**
             * Find and return the GATTCharacterisicsDecl within given list of primary services
             * via given characteristic handle.
             * <p>
             * Returns nullptr if not found.
             * </p>
             */
            GATTCharacterisicsDeclRef findCharacterisics(const uint16_t charHandle, std::vector<GATTPrimaryServiceRef> &services);

            /**
             * Find and return the GATTCharacterisicsDecl within given primary service
             * via given characteristic handle.
             * <p>
             * Returns nullptr if not found.
             * </p>
             */
            GATTCharacterisicsDeclRef findCharacterisics(const uint16_t charHandle, GATTPrimaryServiceRef service);

            /**
             * Discover all primary services _and_ all its characteristics declarations
             * including their client config.
             * <p>
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.4.1 Discover All Primary Services
             * </p>
             * Method returns reference to GATTHandler internal data.
             */
            std::vector<GATTPrimaryServiceRef> & discoverCompletePrimaryServices();

            /**
             * Discover all primary services _only_.
             * <p>
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.4.1 Discover All Primary Services
             * </p>
             */
            bool discoverPrimaryServices(std::vector<GATTPrimaryServiceRef> & result);

            /**
             * Discover all characteristics of a service and declaration attributes _only_.
             * <p>
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.6.1 Discover All Characteristics of a Service
             * </p>
             * <p>
             * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.1 Characterisic Declaration Attribute Value
             * </p>
             */
            bool discoverCharacteristics(GATTPrimaryServiceRef service);

            /**
             * Discover all client characteristics config declaration _only_.
             * <p>
             * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration
             * </p>
             */
            bool discoverClientCharacteristicConfig(GATTPrimaryServiceRef service);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.7.1 Discover All Characteristic Descriptors
             * <p>
             * This is of little use, prefer discoverCharacteristics(..) and more elaborate GATTCharacterisicsDecl type!
             * </p>
             */
            bool discoverCharacteristicDescriptors(const GATTUUIDHandleRange & service, std::vector<GATTUUIDHandle> & result);

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
            bool readCharacteristicValue(const GATTCharacterisicsDecl & decl, POctets & res, int expectedLength=-1);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.8.2 Read Value Using Characteristic UUID
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
            bool readCharacteristicValue(const GATTUUIDHandleRange & charUUIDHandleRange, POctets & res, int expectedLength=-1);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.9.3 Write Characteristic Value
             * <p>
             * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration
             * </p>
             */
            bool writeClientCharacteristicConfigReq(const GATTClientCharacteristicConfigDecl & cccd, const TROOctets & value);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.9.1 Write Characteristic Value Without Response
             */
            bool writeClientCharacteristicConfigCmd(const GATTClientCharacteristicConfigDecl & cccd, const TROOctets & value);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.9.3 Write Characteristic Value
             */
            bool writeCharacteristicValueReq(const GATTCharacterisicsDecl & decl, const TROOctets & value);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 4.9.1 Write Characteristic Value Without Response
             */
            bool writeCharacteristicValueCmd(const GATTCharacterisicsDecl & decl, const TROOctets & value);

            /*****************************************************/
            /** Higher level semantic functionality **/
            /*****************************************************/

            std::shared_ptr<GenericAccess> getGenericAccess(std::vector<GATTPrimaryServiceRef> & primServices);
            std::shared_ptr<GenericAccess> getGenericAccess(std::vector<GATTCharacterisicsDeclRef> & genericAccessCharDeclList);

            std::shared_ptr<DeviceInformation> getDeviceInformation(std::vector<GATTPrimaryServiceRef> & primServices);
            std::shared_ptr<DeviceInformation> getDeviceInformation(std::vector<GATTCharacterisicsDeclRef> & deviceInfoCharDeclList);

            /**
             * BT Core Spec v5.2: Vol 3, Part G GATT: 3.3.3.3 Client Characteristic Configuration
             */
            bool configIndicationNotification(const GATTClientCharacteristicConfigDecl & decl, const bool enableNotification, const bool enableIndication);
    };

} // namespace direct_bt

#endif /* GATT_HANDLER_HPP_ */
