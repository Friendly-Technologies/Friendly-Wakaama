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

CborError senml_cbor_parseValue(CborValue* valueP, CborType* typeP, lwm2m_data_t* dataP)
{
    CborError err;
    LOG_ARG("type = %d", *typeP);
    switch (*typeP)
    {
        case CborArrayType:
        case CborMapType:
            LOG_ARG("Something went wrong 0x%x\n", CborArrayType);
            return -1; 
            break;
        case CborTagType:
        {
            CborTag tag;
            err = cbor_value_get_tag(valueP, &tag);
            if (err != CborNoError)
            {
                LOG("Error: cbor_value_get_tag \n");
                return -1;
            }
            if (tag != CborUnixTime_tTag)
            {
                LOG("Error: tag is NOT CborUnixTime_tTag \n"); /// now we support only this kind of a TAG
                return -1;
            }
            err = cbor_value_advance_fixed(valueP);
            if (err != CborNoError)
            {
                LOG("Error: cbor_value_advance_fixed \n");
                return -1;
            }
            err = cbor_value_get_int64_checked(valueP, &dataP->value.asInteger);
            if (err != CborNoError)
            {
                LOG("Error: cbor_value_get_int64_checked \n");
                return -1;
            }
            dataP->type = LWM2M_TYPE_TIME; /// now it's the TIME is the only type with a TAG
            break;
        }
        case CborSimpleType:
        case CborIntegerType:
            if (cbor_value_is_unsigned_integer(valueP)){
                err = cbor_value_get_uint64(valueP, &dataP->value.asUnsigned);
                if (err != CborNoError)
                {
                    LOG_ARG("cbor_value_get_uint64 FAILED with error %d", err);
                    lwm2m_free(dataP);
                    return -1;
                }
                dataP->type = LWM2M_TYPE_UNSIGNED_INTEGER;
            }
            else {
                err = cbor_value_get_int64_checked(valueP, &dataP->value.asInteger);
                if (err != CborNoError)
                {
                    LOG_ARG("cbor_value_get_int64_checked FAILED with error %d", err);
                    lwm2m_free(dataP);
                    return -1;
                }
                dataP->type = LWM2M_TYPE_INTEGER;
            }
            break;
        case CborByteStringType:
            {
                uint8_t *temp = NULL;
                size_t temPlen = 0;
              
                err = cbor_value_calculate_string_length(valueP, &temPlen);
                if (err != CborNoError) {
                    LOG_ARG("Error%d: cbor_value_calculate_string_length\n", err);
                    lwm2m_free(dataP);
                    return -1;
                }

                temp = (uint8_t *)lwm2m_malloc(temPlen);
                if (temp == NULL) {
                    LOG("Error: lwm2m_malloc failed\n");
                    lwm2m_free(dataP);
                    return -1;
                }

                err = cbor_value_dup_byte_string(valueP, &temp, &temPlen, valueP);
                if (err!= CborNoError){
                    LOG_ARG("Error%d: cbor_value_dup_byte_string\n", err);
                    lwm2m_free(dataP);
                    return -1;     
                }

                dataP->value.asBuffer.buffer = (uint8_t *)temp;
                dataP->value.asBuffer.length = temPlen;
                dataP->type = LWM2M_TYPE_OPAQUE;
                break;
            }
        case CborTextStringType:
            {
                char *temp = NULL;
                size_t temPlen = 0;

                err = cbor_value_calculate_string_length(valueP, &temPlen);
                if (err != CborNoError) {
                    LOG_ARG("Error%d: cbor_value_calculate_string_length\n", err);
                    lwm2m_free(dataP);
                    return -1;
                }

                temp = (char *)lwm2m_malloc(temPlen);
                if (temp == NULL) {
                    LOG("Error: Memory allocation failed\n");
                    lwm2m_free(dataP);
                    return -1;
                }

                err = cbor_value_dup_text_string(valueP, &temp, &temPlen, valueP);
                if (err!= CborNoError){
                    LOG_ARG("Error%d: cbor_value_dup_text_string\n", err);
                    lwm2m_free(dataP);
                    return -1;
                }

                dataP->value.asBuffer.buffer = (uint8_t *)temp;
                dataP->value.asBuffer.length = temPlen;
                dataP->type = LWM2M_TYPE_STRING;
                break;
            }
        case CborBooleanType:
            err = cbor_value_get_boolean(valueP, &dataP->value.asBoolean);
            if (err != CborNoError)
            {
                LOG_ARG("cbor_value_get_boolean FAILED with error %d", err);
                lwm2m_free(dataP);
                return -1;
            }
            dataP->type = LWM2M_TYPE_BOOLEAN;
            break;
        case CborDoubleType:
            err = cbor_value_get_double(valueP, &dataP->value.asFloat);
            if (err != CborNoError)
            {
                LOG_ARG("cbor_value_get_double FAILED with error %d", err);
                lwm2m_free(dataP);
                return -1;
            }
            dataP->type = LWM2M_TYPE_FLOAT;
            break;
        case CborFloatType:
            {
                float tempFloatValue;
                err = cbor_value_get_float(valueP, &tempFloatValue);
                if (err != CborNoError)
                {
                    LOG_ARG("cbor_value_get_float FAILED with error %d", err);
                    lwm2m_free(dataP);
                    return -1;
                }
                dataP->value.asFloat = (double)tempFloatValue;
                dataP->type = LWM2M_TYPE_FLOAT;
                break;
            }
        case CborHalfFloatType:
            {
                float tempFloatValue;
                err = cbor_value_get_half_float_as_float(valueP, &tempFloatValue);
                if (err != CborNoError)
                {
                    LOG_ARG("cbor_value_get_half_float_as_float FAILED with error %d", err);
                    lwm2m_free(dataP);
                    return -1;
                }
                dataP->value.asFloat = (double)tempFloatValue;
                dataP->type = LWM2M_TYPE_FLOAT;
                break;
            }
        case CborUndefinedType:
        case CborNullType:
        case CborInvalidType:
            LOG_ARG("CborInvalidType 0x%x\n", CborInvalidType);
            lwm2m_free(dataP);
            return -1; 
            break;
        default:
            return -1; 
            break;
    }///!switch-case

    if (dataP == NULL) {
        lwm2m_free(dataP);
        LOG("data is empty ");
        return -1; 
    }
    
    return err;
}


