#include "BluetoothEvent.h"

jobject Java_BluetoothEvent_getType(JNIEnv *env, jobject obj)
{
    (void)env;
    (void)obj;

    return NULL;
}

jstring Java_BluetoothEvent_getName(JNIEnv *env, jobject obj)
{
    (void)env;
    (void)obj;

    return NULL;
}

jstring Java_BluetoothEvent_getIdentifier(JNIEnv *env, jobject obj)
{
    (void)env;
    (void)obj;

    return NULL;
}

jboolean Java_BluetoothEvent_executeCallback(JNIEnv *env, jobject obj)
{
    (void)env;
    (void)obj;

    return JNI_FALSE;
}

jboolean Java_BluetoothEvent_hasCallback(JNIEnv *env, jobject obj)
{
    (void)env;
    (void)obj;

    return JNI_FALSE;
}

void Java_BluetoothEvent_init(JNIEnv *env, jobject obj, jobject type, jstring name,
                                jstring identifier, jobject parent, jobject callback,
                                jobject arg_data)
{
    (void)env;
    (void)obj;
    (void)type;
    (void)name;
    (void)identifier;
    (void)parent;
    (void)callback;
    (void)arg_data;
}

void Java_BluetoothEvent_delete(JNIEnv *env, jobject obj)
{
    (void)env;
    (void)obj;
}

