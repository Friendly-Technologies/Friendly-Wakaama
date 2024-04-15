#include "internals.h"
#include <cbor.h>


/**
 * SENML CBOR Representation: integers for map keys:
 *  ______________________________________________
 * |      Name        |  JSON Label  | CBOR Label |
 * |------------------|--------------|------------|
 * |     Version      |     ver      |     -1     |
 * |    Base Name     |     bn       |     -2     |
 * |    Base Time     |     bt       |     -3     |
 * |   Base  Units    |     bu       |     -4     |
 * |      Name        |     n        |      0     |
 * |      Units       |     u        |      1     |
 * |      Value       |     v        |      2     |
 * |  String  Value   |     sv       |      3     |
 * |  Boolean Value   |     bv       |      4     |
 * |   Value  Sum     |     s        |      5     |
 * |       Time       |     t        |      6     |
 * |   Update Time    |     ut       |      7     |
 * |------------------|--------------|------------|
 * |     Opaque       |     vd       |      8     |
 * |------------------|--------------|------------|
*/

#define SENML_CBOR_MAP_VERSION      (-1)
#define SENML_CBOR_MAP_BASE_NAME    (-2)
#define SENML_CBOR_MAP_BASE_TIME    (-3)
#define SENML_CBOR_MAP_BASE_UNITS   (-4)
#define SENML_CBOR_MAP_NAME         (0)
#define SENML_CBOR_MAP_UNITS        (1)
#define SENML_CBOR_MAP_VALUE        (2)
#define SENML_CBOR_MAP_STRING_VALUE (3)
#define SENML_CBOR_MAP_BOOL_VALUE   (4)
#define SENML_CBOR_MAP_VALUE_SUM    (5)
#define SENML_CBOR_MAP_TIME         (6)
#define SENML_CBOR_MAP_UPD_TIME     (7)
#define SENML_CBOR_MAP_OPAQUE       (8)
#define SENML_CBOR_MAP_OBJ_LINK     (9)

