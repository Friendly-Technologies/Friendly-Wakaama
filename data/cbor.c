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
 
    // while (!cbor_value_at_end(value))
    // {
        CborError err;
        CborType type = cbor_value_get_type(value);
        LOG_ARG("CBOR  CborType type = 0x%x (%d) ", type, type);
        LOG_ARG("CBOR 1 cbor_value_at_end (%d) ", cbor_value_at_end(value));

        switch (type)
        {
            case CborArrayType:
            case CborMapType:
            {
                /* Recursive type. */
                CborValue recursed;
                // LOG_ARG("CBOR %s%s", indent, type == CborArrayType ? "Array[" : "Map[");
                err = cbor_value_enter_container(value, &recursed);
                if (err != CborNoError)
                {
                    lwm2m_free(data); // Free allocated memory/* Parse error. */
                    return err;
                }
                err = cbor_dump_recursive(&recursed, data, dataSize, nestingLevel + 1);
                if (err != CborNoError)
                {
                    /* Parse error. */
                    return err;
                }
                err = cbor_value_leave_container(value, &recursed);
                if (err != CborNoError)
                {
                    lwm2m_free(data); // Free allocated memory /* Parse error. */
                    return err;
                }
                // LOG_ARG( "CBOR [%s]", indent);
                // continue;
                break;
            }

            case CborIntegerType:
            {
                err = cbor_value_get_int64(value, &data->value.asInteger); // Example: Parse an integer
                if (err != CborNoError) {
                    // Error handling
                    lwm2m_free(data); // Free allocated memory // Free allocated memory
                    return err;
                }
                break;
            }

            case CborByteStringType:
            {
                // err = cbor_value_copy_byte_string(&value, &data->value.asChildren.array, &data->value.asChildren.count, &value);
                if (err != CborNoError)
                {
                    /* Parse error. */
                    lwm2m_free(data); // Free allocated memory
                    return err;
                }
                // continue;
                break;
            }

            case CborTextStringType:
            {
                char strData[64];
                size_t n = 64;
                err = cbor_value_copy_text_string(value, strData, &n, value);
                if (err != CborNoError)
                {
                    /* Parse error. */
                    lwm2m_free(data); // Free allocated memory
                    return err;
                }
                // LOG_ARG("CBOR %sTextString: %s", indent, data);
                // continue;
                break;
            }

            case CborTagType:
            {
                CborTag tag;
                err = cbor_value_get_tag(value, &tag); // can't fail
                if (err != CborNoError)
                {
                    /* Parse error. */
                    lwm2m_free(data); // Free allocated memory
                    return err;
                }
                // break;
                LOG_ARG("CBOR 2 CborType type = 0x%x (%d) ", type, type);
                err = cbor_value_advance(value);
                if (err != CborNoError)
                {
                    /* Parse error. */
                    lwm2m_free(data); // Free allocated memory
                    return err;
                }
                LOG_ARG("CBOR 3 CborType type = 0x%x (%d) ", type, type);

                // if (value == CborIntegerType)
                // {
                //     err = cbor_value_get_int64(value, &data->value.asInteger); // Example: Parse an integer
                //     if (err != CborNoError) {
                //         // Error handling
                //         lwm2m_free(data); // Free allocated memory // Free allocated memory
                //         return err;
                //     }
                //     break;
                // }
                break;
            }

            

            case CborSimpleType:
            {
                uint8_t sType;
                err = cbor_value_get_simple_type(value, &sType); // can't fail
                    if (err != CborNoError)
                {
                    /* Parse error. */
                    lwm2m_free(data); // Free allocated memory
                    return err;
                }
                break;
            }

            case CborNullType:
            {
                // LOG_ARG("CBOR %sNull", indent);
                break;
            }

            case CborUndefinedType:
            {
                // LOG_ARG("CBOR %sUndefined", indent);
                lwm2m_free(data); // Free allocated memory
                break;
            }

            case CborBooleanType:
            {
                err = cbor_value_get_boolean(value, &data->value.asBoolean );
                if (err != CborNoError)
                {
                    /* Parse error. */
                    lwm2m_free(data); // Free allocated memory
                    return err;
                }
                break;
            }

            case CborDoubleType:
            case CborFloatType:
            case CborHalfFloatType:
            {
                err = cbor_value_get_double(value, &data->value.asFloat );
                if (err != CborNoError)
                {
                    /* Parse error. */
                    lwm2m_free(data); // Free allocated memory
                    return err;
                }
                break;
            }

            case CborInvalidType:
            {
                /* Can't happen. */
                // LOG_ARG("CBOR %sInvalidType", indent);
                lwm2m_free(data); // Free allocated memory
                break;
            }

            err = cbor_value_advance_fixed(value);
            if (err != CborNoError)
            {
                /* Parse error. */
                lwm2m_free(data); // Free allocated memory
                return err;
            }
        
            // Move to the next CBOR data item
            // err = cbor_value_advance(&value);
            // if (err != CborNoError) {
            //     // Error handling
            //     lwm2m_free(data); // Free allocated memory
            //     return err;
            // }

            dataSize++;
            LOG_ARG("CBOR 2 cbor_value_at_end (%d) ", cbor_value_at_end(value));
        } /// !switch
    // } /// !while
    return CborNoError;
}

int cbor_parse(const uint8_t * buffer,
              size_t bufferLen,
              lwm2m_data_t ** dataP)
{
    CborParser parser;
    CborValue value;
    size_t dataSize = 0;
    lwm2m_data_t *data = NULL;
    CborError err;

    LOG_ARG("bufferLen: %d", bufferLen);

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
    if (dataSize == 0) {
        data = lwm2m_data_new(dataSize + 1);
        if (data == NULL) {
            return -1; // Memory allocation failed
        }
    }

    err = cbor_dump_recursive(&value, data, dataSize, 0);
    if (err != CborNoError)
    {
        LOG_ARG("CBOR parsing failure at offset %ld: %s", value.source.ptr - buffer, cbor_error_string(err));
    }

    *dataP = data;
    lwm2m_free(data); // Free allocated memory

    // Return the size of the parsed data
    return dataSize;
}

int cbor_serialize(bool isResourceInstance, 
                  int size,
                  lwm2m_data_t * dataP,
                  uint8_t ** bufferP)
{
    //Encode
    uint8_t buff[100];

    CborEncoder encoder;
    // CborError err;
    // CborType type = cbor_value_get_type(it);

    int length;

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


    LOG_ARG("isResourceInstance: %s, size: %d", isResourceInstance?"true":"false", size);

    cbor_encoder_init(&encoder, buff, sizeof(buff), 0);

    *bufferP = NULL;
    // length = prv_getLength(size, dataP);
    if (length <= 0) return length;

    *bufferP = (uint8_t *)lwm2m_malloc(length);
    if (*bufferP == NULL) return 0;

    if (length < 0)
    {
        lwm2m_free(*bufferP);
        *bufferP = NULL;
    }

    LOG_ARG("returning %u", length);

    return length;


    return 0;
}
