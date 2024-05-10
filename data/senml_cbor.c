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
#define DEFAULT_BUFF_SIZE           (1024UL)
#define MAX_URI_VAL                 (65535UL)

/// @brief Function to count backslashes in the string(uri-string in almost cases)
/// @param str 
/// @return 
int countBackSlashes(const char* str) {
    int count = 0;
    while (*str) {
        if (*str == '/') {
            count++;
        }
        str++;
    }
    return count;
}

/// @brief Function that parses the value according to the map-values from the table(see above)
/// @param valueP 
/// @param mapValP 
/// @param dataP 
/// @return 
bool prv_parse_value(CborValue valueP, uint64_t mapValP, lwm2m_data_t *dataP)
{
    switch (mapValP)
    {
        case SENML_CBOR_MAP_TIME:
            if (cbor_value_get_int64_checked(&valueP, &dataP->value.asInteger))
            {
                LOG("cbor_value_get_int64_checked FAILED with");
                return false;
            }
            dataP->type = LWM2M_TYPE_TIME; /// SENML-CBOR does not need the tag-time, it has time-map-key instead 
            break;
        case SENML_CBOR_MAP_VALUE:
            if (cbor_value_is_unsigned_integer(&valueP)){
                if (cbor_value_get_uint64(&valueP, &dataP->value.asUnsigned))
                {
                    LOG("cbor_value_get_uint64 FAILED with error");
                    lwm2m_free(dataP);
                    return false;
                }
                dataP->type = LWM2M_TYPE_UNSIGNED_INTEGER;
                break;
            }
            else if (cbor_value_is_integer(&valueP)){
                if (cbor_value_get_int64_checked(&valueP, &dataP->value.asInteger))
                {
                    LOG("cbor_value_get_int64_checked FAILED with error");
                    return false;
                }
                dataP->type = LWM2M_TYPE_INTEGER;
                break;
            }
            else if (cbor_value_is_double(&valueP)){
                if (cbor_value_get_double(&valueP, &dataP->value.asFloat))
                {
                    LOG("cbor_value_get_double FAILED with error");
                    return false;
                }
                dataP->type = LWM2M_TYPE_FLOAT;
                break;
            }
            else if (cbor_value_is_float(&valueP)){
                float tempFloatValue;
                if (cbor_value_get_float(&valueP, &tempFloatValue))
                {
                    LOG("cbor_value_get_float FAILED with error");
                    return false;
                }
                dataP->value.asFloat = (double)tempFloatValue;
                dataP->type = LWM2M_TYPE_FLOAT;
                break;
            }
            else if (cbor_value_is_half_float(&valueP)){
                float tempFloatValue;
                if (cbor_value_get_half_float_as_float(&valueP, &tempFloatValue)) {
                    LOG("cbor_value_get_half_float_as_float FAILED with error");
                    return false;
                }
                dataP->value.asFloat = (double)tempFloatValue;
                dataP->type = LWM2M_TYPE_FLOAT;
                break;
            }
            break;
        case SENML_CBOR_MAP_OPAQUE:
            {
                uint8_t *temp = NULL;
                size_t tempLen = 0;
                if (cbor_value_dup_byte_string(&valueP, &temp, &tempLen, NULL)) {
                    LOG("cbor_value_dup_byte_string FAILED with error");
                    return false;     
                }
                dataP->value.asBuffer.buffer = (uint8_t *)temp;
                dataP->value.asBuffer.length = tempLen;
                dataP->type = LWM2M_TYPE_OPAQUE;
                break;
            }
        case SENML_CBOR_MAP_STRING_VALUE:
            {
                char *temp = NULL;
                size_t tempLen = 0;
                if (cbor_value_dup_text_string(&valueP, &temp, &tempLen, NULL)) {
                    LOG("cbor_value_dup_text_string FAILED with error");
                    return false;
                }

                dataP->value.asBuffer.buffer = (uint8_t *)temp;
                dataP->value.asBuffer.length = tempLen;
                dataP->type = LWM2M_TYPE_STRING;
                break;
            }
        case SENML_CBOR_MAP_BOOL_VALUE:
            if (cbor_value_get_boolean(&valueP, &dataP->value.asBoolean))
            {
                LOG("cbor_value_get_boolean FAILED with error");
                return false;
            }
            dataP->type = LWM2M_TYPE_BOOLEAN;
            break;
        default:
            LOG_ARG("Error: Senml Cbor Wrong MapType 0x%x", mapValP);
            return false; 
    }

    return true;
}

