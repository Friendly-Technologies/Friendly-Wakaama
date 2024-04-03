/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v20.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Fabien Fleutot - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *
 *******************************************************************************/

#include "internals.h"
#include <float.h>

#define _PRV_STR_LENGTH 32

// dataP array length is assumed to be 1.
static int prv_textSerialize(lwm2m_data_t * dataP,
                             uint8_t ** bufferP)
{
    size_t res;

    switch (dataP->type)
    {
    case LWM2M_TYPE_STRING:
    case LWM2M_TYPE_CORE_LINK:
        *bufferP = (uint8_t *)lwm2m_malloc(dataP->value.asBuffer.length);
        if (*bufferP == NULL) return 0;
        memcpy(*bufferP, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
        return (int)dataP->value.asBuffer.length;
    
    case LWM2M_TYPE_INTEGER:
    case LWM2M_TYPE_TIME:
    {
        uint8_t intString[_PRV_STR_LENGTH];

        res = utils_intToText(dataP->value.asInteger, intString, _PRV_STR_LENGTH);
        if (res == 0) return -1;

        *bufferP = (uint8_t *)lwm2m_malloc(res);
        if (NULL == *bufferP) return -1;

        memcpy(*bufferP, intString, res);

        return (int)res;
    }

    case LWM2M_TYPE_UNSIGNED_INTEGER:
    {
        uint8_t intString[_PRV_STR_LENGTH];

        res = utils_uintToText(dataP->value.asUnsigned, intString, _PRV_STR_LENGTH);
        if (res == 0) return -1;

        *bufferP = (uint8_t *)lwm2m_malloc(res);
        if (NULL == *bufferP) return -1;

        memcpy(*bufferP, intString, res);

        return (int)res;
    }

    case LWM2M_TYPE_FLOAT:
    {
        uint8_t floatString[_PRV_STR_LENGTH * 2];

        res = utils_floatToText(dataP->value.asFloat, floatString, _PRV_STR_LENGTH * 2, false);
        if (res == 0) return -1;

        *bufferP = (uint8_t *)lwm2m_malloc(res);
        if (NULL == *bufferP) return -1;

        memcpy(*bufferP, floatString, res);

        return (int)res;
    }

    case LWM2M_TYPE_BOOLEAN:
        *bufferP = (uint8_t *)lwm2m_malloc(1);
        if (NULL == *bufferP) return -1;

        *bufferP[0] = dataP->value.asBoolean ? '1' : '0';

        return 1;

    case LWM2M_TYPE_OBJECT_LINK:
    {
        char stringBuffer[11];
        size_t length;
        
        length = utils_intToText(dataP->value.asObjLink.objectId, (uint8_t*)stringBuffer, 5);
        if (length == 0) return -1;

        stringBuffer[5] = ':';
        res = length + 1;

        length = utils_intToText(dataP->value.asObjLink.objectInstanceId, (uint8_t*)stringBuffer + res, 5);
        if (length == 0) return -1;

        res += length;

        *bufferP = (uint8_t *)lwm2m_malloc(res);
        if (*bufferP == NULL) return -1;

        memcpy(*bufferP, stringBuffer, res);

        return res;
    }

    case LWM2M_TYPE_OPAQUE:
    {
        size_t length;

        length = utils_base64GetSize(dataP->value.asBuffer.length);
        *bufferP = (uint8_t *)lwm2m_malloc(length);
        if (*bufferP == NULL) return 0;
        length = utils_base64Encode(dataP->value.asBuffer.buffer, dataP->value.asBuffer.length, *bufferP, length);
        if (length == 0)
        {
            lwm2m_free(*bufferP);
            *bufferP = NULL;
            return 0;
        }
        return (int)length;
    }

    case LWM2M_TYPE_UNDEFINED:
    default:
        return -1;
    }
}

static int prv_setBuffer(lwm2m_data_t * dataP,
                         const uint8_t * buffer,
                         size_t bufferLen)
{
    dataP->value.asBuffer.buffer = (uint8_t *)lwm2m_malloc(bufferLen);
    if (dataP->value.asBuffer.buffer == NULL)
    {
        return 0;
    }
    dataP->value.asBuffer.length = bufferLen;
    memcpy(dataP->value.asBuffer.buffer, buffer, bufferLen);

    return 1;
}

lwm2m_data_t * lwm2m_data_new(int size)
{
    lwm2m_data_t * dataP;

    LOG_ARG("size: %d", size);
    if (size <= 0) return NULL;

    dataP = (lwm2m_data_t *)lwm2m_malloc(size * sizeof(lwm2m_data_t));

    if (dataP != NULL)
    {
        memset(dataP, 0, size * sizeof(lwm2m_data_t));
    }

    return dataP;
}

void lwm2m_data_free(int size,
                     lwm2m_data_t * dataP)
{
    int i;

    LOG_ARG("size: %d", size);
    if (size == 0 || dataP == NULL) return;

    for (i = 0; i < size; i++)
    {
        switch (dataP[i].type)
        {
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
        case LWM2M_TYPE_OBJECT_INSTANCE:
        case LWM2M_TYPE_OBJECT:
            lwm2m_data_free(dataP[i].value.asChildren.count, dataP[i].value.asChildren.array);
            break;

        case LWM2M_TYPE_STRING:
        case LWM2M_TYPE_OPAQUE:
        case LWM2M_TYPE_CORE_LINK:
            if (dataP[i].value.asBuffer.buffer != NULL)
            {
                lwm2m_free(dataP[i].value.asBuffer.buffer);
            }
            break;

        default:
            // do nothing
            break;
        }
    }
    lwm2m_free(dataP);
}

void lwm2m_data_encode_string(const char * string,
                              lwm2m_data_t * dataP)
{
    size_t len;
    int res;

    LOG_ARG("\"%s\"", STR_NULL2EMPTY(string));
    if (string == NULL)
    {
        len = 0;
    }
    else
    {
        for (len = 0; string[len] != 0; len++);
    }

    if (len == 0)
    {
        dataP->value.asBuffer.length = 0;
        dataP->value.asBuffer.buffer = NULL;
        res = 1;
    }
    else
    {
        res = prv_setBuffer(dataP, (uint8_t *)string, len);
    }

    if (res == 1)
    {
        dataP->type = LWM2M_TYPE_STRING;
    }
    else
    {
        dataP->type = LWM2M_TYPE_UNDEFINED;
    }
}

void lwm2m_data_encode_opaque(const uint8_t * buffer,
                              size_t length,
                              lwm2m_data_t * dataP)
{
    int res;

    LOG_ARG("length: %d", length);
    if (length == 0)
    {
        dataP->value.asBuffer.length = 0;
        dataP->value.asBuffer.buffer = NULL;
        res = 1;
    }
    else
    {
        res = prv_setBuffer(dataP, buffer, length);
    }

    if (res == 1)
    {
        dataP->type = LWM2M_TYPE_OPAQUE;
    }
    else
    {
        dataP->type = LWM2M_TYPE_UNDEFINED;
    }
}

void lwm2m_data_encode_nstring(const char * string,
                               size_t length,
                               lwm2m_data_t * dataP)
{
    LOG_ARG("length: %d, string: \"%.*s\"", length, length, STR_NULL2EMPTY(string));
    lwm2m_data_encode_opaque((uint8_t *)string, length, dataP);

    if (dataP->type == LWM2M_TYPE_OPAQUE)
    {
        dataP->type = LWM2M_TYPE_STRING;
    }
}

int lwm2m_data_decode_time(const lwm2m_data_t * dataP,
                          int64_t * valueP)
{
    int result = 0;

    LOG("Entering");
    switch (dataP->type)
    {
        case LWM2M_TYPE_TIME:
        case LWM2M_TYPE_INTEGER:
            *valueP = dataP->value.asInteger;
            result = 1;
            break;
        case LWM2M_TYPE_UNSIGNED_INTEGER:
            if (dataP->value.asUnsigned <= INT64_MAX)
            {
                *valueP = dataP->value.asUnsigned;
                result = 1;
            }
            break;
        case LWM2M_TYPE_STRING:
            result = utils_textToInt(dataP->value.asBuffer.buffer, dataP->value.asBuffer.length, valueP);
            break;
        case LWM2M_TYPE_OPAQUE:
            switch (dataP->value.asBuffer.length)
            {
                case 1:
                    *valueP = (int8_t)dataP->value.asBuffer.buffer[0];
                    result = 1;
                    break;
                case 2:
                {
                    int16_t value;
                    utils_copyValue(&value, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
                    *valueP = value;
                    result = 1;
                    break;
                }
                case 4:
                {
                    int32_t value;
                    utils_copyValue(&value, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
                    *valueP = value;
                    result = 1;
                    break;
                }
                case 8:
                    utils_copyValue(valueP, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
                    result = 1;
                    break;
                default:
                    break;
            }//!LWM2M_TYPE_OPAQUE-case
            break;

        default:
            break;
    } ///!main switch-case
    LOG_ARG("result: %d, value: %" PRId64, result, *valueP);
    return result;
}

void lwm2m_data_encode_time(uint64_t value, lwm2m_data_t * dataP)
{
   LOG_ARG("value: %" PRId64 "", value);
   dataP->type = LWM2M_TYPE_TIME;
   dataP->value.asInteger = value;
}

void lwm2m_data_encode_int(int64_t value,
                           lwm2m_data_t * dataP)
{
    LOG_ARG("value: %" PRId64 "", value);
    dataP->type = LWM2M_TYPE_INTEGER;
    dataP->value.asInteger = value;
}

int lwm2m_data_decode_int(const lwm2m_data_t * dataP,
                          int64_t * valueP)
{
    int result = 0;

    LOG("Entering");
    switch (dataP->type)
    {
    case LWM2M_TYPE_INTEGER:
    case LWM2M_TYPE_TIME:
        *valueP = dataP->value.asInteger;
        result = 1;
        break;

    case LWM2M_TYPE_UNSIGNED_INTEGER:
        if (dataP->value.asUnsigned <= INT64_MAX)
        {
            *valueP = dataP->value.asUnsigned;
            result = 1;
        }
        break;

    case LWM2M_TYPE_STRING:
        result = utils_textToInt(dataP->value.asBuffer.buffer, dataP->value.asBuffer.length, valueP);
        break;

    case LWM2M_TYPE_OPAQUE:
        switch (dataP->value.asBuffer.length)
        {
        case 1:
            *valueP = (int8_t)dataP->value.asBuffer.buffer[0];
            result = 1;
            break;

        case 2:
        {
            int16_t value;

            utils_copyValue(&value, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);

            *valueP = value;
            result = 1;
            break;
        }

        case 4:
        {
            int32_t value;

            utils_copyValue(&value, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);

            *valueP = value;
            result = 1;
            break;
        }

        case 8:
            utils_copyValue(valueP, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
            result = 1;
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }
    LOG_ARG("result: %d, value: %" PRId64, result, *valueP);

    return result;
}

void lwm2m_data_encode_uint(uint64_t value,
                            lwm2m_data_t * dataP)
{
    LOG_ARG("value: %" PRIu64 "", value);
    dataP->type = LWM2M_TYPE_UNSIGNED_INTEGER;
    dataP->value.asUnsigned = value;
}

int lwm2m_data_decode_uint(const lwm2m_data_t * dataP,
                           uint64_t * valueP)
{
    int result = 0;

    LOG("Entering");
    switch (dataP->type)
    {
    case LWM2M_TYPE_INTEGER:
        if (dataP->value.asInteger >= 0)
        {
            *valueP = dataP->value.asInteger;
            result = 1;
        }
        break;

    case LWM2M_TYPE_UNSIGNED_INTEGER:
        *valueP = dataP->value.asUnsigned;
        result = 1;
        break;

    case LWM2M_TYPE_STRING:
        result = utils_textToUInt(dataP->value.asBuffer.buffer, dataP->value.asBuffer.length, valueP);
        break;

    case LWM2M_TYPE_OPAQUE:
        switch (dataP->value.asBuffer.length)
        {
        case 1:
            *valueP = (uint8_t)dataP->value.asBuffer.buffer[0];
            result = 1;
            break;

        case 2:
        {
            uint16_t value;

            utils_copyValue(&value, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);

            *valueP = value;
            result = 1;
            break;
        }

        case 4:
        {
            uint32_t value;

            utils_copyValue(&value, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);

            *valueP = value;
            result = 1;
            break;
        }

        case 8:
            utils_copyValue(valueP, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
            result = 1;
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }
    LOG_ARG("result: %d, value: %" PRIu64, result, *valueP);

    return result;
}

void lwm2m_data_encode_float(double value,
                             lwm2m_data_t * dataP)
{
    LOG_ARG("value: %f", value);
    dataP->type = LWM2M_TYPE_FLOAT;
    dataP->value.asFloat = value;
}

int lwm2m_data_decode_float(const lwm2m_data_t * dataP,
                            double * valueP)
{
    int result = 0;

    LOG("Entering");
    switch (dataP->type)
    {
    case LWM2M_TYPE_FLOAT:
        *valueP = dataP->value.asFloat;
        result = 1;
        break;

    case LWM2M_TYPE_INTEGER:
        *valueP = (double)dataP->value.asInteger;
        result = 1;
        break;

    case LWM2M_TYPE_UNSIGNED_INTEGER:
        *valueP = (double)dataP->value.asUnsigned;
        result = 1;
        break;

    case LWM2M_TYPE_STRING:
        result = utils_textToFloat(dataP->value.asBuffer.buffer, dataP->value.asBuffer.length, valueP, false);
        break;

    case LWM2M_TYPE_OPAQUE:
        switch (dataP->value.asBuffer.length)
        {
        case 4:
        {
            float temp;

            utils_copyValue(&temp, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);

            *valueP = temp;
            result = 1;
        }
        break;

        case 8:
            utils_copyValue(valueP, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
            result = 1;
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }

    LOG_ARG("result: %d, value: %f", result, *valueP);

    return result;
}

void lwm2m_data_encode_bool(bool value,
                            lwm2m_data_t * dataP)
{
    LOG_ARG("value: %s", value?"true":"false");
    dataP->type = LWM2M_TYPE_BOOLEAN;
    dataP->value.asBoolean = value;
}

int lwm2m_data_decode_bool(const lwm2m_data_t * dataP,
                           bool * valueP)
{
    int result;

    LOG("Entering");
    switch (dataP->type)
    {
    case LWM2M_TYPE_BOOLEAN:
        *valueP = dataP->value.asBoolean;
        result = 1;
        break;

    case LWM2M_TYPE_STRING:
        if (dataP->value.asBuffer.length != 1) return 0;

        switch (dataP->value.asBuffer.buffer[0])
        {
        case '0':
            *valueP = false;
            result = 1;
            break;
        case '1':
            *valueP = true;
            result = 1;
            break;
        default:
            result = 0;
            break;
        }
        break;

    case LWM2M_TYPE_OPAQUE:
        if (dataP->value.asBuffer.length != 1) return 0;

        switch (dataP->value.asBuffer.buffer[0])
        {
        case 0:
            *valueP = false;
            result = 1;
            break;
        case 1:
            *valueP = true;
            result = 1;
            break;
        default:
            result = 0;
            break;
        }
        break;

    default:
        result = 0;
        break;
    }

    LOG_ARG("result: %d, value: %s", result, *valueP ? "true" : "false");

    return result;
}

void lwm2m_data_encode_corelink(const char * corelink, lwm2m_data_t * dataP)
{
    LOG_ARG("\"%s\"", STR_NULL2EMPTY(corelink));
    lwm2m_data_encode_string(corelink, dataP);
    if (dataP->type == LWM2M_TYPE_STRING)
    {
        dataP->type = LWM2M_TYPE_CORE_LINK;
    }
}

void lwm2m_data_encode_objlink(uint16_t objectId,
                           uint16_t objectInstanceId,
                           lwm2m_data_t * dataP)
{
    LOG_ARG("value: %d/%d", objectId, objectInstanceId);
    dataP->type = LWM2M_TYPE_OBJECT_LINK;
    dataP->value.asObjLink.objectId = objectId;
    dataP->value.asObjLink.objectInstanceId = objectInstanceId;
}

int lwm2m_data_decode_objlink(const lwm2m_data_t * dataP,
                           uint16_t* objectId,
                           uint16_t* objectInstanceId)
{
    int result;

    switch (dataP->type)
    {
        case LWM2M_TYPE_OBJECT_LINK:
            *objectId = dataP->value.asObjLink.objectId;
            *objectInstanceId = dataP->value.asObjLink.objectInstanceId;
            result = 1;
            break;
        case LWM2M_TYPE_STRING:
            if (dataP->value.asBuffer.length <= 0) return 0; /// here the length can be more than 1
            result = utils_textToObjLink(dataP->value.asBuffer.buffer, dataP->value.asBuffer.length, objectId, objectInstanceId);
            break;
        default:
            result = 0;
            break;
    }
    LOG_ARG("result: %d, value: objectId: %d objectInstanceId: %d", result, objectId, objectInstanceId);
    return result;
}

void lwm2m_data_include(lwm2m_data_t * subDataP,
                        size_t count,
                        lwm2m_data_t * dataP)
{
    LOG_ARG("count: %d", count);
    if (subDataP == NULL || count == 0) return;

    switch (subDataP[0].type)
    {
    case LWM2M_TYPE_STRING:
    case LWM2M_TYPE_OPAQUE:
    case LWM2M_TYPE_INTEGER:
    case LWM2M_TYPE_TIME:
    case LWM2M_TYPE_UNSIGNED_INTEGER:
    case LWM2M_TYPE_FLOAT:
    case LWM2M_TYPE_BOOLEAN:
    case LWM2M_TYPE_OBJECT_LINK:
    case LWM2M_TYPE_CORE_LINK:
    case LWM2M_TYPE_MULTIPLE_RESOURCE:
        dataP->type = LWM2M_TYPE_OBJECT_INSTANCE;
        break;
    case LWM2M_TYPE_OBJECT_INSTANCE:
        dataP->type = LWM2M_TYPE_OBJECT;
        break;
    default:
        dataP->type = LWM2M_TYPE_MULTIPLE_RESOURCE;
        break;
    }
    dataP->value.asChildren.count = count;
    dataP->value.asChildren.array = subDataP;
}

void lwm2m_data_encode_instances(lwm2m_data_t * subDataP,
                                 size_t count,
                                 lwm2m_data_t * dataP)
{
    LOG_ARG("count: %d", count);
    lwm2m_data_include(subDataP, count, dataP);
    dataP->type = LWM2M_TYPE_MULTIPLE_RESOURCE;
}

int lwm2m_data_parse(lwm2m_uri_t * uriP,
                     const uint8_t * buffer,
                     size_t bufferLen,
                     lwm2m_media_type_t format,
                     lwm2m_data_t ** dataP)
{
    int res;

    LOG_ARG("format: %s, bufferLen: %d", STR_MEDIA_TYPE(format), bufferLen);
    LOG_URI(uriP);
    switch (format)
    {
    case LWM2M_CONTENT_TEXT:
        if (!LWM2M_URI_IS_SET_RESOURCE(uriP)) return 0;
        *dataP = lwm2m_data_new(1);
        if (*dataP == NULL) return 0;
        (*dataP)->id = uriP->resourceId;
#ifndef LWM2M_VERSION_1_0
        if (LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP))
            (*dataP)->id = uriP->resourceInstanceId;
#endif
        (*dataP)->type = LWM2M_TYPE_STRING;
        res = prv_setBuffer(*dataP, buffer, bufferLen);
        if (res == 0)
        {
            lwm2m_data_free(1, *dataP);
            *dataP = NULL;
        }
        return res;

    case LWM2M_CONTENT_OPAQUE:
        if (!LWM2M_URI_IS_SET_RESOURCE(uriP)) return 0;
        *dataP = lwm2m_data_new(1);
        if (*dataP == NULL) return 0;
        (*dataP)->id = uriP->resourceId;
#ifndef LWM2M_VERSION_1_0
        if (LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP))
            (*dataP)->id = uriP->resourceInstanceId;
#endif
        (*dataP)->type = LWM2M_TYPE_OPAQUE;
        res = prv_setBuffer(*dataP, buffer, bufferLen);
        if (res == 0)
        {
            lwm2m_data_free(1, *dataP);
            *dataP = NULL;
        }
        return res;

#ifdef LWM2M_SUPPORT_TLV
#ifdef LWM2M_OLD_CONTENT_FORMAT_SUPPORT
    case LWM2M_CONTENT_TLV_OLD:
#endif
    case LWM2M_CONTENT_TLV: {
        int size = tlv_parse(buffer, bufferLen, dataP);
        if (size == 1 && uriP != NULL && LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP) && (*dataP)->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
        {
            if((*dataP)->value.asChildren.count != 1) return -1;
            lwm2m_data_t *tmpDataP = (*dataP);
            size = tmpDataP->value.asChildren.count;
            *dataP = tmpDataP->value.asChildren.array;
            lwm2m_free(tmpDataP);
        }

        return size;
    }
#endif

#ifdef LWM2M_SUPPORT_CBOR
    case LWM2M_CONTENT_CBOR:
        LOG("CBOR cbor_parse ");
        return cbor_parse(uriP, buffer, bufferLen, dataP);
#endif

#ifdef LWM2M_SUPPORT_JSON
#ifdef LWM2M_OLD_CONTENT_FORMAT_SUPPORT
    case LWM2M_CONTENT_JSON_OLD:
#endif
    case LWM2M_CONTENT_JSON:
        return json_parse(uriP, buffer, bufferLen, dataP);
#endif

#ifdef LWM2M_SUPPORT_SENML_JSON
    case LWM2M_CONTENT_SENML_JSON:
        return senml_json_parse(uriP, buffer, bufferLen, dataP);
#endif

    default:
        return 0;
    }
}

int lwm2m_data_serialize(lwm2m_uri_t * uriP,
                         int size,
                         lwm2m_data_t * dataP,
                         lwm2m_media_type_t * formatP,
                         uint8_t ** bufferP)
{
    LOG_URI(uriP);
    LOG_ARG("size: %d, formatP: %s", size, STR_MEDIA_TYPE(*formatP));
    // Check format
    if (*formatP == LWM2M_CONTENT_TEXT
     || *formatP == LWM2M_CONTENT_OPAQUE)
    {
        if (size != 1
         || (uriP != NULL && !LWM2M_URI_IS_SET_RESOURCE(uriP))
         || dataP->type == LWM2M_TYPE_OBJECT
         || dataP->type == LWM2M_TYPE_OBJECT_INSTANCE
         || dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
        {
#ifdef LWM2M_SUPPORT_SENML_JSON
            *formatP = LWM2M_CONTENT_SENML_JSON;
#elif defined(LWM2M_SUPPORT_JSON)
            *formatP = LWM2M_CONTENT_JSON;
#elif defined(LWM2M_SUPPORT_TLV)
            *formatP = LWM2M_CONTENT_TLV;
#else
            return -1;
#endif
        }
    }

    if (*formatP == LWM2M_CONTENT_OPAQUE
     && dataP->type != LWM2M_TYPE_OPAQUE)
    {
        LOG("Opaque format is reserved to opaque resources.");
        return -1;
    }

    LOG_ARG("Final format: %s", STR_MEDIA_TYPE(*formatP));

    switch (*formatP)
    {
    case LWM2M_CONTENT_TEXT:
        return prv_textSerialize(dataP, bufferP);

    case LWM2M_CONTENT_OPAQUE:
        *bufferP = (uint8_t *)lwm2m_malloc(dataP->value.asBuffer.length);
        if (*bufferP == NULL) return -1;
        memcpy(*bufferP, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
        return (int)dataP->value.asBuffer.length;

#ifdef LWM2M_SUPPORT_TLV
    case LWM2M_CONTENT_TLV:
#ifdef LWM2M_OLD_CONTENT_FORMAT_SUPPORT
    case LWM2M_CONTENT_TLV_OLD:
#endif
    {
        bool isResourceInstance;

#ifndef LWM2M_VERSION_1_0
        if (uriP != NULL && LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP))
        {
            if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
                size = dataP->value.asChildren.count;
                dataP = dataP->value.asChildren.array;
            }
            if(size != 1) return -1;
            isResourceInstance = true;
        }
        else
#endif
        if (uriP != NULL && LWM2M_URI_IS_SET_RESOURCE(uriP)
         && (size != 1 || dataP->id != uriP->resourceId))
        {
            isResourceInstance = true;
        }
        else
        {
            isResourceInstance = false;
        }
        return tlv_serialize(isResourceInstance, size, dataP, bufferP);
    }
#endif

#ifdef LWM2M_SUPPORT_CBOR
    case LWM2M_CONTENT_CBOR:
        #ifndef LWM2M_VERSION_1_0
        if (uriP != NULL && LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP))
        {
            if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
                size = dataP->value.asChildren.count;
                dataP = dataP->value.asChildren.array;
            }
            if(size != 1) return -1;
        }
        #endif
        return cbor_serialize(size, dataP, bufferP);
#endif

#ifdef LWM2M_CLIENT_MODE
    case LWM2M_CONTENT_LINK:
        return discover_serialize(NULL, uriP, NULL, size, dataP, bufferP);
#endif
#ifdef LWM2M_SUPPORT_JSON
    case LWM2M_CONTENT_JSON:
#ifdef LWM2M_OLD_CONTENT_FORMAT_SUPPORT
    case LWM2M_CONTENT_JSON_OLD:
#endif
        return json_serialize(uriP, size, dataP, bufferP);
#endif

#ifdef LWM2M_SUPPORT_SENML_JSON
    case LWM2M_CONTENT_SENML_JSON:
        return senml_json_serialize(uriP, size, dataP, bufferP);
#endif

    default:
        return -1;
    }
}

