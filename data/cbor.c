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
    *dataP = NULL;

    LOG_ARG("bufferLen: %d", bufferLen);
    LOG_URI(uriP);
    if (uriP == NULL){
        return -1;
    }
    
    err = cbor_parser_init(buffer, bufferLen, 0, &parser, &value);    
    if (err != CborNoError)
    {
        LOG_ARG("cbor_parser_init FAILED with error %d", err);
        return -1;
    }

    err = cbor_value_validate(&value, CborValidateStrictMode);
    if (err != CborNoError)
    {
        LOG_ARG("CBOR parsing failure at offset %ld: %s", value.source.ptr - buffer, cbor_error_string(err));
        return -1;
    }

    data = lwm2m_data_new(dataSize + 1);
    if (data == NULL) {
        lwm2m_free(data);
        LOG("lwm2m_data_new FAILED");
        return -1; 
    }
    if (!LWM2M_URI_IS_SET_RESOURCE(uriP)){
        lwm2m_free(data);
        LOG("LWM2M_URI_IS_SET_RESOURCE FAILED");
        return -1; 
    }
    data->id = uriP->resourceId;

    CborType type = cbor_value_get_type(&value);
    switch (type)
    {
        case CborArrayType:
        case CborMapType:
            LOG_ARG("Cbor Yet Unsupported Type 0x%x\n", CborTagType);
            return -1; 
            break;
        case CborTagType:
        {
            CborTag tag;
            err = cbor_value_get_tag(&value, &tag);
            if (err != CborNoError)
            {
                LOG("Error: cbor_value_get_tag \n");
                return -1;
            }
            err = cbor_value_advance_fixed(&value);
            if (err != CborNoError)
            {
                LOG("Error: cbor_value_advance_fixed \n");
                return -1;
            }
            err = cbor_value_get_int64_checked(&value, &data->value.asInteger);
            if (err != CborNoError)
            {
                LOG("Error: cbor_value_get_int64_checked \n");
                return -1;
            }
            data->type = LWM2M_TYPE_TIME; /// now it's the TIME is the only type with a TAG
            break;
        }
        case CborSimpleType:
        case CborIntegerType:
            if (cbor_value_is_unsigned_integer(&value)){
                err = cbor_value_get_uint64(&value, &data->value.asUnsigned);
                if (err != CborNoError)
                {
                    LOG_ARG("cbor_value_get_uint64 FAILED with error %d", err);
                    lwm2m_free(data);
                    return -1;
                }
                data->type = LWM2M_TYPE_UNSIGNED_INTEGER;
            }
            else {
                err = cbor_value_get_int64_checked(&value, &data->value.asInteger);
                if (err != CborNoError)
                {
                    LOG_ARG("cbor_value_get_int64_checked FAILED with error %d", err);
                    lwm2m_free(data);
                    return -1;
                }
                data->type = LWM2M_TYPE_INTEGER;
            }
            break;
        case CborByteStringType:
            {
                uint8_t *temp = NULL;
                size_t temPlen = 0;
              
                err = cbor_value_calculate_string_length(&value, &temPlen);
                if (err != CborNoError) {
                    LOG_ARG("Error%d: cbor_value_calculate_string_length\n", err);
                    lwm2m_free(data);
                    return -1;
                }

                temp = (uint8_t *)lwm2m_malloc(temPlen);
                if (temp == NULL) {
                    LOG("Error: lwm2m_malloc failed\n");
                    lwm2m_free(data);
                    return -1;
                }

                err = cbor_value_dup_byte_string(&value, &temp, &temPlen, &value);
                if (err!= CborNoError){
                    LOG_ARG("Error%d: cbor_value_dup_byte_string\n", err);
                    lwm2m_free(data);
                    return -1;     
                }

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
                    lwm2m_free(data);
                    return -1;
                }

                temp = (char *)lwm2m_malloc(temPlen);
                if (temp == NULL) {
                    LOG("Error: Memory allocation failed\n");
                    lwm2m_free(data);
                    return -1;
                }

                err = cbor_value_dup_text_string(&value, &temp, &temPlen, &value);
                if (err!= CborNoError){
                    LOG_ARG("Error%d: cbor_value_dup_text_string\n", err);
                    lwm2m_free(data);
                    return -1;
                }

                data->value.asBuffer.buffer = (uint8_t *)temp;
                data->value.asBuffer.length = temPlen;
                data->type = LWM2M_TYPE_STRING;
                break;
            }
        case CborBooleanType:
            err = cbor_value_get_boolean(&value, &data->value.asBoolean);
            if (err != CborNoError)
            {
                LOG_ARG("cbor_value_get_boolean FAILED with error %d", err);
                lwm2m_free(data);
                return -1;
            }
            data->type = LWM2M_TYPE_BOOLEAN;
            break;
        case CborDoubleType:
            err = cbor_value_get_double(&value, &data->value.asFloat);
            if (err != CborNoError)
            {
                LOG_ARG("cbor_value_get_double FAILED with error %d", err);
                lwm2m_free(data);
                return -1;
            }
            data->type = LWM2M_TYPE_FLOAT;
            break;
        case CborFloatType:
            {
                float tempFloatValue;
                err = cbor_value_get_float(&value, &tempFloatValue);
                if (err != CborNoError)
                {
                    LOG_ARG("cbor_value_get_float FAILED with error %d", err);
                    lwm2m_free(data);
                    return -1;
                }
                data->value.asFloat = (double)tempFloatValue;
                data->type = LWM2M_TYPE_FLOAT;
                break;
            }
        case CborHalfFloatType:
            {
                float tempFloatValue;
                err = cbor_value_get_half_float_as_float(&value, &tempFloatValue);
                if (err != CborNoError)
                {
                    LOG_ARG("cbor_value_get_half_float_as_float FAILED with error %d", err);
                    lwm2m_free(data);
                    return -1;
                }
                data->value.asFloat = (double)tempFloatValue;
                data->type = LWM2M_TYPE_FLOAT;
                break;
            }
        case CborUndefinedType:
        case CborNullType:
        case CborInvalidType:
            LOG_ARG("CborInvalidType 0x%x\n", CborInvalidType);
            lwm2m_free(data);
            return -1; 
            break;
        default:
            return -1; 
            break;
    }///!switch-case

    if (data == NULL) {
        lwm2m_free(data);
        LOG("data is empty ");
        return 0; 
    }
    dataSize++; /// if all is OK, temporary we set dataSize == 1
    *dataP = data;
    
    return dataSize;
}