static lwm2m_data_t * prv_crate_lwm2m_data_children(lwm2m_data_t * baseDataP, uint16_t id, lwm2m_data_type_t type) {
    lwm2m_data_t *children = NULL;

    if (baseDataP == NULL) {
        LOG("Error: baseDataP is NULL");
        return NULL;
    }

    if (baseDataP->value.asChildren.count == 0) {
        children = lwm2m_data_new(1);
        if (children == NULL) {
            return NULL;
        }
        children->id = id;
        children->type = type;
        baseDataP->value.asChildren.array = children;
        baseDataP->value.asChildren.count = 1;
    } else {
        size_t chCnt = baseDataP->value.asChildren.count;
        lwm2m_data_t *chArr = baseDataP->value.asChildren.array;
        for (size_t i = 0; i < chCnt; i++) {
            if (chArr[i].id == id) {
                children = &chArr[i];
                break;
            }
        }
        if (children == NULL) {
            lwm2m_data_t *tmpArr = lwm2m_data_new(chCnt + 1);
            if (tmpArr == NULL) {
                return NULL;
            }
            memcpy(tmpArr, chArr, chCnt * sizeof(lwm2m_data_t));
            lwm2m_free(chArr);
            baseDataP->value.asChildren.array = tmpArr;
            baseDataP->value.asChildren.count++;
            children = &baseDataP->value.asChildren.array[chCnt];
            children->id = id;
            children->type = type;
        }
    }

    return children;
}

static lwm2m_data_t * prv_create_lwm2m_data_item_by_uri(lwm2m_data_t ** baseDataP, lwm2m_uri_t *uriP) {
    lwm2m_data_t *objItem = NULL;
    lwm2m_data_t *instItem = NULL;
    lwm2m_data_t *resItem = NULL;
    lwm2m_data_t *resInstItem = NULL;

    if (baseDataP == NULL || uriP == NULL) {
        LOG("Error: baseDataP or uri is NULL");
        return NULL;
    }
    if (uriP->objectId == LWM2M_MAX_ID) {
        LOG("Error: Object ID is not set");
        return NULL;
    }

    // Find the object
    if (*baseDataP == NULL) {
        objItem = lwm2m_data_new(1);
        if (objItem == NULL) {
            return NULL;
        }
        objItem->id = uriP->objectId;
        objItem->type = LWM2M_TYPE_OBJECT;
        *baseDataP = objItem;
    } else if ((*baseDataP)->id == uriP->objectId) {
        objItem = *baseDataP;
    } else {
        LOG("Error: Object ID does not match");
        return NULL;
    }
    // Return the object if no instance is set
    if (!LWM2M_URI_IS_SET_INSTANCE(uriP)) return objItem;

    // Find or create the object instance
    instItem = prv_crate_lwm2m_data_children(objItem, uriP->instanceId, LWM2M_TYPE_OBJECT_INSTANCE);
    if (instItem == NULL) {
        LOG("Error: Failed to create object instance");
        return NULL;
    }
    // Return the object instance if no resource is set
    if (!LWM2M_URI_IS_SET_RESOURCE(uriP)) return instItem;

    // Find or create the resource
    resItem = prv_crate_lwm2m_data_children(instItem, uriP->resourceId, LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP)? LWM2M_TYPE_MULTIPLE_RESOURCE : LWM2M_TYPE_UNDEFINED);
    if (resItem == NULL) {
        LOG("Error: Failed to create resource");
        return NULL;
    }
    // Return the resource if no resource instance is set
    if (!LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP)) return resItem;

    // Find or create the resource instance
    resInstItem = prv_crate_lwm2m_data_children(resItem, uriP->resourceInstanceId, LWM2M_TYPE_UNDEFINED);
    if (resInstItem == NULL) {
        LOG("Error: Failed to create resource instance");
        return NULL;
    }

    return resInstItem;
}

