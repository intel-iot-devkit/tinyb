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

#include "direct_bt/dbt_debug.hpp"

#include <cstdarg>

using namespace direct_bt;

void direct_bt::DBG_PRINT(const char * format, ...) {
    if(direct_bt::DBTEnv::get().DEBUG) {
        fprintf(stderr, "[%'9" PRIu64 "] Debug: ", direct_bt::DBTEnv::getElapsedMillisecond());
        va_list args;
        va_start (args, format);
        vfprintf(stderr, format, args);
        va_end (args);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

void direct_bt::INFO_PRINT(const char * format, ...) {
    if(direct_bt::DBTEnv::get().VERBOSE) {
        fprintf(stderr, "[%'9" PRIu64 "] Info: ", direct_bt::DBTEnv::getElapsedMillisecond());
        va_list args;
        va_start (args, format);
        vfprintf(stderr, format, args);
        va_end (args);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

void direct_bt::ERR_PRINT(const char * format, ...) {
    fprintf(stderr, "[%'9" PRIu64 "] Error @ %s:%d: ", direct_bt::DBTEnv::getElapsedMillisecond(), __FILE__, __LINE__);
    va_list args;
    va_start (args, format);
    vfprintf(stderr, format, args);
    va_end (args);
    fprintf(stderr, "; last errno %d %s\n", errno, strerror(errno));
    fflush(stderr);
}

void direct_bt::WARN_PRINT(const char * format, ...) {
    fprintf(stderr, "[%'9" PRIu64 "] Warning @ %s:%d: ", direct_bt::DBTEnv::getElapsedMillisecond(), __FILE__, __LINE__);
    va_list args;
    va_start (args, format);
    vfprintf(stderr, format, args);
    va_end (args);
    fprintf(stderr, "\n");
    fflush(stderr);
}

void direct_bt::PLAIN_PRINT(const char * format, ...) {
    fprintf(stderr, "[%'9" PRIu64 "] ", direct_bt::DBTEnv::getElapsedMillisecond());
    va_list args;
    va_start (args, format);
    vfprintf(stderr, format, args);
    va_end (args);
    fprintf(stderr, "\n");
    fflush(stderr);
}

void direct_bt::COND_PRINT(const bool condition, const char * format, ...) {
    if( condition ) {
        fprintf(stderr, "[%'9" PRIu64 "] ", direct_bt::DBTEnv::getElapsedMillisecond());
        va_list args;
        va_start (args, format);
        vfprintf(stderr, format, args);
        va_end (args);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}
