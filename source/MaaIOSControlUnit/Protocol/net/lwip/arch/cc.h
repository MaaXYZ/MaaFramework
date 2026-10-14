#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif
#include <stdint.h>
#include <stdlib.h>

#ifdef BYTE_ORDER
#undef BYTE_ORDER
#endif
#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define BYTE_ORDER BIG_ENDIAN
#else
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#ifdef __cplusplus
extern "C"
{
#endif
    uint32_t maa_ios_lwip_random(void);
#ifdef __cplusplus
}
#endif
#define LWIP_RAND() maa_ios_lwip_random()
