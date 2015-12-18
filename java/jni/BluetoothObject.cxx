#include "tinyb/BluetoothObject.hpp"

#include "BluetoothObject.h"

#include "helper.h"

using namespace tinyb;

jobject Java_BluetoothObject_getBluetoothType(JNIEnv *env, jobject obj)
{
    (void)obj;

    return get_bluetooth_type(env, "NONE");
}

jobject Java_BluetoothObject_clone(JNIEnv *env, jobject obj)
{
    return generic_clone<BluetoothObject>(env, obj, "BluetoothObject");
}

void Java_BluetoothObject_delete(JNIEnv *env, jobject obj)
{
    BluetoothObject *obj_b = getInstance<BluetoothObject>(env, obj);

    delete obj_b;
}

jboolean Java_BluetoothObject_operatorEqual(JNIEnv *env, jobject obj, jobject other)
{
    BluetoothObject *obj_b = getInstance<BluetoothObject>(env, obj);
    BluetoothObject *obj_other = getInstance<BluetoothObject>(env, other);

    return (*obj_b) == (*obj_other);
}

