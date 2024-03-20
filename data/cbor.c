#include "internals.h"
#include <cbor.h>

// static bool dumprecursive(CborValue *it, int nestingLevel)
// {
//     char indent[32];
//     int idx = 0, lvl = nestingLevel;
 
//     while (lvl--)
//     {
//         indent[idx++] = ' ';
//         indent[idx++] = ' ';
//     }
//     indent[idx] = '\0';
 
//     while (!cbor_value_at_end(it))
//     {
//         CborError err;
//         CborType type = cbor_value_get_type(it);
 
//         switch (type)
//         {
//         case CborArrayType:
//         case CborMapType:
//         {
//             /* Recursive type. */
//             CborValue recursed;
//             LOG_ARG("CBOR %s%s", indent, type == CborArrayType ? "Array[" : "Map[");
//             err = cbor_value_enter_container(it, &recursed);
//             if (err != CborNoError)
//             {
//                 /* Parse error. */
//                 return err;
//             }
//             err = dumprecursive(&recursed, nestingLevel + 1);
//             if (err != CborNoError)
//             {
//                 /* Parse error. */
//                 return err;
//             }
//             err = cbor_value_leave_container(it, &recursed);
//             if (err != CborNoError)
//             {
//                 /* Parse error. */
//                 return err;
//             }
//             LOG_ARG( "CBOR [%s]", indent);
//             continue;
//         }
 
//         case CborIntegerType:
//         {
//             int val;
//             err = cbor_value_get_int_checked(it, &val);
//             if (err != CborNoError)
//             {
//                 /* Parse error. */
//                 return err;
//             }
//             LOG_ARG("CBOR %sInteger: %d", indent, val);
//             break;
//         }
 
//         case CborByteStringType:
//         {
//             uint8_t data[64];
//             size_t n = 64;
//             err = cbor_value_copy_byte_string(it, data, &n, it);
//             if (err != CborNoError)
//             {
//                 /* Parse error. */
//                 return err;
//             }
//             LOG_ARG("CBOR %sByteString: ", indent);
//             LOG_ARG("CBOR size%d" , n);
 
//             continue;
//         }
 
//         case CborTextStringType:
//         {
//             char data[64];
//             size_t n = 64;
//             err = cbor_value_copy_text_string(it, data, &n, it);
//             if (err != CborNoError)
//             {
//                 /* Parse error. */
//                 return err;
//             }
//             LOG_ARG("CBOR %sTextString: %s", indent, data);
//             continue;
//         }
 
//         case CborTagType:
//         {
//             CborTag tag;
//             cbor_value_get_tag(it, &tag); // can't fail
//             LOG_ARG("CBOR %sTag(%d)", indent, tag);
//             break;
//         }
 
//         case CborSimpleType:
//         {
//             uint8_t sType;
//             cbor_value_get_simple_type(it, &sType); // can't fail
//             LOG_ARG("CBOR %sSimple(%u)", indent, sType);
//             break;
//         }
 
//         case CborNullType:
//         {
//             LOG_ARG("CBOR %sNull", indent);
//             break;
//         }
 
//         case CborUndefinedType:
//         {
//             LOG_ARG("CBOR %sUndefined", indent);
//             break;
//         }
 
//         case CborBooleanType:
//         {
//             bool val;
//             cbor_value_get_boolean(it, &val); // can't fail
//             LOG_ARG("CBOR %sBoolean: %s", indent, val ? "true" : "false");
//             break;
//         }
 
//         case CborDoubleType:
//         case CborFloatType:
//         case CborHalfFloatType:
//         {
//             /* Not managed. */
//             LOG_ARG("CBOR %sFloat: (not managed)", indent);
//             break;
//         }
 
//         case CborInvalidType:
//             /* Can't happen. */
//             LOG_ARG("CBOR %sInvalidType", indent);
//             break;
//         }
 
//         err = cbor_value_advance_fixed(it);
//         if (err != CborNoError)
//         {
//             /* Parse error. */
//             return err;
//         }
//     }
//     return CborNoError;
// }