static size_t prv_generate_lwm2m_structure(CborValue cborArray, lwm2m_data_t ** dataP) {
    CborError err;
    CborValue arrayElement;

    // Iterate over each element in the array
    if (cbor_value_enter_container(&cborArray, &arrayElement)) {
        LOG_ARG("cbor_value_enter_container array FAILED with error %d", err);
        goto error;
    }

    // Throw away the array container
    while (!cbor_value_at_end(&arrayElement)) {
        CborValue mapEntry;
        size_t mapSize = 0;
        lwm2m_data_t *newDataP = NULL;
        
        if (cbor_value_get_type(&arrayElement) != CborMapType) {
            LOG("Expected a map in array");
            goto error;
        }
        if (cbor_value_get_map_length(&arrayElement, &mapSize) || mapSize != 2) {
            LOG("Error getting map length or incorrect map size");
            goto error;
        }

        // Enter the map container
        if (cbor_value_enter_container(&arrayElement, &mapEntry)) {
            LOG("Error entering map container");
            goto error;
        }

        // Throw away the map container
        while (!cbor_value_at_end(&mapEntry)) {
            // Process each key-value pair in the map
            uint64_t keyValue;
            if (cbor_value_get_uint64(&mapEntry, &keyValue)) {
                LOG("Error getting key value");
                goto error;
            }

            // Move to the value
            if (cbor_value_advance(&mapEntry)) {
                LOG("Error advancing map entry");
                goto error;
            }

            if (keyValue == SENML_CBOR_MAP_NAME) {
                lwm2m_uri_t uri;
                size_t len = URI_MAX_STRING_LEN;
                char uriStr[len];
                if (cbor_value_copy_text_string(&mapEntry, uriStr, &len, NULL)) {
                    LOG("Error copying text string");
                    goto error;
                }
                // Convert the URI string to a URI structure
                lwm2m_stringToUri(uriStr, len, &uri);
                newDataP = prv_create_lwm2m_data_item_by_uri(dataP, &uri);
            } else if (newDataP) {
                if (!prv_parse_value(mapEntry, keyValue, newDataP)) {
                    LOG_ARG("prv_parse_value FAILED with error %d", err);
                    goto error;
                }
            } else {
                LOG_ARG("Unexpected key value: %d", keyValue);
                goto error;
            }

            // Move to next map entry
            if (cbor_value_advance(&mapEntry)) {
                LOG("Error advancing map entry");
                goto error;
            }
        }

        // Leave the map container
        if (cbor_value_leave_container(&arrayElement, &mapEntry)) {
            LOG("Error leaving map container");
            goto error;
        }
    }

    // Leave the array container
    if (cbor_value_leave_container(&cborArray, &arrayElement)) {
        LOG("Error leaving array container");
        goto error;
    }

    return 1;

error:
    LOG("Error in parsing CBOR array");
    if (*dataP) {
        // *dataP always contain one object instance
        lwm2m_data_free(1, *dataP);
        *dataP = NULL;
    }
    return 0;
}

