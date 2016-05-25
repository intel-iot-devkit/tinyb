/*
 * Author: Andrei Vasiliu <andrei.vasiliu@intel.com>
 * Copyright (c) 2015 Intel Corporation.
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

#include "tinyb_utils.hpp"
#include "BluetoothException.hpp"

std::vector<unsigned char> tinyb::from_gbytes_to_vector(const GBytes *bytes)
{
    gsize result_size;
    const unsigned char *aux_array = (const unsigned char *)g_bytes_get_data(const_cast<GBytes *>(bytes), &result_size);

    if (aux_array == nullptr || result_size == 0)
        throw std::runtime_error("Trying to read empty value");

    std::vector<unsigned char> result(result_size);
    std::copy(aux_array, aux_array + result_size, result.begin());

    return result;
}

/* it allocates memory - the result that is being returned is from heap */
GBytes *tinyb::from_vector_to_gbytes(const std::vector<unsigned char>& vector)
{
    unsigned int vector_size = vector.size();
    const unsigned char *vector_content = vector.data();

    GBytes *result = g_bytes_new(vector_content, vector_size);
    if (result == nullptr)
        throw std::bad_alloc();

    return result;
}

std::vector<unsigned char> tinyb::from_iter_to_vector(GVariant *iter)
{
    GVariantIter *value_iter;
    guchar value_byte;

    g_variant_get (iter,
        "ay",
        &value_iter);

    if (value_iter == nullptr)
        throw std::invalid_argument("GVariant should be a container of an array of bytes");

    std::vector<unsigned char> value;
    while (g_variant_iter_loop(value_iter, "y", &value_byte)) {
        value.push_back(value_byte);
    }

    g_variant_iter_free(value_iter);
    return value;
}

void tinyb::handle_error(GError *error)
{
    if (error != nullptr) {
        BluetoothException e(error->message);
        g_error_free(error);
        throw e;
    }
}
