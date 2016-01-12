#include "tinyb/BluetoothDevice.hpp"
#include "tinyb/BluetoothGattService.hpp"
#include "tinyb/BluetoothGattCharacteristic.hpp"
#include "tinyb/BluetoothObject.hpp"

#include "BluetoothGattService.h"

#include "helper.h"

using namespace tinyb;

jobject Java_BluetoothGattService_getBluetoothType(JNIEnv *env, jobject obj)
{
    (void)obj;

    return get_bluetooth_type(env, "GATT_SERVICE");
}

jobject Java_BluetoothGattService_clone(JNIEnv *env, jobject obj)
{
    return generic_clone<BluetoothGattService>(env, obj, "BluetoothGattService");
}

jstring Java_BluetoothGattService_getUuid(JNIEnv *env, jobject obj)
{
    BluetoothGattService *obj_gatt_serv = getInstance<BluetoothGattService>(env, obj);
    std::string uuid = obj_gatt_serv->get_uuid();

    return env->NewStringUTF((const char *)uuid.c_str());
}

jobject Java_BluetoothGattService_getDevice(JNIEnv *env, jobject obj)
{
    BluetoothGattService *obj_gatt_serv = getInstance<BluetoothGattService>(env, obj);
    BluetoothDevice *obj_device = obj_gatt_serv->get_device().clone();

    jclass b_device_class = search_class(env, "BluetoothDevice");
    jmethodID b_device_ctor = search_method(env, b_device_class, "<init>",
                                            "(J)V", false);
    jobject result = env->NewObject(b_device_class, b_device_ctor, (jlong)obj_device);
    if (result == NULL)
    {
        throw std::runtime_error("cannot create instance of class\n");
    }

    return result;
}

jboolean Java_BluetoothGattService_getPrimary(JNIEnv *env, jobject obj)
{
    BluetoothGattService *obj_gatt_serv = getInstance<BluetoothGattService>(env, obj);

    return obj_gatt_serv->get_primary() ? JNI_TRUE : JNI_FALSE;
}

jobject Java_BluetoothGattService_getCharacteristics(JNIEnv *env, jobject obj)
{
    BluetoothGattService *obj_gatt_serv = getInstance<BluetoothGattService>(env, obj);
    std::vector<std::unique_ptr<BluetoothGattCharacteristic>> array =
                                                obj_gatt_serv->get_characteristics();
    jobject result = convert_vector_to_jobject<BluetoothGattCharacteristic>(env, array,
                                                                    "BluetoothGattCharacteristic",
                                                                    "(J)V");
    return result;
}

void Java_BluetoothGattService_delete(JNIEnv *env, jobject obj)
{
    BluetoothGattService *obj_gatt_serv = getInstance<BluetoothGattService>(env, obj);
    delete obj_gatt_serv;
}

