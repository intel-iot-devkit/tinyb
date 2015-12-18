#include "tinyb/BluetoothAdapter.hpp"
#include "tinyb/BluetoothDevice.hpp"
#include "tinyb/BluetoothGattService.hpp"
#include "tinyb/BluetoothManager.hpp"

#include "BluetoothManager.h"

#include "helper.h"

using namespace tinyb;

jobject Java_BluetoothManager_getBluetoothType(JNIEnv *env, jobject obj)
{
    (void)obj;

    return get_bluetooth_type(env, "NONE");
}

jobject Java_BluetoothManager_getObject(JNIEnv *env, jobject obj, jobject type,
                                        jstring name, jstring identifier, jobject parent)
{
    (void)env;
    (void)obj;
    (void)type;
    (void)name;
    (void)identifier;
    (void)parent;

    return nullptr;
}

jobject Java_BluetoothManager_getObjects(JNIEnv *env, jobject obj, jobject type,
                                        jstring name, jstring identifier, jobject parent)
{
    (void)env;
    (void)obj;
    (void)type;
    (void)name;
    (void)identifier;
    (void)parent;

    return nullptr;
}

jobject Java_BluetoothManager_getAdapters(JNIEnv *env, jobject obj)
{
    BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);

    std::vector<std::unique_ptr<BluetoothAdapter>> array = manager->get_adapters();
    jobject result = convert_vector_to_jobject<BluetoothAdapter>(env, array,
                                                                "BluetoothAdapter",
                                                                "(J)V");
    return result;
}

jobject Java_BluetoothManager_getDevices(JNIEnv *env, jobject obj)
{
    BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);

    std::vector<std::unique_ptr<BluetoothDevice>> array = manager->get_devices();
    jobject result = convert_vector_to_jobject<BluetoothDevice>(env, array,
                                                                "BluetoothDevice",
                                                                "(J)V");
    return result;

}

jobject Java_BluetoothManager_getServices(JNIEnv *env, jobject obj)
{
    BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);

    std::vector<std::unique_ptr<BluetoothGattService>> array = manager->get_services();
    jobject result = convert_vector_to_jobject<BluetoothGattService>(env, array,
                                                                "BluetoothGattService",
                                                                "(J)V");
    return result;
}

jboolean Java_BluetoothManager_setDefaultAdapter(JNIEnv *env, jobject obj, jobject adapter)
{
    BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);
    BluetoothAdapter *b_adapter = getInstance<BluetoothAdapter>(env, adapter);

    return manager->set_default_adapter(b_adapter);
}

jboolean Java_BluetoothManager_startDiscovery(JNIEnv *env, jobject obj)
{
    BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);
    return manager->start_discovery() ? JNI_TRUE : JNI_FALSE;
}

jboolean Java_BluetoothManager_stopDiscovery(JNIEnv *env, jobject obj)
{
    BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);
    return manager->start_discovery() ? JNI_TRUE : JNI_FALSE;
}

void Java_BluetoothManager_init(JNIEnv *env, jobject obj)
{
    BluetoothManager *manager = BluetoothManager::get_bluetooth_manager();
    setInstance<BluetoothManager>(env, obj, manager);
}

void Java_BluetoothManager_delete(JNIEnv *env, jobject obj)
{
    BluetoothManager *manager = getInstance<BluetoothManager>(env, obj);
    delete manager;
}

