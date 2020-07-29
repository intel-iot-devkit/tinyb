/*
 * Author: Sven Gothel <sgothel@jausoft.com>
 * Copyright (c) 2020 Gothel Software e.K.
 * Copyright (c) 2020 ZAFENA AB
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "org_tinyb_BluetoothFactory.h"

#include "version.h"

#include "JNIMem.hpp"
#include "helper_base.hpp"

jstring Java_org_tinyb_BluetoothFactory_getNativeAPIVersion(JNIEnv *env, jclass clazz)
{
    try {
        (void) clazz;

        std::string api_version = std::string(gVERSION_API);
        return env->NewStringUTF(api_version.c_str());
    } catch (std::bad_alloc &e) {
        raise_java_exception(env, e);
    } catch (std::runtime_error &e) {
        raise_java_exception(env, e);
    } catch (std::invalid_argument &e) {
        raise_java_exception(env, e);
    } catch (std::exception &e) {
        raise_java_exception(env, e);
    }
    return nullptr;
}

