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
#include "BluetoothAdapter.hpp"
#include "BluetoothDevice.hpp"
#include "BluetoothManager.hpp"
#include "BluetoothException.hpp"

using namespace tinyb;

void BluetoothNotificationHandler::on_properties_changed_adapter(GDBusProxy *proxy, GVariant *changed_properties, GStrv invalidated_properties, gpointer userdata) {

    auto c = static_cast<BluetoothAdapter*>(userdata);

    if (!c->lock())
        return;

    if(g_variant_n_children(changed_properties) > 0) {
        GVariantIter *iter = NULL;

        GVariant *value;
        const gchar *key;
        g_variant_get(changed_properties, "a{sv}", &iter);
        while (iter != nullptr && g_variant_iter_loop(iter, "{&sv}", &key, &value)) {
            auto powered_callback = c->powered_callback;
            if (powered_callback != nullptr && g_ascii_strncasecmp(key, "powered", 8) == 0) {
                bool new_value;
                g_variant_get(value, "b", &new_value);
                powered_callback(new_value);
                continue;
            }
            auto discoverable_callback = c->discoverable_callback;
            if (discoverable_callback != nullptr && g_ascii_strncasecmp(key, "discoverable", 13) == 0) {
                bool new_value;
                g_variant_get(value, "b", &new_value);
                discoverable_callback(new_value);
                continue;
            }
            auto pairable_callback = c->pairable_callback;
            if (pairable_callback != nullptr && g_ascii_strncasecmp(key, "pairable", 9) == 0) {
                bool new_value;
                g_variant_get(value, "b", &new_value);
                pairable_callback(new_value);
                continue;
            }
            auto discovering_callback = c->discovering_callback;
            if (discovering_callback != nullptr && g_ascii_strncasecmp(key, "discovering", 12) == 0) {
                bool new_value;
                g_variant_get(value, "b", &new_value);
                discovering_callback(new_value);
                continue;
            }
        }
        g_variant_iter_free (iter);
    }

    c->unlock();
}

std::string BluetoothAdapter::get_class_name() const
{
    return std::string("BluetoothAdapter");
}

std::string BluetoothAdapter::get_java_class() const
{
    return std::string(JAVA_PACKAGE "/BluetoothAdapter");
}

std::string BluetoothAdapter::get_object_path() const
{
    return std::string(g_dbus_proxy_get_object_path(G_DBUS_PROXY(object)));
}

BluetoothType BluetoothAdapter::get_bluetooth_type() const
{
    return BluetoothType::ADAPTER;
}

BluetoothAdapter::BluetoothAdapter(Adapter1 *object)
{
    this->object = object;
    g_object_ref(object);

    g_signal_connect(G_DBUS_PROXY(object), "g-properties-changed",
        G_CALLBACK(BluetoothNotificationHandler::on_properties_changed_adapter), this);
    valid = true;
}

BluetoothAdapter::BluetoothAdapter(const BluetoothAdapter &object)
{
    BluetoothAdapter(object.object);
}

BluetoothAdapter *BluetoothAdapter::clone() const
{
    return new BluetoothAdapter(object);
}

BluetoothAdapter::~BluetoothAdapter()
{
    valid = false;
    g_signal_handlers_disconnect_by_data(object, this);
    lk.lock();

    g_object_unref(object);
}

std::unique_ptr<BluetoothAdapter> BluetoothAdapter::make(Object *object,
        BluetoothType type, std::string *name, std::string *identifier,
        BluetoothObject *parent)
{
    Adapter1 *adapter;
    if((type == BluetoothType::NONE || type == BluetoothType::ADAPTER) &&
        (adapter = object_get_adapter1(object)) != NULL) {

        std::unique_ptr<BluetoothAdapter> p(new BluetoothAdapter(adapter));
        g_object_unref(adapter);

        if ((name == nullptr || *name == p->get_name()) &&
            (identifier == nullptr || *identifier == p->get_address()) &&
            (parent == nullptr))
            return p;
    }

    return std::unique_ptr<BluetoothAdapter>();
}

