/*
 * Author: Henry Bruce <henry.bruce@intel.com>
 * Copyright (c) 2016 Intel Corporation.
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

#include <tinyb.hpp>
#include <tinyb/BluetoothException.hpp>
#include <vector>
#include <iostream>
#include <iomanip>
#include <thread>
#include <signal.h>

using namespace tinyb;

const std::string BT_UUID_ESS         = "0000181a-0000-1000-8000-00805f9b34fb";
const std::string BT_UUID_TEMPERATURE = "00002a6e-0000-1000-8000-00805f9b34fb";

static bool interrupted = false;

static void signal_handler(int signal_value)
{
    interrupted = true;
}

static void add_signal_handler()
{
    struct sigaction action;
    action.sa_handler = signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT, &action, NULL);
}

/** This program reads the temperature from a device running the Environmental Sensing Senvice.
 */
int main(int argc, char **argv)
{
    BluetoothManager *manager = nullptr;
    try {
        manager = BluetoothManager::get_bluetooth_manager();
    } catch(const std::runtime_error& e) {
        std::cerr << "Error while initializing libtinyb: " << e.what() << std::endl;
        exit(1);
    }

    std::unique_ptr<BluetoothAdapter> adapter = manager->get_default_adapter();
    BluetoothDevice *ess_device = NULL;

    // Start the discovery of devices */
    bool ret = manager->start_discovery();
    if (!ret) {
        std::cout << "Failed to start discovery" << std::endl;
        return 1;
    }
    std::cout << "Discovering BLE devices";
    int num_devices = 0;
    bool no_new_devices = false;
    std::vector<std::unique_ptr<BluetoothDevice>> list_devices;
    while (ess_device == NULL && !no_new_devices) {
        list_devices = manager->get_devices();
        std::cout << "." << std::flush;
        if (list_devices.size() > 0 && list_devices.size() == num_devices)
            no_new_devices = true;
        num_devices = list_devices.size();

        // Look for active ESS device
        for (auto it = list_devices.begin(); it != list_devices.end(); ++it) {
            if ((*it)->get_rssi() != 0) {
                auto list_uuids = (*it)->get_uuids();
                for (auto uuit = list_uuids.begin(); uuit != list_uuids.end(); ++uuit) {
                    if (*(uuit) == BT_UUID_ESS) {
                        ess_device = (*it).release();
                        break;
                    }
                }
            }
        }

        if (ess_device == NULL)
            std::this_thread::sleep_for(std::chrono::seconds(4));
    }
    ret = manager->stop_discovery();
    std::cout << std::endl;

    // Now try to connect
    if (ess_device != NULL) {
        std::cout << "Connecting to " << ess_device->get_name() << " with addr " << ess_device->get_address() << std::flush;
        try {
            if (ess_device->connect()) {
                std::cout << ". Connected" << std::endl;
            }
            else
                std::cout << ". Failed" << std::endl;
        } catch (BluetoothException& e) {
            std::cout  << std::endl << e.what() << std::endl;
            return 1;
        }
    } else {
        std::cout << "ESS device not found." << std::endl;
        delete ess_device;
        return 1;
    }

    if (!ess_device->get_connected()) {
        delete ess_device;
        return 1;
    }

    std::cout << "Getting environmental service" << std::endl;
    std::unique_ptr<BluetoothGattService> environmental_service = ess_device->find(const_cast<std::string*>(&BT_UUID_ESS));
    std::cout << "Getting temperature characteristic" << std::endl;
    BluetoothGattCharacteristic *temp_characteristic = environmental_service->find(const_cast<std::string*>(&BT_UUID_TEMPERATURE)).release();

    if (temp_characteristic != NULL) {
        std::cout << "Starting temperature readings" << std::endl;
        bool bt_error = false;
        /* Activate the temperature measurements by enabling notifications */
        add_signal_handler();
        while (!bt_error && !interrupted) {
            /* Read temperature data and display it */
            try {
                std::vector<unsigned char> response = temp_characteristic->read_value();
                unsigned int size = response.size();
                if (size == 2) {
                    int16_t* data = reinterpret_cast<int16_t*>(response.data());
                    std::cout << "Raw data = " << std::hex << std::setfill('0') << std::setw(4) << *data  << ". ";
                    uint16_t ambient_temp = (*data + 50) / 100;
                    std::cout << "Temperature = " << std::dec << ambient_temp << "C " << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            } catch (BluetoothException& e) {
                std::cout << "Read_value failed. " << e.what() << std::endl;
                bt_error = true;
            }
        }
        delete temp_characteristic;
    }

    /* Disconnect from the device */
    ess_device->disconnect();
    delete ess_device;
    return 0;
}