/// @brief Main function to parse the SENML-CBOR arrays into the lwm2m_data_t structures
/// @param uriP 
/// @param buffer 
/// @param bufferLen 
/// @param dataP 
/// @return 
int senml_cbor_parse(const lwm2m_uri_t * uriP,
                     const uint8_t * buffer, 
                     size_t bufferLen, 
                     lwm2m_data_t ** dataP)
{
    CborParser parser;
    CborValue array;
    *dataP = NULL;               /// pointer to the final lwm2m struct
    size_t arrLen = 0, dataCnt = 0;

    if (uriP == NULL){
        LOG("UriP is NULL, some error occured!");
        return -1;
    }
    LOG_URI(uriP);
    LOG_ARG("bufferLen: %d", bufferLen);
 
    if (cbor_parser_init(buffer, bufferLen, 0, &parser, &array))
    {
        LOG("cbor_parser_init FAILED with error");
        return -1;
    }

    if (cbor_value_get_type(&array) != CborArrayType) 
    {
        LOG("Error: type is not CborArrayType");
        return -1;
    }

    if (cbor_value_get_array_length(&array, &arrLen))
    {
        LOG("cbor_value_get_array_length FAILED with error");
        return -1;
    }
    if (arrLen <= 0) {
        LOG_ARG("wrong array length(%d) FAILED", arrLen);
        return -1;
    }

    dataCnt = prv_generate_lwm2m_structure(array, dataP);
    if (dataCnt == 0) {
        LOG("prv_generate_lwm2m_structure FAILED");
        return -1;
    }

    if (LWM2M_URI_IS_SET_OBJECT(uriP)) {
        lwm2m_data_t *tmpDataP = *dataP;
        if (dataCnt != 1) {
            LOG("Error occured during data parsing");
            lwm2m_data_free(dataCnt, *dataP);
            *dataP = NULL;
            return -1; 
        }
        dataCnt = tmpDataP->value.asChildren.count;
        *dataP = tmpDataP->value.asChildren.array;
        lwm2m_free(tmpDataP);
    } else {
        return dataCnt;
    }

    if (LWM2M_URI_IS_SET_INSTANCE(uriP)) {
        lwm2m_data_t *tmpDataP = *dataP;
        if (dataCnt != 1) {
            LOG("Error occured during data parsing");
            lwm2m_data_free(dataCnt, *dataP);
            *dataP = NULL;
            return -1; 
        }
        dataCnt = tmpDataP->value.asChildren.count;
        *dataP = tmpDataP->value.asChildren.array;
        lwm2m_free(tmpDataP);
    } else {
        return dataCnt;
    }

    if (LWM2M_URI_IS_SET_RESOURCE(uriP) && LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP)) {
        lwm2m_data_t *tmpDataP = *dataP;
        if (dataCnt != 1) {
            LOG("Error occured during data parsing");
            lwm2m_data_free(dataCnt, *dataP);
            *dataP = NULL;
            return -1;
        }
        dataCnt = tmpDataP->value.asChildren.count;
        *dataP = tmpDataP->value.asChildren.array;
        lwm2m_free(tmpDataP);
    } else {
        return dataCnt;
    }

    if (dataCnt != 1) {
        LOG("Error occured during data parsing");
        lwm2m_data_free(dataCnt, *dataP);
        *dataP = NULL;
        return -1;
    }

    return dataCnt;
}

