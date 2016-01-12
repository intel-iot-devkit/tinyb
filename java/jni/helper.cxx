#include <jni.h>
#include <memory>
#include <stdexcept>
#include <vector>

#include "helper.h"

jfieldID getInstanceField(JNIEnv *env, jobject obj)
{
    jclass clazz = env->GetObjectClass(obj);
    // J == long
    return env->GetFieldID(clazz, "nativeInstance", "J");
}

jclass search_class(JNIEnv *env, const char *clazz_name)
{
    jclass clazz = env->FindClass(clazz_name);
    if (clazz == NULL)
    {
        throw std::runtime_error("no class found\n");
    }
    return clazz;
}

jmethodID search_method(JNIEnv *env, jclass clazz, const char *method_name,
                const char *prototype, bool is_static)
{
    jmethodID method;
    if(is_static)
    {
        method = env->GetStaticMethodID(clazz, method_name, prototype);
    }
    else
    {
        method = env->GetMethodID(clazz, method_name, prototype);
    }

    if(method == NULL)
    {
        throw std::runtime_error("no method found\n");
    }

    return method;
}

jfieldID search_field(JNIEnv *env, jclass clazz, const char *field_name,
                const char *type, bool is_static)
{
    jfieldID field;
    if(is_static)
    {
        field = env->GetStaticFieldID(clazz, field_name, type);
    }
    else
    {
        field = env->GetFieldID(clazz, field_name, type);
    }

    if(field == NULL)
    {
        throw std::runtime_error("no method found\n");
    }

    return field;
}

bool from_jboolean_to_bool(jboolean val)
{
    bool result;

    if (val == JNI_TRUE)
    {
        result = true;
    }
    else
    {
        if (val == JNI_FALSE)
        {
            result = false;
        }
        else
        {
            throw std::invalid_argument("the jboolean value is not true/false\n");
        }
    }

    return result;
}

jobject get_bluetooth_type(JNIEnv *env, const char *field_name)
{
    jclass b_type_enum = search_class(env, "BluetoothType");

    jfieldID b_type_field = search_field(env, b_type_enum, field_name, "l", true);

    jobject result = env->GetStaticObjectField(b_type_enum, b_type_field);
    return result;
}


jobject get_new_arraylist(JNIEnv *env, unsigned int size, jmethodID *add)
{
    jclass arraylist_class = search_class(env, "Ljava/util/ArrayList;");
    jmethodID arraylist_ctor = search_method(env, arraylist_class, "<init>", "(I)V", false);

    jobject result = env->NewObject(arraylist_class, arraylist_ctor, size);
    if (result == NULL)
    {
        throw std::runtime_error("cannot create instance of class\n");
    }

    *add = search_method(env, arraylist_class, "add", "(Ljava/lang/Object;)Z", false);

    return result;
}