int senml_cbor_parse(const lwm2m_uri_t * uriP,
                     const uint8_t * buffer, 
                     size_t bufferLen, 
                     lwm2m_data_t ** dataP)
{
    CborParser parser;
    CborValue array;
    CborError err;
    lwm2m_data_t *inData = NULL;
    size_t dataSize = 0;
    *dataP = NULL;

    LOG_ARG("bufferLen: %d", bufferLen);
    LOG("SENML CBOR parse --- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> ");
    for (uint32_t i = 0; i < bufferLen; i++)
    {
        lwm2m_printf("%02x", buffer[i]);
    }
    LOG("SENML CBOR parse --- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> ");
    
    LOG_URI(uriP);
    if (uriP == NULL){
        return -1;
    }

    inData = lwm2m_data_new(dataSize + 1);
    if (inData == NULL) {
        lwm2m_free(inData);
        LOG("lwm2m_data_new FAILED");
        return -1; 
    }

    if (LWM2M_URI_IS_SET_RESOURCE(uriP)){
        inData->id = uriP->resourceId;
    }

    err = cbor_parser_init(buffer, bufferLen, 0, &parser, &array);    
    if (err != CborNoError)
    {
        LOG_ARG("cbor_parser_init FAILED with error %d", err);
        return -1;
    }

    // err = cbor_value_validate(&array, CborValidateStrictMode);
    // if (err != CborNoError)
    // {
    //     LOG_ARG("CBOR parsing failure at offset %ld: %s", array.source.ptr - buffer, cbor_error_string(err));
    //     return -1;
    // }

    CborType type = cbor_value_get_type(&array);
    LOG_ARG("type = %x(%d)", type, type);
    if (type == CborArrayType) 
    {
        LOG("=============================== CHECKPOINT 1 ===================================================");
        // Enter the array
        err = cbor_value_enter_container(&array, &array);
        if (err != CborNoError)
        {
            LOG_ARG("cbor_value_enter_container FAILED with error %d", err);
            return -1;
        }
        type = cbor_value_get_type(&array);
        LOG_ARG("type 2 = %x(%d)", type, type );
        if (type == CborMapType) 
        {

            CborValue map;
            size_t len;
            err = cbor_value_get_map_length(&array, &len);
            LOG_ARG("map length = %d", len);
            
            err = cbor_value_enter_container(&array, &map);
            if (err != CborNoError)
            {
                LOG_ARG("cbor_value_enter_container FAILED with error %d", err);
                return -1;
            }
            uint64_t tempInt;

            if (cbor_value_is_unsigned_integer(&map)){
                err = cbor_value_get_uint64(&map, &tempInt);
                if (err != CborNoError)
                {
                    LOG_ARG("cbor_value_get_uint64 FAILED with error %d", err);
                    return -1;
                }
            }
            LOG_ARG("tempInt = %d", tempInt);
            err = cbor_value_advance_fixed(&map);
            if (err != CborNoError)
            {
                LOG("Error: cbor_value_advance_fixed \n");
                return -1;
            }
            
            if (tempInt == SENML_CBOR_MAP_NAME){
                char *temp = NULL;
                size_t temPlen = 0;
                LOG("=============================== CHECKPOINT 3 ===================================================");
                err = cbor_value_get_string_length(&map, &temPlen);
                if (err != CborNoError) {
                    LOG_ARG("Error%d: cbor_value_calculate_string_length\n", err);
                    return -1;
                }
                LOG_ARG("string length = %d", temPlen);
                LOG("=============================== CHECKPOINT 4 ===================================================");
                temp = (char *)lwm2m_malloc(temPlen);
                if (temp == NULL) {
                    LOG("Error: Memory allocation failed\n");
                    return -1;
                }
                LOG("=============================== CHECKPOINT 5 ===================================================");
                err = cbor_value_dup_text_string(&map, &temp, &temPlen, &map);
                if (err!= CborNoError){
                    LOG_ARG("Error%d: cbor_value_dup_text_string\n", err);
                    return -1;
                }
                LOG("=============================== CHECKPOINT 6 ===================================================");
                uint64_t mapVal;
                if (cbor_value_is_unsigned_integer(&map)){
                    err = cbor_value_get_uint64(&map, &mapVal);
                    if (err != CborNoError)
                    {
                        LOG_ARG("cbor_value_get_uint64 FAILED with error %d", err);
                        return -1;
                    }
                }
                LOG_ARG("mapVal = %d", mapVal);
                err = cbor_value_advance_fixed(&map);
                if (err != CborNoError)
                {
                    LOG("Error: cbor_value_advance_fixed \n");
                    return -1;
                }

                if (mapVal == SENML_CBOR_MAP_VALUE)
                {
                    if (cbor_value_is_unsigned_integer(&map))
                    {
                        err = cbor_value_get_uint64(&map, &inData->value.asUnsigned);
                        if (err != CborNoError)
                        {
                            LOG_ARG("cbor_value_get_uint64 FAILED with error %d", err);
                            lwm2m_free(inData);
                            return -1;
                        }
                        inData->type = LWM2M_TYPE_UNSIGNED_INTEGER;
                        LOG_ARG("inData-value = %d", inData->value.asUnsigned);
                    }
                    else {
                        err = cbor_value_get_int64_checked(&map, &inData->value.asInteger);
                        if (err != CborNoError)
                        {
                            LOG_ARG("cbor_value_get_int64_checked FAILED with error %d", err);
                            lwm2m_free(inData);
                            return -1;
                        }
                        inData->type = LWM2M_TYPE_INTEGER;
                        LOG_ARG("inData-value = %d", inData->value.asInteger);
                    } 
                }///!mapValue
                LOG_ARG("map-remaining = %d", map.remaining);
                // err = cbor_value_advance(&map);
                // if (err != CborNoError)
                // {
                //     LOG("Error: cbor_value_advance_fixed \n");
                //     return -1;
                // }

                // err = cbor_value_leave_container(&map, &array);/// leaving map-container
                // if (err != CborNoError) {
                //     // Error handling
                //     return -1;
                // }
            }///!mapName
        }///!mapType
        // err = cbor_value_leave_container(&array, &array);/// leaving array-container
        // if (err != CborNoError) {
        //     // Error handling
        //     return -1;
        // }
    }///!arrayType
    else {
        LOG("=============================== CHECKPOINT 2 ===================================================");
        err = senml_cbor_parseValue(&array, &type, inData);
        if (err != CborNoError)
        {
            LOG_ARG("senml_cbor_parseValue FAILED with error %d", err);
            return -1;
        }
    }
    
    LOG_ARG("senml_cbor_parse: uriP.objectId = %d, ", uriP->objectId);
    LOG_ARG("senml_cbor_parse: uriP.instanceId = %d, ", uriP->instanceId);
    LOG_ARG("senml_cbor_parse: uriP.resourceId = %d, ", uriP->resourceId);
    LOG("---------------------------");
    LOG_ARG("senml_cbor_parse: dataP.type = %d, ", inData->type);
    LOG_ARG("senml_cbor_parse: dataP.ID = %d, ", inData->id);
    LOG_ARG("senml_cbor_parse: dataP.asBoolean = %d, ", inData->value.asBoolean);
    LOG_ARG("senml_cbor_parse: dataP.asInteger = %d, ", inData->value.asInteger);
    LOG_ARG("senml_cbor_parse: dataP.asUnsigned = %lu, ", inData->value.asUnsigned);
    LOG_ARG("senml_cbor_parse: dataP.asFloat = %f, ", inData->value.asFloat);
    LOG_ARG("senml_cbor_parse: dataP.asBufferLength = %d, ", inData->value.asBuffer.length);
    LOG_ARG("senml_cbor_parse: dataP.asChildrenCount = %d, ", inData->value.asChildren.count);
    LOG_ARG("senml_cbor_parse: dataP.asObjLink.objectId = %d, ", inData->value.asObjLink.objectId);
    LOG_ARG("senml_cbor_parse: dataP.asObjLink.objectInstanceId = %d, ", inData->value.asObjLink.objectInstanceId);
    *dataP = inData; /// copying parsed data into the final lwm2m_data struct
    dataSize++; /// if all is OK, temporary we set dataSize == 1
    return dataSize;
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
            err = cbor_encode_int(encoder, SENML_CBOR_MAP_TIME);
            if (err != CborNoError) {
                LOG_ARG("cbor_encode_int FAILED with error %d", err);
                return -1; 
            }
            // err = cbor_encode_tag(encoder, CborUnixTime_tTag);/// now unixTime is the only tag-ed type
            // if (err != CborNoError) {
            //     return err; 
            // }
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

#define DEFAULT_BUFF_SIZE       (1024UL)
#define MSX_URI_VAL             (65535UL)

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
    if (*id1 == MSX_URI_VAL){
        *id1 = *currId;
    }
    else if (*id2 == MSX_URI_VAL){
        *id2 = *currId;
    }
    else {
        LOG(" ERROR 1 with ids"); //error
        return -1;
    }
    return CborNoError;
}

