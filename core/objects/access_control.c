/**
 * TODO: Add license description
 */

#include "internals.h"
#include <stdio.h>

// TODO: When an object has multiple instances and the server performs the LWM2M_OBJ_OP_OBSERVE operation
// on the entire object, but the server does not have access to all instances of the object, an unexpected
// behavior occurs. When changing the value of one of the instances of the object, even if this instance is
// not available for a specific server, a message about the change of value will still be generated, but only
// those instances to which it has access will be transferred to the server. What is the problem with generating
// notifications for the server even if the specific instance is not available to the server.

/**
 * @brief Get the Access Control Object Instance target object and instance identifiers 
 */
static bool prv_get_ac_target_id(lwm2m_data_t *acInstance, uint16_t *objId, uint16_t *instId) {
    int i = 0;
    int resCount = acInstance->value.asChildren.count;
    lwm2m_data_t *resources = acInstance->value.asChildren.array;
    uint8_t found = 0;

    for (i = 0; i < resCount && found != 2; i++) {
        if (resources[i].id == LWM2M_AC_RES_OBJECT_ID) {
            *objId = resources[i].value.asInteger;
            found++;
        }
        if (resources[i].id == LWM2M_AC_RES_INSTANCE_ID) {
            *instId = resources[i].value.asInteger;
            found++;
        }
    }

    return found == 2;
}

/**
 * @brief Get the Access Control Object Instance for the target object and instance identifiers
 */
static lwm2m_data_t * prv_get_ac_instance_for_target(lwm2m_data_t *acInstances, int acInstCount, uint16_t objId, uint16_t instId) {
    int i = 0;
    for (i = 0; i < acInstCount; i++) {
        uint16_t tmpObjId, tmpInstId;
        if (prv_get_ac_target_id(acInstances + i, &tmpObjId, &tmpInstId) && tmpObjId == objId && tmpInstId == instId) {
            return acInstances + i;
        }
    }
    return NULL;
}

/**
 * @brief Get the Access Control Owner of the Access Control Object Instance
 */
static uint16_t prv_get_ac_owner(lwm2m_data_t *acInstance) {
    int i = 0;
    int resCount = acInstance->value.asChildren.count;
    lwm2m_data_t *resources = acInstance->value.asChildren.array;

    for (i = 0; i < resCount; i++) {
        if (resources[i].id == LWM2M_AC_RES_OWNER_ID) return resources[i].value.asInteger;
    }
    return LWM2M_MAX_ID;

}

/**
 * @brief Get the ACL value for the target server 
 */
static int prv_get_acl_for_server(lwm2m_data_t *acInstance, lwm2m_server_t * serverP) {
    int i = 0;
    int resCount = acInstance->value.asChildren.count;
    lwm2m_data_t *resources = acInstance->value.asChildren.array;
    int aclCount = 0;
    lwm2m_data_t *aclInst = NULL;

    // Get ACL resource
    for (i = 0; i < resCount; i++) {
        if (resources[i].id == LWM2M_AC_RES_ACL_ID) {
            aclInst = resources[i].value.asChildren.array;
            aclCount = resources[i].value.asChildren.count;
            break;
        }
    }
    // Find ACL for target server
    for (i = 0; i < aclCount; i++) {
        if (aclInst[i].id == serverP->shortID) return aclInst[i].value.asInteger;
    }

    return LWM2M_AC_ACL_MAX_VALUE;
}

/**
 * @brief Get the default ACL value for the Access Control Object Instance
 */
static int prv_get_default_acl(lwm2m_data_t *acInstance) {
    int i = 0;
    int resCount = acInstance->value.asChildren.count;
    lwm2m_data_t *resources = acInstance->value.asChildren.array;
    int aclCount = 0;
    lwm2m_data_t *aclInst = NULL;

    // Get ACL resource
    for (i = 0; i < resCount; i++) {
        if (resources[i].id == LWM2M_AC_RES_ACL_ID) {
            aclInst = resources[i].value.asChildren.array;
            aclCount = resources[i].value.asChildren.count;
            break;
        }
    }
    // Find default ACL
    for (i = 0; i < aclCount; i++) {
        if (aclInst[i].id == LWM2M_AC_ACL_DEFAULT_ID) return aclInst[i].value.asInteger;
    }

    return LWM2M_AC_ACL_MAX_VALUE;
}

