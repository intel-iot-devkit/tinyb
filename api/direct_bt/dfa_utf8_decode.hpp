/*
 * Author: Sven Gothel <sgothel@jausoft.com>
 * Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de> (see details below)
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

#ifndef DFA_UTF8_DECODE_HPP_
#define DFA_UTF8_DECODE_HPP_

#define DFA_UTF8_ACCEPT 0
#define DFA_UTF8_REJECT 12

#include <string>
#include <cstdint>
#include <cinttypes>

uint32_t dfa_utf8_decode(uint32_t & state, uint32_t & codep, const uint32_t byte_value);

/**
 * Returns all valid consecutive UTF-8 characters within buffer
 * in the range up to buffer_size or until EOS.
 * <p>
 * In case a non UTF-8 character has been detected,
 * the content will be cut off and the decoding loop ends.
 * </p>
 * <p>
 * Method utilizes a finite state machine detecting variable length UTF-8 codes.
 * See Bjoern Hoehrmann's site <http://bjoern.hoehrmann.de/utf-8/decoder/dfa/> for details.
 * </p>
 */
std::string dfa_utf8_decode(const uint8_t *buffer, const size_t buffer_size);

#endif /* DFA_UTF8_DECODE_HPP_ */
