#include "internals.h"
#include <cbor.h>

int cbor_parse(lwm2m_uri_t * uriP,
              const uint8_t * buffer,
              size_t bufferLen,
              lwm2m_data_t ** dataP)
{
    CborParser parser;
    CborValue value;
    CborError err;
    size_t dataSize = 0;
    lwm2m_data_t *data = NULL;

    LOG_ARG("bufferLen: %d", bufferLen);

    *dataP = NULL;
    
    err = cbor_parser_init(buffer, bufferLen, 0, &parser, &value);    
    if (err != CborNoError)
    {
        LOG_ARG("cbor_parser_init FAILED with error %d", err);
        return err;
    }

    err = cbor_value_validate(&value, CborValidateStrictMode);
    if (err != CborNoError)
    {
        LOG_ARG("CBOR parsing failure at offset %ld: %s", value.source.ptr - buffer, cbor_error_string(err));
        return err;
    }

    data = lwm2m_data_new(dataSize + 1);
    if (data == NULL) {
        lwm2m_data_free(dataSize, data);
        return 0; // Memory allocation failed
    }
    data->id = uriP->resourceId; // setting data->id to the resource ID

    CborType type = cbor_value_get_type(&value);

    switch (type)
    {
        case CborArrayType:
        case CborMapType:
        case CborTagType:
            LOG_ARG("Cbor Yet Unsupported Type 0x%x\n", CborTagType);
            break;
        case CborSimpleType:
        case CborIntegerType:
            if (cbor_value_is_unsigned_integer(&value)){
                err = cbor_value_get_uint64(&value, &data->value.asUnsigned);
                if (err != CborNoError)
                {
                    LOG("Error: cbor_value_get_uint64 \n");
                    return err;
                }
                data->type = LWM2M_TYPE_UNSIGNED_INTEGER;
                LOG_ARG("Parsed uint value: %d\n", &data->value.asUnsigned);
            }
            else {
                err = cbor_value_get_int64_checked(&value, &data->value.asInteger);
                if (err != CborNoError)
                {
                    LOG("Error: cbor_value_get_int64_checked \n");
                    return err;
                }
                data->type = LWM2M_TYPE_INTEGER;
                LOG_ARG("Parsed integer value: %d\n", &data->value.asInteger);
            }
            break;
        case CborByteStringType:
            {
                uint8_t *temp = NULL;
                size_t temPlen = 0;

                err = cbor_value_calculate_string_length(&value, &temPlen);
                if (err != CborNoError) {
                    LOG_ARG("Error%d: cbor_value_calculate_string_length\n", err);
                    return err;
                }

                temp = (uint8_t *)malloc(temPlen + 1); // +1 for null terminator
                if (temp == NULL) {
                    LOG("Error: Memory allocation failed\n");
                    return CborErrorOutOfMemory;
                }

                uint8_t *currPos = temp;
                do {
                    size_t chunkLen = temPlen - (currPos - temp);
                    err = cbor_value_copy_byte_string(&value, currPos, &chunkLen, &value);
                    if (err != CborNoError) {
                        LOG("Error: cbor_value_copy_text_string\n");
                        free(temp); // Clean up allocated memory
                        return err;
                    }
                    currPos += chunkLen;

                } while (!cbor_value_at_end(&value));
                
                *currPos = '\0'; /// Null-terminate the string

                data->value.asBuffer.buffer = (uint8_t *)temp;
                data->value.asBuffer.length = temPlen;
                data->type = LWM2M_TYPE_OPAQUE;
                break;
            }
        case CborTextStringType:
            {
                char *temp = NULL;
                size_t temPlen = 0;

                err = cbor_value_calculate_string_length(&value, &temPlen);
                if (err != CborNoError) {
                    LOG_ARG("Error%d: cbor_value_calculate_string_length\n", err);
                    return err;
                }

                temp = (char *)malloc(temPlen + 1); // +1 for null terminator
                if (temp == NULL) {
                    LOG("Error: Memory allocation failed\n");
                    return CborErrorOutOfMemory;
                }

                char *currPos = temp;
                do {
                    size_t chunkLen = temPlen - (currPos - temp);
                    err = cbor_value_copy_text_string(&value, currPos, &chunkLen, &value);
                    if (err != CborNoError) {
                        LOG("Error: cbor_value_copy_text_string\n");
                        free(temp); 
                        return err;
                    }
                    currPos += chunkLen;/// Move the current position pointer to the end of the copied chunk

                } while (!cbor_value_at_end(&value));
                *currPos = '\0'; /// Null-terminate the string

                data->value.asBuffer.buffer = (uint8_t *)temp;
                data->value.asBuffer.length = temPlen;
                data->type = LWM2M_TYPE_STRING;
                break;
            }
        case CborBooleanType:
            err = cbor_value_get_boolean(&value, &data->value.asBoolean);
            if (err != CborNoError)
            {
                LOG("Error: cbor_value_get_boolean \n");
                return err;
            }
            data->type = LWM2M_TYPE_BOOLEAN;
            break;
        case CborDoubleType:
            err = cbor_value_get_double(&value, &data->value.asFloat);
            if (err != CborNoError)
            {
                LOG("Error: cbor_value_get_double \n");
                return err;
            }
            data->type = LWM2M_TYPE_FLOAT;
            break;
        case CborFloatType:
            err = cbor_value_get_float(&value, (float *)&data->value.asFloat);
            if (err != CborNoError)
            {
                LOG("Error: cbor_value_get_float \n");
                return err;
            }
            data->type = LWM2M_TYPE_FLOAT;
            break;
        case CborHalfFloatType:
            err = cbor_value_get_half_float_as_float(&value, (float *)&data->value.asFloat);
            if (err != CborNoError)
            {
                LOG("Error: cbor_value_get_half_float_as_float \n");
                return err;
            }
            data->type = LWM2M_TYPE_FLOAT;
            break;
        case CborUndefinedType:
        case CborNullType:
        case CborInvalidType:
            LOG_ARG("CborInvalidType 0x%x\n", CborInvalidType);
            break;
    }///!switch-case

    dataSize++; /// temporary we set dataSize == 1
    *dataP = data;
    
    return dataSize;
}

