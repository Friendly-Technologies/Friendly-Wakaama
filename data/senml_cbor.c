#include "internals.h"
#include <cbor.h>

int senml_cbor_parse(const lwm2m_uri_t * uriP,
                     const uint8_t * buffer, 
                     size_t bufferLen, 
                     lwm2m_data_t ** dataP)
{
    size_t dataSize = 0;
    *dataP = NULL;
    // lwm2m_data_t *data = NULL;
    LOG("SENML CBOR parse --- >>>>>>>>>>>>> ");
    for (uint32_t i = 0; i < bufferLen; i++)
    {
        lwm2m_printf("%x", buffer[i]);
    }
    LOG("SENML CBOR parse --- >>>>>>>>>>>>> ");
    LOG("SENML CBOR parse --- >>>>>>>>>>>>> ");
    for (uint32_t i = 0; i < bufferLen; i++)
    {
        lwm2m_printf("%x (%d)\n", buffer[i], i);
    }
    LOG("SENML CBOR parse --- >>>>>>>>>>>>> ");

    LOG_ARG("bufferLen: %d", bufferLen);
    LOG_URI(uriP);
    if (uriP == NULL){
        return -1;
    }

    // data = lwm2m_data_new(dataSize + 1);
    // if (data == NULL) {
    //     lwm2m_free(data);
    //     LOG("lwm2m_data_new FAILED");
    //     return -1; 
    // }
    // if (!LWM2M_URI_IS_SET_RESOURCE(uriP)){
    //     lwm2m_free(data);
    //     LOG("LWM2M_URI_IS_SET_RESOURCE FAILED");
    //     return -1; 
    // }
    // data->id = uriP->resourceId;
    
    // LOG_ARG("senml_cbor_parse: uriP.objectId = %d, ", uriP->objectId);
    // LOG_ARG("senml_cbor_parse: uriP.instanceId = %d, ", uriP->instanceId);
    // LOG_ARG("senml_cbor_parse: uriP.resourceId = %d, ", uriP->resourceId);
    // LOG("---------------------------");
    // LOG_ARG("senml_cbor_parse: dataP.type = %d, ", data->type);
    // LOG_ARG("senml_cbor_parse: dataP.ID = %d, ", data->id);
    // LOG_ARG("senml_cbor_parse: dataP.asBoolean = %d, ", data->value.asBoolean);
    // LOG_ARG("senml_cbor_parse: dataP.asInteger = %d, ", data->value.asInteger);
    // LOG_ARG("senml_cbor_parse: dataP.asUnsigned = %lu, ", data->value.asUnsigned);
    // LOG_ARG("senml_cbor_parse: dataP.asFloat = %f, ", data->value.asFloat);
    // LOG_ARG("senml_cbor_parse: dataP.asBufferLength = %d, ", data->value.asBuffer.length);
    // LOG_ARG("senml_cbor_parse: dataP.asChildrenCount = %d, ", data->value.asChildren.count);
    // LOG_ARG("senml_cbor_parse: dataP.asObjLink.objectId = %d, ", data->value.asObjLink.objectId);
    // LOG_ARG("senml_cbor_parse: dataP.asObjLink.objectInstanceId = %d, ", data->value.asObjLink.objectInstanceId);

    return dataSize;
}

#define DEFAULT_BUFF_SIZE (1024UL)


int senml_cbor_serializeData(const lwm2m_data_t * tlvP,
                             const uint8_t * baseUriStr,
                             size_t baseUriLen,
                             uri_depth_t baseLevel,
                             const uint8_t * parentUriStr,
                             size_t parentUriLen,
                             uri_depth_t level,
                             bool *baseNameOutput,
                             CborEncoder *encoder,
                             uint8_t * buffer,
                             size_t bufferLen)
{
    CborError err;
    size_t head = 0;
    int res;

    switch (tlvP->type)
    {
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
        case LWM2M_TYPE_OBJECT:
        case LWM2M_TYPE_OBJECT_INSTANCE:
        {
            uint8_t uriStr[URI_MAX_STRING_LEN];
            size_t uriLen;
            size_t index;

            if (parentUriLen > 0)
            {
                if (URI_MAX_STRING_LEN < parentUriLen) return -1;
                memcpy(uriStr, parentUriStr, parentUriLen);
                uriLen = parentUriLen;
            }
            else
            {
                uriLen = 0;
            }
            res = utils_intToText(tlvP->id,
                                uriStr + uriLen,
                                URI_MAX_STRING_LEN - uriLen);
            if (res <= 0) return -1;
            uriLen += res;
            uriStr[uriLen] = '/';
            uriLen++;

            head = 0;
            for (index = 0 ; index < tlvP->value.asChildren.count; index++)
            {
                if (index != 0)
                {
                    if (head + 1 > bufferLen) return 0;
                }

                res = senml_cbor_serializeData(tlvP->value.asChildren.array + index,
                                        baseUriStr,
                                        baseUriLen,
                                        baseLevel,
                                        uriStr,
                                        uriLen,
                                        level,
                                        baseNameOutput,
                                        encoder,
                                        buffer + head,
                                        bufferLen - head);
                if (res < 0) return -1;
                head += res;
            }
            break;
        }

        default:
            // Encode individual data items
            // Use CBOR encoding functions
            
            // Open a map container
            err = cbor_encoder_create_map(encoder, encoder, 2);
            if (err != CborNoError) return -1;

            // Encode the Object Link
            err = cbor_encode_int(encoder, -2); 
            if (err != CborNoError) return -1;

            // Encode the base URI
            err = cbor_encode_text_string(encoder, (char *)baseUriStr, baseUriLen);
            if (err != CborNoError) return -1;

            // Encode the resource value
            // err = cbor_encode_int(&encoder, tlvP->id); // Assuming resource ID is used as key
            err = cbor_encode_int(encoder, 3); 
            if (err != CborNoError) return -1;
            
            // Encode the resource value as text string
            err = cbor_encode_text_string(encoder, (char *)tlvP->value.asBuffer.buffer, tlvP->value.asBuffer.length);
            if (err != CborNoError) return -1;

            // Close the map container
            err = cbor_encoder_close_container(encoder, encoder);
            if (err != CborNoError) return -1;

            // Get the encoded data length
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            
            break;
    }

    return (int)head;
}

