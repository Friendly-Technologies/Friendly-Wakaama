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


static int senml_cbor_serializeValue(CborEncoder* encoder,
                              const lwm2m_data_t * tlvP,
                              uint8_t * buffer,
                              size_t bufferLen)
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
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
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
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_OBJ_LINK);
            if (err != CborNoError) {
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
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
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_OPAQUE);
            if (err != CborNoError) {
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
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
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_STRING_VALUE);
            if (err != CborNoError) {
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            err = cbor_encode_text_string(encoder,(char *) tlvP->value.asBuffer.buffer, tlvP->value.asBuffer.length);
            if (err != CborNoError) {
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_text_string FAILED with error %d", err);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        case LWM2M_TYPE_INTEGER:
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_VALUE);
            if (err != CborNoError) {
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            err = cbor_encode_int(encoder,tlvP->value.asInteger);
            if (err != CborNoError) {
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        case LWM2M_TYPE_UNSIGNED_INTEGER:
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_VALUE);
            if (err != CborNoError) {
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            err = cbor_encode_uint(encoder, tlvP->value.asUnsigned);
            if (err != CborNoError) {
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        case LWM2M_TYPE_FLOAT:
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_VALUE);
            if (err != CborNoError) {
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            err = cbor_encode_double(encoder, tlvP->value.asFloat);
            if (err != CborNoError) {
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_double FAILED with error %d", err);
                return -1; 
            }
            head = cbor_encoder_get_buffer_size(encoder, buffer);
            break;
        case LWM2M_TYPE_BOOLEAN:
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_BOOL_VALUE);
            if (err != CborNoError) {
                lwm2m_free(buffer);
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
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
                             uint8_t * parentUriStr,
                             size_t parentUriLen,
                             uri_depth_t level,
                             bool *baseNameOutput,
                             uint8_t * buffer,
                             size_t bufferLen)
{
    CborError err;
    CborEncoder parentEncoder;
    int res;
    size_t head = 0;

    cbor_encoder_init(&parentEncoder, buffer, 1024, 0);

    LOG_ARG("baseLevel = %d", baseLevel);
    LOG_ARG("level = %d", level);
    LOG_ARG("tlvP->type = %d", tlvP->type);

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
            LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>  uriStr >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");    
            lwm2m_printf("%.*s", uriLen, uriStr);
            LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");    

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
                                        buffer + head,
                                        bufferLen - head);
                if (res < 0) return -1;
                head += res;
            }
            break;
        }

        default:
        {
            head = 0;
            if (bufferLen < 1) return -1;
            err = cbor_encoder_create_array(&parentEncoder, &parentEncoder, 1);
            if (err != CborNoError) 
            {
                LOG_ARG("cbor_encoder_create_array FAILED err=%d", err);
                return -1;
            }

            if (!*baseNameOutput && baseUriLen > 0)
            {
                err = cbor_encoder_create_map(&parentEncoder, &parentEncoder, 2);
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encoder_create_map FAILED err=%d", err);
                    return -1;
                }

                err = cbor_encode_int(&parentEncoder, -2); /// Base Name code in SENML-CBOR == (-2)
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encode_int FAILED err=%d", err);
                    return -1;
                }

                err = cbor_encode_text_string(&parentEncoder, (char *)baseUriStr, baseUriLen);
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encode_text_string FAILED err=%d", err);
                    return -1;
                }

                err = cbor_encoder_close_container(&parentEncoder, &parentEncoder);/// Closing the map with a base-name + baseUri
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encoder_close_container FAILED err=%d", err);
                    return -1;
                }
                
                head += cbor_encoder_get_buffer_size(&parentEncoder, buffer);
                // // if (bufferLen - head < baseUriLen + JSON_BN_HEADER_SIZE + 2) return -1;
                // memcpy(buffer + head, JSON_BN_HEADER, JSON_BN_HEADER_SIZE);
                // head += JSON_BN_HEADER_SIZE;
                // memcpy(buffer + head, baseUriStr, baseUriLen);
                // head += baseUriLen;
                
                *baseNameOutput = true;
            }

            if (!baseUriLen || level > baseLevel)
            {
                // if (bufferLen - head < JSON_ITEM_URI_SIZE) return -1;
                // memcpy(buffer + head, JSON_ITEM_URI, JSON_ITEM_URI_SIZE);
                // head += JSON_ITEM_URI_SIZE;

                err = cbor_encoder_create_map(&parentEncoder, &parentEncoder, 2);
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encoder_create_map FAILED err=%d", err);
                    return -1;
                }

                err = cbor_encode_int(&parentEncoder, 0); /// Name code in SENML-CBOR == (0)
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encode_int FAILED err=%d", err);
                    return -1;
                }

                if (parentUriLen > 0)
                {
                    if (bufferLen - head < parentUriLen) return -1;
                    parentUriStr[parentUriLen++] = tlvP->id;
                    err = cbor_encode_text_string(&parentEncoder, (char *)parentUriStr, parentUriLen);
                    if (err != CborNoError) 
                    {
                        LOG_ARG("cbor_encode_text_string FAILED err=%d", err);
                        return -1;
                    }
                    // memcpy(buffer + head, parentUriStr, parentUriLen);
                    // head += parentUriLen;
                }
                // res = utils_intToText(tlvP->id, buffer + head, bufferLen - head);
                // if (res <= 0) return -1;
                // head += res;


                if (bufferLen - head < 2) return -1;
                // buffer[head++] = JSON_ITEM_URI_END;
                err = cbor_encoder_close_container(&parentEncoder, &parentEncoder);/// Closing the map with a name + parentUri + ID
                if (err != CborNoError) 
                {
                    LOG_ARG("cbor_encoder_close_container FAILED err=%d", err);
                    return -1;
                }
                head += cbor_encoder_get_buffer_size(&parentEncoder, buffer);
                // if (tlvP->type != LWM2M_TYPE_UNDEFINED)
                // {
                //     buffer[head++] = JSON_SEPARATOR;
                // }
            }

            if (tlvP->type != LWM2M_TYPE_UNDEFINED)
            {
                res = senml_cbor_serializeValue(&parentEncoder, tlvP, buffer + head, bufferLen - head);
                if (res < 0) return -1;
                head += res;
            }

            if (bufferLen - head < 1) return -1;
            err = cbor_encoder_close_container(&parentEncoder, &parentEncoder);/// Closing the whole array
            if (err != CborNoError) 
            {
                LOG_ARG("cbor_encoder_close_container FAILED err=%d", err);
                return -1;
            }

            break;
        }
    }///!switch-case

    LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    for (int i = 0; i <= (int)head; i++){
        lwm2m_printf("%x", buffer[i]);
    }
    LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    // memcpy(buffer, buffer, head);
    return (int)head;
}

