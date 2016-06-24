#pragma once
#include <jni.h>
#include <stdexcept>

extern JavaVM* vm;

class JNIEnvContainer {
private:    
    JNIEnv *env = nullptr;
    
public:
    JNIEnv *operator*() {
        attach();
        return env;
    }

    JNIEnv *operator->() {
        attach();
        return env;
    }

    JNIEnvContainer() {
    }

    ~JNIEnvContainer() {
        detach();
    }

    void attach() {
        if (env != nullptr)
            return;
        jint err = vm->AttachCurrentThreadAsDaemon((void **)&env, NULL);
        if (err != JNI_OK)
            throw std::runtime_error("Attach to VM failed");
    }

    void detach() {
        if (env == nullptr)
            return;
        vm->DetachCurrentThread();
        env = nullptr;
    }
};

extern thread_local JNIEnvContainer jni_env;

class JNIGlobalRef {
private:
    jobject object;

public:
    
    JNIGlobalRef(jobject object) {
        this->object = jni_env->NewGlobalRef(object);
    }

    ~JNIGlobalRef() {
        jni_env->DeleteGlobalRef(object);
    }

    jobject operator*() {
        return object;
    }
};