/// @brief Function to encode a single simple value of data -> just like a simple CBOR format
/// For example:
/// (6476736d71) means "vsmq":
/// 64          # text(4)
/// 76736D71    # "vsmq"
/// @param tlvP 
/// @param buffer 
/// @param bufferSize 
/// @return 
int senml_cbor_encodeValue(const lwm2m_data_t * tlvP, uint8_t * buffer, size_t *bufferSize)
{
    CborError err;
    CborEncoder encoder;
    int length;
    
    cbor_encoder_init(&encoder, buffer, *bufferSize, 0);

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
                return -1; 
            }
            err = cbor_encode_int(&encoder, tlvP->value.asInteger);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            break;
        }
        case LWM2M_TYPE_OBJECT_LINK:
        {
            char tempBuffer[12]; // Sufficient buffer to hold the string representation (e.g., "65535:65535" -> 5 + 1 + 5 == 11 symbols)
            snprintf(tempBuffer, sizeof(tempBuffer), "%u:%u", tlvP->value.asObjLink.objectId, tlvP->value.asObjLink.objectInstanceId);
            err = cbor_encode_text_string(&encoder, tempBuffer, strlen(tempBuffer));
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_text_string/obj_link FAILED with error %d", err);
                return -1; 
            }
            break;
        }
        case LWM2M_TYPE_OPAQUE:
            err = cbor_encode_byte_string(&encoder, tlvP->value.asBuffer.buffer, tlvP->value.asBuffer.length);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_byte_string FAILED with error %d", err);
                return -1; 
            }
            break;
        case LWM2M_TYPE_STRING:
        case LWM2M_TYPE_CORE_LINK:
            err = cbor_encode_text_string(&encoder,(char *) tlvP->value.asBuffer.buffer, tlvP->value.asBuffer.length);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_text_string FAILED with error %d", err);
                return -1; 
            }
            break;
        case LWM2M_TYPE_INTEGER:
            err = cbor_encode_int(&encoder,tlvP->value.asInteger);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            break;
        case LWM2M_TYPE_UNSIGNED_INTEGER:
            err = cbor_encode_uint(&encoder, tlvP->value.asUnsigned);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            break;
        case LWM2M_TYPE_FLOAT:
            err = cbor_encode_double(&encoder, tlvP->value.asFloat);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_double FAILED with error %d", err);
                return -1; 
            }
            break;
        case LWM2M_TYPE_BOOLEAN:
            err = cbor_encode_boolean(&encoder, tlvP->value.asBoolean);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_boolean FAILED with error %d", err);
                return -1; 
            }
            break;
        default:
            LOG_ARG("Something went wrong FAILED with error %d", err);
            return -1;
            break;
    }
    
    length = cbor_encoder_get_buffer_size(&encoder, buffer);
    return length;
}

/// @brief Function to encode the map of {6: 257} -> senml-cbor value-map-type + value itself.
/// Differs from the single-value encoding that we wrapp the value with the MAP-code(see the Table)
/// @param encoder 
/// @param tlvP 
/// @param buffer 
/// @return 
CborError senml_cbor_encodeValueWithMap(CborEncoder* encoder, const lwm2m_data_t * tlvP)
{
    CborError err;
    
    switch (tlvP->type) 
    {
        case LWM2M_TYPE_UNDEFINED:
        case LWM2M_TYPE_OBJECT:
        case LWM2M_TYPE_OBJECT_INSTANCE:
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
            LOG_ARG("Wrong tlvP-type(%d) FAILED ", tlvP->type);
            return -1;
            break;
        case LWM2M_TYPE_TIME:
        {
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_TIME); /// SENML-CBOR has his own map-key for the time-type values(it differs from CBOR-time-keying)
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            err = cbor_encode_int(encoder, tlvP->value.asInteger);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            break;
        }
        case LWM2M_TYPE_OBJECT_LINK:
        {
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_OBJ_LINK);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            char tempBuffer[12]; // Sufficient buffer to hold the string representation (e.g., "65535:65535" -> 5 + 1 + 5 == 11 symbols)
            snprintf(tempBuffer, sizeof(tempBuffer), "%u:%u", tlvP->value.asObjLink.objectId, tlvP->value.asObjLink.objectInstanceId);
            err = cbor_encode_text_string(encoder, tempBuffer, strlen(tempBuffer));
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_text_string/obj_link FAILED with error %d", err);
                return -1; 
            }
            break;
        }
        case LWM2M_TYPE_OPAQUE:
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_OPAQUE);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            err = cbor_encode_byte_string(encoder, tlvP->value.asBuffer.buffer, tlvP->value.asBuffer.length);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_byte_string FAILED with error %d", err);
                return -1; 
            }
            break;
        case LWM2M_TYPE_STRING:
        case LWM2M_TYPE_CORE_LINK:
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_STRING_VALUE);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            err = cbor_encode_text_string(encoder,(char *) tlvP->value.asBuffer.buffer, tlvP->value.asBuffer.length);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_text_string FAILED with error %d", err);
                return -1; 
            }
            break;
        case LWM2M_TYPE_INTEGER:
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_VALUE);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            err = cbor_encode_int(encoder,tlvP->value.asInteger);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            break;
        case LWM2M_TYPE_UNSIGNED_INTEGER:
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_VALUE);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                
                return -1; 
            }
            err = cbor_encode_uint(encoder, tlvP->value.asUnsigned);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            break;
        case LWM2M_TYPE_FLOAT:
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_VALUE);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                
                return -1; 
            }
            err = cbor_encode_double(encoder, tlvP->value.asFloat);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_double FAILED with error %d", err);
                return -1; 
            }
            break;
        case LWM2M_TYPE_BOOLEAN:
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_BOOL_VALUE);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            err = cbor_encode_boolean(encoder, tlvP->value.asBoolean);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_boolean FAILED with error %d", err);
                
                return -1; 
            }
            break;
        default:
            LOG_ARG("ERROR WRONG tlvP-type  %d", tlvP->type );
            return -1;
            break;
    }

    err = cbor_encoder_close_container_checked(encoder, encoder); /// Closing map (it was opened in senml_cbor_encodeUri())
    if (err != CborNoError) 
    {
        LOG_ARG("cbor_encoder_close_container FAILED err=%d", err);
        return -1;
    }

    return err;
}

