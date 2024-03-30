/**
 * TODO: Add license description
 */

#include "internals.h"
#include <stdio.h>

#ifdef LWM2M_CLIENT_MODE
int lwm2m_send_operation(lwm2m_context_t * contextP, lwm2m_uri_t * uriP) {
    return COAP_NO_ERROR;
}
#endif
