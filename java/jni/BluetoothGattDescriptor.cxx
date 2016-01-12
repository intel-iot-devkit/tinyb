#include "tinyb/BluetoothGattCharacteristic.hpp"
#include "tinyb/BluetoothGattDescriptor.hpp"
#include "tinyb/BluetoothObject.hpp"

#include "BluetoothGattDescriptor.h"

#include "helper.h"

using namespace tinyb;

jobject Java_BluetoothGattDescriptor_getBluetoothType(JNIEnv *env, jobject obj)
{
    (void)obj;

    return get_bluetooth_type(env, "GATT_DESCRIPTOR");
}

jobject Java_BluetoothGattDescriptor_clone(JNIEnv *env, jobject obj)
{
    return generic_clone<BluetoothGattDescriptor>(env, obj, "BluetoothGattDescriptor");
}

jobject Java_BluetoothGattDescriptor_readValue(JNIEnv *env, jobject obj)
{
    BluetoothGattDescriptor *obj_gatt_desc = getInstance<BluetoothGattDescriptor>(env, obj);
    std::vector<unsigned char> array = obj_gatt_desc->read_value();
    unsigned int array_size = array.size();

    jmethodID arraylist_add;
    jobject result = get_new_arraylist(env, array_size, &arraylist_add);

    jclass byte_class = search_class(env, "Ljava/lang/Byte");
    jmethodID byte_ctor = search_method(env, byte_class, "<init>", "(B)V", false);

    for (unsigned int i = 0; i < array_size; ++i)
    {
        unsigned char elem = array.at(i);
        jobject byte_obj = env->NewObject(byte_class, byte_ctor, elem);
        if (byte_obj == NULL)
        {
            throw std::runtime_error("cannot create instance of class\n");
        }
        env->CallBooleanMethod(result, arraylist_add, byte_obj);
    }

    return result;
}

jboolean Java_BluetoothGattDescriptor_writeValue(JNIEnv *env, jobject obj, jobject argValue)
{
    BluetoothGattDescriptor *obj_gatt_desc = getInstance<BluetoothGattDescriptor>(env, obj);

    jclass arraylist_class = search_class(env, "Ljava/util/ArrayList;");
    jmethodID arraylist_get = search_method(env, arraylist_class, "get",
                                            "(I)Ljava/lang/Object;", false);
    jmethodID arraylist_size = search_method(env, arraylist_class, "size",
                                            "()I", false);
    unsigned int size = env->CallIntMethod(argValue, arraylist_size);

    std::vector<unsigned char> array(size);
    for (unsigned int i = 0; i < size; ++i)
    {
        unsigned char elem = env->CallByteMethod(argValue, arraylist_get, i);
        array.push_back(elem);
    }

    return obj_gatt_desc->write_value(array);
}

jstring Java_BluetoothGattDescriptor_getUuid(JNIEnv *env, jobject obj)
{
    BluetoothGattDescriptor *obj_gatt_desc = getInstance<BluetoothGattDescriptor>(env, obj);
    std::string uuid = obj_gatt_desc->get_uuid();

    return env->NewStringUTF((const char *)uuid.c_str());
}

jobject Java_BluetoothGattDescriptor_getCharacteristic(JNIEnv *env, jobject obj)
{
    BluetoothGattDescriptor *obj_gatt_desc = getInstance<BluetoothGattDescriptor>(env, obj);
    BluetoothGattCharacteristic *obj_gatt_char = obj_gatt_desc->get_characteristic().clone();

    jclass b_gatt_char_class = search_class(env, "BluetoothGattCharacteristic");
    jmethodID b_gatt_char_ctor = search_method(env, b_gatt_char_class, "<init>",
                                                "(J)V", false);
    jobject result = env->NewObject(b_gatt_char_class, b_gatt_char_ctor, (jlong)obj_gatt_char);
    if (result == NULL)
    {
        throw std::runtime_error("cannot create instance of class\n");
    }

    return result;
}

jobject Java_BluetoothGattDescriptor_getValue(JNIEnv *env, jobject obj)
{
    BluetoothGattDescriptor *obj_gatt_desc = getInstance<BluetoothGattDescriptor>(env, obj);
    std::vector<unsigned char> array = obj_gatt_desc->get_value();
    unsigned int array_size = array.size();

    jmethodID arraylist_add;
    jobject result = get_new_arraylist(env, array_size, &arraylist_add);

    jclass byte_class = search_class(env, "Ljava/lang/Byte");
    jmethodID byte_ctor = search_method(env, byte_class, "<init>", "(B)V", false);

    for (unsigned int i = 0; i < array_size; ++i)
    {
        unsigned char elem = array.at(i);
        jobject byte_obj = env->NewObject(byte_class, byte_ctor, elem);
        if (byte_obj == NULL)
        {
            throw std::runtime_error("cannot create instance of class\n");
        }
        env->CallBooleanMethod(result, arraylist_add, byte_obj);
    }

    return result;

}

void Java_BluetoothGattDescriptor_delete(JNIEnv *env, jobject obj)
{
    BluetoothGattDescriptor *obj_gatt_desc = getInstance<BluetoothGattDescriptor>(env, obj);
    delete obj_gatt_desc;
}