/// @brief Function to encode the map of {0: "3/3/3/3"} -> senml-cbor name + uri in a string format.
/// Here we create a map for 2 pairs: 
/// 1. 0:uri-string
/// 2. senml-cbor-maptype: value-itself
/// but the second pair will be added in the senml_cbor_encodeValueWithMap()
/// @param encoder 
/// @param uriP 
/// @return 
CborError senml_cbor_encodeUri(CborEncoder* encoder, lwm2m_uri_t* uriP)
{
    CborError err;
    size_t uriStrLen = 0;
    uri_depth_t level;
    uint8_t uriStr[URI_MAX_STRING_LEN];
    uriStrLen = uri_toString(uriP, uriStr, URI_MAX_STRING_LEN, &level);

    err = cbor_encoder_create_map(encoder, encoder, 2);/// opening map, will be closed in the senml_cbor_encodeValueWithMap()
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

    err = cbor_encode_text_string(encoder, (char*)uriStr, uriStrLen);/// uri as a textString
    if (err != CborNoError) 
    {
        LOG_ARG("cbor_encode_text_string FAILED err=%d", err);
        return -1;
    }
    
    return err;
}

// Function to encode several Resources
CborError encode_resources(CborEncoder* encoder, lwm2m_uri_t *uriP, const lwm2m_data_t *data) {
    CborError err;
    err = senml_cbor_encodeUri(encoder, uriP);
    if (err != CborNoError) 
    {
        LOG_ARG("senml_cbor_encodeUri FAILED err=%d", err);
        return -1;
    }
    err = senml_cbor_encodeValueWithMap(encoder, data);
    if (err != CborNoError) 
    {
        LOG_ARG("senml_cbor_encodeValueWithMap FAILED err=%d", err);
        return -1;
    }
    return err;
}

/// @brief Sets the uriIds for the future concatenating into one uri-string
/// @param id1 
/// @param id2 
/// @param currId 
/// @return 
CborError setUrisIds(uint16_t *id1, uint16_t *id2, const uint16_t *currId ){
    if (*id1 == MAX_URI_VAL){
        *id1 = *currId;
    }
    else if (*id2 == MAX_URI_VAL){
        *id2 = *currId;
    }
    else {
        LOG("Error setUrisIds with ids"); //error
        return -1;
    }
    return CborNoError;
}