std::vector<std::unique_ptr<BluetoothDevice>> BluetoothAdapter::get_devices()
{
    std::vector<std::unique_ptr<BluetoothDevice>> vector;
    GList *l, *objects = g_dbus_object_manager_get_objects(gdbus_manager);

    for (l = objects; l != NULL; l = l->next) {
        Object *object = OBJECT(l->data);

        auto p = BluetoothDevice::make(object,
            BluetoothType::DEVICE, NULL, NULL, this);
        if (p != nullptr)
            vector.push_back(std::move(p));
    }
    g_list_free_full(objects, g_object_unref);

    return vector;
}

/* D-Bus method calls: */
bool BluetoothAdapter::start_discovery ()
{
    GError *error = NULL;
    if (get_discovering() == true)
        return true;
    bool result = adapter1_call_start_discovery_sync(
        object,
        NULL,
        &error
    );
    handle_error(error);
    return result;
}

bool BluetoothAdapter::stop_discovery ()
{
    GError *error = NULL;
    if (get_discovering() == false)
        return true;
    bool result = adapter1_call_stop_discovery_sync(
        object,
        NULL,
        &error
    );
    handle_error(error);
    return result;
}

bool BluetoothAdapter::remove_device (
    const std::string &arg_device)
{
    GError *error = NULL;
    bool result = adapter1_call_remove_device_sync(
        object,
        arg_device.c_str(),
        NULL,
        &error
    );
    handle_error(error);
    return result;
}

bool BluetoothAdapter::set_discovery_filter (std::vector<BluetoothUUID> uuids,
    int16_t rssi, uint16_t pathloss, const TransportType &transport)
{
    GError *error = NULL;
    bool result = true;
    GVariantDict dict;
    g_variant_dict_init(&dict, NULL);

    if (uuids.size() > 0)
    {
        GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("as"));

        for (std::vector<BluetoothUUID>::iterator i = uuids.begin(); i != uuids.end(); ++i)
            g_variant_builder_add(builder, "s", (*i).get_string().c_str());

        GVariant *uuids_variant = g_variant_new("as", builder);
        g_variant_builder_unref(builder);
        g_variant_dict_insert_value(&dict, "UUIDs", uuids_variant);
    }

    if (rssi != 0)
        g_variant_dict_insert_value(&dict, "RSSI", g_variant_new_int16(rssi));

    if (pathloss != 0)
        g_variant_dict_insert_value(&dict, "Pathloss", g_variant_new_uint16(pathloss));

    std::string transport_str;

    if (transport == TransportType::AUTO)
        transport_str = "auto";
    else if (transport == TransportType::BREDR)
        transport_str = "bredr";
    else if (transport == TransportType::LE)
        transport_str = "le";

    if (!transport_str.empty())
        g_variant_dict_insert_value(&dict, "Transport", g_variant_new_string(transport_str.c_str()));

    GVariant *variant = g_variant_dict_end(&dict);

    result = adapter1_call_set_discovery_filter_sync(
        object,
        variant,
        NULL,
        &error
    );

    handle_error(error);
    return result;
}

/* D-Bus property accessors: */
std::string BluetoothAdapter::get_address ()
{
    return std::string(adapter1_get_address (object));
}

std::string BluetoothAdapter::get_name ()
{
    return std::string(adapter1_get_name (object));
}

std::string BluetoothAdapter::get_alias ()
{
    return std::string(adapter1_get_alias (object));
}

void BluetoothAdapter::set_alias (const std::string &value)
{
    adapter1_set_alias (object, value.c_str());
}

unsigned int BluetoothAdapter::get_class ()
{
    return adapter1_get_class (object);
}

bool BluetoothAdapter::get_powered ()
{
    return adapter1_get_powered (object);
}

