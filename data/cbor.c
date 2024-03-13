#include "internals.h"
#include <cbor.h>

int cbor_parse(const uint8_t * buffer,
              size_t bufferLen,
              lwm2m_data_t ** dataP)
{
    return 0;
}

int cbor_serialize(bool isResourceInstance, 
                  int size,
                  lwm2m_data_t * dataP,
                  uint8_t ** bufferP)
{
    return 0;
}