static bool prv_is_operation_compatible_with_acl(int aclValue, lwm2m_obj_operation_t operation) {
    switch (operation) {
    case LWM2M_OBJ_OP_READ: 
    case LWM2M_OBJ_OP_OBSERVE:
    case LWM2M_OBJ_OP_WRITE_ATTRIBUTES:
        return (LWM2M_AC_READ_OP & aclValue);
    case LWM2M_OBJ_OP_WRITE :
        return (LWM2M_AC_WRITE_OP & aclValue);
    case LWM2M_OBJ_OP_EXECUTE:
        return (LWM2M_AC_EXECUTE_OP & aclValue);
    case LWM2M_OBJ_OP_CREATE:
        return (LWM2M_AC_CREATE_OP & aclValue);
    case LWM2M_OBJ_OP_DELETE:
        return (LWM2M_AC_DELETE_OP & aclValue);
    default:
        break;
    }
    return false;
}

static bool prv_is_server_has_permission(lwm2m_data_t *acInstance, lwm2m_server_t * serverP, lwm2m_obj_operation_t operation) {
    uint16_t aclOwner = prv_get_ac_owner(acInstance);
    int aclValue = prv_get_acl_for_server(acInstance, serverP); 
    int defaultAclValue = prv_get_default_acl(acInstance);

    // If the LwM2M Server is declared as the Access Control Owner of this Object Instance and there is no ACL
    // Resource Instance for that LwM2M Server, then the LwM2M Client gets full access right.
    if (aclOwner == serverP->shortID && aclValue == LWM2M_AC_ACL_MAX_VALUE) return true;

    // If the LwM2M Client has an ACL Resource Instance for the LwM2M Server, the LwM2M Client gets the access
    // right from that ACL Resource Instance.
    if (aclValue != LWM2M_AC_ACL_MAX_VALUE) return prv_is_operation_compatible_with_acl(aclValue, operation);

    // If the LwM2M Server is not declared as the Access Control Owner of this Object Instance and the LwM2M
    // Client does not have the ACL Resource Instance for that Server, the LwM2M Client gets the access right from
    // the ACL Resource Instance (ID:0) containing the default access rights if it exists
    if (defaultAclValue != LWM2M_AC_ACL_MAX_VALUE) return prv_is_operation_compatible_with_acl(defaultAclValue, operation);

    // If the LwM2M Client does not have the ACL Resource Instance ID:0 containing the default access rights, then
    // the LwM2M Server has no access rights.
    return false;
}

/**
 * @brief Check if Access Control Object Instance operation is authorized
 * It is different from the other operations because it is related to the Access Control Object itself
 * and in this case the server has the right to access the instance only if it is its owner.
 */
static bool prv_is_ac_instance_operation_authorized(lwm2m_data_t *acInstances, int acInstCount, lwm2m_server_t * serverP, uint16_t acInstId, lwm2m_obj_operation_t operation) {
    lwm2m_data_t *acInstance = acInstances;
    uint16_t aclOwner;
    if (operation == LWM2M_OBJ_OP_CREATE) {
        // This kind of Access Control Object Instance associated with a certain Object, MUST only be created or updated during a Bootstrap Phase.
        if (acInstId == LWM2M_MAX_ID) return false;
        // Get the Access Control Object Instance for the target object
        lwm2m_data_t *acForObj = prv_get_ac_instance_for_target(acInstances, acInstCount, LWM2M_AC_OBJECT_ID, LWM2M_MAX_ID);
        if (acForObj == NULL) return false;
        return prv_is_server_has_permission(acForObj, serverP, operation);
    } else if (acInstId == LWM2M_MAX_ID) {
        // If the Access Control Object Instance ID is not set, then the operation related to the Access Control Object
        if (operation != LWM2M_OBJ_OP_READ && operation != LWM2M_OBJ_OP_WRITE_ATTRIBUTES && operation != LWM2M_OBJ_OP_OBSERVE) return false;
        while (acInstance != (acInstances + acInstCount)) {
            aclOwner = prv_get_ac_owner(acInstance);
            if (aclOwner == serverP->shortID) return true;
            acInstance++;
        }
        return false;
    } else {
        // If the Access Control Object Instance ID is set, then the operation is related to a specific Access Control Object Instance
        // Find the Access Control Object Instance
        while (acInstance != (acInstances + acInstCount)) {
            if (acInstance->id == acInstId) break;
            acInstance++;
        }
        if (acInstance == (acInstances + acInstCount)) return false;
        // Through the Device Management and Service Enablement Interface, an Access Control Object Instance MUST only
        // be managed by the LwM2M Server declared as the "Access Control Owner" in it.
        aclOwner = prv_get_ac_owner(acInstance);
        return aclOwner == serverP->shortID;
    }
}

