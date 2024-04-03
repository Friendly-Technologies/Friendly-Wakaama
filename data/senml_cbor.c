#include "internals.h"
#include <cbor.h>

int senml_cbor_parse(lwm2m_uri_t * uriP,
              const uint8_t * buffer,
              size_t bufferLen,
              lwm2m_data_t ** dataP)
{
    size_t dataSize = 0;
    *dataP = NULL;

    LOG_ARG("bufferLen: %d", bufferLen);
    LOG_URI(uriP);
    if (uriP == NULL){
        return -1;
    }
    
    return dataSize;
}

#define DEFAULT_BUFF_SIZE (1024UL)

int senml_cbor_serialize(bool isResourceInstance, 
                   int size,
                   lwm2m_data_t *dataP,
                   uint8_t **bufferP) 
{
    int length = 0;
    *bufferP = NULL;
    
    LOG_ARG("returning %u", length);
    return length;
}

