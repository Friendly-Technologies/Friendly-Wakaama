#ifndef WAKAAMA_TINYCBOR_ALLOC_H_
#define WAKAAMA_TINYCBOR_ALLOC_H_

#include <stddef.h>

void * lwm2m_malloc(size_t s);
void lwm2m_free(void * p);

#define cbor_malloc lwm2m_malloc
#define cbor_free   lwm2m_free

#endif // WAKAAMA_TINYCBOR_ALLOC_H_
