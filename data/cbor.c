#include "internals.h"
#include <cbor.h>

CborError cbor_dump_recursive(CborValue *value, lwm2m_data_t* data, size_t dataSize, int nestingLevel)
{
    char indent[32];
    int idx = 0, lvl = nestingLevel;
 
    while (lvl--)
    {
        indent[idx++] = ' ';
        indent[idx++] = ' ';
    }
    indent[idx] = '\0';
 
    CborError err = CborErrorUnknownTag;
    CborTag tag;

    if (!cbor_value_is_tag(value)) {
        CborType type = cbor_value_get_type(value);
 
        switch (type)
        {
            case CborArrayType:
            case CborMapType:
            case CborTagType:
                break;
            case CborIntegerType:
                err = cbor_value_get_int64_checked(value, &data->value.asInteger);
                if (err != CborNoError)
                {
                    LOG("Error: cbor_value_get_int \n");
                    return err;
                }
                data->type = LWM2M_TYPE_INTEGER;
                // Print the parsed integer value
                LOG_ARG("Parsed integer value: %d\n", data->value.asInteger);
                break;
            case CborByteStringType:
            case CborTextStringType:
            case CborSimpleType:
            case CborBooleanType:
            case CborUndefinedType:
            case CborNullType:
            case CborHalfFloatType:
            case CborFloatType:
            case CborDoubleType:
            case CborInvalidType:
                break;
        }
        return CborNoError;
    }
    else {

        err = cbor_value_get_tag(value, &tag);
        if (err != CborNoError)
        {
            return err;
        }
        LOG_ARG("Parsed TAG: %lu (0x%x)\n", tag, tag);

        if (tag != 0xc0) {
            LOG("Error: Expected tag 0xc0\n");
        }

        // Advance to the integer value
        err = cbor_value_advance_fixed(value);
        if (err != CborNoError)
        {
            return err;
        }
        err = cbor_value_get_int64_checked(value, &data->value.asInteger);
        if (err != CborNoError)
        {
            LOG("Error: cbor_value_get_int \n");
            return err;
        }
        // Print the parsed integer value
        LOG_ARG("Parsed integer value: %d\n", data->value.asInteger);
        
        return CborNoError;
    }
}

int cbor_parse(lwm2m_uri_t * uriP,
              const uint8_t * buffer,
              size_t bufferLen,
              lwm2m_data_t ** dataP)
{
    CborParser parser;
    CborValue value;
    size_t dataSize = 0;
    lwm2m_data_t *data = NULL;
    CborError err;

    LOG_ARG("bufferLen: %d", bufferLen);

    LOG("CBOR parse --- >>>>>>>>>>>>> ");

    for (uint16_t i = 0; i <= bufferLen; i++)
    {
        lwm2m_printf("%x", buffer[i]);
    }

    LOG("CBOR parse --- >>>>>>>>>>>>> ");

    *dataP = NULL;
    
    /// Initialize a CBOR parser
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

    /// Allocate memory for data if needed
    data = lwm2m_data_new(dataSize + 1);
    if (data == NULL) {
        return -1; // Memory allocation failed
    }
    data->id = uriP->resourceId;

    err = cbor_dump_recursive(&value, data, dataSize, 0);
    if (err != CborNoError)
    {
        LOG_ARG("CBOR parsing failure at offset %ld: %s", value.source.ptr - buffer, cbor_error_string(err));
        return err;
    }

    *dataP = data;
    dataSize++;

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
    
    // lwm2m_free(data); // Free allocated memory

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

    // Calculate the size needed for the encoder buffer
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

    for (uint16_t i = 0; i < length; i++)
    {
        lwm2m_printf("%x", bufferP[i]);
    }

    // Clean up
    free(encoderBuffer);

    LOG_ARG("returning %u", length);
    return length;
}

