/**
 * TODO: Add license description
 */

#include "internals.h"
#include <stdio.h>

#if defined(LWM2M_CLIENT_MODE) && defined(LWM2M_SUPPORT_SENML_JSON)

static uint8_t create_payload(lwm2m_context_t * contextP, lwm2m_uri_t * uriP, uint8_t ** bufferP, size_t * lengthP)
{
    uint8_t result;
    lwm2m_data_t * dataP = NULL;
    int size = 0;
    int res;
    lwm2m_media_type_t formatP = LWM2M_CONTENT_SENML_JSON;

    LOG_URI(uriP);
    result = object_readData(contextP, NULL, uriP, &size, &dataP);
    if (result == COAP_205_CONTENT)
    {
        res = lwm2m_data_serialize(uriP, size, dataP, &formatP, bufferP);
        if (res < 0)
        {
            result = COAP_500_INTERNAL_SERVER_ERROR;
        }
        else
        {
            *lengthP = (size_t)res;
        }
    }
    lwm2m_data_free(size, dataP);

    return result;
}

bool is_server_valid_for_send(lwm2m_server_t * serverP) {
    if (serverP->status != STATE_REGISTERED && serverP->status != STATE_REG_UPDATE_NEEDED &&
        serverP->status != STATE_REG_LT_UPDATE_NEEDED && serverP->status != STATE_REG_OBJ_UPDATE_NEEDED &&
        serverP->status != STATE_REG_FULL_UPDATE_NEEDED && serverP->status != STATE_REG_UPDATE_PENDING) {
        return false;
    }

    return !serverP->muteSend;
}

int lwm2m_send_operation(lwm2m_context_t * contextP, lwm2m_uri_t * uriP) {
    uint8_t result;
    uint8_t * payload = NULL;
    size_t payload_length = 0;
    lwm2m_transaction_t * transaction;
    lwm2m_server_t * targetP = contextP->serverList;

    if (contextP->state != STATE_READY) {
        return COAP_503_SERVICE_UNAVAILABLE;
    }

    result = create_payload(contextP, uriP, &payload, &payload_length);
    LOG_ARG("Created payload for send, result: %u, payload length: %u", result, payload_length);
    if (result != COAP_205_CONTENT) {
        return result;
    }

    while (targetP != NULL) {
        if (!is_server_valid_for_send(targetP)) {
            LOG_ARG("Server %d is not valid for send", targetP->shortID);
            targetP = targetP->next;
            continue;
        }

        transaction = transaction_new(targetP->sessionH, COAP_POST, NULL, NULL, contextP->nextMID++, 4, NULL);
        if (transaction == NULL) {
            LOG_ARG("Transaction_new failed, server: %d", targetP->shortID);
            targetP = targetP->next;
            continue;
        }

        coap_set_header_uri_path(transaction->message, "/"URI_SEND_SEGMENT);
        coap_set_header_content_type(transaction->message, LWM2M_CONTENT_SENML_JSON);

        if (!transaction_set_payload(transaction, payload, (size_t)payload_length)) {
            LOG_ARG("Transaction_set_payload failed, server: %d", targetP->shortID);
            transaction_free(transaction);
            targetP = targetP->next;
            continue;
        }

        transaction->callback = NULL;
        transaction->userData = NULL;
        contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);
        if (transaction_send(contextP, transaction) != 0) {
            LOG_ARG("Transaction_send failed, server: %d", targetP->shortID);
            targetP = targetP->next;
            continue;
        }

        targetP = targetP->next;
    }
    lwm2m_free(payload);

    return COAP_NO_ERROR;
}

void lwm2m_update_server_mute(lwm2m_context_t * contextP, uint16_t serverId, bool muteSend) {
    lwm2m_server_t *serverP = (lwm2m_server_t *)contextP->serverList;

    if(serverId == 0) return;
    
    while(serverP && serverP->shortID != serverId)
    {
        serverP = serverP->next;
    }

    if(serverP == NULL) return;
    serverP->muteSend = muteSend;
}
#endif