static bool prv_is_object_operation_authorized(lwm2m_data_t *acInstances, int acInstCount, lwm2m_server_t * serverP, uint16_t objId, lwm2m_obj_operation_t operation) {
    if (operation == LWM2M_OBJ_OP_CREATE) {
        lwm2m_data_t *acForObj = prv_get_ac_instance_for_target(acInstances, acInstCount, objId, LWM2M_MAX_ID);
        if (acForObj == NULL) return false;
        return prv_is_server_has_permission(acForObj, serverP, operation);
    } else if (operation == LWM2M_OBJ_OP_READ || operation == LWM2M_OBJ_OP_WRITE_ATTRIBUTES || operation == LWM2M_OBJ_OP_OBSERVE) {
        for (int i = 0; i < acInstCount; i++) {
            uint16_t tmpObjId, tmpInstId;
            // Find the Access Control Object Instance for the target object instance
            if (!prv_get_ac_target_id(acInstances + i, &tmpObjId, &tmpInstId) || tmpObjId != objId || tmpInstId == LWM2M_MAX_ID) continue;
            if (prv_is_server_has_permission(acInstances + i, serverP, operation)) return true;
        }
    }
    return false;
}

static bool prv_is_instance_operation_authorized(lwm2m_data_t *acInstances, int acInstCount, lwm2m_server_t * serverP, lwm2m_uri_t *uriP, lwm2m_obj_operation_t operation) {
    lwm2m_data_t *acForTarget = prv_get_ac_instance_for_target(acInstances, acInstCount, uriP->objectId, uriP->instanceId);
    if (acForTarget == NULL) return false;
    return prv_is_server_has_permission(acForTarget, serverP, operation);
}

static int prv_get_supported_ac_instances(lwm2m_data_t *acInstances, int acInstCount, lwm2m_server_t * serverP, lwm2m_list_t **objInstList) {
    lwm2m_data_t *acInstance = acInstances;
    uint16_t aclOwner;

    while (acInstance != (acInstances + acInstCount)) {
        aclOwner = prv_get_ac_owner(acInstance);
        if (aclOwner == serverP->shortID) {
            lwm2m_list_t *tmpNodeP = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
            if (tmpNodeP == NULL) {
                LWM2M_LIST_FREE(*objInstList);
                *objInstList = NULL;
                return COAP_500_INTERNAL_SERVER_ERROR;
            }
            tmpNodeP->id = acInstance->id;
            tmpNodeP->next = NULL;
            *objInstList = (lwm2m_list_t *)LWM2M_LIST_ADD(*objInstList, tmpNodeP);
        }
        acInstance++;
    }

    return *objInstList? COAP_205_CONTENT : COAP_401_UNAUTHORIZED;
}

static int prv_get_supported_target_instances(lwm2m_data_t *acInstances, int acInstCount, lwm2m_server_t * serverP, uint16_t objId, lwm2m_obj_operation_t operation, lwm2m_list_t **objInstList) {
    // Fill the list of instances that support the operation
    for (int i = 0; i < acInstCount; i++) {
        uint16_t tmpObjId, tmpInstId;
        // Find the Access Control Object Instance for the target object instance
        if (!prv_get_ac_target_id(acInstances + i, &tmpObjId, &tmpInstId) || tmpObjId != objId || 
            (operation == LWM2M_OBJ_OP_CREATE && tmpInstId != LWM2M_MAX_ID) ||
            (operation != LWM2M_OBJ_OP_CREATE && tmpInstId == LWM2M_MAX_ID)) continue;
        // Check if the server has permission for the operation and add the instance to the list
        if (prv_is_server_has_permission(acInstances + i, serverP, operation)) {
            lwm2m_list_t *tmpNodeP = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
            if (tmpNodeP == NULL) {
                LWM2M_LIST_FREE(*objInstList);
                *objInstList = NULL;
                return COAP_500_INTERNAL_SERVER_ERROR;
            }
            tmpNodeP->id = tmpInstId;
            tmpNodeP->next = NULL;
            *objInstList = (lwm2m_list_t *)LWM2M_LIST_ADD(*objInstList, tmpNodeP);
        }
    }

    return *objInstList? COAP_205_CONTENT : COAP_401_UNAUTHORIZED;
}

