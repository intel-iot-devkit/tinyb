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

#include <direct_bt/BTAddress.hpp>
#include <direct_bt/HCITypes.hpp>
#include <direct_bt/ATTPDUTypes.hpp>
#include <direct_bt/GATTHandler.hpp>
#include <direct_bt/GATTNumbers.hpp>
#include <cinttypes>

extern "C" {
    #include <unistd.h>
}

using namespace direct_bt;

class DeviceDiscoveryListener : public direct_bt::HCIDeviceDiscoveryListener {
    void deviceAdded(direct_bt::HCIAdapter const &a, std::shared_ptr<direct_bt::HCIDevice> device) override {
        fprintf(stderr, "****** ADDED__: %s\n", device->toString().c_str());
        fprintf(stderr, "Status HCIAdapter:\n");
        fprintf(stderr, "%s\n", a.toString().c_str());
    }
    void deviceUpdated(direct_bt::HCIAdapter const &a, std::shared_ptr<direct_bt::HCIDevice> device) override {
        fprintf(stderr, "****** UPDATED: %s\n", device->toString().c_str());
        fprintf(stderr, "Status HCIAdapter:\n");
        fprintf(stderr, "%s\n", a.toString().c_str());
    }
};

static const uuid16_t _TEMPERATURE_MEASUREMENT(GattCharacteristicType::TEMPERATURE_MEASUREMENT);

class MyGATTNotificationListener : public direct_bt::GATTNotificationListener {
    void notificationReceived(std::shared_ptr<HCIDevice> dev,
                              GATTCharacterisicsDeclRef charDecl, std::shared_ptr<const AttHandleValueRcv> charValue) override {
        const int64_t tR = direct_bt::getCurrentMilliseconds();
        fprintf(stderr, "****** GATT Notify (td %" PRIu64 " ms, dev-discovered %" PRIu64 " ms): From %s\n",
                (tR-charValue->ts_creation), (tR-dev->ts_creation), dev->toString().c_str());
        if( nullptr != charDecl ) {
            fprintf(stderr, "****** decl %s\n", charDecl->toString().c_str());
        }
        fprintf(stderr, "****** rawv %s\n", charValue->toString().c_str());
    }
};
class MyGATTIndicationListener : public direct_bt::GATTIndicationListener {
    void indicationReceived(std::shared_ptr<HCIDevice> dev,
                            GATTCharacterisicsDeclRef charDecl, std::shared_ptr<const AttHandleValueRcv> charValue,
                            const bool confirmationSent) override
    {
        const int64_t tR = direct_bt::getCurrentMilliseconds();
        fprintf(stderr, "****** GATT Indication (confirmed %d, td(msg %" PRIu64 " ms, dev-discovered %" PRIu64 " ms): From %s\n",
                confirmationSent, (tR-charValue->ts_creation), (tR-dev->ts_creation), dev->toString().c_str());
        if( nullptr != charDecl ) {
            fprintf(stderr, "****** decl %s\n", charDecl->toString().c_str());
            if( _TEMPERATURE_MEASUREMENT == *charDecl->uuid ) {
                std::shared_ptr<TemperatureMeasurementCharateristic> temp = TemperatureMeasurementCharateristic::get(charValue->getValue());
                if( nullptr != temp ) {
                    fprintf(stderr, "****** valu %s\n", temp->toString().c_str());
                }
            }
        }
        fprintf(stderr, "****** rawv %s\n", charValue->toString().c_str());
    }
};

// #define SCAN_CHARACTERISTIC_DESCRIPTORS 1
// #define SHOW_STATIC_SERVICE_CHARACTERISTIC_COMPOSITION 1

