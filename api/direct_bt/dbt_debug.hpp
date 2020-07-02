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

#include <cstring>
#include <string>
#include <cstdint>
#include <cstdio>

#include <vector>
#include <memory>

extern "C" {
    #include <errno.h>
}

#ifndef DBT_DEBUG_HPP_
#define DBT_DEBUG_HPP_

// #define PERF_PRINT_ON 1
// #define VERBOSE_ON 1

#ifdef VERBOSE_ON
    #define DBG_PRINT(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); }
#else
    #define DBG_PRINT(...)
#endif

#ifdef PERF_PRINT_ON
    #define PERF_TS_T0()  const uint64_t _t0 = direct_bt::getCurrentMilliseconds()

    #define PERF_TS_TD(m)  { const uint64_t _td = direct_bt::getCurrentMilliseconds() - _t0; \
                             fprintf(stderr, "%s done in %d ms,\n", (m), (int)_td); }
#else
    #define PERF_TS_T0()
    #define PERF_TS_TD(m)
#endif

#define ERR_PRINT(...) { fprintf(stderr, "Error @ %s:%d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "; last errno %d %s\n", errno, strerror(errno)); fflush(stderr); }

#define WARN_PRINT(...) { fprintf(stderr, "Warning @ %s:%d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); }

#define INFO_PRINT(...) { fprintf(stderr, "INFO: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); }

template<class ListElemType>
inline void printSharedPtrList(std::string prefix, std::vector<std::shared_ptr<ListElemType>> & list) {
    fprintf(stderr, "%s: Start: %zd elements\n", prefix.c_str(), (size_t)list.size());
    int idx = 0;
    for (auto it = list.begin(); it != list.end(); idx++) {
        std::shared_ptr<ListElemType> & e = *it;
        if ( nullptr != e ) {
            fprintf(stderr, "%s[%d]: useCount %zd, mem %p\n", prefix.c_str(), idx, (size_t)e.use_count(), e.get());
        } else {
            fprintf(stderr, "%s[%d]: NULL\n", prefix.c_str(), idx);
        }
        ++it;
    }
}

#endif /* DBT_DEBUG_HPP_ */