/**
 * @brief Check if Access Control Object is enabled
 */
bool ac_is_enabled(lwm2m_context_t * contextP, lwm2m_server_t * serverP) {
    lwm2m_object_t * acObjP = (lwm2m_object_t *)LWM2M_LIST_FIND(contextP->objectList, LWM2M_AC_OBJECT_ID);
    // In bootstrap mode, the bootstrap server can read all object instances,
    // also core has the same ability, otherwise, the server can read only those
    // instances of the object for which it has rights. 
    return contextP->state != STATE_BOOTSTRAPPING && serverP != NULL && acObjP != NULL && acObjP->instanceList != NULL;
}

/**
 * Object operations (uri - /objId/LWM2M_MAX_ID/LWM2M_MAX_ID/LWM2M_MAX_ID):
 *  - LWM2M_OBJ_OP_CREATE. The absence of Access Control Object Instance association for a certain Object, prevents this Object to be instantiated by any LwM2M Server.
 *  - LWM2M_OBJ_OP_READ. If the object has at least one instance with read permission for a specific server, the operation will be allowed.
 *  - LWM2M_OBJ_OP_WRITE_ATTRIBUTES. For the "Write-Attributes" operation, the LwM2M Client MUST perform the operation.
 *  - LWM2M_OBJ_OP_OBSERVE. If the object has at least one instance with read permission for a specific server, the operation will be allowed.
 *  - LWM2M_OBJ_OP_DISCOVER. No access rights are needed for the "Discover" operation.
 * Instance/Resource operations (uri - /objId/instId/resId/resInstId):
 *  - LWM2M_OBJ_OP_READ.
 *  - LWM2M_OBJ_OP_WRITE.
 *  - LWM2M_OBJ_OP_WRITE_ATTRIBUTES.
 *  - LWM2M_OBJ_OP_EXECUTE.
 *  - LWM2M_OBJ_OP_DELETE.
 *  - LWM2M_OBJ_OP_OBSERVE.
 *  - LWM2M_OBJ_OP_DISCOVER.
 */
bool ac_is_operation_authorized(lwm2m_context_t * contextP, lwm2m_server_t * serverP, lwm2m_uri_t *uriP, lwm2m_obj_operation_t operation) {
    lwm2m_data_t *acInstances = NULL;
    lwm2m_uri_t aclUri = {LWM2M_AC_OBJECT_ID, LWM2M_MAX_ID, LWM2M_MAX_ID, LWM2M_MAX_ID};
    int acInstCount = 0;
    bool result = false;

    LOG_ARG("Checking if operation %d is authorized for %d/%d/%d/%d, server: %d", operation, uriP->objectId, uriP->instanceId, uriP->resourceId, uriP->resourceInstanceId, ((serverP)? serverP->shortID : LWM2M_MAX_ID));

    // Validate input parameters
    if (!contextP || !uriP || uriP->objectId == LWM2M_MAX_ID || operation == LWM2M_OBJ_OP_UNKNOWN) return false;
    // If Access Control Object is not enabled, then the operation is always allowed
    if (!ac_is_enabled(contextP, serverP)) return true;

    // Discover operation is always allowed
    if (operation == LWM2M_OBJ_OP_DISCOVER) return true;

    // Get Access Control Object instances
    object_readData(contextP, NULL, &aclUri, &acInstCount, &acInstances);
    if (acInstCount == 0) return false;
    
    // Check if the operation is authorized
    if (uriP->objectId == LWM2M_AC_OBJECT_ID) result = prv_is_ac_instance_operation_authorized(acInstances, acInstCount, serverP, uriP->instanceId, operation);
    else if (!LWM2M_URI_IS_SET_INSTANCE(uriP)) result = prv_is_object_operation_authorized(acInstances, acInstCount, serverP, uriP->objectId, operation);
    else result = prv_is_instance_operation_authorized(acInstances, acInstCount, serverP, uriP, operation);
    lwm2m_data_free(acInstCount, acInstances);

    LOG_ARG("Operation %d is %s for %d/%d/%d/%d, server: %d", operation, result ? "authorized" : "not authorized", uriP->objectId, uriP->instanceId, uriP->resourceId, uriP->resourceInstanceId, (serverP)? serverP->shortID : LWM2M_MAX_ID);

    return result;
}