int cbor_serialize(bool isResourceInstance, 
                   int size,
                   lwm2m_data_t *dataP,
                   uint8_t **bufferP) 
{
    CborEncoder encoder;
    CborError err;
    int length = 0;
    *bufferP = NULL;
    size_t bufferSize = 1024; /// Start with a reasonable default size
    uint8_t *encoderBuffer = (uint8_t *)malloc(bufferSize);
    if (encoderBuffer == NULL) {
        return 0; /// Memory allocation failed
    }

    cbor_encoder_init(&encoder, encoderBuffer, bufferSize, 0);

    switch (dataP->type) {
        case LWM2M_TYPE_UNDEFINED:
        case LWM2M_TYPE_OBJECT:
        case LWM2M_TYPE_OBJECT_INSTANCE:
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
            break;
        case LWM2M_TYPE_TIME:
            {
                CborTag tag;
                err = cbor_encode_tag(&encoder, tag);
                if (err != CborNoError) {
                    free(encoderBuffer);
                    return err; 
                }
                err = cbor_encode_int(&encoder,dataP->value.asInteger);
                if (err != CborNoError) {
                    free(encoderBuffer);
                    return err; 
                }
                length = cbor_encoder_get_buffer_size(&encoder, encoderBuffer);
                break;
            }
        case LWM2M_TYPE_OBJECT_LINK:
        {
            char buffer[12]; // Sufficient buffer to hold the string representation (e.g., "65535:65535" -> 5 + 1 + 5 == 11 symbols)
            snprintf(buffer, sizeof(buffer), "%u:%u", dataP->value.asObjLink.objectId, dataP->value.asObjLink.objectInstanceId);
            err = cbor_encode_text_string(&encoder, buffer, strlen(buffer));
            if (err != CborNoError) {
                free(encoderBuffer);
                return err; 
            }
            length = cbor_encoder_get_buffer_size(&encoder, encoderBuffer);
            break;
        }
        case LWM2M_TYPE_OPAQUE:
            err = cbor_encode_byte_string(&encoder, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
            if (err != CborNoError) {
                free(encoderBuffer);
                return err; 
            }
            length = cbor_encoder_get_buffer_size(&encoder, encoderBuffer);
            break;
        case LWM2M_TYPE_STRING:
        case LWM2M_TYPE_CORE_LINK:
            err = cbor_encode_text_string(&encoder,(char *) dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
            if (err != CborNoError) {
                free(encoderBuffer);
                return err; 
            }
            length = cbor_encoder_get_buffer_size(&encoder, encoderBuffer);
            break;
        case LWM2M_TYPE_INTEGER:
            err = cbor_encode_int(&encoder,dataP->value.asInteger);
            if (err != CborNoError) {
                free(encoderBuffer);
                return err; 
            }
            length = cbor_encoder_get_buffer_size(&encoder, encoderBuffer);
            break;
        case LWM2M_TYPE_UNSIGNED_INTEGER:
            err = cbor_encode_uint(&encoder, dataP->value.asUnsigned);
            if (err != CborNoError) {
                free(encoderBuffer);
                return err; 
            }
            length = cbor_encoder_get_buffer_size(&encoder, encoderBuffer);
            break;
        case LWM2M_TYPE_FLOAT:
            err = cbor_encode_double(&encoder, dataP->value.asFloat);
            if (err != CborNoError) {
                free(encoderBuffer);
                return err; 
            }
            length = cbor_encoder_get_buffer_size(&encoder, encoderBuffer);
            break;
        case LWM2M_TYPE_BOOLEAN:
            err = cbor_encode_boolean(&encoder, dataP->value.asBoolean);
            if (err != CborNoError) {
                free(encoderBuffer);
                return err; 
            }
            length = cbor_encoder_get_buffer_size(&encoder, encoderBuffer);
            break;
    }

    *bufferP = (uint8_t *)malloc(length);
    if (*bufferP == NULL) {
        free(encoderBuffer);
        return 0; /// Memory allocation failed
    }

    memcpy(*bufferP, encoderBuffer, length);
    free(encoderBuffer);

    LOG_ARG("returning %u", length);
    return length;
}

