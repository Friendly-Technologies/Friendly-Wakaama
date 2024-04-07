/**
 * TODO: Add license description
 */

#include "internals.h"
#include <stdio.h>

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
            return acInstances[i].value.asChildren.array;
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

static bool prv_is_object_operation_authorized(lwm2m_data_t *acInstances, int acInstCount, lwm2m_server_t * serverP, uint16_t objId, lwm2m_obj_operation_t operation) {
    if (operation == LWM2M_OBJ_OP_CREATE) {
        lwm2m_data_t *acForObj = prv_get_ac_instance_for_target(acInstances, acInstCount, objId, LWM2M_MAX_ID);
        if (acForObj == NULL) return false;
        return prv_is_server_has_permission(acForObj, serverP, operation);
    } else if (operation == LWM2M_OBJ_OP_READ || operation == LWM2M_OBJ_OP_WRITE_ATTRIBUTES || operation == LWM2M_OBJ_OP_OBSERVE) {
        for (int i = 0; i < acInstCount; i++) {
            uint16_t tmpObjId, tmpInstId;
            if (!prv_get_ac_target_id(acInstances + i, &tmpObjId, &tmpInstId) || tmpObjId != objId) continue;
            if (prv_is_server_has_permission(acInstances + i, serverP, operation)) return true;
        }
    }
    return false;
}

static bool prv_is_instance_operation_authorized(lwm2m_data_t *acInstances, int acInstCount, lwm2m_server_t * serverP, lwm2m_uri_t *uriP, lwm2m_obj_operation_t operation) {
    lwm2m_data_t *acForTarget = prv_get_ac_instance_for_target(acInstances, acInstCount, uriP->objectId, uriP->instanceId);
    if (acForTarget == NULL) return false;
    return prv_is_server_has_permission(acForTarget, serverP, operation);
    return true;
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

    // Validate input parameters
    if (!serverP || !uriP || uriP->objectId == LWM2M_MAX_ID || operation == LWM2M_OBJ_OP_UNKNOWN) return false;

    // Discover operation is always allowed
    if (operation == LWM2M_OBJ_OP_DISCOVER) return true;

    // Get Access Control Object instances
    object_readData(contextP, serverP, &aclUri, &acInstCount, &acInstances);
    // If there is no Access Control Object or its instances, then ACL is not enabled
    if (acInstCount == 0) return true;
    
    // Check if the operation is authorized
    if (!LWM2M_URI_IS_SET_INSTANCE(uriP)) result = prv_is_object_operation_authorized(acInstances, acInstCount, serverP, uriP->instanceId, operation);
    else result = prv_is_instance_operation_authorized(acInstances, acInstCount, serverP, uriP, operation);
    lwm2m_data_free(acInstCount, acInstances);

    return result;
}