/**
 * @brief Create Access Control Object Instance for the target object.
 * Creates an AC instance for the corresponding object instance and assigns the server as its owner. 
 */
int ac_create_instance(lwm2m_context_t * contextP, lwm2m_server_t * serverP, lwm2m_uri_t * uriP) {
    lwm2m_object_t * targetP;
    uint16_t objInstId;
    lwm2m_data_t *resourcesData, *aclData;
    uint8_t result;

    LOG_ARG("Creating Access Control Object Instance for %d/%d/%d/%d", uriP->objectId, uriP->instanceId);

    // Validate input parameters
    if (!contextP || !serverP || !uriP) return COAP_400_BAD_REQUEST;
    // We should not create Access Control Object Instance for Access Control Object
    if (uriP->objectId == LWM2M_AC_OBJECT_ID) return COAP_201_CREATED;

    // Getting the target object
    targetP = (lwm2m_object_t *)LWM2M_LIST_FIND(contextP->objectList, LWM2M_AC_OBJECT_ID);
    if (NULL == targetP) return COAP_404_NOT_FOUND;
    if (NULL == targetP->createFunc) return COAP_405_METHOD_NOT_ALLOWED;

    // Prepare the Access Control Object Instance data
    resourcesData = (lwm2m_data_t *)lwm2m_malloc(LWM2M_AC_RES_CNT * sizeof(lwm2m_data_t));
    if (NULL == resourcesData) return COAP_500_INTERNAL_SERVER_ERROR;
    aclData = (lwm2m_data_t *)lwm2m_malloc(sizeof(lwm2m_data_t));
    if (NULL == aclData) {
        lwm2m_free(resourcesData);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    // Fill object id
    resourcesData[0].id = LWM2M_AC_RES_OBJECT_ID;
    resourcesData[0].type = LWM2M_TYPE_INTEGER;
    resourcesData[0].value.asInteger = uriP->objectId;
    // Fill instance id
    resourcesData[1].id = LWM2M_AC_RES_INSTANCE_ID;
    resourcesData[1].type = LWM2M_TYPE_INTEGER;
    resourcesData[1].value.asInteger = uriP->instanceId;
    // Fill ACL resource
    aclData->id = LWM2M_AC_ACL_DEFAULT_ID;
    aclData->type = LWM2M_TYPE_INTEGER;
    aclData->value.asInteger = LWM2M_AC_NO_ACCESS;
    resourcesData[2].id = LWM2M_AC_RES_ACL_ID;
    resourcesData[2].type = LWM2M_TYPE_MULTIPLE_RESOURCE;
    resourcesData[2].value.asChildren.count = 1;
    resourcesData[2].value.asChildren.array = aclData;
    // Fill owner id
    resourcesData[3].id = LWM2M_AC_RES_OWNER_ID;
    resourcesData[3].type = LWM2M_TYPE_INTEGER;
    resourcesData[3].value.asInteger = serverP? serverP->shortID : LWM2M_MAX_ID;

    objInstId = lwm2m_list_newId(targetP->instanceList);
    result = targetP->createFunc(contextP, NULL, objInstId, LWM2M_AC_RES_CNT, resourcesData, targetP);

    lwm2m_free(resourcesData);
    lwm2m_free(aclData);

    LOG_ARG("Result of creating Access Control Object Instance for %d/%d/%d/%d is %d", uriP->objectId, uriP->instanceId, uriP->resourceId, uriP->resourceInstanceId, result);

    return result;
}

/**
 * @brief Delete Access Control Object Instance for the target object. 
 */
int ac_delete_instance(lwm2m_context_t * contextP, lwm2m_uri_t * uriP) {
    lwm2m_object_t * acObjectP;
    lwm2m_data_t *acInstances, *acInstance;
    lwm2m_uri_t aclUri = {LWM2M_AC_OBJECT_ID, LWM2M_MAX_ID, LWM2M_MAX_ID, LWM2M_MAX_ID};
    int acInstCount = 0;
    uint8_t result = COAP_202_DELETED;

    LOG_ARG("Deleting Access Control Object Instance for %d/%d/%d/%d", uriP->objectId, uriP->instanceId, uriP->resourceId, uriP->resourceInstanceId);

    // Validate input parameters
    if (!contextP || !uriP || !LWM2M_URI_IS_SET_INSTANCE(uriP)) return COAP_400_BAD_REQUEST;
    // We should not delete Access Control Object Instance for Access Control Object
    if (uriP->objectId == LWM2M_AC_OBJECT_ID) return COAP_202_DELETED;

    acObjectP = (lwm2m_object_t *)LWM2M_LIST_FIND(contextP->objectList, LWM2M_AC_OBJECT_ID);
    if (NULL == acObjectP) return COAP_404_NOT_FOUND;
    if (NULL == acObjectP->deleteFunc) return COAP_405_METHOD_NOT_ALLOWED;

    // Get Access Control Object instances
    result = object_readData(contextP, NULL, &aclUri, &acInstCount, &acInstances);
    if (acInstCount == 0) return result;

    // Find the Access Control Object Instance
    acInstance = prv_get_ac_instance_for_target(acInstances, acInstCount, uriP->objectId, uriP->instanceId);
    if (acInstance == NULL) {
        lwm2m_data_free(acInstCount, acInstances);
        return result;
    }

    // Delete the Access Control Object Instance
    result = acObjectP->deleteFunc(contextP, NULL, acInstance->id, acObjectP);
    aclUri.instanceId = acInstance->id;
    if (result == COAP_202_DELETED) observe_clear(contextP, &aclUri);

    LOG_ARG("Result of deleting Access Control Object Instance for %d/%d/%d/%d is %d", uriP->objectId, uriP->instanceId, uriP->resourceId, uriP->resourceInstanceId, result);

    return result;
}

/**
 * @brief Returns a list of instances of a specific object that support the specified operation for the specified server.
 * Returned list should be freed by the caller.
 */
int ac_get_instances_with_support_operation(lwm2m_context_t * contextP, lwm2m_server_t * serverP, lwm2m_object_t * targetObjP, lwm2m_obj_operation_t operation, lwm2m_list_t **objInstList) {
    lwm2m_uri_t aclUri = {LWM2M_AC_OBJECT_ID, LWM2M_MAX_ID, LWM2M_MAX_ID, LWM2M_MAX_ID};
    lwm2m_data_t *acInstances = NULL;
    int acInstCount = 0;
    int result = COAP_205_CONTENT;

    LOG_ARG("Getting instances with support operation %d for object %d", operation, targetObjP->objID);

    // Validate input parameters
    if (!contextP || !targetObjP || operation == LWM2M_OBJ_OP_UNKNOWN || !objInstList) return COAP_400_BAD_REQUEST;
    *objInstList = NULL;

    // If Access Control Object is not enabled, then the operation is always allowed
    // Discover operation is always allowed
    if (!ac_is_enabled(contextP, serverP) || operation == LWM2M_OBJ_OP_DISCOVER) {
        lwm2m_list_t *tmpListP = targetObjP->instanceList;
        while (tmpListP != NULL) {
            lwm2m_list_t *tmpNodeP = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
            if (tmpNodeP == NULL) {
                LWM2M_LIST_FREE(*objInstList);
                *objInstList = NULL;
                return COAP_500_INTERNAL_SERVER_ERROR;
            }
            tmpNodeP->id = tmpListP->id;
            tmpNodeP->next = NULL;
            *objInstList = (lwm2m_list_t *)LWM2M_LIST_ADD(*objInstList, tmpNodeP);
            tmpListP = tmpListP->next;
        }
        return COAP_205_CONTENT;
    }

    // Get Access Control Object instances
    object_readData(contextP, NULL, &aclUri, &acInstCount, &acInstances);
    if (acInstCount == 0) return COAP_500_INTERNAL_SERVER_ERROR;
    
    // We have different logic for Access Control Object and other objects
    if (targetObjP->objID == LWM2M_AC_OBJECT_ID) result = prv_get_supported_ac_instances(acInstances, acInstCount, serverP, objInstList);
    else result = prv_get_supported_target_instances(acInstances, acInstCount, serverP, targetObjP->objID, operation, objInstList);

    return result;
}
