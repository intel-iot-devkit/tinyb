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

#include <tinyb_hci/HCITypes.hpp>

class DeviceDiscoveryListener : public tinyb_hci::HCIDeviceDiscoveryListener {
    void deviceAdded(tinyb_hci::HCIAdapter const &a, std::shared_ptr<tinyb_hci::HCIDevice> device) {
        fprintf(stderr, "****** ADDED__: %s\n", device->toString().c_str());
        fprintf(stderr, "Status HCIAdapter:\n");
        fprintf(stderr, "%s\n", a.toString().c_str());
    }
    void deviceUpdated(tinyb_hci::HCIAdapter const &a, std::shared_ptr<tinyb_hci::HCIDevice> device) {
        fprintf(stderr, "****** UPDATED: %s\n", device->toString().c_str());
        fprintf(stderr, "Status HCIAdapter:\n");
        fprintf(stderr, "%s\n", a.toString().c_str());
    }
};

int main(int argc, char *argv[])
{
    bool ok = true;

    tinyb_hci::HCIAdapter adapter; // default
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

    adapter.setDeviceDiscoveryListener(std::shared_ptr<tinyb_hci::HCIDeviceDiscoveryListener>(new DeviceDiscoveryListener()));

    std::shared_ptr<tinyb_hci::HCISession> session = adapter.open();

    while( ok && nullptr != session ) {
        ok = adapter.startDiscovery(*session);
        if( !ok) {
            fprintf(stderr, "Adapter start discovery failed.\n");
            goto out;
        }

        if( !adapter.discoverDevices(*session, 1000) ) {
            fprintf(stderr, "Adapter discovery failed.\n");
            ok = false;
        }

        if( !adapter.stopDiscovery(*session) ) {
            fprintf(stderr, "Adapter stop discovery failed.\n");
            ok = false;
        }

        if( ok ) {
            const uint64_t t0 = tinyb_hci::getCurrentMilliseconds();
            std::vector<std::shared_ptr<tinyb_hci::HCIDevice>> discoveredDevices = adapter.getDevices();
            int i=0, j=0, k=0;
            for(auto it = discoveredDevices.begin(); it != discoveredDevices.end(); it++) {
                i++;
                std::shared_ptr<tinyb_hci::HCIDevice> p = *it;
                const uint64_t lup = p->getLastUpdateAge(t0);
                if( 2000 > lup && p->hasName() ) {
                    // less than 2s old ..
                    j++;
                    const uint16_t handle = adapter.le_connect(*session, p->getAddress());
                    if( 0 == handle ) {
                        fprintf(stderr, "Connection: Failed %s\n", p->toString().c_str());
                    } else {
                        const uint64_t td = tinyb_hci::getCurrentMilliseconds() - t0;
                        fprintf(stderr, "Connection: Success in %d ms, handle 0x%X made to %s\n", td, handle, p->toString().c_str());
                        k++;
                    }
                }
            }
            fprintf(stderr, "Connection: Got %d devices, tried connected to %d with %d succeeded\n", i, j, k);
        }
    }

out:
    if( nullptr != session ) {
        session->close();
    }
    return 0;
}

