#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "aes_cryptodev.h"

static bool aes_cryptodev_crypt(struct aes_cryptodev_ctx *ctx, const uint8_t iv[16], const uint8_t *in, uint8_t *out, size_t size, __u16 op);

bool aes_cryptodev_init(struct aes_cryptodev_ctx *ctx, const uint8_t *key, unsigned int key_size, const char *mode)
{
	if (! ctx) {
		printf("[AES CryptoDev Lib] Null context\n");
		return false;
	}
	
	bool is_ok = true;
	
	do {
		memset(ctx, 0x00, sizeof (struct aes_cryptodev_ctx));
	
		ctx->fd = open("/dev/crypto", O_RDWR);
		if (ctx->fd == -1) {
			printf("[AES CryptoDev Lib] Open /dev/crypto failed: %s\n", strerror(errno));
			is_ok = false;
			break;
		}
		
		if (! strncasecmp(mode, "ecb", 3)) {
			ctx->sess.cipher = CRYPTO_AES_ECB;
		}
		else if (! strncasecmp(mode, "cbc", 3)) {
			ctx->sess.cipher = CRYPTO_AES_CBC;
		}
		else {
			printf("[AES CryptoDev Lib] Unsupported mode of operation: %s\n", mode);
			is_ok = false;
			break;
		}		
		ctx->sess.keylen = key_size;
		ctx->sess.key = (__u8 *) key;
		if (ioctl(ctx->fd, CIOCGSESSION, &ctx->sess)) {
			printf("[AES CryptoDev Lib] CIOCGSESSION failed: %s\n", strerror(errno));
			is_ok = false;
			break;
		}
		ctx->sess_created = true;
		
		struct session_info_op siop;
		memset(&siop, 0, sizeof(siop));
		siop.ses = ctx->sess.ses;
		if (ioctl(ctx->fd, CIOCGSESSINFO, &siop)) {
			printf("[AES CryptoDev Lib] CIOCGSESSINFO failed: %s\n", strerror(errno));
			is_ok = false;
			break;
		}
		printf("[AES CryptoDev Lib] Got %s with driver %s\n", siop.cipher_info.cra_name, siop.cipher_info.cra_driver_name);
		ctx->alignmask = siop.alignmask;
	}
	while (0);
	
	if (! is_ok) {
		aes_cryptodev_fini(ctx);
	}
	
	return is_ok;
}

void aes_cryptodev_fini(struct aes_cryptodev_ctx *ctx)
{
	if (! ctx) {
		return;
	}
	
	if (ctx->fd >= 0) {
		if (ctx->sess_created) {
			if (ioctl(ctx->fd, CIOCFSESSION, &ctx->sess.ses)) {
				perror("ioctl(CIOCFSESSION)");
			}
			ctx->sess_created = false;
		}
	
		close(ctx->fd);
		ctx->fd = -1;
	}
}

bool aes_cryptodev_encrypt(struct aes_cryptodev_ctx *ctx, const uint8_t iv[16], const uint8_t *plaintext, uint8_t *ciphertext, size_t size)
{	
	return aes_cryptodev_crypt(ctx, iv, plaintext, ciphertext, size, COP_ENCRYPT);
}

bool aes_cryptodev_decrypt(struct aes_cryptodev_ctx *ctx, const uint8_t iv[16], const uint8_t *ciphertext, uint8_t *plaintext, size_t size)
{
	return aes_cryptodev_crypt(ctx, iv, ciphertext, plaintext, size, COP_DECRYPT);
}

bool aes_cryptodev_crypt(struct aes_cryptodev_ctx *ctx, const uint8_t iv[16], const uint8_t *in, uint8_t *out, size_t size, __u16 op)
{
	if (! ctx) {
		printf("[AES CryptoDev Lib] Null context\n");
		return false;
	}
	
	// Check plaintext and ciphertext alignment.
	if (ctx->alignmask) {
		if (((unsigned long) in) & ctx->alignmask) {
			printf("[AES CryptoDev Lib] Not %d-aligned text(0x%08x)\n", ctx->alignmask + 1, in);
			return false;
		}
		
		if (((unsigned long) out) & ctx->alignmask) {
			printf("[AES CryptoDev Lib] Not %d-aligned text(0x%08x)\n", ctx->alignmask + 1, out);
			return false;
		}
	}

	struct crypt_op cryp;
	memset(&cryp, 0, sizeof(cryp));
	cryp.ses = ctx->sess.ses;
	cryp.len = size;
	cryp.src = (__u8 *) in;
	cryp.dst = out;
	cryp.iv = (__u8 *) iv;
	cryp.op = op;
	if (ioctl(ctx->fd, CIOCCRYPT, &cryp)) {
		printf("[AES CryptoDev Lib] CIOCCRYPT failed: %s\n", strerror(errno));
		return false;
	}

	return true;
}