int senml_cbor_serialize(const lwm2m_uri_t * uriP, 
                         int size,
                         const lwm2m_data_t * dataP, 
                         uint8_t ** bufferP)
{
    CborEncoder encoder;
    CborError err;
    size_t length = 0;
    *bufferP = NULL;
    lwm2m_data_t * targetP;

    size_t bufferSize = DEFAULT_BUFF_SIZE; /// TODO Add the ability to calculate the required memory for the cbor buffer
    int baseUriLen;
    int num;
    uri_depth_t rootLevel;
    uri_depth_t baseLevel;
    uint8_t baseUriStr[URI_MAX_STRING_LEN];
    const uint8_t *parentUriStr = NULL;
    size_t parentUriLen = 0;

    baseUriLen = uri_toString(uriP, baseUriStr, URI_MAX_STRING_LEN, &baseLevel);
    if (baseUriLen < 0) return -1;
    if (baseUriLen > 1 && baseLevel != URI_DEPTH_RESOURCE && baseLevel != URI_DEPTH_RESOURCE_INSTANCE)
    {
        if (baseUriLen >= URI_MAX_STRING_LEN -1) return 0;
        baseUriStr[baseUriLen++] = '/';
    }

    num = json_findAndCheckData(uriP, baseLevel, size, dataP, &targetP, &rootLevel);
    if (num < 0) return -1;

    if (baseLevel < rootLevel && baseUriLen > 1 && baseUriStr[baseUriLen - 1] != '/')
    {
        if (baseUriLen >= URI_MAX_STRING_LEN -1) return 0;
        baseUriStr[baseUriLen++] = '/';
    }

    if (!baseUriLen || baseUriStr[baseUriLen - 1] != '/')
    {
        parentUriStr = (const uint8_t *)"/";
        parentUriLen = 1;
    }
    
    uint8_t *encoderBuffer = (uint8_t *)lwm2m_malloc(bufferSize);
    if (encoderBuffer == NULL) {
        LOG("lwm2m_malloc FAILED");
        return -1;
    }

    cbor_encoder_init(&encoder, encoderBuffer, bufferSize, 0);
    err = cbor_encoder_create_array(&encoder, &encoder, size);
    if (err != CborNoError)
    {
        lwm2m_free(encoderBuffer); // Free allocated memory
        return -1;
    }

    bool baseNameOutput = false;
    for (int i = 0; i < num && length < DEFAULT_BUFF_SIZE ; i++)
    {
        int res;

        if (i != 0)
        {
            if (length + 1 > DEFAULT_BUFF_SIZE) return 0;
        }

        res = senml_cbor_serializeData(targetP + i,
                                baseUriStr,
                                baseUriLen,
                                baseLevel,
                                parentUriStr,
                                parentUriLen,
                                rootLevel,
                                &baseNameOutput,
                                &encoder,
                                encoderBuffer + length,
                                DEFAULT_BUFF_SIZE - length);
        if (res < 0) return res;
        length += res;  
    }///!for
    if (length + 1 > DEFAULT_BUFF_SIZE) return 0;

    // Allocate memory for bufferP
    *bufferP = (uint8_t *)lwm2m_malloc(length);
    if (*bufferP == NULL) {
        lwm2m_free(encoderBuffer); // Free allocated memory
        LOG("bufferP is empty");
        return -1; 
    }

    memcpy(*bufferP, encoderBuffer, length);
    lwm2m_free(encoderBuffer); // Free allocated memory

    LOG_ARG("returning %u", length);
    return (int)length;
}

