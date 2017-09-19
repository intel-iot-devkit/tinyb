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
#include "BluetoothDevice.hpp"
#include "BluetoothGattService.hpp"
#include "BluetoothManager.hpp"
#include "BluetoothException.hpp"

using namespace tinyb;

void BluetoothNotificationHandler::on_properties_changed_device(GDBusProxy *proxy, GVariant *changed_properties, GStrv invalidated_properties, gpointer userdata) {

    auto c = static_cast<BluetoothDevice*>(userdata);

    if (!c->lock())
        return;

    if(g_variant_n_children(changed_properties) > 0) {
        GVariantIter *iter = NULL;

        GVariant *value;
        const gchar *key;
        g_variant_get(changed_properties, "a{sv}", &iter);
        while (iter != nullptr && g_variant_iter_loop(iter, "{&sv}", &key, &value)) {
            auto rssi_callback = c->rssi_callback;
            if (rssi_callback != nullptr && g_ascii_strncasecmp(key, "rssi", 5) == 0) {
                int16_t new_value;
                g_variant_get(value, "n", &new_value);
                rssi_callback(new_value);
                continue;
            }
            auto blocked_callback = c->blocked_callback;
            if (blocked_callback != nullptr && g_ascii_strncasecmp(key, "blocked", 8) == 0) {
                bool new_value;
                g_variant_get(value, "b", &new_value);
                blocked_callback(new_value);
                continue;
            }
            auto trusted_callback = c->trusted_callback;
            if (trusted_callback != nullptr && g_ascii_strncasecmp(key, "trusted", 8) == 0) {
                bool new_value;
                g_variant_get(value, "b", &new_value);
                trusted_callback(new_value);
                continue;
            }
            auto paired_callback = c->paired_callback;
            if (paired_callback != nullptr && g_ascii_strncasecmp(key, "paired", 7) == 0) {
                bool new_value;
                g_variant_get(value, "b", &new_value);
                paired_callback(new_value);
                continue;
            }
            auto connected_callback = c->connected_callback;
            if (connected_callback != nullptr && g_ascii_strncasecmp(key, "connected", 10) == 0) {
                bool new_value;
                g_variant_get(value, "b", &new_value);
                connected_callback(new_value);
                continue;
            }
            auto mfg_callback = c->mfg_callback;
            if (mfg_callback != nullptr && g_ascii_strncasecmp(key, "manufacturerdata", 16) == 0) {
                std::map<uint16_t, std::vector<uint8_t>> new_value;

                GVariantIter *iter;
                g_variant_get (value, "a{qv}", &iter);

                GVariant *array;
                uint16_t key;
                uint8_t val;

                while (g_variant_iter_loop(iter, "{qv}", &key, &array)) {
                    GVariantIter it_array;
                    g_variant_iter_init(&it_array, array);
                    while(g_variant_iter_loop(&it_array, "y", &val)) {
                        new_value[key].push_back(val);
                    }
                }

                g_variant_iter_free(iter);
                mfg_callback(new_value);
                continue;
            }
            auto service_callback = c->service_callback;
            if (service_callback != nullptr && g_ascii_strncasecmp(key, "servicedata", 11) == 0) {
                std::map<std::string, std::vector<uint8_t>> new_value;

                GVariantIter *iter;
                g_variant_get (value, "a{sv}", &iter);

                GVariant *array;
                const char* key;
                uint8_t val;

                while (g_variant_iter_loop(iter, "{sv}", &key, &array)) {
                    GVariantIter it_array;
                    g_variant_iter_init(&it_array, array);
                    while(g_variant_iter_loop(&it_array, "y", &val)) {
                        new_value[key].push_back(val);
                    }
                }

                g_variant_iter_free(iter);
                service_callback(new_value);
                continue;
            }
            auto services_resolved_callback = c->services_resolved_callback;
            if (services_resolved_callback != nullptr && g_ascii_strncasecmp(key, "servicesresolved", 16) == 0) {
                bool new_value;
                g_variant_get(value, "b", &new_value);
                services_resolved_callback(new_value);
                continue;
            }
        }
        g_variant_iter_free (iter);
    }

    c->unlock();
}

std::string BluetoothDevice::get_class_name() const
{
    return std::string("BluetoothDevice");
}

