#include "tinyb/BluetoothGattCharacteristic.hpp"
#include "tinyb/BluetoothGattDescriptor.hpp"
#include "tinyb/BluetoothGattService.hpp"
#include "tinyb/BluetoothObject.hpp"

#include "BluetoothGattCharacteristic.h"

#include "helper.h"

using namespace tinyb;

jobject Java_BluetoothGattCharacteristic_getBluetoothType(JNIEnv *env, jobject obj)
{
    (void)obj;

    return get_bluetooth_type(env, "GATT_CHARACTERISTIC");
}

jobject Java_BluetoothGattCharacteristic_clone(JNIEnv *env, jobject obj)
{
    return generic_clone<BluetoothGattCharacteristic>(env, obj, "BluetoothGattCharacteristic");
}

jobject Java_BluetoothGattCharacteristic_readValue(JNIEnv *env, jobject obj)
{
    BluetoothGattCharacteristic *obj_gatt_char =
                                getInstance<BluetoothGattCharacteristic>(env, obj);
    std::vector<unsigned char> array = obj_gatt_char->read_value();
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

jboolean Java_BluetoothGattCharacteristic_writeValue(JNIEnv *env, jobject obj, jobject argValue)
{
    BluetoothGattCharacteristic *obj_gatt_char =
                                getInstance<BluetoothGattCharacteristic>(env, obj);
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

    return obj_gatt_char->write_value(array);
}

jboolean Java_BluetoothGattCharacteristic_startNotify(JNIEnv *env, jobject obj)
{
    BluetoothGattCharacteristic *obj_gatt_char =
                                getInstance<BluetoothGattCharacteristic>(env, obj);
    return obj_gatt_char->start_notify() ? JNI_TRUE : JNI_FALSE;
}

jboolean Java_BluetoothGattCharacteristic_stopNotify(JNIEnv *env, jobject obj)
{
    BluetoothGattCharacteristic *obj_gatt_char =
                                getInstance<BluetoothGattCharacteristic>(env, obj);
    return obj_gatt_char->stop_notify() ? JNI_TRUE : JNI_FALSE;
}

jstring Java_BluetoothGattCharacteristic_getUuid(JNIEnv *env, jobject obj)
{
    BluetoothGattCharacteristic *obj_gatt_char =
                                getInstance<BluetoothGattCharacteristic>(env, obj);
    std::string uuid = obj_gatt_char->get_uuid();

    return env->NewStringUTF((const char *)uuid.c_str());
}

jobject Java_BluetoothGattCharacteristic_getService(JNIEnv *env, jobject obj)
{
    BluetoothGattCharacteristic *obj_gatt_char =
                                getInstance<BluetoothGattCharacteristic>(env, obj);
    BluetoothGattService *obj_gatt_serv = obj_gatt_char->get_service().clone();

    jclass b_gatt_serv_class = search_class(env, "BluetoothGattService");
    jmethodID b_gatt_serv_ctor = search_method(env, b_gatt_serv_class, "<init>",
                                            "(J)V", false);
    jobject result = env->NewObject(b_gatt_serv_class, b_gatt_serv_ctor, (jlong)obj_gatt_serv);
    if (result == NULL)
    {
        throw std::runtime_error("cannot create instance of class\n");
    }

    return result;
}

jobject Java_BluetoothGattCharacteristic_getValue(JNIEnv *env, jobject obj)
{
    BluetoothGattCharacteristic *obj_gatt_char =
                                getInstance<BluetoothGattCharacteristic>(env, obj);
    std::vector<unsigned char> array = obj_gatt_char->get_value();
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

jboolean Java_BluetoothGattCharacteristic_getNotifying(JNIEnv *env, jobject obj)
{
    BluetoothGattCharacteristic *obj_gatt_char =
                                getInstance<BluetoothGattCharacteristic>(env, obj);
    return obj_gatt_char->get_notifying() ? JNI_TRUE : JNI_FALSE;
}

jobjectArray Java_BluetoothGattCharacteristic_getFlags(JNIEnv *env, jobject obj)
{
    BluetoothGattCharacteristic *obj_gatt_char =
                                getInstance<BluetoothGattCharacteristic>(env, obj);
    std::vector<std::string> flags = obj_gatt_char->get_flags();
    unsigned int flags_size = flags.size();

    jclass string_class = search_class(env, "Ljava/lang/String;");
    jobjectArray result = env->NewObjectArray(flags_size, string_class, 0);

    for (unsigned int i = 0; i < flags_size; ++i)
    {
        std::string str_elem = flags.at(i);
        jobject elem = env->NewStringUTF((const char *)str_elem.c_str());
        env->SetObjectArrayElement(result, i, elem);
    }

    return result;
}

jobject Java_BluetoothGattCharacteristic_getDescriptors(JNIEnv *env, jobject obj)
{
    BluetoothGattCharacteristic *obj_gatt_char =
                                getInstance<BluetoothGattCharacteristic>(env, obj);
    std::vector<std::unique_ptr<BluetoothGattDescriptor>> array = obj_gatt_char->get_descriptors();

    jobject result = convert_vector_to_jobject<BluetoothGattDescriptor>(env, array,
                                                                "BluetoothGattDescriptor",
                                                                "(J)V");
    return result;
}

void Java_BluetoothGattCharacteristic_delete(JNIEnv *env, jobject obj)
{
    BluetoothGattCharacteristic *obj_gatt_char =
                                getInstance<BluetoothGattCharacteristic>(env, obj);
    delete obj_gatt_char;
}

