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
    LOG_ARG (" senml_cbor_encodeValue tlvp-type = %d",tlvP->type);

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
    LOG_ARG (" senml_cbor_encodeValueWithMap tlvp-type = %d",tlvP->type);

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

int senml_cbor_encodeUri(CborEncoder* encoder, lwm2m_uri_t* uriP, uint8_t * buffer)
{
    CborError err;
    size_t uriStrLen = 0;
    uri_depth_t level;
    uint8_t uriStr[URI_MAX_STRING_LEN];
    // CborEncoder encoderTest;
    // cbor_encoder_init(&encoderTest, buffer, sizeof(buffer), 0);

    LOG("senml_cbor_encodeUri ------------------------------------- CHECKPOINT 10 - uri encoding ");
    LOG_URI(uriP);
    uriStrLen = uri_toString(uriP, uriStr, URI_MAX_STRING_LEN, &level);
    LOG_ARG("uriLen 1 = %d", uriStrLen);
    // uriStr[uriStrLen++] = 0;
    lwm2m_printf("%.*s", uriStrLen, uriStr);
    // LOG_ARG("uriLen 2 = %d", uriStrLen);

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

    err = cbor_encode_text_string(encoder, (char*)uriStr, uriStrLen);
    if (err != CborNoError) 
    {
        LOG_ARG("cbor_encode_text_string FAILED err=%d", err);
        return -1;
    }
    
    return cbor_encoder_get_buffer_size(encoder, buffer);
}

#define DEFAULT_BUFF_SIZE       (1024UL)

// Function to encode One Single Resource
int encode_one_single_resource(const lwm2m_data_t *data, uint8_t *buffer, size_t *bufferSize) {
    return senml_cbor_encodeValue(data, buffer, bufferSize);
}

// Function to encode several Resources
int encode_resources(CborEncoder* encoder,  lwm2m_uri_t *uriP, const lwm2m_data_t *data, uint8_t *buffer) {
        
    int length = senml_cbor_encodeUri(encoder, uriP, buffer);
    length += senml_cbor_encodeValueWithMap(encoder, data, buffer);
    return length;
}

int prv_serialize(CborEncoder* encoderP, lwm2m_uri_t* uriP, const lwm2m_data_t *dataP,  int size, uint8_t *bufferP) 
{
    int length = 0;
    LOG_URI(uriP);
    for (int i = 0 ; i < size; i++)
    {
        const lwm2m_data_t * cur = &dataP[i];
    
        switch (cur->type)
        {
            case LWM2M_TYPE_MULTIPLE_RESOURCE:
            case LWM2M_TYPE_OBJECT_INSTANCE:
            {
                LOG("---------------------------------------- CHECKPOINT 6 - new uri appending ");
                LOG_ARG("ID = %d", cur->id ); // local variable | array of 4 IDs
                if (uriP->instanceId == 65535){
                    uriP->instanceId = cur->id;
                }
                else if (uriP->resourceId == 65535){
                    uriP->resourceId = cur->id;
                }
                else {
                    //error
                    LOG(" ERROR 1 with ids");
                }
                length += prv_serialize(encoderP, uriP, cur->value.asChildren.array, cur->value.asChildren.count, bufferP);
                if (uriP->resourceId != 65535){
                    uriP->resourceId = 65535;
                }
                else if (uriP->instanceId != 65535){
                    uriP->instanceId = 65535;
                }
                else {
                    LOG(" ERROR  2 with ids");
                }
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
    
                if (uriP->resourceId== 65535){
                    uriP->resourceId = cur->id;
                }
                else if (uriP->resourceInstanceId == 65535){
                    uriP->resourceInstanceId = cur->id;
                }
                else {
                    //error
                     LOG(" ERROR 3 with ids");
                }
                length += encode_resources(encoderP, uriP, dataP, bufferP);
                if (uriP->resourceInstanceId != 65535){
                    uriP->resourceInstanceId = 65535;
                }
                else if (uriP->resourceId!= 65535){
                    uriP->resourceId = 65535;
                }
                else {
                    LOG(" ERROR 4 with ids");
                }
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
    LOG_ARG("type = %d", dataP->type);
        
    if ( (size == 1) && (uriP->resourceId != 65535) && (dataP->type != LWM2M_TYPE_MULTIPLE_RESOURCE))
    {
        // -> single resource
        length += encode_one_single_resource(dataP, encoderBuffer, &bufferSize);
         // Allocate memory for bufferP
        *bufferP = (uint8_t *)lwm2m_malloc(length);
        if (*bufferP == NULL) {
            LOG("bufferP is empty");
            lwm2m_free(encoderBuffer); // Free allocated memory
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
    else if (uriP != NULL && LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP)){
        // -> single resource
        size = dataP->value.asChildren.count;
        dataP = dataP->value.asChildren.array;
        if(size != 1) return -1;
        length += encode_one_single_resource(dataP, encoderBuffer, &bufferSize);
         // Allocate memory for bufferP
        *bufferP = (uint8_t *)lwm2m_malloc(length);
        if (*bufferP == NULL) {
            LOG("bufferP is empty");
            lwm2m_free(encoderBuffer); // Free allocated memory
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
    else 
    {
        CborEncoder encoder;
        CborError err;
        
        LOG("---------------------------------------- CHECKPOINT 2 - entering array");
        cbor_encoder_init(&encoder, encoderBuffer, DEFAULT_BUFF_SIZE, 0);

        err = cbor_encoder_create_array(&encoder, &encoder, 1);
        if (err != CborNoError) 
        {
            LOG_ARG("cbor_encoder_create_array FAILED err=%d", err);
            return -1;
        }

        length += prv_serialize(&encoder, uriP, dataP, size, encoderBuffer);

        err = cbor_encoder_close_container(&encoder, &encoder); /// Closing array
        if (err != CborNoError) 
        {
            LOG_ARG("cbor_encoder_close_container FAILED err=%d", err);
            return -1;
        }
        
        // Allocate memory for bufferP
        *bufferP = (uint8_t *)lwm2m_malloc(length);
        if (*bufferP == NULL) {
            LOG("bufferP is empty");
            lwm2m_free(encoderBuffer); // Free allocated memory
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
}

