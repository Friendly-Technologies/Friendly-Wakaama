#include <cbor.h>

#ifdef LWM2M_SUPPORT_CBOR


int cbor_parse(const uint8_t * buffer,
              size_t bufferLen,
              lwm2m_data_t ** dataP)
{

}

int cbor_serialize(bool isResourceInstance, 
                  int size,
                  lwm2m_data_t * dataP,
                  uint8_t ** bufferP)
{

}

#endif