int cbor_parse(const uint8_t * buffer,
              size_t bufferLen,
              lwm2m_data_t ** dataP)
{
    // Initialize a CBOR parser
    CborParser parser;
    CborValue value;
    size_t dataSize = 0;
    lwm2m_data_t *data = NULL;
    CborError err;

    LOG_ARG("bufferLen: %d", bufferLen);

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

    // while (!cbor_value_at_end(&value)) 
    // {
        data = lwm2m_data_new(dataSize + 1);

        if (dataSize >= 1)
        {
            if (data == NULL)
            {
                lwm2m_data_free(dataSize, *dataP);
                return 0;
            }
            else
            {
                memcpy(data, *dataP, dataSize * sizeof(lwm2m_data_t));
                lwm2m_free(*dataP);
            }
        }

        CborType type = cbor_value_get_type(&value);

        switch (type)
        {
            case CborArrayType:
            case CborMapType:
            {
                /* Recursive type. */
                CborValue recursed;
                // LOG_ARG("CBOR %s%s", indent, type == CborArrayType ? "Array[" : "Map[");
                err = cbor_value_enter_container(&value, &recursed);
                if (err != CborNoError)
                {
                    lwm2m_free(data); // Free allocated memory/* Parse error. */
                    return err;
                }
                // err = dumprecursive(&recursed, nestingLevel + 1);
                // if (err != CborNoError)
                // {
                //     /* Parse error. */
                //     return err;
                // }
                err = cbor_value_leave_container(&value, &recursed);
                if (err != CborNoError)
                {
                    lwm2m_free(data); // Free allocated memory /* Parse error. */
                    return err;
                }
                // LOG_ARG( "CBOR [%s]", indent);
                break;
            }
 
            case CborIntegerType:
            {
                err = cbor_value_get_int64(&value, &data->value.asInteger); // Example: Parse an integer
                if (err != CborNoError) {
                    // Error handling
                    lwm2m_free(data); // Free allocated memory // Free allocated memory
                    return err;
                }
                break;
            }
 
            case CborByteStringType:
            {
                uint8_t strData[64];
                size_t n = 64;
                err = cbor_value_copy_byte_string(&value, strData, &n, &value);
                if (err != CborNoError)
                {
                    /* Parse error. */
                    lwm2m_free(data); // Free allocated memory
                    return err;
                }
                LOG_ARG("CBOR size%d" , n);
    
                break;
            }
 
            case CborTextStringType:
            {
                char strData[64];
                size_t n = 64;
                err = cbor_value_copy_text_string(&value, strData, &n, &value);
                if (err != CborNoError)
                {
                    /* Parse error. */
                    lwm2m_free(data); // Free allocated memory
                    return err;
                }
                // LOG_ARG("CBOR %sTextString: %s", indent, data);
                break;
            }
    
            case CborTagType:
            {
                CborTag tag;
                err = cbor_value_get_tag(&value, &tag); // can't fail
                if (err != CborNoError)
                {
                    /* Parse error. */
                    lwm2m_free(data); // Free allocated memory
                    return err;
                }
                break;
            }
    
            case CborSimpleType:
            {
                uint8_t sType;
                err = cbor_value_get_simple_type(&value, &sType); // can't fail
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
                bool val;
                err = cbor_value_get_boolean(&value, &val); // can't fail
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
                /* Not managed. */
                // LOG_ARG("CBOR %sFloat: (not managed)", indent);
                lwm2m_free(data); // Free allocated memory
                break;
            }
    
            case CborInvalidType:
            {
                /* Can't happen. */
                // LOG_ARG("CBOR %sInvalidType", indent);
                lwm2m_free(data); // Free allocated memory
                break;
            }
 
            err = cbor_value_advance_fixed(&value);
            if (err != CborNoError)
            {
                /* Parse error. */
                lwm2m_free(data); // Free allocated memory
                return err;
            }
      
            // Move to the next CBOR data item
            err = cbor_value_advance(&value);
            if (err != CborNoError) {
                // Error handling
                lwm2m_free(data); // Free allocated memory
                return err;
            }

            dataSize++;
        } /// !switch
    // }/// !while

    // Assign the parsed data to the output pointer
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