#define DEFAULT_BUFF_SIZE (1024UL)

int cbor_serialize(bool isResourceInstance, 
                   int size,
                   lwm2m_data_t *dataP,
                   uint8_t **bufferP) 
{
    CborEncoder encoder;
    CborError err;
    int length = 0;
    *bufferP = NULL;
    size_t bufferSize = DEFAULT_BUFF_SIZE; /// TODO Add the ability to calculate the required memory for the cbor buffer
    uint8_t *encoderBuffer = (uint8_t *)lwm2m_malloc(bufferSize);
    if (encoderBuffer == NULL) {
        LOG("lwm2m_malloc FAILED");
        return -1;
    }

    cbor_encoder_init(&encoder, encoderBuffer, bufferSize, 0);

    switch (dataP->type) {
        case LWM2M_TYPE_UNDEFINED:
        case LWM2M_TYPE_OBJECT:
        case LWM2M_TYPE_OBJECT_INSTANCE:
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
            return -1;
            break;
        case LWM2M_TYPE_TIME:
        {
            err = cbor_encode_tag(&encoder, CborUnixTime_tTag);/// now unixTime is the only tag-ed type
            if (err != CborNoError) {
                free(encoderBuffer);
                return err; 
            }
            err = cbor_encode_int(&encoder, dataP->value.asInteger);
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
                lwm2m_free(encoderBuffer);
                LOG_ARG("cbor_encode_text_string/obj_link FAILED with error %d", err);
                return -1; 
            }
            length = cbor_encoder_get_buffer_size(&encoder, encoderBuffer);
            break;
        }
        case LWM2M_TYPE_OPAQUE:
            err = cbor_encode_byte_string(&encoder, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
            if (err != CborNoError) {
                lwm2m_free(encoderBuffer);
                LOG_ARG("cbor_encode_byte_string FAILED with error %d", err);
                return -1; 
            }
            length = cbor_encoder_get_buffer_size(&encoder, encoderBuffer);
            break;
        case LWM2M_TYPE_STRING:
        case LWM2M_TYPE_CORE_LINK:
            err = cbor_encode_text_string(&encoder,(char *) dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
            if (err != CborNoError) {
                lwm2m_free(encoderBuffer);
                LOG_ARG("cbor_encode_text_string FAILED with error %d", err);
                return -1; 
            }
            length = cbor_encoder_get_buffer_size(&encoder, encoderBuffer);
            break;
        case LWM2M_TYPE_INTEGER:
            err = cbor_encode_int(&encoder,dataP->value.asInteger);
            if (err != CborNoError) {
                lwm2m_free(encoderBuffer);
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            length = cbor_encoder_get_buffer_size(&encoder, encoderBuffer);
            break;
        case LWM2M_TYPE_UNSIGNED_INTEGER:
            err = cbor_encode_uint(&encoder, dataP->value.asUnsigned);
            if (err != CborNoError) {
                lwm2m_free(encoderBuffer);
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            length = cbor_encoder_get_buffer_size(&encoder, encoderBuffer);
            break;
        case LWM2M_TYPE_FLOAT:
            err = cbor_encode_double(&encoder, dataP->value.asFloat);
            if (err != CborNoError) {
                lwm2m_free(encoderBuffer);
                LOG_ARG("cbor_encode_double FAILED with error %d", err);
                return -1; 
            }
            length = cbor_encoder_get_buffer_size(&encoder, encoderBuffer);
            break;
        case LWM2M_TYPE_BOOLEAN:
            err = cbor_encode_boolean(&encoder, dataP->value.asBoolean);
            if (err != CborNoError) {
                lwm2m_free(encoderBuffer);
                LOG_ARG("cbor_encode_boolean FAILED with error %d", err);
                return -1; 
            }
            length = cbor_encoder_get_buffer_size(&encoder, encoderBuffer);
            break;
        default:
            lwm2m_free(encoderBuffer);
            return -1;
            break;
    }

    *bufferP = (uint8_t *)lwm2m_malloc(length);
    if (*bufferP == NULL) {
        lwm2m_free(encoderBuffer);
        LOG("bufferP is empty");
        return 0; 
    }

    if (encoderBuffer == NULL) {
        LOG("encoderBuffer is empty");
        return 0; 
    }

    memcpy(*bufferP, encoderBuffer, length);
    lwm2m_free(encoderBuffer);

    LOG_ARG("returning %u", length);
    return length;
}

