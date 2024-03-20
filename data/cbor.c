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
        LOG("Error: Expected a tag\n");
        return err;
    }

    err = cbor_value_get_tag(value, &tag);
    if (err != CborNoError)
    {
        return err;
    }
    LOG_ARG("Parsed TAG: %lu (0x%x)\n", tag, tag);

    if (tag != 0x19) {
        LOG("Error: Expected tag 0x19\n");
    }

    // Advance to the integer value
    err = cbor_value_advance_fixed(value);
    if (err != CborNoError)
    {
        return err;
    }
    // Get the integer value
    int32_t intValue;
    err = cbor_value_get_int(value, &intValue);
    data->value.asInteger = intValue;

    // Print the parsed integer value
    LOG_ARG("Parsed integer value: %d\n", intValue);
       
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
