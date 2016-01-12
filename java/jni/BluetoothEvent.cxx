#include "BluetoothEvent.h"

jobject Java_BluetoothEvent_getType(JNIEnv *env, jobject obj)
{
}

jstring Java_BluetoothEvent_getName(JNIEnv *env, jobject obj)
{
}

jstring Java_BluetoothEvent_getIdentifier(JNIEnv *env, jobject obj)
{
}

jboolean Java_BluetoothEvent_executeCallback(JNIEnv *env, jobject obj)
{
}

jboolean Java_BluetoothEvent_hasCallback(JNIEnv *env, jobject obj)
{
}

void Java_BluetoothEvent_init(JNIEnv *env, jobject obj, jobject type, jstring name,
                                jstring identifier, jobject parent, jobject callback,
                                jobject arg_data)
{
}

void Java_BluetoothEvent_delete(JNIEnv *env, jobject obj)
{
}