int main(int argc, char *argv[])
{
    bool ok = true, done=false;
    bool waitForEnter=false;
    EUI48 waitForDevice = EUI48_ANY_DEVICE;

    /**
     * BT Core Spec v5.2:  Vol 3, Part A L2CAP Spec: 7.9 PRIORITIZING DATA OVER HCI
     *
     * In order for guaranteed channels to meet their guarantees,
     * L2CAP should prioritize traffic over the HCI transport in devices that support HCI.
     * Packets for Guaranteed channels should receive higher priority than packets for Best Effort channels.
     * ...
     * I have noticed that w/o HCI le_connect, overall communication takes twice as long!!!
     */
    bool doHCI_LEConnect = true;

    for(int i=1; i<argc; i++) {
        if( !strcmp("-wait", argv[i]) ) {
            waitForEnter = true;
        } else if( !strcmp("-skipLEConnect", argv[i]) ) {
            doHCI_LEConnect = false;
        } else if( !strcmp("-mac", argv[i]) && argc > (i+1) ) {
            std::string macstr = std::string(argv[++i]);
            waitForDevice = EUI48(macstr);
            fprintf(stderr, "waitForDevice: %s\n", waitForDevice.toString().c_str());
        }
    }

    if( waitForEnter ) {
        fprintf(stderr, "Press ENTER to continue\n");
        getchar();
    }

    direct_bt::HCIAdapter adapter; // default
    if( !adapter.hasDevId() ) {
        fprintf(stderr, "Default adapter not available.\n");
        exit(1);
    }
    if( !adapter.isValid() ) {
        fprintf(stderr, "Adapter invalid.\n");
        exit(1);
    }
    fprintf(stderr, "Adapter: device %s, address %s\n", 
        adapter.getName().c_str(), adapter.getAddressString().c_str());

    adapter.setDeviceDiscoveryListener(std::shared_ptr<direct_bt::HCIDeviceDiscoveryListener>(new DeviceDiscoveryListener()));

    const int64_t t0 = direct_bt::getCurrentMilliseconds();

    std::shared_ptr<direct_bt::HCISession> session = adapter.open();

    while( ok && !done && nullptr != session ) {
        ok = adapter.startDiscovery(*session);
        if( !ok) {
            perror("Adapter start discovery failed");
            goto out;
        }

        const int deviceCount = adapter.discoverDevices(*session, 1, waitForDevice);
        if( 0 > deviceCount ) {
            perror("Adapter discovery failed");
            ok = false;
        }
        adapter.stopDiscovery(*session);

        if( ok && 0 < deviceCount ) {
            const uint64_t t1 = direct_bt::getCurrentMilliseconds();
            std::vector<std::shared_ptr<direct_bt::HCIDevice>> discoveredDevices = adapter.getDiscoveredDevices();
            int i=0, j=0, k=0;
            for(auto it = discoveredDevices.begin(); it != discoveredDevices.end(); it++) {
                i++;
                std::shared_ptr<direct_bt::HCIDevice> device = *it;
                const uint64_t lup = device->getLastUpdateAge(t1);
                if( 2000 > lup ) {
                    // less than 2s old ..
                    j++;
                    if( waitForDevice == device->getAddress() ) {
                        done = true;
                    }

                    //
                    // HCI LE-Connect
                    // (Without: Overall communication takes ~twice as long!!!)
                    //
                    uint16_t hciLEConnHandle;
                    if( doHCI_LEConnect ) {
                        hciLEConnHandle = device->le_connect(*session);
                        if( 0 == hciLEConnHandle ) {
                            fprintf(stderr, "HCI LE Connection: Failed %s\n", device->toString().c_str());
                        } else {
                            const uint64_t t3 = direct_bt::getCurrentMilliseconds();
                            const uint64_t td0 = t3 - t0;
                            const uint64_t td1 = t3 - t1;
                            fprintf(stderr, "HCI LE Connect: Success\n");
                            fprintf(stderr, "  hci connect-only %" PRIu64 " ms,\n"
                                            "  discovered to hci-connected %" PRIu64 " ms,\n"
                                            "  total %" PRIu64 " ms,\n"
                                            "  handle 0x%X\n",
                                            td1, (t3 - device->getCreationTimestamp()), td0, hciLEConnHandle);
                        }
                    } else {
                        fprintf(stderr, "HCI LE Connection: Skipped %s\n", device->toString().c_str());
                        hciLEConnHandle = 0;
                    }

                    k++;

                    //
                    // GATT Processing
                    //
                    const uint64_t t4 = direct_bt::getCurrentMilliseconds();
                    // let's check further for full GATT
                    direct_bt::GATTHandler gatt(device, 10000);
                    if( gatt.connect() ) {
                        fprintf(stderr, "GATT usedMTU %d (server) -> %d (used)\n", gatt.getServerMTU(), gatt.getUsedMTU());

                        gatt.setGATTIndicationListener(std::shared_ptr<GATTIndicationListener>(new MyGATTIndicationListener()), true /* sendConfirmation */);
                        gatt.setGATTNotificationListener(std::shared_ptr<GATTNotificationListener>(new MyGATTNotificationListener()));

#ifdef SCAN_CHARACTERISTIC_DESCRIPTORS                         
                        std::vector<std::vector<GATTUUIDHandle>> servicesCharacteristicDescriptors;
#endif                        
                        std::vector<GATTPrimaryServiceRef> & primServices = gatt.discoverCompletePrimaryServices();
                        const uint64_t t5 = direct_bt::getCurrentMilliseconds();
#ifdef SCAN_CHARACTERISTIC_DESCRIPTORS                        
                        for(size_t i=0; i<primServices.size(); i++) {
                            std::vector<GATTUUIDHandle> serviceDescriptors;
                            gatt.discoverCharDescriptors(primServices.at(i), serviceDescriptors);
                            servicesCharacteristicDescriptors.push_back(serviceDescriptors);
                        }
#endif                        
                        const uint64_t t7 = direct_bt::getCurrentMilliseconds();
                        {
                            const uint64_t td45 = t5 - t4; // connect -> complete primary services
                            const uint64_t td47 = t7 - t4; // connect -> gatt complete
                            const uint64_t td07 = t7 - t0; // total
                            fprintf(stderr, "\n\n\n");
                            fprintf(stderr, "GATT primary-services completed\n");
                            fprintf(stderr, "  gatt connect -> complete primary-services %" PRIu64 " ms,\n"
                                            "  gatt connect -> gatt complete %" PRIu64 " ms,\n"
                                            "  discovered to gatt complete %" PRIu64 " ms,\n"
                                            "  total %" PRIu64 " ms\n\n",
                                            td45, td47, (t7 - device->getCreationTimestamp()), td07);
                        }
                        {
                            std::shared_ptr<GenericAccess> ga = gatt.getGenericAccess(primServices);
                            if( nullptr != ga ) {
                                fprintf(stderr, "  GenericAccess: %s\n\n", ga->toString().c_str());
                            }
                        }
                        {
                            std::shared_ptr<DeviceInformation> di = gatt.getDeviceInformation(primServices);
                            if( nullptr != di ) {
                                fprintf(stderr, "  DeviceInformation: %s\n\n", di->toString().c_str());
                            }
                        }

                        for(size_t i=0; i<primServices.size(); i++) {
                            GATTPrimaryService & primService = *primServices.at(i);
                            fprintf(stderr, "  [%2.2d] Service %s\n", (int)i, primService.toString().c_str());
                            fprintf(stderr, "  [%2.2d] Service Characteristics\n", (int)i);
                            std::vector<GATTCharacterisicsDeclRef> & serviceCharacteristics = primService.characteristicDeclList;
                            for(size_t j=0; j<serviceCharacteristics.size(); j++) {
                                GATTCharacterisicsDecl & serviceChar = *serviceCharacteristics.at(j);
                                fprintf(stderr, "  [%2.2d.%2.2d] Decla: %s\n", (int)i, (int)j, serviceChar.toString().c_str());
                                if( serviceChar.hasProperties(GATTCharacterisicsDecl::PropertyBitVal::Read) ) {
                                    POctets value(GATTHandler::ClientMaxMTU, 0);
                                    if( gatt.readCharacteristicValue(serviceChar, value) ) {
                                        fprintf(stderr, "  [%2.2d.%2.2d] Value: %s\n", (int)i, (int)j, value.toString().c_str());
                                    }
                                }
                                if( nullptr != serviceChar.config ) {
                                    const bool enableNotification = serviceChar.hasProperties(GATTCharacterisicsDecl::PropertyBitVal::Notify);
                                    const bool enableIndication = serviceChar.hasProperties(GATTCharacterisicsDecl::PropertyBitVal::Indicate);
                                    if( enableNotification || enableIndication ) {
                                        bool res = gatt.configIndicationNotification(*serviceChar.config, enableNotification, enableIndication);
                                        fprintf(stderr, "  [%2.2d.%2.2d] Config Notification(%d), Indication(%d): Result %d\n",
                                                (int)i, (int)j, enableNotification, enableIndication, res);
                                    }
                                }
                            }
#ifdef SCAN_CHARACTERISTIC_DESCRIPTORS                            
                            fprintf(stderr, "  [%2.2d] Service Characteristics Descriptors\n", (int)i);
                            std::vector<GATTUUIDHandle> serviceDescriptors = servicesCharacteristicDescriptors.at(i);
                            for(size_t j=0; j<serviceDescriptors.size(); j++) {
                                fprintf(stderr, "  [%2.2d.%2.2d] %s\n", (int)i, (int)j, serviceDescriptors.at(j).toString().c_str());
                            }
#endif                            
                        }
                        // FIXME sleep 2s for potential callbacks ..
                        sleep(2);
                        gatt.disconnect();
                    } else {
                        fprintf(stderr, "GATT connect failed: %s\n", gatt.getStateString().c_str());
                    }
                    if( 0 != hciLEConnHandle ) {
                        session->disconnect(0); // FIXME: hci_le_disconnect: Input/output error
                    }
                } // if( 2000 > lup )
            } // for(auto it = discoveredDevices.begin(); it != discoveredDevices.end(); it++)
            fprintf(stderr, "Connection: Got %d devices, tried connected to %d with %d succeeded\n", i, j, k);
        }
    }

#ifdef SHOW_STATIC_SERVICE_CHARACTERISTIC_COMPOSITION
    //
    // Show static composition of Services and Characteristics
    //
    for(size_t i=0; i<GATT_SERVICES.size(); i++) {
        const GattServiceCharacteristic * gsc = GATT_SERVICES.at(i);
        fprintf(stderr, "GattServiceCharacteristic %d: %s\n", (int)i, gsc->toString().c_str());
    }
#endif /* SHOW_STATIC_SERVICE_CHARACTERISTIC_COMPOSITION */

out:
    if( nullptr != session ) {
        session->close();
    }
    return 0;
}