/// @brief Clears the uriIds to keep the correct motion through the for cycle in the prv_serialize()
/// @param id1 
/// @param id2 
/// @return 
CborError cleanUrisIds(uint16_t* id1, uint16_t* id2){
    if (*id1 != MAX_URI_VAL){
        *id1 = MAX_URI_VAL;
    }
    else if (*id2 != MAX_URI_VAL){
        *id2 = MAX_URI_VAL;
    }
    else {
        LOG("Error cleanUrisIds with ids"); //error
        return -1;
    }
    return CborNoError;
}

/// @brief Function for the recursive calling to serialize the array of data
/// @param encoderP 
/// @param uriP 
/// @param dataP 
/// @param size 
/// @param bufferP 
/// @return 
CborError prv_serialize(CborEncoder* encoderP, lwm2m_uri_t* uriP, const lwm2m_data_t *dataP,  int size) 
{
    CborError err;
    LOG_URI(uriP);
    for (int i = 0 ; i < size; i++)
    {
        const lwm2m_data_t * cur = &dataP[i];
        switch (cur->type)
        {
            case LWM2M_TYPE_OBJECT_INSTANCE:
            case LWM2M_TYPE_MULTIPLE_RESOURCE:
                {
                    err = setUrisIds(&(uriP->instanceId), &(uriP->resourceId), &(cur->id));
                    if (err != CborNoError) 
                    {
                        LOG_ARG("checkUrisIds FAILED err=%d", err);
                        return -1;
                    }
                    err = prv_serialize(encoderP, uriP, cur->value.asChildren.array, cur->value.asChildren.count);
                    if (err != CborNoError) 
                    {
                        LOG_ARG("prv_serialize FAILED err=%d", err);
                        return -1;
                    }
                    err = cleanUrisIds(&(uriP->resourceId), &(uriP->instanceId));
                    if (err != CborNoError) 
                    {
                        LOG_ARG("prv_serialize FAILED err=%d", err);
                        return -1;
                    }
                }
                break;
            case LWM2M_TYPE_STRING:
            case LWM2M_TYPE_OPAQUE:
            case LWM2M_TYPE_INTEGER:
            case LWM2M_TYPE_UNSIGNED_INTEGER:
            case LWM2M_TYPE_BOOLEAN:
            case LWM2M_TYPE_FLOAT:
            case LWM2M_TYPE_TIME:
            case LWM2M_TYPE_OBJECT_LINK:
            case LWM2M_TYPE_CORE_LINK:
                err = setUrisIds(&(uriP->resourceId), &(uriP->resourceInstanceId), &(cur->id));
                if (err != CborNoError) 
                {
                    LOG_ARG("checkUrisIds FAILED err=%d", err);
                    return -1;
                }
                err = encode_resources(encoderP, uriP, cur);
                if (err != CborNoError) 
                {
                    LOG_ARG("encode_resources FAILED err=%d", err);
                    return -1;
                }
                err = cleanUrisIds(&(uriP->resourceInstanceId), &(uriP->resourceId));
                if (err != CborNoError) 
                {
                    LOG_ARG("prv_serialize FAILED err=%d", err);
                    return -1;
                }
                break;
            default:
                LOG_ARG("prv_serialize FAILED wrong or unsupported type=%d", cur->type);
                return (-1);
                break;
        }///!switch
    }///!for
  
    return err;
}