int senml_cbor_parse(const lwm2m_uri_t * uriP,
                     const uint8_t * buffer, 
                     size_t bufferLen, 
                     lwm2m_data_t ** dataP)
{
    size_t dataSize = 0;
    *dataP = NULL;
    // lwm2m_data_t *data = NULL;
    // LOG("SENML CBOR parse --- >>>>>>>>>>>>> ");
    // for (uint32_t i = 0; i < bufferLen; i++)
    // {
    //     lwm2m_printf("%x", buffer[i]);
    // }
    // LOG("SENML CBOR parse --- >>>>>>>>>>>>> ");
    // LOG("SENML CBOR parse --- >>>>>>>>>>>>> ");
    // for (uint32_t i = 0; i < bufferLen; i++)
    // {
    //     lwm2m_printf("%x (%d)\n", buffer[i], i);
    // }
    // LOG("SENML CBOR parse --- >>>>>>>>>>>>> ");

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
/**
 * ***********************************************************************************************************
 * ***********************************************************************************************************
 * ***********************************************************************************************************
 * ***********************************************************************************************************
 * ***********************************************************************************************************
 * ***********************************************************************************************************
 * ***********************************************************************************************************
 * ***********************************************************************************************************
 * ***********************************************************************************************************
*/

int senml_cbor_encodeValue(const lwm2m_data_t * tlvP,
                           uint8_t * buffer, size_t *bufferSize)
{
    CborError err;
    CborEncoder encoder;
    int head;
    
    cbor_encoder_init(&encoder, buffer, *bufferSize, 0);
    LOG("--------------------------- CHECKPOINT 3 ---- encoder init OK");
    LOG_ARG ("tlvp-type = %d",tlvP->type);

    switch (tlvP->type) {
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
                LOG_ARG("cbor_encode_tag FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            err = cbor_encode_int(&encoder, tlvP->value.asInteger);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(&encoder, buffer);
            break;
        }
        case LWM2M_TYPE_OBJECT_LINK:
        {
            char tempBuffer[12]; // Sufficient buffer to hold the string representation (e.g., "65535:65535" -> 5 + 1 + 5 == 11 symbols)
            snprintf(tempBuffer, sizeof(tempBuffer), "%u:%u", tlvP->value.asObjLink.objectId, tlvP->value.asObjLink.objectInstanceId);
            err = cbor_encode_text_string(&encoder, tempBuffer, strlen(tempBuffer));
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_text_string/obj_link FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(&encoder, buffer);
            break;
        }
        case LWM2M_TYPE_OPAQUE:
            err = cbor_encode_byte_string(&encoder, tlvP->value.asBuffer.buffer, tlvP->value.asBuffer.length);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_byte_string FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(&encoder, buffer);
            break;
        case LWM2M_TYPE_STRING:
        case LWM2M_TYPE_CORE_LINK:
            err = cbor_encode_text_string(&encoder,(char *) tlvP->value.asBuffer.buffer, tlvP->value.asBuffer.length);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_text_string FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(&encoder, buffer);
            break;
        case LWM2M_TYPE_INTEGER:
            err = cbor_encode_int(&encoder,tlvP->value.asInteger);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(&encoder, buffer);
            break;
        case LWM2M_TYPE_UNSIGNED_INTEGER:
            err = cbor_encode_uint(&encoder, tlvP->value.asUnsigned);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(&encoder, buffer);
            break;
        case LWM2M_TYPE_FLOAT:
            err = cbor_encode_double(&encoder, tlvP->value.asFloat);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_double FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(&encoder, buffer);
            break;
        case LWM2M_TYPE_BOOLEAN:
            err = cbor_encode_boolean(&encoder, tlvP->value.asBoolean);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_boolean FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(&encoder, buffer);
            break;
        default:
            LOG_ARG("Something went wrong FAILED with error %d", err);
            lwm2m_free(buffer);
            return -1;
            break;
    }
    
    return head;
}


int senml_cbor_encodeValueWithMap(CborEncoder* encoder,
                              const lwm2m_data_t * tlvP,
                              uint8_t * buffer)
{
    CborError err;
    int head;

    switch (tlvP->type) {
        case LWM2M_TYPE_UNDEFINED:
        case LWM2M_TYPE_OBJECT:
        case LWM2M_TYPE_OBJECT_INSTANCE:
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
            return -1;
            break;
        case LWM2M_TYPE_TIME:
        {
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_TIME);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            // err = cbor_encode_tag(encoder, CborUnixTime_tTag);/// now unixTime is the only tag-ed type
            // if (err != CborNoError) {
            //     free(buffer);
            //     return err; 
            // }
            err = cbor_encode_int(encoder, tlvP->value.asInteger);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        }
        case LWM2M_TYPE_OBJECT_LINK:
        {
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_OBJ_LINK);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            char tempBuffer[12]; // Sufficient buffer to hold the string representation (e.g., "65535:65535" -> 5 + 1 + 5 == 11 symbols)
            snprintf(tempBuffer, sizeof(tempBuffer), "%u:%u", tlvP->value.asObjLink.objectId, tlvP->value.asObjLink.objectInstanceId);
            err = cbor_encode_text_string(encoder, tempBuffer, strlen(tempBuffer));
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_text_string/obj_link FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        }
        case LWM2M_TYPE_OPAQUE:
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_OPAQUE);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            err = cbor_encode_byte_string(encoder, tlvP->value.asBuffer.buffer, tlvP->value.asBuffer.length);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_byte_string FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        case LWM2M_TYPE_STRING:
        case LWM2M_TYPE_CORE_LINK:
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_STRING_VALUE);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            err = cbor_encode_text_string(encoder,(char *) tlvP->value.asBuffer.buffer, tlvP->value.asBuffer.length);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_text_string FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        case LWM2M_TYPE_INTEGER:
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_VALUE);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            err = cbor_encode_int(encoder,tlvP->value.asInteger);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        case LWM2M_TYPE_UNSIGNED_INTEGER:
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_VALUE);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            err = cbor_encode_uint(encoder, tlvP->value.asUnsigned);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        case LWM2M_TYPE_FLOAT:
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_VALUE);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            err = cbor_encode_double(encoder, tlvP->value.asFloat);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_double FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        case LWM2M_TYPE_BOOLEAN:
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_BOOL_VALUE);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            err = cbor_encode_boolean(encoder, tlvP->value.asBoolean);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_boolean FAILED with error %d", err);
                lwm2m_free(buffer);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        default:
            LOG_ARG("ERROR WRONG tlvP-type  %d", tlvP->type );
            lwm2m_free(buffer);
            return -1;
            break;
    }

    err = cbor_encoder_close_container(encoder, encoder); /// Closing map
    if (err != CborNoError) 
    {
        LOG_ARG("cbor_encoder_close_container FAILED err=%d", err);
        return -1;
    }

    return head;
}