std::string BluetoothDevice::get_java_class() const
{
    return std::string(JAVA_PACKAGE "/BluetoothDevice");
}

std::string BluetoothDevice::get_object_path() const
{
    return std::string(g_dbus_proxy_get_object_path(G_DBUS_PROXY(object)));
}

BluetoothType BluetoothDevice::get_bluetooth_type() const
{
    return BluetoothType::DEVICE;
}

BluetoothDevice::BluetoothDevice(Device1 *object)
{
    this->object = object;
    g_object_ref(object);

    g_signal_connect(G_DBUS_PROXY(object), "g-properties-changed",
        G_CALLBACK(BluetoothNotificationHandler::on_properties_changed_device), this);
    valid = true;
}

BluetoothDevice::BluetoothDevice(const BluetoothDevice &object)
{
    BluetoothDevice(object.object);
}

BluetoothDevice::~BluetoothDevice()
{
    valid = false;
    g_signal_handlers_disconnect_by_data(object, this);
    lk.lock();

    g_object_unref(object);
}

std::unique_ptr<BluetoothDevice> BluetoothDevice::make(Object *object,
    BluetoothType type, std::string *name, std::string *identifier,
    BluetoothObject *parent)
{
    Device1 *device;
    if((type == BluetoothType::NONE || type == BluetoothType::DEVICE) &&
        (device = object_get_device1(object)) != NULL) {

        std::unique_ptr<BluetoothDevice> p(new BluetoothDevice(device));
        g_object_unref(device);

        if ((name == nullptr || *name == p->get_name()) &&
            (identifier == nullptr || *identifier == p->get_address()) &&
            (parent == nullptr || *parent == p->get_adapter()))
            return p;
    }

    return std::unique_ptr<BluetoothDevice>();
}

BluetoothDevice *BluetoothDevice::clone() const
{
    return new BluetoothDevice(object);
}

std::vector<std::unique_ptr<BluetoothGattService>> BluetoothDevice::get_services()
{
    std::vector<std::unique_ptr<BluetoothGattService>> vector;
    GList *l, *objects = g_dbus_object_manager_get_objects(gdbus_manager);

    for (l = objects; l != NULL; l = l->next) {
        Object *object = OBJECT(l->data);

        auto p = BluetoothGattService::make(object,
            BluetoothType::GATT_SERVICE, NULL, NULL, this);
        if (p != nullptr)
            vector.push_back(std::move(p));
    }
    g_list_free_full(objects, g_object_unref);

    return vector;
}

/* D-Bus method calls: */
bool BluetoothDevice::disconnect ()
{
    GError *error = NULL;
    bool result;
    result = device1_call_disconnect_sync(
        object,
        NULL,
        &error
    );
    handle_error(error);
    return result;
}

bool BluetoothDevice::connect ()
{
    GError *error = NULL;
    bool result;
    result = device1_call_connect_sync(
        object,
        NULL,
        &error
    );
    handle_error(error);
    return result;
}

bool BluetoothDevice::connect_profile (
    const std::string &arg_UUID)
{
    GError *error = NULL;
    bool result;
    result = device1_call_connect_profile_sync(
        object,
        arg_UUID.c_str(),
        NULL,
        &error
    );
    handle_error(error);
    return result;
}

bool BluetoothDevice::disconnect_profile (
    const std::string &arg_UUID)
{
    GError *error = NULL;
    bool result;
    result = device1_call_disconnect_profile_sync(
        object,
        arg_UUID.c_str(),
        NULL,
        &error
    );
    handle_error(error);
    return result;
}

bool BluetoothDevice::pair ()
{
    GError *error = NULL;
    bool result;
    result = device1_call_pair_sync(
        object,
        NULL,
        &error
    );
    handle_error(error);
    return result;
}

// Remove the device (like an unpair)
bool BluetoothDevice::remove_device(){
    BluetoothAdapter ba = get_adapter();
    return ba.remove_device(get_object_path());
}

bool BluetoothDevice::cancel_pairing ()
{
    GError *error = NULL;
    bool result;
    result = device1_call_cancel_pairing_sync(
        object,
        NULL,
        &error
    );
    handle_error(error);
    return result;
}



/* D-Bus property accessors: */
std::string BluetoothDevice::get_address ()
{
    return std::string(device1_get_address (object));
}

