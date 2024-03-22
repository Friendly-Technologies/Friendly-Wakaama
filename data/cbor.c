#include "internals.h"
#include <cbor.h>

int cbor_parse(lwm2m_uri_t * uriP,
              const uint8_t * buffer,
              size_t bufferLen,
              lwm2m_data_t ** dataP)
{
    CborParser parser;
    CborValue value;
    CborTag tag;
    CborError err;
    size_t dataSize = 0;
    lwm2m_data_t *data = NULL;

    LOG_ARG("bufferLen: %d", bufferLen);

    LOG("CBOR parse --- >>>>>>>>>>>>> ");

    for (uint16_t i = 0; i <= bufferLen; i++)
    {
        lwm2m_printf("%x", buffer[i]);
    }

    LOG("CBOR parse --- >>>>>>>>>>>>> ");

    *dataP = NULL;
    
    err = cbor_parser_init(buffer, bufferLen, 0, &parser, &value);    
    if (err != CborNoError)
    {
        LOG("cbor_parser_init FAILED");
        return err;
    }

    err = cbor_value_validate(&value, CborValidateStrictMode);
    if (err != CborNoError)
    {
        LOG("cbor_value_validate FAILED");
        LOG_ARG("CBOR parsing failure at offset %ld: %s", value.source.ptr - buffer, cbor_error_string(err));
        return err;
    }

    /// Allocate memory for the data 
    data = lwm2m_data_new(dataSize + 1);
    if (data == NULL) {
        lwm2m_data_free(dataSize, data);
        return 0; // Memory allocation failed
    }
    data->id = uriP->resourceId; // setting data->id to the resource ID

    if (!cbor_value_is_tag(&value)) 
    {
        CborType type = cbor_value_get_type(&value);
        LOG_ARG("CBOR type 0x%x", type);
 
        switch (type)
        {
            case CborArrayType:
            case CborMapType:
            case CborTagType:
                break;
            case CborSimpleType:
            case CborIntegerType:
                err = cbor_value_get_int64_checked(&value, &data->value.asInteger);
                if (err != CborNoError)
                {
                    LOG("Error: cbor_value_get_int64_checked \n");
                    return err;
                }
                data->type = LWM2M_TYPE_INTEGER;
                LOG_ARG("Parsed integer value: %d\n", &data->value.asInteger);
                break;
            case CborByteStringType:
                err = cbor_value_get_byte_string_chunk(&value, (const uint8_t**)&data->value.asBuffer.buffer, &data->value.asBuffer.length, &value);
                if (err != CborNoError)
                {
                    LOG("Error: cbor_value_get_byte_string_chunk \n");
                    return err;
                }
                data->type = LWM2M_TYPE_OPAQUE;
                break;
            case CborTextStringType:
                {
                    char *temp = NULL;
                    size_t temPlen = 0;

                    err = cbor_value_calculate_string_length(&value, &temPlen);
                    if (err != CborNoError) {
                        LOG("Error: cbor_value_calculate_string_length\n");
                        return err;
                    }

                    temp = (char *)malloc(temPlen + 1); // +1 for null terminator
                    if (temp == NULL) {
                        LOG("Error: Memory allocation failed\n");
                        return CborErrorOutOfMemory;
                    }

                    // Copy chunks of the text string until all chunks are retrieved
                    char *currPos = temp;
                    do {
                        size_t chunkLen = temPlen - (currPos - temp);
                        err = cbor_value_copy_text_string(&value, currPos, &chunkLen, &value);
                        if (err != CborNoError) {
                            LOG("Error: cbor_value_copy_text_string\n");
                            free(temp); // Clean up allocated memory
                            return err;
                        }

                        // Move the current position pointer to the end of the copied chunk
                        currPos += chunkLen;

                    } while (!cbor_value_at_end(&value));

                    // Null-terminate the string
                    *currPos = '\0';

                    // Set the buffer and its length in the data structure
                    data->value.asBuffer.buffer = (uint8_t *)temp;
                    data->value.asBuffer.length = temPlen;

                    // Set the type of the data
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
                LOG_ARG("Parsed bool value: %d\n", &data->value.asBoolean);
                break;
            case CborDoubleType:
            case CborFloatType:
            case CborHalfFloatType:
                err = cbor_value_get_double(&value, &data->value.asFloat);
                if (err != CborNoError)
                {
                    LOG("Error: cbor_value_get_double \n");
                    return err;
                }
                data->type = LWM2M_TYPE_FLOAT;
                LOG_ARG("Parsed float value: %d\n", &data->value.asFloat);
                break;
            case CborUndefinedType:
            case CborNullType:
            case CborInvalidType:
                LOG_ARG("CborInvalidType 0x%x\n", CborInvalidType);
                break;
        }
    }
    else 
    {
        // For the TIME-CBOR-type - temporary solution
        err = cbor_value_get_tag(&value, &tag);
        if (err != CborNoError)
        {
            return err;
        }
        LOG_ARG("Parsed TAG: %lu (0x%x)\n", tag, tag);

        if (tag != 0xc0) {
            LOG("Error: Expected tag 0xc0\n");
        }

        err = cbor_value_advance_fixed(&value);
        if (err != CborNoError)
        {
            return err;
        }
        err = cbor_value_get_int64_checked(&value, &data->value.asInteger);
        if (err != CborNoError)
        {
            LOG("Error: cbor_value_get_int \n");
            return err;
        }
        LOG_ARG("Parsed integer value: %d\n", &data->value.asInteger);
        
    }

    // *dataP = data;
    dataSize++; // temporary we set dataSize == 1
    memcpy(dataP, data, dataSize * sizeof(lwm2m_data_t));
    lwm2m_data_free(dataSize, data);

    LOG_ARG("cbor_parse: uriP.objectId = %d, ", uriP->objectId);
    LOG_ARG("cbor_parse: uriP.instanceId = %d, ", uriP->instanceId);
    LOG_ARG("cbor_parse: uriP.resourceId = %d, ", uriP->resourceId);
    LOG("---------------------------");
    LOG_ARG("cbor_parse: dataP.type = %d, ", data->type);
    LOG_ARG("cbor_parse: dataP.ID = %d, ", data->id);
    LOG_ARG("cbor_parse: dataP.asBoolean = %d, ", data->value.asBoolean);
    LOG_ARG("cbor_parse: dataP.asInteger = %d, ", data->value.asInteger);
    LOG_ARG("cbor_parse: dataP.asUnsigned = %lu, ", data->value.asUnsigned);
    LOG_ARG("cbor_parse: dataP.asFloat = %f, ", data->value.asFloat);
    LOG_ARG("cbor_parse: dataP.asBufferLength = %d, ", data->value.asBuffer.length);
    LOG_ARG("cbor_parse: dataP.asChildrenCount = %d, ", data->value.asChildren.count);
    LOG_ARG("cbor_parse: dataP.asObjLink.objectId = %d, ", data->value.asObjLink.objectId);
    LOG_ARG("cbor_parse: dataP.asObjLink.objectInstanceId = %d, ", data->value.asObjLink.objectInstanceId);

    // Return the size of the parsed data
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

    LOG_ARG("cbor_serialize: dataP.type = %d, ", dataP->type);
    LOG_ARG("cbor_serialize: dataP.ID = %d, ", dataP->id);
    LOG_ARG("cbor_serialize: dataP.asBoolean = %d, ", dataP->value.asBoolean);
    LOG_ARG("cbor_serialize: dataP.asInteger = %d, ", dataP->value.asInteger);
    LOG_ARG("cbor_serialize: dataP.asUnsigned = %lu, ", dataP->value.asUnsigned);
    LOG_ARG("cbor_serialize: dataP.asFloat = %f, ", dataP->value.asFloat);
    LOG_ARG("cbor_serialize: dataP.asBufferLength = %d, ", dataP->value.asBuffer.length);
    LOG_ARG("cbor_serialize: dataP.asChildrenCount = %d, ", dataP->value.asChildren.count);
    LOG_ARG("cbor_serialize: dataP.asObjLink.objectId = %d, ", dataP->value.asObjLink.objectId);
    LOG_ARG("cbor_serialize: dataP.asObjLink.objectInstanceId = %d, ", dataP->value.asObjLink.objectInstanceId);

    *bufferP = NULL;

    size_t bufferSize = 1024; // Start with a reasonable default size
    uint8_t *encoderBuffer = (uint8_t *)malloc(bufferSize);
    if (encoderBuffer == NULL) {
        return 0; // Memory allocation failed
    }

    // Initialize the CborEncoder with the encoder buffer
    cbor_encoder_init(&encoder, encoderBuffer, bufferSize, 0);

    // Serialize the data based on its type
    switch (dataP->type) {
        case LWM2M_TYPE_UNDEFINED:
        case LWM2M_TYPE_OBJECT:
        case LWM2M_TYPE_OBJECT_INSTANCE:
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
            break;
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

    // Allocate memory for the serialized buffer
    *bufferP = (uint8_t *)malloc(length);
    if (*bufferP == NULL) {
        free(encoderBuffer);
        return 0; // Memory allocation failed
    }

    // Copying the serialized data to the output buffer
    memcpy(*bufferP, encoderBuffer, length);
    free(encoderBuffer);

    LOG_ARG("returning %u", length);
    return length;
}