int senml_cbor_encodeUri(CborEncoder* encoder,
                              uint8_t* uriStrP,
                              uint8_t * buffer)
{
    CborError err;
       
    err = cbor_encoder_create_map(encoder, encoder, 2);/// opening map
    if (err != CborNoError) 
    {
        LOG_ARG("cbor_encoder_create_map FAILED err=%d", err);
        return -1;
    }

    err = cbor_encode_int(encoder, 0); /// Name code in SENML-CBOR == (0)
    if (err != CborNoError) 
    {
        LOG_ARG("cbor_encode_int FAILED err=%d", err);
        return -1;
    }
    LOG("---------------------------------------- CHECKPOINT 9 - uri encoding ");
    lwm2m_printf("%.*s", sizeof(uriStrP), uriStrP);
    err = cbor_encode_text_string(encoder, (char *)uriStrP, sizeof(uriStrP));
    if (err != CborNoError) 
    {
        LOG_ARG("cbor_encode_text_string FAILED err=%d", err);
        return -1;
    }
    
    return cbor_encoder_get_buffer_size(encoder, buffer);
}

#define DEFAULT_BUFF_SIZE       (1024UL)
#define LWM2M_OBJECT            (1)
#define LWM2M_INSTANCE          (2)
#define LWM2M_RESOURCE          (3)
#define LWM2M_MULTIPLE_RESOURCE (4)

// Function to encode One Single Resource
int encode_one_single_resource(const lwm2m_data_t *data, uint8_t *buffer, size_t *bufferSize) {
    return senml_cbor_encodeValue(data, buffer, bufferSize);
}

// Function to encode several Resources
int encode_resources(CborEncoder* encoder, uint8_t *uriStrP, const lwm2m_data_t *data, uint8_t *buffer) {
    int length = senml_cbor_encodeUri(encoder, uriStrP, buffer);
    length += senml_cbor_encodeValueWithMap(encoder, data, buffer);
    return length;
}

int prv_serialize(CborEncoder* encoderP, bool isResourceInstance, uint8_t *uriStrP, int uriLen, int size, const lwm2m_data_t *dataP, uint8_t *bufferP) 
{
    int length = 0;
    LOG("---------------------------------------- CHECKPOINT 6 - uri checking ");
    lwm2m_printf("%.*s", uriLen, uriStrP);
    for (int i = 0 ; i < size; i++)
    {
        bool isInstance;
        const lwm2m_data_t * cur = &dataP[i];

        isInstance = isResourceInstance;
    
        switch (cur->type)
        {
            case LWM2M_TYPE_MULTIPLE_RESOURCE:
                isInstance = true;
            case LWM2M_TYPE_OBJECT_INSTANCE:
            {
                uriStrP[uriLen++] = cur->id;
                LOG("---------------------------------------- CHECKPOINT 7 - uri checking ");
                lwm2m_printf("%.*s", uriLen, uriStrP);
                length += prv_serialize(encoderP, isInstance, uriStrP, uriLen, cur->value.asChildren.count, cur->value.asChildren.array, bufferP);
            }
                break;

            case LWM2M_TYPE_OBJECT_LINK:
            case LWM2M_TYPE_STRING:
            case LWM2M_TYPE_OPAQUE:
            case LWM2M_TYPE_CORE_LINK:
            case LWM2M_TYPE_INTEGER:
            case LWM2M_TYPE_TIME:
            case LWM2M_TYPE_UNSIGNED_INTEGER:
            case LWM2M_TYPE_FLOAT:
            case LWM2M_TYPE_BOOLEAN:
                LOG("---------------------------------------- CHECKPOINT 8 - multi resources ");
                length += encode_resources(encoderP, uriStrP, dataP, bufferP);
                break;

            default:
                length = -1;
                break;
        }
    }

    return length;
}

