#ifndef __AES_CRYPTODEV_H__
#define __AES_CRYPTODEV_H__

#include <stdint.h>
#include "cryptodev.h"

#ifdef  __cplusplus
extern "C"
{
#endif

struct aes_cryptodev_ctx
{
	int					fd;
	struct session_op 	sess;
	bool				sess_created;
	uint16_t 			alignmask;
};

bool aes_cryptodev_init(struct aes_cryptodev_ctx *ctx, const uint8_t *key, unsigned int key_size, const char *mode);
void aes_cryptodev_fini(struct aes_cryptodev_ctx *ctx);
bool aes_cryptodev_encrypt(struct aes_cryptodev_ctx *ctx, const uint8_t iv[16], const uint8_t *plaintext, uint8_t *ciphertext, size_t size);
bool aes_cryptodev_decrypt(struct aes_cryptodev_ctx *ctx, const uint8_t iv[16], const uint8_t *ciphertext, uint8_t *plaintext, size_t size);

#ifdef  __cplusplus
}
#endif	// #ifndef __AES_CRYPTODEV_H__

#endif  // end of __AES_HW_H__
