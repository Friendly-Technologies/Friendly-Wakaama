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
CborError prv_parse_value(CborValue* valueP, uint64_t mapValP, lwm2m_data_t *dataP)
{
    CborError err;

    switch (mapValP)
    {
        case SENML_CBOR_MAP_TIME:
            err = cbor_value_get_int64_checked(valueP, &dataP->value.asInteger);
            if (err != CborNoError)
            {
                LOG_ARG("cbor_value_get_int64_checked FAILED with err=%d\n", err);
                return -1;
            }
            dataP->type = LWM2M_TYPE_TIME; /// SENML-CBOR does not need the tag-time, it has time-map-key instead 
            break;
        case SENML_CBOR_MAP_VALUE:
            if (cbor_value_is_unsigned_integer(valueP)){
                err = cbor_value_get_uint64(valueP, &dataP->value.asUnsigned);
                if (err != CborNoError)
                {
                    LOG_ARG("cbor_value_get_uint64 FAILED with error %d", err);
                    lwm2m_free(dataP);
                    return -1;
                }
                dataP->type = LWM2M_TYPE_UNSIGNED_INTEGER;
                break;
            }
            else if (cbor_value_is_integer(valueP)){
                err = cbor_value_get_int64_checked(valueP, &dataP->value.asInteger);
                if (err != CborNoError)
                {
                    LOG_ARG("cbor_value_get_int64_checked FAILED with error %d", err);
                    lwm2m_free(dataP);
                    return -1;
                }
                dataP->type = LWM2M_TYPE_INTEGER;
                break;
            }
            else if (cbor_value_is_double(valueP)){
                err = cbor_value_get_double(valueP, &dataP->value.asFloat);
                if (err != CborNoError)
                {
                    LOG_ARG("cbor_value_get_double FAILED with error %d", err);
                    lwm2m_free(dataP);
                    return -1;
                }
                dataP->type = LWM2M_TYPE_FLOAT;
                break;
            }
            else if (cbor_value_is_float(valueP)){
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
            else if (cbor_value_is_half_float(valueP)){
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
            break;
        case SENML_CBOR_MAP_OPAQUE:
            {
                uint8_t *temp = NULL;
                size_t temPlen = 0;
              
                err = cbor_value_calculate_string_length(valueP, &temPlen);
                if (err != CborNoError) {
                    LOG_ARG("cbor_value_calculate_string_length FAILED with error %d", err);
                    lwm2m_free(dataP);
                    return -1;
                }

                temp = (uint8_t *)lwm2m_malloc(temPlen);
                if (temp == NULL) {
                    LOG("Error: lwm2m_malloc fro temp failed\n");
                    lwm2m_free(dataP);
                    return -1;
                }

                err = cbor_value_dup_byte_string(valueP, &temp, &temPlen, valueP);
                if (err!= CborNoError){
                    LOG_ARG("cbor_value_dup_byte_string FAILED with error %d", err);
                    lwm2m_free(dataP);
                    return -1;     
                }
                dataP->value.asBuffer.buffer = (uint8_t *)temp;
                dataP->value.asBuffer.length = temPlen;
                dataP->type = LWM2M_TYPE_OPAQUE;
                break;
            }
        case SENML_CBOR_MAP_STRING_VALUE:
            {
                char *temp = NULL;
                size_t temPlen = 0;

                err = cbor_value_calculate_string_length(valueP, &temPlen);
                if (err != CborNoError) {
                    LOG_ARG("cbor_value_calculate_string_length FAILED with error %d", err);
                    lwm2m_free(dataP);
                    return -1;
                }

                temp = (char *)lwm2m_malloc(temPlen);
                if (temp == NULL) {
                    LOG("Error: Memory allocation for temp failed !!!");
                    lwm2m_free(dataP);
                    return -1;
                }

                err = cbor_value_dup_text_string(valueP, &temp, &temPlen, valueP);
                if (err!= CborNoError){
                    LOG_ARG("cbor_value_dup_text_string FAILED with error %d", err);
                    lwm2m_free(dataP);
                    return -1;
                }

                dataP->value.asBuffer.buffer = (uint8_t *)temp;
                dataP->value.asBuffer.length = temPlen;
                dataP->type = LWM2M_TYPE_STRING;
                break;
            }
        case SENML_CBOR_MAP_BOOL_VALUE:
            err = cbor_value_get_boolean(valueP, &dataP->value.asBoolean);
            if (err != CborNoError)
            {
                LOG_ARG("cbor_value_get_boolean FAILED with error %d", err);
                lwm2m_free(dataP);
                return -1;
            }
            dataP->type = LWM2M_TYPE_BOOLEAN;
            break;
        default:
            LOG_ARG("Error: Senml Cbor Wrong MapType 0x%x\n", mapValP);
            lwm2m_free(dataP);
            return -1; 
            break;
    }///!switch-case
    return err;
}

/// @brief Function that parses the resources
/// @param array 
/// @param dataP 
/// @param sizeDataP 
/// @return 
CborError prv_parse_resources(CborValue* array, lwm2m_data_t * dataP, size_t sizeDataP, uint16_t instId)
{
    CborValue map;
    CborError err;
    lwm2m_uri_t uri;
    lwm2m_data_t* newData;

    err = cbor_value_enter_container(array, &map); // Entering each map
    if (err != CborNoError)
    {
        LOG_ARG("cbor_value_enter_container map FAILED with error %d", err);
        lwm2m_free(dataP);
        return -1;
    }

    while (!cbor_value_at_end(&map)) 
    {
        err = cbor_value_advance(&map);    // Skiping the key
        if (err != CborNoError) {
            LOG_ARG("cbor_value_advance FAILED with err=%d", err);
            lwm2m_free(dataP);
            return -1;
        }  
        CborType typeMap = cbor_value_get_type(&map); // Getting the value
        if (typeMap == CborTextStringType) 
        {
            char *uriStr = NULL;
            size_t uriStrlen = 0;
            
            err = cbor_value_get_string_length(&map, &uriStrlen); /// getting uri-length
            if (err != CborNoError) {
                LOG_ARG("Error%d: cbor_value_calculate_string_length", err);
                lwm2m_free(dataP);
                return -1;
            }

            uriStr = (char *)lwm2m_malloc(uriStrlen);
            if (uriStr == NULL) {
                LOG("Error: Memory allocation for uriStr failed");
                lwm2m_free(dataP);
                return -1;
            }
            
            err = cbor_value_dup_text_string(&map, &uriStr, &uriStrlen, &map); /// getting uri itself
            if (err!= CborNoError){
                LOG_ARG("Error%d: cbor_value_dup_text_string\n", err);
                lwm2m_free(dataP);
                return -1;
            }

            int slashCount = countBackSlashes(uriStr);
            lwm2m_stringToUri(uriStr, uriStrlen, &uri );
            instId = uri.instanceId; /// here we take the instance id from the map-uri and will pass it further if needed
            newData = NULL;

            if (slashCount == 3){
                for (size_t i = 0; i < sizeDataP; i++){
                    if (dataP[i].id == MAX_URI_VAL){
                        newData = &dataP[i];
                        newData->id = uri.resourceId;
                        break;
                    }
                }
                if (newData == NULL){
                    LOG("Some error occured (slashCount==3)! newData is NULL !!! ");
                    lwm2m_free(dataP);
                }
            }///!slashCount3
            else if (slashCount == 4){
                for (size_t i = 0; i < sizeDataP; i++){
                    if (dataP[i].id == uri.resourceId){
                        newData = &dataP[i];
                        break;
                    }
                }
                if (newData == NULL){
                    for (size_t i = 0; i < sizeDataP; i++){
                        if (dataP[i].id == MAX_URI_VAL){
                            newData = &dataP[i];
                            newData->id = uri.resourceId;
                            newData->type = LWM2M_TYPE_MULTIPLE_RESOURCE;
                            break;
                        }
                    }
                    if (newData == NULL){
                        LOG("Some error occured(slashCount==4)! newData is NULL !!! ");
                        lwm2m_free(dataP);
                    }
                }
                lwm2m_data_t* tempChild = lwm2m_data_new(newData->value.asChildren.count + 1);/// copying resource
                if (tempChild == NULL) {
                    LOG("lwm2m_data_new for tempChild FAILED");
                    return -1; 
                }
                if (newData->value.asChildren.count > 0){
                    memcpy(tempChild, newData->value.asChildren.array, newData->value.asChildren.count * sizeof(newData->value.asChildren));
                    lwm2m_free(newData->value.asChildren.array);
                }
                newData->value.asChildren.count++;
                newData->value.asChildren.array = tempChild;
                newData->value.asChildren.array[newData->value.asChildren.count - 1].id = uri.resourceInstanceId;
                newData = &newData->value.asChildren.array[newData->value.asChildren.count - 1];
            }///!slashCount 4

            uint64_t valKey;
            if (cbor_value_is_unsigned_integer(&map))///getting map-key for the value
            {
                err = cbor_value_get_uint64(&map, &valKey);
                if (err != CborNoError)
                {
                    LOG_ARG("cbor_value_get_uint64 FAILED with error %d", err);
                    lwm2m_free(dataP);
                    return -1;
                }
            }
            err = cbor_value_advance_fixed(&map);
            if (err != CborNoError)
            {
                LOG_ARG("cbor_value_advance_fixed FAILED with error %d", err);
                lwm2m_free(dataP);
                return -1;
            }
            err = prv_parse_value(&map, valKey, newData);
            if (err != CborNoError)
            {
                LOG_ARG("prv_parse_value FAILED with error %d", err);
                lwm2m_free(dataP);
                return -1;
            }
        }///!typeMap
    }///!while not end map
    err = cbor_value_leave_container(array, &map); /// leave the map
    if (err != CborNoError)
    {
        LOG_ARG("cbor_value_leave_container array FAILED with error %d", err);
        lwm2m_free(dataP);
        return -1;
    }
    
    return err;
}

/// @brief Function that counts the resources in the SENML-CBOR array (if their uris are equal to 3 
/// or if there are several resources with uri==4 and same resourceID)
/// @param array 
/// @param length 
/// @return 
size_t prv_count_resources_in_array(CborValue* array, size_t length) 
{
    CborValue map;
    CborError err;
    CborValue arrayStart; ///creating a separate cbor-value struct to save the array's start
    uint16_t idsArray[length]; // Array to store IDs
    size_t numIds;
    int resourceCount = 0;

    err = cbor_value_enter_container(array, array); // Enter the CBOR array
    if (err != CborNoError)
    {
        LOG_ARG("cbor_value_enter_container array FAILED with error %d", err);
        return -1;
    }
    arrayStart = *array; // Save the starting position of the array

    // Iterate through each map in the array
    while (!cbor_value_at_end(array)) 
    {
        err = cbor_value_enter_container(array, &map); // Enter each map
        if (err != CborNoError)
        {
            LOG_ARG("cbor_value_enter_container map FAILED with error %d", err);
            return -1;
        }

        while (!cbor_value_at_end(&map)) 
        {
            err = cbor_value_advance(&map);/// Skip the key
            if (err != CborNoError) {
                LOG_ARG("cbor_value_advance FAILED with error %d", err);
                return -1;
            }  
            CborType typeMap = cbor_value_get_type(&map);// Get the value
            if (typeMap == CborTextStringType) 
            {
                size_t len;
                char *text;
                
                err = cbor_value_dup_text_string(&map, &text, &len, NULL);
                if (err != CborNoError) {
                    LOG_ARG("cbor_value_dup_text_string FAILED with error %d", err);
                    return -1;
                } 
                int backslashCount = countBackSlashes(text);
                if (backslashCount == 3) {
                    resourceCount++;
                }
                else if (backslashCount == 4){
                    lwm2m_uri_t uriFromMap;
                    lwm2m_stringToUri(text, len, &uriFromMap);
                    bool duplicateFound = false;
                    for (size_t i = 0; i < numIds; i++) {
                        if (idsArray[i] == uriFromMap.resourceId)
                        {
                            duplicateFound = true;/// if there are several resources with the same resourceID - it means they are truly resources
                            resourceCount++;
                            break;
                        }
                    }///!for
                    if (!duplicateFound && numIds < length){
                        idsArray[numIds++] = uriFromMap.resourceId;
                    }
                }
    
            }///!typeMap
            err = cbor_value_advance(&map); /// Move to the next key-value pair
            if (err != CborNoError) {
                LOG_ARG("cbor_value_advance map FAILED with error %d", err);
                return -1;
            }
        }///!while not end map
        err = cbor_value_leave_container(array, &map); /// leave the map
        if (err != CborNoError)
        {
            LOG_ARG("cbor_value_leave_container array FAILED with error %d", err);
            return -1;
        }
    }///!while not end of array
    *array = arrayStart; /// Reset the array pointer to the starting position

    return resourceCount;
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
    CborError err;
    lwm2m_data_t *inData = NULL; /// inner temporary buffer-like structure
    *dataP = NULL;               /// pointer to the final lwm2m struct
    size_t arrLen = 0, resCount = 0;
    uint16_t instId;

    if (uriP == NULL){
        LOG("UriP is NULL, some error occured!");
        return -1;
    }
    LOG_URI(uriP);
    LOG_ARG("bufferLen: %d", bufferLen);
    
    err = cbor_parser_init(buffer, bufferLen, 0, &parser, &array);    
    if (err != CborNoError)
    {
        LOG_ARG("cbor_parser_init FAILED with error %d", err);
        return -1;
    }

    CborType type = cbor_value_get_type(&array);

    if (type == CborArrayType) 
    {
        err = cbor_value_get_array_length(&array, &arrLen);
        if (err != CborNoError)
        {
            LOG_ARG("cbor_value_get_array_length FAILED with error %d", err);
            return -1;
        }
        if (arrLen <= 0) {
            LOG_ARG("wrong array length(%d) FAILED", arrLen);
            return -1;
        }

        if (arrLen > 0) 
        {
            resCount = prv_count_resources_in_array(&array, arrLen);
            inData = lwm2m_data_new(resCount);/// creating main-array-lwm2m_data struct
            if (inData == NULL) {
                LOG("lwm2m_data_new for inData FAILED");
                return -1; 
            }

            for (size_t i = 0; i < resCount; i++){
                inData[i].id = MAX_URI_VAL; /// filing the ids with the un-set value 
            }

            for (size_t i = 0; i < arrLen; i++) 
            {
                err = prv_parse_resources(&array, inData, resCount, instId);
                if (err != CborNoError)
                {
                    LOG_ARG("prv_parse_resources FAILED with error %d", err);
                    return -1;
                }
            }///!for

            if (!LWM2M_URI_IS_SET_INSTANCE(uriP)){
                lwm2m_data_t* wrapStr = lwm2m_data_new(1);
                if (wrapStr == NULL) {
                    LOG("lwm2m_data_new for wrapStr FAILED");
                    return -1; 
                }
                wrapStr->id = instId;
                wrapStr->type = LWM2M_TYPE_OBJECT_INSTANCE;
                wrapStr->value.asChildren.array = inData;
                wrapStr->value.asChildren.count = resCount;
            }
            else if (LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uriP)){
                if ((resCount != 1) || (inData->type != LWM2M_TYPE_MULTIPLE_RESOURCE)){
                    LOG("Some Error occured during !uriP->resourceInstanceId");
                    return -1;
                } 
                lwm2m_data_t *tmpDataP = inData;
                if (tmpDataP == NULL) {
                    LOG("lwm2m_data_new for tmpDataP FAILED");
                    return -1; 
                }
                resCount = tmpDataP->value.asChildren.count;
                *dataP = tmpDataP->value.asChildren.array;
                lwm2m_free(tmpDataP);
                return resCount;
            }
        }///!arrLen > 0
        else {
            LOG_ARG("Array Length is <= 0(%d), error!", arrLen);
            return -1;
        }
    }///!arrayType
    else {
        LOG_ARG("Error: type is not CborArrayType, (%d)", type);
        return -1;
    }
    
    *dataP = inData; /// copying parsed data into the final lwm2m_data struct
    return resCount;
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

