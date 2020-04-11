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
#include <memory>
#include <cstdint>
#include <vector>
#include <cstdio>

#include  <algorithm>

#include "ATTPDUTypes.hpp"

#include "dbt_debug.hpp"

using namespace direct_bt;

#define OPCODE_ENUM(X) \
        X(ATT_PDU_UNDEFINED) \
        X(ATT_ERROR_RSP) \
        X(ATT_EXCHANGE_MTU_REQ) \
        X(ATT_EXCHANGE_MTU_RSP) \
        X(ATT_FIND_INFORMATION_REQ) \
        X(ATT_FIND_INFORMATION_RSP) \
        X(ATT_FIND_BY_TYPE_VALUE_REQ) \
        X(ATT_FIND_BY_TYPE_VALUE_RSP) \
        X(ATT_READ_BY_TYPE_REQ) \
        X(ATT_READ_BY_TYPE_RSP) \
        X(ATT_READ_REQ) \
        X(ATT_READ_RSP) \
        X(ATT_READ_BLOB_REQ) \
        X(ATT_READ_BLOB_RSP) \
        X(ATT_READ_MULTIPLE_REQ) \
        X(ATT_READ_MULTIPLE_RSP) \
        X(ATT_READ_BY_GROUP_TYPE_REQ) \
        X(ATT_READ_BY_GROUP_TYPE_RSP) \
        X(ATT_WRITE_REQ) \
        X(ATT_WRITE_RSP) \
        X(ATT_WRITE_CMD) \
        X(ATT_PREPARE_WRITE_REQ) \
        X(ATT_PREPARE_WRITE_RSP) \
        X(ATT_EXECUTE_WRITE_REQ) \
        X(ATT_EXECUTE_WRITE_RSP) \
        X(ATT_READ_MULTIPLE_VARIABLE_REQ) \
        X(ATT_READ_MULTIPLE_VARIABLE_RSP) \
        X(ATT_MULTIPLE_HANDLE_VALUE_NTF) \
        X(ATT_HANDLE_VALUE_NTF) \
        X(ATT_HANDLE_VALUE_IND) \
        X(ATT_HANDLE_VALUE_CFM) \
        X(ATT_SIGNED_WRITE_CMD)

#define CASE_TO_STRING(V) case V: return #V;

std::string AttPDUMsg::getOpcodeString(const Opcode opc) {
    switch(opc) {
        OPCODE_ENUM(CASE_TO_STRING)
        default: ; // fall through intended
    }
    return "Unknown Opcode";
}

std::string AttErrorRsp::getPlainErrorString(const ErrorCode errorCode) {
    switch(errorCode) {
        case INVALID_HANDLE: return "Invalid Handle";
        case NO_READ_PERM: return "Read Not Permitted";
        case NO_WRITE_PERM: return "Write Not Permitted";
        case INVALID_PDU: return "Invalid PDU";
        case INSUFF_AUTHENTICATION: return "Insufficient Authentication";
        case UNSUPPORTED_REQUEST: return "Request Not Supported";
        case INVALID_OFFSET: return "Invalid Offset";
        case INSUFF_AUTHORIZATION: return "Insufficient Authorization";
        case PREPARE_QUEUE_FULL: return "Prepare Queue Full";
        case ATTRIBUTE_NOT_FOUND: return "Attribute Not Found";
        case ATTRIBUTE_NOT_LONG: return "Attribute Not Long";
        case INSUFF_ENCRYPTION_KEY_SIZE: return "Insufficient Encryption Key Size";
        case INVALID_ATTRIBUTE_VALUE_LEN: return "Invalid Attribute Value Length";
        case UNLIKELY_ERROR: return "Unlikely Error";
        case INSUFF_ENCRYPTION: return "Insufficient Encryption";
        case UNSUPPORTED_GROUP_TYPE: return "Unsupported Group Type";
        case INSUFFICIENT_RESOURCES: return "Insufficient Resources";
        case DB_OUT_OF_SYNC: return "Database Out Of Sync";
        case FORBIDDEN_VALUE: return "Value Not Allowed";
        default: ; // fall through intended
    }
    if( 0x80 <= errorCode && errorCode <= 0x9F ) {
        return "Application Error";
    }
    if( 0xE0 <= errorCode /* && errorCode <= 0xFF */ ) {
        return "Common Profile and Services Error";
    }
    return "Error Reserved for future use";
}

AttPDUMsg* AttPDUMsg::getSpecialized(const uint8_t * buffer, int const buffer_size) {
    const uint8_t opc = *buffer;
    AttPDUMsg * res;
    switch( opc ) {
        case ATT_PDU_UNDEFINED: res = new AttPDUUndefined(buffer, buffer_size); break;
        case ATT_ERROR_RSP: res = new AttErrorRsp(buffer, buffer_size); break;
        case ATT_EXCHANGE_MTU_REQ: res = new AttExchangeMTU(buffer, buffer_size); break;
        case ATT_EXCHANGE_MTU_RSP: res = new AttExchangeMTU(buffer, buffer_size); break;
        case ATT_FIND_INFORMATION_REQ: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_FIND_INFORMATION_RSP: res = new AttFindInfoRsp(buffer, buffer_size); break;
        case ATT_FIND_BY_TYPE_VALUE_REQ: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_FIND_BY_TYPE_VALUE_RSP: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_READ_BY_TYPE_REQ: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_READ_BY_TYPE_RSP: res = new AttReadByTypeRsp(buffer, buffer_size); break;
        case ATT_READ_REQ: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_READ_RSP: res = new AttReadRsp(buffer, buffer_size); break;
        case ATT_READ_BLOB_REQ: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_READ_BLOB_RSP: res = new AttReadBlobRsp(buffer, buffer_size); break;
        case ATT_READ_MULTIPLE_REQ: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_READ_MULTIPLE_RSP: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_READ_BY_GROUP_TYPE_REQ: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_READ_BY_GROUP_TYPE_RSP: res = new AttReadByGroupTypeRsp(buffer, buffer_size); break;
        case ATT_WRITE_REQ: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_WRITE_RSP: res = new AttWriteRsp(buffer, buffer_size); break;
        case ATT_WRITE_CMD: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_PREPARE_WRITE_REQ: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_PREPARE_WRITE_RSP: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_EXECUTE_WRITE_REQ: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_EXECUTE_WRITE_RSP: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_READ_MULTIPLE_VARIABLE_REQ: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_READ_MULTIPLE_VARIABLE_RSP: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_MULTIPLE_HANDLE_VALUE_NTF: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_HANDLE_VALUE_NTF: res = new AttHandleValueRcv(buffer, buffer_size); break;
        case ATT_HANDLE_VALUE_IND: res = new AttHandleValueRcv(buffer, buffer_size); break;
        case ATT_HANDLE_VALUE_CFM: res = new AttPDUMsg(buffer, buffer_size); break;
        case ATT_SIGNED_WRITE_CMD: res = new AttPDUMsg(buffer, buffer_size); break;
        default: res = new AttPDUMsg(buffer, buffer_size); break;
    }
    return res;
}
