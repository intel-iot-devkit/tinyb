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

#include "generated-code.h"
#include "tinyb_utils.hpp"
#include "BluetoothGattService.hpp"
#include "BluetoothGattCharacteristic.hpp"
#include "BluetoothDevice.hpp"
#include "BluetoothException.hpp"

using namespace tinyb;

std::string BluetoothGattService::get_class_name() const
{
    return std::string("BluetoothGattService");
}

std::string BluetoothGattService::get_java_class() const
{
    return std::string(JAVA_PACKAGE "/BluetoothGattService");
}

std::string BluetoothGattService::get_object_path() const
{
    return std::string(g_dbus_proxy_get_object_path(G_DBUS_PROXY(object)));
}

BluetoothType BluetoothGattService::get_bluetooth_type() const
{
    return BluetoothType::GATT_SERVICE;
}

BluetoothGattService::BluetoothGattService(GattService1 *object)
{
    this->object = object;
    g_object_ref(object);
}

BluetoothGattService::BluetoothGattService(const BluetoothGattService &object)
{
    BluetoothGattService(object.object);
}

BluetoothGattService::~BluetoothGattService()
{
    g_object_unref(object);
}

std::unique_ptr<BluetoothGattService> BluetoothGattService::make(
    Object *object, BluetoothType type, std::string *name,
    std::string *identifier, BluetoothObject *parent)
{
    GattService1 *service;
    if((type == BluetoothType::NONE || type == BluetoothType::GATT_SERVICE) &&
        (service = object_get_gatt_service1(object)) != NULL) {

        std::unique_ptr<BluetoothGattService> p(
            new BluetoothGattService(service));
        g_object_unref(service);

        if ((name == nullptr) &&
            (identifier == nullptr || *identifier == p->get_uuid()) &&
            (parent == nullptr || *parent == p->get_device()))
            return p;
    }

    return std::unique_ptr<BluetoothGattService>();
}

BluetoothGattService *BluetoothGattService::clone() const
{
    return new BluetoothGattService(object);
}

/* D-Bus property accessors: */
std::string BluetoothGattService::get_uuid ()
{
    return std::string(gatt_service1_get_uuid (object));
}

BluetoothDevice BluetoothGattService::get_device ()
{
    GError *error = NULL;

    Device1 *device = device1_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE,
        "org.bluez",
        gatt_service1_get_device (object),
        NULL,
        &error);

    if (device == nullptr) {
        std::string error_msg("Error occured while instantiating device: ");
        error_msg += error->message;
        g_error_free(error);
        throw BluetoothException(error_msg);
    }

    auto res = BluetoothDevice(device);
    g_object_unref(device);
    return res;
}

bool BluetoothGattService::get_primary ()
{
    return gatt_service1_get_primary (object);
}

std::vector<std::unique_ptr<BluetoothGattCharacteristic>> BluetoothGattService::get_characteristics ()
{
    std::vector<std::unique_ptr<BluetoothGattCharacteristic>> vector;
    GList *l, *objects = g_dbus_object_manager_get_objects(gdbus_manager);

    for (l = objects; l != NULL; l = l->next) {
        Object *object = OBJECT(l->data);

        auto p = BluetoothGattCharacteristic::make(object,
            BluetoothType::GATT_CHARACTERISTIC, NULL, NULL, this);
        if (p != nullptr)
            vector.push_back(std::move(p));
    }
    g_list_free_full(objects, g_object_unref);

    return vector;
}

