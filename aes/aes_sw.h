#ifndef __AES_SW_H__
#define __AES_SW_H__

#include <stdint.h>

#ifdef  __cplusplus
extern "C"
{
#endif

struct aes_context
{
    int nr;
    uint32_t erk[64];
    uint32_t drk[64];
	char mode[16];
};

int  aes_set_key( struct aes_context *ctx, uint8_t *key, int nbits, const char *mode);
void aes_encrypt( struct aes_context *ctx, uint8_t iv[16], uint8_t* data, uint32_t length);
void aes_decrypt( struct aes_context *ctx, uint8_t iv[16], uint8_t* data, uint32_t length);

#ifdef  __cplusplus
}
#endif

#endif  // end of __AES_SW_H__
