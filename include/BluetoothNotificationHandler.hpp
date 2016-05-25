#include <gio/gio.h>

class tinyb::BluetoothNotificationHandler {

public:
    static void on_properties_changed_adapter(GDBusProxy *proxy, GVariant *changed_properties, GStrv invalidated_properties, gpointer userdata);
    static void on_properties_changed_device(GDBusProxy *proxy, GVariant *changed_properties, GStrv invalidated_properties, gpointer userdata);
    static void on_properties_changed_characteristic(GDBusProxy *proxy, GVariant *changed_properties, GStrv invalidated_properties, gpointer userdata);
    static void on_properties_changed_descriptor(GDBusProxy *proxy, GVariant *changed_properties, GStrv invalidated_properties, gpointer userdata);

};
