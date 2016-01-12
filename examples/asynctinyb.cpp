/*
 * Author: Petre Eftime <petre.p.eftime@intel.com>
 * Copyright (c) 2015 Intel Corporation.
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

#include <vector>
#include <iostream>
#include <thread>

using namespace tinyb;

/** Converts a raw temperature read from the sensor to a Celsius value.
 * @param[in] raw_temp The temperature read from the sensor (two bytes)
 * @return The Celsius value of the temperature
 */
static float celsius_temp(uint16_t raw_temp)
{
    const float SCALE_LSB = 0.03125;
    return ((float)(raw_temp >> 2)) * SCALE_LSB;
}

/** This program reads the temperature from a
 * TI Sensor Tag(http://www.ti.com/ww/en/wireless_connectivity/sensortag2015/?INTC=SensorTag&HQS=sensortag)
 * Pass the MAC address of the sensor as the first parameter of the program.
 */
int main(int argc, char **argv)
{
    BluetoothManager *manager = BluetoothManager::get_bluetooth_manager();

    if (argc < 2) {
        std::cerr << "Run as: " << argv[0] << " <device_address>" << std::endl;
        exit(1);
    }

    /* Start the discovery of devices */
    bool ret = manager->start_discovery();
    std::cout << "Started = " << (ret ? "true" : "false") << std::endl;

    std::unique_ptr<BluetoothGattService> temperature_service;

    std::string device_mac(argv[1]);
    auto sensor_tag = manager->find<BluetoothDevice>(nullptr, &device_mac, nullptr, std::chrono::seconds(10));
    if (sensor_tag == nullptr) {
        std::cout << "Device not found" << std::endl;
        return 1;
    }

//    for (int i = 0; i < 15; ++i) {
//        std::cout << "Discovered devices: " << std::endl;
//        /* Get the list of devices */
//        auto list = manager->get_devices();
//
//        for (auto it = list.begin(); it != list.end(); ++it) {
//
//            std::cout << "Class = " << (*it)->get_class_name() << " ";
//            std::cout << "Path = " << (*it)->get_object_path() << " ";
//            std::cout << "Name = " << (*it)->get_name() << " ";
//            std::cout << "Connected = " << (*it)->get_connected() << " ";
//            std::cout << std::endl;
//
//            /* Search for the device with the address given as a parameter to the program */
//            if ((*it)->get_address() == argv[1])
//                sensor_tag = (*it).release();
//        }
//
//        /* Free the list of devices and stop if the device was found */
//        if (sensor_tag != NULL)
//            break;
//        /* If not, wait and try again */
//        std::this_thread::sleep_for(std::chrono::seconds(4));
//        std::cout << std::endl;
//    }

    /* Stop the discovery (the device was found or number of tries ran out */
    //ret = manager->stop_discovery();
    std::cout << "Stopped = " << (ret ? "true" : "false") << std::endl;

    
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (sensor_tag != nullptr) {
        /* Connect to the device and get the list of services exposed by it */
        sensor_tag->connect();
        std::string service_uuid("f000aa00-0451-4000-b000-000000000000");
        std::cout << "Waiting for service " << service_uuid << "to be discovered" << std::endl; 
        temperature_service =
            manager->find<BluetoothGattService>(nullptr, &service_uuid, sensor_tag.get());
    } else {
       std::cerr << "SensorTag not found after 30 seconds, exiting" << std::endl;
       return 1;
    }


    auto value_uuid = std::string("f000aa01-0451-4000-b000-000000000000");
    auto temp_value = 
        manager->find<BluetoothGattCharacteristic>(nullptr,
        &value_uuid,
        temperature_service.get());

    auto config_uuid = std::string("f000aa02-0451-4000-b000-000000000000");
    auto temp_config =
        manager->find<BluetoothGattCharacteristic>(nullptr,
        &config_uuid,
        temperature_service.get());

    auto period_uuid = std::string("f000aa03-0451-4000-b000-000000000000");
    auto temp_period =
        manager->find<BluetoothGattCharacteristic>(nullptr,
        &period_uuid,
        temperature_service.get());

    /* Activate the temperature measurements */
    std::vector<unsigned char> config_on {0x01};
    temp_config->write_value(config_on);
    while (true) {
        /* Read temperature data and display it */
        std::vector<unsigned char> response = temp_value->read_value();
        unsigned char *data;
        unsigned int size = response.size();
        if (size > 0) {
            data = response.data();

            std::cout << "Raw data=[";
            for (unsigned i = 0; i < response.size(); i++)
                std::cout << std::hex << static_cast<int>(data[i]) << ", ";
            std::cout << "] ";

            uint16_t ambient_temp, object_temp;
            object_temp = data[0] + (data[1] << 8);
            ambient_temp = data[2] + (data[3] << 8);

            std::cout << "Ambient temp: " << celsius_temp(ambient_temp) << "C ";
            std::cout << "Object temp: " << celsius_temp(object_temp) << "C ";
            std::cout << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    /* Disconnect from the device */
    if (sensor_tag != NULL)
        sensor_tag->disconnect();
}
