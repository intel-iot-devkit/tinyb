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
#include "BluetoothGattDescriptor.hpp"
#include "BluetoothGattCharacteristic.hpp"
#include "BluetoothException.hpp"

using namespace tinyb;

void BluetoothNotificationHandler::on_properties_changed_descriptor(GDBusProxy *proxy, GVariant *changed_properties, GStrv invalidated_properties, gpointer userdata) {

    auto c = static_cast<BluetoothGattDescriptor*>(userdata);

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

std::string BluetoothGattDescriptor::get_class_name() const
{
    return std::string("BluetoothGattDescriptor");
}

std::string BluetoothGattDescriptor::get_java_class() const
{
    return std::string(JAVA_PACKAGE "/BluetoothGattDescriptor");
}

std::string BluetoothGattDescriptor::get_object_path() const
{
    return std::string(g_dbus_proxy_get_object_path(G_DBUS_PROXY(object)));
}

BluetoothType BluetoothGattDescriptor::get_bluetooth_type() const
{
    return BluetoothType::GATT_DESCRIPTOR;
}

BluetoothGattDescriptor::BluetoothGattDescriptor(GattDescriptor1 *object)
{
    this->object = object;
    g_object_ref(object);

    g_signal_connect(G_DBUS_PROXY(object), "g-properties-changed",
        G_CALLBACK(BluetoothNotificationHandler::on_properties_changed_descriptor), this);
    valid = true;
}

BluetoothGattDescriptor::BluetoothGattDescriptor(const BluetoothGattDescriptor &object)
{
    BluetoothGattDescriptor(object.object);
}

BluetoothGattDescriptor::~BluetoothGattDescriptor()
{
    valid = false;
    g_signal_handlers_disconnect_by_data(object, this);
    lk.lock();

    g_object_unref(object);
}

std::unique_ptr<BluetoothGattDescriptor> BluetoothGattDescriptor::make(
    Object *object, BluetoothType type, std::string *name,
    std::string *identifier, BluetoothObject *parent)
{
    GattDescriptor1 *descriptor;
    if((type == BluetoothType::NONE || type == BluetoothType::GATT_DESCRIPTOR) &&
        (descriptor = object_get_gatt_descriptor1(object)) != NULL) {

        std::unique_ptr<BluetoothGattDescriptor> p(
            new BluetoothGattDescriptor(descriptor));
        g_object_unref(descriptor);

        if ((name == nullptr) &&
            (identifier == nullptr || *identifier == p->get_uuid()) &&
            (parent == nullptr || *parent == p->get_characteristic()))
            return p;
    }

    return std::unique_ptr<BluetoothGattDescriptor>();
}



BluetoothGattDescriptor *BluetoothGattDescriptor::clone() const
{
    return new BluetoothGattDescriptor(object);
}

/* D-Bus method calls: */
std::vector<unsigned char> BluetoothGattDescriptor::read_value (uint16_t offset)
{
    GError *error = NULL;
    GBytes *result_gbytes;

    GVariantDict dict;
    g_variant_dict_init(&dict, NULL);

    if (offset != 0)
        g_variant_dict_insert_value(&dict, "offset", g_variant_new_uint16(offset));

    GVariant *variant = g_variant_dict_end(&dict);

    gatt_descriptor1_call_read_value_sync(
        object,
        &result_gbytes,
        variant,
        NULL,
        &error
    );
    handle_error(error);

    std::vector<unsigned char> result = from_gbytes_to_vector(result_gbytes);

    /* unref the gbytes pointer */
    g_bytes_unref(result_gbytes);

    return result;
}

bool BluetoothGattDescriptor::write_value (
    const std::vector<unsigned char> &arg_value, uint16_t offset)
{
    GError *error = NULL;
    bool result;

    GBytes *arg_value_gbytes = from_vector_to_gbytes(arg_value);

    GVariantDict dict;
    g_variant_dict_init(&dict, NULL);

    if (offset != 0)
        g_variant_dict_insert_value(&dict, "offset", g_variant_new_uint16(offset));

    GVariant *variant = g_variant_dict_end(&dict);

    result = gatt_descriptor1_call_write_value_sync(
        object,
        arg_value_gbytes,
        variant,
        NULL,
        &error
    );
    handle_error(error);

    /* unref the GBytes allocated inside from_vector_to_gbytes function */
    g_bytes_unref(arg_value_gbytes);

    return result;
}

bool BluetoothGattDescriptor::enable_value_notifications(
    std::function<void(BluetoothGattDescriptor &, std::vector<unsigned char> &,void *)> callback,
    void *userdata)
{
    value_changed_callback = std::bind(callback, std::ref(*this), std::placeholders::_1, userdata);
    return true;
}

bool BluetoothGattDescriptor::enable_value_notifications(
    std::function<void(std::vector<unsigned char> &)> callback)
{
    value_changed_callback = callback;
    return true;
}

bool BluetoothGattDescriptor::disable_value_notifications()
{
    value_changed_callback = nullptr;
    return true;
}

/* D-Bus property accessors: */
std::string BluetoothGattDescriptor::get_uuid ()
{
    return std::string(gatt_descriptor1_get_uuid (object));
}

BluetoothGattCharacteristic BluetoothGattDescriptor::get_characteristic ()
{
    GError *error = NULL;

    GattCharacteristic1* characteristic = gatt_characteristic1_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE,
        "org.bluez",
        gatt_descriptor1_get_characteristic (object),
        NULL,
        &error);

    if (characteristic == NULL) {
        std::string error_msg("Error occured while instantiating characteristic: ");
        error_msg += error->message;
        g_error_free(error);
        throw BluetoothException(error_msg);
    }

    auto res = BluetoothGattCharacteristic(characteristic);
    g_object_unref(characteristic);
    return res;
}

std::vector<unsigned char> BluetoothGattDescriptor::get_value ()
{
    GBytes *value_gbytes = const_cast<GBytes *>(gatt_descriptor1_get_value (object));
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
