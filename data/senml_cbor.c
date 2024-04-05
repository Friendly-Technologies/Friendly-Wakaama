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
                             uint8_t * baseUriStr,
                             size_t baseUriLen,
                             uri_depth_t baseLevel,
                             const uint8_t * parentUriStr,
                             size_t parentUriLen,
                             uri_depth_t level,
                             bool *baseNameOutput,
                             CborEncoder *encoder,
                             uint8_t * buffer,
                             size_t bufferLen,
                             int* size)
{
    CborError err;
    size_t head = 0;

    LOG_ARG("baseLevel = %d", baseLevel);
    LOG_ARG("level = %d", level);
    LOG_ARG("tlvP->type = %d", tlvP->type);

    switch (level)
    {
        case URI_DEPTH_RESOURCE_INSTANCE:

            if (!*baseNameOutput && baseUriLen > 0)
            {
                LOG(">>>>>>>>>>>>> CHECKPOINT 1 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>");

                err = cbor_encoder_create_array(encoder, encoder, 1);
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encoder_create_array FAILED err=%d", err);
                    return -1;
                }
                
                err = cbor_encoder_create_map(encoder, encoder, 3);
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encoder_create_map FAILED err=%d", err);
                    return -1;
                }

                err = cbor_encode_int(encoder, -2); ///Base name code in SENML-CBOR == (-2)
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encode_int FAILED err=%d", err);
                    return -1;
                }

                baseUriStr[baseUriLen++] = '0'; /// for the MULTIPLE objects we need to add 0 to the URI

                err = cbor_encode_text_string(encoder, (char *)baseUriStr, baseUriLen);
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encode_text_string FAILED err=%d", err);
                    return -1;
                }

                *baseNameOutput = true;
            }
            
            if (!baseUriLen || level > baseLevel)
            {
                LOG(">>>>>>>>>>>>> CHECKPOINT 2 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>"); 

                if (!baseNameOutput)
                {
                    err = cbor_encoder_create_map(encoder, encoder, 2);
                    LOG(">>>>>>>>>>>>> CHECKPOINT 2.5 >>>>>>>>>>>>>>>>>>>>>>>>>>>");
                    if (err != CborNoError) 
                    {
                        LOG_ARG("cbor_encoder_create_map FAILED err=%d", err);
                        return -1;
                    }
                }

                err = cbor_encode_int(encoder, 0); /// Name code in SENML-CBOR == (0)
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encode_int FAILED err=%d", err);
                    return -1;
                }

                err = cbor_encode_int(encoder, tlvP->id); // Assuming resource ID is used as key
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encode_int FAILED err=%d", err);
                    return -1;
                }
            }

            if (tlvP->type != LWM2M_TYPE_UNDEFINED)
            {
                LOG(">>>>>>>>>>>>> CHECKPOINT 3 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>"); 
                
                err = cbor_encode_int(encoder, 3); /// String-type in SENML-CBOR == (3)
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encode_int FAILED err=%d", err);
                    return -1;
                }
                
                err = cbor_encode_text_string(encoder, (char *)tlvP->value.asBuffer.buffer, tlvP->value.asBuffer.length);
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encode_text_string FAILED err=%d", err);
                    return -1;
                }

                // err = cbor_encoder_close_container(encoder, encoder); /// one close for MAP
                // if (err != CborNoError) 
                // {
                //     LOG_ARG("cbor_encoder_close_container FAILED err=%d", err);
                //     return -1;
                // }

                head = cbor_encoder_get_buffer_size(encoder, buffer);
            }
            break;
        case URI_DEPTH_RESOURCE:

            err = cbor_encoder_create_array(encoder, encoder, 1);
            if (err != CborNoError) 
            {
                LOG_ARG("cbor_encoder_create_array FAILED err=%d", err);
                return -1;
            }
            
            err = cbor_encoder_create_map(encoder, encoder, 2);
            if (err != CborNoError) 
            {
                LOG_ARG("cbor_encoder_create_map FAILED err=%d", err);
                return -1;
            }

            err = cbor_encode_int(encoder, -2); ///Base name code in SENML-CBOR == (-2)
            if (err != CborNoError) 
            {
                LOG_ARG("cbor_encode_int FAILED err=%d", err);
                return -1;
            }

            err = cbor_encode_text_string(encoder, (char *)baseUriStr, baseUriLen);
            if (err != CborNoError) 
            {
                LOG_ARG("cbor_encode_text_string FAILED err=%d", err);
                return -1;
            }
                        
            err = cbor_encode_int(encoder, 3); /// String-type in SENML-CBOR == (3)
            if (err != CborNoError) 
            {
                LOG_ARG("cbor_encode_int FAILED err=%d", err);
                return -1;
            }
            
            err = cbor_encode_text_string(encoder, (char *)tlvP->value.asBuffer.buffer, tlvP->value.asBuffer.length);
            if (err != CborNoError) 
            {
                LOG_ARG("cbor_encode_text_string FAILED err=%d", err);
                return -1;
            }

            err = cbor_encoder_close_container(encoder, encoder);
            if (err != CborNoError) 
            {
                LOG_ARG("cbor_encoder_close_container FAILED err=%d", err);
                return -1;
            }

            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        default:
            LOG(" default level "); 
            break;
    }///!switch-case

    return (int)head;
}

int senml_cbor_serialize(const lwm2m_uri_t * uriP, 
                         int size,
                         const lwm2m_data_t * dataP, 
                         uint8_t ** bufferP)
{
    CborEncoder encoder;
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

    bool baseNameOutput = false;
    for (int i = 0; i < num && length < DEFAULT_BUFF_SIZE ; i++)
    {
        int res;

        if (i != 0)
        {
            if (length + 1 > DEFAULT_BUFF_SIZE) {
                LOG(" ERROR"); 
                return 0;
            }
        }

        res = senml_cbor_serializeData(targetP + i, baseUriStr, baseUriLen,  baseLevel,
                                parentUriStr, parentUriLen, rootLevel, &baseNameOutput,
                                &encoder, encoderBuffer + length, DEFAULT_BUFF_SIZE - length,
                                &size);
        LOG(">>>>>>>>>>>>> CHECKPOINT 7 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>"); 
    
        if (res < 0) {
            LOG(" ERROR"); 
            return res;
        }
        LOG(">>>>>>>>>>>>> CHECKPOINT 8 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>"); 
        length += res;  
    }///!for
    if (length + 1 > DEFAULT_BUFF_SIZE) return 0;
    LOG(">>>>>>>>>>>>> CHECKPOINT 9 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>"); 

    // Allocate memory for bufferP
    *bufferP = (uint8_t *)lwm2m_malloc(length);
    if (*bufferP == NULL) {
        lwm2m_free(encoderBuffer); // Free allocated memory
        LOG("bufferP is empty");
        return -1; 
    }
    
    LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    for (int i = 0; i <= (int)length; i++){
        lwm2m_printf("%x", encoderBuffer[i]);
    }
    LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");    
    memcpy(*bufferP, encoderBuffer, length);
    lwm2m_free(encoderBuffer); // Free allocated memory

    LOG_ARG("returning %u", length);
    return (int)length;
}