std::string BluetoothDevice::get_name ()
{
    const gchar *name = device1_get_name(object);
    if (name == nullptr)
        return std::string(device1_get_alias(object));
    return std::string(name);
}

std::string BluetoothDevice::get_alias ()
{
    return device1_get_alias (object);
}

void BluetoothDevice::set_alias (const std::string &value)
{
    device1_set_alias (object, value.c_str());
}

unsigned int BluetoothDevice::get_class ()
{
    return device1_get_class (object);
}

uint16_t BluetoothDevice::get_appearance ()
{
    return device1_get_appearance (object);
}

std::unique_ptr<std::string> BluetoothDevice::get_icon ()
{
    const gchar *icon = device1_get_icon (object);
    if (icon == nullptr)
        return std::unique_ptr<std::string>();
    return std::unique_ptr<std::string>(new std::string(icon));
}

bool BluetoothDevice::get_paired ()
{
    return device1_get_paired (object);
}

void BluetoothDevice::enable_paired_notifications(
    std::function<void(BluetoothDevice &, bool, void *)> callback,
    void *userdata) {
    paired_callback = std::bind(callback, std::ref(*this), std::placeholders::_1, userdata);
}
void BluetoothDevice::enable_paired_notifications(
    std::function<void(bool)> callback) {
    paired_callback = callback;
}
void BluetoothDevice::disable_paired_notifications() {
    paired_callback = nullptr;
}

bool BluetoothDevice::get_trusted ()
{
    return device1_get_trusted (object);
}

void BluetoothDevice::set_trusted (bool  value)
{
    device1_set_trusted (object, value);
}

void BluetoothDevice::enable_trusted_notifications(
    std::function<void(BluetoothDevice &, bool, void *)> callback,
    void *userdata) {
    trusted_callback = std::bind(callback, std::ref(*this), std::placeholders::_1, userdata);
}
void BluetoothDevice::enable_trusted_notifications(
    std::function<void(bool)> callback) {
    trusted_callback = callback;
}
void BluetoothDevice::disable_trusted_notifications() {
    trusted_callback = nullptr;
}

bool BluetoothDevice::get_blocked ()
{
    return device1_get_blocked (object);
}

void BluetoothDevice::set_blocked (bool  value)
{
    device1_set_blocked (object, value);
}

void BluetoothDevice::enable_blocked_notifications(
    std::function<void(BluetoothDevice &, bool, void *)> callback,
    void *userdata) {
    blocked_callback = std::bind(callback, std::ref(*this), std::placeholders::_1, userdata);
}
void BluetoothDevice::enable_blocked_notifications(
    std::function<void(bool)> callback) {
    blocked_callback = callback;
}
void BluetoothDevice::disable_blocked_notifications() {
    blocked_callback = nullptr;
}

bool BluetoothDevice::get_legacy_pairing ()
{
    return device1_get_legacy_pairing (object);
}

int16_t BluetoothDevice::get_rssi ()
{
    return device1_get_rssi (object);
}

void BluetoothDevice::enable_rssi_notifications(
    std::function<void(BluetoothDevice &, int16_t, void *)> callback,
    void *userdata) {
    rssi_callback = std::bind(callback, std::ref(*this), std::placeholders::_1, userdata);
}
void BluetoothDevice::enable_rssi_notifications(
    std::function<void(int16_t)> callback) {
    rssi_callback = callback;
}
void BluetoothDevice::disable_rssi_notifications() {
    rssi_callback = nullptr;
}

bool BluetoothDevice::get_connected ()
{
    return device1_get_connected (object);
}

void BluetoothDevice::enable_connected_notifications(
    std::function<void(BluetoothDevice &, bool, void *)> callback,
    void *userdata) {
    connected_callback = std::bind(callback, std::ref(*this), std::placeholders::_1, userdata);
}
void BluetoothDevice::enable_connected_notifications(
    std::function<void(bool)> callback) {
    connected_callback = callback;
}
void BluetoothDevice::disable_connected_notifications() {
    connected_callback = nullptr;
}

std::vector<std::string> BluetoothDevice::get_uuids ()
{

    const char * const *uuids_c = device1_get_uuids (object);
    std::vector<std::string> uuids;
    for (int i = 0; uuids_c[i] != NULL ;i++)
        uuids.push_back(std::string(uuids_c[i]));
    return uuids;
}