/// @brief Function counts the items for the future SENML-CBOR array: (maps)
/// @param dataP 
/// @param size 
/// @return 
size_t get_array_items_count(const lwm2m_data_t *dataP,  int size){
    size_t mapsCount = 0;
    for (int i = 0 ; i < size; i++)
    {
        const lwm2m_data_t * cur = &dataP[i];
        switch (cur->type)
        {
            case LWM2M_TYPE_OBJECT_INSTANCE:
            case LWM2M_TYPE_MULTIPLE_RESOURCE:
                    mapsCount += get_array_items_count(cur->value.asChildren.array, cur->value.asChildren.count);
                break;
            case LWM2M_TYPE_STRING:
            case LWM2M_TYPE_OPAQUE:
            case LWM2M_TYPE_INTEGER:
            case LWM2M_TYPE_UNSIGNED_INTEGER:
            case LWM2M_TYPE_BOOLEAN:
            case LWM2M_TYPE_FLOAT:
            case LWM2M_TYPE_TIME:
            case LWM2M_TYPE_OBJECT_LINK:
            case LWM2M_TYPE_CORE_LINK:
                mapsCount++;
                break;
            default:
                return (-1);
                break;
        }///!switch
    }///!for
  
    return mapsCount;
}

/// @brief Places the encoded SENML-CBOR data into the prepared buffer
/// @param bufferP 
/// @param length 
/// @param encoderBuffer 
/// @return 
int putEncodedIntoBuffer(uint8_t** bufferP, size_t length, uint8_t* encoderBuffer){
    *bufferP = (uint8_t *)lwm2m_malloc(length);
    if (bufferP == NULL) {
        LOG("Error bufferP is empty");
        return -1; 
    }
    
    memcpy(*bufferP, encoderBuffer, length);

    LOG_ARG("returning %u", length);
    return (int)length;
}

/// @brief Function to serialize the lwm2m_data_t structure into the SENML-CBOR coded buffer
/// @param uriP 
/// @param size 
/// @param dataP 
/// @param bufferP 
/// @return 
int senml_cbor_serialize(lwm2m_uri_t * uriP, int size, const lwm2m_data_t * dataP, uint8_t ** bufferP)
{
    size_t length = 0;
    *bufferP = NULL;
    size_t bufferSize = DEFAULT_BUFF_SIZE; /// TODO Add the ability to calculate the required memory for the cbor buffer
    uint8_t encoderBuffer[DEFAULT_BUFF_SIZE];

    LOG_ARG("size: %d", size);
    LOG_URI(uriP);
        
    if ( (size == 1) && (uriP->resourceId != 65535) && (dataP->type != LWM2M_TYPE_MULTIPLE_RESOURCE))
    {
        length = senml_cbor_encodeValue(dataP, encoderBuffer, &bufferSize);
    }
    else if (uriP != NULL && LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP)){
        size = dataP->value.asChildren.count;
        if(size != 1) return -1;
        dataP = dataP->value.asChildren.array;
        length = senml_cbor_encodeValue(dataP, encoderBuffer, &bufferSize);
    }
    else 
    {
        CborEncoder encoder;
        CborError err;
        size_t arraySize = 0;
        if((size == 1) && (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)){
            size = dataP->value.asChildren.count;
            dataP = dataP->value.asChildren.array;
            if(size != 1) return -1;
        }

        arraySize = get_array_items_count(dataP, size); /// Counting data for the future SENML-CBOR array

        cbor_encoder_init(&encoder, encoderBuffer, DEFAULT_BUFF_SIZE, 0);

        err = cbor_encoder_create_array(&encoder, &encoder, arraySize);/// Opening array once
        if (err != CborNoError) 
        {
            LOG_ARG("cbor_encoder_create_array FAILED err=%d", err);
            return -1;
        }

            err = prv_serialize(&encoder, uriP, dataP, size); /// Recoursive call
            if (err != CborNoError) 
            {
                LOG_ARG("cbor_encoder_create_array FAILED err=%d", err);
                return -1;
            }

        err = cbor_encoder_close_container(&encoder, &encoder); /// Closing array once
        if (err != CborNoError) 
        {
            LOG_ARG("cbor_encoder_close_container FAILED err=%d", err);
            return -1;
        }
        length = cbor_encoder_get_buffer_size(&encoder, encoderBuffer);/// Getting the final length of the encoded buffer
    }///!else

    return putEncodedIntoBuffer(bufferP, length, encoderBuffer);
}

