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

int senml_cbor_encodeValue(CborEncoder* encoder,
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
            err = cbor_encode_tag(encoder, CborUnixTime_tTag);/// now unixTime is the only tag-ed type
            if (err != CborNoError) {
                free(buffer);
                return err; 
            }
            err = cbor_encode_int(encoder, tlvP->value.asInteger);
            if (err != CborNoError) {
                free(buffer);
                return err; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        }
        case LWM2M_TYPE_OBJECT_LINK:
        {
            char tempBuffer[12]; // Sufficient buffer to hold the string representation (e.g., "65535:65535" -> 5 + 1 + 5 == 11 symbols)
            snprintf(tempBuffer, sizeof(tempBuffer), "%u:%u", tlvP->value.asObjLink.objectId, tlvP->value.asObjLink.objectInstanceId);
            err = cbor_encode_text_string(encoder, tempBuffer, strlen(tempBuffer));
            if (err != CborNoError) {
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_text_string/obj_link FAILED with error %d", err);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        }
        case LWM2M_TYPE_OPAQUE:
            err = cbor_encode_byte_string(encoder, tlvP->value.asBuffer.buffer, tlvP->value.asBuffer.length);
            if (err != CborNoError) {
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_byte_string FAILED with error %d", err);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        case LWM2M_TYPE_STRING:
        case LWM2M_TYPE_CORE_LINK:
            err = cbor_encode_text_string(encoder,(char *) tlvP->value.asBuffer.buffer, tlvP->value.asBuffer.length);
            if (err != CborNoError) {
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_text_string FAILED with error %d", err);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        case LWM2M_TYPE_INTEGER:
            err = cbor_encode_int(encoder,tlvP->value.asInteger);
            if (err != CborNoError) {
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        case LWM2M_TYPE_UNSIGNED_INTEGER:
            err = cbor_encode_uint(encoder, tlvP->value.asUnsigned);
            if (err != CborNoError) {
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        case LWM2M_TYPE_FLOAT:
            err = cbor_encode_double(encoder, tlvP->value.asFloat);
            if (err != CborNoError) {
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_double FAILED with error %d", err);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        case LWM2M_TYPE_BOOLEAN:
            err = cbor_encode_boolean(encoder, tlvP->value.asBoolean);
            if (err != CborNoError) {
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_boolean FAILED with error %d", err);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        default:
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

    LOG("senml_cbor_encodeValueWithMap ---- CHECKPOINT 1 ------------------");
    LOG_ARG("tlvP-type = %d", &tlvP->type);
    LOG_ARG("tlvP-type = %d", tlvP->type);
    LOG_ARG("tlvP-type = %d", tlvP->type);
    int type = tlvP->type;
    LOG_ARG("tlvP-type = %d", type);
    switch (type) {
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

    LOG("CLOSING map --->");
    err = cbor_encoder_close_container(encoder, encoder); /// Closing map
    if (err != CborNoError) 
    {
        LOG_ARG("cbor_encoder_close_container FAILED err=%d", err);
        return -1;
    }

    return head;
}

int senml_cbor_encodeUri(CborEncoder* encoder,
                              const lwm2m_uri_t* uriP,
                              uint8_t * buffer)
{
    CborError err;
    uint8_t baseUriStr[URI_MAX_STRING_LEN];
    uri_depth_t baseLevel;
    int baseUriLen = 0;
       
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

    baseUriLen = uri_toString(uriP, baseUriStr, URI_MAX_STRING_LEN, &baseLevel);
    lwm2m_printf("%.*s", baseUriLen, baseUriStr);
    LOG("senml_cbor_encodeUri ---- CHECKPOINT 1 ------------------");
    err = cbor_encode_text_string(encoder, (char *)baseUriStr, baseUriLen);
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
int encode_one_single_resource(const lwm2m_uri_t *uri, const lwm2m_data_t *data, uint8_t *buffer) {
    CborEncoder encoder;
    cbor_encoder_init(&encoder, buffer, sizeof(buffer), 0);
    return senml_cbor_encodeValue(&encoder, data, buffer);
}

// Function to encode Resources
int encode_resources(CborEncoder* encoder, const lwm2m_uri_t *uri, const lwm2m_data_t *data, uint8_t *buffer) {
    LOG("encode_resources");
    LOG_ARG("dataP->type: %d", data->type);
    LOG_ARG("dataP->type: %d", &data->type);
    int length = senml_cbor_encodeUri(encoder, uri, buffer);
    length += senml_cbor_encodeValueWithMap(encoder, data, buffer);
    return length;
}

int prv_serialize(CborEncoder* encoderP, lwm2m_uri_t *uriP, int size, const lwm2m_data_t *dataP, uint8_t *bufferP, uri_depth_t *levelP) {
    // Iterate through the children of the Object
    int length = 0;
    
    LOG_ARG("levelP: %d", *levelP);
    LOG_ARG("dataP->type: %d", dataP->type);

    for (int i = 0; i < size; i++) {
        // const lwm2m_data_t *child_data = &data->value.asChildren.array[i];
        lwm2m_uri_t child_uri;
        memcpy(&child_uri, uriP, sizeof(lwm2m_uri_t));
        // Set instance ID and resource ID
        if (*levelP == LWM2M_OBJECT){
            LOG_ARG("levelP instance: %d", *levelP);
            child_uri.instanceId = dataP[i].id;
            *levelP = LWM2M_INSTANCE;
            length += prv_serialize(encoderP, &child_uri, dataP[i].value.asChildren.count, dataP[i].value.asChildren.array, bufferP, levelP);
        }
        if (*levelP == LWM2M_INSTANCE){
            LOG_ARG("levelP resource: %d", *levelP);
            if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE){
                child_uri.resourceId = dataP[i].id;
                *levelP = LWM2M_MULTIPLE_RESOURCE;
                length += prv_serialize(encoderP, &child_uri, dataP[i].value.asChildren.count, dataP[i].value.asChildren.array, bufferP, levelP);
            }
            else
            {
               child_uri.resourceId = dataP[i].id;
               length += encode_resources(encoderP, &child_uri, dataP, bufferP);
            }
        }
        else if (*levelP == LWM2M_RESOURCE){
            LOG_ARG("levelP multi-resource: %d", *levelP);
            child_uri.resourceInstanceId = dataP[i].id;
            length += encode_resources(encoderP, &child_uri, dataP, bufferP);
        }
        else if (*levelP == LWM2M_MULTIPLE_RESOURCE){
            LOG_ARG("levelP multi-resource: %d", *levelP);
            child_uri.resourceInstanceId = dataP[i].id;
            length += encode_resources(encoderP, &child_uri, dataP, bufferP);
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

    // int baseUriLen;
    uri_depth_t baseLevel;
    // uint8_t baseUriStr[URI_MAX_STRING_LEN];

    uint8_t bufferCBOR[1024];

    LOG_ARG("size: %d", size);
    LOG_URI(uriP);
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

    // baseUriLen = uri_toString(uriP, baseUriStr, URI_MAX_STRING_LEN, &baseLevel);
    // LOG_ARG("baseUriLen = %d", baseUriLen);

    LOG_ARG("URI objectId = %d", uriP->objectId);
    LOG_ARG("URI instanceId = %d", uriP->instanceId);
    LOG_ARG("URI resourceId = %d", uriP->resourceId);
    LOG_ARG("URI resourceInstanceId = %d", uriP->resourceInstanceId);
    
    // LOG_ARG("baseLevel = %d", baseLevel);
    if (!LWM2M_URI_IS_SET_OBJECT(uriP)){
        //error 
    }
    else if (!LWM2M_URI_IS_SET_INSTANCE(uriP)){
        baseLevel = LWM2M_OBJECT; /// aray of resources
    }
    else if (!LWM2M_URI_IS_SET_RESOURCE(uriP)){
        baseLevel = LWM2M_INSTANCE; ///array
    }
    else if (!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP)){
        baseLevel = LWM2M_RESOURCE; /// single
    }
    else {
        baseLevel = LWM2M_MULTIPLE_RESOURCE;
    }


    switch (baseLevel)
    {
        case LWM2M_OBJECT:
        case LWM2M_INSTANCE:
        {
            CborEncoder encoder;
            CborError err;
            cbor_encoder_init(&encoder, bufferCBOR, sizeof(bufferCBOR), 0);

            err = cbor_encoder_create_array(&encoder, &encoder, 1);
            if (err != CborNoError) 
            {
                LOG_ARG("cbor_encoder_create_array FAILED err=%d", err);
                return -1;
            }
            length += prv_serialize(&encoder, uriP, size, dataP, bufferCBOR, &baseLevel);
            LOG("CLOSING Array --->");
            err = cbor_encoder_close_container(&encoder, &encoder); /// Closing array
            if (err != CborNoError) 
            {
                LOG_ARG("cbor_encoder_close_container FAILED err=%d", err);
                return -1;
            }
            break;
        }
        case LWM2M_RESOURCE:
            LOG("RESOURCE level");
            if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE) 
            {
                LOG("LWM2M_TYPE_MULTIPLE_RESOURCE ");
                CborEncoder encoder;
                CborError err;
                cbor_encoder_init(&encoder, bufferCBOR, sizeof(bufferCBOR), 0);

                err = cbor_encoder_create_array(&encoder, &encoder, 1);
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encoder_create_array FAILED err=%d", err);
                    return -1;
                }
                length = prv_serialize(&encoder, uriP, size, dataP, bufferCBOR, &baseLevel);
                LOG("CLOSING Array --->");
                err = cbor_encoder_close_container(&encoder, &encoder); /// Closing array
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encoder_close_container FAILED err=%d", err);
                    return -1;
                }
            } 
            else 
            {
                LOG("encode_resource -> SINGLE DATA");
                length += encode_one_single_resource(uriP, dataP, bufferCBOR);
            }
            break;
        case LWM2M_MULTIPLE_RESOURCE:
            if (dataP->type != LWM2M_TYPE_MULTIPLE_RESOURCE){
                LOG(" Some Error occured!");    
                break;
            }
            if (dataP->value.asChildren.count > 1) 
            {
                LOG("MULTIPLE level");
                CborEncoder encoder;
                CborError err;
                cbor_encoder_init(&encoder, bufferCBOR, sizeof(bufferCBOR), 0);

                err = cbor_encoder_create_array(&encoder, &encoder, 1);
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encoder_create_array FAILED err=%d", err);
                    return -1;
                }
                length += prv_serialize(&encoder, uriP, size, dataP, bufferCBOR, &baseLevel);
                LOG("CLOSING Array --->");
                err = cbor_encoder_close_container(&encoder, &encoder); /// Closing array
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encoder_close_container FAILED err=%d", err);
                    return -1;
                }
            }
            else
            {
                LOG("encode_resource -> SINGLE DATA");
                length += encode_one_single_resource(uriP, dataP, bufferCBOR);
            }
            break;
        default:
            break;
    }
    
    // Allocate memory for bufferP
    *bufferP = (uint8_t *)lwm2m_malloc(length);
    if (*bufferP == NULL) {
        lwm2m_free(bufferCBOR); // Free allocated memory
        LOG("bufferP is empty");
        return -1; 
    }
    
    LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    for (int i = 0; i < (int)length; i++){
        lwm2m_printf("%x", bufferCBOR[i]);
    }
    LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    LOG_ARG("length = %d", length);     
    memcpy(*bufferP, bufferCBOR, length);

    LOG_ARG("returning %u", length);
    return (int)length;
}

