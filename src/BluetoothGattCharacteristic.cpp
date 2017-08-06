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
#include "BluetoothNotificationHandler.hpp"
#include "BluetoothGattCharacteristic.hpp"
#include "BluetoothGattService.hpp"
#include "BluetoothGattDescriptor.hpp"
#include "BluetoothException.hpp"

using namespace tinyb;

void BluetoothNotificationHandler::on_properties_changed_characteristic(GDBusProxy *proxy, GVariant *changed_properties, GStrv invalidated_properties, gpointer userdata) {

    auto c = static_cast<BluetoothGattCharacteristic*>(userdata);

    if(g_variant_n_children(changed_properties) > 0) {
        GVariantIter *iter = NULL;

        GVariant *value;
        const gchar *key;
        g_variant_get(changed_properties, "a{sv}", &iter);
        while (iter != nullptr && g_variant_iter_loop(iter, "{&sv}", &key, &value)) {
            auto value_callback = c->value_changed_callback;
            if (value_callback != nullptr && g_ascii_strncasecmp(key, "value", 5) == 0) {
                std::vector<unsigned char> new_value = from_iter_to_vector(value);
                value_callback(new_value);
            }
        }
        g_variant_iter_free (iter);
    }
}

std::string BluetoothGattCharacteristic::get_class_name() const
{
    return std::string("BluetoothGattCharacteristic");
}

std::string BluetoothGattCharacteristic::get_java_class() const
{
    return std::string(JAVA_PACKAGE "/BluetoothGattCharacteristic");
}

std::string BluetoothGattCharacteristic::get_object_path() const
{
    return std::string(g_dbus_proxy_get_object_path(G_DBUS_PROXY(object)));
}

BluetoothType BluetoothGattCharacteristic::get_bluetooth_type() const
{
    return BluetoothType::GATT_CHARACTERISTIC;
}

BluetoothGattCharacteristic::BluetoothGattCharacteristic(GattCharacteristic1 *object)
{
    this->object = object;
    g_object_ref(object);

    g_signal_connect(G_DBUS_PROXY(object), "g-properties-changed",
        G_CALLBACK(BluetoothNotificationHandler::on_properties_changed_characteristic), this);
}

BluetoothGattCharacteristic::BluetoothGattCharacteristic(const BluetoothGattCharacteristic &object)
{
    BluetoothGattCharacteristic(object.object);

}

BluetoothGattCharacteristic::~BluetoothGattCharacteristic()
{
    g_signal_handlers_disconnect_by_data(object, this);
    g_object_unref(object);
}

BluetoothGattCharacteristic *BluetoothGattCharacteristic::clone() const
{
    return new BluetoothGattCharacteristic(object);
}

std::unique_ptr<BluetoothGattCharacteristic> BluetoothGattCharacteristic::make(
    Object *object, BluetoothType type, std::string *name,
    std::string *identifier, BluetoothObject *parent)
{
    GattCharacteristic1 *characteristic;
    if((type == BluetoothType::NONE || type == BluetoothType::GATT_CHARACTERISTIC) &&
        (characteristic = object_get_gatt_characteristic1(object)) != NULL) {

        std::unique_ptr<BluetoothGattCharacteristic> p(
            new BluetoothGattCharacteristic(characteristic));
        g_object_unref(characteristic);

        if ((name == nullptr) &&
            (identifier == nullptr || *identifier == p->get_uuid()) &&
            (parent == nullptr || *parent == p->get_service()))
            return p;
    }

    return std::unique_ptr<BluetoothGattCharacteristic>();
}

/* D-Bus method calls: */
std::vector<unsigned char> BluetoothGattCharacteristic::read_value (uint16_t offset)
{
    GError *error = NULL;
    GBytes *result_gbytes;

    GVariantDict dict;
    g_variant_dict_init(&dict, NULL);

    if (offset != 0)
        g_variant_dict_insert_value(&dict, "offset", g_variant_new_uint16(offset));

    GVariant *variant = g_variant_dict_end(&dict);

    gatt_characteristic1_call_read_value_sync(
        object,
        &result_gbytes,
        variant,
        NULL,
        &error
    );

    handle_error(error);

    std::vector<unsigned char> result = from_gbytes_to_vector(result_gbytes);

    /* free the gbytes array */
    g_bytes_unref(result_gbytes);

    return result;
}