std::unique_ptr<std::string> BluetoothDevice::get_modalias ()
{
    const gchar *modalias= device1_get_modalias (object);
    if (modalias == nullptr)
        return std::unique_ptr<std::string>();
    return std::unique_ptr<std::string>(new std::string(modalias));
}

BluetoothAdapter BluetoothDevice::get_adapter ()
{
    GError *error = NULL;

    Adapter1 *adapter = adapter1_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE,
        "org.bluez",
        device1_get_adapter (object),
        NULL,
        &error);

   if (adapter == NULL) {
        std::string error_msg("Error occured while instantiating adapter: ");
        error_msg += error->message;
        g_error_free(error);
        throw BluetoothException(error_msg);
   }

   auto res = BluetoothAdapter(adapter);
   g_object_unref(adapter);
   return res;
}

std::map<uint16_t, std::vector<uint8_t>> BluetoothDevice::get_manufacturer_data()
{
    std::map<uint16_t, std::vector<uint8_t>> m_data;
    GVariant *v = device1_dup_manufacturer_data (object);

    if (v == nullptr)
        return m_data;

    GVariantIter *iter;
    g_variant_get (v, "a{qv}", &iter);

    GVariant *array;
    uint16_t key;
    uint8_t val;

    while (g_variant_iter_loop(iter, "{qv}", &key, &array)) {

        GVariantIter it_array;
        g_variant_iter_init(&it_array, array);
        while(g_variant_iter_loop(&it_array, "y", &val)) {
            m_data[key].push_back(val);
        }
    }

    g_variant_iter_free(iter);
    g_variant_unref(v);

    return m_data;
}

void BluetoothDevice::enable_manufacturer_data_notifications(
    std::function<void(BluetoothDevice &, std::map<uint16_t, std::vector<uint8_t>> &, void *)> callback,
    void *userdata) {
    mfg_callback = std::bind(callback, std::ref(*this), std::placeholders::_1, userdata);
}
void BluetoothDevice::enable_manufacturer_data_notifications(
    std::function<void(std::map<uint16_t, std::vector<uint8_t>> &)> callback) {
    mfg_callback = callback;
}
void BluetoothDevice::disable_manufacturer_data_notifications() {
    mfg_callback = nullptr;
}

std::map<std::string, std::vector<uint8_t>> BluetoothDevice::get_service_data()
{
    std::map<std::string, std::vector<uint8_t>> m_data;
    GVariant *v = device1_dup_service_data (object);

    if (v == nullptr)
        return m_data;

    GVariantIter *iter;
    g_variant_get (v, "a{sv}", &iter);

    GVariant *array;
    const char* key;
    uint8_t val;

    while (g_variant_iter_loop(iter, "{sv}", &key, &array)) {

        GVariantIter it_array;
        g_variant_iter_init(&it_array, array);
        while(g_variant_iter_loop(&it_array, "y", &val)) {
            m_data[key].push_back(val);
        }
    }

    g_variant_iter_free(iter);
    g_variant_unref(v);

    return m_data;
}

void BluetoothDevice::enable_service_data_notifications(
    std::function<void(BluetoothDevice &, std::map<std::string, std::vector<uint8_t>> &, void *)> callback,
    void *userdata) {
    service_callback = std::bind(callback, std::ref(*this), std::placeholders::_1, userdata);
}
void BluetoothDevice::enable_service_data_notifications(
    std::function<void(std::map<std::string, std::vector<uint8_t>> &)> callback) {
    service_callback = callback;
}
void BluetoothDevice::disable_service_data_notifications() {
    service_callback = nullptr;
}

int16_t BluetoothDevice::get_tx_power ()
{
    return device1_get_tx_power (object);
}

bool BluetoothDevice::get_services_resolved ()
{
    return device1_get_services_resolved (object);
}

void BluetoothDevice::enable_services_resolved_notifications(
    std::function<void(BluetoothDevice &, bool, void *)> callback,
    void *userdata) {
    services_resolved_callback = std::bind(callback, std::ref(*this), std::placeholders::_1, userdata);
}
void BluetoothDevice::enable_services_resolved_notifications(
    std::function<void(bool)> callback) {
    services_resolved_callback = callback;
}
void BluetoothDevice::disable_services_resolved_notifications() {
    services_resolved_callback = nullptr;
}

