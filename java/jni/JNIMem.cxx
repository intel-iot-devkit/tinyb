#include "JNIMem.hpp"


JavaVM* vm;
thread_local JNIEnvContainer jni_env;

jint JNI_OnLoad(JavaVM *initVM, void *reserved) {
    vm = initVM;
    return JNI_VERSION_1_8;
}