int senml_cbor_serialize(const lwm2m_uri_t * uriP, 
                         int size,
                         const lwm2m_data_t * dataP, 
                         uint8_t ** bufferP)
{
    size_t length = 0;
    *bufferP = NULL;
    lwm2m_data_t * targetP;

    uint8_t bufferCBOR[1024];
    int baseUriLen;
    int num;
    uri_depth_t rootLevel;
    uri_depth_t baseLevel;
    uint8_t baseUriStr[URI_MAX_STRING_LEN];
    uint8_t *parentUriStr = NULL;
    size_t parentUriLen = 0;

    baseUriLen = uri_toString(uriP, baseUriStr, URI_MAX_STRING_LEN, &baseLevel);
    if (baseUriLen < 0) return -1;
    if (baseUriLen > 1 && baseLevel != URI_DEPTH_RESOURCE && baseLevel != URI_DEPTH_RESOURCE_INSTANCE)
    {
        if (baseUriLen >= URI_MAX_STRING_LEN -1) return 0;
        baseUriStr[baseUriLen++] = '/';
    }

    num = json_findAndCheckData(uriP, baseLevel, size, dataP, &targetP, &rootLevel);
    LOG_ARG("num = %d", num);
    if (num < 0) return -1;

    if (baseLevel < rootLevel && baseUriLen > 1 && baseUriStr[baseUriLen - 1] != '/')
    {
        if (baseUriLen >= URI_MAX_STRING_LEN -1) return 0;
        baseUriStr[baseUriLen++] = '/';
    }

    LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>baseUri 1 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");    
    lwm2m_printf("%.*s", baseUriLen, baseUriStr);
    LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");    

    if (!baseUriLen || baseUriStr[baseUriLen - 1] != '/')
    {
        parentUriStr = (uint8_t *)"/";
        parentUriLen = 1;
    }
    
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
                                bufferCBOR + length, 1024 - length );
        LOG_ARG("res = %d", res);
        LOG(">>>>>>>>>>>>> CHECKPOINT 7 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>"); 
    
        if (res < 0) {
            LOG(" ERROR"); 
            return res;
        }
        LOG(">>>>>>>>>>>>> CHECKPOINT 8 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>"); 
        length += res; 
        LOG_ARG("length = %d", length); 
    }///!for
    if (length + 1 > DEFAULT_BUFF_SIZE) return 0;
    LOG(">>>>>>>>>>>>> CHECKPOINT 9 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>"); 

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

