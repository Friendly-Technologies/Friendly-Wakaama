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

int senml_cbor_serialize(const lwm2m_uri_t * uriP, 
                         int size,
                         const lwm2m_data_t * dataP, 
                         uint8_t ** bufferP)
{
    int length = 0;
    *bufferP = NULL;
    CborEncoder encoder;
    CborError err;
    size_t bufferSize = DEFAULT_BUFF_SIZE; /// TODO Add the ability to calculate the required memory for the cbor buffer

    LOG_ARG("senml_cbor_serialize: uriP.objectId = %d, ", uriP->objectId);
    LOG_ARG("senml_cbor_serialize: uriP.instanceId = %d, ", uriP->instanceId);
    LOG_ARG("senml_cbor_serialize: uriP.resourceId = %d, ", uriP->resourceId);
    LOG("---------------------------");
    LOG_ARG("senml_cbor_serialize: dataP.type = %d, ", dataP->type);
    LOG_ARG("senml_cbor_serialize: dataP.ID = %d, ", dataP->id);
    LOG_ARG("senml_cbor_serialize: dataP.asBoolean = %d, ", dataP->value.asBoolean);
    LOG_ARG("senml_cbor_serialize: dataP.asInteger = %d, ", dataP->value.asInteger);
    LOG_ARG("senml_cbor_serialize: dataP.asUnsigned = %lu, ", dataP->value.asUnsigned);
    LOG_ARG("senml_cbor_serialize: dataP.asFloat = %f, ", dataP->value.asFloat);
    LOG_ARG("senml_cbor_serialize: dataP.asBufferLength = %d, ", dataP->value.asBuffer.length);
    LOG_ARG("senml_cbor_serialize: dataP.asChildrenCount = %d, ", dataP->value.asChildren.count);
    LOG_ARG("senml_cbor_serialize: dataP.asObjLink.objectId = %d, ", dataP->value.asObjLink.objectId);
    LOG_ARG("senml_cbor_serialize: dataP.asObjLink.objectInstanceId = %d, ", dataP->value.asObjLink.objectInstanceId);
    LOG_ARG("size: %d", size);

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

    for (int i = 0; i < size; ++i)
    {
        err = cbor_encoder_create_map(&encoder, &encoder, 2);
        if (err != CborNoError)
        {
            lwm2m_free(encoderBuffer); // Free allocated memory
            return -1;
        }

        // Encode the Object Link
        err = cbor_encode_int(&encoder, -2);
        if (err != CborNoError)
        {
            lwm2m_free(encoderBuffer); // Free allocated memory
            return -1;
        }
        uint8_t uriString[URI_MAX_STRING_LEN];
        uri_toString(uriP, uriString, URI_MAX_STRING_LEN, NULL);
        err = cbor_encode_text_string(&encoder, (char *)uriString, strlen((char *)uriString));
        if (err != CborNoError)
        {
            lwm2m_free(encoderBuffer); // Free allocated memory
            return -1;
        }

        // Encode the Object Link
        err = cbor_encode_int(&encoder, 3);
        if (err != CborNoError)
        {
            lwm2m_free(encoderBuffer); // Free allocated memory
            return -1;
        }
        
        err = cbor_encode_text_string(&encoder,(char *) dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
        if (err != CborNoError) {
            lwm2m_free(encoderBuffer);
            LOG_ARG("cbor_encode_text_string FAILED with error %d", err);
            return -1; 
        }

        // Close the map
        err = cbor_encoder_close_container(&encoder, &encoder);
        if (err != CborNoError)
        {
            lwm2m_free(encoderBuffer); // Free allocated memory
            return -1;
        }

        // Calculate the length of the encoded data
        length = cbor_encoder_get_buffer_size(&encoder, encoderBuffer);
    }

    // Allocate memory for bufferP
    *bufferP = (uint8_t *)lwm2m_malloc(length);
    if (*bufferP == NULL) {
        lwm2m_free(encoderBuffer); // Free allocated memory
        LOG("bufferP is empty");
        return -1; 
    }

    // Copy encoded data to bufferP
    memcpy(*bufferP, encoderBuffer, length);
    lwm2m_free(encoderBuffer); // Free allocated memory

    LOG_ARG("returning %u", length);
    return length;

}