int senml_cbor_serialize(lwm2m_uri_t * uriP, 
                         int size,
                         const lwm2m_data_t * dataP, 
                         uint8_t ** bufferP)
{
    size_t length = 0;
    *bufferP = NULL;
    size_t bufferSize = DEFAULT_BUFF_SIZE; /// TODO Add the ability to calculate the required memory for the cbor buffer
    bool isResourceInstance;

    LOG("CBOR cbor_serialize ");
    uint8_t *encoderBuffer = (uint8_t *)lwm2m_malloc(bufferSize);
    if (encoderBuffer == NULL) {
        LOG("lwm2m_malloc FAILED");
        return -1;
    }

    LOG_ARG("size: %d", size);
    LOG_URI(uriP);
    LOG_ARG("senml_cbor_serialize: uriP.objectId = %d, ", uriP->objectId);
    LOG_ARG("senml_cbor_serialize: uriP.instanceId = %d, ", uriP->instanceId);
    LOG_ARG("senml_cbor_serialize: uriP.resourceId = %d, ", uriP->resourceId);
    
    if (uriP != NULL && LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP))
        {
            if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
                size = dataP->value.asChildren.count;
                dataP = dataP->value.asChildren.array;
            }
            if(size != 1) return -1;
            isResourceInstance = true;
        }
    else
        if (uriP != NULL && LWM2M_URI_IS_SET_RESOURCE(uriP) && (size != 1 || dataP->id != uriP->resourceId))
        {
            isResourceInstance = true;
        }
        else{
            isResourceInstance = false;
        }
    LOG("--------------------------- CHECKPOINT 2 ---- entering cycle ");

    LOG_ARG("type = %d", dataP->type);
    switch (dataP->type)
    {
        case LWM2M_TYPE_MULTIPLE_RESOURCE:

            isResourceInstance = true;
        case LWM2M_TYPE_OBJECT_INSTANCE:
        {
            CborEncoder encoder;
            CborError err;
            LOG("---------------------------------------- CHECKPOINT 4 - entering array");
            cbor_encoder_init(&encoder, encoderBuffer, sizeof(encoderBuffer), 0);

            err = cbor_encoder_create_array(&encoder, &encoder, 1);
            if (err != CborNoError) 
            {
                LOG_ARG("cbor_encoder_create_array FAILED err=%d", err);
                return -1;
            }
            for (int i = 0 ; i < size; i++)
            {
                uri_depth_t baseLevel;
                int uriLen = 0;
                uint8_t uriStrP[URI_MAX_STRING_LEN];
                uriLen = uri_toString(uriP, uriStrP, URI_MAX_STRING_LEN, &baseLevel);
                LOG("---------------------------------------- CHECKPOINT 5 - uri appending ");
                lwm2m_printf("%.*s", uriLen, uriStrP);
                uriStrP[uriLen++] = dataP->id;
                LOG("---------------------------------------- CHECKPOINT 5 - uri appending ");
                lwm2m_printf("%.*s", uriLen, uriStrP);
                length += prv_serialize(&encoder, isResourceInstance, uriStrP, uriLen, dataP->value.asChildren.count, dataP->value.asChildren.array, encoderBuffer);
            }
            err = cbor_encoder_close_container(&encoder, &encoder); /// Closing array
            if (err != CborNoError) 
            {
                LOG_ARG("cbor_encoder_close_container FAILED err=%d", err);
                return -1;
            }
        }
            break;

        case LWM2M_TYPE_STRING:
        case LWM2M_TYPE_OPAQUE:
        case LWM2M_TYPE_INTEGER:
        case LWM2M_TYPE_UNSIGNED_INTEGER:
        case LWM2M_TYPE_FLOAT:
        case LWM2M_TYPE_BOOLEAN:
        case LWM2M_TYPE_TIME:
        case LWM2M_TYPE_OBJECT_LINK:
        case LWM2M_TYPE_CORE_LINK:
            length += encode_one_single_resource(dataP, encoderBuffer, &bufferSize);
            break;

        default:
            length = -1;
            break;
    }///!switch
    
    // Allocate memory for bufferP
    *bufferP = (uint8_t *)lwm2m_malloc(length);
    if (*bufferP == NULL) {
        lwm2m_free(encoderBuffer); // Free allocated memory
        LOG("bufferP is empty");
        return -1; 
    }
    
    LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>final resulting buffer>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    for (int i = 0; i <= (int)length; i++){
        lwm2m_printf("%x", encoderBuffer[i]);
    }
    LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    LOG_ARG("length = %d", length);     
    memcpy(*bufferP, encoderBuffer, length);
    lwm2m_free(encoderBuffer); // Free allocated memory

    LOG_ARG("returning %u", length);
    return (int)length;
}