void BluetoothAdapter::enable_powered_notifications(
        std::function<void(BluetoothAdapter &adapter, bool powered, void *userdata)> callback,
        void *userdata) {
    powered_callback = std::bind(callback, std::ref(*this), std::placeholders::_1, userdata);
}
void BluetoothAdapter::enable_powered_notifications(std::function<void(bool powered)> callback) {
    powered_callback = callback;
}
void BluetoothAdapter::disable_powered_notifications() {
    powered_callback = nullptr;
}

void BluetoothAdapter::set_powered (bool  value)
{
    if (get_powered() != value)
        adapter1_set_powered (object, value);
}

bool BluetoothAdapter::get_discoverable ()
{
    return adapter1_get_discoverable (object);
}

void BluetoothAdapter::set_discoverable (bool  value)
{
    adapter1_set_discoverable (object, value);
}

void BluetoothAdapter::enable_discoverable_notifications(
    std::function<void(BluetoothAdapter &adapter, bool discoverable, void *userdata)> callback,
    void *userdata) {
    discoverable_callback = std::bind(callback, std::ref(*this), std::placeholders::_1, userdata);
}
void BluetoothAdapter::enable_discoverable_notifications(std::function<void(bool discoverable)> callback) {
    discoverable_callback = callback;
}
void BluetoothAdapter::disable_discoverable_notifications() {
    discoverable_callback = nullptr;
}

unsigned int BluetoothAdapter::get_discoverable_timeout ()
{
    return adapter1_get_discoverable_timeout (object);
}

void BluetoothAdapter::set_discoverable_timeout (unsigned int  value)
{
    adapter1_set_discoverable_timeout (object, value);
}

bool BluetoothAdapter::get_pairable ()
{
    return adapter1_get_pairable (object);
}

void BluetoothAdapter::enable_pairable_notifications(
        std::function<void(BluetoothAdapter &adapter, bool pairable, void *userdata)> callback,
        void *userdata) {
    pairable_callback = std::bind(callback, std::ref(*this), std::placeholders::_1, userdata);
}
void BluetoothAdapter::enable_pairable_notifications(std::function<void(bool pairable)> callback) {
    pairable_callback = callback;
}
void BluetoothAdapter::disable_pairable_notifications() {
    pairable_callback = nullptr;
}

void BluetoothAdapter::set_pairable (bool  value)
{
    adapter1_set_pairable (object, value);
}

unsigned int BluetoothAdapter::get_pairable_timeout ()
{
    return adapter1_get_pairable_timeout (object);
}

void BluetoothAdapter::set_pairable_timeout (unsigned int  value)
{
    adapter1_set_pairable_timeout (object, value);
}

bool BluetoothAdapter::get_discovering ()
{
    return adapter1_get_discovering (object);
}

void BluetoothAdapter::enable_discovering_notifications(
        std::function<void(BluetoothAdapter &adapter, bool discovering, void *userdata)> callback,
        void *userdata) {
    discovering_callback = std::bind(callback, std::ref(*this), std::placeholders::_1, userdata);
}
void BluetoothAdapter::enable_discovering_notifications(std::function<void(bool discovering)> callback) {
    discovering_callback = callback;
}
void BluetoothAdapter::disable_discovering_notifications() {
    discovering_callback = nullptr;
}

std::vector<std::string> BluetoothAdapter::get_uuids ()
{
    const char * const *uuids_c = adapter1_get_uuids (object);
    std::vector<std::string> uuids;
    for (int i = 0; uuids_c[i] != NULL ;i++)
        uuids.push_back(std::string(uuids_c[i]));
    return uuids;
}

std::unique_ptr<std::string> BluetoothAdapter::get_modalias ()
{
    const gchar *modalias= adapter1_get_modalias (object);
    if (modalias == nullptr)
        return std::unique_ptr<std::string>();
    return std::unique_ptr<std::string>(new std::string(modalias));
}