/// @brief Clears the uriIds to keep the correct motion through the for cycle in the prv_serialize()
/// @param id1 
/// @param id2 
/// @return 
CborError cleanUrisIds(uint16_t* id1, uint16_t* id2){
    if (*id1 != MSX_URI_VAL){
        *id1 = MSX_URI_VAL;
    }
    else if (*id2 != MSX_URI_VAL){
        *id2 = MSX_URI_VAL;
    }
    else {
        LOG(" ERROR  2 with ids"); //error
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
        LOG("bufferP is empty");
        return -1; 
    }
    
    LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>final resulting buffer>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    for (int i = 0; i < (int)length; i++){
        lwm2m_printf("%02x", encoderBuffer[i]);
    }
    LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
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
int senml_cbor_serialize(lwm2m_uri_t * uriP, 
                         int size,
                         const lwm2m_data_t * dataP, 
                         uint8_t ** bufferP)
{
    size_t length = 0;
    *bufferP = NULL;
    size_t bufferSize = DEFAULT_BUFF_SIZE; /// TODO Add the ability to calculate the required memory for the cbor buffer
    uint8_t encoderBuffer[DEFAULT_BUFF_SIZE];

    LOG("CBOR cbor_serialize ");
    LOG_ARG("size: %d", size);
    LOG_URI(uriP);
        
    if ( (size == 1) && (uriP->resourceId != 65535) && (dataP->type != LWM2M_TYPE_MULTIPLE_RESOURCE))
    {
        length = senml_cbor_encodeValue(dataP, encoderBuffer, &bufferSize);
    }
    else if (uriP != NULL && LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP)){
        size = dataP->value.asChildren.count;
        dataP = dataP->value.asChildren.array;
        if(size != 1) return -1;
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

        arraySize = get_array_items_count(dataP, size);

        cbor_encoder_init(&encoder, encoderBuffer, DEFAULT_BUFF_SIZE, 0);

        err = cbor_encoder_create_array(&encoder, &encoder, arraySize);/// Opening array once
        if (err != CborNoError) 
        {
            LOG_ARG("cbor_encoder_create_array FAILED err=%d", err);
            return -1;
        }

        err = prv_serialize(&encoder, uriP, dataP, size); /// recoursive call
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
        length = cbor_encoder_get_buffer_size(&encoder, encoderBuffer);/// getting the final length of the encoded buffer
    }///!else

    return putEncodedIntoBuffer(bufferP, length, encoderBuffer);
}