bool BluetoothGattCharacteristic::write_value (
    const std::vector<unsigned char> &arg_value, uint16_t offset)
{
    GError *error = NULL;
    bool result = true;

    GBytes *arg_value_gbytes = from_vector_to_gbytes(arg_value);

    GVariantDict dict;
    g_variant_dict_init(&dict, NULL);

    if (offset != 0)
        g_variant_dict_insert_value(&dict, "offset", g_variant_new_uint16(offset));

    GVariant *variant = g_variant_dict_end(&dict);

    result = gatt_characteristic1_call_write_value_sync(
        object,
        arg_value_gbytes,
        variant,
        NULL,
        &error
    );

    handle_error(error);

    /* freeing the GBytes allocated inside from_vector_to_gbytes function */
    g_bytes_unref(arg_value_gbytes);

    return result;
}

bool BluetoothGattCharacteristic::enable_value_notifications(
    std::function<void(BluetoothGattCharacteristic &, std::vector<unsigned char> &,void *)> callback,
    void *userdata)
{
    value_changed_callback = std::bind(callback, std::ref(*this), std::placeholders::_1, userdata);
    start_notify();
    return true;
}

bool BluetoothGattCharacteristic::enable_value_notifications(
    std::function<void(std::vector<unsigned char> &)> callback)
{
    value_changed_callback = callback;
    start_notify();
    return true;
}

bool BluetoothGattCharacteristic::disable_value_notifications()
{
    stop_notify();
    value_changed_callback = nullptr;
    return true;
}


bool BluetoothGattCharacteristic::start_notify ()
{
    GError *error = NULL;
    bool result;
    result = gatt_characteristic1_call_start_notify_sync(
        object,
        NULL,
        &error
    );

    handle_error(error);
    return result;
}

bool BluetoothGattCharacteristic::stop_notify ()
{
    GError *error = NULL;
    bool result;
    result = gatt_characteristic1_call_stop_notify_sync(
        object,
        NULL,
        &error
    );
    handle_error(error);
    return result;
}

/* D-Bus property accessors: */
std::string BluetoothGattCharacteristic::get_uuid ()
{
    return std::string(gatt_characteristic1_get_uuid (object));
}

BluetoothGattService BluetoothGattCharacteristic::get_service ()
{
    GError *error = NULL;

    GattService1 *service = gatt_service1_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE,
        "org.bluez",
        gatt_characteristic1_get_service (object),
        NULL,
        &error);

    if (service == nullptr) {
        std::string error_msg("Error occured while instantiating service: ");
        error_msg += error->message;
        g_error_free(error);
        throw BluetoothException(error_msg);
    }

    auto res = BluetoothGattService(service);
    g_object_unref(service);
    return res;
}

std::vector<unsigned char> BluetoothGattCharacteristic::get_value ()
{
    GBytes *value_gbytes = const_cast<GBytes *>(gatt_characteristic1_get_value (object));
    std::vector<unsigned char> result;

    try {
        result = from_gbytes_to_vector(value_gbytes);
    } catch (std::exception &e) {
        g_bytes_unref(value_gbytes);
        throw e;
    }

    g_bytes_unref(value_gbytes);

    return result;
}

bool BluetoothGattCharacteristic::get_notifying ()
{
    return gatt_characteristic1_get_notifying (object);
}

std::vector<std::string> BluetoothGattCharacteristic::get_flags ()
{
    const char * const *flags_c = gatt_characteristic1_get_flags (object);
    std::vector<std::string> flags;
    for (int i = 0; flags_c[i] != NULL ;i++)
        flags.push_back(std::string(flags_c[i]));
    return flags;

}

std::vector<std::unique_ptr<BluetoothGattDescriptor>> BluetoothGattCharacteristic::get_descriptors ()
{
    std::vector<std::unique_ptr<BluetoothGattDescriptor>> vector;
    GList *l, *objects = g_dbus_object_manager_get_objects(gdbus_manager);

    for (l = objects; l != NULL; l = l->next) {
        Object *object = OBJECT(l->data);

        auto p = BluetoothGattDescriptor::make(object,
            BluetoothType::GATT_DESCRIPTOR, NULL, NULL, this);
        if (p != nullptr)
            vector.push_back(std::move(p));
    }
    g_list_free_full(objects, g_object_unref);

    return vector;
}

